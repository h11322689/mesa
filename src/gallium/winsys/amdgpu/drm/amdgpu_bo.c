   /*
   * Copyright © 2011 Marek Olšák <maraeo@gmail.com>
   * Copyright © 2015 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include <sys/ioctl.h>
      #include "amdgpu_cs.h"
      #include "util/hash_table.h"
   #include "util/os_time.h"
   #include "util/u_hash_table.h"
   #include "util/u_process.h"
   #include "frontend/drm_driver.h"
   #include "drm-uapi/amdgpu_drm.h"
   #include "drm-uapi/dma-buf.h"
   #include <xf86drm.h>
   #include <stdio.h>
   #include <inttypes.h>
      #ifndef AMDGPU_VA_RANGE_HIGH
   #define AMDGPU_VA_RANGE_HIGH	0x2
   #endif
      /* Set to 1 for verbose output showing committed sparse buffer ranges. */
   #define DEBUG_SPARSE_COMMITS 0
      struct amdgpu_sparse_backing_chunk {
         };
      static bool amdgpu_bo_wait(struct radeon_winsys *rws,
               {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   struct amdgpu_winsys_bo *bo = amdgpu_winsys_bo(_buf);
            if (timeout == 0) {
      if (p_atomic_read(&bo->num_active_ioctls))
         } else {
               /* Wait if any ioctl is being submitted with this buffer. */
   if (!os_wait_until_zero_abs_timeout(&bo->num_active_ioctls, abs_timeout))
               if (bo->bo && bo->u.real.is_shared) {
      /* We can't use user fences for shared buffers, because user fences
   * are local to this process only. If we want to wait for all buffer
   * uses in all processes, we have to use amdgpu_bo_wait_for_idle.
   */
   bool buffer_busy = true;
            r = amdgpu_bo_wait_for_idle(bo->bo, timeout, &buffer_busy);
   if (r)
      fprintf(stderr, "%s: amdgpu_bo_wait_for_idle failed %i\n", __func__,
                  if (timeout == 0) {
      unsigned idle_fences;
                     for (idle_fences = 0; idle_fences < bo->num_fences; ++idle_fences) {
      if (!amdgpu_fence_wait(bo->fences[idle_fences], 0, false))
               /* Release the idle fences to avoid checking them again later. */
   for (unsigned i = 0; i < idle_fences; ++i)
            memmove(&bo->fences[0], &bo->fences[idle_fences],
                  buffer_idle = !bo->num_fences;
               } else {
               simple_mtx_lock(&ws->bo_fence_lock);
   while (bo->num_fences && buffer_idle) {
                              /* Wait for the fence. */
   simple_mtx_unlock(&ws->bo_fence_lock);
   if (amdgpu_fence_wait(fence, abs_timeout, true))
         else
                  /* Release an idle fence to avoid checking it again later, keeping in
   * mind that the fence array may have been modified by other threads.
   */
   if (fence_idle && bo->num_fences && bo->fences[0] == fence) {
      amdgpu_fence_reference(&bo->fences[0], NULL);
   memmove(&bo->fences[0], &bo->fences[1],
                        }
                  }
      static enum radeon_bo_domain amdgpu_bo_get_initial_domain(
         {
         }
      static enum radeon_bo_flag amdgpu_bo_get_flags(
         {
         }
      static void amdgpu_bo_remove_fences(struct amdgpu_winsys_bo *bo)
   {
      for (unsigned i = 0; i < bo->num_fences; ++i)
            FREE(bo->fences);
   bo->num_fences = 0;
      }
      void amdgpu_bo_destroy(struct amdgpu_winsys *ws, struct pb_buffer *_buf)
   {
      struct amdgpu_winsys_bo *bo = amdgpu_winsys_bo(_buf);
                              /* amdgpu_bo_from_handle might have revived the bo */
   if (p_atomic_read(&bo->base.reference.count)) {
      simple_mtx_unlock(&ws->bo_export_table_lock);
                        if (bo->base.placement & RADEON_DOMAIN_VRAM_GTT) {
      amdgpu_bo_va_op(bo->bo, 0, bo->base.size, bo->va, 0, AMDGPU_VA_OP_UNMAP);
                        if (!bo->u.real.is_user_ptr && bo->u.real.cpu_ptr) {
      bo->u.real.cpu_ptr = NULL;
      }
                  #if DEBUG
      if (ws->debug_all_bos) {
      simple_mtx_lock(&ws->global_bo_list_lock);
   list_del(&bo->u.real.global_list_item);
   ws->num_buffers--;
         #endif
         /* Close all KMS handles retrieved for other DRM file descriptions */
   simple_mtx_lock(&ws->sws_list_lock);
   for (sws_iter = ws->sws_list; sws_iter; sws_iter = sws_iter->next) {
               if (!sws_iter->kms_handles)
            entry = _mesa_hash_table_search(sws_iter->kms_handles, bo);
   if (entry) {
               drmIoctl(sws_iter->fd, DRM_IOCTL_GEM_CLOSE, &args);
         }
                     if (bo->base.placement & RADEON_DOMAIN_VRAM)
         else if (bo->base.placement & RADEON_DOMAIN_GTT)
            simple_mtx_destroy(&bo->lock);
      }
      static void amdgpu_bo_destroy_or_cache(struct radeon_winsys *rws, struct pb_buffer *_buf)
   {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
                     if (bo->u.real.use_reusable_pool)
         else
      }
      static void amdgpu_clean_up_buffer_managers(struct amdgpu_winsys *ws)
   {
      for (unsigned i = 0; i < NUM_SLAB_ALLOCATORS; i++)
               }
      static bool amdgpu_bo_do_map(struct radeon_winsys *rws, struct amdgpu_winsys_bo *bo, void **cpu)
   {
               assert(!(bo->base.usage & RADEON_FLAG_SPARSE) && bo->bo && !bo->u.real.is_user_ptr);
   int r = amdgpu_bo_cpu_map(bo->bo, cpu);
   if (r) {
      /* Clean up buffer managers and try again. */
   amdgpu_clean_up_buffer_managers(ws);
   r = amdgpu_bo_cpu_map(bo->bo, cpu);
   if (r)
               if (p_atomic_inc_return(&bo->u.real.map_count) == 1) {
      if (bo->base.placement & RADEON_DOMAIN_VRAM)
         else if (bo->base.placement & RADEON_DOMAIN_GTT)
                        }
      void *amdgpu_bo_map(struct radeon_winsys *rws,
                     {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   struct amdgpu_winsys_bo *bo = (struct amdgpu_winsys_bo*)buf;
   struct amdgpu_winsys_bo *real;
                     /* If it's not unsynchronized bo_map, flush CS if needed and then wait. */
   if (!(usage & PIPE_MAP_UNSYNCHRONIZED)) {
      /* DONTBLOCK doesn't make sense with UNSYNCHRONIZED. */
   if (usage & PIPE_MAP_DONTBLOCK) {
      if (!(usage & PIPE_MAP_WRITE)) {
      /* Mapping for read.
   *
   * Since we are mapping for read, we don't need to wait
   * if the GPU is using the buffer for read too
   * (neither one is changing it).
   *
   * Only check whether the buffer is being used for write. */
   if (cs && amdgpu_bo_is_referenced_by_cs_with_usage(cs, bo,
         RADEON_FLUSH_ASYNC_START_NEXT_GFX_IB_NOW, NULL);
                        if (!amdgpu_bo_wait(rws, (struct pb_buffer*)bo, 0,
                  } else {
      if (cs && amdgpu_bo_is_referenced_by_cs(cs, bo)) {
   RADEON_FLUSH_ASYNC_START_NEXT_GFX_IB_NOW, NULL);
                        if (!amdgpu_bo_wait(rws, (struct pb_buffer*)bo, 0,
                     } else {
               if (!(usage & PIPE_MAP_WRITE)) {
      /* Mapping for read.
   *
   * Since we are mapping for read, we don't need to wait
   * if the GPU is using the buffer for read too
   * (neither one is changing it).
   *
   * Only check whether the buffer is being used for write. */
   if (cs) {
      if (amdgpu_bo_is_referenced_by_cs_with_usage(cs, bo,
         RADEON_FLUSH_START_NEXT_GFX_IB_NOW, NULL);
         } else {
      /* Try to avoid busy-waiting in amdgpu_bo_wait. */
   if (p_atomic_read(&bo->num_active_ioctls))
                  amdgpu_bo_wait(rws, (struct pb_buffer*)bo, OS_TIMEOUT_INFINITE,
      } else {
      /* Mapping for write. */
   if (cs) {
      if (amdgpu_bo_is_referenced_by_cs(cs, bo)) {
   RADEON_FLUSH_START_NEXT_GFX_IB_NOW, NULL);
         } else {
      /* Try to avoid busy-waiting in amdgpu_bo_wait. */
   if (p_atomic_read(&bo->num_active_ioctls))
                  amdgpu_bo_wait(rws, (struct pb_buffer*)bo, OS_TIMEOUT_INFINITE,
                              /* Buffer synchronization has been checked, now actually map the buffer. */
   void *cpu = NULL;
            if (bo->bo) {
         } else {
      real = bo->u.slab.real;
               if (usage & RADEON_MAP_TEMPORARY) {
      if (real->u.real.is_user_ptr) {
         } else {
      if (!amdgpu_bo_do_map(rws, real, &cpu))
         } else {
      cpu = p_atomic_read(&real->u.real.cpu_ptr);
   if (!cpu) {
      simple_mtx_lock(&real->lock);
   /* Must re-check due to the possibility of a race. Re-check need not
   * be atomic thanks to the lock. */
   cpu = real->u.real.cpu_ptr;
   if (!cpu) {
      if (!amdgpu_bo_do_map(rws, real, &cpu)) {
      simple_mtx_unlock(&real->lock);
      }
      }
                     }
      void amdgpu_bo_unmap(struct radeon_winsys *rws, struct pb_buffer *buf)
   {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   struct amdgpu_winsys_bo *bo = (struct amdgpu_winsys_bo*)buf;
                              if (real->u.real.is_user_ptr)
            assert(real->u.real.map_count != 0 && "too many unmaps");
   if (p_atomic_dec_zero(&real->u.real.map_count)) {
      assert(!real->u.real.cpu_ptr &&
            if (real->base.placement & RADEON_DOMAIN_VRAM)
         else if (real->base.placement & RADEON_DOMAIN_GTT)
                        }
      static const struct pb_vtbl amdgpu_winsys_bo_vtbl = {
      /* Cast to void* because one of the function parameters is a struct pointer instead of void*. */
   (void*)amdgpu_bo_destroy_or_cache
      };
      static void amdgpu_add_buffer_to_global_list(struct amdgpu_winsys *ws, struct amdgpu_winsys_bo *bo)
   {
   #if DEBUG
               if (ws->debug_all_bos) {
      simple_mtx_lock(&ws->global_bo_list_lock);
   list_addtail(&bo->u.real.global_list_item, &ws->global_bo_list);
   ws->num_buffers++;
         #endif
   }
      static unsigned amdgpu_get_optimal_alignment(struct amdgpu_winsys *ws,
         {
      /* Increase the alignment for faster address translation and better memory
   * access pattern.
   */
   if (size >= ws->info.pte_fragment_size) {
         } else if (size) {
                  }
      }
      static struct amdgpu_winsys_bo *amdgpu_create_bo(struct amdgpu_winsys *ws,
                                 {
      struct amdgpu_bo_alloc_request request = {0};
   amdgpu_bo_handle buf_handle;
   uint64_t va = 0;
   struct amdgpu_winsys_bo *bo;
   amdgpu_va_handle va_handle = NULL;
   int r;
            /* VRAM or GTT must be specified, but not both at the same time. */
   assert(util_bitcount(initial_domain & (RADEON_DOMAIN_VRAM_GTT |
                                    bo = CALLOC(1, sizeof(struct amdgpu_winsys_bo) +
         if (!bo) {
                  if (init_pb_cache) {
      bo->u.real.use_reusable_pool = true;
   pb_cache_init_entry(&ws->bo_cache, bo->cache_entry, &bo->base,
      }
   request.alloc_size = size;
            if (initial_domain & RADEON_DOMAIN_VRAM) {
               /* Since VRAM and GTT have almost the same performance on APUs, we could
   * just set GTT. However, in order to decrease GTT(RAM) usage, which is
   * shared with the OS, allow VRAM placements too. The idea is not to use
   * VRAM usefully, but to use it so that it's not unused and wasted.
   */
   if (!ws->info.has_dedicated_vram)
               if (initial_domain & RADEON_DOMAIN_GTT)
         if (initial_domain & RADEON_DOMAIN_GDS)
         if (initial_domain & RADEON_DOMAIN_OA)
            if (flags & RADEON_FLAG_NO_CPU_ACCESS)
         if (flags & RADEON_FLAG_GTT_WC)
            if (flags & RADEON_FLAG_DISCARDABLE &&
      ws->info.drm_minor >= 47)
         if (ws->zero_all_vram_allocs &&
      (request.preferred_heap & AMDGPU_GEM_DOMAIN_VRAM))
         if ((flags & RADEON_FLAG_ENCRYPTED) &&
      ws->info.has_tmz_support) {
            if (!(flags & RADEON_FLAG_DRIVER_INTERNAL)) {
      struct amdgpu_screen_winsys *sws_iter;
   simple_mtx_lock(&ws->sws_list_lock);
   for (sws_iter = ws->sws_list; sws_iter; sws_iter = sws_iter->next) {
         }
                  r = amdgpu_bo_alloc(ws->dev, &request, &buf_handle);
   if (r) {
      fprintf(stderr, "amdgpu: Failed to allocate a buffer:\n");
   fprintf(stderr, "amdgpu:    size      : %"PRIu64" bytes\n", size);
   fprintf(stderr, "amdgpu:    alignment : %u bytes\n", alignment);
   fprintf(stderr, "amdgpu:    domains   : %u\n", initial_domain);
   fprintf(stderr, "amdgpu:    flags   : %" PRIx64 "\n", request.flags);
               if (initial_domain & RADEON_DOMAIN_VRAM_GTT) {
               r = amdgpu_va_range_alloc(ws->dev, amdgpu_gpu_va_range_general,
                           if (r)
            unsigned vm_flags = AMDGPU_VM_PAGE_READABLE |
            if (!(flags & RADEON_FLAG_READ_ONLY))
            if (flags & RADEON_FLAG_GL2_BYPASS)
            r = amdgpu_bo_va_op_raw(ws->dev, buf_handle, 0, size, va, vm_flags,
   AMDGPU_VA_OP_MAP);
   if (r)
               simple_mtx_init(&bo->lock, mtx_plain);
   pipe_reference_init(&bo->base.reference, 1);
   bo->base.alignment_log2 = util_logbase2(alignment);
   bo->base.size = size;
   bo->base.vtbl = &amdgpu_winsys_bo_vtbl;
   bo->bo = buf_handle;
   bo->va = va;
   bo->u.real.va_handle = va_handle;
   bo->base.placement = initial_domain;
   bo->base.usage = flags;
            if (initial_domain & RADEON_DOMAIN_VRAM)
         else if (initial_domain & RADEON_DOMAIN_GTT)
                                    error_va_map:
            error_va_alloc:
            error_bo_alloc:
      FREE(bo);
      }
      bool amdgpu_bo_can_reclaim(struct amdgpu_winsys *ws, struct pb_buffer *_buf)
   {
         }
      bool amdgpu_bo_can_reclaim_slab(void *priv, struct pb_slab_entry *entry)
   {
                  }
      static struct pb_slabs *get_slabs(struct amdgpu_winsys *ws, uint64_t size)
   {
      /* Find the correct slab allocator for the given size. */
   for (unsigned i = 0; i < NUM_SLAB_ALLOCATORS; i++) {
               if (size <= 1 << (slabs->min_order + slabs->num_orders - 1))
               assert(0);
      }
      static unsigned get_slab_wasted_size(struct amdgpu_winsys *ws, struct amdgpu_winsys_bo *bo)
   {
      assert(bo->base.size <= bo->u.slab.entry.entry_size);
   assert(bo->base.size < (1 << bo->base.alignment_log2) ||
         bo->base.size < 1 << ws->bo_slabs[0].min_order ||
      }
      static void amdgpu_bo_slab_destroy(struct radeon_winsys *rws, struct pb_buffer *_buf)
   {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   struct amdgpu_winsys_bo *bo = amdgpu_winsys_bo(_buf);
                              if (bo->base.placement & RADEON_DOMAIN_VRAM)
         else
               }
      static const struct pb_vtbl amdgpu_winsys_bo_slab_vtbl = {
      /* Cast to void* because one of the function parameters is a struct pointer instead of void*. */
   (void*)amdgpu_bo_slab_destroy
      };
      /* Return the power of two size of a slab entry matching the input size. */
   static unsigned get_slab_pot_entry_size(struct amdgpu_winsys *ws, unsigned size)
   {
      unsigned entry_size = util_next_power_of_two(size);
               }
      /* Return the slab entry alignment. */
   static unsigned get_slab_entry_alignment(struct amdgpu_winsys *ws, unsigned size)
   {
               if (size <= entry_size * 3 / 4)
               }
      struct pb_slab *amdgpu_bo_slab_alloc(void *priv, unsigned heap, unsigned entry_size,
         {
      struct amdgpu_winsys *ws = priv;
   struct amdgpu_slab *slab = CALLOC_STRUCT(amdgpu_slab);
   enum radeon_bo_domain domains = radeon_domain_from_heap(heap);
   enum radeon_bo_flag flags = radeon_flags_from_heap(heap);
   uint32_t base_id;
            if (!slab)
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
               /* The largest slab should have the same size as the PTE fragment
   * size to get faster address translation.
   */
   if (i == NUM_SLAB_ALLOCATORS - 1 &&
      slab_size < ws->info.pte_fragment_size)
            }
            slab->buffer = amdgpu_winsys_bo(amdgpu_bo_create(ws,
               if (!slab->buffer)
                     slab->base.num_entries = slab_size / entry_size;
   slab->base.num_free = slab->base.num_entries;
   slab->entry_size = entry_size;
   slab->entries = CALLOC(slab->base.num_entries, sizeof(*slab->entries));
   if (!slab->entries)
                              for (unsigned i = 0; i < slab->base.num_entries; ++i) {
               simple_mtx_init(&bo->lock, mtx_plain);
   bo->base.alignment_log2 = util_logbase2(get_slab_entry_alignment(ws, entry_size));
   bo->base.size = entry_size;
   bo->base.vtbl = &amdgpu_winsys_bo_slab_vtbl;
   bo->va = slab->buffer->va + i * entry_size;
   bo->base.placement = domains;
   bo->unique_id = base_id + i;
   bo->u.slab.entry.slab = &slab->base;
   bo->u.slab.entry.group_index = group_index;
            if (slab->buffer->bo) {
      /* The slab is not suballocated. */
      } else {
      /* The slab is allocated out of a bigger slab. */
   bo->u.slab.real = slab->buffer->u.slab.real;
                           /* Wasted alignment due to slabs with 3/4 allocations being aligned to a power of two. */
   assert(slab->base.num_entries * entry_size <= slab_size);
   if (domains & RADEON_DOMAIN_VRAM)
         else
                  fail_buffer:
         fail:
      FREE(slab);
      }
      void amdgpu_bo_slab_free(struct amdgpu_winsys *ws, struct pb_slab *pslab)
   {
      struct amdgpu_slab *slab = amdgpu_slab(pslab);
            assert(slab->base.num_entries * slab->entry_size <= slab_size);
   if (slab->buffer->base.placement & RADEON_DOMAIN_VRAM)
         else
            for (unsigned i = 0; i < slab->base.num_entries; ++i) {
      amdgpu_bo_remove_fences(&slab->entries[i]);
               FREE(slab->entries);
   amdgpu_winsys_bo_reference(ws, &slab->buffer, NULL);
      }
      #if DEBUG_SPARSE_COMMITS
   static void
   sparse_dump(struct amdgpu_winsys_bo *bo, const char *func)
   {
      fprintf(stderr, "%s: %p (size=%"PRIu64", num_va_pages=%u) @ %s\n"
                  struct amdgpu_sparse_backing *span_backing = NULL;
   uint32_t span_first_backing_page = 0;
   uint32_t span_first_va_page = 0;
            for (;;) {
      struct amdgpu_sparse_backing *backing = 0;
            if (va_page < bo->u.sparse.num_va_pages) {
      backing = bo->u.sparse.commitments[va_page].backing;
               if (span_backing &&
      (backing != span_backing ||
   backing_page != span_first_backing_page + (va_page - span_first_va_page))) {
   fprintf(stderr, " %u..%u: backing=%p:%u..%u\n",
                                    if (va_page >= bo->u.sparse.num_va_pages)
            if (backing && !span_backing) {
      span_backing = backing;
   span_first_backing_page = backing_page;
                                    list_for_each_entry(struct amdgpu_sparse_backing, backing, &bo->u.sparse.backing, list) {
      fprintf(stderr, " %p (size=%"PRIu64")\n", backing, backing->bo->base.size);
   for (unsigned i = 0; i < backing->num_chunks; ++i)
         }
   #endif
      /*
   * Attempt to allocate the given number of backing pages. Fewer pages may be
   * allocated (depending on the fragmentation of existing backing buffers),
   * which will be reflected by a change to *pnum_pages.
   */
   static struct amdgpu_sparse_backing *
   sparse_backing_alloc(struct amdgpu_winsys *ws, struct amdgpu_winsys_bo *bo,
         {
      struct amdgpu_sparse_backing *best_backing;
   unsigned best_idx;
            best_backing = NULL;
   best_idx = 0;
            /* This is a very simple and inefficient best-fit algorithm. */
   list_for_each_entry(struct amdgpu_sparse_backing, backing, &bo->u.sparse.backing, list) {
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
            best_backing = CALLOC_STRUCT(amdgpu_sparse_backing);
   if (!best_backing)
            best_backing->max_chunks = 4;
   best_backing->chunks = CALLOC(best_backing->max_chunks,
         if (!best_backing->chunks) {
      FREE(best_backing);
                        size = MIN3(bo->base.size / 16,
                        buf = amdgpu_bo_create(ws, size, RADEON_SPARSE_PAGE_SIZE,
                        bo->base.placement,
   (bo->base.usage & ~RADEON_FLAG_SPARSE &
      if (!buf) {
      FREE(best_backing->chunks);
   FREE(best_backing);
               /* We might have gotten a bigger buffer than requested via caching. */
            best_backing->bo = amdgpu_winsys_bo(buf);
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
   sparse_free_backing_buffer(struct amdgpu_winsys *ws, struct amdgpu_winsys_bo *bo,
         {
               simple_mtx_lock(&ws->bo_fence_lock);
   amdgpu_add_fences(backing->bo, bo->num_fences, bo->fences);
            list_del(&backing->list);
   amdgpu_winsys_bo_reference(ws, &backing->bo, NULL);
   FREE(backing->chunks);
      }
      /*
   * Return a range of pages from the given backing buffer back into the
   * free structure.
   */
   static bool
   sparse_backing_free(struct amdgpu_winsys *ws, struct amdgpu_winsys_bo *bo,
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
   struct amdgpu_sparse_backing_chunk *new_chunks =
      REALLOC(backing->chunks,
                           backing->max_chunks = new_max_chunks;
               memmove(&backing->chunks[low + 1], &backing->chunks[low],
         backing->chunks[low].begin = start_page;
   backing->chunks[low].end = end_page;
               if (backing->num_chunks == 1 && backing->chunks[0].begin == 0 &&
      backing->chunks[0].end == backing->bo->base.size / RADEON_SPARSE_PAGE_SIZE)
            }
      static void amdgpu_bo_sparse_destroy(struct radeon_winsys *rws, struct pb_buffer *_buf)
   {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   struct amdgpu_winsys_bo *bo = amdgpu_winsys_bo(_buf);
                     r = amdgpu_bo_va_op_raw(ws->dev, NULL, 0,
               if (r) {
                  while (!list_is_empty(&bo->u.sparse.backing)) {
      sparse_free_backing_buffer(ws, bo,
                     amdgpu_va_range_free(bo->u.sparse.va_handle);
   FREE(bo->u.sparse.commitments);
   simple_mtx_destroy(&bo->lock);
      }
      static const struct pb_vtbl amdgpu_winsys_bo_sparse_vtbl = {
      /* Cast to void* because one of the function parameters is a struct pointer instead of void*. */
   (void*)amdgpu_bo_sparse_destroy
      };
      static struct pb_buffer *
   amdgpu_bo_sparse_create(struct amdgpu_winsys *ws, uint64_t size,
               {
      struct amdgpu_winsys_bo *bo;
   uint64_t map_size;
   uint64_t va_gap_size;
            /* We use 32-bit page numbers; refuse to attempt allocating sparse buffers
   * that exceed this limit. This is not really a restriction: we don't have
   * that much virtual address space anyway.
   */
   if (size > (uint64_t)INT32_MAX * RADEON_SPARSE_PAGE_SIZE)
            bo = CALLOC_STRUCT(amdgpu_winsys_bo);
   if (!bo)
            simple_mtx_init(&bo->lock, mtx_plain);
   pipe_reference_init(&bo->base.reference, 1);
   bo->base.alignment_log2 = util_logbase2(RADEON_SPARSE_PAGE_SIZE);
   bo->base.size = size;
   bo->base.vtbl = &amdgpu_winsys_bo_sparse_vtbl;
   bo->base.placement = domain;
   bo->unique_id =  __sync_fetch_and_add(&ws->next_bo_unique_id, 1);
            bo->u.sparse.num_va_pages = DIV_ROUND_UP(size, RADEON_SPARSE_PAGE_SIZE);
   bo->u.sparse.commitments = CALLOC(bo->u.sparse.num_va_pages,
         if (!bo->u.sparse.commitments)
                     /* For simplicity, we always map a multiple of the page size. */
   map_size = align64(size, RADEON_SPARSE_PAGE_SIZE);
   va_gap_size = ws->check_vm ? 4 * RADEON_SPARSE_PAGE_SIZE : 0;
   r = amdgpu_va_range_alloc(ws->dev, amdgpu_gpu_va_range_general,
                     if (r)
            r = amdgpu_bo_va_op_raw(ws->dev, NULL, 0, map_size, bo->va,
         if (r)
                  error_va_map:
         error_va_alloc:
         error_alloc_commitments:
      simple_mtx_destroy(&bo->lock);
   FREE(bo);
      }
      static bool
   amdgpu_bo_sparse_commit(struct radeon_winsys *rws, struct pb_buffer *buf,
         {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   struct amdgpu_winsys_bo *bo = amdgpu_winsys_bo(buf);
   struct amdgpu_sparse_commitment *comm;
   uint32_t va_page, end_va_page;
   bool ok = true;
            assert(bo->base.usage & RADEON_FLAG_SPARSE);
   assert(offset % RADEON_SPARSE_PAGE_SIZE == 0);
   assert(offset <= bo->base.size);
   assert(size <= bo->base.size - offset);
            comm = bo->u.sparse.commitments;
   va_page = offset / RADEON_SPARSE_PAGE_SIZE;
                  #if DEBUG_SPARSE_COMMITS
         #endif
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
   backing = sparse_backing_alloc(ws, bo, &backing_start, &backing_size);
   if (!backing) {
                        r = amdgpu_bo_va_op_raw(ws->dev, backing->bo->bo,
                           (uint64_t)backing_start * RADEON_SPARSE_PAGE_SIZE,
   (uint64_t)backing_size * RADEON_SPARSE_PAGE_SIZE,
   bo->va + (uint64_t)span_va_page * RADEON_SPARSE_PAGE_SIZE,
   if (r) {
                                       while (backing_size) {
      comm[span_va_page].backing = backing;
   comm[span_va_page].page = backing_start;
   span_va_page++;
   backing_start++;
               } else {
      r = amdgpu_bo_va_op_raw(ws->dev, NULL, 0,
                     if (r) {
      ok = false;
               while (va_page < end_va_page) {
      struct amdgpu_sparse_backing *backing;
                  /* Skip pages that are already uncommitted. */
   if (!comm[va_page].backing) {
      va_page++;
               /* Group contiguous spans of pages. */
   backing = comm[va_page].backing;
                                 while (va_page < end_va_page &&
         comm[va_page].backing == backing &&
      comm[va_page].backing = NULL;
   va_page++;
               if (!sparse_backing_free(ws, bo, backing, backing_start, span_pages)) {
      /* Couldn't allocate tracking data structures, so we have to leak */
   fprintf(stderr, "amdgpu: leaking PRT backing memory\n");
               out:
                     }
      static unsigned
   amdgpu_bo_find_next_committed_memory(struct pb_buffer *buf,
         {
      struct amdgpu_winsys_bo *bo = amdgpu_winsys_bo(buf);
   struct amdgpu_sparse_commitment *comm;
   uint32_t va_page, end_va_page;
   uint32_t span_va_page, start_va_page;
            if (*range_size == 0)
                     uncommitted_range_prev = uncommitted_range_next = 0;
   comm = bo->u.sparse.commitments;
   start_va_page = va_page = range_offset / RADEON_SPARSE_PAGE_SIZE;
            simple_mtx_lock(&bo->lock);
   /* Lookup the first committed page with backing physical storage */
   while (va_page < end_va_page && !comm[va_page].backing)
            /* Fisrt committed page lookup failed, return early. */
   if (va_page == end_va_page && !comm[va_page].backing) {
      uncommitted_range_prev = *range_size;
   *range_size = 0;
   simple_mtx_unlock(&bo->lock);
               /* Lookup the first uncommitted page without backing physical storage */
   span_va_page = va_page;
   while (va_page < end_va_page && comm[va_page].backing)
                  /* Calc byte count that need to skip before committed range */
   if (span_va_page != start_va_page)
            /* Calc byte count that need to skip after committed range */
   if (va_page != end_va_page || !comm[va_page].backing) {
                  /* Calc size of first committed part */
   *range_size = *range_size - uncommitted_range_next - uncommitted_range_prev;
   return *range_size ? uncommitted_range_prev
      }
      static void amdgpu_buffer_get_metadata(struct radeon_winsys *rws,
                     {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   struct amdgpu_winsys_bo *bo = amdgpu_winsys_bo(_buf);
   struct amdgpu_bo_info info = {0};
                     r = amdgpu_bo_query_info(bo->bo, &info);
   if (r)
            ac_surface_apply_bo_metadata(&ws->info, surf, info.metadata.tiling_info,
            md->size_metadata = info.metadata.size_metadata;
      }
      static void amdgpu_buffer_set_metadata(struct radeon_winsys *rws,
                     {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   struct amdgpu_winsys_bo *bo = amdgpu_winsys_bo(_buf);
                              metadata.size_metadata = md->size_metadata;
               }
      struct pb_buffer *
   amdgpu_bo_create(struct amdgpu_winsys *ws,
                  uint64_t size,
      {
                        /* Handle sparse buffers first. */
   if (flags & RADEON_FLAG_SPARSE) {
                           struct pb_slabs *last_slab = &ws->bo_slabs[NUM_SLAB_ALLOCATORS - 1];
   unsigned max_slab_entry_size = 1 << (last_slab->min_order + last_slab->num_orders - 1);
            /* Sub-allocate small buffers from slabs. */
   if (heap >= 0 && size <= max_slab_entry_size) {
      struct pb_slab_entry *entry;
            /* Always use slabs for sizes less than 4 KB because the kernel aligns
   * everything to 4 KB.
   */
   if (size < alignment && alignment <= 4 * 1024)
            if (alignment > get_slab_entry_alignment(ws, alloc_size)) {
      /* 3/4 allocations can return too small alignment. Try again with a power of two
   * allocation size.
                  if (alignment <= pot_size) {
      /* This size works but wastes some memory to fulfil the alignment. */
      } else {
                     struct pb_slabs *slabs = get_slabs(ws, alloc_size);
   entry = pb_slab_alloc(slabs, alloc_size, heap);
   if (!entry) {
                        }
   if (!entry)
            bo = container_of(entry, struct amdgpu_winsys_bo, u.slab.entry);
   pipe_reference_init(&bo->base.reference, 1);
   bo->base.size = size;
            if (domain & RADEON_DOMAIN_VRAM)
         else
                  no_slab:
         /* Align size to page size. This is the minimum alignment for normal
   * BOs. Aligning this here helps the cached bufmgr. Especially small BOs,
   * like constant/uniform buffers, can benefit from better and more reuse.
   */
   if (domain & RADEON_DOMAIN_VRAM_GTT) {
      size = align64(size, ws->info.gart_page_size);
               bool use_reusable_pool = flags & RADEON_FLAG_NO_INTERPROCESS_SHARING &&
            if (use_reusable_pool) {
      /* RADEON_FLAG_NO_SUBALLOC is irrelevant for the cache. */
   heap = radeon_get_heap_index(domain, flags & ~RADEON_FLAG_NO_SUBALLOC);
            /* Get a buffer from the cache. */
   bo = (struct amdgpu_winsys_bo*)
         if (bo)
               /* Create a new one. */
   bo = amdgpu_create_bo(ws, size, alignment, domain, flags, heap);
   if (!bo) {
      /* Clean up buffer managers and try again. */
            bo = amdgpu_create_bo(ws, size, alignment, domain, flags, heap);
   if (!bo)
                  }
      static struct pb_buffer *
   amdgpu_buffer_create(struct radeon_winsys *ws,
                           {
      struct pb_buffer * res = amdgpu_bo_create(amdgpu_winsys(ws), size, alignment, domain,
            }
      static struct pb_buffer *amdgpu_bo_from_handle(struct radeon_winsys *rws,
                     {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   struct amdgpu_winsys_bo *bo = NULL;
   enum amdgpu_bo_handle_type type;
   struct amdgpu_bo_import_result result = {0};
   uint64_t va;
   amdgpu_va_handle va_handle = NULL;
   struct amdgpu_bo_info info = {0};
   enum radeon_bo_domain initial = 0;
   enum radeon_bo_flag flags = 0;
            switch (whandle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
      type = amdgpu_bo_handle_type_gem_flink_name;
      case WINSYS_HANDLE_TYPE_FD:
      type = amdgpu_bo_handle_type_dma_buf_fd;
      default:
                  r = amdgpu_bo_import(ws->dev, type, whandle->handle, &result);
   if (r)
            simple_mtx_lock(&ws->bo_export_table_lock);
            /* If the amdgpu_winsys_bo instance already exists, bump the reference
   * counter and return it.
   */
   if (bo) {
      p_atomic_inc(&bo->base.reference.count);
            /* Release the buffer handle, because we don't need it anymore.
   * This function is returning an existing buffer, which has its own
   * handle.
   */
   amdgpu_bo_free(result.buf_handle);
               /* Get initial domains. */
   r = amdgpu_bo_query_info(result.buf_handle, &info);
   if (r)
            r = amdgpu_va_range_alloc(ws->dev, amdgpu_gpu_va_range_general,
                           if (r)
            bo = CALLOC_STRUCT(amdgpu_winsys_bo);
   if (!bo)
            r = amdgpu_bo_va_op_raw(ws->dev, result.buf_handle, 0, result.alloc_size, va,
                           if (r)
            if (info.preferred_heap & AMDGPU_GEM_DOMAIN_VRAM)
         if (info.preferred_heap & AMDGPU_GEM_DOMAIN_GTT)
         if (info.alloc_flags & AMDGPU_GEM_CREATE_NO_CPU_ACCESS)
         if (info.alloc_flags & AMDGPU_GEM_CREATE_CPU_GTT_USWC)
         if (info.alloc_flags & AMDGPU_GEM_CREATE_ENCRYPTED) {
      /* Imports are always possible even if the importer isn't using TMZ.
   * For instance libweston needs to import the buffer to be able to determine
   * if it can be used for scanout.
   */
   flags |= RADEON_FLAG_ENCRYPTED;
               /* Initialize the structure. */
   simple_mtx_init(&bo->lock, mtx_plain);
   pipe_reference_init(&bo->base.reference, 1);
   bo->base.alignment_log2 = util_logbase2(info.phys_alignment ?
   info.phys_alignment : ws->info.gart_page_size);
   bo->bo = result.buf_handle;
   bo->base.size = result.alloc_size;
   bo->base.vtbl = &amdgpu_winsys_bo_vtbl;
   bo->va = va;
   bo->u.real.va_handle = va_handle;
   bo->base.placement = initial;
   bo->base.usage = flags;
   bo->unique_id = __sync_fetch_and_add(&ws->next_bo_unique_id, 1);
            if (bo->base.placement & RADEON_DOMAIN_VRAM)
         else if (bo->base.placement & RADEON_DOMAIN_GTT)
                              _mesa_hash_table_insert(ws->bo_export_table, bo->bo, bo);
                  error:
      simple_mtx_unlock(&ws->bo_export_table_lock);
   if (bo)
         if (va_handle)
         amdgpu_bo_free(result.buf_handle);
      }
      static bool amdgpu_bo_get_handle(struct radeon_winsys *rws,
               {
      struct amdgpu_screen_winsys *sws = amdgpu_screen_winsys(rws);
   struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   struct amdgpu_winsys_bo *bo = amdgpu_winsys_bo(buffer);
   enum amdgpu_bo_handle_type type;
   struct hash_entry *entry;
            /* Don't allow exports of slab entries and sparse buffers. */
   if (!bo->bo)
                     switch (whandle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
      type = amdgpu_bo_handle_type_gem_flink_name;
      case WINSYS_HANDLE_TYPE_KMS:
      if (sws->fd == ws->fd) {
                                          simple_mtx_lock(&ws->sws_list_lock);
   entry = _mesa_hash_table_search(sws->kms_handles, bo);
   simple_mtx_unlock(&ws->sws_list_lock);
   if (entry) {
      whandle->handle = (uintptr_t)entry->data;
      }
      case WINSYS_HANDLE_TYPE_FD:
      type = amdgpu_bo_handle_type_dma_buf_fd;
      default:
                  r = amdgpu_bo_export(bo->bo, type, &whandle->handle);
   if (r)
         #if defined(DMA_BUF_SET_NAME_B)
      if (whandle->type == WINSYS_HANDLE_TYPE_FD &&
      !bo->u.real.is_shared) {
   char dmabufname[32];
   snprintf(dmabufname, 32, "%d-%s", getpid(), util_get_process_name());
         #endif
         if (whandle->type == WINSYS_HANDLE_TYPE_KMS) {
               r = drmPrimeFDToHandle(sws->fd, dma_fd, &whandle->handle);
            if (r)
            simple_mtx_lock(&ws->sws_list_lock);
   _mesa_hash_table_insert_pre_hashed(sws->kms_handles,
                        hash_table_set:
      simple_mtx_lock(&ws->bo_export_table_lock);
   _mesa_hash_table_insert(ws->bo_export_table, bo->bo, bo);
            bo->u.real.is_shared = true;
      }
      static struct pb_buffer *amdgpu_bo_from_ptr(struct radeon_winsys *rws,
               {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   amdgpu_bo_handle buf_handle;
   struct amdgpu_winsys_bo *bo;
   uint64_t va;
   amdgpu_va_handle va_handle;
   /* Avoid failure when the size is not page aligned */
            bo = CALLOC_STRUCT(amdgpu_winsys_bo);
   if (!bo)
            if (amdgpu_create_bo_from_user_mem(ws->dev, pointer,
                  if (amdgpu_va_range_alloc(ws->dev, amdgpu_gpu_va_range_general,
                                    if (amdgpu_bo_va_op(buf_handle, 0, aligned_size, va, 0, AMDGPU_VA_OP_MAP))
            /* Initialize it. */
   bo->u.real.is_user_ptr = true;
   pipe_reference_init(&bo->base.reference, 1);
   simple_mtx_init(&bo->lock, mtx_plain);
   bo->bo = buf_handle;
   bo->base.alignment_log2 = 0;
   bo->base.size = size;
   bo->base.vtbl = &amdgpu_winsys_bo_vtbl;
   bo->u.real.cpu_ptr = pointer;
   bo->va = va;
   bo->u.real.va_handle = va_handle;
   bo->base.placement = RADEON_DOMAIN_GTT;
                                             error_va_map:
            error_va_alloc:
            error:
      FREE(bo);
      }
      static bool amdgpu_bo_is_user_ptr(struct pb_buffer *buf)
   {
                  }
      static bool amdgpu_bo_is_suballocated(struct pb_buffer *buf)
   {
                  }
      static uint64_t amdgpu_bo_get_va(struct pb_buffer *buf)
   {
         }
      void amdgpu_bo_init_functions(struct amdgpu_screen_winsys *ws)
   {
      ws->base.buffer_set_metadata = amdgpu_buffer_set_metadata;
   ws->base.buffer_get_metadata = amdgpu_buffer_get_metadata;
   ws->base.buffer_map = amdgpu_bo_map;
   ws->base.buffer_unmap = amdgpu_bo_unmap;
   ws->base.buffer_wait = amdgpu_bo_wait;
   ws->base.buffer_create = amdgpu_buffer_create;
   ws->base.buffer_from_handle = amdgpu_bo_from_handle;
   ws->base.buffer_from_ptr = amdgpu_bo_from_ptr;
   ws->base.buffer_is_user_ptr = amdgpu_bo_is_user_ptr;
   ws->base.buffer_is_suballocated = amdgpu_bo_is_suballocated;
   ws->base.buffer_get_handle = amdgpu_bo_get_handle;
   ws->base.buffer_commit = amdgpu_bo_sparse_commit;
   ws->base.buffer_find_next_committed_memory = amdgpu_bo_find_next_committed_memory;
   ws->base.buffer_get_virtual_address = amdgpu_bo_get_va;
   ws->base.buffer_get_initial_domain = amdgpu_bo_get_initial_domain;
      }
