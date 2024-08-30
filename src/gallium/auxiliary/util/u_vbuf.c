   /**************************************************************************
   *
   * Copyright 2011 Marek Olšák <maraeo@gmail.com>
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * This module uploads user buffers and translates the vertex buffers which
   * contain incompatible vertices (i.e. not supported by the driver/hardware)
   * into compatible ones, based on the Gallium CAPs.
   *
   * It does not upload index buffers.
   *
   * The module heavily uses bitmasks to represent per-buffer and
   * per-vertex-element flags to avoid looping over the list of buffers just
   * to see if there's a non-zero stride, or user buffer, or unsupported format,
   * etc.
   *
   * There are 3 categories of vertex elements, which are processed separately:
   * - per-vertex attribs (stride != 0, instance_divisor == 0)
   * - instanced attribs (stride != 0, instance_divisor > 0)
   * - constant attribs (stride == 0)
   *
   * All needed uploads and translations are performed every draw command, but
   * only the subset of vertices needed for that draw command is uploaded or
   * translated. (the module never translates whole buffers)
   *
   *
   * The module consists of two main parts:
   *
   *
   * 1) Translate (u_vbuf_translate_begin/end)
   *
   * This is pretty much a vertex fetch fallback. It translates vertices from
   * one vertex buffer to another in an unused vertex buffer slot. It does
   * whatever is needed to make the vertices readable by the hardware (changes
   * vertex formats and aligns offsets and strides). The translate module is
   * used here.
   *
   * Each of the 3 categories is translated to a separate buffer.
   * Only the [min_index, max_index] range is translated. For instanced attribs,
   * the range is [start_instance, start_instance+instance_count]. For constant
   * attribs, the range is [0, 1].
   *
   *
   * 2) User buffer uploading (u_vbuf_upload_buffers)
   *
   * Only the [min_index, max_index] range is uploaded (just like Translate)
   * with a single memcpy.
   *
   * This method works best for non-indexed draw operations or indexed draw
   * operations where the [min_index, max_index] range is not being way bigger
   * than the vertex count.
   *
   * If the range is too big (e.g. one triangle with indices {0, 1, 10000}),
   * the per-vertex attribs are uploaded via the translate module, all packed
   * into one vertex buffer, and the indexed draw call is turned into
   * a non-indexed one in the process. This adds additional complexity
   * to the translate part, but it prevents bad apps from bringing your frame
   * rate down.
   *
   *
   * If there is nothing to do, it forwards every command to the driver.
   * The module also has its own CSO cache of vertex element states.
   */
      #include "util/u_vbuf.h"
      #include "util/u_dump.h"
   #include "util/format/u_format.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_prim_restart.h"
   #include "util/u_screen.h"
   #include "util/u_upload_mgr.h"
   #include "indices/u_primconvert.h"
   #include "translate/translate.h"
   #include "translate/translate_cache.h"
   #include "cso_cache/cso_cache.h"
   #include "cso_cache/cso_hash.h"
      struct u_vbuf_elements {
      unsigned count;
                     /* If (velem[i].src_format != native_format[i]), the vertex buffer
   * referenced by the vertex element cannot be used for rendering and
   * its vertex data must be translated to native_format[i]. */
   enum pipe_format native_format[PIPE_MAX_ATTRIBS];
   unsigned native_format_size[PIPE_MAX_ATTRIBS];
   unsigned component_size[PIPE_MAX_ATTRIBS];
   /* buffer-indexed */
            /* Which buffers are used by the vertex element state. */
   uint32_t used_vb_mask;
   /* This might mean two things:
   * - src_format != native_format, as discussed above.
   * - src_offset % 4 != 0 (if the caps don't allow such an offset). */
   uint32_t incompatible_elem_mask; /* each bit describes a corresp. attrib  */
   /* Which buffer has at least one vertex element referencing it
   * incompatible. */
   uint32_t incompatible_vb_mask_any;
   /* Which buffer has all vertex elements referencing it incompatible. */
   uint32_t incompatible_vb_mask_all;
   /* Which buffer has at least one vertex element referencing it
   * compatible. */
   uint32_t compatible_vb_mask_any;
   uint32_t vb_align_mask[2]; //which buffers require 2/4 byte alignments
   /* Which buffer has all vertex elements referencing it compatible. */
            /* Which buffer has at least one vertex element referencing it
   * non-instanced. */
            /* Which buffers are used by multiple vertex attribs. */
            /* Which buffer has a non-zero stride. */
            /* Which buffer is incompatible (unaligned). */
               };
      enum {
      VB_VERTEX = 0,
   VB_INSTANCE = 1,
   VB_CONST = 2,
      };
      struct u_vbuf {
      struct u_vbuf_caps caps;
            struct pipe_context *pipe;
   struct translate_cache *translate_cache;
            struct primconvert_context *pc;
            /* This is what was set in set_vertex_buffers.
   * May contain user buffers. */
   struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
                     /* Vertex buffers for the driver.
   * There are usually no user buffers. */
   struct pipe_vertex_buffer real_vertex_buffer[PIPE_MAX_ATTRIBS];
   uint32_t dirty_real_vb_mask; /* which buffers are dirty since the last
            /* Vertex elements. */
            /* Vertex elements used for the translate fallback. */
   struct cso_velems_state fallback_velems;
   /* If non-NULL, this is a vertex element state used for the translate
   * fallback and therefore used for rendering too. */
   bool using_translate;
   /* The vertex buffer slot index where translated vertices have been
   * stored in. */
   unsigned fallback_vbs[VB_NUM];
            /* Which buffer is a user buffer. */
   uint32_t user_vb_mask; /* each bit describes a corresp. buffer */
   /* Which buffer is incompatible (unaligned). */
   uint32_t incompatible_vb_mask; /* each bit describes a corresp. buffer */
   /* Which buffers are allowed (supported by hardware). */
      };
      static void *
   u_vbuf_create_vertex_elements(struct u_vbuf *mgr, unsigned count,
         static void u_vbuf_delete_vertex_elements(void *ctx, void *state,
            static const struct {
         } vbuf_format_fallbacks[] = {
      { PIPE_FORMAT_R32_FIXED,            PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R32G32_FIXED,         PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R32G32B32_FIXED,      PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R32G32B32A32_FIXED,   PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R16_FLOAT,            PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R16G16_FLOAT,         PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R16G16B16_FLOAT,      PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R16G16B16A16_FLOAT,   PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R64_FLOAT,            PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R64G64_FLOAT,         PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R64G64B64_FLOAT,      PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R64G64B64A64_FLOAT,   PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R32_UNORM,            PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R32G32_UNORM,         PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R32G32B32_UNORM,      PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R32G32B32A32_UNORM,   PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R32_SNORM,            PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R32G32_SNORM,         PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R32G32B32_SNORM,      PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R32G32B32A32_SNORM,   PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R32_USCALED,          PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R32G32_USCALED,       PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R32G32B32_USCALED,    PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R32G32B32A32_USCALED, PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R32_SSCALED,          PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R32G32_SSCALED,       PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R32G32B32_SSCALED,    PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R32G32B32A32_SSCALED, PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R16_UNORM,            PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R16G16_UNORM,         PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R16G16B16_UNORM,      PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R16G16B16A16_UNORM,   PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R16_SNORM,            PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R16G16_SNORM,         PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R16G16B16_SNORM,      PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R16G16B16_SINT,       PIPE_FORMAT_R32G32B32_SINT },
   { PIPE_FORMAT_R16G16B16_UINT,       PIPE_FORMAT_R32G32B32_UINT },
   { PIPE_FORMAT_R16G16B16A16_SNORM,   PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R16_USCALED,          PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R16G16_USCALED,       PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R16G16B16_USCALED,    PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R16G16B16A16_USCALED, PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R16_SSCALED,          PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R16G16_SSCALED,       PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R16G16B16_SSCALED,    PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R8_UNORM,             PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R8G8_UNORM,           PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R8G8B8_UNORM,         PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R8G8B8A8_UNORM,       PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R8_SNORM,             PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R8G8_SNORM,           PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R8G8B8_SNORM,         PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R8G8B8A8_SNORM,       PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R8_USCALED,           PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R8G8_USCALED,         PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R8G8B8_USCALED,       PIPE_FORMAT_R32G32B32_FLOAT },
   { PIPE_FORMAT_R8G8B8A8_USCALED,     PIPE_FORMAT_R32G32B32A32_FLOAT },
   { PIPE_FORMAT_R8_SSCALED,           PIPE_FORMAT_R32_FLOAT },
   { PIPE_FORMAT_R8G8_SSCALED,         PIPE_FORMAT_R32G32_FLOAT },
   { PIPE_FORMAT_R8G8B8_SSCALED,       PIPE_FORMAT_R32G32B32_FLOAT },
      };
      void u_vbuf_get_caps(struct pipe_screen *screen, struct u_vbuf_caps *caps,
         {
                        /* I'd rather have a bitfield of which formats are supported and a static
   * table of the translations indexed by format, but since we don't have C99
   * we can't easily make a sparsely-populated table indexed by format.  So,
   * we construct the sparse table here.
   */
   for (i = 0; i < PIPE_FORMAT_COUNT; i++)
            for (i = 0; i < ARRAY_SIZE(vbuf_format_fallbacks); i++) {
      enum pipe_format format = vbuf_format_fallbacks[i].from;
            if ((comp_bits > 32) && !needs64b)
            if (!screen->is_format_supported(screen, format, PIPE_BUFFER, 0, 0,
            caps->format_translation[format] = vbuf_format_fallbacks[i].to;
                  caps->buffer_offset_unaligned =
      !screen->get_param(screen,
      caps->buffer_stride_unaligned =
   !screen->get_param(screen,
         caps->velem_src_offset_unaligned =
      !screen->get_param(screen,
      caps->attrib_component_unaligned =
      !screen->get_param(screen,
      assert(caps->attrib_component_unaligned ||
         caps->user_vertex_buffers =
         caps->max_vertex_buffers =
            if (screen->get_param(screen, PIPE_CAP_PRIMITIVE_RESTART) ||
      screen->get_param(screen, PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX)) {
   caps->rewrite_restart_index = screen->get_param(screen, PIPE_CAP_EMULATE_NONFIXED_PRIMITIVE_RESTART);
   caps->supported_restart_modes = screen->get_param(screen, PIPE_CAP_SUPPORTED_PRIM_MODES_WITH_RESTART);
   caps->supported_restart_modes |= BITFIELD_BIT(MESA_PRIM_PATCHES);
   if (caps->supported_restart_modes != BITFIELD_MASK(MESA_PRIM_COUNT))
            }
   caps->supported_prim_modes = screen->get_param(screen, PIPE_CAP_SUPPORTED_PRIM_MODES);
   if (caps->supported_prim_modes != BITFIELD_MASK(MESA_PRIM_COUNT))
            if (!screen->is_format_supported(screen, PIPE_FORMAT_R8_UINT, PIPE_BUFFER, 0, 0, PIPE_BIND_INDEX_BUFFER))
            /* OpenGL 2.0 requires a minimum of 16 vertex buffers */
   if (caps->max_vertex_buffers < 16)
            if (!caps->buffer_offset_unaligned ||
      !caps->buffer_stride_unaligned ||
   !caps->attrib_component_unaligned ||
   !caps->velem_src_offset_unaligned)
         if (!caps->fallback_always && !caps->user_vertex_buffers)
      }
      struct u_vbuf *
   u_vbuf_create(struct pipe_context *pipe, struct u_vbuf_caps *caps)
   {
               mgr->caps = *caps;
   mgr->pipe = pipe;
   if (caps->rewrite_ubyte_ibs || caps->rewrite_restart_index ||
      /* require all but patches */
   ((caps->supported_prim_modes & caps->supported_restart_modes & BITFIELD_MASK(MESA_PRIM_COUNT))) !=
         struct primconvert_config cfg;
   cfg.fixed_prim_restart = caps->rewrite_restart_index;
   cfg.primtypes_mask = caps->supported_prim_modes;
   cfg.restart_primtypes_mask = caps->supported_restart_modes;
      }
   mgr->translate_cache = translate_cache_create();
   memset(mgr->fallback_vbs, ~0, sizeof(mgr->fallback_vbs));
            mgr->has_signed_vb_offset =
      pipe->screen->get_param(pipe->screen,
         cso_cache_init(&mgr->cso_cache, pipe);
   cso_cache_set_delete_cso_callback(&mgr->cso_cache,
               }
      /* u_vbuf uses its own caching for vertex elements, because it needs to keep
   * its own preprocessed state per vertex element CSO. */
   static struct u_vbuf_elements *
   u_vbuf_set_vertex_elements_internal(struct u_vbuf *mgr,
         {
      struct pipe_context *pipe = mgr->pipe;
   unsigned key_size, hash_key;
   struct cso_hash_iter iter;
            /* need to include the count into the stored state data too. */
   key_size = sizeof(struct pipe_vertex_element) * velems->count +
         hash_key = cso_construct_key(velems, key_size);
   iter = cso_find_state_template(&mgr->cso_cache, hash_key, CSO_VELEMENTS,
            if (cso_hash_iter_is_null(iter)) {
      struct cso_velements *cso = MALLOC_STRUCT(cso_velements);
   memcpy(&cso->state, velems, key_size);
   cso->data = u_vbuf_create_vertex_elements(mgr, velems->count,
            iter = cso_insert_state(&mgr->cso_cache, hash_key, CSO_VELEMENTS, cso);
      } else {
                           if (ve != mgr->ve)
               }
      void u_vbuf_set_vertex_elements(struct u_vbuf *mgr,
         {
         }
      void u_vbuf_set_flatshade_first(struct u_vbuf *mgr, bool flatshade_first)
   {
         }
      void u_vbuf_unset_vertex_elements(struct u_vbuf *mgr)
   {
         }
      void u_vbuf_destroy(struct u_vbuf *mgr)
   {
      struct pipe_screen *screen = mgr->pipe->screen;
   unsigned i;
   const unsigned num_vb = screen->get_shader_param(screen, PIPE_SHADER_VERTEX,
                     for (i = 0; i < PIPE_MAX_ATTRIBS; i++)
         for (i = 0; i < PIPE_MAX_ATTRIBS; i++)
            if (mgr->pc)
            translate_cache_destroy(mgr->translate_cache);
   cso_cache_delete(&mgr->cso_cache);
      }
      static enum pipe_error
   u_vbuf_translate_buffers(struct u_vbuf *mgr, struct translate_key *key,
                           const struct pipe_draw_info *info,
   {
      struct translate *tr;
   struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS] = {0};
   struct pipe_resource *out_buffer = NULL;
   uint8_t *out_map;
            /* Get a translate object. */
            /* Map buffers we want to translate. */
   mask = vb_mask;
   while (mask) {
      struct pipe_vertex_buffer *vb;
   unsigned offset;
   uint8_t *map;
   unsigned i = u_bit_scan(&mask);
            vb = &mgr->vertex_buffer[i];
            if (vb->is_user_buffer) {
         } else {
                     if (!vb->buffer.resource) {
      static uint64_t dummy_buf[4] = { 0 };
   tr->set_buffer(tr, i, dummy_buf, 0, 0);
               if (stride) {
      /* the stride cannot be used to calculate the map size of the buffer,
   * as it only determines the bytes between elements, not the size of elements
   * themselves, meaning that if stride < element_size, the mapped size will
   * be too small and conversion will overrun the map buffer
   *
   * instead, add the size of the largest possible attribute to the final attribute's offset
   * in order to ensure the map is large enough
   */
   unsigned last_offset = size - stride;
               if (offset + size > vb->buffer.resource->width0) {
      /* Don't try to map past end of buffer.  This often happens when
   * we're translating an attribute that's at offset > 0 from the
   * start of the vertex.  If we'd subtract attrib's offset from
   * the size, this probably wouldn't happen.
                  /* Also adjust num_vertices.  A common user error is to call
   * glDrawRangeElements() with incorrect 'end' argument.  The 'end
   * value should be the max index value, but people often
   * accidentally add one to this value.  This adjustment avoids
   * crashing (by reading past the end of a hardware buffer mapping)
   * when people do that.
   */
               map = pipe_buffer_map_range(mgr->pipe, vb->buffer.resource, offset, size,
               /* Subtract min_index so that indexing with the index buffer works. */
   if (unroll_indices) {
                              /* Translate. */
   if (unroll_indices) {
      struct pipe_transfer *transfer = NULL;
   const unsigned offset = draw->start * info->index_size;
            /* Create and map the output buffer. */
   u_upload_alloc(mgr->pipe->stream_uploader, 0,
                     if (!out_buffer)
            if (info->has_user_indices) {
         } else {
      map = pipe_buffer_map_range(mgr->pipe, info->index.resource, offset,
                     switch (info->index_size) {
   case 4:
      tr->run_elts(tr, (unsigned*)map, draw->count, 0, 0, out_map);
      case 2:
      tr->run_elts16(tr, (uint16_t*)map, draw->count, 0, 0, out_map);
      case 1:
      tr->run_elts8(tr, map, draw->count, 0, 0, out_map);
               if (transfer) {
            } else {
      /* Create and map the output buffer. */
   u_upload_alloc(mgr->pipe->stream_uploader,
                  mgr->has_signed_vb_offset ?
            if (!out_buffer)
                                 /* Unmap all buffers. */
   mask = vb_mask;
   while (mask) {
               if (vb_transfer[i]) {
                     /* Setup the new vertex buffer. */
            /* Move the buffer reference. */
   pipe_vertex_buffer_unreference(&mgr->real_vertex_buffer[out_vb]);
   mgr->real_vertex_buffer[out_vb].buffer.resource = out_buffer;
               }
      static bool
   u_vbuf_translate_find_free_vb_slots(struct u_vbuf *mgr,
         {
      unsigned type;
   unsigned fallback_vbs[VB_NUM];
   /* Set the bit for each buffer which is incompatible, or isn't set. */
   uint32_t unused_vb_mask =
      mgr->ve->incompatible_vb_mask_all | mgr->incompatible_vb_mask | mgr->ve->incompatible_vb_mask |
      uint32_t unused_vb_mask_orig;
            /* No vertex buffers available at all */
   if (!unused_vb_mask)
            memset(fallback_vbs, ~0, sizeof(fallback_vbs));
            /* Find free slots for each type if needed. */
   unused_vb_mask_orig = unused_vb_mask;
   for (type = 0; type < VB_NUM; type++) {
      if (mask[type]) {
               if (!unused_vb_mask) {
      insufficient_buffers = true;
               index = ffs(unused_vb_mask) - 1;
   fallback_vbs[type] = index;
   mgr->fallback_vbs_mask |= 1 << index;
   unused_vb_mask &= ~(1 << index);
                  if (insufficient_buffers) {
      /* not enough vbs for all types supported by the hardware, they will have to share one
   * buffer */
   uint32_t index = ffs(unused_vb_mask_orig) - 1;
   /* When sharing one vertex buffer use per-vertex frequency for everything. */
   fallback_vbs[VB_VERTEX] = index;
   mgr->fallback_vbs_mask = 1 << index;
   mask[VB_VERTEX] = mask[VB_VERTEX] | mask[VB_CONST] | mask[VB_INSTANCE];
   mask[VB_CONST] = 0;
               for (type = 0; type < VB_NUM; type++) {
      if (mask[type]) {
                     memcpy(mgr->fallback_vbs, fallback_vbs, sizeof(fallback_vbs));
      }
      static bool
   u_vbuf_translate_begin(struct u_vbuf *mgr,
                        const struct pipe_draw_info *info,
      {
      unsigned mask[VB_NUM] = {0};
   struct translate_key key[VB_NUM];
   unsigned elem_index[VB_NUM][PIPE_MAX_ATTRIBS]; /* ... into key.elements */
   unsigned i, type;
   const unsigned incompatible_vb_mask = (misaligned | mgr->incompatible_vb_mask | mgr->ve->incompatible_vb_mask) &
            const int start[VB_NUM] = {
      start_vertex,           /* VERTEX */
   info->start_instance,   /* INSTANCE */
               const unsigned num[VB_NUM] = {
      num_vertices,           /* VERTEX */
   info->instance_count,   /* INSTANCE */
               memset(key, 0, sizeof(key));
            /* See if there are vertex attribs of each type to translate and
   * which ones. */
   for (i = 0; i < mgr->ve->count; i++) {
               if (!mgr->ve->ve[i].src_stride) {
      if (!(mgr->ve->incompatible_elem_mask & (1 << i)) &&
      !(incompatible_vb_mask & (1 << vb_index))) {
      }
      } else if (mgr->ve->ve[i].instance_divisor) {
      if (!(mgr->ve->incompatible_elem_mask & (1 << i)) &&
      !(incompatible_vb_mask & (1 << vb_index))) {
      }
      } else {
      if (!unroll_indices &&
      !(mgr->ve->incompatible_elem_mask & (1 << i)) &&
   !(incompatible_vb_mask & (1 << vb_index))) {
      }
                           /* Find free vertex buffer slots. */
   if (!u_vbuf_translate_find_free_vb_slots(mgr, mask)) {
                  unsigned min_alignment[VB_NUM] = {0};
   /* Initialize the translate keys. */
   for (i = 0; i < mgr->ve->count; i++) {
      struct translate_key *k;
   struct translate_element *te;
   enum pipe_format output_format = mgr->ve->native_format[i];
   unsigned bit, vb_index = mgr->ve->ve[i].vertex_buffer_index;
            if (!(mgr->ve->incompatible_elem_mask & (1 << i)) &&
      !(incompatible_vb_mask & (1 << vb_index)) &&
   (!unroll_indices || !(mask[VB_VERTEX] & bit))) {
               /* Set type to what we will translate.
   * Whether vertex, instance, or constant attribs. */
   for (type = 0; type < VB_NUM; type++) {
      if (mask[type] & bit) {
            }
   assert(type < VB_NUM);
   if (mgr->ve->ve[i].src_format != output_format)
                  /* Add the vertex element. */
   k = &key[type];
            te = &k->element[k->nr_elements];
   te->type = TRANSLATE_ELEMENT_NORMAL;
   te->instance_divisor = 0;
   te->input_buffer = vb_index;
   te->input_format = mgr->ve->ve[i].src_format;
   te->input_offset = mgr->ve->ve[i].src_offset;
   te->output_format = output_format;
   te->output_offset = k->output_stride;
   unsigned adjustment = 0;
   if (!mgr->caps.attrib_component_unaligned &&
      te->output_offset % mgr->ve->component_size[i] != 0) {
   unsigned aligned = align(te->output_offset, mgr->ve->component_size[i]);
   adjustment = aligned - te->output_offset;
               k->output_stride += mgr->ve->native_format_size[i] + adjustment;
   k->nr_elements++;
               /* Translate buffers. */
   for (type = 0; type < VB_NUM; type++) {
      if (key[type].nr_elements) {
      enum pipe_error err;
   if (!mgr->caps.attrib_component_unaligned)
         err = u_vbuf_translate_buffers(mgr, &key[type], info, draw,
                     if (err != PIPE_OK)
                  /* Setup new vertex elements. */
   for (i = 0; i < mgr->ve->count; i++) {
      for (type = 0; type < VB_NUM; type++) {
      if (elem_index[type][i] < key[type].nr_elements) {
      struct translate_element *te = &key[type].element[elem_index[type][i]];
   mgr->fallback_velems.velems[i].instance_divisor = mgr->ve->ve[i].instance_divisor;
   mgr->fallback_velems.velems[i].src_format = te->output_format;
                  /* Fixup the stride for constant attribs. */
   if (type == VB_CONST)
                        /* elem_index[type][i] can only be set for one type. */
   assert(type > VB_INSTANCE || elem_index[type+1][i] == ~0u);
   assert(type > VB_VERTEX   || elem_index[type+2][i] == ~0u);
         }
   /* No translating, just copy the original vertex element over. */
   if (type == VB_NUM) {
      memcpy(&mgr->fallback_velems.velems[i], &mgr->ve->ve[i],
                           u_vbuf_set_vertex_elements_internal(mgr, &mgr->fallback_velems);
   mgr->using_translate = true;
      }
      static void u_vbuf_translate_end(struct u_vbuf *mgr)
   {
               /* Restore vertex elements. */
   mgr->pipe->bind_vertex_elements_state(mgr->pipe, mgr->ve->driver_cso);
            /* Unreference the now-unused VBOs. */
   for (i = 0; i < VB_NUM; i++) {
      unsigned vb = mgr->fallback_vbs[i];
   if (vb != ~0u) {
      pipe_resource_reference(&mgr->real_vertex_buffer[vb].buffer.resource, NULL);
         }
   /* This will cause the buffer to be unbound in the driver later. */
   mgr->dirty_real_vb_mask |= mgr->fallback_vbs_mask;
      }
      static void *
   u_vbuf_create_vertex_elements(struct u_vbuf *mgr, unsigned count,
         {
      struct pipe_vertex_element tmp[PIPE_MAX_ATTRIBS];
            struct pipe_context *pipe = mgr->pipe;
   unsigned i;
   struct pipe_vertex_element driver_attribs[PIPE_MAX_ATTRIBS];
   struct u_vbuf_elements *ve = CALLOC_STRUCT(u_vbuf_elements);
                     memcpy(ve->ve, attribs, sizeof(struct pipe_vertex_element) * count);
            /* Set the best native format in case the original format is not
   * supported. */
   for (i = 0; i < count; i++) {
      enum pipe_format format = ve->ve[i].src_format;
                     if (used_buffers & vb_index_bit)
                     if (!ve->ve[i].instance_divisor) {
                           driver_attribs[i].src_format = format;
   ve->native_format[i] = format;
   ve->native_format_size[i] =
            const struct util_format_description *desc = util_format_description(format);
   bool is_packed = false;
   for (unsigned c = 0; c < desc->nr_channels; c++)
         unsigned component_size = is_packed ?
                  if (ve->ve[i].src_format != format ||
      (!mgr->caps.velem_src_offset_unaligned &&
   ve->ve[i].src_offset % 4 != 0) ||
   (!mgr->caps.attrib_component_unaligned &&
   ve->ve[i].src_offset % component_size != 0)) {
   ve->incompatible_elem_mask |= 1 << i;
      } else {
      ve->compatible_vb_mask_any |= vb_index_bit;
   if (component_size == 2) {
      ve->vb_align_mask[0] |= vb_index_bit;
   if (ve->ve[i].src_stride % 2 != 0)
      }
   else if (component_size == 4) {
      ve->vb_align_mask[1] |= vb_index_bit;
   if (ve->ve[i].src_stride % 4 != 0)
         }
   ve->strides[ve->ve[i].vertex_buffer_index] = ve->ve[i].src_stride;
   if (ve->ve[i].src_stride) {
         }
   if (!mgr->caps.buffer_stride_unaligned && ve->ve[i].src_stride % 4 != 0)
               if (used_buffers & ~mgr->allowed_vb_mask) {
      /* More vertex buffers are used than the hardware supports.  In
   * principle, we only need to make sure that less vertex buffers are
   * used, and mark some of the latter vertex buffers as incompatible.
   * For now, mark all vertex buffers as incompatible.
   */
   ve->incompatible_vb_mask_any = used_buffers;
   ve->compatible_vb_mask_any = 0;
               ve->used_vb_mask = used_buffers;
   ve->compatible_vb_mask_all = ~ve->incompatible_vb_mask_any & used_buffers;
            /* Align the formats and offsets to the size of DWORD if needed. */
   if (!mgr->caps.velem_src_offset_unaligned) {
      for (i = 0; i < count; i++) {
      ve->native_format_size[i] = align(ve->native_format_size[i], 4);
                  /* Only create driver CSO if no incompatible elements */
   if (!ve->incompatible_elem_mask) {
      ve->driver_cso =
                  }
      static void u_vbuf_delete_vertex_elements(void *ctx, void *state,
         {
      struct pipe_context *pipe = (struct pipe_context*)ctx;
   struct cso_velements *cso = (struct cso_velements*)state;
            if (ve->driver_cso)
         FREE(ve);
      }
      void u_vbuf_set_vertex_buffers(struct u_vbuf *mgr,
                           {
      unsigned i;
   /* which buffers are enabled */
   uint32_t enabled_vb_mask = 0;
   /* which buffers are in user memory */
   uint32_t user_vb_mask = 0;
   /* which buffers are incompatible with the driver */
   uint32_t incompatible_vb_mask = 0;
   /* which buffers are unaligned to 2/4 bytes */
   uint32_t unaligned_vb_mask[2] = {0};
            if (!bufs) {
      struct pipe_context *pipe = mgr->pipe;
   /* Unbind. */
   unsigned total_count = count + unbind_num_trailing_slots;
            /* Zero out the bits we are going to rewrite completely. */
   mgr->user_vb_mask &= mask;
   mgr->incompatible_vb_mask &= mask;
   mgr->enabled_vb_mask &= mask;
   mgr->unaligned_vb_mask[0] &= mask;
            for (i = 0; i < total_count; i++) {
               pipe_vertex_buffer_unreference(&mgr->vertex_buffer[dst_index]);
               pipe->set_vertex_buffers(pipe, count, unbind_num_trailing_slots, false, NULL);
               for (i = 0; i < count; i++) {
      unsigned dst_index = i;
   const struct pipe_vertex_buffer *vb = &bufs[i];
   struct pipe_vertex_buffer *orig_vb = &mgr->vertex_buffer[dst_index];
            if (!vb->buffer.resource) {
      pipe_vertex_buffer_unreference(orig_vb);
   pipe_vertex_buffer_unreference(real_vb);
               bool not_user = !vb->is_user_buffer && vb->is_user_buffer == orig_vb->is_user_buffer;
   /* struct isn't tightly packed: do not use memcmp */
   if (not_user &&
      orig_vb->buffer_offset == vb->buffer_offset && orig_vb->buffer.resource == vb->buffer.resource) {
   mask |= BITFIELD_BIT(dst_index);
   if (take_ownership) {
      pipe_vertex_buffer_unreference(orig_vb);
   /* the pointer was unset in the line above, so copy it back */
      }
   if (mask == UINT32_MAX)
                     if (take_ownership) {
      pipe_vertex_buffer_unreference(orig_vb);
      } else {
                           if ((!mgr->caps.buffer_offset_unaligned && vb->buffer_offset % 4 != 0)) {
      incompatible_vb_mask |= 1 << dst_index;
   real_vb->buffer_offset = vb->buffer_offset;
   pipe_vertex_buffer_unreference(real_vb);
   real_vb->is_user_buffer = false;
               if (!mgr->caps.attrib_component_unaligned) {
      if (vb->buffer_offset % 2 != 0)
         if (vb->buffer_offset % 4 != 0)
               if (!mgr->caps.user_vertex_buffers && vb->is_user_buffer) {
      user_vb_mask |= 1 << dst_index;
   real_vb->buffer_offset = vb->buffer_offset;
   pipe_vertex_buffer_unreference(real_vb);
   real_vb->is_user_buffer = false;
                           for (i = 0; i < unbind_num_trailing_slots; i++) {
               pipe_vertex_buffer_unreference(&mgr->vertex_buffer[dst_index]);
                  /* Zero out the bits we are going to rewrite completely. */
   mgr->user_vb_mask &= mask;
   mgr->incompatible_vb_mask &= mask;
   mgr->enabled_vb_mask &= mask;
   mgr->unaligned_vb_mask[0] &= mask;
            mgr->user_vb_mask |= user_vb_mask;
   mgr->incompatible_vb_mask |= incompatible_vb_mask;
   mgr->enabled_vb_mask |= enabled_vb_mask;
   mgr->unaligned_vb_mask[0] |= unaligned_vb_mask[0];
            /* All changed buffers are marked as dirty, even the NULL ones,
   * which will cause the NULL buffers to be unbound in the driver later. */
      }
      static ALWAYS_INLINE bool
   get_upload_offset_size(struct u_vbuf *mgr,
                        const struct pipe_vertex_buffer *vb,
   struct u_vbuf_elements *ve,
   const struct pipe_vertex_element *velem,
      {
      /* Skip the buffers generated by translate. */
   if ((1 << vb_index) & mgr->fallback_vbs_mask || !vb->is_user_buffer)
            unsigned instance_div = velem->instance_divisor;
            if (!velem->src_stride) {
      /* Constant attrib. */
      } else if (instance_div) {
               /* Figure out how many instances we'll render given instance_div.  We
   * can't use the typical div_round_up() pattern because the CTS uses
   * instance_div = ~0 for a test, which overflows div_round_up()'s
   * addition.
   */
   unsigned count = num_instances / instance_div;
   if (count * instance_div != num_instances)
            *offset += velem->src_stride * start_instance;
      } else {
      /* Per-vertex attrib. */
   *offset += velem->src_stride * start_vertex;
      }
      }
         static enum pipe_error
   u_vbuf_upload_buffers(struct u_vbuf *mgr,
               {
      unsigned i;
   struct u_vbuf_elements *ve = mgr->ve;
   unsigned nr_velems = ve->count;
   const struct pipe_vertex_element *velems =
            /* Faster path when no vertex attribs are interleaved. */
   if ((ve->interleaved_vb_mask & mgr->user_vb_mask) == 0) {
      for (i = 0; i < nr_velems; i++) {
      const struct pipe_vertex_element *velem = &velems[i];
   unsigned index = velem->vertex_buffer_index;
                  if (!get_upload_offset_size(mgr, vb, ve, velem, index, i, start_vertex,
                                       u_upload_data(mgr->pipe->stream_uploader,
               mgr->has_signed_vb_offset ? 0 : offset,
                     }
               unsigned start_offset[PIPE_MAX_ATTRIBS];
   unsigned end_offset[PIPE_MAX_ATTRIBS];
            /* Slower path supporting interleaved vertex attribs using 2 loops. */
   /* Determine how much data needs to be uploaded. */
   for (i = 0; i < nr_velems; i++) {
      const struct pipe_vertex_element *velem = &velems[i];
   unsigned index = velem->vertex_buffer_index;
   struct pipe_vertex_buffer *vb = &mgr->vertex_buffer[index];
            if (!get_upload_offset_size(mgr, vb, ve, velem, index, i, start_vertex,
                                 /* Update offsets. */
   if (!(buffer_mask & index_bit)) {
      start_offset[index] = first;
      } else {
      if (first < start_offset[index])
         if (first + size > end_offset[index])
                           /* Upload buffers. */
   while (buffer_mask) {
      unsigned start, end;
   struct pipe_vertex_buffer *real_vb;
                     start = start_offset[i];
   end = end_offset[i];
            real_vb = &mgr->real_vertex_buffer[i];
            u_upload_data(mgr->pipe->stream_uploader,
               mgr->has_signed_vb_offset ? 0 : start,
   if (!real_vb->buffer.resource)
                           }
      static bool u_vbuf_need_minmax_index(const struct u_vbuf *mgr, uint32_t misaligned)
   {
      /* See if there are any per-vertex attribs which will be uploaded or
   * translated. Use bitmasks to get the info instead of looping over vertex
   * elements. */
   return (mgr->ve->used_vb_mask &
         ((mgr->user_vb_mask |
      mgr->incompatible_vb_mask | mgr->ve->incompatible_vb_mask |
   misaligned |
   mgr->ve->incompatible_vb_mask_any) &
   }
      static bool u_vbuf_mapping_vertex_buffer_blocks(const struct u_vbuf *mgr, uint32_t misaligned)
   {
      /* Return true if there are hw buffers which don't need to be translated.
   *
   * We could query whether each buffer is busy, but that would
   * be way more costly than this. */
   return (mgr->ve->used_vb_mask &
         (~mgr->user_vb_mask &
      ~mgr->incompatible_vb_mask &
   ~mgr->ve->incompatible_vb_mask &
   ~misaligned &
   mgr->ve->compatible_vb_mask_all &
   }
      static void
   u_vbuf_get_minmax_index_mapped(const struct pipe_draw_info *info,
                     {
      if (!count) {
      *out_min_index = 0;
   *out_max_index = 0;
               switch (info->index_size) {
   case 4: {
      const unsigned *ui_indices = (const unsigned*)indices;
   unsigned max = 0;
   unsigned min = ~0u;
   if (info->primitive_restart) {
      for (unsigned i = 0; i < count; i++) {
      if (ui_indices[i] != info->restart_index) {
      if (ui_indices[i] > max) max = ui_indices[i];
            }
   else {
      for (unsigned i = 0; i < count; i++) {
      if (ui_indices[i] > max) max = ui_indices[i];
         }
   *out_min_index = min;
   *out_max_index = max;
      }
   case 2: {
      const unsigned short *us_indices = (const unsigned short*)indices;
   unsigned short max = 0;
   unsigned short min = ~((unsigned short)0);
   if (info->primitive_restart) {
      for (unsigned i = 0; i < count; i++) {
      if (us_indices[i] != info->restart_index) {
      if (us_indices[i] > max) max = us_indices[i];
            }
   else {
      for (unsigned i = 0; i < count; i++) {
      if (us_indices[i] > max) max = us_indices[i];
         }
   *out_min_index = min;
   *out_max_index = max;
      }
   case 1: {
      const unsigned char *ub_indices = (const unsigned char*)indices;
   unsigned char max = 0;
   unsigned char min = ~((unsigned char)0);
   if (info->primitive_restart) {
      for (unsigned i = 0; i < count; i++) {
      if (ub_indices[i] != info->restart_index) {
      if (ub_indices[i] > max) max = ub_indices[i];
            }
   else {
      for (unsigned i = 0; i < count; i++) {
      if (ub_indices[i] > max) max = ub_indices[i];
         }
   *out_min_index = min;
   *out_max_index = max;
      }
   default:
            }
      void u_vbuf_get_minmax_index(struct pipe_context *pipe,
                     {
      struct pipe_transfer *transfer = NULL;
            if (info->has_user_indices) {
      indices = (uint8_t*)info->index.user +
      } else {
      indices = pipe_buffer_map_range(pipe, info->index.resource,
                           u_vbuf_get_minmax_index_mapped(info, draw->count, indices,
            if (transfer) {
            }
      static void u_vbuf_set_driver_vertex_buffers(struct u_vbuf *mgr)
   {
      struct pipe_context *pipe = mgr->pipe;
            if (mgr->dirty_real_vb_mask == mgr->enabled_vb_mask &&
      mgr->dirty_real_vb_mask == mgr->user_vb_mask) {
   /* Fast path that allows us to transfer the VBO references to the driver
   * to skip atomic reference counting there. These are freshly uploaded
   * user buffers that can be discarded after this call.
   */
            /* We don't own the VBO references now. Set them to NULL. */
   for (unsigned i = 0; i < count; i++) {
      assert(!mgr->real_vertex_buffer[i].is_user_buffer);
         } else {
      /* Slow path where we have to keep VBO references. */
      }
      }
      static void
   u_vbuf_split_indexed_multidraw(struct u_vbuf *mgr, struct pipe_draw_info *info,
                     {
      /* Increase refcount to be able to use take_index_buffer_ownership with
   * all draws.
   */
   if (draw_count > 1 && info->take_index_buffer_ownership)
                     for (unsigned i = 0; i < draw_count; i++) {
      struct pipe_draw_start_count_bias draw;
            draw.count = indirect_data[offset + 0];
   info->instance_count = indirect_data[offset + 1];
   draw.start = indirect_data[offset + 2];
   draw.index_bias = indirect_data[offset + 3];
                  }
      void u_vbuf_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info,
                           {
      struct u_vbuf *mgr = pipe->vbuf;
   int start_vertex;
   unsigned min_index;
   unsigned num_vertices;
   bool unroll_indices = false;
   const uint32_t used_vb_mask = mgr->ve->used_vb_mask;
   uint32_t user_vb_mask = mgr->user_vb_mask & used_vb_mask;
            uint32_t misaligned = 0;
   if (!mgr->caps.attrib_component_unaligned) {
      for (unsigned i = 0; i < ARRAY_SIZE(mgr->unaligned_vb_mask); i++) {
            }
   const uint32_t incompatible_vb_mask =
            /* Normal draw. No fallback and no user buffers. */
   if (!incompatible_vb_mask &&
      !mgr->ve->incompatible_elem_mask &&
   !user_vb_mask &&
   (info->index_size != 1 || !mgr->caps.rewrite_ubyte_ibs) &&
   (!info->primitive_restart ||
   info->restart_index == fixed_restart_index ||
   !mgr->caps.rewrite_restart_index) &&
   (!info->primitive_restart || mgr->caps.supported_restart_modes & BITFIELD_BIT(info->mode)) &&
            /* Set vertex buffers if needed. */
   if (mgr->dirty_real_vb_mask & used_vb_mask) {
                  pipe->draw_vbo(pipe, info, drawid_offset, indirect, draws, num_draws);
               /* Increase refcount to be able to use take_index_buffer_ownership with
   * all draws.
   */
   if (num_draws > 1 && info->take_index_buffer_ownership)
            for (unsigned d = 0; d < num_draws; d++) {
      struct pipe_draw_info new_info = *info;
            /* Handle indirect (multi)draws. */
   if (indirect && indirect->buffer) {
                              /* Get the number of draws. */
   if (indirect->indirect_draw_count) {
      pipe_buffer_read(pipe, indirect->indirect_draw_count,
            } else {
                                 unsigned data_size = (draw_count - 1) * indirect->stride +
         unsigned *data = malloc(data_size);
                  /* Read the used buffer range only once, because the read can be
   * uncached.
   */
                  if (info->index_size) {
      /* Indexed multidraw. */
                  /* If we invoke the translate path, we have to split the multidraw. */
   if (incompatible_vb_mask ||
      mgr->ve->incompatible_elem_mask) {
   u_vbuf_split_indexed_multidraw(mgr, &new_info, drawid_offset, data,
         free(data);
                     /* See if index_bias is the same for all draws. */
   for (unsigned i = 1; i < draw_count; i++) {
      if (data[i * indirect->stride / 4 + 3] != index_bias0) {
      index_bias_same = false;
                  /* Split the multidraw if index_bias is different. */
   if (!index_bias_same) {
      u_vbuf_split_indexed_multidraw(mgr, &new_info, drawid_offset, data,
         free(data);
                     /* If we don't need to use the translate path and index_bias is
   * the same, we can process the multidraw with the time complexity
   * equal to 1 draw call (except for the index range computation).
   * We only need to compute the index range covering all draw calls
   * of the multidraw.
   *
   * The driver will not look at these values because indirect != NULL.
   * These values determine the user buffer bounds to upload.
   */
   new_draw.index_bias = index_bias0;
   new_info.index_bounds_valid = true;
   new_info.min_index = ~0u;
   new_info.max_index = 0;
                                 if (info->has_user_indices) {
         } else {
                        for (unsigned i = 0; i < draw_count; i++) {
      unsigned offset = i * indirect->stride / 4;
   unsigned start = data[offset + 2];
                                       /* Update the ranges of instances. */
                        /* Update the index range. */
   unsigned min, max;
   u_vbuf_get_minmax_index_mapped(&new_info, count,
                        new_info.min_index = MIN2(new_info.min_index, min);
                                                   if (new_info.start_instance == ~0u || !new_info.instance_count)
      } else {
      /* Non-indexed multidraw.
   *
   * Keep the draw call indirect and compute minimums & maximums,
   * which will determine the user buffer bounds to upload, but
   * the driver will not look at these values because indirect != NULL.
   *
   * This efficiently processes the multidraw with the time complexity
   * equal to 1 draw call.
   */
   new_draw.start = ~0u;
   new_info.start_instance = ~0u;
                  for (unsigned i = 0; i < draw_count; i++) {
      unsigned offset = i * indirect->stride / 4;
   unsigned start = data[offset + 2];
                                             end_vertex = MAX2(end_vertex, start + count);
                     /* Set the final counts. */
                  if (new_draw.start == ~0u || !new_draw.count || !new_info.instance_count)
         } else {
      if ((!indirect && !new_draw.count) || !new_info.instance_count)
               if (new_info.index_size) {
      /* See if anything needs to be done for per-vertex attribs. */
                     if (new_info.index_bounds_valid) {
      min_index = new_info.min_index;
      } else {
                                                /* Primitive restart doesn't work when unrolling indices.
   * We would have to break this drawing operation into several ones. */
   /* Use some heuristic to see if unrolling indices improves
   * performance. */
   if (!indirect &&
      !new_info.primitive_restart &&
   util_is_vbo_upload_ratio_too_large(new_draw.count, num_vertices) &&
   !u_vbuf_mapping_vertex_buffer_blocks(mgr, misaligned)) {
   unroll_indices = true;
   user_vb_mask &= ~(mgr->ve->nonzero_stride_vb_mask &
         } else {
      /* Nothing to do for per-vertex attribs. */
   start_vertex = 0;
   num_vertices = 0;
         } else {
      start_vertex = new_draw.start;
   num_vertices = new_draw.count;
               /* Translate vertices with non-native layouts or formats. */
   if (unroll_indices ||
      incompatible_vb_mask ||
   mgr->ve->incompatible_elem_mask) {
   if (!u_vbuf_translate_begin(mgr, &new_info, &new_draw,
                  debug_warn_once("u_vbuf_translate_begin() failed");
               if (unroll_indices) {
      if (!new_info.has_user_indices && info->take_index_buffer_ownership)
         new_info.index_size = 0;
   new_draw.index_bias = 0;
   new_info.index_bounds_valid = true;
   new_info.min_index = 0;
   new_info.max_index = new_draw.count - 1;
               user_vb_mask &= ~(incompatible_vb_mask |
               /* Upload user buffers. */
   if (user_vb_mask) {
      if (u_vbuf_upload_buffers(mgr, start_vertex, num_vertices,
                  debug_warn_once("u_vbuf_upload_buffers() failed");
                           /*
   if (unroll_indices) {
      printf("unrolling indices: start_vertex = %i, num_vertices = %i\n",
         util_dump_draw_info(stdout, info);
               unsigned i;
   for (i = 0; i < mgr->nr_vertex_buffers; i++) {
      printf("input %i: ", i);
   util_dump_vertex_buffer(stdout, mgr->vertex_buffer+i);
      }
   for (i = 0; i < mgr->nr_real_vertex_buffers; i++) {
      printf("real %i: ", i);
   util_dump_vertex_buffer(stdout, mgr->real_vertex_buffer+i);
      }
            u_upload_unmap(pipe->stream_uploader);
   if (mgr->dirty_real_vb_mask)
            if ((new_info.index_size == 1 && mgr->caps.rewrite_ubyte_ibs) ||
      (new_info.primitive_restart &&
   ((new_info.restart_index != fixed_restart_index && mgr->caps.rewrite_restart_index) ||
   !(mgr->caps.supported_restart_modes & BITFIELD_BIT(new_info.mode)))) ||
   !(mgr->caps.supported_prim_modes & BITFIELD_BIT(new_info.mode))) {
   util_primconvert_save_flatshade_first(mgr->pc, mgr->flatshade_first);
      } else
         if (info->increment_draw_id)
               if (mgr->using_translate) {
         }
         cleanup:
      if (info->take_index_buffer_ownership) {
      struct pipe_resource *indexbuf = info->index.resource;
         }
      void u_vbuf_save_vertex_elements(struct u_vbuf *mgr)
   {
      assert(!mgr->ve_saved);
      }
      void u_vbuf_restore_vertex_elements(struct u_vbuf *mgr)
   {
      if (mgr->ve != mgr->ve_saved) {
               mgr->ve = mgr->ve_saved;
   pipe->bind_vertex_elements_state(pipe,
      }
      }
