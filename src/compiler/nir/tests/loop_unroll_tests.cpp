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
      #include "gtest/gtest.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_phi_builder.h"
      #define UNROLL_TEST_INSERT(_label, _type, _init, _limit, _step,         \
                  TEST_F(nir_loop_unroll_test, _label)                                 \
   {                                                                    \
      nir_def *init = nir_imm_##_type(&bld, _init);                 \
   nir_def *limit = nir_imm_##_type(&bld, _limit);               \
   nir_def *step = nir_imm_##_type(&bld, _step);                 \
   loop_unroll_test_helper(&bld, init, limit, step,                  \
         EXPECT_##_exp_res(nir_opt_loop_unroll(bld.shader));               \
   EXPECT_EQ(_exp_instr_count, count_instr(nir_op_##_incr));         \
            namespace {
      class nir_loop_unroll_test : public ::testing::Test {
   protected:
      nir_loop_unroll_test()
   {
      glsl_type_singleton_init_or_ref();
   static nir_shader_compiler_options options = { };
   options.max_unroll_iterations = 32;
   options.force_indirect_unrolling_sampler = false;
   options.force_indirect_unrolling = nir_var_all;
   bld = nir_builder_init_simple_shader(MESA_SHADER_VERTEX, &options,
      }
   ~nir_loop_unroll_test()
   {
      ralloc_free(bld.shader);
               int count_instr(nir_op op);
               };
      } /* namespace */
      int
   nir_loop_unroll_test::count_instr(nir_op op)
   {
      int count = 0;
   nir_foreach_block(block, bld.impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type != nir_instr_type_alu)
         nir_alu_instr *alu_instr = nir_instr_as_alu(instr);
   if (alu_instr->op == op)
                     }
      int
   nir_loop_unroll_test::count_loops(void)
   {
      int count = 0;
   foreach_list_typed(nir_cf_node, cf_node, node, &bld.impl->body) {
      if (cf_node->type == nir_cf_node_loop)
                  }
      void
   loop_unroll_test_helper(nir_builder *bld, nir_def *init,
                           nir_def *limit, nir_def *step,
   nir_def* (*cond_instr)(nir_builder*,
               {
               nir_block *top_block =
                  nir_phi_instr *phi = nir_phi_instr_create(bld->shader);
                     nir_def *cond = cond_instr(bld,
                  nir_if *nif = nir_push_if(bld, cond);
   nir_jump(bld, nir_jump_break);
                                       bld->cursor = nir_after_phis(head_block);
               }
      UNROLL_TEST_INSERT(iadd,     int,   0,     24,   4,
         UNROLL_TEST_INSERT(iadd_rev, int,   0,     24,   4,
         #ifndef __s390x__
   UNROLL_TEST_INSERT(fadd,     float, 0.0,   24.0, 4.0,
         UNROLL_TEST_INSERT(fadd_rev, float, 0.0,   24.0, 4.0,
         #endif
   UNROLL_TEST_INSERT(imul,     int,   1,     81,   3,
         UNROLL_TEST_INSERT(imul_rev, int,   1,     81,   3,
         #if 0 /* Disable tests until support is re-enabled in loop_analyze. */
   UNROLL_TEST_INSERT(fmul,     float, 1.5,   81.0, 3.0,
         UNROLL_TEST_INSERT(fmul_rev, float, 1.0,   81.0, 3.0,
         #endif
   UNROLL_TEST_INSERT(ishl,     int,   1,     128,  1,
         UNROLL_TEST_INSERT(ishl_rev, int,   1,     128,  1,
         UNROLL_TEST_INSERT(ishr,     int,   64,    4,    1,
         UNROLL_TEST_INSERT(ishr_rev, int,   64,    4,    1,
         UNROLL_TEST_INSERT(ushr,     int,   64,    4,    1,
         UNROLL_TEST_INSERT(ushr_rev, int,   64,    4,    1,
            UNROLL_TEST_INSERT(lshl_neg,     int,  0xf0f0f0f0, 0,    1,
         UNROLL_TEST_INSERT(lshl_neg_rev, int,  0xf0f0f0f0, 0,    1,
                     ilt,          ishl, true,       TRUE, 4, 0)
