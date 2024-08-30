   /*
   * Copyright (C) 2019 Collabora, Ltd.
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
   *    Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   */
      #include "compiler.h"
      /* Midgard texture/derivative operations have a pair of bits controlling the
   * behaviour of helper invocations:
   *
   *  - Should a helper invocation terminate after executing this instruction?
   *  - Should a helper invocation actually execute this instruction?
   *
   * The terminate bit should be set on the last instruction requiring helper
   * invocations. Without control flow, that's literally the last instruction;
   * with control flow, there may be multiple such instructions (with ifs) or no
   * such instruction (with loops).
   *
   * The execute bit should be set if the value of this instruction is required
   * by a future instruction requiring helper invocations. Consider:
   *
   *      0 = texture ...
   *      1 = fmul 0, #10
   *      2 = dfdx 1
   *      store 2
   *
   * Since the derivative calculation 2 requires helper invocations, the value 1
   * must be calculated by helper invocations, and since it depends on 0, 0 must
   * be calculated by helpers. Hence the texture op has the execute bit set, and
   * the derivative op has the terminate bit set.
   *
   * Calculating the terminate bit occurs by forward dataflow analysis to
   * determine which blocks require helper invocations. A block requires
   * invocations in if any of its instructions use helper invocations, or if it
   * depends on a block that requires invocation. With that analysis, the
   * terminate bit is set on the last instruction using invocations within any
   * block that does *not* require invocations out.
   *
   * Likewise, calculating the execute bit requires backward dataflow analysis
   * with union as the join operation and the generating set being the union of
   * sources of instructions writing executed values.
   */
      /* Does a block use helpers directly */
   static bool
   mir_block_uses_helpers(gl_shader_stage stage, midgard_block *block)
   {
      mir_foreach_instr_in_block(block, ins) {
      if (ins->type != TAG_TEXTURE_4)
         if (mir_op_computes_derivatives(stage, ins->op))
                  }
      static bool
   mir_block_terminates_helpers(midgard_block *block)
   {
      /* Can't terminate if there are no helpers */
   if (!block->helpers_in)
            /* Can't terminate if a successor needs helpers */
   pan_foreach_successor((&block->base), succ) {
      if (((midgard_block *)succ)->helpers_in)
               /* Otherwise we terminate */
      }
      void
   mir_analyze_helper_terminate(compiler_context *ctx)
   {
      /* Set blocks as directly requiring helpers, and if they do add them to
            struct set *worklist =
            struct set *visited =
            mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
            if (block->helpers_in)
               /* Next, propagate back. Since there are a finite number of blocks, the
   * worklist (a subset of all the blocks) is finite. Since a block can
   * only be added to the worklist if it is not on the visited list and
   * the visited list - also a subset of the blocks - grows every
                     while ((cur = _mesa_set_next_entry(worklist, NULL)) != NULL) {
      /* Pop off a block requiring helpers */
   pan_block *blk = (struct pan_block *)cur->key;
            /* Its predecessors also require helpers */
   pan_foreach_predecessor(blk, pred) {
      if (!_mesa_set_search(visited, pred)) {
      ((midgard_block *)pred)->helpers_in = true;
                              _mesa_set_destroy(visited, NULL);
            /* Finally, set helper_terminate on the last derivative-calculating
   * instruction in a block that terminates helpers */
   mir_foreach_block(ctx, _block) {
               if (!mir_block_terminates_helpers(block))
            mir_foreach_instr_in_block_rev(block, ins) {
      if (ins->type != TAG_TEXTURE_4)
                        ins->helper_terminate = true;
            }
      static bool
   mir_helper_block_update(BITSET_WORD *deps, pan_block *_block,
         {
      bool progress = false;
            mir_foreach_instr_in_block_rev(block, ins) {
      /* Ensure we write to a helper dependency */
   if (ins->dest >= temp_count || !BITSET_TEST(deps, ins->dest))
            /* Then add all of our dependencies */
   mir_foreach_src(ins, s) {
                     /* Progress if the dependency set changes */
   progress |= !BITSET_TEST(deps, ins->src[s]);
                     }
      void
   mir_analyze_helper_requirements(compiler_context *ctx)
   {
      mir_compute_temp_count(ctx);
   unsigned temp_count = ctx->temp_count;
            /* Initialize with the sources of instructions consuming
            mir_foreach_instr_global(ctx, ins) {
      if (ins->type != TAG_TEXTURE_4)
         if (ins->dest >= ctx->temp_count)
         if (!mir_op_computes_derivatives(ctx->stage, ins->op))
            mir_foreach_src(ins, s) {
      if (ins->src[s] < temp_count)
                           struct set *work_list =
            struct set *visited =
            struct set_entry *cur =
            do {
      pan_block *blk = (struct pan_block *)cur->key;
                     if (progress || !_mesa_set_search(visited, blk)) {
      pan_foreach_predecessor(blk, pred)
                           _mesa_set_destroy(visited, NULL);
                     mir_foreach_instr_global(ctx, ins) {
      if (ins->type != TAG_TEXTURE_4)
         if (ins->dest >= ctx->temp_count)
                           }
