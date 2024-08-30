   /**************************************************************************
   *
   * Copyright 2013 VMware, Inc.
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
   * Format conversion code for "special" float formats.
   *
   * @author Roland Scheidegger <sroland@vmware.com>
   */
         #include "util/u_debug.h"
      #include "lp_bld_type.h"
   #include "lp_bld_const.h"
   #include "lp_bld_arit.h"
   #include "lp_bld_bitarit.h"
   #include "lp_bld_logic.h"
   #include "lp_bld_format.h"
         /**
   * Convert float32 to a float-like value with less exponent and mantissa
   * bits. The mantissa is still biased, and the mantissa still has an implied 1,
   * and there may be a sign bit.
   *
   * @param src             (vector) float value to convert
   * @param mantissa_bits   the number of mantissa bits
   * @param exponent_bits   the number of exponent bits
   * @param mantissa_start  the start position of the small float in result value
   * @param has_sign        if the small float has a sign bit
   *
   * This implements round-towards-zero (trunc) hence too large numbers get
   * converted to largest representable number, not infinity.
   * Small numbers may get converted to denorms, depending on normal
   * float denorm handling of the cpu.
   * Note that compared to the references, below, we skip any rounding bias
   * since we do rounding towards zero - OpenGL allows rounding towards zero
   * (though not preferred) and DX10 even seems to require it.
   * Note that this will pack mantissa, exponent and sign bit (if any) together,
   * and shift the result to mantissa_start.
   *
   * ref http://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/
   * ref https://gist.github.com/rygorous/2156668
   */
   LLVMValueRef
   lp_build_float_to_smallfloat(struct gallivm_state *gallivm,
                              struct lp_type i32_type,
      {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef i32_floatexpmask, i32_smallexpmask, magic, normal;
   LLVMValueRef rescale_src, i32_roundmask, small_max;
   LLVMValueRef i32_qnanbit, shift, res;
   LLVMValueRef is_nan_or_inf, nan_or_inf, mask, i32_src;
   struct lp_type f32_type = lp_type_float_vec(32, 32 * i32_type.length);
   struct lp_build_context f32_bld, i32_bld;
   LLVMValueRef zero = lp_build_const_vec(gallivm, f32_type, 0.0f);
   unsigned exponent_start = mantissa_start + mantissa_bits;
   bool always_preserve_nans = true;
            lp_build_context_init(&f32_bld, gallivm, f32_type);
            i32_smallexpmask = lp_build_const_int_vec(gallivm, i32_type,
                           if (has_sign) {
         }
   else {
      /* clamp to pos range (can still have sign bit if NaN or negative zero) */
      }
            /* "ordinary" number */
   /*
   * get rid of excess mantissa bits and sign bit
   * This is only really needed for correct rounding of denorms I think
   * but only if we use the preserve NaN path does using
   * src_abs instead save us any instruction.
   */
   if (maybe_correct_denorm_rounding || !always_preserve_nans) {
      i32_roundmask = lp_build_const_int_vec(gallivm, i32_type,
               rescale_src = LLVMBuildBitCast(builder, rescale_src, i32_bld.vec_type, "");
   rescale_src = lp_build_and(&i32_bld, rescale_src, i32_roundmask);
      }
   else {
                  /* bias exponent (and denormalize if necessary) */
   magic = lp_build_const_int_vec(gallivm, i32_type,
         magic = LLVMBuildBitCast(builder, magic, f32_bld.vec_type, "");
            /* clamp to max value - largest non-infinity number */
   small_max = lp_build_const_int_vec(gallivm, i32_type,
               small_max = LLVMBuildBitCast(builder, small_max, f32_bld.vec_type, "");
   normal = lp_build_min(&f32_bld, normal, small_max);
            /*
   * handle nan/inf cases
   * a little bit tricky since -Inf -> 0, +Inf -> +Inf, +-Nan -> +Nan
   * (for no sign) else ->Inf -> ->Inf too.
   * could use explicit "unordered" comparison checking for NaNs
   * which might save us from calculating src_abs too.
   * (Cannot actually save the comparison since we need to distinguish
   * Inf and NaN cases anyway, but it would be better for AVX.)
   */
   if (always_preserve_nans) {
      LLVMValueRef infcheck_src, is_inf, is_nan;
   LLVMValueRef src_abs = lp_build_abs(&f32_bld, src);
            if (has_sign) {
         }
   else {
         }
   is_nan = lp_build_compare(gallivm, i32_type, PIPE_FUNC_GREATER,
         is_inf = lp_build_compare(gallivm, i32_type, PIPE_FUNC_EQUAL,
         is_nan_or_inf = lp_build_or(&i32_bld, is_nan, is_inf);
   /* could also set more mantissa bits but need at least the highest mantissa bit */
   i32_qnanbit = lp_build_const_vec(gallivm, i32_type, 1 << 22);
   /* combine maxexp with qnanbit */
   nan_or_inf = lp_build_or(&i32_bld, i32_smallexpmask,
      }
   else {
      /*
   * A couple simplifications, with mostly 2 drawbacks (so disabled):
   * - it will promote some SNaNs (those which only had bits set
   * in the mantissa part which got chopped off) to +-Infinity.
   * (Those bits get chopped off anyway later so can as well use
   * rescale_src instead of src_abs here saving the calculation of that.)
   * - for no sign case, it relies on the max() being used for rescale_src
   * to give back the NaN (which is NOT ieee754r behavior, but should work
   * with sse2 on a full moon (rather if I got the operand order right) -
   * we _don't_ have well-defined behavior specified with min/max wrt NaNs,
   * however, and if it gets converted to cmp/select it may not work (we
   * don't really have specified behavior for cmp wrt NaNs neither).
   */
   rescale_src = LLVMBuildBitCast(builder, rescale_src, i32_bld.vec_type, "");
   is_nan_or_inf = lp_build_compare(gallivm, i32_type, PIPE_FUNC_GEQUAL,
         /* note this will introduce excess exponent bits */
      }
            if (mantissa_start > 0 || !always_preserve_nans) {
      /* mask off excess bits */
   unsigned maskbits = (1 << (mantissa_bits + exponent_bits)) - 1;
   mask = lp_build_const_int_vec(gallivm, i32_type,
                     /* add back sign bit at right position */
   if (has_sign) {
      LLVMValueRef sign;
   struct lp_type u32_type = lp_type_uint_vec(32, 32 * i32_type.length);
   struct lp_build_context u32_bld;
            mask = lp_build_const_int_vec(gallivm, i32_type, 0x80000000);
   shift = lp_build_const_int_vec(gallivm, i32_type, 8 - exponent_bits);
   sign = lp_build_and(&i32_bld, mask, i32_src);
   sign = lp_build_shr(&u32_bld, sign, shift);
               /* shift to final position */
   if (exponent_start < 23) {
      shift = lp_build_const_int_vec(gallivm, i32_type, 23 - exponent_start);
      }
   else {
      shift = lp_build_const_int_vec(gallivm, i32_type, exponent_start - 23);
      }
      }
         /**
   * Convert rgba float SoA values to packed r11g11b10 values.
   *
   * @param src   SoA float (vector) values to convert.
   */
   LLVMValueRef
   lp_build_float_to_r11g11b10(struct gallivm_state *gallivm,
         {
      LLVMValueRef dst, rcomp, bcomp, gcomp;
   struct lp_build_context i32_bld;
   LLVMTypeRef src_type = LLVMTypeOf(*src);
   unsigned src_length = LLVMGetTypeKind(src_type) == LLVMVectorTypeKind ?
                           /* "rescale" and put in right position */
   rcomp = lp_build_float_to_smallfloat(gallivm, i32_type, src[0], 6, 5, 0, false);
   gcomp = lp_build_float_to_smallfloat(gallivm, i32_type, src[1], 6, 5, 11, false);
            /* combine the values */
   dst = lp_build_or(&i32_bld, rcomp, gcomp);
      }
         /**
   * Convert a float-like value with less exponent and mantissa
   * bits than a normal float32 to a float32. The mantissa of
   * the source value is assumed to have an implied 1, and the exponent
   * is biased. There may be a sign bit.
   * The source value to extract must be in a 32bit int (bits not part of
   * the value to convert will be masked off).
   * This works for things like 11-bit floats or half-floats,
   * mantissa, exponent (and sign if present) must be packed
   * the same as they are in a ordinary float.
   *
   * @param src             (vector) value to convert
   * @param mantissa_bits   the number of mantissa bits
   * @param exponent_bits   the number of exponent bits
   * @param mantissa_start  the bit start position of the packed component
   * @param has_sign        if the small float has a sign bit
   *
   * ref http://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/
   * ref https://gist.github.com/rygorous/2156668
   */
   LLVMValueRef
   lp_build_smallfloat_to_float(struct gallivm_state *gallivm,
                              struct lp_type f32_type,
      {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef smallexpmask, i32_floatexpmask, magic;
   LLVMValueRef wasinfnan, tmp, res, shift, maskabs, srcabs, sign;
   unsigned exponent_start = mantissa_start + mantissa_bits;
   struct lp_type i32_type = lp_type_int_vec(32, 32 * f32_type.length);
            lp_build_context_init(&f32_bld, gallivm, f32_type);
            /* extract the component to "float position" */
   if (exponent_start < 23) {
      shift = lp_build_const_int_vec(gallivm, i32_type, 23 - exponent_start);
      }
   else {
      shift = lp_build_const_int_vec(gallivm, i32_type, exponent_start - 23);
      }
   maskabs = lp_build_const_int_vec(gallivm, i32_type,
                        /* now do the actual scaling */
   smallexpmask = lp_build_const_int_vec(gallivm, i32_type,
                  if (0) {
   /*
      * Note that this code path, while simpler, will convert small
   * float denorms to floats according to current cpu denorm mode, if
   * denorms are disabled it will flush them to zero!
   * If cpu denorms are enabled, it should be faster though as long as
   * there's no denorms in the inputs, but if there are actually denorms
   * it's likely to be an order of magnitude slower (on x86 cpus).
                     /*
   * magic number has exponent new exp bias + (new exp bias - old exp bias),
   * mantissa is 0.
   */
   magic = lp_build_const_int_vec(gallivm, i32_type,
                  /* adjust exponent and fix denorms */
            /*
   * if exp was max (== NaN or Inf) set new exp to max (keep mantissa),
   * so a simple "or" will do (because exp adjust will leave mantissa intact)
   */
   /* use float compare (better for AVX 8-wide / no AVX2 but else should use int) */
   smallexpmask = LLVMBuildBitCast(builder, smallexpmask, f32_bld.vec_type, "");
   wasinfnan = lp_build_compare(gallivm, f32_type, PIPE_FUNC_GEQUAL, srcabs, smallexpmask);
   res = LLVMBuildBitCast(builder, res, i32_bld.vec_type, "");
   tmp = lp_build_and(&i32_bld, i32_floatexpmask, wasinfnan);
               else {
               /* denorm (or zero) if exponent is zero */
   exp_one = lp_build_const_int_vec(gallivm, i32_type, 1 << 23);
            /* inf or nan if exponent is max */
            /* for denormal (or zero), add (== or) magic exp to mantissa (== srcabs) (as int)
   * then subtract it (as float).
   * Another option would be to just do inttofp then do a rescale mul.
   */
   magic = lp_build_const_int_vec(gallivm, i32_type,
         denorm = lp_build_or(&i32_bld, srcabs, magic);
   denorm = LLVMBuildBitCast(builder, denorm, f32_bld.vec_type, "");
   denorm = lp_build_sub(&f32_bld, denorm,
                  /* for normals, Infs, Nans fix up exponent */
   exp_adj = lp_build_const_int_vec(gallivm, i32_type,
         normal = lp_build_add(&i32_bld, srcabs, exp_adj);
   tmp = lp_build_and(&i32_bld, wasinfnan, i32_floatexpmask);
                        if (has_sign) {
      LLVMValueRef signmask = lp_build_const_int_vec(gallivm, i32_type, 0x80000000);
   shift = lp_build_const_int_vec(gallivm, i32_type, 8 - exponent_bits);
   sign = lp_build_shl(&i32_bld, src, shift);
   sign = lp_build_and(&i32_bld, signmask, sign);
                  }
         /**
   * Convert packed float format (r11g11b10) value(s) to rgba float SoA values.
   *
   * @param src   packed AoS r11g11b10 values (as (vector) int32)
   * @param dst   pointer to the SoA result values
   */
   void
   lp_build_r11g11b10_to_float(struct gallivm_state *gallivm,
               {
      LLVMTypeRef src_type = LLVMTypeOf(src);
   unsigned src_length = LLVMGetTypeKind(src_type) == LLVMVectorTypeKind ?
                  dst[0] = lp_build_smallfloat_to_float(gallivm, f32_type, src, 6, 5, 0, false);
   dst[1] = lp_build_smallfloat_to_float(gallivm, f32_type, src, 6, 5, 11, false);
            /* Just set alpha to one */
      }
         static LLVMValueRef
   lp_build_rgb9_to_float_helper(struct gallivm_state *gallivm,
                           {
               struct lp_type i32_type = lp_type_int_vec(32, 32 * f32_type.length);
            lp_build_context_init(&i32_bld, gallivm, i32_type);
            /*
   * This is much easier as other weirdo float formats, since
   * there's no sign, no Inf/NaN, and there's nothing special
   * required for normals/denormals neither (as without the implied one
   * for the mantissa for other formats, everything looks like a denormal).
   * So just do (float)comp_bits * scale
   */
   shift = lp_build_const_int_vec(gallivm, i32_type, mantissa_start);
   mask = lp_build_const_int_vec(gallivm, i32_type, 0x1ff);
   src = lp_build_shr(&i32_bld, src, shift);
   src = lp_build_and(&i32_bld, src, mask);
   src = lp_build_int_to_float(&f32_bld, src);
      }
         /**
   * Convert shared exponent format (rgb9e5) value(s) to rgba float SoA values.
   *
   * @param src   packed AoS rgb9e5 values (as (vector) int32)
   * @param dst   pointer to the SoA result values
   */
   void
   lp_build_rgb9e5_to_float(struct gallivm_state *gallivm,
               {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef src_type = LLVMTypeOf(src);
   LLVMValueRef shift, scale, bias, exp;
   unsigned src_length = LLVMGetTypeKind(src_type) == LLVMVectorTypeKind ?
         struct lp_type i32_type = lp_type_int_vec(32, 32 * src_length);
   struct lp_type u32_type = lp_type_uint_vec(32, 32 * src_length);
   struct lp_type f32_type = lp_type_float_vec(32, 32 * src_length);
            lp_build_context_init(&i32_bld, gallivm, i32_type);
   lp_build_context_init(&u32_bld, gallivm, u32_type);
            /* extract exponent */
   shift = lp_build_const_int_vec(gallivm, i32_type, 27);
   /* this shift needs to be unsigned otherwise need mask */
            /*
   * scale factor is 2 ^ (exp - bias)
   * (and additionally corrected here for the mantissa bits)
   * not using shift because
   * a) don't have vector shift in a lot of cases
   * b) shift direction changes hence need 2 shifts + conditional
   *    (or rotate instruction which is even more rare (for instance XOP))
   * so use whacky float 2 ^ function instead manipulating exponent
   * (saves us the float conversion at the end too)
   */
   bias = lp_build_const_int_vec(gallivm, i32_type, 127 - (15 + 9));
   scale = lp_build_add(&i32_bld, exp, bias);
   shift = lp_build_const_int_vec(gallivm, i32_type, 23);
   scale = lp_build_shl(&i32_bld, scale, shift);
            dst[0] = lp_build_rgb9_to_float_helper(gallivm, f32_type, src, scale, 0);
   dst[1] = lp_build_rgb9_to_float_helper(gallivm, f32_type, src, scale, 9);
            /* Just set alpha to one */
      }
