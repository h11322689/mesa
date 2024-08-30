   /*
   * Copyright © 2010 Intel Corporation
   * Copyright © 2014 Broadcom
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      /**
   * @file vc4_qpu_schedule.c
   *
   * The basic model of the list scheduler is to take a basic block, compute a
   * DAG of the dependencies, and make a list of the DAG heads.  Heuristically
   * pick a DAG head, then put all the children that are now DAG heads into the
   * list of things to schedule.
   *
   * The goal of scheduling here is to pack pairs of operations together in a
   * single QPU instruction.
   */
      #include "vc4_qir.h"
   #include "vc4_qpu.h"
   #include "util/ralloc.h"
   #include "util/dag.h"
      static bool debug;
      struct schedule_node_child;
      struct schedule_node {
         struct dag_node dag;
   struct list_head link;
            /* Longest cycles + instruction_latency() of any parent of this node. */
            /**
      * Minimum number of cycles from scheduling this instruction until the
   * end of the program, based on the slowest dependency chain through
   * the children.
               /**
      * cycles between this instruction being scheduled and when its result
   * can be consumed.
               /**
      * Which uniform from uniform_data[] this instruction read, or -1 if
   * not reading a uniform.
      };
      /* When walking the instructions in reverse, we need to swap before/after in
   * add_dep().
   */
   enum direction { F, R };
      struct schedule_state {
         struct dag *dag;
   struct schedule_node *last_r[6];
   struct schedule_node *last_ra[32];
   struct schedule_node *last_rb[32];
   struct schedule_node *last_sf;
   struct schedule_node *last_vpm_read;
   struct schedule_node *last_tmu_write;
   struct schedule_node *last_tlb;
   struct schedule_node *last_vpm;
   struct schedule_node *last_uniforms_reset;
   enum direction dir;
   /* Estimated cycle when the current instruction would start. */
   };
      static void
   add_dep(struct schedule_state *state,
         struct schedule_node *before,
   struct schedule_node *after,
   {
         bool write_after_read = !write && state->dir == R;
            if (!before || !after)
                     if (state->dir == F)
         else
   }
      static void
   add_read_dep(struct schedule_state *state,
               {
         }
      static void
   add_write_dep(struct schedule_state *state,
               {
         add_dep(state, *before, after, true);
   }
      static bool
   qpu_writes_r4(uint64_t inst)
   {
                  switch(sig) {
   case QPU_SIG_COLOR_LOAD:
   case QPU_SIG_LOAD_TMU0:
   case QPU_SIG_LOAD_TMU1:
   case QPU_SIG_ALPHA_MASK_LOAD:
         default:
         }
      static void
   process_raddr_deps(struct schedule_state *state, struct schedule_node *n,
         {
         switch (raddr) {
   case QPU_R_VARY:
                  case QPU_R_VPM:
                  case QPU_R_UNIF:
                  case QPU_R_NOP:
   case QPU_R_ELEM_QPU:
   case QPU_R_XY_PIXEL_COORD:
   case QPU_R_MS_REV_FLAGS:
            default:
            if (raddr < 32) {
            if (is_a)
            } else {
                  }
      static bool
   is_tmu_write(uint32_t waddr)
   {
         switch (waddr) {
   case QPU_W_TMU0_S:
   case QPU_W_TMU0_T:
   case QPU_W_TMU0_R:
   case QPU_W_TMU0_B:
   case QPU_W_TMU1_S:
   case QPU_W_TMU1_T:
   case QPU_W_TMU1_R:
   case QPU_W_TMU1_B:
         default:
         }
      static bool
   reads_uniform(uint64_t inst)
   {
         if (QPU_GET_FIELD(inst, QPU_SIG) == QPU_SIG_LOAD_IMM)
            return (QPU_GET_FIELD(inst, QPU_RADDR_A) == QPU_R_UNIF ||
            (QPU_GET_FIELD(inst, QPU_RADDR_B) == QPU_R_UNIF &&
      }
      static void
   process_mux_deps(struct schedule_state *state, struct schedule_node *n,
         {
         if (mux != QPU_MUX_A && mux != QPU_MUX_B)
   }
         static void
   process_waddr_deps(struct schedule_state *state, struct schedule_node *n,
         {
         uint64_t inst = n->inst->inst;
            if (waddr < 32) {
            if (is_a) {
         } else {
      } else if (is_tmu_write(waddr)) {
               } else if (qpu_waddr_is_tlb(waddr) ||
               } else {
            switch (waddr) {
   case QPU_W_ACC0:
   case QPU_W_ACC1:
   case QPU_W_ACC2:
   case QPU_W_ACC3:
   case QPU_W_ACC5:
                                             case QPU_W_VPMVCD_SETUP:
            if (is_a)
                     case QPU_W_SFU_RECIP:
   case QPU_W_SFU_RECIPSQRT:
   case QPU_W_SFU_EXP:
                        case QPU_W_TLB_STENCIL_SETUP:
            /* This isn't a TLB operation that does things like
   * implicitly lock the scoreboard, but it does have to
   * appear before TLB_Z, and each of the TLB_STENCILs
   * have to schedule in the same order relative to each
                                                                              default:
            }
      static void
   process_cond_deps(struct schedule_state *state, struct schedule_node *n,
         {
         switch (cond) {
   case QPU_COND_NEVER:
   case QPU_COND_ALWAYS:
         default:
               }
      /**
   * Common code for dependencies that need to be tracked both forward and
   * backward.
   *
   * This is for things like "all reads of r4 have to happen between the r4
   * writes that surround them".
   */
   static void
   calculate_deps(struct schedule_state *state, struct schedule_node *n)
   {
         uint64_t inst = n->inst->inst;
   uint32_t add_op = QPU_GET_FIELD(inst, QPU_OP_ADD);
   uint32_t mul_op = QPU_GET_FIELD(inst, QPU_OP_MUL);
   uint32_t waddr_add = QPU_GET_FIELD(inst, QPU_WADDR_ADD);
   uint32_t waddr_mul = QPU_GET_FIELD(inst, QPU_WADDR_MUL);
   uint32_t sig = QPU_GET_FIELD(inst, QPU_SIG);
   uint32_t raddr_a = sig == QPU_SIG_BRANCH ?
               uint32_t raddr_b = QPU_GET_FIELD(inst, QPU_RADDR_B);
   uint32_t add_a = QPU_GET_FIELD(inst, QPU_ADD_A);
   uint32_t add_b = QPU_GET_FIELD(inst, QPU_ADD_B);
   uint32_t mul_a = QPU_GET_FIELD(inst, QPU_MUL_A);
            if (sig != QPU_SIG_LOAD_IMM) {
            process_raddr_deps(state, n, raddr_a, true);
   if (sig != QPU_SIG_SMALL_IMM &&
               if (sig != QPU_SIG_LOAD_IMM && sig != QPU_SIG_BRANCH) {
            if (add_op != QPU_A_NOP) {
               }
   if (mul_op != QPU_M_NOP) {
                     process_waddr_deps(state, n, waddr_add, true);
   process_waddr_deps(state, n, waddr_mul, false);
   if (qpu_writes_r4(inst))
            switch (sig) {
   case QPU_SIG_SW_BREAKPOINT:
   case QPU_SIG_NONE:
   case QPU_SIG_SMALL_IMM:
   case QPU_SIG_LOAD_IMM:
            case QPU_SIG_THREAD_SWITCH:
   case QPU_SIG_LAST_THREAD_SWITCH:
            /* All accumulator contents and flags are undefined after the
   * switch.
   */
                        /* Scoreboard-locking operations have to stay after the last
                              case QPU_SIG_LOAD_TMU0:
   case QPU_SIG_LOAD_TMU1:
            /* TMU loads are coming from a FIFO, so ordering is important.
               case QPU_SIG_COLOR_LOAD:
                  case QPU_SIG_BRANCH:
                  case QPU_SIG_PROG_END:
   case QPU_SIG_WAIT_FOR_SCOREBOARD:
   case QPU_SIG_SCOREBOARD_UNLOCK:
   case QPU_SIG_COVERAGE_LOAD:
   case QPU_SIG_COLOR_LOAD_END:
   case QPU_SIG_ALPHA_MASK_LOAD:
                        process_cond_deps(state, n, QPU_GET_FIELD(inst, QPU_COND_ADD));
   process_cond_deps(state, n, QPU_GET_FIELD(inst, QPU_COND_MUL));
   if ((inst & QPU_SF) && sig != QPU_SIG_BRANCH)
   }
      static void
   calculate_forward_deps(struct vc4_compile *c, struct dag *dag,
         {
                  memset(&state, 0, sizeof(state));
   state.dag = dag;
            list_for_each_entry(struct schedule_node, node, schedule_list, link)
   }
      static void
   calculate_reverse_deps(struct vc4_compile *c, struct dag *dag,
         {
                  memset(&state, 0, sizeof(state));
   state.dag = dag;
            list_for_each_entry_rev(struct schedule_node, node, schedule_list,
               }
      struct choose_scoreboard {
         struct dag *dag;
   int tick;
   int last_sfu_write_tick;
   int last_uniforms_reset_tick;
   uint32_t last_waddr_a, last_waddr_b;
   };
      static bool
   reads_too_soon_after_write(struct choose_scoreboard *scoreboard, uint64_t inst)
   {
         uint32_t raddr_a = QPU_GET_FIELD(inst, QPU_RADDR_A);
   uint32_t raddr_b = QPU_GET_FIELD(inst, QPU_RADDR_B);
            /* Full immediate loads don't read any registers. */
   if (sig == QPU_SIG_LOAD_IMM)
            uint32_t src_muxes[] = {
            QPU_GET_FIELD(inst, QPU_ADD_A),
   QPU_GET_FIELD(inst, QPU_ADD_B),
      };
   for (int i = 0; i < ARRAY_SIZE(src_muxes); i++) {
            if ((src_muxes[i] == QPU_MUX_A &&
         raddr_a < 32 &&
      (src_muxes[i] == QPU_MUX_B &&
      sig != QPU_SIG_SMALL_IMM &&
                        if (src_muxes[i] == QPU_MUX_R4) {
            if (scoreboard->tick -
                  if (sig == QPU_SIG_SMALL_IMM &&
         QPU_GET_FIELD(inst, QPU_SMALL_IMM) >= QPU_SMALL_IMM_MUL_ROT) {
                     if (scoreboard->last_waddr_a == mux_a + QPU_W_ACC0 ||
      scoreboard->last_waddr_a == mux_b + QPU_W_ACC0 ||
   scoreboard->last_waddr_b == mux_a + QPU_W_ACC0 ||
   scoreboard->last_waddr_b == mux_b + QPU_W_ACC0) {
            if (reads_uniform(inst) &&
         scoreboard->tick - scoreboard->last_uniforms_reset_tick <= 2) {
            }
      static bool
   pixel_scoreboard_too_soon(struct choose_scoreboard *scoreboard, uint64_t inst)
   {
         }
      static int
   get_instruction_priority(uint64_t inst)
   {
         uint32_t waddr_add = QPU_GET_FIELD(inst, QPU_WADDR_ADD);
   uint32_t waddr_mul = QPU_GET_FIELD(inst, QPU_WADDR_MUL);
   uint32_t sig = QPU_GET_FIELD(inst, QPU_SIG);
   uint32_t baseline_score;
            /* Schedule TLB operations as late as possible, to get more
      * parallelism between shaders.
      if (qpu_inst_is_tlb(inst))
                  /* Schedule texture read results collection late to hide latency. */
   if (sig == QPU_SIG_LOAD_TMU0 || sig == QPU_SIG_LOAD_TMU1)
                  /* Default score for things that aren't otherwise special. */
   baseline_score = next_score;
            /* Schedule texture read setup early to hide their latency better. */
   if (is_tmu_write(waddr_add) || is_tmu_write(waddr_mul))
                  }
      static struct schedule_node *
   choose_instruction_to_schedule(struct choose_scoreboard *scoreboard,
               {
         struct schedule_node *chosen = NULL;
            /* Don't pair up anything with a thread switch signal -- emit_thrsw()
      * will handle pairing it along with filling the delay slots.
      if (prev_inst) {
            uint32_t prev_sig = QPU_GET_FIELD(prev_inst->inst->inst,
         if (prev_sig == QPU_SIG_THREAD_SWITCH ||
      prev_sig == QPU_SIG_LAST_THREAD_SWITCH) {
            list_for_each_entry(struct schedule_node, n, &scoreboard->dag->heads,
                                 /* Don't choose the branch instruction until it's the last one
   * left.  XXX: We could potentially choose it before it's the
   * last one, if the remaining instructions fit in the delay
   * slots.
   */
   if (sig == QPU_SIG_BRANCH &&
                        /* "An instruction must not read from a location in physical
   *  regfile A or B that was written to by the previous
   *  instruction."
                        /* "A scoreboard wait must not occur in the first two
   *  instructions of a fragment shader. This is either the
   *  explicit Wait for Scoreboard signal or an implicit wait
   *  with the first tile-buffer read or write instruction."
                        /* If we're trying to pair with another instruction, check
   * that they're compatible.
   */
   if (prev_inst) {
            /* Don't pair up a thread switch signal -- we'll
   * handle pairing it when we pick it on its own.
   */
                                             /* Don't merge in something that will lock the TLB.
   * Hopefully what we have in inst will release some
   * other instructions, allowing us to delay the
                                                         /* Found a valid instruction.  If nothing better comes along,
   * this one works.
   */
   if (!chosen) {
                              if (prio > chosen_prio) {
                                    if (n->delay > chosen->delay) {
               } else if (n->delay < chosen->delay) {
               }
      static void
   update_scoreboard_for_chosen(struct choose_scoreboard *scoreboard,
         {
         uint32_t waddr_add = QPU_GET_FIELD(inst, QPU_WADDR_ADD);
            if (!(inst & QPU_WS)) {
               } else {
                        if ((waddr_add >= QPU_W_SFU_RECIP && waddr_add <= QPU_W_SFU_LOG) ||
         (waddr_mul >= QPU_W_SFU_RECIP && waddr_mul <= QPU_W_SFU_LOG)) {
            if (waddr_add == QPU_W_UNIFORMS_ADDRESS ||
         waddr_mul == QPU_W_UNIFORMS_ADDRESS) {
            if (qpu_inst_is_tlb(inst))
   }
      static void
   dump_state(struct dag *dag)
   {
         list_for_each_entry(struct schedule_node, n, &dag->heads, dag.link) {
                                 util_dynarray_foreach(&n->dag.edges, struct dag_edge, edge) {
                                       fprintf(stderr, "                 - ");
   vc4_qpu_disasm(&child->inst->inst, 1);
   fprintf(stderr, " (%d parents, %c)\n",
   }
      static uint32_t waddr_latency(uint32_t waddr, uint64_t after)
   {
         if (waddr < 32)
            /* Apply some huge latency between texture fetch requests and getting
      * their results back.
   *
   * FIXME: This is actually pretty bogus.  If we do:
   *
   * mov tmu0_s, a
   * <a bit of math>
   * mov tmu0_s, b
   * load_tmu0
   * <more math>
   * load_tmu0
   *
   * we count that as worse than
   *
   * mov tmu0_s, a
   * mov tmu0_s, b
   * <lots of math>
   * load_tmu0
   * <more math>
   * load_tmu0
   *
   * because we associate the first load_tmu0 with the *second* tmu0_s.
      if (waddr == QPU_W_TMU0_S) {
               }
   if (waddr == QPU_W_TMU1_S) {
                        switch(waddr) {
   case QPU_W_SFU_RECIP:
   case QPU_W_SFU_RECIPSQRT:
   case QPU_W_SFU_EXP:
   case QPU_W_SFU_LOG:
         default:
         }
      static uint32_t
   instruction_latency(struct schedule_node *before, struct schedule_node *after)
   {
         uint64_t before_inst = before->inst->inst;
            return MAX2(waddr_latency(QPU_GET_FIELD(before_inst, QPU_WADDR_ADD),
               }
      /** Recursive computation of the delay member of a node. */
   static void
   compute_delay(struct dag_node *node, void *state)
   {
                           util_dynarray_foreach(&n->dag.edges, struct dag_edge, edge) {
            struct schedule_node *child =
            }
      /* Removes a DAG head, but removing only the WAR edges. (dag_prune_head()
   * should be called on it later to finish pruning the other edges).
   */
   static void
   pre_remove_head(struct dag *dag, struct schedule_node *n)
   {
                  util_dynarray_foreach(&n->dag.edges, struct dag_edge, edge) {
               }
      static void
   mark_instruction_scheduled(struct dag *dag,
               {
         if (!node)
            util_dynarray_foreach(&node->dag.edges, struct dag_edge, edge) {
                                                      }
   }
      /**
   * Emits a THRSW/LTHRSW signal in the stream, trying to move it up to pair
   * with another instruction.
   */
   static void
   emit_thrsw(struct vc4_compile *c,
               {
                  /* There should be nothing in a thrsw inst being scheduled other than
      * the signal bits.
      assert(QPU_GET_FIELD(inst, QPU_OP_ADD) == QPU_A_NOP);
            /* Try to find an earlier scheduled instruction that we can merge the
      * thrsw into.
      int thrsw_ip = c->qpu_inst_count;
   for (int i = 1; i <= MIN2(c->qpu_inst_count, 3); i++) {
                                       if (thrsw_ip != c->qpu_inst_count) {
            /* Merge the thrsw into the existing instruction. */
      } else {
                        /* Fill the delay slots. */
   while (c->qpu_inst_count < thrsw_ip + 3) {
               }
      static uint32_t
   schedule_instructions(struct vc4_compile *c,
                        struct choose_scoreboard *scoreboard,
   struct qblock *block,
      {
                  while (!list_is_empty(&scoreboard->dag->heads)) {
            struct schedule_node *chosen =
                              /* If there are no valid instructions to schedule, drop a NOP
                        if (debug) {
            fprintf(stderr, "t=%4d: current list:\n",
         dump_state(scoreboard->dag);
                     /* Schedule this instruction onto the QPU list. Also try to
   * find an instruction to pair with it.
   */
   if (chosen) {
            time = MAX2(chosen->unblocked_time, time);
   pre_remove_head(scoreboard->dag, chosen);
   if (chosen->uniform != -1) {
         c->uniform_data[*next_uniform] =
                              merge = choose_instruction_to_schedule(scoreboard,
               if (merge) {
         time = MAX2(merge->unblocked_time, time);
   inst = qpu_merge_inst(inst, merge->inst->inst);
   assert(inst != 0);
   if (merge->uniform != -1) {
                                          if (debug) {
            fprintf(stderr, "t=%4d: merging: ",
         vc4_qpu_disasm(&merge->inst->inst, 1);
                                             /* Now that we've scheduled a new instruction, some of its
   * children can be promoted to the list of instructions ready to
   * be scheduled.  Update the children's unblocked time for this
   * DAG edge as we do so.
                        if (QPU_GET_FIELD(inst, QPU_SIG) == QPU_SIG_THREAD_SWITCH ||
      QPU_GET_FIELD(inst, QPU_SIG) == QPU_SIG_LAST_THREAD_SWITCH) {
      } else {
                                       if (QPU_GET_FIELD(inst, QPU_SIG) == QPU_SIG_BRANCH) {
            block->branch_qpu_ip = c->qpu_inst_count - 1;
   /* Fill the delay slots.
   *
   * We should fill these with actual instructions,
   * instead, but that will probably need to be done
   * after this, once we know what the leading
   * instructions of the successors are (so we can
   * handle A/B register file write latency)
   */
   inst = qpu_NOP();
   update_scoreboard_for_chosen(scoreboard, inst);
   qpu_serialize_one_inst(c, inst);
            }
      static uint32_t
   qpu_schedule_instructions_block(struct vc4_compile *c,
                                 {
         scoreboard->dag = dag_create(NULL);
                     /* Wrap each instruction in a scheduler structure. */
   uint32_t next_sched_uniform = *next_uniform;
   while (!list_is_empty(&block->qpu_inst_list)) {
            struct queued_qpu_inst *inst =
                                       if (reads_uniform(inst->inst)) {
         } else {
         }
               calculate_forward_deps(c, scoreboard->dag, &setup_list);
                     uint32_t cycles = schedule_instructions(c, scoreboard, block,
                              ralloc_free(scoreboard->dag);
            }
      static void
   qpu_set_branch_targets(struct vc4_compile *c)
   {
         qir_for_each_block(block, c) {
                                 /* If there was no branch instruction, then the successor
   * block must follow immediately after this one.
   */
   if (block->branch_qpu_ip == ~0) {
                              /* Set the branch target for the block that doesn't follow
   * immediately after ours.
   */
                        uint32_t branch_target =
                              /* Make sure that the if-we-don't-jump successor was scheduled
   * just after the delay slots.
   */
   if (block->successors[1]) {
            }
      uint32_t
   qpu_schedule_instructions(struct vc4_compile *c)
   {
         /* We reorder the uniforms as we schedule instructions, so save the
      * old data off and replace it.
      uint32_t *uniform_data = c->uniform_data;
   enum quniform_contents *uniform_contents = c->uniform_contents;
   c->uniform_contents = ralloc_array(c, enum quniform_contents,
         c->uniform_data = ralloc_array(c, uint32_t, c->num_uniforms);
   c->uniform_array_size = c->num_uniforms;
            struct choose_scoreboard scoreboard;
   memset(&scoreboard, 0, sizeof(scoreboard));
   scoreboard.last_waddr_a = ~0;
   scoreboard.last_waddr_b = ~0;
   scoreboard.last_sfu_write_tick = -10;
            if (debug) {
            fprintf(stderr, "Pre-schedule instructions\n");
   qir_for_each_block(block, c) {
            fprintf(stderr, "BLOCK %d\n", block->index);
   list_for_each_entry(struct queued_qpu_inst, q,
                              uint32_t cycles = 0;
   qir_for_each_block(block, c) {
                           cycles += qpu_schedule_instructions_block(c,
                                                            if (debug) {
            fprintf(stderr, "Post-schedule instructions\n");
               }
