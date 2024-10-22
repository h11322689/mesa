   /*
   * Copyright 2011 Nouveau Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   *
   * Authors: Christoph Bumiller
   */
      #define NV50_PUSH_EXPLICIT_SPACE_CHECKING
      #include "nv50/nv50_context.h"
   #include "nv50/nv50_query.h"
   #include "nv50/nv50_query_hw.h"
   #include "nv50/nv50_query_hw_metric.h"
   #include "nv50/nv50_query_hw_sm.h"
      static struct pipe_query *
   nv50_create_query(struct pipe_context *pipe, unsigned type, unsigned index)
   {
      struct nv50_context *nv50 = nv50_context(pipe);
            q = nv50_hw_create_query(nv50, type, index);
      }
      static void
   nv50_destroy_query(struct pipe_context *pipe, struct pipe_query *pq)
   {
      struct nv50_query *q = nv50_query(pq);
      }
      static bool
   nv50_begin_query(struct pipe_context *pipe, struct pipe_query *pq)
   {
      struct nv50_query *q = nv50_query(pq);
      }
      static bool
   nv50_end_query(struct pipe_context *pipe, struct pipe_query *pq)
   {
      struct nv50_query *q = nv50_query(pq);
   q->funcs->end_query(nv50_context(pipe), q);
      }
      static bool
   nv50_get_query_result(struct pipe_context *pipe, struct pipe_query *pq,
         {
      struct nv50_query *q = nv50_query(pq);
      }
      static void
   nv50_render_condition(struct pipe_context *pipe,
               {
      struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_query *q = nv50_query(pq);
   struct nv50_hw_query *hq = nv50_hw_query(q);
   uint32_t cond;
   bool wait =
      mode != PIPE_RENDER_COND_NO_WAIT &&
         if (!pq) {
         }
   else {
      /* NOTE: comparison of 2 queries only works if both have completed */
   switch (q->type) {
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      cond = condition ? NV50_3D_COND_MODE_EQUAL :
         wait = true;
      case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      if (hq->state == NV50_HW_QUERY_STATE_READY)
         if (likely(!condition)) {
         } else {
         }
      default:
      assert(!"render condition query not a predicate");
   cond = NV50_3D_COND_MODE_ALWAYS;
                  nv50->cond_query = pq;
   nv50->cond_cond = condition;
   nv50->cond_condmode = cond;
            if (!pq) {
      PUSH_SPACE(push, 2);
   BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
   PUSH_DATA (push, cond);
                        if (wait && hq->state != NV50_HW_QUERY_STATE_READY) {
      BEGIN_NV04(push, SUBC_3D(NV50_GRAPH_SERIALIZE), 1);
               PUSH_REF1 (push, hq->bo, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   BEGIN_NV04(push, NV50_3D(COND_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, hq->bo->offset + hq->offset);
   PUSH_DATA (push, hq->bo->offset + hq->offset);
            BEGIN_NV04(push, NV50_2D(COND_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, hq->bo->offset + hq->offset);
      }
      static void
   nv50_set_active_query_state(struct pipe_context *pipe, bool enable)
   {
   }
      void
   nv50_init_query_functions(struct nv50_context *nv50)
   {
               pipe->create_query = nv50_create_query;
   pipe->destroy_query = nv50_destroy_query;
   pipe->begin_query = nv50_begin_query;
   pipe->end_query = nv50_end_query;
   pipe->get_query_result = nv50_get_query_result;
   pipe->set_active_query_state = nv50_set_active_query_state;
   pipe->render_condition = nv50_render_condition;
      }
      int
   nv50_screen_get_driver_query_info(struct pipe_screen *pscreen,
               {
      struct nv50_screen *screen = nv50_screen(pscreen);
                     if (!info)
            /* Init default values. */
   info->name = "this_is_not_the_query_you_are_looking_for";
   info->query_type = 0xdeadd01d;
   info->max_value.u64 = 0;
   info->type = PIPE_DRIVER_QUERY_TYPE_UINT64;
   info->group_id = -1;
               }
      int
   nv50_screen_get_driver_query_group_info(struct pipe_screen *pscreen,
               {
      struct nv50_screen *screen = nv50_screen(pscreen);
            if (screen->compute)
      if (screen->base.class_3d >= NV84_3D_CLASS)
         if (!info)
            if (id == NV50_HW_SM_QUERY_GROUP) {
      if (screen->compute) {
                        /* Expose the maximum number of hardware counters available,
   * although some queries use more than one counter. Expect failures
   * in that case but as performance counters are for developers,
   * this should not have a real impact. */
   info->max_active_queries = 4;
   info->num_queries = NV50_HW_SM_QUERY_COUNT;
            } else
   if (id == NV50_HW_METRIC_QUERY_GROUP) {
      if (screen->compute) {
      if (screen->base.class_3d >= NV84_3D_CLASS) {
      info->name = "Performance metrics";
   info->max_active_queries = 2; /* A metric uses at least 2 queries */
   info->num_queries = NV50_HW_METRIC_QUERY_COUNT;
                     /* user asked for info about non-existing query group */
   info->name = "this_is_not_the_query_group_you_are_looking_for";
   info->max_active_queries = 0;
   info->num_queries = 0;
      }
