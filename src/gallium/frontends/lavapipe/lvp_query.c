   /*
   * Copyright © 2019 Red Hat.
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
      #include "lvp_private.h"
   #include "pipe/p_context.h"
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateQueryPool(
      VkDevice                                    _device,
   const VkQueryPoolCreateInfo*                pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
               enum pipe_query_type pipeq;
   switch (pCreateInfo->queryType) {
   case VK_QUERY_TYPE_OCCLUSION:
      pipeq = PIPE_QUERY_OCCLUSION_COUNTER;
      case VK_QUERY_TYPE_TIMESTAMP:
      pipeq = PIPE_QUERY_TIMESTAMP;
      case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      pipeq = PIPE_QUERY_SO_STATISTICS;
      case VK_QUERY_TYPE_PIPELINE_STATISTICS:
      pipeq = PIPE_QUERY_PIPELINE_STATISTICS;
      case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
   case VK_QUERY_TYPE_MESH_PRIMITIVES_GENERATED_EXT:
      pipeq = PIPE_QUERY_PRIMITIVES_GENERATED;
      default:
                  struct lvp_query_pool *pool;
   size_t pool_size = sizeof(*pool)
            pool = vk_zalloc2(&device->vk.alloc, pAllocator,
               if (!pool)
            vk_object_base_init(&device->vk, &pool->base,
         pool->type = pCreateInfo->queryType;
   pool->count = pCreateInfo->queryCount;
   pool->base_type = pipeq;
            *pQueryPool = lvp_query_pool_to_handle(pool);
      }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroyQueryPool(
      VkDevice                                    _device,
   VkQueryPool                                 _pool,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            if (!pool)
            for (unsigned i = 0; i < pool->count; i++)
      if (pool->queries[i])
      vk_object_base_finish(&pool->base);
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_GetQueryPoolResults(
      VkDevice                                    _device,
   VkQueryPool                                 queryPool,
   uint32_t                                    firstQuery,
   uint32_t                                    queryCount,
   size_t                                      dataSize,
   void*                                       pData,
   VkDeviceSize                                stride,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   LVP_FROM_HANDLE(lvp_query_pool, pool, queryPool);
                     for (unsigned i = firstQuery; i < firstQuery + queryCount; i++) {
      uint8_t *dest = (uint8_t *)((char *)pData + (stride * (i - firstQuery)));
   union pipe_query_result result;
            if (pool->queries[i]) {
      ready = device->queue.ctx->get_query_result(device->queue.ctx,
                  } else {
                  if (!ready && !(flags & VK_QUERY_RESULT_PARTIAL_BIT))
            if (flags & VK_QUERY_RESULT_64_BIT) {
      uint64_t *dest64 = (uint64_t *) dest;
   if (ready || (flags & VK_QUERY_RESULT_PARTIAL_BIT)) {
      if (pool->type == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
      uint32_t mask = pool->pipeline_stats;
   const uint64_t *pstats = result.pipeline_statistics.counters;
   while (mask) {
      uint32_t i = u_bit_scan(&mask);
         } else if (pool->type == VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT) {
      *dest64++ = result.so_statistics.num_primitives_written;
      } else {
            } else {
      if (pool->type == VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT) {
         } else {
            }
   if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
            } else {
      uint32_t *dest32 = (uint32_t *) dest;
   if (ready || (flags & VK_QUERY_RESULT_PARTIAL_BIT)) {
      if (pool->type == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
      uint32_t mask = pool->pipeline_stats;
   const uint64_t *pstats = result.pipeline_statistics.counters;
   while (mask) {
      uint32_t i = u_bit_scan(&mask);
         } else if (pool->type == VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT) {
      *dest32++ = (uint32_t)
         *dest32++ = (uint32_t)
      } else {
            } else {
      if (pool->type == VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT) {
         } else {
            }
   if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
               }
      }
      VKAPI_ATTR void VKAPI_CALL lvp_ResetQueryPool(
      VkDevice                                    _device,
   VkQueryPool                                 queryPool,
   uint32_t                                    firstQuery,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            for (uint32_t i = 0; i < queryCount; i++) {
               if (pool->queries[idx]) {
      device->queue.ctx->destroy_query(device->queue.ctx, pool->queries[idx]);
            }
