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
      /**
   * This file implements VkQueue
   */
      #include "anv_private.h"
      VkResult
   anv_queue_init(struct anv_device *device, struct anv_queue *queue,
                     {
      struct anv_physical_device *pdevice = device->physical;
            result = vk_queue_init(&queue->vk, &device->vk, pCreateInfo,
         if (result != VK_SUCCESS)
            if (INTEL_DEBUG(DEBUG_SYNC)) {
      result = vk_sync_create(&device->vk,
               if (result != VK_SUCCESS) {
      vk_queue_finish(&queue->vk);
                                    assert(queue->vk.queue_family_index < pdevice->queue.family_count);
   queue->family = &pdevice->queue.families[queue->vk.queue_family_index];
               }
      void
   anv_queue_finish(struct anv_queue *queue)
   {
      if (queue->sync)
               }
