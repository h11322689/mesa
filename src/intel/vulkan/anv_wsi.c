   /*
   * Copyright © 2015 Intel Corporation
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
      #include "anv_private.h"
   #include "anv_measure.h"
   #include "wsi_common.h"
   #include "vk_fence.h"
   #include "vk_queue.h"
   #include "vk_semaphore.h"
   #include "vk_util.h"
      static PFN_vkVoidFunction
   anv_wsi_proc_addr(VkPhysicalDevice physicalDevice, const char *pName)
   {
      ANV_FROM_HANDLE(anv_physical_device, pdevice, physicalDevice);
      }
      static VkQueue
   anv_wsi_get_prime_blit_queue(VkDevice _device)
   {
               vk_foreach_queue(_queue, &device->vk) {
      struct anv_queue *queue = (struct anv_queue *)_queue;
   if (queue->family->queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))
      }
      }
      VkResult
   anv_init_wsi(struct anv_physical_device *physical_device)
   {
               result = wsi_device_init(&physical_device->wsi_device,
                           anv_physical_device_to_handle(physical_device),
   anv_wsi_proc_addr,
   if (result != VK_SUCCESS)
            physical_device->wsi_device.supports_modifiers = true;
   physical_device->wsi_device.get_blit_queue = anv_wsi_get_prime_blit_queue;
   if (physical_device->info.kmd_type == INTEL_KMD_TYPE_I915) {
      physical_device->wsi_device.signal_semaphore_with_memory = true;
                        wsi_device_setup_syncobj_fd(&physical_device->wsi_device,
               }
      void
   anv_finish_wsi(struct anv_physical_device *physical_device)
   {
      physical_device->vk.wsi_device = NULL;
   wsi_device_finish(&physical_device->wsi_device,
      }
      VkResult anv_AcquireNextImage2KHR(
      VkDevice _device,
   const VkAcquireNextImageInfoKHR *pAcquireInfo,
      {
               VkResult result =
      wsi_common_acquire_next_image2(&device->physical->wsi_device,
      if (result == VK_SUCCESS)
               }
      VkResult anv_QueuePresentKHR(
      VkQueue                                  _queue,
      {
      ANV_FROM_HANDLE(anv_queue, queue, _queue);
   struct anv_device *device = queue->device;
            if (device->debug_frame_desc) {
      #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
         if (device->physical->memory.need_flush) {
      intel_flush_range(device->debug_frame_desc,
      #endif
               if (u_trace_should_process(&device->ds.trace_context))
            result = vk_queue_wait_before_present(&queue->vk, pPresentInfo);
   if (result != VK_SUCCESS)
            result = wsi_common_queue_present(&device->physical->wsi_device,
                        if (u_trace_should_process(&device->ds.trace_context))
               }
