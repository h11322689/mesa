   /*
   * Copyright (c) 2017 Etnaviv Project
   * Copyright (C) 2017 Zodiac Inflight Innovations
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   *    Christian Gmeiner <christian.gmeiner@gmail.com>
   */
      #include "util/u_inlines.h"
   #include "util/u_memory.h"
      #include "etnaviv_context.h"
   #include "etnaviv_debug.h"
   #include "etnaviv_emit.h"
   #include "etnaviv_query_acc.h"
   #include "etnaviv_screen.h"
         extern const struct etna_acc_sample_provider occlusion_provider;
   extern const struct etna_acc_sample_provider perfmon_provider;
      static const struct etna_acc_sample_provider *acc_sample_provider[] =
   {
      &occlusion_provider,
      };
      static void
   etna_acc_destroy_query(struct etna_context *ctx, struct etna_query *q)
   {
               pipe_resource_reference(&aq->prsc, NULL);
               }
      static void
   realloc_query_bo(struct etna_context *ctx, struct etna_acc_query *aq)
   {
      struct etna_resource *rsc;
                     aq->prsc = pipe_buffer_create(&ctx->screen->base, PIPE_BIND_QUERY_BUFFER,
            /* don't assume the buffer is zero-initialized */
                     map = etna_bo_map(rsc->bo);
   memset(map, 0, 0x1000);
      }
      static void
   etna_acc_begin_query(struct etna_context *ctx, struct etna_query *q)
   {
      struct etna_acc_query *aq = etna_acc_query(q);
            /* ->begin_query() discards previous results, so realloc bo */
   realloc_query_bo(ctx, aq);
                     /* add to active list */
   assert(list_is_empty(&aq->node));
      }
      static void
   etna_acc_end_query(struct etna_context *ctx, struct etna_query *q)
   {
      struct etna_acc_query *aq = etna_acc_query(q);
                     /* remove from active list */
      }
      static bool
   etna_acc_get_query_result(struct etna_context *ctx, struct etna_query *q,
         {
      struct etna_acc_query *aq = etna_acc_query(q);
   struct etna_resource *rsc = etna_resource(aq->prsc);
                     if (etna_resource_status(ctx, rsc) & ETNA_PENDING_WRITE) {
      if (!wait) {
      /* piglit spec@arb_occlusion_query@occlusion_query_conform
   * test, and silly apps perhaps, get stuck in a loop trying
   * to get query result forever with wait==false..  we don't
   * wait to flush unnecessarily but we also don't want to
   * spin forever.
   */
   if (aq->no_wait_cnt++ > 5) {
      etna_flush(&ctx->base, NULL, 0, true);
                  } else {
      /* flush that GPU executes all query related actions */
                  /* get the result */
   int ret = etna_bo_cpu_prep(rsc->bo, DRM_ETNA_PREP_READ);
   if (ret)
            void *ptr = etna_bo_map(rsc->bo);
                        }
      static const struct etna_query_funcs acc_query_funcs = {
      .destroy_query = etna_acc_destroy_query,
   .begin_query = etna_acc_begin_query,
   .end_query = etna_acc_end_query,
      };
      struct etna_query *
   etna_acc_create_query(struct etna_context *ctx, unsigned query_type)
   {
      const struct etna_acc_sample_provider *p = NULL;
   struct etna_acc_query *aq;
            /* find a sample provide for the requested query type */
   for (unsigned i = 0; i < ARRAY_SIZE(acc_sample_provider); i++) {
               if (p->supports(query_type))
         else
               if (!p)
            aq = p->allocate(ctx, query_type);
   if (!aq)
                              q = &aq->base;
   q->funcs = &acc_query_funcs;
               }
