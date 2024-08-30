   /*
   * Copyright 2023 Alyssa Rosenzweig
   * Copyright 2022 Collabora Ltd.
   * SPDX-License-Identifier: MIT
   */
      /* Bottom-up local scheduler to reduce register pressure */
      #include "util/dag.h"
   #include "agx_compiler.h"
   #include "agx_opcodes.h"
      struct sched_ctx {
      /* Dependency graph */
            /* Live set */
      };
      struct sched_node {
               /* Instruction this node represents */
      };
      static void
   add_dep(struct sched_node *a, struct sched_node *b)
   {
               if (a && b)
      }
      static void
   serialize(struct sched_node *a, struct sched_node **b)
   {
      add_dep(a, *b);
      }
      static struct dag *
   create_dag(agx_context *ctx, agx_block *block, void *memctx)
   {
               struct sched_node **last_write =
         struct sched_node *coverage = NULL;
            /* Last memory load, to serialize stores against */
            /* Last memory store, to serialize loads and stores against */
            agx_foreach_instr_in_block(block, I) {
      /* Don't touch control flow */
   if (instr_after_logical_end(I))
            struct sched_node *node = rzalloc(memctx, struct sched_node);
   node->instr = I;
            /* Reads depend on writes, no other hazards in SSA */
   agx_foreach_ssa_src(I, s) {
                  agx_foreach_ssa_dest(I, d) {
      assert(I->dest[d].value < ctx->alloc);
               /* Classify the instruction and add dependencies according to the class */
   enum agx_schedule_class dep = agx_opcodes_info[I->op].schedule_class;
            bool barrier = dep == AGX_SCHEDULE_CLASS_BARRIER;
   bool discards =
            if (dep == AGX_SCHEDULE_CLASS_STORE)
         else if (dep == AGX_SCHEDULE_CLASS_ATOMIC || barrier)
            if (dep == AGX_SCHEDULE_CLASS_LOAD || dep == AGX_SCHEDULE_CLASS_STORE ||
                  if (dep == AGX_SCHEDULE_CLASS_COVERAGE || barrier)
            /* Make sure side effects happen before a discard */
   if (discards)
            if (dep == AGX_SCHEDULE_CLASS_PRELOAD)
         else
                           }
      /*
   * Calculate the change in register pressure from scheduling a given
   * instruction. Equivalently, calculate the difference in the number of live
   * registers before and after the instruction, given the live set after the
   * instruction. This calculation follows immediately from the dataflow
   * definition of liveness:
   *
   *      live_in = (live_out - KILL) + GEN
   */
   static signed
   calculate_pressure_delta(agx_instr *I, BITSET_WORD *live)
   {
               /* Destinations must be unique */
   agx_foreach_ssa_dest(I, d) {
      if (BITSET_TEST(live, I->dest[d].value))
               agx_foreach_ssa_src(I, src) {
      /* Filter duplicates */
            for (unsigned i = 0; i < src; ++i) {
      if (agx_is_equiv(I->src[i], I->src[src])) {
      dupe = true;
                  if (!dupe && !BITSET_TEST(live, I->src[src].value))
                  }
      /*
   * Choose the next instruction, bottom-up. For now we use a simple greedy
   * heuristic: choose the instruction that has the best effect on liveness, while
   * hoisting sample_mask.
   */
   static struct sched_node *
   choose_instr(struct sched_ctx *s)
   {
      int32_t min_delta = INT32_MAX;
            list_for_each_entry(struct sched_node, n, &s->dag->heads, dag.link) {
      /* Heuristic: hoist sample_mask/zs_emit. This allows depth/stencil tests
   * to run earlier, and potentially to discard the entire quad invocation
   * earlier, reducing how much redundant fragment shader we run.
   *
   * Since we schedule backwards, we make that happen by only choosing
   * sample_mask when all other instructions have been exhausted.
   */
   if (n->instr->op == AGX_OPCODE_SAMPLE_MASK ||
               if (!best) {
      best = n;
                                    if (delta < min_delta) {
      best = n;
                     }
      static void
   pressure_schedule_block(agx_context *ctx, agx_block *block, struct sched_ctx *s)
   {
      /* off by a constant, that's ok */
   signed pressure = 0;
   signed orig_max_pressure = 0;
            memcpy(s->live, block->live_out,
            agx_foreach_instr_in_block_rev(block, I) {
      pressure += calculate_pressure_delta(I, s->live);
   orig_max_pressure = MAX2(pressure, orig_max_pressure);
   agx_liveness_ins_update(s->live, I);
               memcpy(s->live, block->live_out,
            /* off by a constant, that's ok */
   signed max_pressure = 0;
            struct sched_node **schedule = calloc(nr_ins, sizeof(struct sched_node *));
            while (!list_is_empty(&s->dag->heads)) {
      struct sched_node *node = choose_instr(s);
   pressure += calculate_pressure_delta(node->instr, s->live);
   max_pressure = MAX2(pressure, max_pressure);
            schedule[nr_ins++] = node;
               /* Bail if it looks like it's worse */
   if (max_pressure >= orig_max_pressure) {
      free(schedule);
               /* Apply the schedule */
   for (unsigned i = 0; i < nr_ins; ++i) {
      agx_remove_instruction(schedule[i]->instr);
                  }
      void
   agx_pressure_schedule(agx_context *ctx)
   {
      agx_compute_liveness(ctx);
   void *memctx = ralloc_context(ctx);
   BITSET_WORD *live =
            agx_foreach_block(ctx, block) {
      struct sched_ctx sctx = {
      .dag = create_dag(ctx, block, memctx),
                           /* Clean up after liveness analysis */
   agx_foreach_instr_global(ctx, I) {
      agx_foreach_ssa_src(I, s)
                  }
