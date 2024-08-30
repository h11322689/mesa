   /*
   * Copyright © 2022 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "brw_nir.h"
   #include "compiler/nir/nir_builder.h"
      /**
   * Implement a peephole pass to convert integer multiplications to imul32x16.
   */
      struct pass_data {
         };
      static void
   replace_imul_instr(nir_builder *b, nir_alu_instr *imul, unsigned small_val,
         {
                                 nir_alu_src_copy(&imul_32x16->src[0], &imul->src[1 - small_val]);
            nir_def_init(&imul_32x16->instr, &imul_32x16->def,
            nir_def_rewrite_uses(&imul->def,
                     nir_instr_remove(&imul->instr);
      }
      enum root_operation {
      non_unary = 0,
   integer_neg = 1 << 0,
   integer_abs = 1 << 1,
   integer_neg_abs = integer_neg | integer_abs,
      };
      static enum root_operation
   signed_integer_range_analysis(nir_shader *shader, struct hash_table *range_ht,
         {
      if (nir_scalar_is_const(scalar)) {
      *lo = nir_scalar_as_int(scalar);
   *hi = *lo;
               if (nir_scalar_is_alu(scalar)) {
      switch (nir_scalar_alu_op(scalar)) {
   case nir_op_iabs:
      signed_integer_range_analysis(shader, range_ht,
                  if (*lo == INT32_MIN) {
         } else {
                     *lo = MIN2(a, b);
               /* Absolute value wipes out any inner negations, and it is redundant
   * with any inner absolute values.
               case nir_op_ineg: {
      const enum root_operation root =
      signed_integer_range_analysis(shader, range_ht,
               if (*lo == INT32_MIN) {
         } else {
                     *lo = MIN2(a, b);
               /* Negation of a negation cancels out, but negation of absolute value
   * must preserve the integer_abs bit.
   */
               case nir_op_imax: {
                     signed_integer_range_analysis(shader, range_ht,
               signed_integer_range_analysis(shader, range_ht,
                                             case nir_op_imin: {
                     signed_integer_range_analysis(shader, range_ht,
               signed_integer_range_analysis(shader, range_ht,
                                             default:
                     /* Any value with the sign-bit set is problematic. Consider the case when
   * bound is 0x80000000. As an unsigned value, this means the value must be
   * in the range [0, 0x80000000]. As a signed value, it means the value must
   * be in the range [0, INT_MAX] or it must be INT_MIN.
   *
   * If bound is -2, it means the value is either in the range [INT_MIN, -2]
   * or it is in the range [0, INT_MAX].
   *
   * This function only returns a single, contiguous range. The union of the
   * two ranges for any value of bound with the sign-bit set is [INT_MIN,
   * INT_MAX].
   */
   const int32_t bound = nir_unsigned_upper_bound(shader, range_ht,
         if (bound < 0) {
      *lo = INT32_MIN;
      } else {
      *lo = 0;
                  }
      static bool
   brw_nir_opt_peephole_imul32x16_instr(nir_builder *b,
               {
      struct pass_data *d = (struct pass_data *) cb_data;
            if (instr->type != nir_instr_type_alu)
            nir_alu_instr *imul = nir_instr_as_alu(instr);
   if (imul->op != nir_op_imul)
            if (imul->def.bit_size != 32)
                     unsigned i;
   for (i = 0; i < 2; i++) {
      if (!nir_src_is_const(imul->src[i].src))
            int64_t lo = INT64_MAX;
            for (unsigned comp = 0; comp < imul->def.num_components; comp++) {
                              if (v > hi)
               if (lo >= INT16_MIN && hi <= INT16_MAX) {
      new_opcode = nir_op_imul_32x16;
      } else if (lo >= 0 && hi <= UINT16_MAX) {
      new_opcode = nir_op_umul_32x16;
                  if (new_opcode != nir_num_opcodes) {
      replace_imul_instr(b, imul, i, new_opcode);
               if (imul->def.num_components > 1)
            const nir_scalar imul_scalar = { &imul->def, 0 };
   int idx = -1;
            for (i = 0; i < 2; i++) {
      /* All constants were previously processed.  There is nothing more to
   * learn from a constant here.
   */
   if (imul->src[i].src.ssa->parent_instr->type == nir_instr_type_load_const)
            nir_scalar scalar = nir_scalar_chase_alu_src(imul_scalar, i);
   int lo = INT32_MIN;
            const enum root_operation root =
            /* Copy propagation (in the backend) has trouble handling cases like
   *
   *    mov(8)          g60<1>D         -g59<8,8,1>D
   *    mul(8)          g61<1>D         g63<8,8,1>D     g60<16,8,2>W
   *
   * If g59 had absolute value instead of negation, even improved copy
   * propagation would not be able to make progress.
   *
   * In cases where both sources to the integer multiplication can fit in
   * 16-bits, choose the source that does not have a source modifier.
   */
   if (root < prev_root) {
      if (lo >= INT16_MIN && hi <= INT16_MAX) {
      new_opcode = nir_op_imul_32x16;
                  if (root == non_unary)
      } else if (lo >= 0 && hi <= UINT16_MAX) {
      new_opcode = nir_op_umul_32x16;
                  if (root == non_unary)
                     if (new_opcode == nir_num_opcodes) {
      assert(idx == -1);
   assert(prev_root == invalid_root);
               assert(idx != -1);
            replace_imul_instr(b, imul, idx, new_opcode);
      }
      bool
   brw_nir_opt_peephole_imul32x16(nir_shader *shader)
   {
                        bool progress = nir_shader_instructions_pass(shader,
                                          }
   