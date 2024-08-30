   /*
   * Copyright © 2012-2017 Intel Corporation
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
   * \file performance_query.c
   * Core Mesa support for the INTEL_performance_query extension.
   */
      #include <stdbool.h>
   #include "util/glheader.h"
   #include "context.h"
   #include "enums.h"
   #include "hash.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "performance_query.h"
   #include "util/ralloc.h"
   #include "api_exec_decl.h"
      #include "pipe/p_context.h"
      #include "state_tracker/st_cb_flush.h"
      void
   _mesa_init_performance_queries(struct gl_context *ctx)
   {
         }
      static void
   free_performance_query(void *data, void *user)
   {
      struct gl_perf_query_object *m = data;
            /* Don't confuse the implementation by deleting an active query. We can
   * toggle Active/Used to false because we're tearing down the GL context
   * and it's already idle (see _mesa_free_context_data).
   */
   m->Active = false;
   m->Used = false;
      }
      void
   _mesa_free_performance_queries(struct gl_context *ctx)
   {
      _mesa_HashDeleteAll(ctx->PerfQuery.Objects,
            }
      static inline struct gl_perf_query_object *
   lookup_object(struct gl_context *ctx, GLuint id)
   {
         }
      static GLuint
   init_performance_query_info(struct gl_context *ctx)
   {
         }
      /* For INTEL_performance_query, query id 0 is reserved to be invalid. */
   static inline unsigned
   queryid_to_index(GLuint queryid)
   {
         }
      static inline GLuint
   index_to_queryid(unsigned index)
   {
         }
      static inline bool
   queryid_valid(const struct gl_context *ctx, unsigned numQueries, GLuint queryid)
   {
      /* The GL_INTEL_performance_query spec says:
   *
   *  "Performance counter ids values start with 1. Performance counter id 0
   *  is reserved as an invalid counter."
   */
      }
      static inline GLuint
   counterid_to_index(GLuint counterid)
   {
         }
      static void
   output_clipped_string(GLchar *stringRet,
               {
      if (!stringRet)
                     /* No specification given about whether returned strings needs
   * to be zero-terminated. Zero-terminate the string always as we
   * don't otherwise communicate the length of the returned
   * string.
   */
   if (stringMaxLen > 0)
      }
      /*****************************************************************************/
      extern void GLAPIENTRY
   _mesa_GetFirstPerfQueryIdINTEL(GLuint *queryId)
   {
                        /* The GL_INTEL_performance_query spec says:
   *
   *    "If queryId pointer is equal to 0, INVALID_VALUE error is generated."
   */
   if (!queryId) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                              /* The GL_INTEL_performance_query spec says:
   *
   *    "If the given hardware platform doesn't support any performance
   *    queries, then the value of 0 is returned and INVALID_OPERATION error
   *    is raised."
   */
   if (numQueries == 0) {
      *queryId = 0;
   _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
      extern void GLAPIENTRY
   _mesa_GetNextPerfQueryIdINTEL(GLuint queryId, GLuint *nextQueryId)
   {
                        /* The GL_INTEL_performance_query spec says:
   *
   *    "The result is passed in location pointed by nextQueryId. If query
   *    identified by queryId is the last query available the value of 0 is
   *    returned. If the specified performance query identifier is invalid
   *    then INVALID_VALUE error is generated. If nextQueryId pointer is
   *    equal to 0, an INVALID_VALUE error is generated.  Whenever error is
   *    generated, the value of 0 is returned."
            if (!nextQueryId) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                              if (!queryid_valid(ctx, numQueries, queryId)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (queryid_valid(ctx, numQueries, ++queryId))
         else
      }
      extern void GLAPIENTRY
   _mesa_GetPerfQueryIdByNameINTEL(char *queryName, GLuint *queryId)
   {
               unsigned numQueries;
            /* The GL_INTEL_performance_query spec says:
   *
   *    "If queryName does not reference a valid query name, an INVALID_VALUE
   *    error is generated."
   */
   if (!queryName) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* The specification does not state that this produces an error but
   * to be consistent with glGetFirstPerfQueryIdINTEL we generate an
   * INVALID_VALUE error
   */
   if (!queryId) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                              for (i = 0; i < numQueries; ++i) {
      const GLchar *name;
            ctx->pipe->get_intel_perf_query_info(ctx->pipe, i, &name,
            if (strcmp(name, queryName) == 0) {
      *queryId = index_to_queryid(i);
                  _mesa_error(ctx, GL_INVALID_VALUE,
      }
      extern void GLAPIENTRY
   _mesa_GetPerfQueryInfoINTEL(GLuint queryId,
                                 {
               unsigned numQueries = init_performance_query_info(ctx);
   unsigned queryIndex = queryid_to_index(queryId);
   const char *queryName;
   GLuint queryDataSize;
   GLuint queryNumCounters;
            if (!queryid_valid(ctx, numQueries, queryId)) {
      /* The GL_INTEL_performance_query spec says:
   *
   *    "If queryId does not reference a valid query type, an
   *    INVALID_VALUE error is generated."
   */
   _mesa_error(ctx, GL_INVALID_VALUE,
                     ctx->pipe->get_intel_perf_query_info(ctx->pipe, queryIndex, &queryName,
                           if (dataSize)
            if (numCounters)
            /* The GL_INTEL_performance_query spec says:
   *
   *    "-- the actual number of already created query instances in
   *    maxInstances location"
   *
   * 1) Typo in the specification, should be noActiveInstances.
   * 2) Another typo in the specification, maxInstances parameter is not listed
   *    in the declaration of this function in the list of new functions.
   */
   if (numActive)
            /* Assume for now that all queries are per-context */
   if (capsMask)
      }
      static uint32_t
   pipe_counter_type_enum_to_gl_type(enum pipe_perf_counter_type type)
   {
      switch (type) {
   case PIPE_PERF_COUNTER_TYPE_EVENT: return GL_PERFQUERY_COUNTER_EVENT_INTEL;
   case PIPE_PERF_COUNTER_TYPE_DURATION_NORM: return GL_PERFQUERY_COUNTER_DURATION_NORM_INTEL;
   case PIPE_PERF_COUNTER_TYPE_DURATION_RAW: return GL_PERFQUERY_COUNTER_DURATION_RAW_INTEL;
   case PIPE_PERF_COUNTER_TYPE_THROUGHPUT: return GL_PERFQUERY_COUNTER_THROUGHPUT_INTEL;
   case PIPE_PERF_COUNTER_TYPE_RAW: return GL_PERFQUERY_COUNTER_RAW_INTEL;
   case PIPE_PERF_COUNTER_TYPE_TIMESTAMP: return GL_PERFQUERY_COUNTER_TIMESTAMP_INTEL;
   default:
            }
      static uint32_t
   pipe_counter_data_type_to_gl_type(enum pipe_perf_counter_data_type type)
   {
      switch (type) {
   case PIPE_PERF_COUNTER_DATA_TYPE_BOOL32: return GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL;
   case PIPE_PERF_COUNTER_DATA_TYPE_UINT32: return GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL;
   case PIPE_PERF_COUNTER_DATA_TYPE_UINT64: return GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL;
   case PIPE_PERF_COUNTER_DATA_TYPE_FLOAT: return GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL;
   case PIPE_PERF_COUNTER_DATA_TYPE_DOUBLE: return GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL;
   default:
            }
      static void
   get_perf_counter_info(struct gl_context *ctx,
                        unsigned query_index,
   unsigned counter_index,
   const char **name,
   const char **desc,
   GLuint *offset,
      {
      struct pipe_context *pipe = ctx->pipe;
   uint32_t pipe_type_enum;
            pipe->get_intel_perf_query_counter_info(pipe, query_index, counter_index,
               *type_enum = pipe_counter_type_enum_to_gl_type(pipe_type_enum);
      }
      extern void GLAPIENTRY
   _mesa_GetPerfCounterInfoINTEL(GLuint queryId, GLuint counterId,
                                 GLuint nameLength, GLchar *name,
   GLuint descLength, GLchar *desc,
   {
               unsigned numQueries = init_performance_query_info(ctx);
   unsigned queryIndex = queryid_to_index(queryId);
   const char *queryName;
   GLuint queryDataSize;
   GLuint queryNumCounters;
   GLuint queryNumActive;
   unsigned counterIndex;
   const char *counterName;
   const char *counterDesc;
   GLuint counterOffset;
   GLuint counterDataSize;
   GLuint counterTypeEnum;
   GLuint counterDataTypeEnum;
            if (!queryid_valid(ctx, numQueries, queryId)) {
      /* The GL_INTEL_performance_query spec says:
   *
   *    "If the pair of queryId and counterId does not reference a valid
   *    counter, an INVALID_VALUE error is generated."
   */
   _mesa_error(ctx, GL_INVALID_VALUE,
                     ctx->pipe->get_intel_perf_query_info(ctx->pipe, queryIndex, &queryName,
                           if (counterIndex >= queryNumCounters) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     get_perf_counter_info(ctx, queryIndex, counterIndex,
                        &counterName,
   &counterDesc,
   &counterOffset,
         output_clipped_string(name, nameLength, counterName);
            if (offset)
            if (dataSize)
            if (typeEnum)
            if (dataTypeEnum)
            if (rawCounterMaxValue)
            if (rawCounterMaxValue) {
      /* The GL_INTEL_performance_query spec says:
   *
   *    "for some raw counters for which the maximal value is
   *    deterministic, the maximal value of the counter in 1 second is
   *    returned in the location pointed by rawCounterMaxValue, otherwise,
   *    the location is written with the value of 0."
   *
   *    Since it's very useful to be able to report a maximum value for
   *    more that just counters using the _COUNTER_RAW_INTEL or
   *    _COUNTER_DURATION_RAW_INTEL enums (e.g. for a _THROUGHPUT tools
   *    want to be able to visualize the absolute throughput with respect
   *    to the theoretical maximum that's possible) and there doesn't seem
   *    to be any reason not to allow _THROUGHPUT counters to also be
   *    considerer "raw" here, we always leave it up to the backend to
   *    decide when it's appropriate to report a maximum counter value or 0
   *    if not.
   */
         }
      extern void GLAPIENTRY
   _mesa_CreatePerfQueryINTEL(GLuint queryId, GLuint *queryHandle)
   {
               unsigned numQueries = init_performance_query_info(ctx);
   GLuint id;
            /* The GL_INTEL_performance_query spec says:
   *
   *    "If queryId does not reference a valid query type, an INVALID_VALUE
   *    error is generated."
   */
   if (!queryid_valid(ctx, numQueries, queryId)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* This is not specified in the extension, but is the only sane thing to
   * do.
   */
   if (queryHandle == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     id = _mesa_HashFindFreeKeyBlock(ctx->PerfQuery.Objects, 1);
   if (!id) {
      /* The GL_INTEL_performance_query spec says:
   *
   *    "If the query instance cannot be created due to exceeding the
   *    number of allowed instances or driver fails query creation due to
   *    an insufficient memory reason, an OUT_OF_MEMORY error is
   *    generated, and the location pointed by queryHandle returns NULL."
   */
   _mesa_error_no_memory(__func__);
               obj = (struct gl_perf_query_object *)ctx->pipe->new_intel_perf_query_obj(ctx->pipe,
         if (obj == NULL) {
      _mesa_error_no_memory(__func__);
               obj->Id = id;
   obj->Active = false;
            _mesa_HashInsert(ctx->PerfQuery.Objects, id, obj, true);
      }
      extern void GLAPIENTRY
   _mesa_DeletePerfQueryINTEL(GLuint queryHandle)
   {
                        /* The GL_INTEL_performance_query spec says:
   *
   *    "If a query handle doesn't reference a previously created performance
   *    query instance, an INVALID_VALUE error is generated."
   */
   if (obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* To avoid complications in the backend we never ask the backend to
   * delete an active query or a query object while we are still
   * waiting for data.
            if (obj->Active)
            if (obj->Used && !obj->Ready) {
      ctx->pipe->wait_intel_perf_query(ctx->pipe, (struct pipe_query *)obj);
               _mesa_HashRemove(ctx->PerfQuery.Objects, queryHandle);
      }
      extern void GLAPIENTRY
   _mesa_BeginPerfQueryINTEL(GLuint queryHandle)
   {
                        /* The GL_INTEL_performance_query spec says:
   *
   *    "If a query handle doesn't reference a previously created performance
   *    query instance, an INVALID_VALUE error is generated."
   */
   if (obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* The GL_INTEL_performance_query spec says:
   *
   *    "Note that some query types, they cannot be collected in the same
   *    time. Therefore calls of BeginPerfQueryINTEL() cannot be nested if
   *    they refer to queries of such different types. In such case
   *    INVALID_OPERATION error is generated."
   *
   * We also generate an INVALID_OPERATION error if the driver can't begin
   * a query for its own reasons, and for nesting the same query.
   */
   if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* To avoid complications in the backend we never ask the backend to
   * reuse a query object and begin a new query while we are still
   * waiting for data on that object.
   */
   if (obj->Used && !obj->Ready) {
      ctx->pipe->wait_intel_perf_query(ctx->pipe, (struct pipe_query *)obj);
               if (ctx->pipe->begin_intel_perf_query(ctx->pipe, (struct pipe_query *)obj)) {
      obj->Used = true;
   obj->Active = true;
      } else {
      _mesa_error(ctx, GL_INVALID_OPERATION,
         }
      extern void GLAPIENTRY
   _mesa_EndPerfQueryINTEL(GLuint queryHandle)
   {
                        /* Not explicitly covered in the spec, but for consistency... */
   if (obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* The GL_INTEL_performance_query spec says:
   *
   *    "If a performance query is not currently started, an
   *    INVALID_OPERATION error will be generated."
            if (!obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                              obj->Active = false;
      }
      extern void GLAPIENTRY
   _mesa_GetPerfQueryDataINTEL(GLuint queryHandle, GLuint flags,
         {
                        /* Not explicitly covered in the spec, but for consistency... */
   if (obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* The GL_INTEL_performance_query spec says:
   *
   *    "If bytesWritten or data pointers are NULL then an INVALID_VALUE
   *    error is generated."
   */
   if (!bytesWritten || !data) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* Just for good measure in case a lazy application is only
   * checking this and not checking for errors...
   */
            /* Not explicitly covered in the spec but a query that was never started
   * cannot return any data.
   */
   if (!obj->Used) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* Not explicitly covered in the spec but to be consistent with
   * EndPerfQuery which validates that an application only ends an
   * active query we also validate that an application doesn't try
   * and get the data for a still active query...
   */
   if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (!obj->Ready)
      obj->Ready = ctx->pipe->is_intel_perf_query_ready(ctx->pipe,
         if (!obj->Ready) {
      if (flags == GL_PERFQUERY_FLUSH_INTEL) {
         } else if (flags == GL_PERFQUERY_WAIT_INTEL) {
      ctx->pipe->wait_intel_perf_query(ctx->pipe, (struct pipe_query *)obj);
                  if (obj->Ready) {
      if (!ctx->pipe->get_intel_perf_query_data(ctx->pipe, (struct pipe_query *)obj,
                           _mesa_error(ctx, GL_INVALID_OPERATION,
            }
