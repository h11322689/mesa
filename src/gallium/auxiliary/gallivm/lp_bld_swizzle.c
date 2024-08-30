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
   * Helper functions for swizzling/shuffling.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
      #include <inttypes.h>  /* for PRIx64 macro */
   #include "util/compiler.h"
   #include "util/u_debug.h"
      #include "lp_bld_type.h"
   #include "lp_bld_const.h"
   #include "lp_bld_init.h"
   #include "lp_bld_logic.h"
   #include "lp_bld_swizzle.h"
   #include "lp_bld_pack.h"
         LLVMValueRef
   lp_build_broadcast(struct gallivm_state *gallivm,
               {
               if (LLVMGetTypeKind(vec_type) != LLVMVectorTypeKind) {
      /* scalar */
   assert(vec_type == LLVMTypeOf(scalar));
      } else {
      LLVMBuilderRef builder = gallivm->builder;
   const unsigned length = LLVMGetVectorSize(vec_type);
   LLVMValueRef undef = LLVMGetUndef(vec_type);
   /* The shuffle vector is always made of int32 elements */
   LLVMTypeRef i32_type = LLVMInt32TypeInContext(gallivm->context);
                     res = LLVMBuildInsertElement(builder, undef, scalar, LLVMConstNull(i32_type), "");
                  }
         /**
   * Broadcast
   */
   LLVMValueRef
   lp_build_broadcast_scalar(struct lp_build_context *bld,
         {
                  }
         /**
   * Combined extract and broadcast (mere shuffle in most cases)
   */
   LLVMValueRef
   lp_build_extract_broadcast(struct gallivm_state *gallivm,
                           {
      LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
            assert(src_type.floating == dst_type.floating);
            assert(lp_check_value(src_type, vector));
            if (src_type.length == 1) {
      if (dst_type.length == 1) {
      /*
   * Trivial scalar -> scalar.
   */
      } else {
      /*
   * Broadcast scalar -> vector.
   */
   res = lp_build_broadcast(gallivm,
               } else {
      if (dst_type.length > 1) {
      /*
   * shuffle - result can be of different length.
   */
   LLVMValueRef shuffle;
   shuffle = lp_build_broadcast(gallivm,
               res = LLVMBuildShuffleVector(gallivm->builder, vector,
            } else {
      /*
   * Trivial extract scalar from vector.
   */
                     }
         /**
   * Swizzle one channel into other channels.
   */
   LLVMValueRef
   lp_build_swizzle_scalar_aos(struct lp_build_context *bld,
                     {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
            if (a == bld->undef || a == bld->zero || a == bld->one || num_channels == 1)
                     /* XXX: SSE3 has PSHUFB which should be better than bitmasks, but forcing
   * using shuffles here actually causes worst results. More investigation is
   * needed. */
   if (LLVMIsConstant(a) || type.width >= 16) {
      /*
   * Shuffle.
   */
   LLVMTypeRef elem_type = LLVMInt32TypeInContext(bld->gallivm->context);
            for (unsigned j = 0; j < n; j += num_channels)
                     } else if (num_channels == 2) {
      /*
   * Bit mask and shifts
   *
   *   XY XY .... XY  <= input
   *   0Y 0Y .... 0Y
   *   YY YY .... YY
   *   YY YY .... YY  <= output
   */
   struct lp_type type2;
   LLVMValueRef tmp = NULL;
            a = LLVMBuildAnd(builder, a,
                  type2 = type;
   type2.floating = false;
   type2.width *= 2;
                     /*
   * Vector element 0 is always channel X.
   *
   *                        76 54 32 10 (array numbering)
   * Little endian reg in:  YX YX YX YX
   * Little endian reg out: YY YY YY YY if shift right (shift == -1)
   *                        XX XX XX XX if shift left (shift == 1)
   *
   *                        01 23 45 67 (array numbering)
   * Big endian reg in:     XY XY XY XY
   * Big endian reg out:    YY YY YY YY if shift left (shift == 1)
   *                        XX XX XX XX if shift right (shift == -1)
   *
   #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
            if (shift > 0) {
         } else if (shift < 0) {
                  assert(tmp);
   if (tmp) {
                     } else {
      /*
   * Bit mask and recursive shifts
   *
   * Little-endian registers:
   *
   *   7654 3210
   *   WZYX WZYX .... WZYX  <= input
   *   00Y0 00Y0 .... 00Y0  <= mask
   *   00YY 00YY .... 00YY  <= shift right 1 (shift amount -1)
   *   YYYY YYYY .... YYYY  <= shift left 2 (shift amount 2)
   *
   * Big-endian registers:
   *
   *   0123 4567
   *   XYZW XYZW .... XYZW  <= input
   *   0Y00 0Y00 .... 0Y00  <= mask
   *   YY00 YY00 .... YY00  <= shift left 1 (shift amount 1)
   *   YYYY YYYY .... YYYY  <= shift right 2 (shift amount -2)
   *
   * shifts[] gives little-endian shift amounts; we need to negate for big-endian.
   */
   static const int shifts[4][2] = {
      { 1,  2},
   {-1,  2},
   { 1, -2},
               a = LLVMBuildAnd(builder, a,
                  /*
   * Build a type where each element is an integer that cover the four
   * channels.
            struct lp_type type4 = type;
   type4.floating = false;
   type4.width *= 4;
                     for (unsigned i = 0; i < 2; ++i) {
                  #if UTIL_ARCH_BIG_ENDIAN
         #endif
               if (shift > 0)
                        assert(tmp);
   if (tmp)
                     }
         /**
   * Swizzle a vector consisting of an array of XYZW structs.
   *
   * This fills a vector of dst_len length with the swizzled channels from src.
   *
   * e.g. with swizzles = { 2, 1, 0 } and swizzle_count = 6 results in
   *      RGBA RGBA = BGR BGR BG
   *
   * @param swizzles        the swizzle array
   * @param num_swizzles    the number of elements in swizzles
   * @param dst_len         the length of the result
   */
   LLVMValueRef
   lp_build_swizzle_aos_n(struct gallivm_state* gallivm,
                           {
      LLVMBuilderRef builder = gallivm->builder;
                     for (unsigned i = 0; i < dst_len; ++i) {
               if (swizzle == LP_BLD_SWIZZLE_DONTCARE) {
         } else {
                     return LLVMBuildShuffleVector(builder, src,
            }
         LLVMValueRef
   lp_build_swizzle_aos(struct lp_build_context *bld,
               {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_type type = bld->type;
            if (swizzles[0] == PIPE_SWIZZLE_X &&
      swizzles[1] == PIPE_SWIZZLE_Y &&
   swizzles[2] == PIPE_SWIZZLE_Z &&
   swizzles[3] == PIPE_SWIZZLE_W) {
               if (swizzles[0] == swizzles[1] &&
      swizzles[1] == swizzles[2] &&
   swizzles[2] == swizzles[3]) {
   switch (swizzles[0]) {
   case PIPE_SWIZZLE_X:
   case PIPE_SWIZZLE_Y:
   case PIPE_SWIZZLE_Z:
   case PIPE_SWIZZLE_W:
         case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
         case LP_BLD_SWIZZLE_DONTCARE:
         default:
      assert(0);
                  if (LLVMIsConstant(a) ||
      type.width >= 16) {
   /*
   * Shuffle.
   */
   LLVMValueRef undef = LLVMGetUndef(lp_build_elem_type(bld->gallivm, type));
   LLVMTypeRef i32t = LLVMInt32TypeInContext(bld->gallivm->context);
   LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];
                     for (unsigned j = 0; j < n; j += 4) {
      for (unsigned i = 0; i < 4; ++i) {
      unsigned shuffle;
   switch (swizzles[i]) {
   default:
         case PIPE_SWIZZLE_X:
   case PIPE_SWIZZLE_Y:
   case PIPE_SWIZZLE_Z:
   case PIPE_SWIZZLE_W:
      shuffle = j + swizzles[i];
   shuffles[j + i] = LLVMConstInt(i32t, shuffle, 0);
      case PIPE_SWIZZLE_0:
      shuffle = type.length + 0;
   shuffles[j + i] = LLVMConstInt(i32t, shuffle, 0);
   if (!aux[0]) {
         }
      case PIPE_SWIZZLE_1:
      shuffle = type.length + 1;
   shuffles[j + i] = LLVMConstInt(i32t, shuffle, 0);
   if (!aux[1]) {
         }
      case LP_BLD_SWIZZLE_DONTCARE:
      shuffles[j + i] = LLVMGetUndef(i32t);
                     for (unsigned i = 0; i < n; ++i) {
      if (!aux[i]) {
                     return LLVMBuildShuffleVector(builder, a,
            } else {
      /*
   * Bit mask and shifts.
   *
   * For example, this will convert BGRA to RGBA by doing
   *
   * Little endian:
   *   rgba = (bgra & 0x00ff0000) >> 16
   *        | (bgra & 0xff00ff00)
   *        | (bgra & 0x000000ff) << 16
   *
   * Big endian:A
   *   rgba = (bgra & 0x0000ff00) << 16
   *        | (bgra & 0x00ff00ff)
   *        | (bgra & 0xff000000) >> 16
   *
   * This is necessary not only for faster cause, but because X86 backend
   * will refuse shuffles of <4 x i8> vectors
            /*
   * Start with a mixture of 1 and 0.
   */
   unsigned cond = 0;
   for (unsigned chan = 0; chan < 4; ++chan) {
      if (swizzles[chan] == PIPE_SWIZZLE_1) {
            }
   LLVMValueRef res =
            /*
   * Build a type where each element is an integer that cover the four
   * channels.
   */
   struct lp_type type4 = type;
   type4.floating = false;
   type4.width *= 4;
            a = LLVMBuildBitCast(builder, a, lp_build_vec_type(bld->gallivm, type4), "");
            /*
   * Mask and shift the channels, trying to group as many channels in the
   * same shift as possible.  The shift amount is positive for shifts left
   * and negative for shifts right.
   */
   for (int shift = -3; shift <= 3; ++shift) {
                        /*
   * Vector element numbers follow the XYZW order, so 0 is always X,
   * etc.  After widening 4 times we have:
   *
   *                                3210
   * Little-endian register layout: WZYX
   *
   *                                0123
   * Big-endian register layout:    XYZW
   *
   * For little-endian, higher-numbered channels are obtained by a
   * shift right (negative shift amount) and lower-numbered channels by
   * a shift left (positive shift amount).  The opposite is true for
   * big-endian.
   */
   for (unsigned chan = 0; chan < 4; ++chan) {
      #if UTIL_ARCH_LITTLE_ENDIAN
                     #else
                     #endif
                           if (mask) {
      LLVMValueRef masked;
   LLVMValueRef shifted;
                  masked = LLVMBuildAnd(builder, a,
         if (shift > 0) {
      shifted = LLVMBuildShl(builder, masked,
      } else if (shift < 0) {
      shifted = LLVMBuildLShr(builder, masked,
      } else {
                                 return LLVMBuildBitCast(builder, res,
         }
         /**
   * Extended swizzle of a single channel of a SoA vector.
   *
   * @param bld         building context
   * @param unswizzled  array with the 4 unswizzled values
   * @param swizzle     one of the PIPE_SWIZZLE_*
   *
   * @return  the swizzled value.
   */
   LLVMValueRef
   lp_build_swizzle_soa_channel(struct lp_build_context *bld,
               {
      switch (swizzle) {
   case PIPE_SWIZZLE_X:
   case PIPE_SWIZZLE_Y:
   case PIPE_SWIZZLE_Z:
   case PIPE_SWIZZLE_W:
         case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
         default:
      assert(0);
         }
         /**
   * Extended swizzle of a SoA vector.
   *
   * @param bld         building context
   * @param unswizzled  array with the 4 unswizzled values
   * @param swizzles    array of PIPE_SWIZZLE_*
   * @param swizzled    output swizzled values
   */
   void
   lp_build_swizzle_soa(struct lp_build_context *bld,
                     {
      for (unsigned chan = 0; chan < 4; ++chan) {
      swizzled[chan] = lp_build_swizzle_soa_channel(bld, unswizzled,
         }
         /**
   * Do an extended swizzle of a SoA vector inplace.
   *
   * @param bld         building context
   * @param values      intput/output array with the 4 values
   * @param swizzles    array of PIPE_SWIZZLE_*
   */
   void
   lp_build_swizzle_soa_inplace(struct lp_build_context *bld,
               {
               for (unsigned chan = 0; chan < 4; ++chan) {
                     }
         /**
   * Transpose from AOS <-> SOA
   *
   * @param single_type_lp   type of pixels
   * @param src              the 4 * n pixel input
   * @param dst              the 4 * n pixel output
   */
   void
   lp_build_transpose_aos(struct gallivm_state *gallivm,
                     {
      struct lp_type double_type_lp = single_type_lp;
   double_type_lp.length >>= 1;
            LLVMTypeRef double_type = lp_build_vec_type(gallivm, double_type_lp);
            LLVMValueRef double_type_zero = LLVMConstNull(double_type);
            /* Interleave x, y, z, w -> xy and zw */
   if (src[0] || src[1]) {
      LLVMValueRef src0 = src[0];
   LLVMValueRef src1 = src[1];
   if (!src0)
         if (!src1)
         t0 = lp_build_interleave2_half(gallivm, single_type_lp, src0, src1, 0);
            /* Cast to double width type for second interleave */
   t0 = LLVMBuildBitCast(gallivm->builder, t0, double_type, "t0");
      }
   if (src[2] || src[3]) {
      LLVMValueRef src2 = src[2];
   LLVMValueRef src3 = src[3];
   if (!src2)
         if (!src3)
         t1 = lp_build_interleave2_half(gallivm, single_type_lp, src2, src3, 0);
            /* Cast to double width type for second interleave */
   t1 = LLVMBuildBitCast(gallivm->builder, t1, double_type, "t1");
               if (!t0)
         if (!t1)
         if (!t2)
         if (!t3)
            /* Interleave xy, zw -> xyzw */
   dst[0] = lp_build_interleave2_half(gallivm, double_type_lp, t0, t1, 0);
   dst[1] = lp_build_interleave2_half(gallivm, double_type_lp, t0, t1, 1);
   dst[2] = lp_build_interleave2_half(gallivm, double_type_lp, t2, t3, 0);
            /* Cast back to original single width type */
   dst[0] = LLVMBuildBitCast(gallivm->builder, dst[0], single_type, "dst0");
   dst[1] = LLVMBuildBitCast(gallivm->builder, dst[1], single_type, "dst1");
   dst[2] = LLVMBuildBitCast(gallivm->builder, dst[2], single_type, "dst2");
      }
         /**
   * Transpose from AOS <-> SOA for num_srcs
   */
   void
   lp_build_transpose_aos_n(struct gallivm_state *gallivm,
                           {
      switch (num_srcs) {
   case 1:
      dst[0] = src[0];
      case 2:
   {
      /* Note: we must use a temporary incase src == dst */
            lo = lp_build_interleave2_half(gallivm, type, src[0], src[1], 0);
            dst[0] = lo;
   dst[1] = hi;
      }
   case 4:
      lp_build_transpose_aos(gallivm, type, src, dst);
      default:
            }
         /**
   * Pack n-th element of aos values,
   * pad out to destination size.
   * i.e. x1 y1 _ _ x2 y2 _ _ will become x1 x2 _ _
   */
   LLVMValueRef
   lp_build_pack_aos_scalars(struct gallivm_state *gallivm,
                           {
      LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMValueRef undef = LLVMGetUndef(i32t);
   LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];
   unsigned num_src = src_type.length / 4;
                     for (unsigned i = 0; i < num_src; i++) {
         }
   for (unsigned i = num_src; i < num_dst; i++) {
                  if (num_dst == 1) {
         }
   else {
      return LLVMBuildShuffleVector(gallivm->builder, src, src,
         }
         /**
   * Unpack and broadcast packed aos values consisting of only the
   * first value, i.e. x1 x2 _ _ will become x1 x1 x1 x1 x2 x2 x2 x2
   */
   LLVMValueRef
   lp_build_unpack_broadcast_aos_scalars(struct gallivm_state *gallivm,
                     {
      LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];
   unsigned num_dst = dst_type.length;
                     for (unsigned i = 0; i < num_src; i++) {
      shuffles[i*4] = LLVMConstInt(i32t, i, 0);
   shuffles[i*4+1] = LLVMConstInt(i32t, i, 0);
   shuffles[i*4+2] = LLVMConstInt(i32t, i, 0);
               if (num_src == 1) {
      return lp_build_extract_broadcast(gallivm, src_type, dst_type,
      } else {
      return LLVMBuildShuffleVector(gallivm->builder, src, src,
         }
   