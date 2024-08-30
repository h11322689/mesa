   /*
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
      #include "util/list.h"
   #include "util/set.h"
   #include "util/u_memory.h"
   #include "pan_ir.h"
      /* Routines for liveness analysis. Liveness is tracked per byte per node. Per
   * byte granularity is necessary for proper handling of int8 */
      void
   pan_liveness_gen(uint16_t *live, unsigned node, unsigned max, uint16_t mask)
   {
      if (node >= max)
               }
      void
   pan_liveness_kill(uint16_t *live, unsigned node, unsigned max, uint16_t mask)
   {
      if (node >= max)
               }
      bool
   pan_liveness_get(uint16_t *live, unsigned node, uint16_t max)
   {
      if (node >= max)
               }
      /* live_out[s] = sum { p in succ[s] } ( live_in[p] ) */
      static void
   liveness_block_live_out(pan_block *blk, unsigned temp_count)
   {
      pan_foreach_successor(blk, succ) {
      for (unsigned i = 0; i < temp_count; ++i)
         }
      /* Liveness analysis is a backwards-may dataflow analysis pass. Within a block,
   * we compute live_out from live_in. The intrablock pass is linear-time. It
   * returns whether progress was made. */
      static bool
   liveness_block_update(pan_block *blk, unsigned temp_count,
         {
                        uint16_t *live = ralloc_array(blk, uint16_t, temp_count);
            pan_foreach_instr_in_block_rev(blk, ins)
                     for (unsigned i = 0; (i < temp_count) && !progress; ++i)
            ralloc_free(blk->live_in);
               }
      /* Globally, liveness analysis uses a fixed-point algorithm based on a
   * worklist. We initialize a work list with the exit block. We iterate the work
   * list to compute live_in from live_out for each block on the work list,
   * adding the predecessors of the block to the work list if we made progress.
   */
      void
   pan_compute_liveness(struct list_head *blocks, unsigned temp_count,
         {
         /* Set of pan_block */
   struct set *work_list =
            struct set *visited =
                              list_for_each_entry(pan_block, block, blocks, link) {
      block->live_in = rzalloc_array(block, uint16_t, temp_count);
               /* Initialize the work list with the exit block */
                              do {
      /* Pop off a block */
   pan_block *blk = (struct pan_block *)cur->key;
            /* Update its liveness information */
                     if (progress || !_mesa_set_search(visited, blk)) {
      pan_foreach_predecessor(blk, pred)
                           _mesa_set_destroy(visited, NULL);
      }
      void
   pan_free_liveness(struct list_head *blocks)
   {
      list_for_each_entry(pan_block, block, blocks, link) {
      if (block->live_in)
            if (block->live_out)
            block->live_in = NULL;
         }
