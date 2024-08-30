   /*
   * Copyright © 2020 Google, Inc.
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
      #include <stdlib.h>
      #include "util/ralloc.h"
      #include "ir3.h"
      struct ir3_validate_ctx {
               /* Current block being validated: */
            /* Current instruction being validated: */
            /* Set of instructions found so far, used to validate that we
   * don't have SSA uses that occure before def's
   */
      };
      static void
   validate_error(struct ir3_validate_ctx *ctx, const char *condstr)
   {
      fprintf(stderr, "validation fail: %s\n", condstr);
   if (ctx->current_instr) {
      fprintf(stderr, "  -> for instruction: ");
      } else {
         }
      }
      #define validate_assert(ctx, cond)                                             \
      do {                                                                        \
      if (!(cond)) {                                                           \
                  static unsigned
   reg_class_flags(struct ir3_register *reg)
   {
         }
      static void
   validate_src(struct ir3_validate_ctx *ctx, struct ir3_instruction *instr,
         {
      if (reg->flags & IR3_REG_IMMED)
            if (!(reg->flags & IR3_REG_SSA) || !reg->def)
                     validate_assert(ctx, _mesa_set_search(ctx->defs, src->instr));
   validate_assert(ctx, src->wrmask == reg->wrmask);
            if (reg->tied) {
      validate_assert(ctx, reg->tied->tied == reg);
   bool found = false;
   foreach_dst (dst, instr) {
      if (dst == reg->tied) {
      found = true;
         }
   validate_assert(ctx,
         }
      /* phi sources are logically read at the end of the predecessor basic block,
   * and we have to validate them then in order to correctly validate that the
   * use comes after the definition for loop phis.
   */
   static void
   validate_phi_src(struct ir3_validate_ctx *ctx, struct ir3_block *block,
         {
               foreach_instr (phi, &block->instr_list) {
      if (phi->opc != OPC_META_PHI)
            ctx->current_instr = phi;
   validate_assert(ctx, phi->srcs_count == block->predecessors_count);
         }
      static void
   validate_phi(struct ir3_validate_ctx *ctx, struct ir3_instruction *phi)
   {
      _mesa_set_add(ctx->defs, phi);
   validate_assert(ctx, phi->dsts_count == 1);
      }
      static void
   validate_dst(struct ir3_validate_ctx *ctx, struct ir3_instruction *instr,
         {
      if (reg->tied) {
      validate_assert(ctx, reg->tied->tied == reg);
   validate_assert(ctx, reg_class_flags(reg->tied) == reg_class_flags(reg));
   validate_assert(ctx, reg->tied->wrmask == reg->wrmask);
   if (reg->flags & IR3_REG_ARRAY) {
      validate_assert(ctx, reg->tied->array.base == reg->array.base);
      }
   bool found = false;
   foreach_src (src, instr) {
      if (src == reg->tied) {
      found = true;
         }
   validate_assert(ctx,
               if (reg->flags & IR3_REG_SSA)
            if (reg->flags & IR3_REG_RELATIV)
      }
      #define validate_reg_size(ctx, reg, type)                                      \
      validate_assert(                                                            \
         static void
   validate_instr(struct ir3_validate_ctx *ctx, struct ir3_instruction *instr)
   {
               foreach_src_n (reg, n, instr) {
      if (reg->flags & IR3_REG_RELATIV)
                     /* Validate that all src's are either half of full.
   *
   * Note: tex instructions w/ .s2en are a bit special in that the
   * tex/samp src reg is half-reg for non-bindless and full for
   * bindless, irrespective of the precision of other srcs. The
   * tex/samp src is the first src reg when .s2en is set
   */
   if (reg->tied) {
      /* must have the same size as the destination, handled in
   * validate_reg().
      } else if (reg == instr->address) {
         } else if ((instr->flags & IR3_INSTR_S2EN) && (n < 2)) {
      if (n == 0) {
      if (instr->flags & IR3_INSTR_B)
         else
         } else if (opc_cat(instr->opc) == 1 || opc_cat(instr->opc) == 6) {
         } else if (opc_cat(instr->opc) == 0) {
         } else if (instr->opc == OPC_META_PARALLEL_COPY) {
      /* pcopy sources have to match with their destination but can have
   * different sizes from each other.
      } else if (instr->opc == OPC_ANY_MACRO || instr->opc == OPC_ALL_MACRO ||
            instr->opc == OPC_READ_FIRST_MACRO ||
      } else if (n > 0) {
      validate_assert(ctx, (last_reg->flags & IR3_REG_HALF) ==
                           for (unsigned i = 0; i < instr->dsts_count; i++) {
                                    /* Check that src/dst types match the register types, and for
   * instructions that have different opcodes depending on type,
   * that the opcodes are correct.
   */
   switch (opc_cat(instr->opc)) {
   case 1: /* move instructions */
      if (instr->opc == OPC_MOVMSK || instr->opc == OPC_BALLOT_MACRO) {
      validate_assert(ctx, instr->dsts_count == 1);
   validate_assert(ctx, instr->dsts[0]->flags & IR3_REG_SHARED);
   validate_assert(ctx, !(instr->dsts[0]->flags & IR3_REG_HALF));
   validate_assert(
      } else if (instr->opc == OPC_ANY_MACRO || instr->opc == OPC_ALL_MACRO ||
            instr->opc == OPC_READ_FIRST_MACRO ||
      } else if (instr->opc == OPC_ELECT_MACRO || instr->opc == OPC_SHPS_MACRO) {
      validate_assert(ctx, instr->dsts_count == 1);
      } else if (instr->opc == OPC_SCAN_MACRO) {
      validate_assert(ctx, instr->dsts_count == 3);
   validate_assert(ctx, instr->srcs_count == 2);
   validate_assert(ctx, reg_class_flags(instr->dsts[0]) ==
         validate_assert(ctx, reg_class_flags(instr->dsts[1]) ==
            } else {
      foreach_dst (dst, instr)
         foreach_src (src, instr) {
      if (!src->tied && src != instr->address)
               switch (instr->opc) {
   case OPC_SWZ:
      validate_assert(ctx, instr->srcs_count == 2);
   validate_assert(ctx, instr->dsts_count == 2);
      case OPC_GAT:
      validate_assert(ctx, instr->srcs_count == 4);
   validate_assert(ctx, instr->dsts_count == 1);
      case OPC_SCT:
      validate_assert(ctx, instr->srcs_count == 1);
   validate_assert(ctx, instr->dsts_count == 4);
      default:
                     if (instr->opc != OPC_MOV)
               case 3:
      /* Validate that cat3 opc matches the src type.  We've already checked
   * that all the src regs are same type
   */
   if (instr->srcs[0]->flags & IR3_REG_HALF) {
         } else {
         }
      case 4:
      /* Validate that cat4 opc matches the dst type: */
   if (instr->dsts[0]->flags & IR3_REG_HALF) {
         } else {
         }
      case 5:
      validate_reg_size(ctx, instr->dsts[0], instr->cat5.type);
      case 6:
      switch (instr->opc) {
   case OPC_RESINFO:
   case OPC_RESFMT:
      validate_reg_size(ctx, instr->dsts[0], instr->cat6.type);
   validate_reg_size(ctx, instr->srcs[0], instr->cat6.type);
      case OPC_L2G:
   case OPC_G2L:
      validate_assert(ctx, !(instr->dsts[0]->flags & IR3_REG_HALF));
   validate_assert(ctx, !(instr->srcs[0]->flags & IR3_REG_HALF));
      case OPC_STG:
      validate_assert(ctx, !(instr->srcs[0]->flags & IR3_REG_HALF));
   validate_assert(ctx, !(instr->srcs[1]->flags & IR3_REG_HALF));
   validate_reg_size(ctx, instr->srcs[2], instr->cat6.type);
   validate_assert(ctx, !(instr->srcs[3]->flags & IR3_REG_HALF));
      case OPC_STG_A:
      validate_assert(ctx, !(instr->srcs[0]->flags & IR3_REG_HALF));
   validate_assert(ctx, !(instr->srcs[2]->flags & IR3_REG_HALF));
   validate_assert(ctx, !(instr->srcs[3]->flags & IR3_REG_HALF));
   validate_reg_size(ctx, instr->srcs[4], instr->cat6.type);
   validate_assert(ctx, !(instr->srcs[5]->flags & IR3_REG_HALF));
      case OPC_STL:
   case OPC_STP:
   case OPC_STLW:
   case OPC_SPILL_MACRO:
      validate_assert(ctx, !(instr->srcs[0]->flags & IR3_REG_HALF));
   validate_reg_size(ctx, instr->srcs[1], instr->cat6.type);
   validate_assert(ctx, !(instr->srcs[2]->flags & IR3_REG_HALF));
      case OPC_STIB:
      validate_assert(ctx, !(instr->srcs[0]->flags & IR3_REG_HALF));
   validate_assert(ctx, !(instr->srcs[1]->flags & IR3_REG_HALF));
   validate_reg_size(ctx, instr->srcs[2], instr->cat6.type);
      case OPC_GETFIBERID:
   case OPC_GETSPID:
   case OPC_GETWID:
      validate_reg_size(ctx, instr->dsts[0], instr->cat6.type);
      case OPC_STC:
   case OPC_STSC:
      validate_reg_size(ctx, instr->srcs[0], instr->cat6.type);
   validate_assert(ctx, !(instr->srcs[1]->flags & IR3_REG_HALF));
      case OPC_LDC_K:
      validate_assert(ctx, !(instr->srcs[0]->flags & IR3_REG_HALF));
   validate_assert(ctx, !(instr->srcs[1]->flags & IR3_REG_HALF));
      default:
      validate_reg_size(ctx, instr->dsts[0], instr->cat6.type);
   validate_assert(ctx, !(instr->srcs[0]->flags & IR3_REG_HALF));
   if (instr->srcs_count > 1)
                        if (instr->opc == OPC_META_PARALLEL_COPY) {
      foreach_src_n (src, n, instr) {
      validate_assert(ctx, reg_class_flags(src) ==
            }
      static bool
   is_physical_successor(struct ir3_block *block, struct ir3_block *succ)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(block->physical_successors); i++)
      if (block->physical_successors[i] == succ)
         }
      void
   ir3_validate(struct ir3 *ir)
   {
   #ifdef NDEBUG
   #define VALIDATE 0
   #else
   #define VALIDATE 1
   #endif
         if (!VALIDATE)
                     ctx->ir = ir;
            foreach_block (block, &ir->block_list) {
      ctx->current_block = block;
            /* We require that the first block does not have any predecessors,
   * which allows us to assume that phi nodes and meta:input's do not
   * appear in the same basic block.
   */
   validate_assert(
            struct ir3_instruction *prev = NULL;
   foreach_instr (instr, &block->instr_list) {
      ctx->current_instr = instr;
   if (instr->opc == OPC_META_PHI) {
      /* phis must be the first in the block */
   validate_assert(ctx, prev == NULL || prev->opc == OPC_META_PHI);
      } else {
         }
               for (unsigned i = 0; i < 2; i++) {
                                 /* Each logical successor should also be a physical successor: */
                  validate_assert(ctx, block->successors[0] || !block->successors[1]);
                  }
