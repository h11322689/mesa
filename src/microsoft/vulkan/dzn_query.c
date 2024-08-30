   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "dzn_private.h"
   #include "dzn_abi_helper.h"
      #include "vk_alloc.h"
   #include "vk_debug_report.h"
   #include "vk_util.h"
      #include "util/os_time.h"
      static D3D12_QUERY_HEAP_TYPE
   dzn_query_pool_get_heap_type(VkQueryType in)
   {
      switch (in) {
   case VK_QUERY_TYPE_OCCLUSION: return D3D12_QUERY_HEAP_TYPE_OCCLUSION;
   case VK_QUERY_TYPE_PIPELINE_STATISTICS: return D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
   case VK_QUERY_TYPE_TIMESTAMP: return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
   default: unreachable("Unsupported query type");
      }
      D3D12_QUERY_TYPE
   dzn_query_pool_get_query_type(const struct dzn_query_pool *qpool,
         {
      switch (qpool->heap_type) {
   case D3D12_QUERY_HEAP_TYPE_OCCLUSION:
      return flags & VK_QUERY_CONTROL_PRECISE_BIT ?
      case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS: return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
   case D3D12_QUERY_HEAP_TYPE_TIMESTAMP: return D3D12_QUERY_TYPE_TIMESTAMP;
   default: unreachable("Unsupported query type");
      }
      static void
   dzn_query_pool_destroy(struct dzn_query_pool *qpool,
         {
      if (!qpool)
                     if (qpool->collect_map)
            if (qpool->collect_buffer)
            if (qpool->resolve_buffer)
            if (qpool->heap)
            for (uint32_t q = 0; q < qpool->query_count; q++) {
      if (qpool->queries[q].fence)
               mtx_destroy(&qpool->queries_lock);
   vk_object_base_finish(&qpool->base);
      }
      static VkResult
   dzn_query_pool_create(struct dzn_device *device,
                     {
      VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct dzn_query_pool, qpool, 1);
            if (!vk_multialloc_zalloc2(&ma, &device->vk.alloc, alloc,
                           mtx_init(&qpool->queries_lock, mtx_plain);
   qpool->query_count = info->queryCount;
            D3D12_QUERY_HEAP_DESC desc = { 0 };
   qpool->heap_type = desc.Type = dzn_query_pool_get_heap_type(info->queryType);
   desc.Count = info->queryCount;
            HRESULT hres =
      ID3D12Device1_CreateQueryHeap(device->dev, &desc,
            if (FAILED(hres)) {
      dzn_query_pool_destroy(qpool, alloc);
               switch (info->queryType) {
   case VK_QUERY_TYPE_OCCLUSION:
   case VK_QUERY_TYPE_TIMESTAMP:
      qpool->query_size = sizeof(uint64_t);
      case VK_QUERY_TYPE_PIPELINE_STATISTICS:
      qpool->pipeline_statistics = info->pipelineStatistics;
   qpool->query_size = sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
      default: unreachable("Unsupported query type");
            D3D12_HEAP_PROPERTIES hprops =
         D3D12_RESOURCE_DESC rdesc = {
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
   .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
   .Width = info->queryCount * qpool->query_size,
   .Height = 1,
   .DepthOrArraySize = 1,
   .MipLevels = 1,
   .Format = DXGI_FORMAT_UNKNOWN,
   .SampleDesc = { .Count = 1, .Quality = 0 },
   .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
               hres = ID3D12Device1_CreateCommittedResource(device->dev, &hprops,
                                       if (FAILED(hres)) {
      dzn_query_pool_destroy(qpool, alloc);
               hprops = dzn_ID3D12Device4_GetCustomHeapProperties(device->dev, 0,
         rdesc.Width = info->queryCount * (qpool->query_size + sizeof(uint64_t));
   hres = ID3D12Device1_CreateCommittedResource(device->dev, &hprops,
                                       if (FAILED(hres)) {
      dzn_query_pool_destroy(qpool, alloc);
               hres = ID3D12Resource_Map(qpool->collect_buffer, 0, NULL, (void **)&qpool->collect_map);
   if (FAILED(hres)) {
      dzn_query_pool_destroy(qpool, alloc);
                        *out = dzn_query_pool_to_handle(qpool);
      }
      uint32_t
   dzn_query_pool_get_result_offset(const struct dzn_query_pool *qpool, uint32_t query)
   {
         }
      uint32_t
   dzn_query_pool_get_result_size(const struct dzn_query_pool *qpool, uint32_t query_count)
   {
         }
      uint32_t
   dzn_query_pool_get_availability_offset(const struct dzn_query_pool *qpool, uint32_t query)
   {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateQueryPool(VkDevice device,
                     {
      return dzn_query_pool_create(dzn_device_from_handle(device),
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroyQueryPool(VkDevice device,
               {
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_ResetQueryPool(VkDevice device,
                     {
               mtx_lock(&qpool->queries_lock);
   for (uint32_t q = 0; q < queryCount; q++) {
               query->fence_value = 0;
   if (query->fence) {
      ID3D12Fence_Release(query->fence);
         }
            memset((uint8_t *)qpool->collect_map + dzn_query_pool_get_result_offset(qpool, firstQuery),
         memset((uint8_t *)qpool->collect_map + dzn_query_pool_get_availability_offset(qpool, firstQuery),
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_GetQueryPoolResults(VkDevice device,
                           VkQueryPool queryPool,
   uint32_t firstQuery,
   uint32_t queryCount,
   {
               uint32_t step = (flags & VK_QUERY_RESULT_64_BIT) ?
                  for (uint32_t q = 0; q < queryCount; q++) {
               uint8_t *dst_ptr = (uint8_t *)pData + (stride * q);
   uint8_t *src_ptr =
      (uint8_t *)qpool->collect_map +
               if (flags & VK_QUERY_RESULT_WAIT_BIT) {
                     while (true) {
      mtx_lock(&qpool->queries_lock);
   if (query->fence) {
      query_fence = query->fence;
      }
                                 /* Check again in 10ms.
   * FIXME: decrease the polling period if it happens to hurt latency.
   */
               ID3D12Fence_SetEventOnCompletion(query_fence, query_fence_val, NULL);
   ID3D12Fence_Release(query_fence);
      } else {
      ID3D12Fence *query_fence = NULL;
   mtx_lock(&qpool->queries_lock);
   if (query->fence) {
      query_fence = query->fence;
      }
                  if (query_fence) {
      if (ID3D12Fence_GetCompletedValue(query_fence) >= query_fence_val)
                        if (qpool->heap_type != D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS) {
      if (available)
                           } else {
      for (uint32_t c = 0; c < sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS) / sizeof(uint64_t); c++) {
                     if (available)
                                       if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT)
            if (!available && !(flags & VK_QUERY_RESULT_PARTIAL_BIT))
                  }
