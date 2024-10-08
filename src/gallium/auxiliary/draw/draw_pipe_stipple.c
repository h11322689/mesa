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
      /* Authors:  Keith Whitwell <keithw@vmware.com>
   */
      /* Implement line stipple by cutting lines up into smaller lines.
   * There are hundreds of ways to implement line stipple, this is one
   * choice that should work in all situations, requires no state
   * manipulations, but with a penalty in terms of large amounts of
   * generated geometry.
   */
         #include "pipe/p_defines.h"
   #include "pipe/p_shader_tokens.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "draw/draw_pipe.h"
         /** Subclass of draw_stage */
   struct stipple_stage {
      struct draw_stage stage;
   unsigned counter;
   uint16_t pattern;
   uint16_t factor;
      };
         static inline struct stipple_stage *
   stipple_stage(struct draw_stage *stage)
   {
         }
         /**
   * Compute interpolated vertex attributes for 'dst' at position 't'
   * between 'v0' and 'v1'.
   * XXX using linear interpolation for all attribs at this time.
   */
   static void
   screen_interp(struct draw_context *draw,
               struct vertex_header *dst,
   float t,
   {
      unsigned attr;
   unsigned num_outputs = draw_current_shader_outputs(draw);
   for (attr = 0; attr < num_outputs; attr++) {
      const float *val0 = v0->data[attr];
   const float *val1 = v1->data[attr];
   float *newv = dst->data[attr];
   unsigned i;
   for (i = 0; i < 4; i++) {
               }
         static void
   emit_segment(struct draw_stage *stage, struct prim_header *header,
         {
      struct vertex_header *v0new = dup_vert(stage, header->v[0], 0);
   struct vertex_header *v1new = dup_vert(stage, header->v[1], 1);
            if (t0 > 0.0) {
      screen_interp(stage->draw, v0new, t0, header->v[0], header->v[1]);
               if (t1 < 1.0) {
      screen_interp(stage->draw, v1new, t1, header->v[0], header->v[1]);
                  }
         static inline bool
   stipple_test(unsigned counter, uint16_t pattern, uint16_t factor)
   {
      unsigned b = (counter / factor) & 0xf;
      }
         static void
   stipple_line(struct draw_stage *stage, struct prim_header *header)
   {
      struct stipple_stage *stipple = stipple_stage(stage);
   struct vertex_header *v0 = header->v[0];
   struct vertex_header *v1 = header->v[1];
   const unsigned pos = draw_current_shader_position_output(stage->draw);
   const float *pos0 = v0->data[pos];
   const float *pos1 = v1->data[pos];
   float start = 0;
            float x0 = pos0[0];
   float x1 = pos1[0];
   float y0 = pos0[1];
            float length;
   int i;
            if (header->flags & DRAW_PIPE_RESET_STIPPLE)
            if (stipple->rectangular) {
      float dx = x1 - x0;
   float dy = y1 - y0;
      } else {
      float dx = x0 > x1 ? x0 - x1 : x1 - x0;
   float dy = y0 > y1 ? y0 - y1 : y1 - y0;
               if (util_is_inf_or_nan(length))
         else
            /* XXX ToDo: instead of iterating pixel-by-pixel, use a look-up table.
   */
   for (i = 0; i < intlength; i++) {
      bool result = stipple_test(stipple->counter + i,
         if (result != state) {
      /* changing from "off" to "on" or vice versa */
   if (state) {
      /* finishing an "on" segment */
      }
   else {
      /* starting an "on" segment */
      }
                  if (state && start < length)
               }
         static void
   reset_stipple_counter(struct draw_stage *stage)
   {
      struct stipple_stage *stipple = stipple_stage(stage);
   stipple->counter = 0;
      }
      static void
   stipple_reset_point(struct draw_stage *stage, struct prim_header *header)
   {
      struct stipple_stage *stipple = stipple_stage(stage);
   stipple->counter = 0;
      }
      static void
   stipple_reset_tri(struct draw_stage *stage, struct prim_header *header)
   {
      struct stipple_stage *stipple = stipple_stage(stage);
   stipple->counter = 0;
      }
         static void
   stipple_first_line(struct draw_stage *stage,
         {
      struct stipple_stage *stipple = stipple_stage(stage);
            stipple->pattern = draw->rasterizer->line_stipple_pattern;
   stipple->factor = draw->rasterizer->line_stipple_factor + 1;
            stage->line = stipple_line;
      }
         static void
   stipple_flush(struct draw_stage *stage, unsigned flags)
   {
      stage->line = stipple_first_line;
      }
         static void
   stipple_destroy(struct draw_stage *stage)
   {
      draw_free_temp_verts(stage);
      }
         /**
   * Create line stippler stage
   */
   struct draw_stage *
   draw_stipple_stage(struct draw_context *draw)
   {
      struct stipple_stage *stipple = CALLOC_STRUCT(stipple_stage);
   if (!stipple)
            stipple->stage.draw = draw;
   stipple->stage.name = "stipple";
   stipple->stage.next = NULL;
   stipple->stage.point = stipple_reset_point;
   stipple->stage.line = stipple_first_line;
   stipple->stage.tri = stipple_reset_tri;
   stipple->stage.reset_stipple_counter = reset_stipple_counter;
   stipple->stage.flush = stipple_flush;
            if (!draw_alloc_temp_verts(&stipple->stage, 2))
                  fail:
      if (stipple)
               }
