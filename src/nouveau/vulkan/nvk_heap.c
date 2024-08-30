   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_heap.h"
      #include "nvk_device.h"
   #include "nvk_physical_device.h"
   #include "nvk_queue.h"
      #include "util/macros.h"
      #include "nv_push.h"
   #include "nvk_cl90b5.h"
      VkResult
   nvk_heap_init(struct nvk_device *dev, struct nvk_heap *heap,
               enum nouveau_ws_bo_flags bo_flags,
   {
               heap->bo_flags = bo_flags;
   if (map_flags)
         heap->map_flags = map_flags;
   heap->overalloc = overalloc;
            simple_mtx_init(&heap->mutex, mtx_plain);
            heap->total_size = 0;
               }
      void
   nvk_heap_finish(struct nvk_device *dev, struct nvk_heap *heap)
   {
      for (uint32_t bo_idx = 0; bo_idx < heap->bo_count; bo_idx++) {
      nouveau_ws_bo_unmap(heap->bos[bo_idx].bo, heap->bos[bo_idx].map);
               util_vma_heap_finish(&heap->heap);
      }
      static uint64_t
   encode_vma(uint32_t bo_idx, uint64_t bo_offset)
   {
      assert(bo_idx < UINT16_MAX - 1);
   assert(bo_offset < (1ull << 48));
      }
      static uint32_t
   vma_bo_idx(uint64_t offset)
   {
      offset = offset >> 48;
   assert(offset > 0);
      }
      static uint64_t
   vma_bo_offset(uint64_t offset)
   {
         }
      static VkResult
   nvk_heap_grow_locked(struct nvk_device *dev, struct nvk_heap *heap)
   {
               if (heap->contiguous) {
      if (heap->total_size >= NVK_HEAP_MAX_SIZE) {
      return vk_errorf(dev, VK_ERROR_OUT_OF_DEVICE_MEMORY,
               const uint64_t new_bo_size =
            void *new_bo_map;
   struct nouveau_ws_bo *new_bo =
      nouveau_ws_bo_new_mapped(dev->ws_dev,
                  if (new_bo == NULL) {
      return vk_errorf(dev, VK_ERROR_OUT_OF_DEVICE_MEMORY,
               if (heap->bo_count > 0) {
                     assert(util_is_power_of_two_nonzero(heap->total_size));
   assert(heap->total_size >= NVK_HEAP_MIN_SIZE);
                  unsigned line_bytes = MIN2(heap->total_size, 1 << 17);
                  uint32_t push_dw[12];
   struct nv_push push;
                  P_MTHD(p, NV90B5, OFFSET_IN_UPPER);
   P_NV90B5_OFFSET_IN_UPPER(p, old_bo->offset >> 32);
   P_NV90B5_OFFSET_IN_LOWER(p, old_bo->offset & 0xffffffff);
   P_NV90B5_OFFSET_OUT_UPPER(p, new_bo->offset >> 32);
   P_NV90B5_OFFSET_OUT_LOWER(p, new_bo->offset & 0xffffffff);
   P_NV90B5_PITCH_IN(p, line_bytes);
   P_NV90B5_PITCH_OUT(p, line_bytes);
                  P_IMMD(p, NV90B5, LAUNCH_DMA, {
      .data_transfer_type = DATA_TRANSFER_TYPE_NON_PIPELINED,
   .multi_line_enable = MULTI_LINE_ENABLE_TRUE,
   .flush_enable = FLUSH_ENABLE_TRUE,
   .src_memory_layout = SRC_MEMORY_LAYOUT_PITCH,
               struct nouveau_ws_bo *push_bos[] = { new_bo, old_bo, };
   result = nvk_queue_submit_simple(&dev->queue,
               if (result != VK_SUCCESS) {
      nouveau_ws_bo_unmap(new_bo, new_bo_map);
   nouveau_ws_bo_destroy(new_bo);
               nouveau_ws_bo_unmap(heap->bos[0].bo, heap->bos[0].map);
               uint64_t vma = encode_vma(0, heap->total_size);
            heap->total_size = new_bo_size;
   heap->bo_count = 1;
   heap->bos[0].bo = new_bo;
               } else {
      if (heap->bo_count >= NVK_HEAP_MAX_BO_COUNT) {
      return vk_errorf(dev, VK_ERROR_OUT_OF_DEVICE_MEMORY,
               /* First two BOs are MIN_SIZE, double after that */
   const uint64_t new_bo_size =
            heap->bos[heap->bo_count].bo =
      nouveau_ws_bo_new_mapped(dev->ws_dev,
                  if (heap->bos[heap->bo_count].bo == NULL) {
      return vk_errorf(dev, VK_ERROR_OUT_OF_DEVICE_MEMORY,
               uint64_t vma = encode_vma(heap->bo_count, 0);
            heap->total_size += new_bo_size;
                  }
      static VkResult
   nvk_heap_alloc_locked(struct nvk_device *dev, struct nvk_heap *heap,
               {
      while (1) {
      uint64_t vma = util_vma_heap_alloc(&heap->heap, size, alignment);
   if (vma != 0) {
                     assert(bo_idx < heap->bo_count);
   assert(heap->bos[bo_idx].bo != NULL);
                  if (heap->contiguous) {
      assert(bo_idx == 0);
      } else {
                                    VkResult result = nvk_heap_grow_locked(dev, heap);
   if (result != VK_SUCCESS)
         }
      static void
   nvk_heap_free_locked(struct nvk_device *dev, struct nvk_heap *heap,
         {
               for (uint32_t bo_idx = 0; bo_idx < heap->bo_count; bo_idx++) {
      if (addr < heap->bos[bo_idx].bo->offset)
            uint64_t bo_offset = addr - heap->bos[bo_idx].bo->offset;
   if (bo_offset >= heap->bos[bo_idx].bo->size)
            assert(bo_offset + size <= heap->bos[bo_idx].bo->size);
                           }
      VkResult
   nvk_heap_alloc(struct nvk_device *dev, struct nvk_heap *heap,
               {
      /* We can't return maps from contiguous heaps because the the map may go
   * away at any time when the lock isn't taken and we don't want to trust
   * the caller with racy maps.
   */
            simple_mtx_lock(&heap->mutex);
   VkResult result = nvk_heap_alloc_locked(dev, heap, size, alignment,
                     }
      VkResult
   nvk_heap_upload(struct nvk_device *dev, struct nvk_heap *heap,
               {
               void *map;
   VkResult result = nvk_heap_alloc_locked(dev, heap, size, alignment,
         if (result == VK_SUCCESS)
                     }
      void
   nvk_heap_free(struct nvk_device *dev, struct nvk_heap *heap,
         {
      simple_mtx_lock(&heap->mutex);
   nvk_heap_free_locked(dev, heap, addr, size);
      }
