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
   #include "lp_rast.h"
   #include "lp_debug.h"
   #include "lp_state_fs.h"
   #include "lp_linear_priv.h"
         #if DETECT_ARCH_SSE
      #define FIXED15_ONE 0x7fff
         /* Translate floating point value to 1.15 unsigned fixed-point.
   */
   static inline uint16_t
   float_to_ufixed_1_15(float f)
   {
         }
         /* Translate floating point value to 1.15 signed fixed-point.
   */
   static inline int16_t
   float_to_sfixed_1_15(float f)
   {
         }
         /* Interpolate in 1.15 space, but produce a packed row of 0.8 values.
   */
   static const uint32_t *
   interp_0_8(struct lp_linear_elem *elem)
   {
      struct lp_linear_interp *interp = (struct lp_linear_interp *)elem;
   uint32_t *row = interp->row;
   __m128i a0 = interp->a0;
   const __m128i dadx = interp->dadx;
            for (int i = 0; i < width; i += 4) {
      __m128i l = _mm_srai_epi16(a0, 7); // l = a0 >> 7
            __m128i h = _mm_srai_epi16(a0, 7); // h = a0 >> 7
            // pack l[0..7] and h[0..7] as 16 bytes
               // advance to next row
   interp->a0 = _mm_add_epi16(interp->a0, interp->dady);
      }
         static const uint32_t *
   interp_noop(struct lp_linear_elem *elem)
   {
      struct lp_linear_interp *interp = (struct lp_linear_interp *)elem;
      }
         static const uint32_t *
   interp_check(struct lp_linear_elem *elem)
   {
      struct lp_linear_interp *interp = (struct lp_linear_interp *)elem;
   interp->row[0] = 1;
      }
         /* Not quite a noop - we use row[0] to track whether this gets called
   * or not, so we can optimize which interpolants we care about.
   */
   void
   lp_linear_init_noop_interp(struct lp_linear_interp *interp)
   {
      interp->row[0] = 0;
      }
         bool
   lp_linear_init_interp(struct lp_linear_interp *interp,
                        int x, int y, int width, int height,
   unsigned usage_mask,
   bool perspective,
      {
      float s0[4];
   float dsdx[4];
   float dsdy[4];
   int16_t s0_fp[8];
   int16_t dsdx_fp[4];
            /* Zero coefficients to avoid using uninitialised values */
   memset(s0, 0, sizeof(s0));
   memset(dsdx, 0, sizeof(dsdx));
   memset(dsdy, 0, sizeof(dsdy));
   memset(s0_fp, 0, sizeof(s0_fp));
   memset(dsdx_fp, 0, sizeof(dsdx_fp));
            if (perspective && oow != 1.0f) {
      for (unsigned j = 0; j < 4; j++) {
      if (usage_mask & (1<<j)) {
      s0[j]   =   a0[j] * oow;
   dsdx[j] = dadx[j] * oow;
            } else {
      for (unsigned j = 0; j < 4; j++) {
      if (usage_mask & (1<<j)) {
      s0[j]   =   a0[j];
   dsdx[j] = dadx[j];
                     s0[0] += x * dsdx[0] + y * dsdy[0];
   s0[1] += x * dsdx[1] + y * dsdy[1];
   s0[2] += x * dsdx[2] + y * dsdy[2];
            /* XXX: lift all of this into the rectangle setup code.
   *
   * For rectangles with linear shaders, at setup time:
   *    - if w is constant (else mark as non-fastpath)
   *        - premultiply perspective interpolants by w
   *        - set w = 1 in position
   *   - check all interpolants for min/max 0..1 (else mark as
   *          non-fastpath)
   */
   for (unsigned j = 0; j < 4; j++) {
      if (usage_mask & (1<<j)) {
      // compute texcoords at rect corners
   float a = s0[j];
   float b = s0[j] + (width  - 1) * dsdx[j];
                                                                                             interp->width = align(width, 4);
   /* RGBA->BGRA swizzle here */
   interp->a0    = _mm_setr_epi16(s0_fp[2], s0_fp[1], s0_fp[0], s0_fp[3],
            interp->dadx  = _mm_setr_epi16(dsdx_fp[2], dsdx_fp[1], dsdx_fp[0], dsdx_fp[3],
            interp->dady  = _mm_setr_epi16(dsdy_fp[2], dsdy_fp[1], dsdy_fp[0], dsdy_fp[3],
            /* If the value is y-invariant, eagerly calculate it here and then
   * always return the precalculated value.
   */
   if (dsdy[0] == 0 &&
      dsdy[1] == 0 &&
   dsdy[2] == 0 &&
   dsdy[3] == 0) {
   interp_0_8(&interp->base);
      } else {
                     }
      #else //DETECT_ARCH_SSE
      bool
   lp_linear_init_interp(struct lp_linear_interp *interp,
                        int x, int y, int width, int height,
   unsigned usage_mask,
   bool perspective,
      {
         }
      #endif //DETECT_ARCH_SSE
