   /*
   * Copyright © 2022 Imagination Technologies Ltd.
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
   #include "util/macros.h"
      #include <stdbool.h>
      /**
   * \file rogue_dce.c
   *
   * \brief Contains the rogue_dce pass.
   */
      /* TODO:
   * 0) Add bools/flags to registers in rogue_info that specifies whether they can
   * be I/O registers (i.e. populated by driver/used by driver).
   * 1) Loop through instructions and delete ones that have their destinations
   * unused (with exception of pixel output, shared, etc.).
   * 2) Loop through registers and delete ones that have no uses/writes.
   */
      static bool rogue_dce_alu_instr(rogue_alu_instr *alu)
   {
               switch (alu->op) {
   case ROGUE_ALU_OP_MOV:
   case ROGUE_ALU_OP_MBYP:
      if (!alu->mod && rogue_instr_dst_src_equal(&alu->dst[0], &alu->src[0])) {
      rogue_instr_delete(&alu->instr);
      }
         default:
                     }
      static bool rogue_dce_instrs(rogue_shader *shader)
   {
               rogue_foreach_instr_in_shader_safe (instr, shader) {
      switch (instr->type) {
   case ROGUE_INSTR_TYPE_ALU:
                  case ROGUE_INSTR_TYPE_BACKEND:
            case ROGUE_INSTR_TYPE_CTRL:
            case ROGUE_INSTR_TYPE_BITWISE:
            default:
      unreachable("Unsupported instruction type.");
                     }
      /* TODO: Do this in rogue_trim instead? */
   static bool rogue_try_release_reg(rogue_reg *reg)
   {
      /* Check if the register is used or written to. */
   if (!rogue_reg_is_unused(reg))
            /* Check if the register is part of a regarray. */
   if (reg->regarray)
            /* Register is unused, delete it. */
               }
      static bool rogue_dce_regs(rogue_shader *shader)
   {
               /* Remove unused SSA/temp registers that aren't in any regarrays. */
   rogue_foreach_reg_safe (reg, shader, ROGUE_REG_CLASS_SSA) {
                  rogue_foreach_reg_safe (reg, shader, ROGUE_REG_CLASS_TEMP) {
                     }
      PUBLIC
   bool rogue_dce(rogue_shader *shader)
   {
      if (shader->is_grouped)
                     progress |= rogue_dce_instrs(shader);
               }
