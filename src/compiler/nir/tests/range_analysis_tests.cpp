   /*
   * Copyright © 2021 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
   #include "nir_test.h"
   #include "nir_range_analysis.h"
      class ssa_def_bits_used_test : public nir_test {
   protected:
      ssa_def_bits_used_test()
         {
               };
      class unsigned_upper_bound_test : public nir_test {
   protected:
      unsigned_upper_bound_test()
         {
      };
      static bool
   is_used_once(const nir_def *def)
   {
         }
      nir_alu_instr *
   ssa_def_bits_used_test::build_alu_instr(nir_op op,
         {
               if (def == NULL)
                     if (alu == NULL)
                        }
      TEST_F(ssa_def_bits_used_test, iand_with_const_vector)
   {
               nir_def *src0 = nir_imm_ivec4(b,
                                          for (unsigned i = 0; i < 4; i++) {
      /* If the test is changed, and somehow src1 is used multiple times,
   * nir_def_bits_used will accumulate *all* the uses (as it should).
   * This isn't what we're trying to test here.
   */
                              /* The answer should be the value swizzled from src0. */
         }
      TEST_F(ssa_def_bits_used_test, ior_with_const_vector)
   {
               nir_def *src0 = nir_imm_ivec4(b,
                                          for (unsigned i = 0; i < 4; i++) {
      /* If the test is changed, and somehow src1 is used multiple times,
   * nir_def_bits_used will accumulate *all* the uses (as it should).
   * This isn't what we're trying to test here.
   */
                              /* The answer should be the value swizzled from ~src0. */
         }
      TEST_F(ssa_def_bits_used_test, extract_i16_with_const_index)
   {
                        nir_def *src1 = nir_imm_ivec4(b,
                                                for (unsigned i = 1; i < 3; i++) {
      /* If the test is changed, and somehow src1 is used multiple times,
   * nir_def_bits_used will accumulate *all* the uses (as it should).
   * This isn't what we're trying to test here.
   */
                                    }
      TEST_F(ssa_def_bits_used_test, extract_u16_with_const_index)
   {
                        nir_def *src1 = nir_imm_ivec4(b,
                                                for (unsigned i = 1; i < 3; i++) {
      /* If the test is changed, and somehow src1 is used multiple times,
   * nir_def_bits_used will accumulate *all* the uses (as it should).
   * This isn't what we're trying to test here.
   */
                                    }
      TEST_F(ssa_def_bits_used_test, extract_i8_with_const_index)
   {
                        nir_def *src1 = nir_imm_ivec4(b,
                                                for (unsigned i = 0; i < 4; i++) {
      /* If the test is changed, and somehow src1 is used multiple times,
   * nir_def_bits_used will accumulate *all* the uses (as it should).
   * This isn't what we're trying to test here.
   */
                                    }
      TEST_F(ssa_def_bits_used_test, extract_u8_with_const_index)
   {
                        nir_def *src1 = nir_imm_ivec4(b,
                                                for (unsigned i = 0; i < 4; i++) {
      /* If the test is changed, and somehow src1 is used multiple times,
   * nir_def_bits_used will accumulate *all* the uses (as it should).
   * This isn't what we're trying to test here.
   */
                                    }
      /* Unsigned upper bound analysis should look through a bcsel which uses the phi. */
   TEST_F(unsigned_upper_bound_test, loop_phi_bcsel)
   {
      /*
   * impl main {
   *     block b0:  // preds:
   *     32    %0 = load_const (0x00000000 = 0.000000)
   *     32    %1 = load_const (0x00000002 = 0.000000)
   *     1     %2 = load_const (false)
   *                // succs: b1
   *     loop {
   *         block b1:  // preds: b0 b1
   *         32    %4 = phi b0: %0 (0x0), b1: %3
   *         32    %3 = bcsel %2 (false), %4, %1 (0x2)
   *                    // succs: b1
   *     }
   *     block b2:  // preds: , succs: b3
   *     block b3:
   * }
   */
   nir_def *zero = nir_imm_int(b, 0);
   nir_def *two = nir_imm_int(b, 2);
            nir_phi_instr *const phi = nir_phi_instr_create(b->shader);
            nir_push_loop(b);
   nir_def *sel = nir_bcsel(b, cond, &phi->def, two);
            nir_phi_instr_add_src(phi, zero->parent_instr->block, zero);
   nir_phi_instr_add_src(phi, sel->parent_instr->block, sel);
   b->cursor = nir_before_instr(sel->parent_instr);
                     struct hash_table *range_ht = _mesa_pointer_hash_table_create(NULL);
   nir_scalar scalar = nir_get_scalar(&phi->def, 0);
   EXPECT_EQ(nir_unsigned_upper_bound(b->shader, range_ht, scalar, NULL), 2);
      }
