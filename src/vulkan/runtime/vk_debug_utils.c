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
      #include "vk_debug_utils.h"
      #include "vk_common_entrypoints.h"
   #include "vk_command_buffer.h"
   #include "vk_device.h"
   #include "vk_queue.h"
   #include "vk_object.h"
   #include "vk_alloc.h"
   #include "vk_util.h"
   #include "stdarg.h"
   #include "util/u_dynarray.h"
      void
   vk_debug_message(struct vk_instance *instance,
                     {
               list_for_each_entry(struct vk_debug_utils_messenger, messenger,
            if ((messenger->severity & severity) &&
      (messenger->type & types))
               }
      /* This function intended to be used by the drivers to report a
   * message to the special messenger, provided in the pNext chain while
   * creating an instance. It's only meant to be used during
   * vkCreateInstance or vkDestroyInstance calls.
   */
   void
   vk_debug_message_instance(struct vk_instance *instance,
                           VkDebugUtilsMessageSeverityFlagBitsEXT severity,
   {
      if (list_is_empty(&instance->debug_utils.instance_callbacks))
            const VkDebugUtilsMessengerCallbackDataEXT cbData = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT,
   .pMessageIdName = pMessageIdName,
   .messageIdNumber = messageIdNumber,
               list_for_each_entry(struct vk_debug_utils_messenger, messenger,
            if ((messenger->severity & severity) &&
      (messenger->type & types))
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_CreateDebugUtilsMessengerEXT(
      VkInstance _instance,
   const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
   const VkAllocationCallbacks *pAllocator,
      {
               struct vk_debug_utils_messenger *messenger =
      vk_alloc2(&instance->alloc, pAllocator,
               if (!messenger)
            if (pAllocator)
         else
            vk_object_base_init(NULL, &messenger->base,
            messenger->severity = pCreateInfo->messageSeverity;
   messenger->type = pCreateInfo->messageType;
   messenger->callback = pCreateInfo->pfnUserCallback;
            mtx_lock(&instance->debug_utils.callbacks_mutex);
   list_addtail(&messenger->link, &instance->debug_utils.callbacks);
                        }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_SubmitDebugUtilsMessageEXT(
      VkInstance _instance,
   VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
   VkDebugUtilsMessageTypeFlagsEXT messageTypes,
      {
                  }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DestroyDebugUtilsMessengerEXT(
      VkInstance _instance,
   VkDebugUtilsMessengerEXT _messenger,
      {
      VK_FROM_HANDLE(vk_instance, instance, _instance);
            if (messenger == NULL)
            mtx_lock(&instance->debug_utils.callbacks_mutex);
   list_del(&messenger->link);
            vk_object_base_finish(&messenger->base);
      }
      static VkResult
   vk_common_set_object_name_locked(
      struct vk_device *device,
      {
      if (unlikely(device->swapchain_name == NULL)) {
      /* Even though VkSwapchain/Surface are non-dispatchable objects, we know
   * a priori that these are actually pointers so we can use
   * the pointer hash table for them.
   */
   device->swapchain_name = _mesa_pointer_hash_table_create(NULL);
   if (device->swapchain_name == NULL)
               char *object_name = vk_strdup(&device->alloc, pNameInfo->pObjectName,
         if (object_name == NULL)
         struct hash_entry *entry =
      _mesa_hash_table_search(device->swapchain_name,
      if (unlikely(entry == NULL)) {
      entry = _mesa_hash_table_insert(device->swapchain_name,
               if (entry == NULL) {
      vk_free(&device->alloc, object_name);
         } else {
      vk_free(&device->alloc, entry->data);
      }
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_SetDebugUtilsObjectNameEXT(
      VkDevice _device,
      {
            #ifdef ANDROID
      if (pNameInfo->objectType == VK_OBJECT_TYPE_SWAPCHAIN_KHR ||
      #else
         #endif
         mtx_lock(&device->swapchain_name_mtx);
   VkResult res = vk_common_set_object_name_locked(device, pNameInfo);
   mtx_unlock(&device->swapchain_name_mtx);
               struct vk_object_base *object =
      vk_object_base_from_u64_handle(pNameInfo->objectHandle,
         assert(object->device != NULL || object->instance != NULL);
   VkAllocationCallbacks *alloc = object->device != NULL ?
         if (object->object_name) {
      vk_free(alloc, object->object_name);
      }
   object->object_name = vk_strdup(alloc, pNameInfo->pObjectName,
         if (!object->object_name)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_SetDebugUtilsObjectTagEXT(
      VkDevice _device,
      {
      /* no-op */
      }
      static void
   vk_common_append_debug_label(struct vk_device *device,
               {
      util_dynarray_append(labels, VkDebugUtilsLabelEXT, *pLabelInfo);
   VkDebugUtilsLabelEXT *current_label =
         current_label->pLabelName =
      vk_strdup(&device->alloc, current_label->pLabelName,
   }
      static void
   vk_common_pop_debug_label(struct vk_device *device,
         {
      if (labels->size == 0)
            VkDebugUtilsLabelEXT previous_label =
            }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdBeginDebugUtilsLabelEXT(
      VkCommandBuffer _commandBuffer,
      {
               /* If the latest label was submitted by CmdInsertDebugUtilsLabelEXT, we
   * should remove it first.
   */
   if (!command_buffer->region_begin) {
      vk_common_pop_debug_label(command_buffer->base.device,
               vk_common_append_debug_label(command_buffer->base.device,
                  }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdEndDebugUtilsLabelEXT(VkCommandBuffer _commandBuffer)
   {
               /* If the latest label was submitted by CmdInsertDebugUtilsLabelEXT, we
   * should remove it first.
   */
   if (!command_buffer->region_begin) {
      vk_common_pop_debug_label(command_buffer->base.device,
               vk_common_pop_debug_label(command_buffer->base.device,
            }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdInsertDebugUtilsLabelEXT(
      VkCommandBuffer _commandBuffer,
      {
               /* If the latest label was submitted by CmdInsertDebugUtilsLabelEXT, we
   * should remove it first.
   */
   if (!command_buffer->region_begin) {
      vk_common_append_debug_label(command_buffer->base.device,
                     vk_common_append_debug_label(command_buffer->base.device,
                  }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_QueueBeginDebugUtilsLabelEXT(
      VkQueue _queue,
      {
               /* If the latest label was submitted by QueueInsertDebugUtilsLabelEXT, we
   * should remove it first.
   */
   if (!queue->region_begin)
            vk_common_append_debug_label(queue->base.device,
                  }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_QueueEndDebugUtilsLabelEXT(VkQueue _queue)
   {
               /* If the latest label was submitted by QueueInsertDebugUtilsLabelEXT, we
   * should remove it first.
   */
   if (!queue->region_begin)
            vk_common_pop_debug_label(queue->base.device, &queue->labels);
      }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_QueueInsertDebugUtilsLabelEXT(
      VkQueue _queue,
      {
               /* If the latest label was submitted by QueueInsertDebugUtilsLabelEXT, we
   * should remove it first.
   */
   if (!queue->region_begin)
            vk_common_append_debug_label(queue->base.device,
                  }
