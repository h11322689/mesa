   /**************************************************************************
   *
   * Copyright 2013 Marek Olšák <maraeo@gmail.com>
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /* This file contains code for reading values from pipe queries
   * for displaying on the HUD. To prevent stalls when reading queries, we
   * keep a list of busy queries in a ring. We read only those queries which
   * are idle.
   */
      #include "hud/hud_private.h"
   #include "pipe/p_screen.h"
   #include "util/os_time.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include <stdio.h>
      // Must be a power of two
   #define NUM_QUERIES 8
      struct hud_batch_query_context {
      unsigned num_query_types;
   unsigned allocated_query_types;
            bool failed;
   struct pipe_query *query[NUM_QUERIES];
   union pipe_query_result *result[NUM_QUERIES];
      };
      void
   hud_batch_query_update(struct hud_batch_query_context *bq,
         {
      if (!bq || bq->failed)
            if (bq->query[bq->head])
                     while (bq->pending) {
      unsigned idx = (bq->head - bq->pending + 1) % NUM_QUERIES;
            if (!bq->result[idx])
      bq->result[idx] = MALLOC(sizeof(bq->result[idx]->batch[0]) *
      if (!bq->result[idx]) {
      fprintf(stderr, "gallium_hud: out of memory.\n");
   bq->failed = true;
               if (!pipe->get_query_result(pipe, query, false, bq->result[idx]))
            ++bq->results;
                        if (bq->pending == NUM_QUERIES) {
      fprintf(stderr,
                           pipe->destroy_query(pipe, bq->query[bq->head]);
                        if (!bq->query[bq->head]) {
      bq->query[bq->head] = pipe->create_batch_query(pipe,
                  if (!bq->query[bq->head]) {
      fprintf(stderr,
         "gallium_hud: create_batch_query failed. You may have "
   bq->failed = true;
            }
      void
   hud_batch_query_begin(struct hud_batch_query_context *bq,
         {
      if (!bq || bq->failed || !bq->query[bq->head])
            if (!pipe->begin_query(pipe, bq->query[bq->head])) {
      fprintf(stderr,
         "gallium_hud: could not begin batch query. You may have "
         }
      static bool
   batch_query_add(struct hud_batch_query_context **pbq,
         {
      struct hud_batch_query_context *bq = *pbq;
            if (!bq) {
      bq = CALLOC_STRUCT(hud_batch_query_context);
   if (!bq)
                     for (i = 0; i < bq->num_query_types; ++i) {
      if (bq->query_types[i] == query_type) {
      *result_index = i;
                  if (bq->num_query_types == bq->allocated_query_types) {
      unsigned new_alloc = MAX2(16, bq->allocated_query_types * 2);
   unsigned *new_query_types
      = REALLOC(bq->query_types,
            if (!new_query_types)
         bq->query_types = new_query_types;
               bq->query_types[bq->num_query_types] = query_type;
   *result_index = bq->num_query_types++;
      }
      void
   hud_batch_query_cleanup(struct hud_batch_query_context **pbq,
         {
      struct hud_batch_query_context *bq = *pbq;
            if (!bq)
                     if (bq->query[bq->head] && !bq->failed)
            for (idx = 0; idx < NUM_QUERIES; ++idx) {
      if (bq->query[idx])
                     FREE(bq->query_types);
      }
      struct query_info {
      struct hud_batch_query_context *batch;
            /** index to choose fields in pipe_query_data_pipeline_statistics,
   * for example.
   */
   unsigned result_index;
   enum pipe_driver_query_result_type result_type;
            /* Ring of queries. If a query is busy, we use another slot. */
   struct pipe_query *query[NUM_QUERIES];
            uint64_t last_time;
   uint64_t results_cumulative;
      };
      static void
   query_new_value_batch(struct query_info *info)
   {
      struct hud_batch_query_context *bq = info->batch;
   unsigned result_index = info->result_index;
   unsigned idx = (bq->head - bq->pending) % NUM_QUERIES;
            while (results) {
      info->results_cumulative += bq->result[idx]->batch[result_index].u64;
            --results;
         }
      static void
   query_new_value_normal(struct query_info *info, struct pipe_context *pipe)
   {
      if (info->last_time) {
      if (info->query[info->head])
            /* read query results */
   while (1) {
      struct pipe_query *query = info->query[info->tail];
                  if (query && pipe->get_query_result(pipe, query, false, &result)) {
      if (info->type == PIPE_DRIVER_QUERY_TYPE_FLOAT) {
      assert(info->result_index == 0);
      }
   else {
                                          }
   else {
      /* the oldest query is busy */
   if ((info->head+1) % NUM_QUERIES == info->tail) {
      /* all queries are busy, throw away the last query and create
   * a new one */
   fprintf(stderr,
         "gallium_hud: all queries are busy after %i frames, "
   "can't add another query\n",
   if (info->query[info->head])
         info->query[info->head] =
      }
   else {
      /* the last query is busy, we need to add a new one we can use
   * for this frame */
   info->head = (info->head+1) % NUM_QUERIES;
   if (!info->query[info->head]) {
      info->query[info->head] =
         }
            }
   else {
      /* initialize */
         }
      static void
   begin_query(struct hud_graph *gr, struct pipe_context *pipe)
   {
               assert(!info->batch);
   if (info->query[info->head])
      }
      static void
   query_new_value(struct hud_graph *gr, struct pipe_context *pipe)
   {
      struct query_info *info = gr->query_data;
            if (info->batch) {
         } else {
                  if (!info->last_time) {
      info->last_time = now;
               if (info->num_results && info->last_time + gr->pane->period <= now) {
               switch (info->result_type) {
   default:
   case PIPE_DRIVER_QUERY_RESULT_TYPE_AVERAGE:
      value = info->results_cumulative / info->num_results;
      case PIPE_DRIVER_QUERY_RESULT_TYPE_CUMULATIVE:
      value = info->results_cumulative;
               if (info->type == PIPE_DRIVER_QUERY_TYPE_FLOAT) {
                           info->last_time = now;
   info->results_cumulative = 0;
         }
      static void
   free_query_info(void *ptr, struct pipe_context *pipe)
   {
               if (!info->batch && info->last_time) {
                        for (i = 0; i < ARRAY_SIZE(info->query); i++) {
      if (info->query[i]) {
               }
      }
         /**
   * \param result_index  to select fields of pipe_query_data_pipeline_statistics,
   *                      for example.
   */
   void
   hud_pipe_query_install(struct hud_batch_query_context **pbq,
                        struct hud_pane *pane,
   const char *name,
   enum pipe_query_type query_type,
      {
      struct hud_graph *gr;
            gr = CALLOC_STRUCT(hud_graph);
   if (!gr)
            strncpy(gr->name, name, sizeof(gr->name));
   gr->name[sizeof(gr->name) - 1] = '\0';
   gr->query_data = CALLOC_STRUCT(query_info);
   if (!gr->query_data)
            gr->query_new_value = query_new_value;
            info = gr->query_data;
   info->result_type = result_type;
            if (flags & PIPE_DRIVER_QUERY_FLAG_BATCH) {
      if (!batch_query_add(pbq, query_type, &info->result_index))
            } else {
      gr->begin_query = begin_query;
   info->query_type = query_type;
               hud_pane_add_graph(pane, gr);
            if (pane->max_value < max_value)
               fail_info:
         fail_gr:
         }
      bool
   hud_driver_query_install(struct hud_batch_query_context **pbq,
               {
      struct pipe_driver_query_info query = { 0 };
   unsigned num_queries, i;
            if (!screen->get_driver_query_info)
                     for (i = 0; i < num_queries; i++) {
      if (screen->get_driver_query_info(screen, i, &query) &&
      strcmp(query.name, name) == 0) {
   found = true;
                  if (!found)
            hud_pipe_query_install(pbq, pane, query.name, query.query_type, 0,
                     }
