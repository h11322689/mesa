   /*
   * Copyright 2023 Valve Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "asahi/compiler/agx_compile.h"
   #include "compiler/glsl_types.h"
   #include "compiler/nir/nir_builder.h"
   #include "agx_state.h"
   #include "nir_intrinsics_indices.h"
      #define AGX_TEXTURE_DESC_STRIDE 24
      /*
   * Construct a bindless handle corresponding to an index into the binding
   * tables. Our driver ABI maps everything to a table addressed by u0_u1, with
   * indices mapped 1:1 with the binding table. So we want the bindless handle
   * (u0_u1, index) which is encoded in NIR as (0, index).
   */
   static nir_def *
   index_to_handle(nir_builder *b, nir_def *index)
   {
      nir_def *table = nir_imm_int(b, 0);
               }
      /*
   * Lower binding table textures and images to texture state registers and (if
   * necessary) bindless access into an internal table mapped like additional
   * texture state registers. The following layout is used:
   *
   *    1. Textures
   *    2. Images (read/write interleaved)
   */
   static bool
   lower(nir_builder *b, nir_instr *instr, void *data)
   {
      bool *internal_bindless = data;
   bool force_bindless = agx_nir_needs_texture_crawl(instr);
            if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
      #define CASE(op)                                                               \
      case nir_intrinsic_##op:                                                    \
      bindless_op = nir_intrinsic_bindless_##op;                               \
            switch (intr->intrinsic) {
      CASE(image_load)
   CASE(image_store)
   CASE(image_size)
   CASE(image_atomic)
      default:
         #undef CASE
            nir_def *index = intr->src[0].ssa;
            /* Remap according to the driver layout */
            /* For reads and image_size, we use the texture descriptor which is first.
   * Writes and atomics use the PBE descriptor.
   */
   if (intr->intrinsic != nir_intrinsic_image_load &&
                  /* If we can determine statically that the image fits in texture state
   * registers, avoid lowering to bindless access.
   */
   if (nir_scalar_is_const(index_scalar) && !force_bindless) {
               if (idx < AGX_NUM_TEXTURE_STATE_REGS) {
      nir_src_rewrite(&intr->src[0], nir_imm_intN_t(b, idx, 16));
                  nir_atomic_op op = nir_atomic_op_iadd /* irrelevant */;
   if (nir_intrinsic_has_atomic_op(intr))
            /* Otherwise, lower to bindless */
            if (nir_intrinsic_has_atomic_op(intr))
                     index = nir_iadd_imm(b, nir_imul_imm(b, index, 2), offset);
      } else if (instr->type == nir_instr_type_tex) {
               /* Nothing to do for "real" bindless */
   if (nir_tex_instr_src_index(tex, nir_tex_src_texture_handle) >= 0)
            /* Textures are mapped 1:1, so if we can prove it fits in a texture state
   * register, use the texture state register.
   */
   if (tex->texture_index < AGX_NUM_TEXTURE_STATE_REGS &&
      nir_tex_instr_src_index(tex, nir_tex_src_texture_offset) == -1 &&
               /* Otherwise, lower to bindless. Could be optimized. */
   nir_def *index = nir_steal_tex_src(tex, nir_tex_src_texture_offset);
   if (!index)
            *internal_bindless = true;
   nir_tex_instr_add_src(tex, nir_tex_src_texture_handle,
                  }
      bool
   agx_nir_lower_bindings(nir_shader *shader, bool *internal_bindless)
   {
      /* First lower index to offset so we can lower more naturally */
   bool progress = nir_lower_tex(
            /* Next run constant folding so the constant optimizations above have a
   * chance.
   */
            progress |= nir_shader_instructions_pass(
      shader, lower, nir_metadata_block_index | nir_metadata_dominance,
         }
