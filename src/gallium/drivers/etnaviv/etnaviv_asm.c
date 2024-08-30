   /*
   * Copyright (c) 2012-2015 Etnaviv Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Wladimir J. van der Laan <laanwj@gmail.com>
   */
      #include "etnaviv_asm.h"
   #include "etnaviv_debug.h"
   #include "etnaviv_util.h"
      /* An instruction can only read from one distinct uniform.
   * This function verifies this property and returns true if the instruction
   * is deemed correct and false otherwise.
   */
   static bool
   check_uniforms(const struct etna_inst *inst)
   {
      unsigned uni_rgroup = -1;
   unsigned uni_reg = -1;
            for (unsigned i = 0; i < ETNA_NUM_SRC; i++) {
               if (!etna_rgroup_is_uniform(src->rgroup))
            if (uni_reg == -1) { /* first uniform used */
      uni_rgroup = src->rgroup;
      } else { /* second or later; check that it is a re-use */
      if (uni_rgroup != src->rgroup || uni_reg != src->reg) {
                           }
      int
   etna_assemble(uint32_t *out, const struct etna_inst *inst)
   {
      /* cannot have both src2 and imm */
   if (inst->imm && inst->src[2].use)
            if (!inst->no_oneconst_limit && !check_uniforms(inst))
                     out[0] = VIV_ISA_WORD_0_OPCODE(inst->opcode & 0x3f) |
            VIV_ISA_WORD_0_COND(inst->cond) |
   COND(inst->sat, VIV_ISA_WORD_0_SAT) |
   COND(inst->dst.use, VIV_ISA_WORD_0_DST_USE) |
   VIV_ISA_WORD_0_DST_AMODE(inst->dst.amode) |
   VIV_ISA_WORD_0_DST_REG(inst->dst.reg) |
      out[1] = VIV_ISA_WORD_1_TEX_AMODE(inst->tex.amode) |
            VIV_ISA_WORD_1_TEX_SWIZ(inst->tex.swiz) |
   COND(inst->src[0].use, VIV_ISA_WORD_1_SRC0_USE) |
   VIV_ISA_WORD_1_SRC0_REG(inst->src[0].reg) |
   COND(inst->type & 0x4, VIV_ISA_WORD_1_TYPE_BIT2) |
   VIV_ISA_WORD_1_SRC0_SWIZ(inst->src[0].swiz) |
      out[2] = VIV_ISA_WORD_2_SRC0_AMODE(inst->src[0].amode) |
            VIV_ISA_WORD_2_SRC0_RGROUP(inst->src[0].rgroup) |
   COND(inst->src[1].use, VIV_ISA_WORD_2_SRC1_USE) |
   VIV_ISA_WORD_2_SRC1_REG(inst->src[1].reg) |
   COND(inst->opcode & 0x40, VIV_ISA_WORD_2_OPCODE_BIT6) |
   VIV_ISA_WORD_2_SRC1_SWIZ(inst->src[1].swiz) |
   COND(inst->src[1].neg, VIV_ISA_WORD_2_SRC1_NEG) |
   COND(inst->src[1].abs, VIV_ISA_WORD_2_SRC1_ABS) |
      out[3] = VIV_ISA_WORD_3_SRC1_RGROUP(inst->src[1].rgroup) |
            COND(inst->src[2].use, VIV_ISA_WORD_3_SRC2_USE) |
   VIV_ISA_WORD_3_SRC2_REG(inst->src[2].reg) |
   VIV_ISA_WORD_3_SRC2_SWIZ(inst->src[2].swiz) |
   COND(inst->src[2].neg, VIV_ISA_WORD_3_SRC2_NEG) |
   COND(inst->src[2].abs, VIV_ISA_WORD_3_SRC2_ABS) |
   VIV_ISA_WORD_3_SRC2_AMODE(inst->src[2].amode) |
   VIV_ISA_WORD_3_SRC2_RGROUP(inst->src[2].rgroup) |
   COND(inst->sel_bit0, VIV_ISA_WORD_3_SEL_BIT0) |
                     }
