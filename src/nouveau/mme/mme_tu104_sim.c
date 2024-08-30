   /*
   * Copyright © 2022 Collabora Ltd.
   * SPDX-License-Identifier: MIT
   */
   #include "mme_tu104_sim.h"
      #include <inttypes.h>
      #include "mme_tu104.h"
   #include "util/u_math.h"
      #include "nvk_clc597.h"
      struct mme_tu104_sim {
      uint32_t param_count;
                     /* Bound memory ranges */
   uint32_t mem_count;
            /* SET_MME_MEM_ADDRESS_A/B */
            /* RAM, accessed by DREAD/DWRITE */
   struct {
               /* SET_MME_MEM_RAM_ADDRESS */
               struct {
      struct {
      uint32_t data[1024];
                  /* NVC597_SET_MME_SHADOW_SCRATCH(i) */
            struct {
      unsigned mthd:16;
   unsigned inc:4;
   bool has_mthd:1;
   unsigned _pad:5;
   unsigned data_len:8;
               uint32_t set_regs;
   uint32_t regs[23];
   uint32_t alu_res[2];
            uint16_t ip;
   uint16_t next_ip;
            uint32_t loop_count;
   uint16_t loop_start;
      };
      static bool
   inst_loads_reg(const struct mme_tu104_inst *inst,
         {
      return inst->pred == reg ||
         inst->alu[0].src[0] == reg ||
   inst->alu[0].src[1] == reg ||
      }
      static bool
   inst_loads_out(const struct mme_tu104_inst *inst,
         {
      return inst->out[0].mthd == out ||
         inst->out[0].emit == out ||
      }
      static void
   load_params(struct mme_tu104_sim *sim,
         {
      const bool has_load0 = inst_loads_reg(inst, MME_TU104_REG_LOAD0) ||
         const bool has_load1 = inst_loads_reg(inst, MME_TU104_REG_LOAD1) ||
                  if (has_load0) {
      sim->load[0] = *sim->params;
   sim->params++;
               if (has_load1) {
      sim->load[1] = *sim->params;
   sim->params++;
         }
      static uint32_t
   load_state(struct mme_tu104_sim *sim, uint16_t state)
   {
               if (NVC597_SET_MME_SHADOW_SCRATCH(0) <= state &&
      state < NVC597_CALL_MME_MACRO(0)) {
   uint32_t i = (state - NVC597_SET_MME_SHADOW_SCRATCH(0)) / 4;
   assert(i <= ARRAY_SIZE(sim->scratch));
                  }
      static uint32_t *
   find_mem(struct mme_tu104_sim *sim, uint64_t addr, const char *op_desc)
   {
      for (uint32_t i = 0; i < sim->mem_count; i++) {
      if (addr < sim->mems[i].addr)
            uint64_t offset = addr - sim->mems[i].addr;
   if (offset >= sim->mems[i].size)
            assert(sim->mems[i].data != NULL);
               fprintf(stderr, "FAULT in %s at address 0x%"PRIx64"\n", op_desc, addr);
      }
      static void
   finish_dma_read_fifo(struct mme_tu104_sim *sim)
   {
      if (sim->dma.read_fifo.count == 0)
            for (uint32_t i = 0; i < sim->dma.read_fifo.count; i++) {
      uint32_t *src = find_mem(sim, sim->mem_addr + i * 4,
         assert(src != NULL);
               sim->param_count = sim->dma.read_fifo.count;
      }
      static void
   flush_mthd(struct mme_tu104_sim *sim)
   {
      if (!sim->mthd.has_mthd)
            uint16_t mthd = sim->mthd.mthd;
   const uint32_t *p = sim->mthd.data;
   const uint32_t *end = sim->mthd.data + sim->mthd.data_len;
   while (p < end) {
      uint32_t dw_used = 1;
   if (NVC597_SET_MME_SHADOW_SCRATCH(0) <= mthd &&
      mthd < NVC597_CALL_MME_MACRO(0)) {
   uint32_t i = (mthd - NVC597_SET_MME_SHADOW_SCRATCH(0)) / 4;
   assert(i <= ARRAY_SIZE(sim->scratch));
      } else {
      switch (mthd) {
   case NVC597_SET_REPORT_SEMAPHORE_A: {
      assert(p + 4 <= end);
   uint64_t addr = ((uint64_t)p[0] << 32) | p[1];
   uint32_t data = p[2];
                  uint32_t *mem = find_mem(sim, addr, "SET_REPORT_SEMAPHORE");
   *mem = data;
      }
   case NVC597_SET_MME_DATA_RAM_ADDRESS:
      sim->ram.addr = *p;
      case NVC597_SET_MME_MEM_ADDRESS_A:
      assert(p + 2 <= end);
   sim->mem_addr = ((uint64_t)p[0] << 32) | p[1];
   dw_used = 2;
      case NVC597_MME_DMA_READ_FIFOED:
      sim->dma.read_fifo.count = *p;
      default:
      fprintf(stdout, "%s:\n", P_PARSE_NVC597_MTHD(mthd));
   P_DUMP_NVC597_MTHD_DATA(stdout, mthd, *p, "    ");
                  p += dw_used;
   assert(sim->mthd.inc == 1);
                  }
      static void
   eval_extended(struct mme_tu104_sim *sim,
         {
      /* The only extended method we know about appears to be some sort of
   * barrier required when using READ_FIFOED.
   */
   assert(x == 0x1000);
   assert(y == 1);
   flush_mthd(sim);
      }
      static uint32_t
   load_reg(struct mme_tu104_sim *sim,
               {
      if (reg <= MME_TU104_REG_R23) {
      assert(sim->set_regs & BITFIELD_BIT(reg));
               switch (reg) {
   case MME_TU104_REG_ZERO:
         case MME_TU104_REG_IMM:
      assert(imm_idx < 2);
   /* Immediates are treated as signed for ALU ops */
      case MME_TU104_REG_IMMPAIR:
      assert(imm_idx < 2);
   /* Immediates are treated as signed for ALU ops */
      case MME_TU104_REG_IMM32:
         case MME_TU104_REG_LOAD0:
         case MME_TU104_REG_LOAD1:
         default:
            }
      static uint8_t
   load_pred(struct mme_tu104_sim *sim,
         {
      if (inst->pred_mode == MME_TU104_PRED_UUUU)
            uint32_t val = load_reg(sim, inst, -1, inst->pred);
            uint8_t mask = 0;
   for (unsigned i = 0; i < 4; i++) {
      if (pred[i] != (val ? 'T' : 'F'))
                  }
      static void
   store_reg(struct mme_tu104_sim *sim,
               {
      if (reg <= MME_TU104_REG_R23) {
      sim->set_regs |= BITFIELD_BIT(reg);
      } else if (reg <= MME_TU104_REG_ZERO) {
         } else {
            }
      static bool
   eval_cond(enum mme_tu104_alu_op op, uint32_t x, uint32_t y)
   {
      switch (op) {
   case MME_TU104_ALU_OP_BLT:
   case MME_TU104_ALU_OP_SLT:
         case MME_TU104_ALU_OP_BLTU:
   case MME_TU104_ALU_OP_SLTU:
         case MME_TU104_ALU_OP_BLE:
   case MME_TU104_ALU_OP_SLE:
         case MME_TU104_ALU_OP_BLEU:
   case MME_TU104_ALU_OP_SLEU:
         case MME_TU104_ALU_OP_BEQ:
   case MME_TU104_ALU_OP_SEQ:
         default:
            }
      static void
   eval_alu(struct mme_tu104_sim *sim,
               {
      const struct mme_tu104_alu *alu = &inst->alu[alu_idx];
   const uint32_t x = load_reg(sim, inst, alu_idx, alu->src[0]);
            uint32_t res = 0;
   switch (inst->alu[alu_idx].op) {
   case MME_TU104_ALU_OP_ADD:
      res = x + y;
   sim->alu_carry = res < x;
      case MME_TU104_ALU_OP_ADDC:
      assert(alu_idx == 1);
   assert(inst->alu[0].op == MME_TU104_ALU_OP_ADD);
   res = x + y + sim->alu_carry;
      case MME_TU104_ALU_OP_SUB:
      res = x - y;
   sim->alu_carry = res > x;
      case MME_TU104_ALU_OP_SUBB:
      assert(alu_idx == 1);
   assert(inst->alu[0].op == MME_TU104_ALU_OP_SUB);
   res = x - y - sim->alu_carry;
      case MME_TU104_ALU_OP_MUL: {
      /* Sign extend but use uint64_t for the multiply so that we avoid
   * undefined behavior from possible signed multiply roll-over.
   */
   const uint64_t x_u64 = (int64_t)(int32_t)x;
   const uint64_t y_u64 = (int64_t)(int32_t)y;
   const uint64_t xy_u64 = x_u64 * y_u64;
   res = xy_u64;
   sim->alu_carry = xy_u64 >> 32;
      }
   case MME_TU104_ALU_OP_MULH:
      assert(inst->alu[alu_idx].src[0] == MME_TU104_REG_ZERO);
   assert(inst->alu[alu_idx].src[1] == MME_TU104_REG_ZERO);
   res = sim->alu_carry;
      case MME_TU104_ALU_OP_MULU: {
      const uint64_t x_u64 = x;
   const uint64_t y_u64 = y;
   const uint64_t xy_u64 = x_u64 * y_u64;
   res = xy_u64;
   sim->alu_carry = xy_u64 >> 32;
      }
   case MME_TU104_ALU_OP_EXTENDED:
      eval_extended(sim, x, y);
      case MME_TU104_ALU_OP_CLZ:
      res = __builtin_clz(x);
      case MME_TU104_ALU_OP_SLL:
      res = x << (y & 31);
      case MME_TU104_ALU_OP_SRL:
      res = x >> (y & 31);
      case MME_TU104_ALU_OP_SRA:
      res = (int32_t)x >> (y & 31);
      case MME_TU104_ALU_OP_AND:
      res = x & y;
      case MME_TU104_ALU_OP_NAND:
      res = ~(x & y);
      case MME_TU104_ALU_OP_OR:
      res = x | y;
      case MME_TU104_ALU_OP_XOR:
      res = x ^ y;
      case MME_TU104_ALU_OP_MERGE: {
      uint16_t immed = inst->imm[alu_idx];
   uint32_t dst_pos  = (immed >> 10) & 0x3f;
   uint32_t bits     = (immed >> 5)  & 0x1f;
   uint32_t src_pos  = (immed >> 0)  & 0x1f;
   res = (x & ~(BITFIELD_MASK(bits) << dst_pos)) |
            }
   case MME_TU104_ALU_OP_SLT:
   case MME_TU104_ALU_OP_SLTU:
   case MME_TU104_ALU_OP_SLE:
   case MME_TU104_ALU_OP_SLEU:
   case MME_TU104_ALU_OP_SEQ:
      res = eval_cond(inst->alu[alu_idx].op, x, y) ? ~0u : 0u;
      case MME_TU104_ALU_OP_STATE:
      flush_mthd(sim);
   res = load_state(sim, (uint16_t)(x + y) * 4);
      case MME_TU104_ALU_OP_LOOP:
      assert(sim->loop_count == 0);
   assert(inst->alu[alu_idx].dst == MME_TU104_REG_ZERO);
   assert(inst->alu[alu_idx].src[1] == MME_TU104_REG_ZERO);
   sim->loop_count = MAX2(1, x) - 1;
   sim->loop_start = sim->ip;
   sim->loop_end = sim->ip + inst->imm[alu_idx] - 1;
   assert(sim->loop_end > sim->ip);
      case MME_TU104_ALU_OP_JAL: {
      assert(inst->alu[alu_idx].dst == MME_TU104_REG_ZERO);
   assert(inst->alu[alu_idx].src[0] == MME_TU104_REG_ZERO);
   assert(inst->alu[alu_idx].src[1] == MME_TU104_REG_ZERO);
   /* No idea what bit 15 does.  The NVIDIA blob always sets it. */
   assert(inst->imm[alu_idx] & BITFIELD_BIT(15));
   uint16_t offset = (inst->imm[alu_idx] & BITFIELD_MASK(15));
   sim->next_ip = sim->ip + offset;
   res = 0;
      }
   case MME_TU104_ALU_OP_BLT:
   case MME_TU104_ALU_OP_BLTU:
   case MME_TU104_ALU_OP_BLE:
   case MME_TU104_ALU_OP_BLEU:
   case MME_TU104_ALU_OP_BEQ: {
      assert(inst->alu[alu_idx].dst == MME_TU104_REG_ZERO);
   bool expect = (inst->imm[alu_idx] & BITFIELD_BIT(15)) != 0;
   if (eval_cond(inst->alu[alu_idx].op, x, y) == expect) {
      int16_t offset = util_mask_sign_extend(inst->imm[alu_idx], 13);
   if ((uint16_t)offset == 0xf000) {
      sim->stop = true;
               assert((int)sim->ip + offset >= 0);
   assert((int)sim->ip + offset < 0x1000);
      }
      }
   case MME_TU104_ALU_OP_DREAD:
      assert(inst->alu[alu_idx].src[1] == MME_TU104_REG_ZERO);
   assert(x < ARRAY_SIZE(sim->ram.data));
   res = sim->ram.data[x];
      case MME_TU104_ALU_OP_DWRITE:
      assert(inst->alu[alu_idx].dst == MME_TU104_REG_ZERO);
   assert(x < ARRAY_SIZE(sim->ram.data));
   sim->ram.data[x] = y;
      default:
                  sim->alu_res[alu_idx] = res;
      }
      static uint32_t
   load_out(struct mme_tu104_sim *sim,
               {
      switch (op) {
   case MME_TU104_OUT_OP_ALU0:
         case MME_TU104_OUT_OP_ALU1:
         case MME_TU104_OUT_OP_LOAD0:
         case MME_TU104_OUT_OP_LOAD1:
         case MME_TU104_OUT_OP_IMM0:
         case MME_TU104_OUT_OP_IMM1:
         case MME_TU104_OUT_OP_IMMHIGH0:
         case MME_TU104_OUT_OP_IMMHIGH1:
         case MME_TU104_OUT_OP_IMM32:
         default:
            }
      static void
   eval_out(struct mme_tu104_sim *sim,
               {
      if (inst->out[out_idx].mthd != MME_TU104_OUT_OP_NONE) {
               flush_mthd(sim);
   sim->mthd.mthd = (data & 0xfff) << 2;
   sim->mthd.inc = (data >> 12) & 0xf;
   sim->mthd.has_mthd = true;
               if (inst->out[out_idx].emit != MME_TU104_OUT_OP_NONE) {
               assert(sim->mthd.data_len < ARRAY_SIZE(sim->mthd.data));
         }
      void
   mme_tu104_sim(uint32_t inst_count, const struct mme_tu104_inst *insts,
               {
      struct mme_tu104_sim sim = {
      .param_count = param_count,
   .params = params,
   .mem_count = mem_count,
               bool end_next = false;
   while (true) {
      assert(sim.ip < inst_count);
   const struct mme_tu104_inst *inst = &insts[sim.ip];
                              /* No idea why the HW has this rule but it does */
   assert(inst->alu[0].op != MME_TU104_ALU_OP_STATE ||
            if (pred & BITFIELD_BIT(0))
         if (pred & BITFIELD_BIT(1))
         if (pred & BITFIELD_BIT(2))
         if (pred & BITFIELD_BIT(3))
            if (end_next || sim.stop)
                     if (sim.loop_count > 0 && sim.ip == sim.loop_end) {
      sim.loop_count--;
                              }
