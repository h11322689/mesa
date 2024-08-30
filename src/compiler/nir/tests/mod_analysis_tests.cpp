   /*
   * Copyright © 2022 Intel Corporation
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
   #include "util/u_math.h"
      static inline bool
   nir_mod_analysis_comp0(nir_def *val, nir_alu_type val_type, unsigned div, unsigned *mod)
   {
         }
      class nir_mod_analysis_test : public nir_test {
   protected:
                        nir_def *v[50];
      };
      nir_mod_analysis_test::nir_mod_analysis_test()
         {
      for (int i = 0; i < 50; ++i)
            }
      /* returns src0 * src1.y */
   nir_def *
   nir_mod_analysis_test::nir_imul_vec2y(nir_builder *b, nir_def *src0, nir_def *src1)
   {
               instr->src[0].src = nir_src_for_ssa(src0);
   instr->src[1].src = nir_src_for_ssa(src1);
                     nir_builder_instr_insert(b, &instr->instr);
      }
      TEST_F(nir_mod_analysis_test, const_val)
   {
      /* const % const_mod should be always known */
   for (unsigned const_mod = 1; const_mod <= 1024; const_mod *= 2) {
      for (int cnst = 0; cnst < 10; ++cnst) {
               EXPECT_TRUE(nir_mod_analysis_comp0(v[cnst], nir_type_uint, const_mod, &mod));
            }
      TEST_F(nir_mod_analysis_test, dynamic)
   {
                        EXPECT_TRUE(nir_mod_analysis_comp0(invocation, nir_type_uint, 1, &mod));
            for (unsigned const_mod = 2; const_mod <= 1024; const_mod *= 2)
      }
      TEST_F(nir_mod_analysis_test, const_plus_const)
   {
      /* (const1 + const2) % const_mod should always be known */
   for (unsigned const_mod = 1; const_mod <= 1024; const_mod *= 2) {
      for (unsigned c1 = 0; c1 < 10; ++c1) {
                                 EXPECT_TRUE(nir_mod_analysis_comp0(sum, nir_type_uint, const_mod, &mod));
               }
      TEST_F(nir_mod_analysis_test, dynamic_plus_const)
   {
      /* (invocation + const) % const_mod should never be known unless const_mod is 1 */
   for (unsigned const_mod = 1; const_mod <= 1024; const_mod *= 2) {
      for (unsigned c = 0; c < 10; ++c) {
                        if (const_mod == 1) {
      EXPECT_TRUE(nir_mod_analysis_comp0(sum, nir_type_uint, const_mod, &mod));
      } else {
                  }
      TEST_F(nir_mod_analysis_test, const_mul_const)
   {
      /* (const1 * const2) % const_mod should always be known */
   for (unsigned const_mod = 1; const_mod <= 1024; const_mod *= 2) {
      for (unsigned c1 = 0; c1 < 10; ++c1) {
                                 EXPECT_TRUE(nir_mod_analysis_comp0(mul, nir_type_uint, const_mod, &mod));
               }
      TEST_F(nir_mod_analysis_test, dynamic_mul_const)
   {
      /* (invocation * const) % const_mod == 0 only if const % const_mod == 0, unknown otherwise */
   for (unsigned const_mod = 2; const_mod <= 1024; const_mod *= 2) {
      for (unsigned c = 0; c < 10; ++c) {
                        if (c % const_mod == 0) {
      EXPECT_TRUE(nir_mod_analysis_comp0(mul, nir_type_uint, const_mod, &mod));
      } else {
                  }
      TEST_F(nir_mod_analysis_test, dynamic_mul_const_swizzled)
   {
      /* (invocation * const.y) % const_mod == 0 only if const.y % const_mod == 0, unknown otherwise */
   for (unsigned const_mod = 2; const_mod <= 1024; const_mod *= 2) {
      for (unsigned c = 0; c < 10; ++c) {
                              if (c % const_mod == 0) {
      EXPECT_TRUE(nir_mod_analysis_comp0(mul, nir_type_uint, const_mod, &mod));
      } else {
                  }
      TEST_F(nir_mod_analysis_test, dynamic_mul32x16_const)
   {
      /* (invocation mul32x16 const) % const_mod == 0 only if const % const_mod == 0
   * and const_mod <= 2^16, unknown otherwise
   */
   for (unsigned const_mod = 1; const_mod <= (1u << 24); const_mod *= 2) {
      for (unsigned c = 0; c < 10; ++c) {
                        if (c % const_mod == 0 && const_mod <= (1u << 16)) {
      EXPECT_TRUE(nir_mod_analysis_comp0(mul, nir_type_uint, const_mod, &mod));
      } else {
                  }
      TEST_F(nir_mod_analysis_test, dynamic_shl_const)
   {
      /* (invocation << const) % const_mod == 0 only if const >= log2(const_mod), unknown otherwise */
   for (unsigned const_mod = 1; const_mod <= 1024; const_mod *= 2) {
      for (unsigned c = 0; c < 10; ++c) {
                        if (c >= util_logbase2(const_mod)) {
      EXPECT_TRUE(nir_mod_analysis_comp0(shl, nir_type_uint, const_mod, &mod));
      } else {
                  }
      TEST_F(nir_mod_analysis_test, dynamic_shr_const)
   {
      /* (invocation >> const) % const_mod should never be known, unless const_mod is 1 */
   for (unsigned const_mod = 1; const_mod <= 1024; const_mod *= 2) {
      for (unsigned i = 0; i < 10; ++i) {
                        if (const_mod == 1) {
      EXPECT_TRUE(nir_mod_analysis_comp0(shr, nir_type_uint, const_mod, &mod));
      } else {
                  }
      TEST_F(nir_mod_analysis_test, dynamic_mul_const_shr_const)
   {
      /* ((invocation * 32) >> const) % const_mod == 0 only if
   *   const_mod is 1 or
   *   (32 >> const) is not 0 and (32 >> const) % const_mod == 0
   *
   */
   nir_def *inv_mul_32 = nir_imul(b, invocation, v[32]);
   for (unsigned const_mod = 1; const_mod <= 1024; const_mod *= 2) {
      for (unsigned c = 0; c < 8; ++c) {
                        if (const_mod == 1 || ((32 >> c) > 0 && (32 >> c) % const_mod == 0)) {
      EXPECT_TRUE(nir_mod_analysis_comp0(shr, nir_type_uint, const_mod, &mod));
      } else {
                  }
      TEST_F(nir_mod_analysis_test, dynamic_mul_const_swizzled_shr_const)
   {
      /* ((invocation * ivec2(31, 32).y) >> const) % const_mod == 0 only if
   *   const_mod is 1 or
   *   (32 >> const) is not 0 and (32 >> const) % const_mod == 0
   *
   */
   nir_def *vec2 = nir_imm_ivec2(b, 31, 32);
            for (unsigned const_mod = 1; const_mod <= 1024; const_mod *= 2) {
      for (unsigned c = 0; c < 8; ++c) {
                        if (const_mod == 1 || ((32 >> c) > 0 && (32 >> c) % const_mod == 0)) {
      EXPECT_TRUE(nir_mod_analysis_comp0(shr, nir_type_uint, const_mod, &mod));
      } else {
                  }
      TEST_F(nir_mod_analysis_test, const_shr_const)
   {
      /* (const >> const) % const_mod should always be known */
   for (unsigned const_mod = 1; const_mod <= 1024; const_mod *= 2) {
      for (unsigned i = 0; i < 50; ++i) {
                                 EXPECT_TRUE(nir_mod_analysis_comp0(shr, nir_type_uint, const_mod, &mod));
               }
      TEST_F(nir_mod_analysis_test, const_shr_const_overflow)
   {
      /* (large_const >> const_shr) % const_mod should be known if
   * const_mod << const_shr is still below UINT32_MAX.
   */
   unsigned large_const_int = 0x12345678;
            for (unsigned shift = 0; shift < 30; ++shift) {
               for (unsigned const_mod = 1; const_mod <= 1024; const_mod *= 2) {
               if ((((uint64_t)const_mod) << shift) > UINT32_MAX) {
         } else {
      EXPECT_TRUE(nir_mod_analysis_comp0(shr, nir_type_uint, const_mod, &mod));
               }
