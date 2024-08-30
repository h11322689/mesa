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
   * Helper functions for packing/unpacking.
   *
   * Pack/unpacking is necessary for conversion between types of different
   * bit width.
   *
   * They are also commonly used when an computation needs higher
   * precision for the intermediate values. For example, if one needs the
   * function:
   *
   *   c = compute(a, b);
   *
   * to use more precision for intermediate results then one should implement it
   * as:
   *
   *   LLVMValueRef
   *   compute(LLVMBuilderRef builder struct lp_type type, LLVMValueRef a, LLVMValueRef b)
   *   {
   *      struct lp_type wide_type = lp_wider_type(type);
   *      LLVMValueRef al, ah, bl, bh, cl, ch, c;
   *
   *      lp_build_unpack2(builder, type, wide_type, a, &al, &ah);
   *      lp_build_unpack2(builder, type, wide_type, b, &bl, &bh);
   *
   *      cl = compute_half(al, bl);
   *      ch = compute_half(ah, bh);
   *
   *      c = lp_build_pack2(bld->builder, wide_type, type, cl, ch);
   *
   *      return c;
   *   }
   *
   * where compute_half() would do the computation for half the elements with
   * twice the precision.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "util/u_debug.h"
   #include "util/u_math.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_memory.h"
      #include "lp_bld_type.h"
   #include "lp_bld_const.h"
   #include "lp_bld_init.h"
   #include "lp_bld_intr.h"
   #include "lp_bld_arit.h"
   #include "lp_bld_pack.h"
   #include "lp_bld_swizzle.h"
         /**
   * Build shuffle vectors that match PUNPCKLxx and PUNPCKHxx instructions.
   */
   static LLVMValueRef
   lp_build_const_unpack_shuffle(struct gallivm_state *gallivm,
         {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
            assert(n <= LP_MAX_VECTOR_LENGTH);
                     for(i = 0, j = lo_hi*n/2; i < n; i += 2, ++j) {
      elems[i + 0] = lp_build_const_int32(gallivm, 0 + j);
                  }
      /**
   * Similar to lp_build_const_unpack_shuffle but for special AVX 256bit unpack.
   * See comment above lp_build_interleave2_half for more details.
   */
   static LLVMValueRef
   lp_build_const_unpack_shuffle_half(struct gallivm_state *gallivm,
         {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
            assert(n <= LP_MAX_VECTOR_LENGTH);
            for (i = 0, j = lo_hi*(n/4); i < n; i += 2, ++j) {
      if (i == (n / 2))
            elems[i + 0] = lp_build_const_int32(gallivm, 0 + j);
                  }
      /**
   * Similar to lp_build_const_unpack_shuffle_half, but for AVX512
   * See comment above lp_build_interleave2_half for more details.
   */
   static LLVMValueRef
   lp_build_const_unpack_shuffle_16wide(struct gallivm_state *gallivm,
         {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
                     // for the following lo_hi setting, convert 0 -> f to:
   // 0: 0 16 4 20  8 24 12 28 1 17 5 21  9 25 13 29
   // 1: 2 18 6 22 10 26 14 30 3 19 7 23 11 27 15 31
   for (i = 0; i < 16; i++) {
                              }
      /**
   * Build shuffle vectors that match PACKxx (SSE) instructions or
   * VPERM (Altivec).
   */
   static LLVMValueRef
   lp_build_const_pack_shuffle(struct gallivm_state *gallivm, unsigned n)
   {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
                        #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
            }
      /**
   * Return a vector with elements src[start:start+size]
   * Most useful for getting half the values out of a 256bit sized vector,
   * otherwise may cause data rearrangement to happen.
   */
   LLVMValueRef
   lp_build_extract_range(struct gallivm_state *gallivm,
                     {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
                     for (i = 0; i < size; ++i)
            if (size == 1) {
         }
   else {
      return LLVMBuildShuffleVector(gallivm->builder, src, src,
         }
      /**
   * Concatenates several (must be a power of 2) vectors (of same type)
   * into a larger one.
   * Most useful for building up a 256bit sized vector out of two 128bit ones.
   */
   LLVMValueRef
   lp_build_concat(struct gallivm_state *gallivm,
                     {
      unsigned new_length, i;
   LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH/2];
            assert(src_type.length * num_vectors <= ARRAY_SIZE(shuffles));
                     for (i = 0; i < num_vectors; i++)
            while (num_vectors > 1) {
      num_vectors >>= 1;
   new_length <<= 1;
   for (i = 0; i < new_length; i++) {
         }
   for (i = 0; i < num_vectors; i++) {
      tmp[i] = LLVMBuildShuffleVector(gallivm->builder, tmp[i*2], tmp[i*2 + 1],
                     }
         /**
   * Combines vectors to reduce from num_srcs to num_dsts.
   * Returns the number of src vectors concatenated in a single dst.
   *
   * num_srcs must be exactly divisible by num_dsts.
   *
   * e.g. For num_srcs = 4 and src = [x, y, z, w]
   *          num_dsts = 1  dst = [xyzw]    return = 4
   *          num_dsts = 2  dst = [xy, zw]  return = 2
   */
   int
   lp_build_concat_n(struct gallivm_state *gallivm,
                     struct lp_type src_type,
   LLVMValueRef *src,
   {
      int size = num_srcs / num_dsts;
            assert(num_srcs >= num_dsts);
            if (num_srcs == num_dsts) {
      for (i = 0; i < num_dsts; ++i) {
         }
               for (i = 0; i < num_dsts; ++i) {
                     }
         /**
   * Un-interleave vector.
   * This will return a vector consisting of every second element
   * (depending on lo_hi, beginning at 0 or 1).
   * The returned vector size (elems and width) will only be half
   * that of the source vector.
   */
   LLVMValueRef
   lp_build_uninterleave1(struct gallivm_state *gallivm,
                     {
      LLVMValueRef shuffle, elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;
            for (i = 0; i < num_elems / 2; ++i)
                        }
         /**
   * Interleave vector elements.
   *
   * Matches the PUNPCKLxx and PUNPCKHxx SSE instructions
   * (but not for 256bit AVX vectors).
   */
   LLVMValueRef
   lp_build_interleave2(struct gallivm_state *gallivm,
                           {
               if (type.length == 2 && type.width == 128 && util_get_cpu_caps()->has_avx) {
      /*
   * XXX: This is a workaround for llvm code generation deficiency. Strangely
   * enough, while this needs vinsertf128/vextractf128 instructions (hence
   * a natural match when using 2x128bit vectors) the "normal" unpack shuffle
   * generates code ranging from atrocious (llvm 3.1) to terrible (llvm 3.2, 3.3).
   * So use some different shuffles instead (the exact shuffles don't seem to
   * matter, as long as not using 128bit wide vectors, works with 8x32 or 4x64).
   */
   struct lp_type tmp_type = type;
   LLVMValueRef srchalf[2], tmpdst;
   tmp_type.length = 4;
   tmp_type.width = 64;
   a = LLVMBuildBitCast(gallivm->builder, a, lp_build_vec_type(gallivm, tmp_type), "");
   b = LLVMBuildBitCast(gallivm->builder, b, lp_build_vec_type(gallivm, tmp_type), "");
   srchalf[0] = lp_build_extract_range(gallivm, a, lo_hi * 2, 2);
   srchalf[1] = lp_build_extract_range(gallivm, b, lo_hi * 2, 2);
   tmp_type.length = 2;
   tmpdst = lp_build_concat(gallivm, srchalf, tmp_type, 2);
                           }
      /**
   * Interleave vector elements but with 256 (or 512) bit,
   * treats it as interleave with 2 concatenated 128 (or 256) bit vectors.
   *
   * This differs to lp_build_interleave2 as that function would do the following (for lo):
   * a0 b0 a1 b1 a2 b2 a3 b3, and this does not compile into an AVX unpack instruction.
   *
   *
   * An example interleave 8x float with 8x float on AVX 256bit unpack:
   *   a0 a1 a2 a3 a4 a5 a6 a7 <-> b0 b1 b2 b3 b4 b5 b6 b7
   *
   * Equivalent to interleaving 2x 128 bit vectors
   *   a0 a1 a2 a3 <-> b0 b1 b2 b3 concatenated with a4 a5 a6 a7 <-> b4 b5 b6 b7
   *
   * So interleave-lo would result in:
   *   a0 b0 a1 b1 a4 b4 a5 b5
   *
   * And interleave-hi would result in:
   *   a2 b2 a3 b3 a6 b6 a7 b7
   *
   * For 512 bits, the following are true:
   *
   * Interleave-lo would result in (capital letters denote hex indices):
   *   a0 b0 a1 b1 a4 b4 a5 b5 a8 b8 a9 b9 aC bC aD bD
   *
   * Interleave-hi would result in:
   *   a2 b2 a3 b3 a6 b6 a7 b7 aA bA aB bB aE bE aF bF
   */
   LLVMValueRef
   lp_build_interleave2_half(struct gallivm_state *gallivm,
                           {
      if (type.length * type.width == 256) {
      LLVMValueRef shuffle = lp_build_const_unpack_shuffle_half(gallivm, type.length, lo_hi);
      } else if ((type.length == 16) && (type.width == 32)) {
      LLVMValueRef shuffle = lp_build_const_unpack_shuffle_16wide(gallivm, lo_hi);
      } else {
            }
         /**
   * Double the bit width.
   *
   * This will only change the number of bits the values are represented, not the
   * values themselves.
   *
   */
   void
   lp_build_unpack2(struct gallivm_state *gallivm,
                  struct lp_type src_type,
   struct lp_type dst_type,
      {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef msb;
            assert(!src_type.floating);
   assert(!dst_type.floating);
   assert(dst_type.width == src_type.width * 2);
            if(dst_type.sign && src_type.sign) {
      /* Replicate the sign bit in the most significant bits */
      }
   else
      /* Most significant bits always zero */
            #if UTIL_ARCH_LITTLE_ENDIAN
      *dst_lo = lp_build_interleave2(gallivm, src_type, src, msb, 0);
         #else
      *dst_lo = lp_build_interleave2(gallivm, src_type, msb, src, 0);
      #endif
                           *dst_lo = LLVMBuildBitCast(builder, *dst_lo, dst_vec_type, "");
      }
         /**
   * Double the bit width, with an order which fits the cpu nicely.
   *
   * This will only change the number of bits the values are represented, not the
   * values themselves.
   *
   * The order of the results is not guaranteed, other than it will match
   * the corresponding lp_build_pack2_native call.
   */
   void
   lp_build_unpack2_native(struct gallivm_state *gallivm,
                           struct lp_type src_type,
   {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef msb;
            assert(!src_type.floating);
   assert(!dst_type.floating);
   assert(dst_type.width == src_type.width * 2);
            if(dst_type.sign && src_type.sign) {
      /* Replicate the sign bit in the most significant bits */
   msb = LLVMBuildAShr(builder, src,
      }
   else
      /* Most significant bits always zero */
            #if UTIL_ARCH_LITTLE_ENDIAN
      if (src_type.length * src_type.width == 256 && util_get_cpu_caps()->has_avx2) {
      *dst_lo = lp_build_interleave2_half(gallivm, src_type, src, msb, 0);
      } else {
      *dst_lo = lp_build_interleave2(gallivm, src_type, src, msb, 0);
         #else
      *dst_lo = lp_build_interleave2(gallivm, src_type, msb, src, 0);
      #endif
                           *dst_lo = LLVMBuildBitCast(builder, *dst_lo, dst_vec_type, "");
      }
         /**
   * Expand the bit width.
   *
   * This will only change the number of bits the values are represented, not the
   * values themselves.
   */
   void
   lp_build_unpack(struct gallivm_state *gallivm,
                  struct lp_type src_type,
      {
      unsigned num_tmps;
            /* Register width must remain constant */
            /* We must not loose or gain channels. Only precision */
            num_tmps = 1;
            while(src_type.width < dst_type.width) {
               tmp_type.width *= 2;
            for(i = num_tmps; i--; ) {
      lp_build_unpack2(gallivm, src_type, tmp_type, dst[i], &dst[2*i + 0],
                                       }
         /**
   * Non-interleaved pack.
   *
   * This will move values as
   *         (LSB)                     (MSB)
   *   lo =   l0 __ l1 __ l2 __..  __ ln __
   *   hi =   h0 __ h1 __ h2 __..  __ hn __
   *   res =  l0 l1 l2 .. ln h0 h1 h2 .. hn
   *
   * This will only change the number of bits the values are represented, not the
   * values themselves.
   *
   * It is assumed the values are already clamped into the destination type range.
   * Values outside that range will produce undefined results. Use
   * lp_build_packs2 instead.
   */
   LLVMValueRef
   lp_build_pack2(struct gallivm_state *gallivm,
                  struct lp_type src_type,
      {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef dst_vec_type = lp_build_vec_type(gallivm, dst_type);
   LLVMValueRef shuffle;
   LLVMValueRef res = NULL;
            assert(!src_type.floating);
   assert(!dst_type.floating);
   assert(src_type.width == dst_type.width * 2);
            /* Check for special cases first */
   if ((util_get_cpu_caps()->has_sse2 || util_get_cpu_caps()->has_altivec) &&
      src_type.width * src_type.length >= 128) {
   const char *intrinsic = NULL;
            switch(src_type.width) {
   case 32:
      if (util_get_cpu_caps()->has_sse2) {
   if (dst_type.sign) {
         } else {
      if (util_get_cpu_caps()->has_sse4_1) {
            }
   } else if (util_get_cpu_caps()->has_altivec) {
      if (dst_type.sign) {
         } else {
   #if UTIL_ARCH_LITTLE_ENDIAN
         #endif
            }
      case 16:
      if (dst_type.sign) {
      if (util_get_cpu_caps()->has_sse2) {
         #if UTIL_ARCH_LITTLE_ENDIAN
         #endif
                  } else {
      if (util_get_cpu_caps()->has_sse2) {
         #if UTIL_ARCH_LITTLE_ENDIAN
         #endif
                  }
      /* default uses generic shuffle below */
   }
   if (intrinsic) {
      if (src_type.width * src_type.length == 128) {
      LLVMTypeRef intr_vec_type = lp_build_vec_type(gallivm, intr_type);
   if (swap_intrinsic_operands) {
         } else {
         }
   if (dst_vec_type != intr_vec_type) {
            }
   else {
      int num_split = src_type.width * src_type.length / 128;
   int i;
   int nlen = 128 / src_type.width;
   int lo_off = swap_intrinsic_operands ? nlen : 0;
   int hi_off = swap_intrinsic_operands ? 0 : nlen;
   struct lp_type ndst_type = lp_type_unorm(dst_type.width, 128);
   struct lp_type nintr_type = lp_type_unorm(intr_type.width, 128);
   LLVMValueRef tmpres[LP_MAX_VECTOR_WIDTH / 128];
   LLVMValueRef tmplo, tmphi;
                           for (i = 0; i < num_split / 2; i++) {
      tmplo = lp_build_extract_range(gallivm,
         tmphi = lp_build_extract_range(gallivm,
         tmpres[i] = lp_build_intrinsic_binary(builder, intrinsic,
         if (ndst_vec_type != nintr_vec_type) {
            }
   for (i = 0; i < num_split / 2; i++) {
      tmplo = lp_build_extract_range(gallivm,
         tmphi = lp_build_extract_range(gallivm,
         tmpres[i+num_split/2] = lp_build_intrinsic_binary(builder, intrinsic,
               if (ndst_vec_type != nintr_vec_type) {
      tmpres[i+num_split/2] = LLVMBuildBitCast(builder, tmpres[i+num_split/2],
         }
      }
                  /* generic shuffle */
   lo = LLVMBuildBitCast(builder, lo, dst_vec_type, "");
                                 }
         /**
   * Non-interleaved native pack.
   *
   * Similar to lp_build_pack2, but the ordering of values is not
   * guaranteed, other than it will match lp_build_unpack2_native.
   *
   * In particular, with avx2, the lower and upper 128bits of the vectors will
   * be packed independently, so that (with 32bit->16bit values)
   *         (LSB)                                       (MSB)
   *   lo =   l0 __ l1 __ l2 __ l3 __ l4 __ l5 __ l6 __ l7 __
   *   hi =   h0 __ h1 __ h2 __ h3 __ h4 __ h5 __ h6 __ h7 __
   *   res =  l0 l1 l2 l3 h0 h1 h2 h3 l4 l5 l6 l7 h4 h5 h6 h7
   *
   * This will only change the number of bits the values are represented, not the
   * values themselves.
   *
   * It is assumed the values are already clamped into the destination type range.
   * Values outside that range will produce undefined results.
   */
   LLVMValueRef
   lp_build_pack2_native(struct gallivm_state *gallivm,
                           {
      LLVMBuilderRef builder = gallivm->builder;
   struct lp_type intr_type = dst_type;
            assert(!src_type.floating);
   assert(!dst_type.floating);
   assert(src_type.width == dst_type.width * 2);
            /* At this point only have special case for avx2 */
   if (src_type.length * src_type.width == 256 &&
      util_get_cpu_caps()->has_avx2) {
   switch(src_type.width) {
   case 32:
      if (dst_type.sign) {
         } else {
         }
      case 16:
      if (dst_type.sign) {
         } else {
         }
         }
   if (intrinsic) {
      LLVMTypeRef intr_vec_type = lp_build_vec_type(gallivm, intr_type);
   return lp_build_intrinsic_binary(builder, intrinsic, intr_vec_type,
      }
   else {
            }
      /**
   * Non-interleaved pack and saturate.
   *
   * Same as lp_build_pack2 but will saturate values so that they fit into the
   * destination type.
   */
   LLVMValueRef
   lp_build_packs2(struct gallivm_state *gallivm,
                  struct lp_type src_type,
      {
               assert(!src_type.floating);
   assert(!dst_type.floating);
   assert(src_type.sign == dst_type.sign);
   assert(src_type.width == dst_type.width * 2);
                     /* All X86 SSE non-interleaved pack instructions take signed inputs and
   * saturate them, so no need to clamp for those cases. */
   if(util_get_cpu_caps()->has_sse2 &&
      src_type.width * src_type.length >= 128 &&
   src_type.sign &&
   (src_type.width == 32 || src_type.width == 16))
         if(clamp) {
      struct lp_build_context bld;
   unsigned dst_bits = dst_type.sign ? dst_type.width - 1 : dst_type.width;
   LLVMValueRef dst_max = lp_build_const_int_vec(gallivm, src_type,
         lp_build_context_init(&bld, gallivm, src_type);
   lo = lp_build_min(&bld, lo, dst_max);
   hi = lp_build_min(&bld, hi, dst_max);
                  }
         /**
   * Truncate the bit width.
   *
   * TODO: Handle saturation consistently.
   */
   LLVMValueRef
   lp_build_pack(struct gallivm_state *gallivm,
               struct lp_type src_type,
   struct lp_type dst_type,
   {
      LLVMValueRef (*pack2)(struct gallivm_state *gallivm,
                           LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH];
            /* Register width must remain constant */
            /* We must not loose or gain channels. Only precision */
            if(clamped)
         else
            for(i = 0; i < num_srcs; ++i)
            while(src_type.width > dst_type.width) {
               tmp_type.width /= 2;
            /* Take in consideration the sign changes only in the last step */
   if(tmp_type.width == dst_type.width)
                     for(i = 0; i < num_srcs; ++i)
                                          }
         /**
   * Truncate or expand the bitwidth.
   *
   * NOTE: Getting the right sign flags is crucial here, as we employ some
   * intrinsics that do saturation.
   */
   void
   lp_build_resize(struct gallivm_state *gallivm,
                  struct lp_type src_type,
      {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH];
            /*
   * We don't support float <-> int conversion here. That must be done
   * before/after calling this function.
   */
            /*
   * We don't support double <-> float conversion yet, although it could be
   * added with little effort.
   */
   assert((!src_type.floating && !dst_type.floating) ||
            /* We must not loose or gain channels. Only precision */
            assert(src_type.length <= LP_MAX_VECTOR_LENGTH);
   assert(dst_type.length <= LP_MAX_VECTOR_LENGTH);
   assert(num_srcs <= LP_MAX_VECTOR_LENGTH);
            if (src_type.width > dst_type.width) {
      /*
   * Truncate bit width.
            /* Conversion must be M:1 */
            if (src_type.width * src_type.length == dst_type.width * dst_type.length) {
   /*
      * Register width remains constant -- use vector packing intrinsics
   */
      }
   else {
      if (src_type.width / dst_type.width > num_srcs) {
      /*
   * First change src vectors size (with shuffle) so they have the
   * same size as the destination vector, then pack normally.
   * Note: cannot use cast/extract because llvm generates atrocious code.
   */
   unsigned size_ratio = (src_type.width * src_type.length) /
                  for (i = 0; i < size_ratio * num_srcs; i++) {
      unsigned start_index = (i % size_ratio) * new_length;
   tmp[i] = lp_build_extract_range(gallivm, src[i / size_ratio],
      }
   num_srcs *= size_ratio;
   src_type.length = new_length;
      }
   else {
      /*
   * Truncate bit width but expand vector size - first pack
   * then expand simply because this should be more AVX-friendly
   * for the cases we probably hit.
   */
   unsigned size_ratio = (dst_type.width * dst_type.length) /
                        for (i = 0; i < size_ratio; i++) {
      tmp[i] = lp_build_pack(gallivm, src_type, dst_type, true,
      }
            }
   else if (src_type.width < dst_type.width) {
      /*
   * Expand bit width.
            /* Conversion must be 1:N */
            if (src_type.width * src_type.length == dst_type.width * dst_type.length) {
      /*
   * Register width remains constant -- use vector unpack intrinsics
   */
      }
   else {
      /*
   * Do it element-wise.
                  for (i = 0; i < num_dsts; i++) {
                  for (i = 0; i < src_type.length; ++i) {
      unsigned j = i / dst_type.length;
   LLVMValueRef srcindex = lp_build_const_int32(gallivm, i);
                  if (src_type.sign && dst_type.sign) {
         } else {
         }
            }
   else {
      /*
   * No-op
            /* "Conversion" must be N:N */
            for(i = 0; i < num_dsts; ++i)
               for(i = 0; i < num_dsts; ++i)
      }
         /**
   * Expands src vector from src.length to dst_length
   */
   LLVMValueRef
   lp_build_pad_vector(struct gallivm_state *gallivm,
               {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   LLVMValueRef undef;
   LLVMTypeRef type;
                     if (LLVMGetTypeKind(type) != LLVMVectorTypeKind) {
      /* Can't use ShuffleVector on non-vector type */
   undef = LLVMGetUndef(LLVMVectorType(type, dst_length));
               undef      = LLVMGetUndef(type);
            assert(dst_length <= ARRAY_SIZE(elems));
            if (src_length == dst_length)
            /* All elements from src vector */
   for (i = 0; i < src_length; ++i)
            /* Undef fill remaining space */
   for (i = src_length; i < dst_length; ++i)
            /* Combine the two vectors */
      }
