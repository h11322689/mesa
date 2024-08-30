   /*
   * Copyright © 2021 Raspberry Pi Ltd
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      /**
   * Gallium query object support for performance counters
   *
   * This contains the performance V3D counters queries.
   */
      #include "v3d_query.h"
      #include "common/v3d_performance_counters.h"
      struct v3d_query_perfcnt
   {
                  unsigned num_queries;
   };
      static void
   kperfmon_destroy(struct v3d_context *v3d, struct v3d_perfmon_state *perfmon)
   {
                  destroyreq.id = perfmon->kperfmon_id;
   int ret = v3d_ioctl(v3d->fd, DRM_IOCTL_V3D_PERFMON_DESTROY, &destroyreq);
   if (ret != 0)
         }
      int
   v3dX(get_driver_query_group_info_perfcnt)(struct v3d_screen *screen, unsigned index,
         {
         if (!screen->has_perfmon)
            if (!info)
            if (index > 0)
            info->name = "V3D counters";
   info->max_active_queries = DRM_V3D_MAX_PERF_COUNTERS;
            }
      int
   v3dX(get_driver_query_info_perfcnt)(struct v3d_screen *screen, unsigned index,
         {
         if (!screen->has_perfmon)
            if (!info)
            if (index >= ARRAY_SIZE(v3d_performance_counters))
            info->group_id = 0;
   info->name = v3d_performance_counters[index][V3D_PERFCNT_NAME];
   info->query_type = PIPE_QUERY_DRIVER_SPECIFIC + index;
   info->result_type = PIPE_DRIVER_QUERY_RESULT_TYPE_CUMULATIVE;
   info->type = PIPE_DRIVER_QUERY_TYPE_UINT64;
            }
      static void
   v3d_destroy_query_perfcnt(struct v3d_context *v3d, struct v3d_query *query)
   {
                           if (v3d->active_perfmon == pquery->perfmon) {
               }
   if (pquery->perfmon->kperfmon_id)
            v3d_fence_unreference(&pquery->perfmon->last_job_fence);
   free(pquery->perfmon);
   }
      static bool
   v3d_begin_query_perfcnt(struct v3d_context *v3d, struct v3d_query *query)
   {
         struct v3d_query_perfcnt *pquery = (struct v3d_query_perfcnt *)query;
   struct drm_v3d_perfmon_create createreq = { 0 };
            /* Only one perfmon can be activated per context */
   if (v3d->active_perfmon) {
            fprintf(stderr,
                              /* Reset the counters by destroying the previously allocated perfmon */
   if (pquery->perfmon->kperfmon_id)
            for (i = 0; i < pquery->num_queries; i++)
            createreq.ncounters = pquery->num_queries;
   ret = v3d_ioctl(v3d->fd, DRM_IOCTL_V3D_PERFMON_CREATE, &createreq);
   if (ret != 0)
            pquery->perfmon->kperfmon_id = createreq.id;
   pquery->perfmon->job_submitted = false;
            /* Ensure all pending jobs are flushed before activating the
      * perfmon
      v3d_flush((struct pipe_context *)v3d);
            }
      static bool
   v3d_end_query_perfcnt(struct v3d_context *v3d, struct v3d_query *query)
   {
                           if (v3d->active_perfmon != pquery->perfmon) {
                        /* Ensure all pending jobs are flushed before deactivating the
      * perfmon
               /* Get a copy of latest submitted job's fence to wait for its
      * completion
      if (v3d->active_perfmon->job_submitted)
                     }
      static bool
   v3d_get_query_result_perfcnt(struct v3d_context *v3d, struct v3d_query *query,
         {
         struct v3d_query_perfcnt *pquery = (struct v3d_query_perfcnt *)query;
   struct drm_v3d_perfmon_get_values req = { 0 };
                     if (pquery->perfmon->job_submitted) {
            if (!v3d_fence_wait(v3d->screen,
                        req.id = pquery->perfmon->kperfmon_id;
   req.values_ptr = (uintptr_t)pquery->perfmon->values;
   ret = v3d_ioctl(v3d->fd, DRM_IOCTL_V3D_PERFMON_GET_VALUES, &req);
   if (ret != 0) {
                     for (i = 0; i < pquery->num_queries; i++)
            }
      static const struct v3d_query_funcs perfcnt_query_funcs = {
         .destroy_query = v3d_destroy_query_perfcnt,
   .begin_query = v3d_begin_query_perfcnt,
   .end_query = v3d_end_query_perfcnt,
   };
      struct pipe_query *
   v3dX(create_batch_query_perfcnt)(struct v3d_context *v3d, unsigned num_queries,
         {
         struct v3d_query_perfcnt *pquery = NULL;
   struct v3d_query *query;
   struct v3d_perfmon_state *perfmon = NULL;
            /* Validate queries */
   for (i = 0; i < num_queries; i++) {
            if (query_types[i] < PIPE_QUERY_DRIVER_SPECIFIC ||
      query_types[i] >= PIPE_QUERY_DRIVER_SPECIFIC +
   ARRAY_SIZE(v3d_performance_counters)) {
                  pquery = calloc(1, sizeof(*pquery));
   if (!pquery)
            perfmon = calloc(1, sizeof(*perfmon));
   if (!perfmon) {
                        for (i = 0; i < num_queries; i++)
            pquery->perfmon = perfmon;
            query = &pquery->base;
            /* Note that struct pipe_query isn't actually defined anywhere. */
   }
