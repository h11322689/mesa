   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_descriptor_table.h"
      #include "nvk_device.h"
   #include "nvk_physical_device.h"
      #include <sys/mman.h>
      static VkResult
   nvk_descriptor_table_grow_locked(struct nvk_device *dev,
               {
      struct nouveau_ws_bo *new_bo;
   void *new_map;
                     const uint32_t new_bo_size = new_alloc * table->desc_size;
   new_bo = nouveau_ws_bo_new(dev->ws_dev, new_bo_size, 256,
                     if (new_bo == NULL) {
      return vk_errorf(dev, VK_ERROR_OUT_OF_DEVICE_MEMORY,
               new_map = nouveau_ws_bo_map(new_bo, NOUVEAU_WS_BO_WR);
   if (new_map == NULL) {
      nouveau_ws_bo_destroy(new_bo);
   return vk_errorf(dev, VK_ERROR_OUT_OF_DEVICE_MEMORY,
               if (table->bo) {
      assert(new_bo_size >= table->bo->size);
            nouveau_ws_bo_unmap(table->bo, table->map);
      }
   table->bo = new_bo;
            const size_t new_free_table_size = new_alloc * sizeof(uint32_t);
   new_free_table = vk_realloc(&dev->vk.alloc, table->free_table,
               if (new_free_table == NULL) {
      return vk_errorf(dev, VK_ERROR_OUT_OF_HOST_MEMORY,
      }
                        }
      VkResult
   nvk_descriptor_table_init(struct nvk_device *dev,
                           {
      memset(table, 0, sizeof(*table));
                     assert(util_is_power_of_two_nonzero(min_descriptor_count));
            table->desc_size = descriptor_size;
   table->alloc = 0;
   table->max_alloc = max_descriptor_count;
   table->next_desc = 0;
            result = nvk_descriptor_table_grow_locked(dev, table, min_descriptor_count);
   if (result != VK_SUCCESS) {
      nvk_descriptor_table_finish(dev, table);
                  }
      void
   nvk_descriptor_table_finish(struct nvk_device *dev,
         {
      if (table->bo != NULL) {
      nouveau_ws_bo_unmap(table->bo, table->map);
      }
   vk_free(&dev->vk.alloc, table->free_table);
      }
      #define NVK_IMAGE_DESC_INVALID
      static VkResult
   nvk_descriptor_table_alloc_locked(struct nvk_device *dev,
               {
               if (table->free_count > 0) {
      *index_out = table->free_table[--table->free_count];
               if (table->next_desc < table->alloc) {
      *index_out = table->next_desc++;
               if (table->next_desc >= table->max_alloc) {
      return vk_errorf(dev, VK_ERROR_OUT_OF_HOST_MEMORY,
               result = nvk_descriptor_table_grow_locked(dev, table, table->alloc * 2);
   if (result != VK_SUCCESS)
            assert(table->next_desc < table->alloc);
               }
      static VkResult
   nvk_descriptor_table_add_locked(struct nvk_device *dev,
                     {
      VkResult result = nvk_descriptor_table_alloc_locked(dev, table, index_out);
   if (result != VK_SUCCESS)
                     assert(desc_size == table->desc_size);
               }
         VkResult
   nvk_descriptor_table_add(struct nvk_device *dev,
                     {
      simple_mtx_lock(&table->mutex);
   VkResult result = nvk_descriptor_table_add_locked(dev, table, desc_data,
                     }
      void
   nvk_descriptor_table_remove(struct nvk_device *dev,
               {
               void *map = (char *)table->map + (index * table->desc_size);
            /* Sanity check for double-free */
   assert(table->free_count < table->alloc);
   for (uint32_t i = 0; i < table->free_count; i++)
                        }
