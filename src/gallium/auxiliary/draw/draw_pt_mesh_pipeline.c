   /**************************************************************************
   *
   * Copyright 2023 Red Hat.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "draw/draw_context.h"
   #include "draw/draw_private.h"
   #include "draw/draw_pt.h"
   #include "draw/draw_mesh.h"
      /*
   * Mesh shader middle end,
   * This doesn't need a frontend as mesh shader frontend
   * is done with compute shaders in llvmpipe.
   */
      struct mesh_pipeline_middle_end {
      struct draw_pt_middle_end base;
            struct pt_so_emit *so_emit;
      };
      /** cast wrapper */
   static inline struct mesh_pipeline_middle_end *
   mesh_pipeline_middle_end(struct draw_pt_middle_end *middle)
   {
         }
      static void
   mesh_pipeline_destroy(struct draw_pt_middle_end *middle)
   {
               if (mpme->so_emit)
         if (mpme->post_vs)
            }
      static void
   mesh_middle_end_prepare(struct draw_pt_middle_end *middle,
                     {
      struct mesh_pipeline_middle_end *mpme = mesh_pipeline_middle_end(middle);
   struct draw_context *draw = mpme->draw;
   enum mesa_prim out_prim = draw->ms.mesh_shader->output_primitive;
   unsigned point_clip = draw->rasterizer->fill_front == PIPE_POLYGON_MODE_POINT ||
         draw_pt_post_vs_prepare(mpme->post_vs,
                           draw->clip_xy,
   draw->clip_z,
   draw->clip_user,
                           }
      void
   draw_mesh_middle_end_run(struct draw_pt_middle_end *middle,
               {
                        /* just for primgen query */
   draw_pt_so_emit(mpme->so_emit, 1, vert_info, prim_info);
      }
      struct draw_pt_middle_end *
   draw_pt_mesh_pipeline_or_emit(struct draw_context *draw)
   {
      struct mesh_pipeline_middle_end *mpme =
         if (!mpme)
            mpme->base.prepare = mesh_middle_end_prepare;
   mpme->base.destroy = mesh_pipeline_destroy;
            mpme->post_vs = draw_pt_post_vs_create(draw);
   if (!mpme->post_vs)
            mpme->so_emit = draw_pt_so_emit_create(draw);
   if (!mpme->so_emit)
               fail:
      if (mpme)
               }
