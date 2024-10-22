   /*
   * Copyright (c) 2012-2015 Etnaviv Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Wladimir J. van der Laan <laanwj@gmail.com>
   */
      #include "etnaviv_blend.h"
      #include "etnaviv_context.h"
   #include "etnaviv_screen.h"
   #include "etnaviv_translate.h"
   #include "hw/common.xml.h"
   #include "pipe/p_defines.h"
   #include "util/u_memory.h"
   #include "util/half_float.h"
      void *
   etna_blend_state_create(struct pipe_context *pctx,
         {
      struct etna_context *ctx = etna_context(pctx);
   const struct pipe_rt_blend_state *rt0 = &so->rt[0];
   struct etna_blend_state *co = CALLOC_STRUCT(etna_blend_state);
            /* pipe_blend_func happens to match the hardware. */
   STATIC_ASSERT(PIPE_BLEND_ADD == BLEND_EQ_ADD);
   STATIC_ASSERT(PIPE_BLEND_SUBTRACT == BLEND_EQ_SUBTRACT);
   STATIC_ASSERT(PIPE_BLEND_REVERSE_SUBTRACT == BLEND_EQ_REVERSE_SUBTRACT);
   STATIC_ASSERT(PIPE_BLEND_MIN == BLEND_EQ_MIN);
            if (!co)
                     /* Enable blending if
   * - blend enabled in blend state
   * - NOT source factor is ONE and destination factor ZERO and eq is ADD for
   *   both rgb and alpha (which mean that blending is effectively disabled)
   */
   alpha_enable = rt0->blend_enable &&
               !(rt0->rgb_src_factor == PIPE_BLENDFACTOR_ONE &&
      rt0->rgb_dst_factor == PIPE_BLENDFACTOR_ZERO &&
   rt0->rgb_func == PIPE_BLEND_ADD &&
         /* Enable separate alpha if
   * - Blending enabled (see above)
   * - NOT source/destination factor and eq is same for both rgb and alpha
   *   (which would effectively that mean alpha is not separate), and
   */
   bool separate_alpha = alpha_enable &&
                        if (alpha_enable) {
      co->PE_ALPHA_CONFIG =
      VIVS_PE_ALPHA_CONFIG_BLEND_ENABLE_COLOR |
   COND(separate_alpha, VIVS_PE_ALPHA_CONFIG_BLEND_SEPARATE_ALPHA) |
   VIVS_PE_ALPHA_CONFIG_SRC_FUNC_COLOR(translate_blend_factor(rt0->rgb_src_factor)) |
   VIVS_PE_ALPHA_CONFIG_SRC_FUNC_ALPHA(translate_blend_factor(rt0->alpha_src_factor)) |
   VIVS_PE_ALPHA_CONFIG_DST_FUNC_COLOR(translate_blend_factor(rt0->rgb_dst_factor)) |
   VIVS_PE_ALPHA_CONFIG_DST_FUNC_ALPHA(translate_blend_factor(rt0->alpha_dst_factor)) |
   VIVS_PE_ALPHA_CONFIG_EQ_COLOR(rt0->rgb_func) |
   } else {
                  logicop_enable = so->logicop_enable &&
            co->PE_LOGIC_OP =
         VIVS_PE_LOGIC_OP_OP(logicop_enable ? so->logicop_func : LOGIC_OP_COPY) |
                     /* independent_blend_enable not needed: only one rt supported */
   /* XXX alpha_to_coverage / alpha_to_one? */
   /* Set dither registers based on dither status. These registers set the
   * dither pattern,
   * for now, set the same values as the blob.
   */
   if (so->dither &&
      (!alpha_enable ||
   VIV_FEATURE(ctx->screen, chipMinorFeatures3, PE_DITHER_FIX))) {
   co->PE_DITHER[0] = 0x6e4ca280;
      } else {
      co->PE_DITHER[0] = 0xffffffff;
                  }
      bool
   etna_update_blend(struct etna_context *ctx)
   {
      struct pipe_framebuffer_state *pfb = &ctx->framebuffer_s;
   struct pipe_blend_state *pblend = ctx->blend;
   struct etna_blend_state *blend = etna_blend_state(pblend);
   const struct pipe_rt_blend_state *rt0 = &pblend->rt[0];
   const struct util_format_description *desc;
            if (pfb->cbufs[0] &&
      translate_pe_format_rb_swap(pfb->cbufs[0]->format)) {
   colormask = rt0->colormask & (PIPE_MASK_A | PIPE_MASK_G);
   if (rt0->colormask & PIPE_MASK_R)
         if (rt0->colormask & PIPE_MASK_B)
      } else {
                  /* If the complete render target is written, set full_overwrite:
   * - The color mask covers all channels of the render target
   * - No blending or logicop is used
   */
   if (pfb->cbufs[0])
         bool full_overwrite = !pfb->cbufs[0] || ((blend->fo_allowed &&
         blend->PE_COLOR_FORMAT =
                     }
      void
   etna_set_blend_color(struct pipe_context *pctx, const struct pipe_blend_color *bc)
   {
      struct etna_context *ctx = etna_context(pctx);
                        }
      bool
   etna_update_blend_color(struct etna_context *ctx)
   {
      struct pipe_framebuffer_state *pfb = &ctx->framebuffer_s;
   struct compiled_blend_color *cs = &ctx->blend_color;
            cs->PE_ALPHA_BLEND_COLOR =
      VIVS_PE_ALPHA_BLEND_COLOR_R(float_to_ubyte(cs->color[rb_swap ? 2 : 0])) |
   VIVS_PE_ALPHA_BLEND_COLOR_G(float_to_ubyte(cs->color[1])) |
   VIVS_PE_ALPHA_BLEND_COLOR_B(float_to_ubyte(cs->color[rb_swap ? 0 : 2])) |
         cs->PE_ALPHA_COLOR_EXT0 =
      VIVS_PE_ALPHA_COLOR_EXT0_B(_mesa_float_to_half(cs->color[rb_swap ? 2 : 0])) |
      cs->PE_ALPHA_COLOR_EXT1 =
      VIVS_PE_ALPHA_COLOR_EXT1_R(_mesa_float_to_half(cs->color[rb_swap ? 0 : 2])) |
            }
