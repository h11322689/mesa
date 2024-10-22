   /*
   * Copyright © 2014 Intel Corporation
   * Copyright © 2021 Valve Corporation
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
   *
   */
      #include "ir3.h"
   #include "util/ralloc.h"
      /*
   * Implements the algorithms for computing the dominance tree and the
   * dominance frontier from "A Simple, Fast Dominance Algorithm" by Cooper,
   * Harvey, and Kennedy.
   */
      static struct ir3_block *
   intersect(struct ir3_block *b1, struct ir3_block *b2)
   {
      while (b1 != b2) {
      /*
   * Note, the comparisons here are the opposite of what the paper says
   * because we index blocks from beginning -> end (i.e. reverse
   * post-order) instead of post-order like they assume.
   */
   while (b1->index > b2->index)
         while (b2->index > b1->index)
                  }
      static bool
   calc_dominance(struct ir3_block *block)
   {
      struct ir3_block *new_idom = NULL;
   for (unsigned i = 0; i < block->predecessors_count; i++) {
               if (pred->imm_dom) {
      if (new_idom)
         else
                  if (block->imm_dom != new_idom) {
      block->imm_dom = new_idom;
                  }
      static unsigned
   calc_dfs_indices(struct ir3_block *block, unsigned index)
   {
      block->dom_pre_index = index++;
   for (unsigned i = 0; i < block->dom_children_count; i++)
         block->dom_post_index = index++;
      }
      void
   ir3_calc_dominance(struct ir3 *ir)
   {
      unsigned i = 0;
   foreach_block (block, &ir->block_list) {
      block->index = i++;
   if (block == ir3_start_block(ir))
         else
         block->dom_children = NULL;
               bool progress = true;
   while (progress) {
      progress = false;
   foreach_block (block, &ir->block_list) {
      if (block != ir3_start_block(ir))
                           foreach_block (block, &ir->block_list) {
      if (block->imm_dom)
                  }
      /* Return true if a dominates b. This includes if a == b. */
   bool
   ir3_block_dominates(struct ir3_block *a, struct ir3_block *b)
   {
      return a->dom_pre_index <= b->dom_pre_index &&
      }
