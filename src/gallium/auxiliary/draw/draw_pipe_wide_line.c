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
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_shader_tokens.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "draw_private.h"
   #include "draw_pipe.h"
         struct wideline_stage {
         };
         /**
   * Draw a wide line by drawing a quad (two triangles).
   */
   static void
   wideline_line(struct draw_stage *stage,
         {
      const unsigned pos = draw_current_shader_position_output(stage->draw);
                     struct vertex_header *v0 = dup_vert(stage, header->v[0], 0);
   struct vertex_header *v1 = dup_vert(stage, header->v[0], 1);
   struct vertex_header *v2 = dup_vert(stage, header->v[1], 2);
            float *pos0 = v0->data[pos];
   float *pos1 = v1->data[pos];
   float *pos2 = v2->data[pos];
            const float dx = fabsf(pos0[0] - pos2[0]);
            const bool half_pixel_center =
            /* small tweak to meet GL specification */
            /*
   * Draw wide line as a quad (two tris) by "stretching" the line along
   * X or Y.
   * We need to tweak coords in several ways to be conformant here.
            if (dx > dy) {
      /* x-major line */
   pos0[1] = pos0[1] - half_width - bias;
   pos1[1] = pos1[1] + half_width - bias;
   pos2[1] = pos2[1] - half_width - bias;
   pos3[1] = pos3[1] + half_width - bias;
   if (half_pixel_center) {
      if (pos0[0] < pos2[0]) {
      /* left to right line */
   pos0[0] -= 0.5f;
   pos1[0] -= 0.5f;
   pos2[0] -= 0.5f;
      } else {
      /* right to left line */
   pos0[0] += 0.5f;
   pos1[0] += 0.5f;
   pos2[0] += 0.5f;
            } else {
      /* y-major line */
   pos0[0] = pos0[0] - half_width + bias;
   pos1[0] = pos1[0] + half_width + bias;
   pos2[0] = pos2[0] - half_width + bias;
   pos3[0] = pos3[0] + half_width + bias;
   if (half_pixel_center) {
      if (pos0[1] < pos2[1]) {
      /* top to bottom line */
   pos0[1] -= 0.5f;
   pos1[1] -= 0.5f;
   pos2[1] -= 0.5f;
      } else {
      /* bottom to top line */
   pos0[1] += 0.5f;
   pos1[1] += 0.5f;
   pos2[1] += 0.5f;
                     tri.det = header->det;  /* only the sign matters */
   tri.v[0] = v0;
   tri.v[1] = v2;
   tri.v[2] = v3;
            tri.v[0] = v0;
   tri.v[1] = v3;
   tri.v[2] = v1;
      }
         static void
   wideline_first_line(struct draw_stage *stage,
         {
      struct draw_context *draw = stage->draw;
   struct pipe_context *pipe = draw->pipe;
   const struct pipe_rasterizer_state *rast = draw->rasterizer;
            /* Disable triangle culling, stippling, unfilled mode etc. */
   r = draw_get_rasterizer_no_cull(draw, rast);
   draw->suspend_flushing = true;
   pipe->bind_rasterizer_state(pipe, r);
                        }
         static void
   wideline_flush(struct draw_stage *stage, unsigned flags)
   {
      struct draw_context *draw = stage->draw;
            stage->line = wideline_first_line;
            /* restore original rasterizer state */
   if (draw->rast_handle) {
      draw->suspend_flushing = true;
   pipe->bind_rasterizer_state(pipe, draw->rast_handle);
         }
         static void
   wideline_reset_stipple_counter(struct draw_stage *stage)
   {
         }
         static void
   wideline_destroy(struct draw_stage *stage)
   {
      draw_free_temp_verts(stage);
      }
         struct draw_stage *
   draw_wide_line_stage(struct draw_context *draw)
   {
      struct wideline_stage *wide = CALLOC_STRUCT(wideline_stage);
   if (!wide)
            wide->stage.draw = draw;
   wide->stage.name = "wide-line";
   wide->stage.next = NULL;
   wide->stage.point = draw_pipe_passthrough_point;
   wide->stage.line = wideline_first_line;
   wide->stage.tri = draw_pipe_passthrough_tri;
   wide->stage.flush = wideline_flush;
   wide->stage.reset_stipple_counter = wideline_reset_stipple_counter;
            if (!draw_alloc_temp_verts(&wide->stage, 4))
                  fail:
      if (wide)
               }
