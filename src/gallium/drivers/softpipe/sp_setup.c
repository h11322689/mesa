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
      /**
   * \brief  Primitive rasterization/rendering (points, lines, triangles)
   *
   * \author  Keith Whitwell <keithw@vmware.com>
   * \author  Brian Paul
   */
      #include "sp_context.h"
   #include "sp_screen.h"
   #include "sp_quad.h"
   #include "sp_quad_pipe.h"
   #include "sp_setup.h"
   #include "sp_state.h"
   #include "draw/draw_context.h"
   #include "pipe/p_shader_tokens.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
         #define DEBUG_VERTS 0
   #define DEBUG_FRAGS 0
         /**
   * Triangle edge info
   */
   struct edge {
      float dx;		/**< X(v1) - X(v0), used only during setup */
   float dy;		/**< Y(v1) - Y(v0), used only during setup */
   float dxdy;		/**< dx/dy */
   float sx, sy;	/**< first sample point coord */
      };
         /**
   * Max number of quads (2x2 pixel blocks) to process per batch.
   * This can't be arbitrarily increased since we depend on some 32-bit
   * bitmasks (two bits per quad).
   */
   #define MAX_QUADS 16
         /**
   * Triangle setup info.
   * Also used for line drawing (taking some liberties).
   */
   struct setup_context {
               /* Vertices are just an array of floats making up each attribute in
   * turn.  Currently fixed at 4 floats, but should change in time.
   * Codegen will help cope with this.
   */
   const float (*vmax)[4];
   const float (*vmid)[4];
   const float (*vmin)[4];
            struct edge ebot;
   struct edge etop;
            float oneoverarea;
            float pixel_offset;
            struct quad_header quad[MAX_QUADS];
   struct quad_header *quad_ptrs[MAX_QUADS];
            struct tgsi_interp_coef coef[PIPE_MAX_SHADER_INPUTS];
            struct {
      int left[2];   /**< [0] = row0, [1] = row1 */
   int right[2];
            #if DEBUG_FRAGS
      uint numFragsEmitted;  /**< per primitive */
      #endif
         unsigned cull_face;		/* which faces cull */
      };
                        /**
   * Clip setup->quad against the scissor/surface bounds.
   */
   static inline void
   quad_clip(struct setup_context *setup, struct quad_header *quad)
   {
      unsigned viewport_index = quad[0].input.viewport_index;
   const struct pipe_scissor_state *cliprect = &setup->softpipe->cliprect[viewport_index];
   const int minx = (int) cliprect->minx;
   const int maxx = (int) cliprect->maxx;
   const int miny = (int) cliprect->miny;
            if (quad->input.x0 >= maxx ||
      quad->input.y0 >= maxy ||
   quad->input.x0 + 1 < minx ||
   quad->input.y0 + 1 < miny) {
   /* totally clipped */
   quad->inout.mask = 0x0;
      }
   if (quad->input.x0 < minx)
         if (quad->input.y0 < miny)
         if (quad->input.x0 == maxx - 1)
         if (quad->input.y0 == maxy - 1)
      }
         /**
   * Emit a quad (pass to next stage) with clipping.
   */
   static inline void
   clip_emit_quad(struct setup_context *setup, struct quad_header *quad)
   {
               if (quad->inout.mask) {
         #if DEBUG_FRAGS
         #endif
                  }
            /**
   * Given an X or Y coordinate, return the block/quad coordinate that it
   * belongs to.
   */
   static inline int
   block(int x)
   {
         }
         static inline int
   block_x(int x)
   {
         }
         /**
   * Render a horizontal span of quads
   */
   static void
   flush_spans(struct setup_context *setup)
   {
      const int step = MAX_QUADS;
   const int xleft0 = setup->span.left[0];
   const int xleft1 = setup->span.left[1];
   const int xright0 = setup->span.right[0];
   const int xright1 = setup->span.right[1];
            const int minleft = block_x(MIN2(xleft0, xleft1));
   const int maxright = MAX2(xright0, xright1);
            /* process quads in horizontal chunks of 16 */
   for (x = minleft; x < maxright; x += step) {
      unsigned skip_left0 = CLAMP(xleft0 - x, 0, step);
   unsigned skip_left1 = CLAMP(xleft1 - x, 0, step);
   unsigned skip_right0 = CLAMP(x + step - xright0, 0, step);
   unsigned skip_right1 = CLAMP(x + step - xright1, 0, step);
   unsigned lx = x;
            unsigned skipmask_left0 = (1U << skip_left0) - 1U;
            /* These calculations fail when step == 32 and skip_right == 0.
   */
   unsigned skipmask_right0 = ~0U << (unsigned)(step - skip_right0);
            unsigned mask0 = ~skipmask_left0 & ~skipmask_right0;
            if (mask0 | mask1) {
      do {
      unsigned quadmask = (mask0 & 3) | ((mask1 & 3) << 2);
   if (quadmask) {
      setup->quad[q].input.x0 = lx;
   setup->quad[q].input.y0 = setup->span.y;
   setup->quad[q].input.facing = setup->facing;
      #if DEBUG_FRAGS
         #endif
               }
   mask0 >>= 2;
   mask1 >>= 2;
                                 setup->span.y = 0;
   setup->span.right[0] = 0;
   setup->span.right[1] = 0;
   setup->span.left[0] = 1000000;     /* greater than right[0] */
      }
         #if DEBUG_VERTS
   static void
   print_vertex(const struct setup_context *setup,
         {
      int i;
   debug_printf("   Vertex: (%p)\n", (void *) v);
   for (i = 0; i < setup->nr_vertex_attrs; i++) {
      debug_printf("     %d: %f %f %f %f\n",  i,
         if (util_is_inf_or_nan(v[i][0])) {
               }
   #endif
         /**
   * Sort the vertices from top to bottom order, setting up the triangle
   * edge fields (ebot, emaj, etop).
   * \return FALSE if coords are inf/nan (cull the tri), TRUE otherwise
   */
   static bool
   setup_sort_vertices(struct setup_context *setup,
                     float det,
   {
      if (setup->softpipe->rasterizer->flatshade_first)
         else
            /* determine bottom to top order of vertices */
   {
      float y0 = v0[0][1];
   float y1 = v1[0][1];
   float y2 = v2[0][1];
   if (y1 <= y2) {
      /* y0<=y1<=y2 */
   setup->vmin = v0;
   setup->vmid = v1;
      }
   else if (y2 <= y0) {
      /* y2<=y0<=y1 */
   setup->vmin = v2;
   setup->vmid = v0;
      }
   else {
      /* y0<=y2<=y1 */
   setup->vmin = v0;
   setup->vmid = v2;
      }
         }
   if (y0 <= y2) {
      /* y1<=y0<=y2 */
   setup->vmin = v1;
   setup->vmid = v0;
      }
   else if (y2 <= y1) {
      /* y2<=y1<=y0 */
   setup->vmin = v2;
   setup->vmid = v1;
      }
   else {
      /* y1<=y2<=y0 */
   setup->vmin = v1;
   setup->vmid = v2;
      }
                     setup->ebot.dx = setup->vmid[0][0] - setup->vmin[0][0];
   setup->ebot.dy = setup->vmid[0][1] - setup->vmin[0][1];
   setup->emaj.dx = setup->vmax[0][0] - setup->vmin[0][0];
   setup->emaj.dy = setup->vmax[0][1] - setup->vmin[0][1];
   setup->etop.dx = setup->vmax[0][0] - setup->vmid[0][0];
            /*
   * Compute triangle's area.  Use 1/area to compute partial
   * derivatives of attributes later.
   *
   * The area will be the same as prim->det, but the sign may be
   * different depending on how the vertices get sorted above.
   *
   * To determine whether the primitive is front or back facing we
   * use the prim->det value because its sign is correct.
   */
   {
      const float area = (setup->emaj.dx * setup->ebot.dy -
                     /*
   debug_printf("%s one-over-area %f  area %f  det %f\n",
         */
   if (util_is_inf_or_nan(setup->oneoverarea))
               /* We need to know if this is a front or back-facing triangle for:
   *  - the GLSL gl_FrontFacing fragment attribute (bool)
   *  - two-sided stencil test
   * 0 = front-facing, 1 = back-facing
   */
   setup->facing = 
      ((det < 0.0) ^ 
         {
               return false;
                  }
         /**
   * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
   * The value value comes from vertex[slot][i].
   * The result will be put into setup->coef[slot].a0[i].
   * \param slot  which attribute slot
   * \param i  which component of the slot (0..3)
   */
   static void
   const_coeff(struct setup_context *setup,
               {
               coef->dadx[i] = 0;
            /* need provoking vertex info!
   */
      }
         /**
   * Compute a0, dadx and dady for a linearly interpolated coefficient,
   * for a triangle.
   * v[0], v[1] and v[2] are vmin, vmid and vmax, respectively.
   */
   static void
   tri_linear_coeff(struct setup_context *setup,
                     {
      float botda = v[1] - v[0];
   float majda = v[2] - v[0];
   float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
   float dadx = a * setup->oneoverarea;
                     coef->dadx[i] = dadx;
            /* calculate a0 as the value which would be sampled for the
   * fragment at (0,0), taking into account that we want to sample at
   * pixel centers, in other words (pixel_offset, pixel_offset).
   *
   * this is neat but unfortunately not a good way to do things for
   * triangles with very large values of dadx or dady as it will
   * result in the subtraction and re-addition from a0 of a very
   * large number, which means we'll end up loosing a lot of the
   * fractional bits and precision from a0.  the way to fix this is
   * to define a0 as the sample at a pixel center somewhere near vmin
   * instead - i'll switch to this later.
   */
   coef->a0[i] = (v[0] -
            }
         /**
   * Compute a0, dadx and dady for a perspective-corrected interpolant,
   * for a triangle.
   * We basically multiply the vertex value by 1/w before computing
   * the plane coefficients (a0, dadx, dady).
   * Later, when we compute the value at a particular fragment position we'll
   * divide the interpolated value by the interpolated W at that fragment.
   * v[0], v[1] and v[2] are vmin, vmid and vmax, respectively.
   */
   static void
   tri_persp_coeff(struct setup_context *setup,
                     {
      /* premultiply by 1/w  (v[0][3] is always W):
   */
   float mina = v[0] * setup->vmin[0][3];
   float mida = v[1] * setup->vmid[0][3];
   float maxa = v[2] * setup->vmax[0][3];
   float botda = mida - mina;
   float majda = maxa - mina;
   float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
   float dadx = a * setup->oneoverarea;
                     coef->dadx[i] = dadx;
   coef->dady[i] = dady;
   coef->a0[i] = (mina -
            }
         /**
   * Special coefficient setup for gl_FragCoord.
   * X and Y are trivial, though Y may have to be inverted for OpenGL.
   * Z and W are copied from posCoef which should have already been computed.
   * We could do a bit less work if we'd examine gl_FragCoord's swizzle mask.
   */
   static void
   setup_fragcoord_coeff(struct setup_context *setup, uint slot)
   {
      const struct tgsi_shader_info *fsInfo = &setup->softpipe->fs_variant->info;
   bool origin_lower_left =
         bool pixel_center_integer =
            /*X*/
   setup->coef[slot].a0[0] = pixel_center_integer ? 0.0f : 0.5f;
   setup->coef[slot].dadx[0] = 1.0f;
   setup->coef[slot].dady[0] = 0.0f;
   /*Y*/
   setup->coef[slot].a0[1] =
   (origin_lower_left ? setup->softpipe->framebuffer.height-1 : 0)
   + (pixel_center_integer ? 0.0f : 0.5f);
   setup->coef[slot].dadx[1] = 0.0f;
   setup->coef[slot].dady[1] = origin_lower_left ? -1.0f : 1.0f;
   /*Z*/
   setup->coef[slot].a0[2] = setup->posCoef.a0[2];
   setup->coef[slot].dadx[2] = setup->posCoef.dadx[2];
   setup->coef[slot].dady[2] = setup->posCoef.dady[2];
   /*W*/
   setup->coef[slot].a0[3] = setup->posCoef.a0[3];
   setup->coef[slot].dadx[3] = setup->posCoef.dadx[3];
      }
            /**
   * Compute the setup->coef[] array dadx, dady, a0 values.
   * Must be called after setup->vmin,vmid,vmax,vprovoke are initialized.
   */
   static void
   setup_tri_coefficients(struct setup_context *setup)
   {
      struct softpipe_context *softpipe = setup->softpipe;
   const struct tgsi_shader_info *fsInfo = &setup->softpipe->fs_variant->info;
   const struct sp_setup_info *sinfo = &softpipe->setup_info;
   uint fragSlot;
                     /* z and w are done by linear interpolation:
   */
   v[0] = setup->vmin[0][2];
   v[1] = setup->vmid[0][2];
   v[2] = setup->vmax[0][2];
            v[0] = setup->vmin[0][3];
   v[1] = setup->vmid[0][3];
   v[2] = setup->vmax[0][3];
            /* setup interpolation for all the remaining attributes:
   */
   for (fragSlot = 0; fragSlot < fsInfo->num_inputs; fragSlot++) {
      const uint vertSlot = sinfo->attrib[fragSlot].src_index;
            switch (sinfo->attrib[fragSlot].interp) {
   case SP_INTERP_CONSTANT:
      for (j = 0; j < TGSI_NUM_CHANNELS; j++) {
         }
      case SP_INTERP_LINEAR:
      for (j = 0; j < TGSI_NUM_CHANNELS; j++) {
      v[0] = setup->vmin[vertSlot][j];
   v[1] = setup->vmid[vertSlot][j];
   v[2] = setup->vmax[vertSlot][j];
      }
      case SP_INTERP_PERSPECTIVE:
      for (j = 0; j < TGSI_NUM_CHANNELS; j++) {
      v[0] = setup->vmin[vertSlot][j];
   v[1] = setup->vmid[vertSlot][j];
   v[2] = setup->vmax[vertSlot][j];
      }
      case SP_INTERP_POS:
      setup_fragcoord_coeff(setup, fragSlot);
      default:
                  if (fsInfo->input_semantic_name[fragSlot] == TGSI_SEMANTIC_FACE) {
      /* convert 0 to 1.0 and 1 to -1.0 */
   setup->coef[fragSlot].a0[0] = setup->facing * -2.0f + 1.0f;
   setup->coef[fragSlot].dadx[0] = 0.0;
               if (0) {
      for (j = 0; j < TGSI_NUM_CHANNELS; j++) {
      debug_printf("attr[%d].%c: a0:%f dx:%f dy:%f\n",
               fragSlot, "xyzw"[j],
               }
         static void
   setup_tri_edges(struct setup_context *setup)
   {
      float vmin_x = setup->vmin[0][0] + setup->pixel_offset;
            float vmin_y = setup->vmin[0][1] - setup->pixel_offset;
   float vmid_y = setup->vmid[0][1] - setup->pixel_offset;
            setup->emaj.sy = ceilf(vmin_y);
   setup->emaj.lines = (int) ceilf(vmax_y - setup->emaj.sy);
   setup->emaj.dxdy = setup->emaj.dy ? setup->emaj.dx / setup->emaj.dy : .0f;
            setup->etop.sy = ceilf(vmid_y);
   setup->etop.lines = (int) ceilf(vmax_y - setup->etop.sy);
   setup->etop.dxdy = setup->etop.dy ? setup->etop.dx / setup->etop.dy : .0f;
            setup->ebot.sy = ceilf(vmin_y);
   setup->ebot.lines = (int) ceilf(vmid_y - setup->ebot.sy);
   setup->ebot.dxdy = setup->ebot.dy ? setup->ebot.dx / setup->ebot.dy : .0f;
      }
         /**
   * Render the upper or lower half of a triangle.
   * Scissoring/cliprect is applied here too.
   */
   static void
   subtriangle(struct setup_context *setup,
               struct edge *eleft,
   struct edge *eright,
   {
      const struct pipe_scissor_state *cliprect = &setup->softpipe->cliprect[viewport_index];
   const int minx = (int) cliprect->minx;
   const int maxx = (int) cliprect->maxx;
   const int miny = (int) cliprect->miny;
   const int maxy = (int) cliprect->maxy;
   int y, start_y, finish_y;
            assert((int)eleft->sy == (int) eright->sy);
            /* clip top/bottom */
   start_y = sy;
   if (start_y < miny)
            finish_y = sy + lines;
   if (finish_y > maxy)
            start_y -= sy;
            /*
   debug_printf("%s %d %d\n", __func__, start_y, finish_y);
                        /* avoid accumulating adds as floats don't have the precision to
   * accurately iterate large triangle edges that way.  luckily we
   * can just multiply these days.
   *
   * this is all drowned out by the attribute interpolation anyway.
   */
   int left = (int)(eleft->sx + y * eleft->dxdy);
            /* clip left/right */
   if (left < minx)
         if (right > maxx)
            if (left < right) {
      int _y = sy + y;
   if (block(_y) != setup->span.y) {
      flush_spans(setup);
               setup->span.left[_y&1] = left;
                     /* save the values so that emaj can be restarted:
   */
   eleft->sx += lines * eleft->dxdy;
   eright->sx += lines * eright->dxdy;
   eleft->sy += lines;
      }
         /**
   * Recalculate prim's determinant.  This is needed as we don't have
   * get this information through the vbuf_render interface & we must
   * calculate it here.
   */
   static float
   calc_det(const float (*v0)[4],
               {
      /* edge vectors e = v0 - v2, f = v1 - v2 */
   const float ex = v0[0][0] - v2[0][0];
   const float ey = v0[0][1] - v2[0][1];
   const float fx = v1[0][0] - v2[0][0];
            /* det = cross(e,f).z */
      }
         /**
   * Do setup for triangle rasterization, then render the triangle.
   */
   void
   sp_setup_tri(struct setup_context *setup,
               const float (*v0)[4],
   {
      float det;
   uint layer = 0;
      #if DEBUG_VERTS
      debug_printf("Setup triangle:\n");
   print_vertex(setup, v0);
   print_vertex(setup, v1);
      #endif
         if (unlikely(sp_debug & SP_DBG_NO_RAST) ||
      setup->softpipe->rasterizer->rasterizer_discard)
         det = calc_det(v0, v1, v2);
   /*
   debug_printf("%s\n", __func__ );
         #if DEBUG_FRAGS
      setup->numFragsEmitted = 0;
      #endif
         if (!setup_sort_vertices( setup, det, v0, v1, v2 ))
            setup_tri_coefficients( setup );
                     setup->span.y = 0;
   setup->span.right[0] = 0;
   setup->span.right[1] = 0;
   /*   setup->span.z_mode = tri_z_mode( setup->ctx ); */
   if (setup->softpipe->layer_slot > 0) {
      layer = *(unsigned *)setup->vprovoke[setup->softpipe->layer_slot];
      }
            if (setup->softpipe->viewport_index_slot > 0) {
      unsigned *udata = (unsigned*)v0[setup->softpipe->viewport_index_slot];
      }
                     if (setup->oneoverarea < 0.0) {
      /* emaj on left:
   */
   subtriangle(setup, &setup->emaj, &setup->ebot, setup->ebot.lines, viewport_index);
      }
   else {
      /* emaj on right:
   */
   subtriangle(setup, &setup->ebot, &setup->emaj, setup->ebot.lines, viewport_index);
                        if (setup->softpipe->active_statistics_queries) {
               #if DEBUG_FRAGS
      printf("Tri: %u frags emitted, %u written\n",
            #endif
   }
         /**
   * Compute a0, dadx and dady for a linearly interpolated coefficient,
   * for a line.
   * v[0] and v[1] are vmin and vmax, respectively.
   */
   static void
   line_linear_coeff(const struct setup_context *setup,
                     {
      const float da = v[1] - v[0];
   const float dadx = da * setup->emaj.dx * setup->oneoverarea;
   const float dady = da * setup->emaj.dy * setup->oneoverarea;
   coef->dadx[i] = dadx;
   coef->dady[i] = dady;
   coef->a0[i] = (v[0] -
            }
         /**
   * Compute a0, dadx and dady for a perspective-corrected interpolant,
   * for a line.
   * v[0] and v[1] are vmin and vmax, respectively.
   */
   static void
   line_persp_coeff(const struct setup_context *setup,
                     {
      const float a0 = v[0] * setup->vmin[0][3];
   const float a1 = v[1] * setup->vmax[0][3];
   const float da = a1 - a0;
   const float dadx = da * setup->emaj.dx * setup->oneoverarea;
   const float dady = da * setup->emaj.dy * setup->oneoverarea;
   coef->dadx[i] = dadx;
   coef->dady[i] = dady;
   coef->a0[i] = (a0 -
            }
         /**
   * Compute the setup->coef[] array dadx, dady, a0 values.
   * Must be called after setup->vmin,vmax are initialized.
   */
   static bool
   setup_line_coefficients(struct setup_context *setup,
               {
      struct softpipe_context *softpipe = setup->softpipe;
   const struct tgsi_shader_info *fsInfo = &setup->softpipe->fs_variant->info;
   const struct sp_setup_info *sinfo = &softpipe->setup_info;
   uint fragSlot;
   float area;
                     /* use setup->vmin, vmax to point to vertices */
   if (softpipe->rasterizer->flatshade_first)
         else
         setup->vmin = v0;
            setup->emaj.dx = setup->vmax[0][0] - setup->vmin[0][0];
            /* NOTE: this is not really area but something proportional to it */
   area = setup->emaj.dx * setup->emaj.dx + setup->emaj.dy * setup->emaj.dy;
   if (area == 0.0f || util_is_inf_or_nan(area))
                  /* z and w are done by linear interpolation:
   */
   v[0] = setup->vmin[0][2];
   v[1] = setup->vmax[0][2];
            v[0] = setup->vmin[0][3];
   v[1] = setup->vmax[0][3];
            /* setup interpolation for all the remaining attributes:
   */
   for (fragSlot = 0; fragSlot < fsInfo->num_inputs; fragSlot++) {
      const uint vertSlot = sinfo->attrib[fragSlot].src_index;
            switch (sinfo->attrib[fragSlot].interp) {
   case SP_INTERP_CONSTANT:
      for (j = 0; j < TGSI_NUM_CHANNELS; j++)
            case SP_INTERP_LINEAR:
      for (j = 0; j < TGSI_NUM_CHANNELS; j++) {
      v[0] = setup->vmin[vertSlot][j];
   v[1] = setup->vmax[vertSlot][j];
      }
      case SP_INTERP_PERSPECTIVE:
      for (j = 0; j < TGSI_NUM_CHANNELS; j++) {
      v[0] = setup->vmin[vertSlot][j];
   v[1] = setup->vmax[vertSlot][j];
      }
      case SP_INTERP_POS:
      setup_fragcoord_coeff(setup, fragSlot);
      default:
                  if (fsInfo->input_semantic_name[fragSlot] == TGSI_SEMANTIC_FACE) {
      /* convert 0 to 1.0 and 1 to -1.0 */
   setup->coef[fragSlot].a0[0] = setup->facing * -2.0f + 1.0f;
   setup->coef[fragSlot].dadx[0] = 0.0;
         }
      }
         /**
   * Plot a pixel in a line segment.
   */
   static inline void
   plot(struct setup_context *setup, int x, int y)
   {
      const int iy = y & 1;
   const int ix = x & 1;
   const int quadX = x - ix;
   const int quadY = y - iy;
            if (quadX != setup->quad[0].input.x0 ||
         {
               if (setup->quad[0].input.x0 != -1)
            setup->quad[0].input.x0 = quadX;
   setup->quad[0].input.y0 = quadY;
                  }
         /**
   * Do setup for line rasterization, then render the line.
   * Single-pixel width, no stipple, etc.  We rely on the 'draw' module
   * to handle stippling and wide lines.
   */
   void
   sp_setup_line(struct setup_context *setup,
               {
      int x0 = (int) v0[0][0];
   int x1 = (int) v1[0][0];
   int y0 = (int) v0[0][1];
   int y1 = (int) v1[0][1];
   int dx = x1 - x0;
   int dy = y1 - y0;
   int xstep, ystep;
   uint layer = 0;
         #if DEBUG_VERTS
      debug_printf("Setup line:\n");
   print_vertex(setup, v0);
      #endif
         if (unlikely(sp_debug & SP_DBG_NO_RAST) ||
      setup->softpipe->rasterizer->rasterizer_discard)
         if (dx == 0 && dy == 0)
            if (!setup_line_coefficients(setup, v0, v1))
            assert(v0[0][0] < 1.0e9);
   assert(v0[0][1] < 1.0e9);
   assert(v1[0][0] < 1.0e9);
            if (dx < 0) {
      dx = -dx;   /* make positive */
      }
   else {
                  if (dy < 0) {
      dy = -dy;   /* make positive */
      }
   else {
                  assert(dx >= 0);
   assert(dy >= 0);
            setup->quad[0].input.x0 = setup->quad[0].input.y0 = -1;
   setup->quad[0].inout.mask = 0x0;
   if (setup->softpipe->layer_slot > 0) {
      layer = *(unsigned *)setup->vprovoke[setup->softpipe->layer_slot];
      }
            if (setup->softpipe->viewport_index_slot > 0) {
      unsigned *udata = (unsigned*)setup->vprovoke[setup->softpipe->viewport_index_slot];
      }
            /* XXX temporary: set coverage to 1.0 so the line appears
   * if AA mode happens to be enabled.
   */
   setup->quad[0].input.coverage[0] =
   setup->quad[0].input.coverage[1] =
   setup->quad[0].input.coverage[2] =
            if (dx > dy) {
      /*** X-major line ***/
   int i;
   const int errorInc = dy + dy;
   int error = errorInc - dx;
            for (i = 0; i < dx; i++) {
               x0 += xstep;
   if (error < 0) {
         }
   else {
      error += errorDec;
            }
   else {
      /*** Y-major line ***/
   int i;
   const int errorInc = dx + dx;
   int error = errorInc - dy;
            for (i = 0; i < dy; i++) {
               y0 += ystep;
   if (error < 0) {
         }
   else {
      error += errorDec;
                     /* draw final quad */
   if (setup->quad[0].inout.mask) {
            }
         static void
   point_persp_coeff(const struct setup_context *setup,
                     {
      assert(i <= 3);
   coef->dadx[i] = 0.0F;
   coef->dady[i] = 0.0F;
      }
         /**
   * Do setup for point rasterization, then render the point.
   * Round or square points...
   * XXX could optimize a lot for 1-pixel points.
   */
   void
   sp_setup_point(struct setup_context *setup,
         {
      struct softpipe_context *softpipe = setup->softpipe;
   const struct tgsi_shader_info *fsInfo = &setup->softpipe->fs_variant->info;
   const int sizeAttr = setup->softpipe->psize_slot;
   const float size
      = sizeAttr > 0 ? v0[sizeAttr][0]
      const float halfSize = 0.5F * size;
   const bool round = (bool) setup->softpipe->rasterizer->point_smooth;
   const float x = v0[0][0];  /* Note: data[0] is always position */
   const float y = v0[0][1];
   const struct sp_setup_info *sinfo = &softpipe->setup_info;
   uint fragSlot;
   uint layer = 0;
      #if DEBUG_VERTS
      debug_printf("Setup point:\n");
      #endif
                  if (unlikely(sp_debug & SP_DBG_NO_RAST) ||
      setup->softpipe->rasterizer->rasterizer_discard)
                  if (setup->softpipe->layer_slot > 0) {
      layer = *(unsigned *)v0[setup->softpipe->layer_slot];
      }
            if (setup->softpipe->viewport_index_slot > 0) {
      unsigned *udata = (unsigned*)v0[setup->softpipe->viewport_index_slot];
      }
            /* For points, all interpolants are constant-valued.
   * However, for point sprites, we'll need to setup texcoords appropriately.
   * XXX: which coefficients are the texcoords???
   * We may do point sprites as textured quads...
   *
   * KW: We don't know which coefficients are texcoords - ultimately
   * the choice of what interpolation mode to use for each attribute
   * should be determined by the fragment program, using
   * per-attribute declaration statements that include interpolation
   * mode as a parameter.  So either the fragment program will have
   * to be adjusted for pointsprite vs normal point behaviour, or
   * otherwise a special interpolation mode will have to be defined
   * which matches the required behaviour for point sprites.  But -
   * the latter is not a feature of normal hardware, and as such
   * probably should be ruled out on that basis.
   */
            /* setup Z, W */
   const_coeff(setup, &setup->posCoef, 0, 2);
            for (fragSlot = 0; fragSlot < fsInfo->num_inputs; fragSlot++) {
      const uint vertSlot = sinfo->attrib[fragSlot].src_index;
            switch (sinfo->attrib[fragSlot].interp) {
   case SP_INTERP_CONSTANT:
         case SP_INTERP_LINEAR:
      for (j = 0; j < TGSI_NUM_CHANNELS; j++)
            case SP_INTERP_PERSPECTIVE:
      for (j = 0; j < TGSI_NUM_CHANNELS; j++)
      point_persp_coeff(setup, setup->vprovoke,
         case SP_INTERP_POS:
      setup_fragcoord_coeff(setup, fragSlot);
      default:
                  if (fsInfo->input_semantic_name[fragSlot] == TGSI_SEMANTIC_FACE) {
      /* convert 0 to 1.0 and 1 to -1.0 */
   setup->coef[fragSlot].a0[0] = setup->facing * -2.0f + 1.0f;
   setup->coef[fragSlot].dadx[0] = 0.0;
                     if (halfSize <= 0.5 && !round) {
      /* special case for 1-pixel points */
   const int ix = ((int) x) & 1;
   const int iy = ((int) y) & 1;
   setup->quad[0].input.x0 = (int) x - ix;
   setup->quad[0].input.y0 = (int) y - iy;
   setup->quad[0].inout.mask = (1 << ix) << (2 * iy);
      }
   else {
      if (round) {
      /* rounded points */
   const int ixmin = block((int) (x - halfSize));
   const int ixmax = block((int) (x + halfSize));
   const int iymin = block((int) (y - halfSize));
   const int iymax = block((int) (y + halfSize));
   const float rmin = halfSize - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
   const float rmax = halfSize + 0.7071F;
   const float rmin2 = MAX2(0.0F, rmin * rmin);
   const float rmax2 = rmax * rmax;
                  for (iy = iymin; iy <= iymax; iy += 2) {
                                 dx = (ix + 0.5f) - x;
   dy = (iy + 0.5f) - y;
   dist2 = dx * dx + dy * dy;
   if (dist2 <= rmax2) {
      cover = 1.0F - (dist2 - rmin2) * cscale;
                     dx = (ix + 1.5f) - x;
   dy = (iy + 0.5f) - y;
   dist2 = dx * dx + dy * dy;
   if (dist2 <= rmax2) {
      cover = 1.0F - (dist2 - rmin2) * cscale;
                     dx = (ix + 0.5f) - x;
   dy = (iy + 1.5f) - y;
   dist2 = dx * dx + dy * dy;
   if (dist2 <= rmax2) {
      cover = 1.0F - (dist2 - rmin2) * cscale;
                     dx = (ix + 1.5f) - x;
   dy = (iy + 1.5f) - y;
   dist2 = dx * dx + dy * dy;
   if (dist2 <= rmax2) {
      cover = 1.0F - (dist2 - rmin2) * cscale;
                     if (setup->quad[0].inout.mask) {
      setup->quad[0].input.x0 = ix;
   setup->quad[0].input.y0 = iy;
               }
   else {
      /* square points */
   const int xmin = (int) (x + 0.75 - halfSize);
   const int ymin = (int) (y + 0.25 - halfSize);
   const int xmax = xmin + (int) size;
   const int ymax = ymin + (int) size;
   /* XXX could apply scissor to xmin,ymin,xmax,ymax now */
   const int ixmin = block(xmin);
   const int ixmax = block(xmax - 1);
   const int iymin = block(ymin);
                  /*
   debug_printf("(%f, %f) -> X:%d..%d Y:%d..%d\n", x, y, xmin, xmax,ymin,ymax);
   */
   for (iy = iymin; iy <= iymax; iy += 2) {
      uint rowMask = 0xf;
   if (iy < ymin) {
      /* above the top edge */
      }
   if (iy + 1 >= ymax) {
                                          if (ix < xmin) {
      /* fragment is past left edge of point, turn off left bits */
      }
   if (ix + 1 >= xmax) {
                        setup->quad[0].inout.mask = mask;
   setup->quad[0].input.x0 = ix;
   setup->quad[0].input.y0 = iy;
                  }
         /**
   * Called by vbuf code just before we start buffering primitives.
   */
   void
   sp_setup_prepare(struct setup_context *setup)
   {
      struct softpipe_context *sp = setup->softpipe;
   int i;
   unsigned max_layer = ~0;
   if (sp->dirty) {
                  /* Note: nr_attrs is only used for debugging (vertex printing) */
            /*
   * Determine how many layers the fb has (used for clamping layer value).
   * OpenGL (but not d3d10) permits different amount of layers per rt, however
   * results are undefined if layer exceeds the amount of layers of ANY
   * attachment hence don't need separate per cbuf and zsbuf max.
   */
   for (i = 0; i < setup->softpipe->framebuffer.nr_cbufs; i++) {
      struct pipe_surface *cbuf = setup->softpipe->framebuffer.cbufs[i];
   if (cbuf) {
                              /* Prepare pixel offset for rasterisation:
   *  - pixel center (0.5, 0.5) for GL, or
   *  - assume (0.0, 0.0) for other APIs.
   */
   if (setup->softpipe->rasterizer->half_pixel_center) {
         } else {
                                    if (sp->reduced_api_prim == MESA_PRIM_TRIANGLES &&
      sp->rasterizer->fill_front == PIPE_POLYGON_MODE_FILL &&
   sp->rasterizer->fill_back == PIPE_POLYGON_MODE_FILL) {
   /* we'll do culling */
      }
   else {
      /* 'draw' will do culling */
         }
         void
   sp_setup_destroy_context(struct setup_context *setup)
   {
         }
         /**
   * Create a new primitive setup/render stage.
   */
   struct setup_context *
   sp_setup_create_context(struct softpipe_context *softpipe)
   {
      struct setup_context *setup = CALLOC_STRUCT(setup_context);
                     for (i = 0; i < MAX_QUADS; i++) {
      setup->quad[i].coef = setup->coef;
               setup->span.left[0] = 1000000;     /* greater than right[0] */
               }
