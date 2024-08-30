   /*
   * Copyright (C) 2015 Rob Clark <robclark@freedesktop.org>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include <stdarg.h>
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "ir3_compiler.h"
   #include "ir3_image.h"
   #include "ir3_nir.h"
   #include "ir3_shader.h"
      #include "instr-a3xx.h"
   #include "ir3.h"
   #include "ir3_context.h"
      void
   ir3_handle_nonuniform(struct ir3_instruction *instr,
         {
      if (nir_intrinsic_has_access(intrin) &&
      (nir_intrinsic_access(intrin) & ACCESS_NON_UNIFORM)) {
         }
      void
   ir3_handle_bindless_cat6(struct ir3_instruction *instr, nir_src rsrc)
   {
      nir_intrinsic_instr *intrin = ir3_bindless_resource(rsrc);
   if (!intrin)
            instr->flags |= IR3_INSTR_B;
      }
      static struct ir3_instruction *
   create_input(struct ir3_context *ctx, unsigned compmask)
   {
               in = ir3_instr_create(ctx->in_block, OPC_META_INPUT, 1, 0);
   in->input.sysval = ~0;
                        }
      static struct ir3_instruction *
   create_frag_input(struct ir3_context *ctx, struct ir3_instruction *coord,
         {
      struct ir3_block *block = ctx->block;
   struct ir3_instruction *instr;
   /* packed inloc is fixed up later: */
            if (coord) {
         } else if (ctx->compiler->flat_bypass) {
      if (ctx->compiler->gen >= 6) {
         } else {
      instr = ir3_LDLV(block, inloc, 0, create_immed(block, 1), 0);
   instr->cat6.type = TYPE_U32;
         } else {
      instr = ir3_BARY_F(block, inloc, 0, ctx->ij[IJ_PERSP_PIXEL], 0);
                  }
      static struct ir3_instruction *
   create_driver_param(struct ir3_context *ctx, enum ir3_driver_param dp)
   {
      /* first four vec4 sysval's reserved for UBOs: */
   /* NOTE: dp is in scalar, but there can be >4 dp components: */
   struct ir3_const_state *const_state = ir3_const_state(ctx->so);
   unsigned n = const_state->offsets.driver_param;
   unsigned r = regid(n + dp / 4, dp % 4);
      }
      static struct ir3_instruction *
   create_driver_param_indirect(struct ir3_context *ctx, enum ir3_driver_param dp,
         {
      /* first four vec4 sysval's reserved for UBOs: */
   /* NOTE: dp is in scalar, but there can be >4 dp components: */
   struct ir3_const_state *const_state = ir3_const_state(ctx->so);
   unsigned n = const_state->offsets.driver_param;
      }
      /*
   * Adreno's comparisons produce a 1 for true and 0 for false, in either 16 or
   * 32-bit registers.  We use NIR's 1-bit integers to represent bools, and
   * trust that we will only see and/or/xor on those 1-bit values, so we can
   * safely store NIR i1s in a 32-bit reg while always containing either a 1 or
   * 0.
   */
      /*
   * alu/sfu instructions:
   */
      static struct ir3_instruction *
   create_cov(struct ir3_context *ctx, struct ir3_instruction *src,
         {
               switch (op) {
   case nir_op_f2f32:
   case nir_op_f2f16_rtne:
   case nir_op_f2f16_rtz:
   case nir_op_f2f16:
   case nir_op_f2i32:
   case nir_op_f2i16:
   case nir_op_f2i8:
   case nir_op_f2u32:
   case nir_op_f2u16:
   case nir_op_f2u8:
      switch (src_bitsize) {
   case 32:
      src_type = TYPE_F32;
      case 16:
      src_type = TYPE_F16;
      default:
         }
         case nir_op_i2f32:
   case nir_op_i2f16:
   case nir_op_i2i32:
   case nir_op_i2i16:
   case nir_op_i2i8:
      switch (src_bitsize) {
   case 32:
      src_type = TYPE_S32;
      case 16:
      src_type = TYPE_S16;
      case 8:
      src_type = TYPE_S8;
      default:
         }
         case nir_op_u2f32:
   case nir_op_u2f16:
   case nir_op_u2u32:
   case nir_op_u2u16:
   case nir_op_u2u8:
      switch (src_bitsize) {
   case 32:
      src_type = TYPE_U32;
      case 16:
      src_type = TYPE_U16;
      case 8:
      src_type = TYPE_U8;
      default:
         }
         case nir_op_b2f16:
   case nir_op_b2f32:
   case nir_op_b2i8:
   case nir_op_b2i16:
   case nir_op_b2i32:
      src_type = ctx->compiler->bool_type;
         default:
                  switch (op) {
   case nir_op_f2f32:
   case nir_op_i2f32:
   case nir_op_u2f32:
   case nir_op_b2f32:
      dst_type = TYPE_F32;
         case nir_op_f2f16_rtne:
   case nir_op_f2f16_rtz:
   case nir_op_f2f16:
   case nir_op_i2f16:
   case nir_op_u2f16:
   case nir_op_b2f16:
      dst_type = TYPE_F16;
         case nir_op_f2i32:
   case nir_op_i2i32:
   case nir_op_b2i32:
      dst_type = TYPE_S32;
         case nir_op_f2i16:
   case nir_op_i2i16:
   case nir_op_b2i16:
      dst_type = TYPE_S16;
         case nir_op_f2i8:
   case nir_op_i2i8:
   case nir_op_b2i8:
      dst_type = TYPE_S8;
         case nir_op_f2u32:
   case nir_op_u2u32:
      dst_type = TYPE_U32;
         case nir_op_f2u16:
   case nir_op_u2u16:
      dst_type = TYPE_U16;
         case nir_op_f2u8:
   case nir_op_u2u8:
      dst_type = TYPE_U8;
         default:
                  if (src_type == dst_type)
                     if (op == nir_op_f2f16_rtne) {
         } else if (op == nir_op_f2f16) {
      unsigned execution_mode = ctx->s->info.float_controls_execution_mode;
   nir_rounding_mode rounding_mode =
      nir_get_rounding_mode_from_float_controls(execution_mode,
      if (rounding_mode == nir_rounding_mode_rtne)
                  }
      /* For shift instructions NIR always has shift amount as 32 bit integer */
   static struct ir3_instruction *
   resize_shift_amount(struct ir3_context *ctx, struct ir3_instruction *src,
         {
      if (bs != 16)
               }
      static void
   emit_alu_dot_4x8_as_dp4acc(struct ir3_context *ctx, nir_alu_instr *alu,
               {
      struct ir3_instruction *accumulator = NULL;
   if (alu->op == nir_op_udot_4x8_uadd_sat) {
         } else {
                           if (alu->op == nir_op_udot_4x8_uadd ||
      alu->op == nir_op_udot_4x8_uadd_sat) {
      } else {
                  /* For some reason (sat) doesn't work in unsigned case so
   * we have to emulate it.
   */
   if (alu->op == nir_op_udot_4x8_uadd_sat) {
      dst[0] = ir3_ADD_U(ctx->block, dst[0], 0, src[2], 0);
      } else if (alu->op == nir_op_sudot_4x8_iadd_sat) {
            }
      static void
   emit_alu_dot_4x8_as_dp2acc(struct ir3_context *ctx, nir_alu_instr *alu,
               {
      int signedness;
   if (alu->op == nir_op_udot_4x8_uadd ||
      alu->op == nir_op_udot_4x8_uadd_sat) {
      } else {
                  struct ir3_instruction *accumulator = NULL;
   if (alu->op == nir_op_udot_4x8_uadd_sat ||
      alu->op == nir_op_sudot_4x8_iadd_sat) {
      } else {
                  dst[0] = ir3_DP2ACC(ctx->block, src[0], 0, src[1], 0, accumulator, 0);
   dst[0]->cat3.packed = IR3_SRC_PACKED_LOW;
            dst[0] = ir3_DP2ACC(ctx->block, src[0], 0, src[1], 0, dst[0], 0);
   dst[0]->cat3.packed = IR3_SRC_PACKED_HIGH;
            if (alu->op == nir_op_udot_4x8_uadd_sat) {
      dst[0] = ir3_ADD_U(ctx->block, dst[0], 0, src[2], 0);
      } else if (alu->op == nir_op_sudot_4x8_iadd_sat) {
      dst[0] = ir3_ADD_S(ctx->block, dst[0], 0, src[2], 0);
         }
      static void
   emit_alu(struct ir3_context *ctx, nir_alu_instr *alu)
   {
      const nir_op_info *info = &nir_op_infos[alu->op];
   struct ir3_instruction **dst, *src[info->num_inputs];
   unsigned bs[info->num_inputs]; /* bit size */
   struct ir3_block *b = ctx->block;
   unsigned dst_sz, wrmask;
            dst_sz = alu->def.num_components;
                     /* Vectors are special in that they have non-scalarized writemasks,
   * and just take the first swizzle channel for each argument in
   * order into each writemask channel.
   */
   if ((alu->op == nir_op_vec2) || (alu->op == nir_op_vec3) ||
      (alu->op == nir_op_vec4) || (alu->op == nir_op_vec8) ||
            for (int i = 0; i < info->num_inputs; i++) {
               src[i] = ir3_get_src(ctx, &asrc->src)[asrc->swizzle[0]];
   if (!src[i])
                     ir3_put_def(ctx, &alu->def);
               /* We also get mov's with more than one component for mov's so
   * handle those specially:
   */
   if (alu->op == nir_op_mov) {
      nir_alu_src *asrc = &alu->src[0];
            for (unsigned i = 0; i < dst_sz; i++) {
      if (wrmask & (1 << i)) {
         } else {
                     ir3_put_def(ctx, &alu->def);
               /* General case: We can just grab the one used channel per src. */
            for (int i = 0; i < info->num_inputs; i++) {
               src[i] = ir3_get_src(ctx, &asrc->src)[asrc->swizzle[0]];
                        switch (alu->op) {
   case nir_op_f2f32:
   case nir_op_f2f16_rtne:
   case nir_op_f2f16_rtz:
   case nir_op_f2f16:
   case nir_op_f2i32:
   case nir_op_f2i16:
   case nir_op_f2i8:
   case nir_op_f2u32:
   case nir_op_f2u16:
   case nir_op_f2u8:
   case nir_op_i2f32:
   case nir_op_i2f16:
   case nir_op_i2i32:
   case nir_op_i2i16:
   case nir_op_i2i8:
   case nir_op_u2f32:
   case nir_op_u2f16:
   case nir_op_u2u32:
   case nir_op_u2u16:
   case nir_op_u2u8:
   case nir_op_b2f16:
   case nir_op_b2f32:
   case nir_op_b2i8:
   case nir_op_b2i16:
   case nir_op_b2i32:
      dst[0] = create_cov(ctx, src[0], bs[0], alu->op);
         case nir_op_fquantize2f16:
      dst[0] = create_cov(ctx, create_cov(ctx, src[0], 32, nir_op_f2f16_rtne),
               case nir_op_b2b1:
      /* b2b1 will appear when translating from
   *
   * - nir_intrinsic_load_shared of a 32-bit 0/~0 value.
   * - nir_intrinsic_load_constant of a 32-bit 0/~0 value
   *
   * A negate can turn those into a 1 or 0 for us.
   */
   dst[0] = ir3_ABSNEG_S(b, src[0], IR3_REG_SNEG);
         case nir_op_b2b32:
      /* b2b32 will appear when converting our 1-bit bools to a store_shared
   * argument.
   *
   * A negate can turn those into a ~0 for us.
   */
   dst[0] = ir3_ABSNEG_S(b, src[0], IR3_REG_SNEG);
         case nir_op_fneg:
      dst[0] = ir3_ABSNEG_F(b, src[0], IR3_REG_FNEG);
      case nir_op_fabs:
      dst[0] = ir3_ABSNEG_F(b, src[0], IR3_REG_FABS);
      case nir_op_fmax:
      dst[0] = ir3_MAX_F(b, src[0], 0, src[1], 0);
      case nir_op_fmin:
      dst[0] = ir3_MIN_F(b, src[0], 0, src[1], 0);
      case nir_op_fsat:
      /* if there is just a single use of the src, and it supports
   * (sat) bit, we can just fold the (sat) flag back to the
   * src instruction and create a mov.  This is easier for cp
   * to eliminate.
   */
   if (is_sat_compatible(src[0]->opc) &&
      (list_length(&alu->src[0].src.ssa->uses) == 1)) {
   src[0]->flags |= IR3_INSTR_SAT;
      } else {
      /* otherwise generate a max.f that saturates.. blob does
   * similar (generating a cat2 mov using max.f)
   */
   dst[0] = ir3_MAX_F(b, src[0], 0, src[0], 0);
      }
      case nir_op_fmul:
      dst[0] = ir3_MUL_F(b, src[0], 0, src[1], 0);
      case nir_op_fadd:
      dst[0] = ir3_ADD_F(b, src[0], 0, src[1], 0);
      case nir_op_fsub:
      dst[0] = ir3_ADD_F(b, src[0], 0, src[1], IR3_REG_FNEG);
      case nir_op_ffma:
      dst[0] = ir3_MAD_F32(b, src[0], 0, src[1], 0, src[2], 0);
      case nir_op_fddx:
   case nir_op_fddx_coarse:
      dst[0] = ir3_DSX(b, src[0], 0);
   dst[0]->cat5.type = TYPE_F32;
      case nir_op_fddx_fine:
      dst[0] = ir3_DSXPP_MACRO(b, src[0], 0);
   dst[0]->cat5.type = TYPE_F32;
      case nir_op_fddy:
   case nir_op_fddy_coarse:
      dst[0] = ir3_DSY(b, src[0], 0);
   dst[0]->cat5.type = TYPE_F32;
   break;
      case nir_op_fddy_fine:
      dst[0] = ir3_DSYPP_MACRO(b, src[0], 0);
   dst[0]->cat5.type = TYPE_F32;
      case nir_op_flt:
      dst[0] = ir3_CMPS_F(b, src[0], 0, src[1], 0);
   dst[0]->cat2.condition = IR3_COND_LT;
      case nir_op_fge:
      dst[0] = ir3_CMPS_F(b, src[0], 0, src[1], 0);
   dst[0]->cat2.condition = IR3_COND_GE;
      case nir_op_feq:
      dst[0] = ir3_CMPS_F(b, src[0], 0, src[1], 0);
   dst[0]->cat2.condition = IR3_COND_EQ;
      case nir_op_fneu:
      dst[0] = ir3_CMPS_F(b, src[0], 0, src[1], 0);
   dst[0]->cat2.condition = IR3_COND_NE;
      case nir_op_fceil:
      dst[0] = ir3_CEIL_F(b, src[0], 0);
      case nir_op_ffloor:
      dst[0] = ir3_FLOOR_F(b, src[0], 0);
      case nir_op_ftrunc:
      dst[0] = ir3_TRUNC_F(b, src[0], 0);
      case nir_op_fround_even:
      dst[0] = ir3_RNDNE_F(b, src[0], 0);
      case nir_op_fsign:
      dst[0] = ir3_SIGN_F(b, src[0], 0);
         case nir_op_fsin:
      dst[0] = ir3_SIN(b, src[0], 0);
      case nir_op_fcos:
      dst[0] = ir3_COS(b, src[0], 0);
      case nir_op_frsq:
      dst[0] = ir3_RSQ(b, src[0], 0);
      case nir_op_frcp:
      dst[0] = ir3_RCP(b, src[0], 0);
      case nir_op_flog2:
      dst[0] = ir3_LOG2(b, src[0], 0);
      case nir_op_fexp2:
      dst[0] = ir3_EXP2(b, src[0], 0);
      case nir_op_fsqrt:
      dst[0] = ir3_SQRT(b, src[0], 0);
         case nir_op_iabs:
      dst[0] = ir3_ABSNEG_S(b, src[0], IR3_REG_SABS);
      case nir_op_iadd:
      dst[0] = ir3_ADD_U(b, src[0], 0, src[1], 0);
      case nir_op_ihadd:
      dst[0] = ir3_ADD_S(b, src[0], 0, src[1], 0);
   dst[0]->dsts[0]->flags |= IR3_REG_EI;
      case nir_op_uhadd:
      dst[0] = ir3_ADD_U(b, src[0], 0, src[1], 0);
   dst[0]->dsts[0]->flags |= IR3_REG_EI;
      case nir_op_iand:
      dst[0] = ir3_AND_B(b, src[0], 0, src[1], 0);
      case nir_op_imax:
      dst[0] = ir3_MAX_S(b, src[0], 0, src[1], 0);
      case nir_op_umax:
      dst[0] = ir3_MAX_U(b, src[0], 0, src[1], 0);
      case nir_op_imin:
      dst[0] = ir3_MIN_S(b, src[0], 0, src[1], 0);
      case nir_op_umin:
      dst[0] = ir3_MIN_U(b, src[0], 0, src[1], 0);
      case nir_op_umul_low:
      dst[0] = ir3_MULL_U(b, src[0], 0, src[1], 0);
      case nir_op_imadsh_mix16:
      dst[0] = ir3_MADSH_M16(b, src[0], 0, src[1], 0, src[2], 0);
      case nir_op_imad24_ir3:
      dst[0] = ir3_MAD_S24(b, src[0], 0, src[1], 0, src[2], 0);
      case nir_op_imul:
      compile_assert(ctx, alu->def.bit_size == 16);
   dst[0] = ir3_MUL_S24(b, src[0], 0, src[1], 0);
      case nir_op_imul24:
      dst[0] = ir3_MUL_S24(b, src[0], 0, src[1], 0);
      case nir_op_ineg:
      dst[0] = ir3_ABSNEG_S(b, src[0], IR3_REG_SNEG);
      case nir_op_inot:
      if (bs[0] == 1) {
      struct ir3_instruction *one =
            } else {
         }
      case nir_op_ior:
      dst[0] = ir3_OR_B(b, src[0], 0, src[1], 0);
      case nir_op_ishl:
      dst[0] =
            case nir_op_ishr:
      dst[0] =
            case nir_op_isub:
      dst[0] = ir3_SUB_U(b, src[0], 0, src[1], 0);
      case nir_op_ixor:
      dst[0] = ir3_XOR_B(b, src[0], 0, src[1], 0);
      case nir_op_ushr:
      dst[0] =
            case nir_op_ilt:
      dst[0] = ir3_CMPS_S(b, src[0], 0, src[1], 0);
   dst[0]->cat2.condition = IR3_COND_LT;
      case nir_op_ige:
      dst[0] = ir3_CMPS_S(b, src[0], 0, src[1], 0);
   dst[0]->cat2.condition = IR3_COND_GE;
      case nir_op_ieq:
      dst[0] = ir3_CMPS_S(b, src[0], 0, src[1], 0);
   dst[0]->cat2.condition = IR3_COND_EQ;
      case nir_op_ine:
      dst[0] = ir3_CMPS_S(b, src[0], 0, src[1], 0);
   dst[0]->cat2.condition = IR3_COND_NE;
      case nir_op_ult:
      dst[0] = ir3_CMPS_U(b, src[0], 0, src[1], 0);
   dst[0]->cat2.condition = IR3_COND_LT;
      case nir_op_uge:
      dst[0] = ir3_CMPS_U(b, src[0], 0, src[1], 0);
   dst[0]->cat2.condition = IR3_COND_GE;
         case nir_op_bcsel: {
               /* If src[0] is a negation (likely as a result of an ir3_b2n(cond)),
   * we can ignore that and use original cond, since the nonzero-ness of
   * cond stays the same.
   */
   if (cond->opc == OPC_ABSNEG_S && cond->flags == 0 &&
      (cond->srcs[0]->flags & (IR3_REG_SNEG | IR3_REG_SABS)) ==
                              /* The condition's size has to match the other two arguments' size, so
   * convert down if necessary.
   *
   * Single hashtable is fine, because the conversion will either be
   * 16->32 or 32->16, but never both
   */
   if (is_half(src[1]) != is_half(cond)) {
      struct hash_entry *prev_entry =
         if (prev_entry) {
         } else {
      if (is_half(cond)) {
         } else {
         }
                  if (is_half(src[1])) {
         } else {
                     }
   case nir_op_bit_count: {
      if (ctx->compiler->gen < 5 || (src[0]->dsts[0]->flags & IR3_REG_HALF)) {
      dst[0] = ir3_CBITS_B(b, src[0], 0);
               // We need to do this 16b at a time on a5xx+a6xx.  Once half-precision
   // support is in place, this should probably move to a NIR lowering pass:
            hi = ir3_COV(b, ir3_SHR_B(b, src[0], 0, create_immed(b, 16), 0), TYPE_U32,
                  hi = ir3_CBITS_B(b, hi, 0);
            // TODO maybe the builders should default to making dst half-precision
   // if the src's were half precision, to make this less awkward.. otoh
   // we should probably just do this lowering in NIR.
   hi->dsts[0]->flags |= IR3_REG_HALF;
            dst[0] = ir3_ADD_S(b, hi, 0, lo, 0);
   dst[0]->dsts[0]->flags |= IR3_REG_HALF;
   dst[0] = ir3_COV(b, dst[0], TYPE_U16, TYPE_U32);
      }
   case nir_op_ifind_msb: {
      struct ir3_instruction *cmp;
   dst[0] = ir3_CLZ_S(b, src[0], 0);
   cmp = ir3_CMPS_S(b, dst[0], 0, create_immed(b, 0), 0);
   cmp->cat2.condition = IR3_COND_GE;
   dst[0] = ir3_SEL_B32(b, ir3_SUB_U(b, create_immed(b, 31), 0, dst[0], 0),
            }
   case nir_op_ufind_msb:
      dst[0] = ir3_CLZ_B(b, src[0], 0);
   dst[0] = ir3_SEL_B32(b, ir3_SUB_U(b, create_immed(b, 31), 0, dst[0], 0),
            case nir_op_find_lsb:
      dst[0] = ir3_BFREV_B(b, src[0], 0);
   dst[0] = ir3_CLZ_B(b, dst[0], 0);
      case nir_op_bitfield_reverse:
      dst[0] = ir3_BFREV_B(b, src[0], 0);
         case nir_op_uadd_sat:
      dst[0] = ir3_ADD_U(b, src[0], 0, src[1], 0);
   dst[0]->flags |= IR3_INSTR_SAT;
      case nir_op_iadd_sat:
      dst[0] = ir3_ADD_S(b, src[0], 0, src[1], 0);
   dst[0]->flags |= IR3_INSTR_SAT;
      case nir_op_usub_sat:
      dst[0] = ir3_SUB_U(b, src[0], 0, src[1], 0);
   dst[0]->flags |= IR3_INSTR_SAT;
      case nir_op_isub_sat:
      dst[0] = ir3_SUB_S(b, src[0], 0, src[1], 0);
   dst[0]->flags |= IR3_INSTR_SAT;
         case nir_op_udot_4x8_uadd:
   case nir_op_udot_4x8_uadd_sat:
   case nir_op_sudot_4x8_iadd:
   case nir_op_sudot_4x8_iadd_sat: {
      if (ctx->compiler->has_dp4acc) {
         } else if (ctx->compiler->has_dp2acc) {
         } else {
      ir3_context_error(ctx, "ALU op should have been lowered: %s\n",
                           default:
      ir3_context_error(ctx, "Unhandled ALU op: %s\n",
                     if (nir_alu_type_get_base_type(info->output_type) == nir_type_bool) {
      assert(alu->def.bit_size == 1 || alu->op == nir_op_b2b32);
      } else {
      /* 1-bit values stored in 32-bit registers are only valid for certain
   * ALU ops.
   */
   switch (alu->op) {
   case nir_op_iand:
   case nir_op_ior:
   case nir_op_ixor:
   case nir_op_inot:
   case nir_op_bcsel:
         default:
                        }
      static void
   emit_intrinsic_load_ubo_ldc(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
               /* This is only generated for us by nir_lower_ubo_vec4, which leaves base =
   * 0.
   */
            unsigned ncomp = intr->num_components;
   struct ir3_instruction *offset = ir3_get_src(ctx, &intr->src[1])[0];
   struct ir3_instruction *idx = ir3_get_src(ctx, &intr->src[0])[0];
   struct ir3_instruction *ldc = ir3_LDC(b, idx, 0, offset, 0);
   ldc->dsts[0]->wrmask = MASK(ncomp);
   ldc->cat6.iim_val = ncomp;
   ldc->cat6.d = nir_intrinsic_component(intr);
            ir3_handle_bindless_cat6(ldc, intr->src[0]);
   if (ldc->flags & IR3_INSTR_B)
                     }
      static void
   emit_intrinsic_copy_ubo_to_uniform(struct ir3_context *ctx,
         {
               unsigned base = nir_intrinsic_base(intr);
                     struct ir3_instruction *offset = ir3_get_src(ctx, &intr->src[1])[0];
   struct ir3_instruction *idx = ir3_get_src(ctx, &intr->src[0])[0];
   struct ir3_instruction *ldc = ir3_LDC_K(b, idx, 0, offset, 0);
   ldc->cat6.iim_val = size;
            ir3_handle_bindless_cat6(ldc, intr->src[0]);
   if (ldc->flags & IR3_INSTR_B)
                        }
      /* handles direct/indirect UBO reads: */
   static void
   emit_intrinsic_load_ubo(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *base_lo, *base_hi, *addr, *src0, *src1;
   const struct ir3_const_state *const_state = ir3_const_state(ctx->so);
   unsigned ubo = regid(const_state->offsets.ubo, 0);
                     /* First src is ubo index, which could either be an immed or not: */
   src0 = ir3_get_src(ctx, &intr->src[0])[0];
   if (is_same_type_mov(src0) && (src0->srcs[0]->flags & IR3_REG_IMMED)) {
      base_lo = create_uniform(b, ubo + (src0->srcs[0]->iim_val * ptrsz));
      } else {
      base_lo = create_uniform_indirect(b, ubo, TYPE_U32,
         base_hi = create_uniform_indirect(b, ubo + 1, TYPE_U32,
            /* NOTE: since relative addressing is used, make sure constlen is
   * at least big enough to cover all the UBO addresses, since the
   * assembler won't know what the max address reg is.
   */
   ctx->so->constlen =
      MAX2(ctx->so->constlen,
            /* note: on 32bit gpu's base_hi is ignored and DCE'd */
            if (nir_src_is_const(intr->src[1])) {
         } else {
      /* For load_ubo_indirect, second src is indirect offset: */
            /* and add offset to addr: */
               /* if offset is to large to encode in the ldg, split it out: */
   if ((off + (intr->num_components * 4)) > 1024) {
      /* split out the minimal amount to improve the odds that
   * cp can fit the immediate in the add.s instruction:
   */
   unsigned off2 = off + (intr->num_components * 4) - 1024;
   addr = ir3_ADD_S(b, addr, 0, create_immed(b, off2), 0);
               if (ptrsz == 2) {
               /* handle 32b rollover, ie:
   *   if (addr < base_lo)
   *      base_hi++
   */
   carry = ir3_CMPS_U(b, addr, 0, base_lo, 0);
   carry->cat2.condition = IR3_COND_LT;
                        for (int i = 0; i < intr->num_components; i++) {
      struct ir3_instruction *load =
      ir3_LDG(b, addr, 0, create_immed(b, off + i * 4), 0,
      load->cat6.type = TYPE_U32;
         }
      /* Load a kernel param: src[] = { address }. */
   static void
   emit_intrinsic_load_kernel_input(struct ir3_context *ctx,
               {
      const struct ir3_const_state *const_state = ir3_const_state(ctx->so);
   struct ir3_block *b = ctx->block;
   unsigned offset = nir_intrinsic_base(intr);
                     if (is_same_type_mov(src0) && (src0->srcs[0]->flags & IR3_REG_IMMED)) {
               /* kernel param position is in bytes, but constant space is 32b registers: */
               } else {
      /* kernel param position is in bytes, but constant space is 32b registers: */
            /* TODO we should probably be lowering this in nir, and also handling
   * non-32b inputs.. Also we probably don't want to be using
   * SP_MODE_CONTROL.CONSTANT_DEMOTION_ENABLE for KERNEL shaders..
   */
            dst[0] = create_uniform_indirect(b, offset / 4, TYPE_U32,
         }
      /* src[] = { block_index } */
   static void
   emit_intrinsic_ssbo_size(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *ibo = ir3_ssbo_to_ibo(ctx, intr->src[0]);
   struct ir3_instruction *resinfo = ir3_RESINFO(b, ibo, 0);
   resinfo->cat6.iim_val = 1;
   resinfo->cat6.d = ctx->compiler->gen >= 6 ? 1 : 2;
   resinfo->cat6.type = TYPE_U32;
   resinfo->cat6.typed = false;
   /* resinfo has no writemask and always writes out 3 components */
   resinfo->dsts[0]->wrmask = MASK(3);
   ir3_handle_bindless_cat6(resinfo, intr->src[0]);
            if (ctx->compiler->gen >= 6) {
         } else {
      /* On a5xx, resinfo returns the low 16 bits of ssbo size in .x and the high 16 bits in .y */
   struct ir3_instruction *resinfo_dst[2];
   ir3_split_dest(b, resinfo_dst, resinfo, 0, 2);
         }
      /* src[] = { offset }. const_index[] = { base } */
   static void
   emit_intrinsic_load_shared(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *ldl, *offset;
            offset = ir3_get_src(ctx, &intr->src[0])[0];
            ldl = ir3_LDL(b, offset, 0, create_immed(b, base), 0,
            ldl->cat6.type = utype_def(&intr->def);
            ldl->barrier_class = IR3_BARRIER_SHARED_R;
               }
      /* src[] = { value, offset }. const_index[] = { base, write_mask } */
   static void
   emit_intrinsic_store_shared(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *stl, *offset;
   struct ir3_instruction *const *value;
            value = ir3_get_src(ctx, &intr->src[0]);
            base = nir_intrinsic_base(intr);
   wrmask = nir_intrinsic_write_mask(intr);
                     stl = ir3_STL(b, offset, 0, ir3_create_collect(b, value, ncomp), 0,
         stl->cat6.dst_offset = base;
   stl->cat6.type = utype_src(intr->src[0]);
   stl->barrier_class = IR3_BARRIER_SHARED_W;
               }
      /* src[] = { offset }. const_index[] = { base } */
   static void
   emit_intrinsic_load_shared_ir3(struct ir3_context *ctx,
               {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *load, *offset;
            offset = ir3_get_src(ctx, &intr->src[0])[0];
            load = ir3_LDLW(b, offset, 0, create_immed(b, base), 0,
            /* for a650, use LDL for tess ctrl inputs: */
   if (ctx->so->type == MESA_SHADER_TESS_CTRL && ctx->compiler->tess_use_shared)
            load->cat6.type = utype_def(&intr->def);
            load->barrier_class = IR3_BARRIER_SHARED_R;
               }
      /* src[] = { value, offset }. const_index[] = { base } */
   static void
   emit_intrinsic_store_shared_ir3(struct ir3_context *ctx,
         {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *store, *offset;
            value = ir3_get_src(ctx, &intr->src[0]);
            store = ir3_STLW(b, offset, 0,
                  /* for a650, use STL for vertex outputs used by tess ctrl shader: */
   if (ctx->so->type == MESA_SHADER_VERTEX && ctx->so->key.tessellation &&
      ctx->compiler->tess_use_shared)
         store->cat6.dst_offset = nir_intrinsic_base(intr);
   store->cat6.type = utype_src(intr->src[0]);
   store->barrier_class = IR3_BARRIER_SHARED_W;
               }
      /*
   * CS shared variable atomic intrinsics
   *
   * All of the shared variable atomic memory operations read a value from
   * memory, compute a new value using one of the operations below, write the
   * new value to memory, and return the original value read.
   *
   * All operations take 2 sources except CompSwap that takes 3. These
   * sources represent:
   *
   * 0: The offset into the shared variable storage region that the atomic
   *    operation will operate on.
   * 1: The data parameter to the atomic function (i.e. the value to add
   *    in, etc).
   * 2: For CompSwap only: the second data parameter.
   */
   static struct ir3_instruction *
   emit_intrinsic_atomic_shared(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *atomic, *src0, *src1;
            src0 = ir3_get_src(ctx, &intr->src[0])[0]; /* offset */
            switch (nir_intrinsic_atomic_op(intr)) {
   case nir_atomic_op_iadd:
      atomic = ir3_ATOMIC_ADD(b, src0, 0, src1, 0);
      case nir_atomic_op_imin:
      atomic = ir3_ATOMIC_MIN(b, src0, 0, src1, 0);
   type = TYPE_S32;
      case nir_atomic_op_umin:
      atomic = ir3_ATOMIC_MIN(b, src0, 0, src1, 0);
      case nir_atomic_op_imax:
      atomic = ir3_ATOMIC_MAX(b, src0, 0, src1, 0);
   type = TYPE_S32;
      case nir_atomic_op_umax:
      atomic = ir3_ATOMIC_MAX(b, src0, 0, src1, 0);
      case nir_atomic_op_iand:
      atomic = ir3_ATOMIC_AND(b, src0, 0, src1, 0);
      case nir_atomic_op_ior:
      atomic = ir3_ATOMIC_OR(b, src0, 0, src1, 0);
      case nir_atomic_op_ixor:
      atomic = ir3_ATOMIC_XOR(b, src0, 0, src1, 0);
      case nir_atomic_op_xchg:
      atomic = ir3_ATOMIC_XCHG(b, src0, 0, src1, 0);
      case nir_atomic_op_cmpxchg:
      /* for cmpxchg, src1 is [ui]vec2(data, compare): */
   src1 = ir3_collect(b, ir3_get_src(ctx, &intr->src[2])[0], src1);
   atomic = ir3_ATOMIC_CMPXCHG(b, src0, 0, src1, 0);
      default:
                  atomic->cat6.iim_val = 1;
   atomic->cat6.d = 1;
   atomic->cat6.type = type;
   atomic->barrier_class = IR3_BARRIER_SHARED_W;
            /* even if nothing consume the result, we can't DCE the instruction: */
               }
      static void
   stp_ldp_offset(struct ir3_context *ctx, nir_src *src,
         {
               if (nir_src_is_const(*src)) {
      unsigned src_offset = nir_src_as_uint(*src);
   /* The base offset field is only 13 bits, and it's signed. Try to make the
   * offset constant whenever the original offsets are similar, to avoid
   * creating too many constants in the final shader.
   */
   *base = ((int32_t) src_offset << (32 - 13)) >> (32 - 13);
   uint32_t offset_val = src_offset - *base;
      } else {
      /* TODO: match on nir_iadd with a constant that fits */
   *base = 0;
         }
      /* src[] = { offset }. */
   static void
   emit_intrinsic_load_scratch(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *ldp, *offset;
                     ldp = ir3_LDP(b, offset, 0, create_immed(b, base), 0,
            ldp->cat6.type = utype_def(&intr->def);
            ldp->barrier_class = IR3_BARRIER_PRIVATE_R;
               }
      /* src[] = { value, offset }. const_index[] = { write_mask } */
   static void
   emit_intrinsic_store_scratch(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction *stp, *offset;
   struct ir3_instruction *const *value;
   unsigned wrmask, ncomp;
                              wrmask = nir_intrinsic_write_mask(intr);
                     stp = ir3_STP(b, offset, 0, ir3_create_collect(b, value, ncomp), 0,
         stp->cat6.dst_offset = base;
   stp->cat6.type = utype_src(intr->src[0]);
   stp->barrier_class = IR3_BARRIER_PRIVATE_W;
               }
      struct tex_src_info {
      /* For prefetch */
   unsigned tex_base, samp_base, tex_idx, samp_idx;
   /* For normal tex instructions */
   unsigned base, a1_val, flags;
      };
      /* TODO handle actual indirect/dynamic case.. which is going to be weird
   * to handle with the image_mapping table..
   */
   static struct tex_src_info
   get_image_ssbo_samp_tex_src(struct ir3_context *ctx, nir_src *src, bool image)
   {
      struct ir3_block *b = ctx->block;
   struct tex_src_info info = {0};
            if (bindless_tex) {
      /* Bindless case */
   ctx->so->bindless_tex = true;
            /* Gather information required to determine which encoding to
   * choose as well as for prefetch.
   */
   info.tex_base = nir_intrinsic_desc_set(bindless_tex);
   bool tex_const = nir_src_is_const(bindless_tex->src[0]);
   if (tex_const)
                  /* Choose encoding. */
   if (tex_const && info.tex_idx < 256) {
      if (info.tex_idx < 16) {
      /* Everything fits within the instruction */
      } else {
      info.base = info.tex_base;
   if (ctx->compiler->gen <= 6) {
         } else {
         }
      }
      } else {
                                    texture = ir3_get_src(ctx, src)[0];
   sampler = create_immed(b, 0);
         } else {
      info.flags |= IR3_INSTR_S2EN;
   unsigned slot = nir_src_as_uint(*src);
   unsigned tex_idx = image ?
         ir3_image_to_tex(&ctx->so->image_mapping, slot) :
                     texture = create_immed_typed(ctx->block, tex_idx, TYPE_U16);
                           }
      static struct ir3_instruction *
   emit_sam(struct ir3_context *ctx, opc_t opc, struct tex_src_info info,
               {
      struct ir3_instruction *sam, *addr;
   if (info.flags & IR3_INSTR_A1EN) {
         }
   sam = ir3_SAM(ctx->block, opc, type, wrmask, info.flags, info.samp_tex, src0,
         if (info.flags & IR3_INSTR_A1EN) {
         }
   if (info.flags & IR3_INSTR_B) {
      sam->cat5.tex_base = info.base;
   sam->cat5.samp = info.samp_idx;
      }
      }
      /* src[] = { deref, coord, sample_index }. const_index[] = {} */
   static void
   emit_intrinsic_load_image(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
      /* If the image can be written, must use LDIB to retrieve data, rather than
   * through ISAM (which uses the texture cache and won't get previous writes).
   */
   if (!(nir_intrinsic_access(intr) & ACCESS_CAN_REORDER)) {
      ctx->funcs->emit_intrinsic_load_image(ctx, intr, dst);
               /* The sparse set of texture descriptors for non-coherent load_images means we can't do indirection, so
   * fall back to coherent load.
   */
   if (ctx->compiler->gen >= 5 &&
      !ir3_bindless_resource(intr->src[0]) &&
   !nir_src_is_const(intr->src[0])) {
   ctx->funcs->emit_intrinsic_load_image(ctx, intr, dst);
               struct ir3_block *b = ctx->block;
   struct tex_src_info info = get_image_ssbo_samp_tex_src(ctx, &intr->src[0], true);
   struct ir3_instruction *sam;
   struct ir3_instruction *const *src0 = ir3_get_src(ctx, &intr->src[1]);
   struct ir3_instruction *coords[4];
   unsigned flags, ncoords = ir3_get_image_coords(intr, &flags);
                     /* hw doesn't do 1d, so we treat it as 2d with height of 1, and patch up the
   * y coord. Note that the array index must come after the fake y coord.
   */
   enum glsl_sampler_dim dim = nir_intrinsic_image_dim(intr);
   if (dim == GLSL_SAMPLER_DIM_1D || dim == GLSL_SAMPLER_DIM_BUF) {
      coords[0] = src0[0];
   coords[1] = create_immed(b, 0);
   for (unsigned i = 1; i < ncoords; i++)
            } else {
      for (unsigned i = 0; i < ncoords; i++)
               sam = emit_sam(ctx, OPC_ISAM, info, type, 0b1111,
                     sam->barrier_class = IR3_BARRIER_IMAGE_R;
               }
      /* A4xx version of image_size, see ir3_a6xx.c for newer resinfo version. */
   void
   emit_intrinsic_image_size_tex(struct ir3_context *ctx,
               {
      struct ir3_block *b = ctx->block;
   struct tex_src_info info = get_image_ssbo_samp_tex_src(ctx, &intr->src[0], true);
   struct ir3_instruction *sam, *lod;
   unsigned flags, ncoords = ir3_get_image_coords(intr, &flags);
            info.flags |= flags;
   assert(nir_src_as_uint(intr->src[1]) == 0);
   lod = create_immed(b, 0);
            /* Array size actually ends up in .w rather than .z. This doesn't
   * matter for miplevel 0, but for higher mips the value in z is
   * minified whereas w stays. Also, the value in TEX_CONST_3_DEPTH is
   * returned, which means that we have to add 1 to it for arrays for
   * a3xx.
   *
   * Note use a temporary dst and then copy, since the size of the dst
   * array that is passed in is based on nir's understanding of the
   * result size, not the hardware's
   */
                     for (unsigned i = 0; i < ncoords; i++)
            if (flags & IR3_INSTR_A) {
      if (ctx->compiler->levels_add_one) {
         } else {
               }
      /* src[] = { buffer_index, offset }. No const_index */
   static void
   emit_intrinsic_load_ssbo(struct ir3_context *ctx,
               {
      /* Note: isam currently can't handle vectorized loads/stores */
   if (!(nir_intrinsic_access(intr) & ACCESS_CAN_REORDER) ||
      intr->def.num_components > 1 ||
   !ctx->compiler->has_isam_ssbo) {
   ctx->funcs->emit_intrinsic_load_ssbo(ctx, intr, dst);
               struct ir3_block *b = ctx->block;
   struct ir3_instruction *offset = ir3_get_src(ctx, &intr->src[2])[0];
   struct ir3_instruction *coords = ir3_collect(b, offset, create_immed(b, 0));
            unsigned num_components = intr->def.num_components;
   struct ir3_instruction *sam =
      emit_sam(ctx, OPC_ISAM, info, utype_for_size(intr->def.bit_size),
                  sam->barrier_class = IR3_BARRIER_BUFFER_R;
               }
      static void
   emit_control_barrier(struct ir3_context *ctx)
   {
      /* Hull shaders dispatch 32 wide so an entire patch will always
   * fit in a single warp and execute in lock-step. Consequently,
   * we don't need to do anything for TCS barriers. Emitting
   * barrier instruction will deadlock.
   */
   if (ctx->so->type == MESA_SHADER_TESS_CTRL)
            struct ir3_block *b = ctx->block;
   struct ir3_instruction *barrier = ir3_BAR(b);
   barrier->cat7.g = true;
   if (ctx->compiler->gen < 6)
         barrier->flags = IR3_INSTR_SS | IR3_INSTR_SY;
   barrier->barrier_class = IR3_BARRIER_EVERYTHING;
               }
      static void
   emit_intrinsic_barrier(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_block *b = ctx->block;
            /* TODO: find out why there is a major difference of .l usage
   * between a5xx and a6xx,
            mesa_scope exec_scope = nir_intrinsic_execution_scope(intr);
   mesa_scope mem_scope = nir_intrinsic_memory_scope(intr);
   nir_variable_mode modes = nir_intrinsic_memory_modes(intr);
   /* loads/stores are always cache-coherent so we can filter out
   * available/visible.
   */
   nir_memory_semantics semantics =
      nir_intrinsic_memory_semantics(intr) & (NIR_MEMORY_ACQUIRE |
         if (ctx->so->type == MESA_SHADER_TESS_CTRL) {
      /* Remove mode corresponding to TCS patch barriers because hull shaders
   * dispatch 32 wide so an entire patch will always fit in a single warp
   * and execute in lock-step.
   *
   * TODO: memory barrier also tells us not to reorder stores, this
   * information is lost here (backend doesn't reorder stores so we
   * are safe for now).
   */
                        if ((modes & (nir_var_mem_shared | nir_var_mem_ssbo | nir_var_mem_global |
            barrier = ir3_FENCE(b);
   barrier->cat7.r = true;
            if (modes & (nir_var_mem_ssbo | nir_var_image | nir_var_mem_global)) {
                  if (ctx->compiler->gen >= 6) {
      if (modes & (nir_var_mem_ssbo | nir_var_image)) {
            } else {
      if (modes & (nir_var_mem_shared | nir_var_mem_ssbo | nir_var_image)) {
                     barrier->barrier_class = 0;
            if (modes & nir_var_mem_shared) {
      barrier->barrier_class |= IR3_BARRIER_SHARED_W;
   barrier->barrier_conflict |=
               if (modes & (nir_var_mem_ssbo | nir_var_mem_global)) {
      barrier->barrier_class |= IR3_BARRIER_BUFFER_W;
   barrier->barrier_conflict |=
               if (modes & nir_var_image) {
      barrier->barrier_class |= IR3_BARRIER_IMAGE_W;
   barrier->barrier_conflict |=
               /* make sure barrier doesn't get DCE'd */
            if (ctx->compiler->gen >= 7 && mem_scope > SCOPE_WORKGROUP &&
      modes & (nir_var_mem_ssbo | nir_var_image) &&
   semantics & NIR_MEMORY_ACQUIRE) {
   /* "r + l" is not enough to synchronize reads with writes from other
   * workgroups, we can disable them since they are useless here.
   */
                  struct ir3_instruction *ccinv = ir3_CCINV(b);
   /* A7XX TODO: ccinv should just stick to the barrier,
   * the barrier class/conflict introduces unnecessary waits.
   */
   ccinv->barrier_class = barrier->barrier_class;
   ccinv->barrier_conflict = barrier->barrier_conflict;
                  if (exec_scope >= SCOPE_WORKGROUP) {
            }
      static void
   add_sysval_input_compmask(struct ir3_context *ctx, gl_system_value slot,
         {
      struct ir3_shader_variant *so = ctx->so;
            assert(instr->opc == OPC_META_INPUT);
   instr->input.inidx = n;
            so->inputs[n].sysval = true;
   so->inputs[n].slot = slot;
   so->inputs[n].compmask = compmask;
               }
      static struct ir3_instruction *
   create_sysval_input(struct ir3_context *ctx, gl_system_value slot,
         {
      assert(compmask);
   struct ir3_instruction *sysval = create_input(ctx, compmask);
   add_sysval_input_compmask(ctx, slot, compmask, sysval);
      }
      static struct ir3_instruction *
   get_barycentric(struct ir3_context *ctx, enum ir3_bary bary)
   {
      STATIC_ASSERT(SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL + IJ_PERSP_PIXEL ==
         STATIC_ASSERT(SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL + IJ_PERSP_SAMPLE ==
         STATIC_ASSERT(SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL + IJ_PERSP_CENTROID ==
         STATIC_ASSERT(SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL + IJ_PERSP_CENTER_RHW ==
         STATIC_ASSERT(SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL + IJ_LINEAR_PIXEL ==
         STATIC_ASSERT(SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL + IJ_LINEAR_CENTROID ==
         STATIC_ASSERT(SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL + IJ_LINEAR_SAMPLE ==
            if (!ctx->ij[bary]) {
      struct ir3_instruction *xy[2];
            ij = create_sysval_input(ctx, SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL +
                                 }
      /* TODO: make this a common NIR helper?
   * there is a nir_system_value_from_intrinsic but it takes nir_intrinsic_op so
   * it can't be extended to work with this
   */
   static gl_system_value
   nir_intrinsic_barycentric_sysval(nir_intrinsic_instr *intr)
   {
      enum glsl_interp_mode interp_mode = nir_intrinsic_interp_mode(intr);
            switch (intr->intrinsic) {
   case nir_intrinsic_load_barycentric_pixel:
      if (interp_mode == INTERP_MODE_NOPERSPECTIVE)
         else
            case nir_intrinsic_load_barycentric_centroid:
      if (interp_mode == INTERP_MODE_NOPERSPECTIVE)
         else
            case nir_intrinsic_load_barycentric_sample:
      if (interp_mode == INTERP_MODE_NOPERSPECTIVE)
         else
            default:
                     }
      static void
   emit_intrinsic_barycentric(struct ir3_context *ctx, nir_intrinsic_instr *intr,
         {
               if (!ctx->so->key.msaa && ctx->compiler->gen < 6) {
      switch (sysval) {
   case SYSTEM_VALUE_BARYCENTRIC_PERSP_SAMPLE:
      sysval = SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL;
      case SYSTEM_VALUE_BARYCENTRIC_PERSP_CENTROID:
      sysval = SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL;
      case SYSTEM_VALUE_BARYCENTRIC_LINEAR_SAMPLE:
      sysval = SYSTEM_VALUE_BARYCENTRIC_LINEAR_PIXEL;
      case SYSTEM_VALUE_BARYCENTRIC_LINEAR_CENTROID:
      sysval = SYSTEM_VALUE_BARYCENTRIC_LINEAR_PIXEL;
      default:
                              struct ir3_instruction *ij = get_barycentric(ctx, bary);
      }
      static struct ir3_instruction *
   get_frag_coord(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      if (!ctx->frag_coord) {
      struct ir3_block *b = ir3_after_preamble(ctx->ir);
   struct ir3_instruction *xyzw[4];
            hw_frag_coord = create_sysval_input(ctx, SYSTEM_VALUE_FRAG_COORD, 0xf);
            /* for frag_coord.xy, we get unsigned values.. we need
   * to subtract (integer) 8 and divide by 16 (right-
   * shift by 4) then convert to float:
   *
   *    sub.s tmp, src, 8
   *    shr.b tmp, tmp, 4
   *    mov.u32f32 dst, tmp
   *
   */
   for (int i = 0; i < 2; i++) {
      xyzw[i] = ir3_COV(b, xyzw[i], TYPE_U32, TYPE_F32);
   xyzw[i] =
                                       }
      /* This is a bit of a hack until ir3_context is converted to store SSA values
   * as ir3_register's instead of ir3_instruction's. Pick out a given destination
   * of an instruction with multiple destinations using a mov that will get folded
   * away by ir3_cp.
   */
   static struct ir3_instruction *
   create_multidst_mov(struct ir3_block *block, struct ir3_register *dst)
   {
      struct ir3_instruction *mov = ir3_instr_create(block, OPC_MOV, 1, 1);
   unsigned dst_flags = dst->flags & IR3_REG_HALF;
            __ssa_dst(mov)->flags |= dst_flags;
   struct ir3_register *src =
         src->wrmask = dst->wrmask;
   src->def = dst;
   assert(!(dst->flags & IR3_REG_RELATIV));
   mov->cat1.src_type = mov->cat1.dst_type =
            }
      static reduce_op_t
   get_reduce_op(nir_op opc)
   {
      switch (opc) {
   case nir_op_iadd: return REDUCE_OP_ADD_U;
   case nir_op_fadd: return REDUCE_OP_ADD_F;
   case nir_op_imul: return REDUCE_OP_MUL_U;
   case nir_op_fmul: return REDUCE_OP_MUL_F;
   case nir_op_umin: return REDUCE_OP_MIN_U;
   case nir_op_imin: return REDUCE_OP_MIN_S;
   case nir_op_fmin: return REDUCE_OP_MIN_F;
   case nir_op_umax: return REDUCE_OP_MAX_U;
   case nir_op_imax: return REDUCE_OP_MAX_S;
   case nir_op_fmax: return REDUCE_OP_MAX_F;
   case nir_op_iand: return REDUCE_OP_AND_B;
   case nir_op_ior:  return REDUCE_OP_OR_B;
   case nir_op_ixor: return REDUCE_OP_XOR_B;
   default:
            }
      static uint32_t
   get_reduce_identity(nir_op opc, unsigned size)
   {
      switch (opc) {
   case nir_op_iadd:
         case nir_op_fadd: 
         case nir_op_imul:
         case nir_op_fmul:
         case nir_op_umax:
         case nir_op_imax:
         case nir_op_fmax:
         case nir_op_umin:
         case nir_op_imin:
         case nir_op_fmin:
         case nir_op_iand:
         case nir_op_ior:
         case nir_op_ixor:
         default:
            }
      static struct ir3_instruction *
   emit_intrinsic_reduce(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_instruction *src = ir3_get_src(ctx, &intr->src[0])[0];
   nir_op nir_reduce_op = (nir_op) nir_intrinsic_reduction_op(intr);
   reduce_op_t reduce_op = get_reduce_op(nir_reduce_op);
   unsigned dst_size = intr->def.bit_size;
            /* Note: the shared reg is initialized to the identity, so we need it to
   * always be 32-bit even when the source isn't because half shared regs are
   * not supported.
   */
   struct ir3_instruction *identity =
         identity = ir3_READ_FIRST_MACRO(ctx->block, identity, 0);
            /* OPC_SCAN_MACRO has the following destinations:
   * - Exclusive scan result (interferes with source)
   * - Inclusive scan result
   * - Shared reg reduction result, must be initialized to the identity
   *
   * The loop computes all three results at the same time, we just have to
   * choose which destination to return.
   */
   struct ir3_instruction *scan =
                  struct ir3_register *exclusive = __ssa_dst(scan);
   exclusive->flags |= flags | IR3_REG_EARLY_CLOBBER;
   struct ir3_register *inclusive = __ssa_dst(scan);
   inclusive->flags |= flags;
   struct ir3_register *reduce = __ssa_dst(scan);
            /* The 32-bit multiply macro reads its sources after writing a partial result
   * to the destination, therefore inclusive also interferes with the source.
   */
   if (reduce_op == REDUCE_OP_MUL_U && dst_size == 32)
            /* Normal source */
            /* shared reg tied source */
   struct ir3_register *reduce_init = __ssa_src(scan, identity, IR3_REG_SHARED);
   ir3_reg_tie(reduce, reduce_init);
      struct ir3_register *dst;
   switch (intr->intrinsic) {
   case nir_intrinsic_reduce: dst = reduce; break;
   case nir_intrinsic_inclusive_scan: dst = inclusive; break;
   case nir_intrinsic_exclusive_scan: dst = exclusive; break;
   default:
                     }
      static void setup_input(struct ir3_context *ctx, nir_intrinsic_instr *intr);
   static void setup_output(struct ir3_context *ctx, nir_intrinsic_instr *intr);
      static void
   emit_intrinsic(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      const nir_intrinsic_info *info = &nir_intrinsic_infos[intr->intrinsic];
   struct ir3_instruction **dst;
   struct ir3_instruction *const *src;
   struct ir3_block *b = ctx->block;
   unsigned dest_components = nir_intrinsic_dest_components(intr);
            if (info->has_dest) {
         } else {
                  const struct ir3_const_state *const_state = ir3_const_state(ctx->so);
   const unsigned primitive_param = const_state->offsets.primitive_param * 4;
            switch (intr->intrinsic) {
   case nir_intrinsic_decl_reg:
      /* There's logically nothing to do, but this has a destination in NIR so
   * plug in something... It will get DCE'd.
   */
   dst[0] = create_immed(ctx->block, 0);
         case nir_intrinsic_load_reg:
   case nir_intrinsic_load_reg_indirect: {
      struct ir3_array *arr = ir3_get_array(ctx, intr->src[0].ssa);
            if (intr->intrinsic == nir_intrinsic_load_reg_indirect) {
      addr = ir3_get_addr0(ctx, ir3_get_src(ctx, &intr->src[1])[0],
               ASSERTED nir_intrinsic_instr *decl = nir_reg_get_decl(intr->src[0].ssa);
            for (unsigned i = 0; i < dest_components; i++) {
      unsigned n = nir_intrinsic_base(intr) * dest_components + i;
   compile_assert(ctx, n < arr->length);
                           case nir_intrinsic_store_reg:
   case nir_intrinsic_store_reg_indirect: {
      struct ir3_array *arr = ir3_get_array(ctx, intr->src[1].ssa);
   unsigned num_components = nir_src_num_components(intr->src[0]);
            ASSERTED nir_intrinsic_instr *decl = nir_reg_get_decl(intr->src[1].ssa);
                     if (intr->intrinsic == nir_intrinsic_store_reg_indirect) {
      addr = ir3_get_addr0(ctx, ir3_get_src(ctx, &intr->src[2])[0],
               u_foreach_bit(i, nir_intrinsic_write_mask(intr)) {
               unsigned n = nir_intrinsic_base(intr) * num_components + i;
   compile_assert(ctx, n < arr->length);
   if (value[i])
                           case nir_intrinsic_load_uniform:
      idx = nir_intrinsic_base(intr);
   if (nir_src_is_const(intr->src[0])) {
      idx += nir_src_as_uint(intr->src[0]);
   for (int i = 0; i < dest_components; i++) {
      dst[i] = create_uniform_typed(
      b, idx + i,
      } else {
      src = ir3_get_src(ctx, &intr->src[0]);
   for (int i = 0; i < dest_components; i++) {
      dst[i] = create_uniform_indirect(
      b, idx + i,
   intr->def.bit_size == 16 ? TYPE_F16 : TYPE_F32,
   }
   /* NOTE: if relative addressing is used, we set
   * constlen in the compiler (to worst-case value)
   * since we don't know in the assembler what the max
   * addr reg value can be:
   */
   ctx->so->constlen =
      MAX2(ctx->so->constlen,
         }
         case nir_intrinsic_load_vs_primitive_stride_ir3:
      dst[0] = create_uniform(b, primitive_param + 0);
      case nir_intrinsic_load_vs_vertex_stride_ir3:
      dst[0] = create_uniform(b, primitive_param + 1);
      case nir_intrinsic_load_hs_patch_stride_ir3:
      dst[0] = create_uniform(b, primitive_param + 2);
      case nir_intrinsic_load_patch_vertices_in:
      dst[0] = create_uniform(b, primitive_param + 3);
      case nir_intrinsic_load_tess_param_base_ir3:
      dst[0] = create_uniform(b, primitive_param + 4);
   dst[1] = create_uniform(b, primitive_param + 5);
      case nir_intrinsic_load_tess_factor_base_ir3:
      dst[0] = create_uniform(b, primitive_param + 6);
   dst[1] = create_uniform(b, primitive_param + 7);
         case nir_intrinsic_load_primitive_location_ir3:
      idx = nir_intrinsic_driver_location(intr);
   dst[0] = create_uniform(b, primitive_map + idx);
         case nir_intrinsic_load_gs_header_ir3:
      dst[0] = ctx->gs_header;
      case nir_intrinsic_load_tcs_header_ir3:
      dst[0] = ctx->tcs_header;
         case nir_intrinsic_load_rel_patch_id_ir3:
      dst[0] = ctx->rel_patch_id;
         case nir_intrinsic_load_primitive_id:
      if (!ctx->primitive_id) {
      ctx->primitive_id =
      }
   dst[0] = ctx->primitive_id;
         case nir_intrinsic_load_tess_coord_xy:
      if (!ctx->tess_coord) {
      ctx->tess_coord =
      }
   ir3_split_dest(b, dst, ctx->tess_coord, 0, 2);
         case nir_intrinsic_end_patch_ir3:
      assert(ctx->so->type == MESA_SHADER_TESS_CTRL);
   struct ir3_instruction *end = ir3_PREDE(b);
            end->barrier_class = IR3_BARRIER_EVERYTHING;
   end->barrier_conflict = IR3_BARRIER_EVERYTHING;
         case nir_intrinsic_store_global_ir3:
      ctx->funcs->emit_intrinsic_store_global_ir3(ctx, intr);
      case nir_intrinsic_load_global_ir3:
      ctx->funcs->emit_intrinsic_load_global_ir3(ctx, intr, dst);
         case nir_intrinsic_load_ubo:
      emit_intrinsic_load_ubo(ctx, intr, dst);
      case nir_intrinsic_load_ubo_vec4:
      emit_intrinsic_load_ubo_ldc(ctx, intr, dst);
      case nir_intrinsic_copy_ubo_to_uniform_ir3:
      emit_intrinsic_copy_ubo_to_uniform(ctx, intr);
      case nir_intrinsic_load_frag_coord:
   case nir_intrinsic_load_frag_coord_unscaled_ir3:
      ir3_split_dest(b, dst, get_frag_coord(ctx, intr), 0, 4);
      case nir_intrinsic_load_sample_pos_from_id: {
      /* NOTE: blob seems to always use TYPE_F16 and then cov.f16f32,
   * but that doesn't seem necessary.
   */
   struct ir3_instruction *offset =
         offset->dsts[0]->wrmask = 0x3;
                        }
   case nir_intrinsic_load_persp_center_rhw_ir3:
      if (!ctx->ij[IJ_PERSP_CENTER_RHW]) {
      ctx->ij[IJ_PERSP_CENTER_RHW] =
      }
   dst[0] = ctx->ij[IJ_PERSP_CENTER_RHW];
      case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_sample:
   case nir_intrinsic_load_barycentric_pixel:
      emit_intrinsic_barycentric(ctx, intr, dst);
      case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_input:
      setup_input(ctx, intr);
      case nir_intrinsic_load_kernel_input:
      emit_intrinsic_load_kernel_input(ctx, intr, dst);
      /* All SSBO intrinsics should have been lowered by 'lower_io_offsets'
   * pass and replaced by an ir3-specifc version that adds the
   * dword-offset in the last source.
   */
   case nir_intrinsic_load_ssbo_ir3:
      emit_intrinsic_load_ssbo(ctx, intr, dst);
      case nir_intrinsic_store_ssbo_ir3:
      ctx->funcs->emit_intrinsic_store_ssbo(ctx, intr);
      case nir_intrinsic_get_ssbo_size:
      emit_intrinsic_ssbo_size(ctx, intr, dst);
      case nir_intrinsic_ssbo_atomic_ir3:
   case nir_intrinsic_ssbo_atomic_swap_ir3:
      dst[0] = ctx->funcs->emit_intrinsic_atomic_ssbo(ctx, intr);
      case nir_intrinsic_load_shared:
      emit_intrinsic_load_shared(ctx, intr, dst);
      case nir_intrinsic_store_shared:
      emit_intrinsic_store_shared(ctx, intr);
      case nir_intrinsic_shared_atomic:
   case nir_intrinsic_shared_atomic_swap:
      dst[0] = emit_intrinsic_atomic_shared(ctx, intr);
      case nir_intrinsic_load_scratch:
      emit_intrinsic_load_scratch(ctx, intr, dst);
      case nir_intrinsic_store_scratch:
      emit_intrinsic_store_scratch(ctx, intr);
      case nir_intrinsic_image_load:
   case nir_intrinsic_bindless_image_load:
      emit_intrinsic_load_image(ctx, intr, dst);
      case nir_intrinsic_image_store:
   case nir_intrinsic_bindless_image_store:
      ctx->funcs->emit_intrinsic_store_image(ctx, intr);
      case nir_intrinsic_image_size:
   case nir_intrinsic_bindless_image_size:
      ctx->funcs->emit_intrinsic_image_size(ctx, intr, dst);
      case nir_intrinsic_image_atomic:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_bindless_image_atomic_swap:
      dst[0] = ctx->funcs->emit_intrinsic_atomic_image(ctx, intr);
      case nir_intrinsic_barrier:
      emit_intrinsic_barrier(ctx, intr);
   /* note that blk ptr no longer valid, make that obvious: */
   b = NULL;
      case nir_intrinsic_store_output:
      setup_output(ctx, intr);
      case nir_intrinsic_load_base_vertex:
   case nir_intrinsic_load_first_vertex:
      if (!ctx->basevertex) {
         }
   dst[0] = ctx->basevertex;
      case nir_intrinsic_load_is_indexed_draw:
      if (!ctx->is_indexed_draw) {
         }
   dst[0] = ctx->is_indexed_draw;
      case nir_intrinsic_load_draw_id:
      if (!ctx->draw_id) {
         }
   dst[0] = ctx->draw_id;
      case nir_intrinsic_load_base_instance:
      if (!ctx->base_instance) {
         }
   dst[0] = ctx->base_instance;
      case nir_intrinsic_load_view_index:
      if (!ctx->view_index) {
      ctx->view_index =
      }
   dst[0] = ctx->view_index;
      case nir_intrinsic_load_vertex_id_zero_base:
   case nir_intrinsic_load_vertex_id:
      if (!ctx->vertex_id) {
      gl_system_value sv = (intr->intrinsic == nir_intrinsic_load_vertex_id)
                  }
   dst[0] = ctx->vertex_id;
      case nir_intrinsic_load_instance_id:
      if (!ctx->instance_id) {
      ctx->instance_id =
      }
   dst[0] = ctx->instance_id;
      case nir_intrinsic_load_sample_id:
   case nir_intrinsic_load_sample_id_no_per_sample:
      if (!ctx->samp_id) {
      ctx->samp_id = create_sysval_input(ctx, SYSTEM_VALUE_SAMPLE_ID, 0x1);
      }
   dst[0] = ir3_COV(b, ctx->samp_id, TYPE_U16, TYPE_U32);
      case nir_intrinsic_load_sample_mask_in:
      if (!ctx->samp_mask_in) {
      ctx->samp_mask_in =
      }
   dst[0] = ctx->samp_mask_in;
      case nir_intrinsic_load_user_clip_plane:
      idx = nir_intrinsic_ucp_id(intr);
   for (int i = 0; i < dest_components; i++) {
      unsigned n = idx * 4 + i;
      }
      case nir_intrinsic_load_front_face:
      if (!ctx->frag_face) {
      ctx->so->frag_face = true;
   ctx->frag_face =
            }
   /* for fragface, we get -1 for back and 0 for front. However this is
   * the inverse of what nir expects (where ~0 is true).
   */
   dst[0] = ir3_CMPS_S(b, ctx->frag_face, 0,
         dst[0]->cat2.condition = IR3_COND_EQ;
      case nir_intrinsic_load_local_invocation_id:
      if (!ctx->local_invocation_id) {
      ctx->local_invocation_id =
      }
   ir3_split_dest(b, dst, ctx->local_invocation_id, 0, 3);
      case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_workgroup_id_zero_base:
      if (ctx->compiler->has_shared_regfile) {
      if (!ctx->work_group_id) {
      ctx->work_group_id =
            }
      } else {
      /* For a3xx/a4xx, this comes in via const injection by the hw */
   for (int i = 0; i < dest_components; i++) {
            }
      case nir_intrinsic_load_base_workgroup_id:
      for (int i = 0; i < dest_components; i++) {
         }
      case nir_intrinsic_load_num_workgroups:
      for (int i = 0; i < dest_components; i++) {
         }
      case nir_intrinsic_load_workgroup_size:
      for (int i = 0; i < dest_components; i++) {
         }
      case nir_intrinsic_load_subgroup_size: {
      assert(ctx->so->type == MESA_SHADER_COMPUTE ||
         enum ir3_driver_param size = ctx->so->type == MESA_SHADER_COMPUTE ?
         dst[0] = create_driver_param(ctx, size);
      }
   case nir_intrinsic_load_subgroup_id_shift_ir3:
      dst[0] = create_driver_param(ctx, IR3_DP_SUBGROUP_ID_SHIFT);
      case nir_intrinsic_load_work_dim:
      dst[0] = create_driver_param(ctx, IR3_DP_WORK_DIM);
      case nir_intrinsic_load_subgroup_invocation:
      assert(ctx->compiler->has_getfiberid);
   dst[0] = ir3_GETFIBERID(b);
   dst[0]->cat6.type = TYPE_U32;
   __ssa_dst(dst[0]);
      case nir_intrinsic_load_tess_level_outer_default:
      for (int i = 0; i < dest_components; i++) {
         }
      case nir_intrinsic_load_tess_level_inner_default:
      for (int i = 0; i < dest_components; i++) {
         }
      case nir_intrinsic_load_frag_invocation_count:
      dst[0] = create_driver_param(ctx, IR3_DP_FS_FRAG_INVOCATION_COUNT);
      case nir_intrinsic_load_frag_size_ir3:
   case nir_intrinsic_load_frag_offset_ir3: {
      enum ir3_driver_param param =
      intr->intrinsic == nir_intrinsic_load_frag_size_ir3 ?
      if (nir_src_is_const(intr->src[0])) {
      uint32_t view = nir_src_as_uint(intr->src[0]);
   for (int i = 0; i < dest_components; i++) {
            } else {
      struct ir3_instruction *view = ir3_get_src(ctx, &intr->src[0])[0];
   for (int i = 0; i < dest_components; i++) {
      dst[i] = create_driver_param_indirect(ctx, param + i,
      }
   ctx->so->constlen =
      MAX2(ctx->so->constlen,
         }
      }
   case nir_intrinsic_discard_if:
   case nir_intrinsic_discard:
   case nir_intrinsic_demote:
   case nir_intrinsic_demote_if:
   case nir_intrinsic_terminate:
   case nir_intrinsic_terminate_if: {
               if (intr->intrinsic == nir_intrinsic_discard_if ||
      intr->intrinsic == nir_intrinsic_demote_if ||
   intr->intrinsic == nir_intrinsic_terminate_if) {
   /* conditional discard: */
   src = ir3_get_src(ctx, &intr->src[0]);
      } else {
      /* unconditional discard: */
               /* NOTE: only cmps.*.* can write p0.x: */
   struct ir3_instruction *zero =
         cond = ir3_CMPS_S(b, cond, 0, zero, 0);
            /* condition always goes in predicate register: */
   cond->dsts[0]->num = regid(REG_P0, 0);
            if (intr->intrinsic == nir_intrinsic_demote ||
      intr->intrinsic == nir_intrinsic_demote_if) {
      } else {
                  /* - Side-effects should not be moved on a different side of the kill
   * - Instructions that depend on active fibers should not be reordered
   */
   kill->barrier_class = IR3_BARRIER_IMAGE_W | IR3_BARRIER_BUFFER_W |
         kill->barrier_conflict = IR3_BARRIER_IMAGE_W | IR3_BARRIER_BUFFER_W |
         kill->srcs[0]->num = regid(REG_P0, 0);
            array_insert(b, b->keeps, kill);
                        case nir_intrinsic_cond_end_ir3: {
               src = ir3_get_src(ctx, &intr->src[0]);
            /* NOTE: only cmps.*.* can write p0.x: */
   struct ir3_instruction *zero =
         cond = ir3_CMPS_S(b, cond, 0, zero, 0);
            /* condition always goes in predicate register: */
                     kill->barrier_class = IR3_BARRIER_EVERYTHING;
            array_insert(ctx->ir, ctx->ir->predicates, kill);
   array_insert(b, b->keeps, kill);
               case nir_intrinsic_vote_any:
   case nir_intrinsic_vote_all: {
      struct ir3_instruction *src = ir3_get_src(ctx, &intr->src[0])[0];
   struct ir3_instruction *pred = ir3_get_predicate(ctx, src);
   if (intr->intrinsic == nir_intrinsic_vote_any)
         else
         dst[0]->srcs[0]->num = regid(REG_P0, 0);
   array_insert(ctx->ir, ctx->ir->predicates, dst[0]);
      }
   case nir_intrinsic_elect:
      dst[0] = ir3_ELECT_MACRO(ctx->block);
   /* This may expand to a divergent if/then, so allocate stack space for
   * it.
   */
   ctx->max_stack = MAX2(ctx->max_stack, ctx->stack + 1);
      case nir_intrinsic_preamble_start_ir3:
      dst[0] = ir3_SHPS_MACRO(ctx->block);
   ctx->max_stack = MAX2(ctx->max_stack, ctx->stack + 1);
         case nir_intrinsic_read_invocation_cond_ir3: {
      struct ir3_instruction *src = ir3_get_src(ctx, &intr->src[0])[0];
   struct ir3_instruction *cond = ir3_get_src(ctx, &intr->src[1])[0];
   dst[0] = ir3_READ_COND_MACRO(ctx->block, ir3_get_predicate(ctx, cond), 0,
         dst[0]->dsts[0]->flags |= IR3_REG_SHARED;
   dst[0]->srcs[0]->num = regid(REG_P0, 0);
   array_insert(ctx->ir, ctx->ir->predicates, dst[0]);
   ctx->max_stack = MAX2(ctx->max_stack, ctx->stack + 1);
               case nir_intrinsic_read_first_invocation: {
      struct ir3_instruction *src = ir3_get_src(ctx, &intr->src[0])[0];
   dst[0] = ir3_READ_FIRST_MACRO(ctx->block, src, 0);
   dst[0]->dsts[0]->flags |= IR3_REG_SHARED;
   ctx->max_stack = MAX2(ctx->max_stack, ctx->stack + 1);
               case nir_intrinsic_ballot: {
      struct ir3_instruction *ballot;
   unsigned components = intr->def.num_components;
   if (nir_src_is_const(intr->src[0]) && nir_src_as_bool(intr->src[0])) {
      /* ballot(true) is just MOVMSK */
      } else {
      struct ir3_instruction *src = ir3_get_src(ctx, &intr->src[0])[0];
   struct ir3_instruction *pred = ir3_get_predicate(ctx, src);
   ballot = ir3_BALLOT_MACRO(ctx->block, pred, components);
   ballot->srcs[0]->num = regid(REG_P0, 0);
   array_insert(ctx->ir, ctx->ir->predicates, ballot);
               ballot->barrier_class = IR3_BARRIER_ACTIVE_FIBERS_R;
            ir3_split_dest(ctx->block, dst, ballot, 0, components);
               case nir_intrinsic_quad_broadcast: {
      struct ir3_instruction *src = ir3_get_src(ctx, &intr->src[0])[0];
                     if (dst_type != TYPE_U32)
            dst[0] = ir3_QUAD_SHUFFLE_BRCST(ctx->block, src, 0, idx, 0);
   dst[0]->cat5.type = dst_type;
               case nir_intrinsic_quad_swap_horizontal: {
      struct ir3_instruction *src = ir3_get_src(ctx, &intr->src[0])[0];
   dst[0] = ir3_QUAD_SHUFFLE_HORIZ(ctx->block, src, 0);
   dst[0]->cat5.type = type_uint_size(intr->def.bit_size);
               case nir_intrinsic_quad_swap_vertical: {
      struct ir3_instruction *src = ir3_get_src(ctx, &intr->src[0])[0];
   dst[0] = ir3_QUAD_SHUFFLE_VERT(ctx->block, src, 0);
   dst[0]->cat5.type = type_uint_size(intr->def.bit_size);
               case nir_intrinsic_quad_swap_diagonal: {
      struct ir3_instruction *src = ir3_get_src(ctx, &intr->src[0])[0];
   dst[0] = ir3_QUAD_SHUFFLE_DIAG(ctx->block, src, 0);
   dst[0]->cat5.type = type_uint_size(intr->def.bit_size);
               case nir_intrinsic_load_shared_ir3:
      emit_intrinsic_load_shared_ir3(ctx, intr, dst);
      case nir_intrinsic_store_shared_ir3:
      emit_intrinsic_store_shared_ir3(ctx, intr);
      case nir_intrinsic_bindless_resource_ir3:
      dst[0] = ir3_get_src(ctx, &intr->src[0])[0];
      case nir_intrinsic_global_atomic_ir3:
   case nir_intrinsic_global_atomic_swap_ir3: {
      dst[0] = ctx->funcs->emit_intrinsic_atomic_global(ctx, intr);
               case nir_intrinsic_reduce:
   case nir_intrinsic_inclusive_scan:
   case nir_intrinsic_exclusive_scan:
      dst[0] = emit_intrinsic_reduce(ctx, intr);
         case nir_intrinsic_preamble_end_ir3: {
      struct ir3_instruction *instr = ir3_SHPE(ctx->block);
   instr->barrier_class = instr->barrier_conflict = IR3_BARRIER_CONST_W;
   array_insert(b, b->keeps, instr);
      }
   case nir_intrinsic_store_uniform_ir3: {
      unsigned components = nir_src_num_components(intr->src[0]);
   unsigned dst = nir_intrinsic_base(intr);
   unsigned dst_lo = dst & 0xff;
            struct ir3_instruction *src =
         struct ir3_instruction *a1 = NULL;
   if (dst_hi) {
      /* Encode only the high part of the destination in a1.x to increase the
   * chance that we can reuse the a1.x value in subsequent stc
   * instructions.
   */
               struct ir3_instruction *stc =
         stc->cat6.iim_val = components;
   stc->cat6.type = TYPE_U32;
   stc->barrier_conflict = IR3_BARRIER_CONST_W;
   if (a1) {
      ir3_instr_set_address(stc, a1);
      }
   array_insert(b, b->keeps, stc);
      }
   case nir_intrinsic_copy_push_const_to_uniform_ir3: {
      struct ir3_instruction *load =
                  load->push_consts.dst_base = nir_src_as_uint(intr->src[0]);
   load->push_consts.src_base = nir_intrinsic_base(intr);
            ctx->so->constlen =
      MAX2(ctx->so->constlen,
      DIV_ROUND_UP(
      }
   default:
      ir3_context_error(ctx, "Unhandled intrinsic type: %s\n",
                     if (info->has_dest)
      }
      static void
   emit_load_const(struct ir3_context *ctx, nir_load_const_instr *instr)
   {
      struct ir3_instruction **dst =
                  if (bit_size <= 8) {
      for (int i = 0; i < instr->def.num_components; i++)
      } else if (bit_size <= 16) {
      for (int i = 0; i < instr->def.num_components; i++)
      } else {
      for (int i = 0; i < instr->def.num_components; i++)
         }
      static void
   emit_undef(struct ir3_context *ctx, nir_undef_instr *undef)
   {
      struct ir3_instruction **dst =
                  /* backend doesn't want undefined instructions, so just plug
   * in 0.0..
   */
   for (int i = 0; i < undef->def.num_components; i++)
      }
      /*
   * texture fetch/sample instructions:
   */
      static type_t
   get_tex_dest_type(nir_tex_instr *tex)
   {
               switch (tex->dest_type) {
   case nir_type_float32:
         case nir_type_float16:
         case nir_type_int32:
         case nir_type_int16:
         case nir_type_bool32:
   case nir_type_uint32:
         case nir_type_bool16:
   case nir_type_uint16:
         case nir_type_invalid:
   default:
                     }
      static void
   tex_info(nir_tex_instr *tex, unsigned *flagsp, unsigned *coordsp)
   {
      unsigned coords =
                  /* note: would use tex->coord_components.. except txs.. also,
   * since array index goes after shadow ref, we don't want to
   * count it:
   */
   if (coords == 3)
            if (tex->is_shadow && tex->op != nir_texop_lod)
            if (tex->is_array && tex->op != nir_texop_lod)
            *flagsp = flags;
      }
      /* Gets the sampler/texture idx as a hvec2.  Which could either be dynamic
   * or immediate (in which case it will get lowered later to a non .s2en
   * version of the tex instruction which encode tex/samp as immediates:
   */
   static struct tex_src_info
   get_tex_samp_tex_src(struct ir3_context *ctx, nir_tex_instr *tex)
   {
      struct ir3_block *b = ctx->block;
   struct tex_src_info info = {0};
   int texture_idx = nir_tex_instr_src_index(tex, nir_tex_src_texture_handle);
   int sampler_idx = nir_tex_instr_src_index(tex, nir_tex_src_sampler_handle);
            if (texture_idx >= 0 || sampler_idx >= 0) {
      /* Bindless case */
            if (tex->texture_non_uniform || tex->sampler_non_uniform)
            /* Gather information required to determine which encoding to
   * choose as well as for prefetch.
   */
   nir_intrinsic_instr *bindless_tex = NULL;
   bool tex_const;
   if (texture_idx >= 0) {
      ctx->so->bindless_tex = true;
   bindless_tex = ir3_bindless_resource(tex->src[texture_idx].src);
   assert(bindless_tex);
   info.tex_base = nir_intrinsic_desc_set(bindless_tex);
   tex_const = nir_src_is_const(bindless_tex->src[0]);
   if (tex_const)
      } else {
      /* To simplify some of the logic below, assume the index is
   * constant 0 when it's not enabled.
   */
   tex_const = true;
      }
   nir_intrinsic_instr *bindless_samp = NULL;
   bool samp_const;
   if (sampler_idx >= 0) {
      ctx->so->bindless_samp = true;
   bindless_samp = ir3_bindless_resource(tex->src[sampler_idx].src);
   assert(bindless_samp);
   info.samp_base = nir_intrinsic_desc_set(bindless_samp);
   samp_const = nir_src_is_const(bindless_samp->src[0]);
   if (samp_const)
      } else {
      samp_const = true;
               /* Choose encoding. */
   if (tex_const && samp_const && info.tex_idx < 256 &&
      info.samp_idx < 256) {
   if (info.tex_idx < 16 && info.samp_idx < 16 &&
      (!bindless_tex || !bindless_samp ||
   info.tex_base == info.samp_base)) {
   /* Everything fits within the instruction */
      } else {
      info.base = info.tex_base;
   if (ctx->compiler->gen <= 6) {
         } else {
                     }
      } else {
      info.flags |= IR3_INSTR_S2EN;
   /* In the indirect case, we only use a1.x to store the sampler
   * base if it differs from the texture base.
   */
   if (!bindless_tex || !bindless_samp ||
      info.tex_base == info.samp_base) {
      } else {
      info.base = info.tex_base;
   info.a1_val = info.samp_base;
               /* Note: the indirect source is now a vec2 instead of hvec2, and
   * for some reason the texture and sampler are swapped.
                  if (bindless_tex) {
         } else {
                  if (bindless_samp) {
         } else {
         }
         } else {
      info.flags |= IR3_INSTR_S2EN;
   texture_idx = nir_tex_instr_src_index(tex, nir_tex_src_texture_offset);
   sampler_idx = nir_tex_instr_src_index(tex, nir_tex_src_sampler_offset);
   if (texture_idx >= 0) {
      texture = ir3_get_src(ctx, &tex->src[texture_idx].src)[0];
      } else {
      /* TODO what to do for dynamic case? I guess we only need the
   * max index for astc srgb workaround so maybe not a problem
   * to worry about if we don't enable indirect samplers for
   * a4xx?
   */
   ctx->max_texture_index =
         texture = create_immed_typed(ctx->block, tex->texture_index, TYPE_U16);
               if (sampler_idx >= 0) {
      sampler = ir3_get_src(ctx, &tex->src[sampler_idx].src)[0];
      } else {
      sampler = create_immed_typed(ctx->block, tex->sampler_index, TYPE_U16);
                              }
      static void
   emit_tex(struct ir3_context *ctx, nir_tex_instr *tex)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction **dst, *sam, *src0[12], *src1[4];
   struct ir3_instruction *const *coord, *const *off, *const *ddx, *const *ddy;
   struct ir3_instruction *lod, *compare, *proj, *sample_index;
   struct tex_src_info info = {0};
   bool has_bias = false, has_lod = false, has_proj = false, has_off = false;
   unsigned i, coords, flags, ncomp;
   unsigned nsrc0 = 0, nsrc1 = 0;
   type_t type;
                     coord = off = ddx = ddy = NULL;
                     for (unsigned i = 0; i < tex->num_srcs; i++) {
      switch (tex->src[i].src_type) {
   case nir_tex_src_coord:
      coord = ir3_get_src(ctx, &tex->src[i].src);
      case nir_tex_src_bias:
      lod = ir3_get_src(ctx, &tex->src[i].src)[0];
   has_bias = true;
      case nir_tex_src_lod:
      lod = ir3_get_src(ctx, &tex->src[i].src)[0];
   has_lod = true;
      case nir_tex_src_comparator: /* shadow comparator */
      compare = ir3_get_src(ctx, &tex->src[i].src)[0];
      case nir_tex_src_projector:
      proj = ir3_get_src(ctx, &tex->src[i].src)[0];
   has_proj = true;
      case nir_tex_src_offset:
      off = ir3_get_src(ctx, &tex->src[i].src);
   has_off = true;
      case nir_tex_src_ddx:
      ddx = ir3_get_src(ctx, &tex->src[i].src);
      case nir_tex_src_ddy:
      ddy = ir3_get_src(ctx, &tex->src[i].src);
      case nir_tex_src_ms_index:
      sample_index = ir3_get_src(ctx, &tex->src[i].src)[0];
      case nir_tex_src_texture_offset:
   case nir_tex_src_sampler_offset:
   case nir_tex_src_texture_handle:
   case nir_tex_src_sampler_handle:
      /* handled in get_tex_samp_src() */
      default:
      ir3_context_error(ctx, "Unhandled NIR tex src type: %d\n",
                        switch (tex->op) {
   case nir_texop_tex_prefetch:
      compile_assert(ctx, !has_bias);
   compile_assert(ctx, !has_lod);
   compile_assert(ctx, !compare);
   compile_assert(ctx, !has_proj);
   compile_assert(ctx, !has_off);
   compile_assert(ctx, !ddx);
   compile_assert(ctx, !ddy);
   compile_assert(ctx, !sample_index);
   compile_assert(
         compile_assert(
            if (ctx->so->num_sampler_prefetch < ctx->prefetch_limit) {
      opc = OPC_META_TEX_PREFETCH;
   ctx->so->num_sampler_prefetch++;
      }
      case nir_texop_tex:
      opc = has_lod ? OPC_SAML : OPC_SAM;
      case nir_texop_txb:
      opc = OPC_SAMB;
      case nir_texop_txl:
      opc = OPC_SAML;
      case nir_texop_txd:
      opc = OPC_SAMGQ;
      case nir_texop_txf:
      opc = OPC_ISAML;
      case nir_texop_lod:
      opc = OPC_GETLOD;
      case nir_texop_tg4:
      switch (tex->component) {
   case 0:
      opc = OPC_GATHER4R;
      case 1:
      opc = OPC_GATHER4G;
      case 2:
      opc = OPC_GATHER4B;
      case 3:
      opc = OPC_GATHER4A;
      }
      case nir_texop_txf_ms_fb:
   case nir_texop_txf_ms:
      opc = OPC_ISAMM;
      default:
      ir3_context_error(ctx, "Unhandled NIR tex type: %d\n", tex->op);
                        /*
   * lay out the first argument in the proper order:
   *  - actual coordinates first
   *  - shadow reference
   *  - array index
   *  - projection w
   *  - starting at offset 4, dpdx.xy, dpdy.xy
   *
   * bias/lod go into the second arg
            /* insert tex coords: */
   for (i = 0; i < coords; i++)
                     type_t coord_pad_type = is_half(coord[0]) ? TYPE_U16 : TYPE_U32;
   /* scale up integer coords for TXF based on the LOD */
   if (ctx->compiler->unminify_coords && (opc == OPC_ISAML)) {
      assert(has_lod);
   for (i = 0; i < coords; i++)
               if (coords == 1) {
      /* hw doesn't do 1d, so we treat it as 2d with
   * height of 1, and patch up the y coord.
   */
   if (is_isam(opc)) {
         } else if (is_half(coord[0])) {
         } else {
                     if (tex->is_shadow && tex->op != nir_texop_lod)
            if (tex->is_array && tex->op != nir_texop_lod)
            if (has_proj) {
      src0[nsrc0++] = proj;
               /* pad to 4, then ddx/ddy: */
   if (tex->op == nir_texop_txd) {
      while (nsrc0 < 4)
         for (i = 0; i < coords; i++)
         if (coords < 2)
         for (i = 0; i < coords; i++)
         if (coords < 2)
               /* NOTE a3xx (and possibly a4xx?) might be different, using isaml
   * with scaled x coord according to requested sample:
   */
   if (opc == OPC_ISAMM) {
      if (ctx->compiler->txf_ms_with_isaml) {
      /* the samples are laid out in x dimension as
   *     0 1 2 3
   * x_ms = (x << ms) + sample_index;
   */
                                    } else {
                     /*
   * second argument (if applicable):
   *  - offsets
   *  - lod
   *  - bias
   */
   if (has_off | has_lod | has_bias) {
      if (has_off) {
      unsigned off_coords = coords;
   if (tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE)
         for (i = 0; i < off_coords; i++)
         if (off_coords < 2)
                     if (has_lod | has_bias)
                        if (opc == OPC_GETLOD)
            if (tex->op == nir_texop_txf_ms_fb) {
               ctx->so->fb_read = true;
   if (ctx->compiler->options.bindless_fb_read_descriptor >= 0) {
      ctx->so->bindless_tex = true;
   info.flags = IR3_INSTR_B;
                  int base_index =
                  if (nir_src_is_const(tex_src)) {
      texture = create_immed_typed(b,
      nir_src_as_uint(tex_src) + ctx->compiler->options.bindless_fb_read_slot,
   } else {
      texture = create_immed_typed(
         struct ir3_instruction *base =
            }
   sampler = create_immed_typed(ctx->block, 0, TYPE_U32);
   info.samp_tex = ir3_collect(b, texture, sampler);
   info.flags |= IR3_INSTR_S2EN;
   if (tex->texture_non_uniform) {
            } else {
      /* Otherwise append a sampler to be patched into the texture
   * state:
   */
   info.samp_tex = ir3_collect(
         b, create_immed_typed(ctx->block, ctx->so->num_samp, TYPE_U16),
                  } else {
                  bool tg4_swizzle_fixup = false;
   if (tex->op == nir_texop_tg4 && ctx->compiler->gen == 4 &&
            uint16_t swizzles = ctx->sampler_swizzles[tex->texture_index];
   uint16_t swizzle = (swizzles >> (tex->component * 3)) & 7;
   if (swizzle > 3) {
      /* this would mean that we can just return 0 / 1, no texturing
   * necessary
   */
   struct ir3_instruction *imm = create_immed(b,
         for (int i = 0; i < 4; i++)
         ir3_put_def(ctx, &tex->def);
      }
   opc = OPC_GATHER4R + swizzle;
               struct ir3_instruction *col0 = ir3_create_collect(b, src0, nsrc0);
            if (opc == OPC_META_TEX_PREFETCH) {
                  sam = ir3_SAM(ctx->in_block, opc, type, MASK(ncomp), 0, NULL,
         sam->prefetch.input_offset = ir3_nir_coord_offset(tex->src[idx].src.ssa);
   /* make sure not to add irrelevant flags like S2EN */
   sam->flags = flags | (info.flags & IR3_INSTR_B);
   sam->prefetch.tex = info.tex_idx;
   sam->prefetch.samp = info.samp_idx;
   sam->prefetch.tex_base = info.tex_base;
      } else {
      info.flags |= flags;
               if (tg4_swizzle_fixup) {
      /* TODO: fix-up for ASTC when alpha is selected? */
                     uint8_t tex_bits = ctx->sampler_swizzles[tex->texture_index] >> 12;
   if (!type_float(type) && tex_bits != 3 /* 32bpp */ &&
            uint8_t bits = 0;
   switch (tex_bits) {
   case 1: /* 8bpp */
      bits = 8;
      case 2: /* 16bpp */
      bits = 16;
      case 4: /* 10bpp or 2bpp for alpha */
      if (opc == OPC_GATHER4A)
         else
            default:
                  sam->cat5.type = TYPE_F32;
   for (int i = 0; i < 4; i++) {
      /* scale and offset the unorm data */
   dst[i] = ir3_MAD_F32(b, dst[i], 0, create_immed(b, fui((1 << bits) - 1)), 0, create_immed(b, fui(0.5f)), 0);
   /* convert the scaled value to integer */
   dst[i] = ir3_COV(b, dst[i], TYPE_F32, TYPE_U32);
   /* sign extend for signed values */
   if (type == TYPE_S32) {
      dst[i] = ir3_SHL_B(b, dst[i], 0, create_immed(b, 32 - bits), 0);
               } else if ((ctx->astc_srgb & (1 << tex->texture_index)) &&
      tex->op != nir_texop_tg4 && /* leave out tg4, unless it's on alpha? */
   !nir_tex_instr_is_query(tex)) {
            /* only need first 3 components: */
   sam->dsts[0]->wrmask = 0x7;
            /* we need to sample the alpha separately with a non-SRGB
   * texture state:
   */
   sam = ir3_SAM(b, opc, type, 0b1000, flags | info.flags, info.samp_tex,
                     /* fixup .w component: */
      } else {
      /* normal (non-workaround) case: */
               /* GETLOD returns results in 4.8 fixed point */
   if (opc == OPC_GETLOD) {
      bool half = tex->def.bit_size == 16;
   struct ir3_instruction *factor =
                  for (i = 0; i < 2; i++) {
      dst[i] = ir3_MUL_F(
      b, ir3_COV(b, dst[i], TYPE_S32, half ? TYPE_F16 : TYPE_F32), 0,
                  }
      static void
   emit_tex_info(struct ir3_context *ctx, nir_tex_instr *tex, unsigned idx)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction **dst, *sam;
   type_t dst_type = get_tex_dest_type(tex);
                              /* even though there is only one component, since it ends
   * up in .y/.z/.w rather than .x, we need a split_dest()
   */
            /* The # of levels comes from getinfo.z. We need to add 1 to it, since
   * the value in TEX_CONST_0 is zero-based.
   */
   if (ctx->compiler->levels_add_one)
               }
      static void
   emit_tex_txs(struct ir3_context *ctx, nir_tex_instr *tex)
   {
      struct ir3_block *b = ctx->block;
   struct ir3_instruction **dst, *sam;
   struct ir3_instruction *lod;
   unsigned flags, coords;
   type_t dst_type = get_tex_dest_type(tex);
            tex_info(tex, &flags, &coords);
            /* Actually we want the number of dimensions, not coordinates. This
   * distinction only matters for cubes.
   */
   if (tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE)
                     int lod_idx = nir_tex_instr_src_index(tex, nir_tex_src_lod);
                     if (tex->sampler_dim != GLSL_SAMPLER_DIM_BUF) {
         } else {
      /*
   * The maximum value which OPC_GETSIZE could return for one dimension
   * is 0x007ff0, however sampler buffer could be much bigger.
   * Blob uses OPC_GETBUF for them.
   */
                        /* Array size actually ends up in .w rather than .z. This doesn't
   * matter for miplevel 0, but for higher mips the value in z is
   * minified whereas w stays. Also, the value in TEX_CONST_3_DEPTH is
   * returned, which means that we have to add 1 to it for arrays.
   */
   if (tex->is_array) {
      if (ctx->compiler->levels_add_one) {
         } else {
                        }
      /* phi instructions are left partially constructed.  We don't resolve
   * their srcs until the end of the shader, since (eg. loops) one of
   * the phi's srcs might be defined after the phi due to back edges in
   * the CFG.
   */
   static void
   emit_phi(struct ir3_context *ctx, nir_phi_instr *nphi)
   {
               /* NOTE: phi's should be lowered to scalar at this point */
                     phi = ir3_instr_create(ctx->block, OPC_META_PHI, 1,
         __ssa_dst(phi);
                        }
      static struct ir3_block *get_block(struct ir3_context *ctx,
            static struct ir3_instruction *
   read_phi_src(struct ir3_context *ctx, struct ir3_block *blk,
         {
      if (!blk->nblock) {
      struct ir3_instruction *continue_phi =
                  for (unsigned i = 0; i < blk->predecessors_count; i++) {
      struct ir3_instruction *src =
         if (src)
         else
                           nir_foreach_phi_src (nsrc, nphi) {
      if (blk->nblock == nsrc->pred) {
      if (nsrc->src.ssa->parent_instr->type == nir_instr_type_undef) {
      /* Create an ir3 undef */
      } else {
                        unreachable("couldn't find phi node ir3 block");
      }
      static void
   resolve_phis(struct ir3_context *ctx, struct ir3_block *block)
   {
      foreach_instr (phi, &block->instr_list) {
      if (phi->opc != OPC_META_PHI)
                     if (!nphi) /* skip continue phis created above */
            for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
   struct ir3_instruction *src = read_phi_src(ctx, pred, phi, nphi);
   if (src) {
         } else {
      /* Create an ir3 undef */
               }
      static void
   emit_jump(struct ir3_context *ctx, nir_jump_instr *jump)
   {
      switch (jump->type) {
   case nir_jump_break:
   case nir_jump_continue:
   case nir_jump_return:
      /* I *think* we can simply just ignore this, and use the
   * successor block link to figure out where we need to
   * jump to for break/continue
   */
      default:
      ir3_context_error(ctx, "Unhandled NIR jump type: %d\n", jump->type);
         }
      static void
   emit_instr(struct ir3_context *ctx, nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_alu:
      emit_alu(ctx, nir_instr_as_alu(instr));
      case nir_instr_type_deref:
      /* ignored, handled as part of the intrinsic they are src to */
      case nir_instr_type_intrinsic:
      emit_intrinsic(ctx, nir_instr_as_intrinsic(instr));
      case nir_instr_type_load_const:
      emit_load_const(ctx, nir_instr_as_load_const(instr));
      case nir_instr_type_undef:
      emit_undef(ctx, nir_instr_as_undef(instr));
      case nir_instr_type_tex: {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
   /* couple tex instructions get special-cased:
   */
   switch (tex->op) {
   case nir_texop_txs:
      emit_tex_txs(ctx, tex);
      case nir_texop_query_levels:
      emit_tex_info(ctx, tex, 2);
      case nir_texop_texture_samples:
      emit_tex_info(ctx, tex, 3);
      default:
      emit_tex(ctx, tex);
      }
      }
   case nir_instr_type_jump:
      emit_jump(ctx, nir_instr_as_jump(instr));
      case nir_instr_type_phi:
      emit_phi(ctx, nir_instr_as_phi(instr));
      case nir_instr_type_call:
   case nir_instr_type_parallel_copy:
      ir3_context_error(ctx, "Unhandled NIR instruction type: %d\n",
               }
      static struct ir3_block *
   get_block(struct ir3_context *ctx, const nir_block *nblock)
   {
      struct ir3_block *block;
            hentry = _mesa_hash_table_search(ctx->block_ht, nblock);
   if (hentry)
            block = ir3_block_create(ctx->ir);
   block->nblock = nblock;
               }
      static struct ir3_block *
   get_block_or_continue(struct ir3_context *ctx, const nir_block *nblock)
   {
               hentry = _mesa_hash_table_search(ctx->continue_block_ht, nblock);
   if (hentry)
               }
      static struct ir3_block *
   create_continue_block(struct ir3_context *ctx, const nir_block *nblock)
   {
      struct ir3_block *block = ir3_block_create(ctx->ir);
   block->nblock = NULL;
   _mesa_hash_table_insert(ctx->continue_block_ht, nblock, block);
      }
      static void
   emit_block(struct ir3_context *ctx, nir_block *nblock)
   {
                        ctx->block->loop_id = ctx->loop_id;
            /* re-emit addr register in each block if needed: */
   for (int i = 0; i < ARRAY_SIZE(ctx->addr0_ht); i++) {
      _mesa_hash_table_destroy(ctx->addr0_ht[i], NULL);
               _mesa_hash_table_u64_destroy(ctx->addr1_ht);
            nir_foreach_instr (instr, nblock) {
      ctx->cur_instr = instr;
   emit_instr(ctx, instr);
   ctx->cur_instr = NULL;
   if (ctx->error)
               for (int i = 0; i < ARRAY_SIZE(ctx->block->successors); i++) {
      if (nblock->successors[i]) {
      ctx->block->successors[i] =
                           }
      static void emit_cf_list(struct ir3_context *ctx, struct exec_list *list);
      static void
   emit_if(struct ir3_context *ctx, nir_if *nif)
   {
               if (condition->opc == OPC_ANY_MACRO && condition->block == ctx->block) {
      ctx->block->condition = ssa(condition->srcs[0]);
      } else if (condition->opc == OPC_ALL_MACRO &&
            ctx->block->condition = ssa(condition->srcs[0]);
      } else if (condition->opc == OPC_ELECT_MACRO &&
            ctx->block->condition = NULL;
      } else if (condition->opc == OPC_SHPS_MACRO &&
            /* TODO: technically this only works if the block is the only user of the
   * shps, but we only use it in very constrained scenarios so this should
   * be ok.
   */
   ctx->block->condition = NULL;
      } else {
      ctx->block->condition = ir3_get_predicate(ctx, condition);
               emit_cf_list(ctx, &nif->then_list);
            struct ir3_block *last_then = get_block(ctx, nir_if_last_then_block(nif));
   struct ir3_block *first_else = get_block(ctx, nir_if_first_else_block(nif));
   assert(last_then->physical_successors[0] &&
                  struct ir3_block *last_else = get_block(ctx, nir_if_last_else_block(nif));
   struct ir3_block *after_if =
         assert(last_else->physical_successors[0] &&
         if (after_if != last_else->physical_successors[0])
      }
      static void
   emit_loop(struct ir3_context *ctx, nir_loop *nloop)
   {
      assert(!nir_loop_has_continue_construct(nloop));
   unsigned old_loop_id = ctx->loop_id;
   ctx->loop_id = ctx->so->loops + 1;
            struct nir_block *nstart = nir_loop_first_block(nloop);
            /* There's always one incoming edge from outside the loop, and if there
   * are more than two backedges from inside the loop (so more than 2 total
   * edges) then we need to create a continue block after the loop to ensure
   * that control reconverges at the end of each loop iteration.
   */
   if (nstart->predecessors->entries > 2) {
                           if (continue_blk) {
      struct ir3_block *start = get_block(ctx, nstart);
   continue_blk->successors[0] = start;
   continue_blk->physical_successors[0] = start;
   continue_blk->loop_id = ctx->loop_id;
   continue_blk->loop_depth = ctx->loop_depth;
               ctx->so->loops++;
   ctx->loop_depth--;
      }
      static void
   stack_push(struct ir3_context *ctx)
   {
      ctx->stack++;
      }
      static void
   stack_pop(struct ir3_context *ctx)
   {
      compile_assert(ctx, ctx->stack > 0);
      }
      static void
   emit_cf_list(struct ir3_context *ctx, struct exec_list *list)
   {
      foreach_list_typed (nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_block:
      emit_block(ctx, nir_cf_node_as_block(node));
      case nir_cf_node_if:
      stack_push(ctx);
   emit_if(ctx, nir_cf_node_as_if(node));
   stack_pop(ctx);
      case nir_cf_node_loop:
      stack_push(ctx);
   emit_loop(ctx, nir_cf_node_as_loop(node));
   stack_pop(ctx);
      case nir_cf_node_function:
      ir3_context_error(ctx, "TODO\n");
            }
      /* emit stream-out code.  At this point, the current block is the original
   * (nir) end block, and nir ensures that all flow control paths terminate
   * into the end block.  We re-purpose the original end block to generate
   * the 'if (vtxcnt < maxvtxcnt)' condition, then append the conditional
   * block holding stream-out write instructions, followed by the new end
   * block:
   *
   *   blockOrigEnd {
   *      p0.x = (vtxcnt < maxvtxcnt)
   *      // succs: blockStreamOut, blockNewEnd
   *   }
   *   blockStreamOut {
   *      // preds: blockOrigEnd
   *      ... stream-out instructions ...
   *      // succs: blockNewEnd
   *   }
   *   blockNewEnd {
   *      // preds: blockOrigEnd, blockStreamOut
   *   }
   */
   static void
   emit_stream_out(struct ir3_context *ctx)
   {
      struct ir3 *ir = ctx->ir;
   struct ir3_stream_output_info *strmout = &ctx->so->stream_output;
   struct ir3_block *orig_end_block, *stream_out_block, *new_end_block;
   struct ir3_instruction *vtxcnt, *maxvtxcnt, *cond;
            /* create vtxcnt input in input block at top of shader,
   * so that it is seen as live over the entire duration
   * of the shader:
   */
   vtxcnt = create_sysval_input(ctx, SYSTEM_VALUE_VERTEX_CNT, 0x1);
            /* at this point, we are at the original 'end' block,
   * re-purpose this block to stream-out condition, then
   * append stream-out block and new-end block
   */
            // maybe w/ store_global intrinsic, we could do this
            stream_out_block = ir3_block_create(ir);
            new_end_block = ir3_block_create(ir);
            orig_end_block->successors[0] = stream_out_block;
            orig_end_block->physical_successors[0] = stream_out_block;
                              /* setup 'if (vtxcnt < maxvtxcnt)' condition: */
   cond = ir3_CMPS_S(ctx->block, vtxcnt, 0, maxvtxcnt, 0);
   cond->dsts[0]->num = regid(REG_P0, 0);
   cond->dsts[0]->flags &= ~IR3_REG_SSA;
            /* condition goes on previous block to the conditional,
   * since it is used to pick which of the two successor
   * paths to take:
   */
            /* switch to stream_out_block to generate the stream-out
   * instructions:
   */
            /* Calculate base addresses based on vtxcnt.  Instructions
   * generated for bases not used in following loop will be
   * stripped out in the backend.
   */
   for (unsigned i = 0; i < IR3_MAX_SO_BUFFERS; i++) {
      const struct ir3_const_state *const_state = ir3_const_state(ctx->so);
   unsigned stride = strmout->stride[i];
                     /* 24-bit should be enough: */
   off = ir3_MUL_U24(ctx->block, vtxcnt, 0,
                        /* Generate the per-output store instructions: */
   for (unsigned i = 0; i < strmout->num_outputs; i++) {
      for (unsigned j = 0; j < strmout->output[i].num_components; j++) {
                                    stg = ir3_STG(
      ctx->block, base, 0,
   create_immed(ctx->block, (strmout->output[i].dst_offset + j) * 4),
                              /* and finally switch to the new_end_block: */
      }
      static void
   setup_predecessors(struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list) {
      for (int i = 0; i < ARRAY_SIZE(block->successors); i++) {
      if (block->successors[i])
         if (block->physical_successors[i])
      ir3_block_add_physical_predecessor(block->physical_successors[i],
         }
      static void
   emit_function(struct ir3_context *ctx, nir_function_impl *impl)
   {
                        emit_cf_list(ctx, &impl->body);
                     /* at this point, we should have a single empty block,
   * into which we emit the 'end' instruction.
   */
            /* If stream-out (aka transform-feedback) enabled, emit the
   * stream-out instructions, followed by a new empty block (into
   * which the 'end' instruction lands).
   *
   * NOTE: it is done in this order, rather than inserting before
   * we emit end_block, because NIR guarantees that all blocks
   * flow into end_block, and that end_block has no successors.
   * So by re-purposing end_block as the first block of stream-
   * out, we guarantee that all exit paths flow into the stream-
   * out instructions.
   */
   if ((ctx->compiler->gen < 5) &&
      (ctx->so->stream_output.num_outputs > 0) &&
   !ctx->so->binning_pass) {
   assert(ctx->so->type == MESA_SHADER_VERTEX);
               setup_predecessors(ctx->ir);
   foreach_block (block, &ctx->ir->block_list) {
            }
      static void
   setup_input(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_shader_variant *so = ctx->so;
            if (intr->intrinsic == nir_intrinsic_load_interpolated_input)
                     unsigned frac = nir_intrinsic_component(intr);
   unsigned offset = nir_src_as_uint(intr->src[coord ? 1 : 0]);
   unsigned ncomp = nir_intrinsic_dest_components(intr);
   unsigned n = nir_intrinsic_base(intr) + offset;
   unsigned slot = nir_intrinsic_io_semantics(intr).location + offset;
            /* Inputs are loaded using ldlw or ldg for other stages. */
   compile_assert(ctx, ctx->so->type == MESA_SHADER_FRAGMENT ||
            if (ctx->so->type == MESA_SHADER_FRAGMENT)
         else
            /* for a4xx+ rasterflat */
   if (so->inputs[n].rasterflat && ctx->so->key.rasterflat)
                     so->inputs[n].slot = slot;
   so->inputs[n].compmask |= compmask;
   so->inputs_count = MAX2(so->inputs_count, n + 1);
   compile_assert(ctx, so->inputs_count < ARRAY_SIZE(so->inputs));
            if (ctx->so->type == MESA_SHADER_FRAGMENT) {
                        for (int i = 0; i < ncomp; i++) {
      unsigned idx = (n * 4) + i + frac;
               if (slot == VARYING_SLOT_PRIMITIVE_ID)
      } else {
               foreach_input (in, ctx->ir) {
      if (in->input.inidx == n) {
      input = in;
                  if (!input) {
      input = create_input(ctx, compmask);
      } else {
      /* For aliased inputs, just append to the wrmask.. ie. if we
   * first see a vec2 index at slot N, and then later a vec4,
   * the wrmask of the resulting overlapped vec2 and vec4 is 0xf
   */
               for (int i = 0; i < ncomp + frac; i++) {
                     /* fixup the src wrmask to avoid validation fail */
   if (ctx->inputs[idx] && (ctx->inputs[idx] != input)) {
      ctx->inputs[idx]->srcs[0]->wrmask = input->dsts[0]->wrmask;
                           for (int i = 0; i < ncomp; i++) {
      unsigned idx = (n * 4) + i + frac;
            }
      /* Initially we assign non-packed inloc's for varyings, as we don't really
   * know up-front which components will be unused.  After all the compilation
   * stages we scan the shader to see which components are actually used, and
   * re-pack the inlocs to eliminate unneeded varyings.
   */
   static void
   pack_inlocs(struct ir3_context *ctx)
   {
      struct ir3_shader_variant *so = ctx->so;
                     /*
   * First Step: scan shader to find which bary.f/ldlv remain:
            foreach_block (block, &ctx->ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      if (is_input(instr)) {
      unsigned inloc = instr->srcs[0]->iim_val;
                                    } else if (instr->opc == OPC_META_TEX_PREFETCH) {
      for (int n = 0; n < 2; n++) {
                                                         /*
   * Second Step: reassign varying inloc/slots:
                     /* for clip+cull distances, unused components can't be eliminated because
   * they're read by fixed-function, even if there's a hole.  Note that
   * clip/cull distance arrays must be declared in the FS, so we can just
   * use the NIR clip/cull distances to avoid reading ucp_enables in the
   * shader key.
   */
            for (unsigned i = 0; i < so->inputs_count; i++) {
               so->inputs[i].inloc = inloc;
            if (so->inputs[i].slot == VARYING_SLOT_CLIP_DIST0 ||
      so->inputs[i].slot == VARYING_SLOT_CLIP_DIST1) {
   if (so->inputs[i].slot == VARYING_SLOT_CLIP_DIST0)
         else
                     for (unsigned j = 0; j < 4; j++) {
                                    /* at this point, since used_components[i] mask is only
   * considering varyings (ie. not sysvals) we know this
   * is a varying:
   */
               if (so->inputs[i].bary) {
      so->varying_in++;
   so->inputs[i].compmask = (1 << maxcomp) - 1;
                  /*
   * Third Step: reassign packed inloc's:
            foreach_block (block, &ctx->ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      if (is_input(instr)) {
      unsigned inloc = instr->srcs[0]->iim_val;
                  instr->srcs[0]->iim_val = so->inputs[i].inloc + j;
   if (instr->opc == OPC_FLAT_B)
      } else if (instr->opc == OPC_META_TEX_PREFETCH) {
      unsigned i = instr->prefetch.input_offset / 4;
   unsigned j = instr->prefetch.input_offset % 4;
               }
      static void
   setup_output(struct ir3_context *ctx, nir_intrinsic_instr *intr)
   {
      struct ir3_shader_variant *so = ctx->so;
                     unsigned offset = nir_src_as_uint(intr->src[1]);
   unsigned n = nir_intrinsic_base(intr) + offset;
   unsigned frac = nir_intrinsic_component(intr);
            /* For per-view variables, each user-facing slot corresponds to multiple
   * views, each with a corresponding driver_location, and the offset is for
   * the driver_location. To properly figure out of the slot, we'd need to
   * plumb through the number of views. However, for now we only use
   * per-view with gl_Position, so we assume that the variable is not an
   * array or matrix (so there are no indirect accesses to the variable
   * itself) and the indirect offset corresponds to the view.
   */
            if (io.per_view && offset > 0)
            if (ctx->so->type == MESA_SHADER_FRAGMENT) {
      switch (slot) {
   case FRAG_RESULT_DEPTH:
      so->writes_pos = true;
      case FRAG_RESULT_COLOR:
      if (!ctx->s->info.fs.color_is_dual_source) {
         } else {
      slot = FRAG_RESULT_DATA0 + io.dual_source_blend_index;
   if (io.dual_source_blend_index > 0)
      }
      case FRAG_RESULT_SAMPLE_MASK:
      so->writes_smask = true;
      case FRAG_RESULT_STENCIL:
      so->writes_stencilref = true;
      default:
      slot += io.dual_source_blend_index; /* For dual-src blend */
   if (io.dual_source_blend_index > 0)
         if (slot >= FRAG_RESULT_DATA0)
         ir3_context_error(ctx, "unknown FS output name: %s\n",
         } else if (ctx->so->type == MESA_SHADER_VERTEX ||
            ctx->so->type == MESA_SHADER_TESS_EVAL ||
   switch (slot) {
   case VARYING_SLOT_POS:
      so->writes_pos = true;
      case VARYING_SLOT_PSIZ:
      so->writes_psize = true;
      case VARYING_SLOT_VIEWPORT:
      so->writes_viewport = true;
      case VARYING_SLOT_PRIMITIVE_ID:
   case VARYING_SLOT_GS_VERTEX_FLAGS_IR3:
      assert(ctx->so->type == MESA_SHADER_GEOMETRY);
      case VARYING_SLOT_COL0:
   case VARYING_SLOT_COL1:
   case VARYING_SLOT_BFC0:
   case VARYING_SLOT_BFC1:
   case VARYING_SLOT_FOGC:
   case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1:
   case VARYING_SLOT_CLIP_VERTEX:
   case VARYING_SLOT_LAYER:
         default:
      if (slot >= VARYING_SLOT_VAR0)
         if ((VARYING_SLOT_TEX0 <= slot) && (slot <= VARYING_SLOT_TEX7))
         ir3_context_error(ctx, "unknown %s shader output name: %s\n",
               } else {
                  so->outputs_count = MAX2(so->outputs_count, n + 1);
            so->outputs[n].slot = slot;
   if (io.per_view)
            for (int i = 0; i < ncomp; i++) {
      unsigned idx = (n * 4) + i + frac;
   compile_assert(ctx, idx < ctx->noutputs);
               /* if varying packing doesn't happen, we could end up in a situation
   * with "holes" in the output, and since the per-generation code that
   * sets up varying linkage registers doesn't expect to have more than
   * one varying per vec4 slot, pad the holes.
   *
   * Note that this should probably generate a performance warning of
   * some sort.
   */
   for (int i = 0; i < frac; i++) {
      unsigned idx = (n * 4) + i;
   if (!ctx->outputs[idx]) {
                     struct ir3_instruction *const *src = ir3_get_src(ctx, &intr->src[0]);
   for (int i = 0; i < ncomp; i++) {
      unsigned idx = (n * 4) + i + frac;
         }
      static bool
   uses_load_input(struct ir3_shader_variant *so)
   {
         }
      static bool
   uses_store_output(struct ir3_shader_variant *so)
   {
      switch (so->type) {
   case MESA_SHADER_VERTEX:
         case MESA_SHADER_TESS_EVAL:
         case MESA_SHADER_GEOMETRY:
   case MESA_SHADER_FRAGMENT:
         case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_COMPUTE:
   case MESA_SHADER_KERNEL:
         default:
            }
      static void
   emit_instructions(struct ir3_context *ctx)
   {
                        /* some varying setup which can't be done in setup_input(): */
   if (ctx->so->type == MESA_SHADER_FRAGMENT) {
      nir_foreach_shader_in_variable (var, ctx->s) {
      /* set rasterflat flag for front/back color */
   if (var->data.interpolation == INTERP_MODE_NONE) {
      switch (var->data.location) {
   case VARYING_SLOT_COL0:
   case VARYING_SLOT_COL1:
   case VARYING_SLOT_BFC0:
   case VARYING_SLOT_BFC1:
      ctx->so->inputs[var->data.driver_location].rasterflat = true;
      default:
                           if (uses_load_input(ctx->so)) {
      ctx->so->inputs_count = ctx->s->num_inputs;
   compile_assert(ctx, ctx->so->inputs_count < ARRAY_SIZE(ctx->so->inputs));
   ctx->ninputs = ctx->s->num_inputs * 4;
      } else {
      ctx->ninputs = 0;
               if (uses_store_output(ctx->so)) {
      ctx->noutputs = ctx->s->num_outputs * 4;
   ctx->outputs =
      } else {
                           /* Create inputs in first block: */
   ctx->block = get_block(ctx, nir_start_block(fxn));
            /* for fragment shader, the vcoord input register is used as the
   * base for bary.f varying fetch instrs:
   *
   * TODO defer creating ctx->ij_pixel and corresponding sysvals
   * until emit_intrinsic when we know they are actually needed.
   * For now, we defer creating ctx->ij_centroid, etc, since we
   * only need ij_pixel for "old style" varying inputs (ie.
   * tgsi_to_nir)
   */
   if (ctx->so->type == MESA_SHADER_FRAGMENT) {
                  /* Defer add_sysval_input() stuff until after setup_inputs(),
   * because sysvals need to be appended after varyings:
   */
   if (ctx->ij[IJ_PERSP_PIXEL]) {
      add_sysval_input_compmask(ctx, SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL, 0x3,
               /* Tesselation shaders always need primitive ID for indexing the
   * BO. Geometry shaders don't always need it but when they do it has be
   * delivered and unclobbered in the VS. To make things easy, we always
   * make room for it in VS/DS.
   */
   bool has_tess = ctx->so->key.tessellation != IR3_TESS_NONE;
   bool has_gs = ctx->so->key.has_gs;
   switch (ctx->so->type) {
   case MESA_SHADER_VERTEX:
      if (has_tess) {
      ctx->tcs_header =
         ctx->rel_patch_id =
         ctx->primitive_id =
      } else if (has_gs) {
      ctx->gs_header =
         ctx->primitive_id =
      }
      case MESA_SHADER_TESS_CTRL:
      ctx->tcs_header =
         ctx->rel_patch_id =
            case MESA_SHADER_TESS_EVAL:
      if (has_gs) {
      ctx->gs_header =
         ctx->primitive_id =
      }
   ctx->rel_patch_id =
            case MESA_SHADER_GEOMETRY:
      ctx->gs_header =
            default:
                  /* Find # of samplers. Just assume that we'll be reading from images.. if
   * it is write-only we don't have to count it, but after lowering derefs
   * is too late to compact indices for that.
   */
   ctx->so->num_samp =
            /* Save off clip+cull information. Note that in OpenGL clip planes may
   * be individually enabled/disabled, and some gens handle lowering in
   * backend, so we also need to consider the shader key:
   */
   ctx->so->clip_mask = ctx->so->key.ucp_enables |
         ctx->so->cull_mask = MASK(ctx->s->info.cull_distance_array_size)
            ctx->so->pvtmem_size = ctx->s->scratch_size;
            /* NOTE: need to do something more clever when we support >1 fxn */
   nir_foreach_reg_decl (decl, fxn) {
                  if (ctx->so->type == MESA_SHADER_TESS_CTRL &&
      ctx->compiler->tess_use_shared) {
   struct ir3_instruction *barrier = ir3_BAR(ctx->block);
   barrier->flags = IR3_INSTR_SS | IR3_INSTR_SY;
   barrier->barrier_class = IR3_BARRIER_EVERYTHING;
   array_insert(ctx->block, ctx->block->keeps, barrier);
               /* And emit the body: */
   ctx->impl = fxn;
      }
      /* Fixup tex sampler state for astc/srgb workaround instructions.  We
   * need to assign the tex state indexes for these after we know the
   * max tex index.
   */
   static void
   fixup_astc_srgb(struct ir3_context *ctx)
   {
      struct ir3_shader_variant *so = ctx->so;
   /* indexed by original tex idx, value is newly assigned alpha sampler
   * state tex idx.  Zero is invalid since there is at least one sampler
   * if we get here.
   */
   unsigned alt_tex_state[16] = {0};
   unsigned tex_idx = ctx->max_texture_index + 1;
                     for (unsigned i = 0; i < ctx->ir->astc_srgb_count; i++) {
                        if (alt_tex_state[sam->cat5.tex] == 0) {
      /* assign new alternate/alpha tex state slot: */
   alt_tex_state[sam->cat5.tex] = tex_idx++;
   so->astc_srgb.orig_idx[idx++] = sam->cat5.tex;
                     }
      /* Fixup tex sampler state for tg4 workaround instructions.  We
   * need to assign the tex state indexes for these after we know the
   * max tex index.
   */
   static void
   fixup_tg4(struct ir3_context *ctx)
   {
      struct ir3_shader_variant *so = ctx->so;
   /* indexed by original tex idx, value is newly assigned alpha sampler
   * state tex idx.  Zero is invalid since there is at least one sampler
   * if we get here.
   */
   unsigned alt_tex_state[16] = {0};
   unsigned tex_idx = ctx->max_texture_index + so->astc_srgb.count + 1;
                     for (unsigned i = 0; i < ctx->ir->tg4_count; i++) {
                        if (alt_tex_state[sam->cat5.tex] == 0) {
      /* assign new alternate/alpha tex state slot: */
   alt_tex_state[sam->cat5.tex] = tex_idx++;
   so->tg4.orig_idx[idx++] = sam->cat5.tex;
                     }
      static bool
   output_slot_used_for_binning(gl_varying_slot slot)
   {
      return slot == VARYING_SLOT_POS || slot == VARYING_SLOT_PSIZ ||
            }
      static struct ir3_instruction *
   find_end(struct ir3 *ir)
   {
      foreach_block_rev (block, &ir->block_list) {
      foreach_instr_rev (instr, &block->instr_list) {
      if (instr->opc == OPC_END || instr->opc == OPC_CHMASK)
         }
      }
      static void
   fixup_binning_pass(struct ir3_context *ctx, struct ir3_instruction *end)
   {
      struct ir3_shader_variant *so = ctx->so;
            /* first pass, remove unused outputs from the IR level outputs: */
   for (i = 0, j = 0; i < end->srcs_count; i++) {
      unsigned outidx = end->end.outidxs[i];
            if (output_slot_used_for_binning(slot)) {
      end->srcs[j] = end->srcs[i];
   end->end.outidxs[j] = end->end.outidxs[i];
         }
            /* second pass, cleanup the unused slots in ir3_shader_variant::outputs
   * table:
   */
   for (i = 0, j = 0; i < so->outputs_count; i++) {
               if (output_slot_used_for_binning(slot)) {
               /* fixup outidx to point to new output table entry: */
   for (unsigned k = 0; k < end->srcs_count; k++) {
      if (end->end.outidxs[k] == i) {
      end->end.outidxs[k] = j;
                        }
      }
      static void
   collect_tex_prefetches(struct ir3_context *ctx, struct ir3 *ir)
   {
               /* Collect sampling instructions eligible for pre-dispatch. */
   foreach_block (block, &ir->block_list) {
      foreach_instr_safe (instr, &block->instr_list) {
      if (instr->opc == OPC_META_TEX_PREFETCH) {
      assert(idx < ARRAY_SIZE(ctx->so->sampler_prefetch));
   struct ir3_sampler_prefetch *fetch =
                  fetch->bindless = instr->flags & IR3_INSTR_B;
   if (fetch->bindless) {
      /* In bindless mode, the index is actually the base */
   fetch->tex_id = instr->prefetch.tex_base;
   fetch->samp_id = instr->prefetch.samp_base;
   fetch->tex_bindless_id = instr->prefetch.tex;
      } else {
      fetch->tex_id = instr->prefetch.tex;
      }
   fetch->tex_opc = OPC_SAM;
   fetch->wrmask = instr->dsts[0]->wrmask;
                  /* These are the limits on a5xx/a6xx, we might need to
   * revisit if SP_FS_PREFETCH[n] changes on later gens:
   */
   assert(fetch->dst <= 0x3f);
                                          /* Remove the prefetch placeholder instruction: */
               }
      int
   ir3_compile_shader_nir(struct ir3_compiler *compiler,
               {
      struct ir3_context *ctx;
   struct ir3 *ir;
   int ret = 0, max_bary;
                              ctx = ir3_context_init(compiler, shader, so);
   if (!ctx) {
      DBG("INIT failed!");
   ret = -1;
                        if (ctx->error) {
      DBG("EMIT failed!");
   ret = -1;
                        if (gl_shader_stage_is_compute(so->type)) {
      so->local_size[0] = ctx->s->info.workgroup_size[0];
   so->local_size[1] = ctx->s->info.workgroup_size[1];
   so->local_size[2] = ctx->s->info.workgroup_size[2];
               /* Vertex shaders in a tessellation or geometry pipeline treat END as a
   * NOP and has an epilogue that writes the VS outputs to local storage, to
   * be read by the HS.  Then it resets execution mask (chmask) and chains
   * to the next shader (chsh). There are also a few output values which we
   * must send to the next stage via registers, and in order for both stages
   * to agree on the register used we must force these to be in specific
   * registers.
   */
   if ((so->type == MESA_SHADER_VERTEX &&
      (so->key.has_gs || so->key.tessellation)) ||
   (so->type == MESA_SHADER_TESS_EVAL && so->key.has_gs)) {
   struct ir3_instruction *outputs[3];
   unsigned outidxs[3];
   unsigned regids[3];
            if (ctx->primitive_id) {
                     struct ir3_instruction *out = ir3_collect(ctx->block, ctx->primitive_id);
   outputs[outputs_count] = out;
   outidxs[outputs_count] = n;
   if (so->type == MESA_SHADER_VERTEX && ctx->rel_patch_id)
         else
                     if (so->type == MESA_SHADER_VERTEX && ctx->rel_patch_id) {
      unsigned n = so->outputs_count++;
   so->outputs[n].slot = VARYING_SLOT_REL_PATCH_ID_IR3;
   struct ir3_instruction *out = ir3_collect(ctx->block, ctx->rel_patch_id);
   outputs[outputs_count] = out;
   outidxs[outputs_count] = n;
   regids[outputs_count] = regid(0, 1);
               if (ctx->gs_header) {
      unsigned n = so->outputs_count++;
   so->outputs[n].slot = VARYING_SLOT_GS_HEADER_IR3;
   struct ir3_instruction *out = ir3_collect(ctx->block, ctx->gs_header);
   outputs[outputs_count] = out;
   outidxs[outputs_count] = n;
   regids[outputs_count] = regid(0, 0);
               if (ctx->tcs_header) {
      unsigned n = so->outputs_count++;
   so->outputs[n].slot = VARYING_SLOT_TCS_HEADER_IR3;
   struct ir3_instruction *out = ir3_collect(ctx->block, ctx->tcs_header);
   outputs[outputs_count] = out;
   outidxs[outputs_count] = n;
   regids[outputs_count] = regid(0, 0);
               struct ir3_instruction *chmask =
         chmask->barrier_class = IR3_BARRIER_EVERYTHING;
            for (unsigned i = 0; i < outputs_count; i++)
            chmask->end.outidxs = ralloc_array(chmask, unsigned, outputs_count);
                     struct ir3_instruction *chsh = ir3_CHSH(ctx->block);
   chsh->barrier_class = IR3_BARRIER_EVERYTHING;
      } else {
      assert((ctx->noutputs % 4) == 0);
   unsigned outidxs[ctx->noutputs / 4];
   struct ir3_instruction *outputs[ctx->noutputs / 4];
            struct ir3_block *b = ctx->block;
   /* Insert these collect's in the block before the end-block if
   * possible, so that any moves they generate can be shuffled around to
   * reduce nop's:
   */
   if (ctx->block->predecessors_count == 1)
            /* Setup IR level outputs, which are "collects" that gather
   * the scalar components of outputs.
   */
   for (unsigned i = 0; i < ctx->noutputs; i += 4) {
      unsigned ncomp = 0;
   /* figure out the # of components written:
   *
   * TODO do we need to handle holes, ie. if .x and .z
   * components written, but .y component not written?
   */
   for (unsigned j = 0; j < 4; j++) {
      if (!ctx->outputs[i + j])
                     /* Note that in some stages, like TCS, store_output is
   * lowered to memory writes, so no components of the
   * are "written" from the PoV of traditional store-
   * output instructions:
   */
                                                outidxs[outputs_count] = outidx;
   outputs[outputs_count] = out;
               /* for a6xx+, binning and draw pass VS use same VBO state, so we
   * need to make sure not to remove any inputs that are used by
   * the nonbinning VS.
   */
   if (ctx->compiler->gen >= 6 && so->binning_pass &&
      so->type == MESA_SHADER_VERTEX) {
                                                                           /* be sure to keep inputs, even if only used in VS */
   if (so->nonbinning->inputs[n].compmask & (1 << c))
                  struct ir3_instruction *end =
            for (unsigned i = 0; i < outputs_count; i++) {
                  end->end.outidxs = ralloc_array(end, unsigned, outputs_count);
                     /* at this point, for binning pass, throw away unneeded outputs: */
   if (so->binning_pass && (ctx->compiler->gen < 6))
               if (so->type == MESA_SHADER_FRAGMENT &&
      ctx->s->info.fs.needs_quad_helper_invocations) {
   so->need_pixlod = true;
               ir3_debug_print(ir, "AFTER: nir->ir3");
                              do {
               /* the folding doesn't seem to work reliably on a4xx */
   if (ctx->compiler->gen != 4)
         progress |= IR3_PASS(ir, ir3_cp, so);
   progress |= IR3_PASS(ir, ir3_cse);
               /* at this point, for binning pass, throw away unneeded outputs:
   * Note that for a6xx and later, we do this after ir3_cp to ensure
   * that the uniform/constant layout for BS and VS matches, so that
   * we can re-use same VS_CONST state group.
   */
   if (so->binning_pass && (ctx->compiler->gen >= 6)) {
      fixup_binning_pass(ctx, find_end(ctx->so->ir));
   /* cleanup the result of removing unneeded outputs: */
   while (IR3_PASS(ir, ir3_dce, so)) {
                        /* At this point, all the dead code should be long gone: */
            ret = ir3_sched(ir);
   if (ret) {
      DBG("SCHED failed!");
                        /* Pre-assign VS inputs on a6xx+ binning pass shader, to align
   * with draw pass VS, so binning and draw pass can both use the
   * same VBO state.
   *
   * Note that VS inputs are expected to be full precision.
   */
   bool pre_assign_inputs = (ir->compiler->gen >= 6) &&
                  if (pre_assign_inputs) {
      foreach_input (in, ir) {
                           } else if (ctx->tcs_header) {
      /* We need to have these values in the same registers between VS and TCS
   * since the VS chains to TCS and doesn't get the sysvals redelivered.
            ctx->tcs_header->dsts[0]->num = regid(0, 0);
   ctx->rel_patch_id->dsts[0]->num = regid(0, 1);
   if (ctx->primitive_id)
      } else if (ctx->gs_header) {
      /* We need to have these values in the same registers between producer
   * (VS or DS) and GS since the producer chains to GS and doesn't get
   * the sysvals redelivered.
            ctx->gs_header->dsts[0]->num = regid(0, 0);
   if (ctx->primitive_id)
      } else if (so->num_sampler_prefetch) {
      assert(so->type == MESA_SHADER_FRAGMENT);
            foreach_input (instr, ir) {
                     assert(idx < 2);
   instr->dsts[0]->num = idx;
                           if (ret) {
      mesa_loge("ir3_ra() failed!");
                        IR3_PASS(ir, ir3_legalize_relative);
            if (so->type == MESA_SHADER_FRAGMENT)
            /*
   * Fixup inputs/outputs to point to the actual registers assigned:
   *
   * 1) initialize to r63.x (invalid/unused)
   * 2) iterate IR level inputs/outputs and update the variants
   *    inputs/outputs table based on the assigned registers for
   *    the remaining inputs/outputs.
            for (unsigned i = 0; i < so->inputs_count; i++)
         for (unsigned i = 0; i < so->outputs_count; i++)
                     for (unsigned i = 0; i < end->srcs_count; i++) {
      unsigned outidx = end->end.outidxs[i];
            so->outputs[outidx].regid = reg->num;
               foreach_input (in, ir) {
      assert(in->opc == OPC_META_INPUT);
            if (pre_assign_inputs && !so->inputs[inidx].sysval) {
      if (VALIDREG(so->nonbinning->inputs[inidx].regid)) {
      compile_assert(
         compile_assert(ctx, !!(in->dsts[0]->flags & IR3_REG_HALF) ==
      }
   so->inputs[inidx].regid = so->nonbinning->inputs[inidx].regid;
      } else {
      so->inputs[inidx].regid = in->dsts[0]->num;
                  if (ctx->astc_srgb)
            if (ctx->compiler->gen == 4 && ctx->s->info.uses_texture_gather)
            /* We need to do legalize after (for frag shader's) the "bary.f"
   * offsets (inloc) have been assigned.
   */
            /* Set (ss)(sy) on first TCS and GEOMETRY instructions, since we don't
   * know what we might have to wait on when coming in from VS chsh.
   */
   if (so->type == MESA_SHADER_TESS_CTRL || so->type == MESA_SHADER_GEOMETRY) {
      foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      instr->flags |= IR3_INSTR_SS | IR3_INSTR_SY;
                     if (ctx->compiler->gen >= 7 && so->type == MESA_SHADER_COMPUTE) {
      struct ir3_instruction *end = find_end(so->ir);
   struct ir3_instruction *lock =
         /* TODO: This flags should be set by scheduler only when needed */
   lock->flags = IR3_INSTR_SS | IR3_INSTR_SY | IR3_INSTR_JP;
   ir3_instr_move_before(lock, end);
   struct ir3_instruction *unlock =
                                       /* Note that max_bary counts inputs that are not bary.f'd for FS: */
   if (so->type == MESA_SHADER_FRAGMENT)
            /* Collect sampling instructions eligible for pre-dispatch. */
            if ((ctx->so->type == MESA_SHADER_FRAGMENT) &&
      !ctx->s->info.fs.early_fragment_tests)
         if ((ctx->so->type == MESA_SHADER_FRAGMENT) &&
      ctx->s->info.fs.post_depth_coverage)
               out:
      if (ret) {
      if (so->ir)
            }
               }
