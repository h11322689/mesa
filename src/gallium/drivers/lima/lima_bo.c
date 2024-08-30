   /*
   * Copyright (C) 2017-2019 Lima Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   *
   */
      #include <stdlib.h>
   #include <sys/types.h>
   #include <unistd.h>
   #include <fcntl.h>
      #include "xf86drm.h"
   #include "drm-uapi/lima_drm.h"
      #include "util/u_hash_table.h"
   #include "util/u_math.h"
   #include "util/os_time.h"
   #include "util/os_mman.h"
      #include "frontend/drm_driver.h"
      #include "lima_screen.h"
   #include "lima_bo.h"
   #include "lima_util.h"
      bool lima_bo_table_init(struct lima_screen *screen)
   {
      screen->bo_handles = util_hash_table_create_ptr_keys();
   if (!screen->bo_handles)
            screen->bo_flink_names = util_hash_table_create_ptr_keys();
   if (!screen->bo_flink_names)
            mtx_init(&screen->bo_table_lock, mtx_plain);
         err_out0:
      _mesa_hash_table_destroy(screen->bo_handles, NULL);
      }
      bool lima_bo_cache_init(struct lima_screen *screen)
   {
      mtx_init(&screen->bo_cache_lock, mtx_plain);
   list_inithead(&screen->bo_cache_time);
   for (int i = 0; i < NR_BO_CACHE_BUCKETS; i++)
               }
      void lima_bo_table_fini(struct lima_screen *screen)
   {
      mtx_destroy(&screen->bo_table_lock);
   _mesa_hash_table_destroy(screen->bo_handles, NULL);
      }
      static void
   lima_bo_cache_remove(struct lima_bo *bo)
   {
      list_del(&bo->size_list);
      }
      static void lima_close_kms_handle(struct lima_screen *screen, uint32_t handle)
   {
      struct drm_gem_close args = {
                     }
      static void
   lima_bo_free(struct lima_bo *bo)
   {
               if (lima_debug & LIMA_DEBUG_BO_CACHE)
      fprintf(stderr, "%s: %p (size=%d)\n", __func__,
         mtx_lock(&screen->bo_table_lock);
   _mesa_hash_table_remove_key(screen->bo_handles,
         if (bo->flink_name)
      _mesa_hash_table_remove_key(screen->bo_flink_names,
               if (bo->map)
            lima_close_kms_handle(screen, bo->handle);
      }
      void lima_bo_cache_fini(struct lima_screen *screen)
   {
               list_for_each_entry_safe(struct lima_bo, entry,
            lima_bo_cache_remove(entry);
         }
      static bool lima_bo_get_info(struct lima_bo *bo)
   {
      struct drm_lima_gem_info req = {
                  if(drmIoctl(bo->screen->fd, DRM_IOCTL_LIMA_GEM_INFO, &req))
            bo->offset = req.offset;
   bo->va = req.va;
      }
      static unsigned
   lima_bucket_index(unsigned size)
   {
                        /* Clamp the bucket index; all huge allocations will be
   * sorted into the largest bucket */
   bucket_index = CLAMP(bucket_index, MIN_BO_CACHE_BUCKET,
            /* Reindex from 0 */
      }
      static struct list_head *
   lima_bo_cache_get_bucket(struct lima_screen *screen, unsigned size)
   {
         }
      static void
   lima_bo_cache_free_stale_bos(struct lima_screen *screen, time_t time)
   {
      unsigned cnt = 0;
   list_for_each_entry_safe(struct lima_bo, entry,
            /* Free BOs that are sitting idle for longer than 5 seconds */
   if (time - entry->free_time > 6) {
      lima_bo_cache_remove(entry);
   lima_bo_free(entry);
      } else
      }
   if ((lima_debug & LIMA_DEBUG_BO_CACHE) && cnt)
      }
      static void
   lima_bo_cache_print_stats(struct lima_screen *screen)
   {
      fprintf(stderr, "===============\n");
   fprintf(stderr, "BO cache stats:\n");
   unsigned total_size = 0;
   for (int i = 0; i < NR_BO_CACHE_BUCKETS; i++) {
      struct list_head *bucket = &screen->bo_cache_buckets[i];
   unsigned bucket_size = 0;
   list_for_each_entry(struct lima_bo, entry, bucket, size_list) {
      bucket_size += entry->size;
      }
   fprintf(stderr, "Bucket #%d, BOs: %d, size: %u\n", i,
            }
      }
      static bool
   lima_bo_cache_put(struct lima_bo *bo)
   {
      if (!bo->cacheable)
                     mtx_lock(&screen->bo_cache_lock);
            if (!bucket) {
      mtx_unlock(&screen->bo_cache_lock);
               struct timespec time;
   clock_gettime(CLOCK_MONOTONIC, &time);
   bo->free_time = time.tv_sec;
   list_addtail(&bo->size_list, bucket);
   list_addtail(&bo->time_list, &screen->bo_cache_time);
   lima_bo_cache_free_stale_bos(screen, time.tv_sec);
   if (lima_debug & LIMA_DEBUG_BO_CACHE) {
      fprintf(stderr, "%s: put BO: %p (size=%d)\n", __func__, bo, bo->size);
      }
               }
      static struct lima_bo *
   lima_bo_cache_get(struct lima_screen *screen, uint32_t size, uint32_t flags)
   {
      /* we won't cache heap buffer */
   if (flags & LIMA_BO_FLAG_HEAP)
            struct lima_bo *bo = NULL;
   mtx_lock(&screen->bo_cache_lock);
            if (!bucket) {
      mtx_unlock(&screen->bo_cache_lock);
               list_for_each_entry_safe(struct lima_bo, entry, bucket, size_list) {
      if (entry->size >= size) {
      /* Check if BO is idle. If it's not it's better to allocate new one */
   if (!lima_bo_wait(entry, LIMA_GEM_WAIT_WRITE, 0)) {
      if (lima_debug & LIMA_DEBUG_BO_CACHE) {
      fprintf(stderr, "%s: found BO %p but it's busy\n", __func__,
      }
               lima_bo_cache_remove(entry);
   p_atomic_set(&entry->refcnt, 1);
   entry->flags = flags;
   bo = entry;
   if (lima_debug & LIMA_DEBUG_BO_CACHE) {
      fprintf(stderr, "%s: got BO: %p (size=%d), requested size %d\n",
            }
                              }
      struct lima_bo *lima_bo_create(struct lima_screen *screen,
         {
                        /* Try to get bo from cache first */
   bo = lima_bo_cache_get(screen, size, flags);
   if (bo)
            struct drm_lima_gem_create req = {
      .size = size,
               if (!(bo = calloc(1, sizeof(*bo))))
            list_inithead(&bo->time_list);
            if (drmIoctl(screen->fd, DRM_IOCTL_LIMA_GEM_CREATE, &req))
            bo->screen = screen;
   bo->size = req.size;
   bo->flags = req.flags;
   bo->handle = req.handle;
   bo->cacheable = !(lima_debug & LIMA_DEBUG_NO_BO_CACHE ||
                  if (!lima_bo_get_info(bo))
            if (lima_debug & LIMA_DEBUG_BO_CACHE)
      fprintf(stderr, "%s: %p (size=%d)\n", __func__,
               err_out1:
         err_out0:
      free(bo);
      }
      void lima_bo_unreference(struct lima_bo *bo)
   {
      if (!p_atomic_dec_zero(&bo->refcnt))
            /* Try to put it into cache */
   if (lima_bo_cache_put(bo))
               }
      void *lima_bo_map(struct lima_bo *bo)
   {
      if (!bo->map) {
      bo->map = os_mmap(0, bo->size, PROT_READ | PROT_WRITE,
         if (bo->map == MAP_FAILED)
                  }
      void lima_bo_unmap(struct lima_bo *bo)
   {
      if (bo->map) {
      os_munmap(bo->map, bo->size);
         }
      bool lima_bo_export(struct lima_bo *bo, struct winsys_handle *handle)
   {
               /* Don't cache exported BOs */
            switch (handle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
      if (!bo->flink_name) {
      struct drm_gem_flink flink = {
      .handle = bo->handle,
      };
                           mtx_lock(&screen->bo_table_lock);
   _mesa_hash_table_insert(screen->bo_flink_names,
            }
   handle->handle = bo->flink_name;
         case WINSYS_HANDLE_TYPE_KMS:
      mtx_lock(&screen->bo_table_lock);
   _mesa_hash_table_insert(screen->bo_handles,
                  handle->handle = bo->handle;
         case WINSYS_HANDLE_TYPE_FD:
      if (drmPrimeHandleToFD(screen->fd, bo->handle, DRM_CLOEXEC,
                  mtx_lock(&screen->bo_table_lock);
   _mesa_hash_table_insert(screen->bo_handles,
         mtx_unlock(&screen->bo_table_lock);
         default:
            }
      struct lima_bo *lima_bo_import(struct lima_screen *screen,
         {
      struct lima_bo *bo = NULL;
   struct drm_gem_open req = {0};
   uint32_t dma_buf_size = 0;
                     /* Convert a DMA buf handle to a KMS handle now. */
   if (handle->type == WINSYS_HANDLE_TYPE_FD) {
      uint32_t prime_handle;
            /* Get a KMS handle. */
   if (drmPrimeFDToHandle(screen->fd, h, &prime_handle)) {
      mtx_unlock(&screen->bo_table_lock);
               /* Query the buffer size. */
   size = lseek(h, 0, SEEK_END);
   if (size == (off_t)-1) {
      mtx_unlock(&screen->bo_table_lock);
   lima_close_kms_handle(screen, prime_handle);
      }
            dma_buf_size = size;
               switch (handle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
      bo = util_hash_table_get(screen->bo_flink_names,
            case WINSYS_HANDLE_TYPE_KMS:
   case WINSYS_HANDLE_TYPE_FD:
      bo = util_hash_table_get(screen->bo_handles,
            default:
      mtx_unlock(&screen->bo_table_lock);
               if (bo) {
      p_atomic_inc(&bo->refcnt);
   /* Don't cache imported BOs */
   bo->cacheable = false;
   mtx_unlock(&screen->bo_table_lock);
               if (!(bo = calloc(1, sizeof(*bo)))) {
      mtx_unlock(&screen->bo_table_lock);
   if (handle->type == WINSYS_HANDLE_TYPE_FD)
                     /* Don't cache imported BOs */
   bo->cacheable = false;
   list_inithead(&bo->time_list);
   list_inithead(&bo->size_list);
   bo->screen = screen;
            switch (handle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
      req.name = h;
   if (drmIoctl(screen->fd, DRM_IOCTL_GEM_OPEN, &req)) {
      mtx_unlock(&screen->bo_table_lock);
   free(bo);
      }
   bo->handle = req.handle;
   bo->flink_name = h;
   bo->size = req.size;
      case WINSYS_HANDLE_TYPE_FD:
      bo->handle = h;
   bo->size = dma_buf_size;
      default:
      /* not possible */
               if (lima_bo_get_info(bo)) {
      if (handle->type == WINSYS_HANDLE_TYPE_SHARED)
      _mesa_hash_table_insert(screen->bo_flink_names,
      _mesa_hash_table_insert(screen->bo_handles,
      }
   else {
      lima_close_kms_handle(screen, bo->handle);
   free(bo);
                           }
      bool lima_bo_wait(struct lima_bo *bo, uint32_t op, uint64_t timeout_ns)
   {
               if (timeout_ns == 0)
         else
            if (abs_timeout == OS_TIMEOUT_INFINITE)
            struct drm_lima_gem_wait req = {
      .handle = bo->handle,
   .op = op,
                  }
