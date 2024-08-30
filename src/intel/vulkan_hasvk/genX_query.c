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
      /* We reserve :
   *    - GPR 14 for perf queries
   *    - GPR 15 for conditional rendering
   */
   #define MI_BUILDER_NUM_ALLOC_GPRS 14
   #define MI_BUILDER_CAN_WRITE_BATCH GFX_VER >= 8
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
      VkResult genX(CreateQueryPool)(
      VkDevice                                    _device,
   const VkQueryPoolCreateInfo*                pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
      #if GFX_VER >= 8
      const VkQueryPoolPerformanceCreateInfoKHR *perf_query_info = NULL;
   struct intel_perf_counter_pass *counter_pass;
   struct intel_perf_query_info **pass_query;
      #endif
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
         #if GFX_VER >= 8
      case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      const struct intel_perf_query_field_layout *layout =
            perf_query_info = vk_find_struct_const(pCreateInfo->pNext,
         n_passes = intel_perf_get_n_passes(pdevice->perf,
                     vk_multialloc_add(&ma, &counter_pass, struct intel_perf_counter_pass,
         vk_multialloc_add(&ma, &pass_query, struct intel_perf_query_info *,
         uint64s_per_slot = 4 /* availability + small batch */;
   /* Align to the requirement of the layout */
   uint64s_per_slot = align(uint64s_per_slot,
         data_offset = uint64s_per_slot * sizeof(uint64_t);
   /* Add the query data for begin & end commands */
   uint64s_per_slot += 2 * DIV_ROUND_UP(layout->size, sizeof(uint64_t));
   /* Multiply by the number of passes */
   uint64s_per_slot *= n_passes;
         #endif
      case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      /* Query has two values: begin and end. */
   uint64s_per_slot = 1 + 2;
      default:
                  if (!vk_object_multialloc(&device->vk, &ma, pAllocator,
                  pool->type = pCreateInfo->queryType;
   pool->pipeline_statistics = pipeline_statistics;
   pool->stride = uint64s_per_slot * sizeof(uint64_t);
            if (pool->type == VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL) {
      pool->data_offset = data_offset;
         #if GFX_VER >= 8
      else if (pool->type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      pool->pass_size = pool->stride / n_passes;
   pool->data_offset = data_offset;
   pool->snapshot_size = (pool->pass_size - data_offset) / 2;
   pool->n_counters = perf_query_info->counterIndexCount;
   pool->counter_pass = counter_pass;
   intel_perf_get_counters_passes(pdevice->perf,
                     pool->n_passes = n_passes;
   pool->pass_query = pass_query;
   intel_perf_get_n_passes(pdevice->perf,
                     #endif
         uint64_t size = pool->slots * (uint64_t)pool->stride;
   result = anv_device_alloc_bo(device, "query-pool", size,
                           if (result != VK_SUCCESS)
         #if GFX_VER >= 8
      if (pool->type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      for (uint32_t p = 0; p < pool->n_passes; p++) {
      struct mi_builder b;
   struct anv_batch batch = {
      .start = pool->bo->map + khr_perf_query_preamble_offset(pool, p),
                     mi_builder_init(&b, device->info, &batch);
   mi_store(&b, mi_reg64(ANV_PERF_QUERY_OFFSET_REG),
                  #endif
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
      #if GFX_VER >= 8
   /**
   * VK_KHR_performance_query layout  :
   *
   * --------------------------------------------
   * |       availability (8b)       | |        |
   * |-------------------------------| |        |
   * |      Small batch loading      | |        |
   * |   ANV_PERF_QUERY_OFFSET_REG   | |        |
   * |            (24b)              | | Pass 0 |
   * |-------------------------------| |        |
   * |       some padding (see       | |        |
   * | query_field_layout:alignment) | |        |
   * |-------------------------------| |        |
   * |           query data          | |        |
   * | (2 * query_field_layout:size) | |        |
   * |-------------------------------|--        | Query 0
   * |       availability (8b)       | |        |
   * |-------------------------------| |        |
   * |      Small batch loading      | |        |
   * |   ANV_PERF_QUERY_OFFSET_REG   | |        |
   * |            (24b)              | | Pass 1 |
   * |-------------------------------| |        |
   * |       some padding (see       | |        |
   * | query_field_layout:alignment) | |        |
   * |-------------------------------| |        |
   * |           query data          | |        |
   * | (2 * query_field_layout:size) | |        |
   * |-------------------------------|-----------
   * |       availability (8b)       | |        |
   * |-------------------------------| |        |
   * |      Small batch loading      | |        |
   * |   ANV_PERF_QUERY_OFFSET_REG   | |        |
   * |            (24b)              | | Pass 0 |
   * |-------------------------------| |        |
   * |       some padding (see       | |        |
   * | query_field_layout:alignment) | |        |
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
   #endif
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
   #if GFX_VER >= 8
      if (pool->type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      for (uint32_t p = 0; p < pool->n_passes; p++) {
      volatile uint64_t *slot =
         if (!slot[0])
      }
         #endif
            }
      static VkResult
   wait_for_available(struct anv_device *device,
         {
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
            assert(pool->type == VK_QUERY_TYPE_OCCLUSION ||
         pool->type == VK_QUERY_TYPE_PIPELINE_STATISTICS ||
   pool->type == VK_QUERY_TYPE_TIMESTAMP ||
   pool->type == VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT ||
   pool->type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR ||
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
   switch (pool->type) {
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
   uint32_t statistics = pool->pipeline_statistics;
   while (statistics) {
      uint32_t stat = u_bit_scan(&statistics);
                     /* WaDividePSInvocationCountBy4:HSW,BDW */
                           }
      }
   assert(idx == util_bitcount(pool->pipeline_statistics));
               case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT: {
      uint64_t *slot = query_slot(pool, firstQuery + i);
   if (write_results)
         idx++;
   if (write_results)
         idx++;
               case VK_QUERY_TYPE_TIMESTAMP: {
      uint64_t *slot = query_slot(pool, firstQuery + i);
   if (write_results)
         idx++;
         #if GFX_VER >= 8
         case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      const struct anv_physical_device *pdevice = device->physical;
   assert((flags & (VK_QUERY_RESULT_WITH_AVAILABILITY_BIT |
         for (uint32_t p = 0; p < pool->n_passes; p++) {
      const struct intel_perf_query_info *query = pool->pass_query[p];
   struct intel_perf_query_result result;
   intel_perf_query_result_clear(&result);
   intel_perf_query_result_accumulate_fields(&result, query,
                        }
      #endif
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
            anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.DestinationAddressType  = DAT_PPGTT;
   pc.PostSyncOperation       = WritePSDepthCount;
   pc.DepthStallEnable        = true;
            if (GFX_VER == 9 && cmd_buffer->device->info->gt == 4)
         }
      static void
   emit_query_mi_availability(struct mi_builder *b,
               {
         }
      static void
   emit_query_pc_availability(struct anv_cmd_buffer *cmd_buffer,
               {
      cmd_buffer->state.pending_pipe_bits |= ANV_PIPE_POST_SYNC_BIT;
            anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.DestinationAddressType  = DAT_PPGTT;
   pc.PostSyncOperation       = WriteImmediateData;
   pc.Address                 = addr;
         }
      /**
   * Goes through a series of consecutive query indices in the given pool
   * setting all element values to 0 and emitting them as available.
   */
   static void
   emit_zero_queries(struct anv_cmd_buffer *cmd_buffer,
               {
      switch (pool->type) {
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
      #if GFX_VER >= 8
      case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      for (uint32_t i = 0; i < num_queries; i++) {
      for (uint32_t p = 0; p < pool->n_passes; p++) {
      mi_memset(b, khr_perf_query_data_address(pool, first_index + i, p, false),
         emit_query_mi_availability(b,
               }
         #endif
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
            switch (pool->type) {
   case VK_QUERY_TYPE_OCCLUSION:
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
                  #if GFX_VER >= 8
      case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      struct mi_builder b;
            for (uint32_t i = 0; i < queryCount; i++) {
      for (uint32_t p = 0; p < pool->n_passes; p++) {
      emit_query_mi_availability(
      &b,
   khr_perf_query_availability_address(pool, firstQuery + i, p),
      }
         #endif
         case VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL: {
      struct mi_builder b;
            for (uint32_t i = 0; i < queryCount; i++)
                     default:
            }
      void genX(ResetQueryPool)(
      VkDevice                                    _device,
   VkQueryPool                                 queryPool,
   uint32_t                                    firstQuery,
      {
               for (uint32_t i = 0; i < queryCount; i++) {
      #if GFX_VER >= 8
            for (uint32_t p = 0; p < pool->n_passes; p++) {
      uint64_t *pass_slot = pool->bo->map +
         #endif
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
      void genX(CmdBeginQuery)(
      VkCommandBuffer                             commandBuffer,
   VkQueryPool                                 queryPool,
   uint32_t                                    query,
      {
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
            switch (pool->type) {
   case VK_QUERY_TYPE_OCCLUSION:
      emit_ps_depth_count(cmd_buffer, anv_address_add(query_addr, 8));
         case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
      }
   mi_store(&b, mi_mem64(anv_address_add(query_addr, 8)),
               case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
      /* TODO: This might only be necessary for certain stats */
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
               uint32_t statistics = pool->pipeline_statistics;
   uint32_t offset = 8;
   while (statistics) {
      uint32_t stat = u_bit_scan(&statistics);
   emit_pipeline_stat(&b, stat, anv_address_add(query_addr, offset));
      }
               case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
      }
   emit_xfb_query(&b, index, anv_address_add(query_addr, 8));
      #if GFX_VER >= 8
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
            anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
      }
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
         #endif
         case VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL: {
      anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
      }
   emit_perf_intel_query(cmd_buffer, pool, &b, query_addr, false);
               default:
            }
      void genX(CmdEndQuery)(
      VkCommandBuffer                             commandBuffer,
   VkQueryPool                                 queryPool,
      {
         }
      void genX(CmdEndQueryIndexedEXT)(
      VkCommandBuffer                             commandBuffer,
   VkQueryPool                                 queryPool,
   uint32_t                                    query,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_query_pool, pool, queryPool);
            struct mi_builder b;
            switch (pool->type) {
   case VK_QUERY_TYPE_OCCLUSION:
      emit_ps_depth_count(cmd_buffer, anv_address_add(query_addr, 16));
   emit_query_pc_availability(cmd_buffer, query_addr, true);
         case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      /* Ensure previous commands have completed before capturing the register
   * value.
   */
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
               mi_store(&b, mi_mem64(anv_address_add(query_addr, 16)),
         emit_query_mi_availability(&b, query_addr, true);
         case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
      /* TODO: This might only be necessary for certain stats */
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
               uint32_t statistics = pool->pipeline_statistics;
   uint32_t offset = 16;
   while (statistics) {
      uint32_t stat = u_bit_scan(&statistics);
   emit_pipeline_stat(&b, stat, anv_address_add(query_addr, offset));
               emit_query_mi_availability(&b, query_addr, true);
               case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
               emit_xfb_query(&b, index, anv_address_add(query_addr, 16));
   emit_query_mi_availability(&b, query_addr, true);
      #if GFX_VER >= 8
      case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
      }
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
         #endif
         case VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL: {
      anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
      }
   uint32_t marker_offset = intel_perf_marker_offset();
   mi_store(&b, mi_mem64(anv_address_add(query_addr, marker_offset)),
         emit_perf_intel_query(cmd_buffer, pool, &b, query_addr, true);
   emit_query_mi_availability(&b, query_addr, true);
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
                     struct mi_builder b;
            if (stage == VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT) {
      mi_store(&b, mi_mem64(anv_address_add(query_addr, 8)),
            } else {
      /* Everything else is bottom-of-pipe */
   cmd_buffer->state.pending_pipe_bits |= ANV_PIPE_POST_SYNC_BIT;
            anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.DestinationAddressType  = DAT_PPGTT;
                  if (GFX_VER == 9 && cmd_buffer->device->info->gt == 4)
      }
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
      #if GFX_VERx10 >= 75
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
            struct mi_builder b;
   mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);
            /* If render target writes are ongoing, request a render target cache flush
   * to ensure proper ordering of the commands from the 3d pipe and the
   * command streamer.
   */
   if (cmd_buffer->state.pending_pipe_bits & ANV_PIPE_RENDER_TARGET_BUFFER_WRITES) {
      anv_add_pending_pipe_bits(cmd_buffer,
                           if ((flags & VK_QUERY_RESULT_WAIT_BIT) ||
      (cmd_buffer->state.pending_pipe_bits & ANV_PIPE_FLUSH_BITS) ||
   /* Occlusion & timestamp queries are written using a PIPE_CONTROL and
   * because we're about to copy values from MI commands, we need to
   * stall the command streamer to make sure the PIPE_CONTROL values have
   * landed, otherwise we could see inconsistent values & availability.
   *
   *  From the vulkan spec:
   *
   *     "vkCmdCopyQueryPoolResults is guaranteed to see the effect of
   *     previous uses of vkCmdResetQueryPool in the same queue, without
   *     any additional synchronization."
   */
   pool->type == VK_QUERY_TYPE_OCCLUSION ||
   pool->type == VK_QUERY_TYPE_TIMESTAMP) {
   anv_add_pending_pipe_bits(cmd_buffer,
                           struct anv_address dest_addr = anv_address_add(buffer->address, destOffset);
   for (uint32_t i = 0; i < queryCount; i++) {
      struct anv_address query_addr = anv_query_address(pool, firstQuery + i);
   uint32_t idx = 0;
   switch (pool->type) {
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
      uint32_t statistics = pool->pipeline_statistics;
                                    /* WaDividePSInvocationCountBy4:HSW,BDW */
   if ((cmd_buffer->device->info->ver == 8 ||
      cmd_buffer->device->info->verx10 == 75) &&
                        }
   assert(idx == util_bitcount(pool->pipeline_statistics));
               case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      result = compute_query_result(&b, anv_address_add(query_addr, 8));
   gpu_write_query_result(&b, dest_addr, flags, idx++, result);
   result = compute_query_result(&b, anv_address_add(query_addr, 24));
               case VK_QUERY_TYPE_TIMESTAMP:
      result = mi_mem64(anv_address_add(query_addr, 8));
         #if GFX_VER >= 8
         case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR:
         #endif
            default:
                  if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
      gpu_write_query_result(&b, dest_addr, flags, idx,
                     }
      #else
   void genX(CmdCopyQueryPoolResults)(
      VkCommandBuffer                             commandBuffer,
   VkQueryPool                                 queryPool,
   uint32_t                                    firstQuery,
   uint32_t                                    queryCount,
   VkBuffer                                    destBuffer,
   VkDeviceSize                                destOffset,
   VkDeviceSize                                destStride,
      {
         }
   #endif
