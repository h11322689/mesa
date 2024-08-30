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
      static std::string
   to_string(const bi_instr *I)
   {
      char *cstr = NULL;
   size_t size = 0;
   FILE *f = open_memstream(&cstr, &size);
   bi_print_instr(I, f);
   fclose(f);
   auto str = std::string(cstr);
   free(cstr);
      }
      static testing::AssertionResult
   constant_fold_pred(const char *I_expr, const char *expected_expr, bi_instr *I,
         {
      bool unsupported = false;
   uint32_t v = bi_fold_constant(I, &unsupported);
   if (unsupported) {
      return testing::AssertionFailure()
            } else if (v != expected) {
      return testing::AssertionFailure()
         << "Unexpected result when constant folding instruction\n\n"
   << "  " << to_string(I) << "\n"
      } else {
            }
      #define EXPECT_FOLD(i, e) EXPECT_PRED_FORMAT2(constant_fold_pred, i, e)
      static testing::AssertionResult
   not_constant_fold_pred(const char *I_expr, bi_instr *I)
   {
      bool unsupported = false;
   uint32_t v = bi_fold_constant(I, &unsupported);
   if (unsupported) {
         } else {
      return testing::AssertionFailure()
         << "Instruction\n\n"
         }
      #define EXPECT_NOT_FOLD(i) EXPECT_PRED_FORMAT1(not_constant_fold_pred, i)
      class ConstantFold : public testing::Test {
   protected:
      ConstantFold()
   {
      mem_ctx = ralloc_context(NULL);
      }
   ~ConstantFold()
   {
                  void *mem_ctx;
      };
      TEST_F(ConstantFold, Swizzles)
   {
                        EXPECT_FOLD(
      bi_swz_v2i16_to(b, reg, bi_swz_16(bi_imm_u32(0xCAFEBABE), false, false)),
         EXPECT_FOLD(
      bi_swz_v2i16_to(b, reg, bi_swz_16(bi_imm_u32(0xCAFEBABE), true, false)),
         EXPECT_FOLD(
      bi_swz_v2i16_to(b, reg, bi_swz_16(bi_imm_u32(0xCAFEBABE), true, true)),
   }
      TEST_F(ConstantFold, VectorConstructions2i16)
   {
               EXPECT_FOLD(
      bi_mkvec_v2i16_to(b, reg, bi_imm_u16(0xCAFE), bi_imm_u16(0xBABE)),
         EXPECT_FOLD(
      bi_mkvec_v2i16_to(b, reg, bi_swz_16(bi_imm_u32(0xCAFEBABE), true, true),
               EXPECT_FOLD(
      bi_mkvec_v2i16_to(b, reg, bi_swz_16(bi_imm_u32(0xCAFEBABE), true, true),
         }
      TEST_F(ConstantFold, VectorConstructions4i8)
   {
      bi_index reg = bi_register(0);
            bi_index a = bi_byte(u32, 0); /* 0xBE */
            EXPECT_FOLD(bi_mkvec_v4i8_to(b, reg, a, a, a, a), 0xBEBEBEBE);
   EXPECT_FOLD(bi_mkvec_v4i8_to(b, reg, a, c, a, c), 0xFEBEFEBE);
   EXPECT_FOLD(bi_mkvec_v4i8_to(b, reg, c, a, c, a), 0xBEFEBEFE);
      }
      TEST_F(ConstantFold, VectorConstructions2i8)
   {
      bi_index reg = bi_register(0);
   bi_index u32 = bi_imm_u32(0xCAFEBABE);
            bi_index a = bi_byte(u32, 0); /* 0xBE */
   bi_index B = bi_byte(u32, 1); /* 0xBA */
   bi_index c = bi_byte(u32, 2); /* 0xFE */
            EXPECT_FOLD(bi_mkvec_v2i8_to(b, reg, a, B, rem), 0x1234BABE);
   EXPECT_FOLD(bi_mkvec_v2i8_to(b, reg, a, d, rem), 0x1234CABE);
   EXPECT_FOLD(bi_mkvec_v2i8_to(b, reg, c, d, rem), 0x1234CAFE);
      }
      TEST_F(ConstantFold, LimitedShiftsForTexturing)
   {
               EXPECT_FOLD(bi_lshift_or_i32_to(b, reg, bi_imm_u32(0xCAFE),
                  EXPECT_NOT_FOLD(bi_lshift_or_i32_to(
            EXPECT_NOT_FOLD(bi_lshift_or_i32_to(b, reg, bi_not(bi_imm_u32(0xCAFE)),
            bi_instr *I = bi_lshift_or_i32_to(b, reg, bi_imm_u32(0xCAFE),
         I->not_result = true;
      }
      TEST_F(ConstantFold, NonConstantSourcesCannotBeFolded)
   {
               EXPECT_NOT_FOLD(bi_swz_v2i16_to(b, reg, bi_temp(b->shader)));
   EXPECT_NOT_FOLD(
         EXPECT_NOT_FOLD(
         EXPECT_NOT_FOLD(
      }
      TEST_F(ConstantFold, OtherOperationsShouldNotFold)
   {
      bi_index zero = bi_fau(bir_fau(BIR_FAU_IMMEDIATE | 0), false);
            EXPECT_NOT_FOLD(bi_fma_f32_to(b, reg, zero, zero, zero));
      }
