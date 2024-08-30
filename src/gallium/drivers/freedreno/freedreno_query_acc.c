   /*
   * Copyright (C) 2017 Rob Clark <robclark@freedesktop.org>
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
      #include "util/u_inlines.h"
   #include "util/u_memory.h"
      #include "freedreno_context.h"
   #include "freedreno_query_acc.h"
   #include "freedreno_resource.h"
   #include "freedreno_util.h"
      static void
   fd_acc_destroy_query(struct fd_context *ctx, struct fd_query *q) assert_dt
   {
                        pipe_resource_reference(&aq->prsc, NULL);
            free(aq->query_data);
      }
      static void
   realloc_query_bo(struct fd_context *ctx, struct fd_acc_query *aq)
   {
      struct fd_resource *rsc;
                     aq->prsc =
            /* don't assume the buffer is zero-initialized: */
                     map = fd_bo_map(rsc->bo);
      }
      static void
   fd_acc_query_pause(struct fd_acc_query *aq) assert_dt
   {
               if (!aq->batch)
            fd_batch_needs_flush(aq->batch);
   p->pause(aq, aq->batch);
      }
      static void
   fd_acc_query_resume(struct fd_acc_query *aq, struct fd_batch *batch) assert_dt
   {
               fd_screen_lock(batch->ctx->screen);
   fd_batch_resource_write(batch, fd_resource(aq->prsc));
            aq->batch = batch;
   fd_batch_needs_flush(aq->batch);
      }
      static void
   fd_acc_begin_query(struct fd_context *ctx, struct fd_query *q) assert_dt
   {
                        /* ->begin_query() discards previous results, so realloc bo: */
            /* Signal that we need to update the active queries on the next draw */
            /* add to active list: */
   assert(list_is_empty(&aq->node));
            /* TIMESTAMP/GPU_FINISHED and don't do normal bracketing at draw time, we
   * need to just emit the capture at this moment.
   */
   if (skip_begin_query(q->type)) {
      struct fd_batch *batch = fd_context_batch(ctx);
   fd_acc_query_resume(aq, batch);
         }
      static void
   fd_acc_end_query(struct fd_context *ctx, struct fd_query *q) assert_dt
   {
                                 /* remove from active list: */
            /* mark the result available: */
   struct fd_batch *batch = fd_context_batch(ctx);
   struct fd_ringbuffer *ring = batch->draw;
            if (ctx->screen->gen < 5) {
      OUT_PKT3(ring, CP_MEM_WRITE, 3);
   OUT_RELOC(ring, rsc->bo, 0, 0, 0);
   OUT_RING(ring, 1);     /* low 32b */
      } else {
      OUT_PKT7(ring, CP_MEM_WRITE, 4);
   OUT_RELOC(ring, rsc->bo, 0, 0, 0);
   OUT_RING(ring, 1);     /* low 32b */
                  }
      static bool
   fd_acc_get_query_result(struct fd_context *ctx, struct fd_query *q, bool wait,
         {
      struct fd_acc_query *aq = fd_acc_query(q);
   const struct fd_acc_sample_provider *p = aq->provider;
                              /* ARB_occlusion_query says:
   *
   *     "Querying the state for a given occlusion query forces that
   *      occlusion query to complete within a finite amount of time."
   *
   * So, regardless of whether we are supposed to wait or not, we do need to
   * flush now.
   */
   if (fd_get_query_result_in_driver_thread(q)) {
      tc_assert_driver_thread(ctx->tc);
   fd_context_access_begin(ctx);
   fd_bc_flush_writer(ctx, rsc);
               if (!wait) {
      int ret = fd_resource_wait(
         if (ret)
      } else {
                  struct fd_acc_query_sample *s = fd_bo_map(rsc->bo);
               }
      static void
   fd_acc_get_query_result_resource(struct fd_context *ctx, struct fd_query *q,
                                 {
      struct fd_acc_query *aq = fd_acc_query(q);
   const struct fd_acc_sample_provider *p = aq->provider;
                     fd_screen_lock(batch->ctx->screen);
   fd_batch_resource_write(batch, dst);
            /* query_buffer_object isn't really the greatest thing for a tiler,
   * if the app tries to use the result of the query in the same batch.
   * In general the query result isn't truly ready until the last gmem
   * bin/tile.
   *
   * So, we mark the query result as not being available in the draw
   * ring (which technically is true), and then in epilogue ring we
   * update the query dst buffer with the *actual* results and status.
   */
   if (index == -1) {
      /* Mark the query as not-ready in the draw ring: */
   struct fd_ringbuffer *ring = batch->draw;
            OUT_PKT7(ring, CP_MEM_WRITE, is_64b ? 4 : 3);
   OUT_RELOC(ring, dst->bo, offset, 0, 0);
   OUT_RING(ring, 0);     /* low 32b */
   if (is_64b)
                        if (index == -1) {
         } else {
                  /* If we are told to wait for results, then we need to flush.  For an IMR
   * this would just be a wait on the GPU, but the expectation is that draws
   * following this one see the results of the query, which means we need to
   * use the big flush-hammer :-(
   */
   if (flags & PIPE_QUERY_WAIT)
               }
      static const struct fd_query_funcs acc_query_funcs = {
      .destroy_query = fd_acc_destroy_query,
   .begin_query = fd_acc_begin_query,
   .end_query = fd_acc_end_query,
   .get_query_result = fd_acc_get_query_result,
      };
      struct fd_query *
   fd_acc_create_query2(struct fd_context *ctx, unsigned query_type,
               {
      struct fd_acc_query *aq;
            aq = CALLOC_STRUCT(fd_acc_query);
   if (!aq)
                     aq->provider = provider;
                     q = &aq->base;
   q->funcs = &acc_query_funcs;
   q->type = query_type;
               }
      struct fd_query *
   fd_acc_create_query(struct fd_context *ctx, unsigned query_type, unsigned index)
   {
               if ((idx < 0) || !ctx->acc_sample_providers[idx])
            return fd_acc_create_query2(ctx, query_type, index,
      }
      /* Called at clear/draw/blit time to enable/disable the appropriate queries in
   * the batch (and transfer active querying between batches in the case of
   * batch reordering).
   */
   void
   fd_acc_query_update_batch(struct fd_batch *batch, bool disable_all)
   {
               if (disable_all || (ctx->dirty & FD_DIRTY_QUERY)) {
      struct fd_acc_query *aq;
   LIST_FOR_EACH_ENTRY (aq, &ctx->acc_active_queries, node) {
      bool batch_change = aq->batch != batch;
   bool was_active = aq->batch != NULL;
                  if (was_active && (!now_active || batch_change))
         if (now_active && (!was_active || batch_change))
            }
      void
   fd_acc_query_register_provider(struct pipe_context *pctx,
         {
      struct fd_context *ctx = fd_context(pctx);
            assert((0 <= idx) && (idx < MAX_HW_SAMPLE_PROVIDERS));
               }
