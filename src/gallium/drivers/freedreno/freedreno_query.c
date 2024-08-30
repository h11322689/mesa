   /*
   * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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
   #include "util/u_memory.h"
      #include "freedreno_context.h"
   #include "freedreno_query.h"
   #include "freedreno_query_hw.h"
   #include "freedreno_query_sw.h"
   #include "freedreno_resource.h"
   #include "freedreno_util.h"
      /*
   * Pipe Query interface:
   */
      static struct pipe_query *
   fd_create_query(struct pipe_context *pctx, unsigned query_type, unsigned index)
   {
      struct fd_context *ctx = fd_context(pctx);
            if (ctx->create_query)
         if (!q)
               }
      static void
   fd_destroy_query(struct pipe_context *pctx, struct pipe_query *pq) in_dt
   {
      struct fd_query *q = fd_query(pq);
      }
      static bool
   fd_begin_query(struct pipe_context *pctx, struct pipe_query *pq) in_dt
   {
                           }
      static bool
   fd_end_query(struct pipe_context *pctx, struct pipe_query *pq) in_dt
   {
               /* there are a couple special cases, which don't have
   * a matching ->begin_query():
   */
   if (skip_begin_query(q->type))
                        }
      static bool
   fd_get_query_result(struct pipe_context *pctx, struct pipe_query *pq, bool wait,
         {
                           }
      static void
   fd_get_query_result_resource(struct pipe_context *pctx, struct pipe_query *pq,
                                 {
               q->funcs->get_query_result_resource(fd_context(pctx), q, flags, result_type,
      }
      static void
   fd_render_condition(struct pipe_context *pctx, struct pipe_query *pq,
         {
      struct fd_context *ctx = fd_context(pctx);
   ctx->cond_query = pq;
   ctx->cond_cond = condition;
      }
      #define _Q(_name, _query_type, _type, _result_type) {                          \
         .name = _name, .query_type = _query_type,                                \
   .type = PIPE_DRIVER_QUERY_TYPE_##_type,                                  \
   .result_type = PIPE_DRIVER_QUERY_RESULT_TYPE_##_result_type,             \
            #define FQ(_name, _query_type, _type, _result_type)                            \
            #define PQ(_name, _query_type, _type, _result_type)                            \
            static const struct pipe_driver_query_info sw_query_list[] = {
      FQ("draw-calls", DRAW_CALLS, UINT64, AVERAGE),
   FQ("batches", BATCH_TOTAL, UINT64, AVERAGE),
   FQ("batches-sysmem", BATCH_SYSMEM, UINT64, AVERAGE),
   FQ("batches-gmem", BATCH_GMEM, UINT64, AVERAGE),
   FQ("batches-nondraw", BATCH_NONDRAW, UINT64, AVERAGE),
   FQ("restores", BATCH_RESTORE, UINT64, AVERAGE),
   PQ("prims-emitted", PRIMITIVES_EMITTED, UINT64, AVERAGE),
   FQ("staging", STAGING_UPLOADS, UINT64, AVERAGE),
   FQ("shadow", SHADOW_UPLOADS, UINT64, AVERAGE),
   FQ("vsregs", VS_REGS, FLOAT, AVERAGE),
      };
      static int
   fd_get_driver_query_info(struct pipe_screen *pscreen, unsigned index,
         {
               if (!info)
            if (index >= ARRAY_SIZE(sw_query_list)) {
      index -= ARRAY_SIZE(sw_query_list);
   if (index >= screen->num_perfcntr_queries)
         *info = screen->perfcntr_queries[index];
               *info = sw_query_list[index];
      }
      static int
   fd_get_driver_query_group_info(struct pipe_screen *pscreen, unsigned index,
         {
               if (!info)
            if (index >= screen->num_perfcntr_groups)
                     info->name = g->name;
   info->max_active_queries = g->num_counters;
               }
      static void
   fd_set_active_query_state(struct pipe_context *pctx, bool enable) assert_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->active_queries = enable;
      }
      static enum pipe_driver_query_type
   query_type(enum fd_perfcntr_type type)
   {
   #define ENUM(t)                                                                \
      case FD_PERFCNTR_##t:                                                       \
         switch (type) {
      ENUM(TYPE_UINT64);
   ENUM(TYPE_UINT);
   ENUM(TYPE_FLOAT);
   ENUM(TYPE_PERCENTAGE);
   ENUM(TYPE_BYTES);
   ENUM(TYPE_MICROSECONDS);
   ENUM(TYPE_HZ);
   ENUM(TYPE_DBM);
   ENUM(TYPE_TEMPERATURE);
   ENUM(TYPE_VOLTS);
   ENUM(TYPE_AMPS);
      default:
      unreachable("bad type");
         }
      static enum pipe_driver_query_result_type
   query_result_type(enum fd_perfcntr_result_type type)
   {
      switch (type) {
      ENUM(RESULT_TYPE_AVERAGE);
      default:
      unreachable("bad type");
         }
      static void
   setup_perfcntr_query_info(struct fd_screen *screen)
   {
               for (unsigned i = 0; i < screen->num_perfcntr_groups; i++)
            screen->perfcntr_queries =
                  unsigned idx = 0;
   for (unsigned i = 0; i < screen->num_perfcntr_groups; i++) {
      const struct fd_perfcntr_group *g = &screen->perfcntr_groups[i];
   for (unsigned j = 0; j < g->num_countables; j++) {
                     info->name = c->name;
   info->query_type = FD_QUERY_FIRST_PERFCNTR + idx;
   info->type = query_type(c->query_type);
   info->result_type = query_result_type(c->result_type);
                           }
      void
   fd_query_screen_init(struct pipe_screen *pscreen)
   {
      pscreen->get_driver_query_info = fd_get_driver_query_info;
   pscreen->get_driver_query_group_info = fd_get_driver_query_group_info;
      }
      void
   fd_query_context_init(struct pipe_context *pctx)
   {
      pctx->create_query = fd_create_query;
   pctx->destroy_query = fd_destroy_query;
   pctx->begin_query = fd_begin_query;
   pctx->end_query = fd_end_query;
   pctx->get_query_result = fd_get_query_result;
   pctx->get_query_result_resource  = fd_get_query_result_resource;
   pctx->set_active_query_state = fd_set_active_query_state;
      }
