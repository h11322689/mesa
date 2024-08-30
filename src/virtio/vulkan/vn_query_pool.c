   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_query_pool.h"
      #include "venus-protocol/vn_protocol_driver_query_pool.h"
      #include "vn_device.h"
   #include "vn_feedback.h"
   #include "vn_physical_device.h"
      /* query pool commands */
      VkResult
   vn_CreateQueryPool(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            struct vn_query_pool *pool =
      vk_zalloc(alloc, sizeof(*pool), VN_DEFAULT_ALIGN,
      if (!pool)
                              switch (pCreateInfo->queryType) {
   case VK_QUERY_TYPE_OCCLUSION:
      /*
   * Occlusion queries write one integer value - the number of samples
   * passed.
   */
   pool->result_array_size = 1;
      case VK_QUERY_TYPE_PIPELINE_STATISTICS:
      /*
   * Pipeline statistics queries write one integer value for each bit that
   * is enabled in the pipelineStatistics when the pool is created, and
   * the statistics values are written in bit order starting from the
   * least significant bit.
   */
   pool->result_array_size =
            case VK_QUERY_TYPE_TIMESTAMP:
      /*  Timestamp queries write one integer value. */
   pool->result_array_size = 1;
      case VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT:
      /*
   * Transform feedback queries write two integers; the first integer is
   * the number of primitives successfully written to the corresponding
   * transform feedback buffer and the second is the number of primitives
   * output to the vertex stream, regardless of whether they were
   * successfully captured or not.
   */
   pool->result_array_size = 2;
      case VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT:
      /*
   * Primitives generated queries write one integer value; the number of
   * primitives output to the vertex stream, regardless of whether
   * transform feedback is active or not, or whether they were
   * successfully captured by transform feedback or not. This is identical
   * to the second integer of the transform feedback queries if transform
   * feedback is active.
   */
   pool->result_array_size = 1;
      default:
      unreachable("bad query type");
               if (!VN_PERF(NO_QUERY_FEEDBACK)) {
      /* Feedback results are always 64 bit and include availability bit
   * (also 64 bit)
   */
   const uint32_t slot_size = (pool->result_array_size * 8) + 8;
   VkResult result = vn_feedback_buffer_create(
         if (result != VK_SUCCESS) {
      vn_object_base_fini(&pool->base);
   vk_free(alloc, pool);
                  /* Venus has to handle overflow behavior with query feedback to keep
   * consistency between vkCmdCopyQueryPoolResults and vkGetQueryPoolResults.
   * The default query feedback behavior is to wrap on overflow. However, per
   * spec:
   *
   * If an unsigned integer query’s value overflows the result type, the
   * value may either wrap or saturate.
   *
   * We detect the renderer side implementation to align with the
   * implementation specific behavior.
   */
   switch (dev->physical_device->renderer_driver_id) {
   case VK_DRIVER_ID_ARM_PROPRIETARY:
   case VK_DRIVER_ID_MESA_LLVMPIPE:
   case VK_DRIVER_ID_MESA_TURNIP:
      pool->saturate_on_overflow = true;
      default:
                  VkQueryPool pool_handle = vn_query_pool_to_handle(pool);
   vn_async_vkCreateQueryPool(dev->instance, device, pCreateInfo, NULL,
                        }
      void
   vn_DestroyQueryPool(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_query_pool *pool = vn_query_pool_from_handle(queryPool);
            if (!pool)
                     if (pool->feedback)
                     vn_object_base_fini(&pool->base);
      }
      void
   vn_ResetQueryPool(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
            vn_async_vkResetQueryPool(dev->instance, device, queryPool, firstQuery,
         if (pool->feedback) {
      /* Feedback results are always 64 bit and include availability bit
   * (also 64 bit)
   */
   const uint32_t slot_size = (pool->result_array_size * 8) + 8;
   const uint32_t offset = slot_size * firstQuery;
         }
      static VkResult
   vn_get_query_pool_feedback(struct vn_query_pool *pool,
                                 {
      VkResult result = VK_SUCCESS;
   /* Feedback results are always 64 bit and include availability bit
   * (also 64 bit)
   */
   const uint32_t slot_array_size = pool->result_array_size + 1;
   uint64_t *src = pool->feedback->data;
            uint32_t dst_index = 0;
   uint32_t src_index = 0;
   if (flags & VK_QUERY_RESULT_64_BIT) {
      uint64_t *dst = pData;
   uint32_t index_stride = stride / sizeof(uint64_t);
   for (uint32_t i = 0; i < queryCount; i++) {
      /* Copy the result if its available */
   const uint64_t avail = src[src_index + pool->result_array_size];
   if (avail) {
      memcpy(&dst[dst_index], &src[src_index],
      } else {
      result = VK_NOT_READY;
   /* valid to return result of 0 if partial bit is set */
   if (flags & VK_QUERY_RESULT_PARTIAL_BIT) {
      memset(&dst[dst_index], 0,
         }
   /* Set the availability bit if requested */
                  dst_index += index_stride;
         } else {
      uint32_t *dst = pData;
   uint32_t index_stride = stride / sizeof(uint32_t);
   for (uint32_t i = 0; i < queryCount; i++) {
      /* Copy the result if its available, converting down to uint32_t */
   const uint32_t avail =
         if (avail) {
      for (uint32_t j = 0; j < pool->result_array_size; j++) {
      const uint64_t src_val = src[src_index + j];
   dst[dst_index + j] =
      src_val > UINT32_MAX && pool->saturate_on_overflow
            } else {
      result = VK_NOT_READY;
   /* valid to return result of 0 if partial bit is set */
   if (flags & VK_QUERY_RESULT_PARTIAL_BIT) {
      for (uint32_t j = 0; j < pool->result_array_size; j++)
         }
   /* Set the availability bit if requested */
                  dst_index += index_stride;
         }
      }
      static VkResult
   vn_query_feedback_wait_ready(struct vn_query_pool *pool,
               {
      /* Timeout after 5 seconds */
   uint64_t timeout = 5000ull * 1000 * 1000;
            /* Feedback results are always 64 bit and include availability bit
   * (also 64 bit)
   */
   const uint32_t slot_array_size = pool->result_array_size + 1;
   volatile uint64_t *src = pool->feedback->data;
            uint32_t src_index = 0;
   for (uint32_t i = 0; i < queryCount; i++) {
      while (!src[src_index]) {
                        }
      }
      }
      VkResult
   vn_GetQueryPoolResults(VkDevice device,
                        VkQueryPool queryPool,
   uint32_t firstQuery,
   uint32_t queryCount,
      {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_query_pool *pool = vn_query_pool_from_handle(queryPool);
   const VkAllocationCallbacks *alloc = &pool->allocator;
            const size_t result_width = flags & VK_QUERY_RESULT_64_BIT ? 8 : 4;
   const size_t result_size = pool->result_array_size * result_width;
   const bool result_always_written =
            /* Get results from feedback buffers
   * Not possible for VK_QUERY_RESULT_PARTIAL_BIT
   */
   if (pool->feedback) {
      /* If wait bit is set, wait poll until query is ready */
   if (flags & VK_QUERY_RESULT_WAIT_BIT) {
      result = vn_query_feedback_wait_ready(pool, firstQuery, queryCount);
   if (result != VK_SUCCESS)
      }
   result = vn_get_query_pool_feedback(pool, firstQuery, queryCount, pData,
                     VkQueryResultFlags packed_flags = flags;
   size_t packed_stride = result_size;
   if (!result_always_written)
         if (packed_flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT)
            const size_t packed_size = packed_stride * queryCount;
   void *packed_data;
   if (result_always_written && packed_stride == stride) {
         } else {
      packed_data = vk_alloc(alloc, packed_size, VN_DEFAULT_ALIGN,
         if (!packed_data)
      }
   result = vn_call_vkGetQueryPoolResults(
      dev->instance, device, queryPool, firstQuery, queryCount, packed_size,
         if (packed_data == pData)
            const size_t copy_size =
      result_size +
      const void *src = packed_data;
   void *dst = pData;
   if (result == VK_SUCCESS) {
      for (uint32_t i = 0; i < queryCount; i++) {
      memcpy(dst, src, copy_size);
   src += packed_stride;
         } else if (result == VK_NOT_READY) {
      assert(!result_always_written &&
         if (flags & VK_QUERY_RESULT_64_BIT) {
      for (uint32_t i = 0; i < queryCount; i++) {
      const bool avail = *(const uint64_t *)(src + result_size);
   if (avail)
                        src += packed_stride;
         } else {
      for (uint32_t i = 0; i < queryCount; i++) {
      const bool avail = *(const uint32_t *)(src + result_size);
   if (avail)
                        src += packed_stride;
                     vk_free(alloc, packed_data);
      }
