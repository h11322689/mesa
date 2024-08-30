   /*
   * Copyright (c) 2016 Etnaviv Project
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
   *    Christian Gmeiner <christian.gmeiner@gmail.com>
   */
      #include "etnaviv_disasm.h"
   #include "etnaviv_asm.h"
      #include <assert.h>
   #include <stdbool.h>
   #include <stdio.h>
   #include <stdlib.h>
      #include "hw/isa.xml.h"
   #include "util/u_math.h"
   #include "util/half_float.h"
      struct instr {
      /* dword0: */
   uint32_t opc         : 6;
   uint32_t cond        : 5;
   uint32_t sat         : 1;
   uint32_t dst_use     : 1;
   uint32_t dst_amode   : 3;
   uint32_t dst_reg     : 7;
   uint32_t dst_comps   : 4;
            /* dword1: */
   uint32_t tex_amode   : 3;
   uint32_t tex_swiz    : 8;
   uint32_t src0_use    : 1;
   uint32_t src0_reg    : 9;
   uint32_t type_bit2   : 1;
   uint32_t src0_swiz   : 8;
   uint32_t src0_neg    : 1;
            /* dword2: */
   uint32_t src0_amode  : 3;
   uint32_t src0_rgroup : 3;
   uint32_t src1_use    : 1;
   uint32_t src1_reg    : 9;
   uint32_t opcode_bit6 : 1;
   uint32_t src1_swiz   : 8;
   uint32_t src1_neg    : 1;
   uint32_t src1_abs    : 1;
   uint32_t src1_amode  : 3;
            /* dword3: */
   union {
      struct {
      uint32_t src1_rgroup : 3;
   uint32_t src2_use    : 1;
   uint32_t src2_reg    : 9;
   uint32_t sel_0       : 1;
   uint32_t src2_swiz   : 8;
   uint32_t src2_neg    : 1;
   uint32_t src2_abs    : 1;
   uint32_t sel_1       : 1;
   uint32_t src2_amode  : 3;
   uint32_t src2_rgroup : 3;
      };
         };
   struct opc_operands {
      struct etna_inst_dst *dst;
   struct etna_inst_tex *tex;
   struct etna_inst_src *src0;
   struct etna_inst_src *src1;
               };
      static void
   printf_type(uint8_t type)
   {
      switch(type) {
   case INST_TYPE_F32:
      /* as f32 is the default print nothing */
         case INST_TYPE_S32:
      printf(".s32");
         case INST_TYPE_S8:
      printf(".s8");
         case INST_TYPE_U16:
      printf(".u16");
         case INST_TYPE_F16:
      printf(".f16");
         case INST_TYPE_S16:
      printf(".s16");
         case INST_TYPE_U32:
      printf(".u32");
         case INST_TYPE_U8:
      printf(".u8");
         default:
      abort();
         }
      static void
   print_condition(uint8_t condition)
   {
      switch (condition) {
   case INST_CONDITION_TRUE:
            case INST_CONDITION_GT:
      printf(".GT");
         case INST_CONDITION_LT:
      printf(".LT");
         case INST_CONDITION_GE:
      printf(".GE");
         case INST_CONDITION_LE:
      printf(".LE");
         case INST_CONDITION_EQ:
      printf(".EQ");
         case INST_CONDITION_NE:
      printf(".NE");
         case INST_CONDITION_AND:
      printf(".AND");
         case INST_CONDITION_OR:
      printf(".OR");
         case INST_CONDITION_XOR:
      printf(".XOR");
         case INST_CONDITION_NOT:
      printf(".NOT");
         case INST_CONDITION_NZ:
      printf(".NZ");
         case INST_CONDITION_GEZ:
      printf(".GEZ");
         case INST_CONDITION_GZ:
      printf(".GZ");
         case INST_CONDITION_LEZ:
      printf(".LEZ");
         case INST_CONDITION_LZ:
      printf(".LZ");
         default:
      abort();
         }
      static void
   print_rgroup(uint8_t rgoup)
   {
      switch (rgoup) {
   case INST_RGROUP_TEMP:
      printf("t");
         case INST_RGROUP_INTERNAL:
      printf("i");
         case INST_RGROUP_UNIFORM_0:
   case INST_RGROUP_UNIFORM_1:
      printf("u");
      case 4:
      printf("th");
         }
      static void
   print_components(uint8_t components)
   {
      if (components == 15)
            printf(".");
   if (components & INST_COMPS_X)
         else
            if (components & INST_COMPS_Y)
         else
            if (components & INST_COMPS_Z)
         else
            if (components & INST_COMPS_W)
         else
      }
      static inline void
   print_swiz_comp(uint8_t swiz_comp)
   {
      switch (swiz_comp) {
   case INST_SWIZ_COMP_X:
      printf("x");
         case INST_SWIZ_COMP_Y:
      printf("y");
         case INST_SWIZ_COMP_Z:
      printf("z");
         case INST_SWIZ_COMP_W:
      printf("w");
         default:
      abort();
         }
      static void
   print_swiz(uint8_t swiz)
   {
      // if a null swizzle
   if (swiz == 0xe4)
            const unsigned x = swiz & 0x3;
   const unsigned y = (swiz & 0x0C) >> 2;
   const unsigned z = (swiz & 0x30) >> 4;
            printf(".");
   print_swiz_comp(x);
   print_swiz_comp(y);
   print_swiz_comp(z);
      }
      static void
   print_amode(uint8_t amode)
   {
      switch (amode) {
   case INST_AMODE_DIRECT:
      /* nothing to output */
         case INST_AMODE_ADD_A_X:
      printf("[a.x]");
         case INST_AMODE_ADD_A_Y:
      printf("[a.y]");
         case INST_AMODE_ADD_A_Z:
      printf("[a.z]");
         case INST_AMODE_ADD_A_W:
      printf("[a.w]");
         default:
      abort();
         }
      static void
   print_dst(struct etna_inst_dst *dst, bool sep)
   {
      if (dst->use) {
      printf("t%u", dst->reg);
   print_amode(dst->amode);
      } else {
                  if (sep)
      }
      static void
   print_tex(struct etna_inst_tex *tex, bool sep)
   {
      printf("tex%u", tex->id);
   print_amode(tex->amode);
            if (sep)
      }
      static void
   print_src(struct etna_inst_src *src, bool sep)
   {
      if (src->use) {
      if (src->rgroup == INST_RGROUP_IMMEDIATE) {
      switch (src->imm_type) {
   case 0: /* float */
      printf("%f", uif(src->imm_val << 12));
      case 1: /* signed */
      printf("%d", ((int) src->imm_val << 12) >> 12);
      case 2: /* unsigned */
      printf("%d", src->imm_val);
      case 3: /* 16-bit */
      printf("%f/%.5X", _mesa_half_to_float(src->imm_val), src->imm_val);
         } else {
                                                   print_rgroup(src->rgroup);
   printf("%u", src->reg);
                  if (src->abs)
         } else {
                  if (sep)
      }
      static void
   print_opc_default(struct opc_operands *operands)
   {
      print_dst(operands->dst, true);
   print_src(operands->src0, true);
   print_src(operands->src1, true);
      }
      static void
   print_opc_mov(struct opc_operands *operands)
   {
      // dst (areg)
   printf("a%u", operands->dst->reg);
   print_components(operands->dst->write_mask);
            print_src(operands->src0, true);
   print_src(operands->src1, true);
      }
      static void
   print_opc_tex(struct opc_operands *operands)
   {
      print_dst(operands->dst, true);
   print_tex(operands->tex, true);
   print_src(operands->src0, true);
   print_src(operands->src1, true);
      }
      static void
   print_opc_imm(struct opc_operands *operands)
   {
      print_dst(operands->dst, true);
   print_src(operands->src0, true);
   print_src(operands->src1, true);
      }
      static void
   print_opc_store(struct opc_operands *operands)
   {
      printf("mem");
   print_components(operands->dst->write_mask);
            print_src(operands->src0, true);
   print_src(operands->src1, true);
      }
      #define OPC_BITS 7
      static const struct opc_info {
      const char *name;
      } opcs[1 << OPC_BITS] = {
   #define OPC(opc) [INST_OPCODE_##opc] = {#opc, print_opc_default}
   #define OPC_MOV(opc) [INST_OPCODE_##opc] = {#opc, print_opc_mov}
   #define OPC_TEX(opc) [INST_OPCODE_##opc] = {#opc, print_opc_tex}
   #define OPC_IMM(opc) [INST_OPCODE_##opc] = {#opc, print_opc_imm}
   #define OPC_STORE(opc) [INST_OPCODE_##opc] = {#opc, print_opc_store}
      OPC(NOP),
   OPC(ADD),
   OPC(MAD),
   OPC(MUL),
   OPC(DST),
   OPC(DP3),
   OPC(DP4),
   OPC(DSX),
   OPC(DSY),
   OPC(MOV),
   OPC_MOV(MOVAR),
   OPC_MOV(MOVAF),
   OPC_MOV(MOVAI),
   OPC(RCP),
   OPC(RSQ),
   OPC(LITP),
   OPC(SELECT),
   OPC(SET),
   OPC(EXP),
   OPC(LOG),
   OPC(FRC),
   OPC_IMM(CALL),
   OPC(RET),
   OPC_IMM(BRANCH),
   OPC_TEX(TEXKILL),
   OPC_TEX(TEXLD),
   OPC_TEX(TEXLDB),
   OPC_TEX(TEXLDD),
   OPC_TEX(TEXLDL),
   OPC_TEX(TEXLDPCF),
   OPC_TEX(TEXLDLPCF),
   OPC_TEX(TEXLDGPCF),
   OPC(REP),
   OPC(ENDREP),
   OPC(LOOP),
   OPC(ENDLOOP),
   OPC(SQRT),
   OPC(SIN),
   OPC(COS),
   OPC(FLOOR),
   OPC(CEIL),
   OPC(SIGN),
   OPC(I2I),
   OPC(I2F),
   OPC(F2I),
   OPC(CMP),
   OPC(LOAD),
   OPC_STORE(STORE),
   OPC(IMULLO0),
   OPC(IMULHI0),
   OPC(IMADLO0),
   OPC(IMADHI0),
   OPC(LEADZERO),
   OPC(LSHIFT),
   OPC(RSHIFT),
   OPC(ROTATE),
   OPC(OR),
   OPC(AND),
   OPC(XOR),
   OPC(NOT),
   OPC(DP2),
   OPC(DIV),
      };
      static void
   print_instr(uint32_t *dwords, int n, enum debug_t debug)
   {
      struct instr *instr = (struct instr *)dwords;
   const unsigned opc = instr->opc | (instr->opcode_bit6 << 6);
            printf("%04d: ", n);
   if (debug & PRINT_RAW)
      printf("%08x %08x %08x %08x  ", dwords[0], dwords[1], dwords[2],
                     struct etna_inst_dst dst = {
      .use = instr->dst_use,
   .amode = instr->dst_amode,
   .reg = instr->dst_reg,
               struct etna_inst_tex tex = {
      .id = instr->tex_id,
   .amode = instr->tex_amode,
               struct etna_inst_src src0 = {
      .use = instr->src0_use,
   .neg = instr->src0_neg,
   .abs = instr->src0_abs,
   .rgroup = instr->src0_rgroup,
   .reg = instr->src0_reg,
   .swiz = instr->src0_swiz,
               struct etna_inst_src src1 = {
      .use = instr->src1_use,
   .neg = instr->src1_neg,
   .abs = instr->src1_abs,
   .rgroup = instr->src1_rgroup,
   .reg = instr->src1_reg,
   .swiz = instr->src1_swiz,
               struct etna_inst_src src2 = {
      .use = instr->src2_use,
   .neg = instr->src2_neg,
   .abs = instr->src2_abs,
   .rgroup = instr->src2_rgroup,
   .reg = instr->src2_reg,
   .swiz = instr->src2_swiz,
               int imm = (instr->dword3 & VIV_ISA_WORD_3_SRC2_IMM__MASK)
            struct opc_operands operands = {
      .dst = &dst,
   .tex = &tex,
   .src0 = &src0,
   .src1 = &src1,
   .src2 = &src2,
                        printf("%s", name);
   printf_type(type);
   if (instr->sat)
         print_condition(instr->cond);
   printf(" ");
   if (instr->sel_0)
         if (instr->sel_1)
         if (instr->dst_full)
            } else {
                     }
      void
   etna_disasm(uint32_t *dwords, int sizedwords, enum debug_t debug)
   {
                        for (i = 0; i < sizedwords; i += 4)
      }
