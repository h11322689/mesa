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
      /** @file brw_dead_control_flow.cpp
   *
   * This file implements the dead control flow elimination optimization pass.
   */
      #include "brw_shader.h"
   #include "brw_cfg.h"
      using namespace brw;
      /* Look for and eliminate dead control flow:
   *
   *   - if/endif
   *   - else in else/endif
   *   - then in if/else/endif
   */
   bool
   dead_control_flow_eliminate(backend_shader *s)
   {
               foreach_block_safe (block, s->cfg) {
               if (!prev_block)
            backend_instruction *const inst = block->start();
            /* ENDIF instructions, by definition, can only be found at the start of
   * basic blocks.
   */
   if (inst->opcode == BRW_OPCODE_ENDIF &&
      prev_inst->opcode == BRW_OPCODE_ELSE) {
                  else_inst->remove(else_block);
      } else if (inst->opcode == BRW_OPCODE_ENDIF &&
            bblock_t *const endif_block = block;
   bblock_t *const if_block = prev_block;
                           if (if_block->start_ip == if_block->end_ip) {
         } else {
                        if (endif_block->start_ip == endif_block->end_ip) {
         } else {
                        assert((earlier_block == NULL) == (later_block == NULL));
                     /* If ENDIF was in its own block, then we've now deleted it and
   * merged the two surrounding blocks, the latter of which the
   * __next block pointer was pointing to.
   */
   if (endif_block != later_block) {
                        } else if (inst->opcode == BRW_OPCODE_ELSE &&
            bblock_t *const else_block = block;
                  /* Since the else-branch is becoming the new then-branch, the
   * condition has to be inverted.
   */
                                 if (progress)
               }
