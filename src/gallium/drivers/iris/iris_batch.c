   /*
   * Copyright © 2017 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * @file iris_batch.c
   *
   * Batchbuffer and command submission module.
   *
   * Every API draw call results in a number of GPU commands, which we
   * collect into a "batch buffer".  Typically, many draw calls are grouped
   * into a single batch to amortize command submission overhead.
   *
   * We submit batches to the kernel using the I915_GEM_EXECBUFFER2 ioctl.
   * One critical piece of data is the "validation list", which contains a
   * list of the buffer objects (BOs) which the commands in the GPU need.
   * The kernel will make sure these are resident and pinned at the correct
   * virtual memory address before executing our batch.  If a BO is not in
   * the validation list, it effectively does not exist, so take care.
   */
      #include "iris_batch.h"
   #include "iris_bufmgr.h"
   #include "iris_context.h"
   #include "iris_fence.h"
   #include "iris_kmd_backend.h"
   #include "iris_utrace.h"
   #include "i915/iris_batch.h"
   #include "xe/iris_batch.h"
      #include "common/intel_aux_map.h"
   #include "common/intel_defines.h"
   #include "intel/common/intel_gem.h"
   #include "intel/ds/intel_tracepoints.h"
   #include "util/hash_table.h"
   #include "util/u_debug.h"
   #include "util/set.h"
   #include "util/u_upload_mgr.h"
      #include <errno.h>
   #include <xf86drm.h>
      #ifdef HAVE_VALGRIND
   #include <valgrind.h>
   #include <memcheck.h>
   #define VG(x) x
   #else
   #define VG(x)
   #endif
      #define FILE_DEBUG_FLAG DEBUG_BUFMGR
      static void
   iris_batch_reset(struct iris_batch *batch);
      unsigned
   iris_batch_num_fences(struct iris_batch *batch)
   {
      return util_dynarray_num_elements(&batch->exec_fences,
      }
      /**
   * Debugging code to dump the fence list, used by INTEL_DEBUG=submit.
   */
   void
   iris_dump_fence_list(struct iris_batch *batch)
   {
               util_dynarray_foreach(&batch->exec_fences, struct iris_batch_fence, f) {
      fprintf(stderr, "%s%u%s ",
         (f->flags & IRIS_BATCH_FENCE_WAIT) ? "..." : "",
                  }
      /**
   * Debugging code to dump the validation list, used by INTEL_DEBUG=submit.
   */
   void
   iris_dump_bo_list(struct iris_batch *batch)
   {
               for (int i = 0; i < batch->exec_count; i++) {
      struct iris_bo *bo = batch->exec_bos[i];
   struct iris_bo *backing = iris_get_backing_bo(bo);
   bool written = BITSET_TEST(batch->bos_written, i);
   bool exported = iris_bo_is_exported(bo);
            fprintf(stderr, "[%2d]: %3d (%3d) %-14s @ 0x%016"PRIx64" (%-15s %8"PRIu64"B) %2d refs %s%s%s\n",
         i,
   bo->gem_handle,
   backing->gem_handle,
   bo->name,
   bo->address,
   iris_heap_to_string[backing->real.heap],
   bo->size,
   bo->refcount,
   written ? " write" : "",
         }
      /**
   * Return BO information to the batch decoder (for debugging).
   */
   static struct intel_batch_decode_bo
   decode_get_bo(void *v_batch, bool ppgtt, uint64_t address)
   {
                        for (int i = 0; i < batch->exec_count; i++) {
      struct iris_bo *bo = batch->exec_bos[i];
   /* The decoder zeroes out the top 16 bits, so we need to as well */
            if (address >= bo_address && address < bo_address + bo->size) {
                     return (struct intel_batch_decode_bo) {
      .addr = bo_address,
   .size = bo->size,
                        }
      static unsigned
   decode_get_state_size(void *v_batch,
               {
      struct iris_batch *batch = v_batch;
   unsigned size = (uintptr_t)
               }
      /**
   * Decode the current batch.
   */
   void
   iris_batch_decode_batch(struct iris_batch *batch)
   {
      void *map = iris_bo_map(batch->dbg, batch->exec_bos[0], MAP_READ);
   intel_print_batch(&batch->decoder, map, batch->primary_batch_size,
      }
      static void
   iris_init_batch(struct iris_context *ice,
         {
      struct iris_batch *batch = &ice->batches[name];
            /* Note: screen, ctx_id, exec_flags and has_engines_context fields are
   * initialized at an earlier phase when contexts are created.
   *
   * See iris_init_batches(), which calls either iris_init_engines_context()
   * or iris_init_non_engine_contexts().
            batch->dbg = &ice->dbg;
   batch->reset = &ice->reset;
   batch->state_sizes = ice->state.sizes;
   batch->name = name;
   batch->ice = ice;
   batch->screen = screen;
            batch->fine_fences.uploader =
      u_upload_create(&ice->ctx, 4096, PIPE_BIND_CUSTOM,
               util_dynarray_init(&batch->exec_fences, ralloc_context(NULL));
            batch->exec_count = 0;
   batch->max_gem_handle = 0;
   batch->exec_array_size = 128;
   batch->exec_bos =
         batch->bos_written =
            batch->bo_aux_modes = _mesa_hash_table_create(NULL, _mesa_hash_pointer,
            batch->num_other_batches = 0;
            iris_foreach_batch(ice, other_batch) {
      if (batch != other_batch)
               if (INTEL_DEBUG(DEBUG_ANY)) {
      const unsigned decode_flags = INTEL_BATCH_DECODE_DEFAULT_FLAGS |
            intel_batch_decode_ctx_init(&batch->decoder, &screen->compiler->isa,
                     batch->decoder.dynamic_base = IRIS_MEMZONE_DYNAMIC_START;
   batch->decoder.instruction_base = IRIS_MEMZONE_SHADER_START;
   batch->decoder.surface_base = IRIS_MEMZONE_BINDER_START;
   batch->decoder.max_vbo_decoded_lines = 32;
   if (batch->name == IRIS_BATCH_BLITTER)
                                    }
      void
   iris_init_batches(struct iris_context *ice)
   {
      struct iris_screen *screen = (struct iris_screen *)ice->ctx.screen;
   struct iris_bufmgr *bufmgr = screen->bufmgr;
            switch (devinfo->kmd_type) {
   case INTEL_KMD_TYPE_I915:
      iris_i915_init_batches(ice);
      case INTEL_KMD_TYPE_XE:
      iris_xe_init_batches(ice);
      default:
                  iris_foreach_batch(ice, batch)
      }
      static int
   find_exec_index(struct iris_batch *batch, struct iris_bo *bo)
   {
               if (index == -1)
            if (index < batch->exec_count && batch->exec_bos[index] == bo)
            /* May have been shared between multiple active batches */
   for (index = 0; index < batch->exec_count; index++) {
      if (batch->exec_bos[index] == bo)
                  }
      static void
   ensure_exec_obj_space(struct iris_batch *batch, uint32_t count)
   {
      while (batch->exec_count + count > batch->exec_array_size) {
               batch->exec_array_size *= 2;
   batch->exec_bos =
      realloc(batch->exec_bos,
      batch->bos_written =
      rerzalloc(NULL, batch->bos_written, BITSET_WORD,
            }
      static void
   add_bo_to_batch(struct iris_batch *batch, struct iris_bo *bo, bool writable)
   {
                                 if (writable)
            bo->index = batch->exec_count;
   batch->exec_count++;
            batch->max_gem_handle =
      }
      static void
   flush_for_cross_batch_dependencies(struct iris_batch *batch,
               {
      if (batch->measure && bo == batch->measure->bo)
            /* When a batch uses a buffer for the first time, or newly writes a buffer
   * it had already referenced, we may need to flush other batches in order
   * to correctly synchronize them.
   */
   for (int b = 0; b < batch->num_other_batches; b++) {
      struct iris_batch *other_batch = batch->other_batches[b];
            /* If the buffer is referenced by another batch, and either batch
   * intends to write it, then flush the other batch and synchronize.
   *
   * Consider these cases:
   *
   * 1. They read, we read   =>  No synchronization required.
   * 2. They read, we write  =>  Synchronize (they need the old value)
   * 3. They write, we read  =>  Synchronize (we need their new value)
   * 4. They write, we write =>  Synchronize (order writes)
   *
   * The read/read case is very common, as multiple batches usually
   * share a streaming state buffer or shader assembly buffer, and
   * we want to avoid synchronizing in this case.
   */
   if (other_index != -1 &&
      (writable || BITSET_TEST(other_batch->bos_written, other_index)))
      }
      /**
   * Add a buffer to the current batch's validation list.
   *
   * You must call this on any BO you wish to use in this batch, to ensure
   * that it's resident when the GPU commands execute.
   */
   void
   iris_use_pinned_bo(struct iris_batch *batch,
               {
      assert(iris_get_backing_bo(bo)->real.kflags & EXEC_OBJECT_PINNED);
            /* Never mark the workaround BO with EXEC_OBJECT_WRITE.  We don't care
   * about the order of any writes to that buffer, and marking it writable
   * would introduce data dependencies between multiple batches which share
   * the buffer. It is added directly to the batch using add_bo_to_batch()
   * during batch reset time.
   */
   if (bo == batch->screen->workaround_bo)
            if (access < NUM_IRIS_DOMAINS) {
      assert(batch->sync_region_depth);
                        if (existing_index == -1) {
               ensure_exec_obj_space(batch, 1);
      } else if (writable && !BITSET_TEST(batch->bos_written, existing_index)) {
               /* The BO is already in the list; mark it writable */
         }
      static void
   create_batch(struct iris_batch *batch)
   {
      struct iris_screen *screen = batch->screen;
            /* TODO: We probably could suballocate batches... */
   batch->bo = iris_bo_alloc(bufmgr, "command buffer",
               iris_get_backing_bo(batch->bo)->real.kflags |= EXEC_OBJECT_CAPTURE;
   batch->map = iris_bo_map(NULL, batch->bo, MAP_READ | MAP_WRITE);
            ensure_exec_obj_space(batch, 1);
      }
      static void
   iris_batch_maybe_noop(struct iris_batch *batch)
   {
      /* We only insert the NOOP at the beginning of the batch. */
            if (batch->noop_enabled) {
      /* Emit MI_BATCH_BUFFER_END to prevent any further command to be
   * executed.
   */
                           }
      static void
   iris_batch_reset(struct iris_batch *batch)
   {
      struct iris_screen *screen = batch->screen;
   struct iris_bufmgr *bufmgr = screen->bufmgr;
                     iris_bo_unreference(batch->bo);
   batch->primary_batch_size = 0;
   batch->total_chained_batch_size = 0;
   batch->contains_draw = false;
   batch->contains_fence_signal = false;
   if (devinfo->ver < 11)
         else
            create_batch(batch);
            memset(batch->bos_written, 0,
            struct iris_syncobj *syncobj = iris_create_syncobj(bufmgr);
   iris_batch_add_syncobj(batch, syncobj, IRIS_BATCH_FENCE_SIGNAL);
            assert(!batch->sync_region_depth);
   iris_batch_sync_boundary(batch);
            /* Always add the workaround BO, it contains a driver identifier at the
   * beginning quite helpful to debug error states.
   */
                     u_trace_init(&batch->trace, &batch->ice->ds.trace_context);
      }
      static void
   iris_batch_free(const struct iris_context *ice, struct iris_batch *batch)
   {
      struct iris_screen *screen = batch->screen;
   struct iris_bufmgr *bufmgr = screen->bufmgr;
            for (int i = 0; i < batch->exec_count; i++) {
         }
   free(batch->exec_bos);
                              util_dynarray_foreach(&batch->syncobjs, struct iris_syncobj *, s)
                  iris_fine_fence_reference(batch->screen, &batch->last_fence, NULL);
            iris_bo_unreference(batch->bo);
   batch->bo = NULL;
   batch->map = NULL;
            switch (devinfo->kmd_type) {
   case INTEL_KMD_TYPE_I915:
      iris_i915_destroy_batch(batch);
      case INTEL_KMD_TYPE_XE:
      iris_xe_destroy_batch(batch);
      default:
                  iris_destroy_batch_measure(batch->measure);
                              if (INTEL_DEBUG(DEBUG_ANY))
      }
      void
   iris_destroy_batches(struct iris_context *ice)
   {
      iris_foreach_batch(ice, batch)
      }
      void iris_batch_maybe_begin_frame(struct iris_batch *batch)
   {
               if (ice->utrace.begin_frame != ice->frame) {
      trace_intel_begin_frame(&batch->trace, batch);
         }
      /**
   * If we've chained to a secondary batch, or are getting near to the end,
   * then flush.  This should only be called between draws.
   */
   void
   iris_batch_maybe_flush(struct iris_batch *batch, unsigned estimate)
   {
      if (batch->bo != batch->exec_bos[0] ||
      iris_batch_bytes_used(batch) + estimate >= BATCH_SZ) {
         }
      static void
   record_batch_sizes(struct iris_batch *batch)
   {
                        if (batch->bo == batch->exec_bos[0])
               }
      void
   iris_chain_to_new_batch(struct iris_batch *batch)
   {
      uint32_t *cmd = batch->map_next;
   uint64_t *addr = batch->map_next + 4;
                     /* No longer held by batch->bo, still held by validation list */
   iris_bo_unreference(batch->bo);
            /* Emit MI_BATCH_BUFFER_START to chain to another batch. */
   *cmd = (0x31 << 23) | (1 << 8) | (3 - 2);
      }
      static void
   add_aux_map_bos_to_batch(struct iris_batch *batch)
   {
      void *aux_map_ctx = iris_bufmgr_get_aux_map_context(batch->screen->bufmgr);
   if (!aux_map_ctx)
            uint32_t count = intel_aux_map_get_num_buffers(aux_map_ctx);
   ensure_exec_obj_space(batch, count);
   intel_aux_map_fill_bos(aux_map_ctx,
         for (uint32_t i = 0; i < count; i++) {
      struct iris_bo *bo = batch->exec_bos[batch->exec_count];
         }
      static void
   finish_seqno(struct iris_batch *batch)
   {
      struct iris_fine_fence *sq = iris_fine_fence_new(batch);
   if (!sq)
            iris_fine_fence_reference(batch->screen, &batch->last_fence, sq);
      }
      /**
   * Terminate a batch with MI_BATCH_BUFFER_END.
   */
   static void
   iris_finish_batch(struct iris_batch *batch)
   {
               if (devinfo->ver == 12 && batch->name == IRIS_BATCH_RENDER) {
      /* We re-emit constants at the beginning of every batch as a hardware
   * bug workaround, so invalidate indirect state pointers in order to
   * save ourselves the overhead of restoring constants redundantly when
   * the next render batch is executed.
   */
   iris_emit_pipe_control_flush(batch, "ISP invalidate at batch end",
                                                      struct iris_context *ice = batch->ice;
   if (ice->utrace.end_frame != ice->frame) {
      trace_intel_end_frame(&batch->trace, batch, ice->utrace.end_frame);
               /* Emit MI_BATCH_BUFFER_END to finish our batch. */
                                 }
      /**
   * Replace our current GEM context with a new one (in case it got banned).
   */
   static bool
   replace_kernel_ctx(struct iris_batch *batch)
   {
      struct iris_screen *screen = batch->screen;
   struct iris_bufmgr *bufmgr = screen->bufmgr;
                     switch (devinfo->kmd_type) {
   case INTEL_KMD_TYPE_I915:
         case INTEL_KMD_TYPE_XE:
         default:
      unreachable("missing");
         }
      enum pipe_reset_status
   iris_batch_check_for_reset(struct iris_batch *batch)
   {
      struct iris_screen *screen = batch->screen;
   struct iris_bufmgr *bufmgr = screen->bufmgr;
   struct iris_context *ice = batch->ice;
   const struct iris_kmd_backend *backend;
            /* Banned context was already signalled to application */
   if (ice->context_reset_signaled)
            backend = iris_bufmgr_get_kernel_driver_backend(bufmgr);
            if (status != PIPE_NO_RESET)
               }
      static void
   move_syncobj_to_batch(struct iris_batch *batch,
               {
               if (!*p_syncobj)
            bool found = false;
   util_dynarray_foreach(&batch->syncobjs, struct iris_syncobj *, s) {
      if (*p_syncobj == *s) {
      found = true;
                  if (!found)
               }
      static void
   update_bo_syncobjs(struct iris_batch *batch, struct iris_bo *bo, bool write)
   {
      struct iris_screen *screen = batch->screen;
   struct iris_bufmgr *bufmgr = screen->bufmgr;
                     /* Make sure bo->deps is big enough */
   if (screen->id >= bo->deps_size) {
      int new_size = screen->id + 1;
   bo->deps = realloc(bo->deps, new_size * sizeof(bo->deps[0]));
   assert(bo->deps);
   memset(&bo->deps[bo->deps_size], 0,
                        /* When it comes to execbuf submission of non-shared buffers, we only need
   * to care about the reads and writes done by the other batches of our own
   * screen, and we also don't care about the reads and writes done by our
   * own batch, although we need to track them. Just note that other places of
   * our code may need to care about all the operations done by every batch
   * on every screen.
   */
   struct iris_bo_screen_deps *bo_deps = &bo->deps[screen->id];
            /* Make our batch depend on additional syncobjs depending on what other
   * batches have been doing to this bo.
   *
   * We also look at the dependencies set by our own batch since those could
   * have come from a different context, and apps don't like it when we don't
   * do inter-context tracking.
   */
   iris_foreach_batch(ice, batch_i) {
               /* If the bo is being written to by others, wait for them. */
   if (bo_deps->write_syncobjs[i])
                  /* If we're writing to the bo, wait on the reads from other batches. */
   if (write)
      move_syncobj_to_batch(batch, &bo_deps->read_syncobjs[i],
            struct iris_syncobj *batch_syncobj =
            /* Update bo_deps depending on what we're doing with the bo in this batch
   * by putting the batch's syncobj in the bo_deps lists accordingly. Only
   * keep track of the last time we wrote to or read the BO.
   */
   if (write) {
      iris_syncobj_reference(bufmgr, &bo_deps->write_syncobjs[batch_idx],
      } else {
      iris_syncobj_reference(bufmgr, &bo_deps->read_syncobjs[batch_idx],
         }
      void
   iris_batch_update_syncobjs(struct iris_batch *batch)
   {
      for (int i = 0; i < batch->exec_count; i++) {
      struct iris_bo *bo = batch->exec_bos[i];
            if (bo == batch->screen->workaround_bo)
                  }
      /**
   * Convert the syncobj which will be signaled when this batch completes
   * to a SYNC_FILE object, for use with import/export sync ioctls.
   */
   bool
   iris_batch_syncobj_to_sync_file_fd(struct iris_batch *batch, int *out_fd)
   {
               struct iris_syncobj *batch_syncobj =
            struct drm_syncobj_handle syncobj_to_fd_ioctl = {
      .handle = batch_syncobj->handle,
   .flags = DRM_SYNCOBJ_HANDLE_TO_FD_FLAGS_EXPORT_SYNC_FILE,
      };
   if (intel_ioctl(drm_fd, DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD,
            fprintf(stderr, "DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD ioctl failed (%d)\n",
                     assert(syncobj_to_fd_ioctl.fd >= 0);
               }
      const char *
   iris_batch_name_to_string(enum iris_batch_name name)
   {
      const char *names[IRIS_BATCH_COUNT] = {
      [IRIS_BATCH_RENDER]  = "render",
   [IRIS_BATCH_COMPUTE] = "compute",
      };
      }
      static inline bool
   context_or_exec_queue_was_banned(struct iris_bufmgr *bufmgr, int ret)
   {
               /* In i915 EIO means our context is banned, while on Xe ECANCELED means
   * our exec queue was banned
   */
   if ((kmd_type == INTEL_KMD_TYPE_I915 && ret == -EIO) ||
      (kmd_type == INTEL_KMD_TYPE_XE && ret == -ECANCELED))
            }
      /**
   * Flush the batch buffer, submitting it to the GPU and resetting it so
   * we're ready to emit the next batch.
   */
   void
   _iris_batch_flush(struct iris_batch *batch, const char *file, int line)
   {
      struct iris_screen *screen = batch->screen;
   struct iris_context *ice = batch->ice;
            /* If a fence signals we need to flush it. */
   if (iris_batch_bytes_used(batch) == 0 && !batch->contains_fence_signal)
                              if (INTEL_DEBUG(DEBUG_BATCH | DEBUG_SUBMIT | DEBUG_PIPE_CONTROL)) {
      const char *basefile = strstr(file, "iris/");
   if (basefile)
            enum intel_kmd_type kmd_type = iris_bufmgr_get_device_info(bufmgr)->kmd_type;
   uint32_t batch_ctx_id = kmd_type == INTEL_KMD_TYPE_I915 ?
         fprintf(stderr, "%19s:%-3d: %s batch [%u] flush with %5db (%0.1f%%) "
         "(cmds), %4d BOs (%0.1fMb aperture)\n",
   file, line, iris_batch_name_to_string(batch->name),
   batch_ctx_id, batch->total_chained_batch_size,
   100.0f * batch->total_chained_batch_size / BATCH_SZ,
                  uint64_t start_ts = intel_ds_begin_submit(&batch->ds);
   uint64_t submission_id = batch->ds.submission_id;
   int ret = iris_bufmgr_get_kernel_driver_backend(bufmgr)->batch_submit(batch);
            /* When batch submission fails, our end-of-batch syncobj remains
   * unsignalled, and in fact is not even considered submitted.
   *
   * In the hang recovery case (-EIO) or -ENOMEM, we recreate our context and
   * attempt to carry on.  In that case, we need to signal our syncobj,
   * dubiously claiming that this batch completed, because future batches may
   * depend on it.  If we don't, then execbuf would fail with -EINVAL for
   * those batches, because they depend on a syncobj that's considered to be
   * "never submitted".  This would lead to an abort().  So here, we signal
   * the failing batch's syncobj to try and allow further progress to be
   * made, knowing we may have broken our dependency tracking.
   */
   if (ret < 0)
            batch->exec_count = 0;
   batch->max_gem_handle = 0;
            util_dynarray_foreach(&batch->syncobjs, struct iris_syncobj *, s)
                           if (INTEL_DEBUG(DEBUG_SYNC)) {
      dbg_printf("waiting for idle\n");
               if (u_trace_should_process(&ice->ds.trace_context))
            /* Start a new batch buffer. */
            /* Check if context or engine was banned, if yes try to replace it
   * with a new logical context, and inform iris_context that all state
   * has been lost and needs to be re-initialized.  If this succeeds,
   * dubiously claim success...
   */
   if (ret && context_or_exec_queue_was_banned(bufmgr, ret)) {
               if (status != PIPE_NO_RESET || ice->context_reset_signaled)
            if (batch->reset->reset) {
      /* Tell gallium frontends the device is lost and it was our fault. */
                              #ifdef DEBUG
         const bool color = INTEL_DEBUG(DEBUG_COLOR);
   fprintf(stderr, "%siris: Failed to submit batchbuffer: %-80s%s\n",
   #endif
               }
      /**
   * Does the current batch refer to the given BO?
   *
   * (In other words, is the BO in the current batch's validation list?)
   */
   bool
   iris_batch_references(struct iris_batch *batch, struct iris_bo *bo)
   {
         }
      /**
   * Updates the state of the noop feature.  Returns true if there was a noop
   * transition that led to state invalidation.
   */
   bool
   iris_batch_prepare_noop(struct iris_batch *batch, bool noop_enable)
   {
      if (batch->noop_enabled == noop_enable)
                              /* If the batch was empty, flush had no effect, so insert our noop. */
   if (iris_batch_bytes_used(batch) == 0)
            /* We only need to update the entire state if we transition from noop ->
   * not-noop.
   */
      }
