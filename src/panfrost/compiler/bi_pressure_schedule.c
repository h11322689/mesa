   /*
   * Copyright (C) 2022 Collabora Ltd.
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
   * Authors (Collabora):
   *      Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   */
      /* Bottom-up local scheduler to reduce register pressure */
      #include "util/dag.h"
   #include "compiler.h"
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
      static struct dag *
   create_dag(bi_context *ctx, bi_block *block, void *memctx)
   {
               struct sched_node **last_write =
         struct sched_node *coverage = NULL;
            /* Last memory load, to serialize stores against */
            /* Last memory store, to serialize loads and stores against */
            bi_foreach_instr_in_block(block, I) {
      /* Leave branches at the end */
   if (I->op == BI_OPCODE_JUMP || bi_opcode_props[I->op].branch)
                     struct sched_node *node = rzalloc(memctx, struct sched_node);
   node->instr = I;
            /* Reads depend on writes, no other hazards in SSA */
   bi_foreach_ssa_src(I, s)
            bi_foreach_dest(I, d)
            switch (bi_opcode_props[I->op].message) {
   case BIFROST_MESSAGE_LOAD:
      /* Regular memory loads needs to be serialized against
   * other memory access. However, UBO memory is read-only
   * so it can be moved around freely.
   */
   if (I->seg != BI_SEG_UBO) {
      add_dep(node, memory_store);
                     case BIFROST_MESSAGE_ATTRIBUTE:
      /* Regular attribute loads can be reordered, but
   * writeable attributes can't be. Our one use of
   * writeable attributes are images.
   */
   if ((I->op == BI_OPCODE_LD_TEX) || (I->op == BI_OPCODE_LD_TEX_IMM) ||
      (I->op == BI_OPCODE_LD_ATTR_TEX)) {
   add_dep(node, memory_store);
                     case BIFROST_MESSAGE_STORE:
      assert(I->seg != BI_SEG_UBO);
   add_dep(node, memory_load);
   add_dep(node, memory_store);
               case BIFROST_MESSAGE_ATOMIC:
   case BIFROST_MESSAGE_BARRIER:
      add_dep(node, memory_load);
   add_dep(node, memory_store);
   memory_load = node;
               case BIFROST_MESSAGE_BLEND:
   case BIFROST_MESSAGE_Z_STENCIL:
   case BIFROST_MESSAGE_TILE:
      add_dep(node, coverage);
               case BIFROST_MESSAGE_ATEST:
      /* ATEST signals the end of shader side effects */
                  /* ATEST also updates coverage */
   add_dep(node, coverage);
   coverage = node;
      default:
                           if (I->op == BI_OPCODE_DISCARD_F32) {
      /* Serialize against ATEST */
                  /* Also serialize against memory and barriers */
   add_dep(node, memory_load);
   add_dep(node, memory_store);
   memory_load = node;
      } else if ((I->op == BI_OPCODE_PHI) ||
            (I->op == BI_OPCODE_MOV_I32 &&
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
   calculate_pressure_delta(bi_instr *I, BITSET_WORD *live)
   {
               /* Destinations must be unique */
   bi_foreach_dest(I, d) {
      if (BITSET_TEST(live, I->dest[d].value))
               bi_foreach_ssa_src(I, src) {
      /* Filter duplicates */
            for (unsigned i = 0; i < src; ++i) {
      if (bi_is_equiv(I->src[i], I->src[src])) {
      dupe = true;
                  if (!dupe && !BITSET_TEST(live, I->src[src].value))
                  }
      /*
   * Choose the next instruction, bottom-up. For now we use a simple greedy
   * heuristic: choose the instruction that has the best effect on liveness.
   */
   static struct sched_node *
   choose_instr(struct sched_ctx *s)
   {
      int32_t min_delta = INT32_MAX;
            list_for_each_entry(struct sched_node, n, &s->dag->heads, dag.link) {
               if (delta < min_delta) {
      best = n;
                     }
      static void
   pressure_schedule_block(bi_context *ctx, bi_block *block, struct sched_ctx *s)
   {
      /* off by a constant, that's ok */
   signed pressure = 0;
   signed orig_max_pressure = 0;
            memcpy(s->live, block->ssa_live_out,
            bi_foreach_instr_in_block_rev(block, I) {
      pressure += calculate_pressure_delta(I, s->live);
   orig_max_pressure = MAX2(pressure, orig_max_pressure);
   bi_liveness_ins_update_ssa(s->live, I);
               memcpy(s->live, block->ssa_live_out,
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
      bi_remove_instruction(schedule[i]->instr);
                  }
      void
   bi_pressure_schedule(bi_context *ctx)
   {
      bi_compute_liveness_ssa(ctx);
   void *memctx = ralloc_context(ctx);
   BITSET_WORD *live =
            bi_foreach_block(ctx, block) {
      struct sched_ctx sctx = {.dag = create_dag(ctx, block, memctx),
                           }
