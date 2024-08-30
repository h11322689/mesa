   /*
   * Copyright © 2015 Intel Corporation
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
      #include <sys/ioctl.h>
   #include <sys/types.h>
   #include <sys/mman.h>
   #include <string.h>
   #include <errno.h>
   #include <unistd.h>
   #include <fcntl.h>
      #include "anv_private.h"
   #include "common/intel_defines.h"
   #include "common/intel_gem.h"
      /**
   * Wrapper around DRM_IOCTL_I915_GEM_CREATE.
   *
   * Return gem handle, or 0 on failure. Gem handles are never 0.
   */
   uint32_t
   anv_gem_create(struct anv_device *device, uint64_t size)
   {
      struct drm_i915_gem_create gem_create = {
                  int ret = intel_ioctl(device->fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
   if (ret != 0) {
      /* FIXME: What do we do if this fails? */
                  }
      void
   anv_gem_close(struct anv_device *device, uint32_t gem_handle)
   {
      struct drm_gem_close close = {
                     }
      /**
   * Wrapper around DRM_IOCTL_I915_GEM_MMAP. Returns MAP_FAILED on error.
   */
   static void*
   anv_gem_mmap_offset(struct anv_device *device, uint32_t gem_handle,
         {
      struct drm_i915_gem_mmap_offset gem_mmap = {
      .handle = gem_handle,
   .flags = device->info->has_local_mem ? I915_MMAP_OFFSET_FIXED :
      };
            /* Get the fake offset back */
   int ret = intel_ioctl(device->fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &gem_mmap);
   if (ret != 0)
            /* And map it */
   void *map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
            }
      static void*
   anv_gem_mmap_legacy(struct anv_device *device, uint32_t gem_handle,
         {
               struct drm_i915_gem_mmap gem_mmap = {
      .handle = gem_handle,
   .offset = offset,
   .size = size,
               int ret = intel_ioctl(device->fd, DRM_IOCTL_I915_GEM_MMAP, &gem_mmap);
   if (ret != 0)
               }
      /**
   * Wrapper around DRM_IOCTL_I915_GEM_MMAP. Returns MAP_FAILED on error.
   */
   void*
   anv_gem_mmap(struct anv_device *device, uint32_t gem_handle,
         {
      void *map;
   if (device->physical->info.has_mmap_offset)
         else
            if (map != MAP_FAILED)
               }
      /* This is just a wrapper around munmap, but it also notifies valgrind that
   * this map is no longer valid.  Pair this with anv_gem_mmap().
   */
   void
   anv_gem_munmap(struct anv_device *device, void *p, uint64_t size)
   {
      VG(VALGRIND_FREELIKE_BLOCK(p, 0));
      }
      uint32_t
   anv_gem_userptr(struct anv_device *device, void *mem, size_t size)
   {
      struct drm_i915_gem_userptr userptr = {
      .user_ptr = (__u64)((unsigned long) mem),
   .user_size = size,
               if (device->physical->info.has_userptr_probe)
            int ret = intel_ioctl(device->fd, DRM_IOCTL_I915_GEM_USERPTR, &userptr);
   if (ret == -1)
               }
      int
   anv_gem_set_caching(struct anv_device *device,
         {
      struct drm_i915_gem_caching gem_caching = {
      .handle = gem_handle,
                  }
      /**
   * On error, \a timeout_ns holds the remaining time.
   */
   int
   anv_gem_wait(struct anv_device *device, uint32_t gem_handle, int64_t *timeout_ns)
   {
      struct drm_i915_gem_wait wait = {
      .bo_handle = gem_handle,
   .timeout_ns = *timeout_ns,
               int ret = intel_ioctl(device->fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
               }
      int
   anv_gem_execbuffer(struct anv_device *device,
         {
      if (execbuf->flags & I915_EXEC_FENCE_OUT)
         else
      }
      /** Return -1 on error. */
   int
   anv_gem_get_tiling(struct anv_device *device, uint32_t gem_handle)
   {
      if (!device->info->has_tiling_uapi)
            struct drm_i915_gem_get_tiling get_tiling = {
                  /* FIXME: On discrete platforms we don't have DRM_IOCTL_I915_GEM_GET_TILING
   * anymore, so we will need another way to get the tiling. Apparently this
   * is only used in Android code, so we may need some other way to
   * communicate the tiling mode.
   */
   if (intel_ioctl(device->fd, DRM_IOCTL_I915_GEM_GET_TILING, &get_tiling)) {
      assert(!"Failed to get BO tiling");
                  }
      int
   anv_gem_set_tiling(struct anv_device *device,
         {
      /* On discrete platforms we don't have DRM_IOCTL_I915_GEM_SET_TILING. So
   * nothing needs to be done.
   */
   if (!device->info->has_tiling_uapi)
            /* set_tiling overwrites the input on the error path, so we have to open
   * code intel_ioctl.
   */
   struct drm_i915_gem_set_tiling set_tiling = {
      .handle = gem_handle,
   .tiling_mode = tiling,
                  }
      bool
   anv_gem_has_context_priority(int fd, int priority)
   {
      return !anv_gem_set_context_param(fd, 0, I915_CONTEXT_PARAM_PRIORITY,
      }
      int
   anv_gem_set_context_param(int fd, uint32_t context, uint32_t param, uint64_t value)
   {
      int err = 0;
   if (!intel_gem_set_context_param(fd, context, param, value))
            }
      int
   anv_gem_context_get_reset_stats(int fd, int context,
         {
      struct drm_i915_reset_stats stats = {
                  int ret = intel_ioctl(fd, DRM_IOCTL_I915_GET_RESET_STATS, &stats);
   if (ret == 0) {
      *active = stats.batch_active;
                  }
      int
   anv_gem_handle_to_fd(struct anv_device *device, uint32_t gem_handle)
   {
      struct drm_prime_handle args = {
      .handle = gem_handle,
               int ret = intel_ioctl(device->fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args);
   if (ret == -1)
               }
      uint32_t
   anv_gem_fd_to_handle(struct anv_device *device, int fd)
   {
      struct drm_prime_handle args = {
                  int ret = intel_ioctl(device->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &args);
   if (ret == -1)
               }
