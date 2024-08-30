   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_queue.h"
      #include "nvk_cmd_buffer.h"
   #include "nvk_device.h"
   #include "nvk_physical_device.h"
   #include "nv_push.h"
      #include "nouveau_context.h"
      #include <xf86drm.h>
      #include "nvk_cl9097.h"
   #include "nvk_cl90b5.h"
   #include "nvk_cla0c0.h"
   #include "cla1c0.h"
   #include "nvk_clc3c0.h"
   #include "nvk_clc397.h"
      static void
   nvk_queue_state_init(struct nvk_queue_state *qs)
   {
         }
      static void
   nvk_queue_state_finish(struct nvk_device *dev,
         {
      if (qs->images.bo)
         if (qs->samplers.bo)
         if (qs->shaders.bo)
         if (qs->slm.bo)
         if (qs->push.bo) {
      nouveau_ws_bo_unmap(qs->push.bo, qs->push.bo_map);
         }
      static void
   nvk_queue_state_dump_push(struct nvk_device *dev,
         {
      struct nv_push push = {
      .start = (uint32_t *)qs->push.bo_map,
      };
      }
      VkResult
   nvk_queue_state_update(struct nvk_device *dev,
         {
      struct nouveau_ws_bo *bo;
   uint32_t alloc_count, bytes_per_warp, bytes_per_tpc;
            bo = nvk_descriptor_table_get_bo_ref(&dev->images, &alloc_count);
   if (qs->images.bo != bo || qs->images.alloc_count != alloc_count) {
      if (qs->images.bo)
         qs->images.bo = bo;
   qs->images.alloc_count = alloc_count;
      } else {
      /* No change */
   if (bo)
               bo = nvk_descriptor_table_get_bo_ref(&dev->samplers, &alloc_count);
   if (qs->samplers.bo != bo || qs->samplers.alloc_count != alloc_count) {
      if (qs->samplers.bo)
         qs->samplers.bo = bo;
   qs->samplers.alloc_count = alloc_count;
      } else {
      /* No change */
   if (bo)
               if (dev->shader_heap.contiguous) {
      bo = nvk_heap_get_contiguous_bo_ref(&dev->shader_heap);
   if (qs->shaders.bo != bo) {
      if (qs->shaders.bo)
         qs->shaders.bo = bo;
      } else {
      if (bo)
                  bo = nvk_slm_area_get_bo_ref(&dev->slm, &bytes_per_warp, &bytes_per_tpc);
   if (qs->slm.bo != bo || qs->slm.bytes_per_warp != bytes_per_warp ||
      qs->slm.bytes_per_tpc != bytes_per_tpc) {
   if (qs->slm.bo)
         qs->slm.bo = bo;
   qs->slm.bytes_per_warp = bytes_per_warp;
   qs->slm.bytes_per_tpc = bytes_per_tpc;
      } else {
      /* No change */
   if (bo)
               /* TODO: We're currently depending on kernel reference counting to protect
   * us here.  If we ever stop reference counting in the kernel, we will
   * either need to delay destruction or hold on to our extra BO references
   * and insert a GPU stall here if anything has changed before dropping our
   * old references.
            if (!dirty)
            struct nouveau_ws_bo *push_bo;
   void *push_map;
   push_bo = nouveau_ws_bo_new_mapped(dev->ws_dev, 256 * 4, 0,
                           if (push_bo == NULL)
            struct nv_push push;
   nv_push_init(&push, push_map, 256);
            if (qs->images.bo) {
      /* Compute */
   P_MTHD(p, NVA0C0, SET_TEX_HEADER_POOL_A);
   P_NVA0C0_SET_TEX_HEADER_POOL_A(p, qs->images.bo->offset >> 32);
   P_NVA0C0_SET_TEX_HEADER_POOL_B(p, qs->images.bo->offset);
   P_NVA0C0_SET_TEX_HEADER_POOL_C(p, qs->images.alloc_count - 1);
   P_IMMD(p, NVA0C0, INVALIDATE_TEXTURE_HEADER_CACHE_NO_WFI, {
                  /* 3D */
   P_MTHD(p, NV9097, SET_TEX_HEADER_POOL_A);
   P_NV9097_SET_TEX_HEADER_POOL_A(p, qs->images.bo->offset >> 32);
   P_NV9097_SET_TEX_HEADER_POOL_B(p, qs->images.bo->offset);
   P_NV9097_SET_TEX_HEADER_POOL_C(p, qs->images.alloc_count - 1);
   P_IMMD(p, NV9097, INVALIDATE_TEXTURE_HEADER_CACHE_NO_WFI, {
                     if (qs->samplers.bo) {
      /* Compute */
   P_MTHD(p, NVA0C0, SET_TEX_SAMPLER_POOL_A);
   P_NVA0C0_SET_TEX_SAMPLER_POOL_A(p, qs->samplers.bo->offset >> 32);
   P_NVA0C0_SET_TEX_SAMPLER_POOL_B(p, qs->samplers.bo->offset);
   P_NVA0C0_SET_TEX_SAMPLER_POOL_C(p, qs->samplers.alloc_count - 1);
   P_IMMD(p, NVA0C0, INVALIDATE_SAMPLER_CACHE_NO_WFI, {
                  /* 3D */
   P_MTHD(p, NV9097, SET_TEX_SAMPLER_POOL_A);
   P_NV9097_SET_TEX_SAMPLER_POOL_A(p, qs->samplers.bo->offset >> 32);
   P_NV9097_SET_TEX_SAMPLER_POOL_B(p, qs->samplers.bo->offset);
   P_NV9097_SET_TEX_SAMPLER_POOL_C(p, qs->samplers.alloc_count - 1);
   P_IMMD(p, NV9097, INVALIDATE_SAMPLER_CACHE_NO_WFI, {
                     if (qs->shaders.bo) {
      /* Compute */
   assert(dev->pdev->info.cls_compute < VOLTA_COMPUTE_A);
   P_MTHD(p, NVA0C0, SET_PROGRAM_REGION_A);
   P_NVA0C0_SET_PROGRAM_REGION_A(p, qs->shaders.bo->offset >> 32);
            /* 3D */
   assert(dev->pdev->info.cls_eng3d < VOLTA_A);
   P_MTHD(p, NV9097, SET_PROGRAM_REGION_A);
   P_NV9097_SET_PROGRAM_REGION_A(p, qs->shaders.bo->offset >> 32);
               if (qs->slm.bo) {
      const uint64_t slm_addr = qs->slm.bo->offset;
   const uint64_t slm_size = qs->slm.bo->size;
   const uint64_t slm_per_warp = qs->slm.bytes_per_warp;
   const uint64_t slm_per_tpc = qs->slm.bytes_per_tpc;
            /* Compute */
   P_MTHD(p, NVA0C0, SET_SHADER_LOCAL_MEMORY_A);
   P_NVA0C0_SET_SHADER_LOCAL_MEMORY_A(p, slm_addr >> 32);
            P_MTHD(p, NVA0C0, SET_SHADER_LOCAL_MEMORY_NON_THROTTLED_A);
   P_NVA0C0_SET_SHADER_LOCAL_MEMORY_NON_THROTTLED_A(p, slm_per_tpc >> 32);
   P_NVA0C0_SET_SHADER_LOCAL_MEMORY_NON_THROTTLED_B(p, slm_per_tpc);
            if (dev->pdev->info.cls_compute < VOLTA_COMPUTE_A) {
      P_MTHD(p, NVA0C0, SET_SHADER_LOCAL_MEMORY_THROTTLED_A);
   P_NVA0C0_SET_SHADER_LOCAL_MEMORY_THROTTLED_A(p, slm_per_tpc >> 32);
   P_NVA0C0_SET_SHADER_LOCAL_MEMORY_THROTTLED_B(p, slm_per_tpc);
               /* 3D */
   P_MTHD(p, NV9097, SET_SHADER_LOCAL_MEMORY_A);
   P_NV9097_SET_SHADER_LOCAL_MEMORY_A(p, slm_addr >> 32);
   P_NV9097_SET_SHADER_LOCAL_MEMORY_B(p, slm_addr);
   P_NV9097_SET_SHADER_LOCAL_MEMORY_C(p, slm_size >> 32);
   P_NV9097_SET_SHADER_LOCAL_MEMORY_D(p, slm_size);
               /* We set memory windows unconditionally.  Otherwise, the memory window
   * might be in a random place and cause us to fault off into nowhere.
   */
   if (dev->pdev->info.cls_compute >= VOLTA_COMPUTE_A) {
      uint64_t temp = 0xfeULL << 24;
   P_MTHD(p, NVC3C0, SET_SHADER_SHARED_MEMORY_WINDOW_A);
   P_NVC3C0_SET_SHADER_SHARED_MEMORY_WINDOW_A(p, temp >> 32);
            temp = 0xffULL << 24;
   P_MTHD(p, NVC3C0, SET_SHADER_LOCAL_MEMORY_WINDOW_A);
   P_NVC3C0_SET_SHADER_LOCAL_MEMORY_WINDOW_A(p, temp >> 32);
      } else {
      P_MTHD(p, NVA0C0, SET_SHADER_LOCAL_MEMORY_WINDOW);
            P_MTHD(p, NVA0C0, SET_SHADER_SHARED_MEMORY_WINDOW);
               /* From nvc0_screen.c:
   *
   *    "Reduce likelihood of collision with real buffers by placing the
   *    hole at the top of the 4G area. This will have to be dealt with
   *    for real eventually by blocking off that area from the VM."
   *
   * Really?!?  TODO: Fix this for realz.  Annoyingly, we only have a
   * 32-bit pointer for this in 3D rather than a full 48 like we have for
   * compute.
   */
            if (qs->push.bo) {
      nouveau_ws_bo_unmap(qs->push.bo, qs->push.bo_map);
               qs->push.bo = push_bo;
   qs->push.bo_map = push_map;
               }
      static VkResult
   nvk_queue_submit(struct vk_queue *vk_queue,
         {
      struct nvk_queue *queue = container_of(vk_queue, struct nvk_queue, vk);
   struct nvk_device *dev = nvk_queue_device(queue);
            if (vk_queue_is_lost(&queue->vk))
            result = nvk_queue_state_update(dev, &queue->state);
   if (result != VK_SUCCESS) {
      return vk_queue_set_lost(&queue->vk, "Failed to update queue base "
                                 if ((sync && result != VK_SUCCESS) ||
      (dev->ws_dev->debug_flags & NVK_DEBUG_PUSH_DUMP)) {
            for (unsigned i = 0; i < submit->command_buffer_count; i++) {
                                    if (result != VK_SUCCESS)
               }
      VkResult
   nvk_queue_init(struct nvk_device *dev, struct nvk_queue *queue,
               {
               result = vk_queue_init(&queue->vk, &dev->vk, pCreateInfo, index_in_family);
   if (result != VK_SUCCESS)
                     queue->vk.driver_submit = nvk_queue_submit;
   int err = drmSyncobjCreate(dev->ws_dev->fd, 0, &queue->syncobj_handle);
   if (err < 0) {
      result = vk_error(dev, VK_ERROR_OUT_OF_HOST_MEMORY);
                  result = nvk_queue_init_context_draw_state(queue);
   if (result != VK_SUCCESS)
                  fail_empty_push:
   fail_init:
                  }
      void
   nvk_queue_finish(struct nvk_device *dev, struct nvk_queue *queue)
   {
      nvk_queue_state_finish(dev, &queue->state);
   ASSERTED int err = drmSyncobjDestroy(dev->ws_dev->fd, queue->syncobj_handle);
   assert(err == 0);
      }
      VkResult
   nvk_queue_submit_simple(struct nvk_queue *queue,
                     {
      struct nvk_device *dev = nvk_queue_device(queue);
   struct nouveau_ws_bo *push_bo;
            if (vk_queue_is_lost(&queue->vk))
            void *push_map;
   push_bo = nouveau_ws_bo_new_mapped(dev->ws_dev, dw_count * 4, 0,
                           if (push_bo == NULL)
                     result = nvk_queue_submit_simple_drm_nouveau(queue, dw_count, push_bo,
            const bool debug_sync = dev->ws_dev->debug_flags & NVK_DEBUG_PUSH_SYNC;
   if ((debug_sync && result != VK_SUCCESS) ||
      (dev->ws_dev->debug_flags & NVK_DEBUG_PUSH_DUMP)) {
   struct nv_push push = {
      .start = (uint32_t *)dw,
      };
               nouveau_ws_bo_unmap(push_bo, push_map);
            if (result != VK_SUCCESS)
               }
