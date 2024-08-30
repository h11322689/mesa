   /*
   * Copyright © 2019 Valve Corporation
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
      #include "aco_ir.h"
      #include <algorithm>
   #include <vector>
      /*
   * Implements an analysis pass to determine the number of uses
   * for each SSA-definition.
   */
      namespace aco {
   namespace {
      struct dce_ctx {
      int current_block;
   std::vector<uint16_t> uses;
            dce_ctx(Program* program)
         {
      live.reserve(program->blocks.size());
   for (Block& block : program->blocks)
         };
      void
   process_block(dce_ctx& ctx, Block& block)
   {
      std::vector<bool>& live = ctx.live[block.index];
   assert(live.size() == block.instructions.size());
   bool process_predecessors = false;
   for (int idx = block.instructions.size() - 1; idx >= 0; idx--) {
      if (live[idx])
            aco_ptr<Instruction>& instr = block.instructions[idx];
   if (!is_dead(ctx.uses, instr.get())) {
      for (const Operand& op : instr->operands) {
      if (op.isTemp()) {
      if (ctx.uses[op.tempId()] == 0)
               }
                  if (process_predecessors) {
      for (unsigned pred_idx : block.linear_preds)
         }
      } /* end namespace */
      std::vector<uint16_t>
   dead_code_analysis(Program* program)
   {
                  while (ctx.current_block >= 0) {
      unsigned next_block = ctx.current_block--;
                  }
      } // namespace aco
