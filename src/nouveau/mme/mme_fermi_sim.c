   /*
   * Copyright © 2022 Mary Guillemard
   * SPDX-License-Identifier: MIT
   */
   #include "mme_fermi_sim.h"
      #include <inttypes.h>
      #include "mme_fermi.h"
   #include "util/u_math.h"
      #include "nvk_cl9097.h"
   #include "nvk_cl902d.h"
      struct mme_fermi_sim {
      uint32_t param_count;
            /* Bound memory ranges */
   uint32_t mem_count;
            /* SET_MME_MEM_ADDRESS_A/B */
            /* RAM, accessed by STATE */
   struct {
               /* SET_MME_MEM_RAM_ADDRESS */
               struct {
      // TODO: check if read_fifo is supported.
   struct {
      uint32_t data[1024];
                  struct {
      unsigned mthd:16;
   unsigned inc:4;
               /* SET_MME_SHADOW_SCRATCH(i) */
            uint32_t regs[7];
   uint32_t alu_carry;
   uint16_t ip;
      };
      static uint32_t *
   find_mem(struct mme_fermi_sim *sim, uint64_t addr, const char *op_desc)
   {
      for (uint32_t i = 0; i < sim->mem_count; i++) {
      if (addr < sim->mems[i].addr)
            uint64_t offset = addr - sim->mems[i].addr;
   if (offset >= sim->mems[i].size)
            assert(sim->mems[i].data != NULL);
               fprintf(stderr, "FAULT in %s at address 0x%"PRIx64"\n", op_desc, addr);
      }
      static uint32_t
   read_dmem(struct mme_fermi_sim *sim, uint64_t addr, const char *op_desc)
   {
               if (ram_index < ARRAY_SIZE(sim->ram.data)) {
                  if (addr >= NV9097_SET_MME_SHADOW_SCRATCH(0) && addr < NV9097_CALL_MME_MACRO(0)) {
                  fprintf(stderr, "FAULT in %s at DMEM address 0x%"PRIx64"\n (READ)", op_desc, addr);
      }
      static void
   write_dmem(struct mme_fermi_sim *sim, uint64_t addr, uint32_t val, const char *op_desc)
   {
               if (ram_index < ARRAY_SIZE(sim->ram.data)) {
         }
   else if (addr >= NV9097_SET_MME_SHADOW_SCRATCH(0) && addr < NV9097_CALL_MME_MACRO(0)) {
         } else {
      fprintf(stderr, "FAULT in %s at DMEM address 0x%"PRIx64" (WRITE)\n", op_desc, addr);
         }
      static uint64_t
   read_dmem64(struct mme_fermi_sim *sim, uint64_t addr, const char *op_desc)
   {
         }
      static uint32_t load_param(struct mme_fermi_sim *sim)
   {
      if (sim->param_count == 0) {
      // TODO: know what happens on hardware here
                        sim->params++;
               }
      static uint32_t load_reg(struct mme_fermi_sim *sim, enum mme_fermi_reg reg)
   {
      if (reg == MME_FERMI_REG_ZERO) {
                     }
      static void store_reg(struct mme_fermi_sim *sim, enum mme_fermi_reg reg, uint32_t val)
   {
      if (reg == MME_FERMI_REG_ZERO) {
                     }
      static int32_t load_imm(const struct mme_fermi_inst *inst)
   {
         }
      static uint32_t eval_bfe_lsl(uint32_t value, uint32_t src_bit, uint32_t dst_bit, uint8_t size)
   {
      if (dst_bit > 31 || src_bit > 31) {
                     }
      static uint32_t eval_op(struct mme_fermi_sim *sim, const struct mme_fermi_inst *inst) {
               uint32_t x = load_reg(sim, inst->src[0]);
            switch (inst->op) {
      case MME_FERMI_OP_ALU_REG: {
               switch (inst->alu_op) {
      case MME_FERMI_ALU_OP_ADD:
      res = x + y;
   sim->alu_carry = res < x;
      case MME_FERMI_ALU_OP_ADDC:
      res = x + y + sim->alu_carry;
   sim->alu_carry = res < x;
      case MME_FERMI_ALU_OP_SUB:
      res = x - y;
   sim->alu_carry = res > x;
      case MME_FERMI_ALU_OP_SUBB:
      res = x - y - sim->alu_carry;
   sim->alu_carry = res > x;
      case MME_FERMI_ALU_OP_XOR:
      res = x ^ y;
      case MME_FERMI_ALU_OP_OR:
      res = x | y;
      case MME_FERMI_ALU_OP_AND:
      res = x & y;
      case MME_FERMI_ALU_OP_AND_NOT:
      res = x & ~y;
      case MME_FERMI_ALU_OP_NAND:
      res = ~(x & y);
      default:
                  }
   case MME_FERMI_OP_ADD_IMM:
         case MME_FERMI_OP_MERGE:
         case MME_FERMI_OP_BFE_LSL_IMM:
         case MME_FERMI_OP_BFE_LSL_REG:
         case MME_FERMI_OP_STATE:
         // TODO: reverse MME_FERMI_OP_UNK6
   default:
         }
      static void
   set_mthd(struct mme_fermi_sim *sim, uint32_t val)
   {
      sim->mthd.mthd = (val & 0xfff) << 2;
   sim->mthd.inc = (val >> 12) & 0xf;
      }
      static void
   emit_mthd(struct mme_fermi_sim *sim, uint32_t val)
   {
      // TODO: understand what happens on hardware when no mthd has been set.
   if (!sim->mthd.has_mthd)
                              switch (mthd) {
   case NV9097_SET_REPORT_SEMAPHORE_D: {
               uint64_t addr = read_dmem64(sim, NV9097_SET_REPORT_SEMAPHORE_A, "SET_REPORT_SEMAPHORE");
            uint32_t *mem = find_mem(sim, addr, "SET_REPORT_SEMAPHORE");
   *mem = data;
      }
   case NV902D_SET_MME_DATA_RAM_ADDRESS:
      sim->ram.addr = val;
      case NV902D_SET_MME_MEM_ADDRESS_B:
      sim->mem_addr = read_dmem64(sim, NV902D_SET_MME_MEM_ADDRESS_A, "SET_MME_MEM_ADDRESS");
      case NV902D_MME_DMA_READ_FIFOED:
      sim->dma.read_fifo.count = val;
      default:
                     }
      static void
   eval_inst(struct mme_fermi_sim *sim, const struct mme_fermi_inst *inst)
   {
      if (inst->op == MME_FERMI_OP_BRANCH) {
      uint32_t val = load_reg(sim, inst->src[0]);
            if (cond) {
      int32_t offset = load_imm(inst);
   assert((int)sim->ip + offset >= 0);
   assert((int)sim->ip + offset < 0x1000);
         } else {
      uint32_t scratch = eval_op(sim, inst);
   switch (inst->assign_op) {
      case MME_FERMI_ASSIGN_OP_LOAD:
      store_reg(sim, inst->dst, load_param(sim));
      case MME_FERMI_ASSIGN_OP_MOVE:
      store_reg(sim, inst->dst, scratch);
      case MME_FERMI_ASSIGN_OP_MOVE_SET_MADDR:
      store_reg(sim, inst->dst, scratch);
   set_mthd(sim, scratch);
      case MME_FERMI_ASSIGN_OP_LOAD_EMIT:
      store_reg(sim, inst->dst, load_param(sim));
   emit_mthd(sim, scratch);
      case MME_FERMI_ASSIGN_OP_MOVE_EMIT:
      store_reg(sim, inst->dst, scratch);
   emit_mthd(sim, scratch);
      case MME_FERMI_ASSIGN_OP_LOAD_SET_MADDR:
      store_reg(sim, inst->dst, scratch);
   set_mthd(sim, scratch);
      case MME_FERMI_ASSIGN_OP_MOVE_SET_MADDR_LOAD_EMIT:
      store_reg(sim, inst->dst, scratch);
   set_mthd(sim, scratch);
   emit_mthd(sim, load_param(sim));
      case MME_FERMI_ASSIGN_OP_MOVE_SET_MADDR_LOAD_EMIT_HIGH:
      store_reg(sim, inst->dst, scratch);
   set_mthd(sim, scratch);
   emit_mthd(sim, (scratch >> 12) & 0x3f);
      default:
            }
      void mme_fermi_sim(uint32_t inst_count, const struct mme_fermi_inst *insts,
               {
      struct mme_fermi_sim sim = {
      .param_count = param_count,
   .params = params,
   .mem_count = mem_count,
               sim.ip = 0;
   /* First preload first argument in R1*/
            bool end_next = false;
   bool ignore_next_exit = false;
            while (!end_next) {
      assert(sim.ip < inst_count);
            if (!should_delay_branch) {
                                    if (should_delay_branch) {
         } else {
                  if (inst->end_next && should_delay_branch) {
      ignore_next_exit = true;
               end_next = inst->end_next && !ignore_next_exit;
               // Handle delay slot at exit
   assert(sim.ip < inst_count);
      }
