   /**************************************************************************
   *
   * Copyright 2010-2018 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
         /**
   * @file
   * s3tc pixel format manipulation.
   *
   * @author Roland Scheidegger <sroland@vmware.com>
   */
         #include <llvm/Config/llvm-config.h>
      #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_string.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_debug.h"
      #include "lp_bld_arit.h"
   #include "lp_bld_type.h"
   #include "lp_bld_const.h"
   #include "lp_bld_conv.h"
   #include "lp_bld_gather.h"
   #include "lp_bld_format.h"
   #include "lp_bld_logic.h"
   #include "lp_bld_pack.h"
   #include "lp_bld_flow.h"
   #include "lp_bld_printf.h"
   #include "lp_bld_struct.h"
   #include "lp_bld_swizzle.h"
   #include "lp_bld_init.h"
   #include "lp_bld_debug.h"
   #include "lp_bld_intr.h"
         /**
   * Reverse an interleave2_half
   * (ie. pick every second element, independent lower/upper halfs)
   * sse2 can only do that with 32bit (shufps) or larger elements
   * natively. (Otherwise, and/pack (even) or shift/pack (odd)
   * could be used, ideally llvm would do that for us.)
   * XXX: Unfortunately, this does NOT translate to a shufps if those
   * are int vectors (and casting will not help, llvm needs to recognize it
   * as "real" float). Instead, llvm will use a pshufd/pshufd/punpcklqdq
   * sequence which I'm pretty sure is a lot worse despite domain transition
   * penalties with shufps (except maybe on Nehalem).
   */
   static LLVMValueRef
   lp_build_uninterleave2_half(struct gallivm_state *gallivm,
                           {
      LLVMValueRef shuffle, elems[LP_MAX_VECTOR_LENGTH];
            assert(type.length <= LP_MAX_VECTOR_LENGTH);
            if (type.length * type.width == 256) {
      assert(type.length == 8);
   assert(type.width == 32);
   static const unsigned shufvals[8] = {0, 2, 8, 10, 4, 6, 12, 14};
   for (i = 0; i < type.length; ++i) {
            } else {
      for (i = 0; i < type.length; ++i) {
                                    }
         /**
   * Build shuffle for extending vectors.
   */
   static LLVMValueRef
   lp_build_const_extend_shuffle(struct gallivm_state *gallivm,
         {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
            assert(n <= length);
                     for(i = 0; i < n; i++) {
         }
   for (i = n; i < length; i++) {
                     }
      static LLVMValueRef
   lp_build_const_unpackx2_shuffle(struct gallivm_state *gallivm, unsigned n)
   {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
                              for(i = 0, j = 0; i < n; i += 2, ++j) {
      elems[i + 0] = lp_build_const_int32(gallivm, 0 + j);
   elems[i + 1] = lp_build_const_int32(gallivm, n + j);
   elems[n + i + 0] = lp_build_const_int32(gallivm, 0 + n/2 + j);
                  }
      /*
   * broadcast 1 element to all elements
   */
   static LLVMValueRef
   lp_build_const_shuffle1(struct gallivm_state *gallivm,
         {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
                              for (i = 0; i < n; i++) {
                     }
      /*
   * move 1 element to pos 0, rest undef
   */
   static LLVMValueRef
   lp_build_shuffle1undef(struct gallivm_state *gallivm,
         {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH], shuf;
                              for (i = 1; i < n; i++) {
         }
               }
      static bool
   format_dxt1_variant(enum pipe_format format)
   {
   return format == PIPE_FORMAT_DXT1_RGB ||
            format == PIPE_FORMAT_DXT1_RGBA ||
         }
      /**
   * Gather elements from scatter positions in memory into vectors.
   * This is customised for fetching texels from s3tc textures.
   * For SSE, typical value is length=4.
   *
   * @param length length of the offsets
   * @param colors the stored colors of the blocks will be extracted into this.
   * @param codewords the codewords of the blocks will be extracted into this.
   * @param alpha_lo used for storing lower 32bit of alpha components for dxt3/5
   * @param alpha_hi used for storing higher 32bit of alpha components for dxt3/5
   * @param base_ptr base pointer, should be a i8 pointer type.
   * @param offsets vector with offsets
   */
   static void
   lp_build_gather_s3tc(struct gallivm_state *gallivm,
                        unsigned length,
   const struct util_format_description *format_desc,
   LLVMValueRef *colors,
   LLVMValueRef *codewords,
      {
      LLVMBuilderRef builder = gallivm->builder;
   unsigned block_bits = format_desc->block.bits;
   unsigned i;
   LLVMValueRef elems[8];
   LLVMTypeRef type32 = LLVMInt32TypeInContext(gallivm->context);
   LLVMTypeRef type64 = LLVMInt64TypeInContext(gallivm->context);
   LLVMTypeRef type32dxt;
            memset(&lp_type32dxt, 0, sizeof lp_type32dxt);
   lp_type32dxt.width = 32;
   lp_type32dxt.length = block_bits / 32;
            assert(block_bits == 64 || block_bits == 128);
            for (i = 0; i < length; ++i) {
      elems[i] = lp_build_gather_elem(gallivm, length,
                  }
   if (length == 1) {
      LLVMValueRef elem = elems[0];
   if (block_bits == 128) {
      *alpha_lo = LLVMBuildExtractElement(builder, elem,
         *alpha_hi = LLVMBuildExtractElement(builder, elem,
         *colors = LLVMBuildExtractElement(builder, elem,
         *codewords = LLVMBuildExtractElement(builder, elem,
      }
   else {
      *alpha_lo = LLVMGetUndef(type32);
   *alpha_hi = LLVMGetUndef(type32);
   *colors = LLVMBuildExtractElement(builder, elem,
         *codewords = LLVMBuildExtractElement(builder, elem,
         }
   else {
      LLVMValueRef tmp[4], cc01, cc23;
   struct lp_type lp_type32, lp_type64;
   memset(&lp_type32, 0, sizeof lp_type32);
   lp_type32.width = 32;
   lp_type32.length = length;
   memset(&lp_type64, 0, sizeof lp_type64);
   lp_type64.width = 64;
            if (block_bits == 128) {
      if (length == 8) {
      for (i = 0; i < 4; ++i) {
      tmp[0] = elems[i];
   tmp[1] = elems[i+4];
         }
   lp_build_transpose_aos(gallivm, lp_type32, elems, tmp);
   *colors = tmp[2];
   *codewords = tmp[3];
   *alpha_lo = tmp[0];
      } else {
                     for (i = 0; i < length; ++i) {
      /* no-op shuffle */
   elems[i] = LLVMBuildShuffleVector(builder, elems[i],
            }
   if (length == 8) {
      struct lp_type lp_type32_4 = {0};
   lp_type32_4.width = 32;
   lp_type32_4.length = 4;
   for (i = 0; i < 4; ++i) {
      tmp[0] = elems[i];
   tmp[1] = elems[i+4];
         }
   cc01 = lp_build_interleave2_half(gallivm, lp_type32, elems[0], elems[1], 0);
   cc23 = lp_build_interleave2_half(gallivm, lp_type32, elems[2], elems[3], 0);
   cc01 = LLVMBuildBitCast(builder, cc01, type64_vec, "");
   cc23 = LLVMBuildBitCast(builder, cc23, type64_vec, "");
   *colors = lp_build_interleave2_half(gallivm, lp_type64, cc01, cc23, 0);
   *codewords = lp_build_interleave2_half(gallivm, lp_type64, cc01, cc23, 1);
   *colors = LLVMBuildBitCast(builder, *colors, type32_vec, "");
            }
      /** Convert from <n x i32> containing 2 x n rgb565 colors
   * to 2 <n x i32> rgba8888 colors
   * This is the most optimized version I can think of
   * should be nearly as fast as decoding only one color
   * NOTE: alpha channel will be set to 0
   * @param colors  is a <n x i32> vector containing the rgb565 colors
   */
   static void
   color_expand2_565_to_8888(struct gallivm_state *gallivm,
                           {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef r, g, b, rblo, glo;
   LLVMValueRef rgblomask, rb, rgb0, rgb1;
                     memset(&type, 0, sizeof type);
   type.width = 32;
            memset(&type16, 0, sizeof type16);
   type16.width = 16;
            memset(&type8, 0, sizeof type8);
   type8.width = 8;
            rgblomask = lp_build_const_int_vec(gallivm, type16, 0x0707);
   colors = LLVMBuildBitCast(builder, colors,
         /* move r into low 8 bits, b into high 8 bits, g into another reg (low bits)
   * make sure low bits of r are zero - could use AND but requires constant */
   r = LLVMBuildLShr(builder, colors, lp_build_const_int_vec(gallivm, type16, 11), "");
   r = LLVMBuildShl(builder, r, lp_build_const_int_vec(gallivm, type16, 3), "");
   b = LLVMBuildShl(builder, colors, lp_build_const_int_vec(gallivm, type16, 11), "");
   rb = LLVMBuildOr(builder, r, b, "");
   rblo = LLVMBuildLShr(builder, rb, lp_build_const_int_vec(gallivm, type16, 5), "");
   /* don't have byte shift hence need mask */
   rblo = LLVMBuildAnd(builder, rblo, rgblomask, "");
            /* make sure low bits of g are zero */
   g = LLVMBuildAnd(builder, colors, lp_build_const_int_vec(gallivm, type16, 0x07e0), "");
   g = LLVMBuildLShr(builder, g, lp_build_const_int_vec(gallivm, type16, 3), "");
   glo = LLVMBuildLShr(builder, g, lp_build_const_int_vec(gallivm, type16, 6), "");
            rb = LLVMBuildBitCast(builder, rb, lp_build_vec_type(gallivm, type8), "");
   g = LLVMBuildBitCast(builder, g, lp_build_vec_type(gallivm, type8), "");
   rgb0 = lp_build_interleave2_half(gallivm, type8, rb, g, 0);
            rgb0 = LLVMBuildBitCast(builder, rgb0, lp_build_vec_type(gallivm, type), "");
            /* rgb0 is rgb00, rgb01, rgb10, rgb11
   * instead of rgb00, rgb10, rgb20, rgb30 hence need reshuffle
   * on x86 this _should_ just generate one shufps...
   */
   *color0 = lp_build_uninterleave2_half(gallivm, type, rgb0, rgb1, 0);
      }
         /** Convert from <n x i32> containing rgb565 colors
   * (in first 16 bits) to <n x i32> rgba8888 colors
   * bits 16-31 MBZ
   * NOTE: alpha channel will be set to 0
   * @param colors  is a <n x i32> vector containing the rgb565 colors
   */
   static LLVMValueRef
   color_expand_565_to_8888(struct gallivm_state *gallivm,
               {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef rgba, r, g, b, rgblo, glo;
   LLVMValueRef rbhimask, g6mask, rgblomask;
   struct lp_type type;
   memset(&type, 0, sizeof type);
   type.width = 32;
            /* color expansion:
   * first extract and shift colors into their final locations
   * (high bits - low bits zero at this point)
   * then replicate highest bits to the lowest bits
   * note rb replication can be done in parallel but not g
   * (different shift)
   * r5mask = 0xf800, g6mask = 0x07e0, b5mask = 0x001f
   * rhigh = 8, ghigh = 5, bhigh = 19
   * rblow = 5, glow = 6
   * rgblowmask = 0x00070307
   * r = colors >> rhigh
   * b = colors << bhigh
   * g = (colors & g6mask) << ghigh
   * rb = (r | b) rbhimask
   * rbtmp = rb >> rblow
   * gtmp = rb >> glow
   * rbtmp = rbtmp | gtmp
   * rbtmp = rbtmp & rgblowmask
   * rgb = rb | g | rbtmp
   */
   g6mask = lp_build_const_int_vec(gallivm, type, 0x07e0);
   rbhimask = lp_build_const_int_vec(gallivm, type, 0x00f800f8);
            r = LLVMBuildLShr(builder, colors, lp_build_const_int_vec(gallivm, type, 8), "");
   b = LLVMBuildShl(builder, colors, lp_build_const_int_vec(gallivm, type, 19), "");
   g = LLVMBuildAnd(builder, colors, g6mask, "");
   g = LLVMBuildShl(builder, g, lp_build_const_int_vec(gallivm, type, 5), "");
   rgba = LLVMBuildOr(builder, r, b, "");
   rgba = LLVMBuildAnd(builder, rgba, rbhimask, "");
   rgblo = LLVMBuildLShr(builder, rgba, lp_build_const_int_vec(gallivm, type, 5), "");
   glo = LLVMBuildLShr(builder, g, lp_build_const_int_vec(gallivm, type, 6), "");
   rgblo = LLVMBuildOr(builder, rgblo, glo, "");
   rgblo = LLVMBuildAnd(builder, rgblo, rgblomask, "");
   rgba = LLVMBuildOr(builder, rgba, g, "");
               }
         /*
   * Average two byte vectors. (Will always round up.)
   */
   static LLVMValueRef
   lp_build_pavgb(struct lp_build_context *bld8,
               {
      struct gallivm_state *gallivm = bld8->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   assert(bld8->type.width == 8);
   assert(bld8->type.length == 16 || bld8->type.length == 32);
   if (LLVM_VERSION_MAJOR < 6) {
      LLVMValueRef intrargs[2];
   char *intr_name = bld8->type.length == 32 ? "llvm.x86.avx2.pavg.b" :
         intrargs[0] = v0;
   intrargs[1] = v1;
   return lp_build_intrinsic(builder, intr_name,
      } else {
      /*
   * Must match llvm's autoupgrade of pavg.b intrinsic to be useful.
   * You better hope the backend code manages to detect the pattern, and
   * the pattern doesn't change there...
   */
   struct lp_type type_ext = bld8->type;
   LLVMTypeRef vec_type_ext;
   LLVMValueRef res;
   LLVMValueRef ext_one;
   type_ext.width = 16;
   vec_type_ext = lp_build_vec_type(gallivm, type_ext);
            v0 = LLVMBuildZExt(builder, v0, vec_type_ext, "");
   v1 = LLVMBuildZExt(builder, v1, vec_type_ext, "");
   res = LLVMBuildAdd(builder, v0, v1, "");
   res = LLVMBuildAdd(builder, res, ext_one, "");
   res = LLVMBuildLShr(builder, res, ext_one, "");
   res = LLVMBuildTrunc(builder, res, bld8->vec_type, "");
         }
      /**
   * Calculate 1/3(v1-v0) + v0
   * and 2*1/3(v1-v0) + v0
   */
   static void
   lp_build_lerp23(struct lp_build_context *bld,
                  LLVMValueRef v0,
      {
      struct gallivm_state *gallivm = bld->gallivm;
   LLVMValueRef x, x_lo, x_hi, delta_lo, delta_hi;
   LLVMValueRef mul_lo, mul_hi, v0_lo, v0_hi, v1_lo, v1_hi, tmp;
   const struct lp_type type = bld->type;
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_type i16_type = lp_wider_type(type);
            assert(lp_check_value(type, v0));
   assert(lp_check_value(type, v1));
            lp_build_context_init(&bld2, gallivm, i16_type);
   bld2.type.sign = true;
            /* FIXME: use native avx256 unpack/pack */
   lp_build_unpack2(gallivm, type, i16_type, x, &x_lo, &x_hi);
   lp_build_unpack2(gallivm, type, i16_type, v0, &v0_lo, &v0_hi);
   lp_build_unpack2(gallivm, type, i16_type, v1, &v1_lo, &v1_hi);
   delta_lo = lp_build_sub(&bld2, v1_lo, v0_lo);
            mul_lo = LLVMBuildMul(builder, x_lo, delta_lo, "");
            x_lo = LLVMBuildLShr(builder, mul_lo, lp_build_const_int_vec(gallivm, i16_type, 8), "");
   x_hi = LLVMBuildLShr(builder, mul_hi, lp_build_const_int_vec(gallivm, i16_type, 8), "");
   /* lerp optimization: pack now, do add afterwards */
   tmp = lp_build_pack2(gallivm, i16_type, type, x_lo, x_hi);
            x_lo = LLVMBuildLShr(builder, mul_lo, lp_build_const_int_vec(gallivm, i16_type, 7), "");
   x_hi = LLVMBuildLShr(builder, mul_hi, lp_build_const_int_vec(gallivm, i16_type, 7), "");
   /* unlike above still need mask (but add still afterwards). */
   x_lo = LLVMBuildAnd(builder, x_lo, lp_build_const_int_vec(gallivm, i16_type, 0xff), "");
   x_hi = LLVMBuildAnd(builder, x_hi, lp_build_const_int_vec(gallivm, i16_type, 0xff), "");
   tmp = lp_build_pack2(gallivm, i16_type, type, x_lo, x_hi);
      }
      /**
   * Convert from <n x i64> s3tc dxt1 to <4n x i8> RGBA AoS
   * @param colors  is a <n x i32> vector with n x 2x16bit colors
   * @param codewords  is a <n x i32> vector containing the codewords
   * @param i  is a <n x i32> vector with the x pixel coordinate (0 to 3)
   * @param j  is a <n x i32> vector with the y pixel coordinate (0 to 3)
   */
   static LLVMValueRef
   s3tc_dxt1_full_to_rgba_aos(struct gallivm_state *gallivm,
                              unsigned n,
      {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef color0, color1, color2, color3, color2_2, color3_2;
   LLVMValueRef rgba, a, colors0, colors1, col0, col1, const2;
   LLVMValueRef bit_pos, sel_mask, sel_lo, sel_hi, indices;
   struct lp_type type, type8;
   struct lp_build_context bld8, bld32;
            memset(&type, 0, sizeof type);
   type.width = 32;
            memset(&type8, 0, sizeof type8);
   type8.width = 8;
            assert(lp_check_value(type, i));
                     lp_build_context_init(&bld32, gallivm, type);
            /*
   * works as follows:
   * - expand color0/color1 to rgba8888
   * - calculate color2/3 (interpolation) according to color0 < color1 rules
   * - calculate color2/3 according to color0 >= color1 rules
   * - do selection of color2/3 according to comparison of color0/1
   * - extract indices (vector shift).
   * - use compare/select to select the correct color. Since we have 2bit
   *   indices (and 4 colors), needs at least three compare/selects.
   */
   /*
   * expand the two colors
   */
   col0 = LLVMBuildAnd(builder, colors, lp_build_const_int_vec(gallivm, type, 0x0000ffff), "");
   col1 = LLVMBuildLShr(builder, colors, lp_build_const_int_vec(gallivm, type, 16), "");
   if (n > 1) {
         }
   else {
      color0 = color_expand_565_to_8888(gallivm, n, col0);
               /*
   * interpolate colors
   * color2_1 is 2/3 color0 + 1/3 color1
   * color3_1 is 1/3 color0 + 2/3 color1
   * color2_2 is 1/2 color0 + 1/2 color1
   * color3_2 is 0
            colors0 = LLVMBuildBitCast(builder, color0, bld8.vec_type, "");
   colors1 = LLVMBuildBitCast(builder, color1, bld8.vec_type, "");
   /* can combine 2 lerps into one mostly - still looks expensive enough. */
   lp_build_lerp23(&bld8, colors0, colors1, &color2, &color3);
   color2 = LLVMBuildBitCast(builder, color2, bld32.vec_type, "");
            /* dxt3/5 always use 4-color encoding */
   if (is_dxt1_variant) {
      /* fix up alpha */
   if (format == PIPE_FORMAT_DXT1_RGBA ||
      format == PIPE_FORMAT_DXT1_SRGBA) {
   color0 = LLVMBuildOr(builder, color0, a, "");
   color1 = LLVMBuildOr(builder, color1, a, "");
      }
   /*
   * XXX with sse2 and 16x8 vectors, should use pavgb even when n == 1.
   * Much cheaper (but we don't care that much if n == 1).
   */
   if ((util_get_cpu_caps()->has_sse2 && n == 4) ||
      (util_get_cpu_caps()->has_avx2 && n == 8)) {
   color2_2 = lp_build_pavgb(&bld8, colors0, colors1);
      }
   else {
      struct lp_type i16_type = lp_wider_type(type8);
                                 /*
   * This isn't as expensive as it looks (the unpack is the same as
   * for lerp23), with correct rounding.
   * (Note that while rounding is correct, this will always round down,
   * whereas pavgb will always round up.)
   */
   /* FIXME: use native avx256 unpack/pack */
                  addlo = lp_build_add(&bld2, v0_lo, v1_lo);
   addhi = lp_build_add(&bld2, v0_hi, v1_hi);
   addlo = LLVMBuildLShr(builder, addlo,
         addhi = LLVMBuildLShr(builder, addhi,
         color2_2 = lp_build_pack2(gallivm, i16_type, type8, addlo, addhi);
      }
            /* select between colors2/3 */
   /* signed compare is faster saves some xors */
   type.sign = true;
   sel_mask = lp_build_compare(gallivm, type, PIPE_FUNC_GREATER, col0, col1);
   color2 = lp_build_select(&bld32, sel_mask, color2, color2_2);
   color3 = lp_build_select(&bld32, sel_mask, color3, color3_2);
            if (format == PIPE_FORMAT_DXT1_RGBA ||
      format == PIPE_FORMAT_DXT1_SRGBA) {
                  const2 = lp_build_const_int_vec(gallivm, type, 2);
   /* extract 2-bit index values */
   bit_pos = LLVMBuildShl(builder, j, const2, "");
   bit_pos = LLVMBuildAdd(builder, bit_pos, i, "");
   bit_pos = LLVMBuildAdd(builder, bit_pos, bit_pos, "");
   /*
   * NOTE: This innocent looking shift is very expensive with x86/ssex.
   * Shifts with per-elemnent shift count get roughly translated to
   * extract (count), extract (value), shift, move (back to xmm), unpack
   * per element!
   * So about 20 instructions here for 4xi32.
   * Newer llvm versions (3.7+) will not do extract/insert but use a
   * a couple constant count vector shifts plus shuffles. About same
   * amount of instructions unfortunately...
   * Would get much worse with 8xi16 even...
   * We could actually do better here:
   * - subtract bit_pos from 128+30, shl 23, convert float to int...
   * - now do mul with codewords followed by shr 30...
   * But requires 32bit->32bit mul, sse41 only (well that's emulatable
   * with 2 32bit->64bit muls...) and not exactly cheap
   * AVX2, of course, fixes this nonsense.
   */
            /* finally select the colors */
   sel_lo = LLVMBuildAnd(builder, indices, bld32.one, "");
   sel_lo = lp_build_compare(gallivm, type, PIPE_FUNC_EQUAL, sel_lo, bld32.one);
   color0 = lp_build_select(&bld32, sel_lo, color1, color0);
   color2 = lp_build_select(&bld32, sel_lo, color3, color2);
   sel_hi = LLVMBuildAnd(builder, indices, const2, "");
   sel_hi = lp_build_compare(gallivm, type, PIPE_FUNC_EQUAL, sel_hi, const2);
            /* fix up alpha */
   if (format == PIPE_FORMAT_DXT1_RGB ||
      format == PIPE_FORMAT_DXT1_SRGB) {
      }
      }
         static LLVMValueRef
   s3tc_dxt1_to_rgba_aos(struct gallivm_state *gallivm,
                        unsigned n,
   enum pipe_format format,
      {
      return s3tc_dxt1_full_to_rgba_aos(gallivm, n, format,
      }
         /**
   * Convert from <n x i128> s3tc dxt3 to <4n x i8> RGBA AoS
   * @param colors  is a <n x i32> vector with n x 2x16bit colors
   * @param codewords  is a <n x i32> vector containing the codewords
   * @param alphas  is a <n x i64> vector containing the alpha values
   * @param i  is a <n x i32> vector with the x pixel coordinate (0 to 3)
   * @param j  is a <n x i32> vector with the y pixel coordinate (0 to 3)
   */
   static LLVMValueRef
   s3tc_dxt3_to_rgba_aos(struct gallivm_state *gallivm,
                        unsigned n,
   enum pipe_format format,
   LLVMValueRef colors,
   LLVMValueRef codewords,
      {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef rgba, tmp, tmp2;
   LLVMValueRef bit_pos, sel_mask;
   struct lp_type type, type8;
            memset(&type, 0, sizeof type);
   type.width = 32;
            memset(&type8, 0, sizeof type8);
   type8.width = 8;
            assert(lp_check_value(type, i));
                     rgba = s3tc_dxt1_to_rgba_aos(gallivm, n, format,
                     /*
   * Extract alpha values. Since we now need to select from
   * which 32bit vector values are fetched, construct selection
   * mask from highest bit of bit_pos, and use select, then shift
   * according to the bit_pos (without the highest bit).
   * Note this is pointless for n == 1 case. Could just
   * directly use 64bit arithmetic if we'd extract 64bit
   * alpha value instead of 2x32...
   */
   /* pos = 4*(4j+i) */
   bit_pos = LLVMBuildShl(builder, j, lp_build_const_int_vec(gallivm, type, 2), "");
   bit_pos = LLVMBuildAdd(builder, bit_pos, i, "");
   bit_pos = LLVMBuildShl(builder, bit_pos,
         sel_mask = LLVMBuildLShr(builder, bit_pos,
         sel_mask = LLVMBuildSub(builder, sel_mask, bld.one, "");
   tmp = lp_build_select(&bld, sel_mask, alpha_low, alpha_hi);
   bit_pos = LLVMBuildAnd(builder, bit_pos,
         /* Warning: slow shift with per element count (without avx2) */
   /*
   * Could do pshufb here as well - just use appropriate 2 bits in bit_pos
   * to select the right byte with pshufb. Then for the remaining one bit
   * just do shift/select.
   */
            /* combined expand from a4 to a8 and shift into position */
   tmp = LLVMBuildShl(builder, tmp, lp_build_const_int_vec(gallivm, type, 28), "");
   tmp2 = LLVMBuildLShr(builder, tmp, lp_build_const_int_vec(gallivm, type, 4), "");
                        }
      static LLVMValueRef
   lp_build_lerpdxta(struct gallivm_state *gallivm,
                     LLVMValueRef alpha0,
   LLVMValueRef alpha1,
   {
      /*
   * note we're doing lerp in 16bit since 32bit pmulld is only available in sse41
   * (plus pmullw is actually faster...)
   * we just pretend our 32bit values (which are really only 8bit) are 16bits.
   * Note that this is obviously a disaster for the scalar case.
   */
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef delta, ainterp;
   LLVMValueRef weight5, weight7, weight;
   struct lp_type type32, type16, type8;
            memset(&type32, 0, sizeof type32);
   type32.width = 32;
   type32.length = n;
   memset(&type16, 0, sizeof type16);
   type16.width = 16;
   type16.length = 2*n;
   type16.sign = true;
   memset(&type8, 0, sizeof type8);
   type8.width = 8;
            lp_build_context_init(&bld16, gallivm, type16);
   /* 255/7 is a bit off - increase accuracy at the expense of shift later */
   sel_mask = LLVMBuildBitCast(builder, sel_mask, bld16.vec_type, "");
   weight5 = lp_build_const_int_vec(gallivm, type16, 255*64/5);
   weight7 = lp_build_const_int_vec(gallivm, type16, 255*64/7);
            alpha0 = LLVMBuildBitCast(builder, alpha0, bld16.vec_type, "");
   alpha1 = LLVMBuildBitCast(builder, alpha1, bld16.vec_type, "");
   code = LLVMBuildBitCast(builder, code, bld16.vec_type, "");
   /* we'll get garbage in the elements which had code 0 (or larger than 5 or 7)
                  weight = LLVMBuildMul(builder, weight, code, "");
   weight = LLVMBuildLShr(builder, weight,
                     ainterp = LLVMBuildMul(builder, delta, weight, "");
   ainterp = LLVMBuildLShr(builder, ainterp,
            ainterp = LLVMBuildBitCast(builder, ainterp, lp_build_vec_type(gallivm, type8), "");
   alpha0 = LLVMBuildBitCast(builder, alpha0, lp_build_vec_type(gallivm, type8), "");
   ainterp = LLVMBuildAdd(builder, alpha0, ainterp, "");
               }
      static LLVMValueRef
   s3tc_dxt5_alpha_channel(struct gallivm_state *gallivm,
                           {
      LLVMBuilderRef builder = gallivm->builder;
   struct lp_type type, type8;
   LLVMValueRef tmp, alpha0, alpha1, alphac, alphac0, bit_pos, shift;
   LLVMValueRef sel_mask, tmp_mask, alpha, alpha64, code_s;
   LLVMValueRef mask6, mask7, ainterp;
   LLVMTypeRef i64t = LLVMInt64TypeInContext(gallivm->context);
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
            memset(&type, 0, sizeof type);
   type.width = 32;
            memset(&type8, 0, sizeof type8);
   type8.width = 8;
   type8.length = n;
            lp_build_context_init(&bld32, gallivm, type);
   /* this looks pretty complex for vectorization:
   * extract a0/a1 values
   * extract code
   * select weights for interpolation depending on a0 > a1
   * mul weights by code - 1
   * lerp a0/a1/weights
   * use selects for getting either a0, a1, interp a, interp a/0.0, interp a/1.0
            alpha0 = LLVMBuildAnd(builder, alpha_lo,
         if (is_signed) {
      alpha0 = LLVMBuildTrunc(builder, alpha0, lp_build_vec_type(gallivm, type8), "");
               alpha1 = LLVMBuildLShr(builder, alpha_lo,
         alpha1 = LLVMBuildAnd(builder, alpha1,
         if (is_signed) {
      alpha1 = LLVMBuildTrunc(builder, alpha1, lp_build_vec_type(gallivm, type8), "");
               /* pos = 3*(4j+i) */
   bit_pos = LLVMBuildShl(builder, j, lp_build_const_int_vec(gallivm, type, 2), "");
   bit_pos = LLVMBuildAdd(builder, bit_pos, i, "");
   tmp = LLVMBuildAdd(builder, bit_pos, bit_pos, "");
   bit_pos = LLVMBuildAdd(builder, bit_pos, tmp, "");
   /* get rid of first 2 bytes - saves shifts of alpha_lo/hi */
   bit_pos = LLVMBuildAdd(builder, bit_pos,
            if (n == 1) {
      struct lp_type type64;
   memset(&type64, 0, sizeof type64);
   type64.width = 64;
   type64.length = 1;
   /* This is pretty pointless could avoid by just directly extracting
         alpha_lo = LLVMBuildZExt(builder, alpha_lo, i64t, "");
   alpha_hi = LLVMBuildZExt(builder, alpha_hi, i64t, "");
   alphac0 = LLVMBuildShl(builder, alpha_hi,
                  shift = LLVMBuildZExt(builder, bit_pos, i64t, "");
   alphac0 = LLVMBuildLShr(builder, alphac0, shift, "");
   alphac0 = LLVMBuildTrunc(builder, alphac0, i32t, "");
   alphac = LLVMBuildAnd(builder, alphac0,
      }
   else {
      /*
   * Using non-native vector length here (actually, with avx2 and
   * n == 4 llvm will indeed expand to ymm regs...)
   * At least newer llvm versions handle that ok.
   * llvm 3.7+ will even handle the emulated 64bit shift with variable
   * shift count without extraction (and it's actually easier to
   * emulate than the 32bit one).
   */
   alpha64 = LLVMBuildShuffleVector(builder, alpha_lo, alpha_hi,
            alpha64 = LLVMBuildBitCast(builder, alpha64, LLVMVectorType(i64t, n), "");
   shift = LLVMBuildZExt(builder, bit_pos, LLVMVectorType(i64t, n), "");
   alphac = LLVMBuildLShr(builder, alpha64, shift, "");
            alphac = LLVMBuildAnd(builder, alphac,
               /* signed compare is faster saves some xors */
   type.sign = true;
   /* alpha0 > alpha1 selection */
   sel_mask = lp_build_compare(gallivm, type, PIPE_FUNC_GREATER,
                  /*
   * if a0 > a1 then we select a0 for case 0, a1 for case 1, interp otherwise.
   * else we select a0 for case 0, a1 for case 1,
   * interp for case 2-5, 00 for 6 and 0xff(ffffff) for 7
   * a = (c == 0) ? a0 : a1
   * a = (c > 1) ? ainterp : a
   * Finally handle case 6/7 for !(a0 > a1)
   * a = (!(a0 > a1) && c == 6) ? 0 : a (andnot with mask)
   * a = (!(a0 > a1) && c == 7) ? 0xffffffff : a (or with mask)
   */
   tmp_mask = lp_build_compare(gallivm, type, PIPE_FUNC_EQUAL,
         alpha = lp_build_select(&bld32, tmp_mask, alpha0, alpha1);
   tmp_mask = lp_build_compare(gallivm, type, PIPE_FUNC_GREATER,
                  code_s = LLVMBuildAnd(builder, alphac,
         mask6 = lp_build_compare(gallivm, type, PIPE_FUNC_EQUAL,
         mask7 = lp_build_compare(gallivm, type, PIPE_FUNC_EQUAL,
         if (is_signed) {
      alpha = lp_build_select(&bld32, mask6, lp_build_const_int_vec(gallivm, type, -127), alpha);
      } else {
      alpha = LLVMBuildAnd(builder, alpha, LLVMBuildNot(builder, mask6, ""), "");
      }
   /* There can be garbage in upper bits, mask them off for rgtc formats */
               }
      /**
   * Convert from <n x i128> s3tc dxt5 to <4n x i8> RGBA AoS
   * @param colors  is a <n x i32> vector with n x 2x16bit colors
   * @param codewords  is a <n x i32> vector containing the codewords
   * @param alphas  is a <n x i64> vector containing the alpha values
   * @param i  is a <n x i32> vector with the x pixel coordinate (0 to 3)
   * @param j  is a <n x i32> vector with the y pixel coordinate (0 to 3)
   */
   static LLVMValueRef
   s3tc_dxt5_full_to_rgba_aos(struct gallivm_state *gallivm,
                              unsigned n,
   enum pipe_format format,
   LLVMValueRef colors,
      {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef rgba, alpha;
   struct lp_type type, type8;
            memset(&type, 0, sizeof type);
   type.width = 32;
            memset(&type8, 0, sizeof type8);
   type8.width = 8;
            assert(lp_check_value(type, i));
                     assert(lp_check_value(type, i));
            rgba = s3tc_dxt1_to_rgba_aos(gallivm, n, format,
                     alpha = s3tc_dxt5_alpha_channel(gallivm, false, n, alpha_hi, alpha_lo, i, j);
   alpha = LLVMBuildShl(builder, alpha, lp_build_const_int_vec(gallivm, type, 24), "");
               }
         static void
   lp_build_gather_s3tc_simple_scalar(struct gallivm_state *gallivm,
                     {
      LLVMBuilderRef builder = gallivm->builder;
   unsigned block_bits = format_desc->block.bits;
   LLVMValueRef elem, shuf;
   LLVMTypeRef type32 = LLVMIntTypeInContext(gallivm->context, 32);
   LLVMTypeRef src_type = LLVMIntTypeInContext(gallivm->context, block_bits);
                     ptr = LLVMBuildBitCast(builder, ptr, LLVMPointerType(src_type, 0), "");
            if (block_bits == 128) {
      /* just return block as is */
      }
   else {
      LLVMTypeRef type32_2 = LLVMVectorType(type32, 2);
   shuf = lp_build_const_extend_shuffle(gallivm, 2, 4);
   elem = LLVMBuildBitCast(builder, elem, type32_2, "");
   *dxt_block = LLVMBuildShuffleVector(builder, elem,
         }
         static void
   s3tc_store_cached_block(struct gallivm_state *gallivm,
                           {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef ptr, indices[3];
   LLVMTypeRef type_ptr4x32;
            type_ptr4x32 = LLVMPointerType(LLVMVectorType(LLVMInt32TypeInContext(gallivm->context), 4), 0);
   indices[0] = lp_build_const_int32(gallivm, 0);
   indices[1] = lp_build_const_int32(gallivm, LP_BUILD_FORMAT_CACHE_MEMBER_TAGS);
   indices[2] = hash_index;
   LLVMTypeRef cache_type = lp_build_format_cache_type(gallivm);
   ptr = LLVMBuildGEP2(builder, cache_type, cache, indices, ARRAY_SIZE(indices), "");
            indices[1] = lp_build_const_int32(gallivm, LP_BUILD_FORMAT_CACHE_MEMBER_DATA);
   hash_index = LLVMBuildMul(builder, hash_index, lp_build_const_int32(gallivm, 16), "");
   for (count = 0; count < 4; count++) {
      indices[2] = hash_index;
   ptr = LLVMBuildGEP2(builder, cache_type, cache, indices, ARRAY_SIZE(indices), "");
   ptr = LLVMBuildBitCast(builder, ptr, type_ptr4x32, "");
   LLVMBuildStore(builder, col[count], ptr);
         }
      static LLVMValueRef
   lookup_cache_member(struct gallivm_state *gallivm, LLVMValueRef cache, enum cache_member member, LLVMValueRef index) {
      assert(member == LP_BUILD_FORMAT_CACHE_MEMBER_DATA || member == LP_BUILD_FORMAT_CACHE_MEMBER_TAGS);
   LLVMBuilderRef builder = gallivm->builder;
            indices[0] = lp_build_const_int32(gallivm, 0);
   indices[1] = lp_build_const_int32(gallivm, member);
            const char *name =
                  member_ptr = LLVMBuildGEP2(builder, lp_build_format_cache_type(gallivm),
               }
      static LLVMValueRef
   s3tc_lookup_cached_pixel(struct gallivm_state *gallivm,
               {
         }
      static LLVMValueRef
   s3tc_lookup_tag_data(struct gallivm_state *gallivm,
               {
         }
      #if LP_BUILD_FORMAT_CACHE_DEBUG
   static void
   s3tc_update_cache_access(struct gallivm_state *gallivm,
                     {
      LLVMBuilderRef builder = gallivm->builder;
            assert(index == LP_BUILD_FORMAT_CACHE_MEMBER_ACCESS_TOTAL ||
         LLVMTypeRef cache_type = lp_build_format_cache_type(gallivm);
   member_ptr = lp_build_struct_get_ptr2(gallivm, cache_type, ptr, index, "");
   cache_access = LLVMBuildLoad2(builder, LLVMInt64TypeInContext(gallivm->context), member_ptr, "cache_access");
   cache_access = LLVMBuildAdd(builder, cache_access,
            }
   #endif
      /** 
   * Calculate 1/3(v1-v0) + v0 and 2*1/3(v1-v0) + v0.
   * The lerp is performed between the first 2 32bit colors
   * in the source vector, both results are returned packed in result vector.
   */
   static LLVMValueRef
   lp_build_lerp23_single(struct lp_build_context *bld,
         {
      struct gallivm_state *gallivm = bld->gallivm;
   LLVMValueRef x, mul, delta, res, v0, v1, elems[8];
   const struct lp_type type = bld->type;
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_type i16_type = lp_wider_type(type);
   struct lp_type i32_type = lp_wider_type(i16_type);
                     lp_build_context_init(&bld2, gallivm, i16_type);
            /* weights 256/3, 256*2/3, with correct rounding */
   elems[0] = elems[1] = elems[2] = elems[3] =
         elems[4] = elems[5] = elems[6] = elems[7] =
                  /*
   * v01 has col0 in 32bit elem 0, col1 in elem 1.
   * Interleave/unpack will give us separate v0/v1 vectors.
   */
   v01 = lp_build_interleave2(gallivm, i32_type, v01, v01, 0);
            lp_build_unpack2(gallivm, type, i16_type, v01, &v0, &v1);
                     mul = LLVMBuildLShr(builder, mul, lp_build_const_int_vec(gallivm, i16_type, 8), "");
   /* lerp optimization: pack now, do add afterwards */
   res = lp_build_pack2(gallivm, i16_type, type, mul, bld2.undef);
   /* only lower 2 elems are valid - for these v0 is really v0 */
      }
      /*
   * decode one dxt1 block.
   */
   static void
   s3tc_decode_block_dxt1(struct gallivm_state *gallivm,
                     {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef color01, color23, color01_16, color0123;
   LLVMValueRef rgba, tmp, a, sel_mask, indices, code, const2;
   struct lp_type type8, type32, type16, type64;
   struct lp_build_context bld8, bld32, bld16, bld64;
   unsigned i;
            memset(&type32, 0, sizeof type32);
   type32.width = 32;
   type32.length = 4;
            memset(&type8, 0, sizeof type8);
   type8.width = 8;
            memset(&type16, 0, sizeof type16);
   type16.width = 16;
            memset(&type64, 0, sizeof type64);
   type64.width = 64;
            a = lp_build_const_int_vec(gallivm, type32, 0xff000000);
            lp_build_context_init(&bld32, gallivm, type32);
   lp_build_context_init(&bld16, gallivm, type16);
   lp_build_context_init(&bld8, gallivm, type8);
            if (is_dxt1_variant) {
      color01 = lp_build_shuffle1undef(gallivm, dxt_block, 0, 4);
      } else {
      color01 = lp_build_shuffle1undef(gallivm, dxt_block, 2, 4);
      }
   code = LLVMBuildBitCast(builder, code, bld8.vec_type, "");
   /* expand bytes to dwords */
   code = lp_build_interleave2(gallivm, type8, code, code, 0);
               /*
   * works as follows:
   * - expand color0/color1 to rgba8888
   * - calculate color2/3 (interpolation) according to color0 < color1 rules
   * - calculate color2/3 according to color0 >= color1 rules
   * - do selection of color2/3 according to comparison of color0/1
   * - extract indices.
   * - use compare/select to select the correct color. Since we have 2bit
   *   indices (and 4 colors), needs at least three compare/selects.
            /*
   * expand the two colors
   */
   color01 = LLVMBuildBitCast(builder, color01, bld16.vec_type, "");
   color01 = lp_build_interleave2(gallivm, type16, color01,
         color01_16 = LLVMBuildBitCast(builder, color01, bld32.vec_type, "");
            /*
   * interpolate colors
   * color2_1 is 2/3 color0 + 1/3 color1
   * color3_1 is 1/3 color0 + 2/3 color1
   * color2_2 is 1/2 color0 + 1/2 color1
   * color3_2 is 0
            /* TODO: since this is now always scalar, should
   * probably just use control flow here instead of calculating
   * both cases and then selection
   */
   if (format == PIPE_FORMAT_DXT1_RGBA ||
      format == PIPE_FORMAT_DXT1_SRGBA) {
      }
   /* can combine 2 lerps into one mostly */
   color23 = lp_build_lerp23_single(&bld8, color01);
            /* dxt3/5 always use 4-color encoding */
   if (is_dxt1_variant) {
               if (util_get_cpu_caps()->has_sse2) {
      LLVMValueRef intrargs[2];
   intrargs[0] = LLVMBuildBitCast(builder, color01, bld8.vec_type, "");
   /* same interleave as for lerp23 - correct result in 2nd element */
   intrargs[1] = lp_build_interleave2(gallivm, type32, color01, color01, 0);
   intrargs[1] = LLVMBuildBitCast(builder, intrargs[1], bld8.vec_type, "");
      }
   else {
      LLVMValueRef v01, v0, v1, vhalf;
   /*
   * This isn't as expensive as it looks (the unpack is the same as
   * for lerp23, which is the reason why we do the pointless
   * interleave2 too), with correct rounding (the two lower elements
   * will be the same).
   */
   v01 = lp_build_interleave2(gallivm, type32, color01, color01, 0);
   v01 = LLVMBuildBitCast(builder, v01, bld8.vec_type, "");
   lp_build_unpack2(gallivm, type8, type16, v01, &v0, &v1);
   vhalf = lp_build_add(&bld16, v0, v1);
   vhalf = LLVMBuildLShr(builder, vhalf, bld16.one, "");
      }
   /* shuffle in color 3 as elem 2 zero, color 2 elem 1 */
   color23_2 = LLVMBuildBitCast(builder, color2_2, bld64.vec_type, "");
   color23_2 = LLVMBuildLShr(builder, color23_2,
                  tmp = LLVMBuildBitCast(builder, color01_16, bld64.vec_type, "");
   tmp = LLVMBuildLShr(builder, tmp,
         tmp = LLVMBuildBitCast(builder, tmp, bld32.vec_type, "");
   sel_mask = lp_build_compare(gallivm, type32, PIPE_FUNC_GREATER,
         sel_mask = lp_build_interleave2(gallivm, type32, sel_mask, sel_mask, 0);
               if (util_get_cpu_caps()->has_ssse3) {
      /*
   * Use pshufb as mini-lut. (Only doable with intrinsics as the
   * final shuffles are non-constant. pshufb is awesome!)
   */
   LLVMValueRef shuf[16], low2mask;
            color01 = LLVMBuildBitCast(builder, color01, bld64.vec_type, "");
   color23 = LLVMBuildBitCast(builder, color23, bld64.vec_type, "");
   color0123 = lp_build_interleave2(gallivm, type64, color01, color23, 0);
            if (format == PIPE_FORMAT_DXT1_RGB ||
      format == PIPE_FORMAT_DXT1_SRGB) {
               /* shuffle as r0r1r2r3g0g1... */
   for (i = 0; i < 4; i++) {
      shuf[4*i] = lp_build_const_int32(gallivm, 0 + i);
   shuf[4*i+1] = lp_build_const_int32(gallivm, 4 + i);
   shuf[4*i+2] = lp_build_const_int32(gallivm, 8 + i);
      }
   color0123 = LLVMBuildBitCast(builder, color0123, bld8.vec_type, "");
   color0123 = LLVMBuildShuffleVector(builder, color0123, bld8.undef,
            /* lowest 2 bits of each 8 bit value contain index into "LUT" */
   low2mask = lp_build_const_int_vec(gallivm, type8, 3);
   /* add 0/4/8/12 for r/g/b/a */
   lut_adj = lp_build_const_int_vec(gallivm, type32, 0x0c080400);
   lut_adj = LLVMBuildBitCast(builder, lut_adj, bld8.vec_type, "");
   intrargs[0] = color0123;
   for (i = 0; i < 4; i++) {
      lut_ind = LLVMBuildAnd(builder, code, low2mask, "");
   lut_ind = LLVMBuildOr(builder, lut_ind, lut_adj, "");
   intrargs[1] = lut_ind;
   col[i] = lp_build_intrinsic(builder, "llvm.x86.ssse3.pshuf.b.128",
         col[i] = LLVMBuildBitCast(builder, col[i], bld32.vec_type, "");
   code = LLVMBuildBitCast(builder, code, bld32.vec_type, "");
   code = LLVMBuildLShr(builder, code, const2, "");
         }
   else {
      /* Thanks to vectorization can do 4 texels in parallel */
   LLVMValueRef color0, color1, color2, color3;
   if (format == PIPE_FORMAT_DXT1_RGB ||
      format == PIPE_FORMAT_DXT1_SRGB) {
   color01 = LLVMBuildOr(builder, color01, a, "");
      }
   color0 = LLVMBuildShuffleVector(builder, color01, bld32.undef,
         color1 = LLVMBuildShuffleVector(builder, color01, bld32.undef,
         color2 = LLVMBuildShuffleVector(builder, color23, bld32.undef,
         color3 = LLVMBuildShuffleVector(builder, color23, bld32.undef,
                  for (i = 0; i < 4; i++) {
      /* select the colors */
   LLVMValueRef selmasklo, rgba01, rgba23, bitlo;
   bitlo = bld32.one;
   indices = LLVMBuildAnd(builder, code, bitlo, "");
   selmasklo = lp_build_compare(gallivm, type32, PIPE_FUNC_EQUAL,
                  LLVMValueRef selmaskhi;
   indices = LLVMBuildAnd(builder, code, const2, "");
   selmaskhi = lp_build_compare(gallivm, type32, PIPE_FUNC_EQUAL,
                        /*
   * Note that this will give "wrong" order.
   * col0 will be rgba0, rgba4, rgba8, rgba12, col1 rgba1, rgba5, ...
   * This would be easily fixable by using different shuffle, bitlo/hi
   * vectors above (and different shift), but seems slightly easier to
   * deal with for dxt3/dxt5 alpha too. So instead change lookup.
   */
   col[i] = rgba;
            }
      /*
   * decode one dxt3 block.
   */
   static void
   s3tc_decode_block_dxt3(struct gallivm_state *gallivm,
                     {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef alpha, alphas0, alphas1, shift4_16, a[4], mask8hi;
   struct lp_type type32, type8, type16;
            memset(&type32, 0, sizeof type32);
   type32.width = 32;
            memset(&type8, 0, sizeof type8);
   type8.width = 8;
            memset(&type16, 0, sizeof type16);
   type16.width = 16;
                     shift4_16 = lp_build_const_int_vec(gallivm, type16, 4);
            alpha = LLVMBuildBitCast(builder, dxt_block,
         alpha = lp_build_interleave2(gallivm, type8, alpha, alpha, 0);
   alpha = LLVMBuildBitCast(builder, alpha,
         alpha = LLVMBuildAnd(builder, alpha,
         alphas0 = LLVMBuildLShr(builder, alpha, shift4_16, "");
   alphas1 = LLVMBuildShl(builder, alpha, shift4_16, "");
   alpha = LLVMBuildOr(builder, alphas0, alpha, "");
   alpha = LLVMBuildOr(builder, alphas1, alpha, "");
   alpha = LLVMBuildBitCast(builder, alpha,
         /*
   * alpha now contains elems 0,1,2,3,... (ubytes)
   * we need 0,4,8,12, 1,5,9,13 etc. in dwords to match color (which
   * is just as easy as "natural" order - 3 shift/and instead of 6 unpack).
   */
   a[0] = LLVMBuildShl(builder, alpha,
         a[1] = LLVMBuildShl(builder, alpha,
         a[1] = LLVMBuildAnd(builder, a[1], mask8hi, "");
   a[2] = LLVMBuildShl(builder, alpha,
         a[2] = LLVMBuildAnd(builder, a[2], mask8hi, "");
            for (i = 0; i < 4; i++) {
            }
         static LLVMValueRef
   lp_build_lerpdxta_block(struct gallivm_state *gallivm,
                           {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef delta, ainterp;
   LLVMValueRef weight5, weight7, weight;
   struct lp_type type16;
            memset(&type16, 0, sizeof type16);
   type16.width = 16;
   type16.length = 8;
            lp_build_context_init(&bld, gallivm, type16);
   /*
   * 256/7 is only 36.57 so we'd lose quite some precision. Since it would
   * actually be desirable to do this here with even higher accuracy than
   * even 8 bit (more or less required for rgtc, albeit that's not handled
   * here right now), shift the weights after multiplication by code.
   */
   weight5 = lp_build_const_int_vec(gallivm, type16, 256*64/5);
   weight7 = lp_build_const_int_vec(gallivm, type16, 256*64/7);
            /*
   * we'll get garbage in the elements which had code 0 (or larger than
   * 5 or 7) but we don't care (or rather, need to fix up anyway).
   */
            weight = LLVMBuildMul(builder, weight, code, "");
   weight = LLVMBuildLShr(builder, weight,
                     ainterp = LLVMBuildMul(builder, delta, weight, "");
   ainterp = LLVMBuildLShr(builder, ainterp,
                        }
         /*
   * decode one dxt5 block.
   */
   static void
   s3tc_decode_block_dxt5(struct gallivm_state *gallivm,
                     {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef alpha, alpha0, alpha1, ares;
   LLVMValueRef ainterp, ainterp0, ainterp1, shuffle1, sel_mask, sel_mask2;
   LLVMValueRef a[4], acode, tmp0, tmp1;
   LLVMTypeRef i64t, i32t;
   struct lp_type type32, type64, type8, type16;
   struct lp_build_context bld16, bld8;
            memset(&type32, 0, sizeof type32);
   type32.width = 32;
            memset(&type64, 0, sizeof type64);
   type64.width = 64;
            memset(&type8, 0, sizeof type8);
   type8.width = 8;
            memset(&type16, 0, sizeof type16);
   type16.width = 16;
            lp_build_context_init(&bld16, gallivm, type16);
            i64t = lp_build_vec_type(gallivm, type64);
                     /*
   * three possible strategies for vectorizing alpha:
   * 1) compute all 8 values then use scalar extraction
   *    (i.e. have all 8 alpha values packed in one 64bit scalar
   *    and do something like ax = vals >> (codex * 8) followed
   *    by inserting these values back into color)
   * 2) same as 8 but just use pshufb as a mini-LUT for selection.
   *    (without pshufb would need boatloads of cmp/selects trying to
   *    keep things vectorized for essentially scalar selection).
   * 3) do something similar to the uncached case
   *    needs more calculations (need to calc 16 values instead of 8 though
   *    that's only an issue for the lerp which we need to do twice otherwise
   *    everything still fits into 128bit) but keeps things vectorized mostly.
   * Trying 3) here though not sure it's really faster...
   * With pshufb, we try 2) (cheaper and more accurate)
            /*
   * Ideally, we'd use 2 variable 16bit shifts here (byte shifts wouldn't
   * help since code crosses 8bit boundaries). But variable shifts are
   * AVX2 only, and even then only dword/quadword (intel _really_ hates
   * shifts!). Instead, emulate by 16bit muls.
   * Also, the required byte shuffles are essentially non-emulatable, so
   * require ssse3 (albeit other archs might do them fine).
   * This is not directly tied to ssse3 - just need sane byte shuffles.
   * But ordering is going to be different below so use same condition.
               /* vectorize alpha */
   alpha = LLVMBuildBitCast(builder, dxt_block, i64t, "");
   alpha0 = LLVMBuildAnd(builder, alpha,
         alpha0 = LLVMBuildBitCast(builder, alpha0, bld16.vec_type, "");
   alpha = LLVMBuildBitCast(builder, alpha, bld16.vec_type, "");
   alpha1 = LLVMBuildLShr(builder, alpha,
         alpha = LLVMBuildBitCast(builder, alpha,  i64t, "");
   shuffle1 = lp_build_const_shuffle1(gallivm, 0, 8);
   alpha0 = LLVMBuildShuffleVector(builder, alpha0, alpha0, shuffle1, "");
            type16.sign = true;
   sel_mask = lp_build_compare(gallivm, type16, PIPE_FUNC_GREATER,
         type16.sign = false;
            if (!util_get_cpu_caps()->has_ssse3) {
               /* extraction of the 3 bit values into something more useful is HARD */
   /* first steps are actually scalar */
   acode = LLVMBuildLShr(builder, alpha,
         tmp0 = LLVMBuildAnd(builder, acode,
         tmp1 =  LLVMBuildLShr(builder, acode,
         tmp0 = LLVMBuildBitCast(builder, tmp0, i32t, "");
   tmp1 = LLVMBuildBitCast(builder, tmp1, i32t, "");
   acode = lp_build_interleave2(gallivm, type32, tmp0, tmp1, 0);
   /* now have 2x24bit in 4x32bit, order 01234567, 89..., undef, undef */
   tmp0 = LLVMBuildAnd(builder, acode,
         tmp1 =  LLVMBuildLShr(builder, acode,
         acode = lp_build_interleave2(gallivm, type32, tmp0, tmp1, 0);
   /* now have 4x12bit in 4x32bit, order 0123, 4567, ,,, */
   tmp0 = LLVMBuildAnd(builder, acode,
         tmp1 =  LLVMBuildLShr(builder, acode,
         /* use signed pack doesn't matter and otherwise need sse41 */
   type32.sign = type16.sign = true;
   acode = lp_build_pack2(gallivm, type32, type16, tmp0, tmp1);
   type32.sign = type16.sign = false;
   /* now have 8x6bit in 8x16bit, 01, 45, 89, ..., 23, 67, ... */
   acode0 = LLVMBuildAnd(builder, acode,
         acode1 =  LLVMBuildLShr(builder, acode,
         acode = lp_build_pack2(gallivm, type16, type8, acode0, acode1);
            acodeg = LLVMBuildAnd(builder, acode,
         mask1 = lp_build_compare(gallivm, type8, PIPE_FUNC_EQUAL,
            sel_mask = LLVMBuildBitCast(builder, sel_mask, bld16.vec_type, "");
   ainterp0 = lp_build_lerpdxta_block(gallivm, alpha0, alpha1, acode0, sel_mask);
   ainterp1 = lp_build_lerpdxta_block(gallivm, alpha0, alpha1, acode1, sel_mask);
   sel_mask = LLVMBuildBitCast(builder, sel_mask, bld8.vec_type, "");
   ainterp = lp_build_pack2(gallivm, type16, type8, ainterp0, ainterp1);
   alpha0 = lp_build_pack2(gallivm, type16, type8, alpha0, alpha0);
   alpha1 = lp_build_pack2(gallivm, type16, type8, alpha1, alpha1);
   ainterp = LLVMBuildAdd(builder, ainterp, alpha0, "");
   /* Fix up val01 */
   sel_mask2 = lp_build_compare(gallivm, type8, PIPE_FUNC_EQUAL,
         ainterp = lp_build_select(&bld8, sel_mask2, alpha0, ainterp);
            /* fix up val67 if a0 <= a1 */
   sel_mask2 = lp_build_compare(gallivm, type8, PIPE_FUNC_EQUAL,
         ares = LLVMBuildAnd(builder, ainterp, LLVMBuildNot(builder, sel_mask2, ""), "");
   sel_mask2 = lp_build_compare(gallivm, type8, PIPE_FUNC_EQUAL,
                  /* unpack in right order (0,4,8,12,1,5,..) */
   /* this gives us zero, a0, zero, a4, zero, a8, ... for tmp0 */
   tmp0 = lp_build_interleave2(gallivm, type8, bld8.zero, ares, 0);
   tmp1 = lp_build_interleave2(gallivm, type8, bld8.zero, ares, 1);
   tmp0 = LLVMBuildBitCast(builder, tmp0, bld16.vec_type, "");
            a[0] = lp_build_interleave2(gallivm, type16, bld16.zero, tmp0, 0);
   a[1] = lp_build_interleave2(gallivm, type16, bld16.zero, tmp1, 0);
   a[2] = lp_build_interleave2(gallivm, type16, bld16.zero, tmp0, 1);
      }
   else {
      LLVMValueRef elems[16], intrargs[2], shufa, mulclo, mulchi, mask8hi;
   LLVMTypeRef type16s = LLVMInt16TypeInContext(gallivm->context);
   LLVMTypeRef type8s = LLVMInt8TypeInContext(gallivm->context);
   unsigned i, j;
   /*
   * Ideally, we'd use 2 variable 16bit shifts here (byte shifts wouldn't
   * help since code crosses 8bit boundaries). But variable shifts are
   * AVX2 only, and even then only dword/quadword (intel _really_ hates
   * shifts!). Instead, emulate by 16bit muls.
   * Also, the required byte shuffles are essentially non-emulatable, so
   * require ssse3 (albeit other archs might do them fine, but the
   * complete path is ssse3 only for now).
   */
   for (i = 0, j = 0; i < 16; i += 8, j += 3) {
      elems[i+0] = elems[i+1] = elems[i+2] = lp_build_const_int32(gallivm, j+2);
   elems[i+3] = elems[i+4] = lp_build_const_int32(gallivm, j+3);
      }
   shufa = LLVMConstVector(elems, 16);
   alpha = LLVMBuildBitCast(builder, alpha, bld8.vec_type, "");
   acode = LLVMBuildShuffleVector(builder, alpha, bld8.undef, shufa, "");
   acode = LLVMBuildBitCast(builder, acode, bld16.vec_type, "");
   /*
   * Put 0/2/4/6 into high 3 bits of 16 bits (save AND mask)
   * Do the same for 1/3/5/7 (albeit still need mask there - ideally
   * we'd place them into bits 4-7 so could save shift but impossible.)
   */
   for (i = 0; i < 8; i += 4) {
      elems[i+0] = LLVMConstInt(type16s, 1 << (13-0), 0);
   elems[i+1] = LLVMConstInt(type16s, 1 << (13-6), 0);
   elems[i+2] = LLVMConstInt(type16s, 1 << (13-4), 0);
      }
   mulclo = LLVMConstVector(elems, 8);
   for (i = 0; i < 8; i += 4) {
      elems[i+0] = LLVMConstInt(type16s, 1 << (13-3), 0);
   elems[i+1] = LLVMConstInt(type16s, 1 << (13-9), 0);
   elems[i+2] = LLVMConstInt(type16s, 1 << (13-7), 0);
      }
            tmp0 = LLVMBuildMul(builder, acode, mulclo, "");
   tmp1 = LLVMBuildMul(builder, acode, mulchi, "");
   tmp0 = LLVMBuildLShr(builder, tmp0,
         tmp1 = LLVMBuildLShr(builder, tmp1,
         tmp1 = LLVMBuildAnd(builder, tmp1,
         acode = LLVMBuildOr(builder, tmp0, tmp1, "");
            /*
   * Note that ordering is different here to non-ssse3 path:
   * 0/1/2/3/4/5...
            LLVMValueRef weight0, weight1, weight, delta;
   LLVMValueRef constff_elem7, const0_elem6;
   /* weights, correctly rounded (round(256*x/7)) */
   elems[0] = LLVMConstInt(type16s, 256, 0);
   elems[1] = LLVMConstInt(type16s, 0, 0);
   elems[2] = LLVMConstInt(type16s, 219, 0);
   elems[3] =  LLVMConstInt(type16s, 183, 0);
   elems[4] =  LLVMConstInt(type16s, 146, 0);
   elems[5] =  LLVMConstInt(type16s, 110, 0);
   elems[6] =  LLVMConstInt(type16s, 73, 0);
   elems[7] =  LLVMConstInt(type16s, 37, 0);
            elems[0] = LLVMConstInt(type16s, 256, 0);
   elems[1] = LLVMConstInt(type16s, 0, 0);
   elems[2] = LLVMConstInt(type16s, 205, 0);
   elems[3] =  LLVMConstInt(type16s, 154, 0);
   elems[4] =  LLVMConstInt(type16s, 102, 0);
   elems[5] =  LLVMConstInt(type16s, 51, 0);
   elems[6] =  LLVMConstInt(type16s, 0, 0);
   elems[7] =  LLVMConstInt(type16s, 0, 0);
            weight0 = LLVMBuildBitCast(builder, weight0, bld8.vec_type, "");
   weight1 = LLVMBuildBitCast(builder, weight1, bld8.vec_type, "");
   weight = lp_build_select(&bld8, sel_mask, weight0, weight1);
            for (i = 0; i < 16; i++) {
         }
   elems[7] = LLVMConstInt(type8s, 255, 0);
            for (i = 0; i < 16; i++) {
         }
   elems[6] = LLVMConstInt(type8s, 0, 0);
            /* standard simple lerp - but the version we need isn't available */
   delta = LLVMBuildSub(builder, alpha0, alpha1, "");
   ainterp = LLVMBuildMul(builder, delta, weight, "");
   ainterp = LLVMBuildLShr(builder, ainterp,
         ainterp = LLVMBuildBitCast(builder, ainterp, bld8.vec_type, "");
   alpha1 = LLVMBuildBitCast(builder, alpha1, bld8.vec_type, "");
   ainterp = LLVMBuildAdd(builder, ainterp, alpha1, "");
   ainterp = LLVMBuildBitCast(builder, ainterp, bld16.vec_type, "");
            /* fixing 0/0xff case is slightly more complex */
   constff_elem7 = LLVMBuildAnd(builder, constff_elem7,
         const0_elem6 = LLVMBuildOr(builder, const0_elem6, sel_mask, "");
   ainterp = LLVMBuildOr(builder, ainterp, constff_elem7, "");
            /* now pick all 16 elements at once! */
   intrargs[0] = ainterp;
   intrargs[1] = acode;
   ares = lp_build_intrinsic(builder, "llvm.x86.ssse3.pshuf.b.128",
            ares = LLVMBuildBitCast(builder, ares, i32t, "");
   mask8hi = lp_build_const_int_vec(gallivm, type32, 0xff000000);
   a[0] = LLVMBuildShl(builder, ares,
         a[1] = LLVMBuildShl(builder, ares,
         a[1] = LLVMBuildAnd(builder, a[1], mask8hi, "");
   a[2] = LLVMBuildShl(builder, ares,
         a[2] = LLVMBuildAnd(builder, a[2], mask8hi, "");
               for (i = 0; i < 4; i++) {
      a[i] = LLVMBuildBitCast(builder, a[i], i32t, "");
         }
         static void
   generate_update_cache_one_block(struct gallivm_state *gallivm,
               {
      LLVMBasicBlockRef block;
   LLVMBuilderRef old_builder;
   LLVMValueRef ptr_addr;
   LLVMValueRef hash_index;
   LLVMValueRef cache;
   LLVMValueRef dxt_block, tag_value;
            ptr_addr     = LLVMGetParam(function, 0);
   hash_index   = LLVMGetParam(function, 1);
            lp_build_name(ptr_addr,   "ptr_addr"  );
   lp_build_name(hash_index, "hash_index");
            /*
   * Function body
            old_builder = gallivm->builder;
   block = LLVMAppendBasicBlockInContext(gallivm->context, function, "entry");
   gallivm->builder = LLVMCreateBuilderInContext(gallivm->context);
            lp_build_gather_s3tc_simple_scalar(gallivm, format_desc, &dxt_block,
            switch (format_desc->format) {
   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT1_SRGB:
   case PIPE_FORMAT_DXT1_SRGBA:
      s3tc_decode_block_dxt1(gallivm, format_desc->format, dxt_block, col);
      case PIPE_FORMAT_DXT3_RGBA:
   case PIPE_FORMAT_DXT3_SRGBA:
      s3tc_decode_block_dxt3(gallivm, format_desc->format, dxt_block, col);
      case PIPE_FORMAT_DXT5_RGBA:
   case PIPE_FORMAT_DXT5_SRGBA:
      s3tc_decode_block_dxt5(gallivm, format_desc->format, dxt_block, col);
      default:
      assert(0);
   s3tc_decode_block_dxt1(gallivm, format_desc->format, dxt_block, col);
               tag_value = LLVMBuildPtrToInt(gallivm->builder, ptr_addr,
                           LLVMDisposeBuilder(gallivm->builder);
               }
         static void
   update_cached_block(struct gallivm_state *gallivm,
                     const struct util_format_description *format_desc,
      {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMModuleRef module = gallivm->module;
   char name[256];
   LLVMTypeRef i8t = LLVMInt8TypeInContext(gallivm->context);
   LLVMTypeRef pi8t = LLVMPointerType(i8t, 0);
   LLVMValueRef function, inst;
   LLVMBasicBlockRef bb;
            snprintf(name, sizeof name, "%s_update_cache_one_block",
                  LLVMTypeRef ret_type = LLVMVoidTypeInContext(gallivm->context);
   LLVMTypeRef arg_types[3];
   arg_types[0] = pi8t;
   arg_types[1] = LLVMInt32TypeInContext(gallivm->context);
   arg_types[2] = LLVMTypeOf(cache); // XXX: put right type here
            if (!function) {
               for (unsigned arg = 0; arg < ARRAY_SIZE(arg_types); ++arg)
                  LLVMSetFunctionCallConv(function, LLVMFastCallConv);
   LLVMSetVisibility(function, LLVMHiddenVisibility);
               args[0] = ptr_addr;
   args[1] = hash_index;
            LLVMBuildCall2(builder, function_type, function, args, ARRAY_SIZE(args), "");
   bb = LLVMGetInsertBlock(builder);
   inst = LLVMGetLastInstruction(bb);
      }
      /*
   * cached lookup
   */
   static LLVMValueRef
   compressed_fetch_cached(struct gallivm_state *gallivm,
                           const struct util_format_description *format_desc,
   unsigned n,
   LLVMValueRef base_ptr,
      {
      LLVMBuilderRef builder = gallivm->builder;
   unsigned count, low_bit, log2size;
   LLVMValueRef color, offset_stored, addr, ptr_addrtrunc, tmp;
   LLVMValueRef ij_index, hash_index, hash_mask, block_index;
   LLVMTypeRef i8t = LLVMInt8TypeInContext(gallivm->context);
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMTypeRef i64t = LLVMInt64TypeInContext(gallivm->context);
   struct lp_type type;
   struct lp_build_context bld32;
   memset(&type, 0, sizeof type);
   type.width = 32;
                     /*
   * compute hash - we use direct mapped cache, the hash function could
   *                be better but it needs to be simple
   * per-element:
   *    compare offset with offset stored at tag (hash)
   *    if not equal extract block, store block, update tag
   *    extract color from cache
   *    assemble colors
            low_bit = util_logbase2(format_desc->block.bits / 8);
   log2size = util_logbase2(LP_BUILD_FORMAT_CACHE_SIZE);
   addr = LLVMBuildPtrToInt(builder, base_ptr, i64t, "");
   ptr_addrtrunc = LLVMBuildPtrToInt(builder, base_ptr, i32t, "");
   ptr_addrtrunc = lp_build_broadcast_scalar(&bld32, ptr_addrtrunc);
   /* For the hash function, first mask off the unused lowest bits. Then just
         ptr_addrtrunc = LLVMBuildAdd(builder, offset, ptr_addrtrunc, "");
   ptr_addrtrunc = LLVMBuildLShr(builder, ptr_addrtrunc,
         /* This only really makes sense for size 64,128,256 */
   hash_index = ptr_addrtrunc;
   ptr_addrtrunc = LLVMBuildLShr(builder, ptr_addrtrunc,
         hash_index = LLVMBuildXor(builder, ptr_addrtrunc, hash_index, "");
   tmp = LLVMBuildLShr(builder, hash_index,
                  hash_mask = lp_build_const_int_vec(gallivm, type, LP_BUILD_FORMAT_CACHE_SIZE - 1);
   hash_index = LLVMBuildAnd(builder, hash_index, hash_mask, "");
   ij_index = LLVMBuildShl(builder, i, lp_build_const_int_vec(gallivm, type, 2), "");
   ij_index = LLVMBuildAdd(builder, ij_index, j, "");
   block_index = LLVMBuildShl(builder, hash_index,
                  if (n > 1) {
      color = bld32.undef;
   for (count = 0; count < n; count++) {
      LLVMValueRef index, cond, colorx;
                  index = lp_build_const_int32(gallivm, count);
   offsetx = LLVMBuildExtractElement(builder, offset, index, "");
   addrx = LLVMBuildZExt(builder, offsetx, i64t, "");
   addrx = LLVMBuildAdd(builder, addrx, addr, "");
   block_indexx = LLVMBuildExtractElement(builder, block_index, index, "");
   hash_indexx = LLVMBuildLShr(builder, block_indexx,
                        lp_build_if(&if_ctx, gallivm, cond);
   {
      ptr_addrx = LLVMBuildIntToPtr(builder, addrx,
   #if LP_BUILD_FORMAT_CACHE_DEBUG
               #endif
                                    color = LLVMBuildInsertElement(builder, color, colorx,
         }
   else {
      LLVMValueRef cond;
            tmp = LLVMBuildZExt(builder, offset, i64t, "");
   addr = LLVMBuildAdd(builder, tmp, addr, "");
   offset_stored = s3tc_lookup_tag_data(gallivm, cache, hash_index);
            lp_build_if(&if_ctx, gallivm, cond);
   {
         #if LP_BUILD_FORMAT_CACHE_DEBUG
               #endif
         }
                  #if LP_BUILD_FORMAT_CACHE_DEBUG
      s3tc_update_cache_access(gallivm, cache, n,
      #endif
         }
         static LLVMValueRef
   s3tc_dxt5_to_rgba_aos(struct gallivm_state *gallivm,
                        unsigned n,
   enum pipe_format format,
   LLVMValueRef colors,
   LLVMValueRef codewords,
      {
      return s3tc_dxt5_full_to_rgba_aos(gallivm, n, format, colors,
      }
         /**
   * @param n  number of pixels processed (usually n=4, but it should also work with n=1
   *           and multiples of 4)
   * @param base_ptr  base pointer (32bit or 64bit pointer depending on the architecture)
   * @param offset <n x i32> vector with the relative offsets of the S3TC blocks
   * @param i  is a <n x i32> vector with the x subpixel coordinate (0..3)
   * @param j  is a <n x i32> vector with the y subpixel coordinate (0..3)
   * @return  a <4*n x i8> vector with the pixel RGBA values in AoS
   */
   LLVMValueRef
   lp_build_fetch_s3tc_rgba_aos(struct gallivm_state *gallivm,
                              const struct util_format_description *format_desc,
   unsigned n,
      {
      LLVMValueRef rgba;
   LLVMTypeRef i8t = LLVMInt8TypeInContext(gallivm->context);
            assert(format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC);
   assert(format_desc->block.width == 4);
                  /*   debug_printf("format = %d\n", format_desc->format);*/
      if (cache) {
      rgba = compressed_fetch_cached(gallivm, format_desc, n,
                     /*
   * Could use n > 8 here with avx2, but doesn't seem faster.
   */
   if (n > 4) {
      unsigned count;
   LLVMTypeRef i8_vectype = LLVMVectorType(i8t, 4 * n);
   LLVMTypeRef i128_type = LLVMIntTypeInContext(gallivm->context, 128);
   LLVMTypeRef i128_vectype =  LLVMVectorType(i128_type, n / 4);
   LLVMTypeRef i324_vectype = LLVMVectorType(LLVMInt32TypeInContext(
         LLVMValueRef offset4, i4, j4, rgba4[LP_MAX_VECTOR_LENGTH/16];
                              for (count = 0; count < n / 4; count++) {
               i4 = lp_build_extract_range(gallivm, i, count * 4, 4);
                                 switch (format_desc->format) {
   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT1_SRGB:
   case PIPE_FORMAT_DXT1_SRGBA:
      rgba4[count] = s3tc_dxt1_to_rgba_aos(gallivm, 4, format_desc->format,
            case PIPE_FORMAT_DXT3_RGBA:
   case PIPE_FORMAT_DXT3_SRGBA:
      rgba4[count] = s3tc_dxt3_to_rgba_aos(gallivm, 4, format_desc->format, colors,
            case PIPE_FORMAT_DXT5_RGBA:
   case PIPE_FORMAT_DXT5_SRGBA:
      rgba4[count] = s3tc_dxt5_to_rgba_aos(gallivm, 4, format_desc->format, colors,
            default:
      assert(0);
   rgba4[count] = LLVMGetUndef(LLVMVectorType(i8t, 4));
      }
   /* shuffles typically give best results with dword elements...*/
      }
   rgba = lp_build_concat(gallivm, rgba4, lp_324_vectype, n / 4);
      }
   else {
               lp_build_gather_s3tc(gallivm, n, format_desc, &colors, &codewords,
            switch (format_desc->format) {
   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT1_SRGB:
   case PIPE_FORMAT_DXT1_SRGBA:
      rgba = s3tc_dxt1_to_rgba_aos(gallivm, n, format_desc->format,
            case PIPE_FORMAT_DXT3_RGBA:
   case PIPE_FORMAT_DXT3_SRGBA:
      rgba = s3tc_dxt3_to_rgba_aos(gallivm, n, format_desc->format, colors,
            case PIPE_FORMAT_DXT5_RGBA:
   case PIPE_FORMAT_DXT5_SRGBA:
      rgba = s3tc_dxt5_to_rgba_aos(gallivm, n, format_desc->format, colors,
            default:
      assert(0);
   rgba = LLVMGetUndef(LLVMVectorType(i8t, 4*n));
                              }
      /**
   * Gather elements from scatter positions in memory into vectors.
   * This is customised for fetching texels from s3tc textures.
   * For SSE, typical value is length=4.
   *
   * @param length length of the offsets
   * @param colors the stored colors of the blocks will be extracted into this.
   * @param codewords the codewords of the blocks will be extracted into this.
   * @param alpha_lo used for storing lower 32bit of alpha components for dxt3/5
   * @param alpha_hi used for storing higher 32bit of alpha components for dxt3/5
   * @param base_ptr base pointer, should be a i8 pointer type.
   * @param offsets vector with offsets
   */
   static void
   lp_build_gather_rgtc(struct gallivm_state *gallivm,
                        unsigned length,
   const struct util_format_description *format_desc,
      {
      LLVMBuilderRef builder = gallivm->builder;
   unsigned block_bits = format_desc->block.bits;
   unsigned i;
   LLVMValueRef elems[8];
   LLVMTypeRef type32 = LLVMInt32TypeInContext(gallivm->context);
   LLVMTypeRef type64 = LLVMInt64TypeInContext(gallivm->context);
   LLVMTypeRef type32dxt;
            memset(&lp_type32dxt, 0, sizeof lp_type32dxt);
   lp_type32dxt.width = 32;
   lp_type32dxt.length = block_bits / 32;
            assert(block_bits == 64 || block_bits == 128);
            for (i = 0; i < length; ++i) {
      elems[i] = lp_build_gather_elem(gallivm, length,
                  }
   if (length == 1) {
               *red_lo = LLVMBuildExtractElement(builder, elem,
         *red_hi = LLVMBuildExtractElement(builder, elem,
            if (block_bits == 128) {
      *green_lo = LLVMBuildExtractElement(builder, elem,
         *green_hi = LLVMBuildExtractElement(builder, elem,
      } else {
      *green_lo = NULL;
         } else {
      LLVMValueRef tmp[4];
   struct lp_type lp_type32, lp_type64;
   memset(&lp_type32, 0, sizeof lp_type32);
   lp_type32.width = 32;
   lp_type32.length = length;
   lp_type32.sign = lp_type32dxt.sign;
   memset(&lp_type64, 0, sizeof lp_type64);
   lp_type64.width = 64;
   lp_type64.length = length/2;
   if (block_bits == 128) {
      if (length == 8) {
      for (i = 0; i < 4; ++i) {
      tmp[0] = elems[i];
   tmp[1] = elems[i+4];
         }
   lp_build_transpose_aos(gallivm, lp_type32, elems, tmp);
   *green_lo = tmp[2];
   *green_hi = tmp[3];
   *red_lo = tmp[0];
      } else {
      LLVMValueRef red01, red23;
                  for (i = 0; i < length; ++i) {
      /* no-op shuffle */
   elems[i] = LLVMBuildShuffleVector(builder, elems[i],
            }
   if (length == 8) {
      struct lp_type lp_type32_4 = {0};
   lp_type32_4.width = 32;
   lp_type32_4.length = 4;
   for (i = 0; i < 4; ++i) {
      tmp[0] = elems[i];
   tmp[1] = elems[i+4];
         }
   red01 = lp_build_interleave2_half(gallivm, lp_type32, elems[0], elems[1], 0);
   red23 = lp_build_interleave2_half(gallivm, lp_type32, elems[2], elems[3], 0);
   red01 = LLVMBuildBitCast(builder, red01, type64_vec, "");
   red23 = LLVMBuildBitCast(builder, red23, type64_vec, "");
   *red_lo = lp_build_interleave2_half(gallivm, lp_type64, red01, red23, 0);
   *red_hi = lp_build_interleave2_half(gallivm, lp_type64, red01, red23, 1);
   *red_lo = LLVMBuildBitCast(builder, *red_lo, type32_vec, "");
   *red_hi = LLVMBuildBitCast(builder, *red_hi, type32_vec, "");
   *green_lo = NULL;
            }
      static LLVMValueRef
   rgtc1_to_rgba_aos(struct gallivm_state *gallivm,
                     unsigned n,
   enum pipe_format format,
   LLVMValueRef red_lo,
   {
      LLVMBuilderRef builder = gallivm->builder;
   bool is_signed = (format == PIPE_FORMAT_RGTC1_SNORM);
   LLVMValueRef red = s3tc_dxt5_alpha_channel(gallivm, is_signed, n, red_hi, red_lo, i, j);
   LLVMValueRef rgba;
   struct lp_type type, type8;
   memset(&type, 0, sizeof type);
   type.width = 32;
   type.length = n;
   memset(&type8, 0, sizeof type8);
   type8.width = 8;
   type8.length = n*4;
   rgba = lp_build_const_int_vec(gallivm, type, is_signed ? (0x7f << 24) : (0xff << 24));
   rgba = LLVMBuildOr(builder, rgba, red, "");
      }
      static LLVMValueRef
   rgtc2_to_rgba_aos(struct gallivm_state *gallivm,
                     unsigned n,
   enum pipe_format format,
   LLVMValueRef red_lo,
   LLVMValueRef red_hi,
   LLVMValueRef green_lo,
   {
      LLVMBuilderRef builder = gallivm->builder;
   bool is_signed = (format == PIPE_FORMAT_RGTC2_SNORM);
   LLVMValueRef red = s3tc_dxt5_alpha_channel(gallivm, is_signed, n, red_hi, red_lo, i, j);
   LLVMValueRef green = s3tc_dxt5_alpha_channel(gallivm, is_signed, n, green_hi, green_lo, i, j);
   LLVMValueRef rgba;
   struct lp_type type, type8;
   memset(&type, 0, sizeof type);
   type.width = 32;
   type.length = n;
   memset(&type8, 0, sizeof type8);
   type8.width = 8;
   type8.length = n*4;
   rgba = lp_build_const_int_vec(gallivm, type, is_signed ? (0x7f << 24) : (0xff << 24));
   rgba = LLVMBuildOr(builder, rgba, red, "");
   green = LLVMBuildShl(builder, green, lp_build_const_int_vec(gallivm, type, 8), "");
   rgba = LLVMBuildOr(builder, rgba, green, "");
      }
      static LLVMValueRef
   latc1_to_rgba_aos(struct gallivm_state *gallivm,
                     unsigned n,
   enum pipe_format format,
   LLVMValueRef red_lo,
   {
      LLVMBuilderRef builder = gallivm->builder;
   bool is_signed = (format == PIPE_FORMAT_LATC1_SNORM);
   LLVMValueRef red = s3tc_dxt5_alpha_channel(gallivm, is_signed, n, red_hi, red_lo, i, j);
   LLVMValueRef rgba, temp;
   struct lp_type type, type8;
   memset(&type, 0, sizeof type);
   type.width = 32;
   type.length = n;
   memset(&type8, 0, sizeof type8);
   type8.width = 8;
   type8.length = n*4;
   rgba = lp_build_const_int_vec(gallivm, type, is_signed ? (0x7f << 24) : (0xff << 24));
   rgba = LLVMBuildOr(builder, rgba, red, "");
   temp = LLVMBuildShl(builder, red, lp_build_const_int_vec(gallivm, type, 8), "");
   rgba = LLVMBuildOr(builder, rgba, temp, "");
   temp = LLVMBuildShl(builder, red, lp_build_const_int_vec(gallivm, type, 16), "");
   rgba = LLVMBuildOr(builder, rgba, temp, "");
      }
      static LLVMValueRef
   latc2_to_rgba_aos(struct gallivm_state *gallivm,
                     unsigned n,
   enum pipe_format format,
   LLVMValueRef red_lo,
   LLVMValueRef red_hi,
   LLVMValueRef green_lo,
   {
      LLVMBuilderRef builder = gallivm->builder;
   bool is_signed = (format == PIPE_FORMAT_LATC2_SNORM);
   LLVMValueRef red = s3tc_dxt5_alpha_channel(gallivm, is_signed, n, red_hi, red_lo, i, j);
   LLVMValueRef green = s3tc_dxt5_alpha_channel(gallivm, is_signed, n, green_hi, green_lo, i, j);
   LLVMValueRef rgba, temp;
   struct lp_type type, type8;
   memset(&type, 0, sizeof type);
   type.width = 32;
   type.length = n;
   memset(&type8, 0, sizeof type8);
   type8.width = 8;
            temp = LLVMBuildShl(builder, red, lp_build_const_int_vec(gallivm, type, 8), "");
   rgba = LLVMBuildOr(builder, red, temp, "");
   temp = LLVMBuildShl(builder, red, lp_build_const_int_vec(gallivm, type, 16), "");
   rgba = LLVMBuildOr(builder, rgba, temp, "");
   temp = LLVMBuildShl(builder, green, lp_build_const_int_vec(gallivm, type, 24), "");
   rgba = LLVMBuildOr(builder, rgba, temp, "");
      }
      /**
   * @param n  number of pixels processed (usually n=4, but it should also work with n=1
   *           and multiples of 4)
   * @param base_ptr  base pointer (32bit or 64bit pointer depending on the architecture)
   * @param offset <n x i32> vector with the relative offsets of the S3TC blocks
   * @param i  is a <n x i32> vector with the x subpixel coordinate (0..3)
   * @param j  is a <n x i32> vector with the y subpixel coordinate (0..3)
   * @return  a <4*n x i8> vector with the pixel RGBA values in AoS
   */
   LLVMValueRef
   lp_build_fetch_rgtc_rgba_aos(struct gallivm_state *gallivm,
                              const struct util_format_description *format_desc,
   unsigned n,
      {
      LLVMValueRef rgba;
   LLVMTypeRef i8t = LLVMInt8TypeInContext(gallivm->context);
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef red_lo, red_hi, green_lo, green_hi;
   assert(format_desc->layout == UTIL_FORMAT_LAYOUT_RGTC);
   assert(format_desc->block.width == 4);
                     if (n > 4) {
      unsigned count;
   LLVMTypeRef i128_type = LLVMIntTypeInContext(gallivm->context, 128);
   LLVMTypeRef i128_vectype =  LLVMVectorType(i128_type, n / 4);
   LLVMTypeRef i8_vectype = LLVMVectorType(i8t, 4 * n);
   LLVMTypeRef i324_vectype = LLVMVectorType(LLVMInt32TypeInContext(
         LLVMValueRef offset4, i4, j4, rgba4[LP_MAX_VECTOR_LENGTH/16];
                                 i4 = lp_build_extract_range(gallivm, i, count * 4, 4);
                                 switch (format_desc->format) {
   case PIPE_FORMAT_RGTC1_UNORM:
   case PIPE_FORMAT_RGTC1_SNORM:
      rgba4[count] = rgtc1_to_rgba_aos(gallivm, 4, format_desc->format,
            case PIPE_FORMAT_RGTC2_UNORM:
   case PIPE_FORMAT_RGTC2_SNORM:
      rgba4[count] = rgtc2_to_rgba_aos(gallivm, 4, format_desc->format,
            case PIPE_FORMAT_LATC1_UNORM:
   case PIPE_FORMAT_LATC1_SNORM:
      rgba4[count] = latc1_to_rgba_aos(gallivm, 4, format_desc->format,
            case PIPE_FORMAT_LATC2_UNORM:
   case PIPE_FORMAT_LATC2_SNORM:
      rgba4[count] = latc2_to_rgba_aos(gallivm, 4, format_desc->format,
            default:
      assert(0);
   rgba4[count] = LLVMGetUndef(LLVMVectorType(i8t, 4));
      }
   /* shuffles typically give best results with dword elements...*/
      }
   rgba = lp_build_concat(gallivm, rgba4, lp_324_vectype, n / 4);
      } else {
               lp_build_gather_rgtc(gallivm, n, format_desc, &red_lo, &red_hi,
            switch (format_desc->format) {
   case PIPE_FORMAT_RGTC1_UNORM:
   case PIPE_FORMAT_RGTC1_SNORM:
      rgba = rgtc1_to_rgba_aos(gallivm, n, format_desc->format,
            case PIPE_FORMAT_RGTC2_UNORM:
   case PIPE_FORMAT_RGTC2_SNORM:
      rgba = rgtc2_to_rgba_aos(gallivm, n, format_desc->format,
            case PIPE_FORMAT_LATC1_UNORM:
   case PIPE_FORMAT_LATC1_SNORM:
      rgba = latc1_to_rgba_aos(gallivm, n, format_desc->format,
            case PIPE_FORMAT_LATC2_UNORM:
   case PIPE_FORMAT_LATC2_SNORM:
      rgba = latc2_to_rgba_aos(gallivm, n, format_desc->format,
            default:
      assert(0);
   rgba = LLVMGetUndef(LLVMVectorType(i8t, 4*n));
         }
      }
