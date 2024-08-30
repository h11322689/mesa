   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * 'pvr_write_query_to_buffer()' and 'pvr_wait_for_available()' based on anv:
   * Copyright © 2015 Intel Corporation
   */
      #include <assert.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <string.h>
   #include <vulkan/vulkan.h>
      #include "pvr_bo.h"
   #include "pvr_csb.h"
   #include "pvr_device_info.h"
   #include "pvr_private.h"
   #include "util/macros.h"
   #include "util/os_time.h"
   #include "vk_log.h"
   #include "vk_object.h"
      VkResult pvr_CreateQueryPool(VkDevice _device,
                     {
      PVR_FROM_HANDLE(pvr_device, device, _device);
   const uint32_t core_count = device->pdevice->dev_runtime_info.core_count;
   const uint32_t query_size = pCreateInfo->queryCount * sizeof(uint32_t);
   struct pvr_query_pool *pool;
   uint64_t alloc_size;
            /* Vulkan 1.0 supports only occlusion, timestamp, and pipeline statistics
   * query.
   * We don't currently support timestamp queries.
   * VkQueueFamilyProperties->timestampValidBits = 0.
   * We don't currently support pipeline statistics queries.
   * VkPhysicalDeviceFeatures->pipelineStatisticsQuery = false.
   */
   assert(!device->vk.enabled_features.pipelineStatisticsQuery);
            pool = vk_object_alloc(&device->vk,
                     if (!pool)
            pool->result_stride =
                     /* Each Phantom writes to a separate offset within the vis test heap so
   * allocate space for the total number of Phantoms.
   */
            result = pvr_bo_suballoc(&device->suballoc_vis_test,
                           if (result != VK_SUCCESS)
            result = pvr_bo_suballoc(&device->suballoc_general,
                           if (result != VK_SUCCESS)
                           err_free_result_buffer:
            err_free_pool:
                  }
      void pvr_DestroyQueryPool(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_query_pool, pool, queryPool);
            if (!pool)
            pvr_bo_suballoc_free(pool->availability_buffer);
               }
      /* Note: make sure to make the availability buffer's memory defined in
   * accordance to how the device is expected to fill it. We don't make it defined
   * here since that would cover up usage of this function while the underlying
   * buffer region being accessed wasn't expect to have been written by the
   * device.
   */
   static inline bool pvr_query_is_available(const struct pvr_query_pool *pool,
         {
      volatile uint32_t *available =
            }
      #define NSEC_PER_SEC UINT64_C(1000000000)
   #define PVR_WAIT_TIMEOUT UINT64_C(5)
      /* Note: make sure to make the availability buffer's memory defined in
   * accordance to how the device is expected to fill it. We don't make it defined
   * here since that would cover up usage of this function while the underlying
   * buffer region being accessed wasn't expect to have been written by the
   * device.
   */
   /* TODO: Handle device loss scenario properly. */
   static VkResult pvr_wait_for_available(struct pvr_device *device,
               {
      const uint64_t abs_timeout =
            /* From the Vulkan 1.0 spec:
   *
   *    Commands that wait indefinitely for device execution (namely
   *    vkDeviceWaitIdle, vkQueueWaitIdle, vkWaitForFences or
   *    vkAcquireNextImageKHR with a maximum timeout, and
   *    vkGetQueryPoolResults with the VK_QUERY_RESULT_WAIT_BIT bit set in
   *    flags) must return in finite time even in the case of a lost device,
   *    and return either VK_SUCCESS or VK_ERROR_DEVICE_LOST.
   */
   while (os_time_get_nano() < abs_timeout) {
      if (pvr_query_is_available(pool, query_idx) != 0)
                  }
      #undef NSEC_PER_SEC
   #undef PVR_WAIT_TIMEOUT
      static inline void pvr_write_query_to_buffer(uint8_t *buffer,
                     {
      if (flags & VK_QUERY_RESULT_64_BIT) {
      uint64_t *query_data = (uint64_t *)buffer;
      } else {
      uint32_t *query_data = (uint32_t *)buffer;
         }
      VkResult pvr_GetQueryPoolResults(VkDevice _device,
                                    VkQueryPool queryPool,
      {
      PVR_FROM_HANDLE(pvr_query_pool, pool, queryPool);
   PVR_FROM_HANDLE(pvr_device, device, _device);
   VG(volatile uint32_t *available =
         volatile uint32_t *query_results =
         const uint32_t core_count = device->pdevice->dev_runtime_info.core_count;
   uint8_t *data = (uint8_t *)pData;
            /* TODO: Instead of making the memory defined here for valgrind, to better
   * catch out of bounds access and other memory errors we should move them
   * where where the query buffers are changed by the driver or device (e.g.
   * "vkCmdResetQueryPool()", "vkGetQueryPoolResults()", etc.).
            VG(VALGRIND_MAKE_MEM_DEFINED(&available[firstQuery],
            for (uint32_t i = 0; i < core_count; i++) {
      VG(VALGRIND_MAKE_MEM_DEFINED(
      &query_results[firstQuery + i * pool->result_stride],
            for (uint32_t i = 0; i < queryCount; i++) {
      bool is_available = pvr_query_is_available(pool, firstQuery + i);
            if (flags & VK_QUERY_RESULT_WAIT_BIT && !is_available) {
      result = pvr_wait_for_available(device, pool, firstQuery + i);
                              for (uint32_t j = 0; j < core_count; j++)
            if (is_available || (flags & VK_QUERY_RESULT_PARTIAL_BIT))
         else
            if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT)
                        VG(VALGRIND_MAKE_MEM_UNDEFINED(&available[firstQuery],
            for (uint32_t i = 0; i < core_count; i++) {
      VG(VALGRIND_MAKE_MEM_UNDEFINED(
      &query_results[firstQuery + i * pool->result_stride],
               }
      void pvr_CmdResetQueryPool(VkCommandBuffer commandBuffer,
                     {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
                              query_info.reset_query_pool.query_pool = queryPool;
   query_info.reset_query_pool.first_query = firstQuery;
               }
      void pvr_ResetQueryPool(VkDevice _device,
                     {
      PVR_FROM_HANDLE(pvr_query_pool, pool, queryPool);
   uint32_t *availability =
               }
      void pvr_CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer,
                                    VkQueryPool queryPool,
      {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_query_info query_info;
                              query_info.copy_query_results.query_pool = queryPool;
   query_info.copy_query_results.first_query = firstQuery;
   query_info.copy_query_results.query_count = queryCount;
   query_info.copy_query_results.dst_buffer = dstBuffer;
   query_info.copy_query_results.dst_offset = dstOffset;
   query_info.copy_query_results.stride = stride;
            result = pvr_cmd_buffer_start_sub_cmd(cmd_buffer, PVR_SUB_CMD_TYPE_EVENT);
   if (result != VK_SUCCESS)
            /* The Vulkan 1.3.231 spec says:
   *
   *    "vkCmdCopyQueryPoolResults is considered to be a transfer operation,
   *    and its writes to buffer memory must be synchronized using
   *    VK_PIPELINE_STAGE_TRANSFER_BIT and VK_ACCESS_TRANSFER_WRITE_BIT before
   *    using the results."
   *
   */
   /* We record barrier event sub commands to sync the compute job used for the
   * copy query results program with transfer jobs to prevent an overlapping
   * transfer job with the compute job.
            cmd_buffer->state.current_sub_cmd->event = (struct pvr_sub_cmd_event){
      .type = PVR_EVENT_TYPE_BARRIER,
   .barrier = {
      .wait_for_stage_mask = PVR_PIPELINE_STAGE_TRANSFER_BIT,
                  result = pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
   if (result != VK_SUCCESS)
                     result = pvr_cmd_buffer_start_sub_cmd(cmd_buffer, PVR_SUB_CMD_TYPE_EVENT);
   if (result != VK_SUCCESS)
            cmd_buffer->state.current_sub_cmd->event = (struct pvr_sub_cmd_event){
      .type = PVR_EVENT_TYPE_BARRIER,
   .barrier = {
      .wait_for_stage_mask = PVR_PIPELINE_STAGE_OCCLUSION_QUERY_BIT,
            }
      void pvr_CmdBeginQuery(VkCommandBuffer commandBuffer,
                     {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
                     /* Occlusion queries can't be nested. */
            if (state->current_sub_cmd) {
               if (!state->current_sub_cmd->gfx.query_pool) {
         } else if (state->current_sub_cmd->gfx.query_pool != pool) {
                              result = pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
                  result =
                        /* Use existing render setup, but load color attachments from HW
   * BGOBJ.
   */
   state->current_sub_cmd->gfx.barrier_load = true;
   state->current_sub_cmd->gfx.barrier_store = false;
                  state->query_pool = pool;
   state->vis_test_enabled = true;
   state->vis_reg = query;
            /* Add the index to the list for this render. */
      }
      void pvr_CmdEndQuery(VkCommandBuffer commandBuffer,
               {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
                     state->vis_test_enabled = false;
      }
