   /*
   * Copyright © 2021 Google, Inc.
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
   #include <ctype.h>
   #include <inttypes.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <termios.h>
   #include <unistd.h>
      #include "freedreno_pm4.h"
      #include "emu.h"
   #include "util.h"
      /*
   * Emulator User Interface:
   *
   * Handles the user prompts and input parsing.
   */
      static void
   clear_line(void)
   {
      if (!isatty(STDOUT_FILENO))
            }
      static int
   readchar(void)
   {
      static struct termios saved_termios, unbuffered_termios;
                     tcgetattr(STDIN_FILENO, &saved_termios);
   unbuffered_termios = saved_termios;
            tcsetattr(STDIN_FILENO, TCSANOW, &unbuffered_termios);
   do {
         } while (isspace(c));
            /* TODO, read from script until EOF and then read from stdin: */
   if (c == -1)
               }
      static const char *
   extract_string(char **buf)
   {
               /* eat any leading whitespace: */
   while (*p && isspace(*p))
            if (!*p)
                     /* skip to next whitespace: */
   while (*p && !isspace(*p))
            if (*p)
                        }
      static size_t
   readline(char **p)
   {
      static char *buf;
            ssize_t ret = getline(&buf, &n, stdin);
   if (ret < 0)
            *p = buf;
      }
      static ssize_t
   read_two_values(const char **val1, const char **val2)
   {
               ssize_t ret = readline(&p);
   if (ret < 0)
            *val1 = extract_string(&p);
               }
      static ssize_t
   read_one_value(const char **val)
   {
               ssize_t ret = readline(&p);
   if (ret < 0)
                        }
      static void
   print_dst(unsigned reg)
   {
      if (reg == REG_REM)
         else if (reg == REG_ADDR)
         else if (reg == REG_USRADDR)
         else if (reg == REG_DATA)
         else
      }
      static void
   dump_gpr_register(struct emu *emu, unsigned n)
   {
      printf("              GPR:  ");
   print_dst(n);
   printf(": ");
   if (BITSET_TEST(emu->gpr_regs.written, n)) {
         } else {
            }
      static void
   dump_gpr_registers(struct emu *emu)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(emu->gpr_regs.val); i++) {
            }
      static void
   dump_gpu_register(struct emu *emu, unsigned n)
   {
      printf("              GPU:  ");
   char *name = afuc_gpu_reg_name(n);
   if (name) {
      printf("%s", name);
      } else {
         }
   printf(": ");
   if (BITSET_TEST(emu->gpu_regs.written, n)) {
         } else {
            }
      static void
   dump_pipe_register(struct emu *emu, unsigned n)
   {
      printf("              PIPE: ");
   print_pipe_reg(n);
   printf(": ");
   if (BITSET_TEST(emu->pipe_regs.written, n)) {
         } else {
            }
      static void
   dump_control_register(struct emu *emu, unsigned n)
   {
      printf("              CTRL: ");
   print_control_reg(n);
   printf(": ");
   if (BITSET_TEST(emu->control_regs.written, n)) {
         } else {
            }
      static void
   dump_gpumem(struct emu *emu, uintptr_t addr)
   {
               printf("              MEM:  0x%016"PRIxPTR": ", addr);
   if (addr == emu->gpumem_written) {
         } else {
            }
      static void
   emu_write_gpr_prompt(struct emu *emu)
   {
      clear_line();
            const char *name;
            if (read_two_values(&name, &value))
            unsigned offset = afuc_gpr_reg(name);
               }
      static void
   emu_write_control_prompt(struct emu *emu)
   {
      clear_line();
            const char *name;
            if (read_two_values(&name, &value))
            unsigned offset = afuc_control_reg(name);
               }
      static void
   emu_dump_control_prompt(struct emu *emu)
   {
      clear_line();
                     if (read_one_value(&name))
                     unsigned offset = afuc_control_reg(name);
      }
      static void
   emu_write_gpu_prompt(struct emu *emu)
   {
      clear_line();
            const char *name;
            if (read_two_values(&name, &value))
            unsigned offset = afuc_gpu_reg(name);
               }
      static void
   emu_dump_gpu_prompt(struct emu *emu)
   {
      clear_line();
                     if (read_one_value(&name))
                     unsigned offset = afuc_gpu_reg(name);
      }
      static void
   emu_write_mem_prompt(struct emu *emu)
   {
      clear_line();
            const char *offset;
            if (read_two_values(&offset, &value))
            uintptr_t addr = strtoull(offset, NULL, 0);
               }
      static void
   emu_dump_mem_prompt(struct emu *emu)
   {
      clear_line();
                     if (read_one_value(&offset))
                     uintptr_t addr = strtoull(offset, NULL, 0);
      }
      static void
   emu_dump_prompt(struct emu *emu)
   {
      do {
      clear_line();
            int c = readchar();
            if (c == 'r') {
      /* Since there aren't too many GPR registers, just dump
   * them all:
   */
   dump_gpr_registers(emu);
      } else if (c == 'c') {
      emu_dump_control_prompt(emu);
      } else if (c == 'g') {
      emu_dump_gpu_prompt(emu);
      } else if (c == 'm') {
      emu_dump_mem_prompt(emu);
      } else {
      printf("invalid option: '%c'\n", c);
            }
      static void
   emu_write_prompt(struct emu *emu)
   {
      do {
      clear_line();
            int c = readchar();
            if (c == 'r') {
      emu_write_gpr_prompt(emu);
      } else if (c == 'c') {
      emu_write_control_prompt(emu);
      } else if (c == 'g') {
      emu_write_gpu_prompt(emu);
      } else if (c == 'm') {
      emu_write_mem_prompt(emu);
      } else {
      printf("invalid option: '%c'\n", c);
            }
      static void
   emu_packet_prompt(struct emu *emu)
   {
      clear_line();
   printf("  Enter packet (opc or register name), followed by payload: ");
            char *p;
   if (readline(&p) < 0)
                              /* Read the payload, so we can know the size to generate correct header: */
   uint32_t payload[0x7f];
            do {
      const char *val = extract_string(&p);
   if (!val)
            assert(cnt < ARRAY_SIZE(payload));
               uint32_t hdr;
   if (afuc_pm4_id(name) >= 0) {
      unsigned opcode = afuc_pm4_id(name);
      } else {
      unsigned regindx = afuc_gpu_reg(name);
               ASSERTED bool ret = emu_queue_push(&emu->roq, hdr);
            for (unsigned i = 0; i < cnt; i++) {
      ASSERTED bool ret = emu_queue_push(&emu->roq, payload[i]);
         }
      void
   emu_main_prompt(struct emu *emu)
   {
      if (emu->run_mode)
            do {
      clear_line();
                              if (c == 's') {
         } else if (c == 'r') {
      emu->run_mode = true;
      } else if (c == 'd') {
         } else if (c == 'w') {
         } else if (c == 'p') {
         } else if (c == 'h') {
      printf("  (s)tep   - single step to next instruction\n");
   printf("  (r)un    - run until next waitin\n");
   printf("  (d)ump   - dump memory/register menu\n");
   printf("  (w)rite  - write memory/register menu\n");
   printf("  (p)acket - inject a pm4 packet\n");
   printf("  (h)elp   - show this usage message\n");
      } else if (c == 'q') {
      printf("\n");
      } else {
               }
      void
   emu_clear_state_change(struct emu *emu)
   {
      memset(emu->control_regs.written, 0, sizeof(emu->control_regs.written));
   memset(emu->pipe_regs.written,    0, sizeof(emu->pipe_regs.written));
   memset(emu->gpu_regs.written,     0, sizeof(emu->gpu_regs.written));
   memset(emu->gpr_regs.written,     0, sizeof(emu->gpr_regs.written));
      }
      void
   emu_dump_state_change(struct emu *emu)
   {
               if (emu->quiet)
            /* Print the GPRs that changed: */
   BITSET_FOREACH_SET (i, emu->gpr_regs.written, EMU_NUM_GPR_REGS) {
                  BITSET_FOREACH_SET (i, emu->gpu_regs.written, EMU_NUM_GPU_REGS) {
                  BITSET_FOREACH_SET (i, emu->pipe_regs.written, EMU_NUM_PIPE_REGS) {
                  BITSET_FOREACH_SET (i, emu->control_regs.written, EMU_NUM_CONTROL_REGS) {
                  if (emu->gpumem_written != ~0) {
            }
