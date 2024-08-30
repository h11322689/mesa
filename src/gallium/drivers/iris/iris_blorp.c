   /*
   * Copyright © 2018 Intel Corporation
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
   * @file iris_blorp.c
   *
   * ============================= GENXML CODE =============================
   *              [This file is compiled once per generation.]
   * =======================================================================
   *
   * GenX specific code for working with BLORP (blitting, resolves, clears
   * on the 3D engine).  This provides the driver-specific hooks needed to
   * implement the BLORP API.
   *
   * See iris_blit.c, iris_clear.c, and so on.
   */
      #include <assert.h>
      #include "iris_batch.h"
   #include "iris_resource.h"
   #include "iris_context.h"
      #include "util/u_upload_mgr.h"
   #include "intel/common/intel_l3_config.h"
      #include "blorp/blorp_genX_exec.h"
      static uint32_t *
   stream_state(struct iris_batch *batch,
               struct u_upload_mgr *uploader,
   unsigned size,
   unsigned alignment,
   {
      struct pipe_resource *res = NULL;
                     struct iris_bo *bo = iris_resource_bo(res);
            iris_record_state_size(batch->state_sizes,
            /* If the caller has asked for a BO, we leave them the responsibility of
   * adding bo->address (say, by handing an address to genxml).  If not,
   * we assume they want the offset from a base address.
   */
   if (out_bo)
         else
                        }
      static void *
   blorp_emit_dwords(struct blorp_batch *blorp_batch, unsigned n)
   {
      struct iris_batch *batch = blorp_batch->driver_batch;
      }
      static uint64_t
   combine_and_pin_address(struct blorp_batch *blorp_batch,
         {
      struct iris_batch *batch = blorp_batch->driver_batch;
            iris_use_pinned_bo(batch, bo,
                  /* Assume this is a general address, not relative to a base. */
      }
      static uint64_t
   blorp_emit_reloc(struct blorp_batch *blorp_batch, UNUSED void *location,
         {
         }
      static void
   blorp_surface_reloc(struct blorp_batch *blorp_batch, uint32_t ss_offset,
         {
         }
      static uint64_t
   blorp_get_surface_address(struct blorp_batch *blorp_batch,
         {
         }
      UNUSED static struct blorp_address
   blorp_get_surface_base_address(UNUSED struct blorp_batch *blorp_batch)
   {
         }
      static void *
   blorp_alloc_dynamic_state(struct blorp_batch *blorp_batch,
                     {
      struct iris_context *ice = blorp_batch->blorp->driver_ctx;
            return stream_state(batch, ice->state.dynamic_uploader,
      }
      UNUSED static void *
   blorp_alloc_general_state(struct blorp_batch *blorp_batch,
                     {
      /* Use dynamic state range for general state on iris. */
      }
      static void
   blorp_alloc_binding_table(struct blorp_batch *blorp_batch,
                           unsigned num_entries,
   unsigned state_size,
   {
      struct iris_context *ice = blorp_batch->blorp->driver_ctx;
   struct iris_binder *binder = &ice->state.binder;
            unsigned bt_offset =
                                    for (unsigned i = 0; i < num_entries; i++) {
      surface_maps[i] = stream_state(batch, ice->state.surface_uploader,
                                       }
      static uint32_t
   blorp_binding_table_offset_to_pointer(struct blorp_batch *batch,
         {
      /* See IRIS_BT_OFFSET_SHIFT in iris_state.c */
      }
      static void *
   blorp_alloc_vertex_buffer(struct blorp_batch *blorp_batch,
               {
      struct iris_context *ice = blorp_batch->blorp->driver_ctx;
   struct iris_batch *batch = blorp_batch->driver_batch;
   struct iris_bo *bo;
            void *map = stream_state(batch, ice->ctx.const_uploader, size, 64,
            *addr = (struct blorp_address) {
      .buffer = bo,
   .offset = offset,
   .mocs = iris_mocs(bo, &batch->screen->isl_dev,
                        }
      /**
   * See iris_upload_render_state's IRIS_DIRTY_VERTEX_BUFFERS handling for
   * a comment about why these VF invalidations are needed.
   */
   static void
   blorp_vf_invalidate_for_vb_48b_transitions(struct blorp_batch *blorp_batch,
                     {
   #if GFX_VER < 11
      struct iris_context *ice = blorp_batch->blorp->driver_ctx;
   struct iris_batch *batch = blorp_batch->driver_batch;
            for (unsigned i = 0; i < num_vbs; i++) {
      struct iris_bo *bo = addrs[i].buffer;
            if (high_bits != ice->state.last_vbo_high_bits[i]) {
      need_invalidate = true;
                  if (need_invalidate) {
      iris_emit_pipe_control_flush(batch,
                     #endif
   }
      static struct blorp_address
   blorp_get_workaround_address(struct blorp_batch *blorp_batch)
   {
               return (struct blorp_address) {
      .buffer = batch->screen->workaround_address.bo,
   .offset = batch->screen->workaround_address.offset,
   .local_hint =
         }
      static void
   blorp_flush_range(UNUSED struct blorp_batch *blorp_batch,
               {
      /* All allocated states come from the batch which we will flush before we
   * submit it.  There's nothing for us to do here.
      }
      static const struct intel_l3_config *
   blorp_get_l3_config(struct blorp_batch *blorp_batch)
   {
      struct iris_batch *batch = blorp_batch->driver_batch;
      }
      static void
   iris_blorp_exec_render(struct blorp_batch *blorp_batch,
         {
      struct iris_context *ice = blorp_batch->blorp->driver_ctx;
         #if GFX_VER >= 11
      /* The PIPE_CONTROL command description says:
   *
   *    "Whenever a Binding Table Index (BTI) used by a Render Target Message
   *     points to a different RENDER_SURFACE_STATE, SW must issue a Render
   *     Target Cache Flush by enabling this bit. When render target flush
   *     is set due to new association of BTI, PS Scoreboard Stall bit must
   *     be set in this packet."
   */
   iris_emit_pipe_control_flush(batch,
                  #endif
         /* Check if blorp ds state matches ours. */
   if (intel_needs_workaround(batch->screen->devinfo, 18019816803)) {
      const bool blorp_ds_state =
         if (ice->state.ds_write_state != blorp_ds_state) {
      blorp_batch->flags |= BLORP_BATCH_NEED_PSS_STALL_SYNC;
                  if (params->depth.enabled &&
      !(blorp_batch->flags & BLORP_BATCH_NO_EMIT_DEPTH_STENCIL))
               #if GFX_VER == 8
         #endif
         const unsigned scale = params->fast_clear_op ? UINT_MAX : 1;
   if (ice->state.current_hash_scale != scale) {
      genX(emit_hashing_mode)(ice, batch, params->x1 - params->x0,
            #if GFX_VERx10 == 125
      iris_use_pinned_bo(batch, iris_resource_bo(ice->state.pixel_hashing_tables),
      #else
         #endif
      #if GFX_VER >= 12
         #endif
                                    /* We've smashed all state compared to what the normal 3D pipeline
   * rendering tracks for GL.
            uint64_t skip_bits = (IRIS_DIRTY_POLYGON_STIPPLE |
                        IRIS_DIRTY_SO_BUFFERS |
   IRIS_DIRTY_SO_DECL_LIST |
      /* Wa_14016820455
   * On Gfx 12.5 platforms, the SF_CL_VIEWPORT pointer can be invalidated
   * likely by a read cache invalidation when clipping is disabled, so we
   * don't skip its dirty bit here, in order to reprogram it.
   */
   if (GFX_VERx10 != 125)
            uint64_t skip_stage_bits = (IRIS_ALL_STAGE_DIRTY_FOR_COMPUTE |
                              IRIS_STAGE_DIRTY_UNCOMPILED_VS |
   IRIS_STAGE_DIRTY_UNCOMPILED_TCS |
   IRIS_STAGE_DIRTY_UNCOMPILED_TES |
   IRIS_STAGE_DIRTY_UNCOMPILED_GS |
         if (!ice->shaders.prog[MESA_SHADER_TESS_EVAL]) {
      /* BLORP disabled tessellation, but it was already off anyway */
   skip_stage_bits |= IRIS_STAGE_DIRTY_TCS |
                     IRIS_STAGE_DIRTY_TES |
               if (!ice->shaders.prog[MESA_SHADER_GEOMETRY]) {
      /* BLORP disabled geometry shaders, but it was already off anyway */
   skip_stage_bits |= IRIS_STAGE_DIRTY_GS |
                     /* we can skip flagging IRIS_DIRTY_DEPTH_BUFFER, if
   * BLORP_BATCH_NO_EMIT_DEPTH_STENCIL is set.
   */
   if (blorp_batch->flags & BLORP_BATCH_NO_EMIT_DEPTH_STENCIL)
            if (!params->wm_prog_data)
            ice->state.dirty |= ~skip_bits;
            for (int i = 0; i < ARRAY_SIZE(ice->shaders.urb.size); i++)
            if (params->src.enabled)
      iris_bo_bump_seqno(params->src.addr.buffer, batch->next_seqno,
      if (params->dst.enabled)
      iris_bo_bump_seqno(params->dst.addr.buffer, batch->next_seqno,
      if (params->depth.enabled)
      iris_bo_bump_seqno(params->depth.addr.buffer, batch->next_seqno,
      if (params->stencil.enabled)
      iris_bo_bump_seqno(params->stencil.addr.buffer, batch->next_seqno,
   }
      static void
   iris_blorp_exec_blitter(struct blorp_batch *blorp_batch,
         {
               /* Around the length of a XY_BLOCK_COPY_BLT and MI_FLUSH_DW */
                                       if (params->src.enabled) {
      iris_bo_bump_seqno(params->src.addr.buffer, batch->next_seqno,
               iris_bo_bump_seqno(params->dst.addr.buffer, batch->next_seqno,
      }
      static void
   iris_blorp_exec(struct blorp_batch *blorp_batch,
         {
      if (blorp_batch->flags & BLORP_BATCH_USE_BLITTER)
         else
      }
      static void
   blorp_measure_start(struct blorp_batch *blorp_batch,
         {
      struct iris_context *ice = blorp_batch->blorp->driver_ctx;
                     if (batch->measure == NULL)
            iris_measure_snapshot(ice, batch,
            }
         static void
   blorp_measure_end(struct blorp_batch *blorp_batch,
         {
               trace_intel_end_blorp(&batch->trace,
                        params->op,
   params->x1 - params->x0,
   params->y1 - params->y0,
   }
      void
   genX(init_blorp)(struct iris_context *ice)
   {
               blorp_init(&ice->blorp, ice, &screen->isl_dev, NULL);
   ice->blorp.compiler = screen->compiler;
   ice->blorp.lookup_shader = iris_blorp_lookup_shader;
   ice->blorp.upload_shader = iris_blorp_upload_shader;
      }
      static void
   blorp_emit_breakpoint_pre_draw(struct blorp_batch *blorp_batch)
   {
      struct iris_batch *batch = blorp_batch->driver_batch;
      }
      static void
   blorp_emit_breakpoint_post_draw(struct blorp_batch *blorp_batch)
   {
      struct iris_batch *batch = blorp_batch->driver_batch;
      }
