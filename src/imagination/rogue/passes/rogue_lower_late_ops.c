   /*
   * Copyright © 2023 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "rogue.h"
   #include "rogue_builder.h"
   #include "util/macros.h"
      #include <stdbool.h>
      /**
   * \file rogue_lower_late_ops.c
   *
   * \brief Contains the rogue_lower_late_ops pass.
   */
      /* TODO NEXT!: Check if registers are being written to that require special
   * behaviour, like vertex out.
   */
   /* TODO NEXT!: Make sure that SSA regs aren't being used, late passes must
   * happen after SSA.
   */
   static inline bool rogue_lower_CMOV(rogue_builder *b, rogue_alu_instr *cmov)
   {
      rogue_instr *instr_true =
         rogue_instr *instr_false =
            rogue_set_instr_exec_cond(instr_true, ROGUE_EXEC_COND_P0_TRUE);
            rogue_merge_instr_comment(instr_true, &cmov->instr, "cmov (true)");
                        }
      static inline bool rogue_lower_alu_instr(rogue_builder *b, rogue_alu_instr *alu)
   {
      switch (alu->op) {
   case ROGUE_ALU_OP_CMOV:
            default:
                     }
      PUBLIC
   bool rogue_lower_late_ops(rogue_shader *shader)
   {
      if (shader->is_grouped)
                     rogue_builder b;
            rogue_foreach_instr_in_shader_safe (instr, shader) {
      /* Skip real ops. */
   if (rogue_instr_supported_phases(instr))
            b.cursor = rogue_cursor_before_instr(instr);
   switch (instr->type) {
   case ROGUE_INSTR_TYPE_ALU:
                  default:
                        }
