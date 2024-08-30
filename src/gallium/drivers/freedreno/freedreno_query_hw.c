   /*
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
      #include "pipe/p_state.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
      #include "freedreno_context.h"
   #include "freedreno_query_hw.h"
   #include "freedreno_resource.h"
   #include "freedreno_util.h"
      struct fd_hw_sample_period {
      struct fd_hw_sample *start, *end;
      };
      static struct fd_hw_sample *
   get_sample(struct fd_batch *batch, struct fd_ringbuffer *ring,
         {
      struct fd_context *ctx = batch->ctx;
   struct fd_hw_sample *samp = NULL;
                     if (!batch->sample_cache[idx]) {
      struct fd_hw_sample *new_samp =
         fd_hw_sample_reference(ctx, &batch->sample_cache[idx], new_samp);
   util_dynarray_append(&batch->samples, struct fd_hw_sample *, new_samp);
                           }
      static void
   clear_sample_cache(struct fd_batch *batch)
   {
               for (i = 0; i < ARRAY_SIZE(batch->sample_cache); i++)
      }
      static bool
   query_active_in_batch(struct fd_batch *batch, struct fd_hw_query *hq)
   {
      int idx = pidx(hq->provider->query_type);
      }
      static void
   resume_query(struct fd_batch *batch, struct fd_hw_query *hq,
         {
      int idx = pidx(hq->provider->query_type);
   DBG("%p", hq);
   assert(idx >= 0); /* query never would have been created otherwise */
   assert(!hq->period);
   batch->query_providers_used |= (1 << idx);
   batch->query_providers_active |= (1 << idx);
   hq->period = slab_alloc_st(&batch->ctx->sample_period_pool);
   list_inithead(&hq->period->list);
   hq->period->start = get_sample(batch, ring, hq->base.type);
   /* NOTE: slab_alloc_st() does not zero out the buffer: */
      }
      static void
   pause_query(struct fd_batch *batch, struct fd_hw_query *hq,
         {
      ASSERTED int idx = pidx(hq->provider->query_type);
   DBG("%p", hq);
   assert(idx >= 0); /* query never would have been created otherwise */
   assert(hq->period && !hq->period->end);
   assert(query_active_in_batch(batch, hq));
   batch->query_providers_active &= ~(1 << idx);
   hq->period->end = get_sample(batch, ring, hq->base.type);
   list_addtail(&hq->period->list, &hq->periods);
      }
      static void
   destroy_periods(struct fd_context *ctx, struct fd_hw_query *hq)
   {
      struct fd_hw_sample_period *period, *s;
   LIST_FOR_EACH_ENTRY_SAFE (period, s, &hq->periods, list) {
      fd_hw_sample_reference(ctx, &period->start, NULL);
   fd_hw_sample_reference(ctx, &period->end, NULL);
   list_del(&period->list);
         }
      static void
   fd_hw_destroy_query(struct fd_context *ctx, struct fd_query *q)
   {
                        destroy_periods(ctx, hq);
               }
      static void
   fd_hw_begin_query(struct fd_context *ctx, struct fd_query *q) assert_dt
   {
      struct fd_batch *batch = fd_context_batch(ctx);
                     /* begin_query() should clear previous results: */
            if (batch && (ctx->active_queries || hq->provider->always))
            /* add to active list: */
   assert(list_is_empty(&hq->list));
               }
      static void
   fd_hw_end_query(struct fd_context *ctx, struct fd_query *q) assert_dt
   {
      struct fd_batch *batch = fd_context_batch(ctx);
                     if (batch && (ctx->active_queries || hq->provider->always))
            /* remove from active list: */
               }
      /* helper to get ptr to specified sample: */
   static void *
   sampptr(struct fd_hw_sample *samp, uint32_t n, void *ptr)
   {
         }
      static bool
   fd_hw_get_query_result(struct fd_context *ctx, struct fd_query *q, bool wait,
         {
      struct fd_hw_query *hq = fd_hw_query(q);
   const struct fd_hw_sample_provider *p = hq->provider;
                     if (list_is_empty(&hq->periods))
            assert(list_is_empty(&hq->list));
            /* sum the result across all sample periods.  Start with the last period
   * so that no-wait will bail quickly.
   */
   LIST_FOR_EACH_ENTRY_SAFE_REV (period, tmp, &hq->periods, list) {
      struct fd_hw_sample *start = period->start;
   ASSERTED struct fd_hw_sample *end = period->end;
            /* start and end samples should be from same batch: */
   assert(start->prsc == end->prsc);
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
               /* some piglit tests at least do query with no draws, I guess: */
   if (!rsc->bo)
            if (!wait) {
      int ret = fd_resource_wait(
         if (ret)
      } else {
                           for (i = 0; i < start->num_tiles; i++) {
      p->accumulate_result(ctx, sampptr(period->start, i, ptr),
                     }
      static const struct fd_query_funcs hw_query_funcs = {
      .destroy_query = fd_hw_destroy_query,
   .begin_query = fd_hw_begin_query,
   .end_query = fd_hw_end_query,
      };
      struct fd_query *
   fd_hw_create_query(struct fd_context *ctx, unsigned query_type, unsigned index)
   {
      struct fd_hw_query *hq;
   struct fd_query *q;
            if ((idx < 0) || !ctx->hw_sample_providers[idx])
            hq = CALLOC_STRUCT(fd_hw_query);
   if (!hq)
                              list_inithead(&hq->periods);
            q = &hq->base;
   q->funcs = &hw_query_funcs;
   q->type = query_type;
               }
      struct fd_hw_sample *
   fd_hw_sample_init(struct fd_batch *batch, uint32_t size)
   {
      struct fd_hw_sample *samp = slab_alloc_st(&batch->ctx->sample_pool);
   pipe_reference_init(&samp->reference, 1);
   samp->size = size;
   assert(util_is_power_of_two_or_zero(size));
   batch->next_sample_offset = align(batch->next_sample_offset, size);
   samp->offset = batch->next_sample_offset;
   /* NOTE: slab_alloc_st() does not zero out the buffer: */
   samp->prsc = NULL;
   samp->num_tiles = 0;
   samp->tile_stride = 0;
                        }
      void
   __fd_hw_sample_destroy(struct fd_context *ctx, struct fd_hw_sample *samp)
   {
      pipe_resource_reference(&samp->prsc, NULL);
      }
      /* called from gmem code once total storage requirements are known (ie.
   * number of samples times number of tiles)
   */
   void
   fd_hw_query_prepare(struct fd_batch *batch, uint32_t num_tiles)
   {
               if (tile_stride > 0)
                     while (batch->samples.size > 0) {
      struct fd_hw_sample *samp =
         samp->num_tiles = num_tiles;
   samp->tile_stride = tile_stride;
               /* reset things for next batch: */
      }
      void
   fd_hw_query_prepare_tile(struct fd_batch *batch, uint32_t n,
         {
      uint32_t tile_stride = batch->query_tile_stride;
            /* bail if no queries: */
   if (tile_stride == 0)
            fd_wfi(batch, ring);
   OUT_PKT0(ring, HW_QUERY_BASE_REG, 1);
      }
      void
   fd_hw_query_update_batch(struct fd_batch *batch, bool disable_all)
   {
               if (disable_all || (ctx->dirty & FD_DIRTY_QUERY)) {
      struct fd_hw_query *hq;
   LIST_FOR_EACH_ENTRY (hq, &batch->ctx->hw_active_queries, list) {
      bool was_active = query_active_in_batch(batch, hq);
                  if (now_active && !was_active)
         else if (was_active && !now_active)
         }
      }
      /* call the provider->enable() for all the hw queries that were active
   * in the current batch.  This sets up perfctr selector regs statically
   * for the duration of the batch.
   */
   void
   fd_hw_query_enable(struct fd_batch *batch, struct fd_ringbuffer *ring)
   {
      struct fd_context *ctx = batch->ctx;
   for (int idx = 0; idx < MAX_HW_SAMPLE_PROVIDERS; idx++) {
      if (batch->query_providers_used & (1 << idx)) {
      assert(ctx->hw_sample_providers[idx]);
   if (ctx->hw_sample_providers[idx]->enable)
            }
      void
   fd_hw_query_register_provider(struct pipe_context *pctx,
         {
      struct fd_context *ctx = fd_context(pctx);
            assert((0 <= idx) && (idx < MAX_HW_SAMPLE_PROVIDERS));
               }
      void
   fd_hw_query_init(struct pipe_context *pctx)
   {
               slab_create(&ctx->sample_pool, sizeof(struct fd_hw_sample), 16);
   slab_create(&ctx->sample_period_pool, sizeof(struct fd_hw_sample_period),
      }
      void
   fd_hw_query_fini(struct pipe_context *pctx)
   {
               slab_destroy(&ctx->sample_pool);
      }
