   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir_builder.h"
   #include "agx_compiler.h"
      /*
   * Lower load_interpolated_input instructions with unused components of their
   * destination, duplicating the intrinsic and shrinking to avoid the holes.
   * load_interpolated_input becomes iter instructions, which lack a write mask.
   */
   static bool
   pass(struct nir_builder *b, nir_instr *instr, UNUSED void *data)
   {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_load_interpolated_input)
            unsigned mask = nir_def_components_read(&intr->def);
   if (mask == 0 || mask == nir_component_mask(intr->num_components))
            b->cursor = nir_before_instr(instr);
   unsigned bit_size = intr->def.bit_size;
            for (unsigned c = 0; c < intr->num_components; ++c) {
      if (mask & BITFIELD_BIT(c)) {
      /* Count contiguous components to combine with */
   unsigned next_mask = mask >> c;
                                                /* Shrink the load to count contiguous components */
   nir_def_init(clone, &clone_intr->def, count, bit_size);
                  /* The load starts from component c relative to the original load */
                           /* The destination is a vector with `count` components, extract the
   * components so we can recombine into the final vector.
   */
                     } else {
      /* The value of unused components is irrelevant, but use an undef for
   * semantics. It will be eliminated by DCE after copyprop.
   */
                  nir_def_rewrite_uses(&intr->def, nir_vec(b, comps, intr->num_components));
      }
      bool
   agx_nir_lower_load_mask(nir_shader *shader)
   {
      return nir_shader_instructions_pass(
      }
