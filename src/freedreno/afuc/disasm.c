   /*
   * Copyright (c) 2017 Rob Clark <robdclark@gmail.com>
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
   */
      #include <assert.h>
   #include <err.h>
   #include <fcntl.h>
   #include <getopt.h>
   #include <stdarg.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
      #include "util/os_file.h"
      #include "compiler/isaspec/isaspec.h"
      #include "freedreno_pm4.h"
      #include "afuc.h"
   #include "util.h"
   #include "emu.h"
      int gpuver;
      /* non-verbose mode should output something suitable to feed back into
   * assembler.. verbose mode has additional output useful for debugging
   * (like unexpected bits that are set)
   */
   static bool verbose = false;
      /* emulator mode: */
   static bool emulator = false;
      #define printerr(fmt, ...) afuc_printc(AFUC_ERR, fmt, ##__VA_ARGS__)
   #define printlbl(fmt, ...) afuc_printc(AFUC_LBL, fmt, ##__VA_ARGS__)
      static const char *
   getpm4(uint32_t id)
   {
         }
      static void
   print_gpu_reg(FILE *out, uint32_t regbase)
   {
      if (regbase < 0x100)
            char *name = afuc_gpu_reg_name(regbase);
   if (name) {
      fprintf(out, "\t; %s", name);
         }
      void
   print_control_reg(uint32_t id)
   {
      char *name = afuc_control_reg_name(id);
   if (name) {
      printf("@%s", name);
      } else {
            }
      void
   print_pipe_reg(uint32_t id)
   {
      char *name = afuc_pipe_reg_name(id);
   if (name) {
      printf("|%s", name);
      } else {
            }
      struct decode_state {
      uint32_t immed;
   uint8_t shift;
   bool has_immed;
      };
      static void
   field_print_cb(struct isa_print_state *state, const char *field_name, uint64_t val)
   {
      if (!strcmp(field_name, "CONTROLREG")) {
      char *name = afuc_control_reg_name(val);
   if (name) {
      isa_print(state, "@%s", name);
      } else {
               }
      static void
   pre_instr_cb(void *data, unsigned n, void *instr)
   {
      struct decode_state *state = data;
   state->has_immed = state->dst_is_addr = false;
            if (verbose)
      }
      static void
   field_cb(void *data, const char *field_name, struct isa_decode_value *val)
   {
               if (!strcmp(field_name, "RIMMED")) {
      state->immed = val->num;
               if (!strcmp(field_name, "SHIFT")) {
                  if (!strcmp(field_name, "DST")) {
      if (val->num == REG_ADDR)
         }
      static void
   post_instr_cb(void *data, unsigned n, void *instr)
   {
               if (state->has_immed) {
      uint32_t immed = state->immed << state->shift;
   if (state->dst_is_addr && state->shift >= 16) {
      immed &= ~0x40000; /* b18 disables auto-increment of address */
   if ((immed & 0x00ffffff) == 0) {
      printf("\t; ");
         } else {
               }
      /* Assume that instructions that don't match are raw data */
   static void
   no_match(FILE *out, const BITSET_WORD *bitset, size_t size)
   {
      fprintf(out, "[%08x]", bitset[0]);
   print_gpu_reg(out, bitset[0]);
      }
      static void
   get_decode_options(struct isa_decode_options *options)
   {
      *options = (struct isa_decode_options) {
      .gpu_id = gpuver,
   .branch_labels = true,
   .field_cb = field_cb,
   .field_print_cb = field_print_cb,
   .pre_instr_cb = pre_instr_cb,
   .post_instr_cb = post_instr_cb,
         }
      static void
   disasm_instr(struct isa_decode_options *options, uint32_t *instrs, unsigned pc)
   {
         }
      static void
   setup_packet_table(struct isa_decode_options *options,
         {
               for (unsigned i = 0; i < sizedwords; i++) {
      entrypoints[i].offset = jmptbl[i];
   unsigned n = i; // + CP_NOP;
   entrypoints[i].name = afuc_pm_id_name(n);
   if (!entrypoints[i].name) {
      char *name;
   asprintf(&name, "UNKN%d", n);
                  options->entrypoints = entrypoints;
      }
      static void
   disasm(struct emu *emu)
   {
      uint32_t sizedwords = emu->sizedwords;
            EMU_GPU_REG(CP_SQE_INSTR_BASE);
   EMU_GPU_REG(CP_LPAC_SQE_INSTR_BASE);
   EMU_CONTROL_REG(BV_INSTR_BASE);
            emu_init(emu);
            struct isa_decode_options options;
   struct decode_state state;
   get_decode_options(&options);
         #ifdef BOOTSTRAP_DEBUG
      while (true) {
      disasm_instr(&options, emu->instrs, emu->gpr_regs.pc);
         #endif
                  /* Figure out if we have BV/LPAC SQE appended: */
   if (gpuver >= 7) {
      bv_offset = emu_get_reg64(emu, &BV_INSTR_BASE) -
         bv_offset /= 4;
   lpac_offset = emu_get_reg64(emu, &LPAC_INSTR_BASE) -
         lpac_offset /= 4;
      } else {
      if (emu_get_reg64(emu, &CP_LPAC_SQE_INSTR_BASE)) {
      lpac_offset = emu_get_reg64(emu, &CP_LPAC_SQE_INSTR_BASE) -
         lpac_offset /= 4;
                           /* TODO add option to emulate LPAC SQE instead: */
   if (emulator) {
      /* Start from clean slate: */
   emu_fini(emu);
            while (true) {
      disasm_instr(&options, emu->instrs, emu->gpr_regs.pc);
                  /* print instructions: */
            if (bv_offset) {
      printf(";\n");
   printf("; BV microcode:\n");
                     emu->processor = EMU_PROC_BV;
   emu->instrs += bv_offset;
            emu_init(emu);
                                       emu->instrs -= bv_offset;
               if (lpac_offset) {
      printf(";\n");
   printf("; LPAC microcode:\n");
                     emu->processor = EMU_PROC_LPAC;
   emu->instrs += lpac_offset;
            emu_init(emu);
                              emu->instrs -= lpac_offset;
         }
      static void
   disasm_raw(uint32_t *instrs, int sizedwords)
   {
      struct isa_decode_options options;
   struct decode_state state;
   get_decode_options(&options);
               }
      static void
   disasm_legacy(uint32_t *buf, int sizedwords)
   {
      uint32_t *instrs = buf;
   const int jmptbl_start = instrs[1] & 0xffff;
   uint32_t *jmptbl = &buf[jmptbl_start];
            struct isa_decode_options options;
   struct decode_state state;
   get_decode_options(&options);
            /* parse jumptable: */
            /* print instructions: */
            /* print jumptable: */
   if (verbose) {
      printf(";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n");
   printf("; JUMP TABLE\n");
   for (i = 0; i < 0x7f; i++) {
      int n = i; // + CP_NOP;
   uint32_t offset = jmptbl[i];
   const char *name = getpm4(n);
   printf("%3d %02x: ", n, n);
   printf("%04x", offset);
   if (name) {
         } else {
         }
            }
      static void
   usage(void)
   {
      fprintf(stderr, "Usage:\n"
                  "\tdisasm [-g GPUVER] [-v] [-c] [-r] filename.asm\n"
   "\t\t-c - use colors\n"
   "\t\t-e - emulator mode\n"
   "\t\t-g - specify GPU version (5, etc)\n"
         }
      int
   main(int argc, char **argv)
   {
      uint32_t *buf;
   char *file;
   bool colors = false;
   uint32_t gpu_id = 0;
   size_t sz;
   int c, ret;
   bool unit_test = false;
            /* Argument parsing: */
   while ((c = getopt(argc, argv, "ceg:rvu")) != -1) {
      switch (c) {
   case 'c':
      colors = true;
      case 'e':
      emulator = true;
   verbose  = true;
      case 'g':
      gpu_id = atoi(optarg);
      case 'r':
      raw = true;
      case 'v':
      verbose = true;
      case 'u':
      /* special "hidden" flag for unit tests, to avoid file paths (which
   * can differ from reference output)
   */
   unit_test = true;
      default:
                     if (optind >= argc) {
      fprintf(stderr, "no file specified!\n");
                        /* if gpu version not specified, infer from filename: */
   if (!gpu_id) {
      char *str = strstr(file, "a5");
   if (!str)
         if (str)
               if (gpu_id < 500) {
      printf("invalid gpu_id: %d\n", gpu_id);
                        /* a6xx is *mostly* a superset of a5xx, but some opcodes shuffle
   * around, and behavior of special regs is a bit different.  Right
   * now we only bother to support the a6xx variant.
   */
   if (emulator && (gpuver != 6)) {
      fprintf(stderr, "Emulator only supported on a6xx!\n");
               ret = afuc_util_init(gpuver, colors);
   if (ret < 0) {
                                    if (!unit_test)
                  if (raw) {
         } else if (gpuver < 6) {
         } else {
      struct emu emu = {
         .instrs = &buf[1],
   .sizedwords = sz / 4 - 1,
                           }
