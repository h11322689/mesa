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
      #include "i915/anv_queue.h"
   #include "xe/anv_queue.h"
      static VkResult
   anv_create_engine(struct anv_device *device,
               {
      switch (device->info->kmd_type) {
   case INTEL_KMD_TYPE_I915:
         case INTEL_KMD_TYPE_XE:
         default:
      unreachable("Missing");
         }
      static void
   anv_destroy_engine(struct anv_queue *queue)
   {
      struct anv_device *device = queue->device;
   switch (device->info->kmd_type) {
   case INTEL_KMD_TYPE_I915:
      anv_i915_destroy_engine(device, queue);
      case INTEL_KMD_TYPE_XE:
      anv_xe_destroy_engine(device, queue);
      default:
            }
      VkResult
   anv_queue_init(struct anv_device *device, struct anv_queue *queue,
               {
      struct anv_physical_device *pdevice = device->physical;
   assert(queue->vk.queue_family_index < pdevice->queue.family_count);
   struct anv_queue_family *queue_family =
                  result = vk_queue_init(&queue->vk, &device->vk, pCreateInfo,
         if (result != VK_SUCCESS)
            queue->vk.driver_submit = anv_queue_submit;
   queue->device = device;
   queue->family = queue_family;
            result = anv_create_engine(device, queue, pCreateInfo);
   if (result != VK_SUCCESS) {
      vk_queue_finish(&queue->vk);
               if (INTEL_DEBUG(DEBUG_SYNC)) {
      result = vk_sync_create(&device->vk,
               if (result != VK_SUCCESS) {
      anv_queue_finish(queue);
                  if (queue_family->engine_class == INTEL_ENGINE_CLASS_COPY ||
      queue_family->engine_class == INTEL_ENGINE_CLASS_COMPUTE) {
   result = vk_sync_create(&device->vk,
               if (result != VK_SUCCESS) {
      anv_queue_finish(queue);
                     }
      void
   anv_queue_finish(struct anv_queue *queue)
   {
      if (queue->sync)
            if (queue->companion_sync)
            anv_destroy_engine(queue);
      }
