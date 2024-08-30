   /**************************************************************************
   *
   * Copyright 2009-2010 VMware, Inc.
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
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
         /**
   * @file
   * Helper
   *
   * LLVM IR doesn't support all basic arithmetic operations we care about (most
   * notably min/max and saturated operations), and it is often necessary to
   * resort machine-specific intrinsics directly. The functions here hide all
   * these implementation details from the other modules.
   *
   * We also do simple expressions simplification here. Reasons are:
   * - it is very easy given we have all necessary information readily available
   * - LLVM optimization passes fail to simplify several vector expressions
   * - We often know value constraints which the optimization passes have no way
   *   of knowing, such as when source arguments are known to be in [0, 1] range.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include <float.h>
      #include <llvm/Config/llvm-config.h>
      #include "util/u_memory.h"
   #include "util/u_debug.h"
   #include "util/u_math.h"
   #include "util/u_cpu_detect.h"
      #include "lp_bld_type.h"
   #include "lp_bld_const.h"
   #include "lp_bld_init.h"
   #include "lp_bld_intr.h"
   #include "lp_bld_logic.h"
   #include "lp_bld_pack.h"
   #include "lp_bld_debug.h"
   #include "lp_bld_bitarit.h"
   #include "lp_bld_arit.h"
   #include "lp_bld_flow.h"
      #if DETECT_ARCH_SSE
   #include <xmmintrin.h>
   #endif
      #ifndef _MM_DENORMALS_ZERO_MASK
   #define _MM_DENORMALS_ZERO_MASK 0x0040
   #endif
      #ifndef _MM_FLUSH_ZERO_MASK
   #define _MM_FLUSH_ZERO_MASK 0x8000
   #endif
      #define EXP_POLY_DEGREE 5
      #define LOG_POLY_DEGREE 4
         /**
   * Generate min(a, b)
   * No checks for special case values of a or b = 1 or 0 are done.
   * NaN's are handled according to the behavior specified by the
   * nan_behavior argument.
   */
   static LLVMValueRef
   lp_build_min_simple(struct lp_build_context *bld,
                     {
      const struct lp_type type = bld->type;
   const char *intrinsic = NULL;
   unsigned intr_size = 0;
            assert(lp_check_value(type, a));
                     if (type.floating && util_get_cpu_caps()->has_sse) {
      if (type.width == 32) {
      if (type.length == 1) {
      intrinsic = "llvm.x86.sse.min.ss";
      }
   else if (type.length <= 4 || !util_get_cpu_caps()->has_avx) {
      intrinsic = "llvm.x86.sse.min.ps";
      }
   else {
      intrinsic = "llvm.x86.avx.min.ps.256";
         }
   if (type.width == 64 && util_get_cpu_caps()->has_sse2) {
      if (type.length == 1) {
      intrinsic = "llvm.x86.sse2.min.sd";
      }
   else if (type.length == 2 || !util_get_cpu_caps()->has_avx) {
      intrinsic = "llvm.x86.sse2.min.pd";
      }
   else {
      intrinsic = "llvm.x86.avx.min.pd.256";
            }
   else if (type.floating && util_get_cpu_caps()->has_altivec) {
      if (nan_behavior == GALLIVM_NAN_RETURN_NAN_FIRST_NONNAN) {
      debug_printf("%s: altivec doesn't support nan return nan behavior\n",
      }
   if (type.width == 32 && type.length == 4) {
      intrinsic = "llvm.ppc.altivec.vminfp";
         } else if (util_get_cpu_caps()->has_altivec) {
      intr_size = 128;
   if (type.width == 8) {
      if (!type.sign) {
         } else {
            } else if (type.width == 16) {
      if (!type.sign) {
         } else {
            } else if (type.width == 32) {
      if (!type.sign) {
         } else {
                        if (intrinsic) {
      /* We need to handle nan's for floating point numbers. If one of the
   * inputs is nan the other should be returned (required by both D3D10+
   * and OpenCL).
   * The sse intrinsics return the second operator in case of nan by
   * default so we need to special code to handle those.
   */
   if (util_get_cpu_caps()->has_sse && type.floating &&
      nan_behavior == GALLIVM_NAN_RETURN_OTHER) {
   LLVMValueRef isnan, min;
   min = lp_build_intrinsic_binary_anylength(bld->gallivm, intrinsic,
               isnan = lp_build_isnan(bld, b);
      } else {
      return lp_build_intrinsic_binary_anylength(bld->gallivm, intrinsic,
                        if (type.floating) {
      switch (nan_behavior) {
   case GALLIVM_NAN_RETURN_OTHER: {
      LLVMValueRef isnan = lp_build_isnan(bld, a);
   cond = lp_build_cmp(bld, PIPE_FUNC_LESS, a, b);
   cond = LLVMBuildXor(bld->gallivm->builder, cond, isnan, "");
      }
         case GALLIVM_NAN_RETURN_OTHER_SECOND_NONNAN:
      cond = lp_build_cmp_ordered(bld, PIPE_FUNC_LESS, a, b);
      case GALLIVM_NAN_RETURN_NAN_FIRST_NONNAN:
      cond = lp_build_cmp(bld, PIPE_FUNC_LESS, b, a);
      case GALLIVM_NAN_BEHAVIOR_UNDEFINED:
      cond = lp_build_cmp(bld, PIPE_FUNC_LESS, a, b);
   return lp_build_select(bld, cond, a, b);
      default:
      assert(0);
   cond = lp_build_cmp(bld, PIPE_FUNC_LESS, a, b);
         } else {
      cond = lp_build_cmp(bld, PIPE_FUNC_LESS, a, b);
         }
         LLVMValueRef
   lp_build_fmuladd(LLVMBuilderRef builder,
                     {
      LLVMTypeRef type = LLVMTypeOf(a);
   assert(type == LLVMTypeOf(b));
            char intrinsic[32];
   lp_format_intrinsic(intrinsic, sizeof intrinsic, "llvm.fmuladd", type);
   LLVMValueRef args[] = { a, b, c };
      }
         /**
   * Generate max(a, b)
   * No checks for special case values of a or b = 1 or 0 are done.
   * NaN's are handled according to the behavior specified by the
   * nan_behavior argument.
   */
   static LLVMValueRef
   lp_build_max_simple(struct lp_build_context *bld,
                     {
      const struct lp_type type = bld->type;
   const char *intrinsic = NULL;
   unsigned intr_size = 0;
            assert(lp_check_value(type, a));
                     if (type.floating && util_get_cpu_caps()->has_sse) {
      if (type.width == 32) {
      if (type.length == 1) {
      intrinsic = "llvm.x86.sse.max.ss";
      }
   else if (type.length <= 4 || !util_get_cpu_caps()->has_avx) {
      intrinsic = "llvm.x86.sse.max.ps";
      }
   else {
      intrinsic = "llvm.x86.avx.max.ps.256";
         }
   if (type.width == 64 && util_get_cpu_caps()->has_sse2) {
      if (type.length == 1) {
      intrinsic = "llvm.x86.sse2.max.sd";
      }
   else if (type.length == 2 || !util_get_cpu_caps()->has_avx) {
      intrinsic = "llvm.x86.sse2.max.pd";
      }
   else {
      intrinsic = "llvm.x86.avx.max.pd.256";
            }
   else if (type.floating && util_get_cpu_caps()->has_altivec) {
      if (nan_behavior == GALLIVM_NAN_RETURN_NAN_FIRST_NONNAN) {
      debug_printf("%s: altivec doesn't support nan return nan behavior\n",
      }
   if (type.width == 32 || type.length == 4) {
      intrinsic = "llvm.ppc.altivec.vmaxfp";
         } else if (util_get_cpu_caps()->has_altivec) {
   intr_size = 128;
   if (type.width == 8) {
      if (!type.sign) {
         } else {
            } else if (type.width == 16) {
      if (!type.sign) {
         } else {
            } else if (type.width == 32) {
      if (!type.sign) {
         } else {
            }
            if (intrinsic) {
      if (util_get_cpu_caps()->has_sse && type.floating &&
      nan_behavior == GALLIVM_NAN_RETURN_OTHER) {
   LLVMValueRef isnan, max;
   max = lp_build_intrinsic_binary_anylength(bld->gallivm, intrinsic,
               isnan = lp_build_isnan(bld, b);
      } else {
      return lp_build_intrinsic_binary_anylength(bld->gallivm, intrinsic,
                        if (type.floating) {
      switch (nan_behavior) {
   case GALLIVM_NAN_RETURN_OTHER: {
      LLVMValueRef isnan = lp_build_isnan(bld, a);
   cond = lp_build_cmp(bld, PIPE_FUNC_GREATER, a, b);
   cond = LLVMBuildXor(bld->gallivm->builder, cond, isnan, "");
      }
         case GALLIVM_NAN_RETURN_OTHER_SECOND_NONNAN:
      cond = lp_build_cmp_ordered(bld, PIPE_FUNC_GREATER, a, b);
      case GALLIVM_NAN_RETURN_NAN_FIRST_NONNAN:
      cond = lp_build_cmp(bld, PIPE_FUNC_GREATER, b, a);
      case GALLIVM_NAN_BEHAVIOR_UNDEFINED:
      cond = lp_build_cmp(bld, PIPE_FUNC_GREATER, a, b);
   return lp_build_select(bld, cond, a, b);
      default:
      assert(0);
   cond = lp_build_cmp(bld, PIPE_FUNC_GREATER, a, b);
         } else {
      cond = lp_build_cmp(bld, PIPE_FUNC_GREATER, a, b);
         }
         /**
   * Generate 1 - a, or ~a depending on bld->type.
   */
   LLVMValueRef
   lp_build_comp(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
                     if (a == bld->one)
         if (a == bld->zero)
            if (type.norm && !type.floating && !type.fixed && !type.sign) {
      if (LLVMIsConstant(a))
         else
               if (type.floating)
         else
      }
         /**
   * Generate a + b
   */
   LLVMValueRef
   lp_build_add(struct lp_build_context *bld,
               {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
            assert(lp_check_value(type, a));
            if (a == bld->zero)
         if (b == bld->zero)
         if (a == bld->undef || b == bld->undef)
            if (type.norm) {
               if (!type.sign && (a == bld->one || b == bld->one))
            if (!type.floating && !type.fixed) {
      if (LLVM_VERSION_MAJOR >= 8) {
      char intrin[32];
   intrinsic = type.sign ? "llvm.sadd.sat" : "llvm.uadd.sat";
   lp_format_intrinsic(intrin, sizeof intrin, intrinsic, bld->vec_type);
      }
   if (type.width * type.length == 128) {
      if (util_get_cpu_caps()->has_sse2) {
      if (type.width == 8)
   intrinsic = type.sign ? "llvm.x86.sse2.padds.b" : "llvm.x86.sse2.paddus.b";
   if (type.width == 16)
      } else if (util_get_cpu_caps()->has_altivec) {
      if (type.width == 8)
         if (type.width == 16)
         }
   if (type.width * type.length == 256) {
      if (util_get_cpu_caps()->has_avx2) {
      if (type.width == 8)
         if (type.width == 16)
                     if (intrinsic)
      return lp_build_intrinsic_binary(builder, intrinsic,
            if (type.norm && !type.floating && !type.fixed) {
      if (type.sign) {
      uint64_t sign = (uint64_t)1 << (type.width - 1);
   LLVMValueRef max_val = lp_build_const_int_vec(bld->gallivm, type, sign - 1);
   LLVMValueRef min_val = lp_build_const_int_vec(bld->gallivm, type, sign);
   /* a_clamp_max is the maximum a for positive b,
         LLVMValueRef a_clamp_max =
      lp_build_min_simple(bld, a, LLVMBuildSub(builder, max_val, b, ""),
      LLVMValueRef a_clamp_min =
      lp_build_max_simple(bld, a, LLVMBuildSub(builder, min_val, b, ""),
      a = lp_build_select(bld, lp_build_cmp(bld, PIPE_FUNC_GREATER, b,
                  if (type.floating)
         else
            /* clamp to ceiling of 1.0 */
   if (bld->type.norm && (bld->type.floating || bld->type.fixed))
            if (type.norm && !type.floating && !type.fixed) {
      if (!type.sign) {
      /*
   * newer llvm versions no longer support the intrinsics, but recognize
   * the pattern. Since auto-upgrade of intrinsics doesn't work for jit
   * code, it is important we match the pattern llvm uses (and pray llvm
   * doesn't change it - and hope they decide on the same pattern for
   * all backends supporting it...).
   * NOTE: cmp/select does sext/trunc of the mask. Does not seem to
   * interfere with llvm's ability to recognize the pattern but seems
   * a bit brittle.
   * NOTE: llvm 9+ always uses (non arch specific) intrinsic.
   */
   LLVMValueRef overflowed = lp_build_cmp(bld, PIPE_FUNC_GREATER, a, res);
   res = lp_build_select(bld, overflowed,
                              }
         /** Return the scalar sum of the elements of a.
   * Should avoid this operation whenever possible.
   */
   LLVMValueRef
   lp_build_horizontal_add(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef index, res;
   unsigned i, length;
   LLVMValueRef shuffles1[LP_MAX_VECTOR_LENGTH / 2];
   LLVMValueRef shuffles2[LP_MAX_VECTOR_LENGTH / 2];
                     if (type.length == 1) {
                           /*
   * for byte vectors can do much better with psadbw.
   * Using repeated shuffle/adds here. Note with multiple vectors
   * this can be done more efficiently as outlined in the intel
   * optimization manual.
   * Note: could cause data rearrangement if used with smaller element
   * sizes.
            vecres = a;
   length = type.length / 2;
   while (length > 1) {
      LLVMValueRef vec1, vec2;
   for (i = 0; i < length; i++) {
      shuffles1[i] = lp_build_const_int32(bld->gallivm, i);
      }
   vec1 = LLVMBuildShuffleVector(builder, vecres, vecres,
         vec2 = LLVMBuildShuffleVector(builder, vecres, vecres,
         if (type.floating) {
         }
   else {
         }
               /* always have vector of size 2 here */
            index = lp_build_const_int32(bld->gallivm, 0);
   res = LLVMBuildExtractElement(builder, vecres, index, "");
   index = lp_build_const_int32(bld->gallivm, 1);
            if (type.floating)
         else
               }
         /**
   * Return the horizontal sums of 4 float vectors as a float4 vector.
   * This uses the technique as outlined in Intel Optimization Manual.
   */
   static LLVMValueRef
   lp_build_horizontal_add4x4f(struct lp_build_context *bld,
         {
      struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef shuffles[4];
   LLVMValueRef tmp[4];
            /* lower half of regs */
   shuffles[0] = lp_build_const_int32(gallivm, 0);
   shuffles[1] = lp_build_const_int32(gallivm, 1);
   shuffles[2] = lp_build_const_int32(gallivm, 4);
   shuffles[3] = lp_build_const_int32(gallivm, 5);
   tmp[0] = LLVMBuildShuffleVector(builder, src[0], src[1],
         tmp[2] = LLVMBuildShuffleVector(builder, src[2], src[3],
            /* upper half of regs */
   shuffles[0] = lp_build_const_int32(gallivm, 2);
   shuffles[1] = lp_build_const_int32(gallivm, 3);
   shuffles[2] = lp_build_const_int32(gallivm, 6);
   shuffles[3] = lp_build_const_int32(gallivm, 7);
   tmp[1] = LLVMBuildShuffleVector(builder, src[0], src[1],
         tmp[3] = LLVMBuildShuffleVector(builder, src[2], src[3],
            sumtmp[0] = LLVMBuildFAdd(builder, tmp[0], tmp[1], "");
            shuffles[0] = lp_build_const_int32(gallivm, 0);
   shuffles[1] = lp_build_const_int32(gallivm, 2);
   shuffles[2] = lp_build_const_int32(gallivm, 4);
   shuffles[3] = lp_build_const_int32(gallivm, 6);
   shuftmp[0] = LLVMBuildShuffleVector(builder, sumtmp[0], sumtmp[1],
            shuffles[0] = lp_build_const_int32(gallivm, 1);
   shuffles[1] = lp_build_const_int32(gallivm, 3);
   shuffles[2] = lp_build_const_int32(gallivm, 5);
   shuffles[3] = lp_build_const_int32(gallivm, 7);
   shuftmp[1] = LLVMBuildShuffleVector(builder, sumtmp[0], sumtmp[1],
               }
         /*
   * partially horizontally add 2-4 float vectors with length nx4,
   * i.e. only four adjacent values in each vector will be added,
   * assuming values are really grouped in 4 which also determines
   * output order.
   *
   * Return a vector of the same length as the initial vectors,
   * with the excess elements (if any) being undefined.
   * The element order is independent of number of input vectors.
   * For 3 vectors x0x1x2x3x4x5x6x7, y0y1y2y3y4y5y6y7, z0z1z2z3z4z5z6z7
   * the output order thus will be
   * sumx0-x3,sumy0-y3,sumz0-z3,undef,sumx4-x7,sumy4-y7,sumz4z7,undef
   */
   LLVMValueRef
   lp_build_hadd_partial4(struct lp_build_context *bld,
               {
      struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef ret_vec;
   LLVMValueRef tmp[4];
            assert(num_vecs >= 2 && num_vecs <= 4);
            /* only use this with at least 2 vectors, as it is sort of expensive
   * (depending on cpu) and we always need two horizontal adds anyway,
   * so a shuffle/add approach might be better.
            tmp[0] = vectors[0];
            tmp[2] = num_vecs > 2 ? vectors[2] : vectors[0];
            if (util_get_cpu_caps()->has_sse3 && bld->type.width == 32 &&
      bld->type.length == 4) {
      }
   else if (util_get_cpu_caps()->has_avx && bld->type.width == 32 &&
               }
   if (intrinsic) {
      tmp[0] = lp_build_intrinsic_binary(builder, intrinsic,
               if (num_vecs > 2) {
      tmp[1] = lp_build_intrinsic_binary(builder, intrinsic,
            }
   else {
         }
   return lp_build_intrinsic_binary(builder, intrinsic,
                     if (bld->type.length == 4) {
         }
   else {
      LLVMValueRef partres[LP_MAX_VECTOR_LENGTH/4];
   unsigned j;
   unsigned num_iter = bld->type.length / 4;
   struct lp_type parttype = bld->type;
   parttype.length = 4;
   for (j = 0; j < num_iter; j++) {
      LLVMValueRef partsrc[4];
   unsigned i;
   for (i = 0; i < 4; i++) {
         }
      }
      }
      }
         /**
   * Generate a - b
   */
   LLVMValueRef
   lp_build_sub(struct lp_build_context *bld,
               {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
            assert(lp_check_value(type, a));
            if (b == bld->zero)
         if (a == bld->undef || b == bld->undef)
         if (a == b)
            if (type.norm) {
               if (!type.sign && b == bld->one)
            if (!type.floating && !type.fixed) {
      if (LLVM_VERSION_MAJOR >= 8) {
      char intrin[32];
   intrinsic = type.sign ? "llvm.ssub.sat" : "llvm.usub.sat";
   lp_format_intrinsic(intrin, sizeof intrin, intrinsic, bld->vec_type);
      }
   if (type.width * type.length == 128) {
      if (util_get_cpu_caps()->has_sse2) {
      if (type.width == 8)
         if (type.width == 16)
      } else if (util_get_cpu_caps()->has_altivec) {
      if (type.width == 8)
         if (type.width == 16)
         }
   if (type.width * type.length == 256) {
      if (util_get_cpu_caps()->has_avx2) {
      if (type.width == 8)
         if (type.width == 16)
                     if (intrinsic)
      return lp_build_intrinsic_binary(builder, intrinsic,
            if (type.norm && !type.floating && !type.fixed) {
      if (type.sign) {
      uint64_t sign = (uint64_t)1 << (type.width - 1);
   LLVMValueRef max_val =
         LLVMValueRef min_val =
         /* a_clamp_max is the maximum a for negative b,
         LLVMValueRef a_clamp_max =
      lp_build_min_simple(bld, a, LLVMBuildAdd(builder, max_val, b, ""),
      LLVMValueRef a_clamp_min =
      lp_build_max_simple(bld, a, LLVMBuildAdd(builder, min_val, b, ""),
      a = lp_build_select(bld, lp_build_cmp(bld, PIPE_FUNC_GREATER, b,
            } else {
      /*
   * This must match llvm pattern for saturated unsigned sub.
   * (lp_build_max_simple actually does the job with its current
   * definition but do it explicitly here.)
   * NOTE: cmp/select does sext/trunc of the mask. Does not seem to
   * interfere with llvm's ability to recognize the pattern but seems
   * a bit brittle.
   * NOTE: llvm 9+ always uses (non arch specific) intrinsic.
   */
   LLVMValueRef no_ov = lp_build_cmp(bld, PIPE_FUNC_GREATER, a, b);
                  if (type.floating)
         else
            if (bld->type.norm && (bld->type.floating || bld->type.fixed))
               }
         /**
   * Normalized multiplication.
   *
   * There are several approaches for (using 8-bit normalized multiplication as
   * an example):
   *
   * - alpha plus one
   *
   *     makes the following approximation to the division (Sree)
   *
   *       a*b/255 ~= (a*(b + 1)) >> 256
   *
   *     which is the fastest method that satisfies the following OpenGL
   *     criteria of
   *
   *       0*0 = 0 and 255*255 = 255
   *
   * - geometric series
   *
   *     takes the geometric series approximation to the division
   *
   *       t/255 = (t >> 8) + (t >> 16) + (t >> 24) ..
   *
   *     in this case just the first two terms to fit in 16bit arithmetic
   *
   *       t/255 ~= (t + (t >> 8)) >> 8
   *
   *     note that just by itself it doesn't satisfies the OpenGL criteria,
   *     as 255*255 = 254, so the special case b = 255 must be accounted or
   *     roundoff must be used.
   *
   * - geometric series plus rounding
   *
   *     when using a geometric series division instead of truncating the result
   *     use roundoff in the approximation (Jim Blinn)
   *
   *       t/255 ~= (t + (t >> 8) + 0x80) >> 8
   *
   *     achieving the exact results.
   *
   *
   *
   * @sa Alvy Ray Smith, Image Compositing Fundamentals, Tech Memo 4, Aug 15, 1995,
   *     ftp://ftp.alvyray.com/Acrobat/4_Comp.pdf
   * @sa Michael Herf, The "double blend trick", May 2000,
   *     http://www.stereopsis.com/doubleblend.html
   */
   LLVMValueRef
   lp_build_mul_norm(struct gallivm_state *gallivm,
               {
      LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context bld;
   unsigned n;
   LLVMValueRef half;
            assert(!wide_type.floating);
   assert(lp_check_value(wide_type, a));
                     n = wide_type.width / 2;
   if (wide_type.sign) {
                  /*
   * TODO: for 16bits normalized SSE2 vectors we could consider using PMULHUW
   * http://ssp.impulsetrain.com/2011/07/03/multiplying-normalized-16-bit-numbers-with-sse2/
            /*
   * a*b / (2**n - 1) ~= (a*b + (a*b >> n) + half) >> n
            ab = LLVMBuildMul(builder, a, b, "");
            /*
   * half = sgn(ab) * 0.5 * (2 ** n) = sgn(ab) * (1 << (n - 1))
            half = lp_build_const_int_vec(gallivm, wide_type, 1LL << (n - 1));
   if (wide_type.sign) {
      LLVMValueRef minus_half = LLVMBuildNeg(builder, half, "");
   LLVMValueRef sign = lp_build_shr_imm(&bld, ab, wide_type.width - 1);
      }
            /* Final division */
               }
         /**
   * Generate a * b
   */
   LLVMValueRef
   lp_build_mul(struct lp_build_context *bld,
               {
      LLVMBuilderRef builder = bld->gallivm->builder;
            assert(lp_check_value(type, a));
            if (a == bld->zero)
         if (a == bld->one)
         if (b == bld->zero)
         if (b == bld->one)
         if (a == bld->undef || b == bld->undef)
            if (!type.floating && !type.fixed && type.norm) {
      struct lp_type wide_type = lp_wider_type(type);
            lp_build_unpack2_native(bld->gallivm, type, wide_type, a, &al, &ah);
            /* PMULLW, PSRLW, PADDW */
   abl = lp_build_mul_norm(bld->gallivm, wide_type, al, bl);
                                 LLVMValueRef shift = type.fixed
            LLVMValueRef res;
   if (type.floating)
         else
         if (shift) {
      if (type.sign)
         else
                  }
         /*
   * Widening mul, valid for 32x32 bit -> 64bit only.
   * Result is low 32bits, high bits returned in res_hi.
   *
   * Emits code that is meant to be compiled for the host CPU.
   */
   LLVMValueRef
   lp_build_mul_32_lohi_cpu(struct lp_build_context *bld,
                     {
      struct gallivm_state *gallivm = bld->gallivm;
            assert(bld->type.width == 32);
   assert(bld->type.floating == 0);
   assert(bld->type.fixed == 0);
            /*
   * XXX: for some reason, with zext/zext/mul/trunc the code llvm produces
   * for x86 simd is atrocious (even if the high bits weren't required),
   * trying to handle real 64bit inputs (which of course can't happen due
   * to using 64bit umul with 32bit numbers zero-extended to 64bit, but
   * apparently llvm does not recognize this widening mul). This includes 6
   * (instead of 2) pmuludq plus extra adds and shifts
   * The same story applies to signed mul, albeit fixing this requires sse41.
   * https://llvm.org/bugs/show_bug.cgi?id=30845
   * So, whip up our own code, albeit only for length 4 and 8 (which
   * should be good enough)...
   * FIXME: For llvm >= 7.0 we should match the autoupgrade pattern
   * (bitcast/and/mul/shuffle for unsigned, bitcast/shl/ashr/mul/shuffle
   * for signed), which the fallback code does not, without this llvm
   * will likely still produce atrocious code.
   */
   if (LLVM_VERSION_MAJOR < 7 &&
      (bld->type.length == 4 || bld->type.length == 8) &&
   ((util_get_cpu_caps()->has_sse2 && (bld->type.sign == 0)) ||
   util_get_cpu_caps()->has_sse4_1)) {
   const char *intrinsic = NULL;
   LLVMValueRef aeven, aodd, beven, bodd, muleven, mulodd;
   LLVMValueRef shuf[LP_MAX_VECTOR_WIDTH / 32], shuf_vec;
   struct lp_type type_wide = lp_wider_type(bld->type);
   LLVMTypeRef wider_type = lp_build_vec_type(gallivm, type_wide);
   unsigned i;
   for (i = 0; i < bld->type.length; i += 2) {
      shuf[i] = lp_build_const_int32(gallivm, i+1);
      }
   shuf_vec = LLVMConstVector(shuf, bld->type.length);
   aeven = a;
   beven = b;
   aodd = LLVMBuildShuffleVector(builder, aeven, bld->undef, shuf_vec, "");
            if (util_get_cpu_caps()->has_avx2 && bld->type.length == 8) {
      if (bld->type.sign) {
         } else {
         }
   muleven = lp_build_intrinsic_binary(builder, intrinsic,
         mulodd = lp_build_intrinsic_binary(builder, intrinsic,
      }
   else {
      /* for consistent naming look elsewhere... */
   if (bld->type.sign) {
         } else {
         }
   /*
   * XXX If we only have AVX but not AVX2 this is a pain.
   * lp_build_intrinsic_binary_anylength() can't handle it
   * (due to src and dst type not being identical).
   */
   if (bld->type.length == 8) {
      LLVMValueRef aevenlo, aevenhi, bevenlo, bevenhi;
   LLVMValueRef aoddlo, aoddhi, boddlo, boddhi;
   LLVMValueRef muleven2[2], mulodd2[2];
   struct lp_type type_wide_half = type_wide;
   LLVMTypeRef wtype_half;
   type_wide_half.length = 2;
   wtype_half = lp_build_vec_type(gallivm, type_wide_half);
   aevenlo = lp_build_extract_range(gallivm, aeven, 0, 4);
   aevenhi = lp_build_extract_range(gallivm, aeven, 4, 4);
   bevenlo = lp_build_extract_range(gallivm, beven, 0, 4);
   bevenhi = lp_build_extract_range(gallivm, beven, 4, 4);
   aoddlo = lp_build_extract_range(gallivm, aodd, 0, 4);
   aoddhi = lp_build_extract_range(gallivm, aodd, 4, 4);
   boddlo = lp_build_extract_range(gallivm, bodd, 0, 4);
   boddhi = lp_build_extract_range(gallivm, bodd, 4, 4);
   muleven2[0] = lp_build_intrinsic_binary(builder, intrinsic,
         mulodd2[0] = lp_build_intrinsic_binary(builder, intrinsic,
         muleven2[1] = lp_build_intrinsic_binary(builder, intrinsic,
         mulodd2[1] = lp_build_intrinsic_binary(builder, intrinsic,
                     }
   else {
      muleven = lp_build_intrinsic_binary(builder, intrinsic,
         mulodd = lp_build_intrinsic_binary(builder, intrinsic,
         }
   muleven = LLVMBuildBitCast(builder, muleven, bld->vec_type, "");
            for (i = 0; i < bld->type.length; i += 2) {
      shuf[i] = lp_build_const_int32(gallivm, i + 1);
      }
   shuf_vec = LLVMConstVector(shuf, bld->type.length);
            for (i = 0; i < bld->type.length; i += 2) {
      shuf[i] = lp_build_const_int32(gallivm, i);
      }
   shuf_vec = LLVMConstVector(shuf, bld->type.length);
      }
   else {
            }
         /*
   * Widening mul, valid for <= 32 (8, 16, 32) -> 64
   * Result is low N bits, high bits returned in res_hi.
   *
   * Emits generic code.
   */
   LLVMValueRef
   lp_build_mul_32_lohi(struct lp_build_context *bld,
                     {
      struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef tmp, shift, res_lo;
   struct lp_type type_tmp;
            type_tmp = bld->type;
   narrow_type = lp_build_vec_type(gallivm, type_tmp);
   if (bld->type.width < 32)
         else
         wide_type = lp_build_vec_type(gallivm, type_tmp);
            if (bld->type.sign) {
      a = LLVMBuildSExt(builder, a, wide_type, "");
      } else {
      a = LLVMBuildZExt(builder, a, wide_type, "");
      }
                     /* Since we truncate anyway, LShr and AShr are equivalent. */
   tmp = LLVMBuildLShr(builder, tmp, shift, "");
               }
         /* a * b + c */
   LLVMValueRef
   lp_build_mad(struct lp_build_context *bld,
               LLVMValueRef a,
   {
      const struct lp_type type = bld->type;
   if (type.floating) {
         } else {
            }
         /**
   * Small vector x scale multiplication optimization.
   */
   LLVMValueRef
   lp_build_mul_imm(struct lp_build_context *bld,
               {
      LLVMBuilderRef builder = bld->gallivm->builder;
                     if (b == 0)
            if (b == 1)
            if (b == -1)
            if (b == 2 && bld->type.floating)
            if (util_is_power_of_two_or_zero(b)) {
               #if 0
            /*
   * Power of two multiplication by directly manipulating the exponent.
   *
   * XXX: This might not be always faster, it will introduce a small
   * error for multiplication by zero, and it will produce wrong results
   * for Inf and NaN.
   */
   unsigned mantissa = lp_mantissa(bld->type);
   factor = lp_build_const_int_vec(bld->gallivm, bld->type, (unsigned long long)shift << mantissa);
   a = LLVMBuildBitCast(builder, a, lp_build_int_vec_type(bld->type), "");
   a = LLVMBuildAdd(builder, a, factor, "");
      #endif
         }
   else {
      factor = lp_build_const_vec(bld->gallivm, bld->type, shift);
                  factor = lp_build_const_vec(bld->gallivm, bld->type, (double)b);
      }
         /**
   * Generate a / b
   */
   LLVMValueRef
   lp_build_div(struct lp_build_context *bld,
               {
      LLVMBuilderRef builder = bld->gallivm->builder;
            assert(lp_check_value(type, a));
            if (a == bld->zero)
         if (a == bld->one && type.floating)
         if (b == bld->zero)
         if (b == bld->one)
         if (a == bld->undef || b == bld->undef)
            /* fast rcp is disabled (just uses div), so makes no sense to try that */
   if (false &&
      ((util_get_cpu_caps()->has_sse && type.width == 32 && type.length == 4) ||
   (util_get_cpu_caps()->has_avx && type.width == 32 && type.length == 8)) &&
   type.floating)
         if (type.floating)
         else if (type.sign)
         else
      }
         /**
   * Linear interpolation helper.
   *
   * @param normalized whether we are interpolating normalized values,
   *        encoded in normalized integers, twice as wide.
   *
   * @sa http://www.stereopsis.com/doubleblend.html
   */
   static inline LLVMValueRef
   lp_build_lerp_simple(struct lp_build_context *bld,
                           {
      unsigned half_width = bld->type.width/2;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef delta;
            assert(lp_check_value(bld->type, x));
   assert(lp_check_value(bld->type, v0));
                     if (bld->type.floating) {
      assert(flags == 0);
               if (flags & LP_BLD_LERP_WIDE_NORMALIZED) {
      if (!bld->type.sign) {
      if (!(flags & LP_BLD_LERP_PRESCALED_WEIGHTS)) {
      /*
   * Scale x from [0, 2**n - 1] to [0, 2**n] by adding the
   * most-significant-bit to the lowest-significant-bit, so that
                              /* (x * delta) >> n */
   /*
   * For this multiply, higher internal precision is required to pass
   * CTS, the most efficient path to that is pmulhrsw on ssse3 and
   * above.  This could be opencoded on other arches if conformance was
   * required.
   */
   if (bld->type.width == 16 && bld->type.length == 8 && util_get_cpu_caps()->has_ssse3) {
      res = lp_build_intrinsic_binary(builder, "llvm.x86.ssse3.pmul.hr.sw.128", bld->vec_type, x, lp_build_shl_imm(bld, delta, 7));
      } else if (bld->type.width == 16 && bld->type.length == 16 && util_get_cpu_caps()->has_avx2) {
      res = lp_build_intrinsic_binary(builder, "llvm.x86.avx2.pmul.hr.sw", bld->vec_type, x, lp_build_shl_imm(bld, delta, 7));
      } else {
      res = lp_build_mul(bld, x, delta);
         } else {
      /*
   * The rescaling trick above doesn't work for signed numbers, so
   * use the 2**n - 1 divison approximation in lp_build_mul_norm
   * instead.
   */
   assert(!(flags & LP_BLD_LERP_PRESCALED_WEIGHTS));
         } else {
      assert(!(flags & LP_BLD_LERP_PRESCALED_WEIGHTS));
               if ((flags & LP_BLD_LERP_WIDE_NORMALIZED) && !bld->type.sign) {
      /*
   * At this point both res and v0 only use the lower half of the bits,
   * the rest is zero. Instead of add / mask, do add with half wide type.
   */
   struct lp_type narrow_type;
            memset(&narrow_type, 0, sizeof narrow_type);
   narrow_type.sign   = bld->type.sign;
   narrow_type.width  = bld->type.width/2;
            lp_build_context_init(&narrow_bld, bld->gallivm, narrow_type);
   res = LLVMBuildBitCast(builder, res, narrow_bld.vec_type, "");
   v0 = LLVMBuildBitCast(builder, v0, narrow_bld.vec_type, "");
   res = lp_build_add(&narrow_bld, v0, res);
      } else {
               if (bld->type.fixed) {
      /*
   * We need to mask out the high order bits when lerping 8bit
   * normalized colors stored on 16bits
   */
   /* XXX: This step is necessary for lerping 8bit colors stored on
   * 16bits, but it will be wrong for true fixed point use cases.
   * Basically we need a more powerful lp_type, capable of further
   * distinguishing the values interpretation from the value storage.
   */
   LLVMValueRef low_bits;
   low_bits = lp_build_const_int_vec(bld->gallivm, bld->type, (1 << half_width) - 1);
                     }
         /**
   * Linear interpolation.
   */
   LLVMValueRef
   lp_build_lerp(struct lp_build_context *bld,
               LLVMValueRef x,
   LLVMValueRef v0,
   {
      const struct lp_type type = bld->type;
            assert(lp_check_value(type, x));
   assert(lp_check_value(type, v0));
                     if (type.norm) {
      struct lp_type wide_type;
   struct lp_build_context wide_bld;
                     /*
   * Create a wider integer type, enough to hold the
   * intermediate result of the multiplication.
   */
   memset(&wide_type, 0, sizeof wide_type);
   wide_type.sign   = type.sign;
   wide_type.width  = type.width*2;
                     lp_build_unpack2_native(bld->gallivm, type, wide_type, x,  &xl,  &xh);
   lp_build_unpack2_native(bld->gallivm, type, wide_type, v0, &v0l, &v0h);
            /*
   * Lerp both halves.
                     resl = lp_build_lerp_simple(&wide_bld, xl, v0l, v1l, flags);
               } else {
                     }
         /**
   * Bilinear interpolation.
   *
   * Values indices are in v_{yx}.
   */
   LLVMValueRef
   lp_build_lerp_2d(struct lp_build_context *bld,
                  LLVMValueRef x,
   LLVMValueRef y,
   LLVMValueRef v00,
   LLVMValueRef v01,
      {
      LLVMValueRef v0 = lp_build_lerp(bld, x, v00, v01, flags);
   LLVMValueRef v1 = lp_build_lerp(bld, x, v10, v11, flags);
      }
         LLVMValueRef
   lp_build_lerp_3d(struct lp_build_context *bld,
                  LLVMValueRef x,
   LLVMValueRef y,
   LLVMValueRef z,
   LLVMValueRef v000,
   LLVMValueRef v001,
   LLVMValueRef v010,
   LLVMValueRef v011,
   LLVMValueRef v100,
   LLVMValueRef v101,
      {
      LLVMValueRef v0 = lp_build_lerp_2d(bld, x, y, v000, v001, v010, v011, flags);
   LLVMValueRef v1 = lp_build_lerp_2d(bld, x, y, v100, v101, v110, v111, flags);
      }
         /**
   * Generate min(a, b)
   * Do checks for special cases but not for nans.
   */
   LLVMValueRef
   lp_build_min(struct lp_build_context *bld,
               {
      assert(lp_check_value(bld->type, a));
            if (a == bld->undef || b == bld->undef)
            if (a == b)
            if (bld->type.norm) {
      if (!bld->type.sign) {
      if (a == bld->zero || b == bld->zero) {
            }
   if (a == bld->one)
         if (b == bld->one)
                  }
         /**
   * Generate min(a, b)
   * NaN's are handled according to the behavior specified by the
   * nan_behavior argument.
   */
   LLVMValueRef
   lp_build_min_ext(struct lp_build_context *bld,
                     {
      assert(lp_check_value(bld->type, a));
            if (a == bld->undef || b == bld->undef)
            if (a == b)
            if (bld->type.norm) {
      if (!bld->type.sign) {
      if (a == bld->zero || b == bld->zero) {
            }
   if (a == bld->one)
         if (b == bld->one)
                  }
         /**
   * Generate max(a, b)
   * Do checks for special cases, but NaN behavior is undefined.
   */
   LLVMValueRef
   lp_build_max(struct lp_build_context *bld,
               {
      assert(lp_check_value(bld->type, a));
            if (a == bld->undef || b == bld->undef)
            if (a == b)
            if (bld->type.norm) {
      if (a == bld->one || b == bld->one)
         if (!bld->type.sign) {
      if (a == bld->zero) {
         }
   if (b == bld->zero) {
                           }
         /**
   * Generate max(a, b)
   * Checks for special cases.
   * NaN's are handled according to the behavior specified by the
   * nan_behavior argument.
   */
   LLVMValueRef
   lp_build_max_ext(struct lp_build_context *bld,
                     {
      assert(lp_check_value(bld->type, a));
            if (a == bld->undef || b == bld->undef)
            if (a == b)
            if (bld->type.norm) {
      if (a == bld->one || b == bld->one)
         if (!bld->type.sign) {
      if (a == bld->zero) {
         }
   if (b == bld->zero) {
                           }
         /**
   * Generate clamp(a, min, max)
   * NaN behavior (for any of a, min, max) is undefined.
   * Do checks for special cases.
   */
   LLVMValueRef
   lp_build_clamp(struct lp_build_context *bld,
                     {
      assert(lp_check_value(bld->type, a));
   assert(lp_check_value(bld->type, min));
            a = lp_build_min(bld, a, max);
   a = lp_build_max(bld, a, min);
      }
         /**
   * Generate clamp(a, 0, 1)
   * A NaN will get converted to zero.
   */
   LLVMValueRef
   lp_build_clamp_zero_one_nanzero(struct lp_build_context *bld,
         {
      a = lp_build_max_ext(bld, a, bld->zero, GALLIVM_NAN_RETURN_OTHER_SECOND_NONNAN);
   a = lp_build_min(bld, a, bld->one);
      }
         /**
   * Generate abs(a)
   */
   LLVMValueRef
   lp_build_abs(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
                     if (!type.sign)
            if (type.floating) {
      char intrinsic[32];
   lp_format_intrinsic(intrinsic, sizeof intrinsic, "llvm.fabs", vec_type);
               if (type.width*type.length == 128 && util_get_cpu_caps()->has_ssse3 && LLVM_VERSION_MAJOR < 6) {
      switch(type.width) {
   case 8:
         case 16:
         case 32:
            }
   else if (type.width*type.length == 256 && util_get_cpu_caps()->has_avx2 && LLVM_VERSION_MAJOR < 6) {
      switch(type.width) {
   case 8:
         case 16:
         case 32:
                     return lp_build_select(bld, lp_build_cmp(bld, PIPE_FUNC_GREATER, a, bld->zero),
      }
         LLVMValueRef
   lp_build_negate(struct lp_build_context *bld,
         {
                        if (bld->type.floating)
         else
               }
         /** Return -1, 0 or +1 depending on the sign of a */
   LLVMValueRef
   lp_build_sgn(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMValueRef cond;
                     /* Handle non-zero case */
   if (!type.sign) {
      /* if not zero then sign must be positive */
      }
   else if (type.floating) {
      LLVMTypeRef vec_type;
   LLVMTypeRef int_type;
   LLVMValueRef mask;
   LLVMValueRef sign;
   LLVMValueRef one;
            int_type = lp_build_int_vec_type(bld->gallivm, type);
   vec_type = lp_build_vec_type(bld->gallivm, type);
            /* Take the sign bit and add it to 1 constant */
   sign = LLVMBuildBitCast(builder, a, int_type, "");
   sign = LLVMBuildAnd(builder, sign, mask, "");
   one = LLVMConstBitCast(bld->one, int_type);
   res = LLVMBuildOr(builder, sign, one, "");
      }
   else
   {
      /* signed int/norm/fixed point */
   /* could use psign with sse3 and appropriate vectors here */
   LLVMValueRef minus_one = lp_build_const_vec(bld->gallivm, type, -1.0);
   cond = lp_build_cmp(bld, PIPE_FUNC_GREATER, a, bld->zero);
               /* Handle zero */
   cond = lp_build_cmp(bld, PIPE_FUNC_EQUAL, a, bld->zero);
               }
         /**
   * Set the sign of float vector 'a' according to 'sign'.
   * If sign==0, return abs(a).
   * If sign==1, return -abs(a);
   * Other values for sign produce undefined results.
   */
   LLVMValueRef
   lp_build_set_sign(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(bld->gallivm, type);
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
   LLVMValueRef shift = lp_build_const_int_vec(bld->gallivm, type, type.width - 1);
   LLVMValueRef mask = lp_build_const_int_vec(bld->gallivm, type,
                  assert(type.floating);
            /* val = reinterpret_cast<int>(a) */
   val = LLVMBuildBitCast(builder, a, int_vec_type, "");
   /* val = val & mask */
   val = LLVMBuildAnd(builder, val, mask, "");
   /* sign = sign << shift */
   sign = LLVMBuildShl(builder, sign, shift, "");
   /* res = val | sign */
   res = LLVMBuildOr(builder, val, sign, "");
   /* res = reinterpret_cast<float>(res) */
               }
         /**
   * Convert vector of (or scalar) int to vector of (or scalar) float.
   */
   LLVMValueRef
   lp_build_int_to_float(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
                        }
         static bool
   arch_rounding_available(const struct lp_type type)
   {
      if ((util_get_cpu_caps()->has_sse4_1 &&
      (type.length == 1 || type.width*type.length == 128)) ||
   (util_get_cpu_caps()->has_avx && type.width*type.length == 256) ||
   (util_get_cpu_caps()->has_avx512f && type.width*type.length == 512))
      else if ((util_get_cpu_caps()->has_altivec &&
               else if (util_get_cpu_caps()->has_neon)
         else if (util_get_cpu_caps()->family == CPU_S390X)
               }
      enum lp_build_round_mode
   {
      LP_BUILD_ROUND_NEAREST = 0,
   LP_BUILD_ROUND_FLOOR = 1,
   LP_BUILD_ROUND_CEIL = 2,
      };
         static inline LLVMValueRef
   lp_build_iround_nearest_sse2(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef i32t = LLVMInt32TypeInContext(bld->gallivm->context);
   LLVMTypeRef ret_type = lp_build_int_vec_type(bld->gallivm, type);
   const char *intrinsic;
            assert(type.floating);
   /* using the double precision conversions is a bit more complicated */
            assert(lp_check_value(type, a));
            /* This is relying on MXCSR rounding mode, which should always be nearest. */
   if (type.length == 1) {
      LLVMTypeRef vec_type;
   LLVMValueRef undef;
   LLVMValueRef arg;
                                                res = lp_build_intrinsic_unary(builder, intrinsic,
      }
   else {
      if (type.width* type.length == 128) {
         }
   else {
                        }
   res = lp_build_intrinsic_unary(builder, intrinsic,
                  }
         /*
   */
   static inline LLVMValueRef
   lp_build_round_altivec(struct lp_build_context *bld,
               {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
                     assert(lp_check_value(type, a));
                     switch (mode) {
   case LP_BUILD_ROUND_NEAREST:
      intrinsic = "llvm.ppc.altivec.vrfin";
      case LP_BUILD_ROUND_FLOOR:
      intrinsic = "llvm.ppc.altivec.vrfim";
      case LP_BUILD_ROUND_CEIL:
      intrinsic = "llvm.ppc.altivec.vrfip";
      case LP_BUILD_ROUND_TRUNCATE:
      intrinsic = "llvm.ppc.altivec.vrfiz";
                  }
         static inline LLVMValueRef
   lp_build_round_arch(struct lp_build_context *bld,
               {
      if (util_get_cpu_caps()->has_sse4_1 || util_get_cpu_caps()->has_neon ||
      util_get_cpu_caps()->family == CPU_S390X) {
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   const char *intrinsic_root;
            assert(type.floating);
   assert(lp_check_value(type, a));
            switch (mode) {
   case LP_BUILD_ROUND_NEAREST:
      intrinsic_root = "llvm.nearbyint";
      case LP_BUILD_ROUND_FLOOR:
      intrinsic_root = "llvm.floor";
      case LP_BUILD_ROUND_CEIL:
      intrinsic_root = "llvm.ceil";
      case LP_BUILD_ROUND_TRUNCATE:
      intrinsic_root = "llvm.trunc";
      default:
                  lp_format_intrinsic(intrinsic, sizeof intrinsic, intrinsic_root, bld->vec_type);
      }
   else /* (util_get_cpu_caps()->has_altivec) */
      }
         /**
   * Return the integer part of a float (vector) value (== round toward zero).
   * The returned value is a float (vector).
   * Ex: trunc(-1.5) = -1.0
   */
   LLVMValueRef
   lp_build_trunc(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
            assert(type.floating);
            if (type.width == 16) {
      char intrinsic[64];
   lp_format_intrinsic(intrinsic, 64, "llvm.trunc", bld->vec_type);
               if (arch_rounding_available(type)) {
         }
   else {
      const struct lp_type type = bld->type;
   struct lp_type inttype;
   struct lp_build_context intbld;
   LLVMValueRef cmpval = lp_build_const_vec(bld->gallivm, type, 1<<24);
   LLVMValueRef trunc, res, anosign, mask;
   LLVMTypeRef int_vec_type = bld->int_vec_type;
            inttype = type;
   inttype.floating = 0;
            /* round by truncation */
   trunc = LLVMBuildFPToSI(builder, a, int_vec_type, "");
            /* mask out sign bit */
   anosign = lp_build_abs(bld, a);
   /*
   * mask out all values if anosign > 2^24
   * This should work both for large ints (all rounding is no-op for them
   * because such floats are always exact) as well as special cases like
   * NaNs, Infs (taking advantage of the fact they use max exponent).
   * (2^24 is arbitrary anything between 2^24 and 2^31 should work.)
   */
   anosign = LLVMBuildBitCast(builder, anosign, int_vec_type, "");
   cmpval = LLVMBuildBitCast(builder, cmpval, int_vec_type, "");
   mask = lp_build_cmp(&intbld, PIPE_FUNC_GREATER, anosign, cmpval);
         }
         /**
   * Return float (vector) rounded to nearest integer (vector).  The returned
   * value is a float (vector).
   * Ex: round(0.9) = 1.0
   * Ex: round(-1.5) = -2.0
   */
   LLVMValueRef
   lp_build_round(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
            assert(type.floating);
            if (type.width == 16) {
      char intrinsic[64];
   lp_format_intrinsic(intrinsic, 64, "llvm.round", bld->vec_type);
               if (arch_rounding_available(type)) {
         }
   else {
      const struct lp_type type = bld->type;
   struct lp_type inttype;
   struct lp_build_context intbld;
   LLVMValueRef cmpval = lp_build_const_vec(bld->gallivm, type, 1<<24);
   LLVMValueRef res, anosign, mask;
   LLVMTypeRef int_vec_type = bld->int_vec_type;
            inttype = type;
   inttype.floating = 0;
            res = lp_build_iround(bld, a);
            /* mask out sign bit */
   anosign = lp_build_abs(bld, a);
   /*
   * mask out all values if anosign > 2^24
   * This should work both for large ints (all rounding is no-op for them
   * because such floats are always exact) as well as special cases like
   * NaNs, Infs (taking advantage of the fact they use max exponent).
   * (2^24 is arbitrary anything between 2^24 and 2^31 should work.)
   */
   anosign = LLVMBuildBitCast(builder, anosign, int_vec_type, "");
   cmpval = LLVMBuildBitCast(builder, cmpval, int_vec_type, "");
   mask = lp_build_cmp(&intbld, PIPE_FUNC_GREATER, anosign, cmpval);
         }
         /**
   * Return floor of float (vector), result is a float (vector)
   * Ex: floor(1.1) = 1.0
   * Ex: floor(-1.1) = -2.0
   */
   LLVMValueRef
   lp_build_floor(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
            assert(type.floating);
            if (arch_rounding_available(type)) {
         }
   else {
      const struct lp_type type = bld->type;
   struct lp_type inttype;
   struct lp_build_context intbld;
   LLVMValueRef cmpval = lp_build_const_vec(bld->gallivm, type, 1<<24);
   LLVMValueRef trunc, res, anosign, mask;
   LLVMTypeRef int_vec_type = bld->int_vec_type;
            if (type.width != 32) {
      char intrinsic[32];
   lp_format_intrinsic(intrinsic, sizeof intrinsic, "llvm.floor", vec_type);
                        inttype = type;
   inttype.floating = 0;
            /* round by truncation */
   trunc = LLVMBuildFPToSI(builder, a, int_vec_type, "");
            if (type.sign) {
               /*
   * fix values if rounding is wrong (for non-special cases)
   * - this is the case if trunc > a
   */
   mask = lp_build_cmp(bld, PIPE_FUNC_GREATER, res, a);
   /* tmp = trunc > a ? 1.0 : 0.0 */
   tmp = LLVMBuildBitCast(builder, bld->one, int_vec_type, "");
   tmp = lp_build_and(&intbld, mask, tmp);
   tmp = LLVMBuildBitCast(builder, tmp, vec_type, "");
               /* mask out sign bit */
   anosign = lp_build_abs(bld, a);
   /*
   * mask out all values if anosign > 2^24
   * This should work both for large ints (all rounding is no-op for them
   * because such floats are always exact) as well as special cases like
   * NaNs, Infs (taking advantage of the fact they use max exponent).
   * (2^24 is arbitrary anything between 2^24 and 2^31 should work.)
   */
   anosign = LLVMBuildBitCast(builder, anosign, int_vec_type, "");
   cmpval = LLVMBuildBitCast(builder, cmpval, int_vec_type, "");
   mask = lp_build_cmp(&intbld, PIPE_FUNC_GREATER, anosign, cmpval);
         }
         /**
   * Return ceiling of float (vector), returning float (vector).
   * Ex: ceil( 1.1) = 2.0
   * Ex: ceil(-1.1) = -1.0
   */
   LLVMValueRef
   lp_build_ceil(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
            assert(type.floating);
            if (arch_rounding_available(type)) {
         }
   else {
      const struct lp_type type = bld->type;
   struct lp_type inttype;
   struct lp_build_context intbld;
   LLVMValueRef cmpval = lp_build_const_vec(bld->gallivm, type, 1<<24);
   LLVMValueRef trunc, res, anosign, mask, tmp;
   LLVMTypeRef int_vec_type = bld->int_vec_type;
            if (type.width != 32) {
      char intrinsic[32];
   lp_format_intrinsic(intrinsic, sizeof intrinsic, "llvm.ceil", vec_type);
                        inttype = type;
   inttype.floating = 0;
            /* round by truncation */
   trunc = LLVMBuildFPToSI(builder, a, int_vec_type, "");
            /*
   * fix values if rounding is wrong (for non-special cases)
   * - this is the case if trunc < a
   */
   mask = lp_build_cmp(bld, PIPE_FUNC_LESS, trunc, a);
   /* tmp = trunc < a ? 1.0 : 0.0 */
   tmp = LLVMBuildBitCast(builder, bld->one, int_vec_type, "");
   tmp = lp_build_and(&intbld, mask, tmp);
   tmp = LLVMBuildBitCast(builder, tmp, vec_type, "");
            /* mask out sign bit */
   anosign = lp_build_abs(bld, a);
   /*
   * mask out all values if anosign > 2^24
   * This should work both for large ints (all rounding is no-op for them
   * because such floats are always exact) as well as special cases like
   * NaNs, Infs (taking advantage of the fact they use max exponent).
   * (2^24 is arbitrary anything between 2^24 and 2^31 should work.)
   */
   anosign = LLVMBuildBitCast(builder, anosign, int_vec_type, "");
   cmpval = LLVMBuildBitCast(builder, cmpval, int_vec_type, "");
   mask = lp_build_cmp(&intbld, PIPE_FUNC_GREATER, anosign, cmpval);
         }
         /**
   * Return fractional part of 'a' computed as a - floor(a)
   * Typically used in texture coord arithmetic.
   */
   LLVMValueRef
   lp_build_fract(struct lp_build_context *bld,
         {
      assert(bld->type.floating);
      }
         /**
   * Prevent returning 1.0 for very small negative values of 'a' by clamping
   * against 0.99999(9). (Will also return that value for NaNs.)
   */
   static inline LLVMValueRef
   clamp_fract(struct lp_build_context *bld, LLVMValueRef fract)
   {
               /* this is the largest number smaller than 1.0 representable as float */
   max = lp_build_const_vec(bld->gallivm, bld->type,
         return lp_build_min_ext(bld, fract, max,
      }
         /**
   * Same as lp_build_fract, but guarantees that the result is always smaller
   * than one. Will also return the smaller-than-one value for infs, NaNs.
   */
   LLVMValueRef
   lp_build_fract_safe(struct lp_build_context *bld,
         {
         }
         /**
   * Return the integer part of a float (vector) value (== round toward zero).
   * The returned value is an integer (vector).
   * Ex: itrunc(-1.5) = -1
   */
   LLVMValueRef
   lp_build_itrunc(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
            assert(type.floating);
               }
         /**
   * Return float (vector) rounded to nearest integer (vector).  The returned
   * value is an integer (vector).
   * Ex: iround(0.9) = 1
   * Ex: iround(-1.5) = -2
   */
   LLVMValueRef
   lp_build_iround(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = bld->int_vec_type;
                              if ((util_get_cpu_caps()->has_sse2 &&
      ((type.width == 32) && (type.length == 1 || type.length == 4))) ||
   (util_get_cpu_caps()->has_avx && type.width == 32 && type.length == 8)) {
      }
   if (arch_rounding_available(type)) {
         }
   else {
                        if (type.sign) {
      LLVMTypeRef vec_type = bld->vec_type;
   LLVMValueRef mask = lp_build_const_int_vec(bld->gallivm, type,
                  /* get sign bit */
                  /* sign * 0.5 */
   half = LLVMBuildBitCast(builder, half, int_vec_type, "");
   half = LLVMBuildOr(builder, sign, half, "");
                                       }
         /**
   * Return floor of float (vector), result is an int (vector)
   * Ex: ifloor(1.1) = 1.0
   * Ex: ifloor(-1.1) = -2.0
   */
   LLVMValueRef
   lp_build_ifloor(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = bld->int_vec_type;
            assert(type.floating);
            res = a;
   if (type.sign) {
      if (arch_rounding_available(type)) {
         }
   else {
      struct lp_type inttype;
                                 inttype = type;
                  /* round by truncation */
                  /*
   * fix values if rounding is wrong (for non-special cases)
   * - this is the case if trunc > a
   * The results of doing this with NaNs, very large values etc.
   * are undefined but this seems to be the case anyway.
   */
   mask = lp_build_cmp(bld, PIPE_FUNC_GREATER, trunc, a);
   /* cheapie minus one with mask since the mask is minus one / zero */
                  /* round to nearest (toward zero) */
               }
         /**
   * Return ceiling of float (vector), returning int (vector).
   * Ex: iceil( 1.1) = 2
   * Ex: iceil(-1.1) = -1
   */
   LLVMValueRef
   lp_build_iceil(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = bld->int_vec_type;
            assert(type.floating);
            if (arch_rounding_available(type)) {
         }
   else {
      struct lp_type inttype;
   struct lp_build_context intbld;
            assert(type.floating);
            inttype = type;
   inttype.floating = 0;
            /* round by truncation */
   itrunc = LLVMBuildFPToSI(builder, a, int_vec_type, "");
            /*
   * fix values if rounding is wrong (for non-special cases)
   * - this is the case if trunc < a
   * The results of doing this with NaNs, very large values etc.
   * are undefined but this seems to be the case anyway.
   */
   mask = lp_build_cmp(bld, PIPE_FUNC_LESS, trunc, a);
   /* cheapie plus one with mask since the mask is minus one / zero */
               /* round to nearest (toward zero) */
               }
         /**
   * Combined ifloor() & fract().
   *
   * Preferred to calling the functions separately, as it will ensure that the
   * strategy (floor() vs ifloor()) that results in less redundant work is used.
   */
   void
   lp_build_ifloor_fract(struct lp_build_context *bld,
                     {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
            assert(type.floating);
            if (arch_rounding_available(type)) {
      /*
   * floor() is easier.
            ipart = lp_build_floor(bld, a);
   *out_fpart = LLVMBuildFSub(builder, a, ipart, "fpart");
      }
   else {
      /*
   * ifloor() is easier.
            *out_ipart = lp_build_ifloor(bld, a);
   ipart = LLVMBuildSIToFP(builder, *out_ipart, bld->vec_type, "ipart");
         }
         /**
   * Same as lp_build_ifloor_fract, but guarantees that the fractional part is
   * always smaller than one.
   */
   void
   lp_build_ifloor_fract_safe(struct lp_build_context *bld,
                     {
      lp_build_ifloor_fract(bld, a, out_ipart, out_fpart);
      }
         LLVMValueRef
   lp_build_sqrt(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
                     assert(type.floating);
               }
         /**
   * Do one Newton-Raphson step to improve reciprocate precision:
   *
   *   x_{i+1} = x_i + x_i * (1 - a * x_i)
   *
   * XXX: Unfortunately this won't give IEEE-754 conformant results for 0 or
   * +/-Inf, giving NaN instead.  Certain applications rely on this behavior,
   * such as Google Earth, which does RCP(RSQRT(0.0)) when drawing the Earth's
   * halo. It would be necessary to clamp the argument to prevent this.
   *
   * See also:
   * - http://en.wikipedia.org/wiki/Division_(digital)#Newton.E2.80.93Raphson_division
   * - http://softwarecommunity.intel.com/articles/eng/1818.htm
   */
   static inline LLVMValueRef
   lp_build_rcp_refine(struct lp_build_context *bld,
               {
      LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef neg_a;
            neg_a = LLVMBuildFNeg(builder, a, "");
   res = lp_build_fmuladd(builder, neg_a, rcp_a, bld->one);
               }
         LLVMValueRef
   lp_build_rcp(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
                     if (a == bld->zero)
         if (a == bld->one)
         if (a == bld->undef)
                     if (LLVMIsConstant(a))
            /*
   * We don't use RCPPS because:
   * - it only has 10bits of precision
   * - it doesn't even get the reciprocate of 1.0 exactly
   * - doing Newton-Rapshon steps yields wrong (NaN) values for 0.0 or Inf
   * - for recent processors the benefit over DIVPS is marginal, a case
   *   dependent
   *
   * We could still use it on certain processors if benchmarks show that the
   * RCPPS plus necessary workarounds are still preferrable to DIVPS; or for
   * particular uses that require less workarounds.
            if (false && ((util_get_cpu_caps()->has_sse && type.width == 32 && type.length == 4) ||
            const unsigned num_iterations = 0;
   LLVMValueRef res;
   unsigned i;
            if (type.length == 4) {
         }
   else {
                           for (i = 0; i < num_iterations; ++i) {
                                 }
         /**
   * Do one Newton-Raphson step to improve rsqrt precision:
   *
   *   x_{i+1} = 0.5 * x_i * (3.0 - a * x_i * x_i)
   *
   * See also Intel 64 and IA-32 Architectures Optimization Manual.
   */
   static inline LLVMValueRef
   lp_build_rsqrt_refine(struct lp_build_context *bld,
               {
      LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef half = lp_build_const_vec(bld->gallivm, bld->type, 0.5);
   LLVMValueRef three = lp_build_const_vec(bld->gallivm, bld->type, 3.0);
            res = LLVMBuildFMul(builder, rsqrt_a, rsqrt_a, "");
   res = LLVMBuildFMul(builder, a, res, "");
   res = LLVMBuildFSub(builder, three, res, "");
   res = LLVMBuildFMul(builder, rsqrt_a, res, "");
               }
         /**
   * Generate 1/sqrt(a).
   * Result is undefined for values < 0, infinity for +0.
   */
   LLVMValueRef
   lp_build_rsqrt(struct lp_build_context *bld,
         {
                                 /*
   * This should be faster but all denormals will end up as infinity.
   */
   if (0 && lp_build_fast_rsqrt_available(type)) {
      const unsigned num_iterations = 1;
   LLVMValueRef res;
            /* rsqrt(1.0) != 1.0 here */
            if (num_iterations) {
      /*
   * Newton-Raphson will result in NaN instead of infinity for zero,
   * and NaN instead of zero for infinity.
   * Also, need to ensure rsqrt(1.0) == 1.0.
   * All numbers smaller than FLT_MIN will result in +infinity
   * (rsqrtps treats all denormals as zero).
   */
   LLVMValueRef cmp;
                  for (i = 0; i < num_iterations; ++i) {
         }
   cmp = lp_build_compare(bld->gallivm, type, PIPE_FUNC_LESS, a, flt_min);
   res = lp_build_select(bld, cmp, inf, res);
   cmp = lp_build_compare(bld->gallivm, type, PIPE_FUNC_EQUAL, a, inf);
   res = lp_build_select(bld, cmp, bld->zero, res);
   cmp = lp_build_compare(bld->gallivm, type, PIPE_FUNC_EQUAL, a, bld->one);
                              }
         /**
   * If there's a fast (inaccurate) rsqrt instruction available
   * (caller may want to avoid to call rsqrt_fast if it's not available,
   * i.e. for calculating x^0.5 it may do rsqrt_fast(x) * x but if
   * unavailable it would result in sqrt/div/mul so obviously
   * much better to just call sqrt, skipping both div and mul).
   */
   bool
   lp_build_fast_rsqrt_available(struct lp_type type)
   {
               if ((util_get_cpu_caps()->has_sse && type.width == 32 && type.length == 4) ||
      (util_get_cpu_caps()->has_avx && type.width == 32 && type.length == 8)) {
      }
      }
         /**
   * Generate 1/sqrt(a).
   * Result is undefined for values < 0, infinity for +0.
   * Precision is limited, only ~10 bits guaranteed
   * (rsqrt 1.0 may not be 1.0, denorms may be flushed to 0).
   */
   LLVMValueRef
   lp_build_fast_rsqrt(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
                     if (lp_build_fast_rsqrt_available(type)) {
               if (type.length == 4) {
         }
   else {
         }
      }
   else {
         }
      }
         /**
   * Generate sin(a) or cos(a) using polynomial approximation.
   * TODO: it might be worth recognizing sin and cos using same source
   * (i.e. d3d10 sincos opcode). Obviously doing both at the same time
   * would be way cheaper than calculating (nearly) everything twice...
   * Not sure it's common enough to be worth bothering however, scs
   * opcode could also benefit from calculating both though.
   */
   static LLVMValueRef
   lp_build_sin_or_cos(struct lp_build_context *bld,
               {
      struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef b = gallivm->builder;
            /*
   *  take the absolute value,
   *  x = _mm_and_ps(x, *(v4sf*)_ps_inv_sign_mask);
            LLVMValueRef inv_sig_mask = lp_build_const_int_vec(gallivm, bld->type, ~0x80000000);
            LLVMValueRef absi = LLVMBuildAnd(b, a_v4si, inv_sig_mask, "absi");
            /*
   * scale by 4/Pi
   * y = _mm_mul_ps(x, *(v4sf*)_ps_cephes_FOPI);
            LLVMValueRef FOPi = lp_build_const_vec(gallivm, bld->type, 1.27323954473516);
            /*
   * store the integer part of y in mm0
   * emm2 = _mm_cvttps_epi32(y);
                     /*
   * j=(j+1) & (~1) (see the cephes sources)
   * emm2 = _mm_add_epi32(emm2, *(v4si*)_pi32_1);
            LLVMValueRef all_one = lp_build_const_int_vec(gallivm, bld->type, 1);
   LLVMValueRef emm2_add =  LLVMBuildAdd(b, emm2_i, all_one, "emm2_add");
   /*
   * emm2 = _mm_and_si128(emm2, *(v4si*)_pi32_inv1);
   */
   LLVMValueRef inv_one = lp_build_const_int_vec(gallivm, bld->type, ~1);
            /*
   * y = _mm_cvtepi32_ps(emm2);
   */
            LLVMValueRef const_2 = lp_build_const_int_vec(gallivm, bld->type, 2);
   LLVMValueRef const_4 = lp_build_const_int_vec(gallivm, bld->type, 4);
   LLVMValueRef const_29 = lp_build_const_int_vec(gallivm, bld->type, 29);
            /*
   * Argument used for poly selection and sign bit determination
   * is different for sin vs. cos.
   */
   LLVMValueRef emm2_2 = cos ? LLVMBuildSub(b, emm2_and, const_2, "emm2_2") :
            LLVMValueRef sign_bit = cos ? LLVMBuildShl(b, LLVMBuildAnd(b, const_4,
                                          /*
   * get the polynom selection mask
   * there is one polynom for 0 <= x <= Pi/4
   * and another one for Pi/4<x<=Pi/2
   * Both branches will be computed.
   *
   * emm2 = _mm_and_si128(emm2, *(v4si*)_pi32_2);
   * emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());
            LLVMValueRef emm2_3 =  LLVMBuildAnd(b, emm2_2, const_2, "emm2_3");
   LLVMValueRef poly_mask = lp_build_compare(gallivm,
                  /*
   * _PS_CONST(minus_cephes_DP1, -0.78515625);
   * _PS_CONST(minus_cephes_DP2, -2.4187564849853515625e-4);
   * _PS_CONST(minus_cephes_DP3, -3.77489497744594108e-8);
   */
   LLVMValueRef DP1 = lp_build_const_vec(gallivm, bld->type, -0.78515625);
   LLVMValueRef DP2 = lp_build_const_vec(gallivm, bld->type, -2.4187564849853515625e-4);
            /*
   * The magic pass: "Extended precision modular arithmetic"
   * x = ((x - y * DP1) - y * DP2) - y * DP3;
   */
   LLVMValueRef x_1 = lp_build_fmuladd(b, y_2, DP1, x_abs);
   LLVMValueRef x_2 = lp_build_fmuladd(b, y_2, DP2, x_1);
            /*
   * Evaluate the first polynom  (0 <= x <= Pi/4)
   *
   * z = _mm_mul_ps(x,x);
   */
            /*
   * _PS_CONST(coscof_p0,  2.443315711809948E-005);
   * _PS_CONST(coscof_p1, -1.388731625493765E-003);
   * _PS_CONST(coscof_p2,  4.166664568298827E-002);
   */
   LLVMValueRef coscof_p0 = lp_build_const_vec(gallivm, bld->type, 2.443315711809948E-005);
   LLVMValueRef coscof_p1 = lp_build_const_vec(gallivm, bld->type, -1.388731625493765E-003);
            /*
   * y = *(v4sf*)_ps_coscof_p0;
   * y = _mm_mul_ps(y, z);
   */
   LLVMValueRef y_4 = lp_build_fmuladd(b, z, coscof_p0, coscof_p1);
   LLVMValueRef y_6 = lp_build_fmuladd(b, y_4, z, coscof_p2);
   LLVMValueRef y_7 = LLVMBuildFMul(b, y_6, z, "y_7");
               /*
   * tmp = _mm_mul_ps(z, *(v4sf*)_ps_0p5);
   * y = _mm_sub_ps(y, tmp);
   * y = _mm_add_ps(y, *(v4sf*)_ps_1);
   */
   LLVMValueRef half = lp_build_const_vec(gallivm, bld->type, 0.5);
   LLVMValueRef tmp = LLVMBuildFMul(b, z, half, "tmp");
   LLVMValueRef y_9 = LLVMBuildFSub(b, y_8, tmp, "y_8");
   LLVMValueRef one = lp_build_const_vec(gallivm, bld->type, 1.0);
            /*
   * _PS_CONST(sincof_p0, -1.9515295891E-4);
   * _PS_CONST(sincof_p1,  8.3321608736E-3);
   * _PS_CONST(sincof_p2, -1.6666654611E-1);
   */
   LLVMValueRef sincof_p0 = lp_build_const_vec(gallivm, bld->type, -1.9515295891E-4);
   LLVMValueRef sincof_p1 = lp_build_const_vec(gallivm, bld->type, 8.3321608736E-3);
            /*
   * Evaluate the second polynom  (Pi/4 <= x <= 0)
   *
   * y2 = *(v4sf*)_ps_sincof_p0;
   * y2 = _mm_mul_ps(y2, z);
   * y2 = _mm_add_ps(y2, *(v4sf*)_ps_sincof_p1);
   * y2 = _mm_mul_ps(y2, z);
   * y2 = _mm_add_ps(y2, *(v4sf*)_ps_sincof_p2);
   * y2 = _mm_mul_ps(y2, z);
   * y2 = _mm_mul_ps(y2, x);
   * y2 = _mm_add_ps(y2, x);
            LLVMValueRef y2_4 = lp_build_fmuladd(b, z, sincof_p0, sincof_p1);
   LLVMValueRef y2_6 = lp_build_fmuladd(b, y2_4, z, sincof_p2);
   LLVMValueRef y2_7 = LLVMBuildFMul(b, y2_6, z, "y2_7");
            /*
   * select the correct result from the two polynoms
   * xmm3 = poly_mask;
   * y2 = _mm_and_ps(xmm3, y2); //, xmm3);
   * y = _mm_andnot_ps(xmm3, y);
   * y = _mm_or_ps(y,y2);
   */
   LLVMValueRef y2_i = LLVMBuildBitCast(b, y2_9, bld->int_vec_type, "y2_i");
   LLVMValueRef y_i = LLVMBuildBitCast(b, y_10, bld->int_vec_type, "y_i");
   LLVMValueRef y2_and = LLVMBuildAnd(b, y2_i, poly_mask, "y2_and");
   LLVMValueRef poly_mask_inv = LLVMBuildNot(b, poly_mask, "poly_mask_inv");
   LLVMValueRef y_and = LLVMBuildAnd(b, y_i, poly_mask_inv, "y_and");
            /*
   * update the sign
   * y = _mm_xor_ps(y, sign_bit);
   */
   LLVMValueRef y_sign = LLVMBuildXor(b, y_combine, sign_bit, "y_sign");
                     /* clamp output to be within [-1, 1] */
   y_result = lp_build_clamp(bld, y_result,
               /* If a is -inf, inf or NaN then return NaN */
   y_result = lp_build_select(bld, isfinite, y_result,
            }
         /**
   * Generate sin(a)
   */
   LLVMValueRef
   lp_build_sin(struct lp_build_context *bld,
         {
               if (type.width == 16) {
      LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
   char intrinsic[32];
   lp_format_intrinsic(intrinsic, sizeof intrinsic, "llvm.sin", vec_type);
   LLVMValueRef args[] = { a };
                  }
         /**
   * Generate cos(a)
   */
   LLVMValueRef
   lp_build_cos(struct lp_build_context *bld,
         {
               if (type.width == 16) {
      LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
   char intrinsic[32];
   lp_format_intrinsic(intrinsic, sizeof intrinsic, "llvm.cos", vec_type);
   LLVMValueRef args[] = { a };
                  }
         /**
   * Generate pow(x, y)
   */
   LLVMValueRef
   lp_build_pow(struct lp_build_context *bld,
               {
      /* TODO: optimize the constant case */
   if (gallivm_debug & GALLIVM_DEBUG_PERF &&
      LLVMIsConstant(x) && LLVMIsConstant(y)) {
   debug_printf("%s: inefficient/imprecise constant arithmetic\n",
               LLVMValueRef cmp = lp_build_cmp(bld, PIPE_FUNC_EQUAL, x, lp_build_const_vec(bld->gallivm, bld->type, 0.0f));
            res = lp_build_select(bld, cmp, lp_build_const_vec(bld->gallivm, bld->type, 0.0f), res);
      }
         /**
   * Generate exp(x)
   */
   LLVMValueRef
   lp_build_exp(struct lp_build_context *bld,
         {
      /* log2(e) = 1/log(2) */
   LLVMValueRef log2e = lp_build_const_vec(bld->gallivm, bld->type,
                        }
         /**
   * Generate log(x)
   * Behavior is undefined with infs, 0s and nans
   */
   LLVMValueRef
   lp_build_log(struct lp_build_context *bld,
         {
      /* log(2) */
   LLVMValueRef log2 = lp_build_const_vec(bld->gallivm, bld->type,
                        }
         /**
   * Generate log(x) that handles edge cases (infs, 0s and nans)
   */
   LLVMValueRef
   lp_build_log_safe(struct lp_build_context *bld,
         {
      /* log(2) */
   LLVMValueRef log2 = lp_build_const_vec(bld->gallivm, bld->type,
                        }
         /**
   * Generate polynomial.
   * Ex:  coeffs[0] + x * coeffs[1] + x^2 * coeffs[2].
   */
   LLVMValueRef
   lp_build_polynomial(struct lp_build_context *bld,
                     {
      const struct lp_type type = bld->type;
   LLVMValueRef even = NULL, odd = NULL;
   LLVMValueRef x2;
                     /* TODO: optimize the constant case */
   if (gallivm_debug & GALLIVM_DEBUG_PERF &&
      LLVMIsConstant(x)) {
   debug_printf("%s: inefficient/imprecise constant arithmetic\n",
               /*
   * Calculate odd and even terms seperately to decrease data dependency
   * Ex:
   *     c[0] + x^2 * c[2] + x^4 * c[4] ...
   *     + x * (c[1] + x^2 * c[3] + x^4 * c[5]) ...
   */
            for (i = num_coeffs; i--; ) {
                        if (i % 2 == 0) {
      if (even)
         else
      } else {
      if (odd)
         else
                  if (odd)
         else if (even)
         else
      }
         /**
   * Minimax polynomial fit of 2**x, in range [0, 1[
   */
   static const double lp_build_exp2_polynomial[] = {
   #if EXP_POLY_DEGREE == 5
      1.000000000000000000000, /*XXX: was 0.999999925063526176901, recompute others */
   0.693153073200168932794,
   0.240153617044375388211,
   0.0558263180532956664775,
   0.00898934009049466391101,
      #elif EXP_POLY_DEGREE == 4
      1.00000259337069434683,
   0.693003834469974940458,
   0.24144275689150793076,
   0.0520114606103070150235,
      #elif EXP_POLY_DEGREE == 3
      0.999925218562710312959,
   0.695833540494823811697,
   0.226067155427249155588,
      #elif EXP_POLY_DEGREE == 2
      1.00172476321474503578,
   0.657636275736077639316,
      #else
   #error
   #endif
   };
         LLVMValueRef
   lp_build_exp2(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
   LLVMValueRef ipart = NULL;
   LLVMValueRef fpart = NULL;
   LLVMValueRef expipart = NULL;
   LLVMValueRef expfpart = NULL;
            if (type.floating && type.width == 16) {
      char intrinsic[32];
   lp_format_intrinsic(intrinsic, sizeof intrinsic, "llvm.exp2", vec_type);
   LLVMValueRef args[] = { x };
                        /* TODO: optimize the constant case */
   if (gallivm_debug & GALLIVM_DEBUG_PERF &&
      LLVMIsConstant(x)) {
   debug_printf("%s: inefficient/imprecise constant arithmetic\n",
                        /* We want to preserve NaN and make sure than for exp2 if x > 128,
   * the result is INF  and if it's smaller than -126.9 the result is 0 */
   x = lp_build_min_ext(bld, lp_build_const_vec(bld->gallivm, type,  128.0), x,
         x = lp_build_max_ext(bld, lp_build_const_vec(bld->gallivm, type, -126.99999),
            /* ipart = floor(x) */
   /* fpart = x - ipart */
            /* expipart = (float) (1 << ipart) */
   expipart = LLVMBuildAdd(builder, ipart,
         expipart = LLVMBuildShl(builder, expipart,
                  expfpart = lp_build_polynomial(bld, fpart, lp_build_exp2_polynomial,
                        }
         /**
   * Extract the exponent of a IEEE-754 floating point value.
   *
   * Optionally apply an integer bias.
   *
   * Result is an integer value with
   *
   *   ifloor(log2(x)) + bias
   */
   LLVMValueRef
   lp_build_extract_exponent(struct lp_build_context *bld,
               {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   unsigned mantissa = lp_mantissa(type);
                                       res = LLVMBuildLShr(builder, x,
         res = LLVMBuildAnd(builder, res,
         res = LLVMBuildSub(builder, res,
               }
         /**
   * Extract the mantissa of the a floating.
   *
   * Result is a floating point value with
   *
   *   x / floor(log2(x))
   */
   LLVMValueRef
   lp_build_extract_mantissa(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   unsigned mantissa = lp_mantissa(type);
   LLVMValueRef mantmask = lp_build_const_int_vec(bld->gallivm, type,
         LLVMValueRef one = LLVMConstBitCast(bld->one, bld->int_vec_type);
                                       /* res = x / 2**ipart */
   res = LLVMBuildAnd(builder, x, mantmask, "");
   res = LLVMBuildOr(builder, res, one, "");
               }
            /**
   * Minimax polynomial fit of log2((1.0 + sqrt(x))/(1.0 - sqrt(x)))/sqrt(x) ,for x in range of [0, 1/9[
   * These coefficients can be generate with
   * http://www.boost.org/doc/libs/1_36_0/libs/math/doc/sf_and_dist/html/math_toolkit/toolkit/internals2/minimax.html
   */
   static const double lp_build_log2_polynomial[] = {
   #if LOG_POLY_DEGREE == 5
      2.88539008148777786488L,
   0.961796878841293367824L,
   0.577058946784739859012L,
   0.412914355135828735411L,
   0.308591899232910175289L,
      #elif LOG_POLY_DEGREE == 4
      2.88539009343309178325L,
   0.961791550404184197881L,
   0.577440339438736392009L,
   0.403343858251329912514L,
      #elif LOG_POLY_DEGREE == 3
      2.88538959748872753838L,
   0.961932915889597772928L,
   0.571118517972136195241L,
      #else
   #error
   #endif
   };
         /**
   * See http://www.devmaster.net/forums/showthread.php?p=43580
   * http://en.wikipedia.org/wiki/Logarithm#Calculation
   * http://www.nezumi.demon.co.uk/consult/logx.htm
   *
   * If handle_edge_cases is true the function will perform computations
   * to match the required D3D10+ behavior for each of the edge cases.
   * That means that if input is:
   * - less than zero (to and including -inf) then NaN will be returned
   * - equal to zero (-denorm, -0, +0 or +denorm), then -inf will be returned
   * - +infinity, then +infinity will be returned
   * - NaN, then NaN will be returned
   *
   * Those checks are fairly expensive so if you don't need them make sure
   * handle_edge_cases is false.
   */
   void
   lp_build_log2_approx(struct lp_build_context *bld,
                        LLVMValueRef x,
      {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(bld->gallivm, type);
            LLVMValueRef expmask = lp_build_const_int_vec(bld->gallivm, type, 0x7f800000);
   LLVMValueRef mantmask = lp_build_const_int_vec(bld->gallivm, type, 0x007fffff);
            LLVMValueRef i = NULL;
   LLVMValueRef y = NULL;
   LLVMValueRef z = NULL;
   LLVMValueRef exp = NULL;
   LLVMValueRef mant = NULL;
   LLVMValueRef logexp = NULL;
   LLVMValueRef p_z = NULL;
            if (bld->type.width == 16) {
      char intrinsic[32];
   lp_format_intrinsic(intrinsic, sizeof intrinsic, "llvm.log2", bld->vec_type);
   LLVMValueRef args[] = { x };
   if (p_log2)
                              if (p_exp || p_floor_log2 || p_log2) {
      /* TODO: optimize the constant case */
   if (gallivm_debug & GALLIVM_DEBUG_PERF &&
      LLVMIsConstant(x)) {
   debug_printf("%s: inefficient/imprecise constant arithmetic\n",
                        /*
   * We don't explicitly handle denormalized numbers. They will yield a
   * result in the neighbourhood of -127, which appears to be adequate
   * enough.
                     /* exp = (float) exponent(x) */
               if (p_floor_log2 || p_log2) {
      logexp = LLVMBuildLShr(builder, exp, lp_build_const_int_vec(bld->gallivm, type, 23), "");
   logexp = LLVMBuildSub(builder, logexp, lp_build_const_int_vec(bld->gallivm, type, 127), "");
               if (p_log2) {
      /* mant = 1 + (float) mantissa(x) */
   mant = LLVMBuildAnd(builder, i, mantmask, "");
   mant = LLVMBuildOr(builder, mant, one, "");
            /* y = (mant - 1) / (mant + 1) */
   y = lp_build_div(bld,
                  /* z = y^2 */
            /* compute P(z) */
   p_z = lp_build_polynomial(bld, z, lp_build_log2_polynomial,
            /* y * P(z) + logexp */
            if (type.floating && handle_edge_cases) {
      LLVMValueRef negmask, infmask,  zmask;
   negmask = lp_build_cmp(bld, PIPE_FUNC_LESS, x,
         zmask = lp_build_cmp(bld, PIPE_FUNC_EQUAL, x,
                        /* If x is qual to inf make sure we return inf */
   res = lp_build_select(bld, infmask,
               /* If x is qual to 0, return -inf */
   res = lp_build_select(bld, zmask,
               /* If x is nan or less than 0, return nan */
   res = lp_build_select(bld, negmask,
                        if (p_exp) {
      exp = LLVMBuildBitCast(builder, exp, vec_type, "");
               if (p_floor_log2)
            if (p_log2)
      }
         /*
   * log2 implementation which doesn't have special code to
   * handle edge cases (-inf, 0, inf, NaN). It's faster but
   * the results for those cases are undefined.
   */
   LLVMValueRef
   lp_build_log2(struct lp_build_context *bld,
         {
      LLVMValueRef res;
   lp_build_log2_approx(bld, x, NULL, NULL, &res, false);
      }
         /*
   * Version of log2 which handles all edge cases.
   * Look at documentation of lp_build_log2_approx for
   * description of the behavior for each of the edge cases.
   */
   LLVMValueRef
   lp_build_log2_safe(struct lp_build_context *bld,
         {
      LLVMValueRef res;
   lp_build_log2_approx(bld, x, NULL, NULL, &res, true);
      }
         /**
   * Faster (and less accurate) log2.
   *
   *    log2(x) = floor(log2(x)) - 1 + x / 2**floor(log2(x))
   *
   * Piece-wise linear approximation, with exact results when x is a
   * power of two.
   *
   * See http://www.flipcode.com/archives/Fast_log_Function.shtml
   */
   LLVMValueRef
   lp_build_fast_log2(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef ipart;
                              /* ipart = floor(log2(x)) - 1 */
   ipart = lp_build_extract_exponent(bld, x, -1);
            /* fpart = x / 2**ipart */
            /* ipart + fpart */
      }
         /**
   * Fast implementation of iround(log2(x)).
   *
   * Not an approximation -- it should give accurate results all the time.
   */
   LLVMValueRef
   lp_build_ilog2(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef sqrt2 = lp_build_const_vec(bld->gallivm, bld->type, M_SQRT2);
                              /* x * 2^(0.5)   i.e., add 0.5 to the log2(x) */
            /* ipart = floor(log2(x) + 0.5)  */
               }
      LLVMValueRef
   lp_build_mod(struct lp_build_context *bld,
               {
      LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef res;
            assert(lp_check_value(type, x));
            if (type.floating)
         else if (type.sign)
         else
            }
         /*
   * For floating inputs it creates and returns a mask
   * which is all 1's for channels which are NaN.
   * Channels inside x which are not NaN will be 0.
   */
   LLVMValueRef
   lp_build_isnan(struct lp_build_context *bld,
         {
      LLVMValueRef mask;
            assert(bld->type.floating);
            mask = LLVMBuildFCmp(bld->gallivm->builder, LLVMRealOEQ, x, x,
         mask = LLVMBuildNot(bld->gallivm->builder, mask, "");
   mask = LLVMBuildSExt(bld->gallivm->builder, mask, int_vec_type, "isnan");
      }
         /* Returns all 1's for floating point numbers that are
   * finite numbers and returns all zeros for -inf,
   * inf and nan's */
   LLVMValueRef
   lp_build_isfinite(struct lp_build_context *bld,
         {
      LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(bld->gallivm, bld->type);
   struct lp_type int_type = lp_int_type(bld->type);
   LLVMValueRef intx = LLVMBuildBitCast(builder, x, int_vec_type, "");
   LLVMValueRef infornan32 = lp_build_const_int_vec(bld->gallivm, bld->type,
            if (!bld->type.floating) {
         }
   assert(bld->type.floating);
   assert(lp_check_value(bld->type, x));
            intx = LLVMBuildAnd(builder, intx, infornan32, "");
   return lp_build_compare(bld->gallivm, int_type, PIPE_FUNC_NOTEQUAL,
      }
         /*
   * Returns true if the number is nan or inf and false otherwise.
   * The input has to be a floating point vector.
   */
   LLVMValueRef
   lp_build_is_inf_or_nan(struct gallivm_state *gallivm,
               {
      LLVMBuilderRef builder = gallivm->builder;
   struct lp_type int_type = lp_int_type(type);
   LLVMValueRef const0 = lp_build_const_int_vec(gallivm, int_type,
                           ret = LLVMBuildBitCast(builder, x, lp_build_vec_type(gallivm, int_type), "");
   ret = LLVMBuildAnd(builder, ret, const0, "");
   ret = lp_build_compare(gallivm, int_type, PIPE_FUNC_EQUAL,
               }
         LLVMValueRef
   lp_build_fpstate_get(struct gallivm_state *gallivm)
   {
      if (util_get_cpu_caps()->has_sse) {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef mxcsr_ptr = lp_build_alloca(
      gallivm,
   LLVMInt32TypeInContext(gallivm->context),
      LLVMValueRef mxcsr_ptr8 = LLVMBuildPointerCast(builder, mxcsr_ptr,
         lp_build_intrinsic(builder,
                        }
      }
      void
   lp_build_fpstate_set_denorms_zero(struct gallivm_state *gallivm,
         {
      if (util_get_cpu_caps()->has_sse) {
      /* turn on DAZ (64) | FTZ (32768) = 32832 if available */
            LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef mxcsr_ptr = lp_build_fpstate_get(gallivm);
   LLVMValueRef mxcsr =
            if (util_get_cpu_caps()->has_daz) {
      /* Enable denormals are zero mode */
      }
   if (zero) {
      mxcsr = LLVMBuildOr(builder, mxcsr,
      } else {
      mxcsr = LLVMBuildAnd(builder, mxcsr,
               LLVMBuildStore(builder, mxcsr, mxcsr_ptr);
         }
         void
   lp_build_fpstate_set(struct gallivm_state *gallivm,
         {
      if (util_get_cpu_caps()->has_sse) {
      LLVMBuilderRef builder = gallivm->builder;
   mxcsr_ptr = LLVMBuildPointerCast(builder, mxcsr_ptr,
         lp_build_intrinsic(builder,
                     }
