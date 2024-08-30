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
      #include "radv_cs.h"
   #include "radv_private.h"
   #include "sid.h"
      #define SPM_RING_BASE_ALIGN 32
      static bool
   radv_spm_init_bo(struct radv_device *device)
   {
      struct radeon_winsys *ws = device->ws;
   uint64_t size = 32 * 1024 * 1024; /* Default to 1MB. */
   uint16_t sample_interval = 4096;  /* Default to 4096 clk. */
            device->spm.buffer_size = size;
            struct radeon_winsys_bo *bo = NULL;
   result = ws->buffer_create(ws, size, 4096, RADEON_DOMAIN_VRAM,
               device->spm.bo = bo;
   if (result != VK_SUCCESS)
            result = ws->buffer_make_resident(ws, device->spm.bo, true);
   if (result != VK_SUCCESS)
            device->spm.ptr = ws->buffer_map(device->spm.bo);
   if (!device->spm.ptr)
               }
      static void
   radv_emit_spm_counters(struct radv_device *device, struct radeon_cmdbuf *cs, enum radv_queue_family qf)
   {
      const enum amd_gfx_level gfx_level = device->physical_device->rad_info.gfx_level;
            if (gfx_level >= GFX11) {
      for (uint32_t instance = 0; instance < ARRAY_SIZE(spm->sq_wgp); instance++) {
                                                for (uint32_t b = 0; b < num_counters; b++) {
                     radeon_set_uconfig_reg_seq_perfctr(gfx_level, qf, cs, reg_base + b * 4, 1);
                     for (uint32_t instance = 0; instance < ARRAY_SIZE(spm->sqg); instance++) {
               if (!num_counters)
                     radeon_set_uconfig_reg(
                  for (uint32_t b = 0; b < num_counters; b++) {
                     radeon_set_uconfig_reg_seq_perfctr(gfx_level, qf, cs, reg_base + b * 4, 1);
                  for (uint32_t b = 0; b < spm->num_block_sel; b++) {
      struct ac_spm_block_select *block_sel = &spm->block_sel[b];
            for (unsigned i = 0; i < block_sel->num_instances; i++) {
                                                                                 radeon_set_uconfig_reg_seq_perfctr(gfx_level, qf, cs, regs->select1[c], 1);
                     /* Restore global broadcasting. */
   radeon_set_uconfig_reg(
      cs, R_030800_GRBM_GFX_INDEX,
   }
      void
   radv_emit_spm_setup(struct radv_device *device, struct radeon_cmdbuf *cs, enum radv_queue_family qf)
   {
      const enum amd_gfx_level gfx_level = device->physical_device->rad_info.gfx_level;
   struct ac_spm *spm = &device->spm;
   uint64_t va = radv_buffer_get_va(spm->bo);
            /* It's required that the ring VA and the size are correctly aligned. */
   assert(!(va & (SPM_RING_BASE_ALIGN - 1)));
   assert(!(ring_size & (SPM_RING_BASE_ALIGN - 1)));
                     /* Configure the SPM ring buffer. */
   radeon_set_uconfig_reg(cs, R_037200_RLC_SPM_PERFMON_CNTL,
               radeon_set_uconfig_reg(cs, R_037204_RLC_SPM_PERFMON_RING_BASE_LO, va);
   radeon_set_uconfig_reg(cs, R_037208_RLC_SPM_PERFMON_RING_BASE_HI, S_037208_RING_BASE_HI(va >> 32));
            /* Configure the muxsel. */
   uint32_t total_muxsel_lines = 0;
   for (unsigned s = 0; s < AC_SPM_SEGMENT_TYPE_COUNT; s++) {
                           if (device->physical_device->rad_info.gfx_level >= GFX11) {
      radeon_set_uconfig_reg(cs, R_03721C_RLC_SPM_PERFMON_SEGMENT_SIZE,
                           } else {
      radeon_set_uconfig_reg(cs, R_037210_RLC_SPM_PERFMON_SEGMENT_SIZE, 0);
   radeon_set_uconfig_reg(cs, R_03727C_RLC_SPM_PERFMON_SE3TO0_SEGMENT_SIZE,
                           radeon_set_uconfig_reg(cs, R_037280_RLC_SPM_PERFMON_GLB_SEGMENT_SIZE,
                     /* Upload each muxsel ram to the RLC. */
   for (unsigned s = 0; s < AC_SPM_SEGMENT_TYPE_COUNT; s++) {
      unsigned rlc_muxsel_addr, rlc_muxsel_data;
            if (!spm->num_muxsel_lines[s])
            if (s == AC_SPM_SEGMENT_TYPE_GLOBAL) {
               rlc_muxsel_addr =
         rlc_muxsel_data =
      } else {
               rlc_muxsel_addr = gfx_level >= GFX11 ? R_037228_RLC_SPM_SE_MUXSEL_ADDR : R_03721C_RLC_SPM_SE_MUXSEL_ADDR;
                                 for (unsigned l = 0; l < spm->num_muxsel_lines[s]; l++) {
                              /* Write the muxsel line configuration with MUXSEL_DATA. */
   radeon_emit(cs, PKT3(PKT3_WRITE_DATA, 2 + AC_SPM_MUXSEL_LINE_SIZE, 0));
   radeon_emit(cs, S_370_DST_SEL(V_370_MEM_MAPPED_REGISTER) | S_370_WR_CONFIRM(1) | S_370_ENGINE_SEL(V_370_ME) |
         radeon_emit(cs, rlc_muxsel_data >> 2);
   radeon_emit(cs, 0);
                  /* Select SPM counters. */
      }
      bool
   radv_spm_init(struct radv_device *device)
   {
      const struct radeon_info *info = &device->physical_device->rad_info;
            /* We failed to initialize the performance counters. */
   if (!pc->blocks)
            if (!ac_init_spm(info, pc, &device->spm))
            if (!radv_spm_init_bo(device))
               }
      void
   radv_spm_finish(struct radv_device *device)
   {
               if (device->spm.bo) {
      ws->buffer_make_resident(ws, device->spm.bo, false);
                  }
