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
      #include "ir3.h"
      /* Lower several macro-instructions needed for shader subgroup support that
   * must be turned into if statements. We do this after RA and post-RA
   * scheduling to give the scheduler a chance to rearrange them, because RA
   * may need to insert OPC_META_READ_FIRST to handle splitting live ranges, and
   * also because some (e.g. BALLOT and READ_FIRST) must produce a shared
   * register that cannot be spilled to a normal register until after the if,
   * which makes implementing spilling more complicated if they are already
   * lowered.
   */
      static void
   replace_pred(struct ir3_block *block, struct ir3_block *old_pred,
         {
      for (unsigned i = 0; i < block->predecessors_count; i++) {
      if (block->predecessors[i] == old_pred) {
      block->predecessors[i] = new_pred;
            }
      static void
   replace_physical_pred(struct ir3_block *block, struct ir3_block *old_pred,
         {
      for (unsigned i = 0; i < block->physical_predecessors_count; i++) {
      if (block->physical_predecessors[i] == old_pred) {
      block->physical_predecessors[i] = new_pred;
            }
      static void
   mov_immed(struct ir3_register *dst, struct ir3_block *block, unsigned immed)
   {
      struct ir3_instruction *mov = ir3_instr_create(block, OPC_MOV, 1, 1);
   struct ir3_register *mov_dst = ir3_dst_create(mov, dst->num, dst->flags);
   mov_dst->wrmask = dst->wrmask;
   struct ir3_register *src = ir3_src_create(
         src->uim_val = immed;
   mov->cat1.dst_type = (dst->flags & IR3_REG_HALF) ? TYPE_U16 : TYPE_U32;
   mov->cat1.src_type = mov->cat1.dst_type;
      }
      static void
   mov_reg(struct ir3_block *block, struct ir3_register *dst,
         {
               struct ir3_register *mov_dst =
         struct ir3_register *mov_src =
         mov_dst->wrmask = dst->wrmask;
   mov_src->wrmask = src->wrmask;
            mov->cat1.dst_type = (dst->flags & IR3_REG_HALF) ? TYPE_U16 : TYPE_U32;
      }
      static void
   binop(struct ir3_block *block, opc_t opc, struct ir3_register *dst,
         {
      struct ir3_instruction *instr = ir3_instr_create(block, opc, 1, 2);
      unsigned flags = dst->flags & IR3_REG_HALF;
   struct ir3_register *instr_dst = ir3_dst_create(instr, dst->num, flags);
   struct ir3_register *instr_src0 = ir3_src_create(instr, src0->num, flags);
            instr_dst->wrmask = dst->wrmask;
   instr_src0->wrmask = src0->wrmask;
   instr_src1->wrmask = src1->wrmask;
      }
      static void
   triop(struct ir3_block *block, opc_t opc, struct ir3_register *dst,
         struct ir3_register *src0, struct ir3_register *src1,
   {
      struct ir3_instruction *instr = ir3_instr_create(block, opc, 1, 3);
      unsigned flags = dst->flags & IR3_REG_HALF;
   struct ir3_register *instr_dst = ir3_dst_create(instr, dst->num, flags);
   struct ir3_register *instr_src0 = ir3_src_create(instr, src0->num, flags);
   struct ir3_register *instr_src1 = ir3_src_create(instr, src1->num, flags);
            instr_dst->wrmask = dst->wrmask;
   instr_src0->wrmask = src0->wrmask;
   instr_src1->wrmask = src1->wrmask;
   instr_src2->wrmask = src2->wrmask;
      }
      static void
   do_reduce(struct ir3_block *block, reduce_op_t opc,
               {
         #define CASE(name)                                                             \
      case REDUCE_OP_##name:                                                      \
      binop(block, OPC_##name, dst, src0, src1);                               \
         CASE(ADD_U)
   CASE(ADD_F)
   CASE(MUL_F)
   CASE(MIN_U)
   CASE(MIN_S)
   CASE(MIN_F)
   CASE(MAX_U)
   CASE(MAX_S)
   CASE(MAX_F)
   CASE(AND_B)
   CASE(OR_B)
         #undef CASE
         case REDUCE_OP_MUL_U:
      if (dst->flags & IR3_REG_HALF) {
         } else {
      /* 32-bit multiplication macro - see ir3_nir_imul */
   binop(block, OPC_MULL_U, dst, src0, src1);
   triop(block, OPC_MADSH_M16, dst, src0, src1, dst);
      }
         }
      static struct ir3_block *
   split_block(struct ir3 *ir, struct ir3_block *before_block,
         {
      struct ir3_block *after_block = ir3_block_create(ir);
            for (unsigned i = 0; i < ARRAY_SIZE(before_block->successors); i++) {
      after_block->successors[i] = before_block->successors[i];
   if (after_block->successors[i])
               for (unsigned i = 0; i < ARRAY_SIZE(before_block->physical_successors);
      i++) {
   after_block->physical_successors[i] =
         if (after_block->physical_successors[i]) {
      replace_physical_pred(after_block->physical_successors[i],
                  before_block->successors[0] = before_block->successors[1] = NULL;
            foreach_instr_from_safe (rem_instr, &instr->node,
            list_del(&rem_instr->node);
   list_addtail(&rem_instr->node, &after_block->instr_list);
               after_block->brtype = before_block->brtype;
               }
      static void
   link_blocks_physical(struct ir3_block *pred, struct ir3_block *succ,
         {
      pred->physical_successors[index] = succ;
      }
      static void
   link_blocks(struct ir3_block *pred, struct ir3_block *succ, unsigned index)
   {
      pred->successors[index] = succ;
   ir3_block_add_predecessor(succ, pred);
      }
      static struct ir3_block *
   create_if(struct ir3 *ir, struct ir3_block *before_block,
         {
      struct ir3_block *then_block = ir3_block_create(ir);
            link_blocks(before_block, then_block, 0);
   link_blocks(before_block, after_block, 1);
               }
      static bool
   lower_instr(struct ir3 *ir, struct ir3_block **block, struct ir3_instruction *instr)
   {
      switch (instr->opc) {
   case OPC_BALLOT_MACRO:
   case OPC_ANY_MACRO:
   case OPC_ALL_MACRO:
   case OPC_ELECT_MACRO:
   case OPC_READ_COND_MACRO:
   case OPC_READ_FIRST_MACRO:
   case OPC_SWZ_SHARED_MACRO:
   case OPC_SCAN_MACRO:
         default:
                  struct ir3_block *before_block = *block;
            if (instr->opc == OPC_SCAN_MACRO) {
      /* The pseudo-code for the scan macro is:
   *
   * while (true) {
   *    header:
   *    if (elect()) {
   *       exit:
   *       exclusive = reduce;
   *       inclusive = src OP exclusive;
   *       reduce = inclusive;
   *    }
   *    footer:
   * }
   *
   * This is based on the blob's sequence, and carefully crafted to avoid
   * using the shared register "reduce" except in move instructions, since
   * using it in the actual OP isn't possible for half-registers.
   */
   struct ir3_block *header = ir3_block_create(ir);
            struct ir3_block *exit = ir3_block_create(ir);
            struct ir3_block *footer = ir3_block_create(ir);
                     link_blocks(header, exit, 0);
   link_blocks(header, footer, 1);
            link_blocks(exit, after_block, 0);
                     struct ir3_register *exclusive = instr->dsts[0];
   struct ir3_register *inclusive = instr->dsts[1];
   struct ir3_register *reduce = instr->dsts[2];
            mov_reg(exit, exclusive, reduce);
   do_reduce(exit, instr->cat1.reduce_op, inclusive, src, exclusive);
      } else {
               /* For ballot, the destination must be initialized to 0 before we do
   * the movmsk because the condition may be 0 and then the movmsk will
   * be skipped. Because it's a shared register we have to wrap the
   * initialization in a getone block.
   */
   if (instr->opc == OPC_BALLOT_MACRO) {
      before_block->brtype = IR3_BRANCH_GETONE;
   before_block->condition = NULL;
   mov_immed(instr->dsts[0], then_block, 0);
   before_block = after_block;
   after_block = split_block(ir, before_block, instr);
               switch (instr->opc) {
   case OPC_BALLOT_MACRO:
   case OPC_READ_COND_MACRO:
   case OPC_ANY_MACRO:
   case OPC_ALL_MACRO:
      before_block->condition = instr->srcs[0]->def->instr;
      default:
      before_block->condition = NULL;
               switch (instr->opc) {
   case OPC_BALLOT_MACRO:
   case OPC_READ_COND_MACRO:
      before_block->brtype = IR3_BRANCH_COND;
      case OPC_ANY_MACRO:
      before_block->brtype = IR3_BRANCH_ANY;
      case OPC_ALL_MACRO:
      before_block->brtype = IR3_BRANCH_ALL;
      case OPC_ELECT_MACRO:
   case OPC_READ_FIRST_MACRO:
   case OPC_SWZ_SHARED_MACRO:
      before_block->brtype = IR3_BRANCH_GETONE;
      default:
                  switch (instr->opc) {
   case OPC_ALL_MACRO:
   case OPC_ANY_MACRO:
   case OPC_ELECT_MACRO:
      mov_immed(instr->dsts[0], then_block, 1);
               case OPC_BALLOT_MACRO: {
      unsigned comp_count = util_last_bit(instr->dsts[0]->wrmask);
   struct ir3_instruction *movmsk =
         ir3_dst_create(movmsk, instr->dsts[0]->num, instr->dsts[0]->flags);
   movmsk->repeat = comp_count - 1;
               case OPC_READ_COND_MACRO:
   case OPC_READ_FIRST_MACRO: {
      struct ir3_instruction *mov =
         unsigned src = instr->opc == OPC_READ_COND_MACRO ? 1 : 0;
   ir3_dst_create(mov, instr->dsts[0]->num, instr->dsts[0]->flags);
   struct ir3_register *new_src = ir3_src_create(mov, 0, 0);
   *new_src = *instr->srcs[src];
   mov->cat1.dst_type = TYPE_U32;
   mov->cat1.src_type =
                     case OPC_SWZ_SHARED_MACRO: {
      struct ir3_instruction *swz =
         ir3_dst_create(swz, instr->dsts[0]->num, instr->dsts[0]->flags);
   ir3_dst_create(swz, instr->dsts[1]->num, instr->dsts[1]->flags);
   ir3_src_create(swz, instr->srcs[0]->num, instr->srcs[0]->flags);
   ir3_src_create(swz, instr->srcs[1]->num, instr->srcs[1]->flags);
   swz->cat1.dst_type = swz->cat1.src_type = TYPE_U32;
   swz->repeat = 1;
               default:
                     *block = after_block;
   list_delinit(&instr->node);
      }
      static bool
   lower_block(struct ir3 *ir, struct ir3_block **block)
   {
               bool inner_progress;
   do {
      inner_progress = false;
   foreach_instr (instr, &(*block)->instr_list) {
      if (lower_instr(ir, block, instr)) {
      /* restart the loop with the new block we created because the
   * iterator has been invalidated.
   */
   progress = inner_progress = true;
                        }
      bool
   ir3_lower_subgroups(struct ir3 *ir)
   {
               foreach_block (block, &ir->block_list)
               }
