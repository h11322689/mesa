   /**************************************************************************
   *
   * Copyright 2010 VMware, Inc.
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
      #include "draw/draw_private.h"
   #include "draw/draw_vs.h"
   #include "draw/draw_gs.h"
   #include "draw/draw_tess.h"
   #include "draw/draw_context.h"
   #include "draw/draw_vbuf.h"
   #include "draw/draw_vertex.h"
   #include "draw/draw_pt.h"
      #include "pipe/p_state.h"
      #include "util/u_math.h"
   #include "util/u_prim.h"
   #include "util/u_memory.h"
         struct pt_so_emit {
               unsigned input_vertex_stride;
   const float (*inputs)[4];
   const float *pre_clip_pos;
   bool has_so;
   bool use_pre_clip_pos;
   int pos_idx;
   unsigned emitted_primitives;
   unsigned generated_primitives;
      };
         static const struct pipe_stream_output_info *
   draw_so_info(const struct draw_context *draw)
   {
               if (draw->ms.mesh_shader)
         if (draw->gs.geometry_shader) {
         } else if (draw->tes.tess_eval_shader) {
         } else {
                     }
      static inline bool
   draw_has_so(const struct draw_context *draw)
   {
               if (state && state->num_outputs > 0)
               }
      void
   draw_pt_so_emit_prepare(struct pt_so_emit *emit, bool use_pre_clip_pos)
   {
               emit->use_pre_clip_pos = use_pre_clip_pos;
   emit->has_so = draw_has_so(draw);
   if (use_pre_clip_pos)
            /* if we have a state with outputs make sure we have
   * buffers to output to */
   if (emit->has_so) {
      bool has_valid_buffer = false;
   for (unsigned i = 0; i < draw->so.num_targets; ++i) {
      if (draw->so.targets[i]) {
      has_valid_buffer = true;
         }
               if (!emit->has_so)
            /* XXX: need to flush to get prim_vbuf.c to release its allocation??
   */
      }
         static void
   so_emit_prim(struct pt_so_emit *so,
               {
      unsigned input_vertex_stride = so->input_vertex_stride;
   struct draw_context *draw = so->draw;
   const float (*input_ptr)[4];
   const float *pcp_ptr = NULL;
   const struct pipe_stream_output_info *state = draw_so_info(draw);
   int buffer_total_bytes[PIPE_MAX_SO_BUFFERS];
            input_ptr = so->inputs;
   if (so->use_pre_clip_pos)
                     for (unsigned i = 0; i < draw->so.num_targets; i++) {
      struct draw_so_target *target = draw->so.targets[i];
   if (target) {
         } else {
                     /* check have we space to emit prim first - if not don't do anything */
   for (unsigned i = 0; i < num_vertices; ++i) {
      for (unsigned slot = 0; slot < state->num_outputs; ++slot) {
      unsigned num_comps = state->output[slot].num_components;
   int ob = state->output[slot].output_buffer;
                  if (state->output[slot].stream != so->stream)
         /* If a buffer is missing then that's equivalent to
   * an overflow */
   if (!draw->so.targets[ob]) {
         }
   if ((buffer_total_bytes[ob] + write_size + dst_offset) >
      draw->so.targets[ob]->target.buffer_size) {
         }
   for (unsigned ob = 0; ob < draw->so.num_targets; ++ob) {
                     for (unsigned i = 0; i < num_vertices; ++i) {
      const float (*input)[4];
            input = (const float (*)[4])(
            if (pcp_ptr)
                  for (unsigned slot = 0; slot < state->num_outputs; ++slot) {
      unsigned idx = state->output[slot].register_index;
   unsigned start_comp = state->output[slot].start_component;
                                                float *buffer = (float *)((char *)draw->so.targets[ob]->mapping +
                        if (idx == so->pos_idx && pcp_ptr && so->stream == 0) {
      memcpy(buffer, &pre_clip_pos[start_comp],
      } else {
      memcpy(buffer, &input[idx][start_comp],
      #if 0
            {
      debug_printf("VERT[%d], stream = %d, offset = %d, slot[%d] sc = %d, num_c = %d, idx = %d = [",
               i, stream,
   for (unsigned j = 0; j < num_comps; ++j) {
      unsigned *ubuffer = (unsigned*)buffer;
      }
   #endif
                  for (unsigned ob = 0; ob < draw->so.num_targets; ++ob) {
      struct draw_so_target *target = draw->so.targets[ob];
   if (target && buffer_written[ob]) {
               }
      }
         static void
   so_point(struct pt_so_emit *so, int idx)
   {
      unsigned indices[1];
   indices[0] = idx;
      }
         static void
   so_line(struct pt_so_emit *so, int i0, int i1)
   {
      unsigned indices[2];
   indices[0] = i0;
   indices[1] = i1;
      }
         static void
   so_tri(struct pt_so_emit *so, int i0, int i1, int i2)
   {
      unsigned indices[3];
   indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;
      }
         #define FUNC         so_run_linear
   #define GET_ELT(idx) (start + (idx))
   #include "draw_so_emit_tmp.h"
         #define FUNC         so_run_elts
   #define LOCAL_VARS   const uint16_t *elts = input_prims->elts;
   #define GET_ELT(idx) (elts[start + (idx)])
   #include "draw_so_emit_tmp.h"
         void
   draw_pt_so_emit(struct pt_so_emit *emit,
                     {
      struct draw_context *draw = emit->draw;
            if (!emit->has_so && num_vertex_streams == 1) {
      if (draw->collect_primgen) {
      unsigned total = 0;
   for (unsigned i = 0; i < input_prims->primitive_count; i++) {
      total +=
      u_decomposed_prims_for_vertices(input_prims->prim,
   }
      }
               if (!emit->has_so && !draw->collect_primgen)
            /* XXX: need to flush to get prim_vbuf.c to release its allocation??*/
            for (unsigned stream = 0; stream < num_vertex_streams; stream++) {
      emit->emitted_primitives = 0;
   emit->generated_primitives = 0;
   if (emit->use_pre_clip_pos)
            emit->input_vertex_stride = input_verts[stream].stride;
   emit->inputs = (const float (*)[4])input_verts[stream].verts->data;
            unsigned start, i;
   for (start = i = 0; i < input_prims[stream].primitive_count;
                     if (input_prims->linear) {
      so_run_linear(emit, &input_prims[stream], &input_verts[stream],
      } else {
      so_run_elts(emit, &input_prims[stream], &input_verts[stream],
         }
   render->set_stream_output_info(render,
                     }
         struct pt_so_emit *
   draw_pt_so_emit_create(struct draw_context *draw)
   {
      struct pt_so_emit *emit = CALLOC_STRUCT(pt_so_emit);
   if (!emit)
                        }
         void
   draw_pt_so_emit_destroy(struct pt_so_emit *emit)
   {
         }
