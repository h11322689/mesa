   /*
   * Copyright © 2012 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * \file performance_monitor.c
   * Core Mesa support for the AMD_performance_monitor extension.
   *
   * In order to implement this extension, start by defining two enums:
   * one for Groups, and one for Counters.  These will be used as indexes into
   * arrays, so they should start at 0 and increment from there.
   *
   * Counter IDs need to be globally unique.  That is, you can't have counter 7
   * in group A and counter 7 in group B.  A global enum of all available
   * counters is a convenient way to guarantee this.
   */
      #include <stdbool.h>
   #include "util/glheader.h"
   #include "context.h"
   #include "enums.h"
   #include "hash.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "performance_monitor.h"
   #include "util/bitset.h"
   #include "util/ralloc.h"
   #include "util/u_memory.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_bitmap.h"
   #include "state_tracker/st_context.h"
   #include "state_tracker/st_debug.h"
      #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
      void
   _mesa_init_performance_monitors(struct gl_context *ctx)
   {
      ctx->PerfMonitor.Monitors = _mesa_NewHashTable();
   ctx->PerfMonitor.NumGroups = 0;
      }
         static bool
   init_perf_monitor(struct gl_context *ctx, struct gl_perf_monitor_object *m)
   {
      struct pipe_context *pipe = ctx->pipe;
   unsigned *batch = NULL;
   unsigned num_active_counters = 0;
   unsigned max_batch_counters = 0;
   unsigned num_batch_counters = 0;
                     /* Determine the number of active counters. */
   for (gid = 0; gid < ctx->PerfMonitor.NumGroups; gid++) {
               if (m->ActiveGroups[gid] > g->MaxActiveCounters) {
      /* Maximum number of counters reached. Cannot start the session. */
   if (ST_DEBUG & DEBUG_MESA) {
      debug_printf("Maximum number of counters reached. "
      }
               num_active_counters += m->ActiveGroups[gid];
   if (g->has_batch)
               if (!num_active_counters)
            m->active_counters = CALLOC(num_active_counters,
         if (!m->active_counters)
            if (max_batch_counters) {
      batch = CALLOC(max_batch_counters, sizeof(*batch));
   if (!batch)
               /* Create a query for each active counter. */
   for (gid = 0; gid < ctx->PerfMonitor.NumGroups; gid++) {
               BITSET_FOREACH_SET(cid, m->ActiveCounters[gid], g->NumCounters) {
      const struct gl_perf_monitor_counter *c = &g->Counters[cid];
                  cntr->id       = cid;
   cntr->group_id = gid;
   if (c->flags & PIPE_DRIVER_QUERY_FLAG_BATCH) {
      cntr->batch_index = num_batch_counters;
      } else {
      cntr->query = pipe->create_query(pipe, c->query_type, 0);
   if (!cntr->query)
      }
                  /* Create the batch query. */
   if (num_batch_counters) {
      m->batch_query = pipe->create_batch_query(pipe, num_batch_counters,
         m->batch_result = CALLOC(num_batch_counters, sizeof(m->batch_result->batch[0]));
   if (!m->batch_query || !m->batch_result)
               FREE(batch);
         fail:
      FREE(batch);
      }
      static void
   do_reset_perf_monitor(struct gl_perf_monitor_object *m,
         {
               for (i = 0; i < m->num_active_counters; ++i) {
      struct pipe_query *query = m->active_counters[i].query;
   if (query)
      }
   FREE(m->active_counters);
   m->active_counters = NULL;
            if (m->batch_query) {
      pipe->destroy_query(pipe, m->batch_query);
      }
   FREE(m->batch_result);
      }
      static void
   delete_perf_monitor(struct gl_context *ctx, struct gl_perf_monitor_object *m)
   {
               do_reset_perf_monitor(m, pipe);
      }
      static GLboolean
   begin_perf_monitor(struct gl_context *ctx, struct gl_perf_monitor_object *m)
   {
      struct pipe_context *pipe = st_context(ctx)->pipe;
            if (!m->num_active_counters) {
      /* Create a query for each active counter before starting
   * a new monitoring session. */
   if (!init_perf_monitor(ctx, m))
               /* Start the query for each active counter. */
   for (i = 0; i < m->num_active_counters; ++i) {
      struct pipe_query *query = m->active_counters[i].query;
   if (query && !pipe->begin_query(pipe, query))
               if (m->batch_query && !pipe->begin_query(pipe, m->batch_query))
                  fail:
      /* Failed to start the monitoring session. */
   do_reset_perf_monitor(m, pipe);
      }
      static void
   end_perf_monitor(struct gl_context *ctx, struct gl_perf_monitor_object *m)
   {
      struct pipe_context *pipe = st_context(ctx)->pipe;
            /* Stop the query for each active counter. */
   for (i = 0; i < m->num_active_counters; ++i) {
      struct pipe_query *query = m->active_counters[i].query;
   if (query)
               if (m->batch_query)
      }
      static void
   reset_perf_monitor(struct gl_context *ctx, struct gl_perf_monitor_object *m)
   {
               if (!m->Ended)
                     if (m->Active)
      }
      static GLboolean
   is_perf_monitor_result_available(struct gl_context *ctx,
         {
      struct pipe_context *pipe = st_context(ctx)->pipe;
            if (!m->num_active_counters)
            /* The result of a monitoring session is only available if the query of
   * each active counter is idle. */
   for (i = 0; i < m->num_active_counters; ++i) {
      struct pipe_query *query = m->active_counters[i].query;
   union pipe_query_result result;
   if (query && !pipe->get_query_result(pipe, query, false, &result)) {
      /* The query is busy. */
                  if (m->batch_query &&
      !pipe->get_query_result(pipe, m->batch_query, false, m->batch_result))
            }
      static void
   get_perf_monitor_result(struct gl_context *ctx,
                           {
      struct pipe_context *pipe = st_context(ctx)->pipe;
            /* Copy data to the supplied array (data).
   *
   * The output data format is: <group ID, counter ID, value> for each
   * active counter. The API allows counters to appear in any order.
   */
   GLsizei offset = 0;
            if (m->batch_query)
      have_batch_query = pipe->get_query_result(pipe, m->batch_query, true,
         /* Read query results for each active counter. */
   for (i = 0; i < m->num_active_counters; ++i) {
      struct gl_perf_counter_object *cntr = &m->active_counters[i];
   union pipe_query_result result = { 0 };
   int gid, cid;
            cid  = cntr->id;
   gid  = cntr->group_id;
            if (cntr->query) {
      if (!pipe->get_query_result(pipe, cntr->query, true, &result))
      } else {
      if (!have_batch_query)
                     data[offset++] = gid;
   data[offset++] = cid;
   switch (type) {
   case GL_UNSIGNED_INT64_AMD:
      memcpy(&data[offset], &result.u64, sizeof(uint64_t));
   offset += sizeof(uint64_t) / sizeof(GLuint);
      case GL_UNSIGNED_INT:
      memcpy(&data[offset], &result.u32, sizeof(uint32_t));
   offset += sizeof(uint32_t) / sizeof(GLuint);
      case GL_FLOAT:
   case GL_PERCENTAGE_AMD:
      memcpy(&data[offset], &result.f, sizeof(GLfloat));
   offset += sizeof(GLfloat) / sizeof(GLuint);
                  if (bytesWritten)
      }
      void
   _mesa_free_perfomance_monitor_groups(struct gl_context *ctx)
   {
      struct gl_perf_monitor_state *perfmon = &ctx->PerfMonitor;
            for (gid = 0; gid < perfmon->NumGroups; gid++) {
         }
      }
      static inline void
   init_groups(struct gl_context *ctx)
   {
      if (likely(ctx->PerfMonitor.Groups))
            struct gl_perf_monitor_state *perfmon = &ctx->PerfMonitor;
   struct pipe_screen *screen = ctx->pipe->screen;
   struct gl_perf_monitor_group *groups = NULL;
   int num_counters, num_groups;
            /* Get the number of available queries. */
            /* Get the number of available groups. */
   num_groups = screen->get_driver_query_group_info(screen, 0, NULL);
   groups = CALLOC(num_groups, sizeof(*groups));
   if (!groups)
            for (gid = 0; gid < num_groups; gid++) {
      struct gl_perf_monitor_group *g = &groups[perfmon->NumGroups];
   struct pipe_driver_query_group_info group_info;
            if (!screen->get_driver_query_group_info(screen, gid, &group_info))
            g->Name = group_info.name;
            if (group_info.num_queries)
         if (!counters)
                  for (cid = 0; cid < num_counters; cid++) {
                     if (!screen->get_driver_query_info(screen, cid, &info))
                        c->Name = info.name;
   switch (info.type) {
      case PIPE_DRIVER_QUERY_TYPE_UINT64:
   case PIPE_DRIVER_QUERY_TYPE_BYTES:
   case PIPE_DRIVER_QUERY_TYPE_MICROSECONDS:
   case PIPE_DRIVER_QUERY_TYPE_HZ:
      c->Minimum.u64 = 0;
   c->Maximum.u64 = info.max_value.u64 ? info.max_value.u64 : UINT64_MAX;
   c->Type = GL_UNSIGNED_INT64_AMD;
      case PIPE_DRIVER_QUERY_TYPE_UINT:
      c->Minimum.u32 = 0;
   c->Maximum.u32 = info.max_value.u32 ? info.max_value.u32 : UINT32_MAX;
   c->Type = GL_UNSIGNED_INT;
      case PIPE_DRIVER_QUERY_TYPE_FLOAT:
      c->Minimum.f = 0.0;
   c->Maximum.f = info.max_value.f ? info.max_value.f : FLT_MAX;
   c->Type = GL_FLOAT;
      case PIPE_DRIVER_QUERY_TYPE_PERCENTAGE:
      c->Minimum.f = 0.0f;
   c->Maximum.f = 100.0f;
   c->Type = GL_PERCENTAGE_AMD;
      default:
               c->query_type = info.query_type;
   c->flags = info.flags;
                     }
      }
                  fail:
      for (gid = 0; gid < num_groups; gid++) {
         }
      }
      static struct gl_perf_monitor_object *
   new_performance_monitor(struct gl_context *ctx, GLuint index)
   {
      unsigned i;
            if (m == NULL)
                              m->ActiveGroups =
            m->ActiveCounters =
            if (m->ActiveGroups == NULL || m->ActiveCounters == NULL)
            for (i = 0; i < ctx->PerfMonitor.NumGroups; i++) {
               m->ActiveCounters[i] = rzalloc_array(m->ActiveCounters, BITSET_WORD,
         if (m->ActiveCounters[i] == NULL)
                     fail:
      ralloc_free(m->ActiveGroups);
   ralloc_free(m->ActiveCounters);
   delete_perf_monitor(ctx, m);
      }
      static void
   free_performance_monitor(void *data, void *user)
   {
      struct gl_perf_monitor_object *m = data;
            ralloc_free(m->ActiveGroups);
   ralloc_free(m->ActiveCounters);
      }
      void
   _mesa_free_performance_monitors(struct gl_context *ctx)
   {
      _mesa_HashDeleteAll(ctx->PerfMonitor.Monitors,
            }
      static inline struct gl_perf_monitor_object *
   lookup_monitor(struct gl_context *ctx, GLuint id)
   {
      return (struct gl_perf_monitor_object *)
      }
      static inline const struct gl_perf_monitor_group *
   get_group(const struct gl_context *ctx, GLuint id)
   {
      if (id >= ctx->PerfMonitor.NumGroups)
               }
      static inline const struct gl_perf_monitor_counter *
   get_counter(const struct gl_perf_monitor_group *group_obj, GLuint id)
   {
      if (id >= group_obj->NumCounters)
               }
      /*****************************************************************************/
      void GLAPIENTRY
   _mesa_GetPerfMonitorGroupsAMD(GLint *numGroups, GLsizei groupsSize,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (numGroups != NULL)
            if (groupsSize > 0 && groups != NULL) {
      unsigned i;
            /* We just use the index in the Groups array as the ID. */
   for (i = 0; i < n; i++)
         }
      void GLAPIENTRY
   _mesa_GetPerfMonitorCountersAMD(GLuint group, GLint *numCounters,
               {
      GET_CURRENT_CONTEXT(ctx);
                     group_obj = get_group(ctx, group);
   if (group_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (maxActiveCounters != NULL)
            if (numCounters != NULL)
            if (counters != NULL) {
      unsigned i;
   unsigned n = MIN2(group_obj->NumCounters, (GLuint) countersSize);
   for (i = 0; i < n; i++) {
      /* We just use the index in the Counters array as the ID. */
            }
      void GLAPIENTRY
   _mesa_GetPerfMonitorGroupStringAMD(GLuint group, GLsizei bufSize,
         {
      GET_CURRENT_CONTEXT(ctx);
                     group_obj = get_group(ctx, group);
   if (group_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetPerfMonitorGroupStringAMD");
               if (bufSize == 0) {
      /* Return the number of characters that would be required to hold the
   * group string, excluding the null terminator.
   */
   if (length != NULL)
      } else {
      if (length != NULL)
         if (groupString != NULL)
         }
      void GLAPIENTRY
   _mesa_GetPerfMonitorCounterStringAMD(GLuint group, GLuint counter,
               {
               const struct gl_perf_monitor_group *group_obj;
                              if (group_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                              if (counter_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (bufSize == 0) {
      /* Return the number of characters that would be required to hold the
   * counter string, excluding the null terminator.
   */
   if (length != NULL)
      } else {
      if (length != NULL)
         if (counterString != NULL)
         }
      void GLAPIENTRY
   _mesa_GetPerfMonitorCounterInfoAMD(GLuint group, GLuint counter, GLenum pname,
         {
               const struct gl_perf_monitor_group *group_obj;
                              if (group_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                              if (counter_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     switch (pname) {
   case GL_COUNTER_TYPE_AMD:
      *((GLenum *) data) = counter_obj->Type;
         case GL_COUNTER_RANGE_AMD:
      switch (counter_obj->Type) {
   case GL_FLOAT:
   case GL_PERCENTAGE_AMD: {
      float *f_data = data;
   f_data[0] = counter_obj->Minimum.f;
   f_data[1] = counter_obj->Maximum.f;
      }
   case GL_UNSIGNED_INT: {
      uint32_t *u32_data = data;
   u32_data[0] = counter_obj->Minimum.u32;
   u32_data[1] = counter_obj->Maximum.u32;
      }
   case GL_UNSIGNED_INT64_AMD: {
      uint64_t *u64_data = data;
   u64_data[0] = counter_obj->Minimum.u64;
   u64_data[1] = counter_obj->Maximum.u64;
      }
   default:
         }
         default:
      _mesa_error(ctx, GL_INVALID_ENUM,
               }
      void GLAPIENTRY
   _mesa_GenPerfMonitorsAMD(GLsizei n, GLuint *monitors)
   {
               if (MESA_VERBOSE & VERBOSE_API)
                     if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenPerfMonitorsAMD(n < 0)");
               if (monitors == NULL)
            if (_mesa_HashFindFreeKeys(ctx->PerfMonitor.Monitors, monitors, n)) {
      GLsizei i;
   for (i = 0; i < n; i++) {
      struct gl_perf_monitor_object *m =
         if (!m) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenPerfMonitorsAMD");
      }
         } else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenPerfMonitorsAMD");
         }
      void GLAPIENTRY
   _mesa_DeletePerfMonitorsAMD(GLsizei n, GLuint *monitors)
   {
      GLint i;
            if (MESA_VERBOSE & VERBOSE_API)
            if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeletePerfMonitorsAMD(n < 0)");
               if (monitors == NULL)
            for (i = 0; i < n; i++) {
               if (m) {
      /* Give the driver a chance to stop the monitor if it's active. */
   if (m->Active) {
      reset_perf_monitor(ctx, m);
               _mesa_HashRemove(ctx->PerfMonitor.Monitors, monitors[i]);
   ralloc_free(m->ActiveGroups);
   ralloc_free(m->ActiveCounters);
      } else {
      /* "INVALID_VALUE error will be generated if any of the monitor IDs
   *  in the <monitors> parameter to DeletePerfMonitorsAMD do not
   *  reference a valid generated monitor ID."
   */
   _mesa_error(ctx, GL_INVALID_VALUE,
            }
      void GLAPIENTRY
   _mesa_SelectPerfMonitorCountersAMD(GLuint monitor, GLboolean enable,
               {
      GET_CURRENT_CONTEXT(ctx);
   int i;
   struct gl_perf_monitor_object *m;
                     /* "INVALID_VALUE error will be generated if the <monitor> parameter to
   *  SelectPerfMonitorCountersAMD does not reference a monitor created by
   *  GenPerfMonitorsAMD."
   */
   if (m == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                              /* "INVALID_VALUE error will be generated if the <group> parameter to
   *  GetPerfMonitorCountersAMD, GetPerfMonitorCounterStringAMD,
   *  GetPerfMonitorCounterStringAMD, GetPerfMonitorCounterInfoAMD, or
   *  SelectPerfMonitorCountersAMD does not reference a valid group ID."
   */
   if (group_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* "INVALID_VALUE error will be generated if the <numCounters> parameter to
   *  SelectPerfMonitorCountersAMD is less than 0."
   */
   if (numCounters < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* "When SelectPerfMonitorCountersAMD is called on a monitor, any outstanding
   *  results for that monitor become invalidated and the result queries
   *  PERFMON_RESULT_SIZE_AMD and PERFMON_RESULT_AVAILABLE_AMD are reset to 0."
   */
            /* Sanity check the counter ID list. */
   for (i = 0; i < numCounters; i++) {
      if (counterList[i] >= group_obj->NumCounters) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                        if (enable) {
      /* Enable the counters */
   for (i = 0; i < numCounters; i++) {
      if (!BITSET_TEST(m->ActiveCounters[group], counterList[i])) {
      ++m->ActiveGroups[group];
            } else {
      /* Disable the counters */
   for (i = 0; i < numCounters; i++) {
      if (BITSET_TEST(m->ActiveCounters[group], counterList[i])) {
      --m->ActiveGroups[group];
               }
      void GLAPIENTRY
   _mesa_BeginPerfMonitorAMD(GLuint monitor)
   {
                        if (m == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* "INVALID_OPERATION error will be generated if BeginPerfMonitorAMD is
   *  called when a performance monitor is already active."
   */
   if (m->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* The driver is free to return false if it can't begin monitoring for
   * any reason.  This translates into an INVALID_OPERATION error.
   */
   if (begin_perf_monitor(ctx, m)) {
      m->Active = true;
      } else {
      _mesa_error(ctx, GL_INVALID_OPERATION,
         }
      void GLAPIENTRY
   _mesa_EndPerfMonitorAMD(GLuint monitor)
   {
                        if (m == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glEndPerfMonitorAMD(invalid monitor)");
               /* "INVALID_OPERATION error will be generated if EndPerfMonitorAMD is called
   *  when a performance monitor is not currently started."
   */
   if (!m->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEndPerfMonitor(not active)");
                        m->Active = false;
      }
      /**
   * Return the number of bytes needed to store a monitor's result.
   */
   static unsigned
   perf_monitor_result_size(const struct gl_context *ctx,
         {
      unsigned group, counter;
            for (group = 0; group < ctx->PerfMonitor.NumGroups; group++) {
               BITSET_FOREACH_SET(counter, m->ActiveCounters[group], g->NumCounters) {
               size += sizeof(uint32_t); /* Group ID */
   size += sizeof(uint32_t); /* Counter ID */
         }
      }
      void GLAPIENTRY
   _mesa_GetPerfMonitorCounterDataAMD(GLuint monitor, GLenum pname,
               {
               struct gl_perf_monitor_object *m = lookup_monitor(ctx, monitor);
            if (m == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* "It is an INVALID_OPERATION error for <data> to be NULL." */
   if (data == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* We need at least enough room for a single value. */
   if (dataSize < sizeof(GLuint)) {
      if (bytesWritten != NULL)
                     /* If the monitor has never ended, there is no result. */
   result_available = m->Ended &&
            /* AMD appears to return 0 for all queries unless a result is available. */
   if (!result_available) {
      *data = 0;
   if (bytesWritten != NULL)
                     switch (pname) {
   case GL_PERFMON_RESULT_AVAILABLE_AMD:
      *data = 1;
   if (bytesWritten != NULL)
            case GL_PERFMON_RESULT_SIZE_AMD:
      *data = perf_monitor_result_size(ctx, m);
   if (bytesWritten != NULL)
            case GL_PERFMON_RESULT_AMD:
      get_perf_monitor_result(ctx, m, dataSize, data, bytesWritten);
      default:
      _mesa_error(ctx, GL_INVALID_ENUM,
         }
      /**
   * Returns how many bytes a counter's value takes up.
   */
   unsigned
   _mesa_perf_monitor_counter_size(const struct gl_perf_monitor_counter *c)
   {
      switch (c->Type) {
   case GL_FLOAT:
   case GL_PERCENTAGE_AMD:
         case GL_UNSIGNED_INT:
         case GL_UNSIGNED_INT64_AMD:
         default:
      assert(!"Should not get here: invalid counter type");
         }
