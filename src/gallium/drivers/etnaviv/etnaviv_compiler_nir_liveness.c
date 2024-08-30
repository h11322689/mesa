   /*
   * Copyright (c) 2019 Zodiac Inflight Innovations
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Jonathan Marek <jonathan@marek.ca>
   */
      #include "etnaviv_compiler_nir.h"
   #include "compiler/nir/nir_worklist.h"
      static void
   range_include(struct live_def *def, unsigned index)
   {
      if (def->live_start > index)
         if (def->live_end < index)
      }
      struct live_defs_state {
      unsigned num_defs;
            nir_function_impl *impl;
   nir_block *block; /* current block pointer */
            struct live_def *defs;
               };
      static bool
   init_liveness_block(nir_block *block,
         {
      block->live_in = reralloc(block, block->live_in, BITSET_WORD,
                  block->live_out = reralloc(block, block->live_out, BITSET_WORD,
                              }
      static bool
   set_src_live(nir_src *src, void *void_state)
   {
                        if (is_sysval(instr) || instr->type == nir_instr_type_deref)
            switch (instr->type) {
   case nir_instr_type_load_const:
   case nir_instr_type_undef:
         case nir_instr_type_alu: {
      /* alu op bypass */
   nir_alu_instr *alu = nir_instr_as_alu(instr);
   if (instr->pass_flags & BYPASS_SRC) {
      for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++)
            }
      }
   default:
                  unsigned i = state->live_map[src_index(state->impl, src)];
            BITSET_SET(state->block->live_in, i);
               }
      static bool
   propagate_across_edge(nir_block *pred, nir_block *succ,
         {
      BITSET_WORD progress = 0;
   for (unsigned i = 0; i < state->bitset_words; ++i) {
      progress |= succ->live_in[i] & ~pred->live_out[i];
      }
      }
      unsigned
   etna_live_defs(nir_function_impl *impl, struct live_def *defs, unsigned *live_map)
   {
      struct live_defs_state state;
            state.impl = impl;
   state.defs = defs;
            state.num_defs = 0;
   nir_foreach_block(block, impl) {
      block_live_index[block->index] = state.num_defs;
   nir_foreach_instr(instr, block) {
      nir_def *def = def_for_instr(instr);
                  unsigned idx = def_index(impl, def);
   /* register is already in defs */
                           /* input live from the start */
   if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic == nir_intrinsic_load_input ||
                     live_map[idx] = state.num_defs;
         }
                     /* We now know how many unique ssa definitions we have and we can go
   * ahead and allocate live_in and live_out sets and add all of the
   * blocks to the worklist.
   */
   state.bitset_words = BITSET_WORDS(state.num_defs);
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
   nir_block *block = nir_block_worklist_pop_head(&state.worklist);
            memcpy(block->live_in, block->live_out,
                     nir_if *following_if = nir_block_get_following_if(block);
   if (following_if)
            nir_foreach_instr_reverse(instr, block) {
      /* when we come across the next "live" instruction, decrement index */
   if (state.index && instr == defs[state.index - 1].instr) {
      state.index--;
   /* the only source of writes to registers is phis:
   * we don't expect any partial write_mask alus
   * so clearing live_in here is OK
   */
               /* don't set_src_live for not-emitted instructions */
                           /* output live till the end */
   if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic == nir_intrinsic_store_deref)
                        if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic == nir_intrinsic_decl_reg ||
                                       }
            /* Walk over all of the predecessors of the current block updating
   * their live in with the live out of this one.  If anything has
   * changed, add the predecessor to the work list so that we ensure
   * that the new information is used.
   */
   set_foreach(block->predecessors, entry) {
      nir_block *pred = (nir_block *)entry->key;
   if (propagate_across_edge(pred, block, &state))
                                    nir_foreach_block(block, impl) {
               BITSET_FOREACH_SET(i, block->live_in, state.num_defs)
            BITSET_FOREACH_SET(i, block->live_out, state.num_defs)
                  }
