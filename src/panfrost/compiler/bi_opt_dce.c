   /*
   * Copyright (C) 2018 Alyssa Rosenzweig
   * Copyright (C) 2019-2020 Collabora, Ltd.
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
      #include "util/u_memory.h"
   #include "compiler.h"
      /* A simple SSA-based mark-and-sweep dead code elimination pass. */
      void
   bi_opt_dead_code_eliminate(bi_context *ctx)
   {
      /* Mark live values */
   BITSET_WORD *mark =
            u_worklist worklist;
            bi_foreach_block(ctx, block) {
                  while (!u_worklist_is_empty(&worklist)) {
      /* Pop in reverse order for backwards pass */
                     bi_foreach_instr_in_block_rev(blk, I) {
                                             bi_foreach_ssa_src(I, s) {
      progress |= !BITSET_TEST(mark, I->src[s].value);
                  /* XXX: slow */
   if (progress) {
      bi_foreach_block(ctx, block)
                           /* Sweep */
   bi_foreach_instr_global_safe(ctx, I) {
               bi_foreach_dest(I, d)
            if (!needed)
                  }
      /* Post-RA liveness-based dead code analysis to clean up results of bundling */
      uint64_t MUST_CHECK
   bi_postra_liveness_ins(uint64_t live, bi_instr *ins)
   {
      bi_foreach_dest(ins, d) {
      if (ins->dest[d].type == BI_INDEX_REGISTER) {
      unsigned nr = bi_count_write_registers(ins, d);
   unsigned reg = ins->dest[d].value;
                  bi_foreach_src(ins, s) {
      if (ins->src[s].type == BI_INDEX_REGISTER) {
      unsigned nr = bi_count_read_registers(ins, s);
   unsigned reg = ins->src[s].value;
                     }
      static bool
   bi_postra_liveness_block(bi_block *blk)
   {
      bi_foreach_successor(blk, succ)
                     bi_foreach_instr_in_block_rev(blk, ins)
            bool progress = blk->reg_live_in != live;
   blk->reg_live_in = live;
      }
      /* Globally, liveness analysis uses a fixed-point algorithm based on a
   * worklist. We initialize a work list with the exit block. We iterate the work
   * list to compute live_in from live_out for each block on the work list,
   * adding the predecessors of the block to the work list if we made progress.
   */
      void
   bi_postra_liveness(bi_context *ctx)
   {
      u_worklist worklist;
            bi_foreach_block(ctx, block) {
                           while (!u_worklist_is_empty(&worklist)) {
      /* Pop off in reverse order since liveness is backwards */
            /* Update liveness information. If we made progress, we need to
   * reprocess the predecessors
   */
   if (bi_postra_liveness_block(blk)) {
      bi_foreach_predecessor(blk, pred)
                     }
      void
   bi_opt_dce_post_ra(bi_context *ctx)
   {
               bi_foreach_block_rev(ctx, block) {
               bi_foreach_instr_in_block_rev(block, ins) {
                     bi_foreach_dest(ins, d) {
                     unsigned nr = bi_count_write_registers(ins, d);
   unsigned reg = ins->dest[d].value;
   uint64_t mask = (BITFIELD64_MASK(nr) << reg);
                  if (!(live & mask) && cullable)
                        }
