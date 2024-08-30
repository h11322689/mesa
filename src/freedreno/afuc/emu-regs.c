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
   #include <stdio.h>
   #include <stdlib.h>
      #include "emu.h"
   #include "util.h"
      /*
   * Emulator Registers:
   *
   * Handles access to GPR, GPU, control, and pipe registers.
   */
      static bool
   is_draw_state_control_reg(unsigned n)
   {
      char *reg_name = afuc_control_reg_name(n);
   if (!reg_name)
         bool ret = !!strstr(reg_name, "DRAW_STATE");
   free(reg_name);
      }
      uint32_t
   emu_get_control_reg(struct emu *emu, unsigned n)
   {
      assert(n < ARRAY_SIZE(emu->control_regs.val));
   if (is_draw_state_control_reg(n))
            }
      void
   emu_set_control_reg(struct emu *emu, unsigned n, uint32_t val)
   {
      EMU_CONTROL_REG(PACKET_TABLE_WRITE);
   EMU_CONTROL_REG(PACKET_TABLE_WRITE_ADDR);
   EMU_CONTROL_REG(REG_WRITE);
   EMU_CONTROL_REG(REG_WRITE_ADDR);
   EMU_CONTROL_REG(BV_CNTL);
   EMU_CONTROL_REG(LPAC_CNTL);
            assert(n < ARRAY_SIZE(emu->control_regs.val));
   BITSET_SET(emu->control_regs.written, n);
            /* Some control regs have special action on write: */
   if (n == emu_reg_offset(&PACKET_TABLE_WRITE)) {
               assert(write_addr < ARRAY_SIZE(emu->jmptbl));
               } else if (n == emu_reg_offset(&REG_WRITE)) {
               /* Upper bits seem like some flags, not part of the actual
   * register offset.. not sure what they mean yet:
   */
   uint32_t flags = write_addr >> 16;
            emu_set_gpu_reg(emu, write_addr++, val);
      } else if (gpuver >= 7 && n == emu_reg_offset(&BV_CNTL)) {
      /* This is sort-of a hack, but emulate what the BV bootstrap routine
   * does so that the main bootstrap routine doesn't get stuck.
   */
   emu_set_reg32(emu, &THREAD_SYNC,
      } else if (gpuver >= 7 && n == emu_reg_offset(&LPAC_CNTL)) {
      /* This is sort-of a hack, but emulate what the LPAC bootstrap routine
   * does so that the main bootstrap routine doesn't get stuck.
   */
   emu_set_reg32(emu, &THREAD_SYNC,
      } else if (is_draw_state_control_reg(n)) {
            }
      static uint32_t
   emu_get_pipe_reg(struct emu *emu, unsigned n)
   {
      assert(n < ARRAY_SIZE(emu->pipe_regs.val));
      }
      static void
   emu_set_pipe_reg(struct emu *emu, unsigned n, uint32_t val)
   {
      EMU_PIPE_REG(NRT_DATA);
            assert(n < ARRAY_SIZE(emu->pipe_regs.val));
   BITSET_SET(emu->pipe_regs.written, n);
            /* Some pipe regs have special action on write: */
   if (n == emu_reg_offset(&NRT_DATA)) {
                              }
      static uint32_t
   emu_get_gpu_reg(struct emu *emu, unsigned n)
   {
      if (n >= ARRAY_SIZE(emu->gpu_regs.val))
         assert(n < ARRAY_SIZE(emu->gpu_regs.val));
      }
      void
   emu_set_gpu_reg(struct emu *emu, unsigned n, uint32_t val)
   {
      if (n >= ARRAY_SIZE(emu->gpu_regs.val))
         assert(n < ARRAY_SIZE(emu->gpu_regs.val));
   BITSET_SET(emu->gpu_regs.written, n);
      }
      static bool
   is_pipe_reg_addr(unsigned regoff)
   {
         }
      static unsigned
   get_reg_addr(struct emu *emu)
   {
      switch (emu->data_mode) {
   case DATA_PIPE:
   case DATA_ADDR:    return REG_ADDR;
   case DATA_USRADDR: return REG_USRADDR;
   default:
      unreachable("bad data_mode");
         }
      /* Handle reads for special streaming regs: */
   static uint32_t
   emu_get_fifo_reg(struct emu *emu, unsigned n)
   {
      /* TODO the fifo regs are slurping out of a FIFO that the hw is filling
   * in parallel.. we can use `struct emu_queue` to emulate what is actually
   * happening more accurately
            if (n == REG_MEMDATA) {
      /* $memdata */
   EMU_CONTROL_REG(MEM_READ_DWORDS);
            unsigned  read_dwords = emu_get_reg32(emu, &MEM_READ_DWORDS);
            if (read_dwords > 0) {
      emu_set_reg32(emu, &MEM_READ_DWORDS, read_dwords - 1);
                  } else if (n == REG_REGDATA) {
      /* $regdata */
   EMU_CONTROL_REG(REG_READ_DWORDS);
            unsigned read_dwords = emu_get_reg32(emu, &REG_READ_DWORDS);
            /* I think if the fw doesn't write REG_READ_DWORDS before
   * REG_READ_ADDR, it just ends up with a single value written
   * into the FIFO that $regdata is consuming from:
   */
   if (read_dwords > 0) {
      emu_set_reg32(emu, &REG_READ_DWORDS, read_dwords - 1);
                  } else if (n == REG_DATA) {
      /* $data */
   do {
                     uint32_t val;
   if (emu_queue_pop(&emu->roq, &val)) {
      emu_set_gpr_reg(emu, REG_REM, --rem);
               /* If FIFO is empty, prompt for more input: */
   printf("FIFO empty, input a packet!\n");
   emu->run_mode = false;
         } else {
      unreachable("not a FIFO reg");
         }
      static void
   emu_set_fifo_reg(struct emu *emu, unsigned n, uint32_t val)
   {
      if ((n == REG_ADDR) || (n == REG_USRADDR)) {
               /* Treat these as normal register writes so we can see
   * updated values in the output as we step thru the
   * instructions:
   */
   emu->gpr_regs.val[n] = val;
            if (is_pipe_reg_addr(val)) {
      /* "void" pipe regs don't have a value to write, so just
   * treat it as writing zero to the pipe reg:
   */
   if (afuc_pipe_reg_is_void(val >> 24))
               } else if (n == REG_DATA) {
      unsigned reg = get_reg_addr(emu);
   unsigned regoff = emu->gpr_regs.val[reg];
   if (is_pipe_reg_addr(regoff)) {
                        /* If b18 is set, don't auto-increment dest addr.. and if we
   * do auto-increment, we only increment the high 8b
   *
   * Note that we bypass emu_set_gpr_reg() in this case because
   * auto-incrementing isn't triggering a write to "void" pipe
   * regs.
   */
   if (!(regoff & 0x40000)) {
      emu->gpr_regs.val[reg] = regoff + 0x01000000;
                  } else {
      /* writes to gpu registers: */
   emu_set_gpr_reg(emu, reg, regoff+1);
            }
      uint32_t
   emu_get_gpr_reg(struct emu *emu, unsigned n)
   {
               /* Handle special regs: */
   switch (n) {
   case 0x00:
         case REG_MEMDATA:
   case REG_REGDATA:
   case REG_DATA:
         default:
            }
      void
   emu_set_gpr_reg(struct emu *emu, unsigned n, uint32_t val)
   {
               switch (n) {
   case REG_ADDR:
   case REG_USRADDR:
   case REG_DATA:
      emu_set_fifo_reg(emu, n, val);
      default:
      emu->gpr_regs.val[n] = val;
   BITSET_SET(emu->gpr_regs.written, n);
         }
      /*
   * Control/pipe register accessor helpers:
   */
      struct emu_reg_accessor {
      unsigned (*get_offset)(const char *name);
   uint32_t (*get)(struct emu *emu, unsigned n);
      };
      const struct emu_reg_accessor emu_control_accessor = {
         .get_offset = afuc_control_reg,
   .get = emu_get_control_reg,
   };
      const struct emu_reg_accessor emu_pipe_accessor = {
         .get_offset = afuc_pipe_reg,
   .get = emu_get_pipe_reg,
   };
      const struct emu_reg_accessor emu_gpu_accessor = {
         .get_offset = afuc_gpu_reg,
   .get = emu_get_gpu_reg,
   };
      unsigned
   emu_reg_offset(struct emu_reg *reg)
   {
      if (reg->offset == ~0)
            }
      uint32_t
   emu_get_reg32(struct emu *emu, struct emu_reg *reg)
   {
         }
      uint64_t
   emu_get_reg64(struct emu *emu, struct emu_reg *reg)
   {
      uint64_t val = reg->accessor->get(emu, emu_reg_offset(reg) + 1);
   val <<= 32;
   val |= reg->accessor->get(emu, emu_reg_offset(reg));
      }
      void
   emu_set_reg32(struct emu *emu, struct emu_reg *reg, uint32_t val)
   {
         }
      void
   emu_set_reg64(struct emu *emu, struct emu_reg *reg, uint64_t val)
   {
      reg->accessor->set(emu, emu_reg_offset(reg),     val);
      }
