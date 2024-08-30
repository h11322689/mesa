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
   */
      #include "va_compiler.h"
   #include "valhall_enums.h"
      /*
   * Valhall sources may marked as the last use of a register, according
   * to the following rules:
   *
   * 1. The last use of a register should be marked allowing the hardware
   *    to elide register writes.
   * 2. Staging sources may be read at any time before the asynchronous
   *    instruction completes. If a register is used as both a staging source and
   *    a regular source, the regular source cannot be marked until the program
   *    waits for the asynchronous instruction.
   * 3. Marking a register pair marks both registers in the pair.
   *
   * Last use information follows immediately from (post-RA) liveness analysis:
   * a register is dead immediately after its last use.
   *
   * Staging information follows from scoreboard analysis: do not mark registers
   * that are read by a pending asynchronous instruction. Note that the Valhall
   * scoreboard analysis does not track reads, so we handle that with our own
   * (simplified) scoreboard analysis.
   *
   * Register pairs are marked conservatively: if either register in a pair cannot
   * be marked, do not mark either register.
   */
      static uint64_t
   bi_staging_read_mask(const bi_instr *I)
   {
               bi_foreach_src(I, s) {
      if (bi_is_staging_src(I, s) && !bi_is_null(I->src[s])) {
      assert(I->src[s].type == BI_INDEX_REGISTER);
                                    }
      static bool
   bi_writes_reg(const bi_instr *I, unsigned reg)
   {
      bi_foreach_dest(I, d) {
                        if (reg >= I->dest[d].value && (reg - I->dest[d].value) < count)
                  }
      static unsigned
   waits_on_slot(enum va_flow flow, unsigned slot)
   {
      return (flow == VA_FLOW_WAIT) || (flow == VA_FLOW_WAIT0126) ||
      }
      static void
   scoreboard_update(struct bi_scoreboard_state *st, const bi_instr *I)
   {
      /* Mark read staging registers */
            /* Unmark registers after they are waited on */
   for (unsigned i = 0; i < VA_NUM_GENERAL_SLOTS; ++i) {
      if (waits_on_slot(I->flow, i))
         }
      static void
   va_analyze_scoreboard_reads(bi_context *ctx)
   {
      u_worklist worklist;
            bi_foreach_block(ctx, block) {
               /* Reset analysis from previous pass */
   block->scoreboard_in = (struct bi_scoreboard_state){0};
               /* Perform forward data flow analysis to calculate dependencies */
   while (!u_worklist_is_empty(&worklist)) {
      /* Pop from the front for forward analysis */
            bi_foreach_predecessor(blk, pred) {
      for (unsigned i = 0; i < VA_NUM_GENERAL_SLOTS; ++i)
                        bi_foreach_instr_in_block(blk, I)
            /* If there was progress, reprocess successors */
   if (memcmp(&state, &blk->scoreboard_out, sizeof(state)) != 0) {
      bi_foreach_successor(blk, succ)
                              }
      void
   va_mark_last(bi_context *ctx)
   {
      /* Analyze the shader globally */
   bi_postra_liveness(ctx);
            bi_foreach_block(ctx, block) {
               /* Mark all last uses */
   bi_foreach_instr_in_block_rev(block, I) {
      bi_foreach_src(I, s) {
                                                   /* If the register is overwritten this cycle, it is implicitly
   * discarded, but that won't show up in the liveness analysis.
   */
                                    bi_foreach_instr_in_block(block, I) {
      /* Unmark registers read by a pending async instruction */
   bi_foreach_src(I, s) {
                                             if (bi_is_staging_src(I, s) || pending)
               /* Unmark register pairs where one half must be preserved */
   bi_foreach_src(I, s) {
      /* Only look for "real" architectural registers */
                                    I->src[s + 0].discard = both_discard;
                           }
