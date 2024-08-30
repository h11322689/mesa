   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   *
   * based on amdgpu winsys.
   * Copyright © 2011 Marek Olšák <maraeo@gmail.com>
   * Copyright © 2015 Advanced Micro Devices, Inc.
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
      #include <stdio.h>
      #include "radv_amdgpu_bo.h"
   #include "radv_debug.h"
      #include <amdgpu.h>
   #include <inttypes.h>
   #include <pthread.h>
   #include <unistd.h>
   #include "drm-uapi/amdgpu_drm.h"
      #include "util/os_time.h"
   #include "util/u_atomic.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      static void radv_amdgpu_winsys_bo_destroy(struct radeon_winsys *_ws, struct radeon_winsys_bo *_bo);
      static int
   radv_amdgpu_bo_va_op(struct radv_amdgpu_winsys *ws, amdgpu_bo_handle bo, uint64_t offset, uint64_t size, uint64_t addr,
         {
      uint64_t flags = internal_flags;
   if (bo) {
               if ((bo_flags & RADEON_FLAG_VA_UNCACHED) && ws->info.gfx_level >= GFX9)
            if (!(bo_flags & RADEON_FLAG_READ_ONLY))
                           }
      static int
   bo_comparator(const void *ap, const void *bp)
   {
      struct radv_amdgpu_bo *a = *(struct radv_amdgpu_bo *const *)ap;
   struct radv_amdgpu_bo *b = *(struct radv_amdgpu_bo *const *)bp;
      }
      static VkResult
   radv_amdgpu_winsys_rebuild_bo_list(struct radv_amdgpu_winsys_bo *bo)
   {
               if (bo->bo_capacity < bo->range_count) {
      uint32_t new_count = MAX2(bo->bo_capacity * 2, bo->range_count);
   struct radv_amdgpu_winsys_bo **bos = realloc(bo->bos, new_count * sizeof(struct radv_amdgpu_winsys_bo *));
   if (!bos) {
      u_rwlock_wrunlock(&bo->lock);
      }
   bo->bos = bos;
               uint32_t temp_bo_count = 0;
   for (uint32_t i = 0; i < bo->range_count; ++i)
      if (bo->ranges[i].bo)
                  if (!temp_bo_count) {
         } else {
      uint32_t final_bo_count = 1;
   for (uint32_t i = 1; i < temp_bo_count; ++i)
                              u_rwlock_wrunlock(&bo->lock);
      }
      static VkResult
   radv_amdgpu_winsys_bo_virtual_bind(struct radeon_winsys *_ws, struct radeon_winsys_bo *_parent, uint64_t offset,
         {
      struct radv_amdgpu_winsys *ws = radv_amdgpu_winsys(_ws);
   struct radv_amdgpu_winsys_bo *parent = (struct radv_amdgpu_winsys_bo *)_parent;
   struct radv_amdgpu_winsys_bo *bo = (struct radv_amdgpu_winsys_bo *)_bo;
   int range_count_delta, new_idx;
   int first = 0, last;
   struct radv_amdgpu_map_range new_first, new_last;
   VkResult result;
            assert(parent->is_virtual);
            /* When the BO is NULL, AMDGPU will reset the PTE VA range to the initial state. Otherwise, it
   * will first unmap all existing VA that overlap the requested range and then map.
   */
   if (bo) {
         } else {
      r =
               if (r) {
      fprintf(stderr, "radv/amdgpu: Failed to replace a PRT VA region (%d).\n", r);
               /* Do not add the BO to the virtual BO list if it's already in the global list to avoid dangling
   * BO references because it might have been destroyed without being previously unbound. Resetting
   * it to NULL clears the old BO ranges if present.
   *
   * This is going to be clarified in the Vulkan spec:
   * https://gitlab.khronos.org/vulkan/vulkan/-/issues/3125
   *
   * The issue still exists for non-global BO but it will be addressed later, once we are 100% it's
   * RADV fault (mostly because the solution looks more complicated).
   */
   if (bo && bo->base.use_global_list) {
      bo = NULL;
               /* We have at most 2 new ranges (1 by the bind, and another one by splitting a range that
   * contains the newly bound range). */
   if (parent->range_capacity - parent->range_count < 2) {
      uint32_t range_capacity = parent->range_capacity + 2;
   struct radv_amdgpu_map_range *ranges =
         if (!ranges)
         parent->ranges = ranges;
               /*
   * [first, last] is exactly the range of ranges that either overlap the
   * new parent, or are adjacent to it. This corresponds to the bind ranges
   * that may change.
   */
   while (first + 1 < parent->range_count && parent->ranges[first].offset + parent->ranges[first].size < offset)
            last = first;
   while (last + 1 < parent->range_count && parent->ranges[last + 1].offset <= offset + size)
            /* Whether the first or last range are going to be totally removed or just
   * resized/left alone. Note that in the case of first == last, we will split
   * this into a part before and after the new range. The remove flag is then
   * whether to not create the corresponding split part. */
   bool remove_first = parent->ranges[first].offset == offset;
            assert(parent->ranges[first].offset <= offset);
            /* Try to merge the new range with the first range. */
   if (parent->ranges[first].bo == bo &&
      (!bo || offset - bo_offset == parent->ranges[first].offset - parent->ranges[first].bo_offset)) {
   size += offset - parent->ranges[first].offset;
   offset = parent->ranges[first].offset;
   bo_offset = parent->ranges[first].bo_offset;
               /* Try to merge the new range with the last range. */
   if (parent->ranges[last].bo == bo &&
      (!bo || offset - bo_offset == parent->ranges[last].offset - parent->ranges[last].bo_offset)) {
   size = parent->ranges[last].offset + parent->ranges[last].size - offset;
               range_count_delta = 1 - (last - first + 1) + !remove_first + !remove_last;
            /* If the first/last range are not left alone we unmap then and optionally map
   * them again after modifications. Not that this implicitly can do the splitting
   * if first == last. */
   new_first = parent->ranges[first];
            if (parent->ranges[first].offset + parent->ranges[first].size > offset || remove_first) {
      if (!remove_first) {
                     if (parent->ranges[last].offset < offset + size || remove_last) {
      if (!remove_last) {
      new_last.size -= offset + size - new_last.offset;
   new_last.bo_offset += (offset + size - new_last.offset);
                  /* Moves the range list after last to account for the changed number of ranges. */
   memmove(parent->ranges + last + 1 + range_count_delta, parent->ranges + last + 1,
            if (!remove_first)
            if (!remove_last)
            /* Actually set up the new range. */
   parent->ranges[new_idx].offset = offset;
   parent->ranges[new_idx].size = size;
   parent->ranges[new_idx].bo = bo;
                     result = radv_amdgpu_winsys_rebuild_bo_list(parent);
   if (result != VK_SUCCESS)
               }
      struct radv_amdgpu_winsys_bo_log {
      struct list_head list;
   uint64_t va;
   uint64_t size;
   uint64_t timestamp; /* CPU timestamp */
   uint8_t is_virtual : 1;
      };
      static void
   radv_amdgpu_log_bo(struct radv_amdgpu_winsys *ws, struct radv_amdgpu_winsys_bo *bo, bool destroyed)
   {
               if (!ws->debug_log_bos)
            bo_log = malloc(sizeof(*bo_log));
   if (!bo_log)
            bo_log->va = bo->base.va;
   bo_log->size = bo->size;
   bo_log->timestamp = os_time_get_nano();
   bo_log->is_virtual = bo->is_virtual;
            u_rwlock_wrlock(&ws->log_bo_list_lock);
   list_addtail(&bo_log->list, &ws->log_bo_list);
      }
      static int
   radv_amdgpu_global_bo_list_add(struct radv_amdgpu_winsys *ws, struct radv_amdgpu_winsys_bo *bo)
   {
      u_rwlock_wrlock(&ws->global_bo_list.lock);
   if (ws->global_bo_list.count == ws->global_bo_list.capacity) {
      unsigned capacity = MAX2(4, ws->global_bo_list.capacity * 2);
   void *data = realloc(ws->global_bo_list.bos, capacity * sizeof(struct radv_amdgpu_winsys_bo *));
   if (!data) {
      u_rwlock_wrunlock(&ws->global_bo_list.lock);
               ws->global_bo_list.bos = (struct radv_amdgpu_winsys_bo **)data;
               ws->global_bo_list.bos[ws->global_bo_list.count++] = bo;
   bo->base.use_global_list = true;
   u_rwlock_wrunlock(&ws->global_bo_list.lock);
      }
      static void
   radv_amdgpu_global_bo_list_del(struct radv_amdgpu_winsys *ws, struct radv_amdgpu_winsys_bo *bo)
   {
      u_rwlock_wrlock(&ws->global_bo_list.lock);
   for (unsigned i = ws->global_bo_list.count; i-- > 0;) {
      if (ws->global_bo_list.bos[i] == bo) {
      ws->global_bo_list.bos[i] = ws->global_bo_list.bos[ws->global_bo_list.count - 1];
   --ws->global_bo_list.count;
   bo->base.use_global_list = false;
         }
      }
      static void
   radv_amdgpu_winsys_bo_destroy(struct radeon_winsys *_ws, struct radeon_winsys_bo *_bo)
   {
      struct radv_amdgpu_winsys *ws = radv_amdgpu_winsys(_ws);
                     if (bo->is_virtual) {
               /* Clear mappings of this PRT VA region. */
   r = radv_amdgpu_bo_va_op(ws, NULL, 0, bo->size, bo->base.va, 0, 0, AMDGPU_VA_OP_CLEAR);
   if (r) {
                  free(bo->bos);
   free(bo->ranges);
      } else {
      if (ws->debug_all_bos)
         radv_amdgpu_bo_va_op(ws, bo->bo, 0, bo->size, bo->base.va, 0, 0, AMDGPU_VA_OP_UNMAP);
               if (bo->base.initial_domain & RADEON_DOMAIN_VRAM) {
      if (bo->base.vram_no_cpu_access) {
         } else {
                     if (bo->base.initial_domain & RADEON_DOMAIN_GTT)
            amdgpu_va_range_free(bo->va_handle);
      }
      static VkResult
   radv_amdgpu_winsys_bo_create(struct radeon_winsys *_ws, uint64_t size, unsigned alignment,
               {
      struct radv_amdgpu_winsys *ws = radv_amdgpu_winsys(_ws);
   struct radv_amdgpu_winsys_bo *bo;
   struct amdgpu_bo_alloc_request request = {0};
   struct radv_amdgpu_map_range *ranges = NULL;
   amdgpu_bo_handle buf_handle;
   uint64_t va = 0;
   amdgpu_va_handle va_handle;
   int r;
            /* Just be robust for callers that might use NULL-ness for determining if things should be freed.
   */
            bo = CALLOC_STRUCT(radv_amdgpu_winsys_bo);
   if (!bo) {
                  unsigned virt_alignment = alignment;
   if (size >= ws->info.pte_fragment_size)
                     const uint64_t va_flags = AMDGPU_VA_RANGE_HIGH | (flags & RADEON_FLAG_32BIT ? AMDGPU_VA_RANGE_32_BIT : 0) |
         r = amdgpu_va_range_alloc(ws->dev, amdgpu_gpu_va_range_general, size, virt_alignment, replay_address, &va,
         if (r) {
      result = replay_address ? VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS : VK_ERROR_OUT_OF_DEVICE_MEMORY;
               bo->base.va = va;
   bo->va_handle = va_handle;
   bo->size = size;
            if (flags & RADEON_FLAG_VIRTUAL) {
      ranges = realloc(NULL, sizeof(struct radv_amdgpu_map_range));
   if (!ranges) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
                        bo->ranges = ranges;
   bo->range_count = 1;
            bo->ranges[0].offset = 0;
   bo->ranges[0].size = size;
   bo->ranges[0].bo = NULL;
            /* Reserve a PRT VA region. */
   r = radv_amdgpu_bo_va_op(ws, NULL, 0, size, bo->base.va, 0, AMDGPU_VM_PAGE_PRT, AMDGPU_VA_OP_MAP);
   if (r) {
      fprintf(stderr, "radv/amdgpu: Failed to reserve a PRT VA region (%d).\n", r);
   result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
                        *out_bo = (struct radeon_winsys_bo *)bo;
               request.alloc_size = size;
            if (initial_domain & RADEON_DOMAIN_VRAM) {
               /* Since VRAM and GTT have almost the same performance on
   * APUs, we could just set GTT. However, in order to decrease
   * GTT(RAM) usage, which is shared with the OS, allow VRAM
   * placements too. The idea is not to use VRAM usefully, but
   * to use it so that it's not unused and wasted.
   *
   * Furthermore, even on discrete GPUs this is beneficial. If
   * both GTT and VRAM are set then AMDGPU still prefers VRAM
   * for the initial placement, but it makes the buffers
   * spillable. Otherwise AMDGPU tries to place the buffers in
   * VRAM really hard to the extent that we are getting a lot
   * of unnecessary movement. This helps significantly when
   * e.g. Horizon Zero Dawn allocates more memory than we have
   * VRAM.
   */
               if (initial_domain & RADEON_DOMAIN_GTT)
         if (initial_domain & RADEON_DOMAIN_GDS)
         if (initial_domain & RADEON_DOMAIN_OA)
            if (flags & RADEON_FLAG_CPU_ACCESS)
         if (flags & RADEON_FLAG_NO_CPU_ACCESS) {
      bo->base.vram_no_cpu_access = initial_domain & RADEON_DOMAIN_VRAM;
      }
   if (flags & RADEON_FLAG_GTT_WC)
         if (!(flags & RADEON_FLAG_IMPLICIT_SYNC))
         if ((initial_domain & RADEON_DOMAIN_VRAM_GTT) && (flags & RADEON_FLAG_NO_INTERPROCESS_SHARING) &&
      ((ws->perftest & RADV_PERFTEST_LOCAL_BOS) || (flags & RADEON_FLAG_PREFER_LOCAL_BO))) {
   bo->base.is_local = true;
               if (initial_domain & RADEON_DOMAIN_VRAM) {
      if (ws->zero_all_vram_allocs || (flags & RADEON_FLAG_ZERO_VRAM))
               if (flags & RADEON_FLAG_DISCARDABLE && ws->info.drm_minor >= 47)
            r = amdgpu_bo_alloc(ws->dev, &request, &buf_handle);
   if (r) {
      fprintf(stderr, "radv/amdgpu: Failed to allocate a buffer:\n");
   fprintf(stderr, "radv/amdgpu:    size      : %" PRIu64 " bytes\n", size);
   fprintf(stderr, "radv/amdgpu:    alignment : %u bytes\n", alignment);
   fprintf(stderr, "radv/amdgpu:    domains   : %u\n", initial_domain);
   result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
               r = radv_amdgpu_bo_va_op(ws, buf_handle, 0, size, va, flags, 0, AMDGPU_VA_OP_MAP);
   if (r) {
      result = VK_ERROR_UNKNOWN;
               bo->bo = buf_handle;
   bo->base.initial_domain = initial_domain;
   bo->base.use_global_list = bo->base.is_local;
            r = amdgpu_bo_export(buf_handle, amdgpu_bo_handle_type_kms, &bo->bo_handle);
            if (initial_domain & RADEON_DOMAIN_VRAM) {
      /* Buffers allocated in VRAM with the NO_CPU_ACCESS flag
   * aren't mappable and they are counted as part of the VRAM
   * counter.
   *
   * Otherwise, buffers with the CPU_ACCESS flag or without any
   * of both (imported buffers) are counted as part of the VRAM
   * visible counter because they can be mapped.
   */
   if (bo->base.vram_no_cpu_access) {
         } else {
                     if (initial_domain & RADEON_DOMAIN_GTT)
            if (ws->debug_all_bos)
                  *out_bo = (struct radeon_winsys_bo *)bo;
      error_va_map:
            error_bo_alloc:
            error_ranges_alloc:
            error_va_alloc:
      FREE(bo);
      }
      static void *
   radv_amdgpu_winsys_bo_map(struct radeon_winsys_bo *_bo)
   {
      struct radv_amdgpu_winsys_bo *bo = radv_amdgpu_winsys_bo(_bo);
   int ret;
   void *data;
   ret = amdgpu_bo_cpu_map(bo->bo, &data);
   if (ret)
            }
      static void
   radv_amdgpu_winsys_bo_unmap(struct radeon_winsys_bo *_bo)
   {
      struct radv_amdgpu_winsys_bo *bo = radv_amdgpu_winsys_bo(_bo);
      }
      static uint64_t
   radv_amdgpu_get_optimal_vm_alignment(struct radv_amdgpu_winsys *ws, uint64_t size, unsigned alignment)
   {
               /* Increase the VM alignment for faster address translation. */
   if (size >= ws->info.pte_fragment_size)
            /* Gfx9: Increase the VM alignment to the most significant bit set
   * in the size for faster address translation.
   */
   if (ws->info.gfx_level >= GFX9) {
      unsigned msb = util_last_bit64(size); /* 0 = no bit is set */
               }
      }
      static VkResult
   radv_amdgpu_winsys_bo_from_ptr(struct radeon_winsys *_ws, void *pointer, uint64_t size, unsigned priority,
         {
      struct radv_amdgpu_winsys *ws = radv_amdgpu_winsys(_ws);
   amdgpu_bo_handle buf_handle;
   struct radv_amdgpu_winsys_bo *bo;
   uint64_t va;
   amdgpu_va_handle va_handle;
   uint64_t vm_alignment;
   VkResult result = VK_SUCCESS;
            /* Just be robust for callers that might use NULL-ness for determining if things should be freed.
   */
            bo = CALLOC_STRUCT(radv_amdgpu_winsys_bo);
   if (!bo)
            ret = amdgpu_create_bo_from_user_mem(ws->dev, pointer, size, &buf_handle);
   if (ret) {
      if (ret == -EINVAL) {
         } else {
         }
               /* Using the optimal VM alignment also fixes GPU hangs for buffers that
   * are imported.
   */
            if (amdgpu_va_range_alloc(ws->dev, amdgpu_gpu_va_range_general, size, vm_alignment, 0, &va, &va_handle,
            result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
               if (amdgpu_bo_va_op(buf_handle, 0, size, va, 0, AMDGPU_VA_OP_MAP)) {
      result = VK_ERROR_UNKNOWN;
               /* Initialize it */
   bo->base.va = va;
   bo->va_handle = va_handle;
   bo->size = size;
   bo->bo = buf_handle;
   bo->base.initial_domain = RADEON_DOMAIN_GTT;
   bo->base.use_global_list = false;
            ASSERTED int r = amdgpu_bo_export(buf_handle, amdgpu_bo_handle_type_kms, &bo->bo_handle);
                     if (ws->debug_all_bos)
                  *out_bo = (struct radeon_winsys_bo *)bo;
         error_va_map:
            error_va_alloc:
            error:
      FREE(bo);
      }
      static VkResult
   radv_amdgpu_winsys_bo_from_fd(struct radeon_winsys *_ws, int fd, unsigned priority, struct radeon_winsys_bo **out_bo,
         {
      struct radv_amdgpu_winsys *ws = radv_amdgpu_winsys(_ws);
   struct radv_amdgpu_winsys_bo *bo;
   uint64_t va;
   amdgpu_va_handle va_handle;
   enum amdgpu_bo_handle_type type = amdgpu_bo_handle_type_dma_buf_fd;
   struct amdgpu_bo_import_result result;
   struct amdgpu_bo_info info;
   enum radeon_bo_domain initial = 0;
   int r;
            /* Just be robust for callers that might use NULL-ness for determining if things should be freed.
   */
            bo = CALLOC_STRUCT(radv_amdgpu_winsys_bo);
   if (!bo)
            r = amdgpu_bo_import(ws->dev, type, fd, &result);
   if (r) {
      vk_result = VK_ERROR_INVALID_EXTERNAL_HANDLE;
               r = amdgpu_bo_query_info(result.buf_handle, &info);
   if (r) {
      vk_result = VK_ERROR_UNKNOWN;
               if (alloc_size) {
                  r = amdgpu_va_range_alloc(ws->dev, amdgpu_gpu_va_range_general, result.alloc_size, 1 << 20, 0, &va, &va_handle,
         if (r) {
      vk_result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
               r = radv_amdgpu_bo_va_op(ws, result.buf_handle, 0, result.alloc_size, va, 0, 0, AMDGPU_VA_OP_MAP);
   if (r) {
      vk_result = VK_ERROR_UNKNOWN;
               if (info.preferred_heap & AMDGPU_GEM_DOMAIN_VRAM)
         if (info.preferred_heap & AMDGPU_GEM_DOMAIN_GTT)
            bo->bo = result.buf_handle;
   bo->base.va = va;
   bo->va_handle = va_handle;
   bo->base.initial_domain = initial;
   bo->base.use_global_list = false;
   bo->size = result.alloc_size;
            r = amdgpu_bo_export(result.buf_handle, amdgpu_bo_handle_type_kms, &bo->bo_handle);
            if (bo->base.initial_domain & RADEON_DOMAIN_VRAM)
         if (bo->base.initial_domain & RADEON_DOMAIN_GTT)
            if (ws->debug_all_bos)
                  *out_bo = (struct radeon_winsys_bo *)bo;
      error_va_map:
            error_query:
            error:
      FREE(bo);
      }
      static bool
   radv_amdgpu_winsys_get_fd(struct radeon_winsys *_ws, struct radeon_winsys_bo *_bo, int *fd)
   {
      struct radv_amdgpu_winsys_bo *bo = radv_amdgpu_winsys_bo(_bo);
   enum amdgpu_bo_handle_type type = amdgpu_bo_handle_type_dma_buf_fd;
   int r;
   unsigned handle;
   r = amdgpu_bo_export(bo->bo, type, &handle);
   if (r)
            *fd = (int)handle;
      }
      static bool
   radv_amdgpu_bo_get_flags_from_fd(struct radeon_winsys *_ws, int fd, enum radeon_bo_domain *domains,
         {
      struct radv_amdgpu_winsys *ws = radv_amdgpu_winsys(_ws);
   struct amdgpu_bo_import_result result = {0};
   struct amdgpu_bo_info info = {0};
            *domains = 0;
            r = amdgpu_bo_import(ws->dev, amdgpu_bo_handle_type_dma_buf_fd, fd, &result);
   if (r)
            r = amdgpu_bo_query_info(result.buf_handle, &info);
   amdgpu_bo_free(result.buf_handle);
   if (r)
            if (info.preferred_heap & AMDGPU_GEM_DOMAIN_VRAM)
         if (info.preferred_heap & AMDGPU_GEM_DOMAIN_GTT)
         if (info.preferred_heap & AMDGPU_GEM_DOMAIN_GDS)
         if (info.preferred_heap & AMDGPU_GEM_DOMAIN_OA)
            if (info.alloc_flags & AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED)
         if (info.alloc_flags & AMDGPU_GEM_CREATE_NO_CPU_ACCESS)
         if (!(info.alloc_flags & AMDGPU_GEM_CREATE_EXPLICIT_SYNC))
         if (info.alloc_flags & AMDGPU_GEM_CREATE_CPU_GTT_USWC)
         if (info.alloc_flags & AMDGPU_GEM_CREATE_VM_ALWAYS_VALID)
         if (info.alloc_flags & AMDGPU_GEM_CREATE_VRAM_CLEARED)
            }
      static unsigned
   eg_tile_split(unsigned tile_split)
   {
      switch (tile_split) {
   case 0:
      tile_split = 64;
      case 1:
      tile_split = 128;
      case 2:
      tile_split = 256;
      case 3:
      tile_split = 512;
      default:
   case 4:
      tile_split = 1024;
      case 5:
      tile_split = 2048;
      case 6:
      tile_split = 4096;
      }
      }
      static unsigned
   radv_eg_tile_split_rev(unsigned eg_tile_split)
   {
      switch (eg_tile_split) {
   case 64:
         case 128:
         case 256:
         case 512:
         default:
   case 1024:
         case 2048:
         case 4096:
            }
      #define AMDGPU_TILING_DCC_MAX_COMPRESSED_BLOCK_SIZE_SHIFT 45
   #define AMDGPU_TILING_DCC_MAX_COMPRESSED_BLOCK_SIZE_MASK  0x3
      static void
   radv_amdgpu_winsys_bo_set_metadata(struct radeon_winsys *_ws, struct radeon_winsys_bo *_bo,
         {
      struct radv_amdgpu_winsys *ws = radv_amdgpu_winsys(_ws);
   struct radv_amdgpu_winsys_bo *bo = radv_amdgpu_winsys_bo(_bo);
   struct amdgpu_bo_metadata metadata = {0};
            if (ws->info.gfx_level >= GFX9) {
      tiling_flags |= AMDGPU_TILING_SET(SWIZZLE_MODE, md->u.gfx9.swizzle_mode);
   tiling_flags |= AMDGPU_TILING_SET(DCC_OFFSET_256B, md->u.gfx9.dcc_offset_256b);
   tiling_flags |= AMDGPU_TILING_SET(DCC_PITCH_MAX, md->u.gfx9.dcc_pitch_max);
   tiling_flags |= AMDGPU_TILING_SET(DCC_INDEPENDENT_64B, md->u.gfx9.dcc_independent_64b_blocks);
   tiling_flags |= AMDGPU_TILING_SET(DCC_INDEPENDENT_128B, md->u.gfx9.dcc_independent_128b_blocks);
   tiling_flags |= AMDGPU_TILING_SET(DCC_MAX_COMPRESSED_BLOCK_SIZE, md->u.gfx9.dcc_max_compressed_block_size);
      } else {
      if (md->u.legacy.macrotile == RADEON_LAYOUT_TILED)
         else if (md->u.legacy.microtile == RADEON_LAYOUT_TILED)
         else
            tiling_flags |= AMDGPU_TILING_SET(PIPE_CONFIG, md->u.legacy.pipe_config);
   tiling_flags |= AMDGPU_TILING_SET(BANK_WIDTH, util_logbase2(md->u.legacy.bankw));
   tiling_flags |= AMDGPU_TILING_SET(BANK_HEIGHT, util_logbase2(md->u.legacy.bankh));
   if (md->u.legacy.tile_split)
         tiling_flags |= AMDGPU_TILING_SET(MACRO_TILE_ASPECT, util_logbase2(md->u.legacy.mtilea));
            if (md->u.legacy.scanout)
         else
               metadata.tiling_info = tiling_flags;
   metadata.size_metadata = md->size_metadata;
               }
      static void
   radv_amdgpu_winsys_bo_get_metadata(struct radeon_winsys *_ws, struct radeon_winsys_bo *_bo,
         {
      struct radv_amdgpu_winsys *ws = radv_amdgpu_winsys(_ws);
   struct radv_amdgpu_winsys_bo *bo = radv_amdgpu_winsys_bo(_bo);
            int r = amdgpu_bo_query_info(bo->bo, &info);
   if (r)
                     if (ws->info.gfx_level >= GFX9) {
      md->u.gfx9.swizzle_mode = AMDGPU_TILING_GET(tiling_flags, SWIZZLE_MODE);
      } else {
      md->u.legacy.microtile = RADEON_LAYOUT_LINEAR;
            if (AMDGPU_TILING_GET(tiling_flags, ARRAY_MODE) == 4) /* 2D_TILED_THIN1 */
         else if (AMDGPU_TILING_GET(tiling_flags, ARRAY_MODE) == 2) /* 1D_TILED_THIN1 */
            md->u.legacy.pipe_config = AMDGPU_TILING_GET(tiling_flags, PIPE_CONFIG);
   md->u.legacy.bankw = 1 << AMDGPU_TILING_GET(tiling_flags, BANK_WIDTH);
   md->u.legacy.bankh = 1 << AMDGPU_TILING_GET(tiling_flags, BANK_HEIGHT);
   md->u.legacy.tile_split = eg_tile_split(AMDGPU_TILING_GET(tiling_flags, TILE_SPLIT));
   md->u.legacy.mtilea = 1 << AMDGPU_TILING_GET(tiling_flags, MACRO_TILE_ASPECT);
   md->u.legacy.num_banks = 2 << AMDGPU_TILING_GET(tiling_flags, NUM_BANKS);
               md->size_metadata = info.metadata.size_metadata;
      }
      static VkResult
   radv_amdgpu_winsys_bo_make_resident(struct radeon_winsys *_ws, struct radeon_winsys_bo *_bo, bool resident)
   {
      struct radv_amdgpu_winsys *ws = radv_amdgpu_winsys(_ws);
   struct radv_amdgpu_winsys_bo *bo = radv_amdgpu_winsys_bo(_bo);
            /* Do not add the BO to the global list if it's a local BO because the
   * kernel maintains a list for us.
   */
   if (bo->base.is_local)
            /* Do not add the BO twice to the global list if the allbos debug
   * option is enabled.
   */
   if (ws->debug_all_bos)
            if (resident) {
         } else {
                     }
      static int
   radv_amdgpu_bo_va_compare(const void *a, const void *b)
   {
      const struct radv_amdgpu_winsys_bo *bo_a = *(const struct radv_amdgpu_winsys_bo *const *)a;
   const struct radv_amdgpu_winsys_bo *bo_b = *(const struct radv_amdgpu_winsys_bo *const *)b;
      }
      static uint64_t
   radv_amdgpu_canonicalize_va(uint64_t va)
   {
      /* Would be less hardcoded to use addr32_hi (0xffff8000) to generate a mask,
   * but there are confusing differences between page fault reports from kernel where
   * it seems to report the top 48 bits, where addr32_hi has 47-bits. */
      }
      static void
   radv_amdgpu_dump_bo_log(struct radeon_winsys *_ws, FILE *file)
   {
      struct radv_amdgpu_winsys *ws = radv_amdgpu_winsys(_ws);
            if (!ws->debug_log_bos)
            u_rwlock_rdlock(&ws->log_bo_list_lock);
   LIST_FOR_EACH_ENTRY (bo_log, &ws->log_bo_list, list) {
      fprintf(file, "timestamp=%llu, VA=%.16llx-%.16llx, destroyed=%d, is_virtual=%d\n", (long long)bo_log->timestamp,
            }
      }
      static void
   radv_amdgpu_dump_bo_ranges(struct radeon_winsys *_ws, FILE *file)
   {
      struct radv_amdgpu_winsys *ws = radv_amdgpu_winsys(_ws);
   if (ws->debug_all_bos) {
      struct radv_amdgpu_winsys_bo **bos = NULL;
            u_rwlock_rdlock(&ws->global_bo_list.lock);
   bos = malloc(sizeof(*bos) * ws->global_bo_list.count);
   if (!bos) {
      u_rwlock_rdunlock(&ws->global_bo_list.lock);
   fprintf(file, "  Failed to allocate memory to sort VA ranges for dumping\n");
               for (i = 0; i < ws->global_bo_list.count; i++) {
         }
            for (i = 0; i < ws->global_bo_list.count; ++i) {
      fprintf(file, "  VA=%.16llx-%.16llx, handle=%d%s\n", (long long)radv_amdgpu_canonicalize_va(bos[i]->base.va),
            }
   free(bos);
      } else
      }
   void
   radv_amdgpu_bo_init_functions(struct radv_amdgpu_winsys *ws)
   {
      ws->base.buffer_create = radv_amdgpu_winsys_bo_create;
   ws->base.buffer_destroy = radv_amdgpu_winsys_bo_destroy;
   ws->base.buffer_map = radv_amdgpu_winsys_bo_map;
   ws->base.buffer_unmap = radv_amdgpu_winsys_bo_unmap;
   ws->base.buffer_from_ptr = radv_amdgpu_winsys_bo_from_ptr;
   ws->base.buffer_from_fd = radv_amdgpu_winsys_bo_from_fd;
   ws->base.buffer_get_fd = radv_amdgpu_winsys_get_fd;
   ws->base.buffer_set_metadata = radv_amdgpu_winsys_bo_set_metadata;
   ws->base.buffer_get_metadata = radv_amdgpu_winsys_bo_get_metadata;
   ws->base.buffer_virtual_bind = radv_amdgpu_winsys_bo_virtual_bind;
   ws->base.buffer_get_flags_from_fd = radv_amdgpu_bo_get_flags_from_fd;
   ws->base.buffer_make_resident = radv_amdgpu_winsys_bo_make_resident;
   ws->base.dump_bo_ranges = radv_amdgpu_dump_bo_ranges;
      }
