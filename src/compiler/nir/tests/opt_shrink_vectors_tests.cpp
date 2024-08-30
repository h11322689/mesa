   /*
   * Copyright © 2022 Pavel Ondračka
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
      class nir_opt_shrink_vectors_test : public nir_test {
   protected:
               nir_def *in_def;
      };
      nir_opt_shrink_vectors_test::nir_opt_shrink_vectors_test()
         {
      nir_variable *var = nir_variable_create(b->shader, nir_var_shader_in, glsl_vec_type(2), "in");
               }
      static unsigned translate_swizzle(char swz)
   {
      const char *swizzles_dict = "xyzw";
            const char *ptr = strchr(swizzles_dict, swz);
   if (ptr)
         else
      }
      static void set_swizzle(nir_alu_src * src, const char * swizzle)
   {
      unsigned i = 0;
   while (swizzle[i]) {
      src->swizzle[i] = translate_swizzle(swizzle[i]);
         }
      static void check_swizzle(nir_alu_src * src, const char * swizzle)
   {
      unsigned i = 0;
   while (swizzle[i]) {
      ASSERT_TRUE(src->swizzle[i] == translate_swizzle(swizzle[i]));
         }
      TEST_F(nir_opt_shrink_vectors_test, opt_shrink_vectors_load_const_trailing_component_only)
   {
      /* Test that opt_shrink_vectors correctly removes unused trailing channels
   * of load_const.
   *
   * vec4 32 ssa_1 = load_const (1.0, 2.0, 3.0, 4.0)
   * vec1 32 ssa_2 = fmov ssa_1.x
   *
   * to
   *
   * vec1 32 ssa_1 = load_const (1.0)
   * vec1 32 ssa_2 = fmov ssa_1.x
                     nir_def *alu_result = nir_build_alu1(b, nir_op_mov, imm_vec);
   nir_alu_instr *alu_instr = nir_instr_as_alu(alu_result->parent_instr);
   set_swizzle(&alu_instr->src[0], "x");
                                       ASSERT_TRUE(imm_vec->num_components == 1);
   nir_load_const_instr * imm_vec_instr = nir_instr_as_load_const(imm_vec->parent_instr);
               }
      TEST_F(nir_opt_shrink_vectors_test, opt_shrink_vectors_alu_trailing_component_only)
   {
      /* Test that opt_shrink_vectors correctly removes unused trailing channels
   * of alus.
   *
   * vec4 32 ssa_1 = fmov ssa_0.xyzx
   * vec1 32 ssa_2 = fmov ssa_1.x
   *
   * to
   *
   * vec1 32 ssa_1 = fmov ssa_0.x
   * vec1 32 ssa_2 = fmov ssa_1.x
            nir_def *alu_result = nir_build_alu1(b, nir_op_mov, in_def);
   nir_alu_instr *alu_instr = nir_instr_as_alu(alu_result->parent_instr);
   alu_result->num_components = 4;
            nir_def *alu2_result = nir_build_alu1(b, nir_op_mov, alu_result);
   nir_alu_instr *alu2_instr = nir_instr_as_alu(alu2_result->parent_instr);
   set_swizzle(&alu2_instr->src[0], "x");
                                       check_swizzle(&alu_instr->src[0], "x");
               }
      TEST_F(nir_opt_shrink_vectors_test, opt_shrink_vectors_simple)
   {
      /* Tests that opt_shrink_vectors correctly shrinks a simple case.
   *
   * vec4 32 ssa_2 = load_const (3.0, 1.0, 2.0, 1.0)
   * vec4 32 ssa_3 = fadd ssa_1.xxxy, ssa_2.ywyz
   * vec1 32 ssa_4 = fdot3 ssa_3.xzw ssa_3.xzw
   *
   * to
   *
   * vec2 32 ssa_2 = load_const (1.0, 2.0)
   * vec2 32 ssa_3 = fadd ssa_1, ssa_2
   * vec1 32 ssa_4 = fdot3 ssa_3.xxy ssa_3.xxy
                     nir_def *alu_result = nir_build_alu2(b, nir_op_fadd, in_def, imm_vec);
   nir_alu_instr *alu_instr = nir_instr_as_alu(alu_result->parent_instr);
   alu_result->num_components = 4;
   set_swizzle(&alu_instr->src[0], "xxxy");
            nir_def *alu2_result = nir_build_alu2(b, nir_op_fdot3, alu_result, alu_result);
   nir_alu_instr *alu2_instr = nir_instr_as_alu(alu2_result->parent_instr);
   set_swizzle(&alu2_instr->src[0], "xzw");
                                       ASSERT_TRUE(imm_vec->num_components == 2);
   nir_load_const_instr * imm_vec_instr = nir_instr_as_load_const(imm_vec->parent_instr);
   ASSERT_TRUE(nir_const_value_as_float(imm_vec_instr->value[0], 32) == 1.0);
            ASSERT_TRUE(alu_result->num_components == 2);
   check_swizzle(&alu_instr->src[0], "xy");
            check_swizzle(&alu2_instr->src[0], "xxy");
                        }
      TEST_F(nir_opt_shrink_vectors_test, opt_shrink_vectors_vec8)
   {
      /* Tests that opt_shrink_vectors correctly shrinks a case
   * dealing with vec8 shrinking. The shrinking would result in
   * vec6 for load const and vec7 for fadd and is therefore not allowed,
   * but check that we still properly reuse the channels and move
   * the unused channels to the end.
   *
   * vec8 32 ssa_2 = load_const (1.0, 1.0, 2.0, 3.0, 4.0, 5.0, 2.0, 6.0)
   * vec8 32 ssa_3 = fadd ssa_1.xxxxxxxy, ssa_2.afhdefgh
   * vec1 32 ssa_4 = fdot8 ssa_3.accdefgh ssa_3.accdefgh
   *
   * to
   *
   * vec8 32 ssa_2 = load_const (1.0, 3.0, 4.0, 5.0, 2.0, 6.0, .., ..))
   * vec8 32 ssa_3 = fadd ssa_1.xxxxxxy_ ssa_2.afbcdef_
   * vec1 32 ssa_4 = fdot8 ssa_3.abbcdefg ssa_3.abbcdefg
            nir_const_value v[8] = {
      nir_const_value_for_float(1.0, 32),
   nir_const_value_for_float(1.0, 32),
   nir_const_value_for_float(2.0, 32),
   nir_const_value_for_float(3.0, 32),
   nir_const_value_for_float(4.0, 32),
   nir_const_value_for_float(5.0, 32),
   nir_const_value_for_float(2.0, 32),
      };
            nir_def *alu_result = nir_build_alu2(b, nir_op_fadd, in_def, imm_vec);
   nir_alu_instr *alu_instr = nir_instr_as_alu(alu_result->parent_instr);
   alu_result->num_components = 8;
   set_swizzle(&alu_instr->src[0], "xxxxxxxy");
            nir_def *alu2_result = nir_build_alu2(b, nir_op_fdot8, alu_result, alu_result);
   nir_alu_instr *alu2_instr = nir_instr_as_alu(alu2_result->parent_instr);
   set_swizzle(&alu2_instr->src[0], "accdefgh");
                                       ASSERT_TRUE(imm_vec->num_components == 8);
   nir_load_const_instr * imm_vec_instr = nir_instr_as_load_const(imm_vec->parent_instr);
   ASSERT_TRUE(nir_const_value_as_float(imm_vec_instr->value[0], 32) == 1.0);
   ASSERT_TRUE(nir_const_value_as_float(imm_vec_instr->value[1], 32) == 3.0);
   ASSERT_TRUE(nir_const_value_as_float(imm_vec_instr->value[2], 32) == 4.0);
   ASSERT_TRUE(nir_const_value_as_float(imm_vec_instr->value[3], 32) == 5.0);
   ASSERT_TRUE(nir_const_value_as_float(imm_vec_instr->value[4], 32) == 2.0);
            ASSERT_TRUE(alu_result->num_components == 8);
   check_swizzle(&alu_instr->src[0], "xxxxxxy");
            check_swizzle(&alu2_instr->src[0], "abbcdefg");
                        }
      TEST_F(nir_opt_shrink_vectors_test, opt_shrink_phis_loop_simple)
   {
      /* Test that the phi is shrinked in the following case.
   *
   *    v = vec4(0.0, 0.0, 0.0, 0.0);
   *    while (v.y < 3) {
   *       v.y += 1.0;
   *    }
   *
   * This mimics nir for loops that come out of nine+ttn.
   */
   nir_def *v = nir_imm_vec4(b, 0.0, 0.0, 0.0, 0.0);
   nir_def *increment = nir_imm_float(b, 1.0);
            nir_phi_instr *const phi = nir_phi_instr_create(b->shader);
                                       nir_def *fge = nir_fge(b, phi_def, loop_max);
   nir_alu_instr *fge_alu_instr = nir_instr_as_alu(fge->parent_instr);
   fge->num_components = 1;
            nir_if *nif = nir_push_if(b, fge);
   {
      nir_jump_instr *jump = nir_jump_instr_create(b->shader, nir_jump_break);
      }
            nir_def *fadd = nir_fadd(b, phi_def, increment);
   nir_alu_instr *fadd_alu_instr = nir_instr_as_alu(fadd->parent_instr);
   fadd->num_components = 1;
            nir_scalar srcs[4] = {{0}};
   for (unsigned i = 0; i < 4; i++) {
         }
   srcs[1] = nir_get_scalar(fadd, 0);
                              b->cursor = nir_before_block(nir_loop_first_block(loop));
            /* Generated nir:
   *
   * impl main {
   *         block block_0:
   *         * preds: *
   *         vec1 32 ssa_0 = deref_var &in (shader_in vec2)
   *         vec2 32 ssa_1 = intrinsic load_deref (ssa_0) (access=0)
   *         vec4 32 ssa_2 = load_const (0x00000000, 0x00000000, 0x00000000, 0x00000000) = (0.000000, 0.000000, 0.000000, 0.000000)
   *         vec1 32 ssa_3 = load_const (0x3f800000 = 1.000000)
   *         vec1 32 ssa_4 = load_const (0x40400000 = 3.000000)
   *         * succs: block_1 *
   *         loop {
   *                 block block_1:
   *                 * preds: block_0 block_4 *
   *                 vec4 32 ssa_8 = phi block_0: ssa_2, block_4: ssa_7
   *                 vec1  1 ssa_5 = fge ssa_8.y, ssa_4
   *                 * succs: block_2 block_3 *
   *                 if ssa_5 {
   *                         block block_2:
   *                         * preds: block_1 *
   *                         break
   *                         * succs: block_5 *
   *                 } else {
   *                         block block_3:
   *                         * preds: block_1 *
   *                         * succs: block_4 *
   *                 }
   *                 block block_4:
   *                 * preds: block_3 *
   *                 vec1 32 ssa_6 = fadd ssa_8.y, ssa_3
   *                 vec4 32 ssa_7 = vec4 ssa_8.x, ssa_6, ssa_8.z, ssa_8.w
   *                 * succs: block_1 *
   *         }
   *         block block_5:
   *         * preds: block_2 *
   *         * succs: block_6 *
   *         block block_6:
   * }
                     ASSERT_TRUE(nir_opt_shrink_vectors(b->shader));
   ASSERT_TRUE(phi_def->num_components == 1);
   check_swizzle(&fge_alu_instr->src[0], "x");
               }
      TEST_F(nir_opt_shrink_vectors_test, opt_shrink_phis_loop_swizzle)
   {
      /* Test that the phi is shrinked properly in the following case where
   * some swizzling happens in the channels.
   *
   *    v = vec4(0.0, 0.0, 0.0, 0.0);
   *    while (v.z < 3) {
   *       v = vec4(v.x, v.z + 1, v.y, v.w};
   *    }
   */
   nir_def *v = nir_imm_vec4(b, 0.0, 0.0, 0.0, 0.0);
   nir_def *increment = nir_imm_float(b, 1.0);
            nir_phi_instr *const phi = nir_phi_instr_create(b->shader);
                                       nir_def *fge = nir_fge(b, phi_def, loop_max);
   nir_alu_instr *fge_alu_instr = nir_instr_as_alu(fge->parent_instr);
   fge->num_components = 1;
                        nir_jump_instr *jump = nir_jump_instr_create(b->shader, nir_jump_break);
                  nir_def *fadd = nir_fadd(b, phi_def, increment);
   nir_alu_instr *fadd_alu_instr = nir_instr_as_alu(fadd->parent_instr);
   fadd->num_components = 1;
            nir_scalar srcs[4] = {{0}};
   srcs[0] = nir_get_scalar(phi_def, 0);
   srcs[1] = nir_get_scalar(fadd, 0);
   srcs[2] = nir_get_scalar(phi_def, 1);
   srcs[3] = nir_get_scalar(phi_def, 3);
                              b->cursor = nir_before_block(nir_loop_first_block(loop));
            /* Generated nir:
   *
   * impl main {
   *         block block_0:
   *         * preds: *
   *         vec1 32 ssa_0 = deref_var &in (shader_in vec2)
   *         vec2 32 ssa_1 = intrinsic load_deref (ssa_0) (access=0)
   *         vec4 32 ssa_2 = load_const (0x00000000, 0x00000000, 0x00000000, 0x00000000) = (0.000000, 0.000000, 0.000000, 0.000000)
   *         vec1 32 ssa_3 = load_const (0x3f800000 = 1.000000)
   *         vec1 32 ssa_4 = load_const (0x40400000 = 3.000000)
   *         * succs: block_1 *
   *         loop {
   *                 block block_1:
   *                 * preds: block_0 block_4 *
   *                 vec4 32 ssa_8 = phi block_0: ssa_2, block_4: ssa_7
   *                 vec1  1 ssa_5 = fge ssa_8.z, ssa_4
   *                 * succs: block_2 block_3 *
   *                 if ssa_5 {
   *                         block block_2:
   *                         * preds: block_1 *
   *                         break
   *                         * succs: block_5 *
   *                 } else {
   *                         block block_3:
   *                         * preds: block_1 *
   *                         * succs: block_4 *
   *                 }
   *                 block block_4:
   *                 * preds: block_3 *
   *                 vec1 32 ssa_6 = fadd ssa_8.z, ssa_3
   *                 vec4 32 ssa_7 = vec4 ssa_8.x, ssa_6, ssa_8.y, ssa_8.w
   *                 * succs: block_1 *
   *         }
   *         block block_5:
   *         * preds: block_2 *
   *         * succs: block_6 *
   *         block block_6:
   * }
                     ASSERT_TRUE(nir_opt_shrink_vectors(b->shader));
            check_swizzle(&fge_alu_instr->src[0], "y");
               }
      TEST_F(nir_opt_shrink_vectors_test, opt_shrink_phis_loop_phi_out)
   {
      /* Test that the phi is not shrinked when used by intrinsic.
   *
   *    v = vec4(0.0, 0.0, 0.0, 0.0);
   *    while (v.y < 3) {
   *       v.y += 1.0;
   *    }
   *    out = v;
   */
   nir_def *v = nir_imm_vec4(b, 0.0, 0.0, 0.0, 0.0);
   nir_def *increment = nir_imm_float(b, 1.0);
            nir_phi_instr *const phi = nir_phi_instr_create(b->shader);
                                       nir_def *fge = nir_fge(b, phi_def, loop_max);
   nir_alu_instr *fge_alu_instr = nir_instr_as_alu(fge->parent_instr);
   fge->num_components = 1;
            nir_if *nif = nir_push_if(b, fge);
   {
      nir_jump_instr *jump = nir_jump_instr_create(b->shader, nir_jump_break);
      }
            nir_def *fadd = nir_fadd(b, phi_def, increment);
   nir_alu_instr *fadd_alu_instr = nir_instr_as_alu(fadd->parent_instr);
   fadd->num_components = 1;
            nir_scalar srcs[4] = {{0}};
   for (unsigned i = 0; i < 4; i++) {
         }
   srcs[1] = nir_get_scalar(fadd, 0);
                              out_var = nir_variable_create(b->shader,
                           b->cursor = nir_before_block(nir_loop_first_block(loop));
            /* Generated nir:
   *
   * impl main {
   *         block block_0:
   *         * preds: *
   *         vec1 32 ssa_0 = deref_var &in (shader_in vec2)
   *         vec2 32 ssa_1 = intrinsic load_deref (ssa_0) (access=0)
   *         vec4 32 ssa_2 = load_const (0x00000000, 0x00000000, 0x00000000, 0x00000000) = (0.000000, 0.000000, 0.000000, 0.000000)
   *         vec1 32 ssa_3 = load_const (0x3f800000 = 1.000000)
   *         vec1 32 ssa_4 = load_const (0x40400000 = 3.000000)
   *         * succs: block_1 *
   *         loop {
   *                 block block_1:
   *                 * preds: block_0 block_4 *
   *                 vec4 32 ssa_9 = phi block_0: ssa_2, block_4: ssa_7
   *                 vec1  1 ssa_5 = fge ssa_9.y, ssa_4
   *                 * succs: block_2 block_3 *
   *                 if ssa_5 {
   *                         block block_2:
   *                         * preds: block_1 *
   *                         break
   *                         * succs: block_5 *
   *                 } else {
   *                         block block_3:
   *                         * preds: block_1 *
   *                         * succs: block_4 *
   *                 }
   *                 block block_4:
   *                 * preds: block_3 *
   *                 vec1 32 ssa_6 = fadd ssa_9.y, ssa_3
   *                 vec4 32 ssa_7 = vec4 ssa_9.x, ssa_6, ssa_9.z, ssa_9.w
   *                 * succs: block_1 *
   *         }
   *         block block_5:
   *         * preds: block_2 *
   *         vec1 32 ssa_8 = deref_var &out4 (shader_out vec4)
   *         intrinsic store_deref (ssa_8, ssa_9) (wrmask=xyzw *15*, access=0)
   *         * succs: block_6 *
   *         block block_6:
   * }
                     ASSERT_FALSE(nir_opt_shrink_vectors(b->shader));
      }
