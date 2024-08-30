   /*
   * Copyright © 2022 Friedrich Vock
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
      #include "meta/radv_meta.h"
   #include "util/u_process.h"
   #include "radv_private.h"
   #include "vk_acceleration_structure.h"
   #include "vk_common_entrypoints.h"
   #include "wsi_common_entrypoints.h"
      VKAPI_ATTR VkResult VKAPI_CALL
   rra_QueuePresentKHR(VkQueue _queue, const VkPresentInfoKHR *pPresentInfo)
   {
      RADV_FROM_HANDLE(radv_queue, queue, _queue);
   VkResult result = queue->device->layer_dispatch.rra.QueuePresentKHR(_queue, pPresentInfo);
   if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            if (!queue->device->rra_trace.copy_after_build)
                     hash_table_foreach (accel_structs, entry) {
      struct radv_rra_accel_struct_data *data = entry->data;
   if (!data->is_dead)
            radv_destroy_rra_accel_struct_data(radv_device_to_handle(queue->device), data);
                  }
      static VkResult
   rra_init_accel_struct_data_buffer(VkDevice vk_device, struct radv_rra_accel_struct_data *data)
   {
      RADV_FROM_HANDLE(radv_device, device, vk_device);
   VkBufferCreateInfo buffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
               VkResult result = radv_create_buffer(device, &buffer_create_info, NULL, &data->buffer, true);
   if (result != VK_SUCCESS)
            VkMemoryRequirements requirements;
            VkMemoryAllocateFlagsInfo flags_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
               VkMemoryAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   .pNext = &flags_info,
   .allocationSize = requirements.size,
      };
   result = radv_alloc_memory(device, &alloc_info, NULL, &data->memory, true);
   if (result != VK_SUCCESS)
            result = vk_common_BindBufferMemory(vk_device, data->buffer, data->memory, 0);
   if (result != VK_SUCCESS)
               fail_memory:
         fail_buffer:
      radv_DestroyBuffer(vk_device, data->buffer, NULL);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   rra_CreateAccelerationStructureKHR(VkDevice _device, const VkAccelerationStructureCreateInfoKHR *pCreateInfo,
               {
      RADV_FROM_HANDLE(radv_device, device, _device);
            VkResult result = device->layer_dispatch.rra.CreateAccelerationStructureKHR(_device, pCreateInfo, pAllocator,
            if (result != VK_SUCCESS)
            RADV_FROM_HANDLE(vk_acceleration_structure, structure, *pAccelerationStructure);
            struct radv_rra_accel_struct_data *data = calloc(1, sizeof(struct radv_rra_accel_struct_data));
   if (!data) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               data->va = buffer->bo ? vk_acceleration_structure_get_va(structure) : 0;
   data->size = structure->size;
   data->type = pCreateInfo->type;
            VkEventCreateInfo eventCreateInfo = {
                  result = radv_create_event(device, &eventCreateInfo, NULL, &data->build_event, true);
   if (result != VK_SUCCESS)
            if (device->rra_trace.copy_after_build) {
      result = rra_init_accel_struct_data_buffer(_device, data);
   if (result != VK_SUCCESS)
                        if (data->va)
               fail_event:
         fail_data:
         fail_as:
      device->layer_dispatch.rra.DestroyAccelerationStructureKHR(_device, *pAccelerationStructure, pAllocator);
      exit:
      simple_mtx_unlock(&device->rra_trace.data_mtx);
      }
      static void
   handle_accel_struct_write(VkCommandBuffer commandBuffer, struct vk_acceleration_structure *accel_struct,
         {
               VkMemoryBarrier2 barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
   .srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
   .srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
   .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
               VkDependencyInfo dependencyInfo = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   .memoryBarrierCount = 1,
                                 if (!data->va) {
      data->va = vk_acceleration_structure_get_va(accel_struct);
               if (!data->buffer)
            VkBufferCopy2 region = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
   .srcOffset = accel_struct->offset,
               VkCopyBufferInfo2 copyInfo = {
      .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
   .srcBuffer = accel_struct->buffer,
   .dstBuffer = data->buffer,
   .regionCount = 1,
                  }
      VKAPI_ATTR void VKAPI_CALL
   rra_CmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount,
               {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   cmd_buffer->device->layer_dispatch.rra.CmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos,
            simple_mtx_lock(&cmd_buffer->device->rra_trace.data_mtx);
   for (uint32_t i = 0; i < infoCount; ++i) {
      RADV_FROM_HANDLE(vk_acceleration_structure, structure, pInfos[i].dstAccelerationStructure);
            assert(entry);
               }
      }
      VKAPI_ATTR void VKAPI_CALL
   rra_CmdCopyAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureInfoKHR *pInfo)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
                     RADV_FROM_HANDLE(vk_acceleration_structure, structure, pInfo->dst);
            assert(entry);
                        }
      VKAPI_ATTR void VKAPI_CALL
   rra_CmdCopyMemoryToAccelerationStructureKHR(VkCommandBuffer commandBuffer,
         {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
                     RADV_FROM_HANDLE(vk_acceleration_structure, structure, pInfo->dst);
            assert(entry);
                        }
      VKAPI_ATTR void VKAPI_CALL
   rra_DestroyAccelerationStructureKHR(VkDevice _device, VkAccelerationStructureKHR _structure,
         {
      if (!_structure)
            RADV_FROM_HANDLE(radv_device, device, _device);
                              assert(entry);
            if (device->rra_trace.copy_after_build)
         else
                        }
