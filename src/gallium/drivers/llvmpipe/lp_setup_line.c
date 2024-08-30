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
   * Binning code for lines
   */
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "lp_perf.h"
   #include "lp_setup_context.h"
   #include "lp_rast.h"
   #include "lp_state_fs.h"
   #include "lp_state_setup.h"
   #include "lp_context.h"
   #include "draw/draw_context.h"
      #define NUM_CHANNELS 4
      struct lp_line_info {
         float dx;
   float dy;
   float oneoverarea;
            const float (*v1)[4];
            float (*a0)[4];
   float (*dadx)[4];
      };
         /**
   * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
   */
   static void
   constant_coef(struct lp_setup_context *setup,
               struct lp_line_info *info,
   unsigned slot,
   {
      info->a0[slot][i] = value;
   info->dadx[slot][i] = 0.0f;
      }
         /**
   * Compute a0, dadx and dady for a linearly interpolated coefficient,
   * for a triangle.
   */
   static void
   linear_coef(struct lp_setup_context *setup,
               struct lp_line_info *info,
   unsigned slot,
   {
      float a1 = info->v1[vert_attr][i];
   float a2 = info->v2[vert_attr][i];
   float da21 = a1 - a2;
   float dadx = da21 * info->dx * info->oneoverarea;
            info->dadx[slot][i] = dadx;
   info->dady[slot][i] = dady;
   info->a0[slot][i] = (a1 -
            }
         /**
   * Compute a0, dadx and dady for a perspective-corrected interpolant,
   * for a triangle.
   * We basically multiply the vertex value by 1/w before computing
   * the plane coefficients (a0, dadx, dady).
   * Later, when we compute the value at a particular fragment position we'll
   * divide the interpolated value by the interpolated W at that fragment.
   */
   static void
   perspective_coef(struct lp_setup_context *setup,
                  struct lp_line_info *info,
      {
      /* premultiply by 1/w  (v[0][3] is always 1/w):
   */
   float a1 = info->v1[vert_attr][i] * info->v1[0][3];
   float a2 = info->v2[vert_attr][i] * info->v2[0][3];
   float da21 = a1 - a2;
   float dadx = da21 * info->dx * info->oneoverarea;
            info->dadx[slot][i] = dadx;
   info->dady[slot][i] = dady;
   info->a0[slot][i] = (a1 -
            }
         static void
   setup_fragcoord_coef(struct lp_setup_context *setup,
                     {
      /*X*/
   if (usage_mask & TGSI_WRITEMASK_X) {
      info->a0[slot][0] = 0.0;
   info->dadx[slot][0] = 1.0;
               /*Y*/
   if (usage_mask & TGSI_WRITEMASK_Y) {
      info->a0[slot][1] = 0.0;
   info->dadx[slot][1] = 0.0;
               /*Z*/
   if (usage_mask & TGSI_WRITEMASK_Z) {
                  /*W*/
   if (usage_mask & TGSI_WRITEMASK_W) {
            }
         /**
   * Compute the tri->coef[] array dadx, dady, a0 values.
   */
   static void
   setup_line_coefficients(struct lp_setup_context *setup,
         {
      const struct lp_setup_variant_key *key = &setup->setup.variant->key;
            /* setup interpolation for all the remaining attributes:
   */
   for (unsigned slot = 0; slot < key->num_inputs; slot++) {
      unsigned vert_attr = key->inputs[slot].src_index;
   unsigned usage_mask = key->inputs[slot].usage_mask;
            switch (key->inputs[slot].interp) {
   case LP_INTERP_CONSTANT:
      if (key->flatshade_first) {
      for (i = 0; i < NUM_CHANNELS; i++)
      if (usage_mask & (1 << i))
         } else {
      for (i = 0; i < NUM_CHANNELS; i++)
      if (usage_mask & (1 << i))
                     case LP_INTERP_LINEAR:
      for (i = 0; i < NUM_CHANNELS; i++)
      if (usage_mask & (1 << i))
            case LP_INTERP_PERSPECTIVE:
      for (i = 0; i < NUM_CHANNELS; i++)
      if (usage_mask & (1 << i))
                  case LP_INTERP_POSITION:
      /*
   * The generated pixel interpolators will pick up the coeffs from
   * slot 0, so all need to ensure that the usage mask is covers all
   * usages.
   */
               case LP_INTERP_FACING:
      for (i = 0; i < NUM_CHANNELS; i++)
      if (usage_mask & (1 << i))
                  default:
                     /* The internal position input is in slot zero:
   */
      }
            static inline int
   subpixel_snap(float a)
   {
         }
         /**
   * Print line vertex attribs (for debug).
   */
   static void
   print_line(struct lp_setup_context *setup,
               {
               debug_printf("llvmpipe line\n");
   for (unsigned i = 0; i < 1 + key->num_inputs; i++) {
      debug_printf("  v1[%d]:  %f %f %f %f\n", i,
      }
   for (unsigned i = 0; i < 1 + key->num_inputs; i++) {
      debug_printf("  v2[%d]:  %f %f %f %f\n", i,
         }
         static inline bool
   sign(float x)
   {
         }
         /* Used on positive floats only:
   */
   static inline float
   fracf(float f)
   {
         }
         static bool
   try_setup_line(struct lp_setup_context *setup,
               {
      struct llvmpipe_context *lp_context = (struct llvmpipe_context *)setup->pipe;
   struct lp_scene *scene = setup->scene;
   const struct lp_setup_variant_key *key = &setup->setup.variant->key;
            if (lp_context->active_statistics_queries) {
                  if (0)
            if (lp_setup_zero_sample_mask(setup)) {
      if (0) debug_printf("zero sample mask\n");
   LP_COUNT(nr_culled_tris);
               const float (*pv)[4];
   if (setup->flatshade_first) {
         } else {
                  unsigned viewport_index = 0;
   if (setup->viewport_index_slot > 0) {
      unsigned *udata = (unsigned*)pv[setup->viewport_index_slot];
               unsigned layer = 0;
   if (setup->layer_slot > 0) {
      layer = *(unsigned*)pv[setup->layer_slot];
               float dx = v1[0][0] - v2[0][0];
   float dy = v1[0][1] - v2[0][1];
   const float area = dx * dx + dy * dy;
   if (area == 0) {
      LP_COUNT(nr_culled_tris);
               struct lp_line_info info;
   info.oneoverarea = 1.0f / area;
   info.dx = dx;
   info.dy = dy;
   info.v1 = v1;
                     int x[4], y[4];
   if (setup->rectangular_lines) {
      float scale = (setup->line_width * 0.5f) / sqrtf(area);
   int tx = subpixel_snap(-dy * scale);
            x[0] = subpixel_snap(v1[0][0] - pixel_offset) - tx;
   x[1] = subpixel_snap(v2[0][0] - pixel_offset) - tx;
   x[2] = subpixel_snap(v2[0][0] - pixel_offset) + tx;
            y[0] = subpixel_snap(v1[0][1] - pixel_offset) - ty;
   y[1] = subpixel_snap(v2[0][1] - pixel_offset) - ty;
   y[2] = subpixel_snap(v2[0][1] - pixel_offset) + ty;
      } else {
      float x_offset = 0, y_offset=0;
            /* FIXME: not taking into account setup->pixel_offset here is wrong. */
   float x1diff = v1[0][0] - floorf(v1[0][0]) - 0.5f;
   float y1diff = v1[0][1] - floorf(v1[0][1]) - 0.5f;
   float x2diff = v2[0][0] - floorf(v2[0][0]) - 0.5f;
            /* linewidth should be interpreted as integer */
            bool draw_start;
            if (fabsf(dx) >= fabsf(dy)) {
                        if (y2diff == -0.5f && dy < 0.0f) {
                  /*
   * Diamond exit rule test for starting point
   */
   if (fabsf(x1diff) + fabsf(y1diff) < 0.5f) {
         } else if (sign(x1diff) == sign(-dx)) {
         } else if (sign(-y1diff) != sign(dy)) {
         } else {
      /* do intersection test */
   float yintersect = fracf(v1[0][1]) + x1diff * dydx;
               /*
   * Diamond exit rule test for ending point
   */
   if (fabsf(x2diff) + fabsf(y2diff) < 0.5f) {
         } else if (sign(x2diff) != sign(-dx)) {
         } else if (sign(-y2diff) == sign(dy)) {
         } else {
      /* do intersection test */
   float yintersect = fracf(v2[0][1]) + x2diff * dydx;
               /* Are we already drawing start/end? */
                  /* interpolate using the preferred wide-lines formula */
                  if (dx < 0.0f) {
      /* if v2 is to the right of v1, swap pointers */
   const float (*temp)[4] = v1;
                  /* Otherwise shift planes appropriately */
   /* left edge */
   will_draw_start = x1diff <= 0.f;
   if (will_draw_start != draw_start) {
                  }
   /* right edge */
   will_draw_end = x2diff > 0.f;
   if (will_draw_end != draw_end) {
      x_offset = -x2diff - 0.5f;
         } else {
      /* Otherwise shift planes appropriately */
   /* right edge */
   will_draw_start = x1diff > 0.f;
   if (will_draw_start != draw_start) {
      x_offset = -x1diff + 0.5f;
      }
   /* left edge */
   will_draw_end = x2diff <= 0.f;
   if (will_draw_end != draw_end) {
      x_offset_end = -x2diff + 0.5f;
                  /* x/y positions in fixed point */
   x[0] = subpixel_snap(v1[0][0] + x_offset     - pixel_offset);
   x[1] = subpixel_snap(v2[0][0] + x_offset_end - pixel_offset);
                  y[0] = subpixel_snap(v1[0][1] + y_offset     - pixel_offset) - fixed_width/2;
   y[1] = subpixel_snap(v2[0][1] + y_offset_end - pixel_offset) - fixed_width/2;
   y[2] = subpixel_snap(v2[0][1] + y_offset_end - pixel_offset) + fixed_width/2;
      } else {
                        if (x2diff == -0.5f && dx < 0.0f) {
                  /*
   * Diamond exit rule test for starting point
   */
   if (fabsf(x1diff) + fabsf(y1diff) < 0.5f) {
         } else if (sign(-y1diff) == sign(dy)) {
         } else if (sign(x1diff) != sign(-dx)) {
         } else {
      /* do intersection test */
   float xintersect = fracf(v1[0][0]) + y1diff * dxdy;
               /*
   * Diamond exit rule test for ending point
   */
   if (fabsf(x2diff) + fabsf(y2diff) < 0.5f) {
         } else if (sign(-y2diff) != sign(dy)) {
         } else if (sign(x2diff) == sign(-dx)) {
         } else {
      /* do intersection test */
   float xintersect = fracf(v2[0][0]) + y2diff * dxdy;
               /*
   * Are we already drawing start/end?
   * FIXME: this needs to be done with fixed point arithmetic (otherwise
   * the comparisons against zero are not mirroring what actually happens
   * when rasterizing using the plane equations).
   */
                     /* interpolate using the preferred wide-lines formula */
                  if (dy > 0.0f) {
      /* if v2 is on top of v1, swap pointers */
   const float (*temp)[4] = v1;
                  if (setup->bottom_edge_rule) {
      will_draw_start = y1diff >= 0.f;
      } else {
                        /* Otherwise shift planes appropriately */
   /* bottom edge */
   if (will_draw_start != draw_start) {
      y_offset_end = -y1diff + 0.5f;
      }
   /* top edge */
   if (will_draw_end != draw_end) {
      y_offset = -y2diff + 0.5f;
         } else {
      if (setup->bottom_edge_rule) {
      will_draw_start = y1diff < 0.f;
      } else {
                        /* Otherwise shift planes appropriately */
   /* top edge */
   if (will_draw_start != draw_start) {
      y_offset = -y1diff - 0.5f;
      }
   /* bottom edge */
   if (will_draw_end != draw_end) {
      y_offset_end = -y2diff - 0.5f;
                  /* x/y positions in fixed point */
   x[0] = subpixel_snap(v1[0][0] + x_offset     - pixel_offset) - fixed_width/2;
   x[1] = subpixel_snap(v2[0][0] + x_offset_end - pixel_offset) - fixed_width/2;
                  y[0] = subpixel_snap(v1[0][1] + y_offset     - pixel_offset);
   y[1] = subpixel_snap(v2[0][1] + y_offset_end - pixel_offset);
   y[2] = subpixel_snap(v2[0][1] + y_offset_end - pixel_offset);
                  /* Bounding rectangle (in pixels) */
   struct u_rect bbox, bboxpos;
   {
      /* Yes this is necessary to accurately calculate bounding boxes
   * with the two fill-conventions we support.  GL (normally) ends
   * up needing a bottom-left fill convention, which requires
   * slightly different rounding.
   */
            bbox.x0 = (MIN4(x[0], x[1], x[2], x[3]) + (FIXED_ONE-1)) >> FIXED_ORDER;
   bbox.x1 = (MAX4(x[0], x[1], x[2], x[3]) + (FIXED_ONE-1)) >> FIXED_ORDER;
   bbox.y0 = (MIN4(y[0], y[1], y[2], y[3]) + (FIXED_ONE-1) + adj) >> FIXED_ORDER;
            /* Inclusive coordinates:
   */
   bbox.x1--;
               if (!u_rect_test_intersection(&setup->draw_regions[viewport_index], &bbox)) {
      if (0) debug_printf("no intersection\n");
   LP_COUNT(nr_culled_tris);
               int max_szorig = ((bbox.x1 - (bbox.x0 & ~3)) |
         bool use_32bits = max_szorig <= MAX_FIXED_LENGTH32;
            /* Can safely discard negative regions:
   */
   bboxpos.x0 = MAX2(bboxpos.x0, 0);
            int nr_planes = 4;
   /*
   * Determine how many scissor planes we need, that is drop scissor
   * edges if the bounding box of the tri is fully inside that edge.
   */
            bool s_planes[4];
   scissor_planes_needed(s_planes, &bboxpos, scissor);
            struct lp_rast_triangle *line = lp_setup_alloc_triangle(scene,
               if (!line)
         #ifdef DEBUG
      line->v[0][0] = v1[0][0];
   line->v[1][0] = v2[0][0];
   line->v[0][1] = v1[0][1];
      #endif
                  /* calculate the deltas */
   struct lp_rast_plane *plane = GET_PLANES(line);
   plane[0].dcdy = x[0] - x[1];
   plane[1].dcdy = x[1] - x[2];
   plane[2].dcdy = x[2] - x[3];
            plane[0].dcdx = y[0] - y[1];
   plane[1].dcdx = y[1] - y[2];
   plane[2].dcdx = y[2] - y[3];
            if (draw_will_inject_frontface(lp_context->draw) &&
      setup->face_slot > 0) {
      } else {
                  /* Setup parameter interpolants:
   */
   info.a0 = GET_A0(&line->inputs);
   info.dadx = GET_DADX(&line->inputs);
   info.dady = GET_DADY(&line->inputs);
   info.frontfacing = line->inputs.frontfacing;
            line->inputs.disable = false;
   line->inputs.layer = layer;
   line->inputs.viewport_index = viewport_index;
            /*
   * XXX: this code is mostly identical to the one in lp_setup_tri, except it
   * uses 4 planes instead of 3. Could share the code (including the sse
   * assembly, in fact we'd get the 4th plane for free).  The only difference
   * apart from storing the 4th plane would be some different shuffle for
   * calculating dcdx/dcdy.
   */
   for (unsigned i = 0; i < 4; i++) {
      /* half-edge constants, will be iterated over the whole render
   * target.
   */
            /* correct for top-left vs. bottom-left fill convention.
   */
   if (plane[i].dcdx < 0) {
      /* both fill conventions want this - adjust for left edges */
      } else if (plane[i].dcdx == 0) {
      if (setup->bottom_edge_rule == 0) {
      /* correct for top-left fill convention:
   */
      } else {
      /* correct for bottom-left fill convention:
   */
                  plane[i].dcdx *= FIXED_ONE;
            /* find trivial reject offsets for each edge for a single-pixel
   * sized block.  These will be scaled up at each recursive level to
   * match the active blocksize.  Scaling in this way works best if
   * the blocks are square.
   */
   plane[i].eo = 0;
   if (plane[i].dcdx < 0) plane[i].eo -= plane[i].dcdx;
               if (nr_planes > 4) {
      lp_setup_add_scissor_planes(scissor, &plane[4], s_planes,
               return lp_setup_bin_triangle(setup, line, use_32bits, false,
      }
         static void
   lp_setup_line_discard(struct lp_setup_context *setup,
               {
   }
         static void
   lp_setup_line(struct lp_setup_context *setup,
               {
      if (!try_setup_line(setup, v0, v1)) {
      if (!lp_setup_flush_and_restart(setup))
            if (!try_setup_line(setup, v0, v1))
         }
         void
   lp_setup_choose_line(struct lp_setup_context *setup)
   {
      if (setup->rasterizer_discard) {
         } else {
            }
