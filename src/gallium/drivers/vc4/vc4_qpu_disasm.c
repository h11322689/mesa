   /*
   * Copyright © 2014 Broadcom
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <stdbool.h>
   #include <stdio.h>
      #include "vc4_qpu.h"
   #include "vc4_qpu_defines.h"
      static const char *qpu_add_opcodes[] = {
         [QPU_A_NOP] = "nop",
   [QPU_A_FADD] = "fadd",
   [QPU_A_FSUB] = "fsub",
   [QPU_A_FMIN] = "fmin",
   [QPU_A_FMAX] = "fmax",
   [QPU_A_FMINABS] = "fminabs",
   [QPU_A_FMAXABS] = "fmaxabs",
   [QPU_A_FTOI] = "ftoi",
   [QPU_A_ITOF] = "itof",
   [QPU_A_ADD] = "add",
   [QPU_A_SUB] = "sub",
   [QPU_A_SHR] = "shr",
   [QPU_A_ASR] = "asr",
   [QPU_A_ROR] = "ror",
   [QPU_A_SHL] = "shl",
   [QPU_A_MIN] = "min",
   [QPU_A_MAX] = "max",
   [QPU_A_AND] = "and",
   [QPU_A_OR] = "or",
   [QPU_A_XOR] = "xor",
   [QPU_A_NOT] = "not",
   [QPU_A_CLZ] = "clz",
   [QPU_A_V8ADDS] = "v8adds",
   };
      static const char *qpu_mul_opcodes[] = {
         [QPU_M_NOP] = "nop",
   [QPU_M_FMUL] = "fmul",
   [QPU_M_MUL24] = "mul24",
   [QPU_M_V8MULD] = "v8muld",
   [QPU_M_V8MIN] = "v8min",
   [QPU_M_V8MAX] = "v8max",
   [QPU_M_V8ADDS] = "v8adds",
   };
      static const char *qpu_sig[] = {
         [QPU_SIG_SW_BREAKPOINT] = "sig_brk",
   [QPU_SIG_NONE] = "",
   [QPU_SIG_THREAD_SWITCH] = "sig_switch",
   [QPU_SIG_PROG_END] = "sig_end",
   [QPU_SIG_WAIT_FOR_SCOREBOARD] = "sig_wait_score",
   [QPU_SIG_SCOREBOARD_UNLOCK] = "sig_unlock_score",
   [QPU_SIG_LAST_THREAD_SWITCH] = "sig_thread_switch",
   [QPU_SIG_COVERAGE_LOAD] = "sig_coverage_load",
   [QPU_SIG_COLOR_LOAD] = "sig_color_load",
   [QPU_SIG_COLOR_LOAD_END] = "sig_color_load_end",
   [QPU_SIG_LOAD_TMU0] = "load_tmu0",
   [QPU_SIG_LOAD_TMU1] = "load_tmu1",
   [QPU_SIG_ALPHA_MASK_LOAD] = "sig_alpha_mask_load",
   [QPU_SIG_SMALL_IMM] = "sig_small_imm",
   [QPU_SIG_LOAD_IMM] = "sig_load_imm",
   };
      static const char *qpu_pack_mul[] = {
         [QPU_PACK_MUL_NOP] = "",
   [QPU_PACK_MUL_8888] = ".8888",
   [QPU_PACK_MUL_8A] = ".8a",
   [QPU_PACK_MUL_8B] = ".8b",
   [QPU_PACK_MUL_8C] = ".8c",
   };
      /* The QPU unpack for A and R4 files can be described the same, it's just that
   * the R4 variants are convert-to-float only, with no int support.
   */
   static const char *qpu_unpack[] = {
         [QPU_UNPACK_NOP] = "",
   [QPU_UNPACK_16A] = "16a",
   [QPU_UNPACK_16B] = "16b",
   [QPU_UNPACK_8D_REP] = "8d_rep",
   [QPU_UNPACK_8A] = "8a",
   [QPU_UNPACK_8B] = "8b",
   [QPU_UNPACK_8C] = "8c",
   };
      static const char *special_read_a[] = {
         "uni",
   NULL,
   NULL,
   "vary",
   NULL,
   NULL,
   "elem",
   "nop",
   NULL,
   "x_pix",
   "ms_flags",
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   "vpm_read",
   "vpm_ld_busy",
   "vpm_ld_wait",
   };
      static const char *special_read_b[] = {
         "uni",
   NULL,
   NULL,
   "vary",
   NULL,
   NULL,
   "qpu",
   "nop",
   NULL,
   "y_pix",
   "rev_flag",
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   "vpm_read",
   "vpm_st_busy",
   "vpm_st_wait",
   };
      /**
   * This has the B-file descriptions for register writes.
   *
   * Since only a couple of regs are different between A and B, the A overrides
   * are in get_special_write_desc().
   */
   static const char *special_write[] = {
         [QPU_W_ACC0] = "r0",
   [QPU_W_ACC1] = "r1",
   [QPU_W_ACC2] = "r2",
   [QPU_W_ACC3] = "r3",
   [QPU_W_TMU_NOSWAP] = "tmu_noswap",
   [QPU_W_ACC5] = "r5",
   [QPU_W_HOST_INT] = "host_int",
   [QPU_W_NOP] = "nop",
   [QPU_W_UNIFORMS_ADDRESS] = "uniforms_addr",
   [QPU_W_QUAD_XY] = "quad_y",
   [QPU_W_MS_FLAGS] = "ms_flags",
   [QPU_W_TLB_STENCIL_SETUP] = "tlb_stencil_setup",
   [QPU_W_TLB_Z] = "tlb_z",
   [QPU_W_TLB_COLOR_MS] = "tlb_color_ms",
   [QPU_W_TLB_COLOR_ALL] = "tlb_color_all",
   [QPU_W_VPM] = "vpm",
   [QPU_W_VPMVCD_SETUP] = "vw_setup",
   [QPU_W_VPM_ADDR] = "vw_addr",
   [QPU_W_MUTEX_RELEASE] = "mutex_release",
   [QPU_W_SFU_RECIP] = "sfu_recip",
   [QPU_W_SFU_RECIPSQRT] = "sfu_recipsqrt",
   [QPU_W_SFU_EXP] = "sfu_exp",
   [QPU_W_SFU_LOG] = "sfu_log",
   [QPU_W_TMU0_S] = "tmu0_s",
   [QPU_W_TMU0_T] = "tmu0_t",
   [QPU_W_TMU0_R] = "tmu0_r",
   [QPU_W_TMU0_B] = "tmu0_b",
   [QPU_W_TMU1_S] = "tmu1_s",
   [QPU_W_TMU1_T] = "tmu1_t",
   [QPU_W_TMU1_R] = "tmu1_r",
   };
      static const char *qpu_pack_a[] = {
         [QPU_PACK_A_NOP] = "",
   [QPU_PACK_A_16A] = ".16a",
   [QPU_PACK_A_16B] = ".16b",
   [QPU_PACK_A_8888] = ".8888",
   [QPU_PACK_A_8A] = ".8a",
   [QPU_PACK_A_8B] = ".8b",
   [QPU_PACK_A_8C] = ".8c",
            [QPU_PACK_A_32_SAT] = ".sat",
   [QPU_PACK_A_16A_SAT] = ".16a.sat",
   [QPU_PACK_A_16B_SAT] = ".16b.sat",
   [QPU_PACK_A_8888_SAT] = ".8888.sat",
   [QPU_PACK_A_8A_SAT] = ".8a.sat",
   [QPU_PACK_A_8B_SAT] = ".8b.sat",
   [QPU_PACK_A_8C_SAT] = ".8c.sat",
   };
      static const char *qpu_cond[] = {
         [QPU_COND_NEVER] = ".never",
   [QPU_COND_ALWAYS] = "",
   [QPU_COND_ZS] = ".zs",
   [QPU_COND_ZC] = ".zc",
   [QPU_COND_NS] = ".ns",
   [QPU_COND_NC] = ".nc",
   [QPU_COND_CS] = ".cs",
   };
      static const char *qpu_cond_branch[] = {
         [QPU_COND_BRANCH_ALL_ZS] = ".all_zs",
   [QPU_COND_BRANCH_ALL_ZC] = ".all_zc",
   [QPU_COND_BRANCH_ANY_ZS] = ".any_zs",
   [QPU_COND_BRANCH_ANY_ZC] = ".any_zc",
   [QPU_COND_BRANCH_ALL_NS] = ".all_ns",
   [QPU_COND_BRANCH_ALL_NC] = ".all_nc",
   [QPU_COND_BRANCH_ANY_NS] = ".any_ns",
   [QPU_COND_BRANCH_ANY_NC] = ".any_nc",
   [QPU_COND_BRANCH_ALL_CS] = ".all_cs",
   [QPU_COND_BRANCH_ALL_CC] = ".all_cc",
   [QPU_COND_BRANCH_ANY_CS] = ".any_cs",
   [QPU_COND_BRANCH_ANY_CC] = ".any_cc",
   };
      #define DESC(array, index)                                        \
         ((index >= ARRAY_SIZE(array) || !(array)[index]) ?         \
      static const char *
   get_special_write_desc(int reg, bool is_a)
   {
         if (is_a) {
            switch (reg) {
   case QPU_W_QUAD_XY:
         case QPU_W_VPMVCD_SETUP:
         case QPU_W_VPM_ADDR:
               }
      void
   vc4_qpu_disasm_pack_mul(FILE *out, uint32_t pack)
   {
         }
      void
   vc4_qpu_disasm_pack_a(FILE *out, uint32_t pack)
   {
         }
      void
   vc4_qpu_disasm_unpack(FILE *out, uint32_t unpack)
   {
         if (unpack != QPU_UNPACK_NOP)
   }
      void
   vc4_qpu_disasm_cond(FILE *out, uint32_t cond)
   {
         }
      void
   vc4_qpu_disasm_cond_branch(FILE *out, uint32_t cond)
   {
         }
      static void
   print_alu_dst(uint64_t inst, bool is_mul)
   {
         bool is_a = is_mul == ((inst & QPU_WS) != 0);
   uint32_t waddr = (is_mul ?
               const char *file = is_a ? "a" : "b";
            if (waddr <= 31)
         else if (get_special_write_desc(waddr, is_a))
         else
            if (is_mul && (inst & QPU_PM)) {
         } else if (is_a && !(inst & QPU_PM)) {
         }
      static void
   print_alu_src(uint64_t inst, uint32_t mux, bool is_mul)
   {
         bool is_a = mux != QPU_MUX_B;
   const char *file = is_a ? "a" : "b";
   uint32_t raddr = (is_a ?
               uint32_t unpack = QPU_GET_FIELD(inst, QPU_UNPACK);
   bool has_si = QPU_GET_FIELD(inst, QPU_SIG) == QPU_SIG_SMALL_IMM;
            if (mux <= QPU_MUX_R5) {
            fprintf(stderr, "r%d", mux);
      } else if (!is_a && has_si) {
            if (si <= 15)
         else if (si <= 31)
         else if (si <= 39)
         else if (si <= 47)
            } else if (raddr <= 31)
         else {
            if (is_a)
                     if (((mux == QPU_MUX_A && !(inst & QPU_PM)) ||
         (mux == QPU_MUX_R4 && (inst & QPU_PM)))) {
   }
      static void
   print_add_op(uint64_t inst)
   {
         uint32_t op_add = QPU_GET_FIELD(inst, QPU_OP_ADD);
   uint32_t cond = QPU_GET_FIELD(inst, QPU_COND_ADD);
   bool is_mov = (op_add == QPU_A_OR &&
                  if (is_mov)
         else
            if ((inst & QPU_SF) && op_add != QPU_A_NOP)
            if (op_add != QPU_A_NOP)
            fprintf(stderr, " ");
   print_alu_dst(inst, false);
                     if (!is_mov) {
                  }
      static void
   print_mul_op(uint64_t inst)
   {
         uint32_t op_add = QPU_GET_FIELD(inst, QPU_OP_ADD);
   uint32_t op_mul = QPU_GET_FIELD(inst, QPU_OP_MUL);
   uint32_t cond = QPU_GET_FIELD(inst, QPU_COND_MUL);
   bool is_mov = (op_mul == QPU_M_V8MIN &&
                  if (is_mov)
         else
            if ((inst & QPU_SF) && op_add == QPU_A_NOP)
            if (op_mul != QPU_M_NOP)
            fprintf(stderr, " ");
   print_alu_dst(inst, true);
                     if (!is_mov) {
               }
      static void
   print_load_imm(uint64_t inst)
   {
         uint32_t imm = inst;
   uint32_t waddr_add = QPU_GET_FIELD(inst, QPU_WADDR_ADD);
   uint32_t waddr_mul = QPU_GET_FIELD(inst, QPU_WADDR_MUL);
   uint32_t cond_add = QPU_GET_FIELD(inst, QPU_COND_ADD);
                     print_alu_dst(inst, false);
   if (waddr_add != QPU_W_NOP)
                  print_alu_dst(inst, true);
   if (waddr_mul != QPU_W_NOP)
                  }
      void
   vc4_qpu_disasm(const uint64_t *instructions, int num_instructions)
   {
         for (int i = 0; i < num_instructions; i++) {
                           switch (sig) {
   case QPU_SIG_BRANCH:
                                             case QPU_SIG_LOAD_IMM:
               default:
            if (sig != QPU_SIG_NONE)
         print_add_op(inst);
                        }
