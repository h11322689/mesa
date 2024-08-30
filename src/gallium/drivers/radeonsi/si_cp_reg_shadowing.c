   /*
   * Copyright 2020 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_build_pm4.h"
   #include "ac_debug.h"
   #include "ac_shadowed_regs.h"
   #include "util/u_memory.h"
      static void si_set_context_reg_array(struct radeon_cmdbuf *cs, unsigned reg, unsigned num,
         {
      radeon_begin(cs);
   radeon_set_context_reg_seq(reg, num);
   radeon_emit_array(values, num);
      }
      void si_init_cp_reg_shadowing(struct si_context *sctx)
   {
      if (sctx->has_graphics &&
      sctx->screen->info.register_shadowing_required) {
   if (sctx->screen->info.has_fw_based_shadowing) {
      sctx->shadowing.registers =
         si_aligned_buffer_create(sctx->b.screen,
                     sctx->shadowing.csa =
         si_aligned_buffer_create(sctx->b.screen,
                     if (!sctx->shadowing.registers || !sctx->shadowing.csa)
         else
      sctx->ws->cs_set_mcbp_reg_shadowing_va(&sctx->gfx_cs,
         } else {
      sctx->shadowing.registers =
         si_aligned_buffer_create(sctx->b.screen,
                     if (!sctx->shadowing.registers)
                           if (sctx->shadowing.registers) {
      /* We need to clear the shadowed reg buffer. */
   si_cp_dma_clear_buffer(sctx, &sctx->gfx_cs, &sctx->shadowing.registers->b.b,
                  /* Create the shadowing preamble. (allocate enough dwords because the preamble is large) */
            ac_create_shadowing_ib_preamble(&sctx->screen->info,
                  /* Initialize shadowed registers as follows. */
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, sctx->shadowing.registers,
         if (sctx->shadowing.csa)
      radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, sctx->shadowing.csa,
      si_pm4_emit_commands(sctx, shadowing_preamble);
            /* TODO: Gfx11 fails GLCTS if we don't re-emit the preamble at the beginning of every IB. */
   if (sctx->gfx_level < GFX11) {
               /* The register values are shadowed, so we won't need to set them again. */
   si_pm4_free_state(sctx, sctx->cs_preamble_state, ~0);
                        /* Setup preemption. The shadowing preamble will be executed as a preamble IB,
   * which will load register values from memory on a context switch.
   */
   sctx->ws->cs_setup_preemption(&sctx->gfx_cs, shadowing_preamble->pm4,
               }
