   /*
   * Copyright © 2012 Intel Corporation
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
      /** @file glthread.c
   *
   * Support functions for the glthread feature of Mesa.
   *
   * In multicore systems, many applications end up CPU-bound with about half
   * their time spent inside their rendering thread and half inside Mesa.  To
   * alleviate this, we put a shim layer in Mesa at the GL dispatch level that
   * quickly logs the GL commands to a buffer to be processed by a worker
   * thread.
   */
      #include "main/mtypes.h"
   #include "main/glthread.h"
   #include "main/glthread_marshal.h"
   #include "main/hash.h"
   #include "util/u_atomic.h"
   #include "util/u_thread.h"
   #include "util/u_cpu_detect.h"
      #include "state_tracker/st_context.h"
      static void
   glthread_update_global_locking(struct gl_context *ctx)
   {
               /* Determine if we should lock the global mutexes. */
   simple_mtx_lock(&shared->Mutex);
            /* We can only lock the mutexes after NoLockDuration nanoseconds have
   * passed since multiple contexts were active.
   */
   bool lock_mutexes = shared->GLThread.LastContextSwitchTime +
            /* Check if multiple contexts are active (the last executing context is
   * different).
   */
   if (ctx != shared->GLThread.LastExecutingCtx) {
      if (lock_mutexes) {
      /* If we get here, we've been locking the global mutexes for a while
   * and now we are switching contexts. */
   if (shared->GLThread.LastContextSwitchTime +
      120 * ONE_SECOND_IN_NS < current_time) {
   /* If it's been more than 2 minutes of only one active context,
   * indicating that there was no other active context for a long
   * time, reset the no-lock time to its initial state of only 1
   * second. This is most likely an infrequent situation of
   * multi-context loading of game content and shaders.
   * (this is a heuristic)
   */
      } else if (shared->GLThread.NoLockDuration < 32 * ONE_SECOND_IN_NS) {
      /* Double the no-lock duration if we are transitioning from only
   * one active context to multiple active contexts after a short
   * time, up to a maximum of 32 seconds, indicating that multiple
   * contexts are frequently executing. (this is a heuristic)
   */
                           /* There are multiple active contexts. Update the last executing context
   * and the last context switch time. We only start locking global mutexes
   * after LastContextSwitchTime + NoLockDuration passes, so this
   * effectively resets the non-locking stopwatch to 0, so that multiple
   * contexts can execute simultaneously as long as they are not idle.
   */
   shared->GLThread.LastExecutingCtx = ctx;
      }
               }
      static void
   glthread_unmarshal_batch(void *job, void *gdata, int thread_index)
   {
      struct glthread_batch *batch = (struct glthread_batch*)job;
   struct gl_context *ctx = batch->ctx;
   unsigned pos = 0;
   unsigned used = batch->used;
   uint64_t *buffer = batch->buffer;
            /* Determine once every 64 batches whether shared mutexes should be locked.
   * We have to do this less frequently because os_time_get_nano() is very
   * expensive if the clock source is not TSC. See:
   *    https://gitlab.freedesktop.org/mesa/mesa/-/issues/8910
   */
   if (ctx->GLThread.GlobalLockUpdateBatchCounter++ % 64 == 0)
            /* Execute the GL calls. */
            /* Here we lock the mutexes once globally if possible. If not, we just
   * fallback to the individual API calls doing it.
   */
   bool lock_mutexes = ctx->GLThread.LockGlobalMutexes;
   if (lock_mutexes) {
      _mesa_HashLockMutex(shared->BufferObjects);
   ctx->BufferObjectsLocked = true;
   simple_mtx_lock(&shared->TexMutex);
               while (pos < used) {
      const struct marshal_cmd_base *cmd =
                        if (lock_mutexes) {
      ctx->TexturesLocked = false;
   simple_mtx_unlock(&shared->TexMutex);
   ctx->BufferObjectsLocked = false;
               assert(pos == used);
            unsigned batch_index = batch - ctx->GLThread.batches;
   /* Atomically set this to -1 if it's equal to batch_index. */
   p_atomic_cmpxchg(&ctx->GLThread.LastProgramChangeBatch, batch_index, -1);
               }
      static void
   glthread_thread_initialization(void *job, void *gdata, int thread_index)
   {
               st_set_background_context(ctx, &ctx->GLThread.stats);
      }
      static void
   _mesa_glthread_init_dispatch(struct gl_context *ctx,
         {
      _mesa_glthread_init_dispatch0(ctx, table);
   _mesa_glthread_init_dispatch1(ctx, table);
   _mesa_glthread_init_dispatch2(ctx, table);
   _mesa_glthread_init_dispatch3(ctx, table);
   _mesa_glthread_init_dispatch4(ctx, table);
   _mesa_glthread_init_dispatch5(ctx, table);
   _mesa_glthread_init_dispatch6(ctx, table);
      }
      void
   _mesa_glthread_init(struct gl_context *ctx)
   {
      struct pipe_screen *screen = ctx->screen;
   struct glthread_state *glthread = &ctx->GLThread;
            if (!screen->get_param(screen, PIPE_CAP_MAP_UNSYNCHRONIZED_THREAD_SAFE) ||
      !screen->get_param(screen, PIPE_CAP_ALLOW_MAPPED_BUFFERS_DURING_EXECUTION))
         if (!util_queue_init(&glthread->queue, "gl", MARSHAL_MAX_BATCHES - 2,
                        glthread->VAOs = _mesa_NewHashTable();
   if (!glthread->VAOs) {
      util_queue_destroy(&glthread->queue);
               _mesa_glthread_reset_vao(&glthread->DefaultVAO);
            ctx->MarshalExec = _mesa_alloc_dispatch_table(true);
   if (!ctx->MarshalExec) {
      _mesa_DeleteHashTable(glthread->VAOs);
   util_queue_destroy(&glthread->queue);
                        for (unsigned i = 0; i < MARSHAL_MAX_BATCHES; i++) {
      glthread->batches[i].ctx = ctx;
      }
   glthread->next_batch = &glthread->batches[glthread->next];
   glthread->used = 0;
                     /* glthread takes over all L3 pinning */
                     /* Execute the thread initialization function in the thread. */
   struct util_queue_fence fence;
   util_queue_fence_init(&fence);
   util_queue_add_job(&glthread->queue, ctx, &fence,
         util_queue_fence_wait(&fence);
      }
      static void
   free_vao(void *data, UNUSED void *userData)
   {
         }
      void
   _mesa_glthread_destroy(struct gl_context *ctx)
   {
                        if (util_queue_is_initialized(&glthread->queue)) {
               for (unsigned i = 0; i < MARSHAL_MAX_BATCHES; i++)
            _mesa_HashDeleteAll(glthread->VAOs, free_vao, NULL);
   _mesa_DeleteHashTable(glthread->VAOs);
         }
      void _mesa_glthread_enable(struct gl_context *ctx)
   {
      if (ctx->GLThread.enabled ||
      ctx->Dispatch.Current == ctx->Dispatch.ContextLost ||
   ctx->GLThread.DebugOutputSynchronous)
         ctx->GLThread.enabled = true;
            /* Update the dispatch only if the dispatch is current. */
   if (_glapi_get_dispatch() == ctx->Dispatch.Current) {
            }
      void _mesa_glthread_disable(struct gl_context *ctx)
   {
      if (!ctx->GLThread.enabled)
                     ctx->GLThread.enabled = false;
            /* Update the dispatch only if the dispatch is current. */
   if (_glapi_get_dispatch() == ctx->MarshalExec) {
                  /* Unbind VBOs in all VAOs that glthread bound for non-VBO vertex uploads
   * to restore original states.
   */
   if (ctx->API != API_OPENGL_CORE)
      }
      void
   _mesa_glthread_flush_batch(struct gl_context *ctx)
   {
      struct glthread_state *glthread = &ctx->GLThread;
   if (!glthread->enabled)
            if (ctx->Dispatch.Current == ctx->Dispatch.ContextLost) {
      _mesa_glthread_disable(ctx);
               if (!glthread->used)
            /* Pin threads regularly to the same Zen CCX that the main thread is
   * running on. The main thread can move between CCXs.
   */
   if (util_get_cpu_caps()->num_L3_caches > 1 &&
      /* driver support */
   ctx->pipe->set_context_param &&
   ++glthread->pin_thread_counter % 128 == 0) {
            if (cpu >= 0) {
      uint16_t L3_cache = util_get_cpu_caps()->cpu_to_L3[cpu];
   if (L3_cache != U_CPU_INVALID_L3) {
      util_set_thread_affinity(glthread->queue.threads[0],
               ctx->pipe->set_context_param(ctx->pipe,
                                    p_atomic_add(&glthread->stats.num_offloaded_items, glthread->used);
            util_queue_add_job(&glthread->queue, next, &next->fence,
         glthread->last = glthread->next;
   glthread->next = (glthread->next + 1) % MARSHAL_MAX_BATCHES;
   glthread->next_batch = &glthread->batches[glthread->next];
            glthread->LastCallList = NULL;
      }
      /**
   * Waits for all pending batches have been unmarshaled.
   *
   * This can be used by the main thread to synchronize access to the context,
   * since the worker thread will be idle after this.
   */
   void
   _mesa_glthread_finish(struct gl_context *ctx)
   {
      struct glthread_state *glthread = &ctx->GLThread;
   if (!glthread->enabled)
            /* If this is called from the worker thread, then we've hit a path that
   * might be called from either the main thread or the worker (such as some
   * dri interface entrypoints), in which case we don't need to actually
   * synchronize against ourself.
   */
   if (u_thread_is_self(glthread->queue.threads[0]))
            struct glthread_batch *last = &glthread->batches[glthread->last];
   struct glthread_batch *next = glthread->next_batch;
            if (!util_queue_fence_is_signalled(&last->fence)) {
      util_queue_fence_wait(&last->fence);
               if (glthread->used) {
      p_atomic_add(&glthread->stats.num_direct_items, glthread->used);
   next->used = glthread->used;
            glthread->LastCallList = NULL;
            /* Since glthread_unmarshal_batch changes the dispatch to direct,
   * restore it after it's done.
   */
   struct _glapi_table *dispatch = _glapi_get_dispatch();
   glthread_unmarshal_batch(next, NULL, 0);
            /* It's not a sync because we don't enqueue partial batches, but
   * it would be a sync if we did. So count it anyway.
   */
               if (synced)
      }
      void
   _mesa_glthread_finish_before(struct gl_context *ctx, const char *func)
   {
               /* Uncomment this if you want to know where glthread syncs. */
      }
      void
   _mesa_error_glthread_safe(struct gl_context *ctx, GLenum error, bool glthread,
         {
      if (glthread) {
         } else {
      char s[MAX_DEBUG_MESSAGE_LENGTH];
            va_start(args, format);
   ASSERTED size_t len = vsnprintf(s, MAX_DEBUG_MESSAGE_LENGTH, format, args);
            /* Whoever calls _mesa_error should use shorter strings. */
                  }
      bool
   _mesa_glthread_invalidate_zsbuf(struct gl_context *ctx)
   {
      struct glthread_state *glthread = &ctx->GLThread;
   if (!glthread->enabled)
         _mesa_marshal_InternalInvalidateFramebufferAncillaryMESA();
      }
