   /*
   * Copyright © 2023 Intel Corporation
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
      #include <sys/mman.h>
      #include "anv_private.h"
      #include "i915/anv_batch_chain.h"
      #include "drm-uapi/i915_drm.h"
   #include "intel/common/i915/intel_gem.h"
      static int
   i915_gem_set_caching(struct anv_device *device,
         {
      struct drm_i915_gem_caching gem_caching = {
      .handle = gem_handle,
                  }
      static uint32_t
   i915_gem_create(struct anv_device *device,
                  const struct intel_memory_class_instance **regions,
      {
      if (unlikely(!device->info->mem.use_class_instance)) {
      assert(num_regions == 1 &&
            struct drm_i915_gem_create gem_create = {
         };
   if (intel_ioctl(device->fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create))
            if (alloc_flags & ANV_BO_ALLOC_SNOOPED) {
      /* We don't want to change these defaults if it's going to be shared
   * with another process.
                  /* Regular objects are created I915_CACHING_CACHED on LLC platforms and
   * I915_CACHING_NONE on non-LLC platforms.  For many internal state
   * objects, we'd rather take the snooping overhead than risk forgetting
   * a CLFLUSH somewhere.  Userptr objects are always created as
   * I915_CACHING_CACHED, which on non-LLC means snooped so there's no
   * need to do this there.
   */
   if (device->info->has_caching_uapi && !device->info->has_llc)
               *actual_size = gem_create.size;
               struct drm_i915_gem_memory_class_instance i915_regions[2];
            for (uint16_t i = 0; i < num_regions; i++) {
      i915_regions[i].memory_class = regions[i]->klass;
               uint32_t flags = 0;
   if (alloc_flags & (ANV_BO_ALLOC_MAPPED | ANV_BO_ALLOC_LOCAL_MEM_CPU_VISIBLE) &&
      !(alloc_flags & ANV_BO_ALLOC_NO_LOCAL_MEM))
   if (device->physical->vram_non_mappable.size > 0)
         struct drm_i915_gem_create_ext_memory_regions ext_regions = {
      .base = { .name = I915_GEM_CREATE_EXT_MEMORY_REGIONS },
   .num_regions = num_regions,
      };
   struct drm_i915_gem_create_ext gem_create = {
      .size = size,
   .extensions = (uintptr_t) &ext_regions,
               struct drm_i915_gem_create_ext_set_pat set_pat_param = { 0 };
   if (device->info->has_set_pat_uapi) {
      /* Set PAT param */
   if (alloc_flags & (ANV_BO_ALLOC_SNOOPED))
         else if (alloc_flags & (ANV_BO_ALLOC_EXTERNAL | ANV_BO_ALLOC_SCANOUT))
         else
         intel_i915_gem_add_ext(&gem_create.extensions,
                     if (intel_ioctl(device->fd, DRM_IOCTL_I915_GEM_CREATE_EXT, &gem_create))
                     if (alloc_flags & ANV_BO_ALLOC_SNOOPED) {
      assert(alloc_flags & ANV_BO_ALLOC_MAPPED);
   /* We don't want to change these defaults if it's going to be shared
   * with another process.
   */
            /* Regular objects are created I915_CACHING_CACHED on LLC platforms and
   * I915_CACHING_NONE on non-LLC platforms.  For many internal state
   * objects, we'd rather take the snooping overhead than risk forgetting
   * a CLFLUSH somewhere.  Userptr objects are always created as
   * I915_CACHING_CACHED, which on non-LLC means snooped so there's no
   * need to do this there.
   */
   if (device->info->has_caching_uapi && !device->info->has_llc)
                  }
      static void
   i915_gem_close(struct anv_device *device, struct anv_bo *bo)
   {
      struct drm_gem_close close = {
                     }
      static void *
   i915_gem_mmap_offset(struct anv_device *device, struct anv_bo *bo,
         {
      struct drm_i915_gem_mmap_offset gem_mmap = {
      .handle = bo->gem_handle,
      };
   if (intel_ioctl(device->fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &gem_mmap))
            return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
      }
      static void *
   i915_gem_mmap_legacy(struct anv_device *device, struct anv_bo *bo, uint64_t offset,
         {
      struct drm_i915_gem_mmap gem_mmap = {
      .handle = bo->gem_handle,
   .offset = offset,
   .size = size,
      };
   if (intel_ioctl(device->fd, DRM_IOCTL_I915_GEM_MMAP, &gem_mmap))
               }
      static uint32_t
   mmap_calc_flags(struct anv_device *device, struct anv_bo *bo,
         {
      if (device->info->has_local_mem)
            uint32_t flags = 0;
   if (!device->info->has_llc &&
      (property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
      if (!(property_flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT))
            if (likely(device->physical->info.has_mmap_offset))
            }
      static void *
   i915_gem_mmap(struct anv_device *device, struct anv_bo *bo, uint64_t offset,
         {
               if (likely(device->physical->info.has_mmap_offset))
            }
      static int
   i915_vm_bind(struct anv_device *device, int num_binds,
         {
         }
      static int
   i915_vm_bind_bo(struct anv_device *device, struct anv_bo *bo)
   {
         }
      static uint32_t
   i915_gem_create_userptr(struct anv_device *device, void *mem, uint64_t size)
   {
      struct drm_i915_gem_userptr userptr = {
      .user_ptr = (__u64)((unsigned long) mem),
   .user_size = size,
               if (device->physical->info.has_userptr_probe)
            int ret = intel_ioctl(device->fd, DRM_IOCTL_I915_GEM_USERPTR, &userptr);
   if (ret == -1)
               }
      static uint32_t
   i915_bo_alloc_flags_to_bo_flags(struct anv_device *device,
         {
                        if (!(alloc_flags & ANV_BO_ALLOC_32BIT_ADDRESS))
            if (((alloc_flags & ANV_BO_ALLOC_CAPTURE) ||
      INTEL_DEBUG(DEBUG_CAPTURE_ALL)) &&
   pdevice->has_exec_capture)
         if (alloc_flags & ANV_BO_ALLOC_IMPLICIT_WRITE) {
      assert(alloc_flags & ANV_BO_ALLOC_IMPLICIT_SYNC);
               if (!(alloc_flags & ANV_BO_ALLOC_IMPLICIT_SYNC) && pdevice->has_exec_async)
               }
      const struct anv_kmd_backend *
   anv_i915_kmd_backend_get(void)
   {
      static const struct anv_kmd_backend i915_backend = {
      .gem_create = i915_gem_create,
   .gem_create_userptr = i915_gem_create_userptr,
   .gem_close = i915_gem_close,
   .gem_mmap = i915_gem_mmap,
   .vm_bind = i915_vm_bind,
   .vm_bind_bo = i915_vm_bind_bo,
   .vm_unbind_bo = i915_vm_bind_bo,
   .execute_simple_batch = i915_execute_simple_batch,
   .queue_exec_locked = i915_queue_exec_locked,
   .queue_exec_trace = i915_queue_exec_trace,
      };
      }
