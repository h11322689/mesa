   /*
   * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
   * Copyright 2009 Marek Olšák <maraeo@gmail.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      #include "draw/draw_context.h"
      #include "util/u_framebuffer.h"
   #include "util/half_float.h"
   #include "util/u_helpers.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_pack_color.h"
   #include "util/u_transfer.h"
   #include "util/u_blend.h"
      #include "tgsi/tgsi_parse.h"
      #include "util/detect.h"
      #include "r300_cb.h"
   #include "r300_context.h"
   #include "r300_emit.h"
   #include "r300_reg.h"
   #include "r300_screen.h"
   #include "r300_screen_buffer.h"
   #include "r300_state_inlines.h"
   #include "r300_fs.h"
   #include "r300_texture.h"
   #include "r300_vs.h"
   #include "compiler/r300_nir.h"
   #include "compiler/nir_to_rc.h"
      /* r300_state: Functions used to initialize state context by translating
   * Gallium state objects into semi-native r300 state objects. */
      #define UPDATE_STATE(cso, atom) \
      if (cso != atom.state) { \
      atom.state = cso;    \
            static bool blend_discard_if_src_alpha_0(unsigned srcRGB, unsigned srcA,
         {
      /* If the blend equation is ADD or REVERSE_SUBTRACT,
   * SRC_ALPHA == 0, and the following state is set, the colorbuffer
   * will not be changed.
   * Notice that the dst factors are the src factors inverted. */
   return (srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
            srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
      (srcA == PIPE_BLENDFACTOR_SRC_COLOR ||
      srcA == PIPE_BLENDFACTOR_SRC_ALPHA ||
   srcA == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
      (dstRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
         (dstA == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
      }
      static bool blend_discard_if_src_alpha_1(unsigned srcRGB, unsigned srcA,
         {
      /* If the blend equation is ADD or REVERSE_SUBTRACT,
   * SRC_ALPHA == 1, and the following state is set, the colorbuffer
   * will not be changed.
   * Notice that the dst factors are the src factors inverted. */
   return (srcRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
               (srcA == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
      srcA == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
      (dstRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
         (dstA == PIPE_BLENDFACTOR_SRC_COLOR ||
      }
      static bool blend_discard_if_src_color_0(unsigned srcRGB, unsigned srcA,
         {
      /* If the blend equation is ADD or REVERSE_SUBTRACT,
   * SRC_COLOR == (0,0,0), and the following state is set, the colorbuffer
   * will not be changed.
   * Notice that the dst factors are the src factors inverted. */
   return (srcRGB == PIPE_BLENDFACTOR_SRC_COLOR ||
               (srcA == PIPE_BLENDFACTOR_ZERO) &&
   (dstRGB == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
      }
      static bool blend_discard_if_src_color_1(unsigned srcRGB, unsigned srcA,
         {
      /* If the blend equation is ADD or REVERSE_SUBTRACT,
   * SRC_COLOR == (1,1,1), and the following state is set, the colorbuffer
   * will not be changed.
   * Notice that the dst factors are the src factors inverted. */
   return (srcRGB == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
               (srcA == PIPE_BLENDFACTOR_ZERO) &&
   (dstRGB == PIPE_BLENDFACTOR_SRC_COLOR ||
      }
      static bool blend_discard_if_src_alpha_color_0(unsigned srcRGB, unsigned srcA,
         {
      /* If the blend equation is ADD or REVERSE_SUBTRACT,
   * SRC_ALPHA_COLOR == (0,0,0,0), and the following state is set,
   * the colorbuffer will not be changed.
   * Notice that the dst factors are the src factors inverted. */
   return (srcRGB == PIPE_BLENDFACTOR_SRC_COLOR ||
            srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
   srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
      (srcA == PIPE_BLENDFACTOR_SRC_COLOR ||
      srcA == PIPE_BLENDFACTOR_SRC_ALPHA ||
   srcA == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
      (dstRGB == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
      dstRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
      (dstA == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
      }
      static bool blend_discard_if_src_alpha_color_1(unsigned srcRGB, unsigned srcA,
         {
      /* If the blend equation is ADD or REVERSE_SUBTRACT,
   * SRC_ALPHA_COLOR == (1,1,1,1), and the following state is set,
   * the colorbuffer will not be changed.
   * Notice that the dst factors are the src factors inverted. */
   return (srcRGB == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
            srcRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
      (srcA == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
      srcA == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
      (dstRGB == PIPE_BLENDFACTOR_SRC_COLOR ||
      dstRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
      (dstA == PIPE_BLENDFACTOR_SRC_COLOR ||
      }
      static unsigned blend_discard_conditionally(unsigned eqRGB, unsigned eqA,
               {
               /* Optimization: discard pixels which don't change the colorbuffer.
   *
   * The code below is non-trivial and some math is involved.
   *
   * Discarding pixels must be disabled when FP16 AA is enabled.
   * This is a hardware bug. Also, this implementation wouldn't work
   * with FP blending enabled and equation clamping disabled.
   *
   * Equations other than ADD are rarely used and therefore won't be
   * optimized. */
   if ((eqRGB == PIPE_BLEND_ADD || eqRGB == PIPE_BLEND_REVERSE_SUBTRACT) &&
      (eqA == PIPE_BLEND_ADD || eqA == PIPE_BLEND_REVERSE_SUBTRACT)) {
   /* ADD: X+Y
      * REVERSE_SUBTRACT: Y-X
   *
   * The idea is:
   * If X = src*srcFactor = 0 and Y = dst*dstFactor = 1,
   * then CB will not be changed.
   *
   * Given the srcFactor and dstFactor variables, we can derive
   * what src and dst should be equal to and discard appropriate
   * pixels.
      if (blend_discard_if_src_alpha_0(srcRGB, srcA, dstRGB, dstA)) {
         } else if (blend_discard_if_src_alpha_1(srcRGB, srcA,
               } else if (blend_discard_if_src_color_0(srcRGB, srcA,
               } else if (blend_discard_if_src_color_1(srcRGB, srcA,
               } else if (blend_discard_if_src_alpha_color_0(srcRGB, srcA,
               blend_control |=
   } else if (blend_discard_if_src_alpha_color_1(srcRGB, srcA,
               blend_control |=
      }
      }
      /* The hardware colormask is clunky a must be swizzled depending on the format.
   * This was figured out by trial-and-error. */
   static unsigned bgra_cmask(unsigned mask)
   {
      return ((mask & PIPE_MASK_R) << 2) |
            }
      static unsigned rgba_cmask(unsigned mask)
   {
         }
      static unsigned rrrr_cmask(unsigned mask)
   {
      return (mask & PIPE_MASK_R) |
         ((mask & PIPE_MASK_R) << 1) |
      }
      static unsigned aaaa_cmask(unsigned mask)
   {
      return ((mask & PIPE_MASK_A) >> 3) |
         ((mask & PIPE_MASK_A) >> 2) |
      }
      static unsigned grrg_cmask(unsigned mask)
   {
      return ((mask & PIPE_MASK_R) << 1) |
         ((mask & PIPE_MASK_R) << 2) |
      }
      static unsigned arra_cmask(unsigned mask)
   {
      return ((mask & PIPE_MASK_R) << 1) |
         ((mask & PIPE_MASK_R) << 2) |
      }
      static unsigned blend_read_enable(unsigned eqRGB, unsigned eqA,
                     {
               /* Optimization: some operations do not require the destination color.
   *
   * When SRC_ALPHA_SATURATE is used, colorbuffer reads must be enabled,
   * otherwise blending gives incorrect results. It seems to be
   * a hardware bug. */
   if (eqRGB == PIPE_BLEND_MIN || eqA == PIPE_BLEND_MIN ||
      eqRGB == PIPE_BLEND_MAX || eqA == PIPE_BLEND_MAX ||
   dstRGB != PIPE_BLENDFACTOR_ZERO ||
   dstA != PIPE_BLENDFACTOR_ZERO ||
   util_blend_factor_uses_dest(srcRGB, false) ||
   util_blend_factor_uses_dest(srcA, true)) {
   /* Enable reading from the colorbuffer. */
            if (src_alpha_optz) {
         /* Optimization: Depending on incoming pixels, we can
   * conditionally disable the reading in hardware... */
   if (eqRGB != PIPE_BLEND_MIN && eqA != PIPE_BLEND_MIN &&
      eqRGB != PIPE_BLEND_MAX && eqA != PIPE_BLEND_MAX) {
   /* Disable reading if SRC_ALPHA == 0. */
   if ((dstRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
            (dstA == PIPE_BLENDFACTOR_SRC_COLOR ||
      dstA == PIPE_BLENDFACTOR_SRC_ALPHA ||
      (srcRGB != PIPE_BLENDFACTOR_DST_COLOR &&
      srcRGB != PIPE_BLENDFACTOR_DST_ALPHA &&
                        /* Disable reading if SRC_ALPHA == 1. */
   if ((dstRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
            (dstA == PIPE_BLENDFACTOR_INV_SRC_COLOR ||
      dstA == PIPE_BLENDFACTOR_INV_SRC_ALPHA ||
      (srcRGB != PIPE_BLENDFACTOR_DST_COLOR &&
      srcRGB != PIPE_BLENDFACTOR_DST_ALPHA &&
   srcRGB != PIPE_BLENDFACTOR_INV_DST_COLOR &&
   srcRGB != PIPE_BLENDFACTOR_INV_DST_ALPHA)) {
         }
      }
      /* Create a new blend state based on the CSO blend state.
   *
   * This encompasses alpha blending, logic/raster ops, and blend dithering. */
   static void* r300_create_blend_state(struct pipe_context* pipe,
         {
      struct r300_screen* r300screen = r300_screen(pipe->screen);
   struct r300_blend_state* blend = CALLOC_STRUCT(r300_blend_state);
   uint32_t blend_control = 0;       /* R300_RB3D_CBLEND: 0x4e04 */
   uint32_t blend_control_noclamp = 0;    /* R300_RB3D_CBLEND: 0x4e04 */
   uint32_t blend_control_noalpha = 0;    /* R300_RB3D_CBLEND: 0x4e04 */
   uint32_t blend_control_noalpha_noclamp = 0;    /* R300_RB3D_CBLEND: 0x4e04 */
   uint32_t alpha_blend_control = 0; /* R300_RB3D_ABLEND: 0x4e08 */
   uint32_t alpha_blend_control_noclamp = 0; /* R300_RB3D_ABLEND: 0x4e08 */
   uint32_t alpha_blend_control_noalpha = 0; /* R300_RB3D_ABLEND: 0x4e08 */
   uint32_t alpha_blend_control_noalpha_noclamp = 0; /* R300_RB3D_ABLEND: 0x4e08 */
   uint32_t rop = 0;                 /* R300_RB3D_ROPCNTL: 0x4e18 */
   uint32_t dither = 0;              /* R300_RB3D_DITHER_CTL: 0x4e50 */
            const unsigned eqRGB = state->rt[0].rgb_func;
   const unsigned srcRGB = state->rt[0].rgb_src_factor;
            const unsigned eqA = state->rt[0].alpha_func;
   const unsigned srcA = state->rt[0].alpha_src_factor;
            unsigned srcRGBX = srcRGB;
   unsigned dstRGBX = dstRGB;
                     /* force DST_ALPHA to ONE where we can */
   switch (srcRGBX) {
   case PIPE_BLENDFACTOR_DST_ALPHA:
      srcRGBX = PIPE_BLENDFACTOR_ONE;
      case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      srcRGBX = PIPE_BLENDFACTOR_ZERO;
               switch (dstRGBX) {
   case PIPE_BLENDFACTOR_DST_ALPHA:
      dstRGBX = PIPE_BLENDFACTOR_ONE;
      case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      dstRGBX = PIPE_BLENDFACTOR_ZERO;
               /* Get blending register values. */
   if (state->rt[0].blend_enable) {
               /* despite the name, ALPHA_BLEND_ENABLE has nothing to do with alpha,
         blend_control = blend_control_noclamp =
         R300_ALPHA_BLEND_ENABLE |
            blend_control_noalpha = blend_control_noalpha_noclamp =
         R300_ALPHA_BLEND_ENABLE |
            blend_eq = r300_translate_blend_function(eqRGB, true);
            blend_control |= blend_eq;
   blend_control_noalpha |= blend_eq;
   blend_control_noclamp |= blend_eq_noclamp;
            /* Optimization: some operations do not require the destination color. */
   blend_control |= blend_read_enable(eqRGB, eqA, dstRGB, dstA,
         blend_control_noclamp |= blend_read_enable(eqRGB, eqA, dstRGB, dstA,
         blend_control_noalpha |= blend_read_enable(eqRGB, eqA, dstRGBX, dstA,
         blend_control_noalpha_noclamp |= blend_read_enable(eqRGB, eqA, dstRGBX, dstA,
            /* Optimization: discard pixels which don't change the colorbuffer.
         blend_control |= blend_discard_conditionally(eqRGB, eqA, dstRGB, dstA,
         blend_control_noalpha |= blend_discard_conditionally(eqRGB, eqA, dstRGBX, dstA,
            /* separate alpha */
   if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB) {
                        alpha_blend_control = alpha_blend_control_noclamp =
      (r300_translate_blend_factor(srcA) << R300_SRC_BLEND_SHIFT) |
      alpha_blend_control |= r300_translate_blend_function(eqA, true);
   }
   if (srcA != srcRGBX || dstA != dstRGBX || eqA != eqRGB) {
                        alpha_blend_control_noalpha = alpha_blend_control_noalpha_noclamp =
      (r300_translate_blend_factor(srcA) << R300_SRC_BLEND_SHIFT) |
      alpha_blend_control_noalpha |= r300_translate_blend_function(eqA, true);
               /* PIPE_LOGICOP_* don't need to be translated, fortunately. */
   if (state->logicop_enable) {
      rop = R300_RB3D_ROPCNTL_ROP_ENABLE |
               /* Neither fglrx nor classic r300 ever set this, regardless of dithering
   * state. Since it's an optional implementation detail, we can leave it
   * out and never dither.
   *
   * This could be revisited if we ever get quality or conformance hints.
   *
   if (state->dither) {
      dither = R300_RB3D_DITHER_CTL_DITHER_MODE_LUT |
      }
            /* Build a command buffer. */
   {
      unsigned (*func[COLORMASK_NUM_SWIZZLES])(unsigned) = {
         bgra_cmask,
   rgba_cmask,
   rrrr_cmask,
   aaaa_cmask,
   grrg_cmask,
   arra_cmask,
   bgra_cmask,
            for (i = 0; i < COLORMASK_NUM_SWIZZLES; i++) {
                  BEGIN_CB(blend->cb_clamp[i], 8);
   OUT_CB_REG(R300_RB3D_ROPCNTL, rop);
   OUT_CB_REG_SEQ(R300_RB3D_CBLEND, 3);
   OUT_CB(has_alpha ? blend_control : blend_control_noalpha);
   OUT_CB(has_alpha ? alpha_blend_control : alpha_blend_control_noalpha);
   OUT_CB(func[i](state->rt[0].colormask));
   OUT_CB_REG(R300_RB3D_DITHER_CTL, dither);
               /* Build a command buffer (for RGBA16F). */
   BEGIN_CB(blend->cb_noclamp, 8);
   OUT_CB_REG(R300_RB3D_ROPCNTL, rop);
   OUT_CB_REG_SEQ(R300_RB3D_CBLEND, 3);
   OUT_CB(blend_control_noclamp);
   OUT_CB(alpha_blend_control_noclamp);
   OUT_CB(rgba_cmask(state->rt[0].colormask));
   OUT_CB_REG(R300_RB3D_DITHER_CTL, dither);
            /* Build a command buffer (for RGB16F). */
   BEGIN_CB(blend->cb_noclamp_noalpha, 8);
   OUT_CB_REG(R300_RB3D_ROPCNTL, rop);
   OUT_CB_REG_SEQ(R300_RB3D_CBLEND, 3);
   OUT_CB(blend_control_noalpha_noclamp);
   OUT_CB(alpha_blend_control_noalpha_noclamp);
   OUT_CB(rgba_cmask(state->rt[0].colormask));
   OUT_CB_REG(R300_RB3D_DITHER_CTL, dither);
            /* The same as above, but with no colorbuffer reads and writes. */
   BEGIN_CB(blend->cb_no_readwrite, 8);
   OUT_CB_REG(R300_RB3D_ROPCNTL, rop);
   OUT_CB_REG_SEQ(R300_RB3D_CBLEND, 3);
   OUT_CB(0);
   OUT_CB(0);
   OUT_CB(0);
   OUT_CB_REG(R300_RB3D_DITHER_CTL, dither);
               }
      /* Bind blend state. */
   static void r300_bind_blend_state(struct pipe_context* pipe,
         {
      struct r300_context* r300 = r300_context(pipe);
   struct r300_blend_state *blend  = (struct r300_blend_state*)state;
   bool last_alpha_to_one = r300->alpha_to_one;
                     if (!blend)
            r300->alpha_to_one = blend->state.alpha_to_one;
            if (r300->alpha_to_one != last_alpha_to_one && r300->msaa_enable &&
      r300->fs_status == FRAGMENT_SHADER_VALID) {
               if (r300->alpha_to_coverage != last_alpha_to_coverage &&
      r300->msaa_enable) {
         }
      /* Free blend state. */
   static void r300_delete_blend_state(struct pipe_context* pipe,
         {
         }
      /* Convert float to 10bit integer */
   static unsigned float_to_fixed10(float f)
   {
         }
      /* Set blend color.
   * Setup both R300 and R500 registers, figure out later which one to write. */
   static void r300_set_blend_color(struct pipe_context* pipe,
         {
      struct r300_context* r300 = r300_context(pipe);
   struct pipe_framebuffer_state *fb = r300->fb_state.state;
   struct r300_blend_color_state *state =
         struct pipe_blend_color c;
   struct pipe_surface *cb;
   float tmp;
            state->state = *color; /* Save it, so that we can reuse it in set_fb_state */
   c = *color;
            /* The blend color is dependent on the colorbuffer format. */
   if (cb) {
      switch (cb->format) {
   case PIPE_FORMAT_R8_UNORM:
   case PIPE_FORMAT_L8_UNORM:
   case PIPE_FORMAT_I8_UNORM:
                  case PIPE_FORMAT_A8_UNORM:
                  case PIPE_FORMAT_R8G8_UNORM:
                  case PIPE_FORMAT_L8A8_UNORM:
   case PIPE_FORMAT_R8A8_UNORM:
                  case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_R8G8B8X8_UNORM:
         tmp = c.color[0];
   c.color[0] = c.color[2];
            default:;
               if (r300->screen->caps.is_r500) {
      BEGIN_CB(state->cb, 3);
            switch (cb ? cb->format : 0) {
   case PIPE_FORMAT_R16G16B16A16_FLOAT:
   case PIPE_FORMAT_R16G16B16X16_FLOAT:
         OUT_CB(_mesa_float_to_half(c.color[2]) |
         OUT_CB(_mesa_float_to_half(c.color[0]) |
            default:
         OUT_CB(float_to_fixed10(c.color[0]) |
         OUT_CB(float_to_fixed10(c.color[2]) |
               } else {
      union util_color uc;
            BEGIN_CB(state->cb, 2);
   OUT_CB_REG(R300_RB3D_BLEND_COLOR, uc.ui[0]);
                  }
      static void r300_set_clip_state(struct pipe_context* pipe,
         {
      struct r300_context* r300 = r300_context(pipe);
   struct r300_clip_state *clip =
                  if (r300->screen->caps.has_tcl) {
      BEGIN_CB(clip->cb, r300->clip_state.size);
   OUT_CB_REG(R300_VAP_PVS_VECTOR_INDX_REG,
               OUT_CB_ONE_REG(R300_VAP_PVS_UPLOAD_DATA, 6 * 4);
   OUT_CB_TABLE(state->ucp, 6 * 4);
               } else {
            }
      /* Create a new depth, stencil, and alpha state based on the CSO dsa state.
   *
   * This contains the depth buffer, stencil buffer, alpha test, and such.
   * On the Radeon, depth and stencil buffer setup are intertwined, which is
   * the reason for some of the strange-looking assignments across registers. */
   static void* r300_create_dsa_state(struct pipe_context* pipe,
         {
      bool is_r500 = r300_screen(pipe->screen)->caps.is_r500;
   struct r300_dsa_state* dsa = CALLOC_STRUCT(r300_dsa_state);
   CB_LOCALS;
   uint32_t alpha_value_fp16 = 0;
   uint32_t z_buffer_control = 0;
   uint32_t z_stencil_control = 0;
   uint32_t stencil_ref_mask = 0;
                     /* Depth test setup. - separate write mask depth for decomp flush */
   if (state->depth_writemask) {
                  if (state->depth_enabled) {
               z_stencil_control |=
                     /* Stencil buffer setup. */
   if (state->stencil[0].enabled) {
      z_buffer_control |= R300_STENCIL_ENABLE;
   z_stencil_control |=
         (r300_translate_depth_stencil_function(state->stencil[0].func) <<
         (r300_translate_stencil_op(state->stencil[0].fail_op) <<
         (r300_translate_stencil_op(state->stencil[0].zpass_op) <<
                  stencil_ref_mask =
                  if (state->stencil[1].enabled) {
                  z_buffer_control |= R300_STENCIL_FRONT_BACK;
   z_stencil_control |=
   (r300_translate_depth_stencil_function(state->stencil[1].func) <<
         (r300_translate_stencil_op(state->stencil[1].fail_op) <<
         (r300_translate_stencil_op(state->stencil[1].zpass_op) <<
                        stencil_ref_bf =
                  if (is_r500) {
         } else {
      dsa->two_sided_stencil_ref =
      (state->stencil[0].valuemask != state->stencil[1].valuemask ||
               /* Alpha test setup. */
   if (state->alpha_enabled) {
      dsa->alpha_function =
                  dsa->alpha_function |= float_to_ubyte(state->alpha_ref_value);
               BEGIN_CB(&dsa->cb_begin, 8);
   OUT_CB_REG_SEQ(R300_ZB_CNTL, 3);
   OUT_CB(z_buffer_control);
   OUT_CB(z_stencil_control);
   OUT_CB(stencil_ref_mask);
   OUT_CB_REG(R500_ZB_STENCILREFMASK_BF, stencil_ref_bf);
   OUT_CB_REG(R500_FG_ALPHA_VALUE, alpha_value_fp16);
            BEGIN_CB(dsa->cb_zb_no_readwrite, 8);
   OUT_CB_REG_SEQ(R300_ZB_CNTL, 3);
   OUT_CB(0);
   OUT_CB(0);
   OUT_CB(0);
   OUT_CB_REG(R500_ZB_STENCILREFMASK_BF, 0);
   OUT_CB_REG(R500_FG_ALPHA_VALUE, alpha_value_fp16);
               }
      static void r300_dsa_inject_stencilref(struct r300_context *r300)
   {
      struct r300_dsa_state *dsa =
            if (!dsa)
            dsa->stencil_ref_mask =
      (dsa->stencil_ref_mask & ~R300_STENCILREF_MASK) |
      dsa->stencil_ref_bf =
      (dsa->stencil_ref_bf & ~R300_STENCILREF_MASK) |
   }
      /* Bind DSA state. */
   static void r300_bind_dsa_state(struct pipe_context* pipe,
         {
               if (!state) {
                           r300_mark_atom_dirty(r300, &r300->hyperz_state); /* Will be updated before the emission. */
      }
      /* Free DSA state. */
   static void r300_delete_dsa_state(struct pipe_context* pipe,
         {
         }
      static void r300_set_stencil_ref(struct pipe_context* pipe,
         {
                        r300_dsa_inject_stencilref(r300);
      }
      static void r300_print_fb_surf_info(struct pipe_surface *surf, unsigned index,
         {
      struct pipe_resource *tex = surf->texture;
            fprintf(stderr,
                                          binding, index, surf->width, surf->height,
                  rtex->tex.macrotile[0] ? "YES" : " NO",
   rtex->tex.microtile ? "YES" : " NO",
   }
      void r300_mark_fb_state_dirty(struct r300_context *r300,
         {
               r300_mark_atom_dirty(r300, &r300->gpu_flush);
            /* What is marked as dirty depends on the enum r300_fb_state_change. */
   if (change == R300_CHANGED_FB_STATE) {
      r300_mark_atom_dirty(r300, &r300->aa_state);
   r300_mark_atom_dirty(r300, &r300->dsa_state); /* for AlphaRef */
               if (change == R300_CHANGED_FB_STATE ||
      change == R300_CHANGED_HYPERZ_FLAG) {
               if (change == R300_CHANGED_FB_STATE ||
      change == R300_CHANGED_MULTIWRITE) {
               /* Now compute the fb_state atom size. */
            if (r300->cbzb_clear)
         else if (state->zsbuf) {
      r300->fb_state.size += 10;
   if (r300->hyperz_enabled)
               if (r300->cmask_in_use) {
      r300->fb_state.size += 6;
   if (r300->screen->caps.is_r500) {
                        }
      static void
   r300_set_framebuffer_state(struct pipe_context* pipe,
         {
      struct r300_context* r300 = r300_context(pipe);
   struct r300_aa_state *aa = (struct r300_aa_state*)r300->aa_state.state;
   struct pipe_framebuffer_state *current_state = r300->fb_state.state;
   unsigned max_width, max_height, i;
   uint32_t zbuffer_bpp = 0;
            if (r300->screen->caps.is_r500) {
         } else if (r300->screen->caps.is_r400) {
         } else {
                  if (state->width > max_width || state->height > max_height) {
      fprintf(stderr, "r300: Implementation error: Render targets are too "
   "big in %s, refusing to bind framebuffer state!\n", __func__);
               if (current_state->zsbuf && r300->zmask_in_use && !r300->locked_zbuffer) {
      /* There is a zmask in use, what are we gonna do? */
   if (state->zsbuf) {
         if (!pipe_surface_equal(current_state->zsbuf, state->zsbuf)) {
      /* Decompress the currently bound zbuffer before we bind another one. */
   r300_decompress_zmask(r300);
      } else {
         /* We don't bind another zbuffer, so lock the current one. */
      } else if (r300->locked_zbuffer) {
      /* We have a locked zbuffer now, what are we gonna do? */
   if (state->zsbuf) {
         if (!pipe_surface_equal(r300->locked_zbuffer, state->zsbuf)) {
      /* We are binding some other zbuffer, so decompress the locked one,
   * it gets unlocked automatically. */
   r300_decompress_zmask_locked_unsafe(r300);
      } else {
      /* We are binding the locked zbuffer again, so unlock it. */
         }
            /* If zsbuf is set from NULL to non-NULL or vice versa.. */
   if (!!current_state->zsbuf != !!state->zsbuf) {
                           /* Remove trailing NULL colorbuffers. */
   while (current_state->nr_cbufs && !current_state->cbufs[current_state->nr_cbufs-1])
            /* Set whether CMASK can be used. */
   r300->cmask_in_use =
      state->nr_cbufs == 1 && state->cbufs[0] &&
         /* Need to reset clamping or colormask. */
            /* Re-swizzle the blend color. */
            if (unlock_zbuffer) {
                           if (state->zsbuf) {
      switch (util_format_get_blocksize(state->zsbuf->format)) {
   case 2:
         zbuffer_bpp = 16;
   case 4:
         zbuffer_bpp = 24;
            /* Polygon offset depends on the zbuffer bit depth. */
   if (r300->zbuffer_bpp != zbuffer_bpp) {
                  if (r300->polygon_offset_enabled)
                        /* Set up AA config. */
   if (r300->num_samples > 1) {
      switch (r300->num_samples) {
   case 2:
         aa->aa_config = R300_GB_AA_CONFIG_AA_ENABLE |
         case 4:
         aa->aa_config = R300_GB_AA_CONFIG_AA_ENABLE |
         case 6:
         aa->aa_config = R300_GB_AA_CONFIG_AA_ENABLE |
            } else {
                  if (DBG_ON(r300, DBG_FB)) {
      fprintf(stderr, "r300: set_framebuffer_state:\n");
   for (i = 0; i < state->nr_cbufs; i++) {
         if (state->cbufs[i])
   }
   if (state->zsbuf) {
               }
      /* Create fragment shader state. */
   static void* r300_create_fs_state(struct pipe_context* pipe,
         {
      struct r300_context* r300 = r300_context(pipe);
                     /* Copy state directly into shader. */
            if (fs->state.type == PIPE_SHADER_IR_NIR) {
      if (r300->screen->caps.is_r500)
            } else {
      assert(fs->state.type == PIPE_SHADER_IR_TGSI);
   /* we need to keep a local copy of the tokens */
               /* Precompile the fragment shader at creation time to avoid jank at runtime.
   * In most cases we won't have anything in the key at draw time.
   */
   struct r300_fragment_program_external_state precompile_state;
            struct tgsi_shader_info info;
   tgsi_scan_shader(fs->state.tokens, &info);
   for (int i = 0; i < PIPE_MAX_SHADER_SAMPLER_VIEWS; i++) {
      if (info.sampler_targets[i] == TGSI_TEXTURE_SHADOW1D ||
         info.sampler_targets[i] == TGSI_TEXTURE_SHADOW2D ||
   info.sampler_targets[i] == TGSI_TEXTURE_SHADOWRECT) {
   precompile_state.unit[i].compare_mode_enabled = true;
      }
               }
      void r300_mark_fs_code_dirty(struct r300_context *r300)
   {
               r300_mark_atom_dirty(r300, &r300->fs);
   r300_mark_atom_dirty(r300, &r300->fs_rc_constant_state);
   r300_mark_atom_dirty(r300, &r300->fs_constants);
            if (r300->screen->caps.is_r500) {
      r300->fs_rc_constant_state.size = fs->shader->rc_state_count * 7;
      } else {
      r300->fs_rc_constant_state.size = fs->shader->rc_state_count * 5;
               ((struct r300_constant_buffer*)r300->fs_constants.state)->remap_table =
      }
      /* Bind fragment shader state. */
   static void r300_bind_fs_state(struct pipe_context* pipe, void* shader)
   {
      struct r300_context* r300 = r300_context(pipe);
            if (!fs) {
      r300->fs.state = NULL;
               r300->fs.state = fs;
               }
      /* Delete fragment shader state. */
   static void r300_delete_fs_state(struct pipe_context* pipe, void* shader)
   {
      struct r300_fragment_shader* fs = (struct r300_fragment_shader*)shader;
            while (ptr) {
      tmp = ptr;
   ptr = ptr->next;
   rc_constants_destroy(&tmp->code.constants);
   FREE(tmp->cb_code);
      }
   FREE((void*)fs->state.tokens);
      }
      static void r300_set_polygon_stipple(struct pipe_context* pipe,
         {
   }
      /* Create a new rasterizer state based on the CSO rasterizer state.
   *
   * This is a very large chunk of state, and covers most of the graphics
   * backend (GB), geometry assembly (GA), and setup unit (SU) blocks.
   *
   * In a not entirely unironic sidenote, this state has nearly nothing to do
   * with the actual block on the Radeon called the rasterizer (RS). */
   static void* r300_create_rs_state(struct pipe_context* pipe,
         {
      struct r300_rs_state* rs = CALLOC_STRUCT(r300_rs_state);
   uint32_t vap_control_status;    /* R300_VAP_CNTL_STATUS: 0x2140 */
   uint32_t vap_clip_cntl;         /* R300_VAP_CLIP_CNTL: 0x221C */
   uint32_t point_size;            /* R300_GA_POINT_SIZE: 0x421c */
   uint32_t point_minmax;          /* R300_GA_POINT_MINMAX: 0x4230 */
   uint32_t line_control;          /* R300_GA_LINE_CNTL: 0x4234 */
   uint32_t polygon_offset_enable; /* R300_SU_POLY_OFFSET_ENABLE: 0x42b4 */
   uint32_t cull_mode;             /* R300_SU_CULL_MODE: 0x42b8 */
   uint32_t line_stipple_config;   /* R300_GA_LINE_STIPPLE_CONFIG: 0x4328 */
   uint32_t line_stipple_value;    /* R300_GA_LINE_STIPPLE_VALUE: 0x4260 */
   uint32_t polygon_mode;          /* R300_GA_POLY_MODE: 0x4288 */
   uint32_t clip_rule;             /* R300_SC_CLIP_RULE: 0x43D0 */
            /* Point sprites texture coordinates, 0: lower left, 1: upper right */
   float point_texcoord_left = 0;  /* R300_GA_POINT_S0: 0x4200 */
   float point_texcoord_bottom = 0;/* R300_GA_POINT_T0: 0x4204 */
   float point_texcoord_right = 1; /* R300_GA_POINT_S1: 0x4208 */
   float point_texcoord_top = 0;   /* R300_GA_POINT_T1: 0x420c */
   bool vclamp = !r300_context(pipe)->screen->caps.is_r500;
            /* Copy rasterizer state. */
   rs->rs = *state;
            rs->rs.sprite_coord_enable = state->point_quad_rasterization *
                  /* Override some states for Draw. */
   rs->rs_draw.sprite_coord_enable = 0; /* We can do this in HW. */
   rs->rs_draw.offset_point = 0;
   rs->rs_draw.offset_line = 0;
   rs->rs_draw.offset_tri = 0;
         #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
         /* If no TCL engine is present, turn off the HW TCL. */
   if (!r300_screen(pipe->screen)->caps.has_tcl) {
                  /* Point size width and height. */
   point_size =
      pack_float_16_6x(state->point_size) |
         /* Point size clamping. */
   if (state->point_size_per_vertex) {
      /* Per-vertex point size.
         float min_psiz = util_get_min_point_size(state);
   float max_psiz = pipe->screen->get_paramf(pipe->screen,
         point_minmax =
            } else {
      /* We cannot disable the point-size vertex output,
         float psiz = state->point_size;
   point_minmax =
                     /* Line control. */
   line_control = pack_float_16_6x(state->line_width) |
            /* Enable polygon mode */
   polygon_mode = 0;
   if (state->fill_front != PIPE_POLYGON_MODE_FILL ||
      state->fill_back != PIPE_POLYGON_MODE_FILL) {
               /* Front face */
   if (state->front_ccw) 
         else
            /* Polygon offset */
   polygon_offset_enable = 0;
   if (util_get_offset(state, state->fill_front)) {
         }
   if (util_get_offset(state, state->fill_back)) {
                           /* Polygon mode */
   if (polygon_mode) {
      polygon_mode |=
         polygon_mode |=
               if (state->cull_face & PIPE_FACE_FRONT) {
         }
   if (state->cull_face & PIPE_FACE_BACK) {
                  if (state->line_stipple_enable) {
      line_stipple_config =
         R300_GA_LINE_STIPPLE_CONFIG_LINE_RESET_LINE |
   (fui((float)state->line_stipple_factor) &
   /* XXX this might need to be scaled up */
      } else {
      line_stipple_config = 0;
               if (state->flatshade) {
         } else {
                           /* Point sprites coord mode */
   switch (state->sprite_coord_mode) {
      case PIPE_SPRITE_COORD_UPPER_LEFT:
         point_texcoord_top = 0.0f;
   point_texcoord_bottom = 1.0f;
   case PIPE_SPRITE_COORD_LOWER_LEFT:
         point_texcoord_top = 1.0f;
               if (r300_screen(pipe->screen)->caps.has_tcl) {
      vap_clip_cntl = (state->clip_plane_enable & 63) |
      } else {
                  /* Vertex color clamping. FP20 means no clamping. */
   round_mode =
      R300_GA_ROUND_MODE_GEOMETRY_ROUND_NEAREST |
   (!vclamp ? (R300_GA_ROUND_MODE_RGB_CLAMP_FP20 |
         /* Build the main command buffer. */
   BEGIN_CB(rs->cb_main, RS_STATE_MAIN_SIZE);
   OUT_CB_REG(R300_VAP_CNTL_STATUS, vap_control_status);
   OUT_CB_REG(R300_VAP_CLIP_CNTL, vap_clip_cntl);
   OUT_CB_REG(R300_GA_POINT_SIZE, point_size);
   OUT_CB_REG_SEQ(R300_GA_POINT_MINMAX, 2);
   OUT_CB(point_minmax);
   OUT_CB(line_control);
   OUT_CB_REG_SEQ(R300_SU_POLY_OFFSET_ENABLE, 2);
   OUT_CB(polygon_offset_enable);
   rs->cull_mode_index = 11;
   OUT_CB(cull_mode);
   OUT_CB_REG(R300_GA_LINE_STIPPLE_CONFIG, line_stipple_config);
   OUT_CB_REG(R300_GA_LINE_STIPPLE_VALUE, line_stipple_value);
   OUT_CB_REG(R300_GA_POLY_MODE, polygon_mode);
   OUT_CB_REG(R300_GA_ROUND_MODE, round_mode);
   OUT_CB_REG(R300_SC_CLIP_RULE, clip_rule);
   OUT_CB_REG_SEQ(R300_GA_POINT_S0, 4);
   OUT_CB_32F(point_texcoord_left);
   OUT_CB_32F(point_texcoord_bottom);
   OUT_CB_32F(point_texcoord_right);
   OUT_CB_32F(point_texcoord_top);
            /* Build the two command buffers for polygon offset setup. */
   if (polygon_offset_enable) {
      float scale = state->offset_scale * 12;
            BEGIN_CB(rs->cb_poly_offset_zb16, 5);
   OUT_CB_REG_SEQ(R300_SU_POLY_OFFSET_FRONT_SCALE, 4);
   OUT_CB_32F(scale);
   OUT_CB_32F(offset);
   OUT_CB_32F(scale);
   OUT_CB_32F(offset);
                     BEGIN_CB(rs->cb_poly_offset_zb24, 5);
   OUT_CB_REG_SEQ(R300_SU_POLY_OFFSET_FRONT_SCALE, 4);
   OUT_CB_32F(scale);
   OUT_CB_32F(offset);
   OUT_CB_32F(scale);
   OUT_CB_32F(offset);
                  }
      /* Bind rasterizer state. */
   static void r300_bind_rs_state(struct pipe_context* pipe, void* state)
   {
      struct r300_context* r300 = r300_context(pipe);
   struct r300_rs_state* rs = (struct r300_rs_state*)state;
   int last_sprite_coord_enable = r300->sprite_coord_enable;
   bool last_two_sided_color = r300->two_sided_color;
   bool last_msaa_enable = r300->msaa_enable;
   bool last_flatshade = r300->flatshade;
            if (r300->draw && rs) {
                  if (rs) {
      r300->polygon_offset_enabled = rs->polygon_offset_enable;
   r300->sprite_coord_enable = rs->rs.sprite_coord_enable;
   r300->two_sided_color = rs->rs.light_twoside;
   r300->msaa_enable = rs->rs.multisample;
   r300->flatshade = rs->rs.flatshade;
      } else {
      r300->polygon_offset_enabled = false;
   r300->sprite_coord_enable = 0;
   r300->two_sided_color = false;
   r300->msaa_enable = false;
   r300->flatshade = false;
               UPDATE_STATE(state, r300->rs_state);
            if (last_sprite_coord_enable != r300->sprite_coord_enable ||
      last_two_sided_color != r300->two_sided_color ||
   last_flatshade != r300->flatshade) {
               if (last_msaa_enable != r300->msaa_enable) {
      if (r300->alpha_to_coverage) {
                  if (r300->alpha_to_one &&
         r300->fs_status == FRAGMENT_SHADER_VALID) {
               if (r300->screen->caps.has_tcl && last_clip_halfz != r300->clip_halfz) {
            }
      /* Free rasterizer state. */
   static void r300_delete_rs_state(struct pipe_context* pipe, void* state)
   {
         }
      static void*
         r300_create_sampler_state(struct pipe_context* pipe,
   {
      struct r300_context* r300 = r300_context(pipe);
   struct r300_sampler_state* sampler = CALLOC_STRUCT(r300_sampler_state);
   bool is_r500 = r300->screen->caps.is_r500;
                     /* r300 doesn't handle CLAMP and MIRROR_CLAMP correctly when either MAG
   * or MIN filter is NEAREST. Since texwrap produces same results
   * for CLAMP and CLAMP_TO_EDGE, we use them instead. */
   if (sampler->state.min_img_filter == PIPE_TEX_FILTER_NEAREST ||
      sampler->state.mag_img_filter == PIPE_TEX_FILTER_NEAREST) {
   /* Wrap S. */
   if (sampler->state.wrap_s == PIPE_TEX_WRAP_CLAMP)
         else if (sampler->state.wrap_s == PIPE_TEX_WRAP_MIRROR_CLAMP)
            /* Wrap T. */
   if (sampler->state.wrap_t == PIPE_TEX_WRAP_CLAMP)
         else if (sampler->state.wrap_t == PIPE_TEX_WRAP_MIRROR_CLAMP)
            /* Wrap R. */
   if (sampler->state.wrap_r == PIPE_TEX_WRAP_CLAMP)
         else if (sampler->state.wrap_r == PIPE_TEX_WRAP_MIRROR_CLAMP)
               sampler->filter0 |=
      (r300_translate_wrap(sampler->state.wrap_s) << R300_TX_WRAP_S_SHIFT) |
   (r300_translate_wrap(sampler->state.wrap_t) << R300_TX_WRAP_T_SHIFT) |
         sampler->filter0 |= r300_translate_tex_filters(state->min_img_filter,
                                 /* Unfortunately, r300-r500 don't support floating-point mipmap lods. */
   /* We must pass these to the merge function to clamp them properly. */
   sampler->min_lod = (unsigned)MAX2(state->min_lod, 0);
                              /* This is very high quality anisotropic filtering for R5xx.
   * It's good for benchmarking the performance of texturing but
   * in practice we don't want to slow down the driver because it's
   * a pretty good performance killer. Feel free to play with it. */
   if (DBG_ON(r300, DBG_ANISOHQ) && is_r500) {
                  /* R500-specific fixups and optimizations */
   if (r300->screen->caps.is_r500) {
                     }
      static void r300_bind_sampler_states(struct pipe_context* pipe,
                     {
      struct r300_context* r300 = r300_context(pipe);
   struct r300_textures_state* state =
                           if (shader != PIPE_SHADER_FRAGMENT)
            if (count > tex_units)
            memcpy(state->sampler_states, states, sizeof(void*) * count);
               }
      static void r300_delete_sampler_state(struct pipe_context* pipe, void* state)
   {
         }
      static uint32_t r300_assign_texture_cache_region(unsigned index, unsigned num)
   {
      /* This looks like a hack, but I believe it's suppose to work like
   * that. To illustrate how this works, let's assume you have 5 textures.
   * From docs, 5 and the successive numbers are:
   *
   * FOURTH_1     = 5
   * FOURTH_2     = 6
   * FOURTH_3     = 7
   * EIGHTH_0     = 8
   * EIGHTH_1     = 9
   *
   * First 3 textures will get 3/4 of size of the cache, divided evenly
   * between them. The last 1/4 of the cache must be divided between
   * the last 2 textures, each will therefore get 1/8 of the cache.
   * Why not just to use "5 + texture_index" ?
   *
   * This simple trick works for all "num" <= 16.
   */
   if (num <= 1)
         else
      }
      static void r300_set_sampler_views(struct pipe_context* pipe,
                                 {
      struct r300_context* r300 = r300_context(pipe);
   struct r300_textures_state* state =
         struct r300_resource *texture;
   unsigned i, real_num_views = 0, view_index = 0;
   unsigned tex_units = r300->screen->caps.num_tex_units;
                     if (shader != PIPE_SHADER_FRAGMENT || count > tex_units) {
      if (take_ownership) {
      for (unsigned i = 0; i < count; i++) {
      struct pipe_sampler_view *view = views[i];
         }
               /* Calculate the real number of views. */
   for (i = 0; i < count; i++) {
      if (views[i])
               for (i = 0; i < count; i++) {
      if (take_ownership) {
         pipe_sampler_view_reference(
         } else {
         pipe_sampler_view_reference(
                  if (!views[i]) {
                  /* A new sampler view (= texture)... */
            /* Set the texrect factor in the fragment shader.
         texture = r300_resource(views[i]->texture);
   if (texture->tex.is_npot) {
                  state->sampler_views[i]->texcache_region =
                     for (i = count; i < tex_units; i++) {
      if (state->sampler_views[i]) {
         pipe_sampler_view_reference(
                                       if (dirty_tex) {
            }
      struct pipe_sampler_view *
   r300_create_sampler_view_custom(struct pipe_context *pipe,
                           {
      struct r300_sampler_view *view = CALLOC_STRUCT(r300_sampler_view);
   struct r300_resource *tex = r300_resource(texture);
   bool is_r500 = r300_screen(pipe->screen)->caps.is_r500;
            if (view) {
               view->base = *templ;
   view->base.reference.count = 1;
   view->base.context = pipe;
   view->base.texture = NULL;
      view->width0_override = width0_override;
   view->height0_override = height0_override;
         view->swizzle[0] = templ->swizzle_r;
   view->swizzle[1] = templ->swizzle_g;
   view->swizzle[2] = templ->swizzle_b;
            hwformat = r300_translate_texformat(templ->format,
                        if (hwformat == ~0) {
         fprintf(stderr, "r300: Oops. Got unsupported format %s in %s.\n",
   }
      r300_texture_setup_format_state(r300_screen(pipe->screen), tex,
      templ->format, 0,
         &view->format);
      view->format.format1 |= hwformat;
   if (is_r500) {
                        }
      static struct pipe_sampler_view *
   r300_create_sampler_view(struct pipe_context *pipe,
               {
      return r300_create_sampler_view_custom(pipe, texture, templ,
            }
         static void
   r300_sampler_view_destroy(struct pipe_context *pipe,
         {
      pipe_resource_reference(&view->texture, NULL);
      }
      static void r300_set_sample_mask(struct pipe_context *pipe,
         {
                           }
      static void r300_set_scissor_states(struct pipe_context* pipe,
                     {
               memcpy(r300->scissor_state.state, state,
               }
      static void r300_set_viewport_states(struct pipe_context* pipe,
                     {
      struct r300_context* r300 = r300_context(pipe);
   struct r300_viewport_state* viewport =
                     if (r300->draw) {
      draw_set_viewport_states(r300->draw, start_slot, num_viewports, state);
   viewport->vte_control = R300_VTX_XY_FMT | R300_VTX_Z_FMT;
               /* Do the transform in HW. */
            if (state->scale[0] != 1.0f) {
      viewport->xscale = state->scale[0];
      }
   if (state->scale[1] != 1.0f) {
      viewport->yscale = state->scale[1];
      }
   if (state->scale[2] != 1.0f) {
      viewport->zscale = state->scale[2];
      }
   if (state->translate[0] != 0.0f) {
      viewport->xoffset = state->translate[0];
      }
   if (state->translate[1] != 0.0f) {
      viewport->yoffset = state->translate[1];
      }
   if (state->translate[2] != 0.0f) {
      viewport->zoffset = state->translate[2];
               r300_mark_atom_dirty(r300, &r300->viewport_state);
   if (r300->fs.state && r300_fs(r300)->shader &&
      r300_fs(r300)->shader->inputs.wpos != ATTR_UNUSED) {
         }
      static void r300_set_vertex_buffers_hwtcl(struct pipe_context* pipe,
                           {
               util_set_vertex_buffers_count(r300->vertex_buffer,
                        /* There must be at least one vertex buffer set, otherwise it locks up. */
   if (!r300->nr_vertex_buffers) {
      util_set_vertex_buffers_count(r300->vertex_buffer,
                        }
      static void r300_set_vertex_buffers_swtcl(struct pipe_context* pipe,
                           {
      struct r300_context* r300 = r300_context(pipe);
            util_set_vertex_buffers_count(r300->vertex_buffer,
                     draw_set_vertex_buffers(r300->draw, count,
            if (!buffers)
            for (i = 0; i < count; i++) {
      if (buffers[i].is_user_buffer) {
         draw_set_mapped_vertex_buffer(r300->draw, i,
   } else if (buffers[i].buffer.resource) {
         draw_set_mapped_vertex_buffer(r300->draw, i,
         }
      /* Initialize the PSC tables. */
   static void r300_vertex_psc(struct r300_vertex_element_state *velems)
   {
      struct r300_vertex_stream_state *vstream = &velems->vertex_stream;
   uint16_t type, swizzle;
   enum pipe_format format;
            /* Vertex shaders have no semantics on their inputs,
   * so PSC should just route stuff based on the vertex elements,
   * and not on attrib information. */
   for (i = 0; i < velems->count; i++) {
               type = r300_translate_vertex_data_type(format);
   if (type == R300_INVALID_FORMAT) {
         fprintf(stderr, "r300: Bad vertex format %s.\n",
         assert(0);
            type |= i << R300_DST_VEC_LOC_SHIFT;
            if (i & 1) {
         vstream->vap_prog_stream_cntl[i >> 1] |= type << 16;
   } else {
         vstream->vap_prog_stream_cntl[i >> 1] |= type;
               /* Set the last vector in the PSC. */
   if (i) {
         }
   vstream->vap_prog_stream_cntl[i >> 1] |=
               }
      static void* r300_create_vertex_elements_state(struct pipe_context* pipe,
               {
      struct r300_vertex_element_state *velems;
   unsigned i;
            /* R300 Programmable Stream Control (PSC) doesn't support 0 vertex elements. */
   if (!count) {
      dummy_attrib.src_format = PIPE_FORMAT_R8G8B8A8_UNORM;
   attribs = &dummy_attrib;
      } else if (count > 16) {
      fprintf(stderr, "r300: More than 16 vertex elements are not supported,"
                     velems = CALLOC_STRUCT(r300_vertex_element_state);
   if (!velems)
            velems->count = count;
            if (r300_screen(pipe->screen)->caps.has_tcl) {
      /* Setup PSC.
                  for (i = 0; i < count; i++) {
         velems->format_size[i] =
                        }
      static void r300_bind_vertex_elements_state(struct pipe_context *pipe,
         {
      struct r300_context *r300 = r300_context(pipe);
            if (!velems) {
                           if (r300->draw) {
      draw_set_vertex_elements(r300->draw, velems->count, velems->velem);
               UPDATE_STATE(&velems->vertex_stream, r300->vertex_stream_state);
   r300->vertex_stream_state.size = (1 + velems->vertex_stream.count) * 2;
      }
      static void r300_delete_vertex_elements_state(struct pipe_context *pipe, void *state)
   {
         }
      static void* r300_create_vs_state(struct pipe_context* pipe,
         {
      struct r300_context* r300 = r300_context(pipe);
            /* Copy state directly into shader. */
            if (vs->state.type == PIPE_SHADER_IR_NIR) {
      static const struct nir_to_rc_options swtcl_options = {0};
   static const struct nir_to_rc_options hwtcl_r300_options = {
      .lower_cmp = true,
   .lower_fabs = true,
   .ubo_vec4_max = 0x00ff,
      };
   static const struct nir_to_rc_options hwtcl_r500_options = {
      .ubo_vec4_max = 0x00ff,
      };
   const struct nir_to_rc_options *ntr_options;
   if (r300->screen->caps.has_tcl) {
                           /* Only nine should set both NTT shader name and
   * use_legacy_math_rules and D3D9 already mandates
   * the proper range for the trigonometric inputs.
   */
   struct shader_info *info = &(((struct nir_shader *)(shader->ir.nir))->info);
   if (!info->use_legacy_math_rules ||
      !(info->name && !strcmp("TTN", info->name))) {
      }
   else
      } else {
         }
   vs->state.tokens = nir_to_rc_options(shader->ir.nir, pipe->screen,
      } else {
      assert(vs->state.type == PIPE_SHADER_IR_TGSI);
   /* we need to keep a local copy of the tokens */
               if (!vs->first)
         if (r300->screen->caps.has_tcl) {
         } else {
                     }
      static void r300_bind_vs_state(struct pipe_context* pipe, void* shader)
   {
      struct r300_context* r300 = r300_context(pipe);
            if (!vs) {
      r300->vs_state.state = NULL;
      }
   if (vs == r300->vs_state.state) {
         }
            /* The majority of the RS block bits is dependent on the vertex shader. */
            if (r300->screen->caps.has_tcl) {
      unsigned fc_op_dwords = r300->screen->caps.is_r500 ? 3 : 2;
   r300_mark_atom_dirty(r300, &r300->vs_state);
                  r300_mark_atom_dirty(r300, &r300->vs_constants);
   r300->vs_constants.size =
                        ((struct r300_constant_buffer*)r300->vs_constants.state)->remap_table =
               } else {
      draw_bind_vertex_shader(r300->draw,
         }
      static void r300_delete_vs_state(struct pipe_context* pipe, void* shader)
   {
      struct r300_context* r300 = r300_context(pipe);
            if (r300->screen->caps.has_tcl) {
      while (vs->shader) {
         rc_constants_destroy(&vs->shader->code.constants);
   FREE(vs->shader->code.constants_remap_table);
   vs->shader = vs->shader->next;
   }
      } else {
      draw_delete_vertex_shader(r300->draw,
               FREE((void*)vs->state.tokens);
      }
      static void r300_set_constant_buffer(struct pipe_context *pipe,
                     {
      struct r300_context* r300 = r300_context(pipe);
   struct r300_constant_buffer *cbuf;
            if (!cb || (!cb->buffer && !cb->user_buffer))
            switch (shader) {
      case PIPE_SHADER_VERTEX:
         cbuf = (struct r300_constant_buffer*)r300->vs_constants.state;
   case PIPE_SHADER_FRAGMENT:
         cbuf = (struct r300_constant_buffer*)r300->fs_constants.state;
   default:
                  if (cb->user_buffer)
         else {
               if (rbuf && rbuf->malloced_buffer)
         else
               if (shader == PIPE_SHADER_FRAGMENT ||
      (shader == PIPE_SHADER_VERTEX && r300->screen->caps.has_tcl)) {
               if (shader == PIPE_SHADER_VERTEX) {
      if (r300->screen->caps.has_tcl) {
                  if (!vs) {
                        cbuf->buffer_base = r300->vs_const_base;
   r300->vs_const_base += vs->shader->code.constants.Count;
   if (r300->vs_const_base > R500_MAX_PVS_CONST_VECS) {
      r300->vs_const_base = vs->shader->code.constants.Count;
   cbuf->buffer_base = 0;
      }
   } else if (r300->draw) {
         draw_set_mapped_constant_buffer(r300->draw, PIPE_SHADER_VERTEX,
      } else if (shader == PIPE_SHADER_FRAGMENT) {
            }
      static void r300_texture_barrier(struct pipe_context *pipe, unsigned flags)
   {
               r300_mark_atom_dirty(r300, &r300->gpu_flush);
      }
      static void r300_memory_barrier(struct pipe_context *pipe, unsigned flags)
   {
   }
      void r300_init_state_functions(struct r300_context* r300)
   {
      r300->context.create_blend_state = r300_create_blend_state;
   r300->context.bind_blend_state = r300_bind_blend_state;
                     r300->context.set_clip_state = r300_set_clip_state;
                     r300->context.create_depth_stencil_alpha_state = r300_create_dsa_state;
   r300->context.bind_depth_stencil_alpha_state = r300_bind_dsa_state;
                              r300->context.create_fs_state = r300_create_fs_state;
   r300->context.bind_fs_state = r300_bind_fs_state;
                     r300->context.create_rasterizer_state = r300_create_rs_state;
   r300->context.bind_rasterizer_state = r300_bind_rs_state;
            r300->context.create_sampler_state = r300_create_sampler_state;
   r300->context.bind_sampler_states = r300_bind_sampler_states;
            r300->context.set_sampler_views = r300_set_sampler_views;
   r300->context.create_sampler_view = r300_create_sampler_view;
                              if (r300->screen->caps.has_tcl) {
         } else {
                  r300->context.create_vertex_elements_state = r300_create_vertex_elements_state;
   r300->context.bind_vertex_elements_state = r300_bind_vertex_elements_state;
            r300->context.create_vs_state = r300_create_vs_state;
   r300->context.bind_vs_state = r300_bind_vs_state;
            r300->context.texture_barrier = r300_texture_barrier;
      }
