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
   #include <errno.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <sys/mman.h>
   #include <unistd.h>
      #include "util/u_math.h"
      #include "freedreno_pm4.h"
      #include "isaspec.h"
      #include "emu.h"
   #include "util.h"
      #define rotl32(x,r) (((x) << (r)) | ((x) >> (32 - (r))))
   #define rotl64(x,r) (((x) << (r)) | ((x) >> (64 - (r))))
      /**
   * AFUC emulator.  Currently only supports a6xx
   *
   * TODO to add a5xx it might be easier to compile this multiple times
   * with conditional compile to deal with differences between generations.
   */
      static uint32_t
   emu_alu(struct emu *emu, afuc_opc opc, uint32_t src1, uint32_t src2)
   {
      uint64_t tmp;
   switch (opc) {
   case OPC_ADD:
      tmp = (uint64_t)src1 + (uint64_t)src2;
   emu->carry = tmp >> 32;
      case OPC_ADDHI:
         case OPC_SUB:
      tmp = (uint64_t)src1 - (uint64_t)src2;
   emu->carry = tmp >> 32;
      case OPC_SUBHI:
         case OPC_AND:
         case OPC_OR:
         case OPC_XOR:
         case OPC_NOT:
         case OPC_SHL:
         case OPC_USHR:
         case OPC_ISHR:
         case OPC_ROT:
      if (src2 & 0x80000000)
         else
      case OPC_MUL8:
         case OPC_MIN:
         case OPC_MAX:
         case OPC_CMP:
      if (src1 > src2)
         else if (src1 == src2)
            case OPC_BIC:
         case OPC_MSB:
      if (!src2)
            case OPC_SETBIT: {
      unsigned bit = src2 >> 1;
   unsigned val = src2 & 1;
      }
   default:
      printf("unhandled alu opc: 0x%02x\n", opc);
         }
      /**
   * Helper to calculate load/store address based on LOAD_STORE_HI
   */
   static uintptr_t
   load_store_addr(struct emu *emu, unsigned gpr)
   {
               uintptr_t addr = emu_get_reg32(emu, &LOAD_STORE_HI);
               }
      static void
   emu_instr(struct emu *emu, struct afuc_instr *instr)
   {
               switch (instr->opc) {
   case OPC_NOP:
         case OPC_MSB:
   case OPC_ADD ... OPC_BIC: {
      uint32_t val = emu_alu(emu, instr->opc,
                              if (instr->xmov) {
                        if (m == 1) {
      emu_set_gpr_reg(emu, REG_REM, --rem);
   emu_dump_state_change(emu);
   emu_set_gpr_reg(emu, REG_DATA,
      } else if (m == 2) {
      emu_set_gpr_reg(emu, REG_REM, --rem);
   emu_dump_state_change(emu);
   emu_set_gpr_reg(emu, REG_DATA,
         emu_set_gpr_reg(emu, REG_REM, --rem);
   emu_dump_state_change(emu);
   emu_set_gpr_reg(emu, REG_DATA,
      } else if (m == 3) {
      emu_set_gpr_reg(emu, REG_REM, --rem);
   emu_dump_state_change(emu);
   emu_set_gpr_reg(emu, REG_DATA,
         emu_set_gpr_reg(emu, REG_REM, --rem);
   emu_dump_state_change(emu);
   emu_set_gpr_reg(emu, instr->dst,
         emu_set_gpr_reg(emu, REG_REM, --rem);
   emu_dump_state_change(emu);
   emu_set_gpr_reg(emu, REG_DATA,
         }
      }
   case OPC_MOVI: {
      uint32_t val = instr->immed << instr->shift;
   emu_set_gpr_reg(emu, instr->dst, val);
      }
   case OPC_SETBITI: {
      uint32_t src = emu_get_gpr_reg(emu, instr->src1);
   emu_set_gpr_reg(emu, instr->dst, src | (1u << instr->bit));
      }
   case OPC_CLRBIT: {
      uint32_t src = emu_get_gpr_reg(emu, instr->src1);
   emu_set_gpr_reg(emu, instr->dst, src & ~(1u << instr->bit));
      }
   case OPC_UBFX: {
      uint32_t src = emu_get_gpr_reg(emu, instr->src1);
   unsigned lo = instr->bit, hi = instr->immed;
   uint32_t dst = (src >> lo) & BITFIELD_MASK(hi - lo + 1);
   emu_set_gpr_reg(emu, instr->dst, dst);
      }
   case OPC_BFI: {
      uint32_t src = emu_get_gpr_reg(emu, instr->src1);
   unsigned lo = instr->bit, hi = instr->immed;
   src = (src & BITFIELD_MASK(hi - lo + 1)) << lo;
   emu_set_gpr_reg(emu, instr->dst, emu_get_gpr_reg(emu, instr->dst) | src);
      }
   case OPC_CWRITE: {
      uint32_t src1 = emu_get_gpr_reg(emu, instr->src1);
            if (instr->bit == 0x4) {
         } else if (instr->bit && !emu->quiet) {
                  emu_set_control_reg(emu, src2 + instr->immed, src1);
      }
   case OPC_CREAD: {
               if (instr->bit == 0x4) {
         } else if (instr->bit && !emu->quiet) {
                  emu_set_gpr_reg(emu, instr->dst,
            }
   case OPC_LOAD: {
      uintptr_t addr = load_store_addr(emu, instr->src1) +
            if (instr->bit == 0x4) {
      uint32_t src1 = emu_get_gpr_reg(emu, instr->src1);
      } else if (instr->bit && !emu->quiet) {
                                       }
   case OPC_STORE: {
      uintptr_t addr = load_store_addr(emu, instr->src2) +
            if (instr->bit == 0x4) {
      uint32_t src2 = emu_get_gpr_reg(emu, instr->src2);
      } else if (instr->bit && !emu->quiet) {
                                       }
   case OPC_BRNEI ... OPC_BREQB: {
      uint32_t off = emu->gpr_regs.pc + instr->offset;
            if (instr->opc == OPC_BRNEI) {
      if (src != instr->immed)
      } else if (instr->opc == OPC_BREQI) {
      if (src == instr->immed)
      } else if (instr->opc == OPC_BRNEB) {
      if (!(src & (1 << instr->bit)))
      } else if (instr->opc == OPC_BREQB) {
      if (src & (1 << instr->bit))
      } else {
         }
      }
   case OPC_RET: {
               /* counter-part to 'call' instruction, also has a delay slot: */
               }
   case OPC_CALL: {
               /* call looks to have same delay-slot behavior as branch/etc, so
   * presumably the return PC is two instructions later:
   */
   emu->call_stack[emu->call_stack_idx++] = emu->gpr_regs.pc + 2;
               }
   case OPC_WAITIN: {
      assert(!emu->branch_target);
   emu->run_mode = false;
   emu->waitin = true;
      }
   /* OPC_PREEMPTLEAVE6 */
   case OPC_SETSECURE: {
      // TODO this acts like a conditional branch, but in which case
   // does it branch?
      }
   default:
      printf("unhandled opc: 0x%02x\n", instr->opc);
               if (instr->rep) {
      assert(rem > 0);
         }
      void
   emu_step(struct emu *emu)
   {
      struct afuc_instr *instr;
   bool decoded = isa_decode((void *)&instr,
                              if (!decoded) {
      uint32_t instr_val = emu->instrs[emu->gpr_regs.pc];
   if ((instr_val >> 27) == 0) {
      /* This is printed as an undecoded literal to show the immediate
   * payload, but when executing it's just a NOP.
   */
   instr = calloc(1, sizeof(struct afuc_instr));
      } else {
      printf("unmatched instruction: 0x%08x\n", instr_val);
                           uint32_t branch_target = emu->branch_target;
            bool waitin = emu->waitin;
            if (instr->rep) {
      do {
                                    /* defer last state-change dump until after any
   * post-delay-slot handling below:
   */
   if (emu_get_gpr_reg(emu, REG_REM))
         } else {
      emu_clear_state_change(emu);
                        if (branch_target) {
                  if (waitin) {
      uint32_t hdr = emu_get_gpr_reg(emu, 1);
            if (pkt_is_type4(hdr)) {
                     /* Possibly a hack, not sure what the hw actually
   * does here, but we want to mask out the pkt
   * type field from the hdr, so that PKT4 handler
   * doesn't see it and interpret it as part as the
   * register offset:
   */
      } else if (pkt_is_type7(hdr)) {
      id = cp_type7_opcode(hdr);
      } else {
      printf("Invalid opcode: 0x%08x\n", hdr);
                        emu_set_gpr_reg(emu, REG_REM, count);
                           }
      void
   emu_run_bootstrap(struct emu *emu)
   {
               emu->quiet = true;
            while (emu_get_reg32(emu, &PACKET_TABLE_WRITE_ADDR) < 0x80) {
            }
         static void
   check_access(struct emu *emu, uintptr_t gpuaddr, unsigned sz)
   {
      if ((gpuaddr % sz) != 0) {
      printf("unaligned access fault: %p\n", (void *)gpuaddr);
               if ((gpuaddr + sz) >= EMU_MEMORY_SIZE) {
      printf("iova fault: %p\n", (void *)gpuaddr);
         }
      uint32_t
   emu_mem_read_dword(struct emu *emu, uintptr_t gpuaddr)
   {
      check_access(emu, gpuaddr, 4);
      }
      static void
   mem_write_dword(struct emu *emu, uintptr_t gpuaddr, uint32_t val)
   {
      check_access(emu, gpuaddr, 4);
      }
      void
   emu_mem_write_dword(struct emu *emu, uintptr_t gpuaddr, uint32_t val)
   {
      mem_write_dword(emu, gpuaddr, val);
   assert(emu->gpumem_written == ~0);
      }
      void
   emu_init(struct emu *emu)
   {
      emu->gpumem = mmap(NULL, EMU_MEMORY_SIZE,
                     if (emu->gpumem == MAP_FAILED) {
      printf("Could not allocate GPU memory: %s\n", strerror(errno));
               /* Copy the instructions into GPU memory: */
   for (unsigned i = 0; i < emu->sizedwords; i++) {
                  EMU_GPU_REG(CP_SQE_INSTR_BASE);
   EMU_GPU_REG(CP_LPAC_SQE_INSTR_BASE);
   EMU_CONTROL_REG(BV_INSTR_BASE);
            /* Setup the address of the SQE fw, just use the normal CPU ptr address: */
   switch (emu->processor) {
   case EMU_PROC_SQE:
      emu_set_reg64(emu, &CP_SQE_INSTR_BASE, EMU_INSTR_BASE);
      case EMU_PROC_BV:
      emu_set_reg64(emu, &BV_INSTR_BASE, EMU_INSTR_BASE);
      case EMU_PROC_LPAC:
      if (gpuver >= 7)
         else
                     if (emu->gpu_id == 730) {
      emu_set_control_reg(emu, 0xef, 1 << 21);
      } else if (emu->gpu_id == 660) {
         } else if (emu->gpu_id == 650) {
            }
      void
   emu_fini(struct emu *emu)
   {
      uint32_t *instrs = emu->instrs;
   unsigned sizedwords = emu->sizedwords;
            munmap(emu->gpumem, EMU_MEMORY_SIZE);
            emu->instrs = instrs;
   emu->sizedwords = sizedwords;
      }
