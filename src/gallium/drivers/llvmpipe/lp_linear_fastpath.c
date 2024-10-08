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
   #include "util/u_surface.h"
   #include "util/u_sse.h"
      #include "lp_jit.h"
   #include "lp_rast.h"
   #include "lp_debug.h"
   #include "lp_state_fs.h"
   #include "lp_linear_priv.h"
         /* This file contains various special-case fastpaths which implement
   * the entire linear pipeline in a single funciton.
   *
   * These include simple blits and some debug code.
   *
   * These functions fully implement the linear path and do not need to
   * be combined with blending, interpolation or sampling routines.
   */
         #if DETECT_ARCH_SSE
      /* Linear shader which implements the BLIT_RGBA shader with the
   * additional constraints imposed by lp_setup_is_blit().
   */
   static bool
   lp_linear_blit_rgba_blit(const struct lp_rast_state *state,
                           unsigned x, unsigned y,
   unsigned width, unsigned height,
   const float (*a0)[4],
   {
      const struct lp_jit_resources *resources = &state->jit_resources;
                     /* Require w==1.0:
   */
   if (a0[0][3] != 1.0 ||
      dadx[0][3] != 0.0 ||
   dady[0][3] != 0.0)
         const int src_x = x + util_iround(a0[1][0]*texture->width - 0.5f);
            const uint8_t *src = texture->base;
            /* Fall back to blit_rgba() if clamping required:
   */
   if (src_x < 0 ||
      src_y < 0 ||
   src_x + width > texture->width ||
   src_y + height > texture->height)
         util_copy_rect(color, PIPE_FORMAT_B8G8R8A8_UNORM, stride,
                  x, y,
            }
         /* Linear shader which implements the BLIT_RGB1 shader, with the
   * additional constraints imposed by lp_setup_is_blit().
   */
   static bool
   lp_linear_blit_rgb1_blit(const struct lp_rast_state *state,
                  unsigned x, unsigned y,
   unsigned width, unsigned height,
   const float (*a0)[4],
   const float (*dadx)[4],
      {
      const struct lp_jit_resources *resources = &state->jit_resources;
                     /* Require w==1.0:
   */
   if (a0[0][3] != 1.0 ||
      dadx[0][3] != 0.0 ||
   dady[0][3] != 0.0)
                  const int src_x = x + util_iround(a0[1][0]*texture->width - 0.5f);
            const uint8_t *src = texture->base;
   const unsigned src_stride = texture->row_stride[0];
   src += src_x * 4;
            if (src_x < 0 ||
      src_y < 0 ||
   src_x + width > texture->width ||
   src_y + height > texture->height)
         for (y = 0; y < height; y++) {
      const uint32_t *src_row = (const uint32_t *)src;
            for (x = 0; x < width; x++) {
                  color += stride;
                  }
         /* Linear shader which always emits purple.  Used for debugging.
   */
   static bool
   lp_linear_purple(const struct lp_rast_state *state,
                  unsigned x, unsigned y,
   unsigned width, unsigned height,
   const float (*a0)[4],
   const float (*dadx)[4],
      {
               util_pack_color_ub(0xff, 0, 0xff, 0xff,
            util_fill_rect(color,
                  PIPE_FORMAT_B8G8R8A8_UNORM,
   stride,
   x,
   y,
            }
         /* Examine the fragment shader variant and determine whether we can
   * substitute a fastpath linear shader implementation.
   */
   bool
   lp_linear_check_fastpath(struct lp_fragment_shader_variant *variant)
   {
      struct lp_sampler_static_state *samp0 =
            if (!samp0)
            const enum pipe_format tex_format = samp0->texture_state.format;
   if (variant->shader->kind == LP_FS_KIND_BLIT_RGBA &&
      tex_format == PIPE_FORMAT_B8G8R8A8_UNORM &&
   is_nearest_clamp_sampler(samp0) &&
   variant->opaque) {
               if (variant->shader->kind == LP_FS_KIND_BLIT_RGB1 &&
      variant->opaque &&
   (tex_format == PIPE_FORMAT_B8G8R8A8_UNORM ||
   tex_format == PIPE_FORMAT_B8G8R8X8_UNORM) &&
   is_nearest_clamp_sampler(samp0)) {
               if (0) {
                     /* Stop now if jit_linear has been initialized.  Otherwise keep
   * searching - even if jit_linear_blit has been instantiated.
   */
      }
      #else
      bool
   lp_linear_check_fastpath(struct lp_fragment_shader_variant *variant)
   {
         }
      #endif
