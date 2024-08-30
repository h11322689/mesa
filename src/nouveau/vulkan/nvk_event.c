   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_event.h"
      #include "nvk_cmd_buffer.h"
   #include "nvk_device.h"
   #include "nvk_entrypoints.h"
   #include "nvk_mme.h"
      #include "nvk_cl906f.h"
   #include "nvk_cl9097.h"
      #define NVK_EVENT_MEM_SIZE sizeof(VkResult)
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateEvent(VkDevice device,
                     {
      VK_FROM_HANDLE(nvk_device, dev, device);
   struct nvk_event *event;
            event = vk_object_zalloc(&dev->vk, pAllocator, sizeof(*event),
         if (!event)
            result = nvk_heap_alloc(dev, &dev->event_heap,
               if (result != VK_SUCCESS) {
      vk_object_free(&dev->vk, pAllocator, event);
                                    }
      VKAPI_ATTR void VKAPI_CALL
   nvk_DestroyEvent(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            if (!event)
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_GetEventStatus(VkDevice device,
         {
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_SetEvent(VkDevice device,
         {
                           }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_ResetEvent(VkDevice device,
         {
                           }
      static bool
   clear_bits64(uint64_t *bitfield, uint64_t bits)
   {
      bool has_bits = (*bitfield & bits) != 0;
   *bitfield &= ~bits;
      }
      uint32_t
   vk_stage_flags_to_nv9097_pipeline_location(VkPipelineStageFlags2 flags)
   {
      if (clear_bits64(&flags, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR |
                           VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT |
   VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT |
   VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT |
   VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT|
            if (clear_bits64(&flags, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT))
            if (clear_bits64(&flags, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT))
            if (clear_bits64(&flags, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT))
            if (clear_bits64(&flags, VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT))
            if (clear_bits64(&flags, VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT |
                  if (clear_bits64(&flags, VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT))
            if (clear_bits64(&flags, VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT))
            if (clear_bits64(&flags, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT))
            if (clear_bits64(&flags, VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT |
                        clear_bits64(&flags, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT |
                        /* TODO: Doing this on 3D will likely cause a WFI which is probably ok but,
   * if we tracked which subchannel we've used most recently, we can probably
   * do better than that.
   */
                        }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdSetEvent2(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
            VkPipelineStageFlags2 stages = 0;
   for (uint32_t i = 0; i < pDependencyInfo->memoryBarrierCount; i++)
         for (uint32_t i = 0; i < pDependencyInfo->bufferMemoryBarrierCount; i++)
         for (uint32_t i = 0; i < pDependencyInfo->imageMemoryBarrierCount; i++)
            struct nv_push *p = nvk_cmd_buffer_push(cmd, 5);
   P_MTHD(p, NV9097, SET_REPORT_SEMAPHORE_A);
   P_NV9097_SET_REPORT_SEMAPHORE_A(p, event->addr >> 32);
   P_NV9097_SET_REPORT_SEMAPHORE_B(p, event->addr);
   P_NV9097_SET_REPORT_SEMAPHORE_C(p, VK_EVENT_SET);
   P_NV9097_SET_REPORT_SEMAPHORE_D(p, {
      .operation = OPERATION_RELEASE,
   .release = RELEASE_AFTER_ALL_PRECEEDING_WRITES_COMPLETE,
   .pipeline_location = vk_stage_flags_to_nv9097_pipeline_location(stages),
         }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdResetEvent2(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
            struct nv_push *p = nvk_cmd_buffer_push(cmd, 5);
   P_MTHD(p, NV9097, SET_REPORT_SEMAPHORE_A);
   P_NV9097_SET_REPORT_SEMAPHORE_A(p, event->addr >> 32);
   P_NV9097_SET_REPORT_SEMAPHORE_B(p, event->addr);
   P_NV9097_SET_REPORT_SEMAPHORE_C(p, VK_EVENT_RESET);
   P_NV9097_SET_REPORT_SEMAPHORE_D(p, {
      .operation = OPERATION_RELEASE,
   .release = RELEASE_AFTER_ALL_PRECEEDING_WRITES_COMPLETE,
   .pipeline_location =
               }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdWaitEvents2(VkCommandBuffer commandBuffer,
                     {
               for (uint32_t i = 0; i < eventCount; i++) {
               struct nv_push *p = nvk_cmd_buffer_push(cmd, 5);
   __push_mthd(p, SUBC_NV9097, NV906F_SEMAPHOREA);
   P_NV906F_SEMAPHOREA(p, event->addr >> 32);
   P_NV906F_SEMAPHOREB(p, (event->addr & UINT32_MAX) >> 2);
   P_NV906F_SEMAPHOREC(p, VK_EVENT_SET);
   P_NV906F_SEMAPHORED(p, {
      .operation = OPERATION_ACQUIRE,
   .acquire_switch = ACQUIRE_SWITCH_ENABLED,
            }
