   /*
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
      #include "util/ralloc.h"
   #include "util/u_math.h"
      #include "ir3.h"
   #include "ir3_shader.h"
      /*
   * Legalize:
   *
   * The legalize pass handles ensuring sufficient nop's and sync flags for
   * correct execution.
   *
   * 1) Iteratively determine where sync ((sy)/(ss)) flags are needed,
   *    based on state flowing out of predecessor blocks until there is
   *    no further change.  In some cases this requires inserting nops.
   * 2) Mark (ei) on last varying input
   * 3) Final nop scheduling for instruction latency
   * 4) Resolve jumps and schedule blocks, marking potential convergence
   *    points with (jp)
   */
      struct ir3_legalize_ctx {
      struct ir3_compiler *compiler;
   struct ir3_shader_variant *so;
   gl_shader_stage type;
   int max_bary;
   bool early_input_release;
      };
      struct ir3_legalize_state {
      regmask_t needs_ss;
   regmask_t needs_ss_war; /* write after read */
   regmask_t needs_sy;
      };
      struct ir3_legalize_block_data {
      bool valid;
      };
      static inline void
   apply_ss(struct ir3_instruction *instr,
               {
      instr->flags |= IR3_INSTR_SS;
   regmask_init(&state->needs_ss_war, mergedregs);
   regmask_init(&state->needs_ss, mergedregs);
      }
      /* We want to evaluate each block from the position of any other
   * predecessor block, in order that the flags set are the union of
   * all possible program paths.
   *
   * To do this, we need to know the output state (needs_ss/ss_war/sy)
   * of all predecessor blocks.  The tricky thing is loops, which mean
   * that we can't simply recursively process each predecessor block
   * before legalizing the current block.
   *
   * How we handle that is by looping over all the blocks until the
   * results converge.  If the output state of a given block changes
   * in a given pass, this means that all successor blocks are not
   * yet fully legalized.
   */
      static bool
   legalize_block(struct ir3_legalize_ctx *ctx, struct ir3_block *block)
   {
               if (bd->valid)
            struct ir3_instruction *last_n = NULL;
   struct list_head instr_list;
   struct ir3_legalize_state prev_state = bd->state;
   struct ir3_legalize_state *state = &bd->state;
   bool last_input_needs_ss = false;
   bool has_tex_prefetch = false;
            /* our input state is the OR of all predecessor blocks' state: */
   for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *predecessor = block->predecessors[i];
   struct ir3_legalize_block_data *pbd = predecessor->data;
            /* Our input (ss)/(sy) state is based on OR'ing the output
   * state of all our predecessor blocks
   */
   regmask_or(&state->needs_ss, &state->needs_ss, &pstate->needs_ss);
   regmask_or(&state->needs_ss_war, &state->needs_ss_war,
         regmask_or(&state->needs_sy, &state->needs_sy, &pstate->needs_sy);
               /* We need to take phsyical-only edges into account when tracking shared
   * registers.
   */
   for (unsigned i = 0; i < block->physical_predecessors_count; i++) {
      struct ir3_block *predecessor = block->physical_predecessors[i];
   struct ir3_legalize_block_data *pbd = predecessor->data;
                                 foreach_instr (n, &block->instr_list) {
      if (is_input(n)) {
                              /* Either inputs are in the first block or we expect inputs to be released
   * with the end of the program.
   */
   assert(input_count == 0 || !ctx->early_input_release ||
            /* remove all the instructions from the list, we'll be adding
   * them back in as we go
   */
   list_replace(&block->instr_list, &instr_list);
            foreach_instr_safe (n, &instr_list) {
                        /* _meta::tex_prefetch instructions removed later in
   * collect_tex_prefetches()
   */
   if (is_meta(n) && (n->opc != OPC_META_TEX_PREFETCH))
            if (is_input(n)) {
      struct ir3_register *inloc = n->srcs[0];
   assert(inloc->flags & IR3_REG_IMMED);
               if ((last_n && is_barrier(last_n)) || n->opc == OPC_SHPE) {
               n->flags |= IR3_INSTR_SY;
   regmask_init(&state->needs_sy, mergedregs);
               if (last_n && (last_n->opc == OPC_PREDT)) {
                  /* NOTE: consider dst register too.. it could happen that
   * texture sample instruction (for example) writes some
   * components which are unused.  A subsequent instruction
   * that writes the same register can race w/ the sam instr
   * resulting in undefined results:
   */
   for (i = 0; i < n->dsts_count + n->srcs_count; i++) {
      struct ir3_register *reg;
   if (i < n->dsts_count)
                                    /* TODO: we probably only need (ss) for alu
   * instr consuming sfu result.. need to make
   * some tests for both this and (sy)..
   */
   if (regmask_get(&state->needs_ss, reg)) {
                        if (regmask_get(&state->needs_sy, reg)) {
      n->flags |= IR3_INSTR_SY;
         } else if ((reg->flags & IR3_REG_CONST) && state->needs_ss_for_const) {
      apply_ss(n, state, mergedregs);
                  foreach_dst (reg, n) {
      if (regmask_get(&state->needs_ss_war, reg)) {
      apply_ss(n, state, mergedregs);
                  /* cat5+ does not have an (ss) bit, if needed we need to
   * insert a nop to carry the sync flag.  Would be kinda
   * clever if we were aware of this during scheduling, but
   * this should be a pretty rare case:
   */
   if ((n->flags & IR3_INSTR_SS) && (opc_cat(n->opc) >= 5)) {
      struct ir3_instruction *nop;
   nop = ir3_NOP(block);
   nop->flags |= IR3_INSTR_SS;
               /* need to be able to set (ss) on first instruction: */
   if (list_is_empty(&block->instr_list) && (opc_cat(n->opc) >= 5) && !is_meta(n))
            if (ctx->compiler->samgq_workaround &&
      ctx->type != MESA_SHADER_FRAGMENT &&
                           for (i = 0; i < 4; i++) {
      samgp = ir3_instr_clone(n);
   samgp->opc = OPC_SAMGP0 + i;
   if (i > 1)
         } else {
      list_delinit(&n->node);
               if (is_sfu(n))
            foreach_dst (dst, n) {
      if (dst->flags & IR3_REG_SHARED)
               if (is_tex_or_prefetch(n)) {
      regmask_set(&state->needs_sy, n->dsts[0]);
   if (n->opc == OPC_META_TEX_PREFETCH)
      } else if (n->opc == OPC_RESINFO) {
      regmask_set(&state->needs_ss, n->dsts[0]);
   ir3_NOP(block)->flags |= IR3_INSTR_SS;
      } else if (is_load(n)) {
      if (is_local_mem_load(n))
         else
      } else if (is_atomic(n->opc)) {
      if (is_bindless_atomic(n->opc)) {
         } else if (is_global_a3xx_atomic(n->opc) ||
               } else {
            } else if (n->opc == OPC_PUSH_CONSTS_LOAD_MACRO) {
                  if (is_ssbo(n->opc) || is_global_a3xx_atomic(n->opc) ||
                  /* both tex/sfu appear to not always immediately consume
   * their src register(s):
   */
   if (is_tex(n) || is_sfu(n) || is_mem(n)) {
      foreach_src (reg, n) {
                     if (ctx->early_input_release && is_input(n)) {
               assert(inputs_remaining > 0);
   inputs_remaining--;
   if (inputs_remaining == 0) {
      /* This is the last input. We add the (ei) flag to release
   * varying memory after this executes. If it's an ldlv,
   * however, we need to insert a dummy bary.f on which we can
   * set the (ei) flag. We may also need to insert an (ss) to
   * guarantee that all ldlv's have finished fetching their
   * results before releasing the varying memory.
   */
   struct ir3_instruction *last_input = n;
                     /* (ss)bary.f (ei)r63.x, 0, r0.x */
   baryf = ir3_instr_create(block, OPC_BARY_F, 1, 2);
                                    last_input->dsts[0]->flags |= IR3_REG_EI;
   if (last_input_needs_ss) {
                                             if (has_tex_prefetch && !ctx->has_inputs) {
      /* texture prefetch, but *no* inputs.. we need to insert a
   * dummy bary.f at the top of the shader to unblock varying
   * storage:
   */
            /* (ss)bary.f (ei)r63.x, 0, r0.x */
   baryf = ir3_instr_create(block, OPC_BARY_F, 1, 2);
   ir3_dst_create(baryf, regid(63, 0), 0)->flags |= IR3_REG_EI;
   ir3_src_create(baryf, 0, IR3_REG_IMMED)->iim_val = 0;
            /* insert the dummy bary.f at head: */
   list_delinit(&baryf->node);
                        if (memcmp(&prev_state, state, sizeof(*state))) {
      /* our output state changed, this invalidates all of our
   * successors:
   */
   for (unsigned i = 0; i < ARRAY_SIZE(block->successors); i++) {
      if (!block->successors[i])
         struct ir3_legalize_block_data *pbd = block->successors[i]->data;
                     }
      /* Expands dsxpp and dsypp macros to:
   *
   * dsxpp.1 dst, src
   * dsxpp.1.p dst, src
   *
   * We apply this after flags syncing, as we don't want to sync in between the
   * two (which might happen if dst == src).  We do it before nop scheduling
   * because that needs to count actual instructions.
   */
   static bool
   apply_fine_deriv_macro(struct ir3_legalize_ctx *ctx, struct ir3_block *block)
   {
               /* remove all the instructions from the list, we'll be adding
   * them back in as we go
   */
   list_replace(&block->instr_list, &instr_list);
            foreach_instr_safe (n, &instr_list) {
               if (n->opc == OPC_DSXPP_MACRO || n->opc == OPC_DSYPP_MACRO) {
                                                }
      static void
   apply_push_consts_load_macro(struct ir3_legalize_ctx *ctx,
         {
      foreach_instr (n, &block->instr_list) {
      if (n->opc == OPC_PUSH_CONSTS_LOAD_MACRO) {
      struct ir3_instruction *stsc = ir3_instr_create(block, OPC_STSC, 0, 2);
   ir3_instr_move_after(stsc, n);
   ir3_src_create(stsc, 0, IR3_REG_IMMED)->iim_val =
         ir3_src_create(stsc, 0, IR3_REG_IMMED)->iim_val =
                        if (ctx->compiler->stsc_duplication_quirk) {
      struct ir3_instruction *nop = ir3_NOP(block);
   ir3_instr_move_after(nop, stsc);
   nop->flags |= IR3_INSTR_SS;
               list_delinit(&n->node);
      } else if (!is_meta(n)) {
               }
      /* NOTE: branch instructions are always the last instruction(s)
   * in the block.  We take advantage of this as we resolve the
   * branches, since "if (foo) break;" constructs turn into
   * something like:
   *
   *   block3 {
   *   	...
   *   	0029:021: mov.s32s32 r62.x, r1.y
   *   	0082:022: br !p0.x, target=block5
   *   	0083:023: br p0.x, target=block4
   *   	// succs: if _[0029:021: mov.s32s32] block4; else block5;
   *   }
   *   block4 {
   *   	0084:024: jump, target=block6
   *   	// succs: block6;
   *   }
   *   block5 {
   *   	0085:025: jump, target=block7
   *   	// succs: block7;
   *   }
   *
   * ie. only instruction in block4/block5 is a jump, so when
   * resolving branches we can easily detect this by checking
   * that the first instruction in the target block is itself
   * a jump, and setup the br directly to the jump's target
   * (and strip back out the now unreached jump)
   *
   * TODO sometimes we end up with things like:
   *
   *    br !p0.x, #2
   *    br p0.x, #12
   *    add.u r0.y, r0.y, 1
   *
   * If we swapped the order of the branches, we could drop one.
   */
   static struct ir3_block *
   resolve_dest_block(struct ir3_block *block)
   {
      /* special case for last block: */
   if (!block->successors[0])
            /* NOTE that we may or may not have inserted the jump
   * in the target block yet, so conditions to resolve
   * the dest to the dest block's successor are:
   *
   *   (1) successor[1] == NULL &&
   *   (2) (block-is-empty || only-instr-is-jump)
   */
   if (block->successors[1] == NULL) {
      if (list_is_empty(&block->instr_list)) {
         } else if (list_length(&block->instr_list) == 1) {
      struct ir3_instruction *instr =
         if (instr->opc == OPC_JUMP) {
      /* If this jump is backwards, then we will probably convert
   * the jump being resolved to a backwards jump, which will
   * change a loop-with-continue or loop-with-if into a
   * doubly-nested loop and change the convergence behavior.
   * Disallow this here.
   */
   if (block->successors[0]->index <= block->index)
                  }
      }
      static void
   remove_unused_block(struct ir3_block *old_target)
   {
               /* If there are any physical predecessors due to fallthroughs, then they may
   * fall through to any of the physical successors of this block. But we can
   * only fit two, so just pick the "earliest" one, i.e. the fallthrough if
   * possible.
   *
   * TODO: we really ought to have unlimited numbers of physical successors,
   * both because of this and because we currently don't model some scenarios
   * with nested break/continue correctly.
   */
   struct ir3_block *new_target;
   if (old_target->physical_successors[1] &&
      old_target->physical_successors[1]->start_ip <
   old_target->physical_successors[0]->start_ip) {
      } else {
                  for (unsigned i = 0; i < old_target->physical_predecessors_count; i++) {
      struct ir3_block *pred = old_target->physical_predecessors[i];
   if (pred->physical_successors[0] == old_target) {
      if (!new_target) {
      /* If we remove a physical successor, make sure the only physical
   * successor is the first one.
   */
   pred->physical_successors[0] = pred->physical_successors[1];
      } else {
            } else {
      assert(pred->physical_successors[1] == old_target);
      }
   if (new_target)
               /* cleanup dangling predecessors: */
   for (unsigned i = 0; i < ARRAY_SIZE(old_target->successors); i++) {
      if (old_target->successors[i]) {
      struct ir3_block *succ = old_target->successors[i];
                  for (unsigned i = 0; i < ARRAY_SIZE(old_target->physical_successors); i++) {
      if (old_target->physical_successors[i]) {
      struct ir3_block *succ = old_target->physical_successors[i];
            }
      static bool
   retarget_jump(struct ir3_instruction *instr, struct ir3_block *new_target)
   {
      struct ir3_block *old_target = instr->cat0.target;
            /* update current blocks successors to reflect the retargetting: */
   if (cur_block->successors[0] == old_target) {
         } else {
      assert(cur_block->successors[1] == old_target);
               /* also update physical_successors: */
   if (cur_block->physical_successors[0] == old_target) {
         } else {
      assert(cur_block->physical_successors[1] == old_target);
               /* update new target's predecessors: */
   ir3_block_add_predecessor(new_target, cur_block);
            /* and remove old_target's predecessor: */
   ir3_block_remove_predecessor(old_target, cur_block);
                     if (old_target->predecessors_count == 0) {
      remove_unused_block(old_target);
                  }
      static bool
   opt_jump(struct ir3 *ir)
   {
               unsigned index = 0;
   foreach_block (block, &ir->block_list)
            foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
                     struct ir3_block *tblock = resolve_dest_block(instr->cat0.target);
                     /* Exit early if we deleted a block to avoid iterator
   * weirdness/assert fails
   */
   if (retarget_jump(instr, tblock))
                  /* Detect the case where the block ends either with:
   * - A single unconditional jump to the next block.
   * - Two jump instructions with opposite conditions, and one of the
   *   them jumps to the next block.
   * We can remove the one that jumps to the next block in either case.
   */
   if (list_is_empty(&block->instr_list))
            struct ir3_instruction *jumps[2] = {NULL, NULL};
   jumps[0] =
         if (!list_is_singular(&block->instr_list))
                  if (jumps[0]->opc == OPC_JUMP)
         else if (jumps[0]->opc != OPC_B || !jumps[1] || jumps[1]->opc != OPC_B)
            for (unsigned i = 0; i < 2; i++) {
                     struct ir3_block *tblock = jumps[i]->cat0.target;
   if (&tblock->node == block->node.next) {
      list_delinit(&jumps[i]->node);
   progress = true;
                        }
      static void
   resolve_jumps(struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list)
      foreach_instr (instr, &block->instr_list)
      if (is_flow(instr) && instr->cat0.target) {
                     }
      static void
   mark_jp(struct ir3_block *block)
   {
      /* We only call this on the end block (in kill_sched) or after retargeting
   * all jumps to empty blocks (in mark_xvergence_points) so there's no need to
   * worry about empty blocks.
   */
            struct ir3_instruction *target =
            }
      /* Mark points where control flow converges or diverges.
   *
   * Divergence points could actually be re-convergence points where
   * "parked" threads are recoverged with threads that took the opposite
   * path last time around.  Possibly it is easier to think of (jp) as
   * "the execution mask might have changed".
   */
   static void
   mark_xvergence_points(struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list) {
      /* We need to insert (jp) if an entry in the "branch stack" is created for
   * our block. This happens if there is a predecessor to our block that may
   * fallthrough to an earlier block in the physical CFG, either because it
   * ends in a non-uniform conditional branch or because there's a
   * fallthrough for an block in-between that also starts with (jp) and was
   * pushed on the branch stack already.
   */
   for (unsigned i = 0; i < block->predecessors_count; i++) {
               for (unsigned j = 0; j < ARRAY_SIZE(pred->physical_successors); j++) {
      if (pred->physical_successors[j] != NULL &&
                  /* If the predecessor just falls through to this block, we still
   * need to check if it "falls through" by jumping to the block. This
   * can happen if opt_jump fails and the block ends in two branches,
   * or if there's an empty if-statement (which currently can happen
   * with binning shaders after dead-code elimination) and the block
   * before ends with a conditional branch directly to this block.
   */
   if (pred->physical_successors[j] == block) {
      foreach_instr_rev (instr, &pred->instr_list) {
      if (!is_flow(instr))
         if (instr->cat0.target == block) {
      mark_jp(block);
                        }
      /* Insert the branch/jump instructions for flow control between blocks.
   * Initially this is done naively, without considering if the successor
   * block immediately follows the current block (ie. so no jump required),
   * but that is cleaned up in opt_jump().
   *
   * TODO what ensures that the last write to p0.x in a block is the
   * branch condition?  Have we been getting lucky all this time?
   */
   static void
   block_sched(struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list) {
      if (block->successors[1]) {
                     if (block->brtype == IR3_BRANCH_GETONE ||
      block->brtype == IR3_BRANCH_SHPS) {
   /* getone/shps can't be inverted, and it wouldn't even make sense
   * to follow it with an inverted branch, so follow it by an
   * unconditional branch.
   */
   assert(!block->condition);
   if (block->brtype == IR3_BRANCH_GETONE)
         else
                  br2 = ir3_JUMP(block);
                        /* create "else" branch first (since "then" block should
   * frequently/always end up being a fall-thru):
   */
   br1 = ir3_instr_create(block, OPC_B, 0, 1);
   ir3_src_create(br1, regid(REG_P0, 0), 0)->def =
                        /* "then" branch: */
   br2 = ir3_instr_create(block, OPC_B, 0, 1);
   ir3_src_create(br2, regid(REG_P0, 0), 0)->def =
                  switch (block->brtype) {
   case IR3_BRANCH_COND:
      br1->cat0.brtype = br2->cat0.brtype = BRANCH_PLAIN;
      case IR3_BRANCH_ALL:
      br1->cat0.brtype = BRANCH_ANY;
   br2->cat0.brtype = BRANCH_ALL;
      case IR3_BRANCH_ANY:
      br1->cat0.brtype = BRANCH_ALL;
   br2->cat0.brtype = BRANCH_ANY;
      case IR3_BRANCH_GETONE:
   case IR3_BRANCH_SHPS:
               } else if (block->successors[0]) {
                     jmp = ir3_JUMP(block);
            }
      /* Here we workaround the fact that kill doesn't actually kill the thread as
   * GL expects. The last instruction always needs to be an end instruction,
   * which means that if we're stuck in a loop where kill is the only way out,
   * then we may have to jump out to the end. kill may also have the d3d
   * semantics of converting the thread to a helper thread, rather than setting
   * the exec mask to 0, in which case the helper thread could get stuck in an
   * infinite loop.
   *
   * We do this late, both to give the scheduler the opportunity to reschedule
   * kill instructions earlier and to avoid having to create a separate basic
   * block.
   *
   * TODO: Assuming that the wavefront doesn't stop as soon as all threads are
   * killed, we might benefit by doing this more aggressively when the remaining
   * part of the program after the kill is large, since that would let us
   * skip over the instructions when there are no non-killed threads left.
   */
   static void
   kill_sched(struct ir3 *ir, struct ir3_shader_variant *so)
   {
      /* True if we know that this block will always eventually lead to the end
   * block:
   */
   bool always_ends = true;
   bool added = false;
   struct ir3_block *last_block =
            foreach_block_rev (block, &ir->block_list) {
      for (unsigned i = 0; i < 2 && block->successors[i]; i++) {
      if (block->successors[i]->start_ip <= block->end_ip)
               if (always_ends)
            foreach_instr_safe (instr, &block->instr_list) {
                     struct ir3_instruction *br = ir3_instr_create(block, OPC_B, 0, 1);
   ir3_src_create(br, instr->srcs[0]->num, instr->srcs[0]->flags)->wrmask =
                                                      if (added) {
      /* I'm not entirely sure how the branchstack works, but we probably
   * need to add at least one entry for the divergence which is resolved
   * at the end:
   */
            /* We don't update predecessors/successors, so we have to do this
   * manually:
   */
         }
      /* Insert nop's required to make this a legal/valid shader program: */
   static void
   nop_sched(struct ir3 *ir, struct ir3_shader_variant *so)
   {
      foreach_block (block, &ir->block_list) {
      struct ir3_instruction *last = NULL;
            /* remove all the instructions from the list, we'll be adding
   * them back in as we go
   */
   list_replace(&block->instr_list, &instr_list);
            foreach_instr_safe (instr, &instr_list) {
               /* NOTE: I think the nopN encoding works for a5xx and
   * probably a4xx, but not a3xx.  So far only tested on
                  if ((delay > 0) && (ir->compiler->gen >= 6) && last &&
      ((opc_cat(last->opc) == 2) || (opc_cat(last->opc) == 3)) &&
   (last->repeat == 0)) {
   /* the previous cat2/cat3 instruction can encode at most 3 nop's: */
   unsigned transfer = MIN2(delay, 3 - last->nop);
   last->nop += transfer;
               if ((delay > 0) && last && (last->opc == OPC_NOP)) {
      /* the previous nop can encode at most 5 repeats: */
   unsigned transfer = MIN2(delay, 5 - last->repeat);
   last->repeat += transfer;
               if (delay > 0) {
      assert(delay <= 6);
               list_addtail(&instr->node, &block->instr_list);
            }
      struct ir3_helper_block_data {
      /* Whether helper invocations may be used on any path starting at the
   * beginning of the block.
   */
            /* Whether helper invocations may be used by the end of the block. Branch
   * instructions are considered to be "between" blocks, because (eq) has to be
   * inserted after them in the successor blocks, so branch instructions using
   * helpers will result in uses_helpers_end = true for their block.
   */
      };
      /* Insert (eq) after the last instruction using the results of helper
   * invocations. Use a backwards dataflow analysis to determine at which points
   * in the program helper invocations are definitely never used, and then insert
   * (eq) at the point where we cross from a point where they may be used to a
   * point where they are never used.
   */
   static void
   helper_sched(struct ir3_legalize_ctx *ctx, struct ir3 *ir,
         {
               foreach_block (block, &ir->block_list) {
      struct ir3_helper_block_data *bd =
         foreach_instr (instr, &block->instr_list) {
      if (uses_helpers(instr)) {
      bd->uses_helpers_beginning = true;
   if (instr->opc != OPC_META_TEX_PREFETCH) {
      non_prefetch_helpers = true;
                  if (instr->opc == OPC_SHPE) {
      /* (eq) is not allowed in preambles, mark the whole preamble as
   * requiring helpers to avoid putting it there.
   */
   bd->uses_helpers_beginning = true;
                  if (block->brtype == IR3_BRANCH_ALL ||
      block->brtype == IR3_BRANCH_ANY ||
   block->brtype == IR3_BRANCH_GETONE) {
   bd->uses_helpers_beginning = true;
                           /* If only prefetches use helpers then we can disable them in the shader via
   * a register setting.
   */
   if (!non_prefetch_helpers) {
      so->prefetch_end_of_quad = true;
               bool progress;
   do {
      progress = false;
   foreach_block_rev (block, &ir->block_list) {
                              for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
   struct ir3_helper_block_data *pred_bd = pred->data;
   if (!pred_bd->uses_helpers_end) {
         }
   if (!pred_bd->uses_helpers_beginning) {
      pred_bd->uses_helpers_beginning = true;
                        /* Now, we need to determine the points where helper invocations become
   * unused.
   */
   foreach_block (block, &ir->block_list) {
      struct ir3_helper_block_data *bd = block->data;
   if (bd->uses_helpers_end)
            /* We need to check the predecessors because of situations with critical
   * edges like this that can occur after optimizing jumps:
   *
   *    br p0.x, #endif
   *    ...
   *    sam ...
   *    ...
   *    endif:
   *    ...
   *    end
   *
   * The endif block will have uses_helpers_beginning = false and
   * uses_helpers_end = false, but because we jump to there from the
   * beginning of the if where uses_helpers_end = true, we still want to
   * add an (eq) at the beginning of the block:
   *
   *    br p0.x, #endif
   *    ...
   *    sam ...
   *    (eq)nop
   *    ...
   *    endif:
   *    (eq)nop
   *    ...
   *    end
   *
   * This an extra nop in the case where the branch isn't taken, but that's
   * probably preferable to adding an extra jump instruction which is what
   * would happen if we ran this pass before optimizing jumps:
   *
   *    br p0.x, #else
   *    ...
   *    sam ...
   *    (eq)nop
   *    ...
   *    jump #endif
   *    else:
   *    (eq)nop
   *    endif:
   *    ...
   *    end
   *
   * We also need this to make sure we insert (eq) after branches which use
   * helper invocations.
   */
   bool pred_uses_helpers = bd->uses_helpers_beginning;
   for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
   struct ir3_helper_block_data *pred_bd = pred->data;
   if (pred_bd->uses_helpers_end) {
      pred_uses_helpers = true;
                  if (!pred_uses_helpers)
            /* The last use of helpers is somewhere between the beginning and the
   * end. first_instr will be the first instruction where helpers are no
   * longer required, or NULL if helpers are not required just at the end.
   */
   struct ir3_instruction *first_instr = NULL;
   foreach_instr_rev (instr, &block->instr_list) {
      /* Skip prefetches because they actually execute before the block
   * starts and at this stage they aren't guaranteed to be at the start
   * of the block.
   */
   if (uses_helpers(instr) && instr->opc != OPC_META_TEX_PREFETCH)
                     bool killed = false;
   bool expensive_instruction_in_block = false;
   if (first_instr) {
      foreach_instr_from (instr, first_instr, &block->instr_list) {
      /* If there's already a nop, we don't have to worry about whether to
   * insert one.
   */
   if (instr->opc == OPC_NOP) {
      instr->flags |= IR3_INSTR_EQ;
                     /* ALU and SFU instructions probably aren't going to benefit much
   * from killing helper invocations, because they complete at least
   * an entire quad in a cycle and don't access any quad-divergent
   * memory, so delay emitting (eq) in the hopes that we find a nop
   * afterwards.
   */
                  expensive_instruction_in_block = true;
                  /* If this block isn't the last block before the end instruction, assume
   * that there may be expensive instructions in later blocks so it's worth
   * it to insert a nop.
   */
   if (!killed && (expensive_instruction_in_block ||
            struct ir3_instruction *nop = ir3_NOP(block);
   nop->flags |= IR3_INSTR_EQ;
   if (first_instr)
            }
      bool
   ir3_legalize(struct ir3 *ir, struct ir3_shader_variant *so, int *max_bary)
   {
      struct ir3_legalize_ctx *ctx = rzalloc(ir, struct ir3_legalize_ctx);
   bool mergedregs = so->mergedregs;
            ctx->so = so;
   ctx->max_bary = -1;
   ctx->compiler = ir->compiler;
            /* allocate per-block data: */
   foreach_block (block, &ir->block_list) {
      struct ir3_legalize_block_data *bd =
            regmask_init(&bd->state.needs_ss_war, mergedregs);
   regmask_init(&bd->state.needs_ss, mergedregs);
                        /* We may have failed to pull all input loads into the first block.
   * In such case at the moment we aren't able to find a better place
   * to for (ei) than the end of the program.
   * a5xx and a6xx do automatically release varying storage at the end.
   */
   ctx->early_input_release = true;
   struct ir3_block *start_block = ir3_after_preamble(ir);
   foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      if (is_input(instr)) {
      ctx->has_inputs = true;
   if (block != start_block) {
      ctx->early_input_release = false;
                                 /* process each block: */
   do {
      progress = false;
   foreach_block (block, &ir->block_list) {
                              block_sched(ir);
   if (so->type == MESA_SHADER_FRAGMENT)
            foreach_block (block, &ir->block_list) {
                  foreach_block (block, &ir->block_list) {
      if (block->brtype == IR3_BRANCH_GETONE) {
      apply_push_consts_load_macro(ctx, block->successors[0]);
                           while (opt_jump(ir))
            /* TODO: does (eq) exist before a6xx? */
   if (so->type == MESA_SHADER_FRAGMENT && so->need_pixlod &&
      so->compiler->gen >= 6)
         ir3_count_instructions(ir);
                                 }
