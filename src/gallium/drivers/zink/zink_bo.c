   /*
   * Copyright © 2011 Marek Olšák <maraeo@gmail.com>
   * Copyright © 2015 Advanced Micro Devices, Inc.
   * Copyright © 2021 Valve Corporation
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining
   * a copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
   * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   * 
   * Authors:
   *    Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
   */
      #include "zink_context.h"
   #include "zink_bo.h"
   #include "zink_resource.h"
   #include "zink_screen.h"
   #include "util/u_hash_table.h"
      #if !defined(__APPLE__) && !defined(_WIN32)
   #define ZINK_USE_DMABUF
   #include <xf86drm.h>
   #endif
      struct zink_bo;
      struct zink_sparse_backing_chunk {
         };
         /*
   * Sub-allocation information for a real buffer used as backing memory of a
   * sparse buffer.
   */
   struct zink_sparse_backing {
                        /* Sorted list of free chunks. */
   struct zink_sparse_backing_chunk *chunks;
   uint32_t max_chunks;
      };
      struct zink_sparse_commitment {
      struct zink_sparse_backing *backing;
      };
      struct zink_slab {
      struct pb_slab base;
   unsigned entry_size;
   struct zink_bo *buffer;
      };
         ALWAYS_INLINE static struct zink_slab *
   zink_slab(struct pb_slab *pslab)
   {
         }
      static struct pb_slabs *
   get_slabs(struct zink_screen *screen, uint64_t size, enum zink_alloc_flag flags)
   {
      //struct pb_slabs *bo_slabs = ((flags & RADEON_FLAG_ENCRYPTED) && screen->info.has_tmz_support) ?
            struct pb_slabs *bo_slabs = screen->pb.bo_slabs;
   /* Find the correct slab allocator for the given size. */
   for (unsigned i = 0; i < NUM_SLAB_ALLOCATORS; i++) {
               if (size <= 1ULL << (slabs->min_order + slabs->num_orders - 1))
               assert(0);
      }
      /* Return the power of two size of a slab entry matching the input size. */
   static unsigned
   get_slab_pot_entry_size(struct zink_screen *screen, unsigned size)
   {
      unsigned entry_size = util_next_power_of_two(size);
               }
      /* Return the slab entry alignment. */
   static unsigned get_slab_entry_alignment(struct zink_screen *screen, unsigned size)
   {
               if (size <= entry_size * 3 / 4)
               }
      static void
   bo_destroy(struct zink_screen *screen, struct pb_buffer *pbuf)
   {
            #ifdef ZINK_USE_DMABUF
      if (bo->mem && !bo->u.real.use_reusable_pool) {
      simple_mtx_lock(&bo->u.real.export_lock);
   list_for_each_entry_safe(struct bo_export, export, &bo->u.real.exports, link) {
      struct drm_gem_close args = { .handle = export->gem_handle };
   drmIoctl(export->drm_fd, DRM_IOCTL_GEM_CLOSE, &args);
   list_del(&export->link);
      }
   simple_mtx_unlock(&bo->u.real.export_lock);
         #endif
         if (!bo->u.real.is_user_ptr && bo->u.real.cpu_ptr) {
      bo->u.real.map_count = 1;
   bo->u.real.cpu_ptr = NULL;
                        simple_mtx_destroy(&bo->lock);
      }
      static bool
   bo_can_reclaim(struct zink_screen *screen, struct pb_buffer *pbuf)
   {
                  }
      static bool
   bo_can_reclaim_slab(void *priv, struct pb_slab_entry *entry)
   {
                  }
      static void
   bo_slab_free(struct zink_screen *screen, struct pb_slab *pslab)
   {
      struct zink_slab *slab = zink_slab(pslab);
            assert(slab->base.num_entries * slab->entry_size <= slab_size);
   FREE(slab->entries);
   zink_bo_unref(screen, slab->buffer);
      }
      static void
   bo_slab_destroy(struct zink_screen *screen, struct pb_buffer *pbuf)
   {
                        //if (bo->base.usage & RADEON_FLAG_ENCRYPTED)
         //else
      }
      static bool
   clean_up_buffer_managers(struct zink_screen *screen)
   {
      unsigned num_reclaims = 0;
   for (unsigned i = 0; i < NUM_SLAB_ALLOCATORS; i++) {
      num_reclaims += pb_slabs_reclaim(&screen->pb.bo_slabs[i]);
   //if (screen->info.has_tmz_support)
               num_reclaims += pb_cache_release_all_buffers(&screen->pb.bo_cache);
      }
      static unsigned
   get_optimal_alignment(struct zink_screen *screen, uint64_t size, unsigned alignment)
   {
      /* Increase the alignment for faster address translation and better memory
   * access pattern.
   */
   if (size >= 4096) {
         } else if (size) {
                  }
      }
      static void
   bo_destroy_or_cache(struct zink_screen *screen, struct pb_buffer *pbuf)
   {
               assert(bo->mem); /* slab buffers have a separate vtbl */
   bo->reads.u = NULL;
            if (bo->u.real.use_reusable_pool)
         else
      }
      static const struct pb_vtbl bo_vtbl = {
      /* Cast to void* because one of the function parameters is a struct pointer instead of void*. */
   (void*)bo_destroy_or_cache
      };
      static struct zink_bo *
   bo_create_internal(struct zink_screen *screen,
                     uint64_t size,
   unsigned alignment,
   enum zink_heap heap,
   {
      struct zink_bo *bo = NULL;
                     VkMemoryAllocateFlagsInfo ai;
   ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
   ai.pNext = pNext;
   ai.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
   ai.deviceMask = 0;
   if (screen->info.have_KHR_buffer_device_address)
            VkMemoryPriorityAllocateInfoEXT prio = {
      VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT,
   pNext,
      };
   if (screen->info.have_EXT_memory_priority)
            VkMemoryAllocateInfo mai;
   mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   mai.pNext = pNext;
   mai.allocationSize = size;
   mai.memoryTypeIndex = mem_type_idx;
   if (screen->info.mem_props.memoryTypes[mai.memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      alignment = MAX2(alignment, screen->info.props.limits.minMemoryMapAlignment);
      }
   unsigned vk_heap_idx = screen->info.mem_props.memoryTypes[mem_type_idx].heapIndex;
   if (mai.allocationSize > screen->info.mem_props.memoryHeaps[vk_heap_idx].size) {
      mesa_loge("zink: can't allocate %"PRIu64" bytes from heap that's only %"PRIu64" bytes!\n", mai.allocationSize, screen->info.mem_props.memoryHeaps[vk_heap_idx].size);
               /* all non-suballocated bo can cache */
            if (!bo)
         if (!bo) {
                  VkResult ret = VKSCR(AllocateMemory)(screen->dev, &mai, NULL, &bo->mem);
   if (!zink_screen_handle_vkresult(screen, ret)) {
      mesa_loge("zink: couldn't allocate memory: heap=%u size=%" PRIu64, heap, size);
   if (zink_debug & ZINK_DEBUG_MEM) {
      zink_debug_mem_print_stats(screen);
   /* abort with mem debug to allow debugging */
      }
               if (init_pb_cache) {
      bo->u.real.use_reusable_pool = true;
         #ifdef ZINK_USE_DMABUF
         list_inithead(&bo->u.real.exports);
   #endif
                  simple_mtx_init(&bo->lock, mtx_plain);
   pipe_reference_init(&bo->base.reference, 1);
   bo->base.alignment_log2 = util_logbase2(alignment);
   bo->base.size = mai.allocationSize;
   bo->base.vtbl = &bo_vtbl;
   bo->base.placement = mem_type_idx;
   bo->base.usage = flags;
                  fail:
      bo_destroy(screen, (void*)bo);
      }
      /*
   * Attempt to allocate the given number of backing pages. Fewer pages may be
   * allocated (depending on the fragmentation of existing backing buffers),
   * which will be reflected by a change to *pnum_pages.
   */
   static struct zink_sparse_backing *
   sparse_backing_alloc(struct zink_screen *screen, struct zink_bo *bo,
         {
      struct zink_sparse_backing *best_backing;
   unsigned best_idx;
            best_backing = NULL;
   best_idx = 0;
            /* This is a very simple and inefficient best-fit algorithm. */
   list_for_each_entry(struct zink_sparse_backing, backing, &bo->u.sparse.backing, list) {
      for (unsigned idx = 0; idx < backing->num_chunks; ++idx) {
      uint32_t cur_num_pages = backing->chunks[idx].end - backing->chunks[idx].begin;
   if ((best_num_pages < *pnum_pages && cur_num_pages > best_num_pages) ||
      (best_num_pages > *pnum_pages && cur_num_pages < best_num_pages)) {
   best_backing = backing;
   best_idx = idx;
                     /* Allocate a new backing buffer if necessary. */
   if (!best_backing) {
      struct pb_buffer *buf;
   uint64_t size;
            best_backing = CALLOC_STRUCT(zink_sparse_backing);
   if (!best_backing)
            best_backing->max_chunks = 4;
   best_backing->chunks = CALLOC(best_backing->max_chunks,
         if (!best_backing->chunks) {
      FREE(best_backing);
                        size = MIN3(bo->base.size / 16,
                        buf = zink_bo_create(screen, size, ZINK_SPARSE_BUFFER_PAGE_SIZE,
         if (!buf) {
      FREE(best_backing->chunks);
   FREE(best_backing);
               /* We might have gotten a bigger buffer than requested via caching. */
            best_backing->bo = zink_bo(buf);
   best_backing->num_chunks = 1;
   best_backing->chunks[0].begin = 0;
            list_add(&best_backing->list, &bo->u.sparse.backing);
            best_idx = 0;
               *pnum_pages = MIN2(*pnum_pages, best_num_pages);
   *pstart_page = best_backing->chunks[best_idx].begin;
            if (best_backing->chunks[best_idx].begin >= best_backing->chunks[best_idx].end) {
      memmove(&best_backing->chunks[best_idx], &best_backing->chunks[best_idx + 1],
                        }
      static void
   sparse_free_backing_buffer(struct zink_screen *screen, struct zink_bo *bo,
         {
               list_del(&backing->list);
   zink_bo_unref(screen, backing->bo);
   FREE(backing->chunks);
      }
      /*
   * Return a range of pages from the given backing buffer back into the
   * free structure.
   */
   static bool
   sparse_backing_free(struct zink_screen *screen, struct zink_bo *bo,
               {
      uint32_t end_page = start_page + num_pages;
   unsigned low = 0;
            /* Find the first chunk with begin >= start_page. */
   while (low < high) {
               if (backing->chunks[mid].begin >= start_page)
         else
               assert(low >= backing->num_chunks || end_page <= backing->chunks[low].begin);
            if (low > 0 && backing->chunks[low - 1].end == start_page) {
               if (low < backing->num_chunks && end_page == backing->chunks[low].begin) {
      backing->chunks[low - 1].end = backing->chunks[low].end;
   memmove(&backing->chunks[low], &backing->chunks[low + 1],
               } else if (low < backing->num_chunks && end_page == backing->chunks[low].begin) {
         } else {
      if (backing->num_chunks >= backing->max_chunks) {
      unsigned new_max_chunks = 2 * backing->max_chunks;
   struct zink_sparse_backing_chunk *new_chunks =
      REALLOC(backing->chunks,
                           backing->max_chunks = new_max_chunks;
               memmove(&backing->chunks[low + 1], &backing->chunks[low],
         backing->chunks[low].begin = start_page;
   backing->chunks[low].end = end_page;
               if (backing->num_chunks == 1 && backing->chunks[0].begin == 0 &&
      backing->chunks[0].end == backing->bo->base.size / ZINK_SPARSE_BUFFER_PAGE_SIZE)
            }
      static void
   bo_sparse_destroy(struct zink_screen *screen, struct pb_buffer *pbuf)
   {
                        while (!list_is_empty(&bo->u.sparse.backing)) {
      sparse_free_backing_buffer(screen, bo,
                     FREE(bo->u.sparse.commitments);
   simple_mtx_destroy(&bo->lock);
      }
      static const struct pb_vtbl bo_sparse_vtbl = {
      /* Cast to void* because one of the function parameters is a struct pointer instead of void*. */
   (void*)bo_sparse_destroy
      };
      static struct pb_buffer *
   bo_sparse_create(struct zink_screen *screen, uint64_t size)
   {
               /* We use 32-bit page numbers; refuse to attempt allocating sparse buffers
   * that exceed this limit. This is not really a restriction: we don't have
   * that much virtual address space anyway.
   */
   if (size > (uint64_t)INT32_MAX * ZINK_SPARSE_BUFFER_PAGE_SIZE)
            bo = CALLOC_STRUCT(zink_bo);
   if (!bo)
            simple_mtx_init(&bo->lock, mtx_plain);
   pipe_reference_init(&bo->base.reference, 1);
   bo->base.alignment_log2 = util_logbase2(ZINK_SPARSE_BUFFER_PAGE_SIZE);
   bo->base.size = size;
   bo->base.vtbl = &bo_sparse_vtbl;
   unsigned placement = zink_mem_type_idx_from_types(screen, ZINK_HEAP_DEVICE_LOCAL_SPARSE, UINT32_MAX);
   assert(placement != UINT32_MAX);
   bo->base.placement = placement;
   bo->unique_id = p_atomic_inc_return(&screen->pb.next_bo_unique_id);
            bo->u.sparse.num_va_pages = DIV_ROUND_UP(size, ZINK_SPARSE_BUFFER_PAGE_SIZE);
   bo->u.sparse.commitments = CALLOC(bo->u.sparse.num_va_pages,
         if (!bo->u.sparse.commitments)
                           error_alloc_commitments:
      simple_mtx_destroy(&bo->lock);
   FREE(bo);
      }
      struct pb_buffer *
   zink_bo_create(struct zink_screen *screen, uint64_t size, unsigned alignment, enum zink_heap heap, enum zink_alloc_flag flags, unsigned mem_type_idx, const void *pNext)
   {
      struct zink_bo *bo;
   /* pull in sparse flag */
            //struct pb_slabs *slabs = ((flags & RADEON_FLAG_ENCRYPTED) && screen->info.has_tmz_support) ?
                  struct pb_slabs *last_slab = &slabs[NUM_SLAB_ALLOCATORS - 1];
            /* Sub-allocate small buffers from slabs. */
   if (!(flags & (ZINK_ALLOC_NO_SUBALLOC | ZINK_ALLOC_SPARSE)) &&
      size <= max_slab_entry_size) {
            if (heap < 0 || heap >= ZINK_HEAP_MAX)
                     /* Always use slabs for sizes less than 4 KB because the kernel aligns
   * everything to 4 KB.
   */
   if (size < alignment && alignment <= 4 * 1024)
            if (alignment > get_slab_entry_alignment(screen, alloc_size)) {
      /* 3/4 allocations can return too small alignment. Try again with a power of two
   * allocation size.
                  if (alignment <= pot_size) {
      /* This size works but wastes some memory to fulfil the alignment. */
      } else {
                     struct pb_slabs *slabs = get_slabs(screen, alloc_size, flags);
   bool reclaim_all = false;
   if (heap == ZINK_HEAP_DEVICE_LOCAL_VISIBLE && !screen->resizable_bar) {
      unsigned low_bound = 128 * 1024 * 1024; //128MB is a very small BAR
   if (screen->info.driver_props.driverID == VK_DRIVER_ID_NVIDIA_PROPRIETARY)
         unsigned vk_heap_idx = screen->info.mem_props.memoryTypes[mem_type_idx].heapIndex;
      }
   entry = pb_slab_alloc_reclaimed(slabs, alloc_size, mem_type_idx, reclaim_all);
   if (!entry) {
      /* Clean up buffer managers and try again. */
   if (clean_up_buffer_managers(screen))
      }
   if (!entry)
            bo = container_of(entry, struct zink_bo, u.slab.entry);
   assert(bo->base.placement == mem_type_idx);
   pipe_reference_init(&bo->base.reference, 1);
   bo->base.size = size;
                  no_slab:
         if (flags & ZINK_ALLOC_SPARSE) {
                           /* Align size to page size. This is the minimum alignment for normal
   * BOs. Aligning this here helps the cached bufmgr. Especially small BOs,
   * like constant/uniform buffers, can benefit from better and more reuse.
   */
   if (heap == ZINK_HEAP_DEVICE_LOCAL_VISIBLE) {
      size = align64(size, screen->info.props.limits.minMemoryMapAlignment);
                        if (use_reusable_pool) {
      /* Get a buffer from the cache. */
   bo = (struct zink_bo*)
         assert(!bo || bo->base.placement == mem_type_idx);
   if (bo)
               /* Create a new one. */
   bo = bo_create_internal(screen, size, alignment, heap, mem_type_idx, flags, pNext);
   if (!bo) {
      /* Clean up buffer managers and try again. */
   if (clean_up_buffer_managers(screen))
         if (!bo)
      }
               }
      void *
   zink_bo_map(struct zink_screen *screen, struct zink_bo *bo)
   {
      void *cpu = NULL;
   uint64_t offset = 0;
            if (bo->mem) {
         } else {
      real = bo->u.slab.real;
               cpu = p_atomic_read(&real->u.real.cpu_ptr);
   if (!cpu) {
      simple_mtx_lock(&real->lock);
   /* Must re-check due to the possibility of a race. Re-check need not
   * be atomic thanks to the lock. */
   cpu = real->u.real.cpu_ptr;
   if (!cpu) {
      VkResult result = VKSCR(MapMemory)(screen->dev, real->mem, 0, real->base.size, 0, &cpu);
   if (result != VK_SUCCESS) {
      mesa_loge("ZINK: vkMapMemory failed (%s)", vk_Result_to_str(result));
   simple_mtx_unlock(&real->lock);
      }
   if (unlikely(zink_debug & ZINK_DEBUG_MAP)) {
      p_atomic_add(&screen->mapped_vram, real->base.size);
      }
      }
      }
               }
      void
   zink_bo_unmap(struct zink_screen *screen, struct zink_bo *bo)
   {
                        if (p_atomic_dec_zero(&real->u.real.map_count)) {
      p_atomic_set(&real->u.real.cpu_ptr, NULL);
   if (unlikely(zink_debug & ZINK_DEBUG_MAP)) {
      p_atomic_add(&screen->mapped_vram, -real->base.size);
      }
         }
      static VkSemaphore
   buffer_commit_single(struct zink_screen *screen, struct zink_resource *res, struct zink_bo *bo, uint32_t bo_offset, uint32_t offset, uint32_t size, bool commit, VkSemaphore wait)
   {
      VkSemaphore sem = zink_create_semaphore(screen);
   VkBindSparseInfo sparse = {0};
   sparse.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
   sparse.bufferBindCount = res->obj->storage_buffer ? 2 : 1;
   sparse.waitSemaphoreCount = !!wait;
   sparse.pWaitSemaphores = &wait;
   sparse.signalSemaphoreCount = 1;
            VkSparseBufferMemoryBindInfo sparse_bind[2];
   sparse_bind[0].buffer = res->obj->buffer;
   sparse_bind[1].buffer = res->obj->storage_buffer;
   sparse_bind[0].bindCount = 1;
   sparse_bind[1].bindCount = 1;
            VkSparseMemoryBind mem_bind;
   mem_bind.resourceOffset = offset;
   mem_bind.size = MIN2(res->base.b.width0 - offset, size);
   mem_bind.memory = commit ? (bo->mem ? bo->mem : bo->u.slab.real->mem) : VK_NULL_HANDLE;
   mem_bind.memoryOffset = bo_offset * ZINK_SPARSE_BUFFER_PAGE_SIZE + (commit ? (bo->mem ? 0 : bo->offset) : 0);
   mem_bind.flags = 0;
   sparse_bind[0].pBinds = &mem_bind;
            VkResult ret = VKSCR(QueueBindSparse)(screen->queue_sparse, 1, &sparse, VK_NULL_HANDLE);
   if (zink_screen_handle_vkresult(screen, ret))
         VKSCR(DestroySemaphore)(screen->dev, sem, NULL);
      }
      static bool
   buffer_bo_commit(struct zink_screen *screen, struct zink_resource *res, uint32_t offset, uint32_t size, bool commit, VkSemaphore *sem)
   {
      bool ok = true;
   struct zink_bo *bo = res->obj->bo;
   assert(offset % ZINK_SPARSE_BUFFER_PAGE_SIZE == 0);
   assert(offset <= bo->base.size);
   assert(size <= bo->base.size - offset);
                     uint32_t va_page = offset / ZINK_SPARSE_BUFFER_PAGE_SIZE;
   uint32_t end_va_page = va_page + DIV_ROUND_UP(size, ZINK_SPARSE_BUFFER_PAGE_SIZE);
   VkSemaphore cur_sem = VK_NULL_HANDLE;
   if (commit) {
      while (va_page < end_va_page) {
               /* Skip pages that are already committed. */
   if (comm[va_page].backing) {
      va_page++;
               /* Determine length of uncommitted span. */
   span_va_page = va_page;
                  /* Fill the uncommitted span with chunks of backing memory. */
   while (span_va_page < va_page) {
                     backing_size = va_page - span_va_page;
   backing = sparse_backing_alloc(screen, bo, &backing_start, &backing_size);
   if (!backing) {
      ok = false;
      }
   cur_sem = buffer_commit_single(screen, res, backing->bo, backing_start,
               if (!cur_sem) {
                                       while (backing_size) {
      comm[span_va_page].backing = backing;
   comm[span_va_page].page = backing_start;
   span_va_page++;
   backing_start++;
               } else {
      bool done = false;
   uint32_t base_page = va_page;
   while (va_page < end_va_page) {
      struct zink_sparse_backing *backing;
                  /* Skip pages that are already uncommitted. */
   if (!comm[va_page].backing) {
      va_page++;
               if (!done) {
      cur_sem = buffer_commit_single(screen, res, NULL, 0,
               if (!cur_sem) {
      ok = false;
                        /* Group contiguous spans of pages. */
   backing = comm[va_page].backing;
                                 while (va_page < end_va_page &&
         comm[va_page].backing == backing &&
      comm[va_page].backing = NULL;
   va_page++;
               if (!sparse_backing_free(screen, bo, backing, backing_start, span_pages)) {
      /* Couldn't allocate tracking data structures, so we have to leak */
   fprintf(stderr, "zink: leaking sparse backing memory\n");
               out:
      *sem = cur_sem;
      }
      static VkSemaphore
   texture_commit_single(struct zink_screen *screen, struct zink_resource *res, VkSparseImageMemoryBind *ibind, unsigned num_binds, bool commit, VkSemaphore wait)
   {
      VkSemaphore sem = zink_create_semaphore(screen);
   VkBindSparseInfo sparse = {0};
   sparse.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
   sparse.imageBindCount = 1;
   sparse.waitSemaphoreCount = !!wait;
   sparse.pWaitSemaphores = &wait;
   sparse.signalSemaphoreCount = 1;
            VkSparseImageMemoryBindInfo sparse_ibind;
   sparse_ibind.image = res->obj->image;
   sparse_ibind.bindCount = num_binds;
   sparse_ibind.pBinds = ibind;
            VkResult ret = VKSCR(QueueBindSparse)(screen->queue_sparse, 1, &sparse, VK_NULL_HANDLE);
   if (zink_screen_handle_vkresult(screen, ret))
         VKSCR(DestroySemaphore)(screen->dev, sem, NULL);
      }
      static VkSemaphore
   texture_commit_miptail(struct zink_screen *screen, struct zink_resource *res, struct zink_bo *bo, uint32_t bo_offset, uint32_t offset, bool commit, VkSemaphore wait)
   {
      VkSemaphore sem = zink_create_semaphore(screen);
   VkBindSparseInfo sparse = {0};
   sparse.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
   sparse.imageOpaqueBindCount = 1;
   sparse.waitSemaphoreCount = !!wait;
   sparse.pWaitSemaphores = &wait;
   sparse.signalSemaphoreCount = 1;
            VkSparseImageOpaqueMemoryBindInfo sparse_bind;
   sparse_bind.image = res->obj->image;
   sparse_bind.bindCount = 1;
            VkSparseMemoryBind mem_bind;
   mem_bind.resourceOffset = offset;
   mem_bind.size = MIN2(ZINK_SPARSE_BUFFER_PAGE_SIZE, res->sparse.imageMipTailSize - offset);
   mem_bind.memory = commit ? (bo->mem ? bo->mem : bo->u.slab.real->mem) : VK_NULL_HANDLE;
   mem_bind.memoryOffset = bo_offset + (commit ? (bo->mem ? 0 : bo->offset) : 0);
   mem_bind.flags = 0;
            VkResult ret = VKSCR(QueueBindSparse)(screen->queue_sparse, 1, &sparse, VK_NULL_HANDLE);
   if (zink_screen_handle_vkresult(screen, ret))
         VKSCR(DestroySemaphore)(screen->dev, sem, NULL);
      }
      bool
   zink_bo_commit(struct zink_screen *screen, struct zink_resource *res, unsigned level, struct pipe_box *box, bool commit, VkSemaphore *sem)
   {
      bool ok = true;
   struct zink_bo *bo = res->obj->bo;
            if (screen->faked_e5sparse && res->base.b.format == PIPE_FORMAT_R9G9B9E5_FLOAT)
            simple_mtx_lock(&screen->queue_lock);
   simple_mtx_lock(&bo->lock);
   if (res->base.b.target == PIPE_BUFFER) {
      ok = buffer_bo_commit(screen, res, box->x, box->width, commit, &cur_sem);
               int gwidth, gheight, gdepth;
   gwidth = res->sparse.formatProperties.imageGranularity.width;
   gheight = res->sparse.formatProperties.imageGranularity.height;
   gdepth = res->sparse.formatProperties.imageGranularity.depth;
            struct zink_sparse_commitment *comm = bo->u.sparse.commitments;
   VkImageSubresource subresource = { res->aspect, level, 0 };
   unsigned nwidth = DIV_ROUND_UP(box->width, gwidth);
   unsigned nheight = DIV_ROUND_UP(box->height, gheight);
   unsigned ndepth = DIV_ROUND_UP(box->depth, gdepth);
   VkExtent3D lastBlockExtent = {
      (box->width % gwidth) ? box->width % gwidth : gwidth,
   (box->height % gheight) ? box->height % gheight : gheight,
         #define NUM_BATCHED_BINDS 50
      VkSparseImageMemoryBind ibind[NUM_BATCHED_BINDS];
   uint32_t backing_start[NUM_BATCHED_BINDS], backing_size[NUM_BATCHED_BINDS];
   struct zink_sparse_backing *backing[NUM_BATCHED_BINDS];
   unsigned i = 0;
   bool commits_pending = false;
   uint32_t va_page_offset = 0;
   for (unsigned l = 0; l < level; l++) {
      unsigned mipwidth = DIV_ROUND_UP(MAX2(res->base.b.width0 >> l, 1), gwidth);
   unsigned mipheight = DIV_ROUND_UP(MAX2(res->base.b.height0 >> l, 1), gheight);
   unsigned mipdepth = DIV_ROUND_UP(res->base.b.array_size > 1 ? res->base.b.array_size : MAX2(res->base.b.depth0 >> l, 1), gdepth);
      }
   for (unsigned d = 0; d < ndepth; d++) {
      for (unsigned h = 0; h < nheight; h++) {
      for (unsigned w = 0; w < nwidth; w++) {
      ibind[i].subresource = subresource;
   ibind[i].flags = 0;
   // Offset
   ibind[i].offset.x = w * gwidth;
   ibind[i].offset.y = h * gheight;
   if (res->base.b.array_size > 1) {
      ibind[i].subresource.arrayLayer = d * gdepth;
      } else {
         }
   // Size of the page
   ibind[i].extent.width = (w == nwidth - 1) ? lastBlockExtent.width : gwidth;
   ibind[i].extent.height = (h == nheight - 1) ? lastBlockExtent.height : gheight;
   ibind[i].extent.depth = (d == ndepth - 1 && res->base.b.target != PIPE_TEXTURE_CUBE) ? lastBlockExtent.depth : gdepth;
   uint32_t va_page = va_page_offset +
                                 if (commit) {
                        /* Skip pages that are already committed. */
   if (comm[va_page].backing) {
                        /* Determine length of uncommitted span. */
                        /* Fill the uncommitted span with chunks of backing memory. */
   while (span_va_page < va_page) {
      backing_size[i] = va_page - span_va_page;
   backing[i] = sparse_backing_alloc(screen, bo, &backing_start[i], &backing_size[i]);
   if (!backing[i]) {
      ok = false;
      }
   if (level >= res->sparse.imageMipTailFirstLod) {
      uint32_t offset = res->sparse.imageMipTailOffset + d * res->sparse.imageMipTailStride;
   cur_sem = texture_commit_miptail(screen, res, backing[i]->bo, backing_start[i], offset, commit, cur_sem);
   if (!cur_sem)
      } else {
      ibind[i].memory = backing[i]->bo->mem ? backing[i]->bo->mem : backing[i]->bo->u.slab.real->mem;
                           while (backing_size[i]) {
      comm[span_va_page].backing = backing[i];
   comm[span_va_page].page = backing_start[i];
   span_va_page++;
   backing_start[i]++;
      }
            } else {
                     while (va_page < end_va_page) {
      /* Skip pages that are already uncommitted. */
   if (!comm[va_page].backing) {
                        /* Group contiguous spans of pages. */
                                       while (va_page < end_va_page &&
         comm[va_page].backing == backing[i] &&
      comm[va_page].backing = NULL;
   va_page++;
      }
   if (level >= res->sparse.imageMipTailFirstLod) {
      uint32_t offset = res->sparse.imageMipTailOffset + d * res->sparse.imageMipTailStride;
   cur_sem = texture_commit_miptail(screen, res, NULL, 0, offset, commit, cur_sem);
   if (!cur_sem)
      } else {
         }
         }
   if (i == ARRAY_SIZE(ibind)) {
      cur_sem = texture_commit_single(screen, res, ibind, ARRAY_SIZE(ibind), commit, cur_sem);
   if (!cur_sem) {
      for (unsigned s = 0; s < i; s++) {
      ok = sparse_backing_free(screen, backing[s]->bo, backing[s], backing_start[s], backing_size[s]);
   if (!ok) {
      /* Couldn't allocate tracking data structures, so we have to leak */
         }
   ok = false;
      }
   commits_pending = false;
               }
   if (commits_pending) {
      cur_sem = texture_commit_single(screen, res, ibind, i, commit, cur_sem);
   if (!cur_sem) {
      for (unsigned s = 0; s < i; s++) {
      ok = sparse_backing_free(screen, backing[s]->bo, backing[s], backing_start[s], backing_size[s]);
   if (!ok) {
      /* Couldn't allocate tracking data structures, so we have to leak */
         }
            out:
         simple_mtx_unlock(&bo->lock);
   simple_mtx_unlock(&screen->queue_lock);
   *sem = cur_sem;
      }
      bool
   zink_bo_get_kms_handle(struct zink_screen *screen, struct zink_bo *bo, int fd, uint32_t *handle)
   {
   #ifdef ZINK_USE_DMABUF
      assert(bo->mem && !bo->u.real.use_reusable_pool);
   simple_mtx_lock(&bo->u.real.export_lock);
   list_for_each_entry(struct bo_export, export, &bo->u.real.exports, link) {
      if (export->drm_fd == fd) {
      simple_mtx_unlock(&bo->u.real.export_lock);
   *handle = export->gem_handle;
         }
   struct bo_export *export = CALLOC_STRUCT(bo_export);
   if (!export) {
      simple_mtx_unlock(&bo->u.real.export_lock);
      }
   bool success = drmPrimeFDToHandle(screen->drm_fd, fd, handle) == 0;
   if (success) {
      list_addtail(&export->link, &bo->u.real.exports);
   export->gem_handle = *handle;
      } else {
      mesa_loge("zink: failed drmPrimeFDToHandle %s", strerror(errno));
      }
   simple_mtx_unlock(&bo->u.real.export_lock);
      #else
         #endif
   }
      static const struct pb_vtbl bo_slab_vtbl = {
      /* Cast to void* because one of the function parameters is a struct pointer instead of void*. */
   (void*)bo_slab_destroy
      };
      static struct pb_slab *
   bo_slab_alloc(void *priv, unsigned mem_type_idx, unsigned entry_size, unsigned group_index, bool encrypted)
   {
      struct zink_screen *screen = priv;
   uint32_t base_id;
   unsigned slab_size = 0;
            if (!slab)
            //struct pb_slabs *slabs = ((flags & RADEON_FLAG_ENCRYPTED) && screen->info.has_tmz_support) ?
                  /* Determine the slab buffer size. */
   for (unsigned i = 0; i < NUM_SLAB_ALLOCATORS; i++) {
               if (entry_size <= max_entry_size) {
                                       /* If the entry size is 3/4 of a power of two, we would waste space and not gain
   * anything if we allocated only twice the power of two for the backing buffer:
   *   2 * 3/4 = 1.5 usable with buffer size 2
   *
   * Allocating 5 times the entry size leads us to the next power of two and results
   * in a much better memory utilization:
   *   5 * 3/4 = 3.75 usable with buffer size 4
   */
   if (entry_size * 5 > slab_size)
                     }
            slab->buffer = zink_bo(zink_bo_create(screen, slab_size, slab_size, zink_heap_from_domain_flags(screen->info.mem_props.memoryTypes[mem_type_idx].propertyFlags, 0),
         if (!slab->buffer)
                     slab->base.num_entries = slab_size / entry_size;
   slab->base.num_free = slab->base.num_entries;
   slab->entry_size = entry_size;
   slab->entries = CALLOC(slab->base.num_entries, sizeof(*slab->entries));
   if (!slab->entries)
                     base_id = p_atomic_fetch_add(&screen->pb.next_bo_unique_id, slab->base.num_entries);
   for (unsigned i = 0; i < slab->base.num_entries; ++i) {
               simple_mtx_init(&bo->lock, mtx_plain);
   bo->base.alignment_log2 = util_logbase2(get_slab_entry_alignment(screen, entry_size));
   bo->base.size = entry_size;
   bo->base.vtbl = &bo_slab_vtbl;
   bo->offset = slab->buffer->offset + i * entry_size;
   bo->unique_id = base_id + i;
   bo->u.slab.entry.slab = &slab->base;
   bo->u.slab.entry.group_index = group_index;
            if (slab->buffer->mem) {
      /* The slab is not suballocated. */
      } else {
      /* The slab is allocated out of a bigger slab. */
   bo->u.slab.real = slab->buffer->u.slab.real;
      }
                        /* Wasted alignment due to slabs with 3/4 allocations being aligned to a power of two. */
                  fail_buffer:
         fail:
      FREE(slab);
      }
      static struct pb_slab *
   bo_slab_alloc_normal(void *priv, unsigned mem_type_idx, unsigned entry_size, unsigned group_index)
   {
         }
      bool
   zink_bo_init(struct zink_screen *screen)
   {
      uint64_t total_mem = 0;
   for (uint32_t i = 0; i < screen->info.mem_props.memoryHeapCount; ++i)
         /* Create managers. */
   pb_cache_init(&screen->pb.bo_cache, screen->info.mem_props.memoryTypeCount,
                        unsigned min_slab_order = MIN_SLAB_ORDER;  /* 256 bytes */
   unsigned max_slab_order = 20; /* 1 MB (slab size = 2 MB) */
   unsigned num_slab_orders_per_allocator = (max_slab_order - min_slab_order) /
            /* Divide the size order range among slab managers. */
   for (unsigned i = 0; i < NUM_SLAB_ALLOCATORS; i++) {
      unsigned min_order = min_slab_order;
   unsigned max_order = MIN2(min_order + num_slab_orders_per_allocator,
            if (!pb_slabs_init(&screen->pb.bo_slabs[i],
                     min_order, max_order,
   screen->info.mem_props.memoryTypeCount, true,
   screen,
         }
      }
   screen->pb.min_alloc_size = 1 << screen->pb.bo_slabs[0].min_order;
      }
      void
   zink_bo_deinit(struct zink_screen *screen)
   {
      for (unsigned i = 0; i < NUM_SLAB_ALLOCATORS; i++) {
      if (screen->pb.bo_slabs[i].groups)
      }
      }
