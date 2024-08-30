   /*
   * Copyright © 2020 Valve Corporation
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
      #define SQTT_BUFFER_ALIGN_SHIFT 12
      bool
   radv_is_instruction_timing_enabled(void)
   {
         }
      static uint32_t
   gfx11_get_sqtt_ctrl(const struct radv_device *device, bool enable)
   {
      return S_0367B0_MODE(enable) | S_0367B0_HIWATER(5) | S_0367B0_UTIL_TIMER(1) | S_0367B0_RT_FREQ(2) | /* 4096 clk */
      }
      static uint32_t
   gfx10_get_sqtt_ctrl(const struct radv_device *device, bool enable)
   {
      uint32_t sqtt_ctrl = S_008D1C_MODE(enable) | S_008D1C_HIWATER(5) | S_008D1C_UTIL_TIMER(1) |
                        if (device->physical_device->rad_info.gfx_level == GFX10_3)
            if (device->physical_device->rad_info.has_sqtt_auto_flush_mode_bug)
               }
      static enum radv_queue_family
   radv_ip_to_queue_family(enum amd_ip_type t)
   {
      switch (t) {
   case AMD_IP_GFX:
         case AMD_IP_COMPUTE:
         case AMD_IP_SDMA:
         default:
            }
      static void
   radv_emit_wait_for_idle(const struct radv_device *device, struct radeon_cmdbuf *cs, int family)
   {
      const enum radv_queue_family qf = radv_ip_to_queue_family(family);
   enum rgp_flush_bits sqtt_flush_bits = 0;
   si_cs_emit_cache_flush(
      device->ws, cs, device->physical_device->rad_info.gfx_level, NULL, 0, qf,
   (family == RADV_QUEUE_COMPUTE ? RADV_CMD_FLAG_CS_PARTIAL_FLUSH
               }
      static void
   radv_emit_sqtt_start(const struct radv_device *device, struct radeon_cmdbuf *cs, enum radv_queue_family qf)
   {
      const enum amd_gfx_level gfx_level = device->physical_device->rad_info.gfx_level;
   uint32_t shifted_size = device->sqtt.buffer_size >> SQTT_BUFFER_ALIGN_SHIFT;
   const struct radeon_info *rad_info = &device->physical_device->rad_info;
   const unsigned shader_mask = ac_sqtt_get_shader_mask(rad_info);
                     for (unsigned se = 0; se < max_se; se++) {
      uint64_t va = radv_buffer_get_va(device->sqtt.bo);
   uint64_t data_va = ac_sqtt_get_data_va(rad_info, &device->sqtt, va, se);
   uint64_t shifted_va = data_va >> SQTT_BUFFER_ALIGN_SHIFT;
            if (ac_sqtt_se_is_disabled(rad_info, se))
            /* Target SEx and SH0. */
   radeon_set_uconfig_reg(cs, R_030800_GRBM_GFX_INDEX,
            if (device->physical_device->rad_info.gfx_level >= GFX11) {
      /* Order seems important for the following 2 registers. */
                           radeon_set_perfctr_reg(gfx_level, qf, cs, R_0367B4_SQ_THREAD_TRACE_MASK,
                  uint32_t sqtt_token_mask = S_0367B8_REG_INCLUDE(V_0367B8_REG_INCLUDE_SQDEC | V_0367B8_REG_INCLUDE_SHDEC |
                                 if (!radv_is_instruction_timing_enabled()) {
      /* Reduce SQTT traffic when instruction timing isn't enabled. */
   token_exclude |= V_0367B8_TOKEN_EXCLUDE_VMEMEXEC | V_0367B8_TOKEN_EXCLUDE_ALUEXEC |
                                                } else if (device->physical_device->rad_info.gfx_level >= GFX10) {
      /* Order seems important for the following 2 registers. */
                           radeon_set_privileged_config_reg(cs, R_008D14_SQ_THREAD_TRACE_MASK,
                  uint32_t sqtt_token_mask = S_008D18_REG_INCLUDE(V_008D18_REG_INCLUDE_SQDEC | V_008D18_REG_INCLUDE_SHDEC |
                                 if (!radv_is_instruction_timing_enabled()) {
      /* Reduce SQTT traffic when instruction timing isn't enabled. */
   token_exclude |= V_008D18_TOKEN_EXCLUDE_VMEMEXEC | V_008D18_TOKEN_EXCLUDE_ALUEXEC |
            }
                           /* Should be emitted last (it enables thread traces). */
      } else {
                                                uint32_t sqtt_mask = S_030CC8_CU_SEL(active_cu) | S_030CC8_SH_SEL(0) | S_030CC8_SIMD_EN(0xf) |
                  if (device->physical_device->rad_info.gfx_level < GFX9) {
                           /* Trace all tokens and registers. */
                  /* Enable SQTT perf counters for all CUs. */
                                    if (device->physical_device->rad_info.gfx_level == GFX9) {
      /* Reset thread trace status errors. */
               /* Enable the thread trace mode. */
   uint32_t sqtt_mode = S_030CD8_MASK_PS(1) | S_030CD8_MASK_VS(1) | S_030CD8_MASK_GS(1) | S_030CD8_MASK_ES(1) |
                        if (device->physical_device->rad_info.gfx_level == GFX9) {
      /* Count SQTT traffic in TCC perf counters. */
                              /* Restore global broadcasting. */
   radeon_set_uconfig_reg(
      cs, R_030800_GRBM_GFX_INDEX,
         /* Start the thread trace with a different event based on the queue. */
   if (qf == RADV_QUEUE_COMPUTE) {
         } else {
      radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
         }
      static const uint32_t gfx8_sqtt_info_regs[] = {
      R_030CE4_SQ_THREAD_TRACE_WPTR,
   R_030CE8_SQ_THREAD_TRACE_STATUS,
      };
      static const uint32_t gfx9_sqtt_info_regs[] = {
      R_030CE4_SQ_THREAD_TRACE_WPTR,
   R_030CE8_SQ_THREAD_TRACE_STATUS,
      };
      static const uint32_t gfx10_sqtt_info_regs[] = {
      R_008D10_SQ_THREAD_TRACE_WPTR,
   R_008D20_SQ_THREAD_TRACE_STATUS,
      };
      static const uint32_t gfx11_sqtt_info_regs[] = {
      R_0367BC_SQ_THREAD_TRACE_WPTR,
   R_0367D0_SQ_THREAD_TRACE_STATUS,
      };
   static void
   radv_copy_sqtt_info_regs(const struct radv_device *device, struct radeon_cmdbuf *cs, unsigned se_index)
   {
      const struct radv_physical_device *pdevice = device->physical_device;
            if (device->physical_device->rad_info.gfx_level >= GFX11) {
         } else if (device->physical_device->rad_info.gfx_level >= GFX10) {
         } else if (device->physical_device->rad_info.gfx_level == GFX9) {
         } else {
      assert(device->physical_device->rad_info.gfx_level == GFX8);
               /* Get the VA where the info struct is stored for this SE. */
   uint64_t va = radv_buffer_get_va(device->sqtt.bo);
            /* Copy back the info struct one DWORD at a time. */
   for (unsigned i = 0; i < 3; i++) {
      radeon_emit(cs, PKT3(PKT3_COPY_DATA, 4, 0));
   radeon_emit(cs, COPY_DATA_SRC_SEL(COPY_DATA_PERF) | COPY_DATA_DST_SEL(COPY_DATA_TC_L2) | COPY_DATA_WR_CONFIRM);
   radeon_emit(cs, sqtt_info_regs[i] >> 2);
   radeon_emit(cs, 0); /* unused */
   radeon_emit(cs, (info_va + i * 4));
               if (pdevice->rad_info.gfx_level >= GFX11) {
      /* On GFX11, SQ_THREAD_TRACE_WPTR is incremented from the "initial WPTR address" instead of 0.
   * To get the number of bytes (in units of 32 bytes) written by SQTT, the workaround is to
   * subtract SQ_THREAD_TRACE_WPTR from the "initial WPTR address" as follow:
   *
   * 1) get the current buffer base address for this SE
   * 2) shift right by 5 bits because SQ_THREAD_TRACE_WPTR is 32-byte aligned
   * 3) mask off the higher 3 bits because WPTR.OFFSET is 29 bits
   */
   uint64_t data_va = ac_sqtt_get_data_va(&pdevice->rad_info, &device->sqtt, va, se_index);
   uint64_t shifted_data_va = (data_va >> 5);
            radeon_emit(cs, PKT3(PKT3_ATOMIC_MEM, 7, 0));
   radeon_emit(cs, ATOMIC_OP(TC_OP_ATOMIC_SUB_32));
   radeon_emit(cs, info_va);         /* addr lo */
   radeon_emit(cs, info_va >> 32);   /* addr hi */
   radeon_emit(cs, init_wptr_value); /* data lo */
   radeon_emit(cs, 0);               /* data hi */
   radeon_emit(cs, 0);               /* compare data lo */
   radeon_emit(cs, 0);               /* compare data hi */
         }
      static void
   radv_emit_sqtt_stop(const struct radv_device *device, struct radeon_cmdbuf *cs, enum radv_queue_family qf)
   {
      const enum amd_gfx_level gfx_level = device->physical_device->rad_info.gfx_level;
                     /* Stop the thread trace with a different event based on the queue. */
   if (qf == RADV_QUEUE_COMPUTE) {
         } else {
      radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
               radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
            if (device->physical_device->rad_info.has_sqtt_rb_harvest_bug) {
      /* Some chips with disabled RBs should wait for idle because FINISH_DONE doesn't work. */
               for (unsigned se = 0; se < max_se; se++) {
      if (ac_sqtt_se_is_disabled(&device->physical_device->rad_info, se))
            /* Target SEi and SH0. */
   radeon_set_uconfig_reg(cs, R_030800_GRBM_GFX_INDEX,
            if (device->physical_device->rad_info.gfx_level >= GFX11) {
      /* Make sure to wait for the trace buffer. */
   radeon_emit(cs, PKT3(PKT3_WAIT_REG_MEM, 5, 0));
   radeon_emit(cs, WAIT_REG_MEM_NOT_EQUAL); /* wait until the register is equal to the reference value */
   radeon_emit(cs, R_0367D0_SQ_THREAD_TRACE_STATUS >> 2); /* register */
   radeon_emit(cs, 0);
   radeon_emit(cs, 0); /* reference value */
                                 /* Wait for thread trace completion. */
   radeon_emit(cs, PKT3(PKT3_WAIT_REG_MEM, 5, 0));
   radeon_emit(cs, WAIT_REG_MEM_EQUAL); /* wait until the register is equal to the reference value */
   radeon_emit(cs, R_0367D0_SQ_THREAD_TRACE_STATUS >> 2); /* register */
   radeon_emit(cs, 0);
   radeon_emit(cs, 0);              /* reference value */
   radeon_emit(cs, ~C_0367D0_BUSY); /* mask */
      } else if (device->physical_device->rad_info.gfx_level >= GFX10) {
      if (!device->physical_device->rad_info.has_sqtt_rb_harvest_bug) {
      /* Make sure to wait for the trace buffer. */
   radeon_emit(cs, PKT3(PKT3_WAIT_REG_MEM, 5, 0));
   radeon_emit(cs, WAIT_REG_MEM_NOT_EQUAL); /* wait until the register is equal to the reference value */
   radeon_emit(cs, R_008D20_SQ_THREAD_TRACE_STATUS >> 2); /* register */
   radeon_emit(cs, 0);
   radeon_emit(cs, 0); /* reference value */
   radeon_emit(cs, ~C_008D20_FINISH_DONE);
                              /* Wait for thread trace completion. */
   radeon_emit(cs, PKT3(PKT3_WAIT_REG_MEM, 5, 0));
   radeon_emit(cs, WAIT_REG_MEM_EQUAL); /* wait until the register is equal to the reference value */
   radeon_emit(cs, R_008D20_SQ_THREAD_TRACE_STATUS >> 2); /* register */
   radeon_emit(cs, 0);
   radeon_emit(cs, 0);              /* reference value */
   radeon_emit(cs, ~C_008D20_BUSY); /* mask */
      } else {
                     /* Wait for thread trace completion. */
   radeon_emit(cs, PKT3(PKT3_WAIT_REG_MEM, 5, 0));
   radeon_emit(cs, WAIT_REG_MEM_EQUAL); /* wait until the register is equal to the reference value */
   radeon_emit(cs, R_030CE8_SQ_THREAD_TRACE_STATUS >> 2); /* register */
   radeon_emit(cs, 0);
   radeon_emit(cs, 0);              /* reference value */
   radeon_emit(cs, ~C_030CE8_BUSY); /* mask */
                           /* Restore global broadcasting. */
   radeon_set_uconfig_reg(
      cs, R_030800_GRBM_GFX_INDEX,
   }
      void
   radv_emit_sqtt_userdata(const struct radv_cmd_buffer *cmd_buffer, const void *data, uint32_t num_dwords)
   {
      const enum amd_gfx_level gfx_level = cmd_buffer->device->physical_device->rad_info.gfx_level;
   const enum radv_queue_family qf = cmd_buffer->qf;
   struct radv_device *device = cmd_buffer->device;
   struct radeon_cmdbuf *cs = cmd_buffer->cs;
            /* SQTT user data packets aren't supported on SDMA queues. */
   if (cmd_buffer->qf == RADV_QUEUE_TRANSFER)
            while (num_dwords > 0) {
                        /* Without the perfctr bit the CP might not always pass the
   * write on correctly. */
   if (device->physical_device->rad_info.gfx_level >= GFX10)
         else
                  dwords += count;
         }
      void
   radv_emit_spi_config_cntl(const struct radv_device *device, struct radeon_cmdbuf *cs, bool enable)
   {
      if (device->physical_device->rad_info.gfx_level >= GFX9) {
      uint32_t spi_config_cntl = S_031100_GPR_WRITE_PRIORITY(0x2c688) | S_031100_EXP_PRIORITY_ORDER(3) |
            if (device->physical_device->rad_info.gfx_level >= GFX10)
               } else {
      /* SPI_CONFIG_CNTL is a protected register on GFX6-GFX8. */
   radeon_set_privileged_config_reg(cs, R_009100_SPI_CONFIG_CNTL,
         }
      void
   radv_emit_inhibit_clockgating(const struct radv_device *device, struct radeon_cmdbuf *cs, bool inhibit)
   {
      if (device->physical_device->rad_info.gfx_level >= GFX11)
            if (device->physical_device->rad_info.gfx_level >= GFX10) {
         } else if (device->physical_device->rad_info.gfx_level >= GFX8) {
            }
      static bool
   radv_sqtt_init_bo(struct radv_device *device)
   {
      unsigned max_se = device->physical_device->rad_info.max_se;
   struct radeon_winsys *ws = device->ws;
   VkResult result;
            /* The buffer size and address need to be aligned in HW regs. Align the
   * size as early as possible so that we do all the allocation & addressing
   * correctly. */
            /* Compute total size of the thread trace BO for all SEs. */
   size = align64(sizeof(struct ac_sqtt_data_info) * max_se, 1 << SQTT_BUFFER_ALIGN_SHIFT);
            struct radeon_winsys_bo *bo = NULL;
   result = ws->buffer_create(ws, size, 4096, RADEON_DOMAIN_VRAM,
               device->sqtt.bo = bo;
   if (result != VK_SUCCESS)
            result = ws->buffer_make_resident(ws, device->sqtt.bo, true);
   if (result != VK_SUCCESS)
            device->sqtt.ptr = ws->buffer_map(device->sqtt.bo);
   if (!device->sqtt.ptr)
               }
      static void
   radv_sqtt_finish_bo(struct radv_device *device)
   {
               if (unlikely(device->sqtt.bo)) {
      ws->buffer_make_resident(ws, device->sqtt.bo, false);
         }
      static VkResult
   radv_register_queue(struct radv_device *device, struct radv_queue *queue)
   {
      struct ac_sqtt *sqtt = &device->sqtt;
   struct rgp_queue_info *queue_info = &sqtt->rgp_queue_info;
            record = malloc(sizeof(struct rgp_queue_info_record));
   if (!record)
            record->queue_id = (uintptr_t)queue;
   record->queue_context = (uintptr_t)queue->hw_ctx;
   if (queue->vk.queue_family_index == RADV_QUEUE_GENERAL) {
      record->hardware_info.queue_type = SQTT_QUEUE_TYPE_UNIVERSAL;
      } else {
      record->hardware_info.queue_type = SQTT_QUEUE_TYPE_COMPUTE;
               simple_mtx_lock(&queue_info->lock);
   list_addtail(&record->list, &queue_info->record);
   queue_info->record_count++;
               }
      static void
   radv_unregister_queue(struct radv_device *device, struct radv_queue *queue)
   {
      struct ac_sqtt *sqtt = &device->sqtt;
            /* Destroy queue info record. */
   simple_mtx_lock(&queue_info->lock);
   if (queue_info->record_count > 0) {
      list_for_each_entry_safe (struct rgp_queue_info_record, record, &queue_info->record, list) {
      if (record->queue_id == (uintptr_t)queue) {
      queue_info->record_count--;
   list_del(&record->list);
   free(record);
            }
      }
      static void
   radv_register_queues(struct radv_device *device, struct ac_sqtt *sqtt)
   {
      if (device->queue_count[RADV_QUEUE_GENERAL] == 1)
            for (uint32_t i = 0; i < device->queue_count[RADV_QUEUE_COMPUTE]; i++)
      }
      static void
   radv_unregister_queues(struct radv_device *device, struct ac_sqtt *sqtt)
   {
      if (device->queue_count[RADV_QUEUE_GENERAL] == 1)
            for (uint32_t i = 0; i < device->queue_count[RADV_QUEUE_COMPUTE]; i++)
      }
      bool
   radv_sqtt_init(struct radv_device *device)
   {
               /* Default buffer size set to 32MB per SE. */
            if (!radv_sqtt_init_bo(device))
            if (!radv_device_acquire_performance_counters(device))
                                 }
      void
   radv_sqtt_finish(struct radv_device *device)
   {
      struct ac_sqtt *sqtt = &device->sqtt;
                     for (unsigned i = 0; i < 2; i++) {
      if (device->sqtt.start_cs[i])
         if (device->sqtt.stop_cs[i])
                           }
      static bool
   radv_sqtt_resize_bo(struct radv_device *device)
   {
      /* Destroy the previous thread trace BO. */
            /* Double the size of the thread trace buffer per SE. */
            fprintf(stderr,
         "Failed to get the thread trace because the buffer "
            /* Re-create the thread trace BO. */
      }
      bool
   radv_begin_sqtt(struct radv_queue *queue)
   {
      struct radv_device *device = queue->device;
   enum radv_queue_family family = queue->state.qf;
   struct radeon_winsys *ws = device->ws;
   struct radeon_cmdbuf *cs;
            /* Destroy the previous start CS and create a new one. */
   if (device->sqtt.start_cs[family]) {
      ws->cs_destroy(device->sqtt.start_cs[family]);
               cs = ws->cs_create(ws, radv_queue_ring(queue), false);
   if (!cs)
                     switch (family) {
   case RADV_QUEUE_GENERAL:
      radeon_emit(cs, PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
   radeon_emit(cs, CC0_UPDATE_LOAD_ENABLES(1));
   radeon_emit(cs, CC1_UPDATE_SHADOW_ENABLES(1));
      case RADV_QUEUE_COMPUTE:
      radeon_emit(cs, PKT3(PKT3_NOP, 0, 0));
   radeon_emit(cs, 0);
      default:
      unreachable("Incorrect queue family");
               /* Make sure to wait-for-idle before starting SQTT. */
            /* Disable clock gating before starting SQTT. */
            /* Enable SQG events that collects thread trace data. */
                     if (device->spm.bo) {
      /* Enable all shader stages by default. */
                        /* Start SQTT. */
            if (device->spm.bo)
            result = ws->cs_finalize(cs);
   if (result != VK_SUCCESS) {
      ws->cs_destroy(cs);
                           }
      bool
   radv_end_sqtt(struct radv_queue *queue)
   {
      struct radv_device *device = queue->device;
   enum radv_queue_family family = queue->state.qf;
   struct radeon_winsys *ws = device->ws;
   struct radeon_cmdbuf *cs;
            /* Destroy the previous stop CS and create a new one. */
   if (queue->device->sqtt.stop_cs[family]) {
      ws->cs_destroy(device->sqtt.stop_cs[family]);
               cs = ws->cs_create(ws, radv_queue_ring(queue), false);
   if (!cs)
                     switch (family) {
   case RADV_QUEUE_GENERAL:
      radeon_emit(cs, PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
   radeon_emit(cs, CC0_UPDATE_LOAD_ENABLES(1));
   radeon_emit(cs, CC1_UPDATE_SHADOW_ENABLES(1));
      case RADV_QUEUE_COMPUTE:
      radeon_emit(cs, PKT3(PKT3_NOP, 0, 0));
   radeon_emit(cs, 0);
      default:
      unreachable("Incorrect queue family");
               /* Make sure to wait-for-idle before stopping SQTT. */
            if (device->spm.bo)
            /* Stop SQTT. */
                     /* Restore previous state by disabling SQG events. */
            /* Restore previous state by re-enabling clock gating. */
            result = ws->cs_finalize(cs);
   if (result != VK_SUCCESS) {
      ws->cs_destroy(cs);
                           }
      bool
   radv_get_sqtt_trace(struct radv_queue *queue, struct ac_sqtt_trace *sqtt_trace)
   {
      struct radv_device *device = queue->device;
            if (!ac_sqtt_get_trace(&device->sqtt, rad_info, sqtt_trace)) {
      if (!radv_sqtt_resize_bo(device))
                        }
      void
   radv_reset_sqtt_trace(struct radv_device *device)
   {
      struct ac_sqtt *sqtt = &device->sqtt;
            /* Clear clock calibration records. */
   simple_mtx_lock(&clock_calibration->lock);
   list_for_each_entry_safe (struct rgp_clock_calibration_record, record, &clock_calibration->record, list) {
      clock_calibration->record_count--;
   list_del(&record->list);
      }
      }
      static VkResult
   radv_get_calibrated_timestamps(struct radv_device *device, uint64_t *cpu_timestamp, uint64_t *gpu_timestamp)
   {
      uint64_t timestamps[2];
   uint64_t max_deviation;
            const VkCalibratedTimestampInfoEXT timestamp_infos[2] = {{
                                                result =
         if (result != VK_SUCCESS)
            *cpu_timestamp = timestamps[0];
               }
      bool
   radv_sqtt_sample_clocks(struct radv_device *device)
   {
      uint64_t cpu_timestamp = 0, gpu_timestamp = 0;
            result = radv_get_calibrated_timestamps(device, &cpu_timestamp, &gpu_timestamp);
   if (result != VK_SUCCESS)
               }
