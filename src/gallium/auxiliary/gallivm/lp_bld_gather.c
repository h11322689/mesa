   /**************************************************************************
   *
   * Copyright 2010 VMware, Inc.
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
         #include "util/u_debug.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_math.h"
   #include "lp_bld_debug.h"
   #include "lp_bld_const.h"
   #include "lp_bld_format.h"
   #include "lp_bld_gather.h"
   #include "lp_bld_swizzle.h"
   #include "lp_bld_type.h"
   #include "lp_bld_init.h"
   #include "lp_bld_intr.h"
   #include "lp_bld_pack.h"
         /**
   * Get the pointer to one element from scatter positions in memory.
   *
   * @sa lp_build_gather()
   */
   LLVMValueRef
   lp_build_gather_elem_ptr(struct gallivm_state *gallivm,
                           {
      LLVMValueRef offset;
            ASSERTED LLVMTypeRef element_type = LLVMInt8TypeInContext(gallivm->context);
            if (length == 1) {
      assert(i == 0);
      } else {
      LLVMValueRef index = lp_build_const_int32(gallivm, i);
                           }
         /**
   * Gather one element from scatter positions in memory.
   *
   * @sa lp_build_gather()
   */
   LLVMValueRef
   lp_build_gather_elem(struct gallivm_state *gallivm,
                        unsigned length,
   unsigned src_width,
   unsigned dst_width,
   bool aligned,
      {
      LLVMTypeRef src_type = LLVMIntTypeInContext(gallivm->context, src_width);
   LLVMTypeRef dst_elem_type = LLVMIntTypeInContext(gallivm->context, dst_width);
   LLVMValueRef ptr;
                     ptr = lp_build_gather_elem_ptr(gallivm, length, base_ptr, offsets, i);
   ptr = LLVMBuildBitCast(gallivm->builder, ptr, LLVMPointerType(src_type, 0), "");
            /* XXX
   * On some archs we probably really want to avoid having to deal
   * with alignments lower than 4 bytes (if fetch size is a power of
   * two >= 32). On x86 it doesn't matter, however.
   * We should be able to guarantee full alignment for any kind of texture
   * fetch (except ARB_texture_buffer_range, oops), but not vertex fetch
   * (there's PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY and friends
   * but I don't think that's quite what we wanted).
   * For ARB_texture_buffer_range, PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT
   * looks like a good fit, but it seems this cap bit (and OpenGL) aren't
   * enforcing what we want (which is what d3d10 does, the offset needs to
   * be aligned to element size, but GL has bytes regardless of element
   * size which would only leave us with minimum alignment restriction of 16
   * which doesn't make much sense if the type isn't 4x32bit). Due to
   * translation of offsets to first_elem in sampler_views it actually seems
   * gallium could not do anything else except 16 no matter what...
   */
   if (!aligned) {
         } else if (!util_is_power_of_two_or_zero(src_width)) {
      /*
   * Full alignment is impossible, assume the caller really meant
   * the individual elements were aligned (e.g. 3x32bit format).
   * And yes the generated code may otherwise crash, llvm will
   * really assume 128bit alignment with a 96bit fetch (I suppose
   * that makes sense as it can just assume the upper 32bit to be
   * whatever).
   * Maybe the caller should be able to explicitly set this, but
   * this should cover all the 3-channel formats.
   */
   if (((src_width / 24) * 24 == src_width) &&
      util_is_power_of_two_or_zero(src_width / 24)) {
      } else {
                     assert(src_width <= dst_width);
   if (src_width < dst_width) {
      res = LLVMBuildZExt(gallivm->builder, res, dst_elem_type, "");
   #if UTIL_ARCH_BIG_ENDIAN
               #endif
                        }
         /**
   * Gather one element from scatter positions in memory.
   * Nearly the same as above, however the individual elements
   * may be vectors themselves, and fetches may be float type.
   * Can also do pad vector instead of ZExt.
   *
   * @sa lp_build_gather()
   */
   static LLVMValueRef
   lp_build_gather_elem_vec(struct gallivm_state *gallivm,
                           unsigned length,
   unsigned src_width,
   LLVMTypeRef src_type,
   struct lp_type dst_type,
   bool aligned,
   {
      LLVMValueRef ptr, res;
            ptr = lp_build_gather_elem_ptr(gallivm, length, base_ptr, offsets, i);
   ptr = LLVMBuildBitCast(gallivm->builder, ptr, LLVMPointerType(src_type, 0), "");
            /* XXX
   * On some archs we probably really want to avoid having to deal
   * with alignments lower than 4 bytes (if fetch size is a power of
   * two >= 32). On x86 it doesn't matter, however.
   * We should be able to guarantee full alignment for any kind of texture
   * fetch (except ARB_texture_buffer_range, oops), but not vertex fetch
   * (there's PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY and friends
   * but I don't think that's quite what we wanted).
   * For ARB_texture_buffer_range, PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT
   * looks like a good fit, but it seems this cap bit (and OpenGL) aren't
   * enforcing what we want (which is what d3d10 does, the offset needs to
   * be aligned to element size, but GL has bytes regardless of element
   * size which would only leave us with minimum alignment restriction of 16
   * which doesn't make much sense if the type isn't 4x32bit). Due to
   * translation of offsets to first_elem in sampler_views it actually seems
   * gallium could not do anything else except 16 no matter what...
   */
   if (!aligned) {
         } else if (!util_is_power_of_two_or_zero(src_width)) {
      /*
   * Full alignment is impossible, assume the caller really meant
   * the individual elements were aligned (e.g. 3x32bit format).
   * And yes the generated code may otherwise crash, llvm will
   * really assume 128bit alignment with a 96bit fetch (I suppose
   * that makes sense as it can just assume the upper 32bit to be
   * whatever).
   * Maybe the caller should be able to explicitly set this, but
   * this should cover all the 3-channel formats.
   */
   if (((src_width / 24) * 24 == src_width) &&
      util_is_power_of_two_or_zero(src_width / 24)) {
      } else {
                     assert(src_width <= dst_type.width * dst_type.length);
   if (src_width < dst_type.width * dst_type.length) {
      if (dst_type.length > 1) {
      res = lp_build_pad_vector(gallivm, res, dst_type.length);
   /*
   * vector_justify hopefully a non-issue since we only deal
   * with src_width >= 32 here?
      } else {
               /*
   * Only valid if src_ptr_type is int type...
         #if UTIL_ARCH_BIG_ENDIAN
            if (vector_justify) {
   res = LLVMBuildShl(gallivm->builder, res,
               }
   if (src_width == 48) {
      /* Load 3x16 bit vector.
   * The sequence of loads on big-endian hardware proceeds as follows.
   * 16-bit fields are denoted by X, Y, Z, and 0.  In memory, the sequence
   * of three fields appears in the order X, Y, Z.
   *
   * Load 32-bit word: 0.0.X.Y
   * Load 16-bit halfword: 0.0.0.Z
   * Rotate left: 0.X.Y.0
   * Bitwise OR: 0.X.Y.Z
   *
   * The order in which we need the fields in the result is 0.Z.Y.X,
   * the same as on little-endian; permute 16-bit fields accordingly
   * within 64-bit register:
   */
   LLVMValueRef shuffles[4] = {
      lp_build_const_int32(gallivm, 2),
   lp_build_const_int32(gallivm, 1),
   lp_build_const_int32(gallivm, 0),
      };
   res = LLVMBuildBitCast(gallivm->builder, res,
         res = LLVMBuildShuffleVector(gallivm->builder, res, res, LLVMConstVector(shuffles, 4), "");
   #endif
            }
      }
               static LLVMValueRef
   lp_build_gather_avx2(struct gallivm_state *gallivm,
                        unsigned length,
      {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef src_type, src_vec_type;
   LLVMValueRef res;
   struct lp_type res_type = dst_type;
            if (dst_type.floating) {
      src_type = src_width == 64 ? LLVMDoubleTypeInContext(gallivm->context) :
      } else {
         }
            /* XXX should allow hw scaling (can handle i8, i16, i32, i64 for x86) */
            if (0) {
      /*
   * XXX: This will cause LLVM pre 3.7 to hang; it works on LLVM 3.8 but
   * will not use the AVX2 gather instrinsics (even with llvm 4.0), at
   * least with Haswell. See
   * http://lists.llvm.org/pipermail/llvm-dev/2016-January/094448.html
   * And the generated code doing the emulation is quite a bit worse
   * than what we get by doing it ourselves too.
   */
   LLVMTypeRef i32_type = LLVMIntTypeInContext(gallivm->context, 32);
   LLVMTypeRef i32_vec_type = LLVMVectorType(i32_type, length);
   LLVMTypeRef i1_type = LLVMIntTypeInContext(gallivm->context, 1);
   LLVMTypeRef i1_vec_type = LLVMVectorType(i1_type, length);
   LLVMTypeRef src_ptr_type = LLVMPointerType(src_type, 0);
                     /* Rescale offsets from bytes to elements */
   LLVMValueRef scale = LLVMConstInt(i32_type, src_width/8, 0);
   scale = lp_build_broadcast(gallivm, i32_vec_type, scale);
   assert(LLVMTypeOf(offsets) == i32_vec_type);
                     char intrinsic[64];
   snprintf(intrinsic, sizeof intrinsic, "llvm.masked.gather.v%u%s%u",
         LLVMValueRef alignment = LLVMConstInt(i32_type, src_width/8, 0);
   LLVMValueRef mask = LLVMConstAllOnes(i1_vec_type);
                        } else {
      LLVMTypeRef i8_type = LLVMIntTypeInContext(gallivm->context, 8);
   const char *intrinsic = NULL;
            assert(src_width == 32 || src_width == 64);
   if (src_width == 32) {
         } else {
                              {{"llvm.x86.avx2.gather.d.d",
   "llvm.x86.avx2.gather.d.d.256"},
                  {{"llvm.x86.avx2.gather.d.ps",
   "llvm.x86.avx2.gather.d.ps.256"},
   {"llvm.x86.avx2.gather.d.pd",
               if ((src_width == 32 && length == 8) ||
      (src_width == 64 && length == 4)) {
      }
            LLVMValueRef passthru = LLVMGetUndef(src_vec_type);
   LLVMValueRef mask = LLVMConstAllOnes(src_vec_type);
   mask = LLVMConstBitCast(mask, src_vec_type);
                        }
               }
         /**
   * Gather elements from scatter positions in memory into a single vector.
   * Use for fetching texels from a texture.
   * For SSE, typical values are length=4, src_width=32, dst_width=32.
   *
   * When src_width < dst_width, the return value can be justified in
   * one of two ways:
   * "integer justification" is used when the caller treats the destination
   * as a packed integer bitmask, as described by the channels' "shift" and
   * "width" fields;
   * "vector justification" is used when the caller casts the destination
   * to a vector and needs channel X to be in vector element 0.
   *
   * @param length length of the offsets
   * @param src_width src element width in bits
   * @param dst_type result element type (src will be expanded to fit,
   *        but truncation is not allowed)
   *        (this may be a vector, must be pot sized)
   * @param aligned whether the data is guaranteed to be aligned (to src_width)
   * @param base_ptr base pointer, needs to be a i8 pointer type.
   * @param offsets vector with offsets
   * @param vector_justify select vector rather than integer justification
   */
   LLVMValueRef
   lp_build_gather(struct gallivm_state *gallivm,
                  unsigned length,
   unsigned src_width,
   struct lp_type dst_type,
   bool aligned,
      {
      LLVMValueRef res;
   bool need_expansion = src_width < dst_type.width * dst_type.length;
   bool vec_fetch;
   struct lp_type fetch_type, fetch_dst_type;
                     /*
   * This is quite a mess...
   * Figure out if the fetch should be done as:
   * a) scalar or vector
   * b) float or int
   *
   * As an example, for a 96bit fetch expanded into 4x32bit, it is better
   * to use (3x32bit) vector type (then pad the vector). Otherwise, the
   * zext will cause extra instructions.
   * However, the same isn't true for 3x16bit (the codegen for that is
   * completely worthless on x86 simd, and for 3x8bit is is way worse
   * still, don't try that... (To get really good code out of llvm for
   * these cases, the only way is to decompose the fetches manually
   * into 1x32bit/1x16bit, or 1x16/1x8bit respectively, although the latter
   * case requires sse41, otherwise simple scalar zext is way better.
   * But probably not important enough, so don't bother.)
   * Also, we try to honor the floating bit of destination (but isn't
   * possible if caller asks for instance for 2x32bit dst_type with
   * 48bit fetch - the idea would be to use 3x16bit fetch, pad and
   * cast to 2x32f type, so the fetch is always int and on top of that
   * we avoid the vec pad and use scalar zext due the above mentioned
   * issue).
   * Note this is optimized for x86 sse2 and up backend. Could be tweaked
   * for other archs if necessary...
   */
   if (((src_width % 32) == 0) && ((src_width % dst_type.width) == 0) &&
      (dst_type.length > 1)) {
   /* use vector fetch (if dst_type is vector) */
   vec_fetch = true;
   if (dst_type.floating) {
         } else {
         }
   /* intentionally not using lp_build_vec_type here */
   src_type = LLVMVectorType(lp_build_elem_type(gallivm, fetch_type),
         fetch_dst_type = fetch_type;
      } else {
      /* use scalar fetch */
   vec_fetch = false;
   if (dst_type.floating && ((src_width == 32) || (src_width == 64))) {
         } else {
         }
   src_type = lp_build_vec_type(gallivm, fetch_type);
   fetch_dst_type = fetch_type;
               if (length == 1) {
      /* Scalar */
   res = lp_build_gather_elem_vec(gallivm, length,
                     return LLVMBuildBitCast(gallivm->builder, res,
         /*
   * Excluding expansion from these paths because if you need it for
   * 32bit/64bit fetches you're doing it wrong (this is gather, not
   * conversion) and it would be awkward for floats.
      } else if (util_get_cpu_caps()->has_avx2 && !need_expansion &&
            return lp_build_gather_avx2(gallivm, length, src_width, dst_type,
      /*
   * This looks bad on paper wrt throughtput/latency on Haswell.
   * Even on Broadwell it doesn't look stellar.
   * Albeit no measurements were done (but tested to work).
   * Should definitely enable on Skylake.
   * (In general, should be more of a win if the fetch is 256bit wide -
   * this is true for the 32bit case above too.)
   */
   } else if (0 && util_get_cpu_caps()->has_avx2 && !need_expansion &&
            return lp_build_gather_avx2(gallivm, length, src_width, dst_type,
      } else {
               LLVMValueRef elems[LP_MAX_VECTOR_WIDTH / 8];
   unsigned i;
   bool vec_zext = false;
   struct lp_type res_type, gather_res_type;
            res_type = fetch_dst_type;
   res_type.length *= length;
            if (src_width == 16 && dst_type.width == 32 && dst_type.length == 1) {
      /*
   * Note that llvm is never able to optimize zext/insert combos
   * directly (i.e. zero the simd reg, then place the elements into
   * the appropriate place directly). (I think this has to do with
   * scalar/vector transition.) And scalar 16->32bit zext simd loads
   * aren't possible (instead loading to scalar reg first).
   * No idea about other archs...
   * We could do this manually, but instead we just use a vector
   * zext, which is simple enough (and, in fact, llvm might optimize
   * this away).
   * (We're not trying that with other bit widths as that might not be
   * easier, in particular with 8 bit values at least with only sse2.)
   */
   assert(vec_fetch == false);
   gather_res_type.width /= 2;
   fetch_dst_type = fetch_type;
   src_type = lp_build_vec_type(gallivm, fetch_type);
      }
   res_t = lp_build_vec_type(gallivm, res_type);
   gather_res_t = lp_build_vec_type(gallivm, gather_res_type);
   res = LLVMGetUndef(gather_res_t);
   for (i = 0; i < length; ++i) {
      LLVMValueRef index = lp_build_const_int32(gallivm, i);
   elems[i] = lp_build_gather_elem_vec(gallivm, length,
                     if (!vec_fetch) {
            }
   if (vec_zext) {
         #if UTIL_ARCH_BIG_ENDIAN
               unsigned sv = dst_type.width - src_width;
   #endif
               }
   if (vec_fetch) {
      /*
   * Do bitcast now otherwise llvm might get some funny ideas wrt
   * float/int types...
   */
   for (i = 0; i < length; i++) {
      elems[i] = LLVMBuildBitCast(gallivm->builder, elems[i],
      }
      } else {
      struct lp_type really_final_type = dst_type;
   assert(res_type.length * res_type.width ==
         really_final_type.length *= length;
   res = LLVMBuildBitCast(gallivm->builder, res,
                     }
      LLVMValueRef
   lp_build_gather_values(struct gallivm_state * gallivm,
               {
      LLVMTypeRef vec_type = LLVMVectorType(LLVMTypeOf(values[0]), value_count);
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef vec = LLVMGetUndef(vec_type);
            for (i = 0; i < value_count; i++) {
      LLVMValueRef index = lp_build_const_int32(gallivm, i);
      }
      }
      LLVMValueRef
   lp_build_masked_gather(struct gallivm_state *gallivm,
                        unsigned length,
      {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef args[4];
         #if LLVM_VERSION_MAJOR >= 16
      snprintf(intrin_name, 64, "llvm.masked.gather.v%ui%u.v%up0",
      #else
      snprintf(intrin_name, 64, "llvm.masked.gather.v%ui%u.v%up0i%u",
      #endif
         args[0] = offset_ptr;
   args[1] = lp_build_const_int32(gallivm, bit_size / 8);
   args[2] = LLVMBuildICmp(builder, LLVMIntNE, exec_mask,
         args[3] = LLVMConstNull(vec_type);
   return lp_build_intrinsic(builder, intrin_name, vec_type,
         }
      void
   lp_build_masked_scatter(struct gallivm_state *gallivm,
                           unsigned length,
   {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef args[4];
         #if LLVM_VERSION_MAJOR >= 16
      snprintf(intrin_name, 64, "llvm.masked.scatter.v%ui%u.v%up0",
      #else
      snprintf(intrin_name, 64, "llvm.masked.scatter.v%ui%u.v%up0i%u",
      #endif
         args[0] = value_vec;
   args[1] = offset_ptr;
   args[2] = lp_build_const_int32(gallivm, bit_size / 8);
   args[3] = LLVMBuildICmp(builder, LLVMIntNE, exec_mask,
         lp_build_intrinsic(builder, intrin_name, LLVMVoidTypeInContext(gallivm->context),
      }
