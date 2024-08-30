   /*
   * Copyright © 2022 Collabora, Ltd.
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
      #include "vk_buffer.h"
      #include "vk_common_entrypoints.h"
   #include "vk_alloc.h"
   #include "vk_device.h"
   #include "vk_util.h"
      void
   vk_buffer_init(struct vk_device *device,
               {
               assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
            buffer->create_flags = pCreateInfo->flags;
   buffer->size = pCreateInfo->size;
            const VkBufferUsageFlags2CreateInfoKHR *usage2_info =
      vk_find_struct_const(pCreateInfo->pNext,
      if (usage2_info != NULL)
      }
      void *
   vk_buffer_create(struct vk_device *device,
                     {
      struct vk_buffer *buffer =
      vk_zalloc2(&device->alloc, alloc, size, 8,
      if (buffer == NULL)
                        }
      void
   vk_buffer_finish(struct vk_buffer *buffer)
   {
         }
      void
   vk_buffer_destroy(struct vk_device *device,
               {
         }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetBufferMemoryRequirements(VkDevice _device,
               {
               VkBufferMemoryRequirementsInfo2 info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
      };
   VkMemoryRequirements2 reqs = {
         };
               }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetBufferMemoryRequirements2(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_device, device, _device);
            VkBufferCreateInfo pCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
   .pNext = NULL,
   .usage = buffer->usage,
   .size = buffer->size,
   .flags = buffer->create_flags,
   .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
   .queueFamilyIndexCount = 0,
      };
   VkDeviceBufferMemoryRequirements info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
   .pNext = NULL,
      };
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_BindBufferMemory(VkDevice _device,
                     {
               VkBindBufferMemoryInfo bind = {
      .sType         = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
   .buffer        = buffer,
   .memory        = memory,
                  }
