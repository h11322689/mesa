   /*
   * Copyright © 2014-2017 Broadcom
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
      #include "util/u_blitter.h"
   #include "util/u_draw.h"
   #include "util/u_prim.h"
   #include "util/format/u_format.h"
   #include "util/u_helpers.h"
   #include "util/u_pack_color.h"
   #include "util/u_prim_restart.h"
   #include "util/u_upload_mgr.h"
      #include "v3d_context.h"
   #include "v3d_resource.h"
   #include "v3d_cl.h"
   #include "broadcom/compiler/v3d_compiler.h"
   #include "broadcom/common/v3d_macros.h"
   #include "broadcom/common/v3d_util.h"
   #include "broadcom/cle/v3dx_pack.h"
      void
   v3dX(start_binning)(struct v3d_context *v3d, struct v3d_job *job)
   {
                  /* Get space to emit our BCL state, using a branch to jump to a new BO
                           job->submit.bcl_start = job->bcl.bo->offset;
            /* The PTB will request the tile alloc initial size per tile at start
      * of tile binning.
      uint32_t tile_alloc_size =
            /* The PTB allocates in aligned 4k chunks after the initial setup. */
            /* Include the first two chunk allocations that the PTB does so that
      * we definitely clear the OOM condition before triggering one (the HW
   * won't trigger OOM during the first allocations).
               /* For performance, allocate some extra initial memory after the PTB's
      * minimal allocations, so that we hopefully don't have to block the
   * GPU on the kernel handling an OOM signal.
               job->tile_alloc = v3d_bo_alloc(v3d->screen, tile_alloc_size,
         uint32_t tsda_per_tile_size = v3d->screen->devinfo.ver >= 40 ? 256 : 64;
   job->tile_state = v3d_bo_alloc(v3d->screen,
                              #if V3D_VERSION >= 41
         /* This must go before the binning mode configuration. It is
      * required for layered framebuffers to work.
      if (job->num_layers > 0) {
            cl_emit(&job->bcl, NUMBER_OF_LAYERS, config) {
      #endif
            #if V3D_VERSION >= 71
         cl_emit(&job->bcl, TILE_BINNING_MODE_CFG, config) {
                                          /* FIXME: ideallly we would like next assert on the packet header (as is
   * general, so also applies to GL). We would need to expand
   * gen_pack_header for that.
   */
         #endif
      #if V3D_VERSION >= 40 && V3D_VERSION <= 42
         cl_emit(&job->bcl, TILE_BINNING_MODE_CFG, config) {
            config.width_in_pixels = job->draw_width;
                                    #endif
   #if V3D_VERSION < 40
         /* "Binning mode lists start with a Tile Binning Mode Configuration
      * item (120)"
   *
   * Part1 signals the end of binning config setup.
      cl_emit(&job->bcl, TILE_BINNING_MODE_CFG_PART2, config) {
            config.tile_allocation_memory_address =
               cl_emit(&job->bcl, TILE_BINNING_MODE_CFG_PART1, config) {
                           config.width_in_tiles = job->draw_tiles_x;
   config.height_in_tiles = job->draw_tiles_y;
                                    #endif
            /* There's definitely nothing in the VCD cache we want. */
            /* Disable any leftover OQ state from another job. */
            /* "Binning mode lists must have a Start Tile Binning item (6) after
      *  any prefix state data before the binning list proper starts."
      }
   /**
   * Does the initial bining command list setup for drawing to a given FBO.
   */
   static void
   v3d_start_draw(struct v3d_context *v3d)
   {
                  if (job->needs_flush)
            job->needs_flush = true;
   job->draw_width = v3d->framebuffer.width;
   job->draw_height = v3d->framebuffer.height;
            }
      static void
   v3d_predraw_check_stage_inputs(struct pipe_context *pctx,
         {
                  /* Flush writes to textures we're sampling. */
   for (int i = 0; i < v3d->tex[s].num_textures; i++) {
            struct pipe_sampler_view *pview = v3d->tex[s].textures[i];
                                             v3d_flush_jobs_writing_resource(v3d, view->texture,
               /* Flush writes to UBOs. */
   u_foreach_bit(i, v3d->constbuf[s].enabled_mask) {
            struct pipe_constant_buffer *cb = &v3d->constbuf[s].cb[i];
   if (cb->buffer) {
            v3d_flush_jobs_writing_resource(v3d, cb->buffer,
            /* Flush reads/writes to our SSBOs */
   u_foreach_bit(i, v3d->ssbo[s].enabled_mask) {
            struct pipe_shader_buffer *sb = &v3d->ssbo[s].sb[i];
   if (sb->buffer) {
            v3d_flush_jobs_reading_resource(v3d, sb->buffer,
            /* Flush reads/writes to our image views */
   u_foreach_bit(i, v3d->shaderimg[s].enabled_mask) {
                     v3d_flush_jobs_reading_resource(v3d, view->base.resource,
               /* Flush writes to our vertex buffers (i.e. from transform feedback) */
   if (s == PIPE_SHADER_VERTEX) {
                                    v3d_flush_jobs_writing_resource(v3d, vb->buffer.resource,
   }
      static void
   v3d_predraw_check_outputs(struct pipe_context *pctx)
   {
                  /* Flush jobs reading from TF buffers that we are about to write. */
   if (v3d_transform_feedback_enabled(v3d)) {
                                                   const struct pipe_stream_output_target *target =
         v3d_flush_jobs_reading_resource(v3d, target->buffer,
   }
      /**
   * Checks if the state for the current draw reads a particular resource in
   * in the given shader stage.
   */
   static bool
   v3d_state_reads_resource(struct v3d_context *v3d,
               {
                  /* Vertex buffers */
   if (s == PIPE_SHADER_VERTEX) {
            u_foreach_bit(i, v3d->vertexbuf.enabled_mask) {
                                 struct v3d_resource *vb_rsc =
                  /* Constant buffers */
   u_foreach_bit(i, v3d->constbuf[s].enabled_mask) {
                                 struct v3d_resource *cb_rsc = v3d_resource(cb->buffer);
               /* Shader storage buffers */
   u_foreach_bit(i, v3d->ssbo[s].enabled_mask) {
                                 struct v3d_resource *sb_rsc = v3d_resource(sb->buffer);
               /* Textures  */
   for (int i = 0; i < v3d->tex[s].num_textures; i++) {
                                 struct v3d_sampler_view *view = v3d_sampler_view(pview);
   struct v3d_resource *v_rsc = v3d_resource(view->texture);
               }
      static void
   v3d_emit_wait_for_tf(struct v3d_job *job)
   {
         /* XXX: we might be able to skip this in some cases, for now we
      * always emit it.
               cl_emit(&job->bcl, WAIT_FOR_TRANSFORM_FEEDBACK, wait) {
            /* XXX: Wait for all outstanding writes... maybe we can do
   * better in some cases.
               /* We have just flushed all our outstanding TF work in this job so make
      * sure we don't emit TF flushes again for any of it again.
      }
      static void
   v3d_emit_wait_for_tf_if_needed(struct v3d_context *v3d, struct v3d_job *job)
   {
         if (!job->tf_enabled)
            set_foreach(job->tf_write_prscs, entry) {
            struct pipe_resource *prsc = (struct pipe_resource *)entry->key;
   for (int s = 0; s < PIPE_SHADER_COMPUTE; s++) {
            /* Fragment shaders can only start executing after all
   * binning (and thus TF) is complete.
   *
   * XXX: For VS/GS/TES, if the binning shader does not
   * read the resource then we could also avoid emitting
                              if (v3d_state_reads_resource(v3d, prsc, s)) {
         }
      #if V3D_VERSION >= 41
   static void
   v3d_emit_gs_state_record(struct v3d_job *job,
                           {
         cl_emit(&job->indirect, GEOMETRY_SHADER_STATE_RECORD, shader) {
            shader.geometry_bin_mode_shader_code_address =
               shader.geometry_bin_mode_shader_4_way_threadable =
      #if V3D_VERSION <= 42
         #endif
                                 shader.geometry_render_mode_shader_code_address =
         shader.geometry_render_mode_shader_4_way_threadable =
      #if V3D_VERSION <= 42
         #endif
                     }
      static uint8_t
   v3d_gs_output_primitive(enum mesa_prim prim_type)
   {
      switch (prim_type) {
   case MESA_PRIM_POINTS:
         case MESA_PRIM_LINE_STRIP:
         case MESA_PRIM_TRIANGLE_STRIP:
         default:
            }
      static void
   v3d_emit_tes_gs_common_params(struct v3d_job *job,
               {
         /* This, and v3d_emit_tes_gs_shader_params below, fill in default
      * values for tessellation fields even though we don't support
   * tessellation yet because our packing functions (and the simulator)
   * complain if we don't.
      cl_emit(&job->indirect, TESSELLATION_GEOMETRY_COMMON_PARAMS, shader) {
            shader.tessellation_type = TESSELLATION_TYPE_TRIANGLE;
   shader.tessellation_point_mode = false;
                        shader.geometry_shader_output_format =
      }
      static uint8_t
   simd_width_to_gs_pack_mode(uint32_t width)
   {
      switch (width) {
   case 16:
         case 8:
         case 4:
         case 1:
         default:
            }
      static void
   v3d_emit_tes_gs_shader_params(struct v3d_job *job,
                     {
         cl_emit(&job->indirect, TESSELLATION_GEOMETRY_SHADER_PARAMS, shader) {
            shader.tcs_batch_flush_mode = V3D_TCS_FLUSH_MODE_FULLY_PACKED;
   shader.per_patch_data_column_depth = 1;
   shader.tcs_output_segment_size_in_sectors = 1;
   shader.tcs_output_segment_pack_mode = V3D_PACK_MODE_16_WAY;
   shader.tes_output_segment_size_in_sectors = 1;
   shader.tes_output_segment_pack_mode = V3D_PACK_MODE_16_WAY;
   shader.gs_output_segment_size_in_sectors = gs_vpm_output_size;
   shader.gs_output_segment_pack_mode =
         shader.tbg_max_patches_per_tcs_batch = 1;
   shader.tbg_max_extra_vertex_segs_for_patches_after_first = 0;
   shader.tbg_min_tcs_output_segments_required_in_play = 1;
   shader.tbg_min_per_patch_data_segments_required_in_play = 1;
   shader.tpg_max_patches_per_tes_batch = 1;
   shader.tpg_max_vertex_segments_per_tes_batch = 0;
   shader.tpg_max_tcs_output_segments_per_tes_batch = 1;
   shader.tpg_min_tes_output_segments_required_in_play = 1;
   shader.gbg_max_tes_output_vertex_segments_per_gs_batch =
      }
   #endif
      static void
   v3d_emit_gl_shader_state(struct v3d_context *v3d,
         {
         struct v3d_job *job = v3d->job;
   /* V3D_DIRTY_VTXSTATE */
   struct v3d_vertex_stateobj *vtx = v3d->vtx;
   /* V3D_DIRTY_VTXBUF */
            /* Upload the uniforms to the indirect CL first */
   struct v3d_cl_reloc fs_uniforms =
                  struct v3d_cl_reloc gs_uniforms = { NULL, 0 };
   struct v3d_cl_reloc gs_bin_uniforms = { NULL, 0 };
   if (v3d->prog.gs) {
               }
   if (v3d->prog.gs_bin) {
                        struct v3d_cl_reloc vs_uniforms =
               struct v3d_cl_reloc cs_uniforms =
                  /* Update the cache dirty flag based on the shader progs data */
   job->tmu_dirty_rcl |= v3d->prog.cs->prog_data.vs->base.tmu_dirty_rcl;
   job->tmu_dirty_rcl |= v3d->prog.vs->prog_data.vs->base.tmu_dirty_rcl;
   if (v3d->prog.gs_bin) {
               }
   if (v3d->prog.gs) {
               }
            uint32_t num_elements_to_emit = 0;
   for (int i = 0; i < vtx->num_elements; i++) {
            struct pipe_vertex_element *elem = &vtx->pipe[i];
   struct pipe_vertex_buffer *vb =
                     uint32_t shader_state_record_length =
   #if V3D_VERSION >= 41
         if (v3d->prog.gs) {
            shader_state_record_length +=
            #endif
            /* See GFXH-930 workaround below */
   uint32_t shader_rec_offset =
               v3d_cl_ensure_space(&job->indirect,
                  /* XXX perf: We should move most of the SHADER_STATE_RECORD setup to
      * compile time, so that we mostly just have to OR the VS and FS
                        assert(v3d->screen->devinfo.ver >= 41 || !v3d->prog.gs);
   v3d_compute_vpm_config(&v3d->screen->devinfo,
                           v3d->prog.cs->prog_data.vs,
            #if V3D_VERSION >= 41
                                       struct v3d_gs_prog_data *gs = v3d->prog.gs->prog_data.gs;
                        /* Bin Tes/Gs params */
   v3d_emit_tes_gs_shader_params(v3d->job,
                        /* Render Tes/Gs params */
   v3d_emit_tes_gs_shader_params(v3d->job,
      #else
         #endif
                  cl_emit(&job->indirect, GL_SHADER_STATE_RECORD, shader) {
            shader.enable_clipping = true;
   /* V3D_DIRTY_PRIM_MODE | V3D_DIRTY_RASTERIZER */
                        /* Must be set if the shader modifies Z, discards, or modifies
   * the sample mask.  For any of these cases, the fragment
   * shader needs to write the Z value (even just discards).
                        /* Set if the EZ test must be disabled (due to shader side
   * effects and the early_z flag not being present in the
   * shader).
                        #if V3D_VERSION >= 41
                  shader.any_shader_reads_hardware_written_primitive_id =
            #endif
      #if V3D_VERSION >= 40
                  shader.do_scoreboard_wait_on_first_thread_switch =
      #endif
                                    shader.coordinate_shader_code_address =
               shader.vertex_shader_code_address =
                     #if V3D_VERSION <= 42
                                       /* XXX: Use combined input/output size flag in the common
   * case.
   */
   shader.coordinate_shader_has_separate_input_and_output_vpm_blocks =
         shader.vertex_shader_has_separate_input_and_output_vpm_blocks =
         shader.coordinate_shader_input_vpm_segment_size =
                  #endif
                  /* On V3D 7.1 there isn't a specific flag to set if we are using
   * shared/separate segments or not. We just set the value of
      #if V3D_VERSION == 71
                  shader.coordinate_shader_input_vpm_segment_size =
      #endif
                     shader.coordinate_shader_output_vpm_segment_size =
                              #if V3D_VERSION >= 41
                  shader.min_coord_shader_input_segments_required_in_play =
                        shader.min_coord_shader_output_segments_required_in_play_in_addition_to_vcm_cache_size =
                        shader.coordinate_shader_4_way_threadable =
         shader.vertex_shader_4_way_threadable =
                        shader.coordinate_shader_start_in_final_thread_section =
         shader.vertex_shader_start_in_final_thread_section =
      #else
                  shader.coordinate_shader_4_way_threadable =
         shader.coordinate_shader_2_way_threadable =
         shader.vertex_shader_4_way_threadable =
         shader.vertex_shader_2_way_threadable =
         shader.fragment_shader_4_way_threadable =
      #endif
                     shader.vertex_id_read_by_coordinate_shader =
         shader.instance_id_read_by_coordinate_shader =
         shader.vertex_id_read_by_vertex_shader =
         #if V3D_VERSION <= 42
                     #endif
                  bool cs_loaded_any = false;
   for (int i = 0; i < vtx->num_elements; i++) {
            struct pipe_vertex_element *elem = &vtx->pipe[i];
                                       enum { size = cl_packet_length(GL_SHADER_STATE_ATTRIBUTE_RECORD) };
   cl_emit_with_prepacked(&job->indirect,
                        attr.stride = elem->src_stride;
   attr.address = cl_address(rsc->bo,
                                          /* GFXH-930: At least one attribute must be enabled
   * and read by CS and VS.  If we have attributes being
   * consumed by the VS but not the CS, then set up a
   * dummy load of the last attribute into the CS's VPM
   * inputs.  (Since CS is just dead-code-elimination
   * compared to VS, we can't have CS loading but not
   * VS).
   */
   if (v3d->prog.cs->prog_data.vs->vattr_sizes[i])
   #if V3D_VERSION >= 41
         #endif
                              if (num_elements_to_emit == 0) {
            /* GFXH-930: At least one attribute must be enabled and read
   * by CS and VS.  If we have no attributes being consumed by
   * the shader, set up a dummy to be loaded into the VPM.
   */
                                                                     cl_emit(&job->bcl, VCM_CACHE_SIZE, vcm) {
                  #if V3D_VERSION >= 41
         if (v3d->prog.gs) {
            cl_emit(&job->bcl, GL_SHADER_STATE_INCLUDING_GS, state) {
            state.address = cl_address(job->indirect.bo,
   } else {
            cl_emit(&job->bcl, GL_SHADER_STATE, state) {
            state.address = cl_address(job->indirect.bo,
   #else
         assert(!v3d->prog.gs);
   cl_emit(&job->bcl, GL_SHADER_STATE, state) {
               #endif
            v3d_bo_unreference(&cs_uniforms.bo);
   v3d_bo_unreference(&vs_uniforms.bo);
   if (gs_uniforms.bo)
         if (gs_bin_uniforms.bo)
         }
      /**
   * Updates the number of primitives generated from the number of vertices
   * to draw. This only works when no GS is present, since otherwise the number
   * of primitives generated cannot be determined in advance and we need to
   * use the PRIMITIVE_COUNTS_FEEDBACK command instead, however, that requires
   * a sync wait for the draw to complete, so we only use that when GS is present.
   */
   static void
   v3d_update_primitives_generated_counter(struct v3d_context *v3d,
               {
                  if (!v3d->active_queries)
            uint32_t prims = u_prims_for_vertices(info->mode, draw->count);
   }
      static void
   v3d_update_job_ez(struct v3d_context *v3d, struct v3d_job *job)
   {
         /* If first_ez_state is V3D_EZ_DISABLED it means that we have already
      * determined that we should disable EZ completely for all draw calls
   * in this job. This will cause us to disable EZ for the entire job in
   * the Tile Rendering Mode RCL packet and when we do that we need to
   * make sure we never emit a draw call in the job with EZ enabled in
   * the CFG_BITS packet, so ez_state must also be V3D_EZ_DISABLED.
      if (job->first_ez_state == V3D_EZ_DISABLED) {
                        /* If this is the first time we update EZ state for this job we first
      * check if there is anything that requires disabling it completely
   * for the entire job (based on state that is not related to the
   * current draw call and pipeline state).
      if (!job->decided_global_ez_enable) {
                     if (!job->zsbuf) {
                              /* GFXH-1918: the early-Z buffer may load incorrect depth
   * values if the frame has odd width or height. Disable early-Z
   * in this case.
   */
   bool needs_depth_load = v3d->zsa && job->zsbuf &&
               if (needs_depth_load &&
         ((job->draw_width % 2 != 0) || (job->draw_height % 2 != 0))) {
      perf_debug("Loading depth buffer for framebuffer with odd width "
         job->first_ez_state = V3D_EZ_DISABLED;
            switch (v3d->zsa->ez_state) {
   case V3D_EZ_UNDECIDED:
            /* If the Z/S state didn't pick a direction but didn't
   * disable, then go along with the current EZ state.  This
               case V3D_EZ_LT_LE:
   case V3D_EZ_GT_GE:
            /* If the Z/S state picked a direction, then it needs to match
   * the current direction if we've decided on one.
   */
   if (job->ez_state == V3D_EZ_UNDECIDED)
                     case V3D_EZ_DISABLED:
            /* If the current Z/S state disables EZ because of a bad Z
   * func or stencil operation, then we can't do any more EZ in
   * this frame.
   */
               /* If the FS affects the Z of the pixels, then it may update against
      * the chosen EZ direction (though we could use
   * ARB_conservative_depth's hints to avoid this)
      if (v3d->prog.fs->prog_data.fs->writes_z &&
         !v3d->prog.fs->prog_data.fs->writes_z_from_fep) {
            if (job->first_ez_state == V3D_EZ_UNDECIDED &&
         }
      static bool
   v3d_check_compiled_shaders(struct v3d_context *v3d)
   {
                  uint32_t failed_stage = MESA_SHADER_NONE;
   if (!v3d->prog.vs->resource || !v3d->prog.cs->resource) {
         } else if ((v3d->prog.gs_bin && !v3d->prog.gs_bin->resource) ||
               } else if (v3d->prog.fs && !v3d->prog.fs->resource) {
                  if (likely(failed_stage == MESA_SHADER_NONE))
            if (!warned[failed_stage]) {
            fprintf(stderr,
            }
   }
      static void
   v3d_draw_vbo(struct pipe_context *pctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
         if (num_draws > 1) {
                        if (!indirect && (!draws[0].count || !info->instance_count))
                     if (!indirect &&
         !info->primitive_restart &&
            if (!v3d_render_condition_check(v3d))
            /* Fall back for weird desktop GL primitive restart values. */
   if (info->primitive_restart &&
         info->index_size) {
      uint32_t mask = util_prim_restart_index_from_size(info->index_size);
   if (info->restart_index != mask) {
                     /* Before setting up the draw, flush anything writing to the resources
      * that we read from or reading from resources we write to.
      for (int s = 0; s < PIPE_SHADER_COMPUTE; s++)
            if (indirect && indirect->buffer) {
                                 /* If transform feedback is active and we are switching primitive type
      * we need to submit the job before drawing and update the vertex count
   * written to TF based on the primitive type since we will need to
   * know the exact vertex count if the application decides to call
   * glDrawTransformFeedback() later.
      if (v3d->streamout.num_targets > 0 &&
         u_base_prim_type(info->mode) != u_base_prim_type(v3d->prim_mode)) {
                     /* If vertex texturing depends on the output of rendering, we need to
      * ensure that that rendering is complete before we run a coordinate
   * shader that depends on it.
   *
   * Given that doing that is unusual, for now we just block the binner
   * on the last submitted render, rather than tracking the last
   * rendering to each texture's BO.
      if (v3d->tex[PIPE_SHADER_VERTEX].num_textures || (indirect && indirect->buffer)) {
            static bool warned = false;
   if (!warned) {
            perf_debug("Blocking binner on last render due to "
                  /* We also need to ensure that compute is complete when render depends
      * on resources written by it.
      if (v3d->sync_on_last_compute_job) {
                        /* Mark SSBOs and images as being written.  We don't actually know
      * which ones are read vs written, so just assume the worst.
      for (int s = 0; s < PIPE_SHADER_COMPUTE; s++) {
            u_foreach_bit(i, v3d->ssbo[s].enabled_mask) {
                              u_foreach_bit(i, v3d->shaderimg[s].enabled_mask) {
            v3d_job_add_write_resource(job,
            /* Get space to emit our draw call into the BCL, using a branch to
      * jump to a new BO if necessary.
               if (v3d->prim_mode != info->mode) {
                        v3d_start_draw(v3d);
   v3d_update_compiled_shaders(v3d, info->mode);
   if (!v3d_check_compiled_shaders(v3d))
                  /* If this job was writing to transform feedback buffers before this
      * draw and we are reading from them here, then we need to wait for TF
   * to complete before we emit this draw.
   *
   * Notice this check needs to happen before we emit state for the
   * current draw call, where we update job->tf_enabled, so we can ensure
   * that we only check TF writes for prior draws.
                        if (v3d->dirty & (V3D_DIRTY_VTXBUF |
                     V3D_DIRTY_VTXSTATE |
   V3D_DIRTY_PRIM_MODE |
   V3D_DIRTY_RASTERIZER |
   V3D_DIRTY_COMPILED_CS |
   V3D_DIRTY_COMPILED_VS |
   V3D_DIRTY_COMPILED_GS_BIN |
   V3D_DIRTY_COMPILED_GS |
   V3D_DIRTY_COMPILED_FS |
   v3d->prog.cs->uniform_dirty_bits |
   v3d->prog.vs->uniform_dirty_bits |
   (v3d->prog.gs_bin ?
         (v3d->prog.gs ?
                     /* The Base Vertex/Base Instance packet sets those values to nonzero
      * for the next draw call only.
      if ((info->index_size && draws->index_bias) || info->start_instance) {
            cl_emit(&job->bcl, BASE_VERTEX_BASE_INSTANCE, base) {
                     #if V3D_VERSION < 40
         /* V3D 3.x: The HW only processes transform feedback on primitives
      * with the flag set.
      if (v3d->streamout.num_targets)
   #endif
                     if (!v3d->prog.gs && !v3d->prim_restart)
            uint32_t hw_prim_type = v3d_hw_prim_type(info->mode);
   if (info->index_size) {
            uint32_t index_size = info->index_size;
   uint32_t offset = draws[0].start * index_size;
   struct pipe_resource *prsc;
   if (info->has_user_indices) {
            unsigned start_offset = draws[0].start * info->index_size;
   prsc = NULL;
   u_upload_data(v3d->uploader, start_offset,
            } else {
         #if V3D_VERSION >= 40
                  cl_emit(&job->bcl, INDEX_BUFFER_SETUP, ib) {
      #endif
                        #if V3D_VERSION < 40
               #endif /* V3D_VERSION < 40 */
                                                         prim.stride_in_multiples_of_4_bytes = indirect->stride >> 2;
         #if V3D_VERSION >= 40
         #else /* V3D_VERSION < 40 */
                     #endif /* V3D_VERSION < 40 */
                                                   } else {
      #if V3D_VERSION >= 40
         #else /* V3D_VERSION < 40 */
                     #endif /* V3D_VERSION < 40 */
                                                } else {
            if (indirect && indirect->buffer) {
                                       prim.stride_in_multiples_of_4_bytes = indirect->stride >> 2;
      } else if (info->instance_count > 1) {
            struct pipe_stream_output_target *so =
         indirect && indirect->count_from_stream_output ?
   uint32_t vert_count = so ?
         v3d_stream_output_target_get_vertex_count(so) :
   cl_emit(&job->bcl, VERTEX_ARRAY_INSTANCED_PRIMS, prim) {
         prim.mode = hw_prim_type | prim_tf_enable;
   prim.index_of_first_vertex = draws[0].start;
      } else {
            struct pipe_stream_output_target *so =
         indirect && indirect->count_from_stream_output ?
   uint32_t vert_count = so ?
         v3d_stream_output_target_get_vertex_count(so) :
   cl_emit(&job->bcl, VERTEX_ARRAY_PRIMS, prim) {
         prim.mode = hw_prim_type | prim_tf_enable;
            /* A flush is required in between a TF draw and any following TF specs
      * packet, or the GPU may hang.  Just flush each time for now.
      if (v3d->streamout.num_targets)
            job->draw_calls_queued++;
   if (v3d->streamout.num_targets)
            /* Increment the TF offsets by how many verts we wrote.  XXX: This
      * needs some clamping to the buffer size.
   *
   * If primitive restart is enabled or we have a geometry shader, we
   * update it later, when we can query the device to know how many
   * vertices were written.
      if (!v3d->prog.gs && !v3d->prim_restart) {
            for (int i = 0; i < v3d->streamout.num_targets; i++)
               if (v3d->zsa && job->zsbuf && v3d->zsa->base.depth_enabled) {
                           job->load |= PIPE_CLEAR_DEPTH & ~job->clear;
   if (v3d->zsa->base.depth_writemask)
               if (v3d->zsa && job->zsbuf && v3d->zsa->base.stencil[0].enabled) {
                                          job->load |= PIPE_CLEAR_STENCIL & ~job->clear;
   if (v3d->zsa->base.stencil[0].writemask ||
      v3d->zsa->base.stencil[1].writemask) {
                  for (int i = 0; i < job->nr_cbufs; i++) {
                                                job->load |= bit & ~job->clear;
   if (v3d->blend->base.rt[blend_rt].colormask)
               if (job->referenced_size > 768 * 1024 * 1024) {
            perf_debug("Flushing job with %dkb to try to free up memory\n",
               if (V3D_DBG(ALWAYS_FLUSH))
   }
      #if V3D_VERSION >= 41
   #define V3D_CSD_CFG012_WG_COUNT_SHIFT 16
   #define V3D_CSD_CFG012_WG_OFFSET_SHIFT 0
   /* Allow this dispatch to start while the last one is still running. */
   #define V3D_CSD_CFG3_OVERLAP_WITH_PREV (1 << 26)
   /* Maximum supergroup ID.  6 bits. */
   #define V3D_CSD_CFG3_MAX_SG_ID_SHIFT 20
   /* Batches per supergroup minus 1.  8 bits. */
   #define V3D_CSD_CFG3_BATCHES_PER_SG_M1_SHIFT 12
   /* Workgroups per supergroup, 0 means 16 */
   #define V3D_CSD_CFG3_WGS_PER_SG_SHIFT 8
   #define V3D_CSD_CFG3_WG_SIZE_SHIFT 0
      #define V3D_CSD_CFG5_PROPAGATE_NANS (1 << 2)
   #define V3D_CSD_CFG5_SINGLE_SEG (1 << 1)
   #define V3D_CSD_CFG5_THREADING (1 << 0)
      static void
   v3d_launch_grid(struct pipe_context *pctx, const struct pipe_grid_info *info)
   {
         struct v3d_context *v3d = v3d_context(pctx);
                              if (!v3d->prog.compute->resource) {
            static bool warned = false;
   if (!warned) {
            fprintf(stderr,
                        /* Some of the units of scale:
      *
   * - Batches of 16 work items (shader invocations) that will be queued
   *   to the run on a QPU at once.
   *
   * - Workgroups composed of work items based on the shader's layout
   *   declaration.
   *
   * - Supergroups of 1-16 workgroups.  There can only be 16 supergroups
   *   running at a time on the core, so we want to keep them large to
   *   keep the QPUs busy, but a whole supergroup will sync at a barrier
   *   so we want to keep them small if one is present.
      struct drm_v3d_submit_csd submit = { 0 };
            /* Set up the actual number of workgroups, synchronously mapping the
      * indirect buffer if necessary to get the dimensions.
      if (info->indirect) {
            struct pipe_transfer *transfer;
   uint32_t *map = pipe_buffer_map_range(pctx, info->indirect,
                                          if (v3d->compute_num_workgroups[0] == 0 ||
      v3d->compute_num_workgroups[1] == 0 ||
   v3d->compute_num_workgroups[2] == 0) {
         /* Nothing to dispatch, so skip the draw (CSD can't
   * handle 0 workgroups).
   } else {
            v3d->compute_num_workgroups[0] = info->grid[0];
               uint32_t num_wgs = 1;
   for (int i = 0; i < 3; i++) {
            num_wgs *= v3d->compute_num_workgroups[i];
                        struct v3d_compute_prog_data *compute =
         uint32_t wgs_per_sg =
            v3d_csd_choose_workgroups_per_supergroup(
            &v3d->screen->devinfo,
            uint32_t batches_per_sg = DIV_ROUND_UP(wgs_per_sg * wg_size, 16);
   uint32_t whole_sgs = num_wgs / wgs_per_sg;
   uint32_t rem_wgs = num_wgs - whole_sgs * wgs_per_sg;
   uint32_t num_batches = batches_per_sg * whole_sgs +
            submit.cfg[3] |= (wgs_per_sg & 0xf) << V3D_CSD_CFG3_WGS_PER_SG_SHIFT;
   submit.cfg[3] |=
                     /* Number of batches the dispatch will invoke.
      * V3D 7.1.6 and later don't subtract 1 from the number of batches
      if (v3d->screen->devinfo.ver < 71 ||
         (v3d->screen->devinfo.ver == 71 && v3d->screen->devinfo.rev < 6)) {
   } else {
                  /* Make sure we didn't accidentally underflow. */
            v3d_job_add_bo(job, v3d_resource(v3d->prog.compute->resource)->bo);
   submit.cfg[5] = (v3d_resource(v3d->prog.compute->resource)->bo->offset +
         if (v3d->screen->devinfo.ver < 71)
         if (v3d->prog.compute->prog_data.base->single_seg)
         if (v3d->prog.compute->prog_data.base->threads == 4)
            if (v3d->prog.compute->prog_data.compute->shared_size) {
            v3d->compute_shared_memory =
            v3d_bo_alloc(v3d->screen,
            struct v3d_cl_reloc uniforms = v3d_write_uniforms(v3d, job,
               v3d_job_add_bo(job, uniforms.bo);
            /* Pull some job state that was stored in a SUBMIT_CL struct out to
      * our SUBMIT_CSD struct
      submit.bo_handles = job->submit.bo_handles;
            /* Serialize this in the rest of our command stream. */
   submit.in_sync = v3d->out_sync;
            if (v3d->active_perfmon) {
                                 if (!V3D_DBG(NORAST)) {
            int ret = v3d_ioctl(screen->fd, DRM_IOCTL_V3D_SUBMIT_CSD,
         static bool warned = false;
   if (ret && !warned) {
            fprintf(stderr, "CSD submit call returned %s.  "
      } else if (!ret) {
                              /* Mark SSBOs as being written.. we don't actually know which ones are
      * read vs written, so just assume the worst
      u_foreach_bit(i, v3d->ssbo[PIPE_SHADER_COMPUTE].enabled_mask) {
            struct v3d_resource *rsc = v3d_resource(
                     u_foreach_bit(i, v3d->shaderimg[PIPE_SHADER_COMPUTE].enabled_mask) {
            struct v3d_resource *rsc = v3d_resource(
                     v3d_bo_unreference(&uniforms.bo);
   }
   #endif
      /**
   * Implements gallium's clear() hook (glClear()) by drawing a pair of triangles.
   */
   static void
   v3d_draw_clear(struct v3d_context *v3d,
                     {
         v3d_blitter_save(v3d, false, true);
   util_blitter_clear(v3d->blitter,
                           }
      /**
   * Attempts to perform the GL clear by using the TLB's fast clear at the start
   * of the frame.
   */
   static unsigned
   v3d_tlb_clear(struct v3d_job *job, unsigned buffers,
               {
                  if (job->draw_calls_queued) {
            /* If anything in the CL has drawn using the buffer, then the
   * TLB clear we're trying to add now would happen before that
   * drawing.
               /* GFXH-1461: If we were to emit a load of just depth or just stencil,
      * then the clear for the other may get lost.  We need to decide now
   * if it would be possible to need to emit a load of just one after
   * we've set up our TLB clears. This issue is fixed since V3D 4.3.18.
      if (v3d->screen->devinfo.ver <= 42 &&
         buffers & PIPE_CLEAR_DEPTHSTENCIL &&
   (buffers & PIPE_CLEAR_DEPTHSTENCIL) != PIPE_CLEAR_DEPTHSTENCIL &&
   job->zsbuf &&
   util_format_is_depth_and_stencil(job->zsbuf->texture->format)) {
            for (int i = 0; i < job->nr_cbufs; i++) {
                                                                     /*  While hardware supports clamping, this is not applied on
   *  the clear values, so we need to do it manually.
   *
   *  "Clamping is performed on color values immediately as they
   *   enter the TLB and after blending. Clamping is not
   *   performed on the clear color."
                        if (v3d->swap_color_rb & (1 << i)) {
            union pipe_color_union orig_color = clamped_color;
   clamped_color.f[0] = orig_color.f[2];
                                    switch (surf->internal_type) {
   case V3D_INTERNAL_TYPE_8:
            util_pack_color(clamped_color.f, PIPE_FORMAT_R8G8B8A8_UNORM,
            case V3D_INTERNAL_TYPE_8I:
   case V3D_INTERNAL_TYPE_8UI:
            job->clear_color[i][0] = ((clamped_color.ui[0] & 0xff) |
                  case V3D_INTERNAL_TYPE_16F:
            util_pack_color(clamped_color.f, PIPE_FORMAT_R16G16B16A16_FLOAT,
            case V3D_INTERNAL_TYPE_16I:
   case V3D_INTERNAL_TYPE_16UI:
            job->clear_color[i][0] = ((clamped_color.ui[0] & 0xffff) |
         job->clear_color[i][1] = ((clamped_color.ui[2] & 0xffff) |
      case V3D_INTERNAL_TYPE_32F:
   case V3D_INTERNAL_TYPE_32I:
   case V3D_INTERNAL_TYPE_32UI:
                              unsigned zsclear = buffers & PIPE_CLEAR_DEPTHSTENCIL;
   if (zsclear) {
                           if (zsclear & PIPE_CLEAR_DEPTH)
                              job->draw_min_x = 0;
   job->draw_min_y = 0;
   job->draw_max_x = v3d->framebuffer.width;
   job->draw_max_y = v3d->framebuffer.height;
   job->clear |= buffers;
   job->store |= buffers;
                     }
      static void
   v3d_clear(struct pipe_context *pctx, unsigned buffers, const struct pipe_scissor_state *scissor_state,
         {
         struct v3d_context *v3d = v3d_context(pctx);
                     if (!buffers || !v3d_render_condition_check(v3d))
            }
      static void
   v3d_clear_render_target(struct pipe_context *pctx, struct pipe_surface *ps,
                     {
                  if (render_condition_enabled && !v3d_render_condition_check(v3d))
            v3d_blitter_save(v3d, false, render_condition_enabled);
   }
      static void
   v3d_clear_depth_stencil(struct pipe_context *pctx, struct pipe_surface *ps,
                     {
                  if (render_condition_enabled && !v3d_render_condition_check(v3d))
            v3d_blitter_save(v3d, false, render_condition_enabled);
   util_blitter_clear_depth_stencil(v3d->blitter, ps, buffers, depth,
   }
      void
   v3dX(draw_init)(struct pipe_context *pctx)
   {
         pctx->draw_vbo = v3d_draw_vbo;
   pctx->clear = v3d_clear;
   pctx->clear_render_target = v3d_clear_render_target;
   #if V3D_VERSION >= 41
         if (v3d_context(pctx)->screen->has_csd)
   #endif
   }
