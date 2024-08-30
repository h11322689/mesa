   /*
   * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "freedreno_context.h"
   #include "ir3/ir3_cache.h"
   #include "util/u_upload_mgr.h"
   #include "freedreno_blitter.h"
   #include "freedreno_draw.h"
   #include "freedreno_fence.h"
   #include "freedreno_gmem.h"
   #include "freedreno_program.h"
   #include "freedreno_query.h"
   #include "freedreno_query_hw.h"
   #include "freedreno_resource.h"
   #include "freedreno_state.h"
   #include "freedreno_texture.h"
   #include "freedreno_util.h"
   #include "freedreno_tracepoints.h"
   #include "util/u_trace_gallium.h"
      static void
   fd_context_flush(struct pipe_context *pctx, struct pipe_fence_handle **fencep,
         {
      struct fd_context *ctx = fd_context(pctx);
   struct pipe_fence_handle *fence = NULL;
            /* We want to lookup current batch if it exists, but not create a new
   * one if not (unless we need a fence)
   */
                     if (fencep && !batch) {
         } else if (!batch) {
      if (ctx->screen->reorder)
         fd_bc_dump(ctx, "%p: NULL batch, remaining:\n", ctx);
               /* With TC_FLUSH_ASYNC, the fence will have been pre-created from
   * the front-end thread.  But not yet associated with a batch,
   * because we cannot safely access ctx->batch outside of the driver
   * thread.  So instead, replace the existing batch->fence with the
   * one created earlier
   */
   if ((flags & TC_FLUSH_ASYNC) && fencep) {
      /* We don't currently expect async+flush in the fence-fd
   * case.. for that to work properly we'd need TC to tell
   * us in the create_fence callback that it needs an fd.
   */
            fd_pipe_fence_set_batch(*fencep, batch);
            /* If we have nothing to flush, update the pre-created unflushed
   * fence with the current state of the last-fence:
   */
   if (ctx->last_fence) {
      fd_pipe_fence_repopulate(*fencep, ctx->last_fence);
   fd_pipe_fence_ref(&fence, *fencep);
   fd_bc_dump(ctx, "%p: (deferred) reuse last_fence, remaining:\n", ctx);
               /* async flush is not compatible with deferred flush, since
   * nothing triggers the batch flush which fence_flush() would
   * be waiting for
   */
      } else if (!batch->fence) {
                  /* In some sequence of events, we can end up with a last_fence that is
   * not an "fd" fence, which results in eglDupNativeFenceFDANDROID()
   * errors.
   */
   if ((flags & PIPE_FLUSH_FENCE_FD) && ctx->last_fence &&
      !fd_pipe_fence_is_fd(ctx->last_fence))
         /* if no rendering since last flush, ie. app just decided it needed
   * a fence, re-use the last one:
   */
   if (ctx->last_fence) {
      fd_pipe_fence_ref(&fence, ctx->last_fence);
   fd_bc_dump(ctx, "%p: reuse last_fence, remaining:\n", ctx);
               /* Take a ref to the batch's fence (batch can be unref'd when flushed: */
            if (flags & PIPE_FLUSH_FENCE_FD)
            fd_bc_dump(ctx, "%p: flushing %p<%u>, flags=0x%x, pending:\n", ctx,
            /* If we get here, we need to flush for a fence, even if there is
   * no rendering yet:
   */
            if (!ctx->screen->reorder) {
         } else {
                        out:
      if (fencep)
                                       u_trace_context_process(&ctx->trace_context,
      }
      static void
   fd_texture_barrier(struct pipe_context *pctx, unsigned flags) in_dt
   {
      /* On devices that could sample from GMEM we could possibly do better.
   * Or if we knew that we were doing GMEM bypass we could just emit a
   * cache flush, perhaps?  But we don't know if future draws would cause
   * us to use GMEM, and a flush in bypass isn't the end of the world.
   */
      }
      static void
   fd_memory_barrier(struct pipe_context *pctx, unsigned flags)
   {
      if (!(flags & ~PIPE_BARRIER_UPDATE))
               }
      static void
   emit_string_tail(struct fd_ringbuffer *ring, const char *string, int len)
   {
               while (len >= 4) {
      OUT_RING(ring, *buf);
   buf++;
               /* copy remainder bytes without reading past end of input string: */
   if (len > 0) {
      uint32_t w = 0;
   memcpy(&w, buf, len);
         }
      /* for prior to a5xx: */
   void
   fd_emit_string(struct fd_ringbuffer *ring, const char *string, int len)
   {
      /* max packet size is 0x3fff+1 dwords: */
            OUT_PKT3(ring, CP_NOP, align(len, 4) / 4);
      }
      /* for a5xx+ */
   void
   fd_emit_string5(struct fd_ringbuffer *ring, const char *string, int len)
   {
      /* max packet size is 0x3fff dwords: */
            OUT_PKT7(ring, CP_NOP, align(len, 4) / 4);
      }
      /**
   * emit marker string as payload of a no-op packet, which can be
   * decoded by cffdump.
   */
   static void
   fd_emit_string_marker(struct pipe_context *pctx, const char *string,
         {
                        if (!ctx->batch)
                              if (ctx->screen->gen >= 5) {
         } else {
                     }
      static void
   fd_cs_magic_write_string(void *cs, struct u_trace_context *utctx, int magic,
         {
      struct fd_context *ctx =
         int fmt_len = vsnprintf(NULL, 0, fmt, args);
   int len = 4 + fmt_len + 1;
            /* format: <magic><formatted string>\0 */
   *(uint32_t *)string = magic;
            if (ctx->screen->gen >= 5) {
         } else {
         }
      }
      void
   fd_cs_trace_msg(struct u_trace_context *utctx, void *cs, const char *fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   int magic = CP_NOP_MESG;
   fd_cs_magic_write_string(cs, utctx, magic, fmt, args);
      }
      void
   fd_cs_trace_start(struct u_trace_context *utctx, void *cs, const char *fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   int magic = CP_NOP_BEGN;
   fd_cs_magic_write_string(cs, utctx, magic, fmt, args);
      }
      void
   fd_cs_trace_end(struct u_trace_context *utctx, void *cs, const char *fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   int magic = CP_NOP_END;
   fd_cs_magic_write_string(cs, utctx, magic, fmt, args);
      }
      /**
   * If we have a pending fence_server_sync() (GPU side sync), flush now.
   * The alternative to try to track this with batch dependencies gets
   * hairy quickly.
   *
   * Call this before switching to a different batch, to handle this case.
   */
   void
   fd_context_switch_from(struct fd_context *ctx)
   {
      if (ctx->batch && (ctx->batch->in_fence_fd != -1))
      }
      /**
   * If there is a pending fence-fd that we need to sync on, this will
   * transfer the reference to the next batch we are going to render
   * to.
   */
   void
   fd_context_switch_to(struct fd_context *ctx, struct fd_batch *batch)
   {
      if (ctx->in_fence_fd != -1) {
      sync_accumulate("freedreno", &batch->in_fence_fd, ctx->in_fence_fd);
   close(ctx->in_fence_fd);
         }
      void
   fd_context_add_private_bo(struct fd_context *ctx, struct fd_bo *bo)
   {
      assert(ctx->num_private_bos < ARRAY_SIZE(ctx->private_bos));
      }
      /**
   * Return a reference to the current batch, caller must unref.
   */
   struct fd_batch *
   fd_context_batch(struct fd_context *ctx)
   {
                        if (ctx->batch_nondraw) {
      fd_batch_reference(&ctx->batch_nondraw, NULL);
                        if (unlikely(!batch)) {
      batch =
         fd_batch_reference(&ctx->batch, batch);
      }
               }
      /**
   * Return a reference to the current non-draw (compute/blit) batch.
   */
   struct fd_batch *
   fd_context_batch_nondraw(struct fd_context *ctx)
   {
                                 if (unlikely(!batch)) {
      batch = fd_bc_alloc_batch(ctx, true);
   fd_batch_reference(&ctx->batch_nondraw, batch);
      }
               }
      void
   fd_context_destroy(struct pipe_context *pctx)
   {
      struct fd_context *ctx = fd_context(pctx);
                     fd_screen_lock(ctx->screen);
   list_del(&ctx->node);
                     if (ctx->in_fence_fd != -1)
            for (i = 0; i < ARRAY_SIZE(ctx->pvtmem); i++) {
      if (ctx->pvtmem[i].bo)
               util_copy_framebuffer_state(&ctx->framebuffer, NULL);
            /* Make sure nothing in the batch cache references our context any more. */
                     if (ctx->blitter)
            if (pctx->stream_uploader)
            for (i = 0; i < ARRAY_SIZE(ctx->clear_rs_state); i++)
      if (ctx->clear_rs_state[i])
         slab_destroy_child(&ctx->transfer_pool);
            for (i = 0; i < ARRAY_SIZE(ctx->vsc_pipe_bo); i++) {
      if (!ctx->vsc_pipe_bo[i])
                     fd_device_del(ctx->dev);
   fd_pipe_purge(ctx->pipe);
                                                if (FD_DBG(BSTAT) || FD_DBG(MSGS)) {
      mesa_logi(
      "batch_total=%u, batch_sysmem=%u, batch_gmem=%u, batch_nondraw=%u, "
   "batch_restore=%u\n",
   (uint32_t)ctx->stats.batch_total, (uint32_t)ctx->stats.batch_sysmem,
   (uint32_t)ctx->stats.batch_gmem, (uint32_t)ctx->stats.batch_nondraw,
      }
      static void
   fd_set_debug_callback(struct pipe_context *pctx,
         {
      struct fd_context *ctx = fd_context(pctx);
                     if (cb)
         else
      }
      static uint32_t
   fd_get_reset_count(struct fd_context *ctx, bool per_context)
   {
      uint64_t val;
   enum fd_param_id param = per_context ? FD_CTX_FAULTS : FD_GLOBAL_FAULTS;
   ASSERTED int ret = fd_pipe_get_param(ctx->pipe, param, &val);
   assert(!ret);
      }
      static enum pipe_reset_status
   fd_get_device_reset_status(struct pipe_context *pctx)
   {
      struct fd_context *ctx = fd_context(pctx);
   int context_faults = fd_get_reset_count(ctx, true);
   int global_faults = fd_get_reset_count(ctx, false);
            if (context_faults != ctx->context_reset_count) {
         } else if (global_faults != ctx->global_reset_count) {
         } else {
                  ctx->context_reset_count = context_faults;
               }
      static void
   fd_trace_record_ts(struct u_trace *ut, void *cs, void *timestamps,
         {
      struct fd_batch *batch = container_of(ut, struct fd_batch, trace);
   struct fd_ringbuffer *ring = cs;
            if (ring->cur == batch->last_timestamp_cmd) {
      uint64_t *ts = fd_bo_map(fd_resource(buffer)->bo);
   ts[idx] = U_TRACE_NO_TIMESTAMP;
               unsigned ts_offset = idx * sizeof(uint64_t);
   batch->ctx->record_timestamp(ring, fd_resource(buffer)->bo, ts_offset);
      }
      static uint64_t
   fd_trace_read_ts(struct u_trace_context *utctx,
         {
      struct fd_context *ctx =
         struct pipe_resource *buffer = timestamps;
            /* Only need to stall on results for the first entry: */
   if (idx == 0) {
      /* Avoid triggering deferred submits from flushing, since that
   * changes the behavior of what we are trying to measure:
   */
   while (fd_bo_cpu_prep(ts_bo, ctx->pipe, FD_BO_PREP_NOSYNC))
         int ret = fd_bo_cpu_prep(ts_bo, ctx->pipe, FD_BO_PREP_READ);
   if (ret)
                        /* Don't translate the no-timestamp marker: */
   if (ts[idx] == U_TRACE_NO_TIMESTAMP)
               }
      static void
   fd_trace_delete_flush_data(struct u_trace_context *utctx, void *flush_data)
   {
         }
      /* TODO we could combine a few of these small buffers (solid_vbuf,
   * blit_texcoord_vbuf, and vsc_size_mem, into a single buffer and
   * save a tiny bit of memory
   */
      static struct pipe_resource *
   create_solid_vertexbuf(struct pipe_context *pctx)
   {
      static const float init_shader_const[] = {
         };
   struct pipe_resource *prsc =
      pipe_buffer_create(pctx->screen, PIPE_BIND_CUSTOM, PIPE_USAGE_IMMUTABLE,
      pipe_buffer_write(pctx, prsc, 0, sizeof(init_shader_const),
            }
      static struct pipe_resource *
   create_blit_texcoord_vertexbuf(struct pipe_context *pctx)
   {
      struct pipe_resource *prsc = pipe_buffer_create(
            }
      void
   fd_context_setup_common_vbos(struct fd_context *ctx)
   {
               ctx->solid_vbuf = create_solid_vertexbuf(pctx);
            /* setup solid_vbuf_state: */
   ctx->solid_vbuf_state.vtx = pctx->create_vertex_elements_state(
      pctx, 1,
   (struct pipe_vertex_element[]){{
      .vertex_buffer_index = 0,
   .src_offset = 0,
   .src_format = PIPE_FORMAT_R32G32B32_FLOAT,
         ctx->solid_vbuf_state.vertexbuf.count = 1;
            /* setup blit_vbuf_state: */
   ctx->blit_vbuf_state.vtx = pctx->create_vertex_elements_state(
      pctx, 2,
   (struct pipe_vertex_element[]){
      {
      .vertex_buffer_index = 0,
   .src_offset = 0,
   .src_format = PIPE_FORMAT_R32G32_FLOAT,
      },
   {
      .vertex_buffer_index = 1,
   .src_offset = 0,
   .src_format = PIPE_FORMAT_R32G32B32_FLOAT,
      ctx->blit_vbuf_state.vertexbuf.count = 2;
   ctx->blit_vbuf_state.vertexbuf.vb[0].buffer.resource =
            }
      void
   fd_context_cleanup_common_vbos(struct fd_context *ctx)
   {
               pctx->delete_vertex_elements_state(pctx, ctx->solid_vbuf_state.vtx);
            pipe_resource_reference(&ctx->solid_vbuf, NULL);
      }
      struct pipe_context *
   fd_context_init(struct fd_context *ctx, struct pipe_screen *pscreen,
               {
      struct fd_screen *screen = fd_screen(pscreen);
   struct pipe_context *pctx;
            /* lower numerical value == higher priority: */
   if (FD_DBG(HIPRIO))
         else if (flags & PIPE_CONTEXT_HIGH_PRIORITY)
         else if (flags & PIPE_CONTEXT_LOW_PRIORITY)
            /* Some of the stats will get printed out at context destroy, so
   * make sure they are collected:
   */
   if (FD_DBG(BSTAT) || FD_DBG(MSGS))
            ctx->flags = flags;
   ctx->screen = screen;
                     if (fd_device_version(screen->dev) >= FD_VERSION_ROBUSTNESS) {
      ctx->context_reset_count = fd_get_reset_count(ctx, true);
                        /* need some sane default in case gallium frontends don't
   * set some state:
   */
   ctx->sample_mask = 0xffff;
            pctx = &ctx->base;
   pctx->screen = pscreen;
   pctx->priv = priv;
   pctx->flush = fd_context_flush;
   pctx->emit_string_marker = fd_emit_string_marker;
   pctx->set_debug_callback = fd_set_debug_callback;
   pctx->get_device_reset_status = fd_get_device_reset_status;
   pctx->create_fence_fd = fd_create_pipe_fence_fd;
   pctx->fence_server_sync = fd_pipe_fence_server_sync;
   pctx->fence_server_signal = fd_pipe_fence_server_signal;
   pctx->texture_barrier = fd_texture_barrier;
            pctx->stream_uploader = u_upload_create_default(pctx);
   if (!pctx->stream_uploader)
                  slab_create_child(&ctx->transfer_pool, &screen->transfer_pool);
            fd_draw_init(pctx);
   fd_resource_context_init(pctx);
   fd_query_context_init(pctx);
   fd_texture_init(pctx);
            ctx->blitter = util_blitter_create(pctx);
   if (!ctx->blitter)
            list_inithead(&ctx->hw_active_queries);
            fd_screen_lock(ctx->screen);
   ctx->seqno = seqno_next_u16(&screen->ctx_seqno);
   list_add(&ctx->node, &ctx->screen->context_list);
                     fd_gpu_tracepoint_config_variable();
   u_trace_pipe_context_init(&ctx->trace_context, pctx,
                                       fail:
      pctx->destroy(pctx);
      }
      struct pipe_context *
   fd_context_init_tc(struct pipe_context *pctx, unsigned flags)
   {
               if (!(flags & PIPE_CONTEXT_PREFER_THREADED))
            /* Clover (compute-only) is unsupported. */
   if (flags & PIPE_CONTEXT_COMPUTE_ONLY)
            struct pipe_context *tc = threaded_context_create(
      pctx, &ctx->screen->transfer_pool,
   fd_replace_buffer_storage,
   &(struct threaded_context_options){
      .create_fence = fd_pipe_fence_create_unflushed,
   .is_resource_busy = fd_resource_busy,
   .unsynchronized_get_device_reset_status = true,
      },
         if (tc && tc != pctx)
               }
