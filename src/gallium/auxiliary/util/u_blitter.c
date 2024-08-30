   /**************************************************************************
   *
   * Copyright 2009 Marek Olšák <maraeo@gmail.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * @file
   * Blitter utility to facilitate acceleration of the clear, clear_render_target,
   * clear_depth_stencil, resource_copy_region, and blit functions.
   *
   * @author Marek Olšák
   */
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "pipe/p_shader_tokens.h"
   #include "pipe/p_state.h"
      #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "util/u_blitter.h"
   #include "util/u_draw_quad.h"
   #include "util/u_sampler.h"
   #include "util/u_simple_shaders.h"
   #include "util/u_surface.h"
   #include "util/u_texture.h"
   #include "util/u_upload_mgr.h"
      #define INVALID_PTR ((void*)~0)
      #define GET_CLEAR_BLEND_STATE_IDX(clear_buffers) \
            #define NUM_RESOLVE_FRAG_SHADERS 5 /* MSAA 2x, 4x, 8x, 16x, 32x */
   #define GET_MSAA_RESOLVE_FS_IDX(nr_samples) (util_logbase2(nr_samples)-1)
      struct blitter_context_priv
   {
                                 /* Constant state objects. */
   /* Vertex shaders. */
   void *vs; /**< Vertex shader which passes {pos, generic} to the output.*/
   void *vs_nogeneric;
   void *vs_pos_only[4]; /**< Vertex shader which passes pos to the output
                  /* Fragment shaders. */
   void *fs_empty;
   void *fs_write_one_cbuf;
            /* FS which outputs a color from a texture where
   * the 1st index indicates the texture type / destination type,
   * the 2nd index is the PIPE_TEXTURE_* to be sampled,
   * the 3rd index is 0 = use TEX, 1 = use TXF.
   */
            /* FS which outputs a depth from a texture, where
   * the 1st index is the PIPE_TEXTURE_* to be sampled,
   * the 2nd index is 0 = use TEX, 1 = use TXF.
   */
   void *fs_texfetch_depth[PIPE_MAX_TEXTURE_TYPES][2];
   void *fs_texfetch_depthstencil[PIPE_MAX_TEXTURE_TYPES][2];
            /* FS which outputs one sample from a multisample texture. */
   void *fs_texfetch_col_msaa[5][PIPE_MAX_TEXTURE_TYPES];
   void *fs_texfetch_depth_msaa[PIPE_MAX_TEXTURE_TYPES][2];
   void *fs_texfetch_depthstencil_msaa[PIPE_MAX_TEXTURE_TYPES][2];
            /* FS which outputs an average of all samples. */
            /* FS which unpacks color to ZS or packs ZS to color, matching
   * the ZS format. See util_blitter_get_color_format_for_zs().
   */
            /* FS which is meant for replicating indevidual stencil-buffer bits */
            /* Blend state. */
   void *blend[PIPE_MASK_RGBA+1][2]; /**< blend state with writemask */
            /* Depth stencil alpha state. */
   void *dsa_write_depth_stencil;
   void *dsa_write_depth_keep_stencil;
   void *dsa_keep_depth_stencil;
   void *dsa_keep_depth_write_stencil;
            /* Vertex elements states. */
   void *velem_state;
            /* Sampler state. */
   void *sampler_state;
   void *sampler_state_linear;
   void *sampler_state_rect;
            /* Rasterizer state. */
   void *rs_state[2][2];  /**< [scissor][msaa] */
            /* Destination surface dimensions. */
   unsigned dst_width;
                     bool has_geometry_shader;
   bool has_tessellation;
   bool has_layered;
   bool has_stream_out;
   bool has_stencil_export;
   bool has_texture_multisample;
   bool has_tex_lz;
   bool has_txf_txq;
   bool has_sample_shading;
   bool cube_as_2darray;
   bool has_texrect;
            /* The Draw module overrides these functions.
   * Always create the blitter before Draw. */
   void   (*bind_fs_state)(struct pipe_context *, void *);
      };
      struct blitter_context *util_blitter_create(struct pipe_context *pipe)
   {
      struct blitter_context_priv *ctx;
   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_rasterizer_state rs_state;
   struct pipe_sampler_state sampler_state;
   struct pipe_vertex_element velem[2];
            ctx = CALLOC_STRUCT(blitter_context_priv);
   if (!ctx)
            ctx->base.pipe = pipe;
            ctx->bind_fs_state = pipe->bind_fs_state;
            /* init state objects for them to be considered invalid */
   ctx->base.saved_blend_state = INVALID_PTR;
   ctx->base.saved_dsa_state = INVALID_PTR;
   ctx->base.saved_rs_state = INVALID_PTR;
   ctx->base.saved_fs = INVALID_PTR;
   ctx->base.saved_vs = INVALID_PTR;
   ctx->base.saved_gs = INVALID_PTR;
   ctx->base.saved_velem_state = INVALID_PTR;
   ctx->base.saved_fb_state.nr_cbufs = ~0;
   ctx->base.saved_num_sampler_views = ~0;
   ctx->base.saved_num_sampler_states = ~0;
            ctx->has_geometry_shader =
      pipe->screen->get_shader_param(pipe->screen, PIPE_SHADER_GEOMETRY,
         ctx->has_tessellation =
      pipe->screen->get_shader_param(pipe->screen, PIPE_SHADER_TESS_CTRL,
         ctx->has_stream_out =
      pipe->screen->get_param(pipe->screen,
         ctx->has_stencil_export =
                  ctx->has_texture_multisample =
            ctx->has_tex_lz = pipe->screen->get_param(pipe->screen,
         ctx->has_txf_txq = pipe->screen->get_param(pipe->screen,
         ctx->has_sample_shading = pipe->screen->get_param(pipe->screen,
         ctx->cube_as_2darray = pipe->screen->get_param(pipe->screen,
                  /* blend state objects */
            for (i = 0; i <= PIPE_MASK_RGBA; i++) {
      for (j = 0; j < 2; j++) {
      memset(&blend.rt[0], 0, sizeof(blend.rt[0]));
   blend.rt[0].colormask = i;
   if (j) {
      blend.rt[0].blend_enable = 1;
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
      }
                  /* depth stencil alpha state objects */
   memset(&dsa, 0, sizeof(dsa));
   ctx->dsa_keep_depth_stencil =
            dsa.depth_enabled = 1;
   dsa.depth_writemask = 1;
   dsa.depth_func = PIPE_FUNC_ALWAYS;
   ctx->dsa_write_depth_keep_stencil =
            dsa.stencil[0].enabled = 1;
   dsa.stencil[0].func = PIPE_FUNC_ALWAYS;
   dsa.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].valuemask = 0xff;
   dsa.stencil[0].writemask = 0xff;
   ctx->dsa_write_depth_stencil =
            dsa.depth_enabled = 0;
   dsa.depth_writemask = 0;
   ctx->dsa_keep_depth_write_stencil =
            /* sampler state */
   memset(&sampler_state, 0, sizeof(sampler_state));
   sampler_state.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler_state.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler_state.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler_state.unnormalized_coords = 0;
   ctx->sampler_state = pipe->create_sampler_state(pipe, &sampler_state);
   if (ctx->has_texrect) {
      sampler_state.unnormalized_coords = 1;
               sampler_state.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler_state.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler_state.unnormalized_coords = 0;
   ctx->sampler_state_linear = pipe->create_sampler_state(pipe, &sampler_state);
   if (ctx->has_texrect) {
      sampler_state.unnormalized_coords = 1;
               /* rasterizer state */
   memset(&rs_state, 0, sizeof(rs_state));
   rs_state.cull_face = PIPE_FACE_NONE;
   rs_state.half_pixel_center = 1;
   rs_state.bottom_edge_rule = 1;
   rs_state.flatshade = 1;
   rs_state.depth_clip_near = 1;
            unsigned scissor, msaa;
   for (scissor = 0; scissor < 2; scissor++) {
      for (msaa = 0; msaa < 2; msaa++) {
      rs_state.scissor = scissor;
   rs_state.multisample = msaa;
   ctx->rs_state[scissor][msaa] =
                  if (ctx->has_stream_out) {
      rs_state.scissor = rs_state.multisample = 0;
   rs_state.rasterizer_discard = 1;
                        /* vertex elements states */
   memset(&velem[0], 0, sizeof(velem[0]) * 2);
   for (i = 0; i < 2; i++) {
      velem[i].src_offset = i * 4 * sizeof(float);
   velem[i].src_stride = 8 * sizeof(float);
   velem[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      }
            if (ctx->has_stream_out) {
      static enum pipe_format formats[4] = {
      PIPE_FORMAT_R32_UINT,
   PIPE_FORMAT_R32G32_UINT,
   PIPE_FORMAT_R32G32B32_UINT,
               for (i = 0; i < 4; i++) {
      velem[0].src_format = formats[i];
   velem[0].vertex_buffer_index = 0;
   velem[0].src_stride = 0;
   ctx->velem_state_readbuf[i] =
                  ctx->has_layered =
      pipe->screen->get_param(pipe->screen, PIPE_CAP_VS_INSTANCEID) &&
         /* set invariant vertex coordinates */
   for (i = 0; i < 4; i++) {
      ctx->vertices[i][0][2] = 0; /*v.z*/
                  }
      void *util_blitter_get_noop_blend_state(struct blitter_context *blitter)
   {
                  }
      void *util_blitter_get_noop_dsa_state(struct blitter_context *blitter)
   {
                  }
      void *util_blitter_get_discard_rasterizer_state(struct blitter_context *blitter)
   {
                  }
      static void bind_vs_pos_only(struct blitter_context_priv *ctx,
         {
      struct pipe_context *pipe = ctx->base.pipe;
            if (!ctx->vs_pos_only[index]) {
      struct pipe_stream_output_info so;
   static const enum tgsi_semantic semantic_names[] =
                  memset(&so, 0, sizeof(so));
   so.num_outputs = 1;
   so.output[0].num_components = num_so_channels;
            ctx->vs_pos_only[index] =
      util_make_vertex_passthrough_shader_with_so(pipe, 1, semantic_names,
                     }
      static void *get_vs_passthrough_pos_generic(struct blitter_context *blitter)
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
            if (!ctx->vs) {
      static const enum tgsi_semantic semantic_names[] =
         const unsigned semantic_indices[] = { 0, 0 };
   ctx->vs =
      util_make_vertex_passthrough_shader(pipe, 2, semantic_names,
   }
      }
      static void *get_vs_passthrough_pos(struct blitter_context *blitter)
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
            if (!ctx->vs_nogeneric) {
      static const enum tgsi_semantic semantic_names[] =
                  ctx->vs_nogeneric =
      util_make_vertex_passthrough_shader(pipe, 1,
         }
      }
      static void *get_vs_layered(struct blitter_context *blitter)
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
            if (!ctx->vs_layered) {
         }
      }
      static void bind_fs_empty(struct blitter_context_priv *ctx)
   {
               if (!ctx->fs_empty) {
      assert(!ctx->cached_all_shaders);
                  }
      static void bind_fs_write_one_cbuf(struct blitter_context_priv *ctx)
   {
               if (!ctx->fs_write_one_cbuf) {
      assert(!ctx->cached_all_shaders);
   ctx->fs_write_one_cbuf =
      util_make_fragment_passthrough_shader(pipe, TGSI_SEMANTIC_GENERIC,
               }
      static void bind_fs_clear_all_cbufs(struct blitter_context_priv *ctx)
   {
               if (!ctx->fs_clear_all_cbufs) {
      assert(!ctx->cached_all_shaders);
                  }
      void util_blitter_destroy(struct blitter_context *blitter)
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = blitter->pipe;
            for (i = 0; i <= PIPE_MASK_RGBA; i++)
      for (j = 0; j < 2; j++)
         for (i = 0; i < ARRAY_SIZE(ctx->blend_clear); i++) {
      if (ctx->blend_clear[i])
      }
   pipe->delete_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
   pipe->delete_depth_stencil_alpha_state(pipe,
         pipe->delete_depth_stencil_alpha_state(pipe, ctx->dsa_write_depth_stencil);
            for (i = 0; i < ARRAY_SIZE(ctx->dsa_replicate_stencil_bit); i++) {
      if (ctx->dsa_replicate_stencil_bit[i])
               unsigned scissor, msaa;
   for (scissor = 0; scissor < 2; scissor++) {
      for (msaa = 0; msaa < 2; msaa++) {
                     if (ctx->rs_discard_state)
         if (ctx->vs)
         if (ctx->vs_nogeneric)
         for (i = 0; i < 4; i++)
      if (ctx->vs_pos_only[i])
      if (ctx->vs_layered)
         pipe->delete_vertex_elements_state(pipe, ctx->velem_state);
   for (i = 0; i < 4; i++) {
      if (ctx->velem_state_readbuf[i]) {
                     for (i = 0; i < PIPE_MAX_TEXTURE_TYPES; i++) {
      for (unsigned type = 0; type < ARRAY_SIZE(ctx->fs_texfetch_col); ++type) {
      for (unsigned inst = 0; inst < 2; inst++) {
      if (ctx->fs_texfetch_col[type][i][inst])
      }
   if (ctx->fs_texfetch_col_msaa[type][i])
               for (unsigned inst = 0; inst < 2; inst++) {
      if (ctx->fs_texfetch_depth[i][inst])
         if (ctx->fs_texfetch_depthstencil[i][inst])
         if (ctx->fs_texfetch_stencil[i][inst])
               for (unsigned ss = 0; ss < 2; ss++) {
      if (ctx->fs_texfetch_depth_msaa[i][ss])
         if (ctx->fs_texfetch_depthstencil_msaa[i][ss])
         if (ctx->fs_texfetch_stencil_msaa[i][ss])
               for (j = 0; j< ARRAY_SIZE(ctx->fs_resolve[i]); j++)
      for (f = 0; f < 2; f++)
                  for (i = 0; i < ARRAY_SIZE(ctx->fs_pack_color_zs); i++) {
      for (j = 0; j < ARRAY_SIZE(ctx->fs_pack_color_zs[0]); j++) {
      if (ctx->fs_pack_color_zs[i][j])
                  if (ctx->fs_empty)
         if (ctx->fs_write_one_cbuf)
         if (ctx->fs_clear_all_cbufs)
            for (i = 0; i < ARRAY_SIZE(ctx->fs_stencil_blit_fallback); ++i)
      if (ctx->fs_stencil_blit_fallback[i])
         if (ctx->sampler_state_rect_linear)
         if (ctx->sampler_state_rect)
         pipe->delete_sampler_state(pipe, ctx->sampler_state_linear);
   pipe->delete_sampler_state(pipe, ctx->sampler_state);
      }
      void util_blitter_set_texture_multisample(struct blitter_context *blitter,
         {
                  }
      void util_blitter_set_running_flag(struct blitter_context *blitter)
   {
      if (blitter->running) {
         }
               }
      void util_blitter_unset_running_flag(struct blitter_context *blitter)
   {
      if (!blitter->running) {
      _debug_printf("u_blitter:%i: Caught recursion. This is a driver bug.\n",
      }
               }
      static void blitter_check_saved_vertex_states(ASSERTED struct blitter_context_priv *ctx)
   {
      assert(ctx->base.saved_vs != INVALID_PTR);
   assert(!ctx->has_geometry_shader || ctx->base.saved_gs != INVALID_PTR);
   assert(!ctx->has_tessellation || ctx->base.saved_tcs != INVALID_PTR);
   assert(!ctx->has_tessellation || ctx->base.saved_tes != INVALID_PTR);
   assert(!ctx->has_stream_out || ctx->base.saved_num_so_targets != ~0u);
      }
      void util_blitter_restore_vertex_states(struct blitter_context *blitter)
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
            /* Vertex buffer. */
   if (ctx->base.saved_vertex_buffer.buffer.resource) {
      pipe->set_vertex_buffers(pipe, 1, 0, true,
                     /* Vertex elements. */
   if (ctx->base.saved_velem_state != INVALID_PTR) {
      pipe->bind_vertex_elements_state(pipe, ctx->base.saved_velem_state);
               /* Vertex shader. */
   pipe->bind_vs_state(pipe, ctx->base.saved_vs);
            /* Geometry shader. */
   if (ctx->has_geometry_shader) {
      pipe->bind_gs_state(pipe, ctx->base.saved_gs);
               if (ctx->has_tessellation) {
      pipe->bind_tcs_state(pipe, ctx->base.saved_tcs);
   pipe->bind_tes_state(pipe, ctx->base.saved_tes);
   ctx->base.saved_tcs = INVALID_PTR;
               /* Stream outputs. */
   if (ctx->has_stream_out) {
      unsigned offsets[PIPE_MAX_SO_BUFFERS];
   for (i = 0; i < ctx->base.saved_num_so_targets; i++)
         pipe->set_stream_output_targets(pipe,
                  for (i = 0; i < ctx->base.saved_num_so_targets; i++)
                        /* Rasterizer. */
   pipe->bind_rasterizer_state(pipe, ctx->base.saved_rs_state);
      }
      static void blitter_check_saved_fragment_states(ASSERTED struct blitter_context_priv *ctx)
   {
      assert(ctx->base.saved_fs != INVALID_PTR);
   assert(ctx->base.saved_dsa_state != INVALID_PTR);
      }
      void util_blitter_restore_fragment_states(struct blitter_context *blitter)
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
            /* Fragment shader. */
   ctx->bind_fs_state(pipe, ctx->base.saved_fs);
            /* Depth, stencil, alpha. */
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->base.saved_dsa_state);
            /* Blend state. */
   pipe->bind_blend_state(pipe, ctx->base.saved_blend_state);
            /* Sample mask. */
   if (ctx->base.is_sample_mask_saved) {
      pipe->set_sample_mask(pipe, ctx->base.saved_sample_mask);
               if (ctx->base.saved_min_samples != ~0 && pipe->set_min_samples)
                  /* Miscellaneous states. */
   /* XXX check whether these are saved and whether they need to be restored
   * (depending on the operation) */
            if (!blitter->skip_viewport_restore)
            if (blitter->saved_num_window_rectangles) {
      pipe->set_window_rectangles(pipe,
                     }
      static void blitter_check_saved_fb_state(ASSERTED struct blitter_context_priv *ctx)
   {
         }
      static void blitter_disable_render_cond(struct blitter_context_priv *ctx)
   {
               if (ctx->base.saved_render_cond_query) {
            }
      void util_blitter_restore_render_cond(struct blitter_context *blitter)
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
            if (ctx->base.saved_render_cond_query) {
      pipe->render_condition(pipe, ctx->base.saved_render_cond_query,
                     }
      void util_blitter_restore_fb_state(struct blitter_context *blitter)
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
            pipe->set_framebuffer_state(pipe, &ctx->base.saved_fb_state);
      }
      static void blitter_check_saved_textures(ASSERTED struct blitter_context_priv *ctx)
   {
      assert(ctx->base.saved_num_sampler_states != ~0u);
      }
      static void util_blitter_restore_textures_internal(struct blitter_context *blitter, unsigned count)
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
            /* Fragment sampler states. */
   void *states[2] = {NULL};
   assert(count <= ARRAY_SIZE(states));
   if (ctx->base.saved_num_sampler_states)
      pipe->bind_sampler_states(pipe, PIPE_SHADER_FRAGMENT, 0,
            else if (count)
      pipe->bind_sampler_states(pipe, PIPE_SHADER_FRAGMENT, 0,
                        /* Fragment sampler views. */
   if (ctx->base.saved_num_sampler_views)
      pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0,
            else if (count)
      pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0,
               /* Just clear them to NULL because set_sampler_views(take_ownership = true). */
   for (i = 0; i < ctx->base.saved_num_sampler_views; i++)
               }
      void util_blitter_restore_textures(struct blitter_context *blitter)
   {
         }
      void util_blitter_restore_constant_buffer_state(struct blitter_context *blitter)
   {
               pipe->set_constant_buffer(pipe, PIPE_SHADER_FRAGMENT, blitter->cb_slot,
            }
      static void blitter_set_rectangle(struct blitter_context_priv *ctx,
               {
      /* set vertex positions */
   ctx->vertices[0][0][0] = (float)x1 / ctx->dst_width * 2.0f - 1.0f; /*v0.x*/
            ctx->vertices[1][0][0] = (float)x2 / ctx->dst_width * 2.0f - 1.0f; /*v1.x*/
            ctx->vertices[2][0][0] = (float)x2 / ctx->dst_width * 2.0f - 1.0f; /*v2.x*/
            ctx->vertices[3][0][0] = (float)x1 / ctx->dst_width * 2.0f - 1.0f; /*v3.x*/
            for (unsigned i = 0; i < 4; ++i)
            /* viewport */
   struct pipe_viewport_state viewport;
   viewport.scale[0] = 0.5f * ctx->dst_width;
   viewport.scale[1] = 0.5f * ctx->dst_height;
   viewport.scale[2] = 1.0f;
   viewport.translate[0] = 0.5f * ctx->dst_width;
   viewport.translate[1] = 0.5f * ctx->dst_height;
   viewport.translate[2] = 0.0f;
   viewport.swizzle_x = PIPE_VIEWPORT_SWIZZLE_POSITIVE_X;
   viewport.swizzle_y = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Y;
   viewport.swizzle_z = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Z;
   viewport.swizzle_w = PIPE_VIEWPORT_SWIZZLE_POSITIVE_W;
      }
      static void blitter_set_clear_color(struct blitter_context_priv *ctx,
         {
               if (color) {
      for (i = 0; i < 4; i++)
      } else {
      for (i = 0; i < 4; i++)
         }
      static void get_texcoords(struct pipe_sampler_view *src,
                           {
      unsigned level = src->u.tex.first_level;
   bool normalized = !uses_txf &&
                  if (normalized) {
      out->texcoord.x1 = x1 / (float)u_minify(src_width0,  level);
   out->texcoord.y1 = y1 / (float)u_minify(src_height0, level);
   out->texcoord.x2 = x2 / (float)u_minify(src_width0,  level);
      } else {
      out->texcoord.x1 = x1;
   out->texcoord.y1 = y1;
   out->texcoord.x2 = x2;
               out->texcoord.z = 0;
            /* Set the layer. */
   switch (src->target) {
   case PIPE_TEXTURE_3D:
      {
                                 }
         case PIPE_TEXTURE_1D_ARRAY:
      out->texcoord.y1 = out->texcoord.y2 = layer;
         case PIPE_TEXTURE_2D_ARRAY:
      out->texcoord.z = layer;
   out->texcoord.w = sample;
         case PIPE_TEXTURE_CUBE_ARRAY:
      out->texcoord.w = (unsigned)layer / 6;
         case PIPE_TEXTURE_2D:
      out->texcoord.w = sample;
         default:;
      }
      static void blitter_set_dst_dimensions(struct blitter_context_priv *ctx,
         {
      ctx->dst_width = width;
      }
      static void set_texcoords_in_vertices(const union blitter_attrib *attrib,
         {
      out[0] = attrib->texcoord.x1;
   out[1] = attrib->texcoord.y1;
   out += stride;
   out[0] = attrib->texcoord.x2;
   out[1] = attrib->texcoord.y1;
   out += stride;
   out[0] = attrib->texcoord.x2;
   out[1] = attrib->texcoord.y2;
   out += stride;
   out[0] = attrib->texcoord.x1;
      }
      static void *blitter_get_fs_texfetch_col(struct blitter_context_priv *ctx,
                                             {
      struct pipe_context *pipe = ctx->base.pipe;
   enum tgsi_texture_type tgsi_tex =
         enum tgsi_return_type stype;
   enum tgsi_return_type dtype;
                     if (util_format_is_pure_uint(src_format)) {
      stype = TGSI_RETURN_TYPE_UINT;
   if (util_format_is_pure_uint(dst_format)) {
      dtype = TGSI_RETURN_TYPE_UINT;
      } else {
      assert(util_format_is_pure_sint(dst_format));
   dtype = TGSI_RETURN_TYPE_SINT;
         } else if (util_format_is_pure_sint(src_format)) {
      stype = TGSI_RETURN_TYPE_SINT;
   if (util_format_is_pure_sint(dst_format)) {
      dtype = TGSI_RETURN_TYPE_SINT;
      } else {
      assert(util_format_is_pure_uint(dst_format));
   dtype = TGSI_RETURN_TYPE_UINT;
         } else {
      assert(!util_format_is_pure_uint(dst_format) &&
         dtype = stype = TGSI_RETURN_TYPE_FLOAT;
               if (src_nr_samples > 1) {
               /* OpenGL requires that integer textures just copy 1 sample instead
   * of averaging.
   */
   if (dst_nr_samples <= 1 &&
      stype != TGSI_RETURN_TYPE_UINT &&
   stype != TGSI_RETURN_TYPE_SINT) {
                                    if (!*shader) {
      assert(!ctx->cached_all_shaders);
   if (filter == PIPE_TEX_FILTER_LINEAR) {
      *shader = util_make_fs_msaa_resolve_bilinear(pipe, tgsi_tex,
      }
   else {
      *shader = util_make_fs_msaa_resolve(pipe, tgsi_tex,
            }
   else {
      /* The destination has multiple samples, we'll do
   * an MSAA->MSAA copy.
                  /* Create the fragment shader on-demand. */
   if (!*shader) {
      assert(!ctx->cached_all_shaders);
   *shader = util_make_fs_blit_msaa_color(pipe, tgsi_tex, stype, dtype,
                           } else {
               if (use_txf)
         else
            /* Create the fragment shader on-demand. */
   if (!*shader) {
      assert(!ctx->cached_all_shaders);
   *shader = util_make_fragment_tex_shader(pipe, tgsi_tex,
                           }
      static inline
   void *blitter_get_fs_pack_color_zs(struct blitter_context_priv *ctx,
                           {
      struct pipe_context *pipe = ctx->base.pipe;
   enum tgsi_texture_type tgsi_tex =
         int format_index = zs_format == PIPE_FORMAT_Z24_UNORM_S8_UINT ? 0 :
                              if (format_index == -1) {
      assert(0);
               /* The first 5 shaders pack ZS to color, the last 5 shaders unpack color
   * to ZS.
   */
   if (dst_is_color)
                     /* Create the fragment shader on-demand. */
   if (!*shader) {
      assert(!ctx->cached_all_shaders);
   *shader = util_make_fs_pack_color_zs(pipe, tgsi_tex, zs_format,
      }
      }
      static inline
   void *blitter_get_fs_texfetch_depth(struct blitter_context_priv *ctx,
                     {
                        if (src_samples > 1) {
      bool sample_shading = ctx->has_sample_shading && src_samples > 1 &&
                  /* Create the fragment shader on-demand. */
   if (!*shader) {
      enum tgsi_texture_type tgsi_tex;
   assert(!ctx->cached_all_shaders);
   tgsi_tex = util_pipe_tex_to_tgsi_tex(target, src_samples);
   *shader = util_make_fs_blit_msaa_depth(pipe, tgsi_tex, sample_shading,
                  } else {
               if (use_txf)
         else
            /* Create the fragment shader on-demand. */
   if (!*shader) {
      enum tgsi_texture_type tgsi_tex;
   assert(!ctx->cached_all_shaders);
   tgsi_tex = util_pipe_tex_to_tgsi_tex(target, 0);
   *shader = util_make_fs_blit_zs(pipe, PIPE_MASK_Z, tgsi_tex,
                     }
      static inline
   void *blitter_get_fs_texfetch_depthstencil(struct blitter_context_priv *ctx,
                     {
                        if (src_samples > 1) {
      bool sample_shading = ctx->has_sample_shading && src_samples > 1 &&
                  /* Create the fragment shader on-demand. */
   if (!*shader) {
      enum tgsi_texture_type tgsi_tex;
   assert(!ctx->cached_all_shaders);
   tgsi_tex = util_pipe_tex_to_tgsi_tex(target, src_samples);
   *shader = util_make_fs_blit_msaa_depthstencil(pipe, tgsi_tex,
                        } else {
               if (use_txf)
         else
            /* Create the fragment shader on-demand. */
   if (!*shader) {
      enum tgsi_texture_type tgsi_tex;
   assert(!ctx->cached_all_shaders);
   tgsi_tex = util_pipe_tex_to_tgsi_tex(target, 0);
   *shader = util_make_fs_blit_zs(pipe, PIPE_MASK_ZS, tgsi_tex,
                     }
      static inline
   void *blitter_get_fs_texfetch_stencil(struct blitter_context_priv *ctx,
                     {
                        if (src_samples > 1) {
      bool sample_shading = ctx->has_sample_shading && src_samples > 1 &&
                  /* Create the fragment shader on-demand. */
   if (!*shader) {
      enum tgsi_texture_type tgsi_tex;
   assert(!ctx->cached_all_shaders);
   tgsi_tex = util_pipe_tex_to_tgsi_tex(target, src_samples);
   *shader = util_make_fs_blit_msaa_stencil(pipe, tgsi_tex,
                        } else {
               if (use_txf)
         else
            /* Create the fragment shader on-demand. */
   if (!*shader) {
      enum tgsi_texture_type tgsi_tex;
   assert(!ctx->cached_all_shaders);
   tgsi_tex = util_pipe_tex_to_tgsi_tex(target, 0);
   *shader = util_make_fs_blit_zs(pipe, PIPE_MASK_S, tgsi_tex,
                     }
         /**
   * Generate and save all fragment shaders that we will ever need for
   * blitting.  Drivers which use the 'draw' fallbacks will typically use
   * this to make sure we generate/use shaders that don't go through the
   * draw module's wrapper functions.
   */
   void util_blitter_cache_all_shaders(struct blitter_context *blitter)
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = blitter->pipe;
   struct pipe_screen *screen = pipe->screen;
   unsigned samples, j, f, target, max_samples, use_txf;
            max_samples = ctx->has_texture_multisample ? 2 : 1;
   has_arraytex = screen->get_param(screen,
         has_cubearraytex = screen->get_param(screen,
            /* It only matters if i <= 1 or > 1. */
   for (samples = 1; samples <= max_samples; samples++) {
      for (target = PIPE_TEXTURE_1D; target < PIPE_MAX_TEXTURE_TYPES; target++) {
      for (use_txf = 0; use_txf <= ctx->has_txf_txq; use_txf++) {
      if (!has_arraytex &&
      (target == PIPE_TEXTURE_1D_ARRAY ||
   target == PIPE_TEXTURE_2D_ARRAY)) {
      }
   if (!has_cubearraytex &&
      (target == PIPE_TEXTURE_CUBE_ARRAY))
      if (!ctx->has_texrect &&
                  if (samples > 1 &&
                                       /* If samples == 1, the shaders read one texel. If samples >= 1,
   * they read one sample.
   */
   blitter_get_fs_texfetch_col(ctx, PIPE_FORMAT_R32_FLOAT,
               blitter_get_fs_texfetch_col(ctx, PIPE_FORMAT_R32_UINT,
               blitter_get_fs_texfetch_col(ctx, PIPE_FORMAT_R32_UINT,
               blitter_get_fs_texfetch_col(ctx, PIPE_FORMAT_R32_SINT,
               blitter_get_fs_texfetch_col(ctx, PIPE_FORMAT_R32_SINT,
               blitter_get_fs_texfetch_depth(ctx, target, samples, samples, use_txf);
   if (ctx->has_stencil_export) {
                        if (samples == 2) {
      blitter_get_fs_texfetch_depth(ctx, target, samples, 1, use_txf);
   if (ctx->has_stencil_export) {
      blitter_get_fs_texfetch_depthstencil(ctx, target, samples, 1, use_txf);
                                 /* MSAA resolve shaders. */
   for (j = 2; j < 32; j++) {
      if (!screen->is_format_supported(screen, PIPE_FORMAT_R32_FLOAT,
                                                      blitter_get_fs_texfetch_col(ctx, PIPE_FORMAT_R32_FLOAT,
               blitter_get_fs_texfetch_col(ctx, PIPE_FORMAT_R32_UINT,
               blitter_get_fs_texfetch_col(ctx, PIPE_FORMAT_R32_SINT,
                                          ctx->fs_write_one_cbuf =
      util_make_fragment_passthrough_shader(pipe, TGSI_SEMANTIC_GENERIC,
                     }
      static void blitter_set_common_draw_rect_state(struct blitter_context_priv *ctx,
         {
               if (ctx->base.saved_num_window_rectangles)
                     if (ctx->has_geometry_shader)
         if (ctx->has_tessellation) {
      pipe->bind_tcs_state(pipe, NULL);
      }
   if (ctx->has_stream_out)
      }
      static void blitter_draw(struct blitter_context_priv *ctx,
                           {
      struct pipe_context *pipe = ctx->base.pipe;
                     u_upload_data(pipe->stream_uploader, 0, sizeof(ctx->vertices), 4, ctx->vertices,
         if (!vb.buffer.resource)
                  pipe->set_vertex_buffers(pipe, 1, 0, false, &vb);
   pipe->bind_vertex_elements_state(pipe, vertex_elements_cso);
            if (ctx->base.use_index_buffer) {
      /* Note that for V3D,
   * dEQP-GLES3.functional.fbo.blit.rect.nearest_consistency_* require
   * that the last vert of the two tris be the same.
   */
   static uint8_t indices[6] = { 0, 1, 2, 0, 3, 2 };
   util_draw_elements_instanced(pipe, indices, 1, 0,
            } else {
      util_draw_arrays_instanced(pipe, MESA_PRIM_TRIANGLE_FAN, 0, 4,
      }
      }
      void util_blitter_draw_rectangle(struct blitter_context *blitter,
                                       {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
            switch (type) {
      case UTIL_BLITTER_ATTRIB_COLOR:
                  case UTIL_BLITTER_ATTRIB_TEXCOORD_XYZW:
      for (i = 0; i < 4; i++) {
      ctx->vertices[i][1][2] = attrib->texcoord.z;
      }
   set_texcoords_in_vertices(attrib, &ctx->vertices[0][1][0], 8);
      case UTIL_BLITTER_ATTRIB_TEXCOORD_XY:
      /* We clean-up the ZW components, just in case we used before XYZW,
   * to avoid feeding in the shader with wrong values (like on the lod)
   */
   for (i = 0; i < 4; i++) {
      ctx->vertices[i][1][2] = 0;
      }
                           blitter_draw(ctx, vertex_elements_cso, get_vs, x1, y1, x2, y2, depth,
      }
      static void *get_clear_blend_state(struct blitter_context_priv *ctx,
         {
      struct pipe_context *pipe = ctx->base.pipe;
                     /* Return an existing blend state. */
   if (!clear_buffers)
                     if (ctx->blend_clear[index])
            /* Create a new one. */
   {
      struct pipe_blend_state blend = {0};
                     for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      if (clear_buffers & (PIPE_CLEAR_COLOR0 << i)) {
      blend.rt[i].colormask = PIPE_MASK_RGBA;
                     }
      }
      void util_blitter_common_clear_setup(struct blitter_context *blitter,
                     {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
            util_blitter_set_running_flag(blitter);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
            /* bind states */
   if (custom_blend) {
         } else {
                  if (custom_dsa) {
         } else if ((clear_buffers & PIPE_CLEAR_DEPTHSTENCIL) == PIPE_CLEAR_DEPTHSTENCIL) {
         } else if (clear_buffers & PIPE_CLEAR_DEPTH) {
         } else if (clear_buffers & PIPE_CLEAR_STENCIL) {
         } else {
                  pipe->set_sample_mask(pipe, ~0);
   if (pipe->set_min_samples)
            }
      static void util_blitter_clear_custom(struct blitter_context *blitter,
                                       unsigned width, unsigned height,
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
                     util_blitter_common_clear_setup(blitter, width, height, clear_buffers,
            sr.ref_value[0] = stencil & 0xff;
            bool pass_generic = (clear_buffers & PIPE_CLEAR_COLOR) != 0;
            if (pass_generic) {
      struct pipe_constant_buffer cb = {
      .user_buffer = color->f,
      };
   pipe->set_constant_buffer(pipe, PIPE_SHADER_FRAGMENT, blitter->cb_slot,
            } else {
                  if (num_layers > 1 && ctx->has_layered) {
               blitter_set_common_draw_rect_state(ctx, false, msaa);
   blitter->draw_rectangle(blitter, ctx->velem_state, get_vs,
            } else {
               if (pass_generic)
         else
            blitter_set_common_draw_rect_state(ctx, false, msaa);
   blitter->draw_rectangle(blitter, ctx->velem_state, get_vs,
                     util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_fragment_states(blitter);
   util_blitter_restore_constant_buffer_state(blitter);
   util_blitter_restore_render_cond(blitter);
      }
      void util_blitter_clear(struct blitter_context *blitter,
                           unsigned width, unsigned height, unsigned num_layers,
   {
      util_blitter_clear_custom(blitter, width, height, num_layers,
            }
      void util_blitter_custom_clear_depth(struct blitter_context *blitter,
               {
      static const union pipe_color_union color;
   util_blitter_clear_custom(blitter, width, height, 0, 0, &color, depth, 0,
      }
      void util_blitter_default_dst_texture(struct pipe_surface *dst_templ,
                     {
      memset(dst_templ, 0, sizeof(*dst_templ));
   dst_templ->format = util_format_linear(dst->format);
   dst_templ->u.tex.level = dstlevel;
   dst_templ->u.tex.first_layer = dstz;
      }
      static struct pipe_surface *
   util_blitter_get_next_surface_layer(struct pipe_context *pipe,
         {
               memset(&dst_templ, 0, sizeof(dst_templ));
   dst_templ.format = surf->format;
   dst_templ.u.tex.level = surf->u.tex.level;
   dst_templ.u.tex.first_layer = surf->u.tex.first_layer + 1;
               }
      void util_blitter_default_src_texture(struct blitter_context *blitter,
                     {
                        if (ctx->cube_as_2darray &&
      (src->target == PIPE_TEXTURE_CUBE ||
   src->target == PIPE_TEXTURE_CUBE_ARRAY))
      else
            src_templ->format = util_format_linear(src->format);
   src_templ->u.tex.first_level = srclevel;
   src_templ->u.tex.last_level = srclevel;
   src_templ->u.tex.first_layer = 0;
   src_templ->u.tex.last_layer =
      src->target == PIPE_TEXTURE_3D ? u_minify(src->depth0, srclevel) - 1
      src_templ->swizzle_r = PIPE_SWIZZLE_X;
   src_templ->swizzle_g = PIPE_SWIZZLE_Y;
   src_templ->swizzle_b = PIPE_SWIZZLE_Z;
      }
      static bool is_blit_generic_supported(struct blitter_context *blitter,
                                 {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
            if (dst) {
      unsigned bind;
   const struct util_format_description *desc =
                  /* Stencil export must be supported for stencil copy. */
   if ((mask & PIPE_MASK_S) && dst_has_stencil &&
      !ctx->has_stencil_export) {
               if (dst_has_stencil || util_format_has_depth(desc))
         else
            if (!screen->is_format_supported(screen, dst_format, dst->target,
                                 if (src) {
      if (src->nr_samples > 1 && !ctx->has_texture_multisample) {
                  if (!screen->is_format_supported(screen, src_format, src->target,
                              /* Check stencil sampler support for stencil copy. */
   if (mask & PIPE_MASK_S) {
      if (util_format_has_stencil(util_format_description(src_format))) {
      enum pipe_format stencil_format =
                  if (stencil_format != src_format &&
      !screen->is_format_supported(screen, stencil_format,
                                             }
      bool util_blitter_is_copy_supported(struct blitter_context *blitter,
               {
      return is_blit_generic_supported(blitter, dst, dst->format,
      }
      bool util_blitter_is_blit_supported(struct blitter_context *blitter,
         {
      return is_blit_generic_supported(blitter,
                  }
      void util_blitter_copy_texture(struct blitter_context *blitter,
                                 struct pipe_resource *dst,
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_surface *dst_view, dst_templ;
   struct pipe_sampler_view src_templ, *src_view;
            assert(dst && src);
            u_box_3d(dstx, dsty, dstz, abs(srcbox->width), abs(srcbox->height),
            /* Initialize the surface. */
   util_blitter_default_dst_texture(&dst_templ, dst, dst_level, dstz);
            /* Initialize the sampler view. */
   util_blitter_default_src_texture(blitter, &src_templ, src, src_level);
            /* Copy. */
   util_blitter_blit_generic(blitter, dst_view, &dstbox,
                        pipe_surface_reference(&dst_view, NULL);
      }
      static void
   blitter_draw_tex(struct blitter_context_priv *ctx,
                  int dst_x1, int dst_y1, int dst_x2, int dst_y2,
   struct pipe_sampler_view *src,
   unsigned src_width0, unsigned src_height0,
      {
      union blitter_attrib coord;
            get_texcoords(src, src_width0, src_height0,
                  if (src->target == PIPE_TEXTURE_CUBE ||
      src->target == PIPE_TEXTURE_CUBE_ARRAY) {
            set_texcoords_in_vertices(&coord, &face_coord[0][0], 2);
   util_map_texcoords2d_onto_cubemap((unsigned)layer % 6,
                           for (unsigned i = 0; i < 4; i++)
            /* Cubemaps don't use draw_rectangle. */
   blitter_draw(ctx, ctx->velem_state, get_vs,
      } else {
      ctx->base.draw_rectangle(&ctx->base, ctx->velem_state, get_vs,
               }
      static void do_blits(struct blitter_context_priv *ctx,
                        struct pipe_surface *dst,
   const struct pipe_box *dstbox,
   struct pipe_sampler_view *src,
   unsigned src_width0,
   unsigned src_height0,
      {
      struct pipe_context *pipe = ctx->base.pipe;
   unsigned src_samples = src->texture->nr_samples;
   unsigned dst_samples = dst->texture->nr_samples;
   bool sample_shading = ctx->has_sample_shading && src_samples > 1 &&
         enum pipe_texture_target src_target = src->target;
            /* Initialize framebuffer state. */
   fb_state.width = dst->width;
   fb_state.height = dst->height;
                     if ((src_target == PIPE_TEXTURE_1D ||
      src_target == PIPE_TEXTURE_2D ||
   src_target == PIPE_TEXTURE_RECT) &&
   (src_samples <= 1 || sample_shading)) {
   /* Set framebuffer state. */
   if (is_zsbuf) {
         } else {
         }
            /* Draw. */
   pipe->set_sample_mask(pipe, dst_sample ? BITFIELD_BIT(dst_sample - 1) : ~0);
   if (pipe->set_min_samples)
         blitter_draw_tex(ctx, dstbox->x, dstbox->y,
                  dstbox->x + dstbox->width,
   dstbox->y + dstbox->height,
   } else {
      /* Draw the quad with the generic codepath. */
   int dst_z;
   for (dst_z = 0; dst_z < dstbox->depth; dst_z++) {
      struct pipe_surface *old;
   bool flipped = (srcbox->depth < 0);
   float depth_center_offset = 0.0;
                  /* Scale Z properly if the blit is scaled.
   *
   * When downscaling, we want the coordinates centered, so that
   * mipmapping works for 3D textures. For example, when generating
   * a 4x4x4 level, this wouldn't average the pixels:
   *
   *   src Z:  0 1 2 3 4 5 6 7
   *   dst Z:  0   1   2   3
   *
   * Because the pixels are not centered below the pixels of the higher
   * level. Therefore, we want this:
   *   src Z:  0 1 2 3 4 5 6 7
   *   dst Z:   0   1   2   3
   *
   * This calculation is taken from the radv driver.
   */
                  if (flipped) {
      src_z_step *= - 1;
                        /* Set framebuffer state. */
   if (is_zsbuf) {
         } else {
                        /* See if we need to blit a multisample or singlesample buffer. */
   if (sample0_only || (src_samples == dst_samples && dst_samples > 1)) {
                     if (sample_shading) {
      assert(dst_sample == 0);
   pipe->set_sample_mask(pipe, ~0);
   if (pipe->set_min_samples)
         blitter_draw_tex(ctx, dstbox->x, dstbox->y,
                  dstbox->x + dstbox->width,
   dstbox->y + dstbox->height,
   src, src_width0, src_height0,
   srcbox->x, srcbox->y,
   srcbox->x + srcbox->width,
   } else {
                     for (i = 0; i <= max_sample; i++) {
      pipe->set_sample_mask(pipe, 1 << i);
   blitter_draw_tex(ctx, dstbox->x, dstbox->y,
                  dstbox->x + dstbox->width,
   dstbox->y + dstbox->height,
   src, src_width0, src_height0,
   srcbox->x, srcbox->y,
   srcbox->x + srcbox->width,
         } else {
      /* Normal copy, MSAA upsampling, or MSAA resolve. */
   pipe->set_sample_mask(pipe, dst_sample ? BITFIELD_BIT(dst_sample - 1) : ~0);
   if (pipe->set_min_samples)
         blitter_draw_tex(ctx, dstbox->x, dstbox->y,
                  dstbox->x + dstbox->width,
   dstbox->y + dstbox->height,
   src, src_width0, src_height0,
   srcbox->x, srcbox->y,
                  /* Get the next surface or (if this is the last iteration)
   * just unreference the last one. */
   old = dst;
   if (dst_z < dstbox->depth-1) {
         }
   if (dst_z) {
                  }
      void util_blitter_blit_generic(struct blitter_context *blitter,
                                 struct pipe_surface *dst,
   const struct pipe_box *dstbox,
   struct pipe_sampler_view *src,
   const struct pipe_box *srcbox,
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   enum pipe_texture_target src_target = src->target;
   unsigned src_samples = src->texture->nr_samples;
   unsigned dst_samples = dst->texture->nr_samples;
   void *sampler_state;
   const struct util_format_description *src_desc =
         const struct util_format_description *dst_desc =
            bool src_has_color = src_desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS;
   bool src_has_depth = util_format_has_depth(src_desc);
            bool dst_has_color = mask & PIPE_MASK_RGBA &&
         bool dst_has_depth = mask & PIPE_MASK_Z &&
         bool dst_has_stencil = ctx->has_stencil_export &&
                  /* Return if there is nothing to do. */
   if (!dst_has_color && !dst_has_depth && !dst_has_stencil) {
                  bool is_scaled = dstbox->width != abs(srcbox->width) ||
                  if (src_has_stencil || !is_scaled)
                     /* Don't support scaled blits. The TXF shader uses F2I for rounding. */
   if (ctx->has_txf_txq &&
      !is_scaled &&
   filter == PIPE_TEX_FILTER_NEAREST &&
   src->target != PIPE_TEXTURE_CUBE &&
   src->target != PIPE_TEXTURE_CUBE_ARRAY) {
   int src_width = u_minify(src_width0, src->u.tex.first_level);
   int src_height = u_minify(src_height0, src->u.tex.first_level);
   int src_depth = src->u.tex.last_layer + 1;
            /* Eliminate negative width/height/depth. */
   if (box.width < 0) {
      box.x += box.width;
      }
   if (box.height < 0) {
      box.y += box.height;
      }
   if (box.depth < 0) {
      box.z += box.depth;
               /* See if srcbox is in bounds. TXF doesn't clamp the coordinates. */
   use_txf =
      box.x >= 0 && box.x < src_width &&
   box.y >= 0 && box.y < src_height &&
   box.z >= 0 && box.z < src_depth &&
   box.x + box.width > 0 && box.x + box.width <= src_width &&
   box.y + box.height > 0 && box.y + box.height <= src_height &&
            /* Check whether the states are properly saved. */
   util_blitter_set_running_flag(blitter);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_textures(ctx);
   blitter_check_saved_fb_state(ctx);
            /* Blend, DSA, fragment shader. */
   if (dst_has_depth && dst_has_stencil) {
      pipe->bind_blend_state(pipe, ctx->blend[0][0]);
   pipe->bind_depth_stencil_alpha_state(pipe,
         if (src_has_color) {
      assert(use_txf);
   ctx->bind_fs_state(pipe,
      blitter_get_fs_pack_color_zs(ctx, src_target,
   } else {
      ctx->bind_fs_state(pipe,
      blitter_get_fs_texfetch_depthstencil(ctx, src_target, src_samples,
      } else if (dst_has_depth) {
      pipe->bind_blend_state(pipe, ctx->blend[0][0]);
   pipe->bind_depth_stencil_alpha_state(pipe,
         if (src_has_color &&
      (src->format == PIPE_FORMAT_R32_UINT ||
   src->format == PIPE_FORMAT_R32G32_UINT)) {
   assert(use_txf);
   ctx->bind_fs_state(pipe,
      blitter_get_fs_pack_color_zs(ctx, src_target,
   } else {
      ctx->bind_fs_state(pipe,
      blitter_get_fs_texfetch_depth(ctx, src_target, src_samples,
      } else if (dst_has_stencil) {
      pipe->bind_blend_state(pipe, ctx->blend[0][0]);
   pipe->bind_depth_stencil_alpha_state(pipe,
            assert(src_has_stencil); /* unpacking from color is unsupported */
   ctx->bind_fs_state(pipe,
      blitter_get_fs_texfetch_stencil(ctx, src_target, src_samples,
   } else {
               pipe->bind_blend_state(pipe, ctx->blend[colormask][alpha_blend]);
            if (src_has_depth &&
      (dst->format == PIPE_FORMAT_R32_UINT ||
   dst->format == PIPE_FORMAT_R32G32_UINT)) {
   assert(use_txf);
   ctx->bind_fs_state(pipe,
      blitter_get_fs_pack_color_zs(ctx, src_target,
   } else {
      ctx->bind_fs_state(pipe,
      blitter_get_fs_texfetch_col(ctx, src->format, dst->format, src_target,
                     /* Set the linear filter only for scaled color non-MSAA blits. */
   if (filter == PIPE_TEX_FILTER_LINEAR) {
      if (src_target == PIPE_TEXTURE_RECT && ctx->has_texrect) {
         } else {
            } else {
      if (src_target == PIPE_TEXTURE_RECT && ctx->has_texrect) {
         } else {
                     /* Set samplers. */
   unsigned count = 0;
   if (src_has_depth && src_has_stencil &&
      (dst_has_color || (dst_has_depth && dst_has_stencil))) {
   /* Setup two samplers, one for depth and the other one for stencil. */
   struct pipe_sampler_view templ;
   struct pipe_sampler_view *views[2];
            templ = *src;
   templ.format = util_format_stencil_only(templ.format);
            views[0] = src;
            count = 2;
   pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0, 2, 0, false, views);
               } else if (src_has_stencil && dst_has_stencil) {
      /* Set a stencil-only sampler view for it not to sample depth instead. */
   struct pipe_sampler_view templ;
            templ = *src;
   templ.format = util_format_stencil_only(templ.format);
                     count = 1;
   pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0, 1, 0, false, &view);
   pipe->bind_sampler_states(pipe, PIPE_SHADER_FRAGMENT,
               } else {
      count = 1;
   pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0, 1, 0, false, &src);
   pipe->bind_sampler_states(pipe, PIPE_SHADER_FRAGMENT,
               if (scissor) {
                           do_blits(ctx, dst, dstbox, src, src_width0, src_height0,
                  util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_fragment_states(blitter);
   util_blitter_restore_textures_internal(blitter, count);
   util_blitter_restore_fb_state(blitter);
   if (scissor) {
         }
   util_blitter_restore_render_cond(blitter);
      }
      void
   util_blitter_blit(struct blitter_context *blitter,
         {
      struct pipe_resource *dst = info->dst.resource;
   struct pipe_resource *src = info->src.resource;
   struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_surface *dst_view, dst_templ;
            /* Initialize the surface. */
   util_blitter_default_dst_texture(&dst_templ, dst, info->dst.level,
         dst_templ.format = info->dst.format;
            /* Initialize the sampler view. */
   util_blitter_default_src_texture(blitter, &src_templ, src, info->src.level);
   src_templ.format = info->src.format;
            /* Copy. */
   util_blitter_blit_generic(blitter, dst_view, &info->dst.box,
                                    pipe_surface_reference(&dst_view, NULL);
      }
      void util_blitter_generate_mipmap(struct blitter_context *blitter,
                           {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_surface dst_templ, *dst_view;
   struct pipe_sampler_view src_templ, *src_view;
   bool is_depth;
   void *sampler_state;
   const struct util_format_description *desc =
         unsigned src_level;
            if (ctx->cube_as_2darray &&
      (target == PIPE_TEXTURE_CUBE || target == PIPE_TEXTURE_CUBE_ARRAY))
         assert(tex->nr_samples <= 1);
   /* Disallow stencil formats without depth. */
                     /* Check whether the states are properly saved. */
   util_blitter_set_running_flag(blitter);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_textures(ctx);
   blitter_check_saved_fb_state(ctx);
            /* Set states. */
   if (is_depth) {
      pipe->bind_blend_state(pipe, ctx->blend[0][0]);
   pipe->bind_depth_stencil_alpha_state(pipe,
         ctx->bind_fs_state(pipe,
      } else {
      pipe->bind_blend_state(pipe, ctx->blend[PIPE_MASK_RGBA][0]);
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
   ctx->bind_fs_state(pipe,
                     if (target == PIPE_TEXTURE_RECT) {
         } else {
         }
   pipe->bind_sampler_states(pipe, PIPE_SHADER_FRAGMENT,
                     for (src_level = base_level; src_level < last_level; src_level++) {
      struct pipe_box dstbox = {0}, srcbox = {0};
            dstbox.width = u_minify(tex->width0, dst_level);
            srcbox.width = u_minify(tex->width0, src_level);
            if (target == PIPE_TEXTURE_3D) {
      dstbox.depth = util_num_layers(tex, dst_level);
      } else {
      dstbox.z = srcbox.z = first_layer;
               /* Initialize the surface. */
   util_blitter_default_dst_texture(&dst_templ, tex, dst_level,
         dst_templ.format = format;
            /* Initialize the sampler view. */
   util_blitter_default_src_texture(blitter, &src_templ, tex, src_level);
   src_templ.format = format;
                     do_blits(ctx, dst_view, &dstbox, src_view, tex->width0, tex->height0,
            pipe_surface_reference(&dst_view, NULL);
               util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_fragment_states(blitter);
   util_blitter_restore_textures_internal(blitter, 1);
   util_blitter_restore_fb_state(blitter);
   util_blitter_restore_render_cond(blitter);
      }
      /* Clear a region of a color surface to a constant value. */
   void util_blitter_clear_render_target(struct blitter_context *blitter,
                           {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_framebuffer_state fb_state;
   bool msaa;
   unsigned num_layers;
            assert(dstsurf->texture);
   if (!dstsurf->texture)
            /* check the saved state */
   util_blitter_set_running_flag(blitter);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_fb_state(ctx);
            /* bind states */
   pipe->bind_blend_state(pipe, ctx->blend[PIPE_MASK_RGBA][0]);
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
            /* set a framebuffer state */
   fb_state.width = dstsurf->width;
   fb_state.height = dstsurf->height;
   fb_state.nr_cbufs = 1;
   fb_state.cbufs[0] = dstsurf;
   fb_state.zsbuf = NULL;
   fb_state.resolve = NULL;
   pipe->set_framebuffer_state(pipe, &fb_state);
   pipe->set_sample_mask(pipe, ~0);
   if (pipe->set_min_samples)
                  blitter_set_dst_dimensions(ctx, dstsurf->width, dstsurf->height);
            union blitter_attrib attrib;
                     if (num_layers > 1 && ctx->has_layered) {
         } else {
      get_vs = get_vs_passthrough_pos_generic;
               blitter->draw_rectangle(blitter, ctx->velem_state, get_vs,
                  util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_fragment_states(blitter);
   util_blitter_restore_fb_state(blitter);
   util_blitter_restore_render_cond(blitter);
      }
      /* Clear a region of a depth stencil surface. */
   void util_blitter_clear_depth_stencil(struct blitter_context *blitter,
                                       {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_framebuffer_state fb_state;
   struct pipe_stencil_ref sr = { { 0 } };
            assert(dstsurf->texture);
   if (!dstsurf->texture)
            /* check the saved state */
   util_blitter_set_running_flag(blitter);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_fb_state(ctx);
            /* bind states */
   pipe->bind_blend_state(pipe, ctx->blend[0][0]);
   if ((clear_flags & PIPE_CLEAR_DEPTHSTENCIL) == PIPE_CLEAR_DEPTHSTENCIL) {
      sr.ref_value[0] = stencil & 0xff;
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_write_depth_stencil);
      }
   else if (clear_flags & PIPE_CLEAR_DEPTH) {
         }
   else if (clear_flags & PIPE_CLEAR_STENCIL) {
      sr.ref_value[0] = stencil & 0xff;
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_write_stencil);
      }
   else
      /* hmm that should be illegal probably, or make it a no-op somewhere */
                  /* set a framebuffer state */
   fb_state.width = dstsurf->width;
   fb_state.height = dstsurf->height;
   fb_state.nr_cbufs = 0;
   fb_state.cbufs[0] = NULL;
   fb_state.zsbuf = dstsurf;
   fb_state.resolve = NULL;
   pipe->set_framebuffer_state(pipe, &fb_state);
   pipe->set_sample_mask(pipe, ~0);
   if (pipe->set_min_samples)
                     num_layers = dstsurf->u.tex.last_layer - dstsurf->u.tex.first_layer + 1;
   if (num_layers > 1 && ctx->has_layered) {
      blitter_set_common_draw_rect_state(ctx, false, false);
   blitter->draw_rectangle(blitter, ctx->velem_state, get_vs_layered,
            } else {
      blitter_set_common_draw_rect_state(ctx, false, false);
   blitter->draw_rectangle(blitter, ctx->velem_state,
                           util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_fragment_states(blitter);
   util_blitter_restore_fb_state(blitter);
   util_blitter_restore_render_cond(blitter);
      }
      /* draw a rectangle across a region using a custom dsa stage - for r600g */
   void util_blitter_custom_depth_stencil(struct blitter_context *blitter,
                           {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
            assert(zsurf->texture);
   if (!zsurf->texture)
            /* check the saved state */
   util_blitter_set_running_flag(blitter);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_fb_state(ctx);
            /* bind states */
   pipe->bind_blend_state(pipe, cbsurf ? ctx->blend[PIPE_MASK_RGBA][0] :
         pipe->bind_depth_stencil_alpha_state(pipe, dsa_stage);
   if (cbsurf)
         else
            /* set a framebuffer state */
   fb_state.width = zsurf->width;
   fb_state.height = zsurf->height;
   fb_state.nr_cbufs = 1;
   if (cbsurf) {
      fb_state.cbufs[0] = cbsurf;
      } else {
      fb_state.cbufs[0] = NULL;
      }
   fb_state.zsbuf = zsurf;
   fb_state.resolve = NULL;
   pipe->set_framebuffer_state(pipe, &fb_state);
   pipe->set_sample_mask(pipe, sample_mask);
   if (pipe->set_min_samples)
            blitter_set_common_draw_rect_state(ctx, false,
         blitter_set_dst_dimensions(ctx, zsurf->width, zsurf->height);
   blitter->draw_rectangle(blitter, ctx->velem_state, get_vs_passthrough_pos,
                  util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_fragment_states(blitter);
   util_blitter_restore_fb_state(blitter);
   util_blitter_restore_render_cond(blitter);
      }
      void util_blitter_clear_buffer(struct blitter_context *blitter,
                           {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_vertex_buffer vb = {0};
   struct pipe_stream_output_target *so_target = NULL;
            assert(num_channels >= 1);
            /* IMPORTANT:  DON'T DO ANY BOUNDS CHECKING HERE!
   *
   * R600 uses this to initialize texture resources, so width0 might not be
   * what you think it is.
            /* Streamout is required. */
   if (!ctx->has_stream_out) {
      assert(!"Streamout unsupported in util_blitter_clear_buffer()");
               /* Some alignment is required. */
   if (offset % 4 != 0 || size % 4 != 0) {
      assert(!"Bad alignment in util_blitter_clear_buffer()");
               u_upload_data(pipe->stream_uploader, 0, num_channels*4, 4, clear_value,
         if (!vb.buffer.resource)
            util_blitter_set_running_flag(blitter);
   blitter_check_saved_vertex_states(ctx);
            pipe->set_vertex_buffers(pipe, 1, 0, false, &vb);
   pipe->bind_vertex_elements_state(pipe,
         bind_vs_pos_only(ctx, num_channels);
   if (ctx->has_geometry_shader)
         if (ctx->has_tessellation) {
      pipe->bind_tcs_state(pipe, NULL);
      }
            so_target = pipe->create_stream_output_target(pipe, dst, offset, size);
                  out:
      util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_render_cond(blitter);
   util_blitter_unset_running_flag(blitter);
   pipe_so_target_reference(&so_target, NULL);
      }
      /* probably radeon specific */
   void util_blitter_custom_resolve_color(struct blitter_context *blitter,
                                          struct pipe_resource *dst,
      {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
   struct pipe_framebuffer_state fb_state;
            util_blitter_set_running_flag(blitter);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
            /* bind states */
   pipe->bind_blend_state(pipe, custom_blend);
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
   bind_fs_write_one_cbuf(ctx);
   pipe->set_sample_mask(pipe, sample_mask);
   if (pipe->set_min_samples)
            memset(&surf_tmpl, 0, sizeof(surf_tmpl));
   surf_tmpl.format = format;
   surf_tmpl.u.tex.level = dst_level;
   surf_tmpl.u.tex.first_layer = dst_layer;
                     surf_tmpl.u.tex.level = 0;
   surf_tmpl.u.tex.first_layer = src_layer;
                     /* set a framebuffer state */
   fb_state.width = src->width0;
   fb_state.height = src->height0;
   fb_state.nr_cbufs = 2;
   fb_state.cbufs[0] = srcsurf;
   fb_state.cbufs[1] = dstsurf;
   fb_state.zsbuf = NULL;
   fb_state.resolve = NULL;
            blitter_set_common_draw_rect_state(ctx, false,
         blitter_set_dst_dimensions(ctx, src->width0, src->height0);
   blitter->draw_rectangle(blitter, ctx->velem_state, get_vs_passthrough_pos,
               util_blitter_restore_fb_state(blitter);
   util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_fragment_states(blitter);
   util_blitter_restore_render_cond(blitter);
            pipe_surface_reference(&srcsurf, NULL);
      }
      void util_blitter_custom_color(struct blitter_context *blitter,
               {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
            assert(dstsurf->texture);
   if (!dstsurf->texture)
            /* check the saved state */
   util_blitter_set_running_flag(blitter);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_fb_state(ctx);
            /* bind states */
   pipe->bind_blend_state(pipe, custom_blend ? custom_blend
         pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
            /* set a framebuffer state */
   fb_state.width = dstsurf->width;
   fb_state.height = dstsurf->height;
   fb_state.nr_cbufs = 1;
   fb_state.cbufs[0] = dstsurf;
   fb_state.zsbuf = NULL;
   fb_state.resolve = NULL;
   pipe->set_framebuffer_state(pipe, &fb_state);
   pipe->set_sample_mask(pipe, ~0);
   if (pipe->set_min_samples)
            blitter_set_common_draw_rect_state(ctx, false,
         blitter_set_dst_dimensions(ctx, dstsurf->width, dstsurf->height);
   blitter->draw_rectangle(blitter, ctx->velem_state, get_vs_passthrough_pos,
                  util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_fragment_states(blitter);
   util_blitter_restore_fb_state(blitter);
   util_blitter_restore_render_cond(blitter);
      }
      static void *get_custom_vs(struct blitter_context *blitter)
   {
                  }
      /**
   * Performs a custom blit to the destination surface, using the VS and FS
   * provided.
   *
   * Used by vc4 for the 8-bit linear-to-tiled blit.
   */
   void util_blitter_custom_shader(struct blitter_context *blitter,
               {
      struct blitter_context_priv *ctx = (struct blitter_context_priv*)blitter;
   struct pipe_context *pipe = ctx->base.pipe;
                     assert(dstsurf->texture);
   if (!dstsurf->texture)
            /* check the saved state */
   util_blitter_set_running_flag(blitter);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_fb_state(ctx);
            /* bind states */
   pipe->bind_blend_state(pipe, ctx->blend[PIPE_MASK_RGBA][0]);
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->dsa_keep_depth_stencil);
            /* set a framebuffer state */
   fb_state.width = dstsurf->width;
   fb_state.height = dstsurf->height;
   fb_state.nr_cbufs = 1;
   fb_state.cbufs[0] = dstsurf;
   fb_state.resolve = NULL;
   pipe->set_framebuffer_state(pipe, &fb_state);
   pipe->set_sample_mask(pipe, ~0);
   if (pipe->set_min_samples)
            blitter_set_common_draw_rect_state(ctx, false,
         blitter_set_dst_dimensions(ctx, dstsurf->width, dstsurf->height);
   blitter->draw_rectangle(blitter, ctx->velem_state, get_custom_vs,
                  util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_fragment_states(blitter);
   util_blitter_restore_fb_state(blitter);
   util_blitter_restore_render_cond(blitter);
      }
      static void *
   get_stencil_blit_fallback_fs(struct blitter_context_priv *ctx, bool msaa_src)
   {
      if (!ctx->fs_stencil_blit_fallback[msaa_src]) {
      ctx->fs_stencil_blit_fallback[msaa_src] =
                  }
      static void *
   get_stencil_blit_fallback_dsa(struct blitter_context_priv *ctx, unsigned i)
   {
      assert(i < ARRAY_SIZE(ctx->dsa_replicate_stencil_bit));
   if (!ctx->dsa_replicate_stencil_bit[i]) {
      struct pipe_depth_stencil_alpha_state dsa = { 0 };
   dsa.depth_func = PIPE_FUNC_ALWAYS;
   dsa.stencil[0].enabled = 1;
   dsa.stencil[0].func = PIPE_FUNC_ALWAYS;
   dsa.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
   dsa.stencil[0].valuemask = 0xff;
            ctx->dsa_replicate_stencil_bit[i] =
      }
      }
      /**
   * Performs a series of draws to implement stencil blits texture without
   * requiring stencil writes, updating a single bit per pixel at the time.
   */
   void
   util_blitter_stencil_fallback(struct blitter_context *blitter,
                                 struct pipe_resource *dst,
   unsigned dst_level,
   {
      struct blitter_context_priv *ctx = (struct blitter_context_priv *)blitter;
            /* check the saved state */
   util_blitter_set_running_flag(blitter);
   blitter_check_saved_vertex_states(ctx);
   blitter_check_saved_fragment_states(ctx);
   blitter_check_saved_fb_state(ctx);
            /* Initialize the surface. */
   struct pipe_surface *dst_view, dst_templ;
   util_blitter_default_dst_texture(&dst_templ, dst, dst_level, dstbox->z);
            /* Initialize the sampler view. */
   struct pipe_sampler_view src_templ, *src_view;
   util_blitter_default_src_texture(blitter, &src_templ, src, src_level);
   src_templ.format = util_format_stencil_only(src_templ.format);
            /* bind states */
   pipe->bind_blend_state(pipe, ctx->blend[PIPE_MASK_RGBA][0]);
   pipe->bind_fs_state(pipe,
            /* set a framebuffer state */
   struct pipe_framebuffer_state fb_state = { 0 };
   fb_state.width = dstbox->x + dstbox->width;
   fb_state.height = dstbox->y + dstbox->height;
   fb_state.zsbuf = dst_view;
   fb_state.resolve = NULL;
   pipe->set_framebuffer_state(pipe, &fb_state);
   pipe->set_sample_mask(pipe, ~0);
   if (pipe->set_min_samples)
            blitter_set_common_draw_rect_state(ctx, scissor != NULL,
                  if (scissor) {
      pipe->clear_depth_stencil(pipe, dst_view, PIPE_CLEAR_STENCIL, 0.0, 0,
                           MAX2(dstbox->x, scissor->minx),
      } else {
      pipe->clear_depth_stencil(pipe, dst_view, PIPE_CLEAR_STENCIL, 0.0, 0,
                           pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0, 1, 0, false, &src_view);
            unsigned stencil_bits =
      util_format_get_component_bits(dst->format,
         struct pipe_stencil_ref sr = { { (1u << stencil_bits) - 1 } };
            union blitter_attrib coord;
   get_texcoords(src_view, src->width0, src->height0,
               srcbox->x, srcbox->y,
            for (int i = 0; i < stencil_bits; ++i) {
      uint32_t mask = 1 << i;
   struct pipe_constant_buffer cb = {
      .user_buffer = &mask,
      };
   pipe->set_constant_buffer(pipe, PIPE_SHADER_FRAGMENT, blitter->cb_slot,
            pipe->bind_depth_stencil_alpha_state(pipe,
            blitter->draw_rectangle(blitter, ctx->velem_state,
                           get_vs_passthrough_pos_generic,
   dstbox->x, dstbox->y,
               if (scissor)
            util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_fragment_states(blitter);
   util_blitter_restore_textures_internal(blitter, 1);
   util_blitter_restore_fb_state(blitter);
   util_blitter_restore_render_cond(blitter);
   util_blitter_restore_constant_buffer_state(blitter);
            pipe_surface_reference(&dst_view, NULL);
      }
