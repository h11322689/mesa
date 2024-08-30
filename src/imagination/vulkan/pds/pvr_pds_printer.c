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
      #include <stdbool.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
      #include "pvr_rogue_pds_defs.h"
   #include "pvr_rogue_pds_disasm.h"
   #include "pvr_rogue_pds_encode.h"
   #include "util/log.h"
      #define X(lop, str) #str,
   static const char *const LOP[] = { PVR_PDS_LOP };
   #undef X
      static void pvr_pds_disassemble_operand(struct pvr_operand *op,
               {
   #define X(enum, str, size) { #str, #size },
         #undef X
         if (op->type == LITERAL_NUM) {
      snprintf(instr_str,
            instr_len,
   "%s (%llu)",
   } else if (op->type == UNRESOLVED) {
         } else {
      snprintf(instr_str,
            instr_len,
   "%s[%u].%s",
   regs[op->type][0],
      }
      static void pvr_pds_disassemble_instruction_add64(struct pvr_add *add,
               {
      char dst[32];
   char src0[32];
            pvr_pds_disassemble_operand(add->src0, src0, sizeof(src0));
   pvr_pds_disassemble_operand(add->src1, src1, sizeof(src1));
            snprintf(instr_str,
            instr_len,
   "%-16s%s%s = %s %s %s %s",
   "ADD64",
   add->cc ? "? " : "",
   dst,
   src0,
   add->sna ? "-" : "+",
   }
      static void pvr_pds_disassemble_instruction_add32(struct pvr_add *add,
               {
      char dst[32];
   char src0[32];
            pvr_pds_disassemble_operand(add->src0, src0, sizeof(src0));
   pvr_pds_disassemble_operand(add->src1, src1, sizeof(src1));
            snprintf(instr_str,
            instr_len,
   "%-16s%s%s = %s %s %s %s",
   "ADD32",
   add->cc ? "? " : "",
   dst,
   src0,
   add->sna ? "-" : "+",
   }
      static void
   pvr_pds_disassemble_instruction_sftlp32(struct pvr_sftlp *instruction,
               {
      char dst[32];
   char src0[32];
   char src1[32];
            pvr_pds_disassemble_operand(instruction->src0, src0, sizeof(src0));
   pvr_pds_disassemble_operand(instruction->src1, src1, sizeof(src1));
            if (instruction->IM)
         else
            if (instruction->lop == LOP_NONE) {
      snprintf(instr_str,
            instr_len,
   "%-16s%s%s = %s %s %s",
   "SFTLP32",
   instruction->cc ? "? " : "",
   dst,
   src0,
   } else if (instruction->lop == LOP_NOT) {
      snprintf(instr_str,
            instr_len,
   "%-16s%s%s = (~%s) %s %s",
   "SFTLP32",
   instruction->cc ? "? " : "",
   dst,
   src0,
   } else {
      snprintf(instr_str,
            instr_len,
   "%-16s%s%s = (%s %s %s) %s %s",
   "SFTLP32",
   instruction->cc ? "? " : "",
   dst,
   src0,
   LOP[instruction->lop],
   src1,
      }
      static void pvr_pds_disassemble_instruction_stm(struct pvr_stm *instruction,
               {
      char src0[32];
   char src1[32];
   char src2[32];
                     pvr_pds_disassemble_operand(instruction->src0, src0, sizeof(src0));
   pvr_pds_disassemble_operand(instruction->src1, src1, sizeof(src1));
   pvr_pds_disassemble_operand(instruction->src2, src2, sizeof(src2));
            if (instruction->ccs_global)
         else if (instruction->ccs_so)
         else
            snprintf(instr_str,
            instr_len,
   "%-16s%s%s%s stm%u = %s, %s, %s, %s",
   "STM",
   instruction->cc ? "? " : "",
   stm_pred,
   instruction->tst ? " (TST only)" : "",
   instruction->stream_out,
   src0,
   src1,
   }
      static void pds_disassemble_instruction_stmc(struct pvr_stmc *instruction,
               {
                        snprintf(instr_str,
            instr_len,
   "%-16s%s %s",
   "STMC",
   }
      static void
   pvr_pds_disassemble_instruction_sftlp64(struct pvr_sftlp *instruction,
               {
      char dst[32];
   char src0[32];
   char src1[32];
            pvr_pds_disassemble_operand(instruction->src0, src0, sizeof(src0));
   pvr_pds_disassemble_operand(instruction->src1, src1, sizeof(src1));
            if (instruction->IM)
         else
            if (instruction->lop == LOP_NONE) {
      snprintf(instr_str,
            instr_len,
   "%-16s%s%s = %s %s %s",
   "SFTLP64",
   instruction->cc ? "? " : "",
   dst,
   src0,
   } else if (instruction->lop == LOP_NOT) {
      snprintf(instr_str,
            instr_len,
   "%-16s%s%s = (~%s) %s %s",
   "SFTLP64",
   instruction->cc ? "? " : "",
   dst,
   src0,
   } else {
      snprintf(instr_str,
            instr_len,
   "%-16s%s%s = (%s %s %s) %s %s",
   "SFTLP64",
   instruction->cc ? "? " : "",
   dst,
   src0,
   LOP[instruction->lop],
   src1,
      }
      static void pvr_pds_disassemble_instruction_cmp(struct pvr_cmp *cmp,
               {
      char src0[32];
   char src1[32];
                     if (cmp->IM) {
      snprintf(src1,
            sizeof(src1),
   } else {
                  snprintf(instr_str,
            instr_len,
   "%-16s%sP0 = (%s %s %s)",
   "CMP",
   cmp->cc ? "? " : "",
   src0,
   }
      static void pvr_pds_disassemble_instruction_ldst(struct pvr_ldst *ins,
               {
                        if (ins->st) {
      snprintf(instr_str,
            instr_len,
   "%-16s%s%s: mem(%s) <= src(%s)",
   "ST",
   ins->cc ? "? " : "",
   src0,
   } else {
      snprintf(instr_str,
            instr_len,
   "%-16s%s%s: dst(%s) <= mem(%s)",
   "ld",
   ins->cc ? "? " : "",
   src0,
      }
      static void pvr_pds_disassemble_simple(struct pvr_simple *simple,
                     {
         }
      static void pvr_pds_disassemble_instruction_limm(struct pvr_limm *limm,
               {
      int32_t imm = (uint32_t)limm->src0->literal;
                     if (limm->GR) {
               switch (imm) {
   case 0:
      pchGReg = "cluster";
      case 1:
      pchGReg = "instance";
      default:
                  snprintf(instr_str,
            instr_len,
   "%-16s%s%s = G%d (%s)",
   "LIMM",
   limm->cc ? "? " : "",
   dst,
   } else {
      snprintf(instr_str,
            instr_len,
   "%-16s%s%s = %#04x",
   "LIMM",
   limm->cc ? "? " : "",
      }
      static void pvr_pds_disassemble_instruction_ddmad(struct pvr_ddmad *ddmad,
               {
      char src0[PVR_PDS_MAX_INST_STR_LEN];
   char src1[PVR_PDS_MAX_INST_STR_LEN];
   char src2[PVR_PDS_MAX_INST_STR_LEN];
            pvr_pds_disassemble_operand(ddmad->src0, src0, sizeof(src0));
   pvr_pds_disassemble_operand(ddmad->src1, src1, sizeof(src1));
   pvr_pds_disassemble_operand(ddmad->src2, src2, sizeof(src2));
            snprintf(instr_str,
            instr_len,
   "%-16s%sdoutd = (%s * %s) + %s, %s%s",
   "DDMAD",
   ddmad->cc ? "? " : "",
   src0,
   src1,
   src2,
   }
      static void pvr_pds_disassemble_predicate(uint32_t predicate,
               {
      switch (predicate) {
   case PVR_ROGUE_PDSINST_PREDICATE_P0:
      snprintf(buffer, buffer_length, "%s", "p0");
      case PVR_ROGUE_PDSINST_PREDICATE_IF0:
      snprintf(buffer, buffer_length, "%s", "if0");
      case PVR_ROGUE_PDSINST_PREDICATE_IF1:
      snprintf(buffer, buffer_length, "%s", "if1");
      case PVR_ROGUE_PDSINST_PREDICATE_SO_OVERFLOW_PREDICATE_0:
      snprintf(buffer, buffer_length, "%s", "so_overflow_0");
      case PVR_ROGUE_PDSINST_PREDICATE_SO_OVERFLOW_PREDICATE_1:
      snprintf(buffer, buffer_length, "%s", "so_overflow_1");
      case PVR_ROGUE_PDSINST_PREDICATE_SO_OVERFLOW_PREDICATE_2:
      snprintf(buffer, buffer_length, "%s", "so_overflow_2");
      case PVR_ROGUE_PDSINST_PREDICATE_SO_OVERFLOW_PREDICATE_3:
      snprintf(buffer, buffer_length, "%s", "so_overflow_3");
      case PVR_ROGUE_PDSINST_PREDICATE_SO_OVERFLOW_PREDICATE_GLOBAL:
      snprintf(buffer, buffer_length, "%s", "so_overflow_any");
      case PVR_ROGUE_PDSINST_PREDICATE_KEEP:
      snprintf(buffer, buffer_length, "%s", "keep");
      case PVR_ROGUE_PDSINST_PREDICATE_OOB:
      snprintf(buffer, buffer_length, "%s", "oob");
      default:
      snprintf(buffer, buffer_length, "%s", "<ERROR>");
         }
      static void pvr_pds_disassemble_instruction_bra(struct pvr_bra *bra,
               {
      char setc_pred[32];
            pvr_pds_disassemble_predicate(bra->srcc->predicate,
               pvr_pds_disassemble_predicate(bra->setc->predicate,
                  if (bra->setc->predicate != PVR_ROGUE_PDSINST_PREDICATE_KEEP) {
      snprintf(instr_str,
            instr_len,
   "%-16sif %s%s %d ( setc = %s )",
   "BRA",
   bra->srcc->negate ? "! " : "",
   srcc_pred,
   } else {
      snprintf(instr_str,
            instr_len,
   "%-16sif %s%s %d",
   "BRA",
   bra->srcc->negate ? "! " : "",
      }
      static void pvr_pds_disassemble_instruction_mad(struct pvr_mad *mad,
               {
      char src0[PVR_PDS_MAX_INST_STR_LEN];
   char src1[PVR_PDS_MAX_INST_STR_LEN];
   char src2[PVR_PDS_MAX_INST_STR_LEN];
            pvr_pds_disassemble_operand(mad->src0, src0, sizeof(src0));
   pvr_pds_disassemble_operand(mad->src1, src1, sizeof(src1));
   pvr_pds_disassemble_operand(mad->src2, src2, sizeof(src2));
            snprintf(instr_str,
            instr_len,
   "%-16s%s%s = (%s * %s) %s %s%s",
   "MAD",
   mad->cc ? "? " : "",
   dst,
   src0,
   src1,
   mad->sna ? "-" : "+",
   }
      static void pvr_pds_disassemble_instruction_dout(struct pvr_dout *dout,
               {
      char src0[PVR_PDS_MAX_INST_STR_LEN];
         #define X(dout_dst, str) #str,
         #undef X
         pvr_pds_disassemble_operand(dout->src0, src0, sizeof(src0));
            {
      snprintf(instr_str,
            instr_len,
   "%-16s%s%s = %s, %s%s",
   "DOUT",
   dout->cc ? "? " : "",
   dst[dout->dst],
   src0,
      }
      void pvr_pds_disassemble_instruction(char *instr_str,
               {
      if (!instruction) {
      snprintf(instr_str,
                           switch (instruction->type) {
   case INS_LIMM:
      pvr_pds_disassemble_instruction_limm((struct pvr_limm *)instruction,
                  case INS_ADD64:
      pvr_pds_disassemble_instruction_add64((struct pvr_add *)instruction,
                  case INS_ADD32:
      pvr_pds_disassemble_instruction_add32((struct pvr_add *)instruction,
                  case INS_CMP:
      pvr_pds_disassemble_instruction_cmp((struct pvr_cmp *)instruction,
                  case INS_MAD:
      pvr_pds_disassemble_instruction_mad((struct pvr_mad *)instruction,
                  case INS_BRA:
      pvr_pds_disassemble_instruction_bra((struct pvr_bra *)instruction,
                  case INS_DDMAD:
      pvr_pds_disassemble_instruction_ddmad((struct pvr_ddmad *)instruction,
                  case INS_DOUT:
      pvr_pds_disassemble_instruction_dout((struct pvr_dout *)instruction,
                  case INS_LD:
   case INS_ST:
      pvr_pds_disassemble_instruction_ldst((struct pvr_ldst *)instruction,
                  case INS_WDF:
      pvr_pds_disassemble_simple((struct pvr_simple *)instruction,
                        case INS_LOCK:
      pvr_pds_disassemble_simple((struct pvr_simple *)instruction,
                        case INS_RELEASE:
      pvr_pds_disassemble_simple((struct pvr_simple *)instruction,
                        case INS_HALT:
      pvr_pds_disassemble_simple((struct pvr_simple *)instruction,
                        case INS_NOP:
      pvr_pds_disassemble_simple((struct pvr_simple *)instruction,
                        case INS_SFTLP32:
      pvr_pds_disassemble_instruction_sftlp32((struct pvr_sftlp *)instruction,
                  case INS_SFTLP64:
      pvr_pds_disassemble_instruction_sftlp64((struct pvr_sftlp *)instruction,
                  case INS_STM:
      pvr_pds_disassemble_instruction_stm((struct pvr_stm *)instruction,
                  case INS_STMC:
      pds_disassemble_instruction_stmc((struct pvr_stmc *)instruction,
                  default:
      snprintf(instr_str, instr_len, "Printing not implemented\n");
         }
      #if defined(DUMP_PDS)
   void pvr_pds_print_instruction(uint32_t instr)
   {
      char instruction_str[1024];
   struct pvr_instruction *decoded =
            if (!decoded) {
         } else {
      pvr_pds_disassemble_instruction(instruction_str,
                     }
   #endif
