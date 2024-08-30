   /**************************************************************************
   *
   * Copyright 2019 Red Hat.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **************************************************************************/
      #include "lp_bld_nir.h"
   #include "lp_bld_arit.h"
   #include "lp_bld_bitarit.h"
   #include "lp_bld_const.h"
   #include "lp_bld_conv.h"
   #include "lp_bld_gather.h"
   #include "lp_bld_logic.h"
   #include "lp_bld_quad.h"
   #include "lp_bld_flow.h"
   #include "lp_bld_intr.h"
   #include "lp_bld_struct.h"
   #include "lp_bld_debug.h"
   #include "lp_bld_printf.h"
   #include "nir.h"
   #include "nir_deref.h"
   #include "nir_search_helpers.h"
         // Doing AOS (and linear) codegen?
   static bool
   is_aos(const struct lp_build_nir_context *bld_base)
   {
      // AOS is used for vectors of uint8[16]
      }
         static void
   visit_cf_list(struct lp_build_nir_context *bld_base,
               static LLVMValueRef
   cast_type(struct lp_build_nir_context *bld_base, LLVMValueRef val,
         {
      LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   switch (alu_type) {
   case nir_type_float:
      switch (bit_size) {
   case 16:
         case 32:
         case 64:
         default:
      assert(0);
      }
      case nir_type_int:
      switch (bit_size) {
   case 8:
         case 16:
         case 32:
         case 64:
         default:
      assert(0);
      }
      case nir_type_uint:
      switch (bit_size) {
   case 8:
         case 16:
         case 1:
   case 32:
         case 64:
         default:
      assert(0);
      }
      case nir_type_uint32:
         default:
         }
      }
         static unsigned
   glsl_sampler_to_pipe(int sampler_dim, bool is_array)
   {
      unsigned pipe_target = PIPE_BUFFER;
   switch (sampler_dim) {
   case GLSL_SAMPLER_DIM_1D:
      pipe_target = is_array ? PIPE_TEXTURE_1D_ARRAY : PIPE_TEXTURE_1D;
      case GLSL_SAMPLER_DIM_2D:
      pipe_target = is_array ? PIPE_TEXTURE_2D_ARRAY : PIPE_TEXTURE_2D;
      case GLSL_SAMPLER_DIM_SUBPASS:
   case GLSL_SAMPLER_DIM_SUBPASS_MS:
      pipe_target = PIPE_TEXTURE_2D_ARRAY;
      case GLSL_SAMPLER_DIM_3D:
      pipe_target = PIPE_TEXTURE_3D;
      case GLSL_SAMPLER_DIM_MS:
      pipe_target = is_array ? PIPE_TEXTURE_2D_ARRAY : PIPE_TEXTURE_2D;
      case GLSL_SAMPLER_DIM_CUBE:
      pipe_target = is_array ? PIPE_TEXTURE_CUBE_ARRAY : PIPE_TEXTURE_CUBE;
      case GLSL_SAMPLER_DIM_RECT:
      pipe_target = PIPE_TEXTURE_RECT;
      case GLSL_SAMPLER_DIM_BUF:
      pipe_target = PIPE_BUFFER;
      default:
         }
      }
         static LLVMValueRef
   get_src(struct lp_build_nir_context *bld_base, nir_src src)
   {
         }
         static void
   assign_ssa(struct lp_build_nir_context *bld_base, int idx, LLVMValueRef ptr)
   {
         }
         static void
   assign_ssa_dest(struct lp_build_nir_context *bld_base, const nir_def *ssa,
         {
      if ((ssa->num_components == 1 || is_aos(bld_base))) {
         } else {
      assign_ssa(bld_base, ssa->index,
               }
         static LLVMValueRef
   fcmp32(struct lp_build_nir_context *bld_base,
         enum pipe_compare_func compare,
   uint32_t src_bit_size,
   {
      LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   struct lp_build_context *flt_bld = get_flt_bld(bld_base, src_bit_size);
            if (compare != PIPE_FUNC_NOTEQUAL)
         else
         if (src_bit_size == 64)
         else if (src_bit_size == 16)
            }
         static LLVMValueRef
   icmp32(struct lp_build_nir_context *bld_base,
         enum pipe_compare_func compare,
   bool is_unsigned,
   uint32_t src_bit_size,
   {
      LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   struct lp_build_context *i_bld =
         LLVMValueRef result = lp_build_cmp(i_bld, compare, src[0], src[1]);
   if (src_bit_size < 32)
         else if (src_bit_size == 64)
            }
         /**
   * Get a source register value for an ALU instruction.
   * This is where swizzles are handled.  There should be no negation
   * or absolute value modifiers.
   * num_components indicates the number of components needed in the
   * returned array or vector.
   */
   static LLVMValueRef
   get_alu_src(struct lp_build_nir_context *bld_base,
               {
      assert(num_components >= 1);
            struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   const unsigned src_components = nir_src_num_components(src.src);
   assert(src_components > 0);
   LLVMValueRef value = get_src(bld_base, src.src);
            /* check if swizzling needed for the src vector */
   bool need_swizzle = false;
   for (unsigned i = 0; i < src_components; ++i) {
      if (src.swizzle[i] != i) {
      need_swizzle = true;
                  if (is_aos(bld_base) && !need_swizzle) {
                  if (need_swizzle || num_components != src_components) {
      if (is_aos(bld_base) && need_swizzle) {
                     // swizzle vector of ((r,g,b,a), (r,g,b,a), (r,g,b,a), (r,g,b,a))
                  // Do our own swizzle here since lp_build_swizzle_aos_n() does
   // not do what we want.
   // Ex: value = {r0,g0,b0,a0, r1,g1,b1,a1, r2,g2,b2,a2, r3,g3,b3,a3}.
   // aos swizzle = {2,1,0,3}  // swap red/blue
   // shuffles = {2,1,0,3, 6,5,4,7, 10,9,8,11, 14,13,12,15}
   // result = {b0,g0,r0,a0, b1,g1,r1,a1, b2,g2,r2,a2, b3,g3,r3,a3}.
   LLVMValueRef shuffles[LP_MAX_VECTOR_WIDTH];
   for (unsigned i = 0; i < 16; i++) {
      unsigned chan = i % 4;
   /* apply src register swizzle */
   if (chan < num_components) {
         } else {
         }
   /* apply aos swizzle */
   chan = lp_nir_aos_swizzle(bld_base, chan);
      }
   value = LLVMBuildShuffleVector(builder, value,
            } else if (src_components > 1 && num_components == 1) {
      value = LLVMBuildExtractValue(gallivm->builder, value,
      } else if (src_components == 1 && num_components > 1) {
      LLVMValueRef values[] = {value, value, value, value,
                        } else {
      LLVMValueRef arr = LLVMGetUndef(LLVMArrayType(LLVMTypeOf(LLVMBuildExtractValue(builder, value, 0, "")), num_components));
   for (unsigned i = 0; i < num_components; i++)
                           }
         static LLVMValueRef
   emit_b2f(struct lp_build_nir_context *bld_base,
               {
      LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   LLVMValueRef result =
      LLVMBuildAnd(builder, cast_type(bld_base, src0, nir_type_int, 32),
               LLVMBuildBitCast(builder,
                  result = LLVMBuildBitCast(builder, result, bld_base->base.vec_type, "");
   switch (bitsize) {
   case 16:
      result = LLVMBuildFPTrunc(builder, result,
            case 32:
         case 64:
      result = LLVMBuildFPExt(builder, result,
            default:
         }
      }
         static LLVMValueRef
   emit_b2i(struct lp_build_nir_context *bld_base,
               {
      LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   LLVMValueRef result = LLVMBuildAnd(builder,
                     switch (bitsize) {
   case 8:
         case 16:
         case 32:
         case 64:
         default:
            }
         static LLVMValueRef
   emit_b32csel(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef sel = cast_type(bld_base, src[0], nir_type_int, 32);
   LLVMValueRef v = lp_build_compare(bld_base->base.gallivm, bld_base->int_bld.type, PIPE_FUNC_NOTEQUAL, sel, bld_base->int_bld.zero);
   struct lp_build_context *bld = get_int_bld(bld_base, false, src_bit_size[1]);
      }
         static LLVMValueRef
   split_64bit(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMValueRef shuffles[LP_MAX_VECTOR_WIDTH/32];
   LLVMValueRef shuffles2[LP_MAX_VECTOR_WIDTH/32];
   int len = bld_base->base.type.length * 2;
      #if UTIL_ARCH_LITTLE_ENDIAN
         shuffles[i] = lp_build_const_int32(gallivm, i * 2);
   #else
         shuffles[i] = lp_build_const_int32(gallivm, (i * 2) + 1);
   #endif
               src = LLVMBuildBitCast(gallivm->builder, src,
         return LLVMBuildShuffleVector(gallivm->builder, src,
                        }
         static LLVMValueRef
   merge_64bit(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   int i;
   LLVMValueRef shuffles[2 * (LP_MAX_VECTOR_WIDTH/32)];
   int len = bld_base->base.type.length * 2;
               #if UTIL_ARCH_LITTLE_ENDIAN
         shuffles[i] = lp_build_const_int32(gallivm, i / 2);
   #else
         shuffles[i] = lp_build_const_int32(gallivm, i / 2 + bld_base->base.type.length);
   #endif
      }
      }
         static LLVMValueRef
   split_16bit(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMValueRef shuffles[LP_MAX_VECTOR_WIDTH/32];
   LLVMValueRef shuffles2[LP_MAX_VECTOR_WIDTH/32];
   int len = bld_base->base.type.length * 2;
      #if UTIL_ARCH_LITTLE_ENDIAN
         shuffles[i] = lp_build_const_int32(gallivm, i * 2);
   #else
         shuffles[i] = lp_build_const_int32(gallivm, (i * 2) + 1);
   #endif
               src = LLVMBuildBitCast(gallivm->builder, src, LLVMVectorType(LLVMInt16TypeInContext(gallivm->context), len), "");
   return LLVMBuildShuffleVector(gallivm->builder, src,
                        }
         static LLVMValueRef
   merge_16bit(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   int i;
   LLVMValueRef shuffles[2 * (LP_MAX_VECTOR_WIDTH/32)];
   int len = bld_base->int16_bld.type.length * 2;
               #if UTIL_ARCH_LITTLE_ENDIAN
         shuffles[i] = lp_build_const_int32(gallivm, i / 2);
   #else
         shuffles[i] = lp_build_const_int32(gallivm, i / 2 + bld_base->base.type.length);
   #endif
      }
      }
         static LLVMValueRef
   get_signed_divisor(struct gallivm_state *gallivm,
                     struct lp_build_context *int_bld,
   {
      LLVMBuilderRef builder = gallivm->builder;
   /* However for signed divides SIGFPE can occur if the numerator is INT_MIN
         /* set mask if numerator == INT_MIN */
   long long min_val;
   switch (src_bit_size) {
   case 8:
      min_val = INT8_MIN;
      case 16:
      min_val = INT16_MIN;
      default:
   case 32:
      min_val = INT_MIN;
      case 64:
      min_val = INT64_MIN;
      }
   LLVMValueRef div_mask2 = lp_build_cmp(mask_bld, PIPE_FUNC_EQUAL, src,
         /* set another mask if divisor is - 1 */
   LLVMValueRef div_mask3 = lp_build_cmp(mask_bld, PIPE_FUNC_EQUAL, divisor,
                  divisor = lp_build_select(mask_bld, div_mask2, int_bld->one, divisor);
      }
         static LLVMValueRef
   do_int_divide(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *int_bld = get_int_bld(bld_base, is_unsigned, src_bit_size);
            /* avoid divide by 0. Converted divisor from 0 to -1 */
   LLVMValueRef div_mask = lp_build_cmp(mask_bld, PIPE_FUNC_EQUAL, src2,
            LLVMValueRef divisor = LLVMBuildOr(builder, div_mask, src2, "");
   if (!is_unsigned) {
      divisor = get_signed_divisor(gallivm, int_bld, mask_bld,
      }
            if (!is_unsigned) {
      LLVMValueRef not_div_mask = LLVMBuildNot(builder, div_mask, "");
      } else
      /* udiv by zero is guaranteed to return 0xffffffff at least with d3d10
   * may as well do same for idiv */
   }
         static LLVMValueRef
   do_int_mod(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *int_bld = get_int_bld(bld_base, is_unsigned, src_bit_size);
   struct lp_build_context *mask_bld = get_int_bld(bld_base, true, src_bit_size);
   LLVMValueRef div_mask = lp_build_cmp(mask_bld, PIPE_FUNC_EQUAL, src2,
         LLVMValueRef divisor = LLVMBuildOr(builder,
               if (!is_unsigned) {
      divisor = get_signed_divisor(gallivm, int_bld, mask_bld,
      }
   LLVMValueRef result = lp_build_mod(int_bld, src, divisor);
      }
      static LLVMValueRef
   do_alu_action(struct lp_build_nir_context *bld_base,
               const nir_alu_instr *instr,
   {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
            switch (instr->op) {
   case nir_op_b2f16:
      result = emit_b2f(bld_base, src[0], 16);
      case nir_op_b2f32:
      result = emit_b2f(bld_base, src[0], 32);
      case nir_op_b2f64:
      result = emit_b2f(bld_base, src[0], 64);
      case nir_op_b2i8:
      result = emit_b2i(bld_base, src[0], 8);
      case nir_op_b2i16:
      result = emit_b2i(bld_base, src[0], 16);
      case nir_op_b2i32:
      result = emit_b2i(bld_base, src[0], 32);
      case nir_op_b2i64:
      result = emit_b2i(bld_base, src[0], 64);
      case nir_op_b32csel:
      result = emit_b32csel(bld_base, src_bit_size, src);
      case nir_op_bit_count:
      result = lp_build_popcount(get_int_bld(bld_base, false, src_bit_size[0]), src[0]);
   if (src_bit_size[0] < 32)
         else if (src_bit_size[0] > 32)
            case nir_op_bitfield_select:
      result = lp_build_xor(&bld_base->uint_bld, src[2], lp_build_and(&bld_base->uint_bld, src[0], lp_build_xor(&bld_base->uint_bld, src[1], src[2])));
      case nir_op_bitfield_reverse:
      result = lp_build_bitfield_reverse(get_int_bld(bld_base, false, src_bit_size[0]), src[0]);
      case nir_op_f2f16:
      if (src_bit_size[0] == 64)
      src[0] = LLVMBuildFPTrunc(builder, src[0],
      result = LLVMBuildFPTrunc(builder, src[0],
            case nir_op_f2f32:
      if (src_bit_size[0] < 32)
      result = LLVMBuildFPExt(builder, src[0],
      else
      result = LLVMBuildFPTrunc(builder, src[0],
         case nir_op_f2f64:
      result = LLVMBuildFPExt(builder, src[0],
            case nir_op_f2i8:
      result = LLVMBuildFPToSI(builder,
                  case nir_op_f2i16:
      result = LLVMBuildFPToSI(builder,
                  case nir_op_f2i32:
      result = LLVMBuildFPToSI(builder, src[0], bld_base->base.int_vec_type, "");
      case nir_op_f2u8:
      result = LLVMBuildFPToUI(builder,
                  case nir_op_f2u16:
      result = LLVMBuildFPToUI(builder,
                  case nir_op_f2u32:
      result = LLVMBuildFPToUI(builder,
                  case nir_op_f2i64:
      result = LLVMBuildFPToSI(builder,
                  case nir_op_f2u64:
      result = LLVMBuildFPToUI(builder,
                  case nir_op_fabs:
      result = lp_build_abs(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_fadd:
      result = lp_build_add(get_flt_bld(bld_base, src_bit_size[0]),
            case nir_op_fceil:
      result = lp_build_ceil(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_fcos:
      result = lp_build_cos(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_fddx:
   case nir_op_fddx_coarse:
   case nir_op_fddx_fine:
      result = lp_build_ddx(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_fddy:
   case nir_op_fddy_coarse:
   case nir_op_fddy_fine:
      result = lp_build_ddy(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_fdiv:
      result = lp_build_div(get_flt_bld(bld_base, src_bit_size[0]),
            case nir_op_feq32:
      result = fcmp32(bld_base, PIPE_FUNC_EQUAL, src_bit_size[0], src);
      case nir_op_fexp2:
      result = lp_build_exp2(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_ffloor:
      result = lp_build_floor(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_ffma:
      result = lp_build_fmuladd(builder, src[0], src[1], src[2]);
      case nir_op_ffract: {
      struct lp_build_context *flt_bld = get_flt_bld(bld_base, src_bit_size[0]);
   LLVMValueRef tmp = lp_build_floor(flt_bld, src[0]);
   result = lp_build_sub(flt_bld, src[0], tmp);
      }
   case nir_op_fge:
   case nir_op_fge32:
      result = fcmp32(bld_base, PIPE_FUNC_GEQUAL, src_bit_size[0], src);
      case nir_op_find_lsb: {
      struct lp_build_context *int_bld = get_int_bld(bld_base, false, src_bit_size[0]);
   result = lp_build_cttz(int_bld, src[0]);
   if (src_bit_size[0] < 32)
         else if (src_bit_size[0] > 32)
            }
   case nir_op_fisfinite32:
         case nir_op_flog2:
      result = lp_build_log2_safe(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_flt:
   case nir_op_flt32:
      result = fcmp32(bld_base, PIPE_FUNC_LESS, src_bit_size[0], src);
      case nir_op_fmax:
   case nir_op_fmin: {
      enum gallivm_nan_behavior minmax_nan;
            /* If one of the sources is known to be a number (i.e., not NaN), then
   * better code can be generated by passing that information along.
   */
   if (is_a_number(bld_base->range_ht, instr, 1,
                     } else if (is_a_number(bld_base->range_ht, instr, 0,
                  first = 1;
      } else {
                  if (instr->op == nir_op_fmin) {
      result = lp_build_min_ext(get_flt_bld(bld_base, src_bit_size[0]),
      } else {
      result = lp_build_max_ext(get_flt_bld(bld_base, src_bit_size[0]),
      }
      }
   case nir_op_fmod: {
      struct lp_build_context *flt_bld = get_flt_bld(bld_base, src_bit_size[0]);
   result = lp_build_div(flt_bld, src[0], src[1]);
   result = lp_build_floor(flt_bld, result);
   result = lp_build_mul(flt_bld, src[1], result);
   result = lp_build_sub(flt_bld, src[0], result);
      }
   case nir_op_fmul:
      result = lp_build_mul(get_flt_bld(bld_base, src_bit_size[0]),
            case nir_op_fneu32:
      result = fcmp32(bld_base, PIPE_FUNC_NOTEQUAL, src_bit_size[0], src);
      case nir_op_fneg:
      result = lp_build_negate(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_fpow:
      result = lp_build_pow(get_flt_bld(bld_base, src_bit_size[0]), src[0], src[1]);
      case nir_op_frcp:
      result = lp_build_rcp(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_fround_even:
      if (src_bit_size[0] == 16) {
      struct lp_build_context *bld = get_flt_bld(bld_base, 16);
   char intrinsic[64];
   lp_format_intrinsic(intrinsic, 64, "llvm.roundeven", bld->vec_type);
      } else {
         }
      case nir_op_frsq:
      result = lp_build_rsqrt(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_fsat:
      result = lp_build_clamp_zero_one_nanzero(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_fsign:
      result = lp_build_sgn(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_fsin:
      result = lp_build_sin(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_fsqrt:
      result = lp_build_sqrt(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_ftrunc:
      result = lp_build_trunc(get_flt_bld(bld_base, src_bit_size[0]), src[0]);
      case nir_op_i2f16:
      result = LLVMBuildSIToFP(builder, src[0],
            case nir_op_i2f32:
      result = lp_build_int_to_float(&bld_base->base, src[0]);
      case nir_op_i2f64:
      result = lp_build_int_to_float(&bld_base->dbl_bld, src[0]);
      case nir_op_i2i8:
      result = LLVMBuildTrunc(builder, src[0], bld_base->int8_bld.vec_type, "");
      case nir_op_i2i16:
      if (src_bit_size[0] < 16)
         else
            case nir_op_i2i32:
      if (src_bit_size[0] < 32)
         else
            case nir_op_i2i64:
      result = LLVMBuildSExt(builder, src[0], bld_base->int64_bld.vec_type, "");
      case nir_op_iabs:
      result = lp_build_abs(get_int_bld(bld_base, false, src_bit_size[0]), src[0]);
      case nir_op_iadd:
      result = lp_build_add(get_int_bld(bld_base, false, src_bit_size[0]),
            case nir_op_iand:
      result = lp_build_and(get_int_bld(bld_base, false, src_bit_size[0]),
            case nir_op_idiv:
      result = do_int_divide(bld_base, false, src_bit_size[0], src[0], src[1]);
      case nir_op_ieq32:
      result = icmp32(bld_base, PIPE_FUNC_EQUAL, false, src_bit_size[0], src);
      case nir_op_ige32:
      result = icmp32(bld_base, PIPE_FUNC_GEQUAL, false, src_bit_size[0], src);
      case nir_op_ilt32:
      result = icmp32(bld_base, PIPE_FUNC_LESS, false, src_bit_size[0], src);
      case nir_op_imax:
      result = lp_build_max(get_int_bld(bld_base, false, src_bit_size[0]), src[0], src[1]);
      case nir_op_imin:
      result = lp_build_min(get_int_bld(bld_base, false, src_bit_size[0]), src[0], src[1]);
      case nir_op_imul:
   case nir_op_imul24:
      result = lp_build_mul(get_int_bld(bld_base, false, src_bit_size[0]),
            case nir_op_imul_high: {
      LLVMValueRef hi_bits;
   lp_build_mul_32_lohi(get_int_bld(bld_base, false, src_bit_size[0]), src[0], src[1], &hi_bits);
   result = hi_bits;
      }
   case nir_op_ine32:
      result = icmp32(bld_base, PIPE_FUNC_NOTEQUAL, false, src_bit_size[0], src);
      case nir_op_ineg:
      result = lp_build_negate(get_int_bld(bld_base, false, src_bit_size[0]), src[0]);
      case nir_op_inot:
      result = lp_build_not(get_int_bld(bld_base, false, src_bit_size[0]), src[0]);
      case nir_op_ior:
      result = lp_build_or(get_int_bld(bld_base, false, src_bit_size[0]),
            case nir_op_imod:
   case nir_op_irem:
      result = do_int_mod(bld_base, false, src_bit_size[0], src[0], src[1]);
      case nir_op_ishl: {
      struct lp_build_context *uint_bld = get_int_bld(bld_base, true, src_bit_size[0]);
   struct lp_build_context *int_bld = get_int_bld(bld_base, false, src_bit_size[0]);
   if (src_bit_size[0] == 64)
         if (src_bit_size[0] < 32)
         src[1] = lp_build_and(uint_bld, src[1], lp_build_const_int_vec(gallivm, uint_bld->type, (src_bit_size[0] - 1)));
   result = lp_build_shl(int_bld, src[0], src[1]);
      }
   case nir_op_ishr: {
      struct lp_build_context *uint_bld = get_int_bld(bld_base, true, src_bit_size[0]);
   struct lp_build_context *int_bld = get_int_bld(bld_base, false, src_bit_size[0]);
   if (src_bit_size[0] == 64)
         if (src_bit_size[0] < 32)
         src[1] = lp_build_and(uint_bld, src[1], lp_build_const_int_vec(gallivm, uint_bld->type, (src_bit_size[0] - 1)));
   result = lp_build_shr(int_bld, src[0], src[1]);
      }
   case nir_op_isign:
      result = lp_build_sgn(get_int_bld(bld_base, false, src_bit_size[0]), src[0]);
      case nir_op_isub:
      result = lp_build_sub(get_int_bld(bld_base, false, src_bit_size[0]),
            case nir_op_ixor:
      result = lp_build_xor(get_int_bld(bld_base, false, src_bit_size[0]),
            case nir_op_mov:
      result = src[0];
      case nir_op_unpack_64_2x32_split_x:
      result = split_64bit(bld_base, src[0], false);
      case nir_op_unpack_64_2x32_split_y:
      result = split_64bit(bld_base, src[0], true);
         case nir_op_pack_32_2x16_split: {
      LLVMValueRef tmp = merge_16bit(bld_base, src[0], src[1]);
   result = LLVMBuildBitCast(builder, tmp, bld_base->base.vec_type, "");
      }
   case nir_op_unpack_32_2x16_split_x:
      result = split_16bit(bld_base, src[0], false);
      case nir_op_unpack_32_2x16_split_y:
      result = split_16bit(bld_base, src[0], true);
      case nir_op_pack_64_2x32_split: {
      LLVMValueRef tmp = merge_64bit(bld_base, src[0], src[1]);
   result = LLVMBuildBitCast(builder, tmp, bld_base->uint64_bld.vec_type, "");
      }
   case nir_op_pack_32_4x8_split: {
      LLVMValueRef tmp1 = merge_16bit(bld_base, src[0], src[1]);
   LLVMValueRef tmp2 = merge_16bit(bld_base, src[2], src[3]);
   tmp1 = LLVMBuildBitCast(builder, tmp1, bld_base->uint16_bld.vec_type, "");
   tmp2 = LLVMBuildBitCast(builder, tmp2, bld_base->uint16_bld.vec_type, "");
   LLVMValueRef tmp = merge_16bit(bld_base, tmp1, tmp2);
   result = LLVMBuildBitCast(builder, tmp, bld_base->uint_bld.vec_type, "");
      }
   case nir_op_u2f16:
      result = LLVMBuildUIToFP(builder, src[0],
            case nir_op_u2f32:
      result = LLVMBuildUIToFP(builder, src[0], bld_base->base.vec_type, "");
      case nir_op_u2f64:
      result = LLVMBuildUIToFP(builder, src[0], bld_base->dbl_bld.vec_type, "");
      case nir_op_u2u8:
      result = LLVMBuildTrunc(builder, src[0], bld_base->uint8_bld.vec_type, "");
      case nir_op_u2u16:
      if (src_bit_size[0] < 16)
         else
            case nir_op_u2u32:
      if (src_bit_size[0] < 32)
         else
            case nir_op_u2u64:
      result = LLVMBuildZExt(builder, src[0], bld_base->uint64_bld.vec_type, "");
      case nir_op_udiv:
      result = do_int_divide(bld_base, true, src_bit_size[0], src[0], src[1]);
      case nir_op_ufind_msb: {
      struct lp_build_context *uint_bld = get_int_bld(bld_base, true, src_bit_size[0]);
   result = lp_build_ctlz(uint_bld, src[0]);
   result = lp_build_sub(uint_bld, lp_build_const_int_vec(gallivm, uint_bld->type, src_bit_size[0] - 1), result);
   if (src_bit_size[0] < 32)
         else
            }
   case nir_op_uge32:
      result = icmp32(bld_base, PIPE_FUNC_GEQUAL, true, src_bit_size[0], src);
      case nir_op_ult32:
      result = icmp32(bld_base, PIPE_FUNC_LESS, true, src_bit_size[0], src);
      case nir_op_umax:
      result = lp_build_max(get_int_bld(bld_base, true, src_bit_size[0]), src[0], src[1]);
      case nir_op_umin:
      result = lp_build_min(get_int_bld(bld_base, true, src_bit_size[0]), src[0], src[1]);
      case nir_op_umod:
      result = do_int_mod(bld_base, true, src_bit_size[0], src[0], src[1]);
      case nir_op_umul_high: {
      LLVMValueRef hi_bits;
   lp_build_mul_32_lohi(get_int_bld(bld_base, true, src_bit_size[0]), src[0], src[1], &hi_bits);
   result = hi_bits;
      }
   case nir_op_ushr: {
      struct lp_build_context *uint_bld = get_int_bld(bld_base, true, src_bit_size[0]);
   if (src_bit_size[0] == 64)
         if (src_bit_size[0] < 32)
         src[1] = lp_build_and(uint_bld, src[1], lp_build_const_int_vec(gallivm, uint_bld->type, (src_bit_size[0] - 1)));
   result = lp_build_shr(uint_bld, src[0], src[1]);
      }
   case nir_op_bcsel: {
      LLVMTypeRef src1_type = LLVMTypeOf(src[1]);
            if (LLVMGetTypeKind(src1_type) == LLVMPointerTypeKind &&
      LLVMGetTypeKind(src2_type) != LLVMPointerTypeKind) {
      } else if (LLVMGetTypeKind(src2_type) == LLVMPointerTypeKind &&
                        for (int i = 1; i <= 2; i++) {
      LLVMTypeRef type = LLVMTypeOf(src[i]);
   if (LLVMGetTypeKind(type) == LLVMPointerTypeKind)
            }
      }
   default:
      assert(0);
      }
      }
         static void
   visit_alu(struct lp_build_nir_context *bld_base,
         {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMValueRef src[NIR_MAX_VEC_COMPONENTS];
   unsigned src_bit_size[NIR_MAX_VEC_COMPONENTS];
   const unsigned num_components = instr->def.num_components;
            switch (instr->op) {
   case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
   case nir_op_vec8:
   case nir_op_vec16:
      src_components = 1;
      case nir_op_pack_half_2x16:
      src_components = 2;
      case nir_op_unpack_half_2x16:
      src_components = 1;
      case nir_op_cube_amd:
      src_components = 3;
      case nir_op_fsum2:
   case nir_op_fsum3:
   case nir_op_fsum4:
      src_components = nir_op_infos[instr->op].input_sizes[0];
      default:
      src_components = num_components;
               for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      src[i] = get_alu_src(bld_base, instr->src[i], src_components);
               LLVMValueRef result[NIR_MAX_VEC_COMPONENTS];
   if (instr->op == nir_op_vec4 ||
      instr->op == nir_op_vec3 ||
   instr->op == nir_op_vec2 ||
   instr->op == nir_op_vec8 ||
   instr->op == nir_op_vec16) {
   for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      result[i] = cast_type(bld_base, src[i],
               } else if (instr->op == nir_op_fsum4 ||
            instr->op == nir_op_fsum3 ||
   for (unsigned c = 0; c < nir_op_infos[instr->op].input_sizes[0]; c++) {
      LLVMValueRef temp_chan = LLVMBuildExtractValue(gallivm->builder,
         temp_chan = cast_type(bld_base, temp_chan,
               result[0] = (c == 0) ? temp_chan
      : lp_build_add(get_flt_bld(bld_base, src_bit_size[0]),
      } else if (is_aos(bld_base)) {
         } else {
      /* Loop for R,G,B,A channels */
   for (unsigned c = 0; c < num_components; c++) {
               /* Loop over instruction operands */
   for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      if (num_components > 1) {
      src_chan[i] = LLVMBuildExtractValue(gallivm->builder,
      } else {
         }
   src_chan[i] = cast_type(bld_base, src_chan[i],
            }
   result[c] = do_alu_action(bld_base, instr, src_bit_size, src_chan);
   result[c] = cast_type(bld_base, result[c],
               }
      }
         static void
   visit_load_const(struct lp_build_nir_context *bld_base,
         {
      LLVMValueRef result[NIR_MAX_VEC_COMPONENTS];
   bld_base->load_const(bld_base, instr, result);
      }
         static void
   get_deref_offset(struct lp_build_nir_context *bld_base, nir_deref_instr *instr,
                     {
      LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   nir_variable *var = nir_deref_instr_get_variable(instr);
   nir_deref_path path;
                     if (vertex_index_out != NULL || vertex_index_ref != NULL) {
      if (vertex_index_ref) {
      *vertex_index_ref = get_src(bld_base, path.path[idx_lvl]->arr.index);
   if (vertex_index_out)
      } else {
         }
               uint32_t const_offset = 0;
            if (var->data.compact && nir_src_is_const(instr->arr.index)) {
      assert(instr->deref_type == nir_deref_type_array);
   const_offset = nir_src_as_uint(instr->arr.index);
               for (; path.path[idx_lvl]; ++idx_lvl) {
      const struct glsl_type *parent_type = path.path[idx_lvl - 1]->type;
   if (path.path[idx_lvl]->deref_type == nir_deref_type_struct) {
               for (unsigned i = 0; i < index; i++) {
      const struct glsl_type *ft = glsl_get_struct_field(parent_type, i);
         } else if (path.path[idx_lvl]->deref_type == nir_deref_type_array) {
      unsigned size = glsl_count_attribute_slots(path.path[idx_lvl]->type, vs_in);
   if (nir_src_is_const(path.path[idx_lvl]->arr.index)) {
         } else {
      LLVMValueRef idx_src = get_src(bld_base, path.path[idx_lvl]->arr.index);
   idx_src = cast_type(bld_base, idx_src, nir_type_uint, 32);
   LLVMValueRef array_off = lp_build_mul(&bld_base->uint_bld, lp_build_const_int_vec(bld_base->base.gallivm, bld_base->base.type, size),
         if (offset)
         else
         } else
            out:
               if (const_offset && offset)
      offset = LLVMBuildAdd(builder, offset,
            *const_out = const_offset;
      }
         static void
   visit_load_input(struct lp_build_nir_context *bld_base,
               {
      nir_variable var = {0};
   var.data.location = nir_intrinsic_io_semantics(instr).location;
   var.data.driver_location = nir_intrinsic_base(instr);
            unsigned nc = instr->def.num_components;
            nir_src offset = *nir_get_io_offset_src(instr);
   bool indirect = !nir_src_is_const(offset);
   if (!indirect)
                     }
         static void
   visit_store_output(struct lp_build_nir_context *bld_base,
         {
      nir_variable var = {0};
   var.data.location = nir_intrinsic_io_semantics(instr).location;
   var.data.driver_location = nir_intrinsic_base(instr);
                     unsigned bit_size = nir_src_bit_size(instr->src[0]);
            nir_src offset = *nir_get_io_offset_src(instr);
   bool indirect = !nir_src_is_const(offset);
   if (!indirect)
                  if (mask == 0x1 && LLVMGetTypeKind(LLVMTypeOf(src)) == LLVMArrayTypeKind) {
      src = LLVMBuildExtractValue(bld_base->base.gallivm->builder,
               bld_base->store_var(bld_base, nir_var_shader_out, util_last_bit(mask),
      }
         static void
   visit_load_reg(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
            nir_intrinsic_instr *decl = nir_reg_get_decl(instr->src[0].ssa);
            struct hash_entry *entry = _mesa_hash_table_search(bld_base->regs, decl);
            unsigned bit_size = nir_intrinsic_bit_size(decl);
            LLVMValueRef indir_src = NULL;
   if (instr->intrinsic == nir_intrinsic_load_reg_indirect) {
      indir_src = cast_type(bld_base, get_src(bld_base, instr->src[1]),
                        if (!is_aos(bld_base) && instr->def.num_components > 1) {
      for (unsigned i = 0; i < instr->def.num_components; i++)
      } else {
            }
         static void
   visit_store_reg(struct lp_build_nir_context *bld_base,
         {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
            nir_intrinsic_instr *decl = nir_reg_get_decl(instr->src[1].ssa);
   unsigned base = nir_intrinsic_base(instr);
   unsigned write_mask = nir_intrinsic_write_mask(instr);
            LLVMValueRef val = get_src(bld_base, instr->src[0]);
   LLVMValueRef vals[NIR_MAX_VEC_COMPONENTS] = { NULL };
   if (!is_aos(bld_base) && nir_src_num_components(instr->src[0]) > 1) {
      for (unsigned i = 0; i < nir_src_num_components(instr->src[0]); i++)
      } else {
                  struct hash_entry *entry = _mesa_hash_table_search(bld_base->regs, decl);
            unsigned bit_size = nir_intrinsic_bit_size(decl);
            LLVMValueRef indir_src = NULL;
   if (instr->intrinsic == nir_intrinsic_store_reg_indirect) {
      indir_src = cast_type(bld_base, get_src(bld_base, instr->src[2]),
               bld_base->store_reg(bld_base, reg_bld, decl, write_mask, base,
      }
         static bool
   compact_array_index_oob(struct lp_build_nir_context *bld_base, nir_variable *var, const uint32_t index)
   {
      const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, bld_base->shader->info.stage)) {
      assert(glsl_type_is_array(type));
      }
      }
      static void
   visit_load_var(struct lp_build_nir_context *bld_base,
               {
      nir_deref_instr *deref = nir_instr_as_deref(instr->src[0].ssa->parent_instr);
   nir_variable *var = nir_deref_instr_get_variable(deref);
   assert(util_bitcount(deref->modes) == 1);
   nir_variable_mode mode = deref->modes;
   unsigned const_index = 0;
   LLVMValueRef indir_index;
   LLVMValueRef indir_vertex_index = NULL;
   unsigned vertex_index = 0;
   unsigned nc = instr->def.num_components;
   unsigned bit_size = instr->def.bit_size;
   if (var) {
      bool vs_in = bld_base->shader->info.stage == MESA_SHADER_VERTEX &&
         bool gs_in = bld_base->shader->info.stage == MESA_SHADER_GEOMETRY &&
         bool tcs_in = bld_base->shader->info.stage == MESA_SHADER_TESS_CTRL &&
         bool tcs_out = bld_base->shader->info.stage == MESA_SHADER_TESS_CTRL &&
         bool tes_in = bld_base->shader->info.stage == MESA_SHADER_TESS_EVAL &&
                     get_deref_offset(bld_base, deref, vs_in,
                        /* Return undef for loads definitely outside of the array bounds
   * (tcs-tes-levels-out-of-bounds-read.shader_test).
   */
   if (var->data.compact && compact_array_index_oob(bld_base, var, const_index)) {
      struct lp_build_context *undef_bld = get_int_bld(bld_base, true,
         for (int i = 0; i < instr->def.num_components; i++)
               }
   bld_base->load_var(bld_base, mode, nc, bit_size, var, vertex_index,
      }
         static void
   visit_store_var(struct lp_build_nir_context *bld_base,
         {
      nir_deref_instr *deref = nir_instr_as_deref(instr->src[0].ssa->parent_instr);
   nir_variable *var = nir_deref_instr_get_variable(deref);
   assert(util_bitcount(deref->modes) == 1);
   nir_variable_mode mode = deref->modes;
   int writemask = instr->const_index[0];
   unsigned bit_size = nir_src_bit_size(instr->src[1]);
   LLVMValueRef src = get_src(bld_base, instr->src[1]);
   unsigned const_index = 0;
   LLVMValueRef indir_index, indir_vertex_index = NULL;
   if (var) {
      bool tcs_out = bld_base->shader->info.stage == MESA_SHADER_TESS_CTRL &&
         bool mesh_out = bld_base->shader->info.stage == MESA_SHADER_MESH &&
         get_deref_offset(bld_base, deref, false, NULL,
                  /* Skip stores definitely outside of the array bounds
   * (tcs-tes-levels-out-of-bounds-write.shader_test).
   */
   if (var->data.compact && compact_array_index_oob(bld_base, var, const_index))
      }
   bld_base->store_var(bld_base, mode, instr->num_components, bit_size,
            }
         static void
   visit_load_ubo(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef idx = get_src(bld_base, instr->src[0]);
                     if (nir_src_num_components(instr->src[0]) == 1)
            bld_base->load_ubo(bld_base, instr->def.num_components,
            }
         static void
   visit_load_push_constant(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMValueRef offset = get_src(bld_base, instr->src[0]);
   LLVMValueRef idx = lp_build_const_int32(gallivm, 0);
            bld_base->load_ubo(bld_base, instr->def.num_components,
            }
         static void
   visit_load_ssbo(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef idx = get_src(bld_base, instr->src[0]);
   if (nir_src_num_components(instr->src[0]) == 1)
            LLVMValueRef offset = get_src(bld_base, instr->src[1]);
   bool index_and_offset_are_uniform =
      nir_src_is_always_uniform(instr->src[0]) &&
      bld_base->load_mem(bld_base, instr->def.num_components,
            }
         static void
   visit_store_ssbo(struct lp_build_nir_context *bld_base,
         {
               LLVMValueRef idx = get_src(bld_base, instr->src[1]);
   if (nir_src_num_components(instr->src[1]) == 1)
            LLVMValueRef offset = get_src(bld_base, instr->src[2]);
   bool index_and_offset_are_uniform =
      nir_src_is_always_uniform(instr->src[1]) &&
      int writemask = instr->const_index[0];
   int nc = nir_src_num_components(instr->src[0]);
   int bitsize = nir_src_bit_size(instr->src[0]);
   bld_base->store_mem(bld_base, writemask, nc, bitsize,
      }
         static void
   visit_get_ssbo_size(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef idx = get_src(bld_base, instr->src[0]);
   if (nir_src_num_components(instr->src[0]) == 1)
               }
         static void
   visit_ssbo_atomic(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef idx = get_src(bld_base, instr->src[0]);
   if (nir_src_num_components(instr->src[0]) == 1)
            LLVMValueRef offset = get_src(bld_base, instr->src[1]);
   LLVMValueRef val = get_src(bld_base, instr->src[2]);
   LLVMValueRef val2 = NULL;
   int bitsize = nir_src_bit_size(instr->src[2]);
   if (instr->intrinsic == nir_intrinsic_ssbo_atomic_swap)
            bld_base->atomic_mem(bld_base, nir_intrinsic_atomic_op(instr), bitsize, false, idx,
      }
      static void
   img_params_init_resource(struct lp_build_nir_context *bld_base, struct lp_img_params *params, nir_src src)
   {
      if (nir_src_num_components(src) == 1) {
      if (nir_src_is_const(src))
         else
                           }
      static void
   sampler_size_params_init_resource(struct lp_build_nir_context *bld_base, struct lp_sampler_size_query_params *params, nir_src src)
   {
      if (nir_src_num_components(src) == 1) {
      if (nir_src_is_const(src))
         else
                           }
      static void
   visit_load_image(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef coord_val = get_src(bld_base, instr->src[1]);
   LLVMValueRef coords[5];
            params.target = glsl_sampler_to_pipe(nir_intrinsic_image_dim(instr),
         for (unsigned i = 0; i < 4; i++)
         if (params.target == PIPE_TEXTURE_1D_ARRAY)
            params.coords = coords;
   params.outdata = result;
   params.img_op = LP_IMG_LOAD;
   if (nir_intrinsic_image_dim(instr) == GLSL_SAMPLER_DIM_MS ||
      nir_intrinsic_image_dim(instr) == GLSL_SAMPLER_DIM_SUBPASS_MS)
   params.ms_index = cast_type(bld_base, get_src(bld_base, instr->src[2]),
         img_params_init_resource(bld_base, &params, instr->src[0]);
               }
         static void
   visit_store_image(struct lp_build_nir_context *bld_base,
         {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef coord_val = get_src(bld_base, instr->src[1]);
   LLVMValueRef in_val = get_src(bld_base, instr->src[3]);
   LLVMValueRef coords[5];
            params.target = glsl_sampler_to_pipe(nir_intrinsic_image_dim(instr), nir_intrinsic_image_array(instr));
   for (unsigned i = 0; i < 4; i++)
         if (params.target == PIPE_TEXTURE_1D_ARRAY)
                           const struct util_format_description *desc = util_format_description(params.format);
            for (unsigned i = 0; i < 4; i++) {
               if (integer)
         else
      }
   if (nir_intrinsic_image_dim(instr) == GLSL_SAMPLER_DIM_MS)
                           if (params.target == PIPE_TEXTURE_1D_ARRAY)
            }
      LLVMAtomicRMWBinOp
   lp_translate_atomic_op(nir_atomic_op op)
   {
      switch (op) {
   case nir_atomic_op_iadd: return LLVMAtomicRMWBinOpAdd;
   case nir_atomic_op_xchg: return LLVMAtomicRMWBinOpXchg;
   case nir_atomic_op_iand: return LLVMAtomicRMWBinOpAnd;
   case nir_atomic_op_ior:  return LLVMAtomicRMWBinOpOr;
   case nir_atomic_op_ixor: return LLVMAtomicRMWBinOpXor;
   case nir_atomic_op_umin: return LLVMAtomicRMWBinOpUMin;
   case nir_atomic_op_umax: return LLVMAtomicRMWBinOpUMax;
   case nir_atomic_op_imin: return LLVMAtomicRMWBinOpMin;
   case nir_atomic_op_imax: return LLVMAtomicRMWBinOpMax;
      #if LLVM_VERSION_MAJOR >= 15
      case nir_atomic_op_fmin: return LLVMAtomicRMWBinOpFMin;
      #endif
      default:          unreachable("Unexpected atomic");
      }
      void
   lp_img_op_from_intrinsic(struct lp_img_params *params, nir_intrinsic_instr *instr)
   {
      if (instr->intrinsic == nir_intrinsic_image_load ||
      instr->intrinsic == nir_intrinsic_bindless_image_load) {
   params->img_op = LP_IMG_LOAD;
               if (instr->intrinsic == nir_intrinsic_image_store ||
      instr->intrinsic == nir_intrinsic_bindless_image_store) {
   params->img_op = LP_IMG_STORE;
               if (instr->intrinsic == nir_intrinsic_image_atomic_swap ||
      instr->intrinsic == nir_intrinsic_bindless_image_atomic_swap) {
   params->img_op = LP_IMG_ATOMIC_CAS;
               if (instr->intrinsic == nir_intrinsic_image_atomic ||
      instr->intrinsic == nir_intrinsic_bindless_image_atomic) {
   params->img_op = LP_IMG_ATOMIC;
      } else {
            }
         static void
   visit_atomic_image(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_img_params params = { 0 };
   LLVMValueRef coord_val = get_src(bld_base, instr->src[1]);
   LLVMValueRef in_val = get_src(bld_base, instr->src[3]);
            params.target = glsl_sampler_to_pipe(nir_intrinsic_image_dim(instr),
         for (unsigned i = 0; i < 4; i++) {
         }
   if (params.target == PIPE_TEXTURE_1D_ARRAY) {
                                    const struct util_format_description *desc = util_format_description(params.format);
            if (nir_intrinsic_image_dim(instr) == GLSL_SAMPLER_DIM_MS)
            if (instr->intrinsic == nir_intrinsic_image_atomic_swap ||
      instr->intrinsic == nir_intrinsic_bindless_image_atomic_swap) {
   LLVMValueRef cas_val = get_src(bld_base, instr->src[4]);
   params.indata[0] = in_val;
            if (integer)
         else
      } else {
                  if (integer)
         else
                                          }
         static void
   visit_image_size(struct lp_build_nir_context *bld_base,
               {
                        params.target = glsl_sampler_to_pipe(nir_intrinsic_image_dim(instr),
                              }
         static void
   visit_image_samples(struct lp_build_nir_context *bld_base,
               {
                        params.target = glsl_sampler_to_pipe(nir_intrinsic_image_dim(instr),
         params.sizes_out = result;
                        }
         static void
   visit_shared_load(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef offset = get_src(bld_base, instr->src[0]);
   bool offset_is_uniform = nir_src_is_always_uniform(instr->src[0]);
   bld_base->load_mem(bld_base, instr->def.num_components,
            }
         static void
   visit_shared_store(struct lp_build_nir_context *bld_base,
         {
      LLVMValueRef val = get_src(bld_base, instr->src[0]);
   LLVMValueRef offset = get_src(bld_base, instr->src[1]);
   bool offset_is_uniform = nir_src_is_always_uniform(instr->src[1]);
   int writemask = instr->const_index[1];
   int nc = nir_src_num_components(instr->src[0]);
   int bitsize = nir_src_bit_size(instr->src[0]);
   bld_base->store_mem(bld_base, writemask, nc, bitsize,
      }
         static void
   visit_shared_atomic(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef offset = get_src(bld_base, instr->src[0]);
   LLVMValueRef val = get_src(bld_base, instr->src[1]);
   LLVMValueRef val2 = NULL;
   int bitsize = nir_src_bit_size(instr->src[1]);
   if (instr->intrinsic == nir_intrinsic_shared_atomic_swap)
            bld_base->atomic_mem(bld_base, nir_intrinsic_atomic_op(instr), bitsize, false, NULL,
      }
         static void
   visit_barrier(struct lp_build_nir_context *bld_base,
         {
      LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   mesa_scope exec_scope = nir_intrinsic_execution_scope(instr);
            if (nir_semantics) {
      LLVMAtomicOrdering ordering = LLVMAtomicOrderingSequentiallyConsistent;
      }
   if (exec_scope != SCOPE_NONE)
      }
         static void
   visit_discard(struct lp_build_nir_context *bld_base,
         {
      LLVMValueRef cond = NULL;
   if (instr->intrinsic == nir_intrinsic_discard_if) {
      cond = get_src(bld_base, instr->src[0]);
      }
      }
         static void
   visit_load_kernel_input(struct lp_build_nir_context *bld_base,
               {
               bool offset_is_uniform = nir_src_is_always_uniform(instr->src[0]);
   bld_base->load_kernel_arg(bld_base, instr->def.num_components,
                  }
         static void
   visit_load_global(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef addr = get_src(bld_base, instr->src[0]);
   bool offset_is_uniform = nir_src_is_always_uniform(instr->src[0]);
   bld_base->load_global(bld_base, instr->def.num_components,
                  }
         static void
   visit_store_global(struct lp_build_nir_context *bld_base,
         {
      LLVMValueRef val = get_src(bld_base, instr->src[0]);
   int nc = nir_src_num_components(instr->src[0]);
   int bitsize = nir_src_bit_size(instr->src[0]);
   LLVMValueRef addr = get_src(bld_base, instr->src[1]);
   int addr_bitsize = nir_src_bit_size(instr->src[1]);
   int writemask = instr->const_index[0];
   bld_base->store_global(bld_base, writemask, nc, bitsize,
      }
         static void
   visit_global_atomic(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef addr = get_src(bld_base, instr->src[0]);
   LLVMValueRef val = get_src(bld_base, instr->src[1]);
   LLVMValueRef val2 = NULL;
   int addr_bitsize = nir_src_bit_size(instr->src[0]);
   int val_bitsize = nir_src_bit_size(instr->src[1]);
   if (instr->intrinsic == nir_intrinsic_global_atomic_swap)
            bld_base->atomic_global(bld_base, nir_intrinsic_atomic_op(instr),
            }
      #if LLVM_VERSION_MAJOR >= 10
   static void visit_shuffle(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef src = get_src(bld_base, instr->src[0]);
   src = cast_type(bld_base, src, nir_type_int,
         LLVMValueRef index = get_src(bld_base, instr->src[1]);
   index = cast_type(bld_base, index, nir_type_uint,
               }
   #endif
         static void
   visit_interp(struct lp_build_nir_context *bld_base,
               {
      struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   nir_deref_instr *deref = nir_instr_as_deref(instr->src[0].ssa->parent_instr);
   unsigned num_components = instr->def.num_components;
   nir_variable *var = nir_deref_instr_get_variable(deref);
   unsigned const_index;
   LLVMValueRef indir_index;
   LLVMValueRef offsets[2] = { NULL, NULL };
   get_deref_offset(bld_base, deref, false, NULL, NULL,
         bool centroid = instr->intrinsic == nir_intrinsic_interp_deref_at_centroid;
   bool sample = false;
   if (instr->intrinsic == nir_intrinsic_interp_deref_at_offset) {
      for (unsigned i = 0; i < 2; i++) {
      offsets[i] = LLVMBuildExtractValue(builder, get_src(bld_base, instr->src[1]), i, "");
         } else if (instr->intrinsic == nir_intrinsic_interp_deref_at_sample) {
      offsets[0] = get_src(bld_base, instr->src[1]);
   offsets[0] = cast_type(bld_base, offsets[0], nir_type_int, 32);
      }
   bld_base->interp_at(bld_base, num_components, var, centroid, sample,
      }
         static void
   visit_load_scratch(struct lp_build_nir_context *bld_base,
               {
               bld_base->load_scratch(bld_base, instr->def.num_components,
      }
         static void
   visit_store_scratch(struct lp_build_nir_context *bld_base,
         {
      LLVMValueRef val = get_src(bld_base, instr->src[0]);
   LLVMValueRef offset = get_src(bld_base, instr->src[1]);
   int writemask = instr->const_index[2];
   int nc = nir_src_num_components(instr->src[0]);
   int bitsize = nir_src_bit_size(instr->src[0]);
      }
      static void
   visit_payload_load(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef offset = get_src(bld_base, instr->src[0]);
   bool offset_is_uniform = nir_src_is_always_uniform(instr->src[0]);
   bld_base->load_mem(bld_base, instr->def.num_components,
            }
      static void
   visit_payload_store(struct lp_build_nir_context *bld_base,
         {
      LLVMValueRef val = get_src(bld_base, instr->src[0]);
   LLVMValueRef offset = get_src(bld_base, instr->src[1]);
   bool offset_is_uniform = nir_src_is_always_uniform(instr->src[1]);
   int writemask = instr->const_index[1];
   int nc = nir_src_num_components(instr->src[0]);
   int bitsize = nir_src_bit_size(instr->src[0]);
   bld_base->store_mem(bld_base, writemask, nc, bitsize,
      }
      static void
   visit_payload_atomic(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef offset = get_src(bld_base, instr->src[0]);
   LLVMValueRef val = get_src(bld_base, instr->src[1]);
   LLVMValueRef val2 = NULL;
   int bitsize = nir_src_bit_size(instr->src[1]);
   if (instr->intrinsic == nir_intrinsic_task_payload_atomic_swap)
            bld_base->atomic_mem(bld_base, nir_intrinsic_atomic_op(instr), bitsize, true, NULL,
      }
      static void visit_load_param(struct lp_build_nir_context *bld_base,
               {
      LLVMValueRef param = LLVMGetParam(bld_base->func, nir_intrinsic_param_idx(instr) + LP_RESV_FUNC_ARGS);
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   if (instr->num_components == 1)
         else {
      for (unsigned i = 0; i < instr->num_components; i++)
         }
      static void
   visit_intrinsic(struct lp_build_nir_context *bld_base,
         {
      LLVMValueRef result[NIR_MAX_VEC_COMPONENTS] = {0};
   switch (instr->intrinsic) {
   case nir_intrinsic_decl_reg:
      /* already handled */
      case nir_intrinsic_load_reg:
   case nir_intrinsic_load_reg_indirect:
      visit_load_reg(bld_base, instr, result);
      case nir_intrinsic_store_reg:
   case nir_intrinsic_store_reg_indirect:
      visit_store_reg(bld_base, instr);
      case nir_intrinsic_load_input:
      visit_load_input(bld_base, instr, result);
      case nir_intrinsic_store_output:
      visit_store_output(bld_base, instr);
      case nir_intrinsic_load_deref:
      visit_load_var(bld_base, instr, result);
      case nir_intrinsic_store_deref:
      visit_store_var(bld_base, instr);
      case nir_intrinsic_load_ubo:
      visit_load_ubo(bld_base, instr, result);
      case nir_intrinsic_load_push_constant:
      visit_load_push_constant(bld_base, instr, result);
      case nir_intrinsic_load_ssbo:
      visit_load_ssbo(bld_base, instr, result);
      case nir_intrinsic_store_ssbo:
      visit_store_ssbo(bld_base, instr);
      case nir_intrinsic_get_ssbo_size:
      visit_get_ssbo_size(bld_base, instr, result);
      case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_primitive_id:
   case nir_intrinsic_load_instance_id:
   case nir_intrinsic_load_base_instance:
   case nir_intrinsic_load_base_vertex:
   case nir_intrinsic_load_first_vertex:
   case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_local_invocation_id:
   case nir_intrinsic_load_local_invocation_index:
   case nir_intrinsic_load_num_workgroups:
   case nir_intrinsic_load_invocation_id:
   case nir_intrinsic_load_front_face:
   case nir_intrinsic_load_draw_id:
   case nir_intrinsic_load_workgroup_size:
   case nir_intrinsic_load_work_dim:
   case nir_intrinsic_load_tess_coord:
   case nir_intrinsic_load_tess_level_outer:
   case nir_intrinsic_load_tess_level_inner:
   case nir_intrinsic_load_patch_vertices_in:
   case nir_intrinsic_load_sample_id:
   case nir_intrinsic_load_sample_pos:
   case nir_intrinsic_load_sample_mask_in:
   case nir_intrinsic_load_view_index:
   case nir_intrinsic_load_subgroup_invocation:
   case nir_intrinsic_load_subgroup_id:
   case nir_intrinsic_load_num_subgroups:
      bld_base->sysval_intrin(bld_base, instr, result);
      case nir_intrinsic_load_helper_invocation:
      bld_base->helper_invocation(bld_base, &result[0]);
      case nir_intrinsic_discard_if:
   case nir_intrinsic_discard:
      visit_discard(bld_base, instr);
      case nir_intrinsic_emit_vertex:
      bld_base->emit_vertex(bld_base, nir_intrinsic_stream_id(instr));
      case nir_intrinsic_end_primitive:
      bld_base->end_primitive(bld_base, nir_intrinsic_stream_id(instr));
      case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
      visit_ssbo_atomic(bld_base, instr, result);
      case nir_intrinsic_image_load:
   case nir_intrinsic_bindless_image_load:
      visit_load_image(bld_base, instr, result);
      case nir_intrinsic_image_store:
   case nir_intrinsic_bindless_image_store:
      visit_store_image(bld_base, instr);
      case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
      visit_atomic_image(bld_base, instr, result);
      case nir_intrinsic_image_size:
   case nir_intrinsic_bindless_image_size:
      visit_image_size(bld_base, instr, result);
      case nir_intrinsic_image_samples:
   case nir_intrinsic_bindless_image_samples:
      visit_image_samples(bld_base, instr, result);
      case nir_intrinsic_load_shared:
      visit_shared_load(bld_base, instr, result);
      case nir_intrinsic_store_shared:
      visit_shared_store(bld_base, instr);
      case nir_intrinsic_shared_atomic:
   case nir_intrinsic_shared_atomic_swap:
      visit_shared_atomic(bld_base, instr, result);
      case nir_intrinsic_barrier:
      visit_barrier(bld_base, instr);
      case nir_intrinsic_load_kernel_input:
         break;
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
      visit_load_global(bld_base, instr, result);
      case nir_intrinsic_store_global:
      visit_store_global(bld_base, instr);
      case nir_intrinsic_global_atomic:
   case nir_intrinsic_global_atomic_swap:
      visit_global_atomic(bld_base, instr, result);
      case nir_intrinsic_vote_all:
   case nir_intrinsic_vote_any:
   case nir_intrinsic_vote_ieq:
   case nir_intrinsic_vote_feq:
      bld_base->vote(bld_base, cast_type(bld_base, get_src(bld_base, instr->src[0]), nir_type_int, nir_src_bit_size(instr->src[0])), instr, result);
      case nir_intrinsic_elect:
      bld_base->elect(bld_base, result);
      case nir_intrinsic_reduce:
   case nir_intrinsic_inclusive_scan:
   case nir_intrinsic_exclusive_scan:
      bld_base->reduce(bld_base, cast_type(bld_base, get_src(bld_base, instr->src[0]), nir_type_int, nir_src_bit_size(instr->src[0])), instr, result);
      case nir_intrinsic_ballot:
      bld_base->ballot(bld_base, cast_type(bld_base, get_src(bld_base, instr->src[0]), nir_type_int, 32), instr, result);
   #if LLVM_VERSION_MAJOR >= 10
      case nir_intrinsic_shuffle:
      visit_shuffle(bld_base, instr, result);
   #endif
      case nir_intrinsic_read_invocation:
   case nir_intrinsic_read_first_invocation: {
      LLVMValueRef src0 = get_src(bld_base, instr->src[0]);
            LLVMValueRef src1 = NULL;
   if (instr->intrinsic == nir_intrinsic_read_invocation)
            bld_base->read_invocation(bld_base, src0, nir_src_bit_size(instr->src[0]), src1, result);
      }
   case nir_intrinsic_interp_deref_at_offset:
   case nir_intrinsic_interp_deref_at_centroid:
   case nir_intrinsic_interp_deref_at_sample:
      visit_interp(bld_base, instr, result);
      case nir_intrinsic_load_scratch:
      visit_load_scratch(bld_base, instr, result);
      case nir_intrinsic_store_scratch:
      visit_store_scratch(bld_base, instr);
      case nir_intrinsic_shader_clock:
      bld_base->clock(bld_base, result);
      case nir_intrinsic_launch_mesh_workgroups:
      bld_base->launch_mesh_workgroups(bld_base,
            case nir_intrinsic_load_task_payload:
      visit_payload_load(bld_base, instr, result);
      case nir_intrinsic_store_task_payload:
      visit_payload_store(bld_base, instr);
      case nir_intrinsic_task_payload_atomic:
   case nir_intrinsic_task_payload_atomic_swap:
      visit_payload_atomic(bld_base, instr, result);
      case nir_intrinsic_set_vertex_and_primitive_count:
      bld_base->set_vertex_and_primitive_count(bld_base,
                  case nir_intrinsic_load_param:
      visit_load_param(bld_base, instr, result);
      default:
      fprintf(stderr, "Unsupported intrinsic: ");
   nir_print_instr(&instr->instr, stderr);
   fprintf(stderr, "\n");
   assert(0);
      }
   if (result[0]) {
            }
         static void
   visit_txs(struct lp_build_nir_context *bld_base, nir_tex_instr *instr)
   {
      struct lp_sampler_size_query_params params = { 0 };
   LLVMValueRef sizes_out[NIR_MAX_VEC_COMPONENTS];
   LLVMValueRef explicit_lod = NULL;
   LLVMValueRef texture_unit_offset = NULL;
            for (unsigned i = 0; i < instr->num_srcs; i++) {
      switch (instr->src[i].src_type) {
   case nir_tex_src_lod:
      explicit_lod = cast_type(bld_base,
                  case nir_tex_src_texture_offset:
      texture_unit_offset = get_src(bld_base, instr->src[i].src);
      case nir_tex_src_texture_handle:
      resource = get_src(bld_base, instr->src[i].src);
      default:
                     params.target = glsl_sampler_to_pipe(instr->sampler_dim, instr->is_array);
   params.texture_unit = instr->texture_index;
   params.explicit_lod = explicit_lod;
   params.is_sviewinfo = true;
   params.sizes_out = sizes_out;
   params.samples_only = (instr->op == nir_texop_texture_samples);
            if (instr->op == nir_texop_query_levels)
                     bld_base->tex_size(bld_base, &params);
   assign_ssa_dest(bld_base, &instr->def,
      }
         static enum lp_sampler_lod_property
   lp_build_nir_lod_property(gl_shader_stage stage, nir_src lod_src)
   {
               if (nir_src_is_always_uniform(lod_src)) {
         } else if (stage == MESA_SHADER_FRAGMENT) {
      if (gallivm_perf & GALLIVM_PERF_NO_QUAD_LOD)
         else
      } else {
         }
      }
         uint32_t
   lp_build_nir_sample_key(gl_shader_stage stage, nir_tex_instr *instr)
   {
               if (instr->op == nir_texop_txf ||
      instr->op == nir_texop_txf_ms) {
      } else if (instr->op == nir_texop_tg4) {
      sample_key |= LP_SAMPLER_OP_GATHER << LP_SAMPLER_OP_TYPE_SHIFT;
      } else if (instr->op == nir_texop_lod) {
                  bool explicit_lod = false;
            for (unsigned i = 0; i < instr->num_srcs; i++) {
      switch (instr->src[i].src_type) {
   case nir_tex_src_comparator:
      sample_key |= LP_SAMPLER_SHADOW;
      case nir_tex_src_bias:
      sample_key |= LP_SAMPLER_LOD_BIAS << LP_SAMPLER_LOD_CONTROL_SHIFT;
   explicit_lod = true;
   lod_src = i;
      case nir_tex_src_lod:
      sample_key |= LP_SAMPLER_LOD_EXPLICIT << LP_SAMPLER_LOD_CONTROL_SHIFT;
   explicit_lod = true;
   lod_src = i;
      case nir_tex_src_offset:
      sample_key |= LP_SAMPLER_OFFSETS;
      case nir_tex_src_ms_index:
      sample_key |= LP_SAMPLER_FETCH_MS;
      default:
                     enum lp_sampler_lod_property lod_property = LP_SAMPLER_LOD_SCALAR;
   if (explicit_lod)
            if (instr->op == nir_texop_txd) {
               if (stage == MESA_SHADER_FRAGMENT) {
      if (gallivm_perf & GALLIVM_PERF_NO_QUAD_LOD)
         else
      } else
                           }
         static void
   visit_tex(struct lp_build_nir_context *bld_base, nir_tex_instr *instr)
   {
      if (instr->op == nir_texop_txs ||
      instr->op == nir_texop_query_levels ||
   instr->op == nir_texop_texture_samples) {
   visit_txs(bld_base, instr);
               struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef coords[5];
   LLVMValueRef offsets[3] = { NULL };
   LLVMValueRef explicit_lod = NULL, ms_index = NULL;
   struct lp_sampler_params params = { 0 };
   struct lp_derivatives derivs;
   nir_deref_instr *texture_deref_instr = NULL;
   nir_deref_instr *sampler_deref_instr = NULL;
   LLVMValueRef texture_unit_offset = NULL;
   LLVMValueRef texel[NIR_MAX_VEC_COMPONENTS];
   LLVMValueRef coord_undef = LLVMGetUndef(bld_base->base.vec_type);
            LLVMValueRef texture_resource = NULL;
            for (unsigned i = 0; i < instr->num_srcs; i++) {
      switch (instr->src[i].src_type) {
   case nir_tex_src_coord: {
      LLVMValueRef coord = get_src(bld_base, instr->src[i].src);
   if (coord_vals == 1) {
         } else {
      for (unsigned chan = 0; chan < instr->coord_components; ++chan)
      coords[chan] = LLVMBuildExtractValue(builder, coord,
   }
   for (unsigned chan = coord_vals; chan < 5; chan++) {
         }
      }
   case nir_tex_src_texture_deref:
      texture_deref_instr = nir_src_as_deref(instr->src[i].src);
      case nir_tex_src_sampler_deref:
      sampler_deref_instr = nir_src_as_deref(instr->src[i].src);
      case nir_tex_src_comparator:
      coords[4] = get_src(bld_base, instr->src[i].src);
   coords[4] = cast_type(bld_base, coords[4], nir_type_float, 32);
      case nir_tex_src_bias:
      explicit_lod = cast_type(bld_base, get_src(bld_base, instr->src[i].src), nir_type_float, 32);
      case nir_tex_src_lod:
      if (instr->op == nir_texop_txf)
         else
            case nir_tex_src_ddx: {
      int deriv_cnt = instr->coord_components;
   if (instr->is_array)
         LLVMValueRef deriv_val = get_src(bld_base, instr->src[i].src);
   if (deriv_cnt == 1)
         else
      for (unsigned chan = 0; chan < deriv_cnt; ++chan)
      derivs.ddx[chan] = LLVMBuildExtractValue(builder, deriv_val,
   for (unsigned chan = 0; chan < deriv_cnt; ++chan)
            }
   case nir_tex_src_ddy: {
      int deriv_cnt = instr->coord_components;
   if (instr->is_array)
         LLVMValueRef deriv_val = get_src(bld_base, instr->src[i].src);
   if (deriv_cnt == 1)
         else
      for (unsigned chan = 0; chan < deriv_cnt; ++chan)
      derivs.ddy[chan] = LLVMBuildExtractValue(builder, deriv_val,
   for (unsigned chan = 0; chan < deriv_cnt; ++chan)
            }
   case nir_tex_src_offset: {
      int offset_cnt = instr->coord_components;
   if (instr->is_array)
         LLVMValueRef offset_val = get_src(bld_base, instr->src[i].src);
   if (offset_cnt == 1)
         else {
      for (unsigned chan = 0; chan < offset_cnt; ++chan) {
      offsets[chan] = LLVMBuildExtractValue(builder, offset_val,
               }
      }
   case nir_tex_src_ms_index:
                  case nir_tex_src_texture_offset:
      texture_unit_offset = get_src(bld_base, instr->src[i].src);
      case nir_tex_src_sampler_offset:
         case nir_tex_src_texture_handle:
      texture_resource = get_src(bld_base, instr->src[i].src);
      case nir_tex_src_sampler_handle:
      sampler_resource = get_src(bld_base, instr->src[i].src);
      case nir_tex_src_plane:
      assert(nir_src_is_const(instr->src[i].src) && !nir_src_as_uint(instr->src[i].src));
      default:
      assert(0);
         }
   if (!sampler_deref_instr)
            if (!sampler_resource)
            switch (instr->op) {
   case nir_texop_tex:
   case nir_texop_tg4:
   case nir_texop_txb:
   case nir_texop_txl:
   case nir_texop_txd:
   case nir_texop_lod:
      for (unsigned chan = 0; chan < coord_vals; ++chan)
            case nir_texop_txf:
   case nir_texop_txf_ms:
      for (unsigned chan = 0; chan < instr->coord_components; ++chan)
            default:
                  if (instr->is_array && instr->sampler_dim == GLSL_SAMPLER_DIM_1D) {
      /* move layer coord for 1d arrays. */
   coords[2] = coords[1];
               uint32_t samp_base_index = 0, tex_base_index = 0;
   if (!sampler_deref_instr) {
      int samp_src_index = nir_tex_instr_src_index(instr, nir_tex_src_sampler_handle);
   if (samp_src_index == -1) {
            }
   if (!texture_deref_instr) {
      int tex_src_index = nir_tex_instr_src_index(instr, nir_tex_src_texture_handle);
   if (tex_src_index == -1) {
                     if (instr->op == nir_texop_txd)
            params.sample_key = lp_build_nir_sample_key(bld_base->shader->info.stage, instr);
   params.offsets = offsets;
   params.texture_index = tex_base_index;
   params.texture_index_offset = texture_unit_offset;
   params.sampler_index = samp_base_index;
   params.coords = coords;
   params.texel = texel;
   params.lod = explicit_lod;
   params.ms_index = ms_index;
   params.aniso_filter_table = bld_base->aniso_filter_table;
   params.texture_resource = texture_resource;
   params.sampler_resource = sampler_resource;
            if (instr->def.bit_size != 32) {
      assert(instr->def.bit_size == 16);
   LLVMTypeRef vec_type = NULL;
   bool is_float = false;
   switch (nir_alu_type_get_base_type(instr->dest_type)) {
   case nir_type_float:
      is_float = true;
      case nir_type_int:
      vec_type = bld_base->int16_bld.vec_type;
      case nir_type_uint:
      vec_type = bld_base->uint16_bld.vec_type;
      default:
         }
   for (int i = 0; i < instr->def.num_components; ++i) {
      if (is_float) {
         } else {
      texel[i] = LLVMBuildBitCast(builder, texel[i], bld_base->int_bld.vec_type, "");
                        }
         static void
   visit_ssa_undef(struct lp_build_nir_context *bld_base,
         {
      unsigned num_components = instr->def.num_components;
   LLVMValueRef undef[NIR_MAX_VEC_COMPONENTS];
   struct lp_build_context *undef_bld = get_int_bld(bld_base, true,
         for (unsigned i = 0; i < num_components; i++)
         memset(&undef[num_components], 0, NIR_MAX_VEC_COMPONENTS - num_components);
      }
         static void
   visit_jump(struct lp_build_nir_context *bld_base,
         {
      switch (instr->type) {
   case nir_jump_break:
      bld_base->break_stmt(bld_base);
      case nir_jump_continue:
      bld_base->continue_stmt(bld_base);
      default:
            }
         static void
   visit_deref(struct lp_build_nir_context *bld_base,
         {
      if (!nir_deref_mode_is_one_of(instr, nir_var_mem_shared |
                        LLVMValueRef result = NULL;
   switch(instr->deref_type) {
   case nir_deref_type_var: {
      struct hash_entry *entry =
         result = entry->data;
      }
   default:
                     }
      static void
   visit_call(struct lp_build_nir_context *bld_base,
         {
      LLVMValueRef *args;
   struct hash_entry *entry = _mesa_hash_table_search(bld_base->fns, instr->callee);
   struct lp_build_fn *fn = entry->data;
                     args[0] = 0;
   for (unsigned i = 0; i < instr->num_params; i++) {
               if (nir_src_bit_size(instr->params[i]) == 32 && LLVMTypeOf(arg) == bld_base->base.vec_type)
                     bld_base->call(bld_base, fn, instr->num_params + LP_RESV_FUNC_ARGS, args);
      }
      static void
   visit_block(struct lp_build_nir_context *bld_base, nir_block *block)
   {
      nir_foreach_instr(instr, block)
   {
      switch (instr->type) {
   case nir_instr_type_alu:
      visit_alu(bld_base, nir_instr_as_alu(instr));
      case nir_instr_type_load_const:
      visit_load_const(bld_base, nir_instr_as_load_const(instr));
      case nir_instr_type_intrinsic:
      visit_intrinsic(bld_base, nir_instr_as_intrinsic(instr));
      case nir_instr_type_tex:
      visit_tex(bld_base, nir_instr_as_tex(instr));
      case nir_instr_type_phi:
      assert(0);
      case nir_instr_type_undef:
      visit_ssa_undef(bld_base, nir_instr_as_undef(instr));
      case nir_instr_type_jump:
      visit_jump(bld_base, nir_instr_as_jump(instr));
      case nir_instr_type_deref:
      visit_deref(bld_base, nir_instr_as_deref(instr));
      case nir_instr_type_call:
      visit_call(bld_base, nir_instr_as_call(instr));
      default:
      fprintf(stderr, "Unknown NIR instr type: ");
   nir_print_instr(instr, stderr);
   fprintf(stderr, "\n");
            }
         static void
   visit_if(struct lp_build_nir_context *bld_base, nir_if *if_stmt)
   {
               bld_base->if_cond(bld_base, cond);
            if (!exec_list_is_empty(&if_stmt->else_list)) {
      bld_base->else_stmt(bld_base);
      }
      }
         static void
   visit_loop(struct lp_build_nir_context *bld_base, nir_loop *loop)
   {
      assert(!nir_loop_has_continue_construct(loop));
   bld_base->bgnloop(bld_base);
   visit_cf_list(bld_base, &loop->body);
      }
         static void
   visit_cf_list(struct lp_build_nir_context *bld_base,
         {
      foreach_list_typed(nir_cf_node, node, node, list)
   {
      switch (node->type) {
   case nir_cf_node_block:
      visit_block(bld_base, nir_cf_node_as_block(node));
      case nir_cf_node_if:
      visit_if(bld_base, nir_cf_node_as_if(node));
      case nir_cf_node_loop:
      visit_loop(bld_base, nir_cf_node_as_loop(node));
      default:
               }
         static void
   handle_shader_output_decl(struct lp_build_nir_context *bld_base,
               {
         }
         /* vector registers are stored as arrays in LLVM side,
      so we can use GEP on them, as to do exec mask stores
   we need to operate on a single components.
   arrays are:
   0.x, 1.x, 2.x, 3.x
   0.y, 1.y, 2.y, 3.y
      */
   static LLVMTypeRef
   get_register_type(struct lp_build_nir_context *bld_base,
         {
      if (is_aos(bld_base))
            unsigned num_array_elems = nir_intrinsic_num_array_elems(reg);
   unsigned bit_size = nir_intrinsic_bit_size(reg);
            struct lp_build_context *int_bld =
            LLVMTypeRef type = int_bld->vec_type;
   if (num_components > 1)
         if (num_array_elems)
               }
      void
   lp_build_nir_prepasses(struct nir_shader *nir)
   {
      NIR_PASS_V(nir, nir_convert_to_lcssa, true, true);
   NIR_PASS_V(nir, nir_convert_from_ssa, true);
   NIR_PASS_V(nir, nir_lower_locals_to_regs, 32);
   NIR_PASS_V(nir, nir_remove_dead_derefs);
      }
      bool lp_build_nir_llvm(struct lp_build_nir_context *bld_base,
               {
      nir_foreach_shader_out_variable(variable, nir)
            if (nir->info.io_lowered) {
               while (outputs_written) {
                     var.type = glsl_vec4_type();
   var.data.mode = nir_var_shader_out;
   var.data.location = location;
   var.data.driver_location = util_bitcount64(nir->info.outputs_written &
                        bld_base->regs = _mesa_hash_table_create(NULL, _mesa_hash_pointer,
         bld_base->vars = _mesa_hash_table_create(NULL, _mesa_hash_pointer,
                  nir_foreach_reg_decl(reg, impl) {
      LLVMTypeRef type = get_register_type(bld_base, reg);
   LLVMValueRef reg_alloc = lp_build_alloca(bld_base->base.gallivm,
            }
   nir_index_ssa_defs(impl);
   bld_base->ssa_defs = calloc(impl->ssa_alloc, sizeof(LLVMValueRef));
            free(bld_base->ssa_defs);
   ralloc_free(bld_base->vars);
   ralloc_free(bld_base->regs);
   ralloc_free(bld_base->range_ht);
      }
         /* do some basic opts to remove some things we don't want to see. */
   void
   lp_build_opt_nir(struct nir_shader *nir)
   {
               static const struct nir_lower_tex_options lower_tex_options = {
      .lower_tg4_offsets = true,
   .lower_txp = ~0u,
      };
   NIR_PASS_V(nir, nir_lower_tex, &lower_tex_options);
            if (nir->info.stage == MESA_SHADER_TASK) {
      nir_lower_task_shader_options ts_opts = { 0 };
               NIR_PASS_V(nir, nir_lower_flrp, 16|32|64, true);
   NIR_PASS_V(nir, nir_lower_fp16_casts, nir_lower_fp16_all);
   do {
      progress = false;
   NIR_PASS(progress, nir, nir_opt_constant_folding);
   NIR_PASS(progress, nir, nir_opt_algebraic);
            nir_lower_tex_options options = { .lower_invalid_implicit_lod = true, };
            const nir_lower_subgroups_options subgroups_options = {
      .subgroup_size = lp_native_vector_width / 32,
   .ballot_bit_size = 32,
   .ballot_components = 1,
   .lower_to_scalar = true,
   .lower_subgroup_masks = true,
   .lower_relative_shuffle = true,
      };
               do {
      progress = false;
   NIR_PASS(progress, nir, nir_opt_algebraic_late);
   if (progress) {
      NIR_PASS_V(nir, nir_copy_prop);
   NIR_PASS_V(nir, nir_opt_dce);
                  if (nir_lower_bool_to_int32(nir)) {
      NIR_PASS_V(nir, nir_copy_prop);
         }
