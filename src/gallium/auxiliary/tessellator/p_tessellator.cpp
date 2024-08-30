   /**************************************************************************
   *
   * Copyright 2020 Red Hat.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **************************************************************************/
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "pipe/p_defines.h"
   #include "p_tessellator.h"
   #include "tessellator.hpp"
      #include <new>
      namespace pipe_tessellator_wrap
   {
      /// Wrapper class for the CHWTessellator reference tessellator from MSFT
   /// This class will store data not originally stored in CHWTessellator
   class pipe_ts : private CHWTessellator
   {
   private:
      typedef CHWTessellator SUPER;
   enum mesa_prim    prim_mode;
   alignas(32) float      domain_points_u[MAX_POINT_COUNT];
   alignas(32) float      domain_points_v[MAX_POINT_COUNT];
         public:
      void Init(enum mesa_prim tes_prim_mode,
               {
      static PIPE_TESSELLATOR_PARTITIONING CVT_TS_D3D_PARTITIONING[] = {
                              PIPE_TESSELLATOR_OUTPUT_PRIMITIVE out_prim;
   if (tes_point_mode)
         else if (tes_prim_mode == MESA_PRIM_LINES)
         else if (tes_vertex_order_cw)
                                       prim_mode          = tes_prim_mode;
               void Tessellate(const struct pipe_tessellation_factors *tess_factors,
         {
      switch (prim_mode)
      {
   case MESA_PRIM_QUADS:
      SUPER::TessellateQuadDomain(
                                             case MESA_PRIM_TRIANGLES:
      SUPER::TessellateTriDomain(
                                 case MESA_PRIM_LINES:
      SUPER::TessellateIsoLineDomain(
                     default:
                              DOMAIN_POINT *points = SUPER::GetPoints();
   for (uint32_t i = 0; i < num_domain_points; i++) {
      domain_points_u[i] = points[i].u;
      }
   tess_data->num_domain_points = num_domain_points;
                                    } // namespace Tessellator
      /* allocate tessellator */
   struct pipe_tessellator *
   p_tess_init(enum mesa_prim tes_prim_mode,
               {
      void *mem;
                                          }
      /* destroy tessellator */
   void p_tess_destroy(struct pipe_tessellator *pipe_tess)
   {
      using pipe_tessellator_wrap::pipe_ts;
            tessellator->~pipe_ts();
      }
      /* perform tessellation */
   void p_tessellate(struct pipe_tessellator *pipe_tess,
               {
      using pipe_tessellator_wrap::pipe_ts;
               }
   