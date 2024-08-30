   /*
   * Copyright © 2018 Intel Corporation
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
   * @file iris_fence.c
   *
   * Fences for driver and IPC serialisation, scheduling and synchronisation.
   */
      #include "drm-uapi/sync_file.h"
   #include "util/u_debug.h"
   #include "util/u_inlines.h"
   #include "intel/common/intel_gem.h"
      #include "iris_batch.h"
   #include "iris_bufmgr.h"
   #include "iris_context.h"
   #include "iris_fence.h"
   #include "iris_screen.h"
      static uint32_t
   gem_syncobj_create(int fd, uint32_t flags)
   {
      struct drm_syncobj_create args = {
                              }
      static void
   gem_syncobj_destroy(int fd, uint32_t handle)
   {
      struct drm_syncobj_destroy args = {
                     }
      /**
   * Make a new sync-point.
   */
   struct iris_syncobj *
   iris_create_syncobj(struct iris_bufmgr *bufmgr)
   {
      int fd = iris_bufmgr_get_fd(bufmgr);
            if (!syncobj)
            syncobj->handle = gem_syncobj_create(fd, 0);
                        }
      void
   iris_syncobj_destroy(struct iris_bufmgr *bufmgr, struct iris_syncobj *syncobj)
   {
      int fd = iris_bufmgr_get_fd(bufmgr);
   gem_syncobj_destroy(fd, syncobj->handle);
      }
      void
   iris_syncobj_signal(struct iris_bufmgr *bufmgr, struct iris_syncobj *syncobj)
   {
      int fd = iris_bufmgr_get_fd(bufmgr);
   struct drm_syncobj_array args = {
      .handles = (uintptr_t)&syncobj->handle,
               if (intel_ioctl(fd, DRM_IOCTL_SYNCOBJ_SIGNAL, &args)) {
      fprintf(stderr, "failed to signal syncobj %"PRIu32"\n",
         }
      /**
   * Add a sync-point to the batch, with the given flags.
   *
   * \p flags   One of IRIS_BATCH_FENCE_WAIT or IRIS_BATCH_FENCE_SIGNAL.
   */
   void
   iris_batch_add_syncobj(struct iris_batch *batch,
               {
      struct iris_batch_fence *fence =
            *fence = (struct iris_batch_fence) {
      .handle = syncobj->handle,
               struct iris_syncobj **store =
            *store = NULL;
      }
      /**
   * Walk through a batch's dependencies (any IRIS_BATCH_FENCE_WAIT syncobjs)
   * and unreference any which have already passed.
   *
   * Sometimes the compute batch is seldom used, and accumulates references
   * to stale render batches that are no longer of interest, so we can free
   * those up.
   */
   static void
   clear_stale_syncobjs(struct iris_batch *batch)
   {
      struct iris_screen *screen = batch->screen;
                     assert(n == util_dynarray_num_elements(&batch->exec_fences,
            /* Skip the first syncobj, as it's the signalling one. */
   for (int i = n - 1; i > 0; i--) {
      struct iris_syncobj **syncobj =
         struct iris_batch_fence *fence =
      util_dynarray_element(&batch->exec_fences,
               if (iris_wait_syncobj(bufmgr, *syncobj, 0) == false)
            /* This sync object has already passed, there's no need to continue
   * marking it as a dependency; we can stop holding on to the reference.
   */
            /* Remove it from the lists; move the last element here. */
   struct iris_syncobj **nth_syncobj =
         struct iris_batch_fence *nth_fence =
            if (syncobj != nth_syncobj) {
      *syncobj = *nth_syncobj;
            }
      /* ------------------------------------------------------------------- */
      struct pipe_fence_handle {
                           };
      static void
   iris_fence_destroy(struct pipe_screen *p_screen,
         {
               for (unsigned i = 0; i < ARRAY_SIZE(fence->fine); i++)
               }
      static void
   iris_fence_reference(struct pipe_screen *p_screen,
               {
      if (pipe_reference(*dst ? &(*dst)->ref : NULL,
                     }
      bool
   iris_wait_syncobj(struct iris_bufmgr *bufmgr,
               {
      if (!syncobj)
                     struct drm_syncobj_wait args = {
      .handles = (uintptr_t)&syncobj->handle,
   .count_handles = 1,
      };
      }
      #define CSI "\e["
   #define BLUE_HEADER  CSI "0;97;44m"
   #define NORMAL       CSI "0m"
      static void
   iris_fence_flush(struct pipe_context *ctx,
               {
      struct iris_screen *screen = (void *) ctx->screen;
            /* We require DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT (kernel 5.2+) for
   * deferred flushes.  Just ignore the request to defer on older kernels.
   */
   if (!(screen->kernel_features & KERNEL_HAS_WAIT_FOR_SUBMIT))
                     if (flags & PIPE_FLUSH_END_OF_FRAME) {
               if (INTEL_DEBUG(DEBUG_SUBMIT)) {
      fprintf(stderr, "%s ::: FRAME %-10u (ctx %p)%-35c%s\n",
         INTEL_DEBUG(DEBUG_COLOR) ? BLUE_HEADER : "",
                           if (!deferred) {
      iris_foreach_batch(ice, batch)
               if (flags & PIPE_FLUSH_END_OF_FRAME) {
                           if (!out_fence)
            struct pipe_fence_handle *fence = calloc(1, sizeof(*fence));
   if (!fence)
                     if (deferred)
            iris_foreach_batch(ice, batch) {
               if (deferred && iris_batch_bytes_used(batch) > 0) {
      struct iris_fine_fence *fine = iris_fine_fence_new(batch);
   iris_fine_fence_reference(screen, &fence->fine[b], fine);
      } else {
      /* This batch has no commands queued up (perhaps we just flushed,
   * or all the commands are on the other batch).  Wait for the last
   * syncobj on this engine - unless it's already finished by now.
   */
                                 iris_fence_reference(ctx->screen, out_fence, NULL);
      }
      static void
   iris_fence_await(struct pipe_context *ctx,
         {
               /* Unflushed fences from the same context are no-ops. */
   if (ctx && ctx == fence->unflushed_ctx)
            /* XXX: We can't safely flush the other context, because it might be
   *      bound to another thread, and poking at its internals wouldn't
   *      be safe.  In the future we should use MI_SEMAPHORE_WAIT and
   *      block until the other job has been submitted, relying on
   *      kernel timeslicing to preempt us until the other job is
   *      actually flushed and the seqno finally passes.
   */
   if (fence->unflushed_ctx) {
      util_debug_message(&ice->dbg, CONFORMANCE, "%s",
                     for (unsigned i = 0; i < ARRAY_SIZE(fence->fine); i++) {
               if (iris_fine_fence_signaled(fine))
            iris_foreach_batch(ice, batch) {
      /* We're going to make any future work in this batch wait for our
   * fence to have gone by.  But any currently queued work doesn't
   * need to wait.  Flush the batch now, so it can happen sooner.
                                          }
      #define NSEC_PER_SEC (1000 * USEC_PER_SEC)
   #define USEC_PER_SEC (1000 * MSEC_PER_SEC)
   #define MSEC_PER_SEC (1000)
      static uint64_t
   gettime_ns(void)
   {
      struct timespec current;
   clock_gettime(CLOCK_MONOTONIC, &current);
      }
      static uint64_t
   rel2abs(uint64_t timeout)
   {
      if (timeout == 0)
            uint64_t current_time = gettime_ns();
                        }
      static bool
   iris_fence_finish(struct pipe_screen *p_screen,
                     {
               struct iris_context *ice = (struct iris_context *)ctx;
            /* If we created the fence with PIPE_FLUSH_DEFERRED, we may not have
   * flushed yet.  Check if our syncobj is the current batch's signalling
   * syncobj - if so, we haven't flushed and need to now.
   *
   * The Gallium docs mention that a flush will occur if \p ctx matches
   * the context the fence was created with.  It may be NULL, so we check
   * that it matches first.
   */
   if (ctx && ctx == fence->unflushed_ctx) {
      iris_foreach_batch(ice, batch) {
                              if (fine->syncobj == iris_batch_get_signal_syncobj(batch))
               /* The fence is no longer deferred. */
               unsigned int handle_count = 0;
   uint32_t handles[ARRAY_SIZE(fence->fine)];
   for (unsigned i = 0; i < ARRAY_SIZE(fence->fine); i++) {
               if (iris_fine_fence_signaled(fine))
                        if (handle_count == 0)
            struct drm_syncobj_wait args = {
      .handles = (uintptr_t)handles,
   .count_handles = handle_count,
   .timeout_nsec = rel2abs(timeout),
               if (fence->unflushed_ctx) {
      /* This fence had a deferred flush from another context.  We can't
   * safely flush it here, because the context might be bound to a
   * different thread, and poking at its internals wouldn't be safe.
   *
   * Instead, use the WAIT_FOR_SUBMIT flag to block and hope that
   * another thread submits the work.
   */
                  }
      static int
   sync_merge_fd(int sync_fd, int new_fd)
   {
      if (sync_fd == -1)
            if (new_fd == -1)
            struct sync_merge_data args = {
      .name = "iris fence",
   .fd2 = new_fd,
               intel_ioctl(sync_fd, SYNC_IOC_MERGE, &args);
   close(new_fd);
               }
      static int
   iris_fence_get_fd(struct pipe_screen *p_screen,
         {
      struct iris_screen *screen = (struct iris_screen *)p_screen;
            /* Deferred fences aren't supported. */
   if (fence->unflushed_ctx)
            for (unsigned i = 0; i < ARRAY_SIZE(fence->fine); i++) {
               if (iris_fine_fence_signaled(fine))
            struct drm_syncobj_handle args = {
      .handle = fine->syncobj->handle,
   .flags = DRM_SYNCOBJ_HANDLE_TO_FD_FLAGS_EXPORT_SYNC_FILE,
               intel_ioctl(screen->fd, DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD, &args);
               if (fd == -1) {
      /* Our fence has no syncobj's recorded.  This means that all of the
   * batches had already completed, their syncobj's had been signalled,
   * and so we didn't bother to record them.  But we're being asked to
   * export such a fence.  So export a dummy already-signalled syncobj.
   */
   struct drm_syncobj_handle args = {
                  args.handle = gem_syncobj_create(screen->fd, DRM_SYNCOBJ_CREATE_SIGNALED);
   intel_ioctl(screen->fd, DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD, &args);
   gem_syncobj_destroy(screen->fd, args.handle);
                  }
      static void
   iris_fence_create_fd(struct pipe_context *ctx,
                     {
               struct iris_screen *screen = (struct iris_screen *)ctx->screen;
   struct drm_syncobj_handle args = {
                  if (type == PIPE_FD_TYPE_NATIVE_SYNC) {
      args.flags = DRM_SYNCOBJ_FD_TO_HANDLE_FLAGS_IMPORT_SYNC_FILE;
               if (intel_ioctl(screen->fd, DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE, &args) == -1) {
      fprintf(stderr, "DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE failed: %s\n",
         if (type == PIPE_FD_TYPE_NATIVE_SYNC)
         *out = NULL;
               struct iris_syncobj *syncobj = malloc(sizeof(*syncobj));
   if (!syncobj) {
      *out = NULL;
      }
   syncobj->handle = args.handle;
            struct iris_fine_fence *fine = calloc(1, sizeof(*fine));
   if (!fine) {
      free(syncobj);
   *out = NULL;
                        /* Fences work in terms of iris_fine_fence, but we don't actually have a
   * seqno for an imported fence.  So, create a fake one which always
   * returns as 'not signaled' so we fall back to using the sync object.
   */
   fine->seqno = UINT32_MAX;
   fine->map = &zero;
   fine->syncobj = syncobj;
            struct pipe_fence_handle *fence = calloc(1, sizeof(*fence));
   if (!fence) {
      free(fine);
   free(syncobj);
   *out = NULL;
      }
   pipe_reference_init(&fence->ref, 1);
               }
      static void
   iris_fence_signal(struct pipe_context *ctx,
         {
               if (ctx == fence->unflushed_ctx)
            iris_foreach_batch(ice, batch) {
      for (unsigned i = 0; i < ARRAY_SIZE(fence->fine); i++) {
               /* already signaled fence skipped */
                  batch->contains_fence_signal = true;
      }
   if (batch->contains_fence_signal)
         }
      void
   iris_init_screen_fence_functions(struct pipe_screen *screen)
   {
      screen->fence_reference = iris_fence_reference;
   screen->fence_finish = iris_fence_finish;
      }
      void
   iris_init_context_fence_functions(struct pipe_context *ctx)
   {
      ctx->flush = iris_fence_flush;
   ctx->create_fence_fd = iris_fence_create_fd;
   ctx->fence_server_sync = iris_fence_await;
      }
