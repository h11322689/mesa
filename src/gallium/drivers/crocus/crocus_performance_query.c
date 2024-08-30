   /*
   * Copyright © 2019 Intel Corporation
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
      #include <xf86drm.h>
      #include "crocus_context.h"
   #include "crocus_perf.h"
      #include "main/mtypes.h"
      struct crocus_perf_query {
      struct gl_perf_query_object base;
   struct intel_perf_query_object *query;
      };
      static unsigned
   crocus_init_perf_query_info(struct pipe_context *pipe)
   {
      struct crocus_context *ice = (void *) pipe;
   struct crocus_screen *screen = (struct crocus_screen *) ice->ctx.screen;
            /* make sure pipe perf counter type/data-type enums are matched with intel_perf's */
   STATIC_ASSERT(PIPE_PERF_COUNTER_TYPE_EVENT == (enum pipe_perf_counter_type)INTEL_PERF_COUNTER_TYPE_EVENT);
   STATIC_ASSERT(PIPE_PERF_COUNTER_TYPE_DURATION_NORM == (enum pipe_perf_counter_type)INTEL_PERF_COUNTER_TYPE_DURATION_NORM);
   STATIC_ASSERT(PIPE_PERF_COUNTER_TYPE_DURATION_RAW == (enum pipe_perf_counter_type)INTEL_PERF_COUNTER_TYPE_DURATION_RAW);
   STATIC_ASSERT(PIPE_PERF_COUNTER_TYPE_THROUGHPUT == (enum pipe_perf_counter_type)INTEL_PERF_COUNTER_TYPE_THROUGHPUT);
            STATIC_ASSERT(PIPE_PERF_COUNTER_DATA_TYPE_BOOL32 == (enum pipe_perf_counter_data_type)INTEL_PERF_COUNTER_DATA_TYPE_BOOL32);
   STATIC_ASSERT(PIPE_PERF_COUNTER_DATA_TYPE_UINT32 == (enum pipe_perf_counter_data_type)INTEL_PERF_COUNTER_DATA_TYPE_UINT32);
   STATIC_ASSERT(PIPE_PERF_COUNTER_DATA_TYPE_UINT64 == (enum pipe_perf_counter_data_type)INTEL_PERF_COUNTER_DATA_TYPE_UINT64);
   STATIC_ASSERT(PIPE_PERF_COUNTER_DATA_TYPE_FLOAT == (enum pipe_perf_counter_data_type)INTEL_PERF_COUNTER_DATA_TYPE_FLOAT);
            if (!ice->perf_ctx)
            if (unlikely(!ice->perf_ctx))
                     if (perf_cfg)
                              intel_perf_init_metrics(perf_cfg, &screen->devinfo, screen->fd,
                  intel_perf_init_context(ice->perf_ctx,
                        perf_cfg,
   ice,
   ice,
            }
      static struct pipe_query *
   crocus_new_perf_query_obj(struct pipe_context *pipe, unsigned query_index)
   {
      struct crocus_context *ice = (void *) pipe;
   struct intel_perf_context *perf_ctx = ice->perf_ctx;
   struct intel_perf_query_object * obj =
         if (unlikely(!obj))
            struct crocus_perf_query *q = calloc(1, sizeof(struct crocus_perf_query));
   if (unlikely(!q)) {
      intel_perf_delete_query(perf_ctx, obj);
               q->query = obj;
      }
      static bool
   crocus_begin_perf_query(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct crocus_context *ice = (void *) pipe;
   struct crocus_perf_query *perf_query= (struct crocus_perf_query *) q;
   struct intel_perf_query_object *obj = perf_query->query;
               }
      static void
   crocus_end_perf_query(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct crocus_context *ice = (void *) pipe;
   struct crocus_perf_query *perf_query = (struct crocus_perf_query *) q;
   struct intel_perf_query_object *obj = perf_query->query;
            if (perf_query->begin_succeeded)
      }
      static void
   crocus_delete_perf_query(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct crocus_context *ice = (void *) pipe;
   struct crocus_perf_query *perf_query = (struct crocus_perf_query *) q;
   struct intel_perf_query_object *obj = perf_query->query;
            intel_perf_delete_query(perf_ctx, obj);
      }
      static void
   crocus_get_perf_query_info(struct pipe_context *pipe,
                           unsigned query_index,
   {
      struct crocus_context *ice = (void *) pipe;
   struct intel_perf_context *perf_ctx = ice->perf_ctx;
   struct intel_perf_config *perf_cfg = intel_perf_config(perf_ctx);
            *name = info->name;
   *data_size = info->data_size;
   *n_counters = info->n_counters;
      }
      static void
   crocus_get_perf_counter_info(struct pipe_context *pipe,
                              unsigned query_index,
   unsigned counter_index,
   const char **name,
   const char **desc,
      {
      struct crocus_context *ice = (void *) pipe;
   struct intel_perf_context *perf_ctx = ice->perf_ctx;
   struct intel_perf_config *perf_cfg = intel_perf_config(perf_ctx);
   const struct intel_perf_query_info *info = &perf_cfg->queries[query_index];
   const struct intel_perf_query_counter *counter =
                           *name = counter->name;
   *desc = counter->desc;
   *offset = counter->offset;
   *data_size = intel_perf_query_counter_get_size(counter);
   *type_enum = counter->type;
            if (counter->oa_counter_max_uint64) {
      if (counter->data_type == INTEL_PERF_COUNTER_DATA_TYPE_FLOAT ||
      counter->data_type == INTEL_PERF_COUNTER_DATA_TYPE_DOUBLE)
      else
      } else {
            }
      static void
   crocus_wait_perf_query(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct crocus_context *ice = (void *) pipe;
   struct crocus_perf_query *perf_query = (struct crocus_perf_query *) q;
   struct intel_perf_query_object *obj = perf_query->query;
            if (perf_query->begin_succeeded)
      }
      static bool
   crocus_is_perf_query_ready(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct crocus_context *ice = (void *) pipe;
   struct crocus_perf_query *perf_query = (struct crocus_perf_query *) q;
   struct intel_perf_query_object *obj = perf_query->query;
            if (perf_query->base.Ready)
         if (!perf_query->begin_succeeded)
            return intel_perf_is_query_ready(perf_ctx, obj,
      }
      static bool
   crocus_get_perf_query_data(struct pipe_context *pipe,
                           {
      struct crocus_context *ice = (void *) pipe;
   struct crocus_perf_query *perf_query = (struct crocus_perf_query *) q;
   struct intel_perf_query_object *obj = perf_query->query;
            if (perf_query->begin_succeeded) {
      intel_perf_get_query_data(perf_ctx, obj, &ice->batches[CROCUS_BATCH_RENDER],
                  }
      void
   crocus_init_perfquery_functions(struct pipe_context *ctx)
   {
      ctx->init_intel_perf_query_info = crocus_init_perf_query_info;
   ctx->get_intel_perf_query_info = crocus_get_perf_query_info;
   ctx->get_intel_perf_query_counter_info = crocus_get_perf_counter_info;
   ctx->new_intel_perf_query_obj = crocus_new_perf_query_obj;
   ctx->begin_intel_perf_query = crocus_begin_perf_query;
   ctx->end_intel_perf_query = crocus_end_perf_query;
   ctx->delete_intel_perf_query = crocus_delete_perf_query;
   ctx->wait_intel_perf_query = crocus_wait_perf_query;
   ctx->is_intel_perf_query_ready = crocus_is_perf_query_ready;
      }
