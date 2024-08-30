   /*
   * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
   * Copyright 2010 Marek Olšák <maraeo@gmail.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      /* r300_render: Vertex and index buffer primitive emission. Contains both
   * HW TCL fastpath rendering, and SW TCL Draw-assisted rendering. */
      #include "draw/draw_context.h"
   #include "draw/draw_vbuf.h"
      #include "util/u_inlines.h"
      #include "util/format/u_format.h"
   #include "util/u_draw.h"
   #include "util/u_memory.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_prim.h"
      #include "r300_cs.h"
   #include "r300_context.h"
   #include "r300_screen_buffer.h"
   #include "r300_emit.h"
   #include "r300_reg.h"
   #include "r300_vs.h"
   #include "r300_fs.h"
      #include <limits.h>
      #define IMMD_DWORDS 32
      static uint32_t r300_translate_primitive(unsigned prim)
   {
      static const int prim_conv[] = {
      R300_VAP_VF_CNTL__PRIM_POINTS,
   R300_VAP_VF_CNTL__PRIM_LINES,
   R300_VAP_VF_CNTL__PRIM_LINE_LOOP,
   R300_VAP_VF_CNTL__PRIM_LINE_STRIP,
   R300_VAP_VF_CNTL__PRIM_TRIANGLES,
   R300_VAP_VF_CNTL__PRIM_TRIANGLE_STRIP,
   R300_VAP_VF_CNTL__PRIM_TRIANGLE_FAN,
   R300_VAP_VF_CNTL__PRIM_QUADS,
   R300_VAP_VF_CNTL__PRIM_QUAD_STRIP,
   R300_VAP_VF_CNTL__PRIM_POLYGON,
   -1,
   -1,
   -1,
      };
            assert(hwprim != -1);
      }
      static uint32_t r300_provoking_vertex_fixes(struct r300_context *r300,
         {
      struct r300_rs_state* rs = (struct r300_rs_state*)r300->rs_state.state;
            /* By default (see r300_state.c:r300_create_rs_state) color_control is
   * initialized to provoking the first vertex.
   *
   * Triangle fans must be reduced to the second vertex, not the first, in
   * Gallium flatshade-first mode, as per the GL spec.
   * (http://www.opengl.org/registry/specs/ARB/provoking_vertex.txt)
   *
   * Quads never provoke correctly in flatshade-first mode. The first
   * vertex is never considered as provoking, so only the second, third,
   * and fourth vertices can be selected, and both "third" and "last" modes
   * select the fourth vertex. This is probably due to D3D lacking quads.
   *
   * Similarly, polygons reduce to the first, not the last, vertex, when in
   * "last" mode, and all other modes start from the second vertex.
   *
   * ~ C.
            if (rs->rs.flatshade_first) {
      switch (mode) {
         case MESA_PRIM_TRIANGLE_FAN:
      color_control |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_SECOND;
      case MESA_PRIM_QUADS:
   case MESA_PRIM_QUAD_STRIP:
   case MESA_PRIM_POLYGON:
      color_control |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_LAST;
      default:
            } else {
                     }
      void r500_emit_index_bias(struct r300_context *r300, int index_bias)
   {
               BEGIN_CS(2);
   OUT_CS_REG(R500_VAP_INDEX_OFFSET,
            }
      static void r300_emit_draw_init(struct r300_context *r300, unsigned mode,
         {
                        BEGIN_CS(5);
   OUT_CS_REG(R300_GA_COLOR_CONTROL,
         OUT_CS_REG_SEQ(R300_VAP_VF_MAX_VTX_INDX, 2);
   OUT_CS(max_index);
   OUT_CS(0);
      }
      /* This function splits the index bias value into two parts:
   * - buffer_offset: the value that can be safely added to buffer offsets
   *   in r300_emit_vertex_arrays (it must yield a positive offset when added to
   *   a vertex buffer offset)
   * - index_offset: the value that must be manually subtracted from indices
   *   in an index buffer to achieve negative offsets. */
   static void r300_split_index_bias(struct r300_context *r300, int index_bias,
         {
      struct pipe_vertex_buffer *vb, *vbufs = r300->vertex_buffer;
   struct pipe_vertex_element *velem = r300->velems->velem;
   unsigned i, size;
            if (index_bias < 0) {
      /* See how large index bias we may subtract. We must be careful
      * here because negative buffer offsets are not allowed
      max_neg_bias = INT_MAX;
   for (i = 0; i < r300->velems->count; i++) {
         vb = &vbufs[velem[i].vertex_buffer_index];
   size = (vb->buffer_offset + velem[i].src_offset) / velem[i].src_stride;
            /* Now set the minimum allowed value. */
      } else {
      /* A positive index bias is OK. */
                  }
      enum r300_prepare_flags {
      PREP_EMIT_STATES    = (1 << 0), /* call emit_dirty_state and friends? */
   PREP_VALIDATE_VBOS  = (1 << 1), /* validate VBOs? */
   PREP_EMIT_VARRAYS       = (1 << 2), /* call emit_vertex_arrays? */
   PREP_EMIT_VARRAYS_SWTCL = (1 << 3), /* call emit_vertex_arrays_swtcl? */
      };
      /**
   * Check if the requested number of dwords is available in the CS and
   * if not, flush.
   * \param r300          The context.
   * \param flags         See r300_prepare_flags.
   * \param cs_dwords     The number of dwords to reserve in CS.
   * \return TRUE if the CS was flushed
   */
   static bool r300_reserve_cs_dwords(struct r300_context *r300,
               {
      bool flushed        = false;
   bool emit_states    = flags & PREP_EMIT_STATES;
   bool emit_vertex_arrays       = flags & PREP_EMIT_VARRAYS;
            /* Add dirty state, index offset, and AOS. */
   if (emit_states)
            if (r300->screen->caps.is_r500)
            if (emit_vertex_arrays)
            if (emit_vertex_arrays_swtcl)
                     /* Reserve requested CS space. */
   if (!r300->rws->cs_check_space(&r300->cs, cs_dwords)) {
      r300_flush(&r300->context, PIPE_FLUSH_ASYNC, NULL);
                  }
      /**
   * Validate buffers and emit dirty state.
   * \param r300          The context.
   * \param flags         See r300_prepare_flags.
   * \param index_buffer  The index buffer to validate. The parameter may be NULL.
   * \param buffer_offset The offset passed to emit_vertex_arrays.
   * \param index_bias    The index bias to emit.
   * \param instance_id   Index of instance to render
   * \return TRUE if rendering should be skipped
   */
   static bool r300_emit_states(struct r300_context *r300,
                           {
      bool emit_states    = flags & PREP_EMIT_STATES;
   bool emit_vertex_arrays       = flags & PREP_EMIT_VARRAYS;
   bool emit_vertex_arrays_swtcl = flags & PREP_EMIT_VARRAYS_SWTCL;
   bool indexed        = flags & PREP_INDEXED;
            /* Validate buffers and emit dirty state if needed. */
   if (emit_states || (emit_vertex_arrays && validate_vbos)) {
      if (!r300_emit_buffer_validate(r300, validate_vbos,
            fprintf(stderr, "r300: CS space validation failed. "
                        if (emit_states)
            if (r300->screen->caps.is_r500) {
      if (r300->screen->caps.has_tcl)
         else
               if (emit_vertex_arrays &&
      (r300->vertex_arrays_dirty ||
      r300->vertex_arrays_indexed != indexed ||
   r300->vertex_arrays_offset != buffer_offset ||
               r300->vertex_arrays_dirty = false;
   r300->vertex_arrays_indexed = indexed;
   r300->vertex_arrays_offset = buffer_offset;
               if (emit_vertex_arrays_swtcl)
               }
      /**
   * Check if the requested number of dwords is available in the CS and
   * if not, flush. Then validate buffers and emit dirty state.
   * \param r300          The context.
   * \param flags         See r300_prepare_flags.
   * \param index_buffer  The index buffer to validate. The parameter may be NULL.
   * \param cs_dwords     The number of dwords to reserve in CS.
   * \param buffer_offset The offset passed to emit_vertex_arrays.
   * \param index_bias    The index bias to emit.
   * \param instance_id The instance to render.
   * \return TRUE if rendering should be skipped
   */
   static bool r300_prepare_for_rendering(struct r300_context *r300,
                                       {
      /* Make sure there is enough space in the command stream and emit states. */
   if (r300_reserve_cs_dwords(r300, flags, cs_dwords))
            return r300_emit_states(r300, flags, index_buffer, buffer_offset,
      }
      static bool immd_is_good_idea(struct r300_context *r300,
         {
      if (DBG_ON(r300, DBG_NO_IMMD)) {
                  if (count * r300->velems->vertex_size_dwords > IMMD_DWORDS) {
                  /* Buffers can only be used for read by r300 (except query buffers, but
   * those can't be bound by an gallium frontend as vertex buffers). */
      }
      /*****************************************************************************
   * The HWTCL draw functions.                                                 *
   ****************************************************************************/
      static void r300_draw_arrays_immediate(struct r300_context *r300,
               {
      struct pipe_vertex_element* velem;
   struct pipe_vertex_buffer* vbuf;
   unsigned vertex_element_count = r300->velems->count;
            /* Size of the vertex, in dwords. */
            /* The number of dwords for this draw operation. */
            /* Size of the vertex element, in dwords. */
            /* Stride to the same attrib in the next vertex in the vertex buffer,
   * in dwords. */
            /* Mapped vertex buffers. */
   uint32_t* map[PIPE_MAX_ATTRIBS] = {0};
                     if (!r300_prepare_for_rendering(r300, PREP_EMIT_STATES, NULL, dwords, 0, 0, -1))
            /* Calculate the vertex size, offsets, strides etc. and map the buffers. */
   for (i = 0; i < vertex_element_count; i++) {
      velem = &r300->velems->velem[i];
   size[i] = r300->velems->format_size[i] / 4;
   vbi = velem->vertex_buffer_index;
   vbuf = &r300->vertex_buffer[vbi];
            /* Map the buffer. */
   if (!map[vbi]) {
         map[vbi] = (uint32_t*)r300->rws->buffer_map(r300->rws,
      r300_resource(vbuf->buffer.resource)->buf,
      }
                        BEGIN_CS(dwords);
   OUT_CS_REG(R300_VAP_VTX_SIZE, vertex_size);
   OUT_CS_PKT3(R300_PACKET3_3D_DRAW_IMMD_2, draw->count * vertex_size);
   OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_EMBEDDED | (draw->count << 16) |
            /* Emit vertices. */
   for (v = 0; v < draw->count; v++) {
      for (i = 0; i < vertex_element_count; i++) {
            }
      }
      static void r300_emit_draw_arrays(struct r300_context *r300,
               {
      bool alt_num_verts = count > 65535;
            if (count >= (1 << 24)) {
      fprintf(stderr, "r300: Got a huge number of vertices: %i, "
                              BEGIN_CS(2 + (alt_num_verts ? 2 : 0));
   if (alt_num_verts) {
         }
   OUT_CS_PKT3(R300_PACKET3_3D_DRAW_VBUF_2, 0);
   OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (count << 16) |
         r300_translate_primitive(mode) |
      }
      static void r300_emit_draw_elements(struct r300_context *r300,
                                       struct pipe_resource* indexBuffer,
   {
      uint32_t count_dwords, offset_dwords;
   bool alt_num_verts = count > 65535;
            if (count >= (1 << 24)) {
      fprintf(stderr, "r300: Got a huge number of vertices: %i, "
                     DBG(r300, DBG_DRAW, "r300: Indexbuf of %u indices, max %u\n",
                     /* If start is odd, render the first triangle with indices embedded
   * in the command stream. This will increase start by 3 and make it
   * even. We can then proceed without a fallback. */
   if (indexSize == 2 && (start & 1) &&
      mode == MESA_PRIM_TRIANGLES) {
   BEGIN_CS(4);
   OUT_CS_PKT3(R300_PACKET3_3D_DRAW_INDX_2, 2);
   OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (3 << 16) |
         OUT_CS(imm_indices3[1] << 16 | imm_indices3[0]);
   OUT_CS(imm_indices3[2]);
            start += 3;
   count -= 3;
   if (!count)
                        BEGIN_CS(8 + (alt_num_verts ? 2 : 0));
   if (alt_num_verts) {
         }
   OUT_CS_PKT3(R300_PACKET3_3D_DRAW_INDX_2, 0);
   if (indexSize == 4) {
      count_dwords = count;
   OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (count << 16) |
            R300_VAP_VF_CNTL__INDEX_SIZE_32bit |
   } else {
      count_dwords = (count + 1) / 2;
   OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (count << 16) |
                     OUT_CS_PKT3(R300_PACKET3_INDX_BUFFER, 2);
   OUT_CS(R300_INDX_BUFFER_ONE_REG_WR | (R300_VAP_PORT_IDX0 >> 2) |
         OUT_CS(offset_dwords << 2);
   OUT_CS(count_dwords);
   OUT_CS_RELOC(r300_resource(indexBuffer));
      }
      static void r300_draw_elements_immediate(struct r300_context *r300,
               {
      const uint8_t *ptr1;
   const uint16_t *ptr2;
   const uint32_t *ptr4;
   unsigned index_size = info->index_size;
   unsigned i, count_dwords = index_size == 4 ? draw->count :
                  /* 19 dwords for r300_draw_elements_immediate. Give up if the function fails. */
   if (!r300_prepare_for_rendering(r300,
            PREP_EMIT_STATES | PREP_VALIDATE_VBOS | PREP_EMIT_VARRAYS |
                  BEGIN_CS(2 + count_dwords);
            switch (index_size) {
   case 1:
      ptr1 = (uint8_t*)info->index.user;
            OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (draw->count << 16) |
            if (draw->index_bias && !r300->screen->caps.is_r500) {
         for (i = 0; i < draw->count-1; i += 2)
                  if (draw->count & 1)
   } else {
         for (i = 0; i < draw->count-1; i += 2)
                  if (draw->count & 1)
   }
         case 2:
      ptr2 = (uint16_t*)info->index.user;
            OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (draw->count << 16) |
            if (draw->index_bias && !r300->screen->caps.is_r500) {
         for (i = 0; i < draw->count-1; i += 2)
                  if (draw->count & 1)
   } else {
         }
         case 4:
      ptr4 = (uint32_t*)info->index.user;
            OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (draw->count << 16) |
                  if (draw->index_bias && !r300->screen->caps.is_r500) {
         for (i = 0; i < draw->count; i++)
   } else {
         }
      }
      }
      static void r300_draw_elements(struct r300_context *r300,
                     {
      struct pipe_resource *indexBuffer =
         unsigned indexSize = info->index_size;
   struct pipe_resource* orgIndexBuffer = indexBuffer;
   unsigned start = draw->start;
   unsigned count = draw->count;
   bool alt_num_verts = r300->screen->caps.is_r500 &&
         unsigned short_count;
   int buffer_offset = 0, index_offset = 0; /* for index bias emulation */
            if (draw->index_bias && !r300->screen->caps.is_r500) {
      r300_split_index_bias(r300, draw->index_bias, &buffer_offset,
               r300_translate_index_buffer(r300, info, &indexBuffer,
            /* Fallback for misaligned ushort indices. */
   if (indexSize == 2 && (start & 1) && indexBuffer) {
      /* If we got here, then orgIndexBuffer == indexBuffer. */
   uint16_t *ptr = r300->rws->buffer_map(r300->rws, r300_resource(orgIndexBuffer)->buf,
                        if (info->mode == MESA_PRIM_TRIANGLES) {
         } else {
         /* Copy the mapped index buffer directly to the upload buffer.
   * The start index will be aligned simply from the fact that
   * every sub-buffer in the upload buffer is aligned. */
   r300_upload_index_buffer(r300, &indexBuffer, indexSize, &start,
      } else {
      if (info->has_user_indices)
         r300_upload_index_buffer(r300, &indexBuffer, indexSize,
               /* 19 dwords for emit_draw_elements. Give up if the function fails. */
   if (!r300_prepare_for_rendering(r300,
            PREP_EMIT_STATES | PREP_VALIDATE_VBOS | PREP_EMIT_VARRAYS |
   PREP_INDEXED, indexBuffer, 19, buffer_offset, draw->index_bias,
         if (alt_num_verts || count <= 65535) {
      r300_emit_draw_elements(r300, indexBuffer, indexSize,
            } else {
      do {
         /* The maximum must be divisible by 4 and 3,
   * so that quad and triangle lists are split correctly.
   *
                  r300_emit_draw_elements(r300, indexBuffer, indexSize,
                                 /* 15 dwords for emit_draw_elements */
   if (count) {
      if (!r300_prepare_for_rendering(r300,
            PREP_VALIDATE_VBOS | PREP_EMIT_VARRAYS | PREP_INDEXED,
   indexBuffer, 19, buffer_offset, draw->index_bias,
            done:
      if (indexBuffer != orgIndexBuffer) {
            }
      static void r300_draw_arrays(struct r300_context *r300,
                     {
      bool alt_num_verts = r300->screen->caps.is_r500 &&
         unsigned start = draw->start;
   unsigned count = draw->count;
            /* 9 spare dwords for emit_draw_arrays. Give up if the function fails. */
   if (!r300_prepare_for_rendering(r300,
                        if (alt_num_verts || count <= 65535) {
         } else {
      do {
         /* The maximum must be divisible by 4 and 3,
   * so that quad and triangle lists are split correctly.
   *
   * Strips, loops, and fans won't work. */
                                 /* 9 spare dwords for emit_draw_arrays. Give up if the function fails. */
   if (count) {
      if (!r300_prepare_for_rendering(r300,
                        }
      static void r300_draw_arrays_instanced(struct r300_context *r300,
               {
               for (i = 0; i < info->instance_count; i++)
      }
      static void r300_draw_elements_instanced(struct r300_context *r300,
               {
               for (i = 0; i < info->instance_count; i++)
      }
      static unsigned r300_max_vertex_count(struct r300_context *r300)
   {
      unsigned i, nr = r300->velems->count;
   struct pipe_vertex_element *velems = r300->velems->velem;
            for (i = 0; i < nr; i++) {
      struct pipe_vertex_buffer *vb =
                  /* We're not interested in constant and per-instance attribs. */
   if (!vb->buffer.resource ||
      !velems[i].src_stride ||
   velems[i].instance_divisor) {
                        /* Subtract buffer_offset. */
   value = vb->buffer_offset;
   if (value >= size) {
         }
            /* Subtract src_offset. */
   value = velems[i].src_offset;
   if (value >= size) {
         }
            /* Compute the max count. */
   max_count = 1 + size / velems[i].src_stride;
      }
      }
         static void r300_draw_vbo(struct pipe_context* pipe,
                           const struct pipe_draw_info *dinfo,
   {
      if (num_draws > 1) {
      util_draw_multi(pipe, dinfo, drawid_offset, indirect, draws, num_draws);
               struct r300_context* r300 = r300_context(pipe);
   struct pipe_draw_info info = *dinfo;
            if (r300->skip_rendering ||
      !u_trim_pipe_prim(info.mode, &draw.count)) {
               if (r300->sprite_coord_enable != 0 ||
      r300_fs(r300)->shader->inputs.pcoord != ATTR_UNUSED) {
   if ((info.mode == MESA_PRIM_POINTS) != r300->is_point) {
         r300->is_point = !r300->is_point;
                        /* Skip draw if we failed to compile the vertex shader. */
   if (r300_vs(r300)->shader->dummy)
            /* Draw. */
   if (info.index_size) {
               if (!max_count) {
      fprintf(stderr, "r300: Skipping a draw command. There is a buffer "
                     if (max_count == ~0) {
      /* There are no per-vertex vertex elements. Use the hardware maximum. */
                        if (info.instance_count <= 1) {
         if (draw.count <= 8 && info.has_user_indices) {
         } else {
         } else {
            } else {
      if (info.instance_count <= 1) {
         if (immd_is_good_idea(r300, draw.count)) {
         } else {
         } else {
               }
      /****************************************************************************
   * The rest of this file is for SW TCL rendering only. Please be polite and *
   * keep these functions separated so that they are easier to locate. ~C.    *
   ***************************************************************************/
      /* SW TCL elements, using Draw. */
   static void r300_swtcl_draw_vbo(struct pipe_context* pipe,
                                 {
      if (num_draws > 1) {
      util_draw_multi(pipe, info, drawid_offset, indirect, draws, num_draws);
               struct r300_context* r300 = r300_context(pipe);
            if (r300->skip_rendering) {
                  if (!u_trim_pipe_prim(info->mode, &draw.count))
            if (info->index_size) {
      draw_set_indexes(r300->draw,
                                 if (r300->sprite_coord_enable != 0 ||
      r300_fs(r300)->shader->inputs.pcoord != ATTR_UNUSED) {
   if ((info->mode == MESA_PRIM_POINTS) != r300->is_point) {
         r300->is_point = !r300->is_point;
                           draw_vbo(r300->draw, info, drawid_offset, NULL, &draw, 1, 0);
      }
      /* Object for rendering using Draw. */
   struct r300_render {
      /* Parent class */
            /* Pipe context */
            /* Vertex information */
   size_t vertex_size;
   unsigned prim;
            /* VBO */
   size_t vbo_max_used;
      };
      static inline struct r300_render*
   r300_render(struct vbuf_render* render)
   {
         }
      static const struct vertex_info*
   r300_render_get_vertex_info(struct vbuf_render* render)
   {
      struct r300_render* r300render = r300_render(render);
               }
      static bool r300_render_allocate_vertices(struct vbuf_render* render,
               {
      struct r300_render* r300render = r300_render(render);
   struct r300_context* r300 = r300render->r300;
   struct radeon_winsys *rws = r300->rws;
                        pb_reference(&r300->vbo, NULL);
         r300->vbo = NULL;
            r300->vbo = rws->buffer_create(rws,
                           if (!r300->vbo) {
         }
   r300->draw_vbo_offset = 0;
   r300render->vbo_ptr = rws->buffer_map(rws, r300->vbo, &r300->cs,
               r300render->vertex_size = vertex_size;
      }
      static void* r300_render_map_vertices(struct vbuf_render* render)
   {
      struct r300_render* r300render = r300_render(render);
                     assert(r300render->vbo_ptr);
      }
      static void r300_render_unmap_vertices(struct vbuf_render* render,
               {
      struct r300_render* r300render = r300_render(render);
                     r300render->vbo_max_used = MAX2(r300render->vbo_max_used,
      }
      static void r300_render_release_vertices(struct vbuf_render* render)
   {
      struct r300_render* r300render = r300_render(render);
                     r300->draw_vbo_offset += r300render->vbo_max_used;
      }
      static void r300_render_set_primitive(struct vbuf_render* render,
         {
               r300render->prim = prim;
      }
      static void r300_render_draw_arrays(struct vbuf_render* render,
               {
      struct r300_render* r300render = r300_render(render);
   struct r300_context* r300 = r300render->r300;
   uint8_t* ptr;
   unsigned i;
            CS_LOCALS(r300);
            assert(start == 0);
                     if (!r300_prepare_for_rendering(r300,
                              BEGIN_CS(dwords);
   OUT_CS_REG(R300_GA_COLOR_CONTROL,
         OUT_CS_REG(R300_VAP_VF_MAX_VTX_INDX, count - 1);
   OUT_CS_PKT3(R300_PACKET3_3D_DRAW_VBUF_2, 0);
   OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (count << 16) |
            }
      static void r300_render_draw_elements(struct vbuf_render* render,
               {
      struct r300_render* r300render = r300_render(render);
   struct r300_context* r300 = r300render->r300;
   unsigned max_index = (r300->vbo->size - r300->draw_vbo_offset) /
         struct pipe_resource *index_buffer = NULL;
            CS_LOCALS(r300);
            u_upload_data(r300->uploader, 0, count * 2, 4, indices,
         if (!index_buffer) {
                  if (!r300_prepare_for_rendering(r300,
                        pipe_resource_reference(&index_buffer, NULL);
               BEGIN_CS(12);
   OUT_CS_REG(R300_GA_COLOR_CONTROL,
                  OUT_CS_PKT3(R300_PACKET3_3D_DRAW_INDX_2, 0);
   OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (count << 16) |
            OUT_CS_PKT3(R300_PACKET3_INDX_BUFFER, 2);
   OUT_CS(R300_INDX_BUFFER_ONE_REG_WR | (R300_VAP_PORT_IDX0 >> 2));
   OUT_CS(index_buffer_offset);
   OUT_CS((count + 1) / 2);
   OUT_CS_RELOC(r300_resource(index_buffer));
               }
      static void r300_render_destroy(struct vbuf_render* render)
   {
         }
      static struct vbuf_render* r300_render_create(struct r300_context* r300)
   {
                        r300render->base.max_vertex_buffer_bytes = R300_MAX_DRAW_VBO_SIZE;
            r300render->base.get_vertex_info = r300_render_get_vertex_info;
   r300render->base.allocate_vertices = r300_render_allocate_vertices;
   r300render->base.map_vertices = r300_render_map_vertices;
   r300render->base.unmap_vertices = r300_render_unmap_vertices;
   r300render->base.set_primitive = r300_render_set_primitive;
   r300render->base.draw_elements = r300_render_draw_elements;
   r300render->base.draw_arrays = r300_render_draw_arrays;
   r300render->base.release_vertices = r300_render_release_vertices;
               }
      struct draw_stage* r300_draw_stage(struct r300_context* r300)
   {
      struct vbuf_render* render;
                     if (!render) {
                           if (!stage) {
      render->destroy(render);
                           }
      /****************************************************************************
   *                         End of SW TCL functions                          *
   ***************************************************************************/
      /* This functions is used to draw a rectangle for the blitter module.
   *
   * If we rendered a quad, the pixels on the main diagonal
   * would be computed and stored twice, which makes the clear/copy codepaths
   * somewhat inefficient. Instead we use a rectangular point sprite. */
   void r300_blitter_draw_rectangle(struct blitter_context *blitter,
                                       {
      struct r300_context *r300 = r300_context(util_blitter_get_pipe(blitter));
   unsigned last_sprite_coord_enable = r300->sprite_coord_enable;
   unsigned last_is_point = r300->is_point;
   unsigned width = x2 - x1;
   unsigned height = y2 - y1;
   unsigned vertex_size =
         unsigned dwords = 13 + vertex_size +
         static const union blitter_attrib zeros;
            /* XXX workaround for a lockup in MSAA resolve on SWTCL chipsets, this
   * function most probably doesn't handle type=NONE correctly */
   if ((!r300->screen->caps.has_tcl && type == UTIL_BLITTER_ATTRIB_NONE) ||
      type == UTIL_BLITTER_ATTRIB_TEXCOORD_XYZW ||
   num_instances > 1) {
   util_blitter_draw_rectangle(blitter, vertex_elements_cso, get_vs,
                           if (r300->skip_rendering)
            r300->context.bind_vertex_elements_state(&r300->context, vertex_elements_cso);
            if (type == UTIL_BLITTER_ATTRIB_TEXCOORD_XY) {
      r300->sprite_coord_enable = 1;
                        /* Mark some states we don't care about as non-dirty. */
            if (!r300_prepare_for_rendering(r300, PREP_EMIT_STATES, NULL, dwords, 0, 0, -1))
                     BEGIN_CS(dwords);
   /* Set up GA. */
            if (type == UTIL_BLITTER_ATTRIB_TEXCOORD_XY) {
      /* Set up the GA to generate texcoords. */
   OUT_CS_REG(R300_GB_ENABLE, R300_GB_POINT_STUFF_ENABLE |
         OUT_CS_REG_SEQ(R300_GA_POINT_S0, 4);
   OUT_CS_32F(attrib->texcoord.x1);
   OUT_CS_32F(attrib->texcoord.y2);
   OUT_CS_32F(attrib->texcoord.x2);
               /* Set up VAP controls. */
   OUT_CS_REG(R300_VAP_CLIP_CNTL, R300_CLIP_DISABLE);
   OUT_CS_REG(R300_VAP_VTE_CNTL, R300_VTX_XY_FMT | R300_VTX_Z_FMT);
   OUT_CS_REG(R300_VAP_VTX_SIZE, vertex_size);
   OUT_CS_REG_SEQ(R300_VAP_VF_MAX_VTX_INDX, 2);
   OUT_CS(1);
            /* Draw. */
   OUT_CS_PKT3(R300_PACKET3_3D_DRAW_IMMD_2, vertex_size);
   OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_EMBEDDED | (1 << 16) |
            OUT_CS_32F(x1 + width * 0.5f);
   OUT_CS_32F(y1 + height * 0.5f);
   OUT_CS_32F(depth);
            if (vertex_size == 8) {
      if (!attrib)
            }
         done:
      /* Restore the state. */
   r300_mark_atom_dirty(r300, &r300->rs_state);
            r300->sprite_coord_enable = last_sprite_coord_enable;
      }
      void r300_init_render_functions(struct r300_context *r300)
   {
      /* Set draw functions based on presence of HW TCL. */
   if (r300->screen->caps.has_tcl) {
         } else {
                  /* Plug in the two-sided stencil reference value fallback if needed. */
   if (!r300->screen->caps.is_r500)
      }
