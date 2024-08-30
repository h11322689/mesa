   /*
   * Copyright © 2023 Intel Corporation
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
      #include "xe/anv_batch_chain.h"
      #include "anv_private.h"
      #include <xf86drm.h>
      #include "drm-uapi/xe_drm.h"
      VkResult
   xe_execute_simple_batch(struct anv_queue *queue,
                     {
      struct anv_device *device = queue->device;
   VkResult result = VK_SUCCESS;
   uint32_t syncobj_handle;
   uint32_t exec_queue_id = is_companion_rcs_batch ?
               if (drmSyncobjCreate(device->fd, 0, &syncobj_handle))
            struct drm_xe_sync sync = {
      .flags = DRM_XE_SYNC_SYNCOBJ | DRM_XE_SYNC_SIGNAL,
      };
   struct drm_xe_exec exec = {
      .exec_queue_id = exec_queue_id,
   .num_batch_buffer = 1,
   .address = batch_bo->offset,
   .num_syncs = 1,
               if (intel_ioctl(device->fd, DRM_IOCTL_XE_EXEC, &exec)) {
      result = vk_device_set_lost(&device->vk, "XE_EXEC failed: %m");
               struct drm_syncobj_wait wait = {
      .handles = (uintptr_t)&syncobj_handle,
   .timeout_nsec = INT64_MAX,
      };
   if (intel_ioctl(device->fd, DRM_IOCTL_SYNCOBJ_WAIT, &wait))
         exec_error:
                  }
      #define TYPE_SIGNAL true
   #define TYPE_WAIT false
      static void
   xe_exec_fill_sync(struct drm_xe_sync *xe_sync, struct vk_sync *vk_sync,
         {
      if (unlikely(!vk_sync_type_is_drm_syncobj(vk_sync->type))) {
      unreachable("Unsupported sync type");
               const struct vk_drm_syncobj *syncobj = vk_sync_as_drm_syncobj(vk_sync);
            if (value) {
      xe_sync->flags |= DRM_XE_SYNC_TIMELINE_SYNCOBJ;
      } else {
                  if (signal)
      }
      static VkResult
   xe_exec_process_syncs(struct anv_queue *queue,
                        uint32_t wait_count, const struct vk_sync_wait *waits,
      {
      struct anv_device *device = queue->device;
   uint32_t num_syncs = wait_count + signal_count + (utrace_submit ? 1 : 0) +
         if (!num_syncs)
            struct drm_xe_sync *xe_syncs = vk_zalloc(&device->vk.alloc,
               if (!xe_syncs)
                     /* Signal the utrace sync only if it doesn't have a batch. Otherwise the
   * it's the utrace batch that should signal its own sync.
   */
   if (utrace_submit && !utrace_submit->batch_bo) {
                           for (uint32_t i = 0; i < wait_count; i++) {
      struct drm_xe_sync *xe_sync = &xe_syncs[count++];
            xe_exec_fill_sync(xe_sync, vk_wait->sync, vk_wait->wait_value,
               for (uint32_t i = 0; i < signal_count; i++) {
      struct drm_xe_sync *xe_sync = &xe_syncs[count++];
            xe_exec_fill_sync(xe_sync, vk_signal->sync, vk_signal->signal_value,
               if (queue->sync && !is_companion_rcs_queue) {
               xe_exec_fill_sync(xe_sync, queue->sync, 0,
               assert(count == num_syncs);
   *ret = xe_syncs;
   *ret_count = num_syncs;
      }
      static void
   xe_exec_print_debug(struct anv_queue *queue, uint32_t cmd_buffer_count,
                     {
      if (INTEL_DEBUG(DEBUG_SUBMIT))
      fprintf(stderr, "Batch offset=0x%016"PRIx64" on queue %u\n",
         anv_cmd_buffer_exec_batch_debug(queue, cmd_buffer_count, cmd_buffers,
            }
      VkResult
   xe_queue_exec_utrace_locked(struct anv_queue *queue,
         {
      struct anv_device *device = queue->device;
                  #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
      if (device->physical->memory.need_flush)
      intel_flush_range(utrace_submit->batch_bo->map,
   #endif
         struct drm_xe_exec exec = {
      .exec_queue_id = queue->exec_queue_id,
   .num_batch_buffer = 1,
   .syncs = (uintptr_t)&xe_sync,
   .num_syncs = 1,
      };
   if (likely(!device->info->no_hw)) {
      if (intel_ioctl(device->fd, DRM_IOCTL_XE_EXEC, &exec))
                  }
      static VkResult
   xe_companion_rcs_queue_exec_locked(struct anv_queue *queue,
                           {
      struct anv_device *device = queue->device;
            struct vk_sync_signal companion_sync = {
         };
   struct drm_xe_sync *xe_syncs = NULL;
   uint32_t xe_syncs_count = 0;
   result = xe_exec_process_syncs(queue,
                                 wait_count, waits,
   if (result != VK_SUCCESS)
            struct drm_xe_exec exec = {
      .exec_queue_id = queue->companion_rcs_id,
   .num_batch_buffer = 1,
   .syncs = (uintptr_t)xe_syncs,
               struct anv_cmd_buffer *first_cmd_buffer =
         struct anv_batch_bo *first_batch_bo =
      list_first_entry(&first_cmd_buffer->batch_bos, struct anv_batch_bo,
               xe_exec_print_debug(queue, cmd_buffer_count, cmd_buffers, NULL, 0, &exec,
            if (!device->info->no_hw) {
      if (intel_ioctl(device->fd, DRM_IOCTL_XE_EXEC, &exec))
      }
               }
      VkResult
   xe_queue_exec_locked(struct anv_queue *queue,
                        uint32_t wait_count,
   const struct vk_sync_wait *waits,
   uint32_t cmd_buffer_count,
   struct anv_cmd_buffer **cmd_buffers,
   uint32_t signal_count,
      {
      struct anv_device *device = queue->device;
            struct drm_xe_sync *xe_syncs = NULL;
   uint32_t xe_syncs_count = 0;
   result = xe_exec_process_syncs(queue, wait_count, waits,
                           if (result != VK_SUCCESS)
            /* If we have no batch for utrace, just forget about it now. */
   if (utrace_submit && !utrace_submit->batch_bo)
            struct drm_xe_exec exec = {
      .exec_queue_id = queue->exec_queue_id,
   .num_batch_buffer = 1,
   .syncs = (uintptr_t)xe_syncs,
               if (cmd_buffer_count) {
         #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
         if (device->physical->memory.need_flush)
   #endif
            struct anv_cmd_buffer *first_cmd_buffer = cmd_buffers[0];
   struct anv_batch_bo *first_batch_bo = list_first_entry(&first_cmd_buffer->batch_bos,
            } else {
                  xe_exec_print_debug(queue, cmd_buffer_count, cmd_buffers, perf_query_pool,
                           if (!device->info->no_hw) {
      if (intel_ioctl(device->fd, DRM_IOCTL_XE_EXEC, &exec))
      }
            if (cmd_buffer_count != 0 && cmd_buffers[0]->companion_rcs_cmd_buffer)
      result = xe_companion_rcs_queue_exec_locked(queue, cmd_buffer_count,
               if (result == VK_SUCCESS && queue->sync) {
      result = vk_sync_wait(&device->vk, queue->sync, 0,
         if (result != VK_SUCCESS)
               if (result == VK_SUCCESS && utrace_submit)
               }
