   /*
   * Copyright 2019 Collabora, Ltd.
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
   * Authors (Collabora):
   *   Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   */
   #include <errno.h>
   #include <fcntl.h>
   #include <pthread.h>
   #include <stdio.h>
   #include <xf86drm.h>
   #include "drm-uapi/panfrost_drm.h"
      #include "pan_bo.h"
   #include "pan_device.h"
   #include "pan_util.h"
   #include "wrap.h"
      #include "util/os_mman.h"
      #include "util/u_inlines.h"
   #include "util/u_math.h"
      /* This file implements a userspace BO cache. Allocating and freeing
   * GPU-visible buffers is very expensive, and even the extra kernel roundtrips
   * adds more work than we would like at this point. So caching BOs in userspace
   * solves both of these problems and does not require kernel updates.
   *
   * Cached BOs are sorted into a bucket based on rounding their size down to the
   * nearest power-of-two. Each bucket contains a linked list of free panfrost_bo
   * objects. Putting a BO into the cache is accomplished by adding it to the
   * corresponding bucket. Getting a BO from the cache consists of finding the
   * appropriate bucket and sorting. A cache eviction is a kernel-level free of a
   * BO and removing it from the bucket. We special case evicting all BOs from
   * the cache, since that's what helpful in practice and avoids extra logic
   * around the linked list.
   */
      static struct panfrost_bo *
   panfrost_bo_alloc(struct panfrost_device *dev, size_t size, uint32_t flags,
         {
      struct drm_panfrost_create_bo create_bo = {.size = size};
   struct panfrost_bo *bo;
            if (dev->kernel_version->version_major > 1 ||
      dev->kernel_version->version_minor >= 1) {
   if (flags & PAN_BO_GROWABLE)
         if (!(flags & PAN_BO_EXECUTE))
               ret = drmIoctl(dev->fd, DRM_IOCTL_PANFROST_CREATE_BO, &create_bo);
   if (ret) {
      fprintf(stderr, "DRM_IOCTL_PANFROST_CREATE_BO failed: %m\n");
               bo = pan_lookup_bo(dev, create_bo.handle);
            bo->size = create_bo.size;
   bo->ptr.gpu = create_bo.offset;
   bo->gem_handle = create_bo.handle;
   bo->flags = flags;
   bo->dev = dev;
   bo->label = label;
      }
      static void
   panfrost_bo_free(struct panfrost_bo *bo)
   {
      struct drm_gem_close gem_close = {.handle = bo->gem_handle};
   int fd = bo->dev->fd;
            /* BO will be freed with the sparse array, but zero to indicate free */
            ret = drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
   if (ret) {
      fprintf(stderr, "DRM_IOCTL_GEM_CLOSE failed: %m\n");
         }
      /* Returns true if the BO is ready, false otherwise.
   * access_type is encoding the type of access one wants to ensure is done.
   * Waiting is always done for writers, but if wait_readers is set then readers
   * are also waited for.
   */
   bool
   panfrost_bo_wait(struct panfrost_bo *bo, int64_t timeout_ns, bool wait_readers)
   {
      struct drm_panfrost_wait_bo req = {
      .handle = bo->gem_handle,
      };
            /* If the BO has been exported or imported we can't rely on the cached
   * state, we need to call the WAIT_BO ioctl.
   */
   if (!(bo->flags & PAN_BO_SHARED)) {
      /* If ->gpu_access is 0, the BO is idle, no need to wait. */
   if (!bo->gpu_access)
            /* If the caller only wants to wait for writers and no
   * writes are pending, we don't have to wait.
   */
   if (!wait_readers && !(bo->gpu_access & PAN_BO_ACCESS_WRITE))
               /* The ioctl returns >= 0 value when the BO we are waiting for is ready
   * -1 otherwise.
   */
   ret = drmIoctl(bo->dev->fd, DRM_IOCTL_PANFROST_WAIT_BO, &req);
   if (ret != -1) {
      /* Set gpu_access to 0 so that the next call to bo_wait()
   * doesn't have to call the WAIT_BO ioctl.
   */
   bo->gpu_access = 0;
               /* If errno is not ETIMEDOUT or EBUSY that means the handle we passed
   * is invalid, which shouldn't happen here.
   */
   assert(errno == ETIMEDOUT || errno == EBUSY);
      }
      /* Helper to calculate the bucket index of a BO */
      static unsigned
   pan_bucket_index(unsigned size)
   {
                        /* Clamp the bucket index; all huge allocations will be
                     /* Reindex from 0 */
      }
      static struct list_head *
   pan_bucket(struct panfrost_device *dev, unsigned size)
   {
         }
      /* Tries to fetch a BO of sufficient size with the appropriate flags from the
   * BO cache. If it succeeds, it returns that BO and removes the BO from the
   * cache. If it fails, it returns NULL signaling the caller to allocate a new
   * BO. */
      static struct panfrost_bo *
   panfrost_bo_cache_fetch(struct panfrost_device *dev, size_t size,
         {
      pthread_mutex_lock(&dev->bo_cache.lock);
   struct list_head *bucket = pan_bucket(dev, size);
            /* Iterate the bucket looking for something suitable */
   list_for_each_entry_safe(struct panfrost_bo, entry, bucket, bucket_link) {
      if (entry->size < size || entry->flags != flags)
            /* If the oldest BO in the cache is busy, likely so is
   * everything newer, so bail. */
   if (!panfrost_bo_wait(entry, dontwait ? 0 : INT64_MAX, PAN_BO_ACCESS_RW))
            struct drm_panfrost_madvise madv = {
      .handle = entry->gem_handle,
      };
            /* This one works, splice it out of the cache */
   list_del(&entry->bucket_link);
            ret = drmIoctl(dev->fd, DRM_IOCTL_PANFROST_MADVISE, &madv);
   if (!ret && !madv.retained) {
      panfrost_bo_free(entry);
      }
   /* Let's go! */
   bo = entry;
   bo->label = label;
      }
               }
      static void
   panfrost_bo_cache_evict_stale_bos(struct panfrost_device *dev)
   {
               clock_gettime(CLOCK_MONOTONIC, &time);
   list_for_each_entry_safe(struct panfrost_bo, entry, &dev->bo_cache.lru,
            /* We want all entries that have been used more than 1 sec
   * ago to be dropped, others can be kept.
   * Note the <= 2 check and not <= 1. It's here to account for
   * the fact that we're only testing ->tv_sec, not ->tv_nsec.
   * That means we might keep entries that are between 1 and 2
   * seconds old, but we don't really care, as long as unused BOs
   * are dropped at some point.
   */
   if (time.tv_sec - entry->last_used <= 2)
            list_del(&entry->bucket_link);
   list_del(&entry->lru_link);
         }
      /* Tries to add a BO to the cache. Returns if it was
   * successful */
      static bool
   panfrost_bo_cache_put(struct panfrost_bo *bo)
   {
               if (bo->flags & PAN_BO_SHARED || dev->debug & PAN_DBG_NO_CACHE)
            /* Must be first */
            struct list_head *bucket = pan_bucket(dev, MAX2(bo->size, 4096));
   struct drm_panfrost_madvise madv;
            madv.handle = bo->gem_handle;
   madv.madv = PANFROST_MADV_DONTNEED;
                     /* Add us to the bucket */
            /* Add us to the LRU list and update the last_used field. */
   list_addtail(&bo->lru_link, &dev->bo_cache.lru);
   clock_gettime(CLOCK_MONOTONIC, &time);
            /* Let's do some cleanup in the BO cache while we hold the
   * lock.
   */
            /* Update the label to help debug BO cache memory usage issues */
            /* Must be last */
   pthread_mutex_unlock(&dev->bo_cache.lock);
      }
      /* Evicts all BOs from the cache. Called during context
   * destroy or during low-memory situations (to free up
   * memory that may be unused by us just sitting in our
   * cache, but still reserved from the perspective of the
   * OS) */
      void
   panfrost_bo_cache_evict_all(struct panfrost_device *dev)
   {
      pthread_mutex_lock(&dev->bo_cache.lock);
   for (unsigned i = 0; i < ARRAY_SIZE(dev->bo_cache.buckets); ++i) {
               list_for_each_entry_safe(struct panfrost_bo, entry, bucket, bucket_link) {
      list_del(&entry->bucket_link);
   list_del(&entry->lru_link);
         }
      }
      void
   panfrost_bo_mmap(struct panfrost_bo *bo)
   {
      struct drm_panfrost_mmap_bo mmap_bo = {.handle = bo->gem_handle};
            if (bo->ptr.cpu)
            ret = drmIoctl(bo->dev->fd, DRM_IOCTL_PANFROST_MMAP_BO, &mmap_bo);
   if (ret) {
      fprintf(stderr, "DRM_IOCTL_PANFROST_MMAP_BO failed: %m\n");
               bo->ptr.cpu = os_mmap(NULL, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
         if (bo->ptr.cpu == MAP_FAILED) {
      bo->ptr.cpu = NULL;
   fprintf(stderr,
         "mmap failed: result=%p size=0x%llx fd=%i offset=0x%llx %m\n",
         }
      static void
   panfrost_bo_munmap(struct panfrost_bo *bo)
   {
      if (!bo->ptr.cpu)
            if (os_munmap((void *)(uintptr_t)bo->ptr.cpu, bo->size)) {
      perror("munmap");
                  }
      struct panfrost_bo *
   panfrost_bo_create(struct panfrost_device *dev, size_t size, uint32_t flags,
         {
               /* Kernel will fail (confusingly) with EPERM otherwise */
            /* To maximize BO cache usage, don't allocate tiny BOs */
            /* GROWABLE BOs cannot be mmapped */
   if (flags & PAN_BO_GROWABLE)
            /* Ideally, we get a BO that's ready in the cache, or allocate a fresh
   * BO. If allocation fails, we can try waiting for something in the
   * cache. But if there's no nothing suitable, we should flush the cache
   * to make space for the new allocation.
   */
   bo = panfrost_bo_cache_fetch(dev, size, flags, label, true);
   if (!bo)
         if (!bo)
         if (!bo) {
      panfrost_bo_cache_evict_all(dev);
               if (!bo) {
      unreachable("BO creation failed. We don't handle that yet.");
               /* Only mmap now if we know we need to. For CPU-invisible buffers, we
   * never map since we don't care about their contents; they're purely
            if (!(flags & (PAN_BO_INVISIBLE | PAN_BO_DELAY_MMAP)))
                     if (dev->debug & (PAN_DBG_TRACE | PAN_DBG_SYNC)) {
      if (flags & PAN_BO_INVISIBLE)
      pandecode_inject_mmap(dev->decode_ctx, bo->ptr.gpu, NULL, bo->size,
      else if (!(flags & PAN_BO_DELAY_MMAP))
      pandecode_inject_mmap(dev->decode_ctx, bo->ptr.gpu, bo->ptr.cpu,
               }
      void
   panfrost_bo_reference(struct panfrost_bo *bo)
   {
      if (bo) {
      ASSERTED int count = p_atomic_inc_return(&bo->refcnt);
         }
      void
   panfrost_bo_unreference(struct panfrost_bo *bo)
   {
      if (!bo)
            /* Don't return to cache if there are still references */
   if (p_atomic_dec_return(&bo->refcnt))
                              /* Someone might have imported this BO while we were waiting for the
   * lock, let's make sure it's still not referenced before freeing it.
   */
   if (p_atomic_read(&bo->refcnt) == 0) {
      /* When the reference count goes to zero, we need to cleanup */
            if (dev->debug & (PAN_DBG_TRACE | PAN_DBG_SYNC))
            /* Rather than freeing the BO now, we'll cache the BO for later
   * allocations if we're allowed to.
   */
   if (!panfrost_bo_cache_put(bo))
      }
      }
      struct panfrost_bo *
   panfrost_bo_import(struct panfrost_device *dev, int fd)
   {
      struct panfrost_bo *bo;
   struct drm_panfrost_get_bo_offset get_bo_offset = {
         };
   ASSERTED int ret;
                     ret = drmPrimeFDToHandle(dev->fd, fd, &gem_handle);
                     if (!bo->dev) {
      get_bo_offset.handle = gem_handle;
   ret = drmIoctl(dev->fd, DRM_IOCTL_PANFROST_GET_BO_OFFSET, &get_bo_offset);
            bo->dev = dev;
   bo->ptr.gpu = (mali_ptr)get_bo_offset.offset;
   bo->size = lseek(fd, 0, SEEK_END);
   /* Sometimes this can fail and return -1. size of -1 is not
   * a nice thing for mmap to try mmap. Be more robust also
   * for zero sized maps and fail nicely too
   */
   if ((bo->size == 0) || (bo->size == (size_t)-1)) {
      pthread_mutex_unlock(&dev->bo_map_lock);
      }
   bo->flags = PAN_BO_SHARED;
   bo->gem_handle = gem_handle;
      } else {
      /* bo->refcnt == 0 can happen if the BO
   * was being released but panfrost_bo_import() acquired the
   * lock before panfrost_bo_unreference(). In that case, refcnt
   * is 0 and we can't use panfrost_bo_reference() directly, we
   * have to re-initialize the refcnt().
   * Note that panfrost_bo_unreference() checks
   * refcnt value just after acquiring the lock to
   * make sure the object is not freed if panfrost_bo_import()
   * acquired it in the meantime.
   */
   if (p_atomic_read(&bo->refcnt) == 0)
         else
      }
               }
      int
   panfrost_bo_export(struct panfrost_bo *bo)
   {
      struct drm_prime_handle args = {
      .handle = bo->gem_handle,
               int ret = drmIoctl(bo->dev->fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args);
   if (ret == -1)
            bo->flags |= PAN_BO_SHARED;
      }
