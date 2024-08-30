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
      #include <assert.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <stdbool.h>
      #include "pvr_rogue_pds_defs.h"
   #include "pvr_rogue_pds_encode.h"
   #include "pvr_rogue_pds_disasm.h"
   #include "util/macros.h"
      static void pvr_error_check(PVR_ERR_CALLBACK err_callback,
         {
      if (err_callback)
         else
      }
      #define X(a) #a,
   static const char *const instructions[] = { PVR_INSTRUCTIONS };
   #undef X
      static void error_reg_range(uint32_t raw,
                           {
               error.type = PVR_PDS_ERR_PARAM_RANGE;
   error.parameter = parameter;
            if (parameter == 0)
         else
            error.text = malloc(PVR_PDS_MAX_INST_STR_LEN);
            snprintf(error.text,
            PVR_PDS_MAX_INST_STR_LEN,
   "Register out of range, instruction: %s, operand: %s, value: %u",
   instructions[error.instruction],
         }
      static struct pvr_operand *
   pvr_pds_disassemble_regs32(void *context,
                           {
      struct pvr_operand *op = calloc(1, sizeof(*op));
            op->type = UNRESOLVED;
   instruction &= PVR_ROGUE_PDSINST_REGS32_MASK;
   switch (pvr_pds_inst_decode_field_range_regs32(instruction)) {
   case PVR_ROGUE_PDSINST_REGS32_CONST32:
      op->type = CONST32;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS32_CONST32_LOWER;
   op->absolute_address = op->address;
      case PVR_ROGUE_PDSINST_REGS32_TEMP32:
      op->type = TEMP32;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS32_TEMP32_LOWER;
   op->absolute_address = op->address;
      case PVR_ROGUE_PDSINST_REGS32_PTEMP32:
      op->type = PTEMP32;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS32_PTEMP32_LOWER;
   op->absolute_address = op->address;
      default:
         }
      }
   static struct pvr_operand *
   pvr_pds_disassemble_regs32tp(void *context,
                           {
      struct pvr_operand *op = calloc(1, sizeof(*op));
            op->type = UNRESOLVED;
   instruction &= PVR_ROGUE_PDSINST_REGS32TP_MASK;
   switch (pvr_pds_inst_decode_field_range_regs32tp(instruction)) {
   case PVR_ROGUE_PDSINST_REGS32TP_TEMP32:
      op->type = TEMP32;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS32TP_TEMP32_LOWER;
   op->absolute_address = op->address;
      case PVR_ROGUE_PDSINST_REGS32TP_PTEMP32:
      op->type = PTEMP32;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS32TP_PTEMP32_LOWER;
   op->absolute_address = op->address;
      default:
         }
      }
   static struct pvr_operand *
   pvr_pds_disassemble_regs32t(void *context,
                           {
      struct pvr_operand *op = calloc(1, sizeof(*op));
            op->type = UNRESOLVED;
   instruction &= PVR_ROGUE_PDSINST_REGS32T_MASK;
   switch (pvr_pds_inst_decode_field_range_regs32t(instruction)) {
   case PVR_ROGUE_PDSINST_REGS32T_TEMP32:
      op->type = TEMP32;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS32T_TEMP32_LOWER;
   op->absolute_address = op->address;
      default:
         }
      }
      static struct pvr_operand *
   pvr_pds_disassemble_regs64(void *context,
                           {
      struct pvr_operand *op = calloc(1, sizeof(*op));
            op->type = UNRESOLVED;
   instruction &= PVR_ROGUE_PDSINST_REGS64_MASK;
   switch (pvr_pds_inst_decode_field_range_regs64(instruction)) {
   case PVR_ROGUE_PDSINST_REGS64_CONST64:
      op->type = CONST64;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS64_CONST64_LOWER;
   op->absolute_address = op->address * 2;
      case PVR_ROGUE_PDSINST_REGS64_TEMP64:
      op->type = TEMP64;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS64_TEMP64_LOWER;
   op->absolute_address = op->address * 2;
      case PVR_ROGUE_PDSINST_REGS64_PTEMP64:
      op->type = PTEMP64;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS64_PTEMP64_LOWER;
   op->absolute_address = op->address * 2;
      default:
                     }
   static struct pvr_operand *
   pvr_pds_disassemble_regs64t(void *context,
                           {
      struct pvr_operand *op = calloc(1, sizeof(*op));
            op->type = UNRESOLVED;
   instruction &= PVR_ROGUE_PDSINST_REGS64T_MASK;
   switch (pvr_pds_inst_decode_field_range_regs64tp(instruction)) {
   case PVR_ROGUE_PDSINST_REGS64T_TEMP64:
      op->type = TEMP64;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS64T_TEMP64_LOWER;
   op->absolute_address = op->address * 2;
      default:
         }
      }
      static struct pvr_operand *
   pvr_pds_disassemble_regs64C(void *context,
                           {
      struct pvr_operand *op = calloc(1, sizeof(*op));
            op->type = UNRESOLVED;
   instruction &= PVR_ROGUE_PDSINST_REGS64C_MASK;
   switch (pvr_rogue_pds_inst_decode_field_range_regs64c(instruction)) {
   case PVR_ROGUE_PDSINST_REGS64C_CONST64:
      op->type = CONST64;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS64C_CONST64_LOWER;
   op->absolute_address = op->address * 2;
      default:
         }
      }
      static struct pvr_operand *
   pvr_pds_disassemble_regs64tp(void *context,
                           {
      struct pvr_operand *op = calloc(1, sizeof(*op));
            op->type = UNRESOLVED;
   instruction &= PVR_ROGUE_PDSINST_REGS64TP_MASK;
   switch (pvr_pds_inst_decode_field_range_regs64tp(instruction)) {
   case PVR_ROGUE_PDSINST_REGS64TP_TEMP64:
      op->type = TEMP64;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS64TP_TEMP64_LOWER;
   op->absolute_address = op->address * 2;
      case PVR_ROGUE_PDSINST_REGS64TP_PTEMP64:
      op->type = PTEMP64;
   op->address = instruction - PVR_ROGUE_PDSINST_REGS64TP_PTEMP64_LOWER;
   op->absolute_address = op->address * 2;
      default:
         }
      }
      #define PVR_TYPE_OPCODE BITFIELD_BIT(31U)
   #define PVR_TYPE_OPCODE_SP BITFIELD_BIT(27U)
   #define PVR_TYPE_OPCODEB BITFIELD_BIT(30U)
      #define PVR_TYPE_OPCODE_SHIFT 28U
   #define PVR_TYPE_OPCODE_SP_SHIFT 23U
   #define PVR_TYPE_OPCODEB_SHIFT 29U
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_add64(void *context,
                     {
      struct pvr_add *add = malloc(sizeof(*add));
            add->instruction.type = INS_ADD64;
            add->cc = instruction & PVR_ROGUE_PDSINST_ADD64_CC_ENABLE;
   add->alum = instruction & PVR_ROGUE_PDSINST_ADD64_ALUM_SIGNED;
            add->src0 = pvr_pds_disassemble_regs64(context,
                                 add->src0->instruction = &add->instruction;
   add->src1 = pvr_pds_disassemble_regs64(context,
                                 add->src1->instruction = &add->instruction;
   add->dst = pvr_pds_disassemble_regs64tp(context,
                                             }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_add32(void *context,
                     {
      struct pvr_add *add = malloc(sizeof(*add));
            add->instruction.type = INS_ADD32;
            add->cc = instruction & PVR_ROGUE_PDSINST_ADD32_CC_ENABLE;
   add->alum = instruction & PVR_ROGUE_PDSINST_ADD32_ALUM_SIGNED;
            add->src0 = pvr_pds_disassemble_regs32(context,
                                 add->src0->instruction = &add->instruction;
   add->src1 = pvr_pds_disassemble_regs32(context,
                                 add->src1->instruction = &add->instruction;
   add->dst = pvr_pds_disassemble_regs32tp(context,
                                             }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_stm(void *context,
                     {
      struct pvr_stm *stm = malloc(sizeof(*stm));
            stm->instruction.next = NULL;
            stm->cc = instruction & (1 << PVR_ROGUE_PDSINST_STM_CCS_CCS_CC_SHIFT);
   stm->ccs_global = instruction &
         stm->ccs_so = instruction & (1 << PVR_ROGUE_PDSINST_STM_CCS_CCS_SO_SHIFT);
            stm->stream_out = (instruction >> PVR_ROGUE_PDSINST_STM_SO_SHIFT) &
            stm->src0 = pvr_pds_disassemble_regs64tp(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_STM_SO_SRC0_SHIFT,
               stm->src1 = pvr_pds_disassemble_regs64tp(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_STM_SO_SRC1_SHIFT,
               stm->src2 = pvr_pds_disassemble_regs32(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_STM_SO_SRC2_SHIFT,
               stm->src3 = pvr_pds_disassemble_regs64tp(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_STM_SO_SRC3_SHIFT,
                  }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_sftlp32(void *context,
                     {
      struct pvr_sftlp *ins = malloc(sizeof(*ins));
            ins->instruction.next = NULL;
            ins->cc = instruction & PVR_ROGUE_PDSINST_SFTLP32_CC_ENABLE;
   ins->IM = instruction & PVR_ROGUE_PDSINST_SFTLP32_IM_ENABLE;
   ins->lop = (instruction >> PVR_ROGUE_PDSINST_SFTLP32_LOP_SHIFT) &
         ins->src0 = pvr_pds_disassemble_regs32t(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_SFTLP32_SRC0_SHIFT,
      ins->src0->instruction = &ins->instruction;
   ins->src1 = pvr_pds_disassemble_regs32(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_SFTLP32_SRC1_SHIFT,
      ins->src1->instruction = &ins->instruction;
   ins->dst = pvr_pds_disassemble_regs32t(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_SFTLP32_DST_SHIFT,
               if (ins->IM) {
      signed char cImmediate =
      ((instruction >> PVR_ROGUE_PDSINST_SFTLP32_SRC2_SHIFT) &
   PVR_ROGUE_PDSINST_REGS32_MASK)
      ins->src2 = calloc(1, sizeof(*ins->src2));
            ins->src2->literal = abs((cImmediate / 4));
   ins->src2->negate = cImmediate < 0;
      } else {
      ins->src2 = pvr_pds_disassemble_regs32tp(
      context,
   err_callback,
   error,
   (instruction >> PVR_ROGUE_PDSINST_SFTLP32_SRC2_SHIFT),
                     }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_sftlp64(void *context,
                     {
      struct pvr_sftlp *ins = malloc(sizeof(*ins));
            ins->instruction.next = NULL;
            ins->cc = instruction & PVR_ROGUE_PDSINST_SFTLP64_CC_ENABLE;
   ins->IM = instruction & PVR_ROGUE_PDSINST_SFTLP64_IM_ENABLE;
   ins->lop = (instruction >> PVR_ROGUE_PDSINST_SFTLP64_LOP_SHIFT) &
         ins->src0 = pvr_pds_disassemble_regs64tp(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_SFTLP64_SRC0_SHIFT,
      ins->src0->instruction = &ins->instruction;
   ins->src1 = pvr_pds_disassemble_regs64tp(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_SFTLP64_SRC1_SHIFT,
      ins->src1->instruction = &ins->instruction;
   ins->dst = pvr_pds_disassemble_regs64tp(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_SFTLP64_DST_SHIFT,
               if (ins->IM) {
      signed char cImmediate =
      (instruction >> PVR_ROGUE_PDSINST_SFTLP64_SRC2_SHIFT) &
      ins->src2 = calloc(1, sizeof(*ins->src2));
            ins->src2->literal = (abs(cImmediate) > 63) ? 63 : abs(cImmediate);
   ins->src2->negate = (cImmediate < 0);
      } else {
      ins->src2 = pvr_pds_disassemble_regs32(
      context,
   err_callback,
   error,
   (instruction >> PVR_ROGUE_PDSINST_SFTLP64_SRC2_SHIFT),
                     }
   static struct pvr_instruction *
   pvr_pds_disassemble_instruction_cmp(void *context,
                     {
      struct pvr_cmp *cmp = malloc(sizeof(*cmp));
            cmp->instruction.next = NULL;
   cmp->instruction.type = INS_CMP;
   cmp->cc = instruction & PVR_ROGUE_PDSINST_CMP_CC_ENABLE;
   cmp->IM = instruction & PVR_ROGUE_PDSINST_CMP_IM_ENABLE;
   cmp->cop = instruction >> PVR_ROGUE_PDSINST_CMP_COP_SHIFT &
         cmp->src0 = pvr_pds_disassemble_regs64tp(context,
                                          if (cmp->IM) {
      uint32_t immediate = (instruction >> PVR_ROGUE_PDSINST_CMP_SRC1_SHIFT) &
         cmp->src1 = calloc(1, sizeof(*cmp->src1));
            cmp->src1->type = LITERAL_NUM;
      } else {
      cmp->src1 = pvr_pds_disassemble_regs64(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_CMP_SRC1_SHIFT,
   }
               }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_sp_ld_st(void *context,
                                 {
      struct pvr_ldst *ins = malloc(sizeof(*ins));
            ins->instruction.next = NULL;
            ins->cc = cc;
   ins->src0 =
      pvr_pds_disassemble_regs64(context,
                        ins->src0->instruction = &ins->instruction;
               }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_sp_stmc(uint32_t instruction, bool cc)
   {
      struct pvr_stmc *stmc = malloc(sizeof(*stmc));
            stmc->instruction.next = NULL;
            stmc->cc = cc;
   stmc->src0 = calloc(1, sizeof(*stmc->src0));
            stmc->src0->type = LITERAL_NUM;
   stmc->src0->literal = (instruction >> PVR_ROGUE_PDSINST_STMC_SOMASK_SHIFT) &
                     }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_sp_limm(void *context,
                           {
      struct pvr_limm *limm = malloc(sizeof(*limm));
   assert(limm);
   limm->instruction.next = NULL;
            limm->cc = cc;
   limm->GR = (instruction & PVR_ROGUE_PDSINST_LIMM_GR_ENABLE) != 0;
   limm->src0 = calloc(1, sizeof(*limm->src0));
            limm->src0->type = LITERAL_NUM;
   limm->src0->literal = (instruction >> PVR_ROGUE_PDSINST_LIMM_SRC0_SHIFT) &
         limm->src0->instruction = &limm->instruction;
   limm->dst = pvr_pds_disassemble_regs32t(context,
                                             }
      static struct pvr_instruction *
   pvr_pds_disassemble_simple(enum pvr_instruction_type type, bool cc)
   {
      struct pvr_simple *ins = malloc(sizeof(*ins));
            ins->instruction.next = NULL;
   ins->instruction.type = type;
               }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_bra(uint32_t instruction)
   {
      uint32_t branch_addr;
   struct pvr_bra *bra = (struct pvr_bra *)malloc(sizeof(*bra));
            bra->instruction.type = INS_BRA;
            branch_addr = (instruction >> PVR_ROGUE_PDSINST_BRA_ADDR_SHIFT) &
         bra->address = (branch_addr & 0x40000U) ? ((int)branch_addr) - 0x80000
            bra->srcc = malloc(sizeof(*bra->srcc));
            bra->srcc->predicate = (instruction >> PVR_ROGUE_PDSINST_BRA_SRCC_SHIFT) &
                  bra->setc = malloc(sizeof(*bra->setc));
            bra->setc->predicate = (instruction >> PVR_ROGUE_PDSINST_BRA_SETC_SHIFT) &
                        }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_sp(void *context,
                     {
      uint32_t op = (instruction >> PVR_TYPE_OPCODE_SP_SHIFT) &
                  switch (op) {
   case PVR_ROGUE_PDSINST_OPCODESP_LD:
      error.instruction = INS_LD;
   return pvr_pds_disassemble_instruction_sp_ld_st(
      context,
   err_callback,
   error,
   true,
   instruction,
   case PVR_ROGUE_PDSINST_OPCODESP_ST:
      error.instruction = INS_ST;
   return pvr_pds_disassemble_instruction_sp_ld_st(
      context,
   err_callback,
   error,
   false,
   instruction,
   case PVR_ROGUE_PDSINST_OPCODESP_STMC:
      error.instruction = INS_STMC;
      case PVR_ROGUE_PDSINST_OPCODESP_LIMM:
      error.instruction = INS_LIMM;
   return pvr_pds_disassemble_instruction_sp_limm(context,
                        case PVR_ROGUE_PDSINST_OPCODESP_WDF:
      error.instruction = INS_WDF;
      case PVR_ROGUE_PDSINST_OPCODESP_LOCK:
      error.instruction = INS_LOCK;
      case PVR_ROGUE_PDSINST_OPCODESP_RELEASE:
      error.instruction = INS_RELEASE;
      case PVR_ROGUE_PDSINST_OPCODESP_HALT:
      error.instruction = INS_HALT;
      case PVR_ROGUE_PDSINST_OPCODESP_NOP:
      error.instruction = INS_NOP;
      default:
      error.type = PVR_PDS_ERR_SP_UNKNOWN;
   error.text = "opcode unknown for special instruction";
   pvr_error_check(err_callback, error);
         }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_ddmad(void *context,
                     {
      struct pvr_ddmad *ddmad = malloc(sizeof(*ddmad));
            ddmad->instruction.next = NULL;
            ddmad->cc = instruction & PVR_ROGUE_PDSINST_DDMAD_CC_ENABLE;
            ddmad->src0 = pvr_pds_disassemble_regs32(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_DDMAD_SRC0_SHIFT,
               ddmad->src1 = pvr_pds_disassemble_regs32t(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_DDMAD_SRC1_SHIFT,
               ddmad->src2 = pvr_pds_disassemble_regs64(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_DDMAD_SRC2_SHIFT,
               ddmad->src3 = pvr_pds_disassemble_regs64C(
      context,
   err_callback,
   error,
   instruction >> PVR_ROGUE_PDSINST_DDMAD_SRC3_SHIFT,
                  }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_mad(void *context,
                     {
      struct pvr_mad *mad = malloc(sizeof(*mad));
            mad->instruction.next = NULL;
            mad->cc = instruction & PVR_ROGUE_PDSINST_MAD_CC_ENABLE;
   mad->sna = instruction & PVR_ROGUE_PDSINST_MAD_SNA_SUB;
            mad->src0 = pvr_pds_disassemble_regs32(context,
                                          mad->src1 = pvr_pds_disassemble_regs32(context,
                                          mad->src2 = pvr_pds_disassemble_regs64(context,
                                          mad->dst = pvr_pds_disassemble_regs64t(context,
                                             }
      static struct pvr_instruction *
   pvr_pds_disassemble_instruction_dout(void *context,
                     {
      struct pvr_dout *dout = malloc(sizeof(*dout));
            dout->instruction.next = NULL;
            dout->END = instruction & PVR_ROGUE_PDSINST_DOUT_END_ENABLE;
   dout->cc = instruction & PVR_ROGUE_PDSINST_DOUT_CC_ENABLE;
   dout->dst = (instruction >> PVR_ROGUE_PDSINST_DOUT_DST_SHIFT) &
            dout->src0 = pvr_pds_disassemble_regs64(context,
                                          dout->src1 = pvr_pds_disassemble_regs32(context,
                                             }
      static void pvr_pds_free_instruction_limm(struct pvr_limm *inst)
   {
      free(inst->dst);
   free(inst->src0);
      }
      static void pvr_pds_free_instruction_add(struct pvr_add *inst)
   {
      free(inst->dst);
   free(inst->src0);
   free(inst->src1);
      }
      static void pvr_pds_free_instruction_cmp(struct pvr_cmp *inst)
   {
      free(inst->src0);
   free(inst->src1);
      }
      static void pvr_pds_free_instruction_mad(struct pvr_mad *inst)
   {
      free(inst->dst);
   free(inst->src0);
   free(inst->src1);
   free(inst->src2);
      }
      static void pvr_pds_free_instruction_bra(struct pvr_bra *inst)
   {
      free(inst->setc);
   free(inst->srcc);
      }
      static void pvr_pds_free_instruction_ddmad(struct pvr_ddmad *inst)
   {
      free(inst->src0);
   free(inst->src1);
   free(inst->src2);
   free(inst->src3);
      }
      static void pvr_pds_free_instruction_dout(struct pvr_dout *inst)
   {
      free(inst->src0);
   free(inst->src1);
      }
      static void pvr_pds_free_instruction_ldst(struct pvr_ldst *inst)
   {
      free(inst->src0);
      }
      static void pvr_pds_free_instruction_simple(struct pvr_simple *inst)
   {
         }
      static void pvr_pds_free_instruction_sfltp(struct pvr_sftlp *inst)
   {
      free(inst->dst);
   free(inst->src0);
   free(inst->src1);
   free(inst->src2);
      }
      static void pvr_pds_free_instruction_stm(struct pvr_stm *inst)
   {
      free(inst->src0);
   free(inst->src1);
   free(inst->src2);
   free(inst->src3);
      }
      static void pvr_pds_free_instruction_stmc(struct pvr_stmc *inst)
   {
      free(inst->src0);
      }
      void pvr_pds_free_instruction(struct pvr_instruction *instruction)
   {
      if (!instruction)
            switch (instruction->type) {
   case INS_LIMM:
      pvr_pds_free_instruction_limm((struct pvr_limm *)instruction);
      case INS_ADD64:
   case INS_ADD32:
      pvr_pds_free_instruction_add((struct pvr_add *)instruction);
      case INS_CMP:
      pvr_pds_free_instruction_cmp((struct pvr_cmp *)instruction);
      case INS_MAD:
      pvr_pds_free_instruction_mad((struct pvr_mad *)instruction);
      case INS_BRA:
      pvr_pds_free_instruction_bra((struct pvr_bra *)instruction);
      case INS_DDMAD:
      pvr_pds_free_instruction_ddmad((struct pvr_ddmad *)instruction);
      case INS_DOUT:
      pvr_pds_free_instruction_dout((struct pvr_dout *)instruction);
      case INS_LD:
   case INS_ST:
      pvr_pds_free_instruction_ldst((struct pvr_ldst *)instruction);
      case INS_WDF:
   case INS_LOCK:
   case INS_RELEASE:
   case INS_HALT:
   case INS_NOP:
      pvr_pds_free_instruction_simple((struct pvr_simple *)instruction);
      case INS_SFTLP64:
   case INS_SFTLP32:
      pvr_pds_free_instruction_sfltp((struct pvr_sftlp *)instruction);
      case INS_STM:
      pvr_pds_free_instruction_stm((struct pvr_stm *)instruction);
      case INS_STMC:
      pvr_pds_free_instruction_stmc((struct pvr_stmc *)instruction);
         }
      struct pvr_instruction *
   pvr_pds_disassemble_instruction2(void *context,
               {
               /* First we need to find out what type of OPCODE we are dealing with. */
   if (instruction & PVR_TYPE_OPCODE) {
      uint32_t opcode_C = (instruction >> PVR_TYPE_OPCODE_SHIFT) &
         switch (opcode_C) {
   case PVR_ROGUE_PDSINST_OPCODEC_ADD64:
      error.instruction = INS_ADD64;
   return pvr_pds_disassemble_instruction_add64(context,
                  case PVR_ROGUE_PDSINST_OPCODEC_ADD32:
      error.instruction = INS_ADD32;
   return pvr_pds_disassemble_instruction_add32(context,
                  case PVR_ROGUE_PDSINST_OPCODEC_SFTLP64:
      error.instruction = INS_SFTLP64;
   return pvr_pds_disassemble_instruction_sftlp64(context,
                  case PVR_ROGUE_PDSINST_OPCODEC_CMP:
      error.instruction = INS_CMP;
   return pvr_pds_disassemble_instruction_cmp(context,
                  case PVR_ROGUE_PDSINST_OPCODEC_BRA:
      error.instruction = INS_BRA;
      case PVR_ROGUE_PDSINST_OPCODEC_SP:
      return pvr_pds_disassemble_instruction_sp(context,
                  case PVR_ROGUE_PDSINST_OPCODEC_DDMAD:
      error.instruction = INS_DDMAD;
   return pvr_pds_disassemble_instruction_ddmad(context,
                  case PVR_ROGUE_PDSINST_OPCODEC_DOUT:
      error.instruction = INS_DOUT;
   return pvr_pds_disassemble_instruction_dout(context,
                     } else if (instruction & PVR_TYPE_OPCODEB) {
      uint32_t opcode_B = (instruction >> PVR_TYPE_OPCODEB_SHIFT) &
         switch (opcode_B) {
   case PVR_ROGUE_PDSINST_OPCODEB_SFTLP32:
      error.instruction = INS_SFTLP32;
   return pvr_pds_disassemble_instruction_sftlp32(context,
                  case PVR_ROGUE_PDSINST_OPCODEB_STM:
      error.instruction = INS_STM;
   return pvr_pds_disassemble_instruction_stm(context,
                     } else { /* Opcode A - MAD instruction. */
      error.instruction = INS_MAD;
   return pvr_pds_disassemble_instruction_mad(context,
                  }
      }
