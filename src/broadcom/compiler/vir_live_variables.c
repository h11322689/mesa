   /*
   * Copyright © 2012 Intel Corporation
   * Copyright © 2016 Broadcom
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
      #define MAX_INSTRUCTION (1 << 30)
      #include "util/ralloc.h"
   #include "util/register_allocate.h"
   #include "v3d_compiler.h"
      /* Keeps track of conditional / partial writes in a block */
   struct partial_update_state {
         /* Instruction doing a conditional or partial write */
   struct qinst *inst;
   /* Instruction that set the flags for the conditional write */
   };
      static int
   vir_reg_to_var(struct qreg reg)
   {
         if (reg.file == QFILE_TEMP)
            }
      static void
   vir_setup_use(struct v3d_compile *c, struct qblock *block, int ip,
               {
         int var = vir_reg_to_var(src);
   if (var == -1)
            c->temp_start[var] = MIN2(c->temp_start[var], ip);
            /* The use[] bitset marks when the block makes
      * use of a variable without having completely
   * defined that variable within the block.
      if (!BITSET_TEST(block->def, var)) {
            /* If this use of var is conditional and the condition
   * and flags match those of a previous instruction
   * in the same block partially defining var then we
   * consider var completely defined within the block.
   */
   if (BITSET_TEST(block->defout, var)) {
            struct partial_update_state *state =
         if (state->inst) {
         if (vir_get_cond(inst) == vir_get_cond(state->inst) &&
                  }
      /* The def[] bitset marks when an initialization in a
   * block completely screens off previous updates of
   * that variable.
   */
   static void
   vir_setup_def(struct v3d_compile *c, struct qblock *block, int ip,
               {
         if (inst->qpu.type != V3D_QPU_INSTR_TYPE_ALU)
            int var = vir_reg_to_var(inst->dst);
   if (var == -1)
            c->temp_start[var] = MIN2(c->temp_start[var], ip);
            /* Mark the block as having a (partial) def of the var. */
            /* If we've already tracked this as a def that screens off previous
      * uses, or already used it within the block, there's nothing to do.
      if (BITSET_TEST(block->use, var) || BITSET_TEST(block->def, var))
            /* Easy, common case: unconditional full register update.*/
   if ((inst->qpu.flags.ac == V3D_QPU_COND_NONE &&
         inst->qpu.flags.mc == V3D_QPU_COND_NONE) &&
   inst->qpu.alu.add.output_pack == V3D_QPU_PACK_NONE &&
   inst->qpu.alu.mul.output_pack == V3D_QPU_PACK_NONE) {
                  /* Keep track of conditional writes.
      *
   * Notice that the dst's live range for a conditional or partial writes
   * will get extended up the control flow to the top of the program until
   * we find a full write, making register allocation more difficult, so
   * we should try our best to keep track of these and figure out if a
   * combination of them actually writes the entire register so we can
   * stop that process early and reduce liveness.
   *
   * FIXME: Track partial updates via pack/unpack.
      struct partial_update_state *state = &partial_update[var];
   if (inst->qpu.flags.ac != V3D_QPU_COND_NONE ||
         inst->qpu.flags.mc != V3D_QPU_COND_NONE) {
         }
      /* Sets up the def/use arrays for when variables are used-before-defined or
   * defined-before-used in the block.
   *
   * Also initializes the temp_start/temp_end to cover just the instruction IPs
   * where the variable is used, which will be extended later in
   * vir_compute_start_end().
   */
   static void
   vir_setup_def_use(struct v3d_compile *c)
   {
         struct partial_update_state *partial_update =
                  vir_for_each_block(block, c) {
                                             vir_for_each_inst(inst, block) {
                                                                                                            /* Payload registers: for fragment shaders, W,
   * centroid W, and Z will be initialized in r0/1/2
   * until v42, or r1/r2/r3 since v71.
   *
   * For compute shaders, payload is in r0/r2 up to v42,
   * r2/r3 since v71.
   *
   * Register allocation will force their nodes to those
   * registers.
   */
   if (inst->src[0].file == QFILE_REG) {
         uint32_t min_payload_r = c->devinfo->ver >= 71 ? 1 : 0;
   uint32_t max_payload_r = c->devinfo->ver >= 71 ? 3 : 2;
                                       }
      static bool
   vir_live_variables_dataflow(struct v3d_compile *c, int bitset_words)
   {
                  vir_for_each_block_rev(block, c) {
            /* Update live_out: Any successor using the variable
   * on entrance needs us to have the variable live on
   * exit.
   */
   vir_for_each_successor(succ, block) {
            for (int i = 0; i < bitset_words; i++) {
         BITSET_WORD new_live_out = (succ->live_in[i] &
         if (new_live_out) {
                     /* Update live_in */
   for (int i = 0; i < bitset_words; i++) {
            BITSET_WORD new_live_in = (block->use[i] |
               if (new_live_in & ~block->live_in[i]) {
                  }
      static bool
   vir_live_variables_defin_defout_dataflow(struct v3d_compile *c, int bitset_words)
   {
                  vir_for_each_block_rev(block, c) {
            /* Propagate defin/defout down the successors to produce the
   * union of blocks with a reachable (partial) definition of
   * the var.
   *
   * This keeps a conditional first write to a reg from
   * extending its lifetime back to the start of the program.
   */
   vir_for_each_successor(succ, block) {
            for (int i = 0; i < bitset_words; i++) {
         BITSET_WORD new_def = (block->defout[i] &
         succ->defin[i] |= new_def;
            }
      /**
   * Extend the start/end ranges for each variable to account for the
   * new information calculated from control flow.
   */
   static void
   vir_compute_start_end(struct v3d_compile *c, int num_vars)
   {
         vir_for_each_block(block, c) {
            for (int i = 0; i < num_vars; i++) {
            if (BITSET_TEST(block->live_in, i) &&
      BITSET_TEST(block->defin, i)) {
                                 if (BITSET_TEST(block->live_out, i) &&
      BITSET_TEST(block->defout, i)) {
      c->temp_start[i] = MIN2(c->temp_start[i],
         }
      void
   vir_calculate_live_intervals(struct v3d_compile *c)
   {
                  /* We may be called more than once if we've rearranged the program to
      * try to get register allocation to succeed.
      if (c->temp_start) {
                           vir_for_each_block(block, c) {
            ralloc_free(block->def);
   ralloc_free(block->defin);
   ralloc_free(block->defout);
   ralloc_free(block->use);
            c->temp_start = rzalloc_array(c, int, c->num_temps);
            for (int i = 0; i < c->num_temps; i++) {
                        vir_for_each_block(block, c) {
            block->def = rzalloc_array(c, BITSET_WORD, bitset_words);
   block->defin = rzalloc_array(c, BITSET_WORD, bitset_words);
   block->defout = rzalloc_array(c, BITSET_WORD, bitset_words);
   block->use = rzalloc_array(c, BITSET_WORD, bitset_words);
                        while (vir_live_variables_dataflow(c, bitset_words))
            while (vir_live_variables_defin_defout_dataflow(c, bitset_words))
                     }
