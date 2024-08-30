   /**************************************************************************
   *
   * Copyright 2010-2021 VMware, Inc.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
         #include "util/detect.h"
      #include "util/u_math.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_pack_color.h"
   #include "util/u_rect.h"
   #include "util/u_sse.h"
      #include "lp_jit.h"
   #include "lp_debug.h"
   #include "lp_state_fs.h"
   #include "lp_linear_priv.h"
      #if DETECT_ARCH_SSE
      #define FIXED16_SHIFT  16
   #define FIXED16_ONE    (1<<16)
   #define FIXED16_HALF   (1<<15)
      /*
   * Color tolerance.  Allow 1 bit of error in 8 bit unorm colors.
   */
   #define FIXED16_TOL (FIXED16_ONE >> 7)
      /*
   * Tolerance for texture coordinate derivatives when doing linear filtering.
   *
   * (Note that extra care needs to be taken when doing linear filtering as
   * coordinates may snap up to neighbour texels inside the tile).
   */
   #define FIXED16_TOL_DERIV (FIXED16_TOL / TILE_SIZE)
         static inline int
   float_to_fixed16(float f)
   {
         }
         static inline int
   fixed16_frac(int x)
   {
         }
         static inline int
   fixed16_approx(int x, int y, int tol)
   {
         }
      /* set alpha channel of rgba value to 0xff. */
   static inline uint32_t
   rgbx(uint32_t src_val)
   {
         }
      /* swap red/blue channels of a 32-bit rgba value. */
   static inline uint32_t
   rb_swap(uint32_t src_val)
   {
      uint32_t dst_val = src_val & 0xff00ff00;
   dst_val |= (src_val & 0xff) << 16;
   dst_val |= (src_val & 0xff0000) >> 16;
      }
      /* swap red/blue channels and set alpha to 0xff
   * of a 32-bit rgbx value. */
   static inline uint32_t
   rbx_swap(uint32_t src_val)
   {
      uint32_t dst_val = 0xff000000;
   dst_val |= src_val & 0xff00;
   dst_val |= (src_val & 0xff) << 16;
   dst_val |= (src_val & 0xff0000) >> 16;
      }
      /* set alpha channel of 128-bit 4xrgba values to 0xff. */
   static inline __m128i
   rgbx_128(const __m128i src_val)
   {
      const __m128i mask = _mm_set1_epi32(0xff000000);
   __m128i bgrx = _mm_or_si128(src_val, mask);
      }
      /* swap red/blue channels of a 128-bit 4xrgba value. */
   /* ssse3 could use pshufb */
   static inline __m128i
   rb_swap_128(const __m128i src_val)
   {
      const __m128i mask = _mm_set1_epi32(0xff00ff00);
            __m128i rgba = _mm_and_si128(src_val, mask);
   __m128i r = _mm_srli_epi32(src_val, 16);
   __m128i b = _mm_and_si128(src_val, mask_r);
   r = _mm_and_si128(r, mask_r);
   b = _mm_slli_epi32(b, 16);
   rgba = _mm_or_si128(rgba, r);
   rgba = _mm_or_si128(rgba, b);
      }
      /* swap red/blue channels and set alpha to 0xff
   * of a 128-bit 4xrgbx value. */
   static inline __m128i
   rbx_swap_128(const __m128i src_val)
   {
      const __m128i mask_a = _mm_set1_epi32(0xff000000);
   const __m128i mask_g = _mm_set1_epi32(0xff00);
            __m128i rgbx = _mm_and_si128(src_val, mask_g);
   __m128i r = _mm_srli_epi32(src_val, 16);
   __m128i b = _mm_and_si128(src_val, mask_r);
   r = _mm_and_si128(r, mask_r);
   b = _mm_slli_epi32(b, 16);
   rgbx = _mm_or_si128(rgbx, mask_a);
   rgbx = _mm_or_si128(rgbx, r);
   rgbx = _mm_or_si128(rgbx, b);
      }
      /*
   * Unstretched blit of a bgra texture.
   */
   static const uint32_t *
   fetch_memcpy_bgra(struct lp_linear_elem *elem)
   {
      struct lp_linear_sampler *samp = (struct lp_linear_sampler *)elem;
   const struct lp_jit_texture *texture = samp->texture;
   const uint32_t *src_row =
      (const uint32_t *)((const uint8_t *)texture->base +
      const int s     = samp->s;
   const int width = samp->width;
                     if (((uintptr_t)src_row & 0xf) == 0) {
      /* The source texels are already aligned. Return them */
      } else {
      memcpy(samp->row, src_row, width * sizeof *row);
               samp->t += samp->dtdy;
      }
      /**
   * Fetch and stretch one row.
   */
   static inline const uint32_t *
   fetch_and_stretch_bgra_row(struct lp_linear_sampler *samp,
         {
      const struct lp_jit_texture *texture = samp->texture;
   const uint32_t *data = (const uint32_t *)texture->base;
   const int stride = texture->row_stride[0] / sizeof(uint32_t);
            /*
   * Search the stretched row cache first.
            if (y == samp->stretched_row_y[0]) {
      samp->stretched_row_index = 1;
               if (y == samp->stretched_row_y[1]) {
      samp->stretched_row_index = 0;
               /*
   * Replace one entry.
            const uint32_t * restrict src_row = data + y * stride;
            if (fixed16_frac(samp->s) == 0 &&
      samp->dsdx == FIXED16_ONE) { // TODO: could be relaxed
   /*
   * 1:1 blit on the x direction.
   */
            if (((uintptr_t)src_row & 0xf) == 0) {
      /* The source texture is already aligned. Return it */
               /* Copy the source texture */
   for (int i = 0; i < width; i += 4) {
      __m128i src = _mm_loadu_si128((const __m128i *)&src_row[i]);
         } else {
      util_sse2_stretch_row_8unorm((__m128i *)dst_row,
                     samp->stretched_row_y[samp->stretched_row_index] = y;
               }
         /* Maximise only as we fetch unscaled pixels linearly into a size-64
   * temporary.  For minimise, we will want to either have a bigger
   * temporary or fetch sparsely.
   */
   static const uint32_t *
   fetch_axis_aligned_linear_bgra(struct lp_linear_elem *elem)
   {
      struct lp_linear_sampler *samp = (struct lp_linear_sampler *)elem;
   const int width = samp->width;
   uint32_t * restrict row = samp->row;
   const int y = samp->t >> FIXED16_SHIFT;
                              if (w == 0) {
                                    /* Combine the two rows using a constant weight.
   */
   for (int i = 0; i < width; i += 4) {
      __m128i srca = _mm_load_si128((const __m128i *)&src_row0[i]);
                           }
         /* Non-axis-aligned version.  Don't try to take advantage of
   * maximize.
   */
   static const uint32_t *
   fetch_linear_bgra(struct lp_linear_elem *elem)
   {
      struct lp_linear_sampler *samp = (struct lp_linear_sampler *)elem;
   const struct lp_jit_texture *texture = samp->texture;
   const int stride     = texture->row_stride[0] / sizeof(uint32_t);
   const uint32_t *data  = (const uint32_t *)texture->base;
   const int dsdx  = samp->dsdx;
   const int dtdx  = samp->dtdx;
   const int width = samp->width;
   uint32_t *row   = samp->row;
   int s = samp->s;
            for (int i = 0; i < width; i += 4) {
      union m128i si0, si1, si2, si3, ws, wt;
            for (int j = 0; j < 4; j++) {
               si0.ui[j] = src[0];
   si1.ui[j] = src[1];
                                 s += dsdx;
               ws.m = _mm_or_si128(ws.m, _mm_slli_epi32(ws.m, 16));
            wt.m = _mm_or_si128(wt.m, _mm_slli_epi32(wt.m, 16));
            si02 = util_sse2_lerp_epi8_fixed08(si0.m, si2.m, wt.m);
                        samp->s += samp->dsdy;
   samp->t += samp->dtdy;
      }
         /* Clamped, non-axis-aligned version.  Don't try to take advantage of
   * maximize.
   */
   static const uint32_t *
   fetch_clamp_linear_bgra(struct lp_linear_elem *elem)
   {
      struct lp_linear_sampler *samp = (struct lp_linear_sampler *)elem;
   const struct lp_jit_texture *texture = samp->texture;
   const uint32_t *data  = (const uint32_t *)texture->base;
   const int stride     = texture->row_stride[0] / sizeof(uint32_t);
   const int tex_height = texture->height - 1;
   const int tex_width  = texture->width - 1;
   const int dsdx  = samp->dsdx;
   const int dtdx  = samp->dtdx;
   const int width = samp->width;
   uint32_t *row   = samp->row;
   int s = samp->s;
            /* width, height, stride (in pixels) must be smaller than 32768 */
   __m128i dsdx4, dtdx4, s4, t4, stride4, w4, h4, zero, one;
   s4 = _mm_set1_epi32(s);
   t4 = _mm_set1_epi32(t);
   s4 = _mm_add_epi32(s4, _mm_set_epi32(3*dsdx, 2*dsdx, dsdx, 0));
   t4 =  _mm_add_epi32(t4, _mm_set_epi32(3*dtdx, 2*dtdx, dtdx, 0));
   dsdx4 = _mm_set1_epi32(4*dsdx);
   dtdx4 = _mm_set1_epi32(4*dtdx);
   stride4 = _mm_set1_epi32(stride);
   w4 = _mm_set1_epi32(tex_width);
   h4 = _mm_set1_epi32(tex_height);
   zero = _mm_setzero_si128();
            for (int i = 0; i < width; i += 4) {
      union m128i addr[4];
   __m128i ws, wt, wsl, wsh, wtl, wth;
            s4s = _mm_srli_epi32(s4, 16);
   t4s = _mm_srli_epi32(t4, 16);
   cs0 = _mm_min_epi16(_mm_max_epi16(s4s, zero), w4);
   cs1 = _mm_add_epi16(s4s, one);
   cs1 = _mm_min_epi16(_mm_max_epi16(cs1, zero), w4);
   ct0 = _mm_min_epi16(_mm_max_epi16(t4s, zero), h4);
   ct1 = _mm_add_epi16(t4s, one);
   ct1 = _mm_min_epi16(_mm_max_epi16(ct1, zero), h4);
   tmp = _mm_madd_epi16(ct0, stride4);
   addr[0].m = _mm_add_epi32(tmp, cs0);
   addr[1].m = _mm_add_epi32(tmp, cs1);
   tmp = _mm_madd_epi16(ct1, stride4);
   addr[2].m = _mm_add_epi32(tmp, cs0);
            for (int j = 0; j < 4; j++) {
      __m128i ld1, ld2, ld3;
   si[j] = _mm_cvtsi32_si128(data[addr[j].ui[0]]);
   ld1 = _mm_cvtsi32_si128(data[addr[j].ui[1]]);
   si[j] = _mm_unpacklo_epi32(si[j], ld1);
   ld2 = _mm_cvtsi32_si128(data[addr[j].ui[2]]);
   ld3 = _mm_cvtsi32_si128(data[addr[j].ui[3]]);
   ld2 = _mm_unpacklo_epi32(ld2, ld3);
               ws = _mm_srli_epi32(s4, 8);
   ws = _mm_and_si128(ws, _mm_set1_epi32(0xFF));
   wt = _mm_srli_epi32(t4, 8);
            s4 = _mm_add_epi32(s4, dsdx4);
      #if 0
   /* scalar code for reference */
         for (int j = 0; j < 4; j++) {
      int s0 = s >> FIXED16_SHIFT;
   int t0 = t >> FIXED16_SHIFT;
   int cs0 = CLAMP(s0    , 0, tex_width);
   int cs1 = CLAMP(s0 + 1, 0, tex_width);
                  si0.ui[j] = data[ct0 * stride + cs0];
   si1.ui[j] = data[ct0 * stride + cs1];
                                 s += dsdx;
      #endif
            ws = _mm_or_si128(ws, _mm_slli_epi32(ws, 16));
   wsl = _mm_shuffle_epi32(ws, _MM_SHUFFLE(1,1,0,0));
            wt = _mm_or_si128(wt, _mm_slli_epi32(wt, 16));
   wtl = _mm_shuffle_epi32(wt, _MM_SHUFFLE(1,1,0,0));
            *(__m128i *)&row[i] = util_sse2_lerp_2d_epi8_fixed88(si[0], si[2],
                           samp->s += samp->dsdy;
               }
      /* don't generate bgra 128-bits or memcpy ops they have their own path */
   #define FETCH_TYPE bgra
   #define OP
   #define NO_MEMCPY
   #include "lp_linear_sampler_tmp.h"
      #define FETCH_TYPE bgrx
   #define OP rgbx
   #define OP128 rgbx_128
   #include "lp_linear_sampler_tmp.h"
      #define FETCH_TYPE bgra_swapped
   #define OP rb_swap
   #define OP128 rb_swap_128
   #include "lp_linear_sampler_tmp.h"
      #define FETCH_TYPE bgrx_swapped
   #define OP rbx_swap
   #define OP128 rbx_swap_128
   #include "lp_linear_sampler_tmp.h"
      static bool
   sampler_is_nearest(const struct lp_linear_sampler *samp,
               {
               if (minify)
         else
            /* Is it obviously nearest?
   */
   if (img_filter == PIPE_TEX_FILTER_NEAREST)
            /* Otherwise look for linear samplers which devolve to nearest.
            /* Needs to be axis aligned.
   */
   if (!samp->axis_aligned)
            if (0) {
      /* For maximizing shaders, revert to nearest
   */
   if (samp->dsdx < -FIXED16_HALF && samp->dsdx < FIXED16_HALF &&
                  /* For severely minimising shaders, revert to nearest:
   */
   if ((samp->dsdx < 2 * FIXED16_ONE || samp->dsdx > 2 * FIXED16_ONE) &&
      (samp->dtdy < 2 * FIXED16_ONE || samp->dtdy > 2 * FIXED16_ONE))
            /*
   * Must be near a pixel center:
   */
   if (!fixed16_approx(fixed16_frac(samp->s), FIXED16_HALF, FIXED16_TOL) ||
      !fixed16_approx(fixed16_frac(samp->t), FIXED16_HALF, FIXED16_TOL))
         /*
   * Must make a full step between pixels:
   */
   if (!fixed16_approx(samp->dsdx, FIXED16_ONE, FIXED16_TOL_DERIV) ||
      !fixed16_approx(samp->dtdy, FIXED16_ONE, FIXED16_TOL_DERIV))
         /* Treat it as nearest!
   */
      }
         /* XXX: Lots of static-state parameters being passed in here but very
   * little info is extracted from each one.  Consolidate it all down to
   * something succinct in the prepare phase?
   */
   bool
   lp_linear_init_sampler(struct lp_linear_sampler *samp,
                        const struct lp_tgsi_texture_info *info,
   const struct lp_sampler_static_state *sampler_state,
   const struct lp_jit_texture *texture,
   int x0, int y0, int width, int height,
      {
      const struct lp_tgsi_channel_info *schan = &info->coord[0];
            assert(schan->file == TGSI_FILE_INPUT);
                     int foo = 1;
   float s0   =   a0[schan->u.index+foo][schan->swizzle];
   float dsdx = dadx[schan->u.index+foo][schan->swizzle];
            float t0   =   a0[tchan->u.index+foo][tchan->swizzle];
   float dtdx = dadx[tchan->u.index+foo][tchan->swizzle];
            int mins, mint, maxs, maxt;
   float oow = 1.0f / w0;
   float width_oow = texture->width * oow;
   float height_oow = texture->height * oow;
   float fdsdx = dsdx * width_oow;
   float fdsdy = dsdy * width_oow;
   float fdtdx = dtdx * height_oow;
   float fdtdy = dtdy * height_oow;
   int fetch_width;
   int fetch_height;
   bool minify;
   bool need_wrap;
            samp->texture = texture;
            samp->s = float_to_fixed16(fdsdx * x0 +
                  samp->t = float_to_fixed16(fdtdx * x0 +
                  samp->dsdx = float_to_fixed16(fdsdx);
   samp->dsdy = float_to_fixed16(fdsdy);
   samp->dtdx = float_to_fixed16(fdtdx);
               samp->axis_aligned = (samp->dsdy == 0 &&
            {
      int dsdx = samp->dsdx >= 0 ? samp->dsdx : -samp->dsdx;
   int dsdy = samp->dsdy >= 0 ? samp->dsdy : -samp->dsdy;
   int dtdx = samp->dtdx >= 0 ? samp->dtdx : -samp->dtdx;
   int dtdy = samp->dtdy >= 0 ? samp->dtdy : -samp->dtdy;
                                 if (!is_nearest) {
      samp->s -= FIXED16_HALF;
               /* Check for clamping.  This rarely happens as we're rejecting interpolants
   * which fall outside the 0..1 range.
            if (is_nearest) {
      /* Nearest fetch routines don't employ SSE and always operate one pixel
   * at a time.
   */
      } else {
      /* Linear fetch routines employ SSE, and always fetch groups of four
   * texels.
   */
      }
            if (samp->axis_aligned) {
      int s0 = samp->s;
   int s1 = samp->s + fetch_width  * samp->dsdx;
   int t0 = samp->t;
            mins = MIN2(s0, s1);
   mint = MIN2(t0, t1);
   maxs = MAX2(s0, s1);
      } else {
      int s0 = samp->s;
   int s1 = samp->s + fetch_width  * samp->dsdx;
   int s2 = samp->s + fetch_height * samp->dsdy;
   int s3 = samp->s + fetch_width  * samp->dsdx + fetch_height * samp->dsdy;
   int t0 = samp->t;
   int t1 = samp->t + fetch_width  * samp->dtdx;
   int t2 = samp->t + fetch_height * samp->dtdy;
            mins = MIN4(s0, s1, s2, s3);
   mint = MIN4(t0, t1, t2, t3);
   maxs = MAX4(s0, s1, s2, s3);
               if (is_nearest) {
      need_wrap = (mins < 0 ||
                  } else {
      need_wrap = (mins < 0 ||
                           if (0 && need_wrap) {
      debug_printf("%u x %u %s\n",
               debug_printf("mins = %f\n", mins*1.0f/FIXED16_ONE);
   debug_printf("mint = %f\n", mint*1.0f/FIXED16_ONE);
   debug_printf("maxs = %f\n", maxs*1.0f/FIXED16_ONE);
   debug_printf("maxt = %f\n", maxt*1.0f/FIXED16_ONE);
               /* We accept any mode below, but we only implement clamping.
   */
   if (need_wrap &&
      (sampler_state->sampler_state.wrap_s != PIPE_TEX_WRAP_CLAMP_TO_EDGE ||
   sampler_state->sampler_state.wrap_t != PIPE_TEX_WRAP_CLAMP_TO_EDGE)) {
               if (is_nearest) {
      switch (sampler_state->texture_state.format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      if (rgba_order) {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else if (samp->dsdx != FIXED16_ONE) // TODO: could be relaxed
         else
      } else {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else if (samp->dsdx != FIXED16_ONE) // TODO: could be relaxed
         else
      }
      case PIPE_FORMAT_B8G8R8X8_UNORM:
      if (rgba_order) {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else if (samp->dsdx != FIXED16_ONE) // TODO: could be relaxed
         else
      } else {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else if (samp->dsdx != FIXED16_ONE) // TODO: could be relaxed
         else
      }
      case PIPE_FORMAT_R8G8B8A8_UNORM:
      if (!rgba_order) {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else if (samp->dsdx != FIXED16_ONE) // TODO: could be relaxed
         else
      } else {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else if (samp->dsdx != FIXED16_ONE) // TODO: could be relaxed
         else
      }
      case PIPE_FORMAT_R8G8B8X8_UNORM:
      if (!rgba_order) {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else if (samp->dsdx != FIXED16_ONE) // TODO: could be relaxed
         else
      } else {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else if (samp->dsdx != FIXED16_ONE) // TODO: could be relaxed
         else
      }
      default:
                     } else {
      samp->stretched_row_y[0] = -1;
   samp->stretched_row_y[1] = -1;
            switch (sampler_state->texture_state.format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      if (rgba_order) {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else
      } else {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else
      }
      case PIPE_FORMAT_B8G8R8X8_UNORM:
      if (rgba_order) {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else
      } else {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else
      }
      case PIPE_FORMAT_R8G8B8A8_UNORM:
      if (!rgba_order) {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else
      } else {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else
      }
      case PIPE_FORMAT_R8G8B8X8_UNORM:
      if (!rgba_order) {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else
      } else {
      if (need_wrap)
         else if (!samp->axis_aligned)
         else
      }
      default:
                        }
         static const uint32_t *
   fetch_noop(struct lp_linear_elem *elem)
   {
      struct lp_linear_sampler *samp = (struct lp_linear_sampler *)elem;
      }
         void
   lp_linear_init_noop_sampler(struct lp_linear_sampler *samp)
   {
         }
         /*
   * Check the given sampler and texture info for linear path compatibility.
   */
   bool
   lp_linear_check_sampler(const struct lp_sampler_static_state *sampler,
         {
      if (tex->modifier != LP_BLD_TEX_MODIFIER_NONE)
            if (tex->target != TGSI_TEXTURE_2D)
            if (tex->coord[0].file != TGSI_FILE_INPUT ||
      tex->coord[1].file != TGSI_FILE_INPUT)
         /* These are the only sampling modes we support at the moment.
   *
   * Actually we'll accept any mode as we're failing on any
   * interpolant which exceeds 0..1.  Clamping is applied only to
   * avoid invalid reads.
   */
   if (!is_nearest_sampler(sampler) &&
      !is_linear_sampler(sampler))
         /* These are the only texture formats we support at the moment
   */
   if (sampler->texture_state.format != PIPE_FORMAT_B8G8R8A8_UNORM &&
      sampler->texture_state.format != PIPE_FORMAT_B8G8R8X8_UNORM &&
   sampler->texture_state.format != PIPE_FORMAT_R8G8B8A8_UNORM &&
   sampler->texture_state.format != PIPE_FORMAT_R8G8B8X8_UNORM)
         /* We don't support sampler view swizzling on the linear path */
   if (sampler->texture_state.swizzle_r != PIPE_SWIZZLE_X ||
      sampler->texture_state.swizzle_g != PIPE_SWIZZLE_Y ||
   sampler->texture_state.swizzle_b != PIPE_SWIZZLE_Z ||
   sampler->texture_state.swizzle_a != PIPE_SWIZZLE_W) {
                  }
      #else  // DETECT_ARCH_SSE
      bool
   lp_linear_check_sampler(const struct lp_sampler_static_state *sampler,
         {
         }
      #endif  // DETECT_ARCH_SSE
