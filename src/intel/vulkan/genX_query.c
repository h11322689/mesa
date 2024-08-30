   /*
   * Copyright © 2015 Intel Corporation
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
      #include <assert.h>
   #include <stdbool.h>
   #include <string.h>
   #include <unistd.h>
   #include <fcntl.h>
      #include "anv_private.h"
      #include "util/os_time.h"
      #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
      #include "ds/intel_tracepoints.h"
      #include "anv_internal_kernels.h"
      /* We reserve :
   *    - GPR 14 for perf queries
   *    - GPR 15 for conditional rendering
   */
   #define MI_BUILDER_NUM_ALLOC_GPRS 14
   #define MI_BUILDER_CAN_WRITE_BATCH true
   #define __gen_get_batch_dwords anv_batch_emit_dwords
   #define __gen_address_offset anv_address_add
   #define __gen_get_batch_address(b, a) anv_batch_address(b, a)
   #include "common/mi_builder.h"
   #include "perf/intel_perf.h"
   #include "perf/intel_perf_mdapi.h"
   #include "perf/intel_perf_regs.h"
      #include "vk_util.h"
      static struct anv_address
   anv_query_address(struct anv_query_pool *pool, uint32_t query)
   {
      return (struct anv_address) {
      .bo = pool->bo,
         }
      static void
   emit_query_mi_flush_availability(struct anv_cmd_buffer *cmd_buffer,
               {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_FLUSH_DW), flush) {
      flush.PostSyncOperation = WriteImmediateData;
   flush.Address = addr;
         }
      VkResult genX(CreateQueryPool)(
      VkDevice                                    _device,
   const VkQueryPoolCreateInfo*                pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   const struct anv_physical_device *pdevice = device->physical;
   const VkQueryPoolPerformanceCreateInfoKHR *perf_query_info = NULL;
   struct intel_perf_counter_pass *counter_pass;
   struct intel_perf_query_info **pass_query;
   uint32_t n_passes = 0;
   uint32_t data_offset = 0;
   VK_MULTIALLOC(ma);
                     /* Query pool slots are made up of some number of 64-bit values packed
   * tightly together. For most query types have the first 64-bit value is
   * the "available" bit which is 0 when the query is unavailable and 1 when
   * it is available. The 64-bit values that follow are determined by the
   * type of query.
   *
   * For performance queries, we have a requirement to align OA reports at
   * 64bytes so we put those first and have the "available" bit behind
   * together with some other counters.
   */
                     VkQueryPipelineStatisticFlags pipeline_statistics = 0;
   switch (pCreateInfo->queryType) {
   case VK_QUERY_TYPE_OCCLUSION:
      /* Occlusion queries have two values: begin and end. */
   uint64s_per_slot = 1 + 2;
      case VK_QUERY_TYPE_TIMESTAMP:
      /* Timestamps just have the one timestamp value */
   uint64s_per_slot = 1 + 1;
      case VK_QUERY_TYPE_PIPELINE_STATISTICS:
      pipeline_statistics = pCreateInfo->pipelineStatistics;
   /* We're going to trust this field implicitly so we need to ensure that
   * no unhandled extension bits leak in.
   */
            /* Statistics queries have a min and max for every statistic */
   uint64s_per_slot = 1 + 2 * util_bitcount(pipeline_statistics);
      case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      /* Transform feedback queries are 4 values, begin/end for
   * written/available.
   */
   uint64s_per_slot = 1 + 4;
      case VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL: {
      const struct intel_perf_query_field_layout *layout =
            uint64s_per_slot = 2; /* availability + marker */
   /* Align to the requirement of the layout */
   uint64s_per_slot = align(uint64s_per_slot,
         data_offset = uint64s_per_slot * sizeof(uint64_t);
   /* Add the query data for begin & end commands */
   uint64s_per_slot += 2 * DIV_ROUND_UP(layout->size, sizeof(uint64_t));
      }
   case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      const struct intel_perf_query_field_layout *layout =
            perf_query_info = vk_find_struct_const(pCreateInfo->pNext,
         n_passes = intel_perf_get_n_passes(pdevice->perf,
                     vk_multialloc_add(&ma, &counter_pass, struct intel_perf_counter_pass,
         vk_multialloc_add(&ma, &pass_query, struct intel_perf_query_info *,
         uint64s_per_slot = 1 /* availability */;
   /* Align to the requirement of the layout */
   uint64s_per_slot = align(uint64s_per_slot,
         data_offset = uint64s_per_slot * sizeof(uint64_t);
   /* Add the query data for begin & end commands */
   uint64s_per_slot += 2 * DIV_ROUND_UP(layout->size, sizeof(uint64_t));
   /* Multiply by the number of passes */
   uint64s_per_slot *= n_passes;
      }
   case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      /* Query has two values: begin and end. */
   uint64s_per_slot = 1 + 2;
   #if GFX_VERx10 >= 125
      case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
      uint64s_per_slot = 1 + 1 /* availability + size (PostbuildInfoCurrentSize, PostbuildInfoCompactedSize) */;
         case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR:
      uint64s_per_slot = 1 + 2 /* availability + size (PostbuildInfoSerializationDesc) */;
      #endif
      case VK_QUERY_TYPE_RESULT_STATUS_ONLY_KHR:
      uint64s_per_slot = 1;
      default:
                  if (!vk_multialloc_zalloc2(&ma, &device->vk.alloc, pAllocator,
                  vk_query_pool_init(&device->vk, &pool->vk, pCreateInfo);
            if (pool->vk.query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL) {
      pool->data_offset = data_offset;
      }
   else if (pool->vk.query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      pool->pass_size = pool->stride / n_passes;
   pool->data_offset = data_offset;
   pool->snapshot_size = (pool->pass_size - data_offset) / 2;
   pool->n_counters = perf_query_info->counterIndexCount;
   pool->counter_pass = counter_pass;
   intel_perf_get_counters_passes(pdevice->perf,
                     pool->n_passes = n_passes;
   pool->pass_query = pass_query;
   intel_perf_get_n_passes(pdevice->perf,
                                    /* For KHR_performance_query we need some space in the buffer for a small
   * batch updating ANV_PERF_QUERY_OFFSET_REG.
   */
   if (pool->vk.query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      pool->khr_perf_preamble_stride = 32;
   pool->khr_perf_preambles_offset = size;
               result = anv_device_alloc_bo(device, "query-pool", size,
                           if (result != VK_SUCCESS)
            if (pool->vk.query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      for (uint32_t p = 0; p < pool->n_passes; p++) {
      struct mi_builder b;
   struct anv_batch batch = {
      .start = pool->bo->map + khr_perf_query_preamble_offset(pool, p),
                     mi_builder_init(&b, device->info, &batch);
   mi_store(&b, mi_reg64(ANV_PERF_QUERY_OFFSET_REG),
                                       fail:
                  }
      void genX(DestroyQueryPool)(
      VkDevice                                    _device,
   VkQueryPool                                 _pool,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!pool)
            anv_device_release_bo(device, pool->bo);
      }
      /**
   * VK_KHR_performance_query layout  :
   *
   * --------------------------------------------
   * |       availability (8b)       | |        |
   * |-------------------------------| |        |
   * |       some padding (see       | |        |
   * | query_field_layout:alignment) | | Pass 0 |
   * |-------------------------------| |        |
   * |           query data          | |        |
   * | (2 * query_field_layout:size) | |        |
   * |-------------------------------|--        | Query 0
   * |       availability (8b)       | |        |
   * |-------------------------------| |        |
   * |       some padding (see       | |        |
   * | query_field_layout:alignment) | | Pass 1 |
   * |-------------------------------| |        |
   * |           query data          | |        |
   * | (2 * query_field_layout:size) | |        |
   * |-------------------------------|-----------
   * |       availability (8b)       | |        |
   * |-------------------------------| |        |
   * |       some padding (see       | |        |
   * | query_field_layout:alignment) | | Pass 0 |
   * |-------------------------------| |        |
   * |           query data          | |        |
   * | (2 * query_field_layout:size) | |        |
   * |-------------------------------|--        | Query 1
   * |               ...             | |        |
   * --------------------------------------------
   */
      static uint64_t
   khr_perf_query_availability_offset(struct anv_query_pool *pool, uint32_t query, uint32_t pass)
   {
         }
      static uint64_t
   khr_perf_query_data_offset(struct anv_query_pool *pool, uint32_t query, uint32_t pass, bool end)
   {
      return query * (uint64_t)pool->stride + pass * (uint64_t)pool->pass_size +
      }
      static struct anv_address
   khr_perf_query_availability_address(struct anv_query_pool *pool, uint32_t query, uint32_t pass)
   {
      return anv_address_add(
      (struct anv_address) { .bo = pool->bo, },
   }
      static struct anv_address
   khr_perf_query_data_address(struct anv_query_pool *pool, uint32_t query, uint32_t pass, bool end)
   {
      return anv_address_add(
      (struct anv_address) { .bo = pool->bo, },
   }
      static bool
   khr_perf_query_ensure_relocs(struct anv_cmd_buffer *cmd_buffer)
   {
      if (anv_batch_has_error(&cmd_buffer->batch))
            if (cmd_buffer->self_mod_locations)
            struct anv_device *device = cmd_buffer->device;
            cmd_buffer->self_mod_locations =
      vk_alloc(&cmd_buffer->vk.pool->alloc,
               if (!cmd_buffer->self_mod_locations) {
      anv_batch_set_error(&cmd_buffer->batch, VK_ERROR_OUT_OF_HOST_MEMORY);
                  }
      /**
   * VK_INTEL_performance_query layout :
   *
   * ---------------------------------
   * |       availability (8b)       |
   * |-------------------------------|
   * |          marker (8b)          |
   * |-------------------------------|
   * |       some padding (see       |
   * | query_field_layout:alignment) |
   * |-------------------------------|
   * |           query data          |
   * | (2 * query_field_layout:size) |
   * ---------------------------------
   */
      static uint32_t
   intel_perf_marker_offset(void)
   {
         }
      static uint32_t
   intel_perf_query_data_offset(struct anv_query_pool *pool, bool end)
   {
         }
      static void
   cpu_write_query_result(void *dst_slot, VkQueryResultFlags flags,
         {
      if (flags & VK_QUERY_RESULT_64_BIT) {
      uint64_t *dst64 = dst_slot;
      } else {
      uint32_t *dst32 = dst_slot;
         }
      static void *
   query_slot(struct anv_query_pool *pool, uint32_t query)
   {
         }
      static bool
   query_is_available(struct anv_query_pool *pool, uint32_t query)
   {
      if (pool->vk.query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      for (uint32_t p = 0; p < pool->n_passes; p++) {
      volatile uint64_t *slot =
         if (!slot[0])
      }
                  }
      static VkResult
   wait_for_available(struct anv_device *device,
         {
      /* By default we leave a 2s timeout before declaring the device lost. */
   uint64_t rel_timeout = 2 * NSEC_PER_SEC;
   if (pool->vk.query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      /* With performance queries, there is an additional 500us reconfiguration
   * time in i915.
   */
   rel_timeout += 500 * 1000;
   /* Additionally a command buffer can be replayed N times to gather data
   * for each of the metric sets to capture all the counters requested.
   */
      }
            while (os_time_get_nano() < abs_timeout_ns) {
      if (query_is_available(pool, query))
         VkResult status = vk_device_check_status(&device->vk);
   if (status != VK_SUCCESS)
                  }
      VkResult genX(GetQueryPoolResults)(
      VkDevice                                    _device,
   VkQueryPool                                 queryPool,
   uint32_t                                    firstQuery,
   uint32_t                                    queryCount,
   size_t                                      dataSize,
   void*                                       pData,
   VkDeviceSize                                stride,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
               #if GFX_VERx10 >= 125
      pool->vk.query_type == VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR ||
   pool->vk.query_type == VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR ||
   pool->vk.query_type == VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR ||
      #endif
      pool->vk.query_type == VK_QUERY_TYPE_OCCLUSION ||
   pool->vk.query_type == VK_QUERY_TYPE_PIPELINE_STATISTICS ||
   pool->vk.query_type == VK_QUERY_TYPE_TIMESTAMP ||
   pool->vk.query_type == VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT ||
   pool->vk.query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR ||
   pool->vk.query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL ||
   pool->vk.query_type == VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT ||
            if (vk_device_is_lost(&device->vk))
            if (pData == NULL)
                     VkResult status = VK_SUCCESS;
   for (uint32_t i = 0; i < queryCount; i++) {
               if (!available && (flags & VK_QUERY_RESULT_WAIT_BIT)) {
      status = wait_for_available(device, pool, firstQuery + i);
   if (status != VK_SUCCESS) {
                              /* From the Vulkan 1.0.42 spec:
   *
   *    "If VK_QUERY_RESULT_WAIT_BIT and VK_QUERY_RESULT_PARTIAL_BIT are
   *    both not set then no result values are written to pData for
   *    queries that are in the unavailable state at the time of the call,
   *    and vkGetQueryPoolResults returns VK_NOT_READY. However,
   *    availability state is still written to pData for those queries if
   *    VK_QUERY_RESULT_WITH_AVAILABILITY_BIT is set."
   *
   * From VK_KHR_performance_query :
   *
   *    "VK_QUERY_RESULT_PERFORMANCE_QUERY_RECORDED_COUNTERS_BIT_KHR specifies
   *     that the result should contain the number of counters that were recorded
   *     into a query pool of type ename:VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR"
   */
            uint32_t idx = 0;
   switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
   case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT: {
      uint64_t *slot = query_slot(pool, firstQuery + i);
   if (write_results) {
      /* From the Vulkan 1.2.132 spec:
   *
   *    "If VK_QUERY_RESULT_PARTIAL_BIT is set,
   *    VK_QUERY_RESULT_WAIT_BIT is not set, and the query’s status
   *    is unavailable, an intermediate result value between zero and
   *    the final result value is written to pData for that query."
   */
   uint64_t result = available ? slot[2] - slot[1] : 0;
      }
   idx++;
               case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
      uint64_t *slot = query_slot(pool, firstQuery + i);
   uint32_t statistics = pool->vk.pipeline_statistics;
   while (statistics) {
      UNUSED uint32_t stat = u_bit_scan(&statistics);
   if (write_results) {
      uint64_t result = slot[idx * 2 + 2] - slot[idx * 2 + 1];
      }
      }
   assert(idx == util_bitcount(pool->vk.pipeline_statistics));
               case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT: {
      uint64_t *slot = query_slot(pool, firstQuery + i);
   if (write_results)
         idx++;
   if (write_results)
         idx++;
         #if GFX_VERx10 >= 125
         case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR: {
      uint64_t *slot = query_slot(pool, firstQuery + i);
   if (write_results)
         idx++;
               case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR: {
      uint64_t *slot = query_slot(pool, firstQuery + i);
   if (write_results)
         idx++;
      #endif
            case VK_QUERY_TYPE_TIMESTAMP: {
      uint64_t *slot = query_slot(pool, firstQuery + i);
   if (write_results)
         idx++;
               case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      const struct anv_physical_device *pdevice = device->physical;
   assert((flags & (VK_QUERY_RESULT_WITH_AVAILABILITY_BIT |
         for (uint32_t p = 0; p < pool->n_passes; p++) {
      const struct intel_perf_query_info *query = pool->pass_query[p];
   struct intel_perf_query_result result;
   intel_perf_query_result_clear(&result);
   intel_perf_query_result_accumulate_fields(&result, query,
                        }
               case VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL: {
      if (!write_results)
         const void *query_data = query_slot(pool, firstQuery + i);
   const struct intel_perf_query_info *query = &device->physical->perf->queries[0];
   struct intel_perf_query_result result;
   intel_perf_query_result_clear(&result);
   intel_perf_query_result_accumulate_fields(&result, query,
                     intel_perf_query_result_write_mdapi(pData, stride,
               const uint64_t *marker = query_data + intel_perf_marker_offset();
   intel_perf_query_mdapi_write_marker(pData, stride, device->info, *marker);
               case VK_QUERY_TYPE_RESULT_STATUS_ONLY_KHR:
      if (!write_results)
         const uint32_t *query_data = query_slot(pool, firstQuery + i);
   uint32_t result = available ? *query_data : 0;
               default:
                  if (!write_results)
            if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT)
            pData += stride;
   if (pData >= data_end)
                  }
      static void
   emit_ps_depth_count(struct anv_cmd_buffer *cmd_buffer,
         {
      cmd_buffer->state.pending_pipe_bits |= ANV_PIPE_POST_SYNC_BIT;
            bool cs_stall_needed = (GFX_VER == 9 && cmd_buffer->device->info->gt == 4);
   genx_batch_emit_pipe_control_write
      (&cmd_buffer->batch, cmd_buffer->device->info, WritePSDepthCount, addr, 0,
   }
      static void
   emit_query_mi_availability(struct mi_builder *b,
               {
         }
      static void
   emit_query_pc_availability(struct anv_cmd_buffer *cmd_buffer,
               {
      cmd_buffer->state.pending_pipe_bits |= ANV_PIPE_POST_SYNC_BIT;
            genx_batch_emit_pipe_control_write
      (&cmd_buffer->batch, cmd_buffer->device->info, WriteImmediateData, addr,
   }
      /**
   * Goes through a series of consecutive query indices in the given pool
   * setting all element values to 0 and emitting them as available.
   */
   static void
   emit_zero_queries(struct anv_cmd_buffer *cmd_buffer,
               {
      switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
   case VK_QUERY_TYPE_TIMESTAMP:
      /* These queries are written with a PIPE_CONTROL so clear them using the
   * PIPE_CONTROL as well so we don't have to synchronize between 2 types
   * of operations.
   */
   assert((pool->stride % 8) == 0);
   for (uint32_t i = 0; i < num_queries; i++) {
                     for (uint32_t qword = 1; qword < (pool->stride / 8); qword++) {
      emit_query_pc_availability(cmd_buffer,
            }
      }
         case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
   case VK_QUERY_TYPE_PIPELINE_STATISTICS:
   case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      for (uint32_t i = 0; i < num_queries; i++) {
      struct anv_address slot_addr =
         mi_memset(b, anv_address_add(slot_addr, 8), 0, pool->stride - 8);
      }
         case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      for (uint32_t i = 0; i < num_queries; i++) {
      for (uint32_t p = 0; p < pool->n_passes; p++) {
      mi_memset(b, khr_perf_query_data_address(pool, first_index + i, p, false),
         emit_query_mi_availability(b,
               }
               case VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL:
      for (uint32_t i = 0; i < num_queries; i++) {
      struct anv_address slot_addr =
         mi_memset(b, anv_address_add(slot_addr, 8), 0, pool->stride - 8);
      }
         default:
            }
      void genX(CmdResetQueryPool)(
      VkCommandBuffer                             commandBuffer,
   VkQueryPool                                 queryPool,
   uint32_t                                    firstQuery,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_query_pool, pool, queryPool);
            /* Temporarily disable on MTL until we understand why some tests hang.
   */
   if (queryCount >= pdevice->instance->query_clear_with_blorp_threshold &&
      !intel_device_info_is_mtl(cmd_buffer->device->info)) {
            anv_cmd_buffer_fill_area(cmd_buffer,
                        cmd_buffer->state.queries.clear_bits =
      (cmd_buffer->queue_family->queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 ?
               trace_intel_end_query_clear_blorp(&cmd_buffer->trace, queryCount);
                        switch (pool->vk.query_type) {
      #if GFX_VERx10 >= 125
      case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
      #endif
         for (uint32_t i = 0; i < queryCount; i++) {
      emit_query_pc_availability(cmd_buffer,
            }
         case VK_QUERY_TYPE_TIMESTAMP: {
      for (uint32_t i = 0; i < queryCount; i++) {
      emit_query_pc_availability(cmd_buffer,
                     /* Add a CS stall here to make sure the PIPE_CONTROL above has
   * completed. Otherwise some timestamps written later with MI_STORE_*
   * commands might race with the PIPE_CONTROL in the loop above.
   */
   anv_add_pending_pipe_bits(cmd_buffer, ANV_PIPE_CS_STALL_BIT,
         genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);
               case VK_QUERY_TYPE_PIPELINE_STATISTICS:
   case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
   case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT: {
      struct mi_builder b;
            for (uint32_t i = 0; i < queryCount; i++)
                     case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      struct mi_builder b;
            for (uint32_t i = 0; i < queryCount; i++) {
      for (uint32_t p = 0; p < pool->n_passes; p++) {
      emit_query_mi_availability(
      &b,
   khr_perf_query_availability_address(pool, firstQuery + i, p),
      }
               case VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL: {
      struct mi_builder b;
            for (uint32_t i = 0; i < queryCount; i++)
            }
   case VK_QUERY_TYPE_RESULT_STATUS_ONLY_KHR:
      for (uint32_t i = 0; i < queryCount; i++)
            default:
                     }
      void genX(ResetQueryPool)(
      VkDevice                                    _device,
   VkQueryPool                                 queryPool,
   uint32_t                                    firstQuery,
      {
               for (uint32_t i = 0; i < queryCount; i++) {
      if (pool->vk.query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      for (uint32_t p = 0; p < pool->n_passes; p++) {
      uint64_t *pass_slot = pool->bo->map +
               } else {
      uint64_t *slot = query_slot(pool, firstQuery + i);
            }
      static const uint32_t vk_pipeline_stat_to_reg[] = {
      GENX(IA_VERTICES_COUNT_num),
   GENX(IA_PRIMITIVES_COUNT_num),
   GENX(VS_INVOCATION_COUNT_num),
   GENX(GS_INVOCATION_COUNT_num),
   GENX(GS_PRIMITIVES_COUNT_num),
   GENX(CL_INVOCATION_COUNT_num),
   GENX(CL_PRIMITIVES_COUNT_num),
   GENX(PS_INVOCATION_COUNT_num),
   GENX(HS_INVOCATION_COUNT_num),
   GENX(DS_INVOCATION_COUNT_num),
      };
      static void
   emit_pipeline_stat(struct mi_builder *b, uint32_t stat,
         {
      STATIC_ASSERT(ANV_PIPELINE_STATISTICS_MASK ==
            assert(stat < ARRAY_SIZE(vk_pipeline_stat_to_reg));
      }
      static void
   emit_xfb_query(struct mi_builder *b, uint32_t stream,
         {
               mi_store(b, mi_mem64(anv_address_add(addr, 0)),
         mi_store(b, mi_mem64(anv_address_add(addr, 16)),
      }
      static void
   emit_perf_intel_query(struct anv_cmd_buffer *cmd_buffer,
                           {
      const struct intel_perf_query_field_layout *layout =
         struct anv_address data_addr =
            for (uint32_t f = 0; f < layout->n_fields; f++) {
      const struct intel_perf_query_field *field =
            switch (field->type) {
   case INTEL_PERF_QUERY_FIELD_TYPE_MI_RPC:
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_REPORT_PERF_COUNT), rpc) {
                     case INTEL_PERF_QUERY_FIELD_TYPE_SRM_PERFCNT:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_RPSTAT:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_A:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_B:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_C: {
      struct anv_address addr = anv_address_add(data_addr, field->location);
   struct mi_value src = field->size == 8 ?
      mi_reg64(field->mmio_offset) :
      struct mi_value dst = field->size == 8 ?
         mi_store(b, dst, src);
               default:
      unreachable("Invalid query field");
            }
      static void
   emit_query_clear_flush(struct anv_cmd_buffer *cmd_buffer,
               {
      if (cmd_buffer->state.queries.clear_bits == 0)
            anv_add_pending_pipe_bits(cmd_buffer,
                        }
         void genX(CmdBeginQueryIndexedEXT)(
      VkCommandBuffer                             commandBuffer,
   VkQueryPool                                 queryPool,
   uint32_t                                    query,
   VkQueryControlFlags                         flags,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_query_pool, pool, queryPool);
                     struct mi_builder b;
   mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);
   const uint32_t mocs = anv_mocs_for_address(cmd_buffer->device, &query_addr);
            switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
      cmd_buffer->state.gfx.n_occlusion_queries++;
   cmd_buffer->state.gfx.dirty |= ANV_CMD_DIRTY_OCCLUSION_QUERY_ACTIVE;
   emit_ps_depth_count(cmd_buffer, anv_address_add(query_addr, 8));
         case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      genx_batch_emit_pipe_control(&cmd_buffer->batch,
                     mi_store(&b, mi_mem64(anv_address_add(query_addr, 8)),
               case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
      /* TODO: This might only be necessary for certain stats */
   genx_batch_emit_pipe_control(&cmd_buffer->batch,
                        uint32_t statistics = pool->vk.pipeline_statistics;
   uint32_t offset = 8;
   while (statistics) {
      uint32_t stat = u_bit_scan(&statistics);
   emit_pipeline_stat(&b, stat, anv_address_add(query_addr, offset));
      }
               case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      genx_batch_emit_pipe_control(&cmd_buffer->batch,
                     emit_xfb_query(&b, index, anv_address_add(query_addr, 8));
         case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      if (!khr_perf_query_ensure_relocs(cmd_buffer))
            const struct anv_physical_device *pdevice = cmd_buffer->device->physical;
            uint32_t reloc_idx = 0;
   for (uint32_t end = 0; end < 2; end++) {
      for (uint32_t r = 0; r < layout->n_fields; r++) {
      const struct intel_perf_query_field *field =
         struct mi_value reg_addr =
      mi_iadd(
      &b,
   mi_imm(intel_canonical_address(pool->bo->offset +
                        if (field->type != INTEL_PERF_QUERY_FIELD_TYPE_MI_RPC &&
      field->size == 8) {
   reg_addr =
      mi_iadd(
      &b,
   mi_imm(intel_canonical_address(pool->bo->offset +
                                 struct mi_value availability_write_offset =
      mi_iadd(
      &b,
   mi_imm(
      intel_canonical_address(
      pool->bo->offset +
   cmd_buffer->self_mod_locations[reloc_idx++] =
                     const struct intel_device_info *devinfo = cmd_buffer->device->info;
   const enum intel_engine_class engine_class = cmd_buffer->queue_family->engine_class;
            genx_batch_emit_pipe_control(&cmd_buffer->batch,
                              cmd_buffer->perf_reloc_idx = 0;
   for (uint32_t r = 0; r < layout->n_fields; r++) {
      const struct intel_perf_query_field *field =
                  switch (field->type) {
   case INTEL_PERF_QUERY_FIELD_TYPE_MI_RPC:
      dws = anv_batch_emitn(&cmd_buffer->batch,
                     _mi_resolve_address_token(&b,
                           case INTEL_PERF_QUERY_FIELD_TYPE_SRM_PERFCNT:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_RPSTAT:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_A:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_B:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_C:
      dws =
      anv_batch_emitn(&cmd_buffer->batch,
                  GENX(MI_STORE_REGISTER_MEM_length),
   _mi_resolve_address_token(&b,
                     if (field->size == 8) {
      dws =
      anv_batch_emitn(&cmd_buffer->batch,
                  GENX(MI_STORE_REGISTER_MEM_length),
   _mi_resolve_address_token(&b,
                              default:
      unreachable("Invalid query field");
         }
               case VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL: {
      genx_batch_emit_pipe_control(&cmd_buffer->batch,
                     emit_perf_intel_query(cmd_buffer, pool, &b, query_addr, false);
      }
   case VK_QUERY_TYPE_RESULT_STATUS_ONLY_KHR:
      emit_query_mi_flush_availability(cmd_buffer, query_addr, false);
      default:
            }
      void genX(CmdEndQueryIndexedEXT)(
      VkCommandBuffer                             commandBuffer,
   VkQueryPool                                 queryPool,
   uint32_t                                    query,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_query_pool, pool, queryPool);
            struct mi_builder b;
            switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
      emit_ps_depth_count(cmd_buffer, anv_address_add(query_addr, 16));
   emit_query_pc_availability(cmd_buffer, query_addr, true);
   cmd_buffer->state.gfx.n_occlusion_queries--;
   cmd_buffer->state.gfx.dirty |= ANV_CMD_DIRTY_OCCLUSION_QUERY_ACTIVE;
         case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      /* Ensure previous commands have completed before capturing the register
   * value.
   */
   genx_batch_emit_pipe_control(&cmd_buffer->batch,
                        mi_store(&b, mi_mem64(anv_address_add(query_addr, 16)),
         emit_query_mi_availability(&b, query_addr, true);
         case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
      /* TODO: This might only be necessary for certain stats */
   genx_batch_emit_pipe_control(&cmd_buffer->batch,
                        uint32_t statistics = pool->vk.pipeline_statistics;
   uint32_t offset = 16;
   while (statistics) {
      uint32_t stat = u_bit_scan(&statistics);
   emit_pipeline_stat(&b, stat, anv_address_add(query_addr, offset));
               emit_query_mi_availability(&b, query_addr, true);
               case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      genx_batch_emit_pipe_control(&cmd_buffer->batch,
                     emit_xfb_query(&b, index, anv_address_add(query_addr, 16));
   emit_query_mi_availability(&b, query_addr, true);
         case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      genx_batch_emit_pipe_control(&cmd_buffer->batch,
                              if (!khr_perf_query_ensure_relocs(cmd_buffer))
            const struct anv_physical_device *pdevice = cmd_buffer->device->physical;
            void *dws;
   for (uint32_t r = 0; r < layout->n_fields; r++) {
               switch (field->type) {
   case INTEL_PERF_QUERY_FIELD_TYPE_MI_RPC:
      dws = anv_batch_emitn(&cmd_buffer->batch,
                     _mi_resolve_address_token(&b,
                           case INTEL_PERF_QUERY_FIELD_TYPE_SRM_PERFCNT:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_RPSTAT:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_A:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_B:
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_C:
      dws =
      anv_batch_emitn(&cmd_buffer->batch,
                  GENX(MI_STORE_REGISTER_MEM_length),
   _mi_resolve_address_token(&b,
                     if (field->size == 8) {
      dws =
      anv_batch_emitn(&cmd_buffer->batch,
                  GENX(MI_STORE_REGISTER_MEM_length),
   _mi_resolve_address_token(&b,
                              default:
      unreachable("Invalid query field");
                  dws =
      anv_batch_emitn(&cmd_buffer->batch,
                  _mi_resolve_address_token(&b,
                        assert(cmd_buffer->perf_reloc_idx == pdevice->n_perf_query_commands);
               case VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL: {
      genx_batch_emit_pipe_control(&cmd_buffer->batch,
                     uint32_t marker_offset = intel_perf_marker_offset();
   mi_store(&b, mi_mem64(anv_address_add(query_addr, marker_offset)),
         emit_perf_intel_query(cmd_buffer, pool, &b, query_addr, true);
   emit_query_mi_availability(&b, query_addr, true);
      }
   case VK_QUERY_TYPE_RESULT_STATUS_ONLY_KHR:
      emit_query_mi_flush_availability(cmd_buffer, query_addr, true);
         default:
                  /* When multiview is active the spec requires that N consecutive query
   * indices are used, where N is the number of active views in the subpass.
   * The spec allows that we only write the results to one of the queries
   * but we still need to manage result availability for all the query indices.
   * Since we only emit a single query for all active views in the
   * first index, mark the other query indices as being already available
   * with result 0.
   */
   if (cmd_buffer->state.gfx.view_mask) {
      const uint32_t num_queries =
         if (num_queries > 1)
         }
      #define TIMESTAMP 0x2358
      void genX(CmdWriteTimestamp2)(
      VkCommandBuffer                             commandBuffer,
   VkPipelineStageFlags2                       stage,
   VkQueryPool                                 queryPool,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_query_pool, pool, queryPool);
                     emit_query_clear_flush(cmd_buffer, pool,
            struct mi_builder b;
            if (stage == VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT) {
      mi_store(&b, mi_mem64(anv_address_add(query_addr, 8)),
            } else {
      /* Everything else is bottom-of-pipe */
   cmd_buffer->state.pending_pipe_bits |= ANV_PIPE_POST_SYNC_BIT;
            bool cs_stall_needed =
            if (anv_cmd_buffer_is_blitter_queue(cmd_buffer) ||
      anv_cmd_buffer_is_video_queue(cmd_buffer)) {
   anv_batch_emit(&cmd_buffer->batch, GENX(MI_FLUSH_DW), dw) {
      dw.Address = anv_address_add(query_addr, 8);
      }
      } else {
      genx_batch_emit_pipe_control_write
      (&cmd_buffer->batch, cmd_buffer->device->info, WriteTimestamp,
   anv_address_add(query_addr, 8), 0,
                           /* When multiview is active the spec requires that N consecutive query
   * indices are used, where N is the number of active views in the subpass.
   * The spec allows that we only write the results to one of the queries
   * but we still need to manage result availability for all the query indices.
   * Since we only emit a single query for all active views in the
   * first index, mark the other query indices as being already available
   * with result 0.
   */
   if (cmd_buffer->state.gfx.view_mask) {
      const uint32_t num_queries =
         if (num_queries > 1)
         }
      #define MI_PREDICATE_SRC0    0x2400
   #define MI_PREDICATE_SRC1    0x2408
   #define MI_PREDICATE_RESULT  0x2418
      /**
   * Writes the results of a query to dst_addr is the value at poll_addr is equal
   * to the reference value.
   */
   static void
   gpu_write_query_result_cond(struct anv_cmd_buffer *cmd_buffer,
                              struct mi_builder *b,
   struct anv_address poll_addr,
      {
      mi_store(b, mi_reg64(MI_PREDICATE_SRC0), mi_mem64(poll_addr));
   mi_store(b, mi_reg64(MI_PREDICATE_SRC1), mi_imm(ref_value));
   anv_batch_emit(&cmd_buffer->batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
   mip.CombineOperation = COMBINE_SET;
               if (flags & VK_QUERY_RESULT_64_BIT) {
      struct anv_address res_addr = anv_address_add(dst_addr, value_index * 8);
      } else {
      struct anv_address res_addr = anv_address_add(dst_addr, value_index * 4);
         }
      static void
   gpu_write_query_result(struct mi_builder *b,
                           {
      if (flags & VK_QUERY_RESULT_64_BIT) {
      struct anv_address res_addr = anv_address_add(dst_addr, value_index * 8);
      } else {
      struct anv_address res_addr = anv_address_add(dst_addr, value_index * 4);
         }
      static struct mi_value
   compute_query_result(struct mi_builder *b, struct anv_address addr)
   {
      return mi_isub(b, mi_mem64(anv_address_add(addr, 8)),
      }
      static void
   copy_query_results_with_cs(struct anv_cmd_buffer *cmd_buffer,
                              struct anv_query_pool *pool,
      {
                        /* If render target writes are ongoing, request a render target cache flush
   * to ensure proper ordering of the commands from the 3d pipe and the
   * command streamer.
   */
   if ((cmd_buffer->state.queries.buffer_write_bits |
      cmd_buffer->state.queries.clear_bits) &
   ANV_QUERY_WRITES_RT_FLUSH)
         if ((cmd_buffer->state.queries.buffer_write_bits |
      cmd_buffer->state.queries.clear_bits) &
   ANV_QUERY_WRITES_TILE_FLUSH)
         if ((cmd_buffer->state.queries.buffer_write_bits |
      cmd_buffer->state.queries.clear_bits) &
   ANV_QUERY_WRITES_DATA_FLUSH) {
   needed_flushes |= (ANV_PIPE_DATA_CACHE_FLUSH_BIT |
                     if ((cmd_buffer->state.queries.buffer_write_bits |
      cmd_buffer->state.queries.clear_bits) &
   ANV_QUERY_WRITES_CS_STALL)
         /* Occlusion & timestamp queries are written using a PIPE_CONTROL and
   * because we're about to copy values from MI commands, we need to stall
   * the command streamer to make sure the PIPE_CONTROL values have
   * landed, otherwise we could see inconsistent values & availability.
   *
   *  From the vulkan spec:
   *
   *     "vkCmdCopyQueryPoolResults is guaranteed to see the effect of
   *     previous uses of vkCmdResetQueryPool in the same queue, without any
   *     additional synchronization."
   */
   if (pool->vk.query_type == VK_QUERY_TYPE_OCCLUSION ||
      pool->vk.query_type == VK_QUERY_TYPE_TIMESTAMP)
         if (needed_flushes) {
      anv_add_pending_pipe_bits(cmd_buffer,
                           struct mi_builder b;
   mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);
            for (uint32_t i = 0; i < query_count; i++) {
      struct anv_address query_addr = anv_query_address(pool, first_query + i);
                     /* Wait for the availability write to land before we go read the data */
   if (flags & VK_QUERY_RESULT_WAIT_BIT) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_SEMAPHORE_WAIT), sem) {
      sem.WaitMode            = PollingMode;
   sem.CompareOperation    = COMPARE_SAD_EQUAL_SDD;
   sem.SemaphoreDataDword  = true;
                  uint32_t idx = 0;
   switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
   case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      result = compute_query_result(&b, anv_address_add(query_addr, 8));
   /* Like in the case of vkGetQueryPoolResults, if the query is
   * unavailable and the VK_QUERY_RESULT_PARTIAL_BIT flag is set,
   * conservatively write 0 as the query result. If the
   * VK_QUERY_RESULT_PARTIAL_BIT isn't set, don't write any value.
   */
   gpu_write_query_result_cond(cmd_buffer, &b, query_addr, dest_addr,
         if (flags & VK_QUERY_RESULT_PARTIAL_BIT) {
      gpu_write_query_result_cond(cmd_buffer, &b, query_addr, dest_addr,
      }
               case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
      uint32_t statistics = pool->vk.pipeline_statistics;
   while (statistics) {
      UNUSED uint32_t stat = u_bit_scan(&statistics);
   result = compute_query_result(&b, anv_address_add(query_addr,
            }
   assert(idx == util_bitcount(pool->vk.pipeline_statistics));
               case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      result = compute_query_result(&b, anv_address_add(query_addr, 8));
   gpu_write_query_result(&b, dest_addr, flags, idx++, result);
   result = compute_query_result(&b, anv_address_add(query_addr, 24));
               case VK_QUERY_TYPE_TIMESTAMP:
      result = mi_mem64(anv_address_add(query_addr, 8));
         #if GFX_VERx10 >= 125
         case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
      result = mi_mem64(anv_address_add(query_addr, 8));
               case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR:
      result = mi_mem64(anv_address_add(query_addr, 16));
      #endif
            case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR:
                  default:
                  if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
      gpu_write_query_result(&b, dest_addr, flags, idx,
                              }
      static void
   copy_query_results_with_shader(struct anv_cmd_buffer *cmd_buffer,
                                 struct anv_query_pool *pool,
   {
      struct anv_device *device = cmd_buffer->device;
                     /* If this is the first command in the batch buffer, make sure we have
   * consistent pipeline mode.
   */
   if (cmd_buffer->state.current_pipeline == UINT32_MAX)
            if ((cmd_buffer->state.queries.buffer_write_bits |
      cmd_buffer->state.queries.clear_bits) & ANV_QUERY_WRITES_RT_FLUSH)
         if ((cmd_buffer->state.queries.buffer_write_bits |
      cmd_buffer->state.queries.clear_bits) & ANV_QUERY_WRITES_DATA_FLUSH) {
   needed_flushes |= (ANV_PIPE_HDC_PIPELINE_FLUSH_BIT |
               /* Flushes for the queries to complete */
   if (flags & VK_QUERY_RESULT_WAIT_BIT) {
      /* Some queries are done with shaders, so we need to have them flush
   * high level caches writes. The L3 should be shared across the GPU.
   */
   if (pool->vk.query_type == VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR ||
      pool->vk.query_type == VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR ||
   pool->vk.query_type == VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR ||
   pool->vk.query_type == VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR) {
      }
   /* And we need to stall for previous CS writes to land or the flushes to
   * complete.
   */
               /* Occlusion & timestamp queries are written using a PIPE_CONTROL and
   * because we're about to copy values from MI commands, we need to stall
   * the command streamer to make sure the PIPE_CONTROL values have
   * landed, otherwise we could see inconsistent values & availability.
   *
   *  From the vulkan spec:
   *
   *     "vkCmdCopyQueryPoolResults is guaranteed to see the effect of
   *     previous uses of vkCmdResetQueryPool in the same queue, without any
   *     additional synchronization."
   */
   if (pool->vk.query_type == VK_QUERY_TYPE_OCCLUSION ||
      pool->vk.query_type == VK_QUERY_TYPE_TIMESTAMP)
         if (needed_flushes) {
      anv_add_pending_pipe_bits(cmd_buffer,
                           struct anv_simple_shader state = {
      .device               = cmd_buffer->device,
   .cmd_buffer           = cmd_buffer,
   .dynamic_state_stream = &cmd_buffer->dynamic_state_stream,
   .general_state_stream = &cmd_buffer->general_state_stream,
   .batch                = &cmd_buffer->batch,
   .kernel               = device->internal_kernels[
      cmd_buffer->state.current_pipeline == GPGPU ?
   ANV_INTERNAL_KERNEL_COPY_QUERY_RESULTS_COMPUTE :
         };
            struct anv_state push_data_state =
      genX(simple_shader_alloc_push)(&state,
               uint32_t copy_flags =
      ((flags & VK_QUERY_RESULT_64_BIT) ? ANV_COPY_QUERY_FLAG_RESULT64 : 0) |
         uint32_t num_items = 1;
   uint32_t data_offset = 8 /* behind availability */;
   switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
   case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      copy_flags |= ANV_COPY_QUERY_FLAG_DELTA;
   /* These 2 queries are the only ones where we would have partial data
   * because they are capture with a PIPE_CONTROL post sync operation. The
   * other ones are captured with MI_STORE_REGISTER_DATA so we're always
   * available by the time we reach the copy command.
   */
   copy_flags |= (flags & VK_QUERY_RESULT_PARTIAL_BIT) ? ANV_COPY_QUERY_FLAG_PARTIAL : 0;
         case VK_QUERY_TYPE_PIPELINE_STATISTICS:
      num_items = util_bitcount(pool->vk.pipeline_statistics);
   copy_flags |= ANV_COPY_QUERY_FLAG_DELTA;
         case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      num_items = 2;
   copy_flags |= ANV_COPY_QUERY_FLAG_DELTA;
         case VK_QUERY_TYPE_TIMESTAMP:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
            case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR:
      data_offset += 8;
         default:
                  *params = (struct anv_query_copy_params) {
      .copy = {
      .flags              = copy_flags,
   .num_queries        = query_count,
   .num_items          = num_items,
   .query_base         = first_query,
   .query_stride       = pool->stride,
   .query_data_offset  = data_offset,
      },
   .query_data_addr  = anv_address_physical(
      (struct anv_address) {
                                 anv_add_pending_pipe_bits(cmd_buffer,
                                 }
      void genX(CmdCopyQueryPoolResults)(
      VkCommandBuffer                             commandBuffer,
   VkQueryPool                                 queryPool,
   uint32_t                                    firstQuery,
   uint32_t                                    queryCount,
   VkBuffer                                    destBuffer,
   VkDeviceSize                                destOffset,
   VkDeviceSize                                destStride,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_query_pool, pool, queryPool);
   ANV_FROM_HANDLE(anv_buffer, buffer, destBuffer);
   struct anv_device *device = cmd_buffer->device;
            if (queryCount > pdevice->instance->query_copy_with_shader_threshold &&
      !intel_device_info_is_mtl(device->info)) {
   copy_query_results_with_shader(cmd_buffer, pool,
                                    } else {
      copy_query_results_with_cs(cmd_buffer, pool,
                              anv_address_add(buffer->address,
      }
      #if GFX_VERx10 == 125 && ANV_SUPPORT_RT
      #include "grl/include/GRLRTASCommon.h"
   #include "grl/grl_metakernel_postbuild_info.h"
      void
   genX(CmdWriteAccelerationStructuresPropertiesKHR)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    accelerationStructureCount,
   const VkAccelerationStructureKHR*           pAccelerationStructures,
   VkQueryType                                 queryType,
   VkQueryPool                                 queryPool,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            assert(queryType == VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR ||
         queryType == VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR ||
            emit_query_clear_flush(cmd_buffer, pool,
            struct mi_builder b;
            for (uint32_t i = 0; i < accelerationStructureCount; i++) {
      ANV_FROM_HANDLE(vk_acceleration_structure, accel, pAccelerationStructures[i]);
   struct anv_address query_addr =
            switch (queryType) {
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR:
      genX(grl_postbuild_info_compacted_size)(cmd_buffer,
                     case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR:
      genX(grl_postbuild_info_current_size)(cmd_buffer,
                     case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR:
   case VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR:
      genX(grl_postbuild_info_serialized_size)(cmd_buffer,
                     default:
                     /* TODO: Figure out why MTL needs ANV_PIPE_DATA_CACHE_FLUSH_BIT in order
   * to not lose the availability bit.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                              for (uint32_t i = 0; i < accelerationStructureCount; i++)
      }
   #endif
