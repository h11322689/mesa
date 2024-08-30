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
   assert(cmd_buffer->batch.start <= location &&
         return anv_batch_emit_reloc(&cmd_buffer->batch, location,
      }
      static void
   blorp_surface_reloc(struct blorp_batch *batch, uint32_t ss_offset,
         {
      struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
            if (ANV_ALWAYS_SOFTPIN) {
      result = anv_reloc_list_add_bo(&cmd_buffer->surface_relocs,
               if (unlikely(result != VK_SUCCESS))
                     uint64_t address_u64 = 0;
   result = anv_reloc_list_add(&cmd_buffer->surface_relocs,
                           if (result != VK_SUCCESS)
            void *dest = anv_block_pool_map(
            }
      static uint64_t
   blorp_get_surface_address(struct blorp_batch *blorp_batch,
         {
      if (ANV_ALWAYS_SOFTPIN) {
      struct anv_address anv_addr = {
      .bo = address.buffer,
      };
      } else {
      /* We'll let blorp_surface_reloc write the address. */
         }
      static struct blorp_address
   blorp_get_surface_base_address(struct blorp_batch *batch)
   {
      struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
   return (struct blorp_address) {
      .buffer = cmd_buffer->device->surface_state_pool.block_pool.bo,
         }
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
      static void
   blorp_exec_on_render(struct blorp_batch *batch,
         {
               struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
                     /* Apply any outstanding flushes in case pipeline select haven't. */
                     /* BLORP doesn't do anything fancy with depth such as discards, so we want
   * the PMA fix off.  Also, off is always the safe option.
   */
                     /* Calculate state that does not get touched by blorp.
   * Flush everything else.
   */
   anv_cmd_dirty_mask_t dirty = ~(ANV_CMD_DIRTY_INDEX_BUFFER |
            BITSET_DECLARE(dyn_dirty, MESA_VK_DYNAMIC_GRAPHICS_STATE_ENUM_MAX);
   BITSET_ONES(dyn_dirty);
   BITSET_CLEAR(dyn_dirty, MESA_VK_DYNAMIC_IA_PRIMITIVE_RESTART_ENABLE);
   BITSET_CLEAR(dyn_dirty, MESA_VK_DYNAMIC_VP_SCISSOR_COUNT);
   BITSET_CLEAR(dyn_dirty, MESA_VK_DYNAMIC_VP_SCISSORS);
   BITSET_CLEAR(dyn_dirty, MESA_VK_DYNAMIC_RS_LINE_STIPPLE);
   BITSET_CLEAR(dyn_dirty, MESA_VK_DYNAMIC_FSR);
   BITSET_CLEAR(dyn_dirty, MESA_VK_DYNAMIC_MS_SAMPLE_LOCATIONS);
   if (!params->wm_prog_data) {
      BITSET_CLEAR(dyn_dirty, MESA_VK_DYNAMIC_CB_COLOR_WRITE_ENABLES);
               cmd_buffer->state.gfx.vb_dirty = ~0;
   cmd_buffer->state.gfx.dirty |= dirty;
   BITSET_OR(cmd_buffer->vk.dynamic_graphics_state.dirty,
            }
      static void
   blorp_exec_on_compute(struct blorp_batch *batch,
         {
               struct anv_cmd_buffer *cmd_buffer = batch->driver_batch;
                     /* Apply any outstanding flushes in case pipeline select haven't. */
                        }
      void
   genX(blorp_exec)(struct blorp_batch *batch,
         {
               if (!cmd_buffer->state.current_l3_config) {
      const struct intel_l3_config *cfg =
                  #if GFX_VER == 7
      /* The MI_LOAD/STORE_REGISTER_MEM commands which BLORP uses to implement
   * indirect fast-clear colors can cause GPU hangs if we don't stall first.
   * See genX(cmd_buffer_mi_memcpy) for more details.
   */
   if (params->src.clear_color_addr.buffer ||
      params->dst.clear_color_addr.buffer) {
   anv_add_pending_pipe_bits(cmd_buffer,
               #endif
         if (batch->flags & BLORP_BATCH_USE_COMPUTE)
         else
      }
      static void
   blorp_emit_breakpoint_pre_draw(struct blorp_batch *batch)
   {
         }
      static void
   blorp_emit_breakpoint_post_draw(struct blorp_batch *batch)
   {
         }
