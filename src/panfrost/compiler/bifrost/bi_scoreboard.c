   /*
   * Copyright (C) 2020 Collabora Ltd.
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
      #include "compiler.h"
      /* Assign dependency slots to each clause and calculate dependencies, This pass
   * must be run after scheduling.
   *
   * 1. A clause that does not produce a message must use the sentinel slot #0
   * 2a. A clause that depends on the results of a previous message-passing
   * instruction must depend on that instruction's dependency slot, unless all
   * reaching code paths already depended on it.
   * 2b. More generally, any dependencies must be encoded. This includes
   * Write-After-Write and Write-After-Read hazards with LOAD/STORE to memory.
   * 3. The shader must wait on slot #6 before running BLEND, ATEST
   * 4. The shader must wait on slot #7 before running BLEND, ST_TILE
   * 5. ATEST, ZS_EMIT must be issued with slot #0
   * 6. BARRIER must be issued with slot #7 and wait on every active slot.
   * 7. Only slots #0 through #5 may be used for clauses not otherwise specified.
   * 8. If a clause writes to a read staging register of an unresolved
   * dependency, it must set a staging barrier.
   *
   * Note it _is_ legal to reuse slots for multiple message passing instructions
   * with overlapping liveness, albeit with a slight performance penalty. As such
   * the problem is significantly easier than register allocation, rather than
   * spilling we may simply reuse slots. (TODO: does this have an optimal
   * linear-time solution).
   *
   * Within these constraints we are free to assign slots as we like. This pass
   * attempts to minimize stalls (TODO).
   */
      #define BI_NUM_GENERAL_SLOTS 6
   #define BI_NUM_SLOTS         8
   #define BI_NUM_REGISTERS     64
   #define BI_SLOT_SERIAL       0 /* arbitrary */
      /*
   * Due to the crude scoreboarding we do, we need to serialize varying loads and
   * memory access. Identify these instructions here.
   */
   static bool
   bi_should_serialize(bi_instr *I)
   {
      /* For debug, serialize everything to disable scoreboard opts */
   if (bifrost_debug & BIFROST_DBG_NOSB)
            /* Although nominally on the attribute unit, image loads have the same
   * coherency requirements as general memory loads. Serialize them for
   * now until we can do something more clever.
   */
   if (I->op == BI_OPCODE_LD_ATTR_TEX)
            switch (bi_opcode_props[I->op].message) {
   case BIFROST_MESSAGE_VARYING:
   case BIFROST_MESSAGE_LOAD:
   case BIFROST_MESSAGE_STORE:
   case BIFROST_MESSAGE_ATOMIC:
         default:
            }
      /* Given a scoreboard model, choose a slot for a clause wrapping a given
   * message passing instruction. No side effects. */
      static unsigned
   bi_choose_scoreboard_slot(bi_instr *message)
   {
      /* ATEST, ZS_EMIT must be issued with slot #0 */
   if (message->op == BI_OPCODE_ATEST || message->op == BI_OPCODE_ZS_EMIT)
            /* BARRIER must be issued with slot #7 */
   if (message->op == BI_OPCODE_BARRIER)
            /* For now, make serialization is easy */
   if (bi_should_serialize(message))
               }
      static uint64_t
   bi_read_mask(bi_instr *I, bool staging_only)
   {
               if (staging_only && !bi_opcode_props[I->op].sr_read)
            bi_foreach_src(I, s) {
      if (I->src[s].type == BI_INDEX_REGISTER) {
                                 if (staging_only)
                  }
      static uint64_t
   bi_write_mask(bi_instr *I)
   {
               bi_foreach_dest(I, d) {
      if (bi_is_null(I->dest[d]))
                     unsigned reg = I->dest[d].value;
                        /* Instructions like AXCHG.i32 unconditionally both read and write
   * staging registers. Even if we discard the result, the write still
   * happens logically and needs to be included in our calculations.
   * Obscurely, ATOM_CX is sr_write but can ignore the staging register in
   * certain circumstances; this does not require consideration.
   */
   if (bi_opcode_props[I->op].sr_write && I->nr_dests && I->nr_srcs &&
               unsigned reg = I->src[0].value;
                           }
      /* Update the scoreboard model to assign an instruction to a given slot */
      static void
   bi_push_clause(struct bi_scoreboard_state *st, bi_clause *clause)
   {
      bi_instr *I = clause->message;
            if (!I)
                     if (bi_opcode_props[I->op].sr_write)
      }
      /* Adds a dependency on each slot writing any specified register */
      static void
   bi_depend_on_writers(bi_clause *clause, struct bi_scoreboard_state *st,
         {
      for (unsigned slot = 0; slot < ARRAY_SIZE(st->write); ++slot) {
      if (!(st->write[slot] & regmask))
            st->write[slot] = 0;
                  }
      static void
   bi_set_staging_barrier(bi_clause *clause, struct bi_scoreboard_state *st,
         {
      for (unsigned slot = 0; slot < ARRAY_SIZE(st->read); ++slot) {
      if (!(st->read[slot] & regmask))
            st->read[slot] = 0;
         }
      /* Sets the dependencies for a given clause, updating the model */
      static void
   bi_set_dependencies(bi_block *block, bi_clause *clause,
         {
      bi_foreach_instr_in_clause(block, clause, I) {
      uint64_t read = bi_read_mask(I, false);
            /* Read-after-write; write-after-write */
            /* Write-after-read */
               /* LD_VAR instructions must be serialized per-quad. Just always depend
   * on any LD_VAR instructions. This isn't optimal, but doing better
   * requires divergence-aware data flow analysis.
   *
   * Similarly, memory loads/stores need to be synchronized. For now,
   * force them to be serialized. This is not optimal.
   */
   if (clause->message && bi_should_serialize(clause->message))
            /* Barriers must wait on all slots to flush existing work. It might be
   * possible to skip this with more information about the barrier. For
   * now, be conservative.
   */
   if (clause->message && clause->message->op == BI_OPCODE_BARRIER)
      }
      static bool
   scoreboard_block_update(bi_block *blk)
   {
               /* pending_in[s] = sum { p in pred[s] } ( pending_out[p] ) */
   bi_foreach_predecessor(blk, pred) {
      for (unsigned i = 0; i < BI_NUM_SLOTS; ++i) {
      blk->scoreboard_in.read[i] |= (*pred)->scoreboard_out.read[i];
                                    bi_foreach_clause_in_block(blk, clause) {
      bi_set_dependencies(blk, clause, &state);
                        for (unsigned i = 0; i < BI_NUM_SLOTS; ++i)
                        }
      void
   bi_assign_scoreboard(bi_context *ctx)
   {
      u_worklist worklist;
            /* First, assign slots. */
   bi_foreach_block(ctx, block) {
      bi_foreach_clause_in_block(block, clause) {
      if (clause->message) {
      unsigned slot = bi_choose_scoreboard_slot(clause->message);
                              /* Next, perform forward data flow analysis to calculate dependencies */
   while (!u_worklist_is_empty(&worklist)) {
      /* Pop from the front for forward analysis */
            if (scoreboard_block_update(blk)) {
      bi_foreach_successor(blk, succ)
                     }
