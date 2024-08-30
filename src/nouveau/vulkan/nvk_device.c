   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_device.h"
      #include "nvk_cmd_buffer.h"
   #include "nvk_entrypoints.h"
   #include "nvk_instance.h"
   #include "nvk_physical_device.h"
      #include "vk_pipeline_cache.h"
   #include "vulkan/wsi/wsi_common.h"
      #include "nouveau_context.h"
      #include <fcntl.h>
   #include <xf86drm.h>
      #include "cl9097.h"
   #include "clb097.h"
   #include "clc397.h"
      static void
   nvk_slm_area_init(struct nvk_slm_area *area)
   {
      memset(area, 0, sizeof(*area));
      }
      static void
   nvk_slm_area_finish(struct nvk_slm_area *area)
   {
      simple_mtx_destroy(&area->mutex);
   if (area->bo)
      }
      struct nouveau_ws_bo *
   nvk_slm_area_get_bo_ref(struct nvk_slm_area *area,
               {
      simple_mtx_lock(&area->mutex);
   struct nouveau_ws_bo *bo = area->bo;
   if (bo)
         *bytes_per_warp_out = area->bytes_per_warp;
   *bytes_per_tpc_out = area->bytes_per_tpc;
               }
      static VkResult
   nvk_slm_area_ensure(struct nvk_device *dev,
               {
               /* TODO: Volta+doesn't use CRC */
                     /* The hardware seems to require this alignment for
   * NV9097_SET_SHADER_LOCAL_MEMORY_E_DEFAULT_SIZE_PER_WARP
   */
            uint64_t bytes_per_mp = bytes_per_warp * dev->pdev->info.max_warps_per_mp;
            /* The hardware seems to require this alignment for
   * NVA0C0_SET_SHADER_LOCAL_MEMORY_NON_THROTTLED_A_SIZE_LOWER.
   */
            /* nvk_slm_area::bytes_per_mp only ever increases so we can check this
   * outside the lock and exit early in the common case.  We only need to
   * take the lock if we're actually going to resize.
   *
   * Also, we only care about bytes_per_mp and not bytes_per_warp because
   * they are integer multiples of each other.
   */
   if (likely(bytes_per_tpc <= area->bytes_per_tpc))
                     /* The hardware seems to require this alignment for
   * NV9097_SET_SHADER_LOCAL_MEMORY_D_SIZE_LOWER.
   */
            struct nouveau_ws_bo *bo =
      nouveau_ws_bo_new(dev->ws_dev, size, 0,
      if (bo == NULL)
            struct nouveau_ws_bo *unref_bo;
   simple_mtx_lock(&area->mutex);
   if (bytes_per_tpc <= area->bytes_per_tpc) {
      /* We lost the race, throw away our BO */
   assert(area->bytes_per_warp == bytes_per_warp);
      } else {
      unref_bo = area->bo;
   area->bo = bo;
   area->bytes_per_warp = bytes_per_warp;
      }
            if (unref_bo)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateDevice(VkPhysicalDevice physicalDevice,
                     {
      VK_FROM_HANDLE(nvk_physical_device, pdev, physicalDevice);
   VkResult result = VK_ERROR_OUT_OF_HOST_MEMORY;
            dev = vk_zalloc2(&pdev->vk.instance->alloc, pAllocator,
         if (!dev)
            struct vk_device_dispatch_table dispatch_table;
   vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         vk_device_dispatch_table_from_entrypoints(&dispatch_table,
            result = vk_device_init(&dev->vk, &pdev->vk, &dispatch_table,
         if (result != VK_SUCCESS)
            drmDevicePtr drm_device = NULL;
   int ret = drmGetDeviceFromDevId(pdev->render_dev, 0, &drm_device);
   if (ret != 0) {
      result = vk_errorf(dev, VK_ERROR_INITIALIZATION_FAILED,
                     dev->ws_dev = nouveau_ws_device_new(drm_device);
   drmFreeDevice(&drm_device);
   if (dev->ws_dev == NULL) {
      result = vk_errorf(dev, VK_ERROR_INITIALIZATION_FAILED,
                     vk_device_set_drm_fd(&dev->vk, dev->ws_dev->fd);
   dev->vk.command_buffer_ops = &nvk_cmd_buffer_ops;
            ret = nouveau_ws_context_create(dev->ws_dev, &dev->ws_ctx);
   if (ret) {
      if (ret == -ENOSPC)
         else
                     result = nvk_descriptor_table_init(dev, &dev->images,
               if (result != VK_SUCCESS)
            /* Reserve the descriptor at offset 0 to be the null descriptor */
   uint32_t null_image[8] = { 0, };
   ASSERTED uint32_t null_image_index;
   result = nvk_descriptor_table_add(dev, &dev->images,
               assert(result == VK_SUCCESS);
            result = nvk_descriptor_table_init(dev, &dev->samplers,
               if (result != VK_SUCCESS)
            /* The I-cache pre-fetches and we don't really know by how much.  Over-
   * allocate shader BOs by 4K to ensure we don't run past.
   */
   result = nvk_heap_init(dev, &dev->shader_heap,
                           if (result != VK_SUCCESS)
            result = nvk_heap_init(dev, &dev->event_heap,
                     if (result != VK_SUCCESS)
                     void *zero_map;
   dev->zero_page = nouveau_ws_bo_new_mapped(dev->ws_dev, 0x1000, 0,
                     if (dev->zero_page == NULL)
            memset(zero_map, 0, 0x1000);
            if (dev->pdev->info.cls_eng3d >= FERMI_A &&
      dev->pdev->info.cls_eng3d < MAXWELL_A) {
   /* max size is 256k */
   dev->vab_memory = nouveau_ws_bo_new(dev->ws_dev, 1 << 17, 1 << 20,
               if (dev->vab_memory == NULL)
               result = nvk_queue_init(dev, &dev->queue,
         if (result != VK_SUCCESS)
            struct vk_pipeline_cache_create_info cache_info = {
         };
   dev->mem_cache = vk_pipeline_cache_create(&dev->vk, &cache_info, NULL);
   if (dev->mem_cache == NULL) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               result = nvk_device_init_meta(dev);
   if (result != VK_SUCCESS)
                           fail_mem_cache:
         fail_queue:
         fail_vab_memory:
      if (dev->vab_memory)
      fail_zero_page:
         fail_slm:
      nvk_slm_area_finish(&dev->slm);
      fail_shader_heap:
         fail_samplers:
         fail_images:
         fail_ws_ctx:
         fail_ws_dev:
         fail_init:
         fail_alloc:
      vk_free(&dev->vk.alloc, dev);
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_DestroyDevice(VkDevice _device, const VkAllocationCallbacks *pAllocator)
   {
               if (!dev)
                     vk_pipeline_cache_destroy(dev->mem_cache, NULL);
   nvk_queue_finish(dev, &dev->queue);
   if (dev->vab_memory)
         nouveau_ws_bo_destroy(dev->zero_page);
   vk_device_finish(&dev->vk);
   nvk_slm_area_finish(&dev->slm);
   nvk_heap_finish(dev, &dev->event_heap);
   nvk_heap_finish(dev, &dev->shader_heap);
   nvk_descriptor_table_finish(dev, &dev->samplers);
   nvk_descriptor_table_finish(dev, &dev->images);
   nouveau_ws_context_destroy(dev->ws_ctx);
   nouveau_ws_device_destroy(dev->ws_dev);
      }
      VkResult
   nvk_device_ensure_slm(struct nvk_device *dev,
         {
         }
