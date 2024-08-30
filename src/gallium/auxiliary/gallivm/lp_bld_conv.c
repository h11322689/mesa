   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
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
   * Helper functions for type conversions.
   *
   * We want to use the fastest type for a given computation whenever feasible.
   * The other side of this is that we need to be able convert between several
   * types accurately and efficiently.
   *
   * Conversion between types of different bit width is quite complex since a
   *
   * To remember there are a few invariants in type conversions:
   *
   * - register width must remain constant:
   *
   *     src_type.width * src_type.length == dst_type.width * dst_type.length
   *
   * - total number of elements must remain constant:
   *
   *     src_type.length * num_srcs == dst_type.length * num_dsts
   *
   * It is not always possible to do the conversion both accurately and
   * efficiently, usually due to lack of adequate machine instructions. In these
   * cases it is important not to cut shortcuts here and sacrifice accuracy, as
   * there this functions can be used anywhere. In the future we might have a
   * precision parameter which can gauge the accuracy vs efficiency compromise,
   * but for now if the data conversion between two stages happens to be the
   * bottleneck, then most likely should just avoid converting at all and run
   * both stages with the same type.
   *
   * Make sure to run lp_test_conv unit test after any change to this file.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "util/u_debug.h"
   #include "util/u_math.h"
   #include "util/half_float.h"
   #include "util/u_cpu_detect.h"
      #include "lp_bld_type.h"
   #include "lp_bld_const.h"
   #include "lp_bld_arit.h"
   #include "lp_bld_bitarit.h"
   #include "lp_bld_pack.h"
   #include "lp_bld_conv.h"
   #include "lp_bld_logic.h"
   #include "lp_bld_intr.h"
   #include "lp_bld_printf.h"
   #include "lp_bld_format.h"
         /* the lp_test_format test fails on mingw/i686 at -O2 with gcc 10.x
   * ref https://gitlab.freedesktop.org/mesa/mesa/-/issues/3906
   */
      #if defined(__MINGW32__) && !defined(__MINGW64__) && (__GNUC__ == 10)
   #warning "disabling caller-saves optimization for this file to work around compiler bug"
   #pragma GCC optimize("-fno-caller-saves")
   #endif
      /**
   * Converts int16 half-float to float32
   * Note this can be performed in 1 instruction if vcvtph2ps exists (f16c/cvt16)
   * [llvm.x86.vcvtph2ps / _mm_cvtph_ps]
   *
   * @param src           value to convert
   *
   */
   LLVMValueRef
   lp_build_half_to_float(struct gallivm_state *gallivm,
         {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef src_type = LLVMTypeOf(src);
   unsigned src_length = LLVMGetTypeKind(src_type) == LLVMVectorTypeKind ?
            struct lp_type f32_type = lp_type_float_vec(32, 32 * src_length);
   struct lp_type i32_type = lp_type_int_vec(32, 32 * src_length);
   LLVMTypeRef int_vec_type = lp_build_vec_type(gallivm, i32_type);
            if (util_get_cpu_caps()->has_f16c &&
      (src_length == 4 || src_length == 8)) {
   if (LLVM_VERSION_MAJOR < 11) {
      const char *intrinsic = NULL;
   if (src_length == 4) {
      src = lp_build_pad_vector(gallivm, src, 8);
      }
   else {
         }
   src = LLVMBuildBitCast(builder, src,
         return lp_build_intrinsic_unary(builder, intrinsic,
      } else {
      /*
   * XXX: could probably use on other archs as well.
   * But if the cpu doesn't support it natively it looks like the backends still
   * can't lower it and will try to call out to external libraries, which will crash.
   */
   /*
   * XXX: lp_build_vec_type() would use int16 vector. Probably need to revisit
   * this at some point.
   */
   src = LLVMBuildBitCast(builder, src,
                        h = LLVMBuildZExt(builder, src, int_vec_type, "");
      }
         /**
   * Converts float32 to int16 half-float
   * Note this can be performed in 1 instruction if vcvtps2ph exists (f16c/cvt16)
   * [llvm.x86.vcvtps2ph / _mm_cvtps_ph]
   *
   * @param src           value to convert
   *
   * Convert float32 to half floats, preserving Infs and NaNs,
   * with rounding towards zero (trunc).
   * XXX: For GL, would prefer rounding towards nearest(-even).
   */
   LLVMValueRef
   lp_build_float_to_half(struct gallivm_state *gallivm,
         {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef f32_vec_type = LLVMTypeOf(src);
   unsigned length = LLVMGetTypeKind(f32_vec_type) == LLVMVectorTypeKind
         struct lp_type i32_type = lp_type_int_vec(32, 32 * length);
   struct lp_type i16_type = lp_type_int_vec(16, 16 * length);
            /*
   * Note: Newer llvm versions (3.6 or so) support fptrunc to 16 bits
   * directly, without any (x86 or generic) intrinsics.
   * Albeit the rounding mode cannot be specified (and is undefined,
   * though in practice on x86 seems to do nearest-even but it may
   * be dependent on instruction set support), so is essentially
   * useless.
            if (util_get_cpu_caps()->has_f16c &&
      (length == 4 || length == 8)) {
   struct lp_type i168_type = lp_type_int_vec(16, 16 * 8);
   unsigned mode = 3; /* same as LP_BUILD_ROUND_TRUNCATE */
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   const char *intrinsic = NULL;
   if (length == 4) {
         }
   else {
         }
   result = lp_build_intrinsic_binary(builder, intrinsic,
               if (length == 4) {
         }
               else {
      result = lp_build_float_to_smallfloat(gallivm, i32_type, src, 10, 5, 0, true);
   /* Convert int32 vector to int16 vector by trunc (might generate bad code) */
               /*
   * Debugging code.
   */
   if (0) {
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMTypeRef i16t = LLVMInt16TypeInContext(gallivm->context);
   LLVMTypeRef f32t = LLVMFloatTypeInContext(gallivm->context);
   LLVMValueRef ref_result = LLVMGetUndef(LLVMVectorType(i16t, length));
            LLVMTypeRef func_type = LLVMFunctionType(i16t, &f32t, 1, 0);
   LLVMValueRef func = lp_build_const_int_pointer(gallivm, func_to_pointer((func_pointer)_mesa_float_to_half));
            for (i = 0; i < length; ++i) {
      LLVMValueRef index = LLVMConstInt(i32t, i, 0);
   #if 0
         /*
      * XXX: not really supported by backends.
   * Even if they would now, rounding mode cannot be specified and
   * is undefined.
      #else
         #endif
                     lp_build_print_value(gallivm, "src  = ", src);
   lp_build_print_value(gallivm, "llvm = ", result);
   lp_build_print_value(gallivm, "util = ", ref_result);
      }
            }
         /**
   * Special case for converting clamped IEEE-754 floats to unsigned norms.
   *
   * The mathematical voodoo below may seem excessive but it is actually
   * paramount we do it this way for several reasons. First, there is no single
   * precision FP to unsigned integer conversion Intel SSE instruction. Second,
   * secondly, even if there was, since the FP's mantissa takes only a fraction
   * of register bits the typically scale and cast approach would require double
   * precision for accurate results, and therefore half the throughput
   *
   * Although the result values can be scaled to an arbitrary bit width specified
   * by dst_width, the actual result type will have the same width.
   *
   * Ex: src = { float, float, float, float }
   * return { i32, i32, i32, i32 } where each value is in [0, 2^dst_width-1].
   */
   LLVMValueRef
   lp_build_clamped_float_to_unsigned_norm(struct gallivm_state *gallivm,
                     {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(gallivm, src_type);
   LLVMValueRef res;
            assert(src_type.floating);
   assert(dst_width <= src_type.width);
                     if (dst_width <= mantissa) {
      /*
   * Apply magic coefficients that will make the desired result to appear
   * in the lowest significant bits of the mantissa, with correct rounding.
   *
   * This only works if the destination width fits in the mantissa.
            unsigned long long ubound;
   unsigned long long mask;
   double scale;
            ubound = (1ULL << dst_width);
   mask = ubound - 1;
   scale = (double)mask/ubound;
            res = LLVMBuildFMul(builder, src, lp_build_const_vec(gallivm, src_type, scale), "");
   /* instead of fadd/and could (with sse2) just use lp_build_iround */
   res = LLVMBuildFAdd(builder, res, lp_build_const_vec(gallivm, src_type, bias), "");
   res = LLVMBuildBitCast(builder, res, int_vec_type, "");
   res = LLVMBuildAnd(builder, res,
      }
   else if (dst_width == (mantissa + 1)) {
      /*
   * The destination width matches exactly what can be represented in
   * floating point (i.e., mantissa + 1 bits). Even so correct rounding
   * still needs to be applied (only for numbers in [0.5-1.0] would
   * conversion using truncation after scaling be sufficient).
   */
   double scale;
            lp_build_context_init(&uf32_bld, gallivm, src_type);
            res = LLVMBuildFMul(builder, src,
            }
   else {
      /*
   * The destination exceeds what can be represented in the floating point.
   * So multiply by the largest power two we get away with, and when
   * subtract the most significant bit to rescale to normalized values.
   *
   * The largest power of two factor we can get away is
   * (1 << (src_type.width - 1)), because we need to use signed . In theory it
   * should be (1 << (src_type.width - 2)), but IEEE 754 rules states
   * INT_MIN should be returned in FPToSI, which is the correct result for
   * values near 1.0!
   *
   * This means we get (src_type.width - 1) correct bits for values near 0.0,
   * and (mantissa + 1) correct bits for values near 1.0. Equally or more
   * important, we also get exact results for 0.0 and 1.0.
                     double scale = (double)(1ULL << n);
   unsigned lshift = dst_width - n;
   unsigned rshift = n;
   LLVMValueRef lshifted;
            res = LLVMBuildFMul(builder, src,
         if (!src_type.sign && src_type.width == 32)
         else
            /*
   * Align the most significant bit to its final place.
   *
   * This will cause 1.0 to overflow to 0, but the later adjustment will
   * get it right.
   */
   if (lshift) {
      lshifted = LLVMBuildShl(builder, res,
            } else {
                  /*
   * Align the most significant bit to the right.
   */
   rshifted =  LLVMBuildLShr(builder, res,
                  /*
   * Subtract the MSB to the LSB, therefore re-scaling from
   * (1 << dst_width) to ((1 << dst_width) - 1).
                           }
         /**
   * Inverse of lp_build_clamped_float_to_unsigned_norm above.
   * Ex: src = { i32, i32, i32, i32 } with values in range [0, 2^src_width-1]
   * return {float, float, float, float} with values in range [0, 1].
   */
   LLVMValueRef
   lp_build_unsigned_norm_to_float(struct gallivm_state *gallivm,
                     {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef vec_type = lp_build_vec_type(gallivm, dst_type);
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(gallivm, dst_type);
   LLVMValueRef bias_;
   LLVMValueRef res;
   unsigned mantissa;
   unsigned n;
   unsigned long long ubound;
   unsigned long long mask;
   double scale;
                              if (src_width <= (mantissa + 1)) {
      /*
   * The source width matches fits what can be represented in floating
   * point (i.e., mantissa + 1 bits). So do a straight multiplication
   * followed by casting. No further rounding is necessary.
            scale = 1.0/(double)((1ULL << src_width) - 1);
   res = LLVMBuildSIToFP(builder, src, vec_type, "");
   res = LLVMBuildFMul(builder, res,
            }
   else {
      /*
   * The source width exceeds what can be represented in floating
   * point. So truncate the incoming values.
                     ubound = ((unsigned long long)1 << n);
   mask = ubound - 1;
   scale = (double)ubound/mask;
                     if (src_width > mantissa) {
      int shift = src_width - mantissa;
   res = LLVMBuildLShr(builder, res,
                        res = LLVMBuildOr(builder,
                           res = LLVMBuildFSub(builder, res, bias_, "");
                  }
         /**
   * Pick a suitable num_dsts for lp_build_conv to ensure optimal cases are used.
   *
   * Returns the number of dsts created from src
   */
   int lp_build_conv_auto(struct gallivm_state *gallivm,
                        struct lp_type src_type,
      {
      unsigned i;
            if (src_type.floating == dst_type->floating &&
      src_type.width == dst_type->width &&
   src_type.length == dst_type->length &&
   src_type.fixed == dst_type->fixed &&
   src_type.norm == dst_type->norm &&
   src_type.sign == dst_type->sign)
         /* Special case 4x4x32 -> 1x16x8 or 2x8x32 -> 1x16x8
   */
   if (src_type.norm     == 0 &&
      src_type.width    == 32 &&
            dst_type->floating == 0 &&
   dst_type->fixed    == 0 &&
            ((src_type.floating == 1 && src_type.sign == 1 && dst_type->norm == 1) ||
   (src_type.floating == 0 && dst_type->floating == 0 &&
            /* Special case 4x4x32 --> 1x16x8 */
   if (src_type.length == 4 &&
         {
                     lp_build_conv(gallivm, src_type, *dst_type, src, num_srcs, dst, num_dsts);
               /* Special case 2x8x32 --> 1x16x8 */
   if (src_type.length == 8 &&
         {
                     lp_build_conv(gallivm, src_type, *dst_type, src, num_srcs, dst, num_dsts);
                  /* lp_build_resize does not support M:N */
   if (src_type.width == dst_type->width) {
         } else {
      /*
   * If dst_width is 16 bits and src_width 32 and the dst vector size
   * 64bit, try feeding 2 vectors at once so pack intrinsics can be used.
   * (For AVX, this isn't needed, since we usually get 256bit src and
   * 128bit dst vectors which works ok. If we do AVX2 pack this should
   * be extended but need to be able to tell conversion code about pack
   * ordering first.)
   */
   unsigned ratio = 1;
   if (src_type.width == 2 * dst_type->width &&
      src_type.length == dst_type->length &&
   dst_type->floating == 0 && (num_srcs % 2 == 0) &&
   dst_type->width * dst_type->length == 64) {
   ratio = 2;
   num_dsts /= 2;
      }
   for (i = 0; i < num_dsts; i++) {
                        }
         /**
   * Generic type conversion.
   *
   * TODO: Take a precision argument, or even better, add a new precision member
   * to the lp_type union.
   */
   void
   lp_build_conv(struct gallivm_state *gallivm,
               struct lp_type src_type,
   struct lp_type dst_type,
   {
      LLVMBuilderRef builder = gallivm->builder;
   struct lp_type tmp_type;
   LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH];
   unsigned num_tmps;
            /* We must not loose or gain channels. Only precision */
            assert(src_type.length <= LP_MAX_VECTOR_LENGTH);
   assert(dst_type.length <= LP_MAX_VECTOR_LENGTH);
   assert(num_srcs <= LP_MAX_VECTOR_LENGTH);
            tmp_type = src_type;
   for(i = 0; i < num_srcs; ++i) {
      assert(lp_check_value(src_type, src[i]));
      }
               /*
   * Special case 4x4x32 --> 1x16x8, 2x4x32 -> 1x8x8, 1x4x32 -> 1x4x8
   * Only float -> s/unorm8 and (u)int32->(u)int8.
   * XXX: This should cover all interesting backend cases for 8 bit,
   * but should use same strategy if dst is 16 bit.
   */
   if (src_type.norm     == 0 &&
      src_type.width    == 32 &&
   src_type.length   == 4 &&
            dst_type.floating == 0 &&
   dst_type.fixed    == 0 &&
            ((src_type.floating == 1 && src_type.sign == 1 && dst_type.norm == 1) ||
   (src_type.floating == 0 && dst_type.floating == 0 &&
            ((dst_type.length == 16 && 4 * num_dsts == num_srcs) ||
               {
      struct lp_build_context bld;
   struct lp_type int16_type, int32_type;
   struct lp_type dst_type_ext = dst_type;
   LLVMValueRef const_scale;
                     dst_type_ext.length = 16;
            int16_type.width *= 2;
   int16_type.length /= 2;
            int32_type.width *= 4;
   int32_type.length /= 4;
                     for (i = 0; i < num_dsts; ++i, src += 4) {
               if (src_type.floating) {
      for (j = 0; j < dst_type.length / 4; ++j) {
      /*
   * XXX This is not actually fully correct. The float to int
   * conversion will produce 0x80000000 value for everything
   * out of range and NaNs (on x86, llvm.x86.sse2.cvtps2dq).
   * Hence, NaNs and negatives will get clamped just fine to zero
   * (relying on clamping pack behavior) when converting to unorm,
   * however too large values (both finite and infinite) will also
   * end up as zero, not 255.
   * For snorm, for now we'll keep bug compatibility with generic
   * conversion path (meaning too large values are fine, but
   * NaNs get converted to -128 (purely by luck, as we don't
   * specify nan behavior for the max there) instead of 0).
   *
   * dEQP has GLES31 tests that expect +inf -> 255.0.
                        }
   else {
      if (1) {
      tmp[j] = lp_build_min_ext(&bld, bld.one, src[j],
      }
      }
   tmp[j] = LLVMBuildFMul(builder, tmp[j], const_scale, "");
         } else {
      for (j = 0; j < dst_type.length / 4; ++j) {
      if (!dst_type.sign) {
      /*
   * Pack clamp is always signed->unsigned (or signed->signed).
   * Hence need min.
   */
   LLVMValueRef const_max;
   const_max = lp_build_const_int_vec(gallivm, src_type, 255);
      } else {
                        if (num_srcs == 1) {
                                 if (num_srcs < 4) {
         }
   else {
         }
      }
   if (num_srcs < 4) {
                              /* Special case 2x8x32 --> 1x16x8, 1x8x32 ->1x8x8
   */
   else if (src_type.norm     == 0 &&
      src_type.width    == 32 &&
   src_type.length   == 8 &&
            dst_type.floating == 0 &&
   dst_type.fixed    == 0 &&
            ((src_type.floating == 1 && src_type.sign == 1 && dst_type.norm == 1) ||
   (src_type.floating == 0 && dst_type.floating == 0 &&
            ((dst_type.length == 16 && 2 * num_dsts == num_srcs) ||
                     struct lp_build_context bld;
   struct lp_type int16_type, int32_type;
   struct lp_type dst_type_ext = dst_type;
   LLVMValueRef const_scale;
                     dst_type_ext.length = 16;
            int16_type.width *= 2;
   int16_type.length /= 2;
            int32_type.width *= 4;
   int32_type.length /= 4;
                     for (i = 0; i < num_dsts; ++i, src += 2) {
      unsigned j;
                     a = src[j];
   if (src_type.floating) {
                     }
   else {
      if (1) {
      a = lp_build_min_ext(&bld, bld.one, a,
         }
   a = LLVMBuildFMul(builder, a, const_scale, "");
      } else {
      if (!dst_type.sign) {
      LLVMValueRef const_max;
   const_max = lp_build_const_int_vec(gallivm, src_type, 255);
         }
   lo = lp_build_extract_range(gallivm, a, 0, 4);
   hi = lp_build_extract_range(gallivm, a, 4, 4);
   /* relying on clamping behavior of sse2 intrinsics here */
               if (num_srcs == 1) {
         }
               if (num_srcs == 1) {
                              /* Special case -> 16bit half-float
   */
   else if (dst_type.floating && dst_type.width == 16)
   {
      /* Only support src as 32bit float currently */
            for(i = 0; i < num_tmps; ++i)
                        /* Pre convert half-floats to floats
   */
   else if (src_type.floating && src_type.width == 16)
   {
      for(i = 0; i < num_tmps; ++i)
                        /*
   * Clamp if necessary
            if(memcmp(&src_type, &dst_type, sizeof src_type) != 0) {
      struct lp_build_context bld;
   double src_min = lp_const_min(src_type);
   double dst_min = lp_const_min(dst_type);
   double src_max = lp_const_max(src_type);
   double dst_max = lp_const_max(dst_type);
                     if(src_min < dst_min) {
      if(dst_min == 0.0)
         else
         for(i = 0; i < num_tmps; ++i)
               if(src_max > dst_max) {
      if(dst_max == 1.0)
         else
         for(i = 0; i < num_tmps; ++i)
                  /*
   * Scale to the narrowest range
            if(dst_type.floating) {
         }
   else if(tmp_type.floating) {
      if(!dst_type.fixed && !dst_type.sign && dst_type.norm) {
      for(i = 0; i < num_tmps; ++i) {
      tmp[i] = lp_build_clamped_float_to_unsigned_norm(gallivm,
                  }
      }
   else {
               if (dst_scale != 1.0) {
      LLVMValueRef scale = lp_build_const_vec(gallivm, tmp_type, dst_scale);
   for(i = 0; i < num_tmps; ++i)
               /*
   * these functions will use fptosi in some form which won't work
   * with 32bit uint dst. Causes lp_test_conv failures though.
   */
                                    lp_build_context_init(&bld, gallivm, tmp_type);
   for(i = 0; i < num_tmps; ++i) {
         }
      }
                     tmp_type.floating = false;
   #if 0
                  if(dst_type.sign)
      #else
               #endif
                        }
   else {
      unsigned src_shift = lp_const_shift(src_type);
   unsigned dst_shift = lp_const_shift(dst_type);
   unsigned src_offset = lp_const_offset(src_type);
   unsigned dst_offset = lp_const_offset(dst_type);
   struct lp_build_context bld;
            /* Compensate for different offsets */
   /* sscaled -> unorm and similar would cause negative shift count, skip */
   if (dst_offset > src_offset && src_type.width > dst_type.width && src_shift > 0) {
                        shifted = lp_build_shr_imm(&bld, tmp[i], src_shift - 1);
                  if(src_shift > dst_shift) {
      for(i = 0; i < num_tmps; ++i)
                  /*
   * Truncate or expand bit width
   *
   * No data conversion should happen here, although the sign bits are
   * crucial to avoid bad clamping.
            {
               new_type = tmp_type;
   new_type.sign   = dst_type.sign;
   new_type.width  = dst_type.width;
            /*
   * Note that resize when using packs can sometimes get min/max
   * clamping for free. Should be able to exploit this...
   */
            tmp_type = new_type;
               /*
   * Scale to the widest range
            if(src_type.floating) {
         }
   else if(!src_type.floating && dst_type.floating) {
      if(!src_type.fixed && !src_type.sign && src_type.norm) {
      for(i = 0; i < num_tmps; ++i) {
      tmp[i] = lp_build_unsigned_norm_to_float(gallivm,
                  }
      }
   else {
                     /* Use an equally sized integer for intermediate computations */
   tmp_type.floating = true;
   tmp_type.sign = true;
      #if 0
               if(dst_type.sign)
         #else
               #endif
                     if (src_scale != 1.0) {
      LLVMValueRef scale = lp_build_const_vec(gallivm, tmp_type, 1.0/src_scale);
   for(i = 0; i < num_tmps; ++i)
               /* the formula above will produce value below -1.0 for most negative
   * value but everything seems happy with that hence disable for now */
                     lp_build_context_init(&bld, gallivm, dst_type);
   for(i = 0; i < num_tmps; ++i) {
      tmp[i] = lp_build_max(&bld, tmp[i],
               }
   else {
      unsigned src_shift = lp_const_shift(src_type);
   unsigned dst_shift = lp_const_shift(dst_type);
   unsigned src_offset = lp_const_offset(src_type);
   unsigned dst_offset = lp_const_offset(dst_type);
   struct lp_build_context bld;
            if (src_shift < dst_shift) {
               if (dst_shift - src_shift < dst_type.width) {
      for (i = 0; i < num_tmps; ++i) {
      pre_shift[i] = tmp[i];
         }
   else {
      /*
   * This happens for things like sscaled -> unorm conversions. Shift
   * counts equal to bit width cause undefined results, so hack around it.
   */
   for (i = 0; i < num_tmps; ++i) {
      pre_shift[i] = tmp[i];
                  /* Compensate for different offsets */
   if (dst_offset > src_offset) {
      for (i = 0; i < num_tmps; ++i) {
                           for(i = 0; i < num_dsts; ++i) {
      dst[i] = tmp[i];
         }
         /**
   * Bit mask conversion.
   *
   * This will convert the integer masks that match the given types.
   *
   * The mask values should 0 or -1, i.e., all bits either set to zero or one.
   * Any other value will likely cause unpredictable results.
   *
   * This is basically a very trimmed down version of lp_build_conv.
   */
   void
   lp_build_conv_mask(struct gallivm_state *gallivm,
                     struct lp_type src_type,
   {
         /* We must not loose or gain channels. Only precision */
            /*
   * Drop
   *
   * We assume all values are 0 or -1
            src_type.floating = false;
   src_type.fixed = false;
   src_type.sign = true;
            dst_type.floating = false;
   dst_type.fixed = false;
   dst_type.sign = true;
            /*
   * Truncate or expand bit width
               }
