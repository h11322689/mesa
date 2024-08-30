   /*
   * Copyright 2014, 2015 Red Hat.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "virgl_context.h"
   #include "virgl_encode.h"
   #include "virtio-gpu/virgl_protocol.h"
   #include "virgl_resource.h"
   #include "virgl_screen.h"
      struct virgl_query {
      struct virgl_resource *buf;
   uint32_t handle;
   uint32_t result_size;
            bool ready;
      };
      #define VIRGL_QUERY_OCCLUSION_COUNTER     0
   #define VIRGL_QUERY_OCCLUSION_PREDICATE   1
   #define VIRGL_QUERY_TIMESTAMP             2
   #define VIRGL_QUERY_TIMESTAMP_DISJOINT    3
   #define VIRGL_QUERY_TIME_ELAPSED          4
   #define VIRGL_QUERY_PRIMITIVES_GENERATED  5
   #define VIRGL_QUERY_PRIMITIVES_EMITTED    6
   #define VIRGL_QUERY_SO_STATISTICS         7
   #define VIRGL_QUERY_SO_OVERFLOW_PREDICATE 8
   #define VIRGL_QUERY_GPU_FINISHED          9
   #define VIRGL_QUERY_PIPELINE_STATISTICS  10
   #define VIRGL_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE 11
   #define VIRGL_QUERY_SO_OVERFLOW_ANY_PREDICATE 12
      static const int pquery_map[] =
   {
      VIRGL_QUERY_OCCLUSION_COUNTER,
   VIRGL_QUERY_OCCLUSION_PREDICATE,
   VIRGL_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE,
   VIRGL_QUERY_TIMESTAMP,
   VIRGL_QUERY_TIMESTAMP_DISJOINT,
   VIRGL_QUERY_TIME_ELAPSED,
   VIRGL_QUERY_PRIMITIVES_GENERATED,
   VIRGL_QUERY_PRIMITIVES_EMITTED,
   VIRGL_QUERY_SO_STATISTICS,
   VIRGL_QUERY_SO_OVERFLOW_PREDICATE,
   VIRGL_QUERY_SO_OVERFLOW_ANY_PREDICATE,
   VIRGL_QUERY_GPU_FINISHED,
      };
      static int pipe_to_virgl_query(enum pipe_query_type ptype)
   {
         }
      static const enum virgl_statistics_query_index stats_index_map[] = {
      [PIPE_STAT_QUERY_IA_VERTICES] = VIRGL_STAT_QUERY_IA_VERTICES,
   [PIPE_STAT_QUERY_IA_PRIMITIVES] = VIRGL_STAT_QUERY_IA_PRIMITIVES,
   [PIPE_STAT_QUERY_VS_INVOCATIONS] = VIRGL_STAT_QUERY_VS_INVOCATIONS,
   [PIPE_STAT_QUERY_GS_INVOCATIONS] = VIRGL_STAT_QUERY_GS_INVOCATIONS,
   [PIPE_STAT_QUERY_GS_PRIMITIVES] = VIRGL_STAT_QUERY_GS_PRIMITIVES,
   [PIPE_STAT_QUERY_C_INVOCATIONS] = VIRGL_STAT_QUERY_C_INVOCATIONS,
   [PIPE_STAT_QUERY_C_PRIMITIVES] = VIRGL_STAT_QUERY_C_PRIMITIVES,
   [PIPE_STAT_QUERY_PS_INVOCATIONS] = VIRGL_STAT_QUERY_PS_INVOCATIONS,
   [PIPE_STAT_QUERY_HS_INVOCATIONS] = VIRGL_STAT_QUERY_HS_INVOCATIONS,
   [PIPE_STAT_QUERY_DS_INVOCATIONS] = VIRGL_STAT_QUERY_DS_INVOCATIONS,
      };
      static enum virgl_statistics_query_index
   pipe_stats_query_to_virgl(enum pipe_statistics_query_index index)
   {
         }
      static inline struct virgl_query *virgl_query(struct pipe_query *q)
   {
         }
      static void virgl_render_condition(struct pipe_context *ctx,
                     {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_query *query = virgl_query(q);
   uint32_t handle = 0;
   if (q)
            }
      static struct pipe_query *virgl_create_query(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
            query = CALLOC_STRUCT(virgl_query);
   if (!query)
            query->buf = (struct virgl_resource *)
      pipe_buffer_create(ctx->screen, PIPE_BIND_CUSTOM, PIPE_USAGE_STAGING,
      if (!query->buf) {
      FREE(query);
               query->handle = virgl_object_assign_handle();
   query->result_size = (query_type == PIPE_QUERY_TIMESTAMP ||
            if (query_type == PIPE_QUERY_PIPELINE_STATISTICS) {
                  } else {
                  util_range_add(&query->buf->b, &query->buf->valid_buffer_range, 0,
                  virgl_encoder_create_query(vctx, query->handle,
               }
      static void virgl_destroy_query(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
                     pipe_resource_reference((struct pipe_resource **)&query->buf, NULL);
      }
      static bool virgl_begin_query(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
                        }
      static bool virgl_end_query(struct pipe_context *ctx,
         {
      struct virgl_screen *vs = virgl_screen(ctx->screen);
   struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_query *query = virgl_query(q);
            host_state = vs->vws->resource_map(vs->vws, query->buf->hw_res);
   if (!host_state)
            host_state->query_state = VIRGL_QUERY_STATE_WAIT_HOST;
                     /* start polling now */
   virgl_encoder_get_query_result(vctx, query->handle, 0);
               }
      static bool virgl_get_query_result(struct pipe_context *ctx,
                     {
               if (!query->ready) {
      struct virgl_screen *vs = virgl_screen(ctx->screen);
   struct virgl_context *vctx = virgl_context(ctx);
   volatile struct virgl_host_query_state *host_state;
            if (vs->vws->res_is_referenced(vs->vws, vctx->cbuf, query->buf->hw_res))
            if (wait)
         else if (vs->vws->resource_is_busy(vs->vws, query->buf->hw_res))
                     /* The resource is idle and the result should be available at this point,
   * unless we are dealing with an older host.  In that case,
   * VIRGL_CCMD_GET_QUERY_RESULT is not fenced, the buffer is not
   * coherent, and transfers are unsynchronized.  We have to repeatedly
   * transfer until we get the result back.
   */
   while (host_state->query_state != VIRGL_QUERY_STATE_DONE) {
               if (transfer) {
      pipe_buffer_unmap(ctx, transfer);
   if (!wait)
               host_state = pipe_buffer_map(ctx, &query->buf->b,
               if (query->result_size == 8)
         else
            if (transfer)
                        switch (query->pipeline_stats) {
   case PIPE_STAT_QUERY_IA_VERTICES: result->pipeline_statistics.ia_vertices = query->result; break;
   case PIPE_STAT_QUERY_IA_PRIMITIVES: result->pipeline_statistics.ia_primitives = query->result; break;
   case PIPE_STAT_QUERY_VS_INVOCATIONS: result->pipeline_statistics.vs_invocations = query->result; break;
   case PIPE_STAT_QUERY_GS_INVOCATIONS: result->pipeline_statistics.gs_invocations = query->result; break;
   case PIPE_STAT_QUERY_GS_PRIMITIVES: result->pipeline_statistics.gs_primitives = query->result; break;
   case PIPE_STAT_QUERY_PS_INVOCATIONS: result->pipeline_statistics.ps_invocations = query->result; break;
   case PIPE_STAT_QUERY_HS_INVOCATIONS: result->pipeline_statistics.hs_invocations = query->result; break;
   case PIPE_STAT_QUERY_CS_INVOCATIONS: result->pipeline_statistics.cs_invocations = query->result; break;
   case PIPE_STAT_QUERY_C_INVOCATIONS: result->pipeline_statistics.c_invocations = query->result; break;
   case PIPE_STAT_QUERY_C_PRIMITIVES: result->pipeline_statistics.c_primitives = query->result; break;
   case PIPE_STAT_QUERY_DS_INVOCATIONS: result->pipeline_statistics.ds_invocations = query->result; break;
   default:
                     }
      static void
   virgl_set_active_query_state(struct pipe_context *pipe, bool enable)
   {
   }
      static void
   virgl_get_query_result_resource(struct pipe_context *ctx,
                                 struct pipe_query *q,
   {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_query *query = virgl_query(q);
            virgl_resource_dirty(qbo, 0);
      }
      void virgl_init_query_functions(struct virgl_context *vctx)
   {
      vctx->base.render_condition = virgl_render_condition;
   vctx->base.create_query = virgl_create_query;
   vctx->base.destroy_query = virgl_destroy_query;
   vctx->base.begin_query = virgl_begin_query;
   vctx->base.end_query = virgl_end_query;
   vctx->base.get_query_result = virgl_get_query_result;
   vctx->base.set_active_query_state = virgl_set_active_query_state;
      }
