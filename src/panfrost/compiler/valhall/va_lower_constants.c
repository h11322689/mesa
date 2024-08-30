   /*
   * Copyright (C) 2021 Collabora Ltd.
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
   #include "va_compiler.h"
   #include "valhall.h"
      /* Only some special immediates are available, as specified in the Table of
   * Immediates in the specification. Other immediates must be lowered, either to
   * uniforms or to moves.
   */
      static bi_index
   va_mov_imm(bi_builder *b, uint32_t imm)
   {
      bi_index zero = bi_fau(BIR_FAU_IMMEDIATE | 0, false);
      }
      static bi_index
   va_lut_index_32(uint32_t imm)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(valhall_immediates); ++i) {
      if (valhall_immediates[i] == imm)
                  }
      static bi_index
   va_lut_index_16(uint16_t imm)
   {
               for (unsigned i = 0; i < (2 * ARRAY_SIZE(valhall_immediates)); ++i) {
      if (arr16[i] == imm)
                  }
      UNUSED static bi_index
   va_lut_index_8(uint8_t imm)
   {
               for (unsigned i = 0; i < (4 * ARRAY_SIZE(valhall_immediates)); ++i) {
      if (arr8[i] == imm)
                  }
      static bi_index
   va_demote_constant_fp16(uint32_t value)
   {
               /* Only convert if it is exact */
   if (fui(_mesa_half_to_float(fp16)) == value)
         else
      }
      /*
   * Test if a 32-bit word arises as a sign or zero extension of some 8/16-bit
   * value.
   */
   static bool
   is_extension_of_8(uint32_t x, bool is_signed)
   {
      if (is_signed)
         else
      }
      static bool
   is_extension_of_16(uint32_t x, bool is_signed)
   {
      if (is_signed)
         else
      }
      static bi_index
   va_resolve_constant(bi_builder *b, uint32_t value, struct va_src_info info,
         {
      /* Try the constant as-is */
   if (!staging) {
      bi_index lut = va_lut_index_32(value);
   if (!bi_is_null(lut))
            /* ...or negated as a FP32 constant */
   if (info.absneg && info.size == VA_SIZE_32) {
      lut = bi_neg(va_lut_index_32(fui(-uif(value))));
   if (!bi_is_null(lut))
               /* ...or negated as a FP16 constant */
   if (info.absneg && info.size == VA_SIZE_16) {
      lut = bi_neg(va_lut_index_32(value ^ 0x80008000));
   if (!bi_is_null(lut))
                  /* Try using a single half of a FP16 constant */
   bool replicated_halves = (value & 0xFFFF) == (value >> 16);
   if (!staging && info.swizzle && info.size == VA_SIZE_16 &&
      replicated_halves) {
   bi_index lut = va_lut_index_16(value & 0xFFFF);
   if (!bi_is_null(lut))
            /* ...possibly negated */
   if (info.absneg) {
      lut = bi_neg(va_lut_index_16((value & 0xFFFF) ^ 0x8000));
   if (!bi_is_null(lut))
                  /* Try extending a byte */
   if (!staging && (info.widen || info.lanes || info.lane) &&
               bi_index lut = va_lut_index_8(value & 0xFF);
   if (!bi_is_null(lut))
               /* Try extending a halfword */
               bi_index lut = va_lut_index_16(value & 0xFFFF);
   if (!bi_is_null(lut))
               /* Try demoting the constant to FP16 */
   if (!staging && info.swizzle && info.size == VA_SIZE_32) {
      bi_index lut = va_demote_constant_fp16(value);
   if (!bi_is_null(lut))
            if (info.absneg) {
      bi_index lut = bi_neg(va_demote_constant_fp16(fui(-uif(value))));
   if (!bi_is_null(lut))
                  /* TODO: Optimize to uniform */
      }
      void
   va_lower_constants(bi_context *ctx, bi_instr *I)
   {
               bi_foreach_src(I, s) {
      if (I->src[s].type == BI_INDEX_CONSTANT) {
                     bool is_signed = valhall_opcodes[I->op].is_signed;
   bool staging = (s < valhall_opcodes[I->op].nr_staging_srcs);
   struct va_src_info info = va_src_info(I->op, s);
                  /* Resolve any swizzle, keeping in mind the different interpretations
   * swizzles in different contexts.
   */
   if (info.size == VA_SIZE_32) {
      /* Extracting a half from the 32-bit value */
   if (swz == BI_SWIZZLE_H00)
         else if (swz == BI_SWIZZLE_H11)
                        /* FP16 -> FP32 */
   if (info.swizzle && swz != BI_SWIZZLE_H01)
      } else if (info.size == VA_SIZE_16) {
      assert(swz >= BI_SWIZZLE_H00 && swz <= BI_SWIZZLE_H11);
      } else if (info.size == VA_SIZE_8 && (info.lane || info.lanes)) {
      /* 8-bit extract */
                     } else {
      /* TODO: Any other special handling? */
               bi_index cons =
                        /* If we're selecting a single 8-bit lane, we should return a single
   * 8-bit lane to ensure the result is encodeable. By convention,
   * applying the lane select puts the desired constant (at least) in the
   * bottom byte, so we can always select the bottom byte.
   */
   if (info.lane && I->src[s].swizzle == BI_SWIZZLE_H01) {
      assert(info.size == VA_SIZE_8);
               }
