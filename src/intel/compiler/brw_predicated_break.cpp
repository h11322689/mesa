   /*
   * Copyright © 2013 Intel Corporation
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
      #include "brw_shader.h"
      using namespace brw;
      /** @file brw_predicated_break.cpp
   *
   * Loops are often structured as
   *
   * loop:
   *    CMP.f0
   *    (+f0) IF
   *    BREAK
   *    ENDIF
   *    ...
   *    WHILE loop
   *
   * This peephole pass removes the IF and ENDIF instructions and predicates the
   * BREAK, dropping two instructions from the loop body.
   *
   * If the loop was a DO { ... } WHILE loop, it looks like
   *
   * loop:
   *    ...
   *    CMP.f0
   *    (+f0) IF
   *    BREAK
   *    ENDIF
   *    WHILE loop
   *
   * and we can remove the BREAK instruction and predicate the WHILE.
   */
      #define MAX_NESTING 128
      struct loop_continue_tracking {
      BITSET_WORD has_continue[BITSET_WORDS(MAX_NESTING)];
      };
      static void
   enter_loop(struct loop_continue_tracking *s)
   {
               /* Any loops deeper than that maximum nesting will just re-use the last
   * flag.  This simplifies most of the code.  MAX_NESTING is chosen to be
   * large enough that it is unlikely to occur.  Even if it does, the
   * optimization that uses this tracking is unlikely to make much
   * difference.
   */
   if (s->depth < MAX_NESTING)
      }
      static void
   exit_loop(struct loop_continue_tracking *s)
   {
      assert(s->depth > 0);
      }
      static void
   set_continue(struct loop_continue_tracking *s)
   {
                  }
      static bool
   has_continue(const struct loop_continue_tracking *s)
   {
                  }
      bool
   opt_predicated_break(backend_shader *s)
   {
      bool progress = false;
            foreach_block (block, s->cfg) {
      /* DO instructions, by definition, can only be found at the beginning of
   * basic blocks.
   */
            /* BREAK, CONTINUE, and WHILE instructions, by definition, can only be
   * found at the ends of basic blocks.
   */
            if (do_inst->opcode == BRW_OPCODE_DO)
            if (jump_inst->opcode == BRW_OPCODE_CONTINUE)
         else if (jump_inst->opcode == BRW_OPCODE_WHILE)
            if (block->start_ip != block->end_ip)
            if (jump_inst->opcode != BRW_OPCODE_BREAK &&
                  backend_instruction *if_inst = block->prev()->end();
   if (if_inst->opcode != BRW_OPCODE_IF)
            backend_instruction *endif_inst = block->next()->start();
   if (endif_inst->opcode != BRW_OPCODE_ENDIF)
            bblock_t *jump_block = block;
   bblock_t *if_block = jump_block->prev();
            jump_inst->predicate = if_inst->predicate;
            bblock_t *earlier_block = if_block;
   if (if_block->start_ip == if_block->end_ip) {
                           bblock_t *later_block = endif_block;
   if (endif_block->start_ip == endif_block->end_ip) {
         }
            if (!earlier_block->ends_with_control_flow()) {
      earlier_block->children.make_empty();
   earlier_block->add_successor(s->cfg->mem_ctx, jump_block,
               if (!later_block->starts_with_control_flow()) {
         }
   jump_block->add_successor(s->cfg->mem_ctx, later_block,
            if (earlier_block->can_combine_with(jump_block)) {
                           /* Now look at the first instruction of the block following the BREAK. If
   * it's a WHILE, we can delete the break, predicate the WHILE, and join
   * the two basic blocks.
   *
   * This optimization can only be applied if the only instruction that
   * can transfer control to the WHILE is the BREAK.  If other paths can
   * lead to the while, the flags may be in an unknown state, and the loop
   * could terminate prematurely.  This can occur if the loop contains a
   * CONT instruction.
   */
   bblock_t *while_block = earlier_block->next();
            if (jump_inst->opcode == BRW_OPCODE_BREAK &&
      while_inst->opcode == BRW_OPCODE_WHILE &&
   while_inst->predicate == BRW_PREDICATE_NONE &&
   !has_continue(&state)) {
   jump_inst->remove(earlier_block);
                  assert(earlier_block->can_combine_with(while_block));
                           if (progress)
               }
