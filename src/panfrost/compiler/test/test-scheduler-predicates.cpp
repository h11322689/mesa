   /*
   * Copyright (C) 2021 Collabora, Ltd.
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
   #include "compiler.h"
      #include <gtest/gtest.h>
      class SchedulerPredicates : public testing::Test {
   protected:
      SchedulerPredicates()
   {
      mem_ctx = ralloc_context(NULL);
      }
   ~SchedulerPredicates()
   {
                  bi_index TMP()
   {
                  void *mem_ctx;
      };
      TEST_F(SchedulerPredicates, MOV)
   {
      bi_instr *mov = bi_mov_i32_to(b, TMP(), TMP());
   ASSERT_TRUE(bi_can_fma(mov));
   ASSERT_TRUE(bi_can_add(mov));
   ASSERT_FALSE(bi_must_message(mov));
   ASSERT_TRUE(bi_reads_zero(mov));
   ASSERT_TRUE(bi_reads_temps(mov, 0));
      }
      TEST_F(SchedulerPredicates, FMA)
   {
      bi_instr *fma = bi_fma_f32_to(b, TMP(), TMP(), TMP(), bi_zero());
   ASSERT_TRUE(bi_can_fma(fma));
   ASSERT_FALSE(bi_can_add(fma));
   ASSERT_FALSE(bi_must_message(fma));
   ASSERT_TRUE(bi_reads_zero(fma));
   for (unsigned i = 0; i < 3; ++i) {
      ASSERT_TRUE(bi_reads_temps(fma, i));
         }
      TEST_F(SchedulerPredicates, LOAD)
   {
      bi_instr *load = bi_load_i128_to(b, TMP(), TMP(), TMP(), BI_SEG_UBO, 0);
   ASSERT_FALSE(bi_can_fma(load));
   ASSERT_TRUE(bi_can_add(load));
   ASSERT_TRUE(bi_must_message(load));
   for (unsigned i = 0; i < 2; ++i) {
      ASSERT_TRUE(bi_reads_temps(load, i));
         }
      TEST_F(SchedulerPredicates, BLEND)
   {
      bi_instr *blend = bi_blend_to(b, TMP(), TMP(), TMP(), TMP(), TMP(), TMP(),
         ASSERT_FALSE(bi_can_fma(blend));
   ASSERT_TRUE(bi_can_add(blend));
   ASSERT_TRUE(bi_must_message(blend));
   for (unsigned i = 0; i < 4; ++i)
         ASSERT_FALSE(bi_reads_t(blend, 0));
   ASSERT_TRUE(bi_reads_t(blend, 1));
   ASSERT_FALSE(bi_reads_t(blend, 2));
      }
      TEST_F(SchedulerPredicates, RestrictionsOnModifiersOfSameCycleTemporaries)
   {
      bi_instr *fadd = bi_fadd_f32_to(b, TMP(), TMP(), TMP());
            for (unsigned i = 0; i < 2; ++i) {
      for (unsigned j = 0; j < 2; ++j) {
      bi_instr *fadd = bi_fadd_f32_to(b, TMP(), TMP(), TMP());
   fadd->src[i] = bi_swz_16(TMP(), j, j);
   ASSERT_TRUE(bi_reads_t(fadd, 1 - i));
            }
      TEST_F(SchedulerPredicates, RestrictionsOnFAddV2F16)
   {
      bi_index x = bi_register(0);
            /* Basic */
            ASSERT_TRUE(bi_can_fma(fadd));
            /* With imbalanced abs */
            ASSERT_TRUE(bi_can_fma(fadd));
            /* With equal abs */
            ASSERT_FALSE(bi_can_fma(fadd));
            /* With equal abs but different sources */
            ASSERT_TRUE(bi_can_fma(fadd));
            /* With clamp */
            ASSERT_TRUE(bi_can_fma(fadd));
            /* Impossible encoding (should never be seen) */
            ASSERT_FALSE(bi_can_fma(fadd));
      }
      TEST_F(SchedulerPredicates, BranchOffset)
   {
      bi_instr *jump = bi_jump(b, TMP());
      }
