   /*
   * Copyright 2023 Alyssa Rosenzweig
   * Copyright 2021 Intel Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "agx_tilebuffer.h"
   #include "nir.h"
   #include "nir_builder.h"
      static bool
   lower_wrapped(nir_builder *b, nir_instr *instr, void *data)
   {
      nir_def *sample_id = data;
   if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
            switch (intr->intrinsic) {
   case nir_intrinsic_load_sample_id: {
      unsigned size = intr->def.bit_size;
   nir_def_rewrite_uses(&intr->def, nir_u2uN(b, sample_id, size));
   nir_instr_remove(instr);
               case nir_intrinsic_load_local_pixel_agx:
   case nir_intrinsic_store_local_pixel_agx:
   case nir_intrinsic_store_zs_agx:
   case nir_intrinsic_discard_agx: {
      /* Fragment I/O inside the loop should only affect one sample. */
   unsigned mask_index =
            nir_def *mask = intr->src[mask_index].ssa;
   nir_def *id_mask = nir_ishl(b, nir_imm_intN_t(b, 1, mask->bit_size),
         nir_src_rewrite(&intr->src[mask_index], nir_iand(b, mask, id_mask));
               default:
            }
      /*
   * In a monolithic pixel shader, we wrap the fragment shader in a loop over
   * each sample, and then let optimizations (like loop unrolling) go to town.
   * This lowering is not compatible with fragment epilogues, which require
   * something similar at the binary level since the NIR is long gone by then.
   */
   static bool
   agx_nir_wrap_per_sample_loop(nir_shader *shader, uint8_t nr_samples)
   {
               /* Get the original function */
            nir_cf_list list;
            /* Create a builder for the wrapped function */
            nir_variable *i =
         nir_store_var(&b, i, nir_imm_intN_t(&b, 0, 16), ~0);
            /* Create a loop in the wrapped function */
   nir_loop *loop = nir_push_loop(&b);
   {
      index = nir_load_var(&b, i);
   nir_push_if(&b, nir_uge(&b, index, nir_imm_intN_t(&b, nr_samples, 16)));
   {
         }
            b.cursor = nir_cf_reinsert(&list, b.cursor);
      }
            /* We've mucked about with control flow */
            /* Use the loop counter as the sample ID each iteration */
   nir_shader_instructions_pass(
      shader, lower_wrapped, nir_metadata_block_index | nir_metadata_dominance,
         }
      static bool
   lower_sample_mask_write(nir_builder *b, nir_instr *instr, void *data)
   {
      struct agx_msaa_state *state = data;
   if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   b->cursor = nir_before_instr(instr);
   if (intr->intrinsic != nir_intrinsic_store_output)
            nir_io_semantics sem = nir_intrinsic_io_semantics(intr);
   if (sem.location != FRAG_RESULT_SAMPLE_MASK)
            /* Sample mask writes are ignored unless multisampling is used. */
   if (state->nr_samples == 1) {
      nir_instr_remove(instr);
               /* The Vulkan spec says:
   *
   *    If sample shading is enabled, bits written to SampleMask
   *    corresponding to samples that are not being shaded by the fragment
   *    shader invocation are ignored.
   *
   * That will be satisfied by outputting gl_SampleMask for the whole pixel
   * and then lowering sample shading after (splitting up discard targets).
   */
   nir_discard_agx(b, nir_inot(b, nir_u2u16(b, intr->src[0].ssa)));
   b->shader->info.fs.uses_discard = true;
   nir_instr_remove(instr);
      }
      /*
   * Apply API sample mask to sample mask inputs, lowering:
   *
   *    sample_mask_in --> sample_mask_in & api_sample_mask
   */
   static bool
   lower_sample_mask_read(nir_builder *b, nir_intrinsic_instr *intr,
         {
               if (intr->intrinsic != nir_intrinsic_load_sample_mask_in)
            nir_def *old = &intr->def;
   nir_def *lowered = nir_iand(
            nir_def_rewrite_uses_after(old, lowered, lowered->parent_instr);
      }
      /* glSampleMask(x) --> gl_SampleMask = x */
   static void
   insert_sample_mask_write(nir_shader *s)
   {
      nir_builder b;
   nir_function_impl *impl = nir_shader_get_entrypoint(s);
            /* Kill samples that are NOT covered by the mask */
   nir_discard_agx(&b, nir_inot(&b, nir_load_api_sample_mask_agx(&b)));
      }
      /*
   * Lower a fragment shader into a monolithic pixel shader, with static sample
   * count, blend state, and tilebuffer formats in the shader key. For dynamic,
   * epilogs must be used, which have separate lowerings.
   */
   bool
   agx_nir_lower_monolithic_msaa(nir_shader *shader, struct agx_msaa_state *state)
   {
      assert(shader->info.stage == MESA_SHADER_FRAGMENT);
   assert(state->nr_samples == 1 || state->nr_samples == 2 ||
            /* Lower gl_SampleMask writes */
   if (shader->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK)) {
      nir_shader_instructions_pass(
      shader, lower_sample_mask_write,
            /* Lower API sample masks */
   if ((state->nr_samples > 1) && state->api_sample_mask)
            /* Additional, sample_mask_in needs to account for the API-level mask */
   nir_shader_intrinsics_pass(shader, lower_sample_mask_read,
                  /* In single sampled programs, interpolateAtSample needs to return the
   * center pixel. TODO: Generalize for dynamic sample count.
   */
   if (state->nr_samples == 1)
         else if (shader->info.fs.uses_sample_shading)
               }
