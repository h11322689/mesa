   /*
   * Copyright 2023 Valve Corporation
   * Copyright 2014 Intel Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_intrinsics.h"
   #include "nir_intrinsics_indices.h"
   #include "nir_phi_builder.h"
   #include "nir_vla.h"
      static bool
   should_lower_reg(nir_intrinsic_instr *decl)
   {
      /* This pass only really works on "plain" registers. In particular,
   * base/indirects are not handled. If it's a packed or array register,
   * just set the value to NULL so that the rewrite portion of the pass
   * will know to ignore it.
   */
      }
      struct regs_to_ssa_state {
               /* Scratch bitset for use in setup_reg */
   unsigned defs_words;
            struct nir_phi_builder *phi_builder;
      };
      static void
   setup_reg(nir_intrinsic_instr *decl, struct regs_to_ssa_state *state)
   {
      assert(state->values[decl->def.index] == NULL);
   if (!should_lower_reg(decl))
            const unsigned num_components = nir_intrinsic_num_components(decl);
                     nir_foreach_reg_store(store, decl)
            state->values[decl->def.index] =
      nir_phi_builder_add_value(state->phi_builder, num_components,
   }
      static void
   rewrite_load(nir_intrinsic_instr *load, struct regs_to_ssa_state *state)
   {
      nir_block *block = load->instr.block;
            struct nir_phi_builder_value *value = state->values[reg->index];
   if (!value)
            nir_intrinsic_instr *decl = nir_instr_as_intrinsic(reg->parent_instr);
            nir_def_rewrite_uses(&load->def, def);
            if (nir_def_is_unused(&decl->def))
      }
      static void
   rewrite_store(nir_intrinsic_instr *store, struct regs_to_ssa_state *state)
   {
      nir_block *block = store->instr.block;
   nir_def *new_value = store->src[0].ssa;
            struct nir_phi_builder_value *value = state->values[reg->index];
   if (!value)
            nir_intrinsic_instr *decl = nir_instr_as_intrinsic(reg->parent_instr);
   unsigned num_components = nir_intrinsic_num_components(decl);
            /* Implement write masks by combining together the old/new values */
   if (write_mask != BITFIELD_MASK(num_components)) {
      nir_def *old_value =
            nir_def *channels[NIR_MAX_VEC_COMPONENTS] = { NULL };
            for (unsigned i = 0; i < num_components; ++i) {
      if (write_mask & BITFIELD_BIT(i))
         else
                           nir_phi_builder_value_set_block_def(value, block, new_value);
            if (nir_def_is_unused(&decl->def))
      }
      bool
   nir_lower_reg_intrinsics_to_ssa_impl(nir_function_impl *impl)
   {
      bool need_lower_reg = false;
   nir_foreach_reg_decl(reg, impl) {
      if (should_lower_reg(reg)) {
      need_lower_reg = true;
         }
   if (!need_lower_reg) {
      nir_metadata_preserve(impl, nir_metadata_all);
               nir_metadata_require(impl, nir_metadata_block_index |
                  void *dead_ctx = ralloc_context(NULL);
   struct regs_to_ssa_state state;
   state.b = nir_builder_create(impl);
   state.defs_words = BITSET_WORDS(impl->num_blocks);
   state.defs = ralloc_array(dead_ctx, BITSET_WORD, state.defs_words);
   state.phi_builder = nir_phi_builder_create(state.b.impl);
   state.values = rzalloc_array(dead_ctx, struct nir_phi_builder_value *,
            nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_decl_reg:
      setup_reg(intr, &state);
      case nir_intrinsic_load_reg:
      rewrite_load(intr, &state);
      case nir_intrinsic_store_reg:
      rewrite_store(intr, &state);
      default:
                                          nir_metadata_preserve(impl, nir_metadata_block_index |
            }
      bool
   nir_lower_reg_intrinsics_to_ssa(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
                     }
