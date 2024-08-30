   /*
   * Copyright (C) 2021 Collabora, Ltd.
   * Copyright (C) 2021 Alyssa Rosenzweig <alyssa@rosenzweig.io>
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
   */
      #include "bi_builder.h"
   #include "compiler.h"
      /*
   * Due to a Bifrost encoding restriction, some instructions cannot have an abs
   * modifier on both sources. Check if adding a fabs modifier to a given source
   * of a binary instruction would cause this restriction to be hit.
   */
   static bool
   bi_would_impact_abs(unsigned arch, bi_instr *I, bi_index repl, unsigned s)
   {
      return (arch <= 8) && I->src[1 - s].abs &&
      }
      static bool
   bi_takes_fabs(unsigned arch, bi_instr *I, bi_index repl, unsigned s)
   {
      switch (I->op) {
   case BI_OPCODE_FCMP_V2F16:
   case BI_OPCODE_FMAX_V2F16:
   case BI_OPCODE_FMIN_V2F16:
         case BI_OPCODE_FADD_V2F16:
      /*
   * For FADD.v2f16, the FMA pipe has the abs encoding hazard,
   * while the FADD pipe cannot encode a clamp. Either case in
   * isolation can be worked around in the scheduler, but both
   * together is impossible to encode. Avoid the hazard.
   */
      case BI_OPCODE_V2F32_TO_V2F16:
      /* TODO: Needs both match or lower */
      case BI_OPCODE_FLOG_TABLE_F32:
      /* TODO: Need to check mode */
      default:
            }
      static bool
   bi_takes_fneg(unsigned arch, bi_instr *I, unsigned s)
   {
      switch (I->op) {
   case BI_OPCODE_CUBE_SSEL:
   case BI_OPCODE_CUBE_TSEL:
   case BI_OPCODE_CUBEFACE:
      /* TODO: Bifrost encoding restriction: need to match or lower */
      case BI_OPCODE_FREXPE_F32:
   case BI_OPCODE_FREXPE_V2F16:
   case BI_OPCODE_FLOG_TABLE_F32:
      /* TODO: Need to check mode */
      default:
            }
      static bool
   bi_is_fabsneg(enum bi_opcode op, enum bi_size size)
   {
      return (size == BI_SIZE_32 && op == BI_OPCODE_FABSNEG_F32) ||
      }
      static enum bi_swizzle
   bi_compose_swizzle_16(enum bi_swizzle a, enum bi_swizzle b)
   {
      assert(a <= BI_SWIZZLE_H11);
            bool al = (a & BI_SWIZZLE_H10);
   bool ar = (a & BI_SWIZZLE_H01);
   bool bl = (b & BI_SWIZZLE_H10);
            return ((al ? br : bl) ? BI_SWIZZLE_H10 : 0) |
      }
      /* Like bi_replace_index, but composes instead of overwrites */
      static inline bi_index
   bi_compose_float_index(bi_index old, bi_index repl)
   {
      /* abs(-x) = abs(+x) so ignore repl.neg if old.abs is set, otherwise
   * -(-x) = x but -(+x) = +(-x) so need to exclusive-or the negates */
            /* +/- abs(+/- abs(x)) = +/- abs(x), etc so just or the two */
            /* Use the old swizzle to select from the replacement swizzle */
               }
      /* DISCARD.b32(FCMP.f(x, y)) --> DISCARD.f(x, y) */
      static inline bool
   bi_fuse_discard_fcmp(bi_context *ctx, bi_instr *I, bi_instr *mod)
   {
      if (!mod)
         if (I->op != BI_OPCODE_DISCARD_B32)
         if (mod->op != BI_OPCODE_FCMP_F32 && mod->op != BI_OPCODE_FCMP_V2F16)
         if (mod->cmpf >= BI_CMPF_GTLT)
                     /* .abs and .neg modifiers allowed on Valhall DISCARD but not Bifrost */
   bool absneg = mod->src[0].neg || mod->src[0].abs;
            if (ctx->arch <= 8 && absneg)
                     bi_builder b = bi_init_builder(ctx, bi_before_instr(I));
            if (mod->op == BI_OPCODE_FCMP_V2F16) {
      I->src[0].swizzle = bi_compose_swizzle_16(r, I->src[0].swizzle);
                  }
      /*
   * S32_TO_F32(S8_TO_S32(x)) -> S8_TO_F32 and friends. Round modes don't matter
   * because all 8-bit and 16-bit integers may be represented exactly as fp32.
   */
   struct {
      enum bi_opcode inner;
   enum bi_opcode outer;
      } bi_small_int_patterns[] = {
      {BI_OPCODE_S8_TO_S32, BI_OPCODE_S32_TO_F32, BI_OPCODE_S8_TO_F32},
   {BI_OPCODE_U8_TO_U32, BI_OPCODE_U32_TO_F32, BI_OPCODE_U8_TO_F32},
   {BI_OPCODE_U8_TO_U32, BI_OPCODE_S32_TO_F32, BI_OPCODE_U8_TO_F32},
   {BI_OPCODE_S16_TO_S32, BI_OPCODE_S32_TO_F32, BI_OPCODE_S16_TO_F32},
   {BI_OPCODE_U16_TO_U32, BI_OPCODE_U32_TO_F32, BI_OPCODE_U16_TO_F32},
      };
      static inline void
   bi_fuse_small_int_to_f32(bi_instr *I, bi_instr *mod)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(bi_small_int_patterns); ++i) {
      if (I->op != bi_small_int_patterns[i].outer)
         if (mod->op != bi_small_int_patterns[i].inner)
            assert(I->src[0].swizzle == BI_SWIZZLE_H01);
   I->src[0] = mod->src[0];
   I->round = BI_ROUND_NONE;
         }
      void
   bi_opt_mod_prop_forward(bi_context *ctx)
   {
               bi_foreach_instr_global_safe(ctx, I) {
      /* Try fusing FCMP into DISCARD.b32, building a new DISCARD.f32
   * instruction. As this is the only optimization DISCARD is
   * involved in, this shortcircuits other processing.
   */
   if (I->op == BI_OPCODE_DISCARD_B32) {
      if (bi_is_ssa(I->src[0]) &&
      bi_fuse_discard_fcmp(ctx, I, lut[I->src[0].value])) {
                           bi_foreach_dest(I, d) {
                  bi_foreach_ssa_src(I, s) {
                                                if (bi_is_fabsneg(mod->op, size)) {
                                                         }
      /* RSCALE has restrictions on how the clamp may be used, only used for
   * specialized transcendental sequences that set the clamp explicitly anyway */
      static bool
   bi_takes_clamp(bi_instr *I)
   {
      switch (I->op) {
   case BI_OPCODE_FMA_RSCALE_F32:
   case BI_OPCODE_FMA_RSCALE_V2F16:
   case BI_OPCODE_FADD_RSCALE_F32:
         case BI_OPCODE_FADD_V2F16:
      /* Encoding restriction */
   return !(I->src[0].abs && I->src[1].abs &&
      default:
            }
      static bool
   bi_is_fclamp(enum bi_opcode op, enum bi_size size)
   {
      return (size == BI_SIZE_32 && op == BI_OPCODE_FCLAMP_F32) ||
      }
      static bool
   bi_optimizer_clamp(bi_instr *I, bi_instr *use)
   {
      if (!bi_is_fclamp(use->op, bi_opcode_props[I->op].size))
         if (!bi_takes_clamp(I))
            /* Clamps are bitfields (clamp_m1_1/clamp_0_inf) so composition is OR */
   I->clamp |= use->clamp;
   I->dest[0] = use->dest[0];
      }
      static enum bi_opcode
   bi_sized_mux_op(unsigned size)
   {
      switch (size) {
   case 8:
         case 16:
         case 32:
         default:
            }
      static bool
   bi_is_fixed_mux(bi_instr *I, unsigned size, bi_index v1)
   {
      return I->op == bi_sized_mux_op(size) &&
            }
      static bool
   bi_takes_int_result_type(enum bi_opcode op)
   {
      switch (op) {
   case BI_OPCODE_ICMP_I32:
   case BI_OPCODE_ICMP_S32:
   case BI_OPCODE_ICMP_U32:
   case BI_OPCODE_ICMP_V2I16:
   case BI_OPCODE_ICMP_V2S16:
   case BI_OPCODE_ICMP_V2U16:
   case BI_OPCODE_ICMP_V4I8:
   case BI_OPCODE_ICMP_V4S8:
   case BI_OPCODE_ICMP_V4U8:
   case BI_OPCODE_FCMP_F32:
   case BI_OPCODE_FCMP_V2F16:
         default:
            }
      static bool
   bi_takes_float_result_type(enum bi_opcode op)
   {
         }
      /* CMP+MUX -> CMP with result type */
   static bool
   bi_optimizer_result_type(bi_instr *I, bi_instr *mux)
   {
      if (bi_opcode_props[I->op].size != bi_opcode_props[mux->op].size)
            if (bi_is_fixed_mux(mux, 32, bi_imm_f32(1.0)) ||
               if (!bi_takes_float_result_type(I->op))
               } else if (bi_is_fixed_mux(mux, 32, bi_imm_u32(1)) ||
                     if (!bi_takes_int_result_type(I->op))
               } else {
                  I->dest[0] = mux->dest[0];
      }
      static bool
   bi_is_var_tex(bi_instr *var, bi_instr *tex)
   {
      return (var->op == BI_OPCODE_LD_VAR_IMM) &&
         (tex->op == BI_OPCODE_TEXS_2D_F16 ||
   tex->op == BI_OPCODE_TEXS_2D_F32) &&
   (var->register_format == BI_REGISTER_FORMAT_F32) &&
   ((var->sample == BI_SAMPLE_CENTER &&
         (var->sample == BI_SAMPLE_NONE &&
            }
      static bool
   bi_optimizer_var_tex(bi_context *ctx, bi_instr *var, bi_instr *tex)
   {
      if (!bi_is_var_tex(var, tex))
            /* Construct the corresponding VAR_TEX intruction */
            bi_instr *I = bi_var_tex_f32_to(&b, tex->dest[0], tex->lod_mode, var->sample,
                  if (tex->op == BI_OPCODE_TEXS_2D_F16)
            /* Dead code elimination will clean up for us */
      }
      void
   bi_opt_mod_prop_backward(bi_context *ctx)
   {
      unsigned count = ctx->ssa_alloc;
   bi_instr **uses = calloc(count, sizeof(*uses));
            bi_foreach_instr_global_rev(ctx, I) {
      bi_foreach_ssa_src(I, s) {
               if (uses[v] && uses[v] != I)
         else
               if (!I->nr_dests)
                     if (!use || BITSET_TEST(multiple, I->dest[0].value))
            /* Destination has a single use, try to propagate */
   bool propagated =
            if (!propagated && I->op == BI_OPCODE_LD_VAR_IMM &&
      use->op == BI_OPCODE_SPLIT_I32) {
   /* Need to see through the split in a
   * ld_var_imm/split/var_tex  sequence
                                 use = tex;
               if (propagated) {
      bi_remove_instruction(use);
                  free(uses);
      }
      /*
   * Lower pseudo instructions that exist to simplify the optimizer. Returns the
   * replacement instruction, or NULL if no replacement is needed.
   */
   static bool
   bi_lower_opt_instruction_helper(bi_builder *b, bi_instr *I)
   {
               switch (I->op) {
   case BI_OPCODE_FABSNEG_F32:
   case BI_OPCODE_FCLAMP_F32:
      repl = bi_fadd_f32_to(b, I->dest[0], I->src[0], bi_negzero());
   repl->clamp = I->clamp;
         case BI_OPCODE_FABSNEG_V2F16:
   case BI_OPCODE_FCLAMP_V2F16:
      repl = bi_fadd_v2f16_to(b, I->dest[0], I->src[0], bi_negzero());
   repl->clamp = I->clamp;
         case BI_OPCODE_DISCARD_B32:
      bi_discard_f32(b, I->src[0], bi_zero(), BI_CMPF_NE);
         default:
            }
      void
   bi_lower_opt_instructions(bi_context *ctx)
   {
      bi_foreach_instr_global_safe(ctx, I) {
               if (bi_lower_opt_instruction_helper(&b, I))
         }
