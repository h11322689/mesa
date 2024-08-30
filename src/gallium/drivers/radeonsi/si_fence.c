   /*
   * Copyright 2013-2017 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_build_pm4.h"
   #include "util/os_time.h"
   #include "util/u_memory.h"
   #include "util/u_queue.h"
   #include "util/u_upload_mgr.h"
      #include <libsync.h>
      struct si_fine_fence {
      struct si_resource *buf;
      };
      struct si_fence {
      struct pipe_reference reference;
   struct pipe_fence_handle *gfx;
   struct tc_unflushed_batch_token *tc_token;
            /* If the context wasn't flushed at fence creation, this is non-NULL. */
   struct {
      struct si_context *ctx;
                  };
      /**
   * Write an EOP event.
   *
   * \param event        EVENT_TYPE_*
   * \param event_flags  Optional cache flush flags (TC)
   * \param dst_sel      MEM or TC_L2
   * \param int_sel      NONE or SEND_DATA_AFTER_WR_CONFIRM
   * \param data_sel     DISCARD, VALUE_32BIT, TIMESTAMP, or GDS
   * \param buf          Buffer
   * \param va           GPU address
   * \param old_value    Previous fence value (for a bug workaround)
   * \param new_value    Fence value to write for this event.
   */
   void si_cp_release_mem(struct si_context *ctx, struct radeon_cmdbuf *cs, unsigned event,
                     {
      unsigned op = EVENT_TYPE(event) |
               unsigned sel = EOP_DST_SEL(dst_sel) | EOP_INT_SEL(int_sel) | EOP_DATA_SEL(data_sel);
                     if (ctx->gfx_level >= GFX9 || (compute_ib && ctx->gfx_level >= GFX7)) {
      /* A ZPASS_DONE or PIXEL_STAT_DUMP_EVENT (of the DB occlusion
   * counters) must immediately precede every timestamp event to
   * prevent a GPU hang on GFX9.
   *
   * Occlusion queries don't need to do it here, because they
   * always do ZPASS_DONE before the timestamp.
   */
   if (ctx->gfx_level == GFX9 && !compute_ib && query_type != PIPE_QUERY_OCCLUSION_COUNTER &&
      query_type != PIPE_QUERY_OCCLUSION_PREDICATE &&
   query_type != PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE) {
                  if (!ctx->ws->cs_is_secure(&ctx->gfx_cs)) {
         } else {
      assert(ctx->screen->info.has_tmz_support);
   if (!ctx->eop_bug_scratch_tmz)
      ctx->eop_bug_scratch_tmz =
      si_aligned_buffer_create(&sscreen->b,
                                          assert(16 * ctx->screen->info.max_render_backends <= scratch->b.b.width0);
   radeon_emit(PKT3(PKT3_EVENT_WRITE, 2, 0));
   radeon_emit(EVENT_TYPE(V_028A90_ZPASS_DONE) | EVENT_INDEX(1));
                  radeon_add_to_buffer_list(ctx, &ctx->gfx_cs, scratch,
               radeon_emit(PKT3(PKT3_RELEASE_MEM, ctx->gfx_level >= GFX9 ? 6 : 5, 0));
   radeon_emit(op);
   radeon_emit(sel);
   radeon_emit(va);        /* address lo */
   radeon_emit(va >> 32);  /* address hi */
   radeon_emit(new_fence); /* immediate data lo */
   radeon_emit(0);         /* immediate data hi */
   if (ctx->gfx_level >= GFX9)
      } else {
      if (ctx->gfx_level == GFX7 || ctx->gfx_level == GFX8) {
                     /* Two EOP events are required to make all engines go idle
   * (and optional cache flushes executed) before the timestamp
   * is written.
   */
   radeon_emit(PKT3(PKT3_EVENT_WRITE_EOP, 4, 0));
   radeon_emit(op);
   radeon_emit(va);
   radeon_emit(((va >> 32) & 0xffff) | sel);
                  radeon_add_to_buffer_list(ctx, &ctx->gfx_cs, scratch,
               radeon_emit(PKT3(PKT3_EVENT_WRITE_EOP, 4, 0));
   radeon_emit(op);
   radeon_emit(va);
   radeon_emit(((va >> 32) & 0xffff) | sel);
   radeon_emit(new_fence); /* immediate data */
                        if (buf) {
            }
      unsigned si_cp_write_fence_dwords(struct si_screen *screen)
   {
               if (screen->info.gfx_level == GFX7 || screen->info.gfx_level == GFX8)
               }
      void si_cp_wait_mem(struct si_context *ctx, struct radeon_cmdbuf *cs, uint64_t va, uint32_t ref,
         {
      radeon_begin(cs);
   radeon_emit(PKT3(PKT3_WAIT_REG_MEM, 5, 0));
   radeon_emit(WAIT_REG_MEM_MEM_SPACE(1) | flags);
   radeon_emit(va);
   radeon_emit(va >> 32);
   radeon_emit(ref);  /* reference value */
   radeon_emit(mask); /* mask */
   radeon_emit(4);    /* poll interval */
      }
      static void si_add_fence_dependency(struct si_context *sctx, struct pipe_fence_handle *fence)
   {
                  }
      static void si_add_syncobj_signal(struct si_context *sctx, struct pipe_fence_handle *fence)
   {
         }
      static void si_fence_reference(struct pipe_screen *screen, struct pipe_fence_handle **dst,
         {
      struct radeon_winsys *ws = ((struct si_screen *)screen)->ws;
   struct si_fence **sdst = (struct si_fence **)dst;
            if (pipe_reference(&(*sdst)->reference, &ssrc->reference)) {
      ws->fence_reference(&(*sdst)->gfx, NULL);
   tc_unflushed_batch_token_reference(&(*sdst)->tc_token, NULL);
   si_resource_reference(&(*sdst)->fine.buf, NULL);
      }
      }
      static struct si_fence *si_alloc_fence()
   {
      struct si_fence *fence = CALLOC_STRUCT(si_fence);
   if (!fence)
            pipe_reference_init(&fence->reference, 1);
               }
      struct pipe_fence_handle *si_create_fence(struct pipe_context *ctx,
         {
      struct si_fence *fence = si_alloc_fence();
   if (!fence)
            util_queue_fence_reset(&fence->ready);
               }
      static bool si_fine_fence_signaled(struct radeon_winsys *rws, const struct si_fine_fence *fine)
   {
      char *map =
         if (!map)
            uint32_t *fence = (uint32_t *)(map + fine->offset);
      }
      static void si_fine_fence_set(struct si_context *ctx, struct si_fine_fence *fine, unsigned flags)
   {
                        /* Use cached system memory for the fence. */
   u_upload_alloc(ctx->cached_gtt_allocator, 0, 4, 4, &fine->offset,
         if (!fine->buf)
                     if (flags & PIPE_FLUSH_TOP_OF_PIPE) {
                  } else if (flags & PIPE_FLUSH_BOTTOM_OF_PIPE) {
               radeon_add_to_buffer_list(ctx, &ctx->gfx_cs, fine->buf, RADEON_USAGE_WRITE | RADEON_PRIO_QUERY);
   si_cp_release_mem(ctx, &ctx->gfx_cs, V_028A90_BOTTOM_OF_PIPE_TS, 0, EOP_DST_SEL_MEM,
            } else {
            }
      static bool si_fence_finish(struct pipe_screen *screen, struct pipe_context *ctx,
         {
      struct radeon_winsys *rws = ((struct si_screen *)screen)->ws;
   struct si_fence *sfence = (struct si_fence *)fence;
   struct si_context *sctx;
            ctx = threaded_context_unwrap_sync(ctx);
            if (!util_queue_fence_is_signalled(&sfence->ready)) {
      if (sfence->tc_token) {
      /* Ensure that si_flush_from_st will be called for
   * this fence, but only if we're in the API thread
   * where the context is current.
   *
   * Note that the batch containing the flush may already
   * be in flight in the driver thread, so the fence
   * may not be ready yet when this call returns.
   */
               if (!timeout)
            if (timeout == OS_TIMEOUT_INFINITE) {
         } else {
      if (!util_queue_fence_wait_timeout(&sfence->ready, abs_timeout))
               if (timeout && timeout != OS_TIMEOUT_INFINITE) {
      int64_t time = os_time_get_nano();
                  if (!sfence->gfx)
            if (sfence->fine.buf && si_fine_fence_signaled(rws, &sfence->fine)) {
      rws->fence_reference(&sfence->gfx, NULL);
   si_resource_reference(&sfence->fine.buf, NULL);
               /* Flush the gfx IB if it hasn't been flushed yet. */
   if (sctx && sfence->gfx_unflushed.ctx == sctx &&
      sfence->gfx_unflushed.ib_index == sctx->num_gfx_cs_flushes) {
   /* Section 4.1.2 (Signaling) of the OpenGL 4.6 (Core profile)
   * spec says:
   *
   *    "If the sync object being blocked upon will not be
   *     signaled in finite time (for example, by an associated
   *     fence command issued previously, but not yet flushed to
   *     the graphics pipeline), then ClientWaitSync may hang
   *     forever. To help prevent this behavior, if
   *     ClientWaitSync is called and all of the following are
   *     true:
   *
   *     * the SYNC_FLUSH_COMMANDS_BIT bit is set in flags,
   *     * sync is unsignaled when ClientWaitSync is called,
   *     * and the calls to ClientWaitSync and FenceSync were
   *       issued from the same context,
   *
   *     then the GL will behave as if the equivalent of Flush
   *     were inserted immediately after the creation of sync."
   *
   * This means we need to flush for such fences even when we're
   * not going to wait.
   */
   si_flush_gfx_cs(sctx, (timeout ? 0 : PIPE_FLUSH_ASYNC) | RADEON_FLUSH_START_NEXT_GFX_IB_NOW,
                  if (!timeout)
            /* Recompute the timeout after all that. */
   if (timeout && timeout != OS_TIMEOUT_INFINITE) {
      int64_t time = os_time_get_nano();
                  if (rws->fence_wait(rws, sfence->gfx, timeout))
            /* Re-check in case the GPU is slow or hangs, but the commands before
   * the fine-grained fence have completed. */
   if (sfence->fine.buf && si_fine_fence_signaled(rws, &sfence->fine))
               }
      static void si_create_fence_fd(struct pipe_context *ctx, struct pipe_fence_handle **pfence, int fd,
         {
      struct si_screen *sscreen = (struct si_screen *)ctx->screen;
   struct radeon_winsys *ws = sscreen->ws;
                     sfence = si_alloc_fence();
   if (!sfence)
            switch (type) {
   case PIPE_FD_TYPE_NATIVE_SYNC:
      if (!sscreen->info.has_fence_to_handle)
            sfence->gfx = ws->fence_import_sync_file(ws, fd);
         case PIPE_FD_TYPE_SYNCOBJ:
      if (!sscreen->info.has_syncobj)
            sfence->gfx = ws->fence_import_syncobj(ws, fd);
         default:
               finish:
      if (!sfence->gfx) {
      FREE(sfence);
                  }
      static int si_fence_get_fd(struct pipe_screen *screen, struct pipe_fence_handle *fence)
   {
      struct si_screen *sscreen = (struct si_screen *)screen;
   struct radeon_winsys *ws = sscreen->ws;
   struct si_fence *sfence = (struct si_fence *)fence;
            if (!sscreen->info.has_fence_to_handle)
                     /* Deferred fences aren't supported. */
   assert(!sfence->gfx_unflushed.ctx);
   if (sfence->gfx_unflushed.ctx)
            if (sfence->gfx) {
      gfx_fd = ws->fence_export_sync_file(ws, sfence->gfx);
   if (gfx_fd == -1) {
                     /* If we don't have FDs at this point, it means we don't have fences
   * either. */
   if (gfx_fd == -1)
               }
      static void si_flush_all_queues(struct pipe_context *ctx,
               {
      struct pipe_screen *screen = ctx->screen;
   struct si_context *sctx = (struct si_context *)ctx;
   struct radeon_winsys *ws = sctx->ws;
   struct pipe_fence_handle *gfx_fence = NULL;
   bool deferred_fence = false;
   struct si_fine_fence fine = {};
            if (!(flags & PIPE_FLUSH_DEFERRED)) {
                  if (flags & PIPE_FLUSH_END_OF_FRAME)
            if (flags & (PIPE_FLUSH_TOP_OF_PIPE | PIPE_FLUSH_BOTTOM_OF_PIPE)) {
      assert(flags & PIPE_FLUSH_DEFERRED);
                        if (force_flush) {
                  if (!radeon_emitted(&sctx->gfx_cs, sctx->initial_gfx_cs_size)) {
      if (fence)
         if (!(flags & PIPE_FLUSH_DEFERRED))
                     if (unlikely(sctx->sqtt && (flags & PIPE_FLUSH_END_OF_FRAME))) {
         }
      if (u_trace_perfetto_active(&sctx->ds.trace_context)) {
            } else {
      /* Instead of flushing, create a deferred fence. Constraints:
   * - the gallium frontend must allow a deferred flush.
   * - the gallium frontend must request a fence.
   * - fence_get_fd is not allowed.
   * Thread safety in fence_finish must be ensured by the gallium frontend.
   */
   if (flags & PIPE_FLUSH_DEFERRED && !(flags & PIPE_FLUSH_FENCE_FD) && fence) {
      gfx_fence = sctx->ws->cs_get_next_fence(&sctx->gfx_cs);
      } else {
                     /* Both engines can signal out of order, so we need to keep both fences. */
   if (fence) {
               if (flags & TC_FLUSH_ASYNC) {
      new_fence = (struct si_fence *)*fence;
      } else {
      new_fence = si_alloc_fence();
   if (!new_fence) {
      ws->fence_reference(&gfx_fence, NULL);
               screen->fence_reference(screen, fence, NULL);
               /* If both fences are NULL, fence_finish will always return true. */
            if (deferred_fence) {
      new_fence->gfx_unflushed.ctx = sctx;
               new_fence->fine = fine;
            if (flags & TC_FLUSH_ASYNC) {
      util_queue_fence_signal(&new_fence->ready);
         }
      finish:
      if (!(flags & (PIPE_FLUSH_DEFERRED | PIPE_FLUSH_ASYNC))) {
            }
      static void si_flush_from_st(struct pipe_context *ctx, struct pipe_fence_handle **fence,
         {
         }
      static void si_fence_server_signal(struct pipe_context *ctx, struct pipe_fence_handle *fence)
   {
      struct si_context *sctx = (struct si_context *)ctx;
                     if (sfence->gfx)
            /**
   * The spec requires a flush here. We insert a flush
   * because syncobj based signals are not directly placed into
   * the command stream. Instead the signal happens when the
   * submission associated with the syncobj finishes execution.
   *
   * Therefore, we must make sure that we flush the pipe to avoid
   * new work being emitted and getting executed before the signal
   * operation.
   *
   * Forces a flush even if the GFX CS is empty.
   *
   * The flush must not be asynchronous because the kernel must receive
   * the scheduled "signal" operation before any wait.
   */
      }
      static void si_fence_server_sync(struct pipe_context *ctx, struct pipe_fence_handle *fence)
   {
      struct si_context *sctx = (struct si_context *)ctx;
                     /* Unflushed fences from the same context are no-ops. */
   if (sfence->gfx_unflushed.ctx && sfence->gfx_unflushed.ctx == sctx)
            /* All unflushed commands will not start execution before this fence
   * dependency is signalled. That's fine. Flushing is very expensive
   * if we get fence_server_sync after every draw call. (which happens
   * with Android/SurfaceFlinger)
   *
   * In a nutshell, when CPU overhead is greater than GPU overhead,
   * or when the time it takes to execute an IB on the GPU is less than
   * the time it takes to create and submit that IB, flushing decreases
   * performance. Therefore, DO NOT FLUSH.
   */
   if (sfence->gfx)
      }
      void si_init_fence_functions(struct si_context *ctx)
   {
      ctx->b.flush = si_flush_from_st;
   ctx->b.create_fence_fd = si_create_fence_fd;
   ctx->b.fence_server_sync = si_fence_server_sync;
      }
      void si_init_screen_fence_functions(struct si_screen *screen)
   {
      screen->b.fence_finish = si_fence_finish;
   screen->b.fence_reference = si_fence_reference;
      }
