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
      #include "util/dag.h"
   #include "util/u_math.h"
      #include "ir3.h"
   #include "ir3_compiler.h"
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
   mesa_log_stream_printf(stream, "SCHED: " fmt ": ", ##__VA_ARGS__);    \
   ir3_print_instr_stream(stream, instr);                                \
               /*
   * Instruction Scheduling:
   *
   * A block-level pre-RA scheduler, which works by creating a DAG of
   * instruction dependencies, and heuristically picking a DAG head
   * (instruction with no unscheduled dependencies).
   *
   * Where possible, it tries to pick instructions that avoid nop delay
   * slots, but it will prefer to pick instructions that reduce (or do
   * not increase) the number of live values.
   *
   * If the only possible choices are instructions that increase the
   * number of live values, it will try to pick the one with the earliest
   * consumer (based on pre-sched program order).
   *
   * There are a few special cases that need to be handled, since sched
   * is currently independent of register allocation.  Usages of address
   * register (a0.x) or predicate register (p0.x) must be serialized.  Ie.
   * if you have two pairs of instructions that write the same special
   * register and then read it, then those pairs cannot be interleaved.
   * To solve this, when we are in such a scheduling "critical section",
   * and we encounter a conflicting write to a special register, we try
   * to schedule any remaining instructions that use that value first.
   *
   * TODO we can detect too-large live_values here.. would be a good place
   * to "spill" cheap things, like move from uniform/immed.  (Constructing
   * list of ssa def consumers before sched pass would make this easier.
   * Also, in general it is general it might be best not to re-use load_immed
   * across blocks.
   *
   * TODO we can use (abs)/(neg) src modifiers in a lot of cases to reduce
   * the # of immediates in play (or at least that would help with
   * dEQP-GLES31.functional.ubo.random.all_per_block_buffers.*).. probably
   * do this in a nir pass that inserts fneg/etc?  The cp pass should fold
   * these into src modifiers..
   */
      struct ir3_sched_ctx {
      struct ir3_block *block; /* the current block */
            struct list_head unscheduled_list; /* unscheduled instructions */
   struct ir3_instruction *scheduled; /* last scheduled instr */
   struct ir3_instruction *addr0;     /* current a0.x user, if any */
   struct ir3_instruction *addr1;     /* current a1.x user, if any */
                     int remaining_kills;
                              int sy_delay;
            /* We order the scheduled (sy)/(ss) producers, and keep track of the
   * index of the last waited on instruction, so we can know which
   * instructions are still outstanding (and therefore would require us to
   * wait for all outstanding instructions before scheduling a use).
   */
   int sy_index, first_outstanding_sy_index;
      };
      struct ir3_sched_node {
      struct dag_node dag; /* must be first for util_dynarray_foreach */
            unsigned delay;
            unsigned sy_index;
            /* For ready instructions, the earliest possible ip that it could be
   * scheduled.
   */
            /* For instructions that are a meta:collect src, once we schedule
   * the first src of the collect, the entire vecN is live (at least
   * from the PoV of the first RA pass.. the 2nd scalar pass can fill
   * in some of the gaps, but often not all).  So we want to help out
   * RA, and realize that as soon as we schedule the first collect
   * src, there is no penalty to schedule the remainder (ie. they
   * don't make additional values live).  In fact we'd prefer to
   * schedule the rest ASAP to minimize the live range of the vecN.
   *
   * For instructions that are the src of a collect, we track the
   * corresponding collect, and mark them as partially live as soon
   * as any one of the src's is scheduled.
   */
   struct ir3_instruction *collect;
            /* Is this instruction a direct or indirect dependency for a kill?
   * If so, we should prioritize it when possible
   */
            /* This node represents a shader output.  A semi-common pattern in
   * shaders is something along the lines of:
   *
   *    fragcolor.w = 1.0
   *
   * Which we'd prefer to schedule as late as possible, since it
   * produces a live value that is never killed/consumed.  So detect
   * outputs up-front, and avoid scheduling them unless the reduce
   * register pressure (or at least are neutral)
   */
      };
      #define foreach_sched_node(__n, __list)                                        \
            static void sched_node_init(struct ir3_sched_ctx *ctx,
         static void sched_node_add_dep(struct ir3_instruction *instr,
            static bool
   is_scheduled(struct ir3_instruction *instr)
   {
         }
      /* check_src_cond() passing a ir3_sched_ctx. */
   static bool
   sched_check_src_cond(struct ir3_instruction *instr,
                     {
      foreach_ssa_src (src, instr) {
      /* meta:split/collect aren't real instructions, the thing that
   * we actually care about is *their* srcs
   */
   if ((src->opc == OPC_META_SPLIT) || (src->opc == OPC_META_COLLECT)) {
      if (sched_check_src_cond(src, cond, ctx))
      } else {
      if (cond(src, ctx))
                     }
      /* Is this a sy producer that hasn't been waited on yet? */
      static bool
   is_outstanding_sy(struct ir3_instruction *instr, struct ir3_sched_ctx *ctx)
   {
      if (!is_sy_producer(instr))
            /* The sched node is only valid within the same block, we cannot
   * really say anything about src's from other blocks
   */
   if (instr->block != ctx->block)
            struct ir3_sched_node *n = instr->data;
      }
      static bool
   is_outstanding_ss(struct ir3_instruction *instr, struct ir3_sched_ctx *ctx)
   {
      if (!is_ss_producer(instr))
            /* The sched node is only valid within the same block, we cannot
   * really say anything about src's from other blocks
   */
   if (instr->block != ctx->block)
            struct ir3_sched_node *n = instr->data;
      }
      static unsigned
   cycle_count(struct ir3_instruction *instr)
   {
      if (instr->opc == OPC_META_COLLECT) {
      /* Assume that only immed/const sources produce moves */
   unsigned n = 0;
   foreach_src (src, instr) {
      if (src->flags & (IR3_REG_IMMED | IR3_REG_CONST))
      }
      } else if (is_meta(instr)) {
         } else {
            }
      static void
   schedule(struct ir3_sched_ctx *ctx, struct ir3_instruction *instr)
   {
               /* remove from depth list:
   */
            if (writes_addr0(instr)) {
      assert(ctx->addr0 == NULL);
               if (writes_addr1(instr)) {
      assert(ctx->addr1 == NULL);
               if (writes_pred(instr)) {
      assert(ctx->pred == NULL);
                                 list_addtail(&instr->node, &instr->block->instr_list);
            if (is_kill_or_demote(instr)) {
      assert(ctx->remaining_kills > 0);
                        /* If this instruction is a meta:collect src, mark the remaining
   * collect srcs as partially live.
   */
   if (n->collect) {
      foreach_ssa_src (src, n->collect) {
      if (src->block != instr->block)
         struct ir3_sched_node *sn = src->data;
                           /* TODO: switch to "cycles". For now try to match ir3_delay. */
            /* We insert any nop's needed to get to earliest_ip, then advance
   * delay_cycles by scheduling the instruction.
   */
            util_dynarray_foreach (&n->dag.edges, struct dag_edge, edge) {
      unsigned delay = (unsigned)(uintptr_t)edge->data;
   struct ir3_sched_node *child =
                                       if (is_ss_producer(instr)) {
      ctx->ss_delay = soft_ss_delay(instr);
      } else if (!is_meta(instr) &&
            ctx->ss_delay = 0;
      } else if (ctx->ss_delay > 0) {
                  if (is_sy_producer(instr)) {
      /* NOTE that this isn't an attempt to hide texture fetch latency,
   * but an attempt to hide the cost of switching to another warp.
   * If we can, we'd like to try to schedule another texture fetch
   * before scheduling something that would sync.
   */
   ctx->sy_delay = soft_sy_delay(instr, ctx->block->shader);
   assert(ctx->remaining_tex > 0);
   ctx->remaining_tex--;
      } else if (!is_meta(instr) &&
            ctx->sy_delay = 0;
      } else if (ctx->sy_delay > 0) {
               }
      struct ir3_sched_notes {
      /* there is at least one kill which could be scheduled, except
   * for unscheduled bary.f's:
   */
   bool blocked_kill;
   /* there is at least one instruction that could be scheduled,
   * except for conflicting address/predicate register usage:
   */
      };
      static bool
   should_skip(struct ir3_sched_ctx *ctx, struct ir3_instruction *instr)
   {
      if (ctx->remaining_kills && (is_tex(instr) || is_mem(instr))) {
      /* avoid texture/memory access if we have unscheduled kills
   * that could make the expensive operation unnecessary.  By
   * definition, if there are remaining kills, and this instr
   * is not a dependency of a kill, there are other instructions
   * that we can choose from.
   */
   struct ir3_sched_node *n = instr->data;
   if (!n->kill_path)
                  }
      /* could an instruction be scheduled if specified ssa src was scheduled? */
   static bool
   could_sched(struct ir3_sched_ctx *ctx,
         {
      foreach_ssa_src (other_src, instr) {
      /* if dependency not scheduled, we aren't ready yet: */
   if ((src != other_src) && !is_scheduled(other_src)) {
                     /* Instructions not in the current block can never be scheduled.
   */
   if (instr->block != src->block)
               }
      /* Check if instruction is ok to schedule.  Make sure it is not blocked
   * by use of addr/predicate register, etc.
   */
   static bool
   check_instr(struct ir3_sched_ctx *ctx, struct ir3_sched_notes *notes,
         {
               if (instr == ctx->split) {
      /* Don't schedule instructions created by splitting a a0.x/a1.x/p0.x
   * write until another "normal" instruction has been scheduled.
   */
               if (should_skip(ctx, instr))
            /* For instructions that write address register we need to
   * make sure there is at least one instruction that uses the
   * addr value which is otherwise ready.
   *
   * NOTE if any instructions use pred register and have other
   * src args, we would need to do the same for writes_pred()..
   */
   if (writes_addr0(instr)) {
      struct ir3 *ir = instr->block->shader;
   bool ready = false;
   for (unsigned i = 0; (i < ir->a0_users_count) && !ready; i++) {
      struct ir3_instruction *indirect = ir->a0_users[i];
   if (!indirect)
         if (indirect->address->def != instr->dsts[0])
                     /* nothing could be scheduled, so keep looking: */
   if (!ready)
               if (writes_addr1(instr)) {
      struct ir3 *ir = instr->block->shader;
   bool ready = false;
   for (unsigned i = 0; (i < ir->a1_users_count) && !ready; i++) {
      struct ir3_instruction *indirect = ir->a1_users[i];
   if (!indirect)
         if (indirect->address->def != instr->dsts[0])
                     /* nothing could be scheduled, so keep looking: */
   if (!ready)
               /* if this is a write to address/predicate register, and that
   * register is currently in use, we need to defer until it is
   * free:
   */
   if (writes_addr0(instr) && ctx->addr0) {
      assert(ctx->addr0 != instr);
   notes->addr0_conflict = true;
               if (writes_addr1(instr) && ctx->addr1) {
      assert(ctx->addr1 != instr);
   notes->addr1_conflict = true;
               if (writes_pred(instr) && ctx->pred) {
      assert(ctx->pred != instr);
   notes->pred_conflict = true;
               /* if the instruction is a kill, we need to ensure *every*
   * bary.f is scheduled.  The hw seems unhappy if the thread
   * gets killed before the end-input (ei) flag is hit.
   *
   * We could do this by adding each bary.f instruction as
   * virtual ssa src for the kill instruction.  But we have
   * fixed length instr->srcs[].
   *
   * TODO we could handle this by false-deps now, probably.
   */
   if (is_kill_or_demote(instr)) {
               for (unsigned i = 0; i < ir->baryfs_count; i++) {
      struct ir3_instruction *baryf = ir->baryfs[i];
   if (baryf->flags & IR3_INSTR_UNUSED)
         if (!is_scheduled(baryf)) {
      notes->blocked_kill = true;
                        }
      /* Find the instr->ip of the closest use of an instruction, in
   * pre-sched order.  This isn't going to be the same as post-sched
   * order, but it is a reasonable approximation to limit scheduling
   * instructions *too* early.  This is mostly to prevent bad behavior
   * in cases where we have a large number of possible instructions
   * to choose, to avoid creating too much parallelism (ie. blowing
   * up register pressure)
   *
   * See
   * dEQP-GLES31.functional.atomic_counter.layout.reverse_offset.inc_dec.8_counters_5_calls_1_thread
   */
   static int
   nearest_use(struct ir3_instruction *instr)
   {
      unsigned nearest = ~0;
   foreach_ssa_use (use, instr)
      if (!is_scheduled(use))
         /* slight hack.. this heuristic tends to push bary.f's to later
   * in the shader, closer to their uses.  But we actually would
   * prefer to get these scheduled earlier, to unlock varying
   * storage for more VS jobs:
   */
   if (is_input(instr))
               }
      static bool
   is_only_nonscheduled_use(struct ir3_instruction *instr,
         {
      foreach_ssa_use (other_use, instr) {
      if (other_use != use && !is_scheduled(other_use))
                  }
      static unsigned
   new_regs(struct ir3_instruction *instr)
   {
               foreach_dst (dst, instr) {
      if (!is_dest_gpr(dst))
                        }
      /* find net change to live values if instruction were scheduled: */
   static int
   live_effect(struct ir3_instruction *instr)
   {
      struct ir3_sched_node *n = instr->data;
   int new_live =
      (n->partially_live || !instr->uses || instr->uses->entries == 0)
      ? 0
            /* if we schedule something that causes a vecN to be live,
   * then count all it's other components too:
   */
   if (n->collect)
            foreach_ssa_src_n (src, n, instr) {
      if (__is_false_dep(instr, n))
            if (instr->block != src->block)
            if (is_only_nonscheduled_use(src, instr))
                  }
      /* Determine if this is an instruction that we'd prefer not to schedule
   * yet, in order to avoid an (ss)/(sy) sync.  This is limited by the
   * ss_delay/sy_delay counters, ie. the more cycles it has been since
   * the last SFU/tex, the less costly a sync would be, and the number of
   * outstanding SFU/tex instructions to prevent a blowup in register pressure.
   */
   static bool
   should_defer(struct ir3_sched_ctx *ctx, struct ir3_instruction *instr)
   {
      if (ctx->ss_delay) {
      if (sched_check_src_cond(instr, is_outstanding_ss, ctx))
               /* We mostly just want to try to schedule another texture fetch
   * before scheduling something that would (sy) sync, so we can
   * limit this rule to cases where there are remaining texture
   * fetches
   */
   if (ctx->sy_delay && ctx->remaining_tex) {
      if (sched_check_src_cond(instr, is_outstanding_sy, ctx))
               /* Avoid scheduling too many outstanding texture or sfu instructions at
   * once by deferring further tex/SFU instructions. This both prevents
   * stalls when the queue of texture/sfu instructions becomes too large,
   * and prevents unacceptably large increases in register pressure from too
   * many outstanding texture instructions.
   */
   if (ctx->sy_index - ctx->first_outstanding_sy_index >= 8 && is_sy_producer(instr))
            if (ctx->ss_index - ctx->first_outstanding_ss_index >= 8 && is_ss_producer(instr))
               }
      static struct ir3_sched_node *choose_instr_inc(struct ir3_sched_ctx *ctx,
                  enum choose_instr_dec_rank {
      DEC_NEUTRAL,
   DEC_NEUTRAL_READY,
   DEC_FREED,
      };
      static const char *
   dec_rank_name(enum choose_instr_dec_rank rank)
   {
      switch (rank) {
   case DEC_NEUTRAL:
         case DEC_NEUTRAL_READY:
         case DEC_FREED:
         case DEC_FREED_READY:
         default:
            }
      static unsigned
   node_delay(struct ir3_sched_ctx *ctx, struct ir3_sched_node *n)
   {
         }
      /**
   * Chooses an instruction to schedule using the Goodman/Hsu (1988) CSR (Code
   * Scheduling for Register pressure) heuristic.
   *
   * Only handles the case of choosing instructions that reduce register pressure
   * or are even.
   */
   static struct ir3_sched_node *
   choose_instr_dec(struct ir3_sched_ctx *ctx, struct ir3_sched_notes *notes,
         {
      const char *mode = defer ? "-d" : "";
   struct ir3_sched_node *chosen = NULL;
            foreach_sched_node (n, &ctx->dag->heads) {
      if (defer && should_defer(ctx, n->instr))
                     int live = live_effect(n->instr);
   if (live > 0)
            if (!check_instr(ctx, notes, n->instr))
            enum choose_instr_dec_rank rank;
   if (live < 0) {
      /* Prioritize instrs which free up regs and can be scheduled with no
   * delay.
   */
   if (d == 0)
         else
      } else {
      /* Contra the paper, pick a leader with no effect on used regs.  This
   * may open up new opportunities, as otherwise a single-operand instr
   * consuming a value will tend to block finding freeing that value.
   * This had a massive effect on reducing spilling on V3D.
   *
   * XXX: Should this prioritize ready?
   */
   if (d == 0)
         else
               /* Prefer higher-ranked instructions, or in the case of a rank tie, the
   * highest latency-to-end-of-program instruction.
   */
   if (!chosen || rank > chosen_rank ||
      (rank == chosen_rank && chosen->max_delay < n->max_delay)) {
   chosen = n;
                  if (chosen) {
      di(chosen->instr, "dec%s: chose (%s)", mode, dec_rank_name(chosen_rank));
                  }
      enum choose_instr_inc_rank {
      INC_DISTANCE,
      };
      static const char *
   inc_rank_name(enum choose_instr_inc_rank rank)
   {
      switch (rank) {
   case INC_DISTANCE:
         case INC_DISTANCE_READY:
         default:
            }
      /**
   * When we can't choose an instruction that reduces register pressure or
   * is neutral, we end up here to try and pick the least bad option.
   */
   static struct ir3_sched_node *
   choose_instr_inc(struct ir3_sched_ctx *ctx, struct ir3_sched_notes *notes,
         {
      const char *mode = defer ? "-d" : "";
   struct ir3_sched_node *chosen = NULL;
            /*
   * From hear on out, we are picking something that increases
   * register pressure.  So try to pick something which will
   * be consumed soon:
   */
            /* Pick the max delay of the remaining ready set. */
   foreach_sched_node (n, &ctx->dag->heads) {
      if (avoid_output && n->output)
            if (defer && should_defer(ctx, n->instr))
            if (!check_instr(ctx, notes, n->instr))
                     enum choose_instr_inc_rank rank;
   if (d == 0)
         else
                     if (!chosen || rank > chosen_rank ||
      (rank == chosen_rank && distance < chosen_distance)) {
   chosen = n;
   chosen_distance = distance;
                  if (chosen) {
      di(chosen->instr, "inc%s: chose (%s)", mode, inc_rank_name(chosen_rank));
                  }
      /* Handles instruction selections for instructions we want to prioritize
   * even if csp/csr would not pick them.
   */
   static struct ir3_sched_node *
   choose_instr_prio(struct ir3_sched_ctx *ctx, struct ir3_sched_notes *notes)
   {
               foreach_sched_node (n, &ctx->dag->heads) {
      /*
   * - phi nodes and inputs must be scheduled first
   * - split should be scheduled first, so that the vector value is
   *   killed as soon as possible. RA cannot split up the vector and
   *   reuse components that have been killed until it's been killed.
   * - collect, on the other hand, should be treated as a "normal"
   *   instruction, and may add to register pressure if its sources are
   *   part of another vector or immediates.
   */
   if (!is_meta(n->instr) || n->instr->opc == OPC_META_COLLECT)
            if (!chosen || (chosen->max_delay < n->max_delay))
               if (chosen) {
      di(chosen->instr, "prio: chose (meta)");
                  }
      static void
   dump_state(struct ir3_sched_ctx *ctx)
   {
      if (!SCHED_DEBUG)
            foreach_sched_node (n, &ctx->dag->heads) {
      di(n->instr, "maxdel=%3d le=%d del=%u ", n->max_delay,
            util_dynarray_foreach (&n->dag.edges, struct dag_edge, edge) {
                        }
      /* find instruction to schedule: */
   static struct ir3_instruction *
   choose_instr(struct ir3_sched_ctx *ctx, struct ir3_sched_notes *notes)
   {
                        chosen = choose_instr_prio(ctx, notes);
   if (chosen)
            chosen = choose_instr_dec(ctx, notes, true);
   if (chosen)
            chosen = choose_instr_dec(ctx, notes, false);
   if (chosen)
            chosen = choose_instr_inc(ctx, notes, false, false);
   if (chosen)
               }
      static struct ir3_instruction *
   split_instr(struct ir3_sched_ctx *ctx, struct ir3_instruction *orig_instr)
   {
      struct ir3_instruction *new_instr = ir3_instr_clone(orig_instr);
   di(new_instr, "split instruction");
   sched_node_init(ctx, new_instr);
      }
      /* "spill" the address registers by remapping any unscheduled
   * instructions which depend on the current address register
   * to a clone of the instruction which wrote the address reg.
   */
   static struct ir3_instruction *
   split_addr(struct ir3_sched_ctx *ctx, struct ir3_instruction **addr,
         {
      struct ir3_instruction *new_addr = NULL;
                     for (i = 0; i < users_count; i++) {
               if (!indirect)
            /* skip instructions already scheduled: */
   if (is_scheduled(indirect))
            /* remap remaining instructions using current addr
   * to new addr:
   */
   if (indirect->address->def == (*addr)->dsts[0]) {
      if (!new_addr) {
      new_addr = split_instr(ctx, *addr);
   /* original addr is scheduled, but new one isn't: */
      }
   indirect->address->def = new_addr->dsts[0];
   /* don't need to remove old dag edge since old addr is
   * already scheduled:
   */
   sched_node_add_dep(indirect, new_addr, 0);
                  /* all remaining indirects remapped to new addr: */
               }
      /* "spill" the predicate register by remapping any unscheduled
   * instructions which depend on the current predicate register
   * to a clone of the instruction which wrote the address reg.
   */
   static struct ir3_instruction *
   split_pred(struct ir3_sched_ctx *ctx)
   {
      struct ir3 *ir;
   struct ir3_instruction *new_pred = NULL;
                              for (i = 0; i < ir->predicates_count; i++) {
               if (!predicated)
            /* skip instructions already scheduled: */
   if (is_scheduled(predicated))
            /* remap remaining instructions using current pred
   * to new pred:
   *
   * TODO is there ever a case when pred isn't first
   * (and only) src?
   */
   if (ssa(predicated->srcs[0]) == ctx->pred) {
      if (!new_pred) {
      new_pred = split_instr(ctx, ctx->pred);
   /* original pred is scheduled, but new one isn't: */
      }
   predicated->srcs[0]->def->instr = new_pred;
   /* don't need to remove old dag edge since old pred is
   * already scheduled:
   */
   sched_node_add_dep(predicated, new_pred, 0);
                  if (ctx->block->condition == ctx->pred) {
      if (!new_pred) {
      new_pred = split_instr(ctx, ctx->pred);
   /* original pred is scheduled, but new one isn't: */
      }
   ctx->block->condition = new_pred;
               /* all remaining predicated remapped to new pred: */
               }
      static void
   sched_node_init(struct ir3_sched_ctx *ctx, struct ir3_instruction *instr)
   {
                        n->instr = instr;
      }
      static void
   sched_node_add_dep(struct ir3_instruction *instr, struct ir3_instruction *src,
         {
      /* don't consider dependencies in other blocks: */
   if (src->block != instr->block)
            /* we could have false-dep's that end up unused: */
   if (src->flags & IR3_INSTR_UNUSED) {
      assert(__is_false_dep(instr, i));
               struct ir3_sched_node *n = instr->data;
            /* If src is consumed by a collect, track that to realize that once
   * any of the collect srcs are live, we should hurry up and schedule
   * the rest.
   */
   if (instr->opc == OPC_META_COLLECT)
            unsigned d_soft = ir3_delayslots(src, instr, i, true);
            /* delays from (ss) and (sy) are considered separately and more accurately in
   * the scheduling heuristic, so ignore it when calculating the ip of
   * instructions, but do consider it when prioritizing which instructions to
   * schedule.
   */
               }
      static void
   mark_kill_path(struct ir3_instruction *instr)
   {
               if (n->kill_path) {
                           foreach_ssa_src (src, instr) {
      if (src->block != instr->block)
               }
      /* Is it an output? */
   static bool
   is_output_collect(struct ir3_instruction *instr)
   {
      if (instr->opc != OPC_META_COLLECT)
            foreach_ssa_use (use, instr) {
      if (use->opc != OPC_END && use->opc != OPC_CHMASK)
                  }
      /* Is it's only use as output? */
   static bool
   is_output_only(struct ir3_instruction *instr)
   {
      foreach_ssa_use (use, instr)
      if (!is_output_collect(use))
            }
      static void
   sched_node_add_deps(struct ir3_instruction *instr)
   {
      /* There's nothing to do for phi nodes, since they always go first. And
   * phi nodes can reference sources later in the same block, so handling
   * sources is not only unnecessary but could cause problems.
   */
   if (instr->opc == OPC_META_PHI)
            /* Since foreach_ssa_src() already handles false-dep's we can construct
   * the DAG easily in a single pass.
   */
   foreach_ssa_src_n (src, i, instr) {
                  /* NOTE that all inputs must be scheduled before a kill, so
   * mark these to be prioritized as well:
   */
   if (is_kill_or_demote(instr) || is_input(instr)) {
                  if (is_output_only(instr)) {
      struct ir3_sched_node *n = instr->data;
         }
      static void
   sched_dag_max_delay_cb(struct dag_node *node, void *state)
   {
      struct ir3_sched_node *n = (struct ir3_sched_node *)node;
            util_dynarray_foreach (&n->dag.edges, struct dag_edge, edge) {
      struct ir3_sched_node *child = (struct ir3_sched_node *)edge->child;
                  }
      static void
   sched_dag_validate_cb(const struct dag_node *node, void *data)
   {
                  }
      static void
   sched_dag_init(struct ir3_sched_ctx *ctx)
   {
               foreach_instr (instr, &ctx->unscheduled_list)
                     foreach_instr (instr, &ctx->unscheduled_list)
               }
      static void
   sched_dag_destroy(struct ir3_sched_ctx *ctx)
   {
      ralloc_free(ctx->dag);
      }
      static void
   sched_block(struct ir3_sched_ctx *ctx, struct ir3_block *block)
   {
               /* addr/pred writes are per-block: */
   ctx->addr0 = NULL;
   ctx->addr1 = NULL;
   ctx->pred = NULL;
   ctx->sy_delay = 0;
   ctx->ss_delay = 0;
   ctx->sy_index = ctx->first_outstanding_sy_index = 0;
            /* move all instructions to the unscheduled list, and
   * empty the block's instruction list (to which we will
   * be inserting).
   */
   list_replace(&block->instr_list, &ctx->unscheduled_list);
                     ctx->remaining_kills = 0;
   ctx->remaining_tex = 0;
   foreach_instr_safe (instr, &ctx->unscheduled_list) {
      if (is_kill_or_demote(instr))
         if (is_sy_producer(instr))
               /* First schedule all meta:input and meta:phi instructions, followed by
   * tex-prefetch.  We want all of the instructions that load values into
   * registers before the shader starts to go before any other instructions.
   * But in particular we want inputs to come before prefetches.  This is
   * because a FS's bary_ij input may not actually be live in the shader,
   * but it should not be scheduled on top of any other input (but can be
   * overwritten by a tex prefetch)
   *
   * Note: Because the first block cannot have predecessors, meta:input and
   * meta:phi cannot exist in the same block.
   */
   foreach_instr_safe (instr, &ctx->unscheduled_list)
      if (instr->opc == OPC_META_INPUT || instr->opc == OPC_META_PHI)
         foreach_instr_safe (instr, &ctx->unscheduled_list)
      if (instr->opc == OPC_META_TEX_PREFETCH)
         foreach_instr_safe (instr, &ctx->unscheduled_list)
      if (instr->opc == OPC_PUSH_CONSTS_LOAD_MACRO)
         while (!list_is_empty(&ctx->unscheduled_list)) {
      struct ir3_sched_notes notes = {0};
            instr = choose_instr(ctx, &notes);
   if (instr) {
                                       /* Since we've scheduled a "real" instruction, we can now
   * schedule any split instruction created by the scheduler again.
   */
      } else {
                     /* nothing available to schedule.. if we are blocked on
   * address/predicate register conflict, then break the
   * deadlock by cloning the instruction that wrote that
   * reg:
   */
   if (notes.addr0_conflict) {
      new_instr =
      } else if (notes.addr1_conflict) {
      new_instr =
      } else if (notes.pred_conflict) {
         } else {
      d("unscheduled_list:");
   foreach_instr (instr, &ctx->unscheduled_list)
         assert(0);
   ctx->error = true;
               if (new_instr) {
      list_delinit(&new_instr->node);
               /* If we produced a new instruction, do not schedule it next to
   * guarantee progress.
   */
                     }
      int
   ir3_sched(struct ir3 *ir)
   {
               foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
                     ir3_count_instructions(ir);
   ir3_clear_mark(ir);
            foreach_block (block, &ir->block_list) {
                                       }
      static unsigned
   get_array_id(struct ir3_instruction *instr)
   {
      /* The expectation is that there is only a single array
   * src or dst, ir3_cp should enforce this.
            foreach_dst (dst, instr)
      if (dst->flags & IR3_REG_ARRAY)
      foreach_src (src, instr)
      if (src->flags & IR3_REG_ARRAY)
            }
      /* does instruction 'prior' need to be scheduled before 'instr'? */
   static bool
   depends_on(struct ir3_instruction *instr, struct ir3_instruction *prior)
   {
      /* TODO for dependencies that are related to a specific object, ie
   * a specific SSBO/image/array, we could relax this constraint to
   * make accesses to unrelated objects not depend on each other (at
   * least as long as not declared coherent)
   */
   if (((instr->barrier_class & IR3_BARRIER_EVERYTHING) &&
      prior->barrier_class) ||
   ((prior->barrier_class & IR3_BARRIER_EVERYTHING) &&
   instr->barrier_class))
         if (instr->barrier_class & prior->barrier_conflict) {
      if (!(instr->barrier_class &
            /* if only array barrier, then we can further limit false-deps
   * by considering the array-id, ie reads/writes to different
   * arrays do not depend on each other (no aliasing)
   */
   if (get_array_id(instr) != get_array_id(prior)) {
                                    }
      static void
   add_barrier_deps(struct ir3_block *block, struct ir3_instruction *instr)
   {
      struct list_head *prev = instr->node.prev;
            /* add dependencies on previous instructions that must be scheduled
   * prior to the current instruction
   */
   while (prev != &block->instr_list) {
      struct ir3_instruction *pi =
                     if (is_meta(pi))
            if (instr->barrier_class == pi->barrier_class) {
      ir3_instr_add_dep(instr, pi);
               if (depends_on(instr, pi))
               /* add dependencies on this instruction to following instructions
   * that must be scheduled after the current instruction:
   */
   while (next != &block->instr_list) {
      struct ir3_instruction *ni =
                     if (is_meta(ni))
            if (instr->barrier_class == ni->barrier_class) {
      ir3_instr_add_dep(ni, instr);
               if (depends_on(ni, instr))
         }
      /* before scheduling a block, we need to add any necessary false-dependencies
   * to ensure that:
   *
   *  (1) barriers are scheduled in the right order wrt instructions related
   *      to the barrier
   *
   *  (2) reads that come before a write actually get scheduled before the
   *      write
   */
   bool
   ir3_sched_add_deps(struct ir3 *ir)
   {
               foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      if (instr->barrier_class) {
      add_barrier_deps(block, instr);
                        }
