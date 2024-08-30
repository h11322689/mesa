   /*
   * Copyright © 2019 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "crocus_monitor.h"
      #include <xf86drm.h>
      #include "crocus_screen.h"
   #include "crocus_context.h"
   #include "crocus_perf.h"
      #include "perf/intel_perf.h"
   #include "perf/intel_perf_query.h"
   #include "perf/intel_perf_regs.h"
      struct crocus_monitor_object {
      int num_active_counters;
            size_t result_size;
               };
      int
   crocus_get_monitor_info(struct pipe_screen *pscreen, unsigned index,
         {
      const struct crocus_screen *screen = (struct crocus_screen *)pscreen;
   struct intel_perf_config *perf_cfg = screen->perf_cfg;
   assert(perf_cfg);
   if (!perf_cfg)
            if (!info) {
      /* return the number of metrics */
               struct intel_perf_query_counter_info *counter_info = &perf_cfg->counter_infos[index];
   struct intel_perf_query_info *query_info =
         struct intel_perf_query_counter *counter = counter_info->counter;
                     info->group_id = counter_info->location.group_idx;
   info->name = counter->name;
            if (counter->type == INTEL_PERF_COUNTER_TYPE_THROUGHPUT)
         else
         switch (counter->data_type) {
   case INTEL_PERF_COUNTER_DATA_TYPE_BOOL32:
   case INTEL_PERF_COUNTER_DATA_TYPE_UINT32:
      info->type = PIPE_DRIVER_QUERY_TYPE_UINT;
   uint64_t val =
      counter->oa_counter_max_uint64 ?
      info->max_value.u32 = (uint32_t)val;
      case INTEL_PERF_COUNTER_DATA_TYPE_UINT64:
      info->type = PIPE_DRIVER_QUERY_TYPE_UINT64;
   info->max_value.u64 =
      counter->oa_counter_max_uint64 ?
         case INTEL_PERF_COUNTER_DATA_TYPE_FLOAT:
   case INTEL_PERF_COUNTER_DATA_TYPE_DOUBLE:
      info->type = PIPE_DRIVER_QUERY_TYPE_FLOAT;
   info->max_value.f =
      counter->oa_counter_max_float ?
         default:
      assert(false);
               /* indicates that this is an OA query, not a pipeline statistics query */
   info->flags = PIPE_DRIVER_QUERY_FLAG_BATCH;
      }
      static bool
   crocus_monitor_init_metrics(struct crocus_screen *screen)
   {
      struct intel_perf_config *perf_cfg = intel_perf_new(screen);
   if (unlikely(!perf_cfg))
                                          }
      int
   crocus_get_monitor_group_info(struct pipe_screen *pscreen,
               {
      struct crocus_screen *screen = (struct crocus_screen *)pscreen;
   if (!screen->perf_cfg) {
      if (!crocus_monitor_init_metrics(screen))
                        if (!info) {
      /* return the count that can be queried */
               if (group_index >= perf_cfg->n_queries) {
      /* out of range */
                        info->name = query->name;
   info->max_active_queries = query->n_counters;
               }
      static void
   crocus_init_monitor_ctx(struct crocus_context *ice)
   {
               ice->perf_ctx = intel_perf_new_context(ice);
   if (unlikely(!ice->perf_ctx))
            struct intel_perf_context *perf_ctx = ice->perf_ctx;
   struct intel_perf_config *perf_cfg = screen->perf_cfg;
   intel_perf_init_context(perf_ctx,
                           perf_cfg,
   ice,
      }
      /* entry point for GenPerfMonitorsAMD */
   struct crocus_monitor_object *
   crocus_create_monitor_object(struct crocus_context *ice,
               {
      struct crocus_screen *screen = (struct crocus_screen *) ice->ctx.screen;
   struct intel_perf_config *perf_cfg = screen->perf_cfg;
            /* initialize perf context if this has not already been done.  This
   * function is the first entry point that carries the gl context.
   */
   if (ice->perf_ctx == NULL) {
         }
            assert(num_queries > 0);
   int query_index = query_types[0] - PIPE_QUERY_DRIVER_SPECIFIC;
   assert(query_index <= perf_cfg->n_counters);
            struct crocus_monitor_object *monitor =
         if (unlikely(!monitor))
            monitor->num_active_counters = num_queries;
   monitor->active_counters = calloc(num_queries, sizeof(int));
   if (unlikely(!monitor->active_counters))
            for (int i = 0; i < num_queries; ++i) {
      unsigned current_query = query_types[i];
            /* all queries must be in the same group */
   assert(current_query_index <= perf_cfg->n_counters);
   assert(perf_cfg->counter_infos[current_query_index].location.group_idx == group);
   monitor->active_counters[i] =
               /* create the intel_perf_query */
   query_obj = intel_perf_new_query(perf_ctx, group);
   if (unlikely(!query_obj))
            monitor->query = query_obj;
   monitor->result_size = perf_cfg->queries[group].data_size;
   monitor->result_buffer = calloc(1, monitor->result_size);
   if (unlikely(!monitor->result_buffer))
                  allocation_failure:
      if (monitor) {
      free(monitor->active_counters);
      }
   free(query_obj);
   free(monitor);
      }
      void
   crocus_destroy_monitor_object(struct pipe_context *ctx,
         {
               intel_perf_delete_query(ice->perf_ctx, monitor->query);
   free(monitor->result_buffer);
   monitor->result_buffer = NULL;
   free(monitor->active_counters);
   monitor->active_counters = NULL;
      }
      bool
   crocus_begin_monitor(struct pipe_context *ctx,
         {
      struct crocus_context *ice = (void *) ctx;
               }
      bool
   crocus_end_monitor(struct pipe_context *ctx,
         {
      struct crocus_context *ice = (void *) ctx;
            intel_perf_end_query(perf_ctx, monitor->query);
      }
      bool
   crocus_get_monitor_result(struct pipe_context *ctx,
                     {
      struct crocus_context *ice = (void *) ctx;
   struct intel_perf_context *perf_ctx = ice->perf_ctx;
            bool monitor_ready =
            if (!monitor_ready) {
      if (!wait)
                              unsigned bytes_written;
   intel_perf_get_query_data(perf_ctx, monitor->query, batch,
                     if (bytes_written != monitor->result_size)
            /* copy metrics into the batch result */
   for (int i = 0; i < monitor->num_active_counters; ++i) {
      int current_counter = monitor->active_counters[i];
   const struct intel_perf_query_info *info =
         const struct intel_perf_query_counter *counter =
         assert(intel_perf_query_counter_get_size(counter));
   switch (counter->data_type) {
   case INTEL_PERF_COUNTER_DATA_TYPE_UINT64:
      result[i].u64 = *(uint64_t*)(monitor->result_buffer + counter->offset);
      case INTEL_PERF_COUNTER_DATA_TYPE_FLOAT:
      result[i].f = *(float*)(monitor->result_buffer + counter->offset);
      case INTEL_PERF_COUNTER_DATA_TYPE_UINT32:
   case INTEL_PERF_COUNTER_DATA_TYPE_BOOL32:
      result[i].u64 = *(uint32_t*)(monitor->result_buffer + counter->offset);
      case INTEL_PERF_COUNTER_DATA_TYPE_DOUBLE: {
      double v = *(double*)(monitor->result_buffer + counter->offset);
   result[i].f = v;
      }
   default:
            }
      }
