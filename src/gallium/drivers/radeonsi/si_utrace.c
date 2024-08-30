   /*
   * Copyright 2023 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_utrace.h"
   #include "si_perfetto.h"
   #include "amd/common/ac_gpu_info.h"
      #include "util/u_trace_gallium.h"
   #include "util/hash_table.h"
         static void si_utrace_record_ts(struct u_trace *trace, void *cs, void *timestamps, 
         {
      struct si_context *ctx = container_of(trace, struct si_context, trace);
   struct pipe_resource *buffer = timestamps;
            if (ctx->gfx_cs.current.buf == ctx->last_timestamp_cmd && 
      ctx->gfx_cs.current.cdw == ctx->last_timestamp_cmd_cdw) {
   uint64_t *ts = si_buffer_map(ctx, ts_bo, PIPE_MAP_READ);
   ts[idx] = U_TRACE_NO_TIMESTAMP;
                        si_emit_ts(ctx, ts_bo, ts_offset);
   ctx->last_timestamp_cmd = ctx->gfx_cs.current.buf;
      }
      static uint64_t si_utrace_read_ts(struct u_trace_context *utctx, void *timestamps, 
         {
      struct si_context *ctx = container_of(utctx, struct si_context, ds.trace_context);
   struct pipe_resource *buffer = timestamps;
               /* Don't translate the no-timestamp marker: */
   if (ts[idx] == U_TRACE_NO_TIMESTAMP)
               }
      static void si_utrace_delete_flush_data(struct u_trace_context *utctx, void *flush_data)
   {
         }
      void si_utrace_init(struct si_context *sctx)
   {
      char buf[64];
   snprintf(buf, sizeof(buf), "%u:%u:%u:%u:%u", sctx->screen->info.pci.domain,
                        si_ds_device_init(&sctx->ds, &sctx->screen->info, gpu_id, AMD_DS_API_OPENGL);
   u_trace_pipe_context_init(&sctx->ds.trace_context, &sctx->b, si_utrace_record_ts,
               }
      void si_utrace_fini(struct si_context *sctx)
   {
         }
      void si_utrace_flush(struct si_context *sctx, uint64_t submission_id)
   {
      struct si_ds_flush_data *flush_data = malloc(sizeof(*flush_data));
   si_ds_flush_data_init(flush_data, &sctx->ds_queue, submission_id);
      }
