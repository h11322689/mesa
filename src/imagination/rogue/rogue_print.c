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
      #include "compiler/shader_enums.h"
   #include "rogue.h"
   #include "util/bitscan.h"
   #include "util/macros.h"
      #include <inttypes.h>
   #include <stdbool.h>
      /**
   * \file rogue_print.c
   *
   * \brief Contains functions to print Rogue IR types and structures.
   */
      /* TODO NEXT: Go through and make types the same, i.e. decide on using ONLY
   * unsigned, uint32/64_t, etc., and then use inttypes if so */
   /* TODO NEXT: Make fp the last argument. */
      enum color_esc {
      ESC_RESET = 0,
   ESC_BLACK,
   ESC_RED,
   ESC_GREEN,
   ESC_YELLOW,
   ESC_BLUE,
   ESC_PURPLE,
   ESC_CYAN,
               };
      static
   const char *color_esc[2][ESC_COUNT] = {
      [0] = {
      [ESC_RESET] = "",
   [ESC_BLACK] = "",
   [ESC_RED] = "",
   [ESC_GREEN] = "",
   [ESC_YELLOW] = "",
   [ESC_BLUE] = "",
   [ESC_PURPLE] = "",
   [ESC_CYAN] = "",
      },
   [1] = {
      [ESC_RESET] = "\033[0m",
   [ESC_BLACK] = "\033[0;30m",
   [ESC_RED] = "\033[0;31m",
   [ESC_GREEN] = "\033[0;32m",
   [ESC_YELLOW] = "\033[0;33m",
   [ESC_BLUE] = "\033[0;34m",
   [ESC_PURPLE] = "\033[0;35m",
   [ESC_CYAN] = "\033[0;36m",
         };
      static inline void RESET(FILE *fp)
   {
         }
      static inline void BLACK(FILE *fp)
   {
         }
      static inline void RED(FILE *fp)
   {
         }
      static inline void GREEN(FILE *fp)
   {
         }
      static inline void YELLOW(FILE *fp)
   {
         }
      static inline void BLUE(FILE *fp)
   {
         }
      static inline void PURPLE(FILE *fp)
   {
         }
      static inline void CYAN(FILE *fp)
   {
         }
      static inline void WHITE(FILE *fp)
   {
         }
      static inline void rogue_print_val(FILE *fp, unsigned val)
   {
      PURPLE(fp);
   fprintf(fp, "%u", val);
      }
      static inline void rogue_print_reg(FILE *fp, const rogue_reg *reg)
   {
      const rogue_reg_info *info = &rogue_reg_infos[reg->class];
   YELLOW(fp);
   fprintf(fp, "%s%" PRIu32, info->str, reg->index);
      }
      static inline void rogue_print_regarray(FILE *fp,
         {
      const rogue_reg *reg = regarray->regs[0];
   const rogue_reg_info *info = &rogue_reg_infos[reg->class];
   YELLOW(fp);
   fprintf(fp, "%s[%" PRIu32, info->str, reg->index);
   if (regarray->size > 1) {
      RESET(fp);
   fputs("..", fp);
   YELLOW(fp);
      }
   fputs("]", fp);
      }
      static inline void rogue_print_imm(FILE *fp, const rogue_imm *imm)
   {
      PURPLE(fp);
   fprintf(fp, "0x%" PRIx32, imm->imm.u32);
      }
      static inline void rogue_print_io(FILE *fp, enum rogue_io io)
   {
      const rogue_io_info *info = &rogue_io_infos[io];
   BLUE(fp);
   fprintf(fp, "%s", info->str);
      }
      static inline void rogue_print_drc(FILE *fp, const rogue_drc *drc)
   {
      RED(fp);
   fprintf(fp, "drc%u", drc->index);
      }
      static inline void rogue_print_ref(FILE *fp, const rogue_ref *ref)
   {
      switch (ref->type) {
   case ROGUE_REF_TYPE_VAL:
      rogue_print_val(fp, ref->val);
         case ROGUE_REF_TYPE_REG:
      rogue_print_reg(fp, ref->reg);
         case ROGUE_REF_TYPE_REGARRAY:
      rogue_print_regarray(fp, ref->regarray);
         case ROGUE_REF_TYPE_IMM:
      rogue_print_imm(fp, &ref->imm);
         case ROGUE_REF_TYPE_IO:
      rogue_print_io(fp, ref->io);
         case ROGUE_REF_TYPE_DRC:
      rogue_print_drc(fp, &ref->drc);
         default:
            }
      static inline void rogue_print_alu_dst(FILE *fp, const rogue_instr_dst *dst)
   {
               uint64_t mod = dst->mod;
   while (mod) {
      enum rogue_alu_dst_mod dst_mod = u_bit_scan64(&mod);
   assert(dst_mod < ROGUE_ALU_DST_MOD_COUNT);
         }
      static inline void rogue_print_alu_src(FILE *fp, const rogue_instr_src *src)
   {
               uint64_t mod = src->mod;
   while (mod) {
      enum rogue_alu_src_mod src_mod = u_bit_scan64(&mod);
   assert(src_mod < ROGUE_ALU_SRC_MOD_COUNT);
         }
      static inline void rogue_print_alu_mods(FILE *fp, const rogue_alu_instr *alu)
   {
      uint64_t mod = alu->mod;
   while (mod) {
      enum rogue_alu_op_mod op_mod = u_bit_scan64(&mod);
   assert(op_mod < ROGUE_ALU_OP_MOD_COUNT);
         }
      static inline void rogue_print_alu_instr(FILE *fp, const rogue_alu_instr *alu)
   {
                                          for (unsigned i = 0; i < info->num_dsts; ++i) {
      if (i > 0)
                                 for (unsigned i = 0; i < info->num_srcs; ++i) {
      if (i == 0 && !info->num_dsts)
         else
                  }
      static inline void rogue_print_block_label(FILE *fp, const rogue_block *block)
   {
      /* For debug purposes. */
   if (block->label)
         else
      }
      static inline void rogue_print_backend_dst(FILE *fp, const rogue_instr_dst *dst)
   {
         }
      static inline void rogue_print_backend_src(FILE *fp, const rogue_instr_src *src)
   {
         }
      static inline void rogue_print_backend_mods(FILE *fp,
         {
      uint64_t mod = backend->mod;
   while (mod) {
      enum rogue_backend_op_mod op_mod = u_bit_scan64(&mod);
   assert(op_mod < ROGUE_BACKEND_OP_MOD_COUNT);
         }
      static inline void rogue_print_backend_instr(FILE *fp,
         {
                                 for (unsigned i = 0; i < info->num_dsts; ++i) {
      if (i > 0)
                                 for (unsigned i = 0; i < info->num_srcs; ++i) {
      if (i == 0 && !info->num_dsts)
         else
                  }
      static inline void rogue_print_ctrl_mods(FILE *fp, const rogue_ctrl_instr *ctrl)
   {
      uint64_t mod = ctrl->mod;
   while (mod) {
      enum rogue_ctrl_op_mod op_mod = u_bit_scan64(&mod);
   assert(op_mod < ROGUE_CTRL_OP_MOD_COUNT);
         }
      static inline void rogue_print_ctrl_src(FILE *fp, const rogue_instr_src *src)
   {
         }
      static inline void rogue_print_ctrl_instr(FILE *fp,
         {
                                          if (ctrl->target_block) {
      fputs(" ", fp);
               /* TODO NEXT: Dests. */
   /* TODO: Special case for the conditional ctrl instructions as they're
            for (unsigned i = 0; i < info->num_srcs; ++i) {
      if (i == 0 && !info->num_dsts)
         else
                  }
      static inline void rogue_print_bitwise_dst(FILE *fp, const rogue_instr_dst *dst)
   {
         }
      static inline void rogue_print_bitwise_src(FILE *fp, const rogue_instr_src *src)
   {
         }
      static inline void rogue_print_bitwise_instr(FILE *fp,
         {
                                 for (unsigned i = 0; i < info->num_dsts; ++i) {
      if (i > 0)
                                 for (unsigned i = 0; i < info->num_srcs; ++i) {
      if (i == 0 && !info->num_dsts)
         else
                  }
      PUBLIC
   void rogue_print_instr(FILE *fp, const rogue_instr *instr)
   {
      if (instr->exec_cond > ROGUE_EXEC_COND_PE_TRUE)
            if (instr->repeat > 1)
            GREEN(fp);
   switch (instr->type) {
   case ROGUE_INSTR_TYPE_ALU:
      rogue_print_alu_instr(fp, rogue_instr_as_alu(instr));
         case ROGUE_INSTR_TYPE_BACKEND:
      rogue_print_backend_instr(fp, rogue_instr_as_backend(instr));
         case ROGUE_INSTR_TYPE_CTRL:
      rogue_print_ctrl_instr(fp, rogue_instr_as_ctrl(instr));
         case ROGUE_INSTR_TYPE_BITWISE:
      rogue_print_bitwise_instr(fp, rogue_instr_as_bitwise(instr));
         default:
         }
            if (instr->end)
            /* For debug purposes. */
            if (instr->comment)
      }
      /* TODO NEXT: Split this up into separate functions for printing lower srcs,
   * upper srcs, etc. since we'd want to print them in-between instructions. */
   /* TODO NEXT: Commonise with printing the ref io stuff. */
   static inline void
   rogue_print_instr_group_io_sel(FILE *fp, const rogue_instr_group_io_sel *io_sel)
   {
                        /* TODO NEXT: Commonise this code!! */
   /* Print upper and lower sources. */
   for (unsigned i = 0; i < ARRAY_SIZE(io_sel->srcs); ++i) {
      if (rogue_ref_is_null(&io_sel->srcs[i]))
            if (present && i > 0)
                     rogue_print_io(fp, ROGUE_IO_S0 + i);
            if (rogue_ref_is_reg(&io_sel->srcs[i]))
         else if (rogue_ref_is_regarray(&io_sel->srcs[i]))
         else if (rogue_ref_is_io(&io_sel->srcs[i]))
         else
      }
   if (present)
            /* Print internal sources. */
   present = false;
   for (unsigned i = 0; i < ARRAY_SIZE(io_sel->iss); ++i) {
      if (rogue_ref_is_null(&io_sel->iss[i]))
            if (present && i > 0)
                     rogue_print_io(fp, ROGUE_IO_IS0 + i);
            if (rogue_ref_is_reg(&io_sel->iss[i]))
         else if (rogue_ref_is_regarray(&io_sel->iss[i]))
         else if (rogue_ref_is_io(&io_sel->iss[i]))
         else
      }
   if (present)
            /* Print destinations. */
   present = false;
   for (unsigned i = 0; i < ARRAY_SIZE(io_sel->dsts); ++i) {
      if (rogue_ref_is_null(&io_sel->dsts[i]))
            if (present && i > 0)
                     rogue_print_io(fp, ROGUE_IO_W0 + i);
            if (rogue_ref_is_reg(&io_sel->dsts[i]))
         else if (rogue_ref_is_regarray(&io_sel->dsts[i]))
         else if (rogue_ref_is_io(&io_sel->dsts[i]))
         else
      }
   if (present)
      }
      static inline void rogue_print_instr_phase(FILE *fp,
               {
      const char *phase_str = rogue_instr_phase_str[alu][phase];
   assert(phase_str);
      }
      static inline void
   rogue_print_instr_group_header(FILE *fp, const rogue_instr_group *group)
   {
      /* ALU specific */
   switch (group->header.alu) {
   case ROGUE_ALU_MAIN:
            case ROGUE_ALU_BITWISE:
            case ROGUE_ALU_CONTROL:
            default:
                  if (group->header.end)
      }
      static inline void rogue_print_instr_group(FILE *fp,
         {
      /* For debug purposes. */
   fprintf(fp, "%u", group->index);
            if (group->header.exec_cond > ROGUE_EXEC_COND_PE_TRUE)
            if (group->header.repeat > 1)
                     CYAN(fp);
   fprintf(fp, "%s", rogue_alu_str[group->header.alu]);
            /* Print each instruction. */
   rogue_foreach_phase_in_set (p, group->header.phases) {
      const rogue_instr *instr = group->instrs[p];
            fputs(" ", fp);
   rogue_print_instr_phase(fp, group->header.alu, p);
   fputs(": ", fp);
               /* Print source/dest mappings (if present). */
                     /* Print group header info. */
      }
      static inline void rogue_print_block(FILE *fp, const rogue_block *block)
   {
      rogue_print_block_label(fp, block);
            if (!block->shader->is_grouped) {
      rogue_foreach_instr_in_block (instr, block) {
      fputs("\t", fp);
   fprintf(fp, "%u", instr->index);
   fputs(": ", fp);
   fprintf(fp, "%s: ", rogue_instr_type_str[instr->type]);
   rogue_print_instr(fp, instr);
         } else {
      rogue_foreach_instr_group_in_block (group, block) {
      fputs("\t", fp);
   rogue_print_instr_group(fp, group);
            }
      PUBLIC
   void rogue_print_shader(FILE *fp, const rogue_shader *shader)
   {
               if (shader->stage == MESA_SHADER_NONE)
         else
            if (shader->name)
                     rogue_foreach_block (block, shader)
      }
      static void rogue_print_instr_ref(FILE *fp,
                           {
      if (is_grouped) {
      fprintf(fp, "%u", instr->group->index);
   fputs(": { ", fp);
      } else {
      fprintf(fp, "%u", instr->index);
   if (index != ~0)
               if (index != ~0) {
      BLUE(fp);
   fprintf(fp, "[%s%u]", dst ? "dst" : "src", index);
               if (is_grouped)
      }
      PUBLIC
   void rogue_print_reg_writes(FILE *fp, const rogue_shader *shader)
   {
      fputs("/* register writes */\n", fp);
   for (enum rogue_reg_class class = 0; class < ROGUE_REG_CLASS_COUNT;
      ++class) {
   rogue_foreach_reg (reg, shader, class) {
                              rogue_foreach_reg_write (write, reg) {
                     fputs(" ", fp);
   rogue_print_instr_ref(fp,
                                 if (reg->regarray) {
      rogue_foreach_regarray_write (write, reg->regarray) {
                     fputs(" ", fp);
   rogue_print_instr_ref(fp,
                                 rogue_foreach_subarray (subarray, reg->regarray) {
      unsigned subarray_start = subarray->regs[0]->index;
                                                fputs(" ", fp);
   rogue_print_instr_ref(fp,
                                       if (unused) {
      fputs(" <none>\n", fp);
                        }
      PUBLIC
   void rogue_print_reg_uses(FILE *fp, const rogue_shader *shader)
   {
      fputs("/* register uses */\n", fp);
   for (enum rogue_reg_class class = 0; class < ROGUE_REG_CLASS_COUNT;
      ++class) {
   rogue_foreach_reg (reg, shader, class) {
                              rogue_foreach_reg_use (use, reg) {
                     fputs(" ", fp);
   rogue_print_instr_ref(fp,
                                 if (reg->regarray) {
      rogue_foreach_regarray_use (use, reg->regarray) {
                     fputs(" ", fp);
   rogue_print_instr_ref(fp,
                                 rogue_foreach_subarray (subarray, reg->regarray) {
      unsigned subarray_start = subarray->regs[0]->index;
                                                fputs(" ", fp);
   rogue_print_instr_ref(fp,
                                       if (unused) {
      fputs(" <none>\n", fp);
                        }
      PUBLIC
   void rogue_print_block_uses(FILE *fp, const rogue_shader *shader)
   {
      fputs("/* block uses */\n", fp);
   rogue_foreach_block (block, shader) {
      rogue_print_block_label(fp, block);
            if (list_is_empty(&block->uses)) {
      if (list_first_entry(&shader->blocks, rogue_block, link) == block)
                                    rogue_foreach_block_use (use, block) {
               fputs(" ", fp);
                     }
      static void rogue_print_drc_trxn(FILE *fp,
                     {
                                 if (drc_trxn->release) {
      rogue_print_instr_ref(fp,
                        } else {
                     }
      PUBLIC
   void rogue_print_drc_trxns(FILE *fp, const rogue_shader *shader)
   {
               rogue_foreach_drc_trxn (drc_trxn, shader, 0) {
                  rogue_foreach_drc_trxn (drc_trxn, shader, 1) {
            }
