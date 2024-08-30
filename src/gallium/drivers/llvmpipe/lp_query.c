   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
   * Copyright 2010 VMware, Inc.
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
      /* Authors:
   *    Keith Whitwell, Qicheng Christopher Li, Brian Paul
   */
      #include "draw/draw_context.h"
   #include "pipe/p_defines.h"
   #include "util/u_memory.h"
   #include "util/os_time.h"
   #include "lp_context.h"
   #include "lp_flush.h"
   #include "lp_fence.h"
   #include "lp_query.h"
   #include "lp_screen.h"
   #include "lp_state.h"
   #include "lp_rast.h"
         static struct llvmpipe_query *
   llvmpipe_query(struct pipe_query *p)
   {
         }
         static struct pipe_query *
   llvmpipe_create_query(struct pipe_context *pipe,
               {
               struct llvmpipe_query *pq = CALLOC_STRUCT(llvmpipe_query);
   if (pq) {
      pq->type = type;
                  }
         static void
   llvmpipe_destroy_query(struct pipe_context *pipe, struct pipe_query *q)
   {
               /* Ideally we would refcount queries & not get destroyed until the
   * last scene had finished with us.
   */
   if (pq->fence) {
      if (!lp_fence_issued(pq->fence))
            if (!lp_fence_signalled(pq->fence))
                           }
         static bool
   llvmpipe_get_query_result(struct pipe_context *pipe,
                     {
      const struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   const unsigned num_threads = MAX2(1, screen->num_threads);
            if (pq->fence) {
      /* only have a fence if there was a scene */
   if (!lp_fence_signalled(pq->fence)) {
                                                   /* Always initialize the first 64-bit result word to zero since some
   * callers don't consider whether the result is actually a 1-byte or 4-byte
   * quantity.
   */
            /* Combine the per-thread results */
   switch (pq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      {
      uint64_t sum = 0;
   for (unsigned i = 0; i < num_threads; i++) {
         }
      }
      case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      result->b = false;
   for (unsigned i = 0; i < num_threads; i++) {
      /* safer (still not guaranteed) when there's an overflow */
   if (pq->end[i] > 0) {
      result->b = true;
         }
      case PIPE_QUERY_TIMESTAMP:
      {
      uint64_t max_time = 0;
   for (unsigned i = 0; i < num_threads; i++) {
         }
      }
      case PIPE_QUERY_TIME_ELAPSED:
      {
      uint64_t start = UINT64_MAX, end = 0;
   for (unsigned i = 0; i < num_threads; i++) {
      if (pq->start[i]) {
         }
   if (pq->end[i]) {
            }
      }
      case PIPE_QUERY_TIMESTAMP_DISJOINT:
      /* os_get_time_nano return nanoseconds */
   result->timestamp_disjoint.frequency = UINT64_C(1000000000);
   result->timestamp_disjoint.disjoint = false;
      case PIPE_QUERY_GPU_FINISHED:
      result->b = true;
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      result->u64 = pq->num_primitives_generated[0];
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      result->u64 = pq->num_primitives_written[0];
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      result->b = false;
   for (unsigned s = 0; s < PIPE_MAX_VERTEX_STREAMS; s++) {
      if (pq->num_primitives_generated[s] > pq->num_primitives_written[s]) {
      result->b = true;
         }
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      result->b = pq->num_primitives_generated[0] > pq->num_primitives_written[0];
      case PIPE_QUERY_SO_STATISTICS:
      result->so_statistics.num_primitives_written = pq->num_primitives_written[0];
   result->so_statistics.primitives_storage_needed = pq->num_primitives_generated[0];
      case PIPE_QUERY_PIPELINE_STATISTICS:
      {
      /* only ps_invocations are per-bin/thread */
   uint64_t sum = 0;
   for (unsigned i = 0; i < num_threads; i++) {
         }
   /* The FS/PS operates on a block of pixels at a time.  The counter is
   * incremented per block so we multiply by pixels per block here.
   * This will not be a pixel-exact result.
   */
   pq->stats.ps_invocations =
            }
      default:
      assert(0);
                  }
         static void
   llvmpipe_get_query_result_resource(struct pipe_context *pipe,
                                       {
      const struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   const unsigned num_threads = MAX2(1, screen->num_threads);
   const struct llvmpipe_query *pq = llvmpipe_query(q);
   const struct llvmpipe_resource *lpr = llvmpipe_resource(resource);
            if (pq->fence) {
      /* only have a fence if there was a scene */
   if (!lp_fence_signalled(pq->fence)) {
                     if (flags & PIPE_QUERY_WAIT)
      }
      } else {
                  uint64_t value = 0, value2 = 0;
   unsigned num_values = 1;
   if (index == -1) {
         } else {
      /* don't write a value if fence hasn't signalled and partial isn't set */
   if (!ready && !(flags & PIPE_QUERY_PARTIAL))
            switch (pq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      for (unsigned i = 0; i < num_threads; i++) {
         }
      case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      for (unsigned i = 0; i < num_threads; i++) {
      /* safer (still not guaranteed) when there's an overflow */
      }
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      value = pq->num_primitives_generated[0];
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      value = pq->num_primitives_written[0];
      case PIPE_QUERY_TIMESTAMP:
      for (unsigned i = 0; i < num_threads; i++) {
      if (pq->end[i] > value) {
            }
      case PIPE_QUERY_TIME_ELAPSED: {
      uint64_t start = (uint64_t)-1, end = 0;
   for (unsigned i = 0; i < num_threads; i++) {
      if (pq->start[i] && pq->start[i] < start)
         if (pq->end[i] && pq->end[i] > end)
      }
   value = end - start;
      }
   case PIPE_QUERY_SO_STATISTICS:
      value = pq->num_primitives_written[0];
   value2 = pq->num_primitives_generated[0];
   num_values = 2;
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      value = 0;
   for (unsigned s = 0; s < PIPE_MAX_VERTEX_STREAMS; s++)
            case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      value = (pq->num_primitives_generated[0] > pq->num_primitives_written[0]);
      case PIPE_QUERY_PIPELINE_STATISTICS:
      switch ((enum pipe_statistics_query_index)index) {
   case PIPE_STAT_QUERY_IA_VERTICES:
      value = pq->stats.ia_vertices;
      case PIPE_STAT_QUERY_IA_PRIMITIVES:
      value = pq->stats.ia_primitives;
      case PIPE_STAT_QUERY_VS_INVOCATIONS:
      value = pq->stats.vs_invocations;
      case PIPE_STAT_QUERY_GS_INVOCATIONS:
      value = pq->stats.gs_invocations;
      case PIPE_STAT_QUERY_GS_PRIMITIVES:
      value = pq->stats.gs_primitives;
      case PIPE_STAT_QUERY_C_INVOCATIONS:
      value = pq->stats.c_invocations;
      case PIPE_STAT_QUERY_C_PRIMITIVES:
      value = pq->stats.c_primitives;
      case PIPE_STAT_QUERY_PS_INVOCATIONS:
      value = 0;
   for (unsigned i = 0; i < num_threads; i++) {
         }
   value *= LP_RASTER_BLOCK_SIZE * LP_RASTER_BLOCK_SIZE;
      case PIPE_STAT_QUERY_HS_INVOCATIONS:
      value = pq->stats.hs_invocations;
      case PIPE_STAT_QUERY_DS_INVOCATIONS:
      value = pq->stats.ds_invocations;
      case PIPE_STAT_QUERY_CS_INVOCATIONS:
      value = pq->stats.cs_invocations;
      case PIPE_STAT_QUERY_TS_INVOCATIONS:
      value = pq->stats.ts_invocations;
      case PIPE_STAT_QUERY_MS_INVOCATIONS:
      value = pq->stats.ms_invocations;
      }
      default:
      fprintf(stderr, "Unknown query type %d\n", pq->type);
                           /* Write 1 or 2 result values */
   for (unsigned i = 0; i < num_values; i++) {
      if (i == 1) {
      value = value2;
   // advance dst pointer by 4 or 8 bytes
   dst += (result_type == PIPE_QUERY_TYPE_I64 ||
      }
   switch (result_type) {
   case PIPE_QUERY_TYPE_I32: {
      int32_t *iptr = (int32_t *)dst;
   *iptr = (int32_t) MIN2(value, INT32_MAX);
      }
   case PIPE_QUERY_TYPE_U32: {
      uint32_t *uptr = (uint32_t *)dst;
   *uptr = (uint32_t) MIN2(value, UINT32_MAX);
      }
   case PIPE_QUERY_TYPE_I64: {
      int64_t *iptr = (int64_t *)dst;
   *iptr = (int64_t)value;
      }
   case PIPE_QUERY_TYPE_U64: {
      uint64_t *uptr = (uint64_t *)dst;
   *uptr = (uint64_t)value;
      }
         }
         static bool
   llvmpipe_begin_query(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
            /* Check if the query is already in the scene.  If so, we need to
   * flush the scene now.  Real apps shouldn't re-use a query in a
   * frame of rendering.
   */
   if (pq->fence && !lp_fence_issued(pq->fence)) {
                  memset(pq->start, 0, sizeof(pq->start));
   memset(pq->end, 0, sizeof(pq->end));
            switch (pq->type) {
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      pq->num_primitives_written[0] = llvmpipe->so_stats[pq->index].num_primitives_written;
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      pq->num_primitives_generated[0] = llvmpipe->so_stats[pq->index].primitives_storage_needed;
   llvmpipe->active_primgen_queries++;
      case PIPE_QUERY_SO_STATISTICS:
      pq->num_primitives_written[0] = llvmpipe->so_stats[pq->index].num_primitives_written;
   pq->num_primitives_generated[0] = llvmpipe->so_stats[pq->index].primitives_storage_needed;
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      for (unsigned s = 0; s < PIPE_MAX_VERTEX_STREAMS; s++) {
      pq->num_primitives_written[s] = llvmpipe->so_stats[s].num_primitives_written;
      }
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      pq->num_primitives_written[0] = llvmpipe->so_stats[pq->index].num_primitives_written;
   pq->num_primitives_generated[0] = llvmpipe->so_stats[pq->index].primitives_storage_needed;
      case PIPE_QUERY_PIPELINE_STATISTICS:
      /* reset our cache */
   if (llvmpipe->active_statistics_queries == 0) {
      memset(&llvmpipe->pipeline_statistics, 0,
      }
   memcpy(&pq->stats, &llvmpipe->pipeline_statistics, sizeof(pq->stats));
   llvmpipe->active_statistics_queries++;
      case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      llvmpipe->active_occlusion_queries++;
   llvmpipe->dirty |= LP_NEW_OCCLUSION_QUERY;
      default:
         }
      }
         static bool
   llvmpipe_end_query(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
                              case PIPE_QUERY_PRIMITIVES_EMITTED:
      pq->num_primitives_written[0] =
            case PIPE_QUERY_PRIMITIVES_GENERATED:
      assert(llvmpipe->active_primgen_queries);
   llvmpipe->active_primgen_queries--;
   pq->num_primitives_generated[0] =
            case PIPE_QUERY_SO_STATISTICS:
      pq->num_primitives_written[0] =
         pq->num_primitives_generated[0] =
            case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      for (unsigned s = 0; s < PIPE_MAX_VERTEX_STREAMS; s++) {
      pq->num_primitives_written[s] =
         pq->num_primitives_generated[s] =
      }
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      pq->num_primitives_written[0] =
         pq->num_primitives_generated[0] =
            case PIPE_QUERY_PIPELINE_STATISTICS:
      pq->stats.ia_vertices =
         pq->stats.ia_primitives =
         pq->stats.vs_invocations =
         pq->stats.gs_invocations =
         pq->stats.gs_primitives =
         pq->stats.c_invocations =
         pq->stats.c_primitives =
         pq->stats.ps_invocations =
         pq->stats.cs_invocations =
         pq->stats.hs_invocations =
         pq->stats.ds_invocations =
         pq->stats.ts_invocations =
         pq->stats.ms_invocations =
         llvmpipe->active_statistics_queries--;
      case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      assert(llvmpipe->active_occlusion_queries);
   llvmpipe->active_occlusion_queries--;
   llvmpipe->dirty |= LP_NEW_OCCLUSION_QUERY;
      default:
                     }
         bool
   llvmpipe_check_render_cond(struct llvmpipe_context *lp)
   {
               if (lp->render_cond_buffer) {
      uint32_t data = *(uint32_t *)((char *)lp->render_cond_buffer->data
            }
   if (!lp->render_cond_query)
            bool wait = (lp->render_cond_mode == PIPE_RENDER_COND_WAIT ||
            uint64_t result;
   bool b = pipe->get_query_result(pipe, lp->render_cond_query, wait,
         if (b)
         else
      }
         static void
   llvmpipe_set_active_query_state(struct pipe_context *pipe, bool enable)
   {
               llvmpipe->queries_disabled = !enable;
   /* for OQs we need to regenerate the fragment shader */
      }
         void
   llvmpipe_init_query_funcs(struct llvmpipe_context *llvmpipe)
   {
      llvmpipe->pipe.create_query = llvmpipe_create_query;
   llvmpipe->pipe.destroy_query = llvmpipe_destroy_query;
   llvmpipe->pipe.begin_query = llvmpipe_begin_query;
   llvmpipe->pipe.end_query = llvmpipe_end_query;
   llvmpipe->pipe.get_query_result = llvmpipe_get_query_result;
   llvmpipe->pipe.get_query_result_resource = llvmpipe_get_query_result_resource;
      }
      