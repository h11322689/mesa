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
      #include "rmv/vk_rmv_common.h"
   #include "rmv/vk_rmv_tokens.h"
   #include "radv_private.h"
   #include "vk_common_entrypoints.h"
   #include "wsi_common_entrypoints.h"
      VKAPI_ATTR VkResult VKAPI_CALL
   rmv_QueuePresentKHR(VkQueue _queue, const VkPresentInfoKHR *pPresentInfo)
   {
      RADV_FROM_HANDLE(radv_queue, queue, _queue);
            VkResult res = queue->device->layer_dispatch.rmv.QueuePresentKHR(_queue, pPresentInfo);
   if ((res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) || !device->vk.memory_trace_data.is_enabled)
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   rmv_FlushMappedMemoryRanges(VkDevice _device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
   {
               VkResult res = device->layer_dispatch.rmv.FlushMappedMemoryRanges(_device, memoryRangeCount, pMemoryRanges);
   if (res != VK_SUCCESS || !device->vk.memory_trace_data.is_enabled)
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   rmv_InvalidateMappedMemoryRanges(VkDevice _device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
   {
               VkResult res = device->layer_dispatch.rmv.InvalidateMappedMemoryRanges(_device, memoryRangeCount, pMemoryRanges);
   if (res != VK_SUCCESS || !device->vk.memory_trace_data.is_enabled)
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   rmv_DebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT *pNameInfo)
   {
      assert(pNameInfo->sType == VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT);
   VkDebugUtilsObjectNameInfoEXT name_info;
   name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
   name_info.objectType = pNameInfo->objectType;
   name_info.objectHandle = pNameInfo->object;
   name_info.pObjectName = pNameInfo->pObjectName;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   rmv_SetDebugUtilsObjectNameEXT(VkDevice _device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo)
   {
      assert(pNameInfo->sType == VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
            VkResult result = device->layer_dispatch.rmv.SetDebugUtilsObjectNameEXT(_device, pNameInfo);
   if (result != VK_SUCCESS || !device->vk.memory_trace_data.is_enabled)
            switch (pNameInfo->objectType) {
   /* only name object types we care about */
   case VK_OBJECT_TYPE_BUFFER:
   case VK_OBJECT_TYPE_DEVICE_MEMORY:
   case VK_OBJECT_TYPE_IMAGE:
   case VK_OBJECT_TYPE_EVENT:
   case VK_OBJECT_TYPE_QUERY_POOL:
   case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
   case VK_OBJECT_TYPE_PIPELINE:
         default:
                  size_t name_len = strlen(pNameInfo->pObjectName);
   char *name_buf = malloc(name_len + 1);
   if (!name_buf) {
      /*
   * Silently fail, so that applications may still continue if possible.
   */
      }
            simple_mtx_lock(&device->vk.memory_trace_data.token_mtx);
   struct vk_rmv_userdata_token token;
   token.name = name_buf;
            vk_rmv_emit_token(&device->vk.memory_trace_data, VK_RMV_TOKEN_TYPE_USERDATA, &token);
               }
