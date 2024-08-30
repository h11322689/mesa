   /**************************************************************************
   *
   * Copyright 2003 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "util/log.h"
   #include "util/ralloc.h"
   #include "util/u_debug.h"
   #include "i915_debug.h"
   #include "i915_debug_private.h"
   #include "i915_reg.h"
      #define PRINTF ralloc_asprintf_append
      static const char *opcodes[0x20] = {
      "NOP",     "ADD", "MOV",  "MUL",  "MAD",  "DP2ADD", "DP3",    "DP4",
   "FRC",     "RCP", "RSQ",  "EXP",  "LOG",  "CMP",    "MIN",    "MAX",
   "FLR",     "MOD", "TRC",  "SGE",  "SLT",  "TEXLD",  "TEXLDP", "TEXLDB",
      };
      static const int args[0x20] = {
      0, /* 0 nop */
   2, /* 1 add */
   1, /* 2 mov */
   2, /* 3 m ul */
   3, /* 4 mad */
   3, /* 5 dp2add */
   2, /* 6 dp3 */
   2, /* 7 dp4 */
   1, /* 8 frc */
   1, /* 9 rcp */
   1, /* a rsq */
   1, /* b exp */
   1, /* c log */
   3, /* d cmp */
   2, /* e min */
   2, /* f max */
   1, /* 10 flr */
   1, /* 11 mod */
   1, /* 12 trc */
   2, /* 13 sge */
   2, /* 14 slt */
      };
      static const char *regname[0x8] = {
         };
      static void
   print_reg_type_nr(char **stream, unsigned type, unsigned nr)
   {
      switch (type) {
   case REG_TYPE_T:
      switch (nr) {
   case T_DIFFUSE:
      PRINTF(stream, "T_DIFFUSE");
      case T_SPECULAR:
      PRINTF(stream, "T_SPECULAR");
      case T_FOG_W:
      PRINTF(stream, "T_FOG_W");
      default:
      PRINTF(stream, "T_TEX%d", nr);
         case REG_TYPE_OC:
      if (nr == 0) {
      PRINTF(stream, "oC");
      }
      case REG_TYPE_OD:
      if (nr == 0) {
      PRINTF(stream, "oD");
      }
      default:
                     }
      #define REG_SWIZZLE_MASK 0x7777
   #define REG_NEGATE_MASK  0x8888
      #define REG_SWIZZLE_XYZW                                                       \
      ((SRC_X << A2_SRC2_CHANNEL_X_SHIFT) | (SRC_Y << A2_SRC2_CHANNEL_Y_SHIFT) |  \
         static void
   print_reg_neg_swizzle(char **stream, unsigned reg)
   {
               if ((reg & REG_SWIZZLE_MASK) == REG_SWIZZLE_XYZW &&
      (reg & REG_NEGATE_MASK) == 0)
                  for (i = 3; i >= 0; i--) {
      if (reg & (1 << ((i * 4) + 3)))
            switch ((reg >> (i * 4)) & 0x7) {
   case 0:
      PRINTF(stream, "x");
      case 1:
      PRINTF(stream, "y");
      case 2:
      PRINTF(stream, "z");
      case 3:
      PRINTF(stream, "w");
      case 4:
      PRINTF(stream, "0");
      case 5:
      PRINTF(stream, "1");
      default:
      PRINTF(stream, "?");
            }
      static void
   print_src_reg(char **stream, unsigned dword)
   {
      unsigned nr = (dword >> A2_SRC2_NR_SHIFT) & REG_NR_MASK;
   unsigned type = (dword >> A2_SRC2_TYPE_SHIFT) & REG_TYPE_MASK;
   print_reg_type_nr(stream, type, nr);
      }
      static void
   print_dest_reg(char **stream, unsigned dword)
   {
      unsigned nr = (dword >> A0_DEST_NR_SHIFT) & REG_NR_MASK;
   unsigned type = (dword >> A0_DEST_TYPE_SHIFT) & REG_TYPE_MASK;
   print_reg_type_nr(stream, type, nr);
   if ((dword & A0_DEST_CHANNEL_ALL) == A0_DEST_CHANNEL_ALL)
         PRINTF(stream, ".");
   if (dword & A0_DEST_CHANNEL_X)
         if (dword & A0_DEST_CHANNEL_Y)
         if (dword & A0_DEST_CHANNEL_Z)
         if (dword & A0_DEST_CHANNEL_W)
      }
      #define GET_SRC0_REG(r0, r1) ((r0 << 14) | (r1 >> A1_SRC0_CHANNEL_W_SHIFT))
   #define GET_SRC1_REG(r0, r1) ((r0 << 8) | (r1 >> A2_SRC1_CHANNEL_W_SHIFT))
   #define GET_SRC2_REG(r)      (r)
      static void
   print_arith_op(char **stream, unsigned opcode, const unsigned *program)
   {
      if (opcode != A0_NOP) {
      print_dest_reg(stream, program[0]);
   if (program[0] & A0_DEST_SATURATE)
         else
                        print_src_reg(stream, GET_SRC0_REG(program[0], program[1]));
   if (args[opcode] == 1)
            PRINTF(stream, ", ");
   print_src_reg(stream, GET_SRC1_REG(program[1], program[2]));
   if (args[opcode] == 2)
            PRINTF(stream, ", ");
   print_src_reg(stream, GET_SRC2_REG(program[2]));
      }
      static void
   print_tex_op(char **stream, unsigned opcode, const unsigned *program)
   {
      print_dest_reg(stream, program[0] | A0_DEST_CHANNEL_ALL);
                              print_reg_type_nr(stream,
            }
      static void
   print_texkil_op(char **stream, unsigned opcode, const unsigned *program)
   {
               print_reg_type_nr(stream,
            }
      static void
   print_dcl_op(char **stream, unsigned opcode, const unsigned *program)
   {
                        unsigned dest_dword = program[0];
   if (type == REG_TYPE_S)
                  if (type == REG_TYPE_S) {
      switch (program[0] & D0_SAMPLE_TYPE_MASK) {
   case D0_SAMPLE_TYPE_2D:
      PRINTF(stream, " 2D");
      case D0_SAMPLE_TYPE_VOLUME:
      PRINTF(stream, " 3D");
      case D0_SAMPLE_TYPE_CUBE:
      PRINTF(stream, " CUBE");
      default:
      PRINTF(stream, " XXX bad type");
            }
      void
   i915_disassemble_program(const unsigned *program, unsigned sz)
   {
                                 program++;
   for (i = 1; i < sz; i += 3, program += 3) {
               char *stream = ralloc_strdup(NULL, "");
   if ((int)opcode >= A0_NOP && opcode <= A0_SLT)
         else if (opcode >= T0_TEXLD && opcode < T0_TEXKILL)
         else if (opcode == T0_TEXKILL)
         else if (opcode == D0_DCL)
         else
            mesa_logi("\t\t %s ", stream);
                  }
