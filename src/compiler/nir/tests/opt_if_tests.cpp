   /*
   * Copyright © 2020 Intel Corporation
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
      class nir_opt_if_test : public nir_test {
   protected:
                        nir_def *in_def;
      };
      nir_opt_if_test::nir_opt_if_test()
         {
      nir_variable *var = nir_variable_create(b->shader, nir_var_shader_in, glsl_int_type(), "in");
               }
      TEST_F(nir_opt_if_test, opt_if_simplification)
   {
      /* Tests that opt_if_simplification correctly optimizes a simple case:
   *
   * vec1 1 ssa_2 = ieq ssa_0, ssa_1
   * if ssa_2 {
   *    block block_2:
   * } else {
   *    block block_3:
   *    do_work()
   * }
                     nir_def *cmp_result = nir_ieq(b, in_def, one);
                     // do_work
                                       ASSERT_TRUE(!exec_list_is_empty((&nir_if_first_then_block(nif)->instr_list)));
      }
      TEST_F(nir_opt_if_test, opt_if_simplification_single_source_phi_after_if)
   {
      /* Tests that opt_if_simplification correctly handles single-source
   * phis after the if.
   *
   * vec1 1 ssa_2 = ieq ssa_0, ssa_1
   * if ssa_2 {
   *    block block_2:
   * } else {
   *    block block_3:
   *    do_work()
   *    return
   * }
   * block block_4:
   * vec1 32 ssa_3 = phi block_2: ssa_0
                     nir_def *cmp_result = nir_ieq(b, in_def, one);
                     // do_work
            nir_jump_instr *jump = nir_jump_instr_create(b->shader, nir_jump_return);
                                                nir_def_init(&phi->instr, &phi->def,
                                       ASSERT_TRUE(nir_block_ends_in_jump(nir_if_last_then_block(nif)));
      }
      TEST_F(nir_opt_if_test, opt_if_alu_of_phi_progress)
   {
      nir_def *two = nir_imm_int(b, 2);
                     nir_loop *loop = nir_push_loop(b);
   {
      nir_def_init(&phi->instr, &phi->def,
                     nir_def *y = nir_iadd(b, &phi->def, two);
   nir_store_var(b, out_var,
               }
            b->cursor = nir_before_block(nir_loop_first_block(loop));
                              int progress_count = 0;
   for (int i = 0; i < 10; i++) {
      progress = nir_opt_if(b->shader, nir_opt_if_optimize_phi_true_false);
   if (progress)
         else
                     EXPECT_LE(progress_count, 2);
      }
