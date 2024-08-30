   /*
   * Copyright (c) 2016 Etnaviv Project
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
      #include "pipe/p_screen.h"
   #include "util/u_inlines.h"
      #include "etnaviv_context.h"
   #include "etnaviv_perfmon.h"
   #include "etnaviv_query.h"
   #include "etnaviv_query_acc.h"
   #include "etnaviv_query_sw.h"
      static struct pipe_query *
   etna_create_query(struct pipe_context *pctx, unsigned query_type,
         {
      struct etna_context *ctx = etna_context(pctx);
            q = etna_sw_create_query(ctx, query_type);
   if (!q)
               }
      static void
   etna_destroy_query(struct pipe_context *pctx, struct pipe_query *pq)
   {
                  }
      static bool
   etna_begin_query(struct pipe_context *pctx, struct pipe_query *pq)
   {
                           }
      static bool
   etna_end_query(struct pipe_context *pctx, struct pipe_query *pq)
   {
                           }
      static bool
   etna_get_query_result(struct pipe_context *pctx, struct pipe_query *pq,
         {
                           }
      static int
   etna_get_driver_query_info(struct pipe_screen *pscreen, unsigned index,
         {
      int nr_sw_queries = etna_sw_get_driver_query_info(pscreen, 0, NULL);
            if (!info)
            if (index < nr_sw_queries)
               }
      static int
   etna_get_driver_query_group_info(struct pipe_screen *pscreen, unsigned index,
         {
      int nr_sw_groups = etna_sw_get_driver_query_group_info(pscreen, 0, NULL);
            if (!info)
            if (index < nr_sw_groups)
               }
      static void
   etna_render_condition(struct pipe_context *pctx, struct pipe_query *pq,
         {
               ctx->cond_query = pq;
   ctx->cond_cond = condition;
      }
      static void
   etna_set_active_query_state(struct pipe_context *pctx, bool enable)
   {
               if (enable) {
      list_for_each_entry(struct etna_acc_query, aq, &ctx->active_acc_queries, node)
      } else {
      list_for_each_entry(struct etna_acc_query, aq, &ctx->active_acc_queries, node)
         }
      void
   etna_query_screen_init(struct pipe_screen *pscreen)
   {
      pscreen->get_driver_query_info = etna_get_driver_query_info;
      }
      void
   etna_query_context_init(struct pipe_context *pctx)
   {
      pctx->create_query = etna_create_query;
   pctx->destroy_query = etna_destroy_query;
   pctx->begin_query = etna_begin_query;
   pctx->end_query = etna_end_query;
   pctx->get_query_result = etna_get_query_result;
   pctx->set_active_query_state = etna_set_active_query_state;
      }
