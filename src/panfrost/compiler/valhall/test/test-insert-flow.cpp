   /*
   * Copyright (C) 2022 Collabora, Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "bi_builder.h"
   #include "bi_test.h"
   #include "va_compiler.h"
   #include "valhall_enums.h"
      #include <gtest/gtest.h>
      static void
   strip_nops(bi_context *ctx)
   {
      bi_foreach_instr_global_safe(ctx, I) {
      if (I->op == BI_OPCODE_NOP)
         }
      #define CASE(shader_stage, test)                                               \
      do {                                                                        \
      bi_builder *A = bit_builder(mem_ctx);                                    \
   bi_builder *B = bit_builder(mem_ctx);                                    \
   {                                                                        \
      UNUSED bi_builder *b = A;                                             \
   A->shader->stage = MESA_SHADER_##shader_stage;                        \
      }                                                                        \
   strip_nops(A->shader);                                                   \
   va_insert_flow_control_nops(A->shader);                                  \
   {                                                                        \
      UNUSED bi_builder *b = B;                                             \
   B->shader->stage = MESA_SHADER_##shader_stage;                        \
      }                                                                        \
            #define flow(f) bi_nop(b)->flow = VA_FLOW_##f
      class InsertFlow : public testing::Test {
   protected:
      InsertFlow()
   {
                  ~InsertFlow()
   {
                     };
      TEST_F(InsertFlow, PreserveEmptyShader)
   {
         }
      TEST_F(InsertFlow, TilebufferWait7)
   {
      CASE(FRAGMENT, {
      flow(DISCARD);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   flow(WAIT);
   bi_blend_to(b, bi_register(0), bi_register(4), bi_register(5),
                           CASE(FRAGMENT, {
      flow(DISCARD);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   flow(WAIT);
   bi_st_tile(b, bi_register(0), bi_register(4), bi_register(5),
                     CASE(FRAGMENT, {
      flow(DISCARD);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   flow(WAIT);
   bi_ld_tile_to(b, bi_register(0), bi_register(4), bi_register(5),
               }
      TEST_F(InsertFlow, AtestWait6AndWait0After)
   {
      CASE(FRAGMENT, {
      flow(DISCARD);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   flow(WAIT0126);
   bi_atest_to(b, bi_register(0), bi_register(4), bi_register(5),
         flow(WAIT0);
         }
      TEST_F(InsertFlow, ZSEmitWait6)
   {
      CASE(FRAGMENT, {
      flow(DISCARD);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   flow(WAIT0126);
   bi_zs_emit_to(b, bi_register(0), bi_register(4), bi_register(5),
               }
      TEST_F(InsertFlow, LoadThenUnrelatedThenUse)
   {
      CASE(VERTEX, {
      bi_ld_attr_imm_to(b, bi_register(16), bi_register(60), bi_register(61),
         bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   flow(WAIT0);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(19));
         }
      TEST_F(InsertFlow, SingleLdVar)
   {
      CASE(FRAGMENT, {
      flow(DISCARD);
   bi_ld_var_buf_imm_f16_to(b, bi_register(2), bi_register(61),
                     flow(WAIT0);
         }
      TEST_F(InsertFlow, SerializeLdVars)
   {
      CASE(FRAGMENT, {
      flow(DISCARD);
   bi_ld_var_buf_imm_f16_to(b, bi_register(16), bi_register(61),
                     bi_ld_var_buf_imm_f16_to(b, bi_register(2), bi_register(61),
                     flow(WAIT0);
   bi_ld_var_buf_imm_f16_to(b, bi_register(8), bi_register(61),
                     flow(WAIT0);
         }
      TEST_F(InsertFlow, Clper)
   {
      CASE(FRAGMENT, {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   bi_clper_i32_to(b, bi_register(0), bi_register(4), bi_register(8),
               flow(DISCARD);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
         }
      TEST_F(InsertFlow, TextureImplicit)
   {
      CASE(FRAGMENT, {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   bi_tex_single_to(b, bi_register(0), bi_register(4), bi_register(8),
                     flow(DISCARD);
   flow(WAIT0);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
         }
      TEST_F(InsertFlow, TextureExplicit)
   {
      CASE(FRAGMENT, {
      flow(DISCARD);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   bi_tex_single_to(b, bi_register(0), bi_register(4), bi_register(8),
                     flow(WAIT0);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
         }
      /*      A
   *     / \
   *    B   C
   *     \ /
   *      D
   */
   TEST_F(InsertFlow, DiamondCFG)
   {
      CASE(FRAGMENT, {
      bi_block *A = bi_start_block(&b->shader->blocks);
   bi_block *B = bit_block(b->shader);
   bi_block *C = bit_block(b->shader);
            bi_block_add_successor(A, B);
            bi_block_add_successor(B, D);
            /* B uses helper invocations, no other block does.
   *
   * That means B and C need to discard helpers.
   */
   b->cursor = bi_after_block(B);
   bi_clper_i32_to(b, bi_register(0), bi_register(4), bi_register(8),
               flow(DISCARD);
            b->cursor = bi_after_block(C);
   flow(DISCARD);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
            b->cursor = bi_after_block(D);
         }
      TEST_F(InsertFlow, BarrierBug)
   {
      CASE(KERNEL, {
      bi_instr *I = bi_store_i32(b, bi_register(0), bi_register(2),
                  bi_fadd_f32_to(b, bi_register(10), bi_register(10), bi_register(10));
   flow(WAIT2);
   bi_barrier(b);
   flow(WAIT);
         }
