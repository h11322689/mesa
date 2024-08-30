   /*
   * Copyright © 2018 Intel Corporation
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
      #include "intel_perf.h"
   #include "intel_perf_mdapi.h"
   #include "intel_perf_private.h"
   #include "intel_perf_regs.h"
      #include "dev/intel_device_info.h"
      #include <drm-uapi/i915_drm.h>
         int
   intel_perf_query_result_write_mdapi(void *data, uint32_t data_size,
                     {
      switch (devinfo->ver) {
   case 7: {
               if (data_size < sizeof(*mdapi_data))
                     for (int i = 0; i < ARRAY_SIZE(mdapi_data->ACounters); i++)
            for (int i = 0; i < ARRAY_SIZE(mdapi_data->NOACounters); i++) {
      mdapi_data->NOACounters[i] =
               mdapi_data->PerfCounter1 = result->accumulator[query->perfcnt_offset + 0];
            mdapi_data->ReportsCount = result->reports_accumulated;
   mdapi_data->TotalTime =
         mdapi_data->CoreFrequency = result->gt_frequency[1];
   mdapi_data->CoreFrequencyChanged = result->gt_frequency[1] != result->gt_frequency[0];
   mdapi_data->SplitOccured = result->query_disjoint;
      }
   case 8: {
               if (data_size < sizeof(*mdapi_data))
            for (int i = 0; i < ARRAY_SIZE(mdapi_data->OaCntr); i++)
         for (int i = 0; i < ARRAY_SIZE(mdapi_data->NoaCntr); i++) {
      mdapi_data->NoaCntr[i] =
               mdapi_data->PerfCounter1 = result->accumulator[query->perfcnt_offset + 0];
            mdapi_data->ReportId = result->hw_id;
   mdapi_data->ReportsCount = result->reports_accumulated;
   mdapi_data->TotalTime =
         mdapi_data->BeginTimestamp =
         mdapi_data->GPUTicks = result->accumulator[1];
   mdapi_data->CoreFrequency = result->gt_frequency[1];
   mdapi_data->CoreFrequencyChanged = result->gt_frequency[1] != result->gt_frequency[0];
   mdapi_data->SliceFrequency =
         mdapi_data->UnsliceFrequency =
         mdapi_data->SplitOccured = result->query_disjoint;
      }
   case 9:
   case 11:
   case 12:{
               if (data_size < sizeof(*mdapi_data))
            for (int i = 0; i < ARRAY_SIZE(mdapi_data->OaCntr); i++)
         for (int i = 0; i < ARRAY_SIZE(mdapi_data->NoaCntr); i++) {
      mdapi_data->NoaCntr[i] =
               mdapi_data->PerfCounter1 = result->accumulator[query->perfcnt_offset + 0];
            mdapi_data->ReportId = result->hw_id;
   mdapi_data->ReportsCount = result->reports_accumulated;
   mdapi_data->TotalTime =
         mdapi_data->BeginTimestamp =
         mdapi_data->GPUTicks = result->accumulator[1];
   mdapi_data->CoreFrequency = result->gt_frequency[1];
   mdapi_data->CoreFrequencyChanged = result->gt_frequency[1] != result->gt_frequency[0];
   mdapi_data->SliceFrequency =
         mdapi_data->UnsliceFrequency =
         mdapi_data->SplitOccured = result->query_disjoint;
      }
   default:
            }
      void
   intel_perf_register_mdapi_statistic_query(struct intel_perf_config *perf_cfg,
         {
      if (!(devinfo->ver >= 7 && devinfo->ver <= 12))
            struct intel_perf_query_info *query =
            query->kind = INTEL_PERF_QUERY_TYPE_PIPELINE;
            /* The order has to match mdapi_pipeline_metrics. */
   intel_perf_query_add_basic_stat_reg(query, IA_VERTICES_COUNT,
         intel_perf_query_add_basic_stat_reg(query, IA_PRIMITIVES_COUNT,
         intel_perf_query_add_basic_stat_reg(query, VS_INVOCATION_COUNT,
         intel_perf_query_add_basic_stat_reg(query, GS_INVOCATION_COUNT,
         intel_perf_query_add_basic_stat_reg(query, GS_PRIMITIVES_COUNT,
         intel_perf_query_add_basic_stat_reg(query, CL_INVOCATION_COUNT,
         intel_perf_query_add_basic_stat_reg(query, CL_PRIMITIVES_COUNT,
         if (devinfo->verx10 == 75 || devinfo->ver == 8) {
      intel_perf_query_add_stat_reg(query, PS_INVOCATION_COUNT, 1, 4,
            } else {
      intel_perf_query_add_basic_stat_reg(query, PS_INVOCATION_COUNT,
      }
   intel_perf_query_add_basic_stat_reg(query, HS_INVOCATION_COUNT,
         intel_perf_query_add_basic_stat_reg(query, DS_INVOCATION_COUNT,
         if (devinfo->ver >= 7) {
      intel_perf_query_add_basic_stat_reg(query, CS_INVOCATION_COUNT,
               if (devinfo->ver >= 10) {
      /* Reuse existing CS invocation register until we can expose this new
   * one.
   */
   intel_perf_query_add_basic_stat_reg(query, CS_INVOCATION_COUNT,
                  }
      static void
   fill_mdapi_perf_query_counter(struct intel_perf_query_info *query,
                           {
                        counter->name = name;
   counter->desc = "Raw counter value";
   counter->type = INTEL_PERF_COUNTER_TYPE_RAW;
   counter->data_type = data_type;
                        }
      #define MDAPI_QUERY_ADD_COUNTER(query, struct_name, field_name, type_name) \
      fill_mdapi_perf_query_counter(query, #field_name,                    \
                        #define MDAPI_QUERY_ADD_ARRAY_COUNTER(ctx, query, struct_name, field_name, idx, type_name) \
      fill_mdapi_perf_query_counter(query,                                 \
                                 void
   intel_perf_register_mdapi_oa_query(struct intel_perf_config *perf,
         {
               /* MDAPI requires different structures for pretty much every generation
   * (right now we have definitions for gen 7 to 12).
   */
   if (!(devinfo->ver >= 7 && devinfo->ver <= 12))
            switch (devinfo->ver) {
   case 7: {
      query = intel_perf_append_query_info(perf, 1 + 45 + 16 + 7);
            struct gfx7_mdapi_metrics metric_data;
            MDAPI_QUERY_ADD_COUNTER(query, metric_data, TotalTime, UINT64);
   for (int i = 0; i < ARRAY_SIZE(metric_data.ACounters); i++) {
      MDAPI_QUERY_ADD_ARRAY_COUNTER(perf->queries, query,
      }
   for (int i = 0; i < ARRAY_SIZE(metric_data.NOACounters); i++) {
      MDAPI_QUERY_ADD_ARRAY_COUNTER(perf->queries, query,
      }
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, PerfCounter1, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, PerfCounter2, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, SplitOccured, BOOL32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, CoreFrequencyChanged, BOOL32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, CoreFrequency, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, ReportId, UINT32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, ReportsCount, UINT32);
      }
   case 8: {
      query = intel_perf_append_query_info(perf, 2 + 36 + 16 + 16);
            struct gfx8_mdapi_metrics metric_data;
            MDAPI_QUERY_ADD_COUNTER(query, metric_data, TotalTime, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, GPUTicks, UINT64);
   for (int i = 0; i < ARRAY_SIZE(metric_data.OaCntr); i++) {
      MDAPI_QUERY_ADD_ARRAY_COUNTER(perf->queries, query,
      }
   for (int i = 0; i < ARRAY_SIZE(metric_data.NoaCntr); i++) {
      MDAPI_QUERY_ADD_ARRAY_COUNTER(perf->queries, query,
      }
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, BeginTimestamp, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, Reserved1, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, Reserved2, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, Reserved3, UINT32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, OverrunOccured, BOOL32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, MarkerUser, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, MarkerDriver, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, SliceFrequency, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, UnsliceFrequency, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, PerfCounter1, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, PerfCounter2, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, SplitOccured, BOOL32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, CoreFrequencyChanged, BOOL32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, CoreFrequency, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, ReportId, UINT32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, ReportsCount, UINT32);
      }
   case 9:
   case 11:
   case 12: {
      query = intel_perf_append_query_info(perf, 2 + 36 + 16 + 16 + 16 + 2);
            struct gfx9_mdapi_metrics metric_data;
            MDAPI_QUERY_ADD_COUNTER(query, metric_data, TotalTime, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, GPUTicks, UINT64);
   for (int i = 0; i < ARRAY_SIZE(metric_data.OaCntr); i++) {
      MDAPI_QUERY_ADD_ARRAY_COUNTER(perf->queries, query,
      }
   for (int i = 0; i < ARRAY_SIZE(metric_data.NoaCntr); i++) {
      MDAPI_QUERY_ADD_ARRAY_COUNTER(perf->queries, query,
      }
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, BeginTimestamp, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, Reserved1, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, Reserved2, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, Reserved3, UINT32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, OverrunOccured, BOOL32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, MarkerUser, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, MarkerDriver, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, SliceFrequency, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, UnsliceFrequency, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, PerfCounter1, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, PerfCounter2, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, SplitOccured, BOOL32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, CoreFrequencyChanged, BOOL32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, CoreFrequency, UINT64);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, ReportId, UINT32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, ReportsCount, UINT32);
   for (int i = 0; i < ARRAY_SIZE(metric_data.UserCntr); i++) {
      MDAPI_QUERY_ADD_ARRAY_COUNTER(perf->queries, query,
      }
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, UserCntrCfgId, UINT32);
   MDAPI_QUERY_ADD_COUNTER(query, metric_data, Reserved4, UINT32);
      }
   default:
      unreachable("Unsupported gen");
               query->kind = INTEL_PERF_QUERY_TYPE_RAW;
   query->name = "Intel_Raw_Hardware_Counters_Set_0_Query";
            {
      /* Accumulation buffer offsets copied from an actual query... */
   const struct intel_perf_query_info *copy_query =
            query->gpu_time_offset = copy_query->gpu_time_offset;
   query->gpu_clock_offset = copy_query->gpu_clock_offset;
   query->a_offset = copy_query->a_offset;
   query->b_offset = copy_query->b_offset;
   query->c_offset = copy_query->c_offset;
         }
