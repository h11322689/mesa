   /*
   * Copyright 2023 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/glsl/list.h"
   #include "compiler/nir/nir_builder.h"
   #include "agx_compiler.h"
   #include "nir.h"
   #include "nir_builder_opcodes.h"
   #include "nir_intrinsics.h"
      /*
   * sample_mask takes two bitmasks as arguments, TARGET and LIVE. Each bit refers
   * to an indexed sample. Roughly, the instruction does:
   *
   *    foreach sample in TARGET {
   *       if sample in LIVE {
   *          run depth/stencil test and update
   *       } else {
   *          kill sample
   *       }
   *    }
   *
   * As a special case, TARGET may be set to all-1s (~0) to refer to all samples
   * regardless of the framebuffer sample count.
   *
   * For example, to discard an entire pixel unconditionally, we could run:
   *
   *    sample_mask ~0, 0
   *
   * sample_mask must follow these rules:
   *
   * 1. All sample_mask instructions affecting a sample must execute before a
   *    local_store_pixel instruction targeting that sample. This ensures that
   *    nothing is written for discarded samples (whether discarded in shader or
   *    due to a failed depth/stencil test).
   *
   * 2. If sample_mask is used anywhere in a shader, then on every execution path,
   *    every sample must be killed or else run depth/stencil tests exactly ONCE.
   *
   * 3. If a sample is killed, future sample_mask instructions have
   *    no effect on that sample. The following code sequence correctly implements
   *    a conditional discard (if there are no other sample_mask instructions in
   *    the shader):
   *
   *       sample_mask discarded, 0
   *       sample_mask ~0, ~0
   *
   *    but this sequence is incorrect:
   *
   *       sample_mask ~0, ~discarded
   *       sample_mask ~0, ~0         <-- incorrect: depth/stencil tests run twice
   *
   * 4. If zs_emit is used anywhere in the shader, sample_mask must not be used.
   * Instead, zs_emit with depth = NaN can be emitted.
   *
   * This pass lowers discard_agx to sample_mask instructions satisfying these
   * rules. Other passes should not generate sample_mask instructions, as there
   * are too many footguns.
   */
      #define ALL_SAMPLES (0xFF)
   #define BASE_Z      1
   #define BASE_S      2
      static bool
   lower_sample_mask_to_zs(nir_builder *b, nir_intrinsic_instr *intr,
         {
      bool depth_written =
         bool stencil_written =
                     /* Existing zs_emit instructions need to be fixed up to write their own depth
   * for consistency.
   */
   if (intr->intrinsic == nir_intrinsic_store_zs_agx && !depth_written) {
      /* Load the current depth at this pixel */
            /* Write it out from this store_zs */
   nir_intrinsic_set_base(intr, nir_intrinsic_base(intr) | BASE_Z);
            /* We'll set outputs_written after the pass in case there are multiple
   * store_zs_agx instructions needing fixup.
   */
   b->shader->info.fs.depth_layout = FRAG_DEPTH_LAYOUT_ANY;
               if (intr->intrinsic != nir_intrinsic_discard_agx)
            /* Write a NaN depth value for discarded samples */
   nir_store_zs_agx(b, intr->src[0].ssa, nir_imm_float(b, NAN),
                        nir_instr_remove(&intr->instr);
      }
      static bool
   lower_discard_to_sample_mask_0(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_discard_agx)
            b->cursor = nir_before_instr(&intr->instr);
   nir_sample_mask_agx(b, intr->src[0].ssa, nir_imm_intN_t(b, 0, 16));
   nir_instr_remove(&intr->instr);
      }
      static nir_intrinsic_instr *
   last_discard_in_block(nir_block *block)
   {
      nir_foreach_instr_reverse(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic == nir_intrinsic_discard_agx)
                  }
      static bool
   cf_node_contains_discard(nir_cf_node *node)
   {
      nir_foreach_block_in_cf_node(block, node) {
      if (last_discard_in_block(block))
                  }
      bool
   agx_nir_lower_sample_mask(nir_shader *shader, unsigned nr_samples)
   {
      if (!shader->info.fs.uses_discard)
            /* sample_mask can't be used with zs_emit, so lower sample_mask to zs_emit */
   if (shader->info.outputs_written & (BITFIELD64_BIT(FRAG_RESULT_DEPTH) |
            bool progress = nir_shader_intrinsics_pass(
                  /* The lowering requires an unconditional depth write. We mark this after
   * lowering so the lowering knows whether there was already a depth write
   */
   assert(progress && "must have lowered something,given the outputs");
                        /* We want to run depth/stencil tests as early as possible, but we have to
   * wait until after the last discard. We find the last discard and
   * execute depth/stencil tests in the first unconditional block after (if in
   * conditional control flow), or fuse depth/stencil tests into the sample
   * instruction (if in unconditional control flow).
   *
   * To do so, we walk the root control flow list backwards, looking for the
   * earliest unconditionally executed instruction after all discard.
   */
   nir_function_impl *impl = nir_shader_get_entrypoint(shader);
   nir_builder b = nir_builder_create(impl);
   foreach_list_typed_reverse(nir_cf_node, node, node, &impl->body) {
      if (node->type == nir_cf_node_block) {
      /* Unconditionally executed block */
                  if (intr) {
                     nir_def *all_samples = nir_imm_intN_t(&b, ALL_SAMPLES, 16);
                  nir_sample_mask_agx(&b, all_samples, live);
   nir_instr_remove(&intr->instr);
      } else {
      /* Set cursor for insertion due to a preceding conditionally
   * executed discard.
   */
         } else if (cf_node_contains_discard(node)) {
      /* Conditionally executed block contains the last discard. Test
   * depth/stencil for remaining samples in unconditional code after.
   */
   nir_sample_mask_agx(&b, nir_imm_intN_t(&b, ALL_SAMPLES, 16),
                        nir_shader_intrinsics_pass(shader, lower_discard_to_sample_mask_0,
                     }
