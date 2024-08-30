   /*
   * Copyright 2023 Valve Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "compiler.h"
   #include "midgard.h"
   #include "midgard_ops.h"
   #include "nir.h"
      static bool
   is_inot(midgard_instruction *I)
   {
      return I->type == TAG_ALU_4 && I->op == midgard_alu_op_inor &&
      }
      static bool
   try_fold_fmov_src(midgard_instruction *use, unsigned src_idx,
         {
      if (use->type != TAG_ALU_4)
         if (fmov->has_constants || fmov->has_inline_constant)
         if (mir_nontrivial_outmod(fmov))
            /* Don't propagate into non-float instructions */
   if (nir_alu_type_get_base_type(use->src_types[src_idx]) != nir_type_float)
            /* TODO: Size conversions not handled yet */
   if (use->src_types[src_idx] != fmov->src_types[1])
            if (use->src_abs[src_idx]) {
         } else {
      /* -(-(abs(x))) = abs(x) */
   use->src_abs[src_idx] = fmov->src_abs[1];
               use->src[src_idx] = fmov->src[1];
   mir_compose_swizzle(use->swizzle[src_idx], fmov->swizzle[1],
            }
      static bool
   try_fold_inot(midgard_instruction *use, unsigned src_idx,
         {
      /* TODO: Size conversions not handled yet */
   if (nir_alu_type_get_type_size(use->src_types[src_idx]) !=
      nir_alu_type_get_type_size(inot->src_types[0]))
         if (use->compact_branch) {
         } else if (use->type == TAG_ALU_4) {
      switch (use->op) {
   case midgard_alu_op_iand:
   case midgard_alu_op_ior:
   case midgard_alu_op_ixor:
         default:
                  use->src_invert[src_idx] ^= true;
   mir_compose_swizzle(use->swizzle[src_idx], inot->swizzle[0],
      } else {
                  use->src[src_idx] = inot->src[0];
      }
      static bool
   midgard_opt_prop_forward(compiler_context *ctx)
   {
               midgard_instruction **defs =
            mir_foreach_block(ctx, block_) {
               mir_foreach_instr_in_block(block, I) {
      /* Record SSA defs */
   if (mir_is_ssa(I->dest)) {
      assert(I->dest < ctx->temp_count);
               mir_foreach_src(I, s) {
      unsigned src = I->src[s];
                  /* Try to fold a source mod in */
   assert(src < ctx->temp_count);
   midgard_instruction *def = defs[src];
                  if (def->type == TAG_ALU_4 && def->op == midgard_alu_op_fmov) {
         } else if (is_inot(def)) {
                           free(defs);
      }
      enum outmod_state {
      outmod_unknown = 0,
   outmod_clamp_0_1,
   outmod_clamp_m1_1,
   outmod_clamp_0_inf,
      };
      static enum outmod_state
   outmod_to_state(unsigned outmod)
   {
      switch (outmod) {
   case midgard_outmod_clamp_0_1:
         case midgard_outmod_clamp_m1_1:
         case midgard_outmod_clamp_0_inf:
         default:
            }
      static enum outmod_state
   union_outmod_state(enum outmod_state a, enum outmod_state b)
   {
      if (a == outmod_unknown)
         else if (b == outmod_unknown)
         else if (a == b)
         else
      }
      static bool
   midgard_opt_prop_backward(compiler_context *ctx)
   {
      bool progress = false;
   enum outmod_state *state = calloc(ctx->temp_count, sizeof(*state));
            /* Scan for outmod states */
   mir_foreach_instr_global(ctx, I) {
      if (I->type == TAG_ALU_4 && I->op == midgard_alu_op_fmov &&
               enum outmod_state outmod = outmod_to_state(I->outmod);
      } else {
      /* Anything used as any other source cannot have an outmod folded in */
   mir_foreach_src(I, s) {
      if (mir_is_ssa(I->src[s]))
                     /* Apply outmods */
   mir_foreach_instr_global(ctx, I) {
      if (!mir_is_ssa(I->dest))
            if (I->type != TAG_ALU_4 && I->type != TAG_TEXTURE_4)
            if (nir_alu_type_get_base_type(I->dest_type) != nir_type_float)
            if (I->outmod != midgard_outmod_none)
            switch (state[I->dest]) {
   case outmod_clamp_0_1:
      I->outmod = midgard_outmod_clamp_0_1;
      case outmod_clamp_m1_1:
      I->outmod = midgard_outmod_clamp_m1_1;
      case outmod_clamp_0_inf:
      I->outmod = midgard_outmod_clamp_0_inf;
      default:
                  if (I->outmod != midgard_outmod_none) {
                     /* Strip outmods from FMOVs to let copyprop go ahead */
   mir_foreach_instr_global(ctx, I) {
      if (I->type == TAG_ALU_4 && I->op == midgard_alu_op_fmov &&
                              free(state);
   free(folded);
      }
      bool
   midgard_opt_prop(compiler_context *ctx)
   {
      bool progress = false;
   mir_compute_temp_count(ctx);
   progress |= midgard_opt_prop_forward(ctx);
   progress |= midgard_opt_prop_backward(ctx);
      }
