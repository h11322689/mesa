   /*
   * Copyright © 2014-2017 Broadcom
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <errno.h>
   #include <err.h>
   #include <sys/mman.h>
   #include <fcntl.h>
   #include <xf86drm.h>
   #include <xf86drmMode.h>
      #include "util/u_hash_table.h"
   #include "util/u_memory.h"
   #include "util/ralloc.h"
      #include "v3d_context.h"
   #include "v3d_screen.h"
      static bool dump_stats = false;
      static void
   v3d_bo_cache_free_all(struct v3d_bo_cache *cache);
      static void
   v3d_bo_dump_stats(struct v3d_screen *screen)
   {
                  uint32_t cache_count = 0;
   uint32_t cache_size = 0;
   list_for_each_entry(struct v3d_bo, bo, &cache->time_list, time_list) {
                        fprintf(stderr, "  BOs allocated:   %d\n", screen->bo_count);
   fprintf(stderr, "  BOs size:        %dkb\n", screen->bo_size / 1024);
   fprintf(stderr, "  BOs cached:      %d\n", cache_count);
            if (!list_is_empty(&cache->time_list)) {
            struct v3d_bo *first = list_first_entry(&cache->time_list,
                                    fprintf(stderr, "  oldest cache time: %ld\n",
                        struct timespec time;
   clock_gettime(CLOCK_MONOTONIC, &time);
      }
      static void
   v3d_bo_remove_from_cache(struct v3d_bo_cache *cache, struct v3d_bo *bo)
   {
         list_del(&bo->time_list);
   }
      static struct v3d_bo *
   v3d_bo_from_cache(struct v3d_screen *screen, uint32_t size, const char *name)
   {
         struct v3d_bo_cache *cache = &screen->bo_cache;
            if (cache->size_list_size <= page_index)
            struct v3d_bo *bo = NULL;
   mtx_lock(&cache->lock);
   if (!list_is_empty(&cache->size_list[page_index])) {
                           /* Check that the BO has gone idle.  If not, then we want to
   * allocate something new instead, since we assume that the
   * user will proceed to CPU map it and fill it with stuff.
   */
   if (!v3d_bo_wait(bo, 0, NULL)) {
                                    }
   mtx_unlock(&cache->lock);
   }
      struct v3d_bo *
   v3d_bo_alloc(struct v3d_screen *screen, uint32_t size, const char *name)
   {
         struct v3d_bo *bo;
            /* The CLIF dumping requires that there is no whitespace in the name.
                           bo = v3d_bo_from_cache(screen, size, name);
   if (bo) {
            if (dump_stats) {
            fprintf(stderr, "Allocated %s %dkb from cache:\n",
                  bo = CALLOC_STRUCT(v3d_bo);
   if (!bo)
            pipe_reference_init(&bo->reference, 1);
   bo->screen = screen;
   bo->size = size;
   bo->name = name;
      retry:
                  bool cleared_and_retried = false;
   struct drm_v3d_create_bo create = {
                  ret = v3d_ioctl(screen->fd, DRM_IOCTL_V3D_CREATE_BO, &create);
   bo->handle = create.handle;
            if (ret != 0) {
            if (!list_is_empty(&screen->bo_cache.time_list) &&
      !cleared_and_retried) {
                                       screen->bo_count++;
   screen->bo_size += bo->size;
   if (dump_stats) {
                        }
      void
   v3d_bo_last_unreference(struct v3d_bo *bo)
   {
                  struct timespec time;
   clock_gettime(CLOCK_MONOTONIC, &time);
   mtx_lock(&screen->bo_cache.lock);
   v3d_bo_last_unreference_locked_timed(bo, time.tv_sec);
   }
      static void
   v3d_bo_free(struct v3d_bo *bo)
   {
                  if (bo->map) {
            if (using_v3d_simulator && bo->name &&
      strcmp(bo->name, "winsys") == 0) {
      } else {
                     struct drm_gem_close c;
   memset(&c, 0, sizeof(c));
   c.handle = bo->handle;
   int ret = v3d_ioctl(screen->fd, DRM_IOCTL_GEM_CLOSE, &c);
   if (ret != 0)
            screen->bo_count--;
            if (dump_stats) {
            fprintf(stderr, "Freed %s%s%dkb:\n",
            bo->name ? bo->name : "",
            }
      static void
   free_stale_bos(struct v3d_screen *screen, time_t time)
   {
         struct v3d_bo_cache *cache = &screen->bo_cache;
            list_for_each_entry_safe(struct v3d_bo, bo, &cache->time_list,
                  /* If it's more than a second old, free it. */
   if (time - bo->free_time > 2) {
            if (dump_stats && !freed_any) {
         fprintf(stderr, "Freeing stale BOs:\n");
   v3d_bo_dump_stats(screen);
   }
      } else {
               if (dump_stats && freed_any) {
               }
      static void
   v3d_bo_cache_free_all(struct v3d_bo_cache *cache)
   {
         mtx_lock(&cache->lock);
   list_for_each_entry_safe(struct v3d_bo, bo, &cache->time_list,
                     }
   }
      void
   v3d_bo_last_unreference_locked_timed(struct v3d_bo *bo, time_t time)
   {
         struct v3d_screen *screen = bo->screen;
   struct v3d_bo_cache *cache = &screen->bo_cache;
            if (!bo->private) {
                        if (cache->size_list_size <= page_index) {
                           /* Move old list contents over (since the array has moved, and
   * therefore the pointers to the list heads have to change).
   */
   for (int i = 0; i < cache->size_list_size; i++) {
            struct list_head *old_head = &cache->size_list[i];
   if (list_is_empty(old_head))
         else {
         new_list[i].next = old_head->next;
   new_list[i].prev = old_head->prev;
                                       bo->free_time = time;
   list_addtail(&bo->size_list, &cache->size_list[page_index]);
   list_addtail(&bo->time_list, &cache->time_list);
   if (dump_stats) {
            fprintf(stderr, "Freed %s %dkb to cache:\n",
      }
            }
      static struct v3d_bo *
   v3d_bo_open_handle(struct v3d_screen *screen,
         {
                  /* Note: the caller is responsible for locking screen->bo_handles_mutex.
                           bo = util_hash_table_get(screen->bo_handles, (void*)(uintptr_t)handle);
   if (bo) {
                        bo = CALLOC_STRUCT(v3d_bo);
   pipe_reference_init(&bo->reference, 1);
   bo->screen = screen;
   bo->handle = handle;
   bo->size = size;
   bo->name = "winsys";
      #ifdef USE_V3D_SIMULATOR
         v3d_simulator_open_from_handle(screen->fd, bo->handle, bo->size);
   #endif
            struct drm_v3d_get_bo_offset get = {
         };
   int ret = v3d_ioctl(screen->fd, DRM_IOCTL_V3D_GET_BO_OFFSET, &get);
   if (ret) {
            fprintf(stderr, "Failed to get BO offset: %s\n",
         free(bo->map);
   free(bo);
      }
   bo->offset = get.offset;
                     screen->bo_count++;
      done:
         mtx_unlock(&screen->bo_handles_mutex);
   }
      struct v3d_bo *
   v3d_bo_open_name(struct v3d_screen *screen, uint32_t name)
   {
         struct drm_gem_open o = {
         };
            int ret = v3d_ioctl(screen->fd, DRM_IOCTL_GEM_OPEN, &o);
   if (ret) {
            fprintf(stderr, "Failed to open bo %d: %s\n",
                     }
      struct v3d_bo *
   v3d_bo_open_dmabuf(struct v3d_screen *screen, int fd)
   {
                           int ret = drmPrimeFDToHandle(screen->fd, fd, &handle);
   int size;
   if (ret) {
            fprintf(stderr, "Failed to get v3d handle for dmabuf %d\n", fd);
               /* Determine the size of the bo we were handed. */
   size = lseek(fd, 0, SEEK_END);
   if (size == -1) {
            fprintf(stderr, "Couldn't get size of dmabuf fd %d.\n", fd);
               }
      int
   v3d_bo_get_dmabuf(struct v3d_bo *bo)
   {
         int fd;
   int ret = drmPrimeHandleToFD(bo->screen->fd, bo->handle,
         if (ret != 0) {
            fprintf(stderr, "Failed to export gem bo %d to dmabuf\n",
               mtx_lock(&bo->screen->bo_handles_mutex);
   bo->private = false;
   _mesa_hash_table_insert(bo->screen->bo_handles, (void *)(uintptr_t)bo->handle, bo);
            }
      bool
   v3d_bo_flink(struct v3d_bo *bo, uint32_t *name)
   {
         struct drm_gem_flink flink = {
         };
   int ret = v3d_ioctl(bo->screen->fd, DRM_IOCTL_GEM_FLINK, &flink);
   if (ret) {
            fprintf(stderr, "Failed to flink bo %d: %s\n",
                     bo->private = false;
            }
      static int v3d_wait_bo_ioctl(int fd, uint32_t handle, uint64_t timeout_ns)
   {
         struct drm_v3d_wait_bo wait = {
               };
   int ret = v3d_ioctl(fd, DRM_IOCTL_V3D_WAIT_BO, &wait);
   if (ret == -1)
         else
      }
      bool
   v3d_bo_wait(struct v3d_bo *bo, uint64_t timeout_ns, const char *reason)
   {
                  if (V3D_DBG(PERF) && timeout_ns && reason) {
            if (v3d_wait_bo_ioctl(screen->fd, bo->handle, 0) == -ETIME) {
                     int ret = v3d_wait_bo_ioctl(screen->fd, bo->handle, timeout_ns);
   if (ret) {
            if (ret != -ETIME) {
                              }
      void *
   v3d_bo_map_unsynchronized(struct v3d_bo *bo)
   {
         uint64_t offset;
            if (bo->map)
            struct drm_v3d_mmap_bo map;
   memset(&map, 0, sizeof(map));
   map.handle = bo->handle;
   ret = v3d_ioctl(bo->screen->fd, DRM_IOCTL_V3D_MMAP_BO, &map);
   offset = map.offset;
   if (ret != 0) {
                        bo->map = mmap(NULL, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
         if (bo->map == MAP_FAILED) {
            fprintf(stderr, "mmap of bo %d (offset 0x%016llx, size %d) failed\n",
      }
            }
      void *
   v3d_bo_map(struct v3d_bo *bo)
   {
                  bool ok = v3d_bo_wait(bo, OS_TIMEOUT_INFINITE, "bo map");
   if (!ok) {
                        }
      void
   v3d_bufmgr_destroy(struct pipe_screen *pscreen)
   {
         struct v3d_screen *screen = v3d_screen(pscreen);
                     if (dump_stats) {
               }
