   /*
   * Copyright © 2011 Marek Olšák <maraeo@gmail.com>
   *
   * SPDX-License-Identifier: MIT
   */
      #include "radeon_drm_cs.h"
      #include "util/u_hash_table.h"
   #include "util/u_memory.h"
   #include "util/u_thread.h"
   #include "util/os_mman.h"
   #include "util/os_time.h"
      #include "frontend/drm_driver.h"
      #include <sys/ioctl.h>
   #include <xf86drm.h>
   #include <errno.h>
   #include <fcntl.h>
   #include <stdio.h>
   #include <inttypes.h>
      static struct pb_buffer *
   radeon_winsys_bo_create(struct radeon_winsys *rws,
                              static inline struct radeon_bo *radeon_bo(struct pb_buffer *bo)
   {
         }
      struct radeon_bo_va_hole {
      struct list_head list;
   uint64_t         offset;
      };
      static bool radeon_real_bo_is_busy(struct radeon_bo *bo)
   {
               args.handle = bo->handle;
   return drmCommandWriteRead(bo->rws->fd, DRM_RADEON_GEM_BUSY,
      }
      static bool radeon_bo_is_busy(struct radeon_bo *bo)
   {
      unsigned num_idle;
            if (bo->handle)
            mtx_lock(&bo->rws->bo_fence_lock);
   for (num_idle = 0; num_idle < bo->u.slab.num_fences; ++num_idle) {
      if (radeon_real_bo_is_busy(bo->u.slab.fences[num_idle])) {
      busy = true;
      }
      }
   memmove(&bo->u.slab.fences[0], &bo->u.slab.fences[num_idle],
         bo->u.slab.num_fences -= num_idle;
               }
      static void radeon_real_bo_wait_idle(struct radeon_bo *bo)
   {
               args.handle = bo->handle;
   while (drmCommandWrite(bo->rws->fd, DRM_RADEON_GEM_WAIT_IDLE,
      }
      static void radeon_bo_wait_idle(struct radeon_bo *bo)
   {
      if (bo->handle) {
         } else {
      mtx_lock(&bo->rws->bo_fence_lock);
   while (bo->u.slab.num_fences) {
      struct radeon_bo *fence = NULL;
                                 mtx_lock(&bo->rws->bo_fence_lock);
   if (bo->u.slab.num_fences && fence == bo->u.slab.fences[0]) {
      radeon_ws_bo_reference(&bo->u.slab.fences[0], NULL);
   memmove(&bo->u.slab.fences[0], &bo->u.slab.fences[1],
            }
      }
         }
      static bool radeon_bo_wait(struct radeon_winsys *rws,
               {
      struct radeon_bo *bo = radeon_bo(_buf);
            /* No timeout. Just query. */
   if (timeout == 0)
                     /* Wait if any ioctl is being submitted with this buffer. */
   if (!os_wait_until_zero_abs_timeout(&bo->num_active_ioctls, abs_timeout))
            /* Infinite timeout. */
   if (abs_timeout == OS_TIMEOUT_INFINITE) {
      radeon_bo_wait_idle(bo);
               /* Other timeouts need to be emulated with a loop. */
   while (radeon_bo_is_busy(bo)) {
      if (os_time_get_nano() >= abs_timeout)
                        }
      static enum radeon_bo_domain get_valid_domain(enum radeon_bo_domain domain)
   {
      /* Zero domains the driver doesn't understand. */
            /* If no domain is set, we must set something... */
   if (!domain)
               }
      static enum radeon_bo_domain radeon_bo_get_initial_domain(
         {
      struct radeon_bo *bo = (struct radeon_bo*)buf;
            memset(&args, 0, sizeof(args));
   args.handle = bo->handle;
            if (drmCommandWriteRead(bo->rws->fd, DRM_RADEON_GEM_OP,
            fprintf(stderr, "radeon: failed to get initial domain: %p 0x%08X\n",
         /* Default domain as returned by get_valid_domain. */
               /* GEM domains and winsys domains are defined the same. */
      }
      static uint64_t radeon_bomgr_find_va(const struct radeon_info *info,
               {
      struct radeon_bo_va_hole *hole, *n;
            /* All VM address space holes will implicitly start aligned to the
   * size alignment, so we don't need to sanitize the alignment here
   */
            mtx_lock(&heap->mutex);
   /* first look for a hole */
   LIST_FOR_EACH_ENTRY_SAFE(hole, n, &heap->holes, list) {
      offset = hole->offset;
   waste = offset % alignment;
   waste = waste ? alignment - waste : 0;
   offset += waste;
   if (offset >= (hole->offset + hole->size)) {
         }
   if (!waste && hole->size == size) {
      offset = hole->offset;
   list_del(&hole->list);
   FREE(hole);
   mtx_unlock(&heap->mutex);
      }
   if ((hole->size - waste) > size) {
      if (waste) {
      n = CALLOC_STRUCT(radeon_bo_va_hole);
   n->size = waste;
   n->offset = hole->offset;
      }
   hole->size -= (size + waste);
   hole->offset += size + waste;
   mtx_unlock(&heap->mutex);
      }
   if ((hole->size - waste) == size) {
      hole->size = waste;
   mtx_unlock(&heap->mutex);
                  offset = heap->start;
   waste = offset % alignment;
            if (offset + waste + size > heap->end) {
      mtx_unlock(&heap->mutex);
               if (waste) {
      n = CALLOC_STRUCT(radeon_bo_va_hole);
   n->size = waste;
   n->offset = offset;
      }
   offset += waste;
   heap->start += size + waste;
   mtx_unlock(&heap->mutex);
      }
      static uint64_t radeon_bomgr_find_va64(struct radeon_drm_winsys *ws,
         {
               /* Try to allocate from the 64-bit address space first.
   * If it doesn't exist (start = 0) or if it doesn't have enough space,
   * fall back to the 32-bit address space.
   */
   if (ws->vm64.start)
         if (!va)
            }
      static void radeon_bomgr_free_va(const struct radeon_info *info,
               {
                        mtx_lock(&heap->mutex);
   if ((va + size) == heap->start) {
      heap->start = va;
   /* Delete uppermost hole if it reaches the new top */
   if (!list_is_empty(&heap->holes)) {
      hole = container_of(heap->holes.next, struct radeon_bo_va_hole, list);
   if ((hole->offset + hole->size) == va) {
      heap->start = hole->offset;
   list_del(&hole->list);
            } else {
               hole = container_of(&heap->holes, struct radeon_bo_va_hole, list);
   LIST_FOR_EACH_ENTRY(next, &heap->holes, list) {
      if (next->offset < va)
                     if (&hole->list != &heap->holes) {
      /* Grow upper hole if it's adjacent */
   if (hole->offset == (va + size)) {
      hole->offset = va;
   hole->size += size;
   /* Merge lower hole if it's adjacent */
   if (next != hole && &next->list != &heap->holes &&
      (next->offset + next->size) == va) {
   next->size += hole->size;
   list_del(&hole->list);
      }
                  /* Grow lower hole if it's adjacent */
   if (next != hole && &next->list != &heap->holes &&
      (next->offset + next->size) == va) {
   next->size += size;
               /* FIXME on allocation failure we just lose virtual address space
   * maybe print a warning
   */
   next = CALLOC_STRUCT(radeon_bo_va_hole);
   if (next) {
      next->size = size;
   next->offset = va;
            out:
         }
      void radeon_bo_destroy(void *winsys, struct pb_buffer *_buf)
   {
      struct radeon_bo *bo = radeon_bo(_buf);
   struct radeon_drm_winsys *rws = bo->rws;
                              mtx_lock(&rws->bo_handles_mutex);
   /* radeon_winsys_bo_from_handle might have revived the bo */
   if (pipe_is_referenced(&bo->base.reference)) {
      mtx_unlock(&rws->bo_handles_mutex);
      }
   _mesa_hash_table_remove_key(rws->bo_handles, (void*)(uintptr_t)bo->handle);
   if (bo->flink_name) {
      _mesa_hash_table_remove_key(rws->bo_names,
      }
            if (bo->u.real.ptr)
            if (rws->info.r600_has_virtual_memory) {
      if (rws->va_unmap_working) {
               va.handle = bo->handle;
   va.vm_id = 0;
   va.operation = RADEON_VA_UNMAP;
   va.flags = RADEON_VM_PAGE_READABLE |
                        if (drmCommandWriteRead(rws->fd, DRM_RADEON_GEM_VA, &va,
            va.operation == RADEON_VA_RESULT_ERROR) {
   fprintf(stderr, "radeon: Failed to deallocate virtual address for buffer:\n");
   fprintf(stderr, "radeon:    size      : %"PRIu64" bytes\n", bo->base.size);
                  radeon_bomgr_free_va(&rws->info,
                     /* Close object. */
   args.handle = bo->handle;
                     if (bo->initial_domain & RADEON_DOMAIN_VRAM)
         else if (bo->initial_domain & RADEON_DOMAIN_GTT)
            if (bo->u.real.map_count >= 1) {
      if (bo->initial_domain & RADEON_DOMAIN_VRAM)
         else
                        }
      static void radeon_bo_destroy_or_cache(void *winsys, struct pb_buffer *_buf)
   {
                        if (bo->u.real.use_reusable_pool)
         else
      }
      void *radeon_bo_do_map(struct radeon_bo *bo)
   {
      struct drm_radeon_gem_mmap args = {0};
   void *ptr;
            /* If the buffer is created from user memory, return the user pointer. */
   if (bo->user_ptr)
            if (bo->handle) {
         } else {
      offset = bo->va - bo->u.slab.real->va;
               /* Map the buffer. */
   mtx_lock(&bo->u.real.map_mutex);
   /* Return the pointer if it's already mapped. */
   if (bo->u.real.ptr) {
      bo->u.real.map_count++;
   mtx_unlock(&bo->u.real.map_mutex);
      }
   args.handle = bo->handle;
   args.offset = 0;
   args.size = (uint64_t)bo->base.size;
   if (drmCommandWriteRead(bo->rws->fd,
                        mtx_unlock(&bo->u.real.map_mutex);
   fprintf(stderr, "radeon: gem_mmap failed: %p 0x%08X\n",
                     ptr = os_mmap(0, args.size, PROT_READ|PROT_WRITE, MAP_SHARED,
         if (ptr == MAP_FAILED) {
      /* Clear the cache and try again. */
            ptr = os_mmap(0, args.size, PROT_READ|PROT_WRITE, MAP_SHARED,
         if (ptr == MAP_FAILED) {
      mtx_unlock(&bo->u.real.map_mutex);
   fprintf(stderr, "radeon: mmap failed, errno: %i\n", errno);
         }
   bo->u.real.ptr = ptr;
            if (bo->initial_domain & RADEON_DOMAIN_VRAM)
         else
                  mtx_unlock(&bo->u.real.map_mutex);
      }
      static void *radeon_bo_map(struct radeon_winsys *rws,
                     {
      struct radeon_bo *bo = (struct radeon_bo*)buf;
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
   if (cs && radeon_bo_is_referenced_by_cs_for_write(cs, bo)) {
      cs->flush_cs(cs->flush_data,
                     if (!radeon_bo_wait(rws, (struct pb_buffer*)bo, 0,
                  } else {
      if (cs && radeon_bo_is_referenced_by_cs(cs, bo)) {
      cs->flush_cs(cs->flush_data,
                     if (!radeon_bo_wait(rws, (struct pb_buffer*)bo, 0,
                     } else {
               if (!(usage & PIPE_MAP_WRITE)) {
      /* Mapping for read.
   *
   * Since we are mapping for read, we don't need to wait
   * if the GPU is using the buffer for read too
   * (neither one is changing it).
   *
   * Only check whether the buffer is being used for write. */
   if (cs && radeon_bo_is_referenced_by_cs_for_write(cs, bo)) {
      cs->flush_cs(cs->flush_data,
      }
   radeon_bo_wait(rws, (struct pb_buffer*)bo, OS_TIMEOUT_INFINITE,
      } else {
      /* Mapping for write. */
   if (cs) {
      if (radeon_bo_is_referenced_by_cs(cs, bo)) {
      cs->flush_cs(cs->flush_data,
      } else {
      /* Try to avoid busy-waiting in radeon_bo_wait. */
   if (p_atomic_read(&bo->num_active_ioctls))
                  radeon_bo_wait(rws, (struct pb_buffer*)bo, OS_TIMEOUT_INFINITE,
                                 }
      static void radeon_bo_unmap(struct radeon_winsys *rws, struct pb_buffer *_buf)
   {
               if (bo->user_ptr)
            if (!bo->handle)
            mtx_lock(&bo->u.real.map_mutex);
   if (!bo->u.real.ptr) {
      mtx_unlock(&bo->u.real.map_mutex);
               assert(bo->u.real.map_count);
   if (--bo->u.real.map_count) {
      mtx_unlock(&bo->u.real.map_mutex);
               os_munmap(bo->u.real.ptr, bo->base.size);
            if (bo->initial_domain & RADEON_DOMAIN_VRAM)
         else
                     }
      static const struct pb_vtbl radeon_bo_vtbl = {
      radeon_bo_destroy_or_cache
      };
      static struct radeon_bo *radeon_create_bo(struct radeon_drm_winsys *rws,
                           {
      struct radeon_bo *bo;
   struct drm_radeon_gem_create args;
                     assert(initial_domains);
   assert((initial_domains &
            args.size = size;
   args.alignment = alignment;
   args.initial_domain = initial_domains;
            /* If VRAM is just stolen system memory, allow both VRAM and
   * GTT, whichever has free space. If a buffer is evicted from
   * VRAM to GTT, it will stay there.
   */
   if (!rws->info.has_dedicated_vram)
            if (flags & RADEON_FLAG_GTT_WC)
         if (flags & RADEON_FLAG_NO_CPU_ACCESS)
            if (drmCommandWriteRead(rws->fd, DRM_RADEON_GEM_CREATE,
            fprintf(stderr, "radeon: Failed to allocate a buffer:\n");
   fprintf(stderr, "radeon:    size      : %u bytes\n", size);
   fprintf(stderr, "radeon:    alignment : %u bytes\n", alignment);
   fprintf(stderr, "radeon:    domains   : %u\n", args.initial_domain);
   fprintf(stderr, "radeon:    flags     : %u\n", args.flags);
                        bo = CALLOC_STRUCT(radeon_bo);
   if (!bo)
            pipe_reference_init(&bo->base.reference, 1);
   bo->base.alignment_log2 = util_logbase2(alignment);
   bo->base.usage = 0;
   bo->base.size = size;
   bo->base.vtbl = &radeon_bo_vtbl;
   bo->rws = rws;
   bo->handle = args.handle;
   bo->va = 0;
   bo->initial_domain = initial_domains;
   bo->hash = __sync_fetch_and_add(&rws->next_bo_hash, 1);
            if (heap >= 0) {
      pb_cache_init_entry(&rws->bo_cache, &bo->u.real.cache_entry, &bo->base,
               if (rws->info.r600_has_virtual_memory) {
      struct drm_radeon_gem_va va;
                     if (flags & RADEON_FLAG_32BIT) {
      bo->va = radeon_bomgr_find_va(&rws->info, &rws->vm32,
            } else {
                  va.handle = bo->handle;
   va.vm_id = 0;
   va.operation = RADEON_VA_MAP;
   va.flags = RADEON_VM_PAGE_READABLE |
               va.offset = bo->va;
   r = drmCommandWriteRead(rws->fd, DRM_RADEON_GEM_VA, &va, sizeof(va));
   if (r && va.operation == RADEON_VA_RESULT_ERROR) {
      fprintf(stderr, "radeon: Failed to allocate virtual address for buffer:\n");
   fprintf(stderr, "radeon:    size      : %d bytes\n", size);
   fprintf(stderr, "radeon:    alignment : %d bytes\n", alignment);
   fprintf(stderr, "radeon:    domains   : %d\n", args.initial_domain);
   fprintf(stderr, "radeon:    va        : 0x%016llx\n", (unsigned long long)bo->va);
   radeon_bo_destroy(NULL, &bo->base);
      }
   mtx_lock(&rws->bo_handles_mutex);
   if (va.operation == RADEON_VA_RESULT_VA_EXIST) {
      struct pb_buffer *b = &bo->base;
                  mtx_unlock(&rws->bo_handles_mutex);
   pb_reference(&b, &old_bo->base);
               _mesa_hash_table_u64_insert(rws->bo_vas, bo->va, bo);
               if (initial_domains & RADEON_DOMAIN_VRAM)
         else if (initial_domains & RADEON_DOMAIN_GTT)
               }
      bool radeon_bo_can_reclaim(void *winsys, struct pb_buffer *_buf)
   {
               if (radeon_bo_is_referenced_by_any_cs(bo))
               }
      bool radeon_bo_can_reclaim_slab(void *priv, struct pb_slab_entry *entry)
   {
                  }
      static void radeon_bo_slab_destroy(void *winsys, struct pb_buffer *_buf)
   {
                           }
      static const struct pb_vtbl radeon_winsys_bo_slab_vtbl = {
      radeon_bo_slab_destroy
      };
      struct pb_slab *radeon_bo_slab_alloc(void *priv, unsigned heap,
               {
      struct radeon_drm_winsys *ws = priv;
   struct radeon_slab *slab = CALLOC_STRUCT(radeon_slab);
   enum radeon_bo_domain domains = radeon_domain_from_heap(heap);
   enum radeon_bo_flag flags = radeon_flags_from_heap(heap);
            if (!slab)
            slab->buffer = radeon_bo(radeon_winsys_bo_create(&ws->base,
               if (!slab->buffer)
                     slab->base.num_entries = slab->buffer->base.size / entry_size;
   slab->base.num_free = slab->base.num_entries;
   slab->entries = CALLOC(slab->base.num_entries, sizeof(*slab->entries));
   if (!slab->entries)
                              for (unsigned i = 0; i < slab->base.num_entries; ++i) {
               bo->base.alignment_log2 = util_logbase2(entry_size);
   bo->base.usage = slab->buffer->base.usage;
   bo->base.size = entry_size;
   bo->base.vtbl = &radeon_winsys_bo_slab_vtbl;
   bo->rws = ws;
   bo->va = slab->buffer->va + i * entry_size;
   bo->initial_domain = domains;
   bo->hash = base_hash + i;
   bo->u.slab.entry.slab = &slab->base;
   bo->u.slab.entry.group_index = group_index;
   bo->u.slab.entry.entry_size = entry_size;
                              fail_buffer:
         fail:
      FREE(slab);
      }
      void radeon_bo_slab_free(void *priv, struct pb_slab *pslab)
   {
               for (unsigned i = 0; i < slab->base.num_entries; ++i) {
      struct radeon_bo *bo = &slab->entries[i];
   for (unsigned j = 0; j < bo->u.slab.num_fences; ++j)
                     FREE(slab->entries);
   radeon_ws_bo_reference(&slab->buffer, NULL);
      }
      static unsigned eg_tile_split(unsigned tile_split)
   {
      switch (tile_split) {
   case 0:     tile_split = 64;    break;
   case 1:     tile_split = 128;   break;
   case 2:     tile_split = 256;   break;
   case 3:     tile_split = 512;   break;
   default:
   case 4:     tile_split = 1024;  break;
   case 5:     tile_split = 2048;  break;
   case 6:     tile_split = 4096;  break;
   }
      }
      static unsigned eg_tile_split_rev(unsigned eg_tile_split)
   {
      switch (eg_tile_split) {
   case 64:    return 0;
   case 128:   return 1;
   case 256:   return 2;
   case 512:   return 3;
   default:
   case 1024:  return 4;
   case 2048:  return 5;
   case 4096:  return 6;
      }
      static void radeon_bo_get_metadata(struct radeon_winsys *rws,
                     {
      struct radeon_bo *bo = radeon_bo(_buf);
                                       drmCommandWriteRead(bo->rws->fd,
                        if (surf) {
      if (args.tiling_flags & RADEON_TILING_MACRO)
         else if (args.tiling_flags & RADEON_TILING_MICRO)
         else
            surf->u.legacy.bankw = (args.tiling_flags >> RADEON_TILING_EG_BANKW_SHIFT) & RADEON_TILING_EG_BANKW_MASK;
   surf->u.legacy.bankh = (args.tiling_flags >> RADEON_TILING_EG_BANKH_SHIFT) & RADEON_TILING_EG_BANKH_MASK;
   surf->u.legacy.tile_split = (args.tiling_flags >> RADEON_TILING_EG_TILE_SPLIT_SHIFT) & RADEON_TILING_EG_TILE_SPLIT_MASK;
   surf->u.legacy.tile_split = eg_tile_split(surf->u.legacy.tile_split);
            if (bo->rws->gen >= DRV_SI && !(args.tiling_flags & RADEON_TILING_R600_NO_SCANOUT))
         else
                     md->u.legacy.microtile = RADEON_LAYOUT_LINEAR;
   md->u.legacy.macrotile = RADEON_LAYOUT_LINEAR;
   if (args.tiling_flags & RADEON_TILING_MICRO)
         else if (args.tiling_flags & RADEON_TILING_MICRO_SQUARE)
            if (args.tiling_flags & RADEON_TILING_MACRO)
            md->u.legacy.bankw = (args.tiling_flags >> RADEON_TILING_EG_BANKW_SHIFT) & RADEON_TILING_EG_BANKW_MASK;
   md->u.legacy.bankh = (args.tiling_flags >> RADEON_TILING_EG_BANKH_SHIFT) & RADEON_TILING_EG_BANKH_MASK;
   md->u.legacy.tile_split = (args.tiling_flags >> RADEON_TILING_EG_TILE_SPLIT_SHIFT) & RADEON_TILING_EG_TILE_SPLIT_MASK;
   md->u.legacy.mtilea = (args.tiling_flags >> RADEON_TILING_EG_MACRO_TILE_ASPECT_SHIFT) & RADEON_TILING_EG_MACRO_TILE_ASPECT_MASK;
   md->u.legacy.tile_split = eg_tile_split(md->u.legacy.tile_split);
      }
      static void radeon_bo_set_metadata(struct radeon_winsys *rws,
                     {
      struct radeon_bo *bo = radeon_bo(_buf);
                                       if (surf) {
      if (surf->u.legacy.level[0].mode >= RADEON_SURF_MODE_1D)
         if (surf->u.legacy.level[0].mode >= RADEON_SURF_MODE_2D)
            args.tiling_flags |= (surf->u.legacy.bankw & RADEON_TILING_EG_BANKW_MASK) <<
         args.tiling_flags |= (surf->u.legacy.bankh & RADEON_TILING_EG_BANKH_MASK) <<
         if (surf->u.legacy.tile_split) {
      args.tiling_flags |= (eg_tile_split_rev(surf->u.legacy.tile_split) &
            }
   args.tiling_flags |= (surf->u.legacy.mtilea & RADEON_TILING_EG_MACRO_TILE_ASPECT_MASK) <<
            if (bo->rws->gen >= DRV_SI && !(surf->flags & RADEON_SURF_SCANOUT))
               } else {
      if (md->u.legacy.microtile == RADEON_LAYOUT_TILED)
         else if (md->u.legacy.microtile == RADEON_LAYOUT_SQUARETILED)
            if (md->u.legacy.macrotile == RADEON_LAYOUT_TILED)
            args.tiling_flags |= (md->u.legacy.bankw & RADEON_TILING_EG_BANKW_MASK) <<
         args.tiling_flags |= (md->u.legacy.bankh & RADEON_TILING_EG_BANKH_MASK) <<
         if (md->u.legacy.tile_split) {
      args.tiling_flags |= (eg_tile_split_rev(md->u.legacy.tile_split) &
            }
   args.tiling_flags |= (md->u.legacy.mtilea & RADEON_TILING_EG_MACRO_TILE_ASPECT_MASK) <<
            if (bo->rws->gen >= DRV_SI && !md->u.legacy.scanout)
                                 drmCommandWriteRead(bo->rws->fd,
                  }
      static struct pb_buffer *
   radeon_winsys_bo_create(struct radeon_winsys *rws,
                           {
      struct radeon_drm_winsys *ws = radeon_drm_winsys(rws);
                              /* Only 32-bit sizes are supported. */
   if (size > UINT_MAX)
                     /* Sub-allocate small buffers from slabs. */
   if (heap >= 0 &&
      size <= (1 << RADEON_SLAB_MAX_SIZE_LOG2) &&
   ws->info.r600_has_virtual_memory &&
   alignment <= MAX2(1 << RADEON_SLAB_MIN_SIZE_LOG2, util_next_power_of_two(size))) {
            entry = pb_slab_alloc(&ws->bo_slabs, size, heap);
   if (!entry) {
                        }
   if (!entry)
                                          /* Align size to page size. This is the minimum alignment for normal
   * BOs. Aligning this here helps the cached bufmgr. Especially small BOs,
   * like constant/uniform buffers, can benefit from better and more reuse.
   */
   size = align(size, ws->info.gart_page_size);
            bool use_reusable_pool = flags & RADEON_FLAG_NO_INTERPROCESS_SHARING &&
            /* Shared resources don't use cached heaps. */
   if (use_reusable_pool) {
      /* RADEON_FLAG_NO_SUBALLOC is irrelevant for the cache. */
   heap = radeon_get_heap_index(domain, flags & ~RADEON_FLAG_NO_SUBALLOC);
            bo = radeon_bo(pb_cache_reclaim_buffer(&ws->bo_cache, size, alignment,
         if (bo)
               bo = radeon_create_bo(ws, size, alignment, domain, flags, heap);
   if (!bo) {
      /* Clear the cache and try again. */
   if (ws->info.r600_has_virtual_memory)
         pb_cache_release_all_buffers(&ws->bo_cache);
   bo = radeon_create_bo(ws, size, alignment, domain, flags, heap);
   if (!bo)
                        mtx_lock(&ws->bo_handles_mutex);
   _mesa_hash_table_insert(ws->bo_handles, (void*)(uintptr_t)bo->handle, bo);
               }
      static struct pb_buffer *radeon_winsys_bo_from_ptr(struct radeon_winsys *rws,
               {
      struct radeon_drm_winsys *ws = radeon_drm_winsys(rws);
   struct drm_radeon_gem_userptr args;
   struct radeon_bo *bo;
            bo = CALLOC_STRUCT(radeon_bo);
   if (!bo)
            memset(&args, 0, sizeof(args));
   args.addr = (uintptr_t)pointer;
            if (flags & RADEON_FLAG_READ_ONLY)
      args.flags = RADEON_GEM_USERPTR_READONLY |
      else
      args.flags = RADEON_GEM_USERPTR_ANONONLY |
               if (drmCommandWriteRead(ws->fd, DRM_RADEON_GEM_USERPTR,
            FREE(bo);
                                 /* Initialize it. */
   pipe_reference_init(&bo->base.reference, 1);
   bo->handle = args.handle;
   bo->base.alignment_log2 = 0;
   bo->base.size = size;
   bo->base.vtbl = &radeon_bo_vtbl;
   bo->rws = ws;
   bo->user_ptr = pointer;
   bo->va = 0;
   bo->initial_domain = RADEON_DOMAIN_GTT;
   bo->hash = __sync_fetch_and_add(&ws->next_bo_hash, 1);
                              if (ws->info.r600_has_virtual_memory) {
                        va.handle = bo->handle;
   va.operation = RADEON_VA_MAP;
   va.vm_id = 0;
   va.offset = bo->va;
   va.flags = RADEON_VM_PAGE_READABLE |
               va.offset = bo->va;
   r = drmCommandWriteRead(ws->fd, DRM_RADEON_GEM_VA, &va, sizeof(va));
   if (r && va.operation == RADEON_VA_RESULT_ERROR) {
      fprintf(stderr, "radeon: Failed to assign virtual address space\n");
   radeon_bo_destroy(NULL, &bo->base);
      }
   mtx_lock(&ws->bo_handles_mutex);
   if (va.operation == RADEON_VA_RESULT_VA_EXIST) {
      struct pb_buffer *b = &bo->base;
                  mtx_unlock(&ws->bo_handles_mutex);
   pb_reference(&b, &old_bo->base);
               _mesa_hash_table_u64_insert(ws->bo_vas, bo->va, bo);
                           }
      static struct pb_buffer *radeon_winsys_bo_from_handle(struct radeon_winsys *rws,
                     {
      struct radeon_drm_winsys *ws = radeon_drm_winsys(rws);
   struct radeon_bo *bo;
   int r;
   unsigned handle;
            /* We must maintain a list of pairs <handle, bo>, so that we always return
   * the same BO for one particular handle. If we didn't do that and created
   * more than one BO for the same handle and then relocated them in a CS,
   * we would hit a deadlock in the kernel.
   *
   * The list of pairs is guarded by a mutex, of course. */
            if (whandle->type == WINSYS_HANDLE_TYPE_SHARED) {
      /* First check if there already is an existing bo for the handle. */
      } else if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
      /* We must first get the GEM handle, as fds are unreliable keys */
   r = drmPrimeFDToHandle(ws->fd, whandle->handle, &handle);
   if (r)
            } else {
      /* Unknown handle type */
               if (bo) {
      /* Increase the refcount. */
   p_atomic_inc(&bo->base.reference.count);
               /* There isn't, create a new one. */
   bo = CALLOC_STRUCT(radeon_bo);
   if (!bo) {
                  if (whandle->type == WINSYS_HANDLE_TYPE_SHARED) {
      struct drm_gem_open open_arg = {};
   memset(&open_arg, 0, sizeof(open_arg));
   /* Open the BO. */
   open_arg.name = whandle->handle;
   if (drmIoctl(ws->fd, DRM_IOCTL_GEM_OPEN, &open_arg)) {
      FREE(bo);
      }
   handle = open_arg.handle;
   size = open_arg.size;
      } else if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
      size = lseek(whandle->handle, 0, SEEK_END);
   /*
   * Could check errno to determine whether the kernel is new enough, but
   * it doesn't really matter why this failed, just that it failed.
   */
   if (size == (off_t)-1) {
      FREE(bo);
      }
                                 /* Initialize it. */
   pipe_reference_init(&bo->base.reference, 1);
   bo->base.alignment_log2 = 0;
   bo->base.size = (unsigned) size;
   bo->base.vtbl = &radeon_bo_vtbl;
   bo->rws = ws;
   bo->va = 0;
   bo->hash = __sync_fetch_and_add(&ws->next_bo_hash, 1);
            if (bo->flink_name)
                  done:
               if (ws->info.r600_has_virtual_memory && !bo->va) {
                        va.handle = bo->handle;
   va.operation = RADEON_VA_MAP;
   va.vm_id = 0;
   va.offset = bo->va;
   va.flags = RADEON_VM_PAGE_READABLE |
               va.offset = bo->va;
   r = drmCommandWriteRead(ws->fd, DRM_RADEON_GEM_VA, &va, sizeof(va));
   if (r && va.operation == RADEON_VA_RESULT_ERROR) {
      fprintf(stderr, "radeon: Failed to assign virtual address space\n");
   radeon_bo_destroy(NULL, &bo->base);
      }
   mtx_lock(&ws->bo_handles_mutex);
   if (va.operation == RADEON_VA_RESULT_VA_EXIST) {
      struct pb_buffer *b = &bo->base;
                  mtx_unlock(&ws->bo_handles_mutex);
   pb_reference(&b, &old_bo->base);
               _mesa_hash_table_u64_insert(ws->bo_vas, bo->va, bo);
                        if (bo->initial_domain & RADEON_DOMAIN_VRAM)
         else if (bo->initial_domain & RADEON_DOMAIN_GTT)
                  fail:
      mtx_unlock(&ws->bo_handles_mutex);
      }
      static bool radeon_winsys_bo_get_handle(struct radeon_winsys *rws,
               {
      struct drm_gem_flink flink;
   struct radeon_bo *bo = radeon_bo(buffer);
            /* Don't allow exports of slab entries. */
   if (!bo->handle)
                              if (whandle->type == WINSYS_HANDLE_TYPE_SHARED) {
      if (!bo->flink_name) {
               if (ioctl(ws->fd, DRM_IOCTL_GEM_FLINK, &flink)) {
                           mtx_lock(&ws->bo_handles_mutex);
   _mesa_hash_table_insert(ws->bo_names, (void*)(uintptr_t)bo->flink_name, bo);
      }
      } else if (whandle->type == WINSYS_HANDLE_TYPE_KMS) {
         } else if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
      if (drmPrimeHandleToFD(ws->fd, bo->handle, DRM_CLOEXEC, (int*)&whandle->handle))
                  }
      static bool radeon_winsys_bo_is_user_ptr(struct pb_buffer *buf)
   {
         }
      static bool radeon_winsys_bo_is_suballocated(struct pb_buffer *buf)
   {
         }
      static uint64_t radeon_winsys_bo_va(struct pb_buffer *buf)
   {
         }
      static unsigned radeon_winsys_bo_get_reloc_offset(struct pb_buffer *buf)
   {
               if (bo->handle)
               }
      void radeon_drm_bo_init_functions(struct radeon_drm_winsys *ws)
   {
      ws->base.buffer_set_metadata = radeon_bo_set_metadata;
   ws->base.buffer_get_metadata = radeon_bo_get_metadata;
   ws->base.buffer_map = radeon_bo_map;
   ws->base.buffer_unmap = radeon_bo_unmap;
   ws->base.buffer_wait = radeon_bo_wait;
   ws->base.buffer_create = radeon_winsys_bo_create;
   ws->base.buffer_from_handle = radeon_winsys_bo_from_handle;
   ws->base.buffer_from_ptr = radeon_winsys_bo_from_ptr;
   ws->base.buffer_is_user_ptr = radeon_winsys_bo_is_user_ptr;
   ws->base.buffer_is_suballocated = radeon_winsys_bo_is_suballocated;
   ws->base.buffer_get_handle = radeon_winsys_bo_get_handle;
   ws->base.buffer_get_virtual_address = radeon_winsys_bo_va;
   ws->base.buffer_get_reloc_offset = radeon_winsys_bo_get_reloc_offset;
      }
