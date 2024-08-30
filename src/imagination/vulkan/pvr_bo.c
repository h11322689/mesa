   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * based in part on tu driver which is:
   * Copyright © 2022 Google LLC
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <assert.h>
   #include <limits.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <vulkan/vulkan.h>
      #if defined(HAVE_VALGRIND)
   #   include <valgrind.h>
   #   include <memcheck.h>
   #endif
      #include "hwdef/rogue_hw_utils.h"
   #include "pvr_bo.h"
   #include "pvr_debug.h"
   #include "pvr_dump.h"
   #include "pvr_private.h"
   #include "pvr_types.h"
   #include "pvr_util.h"
   #include "pvr_winsys.h"
   #include "util/macros.h"
   #include "util/rb_tree.h"
   #include "util/simple_mtx.h"
   #include "util/u_debug.h"
   #include "vk_alloc.h"
   #include "vk_log.h"
      struct pvr_bo_store {
      struct rb_tree tree;
   simple_mtx_t mutex;
      };
      struct pvr_bo_store_entry {
      struct rb_node node;
      };
      #define entry_from_node(node_) \
         #define entry_from_bo(bo_) container_of(bo_, struct pvr_bo_store_entry, bo)
      static inline int pvr_dev_addr_cmp(const pvr_dev_addr_t a,
         {
      const uint64_t addr_a = a.addr;
            if (addr_a < addr_b)
         else if (addr_a > addr_b)
         else
      }
      /* Borrowed from pandecode. Using this comparator allows us to lookup intervals
   * in the RB-tree without storing extra information.
   */
   static inline int pvr_bo_store_entry_cmp_key(const struct rb_node *node,
         {
      const struct pvr_winsys_vma *const vma = entry_from_node(node)->bo.vma;
            if (addr.addr >= vma->dev_addr.addr &&
      addr.addr < (vma->dev_addr.addr + vma->size)) {
                  }
      static inline int pvr_bo_store_entry_cmp(const struct rb_node *const a,
         {
      return pvr_dev_addr_cmp(entry_from_node(a)->bo.vma->dev_addr,
      }
      VkResult pvr_bo_store_create(struct pvr_device *device)
   {
               if (!PVR_IS_DEBUG_SET(TRACK_BOS)) {
      device->bo_store = NULL;
               store = vk_alloc(&device->vk.alloc,
                     if (!store)
            rb_tree_init(&store->tree);
   store->size = 0;
                        }
      void pvr_bo_store_destroy(struct pvr_device *device)
   {
               if (likely(!store))
            if (unlikely(!rb_tree_is_empty(&store->tree))) {
      debug_warning("Non-empty BO store destroyed; dump follows");
                                    }
      static void pvr_bo_store_insert(struct pvr_bo_store *const store,
         {
      if (likely(!store))
            simple_mtx_lock(&store->mutex);
   rb_tree_insert(&store->tree,
               store->size++;
      }
      static void pvr_bo_store_remove(struct pvr_bo_store *const store,
         {
      if (likely(!store))
            simple_mtx_lock(&store->mutex);
   rb_tree_remove(&store->tree, &entry_from_bo(bo)->node);
   store->size--;
      }
      struct pvr_bo *pvr_bo_store_lookup(struct pvr_device *const device,
         {
      struct pvr_bo_store *const store = device->bo_store;
            if (unlikely(!store))
            simple_mtx_lock(&store->mutex);
   node = rb_tree_search(&store->tree, &addr, pvr_bo_store_entry_cmp_key);
            if (!node)
               }
      static void pvr_bo_dump_line(struct pvr_dump_ctx *const ctx,
                     {
      static const char *const pretty_sizes[64 + 1] = {
      "",        "1 B",     "2 B",     "4 B",     "8 B",     "16 B",
   "32 B",    "64 B",    "128 B",   "256 B",   "512 B",   "1 KiB",
   "2 KiB",   "4 KiB",   "8 KiB",   "16 KiB",  "32 KiB",  "64 KiB",
   "128 KiB", "256 KiB", "512 KiB", "1 MiB",   "2 MiB",   "4 MiB",
   "8 MiB",   "16 MiB",  "32 MiB",  "64 MiB",  "128 MiB", "256 MiB",
   "512 MiB", "1 GiB",   "2 GiB",   "4 GiB",   "8 GiB",   "16 GiB",
   "32 GiB",  "64 GiB",  "128 GiB", "256 GiB", "512 GiB", "1 TiB",
   "2 TiB",   "4 TiB",   "8 TiB",   "16 TiB",  "32 TiB",  "64 TiB",
   "128 TiB", "256 TiB", "512 TiB", "1 PiB",   "2 PiB",   "4 PiB",
   "8 PiB",   "16 PiB",  "32 PiB",  "64 PiB",  "128 PiB", "256 PiB",
               const uint64_t size = bo->vma->size;
   const uint32_t size_log2 =
            pvr_dump_println(ctx,
                  "[%0*" PRIu32 "] " PVR_DEV_ADDR_FMT " -> %*p "
   "(%s%s0x%" PRIx64 " bytes)",
   nr_bos_log10,
   index,
   bo->vma->dev_addr.addr,
   (int)sizeof(void *) * 2 + 2, /* nr hex digits + 0x prefix */
   bo->bo->map,
   }
      bool pvr_bo_store_dump(struct pvr_device *const device)
   {
      struct pvr_bo_store *const store = device->bo_store;
   const uint32_t nr_bos = store->size;
   const uint32_t nr_bos_log10 = u32_dec_digits(nr_bos);
   struct pvr_dump_ctx ctx;
            if (unlikely(!store)) {
      debug_warning("Requested BO store dump, but no BO store is present.");
                                 pvr_dump_indent(&ctx);
   rb_tree_foreach_safe (struct pvr_bo_store_entry, entry, &store->tree, node) {
         }
               }
      void pvr_bo_list_dump(struct pvr_dump_ctx *const ctx,
               {
      const uint32_t real_nr_bos = nr_bos ? nr_bos : list_length(bo_list);
   const uint32_t nr_bos_log10 = u32_dec_digits(real_nr_bos);
            list_for_each_entry (struct pvr_bo, bo, bo_list, link) {
            }
      static uint32_t pvr_bo_alloc_to_winsys_flags(uint64_t flags)
   {
               if (flags & (PVR_BO_ALLOC_FLAG_CPU_ACCESS | PVR_BO_ALLOC_FLAG_CPU_MAPPED))
            if (flags & PVR_BO_ALLOC_FLAG_GPU_UNCACHED)
            if (flags & PVR_BO_ALLOC_FLAG_PM_FW_PROTECT)
               }
      static inline struct pvr_bo *
   pvr_bo_alloc_bo(const struct pvr_device *const device)
   {
      size_t size;
            if (unlikely(device->bo_store))
         else
            ptr =
         if (unlikely(!ptr))
            if (unlikely(device->bo_store))
         else
      }
      static inline void pvr_bo_free_bo(const struct pvr_device *const device,
         {
               if (unlikely(device->bo_store))
         else
               }
      /**
   * \brief Helper interface to allocate a GPU buffer and map it to both host and
   * device virtual memory. Host mapping is conditional and is controlled by
   * flags.
   *
   * \param[in] device      Logical device pointer.
   * \param[in] heap        Heap to allocate device virtual address from.
   * \param[in] size        Size of buffer to allocate.
   * \param[in] alignment   Required alignment of the allocation. Must be a power
   *                        of two.
   * \param[in] flags       Controls allocation, CPU and GPU mapping behavior
   *                        using PVR_BO_ALLOC_FLAG_*.
   * \param[out] pvr_bo_out On success output buffer is returned in this pointer.
   * \return VK_SUCCESS on success, or error code otherwise.
   *
   * \sa #pvr_bo_free()
   */
   VkResult pvr_bo_alloc(struct pvr_device *device,
                        struct pvr_winsys_heap *heap,
      {
      struct pvr_bo *pvr_bo;
            pvr_bo = pvr_bo_alloc_bo(device);
   if (!pvr_bo) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
                        result = device->ws->ops->buffer_create(device->ws,
                                 if (result != VK_SUCCESS)
            if (flags & PVR_BO_ALLOC_FLAG_CPU_MAPPED) {
      result = device->ws->ops->buffer_map(pvr_bo->bo);
   if (result != VK_SUCCESS)
                        result = device->ws->ops->heap_alloc(heap, size, alignment, &pvr_bo->vma);
   if (result != VK_SUCCESS)
            result = device->ws->ops->vma_map(pvr_bo->vma, pvr_bo->bo, 0, size, NULL);
   if (result != VK_SUCCESS)
            pvr_bo_store_insert(device->bo_store, pvr_bo);
                  err_heap_free:
            err_buffer_unmap:
      if (flags & PVR_BO_ALLOC_FLAG_CPU_MAPPED)
         err_buffer_destroy:
            err_free_bo:
            err_out:
         }
      /**
   * \brief Interface to map the buffer into host virtual address space.
   *
   * Buffer should have been created with the #PVR_BO_ALLOC_FLAG_CPU_ACCESS
   * flag. It should also not already be mapped or it should have been unmapped
   * using #pvr_bo_cpu_unmap() before mapping again.
   *
   * \param[in] device Logical device pointer.
   * \param[in] pvr_bo Buffer to map.
   * \return Valid host virtual address on success, or NULL otherwise.
   *
   * \sa #pvr_bo_alloc(), #PVR_BO_ALLOC_FLAG_CPU_MAPPED
   */
   VkResult pvr_bo_cpu_map(struct pvr_device *device, struct pvr_bo *pvr_bo)
   {
                  }
      /**
   * \brief Interface to unmap the buffer from host virtual address space.
   *
   * Buffer should have a valid mapping, created either using #pvr_bo_cpu_map() or
   * by passing #PVR_BO_ALLOC_FLAG_CPU_MAPPED flag to #pvr_bo_alloc() at
   * allocation time.
   *
   * Buffer can be remapped using #pvr_bo_cpu_map().
   *
   * \param[in] device Logical device pointer.
   * \param[in] pvr_bo Buffer to unmap.
   */
   void pvr_bo_cpu_unmap(struct pvr_device *device, struct pvr_bo *pvr_bo)
   {
                     #if defined(HAVE_VALGRIND)
      if (!bo->vbits)
      bo->vbits = vk_alloc(&device->vk.alloc,
                  if (bo->vbits) {
      unsigned ret = VALGRIND_GET_VBITS(bo->map, bo->vbits, bo->size);
   if (ret != 0 && ret != 1)
      } else {
            #endif /* defined(HAVE_VALGRIND) */
            }
      /**
   * \brief Interface to free the buffer object.
   *
   * \param[in] device Logical device pointer.
   * \param[in] pvr_bo Buffer to free.
   *
   * \sa #pvr_bo_alloc()
   */
   void pvr_bo_free(struct pvr_device *device, struct pvr_bo *pvr_bo)
   {
      if (!pvr_bo)
            if (!p_atomic_dec_zero(&pvr_bo->ref_count))
         #if defined(HAVE_VALGRIND)
         #endif /* defined(HAVE_VALGRIND) */
                  device->ws->ops->vma_unmap(pvr_bo->vma);
            if (pvr_bo->bo->map)
                        }
      /**
   * \brief Interface to initialize a pvr_suballocator.
   *
   * \param[in] allocator    Sub-allocator to initialize.
   * \param[in] heap         Heap to sub-allocate device virtual address from.
   * \param[in] device       Logical device pointer.
   * \param[in] default_size Minimum size used for pvr bo(s).
   *
   * \sa #pvr_bo_suballocator_fini()
   */
   void pvr_bo_suballocator_init(struct pvr_suballocator *allocator,
                     {
      *allocator = (struct pvr_suballocator){
      .device = device,
   .default_size = default_size,
                  }
      /**
   * \brief Interface to destroy a pvr_suballocator.
   *
   * \param[in] allocator Sub-allocator to clean-up.
   *
   * \sa #pvr_bo_suballocator_init()
   */
   void pvr_bo_suballocator_fini(struct pvr_suballocator *allocator)
   {
      pvr_bo_free(allocator->device, allocator->bo);
               }
      static inline struct pvr_bo *pvr_bo_get_ref(struct pvr_bo *bo)
   {
                  }
      /**
   * \brief Interface to sub-allocate buffer objects.
   *
   * \param[in]  allocator       Sub-allocator used to make a sub-allocation.
   * \param[in]  size            Size of buffer to sub-alllocate.
   * \param[in]  align           Required alignment of the allocation. Must be
   *                             a power of two.
   * \param[in]  zero_on_alloc   Require memory for the sub-allocation to be 0.
   * \param[out] suballoc_bo_out On success points to the sub-allocated buffer
   *                             object.
   * \return VK_SUCCESS on success, or error code otherwise.
   *
   * \sa # pvr_bo_suballoc_free()
   */
   VkResult pvr_bo_suballoc(struct pvr_suballocator *allocator,
                           {
      const struct pvr_device_info *dev_info =
         const uint32_t cache_line_size = rogue_get_slc_cache_line_size(dev_info);
   struct pvr_suballoc_bo *suballoc_bo;
   uint32_t alloc_size, aligned_size;
            suballoc_bo = vk_alloc(&allocator->device->vk.alloc,
                     if (!suballoc_bo)
            /* This cache line value is used for all type of allocations (i.e. USC, PDS,
   * transfer and general), so always align them to at least the size of the
   * cache line.
   */
   align = MAX2(align, cache_line_size);
   assert(util_is_power_of_two_nonzero(align));
                     if (allocator->bo) {
               if (aligned_offset + aligned_size <= allocator->bo->bo->size) {
      suballoc_bo->allocator = allocator;
   suballoc_bo->bo = pvr_bo_get_ref(allocator->bo);
   suballoc_bo->dev_addr =
                                                                  } else {
      pvr_bo_free(allocator->device, allocator->bo);
                           if (allocator->bo_cached) {
               if (alloc_size <= bo_cached->size)
         else
                        if (!allocator->bo) {
      result = pvr_bo_alloc(allocator->device,
                        allocator->heap,
      if (result != VK_SUCCESS) {
      vk_free(&allocator->device->vk.alloc, suballoc_bo);
   simple_mtx_unlock(&allocator->mtx);
                  suballoc_bo->allocator = allocator;
   suballoc_bo->bo = pvr_bo_get_ref(allocator->bo);
   suballoc_bo->dev_addr = allocator->bo->vma->dev_addr;
   suballoc_bo->offset = 0;
                     if (zero_on_alloc)
            *suballoc_bo_out = suballoc_bo;
               }
      /**
   * \brief Interface to free a sub-allocated buffer object.
   *
   * \param[in] suballoc_bo Sub-allocated buffer object to free.
   *
   * \sa #pvr_bo_suballoc()
   */
   void pvr_bo_suballoc_free(struct pvr_suballoc_bo *suballoc_bo)
   {
      if (!suballoc_bo)
                     if (p_atomic_read(&suballoc_bo->bo->ref_count) == 1 &&
      !suballoc_bo->allocator->bo_cached) {
      } else {
                              }
      /**
   * \brief Interface to retrieve sub-allocated memory offset from the host
   * virtual address space.
   *
   * \param[in] suballoc_bo Sub-allocated buffer object pointer.
   *
   * \return Valid host virtual address on success.
   *
   * \sa #pvr_bo_suballoc()
   */
   void *pvr_bo_suballoc_get_map_addr(const struct pvr_suballoc_bo *suballoc_bo)
   {
               assert((uint8_t *)pvr_bo->bo->map + suballoc_bo->offset <
               }
      #if defined(HAVE_VALGRIND)
   VkResult pvr_bo_cpu_map_unchanged(struct pvr_device *device,
         {
      VkResult result = pvr_bo_cpu_map(device, pvr_bo);
   if (result == VK_SUCCESS) {
      unsigned ret = VALGRIND_SET_VBITS(pvr_bo->bo->map,
               if (ret != 0 && ret != 1)
                  }
   #endif /* defined(HAVE_VALGRIND) */
