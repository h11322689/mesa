   /*
   * Copyright © 2021 Valve Corporation
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_nir.h"
   #include "nir.h"
   #include "nir_builder.h"
      static nir_def *
   try_extract_additions(nir_builder *b, nir_scalar scalar, uint64_t *out_const,
         {
      if (!nir_scalar_is_alu(scalar) || nir_scalar_alu_op(scalar) != nir_op_iadd)
            nir_alu_instr *alu = nir_instr_as_alu(scalar.def->parent_instr);
   nir_scalar src0 = nir_scalar_chase_alu_src(scalar, 0);
            for (unsigned i = 0; i < 2; ++i) {
      nir_scalar src = i ? src1 : src0;
   if (nir_scalar_is_const(src)) {
         } else if (nir_scalar_is_alu(src) && nir_scalar_alu_op(src) == nir_op_u2u64) {
      nir_scalar offset_scalar = nir_scalar_chase_alu_src(src, 0);
   nir_def *offset = nir_channel(b, offset_scalar.def, offset_scalar.comp);
   if (*out_offset)
         else
      } else {
                  nir_def *replace_src =
                     nir_def *replace_src0 = try_extract_additions(b, src0, out_const, out_offset);
   nir_def *replace_src1 = try_extract_additions(b, src1, out_const, out_offset);
   if (!replace_src0 && !replace_src1)
            replace_src0 = replace_src0 ? replace_src0 : nir_channel(b, src0.def, src0.comp);
   replace_src1 = replace_src1 ? replace_src1 : nir_channel(b, src1.def, src1.comp);
      }
      static bool
   process_instr(nir_builder *b, nir_intrinsic_instr *intrin, void *_)
   {
      nir_intrinsic_op op;
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
      op = nir_intrinsic_load_global_amd;
      case nir_intrinsic_global_atomic:
      op = nir_intrinsic_global_atomic_amd;
      case nir_intrinsic_global_atomic_swap:
      op = nir_intrinsic_global_atomic_swap_amd;
      case nir_intrinsic_store_global:
      op = nir_intrinsic_store_global_amd;
      default:
         }
                     uint64_t off_const = 0;
   nir_def *offset = NULL;
   nir_scalar src = {addr_src->ssa, 0};
   b->cursor = nir_after_instr(addr_src->ssa->parent_instr);
   nir_def *addr = try_extract_additions(b, src, &off_const, &offset);
                     if (off_const > UINT32_MAX) {
      addr = nir_iadd_imm(b, addr, off_const);
                                 if (op != nir_intrinsic_store_global_amd)
      nir_def_init(&new_intrin->instr, &new_intrin->def,
         unsigned num_src = nir_intrinsic_infos[intrin->intrinsic].num_srcs;
   for (unsigned i = 0; i < num_src; i++)
         new_intrin->src[num_src] = nir_src_for_ssa(offset ? offset : nir_imm_zero(b, 1, 32));
            if (nir_intrinsic_has_access(intrin))
         if (nir_intrinsic_has_align_mul(intrin))
         if (nir_intrinsic_has_align_offset(intrin))
         if (nir_intrinsic_has_write_mask(intrin))
         if (nir_intrinsic_has_atomic_op(intrin))
                  nir_builder_instr_insert(b, &new_intrin->instr);
   if (op != nir_intrinsic_store_global_amd)
                     }
      bool
   ac_nir_lower_global_access(nir_shader *shader)
   {
      return nir_shader_intrinsics_pass(shader, process_instr,
      }
