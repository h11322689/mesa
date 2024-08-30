   /**********************************************************
   * Copyright 2008-2022 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "util/u_bitmask.h"
   #include "util/u_debug.h"
   #include "pipe/p_defines.h"
   #include "util/u_memory.h"
   #include "draw/draw_context.h"
      #include "svga_context.h"
   #include "svga_screen.h"
   #include "svga_state.h"
   #include "svga_draw.h"
   #include "svga_cmd.h"
   #include "svga_hw_reg.h"
      /* This is just enough to decide whether we need to use the draw
   * module (swtnl) or not.
   */
   static const struct svga_tracked_state *need_swtnl_state[] =
   {
      &svga_update_need_swvfetch,
   &svga_update_need_pipeline,
   &svga_update_need_swtnl,
      };
         /* Atoms to update hardware state prior to emitting a clear or draw
   * packet.
   */
   static const struct svga_tracked_state *hw_clear_state[] =
   {
      &svga_hw_scissor,
   &svga_hw_viewport,
   &svga_hw_framebuffer,
      };
         /**
   * Atoms to update hardware state prior to emitting a draw packet
   * for VGPU9 device.
   */
   static const struct svga_tracked_state *hw_draw_state_vgpu9[] =
   {
      &svga_hw_fs,
   &svga_hw_vs,
   &svga_hw_rss,
   &svga_hw_tss,
   &svga_hw_tss_binding,
   &svga_hw_clip_planes,
   &svga_hw_vdecl,
   &svga_hw_fs_constants,
   &svga_hw_vs_constants,
      };
         /**
   * Atoms to update hardware state prior to emitting a draw packet
   * for VGPU10 device.
   * Geometry Shader is new to VGPU10.
   * TSS and TSS bindings are replaced by sampler and sampler bindings.
   */
   static const struct svga_tracked_state *hw_draw_state_vgpu10[] =
   {
      &svga_need_tgsi_transform,
   &svga_hw_fs,
   &svga_hw_gs,
   &svga_hw_vs,
   &svga_hw_rss,
   &svga_hw_sampler,
   &svga_hw_sampler_bindings,
   &svga_hw_clip_planes,
   &svga_hw_vdecl,
   &svga_hw_fs_constants,
   &svga_hw_fs_constbufs,
   &svga_hw_gs_constants,
   &svga_hw_gs_constbufs,
   &svga_hw_vs_constants,
   &svga_hw_vs_constbufs,
      };
         /**
   * Atoms to update hardware state prior to emitting a draw packet
   * for SM5 device.
   * TCS and TES Shaders are new to SM5 device.
   */
   static const struct svga_tracked_state *hw_draw_state_sm5[] =
   {
      &svga_need_tgsi_transform,
   &svga_hw_fs,
   &svga_hw_gs,
   &svga_hw_tes,
   &svga_hw_tcs,
   &svga_hw_vs,
   &svga_hw_rss,
   &svga_hw_sampler,
   &svga_hw_sampler_bindings,
   &svga_hw_clip_planes,
   &svga_hw_vdecl,
   &svga_hw_fs_constants,
   &svga_hw_fs_constbufs,
   &svga_hw_gs_constants,
   &svga_hw_gs_constbufs,
   &svga_hw_tes_constants,
   &svga_hw_tes_constbufs,
   &svga_hw_tcs_constants,
   &svga_hw_tcs_constbufs,
   &svga_hw_vs_constants,
   &svga_hw_vs_constbufs,
      };
         /**
   * Atoms to update hardware state prior to emitting a draw packet
   * for GL43 device which includes uav update.
   */
   static const struct svga_tracked_state *hw_draw_state_gl43[] =
   {
      &svga_need_tgsi_transform,
   &svga_need_rawbuf_srv,
   &svga_hw_uav,
   &svga_hw_fs,
   &svga_hw_gs,
   &svga_hw_tes,
   &svga_hw_tcs,
   &svga_hw_vs,
   &svga_hw_rss,
   &svga_hw_sampler,
   &svga_hw_sampler_bindings,
   &svga_hw_clip_planes,
   &svga_hw_vdecl,
   &svga_hw_fs_constants,
   &svga_hw_fs_constbufs,
   &svga_hw_gs_constants,
   &svga_hw_gs_constbufs,
   &svga_hw_tes_constants,
   &svga_hw_tes_constbufs,
   &svga_hw_tcs_constants,
   &svga_hw_tcs_constbufs,
   &svga_hw_vs_constants,
   &svga_hw_vs_constbufs,
      };
         static const struct svga_tracked_state *swtnl_draw_state[] =
   {
      &svga_update_swtnl_draw,
   &svga_update_swtnl_vdecl,
      };
         /* Flattens the graph of state dependencies.  Could swap the positions
   * of hw_clear_state and need_swtnl_state without breaking anything.
   */
   static const struct svga_tracked_state **state_levels[] =
   {
      need_swtnl_state,
   hw_clear_state,
   NULL,              /* hw_draw_state, to be set to the right version */
      };
         static uint64_t
   check_state(uint64_t a, uint64_t b)
   {
         }
      static void
   accumulate_state(uint64_t *a, uint64_t b)
   {
         }
         static void
   xor_states(uint64_t *result, uint64_t a, uint64_t b)
   {
         }
         static enum pipe_error
   update_state(struct svga_context *svga,
               {
   #ifdef DEBUG
         #else
         #endif
      enum pipe_error ret = PIPE_OK;
            ret = svga_hwtnl_flush( svga->hwtnl );
   if (ret != PIPE_OK)
            if (debug) {
      /* Debug version which enforces various sanity checks on the
   * state flags which are generated and checked to help ensure
   * state atoms are ordered correctly in the list.
   */
            examined = 0;
            for (i = 0; atoms[i] != NULL; i++) {
                              if (check_state(*state, atoms[i]->dirty)) {
      if (0)
         ret = atoms[i]->update( svga, *state );
   if (ret != PIPE_OK)
               /* generated = (prev ^ state)
   * if (examined & generated)
   *     fail;
   */
   xor_states(&generated, prev, *state);
   if (check_state(examined, generated)) {
      debug_printf("state atom %s generated state already examined\n",
                     prev = *state;
         }
   else {
      for (i = 0; atoms[i] != NULL; i++) {
      if (check_state(*state, atoms[i]->dirty)) {
      ret = atoms[i]->update( svga, *state );
   if (ret != PIPE_OK)
                        }
         enum pipe_error
   svga_update_state(struct svga_context *svga, unsigned max_level)
   {
      struct svga_screen *screen = svga_screen(svga->pipe.screen);
   enum pipe_error ret = PIPE_OK;
                     /* Check for updates to bound textures.  This can't be done in an
   * atom as there is no flag which could provoke this test, and we
   * cannot create one.
   */
   if (svga->state.texture_timestamp != screen->texture_timestamp) {
      svga->state.texture_timestamp = screen->texture_timestamp;
               for (i = 0; i <= max_level; i++) {
               if (svga->dirty) {
      ret = update_state( svga,
                                             for (; i < SVGA_STATE_MAX; i++)
                           done:
      SVGA_STATS_TIME_POP(screen->sws);
      }
         /**
   * Update state.  If the first attempt fails, flush the command buffer
   * and retry.
   * \return  true if success, false if second attempt fails.
   */
   bool
   svga_update_state_retry(struct svga_context *svga, unsigned max_level)
   {
                           }
         #define EMIT_RS(_rs, _count, _name, _value)     \
   do {                                            \
      _rs[_count].state = _name;                   \
   _rs[_count].uintValue = _value;              \
      } while (0)
         /* Setup any hardware state which will be constant through the life of
   * a context.
   */
   enum pipe_error
   svga_emit_initial_state(struct svga_context *svga)
   {
      if (svga_have_vgpu10(svga)) {
      SVGA3dRasterizerStateId id = util_bitmask_add(svga->rast_object_id_bm);
            /* XXX preliminary code */
   ret = SVGA3D_vgpu10_DefineRasterizerState(svga->swc,
                                          id,
   SVGA3D_FILLMODE_FILL,
   SVGA3D_CULL_NONE,
   1, /* frontCounterClockwise */
   0, /* depthBias */
   0.0f, /* depthBiasClamp */
   0.0f, /* slopeScaledDepthBiasClamp */
   0, /* depthClampEnable */
                           ret = SVGA3D_vgpu10_SetRasterizerState(svga->swc, id);
      }
   else {
      SVGA3dRenderState *rs;
   unsigned count = 0;
   const unsigned COUNT = 2;
            ret = SVGA3D_BeginSetRenderState( svga->swc, &rs, COUNT );
   if (ret != PIPE_OK)
            /* Always use D3D style coordinate space as this is the only one
   * which is implemented on all backends.
   */
   EMIT_RS(rs, count, SVGA3D_RS_COORDINATETYPE,
                  assert( COUNT == count );
                  }
         void
   svga_init_tracked_state(struct svga_context *svga)
   {
      /* Set the hw_draw_state atom list to the one for the particular gpu version.
   */
   state_levels[2] =
      svga_have_gl43(svga) ? hw_draw_state_gl43 :
      (svga_have_sm5(svga) ? hw_draw_state_sm5 :
      }
         static const struct svga_tracked_state *compute_state[] =
   {
      &svga_hw_cs_uav,
   &svga_hw_cs_sampler,
   &svga_hw_cs_sampler_bindings,
   &svga_hw_cs,
   &svga_hw_cs_constants,
   &svga_hw_cs_constbufs,
      };
      /**
   * Update compute state.
   * If the first attempt fails, flush the command buffer and retry.
   * \return  true if success, false if second attempt fails.
   */
   bool
   svga_update_compute_state(struct svga_context *svga)
   {
      enum pipe_error ret = PIPE_OK;
            if (compute_dirty) {
      SVGA_RETRY_OOM(svga, ret, update_state(svga, compute_state,
            /* Set the dirty flag to the remaining dirty bits which are
   * not processed in the compute pipeline.
   */
                  }
