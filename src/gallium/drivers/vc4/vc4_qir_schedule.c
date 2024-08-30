   /*
   * Copyright © 2010 Intel Corporation
   * Copyright © 2014-2015 Broadcom
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
   * @file vc4_qir_schedule.c
   *
   * The basic model of the list scheduler is to take a basic block, compute a
   * DAG of the dependencies from the bottom up, and make a list of the DAG
   * heads.  Heuristically pick a DAG head and schedule (remove) it, then put
   * all the parents that are now DAG heads into the list of things to
   * schedule.
   *
   * The goal of scheduling here, before register allocation and conversion to
   * QPU instructions, is to reduce register pressure by reordering instructions
   * to consume values when possible.
   */
      #include "vc4_qir.h"
   #include "util/dag.h"
      static bool debug;
      struct schedule_node {
         struct dag_node dag;
   struct list_head link;
            /* Length of the longest (latency) chain from a DAG head to the this
      * instruction.
               /* Longest time + latency_between(parent, this) of any parent of this
      * node.
      };
      struct schedule_state {
                                    };
      /* When walking the instructions in reverse, we need to swap before/after in
   * add_dep().
   */
   enum direction { F, R };
      /**
   * Marks a dependency between two instructions, that \p after must appear after
   * \p before.
   *
   * Our dependencies are tracked as a DAG.  Since we're scheduling bottom-up,
   * the latest instructions with nothing left to schedule are the DAG heads,
   * and their inputs are their children.
   */
   static void
   add_dep(enum direction dir,
         struct schedule_node *before,
   {
         if (!before || !after)
                     if (dir == R) {
            struct schedule_node *t = before;
               }
      static void
   add_write_dep(enum direction dir,
               {
         add_dep(dir, *before, after);
   }
      struct schedule_setup_state {
         struct schedule_node **last_temp_write;
   struct schedule_node *last_sf;
   struct schedule_node *last_vary_read;
   struct schedule_node *last_vpm_read;
   struct schedule_node *last_vpm_write;
   struct schedule_node *last_tex_coord;
   struct schedule_node *last_tex_result;
   struct schedule_node *last_tlb;
   struct schedule_node *last_uniforms_reset;
      /**
            * Texture FIFO tracking.  This is done top-to-bottom, and is used to
   * track the QOP_TEX_RESULTs and add dependencies on previous ones
   * when trying to submit texture coords with TFREQ full or new texture
   * fetches with TXRCV full.
      struct {
               } tex_fifo[8];
   int tfreq_count; /**< Number of texture coords outstanding. */
   int tfrcv_count; /**< Number of texture results outstanding. */
   };
      static void
   block_until_tex_result(struct schedule_setup_state *state, struct schedule_node *n)
   {
                  state->tfreq_count -= state->tex_fifo[0].coords;
            memmove(&state->tex_fifo[0],
               }
      /**
   * Common code for dependencies that need to be tracked both forward and
   * backward.
   *
   * This is for things like "all VPM reads have to happen in order."
   */
   static void
   calculate_deps(struct schedule_setup_state *state, struct schedule_node *n)
   {
         struct qinst *inst = n->inst;
               /* Add deps for temp registers and varyings accesses.  Note that we
      * ignore uniforms accesses, because qir_reorder_uniforms() happens
   * after this.
      for (int i = 0; i < qir_get_nsrc(inst); i++) {
            switch (inst->src[i].file) {
   case QFILE_TEMP:
                                                                  default:
               switch (inst->op) {
   case QOP_VARY_ADD_C:
                  case QOP_TEX_RESULT:
                        case QOP_THRSW:
            /* After a new THRSW, one must collect all texture samples
   * queued since the previous THRSW/program start.  For now, we
   * have one THRSW in between each texture setup and its
   * results collection as our input, and we just make sure that
   * that ordering is maintained.
                                       /* Setup, like the varyings, will need to be drained before we
                        /* The TLB-locking operations have to stay after the last
   * thread switch.
               case QOP_TLB_COLOR_READ:
   case QOP_MS_MASK:
                  default:
                  switch (inst->dst.file) {
   case QFILE_VPM:
                  case QFILE_TEMP:
                  case QFILE_TLB_COLOR_WRITE:
   case QFILE_TLB_COLOR_WRITE_MS:
   case QFILE_TLB_Z_WRITE:
   case QFILE_TLB_STENCIL_SETUP:
                  case QFILE_TEX_S_DIRECT:
   case QFILE_TEX_S:
   case QFILE_TEX_T:
   case QFILE_TEX_R:
   case QFILE_TEX_B:
            /* Texturing setup gets scheduled in order, because
   * the uniforms referenced by them have to land in a
   * specific order.
               default:
                  if (qir_depends_on_flags(inst))
            if (inst->sf)
   }
      static void
   calculate_forward_deps(struct vc4_compile *c, void *mem_ctx,
         {
                  memset(&state, 0, sizeof(state));
   state.last_temp_write = rzalloc_array(mem_ctx, struct schedule_node *,
                  list_for_each_entry(struct schedule_node, n, schedule_list, link) {
                              for (int i = 0; i < qir_get_nsrc(inst); i++) {
            switch (inst->src[i].file) {
   case QFILE_UNIF:
         add_dep(state.dir, state.last_uniforms_reset, n);
                     switch (inst->dst.file) {
   case QFILE_TEX_S_DIRECT:
   case QFILE_TEX_S:
   case QFILE_TEX_T:
   case QFILE_TEX_R:
   case QFILE_TEX_B:
            /* From the VC4 spec:
   *
   *     "The TFREQ input FIFO holds two full lots of s,
   *      t, r, b data, plus associated setup data, per
   *      QPU, that is, there are eight data slots. For
   *      each texture request, slots are only consumed
   *      for the components of s, t, r, and b actually
   *      written. Thus the FIFO can hold four requests
   *      of just (s, t) data, or eight requests of just
   *      s data (for direct addressed data lookups).
   *
   *      Note that there is one FIFO per QPU, and the
   *      FIFO has no concept of threads - that is,
   *      multi-threaded shaders must be careful to use
   *      only 1/2 the FIFO depth before reading
   *      back. Multi-threaded programs must also
   *      therefore always thread switch on texture
   *      fetch as the other thread may have data
   *      waiting in the FIFO."
   *
   * If the texture coordinate fifo is full, block this
   * on the last QOP_TEX_RESULT.
                              /* From the VC4 spec:
   *
   *     "Since the maximum number of texture requests
   *      in the input (TFREQ) FIFO is four lots of (s,
   *      t) data, the output (TFRCV) FIFO is sized to
   *      holds four lots of max-size color data per
   *      QPU. For non-float color, reads are packed
   *      RGBA8888 data (one read per pixel). For 16-bit
   *      float color, two reads are necessary per
   *      pixel, with reads packed as RG1616 then
   *      BA1616. So per QPU there are eight color slots
   *      in the TFRCV FIFO."
   *
   * If the texture result fifo is full, block adding
   * any more to it until the last QOP_TEX_RESULT.
   */
   if (inst->dst.file == QFILE_TEX_S ||
      inst->dst.file == QFILE_TEX_S_DIRECT) {
                                                                  switch (inst->op) {
   case QOP_TEX_RESULT:
            /* Results have to be fetched after the
   * coordinate setup.  Note that we're assuming
   * here that our input shader has the texture
   * coord setup and result fetch in order,
                                                                              default:
      }
      static void
   calculate_reverse_deps(struct vc4_compile *c, void *mem_ctx,
         {
                  memset(&state, 0, sizeof(state));
   state.dir = R;
   state.last_temp_write = rzalloc_array(mem_ctx, struct schedule_node *,
            list_for_each_entry_rev(struct schedule_node, n, schedule_list, link) {
         }
      static int
   get_register_pressure_cost(struct schedule_state *state, struct qinst *inst)
   {
                  if (inst->dst.file == QFILE_TEMP &&
                  for (int i = 0; i < qir_get_nsrc(inst); i++) {
            if (inst->src[i].file != QFILE_TEMP ||
                        bool already_counted = false;
   for (int j = 0; j < i; j++) {
            if (inst->src[i].file == inst->src[j].file &&
      inst->src[i].index == inst->src[j].index) {
   }
               }
      static bool
   locks_scoreboard(struct qinst *inst)
   {
         if (inst->op == QOP_TLB_COLOR_READ)
            switch (inst->dst.file) {
   case QFILE_TLB_Z_WRITE:
   case QFILE_TLB_COLOR_WRITE:
   case QFILE_TLB_COLOR_WRITE_MS:
         default:
         }
      static struct schedule_node *
   choose_instruction(struct schedule_state *state)
   {
                  list_for_each_entry(struct schedule_node, n, &state->dag->heads,
                  /* The branches aren't being tracked as dependencies.  Make
   * sure that they stay scheduled as the last instruction of
   * the block, which is to say the first one we choose to
   * schedule.
                        if (!chosen) {
                        /* Prefer scheduling things that lock the scoreboard, so that
   * they appear late in the program and we get more parallelism
   * between shaders on multiple QPUs hitting the same fragment.
   */
   if (locks_scoreboard(n->inst) &&
      !locks_scoreboard(chosen->inst)) {
            } else if (!locks_scoreboard(n->inst) &&
                        /* If we would block on the previously chosen node, but would
   * block less on this one, then prefer it.
   */
   if (chosen->unblocked_time > state->time &&
      n->unblocked_time < chosen->unblocked_time) {
            } else if (n->unblocked_time > state->time &&
                        /* If we can definitely reduce register pressure, do so
   * immediately.
   */
   int register_pressure_cost =
                        if (register_pressure_cost < chosen_register_pressure_cost) {
               } else if (register_pressure_cost >
                        /* Otherwise, prefer instructions with the deepest chain to
   * the end of the program.  This avoids the problem of
   * "everything generates a temp, nothing finishes freeing one,
   * guess I'll just keep emitting varying mul/adds".
   */
   if (n->delay > chosen->delay) {
               } else if (n->delay < chosen->delay) {
               }
      static void
   dump_state(struct vc4_compile *c, struct schedule_state *state)
   {
         uint32_t i = 0;
   list_for_each_entry(struct schedule_node, n, &state->dag->heads,
                  fprintf(stderr, "%3d: ", i++);
                        util_dynarray_foreach(&n->dag.edges, struct dag_edge, edge) {
            struct schedule_node *child =
         fprintf(stderr, "   - ");
   qir_dump_inst(c, child->inst);
   }
      /* Estimate of how many instructions we should schedule between operations.
   *
   * These aren't in real cycle counts, because we're just estimating cycle
   * times anyway.  QIR instructions will get paired up when turned into QPU
   * instructions, or extra NOP delays will have to be added due to register
   * allocation choices.
   */
   static uint32_t
   latency_between(struct schedule_node *before, struct schedule_node *after)
   {
         if ((before->inst->dst.file == QFILE_TEX_S ||
         before->inst->dst.file == QFILE_TEX_S_DIRECT) &&
            switch (before->inst->op) {
   case QOP_RCP:
   case QOP_RSQ:
   case QOP_EXP2:
   case QOP_LOG2:
            for (int i = 0; i < qir_get_nsrc(after->inst); i++) {
            if (after->inst->src[i].file ==
      before->inst->dst.file &&
   after->inst->src[i].index ==
   before->inst->dst.index) {
      /* There are two QPU delay slots before we can
      * read a math result, which could be up to 4
   * QIR instructions if they packed well.
      default:
                  }
      /** Recursive computation of the delay member of a node. */
   static void
   compute_delay(struct dag_node *node, void *state)
   {
                  /* The color read needs to be scheduled late, to avoid locking the
      * scoreboard early.  This is our best tool for encouraging that.  The
   * other scoreboard locking ops will have this happen by default,
   * since they are generally the DAG heads or close to them.
      if (n->inst->op == QOP_TLB_COLOR_READ)
         else
            util_dynarray_foreach(&n->dag.edges, struct dag_edge, edge) {
                              }
      static void
   schedule_instructions(struct vc4_compile *c,
         {
         if (debug) {
                        state->time = 0;
   while (!list_is_empty(&state->dag->heads)) {
                           if (debug) {
            fprintf(stderr, "current list:\n");
   dump_state(c, state);
   fprintf(stderr, "chose: ");
                                             /* Now that we've scheduled a new instruction, some of its
   * children can be promoted to the list of instructions ready to
   * be scheduled.  Update the children's unblocked time for this
   * DAG edge as we do so.
   */
   util_dynarray_foreach(&chosen->dag.edges,
                                 child->unblocked_time = MAX2(child->unblocked_time,
                           /* Update our tracking of register pressure. */
   for (int i = 0; i < qir_get_nsrc(inst); i++) {
               }
   if (inst->dst.file == QFILE_TEMP) {
                           }
      static void
   qir_schedule_instructions_block(struct vc4_compile *c,
         {
                  state->temp_writes = rzalloc_array(state, uint32_t, c->num_temps);
   state->temp_live = rzalloc_array(state, BITSET_WORD,
                  struct list_head setup_list;
            /* Wrap each instruction in a scheduler structure. */
   qir_for_each_inst_safe(inst, block) {
                     n->inst = inst;
                                    /* Dependencies tracked top-to-bottom. */
   calculate_forward_deps(c, state, &setup_list);
   /* Dependencies tracked bottom-to-top. */
                              }
      void
   qir_schedule_instructions(struct vc4_compile *c)
   {
            if (debug) {
                        qir_for_each_block(block, c)
            if (debug) {
               }
