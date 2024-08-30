   /*
   * Copyright (c) 2020 Etnaviv Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Christian Gmeiner <christian.gmeiner@gmail.com>
   */
      #include <gtest/gtest.h>
      #include "nir.h"
   #include "nir_builder.h"
      extern "C" {
      /* we really do not want to include etnaviv_nir.h as it makes it
   * harder to get this test compiling. as we are only working with
            extern bool
      }
      class nir_lower_ubo_test : public ::testing::Test {
   protected:
      nir_lower_ubo_test();
            nir_intrinsic_instr *intrinsic(nir_intrinsic_op op);
               };
      nir_lower_ubo_test::nir_lower_ubo_test()
   {
               static const nir_shader_compiler_options options = { };
      }
      nir_lower_ubo_test::~nir_lower_ubo_test()
   {
      if (HasFailure()) {
      printf("\nShader from the failed test:\n\n");
                           }
      nir_intrinsic_instr *
   nir_lower_ubo_test::intrinsic(nir_intrinsic_op op)
   {
      nir_foreach_block(block, b.impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic == op)
         }
      }
      unsigned
   nir_lower_ubo_test::count_intrinsic(nir_intrinsic_op op)
   {
               nir_foreach_block(block, b.impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic == op)
         }
      }
      TEST_F(nir_lower_ubo_test, nothing_to_lower)
   {
                                 ASSERT_FALSE(etna_nir_lower_ubo_to_uniform(b.shader));
            ASSERT_EQ(count_intrinsic(nir_intrinsic_load_ubo), 0);
      }
      TEST_F(nir_lower_ubo_test, basic)
   {
      nir_def *offset = nir_imm_int(&b, 4);
            nir_lower_uniforms_to_ubo(b.shader, false, false);
            ASSERT_TRUE(etna_nir_lower_ubo_to_uniform(b.shader));
   nir_validate_shader(b.shader, NULL);
            nir_intrinsic_instr *load_uniform = intrinsic(nir_intrinsic_load_uniform);
   ASSERT_EQ(nir_src_as_uint(load_uniform->src[0]), 4);
      }
      TEST_F(nir_lower_ubo_test, index_not_null)
   {
      nir_def *index = nir_imm_int(&b, 1);
                              ASSERT_FALSE(etna_nir_lower_ubo_to_uniform(b.shader));
   ASSERT_EQ(count_intrinsic(nir_intrinsic_load_ubo), 1);
      }
      TEST_F(nir_lower_ubo_test, indirect_index)
   {
      nir_def *one = nir_imm_int(&b, 1);
   nir_def *index = nir_fadd(&b, one, one);
                              ASSERT_FALSE(etna_nir_lower_ubo_to_uniform(b.shader));
            ASSERT_EQ(count_intrinsic(nir_intrinsic_load_ubo), 1);
      }
      TEST_F(nir_lower_ubo_test, indirect_offset)
   {
      nir_def *one = nir_imm_int(&b, 1);
   nir_def *index = nir_imm_int(&b, 0);
                              ASSERT_FALSE(etna_nir_lower_ubo_to_uniform(b.shader));
            ASSERT_EQ(count_intrinsic(nir_intrinsic_load_ubo), 1);
      }
