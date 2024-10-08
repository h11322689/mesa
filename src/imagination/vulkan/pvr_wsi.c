   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * based on intel anv code:
   * Copyright © 2015 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <stdint.h>
   #include <stddef.h>
   #include <stdbool.h>
   #include <vulkan/vulkan.h>
      #include "pvr_private.h"
   #include "util/u_atomic.h"
   #include "vk_object.h"
   #include "wsi_common.h"
      static PFN_vkVoidFunction pvr_wsi_proc_addr(VkPhysicalDevice physicalDevice,
         {
                  }
      VkResult pvr_wsi_init(struct pvr_physical_device *pdevice)
   {
               result = wsi_device_init(&pdevice->wsi_device,
                           pvr_physical_device_to_handle(pdevice),
   pvr_wsi_proc_addr,
   if (result != VK_SUCCESS)
            pdevice->wsi_device.supports_modifiers = true;
               }
      void pvr_wsi_finish(struct pvr_physical_device *pdevice)
   {
      pdevice->vk.wsi_device = NULL;
      }
      VkResult pvr_QueuePresentKHR(VkQueue _queue,
         {
      PVR_FROM_HANDLE(pvr_queue, queue, _queue);
            result = wsi_common_queue_present(&queue->device->pdevice->wsi_device,
                           if (result != VK_SUCCESS)
                        }
