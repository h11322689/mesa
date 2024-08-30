   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "dzn_private.h"
   #include "vk_util.h"
      static PFN_vkVoidFunction VKAPI_PTR
   dzn_wsi_proc_addr(VkPhysicalDevice physicalDevice, const char *pName)
   {
      VK_FROM_HANDLE(dzn_physical_device, pdevice, physicalDevice);
      }
      static void *
   dzn_wsi_get_d3d12_command_queue(VkDevice dev)
   {
                  }
      static VkQueue
   dzn_wsi_get_blit_queue(VkDevice dev)
   {
                  }
      static bool
   dzn_wsi_needs_blits(VkDevice dev)
   {
      VK_FROM_HANDLE(dzn_device, device, dev);
      }
      static VkResult
   dzn_wsi_create_image_memory(VkDevice dev, void *resource,
               {
               struct dzn_device_memory *mem =
      vk_zalloc2(&device->vk.alloc, alloc, sizeof(*mem), 8,
      if (!mem)
            vk_object_base_init(&device->vk, &mem->base, VK_OBJECT_TYPE_DEVICE_MEMORY);
   mem->dedicated_res = resource;
            *out = dzn_device_memory_to_handle(mem);
      }
      void
   dzn_wsi_finish(struct dzn_physical_device *physical_device)
   {
      wsi_device_finish(&physical_device->wsi_device,
      }
      VkResult
   dzn_wsi_init(struct dzn_physical_device *physical_device)
   {
            #ifdef _WIN32
         #else
         #endif
         result = wsi_device_init(&physical_device->wsi_device,
                                    if (result != VK_SUCCESS)
            physical_device->wsi_device.win32.get_d3d12_command_queue =
         physical_device->wsi_device.win32.requires_blits = dzn_wsi_needs_blits;
   physical_device->wsi_device.get_blit_queue = dzn_wsi_get_blit_queue;
   physical_device->wsi_device.win32.create_image_memory =
         physical_device->wsi_device.supports_modifiers = false;
               }
