   /*
   * Copyright © 2021 Valve Corporation
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
      #include <inttypes.h>
      #include "ac_perfcounter.h"
   #include "amdgfxregs.h"
   #include "radv_cs.h"
   #include "radv_private.h"
   #include "sid.h"
      void
   radv_perfcounter_emit_shaders(struct radv_device *device, struct radeon_cmdbuf *cs, unsigned shaders)
   {
      if (device->physical_device->rad_info.gfx_level >= GFX11) {
         } else {
      radeon_set_uconfig_reg_seq(cs, R_036780_SQ_PERFCOUNTER_CTRL, 2);
   radeon_emit(cs, shaders & 0x7f);
         }
      static void
   radv_emit_windowed_counters(struct radv_device *device, struct radeon_cmdbuf *cs, int family, bool enable)
   {
      if (family == RADV_QUEUE_GENERAL) {
      radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
                  }
      void
   radv_perfcounter_emit_spm_reset(struct radeon_cmdbuf *cs)
   {
      radeon_set_uconfig_reg(cs, R_036020_CP_PERFMON_CNTL,
            }
      void
   radv_perfcounter_emit_spm_start(struct radv_device *device, struct radeon_cmdbuf *cs, int family)
   {
      /* Start SPM counters. */
   radeon_set_uconfig_reg(cs, R_036020_CP_PERFMON_CNTL,
                     }
      void
   radv_perfcounter_emit_spm_stop(struct radv_device *device, struct radeon_cmdbuf *cs, int family)
   {
               /* Stop SPM counters. */
   radeon_set_uconfig_reg(cs, R_036020_CP_PERFMON_CNTL,
                        }
      enum radv_perfcounter_op {
      RADV_PC_OP_SUM,
   RADV_PC_OP_MAX,
   RADV_PC_OP_RATIO_DIVSCALE,
   RADV_PC_OP_REVERSE_RATIO, /* (reg1 - reg0) / reg1 */
      };
      #define S_REG_SEL(x)   ((x)&0xFFFF)
   #define G_REG_SEL(x)   ((x)&0xFFFF)
   #define S_REG_BLOCK(x) ((x) << 16)
   #define G_REG_BLOCK(x) (((x) >> 16) & 0x7FFF)
      #define S_REG_OFFSET(x)    ((x)&0xFFFF)
   #define G_REG_OFFSET(x)    ((x)&0xFFFF)
   #define S_REG_INSTANCES(x) ((x) << 16)
   #define G_REG_INSTANCES(x) (((x) >> 16) & 0x7FFF)
   #define S_REG_CONSTANT(x)  ((x) << 31)
   #define G_REG_CONSTANT(x)  ((x) >> 31)
      struct radv_perfcounter_impl {
      enum radv_perfcounter_op op;
      };
      /* Only append to this list, never insert into the middle or remove (but can rename).
   *
   * The invariant we're trying to get here is counters that have the same meaning, so
   * these can be shared between counters that have different implementations on different
   * GPUs, but should be unique within a GPU.
   */
   enum radv_perfcounter_uuid {
      RADV_PC_UUID_GPU_CYCLES,
   RADV_PC_UUID_SHADER_WAVES,
   RADV_PC_UUID_SHADER_INSTRUCTIONS,
   RADV_PC_UUID_SHADER_INSTRUCTIONS_VALU,
   RADV_PC_UUID_SHADER_INSTRUCTIONS_SALU,
   RADV_PC_UUID_SHADER_INSTRUCTIONS_VMEM_LOAD,
   RADV_PC_UUID_SHADER_INSTRUCTIONS_SMEM_LOAD,
   RADV_PC_UUID_SHADER_INSTRUCTIONS_VMEM_STORE,
   RADV_PC_UUID_SHADER_INSTRUCTIONS_LDS,
   RADV_PC_UUID_SHADER_INSTRUCTIONS_GDS,
   RADV_PC_UUID_SHADER_VALU_BUSY,
   RADV_PC_UUID_SHADER_SALU_BUSY,
   RADV_PC_UUID_VRAM_READ_SIZE,
   RADV_PC_UUID_VRAM_WRITE_SIZE,
   RADV_PC_UUID_L0_CACHE_HIT_RATIO,
   RADV_PC_UUID_L1_CACHE_HIT_RATIO,
      };
      struct radv_perfcounter_desc {
                        char name[VK_MAX_DESCRIPTION_SIZE];
   char category[VK_MAX_DESCRIPTION_SIZE];
   char description[VK_MAX_DESCRIPTION_SIZE];
      };
      #define PC_DESC(arg_op, arg_unit, arg_name, arg_category, arg_description, arg_uuid, ...)                              \
      (struct radv_perfcounter_desc)                                                                                      \
   {                                                                                                                   \
      .impl = {.op = arg_op, .regs = {__VA_ARGS__}}, .unit = VK_PERFORMANCE_COUNTER_UNIT_##arg_unit##_KHR,             \
            #define ADD_PC(op, unit, name, category, description, uuid, ...)                                                       \
      do {                                                                                                                \
      if (descs) {                                                                                                     \
         }                                                                                                                \
         #define CTR(block, ctr) (S_REG_BLOCK(block) | S_REG_SEL(ctr))
   #define CONSTANT(v)     (S_REG_CONSTANT(1) | (uint32_t)(v))
      enum { GRBM_PERF_SEL_GUI_ACTIVE = CTR(GRBM, 2) };
      enum { CPF_PERF_SEL_CPF_STAT_BUSY_GFX10 = CTR(CPF, 0x18) };
      enum {
      GL1C_PERF_SEL_REQ = CTR(GL1C, 0xe),
      };
      enum {
               GL2C_PERF_SEL_MISS_GFX101 = CTR(GL2C, 0x23),
   GL2C_PERF_SEL_MC_WRREQ_GFX101 = CTR(GL2C, 0x4b),
   GL2C_PERF_SEL_EA_WRREQ_64B_GFX101 = CTR(GL2C, 0x4c),
   GL2C_PERF_SEL_EA_RDREQ_32B_GFX101 = CTR(GL2C, 0x59),
   GL2C_PERF_SEL_EA_RDREQ_64B_GFX101 = CTR(GL2C, 0x5a),
   GL2C_PERF_SEL_EA_RDREQ_96B_GFX101 = CTR(GL2C, 0x5b),
            GL2C_PERF_SEL_MISS_GFX103 = CTR(GL2C, 0x2b),
   GL2C_PERF_SEL_MC_WRREQ_GFX103 = CTR(GL2C, 0x53),
   GL2C_PERF_SEL_EA_WRREQ_64B_GFX103 = CTR(GL2C, 0x55),
   GL2C_PERF_SEL_EA_RDREQ_32B_GFX103 = CTR(GL2C, 0x63),
   GL2C_PERF_SEL_EA_RDREQ_64B_GFX103 = CTR(GL2C, 0x64),
   GL2C_PERF_SEL_EA_RDREQ_96B_GFX103 = CTR(GL2C, 0x65),
      };
      enum {
      SQ_PERF_SEL_WAVES = CTR(SQ, 0x4),
   SQ_PERF_SEL_INSTS_ALL_GFX10 = CTR(SQ, 0x31),
   SQ_PERF_SEL_INSTS_GDS_GFX10 = CTR(SQ, 0x37),
   SQ_PERF_SEL_INSTS_LDS_GFX10 = CTR(SQ, 0x3b),
   SQ_PERF_SEL_INSTS_SALU_GFX10 = CTR(SQ, 0x3c),
   SQ_PERF_SEL_INSTS_SMEM_GFX10 = CTR(SQ, 0x3d),
   SQ_PERF_SEL_INSTS_VALU_GFX10 = CTR(SQ, 0x40),
   SQ_PERF_SEL_INSTS_TEX_LOAD_GFX10 = CTR(SQ, 0x45),
   SQ_PERF_SEL_INSTS_TEX_STORE_GFX10 = CTR(SQ, 0x46),
      };
      enum {
      TCP_PERF_SEL_REQ_GFX10 = CTR(TCP, 0x9),
      };
      #define CTR_NUM_SIMD CONSTANT(pdev->rad_info.num_simd_per_compute_unit * pdev->rad_info.num_cu)
   #define CTR_NUM_CUS  CONSTANT(pdev->rad_info.num_cu)
      static void
   radv_query_perfcounter_descs(struct radv_physical_device *pdev, uint32_t *count, struct radv_perfcounter_desc *descs)
   {
               ADD_PC(RADV_PC_OP_MAX, CYCLES, "GPU active cycles", "GRBM", "cycles the GPU is active processing a command buffer.",
            ADD_PC(RADV_PC_OP_SUM, GENERIC, "Waves", "Shaders", "Number of waves executed", SHADER_WAVES, SQ_PERF_SEL_WAVES);
   ADD_PC(RADV_PC_OP_SUM, GENERIC, "Instructions", "Shaders", "Number of Instructions executed", SHADER_INSTRUCTIONS,
         ADD_PC(RADV_PC_OP_SUM, GENERIC, "VALU Instructions", "Shaders", "Number of VALU Instructions executed",
         ADD_PC(RADV_PC_OP_SUM, GENERIC, "SALU Instructions", "Shaders", "Number of SALU Instructions executed",
         ADD_PC(RADV_PC_OP_SUM, GENERIC, "VMEM Load Instructions", "Shaders", "Number of VMEM load instructions executed",
         ADD_PC(RADV_PC_OP_SUM, GENERIC, "SMEM Load Instructions", "Shaders", "Number of SMEM load instructions executed",
         ADD_PC(RADV_PC_OP_SUM, GENERIC, "VMEM Store Instructions", "Shaders", "Number of VMEM store instructions executed",
         ADD_PC(RADV_PC_OP_SUM, GENERIC, "LDS Instructions", "Shaders", "Number of LDS Instructions executed",
         ADD_PC(RADV_PC_OP_SUM, GENERIC, "GDS Instructions", "Shaders", "Number of GDS Instructions executed",
            ADD_PC(RADV_PC_OP_RATIO_DIVSCALE, PERCENTAGE, "VALU Busy", "Shader Utilization",
         "Percentage of time the VALU units are busy", SHADER_VALU_BUSY, SQ_PERF_SEL_INST_CYCLES_VALU_GFX10,
   ADD_PC(RADV_PC_OP_RATIO_DIVSCALE, PERCENTAGE, "SALU Busy", "Shader Utilization",
                  if (pdev->rad_info.gfx_level >= GFX10_3) {
      ADD_PC(RADV_PC_OP_SUM_WEIGHTED_4, BYTES, "VRAM read size", "Memory", "Number of bytes read from VRAM",
         VRAM_READ_SIZE, GL2C_PERF_SEL_EA_RDREQ_32B_GFX103, CONSTANT(32), GL2C_PERF_SEL_EA_RDREQ_64B_GFX103,
   CONSTANT(64), GL2C_PERF_SEL_EA_RDREQ_96B_GFX103, CONSTANT(96), GL2C_PERF_SEL_EA_RDREQ_128B_GFX103,
   ADD_PC(RADV_PC_OP_SUM_WEIGHTED_4, BYTES, "VRAM write size", "Memory", "Number of bytes written to VRAM",
            } else {
      ADD_PC(RADV_PC_OP_SUM_WEIGHTED_4, BYTES, "VRAM read size", "Memory", "Number of bytes read from VRAM",
         VRAM_READ_SIZE, GL2C_PERF_SEL_EA_RDREQ_32B_GFX101, CONSTANT(32), GL2C_PERF_SEL_EA_RDREQ_64B_GFX101,
   CONSTANT(64), GL2C_PERF_SEL_EA_RDREQ_96B_GFX101, CONSTANT(96), GL2C_PERF_SEL_EA_RDREQ_128B_GFX101,
   ADD_PC(RADV_PC_OP_SUM_WEIGHTED_4, BYTES, "VRAM write size", "Memory", "Number of bytes written to VRAM",
                     ADD_PC(RADV_PC_OP_REVERSE_RATIO, BYTES, "L0 cache hit ratio", "Memory", "Hit ratio of L0 cache", L0_CACHE_HIT_RATIO,
         ADD_PC(RADV_PC_OP_REVERSE_RATIO, BYTES, "L1 cache hit ratio", "Memory", "Hit ratio of L1 cache", L1_CACHE_HIT_RATIO,
         if (pdev->rad_info.gfx_level >= GFX10_3) {
      ADD_PC(RADV_PC_OP_REVERSE_RATIO, BYTES, "L2 cache hit ratio", "Memory", "Hit ratio of L2 cache",
      } else {
      ADD_PC(RADV_PC_OP_REVERSE_RATIO, BYTES, "L2 cache hit ratio", "Memory", "Hit ratio of L2 cache",
         }
      static bool
   radv_init_perfcounter_descs(struct radv_physical_device *pdev)
   {
      if (pdev->perfcounters)
            uint32_t count;
            struct radv_perfcounter_desc *descs = malloc(sizeof(*descs) * count);
   if (!descs)
            radv_query_perfcounter_descs(pdev, &count, descs);
   pdev->num_perfcounters = count;
               }
      static int
   cmp_uint32_t(const void *a, const void *b)
   {
      uint32_t l = *(const uint32_t *)a;
               }
      static VkResult
   radv_get_counter_registers(const struct radv_physical_device *pdevice, uint32_t num_indices, const uint32_t *indices,
         {
      ASSERTED uint32_t num_counters = pdevice->num_perfcounters;
            unsigned full_reg_cnt = num_indices * ARRAY_SIZE(descs->impl.regs);
   uint32_t *regs = malloc(full_reg_cnt * sizeof(uint32_t));
   if (!regs)
            unsigned reg_cnt = 0;
   for (unsigned i = 0; i < num_indices; ++i) {
      uint32_t index = indices[i];
   assert(index < num_counters);
   for (unsigned j = 0; j < ARRAY_SIZE(descs[index].impl.regs) && descs[index].impl.regs[j]; ++j) {
      if (!G_REG_CONSTANT(descs[index].impl.regs[j]))
                           unsigned deduped_reg_cnt = 0;
   for (unsigned i = 1; i < reg_cnt; ++i) {
      if (regs[i] != regs[deduped_reg_cnt])
      }
            *out_num_regs = deduped_reg_cnt;
   *out_regs = regs;
      }
      static unsigned
   radv_pc_get_num_instances(const struct radv_physical_device *pdevice, struct ac_pc_block *ac_block)
   {
         }
      static unsigned
   radv_get_num_counter_passes(const struct radv_physical_device *pdevice, unsigned num_regs, const uint32_t *regs)
   {
      enum ac_pc_gpu_block prev_block = NUM_GPU_BLOCK;
   unsigned block_reg_count = 0;
   struct ac_pc_block *ac_block = NULL;
            for (unsigned i = 0; i < num_regs; ++i) {
               if (block != prev_block) {
      block_reg_count = 0;
   prev_block = block;
                                       }
      void
   radv_pc_deinit_query_pool(struct radv_pc_query_pool *pool)
   {
      free(pool->counters);
      }
      VkResult
   radv_pc_init_query_pool(struct radv_physical_device *pdevice, const VkQueryPoolCreateInfo *pCreateInfo,
         {
      const VkQueryPoolPerformanceCreateInfoKHR *perf_info =
                  if (!radv_init_perfcounter_descs(pdevice))
            result = radv_get_counter_registers(pdevice, perf_info->counterIndexCount, perf_info->pCounterIndices,
         if (result != VK_SUCCESS)
                     uint32_t *pc_reg_offsets = malloc(pool->num_pc_regs * sizeof(uint32_t));
   if (!pc_reg_offsets)
            unsigned offset = 0;
   for (unsigned i = 0; i < pool->num_pc_regs; ++i) {
      enum ac_pc_gpu_block block = pool->pc_regs[i] >> 16;
   struct ac_pc_block *ac_block = ac_pc_get_block(&pdevice->ac_perfcounters, block);
            pc_reg_offsets[i] = S_REG_OFFSET(offset) | S_REG_INSTANCES(num_instances);
               /* allow an uint32_t per pass to signal completion. */
            pool->num_counters = perf_info->counterIndexCount;
   pool->counters = malloc(pool->num_counters * sizeof(struct radv_perfcounter_impl));
   if (!pool->counters) {
      free(pc_reg_offsets);
               for (unsigned i = 0; i < pool->num_counters; ++i) {
               for (unsigned j = 0; j < ARRAY_SIZE(pool->counters[i].regs); ++j) {
      uint32_t reg = pool->counters[i].regs[j];
                  unsigned k;
   for (k = 0; k < pool->num_pc_regs; ++k)
      if (pool->pc_regs[k] == reg)
                     free(pc_reg_offsets);
      }
      static void
   radv_emit_instance(struct radv_cmd_buffer *cmd_buffer, int se, int instance)
   {
      struct radeon_cmdbuf *cs = cmd_buffer->cs;
            if (se >= 0) {
         } else {
                  if (instance >= 0) {
         } else {
                     }
      static void
   radv_emit_select(struct radv_cmd_buffer *cmd_buffer, struct ac_pc_block *block, unsigned count, unsigned *selectors)
   {
      const enum amd_gfx_level gfx_level = cmd_buffer->device->physical_device->rad_info.gfx_level;
   const enum radv_queue_family qf = cmd_buffer->qf;
   struct ac_pc_block_base *regs = block->b->b;
   struct radeon_cmdbuf *cs = cmd_buffer->cs;
                     /* Fake counters. */
   if (!regs->select0)
            for (idx = 0; idx < count; ++idx) {
                  for (idx = 0; idx < regs->num_spm_counters; idx++) {
      radeon_set_uconfig_reg_seq(cs, regs->select1[idx], 1);
         }
      static void
   radv_pc_emit_block_instance_read(struct radv_cmd_buffer *cmd_buffer, struct ac_pc_block *block, unsigned count,
         {
      struct ac_pc_block_base *regs = block->b->b;
   struct radeon_cmdbuf *cs = cmd_buffer->cs;
   unsigned reg = regs->counter0_lo;
            assert(regs->select0);
   for (unsigned idx = 0; idx < count; ++idx) {
      if (regs->counters)
            radeon_emit(cs, PKT3(PKT3_COPY_DATA, 4, 0));
   radeon_emit(cs, COPY_DATA_SRC_SEL(COPY_DATA_PERF) | COPY_DATA_DST_SEL(COPY_DATA_TC_L2) | COPY_DATA_WR_CONFIRM |
         radeon_emit(cs, reg >> 2);
   radeon_emit(cs, 0); /* unused */
   radeon_emit(cs, va);
            va += sizeof(uint64_t) * 2 * radv_pc_get_num_instances(cmd_buffer->device->physical_device, block);
         }
      static void
   radv_pc_sample_block(struct radv_cmd_buffer *cmd_buffer, struct ac_pc_block *block, unsigned count, uint64_t va)
   {
      unsigned se_end = 1;
   if (block->b->b->flags & AC_PC_BLOCK_SE)
            for (unsigned se = 0; se < se_end; ++se) {
      for (unsigned instance = 0; instance < block->num_instances; ++instance) {
      radv_emit_instance(cmd_buffer, se, instance);
   radv_pc_emit_block_instance_read(cmd_buffer, block, count, va);
            }
      static void
   radv_pc_wait_idle(struct radv_cmd_buffer *cmd_buffer)
   {
               radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
            radeon_emit(cs, PKT3(PKT3_ACQUIRE_MEM, 6, 0));
   radeon_emit(cs, 0);          /* CP_COHER_CNTL */
   radeon_emit(cs, 0xffffffff); /* CP_COHER_SIZE */
   radeon_emit(cs, 0xffffff);   /* CP_COHER_SIZE_HI */
   radeon_emit(cs, 0);          /* CP_COHER_BASE */
   radeon_emit(cs, 0);          /* CP_COHER_BASE_HI */
   radeon_emit(cs, 0x0000000A); /* POLL_INTERVAL */
            radeon_emit(cs, PKT3(PKT3_PFP_SYNC_ME, 0, 0));
      }
      static void
   radv_pc_stop_and_sample(struct radv_cmd_buffer *cmd_buffer, struct radv_pc_query_pool *pool, uint64_t va, bool end)
   {
      struct radeon_cmdbuf *cs = cmd_buffer->cs;
            radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
                     radv_emit_instance(cmd_buffer, -1, -1);
            radeon_set_uconfig_reg(
      cs, R_036020_CP_PERFMON_CNTL,
         for (unsigned pass = 0; pass < pool->num_passes; ++pass) {
      uint64_t pred_va = radv_buffer_get_va(cmd_buffer->device->perf_counter_bo) + PERF_CTR_BO_PASS_OFFSET + 8 * pass;
            radeon_emit(cs, PKT3(PKT3_COND_EXEC, 3, 0));
   radeon_emit(cs, pred_va);
   radeon_emit(cs, pred_va >> 32);
            uint32_t *skip_dwords = cs->buf + cs->cdw;
            for (unsigned i = 0; i < pool->num_pc_regs;) {
      enum ac_pc_gpu_block block = G_REG_BLOCK(pool->pc_regs[i]);
   struct ac_pc_block *ac_block = ac_pc_get_block(&pdevice->ac_perfcounters, block);
                  unsigned cnt = 1;
                  if (offset < cnt) {
      unsigned pass_reg_cnt = MIN2(cnt - offset, ac_block->b->b->num_counters);
   radv_pc_sample_block(cmd_buffer, ac_block, pass_reg_cnt,
               i += cnt;
               if (end) {
      uint64_t signal_va = va + pool->b.stride - 8 - 8 * pass;
   radeon_emit(cs, PKT3(PKT3_WRITE_DATA, 3, 0));
   radeon_emit(cs, S_370_DST_SEL(V_370_MEM) | S_370_WR_CONFIRM(1) | S_370_ENGINE_SEL(V_370_ME));
   radeon_emit(cs, signal_va);
   radeon_emit(cs, signal_va >> 32);
                              }
      void
   radv_pc_begin_query(struct radv_cmd_buffer *cmd_buffer, struct radv_pc_query_pool *pool, uint64_t va)
   {
      struct radeon_cmdbuf *cs = cmd_buffer->cs;
   struct radv_physical_device *pdevice = cmd_buffer->device->physical_device;
                     cdw_max = radeon_check_space(cmd_buffer->device->ws, cs,
                        radv_cs_add_buffer(cmd_buffer->device->ws, cmd_buffer->cs, pool->b.bo);
            uint64_t perf_ctr_va = radv_buffer_get_va(cmd_buffer->device->perf_counter_bo) + PERF_CTR_BO_FENCE_OFFSET;
   radeon_emit(cs, PKT3(PKT3_WRITE_DATA, 3, 0));
   radeon_emit(cs, S_370_DST_SEL(V_370_MEM) | S_370_WR_CONFIRM(1) | S_370_ENGINE_SEL(V_370_ME));
   radeon_emit(cs, perf_ctr_va);
   radeon_emit(cs, perf_ctr_va >> 32);
                     radeon_set_uconfig_reg(cs, R_036020_CP_PERFMON_CNTL,
            radv_emit_inhibit_clockgating(cmd_buffer->device, cs, true);
   radv_emit_spi_config_cntl(cmd_buffer->device, cs, true);
            for (unsigned pass = 0; pass < pool->num_passes; ++pass) {
               radeon_emit(cs, PKT3(PKT3_COND_EXEC, 3, 0));
   radeon_emit(cs, pred_va);
   radeon_emit(cs, pred_va >> 32);
            uint32_t *skip_dwords = cs->buf + cs->cdw;
            for (unsigned i = 0; i < pool->num_pc_regs;) {
      enum ac_pc_gpu_block block = G_REG_BLOCK(pool->pc_regs[i]);
                  unsigned cnt = 1;
                  if (offset < cnt) {
      unsigned pass_reg_cnt = MIN2(cnt - offset, ac_block->b->b->num_counters);
                                                                  radeon_set_uconfig_reg(cs, R_036020_CP_PERFMON_CNTL,
                        }
      void
   radv_pc_end_query(struct radv_cmd_buffer *cmd_buffer, struct radv_pc_query_pool *pool, uint64_t va)
   {
      struct radeon_cmdbuf *cs = cmd_buffer->cs;
            cdw_max = radeon_check_space(cmd_buffer->device->ws, cs,
                        radv_cs_add_buffer(cmd_buffer->device->ws, cmd_buffer->cs, pool->b.bo);
            uint64_t perf_ctr_va = radv_buffer_get_va(cmd_buffer->device->perf_counter_bo) + PERF_CTR_BO_FENCE_OFFSET;
   si_cs_emit_write_event_eop(cs, cmd_buffer->device->physical_device->rad_info.gfx_level, cmd_buffer->qf,
                        radv_pc_wait_idle(cmd_buffer);
            radeon_set_uconfig_reg(cs, R_036020_CP_PERFMON_CNTL,
         radv_emit_spi_config_cntl(cmd_buffer->device, cs, false);
               }
      static uint64_t
   radv_pc_sum_reg(uint32_t reg, const uint64_t *data)
   {
      unsigned instances = G_REG_INSTANCES(reg);
   unsigned offset = G_REG_OFFSET(reg) / 8;
            if (G_REG_CONSTANT(reg))
            for (unsigned i = 0; i < instances; ++i) {
                     }
      static uint64_t
   radv_pc_max_reg(uint32_t reg, const uint64_t *data)
   {
      unsigned instances = G_REG_INSTANCES(reg);
   unsigned offset = G_REG_OFFSET(reg) / 8;
            if (G_REG_CONSTANT(reg))
            for (unsigned i = 0; i < instances; ++i) {
                     }
      static union VkPerformanceCounterResultKHR
   radv_pc_get_result(const struct radv_perfcounter_impl *impl, const uint64_t *data)
   {
               switch (impl->op) {
   case RADV_PC_OP_MAX:
      result.float64 = radv_pc_max_reg(impl->regs[0], data);
      case RADV_PC_OP_SUM:
      result.float64 = radv_pc_sum_reg(impl->regs[0], data);
      case RADV_PC_OP_RATIO_DIVSCALE:
      result.float64 = radv_pc_sum_reg(impl->regs[0], data) / (double)radv_pc_sum_reg(impl->regs[1], data) /
            case RADV_PC_OP_REVERSE_RATIO: {
      double tmp = radv_pc_sum_reg(impl->regs[1], data);
   result.float64 = (tmp - radv_pc_sum_reg(impl->regs[0], data)) / tmp * 100.0;
      }
   case RADV_PC_OP_SUM_WEIGHTED_4:
      result.float64 = 0.0;
   for (unsigned i = 0; i < 4; ++i)
            default:
         }
      }
      void
   radv_pc_get_results(const struct radv_pc_query_pool *pc_pool, const uint64_t *data, void *out)
   {
               for (unsigned i = 0; i < pc_pool->num_counters; ++i) {
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(
      VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, uint32_t *pCounterCount,
      {
               if (vk_queue_to_radv(pdevice, queueFamilyIndex) != RADV_QUEUE_GENERAL) {
      *pCounterCount = 0;
               if (!radv_init_perfcounter_descs(pdevice))
            uint32_t counter_cnt = pdevice->num_perfcounters;
            if (!pCounters && !pCounterDescriptions) {
      *pCounterCount = counter_cnt;
               VkResult result = counter_cnt > *pCounterCount ? VK_INCOMPLETE : VK_SUCCESS;
   counter_cnt = MIN2(counter_cnt, *pCounterCount);
            for (uint32_t i = 0; i < counter_cnt; ++i) {
      if (pCounters) {
      pCounters[i].sType = VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_KHR;
   pCounters[i].unit = descs[i].unit;
                                 const uint32_t uuid = descs[i].uuid;
               if (pCounterDescriptions) {
      pCounterDescriptions[i].sType = VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_DESCRIPTION_KHR;
   pCounterDescriptions[i].flags = VK_PERFORMANCE_COUNTER_DESCRIPTION_CONCURRENTLY_IMPACTED_BIT_KHR;
   strcpy(pCounterDescriptions[i].name, descs[i].name);
   strcpy(pCounterDescriptions[i].category, descs[i].category);
         }
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(
      VkPhysicalDevice physicalDevice, const VkQueryPoolPerformanceCreateInfoKHR *pPerformanceQueryCreateInfo,
      {
               if (pPerformanceQueryCreateInfo->counterIndexCount == 0) {
      *pNumPasses = 0;
               if (!radv_init_perfcounter_descs(pdevice)) {
      /* Can't return an error, so log */
   fprintf(stderr, "radv: Failed to init perf counters\n");
   *pNumPasses = 1;
                        unsigned num_regs = 0;
   uint32_t *regs = NULL;
   VkResult result = radv_get_counter_registers(pdevice, pPerformanceQueryCreateInfo->counterIndexCount,
         if (result != VK_SUCCESS) {
      /* Can't return an error, so log */
               *pNumPasses = radv_get_num_counter_passes(pdevice, num_regs, regs);
      }
