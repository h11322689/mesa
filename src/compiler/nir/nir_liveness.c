   /*
   * Copyright © 2014 Intel Corporation
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
      #include "nir.h"
   #include "nir_vla.h"
   #include "nir_worklist.h"
      /*
   * Basic liveness analysis.  This works only in SSA form.
   *
   * This liveness pass treats phi nodes as being melded to the space between
   * blocks so that the destinations of a phi are in the livein of the block
   * in which it resides and the sources are in the liveout of the
   * corresponding block.  By formulating the liveness information in this
   * way, we ensure that the definition of any variable dominates its entire
   * live range.  This is true because the only way that the definition of an
   * SSA value may not dominate a use is if the use is in a phi node and the
   * uses in phi no are in the live-out of the corresponding predecessor
   * block but not in the live-in of the block containing the phi node.
   */
      struct live_defs_state {
               /* Used in propagate_across_edge() */
               };
      /* Initialize the liveness data to zero and add the given block to the
   * worklist.
   */
   static void
   init_liveness_block(nir_block *block,
         {
      block->live_in = reralloc(block, block->live_in, BITSET_WORD,
                  block->live_out = reralloc(block, block->live_out, BITSET_WORD,
                     }
      static bool
   set_src_live(nir_src *src, void *void_live)
   {
               if (nir_src_is_undef(*src))
                        }
      static bool
   set_ssa_def_dead(nir_def *def, void *void_live)
   {
                           }
      /** Propagates the live in of succ across the edge to the live out of pred
   *
   * Phi nodes exist "between" blocks and all the phi nodes at the start of a
   * block act "in parallel".  When we propagate from the live_in of one
   * block to the live out of the other, we have to kill any writes from phis
   * and make live any sources.
   *
   * Returns true if updating live out of pred added anything
   */
   static bool
   propagate_across_edge(nir_block *pred, nir_block *succ,
         {
      BITSET_WORD *live = state->tmp_live;
            nir_foreach_phi(phi, succ) {
                  nir_foreach_phi(phi, succ) {
      nir_foreach_phi_src(src, phi) {
      if (src->pred == pred) {
      set_src_live(&src->src, live);
                     BITSET_WORD progress = 0;
   for (unsigned i = 0; i < state->bitset_words; ++i) {
      progress |= live[i] & ~pred->live_out[i];
      }
      }
      void
   nir_live_defs_impl(nir_function_impl *impl)
   {
      struct live_defs_state state = {
         };
            /* Number the instructions so we can do cheap interference tests using the
   * instruction index.
   */
                     /* Allocate live_in and live_out sets and add all of the blocks to the
   * worklist.
   */
   nir_foreach_block(block, impl) {
                  /* We're now ready to work through the worklist and update the liveness
   * sets of each of the blocks.  By the time we get to this point, every
   * block in the function implementation has been pushed onto the
   * worklist in reverse order.  As long as we keep the worklist
   * up-to-date as we go, everything will get covered.
   */
   while (!nir_block_worklist_is_empty(&state.worklist)) {
      /* We pop them off in the reverse order we pushed them on.  This way
   * the first walk of the instructions is backwards so we only walk
   * once in the case of no control flow.
   */
            memcpy(block->live_in, block->live_out,
            nir_if *following_if = nir_block_get_following_if(block);
   if (following_if)
            nir_foreach_instr_reverse(instr, block) {
      /* Phi nodes are handled seperately so we want to skip them.  Since
   * we are going backwards and they are at the beginning, we can just
   * break as soon as we see one.
   */
                  nir_foreach_def(instr, set_ssa_def_dead, block->live_in);
               /* Walk over all of the predecessors of the current block updating
   * their live in with the live out of this one.  If anything has
   * changed, add the predecessor to the work list so that we ensure
   * that the new information is used.
   */
   set_foreach(block->predecessors, entry) {
      nir_block *pred = (nir_block *)entry->key;
   if (propagate_across_edge(pred, block, &state))
                  ralloc_free(state.tmp_live);
      }
      /** Return the live set at a cursor
   *
   * Note: The bitset returned may be the live_in or live_out from the block in
   *       which the instruction lives.  Do not ralloc_free() it directly;
   *       instead, provide a mem_ctx and free that.
   */
   const BITSET_WORD *
   nir_get_live_defs(nir_cursor cursor, void *mem_ctx)
   {
      nir_block *block = nir_cursor_current_block(cursor);
   nir_function_impl *impl = nir_cf_node_get_function(&block->cf_node);
            switch (cursor.option) {
   case nir_cursor_before_block:
            case nir_cursor_after_block:
            case nir_cursor_before_instr:
      if (cursor.instr == nir_block_first_instr(cursor.instr->block))
               case nir_cursor_after_instr:
      if (cursor.instr == nir_block_last_instr(cursor.instr->block))
                     /* If we got here, we're an instruction cursor mid-block */
   const unsigned bitset_words = BITSET_WORDS(impl->ssa_alloc);
   BITSET_WORD *live = ralloc_array(mem_ctx, BITSET_WORD, bitset_words);
            nir_foreach_instr_reverse(instr, block) {
      if (cursor.option == nir_cursor_after_instr && instr == cursor.instr)
            /* If someone asked for liveness in the middle of a bunch of phis,
   * that's an error.  Since we are going backwards and they are at the
   * beginning, we can just blow up as soon as we see one.
   */
   assert(instr->type != nir_instr_type_phi);
   if (instr->type == nir_instr_type_phi)
            nir_foreach_def(instr, set_ssa_def_dead, live);
            if (cursor.option == nir_cursor_before_instr && instr == cursor.instr)
                  }
      static bool
   src_does_not_use_def(nir_src *src, void *def)
   {
         }
      static bool
   search_for_use_after_instr(nir_instr *start, nir_def *def)
   {
      /* Only look for a use strictly after the given instruction */
   struct exec_node *node = start->node.next;
   while (!exec_node_is_tail_sentinel(node)) {
      nir_instr *instr = exec_node_data(nir_instr, node, node);
   if (!nir_foreach_src(instr, src_does_not_use_def, def))
                     /* If uses are considered to be in the block immediately preceding the if
   * so we need to also check the following if condition, if any.
   */
   nir_if *following_if = nir_block_get_following_if(start->block);
   if (following_if && following_if->condition.ssa == def)
               }
      /* Returns true if def is live at instr assuming that def comes before
   * instr in a pre DFS search of the dominance tree.
   */
   static bool
   nir_def_is_live_at(nir_def *def, nir_instr *instr)
   {
      if (BITSET_TEST(instr->block->live_out, def->index)) {
      /* Since def dominates instr, if def is in the liveout of the block,
   * it's live at instr
   */
      } else {
      if (BITSET_TEST(instr->block->live_in, def->index) ||
      def->parent_instr->block == instr->block) {
   /* In this case it is either live coming into instr's block or it
   * is defined in the same block.  In this case, we simply need to
   * see if it is used after instr.
   */
      } else {
               }
      bool
   nir_defs_interfere(nir_def *a, nir_def *b)
   {
      if (a->parent_instr == b->parent_instr) {
      /* Two variables defined at the same time interfere assuming at
   * least one isn't dead.
   */
      } else if (a->parent_instr->type == nir_instr_type_undef ||
            /* If either variable is an ssa_undef, then there's no interference */
      } else if (a->parent_instr->index < b->parent_instr->index) {
         } else {
            }
