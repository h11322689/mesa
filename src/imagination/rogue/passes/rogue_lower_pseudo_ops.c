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
   #include "rogue_builder.h"
   #include "util/macros.h"
      #include <stdbool.h>
      /**
   * \file rogue_lower_pseudo_ops.c
   *
   * \brief Contains the rogue_lower_pseudo_ops pass.
   */
      static inline bool rogue_lower_FABS(rogue_builder *b, rogue_alu_instr *fabs)
   {
      rogue_alu_instr *mbyp = rogue_MBYP(b, fabs->dst[0].ref, fabs->src[0].ref);
   rogue_merge_instr_comment(&mbyp->instr, &fabs->instr, "fabs");
   rogue_set_alu_src_mod(mbyp, 0, ROGUE_ALU_SRC_MOD_ABS);
               }
      static inline bool rogue_lower_FNEG(rogue_builder *b, rogue_alu_instr *fneg)
   {
      rogue_alu_instr *mbyp = rogue_MBYP(b, fneg->dst[0].ref, fneg->src[0].ref);
   rogue_merge_instr_comment(&mbyp->instr, &fneg->instr, "fneg");
   rogue_set_alu_src_mod(mbyp, 0, ROGUE_ALU_SRC_MOD_NEG);
               }
      static inline bool rogue_lower_FNABS(rogue_builder *b, rogue_alu_instr *fnabs)
   {
      rogue_alu_instr *mbyp = rogue_MBYP(b, fnabs->dst[0].ref, fnabs->src[0].ref);
   rogue_merge_instr_comment(&mbyp->instr, &fnabs->instr, "fnabs");
   rogue_set_alu_src_mod(mbyp, 0, ROGUE_ALU_SRC_MOD_ABS);
   rogue_set_alu_src_mod(mbyp, 0, ROGUE_ALU_SRC_MOD_NEG);
               }
      static inline bool rogue_lower_MOV(rogue_builder *b, rogue_alu_instr *mov)
   {
               /* If we're writing to a vertex output register, we need to use uvsw.write.
   */
   if (rogue_ref_is_reg(&mov->dst[0].ref) &&
      mov->dst[0].ref.reg->class == ROGUE_REG_CLASS_VTXOUT) {
      } else if (rogue_ref_is_special_reg(&mov->src[0].ref)) {
      /* If we're loading a special register, use a movc. */
   rogue_alu_instr *alu = rogue_MOVC(b,
                                 rogue_set_alu_dst_mod(alu, 0, ROGUE_ALU_DST_MOD_E0);
   rogue_set_alu_dst_mod(alu, 0, ROGUE_ALU_DST_MOD_E1);
   rogue_set_alu_dst_mod(alu, 0, ROGUE_ALU_DST_MOD_E2);
               } else {
      /* If we're moving an immediate value not in special constants,
   * we need to do a bitwise bypass.
   */
   if (rogue_ref_is_imm(&mov->src[0].ref)) {
      instr = &rogue_BYP0(b,
                     rogue_ref_io(ROGUE_IO_FT0),
   mov->dst[0].ref,
      } else {
                     rogue_merge_instr_comment(instr, &mov->instr, "mov");
               }
      static inline bool rogue_lower_alu_instr(rogue_builder *b, rogue_alu_instr *alu)
   {
      switch (alu->op) {
   case ROGUE_ALU_OP_MOV:
            case ROGUE_ALU_OP_FABS:
            case ROGUE_ALU_OP_FNEG:
            case ROGUE_ALU_OP_FNABS:
            default:
                     }
      static inline bool rogue_lower_END(rogue_builder *b, rogue_ctrl_instr *end)
   {
      rogue_ctrl_instr *nop = rogue_NOP(b);
   rogue_merge_instr_comment(&nop->instr, &end->instr, "end");
   rogue_set_ctrl_op_mod(nop, ROGUE_CTRL_OP_MOD_END);
               }
      static inline bool rogue_lower_ctrl_instr(rogue_builder *b,
         {
      switch (ctrl->op) {
   case ROGUE_CTRL_OP_END:
            default:
                     }
      /* TODO: This should only really be called after a distribute_src_mods pass (to
   * come later). */
   PUBLIC
   bool rogue_lower_pseudo_ops(rogue_shader *shader)
   {
      if (shader->is_grouped)
                     rogue_builder b;
            rogue_foreach_instr_in_shader_safe (instr, shader) {
      /* Skip real ops. */
   if (rogue_instr_supported_phases(instr))
            b.cursor = rogue_cursor_before_instr(instr);
   switch (instr->type) {
   case ROGUE_INSTR_TYPE_ALU:
                  case ROGUE_INSTR_TYPE_CTRL:
                  default:
                        }
