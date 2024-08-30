   /*
   * Copyright © 2022 Konstantin Seurer
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
      #include "radv_radix_sort.h"
   #include "targets/u64/config.h"
   #include "radv_private.h"
   #include "target.h"
      static const uint32_t init_spv[] = {
   #include "radix_sort/shaders/init.comp.spv.h"
   };
      static const uint32_t fill_spv[] = {
   #include "radix_sort/shaders/fill.comp.spv.h"
   };
      static const uint32_t histogram_spv[] = {
   #include "radix_sort/shaders/histogram.comp.spv.h"
   };
      static const uint32_t prefix_spv[] = {
   #include "radix_sort/shaders/prefix.comp.spv.h"
   };
      static const uint32_t scatter_0_even_spv[] = {
   #include "radix_sort/shaders/scatter_0_even.comp.spv.h"
   };
      static const uint32_t scatter_0_odd_spv[] = {
   #include "radix_sort/shaders/scatter_0_odd.comp.spv.h"
   };
      static const uint32_t scatter_1_even_spv[] = {
   #include "radix_sort/shaders/scatter_1_even.comp.spv.h"
   };
      static const uint32_t scatter_1_odd_spv[] = {
   #include "radix_sort/shaders/scatter_1_odd.comp.spv.h"
   };
      static const struct radix_sort_vk_target_config target_config = {
               .histogram =
      {
      .workgroup_size_log2 = RS_HISTOGRAM_WORKGROUP_SIZE_LOG2,
   .subgroup_size_log2 = RS_HISTOGRAM_SUBGROUP_SIZE_LOG2,
            .prefix =
      {
      .workgroup_size_log2 = RS_PREFIX_WORKGROUP_SIZE_LOG2,
            .scatter =
      {
      .workgroup_size_log2 = RS_SCATTER_WORKGROUP_SIZE_LOG2,
   .subgroup_size_log2 = RS_SCATTER_SUBGROUP_SIZE_LOG2,
      };
      radix_sort_vk_t *
   radv_create_radix_sort_u64(VkDevice device, VkAllocationCallbacks const *ac, VkPipelineCache pc)
   {
      const uint32_t *spv[8] = {
      init_spv,           fill_spv,          histogram_spv,      prefix_spv,
      };
   const uint32_t spv_sizes[8] = {
      sizeof(init_spv),           sizeof(fill_spv),           sizeof(histogram_spv),
   sizeof(prefix_spv),         sizeof(scatter_0_even_spv), sizeof(scatter_0_odd_spv),
      };
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   vkCreateShaderModule(VkDevice _device, const VkShaderModuleCreateInfo *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
   return device->vk.dispatch_table.CreateShaderModule(_device, pCreateInfo, pAllocator,
      }
      VKAPI_ATTR void VKAPI_CALL
   vkDestroyShaderModule(VkDevice _device, VkShaderModule shaderModule,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   vkCreatePipelineLayout(VkDevice _device, const VkPipelineLayoutCreateInfo *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
   return device->vk.dispatch_table.CreatePipelineLayout(_device, pCreateInfo, pAllocator,
      }
      VKAPI_ATTR void VKAPI_CALL
   vkDestroyPipelineLayout(VkDevice _device, VkPipelineLayout pipelineLayout,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   vkCreateComputePipelines(VkDevice _device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
               {
      RADV_FROM_HANDLE(radv_device, device, _device);
   return device->vk.dispatch_table.CreateComputePipelines(_device, pipelineCache, createInfoCount,
      }
      VKAPI_ATTR void VKAPI_CALL
   vkDestroyPipeline(VkDevice _device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
      }
      VKAPI_ATTR void VKAPI_CALL
   vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                        VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
   uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
      {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   cmd_buffer->device->vk.dispatch_table.CmdPipelineBarrier(
      commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount,
   pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount,
   }
      VKAPI_ATTR void VKAPI_CALL
   vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout,
               {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   cmd_buffer->device->vk.dispatch_table.CmdPushConstants(commandBuffer, layout, stageFlags, offset,
      }
      VKAPI_ATTR void VKAPI_CALL
   vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
         {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   cmd_buffer->device->vk.dispatch_table.CmdBindPipeline(commandBuffer, pipelineBindPoint,
      }
      VKAPI_ATTR void VKAPI_CALL
   vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY,
         {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   cmd_buffer->device->vk.dispatch_table.CmdDispatch(commandBuffer, groupCountX, groupCountY,
      }
      VKAPI_ATTR VkDeviceAddress VKAPI_CALL
   vkGetBufferDeviceAddress(VkDevice _device, const VkBufferDeviceAddressInfo *pInfo)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
      }
      VKAPI_ATTR void VKAPI_CALL
   vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
         {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   cmd_buffer->device->vk.dispatch_table.CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size,
      }
      VKAPI_ATTR void VKAPI_CALL
   vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
      }
