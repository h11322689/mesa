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
      #include "util/u_cpu_detect.h"
   #include "bi_builder.h"
   #include "bi_test.h"
   #include "va_compiler.h"
      #include <gtest/gtest.h>
      static inline void
   add_imm(bi_context *ctx)
   {
      bi_foreach_instr_global(ctx, I) {
            }
      #define CASE(instr, expected) INSTRUCTION_CASE(instr, expected, add_imm)
   #define NEGCASE(instr)        CASE(instr, instr)
      class AddImm : public testing::Test {
   protected:
      AddImm()
   {
                  ~AddImm()
   {
                     };
      TEST_F(AddImm, Basic)
   {
      CASE(bi_mov_i32_to(b, bi_register(63), bi_imm_u32(0xABAD1DEA)),
            CASE(bi_fadd_f32_to(b, bi_register(1), bi_register(2), bi_imm_f32(42.0)),
            CASE(bi_fadd_f32_to(b, bi_register(1), bi_discard(bi_register(2)),
            bi_fadd_imm_f32_to(b, bi_register(1), bi_discard(bi_register(2)),
         CASE(bi_fadd_f32_to(b, bi_register(1), bi_discard(bi_register(2)),
            bi_fadd_imm_f32_to(b, bi_register(1), bi_discard(bi_register(2)),
   }
      TEST_F(AddImm, Commutativty)
   {
      CASE(bi_fadd_f32_to(b, bi_register(1), bi_imm_f32(42.0), bi_register(2)),
      }
      TEST_F(AddImm, NoModifiers)
   {
      NEGCASE(bi_fadd_f32_to(b, bi_register(1), bi_abs(bi_register(2)),
         NEGCASE(bi_fadd_f32_to(b, bi_register(1), bi_neg(bi_register(2)),
         NEGCASE(bi_fadd_f32_to(b, bi_register(1),
            }
      TEST_F(AddImm, NoClamp)
   {
      NEGCASE({
      bi_instr *I =
               }
      TEST_F(AddImm, OtherTypes)
   {
      CASE(bi_fadd_v2f16_to(b, bi_register(1), bi_register(2), bi_imm_f16(42.0)),
            CASE(bi_iadd_u32_to(b, bi_register(1), bi_register(2),
                  CASE(bi_iadd_v2u16_to(b, bi_register(1), bi_register(2),
                  CASE(bi_iadd_v4u8_to(b, bi_register(1), bi_register(2),
                  CASE(bi_iadd_s32_to(b, bi_register(1), bi_register(2),
                  CASE(bi_iadd_v2s16_to(b, bi_register(1), bi_register(2),
                  CASE(bi_iadd_v4s8_to(b, bi_register(1), bi_register(2),
                  NEGCASE(bi_iadd_u32_to(b, bi_register(1),
               NEGCASE(bi_iadd_v2u16_to(b, bi_register(1),
               NEGCASE(bi_iadd_u32_to(b, bi_register(1), bi_register(2),
         NEGCASE(bi_iadd_s32_to(b, bi_register(1),
               NEGCASE(bi_iadd_v2s16_to(b, bi_register(1),
                  NEGCASE(bi_iadd_s32_to(b, bi_register(1), bi_register(2),
      }
      TEST_F(AddImm, Int8)
   {
      bi_index idx = bi_register(2);
   idx.swizzle = BI_SWIZZLE_B0000;
   NEGCASE(
         NEGCASE(
      }
      TEST_F(AddImm, OnlyRTE)
   {
      NEGCASE({
      bi_instr *I =
                     NEGCASE({
      bi_instr *I =
               }
