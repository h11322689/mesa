   /*
   * Copyright 2015 Samuel Pitoiset
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
   */
      #include "nv50/nv50_context.h"
   #include "nv50/nv50_query_hw_metric.h"
   #include "nv50/nv50_query_hw_sm.h"
      /* === PERFORMANCE MONITORING METRICS for NV84+ === */
   static const char *nv50_hw_metric_names[] =
   {
         };
      struct nv50_hw_metric_query_cfg {
      uint32_t queries[4];
      };
      #define _SM(n) NV50_HW_SM_QUERY(NV50_HW_SM_QUERY_ ##n)
   #define _M(n, c) [NV50_HW_METRIC_QUERY_##n] = c
      /* ==== Compute capability 1.1 (G84+) ==== */
   static const struct nv50_hw_metric_query_cfg
   sm11_branch_efficiency =
   {
      .queries[0]  = _SM(BRANCH),
   .queries[1]  = _SM(DIVERGENT_BRANCH),
      };
      static const struct nv50_hw_metric_query_cfg *sm11_hw_metric_queries[] =
   {
         };
      #undef _SM
   #undef _M
      static const struct nv50_hw_metric_query_cfg *
   nv50_hw_metric_query_get_cfg(struct nv50_context *nv50,
         {
      struct nv50_query *q = &hq->base;
      }
      static void
   nv50_hw_metric_destroy_query(struct nv50_context *nv50,
         {
      struct nv50_hw_metric_query *hmq = nv50_hw_metric_query(hq);
            for (i = 0; i < hmq->num_queries; i++)
      if (hmq->queries[i]->funcs->destroy_query)
         }
      static bool
   nv50_hw_metric_begin_query(struct nv50_context *nv50, struct nv50_hw_query *hq)
   {
      struct nv50_hw_metric_query *hmq = nv50_hw_metric_query(hq);
   bool ret = false;
            for (i = 0; i < hmq->num_queries; i++) {
      ret = hmq->queries[i]->funcs->begin_query(nv50, hmq->queries[i]);
   if (!ret)
      }
      }
      static void
   nv50_hw_metric_end_query(struct nv50_context *nv50, struct nv50_hw_query *hq)
   {
      struct nv50_hw_metric_query *hmq = nv50_hw_metric_query(hq);
            for (i = 0; i < hmq->num_queries; i++)
      }
      static uint64_t
   sm11_hw_metric_calc_result(struct nv50_hw_query *hq, uint64_t res64[4])
   {
      switch (hq->base.type - NV50_HW_METRIC_QUERY(0)) {
   case NV50_HW_METRIC_QUERY_BRANCH_EFFICIENCY:
      /* (branch / (branch + divergent_branch)) * 100 */
   if (res64[0] + res64[1])
            default:
      debug_printf("invalid metric type: %d\n",
            }
      }
      static bool
   nv50_hw_metric_get_query_result(struct nv50_context *nv50,
               {
      struct nv50_hw_metric_query *hmq = nv50_hw_metric_query(hq);
   union pipe_query_result results[4] = {};
   uint64_t res64[4] = {};
   bool ret = false;
            for (i = 0; i < hmq->num_queries; i++) {
      ret = hmq->queries[i]->funcs->get_query_result(nv50, hmq->queries[i],
         if (!ret)
                     *(uint64_t *)result = sm11_hw_metric_calc_result(hq, res64);
      }
      static const struct nv50_hw_query_funcs hw_metric_query_funcs = {
      .destroy_query = nv50_hw_metric_destroy_query,
   .begin_query = nv50_hw_metric_begin_query,
   .end_query = nv50_hw_metric_end_query,
      };
      struct nv50_hw_query *
   nv50_hw_metric_create_query(struct nv50_context *nv50, unsigned type)
   {
      const struct nv50_hw_metric_query_cfg *cfg;
   struct nv50_hw_metric_query *hmq;
   struct nv50_hw_query *hq;
            if (type < NV50_HW_METRIC_QUERY(0) || type > NV50_HW_METRIC_QUERY_LAST)
            hmq = CALLOC_STRUCT(nv50_hw_metric_query);
   if (!hmq)
            hq = &hmq->base;
   hq->funcs = &hw_metric_query_funcs;
                     for (i = 0; i < cfg->num_queries; i++) {
      hmq->queries[i] = nv50_hw_sm_create_query(nv50, cfg->queries[i]);
   if (!hmq->queries[i]) {
      nv50_hw_metric_destroy_query(nv50, hq);
      }
                  }
      int
   nv50_hw_metric_get_driver_query_info(struct nv50_screen *screen, unsigned id,
         {
               if (screen->compute)
      if (screen->base.class_3d >= NV84_3D_CLASS)
         if (!info)
            if (id < count) {
      if (screen->compute) {
      if (screen->base.class_3d >= NV84_3D_CLASS) {
      info->name = nv50_hw_metric_names[id];
   info->query_type = NV50_HW_METRIC_QUERY(id);
   info->group_id = NV50_HW_METRIC_QUERY_GROUP;
            }
      }
