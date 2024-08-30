   /*
   * Copyright 2012 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_build_pm4.h"
   #include "si_query.h"
   #include "si_shader_internal.h"
   #include "sid.h"
   #include "util/fast_idiv_by_const.h"
   #include "util/format/u_format.h"
   #include "util/format/u_format_s3tc.h"
   #include "util/u_dual_blend.h"
   #include "util/u_helpers.h"
   #include "util/u_memory.h"
   #include "util/u_resource.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_blend.h"
      #include "gfx10_format_table.h"
      static unsigned si_map_swizzle(unsigned swizzle)
   {
      switch (swizzle) {
   case PIPE_SWIZZLE_Y:
         case PIPE_SWIZZLE_Z:
         case PIPE_SWIZZLE_W:
         case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
         default: /* PIPE_SWIZZLE_X */
            }
      /* 12.4 fixed-point */
   static unsigned si_pack_float_12p4(float x)
   {
         }
      /*
   * Inferred framebuffer and blender state.
   *
   * CB_TARGET_MASK is emitted here to avoid a hang with dual source blending
   * if there is not enough PS outputs.
   */
   static void si_emit_cb_render_state(struct si_context *sctx, unsigned index)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
   struct si_state_blend *blend = sctx->queued.named.blend;
   /* CB_COLORn_INFO.FORMAT=INVALID should disable unbound colorbuffers,
   * but you never know. */
   uint32_t cb_target_mask = sctx->framebuffer.colorbuf_enabled_4bit & blend->cb_target_mask;
            /* Avoid a hang that happens when dual source blending is enabled
   * but there is not enough color outputs. This is undefined behavior,
   * so disable color writes completely.
   *
   * Reproducible with Unigine Heaven 4.0 and drirc missing.
   */
   if (blend->dual_src_blend && sctx->shader.ps.cso &&
      (sctx->shader.ps.cso->info.colors_written & 0x3) != 0x3)
         /* GFX9: Flush DFSM when CB_TARGET_MASK changes.
   * I think we don't have to do anything between IBs.
   */
   if (sctx->screen->dpbb_allowed && sctx->last_cb_target_mask != cb_target_mask &&
      sctx->screen->pbb_context_states_per_bin > 1) {
            radeon_begin(cs);
   radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
   radeon_emit(EVENT_TYPE(V_028A90_BREAK_BATCH) | EVENT_INDEX(0));
               radeon_begin(cs);
   radeon_opt_set_context_reg(sctx, R_028238_CB_TARGET_MASK, SI_TRACKED_CB_TARGET_MASK,
            if (sctx->gfx_level >= GFX8) {
      /* DCC MSAA workaround.
   * Alternatively, we can set CB_COLORi_DCC_CONTROL.OVERWRITE_-
   * COMBINER_DISABLE, but that would be more complicated.
   */
   bool oc_disable =
            if (sctx->gfx_level >= GFX11) {
      radeon_opt_set_context_reg(sctx, R_028424_CB_FDCC_CONTROL, SI_TRACKED_CB_DCC_CONTROL,
            } else {
      radeon_opt_set_context_reg(
      sctx, R_028424_CB_DCC_CONTROL, SI_TRACKED_CB_DCC_CONTROL,
   S_028424_OVERWRITE_COMBINER_MRT_SHARING_DISABLE(sctx->gfx_level <= GFX9) |
      S_028424_OVERWRITE_COMBINER_WATERMARK(sctx->gfx_level >= GFX10 ? 6 : 4) |
   S_028424_OVERWRITE_COMBINER_DISABLE(oc_disable) |
                  /* RB+ register settings. */
   if (sctx->screen->info.rbplus_allowed) {
      unsigned spi_shader_col_format =
      sctx->shader.ps.cso ? sctx->shader.ps.current->key.ps.part.epilog.spi_shader_col_format
      unsigned sx_ps_downconvert = 0;
   unsigned sx_blend_opt_epsilon = 0;
   unsigned sx_blend_opt_control = 0;
   unsigned num_cbufs = util_last_bit(sctx->framebuffer.colorbuf_enabled_4bit &
            for (i = 0; i < num_cbufs; i++) {
      struct si_surface *surf = (struct si_surface *)sctx->framebuffer.state.cbufs[i];
                  if (!surf) {
      /* If the color buffer is not set, the driver sets 32_R
   * as the SPI color format, because the hw doesn't allow
   * holes between color outputs, so also set this to
   * enable RB+.
   */
   sx_ps_downconvert |= V_028754_SX_RT_EXPORT_32_R << (i * 4);
               format = sctx->gfx_level >= GFX11 ? G_028C70_FORMAT_GFX11(surf->cb_color_info):
         swap = G_028C70_COMP_SWAP(surf->cb_color_info);
                  /* Set if RGB and A are present. */
                  if (format == V_028C70_COLOR_8 || format == V_028C70_COLOR_16 ||
      format == V_028C70_COLOR_32)
                     /* Check the colormask and export format. */
   if (!(colormask & (PIPE_MASK_RGBA & ~PIPE_MASK_A)))
                        if (spi_format == V_028714_SPI_SHADER_ZERO) {
      has_rgb = false;
               /* Disable value checking for disabled channels. */
   if (!has_rgb)
                        /* Enable down-conversion for 32bpp and smaller formats. */
   switch (format) {
   case V_028C70_COLOR_8:
   case V_028C70_COLOR_8_8:
   case V_028C70_COLOR_8_8_8_8:
      /* For 1 and 2-channel formats, use the superset thereof. */
   if (spi_format == V_028714_SPI_SHADER_FP16_ABGR ||
      spi_format == V_028714_SPI_SHADER_UINT16_ABGR ||
   spi_format == V_028714_SPI_SHADER_SINT16_ABGR) {
   sx_ps_downconvert |= V_028754_SX_RT_EXPORT_8_8_8_8 << (i * 4);
   if (G_028C70_NUMBER_TYPE(surf->cb_color_info) != V_028C70_NUMBER_SRGB)
                  case V_028C70_COLOR_5_6_5:
      if (spi_format == V_028714_SPI_SHADER_FP16_ABGR) {
      sx_ps_downconvert |= V_028754_SX_RT_EXPORT_5_6_5 << (i * 4);
                  case V_028C70_COLOR_1_5_5_5:
      if (spi_format == V_028714_SPI_SHADER_FP16_ABGR) {
      sx_ps_downconvert |= V_028754_SX_RT_EXPORT_1_5_5_5 << (i * 4);
                  case V_028C70_COLOR_4_4_4_4:
      if (spi_format == V_028714_SPI_SHADER_FP16_ABGR) {
      sx_ps_downconvert |= V_028754_SX_RT_EXPORT_4_4_4_4 << (i * 4);
                  case V_028C70_COLOR_32:
      if (swap == V_028C70_SWAP_STD && spi_format == V_028714_SPI_SHADER_32_R)
         else if (swap == V_028C70_SWAP_ALT_REV && spi_format == V_028714_SPI_SHADER_32_AR)
               case V_028C70_COLOR_16:
   case V_028C70_COLOR_16_16:
      /* For 1-channel formats, use the superset thereof. */
   if (spi_format == V_028714_SPI_SHADER_UNORM16_ABGR ||
      spi_format == V_028714_SPI_SHADER_SNORM16_ABGR ||
   spi_format == V_028714_SPI_SHADER_UINT16_ABGR ||
   spi_format == V_028714_SPI_SHADER_SINT16_ABGR) {
   if (swap == V_028C70_SWAP_STD || swap == V_028C70_SWAP_STD_REV)
         else
                  case V_028C70_COLOR_10_11_11:
      if (spi_format == V_028714_SPI_SHADER_FP16_ABGR)
               case V_028C70_COLOR_2_10_10_10:
   case V_028C70_COLOR_10_10_10_2:
      if (spi_format == V_028714_SPI_SHADER_FP16_ABGR) {
      sx_ps_downconvert |= V_028754_SX_RT_EXPORT_2_10_10_10 << (i * 4);
                  case V_028C70_COLOR_5_9_9_9:
      if (spi_format == V_028714_SPI_SHADER_FP16_ABGR)
                        /* If there are no color outputs, the first color export is
   * always enabled as 32_R, so also set this to enable RB+.
   */
   if (!sx_ps_downconvert)
            /* SX_PS_DOWNCONVERT, SX_BLEND_OPT_EPSILON, SX_BLEND_OPT_CONTROL */
   radeon_opt_set_context_reg3(sctx, R_028754_SX_PS_DOWNCONVERT, SI_TRACKED_SX_PS_DOWNCONVERT,
      }
      }
      /*
   * Blender functions
   */
      static uint32_t si_translate_blend_function(int blend_func)
   {
      switch (blend_func) {
   case PIPE_BLEND_ADD:
         case PIPE_BLEND_SUBTRACT:
         case PIPE_BLEND_REVERSE_SUBTRACT:
         case PIPE_BLEND_MIN:
         case PIPE_BLEND_MAX:
         default:
      PRINT_ERR("Unknown blend function %d\n", blend_func);
   assert(0);
      }
      }
      static uint32_t si_translate_blend_factor(enum amd_gfx_level gfx_level, int blend_fact)
   {
      switch (blend_fact) {
   case PIPE_BLENDFACTOR_ONE:
         case PIPE_BLENDFACTOR_SRC_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA:
         case PIPE_BLENDFACTOR_DST_ALPHA:
         case PIPE_BLENDFACTOR_DST_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
         case PIPE_BLENDFACTOR_CONST_COLOR:
      return gfx_level >= GFX11 ? V_028780_BLEND_CONSTANT_COLOR_GFX11:
      case PIPE_BLENDFACTOR_CONST_ALPHA:
      return gfx_level >= GFX11 ? V_028780_BLEND_CONSTANT_ALPHA_GFX11 :
      case PIPE_BLENDFACTOR_ZERO:
         case PIPE_BLENDFACTOR_INV_SRC_COLOR:
         case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_COLOR:
         case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      return gfx_level >= GFX11 ? V_028780_BLEND_ONE_MINUS_CONSTANT_COLOR_GFX11:
      case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      return gfx_level >= GFX11 ? V_028780_BLEND_ONE_MINUS_CONSTANT_ALPHA_GFX11:
      case PIPE_BLENDFACTOR_SRC1_COLOR:
      return gfx_level >= GFX11 ? V_028780_BLEND_SRC1_COLOR_GFX11:
      case PIPE_BLENDFACTOR_SRC1_ALPHA:
      return gfx_level >= GFX11 ? V_028780_BLEND_SRC1_ALPHA_GFX11:
      case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
      return gfx_level >= GFX11 ? V_028780_BLEND_INV_SRC1_COLOR_GFX11:
      case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
      return gfx_level >= GFX11 ? V_028780_BLEND_INV_SRC1_ALPHA_GFX11:
      default:
      PRINT_ERR("Bad blend factor %d not supported!\n", blend_fact);
   assert(0);
      }
      }
      static uint32_t si_translate_blend_opt_function(int blend_func)
   {
      switch (blend_func) {
   case PIPE_BLEND_ADD:
         case PIPE_BLEND_SUBTRACT:
         case PIPE_BLEND_REVERSE_SUBTRACT:
         case PIPE_BLEND_MIN:
         case PIPE_BLEND_MAX:
         default:
            }
      static uint32_t si_translate_blend_opt_factor(int blend_fact, bool is_alpha)
   {
      switch (blend_fact) {
   case PIPE_BLENDFACTOR_ZERO:
         case PIPE_BLENDFACTOR_ONE:
         case PIPE_BLENDFACTOR_SRC_COLOR:
      return is_alpha ? V_028760_BLEND_OPT_PRESERVE_A1_IGNORE_A0
      case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      return is_alpha ? V_028760_BLEND_OPT_PRESERVE_A0_IGNORE_A1
      case PIPE_BLENDFACTOR_SRC_ALPHA:
         case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      return is_alpha ? V_028760_BLEND_OPT_PRESERVE_ALL_IGNORE_NONE
      default:
            }
      static void si_blend_check_commutativity(struct si_screen *sscreen, struct si_state_blend *blend,
               {
      /* Src factor is allowed when it does not depend on Dst */
   static const uint32_t src_allowed =
      (1u << PIPE_BLENDFACTOR_ONE) | (1u << PIPE_BLENDFACTOR_SRC_COLOR) |
   (1u << PIPE_BLENDFACTOR_SRC_ALPHA) | (1u << PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE) |
   (1u << PIPE_BLENDFACTOR_CONST_COLOR) | (1u << PIPE_BLENDFACTOR_CONST_ALPHA) |
   (1u << PIPE_BLENDFACTOR_SRC1_COLOR) | (1u << PIPE_BLENDFACTOR_SRC1_ALPHA) |
   (1u << PIPE_BLENDFACTOR_ZERO) | (1u << PIPE_BLENDFACTOR_INV_SRC_COLOR) |
   (1u << PIPE_BLENDFACTOR_INV_SRC_ALPHA) | (1u << PIPE_BLENDFACTOR_INV_CONST_COLOR) |
   (1u << PIPE_BLENDFACTOR_INV_CONST_ALPHA) | (1u << PIPE_BLENDFACTOR_INV_SRC1_COLOR) |
         if (dst == PIPE_BLENDFACTOR_ONE && (src_allowed & (1u << src)) &&
      (func == PIPE_BLEND_MAX || func == PIPE_BLEND_MIN))
   }
      /**
   * Get rid of DST in the blend factors by commuting the operands:
   *    func(src * DST, dst * 0) ---> func(src * 0, dst * SRC)
   */
   static void si_blend_remove_dst(unsigned *func, unsigned *src_factor, unsigned *dst_factor,
         {
      if (*src_factor == expected_dst && *dst_factor == PIPE_BLENDFACTOR_ZERO) {
      *src_factor = PIPE_BLENDFACTOR_ZERO;
            /* Commuting the operands requires reversing subtractions. */
   if (*func == PIPE_BLEND_SUBTRACT)
         else if (*func == PIPE_BLEND_REVERSE_SUBTRACT)
         }
      static void *si_create_blend_state_mode(struct pipe_context *ctx,
         {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_state_blend *blend = CALLOC_STRUCT(si_state_blend);
   struct si_pm4_state *pm4 = &blend->pm4;
   uint32_t sx_mrt_blend_opt[8] = {0};
   uint32_t color_control = 0;
            if (!blend)
                     blend->alpha_to_coverage = state->alpha_to_coverage;
   blend->alpha_to_one = state->alpha_to_one;
   blend->dual_src_blend = util_blend_state_is_dual(state, 0);
   blend->logicop_enable = logicop_enable;
   blend->allows_noop_optimization =
      state->rt[0].rgb_func == PIPE_BLEND_ADD &&
   state->rt[0].alpha_func == PIPE_BLEND_ADD &&
   state->rt[0].rgb_src_factor == PIPE_BLENDFACTOR_DST_COLOR &&
   state->rt[0].alpha_src_factor == PIPE_BLENDFACTOR_DST_COLOR &&
   state->rt[0].rgb_dst_factor == PIPE_BLENDFACTOR_ZERO &&
   state->rt[0].alpha_dst_factor == PIPE_BLENDFACTOR_ZERO &&
         unsigned num_shader_outputs = state->max_rt + 1; /* estimate */
   if (blend->dual_src_blend)
            if (logicop_enable) {
         } else {
                  unsigned db_alpha_to_mask;
   if (state->alpha_to_coverage && state->alpha_to_coverage_dither) {
      db_alpha_to_mask = S_028B70_ALPHA_TO_MASK_ENABLE(state->alpha_to_coverage) |
                  } else {
      db_alpha_to_mask = S_028B70_ALPHA_TO_MASK_ENABLE(state->alpha_to_coverage) |
                                    blend->cb_target_mask = 0;
                     for (int i = 0; i < num_shader_outputs; i++) {
      /* state->rt entries > 0 only written if independent blending */
            unsigned eqRGB = state->rt[j].rgb_func;
   unsigned srcRGB = state->rt[j].rgb_src_factor;
   unsigned dstRGB = state->rt[j].rgb_dst_factor;
   unsigned eqA = state->rt[j].alpha_func;
   unsigned srcA = state->rt[j].alpha_src_factor;
            unsigned srcRGB_opt, dstRGB_opt, srcA_opt, dstA_opt;
            sx_mrt_blend_opt[i] = S_028760_COLOR_COMB_FCN(V_028760_OPT_COMB_BLEND_DISABLED) |
            /* Only set dual source blending for MRT0 to avoid a hang. */
   if (i >= 1 && blend->dual_src_blend) {
      if (i == 1) {
      if (sctx->gfx_level >= GFX11)
         else
               si_pm4_set_reg(pm4, R_028780_CB_BLEND0_CONTROL + i * 4, blend_cntl);
               /* Only addition and subtraction equations are supported with
   * dual source blending.
   */
   if (blend->dual_src_blend && (eqRGB == PIPE_BLEND_MIN || eqRGB == PIPE_BLEND_MAX ||
            assert(!"Unsupported equation for dual source blending");
   si_pm4_set_reg(pm4, R_028780_CB_BLEND0_CONTROL + i * 4, blend_cntl);
               /* cb_render_state will disable unused ones */
   blend->cb_target_mask |= (unsigned)state->rt[j].colormask << (4 * i);
   if (state->rt[j].colormask)
            if (!state->rt[j].colormask || !state->rt[j].blend_enable) {
      si_pm4_set_reg(pm4, R_028780_CB_BLEND0_CONTROL + i * 4, blend_cntl);
               si_blend_check_commutativity(sctx->screen, blend, eqRGB, srcRGB, dstRGB, 0x7 << (4 * i));
            /* Blending optimizations for RB+.
   * These transformations don't change the behavior.
   *
   * First, get rid of DST in the blend factors:
   *    func(src * DST, dst * 0) ---> func(src * 0, dst * SRC)
   */
   si_blend_remove_dst(&eqRGB, &srcRGB, &dstRGB, PIPE_BLENDFACTOR_DST_COLOR,
         si_blend_remove_dst(&eqA, &srcA, &dstA, PIPE_BLENDFACTOR_DST_COLOR,
         si_blend_remove_dst(&eqA, &srcA, &dstA, PIPE_BLENDFACTOR_DST_ALPHA,
            /* Look up the ideal settings from tables. */
   srcRGB_opt = si_translate_blend_opt_factor(srcRGB, false);
   dstRGB_opt = si_translate_blend_opt_factor(dstRGB, false);
   srcA_opt = si_translate_blend_opt_factor(srcA, true);
            /* Handle interdependencies. */
   if (util_blend_factor_uses_dest(srcRGB, false))
         if (util_blend_factor_uses_dest(srcA, false))
            if (srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE &&
      (dstRGB == PIPE_BLENDFACTOR_ZERO || dstRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
               /* Set the final value. */
   sx_mrt_blend_opt[i] = S_028760_COLOR_SRC_OPT(srcRGB_opt) |
                              /* Alpha-to-coverage with blending enabled, depth writes enabled, and having no MRTZ export
   * should disable SX blend optimizations.
   *
   * TODO: Add a piglit test for this. It should fail on gfx11 without this.
   */
   if (sctx->gfx_level >= GFX11 && state->alpha_to_coverage && i == 0) {
      sx_mrt_blend_opt[0] = S_028760_COLOR_COMB_FCN(V_028760_OPT_COMB_NONE) |
               /* Set blend state. */
   blend_cntl |= S_028780_ENABLE(1);
   blend_cntl |= S_028780_COLOR_COMB_FCN(si_translate_blend_function(eqRGB));
   blend_cntl |= S_028780_COLOR_SRCBLEND(si_translate_blend_factor(sctx->gfx_level, srcRGB));
            if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB) {
      blend_cntl |= S_028780_SEPARATE_ALPHA_BLEND(1);
   blend_cntl |= S_028780_ALPHA_COMB_FCN(si_translate_blend_function(eqA));
   blend_cntl |= S_028780_ALPHA_SRCBLEND(si_translate_blend_factor(sctx->gfx_level, srcA));
      }
   si_pm4_set_reg(pm4, R_028780_CB_BLEND0_CONTROL + i * 4, blend_cntl);
                     if (sctx->gfx_level >= GFX8 && sctx->gfx_level <= GFX10)
            /* This is only important for formats without alpha. */
   if (srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA || dstRGB == PIPE_BLENDFACTOR_SRC_ALPHA ||
      srcRGB == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
   dstRGB == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE ||
   srcRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA || dstRGB == PIPE_BLENDFACTOR_INV_SRC_ALPHA)
            if (sctx->gfx_level >= GFX8 && sctx->gfx_level <= GFX10 && logicop_enable)
            if (blend->cb_target_mask) {
         } else {
                  if (sctx->screen->info.rbplus_allowed) {
      /* Disable RB+ blend optimizations for dual source blending.
   * Vulkan does this.
   */
   if (blend->dual_src_blend) {
      for (int i = 0; i < num_shader_outputs; i++) {
      sx_mrt_blend_opt[i] = S_028760_COLOR_COMB_FCN(V_028760_OPT_COMB_NONE) |
                  for (int i = 0; i < num_shader_outputs; i++)
            /* RB+ doesn't work with dual source blending, logic op, and RESOLVE. */
   if (blend->dual_src_blend || logicop_enable || mode == V_028808_CB_RESOLVE)
               si_pm4_set_reg(pm4, R_028808_CB_COLOR_CONTROL, color_control);
   si_pm4_finalize(pm4);
      }
      static void *si_create_blend_state(struct pipe_context *ctx, const struct pipe_blend_state *state)
   {
         }
      static bool si_check_blend_dst_sampler_noop(struct si_context *sctx)
   {
      if (sctx->framebuffer.state.nr_cbufs == 1) {
               if (unlikely(sel->info.writes_1_if_tex_is_1 == 0xff)) {
      /* Wait for the shader to be ready. */
                           /* Determine if this fragment shader always writes vec4(1) if a specific texture
   * is all 1s.
   */
   float in[4] = { 1.0, 1.0, 1.0, 1.0 };
   float out[4];
   int texunit;
   if (si_nir_is_output_const_if_tex_is_const(nir, in, out, &texunit) &&
      !memcmp(in, out, 4 * sizeof(float))) {
      } else {
                              if (sel->info.writes_1_if_tex_is_1 &&
      sel->info.writes_1_if_tex_is_1 != 0xff) {
   /* Now check if the texture is cleared to 1 */
   int unit = sctx->shader.ps.cso->info.writes_1_if_tex_is_1 - 1;
   struct si_samplers *samp = &sctx->samplers[PIPE_SHADER_FRAGMENT];
   if ((1u << unit) & samp->enabled_mask) {
      struct si_texture* tex = (struct si_texture*) samp->views[unit]->texture;
   if (tex->is_depth &&
      tex->depth_cleared_level_mask & BITFIELD_BIT(samp->views[unit]->u.tex.first_level) &&
   tex->depth_clear_value[0] == 1) {
      }
                        }
      static void si_draw_blend_dst_sampler_noop(struct pipe_context *ctx,
                                             if (!si_check_blend_dst_sampler_noop(sctx))
               }
      static void si_draw_vstate_blend_dst_sampler_noop(struct pipe_context *ctx,
                                             if (!si_check_blend_dst_sampler_noop(sctx))
               }
      static void si_bind_blend_state(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_state_blend *old_blend = sctx->queued.named.blend;
            if (!blend)
                     if (old_blend->cb_target_mask != blend->cb_target_mask ||
      old_blend->dual_src_blend != blend->dual_src_blend ||
   (old_blend->dcc_msaa_corruption_4bit != blend->dcc_msaa_corruption_4bit &&
   sctx->framebuffer.has_dcc_msaa))
         if ((sctx->screen->info.has_export_conflict_bug &&
      old_blend->blend_enable_4bit != blend->blend_enable_4bit) ||
   (sctx->occlusion_query_mode == SI_OCCLUSION_QUERY_MODE_PRECISE_BOOLEAN &&
   !!old_blend->cb_target_mask != !!blend->cb_target_enabled_4bit))
         if (old_blend->cb_target_enabled_4bit != blend->cb_target_enabled_4bit ||
      old_blend->alpha_to_coverage != blend->alpha_to_coverage ||
   old_blend->alpha_to_one != blend->alpha_to_one ||
   old_blend->dual_src_blend != blend->dual_src_blend ||
   old_blend->blend_enable_4bit != blend->blend_enable_4bit ||
   old_blend->need_src_alpha_4bit != blend->need_src_alpha_4bit) {
   si_ps_key_update_framebuffer_blend_rasterizer(sctx);
   si_update_ps_inputs_read_or_disabled(sctx);
               if (sctx->screen->dpbb_allowed &&
      (old_blend->alpha_to_coverage != blend->alpha_to_coverage ||
   old_blend->blend_enable_4bit != blend->blend_enable_4bit ||
   old_blend->cb_target_enabled_4bit != blend->cb_target_enabled_4bit))
         if (sctx->screen->info.has_out_of_order_rast &&
      ((old_blend->blend_enable_4bit != blend->blend_enable_4bit ||
      old_blend->cb_target_enabled_4bit != blend->cb_target_enabled_4bit ||
   old_blend->commutative_4bit != blend->commutative_4bit ||
            /* RB+ depth-only rendering. See the comment where we set rbplus_depth_only_opt for more
   * information.
   */
   if (sctx->screen->info.rbplus_allowed &&
      !!old_blend->cb_target_mask != !!blend->cb_target_mask) {
   sctx->framebuffer.dirty_cbufs |= BITFIELD_BIT(0);
               if (likely(!radeon_uses_secure_bos(sctx->ws))) {
      if (unlikely(blend->allows_noop_optimization)) {
      si_install_draw_wrapper(sctx, si_draw_blend_dst_sampler_noop,
      } else {
               }
      static void si_delete_blend_state(struct pipe_context *ctx, void *state)
   {
               if (sctx->queued.named.blend == state)
               }
      static void si_set_blend_color(struct pipe_context *ctx, const struct pipe_blend_color *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
            sctx->blend_color = *state;
   sctx->blend_color_any_nonzeros = memcmp(state, &zeros, sizeof(*state)) != 0;
      }
      static void si_emit_blend_color(struct si_context *sctx, unsigned index)
   {
               radeon_begin(cs);
   radeon_set_context_reg_seq(R_028414_CB_BLEND_RED, 4);
   radeon_emit_array((uint32_t *)sctx->blend_color.color, 4);
      }
      /*
   * Clipping
   */
      static void si_set_clip_state(struct pipe_context *ctx, const struct pipe_clip_state *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct pipe_constant_buffer cb;
            if (memcmp(&sctx->clip_state, state, sizeof(*state)) == 0)
            sctx->clip_state = *state;
   sctx->clip_state_any_nonzeros = memcmp(state, &zeros, sizeof(*state)) != 0;
            cb.buffer = NULL;
   cb.user_buffer = state->ucp;
   cb.buffer_offset = 0;
   cb.buffer_size = 4 * 4 * 8;
      }
      static void si_emit_clip_state(struct si_context *sctx, unsigned index)
   {
               radeon_begin(cs);
   radeon_set_context_reg_seq(R_0285BC_PA_CL_UCP_0_X, 6 * 4);
   radeon_emit_array((uint32_t *)sctx->clip_state.ucp, 6 * 4);
      }
      static void si_emit_clip_regs(struct si_context *sctx, unsigned index)
   {
      struct si_shader *vs = si_get_vs(sctx)->current;
   struct si_shader_selector *vs_sel = vs->selector;
   struct si_shader_info *info = &vs_sel->info;
   struct si_state_rasterizer *rs = sctx->queued.named.rasterizer;
   bool window_space = vs_sel->stage == MESA_SHADER_VERTEX ?
         unsigned clipdist_mask = vs_sel->info.clipdist_mask;
   unsigned ucp_mask = clipdist_mask ? 0 : rs->clip_plane_enable & SI_USER_CLIP_PLANE_MASK;
            /* Clip distances on points have no effect, so need to be implemented
   * as cull distances. This applies for the clipvertex case as well.
   *
   * Setting this for primitives other than points should have no adverse
   * effects.
   */
   clipdist_mask &= rs->clip_plane_enable;
            unsigned pa_cl_cntl = S_02881C_BYPASS_VTX_RATE_COMBINER(sctx->gfx_level >= GFX10_3 &&
                        radeon_begin(&sctx->gfx_cs);
   radeon_opt_set_context_reg(sctx, R_02881C_PA_CL_VS_OUT_CNTL, SI_TRACKED_PA_CL_VS_OUT_CNTL,
         radeon_opt_set_context_reg(sctx, R_028810_PA_CL_CLIP_CNTL, SI_TRACKED_PA_CL_CLIP_CNTL,
            }
      /*
   * inferred state between framebuffer and rasterizer
   */
   static void si_update_poly_offset_state(struct si_context *sctx)
   {
               if (!rs->uses_poly_offset || !sctx->framebuffer.state.zsbuf) {
      si_pm4_bind_state(sctx, poly_offset, NULL);
               /* Use the user format, not db_render_format, so that the polygon
   * offset behaves as expected by applications.
   */
   switch (sctx->framebuffer.state.zsbuf->texture->format) {
   case PIPE_FORMAT_Z16_UNORM:
      si_pm4_bind_state(sctx, poly_offset, &rs->pm4_poly_offset[0]);
      default: /* 24-bit */
      si_pm4_bind_state(sctx, poly_offset, &rs->pm4_poly_offset[1]);
      case PIPE_FORMAT_Z32_FLOAT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      si_pm4_bind_state(sctx, poly_offset, &rs->pm4_poly_offset[2]);
         }
      /*
   * Rasterizer
   */
      static uint32_t si_translate_fill(uint32_t func)
   {
      switch (func) {
   case PIPE_POLYGON_MODE_FILL:
         case PIPE_POLYGON_MODE_LINE:
         case PIPE_POLYGON_MODE_POINT:
         default:
      assert(0);
         }
      static void *si_create_rs_state(struct pipe_context *ctx, const struct pipe_rasterizer_state *state)
   {
      struct si_screen *sscreen = ((struct si_context *)ctx)->screen;
   struct si_state_rasterizer *rs = CALLOC_STRUCT(si_state_rasterizer);
   struct si_pm4_state *pm4 = &rs->pm4;
   unsigned tmp, i;
            if (!rs) {
                           rs->scissor_enable = state->scissor;
   rs->clip_halfz = state->clip_halfz;
   rs->two_side = state->light_twoside;
   rs->multisample_enable = state->multisample;
   rs->force_persample_interp = state->force_persample_interp;
   rs->clip_plane_enable = state->clip_plane_enable;
   rs->half_pixel_center = state->half_pixel_center;
   rs->line_stipple_enable = state->line_stipple_enable;
   rs->poly_stipple_enable = state->poly_stipple_enable;
   rs->line_smooth = state->line_smooth;
   rs->line_width = state->line_width;
   rs->poly_smooth = state->poly_smooth;
   rs->point_smooth = state->point_smooth;
   rs->uses_poly_offset = state->offset_point || state->offset_line || state->offset_tri;
   rs->clamp_fragment_color = state->clamp_fragment_color;
   rs->clamp_vertex_color = state->clamp_vertex_color;
   rs->flatshade = state->flatshade;
   rs->flatshade_first = state->flatshade_first;
   rs->sprite_coord_enable = state->sprite_coord_enable;
   rs->rasterizer_discard = state->rasterizer_discard;
   rs->bottom_edge_rule = state->bottom_edge_rule;
   rs->polygon_mode_is_lines =
      (state->fill_front == PIPE_POLYGON_MODE_LINE && !(state->cull_face & PIPE_FACE_FRONT)) ||
      rs->polygon_mode_is_points =
      (state->fill_front == PIPE_POLYGON_MODE_POINT && !(state->cull_face & PIPE_FACE_FRONT)) ||
      rs->pa_sc_line_stipple = state->line_stipple_enable ?
               /* TODO: implement line stippling with perpendicular end caps. */
   /* Line width > 2 is an internal recommendation. */
   rs->perpendicular_end_caps = state->multisample &&
            rs->pa_cl_clip_cntl = S_028810_DX_CLIP_SPACE_DEF(state->clip_halfz) |
                              rs->ngg_cull_flags_tris = SI_NGG_CULL_TRIANGLES |
                  rs->ngg_cull_flags_lines = SI_NGG_CULL_LINES |
                  if (rs->rasterizer_discard) {
      rs->ngg_cull_flags_tris |= SI_NGG_CULL_FRONT_FACE |
            } else {
               if (!state->front_ccw) {
      cull_front = !!(state->cull_face & PIPE_FACE_FRONT);
      } else {
      cull_back = !!(state->cull_face & PIPE_FACE_FRONT);
               if (cull_front) {
      rs->ngg_cull_flags_tris |= SI_NGG_CULL_FRONT_FACE;
               if (cull_back) {
      rs->ngg_cull_flags_tris |= SI_NGG_CULL_BACK_FACE;
                  unsigned spi_interp_control_0 =
      S_0286D4_FLAT_SHADE_ENA(1) |
   S_0286D4_PNT_SPRITE_ENA(state->point_quad_rasterization) |
   S_0286D4_PNT_SPRITE_OVRD_X(V_0286D4_SPI_PNT_SPRITE_SEL_S) |
   S_0286D4_PNT_SPRITE_OVRD_Y(V_0286D4_SPI_PNT_SPRITE_SEL_T) |
   S_0286D4_PNT_SPRITE_OVRD_Z(V_0286D4_SPI_PNT_SPRITE_SEL_0) |
   S_0286D4_PNT_SPRITE_OVRD_W(V_0286D4_SPI_PNT_SPRITE_SEL_1) |
                  /* point size 12.4 fixed point */
   tmp = (unsigned)(state->point_size * 8.0);
            if (state->point_size_per_vertex) {
      psize_min = util_get_min_point_size(state);
      } else {
      /* Force the point size to be as if the vertex output was disabled. */
   psize_min = state->point_size;
      }
            /* Divide by two, because 0.5 = 1 pixel. */
   si_pm4_set_reg(pm4, R_028A04_PA_SU_POINT_MINMAX,
               si_pm4_set_reg(pm4, R_028A08_PA_SU_LINE_CNTL,
            si_pm4_set_reg(pm4, R_028A48_PA_SC_MODE_CNTL_0,
                  S_028A48_LINE_STIPPLE_ENABLE(state->line_stipple_enable) |
   S_028A48_MSAA_ENABLE(state->multisample || state->poly_smooth ||
         bool polygon_mode_enabled =
      (state->fill_front != PIPE_POLYGON_MODE_FILL && !(state->cull_face & PIPE_FACE_FRONT)) ||
         si_pm4_set_reg(pm4, R_028814_PA_SU_SC_MODE_CNTL,
                  S_028814_PROVOKING_VTX_LAST(!state->flatshade_first) |
   S_028814_CULL_FRONT((state->cull_face & PIPE_FACE_FRONT) ? 1 : 0) |
   S_028814_CULL_BACK((state->cull_face & PIPE_FACE_BACK) ? 1 : 0) |
   S_028814_FACE(!state->front_ccw) |
   S_028814_POLY_OFFSET_FRONT_ENABLE(util_get_offset(state, state->fill_front)) |
   S_028814_POLY_OFFSET_BACK_ENABLE(util_get_offset(state, state->fill_back)) |
   S_028814_POLY_OFFSET_PARA_ENABLE(state->offset_point || state->offset_line) |
   S_028814_POLY_MODE(polygon_mode_enabled) |
   S_028814_POLYMODE_FRONT_PTYPE(si_translate_fill(state->fill_front)) |
   S_028814_POLYMODE_BACK_PTYPE(si_translate_fill(state->fill_back)) |
   /* this must be set if POLY_MODE or PERPENDICULAR_ENDCAP_ENA is set */
         if (sscreen->info.gfx_level >= GFX10) {
      si_pm4_set_reg(pm4, R_028838_PA_CL_NGG_CNTL,
                           if (state->bottom_edge_rule) {
      /* OpenGL windows should set this. */
   si_pm4_set_reg(pm4, R_028230_PA_SC_EDGERULE,
                  S_028230_ER_TRI(0xA) |
   S_028230_ER_POINT(0x5) |
   S_028230_ER_RECT(0x9) |
   S_028230_ER_LINE_LR(0x29) |
   } else {
      /* OpenGL FBOs and Direct3D should set this. */
   si_pm4_set_reg(pm4, R_028230_PA_SC_EDGERULE,
                  S_028230_ER_TRI(0xA) |
   S_028230_ER_POINT(0xA) |
   S_028230_ER_RECT(0xA) |
   S_028230_ER_LINE_LR(0x1A) |
   }
            if (!rs->uses_poly_offset)
            rs->pm4_poly_offset = CALLOC(3, sizeof(struct si_pm4_state));
   if (!rs->pm4_poly_offset) {
      FREE(rs);
               /* Precalculate polygon offset states for 16-bit, 24-bit, and 32-bit zbuffers. */
   for (i = 0; i < 3; i++) {
      struct si_pm4_state *pm4 = &rs->pm4_poly_offset[i];
   float offset_units = state->offset_units;
   float offset_scale = state->offset_scale * 16.0f;
                     if (!state->offset_units_unscaled) {
      switch (i) {
   case 0: /* 16-bit zbuffer */
      offset_units *= 4.0f;
   pa_su_poly_offset_db_fmt_cntl = S_028B78_POLY_OFFSET_NEG_NUM_DB_BITS(-16);
      case 1: /* 24-bit zbuffer */
      offset_units *= 2.0f;
   pa_su_poly_offset_db_fmt_cntl = S_028B78_POLY_OFFSET_NEG_NUM_DB_BITS(-24);
      case 2: /* 32-bit zbuffer */
      offset_units *= 1.0f;
   pa_su_poly_offset_db_fmt_cntl =
                        si_pm4_set_reg(pm4, R_028B78_PA_SU_POLY_OFFSET_DB_FMT_CNTL, pa_su_poly_offset_db_fmt_cntl);
   si_pm4_set_reg(pm4, R_028B7C_PA_SU_POLY_OFFSET_CLAMP, fui(state->offset_clamp));
   si_pm4_set_reg(pm4, R_028B80_PA_SU_POLY_OFFSET_FRONT_SCALE, fui(offset_scale));
   si_pm4_set_reg(pm4, R_028B84_PA_SU_POLY_OFFSET_FRONT_OFFSET, fui(offset_units));
   si_pm4_set_reg(pm4, R_028B88_PA_SU_POLY_OFFSET_BACK_SCALE, fui(offset_scale));
   si_pm4_set_reg(pm4, R_028B8C_PA_SU_POLY_OFFSET_BACK_OFFSET, fui(offset_units));
                  }
      static void si_bind_rs_state(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_state_rasterizer *old_rs = (struct si_state_rasterizer *)sctx->queued.named.rasterizer;
            if (!rs)
            if (old_rs->multisample_enable != rs->multisample_enable) {
               /* Update the small primitive filter workaround if necessary. */
   if (sctx->screen->info.has_small_prim_filter_sample_loc_bug && sctx->framebuffer.nr_samples > 1)
            /* NGG cull state uses multisample_enable. */
   if (sctx->screen->use_ngg_culling)
               if (old_rs->perpendicular_end_caps != rs->perpendicular_end_caps)
            if (sctx->screen->use_ngg_culling &&
      (old_rs->half_pixel_center != rs->half_pixel_center ||
   old_rs->line_width != rs->line_width))
                  si_pm4_bind_state(sctx, rasterizer, rs);
            if (old_rs->scissor_enable != rs->scissor_enable)
            if (old_rs->line_width != rs->line_width || old_rs->max_point_size != rs->max_point_size ||
      old_rs->half_pixel_center != rs->half_pixel_center)
         if (old_rs->clip_halfz != rs->clip_halfz)
            if (old_rs->clip_plane_enable != rs->clip_plane_enable ||
      old_rs->pa_cl_clip_cntl != rs->pa_cl_clip_cntl)
         if (old_rs->sprite_coord_enable != rs->sprite_coord_enable ||
      old_rs->flatshade != rs->flatshade)
         if (sctx->screen->dpbb_allowed && (old_rs->bottom_edge_rule != rs->bottom_edge_rule))
            if (old_rs->clip_plane_enable != rs->clip_plane_enable ||
      old_rs->rasterizer_discard != rs->rasterizer_discard ||
   old_rs->sprite_coord_enable != rs->sprite_coord_enable ||
   old_rs->flatshade != rs->flatshade || old_rs->two_side != rs->two_side ||
   old_rs->multisample_enable != rs->multisample_enable ||
   old_rs->poly_stipple_enable != rs->poly_stipple_enable ||
   old_rs->poly_smooth != rs->poly_smooth || old_rs->line_smooth != rs->line_smooth ||
   old_rs->point_smooth != rs->point_smooth ||
   old_rs->clamp_fragment_color != rs->clamp_fragment_color ||
   old_rs->force_persample_interp != rs->force_persample_interp ||
   old_rs->polygon_mode_is_points != rs->polygon_mode_is_points) {
   si_ps_key_update_framebuffer_blend_rasterizer(sctx);
   si_ps_key_update_rasterizer(sctx);
   si_ps_key_update_framebuffer_rasterizer_sample_shading(sctx);
   si_update_ps_inputs_read_or_disabled(sctx);
               if (old_rs->line_smooth != rs->line_smooth ||
      old_rs->poly_smooth != rs->poly_smooth ||
   old_rs->point_smooth != rs->point_smooth ||
   old_rs->poly_stipple_enable != rs->poly_stipple_enable ||
   old_rs->flatshade != rs->flatshade)
         if (old_rs->flatshade_first != rs->flatshade_first)
      }
      static void si_delete_rs_state(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
            if (sctx->queued.named.rasterizer == state)
            FREE(rs->pm4_poly_offset);
      }
      /*
   * inferred state between dsa and stencil ref
   */
   static void si_emit_stencil_ref(struct si_context *sctx, unsigned index)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
   struct pipe_stencil_ref *ref = &sctx->stencil_ref.state;
            radeon_begin(cs);
   radeon_set_context_reg_seq(R_028430_DB_STENCILREFMASK, 2);
   radeon_emit(S_028430_STENCILTESTVAL(ref->ref_value[0]) |
               S_028430_STENCILMASK(dsa->valuemask[0]) |
   radeon_emit(S_028434_STENCILTESTVAL_BF(ref->ref_value[1]) |
               S_028434_STENCILMASK_BF(dsa->valuemask[1]) |
      }
      static void si_set_stencil_ref(struct pipe_context *ctx, const struct pipe_stencil_ref state)
   {
               if (memcmp(&sctx->stencil_ref.state, &state, sizeof(state)) == 0)
            sctx->stencil_ref.state = state;
      }
      /*
   * DSA
   */
      static uint32_t si_translate_stencil_op(int s_op)
   {
      switch (s_op) {
   case PIPE_STENCIL_OP_KEEP:
         case PIPE_STENCIL_OP_ZERO:
         case PIPE_STENCIL_OP_REPLACE:
         case PIPE_STENCIL_OP_INCR:
         case PIPE_STENCIL_OP_DECR:
         case PIPE_STENCIL_OP_INCR_WRAP:
         case PIPE_STENCIL_OP_DECR_WRAP:
         case PIPE_STENCIL_OP_INVERT:
         default:
      PRINT_ERR("Unknown stencil op %d", s_op);
   assert(0);
      }
      }
      static bool si_order_invariant_stencil_op(enum pipe_stencil_op op)
   {
      /* REPLACE is normally order invariant, except when the stencil
   * reference value is written by the fragment shader. Tracking this
   * interaction does not seem worth the effort, so be conservative. */
      }
      /* Compute whether, assuming Z writes are disabled, this stencil state is order
   * invariant in the sense that the set of passing fragments as well as the
   * final stencil buffer result does not depend on the order of fragments. */
   static bool si_order_invariant_stencil_state(const struct pipe_stencil_state *state)
   {
      return !state->enabled || !state->writemask ||
         /* The following assumes that Z writes are disabled. */
   (state->func == PIPE_FUNC_ALWAYS && si_order_invariant_stencil_op(state->zpass_op) &&
      }
      static void *si_create_dsa_state(struct pipe_context *ctx,
         {
      struct si_state_dsa *dsa = CALLOC_STRUCT(si_state_dsa);
   struct si_pm4_state *pm4 = &dsa->pm4;
   unsigned db_depth_control;
            if (!dsa) {
                           dsa->stencil_ref.valuemask[0] = state->stencil[0].valuemask;
   dsa->stencil_ref.valuemask[1] = state->stencil[1].valuemask;
   dsa->stencil_ref.writemask[0] = state->stencil[0].writemask;
            db_depth_control =
      S_028800_Z_ENABLE(state->depth_enabled) | S_028800_Z_WRITE_ENABLE(state->depth_writemask) |
         /* stencil */
   if (state->stencil[0].enabled) {
      db_depth_control |= S_028800_STENCIL_ENABLE(1);
   db_depth_control |= S_028800_STENCILFUNC(state->stencil[0].func);
   db_stencil_control |=
         db_stencil_control |=
         db_stencil_control |=
            if (state->stencil[1].enabled) {
      db_depth_control |= S_028800_BACKFACE_ENABLE(1);
   db_depth_control |= S_028800_STENCILFUNC_BF(state->stencil[1].func);
   db_stencil_control |=
         db_stencil_control |=
         db_stencil_control |=
                  /* alpha */
   if (state->alpha_enabled) {
               si_pm4_set_reg(pm4, R_00B030_SPI_SHADER_USER_DATA_PS_0 + SI_SGPR_ALPHA_REF * 4,
      } else {
                  si_pm4_set_reg(pm4, R_028800_DB_DEPTH_CONTROL, db_depth_control);
   if (state->stencil[0].enabled)
         if (state->depth_bounds_test) {
      si_pm4_set_reg(pm4, R_028020_DB_DEPTH_BOUNDS_MIN, fui(state->depth_bounds_min));
      }
            dsa->depth_enabled = state->depth_enabled;
   dsa->depth_write_enabled = state->depth_enabled && state->depth_writemask;
   dsa->stencil_enabled = state->stencil[0].enabled;
   dsa->stencil_write_enabled =
                  bool zfunc_is_ordered =
      state->depth_func == PIPE_FUNC_NEVER || state->depth_func == PIPE_FUNC_LESS ||
   state->depth_func == PIPE_FUNC_LEQUAL || state->depth_func == PIPE_FUNC_GREATER ||
         bool nozwrite_and_order_invariant_stencil =
      !dsa->db_can_write ||
   (!dsa->depth_write_enabled && si_order_invariant_stencil_state(&state->stencil[0]) &&
         dsa->order_invariance[1].zs =
                  dsa->order_invariance[1].pass_set =
      nozwrite_and_order_invariant_stencil ||
   (!dsa->stencil_write_enabled &&
      dsa->order_invariance[0].pass_set =
      !dsa->depth_write_enabled ||
            }
      static void si_bind_dsa_state(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_state_dsa *old_dsa = sctx->queued.named.dsa;
            if (!dsa)
                     if (memcmp(&dsa->stencil_ref, &sctx->stencil_ref.dsa_part,
            sctx->stencil_ref.dsa_part = dsa->stencil_ref;
               if (old_dsa->alpha_func != dsa->alpha_func) {
      si_ps_key_update_dsa(sctx);
   si_update_ps_inputs_read_or_disabled(sctx);
               if (sctx->occlusion_query_mode == SI_OCCLUSION_QUERY_MODE_PRECISE_BOOLEAN &&
      (old_dsa->depth_enabled != dsa->depth_enabled ||
   old_dsa->depth_write_enabled != dsa->depth_write_enabled))
         if (sctx->screen->dpbb_allowed && ((old_dsa->depth_enabled != dsa->depth_enabled ||
                        if (sctx->screen->info.has_out_of_order_rast &&
      (memcmp(old_dsa->order_invariance, dsa->order_invariance,
         }
      static void si_delete_dsa_state(struct pipe_context *ctx, void *state)
   {
               if (sctx->queued.named.dsa == state)
               }
      static void *si_create_db_flush_dsa(struct si_context *sctx)
   {
                  }
      /* DB RENDER STATE */
      static void si_set_active_query_state(struct pipe_context *ctx, bool enable)
   {
               /* Pipeline stat & streamout queries. */
   if (enable) {
      /* Disable pipeline stats if there are no active queries. */
   if (sctx->num_hw_pipestat_streamout_queries) {
      sctx->flags &= ~SI_CONTEXT_STOP_PIPELINE_STATS;
   sctx->flags |= SI_CONTEXT_START_PIPELINE_STATS;
         } else {
      if (sctx->num_hw_pipestat_streamout_queries) {
      sctx->flags &= ~SI_CONTEXT_START_PIPELINE_STATS;
   sctx->flags |= SI_CONTEXT_STOP_PIPELINE_STATS;
                  /* Occlusion queries. */
   if (sctx->occlusion_queries_disabled != !enable) {
      sctx->occlusion_queries_disabled = !enable;
         }
      void si_save_qbo_state(struct si_context *sctx, struct si_qbo_state *st)
   {
         }
      void si_restore_qbo_state(struct si_context *sctx, struct si_qbo_state *st)
   {
         }
      static void si_emit_db_render_state(struct si_context *sctx, unsigned index)
   {
               /* DB_RENDER_CONTROL */
   if (sctx->dbcb_depth_copy_enabled || sctx->dbcb_stencil_copy_enabled) {
      db_render_control = S_028000_DEPTH_COPY(sctx->dbcb_depth_copy_enabled) |
            } else if (sctx->db_flush_depth_inplace || sctx->db_flush_stencil_inplace) {
      db_render_control = S_028000_DEPTH_COMPRESS_DISABLE(sctx->db_flush_depth_inplace) |
      } else {
      db_render_control = S_028000_DEPTH_CLEAR_ENABLE(sctx->db_depth_clear) |
               if (sctx->gfx_level >= GFX11) {
               if (sctx->screen->info.has_dedicated_vram) {
      if (sctx->framebuffer.nr_samples == 8)
         else if (sctx->framebuffer.nr_samples == 4)
      } else {
      if (sctx->framebuffer.nr_samples == 8)
               /* TODO: We may want to disable this workaround for future chips. */
   if (sctx->framebuffer.nr_samples >= 4) {
      if (max_allowed_tiles_in_wave)
         else
                           /* DB_COUNT_CONTROL (occlusion queries) */
   if (sctx->occlusion_query_mode == SI_OCCLUSION_QUERY_MODE_DISABLE ||
      sctx->occlusion_queries_disabled) {
   /* Occlusion queries disabled. */
   if (sctx->gfx_level >= GFX7)
         else
      } else {
      /* Occlusion queries enabled. */
            if (sctx->gfx_level >= GFX7) {
      db_count_control |= S_028004_ZPASS_ENABLE(1) |
                     if (sctx->occlusion_query_mode == SI_OCCLUSION_QUERY_MODE_PRECISE_INTEGER ||
      /* Boolean occlusion queries must set PERFECT_ZPASS_COUNTS for depth-only rendering
   * without depth writes or when depth testing is disabled. */
   (sctx->occlusion_query_mode == SI_OCCLUSION_QUERY_MODE_PRECISE_BOOLEAN &&
   (!sctx->queued.named.dsa->depth_enabled ||
      (!sctx->queued.named.blend->cb_target_mask &&
            if (sctx->gfx_level >= GFX10 &&
      sctx->occlusion_query_mode != SI_OCCLUSION_QUERY_MODE_CONSERVATIVE_BOOLEAN)
            /* This should always be set on GFX11. */
   if (sctx->gfx_level >= GFX11)
                     if (sctx->screen->info.has_export_conflict_bug &&
      sctx->queued.named.blend->blend_enable_4bit &&
   si_get_num_coverage_samples(sctx) == 1) {
   db_shader_control |= S_02880C_OVERRIDE_INTRINSIC_RATE_ENABLE(1) |
               if (sctx->gfx_level >= GFX10_3) {
      /* Variable rate shading. */
            if (sctx->allow_flat_shading) {
      mode = V_028064_SC_VRS_COMB_MODE_OVERRIDE;
      } else {
      /* If the shader is using discard, turn off coarse shading because discarding at 2x2 pixel
   * granularity degrades quality too much.
   *
   * The shader writes the VRS rate and we either pass it through or do MIN(shader, 1x1)
   * to disable coarse shading.
   */
   mode = sctx->screen->options.vrs2x2 && G_02880C_KILL_ENABLE(db_shader_control) ?
                     if (sctx->gfx_level >= GFX11) {
      vrs_override_cntl = S_0283D0_VRS_OVERRIDE_RATE_COMBINER_MODE(mode) |
      } else {
      vrs_override_cntl = S_028064_VRS_OVERRIDE_RATE_COMBINER_MODE(mode) |
                        radeon_begin(&sctx->gfx_cs);
   radeon_opt_set_context_reg2(sctx, R_028000_DB_RENDER_CONTROL, SI_TRACKED_DB_RENDER_CONTROL,
            /* DB_RENDER_OVERRIDE2 */
   radeon_opt_set_context_reg(
      sctx, R_028010_DB_RENDER_OVERRIDE2, SI_TRACKED_DB_RENDER_OVERRIDE2,
   S_028010_DISABLE_ZMASK_EXPCLEAR_OPTIMIZATION(sctx->db_depth_disable_expclear) |
   S_028010_DISABLE_SMEM_EXPCLEAR_OPTIMIZATION(sctx->db_stencil_disable_expclear) |
   S_028010_DECOMPRESS_Z_ON_FLUSH(sctx->framebuffer.nr_samples >= 4) |
         radeon_opt_set_context_reg(sctx, R_02880C_DB_SHADER_CONTROL, SI_TRACKED_DB_SHADER_CONTROL,
            if (sctx->gfx_level >= GFX11) {
      radeon_opt_set_context_reg(sctx, R_0283D0_PA_SC_VRS_OVERRIDE_CNTL,
      } else if (sctx->gfx_level >= GFX10_3) {
      radeon_opt_set_context_reg(sctx, R_028064_DB_VRS_OVERRIDE_CNTL,
                  }
      /*
   * format translation
   */
      static uint32_t si_colorformat_endian_swap(uint32_t colorformat)
   {
      if (UTIL_ARCH_BIG_ENDIAN) {
      switch (colorformat) {
   /* 8-bit buffers. */
   case V_028C70_COLOR_8:
            /* 16-bit buffers. */
   case V_028C70_COLOR_5_6_5:
   case V_028C70_COLOR_1_5_5_5:
   case V_028C70_COLOR_4_4_4_4:
   case V_028C70_COLOR_16:
   case V_028C70_COLOR_8_8:
            /* 32-bit buffers. */
   case V_028C70_COLOR_8_8_8_8:
   case V_028C70_COLOR_2_10_10_10:
   case V_028C70_COLOR_10_10_10_2:
   case V_028C70_COLOR_8_24:
   case V_028C70_COLOR_24_8:
   case V_028C70_COLOR_16_16:
            /* 64-bit buffers. */
   case V_028C70_COLOR_16_16_16_16:
            case V_028C70_COLOR_32_32:
            /* 128-bit buffers. */
   case V_028C70_COLOR_32_32_32_32:
         default:
            } else {
            }
      static uint32_t si_translate_dbformat(enum pipe_format format)
   {
      switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
         case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
         case PIPE_FORMAT_Z32_FLOAT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
         default:
            }
      /*
   * Texture translation
   */
      static uint32_t si_translate_texformat(struct pipe_screen *screen, enum pipe_format format,
               {
      struct si_screen *sscreen = (struct si_screen *)screen;
   bool uniform = true;
                     /* Colorspace (return non-RGB formats directly). */
   switch (desc->colorspace) {
   /* Depth stencil formats */
   case UTIL_FORMAT_COLORSPACE_ZS:
      switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
         case PIPE_FORMAT_X24S8_UINT:
   case PIPE_FORMAT_S8X24_UINT:
      /*
   * Implemented as an 8_8_8_8 data format to fix texture
   * gathers in stencil sampling. This affects at least
   * GL45-CTS.texture_cube_map_array.sampling on GFX8.
   */
                  if (format == PIPE_FORMAT_X24S8_UINT)
         else
      case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
         case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
         case PIPE_FORMAT_S8_UINT:
         case PIPE_FORMAT_Z32_FLOAT:
         case PIPE_FORMAT_X32_S8X24_UINT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
         default:
               case UTIL_FORMAT_COLORSPACE_YUV:
            case UTIL_FORMAT_COLORSPACE_SRGB:
      if (desc->nr_channels != 4 && desc->nr_channels != 1)
               default:
                  if (desc->layout == UTIL_FORMAT_LAYOUT_RGTC) {
      switch (format) {
   case PIPE_FORMAT_RGTC1_SNORM:
   case PIPE_FORMAT_LATC1_SNORM:
   case PIPE_FORMAT_RGTC1_UNORM:
   case PIPE_FORMAT_LATC1_UNORM:
         case PIPE_FORMAT_RGTC2_SNORM:
   case PIPE_FORMAT_LATC2_SNORM:
   case PIPE_FORMAT_RGTC2_UNORM:
   case PIPE_FORMAT_LATC2_UNORM:
         default:
                     if (desc->layout == UTIL_FORMAT_LAYOUT_ETC &&
      (sscreen->info.family == CHIP_STONEY || sscreen->info.family == CHIP_VEGA10 ||
   sscreen->info.family == CHIP_RAVEN || sscreen->info.family == CHIP_RAVEN2)) {
   switch (format) {
   case PIPE_FORMAT_ETC1_RGB8:
   case PIPE_FORMAT_ETC2_RGB8:
   case PIPE_FORMAT_ETC2_SRGB8:
         case PIPE_FORMAT_ETC2_RGB8A1:
   case PIPE_FORMAT_ETC2_SRGB8A1:
         case PIPE_FORMAT_ETC2_RGBA8:
   case PIPE_FORMAT_ETC2_SRGBA8:
         case PIPE_FORMAT_ETC2_R11_UNORM:
   case PIPE_FORMAT_ETC2_R11_SNORM:
         case PIPE_FORMAT_ETC2_RG11_UNORM:
   case PIPE_FORMAT_ETC2_RG11_SNORM:
         default:
                     if (desc->layout == UTIL_FORMAT_LAYOUT_BPTC) {
      switch (format) {
   case PIPE_FORMAT_BPTC_RGBA_UNORM:
   case PIPE_FORMAT_BPTC_SRGBA:
         case PIPE_FORMAT_BPTC_RGB_FLOAT:
   case PIPE_FORMAT_BPTC_RGB_UFLOAT:
         default:
                     if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
      switch (format) {
   case PIPE_FORMAT_R8G8_B8G8_UNORM:
   case PIPE_FORMAT_G8R8_B8R8_UNORM:
         case PIPE_FORMAT_G8R8_G8B8_UNORM:
   case PIPE_FORMAT_R8G8_R8B8_UNORM:
         default:
                     if (desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
      switch (format) {
   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT1_SRGB:
   case PIPE_FORMAT_DXT1_SRGBA:
         case PIPE_FORMAT_DXT3_RGBA:
   case PIPE_FORMAT_DXT3_SRGBA:
         case PIPE_FORMAT_DXT5_RGBA:
   case PIPE_FORMAT_DXT5_SRGBA:
         default:
                     if (format == PIPE_FORMAT_R9G9B9E5_FLOAT) {
         } else if (format == PIPE_FORMAT_R11G11B10_FLOAT) {
                  /* Other "OTHER" layouts are unsupported. */
   if (desc->layout == UTIL_FORMAT_LAYOUT_OTHER)
            /* hw cannot support mixed formats (except depth/stencil, since only
   * depth is read).*/
   if (desc->is_mixed && desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS)
            if (first_non_void < 0 || first_non_void > 3)
            /* Reject SCALED formats because we don't implement them for CB and do the same for texturing. */
   if ((desc->channel[first_non_void].type == UTIL_FORMAT_TYPE_UNSIGNED ||
      desc->channel[first_non_void].type == UTIL_FORMAT_TYPE_SIGNED) &&
   !desc->channel[first_non_void].normalized &&
   !desc->channel[first_non_void].pure_integer)
         /* Reject unsupported 32_*NORM and FIXED formats. */
   if (desc->channel[first_non_void].size == 32 &&
      (desc->channel[first_non_void].normalized ||
   desc->channel[first_non_void].type == UTIL_FORMAT_TYPE_FIXED))
         /* This format fails on Gfx8/Carrizo´. */
   if (format == PIPE_FORMAT_A8R8_UNORM)
            /* See whether the components are of the same size. */
   for (i = 1; i < desc->nr_channels; i++) {
                  /* Non-uniform formats. */
   if (!uniform) {
      switch (desc->nr_channels) {
   case 3:
      if (desc->channel[0].size == 5 && desc->channel[1].size == 6 &&
      desc->channel[2].size == 5) {
      }
      case 4:
      /* 5551 and 1555 UINT formats fail on Gfx8/Carrizo´. */
   if (desc->channel[1].size == 5 &&
      desc->channel[2].size == 5 &&
   desc->channel[first_non_void].type == UTIL_FORMAT_TYPE_UNSIGNED &&
               if (desc->channel[0].size == 5 && desc->channel[1].size == 5 &&
      desc->channel[2].size == 5 && desc->channel[3].size == 1) {
      }
   if (desc->channel[0].size == 1 && desc->channel[1].size == 5 &&
      desc->channel[2].size == 5 && desc->channel[3].size == 5) {
      }
   if (desc->channel[0].size == 10 && desc->channel[1].size == 10 &&
      desc->channel[2].size == 10 && desc->channel[3].size == 2) {
      }
      }
               /* uniform formats */
   switch (desc->channel[first_non_void].size) {
   case 4:
      switch (desc->nr_channels) {
   case 4:
      /* 4444 UINT formats fail on Gfx8/Carrizo´. */
   if (desc->channel[first_non_void].type == UTIL_FORMAT_TYPE_UNSIGNED &&
                     }
      case 8:
      switch (desc->nr_channels) {
   case 1:
         case 2:
         case 4:
         }
      case 16:
      switch (desc->nr_channels) {
   case 1:
         case 2:
         case 4:
         }
      case 32:
      switch (desc->nr_channels) {
   case 1:
         case 2:
   #if 0 /* Not supported for render targets */
         case 3:
   #endif
         case 4:
                  out_unknown:
         }
      static unsigned is_wrap_mode_legal(struct si_screen *screen, unsigned wrap)
   {
      if (!screen->info.has_3d_cube_border_color_mipmap) {
      switch (wrap) {
   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
            }
      }
      static unsigned si_tex_wrap(unsigned wrap)
   {
      switch (wrap) {
   default:
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_CLAMP:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
         case PIPE_TEX_WRAP_MIRROR_CLAMP:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
            }
      static unsigned si_tex_mipfilter(unsigned filter)
   {
      switch (filter) {
   case PIPE_TEX_MIPFILTER_NEAREST:
         case PIPE_TEX_MIPFILTER_LINEAR:
         default:
   case PIPE_TEX_MIPFILTER_NONE:
            }
      static unsigned si_tex_compare(unsigned mode, unsigned compare)
   {
      if (mode == PIPE_TEX_COMPARE_NONE)
            switch (compare) {
   default:
   case PIPE_FUNC_NEVER:
         case PIPE_FUNC_LESS:
         case PIPE_FUNC_EQUAL:
         case PIPE_FUNC_LEQUAL:
         case PIPE_FUNC_GREATER:
         case PIPE_FUNC_NOTEQUAL:
         case PIPE_FUNC_GEQUAL:
         case PIPE_FUNC_ALWAYS:
            }
      static unsigned si_tex_dim(struct si_screen *sscreen, struct si_texture *tex, unsigned view_target,
         {
               if (view_target == PIPE_TEXTURE_CUBE || view_target == PIPE_TEXTURE_CUBE_ARRAY)
         /* If interpreting cubemaps as something else, set 2D_ARRAY. */
   else if (res_target == PIPE_TEXTURE_CUBE || res_target == PIPE_TEXTURE_CUBE_ARRAY)
            /* GFX9 allocates 1D textures as 2D. */
   if ((res_target == PIPE_TEXTURE_1D || res_target == PIPE_TEXTURE_1D_ARRAY) &&
      sscreen->info.gfx_level == GFX9 &&
   tex->surface.u.gfx9.resource_type == RADEON_RESOURCE_2D) {
   if (res_target == PIPE_TEXTURE_1D)
         else
               switch (res_target) {
   default:
   case PIPE_TEXTURE_1D:
         case PIPE_TEXTURE_1D_ARRAY:
         case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
         case PIPE_TEXTURE_2D_ARRAY:
         case PIPE_TEXTURE_3D:
         case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
            }
      /*
   * Format support testing
   */
      static bool si_is_sampler_format_supported(struct pipe_screen *screen, enum pipe_format format)
   {
      struct si_screen *sscreen = (struct si_screen *)screen;
            /* Samplers don't support 64 bits per channel. */
   if (desc->layout == UTIL_FORMAT_LAYOUT_PLAIN &&
      desc->channel[0].size == 64)
         if (sscreen->info.gfx_level >= GFX10) {
      const struct gfx10_format *fmt = &ac_get_gfx10_format_table(&sscreen->info)[format];
   if (!fmt->img_format || fmt->buffers_only)
                     return si_translate_texformat(screen, format, desc,
      }
      static uint32_t si_translate_buffer_dataformat(struct pipe_screen *screen,
               {
                        if (desc->format == PIPE_FORMAT_R11G11B10_FLOAT)
                     if (desc->nr_channels == 4 && desc->channel[0].size == 10 && desc->channel[1].size == 10 &&
      desc->channel[2].size == 10 && desc->channel[3].size == 2)
         /* See whether the components are of the same size. */
   for (i = 0; i < desc->nr_channels; i++) {
      if (desc->channel[first_non_void].size != desc->channel[i].size)
               switch (desc->channel[first_non_void].size) {
   case 8:
      switch (desc->nr_channels) {
   case 1:
   case 3: /* 3 loads */
         case 2:
         case 4:
         }
      case 16:
      switch (desc->nr_channels) {
   case 1:
   case 3: /* 3 loads */
         case 2:
         case 4:
         }
      case 32:
      switch (desc->nr_channels) {
   case 1:
         case 2:
         case 3:
         case 4:
         }
      case 64:
      /* Legacy double formats. */
   switch (desc->nr_channels) {
   case 1: /* 1 load */
         case 2: /* 1 load */
         case 3: /* 3 loads */
         case 4: /* 2 loads */
         }
                  }
      static uint32_t si_translate_buffer_numformat(struct pipe_screen *screen,
               {
               if (desc->format == PIPE_FORMAT_R11G11B10_FLOAT)
                     switch (desc->channel[first_non_void].type) {
   case UTIL_FORMAT_TYPE_SIGNED:
   case UTIL_FORMAT_TYPE_FIXED:
      if (desc->channel[first_non_void].size >= 32 || desc->channel[first_non_void].pure_integer)
         else if (desc->channel[first_non_void].normalized)
         else
            case UTIL_FORMAT_TYPE_UNSIGNED:
      if (desc->channel[first_non_void].size >= 32 || desc->channel[first_non_void].pure_integer)
         else if (desc->channel[first_non_void].normalized)
         else
            case UTIL_FORMAT_TYPE_FLOAT:
   default:
            }
      static unsigned si_is_vertex_format_supported(struct pipe_screen *screen, enum pipe_format format,
         {
      struct si_screen *sscreen = (struct si_screen *)screen;
   const struct util_format_description *desc;
   int first_non_void;
            assert((usage & ~(PIPE_BIND_SHADER_IMAGE | PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_VERTEX_BUFFER)) ==
                     /* There are no native 8_8_8 or 16_16_16 data formats, and we currently
   * select 8_8_8_8 and 16_16_16_16 instead. This works reasonably well
   * for read-only access (with caveats surrounding bounds checks), but
   * obviously fails for write access which we have to implement for
   * shader images. Luckily, OpenGL doesn't expect this to be supported
   * anyway, and so the only impact is on PBO uploads / downloads, which
   * shouldn't be expected to be fast for GL_RGB anyway.
   */
   if (desc->block.bits == 3 * 8 || desc->block.bits == 3 * 16) {
      if (usage & (PIPE_BIND_SHADER_IMAGE | PIPE_BIND_SAMPLER_VIEW)) {
      usage &= ~(PIPE_BIND_SHADER_IMAGE | PIPE_BIND_SAMPLER_VIEW);
   if (!usage)
                  if (sscreen->info.gfx_level >= GFX10) {
      const struct gfx10_format *fmt = &ac_get_gfx10_format_table(&sscreen->info)[format];
            if (!fmt->img_format || fmt->img_format >= first_image_only_format)
                     first_non_void = util_format_get_first_non_void_channel(format);
   data_format = si_translate_buffer_dataformat(screen, desc, first_non_void);
   if (data_format == V_008F0C_BUF_DATA_FORMAT_INVALID)
               }
      static bool si_is_colorbuffer_format_supported(enum amd_gfx_level gfx_level,
         {
      return ac_get_cb_format(gfx_level, format) != V_028C70_COLOR_INVALID &&
      }
      static bool si_is_zs_format_supported(enum pipe_format format)
   {
         }
      static bool si_is_format_supported(struct pipe_screen *screen, enum pipe_format format,
               {
      struct si_screen *sscreen = (struct si_screen *)screen;
            if (target >= PIPE_MAX_TEXTURE_TYPES) {
      PRINT_ERR("radeonsi: unsupported texture type %d\n", target);
               /* Require PIPE_BIND_SAMPLER_VIEW support when PIPE_BIND_RENDER_TARGET
   * is requested.
   */
   if (usage & PIPE_BIND_RENDER_TARGET)
            if ((target == PIPE_TEXTURE_3D || target == PIPE_TEXTURE_CUBE) &&
      !sscreen->info.has_3d_cube_border_color_mipmap)
         if (util_format_get_num_planes(format) >= 2)
            if (MAX2(1, sample_count) < MAX2(1, storage_sample_count))
            if (sample_count > 1) {
      if (!screen->get_param(screen, PIPE_CAP_TEXTURE_MULTISAMPLE))
            /* Only power-of-two sample counts are supported. */
   if (!util_is_power_of_two_or_zero(sample_count) ||
                  /* Chips with 1 RB don't increment occlusion queries at 16x MSAA sample rate,
   * so don't expose 16 samples there.
   */
   const unsigned max_eqaa_samples =
      (sscreen->info.gfx_level >= GFX11 ||
               /* MSAA support without framebuffer attachments. */
   if (format == PIPE_FORMAT_NONE && sample_count <= max_eqaa_samples)
            if (!sscreen->info.has_eqaa_surface_allocator || util_format_is_depth_or_stencil(format)) {
      /* Color without EQAA or depth/stencil. */
   if (sample_count > max_samples || sample_count != storage_sample_count)
      } else {
      /* Color with EQAA. */
   if (sample_count > max_eqaa_samples || storage_sample_count > max_samples)
                  if (usage & (PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_SHADER_IMAGE)) {
      if (target == PIPE_BUFFER) {
      retval |= si_is_vertex_format_supported(
      } else {
      if (si_is_sampler_format_supported(screen, format))
                  if ((usage & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SCANOUT |
            si_is_colorbuffer_format_supported(sscreen->info.gfx_level, format)) {
   retval |= usage & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SCANOUT |
         if (!util_format_is_pure_integer(format) && !util_format_is_depth_or_stencil(format))
               if ((usage & PIPE_BIND_DEPTH_STENCIL) && si_is_zs_format_supported(format)) {
                  if (usage & PIPE_BIND_VERTEX_BUFFER) {
                  if (usage & PIPE_BIND_INDEX_BUFFER) {
      if (format == PIPE_FORMAT_R8_UINT ||
      format == PIPE_FORMAT_R16_UINT ||
   format == PIPE_FORMAT_R32_UINT)
            if ((usage & PIPE_BIND_LINEAR) && !util_format_is_compressed(format) &&
      !(usage & PIPE_BIND_DEPTH_STENCIL))
            }
      /*
   * framebuffer handling
   */
      static void si_choose_spi_color_formats(struct si_surface *surf, unsigned format, unsigned swap,
         {
                        surf->spi_shader_col_format = formats.normal;
   surf->spi_shader_col_format_alpha = formats.alpha;
   surf->spi_shader_col_format_blend = formats.blend;
      }
      static void si_initialize_color_surface(struct si_context *sctx, struct si_surface *surf)
   {
      struct si_texture *tex = (struct si_texture *)surf->base.texture;
   unsigned format, swap, ntype, endian;
   const struct util_format_description *desc;
                     ntype = ac_get_cb_number_type(surf->base.format);
            if (format == V_028C70_COLOR_INVALID) {
         }
   assert(format != V_028C70_COLOR_INVALID);
   swap = si_translate_colorswap(sctx->gfx_level, surf->base.format, false);
            /* blend clamp should be set for all NORM/SRGB types */
   if (ntype == V_028C70_NUMBER_UNORM || ntype == V_028C70_NUMBER_SNORM ||
      ntype == V_028C70_NUMBER_SRGB)
         /* set blend bypass according to docs if SINT/UINT or
         if (ntype == V_028C70_NUMBER_UINT || ntype == V_028C70_NUMBER_SINT ||
      format == V_028C70_COLOR_8_24 || format == V_028C70_COLOR_24_8 ||
   format == V_028C70_COLOR_X24_8_32_FLOAT) {
   blend_clamp = 0;
               if (ntype == V_028C70_NUMBER_UINT || ntype == V_028C70_NUMBER_SINT) {
      if (format == V_028C70_COLOR_8 || format == V_028C70_COLOR_8_8 ||
      format == V_028C70_COLOR_8_8_8_8)
      else if (format == V_028C70_COLOR_10_10_10_2 || format == V_028C70_COLOR_2_10_10_10)
               unsigned log_samples = util_logbase2(tex->buffer.b.b.nr_samples);
   unsigned log_fragments = util_logbase2(tex->buffer.b.b.nr_storage_samples);
   /* Intensity is implemented as Red, so treat it that way. */
   bool force_dst_alpha_1 = desc->swizzle[3] == PIPE_SWIZZLE_1 ||
         bool round_mode = ntype != V_028C70_NUMBER_UNORM && ntype != V_028C70_NUMBER_SNORM &&
               /* amdvlk: [min-compressed-block-size] should be set to 32 for dGPU and
   * 64 for APU because all of our APUs to date use DIMMs which have
   * a request granularity size of 64B while all other chips have a
   * 32B request size */
   unsigned min_compressed_block_size = V_028C78_MIN_BLOCK_SIZE_32B;
   if (!sctx->screen->info.has_dedicated_vram)
            surf->cb_color_info = S_028C70_COMP_SWAP(swap) |
                        S_028C70_BLEND_CLAMP(blend_clamp) |
                  /* GFX10.3+ can set a custom pitch for 1D and 2D non-array, but it must be a multiple of
   * 256B.
   *
   * We set the pitch in MIP0_WIDTH.
   */
   if (sctx->gfx_level >= GFX10_3 && tex->surface.u.gfx9.uses_custom_pitch) {
      ASSERTED unsigned min_alignment = 256;
   assert((tex->surface.u.gfx9.surf_pitch * tex->surface.bpe) % min_alignment == 0);
   assert(tex->buffer.b.b.target == PIPE_TEXTURE_2D ||
                           /* Subsampled images have the pitch in the units of blocks. */
   if (tex->surface.blk_w == 2)
               if (sctx->gfx_level >= GFX10) {
      /* Gfx10-11. */
   surf->cb_color_view = S_028C6C_SLICE_START(surf->base.u.tex.first_layer) |
               surf->cb_color_attrib = 0;
   surf->cb_color_attrib2 = S_028C68_MIP0_WIDTH(width0 - 1) |
               surf->cb_color_attrib3 = S_028EE0_MIP0_DEPTH(util_max_layer(&tex->buffer.b.b, 0)) |
               surf->cb_dcc_control = S_028C78_MAX_UNCOMPRESSED_BLOCK_SIZE(V_028C78_MAX_BLOCK_SIZE_256B) |
                        if (sctx->gfx_level >= GFX11) {
      assert(!UTIL_ARCH_BIG_ENDIAN);
   surf->cb_color_info |= S_028C70_FORMAT_GFX11(format);
   surf->cb_color_attrib |= S_028C74_NUM_FRAGMENTS_GFX11(log_fragments) |
            } else {
      surf->cb_color_info |= S_028C70_ENDIAN(endian) |
               surf->cb_color_attrib |= S_028C74_NUM_SAMPLES(log_samples) |
                     } else {
      /* Gfx6-9. */
   surf->cb_color_info |= S_028C70_ENDIAN(endian) |
               surf->cb_color_view = S_028C6C_SLICE_START(surf->base.u.tex.first_layer) |
         surf->cb_color_attrib = S_028C74_NUM_SAMPLES(log_samples) |
               surf->cb_color_attrib2 = 0;
            if (sctx->gfx_level == GFX9) {
      surf->cb_color_view |= S_028C6C_MIP_LEVEL_GFX9(surf->base.u.tex.level);
   surf->cb_color_attrib |= S_028C74_MIP0_DEPTH(util_max_layer(&tex->buffer.b.b, 0)) |
         surf->cb_color_attrib2 |= S_028C68_MIP0_WIDTH(surf->width0 - 1) |
                     if (sctx->gfx_level >= GFX8) {
               if (tex->buffer.b.b.nr_storage_samples > 1) {
      if (tex->surface.bpe == 1)
         else if (tex->surface.bpe == 2)
               surf->cb_dcc_control |= S_028C78_MAX_UNCOMPRESSED_BLOCK_SIZE(max_uncompressed_block_size) |
                     if (sctx->gfx_level == GFX6) {
      /* Due to a hw bug, FMASK_BANK_HEIGHT must still be set on GFX6. (inherited from GFX5) */
   /* This must also be set for fast clear to work without FMASK. */
   unsigned fmask_bankh = tex->surface.fmask_offset ? tex->surface.u.legacy.color.fmask.bankh
                        /* Determine pixel shader export format */
               }
      static void si_init_depth_surface(struct si_context *sctx, struct si_surface *surf)
   {
      struct si_texture *tex = (struct si_texture *)surf->base.texture;
   unsigned level = surf->base.u.tex.level;
            format = si_translate_dbformat(tex->db_render_format);
            assert(format != V_028040_Z_INVALID);
   if (format == V_028040_Z_INVALID)
            if (sctx->gfx_level >= GFX9) {
      surf->db_htile_data_base = 0;
   surf->db_htile_surface = 0;
   surf->db_depth_view = S_028008_SLICE_START(surf->base.u.tex.first_layer) |
         if (sctx->gfx_level >= GFX10) {
      surf->db_depth_view |= S_028008_SLICE_START_HI(surf->base.u.tex.first_layer >> 11) |
               assert(tex->surface.u.gfx9.surf_offset == 0);
   surf->db_depth_base = tex->buffer.gpu_address >> 8;
   surf->db_stencil_base = (tex->buffer.gpu_address + tex->surface.u.gfx9.zs.stencil_offset) >> 8;
   surf->db_z_info = S_028038_FORMAT(format) |
                     S_028038_NUM_SAMPLES(util_logbase2(tex->buffer.b.b.nr_samples)) |
   surf->db_stencil_info = S_02803C_FORMAT(stencil_format) |
                  if (sctx->gfx_level == GFX9) {
      surf->db_z_info2 = S_028068_EPITCH(tex->surface.u.gfx9.epitch);
      }
   surf->db_depth_view |= S_028008_MIPID(level);
   surf->db_depth_size = S_02801C_X_MAX(tex->buffer.b.b.width0 - 1) |
            if (si_htile_enabled(tex, level, PIPE_MASK_ZS)) {
      surf->db_z_info |= S_028038_TILE_SURFACE_ENABLE(1) |
                  if (tex->surface.has_stencil && !tex->htile_stencil_disabled) {
      /* Stencil buffer workaround ported from the GFX6-GFX8 code.
   * See that for explanation.
   */
               surf->db_htile_data_base = (tex->buffer.gpu_address + tex->surface.meta_offset) >> 8;
   surf->db_htile_surface = S_028ABC_FULL_CACHE(1) |
         if (sctx->gfx_level == GFX9) {
               } else {
      /* GFX6-GFX8 */
                     surf->db_depth_base =
         surf->db_stencil_base =
         surf->db_htile_data_base = 0;
   surf->db_htile_surface = 0;
   surf->db_depth_view = S_028008_SLICE_START(surf->base.u.tex.first_layer) |
         surf->db_z_info = S_028040_FORMAT(format) |
         surf->db_stencil_info = S_028044_FORMAT(stencil_format);
            if (sctx->gfx_level >= GFX7) {
      struct radeon_info *info = &sctx->screen->info;
   unsigned index = tex->surface.u.legacy.tiling_index[level];
   unsigned stencil_index = tex->surface.u.legacy.zs.stencil_tiling_index[level];
   unsigned macro_index = tex->surface.u.legacy.macro_tile_index;
   unsigned tile_mode = info->si_tile_mode_array[index];
                  surf->db_depth_info |= S_02803C_ARRAY_MODE(G_009910_ARRAY_MODE(tile_mode)) |
                        S_02803C_PIPE_CONFIG(G_009910_PIPE_CONFIG(tile_mode)) |
      surf->db_z_info |= S_028040_TILE_SPLIT(G_009910_TILE_SPLIT(tile_mode));
      } else {
      unsigned tile_mode_index = si_tile_mode_index(tex, level, false);
   surf->db_z_info |= S_028040_TILE_MODE_INDEX(tile_mode_index);
   tile_mode_index = si_tile_mode_index(tex, level, true);
               surf->db_depth_size = S_028058_PITCH_TILE_MAX((levelinfo->nblk_x / 8) - 1) |
         surf->db_depth_slice =
            if (si_htile_enabled(tex, level, PIPE_MASK_ZS)) {
      surf->db_z_info |= S_028040_TILE_SURFACE_ENABLE(1) |
                  if (tex->surface.has_stencil) {
      /* Workaround: For a not yet understood reason, the
   * combination of MSAA, fast stencil clear and stencil
   * decompress messes with subsequent stencil buffer
   * uses. Problem was reproduced on Verde, Bonaire,
   * Tonga, and Carrizo.
   *
   * Disabling EXPCLEAR works around the problem.
   *
   * Check piglit's arb_texture_multisample-stencil-clear
   * test if you want to try changing this.
   */
   if (tex->buffer.b.b.nr_samples <= 1)
               surf->db_htile_data_base = (tex->buffer.gpu_address + tex->surface.meta_offset) >> 8;
                     }
      void si_set_sampler_depth_decompress_mask(struct si_context *sctx, struct si_texture *tex)
   {
      /* Check all sampler bindings in all shaders where depth textures are bound, and update
   * which samplers should be decompressed.
   */
   u_foreach_bit(sh, sctx->shader_has_depth_tex) {
      u_foreach_bit(i, sctx->samplers[sh].has_depth_tex_mask) {
      if (sctx->samplers[sh].views[i]->texture == &tex->buffer.b.b) {
      sctx->samplers[sh].needs_depth_decompress_mask |= 1 << i;
               }
      void si_update_fb_dirtiness_after_rendering(struct si_context *sctx)
   {
      if (sctx->decompression_enabled)
            if (sctx->framebuffer.state.zsbuf) {
      struct pipe_surface *surf = sctx->framebuffer.state.zsbuf;
                     if (tex->surface.has_stencil)
                        unsigned compressed_cb_mask = sctx->framebuffer.compressed_cb_mask;
   while (compressed_cb_mask) {
      unsigned i = u_bit_scan(&compressed_cb_mask);
   struct pipe_surface *surf = sctx->framebuffer.state.cbufs[i];
            if (tex->surface.fmask_offset) {
      tex->dirty_level_mask |= 1 << surf->u.tex.level;
            }
      static void si_dec_framebuffer_counters(const struct pipe_framebuffer_state *state)
   {
      for (int i = 0; i < state->nr_cbufs; ++i) {
      struct si_surface *surf = NULL;
            if (!state->cbufs[i])
         surf = (struct si_surface *)state->cbufs[i];
                  }
      void si_mark_display_dcc_dirty(struct si_context *sctx, struct si_texture *tex)
   {
      if (!tex->surface.display_dcc_offset || tex->displayable_dcc_dirty)
            if (!(tex->buffer.external_usage & PIPE_HANDLE_USAGE_EXPLICIT_FLUSH)) {
      struct hash_entry *entry = _mesa_hash_table_search(sctx->dirty_implicit_resources, tex);
   if (!entry) {
      struct pipe_resource *dummy = NULL;
   pipe_resource_reference(&dummy, &tex->buffer.b.b);
         }
      }
      static void si_update_display_dcc_dirty(struct si_context *sctx)
   {
               for (unsigned i = 0; i < state->nr_cbufs; i++) {
      if (state->cbufs[i])
         }
      static void si_set_framebuffer_state(struct pipe_context *ctx,
         {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_surface *surf = NULL;
   struct si_texture *tex;
   bool old_any_dst_linear = sctx->framebuffer.any_dst_linear;
   unsigned old_nr_samples = sctx->framebuffer.nr_samples;
   unsigned old_colorbuf_enabled_4bit = sctx->framebuffer.colorbuf_enabled_4bit;
   bool old_has_zsbuf = !!sctx->framebuffer.state.zsbuf;
   bool old_has_stencil =
      old_has_zsbuf &&
               /* Reject zero-sized framebuffers due to a hw bug on GFX6 that occurs
   * when PA_SU_HARDWARE_SCREEN_OFFSET != 0 and any_scissor.BR_X/Y <= 0.
   * We could implement the full workaround here, but it's a useless case.
   */
   if ((!state->width || !state->height) && (state->nr_cbufs || state->zsbuf)) {
      unreachable("the framebuffer shouldn't have zero area");
                        /* Disable DCC if the formats are incompatible. */
   for (i = 0; i < state->nr_cbufs; i++) {
      if (!state->cbufs[i])
            surf = (struct si_surface *)state->cbufs[i];
            if (!surf->dcc_incompatible)
            if (vi_dcc_enabled(tex, surf->base.u.tex.level))
                              /* Only flush TC when changing the framebuffer state, because
   * the only client not using TC that can change textures is
   * the framebuffer.
   *
   * Wait for compute shaders because of possible transitions:
   * - FB write -> shader read
   * - shader write -> FB read
   *
   * Wait for draws because of possible transitions:
   * - texture -> render (eg: glBlitFramebuffer(with src=dst) then glDraw*)
   *
   * DB caches are flushed on demand (using si_decompress_textures).
   *
   * When MSAA is enabled, CB and TC caches are flushed on demand
   * (after FMASK decompression). Shader write -> FB read transitions
   * cannot happen for MSAA textures, because MSAA shader images are
   * not supported.
   *
   * Only flush and wait for CB if there is actually a bound color buffer.
   */
   if (sctx->framebuffer.uncompressed_cb_mask) {
      si_make_CB_shader_coherent(sctx, sctx->framebuffer.nr_samples,
                     sctx->flags |= SI_CONTEXT_CS_PARTIAL_FLUSH | SI_CONTEXT_PS_PARTIAL_FLUSH;
            /* u_blitter doesn't invoke depth decompression when it does multiple
   * blits in a row, but the only case when it matters for DB is when
   * doing generate_mipmap. So here we flush DB manually between
   * individual generate_mipmap blits.
   * Note that lower mipmap levels aren't compressed.
   */
   if (sctx->generate_mipmap_for_depth) {
         } else if (sctx->gfx_level == GFX9) {
      /* It appears that DB metadata "leaks" in a sequence of:
   *  - depth clear
   *  - DCC decompress for shader image writes (with DB disabled)
   *  - render with DEPTH_BEFORE_SHADER=1
   * Flushing DB metadata works around the problem.
   */
   sctx->flags |= SI_CONTEXT_FLUSH_AND_INV_DB_META;
               /* Take the maximum of the old and new count. If the new count is lower,
   * dirtying is needed to disable the unbound colorbuffers.
   */
   sctx->framebuffer.dirty_cbufs |=
                  si_dec_framebuffer_counters(&sctx->framebuffer.state);
            sctx->framebuffer.colorbuf_enabled_4bit = 0;
   sctx->framebuffer.spi_shader_col_format = 0;
   sctx->framebuffer.spi_shader_col_format_alpha = 0;
   sctx->framebuffer.spi_shader_col_format_blend = 0;
   sctx->framebuffer.spi_shader_col_format_blend_alpha = 0;
   sctx->framebuffer.color_is_int8 = 0;
            sctx->framebuffer.compressed_cb_mask = 0;
   sctx->framebuffer.uncompressed_cb_mask = 0;
   sctx->framebuffer.nr_samples = util_framebuffer_get_num_samples(state);
   sctx->framebuffer.nr_color_samples = sctx->framebuffer.nr_samples;
   sctx->framebuffer.log_samples = util_logbase2(sctx->framebuffer.nr_samples);
   sctx->framebuffer.any_dst_linear = false;
   sctx->framebuffer.CB_has_shader_readable_metadata = false;
   sctx->framebuffer.DB_has_shader_readable_metadata = false;
   sctx->framebuffer.all_DCC_pipe_aligned = true;
   sctx->framebuffer.has_dcc_msaa = false;
            for (i = 0; i < state->nr_cbufs; i++) {
      if (!state->cbufs[i])
            surf = (struct si_surface *)state->cbufs[i];
            if (!surf->color_initialized) {
                  sctx->framebuffer.colorbuf_enabled_4bit |= 0xf << (i * 4);
   sctx->framebuffer.spi_shader_col_format |= surf->spi_shader_col_format << (i * 4);
   sctx->framebuffer.spi_shader_col_format_alpha |= surf->spi_shader_col_format_alpha << (i * 4);
   sctx->framebuffer.spi_shader_col_format_blend |= surf->spi_shader_col_format_blend << (i * 4);
   sctx->framebuffer.spi_shader_col_format_blend_alpha |= surf->spi_shader_col_format_blend_alpha
            if (surf->color_is_int8)
         if (surf->color_is_int10)
            if (tex->surface.fmask_offset)
         else
            /* Don't update nr_color_samples for non-AA buffers.
   * (e.g. destination of MSAA resolve)
   */
   if (tex->buffer.b.b.nr_samples >= 2 &&
      tex->buffer.b.b.nr_storage_samples < tex->buffer.b.b.nr_samples) {
   sctx->framebuffer.nr_color_samples =
                     if (tex->surface.is_linear)
            if (vi_dcc_enabled(tex, surf->base.u.tex.level)) {
                              if (tex->buffer.b.b.nr_storage_samples >= 2)
                        /* Update the minimum but don't keep 0. */
   if (!sctx->framebuffer.min_bytes_per_pixel ||
      tex->surface.bpe < sctx->framebuffer.min_bytes_per_pixel)
                     if (state->zsbuf) {
      surf = (struct si_surface *)state->zsbuf;
            if (!surf->depth_initialized) {
                  if (vi_tc_compat_htile_enabled(zstex, surf->base.u.tex.level, PIPE_MASK_ZS))
            /* Update the minimum but don't keep 0. */
   if (!sctx->framebuffer.min_bytes_per_pixel ||
      zstex->surface.bpe < sctx->framebuffer.min_bytes_per_pixel)
            si_update_ps_colorbuf0_slot(sctx);
   si_update_poly_offset_state(sctx);
   si_mark_atom_dirty(sctx, &sctx->atoms.s.cb_render_state);
            /* NGG cull state uses the sample count. */
   if (sctx->screen->use_ngg_culling)
            if (sctx->screen->dpbb_allowed)
            if (sctx->framebuffer.any_dst_linear != old_any_dst_linear)
            if (sctx->screen->info.has_out_of_order_rast &&
      (sctx->framebuffer.colorbuf_enabled_4bit != old_colorbuf_enabled_4bit ||
   !!sctx->framebuffer.state.zsbuf != old_has_zsbuf ||
   (zstex && zstex->surface.has_stencil != old_has_stencil)))
         if (sctx->framebuffer.nr_samples != old_nr_samples) {
               si_mark_atom_dirty(sctx, &sctx->atoms.s.msaa_config);
            if (!sctx->sample_pos_buffer) {
      sctx->sample_pos_buffer = pipe_buffer_create_with_data(&sctx->b, 0, PIPE_USAGE_DEFAULT,
            }
            /* Set sample locations as fragment shader constants. */
   switch (sctx->framebuffer.nr_samples) {
   case 1:
      constbuf.buffer_offset = 0;
      case 2:
      constbuf.buffer_offset =
            case 4:
      constbuf.buffer_offset =
            case 8:
      constbuf.buffer_offset =
            case 16:
      constbuf.buffer_offset =
            default:
      PRINT_ERR("Requested an invalid number of samples %i.\n", sctx->framebuffer.nr_samples);
      }
   constbuf.buffer_size = sctx->framebuffer.nr_samples * 2 * 4;
                        si_ps_key_update_framebuffer(sctx);
   si_ps_key_update_framebuffer_blend_rasterizer(sctx);
   si_ps_key_update_framebuffer_rasterizer_sample_shading(sctx);
   si_update_ps_inputs_read_or_disabled(sctx);
            if (!sctx->decompression_enabled) {
      /* Prevent textures decompression when the framebuffer state
   * changes come from the decompression passes themselves.
   */
         }
      static void si_emit_framebuffer_state(struct si_context *sctx, unsigned index)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
   struct pipe_framebuffer_state *state = &sctx->framebuffer.state;
   unsigned i, nr_cbufs = state->nr_cbufs;
   struct si_texture *tex = NULL;
   struct si_surface *cb = NULL;
   bool is_msaa_resolve = state->nr_cbufs == 2 &&
                  /* CB can't do MSAA resolve on gfx11. */
                     /* Colorbuffers. */
   for (i = 0; i < nr_cbufs; i++) {
      if (!(sctx->framebuffer.dirty_cbufs & (1 << i)))
            /* RB+ depth-only rendering. See the comment where we set rbplus_depth_only_opt for more
   * information.
   */
   if (i == 0 &&
      sctx->screen->info.rbplus_allowed &&
   !sctx->queued.named.blend->cb_target_mask) {
   radeon_set_context_reg(R_028C70_CB_COLOR0_INFO + i * 0x3C,
                                       cb = (struct si_surface *)state->cbufs[i];
   if (!cb) {
      radeon_set_context_reg(R_028C70_CB_COLOR0_INFO + i * 0x3C,
                                 tex = (struct si_texture *)cb->base.texture;
   radeon_add_to_buffer_list(
                  if (tex->cmask_buffer && tex->cmask_buffer != &tex->buffer) {
      radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, tex->cmask_buffer,
                     /* Compute mutable surface parameters. */
   uint64_t cb_color_base = tex->buffer.gpu_address >> 8;
   uint64_t cb_dcc_base = 0;
            if (sctx->gfx_level < GFX11) {
      if (tex->swap_rgb_to_bgr) {
      /* Swap R and B channels. */
   static unsigned rgb_to_bgr[4] = {
      [V_028C70_SWAP_STD] = V_028C70_SWAP_ALT,
   [V_028C70_SWAP_ALT] = V_028C70_SWAP_STD,
   [V_028C70_SWAP_STD_REV] = V_028C70_SWAP_ALT_REV,
                     cb_color_info &= C_028C70_COMP_SWAP;
                                 if (vi_dcc_enabled(tex, cb->base.u.tex.level) && (i != 1 || !is_msaa_resolve))
               /* Set up DCC. */
   if (vi_dcc_enabled(tex, cb->base.u.tex.level)) {
               unsigned dcc_tile_swizzle = tex->surface.tile_swizzle;
   dcc_tile_swizzle &= ((1 << tex->surface.meta_alignment_log2) - 1) >> 8;
               if (sctx->gfx_level >= GFX11) {
               /* Set mutable surface parameters. */
                  cb_color_attrib3 = cb->cb_color_attrib3 |
               cb_fdcc_control = cb->cb_dcc_control |
                  if (sctx->family >= CHIP_GFX1103_R2) {
      cb_fdcc_control |= S_028C78_ENABLE_MAX_COMP_FRAG_OVERRIDE(1) |
                        radeon_set_context_reg_seq(R_028C6C_CB_COLOR0_VIEW + i * 0x3C, 4);
   radeon_emit(cb->cb_color_view);                      /* CB_COLOR0_VIEW */
   radeon_emit(cb_color_info);                          /* CB_COLOR0_INFO */
                  radeon_set_context_reg(R_028C94_CB_COLOR0_DCC_BASE + i * 0x3C, cb_dcc_base);
   radeon_set_context_reg(R_028E40_CB_COLOR0_BASE_EXT + i * 4, cb_color_base >> 32);
   radeon_set_context_reg(R_028EA0_CB_COLOR0_DCC_BASE_EXT + i * 4, cb_dcc_base >> 32);
   radeon_set_context_reg(R_028EC0_CB_COLOR0_ATTRIB2 + i * 4, cb->cb_color_attrib2);
      } else if (sctx->gfx_level >= GFX10) {
                     /* Set mutable surface parameters. */
                  if (tex->surface.fmask_offset) {
      cb_color_fmask = (tex->buffer.gpu_address + tex->surface.fmask_offset) >> 8;
      } else {
                  if (cb->base.u.tex.level > 0)
                        cb_color_attrib3 = cb->cb_color_attrib3 |
                              radeon_set_context_reg_seq(R_028C60_CB_COLOR0_BASE + i * 0x3C, 14);
   radeon_emit(cb_color_base);             /* CB_COLOR0_BASE */
   radeon_emit(0);                         /* hole */
   radeon_emit(0);                         /* hole */
   radeon_emit(cb->cb_color_view);         /* CB_COLOR0_VIEW */
   radeon_emit(cb_color_info);             /* CB_COLOR0_INFO */
   radeon_emit(cb->cb_color_attrib);       /* CB_COLOR0_ATTRIB */
   radeon_emit(cb->cb_dcc_control);        /* CB_COLOR0_DCC_CONTROL */
   radeon_emit(cb_color_cmask);            /* CB_COLOR0_CMASK */
   radeon_emit(0);                         /* hole */
   radeon_emit(cb_color_fmask);            /* CB_COLOR0_FMASK */
   radeon_emit(0);                         /* hole */
   radeon_emit(tex->color_clear_value[0]); /* CB_COLOR0_CLEAR_WORD0 */
                  radeon_set_context_reg(R_028E40_CB_COLOR0_BASE_EXT + i * 4, cb_color_base >> 32);
   radeon_set_context_reg(R_028E60_CB_COLOR0_CMASK_BASE_EXT + i * 4,
         radeon_set_context_reg(R_028E80_CB_COLOR0_FMASK_BASE_EXT + i * 4,
         radeon_set_context_reg(R_028EA0_CB_COLOR0_DCC_BASE_EXT + i * 4, cb_dcc_base >> 32);
   radeon_set_context_reg(R_028EC0_CB_COLOR0_ATTRIB2 + i * 4, cb->cb_color_attrib2);
      } else if (sctx->gfx_level == GFX9) {
      struct gfx9_surf_meta_flags meta = {
      .rb_aligned = 1,
      };
                                 /* Set mutable surface parameters. */
                  if (tex->surface.fmask_offset) {
      cb_color_fmask = (tex->buffer.gpu_address + tex->surface.fmask_offset) >> 8;
      } else {
                  if (cb->base.u.tex.level > 0)
                        cb_color_attrib |= S_028C74_COLOR_SW_MODE(tex->surface.u.gfx9.swizzle_mode) |
                        radeon_set_context_reg_seq(R_028C60_CB_COLOR0_BASE + i * 0x3C, 15);
   radeon_emit(cb_color_base);                            /* CB_COLOR0_BASE */
   radeon_emit(S_028C64_BASE_256B(cb_color_base >> 32));  /* CB_COLOR0_BASE_EXT */
   radeon_emit(cb->cb_color_attrib2);                     /* CB_COLOR0_ATTRIB2 */
   radeon_emit(cb->cb_color_view);                        /* CB_COLOR0_VIEW */
   radeon_emit(cb_color_info);                            /* CB_COLOR0_INFO */
   radeon_emit(cb_color_attrib);                          /* CB_COLOR0_ATTRIB */
   radeon_emit(cb->cb_dcc_control);                       /* CB_COLOR0_DCC_CONTROL */
   radeon_emit(cb_color_cmask);                           /* CB_COLOR0_CMASK */
   radeon_emit(S_028C80_BASE_256B(cb_color_cmask >> 32)); /* CB_COLOR0_CMASK_BASE_EXT */
   radeon_emit(cb_color_fmask);                           /* CB_COLOR0_FMASK */
   radeon_emit(S_028C88_BASE_256B(cb_color_fmask >> 32)); /* CB_COLOR0_FMASK_BASE_EXT */
   radeon_emit(tex->color_clear_value[0]);                /* CB_COLOR0_CLEAR_WORD0 */
   radeon_emit(tex->color_clear_value[1]);                /* CB_COLOR0_CLEAR_WORD1 */
                  radeon_set_context_reg(R_0287A0_CB_MRT0_EPITCH + i * 4,
      } else {
      /* Compute mutable surface parameters (GFX6-GFX8). */
   const struct legacy_surf_level *level_info =
         unsigned pitch_tile_max, slice_tile_max, tile_mode_index;
   unsigned cb_color_pitch, cb_color_slice, cb_color_fmask_slice;
                  cb_color_base += level_info->offset_256B;
   /* Only macrotiled modes can set tile swizzle. */
                  if (tex->surface.fmask_offset) {
      cb_color_fmask = (tex->buffer.gpu_address + tex->surface.fmask_offset) >> 8;
      } else {
                  if (cb->base.u.tex.level > 0)
                                       pitch_tile_max = level_info->nblk_x / 8 - 1;
                  cb_color_attrib |= S_028C74_TILE_MODE_INDEX(tile_mode_index);
                  if (tex->surface.fmask_offset) {
      if (sctx->gfx_level >= GFX7)
      cb_color_pitch |=
      cb_color_attrib |=
            } else {
      /* This must be set for fast clear to work without FMASK. */
   if (sctx->gfx_level >= GFX7)
         cb_color_attrib |= S_028C74_FMASK_TILE_MODE_INDEX(tile_mode_index);
               radeon_set_context_reg_seq(R_028C60_CB_COLOR0_BASE + i * 0x3C,
         radeon_emit(cb_color_base);                              /* CB_COLOR0_BASE */
   radeon_emit(cb_color_pitch);                             /* CB_COLOR0_PITCH */
   radeon_emit(cb_color_slice);                             /* CB_COLOR0_SLICE */
   radeon_emit(cb->cb_color_view);                          /* CB_COLOR0_VIEW */
   radeon_emit(cb_color_info);                              /* CB_COLOR0_INFO */
   radeon_emit(cb_color_attrib);                            /* CB_COLOR0_ATTRIB */
   radeon_emit(cb->cb_dcc_control);                         /* CB_COLOR0_DCC_CONTROL */
   radeon_emit(cb_color_cmask);                             /* CB_COLOR0_CMASK */
   radeon_emit(tex->surface.u.legacy.color.cmask_slice_tile_max); /* CB_COLOR0_CMASK_SLICE */
   radeon_emit(cb_color_fmask);                             /* CB_COLOR0_FMASK */
   radeon_emit(cb_color_fmask_slice);                       /* CB_COLOR0_FMASK_SLICE */
                  if (sctx->gfx_level >= GFX8) /* R_028C94_CB_COLOR0_DCC_BASE */
         }
   for (; i < 8; i++)
      if (sctx->framebuffer.dirty_cbufs & (1 << i))
         /* ZS buffer. */
   if (state->zsbuf && sctx->framebuffer.dirty_zsbuf) {
      struct si_surface *zb = (struct si_surface *)state->zsbuf;
   struct si_texture *tex = (struct si_texture *)zb->base.texture;
   unsigned db_z_info = zb->db_z_info;
   unsigned db_stencil_info = zb->db_stencil_info;
            radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, &tex->buffer, RADEON_USAGE_READWRITE |
                        /* Set fields dependent on tc_compatile_htile. */
   if (sctx->gfx_level >= GFX9 && tc_compat_htile) {
                              if (sctx->gfx_level >= GFX10) {
      bool iterate256 = tex->buffer.b.b.nr_samples >= 2;
   db_z_info |= S_028040_ITERATE_FLUSH(1) |
                        /* Workaround for a DB hang when ITERATE_256 is set to 1. Only affects 4X MSAA D/S images. */
   if (sctx->screen->info.has_two_planes_iterate256_bug && iterate256 &&
      !tex->htile_stencil_disabled && tex->buffer.b.b.nr_samples == 4) {
         } else {
      db_z_info |= S_028038_ITERATE_FLUSH(1);
                                    if (sctx->gfx_level >= GFX10) {
                     if (sctx->gfx_level >= GFX11) {
         } else {
      radeon_set_context_reg_seq(R_02803C_DB_DEPTH_INFO, 7);
      }
   radeon_emit(db_z_info |                  /* DB_Z_INFO */
         radeon_emit(db_stencil_info);     /* DB_STENCIL_INFO */
   radeon_emit(zb->db_depth_base);   /* DB_Z_READ_BASE */
   radeon_emit(zb->db_stencil_base); /* DB_STENCIL_READ_BASE */
                  radeon_set_context_reg_seq(R_028068_DB_Z_READ_BASE_HI, 5);
   radeon_emit(zb->db_depth_base >> 32);      /* DB_Z_READ_BASE_HI */
   radeon_emit(zb->db_stencil_base >> 32);    /* DB_STENCIL_READ_BASE_HI */
   radeon_emit(zb->db_depth_base >> 32);      /* DB_Z_WRITE_BASE_HI */
   radeon_emit(zb->db_stencil_base >> 32);    /* DB_STENCIL_WRITE_BASE_HI */
      } else if (sctx->gfx_level == GFX9) {
      radeon_set_context_reg_seq(R_028014_DB_HTILE_DATA_BASE, 3);
   radeon_emit(zb->db_htile_data_base); /* DB_HTILE_DATA_BASE */
                  radeon_set_context_reg_seq(R_028038_DB_Z_INFO, 10);
   radeon_emit(db_z_info |                                   /* DB_Z_INFO */
         radeon_emit(db_stencil_info);                             /* DB_STENCIL_INFO */
   radeon_emit(zb->db_depth_base);                           /* DB_Z_READ_BASE */
   radeon_emit(S_028044_BASE_HI(zb->db_depth_base >> 32));   /* DB_Z_READ_BASE_HI */
   radeon_emit(zb->db_stencil_base);                         /* DB_STENCIL_READ_BASE */
   radeon_emit(S_02804C_BASE_HI(zb->db_stencil_base >> 32)); /* DB_STENCIL_READ_BASE_HI */
   radeon_emit(zb->db_depth_base);                           /* DB_Z_WRITE_BASE */
   radeon_emit(S_028054_BASE_HI(zb->db_depth_base >> 32));   /* DB_Z_WRITE_BASE_HI */
                  radeon_set_context_reg_seq(R_028068_DB_Z_INFO2, 2);
   radeon_emit(zb->db_z_info2);       /* DB_Z_INFO2 */
      } else {
      /* GFX6-GFX8 */
   /* Set fields dependent on tc_compatile_htile. */
   if (si_htile_enabled(tex, zb->base.u.tex.level, PIPE_MASK_ZS)) {
                        /* 0 = full compression. N = only compress up to N-1 Z planes. */
   if (tex->buffer.b.b.nr_samples <= 1)
         else if (tex->buffer.b.b.nr_samples <= 4)
         else
                           radeon_set_context_reg_seq(R_02803C_DB_DEPTH_INFO, 9);
   radeon_emit(zb->db_depth_info |   /* DB_DEPTH_INFO */
         radeon_emit(db_z_info |           /* DB_Z_INFO */
         radeon_emit(db_stencil_info);     /* DB_STENCIL_INFO */
   radeon_emit(zb->db_depth_base);   /* DB_Z_READ_BASE */
   radeon_emit(zb->db_stencil_base); /* DB_STENCIL_READ_BASE */
   radeon_emit(zb->db_depth_base);   /* DB_Z_WRITE_BASE */
   radeon_emit(zb->db_stencil_base); /* DB_STENCIL_WRITE_BASE */
   radeon_emit(zb->db_depth_size);   /* DB_DEPTH_SIZE */
               radeon_set_context_reg_seq(R_028028_DB_STENCIL_CLEAR, 2);
   radeon_emit(tex->stencil_clear_value[level]);    /* R_028028_DB_STENCIL_CLEAR */
            radeon_set_context_reg(R_028008_DB_DEPTH_VIEW, zb->db_depth_view);
      } else if (sctx->framebuffer.dirty_zsbuf) {
      if (sctx->gfx_level == GFX9)
         else
            /* Gfx11+: DB_Z_INFO.NUM_SAMPLES should match the framebuffer samples if no Z/S is bound.
   * It determines the sample count for VRS, primitive-ordered pixel shading, and occlusion
   * queries.
   */
   radeon_emit(S_028040_FORMAT(V_028040_Z_INVALID) |       /* DB_Z_INFO */
                     /* Framebuffer dimensions. */
   /* PA_SC_WINDOW_SCISSOR_TL is set to 0,0 in gfx*_init_gfx_preamble_state */
   radeon_set_context_reg(R_028208_PA_SC_WINDOW_SCISSOR_BR,
            if (sctx->screen->dpbb_allowed &&
      sctx->screen->pbb_context_states_per_bin > 1) {
   radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
      }
                     sctx->framebuffer.dirty_cbufs = 0;
      }
      static bool si_out_of_order_rasterization(struct si_context *sctx)
   {
      struct si_state_blend *blend = sctx->queued.named.blend;
            if (!sctx->screen->info.has_out_of_order_rast)
                              /* Conservative: No logic op. */
   if (colormask && blend->logicop_enable)
            struct si_dsa_order_invariance dsa_order_invariant = {.zs = true,
            if (sctx->framebuffer.state.zsbuf) {
      struct si_texture *zstex = (struct si_texture *)sctx->framebuffer.state.zsbuf->texture;
   bool has_stencil = zstex->surface.has_stencil;
   dsa_order_invariant = dsa->order_invariance[has_stencil];
   if (!dsa_order_invariant.zs)
            /* The set of PS invocations is always order invariant,
   * except when early Z/S tests are requested. */
   if (sctx->shader.ps.cso && sctx->shader.ps.cso->info.base.writes_memory &&
      sctx->shader.ps.cso->info.base.fs.early_fragment_tests &&
               if (sctx->occlusion_query_mode == SI_OCCLUSION_QUERY_MODE_PRECISE_INTEGER &&
      !dsa_order_invariant.pass_set)
            if (!colormask)
                     if (blendmask) {
      /* Only commutative blending. */
   if (blendmask & ~blend->commutative_4bit)
            if (!dsa_order_invariant.pass_set)
               if (colormask & ~blendmask)
               }
      static void si_emit_msaa_config(struct si_context *sctx, unsigned index)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
   unsigned num_tile_pipes = sctx->screen->info.num_tile_pipes;
   /* 33% faster rendering to linear color buffers */
   bool dst_is_linear = sctx->framebuffer.any_dst_linear;
   bool out_of_order_rast = si_out_of_order_rasterization(sctx);
   unsigned sc_mode_cntl_1 =
      S_028A4C_WALK_SIZE(dst_is_linear) | S_028A4C_WALK_FENCE_ENABLE(!dst_is_linear) |
   S_028A4C_WALK_FENCE_SIZE(num_tile_pipes == 2 ? 2 : 3) |
   S_028A4C_OUT_OF_ORDER_PRIMITIVE_ENABLE(out_of_order_rast) |
   S_028A4C_OUT_OF_ORDER_WATER_MARK(0x7) |
   /* always 1: */
   S_028A4C_WALK_ALIGN8_PRIM_FITS_ST(1) | S_028A4C_SUPERTILE_WALK_ORDER_ENABLE(1) |
   S_028A4C_TILE_WALK_ORDER_ENABLE(1) | S_028A4C_MULTI_SHADER_ENGINE_PRIM_DISCARD_ENABLE(1) |
      unsigned db_eqaa = S_028804_HIGH_QUALITY_INTERSECTIONS(1) | S_028804_INCOHERENT_EQAA_READS(1) |
         unsigned coverage_samples, z_samples;
            /* S: Coverage samples (up to 16x):
   * - Scan conversion samples (PA_SC_AA_CONFIG.MSAA_NUM_SAMPLES)
   * - CB FMASK samples (CB_COLORi_ATTRIB.NUM_SAMPLES)
   *
   * Z: Z/S samples (up to 8x, must be <= coverage samples and >= color samples):
   * - Value seen by DB (DB_Z_INFO.NUM_SAMPLES)
   * - Value seen by CB, must be correct even if Z/S is unbound (DB_EQAA.MAX_ANCHOR_SAMPLES)
   * # Missing samples are derived from Z planes if Z is compressed (up to 16x quality), or
   * # from the closest defined sample if Z is uncompressed (same quality as the number of
   * # Z samples).
   *
   * F: Color samples (up to 8x, must be <= coverage samples):
   * - CB color samples (CB_COLORi_ATTRIB.NUM_FRAGMENTS)
   * - PS iter samples (DB_EQAA.PS_ITER_SAMPLES)
   *
   * Can be anything between coverage and color samples:
   * - SampleMaskIn samples (PA_SC_AA_CONFIG.MSAA_EXPOSED_SAMPLES)
   * - SampleMaskOut samples (DB_EQAA.MASK_EXPORT_NUM_SAMPLES)
   * - Alpha-to-coverage samples (DB_EQAA.ALPHA_TO_MASK_NUM_SAMPLES)
   * - Occlusion query samples (DB_COUNT_CONTROL.SAMPLE_RATE)
   * # All are currently set the same as coverage samples.
   *
   * If color samples < coverage samples, FMASK has a higher bpp to store an "unknown"
   * flag for undefined color samples. A shader-based resolve must handle unknowns
   * or mask them out with AND. Unknowns can also be guessed from neighbors via
   * an edge-detect shader-based resolve, which is required to make "color samples = 1"
   * useful. The CB resolve always drops unknowns.
   *
   * Sensible AA configurations:
   *   EQAA 16s 8z 8f - might look the same as 16x MSAA if Z is compressed
   *   EQAA 16s 8z 4f - might look the same as 16x MSAA if Z is compressed
   *   EQAA 16s 4z 4f - might look the same as 16x MSAA if Z is compressed
   *   EQAA  8s 8z 8f = 8x MSAA
   *   EQAA  8s 8z 4f - might look the same as 8x MSAA
   *   EQAA  8s 8z 2f - might look the same as 8x MSAA with low-density geometry
   *   EQAA  8s 4z 4f - might look the same as 8x MSAA if Z is compressed
   *   EQAA  8s 4z 2f - might look the same as 8x MSAA with low-density geometry if Z is compressed
   *   EQAA  4s 4z 4f = 4x MSAA
   *   EQAA  4s 4z 2f - might look the same as 4x MSAA with low-density geometry
   *   EQAA  2s 2z 2f = 2x MSAA
   */
            /* DCC_DECOMPRESS and ELIMINATE_FAST_CLEAR require MSAA_NUM_SAMPLES=0. */
   if (sctx->gfx_level >= GFX11 && sctx->gfx11_force_msaa_num_samples_zero)
            /* The DX10 diamond test is not required by GL and decreases line rasterization
   * performance, so don't use it.
   */
   unsigned sc_line_cntl = 0;
            if (coverage_samples > 1 && (rs->multisample_enable ||
            /* distance from the pixel center, indexed by log2(nr_samples) */
   static unsigned max_dist[] = {
      0, /* unused */
   4, /* 2x MSAA */
   6, /* 4x MSAA */
   7, /* 8x MSAA */
      };
            sc_line_cntl |= S_028BDC_EXPAND_LINE_WIDTH(1) |
                  S_028BDC_PERPENDICULAR_ENDCAP_ENA(rs->perpendicular_end_caps) |
      sc_aa_config = S_028BE0_MSAA_NUM_SAMPLES(log_samples) |
                           if (sctx->framebuffer.nr_samples > 1 ||
      sctx->smoothing_enabled) {
   if (sctx->framebuffer.state.zsbuf) {
      z_samples = sctx->framebuffer.state.zsbuf->texture->nr_samples;
      } else {
         }
   unsigned log_samples = util_logbase2(coverage_samples);
   unsigned log_z_samples = util_logbase2(z_samples);
   unsigned ps_iter_samples = si_get_ps_iter_samples(sctx);
   unsigned log_ps_iter_samples = util_logbase2(ps_iter_samples);
   if (sctx->framebuffer.nr_samples > 1) {
      db_eqaa |= S_028804_MAX_ANCHOR_SAMPLES(log_z_samples) |
            S_028804_PS_ITER_SAMPLES(log_ps_iter_samples) |
         } else if (sctx->smoothing_enabled) {
                              /* R_028BDC_PA_SC_LINE_CNTL, R_028BE0_PA_SC_AA_CONFIG */
   radeon_opt_set_context_reg2(sctx, R_028BDC_PA_SC_LINE_CNTL, SI_TRACKED_PA_SC_LINE_CNTL,
         /* R_028804_DB_EQAA */
   radeon_opt_set_context_reg(sctx, R_028804_DB_EQAA, SI_TRACKED_DB_EQAA, db_eqaa);
   /* R_028A4C_PA_SC_MODE_CNTL_1 */
   radeon_opt_set_context_reg(sctx, R_028A4C_PA_SC_MODE_CNTL_1, SI_TRACKED_PA_SC_MODE_CNTL_1,
            }
      void si_update_ps_iter_samples(struct si_context *sctx)
   {
      if (sctx->framebuffer.nr_samples > 1)
         if (sctx->screen->dpbb_allowed)
      }
      static void si_set_min_samples(struct pipe_context *ctx, unsigned min_samples)
   {
               /* The hardware can only do sample shading with 2^n samples. */
            if (sctx->ps_iter_samples == min_samples)
                     si_ps_key_update_sample_shading(sctx);
   si_ps_key_update_framebuffer_rasterizer_sample_shading(sctx);
               }
      /*
   * Samplers
   */
      /**
   * Build the sampler view descriptor for a buffer texture.
   * @param state 256-bit descriptor; only the high 128 bits are filled in
   */
   void si_make_buffer_descriptor(struct si_screen *screen, struct si_resource *buf,
               {
      const struct util_format_description *desc;
   unsigned stride;
            desc = util_format_description(format);
            num_records = num_elements;
            /* The NUM_RECORDS field has a different meaning depending on the chip,
   * instruction type, STRIDE, and SWIZZLE_ENABLE.
   *
   * GFX6-7,10:
   * - If STRIDE == 0, it's in byte units.
   * - If STRIDE != 0, it's in units of STRIDE, used with inst.IDXEN.
   *
   * GFX8:
   * - For SMEM and STRIDE == 0, it's in byte units.
   * - For SMEM and STRIDE != 0, it's in units of STRIDE.
   * - For VMEM and STRIDE == 0 or SWIZZLE_ENABLE == 0, it's in byte units.
   * - For VMEM and STRIDE != 0 and SWIZZLE_ENABLE == 1, it's in units of STRIDE.
   * NOTE: There is incompatibility between VMEM and SMEM opcodes due to SWIZZLE_-
   *       ENABLE. The workaround is to set STRIDE = 0 if SWIZZLE_ENABLE == 0 when
   *       using SMEM. This can be done in the shader by clearing STRIDE with s_and.
   *       That way the same descriptor can be used by both SMEM and VMEM.
   *
   * GFX9:
   * - For SMEM and STRIDE == 0, it's in byte units.
   * - For SMEM and STRIDE != 0, it's in units of STRIDE.
   * - For VMEM and inst.IDXEN == 0 or STRIDE == 0, it's in byte units.
   * - For VMEM and inst.IDXEN == 1 and STRIDE != 0, it's in units of STRIDE.
   */
   if (screen->info.gfx_level == GFX8)
            state[4] = 0;
   state[5] = S_008F04_STRIDE(stride);
   state[6] = num_records;
   state[7] = S_008F0C_DST_SEL_X(si_map_swizzle(desc->swizzle[0])) |
            S_008F0C_DST_SEL_Y(si_map_swizzle(desc->swizzle[1])) |
         if (screen->info.gfx_level >= GFX10) {
               /* OOB_SELECT chooses the out-of-bounds check.
   *
   * GFX10:
   *  - 0: (index >= NUM_RECORDS) || (offset >= STRIDE)
   *  - 1: index >= NUM_RECORDS
   *  - 2: NUM_RECORDS == 0
   *  - 3: if SWIZZLE_ENABLE:
   *          swizzle_address >= NUM_RECORDS
   *       else:
   *          offset >= NUM_RECORDS
   *
   * GFX11:
   *  - 0: (index >= NUM_RECORDS) || (offset+payload > STRIDE)
   *  - 1: index >= NUM_RECORDS
   *  - 2: NUM_RECORDS == 0
   *  - 3: if SWIZZLE_ENABLE && STRIDE:
   *          (index >= NUM_RECORDS) || ( offset+payload > STRIDE)
   *       else:
   *          offset+payload > NUM_RECORDS
   */
   state[7] |= S_008F0C_FORMAT(fmt->img_format) |
            } else {
      int first_non_void;
            first_non_void = util_format_get_first_non_void_channel(format);
   num_format = si_translate_buffer_numformat(&screen->b, desc, first_non_void);
                  }
      static unsigned gfx9_border_color_swizzle(const unsigned char swizzle[4])
   {
               if (swizzle[3] == PIPE_SWIZZLE_X) {
      /* For the pre-defined border color values (white, opaque
   * black, transparent black), the only thing that matters is
   * that the alpha channel winds up in the correct place
   * (because the RGB channels are all the same) so either of
   * these enumerations will work.
   */
   if (swizzle[2] == PIPE_SWIZZLE_Y)
         else
      } else if (swizzle[0] == PIPE_SWIZZLE_X) {
      if (swizzle[1] == PIPE_SWIZZLE_Y)
         else
      } else if (swizzle[1] == PIPE_SWIZZLE_X) {
         } else if (swizzle[2] == PIPE_SWIZZLE_X) {
                     }
      /**
   * Translate the parameters to an image descriptor for CDNA image emulation.
   * In this function, we choose our own image descriptor format because we emulate image opcodes
   * using buffer opcodes.
   */
   static void cdna_emu_make_image_descriptor(struct si_screen *screen, struct si_texture *tex,
                                       {
               /* We don't need support these. We only need enough to support VAAPI and OpenMAX. */
   if (target == PIPE_TEXTURE_CUBE ||
      target == PIPE_TEXTURE_CUBE_ARRAY ||
   tex->buffer.b.b.last_level > 0 ||
   tex->buffer.b.b.nr_samples >= 2 ||
   desc->colorspace != UTIL_FORMAT_COLORSPACE_RGB ||
   desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED ||
   util_format_is_compressed(pipe_format)) {
   assert(!"unexpected texture type");
   memset(state, 0, 8 * 4);
               /* Adjust the image parameters according to the texture type. */
   switch (target) {
   case PIPE_TEXTURE_1D:
      height = 1;
      case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      depth = 1;
         case PIPE_TEXTURE_1D_ARRAY:
      height = 1;
      case PIPE_TEXTURE_2D_ARRAY:
      first_layer = MIN2(first_layer, tex->buffer.b.b.array_size - 1);
   last_layer = MIN2(last_layer, tex->buffer.b.b.array_size - 1);
   last_layer = MAX2(last_layer, first_layer);
   depth = last_layer - first_layer + 1;
         case PIPE_TEXTURE_3D:
      first_layer = 0;
         default:
                  unsigned stride = desc->block.bits / 8;
   uint64_t num_records = tex->surface.surf_size / stride;
            /* Prepare the format fields. */
   unsigned char swizzle[4];
            /* Buffer descriptor */
   state[0] = 0;
   state[1] = S_008F04_STRIDE(stride);
   state[2] = num_records;
   state[3] = S_008F0C_DST_SEL_X(si_map_swizzle(swizzle[0])) |
            S_008F0C_DST_SEL_Y(si_map_swizzle(swizzle[1])) |
         if (screen->info.gfx_level >= GFX10) {
               state[3] |= S_008F0C_FORMAT(fmt->img_format) |
            } else {
      int first_non_void = util_format_get_first_non_void_channel(pipe_format);
   unsigned num_format = si_translate_buffer_numformat(&screen->b, desc, first_non_void);
            state[3] |= S_008F0C_NUM_FORMAT(num_format) |
               /* Additional fields used by image opcode emulation. */
   state[4] = width | (height << 16);
   state[5] = depth | (first_layer << 16);
   state[6] = tex->surface.u.gfx9.surf_pitch;
      }
      /**
   * Build the sampler view descriptor for a texture.
   */
   static void gfx10_make_texture_descriptor(
      struct si_screen *screen, struct si_texture *tex, bool sampler, enum pipe_texture_target target,
   enum pipe_format pipe_format, const unsigned char state_swizzle[4], unsigned first_level,
   unsigned last_level, unsigned first_layer, unsigned last_layer, unsigned width, unsigned height,
      {
      if (!screen->info.has_image_opcodes && !get_bo_metadata) {
      cdna_emu_make_image_descriptor(screen, tex, sampler, target, pipe_format, state_swizzle,
                           struct pipe_resource *res = &tex->buffer.b.b;
   const struct util_format_description *desc;
   unsigned img_format;
   unsigned char swizzle[4];
   unsigned type;
            desc = util_format_description(pipe_format);
            if (desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
      const unsigned char swizzle_xxxx[4] = {0, 0, 0, 0};
   const unsigned char swizzle_yyyy[4] = {1, 1, 1, 1};
   const unsigned char swizzle_wwww[4] = {3, 3, 3, 3};
            switch (pipe_format) {
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_X32_S8X24_UINT:
   case PIPE_FORMAT_X8Z24_UNORM:
      util_format_compose_swizzles(swizzle_yyyy, state_swizzle, swizzle);
   is_stencil = true;
      case PIPE_FORMAT_X24S8_UINT:
      /*
   * X24S8 is implemented as an 8_8_8_8 data format, to
   * fix texture gathers. This affects at least
   * GL45-CTS.texture_cube_map_array.sampling on GFX8.
   */
   util_format_compose_swizzles(swizzle_wwww, state_swizzle, swizzle);
   is_stencil = true;
      default:
      util_format_compose_swizzles(swizzle_xxxx, state_swizzle, swizzle);
               if (tex->upgraded_depth && !is_stencil) {
      if (screen->info.gfx_level >= GFX11) {
      assert(img_format == V_008F0C_GFX11_FORMAT_32_FLOAT);
      } else {
      assert(img_format == V_008F0C_GFX10_FORMAT_32_FLOAT);
            } else {
                  if (!sampler && (res->target == PIPE_TEXTURE_CUBE || res->target == PIPE_TEXTURE_CUBE_ARRAY)) {
      /* For the purpose of shader images, treat cube maps as 2D
   * arrays.
   */
      } else {
                  if (type == V_008F1C_SQ_RSRC_IMG_1D_ARRAY) {
      height = 1;
      } else if (type == V_008F1C_SQ_RSRC_IMG_2D_ARRAY || type == V_008F1C_SQ_RSRC_IMG_2D_MSAA_ARRAY) {
      if (sampler || res->target != PIPE_TEXTURE_3D)
      } else if (type == V_008F1C_SQ_RSRC_IMG_CUBE)
            state[0] = 0;
   state[1] = S_00A004_FORMAT(img_format) | S_00A004_WIDTH_LO(width - 1);
   state[2] = S_00A008_WIDTH_HI((width - 1) >> 2) | S_00A008_HEIGHT(height - 1) |
            state[3] =
      S_00A00C_DST_SEL_X(si_map_swizzle(swizzle[0])) |
   S_00A00C_DST_SEL_Y(si_map_swizzle(swizzle[1])) |
   S_00A00C_DST_SEL_Z(si_map_swizzle(swizzle[2])) |
   S_00A00C_DST_SEL_W(si_map_swizzle(swizzle[3])) |
   S_00A00C_BASE_LEVEL(res->nr_samples > 1 ? 0 : first_level) |
   S_00A00C_LAST_LEVEL(res->nr_samples > 1 ? util_logbase2(res->nr_samples) : last_level) |
      /* Depth is the the last accessible layer on gfx9+. The hw doesn't need
   * to know the total number of layers.
   */
   state[4] =
      S_00A010_DEPTH((type == V_008F1C_SQ_RSRC_IMG_3D && sampler) ? depth - 1 : last_layer) |
      state[5] = S_00A014_ARRAY_PITCH(!!(type == V_008F1C_SQ_RSRC_IMG_3D && !sampler)) |
            unsigned max_mip = res->nr_samples > 1 ? util_logbase2(res->nr_samples) :
            if (screen->info.gfx_level >= GFX11) {
         } else {
         }
   state[6] = 0;
            if (vi_dcc_enabled(tex, first_level)) {
      state[6] |= S_00A018_MAX_UNCOMPRESSED_BLOCK_SIZE(V_028C78_MAX_BLOCK_SIZE_256B) |
                     /* Initialize the sampler view for FMASK. */
   if (tex->surface.fmask_offset) {
                  #define FMASK(s, f) (((unsigned)(MAX2(1, s)) * 16) + (MAX2(1, f)))
         switch (FMASK(res->nr_samples, res->nr_storage_samples)) {
   case FMASK(2, 1):
      format = V_008F0C_GFX10_FORMAT_FMASK8_S2_F1;
      case FMASK(2, 2):
      format = V_008F0C_GFX10_FORMAT_FMASK8_S2_F2;
      case FMASK(4, 1):
      format = V_008F0C_GFX10_FORMAT_FMASK8_S4_F1;
      case FMASK(4, 2):
      format = V_008F0C_GFX10_FORMAT_FMASK8_S4_F2;
      case FMASK(4, 4):
      format = V_008F0C_GFX10_FORMAT_FMASK8_S4_F4;
      case FMASK(8, 1):
      format = V_008F0C_GFX10_FORMAT_FMASK8_S8_F1;
      case FMASK(8, 2):
      format = V_008F0C_GFX10_FORMAT_FMASK16_S8_F2;
      case FMASK(8, 4):
      format = V_008F0C_GFX10_FORMAT_FMASK32_S8_F4;
      case FMASK(8, 8):
      format = V_008F0C_GFX10_FORMAT_FMASK32_S8_F8;
      case FMASK(16, 1):
      format = V_008F0C_GFX10_FORMAT_FMASK16_S16_F1;
      case FMASK(16, 2):
      format = V_008F0C_GFX10_FORMAT_FMASK32_S16_F2;
      case FMASK(16, 4):
      format = V_008F0C_GFX10_FORMAT_FMASK64_S16_F4;
      case FMASK(16, 8):
      format = V_008F0C_GFX10_FORMAT_FMASK64_S16_F8;
      default:
         #undef FMASK
         fmask_state[0] = (va >> 8) | tex->surface.fmask_tile_swizzle;
   fmask_state[1] = S_00A004_BASE_ADDRESS_HI(va >> 40) | S_00A004_FORMAT(format) |
         fmask_state[2] = S_00A008_WIDTH_HI((width - 1) >> 2) | S_00A008_HEIGHT(height - 1) |
         fmask_state[3] =
      S_00A00C_DST_SEL_X(V_008F1C_SQ_SEL_X) | S_00A00C_DST_SEL_Y(V_008F1C_SQ_SEL_X) |
   S_00A00C_DST_SEL_Z(V_008F1C_SQ_SEL_X) | S_00A00C_DST_SEL_W(V_008F1C_SQ_SEL_X) |
   S_00A00C_SW_MODE(tex->surface.u.gfx9.color.fmask_swizzle_mode) |
      fmask_state[4] = S_00A010_DEPTH(last_layer) | S_00A010_BASE_ARRAY(first_layer);
   fmask_state[5] = 0;
   fmask_state[6] = S_00A018_META_PIPE_ALIGNED(1);
         }
      /**
   * Build the sampler view descriptor for a texture (SI-GFX9).
   */
   static void si_make_texture_descriptor(struct si_screen *screen, struct si_texture *tex,
                                             {
      if (!screen->info.has_image_opcodes && !get_bo_metadata) {
      cdna_emu_make_image_descriptor(screen, tex, sampler, target, pipe_format, state_swizzle,
                           struct pipe_resource *res = &tex->buffer.b.b;
   const struct util_format_description *desc;
   unsigned char swizzle[4];
   int first_non_void;
   unsigned num_format, data_format, type, num_samples;
                     num_samples = desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS ? MAX2(1, res->nr_samples)
            if (desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
      const unsigned char swizzle_xxxx[4] = {0, 0, 0, 0};
   const unsigned char swizzle_yyyy[4] = {1, 1, 1, 1};
            switch (pipe_format) {
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_X32_S8X24_UINT:
   case PIPE_FORMAT_X8Z24_UNORM:
      util_format_compose_swizzles(swizzle_yyyy, state_swizzle, swizzle);
      case PIPE_FORMAT_X24S8_UINT:
      /*
   * X24S8 is implemented as an 8_8_8_8 data format, to
   * fix texture gathers. This affects at least
   * GL45-CTS.texture_cube_map_array.sampling on GFX8.
   */
   if (screen->info.gfx_level <= GFX8)
         else
            default:
            } else {
                           switch (pipe_format) {
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      num_format = V_008F14_IMG_NUM_FORMAT_UNORM;
      default:
      if (first_non_void < 0) {
      if (util_format_is_compressed(pipe_format)) {
      switch (pipe_format) {
   case PIPE_FORMAT_DXT1_SRGB:
   case PIPE_FORMAT_DXT1_SRGBA:
   case PIPE_FORMAT_DXT3_SRGBA:
   case PIPE_FORMAT_DXT5_SRGBA:
   case PIPE_FORMAT_BPTC_SRGBA:
   case PIPE_FORMAT_ETC2_SRGB8:
   case PIPE_FORMAT_ETC2_SRGB8A1:
   case PIPE_FORMAT_ETC2_SRGBA8:
      num_format = V_008F14_IMG_NUM_FORMAT_SRGB;
      case PIPE_FORMAT_RGTC1_SNORM:
   case PIPE_FORMAT_LATC1_SNORM:
   case PIPE_FORMAT_RGTC2_SNORM:
   case PIPE_FORMAT_LATC2_SNORM:
   case PIPE_FORMAT_ETC2_R11_SNORM:
   case PIPE_FORMAT_ETC2_RG11_SNORM:
   /* implies float, so use SNORM/UNORM to determine
         case PIPE_FORMAT_BPTC_RGB_FLOAT:
      num_format = V_008F14_IMG_NUM_FORMAT_SNORM;
      default:
      num_format = V_008F14_IMG_NUM_FORMAT_UNORM;
         } else if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
         } else {
            } else if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) {
         } else {
               switch (desc->channel[first_non_void].type) {
   case UTIL_FORMAT_TYPE_FLOAT:
      num_format = V_008F14_IMG_NUM_FORMAT_FLOAT;
      case UTIL_FORMAT_TYPE_SIGNED:
      if (desc->channel[first_non_void].normalized)
         else if (desc->channel[first_non_void].pure_integer)
         else
            case UTIL_FORMAT_TYPE_UNSIGNED:
      if (desc->channel[first_non_void].normalized)
         else if (desc->channel[first_non_void].pure_integer)
         else
                     data_format = si_translate_texformat(&screen->b, pipe_format, desc, first_non_void);
   if (data_format == ~0) {
                  /* S8 with Z32 HTILE needs a special format. */
   if (screen->info.gfx_level == GFX9 && pipe_format == PIPE_FORMAT_S8_UINT)
            if (!sampler && (res->target == PIPE_TEXTURE_CUBE || res->target == PIPE_TEXTURE_CUBE_ARRAY ||
            /* For the purpose of shader images, treat cube maps and 3D
   * textures as 2D arrays. For 3D textures, the address
   * calculations for mipmaps are different, so we rely on the
   * caller to effectively disable mipmaps.
   */
               } else {
                  if (type == V_008F1C_SQ_RSRC_IMG_1D_ARRAY) {
      height = 1;
      } else if (type == V_008F1C_SQ_RSRC_IMG_2D_ARRAY || type == V_008F1C_SQ_RSRC_IMG_2D_MSAA_ARRAY) {
      if (sampler || res->target != PIPE_TEXTURE_3D)
      } else if (type == V_008F1C_SQ_RSRC_IMG_CUBE)
            state[0] = 0;
   state[1] = (S_008F14_DATA_FORMAT(data_format) | S_008F14_NUM_FORMAT(num_format));
   state[2] = (S_008F18_WIDTH(width - 1) | S_008F18_HEIGHT(height - 1) | S_008F18_PERF_MOD(4));
   state[3] = (S_008F1C_DST_SEL_X(si_map_swizzle(swizzle[0])) |
               S_008F1C_DST_SEL_Y(si_map_swizzle(swizzle[1])) |
   S_008F1C_DST_SEL_Z(si_map_swizzle(swizzle[2])) |
   S_008F1C_DST_SEL_W(si_map_swizzle(swizzle[3])) |
   S_008F1C_BASE_LEVEL(num_samples > 1 ? 0 : first_level) |
   state[4] = 0;
   state[5] = S_008F24_BASE_ARRAY(first_layer);
   state[6] = 0;
            if (screen->info.gfx_level == GFX9) {
               /* Depth is the the last accessible layer on Gfx9.
   * The hw doesn't need to know the total number of layers.
   */
   if (type == V_008F1C_SQ_RSRC_IMG_3D)
         else
            state[4] |= S_008F20_BC_SWIZZLE(bc_swizzle);
   state[5] |= S_008F24_MAX_MIP(num_samples > 1 ? util_logbase2(num_samples)
      } else {
      state[3] |= S_008F1C_POW2_PAD(res->last_level > 0);
   state[4] |= S_008F20_DEPTH(depth - 1);
               if (vi_dcc_enabled(tex, first_level)) {
         } else {
      /* The last dword is unused by hw. The shader uses it to clear
   * bits in the first dword of sampler state.
   */
   if (screen->info.gfx_level <= GFX7 && res->nr_samples <= 1) {
      if (first_level == last_level)
         else
                  /* Initialize the sampler view for FMASK. */
   if (tex->surface.fmask_offset) {
                  #define FMASK(s, f) (((unsigned)(MAX2(1, s)) * 16) + (MAX2(1, f)))
         if (screen->info.gfx_level == GFX9) {
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK;
   switch (FMASK(res->nr_samples, res->nr_storage_samples)) {
   case FMASK(2, 1):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_8_2_1;
      case FMASK(2, 2):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_8_2_2;
      case FMASK(4, 1):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_8_4_1;
      case FMASK(4, 2):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_8_4_2;
      case FMASK(4, 4):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_8_4_4;
      case FMASK(8, 1):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_8_8_1;
      case FMASK(8, 2):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_16_8_2;
      case FMASK(8, 4):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_32_8_4;
      case FMASK(8, 8):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_32_8_8;
      case FMASK(16, 1):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_16_16_1;
      case FMASK(16, 2):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_32_16_2;
      case FMASK(16, 4):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_64_16_4;
      case FMASK(16, 8):
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_64_16_8;
      default:
            } else {
      switch (FMASK(res->nr_samples, res->nr_storage_samples)) {
   case FMASK(2, 1):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK8_S2_F1;
      case FMASK(2, 2):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK8_S2_F2;
      case FMASK(4, 1):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK8_S4_F1;
      case FMASK(4, 2):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK8_S4_F2;
      case FMASK(4, 4):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK8_S4_F4;
      case FMASK(8, 1):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK8_S8_F1;
      case FMASK(8, 2):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK16_S8_F2;
      case FMASK(8, 4):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK32_S8_F4;
      case FMASK(8, 8):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK32_S8_F8;
      case FMASK(16, 1):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK16_S16_F1;
      case FMASK(16, 2):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK32_S16_F2;
      case FMASK(16, 4):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK64_S16_F4;
      case FMASK(16, 8):
      data_format = V_008F14_IMG_DATA_FORMAT_FMASK64_S16_F8;
      default:
         }
      #undef FMASK
            fmask_state[0] = (va >> 8) | tex->surface.fmask_tile_swizzle;
   fmask_state[1] = S_008F14_BASE_ADDRESS_HI(va >> 40) | S_008F14_DATA_FORMAT(data_format) |
         fmask_state[2] = S_008F18_WIDTH(width - 1) | S_008F18_HEIGHT(height - 1);
   fmask_state[3] =
      S_008F1C_DST_SEL_X(V_008F1C_SQ_SEL_X) | S_008F1C_DST_SEL_Y(V_008F1C_SQ_SEL_X) |
   S_008F1C_DST_SEL_Z(V_008F1C_SQ_SEL_X) | S_008F1C_DST_SEL_W(V_008F1C_SQ_SEL_X) |
      fmask_state[4] = 0;
   fmask_state[5] = S_008F24_BASE_ARRAY(first_layer);
   fmask_state[6] = 0;
            if (screen->info.gfx_level == GFX9) {
      fmask_state[3] |= S_008F1C_SW_MODE(tex->surface.u.gfx9.color.fmask_swizzle_mode);
   fmask_state[4] |=
         fmask_state[5] |= S_008F24_META_PIPE_ALIGNED(1) |
      } else {
      fmask_state[3] |= S_008F1C_TILING_INDEX(tex->surface.u.legacy.color.fmask.tiling_index);
   fmask_state[4] |= S_008F20_DEPTH(depth - 1) |
                  }
      /**
   * Create a sampler view.
   *
   * @param ctx      context
   * @param texture  texture
   * @param state    sampler view template
   */
   static struct pipe_sampler_view *si_create_sampler_view(struct pipe_context *ctx,
               {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_sampler_view *view = CALLOC_STRUCT_CL(si_sampler_view);
   struct si_texture *tex = (struct si_texture *)texture;
   unsigned char state_swizzle[4];
   unsigned last_layer = state->u.tex.last_layer;
   enum pipe_format pipe_format;
            if (!view)
            /* initialize base object */
   view->base = *state;
   view->base.texture = NULL;
   view->base.reference.count = 1;
            assert(texture);
            if (state->format == PIPE_FORMAT_X24S8_UINT || state->format == PIPE_FORMAT_S8X24_UINT ||
      state->format == PIPE_FORMAT_X32_S8X24_UINT || state->format == PIPE_FORMAT_S8_UINT)
         /* Buffer resource. */
   if (texture->target == PIPE_BUFFER) {
      uint32_t elements = si_clamp_texture_texel_count(sctx->screen->max_texel_buffer_elements,
            si_make_buffer_descriptor(sctx->screen, si_resource(texture), state->format,
                     state_swizzle[0] = state->swizzle_r;
   state_swizzle[1] = state->swizzle_g;
   state_swizzle[2] = state->swizzle_b;
            /* This is not needed if gallium frontends set last_layer correctly. */
   if (state->target == PIPE_TEXTURE_1D || state->target == PIPE_TEXTURE_2D ||
      state->target == PIPE_TEXTURE_RECT || state->target == PIPE_TEXTURE_CUBE)
         /* Texturing with separate depth and stencil. */
            /* Depth/stencil texturing sometimes needs separate texture. */
   if (tex->is_depth && !si_can_sample_zs(tex, view->is_stencil_sampler)) {
      if (!tex->flushed_depth_texture && !si_init_flushed_depth_texture(ctx, texture)) {
      pipe_resource_reference(&view->base.texture, NULL);
   FREE(view);
                        /* Override format for the case where the flushed texture
   * contains only Z or only S.
   */
   if (tex->flushed_depth_texture->buffer.b.b.format != tex->buffer.b.b.format)
                                 if (tex->db_compatible) {
      if (!view->is_stencil_sampler)
            switch (pipe_format) {
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      pipe_format = PIPE_FORMAT_Z32_FLOAT;
      case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      /* Z24 is always stored like this for DB
   * compatibility.
   */
   pipe_format = PIPE_FORMAT_Z24X8_UNORM;
      case PIPE_FORMAT_X24S8_UINT:
   case PIPE_FORMAT_S8X24_UINT:
   case PIPE_FORMAT_X32_S8X24_UINT:
      pipe_format = PIPE_FORMAT_S8_UINT;
   surflevel = tex->surface.u.legacy.zs.stencil_level;
      default:;
               view->dcc_incompatible =
            sctx->screen->make_texture_descriptor(
      sctx->screen, tex, true, state->target, pipe_format, state_swizzle,
   state->u.tex.first_level, state->u.tex.last_level,
   state->u.tex.first_layer, last_layer, texture->width0, texture->height0, texture->depth0,
         view->base_level_info = &surflevel[0];
   view->block_width = util_format_get_blockwidth(pipe_format);
      }
      static void si_sampler_view_destroy(struct pipe_context *ctx, struct pipe_sampler_view *state)
   {
               pipe_resource_reference(&state->texture, NULL);
      }
      static bool wrap_mode_uses_border_color(unsigned wrap, bool linear_filter)
   {
      return wrap == PIPE_TEX_WRAP_CLAMP_TO_BORDER || wrap == PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER ||
      }
      static uint32_t si_translate_border_color(struct si_context *sctx,
               {
      bool linear_filter = state->min_img_filter != PIPE_TEX_FILTER_NEAREST ||
            if (!wrap_mode_uses_border_color(state->wrap_s, linear_filter) &&
      !wrap_mode_uses_border_color(state->wrap_t, linear_filter) &&
   !wrap_mode_uses_border_color(state->wrap_r, linear_filter))
      #define simple_border_types(elt)                                                                   \
      do {                                                                                            \
      if (color->elt[0] == 0 && color->elt[1] == 0 && color->elt[2] == 0 && color->elt[3] == 0)    \
         if (color->elt[0] == 0 && color->elt[1] == 0 && color->elt[2] == 0 && color->elt[3] == 1)    \
         if (color->elt[0] == 1 && color->elt[1] == 1 && color->elt[2] == 1 && color->elt[3] == 1)    \
               if (is_integer)
         else
         #undef simple_border_types
                  /* Check if the border has been uploaded already. */
   for (i = 0; i < sctx->border_color_count; i++)
      if (memcmp(&sctx->border_color_table[i], color, sizeof(*color)) == 0)
         if (i >= SI_MAX_BORDER_COLORS) {
      /* Getting 4096 unique border colors is very unlikely. */
   static bool printed;
   if (!printed) {
      fprintf(stderr, "radeonsi: The border color table is full. "
                  }
               if (i == sctx->border_color_count) {
      /* Upload a new border color. */
   memcpy(&sctx->border_color_table[i], color, sizeof(*color));
   util_memcpy_cpu_to_le32(&sctx->border_color_map[i], color, sizeof(*color));
               return (sctx->screen->info.gfx_level >= GFX11 ? S_008F3C_BORDER_COLOR_PTR_GFX11(i):
            }
      static inline int S_FIXED(float value, unsigned frac_bits)
   {
         }
      static inline unsigned si_tex_filter(unsigned filter, unsigned max_aniso)
   {
      if (filter == PIPE_TEX_FILTER_LINEAR)
      return max_aniso > 1 ? V_008F38_SQ_TEX_XY_FILTER_ANISO_BILINEAR
      else
      return max_aniso > 1 ? V_008F38_SQ_TEX_XY_FILTER_ANISO_POINT
   }
      static inline unsigned si_tex_aniso_filter(unsigned filter)
   {
      if (filter < 2)
         if (filter < 4)
         if (filter < 8)
         if (filter < 16)
            }
      static void *si_create_sampler_state(struct pipe_context *ctx,
         {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_screen *sscreen = sctx->screen;
   struct si_sampler_state *rstate = CALLOC_STRUCT(si_sampler_state);
   unsigned max_aniso = sscreen->force_aniso >= 0 ? sscreen->force_aniso : state->max_anisotropy;
   unsigned max_aniso_ratio = si_tex_aniso_filter(max_aniso);
   bool trunc_coord = (state->min_img_filter == PIPE_TEX_FILTER_NEAREST &&
                              if (!rstate) {
                  /* Validate inputs. */
   if (!is_wrap_mode_legal(sscreen, state->wrap_s) ||
      !is_wrap_mode_legal(sscreen, state->wrap_t) ||
   !is_wrap_mode_legal(sscreen, state->wrap_r) ||
   (!sscreen->info.has_3d_cube_border_color_mipmap &&
   (state->min_mip_filter != PIPE_TEX_MIPFILTER_NONE ||
         assert(0);
            #ifndef NDEBUG
         #endif
      rstate->val[0] =
      (S_008F30_CLAMP_X(si_tex_wrap(state->wrap_s)) | S_008F30_CLAMP_Y(si_tex_wrap(state->wrap_t)) |
   S_008F30_CLAMP_Z(si_tex_wrap(state->wrap_r)) | S_008F30_MAX_ANISO_RATIO(max_aniso_ratio) |
   S_008F30_DEPTH_COMPARE_FUNC(si_tex_compare(state->compare_mode, state->compare_func)) |
   S_008F30_FORCE_UNNORMALIZED(state->unnormalized_coords) |
   S_008F30_ANISO_THRESHOLD(max_aniso_ratio >> 1) | S_008F30_ANISO_BIAS(max_aniso_ratio) |
   S_008F30_DISABLE_CUBE_WRAP(!state->seamless_cube_map) |
      rstate->val[1] = (S_008F34_MIN_LOD(S_FIXED(CLAMP(state->min_lod, 0, 15), 8)) |
               rstate->val[2] = (S_008F38_XY_MAG_FILTER(si_tex_filter(state->mag_img_filter, max_aniso)) |
               rstate->val[3] = si_translate_border_color(sctx, state, &state->border_color,
            if (sscreen->info.gfx_level >= GFX10) {
      rstate->val[2] |= S_008F38_LOD_BIAS(S_FIXED(CLAMP(state->lod_bias, -32, 31), 8)) |
      } else {
      rstate->val[0] |= S_008F30_COMPAT_MODE(sctx->gfx_level >= GFX8);
   rstate->val[2] |= S_008F38_LOD_BIAS(S_FIXED(CLAMP(state->lod_bias, -16, 16), 8)) |
                           /* Create sampler resource for upgraded depth textures. */
            for (unsigned i = 0; i < 4; ++i) {
      /* Use channel 0 on purpose, so that we can use OPAQUE_WHITE
   * when the border color is 1.0. */
               if (memcmp(&state->border_color, &clamped_border_color, sizeof(clamped_border_color)) == 0) {
      if (sscreen->info.gfx_level <= GFX9)
      } else {
      rstate->upgraded_depth_val[3] =
                  }
      static void si_set_sample_mask(struct pipe_context *ctx, unsigned sample_mask)
   {
               if (sctx->sample_mask == (uint16_t)sample_mask)
            sctx->sample_mask = sample_mask;
      }
      static void si_emit_sample_mask(struct si_context *sctx, unsigned index)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
            /* Needed for line and polygon smoothing as well as for the Polaris
   * small primitive filter. We expect the gallium frontend to take care of
   * this for us.
   */
   assert(mask == 0xffff || sctx->framebuffer.nr_samples > 1 ||
            radeon_begin(cs);
   radeon_set_context_reg_seq(R_028C38_PA_SC_AA_MASK_X0Y0_X1Y0, 2);
   radeon_emit(mask | (mask << 16));
   radeon_emit(mask | (mask << 16));
      }
      static void si_delete_sampler_state(struct pipe_context *ctx, void *state)
   {
   #ifndef NDEBUG
               assert(s->magic == SI_SAMPLER_STATE_MAGIC);
      #endif
         }
      /*
   * Vertex elements & buffers
   */
      struct si_fast_udiv_info32 si_compute_fast_udiv_info32(uint32_t D, unsigned num_bits)
   {
               struct si_fast_udiv_info32 result = {
      info.multiplier,
   info.pre_shift,
   info.post_shift,
      };
      }
      static void *si_create_vertex_elements(struct pipe_context *ctx, unsigned count,
         {
               if (sscreen->debug_flags & DBG(VERTEX_ELEMENTS)) {
      for (int i = 0; i < count; ++i) {
      const struct pipe_vertex_element *e = elements + i;
   fprintf(stderr, "elements[%d]: offset %2d, buffer_index %d, dual_slot %d, format %3d, divisor %u\n",
                  struct si_vertex_elements *v = CALLOC_STRUCT(si_vertex_elements);
   struct si_fast_udiv_info32 divisor_factors[SI_MAX_ATTRIBS] = {};
   STATIC_ASSERT(sizeof(struct si_fast_udiv_info32) == 16);
   STATIC_ASSERT(sizeof(divisor_factors[0].multiplier) == 4);
   STATIC_ASSERT(sizeof(divisor_factors[0].pre_shift) == 4);
   STATIC_ASSERT(sizeof(divisor_factors[0].post_shift) == 4);
   STATIC_ASSERT(sizeof(divisor_factors[0].increment) == 4);
            assert(count <= SI_MAX_ATTRIBS);
   if (!v)
                     unsigned num_vbos_in_user_sgprs = si_num_vbos_in_user_sgprs(sscreen);
   unsigned alloc_count =
                  for (i = 0; i < count; ++i) {
      const struct util_format_description *desc;
   const struct util_format_channel_description *channel;
   int first_non_void;
            if (vbo_index >= SI_NUM_VERTEX_BUFFERS) {
      FREE(v);
               unsigned instance_divisor = elements[i].instance_divisor;
   if (instance_divisor) {
      if (instance_divisor == 1) {
         } else {
      v->instance_divisor_is_fetched |= 1u << i;
                  desc = util_format_description(elements[i].src_format);
   first_non_void = util_format_get_first_non_void_channel(elements[i].src_format);
            v->format_size[i] = desc->block.bits / 8;
   v->src_offset[i] = elements[i].src_offset;
   v->vertex_buffer_index[i] = vbo_index;
            bool always_fix = false;
   union si_vs_fix_fetch fix_fetch;
            fix_fetch.bits = 0;
            if (channel) {
      switch (channel->type) {
   case UTIL_FORMAT_TYPE_FLOAT:
      fix_fetch.u.format = AC_FETCH_FORMAT_FLOAT;
      case UTIL_FORMAT_TYPE_FIXED:
      fix_fetch.u.format = AC_FETCH_FORMAT_FIXED;
      case UTIL_FORMAT_TYPE_SIGNED: {
      if (channel->pure_integer)
         else if (channel->normalized)
         else
            }
   case UTIL_FORMAT_TYPE_UNSIGNED: {
      if (channel->pure_integer)
         else if (channel->normalized)
         else
            }
   default:
            } else {
      switch (elements[i].src_format) {
   case PIPE_FORMAT_R11G11B10_FLOAT:
      fix_fetch.u.format = AC_FETCH_FORMAT_FLOAT;
      default:
                     if (desc->channel[0].size == 10) {
                     /* The hardware always treats the 2-bit alpha channel as
   * unsigned, so a shader workaround is needed. The affected
   * chips are GFX8 and older except Stoney (GFX8.1).
   */
   always_fix = sscreen->info.gfx_level <= GFX8 && sscreen->info.family != CHIP_STONEY &&
      } else if (elements[i].src_format == PIPE_FORMAT_R11G11B10_FLOAT) {
      fix_fetch.u.log_size = 3; /* special encoding */
   fix_fetch.u.format = AC_FETCH_FORMAT_FIXED;
      } else {
                     /* Always fix up:
   * - doubles (multiple loads + truncate to float)
   * - 32-bit requiring a conversion
   */
   always_fix = (fix_fetch.u.log_size == 3) ||
                        /* Also fixup 8_8_8 and 16_16_16. */
   if (desc->nr_channels == 3 && fix_fetch.u.log_size <= 1) {
      always_fix = true;
                  if (desc->swizzle[0] != PIPE_SWIZZLE_X) {
      assert(desc->swizzle[0] == PIPE_SWIZZLE_Z &&
                     /* Force the workaround for unaligned access here already if the
   * offset relative to the vertex buffer base is unaligned.
   *
   * There is a theoretical case in which this is too conservative:
   * if the vertex buffer's offset is also unaligned in just the
   * right way, we end up with an aligned address after all.
   * However, this case should be extremely rare in practice (it
   * won't happen in well-behaved applications), and taking it
   * into account would complicate the fast path (where everything
   * is nicely aligned).
   */
   bool check_alignment =
         log_hw_load_size >= 1 &&
            if (check_alignment && ((elements[i].src_offset & ((1 << log_hw_load_size) - 1)) != 0 || elements[i].src_stride & 3))
            if (always_fix || check_alignment || opencode)
            if (opencode)
         if (opencode || always_fix)
            if (check_alignment && !opencode) {
               v->fix_fetch_unaligned |= 1 << i;
   v->hw_load_is_dword |= (log_hw_load_size - 1) << i;
               v->rsrc_word3[i] = S_008F0C_DST_SEL_X(si_map_swizzle(desc->swizzle[0])) |
                        if (sscreen->info.gfx_level >= GFX10) {
      const struct gfx10_format *fmt = &ac_get_gfx10_format_table(&sscreen->info)[elements[i].src_format];
   unsigned last_vertex_format = sscreen->info.gfx_level >= GFX11 ? 64 : 128;
   assert(fmt->img_format != 0 && fmt->img_format < last_vertex_format);
   v->rsrc_word3[i] |= S_008F0C_FORMAT(fmt->img_format) |
      } else {
      unsigned data_format, num_format;
   data_format = si_translate_buffer_dataformat(ctx->screen, desc, first_non_void);
   num_format = si_translate_buffer_numformat(ctx->screen, desc, first_non_void);
                  if (v->instance_divisor_is_fetched) {
               v->instance_divisor_factor_buffer = (struct si_resource *)pipe_buffer_create(
         if (!v->instance_divisor_factor_buffer) {
      FREE(v);
      }
   void *map =
            }
      }
      static void si_bind_vertex_elements(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_vertex_elements *old = sctx->vertex_elements;
            if (!v)
            sctx->vertex_elements = v;
   sctx->num_vertex_elements = v->count;
            if (old->instance_divisor_is_one != v->instance_divisor_is_one ||
      old->instance_divisor_is_fetched != v->instance_divisor_is_fetched ||
   (old->vb_alignment_check_mask ^ v->vb_alignment_check_mask) &
   sctx->vertex_buffer_unaligned ||
   ((v->vb_alignment_check_mask & sctx->vertex_buffer_unaligned) &&
   memcmp(old->vertex_buffer_index, v->vertex_buffer_index,
         /* fix_fetch_{always,opencode,unaligned} and hw_load_is_dword are
   * functions of fix_fetch and the src_offset alignment.
   * If they change and fix_fetch doesn't, it must be due to different
   * src_offset alignment, which is reflected in fix_fetch_opencode. */
   old->fix_fetch_opencode != v->fix_fetch_opencode ||
   memcmp(old->fix_fetch, v->fix_fetch, sizeof(v->fix_fetch[0]) *
         si_vs_key_update_inputs(sctx);
               if (v->instance_divisor_is_fetched) {
               cb.buffer = &v->instance_divisor_factor_buffer->b.b;
   cb.user_buffer = NULL;
   cb.buffer_offset = 0;
   cb.buffer_size = 0xffffffff;
         }
      static void si_delete_vertex_element(struct pipe_context *ctx, void *state)
   {
      struct si_context *sctx = (struct si_context *)ctx;
            if (sctx->vertex_elements == state)
            si_resource_reference(&v->instance_divisor_factor_buffer, NULL);
      }
      static void si_set_vertex_buffers(struct pipe_context *ctx, unsigned count,
               {
      struct si_context *sctx = (struct si_context *)ctx;
   unsigned updated_mask = u_bit_consecutive(0, count + unbind_num_trailing_slots);
   uint32_t orig_unaligned = sctx->vertex_buffer_unaligned;
   uint32_t unaligned = 0;
                     if (buffers) {
      if (take_ownership) {
      for (i = 0; i < count; i++) {
      const struct pipe_vertex_buffer *src = buffers + i;
   struct pipe_vertex_buffer *dst = sctx->vertex_buffer + i;
                                                if (buf) {
      si_resource(buf)->bind_history |= SI_BIND_VERTEX_BUFFER;
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(buf),
         }
   /* take_ownership allows us to copy pipe_resource pointers without refcounting. */
      } else {
      for (i = 0; i < count; i++) {
      const struct pipe_vertex_buffer *src = buffers + i;
   struct pipe_vertex_buffer *dst = sctx->vertex_buffer + i;
                                                if (buf) {
      si_resource(buf)->bind_history |= SI_BIND_VERTEX_BUFFER;
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(buf),
               } else {
      for (i = 0; i < count; i++)
               for (i = 0; i < unbind_num_trailing_slots; i++)
            sctx->vertex_buffers_dirty = sctx->num_vertex_elements > 0;
            /* Check whether alignment may have changed in a way that requires
   * shader changes. This check is conservative: a vertex buffer can only
   * trigger a shader change if the misalignment amount changes (e.g.
   * from byte-aligned to short-aligned), but we only keep track of
   * whether buffers are at least dword-aligned, since that should always
   * be the case in well-behaved applications anyway.
   */
   if ((sctx->vertex_elements->vb_alignment_check_mask &
      (unaligned | orig_unaligned) & updated_mask)) {
   si_vs_key_update_inputs(sctx);
         }
      static struct pipe_vertex_state *
   si_create_vertex_state(struct pipe_screen *screen,
                        struct pipe_vertex_buffer *buffer,
      {
      struct si_screen *sscreen = (struct si_screen *)screen;
            util_init_pipe_vertex_state(screen, buffer, elements, num_elements, indexbuf, full_velem_mask,
            /* Initialize the vertex element state in state->element.
   * Do it by creating a vertex element state object and copying it there.
   */
   struct si_context ctx = {};
   ctx.b.screen = screen;
   struct si_vertex_elements *velems = si_create_vertex_elements(&ctx.b, num_elements, elements);
   state->velems = *velems;
            assert(!state->velems.instance_divisor_is_one);
   assert(!state->velems.instance_divisor_is_fetched);
   assert(!state->velems.fix_fetch_always);
   assert(buffer->buffer_offset % 4 == 0);
   assert(!buffer->is_user_buffer);
   for (unsigned i = 0; i < num_elements; i++) {
      assert(elements[i].src_offset % 4 == 0);
   assert(!elements[i].dual_slot);
               for (unsigned i = 0; i < num_elements; i++) {
      si_set_vertex_buffer_descriptor(sscreen, &state->velems, &state->b.input.vbuffer, i,
                  }
      static void si_vertex_state_destroy(struct pipe_screen *screen,
         {
      pipe_vertex_buffer_unreference(&state->input.vbuffer);
   pipe_resource_reference(&state->input.indexbuf, NULL);
      }
      static struct pipe_vertex_state *
   si_pipe_create_vertex_state(struct pipe_screen *screen,
                                 {
               return util_vertex_state_cache_get(screen, buffer, elements, num_elements, indexbuf,
      }
      static void si_pipe_vertex_state_destroy(struct pipe_screen *screen,
         {
                  }
      /*
   * Misc
   */
      static void si_set_tess_state(struct pipe_context *ctx, const float default_outer_level[4],
         {
      struct si_context *sctx = (struct si_context *)ctx;
   struct pipe_constant_buffer cb;
            memcpy(array, default_outer_level, sizeof(float) * 4);
            cb.buffer = NULL;
   cb.user_buffer = array;
   cb.buffer_offset = 0;
               }
      static void si_texture_barrier(struct pipe_context *ctx, unsigned flags)
   {
                        /* Multisample surfaces are flushed in si_decompress_textures. */
   if (sctx->framebuffer.uncompressed_cb_mask) {
      si_make_CB_shader_coherent(sctx, sctx->framebuffer.nr_samples,
               }
      /* This only ensures coherency for shader image/buffer stores. */
   static void si_memory_barrier(struct pipe_context *ctx, unsigned flags)
   {
               if (!(flags & ~PIPE_BARRIER_UPDATE))
            /* Subsequent commands must wait for all shader invocations to
   * complete. */
   sctx->flags |= SI_CONTEXT_PS_PARTIAL_FLUSH | SI_CONTEXT_CS_PARTIAL_FLUSH |
            if (flags & PIPE_BARRIER_CONSTANT_BUFFER)
            if (flags & (PIPE_BARRIER_VERTEX_BUFFER | PIPE_BARRIER_SHADER_BUFFER | PIPE_BARRIER_TEXTURE |
            /* As far as I can tell, L1 contents are written back to L2
   * automatically at end of shader, but the contents of other
   * L1 caches might still be stale. */
            if (flags & (PIPE_BARRIER_IMAGE | PIPE_BARRIER_TEXTURE) &&
      sctx->screen->info.tcc_rb_non_coherent)
            if (flags & PIPE_BARRIER_INDEX_BUFFER) {
      /* Indices are read through TC L2 since GFX8.
   * L1 isn't used.
   */
   if (sctx->screen->info.gfx_level <= GFX7)
               /* MSAA color, any depth and any stencil are flushed in
   * si_decompress_textures when needed.
   */
   if (flags & PIPE_BARRIER_FRAMEBUFFER && sctx->framebuffer.uncompressed_cb_mask) {
               if (sctx->gfx_level <= GFX8)
               /* Indirect buffers use TC L2 on GFX9, but not older hw. */
   if (sctx->screen->info.gfx_level <= GFX8 && flags & PIPE_BARRIER_INDIRECT_BUFFER)
               }
      static void *si_create_blend_custom(struct si_context *sctx, unsigned mode)
   {
               memset(&blend, 0, sizeof(blend));
   blend.independent_blend_enable = true;
   blend.rt[0].colormask = 0xf;
      }
      static void si_emit_cache_flush_state(struct si_context *sctx, unsigned index)
   {
         }
      void si_init_state_compute_functions(struct si_context *sctx)
   {
      sctx->b.create_sampler_state = si_create_sampler_state;
   sctx->b.delete_sampler_state = si_delete_sampler_state;
   sctx->b.create_sampler_view = si_create_sampler_view;
   sctx->b.sampler_view_destroy = si_sampler_view_destroy;
      }
      void si_init_state_functions(struct si_context *sctx)
   {
      sctx->atoms.s.pm4_states[SI_STATE_IDX(blend)].emit = si_pm4_emit_state;
   sctx->atoms.s.pm4_states[SI_STATE_IDX(rasterizer)].emit = si_pm4_emit_state;
   sctx->atoms.s.pm4_states[SI_STATE_IDX(dsa)].emit = si_pm4_emit_state;
   sctx->atoms.s.pm4_states[SI_STATE_IDX(poly_offset)].emit = si_pm4_emit_state;
   sctx->atoms.s.pm4_states[SI_STATE_IDX(sqtt_pipeline)].emit = si_pm4_emit_state;
   sctx->atoms.s.pm4_states[SI_STATE_IDX(ls)].emit = si_pm4_emit_shader;
   sctx->atoms.s.pm4_states[SI_STATE_IDX(hs)].emit = si_pm4_emit_shader;
   sctx->atoms.s.pm4_states[SI_STATE_IDX(es)].emit = si_pm4_emit_shader;
   sctx->atoms.s.pm4_states[SI_STATE_IDX(gs)].emit = si_pm4_emit_shader;
   sctx->atoms.s.pm4_states[SI_STATE_IDX(vs)].emit = si_pm4_emit_shader;
            sctx->atoms.s.framebuffer.emit = si_emit_framebuffer_state;
   sctx->atoms.s.db_render_state.emit = si_emit_db_render_state;
   sctx->atoms.s.dpbb_state.emit = si_emit_dpbb_state;
   sctx->atoms.s.msaa_config.emit = si_emit_msaa_config;
   sctx->atoms.s.sample_mask.emit = si_emit_sample_mask;
   sctx->atoms.s.cb_render_state.emit = si_emit_cb_render_state;
   sctx->atoms.s.blend_color.emit = si_emit_blend_color;
   sctx->atoms.s.clip_regs.emit = si_emit_clip_regs;
   sctx->atoms.s.clip_state.emit = si_emit_clip_state;
   sctx->atoms.s.stencil_ref.emit = si_emit_stencil_ref;
            sctx->b.create_blend_state = si_create_blend_state;
   sctx->b.bind_blend_state = si_bind_blend_state;
   sctx->b.delete_blend_state = si_delete_blend_state;
            sctx->b.create_rasterizer_state = si_create_rs_state;
   sctx->b.bind_rasterizer_state = si_bind_rs_state;
            sctx->b.create_depth_stencil_alpha_state = si_create_dsa_state;
   sctx->b.bind_depth_stencil_alpha_state = si_bind_dsa_state;
                     if (sctx->gfx_level < GFX11) {
      sctx->custom_blend_resolve = si_create_blend_custom(sctx, V_028808_CB_RESOLVE);
   sctx->custom_blend_fmask_decompress = si_create_blend_custom(sctx, V_028808_CB_FMASK_DECOMPRESS);
   sctx->custom_blend_eliminate_fastclear =
               sctx->custom_blend_dcc_decompress =
      si_create_blend_custom(sctx, sctx->gfx_level >= GFX11 ?
               sctx->b.set_clip_state = si_set_clip_state;
                              sctx->b.create_vertex_elements_state = si_create_vertex_elements;
   sctx->b.bind_vertex_elements_state = si_bind_vertex_elements;
   sctx->b.delete_vertex_elements_state = si_delete_vertex_element;
            sctx->b.texture_barrier = si_texture_barrier;
   sctx->b.set_min_samples = si_set_min_samples;
               }
      void si_init_screen_state_functions(struct si_screen *sscreen)
   {
      sscreen->b.is_format_supported = si_is_format_supported;
   sscreen->b.create_vertex_state = si_pipe_create_vertex_state;
            if (sscreen->info.gfx_level >= GFX10)
         else
            util_vertex_state_cache_init(&sscreen->vertex_state_cache,
      }
      static void si_set_grbm_gfx_index(struct si_context *sctx, struct si_pm4_state *pm4, unsigned value)
   {
      unsigned reg = sctx->gfx_level >= GFX7 ? R_030800_GRBM_GFX_INDEX : R_00802C_GRBM_GFX_INDEX;
      }
      static void si_set_grbm_gfx_index_se(struct si_context *sctx, struct si_pm4_state *pm4, unsigned se)
   {
      assert(se == ~0 || se < sctx->screen->info.max_se);
   si_set_grbm_gfx_index(sctx, pm4,
                  }
      static void si_write_harvested_raster_configs(struct si_context *sctx, struct si_pm4_state *pm4,
         {
      unsigned num_se = MAX2(sctx->screen->info.max_se, 1);
   unsigned raster_config_se[4];
                     for (se = 0; se < num_se; se++) {
      si_set_grbm_gfx_index_se(sctx, pm4, se);
      }
            if (sctx->gfx_level >= GFX7) {
            }
      static void si_set_raster_config(struct si_context *sctx, struct si_pm4_state *pm4)
   {
      struct si_screen *sscreen = sctx->screen;
   unsigned num_rb = MIN2(sscreen->info.max_render_backends, 16);
   uint64_t rb_mask = sscreen->info.enabled_rb_mask;
   unsigned raster_config = sscreen->pa_sc_raster_config;
            if (!rb_mask || util_bitcount64(rb_mask) >= num_rb) {
      /* Always use the default config when all backends are enabled
   * (or when we failed to determine the enabled backends).
   */
   si_pm4_set_reg(pm4, R_028350_PA_SC_RASTER_CONFIG, raster_config);
   if (sctx->gfx_level >= GFX7)
      } else {
            }
      unsigned gfx103_get_cu_mask_ps(struct si_screen *sscreen)
   {
      /* It's wasteful to enable all CUs for PS if shader arrays have a different
   * number of CUs. The reason is that the hardware sends the same number of PS
   * waves to each shader array, so the slowest shader array limits the performance.
   * Disable the extra CUs for PS in other shader arrays to save power and thus
   * increase clocks for busy CUs. In the future, we might disable or enable this
   * tweak only for certain apps.
   */
      }
      static void gfx6_init_gfx_preamble_state(struct si_context *sctx)
   {
      struct si_screen *sscreen = sctx->screen;
   uint64_t border_color_va =
         uint32_t compute_cu_en = S_00B858_SH0_CU_EN(sscreen->info.spi_cu_en) |
                  /* We need more space because the preamble is large. */
   struct si_pm4_state *pm4 = si_pm4_create_sized(sscreen, 214, sctx->has_graphics);
   if (!pm4)
            if (sctx->has_graphics && !sctx->shadowing.registers) {
      si_pm4_cmd_add(pm4, PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
   si_pm4_cmd_add(pm4, CC0_UPDATE_LOAD_ENABLES(1));
            if (sscreen->dpbb_allowed) {
      si_pm4_cmd_add(pm4, PKT3(PKT3_EVENT_WRITE, 0, 0));
               if (has_clear_state) {
      si_pm4_cmd_add(pm4, PKT3(PKT3_CLEAR_STATE, 0, 0));
                  /* Compute registers. */
   si_pm4_set_reg(pm4, R_00B834_COMPUTE_PGM_HI, S_00B834_DATA(sctx->screen->info.address32_hi >> 8));
   si_pm4_set_reg(pm4, R_00B858_COMPUTE_STATIC_THREAD_MGMT_SE0, compute_cu_en);
            if (sctx->gfx_level >= GFX7) {
      si_pm4_set_reg(pm4, R_00B864_COMPUTE_STATIC_THREAD_MGMT_SE2, compute_cu_en);
               if (sctx->gfx_level >= GFX9)
            /* Set the pointer to border colors. MI200 doesn't support border colors. */
   if (sctx->gfx_level >= GFX7 && sctx->border_color_buffer) {
      si_pm4_set_reg(pm4, R_030E00_TA_CS_BC_BASE_ADDR, border_color_va >> 8);
   si_pm4_set_reg(pm4, R_030E04_TA_CS_BC_BASE_ADDR_HI,
      } else if (sctx->gfx_level == GFX6) {
                  if (!sctx->has_graphics)
            /* Graphics registers. */
   /* CLEAR_STATE doesn't restore these correctly. */
   si_pm4_set_reg(pm4, R_028240_PA_SC_GENERIC_SCISSOR_TL, S_028240_WINDOW_OFFSET_DISABLE(1));
   si_pm4_set_reg(pm4, R_028244_PA_SC_GENERIC_SCISSOR_BR,
            si_pm4_set_reg(pm4, R_028A18_VGT_HOS_MAX_TESS_LEVEL, fui(64));
   if (!has_clear_state)
            if (!has_clear_state) {
      si_pm4_set_reg(pm4, R_028820_PA_CL_NANINF_CNTL, 0);
   si_pm4_set_reg(pm4, R_028AC0_DB_SRESULTS_COMPARE_STATE0, 0x0);
   si_pm4_set_reg(pm4, R_028AC4_DB_SRESULTS_COMPARE_STATE1, 0x0);
   si_pm4_set_reg(pm4, R_028AC8_DB_PRELOAD_CONTROL, 0x0);
   si_pm4_set_reg(pm4, R_02800C_DB_RENDER_OVERRIDE, 0);
            si_pm4_set_reg(pm4, R_028B98_VGT_STRMOUT_BUFFER_CONFIG, 0x0);
   si_pm4_set_reg(pm4, R_028A5C_VGT_GS_PER_VS, 0x2);
               si_pm4_set_reg(pm4, R_028080_TA_BC_BASE_ADDR, border_color_va >> 8);
   if (sctx->gfx_level >= GFX7)
            if (sctx->gfx_level == GFX6) {
      si_pm4_set_reg(pm4, R_008A14_PA_CL_ENHANCE,
               if (sctx->gfx_level >= GFX7) {
      si_pm4_set_reg(pm4, R_030A00_PA_SU_LINE_STIPPLE_VALUE, 0);
      } else {
      si_pm4_set_reg(pm4, R_008A60_PA_SU_LINE_STIPPLE_VALUE, 0);
               /* If any sample location uses the -8 coordinate, the EXCLUSION fields should be set to 0. */
   si_pm4_set_reg(pm4, R_02882C_PA_SU_PRIM_FILTER_CNTL,
                  if (sctx->family >= CHIP_POLARIS10 && !sctx->screen->info.has_small_prim_filter_sample_loc_bug) {
      /* Polaris10-12 should disable small line culling, but those also have the sample loc bug,
   * so they never enter this branch.
   */
   assert(sctx->family > CHIP_POLARIS12);
   si_pm4_set_reg(pm4, R_028830_PA_SU_SMALL_PRIM_FILTER_CNTL,
               if (sctx->gfx_level <= GFX7 || !has_clear_state) {
      si_pm4_set_reg(pm4, R_028C58_VGT_VERTEX_REUSE_BLOCK_CNTL, 14);
            /* CLEAR_STATE doesn't clear these correctly on certain generations.
   * I don't know why. Deduced by trial and error.
   */
   si_pm4_set_reg(pm4, R_028B28_VGT_STRMOUT_DRAW_OPAQUE_OFFSET, 0);
   si_pm4_set_reg(pm4, R_028204_PA_SC_WINDOW_SCISSOR_TL, S_028204_WINDOW_OFFSET_DISABLE(1));
   si_pm4_set_reg(pm4, R_028030_PA_SC_SCREEN_SCISSOR_TL, 0);
   si_pm4_set_reg(pm4, R_028034_PA_SC_SCREEN_SCISSOR_BR,
               if (sctx->gfx_level >= GFX7) {
      si_pm4_set_reg_idx3(pm4, R_00B01C_SPI_SHADER_PGM_RSRC3_PS,
                           if (sctx->gfx_level <= GFX8) {
               /* FIXME calculate these values somehow ??? */
   si_pm4_set_reg(pm4, R_028A54_VGT_GS_PER_ES, SI_GS_PER_ES);
            /* These registers, when written, also overwrite the CLEAR_STATE
   * context, so we can't rely on CLEAR_STATE setting them.
   * It would be an issue if there was another UMD changing them.
   */
   si_pm4_set_reg(pm4, R_028400_VGT_MAX_VTX_INDX, ~0);
   si_pm4_set_reg(pm4, R_028404_VGT_MIN_VTX_INDX, 0);
               if (sctx->gfx_level == GFX9) {
      si_pm4_set_reg(pm4, R_00B414_SPI_SHADER_PGM_HI_LS,
         si_pm4_set_reg(pm4, R_00B214_SPI_SHADER_PGM_HI_ES,
      } else {
      si_pm4_set_reg(pm4, R_00B524_SPI_SHADER_PGM_HI_LS,
               if (sctx->gfx_level >= GFX7 && sctx->gfx_level <= GFX8) {
      si_pm4_set_reg(pm4, R_00B51C_SPI_SHADER_PGM_RSRC3_LS,
               si_pm4_set_reg(pm4, R_00B41C_SPI_SHADER_PGM_RSRC3_HS, S_00B41C_WAVE_LIMIT(0x3F));
   si_pm4_set_reg(pm4, R_00B31C_SPI_SHADER_PGM_RSRC3_ES,
                  /* If this is 0, Bonaire can hang even if GS isn't being used.
   * Other chips are unaffected. These are suboptimal values,
   * but we don't use on-chip GS.
   */
   si_pm4_set_reg(pm4, R_028A44_VGT_GS_ONCHIP_CNTL,
               if (sctx->gfx_level >= GFX8) {
               if (sctx->gfx_level == GFX9) {
      vgt_tess_distribution = S_028B50_ACCUM_ISOLINE(12) |
                        } else {
      vgt_tess_distribution = S_028B50_ACCUM_ISOLINE(32) |
                        /* Testing with Unigine Heaven extreme tessellation yielded best results
   * with TRAP_SPLIT = 3.
   */
   if (sctx->family == CHIP_FIJI || sctx->family >= CHIP_POLARIS10)
                                    if (sctx->gfx_level == GFX9) {
      si_pm4_set_reg(pm4, R_030920_VGT_MAX_VTX_INDX, ~0);
   si_pm4_set_reg(pm4, R_030924_VGT_MIN_VTX_INDX, 0);
                     si_pm4_set_reg_idx3(pm4, R_00B41C_SPI_SHADER_PGM_RSRC3_HS,
                  si_pm4_set_reg(pm4, R_028C48_PA_SC_BINNER_CNTL_1,
               si_pm4_set_reg(pm4, R_028C4C_PA_SC_CONSERVATIVE_RASTERIZATION_CNTL,
            si_pm4_set_reg(pm4, R_028AAC_VGT_ESGS_RING_ITEMSIZE, 1);
            done:
      si_pm4_finalize(pm4);
   sctx->cs_preamble_state = pm4;
      }
      static void cdna_init_compute_preamble_state(struct si_context *sctx)
   {
      struct si_screen *sscreen = sctx->screen;
   uint64_t border_color_va =
         uint32_t compute_cu_en = S_00B858_SH0_CU_EN(sscreen->info.spi_cu_en) |
            struct si_pm4_state *pm4 = si_pm4_create_sized(sscreen, 48, true);
   if (!pm4)
            /* Compute registers. */
   /* Disable profiling on compute chips. */
   si_pm4_set_reg(pm4, R_00B82C_COMPUTE_PERFCOUNT_ENABLE, 0);
   si_pm4_set_reg(pm4, R_00B834_COMPUTE_PGM_HI, S_00B834_DATA(sctx->screen->info.address32_hi >> 8));
   si_pm4_set_reg(pm4, R_00B858_COMPUTE_STATIC_THREAD_MGMT_SE0, compute_cu_en);
   si_pm4_set_reg(pm4, R_00B85C_COMPUTE_STATIC_THREAD_MGMT_SE1, compute_cu_en);
   si_pm4_set_reg(pm4, R_00B864_COMPUTE_STATIC_THREAD_MGMT_SE2, compute_cu_en);
   si_pm4_set_reg(pm4, R_00B868_COMPUTE_STATIC_THREAD_MGMT_SE3, compute_cu_en);
            if (sscreen->info.family >= CHIP_GFX940) {
      si_pm4_set_reg(pm4, R_00B89C_COMPUTE_TG_CHUNK_SIZE, 0);
      } else {
      si_pm4_set_reg(pm4, R_00B894_COMPUTE_STATIC_THREAD_MGMT_SE4, compute_cu_en);
   si_pm4_set_reg(pm4, R_00B898_COMPUTE_STATIC_THREAD_MGMT_SE5, compute_cu_en);
   si_pm4_set_reg(pm4, R_00B89C_COMPUTE_STATIC_THREAD_MGMT_SE6, compute_cu_en);
                        /* Set the pointer to border colors. Only MI100 supports border colors. */
   if (sscreen->info.family == CHIP_MI100) {
      si_pm4_set_reg(pm4, R_030E00_TA_CS_BC_BASE_ADDR, border_color_va >> 8);
   si_pm4_set_reg(pm4, R_030E04_TA_CS_BC_BASE_ADDR_HI,
               si_pm4_finalize(pm4);
   sctx->cs_preamble_state = pm4;
      }
      static void gfx10_init_gfx_preamble_state(struct si_context *sctx)
   {
      struct si_screen *sscreen = sctx->screen;
   uint64_t border_color_va =
         uint32_t compute_cu_en = S_00B858_SH0_CU_EN(sscreen->info.spi_cu_en) |
         unsigned meta_write_policy, meta_read_policy;
   unsigned no_alloc = sctx->gfx_level >= GFX11 ? V_02807C_CACHE_NOA_GFX11:
         /* Enable CMASK/HTILE/DCC caching in L2 for small chips. */
   if (sscreen->info.max_render_backends <= 4) {
      meta_write_policy = V_02807C_CACHE_LRU_WR; /* cache writes */
      } else {
      meta_write_policy = V_02807C_CACHE_STREAM; /* write combine */
               /* We need more space because the preamble is large. */
   struct si_pm4_state *pm4 = si_pm4_create_sized(sscreen, 214, sctx->has_graphics);
   if (!pm4)
            if (sctx->has_graphics && !sctx->shadowing.registers) {
      si_pm4_cmd_add(pm4, PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
   si_pm4_cmd_add(pm4, CC0_UPDATE_LOAD_ENABLES(1));
            if (sscreen->dpbb_allowed) {
      si_pm4_cmd_add(pm4, PKT3(PKT3_EVENT_WRITE, 0, 0));
               si_pm4_cmd_add(pm4, PKT3(PKT3_CLEAR_STATE, 0, 0));
               /* Non-graphics uconfig registers. */
   if (sctx->gfx_level < GFX11)
         si_pm4_set_reg(pm4, R_030E00_TA_CS_BC_BASE_ADDR, border_color_va >> 8);
            /* Compute registers. */
   si_pm4_set_reg(pm4, R_00B834_COMPUTE_PGM_HI, S_00B834_DATA(sscreen->info.address32_hi >> 8));
   si_pm4_set_reg(pm4, R_00B858_COMPUTE_STATIC_THREAD_MGMT_SE0, compute_cu_en);
            si_pm4_set_reg(pm4, R_00B864_COMPUTE_STATIC_THREAD_MGMT_SE2, compute_cu_en);
            si_pm4_set_reg(pm4, R_00B890_COMPUTE_USER_ACCUM_0, 0);
   si_pm4_set_reg(pm4, R_00B894_COMPUTE_USER_ACCUM_1, 0);
   si_pm4_set_reg(pm4, R_00B898_COMPUTE_USER_ACCUM_2, 0);
            if (sctx->gfx_level >= GFX11) {
      si_pm4_set_reg(pm4, R_00B8AC_COMPUTE_STATIC_THREAD_MGMT_SE4, compute_cu_en);
   si_pm4_set_reg(pm4, R_00B8B0_COMPUTE_STATIC_THREAD_MGMT_SE5, compute_cu_en);
   si_pm4_set_reg(pm4, R_00B8B4_COMPUTE_STATIC_THREAD_MGMT_SE6, compute_cu_en);
            /* How many threads should go to 1 SE before moving onto the next. Think of GL1 cache hits.
   * Only these values are valid: 0 (disabled), 64, 128, 256, 512
   * Recommendation: 64 = RT, 256 = non-RT (run benchmarks to be sure)
   */
      } else {
                           if (!sctx->has_graphics)
            /* Shader registers - PS. */
   unsigned cu_mask_ps = sctx->gfx_level >= GFX10_3 ? gfx103_get_cu_mask_ps(sscreen) : ~0u;
   if (sctx->gfx_level < GFX11) {
      si_pm4_set_reg_idx3(pm4, R_00B004_SPI_SHADER_PGM_RSRC4_PS,
            }
   si_pm4_set_reg_idx3(pm4, R_00B01C_SPI_SHADER_PGM_RSRC3_PS,
                     ac_apply_cu_en(S_00B01C_CU_EN(cu_mask_ps) |
   si_pm4_set_reg(pm4, R_00B0C0_SPI_SHADER_REQ_CTRL_PS,
               si_pm4_set_reg(pm4, R_00B0C8_SPI_SHADER_USER_ACCUM_PS_0, 0);
   si_pm4_set_reg(pm4, R_00B0CC_SPI_SHADER_USER_ACCUM_PS_1, 0);
   si_pm4_set_reg(pm4, R_00B0D0_SPI_SHADER_USER_ACCUM_PS_2, 0);
            /* Shader registers - VS. */
   if (sctx->gfx_level < GFX11) {
      si_pm4_set_reg_idx3(pm4, R_00B104_SPI_SHADER_PGM_RSRC4_VS,
               si_pm4_set_reg(pm4, R_00B1C0_SPI_SHADER_REQ_CTRL_VS, 0);
   si_pm4_set_reg(pm4, R_00B1C8_SPI_SHADER_USER_ACCUM_VS_0, 0);
   si_pm4_set_reg(pm4, R_00B1CC_SPI_SHADER_USER_ACCUM_VS_1, 0);
   si_pm4_set_reg(pm4, R_00B1D0_SPI_SHADER_USER_ACCUM_VS_2, 0);
               /* Shader registers - GS. */
   si_pm4_set_reg(pm4, R_00B2C8_SPI_SHADER_USER_ACCUM_ESGS_0, 0);
   si_pm4_set_reg(pm4, R_00B2CC_SPI_SHADER_USER_ACCUM_ESGS_1, 0);
   si_pm4_set_reg(pm4, R_00B2D0_SPI_SHADER_USER_ACCUM_ESGS_2, 0);
   si_pm4_set_reg(pm4, R_00B2D4_SPI_SHADER_USER_ACCUM_ESGS_3, 0);
   si_pm4_set_reg(pm4, R_00B324_SPI_SHADER_PGM_HI_ES,
            /* Shader registers - HS. */
   if (sctx->gfx_level < GFX11) {
      si_pm4_set_reg_idx3(pm4, R_00B404_SPI_SHADER_PGM_RSRC4_HS,
            }
   si_pm4_set_reg_idx3(pm4, R_00B41C_SPI_SHADER_PGM_RSRC3_HS,
               si_pm4_set_reg(pm4, R_00B4C8_SPI_SHADER_USER_ACCUM_LSHS_0, 0);
   si_pm4_set_reg(pm4, R_00B4CC_SPI_SHADER_USER_ACCUM_LSHS_1, 0);
   si_pm4_set_reg(pm4, R_00B4D0_SPI_SHADER_USER_ACCUM_LSHS_2, 0);
   si_pm4_set_reg(pm4, R_00B4D4_SPI_SHADER_USER_ACCUM_LSHS_3, 0);
   si_pm4_set_reg(pm4, R_00B524_SPI_SHADER_PGM_HI_LS,
            /* Context registers. */
   if (sctx->gfx_level < GFX11) {
         }
   si_pm4_set_reg(pm4, R_02807C_DB_RMI_L2_CACHE_CONTROL,
                  S_02807C_Z_WR_POLICY(V_02807C_CACHE_STREAM) |
   S_02807C_S_WR_POLICY(V_02807C_CACHE_STREAM) |
   S_02807C_HTILE_WR_POLICY(meta_write_policy) |
   S_02807C_ZPCPSD_WR_POLICY(V_02807C_CACHE_STREAM) |
      si_pm4_set_reg(pm4, R_028080_TA_BC_BASE_ADDR, border_color_va >> 8);
            si_pm4_set_reg(pm4, R_028410_CB_RMI_GL2_CACHE_CONTROL,
                  (sctx->gfx_level >= GFX11 ?
      S_028410_DCC_WR_POLICY_GFX11(meta_write_policy) |
   S_028410_COLOR_WR_POLICY_GFX11(V_028410_CACHE_STREAM) |
      :
      S_028410_CMASK_WR_POLICY(meta_write_policy) |
   S_028410_FMASK_WR_POLICY(V_028410_CACHE_STREAM) |
   S_028410_DCC_WR_POLICY_GFX10(meta_write_policy) |
   S_028410_COLOR_WR_POLICY_GFX10(V_028410_CACHE_STREAM) |
            if (sctx->gfx_level >= GFX10_3)
            /* If any sample location uses the -8 coordinate, the EXCLUSION fields should be set to 0. */
   si_pm4_set_reg(pm4, R_02882C_PA_SU_PRIM_FILTER_CNTL,
               si_pm4_set_reg(pm4, R_028830_PA_SU_SMALL_PRIM_FILTER_CNTL,
         if (sctx->gfx_level >= GFX10_3) {
      /* The rate combiners have no effect if they are disabled like this:
   *   VERTEX_RATE:    BYPASS_VTX_RATE_COMBINER = 1
   *   PRIMITIVE_RATE: BYPASS_PRIM_RATE_COMBINER = 1
   *   HTILE_RATE:     VRS_HTILE_ENCODING = 0
   *   SAMPLE_ITER:    PS_ITER_SAMPLE = 0
   *
   * Use OVERRIDE, which will ignore results from previous combiners.
   * (e.g. enabled sample shading overrides the vertex rate)
   */
   si_pm4_set_reg(pm4, R_028848_PA_CL_VRS_CNTL,
                     si_pm4_set_reg(pm4, R_028A18_VGT_HOS_MAX_TESS_LEVEL, fui(64));
   si_pm4_set_reg(pm4, R_028AAC_VGT_ESGS_RING_ITEMSIZE, 1);
   si_pm4_set_reg(pm4, R_028B50_VGT_TESS_DISTRIBUTION,
                  sctx->gfx_level >= GFX11 ?
      S_028B50_ACCUM_ISOLINE(128) |
   S_028B50_ACCUM_TRI(128) |
   S_028B50_ACCUM_QUAD(128) |
   S_028B50_DONUT_SPLIT_GFX9(24) |
      :
      S_028B50_ACCUM_ISOLINE(12) |
            si_pm4_set_reg(pm4, R_028C48_PA_SC_BINNER_CNTL_1,
                  if (sctx->gfx_level >= GFX11_5)
      si_pm4_set_reg(pm4, R_028C54_PA_SC_BINNER_CNTL_2,
         /* Break up a pixel wave if it contains deallocs for more than
   * half the parameter cache.
   *
   * To avoid a deadlock where pixel waves aren't launched
   * because they're waiting for more pixels while the frontend
   * is stuck waiting for PC space, the maximum allowed value is
   * the size of the PC minus the largest possible allocation for
   * a single primitive shader subgroup.
   */
   si_pm4_set_reg(pm4, R_028C50_PA_SC_NGG_MODE_CNTL,
         if (sctx->gfx_level < GFX11)
            /* Uconfig registers. */
   si_pm4_set_reg(pm4, R_030924_GE_MIN_VTX_INDX, 0);
   si_pm4_set_reg(pm4, R_030928_GE_INDX_OFFSET, 0);
   if (sctx->gfx_level >= GFX11) {
      /* This is changed by draws for indexed draws, but we need to set DISABLE_FOR_AUTO_INDEX
   * here, which disables primitive restart for all non-indexed draws, so that those draws
   * won't have to set this state.
   */
      }
   si_pm4_set_reg(pm4, R_030964_GE_MAX_VTX_INDX, ~0);
   si_pm4_set_reg(pm4, R_030968_VGT_INSTANCE_BASE_ID, 0);
   si_pm4_set_reg(pm4, R_03097C_GE_STEREO_CNTL, 0);
            si_pm4_set_reg(pm4, R_030A00_PA_SU_LINE_STIPPLE_VALUE, 0);
            if (sctx->gfx_level >= GFX11) {
               si_pm4_cmd_add(pm4, PKT3(PKT3_EVENT_WRITE, 2, 0));
   si_pm4_cmd_add(pm4, EVENT_TYPE(V_028A90_PIXEL_PIPE_STAT_CONTROL) | EVENT_INDEX(1));
   si_pm4_cmd_add(pm4, PIXEL_PIPE_STATE_CNTL_COUNTER_ID(0) |
                        /* We must wait for idle using an EOP event before changing the attribute ring registers.
   * Use the bottom-of-pipe EOP event, but increment the PWS counter instead of writing memory.
   */
   si_pm4_cmd_add(pm4, PKT3(PKT3_RELEASE_MEM, 6, 0));
   si_pm4_cmd_add(pm4, S_490_EVENT_TYPE(V_028A90_BOTTOM_OF_PIPE_TS) |
               si_pm4_cmd_add(pm4, 0); /* DST_SEL, INT_SEL, DATA_SEL */
   si_pm4_cmd_add(pm4, 0); /* ADDRESS_LO */
   si_pm4_cmd_add(pm4, 0); /* ADDRESS_HI */
   si_pm4_cmd_add(pm4, 0); /* DATA_LO */
   si_pm4_cmd_add(pm4, 0); /* DATA_HI */
            /* Wait for the PWS counter. */
   si_pm4_cmd_add(pm4, PKT3(PKT3_ACQUIRE_MEM, 6, 0));
   si_pm4_cmd_add(pm4, S_580_PWS_STAGE_SEL(V_580_CP_ME) |
                     si_pm4_cmd_add(pm4, 0xffffffff); /* GCR_SIZE */
   si_pm4_cmd_add(pm4, 0x01ffffff); /* GCR_SIZE_HI */
   si_pm4_cmd_add(pm4, 0); /* GCR_BASE_LO */
   si_pm4_cmd_add(pm4, 0); /* GCR_BASE_HI */
   si_pm4_cmd_add(pm4, S_585_PWS_ENA(1));
            si_pm4_set_reg(pm4, R_031110_SPI_GS_THROTTLE_CNTL1, 0x12355123);
                     /* The PS will read inputs from this address. */
   si_pm4_set_reg(pm4, R_031118_SPI_ATTRIBUTE_RING_BASE,
         si_pm4_set_reg(pm4, R_03111C_SPI_ATTRIBUTE_RING_SIZE,
                        done:
      si_pm4_finalize(pm4);
   sctx->cs_preamble_state = pm4;
      }
      void si_init_gfx_preamble_state(struct si_context *sctx)
   {
      if (!sctx->screen->info.has_graphics)
         else if (sctx->gfx_level >= GFX10)
         else
      }
