   /*
   * Copyright 2023 Valve Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "midgard_nir.h"
   #include "nir_opcodes.h"
      static bool
   pass(nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(instr);
   if (alu->op != nir_op_b32csel)
            BITSET_WORD *float_types = data;
   if (BITSET_TEST(float_types, alu->def.index)) {
      alu->op = nir_op_b32fcsel_mdg;
      } else {
            }
      void
   midgard_nir_type_csel(nir_shader *shader)
   {
      nir_function_impl *impl = nir_shader_get_entrypoint(shader);
            BITSET_WORD *float_types =
                  nir_shader_instructions_pass(
      shader, pass, nir_metadata_block_index | nir_metadata_dominance,
            }
