   /*
   * Copyright © 2015-2018 Intel Corporation
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
      #undef _FILE_OFFSET_BITS /* prevent #define open open64 */
      #include <string.h>
   #include <stdlib.h>
   #include <stdio.h>
   #include <stdint.h>
   #include <stdarg.h>
   #include <fcntl.h>
   #include <unistd.h>
   #include <sys/ioctl.h>
   #include <sys/stat.h>
   #include <sys/mman.h>
   #include <sys/sysmacros.h>
   #include <dlfcn.h>
   #include <pthread.h>
   #include "drm-uapi/i915_drm.h"
      #include "util/hash_table.h"
   #include "util/u_math.h"
      #define MESA_LOG_TAG "INTEL-SANITIZE-GPU"
   #include "util/log.h"
   #include "common/intel_mem.h"
      static int (*libc_open)(const char *pathname, int flags, mode_t mode);
   static int (*libc_close)(int fd);
   static int (*libc_ioctl)(int fd, unsigned long request, void *argp);
   static int (*libc_fcntl)(int fd, int cmd, int param);
      #define DRM_MAJOR 226
      /* TODO: we want to make sure that the padding forces
   * the BO to take another page on the (PP)GTT; 4KB
   * may or may not be the page size for the BO. Indeed,
   * depending on GPU, kernel version and GEM size, the
   * page size can be one of 4KB, 64KB or 2M.
   */
   #define PADDING_SIZE 4096
      struct refcnt_hash_table {
      struct hash_table *t;
      };
      pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
   #define MUTEX_LOCK() do {                        \
      if (unlikely(pthread_mutex_lock(&mutex))) {   \
      mesa_loge("mutex_lock failed");           \
         } while (0)
   #define MUTEX_UNLOCK() do {                      \
      if (unlikely(pthread_mutex_unlock(&mutex))) { \
      mesa_loge("mutex_unlock failed");         \
         } while (0)
      static struct hash_table *fds_to_bo_sizes = NULL;
      static inline struct hash_table*
   bo_size_table(int fd)
   {
      struct hash_entry *e = _mesa_hash_table_search(fds_to_bo_sizes,
            }
      static inline uint64_t
   bo_size(int fd, uint32_t handle)
   {
      struct hash_table *t = bo_size_table(fd);
   if (!t)
         struct hash_entry *e = _mesa_hash_table_search(t, (void*)(uintptr_t)handle);
      }
      static inline bool
   is_drm_fd(int fd)
   {
         }
      static inline void
   add_drm_fd(int fd)
   {
      struct refcnt_hash_table *r = malloc(sizeof(*r));
   r->refcnt = 1;
   r->t = _mesa_pointer_hash_table_create(NULL);
   _mesa_hash_table_insert(fds_to_bo_sizes, (void*)(uintptr_t)fd,
      }
      static inline void
   dup_drm_fd(int old_fd, int new_fd)
   {
      struct hash_entry *e = _mesa_hash_table_search(fds_to_bo_sizes,
         struct refcnt_hash_table *r = e->data;
   r->refcnt++;
   _mesa_hash_table_insert(fds_to_bo_sizes, (void*)(uintptr_t)new_fd,
      }
      static inline void
   del_drm_fd(int fd)
   {
      struct hash_entry *e = _mesa_hash_table_search(fds_to_bo_sizes,
         struct refcnt_hash_table *r = e->data;
   if (!--r->refcnt) {
      _mesa_hash_table_remove(fds_to_bo_sizes, e);
   _mesa_hash_table_destroy(r->t, NULL);
         }
      /* Our goal is not to have noise good enough for crypto,
   * but instead values that are unique-ish enough that
   * it is incredibly unlikely that a buffer overwrite
   * will produce the exact same values.
   */
   static uint8_t
   next_noise_value(uint8_t prev_noise)
   {
      uint32_t v = prev_noise;
      }
      static void
   fill_noise_buffer(uint8_t *dst, uint8_t start, uint32_t length)
   {
      for(uint32_t i = 0; i < length; ++i) {
      dst[i] = start;
         }
      static bool
   padding_is_good(int fd, uint32_t handle)
   {
      struct drm_i915_gem_mmap mmap_arg = {
      .handle = handle,
   .offset = align64(bo_size(fd, handle), 4096),
   .size = PADDING_SIZE,
               /* Unknown bo, maybe prime or userptr. Ignore */
   if (mmap_arg.offset == UINT64_MAX)
            uint8_t *mapped;
   int ret;
            ret = libc_ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg);
   if (ret != 0) {
      mesa_logd("Unable to map buffer %d for pad checking.", handle);
                  #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
      /* bah-humbug, we need to see the latest contents and
   * if the bo is not cache coherent we likely need to
   * invalidate the cache lines to get it.
   */
      #endif
         expected_value = handle & 0xFF;
   for (uint32_t i = 0; i < PADDING_SIZE; ++i) {
      if (expected_value != mapped[i]) {
      munmap(mapped, PADDING_SIZE);
      }
      }
               }
      static int
   create_with_padding(int fd, struct drm_i915_gem_create *create)
   {
               create->size = align64(original_size, 4096) + PADDING_SIZE;
   int ret = libc_ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, create);
            if (ret != 0)
            uint8_t *noise_values;
   struct drm_i915_gem_mmap mmap_arg = {
      .handle = create->handle,
   .offset = align64(create->size, 4096),
   .size = PADDING_SIZE,
               ret = libc_ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg);
   if (ret != 0) {
      mesa_logd("Unable to map buffer %d for pad creation.\n", create->handle);
               noise_values = (uint8_t*) (uintptr_t) mmap_arg.addr_ptr;
   fill_noise_buffer(noise_values, create->handle & 0xFF,
                  _mesa_hash_table_insert(bo_size_table(fd), (void*)(uintptr_t)create->handle,
               }
      static int
   exec_and_check_padding(int fd, unsigned long request,
         {
      int ret = libc_ioctl(fd, request, exec);
   if (ret != 0)
            struct drm_i915_gem_exec_object2 *objects =
         uint32_t batch_bo = exec->flags & I915_EXEC_BATCH_FIRST ? objects[0].handle :
            struct drm_i915_gem_wait wait = {
      .bo_handle = batch_bo,
      };
   ret = libc_ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
   if (ret != 0)
                     for (int i = 0; i < exec->buffer_count; i++) {
               if (!padding_is_good(fd, handle)) {
      detected_out_of_bounds_write = true;
                  if (unlikely(detected_out_of_bounds_write)) {
                     }
      static int
   gem_close(int fd, struct drm_gem_close *close)
   {
      int ret = libc_ioctl(fd, DRM_IOCTL_GEM_CLOSE, close);
   if (ret != 0)
            struct hash_table *t = bo_size_table(fd);
   struct hash_entry *e =
            if (e)
               }
      static bool
   is_i915(int fd) {
      struct stat stat;
   if (fstat(fd, &stat))
            if (!S_ISCHR(stat.st_mode) || major(stat.st_rdev) != DRM_MAJOR)
            char name[5] = "";
   drm_version_t version = {
      .name = name,
      };
   if (libc_ioctl(fd, DRM_IOCTL_VERSION, &version))
               }
      __attribute__ ((visibility ("default"))) int
   open(const char *path, int flags, ...)
   {
      va_list args;
            va_start(args, flags);
   mode = va_arg(args, int);
                              if (fd >= 0 && is_i915(fd))
                        }
      __attribute__ ((visibility ("default"), alias ("open"))) int
   open64(const char *path, int flags, ...);
      __attribute__ ((visibility ("default"))) int
   close(int fd)
   {
               if (is_drm_fd(fd))
                        }
      __attribute__ ((visibility ("default"))) int
   fcntl(int fd, int cmd, ...)
   {
      va_list args;
            va_start(args, cmd);
   param = va_arg(args, int);
                              if (is_drm_fd(fd) && cmd == F_DUPFD_CLOEXEC)
                        }
      __attribute__ ((visibility ("default"))) int
   ioctl(int fd, unsigned long request, ...)
   {
      int res;
   va_list args;
                     va_start(args, request);
   argp = va_arg(args, void *);
            if (_IOC_TYPE(request) == DRM_IOCTL_BASE && !is_drm_fd(fd) && is_i915(fd)) {
      mesa_loge("missed drm fd %d", fd);
               if (is_drm_fd(fd)) {
      switch (request) {
   case DRM_IOCTL_GEM_CLOSE:
                  case DRM_IOCTL_I915_GEM_CREATE:
                  case DRM_IOCTL_I915_GEM_EXECBUFFER2:
   case DRM_IOCTL_I915_GEM_EXECBUFFER2_WR:
      res = exec_and_check_padding(fd, request,
               default:
            }
         out:
      MUTEX_UNLOCK();
      }
      static void __attribute__ ((constructor))
   init(void)
   {
      fds_to_bo_sizes = _mesa_pointer_hash_table_create(NULL);
   libc_open = dlsym(RTLD_NEXT, "open");
   libc_close = dlsym(RTLD_NEXT, "close");
   libc_fcntl = dlsym(RTLD_NEXT, "fcntl");
      }
