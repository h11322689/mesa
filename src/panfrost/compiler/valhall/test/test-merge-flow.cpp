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
      #define CASE(test, expected)                                                   \
      do {                                                                        \
      bi_builder *A = bit_builder(mem_ctx);                                    \
   bi_builder *B = bit_builder(mem_ctx);                                    \
   {                                                                        \
      bi_builder *b = A;                                                    \
   A->shader->stage = MESA_SHADER_FRAGMENT;                              \
      }                                                                        \
   va_merge_flow(A->shader);                                                \
   {                                                                        \
      bi_builder *b = B;                                                    \
   B->shader->stage = MESA_SHADER_FRAGMENT;                              \
      }                                                                        \
            #define NEGCASE(test) CASE(test, test)
      #define flow(f) bi_nop(b)->flow = VA_FLOW_##f
      class MergeFlow : public testing::Test {
   protected:
      MergeFlow()
   {
      mem_ctx = ralloc_context(NULL);
               ~MergeFlow()
   {
                  void *mem_ctx;
   bi_instr *I;
      };
      TEST_F(MergeFlow, End)
   {
      CASE(
      {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   bi_blend_to(b, bi_register(0), bi_register(4), bi_register(5),
                  },
   {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   I = bi_blend_to(b, bi_register(0), bi_register(4), bi_register(5),
                  }
      TEST_F(MergeFlow, Reconverge)
   {
      CASE(
      {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   bi_blend_to(b, bi_register(0), bi_register(4), bi_register(5),
                  },
   {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   I = bi_blend_to(b, bi_register(0), bi_register(4), bi_register(5),
                  }
      TEST_F(MergeFlow, TrivialWait)
   {
      CASE(
      {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   flow(WAIT0126);
      },
   {
      I = bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   I->flow = VA_FLOW_WAIT0126;
      }
      TEST_F(MergeFlow, LoadThenUnrelatedThenUse)
   {
      CASE(
      {
      bi_ld_attr_imm_to(b, bi_register(16), bi_register(60), bi_register(61),
         bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   flow(WAIT0);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(19));
      },
   {
      bi_ld_attr_imm_to(b, bi_register(16), bi_register(60), bi_register(61),
         I = bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   I->flow = VA_FLOW_WAIT0;
   I = bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(19));
      }
      TEST_F(MergeFlow, TrivialDiscard)
   {
      CASE(
      {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   bi_clper_i32_to(b, bi_register(0), bi_register(4), bi_register(8),
               flow(DISCARD);
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
      },
   {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   I = bi_clper_i32_to(b, bi_register(0), bi_register(4), bi_register(8),
               I->flow = VA_FLOW_DISCARD;
   I = bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
      }
      TEST_F(MergeFlow, TrivialDiscardAtTheStart)
   {
      CASE(
      {
      flow(DISCARD);
      },
   {
      I = bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
      }
      TEST_F(MergeFlow, MoveDiscardPastWait)
   {
      CASE(
      {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   bi_clper_i32_to(b, bi_register(0), bi_register(4), bi_register(8),
               flow(DISCARD);
   flow(WAIT0);
      },
   {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   I = bi_clper_i32_to(b, bi_register(0), bi_register(4), bi_register(8),
               I->flow = VA_FLOW_WAIT0;
   I = bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
      }
      TEST_F(MergeFlow, OccludedWaitsAndDiscard)
   {
      CASE(
      {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   bi_clper_i32_to(b, bi_register(0), bi_register(4), bi_register(8),
               flow(WAIT0);
   flow(DISCARD);
   flow(WAIT2);
      },
   {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   I = bi_clper_i32_to(b, bi_register(0), bi_register(4), bi_register(8),
               I->flow = VA_FLOW_WAIT02;
   I = bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
      }
      TEST_F(MergeFlow, DeleteUselessWaits)
   {
      CASE(
      {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   flow(WAIT0);
   flow(WAIT2);
      },
   {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   I = bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
      }
      TEST_F(MergeFlow, BlockFullOfUselessWaits)
   {
      CASE(
      {
      flow(WAIT0);
   flow(WAIT2);
   flow(DISCARD);
      },
   }
      TEST_F(MergeFlow, WaitWithMessage)
   {
      CASE(
      {
      bi_ld_attr_imm_to(b, bi_register(16), bi_register(60), bi_register(61),
            },
   {
      I = bi_ld_attr_imm_to(b, bi_register(16), bi_register(60),
                  }
      TEST_F(MergeFlow, CantMoveWaitPastMessage)
   {
      NEGCASE({
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   I =
                  /* Pretend it's blocked for some reason. This doesn't actually happen
   * with the current algorithm, but it's good to handle the special
   * cases correctly in case we change later on.
   */
   I->flow = VA_FLOW_DISCARD;
         }
      TEST_F(MergeFlow, DeletePointlessDiscard)
   {
      CASE(
      {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   bi_tex_single_to(b, bi_register(0), bi_register(4), bi_register(8),
                     flow(DISCARD);
   flow(WAIT0);
   flow(WAIT0126);
   bi_atest_to(b, bi_register(0), bi_register(4), bi_register(5), atest);
   flow(WAIT);
   bi_blend_to(b, bi_register(0), bi_register(4), bi_register(5),
                  },
   {
      bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_register(0));
   I = bi_tex_single_to(
      b, bi_register(0), bi_register(4), bi_register(8), bi_register(12),
   false, BI_DIMENSION_2D, BI_REGISTER_FORMAT_F32, false, false,
      I->flow = VA_FLOW_WAIT0126;
   I = bi_atest_to(b, bi_register(0), bi_register(4), bi_register(5),
         I->flow = VA_FLOW_WAIT;
   I = bi_blend_to(b, bi_register(0), bi_register(4), bi_register(5),
                  }
      TEST_F(MergeFlow, PreserveTerminalBarriers)
   {
      CASE(
      {
      bi_barrier(b);
   flow(WAIT);
      },
   {
      bi_barrier(b)->flow = VA_FLOW_WAIT;
      }
