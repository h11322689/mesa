   /*
   * Copyright 2020 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
         #include "si_pipe.h"
   #include "si_build_pm4.h"
      #include "ac_rgp.h"
   #include "ac_sqtt.h"
   #include "util/hash_table.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "tgsi/tgsi_from_mesa.h"
      static void
   si_emit_spi_config_cntl(struct si_context* sctx,
            static bool si_sqtt_init_bo(struct si_context *sctx) {
   unsigned max_se = sctx->screen->info.max_se;
   struct radeon_winsys *ws = sctx->ws;
   uint64_t size;
      /* The buffer size and address need to be aligned in HW regs. Align the
      * size as early as possible so that we do all the allocation & addressing
      sctx->sqtt->buffer_size =
            /* Compute total size of the thread trace BO for all SEs. */
   size = align64(sizeof(struct ac_sqtt_data_info) * max_se,
         size += sctx->sqtt->buffer_size * (uint64_t)max_se;
      sctx->sqtt->pipeline_bos = _mesa_hash_table_u64_create(NULL);
      sctx->sqtt->bo =
         ws->buffer_create(ws, size, 4096, RADEON_DOMAIN_VRAM,
         if (!sctx->sqtt->bo)
            return true;
   }
      static bool
   si_sqtt_se_is_disabled(const struct radeon_info *info, unsigned se)
   {
      /* FIXME: SQTT only works on SE0 for some unknown reasons. See RADV for the
   * solution */
   if (info->gfx_level == GFX11)
            /* No active CU on the SE means it is disabled. */
      }
      static void si_emit_sqtt_start(struct si_context *sctx,
               struct si_screen *sscreen = sctx->screen;
   uint32_t shifted_size = sctx->sqtt->buffer_size >> SQTT_BUFFER_ALIGN_SHIFT;
   unsigned max_se = sscreen->info.max_se;
      radeon_begin(cs);
      for (unsigned se = 0; se < max_se; se++) {
      uint64_t va = sctx->ws->buffer_get_virtual_address(sctx->sqtt->bo);
   uint64_t data_va =
                  if (si_sqtt_se_is_disabled(&sctx->screen->info, se))
            /* Target SEx and SH0. */
   radeon_set_uconfig_reg(R_030800_GRBM_GFX_INDEX,
                  /* Select the first active CUs */
            if (sctx->gfx_level >= GFX10) {
      uint32_t token_mask =
      V_008D18_REG_INCLUDE_SQDEC | V_008D18_REG_INCLUDE_SHDEC |
   V_008D18_REG_INCLUDE_GFXUDEC | V_008D18_REG_INCLUDE_CONTEXT |
      int wgp = first_active_cu / 2;
            /* Order seems important for the following 2 registers. */
   if (sctx->gfx_level >= GFX11) {
   /* Disable unsupported hw shader stages */
            radeon_set_uconfig_reg(R_0367A4_SQ_THREAD_TRACE_BUF0_SIZE,
                           radeon_set_uconfig_reg(R_0367B4_SQ_THREAD_TRACE_MASK,
                        radeon_set_uconfig_reg(
         R_0367B8_SQ_THREAD_TRACE_TOKEN_MASK,
   S_0367B8_REG_INCLUDE(token_mask) |
   } else {
   radeon_set_privileged_config_reg(
                  radeon_set_privileged_config_reg(R_008D00_SQ_THREAD_TRACE_BUF0_BASE,
            radeon_set_privileged_config_reg(
         R_008D14_SQ_THREAD_TRACE_MASK,
            radeon_set_privileged_config_reg(
         R_008D18_SQ_THREAD_TRACE_TOKEN_MASK,
   S_008D18_REG_INCLUDE(token_mask) |
            /* Should be emitted last (it enables thread traces). */
   uint32_t ctrl = S_008D1C_MODE(1) | S_008D1C_HIWATER(5) |
                  if (sctx->gfx_level == GFX10_3)
            ctrl |= S_008D1C_AUTO_FLUSH_MODE(
            switch (sctx->gfx_level) {
   case GFX10:
   case GFX10_3:
   ctrl |= S_008D1C_REG_STALL_EN(1) | S_008D1C_SPI_STALL_EN(1) |
         radeon_set_privileged_config_reg(R_008D1C_SQ_THREAD_TRACE_CTRL, ctrl);
   break;
   case GFX11:
   ctrl |= S_0367B0_SPI_STALL_EN(1) | S_0367B0_SQ_STALL_EN(1) |
         radeon_set_uconfig_reg(R_0367B0_SQ_THREAD_TRACE_CTRL, ctrl);
   break;
   default:
   assert(false);
      } else {
      /* Order seems important for the following 4 registers. */
   radeon_set_uconfig_reg(R_030CDC_SQ_THREAD_TRACE_BASE2,
                     radeon_set_uconfig_reg(R_030CC4_SQ_THREAD_TRACE_SIZE,
            radeon_set_uconfig_reg(R_030CD4_SQ_THREAD_TRACE_CTRL,
            uint32_t sqtt_mask = S_030CC8_CU_SEL(first_active_cu) |
                                 /* Trace all tokens and registers. */
   radeon_set_uconfig_reg(R_030CCC_SQ_THREAD_TRACE_TOKEN_MASK,
                        /* Enable SQTT perf counters for all CUs. */
   radeon_set_uconfig_reg(R_030CD0_SQ_THREAD_TRACE_PERF_MASK,
                           radeon_set_uconfig_reg(R_030CEC_SQ_THREAD_TRACE_HIWATER,
            if (sctx->gfx_level == GFX9) {
   /* Reset thread trace status errors. */
   radeon_set_uconfig_reg(R_030CE8_SQ_THREAD_TRACE_STATUS,
                  /* Enable the thread trace mode. */
   uint32_t sqtt_mode = S_030CD8_MASK_PS(1) | S_030CD8_MASK_VS(1) |
                        S_030CD8_MASK_GS(1) | S_030CD8_MASK_ES(1) |
               if (sctx->gfx_level == GFX9) {
   /* Count SQTT traffic in TCC perf counters. */
   sqtt_mode |= S_030CD8_TC_PERF_EN(1);
                  }
      /* Restore global broadcasting. */
   radeon_set_uconfig_reg(R_030800_GRBM_GFX_INDEX,
                        /* Start the thread trace with a different event based on the queue. */
   if (queue_family_index == AMD_IP_COMPUTE) {
      radeon_set_sh_reg(R_00B878_COMPUTE_THREAD_TRACE_ENABLE,
      } else {
      radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
      }
   radeon_end();
   }
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
      static void si_copy_sqtt_info_regs(struct si_context *sctx,
               const uint32_t *sqtt_info_regs = NULL;
      switch (sctx->gfx_level) {
   case GFX10_3:
   case GFX10:
      sqtt_info_regs = gfx10_sqtt_info_regs;
      case GFX11:
      sqtt_info_regs = gfx11_sqtt_info_regs;
      case GFX9:
      sqtt_info_regs = gfx9_sqtt_info_regs;
      default:
         }
      /* Get the VA where the info struct is stored for this SE. */
   uint64_t va = sctx->ws->buffer_get_virtual_address(sctx->sqtt->bo);
   uint64_t info_va = ac_sqtt_get_info_va(va, se_index);
      radeon_begin(cs);
      /* Copy back the info struct one DWORD at a time. */
   for (unsigned i = 0; i < 3; i++) {
      radeon_emit(PKT3(PKT3_COPY_DATA, 4, 0));
   radeon_emit(COPY_DATA_SRC_SEL(COPY_DATA_PERF) |
         radeon_emit(sqtt_info_regs[i] >> 2);
   radeon_emit(0); /* unused */
   radeon_emit((info_va + i * 4));
      }
      if (sctx->gfx_level == GFX11) {
      /* On GFX11, WPTR is incremented from the offset of the current buffer base
   * address and it needs to be subtracted to get the correct offset:
   *
   * 1) get the current buffer base address for this SE
   * 2) shift right by 5 bits because SQ_THREAD_TRACE_WPTR is 32-byte aligned
   * 3) mask off the higher 3 bits because WPTR.OFFSET is 29 bits
   */
   uint64_t data_va =
         uint64_t shifted_data_va = (data_va >> 5);
            radeon_emit(PKT3(PKT3_ATOMIC_MEM, 7, 0));
   radeon_emit(ATOMIC_OP(TC_OP_ATOMIC_SUB_32));
   radeon_emit(info_va);
   radeon_emit(info_va >> 32);
   radeon_emit(init_wptr_value);
   radeon_emit(init_wptr_value >> 32);
   radeon_emit(0);
   radeon_emit(0);
      }
      radeon_end();
   }
      static void si_emit_sqtt_stop(struct si_context *sctx, struct radeon_cmdbuf *cs,
         unsigned max_se = sctx->screen->info.max_se;
      radeon_begin(cs);
      /* Stop the thread trace with a different event based on the queue. */
   if (queue_family_index == AMD_IP_COMPUTE) {
      radeon_set_sh_reg(R_00B878_COMPUTE_THREAD_TRACE_ENABLE,
      } else {
      radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
      }
      radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
   radeon_emit(EVENT_TYPE(V_028A90_THREAD_TRACE_FINISH) | EVENT_INDEX(0));
   radeon_end();
      if (sctx->screen->info.has_sqtt_rb_harvest_bug) {
      /* Some chips with disabled RBs should wait for idle because FINISH_DONE
   * doesn't work. */
   sctx->flags |= SI_CONTEXT_FLUSH_AND_INV_CB | SI_CONTEXT_FLUSH_AND_INV_DB |
            }
      for (unsigned se = 0; se < max_se; se++) {
      if (si_sqtt_se_is_disabled(&sctx->screen->info, se))
                     /* Target SEi and SH0. */
   radeon_set_uconfig_reg(R_030800_GRBM_GFX_INDEX,
                  if (sctx->gfx_level >= GFX10) {
      uint32_t tt_status_reg = sctx->gfx_level >= GFX11
               if (!sctx->screen->info.has_sqtt_rb_harvest_bug) {
   /* Make sure to wait for the trace buffer. */
   radeon_emit(PKT3(PKT3_WAIT_REG_MEM, 5, 0));
   radeon_emit(WAIT_REG_MEM_NOT_EQUAL); /* wait until the register is equal
         radeon_emit(tt_status_reg >> 2);     /* register */
   radeon_emit(0);
   radeon_emit(0); /* reference value */
   radeon_emit(sctx->gfx_level >= GFX11
               radeon_emit(4);                           /* poll interval */
            /* Disable the thread trace mode. */
   if (sctx->gfx_level >= GFX11)
   radeon_set_uconfig_reg(R_0367B0_SQ_THREAD_TRACE_CTRL, S_008D1C_MODE(0));
   else
   radeon_set_privileged_config_reg(R_008D1C_SQ_THREAD_TRACE_CTRL,
            /* Wait for thread trace completion. */
   radeon_emit(PKT3(PKT3_WAIT_REG_MEM, 5, 0));
   radeon_emit(WAIT_REG_MEM_EQUAL); /* wait until the register is equal to
         radeon_emit(tt_status_reg >> 2); /* register */
   radeon_emit(0);
   radeon_emit(0); /* reference value */
   radeon_emit(sctx->gfx_level >= GFX11 ? ~C_0367D0_BUSY
            } else {
      /* Disable the thread trace mode. */
            /* Wait for thread trace completion. */
   radeon_emit(PKT3(PKT3_WAIT_REG_MEM, 5, 0));
   radeon_emit(WAIT_REG_MEM_EQUAL); /* wait until the register is equal to
         radeon_emit(R_030CE8_SQ_THREAD_TRACE_STATUS >> 2); /* register */
   radeon_emit(0);
   radeon_emit(0);              /* reference value */
   radeon_emit(~C_030CE8_BUSY); /* mask */
      }
               }
      /* Restore global broadcasting. */
   radeon_begin_again(cs);
   radeon_set_uconfig_reg(R_030800_GRBM_GFX_INDEX,
                     radeon_end();
   }
      static void si_sqtt_start(struct si_context *sctx, int family,
         struct radeon_winsys *ws = sctx->ws;
      radeon_begin(cs);
      switch (family) {
   case AMD_IP_GFX:
      radeon_emit(PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
   radeon_emit(CC0_UPDATE_LOAD_ENABLES(1));
   radeon_emit(CC1_UPDATE_SHADOW_ENABLES(1));
      case AMD_IP_COMPUTE:
      radeon_emit(PKT3(PKT3_NOP, 0, 0));
   radeon_emit(0);
      }
   radeon_end();
      ws->cs_add_buffer(cs, sctx->sqtt->bo, RADEON_USAGE_READWRITE,
         if (sctx->spm.bo)
      ws->cs_add_buffer(cs, sctx->spm.bo, RADEON_USAGE_READWRITE,
         si_cp_dma_wait_for_idle(sctx, cs);
      /* Make sure to wait-for-idle before starting SQTT. */
   sctx->flags |= SI_CONTEXT_PS_PARTIAL_FLUSH | SI_CONTEXT_CS_PARTIAL_FLUSH |
                     sctx->emit_cache_flush(sctx, cs);
      si_inhibit_clockgating(sctx, cs, true);
      /* Enable SQG events that collects thread trace data. */
   si_emit_spi_config_cntl(sctx, cs, true);
      if (sctx->spm.bo) {
      si_pc_emit_spm_reset(cs);
   si_pc_emit_shaders(cs, 0x7f);
      }
      si_emit_sqtt_start(sctx, cs, family);
      if (sctx->spm.bo)
         }
      static void si_sqtt_stop(struct si_context *sctx, int family,
         struct radeon_winsys *ws = sctx->ws;
      radeon_begin(cs);
      switch (family) {
   case AMD_IP_GFX:
      radeon_emit(PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
   radeon_emit(CC0_UPDATE_LOAD_ENABLES(1));
   radeon_emit(CC1_UPDATE_SHADOW_ENABLES(1));
      case AMD_IP_COMPUTE:
      radeon_emit(PKT3(PKT3_NOP, 0, 0));
   radeon_emit(0);
      }
   radeon_end();
      ws->cs_add_buffer(cs, sctx->sqtt->bo, RADEON_USAGE_READWRITE,
            if (sctx->spm.bo)
      ws->cs_add_buffer(cs, sctx->spm.bo, RADEON_USAGE_READWRITE,
         si_cp_dma_wait_for_idle(sctx, cs);
      if (sctx->spm.bo)
      si_pc_emit_spm_stop(cs, sctx->screen->info.never_stop_sq_perf_counters,
         /* Make sure to wait-for-idle before stopping SQTT. */
   sctx->flags |= SI_CONTEXT_PS_PARTIAL_FLUSH | SI_CONTEXT_CS_PARTIAL_FLUSH |
                     sctx->emit_cache_flush(sctx, cs);
      si_emit_sqtt_stop(sctx, cs, family);
      if (sctx->spm.bo)
            /* Restore previous state by disabling SQG events. */
   si_emit_spi_config_cntl(sctx, cs, false);
      si_inhibit_clockgating(sctx, cs, false);
   }
      static void si_sqtt_init_cs(struct si_context *sctx) {
   struct radeon_winsys *ws = sctx->ws;
      /* Thread trace start CS (only handles AMD_IP_GFX). */
   sctx->sqtt->start_cs[AMD_IP_GFX] = CALLOC_STRUCT(radeon_cmdbuf);
   if (!ws->cs_create(sctx->sqtt->start_cs[AMD_IP_GFX], sctx->ctx, AMD_IP_GFX,
            free(sctx->sqtt->start_cs[AMD_IP_GFX]);
   sctx->sqtt->start_cs[AMD_IP_GFX] = NULL;
      }
      si_sqtt_start(sctx, AMD_IP_GFX, sctx->sqtt->start_cs[AMD_IP_GFX]);
      /* Thread trace stop CS. */
   sctx->sqtt->stop_cs[AMD_IP_GFX] = CALLOC_STRUCT(radeon_cmdbuf);
   if (!ws->cs_create(sctx->sqtt->stop_cs[AMD_IP_GFX], sctx->ctx, AMD_IP_GFX,
            free(sctx->sqtt->start_cs[AMD_IP_GFX]);
   sctx->sqtt->start_cs[AMD_IP_GFX] = NULL;
   free(sctx->sqtt->stop_cs[AMD_IP_GFX]);
   sctx->sqtt->stop_cs[AMD_IP_GFX] = NULL;
      }
      si_sqtt_stop(sctx, AMD_IP_GFX, sctx->sqtt->stop_cs[AMD_IP_GFX]);
   }
      static void si_begin_sqtt(struct si_context *sctx, struct radeon_cmdbuf *rcs) {
   struct radeon_cmdbuf *cs = sctx->sqtt->start_cs[AMD_IP_GFX];
   sctx->ws->cs_flush(cs, 0, NULL);
   }
      static void si_end_sqtt(struct si_context *sctx, struct radeon_cmdbuf *rcs) {
   struct radeon_cmdbuf *cs = sctx->sqtt->stop_cs[AMD_IP_GFX];
   sctx->ws->cs_flush(cs, 0, &sctx->last_sqtt_fence);
   }
      static bool si_get_sqtt_trace(struct si_context *sctx,
         unsigned max_se = sctx->screen->info.max_se;
      memset(sqtt, 0, sizeof(*sqtt));
      sctx->sqtt->ptr =
            if (!sctx->sqtt->ptr)
            if (!ac_sqtt_get_trace(sctx->sqtt, &sctx->screen->info, sqtt)) {
               for (unsigned se = 0; se < max_se; se++) {
      uint64_t info_offset = ac_sqtt_get_info_offset(se);
   void *info_ptr = sqtt_ptr + info_offset;
            if (si_sqtt_se_is_disabled(&sctx->screen->info, se))
            if (!ac_is_sqtt_complete(&sctx->screen->info, sctx->sqtt, info)) {
   uint32_t expected_size =
                  fprintf(stderr,
            "Failed to get the thread trace "
   "because the buffer is too small. The "
   "hardware needs %d KB but the "
      fprintf(stderr, "Please update the buffer size with "
         return false;
         }
      return true;
   }
      bool si_init_sqtt(struct si_context *sctx) {
   static bool warn_once = true;
   if (warn_once) {
      fprintf(stderr, "*************************************************\n");
   fprintf(stderr, "* WARNING: Thread trace support is experimental *\n");
   fprintf(stderr, "*************************************************\n");
      }
      sctx->sqtt = CALLOC_STRUCT(ac_sqtt);
      if (sctx->gfx_level < GFX8) {
      fprintf(stderr, "GPU hardware not supported: refer to "
                  }
      if (sctx->gfx_level > GFX11) {
      fprintf(stderr, "radeonsi: Thread trace is not supported "
            }
      /* Default buffer size set to 32MB per SE. */
   sctx->sqtt->buffer_size =
         sctx->sqtt->start_frame = 10;
      const char *trigger = getenv("AMD_THREAD_TRACE_TRIGGER");
   if (trigger) {
      sctx->sqtt->start_frame = atoi(trigger);
   if (sctx->sqtt->start_frame <= 0) {
      /* This isn't a frame number, must be a file */
   sctx->sqtt->trigger_file = strdup(trigger);
         }
      if (!si_sqtt_init_bo(sctx))
            ac_sqtt_init(sctx->sqtt);
      if (sctx->gfx_level >= GFX10 &&
            /* Limit SPM counters to GFX10 and GFX10_3 for now */
   ASSERTED bool r = si_spm_init(sctx);
      }
      si_sqtt_init_cs(sctx);
      sctx->sqtt_next_event = EventInvalid;
      return true;
   }
      void si_destroy_sqtt(struct si_context *sctx) {
   struct si_screen *sscreen = sctx->screen;
   struct pb_buffer *bo = sctx->sqtt->bo;
   radeon_bo_reference(sctx->screen->ws, &bo, NULL);
      if (sctx->sqtt->trigger_file)
            sscreen->ws->cs_destroy(sctx->sqtt->start_cs[AMD_IP_GFX]);
   sscreen->ws->cs_destroy(sctx->sqtt->stop_cs[AMD_IP_GFX]);
      struct rgp_pso_correlation *pso_correlation =
         struct rgp_loader_events *loader_events = &sctx->sqtt->rgp_loader_events;
   struct rgp_code_object *code_object = &sctx->sqtt->rgp_code_object;
   list_for_each_entry_safe(struct rgp_pso_correlation_record, record,
            list_del(&record->list);
   pso_correlation->record_count--;
      }
      list_for_each_entry_safe(struct rgp_loader_events_record, record,
            list_del(&record->list);
   loader_events->record_count--;
      }
      list_for_each_entry_safe(struct rgp_code_object_record, record,
            uint32_t mask = record->shader_stages_mask;
            /* Free the disassembly. */
   while (mask) {
      i = u_bit_scan(&mask);
      }
   list_del(&record->list);
   free(record);
      }
      ac_sqtt_finish(sctx->sqtt);
      hash_table_foreach(sctx->sqtt->pipeline_bos->table, entry) {
      struct si_sqtt_fake_pipeline *pipeline =
         si_resource_reference(&pipeline->bo, NULL);
      }
      free(sctx->sqtt);
   sctx->sqtt = NULL;
      if (sctx->spm.bo)
         }
      static uint64_t num_frames = 0;
      void si_handle_sqtt(struct si_context *sctx, struct radeon_cmdbuf *rcs) {
   /* Should we enable SQTT yet? */
   if (!sctx->sqtt_enabled) {
      bool frame_trigger = num_frames == sctx->sqtt->start_frame;
   bool file_trigger = false;
   if (sctx->sqtt->trigger_file &&
      access(sctx->sqtt->trigger_file, W_OK) == 0) {
   if (unlink(sctx->sqtt->trigger_file) == 0) {
   file_trigger = true;
   } else {
   /* Do not enable tracing if we cannot remove the file,
      * because by then we'll trace every frame.
      fprintf(
         stderr,
               if (frame_trigger || file_trigger) {
      /* Wait for last submission */
   sctx->ws->fence_wait(sctx->ws, sctx->last_gfx_fence,
            /* Start SQTT */
            sctx->sqtt_enabled = true;
            /* Force shader update to make sure si_sqtt_describe_pipeline_bind is
   * called for the current "pipeline".
   */
         } else {
               /* Stop SQTT */
   si_end_sqtt(sctx, rcs);
   sctx->sqtt_enabled = false;
   sctx->sqtt->start_frame = -1;
            /* Wait for SQTT to finish and read back the bo */
   if (sctx->ws->fence_wait(sctx->ws, sctx->last_sqtt_fence,
            si_get_sqtt_trace(sctx, &sqtt_trace)) {
            /* Map the SPM counter buffer */
   if (sctx->spm.bo) {
   sctx->spm.ptr = sctx->ws->buffer_map(
         ac_spm_get_trace(&sctx->spm, &spm_trace);
            ac_dump_rgp_capture(&sctx->screen->info, &sqtt_trace,
            if (sctx->spm.ptr)
      } else {
            }
      num_frames++;
   }
      static void si_emit_sqtt_userdata(struct si_context *sctx,
               const uint32_t *dwords = (uint32_t *)data;
      radeon_begin(cs);
      while (num_dwords > 0) {
               /* Without the perfctr bit the CP might not always pass the
   * write on correctly. */
   radeon_set_uconfig_reg_seq(R_030D08_SQ_THREAD_TRACE_USERDATA_2, count,
                     dwords += count;
      }
   radeon_end();
   }
      static void
   si_emit_spi_config_cntl(struct si_context* sctx,
         {
               if (sctx->gfx_level >= GFX9) {
      uint32_t spi_config_cntl = S_031100_GPR_WRITE_PRIORITY(0x2c688) |
                        if (sctx->gfx_level >= GFX10)
               } else {
      /* SPI_CONFIG_CNTL is a protected register on GFX6-GFX8. */
   radeon_set_privileged_config_reg(R_009100_SPI_CONFIG_CNTL,
            }
      }
      static uint32_t num_events = 0;
   void
   si_sqtt_write_event_marker(struct si_context* sctx, struct radeon_cmdbuf *rcs,
                           {
               marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_EVENT;
   marker.api_type = api_type == EventInvalid ? EventCmdDraw : api_type;
   marker.cmd_id = num_events++;
            if (vertex_offset_user_data == UINT_MAX ||
      instance_offset_user_data == UINT_MAX) {
   vertex_offset_user_data = 0;
               if (draw_index_user_data == UINT_MAX)
            marker.vertex_offset_reg_idx = vertex_offset_user_data;
   marker.instance_offset_reg_idx = instance_offset_user_data;
                        }
      void
   si_write_event_with_dims_marker(struct si_context* sctx, struct radeon_cmdbuf *rcs,
               {
               marker.event.identifier = RGP_SQTT_MARKER_IDENTIFIER_EVENT;
   marker.event.api_type = api_type;
   marker.event.cmd_id = num_events++;
   marker.event.cb_id = 0;
            marker.thread_x = x;
   marker.thread_y = y;
            si_emit_sqtt_userdata(sctx, rcs, &marker, sizeof(marker) / 4);
      }
      void
   si_sqtt_describe_barrier_start(struct si_context* sctx, struct radeon_cmdbuf *rcs)
   {
               marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_BARRIER_START;
   marker.cb_id = 0;
               }
      void
   si_sqtt_describe_barrier_end(struct si_context* sctx, struct radeon_cmdbuf *rcs,
         {
               marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_BARRIER_END;
            if (flags & SI_CONTEXT_VS_PARTIAL_FLUSH)
         if (flags & SI_CONTEXT_PS_PARTIAL_FLUSH)
         if (flags & SI_CONTEXT_CS_PARTIAL_FLUSH)
            if (flags & SI_CONTEXT_PFP_SYNC_ME)
            if (flags & SI_CONTEXT_INV_VCACHE)
         if (flags & SI_CONTEXT_INV_ICACHE)
         if (flags & SI_CONTEXT_INV_SCACHE)
         if (flags & SI_CONTEXT_INV_L2)
            if (flags & SI_CONTEXT_FLUSH_AND_INV_CB) {
      marker.inval_cb = true;
      }
   if (flags & SI_CONTEXT_FLUSH_AND_INV_DB) {
      marker.inval_db = true;
                  }
      void
   si_write_user_event(struct si_context* sctx, struct radeon_cmdbuf *rcs,
               {
      if (type == UserEventPop) {
      assert (str == NULL);
   struct rgp_sqtt_marker_user_event marker = { 0 };
   marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_USER_EVENT;
               } else {
      assert (str != NULL);
   struct rgp_sqtt_marker_user_event_with_length marker = { 0 };
   marker.user_event.identifier = RGP_SQTT_MARKER_IDENTIFIER_USER_EVENT;
   marker.user_event.data_type = type;
   len = MIN2(1024, len);
            uint8_t *buffer = alloca(sizeof(marker) + marker.length);
   memcpy(buffer, &marker, sizeof(marker));
   memcpy(buffer + sizeof(marker), str, len);
            si_emit_sqtt_userdata(sctx, rcs, buffer,
         }
      bool si_sqtt_pipeline_is_registered(struct ac_sqtt *sqtt,
            simple_mtx_lock(&sqtt->rgp_pso_correlation.lock);
   list_for_each_entry_safe(struct rgp_pso_correlation_record, record,
            if (record->pipeline_hash[0] == pipeline_hash) {
      simple_mtx_unlock(&sqtt->rgp_pso_correlation.lock);
         }
               }
      static enum rgp_hardware_stages
   si_sqtt_pipe_to_rgp_shader_stage(union si_shader_key* key, enum pipe_shader_type stage)
   {
      switch (stage) {
   case PIPE_SHADER_VERTEX:
      if (key->ge.as_ls)
         else if (key->ge.as_es)
         else if (key->ge.as_ngg)
         else
      case PIPE_SHADER_TESS_CTRL:
         case PIPE_SHADER_TESS_EVAL:
      if (key->ge.as_es)
         else if (key->ge.as_ngg)
         else
      case PIPE_SHADER_GEOMETRY:
         case PIPE_SHADER_FRAGMENT:
         case PIPE_SHADER_COMPUTE:
         default:
            }
      static bool
   si_sqtt_add_code_object(struct si_context* sctx,
               {
      struct rgp_code_object *code_object = &sctx->sqtt->rgp_code_object;
            record = calloc(1, sizeof(struct rgp_code_object_record));
   if (!record)
            record->shader_stages_mask = 0;
   record->num_shaders_combined = 0;
   record->pipeline_hash[0] = pipeline->code_hash;
            for (unsigned i = 0; i < PIPE_SHADER_TYPES; i++) {
      struct si_shader *shader;
            if (is_compute) {
      if (i != PIPE_SHADER_COMPUTE)
         shader = &sctx->cs_shader_state.program->shader;
      } else if (i != PIPE_SHADER_COMPUTE) {
      if (!sctx->shaders[i].cso || !sctx->shaders[i].current)
         shader = sctx->shaders[i].current;
      } else {
                  uint8_t *code = malloc(shader->binary.uploaded_code_size);
   if (!code) {
      free(record);
      }
            uint64_t va = pipeline->bo->gpu_address + pipeline->offset[i];
   unsigned gl_shader_stage = tgsi_processor_to_shader_stage(i);
   record->shader_data[gl_shader_stage].hash[0] = _mesa_hash_data(code, shader->binary.uploaded_code_size);
   record->shader_data[gl_shader_stage].hash[1] = record->shader_data[gl_shader_stage].hash[0];
   record->shader_data[gl_shader_stage].code_size = shader->binary.uploaded_code_size;
   record->shader_data[gl_shader_stage].code = code;
   record->shader_data[gl_shader_stage].vgpr_count = shader->config.num_vgprs;
   record->shader_data[gl_shader_stage].sgpr_count = shader->config.num_sgprs;
   record->shader_data[gl_shader_stage].base_address = va & 0xffffffffffff;
   record->shader_data[gl_shader_stage].elf_symbol_offset = 0;
   record->shader_data[gl_shader_stage].hw_stage = hw_stage;
   record->shader_data[gl_shader_stage].is_combined = false;
   record->shader_data[gl_shader_stage].scratch_memory_size = shader->config.scratch_bytes_per_wave;
            record->shader_stages_mask |= 1 << gl_shader_stage;
               simple_mtx_lock(&code_object->lock);
   list_addtail(&record->list, &code_object->record);
   code_object->record_count++;
               }
      bool
   si_sqtt_register_pipeline(struct si_context* sctx, struct si_sqtt_fake_pipeline *pipeline, bool is_compute)
   {
               bool result = ac_sqtt_add_pso_correlation(sctx->sqtt, pipeline->code_hash, pipeline->code_hash);
   if (!result)
            result = ac_sqtt_add_code_object_loader_event(
         if (!result)
               }
      void
   si_sqtt_describe_pipeline_bind(struct si_context* sctx,
               {
      struct rgp_sqtt_marker_pipeline_bind marker = {0};
            if (likely(!sctx->sqtt_enabled)) {
                  marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_BIND_PIPELINE;
   marker.cb_id = 0;
   marker.bind_point = bind_point;
   marker.api_pso_hash[0] = pipeline_hash;
               }
