   /*
   * Copyright (C) 2019 Google, Inc.
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
      #include "util/dag.h"
   #include "util/u_math.h"
      #include "ir3.h"
   #include "ir3_compiler.h"
   #include "ir3_context.h"
      #ifdef DEBUG
   #define SCHED_DEBUG (ir3_shader_debug & IR3_DBG_SCHEDMSGS)
   #else
   #define SCHED_DEBUG 0
   #endif
   #define d(fmt, ...)                                                            \
      do {                                                                        \
      if (SCHED_DEBUG) {                                                       \
                  #define di(instr, fmt, ...)                                                    \
      do {                                                                        \
      if (SCHED_DEBUG) {                                                       \
      struct log_stream *stream = mesa_log_streami();                       \
   mesa_log_stream_printf(stream, "PSCHED: " fmt ": ", ##__VA_ARGS__);   \
   ir3_print_instr_stream(stream, instr);                                \
               /*
   * Post RA Instruction Scheduling
   */
      struct ir3_postsched_ctx {
                        void *mem_ctx;
   struct ir3_block *block; /* the current block */
                              int ss_delay;
      };
      struct ir3_postsched_node {
      struct dag_node dag; /* must be first for util_dynarray_foreach */
   struct ir3_instruction *instr;
                              unsigned delay;
      };
      #define foreach_sched_node(__n, __list)                                        \
            static bool
   has_sy_src(struct ir3_instruction *instr)
   {
      struct ir3_postsched_node *node = instr->data;
      }
      static bool
   has_ss_src(struct ir3_instruction *instr)
   {
      struct ir3_postsched_node *node = instr->data;
      }
      static void
   sched_dag_validate_cb(const struct dag_node *node, void *data)
   {
                  }
      static void
   schedule(struct ir3_postsched_ctx *ctx, struct ir3_instruction *instr)
   {
               /* remove from unscheduled_list:
   */
                                                /* We insert any nop's needed to get to earliest_ip, then advance
   * delay_cycles by scheduling the instruction.
   */
            util_dynarray_foreach (&n->dag.edges, struct dag_edge, edge) {
      unsigned delay = (unsigned)(uintptr_t)edge->data;
   struct ir3_postsched_node *child =
                                       if (is_meta(instr) && (instr->opc != OPC_META_TEX_PREFETCH))
            if (is_ss_producer(instr)) {
         } else if (has_ss_src(instr)) {
         } else if (ctx->ss_delay > 0) {
                  if (is_sy_producer(instr)) {
         } else if (has_sy_src(instr)) {
         } else if (ctx->sy_delay > 0) {
            }
      static void
   dump_state(struct ir3_postsched_ctx *ctx)
   {
      if (!SCHED_DEBUG)
            foreach_sched_node (n, &ctx->dag->heads) {
               util_dynarray_foreach (&n->dag.edges, struct dag_edge, edge) {
                              }
      static unsigned
   node_delay(struct ir3_postsched_ctx *ctx, struct ir3_postsched_node *n)
   {
         }
      static unsigned
   node_delay_soft(struct ir3_postsched_ctx *ctx, struct ir3_postsched_node *n)
   {
               /* This takes into account that as when we schedule multiple tex or sfu, the
   * first user has to wait for all of them to complete.
   */
   if (n->has_ss_src)
         if (n->has_sy_src)
               }
      /* find instruction to schedule: */
   static struct ir3_instruction *
   choose_instr(struct ir3_postsched_ctx *ctx)
   {
                        foreach_sched_node (n, &ctx->dag->heads) {
      if (!is_meta(n->instr))
            if (!chosen || (chosen->max_delay < n->max_delay))
               if (chosen) {
      di(chosen->instr, "prio: chose (meta)");
               /* Try to schedule inputs with a higher priority, if possible, as
   * the last bary.f unlocks varying storage to unblock more VS
   * warps.
   */
   foreach_sched_node (n, &ctx->dag->heads) {
      if (!is_input(n->instr))
            if (!chosen || (chosen->max_delay < n->max_delay))
               if (chosen) {
      di(chosen->instr, "prio: chose (input)");
               /* Next prioritize discards: */
   foreach_sched_node (n, &ctx->dag->heads) {
               if (d > 0)
            if (!is_kill_or_demote(n->instr))
            if (!chosen || (chosen->max_delay < n->max_delay))
               if (chosen) {
      di(chosen->instr, "csp: chose (kill, hard ready)");
               /* Next prioritize expensive instructions: */
   foreach_sched_node (n, &ctx->dag->heads) {
               if (d > 0)
            if (!(is_ss_producer(n->instr) || is_sy_producer(n->instr)))
            if (!chosen || (chosen->max_delay < n->max_delay))
               if (chosen) {
      di(chosen->instr, "csp: chose (sfu/tex, soft ready)");
               /* Next try to find a ready leader w/ soft delay (ie. including extra
   * delay for things like tex fetch which can be synchronized w/ sync
   * bit (but we probably do want to schedule some other instructions
   * while we wait). We also allow a small amount of nops, to prefer now-nops
   * over future-nops up to a point, as that gives better results.
   */
   unsigned chosen_delay = 0;
   foreach_sched_node (n, &ctx->dag->heads) {
               if (d > 3)
            if (!chosen || d < chosen_delay) {
      chosen = n;
   chosen_delay = d;
               if (d > chosen_delay)
            if (chosen->max_delay < n->max_delay) {
      chosen = n;
                  if (chosen) {
      di(chosen->instr, "csp: chose (soft ready)");
               /* Next try to find a ready leader that can be scheduled without nop's,
   * which in the case of things that need (sy)/(ss) could result in
   * stalls.. but we've already decided there is not a better option.
   */
   foreach_sched_node (n, &ctx->dag->heads) {
               if (d > 0)
            if (!chosen || (chosen->max_delay < n->max_delay))
               if (chosen) {
      di(chosen->instr, "csp: chose (hard ready)");
               /* Otherwise choose leader with maximum cost:
   */
   foreach_sched_node (n, &ctx->dag->heads) {
      if (!chosen || chosen->max_delay < n->max_delay)
               if (chosen) {
      di(chosen->instr, "csp: chose (leader)");
                  }
      struct ir3_postsched_deps_state {
                                 /* Track the mapping between sched node (instruction) that last
   * wrote a given register (in whichever direction we are iterating
   * the block)
   *
   * Note, this table is twice as big as the # of regs, to deal with
   * half-precision regs.  The approach differs depending on whether
   * the half and full precision register files are "merged" (conflict,
   * ie. a6xx+) in which case we consider each full precision dep
   * as two half-precision dependencies, vs older separate (non-
   * conflicting) in which case the first half of the table is used
   * for full precision and 2nd half for half-precision.
   */
   struct ir3_postsched_node *regs[2 * 256];
      };
      /* bounds checking read/write accessors, since OoB access to stuff on
   * the stack is gonna cause a bad day.
   */
   #define dep_reg(state, idx)                                                    \
      *({                                                                         \
      assert((idx) < ARRAY_SIZE((state)->regs));                               \
            static void
   add_dep(struct ir3_postsched_deps_state *state,
         struct ir3_postsched_node *before, struct ir3_postsched_node *after,
   {
      if (!before || !after)
                     if (state->direction == F) {
         } else {
            }
      static void
   add_single_reg_dep(struct ir3_postsched_deps_state *state,
               {
               unsigned d = 0;
   if (src_n >= 0 && dep && state->direction == F) {
      /* get the dst_n this corresponds to */
   unsigned dst_n = state->dst_n[num];
   unsigned d_soft = ir3_delayslots(dep->instr, node->instr, src_n, true);
   d = ir3_delayslots_with_repeat(dep->instr, node->instr, dst_n, src_n);
   node->delay = MAX2(node->delay, d_soft);
   if (is_sy_producer(dep->instr))
         if (is_ss_producer(dep->instr))
               add_dep(state, dep, node, d);
   if (src_n < 0) {
      dep_reg(state, num) = node;
         }
      /* This is where we handled full vs half-precision, and potential conflicts
   * between half and full precision that result in additional dependencies.
   * The 'reg' arg is really just to know half vs full precision.
   *
   * If src_n is positive, then this adds a dependency on a source register, and
   * src_n is the index passed into ir3_delayslots() for calculating the delay:
   * it corresponds to node->instr->srcs[src_n]. If src_n is negative, then
   * this is for the destination register corresponding to dst_n.
   */
   static void
   add_reg_dep(struct ir3_postsched_deps_state *state,
               {
      if (state->merged) {
      /* Make sure that special registers like a0.x that are written as
   * half-registers don't alias random full registers by pretending that
   * they're full registers:
   */
   if ((reg->flags & IR3_REG_HALF) && !is_reg_special(reg)) {
      /* single conflict in half-reg space: */
      } else {
      /* two conflicts in half-reg space: */
   add_single_reg_dep(state, node, 2 * num + 0, src_n, dst_n);
         } else {
      if (reg->flags & IR3_REG_HALF)
               }
      static void
   calculate_deps(struct ir3_postsched_deps_state *state,
         {
      /* Add dependencies on instructions that previously (or next,
   * in the reverse direction) wrote any of our src registers:
   */
   foreach_src_n (reg, i, node->instr) {
      if (reg->flags & (IR3_REG_CONST | IR3_REG_IMMED))
            if (reg->flags & IR3_REG_RELATIV) {
      /* mark entire array as read: */
   for (unsigned j = 0; j < reg->size; j++) {
            } else {
      assert(reg->wrmask >= 1);
   u_foreach_bit (b, reg->wrmask) {
                        /* And then after we update the state for what this instruction
   * wrote:
   */
   foreach_dst_n (reg, i, node->instr) {
      if (reg->wrmask == 0)
         if (reg->flags & IR3_REG_RELATIV) {
      /* mark the entire array as written: */
   for (unsigned j = 0; j < reg->size; j++) {
            } else {
      assert(reg->wrmask >= 1);
   u_foreach_bit (b, reg->wrmask) {
                  }
      static void
   calculate_forward_deps(struct ir3_postsched_ctx *ctx)
   {
      struct ir3_postsched_deps_state state = {
      .ctx = ctx,
   .direction = F,
               foreach_instr (instr, &ctx->unscheduled_list) {
            }
      static void
   calculate_reverse_deps(struct ir3_postsched_ctx *ctx)
   {
      struct ir3_postsched_deps_state state = {
      .ctx = ctx,
   .direction = R,
               foreach_instr_rev (instr, &ctx->unscheduled_list) {
            }
      static void
   sched_node_init(struct ir3_postsched_ctx *ctx, struct ir3_instruction *instr)
   {
      struct ir3_postsched_node *n =
                     n->instr = instr;
      }
      static void
   sched_dag_max_delay_cb(struct dag_node *node, void *state)
   {
      struct ir3_postsched_node *n = (struct ir3_postsched_node *)node;
            util_dynarray_foreach (&n->dag.edges, struct dag_edge, edge) {
      struct ir3_postsched_node *child =
                        }
      static void
   sched_dag_init(struct ir3_postsched_ctx *ctx)
   {
                        foreach_instr (instr, &ctx->unscheduled_list)
            calculate_forward_deps(ctx);
            /*
   * To avoid expensive texture fetches, etc, from being moved ahead
   * of kills, track the kills we've seen so far, so we can add an
   * extra dependency on them for tex/mem instructions
   */
   struct util_dynarray kills;
            /* The last bary.f with the (ei) flag must be scheduled before any kills,
   * or the hw gets angry. Keep track of inputs here so we can add the
   * false dep on the kill instruction.
   */
   struct util_dynarray inputs;
            /*
   * Normal srcs won't be in SSA at this point, those are dealt with in
   * calculate_forward_deps() and calculate_reverse_deps().  But we still
   * have the false-dep information in SSA form, so go ahead and add
   * dependencies for that here:
   */
   foreach_instr (instr, &ctx->unscheduled_list) {
               foreach_ssa_src_n (src, i, instr) {
                     /* we can end up with unused false-deps.. just skip them: */
                           /* don't consider dependencies in other blocks: */
                              if (is_input(instr)) {
         } else if (is_kill_or_demote(instr)) {
      util_dynarray_foreach (&inputs, struct ir3_instruction *, instrp) {
      struct ir3_instruction *input = *instrp;
   struct ir3_postsched_node *in = input->data;
      }
      } else if (is_tex(instr) || is_mem(instr)) {
      util_dynarray_foreach (&kills, struct ir3_instruction *, instrp) {
      struct ir3_instruction *kill = *instrp;
   struct ir3_postsched_node *kn = kill->data;
                              // TODO do we want to do this after reverse-dependencies?
      }
      static void
   sched_dag_destroy(struct ir3_postsched_ctx *ctx)
   {
      ralloc_free(ctx->mem_ctx);
   ctx->mem_ctx = NULL;
      }
      static void
   sched_block(struct ir3_postsched_ctx *ctx, struct ir3_block *block)
   {
      ctx->block = block;
   ctx->sy_delay = 0;
            /* move all instructions to the unscheduled list, and
   * empty the block's instruction list (to which we will
   * be inserting).
   */
   list_replace(&block->instr_list, &ctx->unscheduled_list);
            // TODO once we are using post-sched for everything we can
   // just not stick in NOP's prior to post-sched, and drop this.
   // for now keep this, since it makes post-sched optional:
   foreach_instr_safe (instr, &ctx->unscheduled_list) {
      switch (instr->opc) {
   case OPC_NOP:
   case OPC_B:
   case OPC_JUMP:
      list_delinit(&instr->node);
      default:
                              /* First schedule all meta:input instructions, followed by
   * tex-prefetch.  We want all of the instructions that load
   * values into registers before the shader starts to go
   * before any other instructions.  But in particular we
   * want inputs to come before prefetches.  This is because
   * a FS's bary_ij input may not actually be live in the
   * shader, but it should not be scheduled on top of any
   * other input (but can be overwritten by a tex prefetch)
   */
   foreach_instr_safe (instr, &ctx->unscheduled_list)
      if (instr->opc == OPC_META_INPUT)
         foreach_instr_safe (instr, &ctx->unscheduled_list)
      if (instr->opc == OPC_META_TEX_PREFETCH)
         foreach_instr_safe (instr, &ctx->unscheduled_list)
      if (instr->opc == OPC_PUSH_CONSTS_LOAD_MACRO)
         while (!list_is_empty(&ctx->unscheduled_list)) {
               unsigned delay = node_delay(ctx, instr->data);
                                    }
      static bool
   is_self_mov(struct ir3_instruction *instr)
   {
      if (!is_same_type_mov(instr))
            if (instr->dsts[0]->num != instr->srcs[0]->num)
            if (instr->dsts[0]->flags & IR3_REG_RELATIV)
            if (instr->cat1.round != ROUND_ZERO)
            if (instr->srcs[0]->flags &
      (IR3_REG_CONST | IR3_REG_IMMED | IR3_REG_RELATIV | IR3_REG_FNEG |
   IR3_REG_FABS | IR3_REG_SNEG | IR3_REG_SABS | IR3_REG_BNOT))
            }
      /* sometimes we end up w/ in-place mov's, ie. mov.u32u32 r1.y, r1.y
   * as a result of places were before RA we are not sure that it is
   * safe to eliminate.  We could eliminate these earlier, but sometimes
   * they are tangled up in false-dep's, etc, so it is easier just to
   * let them exist until after RA
   */
   static void
   cleanup_self_movs(struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list) {
      foreach_instr_safe (instr, &block->instr_list) {
      for (unsigned i = 0; i < instr->deps_count; i++) {
      if (instr->deps[i] && is_self_mov(instr->deps[i])) {
                     if (is_self_mov(instr))
            }
      bool
   ir3_postsched(struct ir3 *ir, struct ir3_shader_variant *v)
   {
      struct ir3_postsched_ctx ctx = {
      .ir = ir,
                        foreach_block (block, &ir->block_list) {
                     }
