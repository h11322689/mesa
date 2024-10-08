   /*
   * Copyright 2023 Valve Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "util/list.h"
   #include "agx_builder.h"
   #include "agx_compiler.h"
      /*
   * Replaces conditional breaks:
   *
   *    if_cmp x, y, n=1 {
   *       break #value
   *    }
   *    pop_exec n=1
   *
   * with break_if_*cmp pseudo-instructions for further optimization. This
   * assumes agx_opt_empty_else has already run.
   */
      static void
   match_block(agx_context *ctx, agx_block *block)
   {
      agx_instr *if_ = agx_last_instr(block);
   if (!if_ ||
      (if_->op != AGX_OPCODE_IF_ICMP && if_->op != AGX_OPCODE_IF_FCMP) ||
   if_->nest != 1)
         /* If's fallthrough to the then */
   agx_block *then_block = agx_next_block(block);
            /* We're searching for a single block then, so the next block is else */
   agx_block *else_block = agx_next_block(then_block);
   if (block->successors[1] != else_block)
            if (!list_is_singular(&then_block->instructions) ||
      !list_is_singular(&else_block->instructions))
         agx_instr *break_ = agx_last_instr(then_block);
            if (break_->op != AGX_OPCODE_BREAK || pop->op != AGX_OPCODE_POP_EXEC ||
      pop->nest != 1)
         /* Find the successor of the if */
   agx_block *after = else_block->successors[0];
            /* We are dropping one level of nesting (the if) when rewriting */
   assert(break_->nest >= 1 && "must at least break out of the if");
            /* Rewrite */
            if (if_->op == AGX_OPCODE_IF_FCMP) {
      agx_break_if_fcmp(&b, if_->src[0], if_->src[1], new_nest,
      } else {
      agx_break_if_icmp(&b, if_->src[0], if_->src[1], new_nest,
               agx_remove_instruction(if_);
   agx_remove_instruction(break_);
      }
      void
   agx_opt_break_if(agx_context *ctx)
   {
      agx_foreach_block(ctx, block) {
            }
