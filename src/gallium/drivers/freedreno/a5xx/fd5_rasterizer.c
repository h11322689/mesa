   /*
   * Copyright (C) 2016 Rob Clark <robclark@freedesktop.org>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "pipe/p_state.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "fd5_context.h"
   #include "fd5_format.h"
   #include "fd5_rasterizer.h"
      void *
   fd5_rasterizer_state_create(struct pipe_context *pctx,
         {
      struct fd5_rasterizer_stateobj *so;
            so = CALLOC_STRUCT(fd5_rasterizer_stateobj);
   if (!so)
                     if (cso->point_size_per_vertex) {
      psize_min = util_get_min_point_size(cso);
      } else {
      /* Force the point size to be as if the vertex output was disabled. */
   psize_min = cso->point_size;
               so->gras_su_point_minmax = A5XX_GRAS_SU_POINT_MINMAX_MIN(psize_min) |
         so->gras_su_point_size = A5XX_GRAS_SU_POINT_SIZE(cso->point_size);
   so->gras_su_poly_offset_scale =
         so->gras_su_poly_offset_offset =
         so->gras_su_poly_offset_clamp =
            so->gras_su_cntl = A5XX_GRAS_SU_CNTL_LINEHALFWIDTH(cso->line_width / 2.0f);
   so->pc_raster_cntl =
      A5XX_PC_RASTER_CNTL_POLYMODE_FRONT_PTYPE(
               if (cso->fill_front != PIPE_POLYGON_MODE_FILL ||
      cso->fill_back != PIPE_POLYGON_MODE_FILL)
         if (cso->cull_face & PIPE_FACE_FRONT)
         if (cso->cull_face & PIPE_FACE_BACK)
         if (!cso->front_ccw)
         if (cso->offset_tri)
            if (!cso->flatshade_first)
         //   if (!cso->depth_clip)
   //      so->gras_cl_clip_cntl |=
   //            A5XX_GRAS_CL_CLIP_CNTL_ZNEAR_CLIP_DISABLE |
   //            A5XX_GRAS_CL_CLIP_CNTL_ZFAR_CLIP_DISABLE;
      if (cso->clip_halfz)
               }
