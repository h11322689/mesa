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
   * @file crocus_blorp.c
   *
   * ============================= GENXML CODE =============================
   *              [This file is compiled once per generation.]
   * =======================================================================
   *
   * GenX specific code for working with BLORP (blitting, resolves, clears
   * on the 3D engine).  This provides the driver-specific hooks needed to
   * implement the BLORP API.
   *
   * See crocus_blit.c, crocus_clear.c, and so on.
   */
      #include <assert.h>
      #include "crocus_batch.h"
   #include "crocus_resource.h"
   #include "crocus_context.h"
      #include "util/u_upload_mgr.h"
   #include "intel/common/intel_l3_config.h"
      #include "blorp/blorp_genX_exec.h"
      #if GFX_VER <= 5
   #include "gen4_blorp_exec.h"
   #endif
      static uint32_t *
   stream_state(struct crocus_batch *batch,
               unsigned size,
   unsigned alignment,
   {
               if (offset + size >= STATE_SZ && !batch->no_wrap) {
      crocus_batch_flush(batch);
      } else if (offset + size >= batch->state.bo->size) {
      const unsigned new_size =
      MIN2(batch->state.bo->size + batch->state.bo->size / 2,
      crocus_grow_buffer(batch, true, batch->state.used, new_size);
                        batch->state.used = offset + size;
            /* If the caller has asked for a BO, we leave them the responsibility of
   * adding bo->gtt_offset (say, by handing an address to genxml).  If not,
   * we assume they want the offset from a base address.
   */
   if (out_bo)
               }
      static void *
   blorp_emit_dwords(struct blorp_batch *blorp_batch, unsigned n)
   {
      struct crocus_batch *batch = blorp_batch->driver_batch;
      }
      static uint64_t
   blorp_emit_reloc(struct blorp_batch *blorp_batch, UNUSED void *location,
         {
      struct crocus_batch *batch = blorp_batch->driver_batch;
            if (GFX_VER < 6 && crocus_ptr_in_state_buffer(batch, location)) {
      offset = (char *)location - (char *)batch->state.map;
   return crocus_state_reloc(batch, offset,
                              offset = (char *)location - (char *)batch->command.map;
   return crocus_command_reloc(batch, offset,
            }
      static void
   blorp_surface_reloc(struct blorp_batch *blorp_batch, uint32_t ss_offset,
         {
      struct crocus_batch *batch = blorp_batch->driver_batch;
            uint64_t reloc_val =
      crocus_state_reloc(batch, ss_offset, bo, addr.offset + delta,
         void *reloc_ptr = (void *)batch->state.map + ss_offset;
      }
      static uint64_t
   blorp_get_surface_address(struct blorp_batch *blorp_batch,
         {
      /* We'll let blorp_surface_reloc write the address. */
      }
      #if GFX_VER >= 7
   static struct blorp_address
   blorp_get_surface_base_address(struct blorp_batch *blorp_batch)
   {
      struct crocus_batch *batch = blorp_batch->driver_batch;
   return (struct blorp_address) {
      .buffer = batch->state.bo,
         }
   #endif
      static void *
   blorp_alloc_dynamic_state(struct blorp_batch *blorp_batch,
                     {
                  }
      UNUSED static void *
   blorp_alloc_general_state(struct blorp_batch *blorp_batch,
                     {
      /* Use dynamic state range for general state on crocus. */
      }
      static void
   blorp_alloc_binding_table(struct blorp_batch *blorp_batch,
                           unsigned num_entries,
   unsigned state_size,
   {
      struct crocus_batch *batch = blorp_batch->driver_batch;
   uint32_t *bt_map = stream_state(batch, num_entries * sizeof(uint32_t), 32,
            for (unsigned i = 0; i < num_entries; i++) {
      surface_maps[i] = stream_state(batch,
                     }
      static uint32_t
   blorp_binding_table_offset_to_pointer(struct blorp_batch *batch,
         {
         }
      static void *
   blorp_alloc_vertex_buffer(struct blorp_batch *blorp_batch,
               {
      struct crocus_batch *batch = blorp_batch->driver_batch;
   struct crocus_bo *bo;
            void *map = stream_state(batch, size, 64,
            *addr = (struct blorp_address) {
      .buffer = bo,
   .offset = offset,
   #if GFX_VER >= 7
         #endif
                  }
      /**
   */
   static void
   blorp_vf_invalidate_for_vb_48b_transitions(struct blorp_batch *blorp_batch,
                     {
   }
      static struct blorp_address
   blorp_get_workaround_address(struct blorp_batch *blorp_batch)
   {
               return (struct blorp_address) {
      .buffer = batch->ice->workaround_bo,
         }
      static void
   blorp_flush_range(UNUSED struct blorp_batch *blorp_batch,
               {
      /* All allocated states come from the batch which we will flush before we
   * submit it.  There's nothing for us to do here.
      }
      #if GFX_VER >= 7
   static const struct intel_l3_config *
   blorp_get_l3_config(struct blorp_batch *blorp_batch)
   {
      struct crocus_batch *batch = blorp_batch->driver_batch;
      }
   #else /* GFX_VER < 7 */
   static void
   blorp_emit_urb_config(struct blorp_batch *blorp_batch,
               {
         #if GFX_VER <= 5
         #else
         #endif
   }
   #endif
      static void
   crocus_blorp_exec(struct blorp_batch *blorp_batch,
         {
      struct crocus_context *ice = blorp_batch->blorp->driver_ctx;
            /* Flush the sampler and render caches.  We definitely need to flush the
   * sampler cache so that we get updated contents from the render cache for
   * the glBlitFramebuffer() source.  Also, we are sometimes warned in the
   * docs to flush the cache between reinterpretations of the same surface
   * data with different formats, which blorp does for stencil and depth
   * data.
   */
   if (params->src.enabled)
         if (params->dst.enabled) {
      crocus_cache_flush_for_render(batch, params->dst.addr.buffer,
            }
   if (params->depth.enabled)
         if (params->stencil.enabled)
            crocus_require_command_space(batch, 1400);
   crocus_require_statebuffer_space(batch, 600);
         #if GFX_VER == 8
         #endif
      #if GFX_VER == 6
      /* Emit workaround flushes when we switch from drawing to blorping. */
      #endif
      #if GFX_VER >= 6
         #endif
         blorp_emit(blorp_batch, GENX(3DSTATE_DRAWING_RECTANGLE), rect) {
      rect.ClippedDrawingRectangleXMax = MAX2(params->x1, params->x0) - 1;
               batch->screen->vtbl.update_surface_base_address(batch);
            batch->contains_draw = true;
            batch->no_wrap = false;
            /* We've smashed all state compared to what the normal 3D pipeline
   * rendering tracks for GL.
            uint64_t skip_bits = (CROCUS_DIRTY_POLYGON_STIPPLE |
                        CROCUS_DIRTY_GEN7_SO_BUFFERS |
   CROCUS_DIRTY_SO_DECL_LIST |
   CROCUS_DIRTY_LINE_STIPPLE |
         uint64_t skip_stage_bits = (CROCUS_ALL_STAGE_DIRTY_FOR_COMPUTE |
                              CROCUS_STAGE_DIRTY_UNCOMPILED_VS |
   CROCUS_STAGE_DIRTY_UNCOMPILED_TCS |
   CROCUS_STAGE_DIRTY_UNCOMPILED_TES |
   CROCUS_STAGE_DIRTY_UNCOMPILED_GS |
         if (!ice->shaders.uncompiled[MESA_SHADER_TESS_EVAL]) {
         skip_stage_bits |= CROCUS_STAGE_DIRTY_TCS |
                        CROCUS_STAGE_DIRTY_TES |
               if (!ice->shaders.uncompiled[MESA_SHADER_GEOMETRY]) {
         skip_stage_bits |= CROCUS_STAGE_DIRTY_GS |
                        /* we can skip flagging CROCUS_DIRTY_DEPTH_BUFFER, if
   * BLORP_BATCH_NO_EMIT_DEPTH_STENCIL is set.
   */
   if (blorp_batch->flags & BLORP_BATCH_NO_EMIT_DEPTH_STENCIL)
            if (!params->wm_prog_data)
            ice->state.dirty |= ~skip_bits;
            ice->urb.vsize = 0;
   ice->urb.gs_present = false;
   ice->urb.gsize = 0;
   ice->urb.tess_present = false;
   ice->urb.hsize = 0;
            if (params->dst.enabled) {
      crocus_render_cache_add_bo(batch, params->dst.addr.buffer,
            }
   if (params->depth.enabled)
         if (params->stencil.enabled)
      }
      static void
   blorp_measure_start(struct blorp_batch *blorp_batch,
         {
   }
      static void
   blorp_measure_end(struct blorp_batch *blorp_batch,
         {
   }
      static void
   blorp_emit_breakpoint_pre_draw(struct blorp_batch *batch)
   {
         }
      static void
   blorp_emit_breakpoint_post_draw(struct blorp_batch *batch)
   {
         }
      void
   genX(crocus_init_blorp)(struct crocus_context *ice)
   {
               blorp_init(&ice->blorp, ice, &screen->isl_dev, NULL);
   ice->blorp.compiler = screen->compiler;
   ice->blorp.lookup_shader = crocus_blorp_lookup_shader;
   ice->blorp.upload_shader = crocus_blorp_upload_shader;
      }
