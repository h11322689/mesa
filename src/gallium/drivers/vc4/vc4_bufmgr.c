   /*
   * Copyright © 2014-2015 Broadcom
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
   #include "util/u_string.h"
   #include "util/ralloc.h"
      #include "vc4_context.h"
   #include "vc4_screen.h"
      static bool dump_stats = false;
      static void
   vc4_bo_cache_free_all(struct vc4_bo_cache *cache);
      void
   vc4_bo_debug_describe(char* buf, const struct vc4_bo *ptr)
   {
      sprintf(buf, "vc4_bo<%s,%u,%u>", ptr->name ? ptr->name : "?",
      }
      void
   vc4_bo_label(struct vc4_screen *screen, struct vc4_bo *bo, const char *fmt, ...)
   {
         /* Perform BO labeling by default on debug builds (so that you get
      * whole-system allocation information), or if VC4_DEBUG=surf is set
      #ifndef DEBUG
         if (!VC4_DBG(SURFACE))
   #endif
         va_list va;
   va_start(va, fmt);
   char *name = ralloc_vasprintf(NULL, fmt, va);
            struct drm_vc4_label_bo label = {
            .handle = bo->handle,
      };
            }
      static void
   vc4_bo_dump_stats(struct vc4_screen *screen)
   {
                  fprintf(stderr, "  BOs allocated:   %d\n", screen->bo_count);
   fprintf(stderr, "  BOs size:        %dkb\n", screen->bo_size / 1024);
   fprintf(stderr, "  BOs cached:      %d\n", cache->bo_count);
            if (!list_is_empty(&cache->time_list)) {
            struct vc4_bo *first = list_entry(cache->time_list.next,
                                    fprintf(stderr, "  oldest cache time: %ld\n",
                        struct timespec time;
   clock_gettime(CLOCK_MONOTONIC, &time);
      }
      static void
   vc4_bo_remove_from_cache(struct vc4_bo_cache *cache, struct vc4_bo *bo)
   {
         list_del(&bo->time_list);
   list_del(&bo->size_list);
   cache->bo_count--;
   }
      static void vc4_bo_purgeable(struct vc4_bo *bo)
   {
         struct drm_vc4_gem_madvise arg = {
                  if (bo->screen->has_madvise)
   vc4_ioctl(bo->screen->fd, DRM_IOCTL_VC4_GEM_MADVISE, &arg);
   }
      static bool vc4_bo_unpurgeable(struct vc4_bo *bo)
   {
         struct drm_vc4_gem_madvise arg = {
                  if (!bo->screen->has_madvise)
   return true;
      if (vc4_ioctl(bo->screen->fd, DRM_IOCTL_VC4_GEM_MADVISE, &arg))
   return false;
      return arg.retained;
   }
      static void
   vc4_bo_free(struct vc4_bo *bo)
   {
                  if (bo->map) {
            if (using_vc4_simulator && bo->name &&
      strcmp(bo->name, "winsys") == 0) {
      } else {
                     struct drm_gem_close c;
   memset(&c, 0, sizeof(c));
   c.handle = bo->handle;
   int ret = vc4_ioctl(screen->fd, DRM_IOCTL_GEM_CLOSE, &c);
   if (ret != 0)
            screen->bo_count--;
            if (dump_stats) {
            fprintf(stderr, "Freed %s%s%dkb:\n",
            bo->name ? bo->name : "",
            }
      static struct vc4_bo *
   vc4_bo_from_cache(struct vc4_screen *screen, uint32_t size, const char *name)
   {
         struct vc4_bo_cache *cache = &screen->bo_cache;
   uint32_t page_index = size / 4096 - 1;
            if (cache->size_list_size <= page_index)
            LIST_FOR_EACH_ENTRY_SAFE(iter, tmp, &cache->size_list[page_index],
      size_list) {
               /* Check that the BO has gone idle.  If not, then none of the
   * other BOs (pushed to the list after later rendering) are
   * likely to be idle, either.
                        if (!vc4_bo_unpurgeable(iter)) {
            /* The BO has been purged. Free it and try to find
   * another one in the cache.
   }
                                          vc4_bo_label(screen, bo, "%s", name);
      }
   mtx_unlock(&cache->lock);
   }
      struct vc4_bo *
   vc4_bo_alloc(struct vc4_screen *screen, uint32_t size, const char *name)
   {
         bool cleared_and_retried = false;
   struct drm_vc4_create_bo create;
   struct vc4_bo *bo;
                     bo = vc4_bo_from_cache(screen, size, name);
   if (bo) {
            if (dump_stats) {
            fprintf(stderr, "Allocated %s %dkb from cache:\n",
                  bo = CALLOC_STRUCT(vc4_bo);
   if (!bo)
            pipe_reference_init(&bo->reference, 1);
   bo->screen = screen;
   bo->size = size;
   bo->name = name;
      retry:
         memset(&create, 0, sizeof(create));
            ret = vc4_ioctl(screen->fd, DRM_IOCTL_VC4_CREATE_BO, &create);
            if (ret != 0) {
            if (!list_is_empty(&screen->bo_cache.time_list) &&
      !cleared_and_retried) {
                                       screen->bo_count++;
   screen->bo_size += bo->size;
   if (dump_stats) {
                                 }
      void
   vc4_bo_last_unreference(struct vc4_bo *bo)
   {
                  struct timespec time;
   clock_gettime(CLOCK_MONOTONIC, &time);
   mtx_lock(&screen->bo_cache.lock);
   vc4_bo_last_unreference_locked_timed(bo, time.tv_sec);
   }
      static void
   free_stale_bos(struct vc4_screen *screen, time_t time)
   {
         struct vc4_bo_cache *cache = &screen->bo_cache;
            list_for_each_entry_safe(struct vc4_bo, bo, &cache->time_list,
                  if (dump_stats && !freed_any) {
                              /* If it's more than a second old, free it. */
   if (time - bo->free_time > 2) {
               } else {
               if (dump_stats && freed_any) {
               }
      static void
   vc4_bo_cache_free_all(struct vc4_bo_cache *cache)
   {
         mtx_lock(&cache->lock);
   list_for_each_entry_safe(struct vc4_bo, bo, &cache->time_list,
                     }
   }
      void
   vc4_bo_last_unreference_locked_timed(struct vc4_bo *bo, time_t time)
   {
         struct vc4_screen *screen = bo->screen;
   struct vc4_bo_cache *cache = &screen->bo_cache;
            if (!bo->private) {
                        if (cache->size_list_size <= page_index) {
                           /* Move old list contents over (since the array has moved, and
   * therefore the pointers to the list heads have to change).
   */
   for (int i = 0; i < cache->size_list_size; i++)
                                    vc4_bo_purgeable(bo);
   bo->free_time = time;
   list_addtail(&bo->size_list, &cache->size_list[page_index]);
   list_addtail(&bo->time_list, &cache->time_list);
   cache->bo_count++;
   cache->bo_size += bo->size;
   if (dump_stats) {
            fprintf(stderr, "Freed %s %dkb to cache:\n",
      }
   bo->name = NULL;
            }
      static struct vc4_bo *
   vc4_bo_open_handle(struct vc4_screen *screen,
         {
                  /* Note: the caller is responsible for locking screen->bo_handles_mutex.
                           bo = util_hash_table_get(screen->bo_handles, (void*)(uintptr_t)handle);
   if (bo) {
                        bo = CALLOC_STRUCT(vc4_bo);
   pipe_reference_init(&bo->reference, 1);
   bo->screen = screen;
   bo->handle = handle;
   bo->size = size;
   bo->name = "winsys";
      #ifdef USE_VC4_SIMULATOR
         vc4_simulator_open_from_handle(screen->fd, bo->handle, bo->size);
   #endif
               done:
         mtx_unlock(&screen->bo_handles_mutex);
   }
      struct vc4_bo *
   vc4_bo_open_name(struct vc4_screen *screen, uint32_t name)
   {
         struct drm_gem_open o = {
                           int ret = vc4_ioctl(screen->fd, DRM_IOCTL_GEM_OPEN, &o);
   if (ret) {
            fprintf(stderr, "Failed to open bo %d: %s\n",
                     }
      struct vc4_bo *
   vc4_bo_open_dmabuf(struct vc4_screen *screen, int fd)
   {
                           int ret = drmPrimeFDToHandle(screen->fd, fd, &handle);
   int size;
   if (ret) {
            fprintf(stderr, "Failed to get vc4 handle for dmabuf %d\n", fd);
               /* Determine the size of the bo we were handed. */
   size = lseek(fd, 0, SEEK_END);
   if (size == -1) {
            fprintf(stderr, "Couldn't get size of dmabuf fd %d.\n", fd);
               }
      int
   vc4_bo_get_dmabuf(struct vc4_bo *bo)
   {
         int fd;
   int ret = drmPrimeHandleToFD(bo->screen->fd, bo->handle,
         if (ret != 0) {
            fprintf(stderr, "Failed to export gem bo %d to dmabuf\n",
               mtx_lock(&bo->screen->bo_handles_mutex);
   bo->private = false;
   _mesa_hash_table_insert(bo->screen->bo_handles, (void *)(uintptr_t)bo->handle, bo);
            }
      struct vc4_bo *
   vc4_bo_alloc_shader(struct vc4_screen *screen, const void *data, uint32_t size)
   {
         struct vc4_bo *bo;
            bo = CALLOC_STRUCT(vc4_bo);
   if (!bo)
            pipe_reference_init(&bo->reference, 1);
   bo->screen = screen;
   bo->size = align(size, 4096);
   bo->name = "code";
            struct drm_vc4_create_shader_bo create = {
                        ret = vc4_ioctl(screen->fd, DRM_IOCTL_VC4_CREATE_SHADER_BO,
                  if (ret != 0) {
                        screen->bo_count++;
   screen->bo_size += bo->size;
   if (dump_stats) {
                        }
      bool
   vc4_bo_flink(struct vc4_bo *bo, uint32_t *name)
   {
         struct drm_gem_flink flink = {
         };
   int ret = vc4_ioctl(bo->screen->fd, DRM_IOCTL_GEM_FLINK, &flink);
   if (ret) {
            fprintf(stderr, "Failed to flink bo %d: %s\n",
                     bo->private = false;
            }
      static int vc4_wait_seqno_ioctl(int fd, uint64_t seqno, uint64_t timeout_ns)
   {
         struct drm_vc4_wait_seqno wait = {
               };
   int ret = vc4_ioctl(fd, DRM_IOCTL_VC4_WAIT_SEQNO, &wait);
   if (ret == -1)
         else
      }
      bool
   vc4_wait_seqno(struct vc4_screen *screen, uint64_t seqno, uint64_t timeout_ns,
         {
         if (screen->finished_seqno >= seqno)
            if (VC4_DBG(PERF) && timeout_ns && reason) {
            if (vc4_wait_seqno_ioctl(screen->fd, seqno, 0) == -ETIME) {
                     int ret = vc4_wait_seqno_ioctl(screen->fd, seqno, timeout_ns);
   if (ret) {
            if (ret != -ETIME) {
                              screen->finished_seqno = seqno;
   }
      static int vc4_wait_bo_ioctl(int fd, uint32_t handle, uint64_t timeout_ns)
   {
         struct drm_vc4_wait_bo wait = {
               };
   int ret = vc4_ioctl(fd, DRM_IOCTL_VC4_WAIT_BO, &wait);
   if (ret == -1)
         else
      }
      bool
   vc4_bo_wait(struct vc4_bo *bo, uint64_t timeout_ns, const char *reason)
   {
                  if (VC4_DBG(PERF) && timeout_ns && reason) {
            if (vc4_wait_bo_ioctl(screen->fd, bo->handle, 0) == -ETIME) {
                     int ret = vc4_wait_bo_ioctl(screen->fd, bo->handle, timeout_ns);
   if (ret) {
            if (ret != -ETIME) {
                              }
      void *
   vc4_bo_map_unsynchronized(struct vc4_bo *bo)
   {
         uint64_t offset;
            if (bo->map)
            struct drm_vc4_mmap_bo map;
   memset(&map, 0, sizeof(map));
   map.handle = bo->handle;
   ret = vc4_ioctl(bo->screen->fd, DRM_IOCTL_VC4_MMAP_BO, &map);
   offset = map.offset;
   if (ret != 0) {
                        bo->map = mmap(NULL, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
         if (bo->map == MAP_FAILED) {
            fprintf(stderr, "mmap of bo %d (offset 0x%016llx, size %d) failed\n",
      }
            }
      void *
   vc4_bo_map(struct vc4_bo *bo)
   {
                  bool ok = vc4_bo_wait(bo, OS_TIMEOUT_INFINITE, "bo map");
   if (!ok) {
                        }
      void
   vc4_bufmgr_destroy(struct pipe_screen *pscreen)
   {
         struct vc4_screen *screen = vc4_screen(pscreen);
                     if (dump_stats) {
               }
