   /*
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "fd4_format.h"
   #include "fd4_texture.h"
      static enum a4xx_tex_clamp
   tex_clamp(unsigned wrap, bool *needs_border)
   {
      switch (wrap) {
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      *needs_border = true;
      case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      /* only works for PoT.. need to emulate otherwise! */
      case PIPE_TEX_WRAP_MIRROR_REPEAT:
         case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      /* these two we could perhaps emulate, but we currently
   * just don't advertise PIPE_CAP_TEXTURE_MIRROR_CLAMP
      default:
      DBG("invalid wrap: %u", wrap);
         }
      static enum a4xx_tex_filter
   tex_filter(unsigned filter, bool aniso)
   {
      switch (filter) {
   case PIPE_TEX_FILTER_NEAREST:
         case PIPE_TEX_FILTER_LINEAR:
         default:
      DBG("invalid filter: %u", filter);
         }
      static void *
   fd4_sampler_state_create(struct pipe_context *pctx,
         {
      struct fd4_sampler_stateobj *so = CALLOC_STRUCT(fd4_sampler_stateobj);
   unsigned aniso = util_last_bit(MIN2(cso->max_anisotropy >> 1, 8));
            if (!so)
            if (cso->min_mip_filter == PIPE_TEX_MIPFILTER_LINEAR)
                     so->needs_border = false;
   so->texsamp0 =
      COND(miplinear, A4XX_TEX_SAMP_0_MIPFILTER_LINEAR_NEAR) |
   A4XX_TEX_SAMP_0_XY_MAG(tex_filter(cso->mag_img_filter, aniso)) |
   A4XX_TEX_SAMP_0_XY_MIN(tex_filter(cso->min_img_filter, aniso)) |
   A4XX_TEX_SAMP_0_ANISO(aniso) |
   A4XX_TEX_SAMP_0_LOD_BIAS(cso->lod_bias) |
   A4XX_TEX_SAMP_0_WRAP_S(tex_clamp(cso->wrap_s, &so->needs_border)) |
   A4XX_TEX_SAMP_0_WRAP_T(tex_clamp(cso->wrap_t, &so->needs_border)) |
         so->texsamp1 =
      //		COND(miplinear, A4XX_TEX_SAMP_1_MIPFILTER_LINEAR_FAR) |
   COND(!cso->seamless_cube_map, A4XX_TEX_SAMP_1_CUBEMAPSEAMLESSFILTOFF) |
         if (cso->min_mip_filter != PIPE_TEX_MIPFILTER_NONE) {
      so->texsamp1 |= A4XX_TEX_SAMP_1_MIN_LOD(cso->min_lod) |
      } else {
      /* If we're not doing mipmap filtering, we still need a slightly > 0
   * LOD clamp so the HW can decide between min and mag filtering of
   * level 0.
   */
   so->texsamp1 |= A4XX_TEX_SAMP_1_MIN_LOD(MIN2(cso->min_lod, 0.125f)) |
               if (cso->compare_mode)
      so->texsamp1 |=
            }
      static bool
   use_astc_srgb_workaround(struct pipe_context *pctx, enum pipe_format format)
   {
      return (fd_screen(pctx->screen)->gpu_id == 420) &&
      }
      static struct pipe_sampler_view *
   fd4_sampler_view_create(struct pipe_context *pctx, struct pipe_resource *prsc,
         {
      struct fd4_pipe_sampler_view *so = CALLOC_STRUCT(fd4_pipe_sampler_view);
   struct fd_resource *rsc = fd_resource(prsc);
   enum pipe_format format = cso->format;
            if (!so)
            if (format == PIPE_FORMAT_X32_S8X24_UINT) {
      rsc = rsc->stencil;
               so->base = *cso;
   pipe_reference(NULL, &prsc->reference);
   so->base.texture = prsc;
   so->base.reference.count = 1;
            so->swizzle = fd4_tex_swiz(format, cso->swizzle_r, cso->swizzle_g,
            so->texconst0 = A4XX_TEX_CONST_0_TYPE(fd4_tex_type(cso->target)) |
                  if (util_format_is_srgb(format)) {
      if (use_astc_srgb_workaround(pctx, format))
                     if (cso->target == PIPE_BUFFER) {
               lvl = 0;
   so->texconst1 =
      A4XX_TEX_CONST_1_WIDTH(elements & MASK(15)) |
      so->texconst2 = A4XX_TEX_CONST_2_BUFFER;
      } else {
               lvl = fd_sampler_first_level(cso);
   miplevels = fd_sampler_last_level(cso) - lvl;
            so->texconst0 |= A4XX_TEX_CONST_0_MIPLVLS(miplevels);
   so->texconst1 = A4XX_TEX_CONST_1_WIDTH(u_minify(prsc->width0, lvl)) |
         so->texconst2 = A4XX_TEX_CONST_2_PITCHALIGN(rsc->layout.pitchalign - 5) |
                     /* NOTE: since we sample z24s8 using 8888_UINT format, the swizzle
   * we get isn't quite right.  Use SWAP(XYZW) as a cheap and cheerful
   * way to re-arrange things so stencil component is where the swiz
   * expects.
   *
   * Note that gallium expects stencil sampler to return (s,s,s,s)
   * which isn't quite true.  To make that happen we'd have to massage
   * the swizzle.  But in practice only the .x component is used.
   */
   if (format == PIPE_FORMAT_X24S8_UINT)
            switch (cso->target) {
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
      so->texconst3 = A4XX_TEX_CONST_3_DEPTH(layers) |
            case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      so->texconst3 = A4XX_TEX_CONST_3_DEPTH(layers / 6) |
            case PIPE_TEXTURE_3D:
      so->texconst3 =
      A4XX_TEX_CONST_3_DEPTH(u_minify(prsc->depth0, lvl)) |
      so->texconst4 = A4XX_TEX_CONST_4_LAYERSZ(
            default:
      so->texconst3 = 0x00000000;
                  }
      static void
   fd4_set_sampler_views(struct pipe_context *pctx, enum pipe_shader_type shader,
                           {
      struct fd_context *ctx = fd_context(pctx);
   struct fd4_context *fd4_ctx = fd4_context(ctx);
   uint16_t astc_srgb = 0;
   uint16_t *sampler_swizzles;
            if (shader == PIPE_SHADER_FRAGMENT) {
         } else if (shader == PIPE_SHADER_VERTEX) {
         } else if (shader == PIPE_SHADER_COMPUTE) {
         } else {
      assert(0);
               for (i = 0; i < nr; i++) {
      if (views[i]) {
      struct fd4_pipe_sampler_view *view = fd4_pipe_sampler_view(views[i]);
   if (view->astc_srgb)
                  const struct util_format_description *desc =
         int c = util_format_get_first_non_void_channel(desc->format);
   if (c >= 0 && desc->channel[c].pure_integer) {
      switch (desc->channel[c].size) {
   case 8:
      sampler_swizzles[start + i] |= 0x1000;
      case 16:
      sampler_swizzles[start + i] |= 0x2000;
      case 32:
      sampler_swizzles[start + i] |= 0x3000;
      case 10:
      sampler_swizzles[start + i] |= 0x4000;
      default:
                           fd_set_sampler_views(pctx, shader, start, nr, unbind_num_trailing_slots,
            for (i = 0; i < unbind_num_trailing_slots; i++) {
      astc_srgb &= ~(1 << (start + nr + i));
               if (shader == PIPE_SHADER_FRAGMENT) {
         } else if (shader == PIPE_SHADER_VERTEX) {
         } else if (shader == PIPE_SHADER_COMPUTE) {
            }
      void
   fd4_texture_init(struct pipe_context *pctx)
   {
      pctx->create_sampler_state = fd4_sampler_state_create;
   pctx->bind_sampler_states = fd_sampler_states_bind;
   pctx->create_sampler_view = fd4_sampler_view_create;
      }
