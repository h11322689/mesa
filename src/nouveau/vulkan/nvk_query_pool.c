   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_query_pool.h"
      #include "nvk_buffer.h"
   #include "nvk_cmd_buffer.h"
   #include "nvk_device.h"
   #include "nvk_entrypoints.h"
   #include "nvk_event.h"
   #include "nvk_mme.h"
   #include "nvk_physical_device.h"
   #include "nvk_pipeline.h"
      #include "vk_meta.h"
   #include "vk_pipeline.h"
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
      #include "nouveau_bo.h"
   #include "nouveau_context.h"
      #include "util/os_time.h"
      #include "nvk_cl906f.h"
   #include "nvk_cl9097.h"
   #include "nvk_cla0c0.h"
   #include "nvk_clc597.h"
      struct nvk_query_report {
      uint64_t value;
      };
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateQueryPool(VkDevice device,
                     {
      VK_FROM_HANDLE(nvk_device, dev, device);
            pool = vk_query_pool_create(&dev->vk, pCreateInfo,
         if (!pool)
            /* We place the availability first and then data */
   pool->query_start = align(pool->vk.query_count * sizeof(uint32_t),
            uint32_t reports_per_query;
   switch (pCreateInfo->queryType) {
   case VK_QUERY_TYPE_OCCLUSION:
      reports_per_query = 2;
      case VK_QUERY_TYPE_TIMESTAMP:
      reports_per_query = 1;
      case VK_QUERY_TYPE_PIPELINE_STATISTICS:
      reports_per_query = 2 * util_bitcount(pool->vk.pipeline_statistics);
      case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      // 2 for primitives succeeded 2 for primitives needed
   reports_per_query = 4;
      default:
         }
            if (pool->vk.query_count > 0) {
      uint32_t bo_size = pool->query_start +
         pool->bo = nouveau_ws_bo_new_mapped(dev->ws_dev, bo_size, 0,
                           if (!pool->bo) {
      vk_query_pool_destroy(&dev->vk, pAllocator, &pool->vk);
               if (dev->ws_dev->debug_flags & NVK_DEBUG_ZERO_MEMORY)
                           }
      VKAPI_ATTR void VKAPI_CALL
   nvk_DestroyQueryPool(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            if (!pool)
            if (pool->bo) {
      nouveau_ws_bo_unmap(pool->bo, pool->bo_map);
      }
      }
      static uint64_t
   nvk_query_available_addr(struct nvk_query_pool *pool, uint32_t query)
   {
      assert(query < pool->vk.query_count);
      }
      static nir_def *
   nvk_nir_available_addr(nir_builder *b, nir_def *pool_addr,
         {
      nir_def *offset = nir_imul_imm(b, query, sizeof(uint32_t));
      }
      static uint32_t *
   nvk_query_available_map(struct nvk_query_pool *pool, uint32_t query)
   {
      assert(query < pool->vk.query_count);
      }
      static uint64_t
   nvk_query_offset(struct nvk_query_pool *pool, uint32_t query)
   {
      assert(query < pool->vk.query_count);
      }
      static uint64_t
   nvk_query_report_addr(struct nvk_query_pool *pool, uint32_t query)
   {
         }
      static nir_def *
   nvk_nir_query_report_addr(nir_builder *b, nir_def *pool_addr,
               {
      nir_def *offset =
            }
      static struct nvk_query_report *
   nvk_query_report_map(struct nvk_query_pool *pool, uint32_t query)
   {
         }
      /**
   * Goes through a series of consecutive query indices in the given pool,
   * setting all element values to 0 and emitting them as available.
   */
   static void
   emit_zero_queries(struct nvk_cmd_buffer *cmd, struct nvk_query_pool *pool,
         {
      switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
   case VK_QUERY_TYPE_TIMESTAMP:
   case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
      for (uint32_t i = 0; i < num_queries; i++) {
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 5);
   P_MTHD(p, NV9097, SET_REPORT_SEMAPHORE_A);
   P_NV9097_SET_REPORT_SEMAPHORE_A(p, addr >> 32);
   P_NV9097_SET_REPORT_SEMAPHORE_B(p, addr);
   P_NV9097_SET_REPORT_SEMAPHORE_C(p, 1);
   P_NV9097_SET_REPORT_SEMAPHORE_D(p, {
      .operation = OPERATION_RELEASE,
   .release = RELEASE_AFTER_ALL_PRECEEDING_WRITES_COMPLETE,
   .pipeline_location = PIPELINE_LOCATION_ALL,
         }
      }
   default:
            }
      VKAPI_ATTR void VKAPI_CALL
   nvk_ResetQueryPool(VkDevice device,
                     {
               uint32_t *available = nvk_query_available_map(pool, firstQuery);
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdResetQueryPool(VkCommandBuffer commandBuffer,
                     {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
            for (uint32_t i = 0; i < queryCount; i++) {
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 5);
   P_MTHD(p, NV9097, SET_REPORT_SEMAPHORE_A);
   P_NV9097_SET_REPORT_SEMAPHORE_A(p, addr >> 32);
   P_NV9097_SET_REPORT_SEMAPHORE_B(p, addr);
   P_NV9097_SET_REPORT_SEMAPHORE_C(p, 0);
   P_NV9097_SET_REPORT_SEMAPHORE_D(p, {
      .operation = OPERATION_RELEASE,
   .release = RELEASE_AFTER_ALL_PRECEEDING_WRITES_COMPLETE,
   .pipeline_location = PIPELINE_LOCATION_ALL,
                  /* Wait for the above writes to complete.  This prevents WaW hazards on any
   * later query availability updates and ensures vkCmdCopyQueryPoolResults
   * will see the query as unavailable if it happens before the query is
   * completed again.
   */
   for (uint32_t i = 0; i < queryCount; i++) {
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 5);
   __push_mthd(p, SUBC_NV9097, NV906F_SEMAPHOREA);
   P_NV906F_SEMAPHOREA(p, addr >> 32);
   P_NV906F_SEMAPHOREB(p, (addr & UINT32_MAX) >> 2);
   P_NV906F_SEMAPHOREC(p, 0);
   P_NV906F_SEMAPHORED(p, {
      .operation = OPERATION_ACQUIRE,
   .acquire_switch = ACQUIRE_SWITCH_ENABLED,
            }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdWriteTimestamp2(VkCommandBuffer commandBuffer,
                     {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
                     uint64_t report_addr = nvk_query_report_addr(pool, query);
   P_MTHD(p, NV9097, SET_REPORT_SEMAPHORE_A);
   P_NV9097_SET_REPORT_SEMAPHORE_A(p, report_addr >> 32);
   P_NV9097_SET_REPORT_SEMAPHORE_B(p, report_addr);
   P_NV9097_SET_REPORT_SEMAPHORE_C(p, 0);
   P_NV9097_SET_REPORT_SEMAPHORE_D(p, {
      .operation = OPERATION_REPORT_ONLY,
   .pipeline_location = vk_stage_flags_to_nv9097_pipeline_location(stage),
               uint64_t available_addr = nvk_query_available_addr(pool, query);
   P_MTHD(p, NV9097, SET_REPORT_SEMAPHORE_A);
   P_NV9097_SET_REPORT_SEMAPHORE_A(p, available_addr >> 32);
   P_NV9097_SET_REPORT_SEMAPHORE_B(p, available_addr);
   P_NV9097_SET_REPORT_SEMAPHORE_C(p, 1);
   P_NV9097_SET_REPORT_SEMAPHORE_D(p, {
      .operation = OPERATION_RELEASE,
   .release = RELEASE_AFTER_ALL_PRECEEDING_WRITES_COMPLETE,
   .pipeline_location = PIPELINE_LOCATION_ALL,
               /* From the Vulkan spec:
   *
   *   "If vkCmdWriteTimestamp2 is called while executing a render pass
   *    instance that has multiview enabled, the timestamp uses N consecutive
   *    query indices in the query pool (starting at query) where N is the
   *    number of bits set in the view mask of the subpass the command is
   *    executed in. The resulting query values are determined by an
   *    implementation-dependent choice of one of the following behaviors:"
   *
   * In our case, only the first query is used, so we emit zeros for the
   * remaining queries, as described in the first behavior listed in the
   * Vulkan spec:
   *
   *   "The first query is a timestamp value and (if more than one bit is set
   *   in the view mask) zero is written to the remaining queries."
   */
   if (cmd->state.gfx.render.view_mask != 0) {
      const uint32_t num_queries =
         if (num_queries > 1)
         }
      struct nvk_3d_stat_query {
      VkQueryPipelineStatisticFlagBits flag;
   uint8_t loc;
      };
      /* This must remain sorted in flag order */
   static const struct nvk_3d_stat_query nvk_3d_stat_queries[] = {{
      .flag    = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT,
   .loc     = NV9097_SET_REPORT_SEMAPHORE_D_PIPELINE_LOCATION_DATA_ASSEMBLER,
      }, {
      .flag    = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT,
   .loc     = NV9097_SET_REPORT_SEMAPHORE_D_PIPELINE_LOCATION_DATA_ASSEMBLER,
      }, {
      .flag    = VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT,
   .loc     = NV9097_SET_REPORT_SEMAPHORE_D_PIPELINE_LOCATION_VERTEX_SHADER,
      }, {
      .flag    = VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT,
   .loc     = NV9097_SET_REPORT_SEMAPHORE_D_PIPELINE_LOCATION_GEOMETRY_SHADER,
      }, {
      .flag    = VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT,
   .loc     = NV9097_SET_REPORT_SEMAPHORE_D_PIPELINE_LOCATION_GEOMETRY_SHADER,
      }, {
      .flag    = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT,
   .loc     = NV9097_SET_REPORT_SEMAPHORE_D_PIPELINE_LOCATION_VPC, /* TODO */
      }, {
      .flag    = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT,
   .loc     = NV9097_SET_REPORT_SEMAPHORE_D_PIPELINE_LOCATION_VPC, /* TODO */
      }, {
      .flag    = VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT,
   .loc     = NV9097_SET_REPORT_SEMAPHORE_D_PIPELINE_LOCATION_PIXEL_SHADER,
      }, {
      .flag    = VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT,
   .loc     = NV9097_SET_REPORT_SEMAPHORE_D_PIPELINE_LOCATION_TESSELATION_INIT_SHADER,
      }, {
      .flag    = VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT,
   .loc     = NV9097_SET_REPORT_SEMAPHORE_D_PIPELINE_LOCATION_TESSELATION_SHADER,
      }, {
      .flag    = VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT,
   .loc     = UINT8_MAX,
      }};
      static void
   mme_store_global(struct mme_builder *b,
               {
      mme_mthd(b, NV9097_SET_REPORT_SEMAPHORE_A);
   mme_emit_addr64(b, addr);
   mme_emit(b, v);
      }
      void
   nvk_mme_write_cs_invocations(struct mme_builder *b)
   {
               struct mme_value accum_hi = mme_state(b,
         struct mme_value accum_lo = mme_state(b,
                  mme_store_global(b, dst_addr, accum.lo);
      }
      static void
   nvk_cmd_begin_end_query(struct nvk_cmd_buffer *cmd,
                     {
      uint64_t report_addr = nvk_query_report_addr(pool, query) +
            struct nv_push *p;
   switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
                        P_MTHD(p, NV9097, SET_REPORT_SEMAPHORE_A);
   P_NV9097_SET_REPORT_SEMAPHORE_A(p, report_addr >> 32);
   P_NV9097_SET_REPORT_SEMAPHORE_B(p, report_addr);
   P_NV9097_SET_REPORT_SEMAPHORE_C(p, 0);
   P_NV9097_SET_REPORT_SEMAPHORE_D(p, {
      .operation = OPERATION_REPORT_ONLY,
   .pipeline_location = PIPELINE_LOCATION_ALL,
   .report = REPORT_ZPASS_PIXEL_CNT64,
      });
         case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
      uint32_t stat_count = util_bitcount(pool->vk.pipeline_statistics);
            ASSERTED uint32_t stats_left = pool->vk.pipeline_statistics;
   for (uint32_t i = 0; i < ARRAY_SIZE(nvk_3d_stat_queries); i++) {
      const struct nvk_3d_stat_query *sq = &nvk_3d_stat_queries[i];
                                 if (sq->flag == VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT) {
      P_1INC(p, NVC597, CALL_MME_MACRO(NVK_MME_WRITE_CS_INVOCATIONS));
   P_INLINE_DATA(p, report_addr >> 32);
      } else {
      P_MTHD(p, NV9097, SET_REPORT_SEMAPHORE_A);
   P_NV9097_SET_REPORT_SEMAPHORE_A(p, report_addr >> 32);
   P_NV9097_SET_REPORT_SEMAPHORE_B(p, report_addr);
   P_NV9097_SET_REPORT_SEMAPHORE_C(p, 0);
   P_NV9097_SET_REPORT_SEMAPHORE_D(p, {
      .operation = OPERATION_REPORT_ONLY,
   .pipeline_location = sq->loc,
   .report = sq->report,
                  report_addr += 2 * sizeof(struct nvk_query_report);
      }
               case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT: {
      const uint32_t xfb_reports[] = {
      NV9097_SET_REPORT_SEMAPHORE_D_REPORT_STREAMING_PRIMITIVES_SUCCEEDED,
      };
   p = nvk_cmd_buffer_push(cmd, 5*ARRAY_SIZE(xfb_reports) + 5*end);
   for (uint32_t i = 0; i < ARRAY_SIZE(xfb_reports); ++i) {
      P_MTHD(p, NV9097, SET_REPORT_SEMAPHORE_A);
   P_NV9097_SET_REPORT_SEMAPHORE_A(p, report_addr >> 32);
   P_NV9097_SET_REPORT_SEMAPHORE_B(p, report_addr);
   P_NV9097_SET_REPORT_SEMAPHORE_C(p, 0);
   P_NV9097_SET_REPORT_SEMAPHORE_D(p, {
         .operation = OPERATION_REPORT_ONLY,
   .pipeline_location = PIPELINE_LOCATION_STREAMING_OUTPUT,
   .report = xfb_reports[i],
   .structure_size = STRUCTURE_SIZE_FOUR_WORDS,
   .sub_report = index,
      }
      }
   default:
                  if (end) {
      uint64_t available_addr = nvk_query_available_addr(pool, query);
   P_MTHD(p, NV9097, SET_REPORT_SEMAPHORE_A);
   P_NV9097_SET_REPORT_SEMAPHORE_A(p, available_addr >> 32);
   P_NV9097_SET_REPORT_SEMAPHORE_B(p, available_addr);
   P_NV9097_SET_REPORT_SEMAPHORE_C(p, 1);
   P_NV9097_SET_REPORT_SEMAPHORE_D(p, {
      .operation = OPERATION_RELEASE,
   .release = RELEASE_AFTER_ALL_PRECEEDING_WRITES_COMPLETE,
   .pipeline_location = PIPELINE_LOCATION_ALL,
            }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdBeginQueryIndexedEXT(VkCommandBuffer commandBuffer,
                           {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
               }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdEndQueryIndexedEXT(VkCommandBuffer commandBuffer,
                     {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
                     /* From the Vulkan spec:
   *
   *   "If queries are used while executing a render pass instance that has
   *    multiview enabled, the query uses N consecutive query indices in
   *    the query pool (starting at query) where N is the number of bits set
   *    in the view mask in the subpass the query is used in. How the
   *    numerical results of the query are distributed among the queries is
   *    implementation-dependent."
   *
   * In our case, only the first query is used, so we emit zeros for the
   * remaining queries.
   */
   if (cmd->state.gfx.render.view_mask != 0) {
      const uint32_t num_queries =
         if (num_queries > 1)
         }
      static bool
   nvk_query_is_available(struct nvk_query_pool *pool, uint32_t query)
   {
      uint32_t *available = nvk_query_available_map(pool, query);
      }
      #define NVK_QUERY_TIMEOUT 2000000000ull
      static VkResult
   nvk_query_wait_for_available(struct nvk_device *dev,
               {
               while (os_time_get_nano() < abs_timeout_ns) {
      if (nvk_query_is_available(pool, query))
            VkResult status = vk_device_check_status(&dev->vk);
   if (status != VK_SUCCESS)
                  }
      static void
   cpu_write_query_result(void *dst, uint32_t idx,
               {
      if (flags & VK_QUERY_RESULT_64_BIT) {
      uint64_t *dst64 = dst;
      } else {
      uint32_t *dst32 = dst;
         }
      static void
   cpu_get_query_delta(void *dst, const struct nvk_query_report *src,
         {
      uint64_t delta = src[idx * 2 + 1].value - src[idx * 2].value;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_GetQueryPoolResults(VkDevice device,
                           VkQueryPool queryPool,
   uint32_t firstQuery,
   uint32_t queryCount,
   {
      VK_FROM_HANDLE(nvk_device, dev, device);
            if (vk_device_is_lost(&dev->vk))
            VkResult status = VK_SUCCESS;
   for (uint32_t i = 0; i < queryCount; i++) {
                        if (!available && (flags & VK_QUERY_RESULT_WAIT_BIT)) {
      status = nvk_query_wait_for_available(dev, pool, query);
                                       const struct nvk_query_report *src = nvk_query_report_map(pool, query);
   assert(i * stride < dataSize);
            uint32_t available_dst_idx = 1;
   switch (pool->vk.query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
      if (write_results)
            case VK_QUERY_TYPE_PIPELINE_STATISTICS: {
      uint32_t stat_count = util_bitcount(pool->vk.pipeline_statistics);
   available_dst_idx = stat_count;
   if (write_results) {
      for (uint32_t j = 0; j < stat_count; j++)
      }
      }
   case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT: {
      const int prims_succeeded_idx = 0;
   const int prims_needed_idx = 1;
   available_dst_idx = 2;
   if (write_results) {
      cpu_get_query_delta(dst, src, prims_succeeded_idx, flags);
      }
      }
   case VK_QUERY_TYPE_TIMESTAMP:
      if (write_results)
            default:
                  if (!write_results)
            if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT)
                  }
      struct nvk_copy_query_push {
      uint64_t pool_addr;
   uint32_t query_start;
   uint32_t query_stride;
   uint32_t first_query;
   uint32_t query_count;
   uint64_t dst_addr;
   uint64_t dst_stride;
      };
      static nir_def *
   load_struct_var(nir_builder *b, nir_variable *var, uint32_t field)
   {
      nir_deref_instr *deref =
            }
      static void
   nir_write_query_result(nir_builder *b, nir_def *dst_addr,
               {
      assert(result->num_components == 1);
            nir_push_if(b, nir_test_mask(b, flags, VK_QUERY_RESULT_64_BIT));
   {
      nir_def *offset = nir_i2i64(b, nir_imul_imm(b, idx, 8));
      }
   nir_push_else(b, NULL);
   {
      nir_def *result32 = nir_u2u32(b, result);
   nir_def *offset = nir_i2i64(b, nir_imul_imm(b, idx, 4));
      }
      }
      static void
   nir_get_query_delta(nir_builder *b, nir_def *dst_addr,
               {
      nir_def *offset =
         nir_def *begin_addr =
         nir_def *end_addr =
            /* nvk_query_report::timestamp is the first uint64_t */
   nir_def *begin = nir_load_global(b, begin_addr, 16, 1, 64);
                        }
      static void
   nvk_nir_copy_query(nir_builder *b, nir_variable *push, nir_def *i)
   {
      nir_def *pool_addr = load_struct_var(b, push, 0);
   nir_def *query_start = nir_u2u64(b, load_struct_var(b, push, 1));
   nir_def *query_stride = load_struct_var(b, push, 2);
   nir_def *first_query = load_struct_var(b, push, 3);
   nir_def *dst_addr = load_struct_var(b, push, 5);
   nir_def *dst_stride = load_struct_var(b, push, 6);
                     nir_def *avail_addr = nvk_nir_available_addr(b, pool_addr, query);
   nir_def *available =
            nir_def *partial = nir_test_mask(b, flags, VK_QUERY_RESULT_PARTIAL_BIT);
            nir_def *report_addr =
      nvk_nir_query_report_addr(b, pool_addr, query_start, query_stride,
               /* Timestamp queries are the only ones use a single report */
   nir_def *is_timestamp =
            nir_def *one = nir_imm_int(b, 1);
   nir_def *num_reports;
   nir_push_if(b, is_timestamp);
   {
      nir_push_if(b, write_results);
   {
      /* This is the timestamp case.  We add 8 because we're loading
   * nvk_query_report::timestamp.
   */
                  nir_write_query_result(b, nir_iadd(b, dst_addr, dst_offset),
      }
      }
   nir_push_else(b, NULL);
   {
      /* Everything that isn't a timestamp has the invariant that the
   * number of destination entries is equal to the query stride divided
   * by the size of two reports.
   */
   num_reports = nir_udiv_imm(b, query_stride,
            nir_push_if(b, write_results);
   {
      nir_variable *r =
                  nir_push_loop(b);
   {
      nir_push_if(b, nir_ige(b, nir_load_var(b, r), num_reports));
   {
                                          }
      }
      }
                     nir_push_if(b, nir_test_mask(b, flags, VK_QUERY_RESULT_WITH_AVAILABILITY_BIT));
   {
      nir_write_query_result(b, nir_iadd(b, dst_addr, dst_offset),
      }
      }
      static nir_shader *
   build_copy_queries_shader(void)
   {
      nir_builder build =
      nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, NULL,
               struct glsl_struct_field push_fields[] = {
      { .type = glsl_uint64_t_type(), .name = "pool_addr", .offset = 0 },
   { .type = glsl_uint_type(), .name = "query_start", .offset = 8 },
   { .type = glsl_uint_type(), .name = "query_stride", .offset = 12 },
   { .type = glsl_uint_type(), .name = "first_query", .offset = 16 },
   { .type = glsl_uint_type(), .name = "query_count", .offset = 20 },
   { .type = glsl_uint64_t_type(), .name = "dst_addr", .offset = 24 },
   { .type = glsl_uint64_t_type(), .name = "dst_stride", .offset = 32 },
      };
   const struct glsl_type *push_iface_type =
      glsl_interface_type(push_fields, ARRAY_SIZE(push_fields),
            nir_variable *push = nir_variable_create(b->shader, nir_var_mem_push_const,
                     nir_variable *i = nir_local_variable_create(b->impl, glsl_uint_type(), "i");
            nir_push_loop(b);
   {
      nir_push_if(b, nir_ige(b, nir_load_var(b, i), query_count));
   {
         }
                        }
               }
      static VkResult
   get_copy_queries_pipeline(struct nvk_device *dev,
               {
      const char key[] = "nvk-meta-copy-query-pool-results";
   VkPipeline cached = vk_meta_lookup_pipeline(&dev->meta, key, sizeof(key));
   if (cached != VK_NULL_HANDLE) {
      *pipeline_out = cached;
               const VkPipelineShaderStageNirCreateInfoMESA nir_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_NIR_CREATE_INFO_MESA,
      };
   const VkComputePipelineCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .pNext = &nir_info,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      },
               return vk_meta_create_compute_pipeline(&dev->vk, &dev->meta, &info,
      }
      static void
   nvk_meta_copy_query_pool_results(struct nvk_cmd_buffer *cmd,
                                       {
      struct nvk_device *dev = nvk_cmd_buffer_device(cmd);
   struct nvk_descriptor_state *desc = &cmd->state.cs.descriptors;
            const struct nvk_copy_query_push push = {
      .pool_addr = pool->bo->offset,
   .query_start = pool->query_start,
   .query_stride = pool->query_stride,
   .first_query = first_query,
   .query_count = query_count,
   .dst_addr = dst_addr,
   .dst_stride = dst_stride,
               const char key[] = "nvk-meta-copy-query-pool-results";
   const VkPushConstantRange push_range = {
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      };
   VkPipelineLayout layout;
   result = vk_meta_get_pipeline_layout(&dev->vk, &dev->meta, NULL, &push_range,
         if (result != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd->vk, result);
               VkPipeline pipeline;
   result = get_copy_queries_pipeline(dev, layout, &pipeline);
   if (result != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd->vk, result);
               /* Save pipeline and push constants */
   struct nvk_compute_pipeline *pipeline_save = cmd->state.cs.pipeline;
   uint8_t push_save[NVK_MAX_PUSH_SIZE];
            nvk_CmdBindPipeline(nvk_cmd_buffer_to_handle(cmd),
            nvk_CmdPushConstants(nvk_cmd_buffer_to_handle(cmd), layout,
                     /* Restore pipeline and push constants */
   if (pipeline_save) {
      nvk_CmdBindPipeline(nvk_cmd_buffer_to_handle(cmd),
            }
      }
      void
   nvk_mme_copy_queries(struct mme_builder *b)
   {
      if (b->devinfo->cls_eng3d < TURING_A)
            struct mme_value64 dst_addr = mme_load_addr64(b);
   struct mme_value64 dst_stride = mme_load_addr64(b);
   struct mme_value64 avail_addr = mme_load_addr64(b);
            struct mme_value query_count = mme_load(b);
            struct mme_value flags = control;
   struct mme_value write64 =
         struct mme_value query_stride =
         struct mme_value is_timestamp =
            mme_while(b, ugt, query_count, mme_zero()) {
      struct mme_value dw_per_query = mme_srl(b, query_stride, mme_imm(2));
   mme_tu104_read_fifoed(b, report_addr, dw_per_query);
            struct mme_value64 write_addr = mme_mov64(b, dst_addr);
   struct mme_value report_count = mme_srl(b, query_stride, mme_imm(4));
   mme_while(b, ugt, report_count, mme_zero()) {
      struct mme_value result_lo = mme_alloc_reg(b);
                  mme_if(b, ine, is_timestamp, mme_zero()) {
      mme_load_to(b, mme_zero());
   mme_load_to(b, mme_zero());
   mme_load_to(b, result_lo);
   mme_load_to(b, result_hi);
      }
   mme_if(b, ieq, is_timestamp, mme_zero()) {
      struct mme_value begin_lo = mme_load(b);
   struct mme_value begin_hi = mme_load(b);
   struct mme_value64 begin = mme_value64(begin_lo, begin_hi);
                  struct mme_value end_lo = mme_load(b);
   struct mme_value end_hi = mme_load(b);
   struct mme_value64 end = mme_value64(end_lo, end_hi);
                                 mme_free_reg64(b, begin);
               mme_store_global(b, write_addr, result_lo);
   mme_add64_to(b, write_addr, write_addr, mme_imm64(4));
   mme_if(b, ine, write64, mme_zero()) {
      mme_store_global(b, write_addr, result_hi);
                  struct mme_value with_availability =
         mme_if(b, ine, with_availability, mme_zero()) {
      mme_tu104_read_fifoed(b, avail_addr, mme_imm(1));
   struct mme_value avail = mme_load(b);
   mme_store_global(b, write_addr, avail);
   mme_if(b, ine, write64, mme_zero()) {
      mme_add64_to(b, write_addr, write_addr, mme_imm64(4));
         }
                     mme_add64_to(b, report_addr, report_addr,
                           }
      static void
   nvk_cmd_copy_query_pool_results_mme(struct nvk_cmd_buffer *cmd,
                                       {
      /* TODO: vkCmdCopyQueryPoolResults() with a compute shader */
            struct nv_push *p = nvk_cmd_buffer_push(cmd, 13);
   P_IMMD(p, NVC597, SET_MME_DATA_FIFO_CONFIG, FIFO_SIZE_SIZE_4KB);
            P_INLINE_DATA(p, dst_addr >> 32);
   P_INLINE_DATA(p, dst_addr);
   P_INLINE_DATA(p, dst_stride >> 32);
            uint64_t avail_start = nvk_query_available_addr(pool, first_query);
   P_INLINE_DATA(p, avail_start >> 32);
            uint64_t report_start = nvk_query_report_addr(pool, first_query);
   P_INLINE_DATA(p, report_start >> 32);
                              uint32_t control = (flags & 0xff) |
                  }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer,
                              VkQueryPool queryPool,
   uint32_t firstQuery,
      {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
   VK_FROM_HANDLE(nvk_query_pool, pool, queryPool);
            if (flags & VK_QUERY_RESULT_WAIT_BIT) {
      for (uint32_t i = 0; i < queryCount; i++) {
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 5);
   __push_mthd(p, SUBC_NV9097, NV906F_SEMAPHOREA);
   P_NV906F_SEMAPHOREA(p, avail_addr >> 32);
   P_NV906F_SEMAPHOREB(p, (avail_addr & UINT32_MAX) >> 2);
   P_NV906F_SEMAPHOREC(p, 1);
   P_NV906F_SEMAPHORED(p, {
      .operation = OPERATION_ACQ_GEQ,
   .acquire_switch = ACQUIRE_SWITCH_ENABLED,
                     uint64_t dst_addr = nvk_buffer_address(dst_buffer, dstOffset);
   nvk_meta_copy_query_pool_results(cmd, pool, firstQuery, queryCount,
      }
