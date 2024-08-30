   /*
   * Copyright © 2021 Intel Corporation
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
      #include "anv_private.h"
   #include "anv_internal_kernels.h"
      #include "ds/intel_tracepoints.h"
   #include "genxml/gen8_pack.h"
   #include "perf/intel_perf.h"
   #include "util/perf/cpu_trace.h"
      #include "vulkan/runtime/vk_common_entrypoints.h"
      /** Timestamp structure format */
   union anv_utrace_timestamp {
      /* Timestamp writtem by either 2 * MI_STORE_REGISTER_MEM or
   * PIPE_CONTROL.
   */
            /* Timestamp written by COMPUTE_WALKER::PostSync
   *
   * Layout is described in PRMs.
   * ATSM PRMs, Volume 2d: Command Reference: Structures, POSTSYNC_DATA:
   *
   *    "The timestamp layout :
   *        [0] = 32b Context Timestamp Start
   *        [1] = 32b Global Timestamp Start
   *        [2] = 32b Context Timestamp End
   *        [3] = 32b Global Timestamp End"
   */
      };
      static uint32_t
   command_buffers_count_utraces(struct anv_device *device,
                     {
      if (!u_trace_should_process(&device->ds.trace_context))
            uint32_t utraces = 0;
   for (uint32_t i = 0; i < cmd_buffer_count; i++) {
      if (u_trace_has_points(&cmd_buffers[i]->trace)) {
      utraces++;
   if (!(cmd_buffers[i]->usage_flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
                     }
      static void
   anv_utrace_delete_submit(struct u_trace_context *utctx, void *submit_data)
   {
      struct anv_device *device =
                           anv_state_stream_finish(&submit->dynamic_state_stream);
            if (submit->trace_bo)
            if (submit->batch_bo) {
      anv_reloc_list_finish(&submit->relocs);
                           }
      static void
   anv_device_utrace_emit_gfx_copy_ts_buffer(struct u_trace_context *utctx,
                           {
      struct anv_device *device =
         struct anv_utrace_submit *submit = cmdstream;
   struct anv_address from_addr = (struct anv_address) {
         struct anv_address to_addr = (struct anv_address) {
            anv_genX(device->info, emit_so_memcpy)(&submit->memcpy_state,
            }
      static void
   anv_device_utrace_emit_cs_copy_ts_buffer(struct u_trace_context *utctx,
                           {
      struct anv_device *device =
         struct anv_utrace_submit *submit = cmdstream;
   struct anv_address from_addr = (struct anv_address) {
         struct anv_address to_addr = (struct anv_address) {
            struct anv_state push_data_state =
      anv_genX(device->info, simple_shader_alloc_push)(
               *params = (struct anv_memcpy_params) {
      .copy = {
         },
   .src_addr = anv_address_physical(from_addr),
               anv_genX(device->info, emit_simple_shader_dispatch)(
      &submit->simple_state, DIV_ROUND_UP(params->copy.num_dwords, 4),
   }
      VkResult
   anv_device_utrace_flush_cmd_buffers(struct anv_queue *queue,
                     {
      struct anv_device *device = queue->device;
   uint32_t utrace_copies = 0;
   uint32_t utraces = command_buffers_count_utraces(device,
                     if (!utraces) {
      *out_submit = NULL;
               VkResult result;
   struct anv_utrace_submit *submit =
      vk_zalloc(&device->vk.alloc, sizeof(struct anv_utrace_submit),
      if (!submit)
                     result = vk_sync_create(&device->vk, &device->physical->sync_syncobj_type,
         if (result != VK_SUCCESS)
            if (utrace_copies > 0) {
      result = anv_bo_pool_alloc(&device->utrace_bo_pool,
               if (result != VK_SUCCESS)
            uint32_t batch_size = 512; /* 128 dwords of setup */
   if (intel_needs_workaround(device->info, 16013994831)) {
      /* Enable/Disable preemption at the begin/end */
   batch_size += 2 * (250 /* 250 MI_NOOPs*/ +
            }
   batch_size += 256 * utrace_copies; /* 64 dwords per copy */
            result = anv_bo_pool_alloc(&device->utrace_bo_pool,
               if (result != VK_SUCCESS)
            const bool uses_relocs = device->physical->uses_relocs;
   result = anv_reloc_list_init(&submit->relocs, &device->vk.alloc, uses_relocs);
   if (result != VK_SUCCESS)
            anv_state_stream_init(&submit->dynamic_state_stream,
         anv_state_stream_init(&submit->general_state_stream,
            submit->batch.alloc = &device->vk.alloc;
   submit->batch.relocs = &submit->relocs;
   anv_batch_set_storage(&submit->batch,
                  /* Only engine class where we support timestamp copies
   *
   * TODO: add INTEL_ENGINE_CLASS_COPY support (should be trivial ;)
   */
   assert(queue->family->engine_class == INTEL_ENGINE_CLASS_RENDER ||
                              anv_genX(device->info, emit_so_memcpy_init)(&submit->memcpy_state,
               uint32_t num_traces = 0;
   for (uint32_t i = 0; i < cmd_buffer_count; i++) {
      if (cmd_buffers[i]->usage_flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) {
      intel_ds_queue_flush_data(&queue->ds, &cmd_buffers[i]->trace,
      } else {
      num_traces += cmd_buffers[i]->trace.num_traces;
   u_trace_clone_append(u_trace_begin_iterator(&cmd_buffers[i]->trace),
                                                            } else {
               submit->simple_state = (struct anv_simple_shader) {
      .device               = device,
   .dynamic_state_stream = &submit->dynamic_state_stream,
   .general_state_stream = &submit->general_state_stream,
   .batch                = &submit->batch,
   .kernel               = device->internal_kernels[
                           uint32_t num_traces = 0;
   for (uint32_t i = 0; i < cmd_buffer_count; i++) {
      num_traces += cmd_buffers[i]->trace.num_traces;
   if (cmd_buffers[i]->usage_flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) {
      intel_ds_queue_flush_data(&queue->ds, &cmd_buffers[i]->trace,
      } else {
      num_traces += cmd_buffers[i]->trace.num_traces;
   u_trace_clone_append(u_trace_begin_iterator(&cmd_buffers[i]->trace),
                                                                        if (submit->batch.status != VK_SUCCESS) {
      result = submit->batch.status;
         } else {
      for (uint32_t i = 0; i < cmd_buffer_count; i++) {
      assert(cmd_buffers[i]->usage_flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
   intel_ds_queue_flush_data(&queue->ds, &cmd_buffers[i]->trace,
                                          error_batch:
         error_reloc_list:
         error_batch_buf:
         error_trace_buf:
         error_sync:
      intel_ds_flush_data_fini(&submit->ds);
   vk_free(&device->vk.alloc, submit);
      }
      static void *
   anv_utrace_create_ts_buffer(struct u_trace_context *utctx, uint32_t size_b)
   {
      struct anv_device *device =
            uint32_t anv_ts_size_b = (size_b / sizeof(uint64_t)) *
            struct anv_bo *bo = NULL;
   UNUSED VkResult result =
      anv_bo_pool_alloc(&device->utrace_bo_pool,
                        #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
      if (device->physical->memory.need_flush)
      #endif
            }
      static void
   anv_utrace_destroy_ts_buffer(struct u_trace_context *utctx, void *timestamps)
   {
      struct anv_device *device =
                     }
      static void
   anv_utrace_record_ts(struct u_trace *ut, void *cs,
               {
      struct anv_device *device =
         struct anv_cmd_buffer *cmd_buffer =
         /* cmd_buffer is only valid if cs == NULL */
   struct anv_batch *batch = cs != NULL ? cs : &cmd_buffer->batch;
            struct anv_address ts_address = (struct anv_address) {
      .bo = bo,
               /* Is this a end of compute trace point? */
   const bool is_end_compute =
            enum anv_timestamp_capture_type capture_type = end_of_pipe ?
      is_end_compute ? ANV_TIMESTAMP_REWRITE_COMPUTE_WALKER :
      device->physical->cmd_emit_timestamp(batch, device, ts_address,
                     if (is_end_compute)
      }
      static uint64_t
   anv_utrace_read_ts(struct u_trace_context *utctx,
         {
      struct anv_device *device =
         struct anv_bo *bo = timestamps;
            /* Only need to stall on results for the first entry: */
   if (idx == 0) {
      MESA_TRACE_SCOPE("anv utrace wait timestamps");
   UNUSED VkResult result =
      vk_sync_wait(&device->vk,
               submit->sync,
                           /* Don't translate the no-timestamp marker: */
   if (ts[idx].timestamp == U_TRACE_NO_TIMESTAMP)
            /* Detect a 16bytes timestamp write */
   if (ts[idx].compute_walker[2] != 0 || ts[idx].compute_walker[3] != 0) {
      /* The timestamp written by COMPUTE_WALKER::PostSync only as 32bits. We
   * need to rebuild the full 64bits using the previous timestamp. We
   * assume that utrace is reading the timestamp in order. Anyway
   * timestamp rollover on 32bits in a few minutes so in most cases that
   * should be correct.
   */
   uint64_t timestamp =
                                          }
      void
   anv_device_utrace_init(struct anv_device *device)
   {
      anv_bo_pool_init(&device->utrace_bo_pool, device, "utrace",
         intel_ds_device_init(&device->ds, device->info, device->fd,
               u_trace_context_init(&device->ds.trace_context,
                        &device->ds,
   anv_utrace_create_ts_buffer,
         for (uint32_t q = 0; q < device->queue_count; q++) {
               intel_ds_device_init_queue(&device->ds, &queue->ds, "%s%u",
               }
      void
   anv_device_utrace_finish(struct anv_device *device)
   {
      intel_ds_device_process(&device->ds, true);
   intel_ds_device_fini(&device->ds);
      }
      enum intel_ds_stall_flag
   anv_pipe_flush_bit_to_ds_stall_flag(enum anv_pipe_bits bits)
   {
      static const struct {
      enum anv_pipe_bits anv;
      } anv_to_ds_flags[] = {
      { .anv = ANV_PIPE_DEPTH_CACHE_FLUSH_BIT,            .ds = INTEL_DS_DEPTH_CACHE_FLUSH_BIT, },
   { .anv = ANV_PIPE_DATA_CACHE_FLUSH_BIT,             .ds = INTEL_DS_DATA_CACHE_FLUSH_BIT, },
   { .anv = ANV_PIPE_TILE_CACHE_FLUSH_BIT,             .ds = INTEL_DS_TILE_CACHE_FLUSH_BIT, },
   { .anv = ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT,    .ds = INTEL_DS_RENDER_TARGET_CACHE_FLUSH_BIT, },
   { .anv = ANV_PIPE_STATE_CACHE_INVALIDATE_BIT,       .ds = INTEL_DS_STATE_CACHE_INVALIDATE_BIT, },
   { .anv = ANV_PIPE_CONSTANT_CACHE_INVALIDATE_BIT,    .ds = INTEL_DS_CONST_CACHE_INVALIDATE_BIT, },
   { .anv = ANV_PIPE_VF_CACHE_INVALIDATE_BIT,          .ds = INTEL_DS_VF_CACHE_INVALIDATE_BIT, },
   { .anv = ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT,     .ds = INTEL_DS_TEXTURE_CACHE_INVALIDATE_BIT, },
   { .anv = ANV_PIPE_INSTRUCTION_CACHE_INVALIDATE_BIT, .ds = INTEL_DS_INST_CACHE_INVALIDATE_BIT, },
   { .anv = ANV_PIPE_DEPTH_STALL_BIT,                  .ds = INTEL_DS_DEPTH_STALL_BIT, },
   { .anv = ANV_PIPE_CS_STALL_BIT,                     .ds = INTEL_DS_CS_STALL_BIT, },
   { .anv = ANV_PIPE_HDC_PIPELINE_FLUSH_BIT,           .ds = INTEL_DS_HDC_PIPELINE_FLUSH_BIT, },
   { .anv = ANV_PIPE_STALL_AT_SCOREBOARD_BIT,          .ds = INTEL_DS_STALL_AT_SCOREBOARD_BIT, },
   { .anv = ANV_PIPE_UNTYPED_DATAPORT_CACHE_FLUSH_BIT, .ds = INTEL_DS_UNTYPED_DATAPORT_CACHE_FLUSH_BIT, },
   { .anv = ANV_PIPE_PSS_STALL_SYNC_BIT,               .ds = INTEL_DS_PSS_STALL_SYNC_BIT, },
   { .anv = ANV_PIPE_END_OF_PIPE_SYNC_BIT,             .ds = INTEL_DS_END_OF_PIPE_BIT, },
               enum intel_ds_stall_flag ret = 0;
   for (uint32_t i = 0; i < ARRAY_SIZE(anv_to_ds_flags); i++) {
      if (anv_to_ds_flags[i].anv & bits)
                  }
      void anv_CmdBeginDebugUtilsLabelEXT(
      VkCommandBuffer _commandBuffer,
      {
                           }
      void anv_CmdEndDebugUtilsLabelEXT(VkCommandBuffer _commandBuffer)
   {
               if (cmd_buffer->vk.labels.size > 0) {
      const VkDebugUtilsLabelEXT *label =
            trace_intel_end_cmd_buffer_annotation(&cmd_buffer->trace,
                        }
      void
   anv_queue_trace(struct anv_queue *queue, const char *label, bool frame, bool begin)
   {
               VkResult result;
   struct anv_utrace_submit *submit =
      vk_zalloc(&device->vk.alloc, sizeof(struct anv_utrace_submit),
      if (!submit)
                              result = vk_sync_create(&device->vk, &device->physical->sync_syncobj_type,
         if (result != VK_SUCCESS)
            result = anv_bo_pool_alloc(&device->utrace_bo_pool, 4096,
         if (result != VK_SUCCESS)
            const bool uses_relocs = device->physical->uses_relocs;
   result = anv_reloc_list_init(&submit->relocs, &device->vk.alloc, uses_relocs);
   if (result != VK_SUCCESS)
            submit->batch.alloc = &device->vk.alloc;
   submit->batch.relocs = &submit->relocs;
   anv_batch_set_storage(&submit->batch,
                  if (frame) {
      if (begin)
         else
      trace_intel_end_frame(&submit->ds.trace, &submit->batch,
   } else {
      if (begin) {
         } else {
      trace_intel_end_queue_annotation(&submit->ds.trace,
                              anv_batch_emit(&submit->batch, GFX8_MI_BATCH_BUFFER_END, bbs);
            if (submit->batch.status != VK_SUCCESS) {
      result = submit->batch.status;
                        pthread_mutex_lock(&device->mutex);
   device->kmd_backend->queue_exec_trace(queue, submit);
                  error_reloc_list:
         error_batch_bo:
         error_sync:
         error_trace:
      intel_ds_flush_data_fini(&submit->ds);
      }
      void
   anv_QueueBeginDebugUtilsLabelEXT(
      VkQueue _queue,
      {
                        anv_queue_trace(queue, pLabelInfo->pLabelName,
      }
      void
   anv_QueueEndDebugUtilsLabelEXT(VkQueue _queue)
   {
               if (queue->vk.labels.size > 0) {
      const VkDebugUtilsLabelEXT *label =
         anv_queue_trace(queue, label->pLabelName,
                           }
