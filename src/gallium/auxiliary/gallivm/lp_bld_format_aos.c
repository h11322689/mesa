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
   * AoS pixel format manipulation.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "util/u_pointer.h"
   #include "util/u_string.h"
   #include "util/u_cpu_detect.h"
      #include "lp_bld_arit.h"
   #include "lp_bld_init.h"
   #include "lp_bld_type.h"
   #include "lp_bld_flow.h"
   #include "lp_bld_const.h"
   #include "lp_bld_conv.h"
   #include "lp_bld_swizzle.h"
   #include "lp_bld_gather.h"
   #include "lp_bld_debug.h"
   #include "lp_bld_format.h"
   #include "lp_bld_pack.h"
   #include "lp_bld_intr.h"
   #include "lp_bld_logic.h"
   #include "lp_bld_bitarit.h"
   #include "lp_bld_misc.h"
      /**
   * Basic swizzling.  Rearrange the order of the unswizzled array elements
   * according to the format description.  PIPE_SWIZZLE_0/ONE are supported
   * too.
   * Ex: if unswizzled[4] = {B, G, R, x}, then swizzled_out[4] = {R, G, B, 1}.
   */
   LLVMValueRef
   lp_build_format_swizzle_aos(const struct util_format_description *desc,
               {
      unsigned char swizzles[4];
                     for (chan = 0; chan < 4; ++chan) {
               if (desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
      /*
   * For ZS formats do RGBA = ZZZ1
   */
   if (chan == 3) {
         } else if (desc->swizzle[0] == PIPE_SWIZZLE_NONE) {
         } else {
            } else {
         }
                  }
         /**
   * Whether the format matches the vector type, apart of swizzles.
   */
   static inline bool
   format_matches_type(const struct util_format_description *desc,
         {
      enum util_format_type chan_type;
                     if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN ||
      desc->colorspace != UTIL_FORMAT_COLORSPACE_RGB ||
   desc->block.width != 1 ||
   desc->block.height != 1) {
               if (type.floating) {
         } else if (type.fixed) {
         } else if (type.sign) {
         } else {
                  for (chan = 0; chan < desc->nr_channels; ++chan) {
      if (desc->channel[chan].size != type.width) {
                  if (desc->channel[chan].type != UTIL_FORMAT_TYPE_VOID) {
      if (desc->channel[chan].type != chan_type ||
      desc->channel[chan].normalized != type.norm) {
                        }
      /*
   * Do rounding when converting small unorm values to larger ones.
   * Not quite 100% accurate, as it's done by appending MSBs, but
   * should be good enough.
   */
      static inline LLVMValueRef
   scale_bits_up(struct gallivm_state *gallivm,
               int src_bits,
   int dst_bits,
   {
      LLVMBuilderRef builder = gallivm->builder;
            if (src_bits == 1 && dst_bits > 1) {
      /*
   * Useful for a1 - we'd need quite some repeated copies otherwise.
   */
   struct lp_build_context bld;
   LLVMValueRef dst_mask;
   lp_build_context_init(&bld, gallivm, src_type);
   dst_mask = lp_build_const_int_vec(gallivm, src_type,
         result = lp_build_cmp(&bld, PIPE_FUNC_EQUAL, src,
            }
   else if (dst_bits > src_bits) {
      /* Scale up bits */
            /* Shift left by difference in bits */
   result = LLVMBuildShl(builder,
                        if (db <= src_bits) {
      /* Enough bits in src to fill the remainder */
   LLVMValueRef lower = LLVMBuildLShr(builder,
                                 } else if (db > src_bits) {
                                       result = LLVMBuildOr(builder,
                        } else {
                     }
      /**
   * Unpack a single pixel into its XYZW components.
   *
   * @param desc  the pixel format for the packed pixel value
   * @param packed integer pixel in a format such as PIPE_FORMAT_B8G8R8A8_UNORM
   *
   * @return XYZW in a float[4] or ubyte[4] or ushort[4] vector.
   */
   static inline LLVMValueRef
   lp_build_unpack_arith_rgba_aos(struct gallivm_state *gallivm,
               {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef shifted, casted, scaled, masked;
   LLVMValueRef shifts[4];
   LLVMValueRef masks[4];
   LLVMValueRef scales[4];
            bool normalized;
   bool needs_uitofp;
            /* TODO: Support more formats */
   assert(desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
   assert(desc->block.width == 1);
   assert(desc->block.height == 1);
            /* Do the intermediate integer computations with 32bit integers since it
   * matches floating point size */
                     /* Broadcast the packed value to all four channels
   * before: packed = BGRA
   * after: packed = {BGRA, BGRA, BGRA, BGRA}
   */
   packed = LLVMBuildInsertElement(builder, LLVMGetUndef(vec32_type), packed,
               packed = LLVMBuildShuffleVector(builder, packed, LLVMGetUndef(vec32_type),
                  /* Initialize vector constants */
   normalized = false;
            /* Loop over 4 color components */
   for (i = 0; i < 4; ++i) {
      unsigned bits = desc->channel[i].size;
            if (desc->channel[i].type == UTIL_FORMAT_TYPE_VOID) {
      shifts[i] = LLVMGetUndef(LLVMInt32TypeInContext(gallivm->context));
   masks[i] = LLVMConstNull(LLVMInt32TypeInContext(gallivm->context));
      }
   else {
                        if (bits == 32) {
                                 if (desc->channel[i].normalized) {
      scales[i] = lp_build_const_float(gallivm, 1.0 / mask);
      }
   else
                  /* Ex: convert packed = {XYZW, XYZW, XYZW, XYZW}
   * into masked = {X, Y, Z, W}
   */
   if (desc->block.bits < 32 && normalized) {
      /*
   * Note: we cannot do the shift below on x86 natively until AVX2.
   *
   * Old llvm versions will resort to scalar extract/shift insert,
   * which is definitely terrible, new versions will just do
   * several vector shifts and shuffle/blend results together.
   * We could turn this into a variable left shift plus a constant
   * right shift, and llvm would then turn the variable left shift
   * into a mul for us (albeit without sse41 the mul needs emulation
   * too...). However, since we're going to do a float mul
   * anyway, we just adjust that mul instead (plus the mask), skipping
   * the shift completely.
   * We could also use a extra mul when the format isn't normalized and
   * we don't have AVX2 support, but don't bother for now. Unfortunately,
   * this strategy doesn't work for 32bit formats (such as rgb10a2 or even
   * rgba8 if it ends up here), as that would require UIToFP, albeit that
   * would be fixable with easy 16bit shuffle (unless there's channels
   * crossing 16bit boundaries).
   */
   for (i = 0; i < 4; ++i) {
      if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
      unsigned bits = desc->channel[i].size;
   unsigned shift = desc->channel[i].shift;
   unsigned long long mask = ((1ULL << bits) - 1) << shift;
   scales[i] = lp_build_const_float(gallivm, 1.0 / mask);
         }
      } else {
      shifted = LLVMBuildLShr(builder, packed, LLVMConstVector(shifts, 4), "");
               if (!needs_uitofp) {
      /* UIToFP can't be expressed in SSE2 */
      } else {
                  /*
   * At this point 'casted' may be a vector of floats such as
   * {255.0, 255.0, 255.0, 255.0}. (Normalized values may be multiplied
   * by powers of two). Next, if the pixel values are normalized
   * we'll scale this to {1.0, 1.0, 1.0, 1.0}.
            if (normalized)
         else
               }
         /**
   * Pack a single pixel.
   *
   * @param rgba 4 float vector with the unpacked components.
   *
   * XXX: This is mostly for reference and testing -- operating a single pixel at
   * a time is rarely if ever needed.
   */
   LLVMValueRef
   lp_build_pack_rgba_aos(struct gallivm_state *gallivm,
               {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef type;
   LLVMValueRef packed = NULL;
   LLVMValueRef swizzles[4];
   LLVMValueRef shifted, casted, scaled, unswizzled;
   LLVMValueRef shifts[4];
   LLVMValueRef scales[4];
   bool normalized;
            assert(desc->layout == UTIL_FORMAT_LAYOUT_PLAIN);
   assert(desc->block.width == 1);
                     /* Unswizzle the color components into the source vector. */
   for (i = 0; i < 4; ++i) {
      for (j = 0; j < 4; ++j) {
      if (desc->swizzle[j] == i)
      }
   if (j < 4)
         else
               unswizzled = LLVMBuildShuffleVector(builder, rgba,
                  normalized = false;
   for (i = 0; i < 4; ++i) {
      unsigned bits = desc->channel[i].size;
            if (desc->channel[i].type == UTIL_FORMAT_TYPE_VOID) {
      shifts[i] = LLVMGetUndef(LLVMInt32TypeInContext(gallivm->context));
      }
   else {
                                       if (desc->channel[i].normalized) {
      scales[i] = lp_build_const_float(gallivm, mask);
      }
   else
                  if (normalized)
         else
                     shifted = LLVMBuildShl(builder, casted, LLVMConstVector(shifts, 4), "");
      /* Bitwise or all components */
   for (i = 0; i < 4; ++i) {
      if (desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED) {
      LLVMValueRef component = LLVMBuildExtractElement(builder, shifted,
         if (packed)
         else
                  if (!packed)
            if (desc->block.bits < 32)
               }
               /**
   * Fetch a pixel into a 4 float AoS.
   *
   * \param format_desc  describes format of the image we're fetching from
   * \param aligned  whether the data is guaranteed to be aligned
   * \param ptr  address of the pixel block (or the texel if uncompressed)
   * \param i, j  the sub-block pixel coordinates.  For non-compressed formats
   *              these will always be (0, 0).
   * \param cache  optional value pointing to a lp_build_format_cache structure
   * \return  a 4 element vector with the pixel's RGBA values.
   */
   LLVMValueRef
   lp_build_fetch_rgba_aos(struct gallivm_state *gallivm,
                           const struct util_format_description *format_desc,
   struct lp_type type,
   bool aligned,
   LLVMValueRef base_ptr,
   {
      const struct util_format_unpack_description *unpack =
         LLVMBuilderRef builder = gallivm->builder;
   unsigned num_pixels = type.length / 4;
            assert(type.length <= LP_MAX_VECTOR_LENGTH);
                     /*
   * Trivial case
   *
   * The format matches the type (apart of a swizzle) so no need for
   * scaling or converting.
            if (format_matches_type(format_desc, type) &&
      format_desc->block.bits <= type.width * 4 &&
   /* XXX this shouldn't be needed */
   util_is_power_of_two_or_zero(format_desc->block.bits)) {
   LLVMValueRef packed;
   LLVMTypeRef dst_vec_type = lp_build_vec_type(gallivm, type);
   struct lp_type fetch_type;
            /*
   * The format matches the type (apart of a swizzle) so no need for
   * scaling or converting.
            fetch_type = lp_type_uint(type.width*4);
   packed = lp_build_gather(gallivm, type.length/4,
                  assert(format_desc->block.bits <= vec_len);
            packed = LLVMBuildBitCast(gallivm->builder, packed, dst_vec_type, "");
               /*
   * Bit arithmetic for converting small_unorm to unorm8.
   *
   * This misses some opportunities for optimizations (like skipping mask
   * for the highest channel for instance, or doing bit scaling in parallel
   * for channels with the same bit width) but it should be passable for
   * all arithmetic formats.
   */
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN &&
      format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB &&
   util_format_fits_8unorm(format_desc) &&
   type.width == 8 && type.norm == 1 && type.sign == 0 &&
   type.fixed == 0 && type.floating == 0) {
   LLVMValueRef packed, res = NULL, chans[4], rgba[4];
   LLVMTypeRef dst_vec_type, conv_vec_type;
   struct lp_type fetch_type, conv_type;
   struct lp_build_context bld_conv;
            fetch_type = lp_type_uint(type.width*4);
   conv_type = lp_type_int_vec(type.width*4, type.width * type.length);
   dst_vec_type = lp_build_vec_type(gallivm, type);
   conv_vec_type = lp_build_vec_type(gallivm, conv_type);
            packed = lp_build_gather(gallivm, type.length/4,
                  assert(format_desc->block.bits * type.length / 4 <=
                     for (j = 0; j < format_desc->nr_channels; ++j) {
                              /* Extract bits from source */
   chans[j] = LLVMBuildLShr(builder, packed,
                  chans[j] = LLVMBuildAnd(builder, chans[j],
                  /* Scale bits */
   if (type.norm) {
      chans[j] = scale_bits_up(gallivm, format_desc->channel[j].size,
         }
   /*
   * This is a hacked lp_build_format_swizzle_soa() since we need a
   * normalized 1 but only 8 bits in a 32bit vector...
   */
   for (j = 0; j < 4; ++j) {
      enum pipe_swizzle swizzle = format_desc->swizzle[j];
   if (swizzle == PIPE_SWIZZLE_1) {
         } else {
         }
   if (j == 0) {
         } else {
      rgba[j] = LLVMBuildShl(builder, rgba[j],
                     }
                        /*
   * Bit arithmetic
            if (format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN &&
      (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB ||
   format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) &&
   format_desc->block.width == 1 &&
   format_desc->block.height == 1 &&
   /* XXX this shouldn't be needed */
   util_is_power_of_two_or_zero(format_desc->block.bits) &&
   format_desc->block.bits <= 32 &&
   format_desc->is_bitmask &&
   !format_desc->is_mixed &&
   (format_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED ||
   format_desc->channel[1].type == UTIL_FORMAT_TYPE_UNSIGNED) &&
            LLVMValueRef tmps[LP_MAX_VECTOR_LENGTH/4];
   LLVMValueRef res[LP_MAX_VECTOR_WIDTH / 128];
   struct lp_type conv_type;
            /*
   * Note this path is generally terrible for fetching multiple pixels.
   * We should make sure we cannot hit this code path for anything but
   * single pixels.
            /*
   * Unpack a pixel at a time into a <4 x float> RGBA vector
            for (k = 0; k < num_pixels; ++k) {
               packed = lp_build_gather_elem(gallivm, num_pixels,
                  tmps[k] = lp_build_unpack_arith_rgba_aos(gallivm,
                     /*
   * Type conversion.
   *
   * TODO: We could avoid floating conversion for integer to
   * integer conversions.
            if (gallivm_debug & GALLIVM_DEBUG_PERF && !type.floating) {
      debug_printf("%s: unpacking %s with floating point\n",
               conv_type = lp_float32_vec4_type();
   num_conv_src = num_pixels;
            if (num_pixels % 8 == 0) {
      lp_build_concat_n(gallivm, lp_float32_vec4_type(),
         conv_type.length *= num_pixels / 4;
   num_conv_src = 4 * num_pixels / 8;
   if (type.width == 8 && type.floating == 0 && type.fixed == 0) {
      /*
   * FIXME: The fast float->unorm path (which is basically
   * skipping the MIN/MAX which are extremely pointless in any
   * case) requires that there's 2 destinations...
   * In any case, we really should make sure we don't hit this
   * code with multiple pixels for unorm8 dst types, it's
   * completely hopeless even if we do hit the right conversion.
   */
   type.length /= num_pixels / 4;
                  lp_build_conv(gallivm, conv_type, type,
            if (num_pixels % 8 == 0 &&
      (type.width == 8 && type.floating == 0 && type.fixed == 0)) {
                           /* If all channels are of same type and we are not using half-floats */
   if (format_desc->is_array &&
      format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB) {
   assert(!format_desc->is_mixed);
               /*
   * YUV / subsampled formats
            if (format_desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
      struct lp_type tmp_type;
            memset(&tmp_type, 0, sizeof tmp_type);
   tmp_type.width = 8;
   tmp_type.length = num_pixels * 4;
            tmp = lp_build_fetch_subsampled_rgba_aos(gallivm,
                                    lp_build_conv(gallivm,
                              /*
   * s3tc rgb formats
            if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
      struct lp_type tmp_type;
            memset(&tmp_type, 0, sizeof tmp_type);
   tmp_type.width = 8;
   tmp_type.length = num_pixels * 4;
            tmp = lp_build_fetch_s3tc_rgba_aos(gallivm,
                                          lp_build_conv(gallivm,
                              /*
   * rgtc rgb formats
            if (format_desc->layout == UTIL_FORMAT_LAYOUT_RGTC) {
      struct lp_type tmp_type;
            memset(&tmp_type, 0, sizeof tmp_type);
   tmp_type.width = 8;
   tmp_type.length = num_pixels * 4;
   tmp_type.norm = true;
   tmp_type.sign = (format_desc->format == PIPE_FORMAT_RGTC1_SNORM ||
                        tmp = lp_build_fetch_rgtc_rgba_aos(gallivm,
                                          lp_build_conv(gallivm,
                              /*
   * Fallback to util_format_description::fetch_rgba_8unorm().
            if (unpack->fetch_rgba_8unorm &&
      !type.floating && type.width == 8 && !type.sign && type.norm) {
   /*
   * Fallback to calling util_format_description::fetch_rgba_8unorm.
   *
   * This is definitely not the most efficient way of fetching pixels, as
   * we miss the opportunity to do vectorization, but this it is a
   * convenient for formats or scenarios for which there was no opportunity
   * or incentive to optimize.
            LLVMTypeRef i8t = LLVMInt8TypeInContext(gallivm->context);
   LLVMTypeRef pi8t = LLVMPointerType(i8t, 0);
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMValueRef function;
   LLVMValueRef tmp_ptr;
   LLVMValueRef tmp;
   LLVMValueRef res;
            if (gallivm_debug & GALLIVM_DEBUG_PERF) {
      debug_printf("%s: falling back to util_format_%s_fetch_rgba_8unorm\n",
               /*
   * Declare and bind format_desc->fetch_rgba_8unorm().
            LLVMTypeRef function_type;
   {
      /*
   * Function to call looks like:
   *   fetch(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
   */
                  ret_type = LLVMVoidTypeInContext(gallivm->context);
   arg_types[0] = pi8t;
   arg_types[1] = pi8t;
   arg_types[2] = i32t;
   arg_types[3] = i32t;
   function_type = LLVMFunctionType(ret_type, arg_types,
               if (gallivm->cache)
         /* make const pointer for the C fetch_rgba_8unorm function */
   function = lp_build_const_int_pointer(gallivm,
         /* cast the callee pointer to the function's type */
                              /*
   * Invoke format_desc->fetch_rgba_8unorm() for each pixel and insert the result
   * in the SoA vectors.
            for (k = 0; k < num_pixels; ++k) {
                     args[0] = LLVMBuildBitCast(builder, tmp_ptr, pi8t, "");
                  if (num_pixels == 1) {
      args[2] = i;
      }
   else {
      args[2] = LLVMBuildExtractElement(builder, i, index, "");
                                 if (num_pixels == 1) {
         }
   else {
                     /* Bitcast from <n x i32> to <4n x i8> */
                        /*
   * Fallback to fetch_rgba().
            util_format_fetch_rgba_func_ptr fetch_rgba =
         if (fetch_rgba) {
      /*
   * Fallback to calling util_format_description::fetch_rgba_float.
   *
   * This is definitely not the most efficient way of fetching pixels, as
   * we miss the opportunity to do vectorization, but this it is a
   * convenient for formats or scenarios for which there was no opportunity
   * or incentive to optimize.
            LLVMTypeRef f32t = LLVMFloatTypeInContext(gallivm->context);
   LLVMTypeRef f32x4t = LLVMVectorType(f32t, 4);
   LLVMTypeRef pf32t = LLVMPointerType(f32t, 0);
   LLVMTypeRef pi8t = LLVMPointerType(LLVMInt8TypeInContext(gallivm->context), 0);
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMValueRef function;
   LLVMValueRef tmp_ptr;
   LLVMValueRef tmps[LP_MAX_VECTOR_LENGTH/4];
   LLVMValueRef res;
            if (gallivm_debug & GALLIVM_DEBUG_PERF) {
      debug_printf("%s: falling back to util_format_%s_fetch_rgba_float\n",
               /*
   * Declare and bind unpack->fetch_rgba_float().
            LLVMTypeRef function_type = NULL;
   {
      /*
   * Function to call looks like:
   *   fetch(float *dst, const uint8_t *src, unsigned i, unsigned j)
   */
                  ret_type = LLVMVoidTypeInContext(gallivm->context);
   arg_types[0] = pf32t;
   arg_types[1] = pi8t;
   arg_types[2] = i32t;
   arg_types[3] = i32t;
      }
   if (gallivm->cache)
         function = lp_build_const_func_pointer_from_type(gallivm,
                                 /*
   * Invoke format_desc->fetch_rgba_float() for each pixel and insert the result
   * in the SoA vectors.
            for (k = 0; k < num_pixels; ++k) {
               args[0] = LLVMBuildBitCast(builder, tmp_ptr, pf32t, "");
                  if (num_pixels == 1) {
      args[2] = i;
      }
   else {
      LLVMValueRef index = lp_build_const_int32(gallivm, k);
   args[2] = LLVMBuildExtractElement(builder, i, index, "");
                                    lp_build_conv(gallivm,
                                             assert(0);
      }
