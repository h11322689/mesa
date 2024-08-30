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
      #include "ir3_compiler.h"
   #include "ir3_ra.h"
   #include "util/ralloc.h"
      /* This pass "merges" compatible phi-web SSA values. First, we insert a bunch
   * of parallelcopy's to trivially turn the program into CSSA form. Then we
   * try to "merge" SSA def's into "merge sets" which could be allocated to a
   * single register in order to eliminate copies. First we merge phi nodes,
   * which should always succeed because of the parallelcopy's we inserted, and
   * then we try to coalesce the copies we introduced.
   *
   * The merged registers are used for three purposes:
   *
   * 1. We always use the same pvtmem slot for spilling all SSA defs in each
   * merge set. This prevents us from having to insert memory-to-memory copies
   * in the spiller and makes sure we don't insert unecessary copies.
   * 2. When two values are live at the same time, part of the same merge
   * set, and they overlap each other in the merge set, they always occupy
   * overlapping physical registers in RA. This reduces register pressure and
   * copies in several important scenarios:
   *	- When sources of a collect are used later by something else, we don't
   *	have to introduce copies.
   *	- We can handle sequences of extracts that "explode" a vector into its
   *	components without any additional copying.
   * 3. We use the merge sets for affinities in register allocation: That is, we
   * try to allocate all the definitions in the same merge set to the
   * same/compatible registers. This helps us e.g. allocate sources of a collect
   * to contiguous registers without too much special code in RA.
   *
   * In a "normal" register allocator, or when spilling, we'd just merge
   * registers in the same merge set to the same register, but with SSA-based
   * register allocation we may have to split the live interval.
   *
   * The implementation is based on "Revisiting Out-of-SSA Translation for
   * Correctness, CodeQuality, and Eﬀiciency," and is broadly similar to the
   * implementation in nir_from_ssa, with the twist that we also try to coalesce
   * META_SPLIT and META_COLLECT. This makes this pass more complicated but
   * prevents us from needing to handle these specially in RA and the spiller,
   * which are already complicated enough. This also forces us to implement that
   * value-comparison optimization they explain, as without it we wouldn't be
   * able to coalesce META_SPLIT even in the simplest of cases.
   */
      /* In order to dynamically reconstruct the dominance forest, we need the
   * instructions ordered by a preorder traversal of the dominance tree:
   */
      static unsigned
   index_instrs(struct ir3_block *block, unsigned index)
   {
      foreach_instr (instr, &block->instr_list)
            for (unsigned i = 0; i < block->dom_children_count; i++)
               }
      /* Definitions within a merge set are ordered by instr->ip as set above: */
      static bool
   def_after(struct ir3_register *a, struct ir3_register *b)
   {
         }
      static bool
   def_dominates(struct ir3_register *a, struct ir3_register *b)
   {
      if (def_after(a, b)) {
         } else if (a->instr->block == b->instr->block) {
         } else {
            }
      /* This represents a region inside a register. The offset is relative to the
   * start of the register, and offset + size <= size(reg).
   */
   struct def_value {
      struct ir3_register *reg;
      };
      /* Chase any copies to get the source of a region inside a register. This is
   * Value(a) in the paper.
   */
   static struct def_value
   chase_copies(struct def_value value)
   {
      while (true) {
      struct ir3_instruction *instr = value.reg->instr;
   if (instr->opc == OPC_META_SPLIT) {
      value.offset += instr->split.off * reg_elem_size(value.reg);
      } else if (instr->opc == OPC_META_COLLECT) {
      if (value.offset % reg_elem_size(value.reg) != 0 ||
      value.size > reg_elem_size(value.reg) ||
   value.offset + value.size > reg_size(value.reg))
      struct ir3_register *src =
         if (!src->def)
         value.offset = 0;
      } else {
      /* TODO: parallelcopy */
                     }
      /* This represents an entry in the merge set, and consists of a register +
   * offset from the merge set base.
   */
   struct merge_def {
      struct ir3_register *reg;
      };
      static bool
   can_skip_interference(const struct merge_def *a, const struct merge_def *b)
   {
      unsigned a_start = a->offset;
   unsigned b_start = b->offset;
   unsigned a_end = a_start + reg_size(a->reg);
            /* Registers that don't overlap never interfere */
   if (a_end <= b_start || b_end <= a_start)
            /* Disallow skipping interference unless one definition contains the
   * other. This restriction is important for register allocation, because
   * it means that at any given point in the program, the live values in a
   * given merge set will form a tree. If they didn't, then one live value
   * would partially overlap another, and they would have overlapping live
   * ranges because they're live at the same point. This simplifies register
   * allocation and spilling.
   */
   if (!((a_start <= b_start && a_end >= b_end) ||
                  /* For each register, chase the intersection of a and b to find the
   * ultimate source.
   */
   unsigned start = MAX2(a_start, b_start);
   unsigned end = MIN2(a_end, b_end);
   struct def_value a_value = chase_copies((struct def_value){
      .reg = a->reg,
   .offset = start - a_start,
      });
   struct def_value b_value = chase_copies((struct def_value){
      .reg = b->reg,
   .offset = start - b_start,
      });
      }
      static struct ir3_merge_set *
   get_merge_set(struct ir3_register *def)
   {
      if (def->merge_set)
            struct ir3_merge_set *set = ralloc(def, struct ir3_merge_set);
   set->preferred_reg = ~0;
   set->interval_start = ~0;
   set->spill_slot = ~0;
   set->size = reg_size(def);
   set->alignment = (def->flags & IR3_REG_HALF) ? 1 : 2;
   set->regs_count = 1;
   set->regs = ralloc(set, struct ir3_register *);
               }
      /* Merges b into a */
   static struct ir3_merge_set *
   merge_merge_sets(struct ir3_merge_set *a, struct ir3_merge_set *b, int b_offset)
   {
      if (b_offset < 0)
            struct ir3_register **new_regs =
            unsigned a_index = 0, b_index = 0, new_index = 0;
   for (; a_index < a->regs_count || b_index < b->regs_count; new_index++) {
      if (b_index < b->regs_count &&
      (a_index == a->regs_count ||
   def_after(a->regs[a_index], b->regs[b_index]))) {
   new_regs[new_index] = b->regs[b_index++];
      } else {
         }
                        /* Technically this should be the lcm, but because alignment is only 1 or
   * 2 so far this should be ok.
   */
   a->alignment = MAX2(a->alignment, b->alignment);
   a->regs_count += b->regs_count;
   ralloc_free(a->regs);
   a->regs = new_regs;
               }
      static bool
   merge_sets_interfere(struct ir3_liveness *live, struct ir3_merge_set *a,
         {
      if (b_offset < 0)
            struct merge_def dom[a->regs_count + b->regs_count];
   unsigned a_index = 0, b_index = 0;
            /* Reject trying to merge the sets if the alignment doesn't work out */
   if (b_offset % a->alignment != 0)
            while (a_index < a->regs_count || b_index < b->regs_count) {
      struct merge_def current;
   if (a_index == a->regs_count) {
      current.reg = b->regs[b_index];
   current.offset = current.reg->merge_set_offset + b_offset;
      } else if (b_index == b->regs_count) {
      current.reg = a->regs[a_index];
   current.offset = current.reg->merge_set_offset;
      } else {
      if (def_after(b->regs[b_index], a->regs[a_index])) {
      current.reg = a->regs[a_index];
   current.offset = current.reg->merge_set_offset;
      } else {
      current.reg = b->regs[b_index];
   current.offset = current.reg->merge_set_offset + b_offset;
                  while (dom_index >= 0 &&
                        /* TODO: in the original paper, just dom[dom_index] needs to be
   * checked for interference. We implement the value-chasing extension
   * as well as support for sub-registers, which complicates this
   * significantly because it's no longer the case that if a dominates b
   * dominates c and a and b don't interfere then we only need to check
   * interference between b and c to be sure a and c don't interfere --
   * this means we may have to check for interference against values
   * higher in the stack then dom[dom_index]. In the paper there's a
   * description of a way to do less interference tests with the
   * value-chasing extension, but we'd have to come up with something
   * ourselves for handling the similar problems that come up with
   * allowing values to contain subregisters. For now we just test
   * everything in the stack.
   */
   for (int i = 0; i <= dom_index; i++) {
                     /* Ok, now we actually have to check interference. Since we know
   * that dom[i] dominates current, this boils down to checking
   * whether dom[i] is live after current.
   */
   if (ir3_def_live_after(live, dom[i].reg, current.reg->instr))
                              }
      static void
   try_merge_defs(struct ir3_liveness *live, struct ir3_register *a,
         {
      struct ir3_merge_set *a_set = get_merge_set(a);
            if (a_set == b_set) {
      /* Note: Even in this case we may not always successfully be able to
   * coalesce this copy, if the offsets don't line up. But in any
   * case, we can't do anything.
   */
                        if (!merge_sets_interfere(live, a_set, b_set, b_set_offset))
      }
      void
   ir3_force_merge(struct ir3_register *a, struct ir3_register *b, int b_offset)
   {
      struct ir3_merge_set *a_set = get_merge_set(a);
            if (a_set == b_set)
            int b_set_offset = a->merge_set_offset + b_offset - b->merge_set_offset;
      }
      static void
   coalesce_phi(struct ir3_liveness *live, struct ir3_instruction *phi)
   {
      for (unsigned i = 0; i < phi->srcs_count; i++) {
      if (phi->srcs[i]->def)
         }
      static void
   aggressive_coalesce_parallel_copy(struct ir3_liveness *live,
         {
      for (unsigned i = 0; i < pcopy->dsts_count; i++) {
      if (!(pcopy->srcs[i]->flags & IR3_REG_SSA))
               }
      static void
   aggressive_coalesce_split(struct ir3_liveness *live,
         {
      try_merge_defs(live, split->srcs[0]->def, split->dsts[0],
      }
      static void
   aggressive_coalesce_collect(struct ir3_liveness *live,
         {
      for (unsigned i = 0, offset = 0; i < collect->srcs_count;
      offset += reg_elem_size(collect->srcs[i]), i++) {
   if (!(collect->srcs[i]->flags & IR3_REG_SSA))
               }
      static void
   create_parallel_copy(struct ir3_block *block)
   {
      for (unsigned i = 0; i < 2; i++) {
      if (!block->successors[i])
                              unsigned phi_count = 0;
   foreach_instr (phi, &succ->instr_list) {
                     /* Avoid undef */
   if ((phi->srcs[pred_idx]->flags & IR3_REG_SSA) &&
                  /* We don't support critical edges. If we were to support them,
   * we'd need to insert parallel copies after the phi node to solve
   * the lost-copy problem.
   */
   assert(i == 0 && !block->successors[1]);
               if (phi_count == 0)
            struct ir3_register *src[phi_count];
   unsigned j = 0;
   foreach_instr (phi, &succ->instr_list) {
      if (phi->opc != OPC_META_PHI)
         if ((phi->srcs[pred_idx]->flags & IR3_REG_SSA) &&
      !phi->srcs[pred_idx]->def)
         }
            struct ir3_instruction *pcopy =
            for (j = 0; j < phi_count; j++) {
      struct ir3_register *reg = __ssa_dst(pcopy);
   reg->flags |= src[j]->flags & (IR3_REG_HALF | IR3_REG_ARRAY);
   reg->size = src[j]->size;
               for (j = 0; j < phi_count; j++) {
      pcopy->srcs[pcopy->srcs_count++] =
               j = 0;
   foreach_instr (phi, &succ->instr_list) {
      if (phi->opc != OPC_META_PHI)
         if ((phi->srcs[pred_idx]->flags & IR3_REG_SSA) &&
      !phi->srcs[pred_idx]->def)
      phi->srcs[pred_idx]->def = pcopy->dsts[j];
   phi->srcs[pred_idx]->flags = pcopy->dsts[j]->flags;
      }
         }
      void
   ir3_create_parallel_copies(struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list) {
            }
      static void
   index_merge_sets(struct ir3_liveness *live, struct ir3 *ir)
   {
      unsigned offset = 0;
   foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
                        unsigned dst_offset;
   struct ir3_merge_set *merge_set = dst->merge_set;
   unsigned size = reg_size(dst);
   if (merge_set) {
      if (merge_set->interval_start == ~0) {
      merge_set->interval_start = offset;
      }
      } else {
                        dst->interval_start = dst_offset;
                        }
      #define RESET      "\x1b[0m"
   #define BLUE       "\x1b[0;34m"
   #define SYN_SSA(x) BLUE x RESET
      static void
   dump_merge_sets(struct ir3 *ir)
   {
      d("merge sets:");
   struct set *merge_sets = _mesa_pointer_set_create(NULL);
   foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
                        struct ir3_merge_set *merge_set = dst->merge_set;
                  d("merge set, size %u, align %u:", merge_set->size,
   merge_set->alignment);
   for (unsigned j = 0; j < merge_set->regs_count; j++) {
      struct ir3_register *reg = merge_set->regs[j];
                                          }
      void
   ir3_merge_regs(struct ir3_liveness *live, struct ir3 *ir)
   {
               /* First pass: coalesce phis, which must be together. */
   foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
                                    /* Second pass: aggressively coalesce parallelcopy, split, collect */
   foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      switch (instr->opc) {
   case OPC_META_SPLIT:
      aggressive_coalesce_split(live, instr);
      case OPC_META_COLLECT:
      aggressive_coalesce_collect(live, instr);
      case OPC_META_PARALLEL_COPY:
      aggressive_coalesce_parallel_copy(live, instr);
      default:
                                 if (ir3_shader_debug & IR3_DBG_RAMSGS)
      }
