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
         #if DETECT_ARCH_SSE
      #include <emmintrin.h>
         struct nearest_sampler {
               const struct lp_jit_texture *texture;
   float fsrc_x;                /* src_x0 */
   float fsrc_y;                /* src_y0 */
   float fdsdx;              /* sx */
   float fdsdy;              /* sx */
   float fdtdx;              /* sy */
   float fdtdy;              /* sy */
   int width;
               };
         struct linear_interp {
      alignas(16) uint32_t out[64];
   __m128i a0;
   __m128i dadx;
   __m128i dady;
   int width;                   /* rounded up to multiple of 4 */
      };
      /* Organize all the information needed for blending in one place.
   * Could have blend function pointer here, but we currently always
   * know which one we want to call.
   */
   struct color_blend {
      const uint32_t *src;
   uint8_t *color;
   int stride;
      };
         /* Organize all the information needed for running each of the shaders
   * in one place.
   */
   struct shader {
      alignas(16) uint32_t out0[64];
   const uint32_t *src0;
   const uint32_t *src1;
   __m128i const0;
      };
         /* For a row of pixels, perform add/one/inv_src_alpha (ie
   * premultiplied alpha) blending between the incoming pixels and the
   * destination buffer.
   *
   * Used to implement the BLIT_RGBA + blend shader, there are no
   * operations from the pixel shader left to implement at this level -
   * effectively the pixel shader was just a texture fetch which has
   * already been performed.  This routine then purely implements
   * blending.
   */
   static void
   blend_premul(struct color_blend *blend)
   {
      const uint32_t *src = blend->src;  /* aligned */
   uint32_t *dst = (uint32_t *)blend->color;      /* unaligned */
   const int width = blend->width;
   int i;
                     for (i = 0; i + 3 < width; i += 4) {
      __m128i tmp;
   tmp = _mm_loadu_si128((const __m128i *)&dst[i]);  /* UNALIGNED READ */
   dstreg.m128 = util_sse2_blend_premul_4(*(const __m128i *)&src[i],
                     if (i < width) {
      int j;
   for (j = 0; j < width - i ; j++) {
         }
   dstreg.m128 = util_sse2_blend_premul_4(*(const __m128i *)&src[i],
         for (; i < width; i++)
         }
         static void
   blend_noop(struct color_blend *blend)
   {
      memcpy(blend->color, blend->src, blend->width * sizeof(unsigned));
      }
         static void
   init_blend(struct color_blend *blend,
            int x, int y, int width, int height,
      {
      blend->color = color + x * 4 + y * stride;
   blend->stride = stride;
      }
         /*
   * Perform nearest filtered lookup of a row of texels.  Texture lookup
   * is assumed to be axis aligned but with arbitrary scaling.
   *
   * Texture coordinate interpolation is performed in 24.8 fixed point.
   * Note that the longest span we will encounter is 64 pixels long,
   * meaning that 8 fractional bits is more than sufficient to represent
   * the shallowest gradient possible within this span.
   *
   * After 64 pixels (ie. in the next tile), the starting point will be
   * recalculated with floating point arithmetic.
   *
   * XXX: migrate this to use Jose's quad blitter texture fetch routines.
   */
   static const uint32_t *
   fetch_row(struct nearest_sampler *samp)
   {
      const int y = samp->y++;
   uint32_t *row = samp->out;
   const struct lp_jit_texture *texture = samp->texture;
   const int yy = util_iround(samp->fsrc_y + samp->fdtdy * y);
   const uint32_t *src_row =
      (const uint32_t *)((const uint8_t *)texture->base +
      const int iscale_x = samp->fdsdx * 256;
   const int width = samp->width;
            for (int i = 0; i < width; i++) {
      row[i] = src_row[acc>>8];
                  }
         /* Version of fetch_row which can cope with texture edges.  In
   * practise, aero never triggers this.
   */
   static const uint32_t *
   fetch_row_clamped(struct nearest_sampler *samp)
   {
      const int y = samp->y++;
   uint32_t *row = samp->out;
   const struct lp_jit_texture *texture = samp->texture;
   const int yy = util_iround(samp->fsrc_y + samp->fdtdy * y);
   const uint32_t *src_row =
      (const uint32_t *)((const uint8_t *)texture->base +
            const float src_x0 = samp->fsrc_x;
   const float scale_x = samp->fdsdx;
            for (int i = 0; i < width; i++) {
      row[i] = src_row[CLAMP(util_iround(src_x0 + i * scale_x),
                  }
      /* It vary rarely happens that some non-axis-aligned texturing creeps
   * into the linear path.  Handle it here.  The alternative would be
   * more pre-checking or an option to fallback by returning false from
   * jit_linear.
   */
   static const uint32_t *
   fetch_row_xy_clamped(struct nearest_sampler *samp)
   {
      const int y = samp->y++;
   uint32_t *row = samp->out;
   const struct lp_jit_texture *texture = samp->texture;
   const float yrow = samp->fsrc_y + samp->fdtdy * y;
   const float xrow = samp->fsrc_x + samp->fdsdy * y;
            for (int i = 0; i < width; i++) {
      int yy = util_iround(yrow + samp->fdtdx * i);
            const uint32_t *src_row =
      (const uint32_t *)((const uint8_t *) texture->base +
                              }
         static bool
   init_nearest_sampler(struct nearest_sampler *samp,
                        const struct lp_jit_texture *texture,
   int x0, int y0,
      {
               if (dwdx != 0.0 || dwdy != 0.0)
            samp->texture = texture;
   samp->width = width;
   samp->fdsdx = dsdx * texture->width * oow;
   samp->fdsdy = dsdy * texture->width * oow;
   samp->fdtdx = dtdx * texture->height * oow;
   samp->fdtdy = dtdy * texture->height * oow;
   samp->fsrc_x = (samp->fdsdx * x0 +
                  samp->fsrc_y = (samp->fdtdx * x0 +
                        /* Because we want to permit consumers of this data to round up to
   * the next multiple of 4, and because we don't want valgrind to
   * complain about uninitialized reads, set the last bit of the
   * buffer to zero:
   */
   for (int i = width; i & 3; i++)
            if (dsdy != 0 || dtdx != 0) {
      /* Arbitrary texture lookup:
   */
      } else {
      /* Axis aligned stretch blit, abitrary scaling factors including
   * flipped, minifying and magnifying:
   */
   int isrc_x = util_iround(samp->fsrc_x);
   int isrc_y = util_iround(samp->fsrc_y);
   int isrc_x1 = util_iround(samp->fsrc_x + width * samp->fdsdx);
            /* Look at the maximum and minimum texture coordinates we will be
   * fetching and figure out if we need to use clamping.  There is
   * similar code in u_blit_sw.c which takes a better approach to
   * this which could be substituted later.
   */
   if (isrc_x  <= texture->width  && isrc_x  >= 0 &&
      isrc_y  <= texture->height && isrc_y  >= 0 &&
   isrc_x1 <= texture->width  && isrc_x1 >= 0 &&
   isrc_y1 <= texture->height && isrc_y1 >= 0) {
      } else {
                        }
         static const uint32_t *
   shade_rgb1(struct shader *shader)
   {
      const __m128i rgb1 = _mm_set1_epi32(0xff000000);
   const uint32_t *src0 = shader->src0;
   uint32_t *dst = shader->out0;
   int width = shader->width;
            for (i = 0; i + 3 < width; i += 4) {
      __m128i s = *(const __m128i *)&src0[i];
                  }
         static void
   init_shader(struct shader *shader,
         {
         }
         /* Linear shader which implements the BLIT_RGBA shader with the
   * additional constraints imposed by lp_setup_is_blit().
   */
   static bool
   blit_rgba_blit(const struct lp_rast_state *state,
                  unsigned x, unsigned y,
   unsigned width, unsigned height,
   const float (*a0)[4],
   const float (*dadx)[4],
      {
      const struct lp_jit_resources *resources = &state->jit_resources;
   const struct lp_jit_texture *texture = &resources->textures[0];
   const uint8_t *src;
   unsigned src_stride;
                     /* Require w==1.0:
   */
   if (a0[0][3] != 1.0 ||
      dadx[0][3] != 0.0 ||
   dady[0][3] != 0.0)
         src_x = x + util_iround(a0[1][0]*texture->width - 0.5f);
            src = texture->base;
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
   blit_rgb1_blit(const struct lp_rast_state *state,
                  unsigned x, unsigned y,
   unsigned width, unsigned height,
   const float (*a0)[4],
   const float (*dadx)[4],
      {
      const struct lp_jit_resources *resources = &state->jit_resources;
   const struct lp_jit_texture *texture = &resources->textures[0];
   const uint8_t *src;
   unsigned src_stride;
                     /* Require w==1.0:
   */
   if (a0[0][3] != 1.0 ||
      dadx[0][3] != 0.0 ||
   dady[0][3] != 0.0)
                  src_x = x + util_iround(a0[1][0]*texture->width - 0.5f);
            src = texture->base;
   src_stride = texture->row_stride[0];
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
         /* Linear shader variant implementing the BLIT_RGBA shader without
   * blending.
   */
   static bool
   blit_rgba(const struct lp_rast_state *state,
            unsigned x, unsigned y,
   unsigned width, unsigned height,
   const float (*a0)[4],
   const float (*dadx)[4],
   const float (*dady)[4],
      {
      const struct lp_jit_resources *resources = &state->jit_resources;
   struct nearest_sampler samp;
                     if (!init_nearest_sampler(&samp,
                           &resources->textures[0],
            init_blend(&blend,
                  /* Rasterize the rectangle and run the shader:
   */
   for (y = 0; y < height; y++) {
      blend.src = samp.fetch(&samp);
                  }
         static bool
   blit_rgb1(const struct lp_rast_state *state,
            unsigned x, unsigned y,
   unsigned width, unsigned height,
   const float (*a0)[4],
   const float (*dadx)[4],
   const float (*dady)[4],
      {
      const struct lp_jit_resources *resources = &state->jit_resources;
   struct nearest_sampler samp;
   struct color_blend blend;
                     if (!init_nearest_sampler(&samp,
                           &resources->textures[0],
                              /* Rasterize the rectangle and run the shader:
   */
   for (y = 0; y < height; y++) {
      shader.src0 = samp.fetch(&samp);
   blend.src = shade_rgb1(&shader);
                  }
         /* Linear shader variant implementing the BLIT_RGBA shader with
   * one/inv_src_alpha blending.
   */
   static bool
   blit_rgba_blend_premul(const struct lp_rast_state *state,
                        unsigned x, unsigned y,
   unsigned width, unsigned height,
   const float (*a0)[4],
      {
      const struct lp_jit_resources *resources = &state->jit_resources;
   struct nearest_sampler samp;
                     if (!init_nearest_sampler(&samp,
                           &resources->textures[0],
                     /* Rasterize the rectangle and run the shader:
   */
   for (y = 0; y < height; y++) {
      blend.src = samp.fetch(&samp);
                  }
         /* Linear shader which always emits red.  Used for debugging.
   */
   static bool
   linear_red(const struct lp_rast_state *state,
            unsigned x, unsigned y,
   unsigned width, unsigned height,
   const float (*a0)[4],
   const float (*dadx)[4],
   const float (*dady)[4],
      {
               util_pack_color_ub(0xff, 0, 0, 0xff,
            util_fill_rect(color,
                  PIPE_FORMAT_B8G8R8A8_UNORM,
   stride,
   x,
   y,
            }
         /* Noop linear shader variant, for debugging.
   */
   static bool
   linear_no_op(const struct lp_rast_state *state,
               unsigned x, unsigned y,
   unsigned width, unsigned height,
   const float (*a0)[4],
   const float (*dadx)[4],
   const float (*dady)[4],
   {
         }
         /* Check for ADD/ONE/INV_SRC_ALPHA, ie premultiplied-alpha blending.
   */
   static bool
   is_one_inv_src_alpha_blend(const struct lp_fragment_shader_variant *variant)
   {
      return
      !variant->key.blend.logicop_enable &&
   variant->key.blend.rt[0].blend_enable &&
   variant->key.blend.rt[0].rgb_func == PIPE_BLEND_ADD &&
   variant->key.blend.rt[0].rgb_src_factor == PIPE_BLENDFACTOR_ONE &&
   variant->key.blend.rt[0].rgb_dst_factor == PIPE_BLENDFACTOR_INV_SRC_ALPHA &&
   variant->key.blend.rt[0].alpha_func == PIPE_BLEND_ADD &&
   variant->key.blend.rt[0].alpha_src_factor == PIPE_BLENDFACTOR_ONE &&
   variant->key.blend.rt[0].alpha_dst_factor == PIPE_BLENDFACTOR_INV_SRC_ALPHA &&
   }
         /* Examine the fragment shader variant and determine whether we can
   * substitute a fastpath linear shader implementation.
   */
   void
   llvmpipe_fs_variant_linear_fastpath(struct lp_fragment_shader_variant *variant)
   {
      if (LP_PERF & PERF_NO_SHADE) {
      variant->jit_linear = linear_red;
               struct lp_sampler_static_state *samp0 =
         if (!samp0)
            enum pipe_format tex_format = samp0->texture_state.format;
   if (variant->shader->kind == LP_FS_KIND_BLIT_RGBA &&
      tex_format == PIPE_FORMAT_B8G8R8A8_UNORM &&
   is_nearest_clamp_sampler(samp0)) {
   if (variant->opaque) {
      variant->jit_linear_blit = blit_rgba_blit;
      } else if (is_one_inv_src_alpha_blend(variant) &&
               }
               if (variant->shader->kind == LP_FS_KIND_BLIT_RGB1 &&
      variant->opaque &&
   (tex_format == PIPE_FORMAT_B8G8R8A8_UNORM ||
   tex_format == PIPE_FORMAT_B8G8R8X8_UNORM) &&
   is_nearest_clamp_sampler(samp0)) {
   variant->jit_linear_blit = blit_rgb1_blit;
   variant->jit_linear = blit_rgb1;
               if (0) {
      variant->jit_linear = linear_no_op;
         }
   #else
   void
   llvmpipe_fs_variant_linear_fastpath(struct lp_fragment_shader_variant *variant)
   {
         }
   #endif
   