   /*
   * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
   * Copyright 2014 Marek Olšák <marek.olsak@amd.com>
   * Copyright 2018 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_query.h"
   #include "si_build_pm4.h"
      #include "amd/common/sid.h"
   #include "si_pipe.h"
   #include "util/os_time.h"
   #include "util/u_memory.h"
   #include "util/u_suballoc.h"
   #include "util/u_upload_mgr.h"
      static const struct si_query_ops query_hw_ops;
   static const struct si_query_hw_ops query_hw_default_hw_ops;
   static const struct si_query_ops sw_query_ops;
      struct si_hw_query_params {
      unsigned start_offset;
   unsigned end_offset;
   unsigned fence_offset;
   unsigned pair_stride;
      };
      /* Queries without buffer handling or suspend/resume. */
   struct si_query_sw {
               uint64_t begin_result;
            uint64_t begin_time;
            /* Fence for GPU_FINISHED. */
      };
      static void si_query_sw_destroy(struct si_context *sctx, struct si_query *squery)
   {
               sctx->b.screen->fence_reference(sctx->b.screen, &query->fence, NULL);
      }
      static enum radeon_value_id winsys_id_from_type(unsigned type)
   {
      switch (type) {
   case SI_QUERY_REQUESTED_VRAM:
         case SI_QUERY_REQUESTED_GTT:
         case SI_QUERY_MAPPED_VRAM:
         case SI_QUERY_MAPPED_GTT:
         case SI_QUERY_SLAB_WASTED_VRAM:
         case SI_QUERY_SLAB_WASTED_GTT:
         case SI_QUERY_BUFFER_WAIT_TIME:
         case SI_QUERY_NUM_MAPPED_BUFFERS:
         case SI_QUERY_NUM_GFX_IBS:
         case SI_QUERY_GFX_BO_LIST_SIZE:
         case SI_QUERY_GFX_IB_SIZE:
         case SI_QUERY_NUM_BYTES_MOVED:
         case SI_QUERY_NUM_EVICTIONS:
         case SI_QUERY_NUM_VRAM_CPU_PAGE_FAULTS:
         case SI_QUERY_VRAM_USAGE:
         case SI_QUERY_VRAM_VIS_USAGE:
         case SI_QUERY_GTT_USAGE:
         case SI_QUERY_GPU_TEMPERATURE:
         case SI_QUERY_CURRENT_GPU_SCLK:
         case SI_QUERY_CURRENT_GPU_MCLK:
         case SI_QUERY_CS_THREAD_BUSY:
         default:
            }
      static bool si_query_sw_begin(struct si_context *sctx, struct si_query *squery)
   {
      struct si_query_sw *query = (struct si_query_sw *)squery;
            switch (query->b.type) {
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_GPU_FINISHED:
         case SI_QUERY_DRAW_CALLS:
      query->begin_result = sctx->num_draw_calls;
      case SI_QUERY_DECOMPRESS_CALLS:
      query->begin_result = sctx->num_decompress_calls;
      case SI_QUERY_COMPUTE_CALLS:
      query->begin_result = sctx->num_compute_calls;
      case SI_QUERY_CP_DMA_CALLS:
      query->begin_result = sctx->num_cp_dma_calls;
      case SI_QUERY_NUM_VS_FLUSHES:
      query->begin_result = sctx->num_vs_flushes;
      case SI_QUERY_NUM_PS_FLUSHES:
      query->begin_result = sctx->num_ps_flushes;
      case SI_QUERY_NUM_CS_FLUSHES:
      query->begin_result = sctx->num_cs_flushes;
      case SI_QUERY_NUM_CB_CACHE_FLUSHES:
      query->begin_result = sctx->num_cb_cache_flushes;
      case SI_QUERY_NUM_DB_CACHE_FLUSHES:
      query->begin_result = sctx->num_db_cache_flushes;
      case SI_QUERY_NUM_L2_INVALIDATES:
      query->begin_result = sctx->num_L2_invalidates;
      case SI_QUERY_NUM_L2_WRITEBACKS:
      query->begin_result = sctx->num_L2_writebacks;
      case SI_QUERY_NUM_RESIDENT_HANDLES:
      query->begin_result = sctx->num_resident_handles;
      case SI_QUERY_TC_OFFLOADED_SLOTS:
      query->begin_result = sctx->tc ? sctx->tc->num_offloaded_slots : 0;
      case SI_QUERY_TC_DIRECT_SLOTS:
      query->begin_result = sctx->tc ? sctx->tc->num_direct_slots : 0;
      case SI_QUERY_TC_NUM_SYNCS:
      query->begin_result = sctx->tc ? sctx->tc->num_syncs : 0;
      case SI_QUERY_REQUESTED_VRAM:
   case SI_QUERY_REQUESTED_GTT:
   case SI_QUERY_MAPPED_VRAM:
   case SI_QUERY_MAPPED_GTT:
   case SI_QUERY_SLAB_WASTED_VRAM:
   case SI_QUERY_SLAB_WASTED_GTT:
   case SI_QUERY_VRAM_USAGE:
   case SI_QUERY_VRAM_VIS_USAGE:
   case SI_QUERY_GTT_USAGE:
   case SI_QUERY_GPU_TEMPERATURE:
   case SI_QUERY_CURRENT_GPU_SCLK:
   case SI_QUERY_CURRENT_GPU_MCLK:
   case SI_QUERY_BACK_BUFFER_PS_DRAW_RATIO:
   case SI_QUERY_NUM_MAPPED_BUFFERS:
      query->begin_result = 0;
      case SI_QUERY_BUFFER_WAIT_TIME:
   case SI_QUERY_GFX_IB_SIZE:
   case SI_QUERY_NUM_GFX_IBS:
   case SI_QUERY_NUM_BYTES_MOVED:
   case SI_QUERY_NUM_EVICTIONS:
   case SI_QUERY_NUM_VRAM_CPU_PAGE_FAULTS: {
      enum radeon_value_id ws_id = winsys_id_from_type(query->b.type);
   query->begin_result = sctx->ws->query_value(sctx->ws, ws_id);
      }
   case SI_QUERY_GFX_BO_LIST_SIZE:
      ws_id = winsys_id_from_type(query->b.type);
   query->begin_result = sctx->ws->query_value(sctx->ws, ws_id);
   query->begin_time = sctx->ws->query_value(sctx->ws, RADEON_NUM_GFX_IBS);
      case SI_QUERY_CS_THREAD_BUSY:
      ws_id = winsys_id_from_type(query->b.type);
   query->begin_result = sctx->ws->query_value(sctx->ws, ws_id);
   query->begin_time = os_time_get_nano();
      case SI_QUERY_GALLIUM_THREAD_BUSY:
      query->begin_result = sctx->tc ? util_queue_get_thread_time_nano(&sctx->tc->queue, 0) : 0;
   query->begin_time = os_time_get_nano();
      case SI_QUERY_GPU_LOAD:
   case SI_QUERY_GPU_SHADERS_BUSY:
   case SI_QUERY_GPU_TA_BUSY:
   case SI_QUERY_GPU_GDS_BUSY:
   case SI_QUERY_GPU_VGT_BUSY:
   case SI_QUERY_GPU_IA_BUSY:
   case SI_QUERY_GPU_SX_BUSY:
   case SI_QUERY_GPU_WD_BUSY:
   case SI_QUERY_GPU_BCI_BUSY:
   case SI_QUERY_GPU_SC_BUSY:
   case SI_QUERY_GPU_PA_BUSY:
   case SI_QUERY_GPU_DB_BUSY:
   case SI_QUERY_GPU_CP_BUSY:
   case SI_QUERY_GPU_CB_BUSY:
   case SI_QUERY_GPU_SDMA_BUSY:
   case SI_QUERY_GPU_PFP_BUSY:
   case SI_QUERY_GPU_MEQ_BUSY:
   case SI_QUERY_GPU_ME_BUSY:
   case SI_QUERY_GPU_SURF_SYNC_BUSY:
   case SI_QUERY_GPU_CP_DMA_BUSY:
   case SI_QUERY_GPU_SCRATCH_RAM_BUSY:
      query->begin_result = si_begin_counter(sctx->screen, query->b.type);
      case SI_QUERY_NUM_COMPILATIONS:
      query->begin_result = p_atomic_read(&sctx->screen->num_compilations);
      case SI_QUERY_NUM_SHADERS_CREATED:
      query->begin_result = p_atomic_read(&sctx->screen->num_shaders_created);
      case SI_QUERY_LIVE_SHADER_CACHE_HITS:
      query->begin_result = sctx->screen->live_shader_cache.hits;
      case SI_QUERY_LIVE_SHADER_CACHE_MISSES:
      query->begin_result = sctx->screen->live_shader_cache.misses;
      case SI_QUERY_MEMORY_SHADER_CACHE_HITS:
      query->begin_result = sctx->screen->num_memory_shader_cache_hits;
      case SI_QUERY_MEMORY_SHADER_CACHE_MISSES:
      query->begin_result = sctx->screen->num_memory_shader_cache_misses;
      case SI_QUERY_DISK_SHADER_CACHE_HITS:
      query->begin_result = sctx->screen->num_disk_shader_cache_hits;
      case SI_QUERY_DISK_SHADER_CACHE_MISSES:
      query->begin_result = sctx->screen->num_disk_shader_cache_misses;
      case SI_QUERY_GPIN_ASIC_ID:
   case SI_QUERY_GPIN_NUM_SIMD:
   case SI_QUERY_GPIN_NUM_RB:
   case SI_QUERY_GPIN_NUM_SPI:
   case SI_QUERY_GPIN_NUM_SE:
         default:
                     }
      static bool si_query_sw_end(struct si_context *sctx, struct si_query *squery)
   {
      struct si_query_sw *query = (struct si_query_sw *)squery;
            switch (query->b.type) {
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
         case PIPE_QUERY_GPU_FINISHED:
      sctx->b.flush(&sctx->b, &query->fence, PIPE_FLUSH_DEFERRED);
      case SI_QUERY_DRAW_CALLS:
      query->end_result = sctx->num_draw_calls;
      case SI_QUERY_DECOMPRESS_CALLS:
      query->end_result = sctx->num_decompress_calls;
      case SI_QUERY_COMPUTE_CALLS:
      query->end_result = sctx->num_compute_calls;
      case SI_QUERY_CP_DMA_CALLS:
      query->end_result = sctx->num_cp_dma_calls;
      case SI_QUERY_NUM_VS_FLUSHES:
      query->end_result = sctx->num_vs_flushes;
      case SI_QUERY_NUM_PS_FLUSHES:
      query->end_result = sctx->num_ps_flushes;
      case SI_QUERY_NUM_CS_FLUSHES:
      query->end_result = sctx->num_cs_flushes;
      case SI_QUERY_NUM_CB_CACHE_FLUSHES:
      query->end_result = sctx->num_cb_cache_flushes;
      case SI_QUERY_NUM_DB_CACHE_FLUSHES:
      query->end_result = sctx->num_db_cache_flushes;
      case SI_QUERY_NUM_L2_INVALIDATES:
      query->end_result = sctx->num_L2_invalidates;
      case SI_QUERY_NUM_L2_WRITEBACKS:
      query->end_result = sctx->num_L2_writebacks;
      case SI_QUERY_NUM_RESIDENT_HANDLES:
      query->end_result = sctx->num_resident_handles;
      case SI_QUERY_TC_OFFLOADED_SLOTS:
      query->end_result = sctx->tc ? sctx->tc->num_offloaded_slots : 0;
      case SI_QUERY_TC_DIRECT_SLOTS:
      query->end_result = sctx->tc ? sctx->tc->num_direct_slots : 0;
      case SI_QUERY_TC_NUM_SYNCS:
      query->end_result = sctx->tc ? sctx->tc->num_syncs : 0;
      case SI_QUERY_REQUESTED_VRAM:
   case SI_QUERY_REQUESTED_GTT:
   case SI_QUERY_MAPPED_VRAM:
   case SI_QUERY_MAPPED_GTT:
   case SI_QUERY_SLAB_WASTED_VRAM:
   case SI_QUERY_SLAB_WASTED_GTT:
   case SI_QUERY_VRAM_USAGE:
   case SI_QUERY_VRAM_VIS_USAGE:
   case SI_QUERY_GTT_USAGE:
   case SI_QUERY_GPU_TEMPERATURE:
   case SI_QUERY_CURRENT_GPU_SCLK:
   case SI_QUERY_CURRENT_GPU_MCLK:
   case SI_QUERY_BUFFER_WAIT_TIME:
   case SI_QUERY_GFX_IB_SIZE:
   case SI_QUERY_NUM_MAPPED_BUFFERS:
   case SI_QUERY_NUM_GFX_IBS:
   case SI_QUERY_NUM_BYTES_MOVED:
   case SI_QUERY_NUM_EVICTIONS:
   case SI_QUERY_NUM_VRAM_CPU_PAGE_FAULTS: {
      enum radeon_value_id ws_id = winsys_id_from_type(query->b.type);
   query->end_result = sctx->ws->query_value(sctx->ws, ws_id);
      }
   case SI_QUERY_GFX_BO_LIST_SIZE:
      ws_id = winsys_id_from_type(query->b.type);
   query->end_result = sctx->ws->query_value(sctx->ws, ws_id);
   query->end_time = sctx->ws->query_value(sctx->ws, RADEON_NUM_GFX_IBS);
      case SI_QUERY_CS_THREAD_BUSY:
      ws_id = winsys_id_from_type(query->b.type);
   query->end_result = sctx->ws->query_value(sctx->ws, ws_id);
   query->end_time = os_time_get_nano();
      case SI_QUERY_GALLIUM_THREAD_BUSY:
      query->end_result = sctx->tc ? util_queue_get_thread_time_nano(&sctx->tc->queue, 0) : 0;
   query->end_time = os_time_get_nano();
      case SI_QUERY_GPU_LOAD:
   case SI_QUERY_GPU_SHADERS_BUSY:
   case SI_QUERY_GPU_TA_BUSY:
   case SI_QUERY_GPU_GDS_BUSY:
   case SI_QUERY_GPU_VGT_BUSY:
   case SI_QUERY_GPU_IA_BUSY:
   case SI_QUERY_GPU_SX_BUSY:
   case SI_QUERY_GPU_WD_BUSY:
   case SI_QUERY_GPU_BCI_BUSY:
   case SI_QUERY_GPU_SC_BUSY:
   case SI_QUERY_GPU_PA_BUSY:
   case SI_QUERY_GPU_DB_BUSY:
   case SI_QUERY_GPU_CP_BUSY:
   case SI_QUERY_GPU_CB_BUSY:
   case SI_QUERY_GPU_SDMA_BUSY:
   case SI_QUERY_GPU_PFP_BUSY:
   case SI_QUERY_GPU_MEQ_BUSY:
   case SI_QUERY_GPU_ME_BUSY:
   case SI_QUERY_GPU_SURF_SYNC_BUSY:
   case SI_QUERY_GPU_CP_DMA_BUSY:
   case SI_QUERY_GPU_SCRATCH_RAM_BUSY:
      query->end_result = si_end_counter(sctx->screen, query->b.type, query->begin_result);
   query->begin_result = 0;
      case SI_QUERY_NUM_COMPILATIONS:
      query->end_result = p_atomic_read(&sctx->screen->num_compilations);
      case SI_QUERY_NUM_SHADERS_CREATED:
      query->end_result = p_atomic_read(&sctx->screen->num_shaders_created);
      case SI_QUERY_BACK_BUFFER_PS_DRAW_RATIO:
      query->end_result = sctx->last_tex_ps_draw_ratio;
      case SI_QUERY_LIVE_SHADER_CACHE_HITS:
      query->end_result = sctx->screen->live_shader_cache.hits;
      case SI_QUERY_LIVE_SHADER_CACHE_MISSES:
      query->end_result = sctx->screen->live_shader_cache.misses;
      case SI_QUERY_MEMORY_SHADER_CACHE_HITS:
      query->end_result = sctx->screen->num_memory_shader_cache_hits;
      case SI_QUERY_MEMORY_SHADER_CACHE_MISSES:
      query->end_result = sctx->screen->num_memory_shader_cache_misses;
      case SI_QUERY_DISK_SHADER_CACHE_HITS:
      query->end_result = sctx->screen->num_disk_shader_cache_hits;
      case SI_QUERY_DISK_SHADER_CACHE_MISSES:
      query->end_result = sctx->screen->num_disk_shader_cache_misses;
      case SI_QUERY_GPIN_ASIC_ID:
   case SI_QUERY_GPIN_NUM_SIMD:
   case SI_QUERY_GPIN_NUM_RB:
   case SI_QUERY_GPIN_NUM_SPI:
   case SI_QUERY_GPIN_NUM_SE:
         default:
                     }
      static bool si_query_sw_get_result(struct si_context *sctx, struct si_query *squery, bool wait,
         {
               switch (query->b.type) {
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
      /* Convert from cycles per millisecond to cycles per second (Hz). */
   result->timestamp_disjoint.frequency = (uint64_t)sctx->screen->info.clock_crystal_freq * 1000;
   result->timestamp_disjoint.disjoint = false;
      case PIPE_QUERY_GPU_FINISHED: {
      struct pipe_screen *screen = sctx->b.screen;
            result->b = screen->fence_finish(screen, ctx, query->fence, wait ? OS_TIMEOUT_INFINITE : 0);
               case SI_QUERY_GFX_BO_LIST_SIZE:
      result->u64 =
            case SI_QUERY_CS_THREAD_BUSY:
   case SI_QUERY_GALLIUM_THREAD_BUSY:
      result->u64 =
            case SI_QUERY_GPIN_ASIC_ID:
      result->u32 = 0;
      case SI_QUERY_GPIN_NUM_SIMD:
      result->u32 = sctx->screen->info.num_cu;
      case SI_QUERY_GPIN_NUM_RB:
      result->u32 = sctx->screen->info.max_render_backends;
      case SI_QUERY_GPIN_NUM_SPI:
      result->u32 = 1; /* all supported chips have one SPI per SE */
      case SI_QUERY_GPIN_NUM_SE:
      result->u32 = sctx->screen->info.max_se;
                        switch (query->b.type) {
   case SI_QUERY_BUFFER_WAIT_TIME:
   case SI_QUERY_GPU_TEMPERATURE:
      result->u64 /= 1000;
      case SI_QUERY_CURRENT_GPU_SCLK:
   case SI_QUERY_CURRENT_GPU_MCLK:
      result->u64 *= 1000000;
                  }
      static struct pipe_query *si_query_sw_create(unsigned query_type)
   {
               query = CALLOC_STRUCT(si_query_sw);
   if (!query)
            query->b.type = query_type;
               }
      void si_query_buffer_destroy(struct si_screen *sscreen, struct si_query_buffer *buffer)
   {
               /* Release all query buffers. */
   while (prev) {
      struct si_query_buffer *qbuf = prev;
   prev = prev->previous;
   si_resource_reference(&qbuf->buf, NULL);
                  }
      void si_query_buffer_reset(struct si_context *sctx, struct si_query_buffer *buffer)
   {
      /* Discard all query buffers except for the oldest. */
   while (buffer->previous) {
      struct si_query_buffer *qbuf = buffer->previous;
            si_resource_reference(&buffer->buf, NULL);
   buffer->buf = qbuf->buf; /* move ownership */
      }
            if (!buffer->buf)
            /* Discard even the oldest buffer if it can't be mapped without a stall. */
   if (si_cs_is_buffer_referenced(sctx, buffer->buf->buf, RADEON_USAGE_READWRITE) ||
      !sctx->ws->buffer_wait(sctx->ws, buffer->buf->buf, 0, RADEON_USAGE_READWRITE)) {
      } else {
            }
      bool si_query_buffer_alloc(struct si_context *sctx, struct si_query_buffer *buffer,
               {
      bool unprepared = buffer->unprepared;
            if (!buffer->buf || buffer->results_end + size > buffer->buf->b.b.width0) {
      if (buffer->buf) {
      struct si_query_buffer *qbuf = MALLOC_STRUCT(si_query_buffer);
   memcpy(qbuf, buffer, sizeof(*qbuf));
      }
            /* Queries are normally read by the CPU after
   * being written by the gpu, hence staging is probably a good
   * usage pattern.
   */
   struct si_screen *screen = sctx->screen;
   unsigned buf_size = MAX2(size, screen->info.min_alloc_size);
   buffer->buf = si_resource(pipe_buffer_create(&screen->b, 0, PIPE_USAGE_STAGING, buf_size));
   if (unlikely(!buffer->buf))
                     if (unprepared && prepare_buffer) {
      if (unlikely(!prepare_buffer(sctx, buffer))) {
      si_resource_reference(&buffer->buf, NULL);
                     }
      static void si_query_hw_destroy(struct si_context *sctx, struct si_query *squery)
   {
               si_query_buffer_destroy(sctx->screen, &query->buffer);
   si_resource_reference(&query->workaround_buf, NULL);
      }
      static bool si_query_hw_prepare_buffer(struct si_context *sctx, struct si_query_buffer *qbuf)
   {
      struct si_query_hw *query = container_of(qbuf, struct si_query_hw, buffer);
            /* The caller ensures that the buffer is currently unused by the GPU. */
   uint32_t *results = screen->ws->buffer_map(sctx->ws, qbuf->buf->buf, NULL,
         if (!results)
                     if (query->b.type == PIPE_QUERY_OCCLUSION_COUNTER ||
      query->b.type == PIPE_QUERY_OCCLUSION_PREDICATE ||
   query->b.type == PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE) {
   unsigned max_rbs = screen->info.max_render_backends;
   uint64_t enabled_rb_mask = screen->info.enabled_rb_mask;
   unsigned num_results;
            /* Set top bits for unused backends. */
   num_results = qbuf->buf->b.b.width0 / query->result_size;
   for (j = 0; j < num_results; j++) {
      for (i = 0; i < max_rbs; i++) {
      if (!(enabled_rb_mask & (1ull << i))) {
      results[(i * 4) + 1] = 0x80000000;
         }
                     }
      static unsigned si_query_pipestats_num_results(struct si_screen *sscreen)
   {
         }
      static unsigned si_query_pipestat_dw_offset(enum pipe_statistics_query_index index)
   {
      switch (index) {
   case PIPE_STAT_QUERY_PS_INVOCATIONS: return 0;
   case PIPE_STAT_QUERY_C_PRIMITIVES: return 2;
   case PIPE_STAT_QUERY_C_INVOCATIONS: return 4;
   case PIPE_STAT_QUERY_VS_INVOCATIONS: return 6;
   case PIPE_STAT_QUERY_GS_INVOCATIONS: return 8;
   case PIPE_STAT_QUERY_GS_PRIMITIVES: return 10;
   case PIPE_STAT_QUERY_IA_PRIMITIVES: return 12;
   case PIPE_STAT_QUERY_IA_VERTICES: return 14;
   case PIPE_STAT_QUERY_HS_INVOCATIONS: return 16;
   case PIPE_STAT_QUERY_DS_INVOCATIONS: return 18;
   case PIPE_STAT_QUERY_CS_INVOCATIONS: return 20;
   /* gfx11: MS_INVOCATIONS */
   /* gfx11: MS_PRIMITIVES */
   /* gfx11: TS_INVOCATIONS */
   default:
         }
      }
      unsigned si_query_pipestat_end_dw_offset(struct si_screen *sscreen,
         {
         }
      static struct pipe_query *si_query_hw_create(struct si_screen *sscreen, unsigned query_type,
         {
      struct si_query_hw *query = CALLOC_STRUCT(si_query_hw);
   if (!query)
            query->b.type = query_type;
   query->b.ops = &query_hw_ops;
            switch (query_type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      query->result_size = 16 * sscreen->info.max_render_backends;
   query->result_size += 16; /* for the fence + alignment */
   query->b.num_cs_dw_suspend = 6 + si_cp_write_fence_dwords(sscreen);
      case PIPE_QUERY_TIME_ELAPSED:
      query->result_size = 24;
   query->b.num_cs_dw_suspend = 8 + si_cp_write_fence_dwords(sscreen);
      case PIPE_QUERY_TIMESTAMP:
      query->result_size = 16;
   query->b.num_cs_dw_suspend = 8 + si_cp_write_fence_dwords(sscreen);
   query->flags = SI_QUERY_HW_FLAG_NO_START;
      case PIPE_QUERY_PRIMITIVES_EMITTED:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_SO_STATISTICS:
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      /* NumPrimitivesWritten, PrimitiveStorageNeeded. */
   query->result_size = 32;
   query->b.num_cs_dw_suspend = 6;
   query->stream = index;
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      /* NumPrimitivesWritten, PrimitiveStorageNeeded. */
   query->result_size = 32 * SI_MAX_STREAMS;
   query->b.num_cs_dw_suspend = 6 * SI_MAX_STREAMS;
      case PIPE_QUERY_PIPELINE_STATISTICS:
      query->result_size = si_query_pipestats_num_results(sscreen) * 16;
   query->result_size += 8; /* for the fence + alignment */
   query->b.num_cs_dw_suspend = 6 + si_cp_write_fence_dwords(sscreen);
   query->index = index;
   if ((index == PIPE_STAT_QUERY_GS_PRIMITIVES || index == PIPE_STAT_QUERY_GS_INVOCATIONS) &&
      sscreen->use_ngg && (sscreen->info.gfx_level >= GFX10 && sscreen->info.gfx_level <= GFX10_3))
         default:
      assert(0);
   FREE(query);
                  }
      static void si_update_occlusion_query_state(struct si_context *sctx, unsigned type, int diff)
   {
      if (type == PIPE_QUERY_OCCLUSION_COUNTER || type == PIPE_QUERY_OCCLUSION_PREDICATE ||
      type == PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE) {
   switch (type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      sctx->num_integer_occlusion_queries += diff;
      case PIPE_QUERY_OCCLUSION_PREDICATE:
      sctx->num_boolean_occlusion_queries += diff;
      case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      sctx->num_conservative_occlusion_queries += diff;
               assert(sctx->num_integer_occlusion_queries >= 0);
   assert(sctx->num_boolean_occlusion_queries >= 0);
            enum si_occlusion_query_mode new_mode =
      sctx->num_integer_occlusion_queries ? SI_OCCLUSION_QUERY_MODE_PRECISE_INTEGER :
   sctx->num_boolean_occlusion_queries ? SI_OCCLUSION_QUERY_MODE_PRECISE_BOOLEAN :
               /* Conservative queries are only available on gfx10+. On gfx11+, they perform worse
   * with late Z, but not early Z. Instead of trying to detect late Z, never enable
   * conservative queries to keep it simple. This is the recommended programming.
   */
   if (new_mode == SI_OCCLUSION_QUERY_MODE_CONSERVATIVE_BOOLEAN &&
                  if (sctx->occlusion_query_mode != new_mode) {
               if (sctx->screen->info.has_out_of_order_rast &&
      (sctx->occlusion_query_mode == SI_OCCLUSION_QUERY_MODE_PRECISE_INTEGER) !=
                        }
      static unsigned event_type_for_stream(unsigned stream)
   {
      switch (stream) {
   default:
   case 0:
         case 1:
         case 2:
         case 3:
            }
      static void emit_sample_streamout(struct radeon_cmdbuf *cs, uint64_t va, unsigned stream)
   {
      radeon_begin(cs);
   radeon_emit(PKT3(PKT3_EVENT_WRITE, 2, 0));
   radeon_emit(EVENT_TYPE(event_type_for_stream(stream)) | EVENT_INDEX(3));
   radeon_emit(va);
   radeon_emit(va >> 32);
      }
      static void si_query_hw_do_emit_start(struct si_context *sctx, struct si_query_hw *query,
         {
               switch (query->b.type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE: {
      radeon_begin(cs);
   if (sctx->gfx_level >= GFX11 &&
      sctx->screen->info.pfp_fw_version >= EVENT_WRITE_ZPASS_PFP_VERSION) {
      } else {
      radeon_emit(PKT3(PKT3_EVENT_WRITE, 2, 0));
   if (sctx->gfx_level >= GFX11)
         else
      }
   radeon_emit(va);
   radeon_emit(va >> 32);
   radeon_end();
      }
   case PIPE_QUERY_PRIMITIVES_EMITTED:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_SO_STATISTICS:
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      emit_sample_streamout(cs, va, query->stream);
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      for (unsigned stream = 0; stream < SI_MAX_STREAMS; ++stream)
            case PIPE_QUERY_TIME_ELAPSED:
      si_cp_release_mem(sctx, cs, V_028A90_BOTTOM_OF_PIPE_TS, 0, EOP_DST_SEL_MEM, EOP_INT_SEL_NONE,
            case PIPE_QUERY_PIPELINE_STATISTICS: {
      if (sctx->screen->use_ngg && query->flags & SI_QUERY_EMULATE_GS_COUNTERS) {
      /* The hw GS primitive counter doesn't work when ngg is active.
   * So if use_ngg is true, we don't use the hw version but instead
   * emulate it in the GS shader.
   * The value is written at the same position, so we don't need to
   * change anything else.
   * If ngg is enabled for the draw, the primitive count is written in
   * gfx10_ngg_gs_emit_epilogue. If ngg is disabled, the number of exported
   * vertices is stored in gs_emitted_vertices and the number of prim
   * is computed based on the output prim type in emit_gs_epilogue.
   */
   struct pipe_shader_buffer sbuf;
   sbuf.buffer = &buffer->b.b;
   sbuf.buffer_offset = query->buffer.results_end;
   sbuf.buffer_size = buffer->bo_size;
                  const uint32_t zero = 0;
   radeon_begin(cs);
   /* Clear the emulated counter end value. We don't clear start because it's unused. */
   va += si_query_pipestat_end_dw_offset(sctx->screen, query->index) * 4;
   radeon_emit(PKT3(PKT3_WRITE_DATA, 2 + 1, 0));
   radeon_emit(S_370_DST_SEL(V_370_MEM) | S_370_WR_CONFIRM(1) | S_370_ENGINE_SEL(V_370_PFP));
   radeon_emit(va);
   radeon_emit(va >> 32);
                     } else {
      radeon_begin(cs);
   radeon_emit(PKT3(PKT3_EVENT_WRITE, 2, 0));
   radeon_emit(EVENT_TYPE(V_028A90_SAMPLE_PIPELINESTAT) | EVENT_INDEX(2));
   radeon_emit(va);
   radeon_emit(va >> 32);
      }
      }
   default:
         }
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, query->buffer.buf,
      }
      static void si_update_hw_pipeline_stats(struct si_context *sctx, unsigned type, int diff)
   {
      if (type == PIPE_QUERY_PIPELINE_STATISTICS ||
      /* All streamout queries: */
   type == PIPE_QUERY_PRIMITIVES_GENERATED ||
   type == PIPE_QUERY_PRIMITIVES_EMITTED ||
   type == PIPE_QUERY_SO_STATISTICS ||
   type == PIPE_QUERY_SO_OVERFLOW_PREDICATE ||
   type == PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE) {
   if (type == PIPE_QUERY_PIPELINE_STATISTICS)
            /* Increment for pipeline statistics and streamout queries. */
            /* Enable/disable pipeline stats if we have any queries. */
   if (diff == 1 && sctx->num_hw_pipestat_streamout_queries == 1) {
      sctx->flags &= ~SI_CONTEXT_STOP_PIPELINE_STATS;
   sctx->flags |= SI_CONTEXT_START_PIPELINE_STATS;
      } else if (diff == -1 && sctx->num_hw_pipestat_streamout_queries == 0) {
      sctx->flags &= ~SI_CONTEXT_START_PIPELINE_STATS;
   sctx->flags |= SI_CONTEXT_STOP_PIPELINE_STATS;
            }
      static void si_query_hw_emit_start(struct si_context *sctx, struct si_query_hw *query)
   {
               if (!query->buffer.buf && query->flags & SI_QUERY_EMULATE_GS_COUNTERS)
            /* Don't realloc pipeline_stats_query_buf */
   if ((!(query->flags & SI_QUERY_EMULATE_GS_COUNTERS) || !sctx->pipeline_stats_query_buf) &&
      !si_query_buffer_alloc(sctx, &query->buffer, query->ops->prepare_buffer, query->result_size))
         if (query->flags & SI_QUERY_EMULATE_GS_COUNTERS)
            si_update_occlusion_query_state(sctx, query->b.type, 1);
   si_update_prims_generated_query_state(sctx, query->b.type, 1);
                     va = query->buffer.buf->gpu_address + query->buffer.results_end;
      }
      static void si_query_hw_do_emit_stop(struct si_context *sctx, struct si_query_hw *query,
         {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
            switch (query->b.type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE: {
      va += 8;
   radeon_begin(cs);
   if (sctx->gfx_level >= GFX11 &&
      sctx->screen->info.pfp_fw_version >= EVENT_WRITE_ZPASS_PFP_VERSION) {
      } else {
      radeon_emit(PKT3(PKT3_EVENT_WRITE, 2, 0));
   if (sctx->gfx_level >= GFX11)
         else
      }
   radeon_emit(va);
   radeon_emit(va >> 32);
            fence_va = va + sctx->screen->info.max_render_backends * 16 - 8;
      }
   case PIPE_QUERY_PRIMITIVES_EMITTED:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_SO_STATISTICS:
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      va += 16;
   emit_sample_streamout(cs, va, query->stream);
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      va += 16;
   for (unsigned stream = 0; stream < SI_MAX_STREAMS; ++stream)
            case PIPE_QUERY_TIME_ELAPSED:
      va += 8;
      case PIPE_QUERY_TIMESTAMP:
      si_cp_release_mem(sctx, cs, V_028A90_BOTTOM_OF_PIPE_TS, 0, EOP_DST_SEL_MEM, EOP_INT_SEL_NONE,
         fence_va = va + 8;
      case PIPE_QUERY_PIPELINE_STATISTICS: {
                        radeon_begin(cs);
   if (sctx->screen->use_ngg && query->flags & SI_QUERY_EMULATE_GS_COUNTERS) {
                     if (--sctx->num_pipeline_stat_emulated_queries == 0) {
      si_set_internal_shader_buffer(sctx, SI_GS_QUERY_BUF, NULL);
         } else {
      radeon_emit(PKT3(PKT3_EVENT_WRITE, 2, 0));
   radeon_emit(EVENT_TYPE(V_028A90_SAMPLE_PIPELINESTAT) | EVENT_INDEX(2));
   radeon_emit(va);
      }
            fence_va = va + sample_size;
      }
   default:
         }
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, query->buffer.buf,
            if (fence_va) {
      si_cp_release_mem(sctx, cs, V_028A90_BOTTOM_OF_PIPE_TS, 0, EOP_DST_SEL_MEM, EOP_INT_SEL_NONE,
               }
      static void si_query_hw_emit_stop(struct si_context *sctx, struct si_query_hw *query)
   {
               /* The queries which need begin already called this in begin_query. */
   if (query->flags & SI_QUERY_HW_FLAG_NO_START) {
      si_need_gfx_cs_space(sctx, 0);
   if (!si_query_buffer_alloc(sctx, &query->buffer, query->ops->prepare_buffer,
                     if (!query->buffer.buf)
            /* emit end query */
                              si_update_occlusion_query_state(sctx, query->b.type, -1);
   si_update_prims_generated_query_state(sctx, query->b.type, -1);
      }
      static void emit_set_predicate(struct si_context *ctx, struct si_resource *buf, uint64_t va,
         {
                        if (ctx->gfx_level >= GFX9) {
      radeon_emit(PKT3(PKT3_SET_PREDICATION, 2, 0));
   radeon_emit(op);
   radeon_emit(va);
      } else {
      radeon_emit(PKT3(PKT3_SET_PREDICATION, 1, 0));
   radeon_emit(va);
      }
               }
      static void si_emit_query_predication(struct si_context *ctx, unsigned index)
   {
      uint32_t op;
            struct si_query_hw *query = (struct si_query_hw *)ctx->render_cond;
   if (!query)
            invert = ctx->render_cond_invert;
   flag_wait = ctx->render_cond_mode == PIPE_RENDER_COND_WAIT ||
            if (ctx->gfx_level >= GFX11 && (query->b.type == PIPE_QUERY_SO_OVERFLOW_PREDICATE ||
            struct gfx11_sh_query *gfx10_query = (struct gfx11_sh_query *)query;
                     /* if true then invert, see GL_ARB_conditional_render_inverted */
   if (!invert)
         else
                     first = gfx10_query->first;
            while (first) {
      qbuf = first;
   if (first != last)
                        unsigned results_base = gfx10_query->first_begin;
                                 unsigned count = (end - begin) / sizeof(struct gfx11_sh_query_buffer_mem);
   do {
      if (gfx10_query->b.type == PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE) {
                        /* set CONTINUE bit for all packets except the first */
         } else {
                                 } else {
               if (query->workaround_buf) {
         } else {
      switch (query->b.type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      op = PRED_OP(PREDICATION_OP_ZPASS);
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
   case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      op = PRED_OP(PREDICATION_OP_PRIMCOUNT);
   invert = !invert;
      default:
      assert(0);
                  /* if true then invert, see GL_ARB_conditional_render_inverted */
   if (invert)
         else
            /* Use the value written by compute shader as a workaround. Note that
   * the wait flag does not apply in this predication mode.
   *
   * The shader outputs the result value to L2. Workarounds only affect GFX8
   * and later, where the CP reads data from L2, so we don't need an
   * additional flush.
   */
   if (query->workaround_buf) {
      uint64_t va = query->workaround_buf->gpu_address + query->workaround_offset;
   emit_set_predicate(ctx, query->workaround_buf, va, op);
                        /* emit predicate packets for all data blocks */
   for (qbuf = &query->buffer; qbuf; qbuf = qbuf->previous) {
                                       if (query->b.type == PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE) {
                        /* set CONTINUE bit for all packets except the first */
         } else {
                                    }
      static struct pipe_query *si_create_query(struct pipe_context *ctx, unsigned query_type,
         {
               if (query_type == PIPE_QUERY_TIMESTAMP_DISJOINT || query_type == PIPE_QUERY_GPU_FINISHED ||
      (query_type >= PIPE_QUERY_DRIVER_SPECIFIC))
         if (sscreen->info.gfx_level >= GFX11 &&
      (query_type == PIPE_QUERY_PRIMITIVES_EMITTED ||
   query_type == PIPE_QUERY_PRIMITIVES_GENERATED || query_type == PIPE_QUERY_SO_STATISTICS ||
   query_type == PIPE_QUERY_SO_OVERFLOW_PREDICATE ||
   query_type == PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE))
            }
      static void si_destroy_query(struct pipe_context *ctx, struct pipe_query *query)
   {
      struct si_context *sctx = (struct si_context *)ctx;
               }
      static bool si_begin_query(struct pipe_context *ctx, struct pipe_query *query)
   {
      struct si_context *sctx = (struct si_context *)ctx;
               }
      static bool si_query_hw_begin(struct si_context *sctx, struct si_query *squery)
   {
               if (query->flags & SI_QUERY_HW_FLAG_NO_START) {
      assert(0);
               if (!(query->flags & SI_QUERY_HW_FLAG_BEGIN_RESUMES))
                     si_query_hw_emit_start(sctx, query);
   if (!query->buffer.buf)
            list_addtail(&query->b.active_list, &sctx->active_queries);
   sctx->num_cs_dw_queries_suspend += query->b.num_cs_dw_suspend;
      }
      static bool si_end_query(struct pipe_context *ctx, struct pipe_query *query)
   {
      struct si_context *sctx = (struct si_context *)ctx;
               }
      static bool si_query_hw_end(struct si_context *sctx, struct si_query *squery)
   {
               if (query->flags & SI_QUERY_HW_FLAG_NO_START)
                     if (!(query->flags & SI_QUERY_HW_FLAG_NO_START)) {
      list_delinit(&query->b.active_list);
               if (!query->buffer.buf)
               }
      static void si_get_hw_query_params(struct si_context *sctx, struct si_query_hw *squery, int index,
         {
               params->pair_stride = 0;
            switch (squery->b.type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      params->start_offset = 0;
   params->end_offset = 8;
   params->fence_offset = max_rbs * 16;
   params->pair_stride = 16;
   params->pair_count = max_rbs;
      case PIPE_QUERY_TIME_ELAPSED:
      params->start_offset = 0;
   params->end_offset = 8;
   params->fence_offset = 16;
      case PIPE_QUERY_TIMESTAMP:
      params->start_offset = 0;
   params->end_offset = 0;
   params->fence_offset = 8;
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      params->start_offset = 8;
   params->end_offset = 24;
   params->fence_offset = params->end_offset + 4;
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      params->start_offset = 0;
   params->end_offset = 16;
   params->fence_offset = params->end_offset + 4;
      case PIPE_QUERY_SO_STATISTICS:
      params->start_offset = 8 - index * 8;
   params->end_offset = 24 - index * 8;
   params->fence_offset = params->end_offset + 4;
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      params->pair_count = SI_MAX_STREAMS;
   params->pair_stride = 32;
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      params->start_offset = 0;
            /* We can re-use the high dword of the last 64-bit value as a
   * fence: it is initialized as 0, and the high bit is set by
   * the write of the streamout stats event.
   */
   params->fence_offset = squery->result_size - 4;
      case PIPE_QUERY_PIPELINE_STATISTICS: {
      params->start_offset = si_query_pipestat_dw_offset(index) * 4;
   params->end_offset = si_query_pipestat_end_dw_offset(sctx->screen, index) * 4;
   params->fence_offset = si_query_pipestats_num_results(sctx->screen) * 16;
      }
   default:
            }
      static unsigned si_query_read_result(void *map, unsigned start_index, unsigned end_index,
         {
      uint32_t *current_result = (uint32_t *)map;
            start = (uint64_t)current_result[start_index] | (uint64_t)current_result[start_index + 1] << 32;
            if (!test_status_bit || ((start & 0x8000000000000000UL) && (end & 0x8000000000000000UL))) {
         }
      }
      static void si_query_hw_add_result(struct si_screen *sscreen, struct si_query_hw *query,
         {
               switch (query->b.type) {
   case PIPE_QUERY_OCCLUSION_COUNTER: {
      for (unsigned i = 0; i < max_rbs; ++i) {
      unsigned results_base = i * 16;
      }
      }
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE: {
      for (unsigned i = 0; i < max_rbs; ++i) {
      unsigned results_base = i * 16;
      }
      }
   case PIPE_QUERY_TIME_ELAPSED:
      result->u64 += si_query_read_result(buffer, 0, 2, false);
      case PIPE_QUERY_TIMESTAMP:
      result->u64 = *(uint64_t *)buffer;
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      /* SAMPLE_STREAMOUTSTATS stores this structure:
   * {
   *    u64 NumPrimitivesWritten;
   *    u64 PrimitiveStorageNeeded;
   * }
   * We only need NumPrimitivesWritten here. */
   result->u64 += si_query_read_result(buffer, 2, 6, true);
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      /* Here we read PrimitiveStorageNeeded. */
   result->u64 += si_query_read_result(buffer, 0, 4, true);
      case PIPE_QUERY_SO_STATISTICS:
      result->so_statistics.num_primitives_written += si_query_read_result(buffer, 2, 6, true);
   result->so_statistics.primitives_storage_needed += si_query_read_result(buffer, 0, 4, true);
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      result->b = result->b || si_query_read_result(buffer, 2, 6, true) !=
            case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      for (unsigned stream = 0; stream < SI_MAX_STREAMS; ++stream) {
      result->b = result->b || si_query_read_result(buffer, 2, 6, true) !=
            }
      case PIPE_QUERY_PIPELINE_STATISTICS:
      for (int i = 0; i < 11; i++) {
      result->pipeline_statistics.counters[i] +=
      si_query_read_result(buffer, si_query_pipestat_dw_offset(i),
   #if 0 /* for testing */
         printf("Pipeline stats: IA verts=%llu, IA prims=%llu, VS=%llu, HS=%llu, "
         "DS=%llu, GS=%llu, GS prims=%llu, Clipper=%llu, "
   "Clipper prims=%llu, PS=%llu, CS=%llu\n",
   result->pipeline_statistics.ia_vertices,
   result->pipeline_statistics.ia_primitives,
   result->pipeline_statistics.vs_invocations,
   result->pipeline_statistics.hs_invocations,
   result->pipeline_statistics.ds_invocations,
   result->pipeline_statistics.gs_invocations,
   result->pipeline_statistics.gs_primitives,
   result->pipeline_statistics.c_invocations,
   result->pipeline_statistics.c_primitives,
   #endif
            default:
            }
      static void si_query_hw_suspend(struct si_context *sctx, struct si_query *query)
   {
         }
      static void si_query_hw_resume(struct si_context *sctx, struct si_query *query)
   {
         }
      static bool si_get_query_result(struct pipe_context *ctx, struct pipe_query *query, bool wait,
         {
      struct si_context *sctx = (struct si_context *)ctx;
               }
      static void si_get_query_result_resource(struct pipe_context *ctx, struct pipe_query *query,
               {
      struct si_context *sctx = (struct si_context *)ctx;
               }
      static void si_query_hw_clear_result(struct si_query_hw *query, union pipe_query_result *result)
   {
         }
      static bool si_query_hw_get_result(struct si_context *sctx, struct si_query *squery, bool wait,
         {
      struct si_screen *sscreen = sctx->screen;
   struct si_query_hw *query = (struct si_query_hw *)squery;
                     for (qbuf = &query->buffer; qbuf; qbuf = qbuf->previous) {
      unsigned usage = PIPE_MAP_READ | (wait ? 0 : PIPE_MAP_DONTBLOCK);
   unsigned results_base = 0;
            if (squery->b.flushed)
         else
            if (!map)
            while (results_base != qbuf->results_end) {
      query->ops->add_result(sscreen, query, map + results_base, result);
                  /* Convert the time to expected units. */
   if (squery->type == PIPE_QUERY_TIME_ELAPSED ||
      squery->type == PIPE_QUERY_TIMESTAMP) {
      }
      }
      static void si_query_hw_get_result_resource(struct si_context *sctx, struct si_query *squery,
                           {
      struct si_query_hw *query = (struct si_query_hw *)squery;
   struct si_query_buffer *qbuf;
   struct si_query_buffer *qbuf_prev;
   struct pipe_resource *tmp_buffer = NULL;
   unsigned tmp_buffer_offset = 0;
   struct si_qbo_state saved_state = {};
   struct pipe_grid_info grid = {};
   struct pipe_constant_buffer constant_buffer = {};
   struct pipe_shader_buffer ssbo[3];
   struct si_hw_query_params params;
   struct {
      uint32_t end_offset;
   uint32_t result_stride;
   uint32_t result_count;
   uint32_t config;
   uint32_t fence_offset;
   uint32_t pair_stride;
               if (!sctx->query_result_shader) {
      sctx->query_result_shader = si_create_query_result_cs(sctx);
   if (!sctx->query_result_shader)
               if (query->buffer.previous) {
      u_suballocator_alloc(&sctx->allocator_zeroed_memory, 16, 16, &tmp_buffer_offset, &tmp_buffer);
   if (!tmp_buffer)
                        si_get_hw_query_params(sctx, query, index >= 0 ? index : 0, &params);
   consts.end_offset = params.end_offset - params.start_offset;
   consts.fence_offset = params.fence_offset - params.start_offset;
   consts.result_stride = query->result_size;
   consts.pair_stride = params.pair_stride;
            constant_buffer.buffer_size = sizeof(consts);
            ssbo[1].buffer = tmp_buffer;
   ssbo[1].buffer_offset = tmp_buffer_offset;
                     grid.block[0] = 1;
   grid.block[1] = 1;
   grid.block[2] = 1;
   grid.grid[0] = 1;
   grid.grid[1] = 1;
            consts.config = 0;
   if (index < 0)
         if (query->b.type == PIPE_QUERY_OCCLUSION_PREDICATE ||
      query->b.type == PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE)
      else if (query->b.type == PIPE_QUERY_SO_OVERFLOW_PREDICATE ||
               else if (query->b.type == PIPE_QUERY_TIMESTAMP || query->b.type == PIPE_QUERY_TIME_ELAPSED)
            switch (result_type) {
   case PIPE_QUERY_TYPE_U64:
   case PIPE_QUERY_TYPE_I64:
      consts.config |= 64;
      case PIPE_QUERY_TYPE_I32:
      consts.config |= 128;
      case PIPE_QUERY_TYPE_U32:
                  sctx->flags |= sctx->screen->barrier_flags.cp_to_L2;
            for (qbuf = &query->buffer; qbuf; qbuf = qbuf_prev) {
      if (query->b.type != PIPE_QUERY_TIMESTAMP) {
      qbuf_prev = qbuf->previous;
   consts.result_count = qbuf->results_end / query->result_size;
   consts.config &= ~3;
   if (qbuf != &query->buffer)
         if (qbuf->previous)
      } else {
      /* Only read the last timestamp. */
   qbuf_prev = NULL;
   consts.result_count = 0;
   consts.config |= 16;
                        ssbo[0].buffer = &qbuf->buf->b.b;
   ssbo[0].buffer_offset = params.start_offset;
            if (!qbuf->previous) {
      ssbo[2].buffer = resource;
   ssbo[2].buffer_offset = offset;
                              if ((flags & PIPE_QUERY_WAIT) && qbuf == &query->buffer) {
               /* Wait for result availability. Wait only for readiness
   * of the last entry, since the fence writes should be
   * serialized in the CP.
   */
                     }
   si_launch_grid_internal_ssbos(sctx, &grid, sctx->query_result_shader,
                     si_restore_qbo_state(sctx, &saved_state);
      }
      static void si_render_condition(struct pipe_context *ctx, struct pipe_query *query, bool condition,
         {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_query_hw *squery = (struct si_query_hw *)query;
            if (query) {
               /* There was a firmware regression in GFX8 which causes successive
   * SET_PREDICATION packets to give the wrong answer for
   * non-inverted stream overflow predication.
   */
   if (((sctx->gfx_level == GFX8 && sctx->screen->info.pfp_fw_feature < 49) ||
      (sctx->gfx_level == GFX9 && sctx->screen->info.pfp_fw_feature < 38)) &&
   !condition &&
   (squery->b.type == PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE ||
   (squery->b.type == PIPE_QUERY_SO_OVERFLOW_PREDICATE &&
                     if (needs_workaround && !squery->workaround_buf) {
                                    /* Reset to NULL to avoid a redundant SET_PREDICATION
   * from launching the compute grid.
                                 /* Settings this in the render cond atom is too late,
   * so set it here. */
                                 sctx->render_cond = query;
   sctx->render_cond_invert = condition;
   sctx->render_cond_mode = mode;
               }
      void si_suspend_queries(struct si_context *sctx)
   {
               LIST_FOR_EACH_ENTRY (query, &sctx->active_queries, active_list)
      }
      void si_resume_queries(struct si_context *sctx)
   {
               /* Check CS space here. Resuming must not be interrupted by flushes. */
            LIST_FOR_EACH_ENTRY (query, &sctx->active_queries, active_list)
      }
      #define XFULL(name_, query_type_, type_, result_type_, group_id_)                                  \
      {                                                                                               \
      .name = name_, .query_type = SI_QUERY_##query_type_, .type = PIPE_DRIVER_QUERY_TYPE_##type_, \
            #define X(name_, query_type_, type_, result_type_)                                                 \
            #define XG(group_, name_, query_type_, type_, result_type_)                                        \
            static struct pipe_driver_query_info si_driver_query_list[] = {
      X("num-compilations", NUM_COMPILATIONS, UINT64, CUMULATIVE),
   X("num-shaders-created", NUM_SHADERS_CREATED, UINT64, CUMULATIVE),
   X("draw-calls", DRAW_CALLS, UINT64, AVERAGE),
   X("decompress-calls", DECOMPRESS_CALLS, UINT64, AVERAGE),
   X("compute-calls", COMPUTE_CALLS, UINT64, AVERAGE),
   X("cp-dma-calls", CP_DMA_CALLS, UINT64, AVERAGE),
   X("num-vs-flushes", NUM_VS_FLUSHES, UINT64, AVERAGE),
   X("num-ps-flushes", NUM_PS_FLUSHES, UINT64, AVERAGE),
   X("num-cs-flushes", NUM_CS_FLUSHES, UINT64, AVERAGE),
   X("num-CB-cache-flushes", NUM_CB_CACHE_FLUSHES, UINT64, AVERAGE),
   X("num-DB-cache-flushes", NUM_DB_CACHE_FLUSHES, UINT64, AVERAGE),
   X("num-L2-invalidates", NUM_L2_INVALIDATES, UINT64, AVERAGE),
   X("num-L2-writebacks", NUM_L2_WRITEBACKS, UINT64, AVERAGE),
   X("num-resident-handles", NUM_RESIDENT_HANDLES, UINT64, AVERAGE),
   X("tc-offloaded-slots", TC_OFFLOADED_SLOTS, UINT64, AVERAGE),
   X("tc-direct-slots", TC_DIRECT_SLOTS, UINT64, AVERAGE),
   X("tc-num-syncs", TC_NUM_SYNCS, UINT64, AVERAGE),
   X("CS-thread-busy", CS_THREAD_BUSY, UINT64, AVERAGE),
   X("gallium-thread-busy", GALLIUM_THREAD_BUSY, UINT64, AVERAGE),
   X("requested-VRAM", REQUESTED_VRAM, BYTES, AVERAGE),
   X("requested-GTT", REQUESTED_GTT, BYTES, AVERAGE),
   X("mapped-VRAM", MAPPED_VRAM, BYTES, AVERAGE),
   X("mapped-GTT", MAPPED_GTT, BYTES, AVERAGE),
   X("slab-wasted-VRAM", SLAB_WASTED_VRAM, BYTES, AVERAGE),
   X("slab-wasted-GTT", SLAB_WASTED_GTT, BYTES, AVERAGE),
   X("buffer-wait-time", BUFFER_WAIT_TIME, MICROSECONDS, CUMULATIVE),
   X("num-mapped-buffers", NUM_MAPPED_BUFFERS, UINT64, AVERAGE),
   X("num-GFX-IBs", NUM_GFX_IBS, UINT64, AVERAGE),
   X("GFX-BO-list-size", GFX_BO_LIST_SIZE, UINT64, AVERAGE),
   X("GFX-IB-size", GFX_IB_SIZE, UINT64, AVERAGE),
   X("num-bytes-moved", NUM_BYTES_MOVED, BYTES, CUMULATIVE),
   X("num-evictions", NUM_EVICTIONS, UINT64, CUMULATIVE),
   X("VRAM-CPU-page-faults", NUM_VRAM_CPU_PAGE_FAULTS, UINT64, CUMULATIVE),
   X("VRAM-usage", VRAM_USAGE, BYTES, AVERAGE),
   X("VRAM-vis-usage", VRAM_VIS_USAGE, BYTES, AVERAGE),
   X("GTT-usage", GTT_USAGE, BYTES, AVERAGE),
   X("back-buffer-ps-draw-ratio", BACK_BUFFER_PS_DRAW_RATIO, UINT64, AVERAGE),
   X("live-shader-cache-hits", LIVE_SHADER_CACHE_HITS, UINT, CUMULATIVE),
   X("live-shader-cache-misses", LIVE_SHADER_CACHE_MISSES, UINT, CUMULATIVE),
   X("memory-shader-cache-hits", MEMORY_SHADER_CACHE_HITS, UINT, CUMULATIVE),
   X("memory-shader-cache-misses", MEMORY_SHADER_CACHE_MISSES, UINT, CUMULATIVE),
   X("disk-shader-cache-hits", DISK_SHADER_CACHE_HITS, UINT, CUMULATIVE),
            /* GPIN queries are for the benefit of old versions of GPUPerfStudio,
   * which use it as a fallback path to detect the GPU type.
   *
   * Note: The names of these queries are significant for GPUPerfStudio
   * (and possibly their order as well). */
   XG(GPIN, "GPIN_000", GPIN_ASIC_ID, UINT, AVERAGE),
   XG(GPIN, "GPIN_001", GPIN_NUM_SIMD, UINT, AVERAGE),
   XG(GPIN, "GPIN_002", GPIN_NUM_RB, UINT, AVERAGE),
   XG(GPIN, "GPIN_003", GPIN_NUM_SPI, UINT, AVERAGE),
            X("temperature", GPU_TEMPERATURE, UINT64, AVERAGE),
   X("shader-clock", CURRENT_GPU_SCLK, HZ, AVERAGE),
            /* The following queries must be at the end of the list because their
   * availability is adjusted dynamically based on the DRM version. */
   X("GPU-load", GPU_LOAD, UINT64, AVERAGE),
   X("GPU-shaders-busy", GPU_SHADERS_BUSY, UINT64, AVERAGE),
   X("GPU-ta-busy", GPU_TA_BUSY, UINT64, AVERAGE),
   X("GPU-gds-busy", GPU_GDS_BUSY, UINT64, AVERAGE),
   X("GPU-vgt-busy", GPU_VGT_BUSY, UINT64, AVERAGE),
   X("GPU-ia-busy", GPU_IA_BUSY, UINT64, AVERAGE),
   X("GPU-sx-busy", GPU_SX_BUSY, UINT64, AVERAGE),
   X("GPU-wd-busy", GPU_WD_BUSY, UINT64, AVERAGE),
   X("GPU-bci-busy", GPU_BCI_BUSY, UINT64, AVERAGE),
   X("GPU-sc-busy", GPU_SC_BUSY, UINT64, AVERAGE),
   X("GPU-pa-busy", GPU_PA_BUSY, UINT64, AVERAGE),
   X("GPU-db-busy", GPU_DB_BUSY, UINT64, AVERAGE),
   X("GPU-cp-busy", GPU_CP_BUSY, UINT64, AVERAGE),
            /* SRBM_STATUS2 */
            /* CP_STAT */
   X("GPU-pfp-busy", GPU_PFP_BUSY, UINT64, AVERAGE),
   X("GPU-meq-busy", GPU_MEQ_BUSY, UINT64, AVERAGE),
   X("GPU-me-busy", GPU_ME_BUSY, UINT64, AVERAGE),
   X("GPU-surf-sync-busy", GPU_SURF_SYNC_BUSY, UINT64, AVERAGE),
   X("GPU-cp-dma-busy", GPU_CP_DMA_BUSY, UINT64, AVERAGE),
      };
      #undef X
   #undef XG
   #undef XFULL
      static unsigned si_get_num_queries(struct si_screen *sscreen)
   {
      /* amdgpu */
   if (sscreen->info.is_amdgpu) {
      if (sscreen->info.gfx_level >= GFX8)
         else
               /* radeon */
   if (sscreen->info.gfx_level == GFX7)
         else
               }
      static int si_get_driver_query_info(struct pipe_screen *screen, unsigned index,
         {
      struct si_screen *sscreen = (struct si_screen *)screen;
            if (!info) {
                           if (index >= num_queries)
                     switch (info->query_type) {
   case SI_QUERY_REQUESTED_VRAM:
   case SI_QUERY_VRAM_USAGE:
   case SI_QUERY_MAPPED_VRAM:
   case SI_QUERY_SLAB_WASTED_VRAM:
      info->max_value.u64 = (uint64_t)sscreen->info.vram_size_kb * 1024;
      case SI_QUERY_REQUESTED_GTT:
   case SI_QUERY_GTT_USAGE:
   case SI_QUERY_MAPPED_GTT:
   case SI_QUERY_SLAB_WASTED_GTT:
      info->max_value.u64 = (uint64_t)sscreen->info.gart_size_kb * 1024;
      case SI_QUERY_GPU_TEMPERATURE:
      info->max_value.u64 = 125;
      case SI_QUERY_VRAM_VIS_USAGE:
      info->max_value.u64 = (uint64_t)sscreen->info.vram_vis_size_kb * 1024;
               if (info->group_id != ~(unsigned)0 && sscreen->perfcounters)
               }
      /* Note: Unfortunately, GPUPerfStudio hardcodes the order of hardware
   * performance counter groups, so be careful when changing this and related
   * functions.
   */
   static int si_get_driver_query_group_info(struct pipe_screen *screen, unsigned index,
         {
      struct si_screen *sscreen = (struct si_screen *)screen;
            if (sscreen->perfcounters)
            if (!info)
            if (index < num_pc_groups)
            index -= num_pc_groups;
   if (index >= SI_NUM_SW_QUERY_GROUPS)
            info->name = "GPIN";
   info->max_active_queries = 5;
   info->num_queries = 5;
      }
      static const struct si_query_ops query_hw_ops = {
      .destroy = si_query_hw_destroy,
   .begin = si_query_hw_begin,
   .end = si_query_hw_end,
   .get_result = si_query_hw_get_result,
            .suspend = si_query_hw_suspend,
      };
      static const struct si_query_ops sw_query_ops = {
      .destroy = si_query_sw_destroy,
   .begin = si_query_sw_begin,
   .end = si_query_sw_end,
   .get_result = si_query_sw_get_result,
      };
      static const struct si_query_hw_ops query_hw_default_hw_ops = {
      .prepare_buffer = si_query_hw_prepare_buffer,
   .emit_start = si_query_hw_do_emit_start,
   .emit_stop = si_query_hw_do_emit_stop,
   .clear_result = si_query_hw_clear_result,
      };
      void si_init_query_functions(struct si_context *sctx)
   {
      sctx->b.create_query = si_create_query;
   sctx->b.create_batch_query = si_create_batch_query;
   sctx->b.destroy_query = si_destroy_query;
   sctx->b.begin_query = si_begin_query;
   sctx->b.end_query = si_end_query;
   sctx->b.get_query_result = si_get_query_result;
            if (sctx->has_graphics) {
      sctx->atoms.s.render_cond.emit = si_emit_query_predication;
                  }
      void si_init_screen_query_functions(struct si_screen *sscreen)
   {
      sscreen->b.get_driver_query_info = si_get_driver_query_info;
      }
