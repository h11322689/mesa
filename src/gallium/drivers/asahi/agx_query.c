   /*
   * Copyright 2022 Alyssa Rosenzweig
   * Copyright 2019-2020 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include "util/u_prim.h"
   #include "agx_state.h"
   #include "pool.h"
      static struct pipe_query *
   agx_create_query(struct pipe_context *ctx, unsigned query_type, unsigned index)
   {
               query->type = query_type;
               }
      static bool
   is_occlusion(struct agx_query *query)
   {
      switch (query->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
         default:
            }
      static void
   agx_destroy_query(struct pipe_context *ctx, struct pipe_query *pquery)
   {
               /* It is legal for the query to be destroyed before its value is read,
   * particularly during application teardown. In this case, don't leave a
   * dangling reference to the query.
   */
   if (query->writer && is_occlusion(query)) {
      *util_dynarray_element(&query->writer->occlusion_queries,
                  }
      static bool
   agx_begin_query(struct pipe_context *pctx, struct pipe_query *pquery)
   {
      struct agx_context *ctx = agx_context(pctx);
                     switch (query->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      ctx->occlusion_query = query;
         case PIPE_QUERY_PRIMITIVES_GENERATED:
      ctx->prims_generated = query;
         case PIPE_QUERY_PRIMITIVES_EMITTED:
      ctx->tf_prims_generated = query;
         default:
                  /* begin_query zeroes, flush so we can do that write. If anything (i.e.
   * other than piglit) actually hits this, we could shadow the query to
   * avoid the flush.
   */
   if (query->writer) {
      agx_flush_batch_for_reason(ctx, query->writer, "Query overwritten");
               assert(query->writer == NULL);
   query->value = 0;
      }
      static bool
   agx_end_query(struct pipe_context *pctx, struct pipe_query *pquery)
   {
      struct agx_context *ctx = agx_context(pctx);
                     switch (query->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      ctx->occlusion_query = NULL;
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      ctx->prims_generated = NULL;
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      ctx->tf_prims_generated = NULL;
      default:
            }
      static bool
   agx_get_query_result(struct pipe_context *pctx, struct pipe_query *pquery,
         {
      struct agx_query *query = (struct agx_query *)pquery;
            /* For GPU queries, flush the writer. When the writer is flushed, the GPU
   * will write the value, and when we wait for the writer, the CPU will read
   * the value into query->value.
   */
   if (query->writer != NULL) {
      /* Querying the result forces a query to finish in finite time, so we
   * need to flush. Furthermore, we need all earlier queries
   * to finish before this query, so we sync unconditionally (so we can
   * maintain the lie that all queries are finished when read).
   *
   * TODO: Optimize based on wait flag.
   */
   struct agx_batch *writer = query->writer;
   agx_flush_batch_for_reason(ctx, writer, "GPU query");
               /* After syncing, there is no writer left, so query->value is ready */
            switch (query->type) {
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      vresult->b = query->value;
         case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      vresult->u64 = query->value;
         default:
            }
      static void
   agx_set_active_query_state(struct pipe_context *pipe, bool enable)
   {
               ctx->active_queries = enable;
      }
      static uint16_t
   agx_add_query_to_batch(struct agx_batch *batch, struct agx_query *query,
         {
      /* If written by another batch, flush it now. If this affects real apps, we
   * could avoid this flush by merging query results.
   */
   if (query->writer && query->writer != batch) {
      agx_flush_batch_for_reason(batch->ctx, query->writer,
               /* Allocate if needed */
   if (query->writer == NULL) {
      query->writer = batch;
   query->writer_index =
                        assert(query->writer == batch);
   assert(*util_dynarray_element(array, struct agx_query *,
               }
      uint16_t
   agx_get_oq_index(struct agx_batch *batch, struct agx_query *query)
   {
                  }
      uint64_t
   agx_get_query_address(struct agx_batch *batch, struct agx_query *query)
   {
                        /* Allocate storage for the query in the batch */
   if (!query->ptr.cpu) {
      query->ptr = agx_pool_alloc_aligned(&batch->pool, sizeof(uint64_t),
            uint64_t *value = query->ptr.cpu;
                  }
      void
   agx_finish_batch_queries(struct agx_batch *batch)
   {
               util_dynarray_foreach(&batch->occlusion_queries, struct agx_query *, it) {
               /* Skip queries that have since been destroyed */
   if (query == NULL)
                     /* Get the result for this batch. If occlusion is NULL, it means that no
   * draws actually enabled any occlusion queries, so there's no change.
   */
   if (occlusion != NULL) {
               /* Accumulate with the previous result (e.g. in case we split a frame
   * into multiple batches so an API-level query spans multiple batches).
   */
   if (query->type == PIPE_QUERY_OCCLUSION_COUNTER)
         else
               query->writer = NULL;
               /* Now handle non-occlusion queries in a similar way */
   util_dynarray_foreach(&batch->nonocclusion_queries, struct agx_query *, it) {
      struct agx_query *query = *it;
   if (query == NULL)
                     /* Accumulate */
   uint64_t *value = query->ptr.cpu;
   query->value += (*value);
   query->writer = NULL;
   query->writer_index = 0;
   query->ptr.cpu = NULL;
         }
      static void
   agx_render_condition(struct pipe_context *pipe, struct pipe_query *query,
         {
               ctx->cond_query = query;
   ctx->cond_cond = condition;
      }
      bool
   agx_render_condition_check_inner(struct agx_context *ctx)
   {
                        union pipe_query_result res = {0};
   bool wait = ctx->cond_mode != PIPE_RENDER_COND_NO_WAIT &&
                     if (agx_get_query_result(&ctx->base, pq, wait, &res))
               }
      void
   agx_init_query_functions(struct pipe_context *pctx)
   {
      pctx->create_query = agx_create_query;
   pctx->destroy_query = agx_destroy_query;
   pctx->begin_query = agx_begin_query;
   pctx->end_query = agx_end_query;
   pctx->get_query_result = agx_get_query_result;
   pctx->set_active_query_state = agx_set_active_query_state;
            /* By default queries are active */
      }
