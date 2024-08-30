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
   #include "iris_kmd_backend.h"
      #include <sys/mman.h>
      #include "common/intel_gem.h"
   #include "dev/intel_debug.h"
   #include "iris/iris_bufmgr.h"
   #include "iris/iris_batch.h"
   #include "iris/iris_context.h"
      #include "drm-uapi/xe_drm.h"
      #define FILE_DEBUG_FLAG DEBUG_BUFMGR
      static uint32_t
   xe_gem_create(struct iris_bufmgr *bufmgr,
               const struct intel_memory_class_instance **regions,
   {
      /* Xe still don't have support for protected content */
   if (alloc_flags & BO_ALLOC_PROTECTED)
            uint32_t vm_id = iris_bufmgr_get_global_vm_id(bufmgr);
            uint32_t flags = 0;
   /* TODO: we might need to consider scanout for shared buffers too as we
   * do not know what the process this is shared with will do with it
   */
   if (alloc_flags & BO_ALLOC_SCANOUT)
         if (!intel_vram_all_mappable(iris_bufmgr_get_device_info(bufmgr)) &&
      heap_flags == IRIS_HEAP_DEVICE_LOCAL_PREFERRED)
         struct drm_xe_gem_create gem_create = {
   .vm_id = vm_id,
   .size = align64(size, iris_bufmgr_get_device_info(bufmgr)->mem_alignment),
   .flags = flags,
   };
   for (uint16_t i = 0; i < regions_count; i++)
            if (intel_ioctl(iris_bufmgr_get_fd(bufmgr), DRM_IOCTL_XE_GEM_CREATE,
                     }
      static void *
   xe_gem_mmap(struct iris_bufmgr *bufmgr, struct iris_bo *bo)
   {
      struct drm_xe_gem_mmap_offset args = {
         };
   if (intel_ioctl(iris_bufmgr_get_fd(bufmgr), DRM_IOCTL_XE_GEM_MMAP_OFFSET, &args))
            void *map = mmap(NULL, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
            }
      static inline int
   xe_gem_vm_bind_op(struct iris_bo *bo, uint32_t op)
   {
      uint32_t handle = op == XE_VM_BIND_OP_UNMAP ? 0 : bo->gem_handle;
   uint64_t range, obj_offset = 0;
            if (iris_bo_is_imported(bo))
         else
      range = align64(bo->size,
         if (bo->real.userptr) {
      handle = 0;
   obj_offset = (uintptr_t)bo->real.map;
   if (op == XE_VM_BIND_OP_MAP)
               struct drm_xe_vm_bind args = {
      .vm_id = iris_bufmgr_get_global_vm_id(bo->bufmgr),
   .num_binds = 1,
   .bind.obj = handle,
   .bind.obj_offset = obj_offset,
   .bind.range = range,
   .bind.addr = intel_48b_address(bo->address),
      };
   ret = intel_ioctl(iris_bufmgr_get_fd(bo->bufmgr), DRM_IOCTL_XE_VM_BIND, &args);
   if (ret) {
                     }
      static bool
   xe_gem_vm_bind(struct iris_bo *bo)
   {
         }
      static bool
   xe_gem_vm_unbind(struct iris_bo *bo)
   {
         }
      static bool
   xe_bo_madvise(struct iris_bo *bo, enum iris_madvice state)
   {
      /* Only applicable if VM was created with DRM_XE_VM_CREATE_FAULT_MODE but
   * that is not compatible with DRM_XE_VM_CREATE_SCRATCH_PAGE
   *
   * So returning as retained.
   */
      }
      static int
   xe_bo_set_caching(struct iris_bo *bo, bool cached)
   {
      /* Xe don't have caching UAPI so this function should never be called */
   assert(0);
      }
      static enum pipe_reset_status
   xe_batch_check_for_reset(struct iris_batch *batch)
   {
      enum pipe_reset_status status = PIPE_NO_RESET;
   struct drm_xe_exec_queue_get_property exec_queue_get_property = {
      .exec_queue_id = batch->xe.exec_queue_id,
      };
   int ret = intel_ioctl(iris_bufmgr_get_fd(batch->screen->bufmgr),
                  if (ret || exec_queue_get_property.value)
               }
      static uint32_t
   xe_batch_submit_external_bo_count(struct iris_batch *batch)
   {
               for (int i = 0; i < batch->exec_count; i++) {
      if (iris_bo_is_external(batch->exec_bos[i]))
                  }
      struct iris_implicit_sync {
      struct iris_implicit_sync_entry {
      struct iris_bo *bo;
      } *entries;
               };
      static bool
   iris_implicit_sync_add_bo(struct iris_batch *batch,
               {
               if (!syncobj)
            sync->entries[sync->entry_count].bo = bo;
   sync->entries[sync->entry_count].iris_syncobj = syncobj;
                        }
      /* Cleans up the state of 'sync'. */
   static void
   iris_implicit_sync_finish(struct iris_batch *batch,
         {
               for (int i = 0; i < sync->entry_count; i++)
            free(sync->entries);
      }
      /* Import implicit synchronization data from the batch bos that require
   * implicit synchronization int our batch buffer so the batch will wait for
   * these bos to be idle before starting.
   */
   static int
   iris_implicit_sync_import(struct iris_batch *batch,
         {
               if (!len)
            sync->entries = malloc(sizeof(*sync->entries) * len);
   if (!sync->entries)
            for (int i = 0; i < batch->exec_count; i++) {
               if (!iris_bo_is_real(bo) || !iris_bo_is_external(bo)) {
      assert(iris_get_backing_bo(bo)->real.prime_fd == -1);
               if (bo->real.prime_fd == -1) {
      fprintf(stderr, "Bo(%s/%i %sported) with prime_fd unset in iris_implicit_sync_import()\n",
                     if (!iris_implicit_sync_add_bo(batch, sync, bo)) {
      iris_implicit_sync_finish(batch, sync);
                     }
      /* Export implicit synchronization data from our batch buffer into the bos
   * that require implicit synchronization so other clients relying on it can do
   * implicit synchronization with these bos, which will wait for the batch
   * buffer we just submitted to signal its syncobj.
   */
   static bool
   iris_implicit_sync_export(struct iris_batch *batch,
         {
               if (!iris_batch_syncobj_to_sync_file_fd(batch, &sync_file_fd))
            for (int i = 0; i < sync->entry_count; i++)
                        }
      static int
   xe_batch_submit(struct iris_batch *batch)
   {
      struct iris_bufmgr *bufmgr = batch->screen->bufmgr;
   simple_mtx_t *bo_deps_lock = iris_bufmgr_get_bo_deps_lock(bufmgr);
   struct iris_implicit_sync implicit_sync = {};
   struct drm_xe_sync *syncs = NULL;
   unsigned long sync_len;
                     /* The decode operation may map and wait on the batch buffer, which could
   * in theory try to grab bo_deps_lock. Let's keep it safe and decode
   * outside the lock.
   */
   if (INTEL_DEBUG(DEBUG_BATCH) &&
      intel_debug_batch_in_range(batch->ice->frame))
                           ret = iris_implicit_sync_import(batch, &implicit_sync);
   if (ret)
            sync_len = iris_batch_num_fences(batch);
   if (sync_len) {
               syncs = calloc(sync_len, sizeof(*syncs));
   if (!syncs) {
      ret = -ENOMEM;
               util_dynarray_foreach(&batch->exec_fences, struct iris_batch_fence,
                                    syncs[i].handle = fence->handle;
   syncs[i].flags = flags;
                  if ((INTEL_DEBUG(DEBUG_BATCH) &&
      intel_debug_batch_in_range(batch->ice->frame)) ||
   INTEL_DEBUG(DEBUG_SUBMIT)) {
   iris_dump_fence_list(batch);
               struct drm_xe_exec exec = {
      .exec_queue_id = batch->xe.exec_queue_id,
   .num_batch_buffer = 1,
   .address = batch->exec_bos[0]->address,
   .syncs = (uintptr_t)syncs,
      };
   if (!batch->screen->devinfo->no_hw)
            if (ret) {
      ret = -errno;
               if (!iris_implicit_sync_export(batch, &implicit_sync))
         error_exec:
                                 for (int i = 0; i < batch->exec_count; i++) {
               bo->idle = false;
                                       error_no_sync_mem:
         error_implicit_sync_import:
      simple_mtx_unlock(bo_deps_lock);
      }
      static int
   xe_gem_close(struct iris_bufmgr *bufmgr, struct iris_bo *bo)
   {
      if (bo->real.userptr)
            struct drm_gem_close close = {
         };
      }
      static uint32_t
   xe_gem_create_userptr(struct iris_bufmgr *bufmgr, void *ptr, uint64_t size)
   {
      /* We return UINT32_MAX, because Xe doesn't create handles for userptrs but
   * it needs a gem_handle different than 0 so iris_bo_is_real() returns true
   * for userptr bos.
   * UINT32_MAX handle here will not conflict with an actual gem handle with
   * same id as userptr bos are not put to slab or bo cache.
   */
      }
      const struct iris_kmd_backend *xe_get_backend(void)
   {
      static const struct iris_kmd_backend xe_backend = {
      .gem_create = xe_gem_create,
   .gem_create_userptr = xe_gem_create_userptr,
   .gem_close = xe_gem_close,
   .gem_mmap = xe_gem_mmap,
   .gem_vm_bind = xe_gem_vm_bind,
   .gem_vm_unbind = xe_gem_vm_unbind,
   .bo_madvise = xe_bo_madvise,
   .bo_set_caching = xe_bo_set_caching,
   .batch_check_for_reset = xe_batch_check_for_reset,
      };
      }
