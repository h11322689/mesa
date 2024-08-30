   /*
   * Copyright © 2022 Raspberry Pi Ltd
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
   #include "compiler/nir/nir_builder.h"
      #include "vk_common_entrypoints.h"
      static nir_shader *
   get_set_event_cs()
   {
      const nir_shader_compiler_options *options = v3dv_pipeline_get_nir_options();
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, options,
            nir_def *buf =
      nir_vulkan_resource_index(&b, 2, 32, nir_imm_int(&b, 0),
                     nir_def *offset =
            nir_def *value =
            nir_store_ssbo(&b, value, buf, offset,
               }
      static nir_shader *
   get_wait_event_cs()
   {
      const nir_shader_compiler_options *options = v3dv_pipeline_get_nir_options();
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, options,
            nir_def *buf =
      nir_vulkan_resource_index(&b, 2, 32, nir_imm_int(&b, 0),
                     nir_def *offset =
            nir_loop *loop = nir_push_loop(&b);
      nir_def *load =
                  nir_if *if_stmt = nir_push_if(&b, nir_ieq_imm(&b, value, 1));
   nir_jump(&b, nir_jump_break);
                  }
      static bool
   create_event_pipelines(struct v3dv_device *device)
   {
               if (!device->events.descriptor_set_layout) {
      /* Pipeline layout:
   *  - 1 storage buffer for the BO with the events state.
   *  - 2 push constants:
   *    0B: offset of the event in the buffer (4 bytes).
   *    4B: value for the event (1 byte), only used with the set_event_pipeline.
   */
   VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
      .binding = 0,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
   .descriptorCount = 1,
               VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
   .bindingCount = 1,
               result =
      v3dv_CreateDescriptorSetLayout(v3dv_device_to_handle(device),
                     if (result != VK_SUCCESS)
               if (!device->events.pipeline_layout) {
      VkPipelineLayoutCreateInfo pipeline_layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &device->events.descriptor_set_layout,
   .pushConstantRangeCount = 1,
   .pPushConstantRanges =
               result =
      v3dv_CreatePipelineLayout(v3dv_device_to_handle(device),
                     if (result != VK_SUCCESS)
                        if (!device->events.set_event_pipeline) {
      nir_shader *set_event_cs_nir = get_set_event_cs();
   result = v3dv_create_compute_pipeline_from_nir(device,
                     ralloc_free(set_event_cs_nir);
   if (result != VK_SUCCESS)
                        if (!device->events.wait_event_pipeline) {
      nir_shader *wait_event_cs_nir = get_wait_event_cs();
   result = v3dv_create_compute_pipeline_from_nir(device,
                     ralloc_free(wait_event_cs_nir);
   if (result != VK_SUCCESS)
                           }
      static void
   destroy_event_pipelines(struct v3dv_device *device)
   {
               v3dv_DestroyPipeline(_device, device->events.set_event_pipeline,
                  v3dv_DestroyPipeline(_device, device->events.wait_event_pipeline,
                  v3dv_DestroyPipelineLayout(_device, device->events.pipeline_layout,
                  v3dv_DestroyDescriptorSetLayout(_device,
                  }
      static void
   init_event(struct v3dv_device *device, struct v3dv_event *event, uint32_t index)
   {
      vk_object_base_init(&device->vk, &event->base, VK_OBJECT_TYPE_EVENT);
   event->index = index;
      }
      VkResult
   v3dv_event_allocate_resources(struct v3dv_device *device)
   {
      VkResult result = VK_SUCCESS;
            /* BO with event states. Make sure we always align to a page size (4096)
   * to ensure we use all the memory the kernel will allocate for the BO.
   *
   * CTS has tests that require over 8192 active events (yes, really) so
   * let's make sure we allow for that.
   */
   const uint32_t bo_size = 3 * 4096;
   struct v3dv_bo *bo = v3dv_bo_alloc(device, bo_size, "events", true);
   if (!bo) {
      result = vk_error(device, VK_ERROR_OUT_OF_DEVICE_MEMORY);
                        if (!v3dv_bo_map(device, bo, bo_size)) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               /* Pre-allocate our events, each event requires 1 byte of BO storage */
   device->events.event_count = bo_size;
   device->events.events =
      vk_zalloc2(&device->vk.alloc, NULL,
            if (!device->events.events) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               list_inithead(&device->events.free_list);
   for (int i = 0; i < device->events.event_count; i++)
            /* Vulkan buffer for the event state BO */
   VkBufferCreateInfo buf_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
   .size = bo->size,
      };
   result = v3dv_CreateBuffer(_device, &buf_info, NULL,
         if (result != VK_SUCCESS)
            struct v3dv_device_memory *mem =
      vk_object_zalloc(&device->vk, NULL, sizeof(*mem),
      if (!mem) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               mem->bo = bo;
            device->events.mem = v3dv_device_memory_to_handle(mem);
   VkBindBufferMemoryInfo bind_info = {
      .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
   .buffer = device->events.buffer,
   .memory = device->events.mem,
      };
            /* Pipelines */
   if (!create_event_pipelines(device)) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               /* Descriptor pool & set to access the buffer */
   VkDescriptorPoolSize pool_size = {
      .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      };
   VkDescriptorPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
   .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
   .maxSets = 1,
   .poolSizeCount = 1,
      };
   result =
      v3dv_CreateDescriptorPool(_device, &pool_info, NULL,
         if (result != VK_SUCCESS)
            VkDescriptorSetAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
   .descriptorPool = device->events.descriptor_pool,
   .descriptorSetCount = 1,
      };
   result = v3dv_AllocateDescriptorSets(_device, &alloc_info,
         if (result != VK_SUCCESS)
            VkDescriptorBufferInfo desc_buf_info = {
      .buffer = device->events.buffer,
   .offset = 0,
               VkWriteDescriptorSet write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstSet = device->events.descriptor_set,
   .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      };
                  fail:
      v3dv_event_free_resources(device);
      }
      void
   v3dv_event_free_resources(struct v3dv_device *device)
   {
      if (device->events.bo) {
      v3dv_bo_free(device, device->events.bo);
               if (device->events.events) {
      vk_free2(&device->vk.alloc, NULL, device->events.events);
               if (device->events.mem) {
      vk_object_free(&device->vk, NULL,
                     v3dv_DestroyBuffer(v3dv_device_to_handle(device),
                  v3dv_FreeDescriptorSets(v3dv_device_to_handle(device),
                        v3dv_DestroyDescriptorPool(v3dv_device_to_handle(device),
                           }
      static struct v3dv_event *
   allocate_event(struct v3dv_device *device)
   {
      mtx_lock(&device->events.lock);
   if (list_is_empty(&device->events.free_list)) {
      mtx_unlock(&device->events.lock);
               struct v3dv_event *event =
         list_del(&event->link);
               }
      static void
   free_event(struct v3dv_device *device, uint32_t index)
   {
      assert(index < device->events.event_count);
   mtx_lock(&device->events.lock);
   list_addtail(&device->events.events[index].link, &device->events.free_list);
      }
      static void
   event_set_value(struct v3dv_device *device,
               {
      assert(value == 0 || value == 1);
   uint8_t *data = (uint8_t *) device->events.bo->map;
      }
      static uint8_t
   event_get_value(struct v3dv_device *device, struct v3dv_event *event)
   {
      uint8_t *data = (uint8_t *) device->events.bo->map;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateEvent(VkDevice _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            struct v3dv_event *event = allocate_event(device);
   if (!event) {
      result = vk_error(device, VK_ERROR_OUT_OF_DEVICE_MEMORY);
               event_set_value(device, event, 0);
   *pEvent = v3dv_event_to_handle(event);
         fail:
         }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyEvent(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (!event)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_GetEventStatus(VkDevice _device, VkEvent _event)
   {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
   V3DV_FROM_HANDLE(v3dv_event, event, _event);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_SetEvent(VkDevice _device, VkEvent _event)
   {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
   V3DV_FROM_HANDLE(v3dv_event, event, _event);
   event_set_value(device, event, 1);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_ResetEvent(VkDevice _device, VkEvent _event)
   {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
   V3DV_FROM_HANDLE(v3dv_event, event, _event);
   event_set_value(device, event, 0);
      }
      static void
   cmd_buffer_emit_set_event(struct v3dv_cmd_buffer *cmd_buffer,
               {
               struct v3dv_device *device = cmd_buffer->device;
                     v3dv_CmdBindPipeline(commandBuffer,
                  v3dv_CmdBindDescriptorSets(commandBuffer,
                        assert(event->index < device->events.event_count);
   uint32_t offset = event->index;
   v3dv_CmdPushConstants(commandBuffer,
                        v3dv_CmdPushConstants(commandBuffer,
                                    }
      static void
   cmd_buffer_emit_wait_event(struct v3dv_cmd_buffer *cmd_buffer,
         {
      struct v3dv_device *device = cmd_buffer->device;
                     v3dv_CmdBindPipeline(commandBuffer,
                  v3dv_CmdBindDescriptorSets(commandBuffer,
                        assert(event->index < device->events.event_count);
   uint32_t offset = event->index;
   v3dv_CmdPushConstants(commandBuffer,
                                    }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_CmdSetEvent2(VkCommandBuffer commandBuffer,
               {
      V3DV_FROM_HANDLE(v3dv_cmd_buffer, cmd_buffer, commandBuffer);
            /* Event (re)sets can only happen outside a render pass instance so we
   * should not be in the middle of job recording.
   */
   assert(cmd_buffer->state.pass == NULL);
            /* We need to add the compute stage to the dstStageMask of all dependencies,
   * so let's go ahead and patch the dependency info we receive.
   */
            uint32_t memory_barrier_count = pDependencyInfo->memoryBarrierCount;
   VkMemoryBarrier2 *memory_barriers = memory_barrier_count ?
      vk_alloc2(&device->vk.alloc, NULL,
            for (int i = 0; i < memory_barrier_count; i++) {
      memory_barriers[i] = pDependencyInfo->pMemoryBarriers[i];
               uint32_t buffer_barrier_count = pDependencyInfo->bufferMemoryBarrierCount;
   VkBufferMemoryBarrier2 *buffer_barriers = buffer_barrier_count ?
      vk_alloc2(&device->vk.alloc, NULL,
            for (int i = 0; i < buffer_barrier_count; i++) {
      buffer_barriers[i] = pDependencyInfo->pBufferMemoryBarriers[i];
               uint32_t image_barrier_count = pDependencyInfo->imageMemoryBarrierCount;
   VkImageMemoryBarrier2 *image_barriers = image_barrier_count ?
      vk_alloc2(&device->vk.alloc, NULL,
            for (int i = 0; i < image_barrier_count; i++) {
      image_barriers[i] = pDependencyInfo->pImageMemoryBarriers[i];
               VkDependencyInfo info = {
      .sType = pDependencyInfo->sType,
   .dependencyFlags = pDependencyInfo->dependencyFlags,
   .memoryBarrierCount = memory_barrier_count,
   .pMemoryBarriers = memory_barriers,
   .bufferMemoryBarrierCount = buffer_barrier_count,
   .pBufferMemoryBarriers = buffer_barriers,
   .imageMemoryBarrierCount = image_barrier_count,
                                 if (memory_barriers)
         if (buffer_barriers)
         if (image_barriers)
      }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_CmdResetEvent2(VkCommandBuffer commandBuffer,
               {
      V3DV_FROM_HANDLE(v3dv_cmd_buffer, cmd_buffer, commandBuffer);
            /* Event (re)sets can only happen outside a render pass instance so we
   * should not be in the middle of job recording.
   */
   assert(cmd_buffer->state.pass == NULL);
            VkMemoryBarrier2 barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
   .srcStageMask = stageMask,
      };
   VkDependencyInfo info = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   .memoryBarrierCount = 1,
      };
               }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_CmdWaitEvents2(VkCommandBuffer commandBuffer,
                     {
      V3DV_FROM_HANDLE(v3dv_cmd_buffer, cmd_buffer, commandBuffer);
   for (uint32_t i = 0; i < eventCount; i++) {
      struct v3dv_event *event = v3dv_event_from_handle(pEvents[i]);;
               /* We need to add the compute stage to the srcStageMask of all dependencies,
   * so let's go ahead and patch the dependency info we receive.
   */
   struct v3dv_device *device = cmd_buffer->device;
   for (int e = 0; e < eventCount; e++) {
               uint32_t memory_barrier_count = info->memoryBarrierCount;
   VkMemoryBarrier2 *memory_barriers = memory_barrier_count ?
      vk_alloc2(&device->vk.alloc, NULL,
            for (int i = 0; i < memory_barrier_count; i++) {
      memory_barriers[i] = info->pMemoryBarriers[i];
               uint32_t buffer_barrier_count = info->bufferMemoryBarrierCount;
   VkBufferMemoryBarrier2 *buffer_barriers = buffer_barrier_count ?
      vk_alloc2(&device->vk.alloc, NULL,
            for (int i = 0; i < buffer_barrier_count; i++) {
      buffer_barriers[i] = info->pBufferMemoryBarriers[i];
               uint32_t image_barrier_count = info->imageMemoryBarrierCount;
   VkImageMemoryBarrier2 *image_barriers = image_barrier_count ?
      vk_alloc2(&device->vk.alloc, NULL,
            for (int i = 0; i < image_barrier_count; i++) {
      image_barriers[i] = info->pImageMemoryBarriers[i];
               VkDependencyInfo new_info = {
      .sType = info->sType,
   .dependencyFlags = info->dependencyFlags,
   .memoryBarrierCount = memory_barrier_count,
   .pMemoryBarriers = memory_barriers,
   .bufferMemoryBarrierCount = buffer_barrier_count,
   .pBufferMemoryBarriers = buffer_barriers,
   .imageMemoryBarrierCount = image_barrier_count,
                        if (memory_barriers)
         if (buffer_barriers)
         if (image_barriers)
         }
