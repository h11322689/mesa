   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_queue.h"
      #include "nvk_cmd_buffer.h"
   #include "nvk_cmd_pool.h"
   #include "nvk_device.h"
   #include "nvk_buffer.h"
   #include "nvk_image.h"
   #include "nvk_device_memory.h"
   #include "nvk_physical_device.h"
      #include "nouveau_context.h"
      #include "drm-uapi/nouveau_drm.h"
      #include "vk_drm_syncobj.h"
      #include <xf86drm.h>
      #define NVK_PUSH_MAX_SYNCS 16
   #define NVK_PUSH_MAX_BINDS 4096
   #define NVK_PUSH_MAX_PUSH 1024
      struct push_builder {
      struct nvk_device *dev;
   uint32_t max_push;
   struct drm_nouveau_sync req_wait[NVK_PUSH_MAX_SYNCS];
   struct drm_nouveau_sync req_sig[NVK_PUSH_MAX_SYNCS];
   struct drm_nouveau_exec_push req_push[NVK_PUSH_MAX_PUSH];
   struct drm_nouveau_exec req;
   struct drm_nouveau_vm_bind vmbind;
   struct drm_nouveau_vm_bind_op bind_ops[NVK_PUSH_MAX_BINDS];
      };
      static void
   push_builder_init(struct nvk_device *dev, struct push_builder *pb,
         {
      pb->dev = dev;
   pb->max_push = is_vmbind ? 0 :
         pb->req = (struct drm_nouveau_exec) {
      .channel = dev->ws_ctx->channel,
   .push_count = 0,
   .wait_count = 0,
   .sig_count = 0,
   .push_ptr = (uintptr_t)&pb->req_push,
   .wait_ptr = (uintptr_t)&pb->req_wait,
      };
   pb->vmbind = (struct drm_nouveau_vm_bind) {
      .flags = DRM_NOUVEAU_VM_BIND_RUN_ASYNC,
   .op_count = 0,
   .op_ptr = (uintptr_t)&pb->bind_ops,
   .wait_count = 0,
   .sig_count = 0,
   .wait_ptr = (uintptr_t)&pb->req_wait,
      };
      }
      static void
   push_add_sync_wait(struct push_builder *pb,
         {
      struct vk_drm_syncobj *sync  = vk_sync_as_drm_syncobj(wait->sync);
   assert(sync);
   assert(pb->req.wait_count < NVK_PUSH_MAX_SYNCS);
   pb->req_wait[pb->req.wait_count++] = (struct drm_nouveau_sync) {
      .flags = wait->wait_value ? DRM_NOUVEAU_SYNC_TIMELINE_SYNCOBJ :
         .handle = sync->syncobj,
         }
      static void
   push_add_sync_signal(struct push_builder *pb,
         {
      struct vk_drm_syncobj *sync  = vk_sync_as_drm_syncobj(sig->sync);
   assert(sync);
   assert(pb->req.sig_count < NVK_PUSH_MAX_SYNCS);
   pb->req_sig[pb->req.sig_count++] = (struct drm_nouveau_sync) {
      .flags = sig->signal_value ? DRM_NOUVEAU_SYNC_TIMELINE_SYNCOBJ :
         .handle = sync->syncobj,
         }
      static void
   push_add_buffer_bind(struct push_builder *pb,
         {
      VK_FROM_HANDLE(nvk_buffer, buffer, bind_info->buffer);
   for (unsigned i = 0; i < bind_info->bindCount; i++) {
      const VkSparseMemoryBind *bind = &bind_info->pBinds[i];
            assert(bind->resourceOffset + bind->size <= buffer->vma_size_B);
            assert(pb->vmbind.op_count < NVK_PUSH_MAX_BINDS);
   pb->bind_ops[pb->vmbind.op_count++] = (struct drm_nouveau_vm_bind_op) {
      .op = mem ? DRM_NOUVEAU_VM_BIND_OP_MAP :
         .handle = mem ? mem->bo->handle : 0,
   .addr = buffer->addr + bind->resourceOffset,
   .bo_offset = bind->memoryOffset,
            }
      static void
   push_add_image_plane_opaque_bind(struct push_builder *pb,
                     {
               /* The offset of the bind range within the image */
   uint64_t image_bind_offset_B = bind->resourceOffset;
   uint64_t mem_bind_offset_B = bind->memoryOffset;
            /* If the bind starts before the plane, clamp from below */
   if (image_bind_offset_B < *image_plane_offset_B) {
      /* The offset of the plane within the range being bound */
   const uint64_t bind_plane_offset_B =
            /* If this plane lies above the bound range, skip this bind */
   if (bind_plane_offset_B >= bind_size_B)
            image_bind_offset_B += bind_plane_offset_B;
   mem_bind_offset_B += bind_plane_offset_B;
                        /* The offset of the bind range within the plane */
   const uint64_t plane_bind_offset_B =
            /* The bound range lies above the plane */
   if (plane_bind_offset_B >= plane->vma_size_B)
            /* Clamp the size to fit inside the plane */
   bind_size_B = MIN2(bind_size_B, plane->vma_size_B - plane_bind_offset_B);
                     assert(plane_bind_offset_B + bind_size_B <= plane->vma_size_B);
            assert(pb->vmbind.op_count < NVK_PUSH_MAX_BINDS);
   pb->bind_ops[pb->vmbind.op_count++] = (struct drm_nouveau_vm_bind_op) {
      .op = mem ? DRM_NOUVEAU_VM_BIND_OP_MAP :
         .handle = mem ? mem->bo->handle : 0,
   .addr = plane->addr + plane_bind_offset_B,
   .bo_offset = mem_bind_offset_B,
   .range = bind_size_B,
            skip:
      assert(plane->vma_size_B == plane->nil.size_B);
      }
      static void
   push_add_image_opaque_bind(struct push_builder *pb,
         {
      VK_FROM_HANDLE(nvk_image, image, bind_info->image);
   for (unsigned i = 0; i < bind_info->bindCount; i++) {
      uint64_t image_plane_offset_B = 0;
   for (unsigned plane = 0; plane < image->plane_count; plane++) {
      push_add_image_plane_opaque_bind(pb, &image->planes[plane],
            }
   if (image->stencil_copy_temp.nil.size_B > 0) {
      push_add_image_plane_opaque_bind(pb, &image->stencil_copy_temp,
                  }
      static void
   push_add_push(struct push_builder *pb, uint64_t addr, uint32_t range,
         {
      /* This is the hardware limit on all current GPUs */
   assert((addr % 4) == 0 && (range % 4) == 0);
            uint32_t flags = 0;
   if (no_prefetch)
            assert(pb->req.push_count < pb->max_push);
   pb->req_push[pb->req.push_count++] = (struct drm_nouveau_exec_push) {
      .va = addr,
   .va_len = range,
         }
      static VkResult
   bind_submit(struct push_builder *pb, struct nvk_queue *queue, bool sync)
   {
               pb->vmbind.wait_count = pb->req.wait_count;
   pb->vmbind.sig_count = pb->req.sig_count;
   err = drmCommandWriteRead(pb->dev->ws_dev->fd,
               if (err) {
      return vk_errorf(queue, VK_ERROR_UNKNOWN,
      }
      }
      static VkResult
   push_submit(struct push_builder *pb, struct nvk_queue *queue, bool sync)
   {
      int err;
   if (sync) {
      assert(pb->req.sig_count < NVK_PUSH_MAX_SYNCS);
   pb->req_sig[pb->req.sig_count++] = (struct drm_nouveau_sync) {
      .flags = DRM_NOUVEAU_SYNC_SYNCOBJ,
   .handle = queue->syncobj_handle,
         }
   err = drmCommandWriteRead(pb->dev->ws_dev->fd,
               if (err) {
      VkResult result = VK_ERROR_UNKNOWN;
   if (err == -ENODEV)
         return vk_errorf(queue, result,
      }
   if (sync) {
      err = drmSyncobjWait(pb->dev->ws_dev->fd,
                     if (err) {
      return vk_errorf(queue, VK_ERROR_UNKNOWN,
         }
      }
      VkResult
   nvk_queue_submit_simple_drm_nouveau(struct nvk_queue *queue,
                           {
               struct push_builder pb;
                        }
      static void
   push_add_queue_state(struct push_builder *pb, struct nvk_queue_state *qs)
   {
      if (qs->push.bo)
      }
      VkResult
   nvk_queue_submit_drm_nouveau(struct nvk_queue *queue,
               {
      struct nvk_device *dev = nvk_queue_device(queue);
            const bool is_vmbind = submit->buffer_bind_count > 0 ||
                  for (uint32_t i = 0; i < submit->wait_count; i++)
            if (is_vmbind) {
               for (uint32_t i = 0; i < submit->buffer_bind_count; i++)
            for (uint32_t i = 0; i < submit->image_opaque_bind_count; i++)
      } else if (submit->command_buffer_count > 0) {
      assert(submit->buffer_bind_count == 0);
                     for (unsigned i = 0; i < submit->command_buffer_count; i++) {
                     util_dynarray_foreach(&cmd->pushes, struct nvk_cmd_push, push) {
                     if (pb.req.push_count >= pb.max_push) {
                                                         for (uint32_t i = 0; i < submit->signal_count; i++)
            if (is_vmbind)
         else
      }
