   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "agx_compiler.h"
   #include "nir_builder_opcodes.h"
      #define ALL_SAMPLES 0xFF
   #define BASE_Z      1
   #define BASE_S      2
      static bool
   lower_zs_emit(nir_block *block)
   {
      nir_intrinsic_instr *zs_emit = NULL;
            nir_foreach_instr_reverse_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_store_output)
            nir_io_semantics sem = nir_intrinsic_io_semantics(intr);
   if (sem.location != FRAG_RESULT_DEPTH &&
                           nir_def *value = intr->src[0].ssa;
            unsigned src_idx = z ? 1 : 2;
            /* In the hw, depth is 32-bit but stencil is 16-bit. Instruction
   * selection checks this, so emit the conversion now.
   */
   if (z)
         else
            if (zs_emit == NULL) {
      /* Multisampling will get lowered later if needed, default to
   * broadcast
   */
   nir_def *sample_mask = nir_imm_intN_t(&b, ALL_SAMPLES, 16);
   zs_emit =
      nir_store_zs_agx(&b, sample_mask, nir_undef(&b, 1, 32) /* depth */,
            assert((nir_intrinsic_base(zs_emit) & base) == 0 &&
            nir_src_rewrite(&zs_emit->src[src_idx], value);
            nir_instr_remove(instr);
                  }
      static bool
   lower_discard(nir_builder *b, nir_intrinsic_instr *intr, UNUSED void *data)
   {
      if (intr->intrinsic != nir_intrinsic_discard &&
      intr->intrinsic != nir_intrinsic_discard_if)
                  nir_def *all_samples = nir_imm_intN_t(b, ALL_SAMPLES, 16);
   nir_def *no_samples = nir_imm_intN_t(b, 0, 16);
            if (intr->intrinsic == nir_intrinsic_discard_if)
            /* This will get lowered later as needed */
   nir_discard_agx(b, killed_samples);
   nir_instr_remove(&intr->instr);
      }
      static bool
   agx_nir_lower_discard(nir_shader *s)
   {
      if (!s->info.fs.uses_discard)
            return nir_shader_intrinsics_pass(
      s, lower_discard, nir_metadata_block_index | nir_metadata_dominance,
   }
      static bool
   agx_nir_lower_zs_emit(nir_shader *s)
   {
      /* If depth/stencil isn't written, there's nothing to lower */
   if (!(s->info.outputs_written & (BITFIELD64_BIT(FRAG_RESULT_STENCIL) |
                           nir_foreach_function_impl(impl, s) {
               nir_foreach_block(block, impl) {
                  if (progress) {
      nir_metadata_preserve(
      } else {
                                 }
      bool
   agx_nir_lower_discard_zs_emit(nir_shader *s)
   {
               /* Lower depth/stencil writes before discard so the interaction works */
   progress |= agx_nir_lower_zs_emit(s);
               }
