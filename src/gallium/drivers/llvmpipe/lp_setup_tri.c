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
   * Binning code for triangles
   */
      #include "util/detect.h"
      #if DETECT_ARCH_SSE
   #include <emmintrin.h>
   #elif defined(_ARCH_PWR8) && UTIL_ARCH_LITTLE_ENDIAN
   #include <altivec.h>
   /*
   altivec.h inclusion in -std=c++98..11 causes bool to be redefined
   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58241
   */
   #undef bool
   #endif
      #include <stdbool.h>
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_rect.h"
   #include "util/u_sse.h"
   #include "lp_perf.h"
   #include "lp_setup_context.h"
   #include "lp_rast.h"
   #include "lp_state_fs.h"
   #include "lp_state_setup.h"
   #include "lp_context.h"
      #include <inttypes.h>
      #if defined(_ARCH_PWR8) && UTIL_ARCH_LITTLE_ENDIAN
   #include "util/u_pwr8.h"
   #endif
      #if !DETECT_ARCH_SSE
      static inline int
   subpixel_snap(float a)
   {
         }
      #endif
      /* Position and area in fixed point coordinates */
   struct fixed_position {
      int32_t x[4];
   int32_t y[4];
   int32_t dx01;
   int32_t dy01;
   int32_t dx20;
      };
         /**
   * Alloc space for a new triangle plus the input.a0/dadx/dady arrays
   * immediately after it.
   * The memory is allocated from the per-scene pool, not per-tile.
   * \param num_inputs  number of fragment shader inputs
   * \return pointer to triangle space
   */
   struct lp_rast_triangle *
   lp_setup_alloc_triangle(struct lp_scene *scene,
               {
      // add 1 for XYZW position
   unsigned input_array_sz = (nr_inputs + 1) * sizeof(float[4]);
                     const unsigned tri_size  = sizeof(struct lp_rast_triangle)
      + 3 * input_array_sz +   // 3 = da + dadx + dady
         struct lp_rast_triangle *tri = lp_scene_alloc_aligned(scene, tri_size, 16);
   if (!tri)
                     {
      ASSERTED char *a = (char *)tri;
                           }
         void
   lp_setup_print_vertex(struct lp_setup_context *setup,
               {
               debug_printf("   wpos (%s[0]) xyzw %f %f %f %f\n",
                  for (int i = 0; i < key->num_inputs; i++) {
               debug_printf("  in[%d] (%s[%d]) %s%s%s%s ",
               i,
   name, key->inputs[i].src_index,
   (key->inputs[i].usage_mask & 0x1) ? "x" : " ",
            for (int j = 0; j < 4; j++)
                        }
         /**
   * Print triangle vertex attribs (for debug).
   */
   void
   lp_setup_print_triangle(struct lp_setup_context *setup,
                     {
               {
      const float ex = v0[0][0] - v2[0][0];
   const float ey = v0[0][1] - v2[0][1];
   const float fx = v1[0][0] - v2[0][0];
            /* det = cross(e,f).z */
   const float det = ex * fy - ey * fx;
   if (det < 0.0f)
         else if (det > 0.0f)
         else
               lp_setup_print_vertex(setup, "v0", v0);
   lp_setup_print_vertex(setup, "v1", v1);
      }
         #define MAX_PLANES 8
   static unsigned
   lp_rast_tri_tab[MAX_PLANES+1] = {
      0,               /* should be impossible */
   LP_RAST_OP_TRIANGLE_1,
   LP_RAST_OP_TRIANGLE_2,
   LP_RAST_OP_TRIANGLE_3,
   LP_RAST_OP_TRIANGLE_4,
   LP_RAST_OP_TRIANGLE_5,
   LP_RAST_OP_TRIANGLE_6,
   LP_RAST_OP_TRIANGLE_7,
      };
      static unsigned
   lp_rast_32_tri_tab[MAX_PLANES+1] = {
      0,               /* should be impossible */
   LP_RAST_OP_TRIANGLE_32_1,
   LP_RAST_OP_TRIANGLE_32_2,
   LP_RAST_OP_TRIANGLE_32_3,
   LP_RAST_OP_TRIANGLE_32_4,
   LP_RAST_OP_TRIANGLE_32_5,
   LP_RAST_OP_TRIANGLE_32_6,
   LP_RAST_OP_TRIANGLE_32_7,
      };
         static unsigned
   lp_rast_ms_tri_tab[MAX_PLANES+1] = {
      0,               /* should be impossible */
   LP_RAST_OP_MS_TRIANGLE_1,
   LP_RAST_OP_MS_TRIANGLE_2,
   LP_RAST_OP_MS_TRIANGLE_3,
   LP_RAST_OP_MS_TRIANGLE_4,
   LP_RAST_OP_MS_TRIANGLE_5,
   LP_RAST_OP_MS_TRIANGLE_6,
   LP_RAST_OP_MS_TRIANGLE_7,
      };
         /*
   * Detect big primitives drawn with an alpha == 1.0.
   *
   * This is used when simulating anti-aliasing primitives in shaders, e.g.,
   * when drawing the windows client area in Aero's flip-3d effect.
   */
   static bool
   check_opaque(const struct lp_setup_context *setup,
               const float (*v1)[4],
   {
      const struct lp_fragment_shader_variant *variant =
            if (variant->opaque)
            if (!variant->potentially_opaque)
            const struct lp_tgsi_channel_info *alpha_info = &variant->shader->info.cbuf[0][3];
   if (alpha_info->file == TGSI_FILE_CONSTANT) {
      const float *constants = setup->fs.current.jit_resources.constants[0].f;
   float alpha = constants[alpha_info->u.index*4 +
                     if (alpha_info->file == TGSI_FILE_INPUT) {
      return (v1[1 + alpha_info->u.index][alpha_info->swizzle] == 1.0f &&
                        }
         /**
   * Do basic setup for triangle rasterization and determine which
   * framebuffer tiles are touched.  Put the triangle in the scene's
   * bins for the tiles which we overlap.
   */
   static bool
   do_triangle_ccw(struct lp_setup_context *setup,
                  struct fixed_position *position,
   const float (*v0)[4],
      {
               const float (*pv)[4];
   if (setup->flatshade_first) {
         } else {
                  unsigned viewport_index = 0;
   if (setup->viewport_index_slot > 0) {
      unsigned *udata = (unsigned*)pv[setup->viewport_index_slot];
               unsigned layer = 0;
   if (setup->layer_slot > 0) {
      layer = *(unsigned*)pv[setup->layer_slot];
               /* Bounding rectangle (in pixels) */
   struct u_rect bbox;
   {
      /* Yes this is necessary to accurately calculate bounding boxes
   * with the two fill-conventions we support.  GL (normally) ends
   * up needing a bottom-left fill convention, which requires
   * slightly different rounding.
   */
            /* Inclusive x0, exclusive x1 */
   bbox.x0 =  MIN3(position->x[0], position->x[1],
         bbox.x1 = (MAX3(position->x[0], position->x[1],
            /* Inclusive / exclusive depending upon adj (bottom-left or top-right) */
   bbox.y0 = (MIN3(position->y[0], position->y[1],
         bbox.y1 = (MAX3(position->y[0], position->y[1],
               if (!u_rect_test_intersection(&setup->draw_regions[viewport_index], &bbox)) {
      if (0) debug_printf("no intersection\n");
   LP_COUNT(nr_culled_tris);
               int max_szorig = ((bbox.x1 - (bbox.x0 & ~3)) |
            #if defined(_ARCH_PWR8) && UTIL_ARCH_LITTLE_ENDIAN
      bool pwr8_limit_check = (bbox.x1 - bbox.x0) <= MAX_FIXED_LENGTH32 &&
      #endif
         /* Can safely discard negative regions, but need to keep hold of
   * information about when the triangle extends past screen
   * boundaries.  See trimmed_box in lp_setup_bin_triangle().
   */
   bbox.x0 = MAX2(bbox.x0, 0);
                     /*
   * Determine how many scissor planes we need, that is drop scissor
   * edges if the bounding box of the tri is fully inside that edge.
   */
   const struct u_rect *scissor = &setup->draw_regions[viewport_index];
   bool s_planes[4];
   scissor_planes_needed(s_planes, &bbox, scissor);
            const struct lp_setup_variant_key *key = &setup->setup.variant->key;
   struct lp_rast_triangle *tri =
         if (!tri)
         #ifdef DEBUG
      tri->v[0][0] = v0[0][0];
   tri->v[1][0] = v1[0][0];
   tri->v[2][0] = v2[0][0];
   tri->v[0][1] = v0[0][1];
   tri->v[1][1] = v1[0][1];
      #endif
                  /*
   * Rotate the tri such that v0 is closest to the fb origin.
   * This can give more accurate a0 value (which is at fb origin)
   * when calculating the interpolants.
   * It can't work when there's flat shading for instance in one
   * of the attributes, hence restrict this to just a single attribute
   * which is what causes some test failures.
   * (This does not address the problem that interpolation may be
   * inaccurate if gradients are relatively steep in small tris far
   * away from the origin. It does however fix the (silly) wgf11rasterizer
   * Interpolator test.)
   * XXX This causes problems with mipgen -EmuTexture for not yet really
   * understood reasons (if the vertices would be submitted in a different
   * order, we'd also generate the same "wrong" results here without
   * rotation). In any case, that we generate different values if a prim
   * has the vertices rotated but is otherwise the same (which is due to
   * numerical issues) is not a nice property. An additional problem by
   * swapping the vertices here (which is possibly worse) is that
   * the same primitive coming in twice might generate different values
   * (in particular for z) due to the swapping potentially not happening
   * both times, if the attributes to be interpolated are different. For now,
   * just restrict this to not get used with dx9 (by checking pixel offset),
   * could also restrict it further to only trigger with wgf11Interpolator
   * Rasterizer test (the only place which needs it, with always the same
   * vertices even).
   */
   if ((LP_DEBUG & DEBUG_ACCURATE_A0) &&
      setup->pixel_offset == 0.5f &&
   key->num_inputs == 1 &&
   (key->inputs[0].interp == LP_INTERP_LINEAR ||
   key->inputs[0].interp == LP_INTERP_PERSPECTIVE) &&
   setup->fs.current_tex_num == 0 &&
   setup->cullmode == 0) {
   float dist0 = v0[0][0] * v0[0][0] + v0[0][1] * v0[0][1];
   float dist1 = v1[0][0] * v1[0][0] + v1[0][1] * v1[0][1];
   float dist2 = v2[0][0] * v2[0][0] + v2[0][1] * v2[0][1];
   if (dist0 > dist1 && dist1 < dist2) {
      const float (*vt)[4];
   vt = v0;
   v0 = v1;
   v1 = v2;
   v2 = vt;
   // rotate positions
   int x = position->x[0];
   int y = position->y[0];
   position->x[0] = position->x[1];
   position->y[0] = position->y[1];
   position->x[1] = position->x[2];
   position->y[1] = position->y[2];
                  position->dx20 = position->dx01;
   position->dy20 = position->dy01;
   position->dx01 = position->x[0] - position->x[1];
      } else if (dist0 > dist2) {
      const float (*vt)[4];
   vt = v0;
   v0 = v2;
   v2 = v1;
   v1 = vt;
   // rotate positions
   int x = position->x[0];
   int y = position->y[0];
   position->x[0] = position->x[2];
   position->y[0] = position->y[2];
   position->x[2] = position->x[1];
   position->y[2] = position->y[1];
                  position->dx01 = position->dx20;
   position->dy01 = position->dy20;
   position->dx20 = position->x[2] - position->x[0];
                  /* Setup parameter interpolants:
   */
   setup->setup.variant->jit_function(v0, v1, v2,
                                    tri->inputs.frontfacing = frontfacing;
   tri->inputs.disable = false;
   tri->inputs.is_blit = false;
   tri->inputs.layer = layer;
   tri->inputs.viewport_index = viewport_index;
            if (0)
      lp_dump_setup_coef(&setup->setup.variant->key,
                           #if DETECT_ARCH_SSE
      if (1) {
      __m128i vertx, verty;
   __m128i shufx, shufy;
   __m128i dcdx, dcdy;
   __m128i cdx02, cdx13, cdy02, cdy13, c02, c13;
   __m128i c01, c23, unused;
   __m128i dcdx_neg_mask;
   __m128i dcdy_neg_mask;
   __m128i dcdx_zero_mask;
   __m128i top_left_flag, c_dec;
   __m128i eo, p0, p1, p2;
            vertx = _mm_load_si128((__m128i *)position->x); /* vertex x coords */
            shufx = _mm_shuffle_epi32(vertx, _MM_SHUFFLE(3,0,2,1));
            dcdx = _mm_sub_epi32(verty, shufy);
            dcdx_neg_mask = _mm_srai_epi32(dcdx, 31);
   dcdx_zero_mask = _mm_cmpeq_epi32(dcdx, zero);
                     c_dec = _mm_or_si128(dcdx_neg_mask,
                        /*
   * 64 bit arithmetic.
   * Note we need _signed_ mul (_mm_mul_epi32) which we emulate.
   */
   cdx02 = mm_mullohi_epi32(dcdx, vertx, &cdx13);
   cdy02 = mm_mullohi_epi32(dcdy, verty, &cdy13);
   c02 = _mm_sub_epi64(cdx02, cdy02);
   c13 = _mm_sub_epi64(cdx13, cdy13);
   c02 = _mm_sub_epi64(c02, _mm_shuffle_epi32(c_dec,
         c13 = _mm_sub_epi64(c13, _mm_shuffle_epi32(c_dec,
            /*
   * Useful for very small fbs/tris (or fewer subpixel bits) only:
   * c = _mm_sub_epi32(mm_mullo_epi32(dcdx, vertx),
   *                   mm_mullo_epi32(dcdy, verty));
   *
   * c = _mm_sub_epi32(c, c_dec);
            /* Scale up to match c:
   */
   dcdx = _mm_slli_epi32(dcdx, FIXED_ORDER);
            /*
   * Calculate trivial reject values:
   * Note eo cannot overflow even if dcdx/dcdy would already have
   * 31 bits (which they shouldn't have). This is because eo
   * is never negative (albeit if we rely on that need to be careful...)
   */
   eo = _mm_sub_epi32(_mm_andnot_si128(dcdy_neg_mask, dcdy),
                     /*
   * Pointless transpose which gets undone immediately in
   * rasterization.
   * It is actually difficult to do away with it - would essentially
   * need GET_PLANES_DX, GET_PLANES_DY etc., but the calculations
   * for this then would need to depend on the number of planes.
   * The transpose is quite special here due to c being 64bit...
   * The store has to be unaligned (unless we'd make the plane size
   * a multiple of 128), and of course storing eo separately...
   */
   c01 = _mm_unpacklo_epi64(c02, c13);
   c23 = _mm_unpackhi_epi64(c02, c13);
   transpose2_64_2_32(&c01, &c23, &dcdx, &dcdy,
         _mm_storeu_si128((__m128i *)&plane[0], p0);
   plane[0].eo = (uint32_t)_mm_cvtsi128_si32(eo);
   _mm_storeu_si128((__m128i *)&plane[1], p1);
   eo = _mm_shuffle_epi32(eo, _MM_SHUFFLE(3,2,0,1));
   plane[1].eo = (uint32_t)_mm_cvtsi128_si32(eo);
   _mm_storeu_si128((__m128i *)&plane[2], p2);
   eo = _mm_shuffle_epi32(eo, _MM_SHUFFLE(0,0,0,2));
         #elif defined(_ARCH_PWR8) && UTIL_ARCH_LITTLE_ENDIAN
      /*
   * XXX this code is effectively disabled for all practical purposes,
   * as the allowed fb size is tiny if FIXED_ORDER is 8.
   */
   if (setup->fb.width <= MAX_FIXED_LENGTH32 &&
      setup->fb.height <= MAX_FIXED_LENGTH32 &&
   pwr8_limit_check) {
   unsigned int bottom_edge;
   __m128i vertx, verty;
   __m128i shufx, shufy;
   __m128i dcdx, dcdy, c;
   __m128i unused;
   __m128i dcdx_neg_mask;
   __m128i dcdy_neg_mask;
   __m128i dcdx_zero_mask;
   __m128i top_left_flag;
   __m128i c_inc_mask, c_inc;
   __m128i eo, p0, p1, p2;
   __m128i_union vshuf_mask;
   __m128i zero = vec_splats((unsigned char) 0);
      #if UTIL_ARCH_LITTLE_ENDIAN
         vshuf_mask.i[0] = 0x07060504;
   vshuf_mask.i[1] = 0x0B0A0908;
   vshuf_mask.i[2] = 0x03020100;
   #else
         vshuf_mask.i[0] = 0x00010203;
   vshuf_mask.i[1] = 0x0C0D0E0F;
   vshuf_mask.i[2] = 0x04050607;
   #endif
            /* vertex x coords */
   vertx = vec_load_si128((const uint32_t *) position->x);
   /* vertex y coords */
            shufx = vec_perm (vertx, vertx, vshuf_mask.m128i);
            dcdx = vec_sub_epi32(verty, shufy);
            dcdx_neg_mask = vec_srai_epi32(dcdx, 31);
   dcdx_zero_mask = vec_cmpeq_epi32(dcdx, zero);
            bottom_edge = (setup->bottom_edge_rule == 0) ? ~0 : 0;
            c_inc_mask = vec_or(dcdx_neg_mask,
                                 c = vec_sub_epi32(vec_mullo_epi32(dcdx, vertx),
                     /* Scale up to match c:
   */
   dcdx = vec_slli_epi32(dcdx, FIXED_ORDER);
            /* Calculate trivial reject values:
   */
   eo = vec_sub_epi32(vec_andnot_si128(dcdy_neg_mask, dcdy),
                     /* Pointless transpose which gets undone immediately in
   * rasterization:
   */
   transpose4_epi32(&c, &dcdx, &dcdy, &eo,
      #define STORE_PLANE(plane, vec) do {                  \
            vec_store_si128((uint32_t *)&temp_vec, vec); \
   plane.c    = (int64_t)temp_vec[0];           \
   plane.dcdx = temp_vec[1];                    \
   plane.dcdy = temp_vec[2];                    \
               STORE_PLANE(plane[0], p0);
   STORE_PLANE(plane[1], p1);
   #undef STORE_PLANE
         #endif
      {
      plane[0].dcdy = position->dx01;
   plane[1].dcdy = position->x[1] - position->x[2];
   plane[2].dcdy = position->dx20;
   plane[0].dcdx = position->dy01;
   plane[1].dcdx = position->y[1] - position->y[2];
            for (int i = 0; i < 3; i++) {
      /* half-edge constants, will be iterated over the whole render
   * target.
   */
                  /* correct for top-left vs. bottom-left fill convention.
   */
   if (plane[i].dcdx < 0) {
      /* both fill conventions want this - adjust for left edges */
      }
   else if (plane[i].dcdx == 0) {
      if (setup->bottom_edge_rule == 0) {
      /* correct for top-left fill convention:
   */
   if (plane[i].dcdy > 0)
      } else {
      /* correct for bottom-left fill convention:
   */
   if (plane[i].dcdy < 0)
                  /* Scale up to match c:
   */
   assert((plane[i].dcdx << FIXED_ORDER) >> FIXED_ORDER == plane[i].dcdx);
   assert((plane[i].dcdy << FIXED_ORDER) >> FIXED_ORDER == plane[i].dcdy);
                  /* find trivial reject offsets for each edge for a single-pixel
   * sized block.  These will be scaled up at each recursive level to
   * match the active blocksize.  Scaling in this way works best if
   * the blocks are square.
   */
   plane[i].eo = 0;
   if (plane[i].dcdx < 0) plane[i].eo -= plane[i].dcdx;
                  if (0) {
      debug_printf("p0: %"PRIx64"/%08x/%08x/%08x\n",
               plane[0].c,
            debug_printf("p1: %"PRIx64"/%08x/%08x/%08x\n",
               plane[1].c,
            debug_printf("p2: %"PRIx64"/%08x/%08x/%08x\n",
               plane[2].c,
               if (nr_planes > 3) {
      lp_setup_add_scissor_planes(scissor, &plane[3],
               return lp_setup_bin_triangle(setup, tri, use_32bits,
            }
      /*
   * Round to nearest less or equal power of two of the input.
   *
   * Undefined if no bit set exists, so code should check against 0 first.
   */
   static inline uint32_t
   floor_pot(uint32_t n)
   {
   #if DETECT_CC_GCC && (DETECT_ARCH_X86 || DETECT_ARCH_X86_64)
      if (n == 0)
            __asm__("bsr %1,%0"
         : "=r" (n)
   : "rm" (n)
      #else
      n |= (n >>  1);
   n |= (n >>  2);
   n |= (n >>  4);
   n |= (n >>  8);
   n |= (n >> 16);
      #endif
   }
         bool
   lp_setup_bin_triangle(struct lp_setup_context *setup,
                        struct lp_rast_triangle *tri,
   bool use_32bits,
      {
      struct lp_scene *scene = setup->scene;
            /* What is the largest power-of-two boundary this triangle crosses:
   */
   const int dx = floor_pot((bbox->x0 ^ bbox->x1) |
            /* The largest dimension of the rasterized area of the triangle
   * (aligned to a 4x4 grid), rounded down to the nearest power of two:
   */
   const int max_sz = ((bbox->x1 - (bbox->x0 & ~3)) |
                  /*
   * NOTE: It is important to use the original bounding box
   * which might contain negative values here, because if the
   * plane math may overflow or not with the 32bit rasterization
   * functions depends on the original extent of the triangle.
            /* Now apply scissor, etc to the bounding box.  Could do this
   * earlier, but it confuses the logic for tri-16 and would force
   * the rasterizer to also respect scissor, etc, just for the rare
   * cases where a small triangle extends beyond the scissor.
   */
   struct u_rect trimmed_box = *bbox;
   u_rect_find_intersection(&setup->draw_regions[viewport_index],
            /* Determine which tile(s) intersect the triangle's bounding box
   */
   if (dx < TILE_SIZE) {
      const int ix0 = bbox->x0 / TILE_SIZE;
   const int iy0 = bbox->y0 / TILE_SIZE;
   unsigned px = bbox->x0 & 63 & ~3;
            assert(iy0 == bbox->y1 / TILE_SIZE &&
            if (nr_planes == 3) {
      if (sz < 4) {
      /* Triangle is contained in a single 4x4 stamp:
   */
   assert(px + 4 <= TILE_SIZE);
   assert(py + 4 <= TILE_SIZE);
   if (setup->multisample)
         else
         return lp_scene_bin_cmd_with_state(scene, ix0, iy0,
                     if (sz < 16) {
                     /*
   * The 16x16 block is only 4x4 aligned, and can exceed the tile
   * dimensions if the triangle is 16 pixels in one dimension but 4
   * in the other. So budge the 16x16 back inside the tile.
   */
                                 if (setup->multisample)
         else
         return lp_scene_bin_cmd_with_state(scene, ix0, iy0,
               } else if (nr_planes == 4 && sz < 16) {
                                    if (setup->multisample)
         else
         return lp_scene_bin_cmd_with_state(scene, ix0, iy0,
                     /* Triangle is contained in a single tile:
   */
   if (setup->multisample)
         else
         return lp_scene_bin_cmd_with_state(scene, ix0, iy0, setup->fs.stored,
                  } else {
      struct lp_rast_plane *plane = GET_PLANES(tri);
   int64_t c[MAX_PLANES];
            int64_t eo[MAX_PLANES];
   int64_t xstep[MAX_PLANES];
            const int ix0 = trimmed_box.x0 / TILE_SIZE;
   const int iy0 = trimmed_box.y0 / TILE_SIZE;
   const int ix1 = trimmed_box.x1 / TILE_SIZE;
            for (int i = 0; i < nr_planes; i++) {
      c[i] = (plane[i].c +
                  ei[i] = (plane[i].dcdy -
                  eo[i] = (int64_t)plane[i].eo << TILE_ORDER;
   xstep[i] = -(((int64_t)plane[i].dcdx) << TILE_ORDER);
                        /* Test tile-sized blocks against the triangle.
   * Discard blocks fully outside the tri.  If the block is fully
   * contained inside the tri, bin an lp_rast_shade_tile command.
   * Else, bin a lp_rast_triangle command.
   */
   for (int y = iy0; y <= iy1; y++) {
                                                      for (int i = 0; i < nr_planes; i++) {
      int64_t planeout = cx[i] + eo[i];
   int64_t planepartial = cx[i] + ei[i] - 1;
                     if (out) {
      /* do nothing */
   if (in)
            } else if (partial) {
      /* Not trivially accepted by at least one plane -
   * rasterize/shade partial tile
                        if (setup->multisample)
         else
         if (!lp_scene_bin_cmd_with_state(scene, x, y,
                           } else {
      /* triangle covers the whole tile- shade whole tile */
   LP_COUNT(nr_fully_covered_64);
   in = true;
                     /* Iterate cx values across the region: */
   for (int i = 0; i < nr_planes; i++)
               /* Iterate c values down the region: */
   for (int i = 0; i < nr_planes; i++)
                        fail:
      /* Need to disable any partially binned triangle.  This is easier
   * than trying to locate all the triangle, shade-tile, etc,
   * commands which may have been binned.
   */
   tri->inputs.disable = true;
      }
         /**
   * Try to draw the triangle, restart the scene on failure.
   */
   static inline void
   retry_triangle_ccw(struct lp_setup_context *setup,
                     struct fixed_position *position,
   const float (*v0)[4],
   {
      if (0)
            if (lp_setup_zero_sample_mask(setup)) {
      if (0) debug_printf("zero sample mask\n");
   LP_COUNT(nr_culled_tris);
               if (!do_triangle_ccw(setup, position, v0, v1, v2, front)) {
      if (!lp_setup_flush_and_restart(setup))
            if (!do_triangle_ccw(setup, position, v0, v1, v2, front))
         }
         /**
   * Calculate fixed position data for a triangle
   * It is unfortunate we need to do that here (as we need area
   * calculated in fixed point), as there's quite some code duplication
   * to what is done in the jit setup prog.
   */
   static inline int8_t
   calc_fixed_position(struct lp_setup_context *setup,
                     struct fixed_position* position,
   {
      float pixel_offset = setup->multisample ? 0.0 : setup->pixel_offset;
   /*
   * The rounding may not be quite the same with DETECT_ARCH_SSE
   * (util_iround right now only does nearest/even on x87,
   * otherwise nearest/away-from-zero).
   * Both should be acceptable, I think.
      #if DETECT_ARCH_SSE
      __m128 v0r, v1r;
   __m128 vxy0xy2, vxy1xy0;
   __m128i vxy0xy2i, vxy1xy0i;
   __m128i dxdy0120, x0x2y0y2, x1x0y1y0, x0120, y0120;
   __m128 pix_offset = _mm_set1_ps(pixel_offset);
   __m128 fixed_one = _mm_set1_ps((float)FIXED_ONE);
   v0r = _mm_castpd_ps(_mm_load_sd((double *)v0[0]));
   vxy0xy2 = _mm_loadh_pi(v0r, (__m64 *)v2[0]);
   v1r = _mm_castpd_ps(_mm_load_sd((double *)v1[0]));
   vxy1xy0 = _mm_movelh_ps(v1r, vxy0xy2);
   vxy0xy2 = _mm_sub_ps(vxy0xy2, pix_offset);
   vxy1xy0 = _mm_sub_ps(vxy1xy0, pix_offset);
   vxy0xy2 = _mm_mul_ps(vxy0xy2, fixed_one);
   vxy1xy0 = _mm_mul_ps(vxy1xy0, fixed_one);
   vxy0xy2i = _mm_cvtps_epi32(vxy0xy2);
   vxy1xy0i = _mm_cvtps_epi32(vxy1xy0);
   dxdy0120 = _mm_sub_epi32(vxy0xy2i, vxy1xy0i);
   _mm_store_si128((__m128i *)&position->dx01, dxdy0120);
   /*
   * For the mul, would need some more shuffles, plus emulation
   * for the signed mul (without sse41), so don't bother.
   */
   x0x2y0y2 = _mm_shuffle_epi32(vxy0xy2i, _MM_SHUFFLE(3,1,2,0));
   x1x0y1y0 = _mm_shuffle_epi32(vxy1xy0i, _MM_SHUFFLE(3,1,2,0));
   x0120 = _mm_unpacklo_epi32(x0x2y0y2, x1x0y1y0);
   y0120 = _mm_unpackhi_epi32(x0x2y0y2, x1x0y1y0);
   _mm_store_si128((__m128i *)&position->x[0], x0120);
         #else
      position->x[0] = subpixel_snap(v0[0][0] - pixel_offset);
   position->x[1] = subpixel_snap(v1[0][0] - pixel_offset);
   position->x[2] = subpixel_snap(v2[0][0] - pixel_offset);
            position->y[0] = subpixel_snap(v0[0][1] - pixel_offset);
   position->y[1] = subpixel_snap(v1[0][1] - pixel_offset);
   position->y[2] = subpixel_snap(v2[0][1] - pixel_offset);
            position->dx01 = position->x[0] - position->x[1];
            position->dx20 = position->x[2] - position->x[0];
      #endif
         uint64_t area = IMUL64(position->dx01, position->dy20) -
            }
         /**
   * Rotate a triangle, flipping its clockwise direction,
   * Swaps values for xy[0] and xy[1]
   */
   static inline void
   rotate_fixed_position_01(struct fixed_position* position)
   {
      int x = position->x[1];
            position->x[1] = position->x[0];
   position->y[1] = position->y[0];
   position->x[0] = x;
            position->dx01 = -position->dx01;
   position->dy01 = -position->dy01;
   position->dx20 = position->x[2] - position->x[0];
      }
         /**
   * Rotate a triangle, flipping its clockwise direction,
   * Swaps values for xy[1] and xy[2]
   */
   static inline void
   rotate_fixed_position_12(struct fixed_position* position)
   {
      int x = position->x[2];
            position->x[2] = position->x[1];
   position->y[2] = position->y[1];
   position->x[1] = x;
            x = position->dx01;
   y = position->dy01;
   position->dx01 = -position->dx20;
   position->dy01 = -position->dy20;
   position->dx20 = -x;
      }
         /**
   * Draw triangle if it's CW, cull otherwise.
   */
   static void
   triangle_cw(struct lp_setup_context *setup,
               const float (*v0)[4],
   {
      alignas(16) struct fixed_position position;
            if (lp_context->active_statistics_queries) {
                           if (area_sign < 0) {
      if (setup->flatshade_first) {
      rotate_fixed_position_12(&position);
   retry_triangle_ccw(setup, &position, v0, v2, v1,
      } else {
      rotate_fixed_position_01(&position);
   retry_triangle_ccw(setup, &position, v1, v0, v2,
            }
         static void
   triangle_ccw(struct lp_setup_context *setup,
               const float (*v0)[4],
   {
      alignas(16) struct fixed_position position;
            if (lp_context->active_statistics_queries) {
                           if (area_sign > 0)
      }
         /**
   * Draw triangle whether it's CW or CCW.
   */
   static void
   triangle_both(struct lp_setup_context *setup,
               const float (*v0)[4],
   {
      alignas(16) struct fixed_position position;
            if (lp_context->active_statistics_queries) {
                           if (0) {
      assert(!util_is_inf_or_nan(v0[0][0]));
   assert(!util_is_inf_or_nan(v0[0][1]));
   assert(!util_is_inf_or_nan(v1[0][0]));
   assert(!util_is_inf_or_nan(v1[0][1]));
   assert(!util_is_inf_or_nan(v2[0][0]));
               if (area_sign > 0) {
      retry_triangle_ccw(setup, &position, v0, v1, v2,
      } else if (area_sign < 0) {
      if (setup->flatshade_first) {
      rotate_fixed_position_12(&position);
   retry_triangle_ccw(setup, &position, v0, v2, v1,
      } else {
      rotate_fixed_position_01(&position);
   retry_triangle_ccw(setup, &position, v1, v0, v2,
            }
         static void
   triangle_noop(struct lp_setup_context *setup,
               const float (*v0)[4],
   {
   }
         void
   lp_setup_choose_triangle(struct lp_setup_context *setup)
   {
      if (setup->rasterizer_discard) {
      setup->triangle = triangle_noop;
      }
   switch (setup->cullmode) {
   case PIPE_FACE_NONE:
      setup->triangle = triangle_both;
      case PIPE_FACE_BACK:
      setup->triangle = setup->ccw_is_frontface ? triangle_ccw : triangle_cw;
      case PIPE_FACE_FRONT:
      setup->triangle = setup->ccw_is_frontface ? triangle_cw : triangle_ccw;
      default:
      setup->triangle = triangle_noop;
         }
