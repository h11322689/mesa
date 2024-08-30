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
   #include "common/intel_gem.h"
      #include "i915/anv_gem.h"
      void *
   anv_gem_mmap(struct anv_device *device, struct anv_bo *bo, uint64_t offset,
         {
      void *map = device->kmd_backend->gem_mmap(device, bo, offset, size,
            if (map != MAP_FAILED)
               }
      /* This is just a wrapper around munmap, but it also notifies valgrind that
   * this map is no longer valid.  Pair this with gem_mmap().
   */
   void
   anv_gem_munmap(struct anv_device *device, void *p, uint64_t size)
   {
      VG(VALGRIND_FREELIKE_BLOCK(p, 0));
      }
      /**
   * On error, \a timeout_ns holds the remaining time.
   */
   int
   anv_gem_wait(struct anv_device *device, uint32_t gem_handle, int64_t *timeout_ns)
   {
      switch (device->info->kmd_type) {
   case INTEL_KMD_TYPE_I915:
         case INTEL_KMD_TYPE_XE:
         default:
      unreachable("missing");
         }
      /** Return -1 on error. */
   int
   anv_gem_get_tiling(struct anv_device *device, uint32_t gem_handle)
   {
      switch (device->info->kmd_type) {
   case INTEL_KMD_TYPE_I915:
         case INTEL_KMD_TYPE_XE:
         default:
      unreachable("missing");
         }
      int
   anv_gem_set_tiling(struct anv_device *device,
         {
      switch (device->info->kmd_type) {
   case INTEL_KMD_TYPE_I915:
         case INTEL_KMD_TYPE_XE:
         default:
      unreachable("missing");
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
      VkResult
   anv_gem_import_bo_alloc_flags_to_bo_flags(struct anv_device *device,
                     {
      switch (device->info->kmd_type) {
   case INTEL_KMD_TYPE_I915:
      return anv_i915_gem_import_bo_alloc_flags_to_bo_flags(device, bo,
            case INTEL_KMD_TYPE_XE:
      *bo_flags = device->kmd_backend->bo_alloc_flags_to_bo_flags(device, alloc_flags);
      default:
      unreachable("missing");
         }
      const struct anv_kmd_backend *anv_stub_kmd_backend_get(void)
   {
         }
