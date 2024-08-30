   /*
   * Copyright 2012 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_pm4.h"
   #include "si_pipe.h"
   #include "si_build_pm4.h"
   #include "sid.h"
   #include "util/u_memory.h"
   #include "ac_debug.h"
      static void si_pm4_set_reg_custom(struct si_pm4_state *state, unsigned reg, uint32_t val,
            static bool opcode_is_packed(unsigned opcode)
   {
      return opcode == PKT3_SET_CONTEXT_REG_PAIRS_PACKED ||
            }
      static unsigned packed_opcode_to_unpacked(unsigned opcode)
   {
      switch (opcode) {
   case PKT3_SET_CONTEXT_REG_PAIRS_PACKED:
         case PKT3_SET_SH_REG_PAIRS_PACKED:
         default:
            }
      static unsigned unpacked_opcode_to_packed(struct si_pm4_state *state, unsigned opcode)
   {
      if (state->screen->info.has_set_pairs_packets) {
      switch (opcode) {
   case PKT3_SET_CONTEXT_REG:
         case PKT3_SET_SH_REG:
                        }
      static bool packed_next_is_reg_offset_pair(struct si_pm4_state *state)
   {
         }
      static bool packed_next_is_reg_value1(struct si_pm4_state *state)
   {
         }
      static bool packed_prev_is_reg_value0(struct si_pm4_state *state)
   {
         }
      static unsigned get_packed_reg_dw_offsetN(struct si_pm4_state *state, unsigned index)
   {
      unsigned i = state->last_pm4 + 2 + (index / 2) * 3;
   assert(i < state->ndw);
      }
      static unsigned get_packed_reg_valueN_idx(struct si_pm4_state *state, unsigned index)
   {
      unsigned i = state->last_pm4 + 2 + (index / 2) * 3 + 1 + (index % 2);
   assert(i < state->ndw);
      }
      static unsigned get_packed_reg_valueN(struct si_pm4_state *state, unsigned index)
   {
         }
      static unsigned get_packed_reg_count(struct si_pm4_state *state)
   {
      int body_size = state->ndw - state->last_pm4 - 2;
   assert(body_size > 0 && body_size % 3 == 0);
      }
      void si_pm4_finalize(struct si_pm4_state *state)
   {
      if (opcode_is_packed(state->last_opcode)) {
      unsigned reg_count = get_packed_reg_count(state);
            if (state->packed_is_padded)
                     /* If the whole packed SET packet only sets consecutive registers, rewrite the packet
   * to be unpacked to make it shorter.
   *
   * This also eliminates the invalid scenario when the packed SET packet sets only
   * 2 registers and the register offsets are equal due to padding.
   */
   for (unsigned i = 1; i < reg_count; i++) {
      if (reg_dw_offset0 != get_packed_reg_dw_offsetN(state, i) - i) {
      all_consecutive = false;
                  if (all_consecutive) {
      assert(state->ndw - state->last_pm4 == 2 + 3 * (reg_count + state->packed_is_padded) / 2);
   state->pm4[state->last_pm4] = PKT3(packed_opcode_to_unpacked(state->last_opcode),
         state->pm4[state->last_pm4 + 1] = reg_dw_offset0;
   for (unsigned i = 0; i < reg_count; i++)
         state->ndw = state->last_pm4 + 2 + reg_count;
      } else {
      /* Set reg_va_low_idx to where the shader address is stored in the pm4 state. */
   if (state->screen->debug_flags & DBG(SQTT) &&
      (state->last_opcode == PKT3_SET_SH_REG_PAIRS_PACKED ||
   state->last_opcode == PKT3_SET_SH_REG_PAIRS_PACKED_N)) {
                                    if (strstr(ac_get_register_name(state->screen->info.gfx_level,
                  state->spi_shader_pgm_lo_reg = reg_offset;
                     /* All SET_*_PAIRS* packets on the gfx queue must set RESET_FILTER_CAM. */
                  /* If it's a packed SET_SH packet, use the *_N variant when possible. */
   if (state->last_opcode == PKT3_SET_SH_REG_PAIRS_PACKED && reg_count <= 14) {
      state->pm4[state->last_pm4] &= PKT3_IT_OPCODE_C;
                     if (state->screen->debug_flags & DBG(SQTT) && state->last_opcode == PKT3_SET_SH_REG) {
      /* Set reg_va_low_idx to where the shader address is stored in the pm4 state. */
   unsigned reg_count = PKT_COUNT_G(state->pm4[state->last_pm4]);
            for (unsigned i = 0; i < reg_count; i++) {
      if (strstr(ac_get_register_name(state->screen->info.gfx_level,
                                       }
      static void si_pm4_cmd_begin(struct si_pm4_state *state, unsigned opcode)
   {
               assert(state->max_dw);
   assert(state->ndw < state->max_dw);
   assert(opcode <= 254);
   state->last_opcode = opcode;
   state->last_pm4 = state->ndw++;
      }
      void si_pm4_cmd_add(struct si_pm4_state *state, uint32_t dw)
   {
      assert(state->max_dw);
   assert(state->ndw < state->max_dw);
   state->pm4[state->ndw++] = dw;
      }
      static void si_pm4_cmd_end(struct si_pm4_state *state, bool predicate)
   {
      unsigned count;
   count = state->ndw - state->last_pm4 - 2;
            if (opcode_is_packed(state->last_opcode)) {
      if (packed_prev_is_reg_value0(state)) {
      /* Duplicate the first register at the end to make the number of registers aligned to 2. */
   si_pm4_set_reg_custom(state, get_packed_reg_dw_offsetN(state, 0) * 4,
                                 }
      static void si_pm4_set_reg_custom(struct si_pm4_state *state, unsigned reg, uint32_t val,
         {
      bool is_packed = opcode_is_packed(opcode);
            assert(state->max_dw);
            if (is_packed) {
      if (opcode != state->last_opcode) {
      si_pm4_cmd_begin(state, opcode); /* reserve space for the header */
         } else if (opcode != state->last_opcode || reg != (state->last_reg + 1) ||
            si_pm4_cmd_begin(state, opcode);
               assert(reg <= UINT16_MAX);
   state->last_reg = reg;
            if (is_packed) {
      if (state->packed_is_padded) {
      /* The packet is padded, which means the first register is written redundantly again
   * at the end. Remove it, so that we can replace it with this register.
   */
   state->packed_is_padded = false;
               if (packed_next_is_reg_offset_pair(state)) {
         } else if (packed_next_is_reg_value1(state)) {
      /* Set the second register offset in the high 16 bits. */
   state->pm4[state->ndw - 2] &= 0x0000ffff;
                  state->pm4[state->ndw++] = val;
      }
      void si_pm4_set_reg(struct si_pm4_state *state, unsigned reg, uint32_t val)
   {
               if (reg >= SI_CONFIG_REG_OFFSET && reg < SI_CONFIG_REG_END) {
      opcode = PKT3_SET_CONFIG_REG;
         } else if (reg >= SI_SH_REG_OFFSET && reg < SI_SH_REG_END) {
      opcode = PKT3_SET_SH_REG;
         } else if (reg >= SI_CONTEXT_REG_OFFSET && reg < SI_CONTEXT_REG_END) {
      opcode = PKT3_SET_CONTEXT_REG;
         } else if (reg >= CIK_UCONFIG_REG_OFFSET && reg < CIK_UCONFIG_REG_END) {
      opcode = PKT3_SET_UCONFIG_REG;
         } else {
      PRINT_ERR("Invalid register offset %08x!\n", reg);
                           }
      void si_pm4_set_reg_idx3(struct si_pm4_state *state, unsigned reg, uint32_t val)
   {
      if (state->screen->info.uses_kernel_cu_mask) {
      assert(state->screen->info.gfx_level >= GFX10);
      } else {
            }
      void si_pm4_clear_state(struct si_pm4_state *state, struct si_screen *sscreen,
         {
      state->screen = sscreen;
   state->ndw = 0;
            if (!state->max_dw)
      }
      void si_pm4_free_state(struct si_context *sctx, struct si_pm4_state *state, unsigned idx)
   {
      if (!state)
            if (idx != ~0) {
      if (sctx->emitted.array[idx] == state)
            if (sctx->queued.array[idx] == state) {
      sctx->queued.array[idx] = NULL;
                     }
      void si_pm4_emit_commands(struct si_context *sctx, struct si_pm4_state *state)
   {
               radeon_begin(cs);
   radeon_emit_array(state->pm4, state->ndw);
      }
      void si_pm4_emit_state(struct si_context *sctx, unsigned index)
   {
      struct si_pm4_state *state = sctx->queued.array[index];
            /* All places should unset dirty_states if this doesn't pass. */
            radeon_begin(cs);
   radeon_emit_array(state->pm4, state->ndw);
               }
      void si_pm4_emit_shader(struct si_context *sctx, unsigned index)
   {
                        radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, ((struct si_shader*)state)->bo,
         if (state->atom.emit)
      }
      void si_pm4_reset_emitted(struct si_context *sctx)
   {
               for (unsigned i = 0; i < SI_NUM_STATES; i++) {
      if (sctx->queued.array[i])
         }
      struct si_pm4_state *si_pm4_create_sized(struct si_screen *sscreen, unsigned max_dw,
         {
      struct si_pm4_state *pm4;
            pm4 = (struct si_pm4_state *)calloc(1, size);
   if (pm4) {
      pm4->max_dw = max_dw;
      }
      }
      struct si_pm4_state *si_pm4_clone(struct si_pm4_state *orig)
   {
      struct si_pm4_state *pm4 = si_pm4_create_sized(orig->screen, orig->max_dw,
         if (pm4)
            }
