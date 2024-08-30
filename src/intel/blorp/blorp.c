   /*
   * Copyright © 2012 Intel Corporation
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
      #include <errno.h>
      #include "blorp_priv.h"
   #include "compiler/brw_compiler.h"
   #include "compiler/brw_nir.h"
   #include "dev/intel_debug.h"
      enum intel_measure_snapshot_type
   blorp_op_to_intel_measure_snapshot(enum blorp_op op)
   {
         #define MAP(name) [BLORP_OP_##name] = INTEL_SNAPSHOT_##name
         MAP(BLIT),
   MAP(COPY),
   MAP(CCS_AMBIGUATE),
   MAP(CCS_COLOR_CLEAR),
   MAP(CCS_PARTIAL_RESOLVE),
   MAP(CCS_RESOLVE),
   MAP(HIZ_AMBIGUATE),
   MAP(HIZ_CLEAR),
   MAP(HIZ_RESOLVE),
   MAP(MCS_AMBIGUATE),
   MAP(MCS_COLOR_CLEAR),
   MAP(MCS_PARTIAL_RESOLVE),
   MAP(SLOW_COLOR_CLEAR),
   #undef MAP
      };
               }
      const char *blorp_op_to_name(enum blorp_op op)
   {
         #define MAP(name) [BLORP_OP_##name] = #name
         MAP(BLIT),
   MAP(COPY),
   MAP(CCS_AMBIGUATE),
   MAP(CCS_COLOR_CLEAR),
   MAP(CCS_PARTIAL_RESOLVE),
   MAP(CCS_RESOLVE),
   MAP(HIZ_AMBIGUATE),
   MAP(HIZ_CLEAR),
   MAP(HIZ_RESOLVE),
   MAP(MCS_AMBIGUATE),
   MAP(MCS_COLOR_CLEAR),
   MAP(MCS_PARTIAL_RESOLVE),
   MAP(SLOW_COLOR_CLEAR),
   #undef MAP
      };
               }
      const char *
   blorp_shader_type_to_name(enum blorp_shader_type type)
   {
      static const char *shader_name[] = {
      [BLORP_SHADER_TYPE_COPY]                = "BLORP-copy",
   [BLORP_SHADER_TYPE_BLIT]                = "BLORP-blit",
   [BLORP_SHADER_TYPE_CLEAR]               = "BLORP-clear",
   [BLORP_SHADER_TYPE_MCS_PARTIAL_RESOLVE] = "BLORP-mcs-partial-resolve",
   [BLORP_SHADER_TYPE_LAYER_OFFSET_VS]     = "BLORP-layer-offset-vs",
      };
               }
      const char *
   blorp_shader_pipeline_to_name(enum blorp_shader_pipeline pipe)
   {
      static const char *pipeline_name[] = {
      [BLORP_SHADER_PIPELINE_RENDER]  = "render",
      };
               }
      void
   blorp_init(struct blorp_context *blorp, void *driver_ctx,
         {
               blorp->driver_ctx = driver_ctx;
   blorp->isl_dev = isl_dev;
   if (config)
      }
      void
   blorp_finish(struct blorp_context *blorp)
   {
         }
      void
   blorp_batch_init(struct blorp_context *blorp,
               {
      batch->blorp = blorp;
   batch->driver_batch = driver_batch;
      }
      void
   blorp_batch_finish(struct blorp_batch *batch)
   {
         }
      void
   brw_blorp_surface_info_init(struct blorp_batch *batch,
                           {
      struct blorp_context *blorp = batch->blorp;
   memset(info, 0, sizeof(*info));
   assert(level < surf->surf->levels);
   assert(layer < MAX2(surf->surf->logical_level0_px.depth >> level,
                     if (format == ISL_FORMAT_UNSUPPORTED)
            info->surf = *surf->surf;
            info->aux_usage = surf->aux_usage;
   if (info->aux_usage != ISL_AUX_USAGE_NONE) {
      info->aux_surf = *surf->aux_surf;
               info->clear_color = surf->clear_color;
            isl_surf_usage_flags_t view_usage;
   if (is_dest) {
      if (batch->flags & BLORP_BATCH_USE_COMPUTE)
         else
      } else {
                  info->view = (struct isl_view) {
      .usage = view_usage,
   .format = format,
   .base_level = level,
   .levels = 1,
               info->view.array_len =
      MAX2(u_minify(info->surf.logical_level0_px.depth, level),
         if (!is_dest &&
      (info->surf.dim == ISL_SURF_DIM_3D ||
   info->surf.msaa_layout == ISL_MSAA_LAYOUT_ARRAY)) {
   /* 3-D textures don't support base_array layer and neither do 2-D
   * multisampled textures on IVB so we need to pass it through the
   * sampler in those cases.  These are also two cases where we are
   * guaranteed that we won't be doing any funny surface hacks.
   */
   info->view.base_array_layer = 0;
      } else {
               assert(info->view.array_len >= info->view.base_array_layer);
   info->view.array_len -= info->view.base_array_layer;
               /* Sandy Bridge and earlier have a limit of a maximum of 512 layers for
   * layered rendering.
   */
   if (is_dest && blorp->isl_dev->info->ver <= 6)
            if (surf->tile_x_sa || surf->tile_y_sa) {
      /* This is only allowed on simple 2D surfaces without MSAA */
   assert(info->surf.dim == ISL_SURF_DIM_2D);
   assert(info->surf.samples == 1);
   assert(info->surf.levels == 1);
   assert(info->surf.logical_level0_px.array_len == 1);
            info->tile_x_sa = surf->tile_x_sa;
            /* Instead of using the X/Y Offset fields in RENDER_SURFACE_STATE, we
   * place the image at the tile boundary and offset our sampling or
   * rendering.  For this reason, we need to grow the image by the offset
   * to ensure that the hardware doesn't think we've gone past the edge.
   */
   info->surf.logical_level0_px.w += surf->tile_x_sa;
   info->surf.logical_level0_px.h += surf->tile_y_sa;
   info->surf.phys_level0_sa.w += surf->tile_x_sa;
         }
         void
   blorp_params_init(struct blorp_params *params)
   {
      memset(params, 0, sizeof(*params));
   params->num_samples = 1;
   params->num_draw_buffers = 1;
      }
      void
   brw_blorp_init_wm_prog_key(struct brw_wm_prog_key *wm_key)
   {
      memset(wm_key, 0, sizeof(*wm_key));
      }
      void
   brw_blorp_init_cs_prog_key(struct brw_cs_prog_key *cs_key)
   {
         }
      const unsigned *
   blorp_compile_fs(struct blorp_context *blorp, void *mem_ctx,
                  struct nir_shader *nir,
      {
                                 wm_prog_data->base.nr_params = 0;
            struct brw_nir_compiler_opts opts = {};
   brw_preprocess_nir(compiler, nir, &opts);
   nir_remove_dead_variables(nir, nir_var_shader_in, NULL);
            if (blorp->compiler->devinfo->ver < 6) {
      if (nir->info.fs.uses_discard)
                        struct brw_compile_fs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
   .log_data = blorp->driver_ctx,
      },
   .key = wm_key,
                           }
      const unsigned *
   blorp_compile_vs(struct blorp_context *blorp, void *mem_ctx,
               {
                        struct brw_nir_compiler_opts opts = {};
   brw_preprocess_nir(compiler, nir, &opts);
                     brw_compute_vue_map(compiler->devinfo,
                                       struct brw_compile_vs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
   .log_data = blorp->driver_ctx,
      },
   .key = &vs_key,
                  }
      static bool
   lower_base_workgroup_id(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (intrin->intrinsic != nir_intrinsic_load_base_workgroup_id)
            b->cursor = nir_instr_remove(&intrin->instr);
   nir_def_rewrite_uses(&intrin->def, nir_imm_zero(b, 3, 32));
      }
      const unsigned *
   blorp_compile_cs(struct blorp_context *blorp, void *mem_ctx,
                     {
                                 struct brw_nir_compiler_opts opts = {};
   brw_preprocess_nir(compiler, nir, &opts);
            NIR_PASS_V(nir, nir_lower_io, nir_var_uniform, type_size_scalar_bytes,
            STATIC_ASSERT(offsetof(struct brw_blorp_wm_inputs, subgroup_id) + 4 ==
         nir->num_uniforms = offsetof(struct brw_blorp_wm_inputs, subgroup_id);
   unsigned nr_params = nir->num_uniforms / 4;
   cs_prog_data->base.nr_params = nr_params;
            NIR_PASS_V(nir, brw_nir_lower_cs_intrinsics);
   NIR_PASS_V(nir, nir_shader_intrinsics_pass, lower_base_workgroup_id,
            struct brw_compile_cs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
   .log_data = blorp->driver_ctx,
      },
   .key = cs_key,
                        ralloc_free(cs_prog_data->base.param);
               }
      struct blorp_sf_key {
      struct brw_blorp_base_key base;
      };
      bool
   blorp_ensure_sf_program(struct blorp_batch *batch,
         {
      struct blorp_context *blorp = batch->blorp;
   const struct brw_wm_prog_data *wm_prog_data = params->wm_prog_data;
            /* Gfx6+ doesn't need a strips and fans program */
   if (blorp->compiler->devinfo->ver >= 6)
            struct blorp_sf_key key = {
                  /* Everything gets compacted in vertex setup, so we just need a
   * pass-through for the correct number of input varyings.
   */
   const uint64_t slots_valid = VARYING_BIT_POS |
            key.key.attrs = slots_valid;
   key.key.primitive = BRW_SF_PRIM_TRIANGLES;
            STATIC_ASSERT(sizeof(key.key.interp_mode) ==
         memcpy(key.key.interp_mode, wm_prog_data->interp_mode,
            if (blorp->lookup_shader(batch, &key, sizeof(key),
                           const unsigned *program;
            struct brw_vue_map vue_map;
            struct brw_sf_prog_data prog_data_tmp;
   program = brw_compile_sf(blorp->compiler, mem_ctx, &key.key,
            bool result =
      blorp->upload_shader(batch, MESA_SHADER_NONE,
                                 }
      void
   blorp_hiz_op(struct blorp_batch *batch, struct blorp_surf *surf,
               {
               struct blorp_params params;
            params.hiz_op = op;
   params.full_surface_hiz_op = true;
   switch (op) {
   case ISL_AUX_OP_FULL_RESOLVE:
      params.op = BLORP_OP_HIZ_RESOLVE;
      case ISL_AUX_OP_AMBIGUATE:
      params.op = BLORP_OP_HIZ_AMBIGUATE;
      case ISL_AUX_OP_FAST_CLEAR:
      params.op = BLORP_OP_HIZ_CLEAR;
      case ISL_AUX_OP_PARTIAL_RESOLVE:
   case ISL_AUX_OP_NONE:
                  for (uint32_t a = 0; a < num_layers; a++) {
               brw_blorp_surface_info_init(batch, &params.depth, surf, level,
            /* Align the rectangle primitive to 8x4 pixels.
   *
   * During fast depth clears, the emitted rectangle primitive  must be
   * aligned to 8x4 pixels.  From the Ivybridge PRM, Vol 2 Part 1 Section
   * 11.5.3.1 Depth Buffer Clear (and the matching section in the
   * Sandybridge PRM):
   *
   *     If Number of Multisamples is NUMSAMPLES_1, the rectangle must be
   *     aligned to an 8x4 pixel block relative to the upper left corner
   *     of the depth buffer [...]
   *
   * For hiz resolves, the rectangle must also be 8x4 aligned. Item
   * WaHizAmbiguate8x4Aligned from the Haswell workarounds page and the
   * Ivybridge simulator require the alignment.
   *
   * To be safe, let's just align the rect for all hiz operations and all
   * hardware generations.
   *
   * However, for some miptree slices of a Z24 texture, emitting an 8x4
   * aligned rectangle that covers the slice may clobber adjacent slices
   * if we strictly adhered to the texture alignments specified in the
   * PRM.  The Ivybridge PRM, Section "Alignment Unit Size", states that
   * SURFACE_STATE.Surface_Horizontal_Alignment should be 4 for Z24
   * surfaces, not 8. But commit 1f112cc increased the alignment from 4 to
   * 8, which prevents the clobbering.
   */
   params.x1 = u_minify(params.depth.surf.logical_level0_px.width,
         params.y1 = u_minify(params.depth.surf.logical_level0_px.height,
         params.x1 = ALIGN(params.x1, 8);
            if (params.depth.view.base_level == 0) {
      /* TODO: What about MSAA? */
   params.depth.surf.logical_level0_px.width = params.x1;
      } else if (devinfo->ver >= 8 && devinfo->ver <= 9 &&
            /* On some platforms, it's not enough to just adjust the clear
   * rectangle when the LOD is greater than 0.
   *
   * From the BDW and SKL PRMs, Vol 7, "Optimized Hierarchical Depth
   * Buffer Resolve":
   *
   *    The following is required when performing a hierarchical depth
   *    buffer resolve:
   *
   *    - A rectangle primitive covering the full render target must be
   *      programmed on Xmin, Ymin, Xmax, and Ymax in the
   *      3DSTATE_WM_HZ_OP command.
   *
   *    - The rectangle primitive size must be aligned to 8x4 pixels.
   *
   * And from the Clear Rectangle programming note in 3DSTATE_WM_HZ_OP
   * (Vol 2a):
   *
   *    Hence the max values must be less than or equal to: ( Surface
   *    Width » LOD ) and ( Surface Height » LOD ) for X Max and Y Max
   *    respectively.
   *
   * This means that the extent of the LOD must be naturally
   * 8x4-aligned after minification of the base LOD. Since the base LOD
   * dimensions affect the placement of smaller LODs, it's not trivial
   * (nor possible, at times) to satisfy the requirement by adjusting
   * the base LOD extent. Just assert that the caller is accessing an
   * LOD that satisfies this requirement.
   */
   assert(u_minify(params.depth.surf.logical_level0_px.width,
         assert(u_minify(params.depth.surf.logical_level0_px.height,
               params.dst.surf.samples = params.depth.surf.samples;
   params.dst.surf.logical_level0_px = params.depth.surf.logical_level0_px;
   params.depth_format =
                        }
