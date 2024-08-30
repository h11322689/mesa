   /*
   * Copyright 2022 Alyssa Rosenzweig
   * Copyright 2019-2020 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include <xf86drm.h>
   #include "asahi/lib/decode.h"
   #include "agx_state.h"
      #define foreach_active(ctx, idx)                                               \
            #define foreach_submitted(ctx, idx)                                            \
            #define batch_debug(batch, fmt, ...)                                           \
      do {                                                                        \
      if (unlikely(agx_device(batch->ctx->base.screen)->debug &                \
                  static unsigned
   agx_batch_idx(struct agx_batch *batch)
   {
         }
      bool
   agx_batch_is_active(struct agx_batch *batch)
   {
         }
      bool
   agx_batch_is_submitted(struct agx_batch *batch)
   {
         }
      static void
   agx_batch_mark_active(struct agx_batch *batch)
   {
                        assert(!BITSET_TEST(batch->ctx->batches.submitted, batch_idx));
   assert(!BITSET_TEST(batch->ctx->batches.active, batch_idx));
      }
      static void
   agx_batch_mark_submitted(struct agx_batch *batch)
   {
                        assert(BITSET_TEST(batch->ctx->batches.active, batch_idx));
   assert(!BITSET_TEST(batch->ctx->batches.submitted, batch_idx));
   BITSET_CLEAR(batch->ctx->batches.active, batch_idx);
      }
      static void
   agx_batch_mark_complete(struct agx_batch *batch)
   {
                        assert(!BITSET_TEST(batch->ctx->batches.active, batch_idx));
   assert(BITSET_TEST(batch->ctx->batches.submitted, batch_idx));
      }
      static void
   agx_batch_init(struct agx_context *ctx,
               {
               batch->ctx = ctx;
   util_copy_framebuffer_state(&batch->key, key);
            agx_pool_init(&batch->pool, dev, 0, true);
            /* These allocations can happen only once and will just be zeroed (not freed)
   * during batch clean up. The memory is owned by the context.
   */
   if (!batch->bo_list.set) {
      batch->bo_list.set = rzalloc_array(ctx, BITSET_WORD, 128);
      } else {
      memset(batch->bo_list.set, 0,
               batch->encoder = agx_bo_create(dev, 0x80000, 0, "Encoder");
   batch->encoder_current = batch->encoder->ptr.cpu;
            util_dynarray_init(&batch->scissor, ctx);
   util_dynarray_init(&batch->depth_bias, ctx);
   util_dynarray_init(&batch->occlusion_queries, ctx);
            batch->clear = 0;
   batch->draw = 0;
   batch->load = 0;
   batch->resolve = 0;
   batch->clear_depth = 0;
   batch->clear_stencil = 0;
   batch->varyings = 0;
   batch->any_draws = false;
   batch->initialized = false;
            /* We need to emit prim state at the start. Max collides with all. */
            if (!batch->syncobj) {
      int ret = drmSyncobjCreate(dev->fd, 0, &batch->syncobj);
                  }
      static void
   agx_batch_print_stats(struct agx_device *dev, struct agx_batch *batch)
   {
         }
      static void
   agx_batch_cleanup(struct agx_context *ctx, struct agx_batch *batch, bool reset)
   {
      struct agx_device *dev = agx_device(ctx->base.screen);
   assert(batch->ctx == ctx);
                     agx_finish_batch_queries(batch);
   batch->occlusion_buffer.cpu = NULL;
            if (reset) {
      int handle;
   AGX_BATCH_FOREACH_BO_HANDLE(batch, handle) {
                           } else {
      int handle;
   AGX_BATCH_FOREACH_BO_HANDLE(batch, handle) {
                                                                     agx_bo_unreference(batch->encoder);
   agx_pool_cleanup(&batch->pool);
            util_dynarray_fini(&batch->scissor);
   util_dynarray_fini(&batch->depth_bias);
   util_dynarray_fini(&batch->occlusion_queries);
            if (!(dev->debug & (AGX_DBG_TRACE | AGX_DBG_SYNC))) {
            }
      int
   agx_cleanup_batches(struct agx_context *ctx)
   {
               unsigned i;
   unsigned count = 0;
   struct agx_batch *batches[AGX_MAX_BATCHES];
   uint32_t syncobjs[AGX_MAX_BATCHES];
            foreach_submitted(ctx, i) {
      batches[count] = &ctx->batches.slots[i];
               if (!count)
            int ret = drmSyncobjWait(dev->fd, syncobjs, count, 0, 0, &first);
   assert(!ret || ret == -ETIME);
   if (ret)
            assert(first < AGX_MAX_BATCHES);
   agx_batch_cleanup(ctx, batches[first], false);
      }
      static struct agx_batch *
   agx_get_batch_for_framebuffer(struct agx_context *ctx,
         {
      /* Look if we have a matching batch */
   unsigned i;
   foreach_active(ctx, i) {
               if (util_framebuffer_state_equal(&candidate->key, state)) {
      /* We found a match, increase the seqnum for the LRU
   * eviction logic.
   */
   candidate->seqnum = ++ctx->batches.seqnum;
                  /* Look for a free batch */
   for (i = 0; i < AGX_MAX_BATCHES; ++i) {
      if (!BITSET_TEST(ctx->batches.active, i) &&
      !BITSET_TEST(ctx->batches.submitted, i)) {
   struct agx_batch *batch = &ctx->batches.slots[i];
   agx_batch_init(ctx, state, batch);
                  /* Try to clean up one batch */
   int freed = agx_cleanup_batches(ctx);
   if (freed >= 0) {
      struct agx_batch *batch = &ctx->batches.slots[freed];
   agx_batch_init(ctx, state, batch);
               /* Else, evict something */
   struct agx_batch *batch = NULL;
   bool submitted = false;
   for (i = 0; i < AGX_MAX_BATCHES; ++i) {
      struct agx_batch *candidate = &ctx->batches.slots[i];
            /* Prefer submitted batches first */
   if (!cand_submitted && submitted)
            if (!batch || batch->seqnum > candidate->seqnum) {
      batch = candidate;
         }
                     /* Batch is now free */
   agx_batch_init(ctx, state, batch);
      }
      struct agx_batch *
   agx_get_batch(struct agx_context *ctx)
   {
      if (!ctx->batch) {
      ctx->batch = agx_get_batch_for_framebuffer(ctx, &ctx->framebuffer);
               assert(util_framebuffer_state_equal(&ctx->framebuffer, &ctx->batch->key));
      }
      struct agx_batch *
   agx_get_compute_batch(struct agx_context *ctx)
   {
               struct pipe_framebuffer_state key = {.width = AGX_COMPUTE_BATCH_WIDTH};
   ctx->batch = agx_get_batch_for_framebuffer(ctx, &key);
      }
      void
   agx_flush_all(struct agx_context *ctx, const char *reason)
   {
      unsigned idx;
   foreach_active(ctx, idx) {
      if (reason)
                  }
      void
   agx_flush_batch_for_reason(struct agx_context *ctx, struct agx_batch *batch,
         {
      if (reason)
            if (agx_batch_is_active(batch))
      }
      static void
   agx_flush_readers_except(struct agx_context *ctx, struct agx_resource *rsrc,
               {
               /* Flush everything to the hardware first */
   foreach_active(ctx, idx) {
               if (batch == except)
            if (agx_batch_uses_bo(batch, rsrc->bo)) {
      perf_debug_ctx(ctx, "Flush reader due to: %s\n", reason);
                  /* Then wait on everything if necessary */
   if (sync) {
      foreach_submitted(ctx, idx) {
                              if (agx_batch_uses_bo(batch, rsrc->bo)) {
      perf_debug_ctx(ctx, "Sync reader due to: %s\n", reason);
               }
      static void
   agx_flush_writer_except(struct agx_context *ctx, struct agx_resource *rsrc,
         {
               if (writer && writer != except &&
      (agx_batch_is_active(writer) || agx_batch_is_submitted(writer))) {
   if (agx_batch_is_active(writer) || sync) {
      perf_debug_ctx(ctx, "%s writer due to: %s\n", sync ? "Sync" : "Flush",
      }
   if (agx_batch_is_active(writer))
         /* Check for submitted state, because if the batch was a no-op it'll
   * already be cleaned up */
   if (sync && agx_batch_is_submitted(writer))
         }
      bool
   agx_any_batch_uses_resource(struct agx_context *ctx, struct agx_resource *rsrc)
   {
      unsigned idx;
   foreach_active(ctx, idx) {
               if (agx_batch_uses_bo(batch, rsrc->bo))
               foreach_submitted(ctx, idx) {
               if (agx_batch_uses_bo(batch, rsrc->bo))
                  }
      void
   agx_flush_readers(struct agx_context *ctx, struct agx_resource *rsrc,
         {
         }
      void
   agx_sync_readers(struct agx_context *ctx, struct agx_resource *rsrc,
         {
         }
      void
   agx_flush_writer(struct agx_context *ctx, struct agx_resource *rsrc,
         {
         }
      void
   agx_sync_writer(struct agx_context *ctx, struct agx_resource *rsrc,
         {
         }
      void
   agx_batch_reads(struct agx_batch *batch, struct agx_resource *rsrc)
   {
      /* Hazard: read-after-write */
   agx_flush_writer_except(batch->ctx, rsrc, batch, "Read from another batch",
                     if (rsrc->separate_stencil)
      }
      void
   agx_batch_writes(struct agx_batch *batch, struct agx_resource *rsrc)
   {
      struct agx_context *ctx = batch->ctx;
                              /* Nothing to do if we're already writing */
   if (writer == batch)
            /* Hazard: writer-after-write, write-after-read */
   if (writer)
            /* Write is strictly stronger than a read */
            writer = agx_writer_get(ctx, rsrc->bo->handle);
            /* We are now the new writer. Disregard the previous writer -- anything that
   * needs to wait for the writer going forward needs to wait for us.
   */
   agx_writer_remove(ctx, rsrc->bo->handle);
            if (rsrc->base.target == PIPE_BUFFER) {
      /* Assume BOs written by the GPU are fully valid */
   rsrc->valid_buffer_range.start = 0;
         }
      static int
   agx_get_in_sync(struct agx_context *ctx)
   {
               if (ctx->in_sync_fd >= 0) {
      int ret =
                  close(ctx->in_sync_fd);
               } else {
            }
      static void
   agx_add_sync(struct drm_asahi_sync *syncs, unsigned *count, uint32_t handle)
   {
      if (!handle)
            syncs[(*count)++] = (struct drm_asahi_sync){
      .sync_type = DRM_ASAHI_SYNC_SYNCOBJ,
         }
      void
   agx_batch_submit(struct agx_context *ctx, struct agx_batch *batch,
               {
                     #ifndef NDEBUG
      /* Debug builds always get feedback (for fault checks) */
      #endif
         if (!feedback)
            /* We allocate the worst-case sync array size since this won't be excessive
   * for most workloads
   */
   unsigned max_syncs = agx_batch_bo_list_bits(batch) + 1;
   unsigned in_sync_count = 0;
   unsigned shared_bo_count = 0;
   struct drm_asahi_sync *in_syncs =
                  struct drm_asahi_sync out_sync = {
      .sync_type = DRM_ASAHI_SYNC_SYNCOBJ,
               int handle;
   AGX_BATCH_FOREACH_BO_HANDLE(batch, handle) {
               if (bo->flags & AGX_BO_SHARED) {
               /* Get a sync file fd from the buffer */
                  /* Create a new syncobj */
   uint32_t sync_handle;
                  /* Import the sync file into it */
   ret = drmSyncobjImportSyncFile(dev->fd, sync_handle, in_sync_fd);
   assert(ret >= 0);
                                 /* And keep track of the BO for cloning the out_sync */
                  /* Add an explicit fence from gallium, if any */
            /* Submit! */
   agx_submit_single(
      dev, cmd_type, barriers, in_syncs, in_sync_count, &out_sync, 1, cmdbuf,
   feedback ? ctx->result_buf->handle : 0, feedback ? batch->result_off : 0,
         /* Now stash our batch fence into any shared BOs. */
   if (shared_bo_count) {
      /* Convert our handle to a sync file */
   int out_sync_fd = -1;
   int ret = drmSyncobjExportSyncFile(dev->fd, batch->syncobj, &out_sync_fd);
   assert(ret >= 0);
            for (unsigned i = 0; i < shared_bo_count; i++) {
                     /* Free the in_sync handle we just acquired */
   ret = drmSyncobjDestroy(dev->fd, in_syncs[i].handle);
   assert(ret >= 0);
   /* And then import the out_sync sync file into it */
   ret = agx_import_sync_file(dev, shared_bos[i], out_sync_fd);
                           /* Record the syncobj on each BO we write, so it can be added post-facto as a
   * fence if the BO is exported later...
   */
   AGX_BATCH_FOREACH_BO_HANDLE(batch, handle) {
      struct agx_bo *bo = agx_lookup_bo(dev, handle);
            if (!writer)
            /* Skip BOs that are written by submitted batches, they're not ours */
   if (agx_batch_is_submitted(writer))
            /* But any BOs written by active batches are ours */
   assert(writer == batch && "exclusive writer");
               free(in_syncs);
            if (dev->debug & (AGX_DBG_TRACE | AGX_DBG_SYNC)) {
      /* Wait so we can get errors reported back */
   int ret = drmSyncobjWait(dev->fd, &batch->syncobj, 1, INT64_MAX, 0, NULL);
            if (dev->debug & AGX_DBG_TRACE) {
      /* agxdecode DRM commands */
   switch (cmd_type) {
   default:
         }
                                    /* Record the last syncobj for fence creation */
            if (ctx->batch == batch)
            /* Try to clean up up to two batches, to keep memory usage down */
   if (agx_cleanup_batches(ctx) >= 0)
      }
      void
   agx_sync_batch(struct agx_context *ctx, struct agx_batch *batch)
   {
               if (agx_batch_is_active(batch))
            /* Empty batch case, already cleaned up */
   if (!agx_batch_is_submitted(batch))
            assert(batch->syncobj);
   int ret = drmSyncobjWait(dev->fd, &batch->syncobj, 1, INT64_MAX, 0, NULL);
   assert(!ret);
      }
      void
   agx_sync_batch_for_reason(struct agx_context *ctx, struct agx_batch *batch,
         {
      if (reason)
               }
      void
   agx_sync_all(struct agx_context *ctx, const char *reason)
   {
      if (reason)
            unsigned idx;
   foreach_active(ctx, idx) {
                  foreach_submitted(ctx, idx) {
            }
      void
   agx_batch_reset(struct agx_context *ctx, struct agx_batch *batch)
   {
                        /* Reset an empty batch. Like submit, but does nothing. */
            if (ctx->batch == batch)
               }
