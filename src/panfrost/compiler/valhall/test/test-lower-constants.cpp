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
   #include "va_compiler.h"
      #include <gtest/gtest.h>
      static inline void
   add_imm(bi_context *ctx)
   {
      bi_foreach_instr_global(ctx, I) {
            }
      #define CASE(instr, expected) INSTRUCTION_CASE(instr, expected, add_imm)
      class LowerConstants : public testing::Test {
   protected:
      LowerConstants()
   {
                  ~LowerConstants()
   {
                     };
      TEST_F(LowerConstants, Float32)
   {
      CASE(bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_imm_f32(0.0)),
            CASE(bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_imm_f32(1.0)),
            CASE(bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_imm_f32(0.1)),
      }
      TEST_F(LowerConstants, WidenFloat16)
   {
      CASE(bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_imm_f32(0.5)),
      bi_fadd_f32_to(b, bi_register(0), bi_register(0),
         CASE(bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_imm_f32(255.0)),
      bi_fadd_f32_to(b, bi_register(0), bi_register(0),
         CASE(bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_imm_f32(256.0)),
      bi_fadd_f32_to(b, bi_register(0), bi_register(0),
         CASE(bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_imm_f32(8.0)),
      bi_fadd_f32_to(b, bi_register(0), bi_register(0),
   }
      TEST_F(LowerConstants, ReplicateFloat16)
   {
      CASE(bi_fadd_v2f16_to(b, bi_register(0), bi_register(0), bi_imm_f16(255.0)),
      bi_fadd_v2f16_to(b, bi_register(0), bi_register(0),
         CASE(bi_fadd_v2f16_to(b, bi_register(0), bi_register(0), bi_imm_f16(4.0)),
      bi_fadd_v2f16_to(b, bi_register(0), bi_register(0),
   }
      TEST_F(LowerConstants, NegateFloat32)
   {
      CASE(bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_imm_f32(-1.0)),
            CASE(bi_fadd_f32_to(b, bi_register(0), bi_register(0), bi_imm_f32(-255.0)),
      bi_fadd_f32_to(b, bi_register(0), bi_register(0),
   }
      TEST_F(LowerConstants, NegateReplicateFloat16)
   {
      CASE(bi_fadd_v2f16_to(b, bi_register(0), bi_register(0), bi_imm_f16(-255.0)),
      bi_fadd_v2f16_to(b, bi_register(0), bi_register(0),
   }
      TEST_F(LowerConstants, NegateVec2Float16)
   {
      CASE(
      bi_fadd_v2f16_to(b, bi_register(0), bi_register(0),
         }
      TEST_F(LowerConstants, Int8InInt32)
   {
      CASE(bi_lshift_or_i32(b, bi_register(0), bi_imm_u32(0), bi_imm_u8(6)),
            CASE(bi_lshift_or_i32(b, bi_register(0), bi_imm_u32(0), bi_imm_u8(-2)),
      }
      TEST_F(LowerConstants, ZeroExtendForUnsigned)
   {
      CASE(bi_icmp_and_u32_to(b, bi_register(0), bi_register(0), bi_imm_u32(0xFF),
            bi_icmp_and_u32_to(b, bi_register(0), bi_register(0),
               CASE(
      bi_icmp_and_u32_to(b, bi_register(0), bi_register(0), bi_imm_u32(0xFFFF),
         bi_icmp_and_u32_to(b, bi_register(0), bi_register(0),
         }
      TEST_F(LowerConstants, SignExtendPositiveForSigned)
   {
      CASE(bi_icmp_and_s32_to(b, bi_register(0), bi_register(0), bi_imm_u32(0x7F),
            bi_icmp_and_s32_to(b, bi_register(0), bi_register(0),
               CASE(
      bi_icmp_and_s32_to(b, bi_register(0), bi_register(0), bi_imm_u32(0x7FFF),
         bi_icmp_and_s32_to(b, bi_register(0), bi_register(0),
         }
      TEST_F(LowerConstants, SignExtendNegativeForSigned)
   {
      CASE(bi_icmp_and_s32_to(b, bi_register(0), bi_register(0),
                  bi_icmp_and_s32_to(b, bi_register(0), bi_register(0),
               CASE(bi_icmp_and_s32_to(b, bi_register(0), bi_register(0),
                  bi_icmp_and_s32_to(b, bi_register(0), bi_register(0),
         }
      TEST_F(LowerConstants, DontZeroExtendForSigned)
   {
      CASE(bi_icmp_and_s32_to(b, bi_register(0), bi_register(0), bi_imm_u32(0xFF),
            bi_icmp_and_s32_to(b, bi_register(0), bi_register(0),
               CASE(
      bi_icmp_and_s32_to(b, bi_register(0), bi_register(0), bi_imm_u32(0xFFFF),
         bi_icmp_and_s32_to(b, bi_register(0), bi_register(0),
         }
      TEST_F(LowerConstants, DontZeroExtendNegative)
   {
      CASE(bi_icmp_and_u32_to(b, bi_register(0), bi_register(0),
                  bi_icmp_and_u32_to(b, bi_register(0), bi_register(0),
               CASE(bi_icmp_and_u32_to(b, bi_register(0), bi_register(0),
                  bi_icmp_and_u32_to(b, bi_register(0), bi_register(0),
         }
      TEST_F(LowerConstants, HandleTrickyNegativesFP16)
   {
      CASE(
      bi_fadd_v2f16_to(b, bi_register(0), bi_register(0), bi_imm_f16(-57216.0)),
   bi_fadd_v2f16_to(b, bi_register(0), bi_register(0),
         CASE(
      bi_fadd_v2f16_to(b, bi_register(0), bi_register(0), bi_imm_f16(57216.0)),
   bi_fadd_v2f16_to(b, bi_register(0), bi_register(0),
   }
      TEST_F(LowerConstants, MaintainMkvecRestrictedSwizzles)
   {
      CASE(bi_mkvec_v2i8_to(b, bi_register(0), bi_register(0), bi_imm_u8(0),
            bi_mkvec_v2i8_to(b, bi_register(0), bi_register(0),
         CASE(bi_mkvec_v2i8_to(b, bi_register(0), bi_register(0), bi_imm_u8(14),
            bi_mkvec_v2i8_to(b, bi_register(0), bi_register(0),
   }
