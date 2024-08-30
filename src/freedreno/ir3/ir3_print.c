   /*
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include <stdarg.h>
   #include <stdio.h>
      #include "util/log.h"
   #include "ir3.h"
      #define PTRID(x) ((unsigned long)(x))
      /* ansi escape sequences: */
   #define RESET   "\x1b[0m"
   #define RED     "\x1b[0;31m"
   #define GREEN   "\x1b[0;32m"
   #define BLUE    "\x1b[0;34m"
   #define MAGENTA "\x1b[0;35m"
      /* syntax coloring, mostly to make it easier to see different sorts of
   * srcs (immediate, constant, ssa, array, ...)
   */
   #define SYN_REG(x)   RED x RESET
   #define SYN_IMMED(x) GREEN x RESET
   #define SYN_CONST(x) GREEN x RESET
   #define SYN_SSA(x)   BLUE x RESET
   #define SYN_ARRAY(x) MAGENTA x RESET
      static const char *
   type_name(type_t type)
   {
      static const char *type_names[] = {
      /* clang-format off */
   [TYPE_F16] = "f16",
   [TYPE_F32] = "f32",
   [TYPE_U16] = "u16",
   [TYPE_U32] = "u32",
   [TYPE_S16] = "s16",
   [TYPE_S32] = "s32",
   [TYPE_U8]  = "u8", 
   [TYPE_S8]  = "s8",
      };
      }
      static void
   print_instr_name(struct log_stream *stream, struct ir3_instruction *instr,
         {
      if (!instr)
      #ifdef DEBUG
         #endif
      mesa_log_stream_printf(stream, "%04u:", instr->ip);
   if (instr->flags & IR3_INSTR_UNUSED) {
         } else {
                  if (flags) {
      mesa_log_stream_printf(stream, "\t");
   if (instr->flags & IR3_INSTR_SY)
         if (instr->flags & IR3_INSTR_SS)
         if (instr->flags & IR3_INSTR_JP)
         if (instr->repeat)
         if (instr->nop)
         if (instr->flags & IR3_INSTR_UL)
      } else {
                  if (is_meta(instr)) {
      switch (instr->opc) {
   case OPC_META_INPUT:
      mesa_log_stream_printf(stream, "_meta:in");
      case OPC_META_SPLIT:
      mesa_log_stream_printf(stream, "_meta:split");
      case OPC_META_COLLECT:
      mesa_log_stream_printf(stream, "_meta:collect");
      case OPC_META_TEX_PREFETCH:
      mesa_log_stream_printf(stream, "_meta:tex_prefetch");
      case OPC_META_PARALLEL_COPY:
      mesa_log_stream_printf(stream, "_meta:parallel_copy");
      case OPC_META_PHI:
                  /* shouldn't hit here.. just for debugging: */
   default:
      mesa_log_stream_printf(stream, "_meta:%d", instr->opc);
         } else if (opc_cat(instr->opc) == 1) {
      if (instr->opc == OPC_MOV) {
      if (instr->cat1.src_type == instr->cat1.dst_type)
         else
      } else {
      mesa_log_stream_printf(stream, "%s",
               if (instr->opc == OPC_SCAN_MACRO) {
      switch (instr->cat1.reduce_op) {
   case REDUCE_OP_ADD_U:
      mesa_log_stream_printf(stream, ".add.u");
      case REDUCE_OP_ADD_F:
      mesa_log_stream_printf(stream, ".add.f");
      case REDUCE_OP_MUL_U:
      mesa_log_stream_printf(stream, ".mul.u");
      case REDUCE_OP_MUL_F:
      mesa_log_stream_printf(stream, ".mul.f");
      case REDUCE_OP_MIN_U:
      mesa_log_stream_printf(stream, ".min.u");
      case REDUCE_OP_MIN_S:
      mesa_log_stream_printf(stream, ".min.s");
      case REDUCE_OP_MIN_F:
      mesa_log_stream_printf(stream, ".min.f");
      case REDUCE_OP_MAX_U:
      mesa_log_stream_printf(stream, ".max.u");
      case REDUCE_OP_MAX_S:
      mesa_log_stream_printf(stream, ".max.s");
      case REDUCE_OP_MAX_F:
      mesa_log_stream_printf(stream, ".max.f");
      case REDUCE_OP_AND_B:
      mesa_log_stream_printf(stream, ".and.b");
      case REDUCE_OP_OR_B:
      mesa_log_stream_printf(stream, ".or.b");
      case REDUCE_OP_XOR_B:
      mesa_log_stream_printf(stream, ".xor.b");
                  if (instr->opc != OPC_MOVMSK && instr->opc != OPC_SCAN_MACRO &&
      instr->opc != OPC_PUSH_CONSTS_LOAD_MACRO) {
   mesa_log_stream_printf(stream, ".%s%s",
               } else if (instr->opc == OPC_B) {
      const char *name[8] = {
      /* clang-format off */
   [BRANCH_PLAIN] = "br",
   [BRANCH_OR]    = "brao",
   [BRANCH_AND]   = "braa",
   [BRANCH_CONST] = "brac",
   [BRANCH_ANY]   = "bany",
   [BRANCH_ALL]   = "ball",
   [BRANCH_X]     = "brax",
      };
      } else {
      mesa_log_stream_printf(stream, "%s", disasm_a3xx_instr_name(instr->opc));
   if (instr->flags & IR3_INSTR_3D)
         if (instr->flags & IR3_INSTR_A)
         if (instr->flags & IR3_INSTR_O)
         if (instr->flags & IR3_INSTR_P)
         if (instr->flags & IR3_INSTR_S)
         if (instr->flags & IR3_INSTR_A1EN)
         if (instr->opc == OPC_LDC)
         if (instr->opc == OPC_LDC_K)
         if (instr->flags & IR3_INSTR_B) {
      mesa_log_stream_printf(
      stream, ".base%d",
   }
   if (instr->flags & IR3_INSTR_S2EN)
            static const char *cond[0x7] = {
                  switch (instr->opc) {
   case OPC_CMPS_F:
   case OPC_CMPS_U:
   case OPC_CMPS_S:
   case OPC_CMPV_F:
   case OPC_CMPV_U:
   case OPC_CMPV_S:
      mesa_log_stream_printf(stream, ".%s",
            default:
               }
      static void
   print_ssa_def_name(struct log_stream *stream, struct ir3_register *reg)
   {
      mesa_log_stream_printf(stream, SYN_SSA("ssa_%u"), reg->instr->serialno);
   if (reg->name != 0)
      }
      static void
   print_ssa_name(struct log_stream *stream, struct ir3_register *reg, bool dst)
   {
      if (!dst) {
      if (!reg->def)
         else
      } else {
                  if (reg->num != INVALID_REG && !(reg->flags & IR3_REG_ARRAY))
      mesa_log_stream_printf(stream, "(" SYN_REG("r%u.%c") ")", reg_num(reg),
   }
      static void
   print_reg_name(struct log_stream *stream, struct ir3_instruction *instr,
         {
      if ((reg->flags & (IR3_REG_FABS | IR3_REG_SABS)) &&
      (reg->flags & (IR3_REG_FNEG | IR3_REG_SNEG | IR3_REG_BNOT)))
      else if (reg->flags & (IR3_REG_FNEG | IR3_REG_SNEG | IR3_REG_BNOT))
         else if (reg->flags & (IR3_REG_FABS | IR3_REG_SABS))
            if (reg->flags & IR3_REG_FIRST_KILL)
         if (reg->flags & IR3_REG_UNUSED)
            if (reg->flags & IR3_REG_R)
            if (reg->flags & IR3_REG_EARLY_CLOBBER)
            /* Right now all instructions that use tied registers only have one
   * destination register, so we can just print (tied) as if it's a flag,
   * although it's more convenient for RA if it's a pointer.
   */
   if (reg->tied)
            if (reg->flags & IR3_REG_SHARED)
         if (reg->flags & IR3_REG_HALF)
            if (reg->flags & IR3_REG_IMMED) {
      mesa_log_stream_printf(stream, SYN_IMMED("imm[%f,%d,0x%x]"), reg->fim_val,
      } else if (reg->flags & IR3_REG_ARRAY) {
      if (reg->flags & IR3_REG_SSA) {
      print_ssa_name(stream, reg, dest);
      }
   mesa_log_stream_printf(stream,
               if (reg->array.base != INVALID_REG)
      mesa_log_stream_printf(stream, "(" SYN_REG("r%u.%c") ")",
         } else if (reg->flags & IR3_REG_SSA) {
         } else if (reg->flags & IR3_REG_RELATIV) {
      if (reg->flags & IR3_REG_CONST)
      mesa_log_stream_printf(stream, SYN_CONST("c<a0.x + %d>"),
      else
      mesa_log_stream_printf(stream, SYN_REG("r<a0.x + %d>") " (%u)",
   } else {
      if (reg->flags & IR3_REG_CONST)
      mesa_log_stream_printf(stream, SYN_CONST("c%u.%c"), reg_num(reg),
      else
      mesa_log_stream_printf(stream, SYN_REG("r%u.%c"), reg_num(reg),
            if (reg->wrmask > 0x1)
      }
      static void
   tab(struct log_stream *stream, int lvl)
   {
      for (int i = 0; i < lvl; i++)
      }
      static void
   print_instr(struct log_stream *stream, struct ir3_instruction *instr, int lvl)
   {
                        if (is_tex(instr)) {
      mesa_log_stream_printf(stream, " (%s)(", type_name(instr->cat5.type));
   for (unsigned i = 0; i < 4; i++)
      if (instr->dsts[0]->wrmask & (1 << i))
         } else if ((instr->srcs_count > 0 || instr->dsts_count > 0) &&
            /* NOTE the b(ranch) instruction has a suffix, which is
   * handled below
   */
               if (!is_flow(instr) || instr->opc == OPC_END || instr->opc == OPC_CHMASK) {
      bool first = true;
   foreach_dst (reg, instr) {
      if (reg->wrmask == 0)
         if (!first)
         print_reg_name(stream, instr, reg, true);
      }
   foreach_src_n (reg, n, instr) {
      if (!first)
         print_reg_name(stream, instr, reg, false);
   if (instr->opc == OPC_END || instr->opc == OPC_CHMASK)
                        if (is_tex(instr) && !(instr->flags & IR3_INSTR_S2EN)) {
      if (!!(instr->flags & IR3_INSTR_B) && !!(instr->flags & IR3_INSTR_A1EN)) {
         } else {
      mesa_log_stream_printf(stream, ", s#%d, t#%d", instr->cat5.samp,
                  if (instr->opc == OPC_META_SPLIT) {
         } else if (instr->opc == OPC_META_TEX_PREFETCH) {
      mesa_log_stream_printf(stream, ", tex=%d, samp=%d, input_offset=%d",
            } else if (instr->opc == OPC_PUSH_CONSTS_LOAD_MACRO) {
      mesa_log_stream_printf(
      stream, " dst_offset=%d, src_offset = %d, src_size = %d",
   instr->push_consts.dst_base, instr->push_consts.src_base,
            if (is_flow(instr) && instr->cat0.target) {
      /* the predicate register src is implied: */
   if (instr->opc == OPC_B) {
      static const struct {
      int nsrc;
      } brinfo[7] = {
      /* clang-format off */
   [BRANCH_PLAIN] = {1, false},
   [BRANCH_OR]    = {2, false},
   [BRANCH_AND]   = {2, false},
   [BRANCH_CONST] = {0, true},
   [BRANCH_ANY]   = {1, false},
   [BRANCH_ALL]   = {1, false},
   [BRANCH_X]     = {0, false},
               if (brinfo[instr->cat0.brtype].idx) {
         }
   if (brinfo[instr->cat0.brtype].nsrc >= 1) {
      mesa_log_stream_printf(stream, " %sp0.%c (",
               print_reg_name(stream, instr, instr->srcs[0], false);
      }
   if (brinfo[instr->cat0.brtype].nsrc >= 2) {
      mesa_log_stream_printf(stream, " %sp0.%c (",
               print_reg_name(stream, instr, instr->srcs[1], false);
         }
   mesa_log_stream_printf(stream, " target=block%u",
               if (instr->deps_count) {
      mesa_log_stream_printf(stream, ", false-deps:");
   unsigned n = 0;
   for (unsigned i = 0; i < instr->deps_count; i++) {
      if (!instr->deps[i])
         if (n++ > 0)
         mesa_log_stream_printf(stream, SYN_SSA("ssa_%u"),
                     }
      void
   ir3_print_instr_stream(struct log_stream *stream, struct ir3_instruction *instr)
   {
         }
      void
   ir3_print_instr(struct ir3_instruction *instr)
   {
      struct log_stream *stream = mesa_log_streami();
   print_instr(stream, instr, 0);
      }
      static void
   print_block(struct ir3_block *block, int lvl)
   {
               tab(stream, lvl);
            if (block->predecessors_count > 0) {
      tab(stream, lvl + 1);
   mesa_log_stream_printf(stream, "pred: ");
   for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
   if (i != 0)
            }
               if (block->physical_predecessors_count > 0) {
      tab(stream, lvl + 1);
   mesa_log_stream_printf(stream, "physical pred: ");
   for (unsigned i = 0; i < block->physical_predecessors_count; i++) {
      struct ir3_block *pred = block->physical_predecessors[i];
   if (i != 0)
            }
               foreach_instr (instr, &block->instr_list) {
                  tab(stream, lvl + 1);
   mesa_log_stream_printf(stream, "/* keeps:\n");
   for (unsigned i = 0; i < block->keeps_count; i++) {
         }
   tab(stream, lvl + 1);
            if (block->successors[1]) {
      /* leading into if/else: */
   tab(stream, lvl + 1);
   mesa_log_stream_printf(stream, "/* succs: if ");
   switch (block->brtype) {
   case IR3_BRANCH_COND:
         case IR3_BRANCH_ANY:
      mesa_log_stream_printf(stream, "any ");
      case IR3_BRANCH_ALL:
      mesa_log_stream_printf(stream, "all ");
      case IR3_BRANCH_GETONE:
      mesa_log_stream_printf(stream, "getone ");
      case IR3_BRANCH_SHPS:
      mesa_log_stream_printf(stream, "shps ");
      }
   if (block->condition)
      mesa_log_stream_printf(stream, SYN_SSA("ssa_%u") " ",
      mesa_log_stream_printf(stream, "block%u; else block%u; */\n",
            } else if (block->successors[0]) {
      tab(stream, lvl + 1);
   mesa_log_stream_printf(stream, "/* succs: block%u; */\n",
      }
   if (block->physical_successors[0]) {
      tab(stream, lvl + 1);
   mesa_log_stream_printf(stream, "/* physical succs: block%u",
         if (block->physical_successors[1]) {
      mesa_log_stream_printf(stream, ", block%u",
      }
      }
   tab(stream, lvl);
      }
      void
   ir3_print(struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list)
      }
