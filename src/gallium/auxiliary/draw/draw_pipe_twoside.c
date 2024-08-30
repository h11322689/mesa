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
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_shader_tokens.h"
   #include "draw_vs.h"
   #include "draw_pipe.h"
      struct twoside_stage {
      struct draw_stage stage;
   float sign;         /**< +1 or -1 */
   int attrib_front0, attrib_back0;
      };
         static inline struct twoside_stage *
   twoside_stage(struct draw_stage *stage)
   {
         }
         /**
   * Copy back color(s) to front color(s).
   */
   static inline struct vertex_header *
   copy_bfc(struct twoside_stage *twoside,
               {
               if (twoside->attrib_back0 >= 0 && twoside->attrib_front0 >= 0) {
      COPY_4FV(tmp->data[twoside->attrib_front0],
      }
   if (twoside->attrib_back1 >= 0 && twoside->attrib_front1 >= 0) {
      COPY_4FV(tmp->data[twoside->attrib_front1],
                  }
         /* Twoside tri:
   */
   static void
   twoside_tri(struct draw_stage *stage,
         {
               if (header->det * twoside->sign < 0.0) {
      /* this is a back-facing triangle */
            tmp.det = header->det;
   tmp.flags = header->flags;
   tmp.pad = header->pad;
   /* copy back attribs to front attribs */
   tmp.v[0] = copy_bfc(twoside, header->v[0], 0);
   tmp.v[1] = copy_bfc(twoside, header->v[1], 1);
               } else {
            }
         static void
   twoside_first_tri(struct draw_stage *stage,
         {
      struct twoside_stage *twoside = twoside_stage(stage);
            twoside->attrib_front0 = -1;
   twoside->attrib_front1 = -1;
   twoside->attrib_back0 = -1;
            /*
   * Find which vertex shader outputs are front/back colors
   * (only first two can be front or back).
   */
   for (unsigned i = 0; i < vs->info.num_outputs; i++) {
      if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_COLOR) {
      if (vs->info.output_semantic_index[i] == 0)
         else if (vs->info.output_semantic_index[i] == 1)
      }
   if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_BCOLOR) {
      if (vs->info.output_semantic_index[i] == 0)
         else if (vs->info.output_semantic_index[i] == 1)
                  /*
   * We'll multiply the primitive's determinant by this sign to determine
   * if the triangle is back-facing (negative).
   * sign = -1 for CCW, +1 for CW
   */
            stage->tri = twoside_tri;
      }
         static void
   twoside_flush(struct draw_stage *stage, unsigned flags)
   {
      stage->tri = twoside_first_tri;
      }
         static void
   twoside_reset_stipple_counter(struct draw_stage *stage)
   {
         }
         static void
   twoside_destroy(struct draw_stage *stage)
   {
      draw_free_temp_verts(stage);
      }
         /**
   * Create twoside pipeline stage.
   */
   struct draw_stage *
   draw_twoside_stage(struct draw_context *draw)
   {
      struct twoside_stage *twoside = CALLOC_STRUCT(twoside_stage);
   if (!twoside)
            twoside->stage.draw = draw;
   twoside->stage.name = "twoside";
   twoside->stage.next = NULL;
   twoside->stage.point = draw_pipe_passthrough_point;
   twoside->stage.line = draw_pipe_passthrough_line;
   twoside->stage.tri = twoside_first_tri;
   twoside->stage.flush = twoside_flush;
   twoside->stage.reset_stipple_counter = twoside_reset_stipple_counter;
            if (!draw_alloc_temp_verts(&twoside->stage, 3))
                  fail:
      if (twoside)
               }
