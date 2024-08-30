   /*
   * Copyright (C) 2012-2018 Rob Clark <robclark@freedesktop.org>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "util/os_mman.h"
      #include "freedreno_drmif.h"
   #include "freedreno_priv.h"
      simple_mtx_t table_lock = SIMPLE_MTX_INITIALIZER;
   simple_mtx_t fence_lock = SIMPLE_MTX_INITIALIZER;
      /* set buffer name, and add to table, call w/ table_lock held: */
   static void
   set_name(struct fd_bo *bo, uint32_t name)
   {
      bo->name = name;
   /* add ourself into the handle table: */
      }
      static struct fd_bo zombie;
      /* lookup a buffer, call w/ table_lock held: */
   static struct fd_bo *
   lookup_bo(struct hash_table *tbl, uint32_t key)
   {
      struct fd_bo *bo = NULL;
                     entry = _mesa_hash_table_search(tbl, &key);
   if (entry) {
               /* We could be racing with final unref in another thread, and won
   * the table_lock preventing the other thread from being able to
   * remove an object it is about to free.  Fortunately since table
   * lookup and removal are protected by the same lock (and table
   * removal happens before obj free) we can easily detect this by
   * checking for refcnt==0 (ie. 1 after p_atomic_inc_return).
   */
   if (p_atomic_inc_return(&bo->refcnt) == 1) {
      /* Restore the zombified reference count, so if another thread
   * that ends up calling lookup_bo() gets the table_lock before
   * the thread deleting the bo does, it doesn't mistakenly see
   * that the BO is live.
   *
   * We are holding the table_lock here so we can't be racing
   * with another caller of lookup_bo()
   */
   p_atomic_dec(&bo->refcnt);
               if (!list_is_empty(&bo->node)) {
      mesa_logw("bo was in cache, size=%u, alloc_flags=0x%x\n",
               /* don't break the bucket if this bo was found in one */
      }
      }
      void
   fd_bo_init_common(struct fd_bo *bo, struct fd_device *dev)
   {
      /* Backend should have initialized these: */
   assert(bo->size);
   assert(bo->handle);
            bo->dev = dev;
   bo->iova = bo->funcs->iova(bo);
            p_atomic_set(&bo->refcnt, 1);
            bo->max_fences = 1;
               }
      /* allocate a new buffer object, call w/ table_lock held */
   static struct fd_bo *
   import_bo_from_handle(struct fd_device *dev, uint32_t size, uint32_t handle)
   {
                        bo = dev->funcs->bo_from_handle(dev, size, handle);
   if (!bo) {
      struct drm_gem_close req = {
         };
   drmIoctl(dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
                        /* add ourself into the handle table: */
               }
      static struct fd_bo *
   bo_new(struct fd_device *dev, uint32_t size, uint32_t flags,
         {
               if (size < FD_BO_HEAP_BLOCK_SIZE) {
      if ((flags == 0) && dev->default_heap)
         if ((flags == RING_FLAGS) && dev->ring_heap)
               /* demote cached-coherent to WC if not supported: */
   if ((flags & FD_BO_CACHED_COHERENT) && !dev->has_cached_coherent)
            bo = fd_bo_cache_alloc(cache, &size, flags);
   if (bo)
            bo = dev->funcs->bo_new(dev, size, flags);
   if (!bo)
            simple_mtx_lock(&table_lock);
   /* add ourself into the handle table: */
   _mesa_hash_table_insert(dev->handle_table, &bo->handle, bo);
                        }
      struct fd_bo *
   _fd_bo_new(struct fd_device *dev, uint32_t size, uint32_t flags)
   {
      struct fd_bo *bo = bo_new(dev, size, flags, &dev->bo_cache);
   if (bo)
            }
      void
   _fd_bo_set_name(struct fd_bo *bo, const char *fmt, va_list ap)
   {
         }
      /* internal function to allocate bo's that use the ringbuffer cache
   * instead of the normal bo_cache.  The purpose is, because cmdstream
   * bo's get vmap'd on the kernel side, and that is expensive, we want
   * to re-use cmdstream bo's for cmdstream and not unrelated purposes.
   */
   struct fd_bo *
   fd_bo_new_ring(struct fd_device *dev, uint32_t size)
   {
      struct fd_bo *bo = bo_new(dev, size, RING_FLAGS, &dev->ring_cache);
   if (bo) {
      bo->bo_reuse = RING_CACHE;
   bo->reloc_flags |= FD_RELOC_DUMP;
      }
      }
      struct fd_bo *
   fd_bo_from_handle(struct fd_device *dev, uint32_t handle, uint32_t size)
   {
                        bo = lookup_bo(dev->handle_table, handle);
   if (bo)
                           out_unlock:
               /* We've raced with the handle being closed, so the handle is no longer
   * valid.  Friends don't let friends share handles.
   */
   if (bo == &zombie)
               }
      struct fd_bo *
   fd_bo_from_dmabuf(struct fd_device *dev, int fd)
   {
      int ret, size;
   uint32_t handle;
         restart:
      simple_mtx_lock(&table_lock);
   ret = drmPrimeFDToHandle(dev->fd, fd, &handle);
   if (ret) {
      simple_mtx_unlock(&table_lock);
               bo = lookup_bo(dev->handle_table, handle);
   if (bo)
            /* lseek() to get bo size */
   size = lseek(fd, 0, SEEK_END);
                           out_unlock:
               if (bo == &zombie)
               }
      struct fd_bo *
   fd_bo_from_name(struct fd_device *dev, uint32_t name)
   {
      struct drm_gem_open req = {
         };
                     /* check name table first, to see if bo is already open: */
   bo = lookup_bo(dev->name_table, name);
   if (bo)
         restart:
      if (drmIoctl(dev->fd, DRM_IOCTL_GEM_OPEN, &req)) {
      ERROR_MSG("gem-open failed: %s", strerror(errno));
               bo = lookup_bo(dev->handle_table, req.handle);
   if (bo)
            bo = import_bo_from_handle(dev, req.size, req.handle);
   if (bo) {
      set_name(bo, name);
            out_unlock:
               if (bo == &zombie)
               }
      void
   fd_bo_mark_for_dump(struct fd_bo *bo)
   {
         }
      struct fd_bo *
   fd_bo_ref(struct fd_bo *bo)
   {
      ref(&bo->refcnt);
      }
      static void
   bo_finalize(struct fd_bo *bo)
   {
      if (bo->funcs->finalize)
      }
      static void
   dev_flush(struct fd_device *dev)
   {
      if (dev->funcs->flush)
      }
      static void
   bo_del(struct fd_bo *bo)
   {
         }
      static bool
   try_recycle(struct fd_bo *bo)
   {
               /* No point in BO cache for suballocated buffers: */
   if (suballoc_bo(bo))
            if (bo->bo_reuse == BO_CACHE)
            if (bo->bo_reuse == RING_CACHE)
               }
      void
   fd_bo_del(struct fd_bo *bo)
   {
      if (!unref(&bo->refcnt))
            if (try_recycle(bo))
                     bo_finalize(bo);
   dev_flush(dev);
      }
      void
   fd_bo_del_array(struct fd_bo **bos, int count)
   {
      if (!count)
                     /*
   * First pass, remove objects from the table that either (a) still have
   * a live reference, or (b) no longer have a reference but are released
   * to the BO cache:
            for (int i = 0; i < count; i++) {
      if (!unref(&bos[i]->refcnt) || try_recycle(bos[i])) {
         } else {
      /* We are going to delete this one, so finalize it first: */
                           /*
   * Second pass, delete all of the objects remaining after first pass.
            for (int i = 0; i < count; i++) {
            }
      /**
   * Special interface for fd_bo_cache to batch delete a list of handles.
   * Similar to fd_bo_del_array() but bypasses the BO cache (since it is
   * called from the BO cache to expire a list of BOs).
   */
   void
   fd_bo_del_list_nocache(struct list_head *list)
   {
      if (list_is_empty(list))
                     foreach_bo (bo, list) {
                           foreach_bo_safe (bo, list) {
      assert(bo->refcnt == 0);
         }
      void
   fd_bo_fini_fences(struct fd_bo *bo)
   {
      for (int i = 0; i < bo->nr_fences; i++)
            if (bo->fences != &bo->_inline_fence)
      }
      /**
   * Helper called by backends bo->funcs->destroy()
   *
   * Called under table_lock, bo_del_flush() *must* be called before
   * table_lock is released (but bo->funcs->destroy() can be called
   * multiple times before bo_del_flush(), as long as table_lock is
   * held the entire time)
   */
   void
   fd_bo_fini_common(struct fd_bo *bo)
   {
      struct fd_device *dev = bo->dev;
                              if (bo->map)
            if (handle) {
      simple_mtx_lock(&table_lock);
   struct drm_gem_close req = {
         };
   drmIoctl(dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
   _mesa_hash_table_remove_key(dev->handle_table, &handle);
   if (bo->name)
                        }
      static void
   bo_flush(struct fd_bo *bo)
   {
               simple_mtx_lock(&fence_lock);
   unsigned nr = bo->nr_fences;
   struct fd_fence *fences[nr];
   for (unsigned i = 0; i < nr; i++)
                  for (unsigned i = 0; i < nr; i++) {
      fd_fence_flush(bo->fences[i]);
         }
      int
   fd_bo_get_name(struct fd_bo *bo, uint32_t *name)
   {
      if (suballoc_bo(bo))
            if (!bo->name) {
      struct drm_gem_flink req = {
         };
            ret = drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_FLINK, &req);
   if (ret) {
                  simple_mtx_lock(&table_lock);
   set_name(bo, req.name);
   simple_mtx_unlock(&table_lock);
   bo->bo_reuse = NO_CACHE;
   bo->alloc_flags |= FD_BO_SHARED;
                           }
      uint32_t
   fd_bo_handle(struct fd_bo *bo)
   {
      if (suballoc_bo(bo))
         bo->bo_reuse = NO_CACHE;
   bo->alloc_flags |= FD_BO_SHARED;
   bo_flush(bo);
      }
      int
   fd_bo_dmabuf(struct fd_bo *bo)
   {
               if (suballoc_bo(bo))
            ret = drmPrimeHandleToFD(bo->dev->fd, bo->handle, DRM_CLOEXEC | DRM_RDWR,
         if (ret) {
      ERROR_MSG("failed to get dmabuf fd: %d", ret);
               bo->bo_reuse = NO_CACHE;
   bo->alloc_flags |= FD_BO_SHARED;
               }
      uint32_t
   fd_bo_size(struct fd_bo *bo)
   {
         }
      bool
   fd_bo_is_cached(struct fd_bo *bo)
   {
         }
      static void *
   bo_map(struct fd_bo *bo)
   {
      if (!bo->map) {
      uint64_t offset;
            ret = bo->funcs->offset(bo, &offset);
   if (ret) {
                  bo->map = os_mmap(0, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
         if (bo->map == MAP_FAILED) {
      ERROR_MSG("mmap failed: %s", strerror(errno));
         }
      }
      void *
   fd_bo_map(struct fd_bo *bo)
   {
      /* don't allow mmap'ing something allocated with FD_BO_NOMAP
   * for sanity
   */
   if (bo->alloc_flags & FD_BO_NOMAP)
               }
      void
   fd_bo_upload(struct fd_bo *bo, void *src, unsigned off, unsigned len)
   {
      if (bo->funcs->upload) {
      bo->funcs->upload(bo, src, off, len);
                  }
      bool
   fd_bo_prefer_upload(struct fd_bo *bo, unsigned len)
   {
      if (bo->funcs->prefer_upload)
               }
      /* a bit odd to take the pipe as an arg, but it's a, umm, quirk of kgsl.. */
   int
   fd_bo_cpu_prep(struct fd_bo *bo, struct fd_pipe *pipe, uint32_t op)
   {
               if (state == FD_BO_STATE_IDLE)
                     if (op & (FD_BO_PREP_NOSYNC | FD_BO_PREP_FLUSH)) {
      if (op & FD_BO_PREP_FLUSH)
            /* If we have *only* been asked to flush, then we aren't really
   * interested about whether shared buffers are busy, so avoid
   * the kernel ioctl.
   */
   if ((state == FD_BO_STATE_BUSY) ||
      (op == FD_BO_PREP_FLUSH))
            /* In case the bo is referenced by a deferred submit, flush up to the
   * required fence now:
   */
            /* FD_BO_PREP_FLUSH is purely a frontend flag, and is not seen/handled
   * by backend or kernel:
   */
            if (!op)
            /* Wait on fences.. first grab a reference under the fence lock, and then
   * wait and drop ref.
   */
   simple_mtx_lock(&fence_lock);
   unsigned nr = bo->nr_fences;
   struct fd_fence *fences[nr];
   for (unsigned i = 0; i < nr; i++)
                  for (unsigned i = 0; i < nr; i++) {
      fd_fence_wait(fences[i]);
               /* expire completed fences */
            /* None shared buffers will not have any external usage (ie. fences
   * that we are not aware of) so nothing more to do.
   */
   if (!(bo->alloc_flags & FD_BO_SHARED))
            /* If buffer is shared, but we are using explicit sync, no need to
   * fallback to implicit sync:
   */
   if (pipe && pipe->no_implicit_sync)
               }
      /**
   * Cleanup fences, dropping pipe references.  If 'expired' is true, only
   * cleanup expired fences.
   *
   * Normally we expect at most a single fence, the exception being bo's
   * shared between contexts
   */
   static void
   cleanup_fences(struct fd_bo *bo)
   {
               for (int i = 0; i < bo->nr_fences; i++) {
               if (fd_fence_before(f->pipe->control->fence, f->ufence))
                     if (bo->nr_fences > 0) {
      /* Shuffle up the last entry to replace the current slot: */
   bo->fences[i] = bo->fences[bo->nr_fences];
                     }
      void
   fd_bo_add_fence(struct fd_bo *bo, struct fd_fence *fence)
   {
               if (bo->alloc_flags & _FD_BO_NOSYNC)
            /* The common case is bo re-used on the same pipe it had previously
   * been used on, so just replace the previous fence.
   */
   for (int i = 0; i < bo->nr_fences; i++) {
      struct fd_fence *f = bo->fences[i];
   if (f == fence)
         if (f->pipe == fence->pipe) {
      assert(fd_fence_before(f->ufence, fence->ufence));
   fd_fence_del_locked(f);
   bo->fences[i] = fd_fence_ref_locked(fence);
                           /* The first time we grow past a single fence, we need some special
   * handling, as we've been using the embedded _inline_fence to avoid
   * a separate allocation:
   */
   if (unlikely((bo->nr_fences == 1) &&
            bo->nr_fences = bo->max_fences = 0;
   bo->fences = NULL;
                  }
      enum fd_bo_state
   fd_bo_state(struct fd_bo *bo)
   {
      /* NOTE: check the nosync case before touching fence_lock in case we end
   * up here recursively from dropping pipe reference in cleanup_fences().
   * The pipe's control buffer is specifically nosync to avoid recursive
   * lock problems here.
   */
   if (bo->alloc_flags & (FD_BO_SHARED | _FD_BO_NOSYNC))
            /* Speculatively check, if we already know we're idle, no need to acquire
   * lock and do the cleanup_fences() dance:
   */
   if (!bo->nr_fences)
            simple_mtx_lock(&fence_lock);
   cleanup_fences(bo);
            if (!bo->nr_fences)
               }
   