   /*
   * Copyright © 2020 Raspberry Pi Ltd
   * based on intel anv code:
   * Copyright © 2015 Intel Corporation
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
   #include "vk_util.h"
   #include "wsi_common.h"
   #include "wsi_common_drm.h"
      static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   v3dv_wsi_proc_addr(VkPhysicalDevice physicalDevice, const char *pName)
   {
      V3DV_FROM_HANDLE(v3dv_physical_device, pdevice, physicalDevice);
      }
      static bool
   v3dv_wsi_can_present_on_device(VkPhysicalDevice _pdevice, int fd)
   {
      V3DV_FROM_HANDLE(v3dv_physical_device, pdevice, _pdevice);
   assert(pdevice->display_fd != -1);
      }
      VkResult
   v3dv_wsi_init(struct v3dv_physical_device *physical_device)
   {
               result = wsi_device_init(&physical_device->wsi_device,
                                    if (result != VK_SUCCESS)
            physical_device->wsi_device.supports_modifiers = true;
   physical_device->wsi_device.can_present_on_device =
                        }
      void
   v3dv_wsi_finish(struct v3dv_physical_device *physical_device)
   {
      physical_device->vk.wsi_device = NULL;
   wsi_device_finish(&physical_device->wsi_device,
      }
      struct v3dv_image *
   v3dv_wsi_get_image_from_swapchain(VkSwapchainKHR swapchain, uint32_t index)
   {
      VkImage image = wsi_common_get_image(swapchain, index);
      }
