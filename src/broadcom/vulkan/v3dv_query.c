   /*
   * Copyright © 2020 Raspberry Pi Ltd
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
      #include "v3dv_private.h"
      #include "util/timespec.h"
   #include "compiler/nir/nir_builder.h"
      static void
   kperfmon_create(struct v3dv_device *device,
               {
      for (uint32_t i = 0; i < pool->perfmon.nperfmons; i++) {
               struct drm_v3d_perfmon_create req = {
      .ncounters = MIN2(pool->perfmon.ncounters -
            };
   memcpy(req.counters,
                  int ret = v3dv_ioctl(device->pdevice->render_fd,
               if (ret)
                  }
      static void
   kperfmon_destroy(struct v3dv_device *device,
               {
      /* Skip destroying if never created */
   if (!pool->queries[query].perf.kperfmon_ids[0])
            for (uint32_t i = 0; i < pool->perfmon.nperfmons; i++) {
      struct drm_v3d_perfmon_destroy req = {
                  int ret = v3dv_ioctl(device->pdevice->render_fd,
                  if (ret) {
      fprintf(stderr, "Failed to destroy perfmon %u: %s\n",
            }
      /**
   * Creates a VkBuffer (and VkDeviceMemory) to access a BO.
   */
   static VkResult
   create_vk_storage_buffer(struct v3dv_device *device,
                     {
               VkBufferCreateInfo buf_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
   .size = bo->size,
      };
   VkResult result = v3dv_CreateBuffer(vk_device, &buf_info, NULL, vk_buf);
   if (result != VK_SUCCESS)
            struct v3dv_device_memory *mem =
      vk_object_zalloc(&device->vk, NULL, sizeof(*mem),
      if (!mem)
            mem->bo = bo;
            *vk_mem = v3dv_device_memory_to_handle(mem);
   VkBindBufferMemoryInfo bind_info = {
      .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
   .buffer = *vk_buf,
   .memory = *vk_mem,
      };
               }
      static void
   destroy_vk_storage_buffer(struct v3dv_device *device,
               {
      if (*vk_mem) {
      vk_object_free(&device->vk, NULL, v3dv_device_memory_from_handle(*vk_mem));
               v3dv_DestroyBuffer(v3dv_device_to_handle(device), *vk_buf, NULL);
      }
      /**
   * Allocates descriptor sets to access query pool BO (availability and
   * occlusion query results) from Vulkan pipelines.
   */
   static VkResult
   create_pool_descriptors(struct v3dv_device *device,
         {
      assert(pool->query_type == VK_QUERY_TYPE_OCCLUSION);
            VkDescriptorPoolSize pool_size = {
      .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      };
   VkDescriptorPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
   .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
   .maxSets = 1,
   .poolSizeCount = 1,
      };
   VkResult result =
      v3dv_CreateDescriptorPool(vk_device, &pool_info, NULL,
         if (result != VK_SUCCESS)
            VkDescriptorSetAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
   .descriptorPool = pool->meta.descriptor_pool,
   .descriptorSetCount = 1,
      };
   result = v3dv_AllocateDescriptorSets(vk_device, &alloc_info,
         if (result != VK_SUCCESS)
            VkDescriptorBufferInfo desc_buf_info = {
      .buffer = pool->meta.buf,
   .offset = 0,
               VkWriteDescriptorSet write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstSet = pool->meta.descriptor_set,
   .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      };
               }
      static void
   destroy_pool_descriptors(struct v3dv_device *device,
         {
               v3dv_FreeDescriptorSets(v3dv_device_to_handle(device),
                        v3dv_DestroyDescriptorPool(v3dv_device_to_handle(device),
            }
      static VkResult
   pool_create_meta_resources(struct v3dv_device *device,
         {
               if (pool->query_type != VK_QUERY_TYPE_OCCLUSION)
            result = create_vk_storage_buffer(device, pool->occlusion.bo,
         if (result != VK_SUCCESS)
            result = create_pool_descriptors(device, pool);
   if (result != VK_SUCCESS)
               }
      static void
   pool_destroy_meta_resources(struct v3dv_device *device,
         {
      if (pool->query_type != VK_QUERY_TYPE_OCCLUSION)
            destroy_pool_descriptors(device, pool);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateQueryPool(VkDevice _device,
                     {
               assert(pCreateInfo->queryType == VK_QUERY_TYPE_OCCLUSION ||
         pCreateInfo->queryType == VK_QUERY_TYPE_TIMESTAMP ||
            struct v3dv_query_pool *pool =
      vk_object_zalloc(&device->vk, pAllocator, sizeof(*pool),
      if (pool == NULL)
            pool->query_type = pCreateInfo->queryType;
            uint32_t query_idx = 0;
            const uint32_t pool_bytes = sizeof(struct v3dv_query) * pool->query_count;
   pool->queries = vk_alloc2(&device->vk.alloc, pAllocator, pool_bytes, 8,
         if (pool->queries == NULL) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               switch (pool->query_type) {
   case VK_QUERY_TYPE_OCCLUSION: {
      /* The hardware allows us to setup groups of 16 queries in consecutive
   * 4-byte addresses, requiring only that each group of 16 queries is
   * aligned to a 1024 byte boundary.
   */
   const uint32_t query_groups = DIV_ROUND_UP(pool->query_count, 16);
   uint32_t bo_size = query_groups * 1024;
   /* After the counters we store avalability data, 1 byte/query */
   pool->occlusion.avail_offset = bo_size;
   bo_size += pool->query_count;
   pool->occlusion.bo = v3dv_bo_alloc(device, bo_size, "query:o", true);
   if (!pool->occlusion.bo) {
      result = vk_error(device, VK_ERROR_OUT_OF_DEVICE_MEMORY);
      }
   if (!v3dv_bo_map(device, pool->occlusion.bo, bo_size)) {
      result = vk_error(device, VK_ERROR_OUT_OF_DEVICE_MEMORY);
      }
      }
   case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      const VkQueryPoolPerformanceCreateInfoKHR *pq_info =
                           pool->perfmon.ncounters = pq_info->counterIndexCount;
   for (uint32_t i = 0; i < pq_info->counterIndexCount; i++)
            pool->perfmon.nperfmons = DIV_ROUND_UP(pool->perfmon.ncounters,
            assert(pool->perfmon.nperfmons <= V3DV_MAX_PERFMONS);
      }
   case VK_QUERY_TYPE_TIMESTAMP:
         default:
                  /* Initialize queries in the pool */
   for (; query_idx < pool->query_count; query_idx++) {
      pool->queries[query_idx].maybe_available = false;
   switch (pool->query_type) {
   case VK_QUERY_TYPE_OCCLUSION: {
      const uint32_t query_group = query_idx / 16;
   const uint32_t query_offset = query_group * 1024 + (query_idx % 16) * 4;
   pool->queries[query_idx].occlusion.offset = query_offset;
   break;
      case VK_QUERY_TYPE_TIMESTAMP:
      pool->queries[query_idx].value = 0;
      case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR: {
      result = vk_sync_create(&device->vk,
                              for (uint32_t j = 0; j < pool->perfmon.nperfmons; j++)
         break;
      default:
                     /* Create meta resources */
   result = pool_create_meta_resources(device, pool);
   if (result != VK_SUCCESS)
                           fail:
      if (pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      for (uint32_t j = 0; j < query_idx; j++)
               if (pool->occlusion.bo)
         if (pool->queries)
         pool_destroy_meta_resources(device, pool);
               }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyQueryPool(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (!pool)
            if (pool->occlusion.bo)
            if (pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      for (uint32_t i = 0; i < pool->query_count; i++) {
      kperfmon_destroy(device, pool, i);
                  if (pool->queries)
                        }
      static void
   write_to_buffer(void *dst, uint32_t idx, bool do_64bit, uint64_t value)
   {
      if (do_64bit) {
      uint64_t *dst64 = (uint64_t *) dst;
      } else {
      uint32_t *dst32 = (uint32_t *) dst;
         }
      static VkResult
   query_wait_available(struct v3dv_device *device,
                     {
      /* For occlusion queries we prefer to poll the availability BO in a loop
   * to waiting on the occlusion query results BO, because the latter would
   * make us wait for any job running occlusion queries, even if those queries
   * do not involve the one we want to wait on.
   */
   if (pool->query_type == VK_QUERY_TYPE_OCCLUSION) {
      uint8_t *q_addr = ((uint8_t *) pool->occlusion.bo->map) +
         while (*q_addr == 0)
                     /* For other queries we need to wait for the queue to signal that
   * the query has been submitted for execution before anything else.
   */
   assert(pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR ||
            VkResult result = VK_SUCCESS;
   if (!q->maybe_available) {
      struct timespec timeout;
   timespec_get(&timeout, TIME_UTC);
            mtx_lock(&device->query_mutex);
   while (!q->maybe_available) {
      if (vk_device_is_lost(&device->vk)) {
      result = VK_ERROR_DEVICE_LOST;
               int ret = cnd_timedwait(&device->query_ended,
               if (ret != thrd_success) {
      mtx_unlock(&device->query_mutex);
   result = vk_device_set_lost(&device->vk, "Query wait failed");
         }
            if (result != VK_SUCCESS)
            /* For performance queries, we also need to wait for the relevant syncobj
   * to be signaled to ensure completion of the GPU work.
   */
   if (pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR &&
      vk_sync_wait(&device->vk, q->perf.last_job_sync,
      return vk_device_set_lost(&device->vk, "Query job wait failed");
                  }
      static VkResult
   query_check_available(struct v3dv_device *device,
                     {
      /* For occlusion and performance queries we check the availability BO */
   if (pool->query_type == VK_QUERY_TYPE_OCCLUSION) {
      const uint8_t *q_addr = ((uint8_t *) pool->occlusion.bo->map) +
                     /* For other queries we need to check if the queue has submitted the query
   * for execution at all.
   */
   assert(pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR ||
         if (!q->maybe_available)
            /* For performance queries, we also need to check if the relevant GPU job
   * has completed.
   */
   if (pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR &&
      vk_sync_wait(&device->vk, q->perf.last_job_sync,
                        }
      static VkResult
   query_is_available(struct v3dv_device *device,
                     struct v3dv_query_pool *pool,
   {
               assert(pool->query_type != VK_QUERY_TYPE_OCCLUSION ||
            if (do_wait) {
      VkResult result = query_wait_available(device, pool, q, query);
   if (result != VK_SUCCESS) {
      *available = false;
                  } else {
      VkResult result = query_check_available(device, pool, q, query);
   assert(result == VK_SUCCESS || result == VK_NOT_READY);
                  }
      static VkResult
   write_occlusion_query_result(struct v3dv_device *device,
                                 {
               if (vk_device_is_lost(&device->vk))
            struct v3dv_query *q = &pool->queries[query];
            const uint8_t *query_addr =
         write_to_buffer(data, slot, do_64bit, (uint64_t) *((uint32_t *)query_addr));
      }
      static VkResult
   write_timestamp_query_result(struct v3dv_device *device,
                                 {
                        write_to_buffer(data, slot, do_64bit, q->value);
      }
      static VkResult
   write_performance_query_result(struct v3dv_device *device,
                                 {
               struct v3dv_query *q = &pool->queries[query];
            for (uint32_t i = 0; i < pool->perfmon.nperfmons; i++) {
      struct drm_v3d_perfmon_get_values req = {
      .id = q->perf.kperfmon_ids[i],
   .values_ptr = (uintptr_t)(&counter_values[i *
               int ret = v3dv_ioctl(device->pdevice->render_fd,
                  if (ret) {
      fprintf(stderr, "failed to get perfmon values: %s\n", strerror(ret));
                  for (uint32_t i = 0; i < pool->perfmon.ncounters; i++)
               }
      static VkResult
   write_query_result(struct v3dv_device *device,
                     struct v3dv_query_pool *pool,
   uint32_t query,
   {
      switch (pool->query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
      return write_occlusion_query_result(device, pool, query, do_64bit,
      case VK_QUERY_TYPE_TIMESTAMP:
      return write_timestamp_query_result(device, pool, query, do_64bit,
      case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR:
      return write_performance_query_result(device, pool, query, do_64bit,
      default:
            }
      static uint32_t
   get_query_result_count(struct v3dv_query_pool *pool)
   {
      switch (pool->query_type) {
   case VK_QUERY_TYPE_OCCLUSION:
   case VK_QUERY_TYPE_TIMESTAMP:
         case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR:
         default:
            }
      VkResult
   v3dv_get_query_pool_results_cpu(struct v3dv_device *device,
                                 struct v3dv_query_pool *pool,
   {
      assert(first < pool->query_count);
   assert(first + count <= pool->query_count);
            const bool do_64bit = flags & VK_QUERY_RESULT_64_BIT ||
         const bool do_wait = flags & VK_QUERY_RESULT_WAIT_BIT;
                     VkResult result = VK_SUCCESS;
   for (uint32_t i = first; i < first + count; i++) {
      bool available = false;
   VkResult query_result =
         if (query_result == VK_ERROR_DEVICE_LOST)
            /**
   * From the Vulkan 1.0 spec:
   *
   *    "If VK_QUERY_RESULT_WAIT_BIT and VK_QUERY_RESULT_PARTIAL_BIT are
   *     both not set then no result values are written to pData for queries
   *     that are in the unavailable state at the time of the call, and
   *     vkGetQueryPoolResults returns VK_NOT_READY. However, availability
   *     state is still written to pData for those queries if
   *     VK_QUERY_RESULT_WITH_AVAILABILITY_BIT is set."
   */
            const bool write_result = available || do_partial;
   if (write_result)
                  if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT)
            if (!write_result && result != VK_ERROR_DEVICE_LOST)
                           }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_GetQueryPoolResults(VkDevice _device,
                           VkQueryPool queryPool,
   uint32_t firstQuery,
   uint32_t queryCount,
   {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            return v3dv_get_query_pool_results_cpu(device, pool, firstQuery, queryCount,
      }
      /* Emits a series of vkCmdDispatchBase calls to execute all the workgroups
   * required to handle a number of queries considering per-dispatch limits.
   */
   static void
   cmd_buffer_emit_dispatch_queries(struct v3dv_cmd_buffer *cmd_buffer,
         {
               uint32_t dispatched = 0;
   const uint32_t max_batch_size = 65535;
   while (dispatched < query_count) {
      uint32_t batch_size = MIN2(query_count - dispatched, max_batch_size);
   v3dv_CmdDispatchBase(vk_cmd_buffer, dispatched, 0, 0, batch_size, 1, 1);
         }
      void
   v3dv_cmd_buffer_emit_set_query_availability(struct v3dv_cmd_buffer *cmd_buffer,
                     {
      assert(pool->query_type == VK_QUERY_TYPE_OCCLUSION ||
            struct v3dv_device *device = cmd_buffer->device;
            /* We are about to emit a compute job to set query availability and we need
   * to ensure this executes after the graphics work using the queries has
   * completed.
   */
   VkMemoryBarrier2 barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
   .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      };
   VkDependencyInfo barrier_info = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   .memoryBarrierCount = 1,
      };
            /* Dispatch queries */
            v3dv_CmdBindPipeline(vk_cmd_buffer,
                  v3dv_CmdBindDescriptorSets(vk_cmd_buffer,
                              struct {
      uint32_t offset;
   uint32_t query;
      } push_data = { pool->occlusion.avail_offset, query, availability };
   v3dv_CmdPushConstants(vk_cmd_buffer,
                                 }
      static void
   cmd_buffer_emit_reset_occlusion_query_pool(struct v3dv_cmd_buffer *cmd_buffer,
               {
      struct v3dv_device *device = cmd_buffer->device;
            /* Ensure the GPU is done with the queries in the graphics queue before
   * we reset in the compute queue.
   */
   VkMemoryBarrier2 barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
   .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      };
   VkDependencyInfo barrier_info = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   .memoryBarrierCount = 1,
      };
            /* Emit compute reset */
            v3dv_CmdBindPipeline(vk_cmd_buffer,
                  v3dv_CmdBindDescriptorSets(vk_cmd_buffer,
                           struct {
      uint32_t offset;
      } push_data = { pool->occlusion.avail_offset, query };
   v3dv_CmdPushConstants(vk_cmd_buffer,
                                          /* Ensure future work in the graphics queue using the queries doesn't start
   * before the reset completed.
   */
   barrier = (VkMemoryBarrier2) {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
   .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
      };
   barrier_info = (VkDependencyInfo) {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   .memoryBarrierCount = 1,
      };
      }
      static void
   cmd_buffer_emit_reset_query_pool(struct v3dv_cmd_buffer *cmd_buffer,
               {
      assert(pool->query_type == VK_QUERY_TYPE_OCCLUSION);
      }
      static void
   cmd_buffer_emit_reset_query_pool_cpu(struct v3dv_cmd_buffer *cmd_buffer,
               {
               struct v3dv_job *job =
      v3dv_cmd_buffer_create_cpu_job(cmd_buffer->device,
            v3dv_return_if_oom(cmd_buffer, NULL);
   job->cpu.query_reset.pool = pool;
   job->cpu.query_reset.first = first;
   job->cpu.query_reset.count = count;
      }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_CmdResetQueryPool(VkCommandBuffer commandBuffer,
                     {
      V3DV_FROM_HANDLE(v3dv_cmd_buffer, cmd_buffer, commandBuffer);
            /* Resets can only happen outside a render pass instance so we should not
   * be in the middle of job recording.
   */
   assert(cmd_buffer->state.pass == NULL);
            assert(firstQuery < pool->query_count);
            /* We can reset occlusion queries in the GPU, but for other query types
   * we emit a CPU job that will call v3dv_reset_query_pool_cpu when executed
   * in the queue.
   */
   if (pool->query_type == VK_QUERY_TYPE_OCCLUSION) {
         } else {
      cmd_buffer_emit_reset_query_pool_cpu(cmd_buffer, pool,
         }
      /**
   * Creates a descriptor pool so we can create a descriptors for the destination
   * buffers of vkCmdCopyQueryResults for queries where this is implemented in
   * the GPU.
   */
   static VkResult
   create_storage_buffer_descriptor_pool(struct v3dv_cmd_buffer *cmd_buffer)
   {
      /* If this is not the first pool we create one for this command buffer
   * size it based on the size of the currently exhausted pool.
   */
   uint32_t descriptor_count = 32;
   if (cmd_buffer->meta.query.dspool != VK_NULL_HANDLE) {
      struct v3dv_descriptor_pool *exhausted_pool =
                     /* Create the descriptor pool */
   cmd_buffer->meta.query.dspool = VK_NULL_HANDLE;
   VkDescriptorPoolSize pool_size = {
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
      };
   VkDescriptorPoolCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
   .maxSets = descriptor_count,
   .poolSizeCount = 1,
   .pPoolSizes = &pool_size,
      };
   VkResult result =
      v3dv_CreateDescriptorPool(v3dv_device_to_handle(cmd_buffer->device),
                     if (result == VK_SUCCESS) {
      assert(cmd_buffer->meta.query.dspool != VK_NULL_HANDLE);
            v3dv_cmd_buffer_add_private_obj(
                  struct v3dv_descriptor_pool *pool =
                        }
      static VkResult
   allocate_storage_buffer_descriptor_set(struct v3dv_cmd_buffer *cmd_buffer,
         {
      /* Make sure we have a descriptor pool */
   VkResult result;
   if (cmd_buffer->meta.query.dspool == VK_NULL_HANDLE) {
      result = create_storage_buffer_descriptor_pool(cmd_buffer);
   if (result != VK_SUCCESS)
      }
            /* Allocate descriptor set */
   struct v3dv_device *device = cmd_buffer->device;
   VkDevice vk_device = v3dv_device_to_handle(device);
   VkDescriptorSetAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
   .descriptorPool = cmd_buffer->meta.query.dspool,
   .descriptorSetCount = 1,
      };
            /* If we ran out of pool space, grow the pool and try again */
   if (result == VK_ERROR_OUT_OF_POOL_MEMORY) {
      result = create_storage_buffer_descriptor_pool(cmd_buffer);
   if (result == VK_SUCCESS) {
      info.descriptorPool = cmd_buffer->meta.query.dspool;
                     }
      static uint32_t
   copy_pipeline_index_from_flags(VkQueryResultFlags flags)
   {
      uint32_t index = 0;
   if (flags & VK_QUERY_RESULT_64_BIT)
         if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT)
         if (flags & VK_QUERY_RESULT_PARTIAL_BIT)
         assert(index < 8);
      }
      static nir_shader *
   get_copy_query_results_cs(VkQueryResultFlags flags);
      static void
   cmd_buffer_emit_copy_query_pool_results(struct v3dv_cmd_buffer *cmd_buffer,
                                 {
      struct v3dv_device *device = cmd_buffer->device;
   VkDevice vk_device = v3dv_device_to_handle(device);
            /* Create the required copy pipeline if not yet created */
   uint32_t pipeline_idx = copy_pipeline_index_from_flags(flags);
   if (!device->queries.copy_pipeline[pipeline_idx]) {
      nir_shader *copy_query_results_cs_nir = get_copy_query_results_cs(flags);
   VkResult result =
      v3dv_create_compute_pipeline_from_nir(
         device, copy_query_results_cs_nir,
      ralloc_free(copy_query_results_cs_nir);
   if (result != VK_SUCCESS) {
      fprintf(stderr, "Failed to create copy query results pipeline\n");
                  /* FIXME: do we need this barrier? Since vkCmdEndQuery should've been called
   * and that already waits maybe we don't (since this is serialized
   * in the compute queue with EndQuery anyway).
   */
   if (flags & VK_QUERY_RESULT_WAIT_BIT) {
      VkMemoryBarrier2 barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
   .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      };
   VkDependencyInfo barrier_info = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   .memoryBarrierCount = 1,
      };
               /* Allocate and setup descriptor set for output buffer */
   VkDescriptorSet out_buf_descriptor_set;
   VkResult result =
      allocate_storage_buffer_descriptor_set(cmd_buffer,
      if (result != VK_SUCCESS) {
      fprintf(stderr, "vkCmdCopyQueryPoolResults failed: "
                     VkDescriptorBufferInfo desc_buf_info = {
      .buffer = v3dv_buffer_to_handle(buf),
   .offset = 0,
      };
   VkWriteDescriptorSet write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstSet = out_buf_descriptor_set,
   .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      };
            /* Dispatch copy */
            assert(device->queries.copy_pipeline[pipeline_idx]);
   v3dv_CmdBindPipeline(vk_cmd_buffer,
                  VkDescriptorSet sets[2] = {
      pool->meta.descriptor_set,
      };
   v3dv_CmdBindDescriptorSets(vk_cmd_buffer,
                        struct {
         } push_data = { pool->occlusion.avail_offset, first, offset, stride, flags };
   v3dv_CmdPushConstants(vk_cmd_buffer,
                                    }
      static void
   cmd_buffer_emit_copy_query_pool_results_cpu(struct v3dv_cmd_buffer *cmd_buffer,
                                             {
      struct v3dv_job *job =
      v3dv_cmd_buffer_create_cpu_job(cmd_buffer->device,
                     job->cpu.query_copy_results.pool = pool;
   job->cpu.query_copy_results.first = first;
   job->cpu.query_copy_results.count = count;
   job->cpu.query_copy_results.dst = dst;
   job->cpu.query_copy_results.offset = offset;
   job->cpu.query_copy_results.stride = stride;
               }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer,
                              VkQueryPool queryPool,
   uint32_t firstQuery,
      {
      V3DV_FROM_HANDLE(v3dv_cmd_buffer, cmd_buffer, commandBuffer);
   V3DV_FROM_HANDLE(v3dv_query_pool, pool, queryPool);
            /* Copies can only happen outside a render pass instance so we should not
   * be in the middle of job recording.
   */
   assert(cmd_buffer->state.pass == NULL);
            assert(firstQuery < pool->query_count);
            /* For occlusion queries we implement the copy in the GPU but for other
   * queries we emit a CPU job that will call v3dv_get_query_pool_results_cpu
   * when executed in the queue.
   */
   if (pool->query_type == VK_QUERY_TYPE_OCCLUSION) {
      cmd_buffer_emit_copy_query_pool_results(cmd_buffer, pool,
                  } else {
      cmd_buffer_emit_copy_query_pool_results_cpu(cmd_buffer, pool,
                     }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_CmdBeginQuery(VkCommandBuffer commandBuffer,
                     {
      V3DV_FROM_HANDLE(v3dv_cmd_buffer, cmd_buffer, commandBuffer);
               }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_CmdEndQuery(VkCommandBuffer commandBuffer,
               {
      V3DV_FROM_HANDLE(v3dv_cmd_buffer, cmd_buffer, commandBuffer);
               }
      void
   v3dv_reset_query_pool_cpu(struct v3dv_device *device,
                     {
               for (uint32_t i = first; i < first + count; i++) {
      assert(i < pool->query_count);
   struct v3dv_query *q = &pool->queries[i];
   q->maybe_available = false;
   switch (pool->query_type) {
   case VK_QUERY_TYPE_OCCLUSION: {
      /* Reset availability */
   uint8_t *base_addr = ((uint8_t *) pool->occlusion.bo->map) +
                  /* Reset occlusion counter */
   const uint8_t *q_addr =
         uint32_t *counter = (uint32_t *) q_addr;
   *counter = 0;
      }
   case VK_QUERY_TYPE_TIMESTAMP:
      q->value = 0;
      case VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR:
      kperfmon_destroy(device, pool, i);
   kperfmon_create(device, pool, i);
   if (vk_sync_reset(&device->vk, q->perf.last_job_sync) != VK_SUCCESS)
            default:
                        }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_ResetQueryPool(VkDevice _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(
      VkPhysicalDevice physicalDevice,
   uint32_t queueFamilyIndex,
   uint32_t *pCounterCount,
   VkPerformanceCounterKHR *pCounters,
      {
               return v3dv_X(pDevice, enumerate_performance_query_counters)(pCounterCount,
            }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(
      VkPhysicalDevice physicalDevice,
   const VkQueryPoolPerformanceCreateInfoKHR *pPerformanceQueryCreateInfo,
      {
      *pNumPasses = DIV_ROUND_UP(pPerformanceQueryCreateInfo->counterIndexCount,
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_AcquireProfilingLockKHR(
      VkDevice _device,
      {
         }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_ReleaseProfilingLockKHR(VkDevice device)
   {
   }
      static inline void
   nir_set_query_availability(nir_builder *b,
                           {
      offset = nir_iadd(b, offset, query_idx); /* we use 1B per query */
      }
      static inline nir_def *
   nir_get_query_availability(nir_builder *b,
                     {
      offset = nir_iadd(b, offset, query_idx); /* we use 1B per query */
   nir_def *avail = nir_load_ssbo(b, 1, 8, buf, offset, .align_mul = 1);
      }
      static nir_shader *
   get_set_query_availability_cs()
   {
      const nir_shader_compiler_options *options = v3dv_pipeline_get_nir_options();
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, options,
            nir_def *buf =
      nir_vulkan_resource_index(&b, 2, 32, nir_imm_int(&b, 0),
                     /* This assumes a local size of 1 and a horizontal-only dispatch. If we
   * ever change any of these parameters we need to update how we compute the
   * query index here.
   */
            nir_def *offset =
            nir_def *query_idx =
            nir_def *avail =
            query_idx = nir_iadd(&b, query_idx, wg_id);
               }
      static inline nir_def *
   nir_get_occlusion_counter_offset(nir_builder *b, nir_def *query_idx)
   {
      nir_def *query_group = nir_udiv_imm(b, query_idx, 16);
   nir_def *query_group_offset = nir_umod_imm(b, query_idx, 16);
   nir_def *offset =
      nir_iadd(b, nir_imul_imm(b, query_group, 1024),
         }
      static inline void
   nir_reset_occlusion_counter(nir_builder *b,
               {
      nir_def *offset = nir_get_occlusion_counter_offset(b, query_idx);
   nir_def *zero = nir_imm_int(b, 0);
      }
      static inline nir_def *
   nir_read_occlusion_counter(nir_builder *b,
               {
      nir_def *offset = nir_get_occlusion_counter_offset(b, query_idx);
      }
      static nir_shader *
   get_reset_occlusion_query_cs()
   {
      const nir_shader_compiler_options *options = v3dv_pipeline_get_nir_options();
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, options,
            nir_def *buf =
      nir_vulkan_resource_index(&b, 2, 32, nir_imm_int(&b, 0),
                     /* This assumes a local size of 1 and a horizontal-only dispatch. If we
   * ever change any of these parameters we need to update how we compute the
   * query index here.
   */
            nir_def *avail_offset =
            nir_def *base_query_idx =
                     nir_set_query_availability(&b, buf, avail_offset, query_idx,
                     }
      static void
   write_query_buffer(nir_builder *b,
                     nir_def *buf,
   {
      if (flag_64bit) {
      /* Create a 64-bit value using a vec2 with the .Y component set to 0
   * so we can write a 64-bit value in a single store.
   */
   nir_def *value64 = nir_vec2(b, value, nir_imm_int(b, 0));
   nir_store_ssbo(b, value64, buf, *offset, .write_mask = 0x3, .align_mul = 8);
      } else {
      nir_store_ssbo(b, value, buf, *offset, .write_mask = 0x1, .align_mul = 4);
         }
      static nir_shader *
   get_copy_query_results_cs(VkQueryResultFlags flags)
   {
      bool flag_64bit = flags & VK_QUERY_RESULT_64_BIT;
   bool flag_avail = flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT;
            const nir_shader_compiler_options *options = v3dv_pipeline_get_nir_options();
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, options,
            nir_def *buf =
      nir_vulkan_resource_index(&b, 2, 32, nir_imm_int(&b, 0),
                     nir_def *buf_out =
      nir_vulkan_resource_index(&b, 2, 32, nir_imm_int(&b, 0),
                     /* Read push constants */
   nir_def *avail_offset =
            nir_def *base_query_idx =
            nir_def *base_offset_out =
            nir_def *stride =
            /* This assumes a local size of 1 and a horizontal-only dispatch. If we
   * ever change any of these parameters we need to update how we compute the
   * query index here.
   */
   nir_def *wg_id = nir_channel(&b, nir_load_workgroup_id(&b), 0);
            /* Read query availability if needed */
   nir_def *avail = NULL;
   if (flag_avail || !flag_partial)
            /* Write occusion query result... */
   nir_def *offset =
            /* ...if partial is requested, we always write */
   if(flag_partial) {
      nir_def *query_res = nir_read_occlusion_counter(&b, buf, query_idx);
      } else {
      /*...otherwise, we only write if the query is available */
   nir_if *if_stmt = nir_push_if(&b, nir_ine_imm(&b, avail, 0));
      nir_def *query_res = nir_read_occlusion_counter(&b, buf, query_idx);
                  /* Write query availability */
   if (flag_avail)
               }
      static bool
   create_query_pipelines(struct v3dv_device *device)
   {
      VkResult result;
            /* Set layout: single storage buffer */
   if (!device->queries.buf_descriptor_set_layout) {
      VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
      .binding = 0,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
   .descriptorCount = 1,
      };
   VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
   .bindingCount = 1,
      };
   result =
      v3dv_CreateDescriptorSetLayout(v3dv_device_to_handle(device),
                  if (result != VK_SUCCESS)
               /* Set availability pipeline.
   *
   * Pipeline layout:
   *  - 1 storage buffer for the BO with the query availability.
   *  - 2 push constants:
   *    0B: offset of the availability info in the buffer (4 bytes)
   *    4B: base query index (4 bytes).
   *    8B: availability (1 byte).
   */
   if (!device->queries.avail_pipeline_layout) {
      VkPipelineLayoutCreateInfo pipeline_layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &device->queries.buf_descriptor_set_layout,
   .pushConstantRangeCount = 1,
   .pPushConstantRanges =
               result =
      v3dv_CreatePipelineLayout(v3dv_device_to_handle(device),
                     if (result != VK_SUCCESS)
               if (!device->queries.avail_pipeline) {
      nir_shader *set_query_availability_cs_nir = get_set_query_availability_cs();
   result = v3dv_create_compute_pipeline_from_nir(device,
                     ralloc_free(set_query_availability_cs_nir);
   if (result != VK_SUCCESS)
                        /* Reset occlusion query pipeline.
   *
   * Pipeline layout:
   *  - 1 storage buffer for the BO with the occlusion and availability data.
   *  - Push constants:
   *    0B: offset of the availability info in the buffer (4B)
   *    4B: base query index (4B)
   */
   if (!device->queries.reset_occlusion_pipeline_layout) {
      VkPipelineLayoutCreateInfo pipeline_layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &device->queries.buf_descriptor_set_layout,
   .pushConstantRangeCount = 1,
   .pPushConstantRanges =
               result =
      v3dv_CreatePipelineLayout(v3dv_device_to_handle(device),
                     if (result != VK_SUCCESS)
               if (!device->queries.reset_occlusion_pipeline) {
      nir_shader *reset_occlusion_query_cs_nir = get_reset_occlusion_query_cs();
   result = v3dv_create_compute_pipeline_from_nir(
               device,
   reset_occlusion_query_cs_nir,
   ralloc_free(reset_occlusion_query_cs_nir);
   if (result != VK_SUCCESS)
                        /* Copy query results pipelines.
   *
   * Pipeline layout:
   *  - 1 storage buffer for the BO with the query availability and occlusion.
   *  - 1 storage buffer for the output.
   *  - Push constants:
   *    0B: offset of the availability info in the buffer (4B)
   *    4B: base query index (4B)
   *    8B: offset into output buffer (4B)
   *    12B: stride (4B)
   *
   * We create multiple specialized pipelines depending on the copy flags
   * to remove conditionals from the copy shader and get more optimized
   * pipelines.
   */
   if (!device->queries.copy_pipeline_layout) {
      VkDescriptorSetLayout set_layouts[2] = {
      device->queries.buf_descriptor_set_layout,
      };
   VkPipelineLayoutCreateInfo pipeline_layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 2,
   .pSetLayouts = set_layouts,
   .pushConstantRangeCount = 1,
   .pPushConstantRanges =
               result =
      v3dv_CreatePipelineLayout(v3dv_device_to_handle(device),
                     if (result != VK_SUCCESS)
               /* Actual copy pipelines are created lazily on demand since there can be up
   * to 8 depending on the flags used, however it is likely that applications
   * will use the same flags every time and only one pipeline is required.
               }
      static void
   destroy_query_pipelines(struct v3dv_device *device)
   {
               /* Availability pipeline */
   v3dv_DestroyPipeline(_device, device->queries.avail_pipeline,
         device->queries.avail_pipeline = VK_NULL_HANDLE;
   v3dv_DestroyPipelineLayout(_device, device->queries.avail_pipeline_layout,
                  /* Reset occlusion pipeline */
   v3dv_DestroyPipeline(_device, device->queries.reset_occlusion_pipeline,
         device->queries.reset_occlusion_pipeline = VK_NULL_HANDLE;
   v3dv_DestroyPipelineLayout(_device,
                        /* Copy pipelines */
   for (int i = 0; i < 8; i++) {
      v3dv_DestroyPipeline(_device, device->queries.copy_pipeline[i],
            }
   v3dv_DestroyPipelineLayout(_device, device->queries.copy_pipeline_layout,
                  v3dv_DestroyDescriptorSetLayout(_device,
                  }
      /**
   * Allocates device resources for implementing certain types of queries.
   */
   VkResult
   v3dv_query_allocate_resources(struct v3dv_device *device)
   {
      if (!create_query_pipelines(device))
               }
      void
   v3dv_query_free_resources(struct v3dv_device *device)
   {
         }
