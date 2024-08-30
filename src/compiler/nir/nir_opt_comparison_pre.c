   /*
   * Copyright © 2018 Intel Corporation
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
      #include "util/u_vector.h"
   #include "nir_builder.h"
   #include "nir_instr_set.h"
   #include "nir_search_helpers.h"
      /* Partial redundancy elimination of compares
   *
   * Seaches for comparisons of the form 'a cmp b' that dominate arithmetic
   * instructions like 'b - a'.  The comparison is replaced by the arithmetic
   * instruction, and the result is compared with zero.  For example,
   *
   *       vec1 32 ssa_111 = flt 0.37, ssa_110.w
   *       if ssa_111 {
   *               block block_1:
   *              vec1 32 ssa_112 = fadd ssa_110.w, -0.37
   *              ...
   *
   * becomes
   *
   *       vec1 32 ssa_111 = fadd ssa_110.w, -0.37
   *       vec1 32 ssa_112 = flt 0.0, ssa_111
   *       if ssa_112 {
   *               block block_1:
   *              ...
   */
      struct block_queue {
      /**
   * Stack of blocks from the current location in the CFG to the entry point
   * of the function.
   *
   * This is sort of a poor man's dominator tree.
   */
            /** List of freed block_instructions structures that can be reused. */
      };
      struct block_instructions {
               /**
   * Set of comparison instructions from the block that are candidates for
   * being replaced by add instructions.
   */
      };
      static void
   block_queue_init(struct block_queue *bq)
   {
      exec_list_make_empty(&bq->blocks);
      }
      static void
   block_queue_finish(struct block_queue *bq)
   {
               while ((n = (struct block_instructions *)exec_list_pop_head(&bq->blocks)) != NULL) {
      u_vector_finish(&n->instructions);
               while ((n = (struct block_instructions *)exec_list_pop_head(&bq->reusable_blocks)) != NULL) {
            }
      static struct block_instructions *
   push_block(struct block_queue *bq)
   {
      struct block_instructions *bi =
            if (bi == NULL) {
               if (bi == NULL)
               if (!u_vector_init_pow2(&bi->instructions, 8, sizeof(nir_alu_instr *))) {
      free(bi);
                           }
      static void
   pop_block(struct block_queue *bq, struct block_instructions *bi)
   {
      u_vector_finish(&bi->instructions);
   exec_node_remove(&bi->node);
      }
      static void
   add_instruction_for_block(struct block_instructions *bi,
         {
      nir_alu_instr **data =
               }
      /**
   * Determine if the ALU instruction is used by an if-condition or used by a
   * logic-not that is used by an if-condition.
   */
   static bool
   is_compatible_condition(const nir_alu_instr *instr)
   {
      if (is_used_by_if(instr))
            nir_foreach_use(src, &instr->def) {
               if (user_instr->type != nir_instr_type_alu)
                     if (user_alu->op != nir_op_inot)
            if (is_used_by_if(user_alu))
                  }
      static void
   rewrite_compare_instruction(nir_builder *bld, nir_alu_instr *orig_cmp,
         {
               /* This is somewhat tricky.  The compare instruction may be something like
   * (fcmp, a, b) while the add instruction is something like (fadd, fneg(a),
   * b).  This is problematic because the SSA value for the fneg(a) may not
   * exist yet at the compare instruction.
   *
   * We fabricate the operands of the new add.  This is done using
   * information provided by zero_on_left.  If zero_on_left is true, we know
   * the resulting compare instruction is (fcmp, 0.0, (fadd, x, y)).  If the
   * original compare instruction was (fcmp, a, b), x = b and y = -a.  If
   * zero_on_left is false, the resulting compare instruction is (fcmp,
   * (fadd, x, y), 0.0) and x = a and y = -b.
   */
   nir_def *const a = nir_ssa_for_alu_src(bld, orig_cmp, 0);
            nir_def *const fadd = zero_on_left
                  nir_def *const zero =
            nir_def *const cmp = zero_on_left
                  /* Generating extra moves of the results is the easy way to make sure the
   * writemasks match the original instructions.  Later optimization passes
   * will clean these up.  This is similar to nir_replace_instr (in
   * nir_search.c).
   */
   nir_alu_instr *mov_add = nir_alu_instr_create(bld->shader, nir_op_mov);
   nir_def_init(&mov_add->instr, &mov_add->def,
                                 nir_alu_instr *mov_cmp = nir_alu_instr_create(bld->shader, nir_op_mov);
   nir_def_init(&mov_cmp->instr, &mov_cmp->def,
                                 nir_def_rewrite_uses(&orig_cmp->def,
         nir_def_rewrite_uses(&orig_add->def,
            /* We know these have no more uses because we just rewrote them all, so we
   * can remove them.
   */
   nir_instr_remove(&orig_cmp->instr);
      }
      static bool
   comparison_pre_block(nir_block *block, struct block_queue *bq, nir_builder *bld)
   {
               struct block_instructions *bi = push_block(bq);
   if (bi == NULL)
            /* Starting with the current block, examine each instruction.  If the
   * instruction is a comparison that matches the '±a cmp ±b' pattern, add it
   * to the block_instructions::instructions set.  If the instruction is an
   * add instruction, walk up the block queue looking at the stored
   * instructions.  If a matching comparison is found, move the addition and
   * replace the comparison with a different comparison based on the result
   * of the addition.  All of the blocks in the queue are guaranteed to be
   * dominators of the current block.
   *
   * After processing the current block, recurse into the blocks dominated by
   * the current block.
   */
   nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_alu)
                     if (alu->def.num_components != 1)
                     switch (alu->op) {
   case nir_op_fadd: {
      /* If the instruction is fadd, check it against comparison
   * instructions that dominate it.
   */
                  while (b->node.next != NULL) {
                     u_vector_foreach(a, &b->instructions)
                                    /* The operands of both instructions are, with some liberty,
   * commutative.  Check all four permutations.  The third and
   * fourth permutations are negations of the first two.
   */
   if ((nir_alu_srcs_equal(cmp, alu, 0, 0) &&
      nir_alu_srcs_negative_equal(cmp, alu, 1, 1)) ||
   (nir_alu_srcs_equal(cmp, alu, 0, 1) &&
   nir_alu_srcs_negative_equal(cmp, alu, 1, 0))) {
   /* These are the cases where (A cmp B) matches either (A +
   * -B) or (-B + A)
   *
                        *a = NULL;
   rewrote_compare = true;
      } else if ((nir_alu_srcs_equal(cmp, alu, 1, 0) &&
                  (nir_alu_srcs_equal(cmp, alu, 1, 1) &&
   /* This is the case where (A cmp B) matches (B + -A) or (-A
   * + B).
   *
                        *a = NULL;
   rewrote_compare = true;
                  /* Bail after a compare in the most dominating block is found.
   * This is necessary because 'alu' has been removed from the
   * instruction stream.  Should there be a matching compare in
   * another block, calling rewrite_compare_instruction again will
   * try to operate on a node that is not in the list as if it were
   * in the list.
   *
   * FINISHME: There may be opportunity for additional optimization
   * here.  I discovered this problem due to a shader in Guacamelee.
   * It may be possible to rewrite the matching compares that are
   * encountered later to reuse the result from the compare that was
   * first rewritten.  It's also possible that this is just taken
   * care of by calling the optimization pass repeatedly.
   */
   if (rewrote_compare) {
                                                case nir_op_flt:
   case nir_op_fge:
   case nir_op_fneu:
   case nir_op_feq:
      /* If the instruction is a comparison that is used by an if-statement
   * and neither operand is immediate value 0, add it to the set.
   */
   if (is_compatible_condition(alu) &&
      is_not_const_zero(NULL, alu, 0, 1, swizzle) &&
                     default:
                     for (unsigned i = 0; i < block->num_dom_children; i++) {
               if (comparison_pre_block(child, bq, bld))
                           }
      bool
   nir_opt_comparison_pre_impl(nir_function_impl *impl)
   {
      struct block_queue bq;
            block_queue_init(&bq);
                     const bool progress =
                     if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      bool
   nir_opt_comparison_pre(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
                     }
