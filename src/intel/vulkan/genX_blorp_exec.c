   /*
   * Copyright © 2016 Intel Corporation
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
      #include <assert.h>
      #include "anv_private.h"
   #include "anv_measure.h"
      /* These are defined in anv_private.h and blorp_genX_exec.h */
   #undef __gen_address_type
   #undef __gen_user_data
   #undef __gen_combine_address
      #include "common/intel_l3_config.h"
   #include "blorp/blorp_genX_exec.h"
      #include "ds/intel_tracepoints.h"
      static void blorp_measure_start(struct blorp_batch *_batch,
         {
      struct anv_cmd_buffer *cmd_buffer = _batch->driver_batch;
   trace_intel_begin_blorp(&cmd_buffer->trace);
   anv_measure_snapshot(cmd_buffer,
            }
      static void blorp_measure_end(struct blorp_batch *_batch,
         {
      struct anv_cmd_buffer *cmd_buffer = _batch->driver_batch;
   trace_intel_end_blorp(&cmd_buffer->trace,
                        params->op,
   params->x1 - params->x0,
   params->y1 - params->y0,
   }
      static void *
   blorp_emit_dwords(struct blorp_batch *batch, unsigned n)
   {
      struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
      }
      static uint64_t
   blorp_emit_reloc(struct blorp_batch *batch,
         {
      struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
   struct anv_address anv_addr = {
      .bo = address.buffer,
      };
   anv_reloc_list_add_bo(cmd_buffer->batch.relocs, anv_addr.bo);
      }
      static void
   blorp_surface_reloc(struct blorp_batch *batch, uint32_t ss_offset,
         {
               VkResult result = anv_reloc_list_add_bo(&cmd_buffer->surface_relocs,
         if (unlikely(result != VK_SUCCESS))
      }
      static uint64_t
   blorp_get_surface_address(struct blorp_batch *blorp_batch,
         {
      struct anv_address anv_addr = {
      .bo = address.buffer,
      };
      }
      #if GFX_VER == 9
   static struct blorp_address
   blorp_get_surface_base_address(struct blorp_batch *batch)
   {
      struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
   return (struct blorp_address) {
      .buffer = cmd_buffer->device->internal_surface_state_pool.block_pool.bo,
         }
   #endif
      static void *
   blorp_alloc_dynamic_state(struct blorp_batch *batch,
                     {
               struct anv_state state =
            *offset = state.offset;
      }
      UNUSED static void *
   blorp_alloc_general_state(struct blorp_batch *batch,
                     {
               struct anv_state state =
      anv_state_stream_alloc(&cmd_buffer->general_state_stream, size,
         *offset = state.offset;
      }
      static void
   blorp_alloc_binding_table(struct blorp_batch *batch, unsigned num_entries,
                     {
               uint32_t state_offset;
            VkResult result =
      anv_cmd_buffer_alloc_blorp_binding_table(cmd_buffer, num_entries,
      if (result != VK_SUCCESS)
            uint32_t *bt_map = bt_state.map;
            for (unsigned i = 0; i < num_entries; i++) {
      struct anv_state surface_state =
         bt_map[i] = surface_state.offset + state_offset;
   surface_offsets[i] = surface_state.offset;
         }
      static uint32_t
   blorp_binding_table_offset_to_pointer(struct blorp_batch *batch,
         {
         }
      static void *
   blorp_alloc_vertex_buffer(struct blorp_batch *batch, uint32_t size,
         {
      struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
   struct anv_state vb_state =
            *addr = (struct blorp_address) {
      .buffer = cmd_buffer->device->dynamic_state_pool.block_pool.bo,
   .offset = vb_state.offset,
   .mocs = isl_mocs(&cmd_buffer->device->isl_dev,
                  }
      static void
   blorp_vf_invalidate_for_vb_48b_transitions(struct blorp_batch *batch,
                     {
   #if GFX_VER == 9
               for (unsigned i = 0; i < num_vbs; i++) {
      struct anv_address anv_addr = {
      .bo = addrs[i].buffer,
      };
   genX(cmd_buffer_set_binding_for_gfx8_vb_flush)(cmd_buffer,
                        /* Technically, we should call this *after* 3DPRIMITIVE but it doesn't
   * really matter for blorp because we never call apply_pipe_flushes after
   * this point.
   */
   genX(cmd_buffer_update_dirty_vbs_for_gfx8_vb_flush)(cmd_buffer, SEQUENTIAL,
      #endif
   }
      UNUSED static struct blorp_address
   blorp_get_workaround_address(struct blorp_batch *batch)
   {
               return (struct blorp_address) {
      .buffer = cmd_buffer->device->workaround_address.bo,
         }
      static void
   blorp_flush_range(struct blorp_batch *batch, void *start, size_t size)
   {
      /* We don't need to flush states anymore, since everything will be snooped.
      }
      static const struct intel_l3_config *
   blorp_get_l3_config(struct blorp_batch *batch)
   {
      struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
      }
      static bool
   blorp_uses_bti_rt_writes(const struct blorp_batch *batch, const struct blorp_params *params)
   {
      if (batch->flags & (BLORP_BATCH_USE_BLITTER | BLORP_BATCH_USE_COMPUTE))
            /* HIZ clears use WM_HZ ops rather than a clear shader using RT writes. */
      }
      static void
   blorp_exec_on_render(struct blorp_batch *batch,
         {
               struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
            struct anv_gfx_dynamic_state *hw_state =
            const unsigned scale = params->fast_clear_op ? UINT_MAX : 1;
   genX(cmd_buffer_emit_hashing_mode)(cmd_buffer, params->x1 - params->x0,
         #if GFX_VER >= 11
      /* The PIPE_CONTROL command description says:
   *
   *    "Whenever a Binding Table Index (BTI) used by a Render Target Message
   *     points to a different RENDER_SURFACE_STATE, SW must issue a Render
   *     Target Cache Flush by enabling this bit. When render target flush
   *     is set due to new association of BTI, PS Scoreboard Stall bit must
   *     be set in this packet."
   */
   if (blorp_uses_bti_rt_writes(batch, params)) {
      anv_add_pending_pipe_bits(cmd_buffer,
                     #endif
      #if GFX_VERx10 >= 125
      /* Check if blorp ds state matches ours. */
   if (intel_needs_workaround(cmd_buffer->device->info, 18019816803)) {
      bool blorp_ds_state = params->depth.enabled || params->stencil.enabled;
   if (cmd_buffer->state.gfx.ds_write_state != blorp_ds_state) {
      batch->flags |= BLORP_BATCH_NEED_PSS_STALL_SYNC;
   cmd_buffer->state.gfx.ds_write_state = blorp_ds_state;
            #endif
         if (params->depth.enabled &&
      !(batch->flags & BLORP_BATCH_NO_EMIT_DEPTH_STENCIL))
                  /* Wa_14015814527 */
            /* Apply any outstanding flushes in case pipeline select haven't. */
            /* BLORP doesn't do anything fancy with depth such as discards, so we want
   * the PMA fix off.  Also, off is always the safe option.
   */
                  #if GFX_VER >= 11
      /* The PIPE_CONTROL command description says:
   *
   *    "Whenever a Binding Table Index (BTI) used by a Render Target Message
   *     points to a different RENDER_SURFACE_STATE, SW must issue a Render
   *     Target Cache Flush by enabling this bit. When render target flush
   *     is set due to new association of BTI, PS Scoreboard Stall bit must
   *     be set in this packet."
   */
   if (blorp_uses_bti_rt_writes(batch, params)) {
      anv_add_pending_pipe_bits(cmd_buffer,
                     #endif
         /* Flag all the instructions emitted by BLORP. */
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_URB);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_VF_STATISTICS);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_VF);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_VF_TOPOLOGY);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_VERTEX_INPUT);
      #if GFX_VER >= 11
         #endif
   #if GFX_VER >= 12
         #endif
      BITSET_SET(hw_state->dirty, ANV_GFX_STATE_VIEWPORT_CC);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_STREAMOUT);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_RASTER);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_CLIP);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_SAMPLE_MASK);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_MULTISAMPLE);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_SF);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_SBE);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_SBE_SWIZ);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_DEPTH_BOUNDS);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_WM);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_WM_DEPTH_STENCIL);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_VS);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_HS);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_DS);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_TE);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_GS);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_PS);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_PS_EXTRA);
   BITSET_SET(hw_state->dirty, ANV_GFX_STATE_BLEND_STATE_POINTERS);
   if (batch->blorp->config.use_mesh_shading) {
      BITSET_SET(hw_state->dirty, ANV_GFX_STATE_MESH_CONTROL);
      }
   if (params->wm_prog_data) {
      BITSET_SET(hw_state->dirty, ANV_GFX_STATE_CC_STATE);
               anv_cmd_dirty_mask_t dirty = ~(ANV_CMD_DIRTY_INDEX_BUFFER |
            cmd_buffer->state.gfx.vb_dirty = ~0;
   cmd_buffer->state.gfx.dirty |= dirty;
      }
      static void
   blorp_exec_on_compute(struct blorp_batch *batch,
         {
               struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
                     /* Apply any outstanding flushes in case pipeline select haven't. */
                        }
      static void
   blorp_exec_on_blitter(struct blorp_batch *batch,
         {
               struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
               }
      void
   genX(blorp_exec)(struct blorp_batch *batch,
         {
               /* Turn on preemption if it was toggled off. */
   if (!cmd_buffer->state.gfx.object_preemption)
            if (!cmd_buffer->state.current_l3_config) {
      const struct intel_l3_config *cfg =
                     if (batch->flags & BLORP_BATCH_USE_BLITTER)
         else if (batch->flags & BLORP_BATCH_USE_COMPUTE)
         else
      }
      static void
   blorp_emit_breakpoint_pre_draw(struct blorp_batch *batch)
   {
      struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
      }
      static void
   blorp_emit_breakpoint_post_draw(struct blorp_batch *batch)
   {
      struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
      }
