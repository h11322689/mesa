   /**************************************************************************
   *
   * Copyright 2017 Advanced Micro Devices, Inc.
   * All Rights Reserved.
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
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "util/u_threaded_context.h"
   #include "util/u_cpu_detect.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_upload_mgr.h"
   #include "driver_trace/tr_context.h"
   #include "util/log.h"
   #include "util/perf/cpu_trace.h"
   #include "compiler/shader_info.h"
      #if TC_DEBUG >= 1
   #define tc_assert assert
   #else
   #define tc_assert(x)
   #endif
      #if TC_DEBUG >= 2
   #define tc_printf mesa_logi
   #define tc_asprintf asprintf
   #define tc_strcmp strcmp
   #else
   #define tc_printf(...)
   #define tc_asprintf(...) 0
   #define tc_strcmp(...) 0
   #endif
      #define TC_SENTINEL 0x5ca1ab1e
      enum tc_call_id {
   #define CALL(name) TC_CALL_##name,
   #include "u_threaded_context_calls.h"
   #undef CALL
         };
      #if TC_DEBUG >= 3 || defined(TC_TRACE)
   static const char *tc_call_names[] = {
   #define CALL(name) #name,
   #include "u_threaded_context_calls.h"
   #undef CALL
   };
   #endif
      #ifdef TC_TRACE
   #  define TC_TRACE_SCOPE(call_id) MESA_TRACE_SCOPE(tc_call_names[call_id])
   #else
   #  define TC_TRACE_SCOPE(call_id)
   #endif
      typedef uint16_t (*tc_execute)(struct pipe_context *pipe, void *call, uint64_t *last);
      static const tc_execute execute_func[TC_NUM_CALLS];
      static void
   tc_buffer_subdata(struct pipe_context *_pipe,
                        static void
   tc_batch_check(UNUSED struct tc_batch *batch)
   {
      tc_assert(batch->sentinel == TC_SENTINEL);
      }
      static void
   tc_debug_check(struct threaded_context *tc)
   {
      for (unsigned i = 0; i < TC_MAX_BATCHES; i++) {
      tc_batch_check(&tc->batch_slots[i]);
         }
      static void
   tc_set_driver_thread(struct threaded_context *tc)
   {
   #ifndef NDEBUG
         #endif
   }
      static void
   tc_clear_driver_thread(struct threaded_context *tc)
   {
   #ifndef NDEBUG
         #endif
   }
      struct tc_batch_rp_info {
      /* this is what drivers can see */
   struct tc_renderpass_info info;
   /* determines whether the info can be "safely" read by drivers or if it may still be in use */
   struct util_queue_fence ready;
   /* when a batch is full, the rp info rollsover onto 'next' */
   struct tc_batch_rp_info *next;
   /* when rp info has rolled over onto this struct, 'prev' is used to update pointers for realloc */
      };
      static struct tc_batch_rp_info *
   tc_batch_rp_info(struct tc_renderpass_info *info)
   {
         }
      static void
   tc_sanitize_renderpass_info(struct threaded_context *tc)
   {
      tc->renderpass_info_recording->cbuf_invalidate = 0;
   tc->renderpass_info_recording->zsbuf_invalidate = false;
   tc->renderpass_info_recording->cbuf_load |= (~tc->renderpass_info_recording->cbuf_clear) & BITFIELD_MASK(PIPE_MAX_COLOR_BUFS);
   if (tc->fb_resources[PIPE_MAX_COLOR_BUFS] && !tc_renderpass_info_is_zsbuf_used(tc->renderpass_info_recording))
      /* this should be a "safe" way to indicate to the driver that both loads and stores are required;
   * driver can always detect invalidation
   */
      if (tc->num_queries_active)
      }
      /* ensure the batch's array of renderpass data is large enough for the current index */
   static void
   tc_batch_renderpass_infos_resize(struct threaded_context *tc, struct tc_batch *batch)
   {
      unsigned size = batch->renderpass_infos.capacity;
            if (size / sizeof(struct tc_batch_rp_info) > cur_num)
            struct tc_batch_rp_info *infos = batch->renderpass_infos.data;
   unsigned old_idx = batch->renderpass_info_idx - 1;
   bool redo = tc->renderpass_info_recording &&
         if (!util_dynarray_resize(&batch->renderpass_infos, struct tc_batch_rp_info, cur_num + 10))
            if (size != batch->renderpass_infos.capacity) {
      /* zero new allocation region */
   uint8_t *data = batch->renderpass_infos.data;
   memset(data + size, 0, batch->renderpass_infos.capacity - size);
   unsigned start = size / sizeof(struct tc_batch_rp_info);
   unsigned count = (batch->renderpass_infos.capacity - size) /
         infos = batch->renderpass_infos.data;
   if (infos->prev)
         for (unsigned i = 0; i < count; i++)
         /* re-set current recording info on resize */
   if (redo)
         }
      /* signal that the renderpass info is "ready" for use by drivers and will no longer be updated */
   static void
   tc_signal_renderpass_info_ready(struct threaded_context *tc)
   {
      if (tc->renderpass_info_recording &&
      !util_queue_fence_is_signalled(&tc_batch_rp_info(tc->renderpass_info_recording)->ready))
   }
      /* increment the current renderpass info struct for recording
   * 'full_copy' is used for preserving data across non-blocking tc batch flushes
   */
   static void
   tc_batch_increment_renderpass_info(struct threaded_context *tc, unsigned batch_idx, bool full_copy)
   {
      struct tc_batch *batch = &tc->batch_slots[batch_idx];
            if (tc_info[0].next || batch->num_total_slots) {
      /* deadlock condition detected: all batches are in flight, renderpass hasn't ended
   * (probably a cts case)
   */
   struct tc_batch_rp_info *info = tc_batch_rp_info(tc->renderpass_info_recording);
   if (!util_queue_fence_is_signalled(&info->ready)) {
      /* this batch is actively executing and the driver is waiting on the recording fence to signal */
   /* force all buffer usage to avoid data loss */
   info->info.cbuf_load = ~(BITFIELD_MASK(8) & info->info.cbuf_clear);
   info->info.zsbuf_clear_partial = true;
   info->info.has_query_ends = tc->num_queries_active > 0;
   /* ensure threaded_context_get_renderpass_info() won't deadlock */
   info->next = NULL;
      }
   /* always wait on the batch to finish since this will otherwise overwrite thread data */
      }
   /* increment rp info and initialize it */
   batch->renderpass_info_idx++;
   tc_batch_renderpass_infos_resize(tc, batch);
            if (full_copy) {
      /* this should only be called when changing batches */
   assert(batch->renderpass_info_idx == 0);
   /* copy the previous data in its entirety: this is still the same renderpass */
   if (tc->renderpass_info_recording) {
      tc_info[batch->renderpass_info_idx].info.data = tc->renderpass_info_recording->data;
   tc_batch_rp_info(tc->renderpass_info_recording)->next = &tc_info[batch->renderpass_info_idx];
   tc_info[batch->renderpass_info_idx].prev = tc_batch_rp_info(tc->renderpass_info_recording);
   /* guard against deadlock scenario */
      } else {
      tc_info[batch->renderpass_info_idx].info.data = 0;
         } else {
      /* selectively copy: only the CSO metadata is copied, and a new framebuffer state will be added later */
   tc_info[batch->renderpass_info_idx].info.data = 0;
   if (tc->renderpass_info_recording) {
      tc_info[batch->renderpass_info_idx].info.data16[2] = tc->renderpass_info_recording->data16[2];
   tc_batch_rp_info(tc->renderpass_info_recording)->next = NULL;
                  assert(!full_copy || !tc->renderpass_info_recording || tc_batch_rp_info(tc->renderpass_info_recording)->next);
   /* signal existing info since it will not be used anymore */
   tc_signal_renderpass_info_ready(tc);
   util_queue_fence_reset(&tc_info[batch->renderpass_info_idx].ready);
   /* guard against deadlock scenario */
   assert(tc->renderpass_info_recording != &tc_info[batch->renderpass_info_idx].info);
   /* this is now the current recording renderpass info */
   tc->renderpass_info_recording = &tc_info[batch->renderpass_info_idx].info;
      }
      static ALWAYS_INLINE struct tc_renderpass_info *
   tc_get_renderpass_info(struct threaded_context *tc)
   {
         }
      /* update metadata at draw time */
   static void
   tc_parse_draw(struct threaded_context *tc)
   {
               if (info) {
      /* all buffers that aren't cleared are considered loaded */
   info->cbuf_load |= ~info->cbuf_clear;
   if (!info->zsbuf_clear)
         /* previous invalidates are no longer relevant */
   info->cbuf_invalidate = 0;
   info->zsbuf_invalidate = false;
   info->has_draw = true;
               tc->in_renderpass = true;
   tc->seen_fb_state = true;
      }
      static void *
   to_call_check(void *ptr, unsigned num_slots)
   {
   #if TC_DEBUG >= 1
      struct tc_call_base *call = ptr;
      #endif
         }
   #define to_call(ptr, type) ((struct type *)to_call_check((void *)(ptr), call_size(type)))
      #define size_to_slots(size)      DIV_ROUND_UP(size, 8)
   #define call_size(type)          size_to_slots(sizeof(struct type))
   #define call_size_with_slots(type, num_slots) size_to_slots( \
         #define get_next_call(ptr, type) ((struct type*)((uint64_t*)ptr + call_size(type)))
      /* Assign src to dst while dst is uninitialized. */
   static inline void
   tc_set_resource_reference(struct pipe_resource **dst, struct pipe_resource *src)
   {
      *dst = src;
      }
      /* Assign src to dst while dst is uninitialized. */
   static inline void
   tc_set_vertex_state_reference(struct pipe_vertex_state **dst,
         {
      *dst = src;
      }
      /* Unreference dst but don't touch the dst pointer. */
   static inline void
   tc_drop_resource_reference(struct pipe_resource *dst)
   {
      if (pipe_reference(&dst->reference, NULL)) /* only decrement refcount */
      }
      /* Unreference dst but don't touch the dst pointer. */
   static inline void
   tc_drop_surface_reference(struct pipe_surface *dst)
   {
      if (pipe_reference(&dst->reference, NULL)) /* only decrement refcount */
      }
      /* Unreference dst but don't touch the dst pointer. */
   static inline void
   tc_drop_so_target_reference(struct pipe_stream_output_target *dst)
   {
      if (pipe_reference(&dst->reference, NULL)) /* only decrement refcount */
      }
      /**
   * Subtract the given number of references.
   */
   static inline void
   tc_drop_vertex_state_references(struct pipe_vertex_state *dst, int num_refs)
   {
               assert(count >= 0);
   /* Underflows shouldn't happen, but let's be safe. */
   if (count <= 0)
      }
      /* We don't want to read or write min_index and max_index, because
   * it shouldn't be needed by drivers at this point.
   */
   #define DRAW_INFO_SIZE_WITHOUT_MIN_MAX_INDEX \
            ALWAYS_INLINE static struct tc_renderpass_info *
   incr_rp_info(struct tc_renderpass_info *tc_info)
   {
      struct tc_batch_rp_info *info = tc_batch_rp_info(tc_info);
      }
      ALWAYS_INLINE static void
   batch_execute(struct tc_batch *batch, struct pipe_context *pipe, uint64_t *last, bool parsing)
   {
      /* if the framebuffer state is persisting from a previous batch,
   * begin incrementing renderpass info on the first set_framebuffer_state call
   */
   bool first = !batch->first_set_fb;
   for (uint64_t *iter = batch->slots; iter != last;) {
                  #if TC_DEBUG >= 3
         #endif
                              if (parsing) {
      if (call->call_id == TC_CALL_flush) {
      /* always increment renderpass info for non-deferred flushes */
   batch->tc->renderpass_info = incr_rp_info(batch->tc->renderpass_info);
   /* if a flush happens, renderpass info is always incremented after */
      } else if (call->call_id == TC_CALL_set_framebuffer_state) {
      /* the renderpass info pointer is already set at the start of the batch,
   * so don't increment on the first set_framebuffer_state call
   */
   if (!first)
            } else if (call->call_id >= TC_CALL_draw_single &&
            /* if a draw happens before a set_framebuffer_state on this batch,
   * begin incrementing renderpass data 
   */
               }
      static void
   tc_batch_execute(void *job, UNUSED void *gdata, int thread_index)
   {
      struct tc_batch *batch = job;
   struct pipe_context *pipe = batch->tc->pipe;
            tc_batch_check(batch);
                     /* setup renderpass info */
            if (batch->tc->options.parse_renderpass_info) {
               struct tc_batch_rp_info *info = batch->renderpass_infos.data;
   for (unsigned i = 0; i < batch->max_renderpass_info_idx + 1; i++) {
      if (info[i].next)
               } else {
                  /* Add the fence to the list of fences for the driver to signal at the next
   * flush, which we use for tracking which buffers are referenced by
   * an unflushed command buffer.
   */
   struct threaded_context *tc = batch->tc;
   struct util_queue_fence *fence =
            if (tc->options.driver_calls_flush_notify) {
               /* Since our buffer lists are chained as a ring, we need to flush
   * the context twice as we go around the ring to make the driver signal
   * the buffer list fences, so that the producer thread can reuse the buffer
   * list structures for the next batches without waiting.
   */
   unsigned half_ring = TC_MAX_BUFFER_LISTS / 2;
   if (batch->buffer_list_index % half_ring == half_ring - 1)
      } else {
                  tc_clear_driver_thread(batch->tc);
   tc_batch_check(batch);
   batch->num_total_slots = 0;
   batch->last_mergeable_call = NULL;
   batch->first_set_fb = false;
      }
      static void
   tc_begin_next_buffer_list(struct threaded_context *tc)
   {
                        /* Clear the buffer list in the new empty batch. */
   struct tc_buffer_list *buf_list = &tc->buffer_lists[tc->next_buf_list];
   assert(util_queue_fence_is_signalled(&buf_list->driver_flushed_fence));
   util_queue_fence_reset(&buf_list->driver_flushed_fence); /* set to unsignalled */
            tc->add_all_gfx_bindings_to_buffer_list = true;
      }
      static void
   tc_batch_flush(struct threaded_context *tc, bool full_copy)
   {
      struct tc_batch *next = &tc->batch_slots[tc->next];
            tc_assert(next->num_total_slots != 0);
   tc_batch_check(next);
   tc_debug_check(tc);
   tc->bytes_mapped_estimate = 0;
            if (next->token) {
      next->token->tc = NULL;
      }
   /* reset renderpass info index for subsequent use */
            /* always increment renderpass info on batch flush;
   * renderpass info can only be accessed by its owner batch during execution
   */
   if (tc->renderpass_info_recording) {
      tc->batch_slots[next_id].first_set_fb = full_copy;
               util_queue_add_job(&tc->queue, next, &next->fence, tc_batch_execute,
         tc->last = tc->next;
   tc->next = next_id;
         }
      /* This is the function that adds variable-sized calls into the current
   * batch. It also flushes the batch if there is not enough space there.
   * All other higher-level "add" functions use it.
   */
   static void *
   tc_add_sized_call(struct threaded_context *tc, enum tc_call_id id,
         {
      TC_TRACE_SCOPE(id);
   struct tc_batch *next = &tc->batch_slots[tc->next];
   assert(num_slots <= TC_SLOTS_PER_BATCH);
            if (unlikely(next->num_total_slots + num_slots > TC_SLOTS_PER_BATCH)) {
      /* copy existing renderpass info during flush */
   tc_batch_flush(tc, true);
   next = &tc->batch_slots[tc->next];
   tc_assert(next->num_total_slots == 0);
                        struct tc_call_base *call = (struct tc_call_base*)&next->slots[next->num_total_slots];
         #if !defined(NDEBUG) && TC_DEBUG >= 1
         #endif
      call->call_id = id;
         #if TC_DEBUG >= 3
         #endif
         tc_debug_check(tc);
      }
      #define tc_add_call(tc, execute, type) \
            #define tc_add_slot_based_call(tc, execute, type, num_slots) \
      ((struct type*)tc_add_sized_call(tc, execute, \
         /* Returns the last mergeable call that was added to the unflushed
   * batch, or NULL if the address of that call is not currently known
   * or no such call exists in the unflushed batch.
   */
   static struct tc_call_base *
   tc_get_last_mergeable_call(struct threaded_context *tc)
   {
      struct tc_batch *batch = &tc->batch_slots[tc->next];
                     if (call && (uint64_t *)call == &batch->slots[batch->num_total_slots - call->num_slots])
         else
      }
      /* Increases the size of the last call in the unflushed batch to the
   * given number of slots, if possible, without changing the call's data.
   */
   static bool
   tc_enlarge_last_mergeable_call(struct threaded_context *tc, unsigned desired_num_slots)
   {
      struct tc_batch *batch = &tc->batch_slots[tc->next];
            tc_assert(call);
                     if (unlikely(batch->num_total_slots + added_slots > TC_SLOTS_PER_BATCH))
            batch->num_total_slots += added_slots;
               }
      static void
   tc_mark_call_mergeable(struct threaded_context *tc, struct tc_call_base *call)
   {
      struct tc_batch *batch = &tc->batch_slots[tc->next];
   tc_assert(call->num_slots <= batch->num_total_slots);
   tc_assert((uint64_t *)call == &batch->slots[batch->num_total_slots - call->num_slots]);
      }
      static bool
   tc_is_sync(struct threaded_context *tc)
   {
      struct tc_batch *last = &tc->batch_slots[tc->last];
            return util_queue_fence_is_signalled(&last->fence) &&
      }
      static void
   _tc_sync(struct threaded_context *tc, UNUSED const char *info, UNUSED const char *func)
   {
      struct tc_batch *last = &tc->batch_slots[tc->last];
   struct tc_batch *next = &tc->batch_slots[tc->next];
                              if (tc->options.parse_renderpass_info && tc->in_renderpass && !tc->flushing) {
      /* corner case: if tc syncs for any reason but a driver flush during a renderpass,
   * then the current renderpass info MUST be signaled to avoid deadlocking the driver
   *
   * this is not a "complete" signal operation, however, as it's unknown what calls may
   * come after this one, which means that framebuffer attachment data is unreliable
   * 
   * to avoid erroneously passing bad state to the driver (e.g., allowing zsbuf elimination),
   * force all attachments active and assume the app was going to get bad perf here anyway
   */
      }
            /* Only wait for queued calls... */
   if (!util_queue_fence_is_signalled(&last->fence)) {
      util_queue_fence_wait(&last->fence);
                        if (next->token) {
      next->token->tc = NULL;
               /* .. and execute unflushed calls directly. */
   if (next->num_total_slots) {
      p_atomic_add(&tc->num_direct_slots, next->num_total_slots);
   tc->bytes_mapped_estimate = 0;
   tc_batch_execute(next, NULL, 0);
   tc_begin_next_buffer_list(tc);
               if (synced) {
               if (tc_strcmp(func, "tc_destroy") != 0) {
                              if (tc->options.parse_renderpass_info) {
      int renderpass_info_idx = next->renderpass_info_idx;
   if (renderpass_info_idx > 0) {
      /* don't reset if fb state is unflushed */
   bool fb_no_draw = tc->seen_fb_state && !tc->renderpass_info_recording->has_draw;
   uint32_t fb_info = tc->renderpass_info_recording->data32[0];
   next->renderpass_info_idx = -1;
   tc_batch_increment_renderpass_info(tc, tc->next, false);
   if (fb_no_draw)
      } else if (tc->renderpass_info_recording->has_draw) {
         }
   tc->seen_fb_state = false;
         }
      #define tc_sync(tc) _tc_sync(tc, "", __func__)
   #define tc_sync_msg(tc, info) _tc_sync(tc, info, __func__)
      /**
   * Call this from fence_finish for same-context fence waits of deferred fences
   * that haven't been flushed yet.
   *
   * The passed pipe_context must be the one passed to pipe_screen::fence_finish,
   * i.e., the wrapped one.
   */
   void
   threaded_context_flush(struct pipe_context *_pipe,
               {
               /* This is called from the gallium frontend / application thread. */
   if (token->tc && token->tc == tc) {
               /* Prefer to do the flush in the driver thread if it is already
   * running. That should be better for cache locality.
   */
   if (prefer_async || !util_queue_fence_is_signalled(&last->fence))
         else
         }
      static void
   tc_add_to_buffer_list(struct threaded_context *tc, struct tc_buffer_list *next, struct pipe_resource *buf)
   {
      uint32_t id = threaded_resource(buf)->buffer_id_unique;
      }
      /* Set a buffer binding and add it to the buffer list. */
   static void
   tc_bind_buffer(struct threaded_context *tc, uint32_t *binding, struct tc_buffer_list *next, struct pipe_resource *buf)
   {
      uint32_t id = threaded_resource(buf)->buffer_id_unique;
   *binding = id;
      }
      /* Reset a buffer binding. */
   static void
   tc_unbind_buffer(uint32_t *binding)
   {
         }
      /* Reset a range of buffer binding slots. */
   static void
   tc_unbind_buffers(uint32_t *binding, unsigned count)
   {
      if (count)
      }
      static void
   tc_add_bindings_to_buffer_list(BITSET_WORD *buffer_list, const uint32_t *bindings,
         {
      for (unsigned i = 0; i < count; i++) {
      if (bindings[i])
         }
      static bool
   tc_rebind_bindings(uint32_t old_id, uint32_t new_id, uint32_t *bindings,
         {
               for (unsigned i = 0; i < count; i++) {
      if (bindings[i] == old_id) {
      bindings[i] = new_id;
         }
      }
      static void
   tc_add_shader_bindings_to_buffer_list(struct threaded_context *tc,
               {
      tc_add_bindings_to_buffer_list(buffer_list, tc->const_buffers[shader],
         if (tc->seen_shader_buffers[shader]) {
      tc_add_bindings_to_buffer_list(buffer_list, tc->shader_buffers[shader],
      }
   if (tc->seen_image_buffers[shader]) {
      tc_add_bindings_to_buffer_list(buffer_list, tc->image_buffers[shader],
      }
   if (tc->seen_sampler_buffers[shader]) {
      tc_add_bindings_to_buffer_list(buffer_list, tc->sampler_buffers[shader],
         }
      static unsigned
   tc_rebind_shader_bindings(struct threaded_context *tc, uint32_t old_id,
         {
               ubo = tc_rebind_bindings(old_id, new_id, tc->const_buffers[shader],
         if (ubo)
         if (tc->seen_shader_buffers[shader]) {
      ssbo = tc_rebind_bindings(old_id, new_id, tc->shader_buffers[shader],
         if (ssbo)
      }
   if (tc->seen_image_buffers[shader]) {
      img = tc_rebind_bindings(old_id, new_id, tc->image_buffers[shader],
         if (img)
      }
   if (tc->seen_sampler_buffers[shader]) {
      sampler = tc_rebind_bindings(old_id, new_id, tc->sampler_buffers[shader],
         if (sampler)
      }
      }
      /* Add all bound buffers used by VS/TCS/TES/GS/FS to the buffer list.
   * This is called by the first draw call in a batch when we want to inherit
   * all bindings set by the previous batch.
   */
   static void
   tc_add_all_gfx_bindings_to_buffer_list(struct threaded_context *tc)
   {
               tc_add_bindings_to_buffer_list(buffer_list, tc->vertex_buffers, tc->max_vertex_buffers);
   if (tc->seen_streamout_buffers)
            tc_add_shader_bindings_to_buffer_list(tc, buffer_list, PIPE_SHADER_VERTEX);
            if (tc->seen_tcs)
         if (tc->seen_tes)
         if (tc->seen_gs)
               }
      /* Add all bound buffers used by compute to the buffer list.
   * This is called by the first compute call in a batch when we want to inherit
   * all bindings set by the previous batch.
   */
   static void
   tc_add_all_compute_bindings_to_buffer_list(struct threaded_context *tc)
   {
               tc_add_shader_bindings_to_buffer_list(tc, buffer_list, PIPE_SHADER_COMPUTE);
      }
      static unsigned
   tc_rebind_buffer(struct threaded_context *tc, uint32_t old_id, uint32_t new_id, uint32_t *rebind_mask)
   {
               vbo = tc_rebind_bindings(old_id, new_id, tc->vertex_buffers,
         if (vbo)
            if (tc->seen_streamout_buffers) {
      so = tc_rebind_bindings(old_id, new_id, tc->streamout_buffers,
         if (so)
      }
            rebound += tc_rebind_shader_bindings(tc, old_id, new_id, PIPE_SHADER_VERTEX, rebind_mask);
            if (tc->seen_tcs)
         if (tc->seen_tes)
         if (tc->seen_gs)
                     if (rebound)
            }
      static bool
   tc_is_buffer_bound_with_mask(uint32_t id, uint32_t *bindings, unsigned binding_mask)
   {
      while (binding_mask) {
      if (bindings[u_bit_scan(&binding_mask)] == id)
      }
      }
      static bool
   tc_is_buffer_shader_bound_for_write(struct threaded_context *tc, uint32_t id,
         {
      if (tc->seen_shader_buffers[shader] &&
      tc_is_buffer_bound_with_mask(id, tc->shader_buffers[shader],
               if (tc->seen_image_buffers[shader] &&
      tc_is_buffer_bound_with_mask(id, tc->image_buffers[shader],
                  }
      static bool
   tc_is_buffer_bound_for_write(struct threaded_context *tc, uint32_t id)
   {
      if (tc->seen_streamout_buffers &&
      tc_is_buffer_bound_with_mask(id, tc->streamout_buffers,
               if (tc_is_buffer_shader_bound_for_write(tc, id, PIPE_SHADER_VERTEX) ||
      tc_is_buffer_shader_bound_for_write(tc, id, PIPE_SHADER_FRAGMENT) ||
   tc_is_buffer_shader_bound_for_write(tc, id, PIPE_SHADER_COMPUTE))
         if (tc->seen_tcs &&
      tc_is_buffer_shader_bound_for_write(tc, id, PIPE_SHADER_TESS_CTRL))
         if (tc->seen_tes &&
      tc_is_buffer_shader_bound_for_write(tc, id, PIPE_SHADER_TESS_EVAL))
         if (tc->seen_gs &&
      tc_is_buffer_shader_bound_for_write(tc, id, PIPE_SHADER_GEOMETRY))
            }
      static bool
   tc_is_buffer_busy(struct threaded_context *tc, struct threaded_resource *tbuf,
         {
      if (!tc->options.is_resource_busy)
                     for (unsigned i = 0; i < TC_MAX_BUFFER_LISTS; i++) {
               /* If the buffer is referenced by a batch that hasn't been flushed (by tc or the driver),
   * then the buffer is considered busy. */
   if (!util_queue_fence_is_signalled(&buf_list->driver_flushed_fence) &&
      BITSET_TEST(buf_list->buffer_list, id_hash))
            /* The buffer isn't referenced by any unflushed batch: we can safely ask to the driver whether
   * this buffer is busy or not. */
      }
      /**
   * allow_cpu_storage should be false for user memory and imported buffers.
   */
   void
   threaded_resource_init(struct pipe_resource *res, bool allow_cpu_storage)
   {
               tres->latest = &tres->b;
   tres->cpu_storage = NULL;
   util_range_init(&tres->valid_buffer_range);
   tres->is_shared = false;
   tres->is_user_ptr = false;
   tres->buffer_id_unique = 0;
   tres->pending_staging_uploads = 0;
            if (allow_cpu_storage &&
      !(res->flags & (PIPE_RESOURCE_FLAG_MAP_PERSISTENT |
               /* We need buffer invalidation and buffer busyness tracking for the CPU
   * storage, which aren't supported with pipe_vertex_state. */
   !(res->bind & PIPE_BIND_VERTEX_STATE))
      else
      }
      void
   threaded_resource_deinit(struct pipe_resource *res)
   {
               if (tres->latest != &tres->b)
         util_range_destroy(&tres->valid_buffer_range);
   util_range_destroy(&tres->pending_staging_uploads_range);
      }
      struct pipe_context *
   threaded_context_unwrap_sync(struct pipe_context *pipe)
   {
      if (!pipe || !pipe->priv)
            tc_sync(threaded_context(pipe));
      }
         /********************************************************************
   * simple functions
   */
      #define TC_FUNC1(func, qualifier, type, deref, addr, ...) \
      struct tc_call_##func { \
      struct tc_call_base base; \
      }; \
   \
   static uint16_t \
   tc_call_##func(struct pipe_context *pipe, void *call, uint64_t *last) \
   { \
      pipe->func(pipe, addr(to_call(call, tc_call_##func)->state)); \
      } \
   \
   static void \
   tc_##func(struct pipe_context *_pipe, qualifier type deref param) \
   { \
      struct threaded_context *tc = threaded_context(_pipe); \
   struct tc_call_##func *p = (struct tc_call_##func*) \
         p->state = deref(param); \
            TC_FUNC1(set_active_query_state, , bool, , )
      TC_FUNC1(set_blend_color, const, struct pipe_blend_color, *, &)
   TC_FUNC1(set_stencil_ref, const, struct pipe_stencil_ref, , )
   TC_FUNC1(set_clip_state, const, struct pipe_clip_state, *, &)
   TC_FUNC1(set_sample_mask, , unsigned, , )
   TC_FUNC1(set_min_samples, , unsigned, , )
   TC_FUNC1(set_polygon_stipple, const, struct pipe_poly_stipple, *, &)
      TC_FUNC1(texture_barrier, , unsigned, , )
   TC_FUNC1(memory_barrier, , unsigned, , )
   TC_FUNC1(delete_texture_handle, , uint64_t, , )
   TC_FUNC1(delete_image_handle, , uint64_t, , )
   TC_FUNC1(set_frontend_noop, , bool, , )
         /********************************************************************
   * queries
   */
      static struct pipe_query *
   tc_create_query(struct pipe_context *_pipe, unsigned query_type,
         {
      struct threaded_context *tc = threaded_context(_pipe);
               }
      static struct pipe_query *
   tc_create_batch_query(struct pipe_context *_pipe, unsigned num_queries,
         {
      struct threaded_context *tc = threaded_context(_pipe);
               }
      struct tc_query_call {
      struct tc_call_base base;
      };
      static uint16_t
   tc_call_destroy_query(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct pipe_query *query = to_call(call, tc_query_call)->query;
            if (list_is_linked(&tq->head_unflushed))
            pipe->destroy_query(pipe, query);
      }
      static void
   tc_destroy_query(struct pipe_context *_pipe, struct pipe_query *query)
   {
                  }
      static uint16_t
   tc_call_begin_query(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      pipe->begin_query(pipe, to_call(call, tc_query_call)->query);
      }
      static bool
   tc_begin_query(struct pipe_context *_pipe, struct pipe_query *query)
   {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_add_call(tc, TC_CALL_begin_query, tc_query_call)->query = query;
      }
      struct tc_end_query_call {
      struct tc_call_base base;
   struct threaded_context *tc;
      };
      static uint16_t
   tc_call_end_query(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_end_query_call *p = to_call(call, tc_end_query_call);
            if (!list_is_linked(&tq->head_unflushed))
            pipe->end_query(pipe, p->query);
      }
      static bool
   tc_end_query(struct pipe_context *_pipe, struct pipe_query *query)
   {
      struct threaded_context *tc = threaded_context(_pipe);
   struct threaded_query *tq = threaded_query(query);
   struct tc_end_query_call *call =
                  call->tc = tc;
            tq->flushed = false;
               }
      static bool
   tc_get_query_result(struct pipe_context *_pipe,
               {
      struct threaded_context *tc = threaded_context(_pipe);
   struct threaded_query *tq = threaded_query(query);
   struct pipe_context *pipe = tc->pipe;
            if (!flushed) {
      tc_sync_msg(tc, wait ? "wait" : "nowait");
                        if (!flushed)
            if (success) {
      tq->flushed = true;
   if (list_is_linked(&tq->head_unflushed)) {
      /* This is safe because it can only happen after we sync'd. */
         }
      }
      struct tc_query_result_resource {
      struct tc_call_base base;
   enum pipe_query_flags flags:8;
   enum pipe_query_value_type result_type:8;
   int8_t index; /* it can be -1 */
   unsigned offset;
   struct pipe_query *query;
      };
      static uint16_t
   tc_call_get_query_result_resource(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->get_query_result_resource(pipe, p->query, p->flags, p->result_type,
         tc_drop_resource_reference(p->resource);
      }
      static void
   tc_get_query_result_resource(struct pipe_context *_pipe,
                           {
                        struct tc_query_result_resource *p =
      tc_add_call(tc, TC_CALL_get_query_result_resource,
      p->query = query;
   p->flags = flags;
   p->result_type = result_type;
   p->index = index;
   tc_set_resource_reference(&p->resource, resource);
   tc_add_to_buffer_list(tc, &tc->buffer_lists[tc->next_buf_list], resource);
      }
      struct tc_render_condition {
      struct tc_call_base base;
   bool condition;
   unsigned mode;
      };
      static uint16_t
   tc_call_render_condition(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_render_condition *p = to_call(call, tc_render_condition);
   pipe->render_condition(pipe, p->query, p->condition, p->mode);
      }
      static void
   tc_render_condition(struct pipe_context *_pipe,
               {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_render_condition *p =
            p->query = query;
   p->condition = condition;
      }
         /********************************************************************
   * constant (immutable) states
   */
      #define TC_CSO_CREATE(name, sname) \
      static void * \
   tc_create_##name##_state(struct pipe_context *_pipe, \
         { \
      struct pipe_context *pipe = threaded_context(_pipe)->pipe; \
            #define TC_CSO_BIND(name, ...) TC_FUNC1(bind_##name##_state, , void *, , , ##__VA_ARGS__)
   #define TC_CSO_DELETE(name) TC_FUNC1(delete_##name##_state, , void *, , )
      #define TC_CSO(name, sname, ...) \
      TC_CSO_CREATE(name, sname) \
   TC_CSO_BIND(name, ##__VA_ARGS__) \
         #define TC_CSO_WHOLE(name) TC_CSO(name, name)
   #define TC_CSO_SHADER(name) TC_CSO(name, shader)
   #define TC_CSO_SHADER_TRACK(name) TC_CSO(name, shader, tc->seen_##name = true;)
      TC_CSO_WHOLE(blend)
   TC_CSO_WHOLE(rasterizer)
   TC_CSO_CREATE(depth_stencil_alpha, depth_stencil_alpha)
   TC_CSO_BIND(depth_stencil_alpha,
      if (param && tc->options.parse_renderpass_info) {
      /* dsa info is only ever added during a renderpass;
   * changes outside of a renderpass reset the data
   */
   if (!tc->in_renderpass) {
      tc_get_renderpass_info(tc)->zsbuf_write_dsa = 0;
      }
   /* let the driver parse its own state */
         )
   TC_CSO_DELETE(depth_stencil_alpha)
   TC_CSO_WHOLE(compute)
   TC_CSO_CREATE(fs, shader)
   TC_CSO_BIND(fs,
      if (param && tc->options.parse_renderpass_info) {
      /* fs info is only ever added during a renderpass;
   * changes outside of a renderpass reset the data
   */
   if (!tc->in_renderpass) {
      tc_get_renderpass_info(tc)->cbuf_fbfetch = 0;
      }
   /* let the driver parse its own state */
         )
   TC_CSO_DELETE(fs)
   TC_CSO_SHADER(vs)
   TC_CSO_SHADER_TRACK(gs)
   TC_CSO_SHADER_TRACK(tcs)
   TC_CSO_SHADER_TRACK(tes)
   TC_CSO_CREATE(sampler, sampler)
   TC_CSO_DELETE(sampler)
   TC_CSO_BIND(vertex_elements)
   TC_CSO_DELETE(vertex_elements)
      static void *
   tc_create_vertex_elements_state(struct pipe_context *_pipe, unsigned count,
         {
                  }
      struct tc_sampler_states {
      struct tc_call_base base;
   uint8_t shader, start, count;
      };
      static uint16_t
   tc_call_bind_sampler_states(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->bind_sampler_states(pipe, p->shader, p->start, p->count, p->slot);
      }
      static void
   tc_bind_sampler_states(struct pipe_context *_pipe,
               {
      if (!count)
            struct threaded_context *tc = threaded_context(_pipe);
   struct tc_sampler_states *p =
            p->shader = shader;
   p->start = start;
   p->count = count;
      }
      static void
   tc_link_shader(struct pipe_context *_pipe, void **shaders)
   {
      struct threaded_context *tc = threaded_context(_pipe);
      }
   /********************************************************************
   * immediate states
   */
      struct tc_framebuffer {
      struct tc_call_base base;
      };
      static uint16_t
   tc_call_set_framebuffer_state(struct pipe_context *pipe, void *call, uint64_t *last)
   {
                        unsigned nr_cbufs = p->nr_cbufs;
   for (unsigned i = 0; i < nr_cbufs; i++)
         tc_drop_surface_reference(p->zsbuf);
   tc_drop_resource_reference(p->resolve);
      }
      static void
   tc_set_framebuffer_state(struct pipe_context *_pipe,
         {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_framebuffer *p =
                  p->state.width = fb->width;
   p->state.height = fb->height;
   p->state.samples = fb->samples;
   p->state.layers = fb->layers;
               if (tc->options.parse_renderpass_info) {
      /* ensure this is treated as the first fb set if no fb activity has occurred */
   if (!tc->renderpass_info_recording->has_draw &&
      !tc->renderpass_info_recording->cbuf_clear &&
   !tc->renderpass_info_recording->cbuf_load &&
   !tc->renderpass_info_recording->zsbuf_load &&
   !tc->renderpass_info_recording->zsbuf_clear_partial)
      /* store existing zsbuf data for possible persistence */
   uint8_t zsbuf = tc->renderpass_info_recording->has_draw ?
               bool zsbuf_changed = tc->fb_resources[PIPE_MAX_COLOR_BUFS] !=
            for (unsigned i = 0; i < nr_cbufs; i++) {
      p->state.cbufs[i] = NULL;
   pipe_surface_reference(&p->state.cbufs[i], fb->cbufs[i]);
   /* full tracking requires storing the fb attachment resources */
      }
   memset(&tc->fb_resources[nr_cbufs], 0,
            tc->fb_resources[PIPE_MAX_COLOR_BUFS] = fb->zsbuf ? fb->zsbuf->texture : NULL;
   tc->fb_resolve = fb->resolve;
   if (tc->seen_fb_state) {
      /* this is the end of a renderpass, so increment the renderpass info */
   tc_batch_increment_renderpass_info(tc, tc->next, false);
   /* if zsbuf hasn't changed (i.e., possibly just adding a color buffer):
   * keep zsbuf usage data
   */
   if (!zsbuf_changed)
      } else {
      /* this is the first time a set_framebuffer_call is triggered;
   * just increment the index and keep using the existing info for recording
   */
      }
   /* future fb state changes will increment the index */
      } else {
      for (unsigned i = 0; i < nr_cbufs; i++) {
      p->state.cbufs[i] = NULL;
         }
   tc->in_renderpass = false;
   p->state.zsbuf = NULL;
   pipe_surface_reference(&p->state.zsbuf, fb->zsbuf);
   p->state.resolve = NULL;
      }
      struct tc_tess_state {
      struct tc_call_base base;
      };
      static uint16_t
   tc_call_set_tess_state(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->set_tess_state(pipe, p, p + 4);
      }
      static void
   tc_set_tess_state(struct pipe_context *_pipe,
               {
      struct threaded_context *tc = threaded_context(_pipe);
            memcpy(p, default_outer_level, 4 * sizeof(float));
      }
      struct tc_patch_vertices {
      struct tc_call_base base;
      };
      static uint16_t
   tc_call_set_patch_vertices(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->set_patch_vertices(pipe, patch_vertices);
      }
      static void
   tc_set_patch_vertices(struct pipe_context *_pipe, uint8_t patch_vertices)
   {
               tc_add_call(tc, TC_CALL_set_patch_vertices,
      }
      struct tc_constant_buffer_base {
      struct tc_call_base base;
   uint8_t shader, index;
      };
      struct tc_constant_buffer {
      struct tc_constant_buffer_base base;
      };
      static uint16_t
   tc_call_set_constant_buffer(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               if (unlikely(p->base.is_null)) {
      pipe->set_constant_buffer(pipe, p->base.shader, p->base.index, false, NULL);
               pipe->set_constant_buffer(pipe, p->base.shader, p->base.index, true, &p->cb);
      }
      static void
   tc_set_constant_buffer(struct pipe_context *_pipe,
                     {
               if (unlikely(!cb || (!cb->buffer && !cb->user_buffer))) {
      struct tc_constant_buffer_base *p =
         p->shader = shader;
   p->index = index;
   p->is_null = true;
   tc_unbind_buffer(&tc->const_buffers[shader][index]);
               struct pipe_resource *buffer;
            if (cb->user_buffer) {
      /* This must be done before adding set_constant_buffer, because it could
   * generate e.g. transfer_unmap and flush partially-uninitialized
   * set_constant_buffer to the driver if it was done afterwards.
   */
   buffer = NULL;
   u_upload_data(tc->base.const_uploader, 0, cb->buffer_size,
         u_upload_unmap(tc->base.const_uploader);
      } else {
      buffer = cb->buffer;
               struct tc_constant_buffer *p =
         p->base.shader = shader;
   p->base.index = index;
   p->base.is_null = false;
   p->cb.user_buffer = NULL;
   p->cb.buffer_offset = offset;
            if (take_ownership)
         else
            if (buffer) {
      tc_bind_buffer(tc, &tc->const_buffers[shader][index],
      } else {
            }
      struct tc_inlinable_constants {
      struct tc_call_base base;
   uint8_t shader;
   uint8_t num_values;
      };
      static uint16_t
   tc_call_set_inlinable_constants(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->set_inlinable_constants(pipe, p->shader, p->num_values, p->values);
      }
      static void
   tc_set_inlinable_constants(struct pipe_context *_pipe,
               {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_inlinable_constants *p =
         p->shader = shader;
   p->num_values = num_values;
      }
      struct tc_sample_locations {
      struct tc_call_base base;
   uint16_t size;
      };
         static uint16_t
   tc_call_set_sample_locations(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->set_sample_locations(pipe, p->size, p->slot);
      }
      static void
   tc_set_sample_locations(struct pipe_context *_pipe, size_t size, const uint8_t *locations)
   {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_sample_locations *p =
      tc_add_slot_based_call(tc, TC_CALL_set_sample_locations,
         p->size = size;
      }
      struct tc_scissors {
      struct tc_call_base base;
   uint8_t start, count;
      };
      static uint16_t
   tc_call_set_scissor_states(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->set_scissor_states(pipe, p->start, p->count, p->slot);
      }
      static void
   tc_set_scissor_states(struct pipe_context *_pipe,
               {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_scissors *p =
            p->start = start;
   p->count = count;
      }
      struct tc_viewports {
      struct tc_call_base base;
   uint8_t start, count;
      };
      static uint16_t
   tc_call_set_viewport_states(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->set_viewport_states(pipe, p->start, p->count, p->slot);
      }
      static void
   tc_set_viewport_states(struct pipe_context *_pipe,
               {
      if (!count)
            struct threaded_context *tc = threaded_context(_pipe);
   struct tc_viewports *p =
            p->start = start;
   p->count = count;
      }
      struct tc_window_rects {
      struct tc_call_base base;
   bool include;
   uint8_t count;
      };
      static uint16_t
   tc_call_set_window_rectangles(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->set_window_rectangles(pipe, p->include, p->count, p->slot);
      }
      static void
   tc_set_window_rectangles(struct pipe_context *_pipe, bool include,
               {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_window_rects *p =
            p->include = include;
   p->count = count;
      }
      struct tc_sampler_views {
      struct tc_call_base base;
   uint8_t shader, start, count, unbind_num_trailing_slots;
      };
      static uint16_t
   tc_call_set_sampler_views(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->set_sampler_views(pipe, p->shader, p->start, p->count,
            }
      static void
   tc_set_sampler_views(struct pipe_context *_pipe,
                           {
      if (!count && !unbind_num_trailing_slots)
            struct threaded_context *tc = threaded_context(_pipe);
   struct tc_sampler_views *p =
      tc_add_slot_based_call(tc, TC_CALL_set_sampler_views, tc_sampler_views,
         p->shader = shader;
            if (views) {
               p->count = count;
            if (take_ownership) {
               for (unsigned i = 0; i < count; i++) {
      if (views[i] && views[i]->target == PIPE_BUFFER) {
      tc_bind_buffer(tc, &tc->sampler_buffers[shader][start + i], next,
      } else {
               } else {
      for (unsigned i = 0; i < count; i++) {
                     if (views[i] && views[i]->target == PIPE_BUFFER) {
      tc_bind_buffer(tc, &tc->sampler_buffers[shader][start + i], next,
      } else {
                        tc_unbind_buffers(&tc->sampler_buffers[shader][start + count],
            } else {
      p->count = 0;
            tc_unbind_buffers(&tc->sampler_buffers[shader][start],
         }
      struct tc_shader_images {
      struct tc_call_base base;
   uint8_t shader, start, count;
   uint8_t unbind_num_trailing_slots;
      };
      static uint16_t
   tc_call_set_shader_images(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_shader_images *p = (struct tc_shader_images *)call;
            if (!p->count) {
      pipe->set_shader_images(pipe, p->shader, p->start, 0,
                     pipe->set_shader_images(pipe, p->shader, p->start, p->count,
            for (unsigned i = 0; i < count; i++)
               }
      static void
   tc_set_shader_images(struct pipe_context *_pipe,
                           {
      if (!count && !unbind_num_trailing_slots)
            struct threaded_context *tc = threaded_context(_pipe);
   struct tc_shader_images *p =
      tc_add_slot_based_call(tc, TC_CALL_set_shader_images, tc_shader_images,
               p->shader = shader;
            if (images) {
      p->count = count;
                     for (unsigned i = 0; i < count; i++) {
                                                            tc_buffer_disable_cpu_storage(resource);
   util_range_add(&tres->b, &tres->valid_buffer_range,
                     } else {
            }
            tc_unbind_buffers(&tc->image_buffers[shader][start + count],
            } else {
      p->count = 0;
            tc_unbind_buffers(&tc->image_buffers[shader][start],
               tc->image_buffers_writeable_mask[shader] &= ~BITFIELD_RANGE(start, count);
      }
      struct tc_shader_buffers {
      struct tc_call_base base;
   uint8_t shader, start, count;
   bool unbind;
   unsigned writable_bitmask;
      };
      static uint16_t
   tc_call_set_shader_buffers(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_shader_buffers *p = (struct tc_shader_buffers *)call;
            if (p->unbind) {
      pipe->set_shader_buffers(pipe, p->shader, p->start, p->count, NULL, 0);
               pipe->set_shader_buffers(pipe, p->shader, p->start, p->count, p->slot,
            for (unsigned i = 0; i < count; i++)
               }
      static void
   tc_set_shader_buffers(struct pipe_context *_pipe,
                           {
      if (!count)
            struct threaded_context *tc = threaded_context(_pipe);
   struct tc_shader_buffers *p =
      tc_add_slot_based_call(tc, TC_CALL_set_shader_buffers, tc_shader_buffers,
         p->shader = shader;
   p->start = start;
   p->count = count;
   p->unbind = buffers == NULL;
            if (buffers) {
               for (unsigned i = 0; i < count; i++) {
                     tc_set_resource_reference(&dst->buffer, src->buffer);
                                             if (writable_bitmask & BITFIELD_BIT(i)) {
      tc_buffer_disable_cpu_storage(src->buffer);
   util_range_add(&tres->b, &tres->valid_buffer_range,
               } else {
            }
      } else {
                  tc->shader_buffers_writeable_mask[shader] &= ~BITFIELD_RANGE(start, count);
      }
      struct tc_vertex_buffers {
      struct tc_call_base base;
   uint8_t count;
   uint8_t unbind_num_trailing_slots;
      };
      static uint16_t
   tc_call_set_vertex_buffers(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_vertex_buffers *p = (struct tc_vertex_buffers *)call;
            if (!count) {
      pipe->set_vertex_buffers(pipe, 0, p->unbind_num_trailing_slots, false, NULL);
               for (unsigned i = 0; i < count; i++)
            pipe->set_vertex_buffers(pipe, count, p->unbind_num_trailing_slots, true, p->slot);
      }
      static void
   tc_set_vertex_buffers(struct pipe_context *_pipe,
                           {
               if (!count && !unbind_num_trailing_slots)
            if (count && buffers) {
      struct tc_vertex_buffers *p =
         p->count = count;
                     if (take_ownership) {
                                 if (buf) {
         } else {
               } else {
      for (unsigned i = 0; i < count; i++) {
      struct pipe_vertex_buffer *dst = &p->slot[i];
                  tc_assert(!src->is_user_buffer);
   dst->is_user_buffer = false;
                  if (buf) {
         } else {
                        tc_unbind_buffers(&tc->vertex_buffers[count],
      } else {
      struct tc_vertex_buffers *p =
         p->count = 0;
            tc_unbind_buffers(&tc->vertex_buffers[0],
         }
      struct tc_stream_outputs {
      struct tc_call_base base;
   unsigned count;
   struct pipe_stream_output_target *targets[PIPE_MAX_SO_BUFFERS];
      };
      static uint16_t
   tc_call_set_stream_output_targets(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_stream_outputs *p = to_call(call, tc_stream_outputs);
            pipe->set_stream_output_targets(pipe, count, p->targets, p->offsets);
   for (unsigned i = 0; i < count; i++)
               }
      static void
   tc_set_stream_output_targets(struct pipe_context *_pipe,
                     {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_stream_outputs *p =
                  for (unsigned i = 0; i < count; i++) {
      p->targets[i] = NULL;
   pipe_so_target_reference(&p->targets[i], tgs[i]);
   if (tgs[i]) {
      tc_buffer_disable_cpu_storage(tgs[i]->buffer);
      } else {
            }
   p->count = count;
            tc_unbind_buffers(&tc->streamout_buffers[count], PIPE_MAX_SO_BUFFERS - count);
   if (count)
      }
      static void
   tc_set_compute_resources(struct pipe_context *_pipe, unsigned start,
         {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_sync(tc);
      }
      static void
   tc_set_global_binding(struct pipe_context *_pipe, unsigned first,
               {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_sync(tc);
      }
         /********************************************************************
   * views
   */
      static struct pipe_surface *
   tc_create_surface(struct pipe_context *_pipe,
               {
      struct pipe_context *pipe = threaded_context(_pipe)->pipe;
   struct pipe_surface *view =
            if (view)
            }
      static void
   tc_surface_destroy(struct pipe_context *_pipe,
         {
                  }
      static struct pipe_sampler_view *
   tc_create_sampler_view(struct pipe_context *_pipe,
               {
      struct pipe_context *pipe = threaded_context(_pipe)->pipe;
   struct pipe_sampler_view *view =
            if (view)
            }
      static void
   tc_sampler_view_destroy(struct pipe_context *_pipe,
         {
                  }
      static struct pipe_stream_output_target *
   tc_create_stream_output_target(struct pipe_context *_pipe,
                     {
      struct pipe_context *pipe = threaded_context(_pipe)->pipe;
   struct threaded_resource *tres = threaded_resource(res);
            util_range_add(&tres->b, &tres->valid_buffer_range, buffer_offset,
            view = pipe->create_stream_output_target(pipe, res, buffer_offset,
         if (view)
            }
      static void
   tc_stream_output_target_destroy(struct pipe_context *_pipe,
         {
                  }
         /********************************************************************
   * bindless
   */
      static uint64_t
   tc_create_texture_handle(struct pipe_context *_pipe,
               {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_sync(tc);
      }
      struct tc_make_texture_handle_resident {
      struct tc_call_base base;
   bool resident;
      };
      static uint16_t
   tc_call_make_texture_handle_resident(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_make_texture_handle_resident *p =
            pipe->make_texture_handle_resident(pipe, p->handle, p->resident);
      }
      static void
   tc_make_texture_handle_resident(struct pipe_context *_pipe, uint64_t handle,
         {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_make_texture_handle_resident *p =
      tc_add_call(tc, TC_CALL_make_texture_handle_resident,
         p->handle = handle;
      }
      static uint64_t
   tc_create_image_handle(struct pipe_context *_pipe,
         {
      struct threaded_context *tc = threaded_context(_pipe);
            if (image->resource->target == PIPE_BUFFER)
            tc_sync(tc);
      }
      struct tc_make_image_handle_resident {
      struct tc_call_base base;
   bool resident;
   unsigned access;
      };
      static uint16_t
   tc_call_make_image_handle_resident(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_make_image_handle_resident *p =
            pipe->make_image_handle_resident(pipe, p->handle, p->access, p->resident);
      }
      static void
   tc_make_image_handle_resident(struct pipe_context *_pipe, uint64_t handle,
         {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_make_image_handle_resident *p =
      tc_add_call(tc, TC_CALL_make_image_handle_resident,
         p->handle = handle;
   p->access = access;
      }
         /********************************************************************
   * transfer
   */
      struct tc_replace_buffer_storage {
      struct tc_call_base base;
   uint16_t num_rebinds;
   uint32_t rebind_mask;
   uint32_t delete_buffer_id;
   struct pipe_resource *dst;
   struct pipe_resource *src;
      };
      static uint16_t
   tc_call_replace_buffer_storage(struct pipe_context *pipe, void *call, uint64_t *last)
   {
                        tc_drop_resource_reference(p->dst);
   tc_drop_resource_reference(p->src);
      }
      /* Return true if the buffer has been invalidated or is idle. */
   static bool
   tc_invalidate_buffer(struct threaded_context *tc,
         {
      if (!tc_is_buffer_busy(tc, tbuf, PIPE_MAP_READ_WRITE)) {
      /* It's idle, so invalidation would be a no-op, but we can still clear
   * the valid range because we are technically doing invalidation, but
   * skipping it because it's useless.
   *
   * If the buffer is bound for write, we can't invalidate the range.
   */
   if (!tc_is_buffer_bound_for_write(tc, tbuf->buffer_id_unique))
                     struct pipe_screen *screen = tc->base.screen;
            /* Shared, pinned, and sparse buffers can't be reallocated. */
   if (tbuf->is_shared ||
      tbuf->is_user_ptr ||
   tbuf->b.flags & (PIPE_RESOURCE_FLAG_SPARSE | PIPE_RESOURCE_FLAG_UNMAPPABLE))
         /* Allocate a new one. */
   new_buf = screen->resource_create(screen, &tbuf->b);
   if (!new_buf)
            /* Replace the "latest" pointer. */
   if (tbuf->latest != &tbuf->b)
                              /* Enqueue storage replacement of the original buffer. */
   struct tc_replace_buffer_storage *p =
      tc_add_call(tc, TC_CALL_replace_buffer_storage,
         p->func = tc->replace_buffer_storage;
   tc_set_resource_reference(&p->dst, &tbuf->b);
   tc_set_resource_reference(&p->src, new_buf);
   p->delete_buffer_id = delete_buffer_id;
            /* Treat the current buffer as the new buffer. */
   bool bound_for_write = tc_is_buffer_bound_for_write(tc, tbuf->buffer_id_unique);
   p->num_rebinds = tc_rebind_buffer(tc, tbuf->buffer_id_unique,
                  /* If the buffer is not bound for write, clear the valid range. */
   if (!bound_for_write)
            tbuf->buffer_id_unique = threaded_resource(new_buf)->buffer_id_unique;
               }
      static unsigned
   tc_improve_map_buffer_flags(struct threaded_context *tc,
               {
      /* Never invalidate inside the driver and never infer "unsynchronized". */
   unsigned tc_flags = TC_TRANSFER_MAP_NO_INVALIDATE |
            /* Prevent a reentry. */
   if (usage & tc_flags)
            /* Use the staging upload if it's preferred. */
   if (usage & (PIPE_MAP_DISCARD_RANGE |
            !(usage & PIPE_MAP_PERSISTENT) &&
   tres->b.flags & PIPE_RESOURCE_FLAG_DONT_MAP_DIRECTLY &&
   tc->use_forced_staging_uploads) {
   usage &= ~(PIPE_MAP_DISCARD_WHOLE_RESOURCE |
                        /* Sparse buffers can't be mapped directly and can't be reallocated
   * (fully invalidated). That may just be a radeonsi limitation, but
   * the threaded context must obey it with radeonsi.
   */
   if (tres->b.flags & (PIPE_RESOURCE_FLAG_SPARSE | PIPE_RESOURCE_FLAG_UNMAPPABLE)) {
      /* We can use DISCARD_RANGE instead of full discard. This is the only
   * fast path for sparse buffers that doesn't need thread synchronization.
   */
   if (usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE)
            /* Allow DISCARD_WHOLE_RESOURCE and infering UNSYNCHRONIZED in drivers.
   * The threaded context doesn't do unsychronized mappings and invalida-
   * tions of sparse buffers, therefore a correct driver behavior won't
   * result in an incorrect behavior with the threaded context.
   */
                        /* Handle CPU reads trivially. */
   if (usage & PIPE_MAP_READ) {
      if (usage & PIPE_MAP_UNSYNCHRONIZED)
            /* Drivers aren't allowed to do buffer invalidations. */
               /* See if the buffer range being mapped has never been initialized or
   * the buffer is idle, in which case it can be mapped unsynchronized. */
   if (!(usage & PIPE_MAP_UNSYNCHRONIZED) &&
      ((!tres->is_shared &&
         !tc_is_buffer_busy(tc, tres, usage)))
         if (!(usage & PIPE_MAP_UNSYNCHRONIZED)) {
      /* If discarding the entire range, discard the whole resource instead. */
   if (usage & PIPE_MAP_DISCARD_RANGE &&
                  /* Discard the whole resource if needed. */
   if (usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE) {
      if (tc_invalidate_buffer(tc, tres))
         else
                  /* We won't need this flag anymore. */
   /* TODO: We might not need TC_TRANSFER_MAP_NO_INVALIDATE with this. */
            /* GL_AMD_pinned_memory and persistent mappings can't use staging
   * buffers. */
   if (usage & (PIPE_MAP_UNSYNCHRONIZED |
            tres->is_user_ptr)
         /* Unsychronized buffer mappings don't have to synchronize the thread. */
   if (usage & PIPE_MAP_UNSYNCHRONIZED) {
      usage &= ~PIPE_MAP_DISCARD_RANGE;
                  }
      static void *
   tc_buffer_map(struct pipe_context *_pipe,
               struct pipe_resource *resource, unsigned level,
   {
      struct threaded_context *tc = threaded_context(_pipe);
   struct threaded_resource *tres = threaded_resource(resource);
            /* PIPE_MAP_THREAD_SAFE is for glthread, which shouldn't use the CPU storage and
   * this shouldn't normally be necessary because glthread only uses large buffers.
   */
   if (usage & PIPE_MAP_THREAD_SAFE)
                     /* If the CPU storage is enabled, return it directly. */
   if (tres->allow_cpu_storage && !(usage & TC_TRANSFER_MAP_UPLOAD_CPU_STORAGE)) {
      /* We can't let resource_copy_region disable the CPU storage. */
            if (!tres->cpu_storage) {
               if (tres->cpu_storage && tres->valid_buffer_range.end) {
      /* The GPU buffer contains valid data. Copy them to the CPU storage. */
                                                void *ret = pipe->buffer_map(pipe, tres->latest ? tres->latest : resource,
         memcpy(&((uint8_t*)tres->cpu_storage)[tres->valid_buffer_range.start],
                                       if (tres->cpu_storage) {
      struct threaded_transfer *ttrans = slab_zalloc(&tc->pool_transfers);
   ttrans->b.resource = resource;
   ttrans->b.usage = usage;
   ttrans->b.box = *box;
   ttrans->valid_buffer_range = &tres->valid_buffer_range;
                     } else {
                     /* Do a staging transfer within the threaded context. The driver should
   * only get resource_copy_region.
   */
   if (usage & PIPE_MAP_DISCARD_RANGE) {
      struct threaded_transfer *ttrans = slab_zalloc(&tc->pool_transfers);
            u_upload_alloc(tc->base.stream_uploader, 0,
                     if (!map) {
      slab_free(&tc->pool_transfers, ttrans);
               ttrans->b.resource = resource;
   ttrans->b.level = 0;
   ttrans->b.usage = usage;
   ttrans->b.box = *box;
   ttrans->b.stride = 0;
   ttrans->b.layer_stride = 0;
   ttrans->valid_buffer_range = &tres->valid_buffer_range;
   ttrans->cpu_storage_mapped = false;
            p_atomic_inc(&tres->pending_staging_uploads);
   util_range_add(resource, &tres->pending_staging_uploads_range,
                        if (usage & PIPE_MAP_UNSYNCHRONIZED &&
      p_atomic_read(&tres->pending_staging_uploads) &&
   util_ranges_intersect(&tres->pending_staging_uploads_range, box->x, box->x + box->width)) {
   /* Write conflict detected between a staging transfer and the direct mapping we're
   * going to do. Resolve the conflict by ignoring UNSYNCHRONIZED so the direct mapping
   * will have to wait for the staging transfer completion.
   * Note: The conflict detection is only based on the mapped range, not on the actual
   * written range(s).
   */
   usage &= ~PIPE_MAP_UNSYNCHRONIZED & ~TC_TRANSFER_MAP_THREADED_UNSYNC;
               /* Unsychronized buffer mappings don't have to synchronize the thread. */
   if (!(usage & TC_TRANSFER_MAP_THREADED_UNSYNC)) {
      tc_sync_msg(tc, usage & PIPE_MAP_DISCARD_RANGE ? "  discard_range" :
                              void *ret = pipe->buffer_map(pipe, tres->latest ? tres->latest : resource,
         threaded_transfer(*transfer)->valid_buffer_range = &tres->valid_buffer_range;
            if (!(usage & TC_TRANSFER_MAP_THREADED_UNSYNC))
               }
      static void *
   tc_texture_map(struct pipe_context *_pipe,
                     {
      struct threaded_context *tc = threaded_context(_pipe);
   struct threaded_resource *tres = threaded_resource(resource);
            tc_sync_msg(tc, "texture");
                     void *ret = pipe->texture_map(pipe, tres->latest ? tres->latest : resource,
            if (!(usage & TC_TRANSFER_MAP_THREADED_UNSYNC))
               }
      struct tc_transfer_flush_region {
      struct tc_call_base base;
   struct pipe_box box;
      };
      static uint16_t
   tc_call_transfer_flush_region(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->transfer_flush_region(pipe, p->transfer, &p->box);
      }
      struct tc_resource_copy_region {
      struct tc_call_base base;
   unsigned dst_level;
   unsigned dstx, dsty, dstz;
   unsigned src_level;
   struct pipe_box src_box;
   struct pipe_resource *dst;
      };
      static void
   tc_resource_copy_region(struct pipe_context *_pipe,
                              static void
   tc_buffer_do_flush_region(struct threaded_context *tc,
               {
               if (ttrans->staging) {
               u_box_1d(ttrans->b.offset + ttrans->b.box.x % tc->map_buffer_alignment +
                  /* Copy the staging buffer into the original one. */
   tc_resource_copy_region(&tc->base, ttrans->b.resource, 0, box->x, 0, 0,
               /* Don't update the valid range when we're uploading the CPU storage
   * because it includes the uninitialized range too.
   */
   if (!(ttrans->b.usage & TC_TRANSFER_MAP_UPLOAD_CPU_STORAGE)) {
      util_range_add(&tres->b, ttrans->valid_buffer_range,
         }
      static void
   tc_transfer_flush_region(struct pipe_context *_pipe,
               {
      struct threaded_context *tc = threaded_context(_pipe);
   struct threaded_transfer *ttrans = threaded_transfer(transfer);
   struct threaded_resource *tres = threaded_resource(transfer->resource);
   unsigned required_usage = PIPE_MAP_WRITE |
            if (tres->b.target == PIPE_BUFFER) {
      if ((transfer->usage & required_usage) == required_usage) {
               u_box_1d(transfer->box.x + rel_box->x, rel_box->width, &box);
               /* Staging transfers don't send the call to the driver.
   *
   * Transfers using the CPU storage shouldn't call transfer_flush_region
   * in the driver because the buffer is not really mapped on the driver
   * side and the CPU storage always re-uploads everything (flush_region
   * makes no difference).
   */
   if (ttrans->staging || ttrans->cpu_storage_mapped)
               struct tc_transfer_flush_region *p =
         p->transfer = transfer;
      }
      static void
   tc_flush(struct pipe_context *_pipe, struct pipe_fence_handle **fence,
            struct tc_buffer_unmap {
      struct tc_call_base base;
   bool was_staging_transfer;
   union {
      struct pipe_transfer *transfer;
         };
      static uint16_t
   tc_call_buffer_unmap(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               if (p->was_staging_transfer) {
      struct threaded_resource *tres = threaded_resource(p->resource);
   /* Nothing to do except keeping track of staging uploads */
   assert(tres->pending_staging_uploads > 0);
   p_atomic_dec(&tres->pending_staging_uploads);
      } else {
                     }
      static void
   tc_buffer_unmap(struct pipe_context *_pipe, struct pipe_transfer *transfer)
   {
      struct threaded_context *tc = threaded_context(_pipe);
   struct threaded_transfer *ttrans = threaded_transfer(transfer);
            /* PIPE_MAP_THREAD_SAFE is only valid with UNSYNCHRONIZED. It can be
   * called from any thread and bypasses all multithreaded queues.
   */
   if (transfer->usage & PIPE_MAP_THREAD_SAFE) {
      assert(transfer->usage & PIPE_MAP_UNSYNCHRONIZED);
   assert(!(transfer->usage & (PIPE_MAP_FLUSH_EXPLICIT |
            struct pipe_context *pipe = tc->pipe;
   util_range_add(&tres->b, ttrans->valid_buffer_range,
            pipe->buffer_unmap(pipe, transfer);
               if (transfer->usage & PIPE_MAP_WRITE &&
      !(transfer->usage & PIPE_MAP_FLUSH_EXPLICIT))
         if (ttrans->cpu_storage_mapped) {
      /* GL allows simultaneous GPU stores with mapped buffers as long as GPU stores don't
   * touch the mapped range. That's a problem because GPU stores free the CPU storage.
   * If that happens, we just ignore the unmap call and don't upload anything to prevent
   * a crash.
   *
   * Disallow the CPU storage in the driver to work around this.
   */
            if (tres->cpu_storage) {
      tc_invalidate_buffer(tc, tres);
   tc_buffer_subdata(&tc->base, &tres->b,
                     /* This shouldn't have been freed by buffer_subdata. */
      } else {
      static bool warned_once = false;
   if (!warned_once) {
      fprintf(stderr, "This application is incompatible with cpu_storage.\n");
   fprintf(stderr, "Use tc_max_cpu_storage_size=0 to disable it and report this issue to Mesa.\n");
                  tc_drop_resource_reference(ttrans->staging);
   slab_free(&tc->pool_transfers, ttrans);
                        if (ttrans->staging) {
               tc_drop_resource_reference(ttrans->staging);
               struct tc_buffer_unmap *p = tc_add_call(tc, TC_CALL_buffer_unmap,
         if (was_staging_transfer) {
      tc_set_resource_reference(&p->resource, &tres->b);
      } else {
      p->transfer = transfer;
               /* tc_buffer_map directly maps the buffers, but tc_buffer_unmap
   * defers the unmap operation to the batch execution.
   * bytes_mapped_estimate is an estimation of the map/unmap bytes delta
   * and if it goes over an optional limit the current batch is flushed,
   * to reclaim some RAM. */
   if (!ttrans->staging && tc->bytes_mapped_limit &&
      tc->bytes_mapped_estimate > tc->bytes_mapped_limit) {
         }
      struct tc_texture_unmap {
      struct tc_call_base base;
      };
      static uint16_t
   tc_call_texture_unmap(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->texture_unmap(pipe, p->transfer);
      }
      static void
   tc_texture_unmap(struct pipe_context *_pipe, struct pipe_transfer *transfer)
   {
      struct threaded_context *tc = threaded_context(_pipe);
                     /* tc_texture_map directly maps the textures, but tc_texture_unmap
   * defers the unmap operation to the batch execution.
   * bytes_mapped_estimate is an estimation of the map/unmap bytes delta
   * and if it goes over an optional limit the current batch is flushed,
   * to reclaim some RAM. */
   if (!ttrans->staging && tc->bytes_mapped_limit &&
      tc->bytes_mapped_estimate > tc->bytes_mapped_limit) {
         }
      struct tc_buffer_subdata {
      struct tc_call_base base;
   unsigned usage, offset, size;
   struct pipe_resource *resource;
      };
      static uint16_t
   tc_call_buffer_subdata(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->buffer_subdata(pipe, p->resource, p->usage, p->offset, p->size,
         tc_drop_resource_reference(p->resource);
      }
      static bool
   is_mergeable_buffer_subdata(const struct tc_call_base *previous_call,
               {
      if (!previous_call || previous_call->call_id != TC_CALL_buffer_subdata)
                     return subdata->usage == usage && subdata->resource == resource
      }
      static void
   tc_buffer_subdata(struct pipe_context *_pipe,
                     {
      struct threaded_context *tc = threaded_context(_pipe);
            if (!size)
                     /* PIPE_MAP_DIRECTLY supresses implicit DISCARD_RANGE. */
   if (!(usage & PIPE_MAP_DIRECTLY))
                     /* Unsychronized and big transfers should use transfer_map. Also handle
   * full invalidations, because drivers aren't allowed to do them.
   */
   if (usage & (PIPE_MAP_UNSYNCHRONIZED |
            size > TC_MAX_SUBDATA_BYTES ||
   tres->cpu_storage) {
   struct pipe_transfer *transfer;
   struct pipe_box box;
                     /* CPU storage is only useful for partial updates. It can add overhead
   * on glBufferData calls so avoid using it.
   */
   if (!tres->cpu_storage && offset == 0 && size == resource->width0)
            map = tc_buffer_map(_pipe, resource, 0, usage, &box, &transfer);
   if (map) {
      memcpy(map, data, size);
      }
                        /* We can potentially merge this subdata call with the previous one (if any),
   * if the application does a whole-buffer upload piecewise. */
   {
      struct tc_call_base *last_call = tc_get_last_mergeable_call(tc);
            if (is_mergeable_buffer_subdata(last_call, usage, offset, resource) &&
      tc_enlarge_last_mergeable_call(tc, call_size_with_slots(tc_buffer_subdata, merge_dest->size + size))) {
                  /* TODO: We *could* do an invalidate + upload here if we detect that
   * the merged subdata call overwrites the entire buffer. However, that's
   * a little complicated since we can't add further calls to our batch
   * until we have removed the merged subdata call, which means that
   * calling tc_invalidate_buffer before we have removed the call will
   * blow things up.
   * 
   * Just leave a large, merged subdata call in the batch for now, which is
                                 /* The upload is small. Enqueue it. */
   struct tc_buffer_subdata *p =
            tc_set_resource_reference(&p->resource, resource);
   /* This is will always be busy because if it wasn't, tc_improve_map_buffer-
   * _flags would set UNSYNCHRONIZED and we wouldn't get here.
   */
   tc_add_to_buffer_list(tc, &tc->buffer_lists[tc->next_buf_list], resource);
   p->usage = usage;
   p->offset = offset;
   p->size = size;
               }
      struct tc_texture_subdata {
      struct tc_call_base base;
   unsigned level, usage, stride;
   struct pipe_box box;
   struct pipe_resource *resource;
   uintptr_t layer_stride;
      };
      static uint16_t
   tc_call_texture_subdata(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->texture_subdata(pipe, p->resource, p->level, p->usage, &p->box,
         tc_drop_resource_reference(p->resource);
      }
      static void
   tc_texture_subdata(struct pipe_context *_pipe,
                     struct pipe_resource *resource,
   unsigned level, unsigned usage,
   {
      struct threaded_context *tc = threaded_context(_pipe);
            assert(box->height >= 1);
            size = (box->depth - 1) * layer_stride +
         (box->height - 1) * (uint64_t)stride +
   if (!size)
            /* Small uploads can be enqueued, big uploads must sync. */
   if (size <= TC_MAX_SUBDATA_BYTES) {
      struct tc_texture_subdata *p =
            tc_set_resource_reference(&p->resource, resource);
   p->level = level;
   p->usage = usage;
   p->box = *box;
   p->stride = stride;
   p->layer_stride = layer_stride;
      } else {
               if (resource->usage != PIPE_USAGE_STAGING &&
      tc->options.parse_renderpass_info && tc->in_renderpass) {
   enum pipe_format format = resource->format;
   if (usage & PIPE_MAP_DEPTH_ONLY)
                        unsigned fmt_stride = util_format_get_stride(format, box->width);
                  struct pipe_resource *pres = pipe_buffer_create(pipe->screen, 0, PIPE_USAGE_STREAM, layer_stride * box->depth);
   pipe->buffer_subdata(pipe, pres, PIPE_MAP_WRITE | TC_TRANSFER_MAP_THREADED_UNSYNC, 0, layer_stride * box->depth, data);
                  if (fmt_stride == stride && fmt_layer_stride == layer_stride) {
      /* if stride matches, single copy is fine*/
      } else {
      /* if stride doesn't match, inline util_copy_box on the GPU and assume the driver will optimize */
   src_box.depth = 1;
   for (unsigned z = 0; z < box->depth; ++z, src_box.x = z * layer_stride) {
      unsigned dst_x = box->x, dst_y = box->y, width = box->width, height = box->height, dst_z = box->z + z;
                                             dst_x /= blockwidth;
                                                   assert(size <= SIZE_MAX);
   assert(dst_x + src_box.width < u_minify(pres->width0, level));
   assert(dst_y + src_box.height < u_minify(pres->height0, level));
   assert(pres->target != PIPE_TEXTURE_3D ||  z + src_box.depth < u_minify(pres->depth0, level));
      } else {
      src_box.height = 1;
   for (unsigned i = 0; i < height; i++, dst_y++, src_box.x += stride)
                        } else {
      tc_sync(tc);
   tc_set_driver_thread(tc);
   pipe->texture_subdata(pipe, resource, level, usage, box, data,
                  }
         /********************************************************************
   * miscellaneous
   */
      #define TC_FUNC_SYNC_RET0(ret_type, func) \
      static ret_type \
   tc_##func(struct pipe_context *_pipe) \
   { \
      struct threaded_context *tc = threaded_context(_pipe); \
   struct pipe_context *pipe = tc->pipe; \
   tc_sync(tc); \
            TC_FUNC_SYNC_RET0(uint64_t, get_timestamp)
      static void
   tc_get_sample_position(struct pipe_context *_pipe,
               {
      struct threaded_context *tc = threaded_context(_pipe);
            pipe->get_sample_position(pipe, sample_count, sample_index,
      }
      static enum pipe_reset_status
   tc_get_device_reset_status(struct pipe_context *_pipe)
   {
      struct threaded_context *tc = threaded_context(_pipe);
            if (!tc->options.unsynchronized_get_device_reset_status)
               }
      static void
   tc_set_device_reset_callback(struct pipe_context *_pipe,
         {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_sync(tc);
      }
      struct tc_string_marker {
      struct tc_call_base base;
   int len;
      };
      static uint16_t
   tc_call_emit_string_marker(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_string_marker *p = (struct tc_string_marker *)call;
   pipe->emit_string_marker(pipe, p->slot, p->len);
      }
      static void
   tc_emit_string_marker(struct pipe_context *_pipe,
         {
               if (len <= TC_MAX_STRING_MARKER_BYTES) {
      struct tc_string_marker *p =
            memcpy(p->slot, string, len);
      } else {
               tc_sync(tc);
   tc_set_driver_thread(tc);
   pipe->emit_string_marker(pipe, string, len);
         }
      static void
   tc_dump_debug_state(struct pipe_context *_pipe, FILE *stream,
         {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_sync(tc);
      }
      static void
   tc_set_debug_callback(struct pipe_context *_pipe,
         {
      struct threaded_context *tc = threaded_context(_pipe);
                     /* Drop all synchronous debug callbacks. Drivers are expected to be OK
   * with this. shader-db will use an environment variable to disable
   * the threaded context.
   */
   if (cb && !cb->async)
         else
      }
      static void
   tc_set_log_context(struct pipe_context *_pipe, struct u_log_context *log)
   {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_sync(tc);
      }
      static void
   tc_create_fence_fd(struct pipe_context *_pipe,
               {
      struct threaded_context *tc = threaded_context(_pipe);
            if (!tc->options.unsynchronized_create_fence_fd)
               }
      struct tc_fence_call {
      struct tc_call_base base;
      };
      static uint16_t
   tc_call_fence_server_sync(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->fence_server_sync(pipe, fence);
   pipe->screen->fence_reference(pipe->screen, &fence, NULL);
      }
      static void
   tc_fence_server_sync(struct pipe_context *_pipe,
         {
      struct threaded_context *tc = threaded_context(_pipe);
   struct pipe_screen *screen = tc->pipe->screen;
   struct tc_fence_call *call = tc_add_call(tc, TC_CALL_fence_server_sync,
            call->fence = NULL;
      }
      static void
   tc_fence_server_signal(struct pipe_context *_pipe,
         {
      struct threaded_context *tc = threaded_context(_pipe);
   struct pipe_context *pipe = tc->pipe;
   tc_sync(tc);
      }
      static struct pipe_video_codec *
   tc_create_video_codec(UNUSED struct pipe_context *_pipe,
         {
      unreachable("Threaded context should not be enabled for video APIs");
      }
      static struct pipe_video_buffer *
   tc_create_video_buffer(UNUSED struct pipe_context *_pipe,
         {
      unreachable("Threaded context should not be enabled for video APIs");
      }
      struct tc_context_param {
      struct tc_call_base base;
   enum pipe_context_param param;
      };
      static uint16_t
   tc_call_set_context_param(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               if (pipe->set_context_param)
               }
      static void
   tc_set_context_param(struct pipe_context *_pipe,
               {
               if (param == PIPE_CONTEXT_PARAM_PIN_THREADS_TO_L3_CACHE) {
      /* Pin the gallium thread as requested. */
   util_set_thread_affinity(tc->queue.threads[0],
                  /* Execute this immediately (without enqueuing).
   * It's required to be thread-safe.
   */
   struct pipe_context *pipe = tc->pipe;
   if (pipe->set_context_param)
                     if (tc->pipe->set_context_param) {
      struct tc_context_param *call =
            call->param = param;
         }
         /********************************************************************
   * draw, launch, clear, blit, copy, flush
   */
      struct tc_flush_deferred_call {
      struct tc_call_base base;
   unsigned flags;
      };
      struct tc_flush_call {
      struct tc_call_base base;
   unsigned flags;
   struct pipe_fence_handle *fence;
      };
      static void
   tc_flush_queries(struct threaded_context *tc)
   {
      struct threaded_query *tq, *tmp;
   LIST_FOR_EACH_ENTRY_SAFE(tq, tmp, &tc->unflushed_queries, head_unflushed) {
               /* Memory release semantics: due to a possible race with
   * tc_get_query_result, we must ensure that the linked list changes
   * are visible before setting tq->flushed.
   */
         }
      static uint16_t
   tc_call_flush_deferred(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_flush_deferred_call *p = to_call(call, tc_flush_deferred_call);
            pipe->flush(pipe, p->fence ? &p->fence : NULL, p->flags);
               }
      static uint16_t
   tc_call_flush(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_flush_call *p = to_call(call, tc_flush_call);
            pipe->flush(pipe, p->fence ? &p->fence : NULL, p->flags);
                        }
      static void
   tc_flush(struct pipe_context *_pipe, struct pipe_fence_handle **fence,
         {
      struct threaded_context *tc = threaded_context(_pipe);
   struct pipe_context *pipe = tc->pipe;
   struct pipe_screen *screen = pipe->screen;
   bool async = flags & (PIPE_FLUSH_DEFERRED | PIPE_FLUSH_ASYNC);
            if (!deferred || !fence)
            if (async && tc->options.create_fence) {
      if (fence) {
               if (!next->token) {
      next->token = malloc(sizeof(*next->token));
                  pipe_reference_init(&next->token->ref, 1);
               screen->fence_reference(screen, fence,
         if (!*fence)
               struct tc_flush_call *p;
   if (deferred) {
      /* these have identical fields */
      } else {
      p = tc_add_call(tc, TC_CALL_flush, tc_flush_call);
      }
   p->fence = fence ? *fence : NULL;
            if (!deferred) {
      /* non-deferred async flushes indicate completion of existing renderpass info */
   tc_signal_renderpass_info_ready(tc);
   tc_batch_flush(tc, false);
                        out_of_memory:
      tc->flushing = true;
   /* renderpass info is signaled during sync */
   tc_sync_msg(tc, flags & PIPE_FLUSH_END_OF_FRAME ? "end of frame" :
            if (!deferred) {
      tc_flush_queries(tc);
   tc->seen_fb_state = false;
      }
   tc_set_driver_thread(tc);
   pipe->flush(pipe, fence, flags);
   tc_clear_driver_thread(tc);
      }
      struct tc_draw_single {
      struct tc_call_base base;
   unsigned index_bias;
      };
      struct tc_draw_single_drawid {
      struct tc_draw_single base;
      };
      static uint16_t
   tc_call_draw_single_drawid(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_draw_single_drawid *info_drawid = to_call(call, tc_draw_single_drawid);
            /* u_threaded_context stores start/count in min/max_index for single draws. */
   /* Drivers using u_threaded_context shouldn't use min/max_index. */
            draw.start = info->info.min_index;
   draw.count = info->info.max_index;
            info->info.index_bounds_valid = false;
   info->info.has_user_indices = false;
            pipe->draw_vbo(pipe, &info->info, info_drawid->drawid_offset, NULL, &draw, 1);
   if (info->info.index_size)
               }
      static void
   simplify_draw_info(struct pipe_draw_info *info)
   {
      /* Clear these fields to facilitate draw merging.
   * Drivers shouldn't use them.
   */
   info->has_user_indices = false;
   info->index_bounds_valid = false;
   info->take_index_buffer_ownership = false;
   info->index_bias_varies = false;
            /* This shouldn't be set when merging single draws. */
            if (info->index_size) {
      if (!info->primitive_restart)
      } else {
      assert(!info->primitive_restart);
   info->primitive_restart = false;
   info->restart_index = 0;
         }
      static bool
   is_next_call_a_mergeable_draw(struct tc_draw_single *first,
         {
      if (next->base.call_id != TC_CALL_draw_single)
            STATIC_ASSERT(offsetof(struct pipe_draw_info, min_index) ==
         STATIC_ASSERT(offsetof(struct pipe_draw_info, max_index) ==
         /* All fields must be the same except start and count. */
   /* u_threaded_context stores start/count in min/max_index for single draws. */
   return memcmp((uint32_t*)&first->info, (uint32_t*)&next->info,
      }
      static uint16_t
   tc_call_draw_single(struct pipe_context *pipe, void *call, uint64_t *last_ptr)
   {
      /* Draw call merging. */
   struct tc_draw_single *first = to_call(call, tc_draw_single);
   struct tc_draw_single *last = (struct tc_draw_single *)last_ptr;
            /* If at least 2 consecutive draw calls can be merged... */
   if (next != last &&
      next->base.call_id == TC_CALL_draw_single) {
   if (is_next_call_a_mergeable_draw(first, next)) {
      /* The maximum number of merged draws is given by the batch size. */
   struct pipe_draw_start_count_bias multi[TC_SLOTS_PER_BATCH / call_size(tc_draw_single)];
                  /* u_threaded_context stores start/count in min/max_index for single draws. */
   multi[0].start = first->info.min_index;
   multi[0].count = first->info.max_index;
   multi[0].index_bias = first->index_bias;
   multi[1].start = next->info.min_index;
                  /* Find how many other draws can be merged. */
   next = get_next_call(next, tc_draw_single);
   for (; next != last && is_next_call_a_mergeable_draw(first, next);
      next = get_next_call(next, tc_draw_single), num_draws++) {
   /* u_threaded_context stores start/count in min/max_index for single draws. */
   multi[num_draws].start = next->info.min_index;
   multi[num_draws].count = next->info.max_index;
   multi[num_draws].index_bias = next->index_bias;
                              /* Since all draws use the same index buffer, drop all references at once. */
                                 /* u_threaded_context stores start/count in min/max_index for single draws. */
   /* Drivers using u_threaded_context shouldn't use min/max_index. */
            draw.start = first->info.min_index;
   draw.count = first->info.max_index;
            first->info.index_bounds_valid = false;
   first->info.has_user_indices = false;
            pipe->draw_vbo(pipe, &first->info, 0, NULL, &draw, 1);
   if (first->info.index_size)
               }
      struct tc_draw_indirect {
      struct tc_call_base base;
   struct pipe_draw_start_count_bias draw;
   struct pipe_draw_info info;
      };
      static uint16_t
   tc_call_draw_indirect(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               info->info.index_bounds_valid = false;
            pipe->draw_vbo(pipe, &info->info, 0, &info->indirect, &info->draw, 1);
   if (info->info.index_size)
            tc_drop_resource_reference(info->indirect.buffer);
   tc_drop_resource_reference(info->indirect.indirect_draw_count);
   tc_drop_so_target_reference(info->indirect.count_from_stream_output);
      }
      struct tc_draw_multi {
      struct tc_call_base base;
   unsigned num_draws;
   struct pipe_draw_info info;
      };
      static uint16_t
   tc_call_draw_multi(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               info->info.has_user_indices = false;
   info->info.index_bounds_valid = false;
            pipe->draw_vbo(pipe, &info->info, 0, NULL, info->slot, info->num_draws);
   if (info->info.index_size)
               }
      #define DRAW_INFO_SIZE_WITHOUT_INDEXBUF_AND_MIN_MAX_INDEX \
            void
   tc_draw_vbo(struct pipe_context *_pipe, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
      STATIC_ASSERT(DRAW_INFO_SIZE_WITHOUT_INDEXBUF_AND_MIN_MAX_INDEX +
            struct threaded_context *tc = threaded_context(_pipe);
   unsigned index_size = info->index_size;
   bool has_user_indices = info->has_user_indices;
   if (tc->options.parse_renderpass_info)
            if (unlikely(indirect)) {
      assert(!has_user_indices);
            struct tc_draw_indirect *p =
                  if (index_size) {
      if (!info->take_index_buffer_ownership) {
      tc_set_resource_reference(&p->info.index.resource,
      }
      }
            tc_set_resource_reference(&p->indirect.buffer, indirect->buffer);
   tc_set_resource_reference(&p->indirect.indirect_draw_count,
         p->indirect.count_from_stream_output = NULL;
   pipe_so_target_reference(&p->indirect.count_from_stream_output,
            if (indirect->buffer)
         if (indirect->indirect_draw_count)
         if (indirect->count_from_stream_output)
            memcpy(&p->indirect, indirect, sizeof(*indirect));
            /* This must be after tc_add_call, which can flush the batch. */
   if (unlikely(tc->add_all_gfx_bindings_to_buffer_list))
                     if (num_draws == 1) {
      /* Single draw. */
   if (index_size && has_user_indices) {
      unsigned size = draws[0].count * index_size;
                                 /* This must be done before adding draw_vbo, because it could generate
   * e.g. transfer_unmap and flush partially-uninitialized draw_vbo
   * to the driver if it was done afterwards.
   */
   u_upload_data(tc->base.stream_uploader, 0, size, 4,
                              struct tc_draw_single *p = drawid_offset > 0 ?
      &tc_add_call(tc, TC_CALL_draw_single_drawid, tc_draw_single_drawid)->base :
      memcpy(&p->info, info, DRAW_INFO_SIZE_WITHOUT_INDEXBUF_AND_MIN_MAX_INDEX);
   p->info.index.resource = buffer;
   if (drawid_offset > 0)
         /* u_threaded_context stores start/count in min/max_index for single draws. */
   p->info.min_index = offset >> util_logbase2(index_size);
   p->info.max_index = draws[0].count;
   p->index_bias = draws[0].index_bias;
      } else {
      /* Non-indexed call or indexed with a real index buffer. */
   struct tc_draw_single *p = drawid_offset > 0 ?
      &tc_add_call(tc, TC_CALL_draw_single_drawid, tc_draw_single_drawid)->base :
      if (index_size) {
      if (!info->take_index_buffer_ownership) {
      tc_set_resource_reference(&p->info.index.resource,
      }
      }
   if (drawid_offset > 0)
         memcpy(&p->info, info, DRAW_INFO_SIZE_WITHOUT_MIN_MAX_INDEX);
   /* u_threaded_context stores start/count in min/max_index for single draws. */
   p->info.min_index = draws[0].start;
   p->info.max_index = draws[0].count;
   p->index_bias = draws[0].index_bias;
               /* This must be after tc_add_call, which can flush the batch. */
   if (unlikely(tc->add_all_gfx_bindings_to_buffer_list))
                     const int draw_overhead_bytes = sizeof(struct tc_draw_multi);
   const int one_draw_slot_bytes = sizeof(((struct tc_draw_multi*)NULL)->slot[0]);
   const int slots_for_one_draw = DIV_ROUND_UP(draw_overhead_bytes + one_draw_slot_bytes,
         /* Multi draw. */
   if (index_size && has_user_indices) {
      struct pipe_resource *buffer = NULL;
   unsigned buffer_offset, total_count = 0;
   unsigned index_size_shift = util_logbase2(index_size);
            /* Get the total count. */
   for (unsigned i = 0; i < num_draws; i++)
            if (!total_count)
            /* Allocate space for all index buffers.
   *
   * This must be done before adding draw_vbo, because it could generate
   * e.g. transfer_unmap and flush partially-uninitialized draw_vbo
   * to the driver if it was done afterwards.
   */
   u_upload_alloc(tc->base.stream_uploader, 0,
               if (unlikely(!buffer))
            int total_offset = 0;
   unsigned offset = 0;
   while (num_draws) {
               int nb_slots_left = TC_SLOTS_PER_BATCH - next->num_total_slots;
   /* If there isn't enough place for one draw, try to fill the next one */
   if (nb_slots_left < slots_for_one_draw)
                                 struct tc_draw_multi *p =
      tc_add_slot_based_call(tc, TC_CALL_draw_multi, tc_draw_multi,
               if (total_offset == 0)
      /* the first slot inherits the reference from u_upload_alloc() */
      else
                           /* Upload index buffers. */
                     if (!count) {
      p->slot[i].start = 0;
   p->slot[i].count = 0;
                     unsigned size = count << index_size_shift;
   memcpy(ptr + offset,
         (uint8_t*)info->index.user +
   p->slot[i].start = (buffer_offset + offset) >> index_size_shift;
   p->slot[i].count = count;
   p->slot[i].index_bias = draws[i + total_offset].index_bias;
               total_offset += dr;
         } else {
      int total_offset = 0;
   bool take_index_buffer_ownership = info->take_index_buffer_ownership;
   while (num_draws) {
               int nb_slots_left = TC_SLOTS_PER_BATCH - next->num_total_slots;
   /* If there isn't enough place for one draw, try to fill the next one */
   if (nb_slots_left < slots_for_one_draw)
                                 /* Non-indexed call or indexed with a real index buffer. */
   struct tc_draw_multi *p =
      tc_add_slot_based_call(tc, TC_CALL_draw_multi, tc_draw_multi,
      if (index_size) {
      if (!take_index_buffer_ownership) {
      tc_set_resource_reference(&p->info.index.resource,
      }
      }
   take_index_buffer_ownership = false;
   memcpy(&p->info, info, DRAW_INFO_SIZE_WITHOUT_MIN_MAX_INDEX);
   p->num_draws = dr;
                                 /* This must be after tc_add_*call, which can flush the batch. */
   if (unlikely(tc->add_all_gfx_bindings_to_buffer_list))
      }
      struct tc_draw_vstate_single {
      struct tc_call_base base;
            /* The following states must be together without holes because they are
   * compared by draw merging.
   */
   struct pipe_vertex_state *state;
   uint32_t partial_velem_mask;
      };
      static bool
   is_next_call_a_mergeable_draw_vstate(struct tc_draw_vstate_single *first,
         {
      if (next->base.call_id != TC_CALL_draw_vstate_single)
            return !memcmp(&first->state, &next->state,
                  }
      static uint16_t
   tc_call_draw_vstate_single(struct pipe_context *pipe, void *call, uint64_t *last_ptr)
   {
      /* Draw call merging. */
   struct tc_draw_vstate_single *first = to_call(call, tc_draw_vstate_single);
   struct tc_draw_vstate_single *last = (struct tc_draw_vstate_single *)last_ptr;
            /* If at least 2 consecutive draw calls can be merged... */
   if (next != last &&
      is_next_call_a_mergeable_draw_vstate(first, next)) {
   /* The maximum number of merged draws is given by the batch size. */
   struct pipe_draw_start_count_bias draws[TC_SLOTS_PER_BATCH /
                  draws[0] = first->draw;
            /* Find how many other draws can be merged. */
   next = get_next_call(next, tc_draw_vstate_single);
   for (; next != last &&
      is_next_call_a_mergeable_draw_vstate(first, next);
   next = get_next_call(next, tc_draw_vstate_single),
               pipe->draw_vertex_state(pipe, first->state, first->partial_velem_mask,
         /* Since all draws use the same state, drop all references at once. */
                        pipe->draw_vertex_state(pipe, first->state, first->partial_velem_mask,
         tc_drop_vertex_state_references(first->state, 1);
      }
      struct tc_draw_vstate_multi {
      struct tc_call_base base;
   uint32_t partial_velem_mask;
   struct pipe_draw_vertex_state_info info;
   unsigned num_draws;
   struct pipe_vertex_state *state;
      };
      static uint16_t
   tc_call_draw_vstate_multi(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->draw_vertex_state(pipe, info->state, info->partial_velem_mask,
         tc_drop_vertex_state_references(info->state, 1);
      }
      static void
   tc_draw_vertex_state(struct pipe_context *_pipe,
                        struct pipe_vertex_state *state,
      {
      struct threaded_context *tc = threaded_context(_pipe);
   if (tc->options.parse_renderpass_info)
            if (num_draws == 1) {
      /* Single draw. */
   struct tc_draw_vstate_single *p =
         p->partial_velem_mask = partial_velem_mask;
   p->draw = draws[0];
   p->info.mode = info.mode;
            /* This should be always 0 for simplicity because we assume that
   * index_bias doesn't vary.
   */
            if (!info.take_vertex_state_ownership)
         else
               /* This must be after tc_add_*call, which can flush the batch. */
   if (unlikely(tc->add_all_gfx_bindings_to_buffer_list))
                     const int draw_overhead_bytes = sizeof(struct tc_draw_vstate_multi);
   const int one_draw_slot_bytes = sizeof(((struct tc_draw_vstate_multi*)NULL)->slot[0]);
   const int slots_for_one_draw = DIV_ROUND_UP(draw_overhead_bytes + one_draw_slot_bytes,
         /* Multi draw. */
   int total_offset = 0;
   bool take_vertex_state_ownership = info.take_vertex_state_ownership;
   while (num_draws) {
               int nb_slots_left = TC_SLOTS_PER_BATCH - next->num_total_slots;
   /* If there isn't enough place for one draw, try to fill the next one */
   if (nb_slots_left < slots_for_one_draw)
                  /* How many draws can we fit in the current batch */
            /* Non-indexed call or indexed with a real index buffer. */
   struct tc_draw_vstate_multi *p =
            if (!take_vertex_state_ownership)
         else
            take_vertex_state_ownership = false;
   p->partial_velem_mask = partial_velem_mask;
   p->info.mode = info.mode;
   p->info.take_vertex_state_ownership = false;
   p->num_draws = dr;
   memcpy(p->slot, &draws[total_offset], sizeof(draws[0]) * dr);
                           /* This must be after tc_add_*call, which can flush the batch. */
   if (unlikely(tc->add_all_gfx_bindings_to_buffer_list))
      }
      struct tc_launch_grid_call {
      struct tc_call_base base;
      };
      static uint16_t
   tc_call_launch_grid(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->launch_grid(pipe, p);
   tc_drop_resource_reference(p->indirect);
      }
      static void
   tc_launch_grid(struct pipe_context *_pipe,
         {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_launch_grid_call *p = tc_add_call(tc, TC_CALL_launch_grid,
                  tc_set_resource_reference(&p->info.indirect, info->indirect);
            if (info->indirect)
            /* This must be after tc_add_*call, which can flush the batch. */
   if (unlikely(tc->add_all_compute_bindings_to_buffer_list))
      }
      static uint16_t
   tc_call_resource_copy_region(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->resource_copy_region(pipe, p->dst, p->dst_level, p->dstx, p->dsty,
         tc_drop_resource_reference(p->dst);
   tc_drop_resource_reference(p->src);
      }
      static void
   tc_resource_copy_region(struct pipe_context *_pipe,
                           {
      struct threaded_context *tc = threaded_context(_pipe);
   struct threaded_resource *tdst = threaded_resource(dst);
   struct tc_resource_copy_region *p =
      tc_add_call(tc, TC_CALL_resource_copy_region,
         if (dst->target == PIPE_BUFFER)
            tc_set_resource_reference(&p->dst, dst);
   p->dst_level = dst_level;
   p->dstx = dstx;
   p->dsty = dsty;
   p->dstz = dstz;
   tc_set_resource_reference(&p->src, src);
   p->src_level = src_level;
            if (dst->target == PIPE_BUFFER) {
               tc_add_to_buffer_list(tc, next, src);
            util_range_add(&tdst->b, &tdst->valid_buffer_range,
         }
      struct tc_blit_call {
      struct tc_call_base base;
      };
      static uint16_t
   tc_call_blit(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->blit(pipe, blit);
   tc_drop_resource_reference(blit->dst.resource);
   tc_drop_resource_reference(blit->src.resource);
      }
      static void
   tc_blit(struct pipe_context *_pipe, const struct pipe_blit_info *info)
   {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_set_resource_reference(&blit->info.dst.resource, info->dst.resource);
   tc_set_resource_reference(&blit->info.src.resource, info->src.resource);
   memcpy(&blit->info, info, sizeof(*info));
   if (tc->options.parse_renderpass_info) {
      tc->renderpass_info_recording->has_resolve = info->src.resource->nr_samples > 1 &&
               }
      struct tc_generate_mipmap {
      struct tc_call_base base;
   enum pipe_format format;
   unsigned base_level;
   unsigned last_level;
   unsigned first_layer;
   unsigned last_layer;
      };
      static uint16_t
   tc_call_generate_mipmap(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      struct tc_generate_mipmap *p = to_call(call, tc_generate_mipmap);
   ASSERTED bool result = pipe->generate_mipmap(pipe, p->res, p->format,
                           assert(result);
   tc_drop_resource_reference(p->res);
      }
      static bool
   tc_generate_mipmap(struct pipe_context *_pipe,
                     struct pipe_resource *res,
   enum pipe_format format,
   unsigned base_level,
   {
      struct threaded_context *tc = threaded_context(_pipe);
   struct pipe_context *pipe = tc->pipe;
   struct pipe_screen *screen = pipe->screen;
            if (util_format_is_depth_or_stencil(format))
         else
            if (!screen->is_format_supported(screen, format, res->target,
                        struct tc_generate_mipmap *p =
            tc_set_resource_reference(&p->res, res);
   p->format = format;
   p->base_level = base_level;
   p->last_level = last_level;
   p->first_layer = first_layer;
   p->last_layer = last_layer;
      }
      struct tc_resource_call {
      struct tc_call_base base;
      };
      static uint16_t
   tc_call_flush_resource(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->flush_resource(pipe, resource);
   tc_drop_resource_reference(resource);
      }
      static void
   tc_flush_resource(struct pipe_context *_pipe, struct pipe_resource *resource)
   {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_resource_call *call = tc_add_call(tc, TC_CALL_flush_resource,
               }
      static uint16_t
   tc_call_invalidate_resource(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->invalidate_resource(pipe, resource);
   tc_drop_resource_reference(resource);
      }
      static void
   tc_invalidate_resource(struct pipe_context *_pipe,
         {
               if (resource->target == PIPE_BUFFER) {
      tc_invalidate_buffer(tc, threaded_resource(resource));
               struct tc_resource_call *call = tc_add_call(tc, TC_CALL_invalidate_resource,
                  struct tc_renderpass_info *info = tc_get_renderpass_info(tc);
   if (info) {
      if (tc->fb_resources[PIPE_MAX_COLOR_BUFS] == resource) {
         } else {
      for (unsigned i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      if (tc->fb_resources[i] == resource)
               }
      struct tc_clear {
      struct tc_call_base base;
   bool scissor_state_set;
   uint8_t stencil;
   uint16_t buffers;
   float depth;
   struct pipe_scissor_state scissor_state;
      };
      static uint16_t
   tc_call_clear(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->clear(pipe, p->buffers, p->scissor_state_set ? &p->scissor_state : NULL, &p->color, p->depth, p->stencil);
      }
      static void
   tc_clear(struct pipe_context *_pipe, unsigned buffers, const struct pipe_scissor_state *scissor_state,
               {
      struct threaded_context *tc = threaded_context(_pipe);
            p->buffers = buffers;
   if (scissor_state) {
      p->scissor_state = *scissor_state;
   struct tc_renderpass_info *info = tc_get_renderpass_info(tc);
   /* partial clear info is useful for drivers to know whether any zs writes occur;
   * drivers are responsible for optimizing partial clear -> full clear
   */
   if (info && buffers & PIPE_CLEAR_DEPTHSTENCIL)
      } else {
      struct tc_renderpass_info *info = tc_get_renderpass_info(tc);
   if (info) {
      /* full clears use a different load operation, but are only valid if draws haven't occurred yet */
   info->cbuf_clear |= (buffers >> 2) & ~info->cbuf_load;
   if (buffers & PIPE_CLEAR_DEPTHSTENCIL) {
      if (!info->zsbuf_load && !info->zsbuf_clear_partial)
         else if (!info->zsbuf_clear)
      /* this is a clear that occurred after a draw: flag as partial to ensure it isn't ignored */
         }
   p->scissor_state_set = !!scissor_state;
   p->color = *color;
   p->depth = depth;
      }
      struct tc_clear_render_target {
      struct tc_call_base base;
   bool render_condition_enabled;
   unsigned dstx;
   unsigned dsty;
   unsigned width;
   unsigned height;
   union pipe_color_union color;
      };
      static uint16_t
   tc_call_clear_render_target(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->clear_render_target(pipe, p->dst, &p->color, p->dstx, p->dsty, p->width, p->height,
         tc_drop_surface_reference(p->dst);
      }
      static void
   tc_clear_render_target(struct pipe_context *_pipe,
                        struct pipe_surface *dst,
      {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_clear_render_target *p = tc_add_call(tc, TC_CALL_clear_render_target, tc_clear_render_target);
   p->dst = NULL;
   pipe_surface_reference(&p->dst, dst);
   p->color = *color;
   p->dstx = dstx;
   p->dsty = dsty;
   p->width = width;
   p->height = height;
      }
         struct tc_clear_depth_stencil {
      struct tc_call_base base;
   bool render_condition_enabled;
   float depth;
   unsigned clear_flags;
   unsigned stencil;
   unsigned dstx;
   unsigned dsty;
   unsigned width;
   unsigned height;
      };
         static uint16_t
   tc_call_clear_depth_stencil(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->clear_depth_stencil(pipe, p->dst, p->clear_flags, p->depth, p->stencil,
               tc_drop_surface_reference(p->dst);
      }
      static void
   tc_clear_depth_stencil(struct pipe_context *_pipe,
                           {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_clear_depth_stencil *p = tc_add_call(tc, TC_CALL_clear_depth_stencil, tc_clear_depth_stencil);
   p->dst = NULL;
   pipe_surface_reference(&p->dst, dst);
   p->clear_flags = clear_flags;
   p->depth = depth;
   p->stencil = stencil;
   p->dstx = dstx;
   p->dsty = dsty;
   p->width = width;
   p->height = height;
      }
      struct tc_clear_buffer {
      struct tc_call_base base;
   uint8_t clear_value_size;
   unsigned offset;
   unsigned size;
   char clear_value[16];
      };
      static uint16_t
   tc_call_clear_buffer(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->clear_buffer(pipe, p->res, p->offset, p->size, p->clear_value,
         tc_drop_resource_reference(p->res);
      }
      static void
   tc_clear_buffer(struct pipe_context *_pipe, struct pipe_resource *res,
               {
      struct threaded_context *tc = threaded_context(_pipe);
   struct threaded_resource *tres = threaded_resource(res);
   struct tc_clear_buffer *p =
                     tc_set_resource_reference(&p->res, res);
   tc_add_to_buffer_list(tc, &tc->buffer_lists[tc->next_buf_list], res);
   p->offset = offset;
   p->size = size;
   memcpy(p->clear_value, clear_value, clear_value_size);
               }
      struct tc_clear_texture {
      struct tc_call_base base;
   unsigned level;
   struct pipe_box box;
   char data[16];
      };
      static uint16_t
   tc_call_clear_texture(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->clear_texture(pipe, p->res, p->level, &p->box, p->data);
   tc_drop_resource_reference(p->res);
      }
      static void
   tc_clear_texture(struct pipe_context *_pipe, struct pipe_resource *res,
         {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_clear_texture *p =
            tc_set_resource_reference(&p->res, res);
   p->level = level;
   p->box = *box;
   memcpy(p->data, data,
      }
      struct tc_resource_commit {
      struct tc_call_base base;
   bool commit;
   unsigned level;
   struct pipe_box box;
      };
      static uint16_t
   tc_call_resource_commit(struct pipe_context *pipe, void *call, uint64_t *last)
   {
               pipe->resource_commit(pipe, p->res, p->level, &p->box, p->commit);
   tc_drop_resource_reference(p->res);
      }
      static bool
   tc_resource_commit(struct pipe_context *_pipe, struct pipe_resource *res,
         {
      struct threaded_context *tc = threaded_context(_pipe);
   struct tc_resource_commit *p =
            tc_set_resource_reference(&p->res, res);
   p->level = level;
   p->box = *box;
   p->commit = commit;
      }
      static unsigned
   tc_init_intel_perf_query_info(struct pipe_context *_pipe)
   {
      struct threaded_context *tc = threaded_context(_pipe);
               }
      static void
   tc_get_intel_perf_query_info(struct pipe_context *_pipe,
                                 {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_sync(tc); /* n_active vs begin/end_intel_perf_query */
   pipe->get_intel_perf_query_info(pipe, query_index, name, data_size,
      }
      static void
   tc_get_intel_perf_query_counter_info(struct pipe_context *_pipe,
                                       unsigned query_index,
   unsigned counter_index,
   const char **name,
   {
      struct threaded_context *tc = threaded_context(_pipe);
            pipe->get_intel_perf_query_counter_info(pipe, query_index, counter_index,
      }
      static struct pipe_query *
   tc_new_intel_perf_query_obj(struct pipe_context *_pipe, unsigned query_index)
   {
      struct threaded_context *tc = threaded_context(_pipe);
               }
      static uint16_t
   tc_call_begin_intel_perf_query(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      (void)pipe->begin_intel_perf_query(pipe, to_call(call, tc_query_call)->query);
      }
      static bool
   tc_begin_intel_perf_query(struct pipe_context *_pipe, struct pipe_query *q)
   {
                        /* assume success, begin failure can be signaled from get_intel_perf_query_data */
      }
      static uint16_t
   tc_call_end_intel_perf_query(struct pipe_context *pipe, void *call, uint64_t *last)
   {
      pipe->end_intel_perf_query(pipe, to_call(call, tc_query_call)->query);
      }
      static void
   tc_end_intel_perf_query(struct pipe_context *_pipe, struct pipe_query *q)
   {
                  }
      static void
   tc_delete_intel_perf_query(struct pipe_context *_pipe, struct pipe_query *q)
   {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_sync(tc); /* flush potentially pending begin/end_intel_perf_queries */
      }
      static void
   tc_wait_intel_perf_query(struct pipe_context *_pipe, struct pipe_query *q)
   {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_sync(tc); /* flush potentially pending begin/end_intel_perf_queries */
      }
      static bool
   tc_is_intel_perf_query_ready(struct pipe_context *_pipe, struct pipe_query *q)
   {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_sync(tc); /* flush potentially pending begin/end_intel_perf_queries */
      }
      static bool
   tc_get_intel_perf_query_data(struct pipe_context *_pipe,
                           {
      struct threaded_context *tc = threaded_context(_pipe);
            tc_sync(tc); /* flush potentially pending begin/end_intel_perf_queries */
      }
      /********************************************************************
   * callback
   */
      struct tc_callback_call {
      struct tc_call_base base;
   void (*fn)(void *data);
      };
      static uint16_t
   tc_call_callback(UNUSED struct pipe_context *pipe, void *call, uint64_t *last)
   {
               p->fn(p->data);
      }
      static void
   tc_callback(struct pipe_context *_pipe, void (*fn)(void *), void *data,
         {
               if (asap && tc_is_sync(tc)) {
      fn(data);
               struct tc_callback_call *p =
         p->fn = fn;
      }
         /********************************************************************
   * create & destroy
   */
      static void
   tc_destroy(struct pipe_context *_pipe)
   {
      struct threaded_context *tc = threaded_context(_pipe);
            if (tc->base.const_uploader &&
      tc->base.stream_uploader != tc->base.const_uploader)
         if (tc->base.stream_uploader)
                     if (util_queue_is_initialized(&tc->queue)) {
               for (unsigned i = 0; i < TC_MAX_BATCHES; i++) {
      util_queue_fence_destroy(&tc->batch_slots[i].fence);
   util_dynarray_fini(&tc->batch_slots[i].renderpass_infos);
                  slab_destroy_child(&tc->pool_transfers);
   assert(tc->batch_slots[tc->next].num_total_slots == 0);
            for (unsigned i = 0; i < TC_MAX_BUFFER_LISTS; i++) {
      if (!util_queue_fence_is_signalled(&tc->buffer_lists[i].driver_flushed_fence))
                        }
      static const tc_execute execute_func[TC_NUM_CALLS] = {
   #define CALL(name) tc_call_##name,
   #include "u_threaded_context_calls.h"
   #undef CALL
   };
      void tc_driver_internal_flush_notify(struct threaded_context *tc)
   {
      /* Allow drivers to call this function even for internal contexts that
   * don't have tc. It simplifies drivers.
   */
   if (!tc)
            /* Signal fences set by tc_batch_execute. */
   for (unsigned i = 0; i < tc->num_signal_fences_next_flush; i++)
               }
      /**
   * Wrap an existing pipe_context into a threaded_context.
   *
   * \param pipe                 pipe_context to wrap
   * \param parent_transfer_pool parent slab pool set up for creating pipe_-
   *                             transfer objects; the driver should have one
   *                             in pipe_screen.
   * \param replace_buffer  callback for replacing a pipe_resource's storage
   *                        with another pipe_resource's storage.
   * \param options         optional TC options/callbacks
   * \param out  if successful, the threaded_context will be returned here in
   *             addition to the return value if "out" != NULL
   */
   struct pipe_context *
   threaded_context_create(struct pipe_context *pipe,
                           {
               if (!pipe)
            if (!debug_get_bool_option("GALLIUM_THREAD", util_get_cpu_caps()->nr_cpus > 1))
            tc = CALLOC_STRUCT(threaded_context);
   if (!tc) {
      pipe->destroy(pipe);
               if (options) {
      /* this is unimplementable */
   assert(!(options->parse_renderpass_info && options->driver_calls_flush_notify));
                        /* The driver context isn't wrapped, so set its "priv" to NULL. */
            tc->pipe = pipe;
   tc->replace_buffer_storage = replace_buffer;
   tc->map_buffer_alignment =
         tc->ubo_alignment =
         tc->base.priv = pipe; /* priv points to the wrapped driver context */
   tc->base.screen = pipe->screen;
   tc->base.destroy = tc_destroy;
            tc->base.stream_uploader = u_upload_clone(&tc->base, pipe->stream_uploader);
   if (pipe->stream_uploader == pipe->const_uploader)
         else
            if (!tc->base.stream_uploader || !tc->base.const_uploader)
                     /* The queue size is the number of batches "waiting". Batches are removed
   * from the queue before being executed, so keep one tc_batch slot for that
   * execution. Also, keep one unused slot for an unflushed batch.
   */
   if (!util_queue_init(&tc->queue, "gdrv", TC_MAX_BATCHES - 2, 1, 0, NULL))
               #if !defined(NDEBUG) && TC_DEBUG >= 1
         #endif
         tc->batch_slots[i].tc = tc;
   util_queue_fence_init(&tc->batch_slots[i].fence);
   tc->batch_slots[i].renderpass_info_idx = -1;
   if (tc->options.parse_renderpass_info) {
      util_dynarray_init(&tc->batch_slots[i].renderpass_infos, NULL);
         }
   for (unsigned i = 0; i < TC_MAX_BUFFER_LISTS; i++)
                              /* If you have different limits in each shader stage, set the maximum. */
   struct pipe_screen *screen = pipe->screen;;
   tc->max_vertex_buffers =
         tc->max_const_buffers =
      screen->get_shader_param(screen, PIPE_SHADER_FRAGMENT,
      tc->max_shader_buffers =
      screen->get_shader_param(screen, PIPE_SHADER_FRAGMENT,
      tc->max_images =
      screen->get_shader_param(screen, PIPE_SHADER_FRAGMENT,
      tc->max_samplers =
      screen->get_shader_param(screen, PIPE_SHADER_FRAGMENT,
               #define CTX_INIT(_member) \
               CTX_INIT(flush);
   CTX_INIT(draw_vbo);
   CTX_INIT(draw_vertex_state);
   CTX_INIT(launch_grid);
   CTX_INIT(resource_copy_region);
   CTX_INIT(blit);
   CTX_INIT(clear);
   CTX_INIT(clear_render_target);
   CTX_INIT(clear_depth_stencil);
   CTX_INIT(clear_buffer);
   CTX_INIT(clear_texture);
   CTX_INIT(flush_resource);
   CTX_INIT(generate_mipmap);
   CTX_INIT(render_condition);
   CTX_INIT(create_query);
   CTX_INIT(create_batch_query);
   CTX_INIT(destroy_query);
   CTX_INIT(begin_query);
   CTX_INIT(end_query);
   CTX_INIT(get_query_result);
   CTX_INIT(get_query_result_resource);
   CTX_INIT(set_active_query_state);
   CTX_INIT(create_blend_state);
   CTX_INIT(bind_blend_state);
   CTX_INIT(delete_blend_state);
   CTX_INIT(create_sampler_state);
   CTX_INIT(bind_sampler_states);
   CTX_INIT(delete_sampler_state);
   CTX_INIT(create_rasterizer_state);
   CTX_INIT(bind_rasterizer_state);
   CTX_INIT(delete_rasterizer_state);
   CTX_INIT(create_depth_stencil_alpha_state);
   CTX_INIT(bind_depth_stencil_alpha_state);
   CTX_INIT(delete_depth_stencil_alpha_state);
   CTX_INIT(link_shader);
   CTX_INIT(create_fs_state);
   CTX_INIT(bind_fs_state);
   CTX_INIT(delete_fs_state);
   CTX_INIT(create_vs_state);
   CTX_INIT(bind_vs_state);
   CTX_INIT(delete_vs_state);
   CTX_INIT(create_gs_state);
   CTX_INIT(bind_gs_state);
   CTX_INIT(delete_gs_state);
   CTX_INIT(create_tcs_state);
   CTX_INIT(bind_tcs_state);
   CTX_INIT(delete_tcs_state);
   CTX_INIT(create_tes_state);
   CTX_INIT(bind_tes_state);
   CTX_INIT(delete_tes_state);
   CTX_INIT(create_compute_state);
   CTX_INIT(bind_compute_state);
   CTX_INIT(delete_compute_state);
   CTX_INIT(create_vertex_elements_state);
   CTX_INIT(bind_vertex_elements_state);
   CTX_INIT(delete_vertex_elements_state);
   CTX_INIT(set_blend_color);
   CTX_INIT(set_stencil_ref);
   CTX_INIT(set_sample_mask);
   CTX_INIT(set_min_samples);
   CTX_INIT(set_clip_state);
   CTX_INIT(set_constant_buffer);
   CTX_INIT(set_inlinable_constants);
   CTX_INIT(set_framebuffer_state);
   CTX_INIT(set_polygon_stipple);
   CTX_INIT(set_sample_locations);
   CTX_INIT(set_scissor_states);
   CTX_INIT(set_viewport_states);
   CTX_INIT(set_window_rectangles);
   CTX_INIT(set_sampler_views);
   CTX_INIT(set_tess_state);
   CTX_INIT(set_patch_vertices);
   CTX_INIT(set_shader_buffers);
   CTX_INIT(set_shader_images);
   CTX_INIT(set_vertex_buffers);
   CTX_INIT(create_stream_output_target);
   CTX_INIT(stream_output_target_destroy);
   CTX_INIT(set_stream_output_targets);
   CTX_INIT(create_sampler_view);
   CTX_INIT(sampler_view_destroy);
   CTX_INIT(create_surface);
   CTX_INIT(surface_destroy);
   CTX_INIT(buffer_map);
   CTX_INIT(texture_map);
   CTX_INIT(transfer_flush_region);
   CTX_INIT(buffer_unmap);
   CTX_INIT(texture_unmap);
   CTX_INIT(buffer_subdata);
   CTX_INIT(texture_subdata);
   CTX_INIT(texture_barrier);
   CTX_INIT(memory_barrier);
   CTX_INIT(resource_commit);
   CTX_INIT(create_video_codec);
   CTX_INIT(create_video_buffer);
   CTX_INIT(set_compute_resources);
   CTX_INIT(set_global_binding);
   CTX_INIT(get_sample_position);
   CTX_INIT(invalidate_resource);
   CTX_INIT(get_device_reset_status);
   CTX_INIT(set_device_reset_callback);
   CTX_INIT(dump_debug_state);
   CTX_INIT(set_log_context);
   CTX_INIT(emit_string_marker);
   CTX_INIT(set_debug_callback);
   CTX_INIT(create_fence_fd);
   CTX_INIT(fence_server_sync);
   CTX_INIT(fence_server_signal);
   CTX_INIT(get_timestamp);
   CTX_INIT(create_texture_handle);
   CTX_INIT(delete_texture_handle);
   CTX_INIT(make_texture_handle_resident);
   CTX_INIT(create_image_handle);
   CTX_INIT(delete_image_handle);
   CTX_INIT(make_image_handle_resident);
   CTX_INIT(set_frontend_noop);
   CTX_INIT(init_intel_perf_query_info);
   CTX_INIT(get_intel_perf_query_info);
   CTX_INIT(get_intel_perf_query_counter_info);
   CTX_INIT(new_intel_perf_query_obj);
   CTX_INIT(begin_intel_perf_query);
   CTX_INIT(end_intel_perf_query);
   CTX_INIT(delete_intel_perf_query);
   CTX_INIT(wait_intel_perf_query);
   CTX_INIT(is_intel_perf_query_ready);
      #undef CTX_INIT
         if (out)
            tc_begin_next_buffer_list(tc);
   if (tc->options.parse_renderpass_info)
               fail:
      tc_destroy(&tc->base);
      }
      void
   threaded_context_init_bytes_mapped_limit(struct threaded_context *tc, unsigned divisor)
   {
      uint64_t total_ram;
   if (os_get_total_physical_memory(&total_ram)) {
      tc->bytes_mapped_limit = total_ram / divisor;
   if (sizeof(void*) == 4)
         }
      const struct tc_renderpass_info *
   threaded_context_get_renderpass_info(struct threaded_context *tc)
   {
      assert(tc->renderpass_info && tc->options.parse_renderpass_info);
   struct tc_batch_rp_info *info = tc_batch_rp_info(tc->renderpass_info);
   while (1) {
      util_queue_fence_wait(&info->ready);
   if (!info->next)
               }
