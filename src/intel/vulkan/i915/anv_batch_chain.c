   /*
   * Copyright © 2022 Intel Corporation
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
      #include "i915/anv_batch_chain.h"
   #include "anv_private.h"
   #include "anv_measure.h"
      #include "perf/intel_perf.h"
   #include "util/u_debug.h"
      #include "drm-uapi/i915_drm.h"
      struct anv_execbuf {
                        struct drm_i915_gem_exec_object2 *        objects;
   uint32_t                                  bo_count;
   uint32_t                                  bo_array_length;
            uint32_t                                  syncobj_count;
   uint32_t                                  syncobj_array_length;
   struct drm_i915_gem_exec_fence *          syncobjs;
            uint32_t                                  cmd_buffer_count;
            const VkAllocationCallbacks *             alloc;
               };
      static void
   anv_execbuf_finish(struct anv_execbuf *exec)
   {
      vk_free(exec->alloc, exec->syncobjs);
   vk_free(exec->alloc, exec->syncobj_values);
   vk_free(exec->alloc, exec->objects);
      }
      static void
   anv_execbuf_add_ext(struct anv_execbuf *exec,
               {
                        while (*iter != 0) {
                              }
      static VkResult
   anv_execbuf_add_bo_bitset(struct anv_device *device,
                              static VkResult
   anv_execbuf_add_bo(struct anv_device *device,
                     struct anv_execbuf *exec,
   {
               if (bo->exec_obj_index < exec->bo_count &&
      exec->bos[bo->exec_obj_index] == bo)
         if (obj == NULL) {
      /* We've never seen this one before.  Add it to the list and assign
   * an id that we can use later.
   */
   if (exec->bo_count >= exec->bo_array_length) {
               struct drm_i915_gem_exec_object2 *new_objects =
      vk_realloc(exec->alloc, exec->objects,
                              struct anv_bo **new_bos =
      vk_realloc(exec->alloc, exec->bos, new_len * sizeof(*new_bos), 8,
                     exec->bos = new_bos;
                        bo->exec_obj_index = exec->bo_count++;
   obj = &exec->objects[bo->exec_obj_index];
            obj->handle = bo->gem_handle;
   obj->relocation_count = 0;
   obj->relocs_ptr = 0;
   obj->alignment = 0;
   obj->offset = bo->offset;
   obj->flags = bo->flags | extra_flags;
   obj->rsvd1 = 0;
               if (extra_flags & EXEC_OBJECT_WRITE) {
      obj->flags |= EXEC_OBJECT_WRITE;
               if (relocs != NULL) {
      return anv_execbuf_add_bo_bitset(device, exec, relocs->dep_words,
                  }
      /* Add BO dependencies to execbuf */
   static VkResult
   anv_execbuf_add_bo_bitset(struct anv_device *device,
                           {
      for (uint32_t w = 0; w < dep_words; w++) {
      BITSET_WORD mask = deps[w];
   while (mask) {
      int i = u_bit_scan(&mask);
   uint32_t gem_handle = w * BITSET_WORDBITS + i;
   struct anv_bo *bo = anv_device_lookup_bo(device, gem_handle);
   assert(bo->refcount > 0);
   VkResult result =
         if (result != VK_SUCCESS)
                     }
      static VkResult
   anv_execbuf_add_syncobj(struct anv_device *device,
                           {
      if (exec->syncobj_count >= exec->syncobj_array_length) {
               struct drm_i915_gem_exec_fence *new_syncobjs =
      vk_realloc(exec->alloc, exec->syncobjs,
      if (new_syncobjs == NULL)
                     if (exec->syncobj_values) {
      uint64_t *new_syncobj_values =
      vk_realloc(exec->alloc, exec->syncobj_values,
                                                   if (timeline_value && !exec->syncobj_values) {
      exec->syncobj_values =
      vk_zalloc(exec->alloc, exec->syncobj_array_length *
            if (!exec->syncobj_values)
               exec->syncobjs[exec->syncobj_count] = (struct drm_i915_gem_exec_fence) {
      .handle = syncobj,
      };
   if (exec->syncobj_values)
                        }
      static VkResult
   anv_execbuf_add_sync(struct anv_device *device,
                           {
      /* It's illegal to signal a timeline with value 0 because that's never
   * higher than the current value.  A timeline wait on value 0 is always
   * trivial because 0 <= uint64_t always.
   */
   if ((sync->flags & VK_SYNC_IS_TIMELINE) && value == 0)
            if (vk_sync_is_anv_bo_sync(sync)) {
      struct anv_bo_sync *bo_sync =
                     return anv_execbuf_add_bo(device, execbuf, bo_sync->bo, NULL,
      } else if (vk_sync_type_is_drm_syncobj(sync->type)) {
               if (!(sync->flags & VK_SYNC_IS_TIMELINE))
            return anv_execbuf_add_syncobj(device, execbuf, syncobj->syncobj,
                              }
      static VkResult
   setup_execbuf_for_cmd_buffer(struct anv_execbuf *execbuf,
         {
      VkResult result;
   /* Add surface dependencies (BOs) to the execbuf */
   result = anv_execbuf_add_bo_bitset(cmd_buffer->device, execbuf,
               if (result != VK_SUCCESS)
            /* First, we walk over all of the bos we've seen and add them and their
   * relocations to the validate list.
   */
   struct anv_batch_bo **bbo;
   u_vector_foreach(bbo, &cmd_buffer->seen_bbos) {
      result = anv_execbuf_add_bo(cmd_buffer->device, execbuf,
         if (result != VK_SUCCESS)
               struct anv_bo **bo_entry;
   u_vector_foreach(bo_entry, &cmd_buffer->dynamic_bos) {
      result = anv_execbuf_add_bo(cmd_buffer->device, execbuf,
         if (result != VK_SUCCESS)
                  }
      static VkResult
   pin_state_pool(struct anv_device *device,
               {
      anv_block_pool_foreach_bo(bo, &pool->block_pool) {
      VkResult result = anv_execbuf_add_bo(device, execbuf, bo, NULL, 0);
   if (result != VK_SUCCESS)
                  }
      static void
   get_context_and_exec_flags(struct anv_queue *queue,
                     {
                        /** Submit batch to index 0 which is the main virtual engine */
            *context_id = device->physical->has_vm_control ?
               is_companion_rcs_batch ?
      }
      static VkResult
   setup_execbuf_for_cmd_buffers(struct anv_execbuf *execbuf,
                           {
      struct anv_device *device = queue->device;
            /* Edit the tail of the command buffers to chain them all together if they
   * can be.
   */
            for (uint32_t i = 0; i < num_cmd_buffers; i++) {
      struct anv_cmd_buffer *cmd_buf =
      is_companion_rcs_cmd_buffer ?
      anv_measure_submit(cmd_buf);
   result = setup_execbuf_for_cmd_buffer(execbuf, cmd_buf);
   if (result != VK_SUCCESS)
               /* Add all the global BOs to the object list for softpin case. */
   result = pin_state_pool(device, execbuf, &device->scratch_surface_state_pool);
   if (result != VK_SUCCESS)
            if (device->physical->va.bindless_surface_state_pool.size > 0) {
      result = pin_state_pool(device, execbuf, &device->bindless_surface_state_pool);
   if (result != VK_SUCCESS)
               if (device->physical->va.push_descriptor_pool.size > 0) {
      result = pin_state_pool(device, execbuf, &device->push_descriptor_pool);
   if (result != VK_SUCCESS)
               result = pin_state_pool(device, execbuf, &device->internal_surface_state_pool);
   if (result != VK_SUCCESS)
            result = pin_state_pool(device, execbuf, &device->dynamic_state_pool);
   if (result != VK_SUCCESS)
            result = pin_state_pool(device, execbuf, &device->general_state_pool);
   if (result != VK_SUCCESS)
            result = pin_state_pool(device, execbuf, &device->instruction_state_pool);
   if (result != VK_SUCCESS)
            result = pin_state_pool(device, execbuf, &device->binding_table_pool);
   if (result != VK_SUCCESS)
            /* Add the BOs for all user allocated memory objects because we can't
   * track after binding updates of VK_EXT_descriptor_indexing.
   */
   list_for_each_entry(struct anv_device_memory, mem,
            result = anv_execbuf_add_bo(device, execbuf, mem->bo, NULL, 0);
   if (result != VK_SUCCESS)
               /* Add all the private BOs from images because we can't track after binding
   * updates of VK_EXT_descriptor_indexing.
   */
   list_for_each_entry(struct anv_image, image,
            struct anv_bo *private_bo =
         result = anv_execbuf_add_bo(device, execbuf, private_bo, NULL, 0);
   if (result != VK_SUCCESS)
               struct list_head *batch_bo =
      is_companion_rcs_cmd_buffer && cmd_buffers[0]->companion_rcs_cmd_buffer ?
   &cmd_buffers[0]->companion_rcs_cmd_buffer->batch_bos :
      struct anv_batch_bo *first_batch_bo =
            /* The kernel requires that the last entry in the validation list be the
   * batch buffer to execute.  We can simply swap the element
   * corresponding to the first batch_bo in the chain with the last
   * element in the list.
   */
   if (first_batch_bo->bo->exec_obj_index != execbuf->bo_count - 1) {
      uint32_t idx = first_batch_bo->bo->exec_obj_index;
            struct drm_i915_gem_exec_object2 tmp_obj = execbuf->objects[idx];
            execbuf->objects[idx] = execbuf->objects[last_idx];
   execbuf->bos[idx] = execbuf->bos[last_idx];
            execbuf->objects[last_idx] = tmp_obj;
   execbuf->bos[last_idx] = first_batch_bo->bo;
            #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
      if (device->physical->memory.need_flush)
      #endif
         assert(!is_companion_rcs_cmd_buffer || device->physical->has_vm_control);
   uint64_t exec_flags = 0;
   uint32_t context_id;
   get_context_and_exec_flags(queue, is_companion_rcs_cmd_buffer, &exec_flags,
            execbuf->execbuf = (struct drm_i915_gem_execbuffer2) {
      .buffers_ptr = (uintptr_t) execbuf->objects,
   .buffer_count = execbuf->bo_count,
   .batch_start_offset = 0,
   .batch_len = 0,
   .cliprects_ptr = 0,
   .num_cliprects = 0,
   .DR1 = 0,
   .DR4 = 0,
   .flags = I915_EXEC_NO_RELOC |
               .rsvd1 = context_id,
                  }
      static VkResult
   setup_empty_execbuf(struct anv_execbuf *execbuf, struct anv_queue *queue)
   {
      struct anv_device *device = queue->device;
   VkResult result = anv_execbuf_add_bo(device, execbuf,
               if (result != VK_SUCCESS)
            uint64_t exec_flags = 0;
   uint32_t context_id;
            execbuf->execbuf = (struct drm_i915_gem_execbuffer2) {
      .buffers_ptr = (uintptr_t) execbuf->objects,
   .buffer_count = execbuf->bo_count,
   .batch_start_offset = 0,
   .batch_len = 8, /* GFX7_MI_BATCH_BUFFER_END and NOOP */
   .flags = I915_EXEC_HANDLE_LUT | exec_flags | I915_EXEC_NO_RELOC,
   .rsvd1 = context_id,
                  }
      static VkResult
   setup_utrace_execbuf(struct anv_execbuf *execbuf, struct anv_queue *queue,
         {
               /* Always add the workaround BO as it includes a driver identifier for the
   * error_state.
   */
   VkResult result = anv_execbuf_add_bo(device, execbuf,
               if (result != VK_SUCCESS)
            result = anv_execbuf_add_bo(device, execbuf,
               if (result != VK_SUCCESS)
            result = anv_execbuf_add_sync(device, execbuf, submit->sync,
         if (result != VK_SUCCESS)
            if (submit->batch_bo->exec_obj_index != execbuf->bo_count - 1) {
      uint32_t idx = submit->batch_bo->exec_obj_index;
            struct drm_i915_gem_exec_object2 tmp_obj = execbuf->objects[idx];
            execbuf->objects[idx] = execbuf->objects[last_idx];
   execbuf->bos[idx] = execbuf->bos[last_idx];
            execbuf->objects[last_idx] = tmp_obj;
   execbuf->bos[last_idx] = submit->batch_bo;
            #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
      if (device->physical->memory.need_flush)
      #endif
         uint64_t exec_flags = 0;
   uint32_t context_id;
            execbuf->execbuf = (struct drm_i915_gem_execbuffer2) {
      .buffers_ptr = (uintptr_t) execbuf->objects,
   .buffer_count = execbuf->bo_count,
   .batch_start_offset = 0,
   .batch_len = submit->batch.next - submit->batch.start,
   .flags = I915_EXEC_NO_RELOC |
            I915_EXEC_HANDLE_LUT |
      .rsvd1 = context_id,
   .rsvd2 = 0,
   .num_cliprects = execbuf->syncobj_count,
                  }
      static int
   anv_gem_execbuffer(struct anv_device *device,
         {
      int ret;
   const unsigned long request = (execbuf->flags & I915_EXEC_FENCE_OUT) ?
      DRM_IOCTL_I915_GEM_EXECBUFFER2_WR :
         do {
                     }
      static VkResult
   anv_queue_exec_utrace_locked(struct anv_queue *queue,
         {
               struct anv_device *device = queue->device;
   struct anv_execbuf execbuf = {
      .alloc = &device->vk.alloc,
               VkResult result = setup_utrace_execbuf(&execbuf, queue, submit);
   if (result != VK_SUCCESS)
            int ret = queue->device->info->no_hw ? 0 :
         if (ret)
         error:
                  }
      static void
   anv_i915_debug_submit(const struct anv_execbuf *execbuf)
   {
      uint32_t total_size_kb = 0, total_vram_only_size_kb = 0;
   for (uint32_t i = 0; i < execbuf->bo_count; i++) {
      const struct anv_bo *bo = execbuf->bos[i];
   total_size_kb += bo->size / 1024;
   if (bo->vram_only)
               fprintf(stderr, "Batch offset=0x%x len=0x%x on queue 0 (aperture: %.1fMb, %.1fMb VRAM only)\n",
         execbuf->execbuf.batch_start_offset, execbuf->execbuf.batch_len,
   (float)total_size_kb / 1024.0f,
   for (uint32_t i = 0; i < execbuf->bo_count; i++) {
               fprintf(stderr, "   BO: addr=0x%016"PRIx64"-0x%016"PRIx64" size=%7"PRIu64
         "KB handle=%05u capture=%u vram_only=%u name=%s\n",
   bo->offset, bo->offset + bo->size - 1, bo->size / 1024,
         }
      static VkResult
   i915_companion_rcs_queue_exec_locked(struct anv_queue *queue,
                           {
      struct anv_device *device = queue->device;
   struct anv_execbuf execbuf = {
      .alloc = &queue->device->vk.alloc,
               /* Always add the workaround BO as it includes a driver identifier for the
   * error_state.
   */
   VkResult result =
         if (result != VK_SUCCESS)
            for (uint32_t i = 0; i < wait_count; i++) {
      result = anv_execbuf_add_sync(device, &execbuf,
                     if (result != VK_SUCCESS)
               if (queue->companion_sync) {
      result = anv_execbuf_add_sync(device, &execbuf,
               if (result != VK_SUCCESS)
               result = setup_execbuf_for_cmd_buffers(&execbuf, queue, cmd_buffers,
               if (result != VK_SUCCESS)
            if (INTEL_DEBUG(DEBUG_SUBMIT))
            anv_cmd_buffer_exec_batch_debug(queue, cmd_buffer_count, cmd_buffers,
            if (execbuf.syncobj_values) {
      execbuf.timeline_fences.fence_count = execbuf.syncobj_count;
   execbuf.timeline_fences.handles_ptr = (uintptr_t)execbuf.syncobjs;
   execbuf.timeline_fences.values_ptr = (uintptr_t)execbuf.syncobj_values;
   anv_execbuf_add_ext(&execbuf,
            } else if (execbuf.syncobjs) {
      execbuf.execbuf.flags |= I915_EXEC_FENCE_ARRAY;
   execbuf.execbuf.num_cliprects = execbuf.syncobj_count;
               int ret = queue->device->info->no_hw ? 0 :
         if (ret) {
      anv_i915_debug_submit(&execbuf);
            error:
      anv_execbuf_finish(&execbuf);
      }
      VkResult
   i915_queue_exec_locked(struct anv_queue *queue,
                        uint32_t wait_count,
   const struct vk_sync_wait *waits,
   uint32_t cmd_buffer_count,
   struct anv_cmd_buffer **cmd_buffers,
   uint32_t signal_count,
      {
      struct anv_device *device = queue->device;
   struct anv_execbuf execbuf = {
      .alloc = &queue->device->vk.alloc,
   .alloc_scope = VK_SYSTEM_ALLOCATION_SCOPE_DEVICE,
      };
            if (utrace_submit && !utrace_submit->batch_bo) {
      result = anv_execbuf_add_sync(device, &execbuf,
                     if (result != VK_SUCCESS)
            /* When The utrace submission doesn't have its own batch buffer*/
               /* Always add the workaround BO as it includes a driver identifier for the
   * error_state.
   */
   result =
         if (result != VK_SUCCESS)
            for (uint32_t i = 0; i < wait_count; i++) {
      result = anv_execbuf_add_sync(device, &execbuf,
                     if (result != VK_SUCCESS)
               for (uint32_t i = 0; i < signal_count; i++) {
      result = anv_execbuf_add_sync(device, &execbuf,
                     if (result != VK_SUCCESS)
               if (queue->sync) {
      result = anv_execbuf_add_sync(device, &execbuf,
                     if (result != VK_SUCCESS)
               if (cmd_buffer_count) {
      result = setup_execbuf_for_cmd_buffers(&execbuf, queue,
                  } else {
                  if (result != VK_SUCCESS)
            const bool has_perf_query =
            if (INTEL_DEBUG(DEBUG_SUBMIT))
            anv_cmd_buffer_exec_batch_debug(queue, cmd_buffer_count, cmd_buffers,
                  if (execbuf.syncobj_values) {
      execbuf.timeline_fences.fence_count = execbuf.syncobj_count;
   execbuf.timeline_fences.handles_ptr = (uintptr_t)execbuf.syncobjs;
   execbuf.timeline_fences.values_ptr = (uintptr_t)execbuf.syncobj_values;
   anv_execbuf_add_ext(&execbuf,
            } else if (execbuf.syncobjs) {
      execbuf.execbuf.flags |= I915_EXEC_FENCE_ARRAY;
   execbuf.execbuf.num_cliprects = execbuf.syncobj_count;
               if (has_perf_query) {
      assert(perf_query_pass < perf_query_pool->n_passes);
   struct intel_perf_query_info *query_info =
            /* Some performance queries just the pipeline statistic HW, no need for
   * OA in that case, so no need to reconfigure.
   */
   if (!INTEL_DEBUG(DEBUG_NO_OACONFIG) &&
      (query_info->kind == INTEL_PERF_QUERY_TYPE_OA ||
   query_info->kind == INTEL_PERF_QUERY_TYPE_RAW)) {
   int ret = intel_ioctl(device->perf_fd, I915_PERF_IOCTL_CONFIG,
         if (ret < 0) {
      result = vk_device_set_lost(&device->vk,
                                 struct drm_i915_gem_exec_object2 query_pass_object = {
      .handle = pass_batch_bo->gem_handle,
   .offset = pass_batch_bo->offset,
               uint64_t exec_flags = 0;
   uint32_t context_id;
            struct drm_i915_gem_execbuffer2 query_pass_execbuf = {
      .buffers_ptr = (uintptr_t) &query_pass_object,
   .buffer_count = 1,
   .batch_start_offset = khr_perf_query_preamble_offset(perf_query_pool,
         .flags = I915_EXEC_HANDLE_LUT | exec_flags,
               int ret = queue->device->info->no_hw ? 0 :
         if (ret)
               int ret = queue->device->info->no_hw ? 0 :
         if (ret) {
      anv_i915_debug_submit(&execbuf);
               if (cmd_buffer_count != 0 && cmd_buffers[0]->companion_rcs_cmd_buffer) {
      struct anv_cmd_buffer *companion_rcs_cmd_buffer =
         assert(companion_rcs_cmd_buffer->is_companion_rcs_cmd_buffer);
   result = i915_companion_rcs_queue_exec_locked(queue, cmd_buffer_count,
                     if (result == VK_SUCCESS && queue->sync) {
      result = vk_sync_wait(&device->vk, queue->sync, 0,
         if (result != VK_SUCCESS)
            error:
               if (result == VK_SUCCESS && utrace_submit)
               }
      VkResult
   i915_execute_simple_batch(struct anv_queue *queue, struct anv_bo *batch_bo,
         {
      struct anv_device *device = queue->device;
   struct anv_execbuf execbuf = {
      .alloc = &queue->device->vk.alloc,
               VkResult result = anv_execbuf_add_bo(device, &execbuf, batch_bo, NULL, 0);
   if (result != VK_SUCCESS)
            assert(!is_companion_rcs_batch || device->physical->has_vm_control);
   uint64_t exec_flags = 0;
   uint32_t context_id;
   get_context_and_exec_flags(queue, is_companion_rcs_batch, &exec_flags,
            execbuf.execbuf = (struct drm_i915_gem_execbuffer2) {
      .buffers_ptr = (uintptr_t) execbuf.objects,
   .buffer_count = execbuf.bo_count,
   .batch_start_offset = 0,
   .batch_len = batch_bo_size,
   .flags = I915_EXEC_HANDLE_LUT | exec_flags | I915_EXEC_NO_RELOC,
   .rsvd1 = context_id,
               if (anv_gem_execbuffer(device, &execbuf.execbuf)) {
      result = vk_device_set_lost(&device->vk, "anv_gem_execbuffer failed: %m");
               result = anv_device_wait(device, batch_bo, INT64_MAX);
   if (result != VK_SUCCESS)
      result = vk_device_set_lost(&device->vk,
      fail:
      anv_execbuf_finish(&execbuf);
      }
      VkResult
   i915_queue_exec_trace(struct anv_queue *queue,
         {
                  }
