   /*
   * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
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
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      #include "util/u_memory.h"
      #include "r300_context.h"
   #include "r300_screen.h"
   #include "r300_emit.h"
      #include <stdio.h>
      static struct pipe_query *r300_create_query(struct pipe_context *pipe,
               {
      struct r300_context *r300 = r300_context(pipe);
   struct r300_screen *r300screen = r300->screen;
            if (query_type != PIPE_QUERY_OCCLUSION_COUNTER &&
      query_type != PIPE_QUERY_OCCLUSION_PREDICATE &&
   query_type != PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE &&
   query_type != PIPE_QUERY_GPU_FINISHED) {
               q = CALLOC_STRUCT(r300_query);
   if (!q)
                     if (query_type == PIPE_QUERY_GPU_FINISHED) {
                  if (r300screen->caps.family == CHIP_RV530)
         else
            q->buf = r300->rws->buffer_create(r300->rws,
                           if (!q->buf) {
      FREE(q);
      }
      }
      static void r300_destroy_query(struct pipe_context* pipe,
         {
               pb_reference(&q->buf, NULL);
      }
      void r300_resume_query(struct r300_context *r300,
         {
      r300->query_current = query;
      }
      static bool r300_begin_query(struct pipe_context* pipe,
         {
      struct r300_context* r300 = r300_context(pipe);
            if (q->type == PIPE_QUERY_GPU_FINISHED)
            if (r300->query_current != NULL) {
      fprintf(stderr, "r300: begin_query: "
         assert(0);
               q->num_results = 0;
   r300_resume_query(r300, q);
      }
      void r300_stop_query(struct r300_context *r300)
   {
      r300_emit_query_end(r300);
      }
      static bool r300_end_query(struct pipe_context* pipe,
         {
      struct r300_context* r300 = r300_context(pipe);
            if (q->type == PIPE_QUERY_GPU_FINISHED) {
      pb_reference(&q->buf, NULL);
   r300_flush(pipe, PIPE_FLUSH_ASYNC,
                     if (q != r300->query_current) {
      fprintf(stderr, "r300: end_query: Got invalid query.\n");
   assert(0);
                           }
      static bool r300_get_query_result(struct pipe_context* pipe,
                     {
      struct r300_context* r300 = r300_context(pipe);
   struct r300_query *q = r300_query(query);
   unsigned i;
            if (q->type == PIPE_QUERY_GPU_FINISHED) {
      if (wait) {
         r300->rws->buffer_wait(r300->rws, q->buf, OS_TIMEOUT_INFINITE,
         } else {
         }
               map = r300->rws->buffer_map(r300->rws, q->buf, &r300->cs,
               if (!map)
            /* Sum up the results. */
   temp = 0;
   for (i = 0; i < q->num_results; i++) {
      /* Convert little endian values written by GPU to CPU byte order */
   temp += util_le32_to_cpu(*map);
               if (q->type == PIPE_QUERY_OCCLUSION_PREDICATE ||
      q->type == PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE) {
      } else {
         }
      }
      static void r300_render_condition(struct pipe_context *pipe,
                     {
      struct r300_context *r300 = r300_context(pipe);
   union pipe_query_result result;
                     if (query) {
      wait = mode == PIPE_RENDER_COND_WAIT ||
            if (r300_get_query_result(pipe, query, wait, &result)) {
         if (r300_query(query)->type == PIPE_QUERY_OCCLUSION_PREDICATE ||
      r300_query(query)->type == PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE) {
      } else {
               }
      static void
   r300_set_active_query_state(struct pipe_context *pipe, bool enable)
   {
   }
      void r300_init_query_functions(struct r300_context* r300)
   {
      r300->context.create_query = r300_create_query;
   r300->context.destroy_query = r300_destroy_query;
   r300->context.begin_query = r300_begin_query;
   r300->context.end_query = r300_end_query;
   r300->context.get_query_result = r300_get_query_result;
   r300->context.set_active_query_state = r300_set_active_query_state;
      }
