   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir_builder.h"
   #include "agx_compiler.h"
      /* Results of pattern matching */
   struct match {
      nir_scalar base, offset;
            /* Signed shift. A negative shift indicates that the offset needs ushr
   * applied. It's cheaper to fold iadd and materialize an extra ushr, than
   * to leave the iadd untouched, so this is good.
   */
      };
      /*
   * Try to match a multiplication with an immediate value. This generalizes to
   * both imul and ishl. If successful, returns true and sets the output
   * variables. Otherwise, returns false.
   */
   static bool
   match_imul_imm(nir_scalar scalar, nir_scalar *variable, uint32_t *imm)
   {
      if (!nir_scalar_is_alu(scalar))
            nir_op op = nir_scalar_alu_op(scalar);
   if (op != nir_op_imul && op != nir_op_ishl)
            nir_scalar inputs[] = {
      nir_scalar_chase_alu_src(scalar, 0),
               /* For imul check both operands for an immediate, since imul is commutative.
   * For ishl, only check the operand on the right.
   */
            for (unsigned i = commutes ? 0 : 1; i < ARRAY_SIZE(inputs); ++i) {
      if (!nir_scalar_is_const(inputs[i]))
                              if (op == nir_op_imul)
         else
                           }
      /*
   * Try to rewrite (a << (#b + #c)) + #d as ((a << #b) + #d') << #c,
   * assuming that #d is a multiple of 1 << #c. This takes advantage of
   * the hardware's implicit << #c and avoids a right-shift.
   *
   * Similarly, try to rewrite (a * (#b << #c)) + #d as ((a * #b) + #d') << #c.
   *
   * This pattern occurs with a struct-of-array layout.
   */
   static bool
   match_soa(nir_builder *b, struct match *match, unsigned format_shift)
   {
      if (!nir_scalar_is_alu(match->offset) ||
      nir_scalar_alu_op(match->offset) != nir_op_iadd)
         nir_scalar summands[] = {
      nir_scalar_chase_alu_src(match->offset, 0),
               for (unsigned i = 0; i < ARRAY_SIZE(summands); ++i) {
      if (!nir_scalar_is_const(summands[i]))
            /* Note: This is treated as signed regardless of the sign of the match.
   * The final addition into the base can be signed or unsigned, but when
   * we shift right by the format shift below we need to always sign extend
   * to ensure that any negative offset remains negative when added into
   * the index. That is, in:
   *
   * addr = base + (u64)((index + offset) << shift)
   *
   * `index` and `offset` are always 32 bits, and a negative `offset` needs
   * to subtract from the index, so it needs to be sign extended when we
   * apply the format shift regardless of the fact that the later conversion
   * to 64 bits does not sign extend.
   *
   * TODO: We need to confirm how the hardware handles 32-bit overflow when
   * applying the format shift, which might need rework here again.
   */
   int offset = nir_scalar_as_int(summands[i]);
   nir_scalar variable;
            /* The other operand must multiply */
   if (!match_imul_imm(summands[1 - i], &variable, &multiplier))
            int offset_shifted = offset >> format_shift;
            /* If the multiplier or the offset are not aligned, we can't rewrite */
   if (multiplier != (multiplier_shifted << format_shift))
            if (offset != (offset_shifted << format_shift))
            /* Otherwise, rewrite! */
            nir_def *rewrite = nir_iadd_imm(
            match->offset = nir_get_scalar(rewrite, 0);
   match->shift = 0;
                  }
      /* Try to pattern match address calculation */
   static struct match
   match_address(nir_builder *b, nir_scalar base, int8_t format_shift)
   {
               /* All address calculations are iadd at the root */
   if (!nir_scalar_is_alu(base) || nir_scalar_alu_op(base) != nir_op_iadd)
            /* Only 64+32 addition is supported, look for an extension */
   nir_scalar summands[] = {
      nir_scalar_chase_alu_src(base, 0),
               for (unsigned i = 0; i < ARRAY_SIZE(summands); ++i) {
      /* We can add a small constant to the 64-bit base for free */
   if (nir_scalar_is_const(summands[i]) &&
                        return (struct match){
      .base = summands[1 - i],
   .offset = nir_get_scalar(nir_imm_int(b, value), 0),
   .shift = -format_shift,
                  /* Otherwise, we can only add an offset extended from 32-bits */
   if (!nir_scalar_is_alu(summands[i]))
                     if (op != nir_op_u2u64 && op != nir_op_i2i64)
            /* We've found a summand, commit to it */
   match.base = summands[1 - i];
   match.offset = nir_scalar_chase_alu_src(summands[i], 0);
            /* Undo the implicit shift from using as offset */
   match.shift = -format_shift;
               /* If we didn't find something to fold in, there's nothing else we can do */
   if (!match.offset.def)
            /* But if we did, we can try to fold in in a multiply */
   nir_scalar multiplied;
            if (match_imul_imm(match.offset, &multiplied, &multiplier)) {
               /* Try to fold in either a full power-of-two, or just the power-of-two
   * part of a non-power-of-two stride.
   */
   if (util_is_power_of_two_nonzero(multiplier)) {
      new_shift += util_logbase2(multiplier);
      } else if (((multiplier >> format_shift) << format_shift) == multiplier) {
      new_shift += format_shift;
      } else {
                           /* Only fold in if we wouldn't overflow the lsl field */
   if (new_shift <= 2) {
      match.offset =
            } else if (new_shift > 0) {
      /* For large shifts, we do need a multiply, but we can
   * shrink the shift to avoid generating an ishr.
                                 match.offset = nir_get_scalar(rewrite, 0);
         } else {
      /* Try to match struct-of-arrays pattern, updating match if possible */
                  }
      static enum pipe_format
   format_for_bitsize(unsigned bitsize)
   {
      switch (bitsize) {
   case 8:
         case 16:
         case 32:
         default:
            }
      static bool
   pass(struct nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      if (intr->intrinsic != nir_intrinsic_load_global &&
      intr->intrinsic != nir_intrinsic_load_global_constant &&
   intr->intrinsic != nir_intrinsic_global_atomic &&
   intr->intrinsic != nir_intrinsic_global_atomic_swap &&
   intr->intrinsic != nir_intrinsic_store_global)
                  unsigned bitsize = intr->intrinsic == nir_intrinsic_store_global
               enum pipe_format format = format_for_bitsize(bitsize);
            nir_src *orig_offset = nir_get_io_offset_src(intr);
   nir_scalar base = nir_scalar_resolved(orig_offset->ssa, 0);
            nir_def *offset = match.offset.def != NULL
                  /* If we were unable to fold in the shift, insert a right-shift now to undo
   * the implicit left shift of the instruction.
   */
   if (match.shift < 0) {
      if (match.sign_extend)
         else
                        /* Hardware offsets must be 32-bits. Upconvert if the source code used
   * smaller integers.
   */
   if (offset->bit_size != 32) {
               if (match.sign_extend)
         else
               assert(match.shift >= 0);
            nir_def *repl = NULL;
   bool has_dest = (intr->intrinsic != nir_intrinsic_store_global);
   unsigned num_components = has_dest ? intr->def.num_components : 0;
            if (intr->intrinsic == nir_intrinsic_load_global) {
      repl =
      nir_load_agx(b, num_components, bit_size, new_base, offset,
            } else if (intr->intrinsic == nir_intrinsic_load_global_constant) {
      repl = nir_load_constant_agx(b, num_components, bit_size, new_base,
                  } else if (intr->intrinsic == nir_intrinsic_global_atomic) {
      offset = nir_ishl_imm(b, offset, match.shift);
   repl =
      nir_global_atomic_agx(b, bit_size, new_base, offset, intr->src[1].ssa,
         } else if (intr->intrinsic == nir_intrinsic_global_atomic_swap) {
      offset = nir_ishl_imm(b, offset, match.shift);
   repl = nir_global_atomic_swap_agx(
      b, bit_size, new_base, offset, intr->src[1].ssa, intr->src[2].ssa,
   .atomic_op = nir_intrinsic_atomic_op(intr),
   } else {
      nir_store_agx(b, intr->src[0].ssa, new_base, offset,
                     if (repl)
            nir_instr_remove(&intr->instr);
      }
      bool
   agx_nir_lower_address(nir_shader *shader)
   {
      return nir_shader_intrinsics_pass(
      }
