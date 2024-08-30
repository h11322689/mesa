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
   #include "compiler.h"
      #include <gtest/gtest.h>
      #define CASE(instr, expected)                                                  \
         #define NEGCASE(instr) CASE(instr, instr)
      class LowerSwizzle : public testing::Test {
   protected:
      LowerSwizzle()
   {
               reg = bi_register(0);
   x = bi_register(1);
   y = bi_register(2);
   z = bi_register(3);
            x3210 = x;
               ~LowerSwizzle()
   {
                           bi_index reg, x, y, z, w;
      };
      TEST_F(LowerSwizzle, Csel16)
   {
      CASE(bi_csel_v2f16_to(b, reg, bi_half(x, 0), y, z, w, BI_CMPF_NE),
      bi_csel_v2f16_to(b, reg, bi_swz_v2i16(b, bi_half(x, 0)), y, z, w,
   }
      TEST_F(LowerSwizzle, Fma16)
   {
      NEGCASE(bi_fadd_v2f16_to(b, reg, bi_half(x, 0), y));
      }
      TEST_F(LowerSwizzle, ClzHadd8)
   {
      CASE(bi_clz_v4u8_to(b, reg, x3210, true),
            CASE(bi_hadd_v4u8_to(b, reg, y, x3210, BI_ROUND_RTP),
      }
      TEST_F(LowerSwizzle, FirstShift8)
   {
      enum bi_opcode ops[] = {
      BI_OPCODE_LSHIFT_AND_V4I8, BI_OPCODE_LSHIFT_OR_V4I8,
   BI_OPCODE_LSHIFT_XOR_V4I8, BI_OPCODE_RSHIFT_AND_V4I8,
               for (unsigned i = 0; i < ARRAY_SIZE(ops); ++i) {
      CASE(
      {
      bi_instr *I = bi_lshift_and_v4i8_to(b, reg, x3210, y, z);
      },
   {
      bi_instr *I =
               }
      TEST_F(LowerSwizzle, ShiftWithNot)
   {
      CASE(bi_lshift_and_v4i8_to(b, reg, y, bi_neg(x3210), z),
      }
      TEST_F(LowerSwizzle, FClampSwap)
   {
      CASE(bi_fclamp_v2f16_to(b, reg, bi_swz_16(x, 1, 0)),
      }
