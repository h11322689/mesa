   /*
   * Copyright © 2017 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * @file iris_query.c
   *
   * ============================= GENXML CODE =============================
   *              [This file is compiled once per generation.]
   * =======================================================================
   *
   * Query object support.  This allows measuring various simple statistics
   * via counters on the GPU.  We use GenX code for MI_MATH calculations.
   */
      #include <stdio.h>
   #include <errno.h>
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/u_inlines.h"
   #include "util/u_upload_mgr.h"
   #include "iris_context.h"
   #include "iris_defines.h"
   #include "iris_fence.h"
   #include "iris_monitor.h"
   #include "iris_resource.h"
   #include "iris_screen.h"
      #include "iris_genx_macros.h"
      #define SO_PRIM_STORAGE_NEEDED(n) (GENX(SO_PRIM_STORAGE_NEEDED0_num) + (n) * 8)
   #define SO_NUM_PRIMS_WRITTEN(n)   (GENX(SO_NUM_PRIMS_WRITTEN0_num) + (n) * 8)
      struct iris_query {
               enum pipe_query_type type;
                                       struct iris_state_ref query_state_ref;
   struct iris_query_snapshots *map;
                              /* Fence for PIPE_QUERY_GPU_FINISHED. */
      };
      struct iris_query_snapshots {
      /** iris_render_condition's saved MI_PREDICATE_RESULT value. */
            /** Have the start/end snapshots landed? */
            /** Starting and ending counter snapshots */
   uint64_t start;
      };
      struct iris_query_so_overflow {
      uint64_t predicate_result;
            struct {
      uint64_t prim_storage_needed[2];
         };
      static struct mi_value
   query_mem64(struct iris_query *q, uint32_t offset)
   {
      struct iris_address addr = {
      .bo = iris_resource_bo(q->query_state_ref.res),
   .offset = q->query_state_ref.offset + offset,
      };
      }
      /**
   * Is this type of query written by PIPE_CONTROL?
   */
   static bool
   iris_is_query_pipelined(struct iris_query *q)
   {
      switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_TIME_ELAPSED:
            default:
            }
      static void
   mark_available(struct iris_context *ice, struct iris_query *q)
   {
      struct iris_batch *batch = &ice->batches[q->batch_idx];
   unsigned flags = PIPE_CONTROL_WRITE_IMMEDIATE;
   unsigned offset = offsetof(struct iris_query_snapshots, snapshots_landed);
   struct iris_bo *bo = iris_resource_bo(q->query_state_ref.res);
            if (!iris_is_query_pipelined(q)) {
         } else {
      /* Order available *after* the query results. */
   flags |= PIPE_CONTROL_FLUSH_ENABLE;
   iris_emit_pipe_control_write(batch, "query: mark available",
         }
      /**
   * Write PS_DEPTH_COUNT to q->(dest) via a PIPE_CONTROL.
   */
   static void
   iris_pipelined_write(struct iris_batch *batch,
                     {
      const struct intel_device_info *devinfo = batch->screen->devinfo;
   const unsigned optional_cs_stall =
                  iris_emit_pipe_control_write(batch, "query: pipelined snapshot write",
            }
      static void
   write_value(struct iris_context *ice, struct iris_query *q, unsigned offset)
   {
      struct iris_batch *batch = &ice->batches[q->batch_idx];
            if (!iris_is_query_pipelined(q)) {
      enum pipe_control_flags flags = PIPE_CONTROL_CS_STALL |
         if (batch->name == IRIS_BATCH_COMPUTE) {
      iris_emit_pipe_control_write(batch,
                                             iris_emit_pipe_control_flush(batch,
                           switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      if (GFX_VER >= 10) {
      /* "Driver must program PIPE_CONTROL with only Depth Stall Enable
   *  bit set prior to programming a PIPE_CONTROL with Write PS Depth
   *  Count sync operation."
   */
   iris_emit_pipe_control_flush(batch,
                  }
   iris_pipelined_write(&ice->batches[IRIS_BATCH_RENDER], q,
                        case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
      iris_pipelined_write(&ice->batches[IRIS_BATCH_RENDER], q,
                  case PIPE_QUERY_PRIMITIVES_GENERATED:
      batch->screen->vtbl.store_register_mem64(batch,
                              case PIPE_QUERY_PRIMITIVES_EMITTED:
      batch->screen->vtbl.store_register_mem64(batch,
                  case PIPE_QUERY_PIPELINE_STATISTICS_SINGLE: {
      static const uint32_t index_to_reg[] = {
      GENX(IA_VERTICES_COUNT_num),
   GENX(IA_PRIMITIVES_COUNT_num),
   GENX(VS_INVOCATION_COUNT_num),
   GENX(GS_INVOCATION_COUNT_num),
   GENX(GS_PRIMITIVES_COUNT_num),
   GENX(CL_INVOCATION_COUNT_num),
   GENX(CL_PRIMITIVES_COUNT_num),
   GENX(PS_INVOCATION_COUNT_num),
   GENX(HS_INVOCATION_COUNT_num),
   GENX(DS_INVOCATION_COUNT_num),
      };
            batch->screen->vtbl.store_register_mem64(batch, reg, bo, offset, false);
      }
   default:
            }
      static void
   write_overflow_values(struct iris_context *ice, struct iris_query *q, bool end)
   {
      struct iris_batch *batch = &ice->batches[IRIS_BATCH_RENDER];
   uint32_t count = q->type == PIPE_QUERY_SO_OVERFLOW_PREDICATE ? 1 : 4;
   struct iris_bo *bo = iris_resource_bo(q->query_state_ref.res);
            iris_emit_pipe_control_flush(batch,
                     for (uint32_t i = 0; i < count; i++) {
      int s = q->index + i;
   int g_idx = offset + offsetof(struct iris_query_so_overflow,
         int w_idx = offset + offsetof(struct iris_query_so_overflow,
         batch->screen->vtbl.store_register_mem64(batch, SO_NUM_PRIMS_WRITTEN(s),
         batch->screen->vtbl.store_register_mem64(batch, SO_PRIM_STORAGE_NEEDED(s),
         }
      static uint64_t
   iris_raw_timestamp_delta(uint64_t time0, uint64_t time1)
   {
      if (time0 > time1) {
         } else {
            }
      static bool
   stream_overflowed(struct iris_query_so_overflow *so, int s)
   {
      return (so->stream[s].prim_storage_needed[1] -
            }
      static void
   calculate_result_on_cpu(const struct intel_device_info *devinfo,
         {
      switch (q->type) {
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      q->result = q->map->end != q->map->start;
      case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
      /* The timestamp is the single starting snapshot. */
   q->result = intel_device_info_timebase_scale(devinfo, q->map->start);
   q->result &= (1ull << TIMESTAMP_BITS) - 1;
      case PIPE_QUERY_TIME_ELAPSED:
      q->result = iris_raw_timestamp_delta(q->map->start, q->map->end);
   q->result = intel_device_info_timebase_scale(devinfo, q->result);
   q->result &= (1ull << TIMESTAMP_BITS) - 1;
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      q->result = stream_overflowed((void *) q->map, q->index);
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      q->result = false;
   for (int i = 0; i < PIPE_MAX_VERTEX_STREAMS; i++)
            case PIPE_QUERY_PIPELINE_STATISTICS_SINGLE:
               /* WaDividePSInvocationCountBy4:HSW,BDW */
   if (GFX_VER == 8 && q->index == PIPE_STAT_QUERY_PS_INVOCATIONS)
            case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
   default:
      q->result = q->map->end - q->map->start;
                  }
      /**
   * Calculate the streamout overflow for stream \p idx:
   *
   * (num_prims[1] - num_prims[0]) - (storage_needed[1] - storage_needed[0])
   */
   static struct mi_value
   calc_overflow_for_stream(struct mi_builder *b,
               {
   #define C(counter, i) query_mem64(q, \
               return mi_isub(b, mi_isub(b, C(num_prims, 1), C(num_prims, 0)),
            #undef C
   }
      /**
   * Calculate whether any stream has overflowed.
   */
   static struct mi_value
   calc_overflow_any_stream(struct mi_builder *b, struct iris_query *q)
   {
      struct mi_value stream_result[PIPE_MAX_VERTEX_STREAMS];
   for (int i = 0; i < PIPE_MAX_VERTEX_STREAMS; i++)
            struct mi_value result = stream_result[0];
   for (int i = 1; i < PIPE_MAX_VERTEX_STREAMS; i++)
               }
      static bool
   query_is_boolean(enum pipe_query_type type)
   {
      switch (type) {
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
   case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
         default:
            }
      /**
   * Calculate the result using MI_MATH.
   */
   static struct mi_value
   calculate_result_on_gpu(const struct intel_device_info *devinfo,
               {
      struct mi_value result;
   struct mi_value start_val =
         struct mi_value end_val =
            switch (q->type) {
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      result = calc_overflow_for_stream(b, q, q->index);
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      result = calc_overflow_any_stream(b, q);
      case PIPE_QUERY_TIMESTAMP: {
      /* TODO: This discards any fractional bits of the timebase scale.
   * We would need to do a bit of fixed point math on the CS ALU, or
   * launch an actual shader to calculate this with full precision.
   */
   uint32_t scale = 1000000000ull / devinfo->timestamp_frequency;
   result = mi_iand(b, mi_imm((1ull << 36) - 1),
            }
   case PIPE_QUERY_TIME_ELAPSED: {
      /* TODO: This discards fractional bits (see above). */
   uint32_t scale = 1000000000ull / devinfo->timestamp_frequency;
   result = mi_imul_imm(b, mi_isub(b, end_val, start_val), scale);
      }
   default:
      result = mi_isub(b, end_val, start_val);
               /* WaDividePSInvocationCountBy4:HSW,BDW */
   if (GFX_VER == 8 &&
      q->type == PIPE_QUERY_PIPELINE_STATISTICS_SINGLE &&
   q->index == PIPE_STAT_QUERY_PS_INVOCATIONS)
         if (query_is_boolean(q->type))
               }
      static struct pipe_query *
   iris_create_query(struct pipe_context *ctx,
               {
               q->type = query_type;
   q->index = index;
            if (q->type == PIPE_QUERY_PIPELINE_STATISTICS_SINGLE &&
      q->index == PIPE_STAT_QUERY_CS_INVOCATIONS)
      else
            }
      static struct pipe_query *
   iris_create_batch_query(struct pipe_context *ctx,
               {
      struct iris_context *ice = (void *) ctx;
   struct iris_query *q = calloc(1, sizeof(struct iris_query));
   if (unlikely(!q))
         q->type = PIPE_QUERY_DRIVER_SPECIFIC;
   q->index = -1;
   q->monitor = iris_create_monitor_object(ice, num_queries, query_types);
   if (unlikely(!q->monitor)) {
      free(q);
                  }
      static void
   iris_destroy_query(struct pipe_context *ctx, struct pipe_query *p_query)
   {
      struct iris_query *query = (void *) p_query;
   struct iris_screen *screen = (void *) ctx->screen;
   if (query->monitor) {
      iris_destroy_monitor_object(ctx, query->monitor);
      } else {
      iris_syncobj_reference(screen->bufmgr, &query->syncobj, NULL);
      }
   pipe_resource_reference(&query->query_state_ref.res, NULL);
      }
         static bool
   iris_begin_query(struct pipe_context *ctx, struct pipe_query *query)
   {
      struct iris_context *ice = (void *) ctx;
            if (q->monitor)
            void *ptr = NULL;
            if (q->type == PIPE_QUERY_SO_OVERFLOW_PREDICATE ||
      q->type == PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE)
      else
            u_upload_alloc(ice->query_buffer_uploader, 0,
                        if (!iris_resource_bo(q->query_state_ref.res))
            q->map = ptr;
   if (!q->map)
            q->result = 0ull;
   q->ready = false;
            if (q->type == PIPE_QUERY_PRIMITIVES_GENERATED && q->index == 0) {
      ice->state.prims_generated_query_active = true;
               if (q->type == PIPE_QUERY_OCCLUSION_COUNTER && q->index == 0) {
      ice->state.occlusion_query_active = true;
               if (q->type == PIPE_QUERY_SO_OVERFLOW_PREDICATE ||
      q->type == PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE)
      else
      write_value(ice, q,
                  }
      static bool
   iris_end_query(struct pipe_context *ctx, struct pipe_query *query)
   {
      struct iris_context *ice = (void *) ctx;
            if (q->monitor)
            if (q->type == PIPE_QUERY_GPU_FINISHED) {
      ctx->flush(ctx, &q->fence, PIPE_FLUSH_DEFERRED);
                        if (q->type == PIPE_QUERY_TIMESTAMP) {
      iris_begin_query(ctx, query);
   iris_batch_reference_signal_syncobj(batch, &q->syncobj);
   mark_available(ice, q);
               if (q->type == PIPE_QUERY_PRIMITIVES_GENERATED && q->index == 0) {
      ice->state.prims_generated_query_active = false;
               if (q->type == PIPE_QUERY_OCCLUSION_COUNTER && q->index == 0) {
      ice->state.occlusion_query_active = false;
               if (q->type == PIPE_QUERY_SO_OVERFLOW_PREDICATE ||
      q->type == PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE)
      else
      write_value(ice, q,
               iris_batch_reference_signal_syncobj(batch, &q->syncobj);
               }
      /**
   * See if the snapshots have landed for a query, and if so, compute the
   * result and mark it ready.  Does not flush (unlike iris_get_query_result).
   */
   static void
   iris_check_query_no_flush(struct iris_context *ice, struct iris_query *q)
   {
      struct iris_screen *screen = (void *) ice->ctx.screen;
            if (!q->ready && READ_ONCE(q->map->snapshots_landed)) {
            }
      static bool
   iris_get_query_result(struct pipe_context *ctx,
                     {
      struct iris_context *ice = (void *) ctx;
            if (q->monitor)
            struct iris_screen *screen = (void *) ctx->screen;
            if (unlikely(screen->devinfo->no_hw)) {
      result->u64 = 0;
               if (q->type == PIPE_QUERY_GPU_FINISHED) {
               result->b = screen->fence_finish(screen, ctx, q->fence,
                     if (!q->ready) {
      struct iris_batch *batch = &ice->batches[q->batch_idx];
   if (q->syncobj == iris_batch_get_signal_syncobj(batch))
            while (!READ_ONCE(q->map->snapshots_landed)) {
      if (wait)
         else
               assert(READ_ONCE(q->map->snapshots_landed));
                                    }
      static void
   iris_get_query_result_resource(struct pipe_context *ctx,
                                 struct pipe_query *query,
   {
      struct iris_context *ice = (void *) ctx;
   struct iris_query *q = (void *) query;
   struct iris_batch *batch = &ice->batches[q->batch_idx];
   const struct intel_device_info *devinfo = batch->screen->devinfo;
   struct iris_resource *res = (void *) p_res;
   struct iris_bo *query_bo = iris_resource_bo(q->query_state_ref.res);
   struct iris_bo *dst_bo = iris_resource_bo(p_res);
   unsigned snapshots_landed_offset =
                     if (index == -1) {
      /* They're asking for the availability of the result.  If we still
   * have commands queued up which produce the result, submit them
   * now so that progress happens.  Either way, copy the snapshots
   * landed field to the destination resource.
   */
   if (q->syncobj == iris_batch_get_signal_syncobj(batch))
            batch->screen->vtbl.copy_mem_mem(batch, dst_bo, offset,
                           if (!q->ready && READ_ONCE(q->map->snapshots_landed)) {
      /* The final snapshots happen to have landed, so let's just compute
   * the result on the CPU now...
   */
               if (q->ready) {
      /* We happen to have the result on the CPU, so just copy it. */
   if (result_type <= PIPE_QUERY_TYPE_U32) {
         } else {
                  /* Make sure QBO is flushed before its result is used elsewhere. */
   iris_dirty_for_history(ice, res);
                        struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
   const uint32_t mocs = iris_mocs(query_bo, &batch->screen->isl_dev, 0);
                     struct mi_value result = calculate_result_on_gpu(devinfo, &b, q);
   struct mi_value dst =
      result_type <= PIPE_QUERY_TYPE_U32 ?
   mi_mem32(rw_bo(dst_bo, offset, IRIS_DOMAIN_OTHER_WRITE)) :
         if (predicated) {
      mi_store(&b, mi_reg32(MI_PREDICATE_RESULT),
            } else {
                     }
      static void
   iris_set_active_query_state(struct pipe_context *ctx, bool enable)
   {
               if (ice->state.statistics_counters_enabled == enable)
            // XXX: most packets aren't paying attention to this yet, because it'd
   // have to be done dynamically at draw time, which is a pain
   ice->state.statistics_counters_enabled = enable;
   ice->state.dirty |= IRIS_DIRTY_CLIP |
                     ice->state.stage_dirty |= IRIS_STAGE_DIRTY_GS |
                  }
      static void
   set_predicate_enable(struct iris_context *ice, bool value)
   {
      if (value)
         else
      }
      static void
   set_predicate_for_result(struct iris_context *ice,
               {
      struct iris_batch *batch = &ice->batches[IRIS_BATCH_RENDER];
                     /* The CPU doesn't have the query result yet; use hardware predication */
            /* Ensure the memory is coherent for MI_LOAD_REGISTER_* commands. */
   iris_emit_pipe_control_flush(batch,
                        struct mi_builder b;
   mi_builder_init(&b, batch->screen->devinfo, batch);
   const uint32_t mocs = iris_mocs(bo, &batch->screen->isl_dev, 0);
                     switch (q->type) {
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      result = calc_overflow_for_stream(&b, q, q->index);
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      result = calc_overflow_any_stream(&b, q);
      default: {
      /* PIPE_QUERY_OCCLUSION_* */
   struct mi_value start =
         struct mi_value end =
         result = mi_isub(&b, end, start);
      }
            result = inverted ? mi_z(&b, result) : mi_nz(&b, result);
            /* We immediately set the predicate on the render batch, as all the
   * counters come from 3D operations.  However, we may need to predicate
   * a compute dispatch, which executes in a different GEM context and has
   * a different MI_PREDICATE_RESULT register.  So, we save the result to
   * memory and reload it in iris_launch_grid.
   */
   mi_value_ref(&b, result);
   mi_store(&b, mi_reg32(MI_PREDICATE_RESULT), result);
   mi_store(&b, query_mem64(q, offsetof(struct iris_query_snapshots,
                     }
      static void
   iris_render_condition(struct pipe_context *ctx,
                     {
      struct iris_context *ice = (void *) ctx;
            /* The old condition isn't relevant; we'll update it if necessary */
            if (!q) {
      ice->state.predicate = IRIS_PREDICATE_STATE_RENDER;
                        if (q->result || q->ready) {
         } else {
      if (mode == PIPE_RENDER_COND_NO_WAIT ||
      mode == PIPE_RENDER_COND_BY_REGION_NO_WAIT) {
   perf_debug(&ice->dbg, "Conditional rendering demoted from "
      }
         }
      void
   genX(init_query)(struct iris_context *ice)
   {
               ctx->create_query = iris_create_query;
   ctx->create_batch_query = iris_create_batch_query;
   ctx->destroy_query = iris_destroy_query;
   ctx->begin_query = iris_begin_query;
   ctx->end_query = iris_end_query;
   ctx->get_query_result = iris_get_query_result;
   ctx->get_query_result_resource = iris_get_query_result_resource;
   ctx->set_active_query_state = iris_set_active_query_state;
      }
