   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "d3d12_query.h"
   #include "d3d12_compiler.h"
   #include "d3d12_context.h"
   #include "d3d12_resource.h"
   #include "d3d12_screen.h"
   #include "d3d12_fence.h"
      #include "util/u_dump.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_threaded_context.h"
      #include <dxguids/dxguids.h>
      static unsigned
   num_sub_queries(unsigned query_type)
   {
      switch (query_type) {
   case PIPE_QUERY_PRIMITIVES_GENERATED:
         default:
            }
      static D3D12_QUERY_HEAP_TYPE
   d3d12_query_heap_type(unsigned query_type, unsigned sub_query)
   {
      switch (query_type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
         case PIPE_QUERY_PIPELINE_STATISTICS:
         case PIPE_QUERY_PRIMITIVES_GENERATED:
      return sub_query == 0 ?
      D3D12_QUERY_HEAP_TYPE_SO_STATISTICS :
   case PIPE_QUERY_PRIMITIVES_EMITTED:
   case PIPE_QUERY_SO_STATISTICS:
         case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIME_ELAPSED:
            default:
      debug_printf("unknown query: %s\n",
               }
      static D3D12_QUERY_TYPE
   d3d12_query_type(unsigned query_type, unsigned sub_query, unsigned index)
   {
      switch (query_type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
         case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
         case PIPE_QUERY_PIPELINE_STATISTICS:
         case PIPE_QUERY_PRIMITIVES_GENERATED:
      return sub_query == 0 ?
      D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0 :
   case PIPE_QUERY_PRIMITIVES_EMITTED:
   case PIPE_QUERY_SO_STATISTICS:
         case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIME_ELAPSED:
         default:
      debug_printf("unknown query: %s\n",
               }
      static struct pipe_query *
   d3d12_create_query(struct pipe_context *pctx,
         {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_screen *screen = d3d12_screen(pctx->screen);
   struct d3d12_query *query = CALLOC_STRUCT(d3d12_query);
            if (!query)
            query->type = (pipe_query_type)query_type;
   for (unsigned i = 0; i < num_sub_queries(query_type); ++i) {
      assert(i < MAX_SUBQUERIES);
   query->subqueries[i].d3d12qtype = d3d12_query_type(query_type, i, index);
            /* With timer queries we want a few more queries, especially since we need two slots
   * per query for TIME_ELAPSED queries
   * For TIMESTAMP, we don't need more than one slot, since there's nothing to accumulate */
   if (unlikely(query_type == PIPE_QUERY_TIME_ELAPSED))
         else if (query_type == PIPE_QUERY_TIMESTAMP)
            query->subqueries[i].curr_query = 0;
   desc.Count = query->subqueries[i].num_queries;
            switch (desc.Type) {
   case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS:
      query->subqueries[i].query_size = sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
      case D3D12_QUERY_HEAP_TYPE_SO_STATISTICS:
      query->subqueries[i].query_size = sizeof(D3D12_QUERY_DATA_SO_STATISTICS);
      default:
      query->subqueries[i].query_size = sizeof(uint64_t);
      }
   if (FAILED(screen->dev->CreateQueryHeap(&desc,
            FREE(query);
               /* Query result goes into a readback buffer */
   size_t buffer_size = query->subqueries[i].query_size * query->subqueries[i].num_queries;
   u_suballocator_alloc(&ctx->query_allocator, buffer_size, 256,
                           }
      static void
   d3d12_destroy_query(struct pipe_context *pctx,
         {
      struct d3d12_query *query = (struct d3d12_query *)q;
   pipe_resource *predicate = &query->predicate->base.b;
   pipe_resource_reference(&predicate, NULL);
   for (unsigned i = 0; i < num_sub_queries(query->type); ++i) {
      query->subqueries[i].query_heap->Release();
      }
      }
      static bool
   accumulate_subresult(struct d3d12_context *ctx, struct d3d12_query *q_parent,
               {
      struct pipe_transfer *transfer = NULL;
   struct d3d12_screen *screen = d3d12_screen(ctx->base.screen);
   struct d3d12_query_impl *q = &q_parent->subqueries[sub_query];
   unsigned access = PIPE_MAP_READ;
            if (write)
                  results = pipe_buffer_map_range(&ctx->base, q->buffer, q->buffer_offset,
                  if (results == NULL)
            uint64_t *results_u64 = (uint64_t *)results;
   D3D12_QUERY_DATA_PIPELINE_STATISTICS *results_stats = (D3D12_QUERY_DATA_PIPELINE_STATISTICS *)results;
            memset(result, 0, sizeof(*result));
   for (unsigned i = 0; i < q->curr_query; ++i) {
      switch (q->d3d12qtype) {
   case D3D12_QUERY_TYPE_BINARY_OCCLUSION:
                  case D3D12_QUERY_TYPE_OCCLUSION:
                  case D3D12_QUERY_TYPE_TIMESTAMP:
      if (q_parent->type == PIPE_QUERY_TIME_ELAPSED)
         else
               case D3D12_QUERY_TYPE_PIPELINE_STATISTICS:
      result->pipeline_statistics.ia_vertices += results_stats[i].IAVertices;
   result->pipeline_statistics.ia_primitives += results_stats[i].IAPrimitives;
   result->pipeline_statistics.vs_invocations += results_stats[i].VSInvocations;
   result->pipeline_statistics.gs_invocations += results_stats[i].GSInvocations;
   result->pipeline_statistics.gs_primitives += results_stats[i].GSPrimitives;
   result->pipeline_statistics.c_invocations += results_stats[i].CInvocations;
   result->pipeline_statistics.c_primitives += results_stats[i].CPrimitives;
   result->pipeline_statistics.ps_invocations += results_stats[i].PSInvocations;
   result->pipeline_statistics.hs_invocations += results_stats[i].HSInvocations;
   result->pipeline_statistics.ds_invocations += results_stats[i].DSInvocations;
               case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0:
   case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1:
   case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM2:
   case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3:
      result->so_statistics.num_primitives_written += results_so[i].NumPrimitivesWritten;
               default:
      debug_printf("unsupported query type: %s\n",
                        if (write) {
      if (q->d3d12qtype == D3D12_QUERY_TYPE_PIPELINE_STATISTICS) {
      results_stats[0].IAVertices = result->pipeline_statistics.ia_vertices;
   results_stats[0].IAPrimitives = result->pipeline_statistics.ia_primitives;
   results_stats[0].VSInvocations = result->pipeline_statistics.vs_invocations;
   results_stats[0].GSInvocations = result->pipeline_statistics.gs_invocations;
   results_stats[0].GSPrimitives = result->pipeline_statistics.gs_primitives;
   results_stats[0].CInvocations = result->pipeline_statistics.c_invocations;
   results_stats[0].CPrimitives = result->pipeline_statistics.c_primitives;
   results_stats[0].PSInvocations = result->pipeline_statistics.ps_invocations;
   results_stats[0].HSInvocations = result->pipeline_statistics.hs_invocations;
   results_stats[0].DSInvocations = result->pipeline_statistics.ds_invocations;
      } else if (d3d12_query_heap_type(q_parent->type, sub_query) == D3D12_QUERY_HEAP_TYPE_SO_STATISTICS) {
      results_so[0].NumPrimitivesWritten = result->so_statistics.num_primitives_written;
      } else {
      if (unlikely(q->d3d12qtype == D3D12_QUERY_TYPE_TIMESTAMP)) {
      results_u64[0] = 0;
      } else {
                                 if (q->d3d12qtype == D3D12_QUERY_TYPE_TIMESTAMP)
               }
      static bool
   accumulate_result(struct d3d12_context *ctx, struct d3d12_query *q,
         {
               switch (q->type) {
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      if (!accumulate_subresult(ctx, q, 0, &local_result, write))
                  if (!accumulate_subresult(ctx, q, 1, &local_result, write))
                  if (!accumulate_subresult(ctx, q, 2, &local_result, write))
         result->u64 += local_result.pipeline_statistics.ia_primitives;
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      if (!accumulate_subresult(ctx, q, 0, &local_result, write))
         result->u64 = local_result.so_statistics.num_primitives_written;
      default:
      assert(num_sub_queries(q->type) == 1);
         }
      static bool
   subquery_should_be_active(struct d3d12_context *ctx, struct d3d12_query *q, unsigned sub_query)
   {
      switch (q->type) {
   case PIPE_QUERY_PRIMITIVES_GENERATED: {
      bool has_xfb = !!ctx->gfx_pipeline_state.num_so_targets;
   struct d3d12_shader_selector *gs = ctx->gfx_stages[PIPE_SHADER_GEOMETRY];
   bool has_gs = gs && !gs->is_variant;
   switch (sub_query) {
   case 0: return has_xfb;
   case 1: return !has_xfb && has_gs;
   case 2: return !has_xfb && !has_gs;
   default: unreachable("Invalid subquery for primitives generated");
   }
      }
   default:
            }
      static bool 
   query_ensure_ready(struct d3d12_screen* screen, struct d3d12_context* ctx, struct d3d12_query* query, bool wait)
   {
      // If the query is not flushed, it won't have 
   // been submitted yet, and won't have a waitable 
   // fence value
   if (query->fence_value == UINT64_MAX) {
                  if (screen->fence->GetCompletedValue() < query->fence_value){
      if (!wait)
                           }
      static void
   begin_subquery(struct d3d12_context *ctx, struct d3d12_query *q_parent, unsigned sub_query)
   {
      struct d3d12_query_impl *q = &q_parent->subqueries[sub_query];
   if (q->curr_query == q->num_queries) {
               query_ensure_ready(d3d12_screen(ctx->base.screen), ctx, q_parent, false);
   d3d12_foreach_submitted_batch(ctx, old_batch) {
      if (old_batch->fence && old_batch->fence->value <= q_parent->fence_value)
               /* Accumulate current results and store in first slot */
   accumulate_subresult(ctx, q_parent, sub_query, &result, true);
               ctx->cmdlist->BeginQuery(q->query_heap, q->d3d12qtype, q->curr_query);
      }
      static void
   begin_query(struct d3d12_context *ctx, struct d3d12_query *q_parent, bool restart)
   {
      for (unsigned i = 0; i < num_sub_queries(q_parent->type); ++i) {
      if (restart)
            if (!subquery_should_be_active(ctx, q_parent, i))
                  }
         static void
   begin_timer_query(struct d3d12_context *ctx, struct d3d12_query *q_parent, bool restart)
   {
               /* For PIPE_QUERY_TIME_ELAPSED we record one time with BeginQuery and one in
   * EndQuery, so we need two query slots */
            if (restart) {
      q->curr_query = 0;
      } else if (query_index == q->num_queries) {
                        query_ensure_ready(d3d12_screen(ctx->base.screen), ctx, q_parent, false);
   d3d12_foreach_submitted_batch(ctx, old_batch) {
      if (old_batch->fence && old_batch->fence->value <= q_parent->fence_value)
               accumulate_subresult(ctx, q_parent, 0, &result, true);
               ctx->cmdlist->EndQuery(q->query_heap, q->d3d12qtype, query_index);
      }
      static bool
   d3d12_begin_query(struct pipe_context *pctx,
         {
      struct d3d12_context *ctx = d3d12_context(pctx);
                     if (unlikely(query->type == PIPE_QUERY_TIME_ELAPSED))
         else {
      begin_query(ctx, query, true);
                  }
      static void
   end_subquery(struct d3d12_context *ctx, struct d3d12_query *q_parent, unsigned sub_query)
   {
               uint64_t offset = 0;
   struct d3d12_batch *batch = d3d12_current_batch(ctx);
   struct d3d12_resource *res = (struct d3d12_resource *)q->buffer;
            /* For TIMESTAMP, there's only one slot */
   if (q_parent->type == PIPE_QUERY_TIMESTAMP)
            /* With QUERY_TIME_ELAPSED we have recorded one value at
      * (2 * q->curr_query), and now we record a value at (2 * q->curr_query + 1)
         unsigned resolve_count = q_parent->type == PIPE_QUERY_TIME_ELAPSED ? 2 : 1;
   unsigned resolve_index = resolve_count * q->curr_query;
            offset += q->buffer_offset + resolve_index * q->query_size;
   ctx->cmdlist->EndQuery(q->query_heap, q->d3d12qtype, end_index);
   d3d12_transition_resource_state(ctx, res, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_TRANSITION_FLAG_INVALIDATE_BINDINGS);
   d3d12_apply_resource_states(ctx, false);
   ctx->cmdlist->ResolveQueryData(q->query_heap, q->d3d12qtype, resolve_index,
            d3d12_batch_reference_object(batch, q->query_heap);
            assert(q->curr_query < q->num_queries);
   q->curr_query++;
      }
      static void
   end_query(struct d3d12_context *ctx, struct d3d12_query *q_parent)
   {
      for (unsigned i = 0; i < num_sub_queries(q_parent->type); ++i) {
      struct d3d12_query_impl *q = &q_parent->subqueries[i];
   if (!q->active)
                  }
      static bool
   d3d12_end_query(struct pipe_context *pctx,
         {
      struct d3d12_context *ctx = d3d12_context(pctx);
            // Assign the sentinel and track now that the query is ended
   query->fence_value = UINT64_MAX;
                     if (query->type != PIPE_QUERY_TIMESTAMP &&
      query->type != PIPE_QUERY_TIME_ELAPSED)
         }
      static bool
   d3d12_get_query_result(struct pipe_context *pctx,
                     {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_screen *screen = d3d12_screen(ctx->base.screen);
            if (!query_ensure_ready(screen, ctx, query, wait))
               }
      void
   d3d12_suspend_queries(struct d3d12_context *ctx)
   {
      list_for_each_entry(struct d3d12_query, query, &ctx->active_queries, active_list) {
            }
      void
   d3d12_resume_queries(struct d3d12_context *ctx)
   {
      list_for_each_entry(struct d3d12_query, query, &ctx->active_queries, active_list) {
            }
      void
   d3d12_validate_queries(struct d3d12_context *ctx)
   {
      /* Nothing to do, all queries are suspended */
   if (ctx->queries_disabled)
            list_for_each_entry(struct d3d12_query, query, &ctx->active_queries, active_list) {
      for (unsigned i = 0; i < num_sub_queries(query->type); ++i) {
      if (query->subqueries[i].active && !subquery_should_be_active(ctx, query, i))
         else if (!query->subqueries[i].active && subquery_should_be_active(ctx, query, i))
            }
      static void
   d3d12_set_active_query_state(struct pipe_context *pctx, bool enable)
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
            if (enable)
         else
      }
      static void
   d3d12_render_condition(struct pipe_context *pctx,
                     {
      struct d3d12_context *ctx = d3d12_context(pctx);
            if (query == nullptr) {
      ctx->cmdlist->SetPredication(nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);
   ctx->current_predication = nullptr;
               assert(num_sub_queries(query->type) == 1);
   if (!query->predicate)
      query->predicate = d3d12_resource(pipe_buffer_create(pctx->screen, 0,
                     query_ensure_ready(d3d12_screen(ctx->base.screen), ctx, query, false);
   d3d12_foreach_submitted_batch(ctx, old_batch) {
      if (old_batch->fence && old_batch->fence->value <= query->fence_value)
               union pipe_query_result result;
               struct d3d12_resource *res = (struct d3d12_resource *)query->subqueries[0].buffer;
   uint64_t source_offset = 0;
   ID3D12Resource *source = d3d12_resource_underlying(res, &source_offset);
   source_offset += query->subqueries[0].buffer_offset;
   d3d12_transition_resource_state(ctx, res, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_TRANSITION_FLAG_INVALIDATE_BINDINGS);
   d3d12_transition_resource_state(ctx, query->predicate, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_TRANSITION_FLAG_NONE);
   d3d12_apply_resource_states(ctx, false);
   ctx->cmdlist->CopyBufferRegion(d3d12_resource_resource(query->predicate), 0,
                  d3d12_transition_resource_state(ctx, query->predicate, D3D12_RESOURCE_STATE_PREDICATION, D3D12_TRANSITION_FLAG_NONE);
            ctx->current_predication = query->predicate;
   ctx->predication_condition = condition;
      }
      void
   d3d12_enable_predication(struct d3d12_context *ctx)
   {
      /* documentation of ID3D12GraphicsCommandList::SetPredication method:
      * "resource manipulation commands are _not_ actually performed
   *  if the resulting predicate data of the predicate is equal to
   *  the operation specified."
      ctx->cmdlist->SetPredication(d3d12_resource_resource(ctx->current_predication), 0,
            }
      void
   d3d12_context_query_init(struct pipe_context *pctx)
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
   list_inithead(&ctx->active_queries);
            u_suballocator_init(&ctx->query_allocator, &ctx->base, 4096, 0, PIPE_USAGE_STAGING,
            pctx->create_query = d3d12_create_query;
   pctx->destroy_query = d3d12_destroy_query;
   pctx->begin_query = d3d12_begin_query;
   pctx->end_query = d3d12_end_query;
   pctx->get_query_result = d3d12_get_query_result;
   pctx->set_active_query_state = d3d12_set_active_query_state;
      }
