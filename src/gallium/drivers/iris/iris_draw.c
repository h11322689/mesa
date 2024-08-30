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
   * @file iris_draw.c
   *
   * The main driver hooks for drawing and launching compute shaders.
   */
      #include <stdio.h>
   #include <errno.h>
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/u_draw.h"
   #include "util/u_inlines.h"
   #include "util/u_transfer.h"
   #include "util/u_upload_mgr.h"
   #include "intel/compiler/brw_compiler.h"
   #include "intel/compiler/brw_eu_defines.h"
   #include "compiler/shader_info.h"
   #include "iris_context.h"
   #include "iris_defines.h"
      static bool
   prim_is_points_or_lines(const struct pipe_draw_info *draw)
   {
      /* We don't need to worry about adjacency - it can only be used with
   * geometry shaders, and we don't care about this info when GS is on.
   */
   return draw->mode == MESA_PRIM_POINTS ||
         draw->mode == MESA_PRIM_LINES ||
      }
      /**
   * Record the current primitive mode and restart information, flagging
   * related packets as dirty if necessary.
   *
   * This must be called before updating compiled shaders, because the patch
   * information informs the TCS key.
   */
   static void
   iris_update_draw_info(struct iris_context *ice,
         {
      struct iris_screen *screen = (struct iris_screen *)ice->ctx.screen;
   const struct intel_device_info *devinfo = screen->devinfo;
            if (ice->state.prim_mode != info->mode) {
      ice->state.prim_mode = info->mode;
               /* For XY Clip enables */
   bool points_or_lines = prim_is_points_or_lines(info);
   if (points_or_lines != ice->state.prim_is_points_or_lines) {
      ice->state.prim_is_points_or_lines = points_or_lines;
                  if (info->mode == MESA_PRIM_PATCHES &&
      ice->state.vertices_per_patch != ice->state.patch_vertices) {
   ice->state.vertices_per_patch = ice->state.patch_vertices;
            /* MULTI_PATCH TCS needs this for key->input_vertices */
   if (compiler->use_tcs_multi_patch)
            /* Flag constants dirty for gl_PatchVerticesIn if needed. */
   const struct shader_info *tcs_info =
         if (tcs_info &&
      BITSET_TEST(tcs_info->system_values_read, SYSTEM_VALUE_VERTICES_IN)) {
   ice->state.stage_dirty |= IRIS_STAGE_DIRTY_CONSTANTS_TCS;
                  /* Track restart_index changes only if primitive_restart is true */
   const unsigned cut_index = info->primitive_restart ? info->restart_index :
         if (ice->state.primitive_restart != info->primitive_restart ||
      ice->state.cut_index != cut_index) {
   ice->state.dirty |= IRIS_DIRTY_VF;
   ice->state.cut_index = cut_index;
   ice->state.dirty |=
      ((ice->state.primitive_restart != info->primitive_restart) &&
            }
      /**
   * Update shader draw parameters, flagging VF packets as dirty if necessary.
   */
   static void
   iris_update_draw_parameters(struct iris_context *ice,
                           {
               if (ice->state.vs_uses_draw_params) {
               if (indirect && indirect->buffer) {
      pipe_resource_reference(&draw_params->res, indirect->buffer);
                  changed = true;
      } else {
               if (!ice->draw.params_valid ||
                     changed = true;
   ice->draw.params.firstvertex = firstvertex;
                  u_upload_data(ice->ctx.const_uploader, 0,
                           if (ice->state.vs_uses_derived_draw_params) {
      struct iris_state_ref *derived_params = &ice->draw.derived_draw_params;
            if (ice->draw.derived_params.drawid != drawid_offset ||
               changed = true;
                  u_upload_data(ice->ctx.const_uploader, 0,
                              if (changed) {
      ice->state.dirty |= IRIS_DIRTY_VERTEX_BUFFERS |
               }
      static void
   iris_indirect_draw_vbo(struct iris_context *ice,
                           {
      struct iris_batch *batch = &ice->batches[IRIS_BATCH_RENDER];
   struct pipe_draw_info info = *dinfo;
            iris_emit_buffer_barrier_for(batch, iris_resource_bo(indirect.buffer),
            if (indirect.indirect_draw_count) {
      struct iris_bo *draw_count_bo =
         iris_emit_buffer_barrier_for(batch, draw_count_bo,
            if (ice->state.predicate == IRIS_PREDICATE_STATE_USE_BIT) {
      /* Upload MI_PREDICATE_RESULT to GPR15.*/
                  const uint64_t orig_dirty = ice->state.dirty;
            for (int i = 0; i < indirect.draw_count; i++) {
                                 ice->state.dirty &= ~IRIS_ALL_DIRTY_FOR_RENDER;
                        if (indirect.indirect_draw_count &&
      ice->state.predicate == IRIS_PREDICATE_STATE_USE_BIT) {
   /* Restore MI_PREDICATE_RESULT. */
               /* Put this back for post-draw resolves, we'll clear it again after. */
   ice->state.dirty = orig_dirty;
      }
      static void
   iris_simple_draw_vbo(struct iris_context *ice,
                           {
                                    }
      /**
   * The pipe->draw_vbo() driver hook.  Performs a draw on the GPU.
   */
   void
   iris_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
      if (num_draws > 1) {
      util_draw_multi(ctx, info, drawid_offset, indirect, draws, num_draws);
               if (!indirect && (!draws[0].count || !info->instance_count))
            struct iris_context *ice = (struct iris_context *) ctx;
   struct iris_screen *screen = (struct iris_screen*)ice->ctx.screen;
   const struct intel_device_info *devinfo = screen->devinfo;
            if (ice->state.predicate == IRIS_PREDICATE_STATE_DONT_RENDER)
            if (INTEL_DEBUG(DEBUG_REEMIT)) {
      ice->state.dirty |= IRIS_ALL_DIRTY_FOR_RENDER;
                        if (devinfo->ver == 9)
                     if (ice->state.dirty & IRIS_DIRTY_RENDER_RESOLVES_AND_FLUSHES) {
      bool draw_aux_buffer_disabled[BRW_MAX_DRAW_BUFFERS] = { };
   for (gl_shader_stage stage = 0; stage < MESA_SHADER_COMPUTE; stage++) {
      if (ice->shaders.prog[stage])
      iris_predraw_resolve_inputs(ice, batch, draw_aux_buffer_disabled,
   }
               if (ice->state.dirty & IRIS_DIRTY_RENDER_MISC_BUFFER_FLUSHES) {
      for (gl_shader_stage stage = 0; stage < MESA_SHADER_COMPUTE; stage++)
                                          if (indirect && indirect->buffer)
         else
                              ice->state.dirty &= ~IRIS_ALL_DIRTY_FOR_RENDER;
      }
      static void
   iris_update_grid_size_resource(struct iris_context *ice,
         {
      const struct iris_screen *screen = (void *) ice->ctx.screen;
   const struct isl_device *isl_dev = &screen->isl_dev;
   struct iris_state_ref *grid_ref = &ice->state.grid_size;
            const struct iris_compiled_shader *shader = ice->shaders.prog[MESA_SHADER_COMPUTE];
   bool grid_needs_surface = shader->bt.used_mask[IRIS_SURFACE_GROUP_CS_WORK_GROUPS];
            if (grid->indirect) {
      pipe_resource_reference(&grid_ref->res, grid->indirect);
            /* Zero out the grid size so that the next non-indirect grid launch will
   * re-upload it properly.
   */
   memset(ice->state.last_grid, 0, sizeof(ice->state.last_grid));
      } else if (memcmp(ice->state.last_grid, grid->grid, sizeof(grid->grid)) != 0) {
      memcpy(ice->state.last_grid, grid->grid, sizeof(grid->grid));
   u_upload_data(ice->state.dynamic_uploader, 0, sizeof(grid->grid), 4,
                     /* If we changed the grid, the old surface state is invalid. */
   if (grid_updated)
            /* Skip surface upload if we don't need it or we already have one */
   if (!grid_needs_surface || state_ref->res)
                     void *surf_map = NULL;
   u_upload_alloc(ice->state.surface_uploader, 0, isl_dev->ss.size,
               state_ref->offset +=
         isl_buffer_fill_state(&screen->isl_dev, surf_map,
                        .address = grid_ref->offset + grid_bo->address,
   .size_B = sizeof(grid->grid),
            }
      void
   iris_launch_grid(struct pipe_context *ctx, const struct pipe_grid_info *grid)
   {
      struct iris_context *ice = (struct iris_context *) ctx;
   struct iris_screen *screen = (struct iris_screen *) ctx->screen;
   const struct intel_device_info *devinfo = screen->devinfo;
            if (ice->state.predicate == IRIS_PREDICATE_STATE_DONT_RENDER)
            if (INTEL_DEBUG(DEBUG_REEMIT)) {
      ice->state.dirty |= IRIS_ALL_DIRTY_FOR_COMPUTE;
               if (ice->state.dirty & IRIS_DIRTY_COMPUTE_RESOLVES_AND_FLUSHES)
            if (ice->state.dirty & IRIS_DIRTY_COMPUTE_MISC_BUFFER_FLUSHES)
                              if (memcmp(ice->state.last_block, grid->block, sizeof(grid->block)) != 0) {
      memcpy(ice->state.last_block, grid->block, sizeof(grid->block));
   ice->state.stage_dirty |= IRIS_STAGE_DIRTY_CONSTANTS_CS;
               if (ice->state.last_grid_dim != grid->work_dim) {
      ice->state.last_grid_dim = grid->work_dim;
   ice->state.stage_dirty |= IRIS_STAGE_DIRTY_CONSTANTS_CS;
                        iris_binder_reserve_compute(ice);
            if (ice->state.compute_predicate) {
      batch->screen->vtbl.load_register_mem64(batch, MI_PREDICATE_RESULT,
                                                ice->state.dirty &= ~IRIS_ALL_DIRTY_FOR_COMPUTE;
            if (devinfo->ver >= 12)
      }
