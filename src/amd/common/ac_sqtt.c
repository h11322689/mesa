   /*
   * Copyright 2020 Advanced Micro Devices, Inc.
   * Copyright 2020 Valve Corporation
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_sqtt.h"
      #include "ac_gpu_info.h"
   #include "util/u_math.h"
   #include "util/os_time.h"
      uint64_t
   ac_sqtt_get_info_offset(unsigned se)
   {
         }
      uint64_t
   ac_sqtt_get_data_offset(const struct radeon_info *rad_info, const struct ac_sqtt *data, unsigned se)
   {
      unsigned max_se = rad_info->max_se;
            data_offset = align64(sizeof(struct ac_sqtt_data_info) * max_se, 1 << SQTT_BUFFER_ALIGN_SHIFT);
               }
      uint64_t
   ac_sqtt_get_info_va(uint64_t va, unsigned se)
   {
         }
      uint64_t
   ac_sqtt_get_data_va(const struct radeon_info *rad_info, const struct ac_sqtt *data, uint64_t va,
         {
         }
      void
   ac_sqtt_init(struct ac_sqtt *data)
   {
      list_inithead(&data->rgp_pso_correlation.record);
            list_inithead(&data->rgp_loader_events.record);
            list_inithead(&data->rgp_code_object.record);
            list_inithead(&data->rgp_clock_calibration.record);
            list_inithead(&data->rgp_queue_info.record);
            list_inithead(&data->rgp_queue_event.record);
      }
      void
   ac_sqtt_finish(struct ac_sqtt *data)
   {
      assert(data->rgp_pso_correlation.record_count == 0);
            assert(data->rgp_loader_events.record_count == 0);
            assert(data->rgp_code_object.record_count == 0);
            assert(data->rgp_clock_calibration.record_count == 0);
            assert(data->rgp_queue_info.record_count == 0);
            assert(data->rgp_queue_event.record_count == 0);
      }
      bool
   ac_is_sqtt_complete(const struct radeon_info *rad_info, const struct ac_sqtt *data,
         {
      if (rad_info->gfx_level >= GFX10) {
      /* GFX10 doesn't have THREAD_TRACE_CNTR but it reports the number of
   * dropped bytes per SE via THREAD_TRACE_DROPPED_CNTR. Though, this
   * doesn't seem reliable because it might still report non-zero even if
   * the SQTT buffer isn't full.
   *
   * The solution here is to compare the number of bytes written by the hw
   * (in units of 32 bytes) to the SQTT buffer size. If it's equal, that
   * means that the buffer is full and should be resized.
   */
               /* Otherwise, compare the current thread trace offset with the number
   * of written bytes.
   */
      }
      uint32_t
   ac_get_expected_buffer_size(struct radeon_info *rad_info, const struct ac_sqtt_data_info *info)
   {
      if (rad_info->gfx_level >= GFX10) {
      uint32_t dropped_cntr_per_se = info->gfx10_dropped_cntr / rad_info->max_se;
                  }
      bool
   ac_sqtt_add_pso_correlation(struct ac_sqtt *sqtt, uint64_t pipeline_hash, uint64_t api_hash)
   {
      struct rgp_pso_correlation *pso_correlation = &sqtt->rgp_pso_correlation;
            record = malloc(sizeof(struct rgp_pso_correlation_record));
   if (!record)
            record->api_pso_hash = api_hash;
   record->pipeline_hash[0] = pipeline_hash;
   record->pipeline_hash[1] = pipeline_hash;
            simple_mtx_lock(&pso_correlation->lock);
   list_addtail(&record->list, &pso_correlation->record);
   pso_correlation->record_count++;
               }
      bool
   ac_sqtt_add_code_object_loader_event(struct ac_sqtt *sqtt, uint64_t pipeline_hash,
         {
      struct rgp_loader_events *loader_events = &sqtt->rgp_loader_events;
            record = malloc(sizeof(struct rgp_loader_events_record));
   if (!record)
            record->loader_event_type = RGP_LOAD_TO_GPU_MEMORY;
   record->reserved = 0;
   record->base_address = base_address & 0xffffffffffff;
   record->code_object_hash[0] = pipeline_hash;
   record->code_object_hash[1] = pipeline_hash;
            simple_mtx_lock(&loader_events->lock);
   list_addtail(&record->list, &loader_events->record);
   loader_events->record_count++;
               }
      bool
   ac_sqtt_add_clock_calibration(struct ac_sqtt *sqtt, uint64_t cpu_timestamp, uint64_t gpu_timestamp)
   {
      struct rgp_clock_calibration *clock_calibration = &sqtt->rgp_clock_calibration;
            record = malloc(sizeof(struct rgp_clock_calibration_record));
   if (!record)
            record->cpu_timestamp = cpu_timestamp;
            simple_mtx_lock(&clock_calibration->lock);
   list_addtail(&record->list, &clock_calibration->record);
   clock_calibration->record_count++;
               }
      /* See https://gitlab.freedesktop.org/mesa/mesa/-/issues/5260
   * On some HW SQTT can hang if we're not in one of the profiling pstates. */
   bool
   ac_check_profile_state(const struct radeon_info *info)
   {
      char path[128];
   char data[128];
            if (!info->pci.valid)
            snprintf(path, sizeof(path),
                  FILE *f = fopen(path, "r");
   if (!f)
         n = fread(data, 1, sizeof(data) - 1, f);
   fclose(f);
   data[n] = 0;
      }
      union rgp_sqtt_marker_cb_id
   ac_sqtt_get_next_cmdbuf_id(struct ac_sqtt *data, enum amd_ip_type ip_type)
   {
               cb_id.global_cb_id.cb_index =
               }
      bool
   ac_sqtt_se_is_disabled(const struct radeon_info *info, unsigned se)
   {
      /* No active CU on the SE means it is disabled. */
      }
      uint32_t
   ac_sqtt_get_active_cu(const struct radeon_info *info, unsigned se)
   {
               if (info->gfx_level >= GFX11) {
      /* GFX11 seems to operate on the last active CU. */
      } else {
      /* Default to the first active CU. */
                  }
      bool
   ac_sqtt_get_trace(struct ac_sqtt *data, const struct radeon_info *info,
         {
      unsigned max_se = info->max_se;
                     for (unsigned se = 0; se < max_se; se++) {
      uint64_t info_offset = ac_sqtt_get_info_offset(se);
   uint64_t data_offset = ac_sqtt_get_data_offset(info, data, se);
   void *info_ptr = (uint8_t *)ptr + info_offset;
   void *data_ptr = (uint8_t *)ptr + data_offset;
   struct ac_sqtt_data_info *trace_info = (struct ac_sqtt_data_info *)info_ptr;
   struct ac_sqtt_data_se data_se = {0};
            if (ac_sqtt_se_is_disabled(info, se))
            if (!ac_is_sqtt_complete(info, data, trace_info))
            data_se.data_ptr = data_ptr;
   data_se.info = *trace_info;
            /* RGP seems to expect units of WGP on GFX10+. */
            sqtt_trace->traces[sqtt_trace->num_traces] = data_se;
               sqtt_trace->rgp_code_object = &data->rgp_code_object;
   sqtt_trace->rgp_loader_events = &data->rgp_loader_events;
   sqtt_trace->rgp_pso_correlation = &data->rgp_pso_correlation;
   sqtt_trace->rgp_queue_info = &data->rgp_queue_info;
   sqtt_trace->rgp_queue_event = &data->rgp_queue_event;
               }
      uint32_t
   ac_sqtt_get_shader_mask(const struct radeon_info *info)
   {
               if (info->gfx_level >= GFX11) {
      /* Disable unsupported hw shader stages */
                  }
