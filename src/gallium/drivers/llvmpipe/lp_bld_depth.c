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
   * Depth/stencil testing to LLVM IR translation.
   *
   * To be done accurately/efficiently the depth/stencil test must be done with
   * the same type/format of the depth/stencil buffer, which implies massaging
   * the incoming depths to fit into place. Using a more straightforward
   * type/format for depth/stencil values internally and only convert when
   * flushing would avoid this, but it would most likely result in depth fighting
   * artifacts.
   *
   * Since we're using linear layout for everything, but we need to deal with
   * 2x2 quads, we need to load/store multiple values and swizzle them into
   * place (we could avoid this by doing depth/stencil testing in linear format,
   * which would be easy for late depth/stencil test as we could do that after
   * the fragment shader loop just as we do for color buffers, but more tricky
   * for early depth test as we'd need both masks and interpolated depth in
   * linear format).
   *
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   * @author Brian Paul <jfonseca@vmware.com>
   */
      #include "pipe/p_state.h"
   #include "util/format/u_format.h"
   #include "util/u_cpu_detect.h"
      #include "gallivm/lp_bld_type.h"
   #include "gallivm/lp_bld_arit.h"
   #include "gallivm/lp_bld_bitarit.h"
   #include "gallivm/lp_bld_const.h"
   #include "gallivm/lp_bld_conv.h"
   #include "gallivm/lp_bld_logic.h"
   #include "gallivm/lp_bld_flow.h"
   #include "gallivm/lp_bld_intr.h"
   #include "gallivm/lp_bld_debug.h"
   #include "gallivm/lp_bld_swizzle.h"
   #include "gallivm/lp_bld_pack.h"
      #include "lp_bld_depth.h"
   #include "lp_state_fs.h"
         /** Used to select fields from pipe_stencil_state */
   enum stencil_op {
      S_FAIL_OP,
   Z_FAIL_OP,
      };
            /**
   * Do the stencil test comparison (compare FB stencil values against ref value).
   * This will be used twice when generating two-sided stencil code.
   * \param stencil  the front/back stencil state
   * \param stencilRef  the stencil reference value, replicated as a vector
   * \param stencilVals  vector of stencil values from framebuffer
   * \return vector mask of pass/fail values (~0 or 0)
   */
   static LLVMValueRef
   lp_build_stencil_test_single(struct lp_build_context *bld,
                     {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const unsigned stencilMax = 255; /* XXX fix */
            /*
   * SSE2 has intrinsics for signed comparisons, but not unsigned ones. Values
   * are between 0..255 so ensure we generate the fastest comparisons for
   * wider elements.
   */
   if (type.width <= 8) {
         } else {
                           if (stencil->valuemask != stencilMax) {
      /* compute stencilRef = stencilRef & valuemask */
   LLVMValueRef valuemask = lp_build_const_int_vec(bld->gallivm, type, stencil->valuemask);
   stencilRef = LLVMBuildAnd(builder, stencilRef, valuemask, "");
   /* compute stencilVals = stencilVals & valuemask */
               LLVMValueRef res = lp_build_cmp(bld, stencil->func,
            }
         /**
   * Do the one or two-sided stencil test comparison.
   * \sa lp_build_stencil_test_single
   * \param front_facing  an integer vector mask, indicating front (~0) or back
   *                      (0) facing polygon. If NULL, assume front-facing.
   */
   static LLVMValueRef
   lp_build_stencil_test(struct lp_build_context *bld,
                           {
                        /* do front face test */
   res = lp_build_stencil_test_single(bld, &stencil[0],
            if (stencil[1].enabled && front_facing != NULL) {
      /* do back face test */
            back_res = lp_build_stencil_test_single(bld, &stencil[1],
                           }
         /**
   * Apply the stencil operator (add/sub/keep/etc) to the given vector
   * of stencil values.
   * \return  new stencil values vector
   */
   static LLVMValueRef
   lp_build_stencil_op_single(struct lp_build_context *bld,
                              {
      LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_type type = bld->type;
                     unsigned stencil_op;
   switch (op) {
   case S_FAIL_OP:
      stencil_op = stencil->fail_op;
      case Z_FAIL_OP:
      stencil_op = stencil->zfail_op;
      case Z_PASS_OP:
      stencil_op = stencil->zpass_op;
      default:
      assert(0 && "Invalid stencil_op mode");
               LLVMValueRef res;
   switch (stencil_op) {
   case PIPE_STENCIL_OP_KEEP:
      res = stencilVals;
   /* we can return early for this case */
      case PIPE_STENCIL_OP_ZERO:
      res = bld->zero;
      case PIPE_STENCIL_OP_REPLACE:
      res = stencilRef;
      case PIPE_STENCIL_OP_INCR:
      res = lp_build_add(bld, stencilVals, bld->one);
   res = lp_build_min(bld, res, max);
      case PIPE_STENCIL_OP_DECR:
      res = lp_build_sub(bld, stencilVals, bld->one);
   res = lp_build_max(bld, res, bld->zero);
      case PIPE_STENCIL_OP_INCR_WRAP:
      res = lp_build_add(bld, stencilVals, bld->one);
   res = LLVMBuildAnd(builder, res, max, "");
      case PIPE_STENCIL_OP_DECR_WRAP:
      res = lp_build_sub(bld, stencilVals, bld->one);
   res = LLVMBuildAnd(builder, res, max, "");
      case PIPE_STENCIL_OP_INVERT:
      res = LLVMBuildNot(builder, stencilVals, "");
   res = LLVMBuildAnd(builder, res, max, "");
      default:
      assert(0 && "bad stencil op mode");
                  }
         /**
   * Do the one or two-sided stencil test op/update.
   */
   static LLVMValueRef
   lp_build_stencil_op(struct lp_build_context *bld,
                     const struct pipe_stencil_state stencil[2],
   enum stencil_op op,
   LLVMValueRef stencilRefs[2],
      {
      LLVMBuilderRef builder = bld->gallivm->builder;
                     /* do front face op */
   res = lp_build_stencil_op_single(bld, &stencil[0], op,
            if (stencil[1].enabled && front_facing != NULL) {
      /* do back face op */
            back_res = lp_build_stencil_op_single(bld, &stencil[1], op,
                        if (stencil[0].writemask != 0xff ||
      (stencil[1].enabled && front_facing != NULL &&
   stencil[1].writemask != 0xff)) {
   /* mask &= stencil[0].writemask */
   LLVMValueRef writemask = lp_build_const_int_vec(bld->gallivm, bld->type,
         if (stencil[1].enabled &&
      stencil[1].writemask != stencil[0].writemask &&
   front_facing != NULL) {
   LLVMValueRef back_writemask =
      lp_build_const_int_vec(bld->gallivm, bld->type,
      writemask = lp_build_select(bld, front_facing,
               mask = LLVMBuildAnd(builder, mask, writemask, "");
   /* res = (res & mask) | (stencilVals & ~mask) */
      } else {
      /* res = mask ? res : stencilVals */
                  }
            /**
   * Return a type that matches the depth/stencil format.
   */
   struct lp_type
   lp_depth_type(const struct util_format_description *format_desc,
         {
      struct lp_type type;
            assert(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS);
   assert(format_desc->block.width == 1);
            memset(&type, 0, sizeof type);
            z_swizzle = format_desc->swizzle[0];
   if (z_swizzle < 4) {
      if (format_desc->channel[z_swizzle].type == UTIL_FORMAT_TYPE_FLOAT) {
      type.floating = true;
   assert(z_swizzle == 0);
      }
   else if (format_desc->channel[z_swizzle].type == UTIL_FORMAT_TYPE_UNSIGNED) {
      assert(format_desc->block.bits <= 32);
   assert(format_desc->channel[z_swizzle].normalized);
   if (format_desc->channel[z_swizzle].size < format_desc->block.bits) {
      /* Prefer signed integers when possible, as SSE has less support
   * for unsigned comparison;
   */
         }
   else
                           }
         /**
   * Compute bitmask and bit shift to apply to the incoming fragment Z values
   * and the Z buffer values needed before doing the Z comparison.
   *
   * Note that we leave the Z bits in the position that we find them
   * in the Z buffer (typically 0xffffff00 or 0x00ffffff).  That lets us
   * get by with fewer bit twiddling steps.
   */
   static bool
   get_z_shift_and_mask(const struct util_format_description *format_desc,
         {
      unsigned total_bits;
            assert(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS);
   assert(format_desc->block.width == 1);
            /* 64bit d/s format is special already extracted 32 bits */
                     if (z_swizzle == PIPE_SWIZZLE_NONE)
            *width = format_desc->channel[z_swizzle].size;
   /* & 31 is for the same reason as the 32-bit limit above */
            if (*width == total_bits) {
         } else {
                     }
         /**
   * Compute bitmask and bit shift to apply to the framebuffer pixel values
   * to put the stencil bits in the least significant position.
   * (i.e. 0x000000ff)
   */
   static bool
   get_s_shift_and_mask(const struct util_format_description *format_desc,
         {
               if (s_swizzle == PIPE_SWIZZLE_NONE)
            /* just special case 64bit d/s format */
   if (format_desc->block.bits > 32) {
      /* XXX big-endian? */
   assert(format_desc->format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT);
   *shift = 0;
   *mask = 0xff;
               *shift = format_desc->channel[s_swizzle].shift;
   const unsigned sz = format_desc->channel[s_swizzle].size;
               }
         /**
   * Perform the occlusion test and increase the counter.
   * Test the depth mask. Add the number of channel which has none zero mask
   * into the occlusion counter. e.g. maskvalue is {-1, -1, -1, -1}.
   * The counter will add 4.
   * TODO: could get that out of the fs loop.
   *
   * \param type holds element type of the mask vector.
   * \param maskvalue is the depth test mask.
   * \param counter is a pointer of the uint32 counter.
   */
   void
   lp_build_occlusion_count(struct gallivm_state *gallivm,
                     {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMContextRef context = gallivm->context;
   LLVMValueRef countmask = lp_build_const_int_vec(gallivm, type, 1);
            assert(type.length <= 16);
            if (util_get_cpu_caps()->has_sse && type.length == 4) {
      const char *movmskintr = "llvm.x86.sse.movmsk.ps";
   const char *popcntintr = "llvm.ctpop.i32";
   LLVMValueRef bits = LLVMBuildBitCast(builder, maskvalue,
         bits = lp_build_intrinsic_unary(builder, movmskintr,
         count = lp_build_intrinsic_unary(builder, popcntintr,
            }
   else if (util_get_cpu_caps()->has_avx && type.length == 8) {
      const char *movmskintr = "llvm.x86.avx.movmsk.ps.256";
   const char *popcntintr = "llvm.ctpop.i32";
   LLVMValueRef bits = LLVMBuildBitCast(builder, maskvalue,
         bits = lp_build_intrinsic_unary(builder, movmskintr,
         count = lp_build_intrinsic_unary(builder, popcntintr,
            } else {
      LLVMValueRef countv = LLVMBuildAnd(builder, maskvalue, countmask, "countv");
   LLVMTypeRef counttype = LLVMIntTypeInContext(context, type.length * 8);
   LLVMTypeRef i8vntype = LLVMVectorType(LLVMInt8TypeInContext(context), type.length * 4);
   LLVMValueRef shufflev, countd;
   LLVMValueRef shuffles[16];
                     #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
                  shufflev = LLVMConstVector(shuffles, type.length);
   countd = LLVMBuildShuffleVector(builder, countv, LLVMGetUndef(i8vntype), shufflev, "");
            /*
   * XXX FIXME
   * this is bad on cpus without popcount (on x86 supported by intel
   * nehalem, amd barcelona, and up - not tied to sse42).
   * Would be much faster to just sum the 4 elements of the vector with
   * some horizontal add (shuffle/add/shuffle/add after the initial and).
   */
   switch (type.length) {
   case 4:
      popcntintr = "llvm.ctpop.i32";
      case 8:
      popcntintr = "llvm.ctpop.i64";
      case 16:
      popcntintr = "llvm.ctpop.i128";
      default:
         }
            if (type.length > 8) {
         }
   else if (type.length < 8) {
            }
   newcount = LLVMBuildLoad2(builder, LLVMTypeOf(count), counter, "origcount");
   newcount = LLVMBuildAdd(builder, newcount, count, "newcount");
      }
         /**
   * Load depth/stencil values.
   * The stored values are linear, swizzle them.
   *
   * \param type  the data type of the fragment depth/stencil values
   * \param format_desc  description of the depth/stencil surface
   * \param is_1d  whether this resource has only one dimension
   * \param loop_counter  the current loop iteration
   * \param depth_ptr  pointer to the depth/stencil values of this 4x4 block
   * \param depth_stride  stride of the depth/stencil buffer
   * \param z_fb  contains z values loaded from fb (may include padding)
   * \param s_fb  contains s values loaded from fb (may include padding)
   */
   void
   lp_build_depth_stencil_load_swizzled(struct gallivm_state *gallivm,
                                       struct lp_type z_src_type,
   const struct util_format_description *format_desc,
   {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH / 4];
   LLVMValueRef depth_offset1, depth_offset2;
   const unsigned depth_bytes = format_desc->block.bits / 8;
            struct lp_type zs_load_type = zs_type;
                     if (z_src_type.length == 4) {
      LLVMValueRef looplsb = LLVMBuildAnd(builder, loop_counter,
         LLVMValueRef loopmsb = LLVMBuildAnd(builder, loop_counter,
         LLVMValueRef offset2 = LLVMBuildMul(builder, loopmsb,
         depth_offset1 = LLVMBuildMul(builder, looplsb,
                  /* just concatenate the loaded 2x2 values into 4-wide vector */
   for (unsigned i = 0; i < 4; i++) {
            } else {
      unsigned i;
   LLVMValueRef loopx2 = LLVMBuildShl(builder, loop_counter,
         assert(z_src_type.length == 8);
   depth_offset1 = LLVMBuildMul(builder, loopx2, depth_stride, "");
   /*
   * We load 2x4 values, and need to swizzle them (order
   * 0,1,4,5,2,3,6,7) - not so hot with avx unfortunately.
   */
   for (i = 0; i < 8; i++) {
                              /* Load current z/stencil values from z/stencil buffer */
   LLVMTypeRef load_ptr_type = LLVMPointerType(zs_dst_type, 0);
   LLVMTypeRef int8_type = LLVMInt8TypeInContext(gallivm->context);
   LLVMValueRef zs_dst_ptr =
         zs_dst_ptr = LLVMBuildBitCast(builder, zs_dst_ptr, load_ptr_type, "");
   LLVMValueRef zs_dst1 = LLVMBuildLoad2(builder, zs_dst_type, zs_dst_ptr, "");
   LLVMValueRef zs_dst2;
   if (is_1d) {
         } else {
      zs_dst_ptr = LLVMBuildGEP2(builder, int8_type, depth_ptr, &depth_offset2, 1, "");
   zs_dst_ptr = LLVMBuildBitCast(builder, zs_dst_ptr, load_ptr_type, "");
               *z_fb = LLVMBuildShuffleVector(builder, zs_dst1, zs_dst2,
                  if (format_desc->block.bits == 8) {
      /* Extend stencil-only 8 bit values (S8_UINT) */
   *s_fb = LLVMBuildZExt(builder, *s_fb,
               if (format_desc->block.bits < z_src_type.width) {
      /* Extend destination ZS values (e.g., when reading from Z16_UNORM) */
   *z_fb = LLVMBuildZExt(builder, *z_fb,
               else if (format_desc->block.bits > 32) {
      /* rely on llvm to handle too wide vector we have here nicely */
   struct lp_type typex2 = zs_type;
   struct lp_type s_type = zs_type;
   LLVMValueRef shuffles1[LP_MAX_VECTOR_LENGTH / 4];
   LLVMValueRef shuffles2[LP_MAX_VECTOR_LENGTH / 4];
            typex2.width = typex2.width / 2;
   typex2.length = typex2.length * 2;
   s_type.width = s_type.width / 2;
            tmp = LLVMBuildBitCast(builder, *z_fb,
            for (unsigned i = 0; i < zs_type.length; i++) {
      shuffles1[i] = lp_build_const_int32(gallivm, i * 2);
      }
   *z_fb = LLVMBuildShuffleVector(builder, tmp, tmp,
         *s_fb = LLVMBuildShuffleVector(builder, tmp, tmp,
         *s_fb = LLVMBuildBitCast(builder, *s_fb,
                     lp_build_name(*z_fb, "z_dst");
   lp_build_name(*s_fb, "s_dst");
      }
         /**
   * Store depth/stencil values.
   * Incoming values are swizzled (typically n 2x2 quads), stored linear.
   * If there's a mask it will do select/store otherwise just store.
   *
   * \param type  the data type of the fragment depth/stencil values
   * \param format_desc  description of the depth/stencil surface
   * \param is_1d  whether this resource has only one dimension
   * \param mask_value the alive/dead pixel mask for the quad (vector)
   * \param z_fb  z values read from fb (with padding)
   * \param s_fb  s values read from fb (with padding)
   * \param loop_counter  the current loop iteration
   * \param depth_ptr  pointer to the depth/stencil values of this 4x4 block
   * \param depth_stride  stride of the depth/stencil buffer
   * \param z_value the depth values to store (with padding)
   * \param s_value the stencil values to store (with padding)
   */
   void
   lp_build_depth_stencil_write_swizzled(struct gallivm_state *gallivm,
                                       struct lp_type z_src_type,
   const struct util_format_description *format_desc,
   bool is_1d,
   LLVMValueRef mask_value,
   LLVMValueRef z_fb,
   {
      struct lp_build_context z_bld;
   LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH / 4];
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef zs_dst1, zs_dst2;
   LLVMValueRef zs_dst_ptr1, zs_dst_ptr2;
   LLVMValueRef depth_offset1, depth_offset2;
   LLVMTypeRef load_ptr_type;
   unsigned depth_bytes = format_desc->block.bits / 8;
   struct lp_type zs_type = lp_depth_type(format_desc, z_src_type.length);
   struct lp_type z_type = zs_type;
            zs_load_type.length = zs_load_type.length / 2;
                              /*
   * This is far from ideal, at least for late depth write we should do this
   * outside the fs loop to avoid all the swizzle stuff.
   */
   if (z_src_type.length == 4) {
      LLVMValueRef looplsb = LLVMBuildAnd(builder, loop_counter,
         LLVMValueRef loopmsb = LLVMBuildAnd(builder, loop_counter,
         LLVMValueRef offset2 = LLVMBuildMul(builder, loopmsb,
         depth_offset1 = LLVMBuildMul(builder, looplsb,
            } else {
      LLVMValueRef loopx2 = LLVMBuildShl(builder, loop_counter,
         assert(z_src_type.length == 8);
   depth_offset1 = LLVMBuildMul(builder, loopx2, depth_stride, "");
   /*
   * We load 2x4 values, and need to swizzle them (order
   * 0,1,4,5,2,3,6,7) - not so hot with avx unfortunately.
   */
   for (unsigned i = 0; i < 8; i++) {
                              LLVMTypeRef int8_type = LLVMInt8TypeInContext(gallivm->context);
   zs_dst_ptr1 = LLVMBuildGEP2(builder, int8_type, depth_ptr, &depth_offset1, 1, "");
   zs_dst_ptr1 = LLVMBuildBitCast(builder, zs_dst_ptr1, load_ptr_type, "");
   zs_dst_ptr2 = LLVMBuildGEP2(builder, int8_type, depth_ptr, &depth_offset2, 1, "");
            if (format_desc->block.bits > 32) {
                  if (mask_value) {
      z_value = lp_build_select(&z_bld, mask_value, z_value, z_fb);
   if (format_desc->block.bits > 32) {
      s_fb = LLVMBuildBitCast(builder, s_fb, z_bld.vec_type, "");
                  if (zs_type.width < z_src_type.width) {
      /* Truncate ZS values (e.g., when writing to Z16_UNORM) */
   z_value = LLVMBuildTrunc(builder, z_value,
               if (format_desc->block.bits <= 32) {
      if (z_src_type.length == 4) {
      zs_dst1 = lp_build_extract_range(gallivm, z_value, 0, 2);
      } else {
      assert(z_src_type.length == 8);
   zs_dst1 = LLVMBuildShuffleVector(builder, z_value, z_value,
               zs_dst2 = LLVMBuildShuffleVector(builder, z_value, z_value,
               } else {
      if (z_src_type.length == 4) {
      zs_dst1 = lp_build_interleave2(gallivm, z_type,
         zs_dst2 = lp_build_interleave2(gallivm, z_type,
      } else {
      LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH / 2];
   assert(z_src_type.length == 8);
   for (unsigned i = 0; i < 8; i++) {
      shuffles[i*2] = lp_build_const_int32(gallivm, (i&1) + (i&2) * 2 + (i&4) / 2);
   shuffles[i*2+1] = lp_build_const_int32(gallivm, (i&1) + (i&2) * 2 + (i&4) / 2 +
      }
   zs_dst1 = LLVMBuildShuffleVector(builder, z_value, s_value,
               zs_dst2 = LLVMBuildShuffleVector(builder, z_value, s_value,
            }
   zs_dst1 = LLVMBuildBitCast(builder, zs_dst1,
         zs_dst2 = LLVMBuildBitCast(builder, zs_dst2,
               LLVMBuildStore(builder, zs_dst1, zs_dst_ptr1);
   if (!is_1d) {
            }
         /**
   * Generate code for performing depth and/or stencil tests.
   * We operate on a vector of values (typically n 2x2 quads).
   *
   * \param depth  the depth test state
   * \param stencil  the front/back stencil state
   * \param type  the data type of the fragment depth/stencil values
   * \param format_desc  description of the depth/stencil surface
   * \param mask  the alive/dead pixel mask for the quad (vector)
   * \param cov_mask coverage mask
   * \param stencil_refs  the front/back stencil ref values (scalar)
   * \param z_src  the incoming depth/stencil values (n 2x2 quad values, float32)
   * \param zs_dst  the depth/stencil values in framebuffer
   * \param face  contains boolean value indicating front/back facing polygon
   */
   void
   lp_build_depth_stencil_test(struct gallivm_state *gallivm,
                              const struct lp_depth_state *depth,
   const struct pipe_stencil_state stencil[2],
   struct lp_type z_src_type,
   const struct util_format_description *format_desc,
   struct lp_build_mask_context *mask,
   LLVMValueRef *cov_mask,
   LLVMValueRef stencil_refs[2],
   LLVMValueRef z_src,
   LLVMValueRef z_fb,
   LLVMValueRef s_fb,
      {
      LLVMBuilderRef builder = gallivm->builder;
   struct lp_type z_type;
   struct lp_build_context z_bld;
   struct lp_build_context s_bld;
   struct lp_type s_type;
   unsigned z_shift = 0, z_width = 0, z_mask = 0;
   LLVMValueRef z_dst = NULL;
   LLVMValueRef stencil_vals = NULL;
   LLVMValueRef z_bitmask = NULL, stencil_shift = NULL;
   LLVMValueRef z_pass = NULL, s_pass_mask = NULL;
   LLVMValueRef current_mask = mask ? lp_build_mask_value(mask) : *cov_mask;
   LLVMValueRef front_facing = NULL;
            /*
   * Depths are expected to be between 0 and 1, even if they are stored in
   * floats. Setting these bits here will ensure that the lp_build_conv() call
   * below won't try to unnecessarily clamp the incoming values.
   * If depths are expected outside 0..1 don't set these bits.
   */
   if (z_src_type.floating) {
      if (restrict_depth) {
      z_src_type.sign = false;
         } else {
      assert(!z_src_type.sign);
               /* Pick the type matching the depth-stencil format. */
            /* Pick the intermediate type for depth operations. */
   z_type.width = z_src_type.width;
            /* FIXME: for non-float depth/stencil might generate better code
   * if we'd always split it up to use 128bit operations.
   * For stencil we'd almost certainly want to pack to 8xi16 values,
   * for z just run twice.
            /* Sanity checking */
   {
      ASSERTED const unsigned z_swizzle = format_desc->swizzle[0];
            assert(z_swizzle != PIPE_SWIZZLE_NONE ||
                     assert(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS);
   assert(format_desc->block.width == 1);
            if (stencil[0].enabled) {
      assert(s_swizzle < 4);
   assert(format_desc->channel[s_swizzle].type == UTIL_FORMAT_TYPE_UNSIGNED);
   assert(format_desc->channel[s_swizzle].pure_integer);
   assert(!format_desc->channel[s_swizzle].normalized);
               if (depth->enabled) {
      assert(z_swizzle < 4);
   if (z_type.floating) {
      assert(z_swizzle == 0);
   assert(format_desc->channel[z_swizzle].type ==
            } else {
      assert(format_desc->channel[z_swizzle].type ==
         assert(format_desc->channel[z_swizzle].normalized);
                        /* Setup build context for Z vals */
            /* Setup build context for stencil vals */
   s_type = lp_int_type(z_type);
            /* Compute and apply the Z/stencil bitmasks and shifts.
   */
   {
               z_dst = z_fb;
            have_z = get_z_shift_and_mask(format_desc, &z_shift, &z_width, &z_mask);
            if (have_z) {
      if (z_mask != 0xffffffff) {
                  /*
   * Align the framebuffer Z 's LSB to the right.
   */
   if (z_shift) {
      LLVMValueRef shift = lp_build_const_int_vec(gallivm, z_type, z_shift);
      } else if (z_bitmask) {
         } else {
                     if (have_s) {
      if (s_shift) {
      LLVMValueRef shift = lp_build_const_int_vec(gallivm, s_type, s_shift);
   stencil_vals = LLVMBuildLShr(builder, stencil_vals, shift, "");
               if (s_mask != 0xffffffff) {
      LLVMValueRef mask = lp_build_const_int_vec(gallivm, s_type, s_mask);
                                          if (face) {
      if (0) {
      /*
   * XXX: the scalar expansion below produces atrocious code
   * (basically producing a 64bit scalar value, then moving the 2
   * 32bit pieces separately to simd, plus 4 shuffles, which is
   * seriously lame). But the scalar-simd transitions are always
   * tricky, so no big surprise there.
   * This here would be way better, however llvm has some serious
   * trouble later using it in the select, probably because it will
   * recognize the expression as constant and move the simd value
   * away (out of the loop) - and then it will suddenly try
   * constructing i1 high-bit masks out of it later...
   * (Try piglit stencil-twoside.)
   * Note this is NOT due to using SExt/Trunc, it fails exactly the
   * same even when using native compare/select.
   * I cannot reproduce this problem when using stand-alone compiler
   * though, suggesting some problem with optimization passes...
   * (With stand-alone compilation, the construction of this mask
   * value, no matter if the easy 3 instruction here or the complex
   * 16+ one below, never gets separated from where it's used.)
   * The scalar code still has the same problem, but the generated
   * code looks a bit better at least for some reason, even if
   * mostly by luck (the fundamental issue clearly is the same).
   */
   front_facing = lp_build_broadcast(gallivm, s_bld.vec_type, face);
   /* front_facing = face != 0 ? ~0 : 0 */
   front_facing = lp_build_compare(gallivm, s_bld.type,
                              /* front_facing = face != 0 ? ~0 : 0 */
   front_facing = LLVMBuildICmp(builder, LLVMIntNE, face, zero, "");
   front_facing = LLVMBuildSExt(builder, front_facing,
                                             s_pass_mask = lp_build_stencil_test(&s_bld, stencil,
                  /* apply stencil-fail operator */
   {
      LLVMValueRef s_fail_mask = lp_build_andnot(&s_bld, current_mask, s_pass_mask);
   stencil_vals = lp_build_stencil_op(&s_bld, stencil, S_FAIL_OP,
                        if (depth->enabled) {
      /*
   * Convert fragment Z to the desired type, aligning the LSB to the right.
            assert(z_type.width == z_src_type.width);
   assert(z_type.length == z_src_type.length);
   assert(lp_check_value(z_src_type, z_src));
   if (z_src_type.floating) {
      /*
                  if (!z_type.floating) {
      z_src = lp_build_clamped_float_to_unsigned_norm(gallivm,
                     } else {
      /*
                  assert(!z_src_type.sign);
   assert(!z_src_type.fixed);
   assert(z_src_type.norm);
   assert(!z_type.floating);
   if (z_src_type.width > z_width) {
      LLVMValueRef shift = lp_build_const_int_vec(gallivm, z_src_type,
               }
                     /* compare src Z to dst Z, returning 'pass' mask */
            /* mask off bits that failed stencil test */
   if (s_pass_mask) {
                  if (!stencil[0].enabled && mask) {
      /* We can potentially skip all remaining operations here, but only
   * if stencil is disabled because we still need to update the stencil
   * buffer values.  Don't need to update Z buffer values.
                  if (do_branch) {
                     if (depth->writemask) {
                              /* Mix the old and new Z buffer values.
   * z_dst[i] = zselectmask[i] ? z_src[i] : z_dst[i]
   */
               if (stencil[0].enabled) {
                     /* apply Z-fail operator */
   z_fail_mask = lp_build_andnot(&s_bld, current_mask, z_pass);
   stencil_vals = lp_build_stencil_op(&s_bld, stencil, Z_FAIL_OP,
                  /* apply Z-pass operator */
   z_pass_mask = LLVMBuildAnd(builder, current_mask, z_pass, "");
   stencil_vals = lp_build_stencil_op(&s_bld, stencil, Z_PASS_OP,
               } else {
      /* No depth test: apply Z-pass operator to stencil buffer values which
   * passed the stencil test.
   */
   s_pass_mask = LLVMBuildAnd(builder, current_mask, s_pass_mask, "");
   stencil_vals = lp_build_stencil_op(&s_bld, stencil, Z_PASS_OP,
                     /* Put Z and stencil bits in the right place */
   if (have_z && z_shift) {
      LLVMValueRef shift = lp_build_const_int_vec(gallivm, z_type, z_shift);
      }
   if (stencil_vals && stencil_shift)
      stencil_vals = LLVMBuildShl(builder, stencil_vals,
         /* Finally, merge the z/stencil values */
   if (format_desc->block.bits <= 32) {
      if (have_z && have_s)
         else if (have_z)
         else
            } else {
      *z_value = z_dst;
               if (mask) {
      if (s_pass_mask)
            if (depth->enabled && stencil[0].enabled)
      } else {
      LLVMValueRef tmp_mask = *cov_mask;
   if (s_pass_mask)
            /* for multisample we don't do the stencil optimisation so update always */
   if (depth->enabled)
               }
   