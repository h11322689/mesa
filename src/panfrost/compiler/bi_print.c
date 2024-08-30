   /*
   * Copyright (C) 2019 Connor Abbott <cwabbott0@gmail.com>
   * Copyright (C) 2019 Lyude Paul <thatslyude@gmail.com>
   * Copyright (C) 2019 Ryan Houdek <Sonicadvance1@gmail.com>
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
      #include "bi_print_common.h"
   #include "compiler.h"
      static const char *
   bi_reg_op_name(enum bifrost_reg_op op)
   {
      switch (op) {
   case BIFROST_OP_IDLE:
         case BIFROST_OP_READ:
         case BIFROST_OP_WRITE:
         case BIFROST_OP_WRITE_LO:
         case BIFROST_OP_WRITE_HI:
         default:
            }
      void
   bi_print_slots(bi_registers *regs, FILE *fp)
   {
      for (unsigned i = 0; i < 2; ++i) {
      if (regs->enabled[i])
               if (regs->slot23.slot2) {
      fprintf(fp, "slot 2 (%s%s): %u\n", bi_reg_op_name(regs->slot23.slot2),
                     if (regs->slot23.slot3) {
      fprintf(fp, "slot 3 (%s %s): %u\n", bi_reg_op_name(regs->slot23.slot3),
         }
      void
   bi_print_tuple(bi_tuple *tuple, FILE *fp)
   {
               for (unsigned i = 0; i < 2; ++i) {
               if (ins[i])
         else
         }
      void
   bi_print_clause(bi_clause *clause, FILE *fp)
   {
               if (clause->dependencies) {
               for (unsigned i = 0; i < 8; ++i) {
      if (clause->dependencies & (1 << i))
                                    if (!clause->next_clause_prefetch)
            if (clause->staging_barrier)
            if (clause->td)
            if (clause->pcrel_idx != ~0)
                     for (unsigned i = 0; i < clause->tuple_count; ++i)
            if (clause->constant_count) {
      for (unsigned i = 0; i < clause->constant_count; ++i)
            if (clause->branch_constant)
                           }
      static void
   bi_print_scoreboard_line(unsigned slot, const char *name, uint64_t mask,
         {
      if (!mask)
                     u_foreach_bit64(reg, mask)
               }
      static void
   bi_print_scoreboard(struct bi_scoreboard_state *state, FILE *fp)
   {
      for (unsigned i = 0; i < BI_NUM_SLOTS; ++i) {
      bi_print_scoreboard_line(i, "reads", state->read[i], fp);
         }
      void
   bi_print_block(bi_block *block, FILE *fp)
   {
      if (block->scheduled) {
      bi_print_scoreboard(&block->scoreboard_in, fp);
                        if (block->scheduled) {
      bi_foreach_clause_in_block(block, clause)
      } else {
      bi_foreach_instr_in_block(block, ins)
                        if (block->successors[0]) {
               bi_foreach_successor((block), succ)
               if (bi_num_predecessors(block)) {
               bi_foreach_predecessor(block, pred)
               if (block->scheduled) {
      fprintf(fp, "\n");
                  }
      void
   bi_print_shader(bi_context *ctx, FILE *fp)
   {
      bi_foreach_block(ctx, block)
      }
