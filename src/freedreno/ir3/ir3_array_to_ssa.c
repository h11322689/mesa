   /*
   * Copyright (C) 2021 Valve Corporation
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
      /* This pass lowers array accesses to SSA.
   *
   * After this pass, instructions writing arrays implicitly read the contents of
   * the array defined in instr->dsts[0]->def (possibly a phi node), perform the
   * operation, and store to instr->dsts[0].
   *
   * This makes arrays appear like "normal" SSA values, even if the false
   * dependencies mean that they always stay in CSSA form (i.e. able to removed
   * out-of-SSA with no copies.) While hopefully they shouldn't induce copies in
   * most cases, we can't make that guarantee while also splitting spilling from
   * RA and guaranteeing a certain number of registers are used, so we have to
   * insert the phi nodes to be able to know when copying should happen.
   *
   * The implementation is based on the idea in "Simple and Efficient Construction
   * of Static Single Assignment Form" of scanning backwards to find the
   * definition. However, since we're not doing this on-the-fly we can simplify
   * things a little by doing a pre-pass to get the last definition of each array
   * in each block. Then we optimize trivial phis in a separate pass, "on the fly"
   * so that we don't have to rewrite (and keep track of) users.
   */
      #include <stdlib.h>
   #include "ir3.h"
      struct array_state {
      struct ir3_register *live_in_definition;
   struct ir3_register *live_out_definition;
   bool constructed;
      };
      struct array_ctx {
      struct array_state *states;
   struct ir3 *ir;
      };
      static struct array_state *
   get_state(struct array_ctx *ctx, struct ir3_block *block, unsigned id)
   {
         }
      static struct ir3_register *read_value_beginning(struct array_ctx *ctx,
                  static struct ir3_register *
   read_value_end(struct array_ctx *ctx, struct ir3_block *block,
         {
      struct array_state *state = get_state(ctx, block, arr->id);
   if (state->live_out_definition)
            state->live_out_definition = read_value_beginning(ctx, block, arr);
      }
      /* Roughly equivalent to readValueRecursive from the paper: */
   static struct ir3_register *
   read_value_beginning(struct array_ctx *ctx, struct ir3_block *block,
         {
               if (state->constructed)
            if (block->predecessors_count == 0) {
      state->constructed = true;
               if (block->predecessors_count == 1) {
      state->live_in_definition =
         state->constructed = true;
               unsigned flags = IR3_REG_ARRAY | (arr->half ? IR3_REG_HALF : 0);
   struct ir3_instruction *phi =
         list_del(&phi->node);
            struct ir3_register *dst = __ssa_dst(phi);
   dst->flags |= flags;
   dst->array.id = arr->id;
            state->live_in_definition = phi->dsts[0];
            for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_register *src =
         struct ir3_register *src_reg;
   if (src) {
         } else {
         }
   src_reg->array.id = arr->id;
      }
      }
      static struct ir3_register *
   remove_trivial_phi(struct ir3_instruction *phi)
   {
      /* Break cycles */
   if (phi->data)
                     struct ir3_register *unique_def = NULL;
   bool unique = true;
   for (unsigned i = 0; i < phi->block->predecessors_count; i++) {
               /* If there are any undef sources, then the remaining sources may not
   * dominate the phi node, even if they are all equal. So we need to
   * bail out in this case.
   *
   * This seems to be a bug in the original paper.
   */
   if (!src->def) {
      unique = false;
                        /* phi sources which point to the phi itself don't count for
   * figuring out if the phi is trivial
   */
   if (src_instr == phi)
            if (src_instr->opc == OPC_META_PHI) {
                  if (unique_def) {
      if (unique_def != src->def) {
      unique = false;
         } else {
                     if (unique) {
      phi->data = unique_def;
      } else {
            }
      static struct ir3_register *
   lookup_value(struct ir3_register *reg)
   {
      if (reg->instr->opc == OPC_META_PHI)
            }
      static struct ir3_register *
   lookup_live_in(struct array_ctx *ctx, struct ir3_block *block, unsigned id)
   {
      struct array_state *state = get_state(ctx, block, id);
   if (state->live_in_definition)
               }
      bool
   ir3_array_to_ssa(struct ir3 *ir)
   {
               foreach_array (array, &ir->array_list) {
                  if (ctx.array_count == 0)
            unsigned i = 0;
   foreach_block (block, &ir->block_list) {
                  ctx.ir = ir;
            foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      foreach_dst (dst, instr) {
      if (dst->flags & IR3_REG_ARRAY) {
      struct array_state *state =
                              foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
                     foreach_dst (reg, instr) {
                        /* Construct any phi nodes necessary to read this value */
         }
   foreach_src (reg, instr) {
                        /* Construct any phi nodes necessary to read this value */
                        foreach_block (block, &ir->block_list) {
      foreach_instr_safe (instr, &block->instr_list) {
      if (instr->opc == OPC_META_PHI)
         else
                  foreach_block (block, &ir->block_list) {
      foreach_instr_safe (instr, &block->instr_list) {
      if (instr->opc == OPC_META_PHI) {
      if (!(instr->flags & IR3_REG_ARRAY))
         if (instr->data != instr->dsts[0]) {
      list_del(&instr->node);
      }
   for (unsigned i = 0; i < instr->srcs_count; i++) {
            } else {
      foreach_dst (reg, instr) {
      if ((reg->flags & IR3_REG_ARRAY)) {
      if (!reg->tied) {
      struct ir3_register *def =
         if (def)
      }
         }
   foreach_src (reg, instr) {
      if ((reg->flags & IR3_REG_ARRAY)) {
      /* It is assumed that before calling
   * ir3_array_to_ssa(), reg->def was set to the
   * previous writer of the array within the current
   * block or NULL if none.
   */
   if (!reg->def) {
         }
                           free(ctx.states);
      }
