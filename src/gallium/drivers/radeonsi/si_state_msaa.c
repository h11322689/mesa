   /*
   * Copyright 2014 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_build_pm4.h"
      /* For MSAA sample positions. */
   #define FILL_SREG(s0x, s0y, s1x, s1y, s2x, s2y, s3x, s3y)                                          \
      ((((unsigned)(s0x)&0xf) << 0) | (((unsigned)(s0y)&0xf) << 4) | (((unsigned)(s1x)&0xf) << 8) |   \
   (((unsigned)(s1y)&0xf) << 12) | (((unsigned)(s2x)&0xf) << 16) |                                \
         /* For obtaining location coordinates from registers */
   #define SEXT4(x)               ((int)((x) | ((x)&0x8 ? 0xfffffff0 : 0)))
   #define GET_SFIELD(reg, index) SEXT4(((reg) >> ((index)*4)) & 0xf)
   #define GET_SX(reg, index)     GET_SFIELD((reg)[(index) / 4], ((index) % 4) * 2)
   #define GET_SY(reg, index)     GET_SFIELD((reg)[(index) / 4], ((index) % 4) * 2 + 1)
      /* The following sample ordering is required by EQAA.
   *
   * Sample 0 is approx. in the top-left quadrant.
   * Sample 1 is approx. in the bottom-right quadrant.
   *
   * Sample 2 is approx. in the bottom-left quadrant.
   * Sample 3 is approx. in the top-right quadrant.
   * (sample I={2,3} adds more detail to the vicinity of sample I-2)
   *
   * Sample 4 is approx. in the same quadrant as sample 0. (top-left)
   * Sample 5 is approx. in the same quadrant as sample 1. (bottom-right)
   * Sample 6 is approx. in the same quadrant as sample 2. (bottom-left)
   * Sample 7 is approx. in the same quadrant as sample 3. (top-right)
   * (sample I={4,5,6,7} adds more detail to the vicinity of sample I-4)
   *
   * The next 8 samples add more detail to the vicinity of the previous samples.
   * (sample I (I >= 8) adds more detail to the vicinity of sample I-8)
   *
   * The ordering is specified such that:
   *   If we take the first 2 samples, we should get good 2x MSAA.
   *   If we add 2 more samples, we should get good 4x MSAA with the same sample locations.
   *   If we add 4 more samples, we should get good 8x MSAA with the same sample locations.
   *   If we add 8 more samples, we should get perfect 16x MSAA with the same sample locations.
   *
   * The ordering also allows finding samples in the same vicinity.
   *
   * Group N of 2 samples in the same vicinity in 16x MSAA: {N,N+8}
   * Group N of 2 samples in the same vicinity in 8x MSAA: {N,N+4}
   * Group N of 2 samples in the same vicinity in 4x MSAA: {N,N+2}
   *
   * Groups of 4 samples in the same vicinity in 16x MSAA:
   *   Top left:     {0,4,8,12}
   *   Bottom right: {1,5,9,13}
   *   Bottom left:  {2,6,10,14}
   *   Top right:    {3,7,11,15}
   *
   * Groups of 4 samples in the same vicinity in 8x MSAA:
   *   Left half:  {0,2,4,6}
   *   Right half: {1,3,5,7}
   *
   * Groups of 8 samples in the same vicinity in 16x MSAA:
   *   Left half:  {0,2,4,6,8,10,12,14}
   *   Right half: {1,3,5,7,9,11,13,15}
   */
      /* Important note: We have to use the standard DX positions because shader-based culling
   * relies on them.
   */
      /* 1x MSAA */
   static const uint32_t sample_locs_1x =
         static const uint64_t centroid_priority_1x = 0x0000000000000000ull;
      /* 2x MSAA (the positions are sorted for EQAA) */
   static const uint32_t sample_locs_2x =
         static const uint64_t centroid_priority_2x = 0x1010101010101010ull;
      /* 4x MSAA (the positions are sorted for EQAA) */
   static const uint32_t sample_locs_4x = FILL_SREG(-2, -6, 2, 6, -6, 2, 6, -2);
   static const uint64_t centroid_priority_4x = 0x3210321032103210ull;
      /* 8x MSAA (the positions are sorted for EQAA) */
   static const uint32_t sample_locs_8x[] = {
      FILL_SREG(-3, -5, 5, 1, -1, 3, 7, -7),
   FILL_SREG(-7, -1, 3, 7, -5, 5, 1, -3),
   /* The following are unused by hardware, but we emit them to IBs
   * instead of multiple SET_CONTEXT_REG packets. */
   0,
      };
   static const uint64_t centroid_priority_8x = 0x3546012735460127ull;
      /* 16x MSAA (the positions are sorted for EQAA) */
   static const uint32_t sample_locs_16x[] = {
      FILL_SREG(-5, -2, 5, 3, -2, 6, 3, -5),
   FILL_SREG(-4, -6, 1, 1, -6, 4, 7, -4),
   FILL_SREG(-1, -3, 6, 7, -3, 2, 0, -7),
   /* We use -7 where DX sample locations want -8, which allows us to make
   * the PA_SU_PRIM_FILTER_CNTL register immutable. That's a quality compromise
   * for underused 16x EQAA.
   */
      };
   static const uint64_t centroid_priority_16x = 0xc97e64b231d0fa85ull;
      static void si_get_sample_position(struct pipe_context *ctx, unsigned sample_count,
         {
               switch (sample_count) {
   case 1:
   default:
      sample_locs = &sample_locs_1x;
      case 2:
      sample_locs = &sample_locs_2x;
      case 4:
      sample_locs = &sample_locs_4x;
      case 8:
      sample_locs = sample_locs_8x;
      case 16:
      sample_locs = sample_locs_16x;
               out_value[0] = (GET_SX(sample_locs, sample_index) + 8) / 16.0f;
      }
      static void si_emit_max_4_sample_locs(struct radeon_cmdbuf *cs, uint64_t centroid_priority,
         {
      radeon_begin(cs);
   radeon_set_context_reg_seq(R_028BD4_PA_SC_CENTROID_PRIORITY_0, 2);
   radeon_emit(centroid_priority);
   radeon_emit(centroid_priority >> 32);
   radeon_set_context_reg(R_028BF8_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0, sample_locs);
   radeon_set_context_reg(R_028C08_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_0, sample_locs);
   radeon_set_context_reg(R_028C18_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_0, sample_locs);
   radeon_set_context_reg(R_028C28_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_0, sample_locs);
      }
      static void si_emit_max_16_sample_locs(struct radeon_cmdbuf *cs, uint64_t centroid_priority,
         {
      radeon_begin(cs);
   radeon_set_context_reg_seq(R_028BD4_PA_SC_CENTROID_PRIORITY_0, 2);
   radeon_emit(centroid_priority);
   radeon_emit(centroid_priority >> 32);
   radeon_set_context_reg_seq(R_028BF8_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0,
         radeon_emit_array(sample_locs, 4);
   radeon_emit_array(sample_locs, 4);
   radeon_emit_array(sample_locs, 4);
   radeon_emit_array(sample_locs, num_samples == 8 ? 2 : 4);
      }
      static void si_emit_sample_locations(struct si_context *sctx, unsigned index)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
   struct si_state_rasterizer *rs = sctx->queued.named.rasterizer;
            /* Smoothing (only possible with nr_samples == 1) uses the same
   * sample locations as the MSAA it simulates.
   */
   if (nr_samples <= 1 && sctx->smoothing_enabled)
            /* Always set MSAA sample locations even with 1x MSAA for simplicity.
   *
   * The only chips that don't need to set them for 1x MSAA are GFX6-8 except Polaris,
   * but there is no benefit in not resetting them to 0 when changing framebuffers from MSAA
   * to non-MSAA.
   */
   if (nr_samples != sctx->sample_locs_num_samples) {
      switch (nr_samples) {
   default:
   case 1:
      si_emit_max_4_sample_locs(cs, centroid_priority_1x, sample_locs_1x);
      case 2:
      si_emit_max_4_sample_locs(cs, centroid_priority_2x, sample_locs_2x);
      case 4:
      si_emit_max_4_sample_locs(cs, centroid_priority_4x, sample_locs_4x);
      case 8:
      si_emit_max_16_sample_locs(cs, centroid_priority_8x, sample_locs_8x, 8);
      case 16:
      si_emit_max_16_sample_locs(cs, centroid_priority_16x, sample_locs_16x, 16);
      }
               if (sctx->screen->info.has_small_prim_filter_sample_loc_bug) {
      /* For hardware with the sample location bug, the problem is that in order to use the small
   * primitive filter, we need to explicitly set the sample locations to 0. But the DB doesn't
   * properly process the change of sample locations without a flush, and so we can end up
   * with incorrect Z values.
   *
   * Instead of doing a flush, just disable the small primitive filter when MSAA is
   * force-disabled.
   *
   * The alternative of setting sample locations to 0 would require a DB flush to avoid
   * Z errors, see https://bugs.freedesktop.org/show_bug.cgi?id=96908
   */
   bool small_prim_filter_enable = sctx->framebuffer.nr_samples <= 1 || rs->multisample_enable;
            radeon_begin(cs);
   radeon_opt_set_context_reg(sctx, R_028830_PA_SU_SMALL_PRIM_FILTER_CNTL,
                                 }
      void si_init_msaa_functions(struct si_context *sctx)
   {
               sctx->atoms.s.sample_locations.emit = si_emit_sample_locations;
                     for (i = 0; i < 2; i++)
         for (i = 0; i < 4; i++)
         for (i = 0; i < 8; i++)
         for (i = 0; i < 16; i++)
      }
