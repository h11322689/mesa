   /*
   * Copyright © 2021 Intel Corporation
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
      #include "vk_alloc.h"
   #include "vk_command_buffer.h"
   #include "vk_common_entrypoints.h"
   #include "vk_device.h"
   #include "vk_queue.h"
   #include "vk_util.h"
   #include "../wsi/wsi_common.h"
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdWriteTimestamp(
      VkCommandBuffer                             commandBuffer,
   VkPipelineStageFlagBits                     pipelineStage,
   VkQueryPool                                 queryPool,
      {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
            device->dispatch_table.CmdWriteTimestamp2KHR(commandBuffer,
                  }
      static VkMemoryBarrier2
   upgrade_memory_barrier(const VkMemoryBarrier *barrier,
               {
      return (VkMemoryBarrier2) {
      .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
   .pNext         = barrier->pNext,
   .srcStageMask  = src_stage_mask2,
   .srcAccessMask = (VkAccessFlags2) barrier->srcAccessMask,
   .dstStageMask  = dst_stage_mask2,
         }
      static VkBufferMemoryBarrier2
   upgrade_buffer_memory_barrier(const VkBufferMemoryBarrier *barrier,
               {
      return (VkBufferMemoryBarrier2) {
      .sType                = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
   .pNext                = barrier->pNext,
   .srcStageMask         = src_stage_mask2,
   .srcAccessMask        = (VkAccessFlags2) barrier->srcAccessMask,
   .dstStageMask         = dst_stage_mask2,
   .dstAccessMask        = (VkAccessFlags2) barrier->dstAccessMask,
   .srcQueueFamilyIndex  = barrier->srcQueueFamilyIndex,
   .dstQueueFamilyIndex  = barrier->dstQueueFamilyIndex,
   .buffer               = barrier->buffer,
   .offset               = barrier->offset,
         }
      static VkImageMemoryBarrier2
   upgrade_image_memory_barrier(const VkImageMemoryBarrier *barrier,
               {
      return (VkImageMemoryBarrier2) {
      .sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
   .pNext                = barrier->pNext,
   .srcStageMask         = src_stage_mask2,
   .srcAccessMask        = (VkAccessFlags2) barrier->srcAccessMask,
   .dstStageMask         = dst_stage_mask2,
   .dstAccessMask        = (VkAccessFlags2) barrier->dstAccessMask,
   .oldLayout            = barrier->oldLayout,
   .newLayout            = barrier->newLayout,
   .srcQueueFamilyIndex  = barrier->srcQueueFamilyIndex,
   .dstQueueFamilyIndex  = barrier->dstQueueFamilyIndex,
   .image                = barrier->image,
         }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdPipelineBarrier(
      VkCommandBuffer                             commandBuffer,
   VkPipelineStageFlags                        srcStageMask,
   VkPipelineStageFlags                        dstStageMask,
   VkDependencyFlags                           dependencyFlags,
   uint32_t                                    memoryBarrierCount,
   const VkMemoryBarrier*                      pMemoryBarriers,
   uint32_t                                    bufferMemoryBarrierCount,
   const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
   uint32_t                                    imageMemoryBarrierCount,
      {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
            STACK_ARRAY(VkMemoryBarrier2, memory_barriers, memoryBarrierCount);
   STACK_ARRAY(VkBufferMemoryBarrier2, buffer_barriers, bufferMemoryBarrierCount);
            VkPipelineStageFlags2 src_stage_mask2 = (VkPipelineStageFlags2) srcStageMask;
            for (uint32_t i = 0; i < memoryBarrierCount; i++) {
      memory_barriers[i] = upgrade_memory_barrier(&pMemoryBarriers[i],
            }
   for (uint32_t i = 0; i < bufferMemoryBarrierCount; i++) {
      buffer_barriers[i] = upgrade_buffer_memory_barrier(&pBufferMemoryBarriers[i],
            }
   for (uint32_t i = 0; i < imageMemoryBarrierCount; i++) {
      image_barriers[i] = upgrade_image_memory_barrier(&pImageMemoryBarriers[i],
                     VkDependencyInfo dep_info = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   .memoryBarrierCount = memoryBarrierCount,
   .pMemoryBarriers = memory_barriers,
   .bufferMemoryBarrierCount = bufferMemoryBarrierCount,
   .pBufferMemoryBarriers = buffer_barriers,
   .imageMemoryBarrierCount = imageMemoryBarrierCount,
                        STACK_ARRAY_FINISH(memory_barriers);
   STACK_ARRAY_FINISH(buffer_barriers);
      }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdSetEvent(
      VkCommandBuffer                             commandBuffer,
   VkEvent                                     event,
      {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
            VkMemoryBarrier2 mem_barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
   .srcStageMask = (VkPipelineStageFlags2) stageMask,
      };
   VkDependencyInfo dep_info = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   .memoryBarrierCount = 1,
                  }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdResetEvent(
      VkCommandBuffer                             commandBuffer,
   VkEvent                                     event,
      {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
            device->dispatch_table.CmdResetEvent2KHR(commandBuffer,
            }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdWaitEvents(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    eventCount,
   const VkEvent*                              pEvents,
   VkPipelineStageFlags                        srcStageMask,
   VkPipelineStageFlags                        destStageMask,
   uint32_t                                    memoryBarrierCount,
   const VkMemoryBarrier*                      pMemoryBarriers,
   uint32_t                                    bufferMemoryBarrierCount,
   const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
   uint32_t                                    imageMemoryBarrierCount,
      {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
                     /* Note that dstStageMask and srcStageMask in the CmdWaitEvent2() call
   * are the same.  This is to match the CmdSetEvent2() call from
   * vk_common_CmdSetEvent().  The actual src->dst stage barrier will
   * happen as part of the CmdPipelineBarrier() call below.
   */
   VkMemoryBarrier2 stage_barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
   .srcStageMask = srcStageMask,
               for (uint32_t i = 0; i < eventCount; i++) {
      deps[i] = (VkDependencyInfo) {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   .memoryBarrierCount = 1,
         }
                     /* Setting dependency to 0 because :
   *
   *    - For BY_REGION_BIT and VIEW_LOCAL_BIT, events are not allowed inside a
   *      render pass so these don't apply.
   *
   *    - For DEVICE_GROUP_BIT, we have the following bit of spec text:
   *
   *        "Semaphore and event dependencies are device-local and only
   *         execute on the one physical device that performs the
   *         dependency."
   */
            device->dispatch_table.CmdPipelineBarrier(commandBuffer,
                              }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdWriteBufferMarkerAMD(
      VkCommandBuffer                             commandBuffer,
   VkPipelineStageFlagBits                     pipelineStage,
   VkBuffer                                    dstBuffer,
   VkDeviceSize                                dstOffset,
      {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
            device->dispatch_table.CmdWriteBufferMarker2AMD(commandBuffer,
                        }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetQueueCheckpointDataNV(
      VkQueue                                     queue,
   uint32_t*                                   pCheckpointDataCount,
      {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_QueueSubmit(
      VkQueue                                     _queue,
   uint32_t                                    submitCount,
   const VkSubmitInfo*                         pSubmits,
      {
      VK_FROM_HANDLE(vk_queue, queue, _queue);
            STACK_ARRAY(VkSubmitInfo2, submit_info_2, submitCount);
   STACK_ARRAY(VkPerformanceQuerySubmitInfoKHR, perf_query_submit_info, submitCount);
            uint32_t n_wait_semaphores = 0;
   uint32_t n_command_buffers = 0;
   uint32_t n_signal_semaphores = 0;
   for (uint32_t s = 0; s < submitCount; s++) {
      n_wait_semaphores += pSubmits[s].waitSemaphoreCount;
   n_command_buffers += pSubmits[s].commandBufferCount;
               STACK_ARRAY(VkSemaphoreSubmitInfo, wait_semaphores, n_wait_semaphores);
   STACK_ARRAY(VkCommandBufferSubmitInfo, command_buffers, n_command_buffers);
            n_wait_semaphores = 0;
   n_command_buffers = 0;
            for (uint32_t s = 0; s < submitCount; s++) {
      const VkTimelineSemaphoreSubmitInfo *timeline_info =
      vk_find_struct_const(pSubmits[s].pNext,
      const uint64_t *wait_values = NULL;
            if (timeline_info && timeline_info->waitSemaphoreValueCount) {
      /* From the Vulkan 1.3.204 spec:
   *
   *    VUID-VkSubmitInfo-pNext-03240
   *
   *    "If the pNext chain of this structure includes a VkTimelineSemaphoreSubmitInfo structure
   *    and any element of pSignalSemaphores was created with a VkSemaphoreType of
   *    VK_SEMAPHORE_TYPE_TIMELINE, then its signalSemaphoreValueCount member must equal
   *    signalSemaphoreCount"
   */
   assert(timeline_info->waitSemaphoreValueCount == pSubmits[s].waitSemaphoreCount);
               if (timeline_info && timeline_info->signalSemaphoreValueCount) {
      /* From the Vulkan 1.3.204 spec:
   *
   *    VUID-VkSubmitInfo-pNext-03241
   *
   *    "If the pNext chain of this structure includes a VkTimelineSemaphoreSubmitInfo structure
   *    and any element of pWaitSemaphores was created with a VkSemaphoreType of
   *    VK_SEMAPHORE_TYPE_TIMELINE, then its waitSemaphoreValueCount member must equal
   *    waitSemaphoreCount"
   */
   assert(timeline_info->signalSemaphoreValueCount == pSubmits[s].signalSemaphoreCount);
               const VkDeviceGroupSubmitInfo *group_info =
            for (uint32_t i = 0; i < pSubmits[s].waitSemaphoreCount; i++) {
      wait_semaphores[n_wait_semaphores + i] = (VkSemaphoreSubmitInfo) {
      .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
   .semaphore   = pSubmits[s].pWaitSemaphores[i],
   .value       = wait_values ? wait_values[i] : 0,
   .stageMask   = pSubmits[s].pWaitDstStageMask[i],
         }
   for (uint32_t i = 0; i < pSubmits[s].commandBufferCount; i++) {
      command_buffers[n_command_buffers + i] = (VkCommandBufferSubmitInfo) {
      .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
   .commandBuffer = pSubmits[s].pCommandBuffers[i],
         }
   for (uint32_t i = 0; i < pSubmits[s].signalSemaphoreCount; i++) {
      signal_semaphores[n_signal_semaphores + i] = (VkSemaphoreSubmitInfo) {
      .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
   .semaphore = pSubmits[s].pSignalSemaphores[i],
   .value     = signal_values ? signal_values[i] : 0,
   .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                  const VkProtectedSubmitInfo *protected_info =
            submit_info_2[s] = (VkSubmitInfo2) {
      .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
   .flags                    = ((protected_info && protected_info->protectedSubmit) ?
         .waitSemaphoreInfoCount   = pSubmits[s].waitSemaphoreCount,
   .pWaitSemaphoreInfos      = &wait_semaphores[n_wait_semaphores],
   .commandBufferInfoCount   = pSubmits[s].commandBufferCount,
   .pCommandBufferInfos      = &command_buffers[n_command_buffers],
   .signalSemaphoreInfoCount = pSubmits[s].signalSemaphoreCount,
               const VkPerformanceQuerySubmitInfoKHR *query_info =
      vk_find_struct_const(pSubmits[s].pNext,
      if (query_info) {
      perf_query_submit_info[s] = *query_info;
   perf_query_submit_info[s].pNext = NULL;
               const struct wsi_memory_signal_submit_info *mem_signal_info =
      vk_find_struct_const(pSubmits[s].pNext,
      if (mem_signal_info) {
      wsi_mem_submit_info[s] = *mem_signal_info;
   wsi_mem_submit_info[s].pNext = NULL;
               n_wait_semaphores += pSubmits[s].waitSemaphoreCount;
   n_command_buffers += pSubmits[s].commandBufferCount;
               VkResult result = device->dispatch_table.QueueSubmit2KHR(_queue,
                        STACK_ARRAY_FINISH(wait_semaphores);
   STACK_ARRAY_FINISH(command_buffers);
   STACK_ARRAY_FINISH(signal_semaphores);
   STACK_ARRAY_FINISH(submit_info_2);
   STACK_ARRAY_FINISH(perf_query_submit_info);
               }
