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
      #include "perf/intel_perf.h"
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
   anv_utrace_delete_flush_data(struct u_trace_context *utctx,
         {
      struct anv_device *device =
                           if (flush->trace_bo) {
      assert(flush->batch_bo);
   anv_reloc_list_finish(&flush->relocs, &device->vk.alloc);
   anv_device_release_bo(device, flush->batch_bo);
                           }
      static void
   anv_device_utrace_emit_copy_ts_buffer(struct u_trace_context *utctx,
                           {
      struct anv_device *device =
         struct anv_utrace_flush_copy *flush = cmdstream;
   struct anv_address from_addr = (struct anv_address) {
         struct anv_address to_addr = (struct anv_address) {
            anv_genX(device->info, emit_so_memcpy)(&flush->memcpy_state,
      }
      VkResult
   anv_device_utrace_flush_cmd_buffers(struct anv_queue *queue,
                     {
      struct anv_device *device = queue->device;
   uint32_t utrace_copies = 0;
   uint32_t utraces = command_buffers_count_utraces(device,
                     if (!utraces) {
      *out_flush_data = NULL;
               VkResult result;
   struct anv_utrace_flush_copy *flush =
      vk_zalloc(&device->vk.alloc, sizeof(struct anv_utrace_flush_copy),
      if (!flush)
                     result = vk_sync_create(&device->vk, &device->physical->sync_syncobj_type,
         if (result != VK_SUCCESS)
            if (utrace_copies > 0) {
      result = anv_bo_pool_alloc(&device->utrace_bo_pool,
               if (result != VK_SUCCESS)
            result = anv_bo_pool_alloc(&device->utrace_bo_pool,
                     if (result != VK_SUCCESS)
            result = anv_reloc_list_init(&flush->relocs, &device->vk.alloc);
   if (result != VK_SUCCESS)
            flush->batch.alloc = &device->vk.alloc;
   flush->batch.relocs = &flush->relocs;
   anv_batch_set_storage(&flush->batch,
                  /* Emit the copies */
   anv_genX(device->info, emit_so_memcpy_init)(&flush->memcpy_state,
               for (uint32_t i = 0; i < cmd_buffer_count; i++) {
      if (cmd_buffers[i]->usage_flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) {
   intel_ds_queue_flush_data(&queue->ds, &cmd_buffers[i]->trace,
         } else {
      u_trace_clone_append(u_trace_begin_iterator(&cmd_buffers[i]->trace),
                           }
                     if (flush->batch.status != VK_SUCCESS) {
      result = flush->batch.status;
         } else {
      for (uint32_t i = 0; i < cmd_buffer_count; i++) {
      assert(cmd_buffers[i]->usage_flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
   intel_ds_queue_flush_data(&queue->ds, &cmd_buffers[i]->trace,
                                          error_batch:
         error_reloc_list:
         error_batch_buf:
         error_trace_buf:
         error_sync:
      vk_free(&device->vk.alloc, flush);
      }
      static void *
   anv_utrace_create_ts_buffer(struct u_trace_context *utctx, uint32_t size_b)
   {
      struct anv_device *device =
            struct anv_bo *bo = NULL;
   UNUSED VkResult result =
      anv_bo_pool_alloc(&device->utrace_bo_pool,
                        }
      static void
   anv_utrace_destroy_ts_buffer(struct u_trace_context *utctx, void *timestamps)
   {
      struct anv_device *device =
                     }
      static void
   anv_utrace_record_ts(struct u_trace *ut, void *cs,
               {
      struct anv_cmd_buffer *cmd_buffer =
         struct anv_device *device = cmd_buffer->device;
            enum anv_timestamp_capture_type capture_type =
      (end_of_pipe) ? ANV_TIMESTAMP_CAPTURE_END_OF_PIPE
      device->physical->cmd_emit_timestamp(&cmd_buffer->batch, device,
                        }
      static uint64_t
   anv_utrace_read_ts(struct u_trace_context *utctx,
         {
      struct anv_device *device =
         struct anv_bo *bo = timestamps;
            /* Only need to stall on results for the first entry: */
   if (idx == 0) {
      UNUSED VkResult result =
      vk_sync_wait(&device->vk,
               flush->sync,
                           /* Don't translate the no-timestamp marker: */
   if (ts[idx] == U_TRACE_NO_TIMESTAMP)
               }
      void
   anv_device_utrace_init(struct anv_device *device)
   {
      anv_bo_pool_init(&device->utrace_bo_pool, device, "utrace");
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
               enum intel_ds_stall_flag ret = 0;
   for (uint32_t i = 0; i < ARRAY_SIZE(anv_to_ds_flags); i++) {
      if (anv_to_ds_flags[i].anv & bits)
                  }
