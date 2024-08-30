   /*
   * Copyright © 2020 Google, Inc.
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
      #include "u_trace.h"
      #include <inttypes.h>
      #include "util/list.h"
   #include "util/u_call_once.h"
   #include "util/u_debug.h"
   #include "util/u_vector.h"
      #define __NEEDS_TRACE_PRIV
   #include "u_trace_priv.h"
      #define PAYLOAD_BUFFER_SIZE 0x100
   #define TIMESTAMP_BUF_SIZE 0x1000
   #define TRACES_PER_CHUNK (TIMESTAMP_BUF_SIZE / sizeof(uint64_t))
      struct u_trace_state {
      util_once_flag once;
   FILE *trace_file;
      };
   static struct u_trace_state u_trace_state = { .once = UTIL_ONCE_FLAG_INIT };
      #ifdef HAVE_PERFETTO
   /**
   * Global list of contexts, so we can defer starting the queue until
   * perfetto tracing is started.
   */
   static struct list_head ctx_list = { &ctx_list, &ctx_list };
      static simple_mtx_t ctx_list_mutex = SIMPLE_MTX_INITIALIZER;
   /* The amount of Perfetto tracers connected */
   int _u_trace_perfetto_count;
   #endif
      struct u_trace_payload_buf {
               uint8_t *buf;
   uint8_t *next;
      };
      struct u_trace_event {
      const struct u_tracepoint *tp;
      };
      /**
   * A "chunk" of trace-events and corresponding timestamp buffer.  As
   * trace events are emitted, additional trace chucks will be allocated
   * as needed.  When u_trace_flush() is called, they are transferred
   * from the u_trace to the u_trace_context queue.
   */
   struct u_trace_chunk {
                        /* The number of traces this chunk contains so far: */
            /* table of trace events: */
            /* table of driver recorded 64b timestamps, index matches index
   * into traces table
   */
            /* Array of u_trace_payload_buf referenced by traces[] elements.
   */
            /* Current payload buffer being written. */
                     bool last; /* this chunk is last in batch */
                     /**
   * Several chunks reference a single flush_data instance thus only
   * one chunk should be designated to free the data.
   */
      };
      struct u_trace_printer {
      void (*start)(struct u_trace_context *utctx);
   void (*end)(struct u_trace_context *utctx);
   void (*start_of_frame)(struct u_trace_context *utctx);
   void (*end_of_frame)(struct u_trace_context *utctx);
   void (*start_of_batch)(struct u_trace_context *utctx);
   void (*end_of_batch)(struct u_trace_context *utctx);
   void (*event)(struct u_trace_context *utctx,
               struct u_trace_chunk *chunk,
      };
      static void
   print_txt_start(struct u_trace_context *utctx)
   {
   }
      static void
   print_txt_end_of_frame(struct u_trace_context *utctx)
   {
         }
      static void
   print_txt_start_of_batch(struct u_trace_context *utctx)
   {
         }
      static void
   print_txt_end_of_batch(struct u_trace_context *utctx)
   {
      uint64_t elapsed = utctx->last_time_ns - utctx->first_time_ns;
      }
      static void
   print_txt_event(struct u_trace_context *utctx,
                  struct u_trace_chunk *chunk,
      {
      if (evt->tp->print) {
      fprintf(utctx->out, "%016" PRIu64 " %+9d: %s: ", ns, delta,
            } else {
      fprintf(utctx->out, "%016" PRIu64 " %+9d: %s\n", ns, delta,
         }
      static struct u_trace_printer txt_printer = {
      .start = &print_txt_start,
   .end = &print_txt_start,
   .start_of_frame = &print_txt_start,
   .end_of_frame = &print_txt_end_of_frame,
   .start_of_batch = &print_txt_start_of_batch,
   .end_of_batch = &print_txt_end_of_batch,
      };
      static void
   print_json_start(struct u_trace_context *utctx)
   {
         }
      static void
   print_json_end(struct u_trace_context *utctx)
   {
         }
      static void
   print_json_start_of_frame(struct u_trace_context *utctx)
   {
      if (utctx->frame_nr != 0)
         fprintf(utctx->out, "{\n\"frame\": %u,\n", utctx->frame_nr);
      }
      static void
   print_json_end_of_frame(struct u_trace_context *utctx)
   {
      fprintf(utctx->out, "]\n}\n");
      }
      static void
   print_json_start_of_batch(struct u_trace_context *utctx)
   {
      if (utctx->batch_nr != 0)
            }
      static void
   print_json_end_of_batch(struct u_trace_context *utctx)
   {
      uint64_t elapsed = utctx->last_time_ns - utctx->first_time_ns;
   fprintf(utctx->out, "],\n");
   fprintf(utctx->out, "\"duration_ns\": %" PRIu64 "\n", elapsed);
      }
      static void
   print_json_event(struct u_trace_context *utctx,
                  struct u_trace_chunk *chunk,
      {
      if (utctx->event_nr != 0)
         fprintf(utctx->out, "{\n\"event\": \"%s\",\n", evt->tp->name);
   fprintf(utctx->out, "\"time_ns\": \"%016" PRIu64 "\",\n", ns);
   fprintf(utctx->out, "\"params\": {");
   if (evt->tp->print)
            }
      static struct u_trace_printer json_printer = {
      .start = print_json_start,
   .end = print_json_end,
   .start_of_frame = &print_json_start_of_frame,
   .end_of_frame = &print_json_end_of_frame,
   .start_of_batch = &print_json_start_of_batch,
   .end_of_batch = &print_json_end_of_batch,
      };
      static struct u_trace_payload_buf *
   u_trace_payload_buf_create(void)
   {
      struct u_trace_payload_buf *payload =
                     payload->buf = (uint8_t *) (payload + 1);
   payload->end = payload->buf + PAYLOAD_BUFFER_SIZE;
               }
      static struct u_trace_payload_buf *
   u_trace_payload_buf_ref(struct u_trace_payload_buf *payload)
   {
      p_atomic_inc(&payload->refcount);
      }
      static void
   u_trace_payload_buf_unref(struct u_trace_payload_buf *payload)
   {
      if (p_atomic_dec_zero(&payload->refcount))
      }
      static void
   free_chunk(void *ptr)
   {
                        /* Unref payloads attached to this chunk. */
   struct u_trace_payload_buf **payload;
   u_vector_foreach (payload, &chunk->payloads)
                  list_del(&chunk->node);
      }
      static void
   free_chunks(struct list_head *chunks)
   {
      while (!list_is_empty(chunks)) {
      struct u_trace_chunk *chunk =
               }
      static struct u_trace_chunk *
   get_chunk(struct u_trace *ut, size_t payload_size)
   {
                        /* do we currently have a non-full chunk to append msgs to? */
   if (!list_is_empty(&ut->trace_chunks)) {
      chunk = list_last_entry(&ut->trace_chunks, struct u_trace_chunk, node);
   /* Can we store a new trace in the chunk? */
   if (chunk->num_traces < TRACES_PER_CHUNK) {
      /* If no payload required, nothing else to check. */
                  /* If the payload buffer has space for the payload, we're good.
   */
   if (chunk->payload &&
                  /* If we don't have enough space in the payload buffer, can we
   * allocate a new one?
   */
   struct u_trace_payload_buf **buf = u_vector_add(&chunk->payloads);
   *buf = u_trace_payload_buf_create();
   chunk->payload = *buf;
      }
   /* we need to expand to add another chunk to the batch, so
   * the current one is no longer the last one of the batch:
   */
               /* .. if not, then create a new one: */
            chunk->utctx = ut->utctx;
   chunk->timestamps =
         chunk->last = true;
   u_vector_init(&chunk->payloads, 4, sizeof(struct u_trace_payload_buf *));
   if (payload_size > 0) {
      struct u_trace_payload_buf **buf = u_vector_add(&chunk->payloads);
   *buf = u_trace_payload_buf_create();
                           }
      static const struct debug_named_value config_control[] = {
      { "print", U_TRACE_TYPE_PRINT, "Enable print" },
      #ifdef HAVE_PERFETTO
         #endif
      { "markers", U_TRACE_TYPE_MARKERS, "Enable marker trace" },
      };
      DEBUG_GET_ONCE_OPTION(trace_file, "MESA_GPU_TRACEFILE", NULL)
      static void
   trace_file_fini(void)
   {
      fclose(u_trace_state.trace_file);
      }
      static void
   u_trace_state_init_once(void)
   {
      u_trace_state.enabled_traces =
         const char *tracefile_name = debug_get_option_trace_file();
   if (tracefile_name && __normal_user()) {
      u_trace_state.trace_file = fopen(tracefile_name, "w");
   if (u_trace_state.trace_file != NULL) {
            }
   if (!u_trace_state.trace_file) {
            }
      void
   u_trace_state_init(void)
   {
         }
      bool
   u_trace_is_enabled(enum u_trace_type type)
   {
      /* Active is only tracked in a given u_trace context, so if you're asking
   * us if U_TRACE_TYPE_PERFETTO (_ENV | _ACTIVE) is enabled, then just check
   * _ENV ("perfetto tracing is desired, but perfetto might not be running").
   */
               }
      static void
   queue_init(struct u_trace_context *utctx)
   {
      if (utctx->queue.jobs)
            bool ret = util_queue_init(
      &utctx->queue, "traceq", 256, 1,
   UTIL_QUEUE_INIT_USE_MINIMUM_PRIORITY | UTIL_QUEUE_INIT_RESIZE_IF_FULL,
               if (!ret)
      }
      void
   u_trace_context_init(struct u_trace_context *utctx,
                        void *pctx,
   u_trace_create_ts_buffer create_timestamp_buffer,
      {
               utctx->enabled_traces = u_trace_state.enabled_traces;
   utctx->pctx = pctx;
   utctx->create_timestamp_buffer = create_timestamp_buffer;
   utctx->delete_timestamp_buffer = delete_timestamp_buffer;
   utctx->record_timestamp = record_timestamp;
   utctx->read_timestamp = read_timestamp;
            utctx->last_time_ns = 0;
   utctx->first_time_ns = 0;
   utctx->frame_nr = 0;
   utctx->batch_nr = 0;
   utctx->event_nr = 0;
                     if (utctx->enabled_traces & U_TRACE_TYPE_PRINT) {
               if (utctx->enabled_traces & U_TRACE_TYPE_JSON) {
         } else {
            } else {
      utctx->out = NULL;
            #ifdef HAVE_PERFETTO
      simple_mtx_lock(&ctx_list_mutex);
   list_add(&utctx->node, &ctx_list);
   if (_u_trace_perfetto_count > 0)
                        #else
         #endif
         if (!(p_atomic_read_relaxed(&utctx->enabled_traces) &
                  if (utctx->out) {
            }
      void
   u_trace_context_fini(struct u_trace_context *utctx)
   {
   #ifdef HAVE_PERFETTO
      simple_mtx_lock(&ctx_list_mutex);
   list_del(&utctx->node);
      #endif
         if (utctx->out) {
      utctx->out_printer->end(utctx);
               if (!utctx->queue.jobs)
         util_queue_finish(&utctx->queue);
   util_queue_destroy(&utctx->queue);
      }
      #ifdef HAVE_PERFETTO
   void
   u_trace_perfetto_start(void)
   {
               list_for_each_entry (struct u_trace_context, utctx, &ctx_list, node) {
      queue_init(utctx);
   p_atomic_set(&utctx->enabled_traces,
                           }
      void
   u_trace_perfetto_stop(void)
   {
               assert(_u_trace_perfetto_count > 0);
   _u_trace_perfetto_count--;
   if (_u_trace_perfetto_count == 0) {
      list_for_each_entry (struct u_trace_context, utctx, &ctx_list, node) {
      p_atomic_set(&utctx->enabled_traces,
                     }
   #endif
      static void
   process_chunk(void *job, void *gdata, int thread_index)
   {
      struct u_trace_chunk *chunk = job;
            if (utctx->start_of_frame) {
      utctx->start_of_frame = false;
   utctx->batch_nr = 0;
   if (utctx->out) {
                     /* For first chunk of batch, accumulated times will be zerod: */
   if (!utctx->last_time_ns) {
      utctx->event_nr = 0;
   if (utctx->out) {
                     for (unsigned idx = 0; idx < chunk->num_traces; idx++) {
               if (!evt->tp)
            uint64_t ns = utctx->read_timestamp(utctx, chunk->timestamps, idx,
                  if (!utctx->first_time_ns)
            if (ns != U_TRACE_NO_TIMESTAMP) {
      delta = utctx->last_time_ns ? ns - utctx->last_time_ns : 0;
      } else {
      /* we skipped recording the timestamp, so it should be
   * the same as last msg:
   */
   ns = utctx->last_time_ns;
               if (utctx->out) {
         #ifdef HAVE_PERFETTO
         if (evt->tp->perfetto &&
      (p_atomic_read_relaxed(&utctx->enabled_traces) &
   U_TRACE_TYPE_PERFETTO_ACTIVE)) {
      #endif
                        if (chunk->last) {
      if (utctx->out) {
                  utctx->batch_nr++;
   utctx->last_time_ns = 0;
               if (chunk->eof) {
      if (utctx->out) {
         }
   utctx->frame_nr++;
               if (chunk->free_flush_data && utctx->delete_flush_data) {
            }
      static void
   cleanup_chunk(void *job, void *gdata, int thread_index)
   {
         }
      void
   u_trace_context_process(struct u_trace_context *utctx, bool eof)
   {
               if (list_is_empty(chunks))
            struct u_trace_chunk *last_chunk =
                  while (!list_is_empty(chunks)) {
      struct u_trace_chunk *chunk =
            /* remove from list before enqueuing, because chunk is freed
   * once it is processed by the queue:
   */
            util_queue_add_job(&utctx->queue, chunk, &chunk->fence, process_chunk,
         }
      void
   u_trace_init(struct u_trace *ut, struct u_trace_context *utctx)
   {
      ut->utctx = utctx;
   ut->num_traces = 0;
      }
      void
   u_trace_fini(struct u_trace *ut)
   {
      /* Normally the list of trace-chunks would be empty, if they
   * have been flushed to the trace-context.
   */
   free_chunks(&ut->trace_chunks);
      }
      bool
   u_trace_has_points(struct u_trace *ut)
   {
         }
      struct u_trace_iterator
   u_trace_begin_iterator(struct u_trace *ut)
   {
      if (list_is_empty(&ut->trace_chunks))
            struct u_trace_chunk *first_chunk =
               }
      struct u_trace_iterator
   u_trace_end_iterator(struct u_trace *ut)
   {
      if (list_is_empty(&ut->trace_chunks))
            struct u_trace_chunk *last_chunk =
            return (struct u_trace_iterator) { ut, last_chunk,
      }
      /* If an iterator was created when there were no chunks and there are now
   * chunks, "sanitize" it to include the first chunk.
   */
   static struct u_trace_iterator
   sanitize_iterator(struct u_trace_iterator iter)
   {
      if (iter.ut && !iter.chunk && !list_is_empty(&iter.ut->trace_chunks)) {
      iter.chunk =
                  }
      bool
   u_trace_iterator_equal(struct u_trace_iterator a, struct u_trace_iterator b)
   {
      a = sanitize_iterator(a);
   b = sanitize_iterator(b);
      }
      void
   u_trace_clone_append(struct u_trace_iterator begin_it,
                           {
      begin_it = sanitize_iterator(begin_it);
            struct u_trace_chunk *from_chunk = begin_it.chunk;
            while (from_chunk != end_it.chunk || from_idx != end_it.event_idx) {
               unsigned to_copy = MIN2(TRACES_PER_CHUNK - to_chunk->num_traces,
         if (from_chunk == end_it.chunk)
            copy_ts_buffer(begin_it.ut->utctx, cmdstream, from_chunk->timestamps,
                  memcpy(&to_chunk->traces[to_chunk->num_traces],
                  /* Take a refcount on payloads from from_chunk if needed. */
   if (begin_it.ut != into) {
      struct u_trace_payload_buf **in_payload;
   u_vector_foreach (in_payload, &from_chunk->payloads) {
                                    into->num_traces += to_copy;
   to_chunk->num_traces += to_copy;
            assert(from_idx <= from_chunk->num_traces);
   if (from_idx == from_chunk->num_traces) {
                     from_idx = 0;
   from_chunk =
            }
      void
   u_trace_disable_event_range(struct u_trace_iterator begin_it,
         {
      begin_it = sanitize_iterator(begin_it);
            struct u_trace_chunk *current_chunk = begin_it.chunk;
            while (current_chunk != end_it.chunk) {
      memset(&current_chunk->traces[start_idx], 0,
         (current_chunk->num_traces - start_idx) *
   start_idx = 0;
   current_chunk =
               memset(&current_chunk->traces[start_idx], 0,
      }
      /**
   * Append a trace event, returning pointer to buffer of tp->payload_sz
   * to be filled in with trace payload.  Called by generated tracepoint
   * functions.
   */
   void *
   u_trace_appendv(struct u_trace *ut,
                     {
               unsigned payload_sz = ALIGN_NPOT(tp->payload_sz + variable_sz, 8);
   struct u_trace_chunk *chunk = get_chunk(ut, payload_sz);
            /* sub-allocate storage for trace payload: */
   void *payload = NULL;
   if (payload_sz > 0) {
      payload = chunk->payload->next;
               /* record a timestamp for the trace: */
   ut->utctx->record_timestamp(ut, cs, chunk->timestamps, tp_idx,
            chunk->traces[tp_idx] = (struct u_trace_event) {
      .tp = tp,
      };
               }
      void
   u_trace_flush(struct u_trace *ut, void *flush_data, bool free_data)
   {
      list_for_each_entry (struct u_trace_chunk, chunk, &ut->trace_chunks,
            chunk->flush_data = flush_data;
               if (free_data && !list_is_empty(&ut->trace_chunks)) {
      struct u_trace_chunk *last_chunk =
                     /* transfer batch's log chunks to context: */
   list_splicetail(&ut->trace_chunks, &ut->utctx->flushed_trace_chunks);
   list_inithead(&ut->trace_chunks);
      }
