   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
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
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /*
   * This file implements the st_draw_vbo() function which is called from
   * Mesa's VBO module.  All point/line/triangle rendering is done through
   * this function whether the user called glBegin/End, glDrawArrays,
   * glDrawElements, glEvalMesh, or glCalList, etc.
   *
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   */
      #include "main/context.h"
   #include "main/errors.h"
      #include "main/image.h"
   #include "main/bufferobj.h"
   #include "main/macros.h"
   #include "main/varray.h"
      #include "compiler/glsl/ir_uniform.h"
      #include "vbo/vbo.h"
      #include "st_context.h"
   #include "st_atom.h"
   #include "st_cb_bitmap.h"
   #include "st_debug.h"
   #include "st_draw.h"
   #include "st_program.h"
   #include "st_util.h"
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_prim.h"
   #include "util/u_draw.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_threaded_context.h"
   #include "draw/draw_context.h"
   #include "cso_cache/cso_context.h"
      /* GL prims should match Gallium prims, spot-check a few */
   static_assert(GL_POINTS == MESA_PRIM_POINTS, "enum mismatch");
   static_assert(GL_QUADS == MESA_PRIM_QUADS, "enum mismatch");
   static_assert(GL_TRIANGLE_STRIP_ADJACENCY == MESA_PRIM_TRIANGLE_STRIP_ADJACENCY, "enum mismatch");
   static_assert(GL_PATCHES == MESA_PRIM_PATCHES, "enum mismatch");
      static inline void
   prepare_draw(struct st_context *st, struct gl_context *ctx, uint64_t state_mask)
   {
      /* Mesa core state should have been validated already */
            if (unlikely(!st->bitmap.cache.empty))
                     /* Validate state. */
            /* Pin threads regularly to the same Zen CCX that the main thread is
   * running on. The main thread can move between CCXs.
   */
   if (unlikely(st->pin_thread_counter != ST_L3_PINNING_DISABLED &&
                           int cpu = util_get_current_cpu();
   if (cpu >= 0) {
                     if (L3_cache != U_CPU_INVALID_L3) {
      pipe->set_context_param(pipe,
                     }
      static bool ALWAYS_INLINE
   prepare_indexed_draw(/* pass both st and ctx to reduce dereferences */
                        struct st_context *st,
      {
      /* Get index bounds for user buffers. */
   if (info->index_size && !info->index_bounds_valid &&
      st->draw_needs_minmax_index) {
   /* Return if this fails, which means all draws have count == 0. */
   if (!vbo_get_minmax_indices_gallium(ctx, info, draws, num_draws))
               }
      }
      static void
   st_draw_gallium(struct gl_context *ctx,
                  struct pipe_draw_info *info,
      {
                        if (!prepare_indexed_draw(st, ctx, info, draws, num_draws))
               }
      static void
   st_draw_gallium_multimode(struct gl_context *ctx,
                           {
                        if (!prepare_indexed_draw(st, ctx, info, draws, num_draws))
            unsigned i, first;
            /* Find consecutive draws where mode doesn't vary. */
   for (i = 0, first = 0; i <= num_draws; i++) {
      if (i == num_draws || mode[i] != mode[first]) {
      info->mode = mode[first];
                  /* We can pass the reference only once. st_buffer_object keeps
   * the reference alive for later draws.
   */
            }
      static void
   rewrite_partial_stride_indirect(struct st_context *st,
                     {
      unsigned draw_count = 0;
   struct u_indirect_params *new_draws = util_draw_indirect_read(st->pipe, info, indirect, &draw_count);
   if (!new_draws)
         for (unsigned i = 0; i < draw_count; i++)
            }
      void
   st_indirect_draw_vbo(struct gl_context *ctx,
                           {
      struct gl_buffer_object *indirect_data = ctx->DrawIndirectBuffer;
   struct gl_buffer_object *indirect_draw_count = ctx->ParameterBuffer;
   struct st_context *st = st_context(ctx);
   struct pipe_draw_info info;
   struct pipe_draw_indirect_info indirect;
            /* If indirect_draw_count is set, drawcount is the maximum draw count.*/
   if (!draw_count)
            assert(stride);
            memset(&indirect, 0, sizeof(indirect));
   util_draw_init_info(&info);
            switch (index_type) {
   case GL_UNSIGNED_BYTE:
      info.index_size = 1;
      case GL_UNSIGNED_SHORT:
      info.index_size = 2;
      case GL_UNSIGNED_INT:
      info.index_size = 4;
                        if (info.index_size) {
               /* indices are always in a real VBO */
            if (st->pipe->draw_vbo == tc_draw_vbo &&
      (draw_count == 1 || st->has_multi_draw_indirect)) {
   /* Fast path for u_threaded_context to eliminate atomics. */
   info.index.resource = _mesa_get_bufferobj_reference(ctx, bufobj);
      } else {
                  /* No index buffer storage allocated - nothing to do. */
   if (!info.index.resource)
                     unsigned index_size_shift = util_logbase2(info.index_size);
   info.restart_index = ctx->Array._RestartIndex[index_size_shift];
               info.mode = mode;
   indirect.buffer = indirect_data->buffer;
            /* Viewperf2020/Maya draws with a buffer that has no storage. */
   if (!indirect.buffer)
            if (!st->has_multi_draw_indirect) {
               indirect.draw_count = 1;
   for (i = 0; i < draw_count; i++) {
      cso_draw_vbo(st->cso_context, &info, i, &indirect, &draw, 1);
         } else {
      indirect.draw_count = draw_count;
   indirect.stride = stride;
   if (!st->has_indirect_partial_stride && stride &&
      (draw_count > 1 || indirect_draw_count)) {
   /* DrawElementsIndirectCommand or DrawArraysIndirectCommand */
   const size_t struct_size = info.index_size ? sizeof(uint32_t) * 5 : sizeof(uint32_t) * 4;
   if (indirect.stride && indirect.stride < struct_size) {
      rewrite_partial_stride_indirect(st, &info, &indirect, draw);
         }
   if (indirect_draw_count) {
      indirect.indirect_draw_count =
            }
               if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH)
      }
      void
   st_draw_transform_feedback(struct gl_context *ctx, GLenum mode,
               {
      struct st_context *st = st_context(ctx);
   struct pipe_draw_info info;
   struct pipe_draw_indirect_info indirect;
                     memset(&indirect, 0, sizeof(indirect));
   util_draw_init_info(&info);
   info.max_index = ~0u; /* so that u_vbuf can tell that it's unknown */
   info.mode = mode;
            /* Transform feedback drawing is always non-indexed. */
   /* Set info.count_from_stream_output. */
   indirect.count_from_stream_output = tfb_vertcount->draw_count[stream];
   if (indirect.count_from_stream_output == NULL)
               }
      static void
   st_draw_gallium_vertex_state(struct gl_context *ctx,
                                 {
                        struct pipe_context *pipe = st->pipe;
            if (!mode) {
         } else {
      /* Find consecutive draws where mode doesn't vary. */
   for (unsigned i = 0, first = 0; i <= num_draws; i++) {
                        /* Increase refcount to be able to use take_vertex_state_ownership
   * with all draws.
   */
                  info.mode = mode[first];
   pipe->draw_vertex_state(pipe, state, velem_mask, info, &draws[first],
                     }
      void
   st_init_draw_functions(struct pipe_screen *screen,
         {
      functions->DrawGallium = st_draw_gallium;
            if (screen->get_param(screen, PIPE_CAP_DRAW_VERTEX_STATE)) {
      functions->DrawGalliumVertexState = st_draw_gallium_vertex_state;
         }
         void
   st_destroy_draw(struct st_context *st)
   {
         }
      /**
   * Getter for the draw_context, so that initialization of it can happen only
   * when needed (the TGSI exec machines take up quite a bit of memory).
   */
   struct draw_context *
   st_get_draw_context(struct st_context *st)
   {
      if (!st->draw) {
      st->draw = draw_create(st->pipe);
   if (!st->draw) {
      _mesa_error(st->ctx, GL_OUT_OF_MEMORY, "feedback fallback allocation");
                  /* Disable draw options that might convert points/lines to tris, etc.
   * as that would foul-up feedback/selection mode.
   */
   draw_wide_line_threshold(st->draw, 1000.0f);
   draw_wide_point_threshold(st->draw, 1000.0f);
   draw_enable_line_stipple(st->draw, false);
               }
      /**
   * Draw a quad with given position, texcoords and color.
   */
   bool
   st_draw_quad(struct st_context *st,
               float x0, float y0, float x1, float y1, float z,
   float s0, float t0, float s1, float t1,
   {
      struct pipe_vertex_buffer vb = {0};
            u_upload_alloc(st->pipe->stream_uploader, 0,
               if (!vb.buffer.resource) {
                  /* lower-left */
   verts[0].x = x0;
   verts[0].y = y1;
   verts[0].z = z;
   verts[0].r = color[0];
   verts[0].g = color[1];
   verts[0].b = color[2];
   verts[0].a = color[3];
   verts[0].s = s0;
            /* lower-right */
   verts[1].x = x1;
   verts[1].y = y1;
   verts[1].z = z;
   verts[1].r = color[0];
   verts[1].g = color[1];
   verts[1].b = color[2];
   verts[1].a = color[3];
   verts[1].s = s1;
            /* upper-right */
   verts[2].x = x1;
   verts[2].y = y0;
   verts[2].z = z;
   verts[2].r = color[0];
   verts[2].g = color[1];
   verts[2].b = color[2];
   verts[2].a = color[3];
   verts[2].s = s1;
            /* upper-left */
   verts[3].x = x0;
   verts[3].y = y0;
   verts[3].z = z;
   verts[3].r = color[0];
   verts[3].g = color[1];
   verts[3].b = color[2];
   verts[3].a = color[3];
   verts[3].s = s0;
                     cso_set_vertex_buffers(st->cso_context, 1, 0, false, &vb);
            if (num_instances > 1) {
      cso_draw_arrays_instanced(st->cso_context, MESA_PRIM_TRIANGLE_FAN, 0, 4,
      } else {
                              }
      static void
   st_hw_select_draw_gallium(struct gl_context *ctx,
                           {
                        if (!prepare_indexed_draw(st, ctx, info, draws, num_draws))
            if (!st_draw_hw_select_prepare_common(ctx) ||
      !st_draw_hw_select_prepare_mode(ctx, info))
            }
      static void
   st_hw_select_draw_gallium_multimode(struct gl_context *ctx,
                           {
                        if (!prepare_indexed_draw(st, ctx, info, draws, num_draws))
            if (!st_draw_hw_select_prepare_common(ctx))
            unsigned i, first;
            /* Find consecutive draws where mode doesn't vary. */
   for (i = 0, first = 0; i <= num_draws; i++) {
      if (i == num_draws || mode[i] != mode[first]) {
                                       /* We can pass the reference only once. st_buffer_object keeps
   * the reference alive for later draws.
   */
            }
      void
   st_init_hw_select_draw_functions(struct pipe_screen *screen,
         {
      functions->DrawGallium = st_hw_select_draw_gallium;
      }
