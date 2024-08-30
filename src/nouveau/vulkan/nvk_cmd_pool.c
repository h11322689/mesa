   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_cmd_pool.h"
      #include "nvk_device.h"
   #include "nvk_entrypoints.h"
   #include "nvk_physical_device.h"
      static VkResult
   nvk_cmd_bo_create(struct nvk_cmd_pool *pool, bool force_gart, struct nvk_cmd_bo **bo_out)
   {
      struct nvk_device *dev = nvk_cmd_pool_device(pool);
            bo = vk_zalloc(&pool->vk.alloc, sizeof(*bo), 8,
         if (bo == NULL)
            uint32_t flags = NOUVEAU_WS_BO_GART | NOUVEAU_WS_BO_MAP | NOUVEAU_WS_BO_NO_SHARE;
   if (force_gart)
         bo->bo = nouveau_ws_bo_new_mapped(dev->ws_dev, NVK_CMD_BO_SIZE, 0,
         if (bo->bo == NULL) {
      vk_free(&pool->vk.alloc, bo);
               *bo_out = bo;
      }
      static void
   nvk_cmd_bo_destroy(struct nvk_cmd_pool *pool, struct nvk_cmd_bo *bo)
   {
      nouveau_ws_bo_unmap(bo->bo, bo->map);
   nouveau_ws_bo_destroy(bo->bo);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateCommandPool(VkDevice _device,
                     {
      VK_FROM_HANDLE(nvk_device, device, _device);
            pool = vk_alloc2(&device->vk.alloc, pAllocator, sizeof(*pool), 8,
         if (pool == NULL)
            VkResult result = vk_command_pool_init(&device->vk, &pool->vk,
         if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, pool);
               list_inithead(&pool->free_bos);
                        }
      static void
   nvk_cmd_pool_destroy_bos(struct nvk_cmd_pool *pool)
   {
      list_for_each_entry_safe(struct nvk_cmd_bo, bo, &pool->free_bos, link)
                     list_for_each_entry_safe(struct nvk_cmd_bo, bo, &pool->free_gart_bos, link)
               }
      VkResult
   nvk_cmd_pool_alloc_bo(struct nvk_cmd_pool *pool, bool force_gart, struct nvk_cmd_bo **bo_out)
   {
      struct nvk_cmd_bo *bo = NULL;
   if (force_gart) {
      if (!list_is_empty(&pool->free_gart_bos))
      } else {
      if (!list_is_empty(&pool->free_bos))
      }
   if (bo) {
      list_del(&bo->link);
   *bo_out = bo;
                  }
      void
   nvk_cmd_pool_free_bo_list(struct nvk_cmd_pool *pool, struct list_head *bos)
   {
      list_splicetail(bos, &pool->free_bos);
      }
      void
   nvk_cmd_pool_free_gart_bo_list(struct nvk_cmd_pool *pool, struct list_head *bos)
   {
      list_splicetail(bos, &pool->free_gart_bos);
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_DestroyCommandPool(VkDevice _device,
               {
      VK_FROM_HANDLE(nvk_device, device, _device);
            if (!pool)
            vk_command_pool_finish(&pool->vk);
   nvk_cmd_pool_destroy_bos(pool);
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_TrimCommandPool(VkDevice device,
               {
               vk_command_pool_trim(&pool->vk, flags);
      }
