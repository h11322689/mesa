   /*
   * Copyright © 2021 Collabora Ltd.
   *
   * Derived from tu_device.c which is:
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "genxml/gen_macros.h"
      #include "decode.h"
      #include "panvk_cs.h"
   #include "panvk_private.h"
      #include "vk_drm_syncobj.h"
      static void
   panvk_queue_submit_batch(struct panvk_queue *queue, struct panvk_batch *batch,
               {
      const struct panvk_device *dev = queue->device;
   unsigned debug = dev->physical_device->instance->debug_flags;
   const struct panfrost_device *pdev = &dev->physical_device->pdev;
            /* Reset the batch if it's already been issued */
   if (batch->issued) {
      util_dynarray_foreach(&batch->jobs, void *, job)
            /* Reset the tiler before re-issuing the batch */
   if (batch->tiler.descs.cpu) {
      memcpy(batch->tiler.descs.cpu, batch->tiler.templ,
                  if (batch->scoreboard.first_job) {
      struct drm_panfrost_submit submit = {
      .bo_handles = (uintptr_t)bos,
   .bo_handle_count = nr_bos,
   .in_syncs = (uintptr_t)in_fences,
   .in_sync_count = nr_in_fences,
   .out_sync = queue->sync,
               ret = drmIoctl(pdev->fd, DRM_IOCTL_PANFROST_SUBMIT, &submit);
            if (debug & (PANVK_DEBUG_TRACE | PANVK_DEBUG_SYNC)) {
      ret =
                     if (debug & PANVK_DEBUG_TRACE)
                  if (debug & PANVK_DEBUG_DUMP)
               if (batch->fragment_job) {
      struct drm_panfrost_submit submit = {
      .bo_handles = (uintptr_t)bos,
   .bo_handle_count = nr_bos,
   .out_sync = queue->sync,
   .jc = batch->fragment_job,
               if (batch->scoreboard.first_job) {
      submit.in_syncs = (uintptr_t)(&queue->sync);
      } else {
      submit.in_syncs = (uintptr_t)in_fences;
               ret = drmIoctl(pdev->fd, DRM_IOCTL_PANFROST_SUBMIT, &submit);
   assert(!ret);
   if (debug & (PANVK_DEBUG_TRACE | PANVK_DEBUG_SYNC)) {
      ret =
                     if (debug & PANVK_DEBUG_TRACE)
            if (debug & PANVK_DEBUG_DUMP)
               if (debug & PANVK_DEBUG_TRACE)
               }
      static void
   panvk_queue_transfer_sync(struct panvk_queue *queue, uint32_t syncobj)
   {
      const struct panfrost_device *pdev = &queue->device->physical_device->pdev;
            struct drm_syncobj_handle handle = {
      .handle = queue->sync,
   .flags = DRM_SYNCOBJ_HANDLE_TO_FD_FLAGS_EXPORT_SYNC_FILE,
               ret = drmIoctl(pdev->fd, DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD, &handle);
   assert(!ret);
            handle.handle = syncobj;
   ret = drmIoctl(pdev->fd, DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE, &handle);
               }
      static void
   panvk_add_wait_event_syncobjs(struct panvk_batch *batch, uint32_t *in_fences,
         {
      util_dynarray_foreach(&batch->event_ops, struct panvk_event_op, op) {
      switch (op->type) {
   case PANVK_EVENT_OP_SET:
      /* Nothing to do yet */
      case PANVK_EVENT_OP_RESET:
      /* Nothing to do yet */
      case PANVK_EVENT_OP_WAIT:
      in_fences[(*nr_in_fences)++] = op->event->syncobj;
      default:
               }
      static void
   panvk_signal_event_syncobjs(struct panvk_queue *queue,
         {
               util_dynarray_foreach(&batch->event_ops, struct panvk_event_op, op) {
      switch (op->type) {
   case PANVK_EVENT_OP_SET: {
      panvk_queue_transfer_sync(queue, op->event->syncobj);
      }
   case PANVK_EVENT_OP_RESET: {
               struct drm_syncobj_array objs = {
                  int ret = drmIoctl(pdev->fd, DRM_IOCTL_SYNCOBJ_RESET, &objs);
   assert(!ret);
      }
   case PANVK_EVENT_OP_WAIT:
      /* Nothing left to do */
      default:
               }
      VkResult
   panvk_per_arch(queue_submit)(struct vk_queue *vk_queue,
         {
      struct panvk_queue *queue = container_of(vk_queue, struct panvk_queue, vk);
            unsigned nr_semaphores = submit->wait_count + 1;
            semaphores[0] = queue->sync;
   for (unsigned i = 0; i < submit->wait_count; i++) {
      assert(vk_sync_type_is_drm_syncobj(submit->waits[i].sync->type));
   struct vk_drm_syncobj *syncobj =
                        for (uint32_t j = 0; j < submit->command_buffer_count; ++j) {
      struct panvk_cmd_buffer *cmdbuf =
            list_for_each_entry(struct panvk_batch, batch, &cmdbuf->batches, node) {
      /* FIXME: should be done at the batch level */
   unsigned nr_bos =
      panvk_pool_num_bos(&cmdbuf->desc_pool) +
   panvk_pool_num_bos(&cmdbuf->varying_pool) +
   panvk_pool_num_bos(&cmdbuf->tls_pool) +
   (batch->fb.info ? batch->fb.info->attachment_count : 0) +
   (batch->blit.src ? 1 : 0) + (batch->blit.dst ? 1 : 0) +
                                                                  if (batch->fb.info) {
      for (unsigned i = 0; i < batch->fb.info->attachment_count; i++) {
      const struct pan_image *image = pan_image_view_get_plane(
                                                                                    /* Merge identical BO entries. */
   for (unsigned x = 0; x < nr_bos; x++) {
      for (unsigned y = x + 1; y < nr_bos;) {
      if (bos[x] == bos[y])
         else
                  unsigned nr_in_fences = 0;
   unsigned max_wait_event_syncobjs = util_dynarray_num_elements(
         uint32_t in_fences[nr_semaphores + max_wait_event_syncobjs];
                                                         /* Transfer the out fence to signal semaphores */
   for (unsigned i = 0; i < submit->signal_count; i++) {
      assert(vk_sync_type_is_drm_syncobj(submit->signals[i].sync->type));
   struct vk_drm_syncobj *syncobj =
                           }
      VkResult
   panvk_per_arch(CreateSampler)(VkDevice _device,
                     {
      VK_FROM_HANDLE(panvk_device, device, _device);
                     sampler = vk_object_alloc(&device->vk, pAllocator, sizeof(*sampler),
         if (!sampler)
            STATIC_ASSERT(sizeof(sampler->desc) >= pan_size(SAMPLER));
   panvk_per_arch(emit_sampler)(pCreateInfo, &sampler->desc);
               }
