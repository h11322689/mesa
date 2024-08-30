   /*
   * Copyright (C) 2017 Rob Clark <robclark@freedesktop.org>
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "util/u_blitter.h"
   #include "util/u_surface.h"
      #include "freedreno_blitter.h"
   #include "freedreno_context.h"
   #include "freedreno_fence.h"
   #include "freedreno_resource.h"
      /* generic blit using u_blitter.. slightly modified version of util_blitter_blit
   * which also handles PIPE_BUFFER:
   */
      static void
   default_dst_texture(struct pipe_surface *dst_templ, struct pipe_resource *dst,
         {
      memset(dst_templ, 0, sizeof(*dst_templ));
   dst_templ->u.tex.level = dstlevel;
   dst_templ->u.tex.first_layer = dstz;
      }
      static void
   default_src_texture(struct pipe_sampler_view *src_templ,
         {
      bool cube_as_2darray =
                     if (cube_as_2darray && (src->target == PIPE_TEXTURE_CUBE ||
               else
            if (src->target == PIPE_BUFFER) {
         }
   src_templ->u.tex.first_level = srclevel;
   src_templ->u.tex.last_level = srclevel;
   src_templ->u.tex.first_layer = 0;
   src_templ->u.tex.last_layer = src->target == PIPE_TEXTURE_3D
               src_templ->swizzle_r = PIPE_SWIZZLE_X;
   src_templ->swizzle_g = PIPE_SWIZZLE_Y;
   src_templ->swizzle_b = PIPE_SWIZZLE_Z;
      }
      static void
   fd_blitter_pipe_begin(struct fd_context *ctx, bool render_cond) assert_dt
   {
      util_blitter_save_vertex_buffer_slot(ctx->blitter, ctx->vtx.vertexbuf.vb);
   util_blitter_save_vertex_elements(ctx->blitter, ctx->vtx.vtx);
   util_blitter_save_vertex_shader(ctx->blitter, ctx->prog.vs);
   util_blitter_save_tessctrl_shader(ctx->blitter, ctx->prog.hs);
   util_blitter_save_tesseval_shader(ctx->blitter, ctx->prog.ds);
   util_blitter_save_geometry_shader(ctx->blitter, ctx->prog.gs);
   util_blitter_save_so_targets(ctx->blitter, ctx->streamout.num_targets,
         util_blitter_save_rasterizer(ctx->blitter, ctx->rasterizer);
   util_blitter_save_viewport(ctx->blitter, &ctx->viewport[0]);
   util_blitter_save_scissor(ctx->blitter, &ctx->scissor[0]);
   util_blitter_save_fragment_shader(ctx->blitter, ctx->prog.fs);
   util_blitter_save_blend(ctx->blitter, ctx->blend);
   util_blitter_save_depth_stencil_alpha(ctx->blitter, ctx->zsa);
   util_blitter_save_stencil_ref(ctx->blitter, &ctx->stencil_ref);
   util_blitter_save_sample_mask(ctx->blitter, ctx->sample_mask, ctx->min_samples);
   util_blitter_save_framebuffer(ctx->blitter, &ctx->framebuffer);
   util_blitter_save_fragment_sampler_states(
      ctx->blitter, ctx->tex[PIPE_SHADER_FRAGMENT].num_samplers,
      util_blitter_save_fragment_sampler_views(
      ctx->blitter, ctx->tex[PIPE_SHADER_FRAGMENT].num_textures,
      if (!render_cond)
      util_blitter_save_render_condition(ctx->blitter, ctx->cond_query,
         if (ctx->batch)
      }
      static void
   fd_blitter_pipe_end(struct fd_context *ctx) assert_dt
   {
   }
      bool
   fd_blitter_blit(struct fd_context *ctx, const struct pipe_blit_info *info)
   {
      struct pipe_context *pctx = &ctx->base;
   struct pipe_resource *dst = info->dst.resource;
   struct pipe_resource *src = info->src.resource;
   struct pipe_context *pipe = &ctx->base;
   struct pipe_surface *dst_view, dst_templ;
            /* If the blit is updating the whole contents of the resource,
   * invalidate it so we don't trigger any unnecessary tile loads in the 3D
   * path.
   */
   if (util_blit_covers_whole_resource(info))
            /* The blit format may not match the resource format in this path, so
   * we need to validate that we can use the src/dst resource with the
   * requested format (and uncompress if necessary).  Normally this would
   * happen in ->set_sampler_view(), ->set_framebuffer_state(), etc.  But
   * that would cause recursion back into u_blitter, which ends in tears.
   *
   * To avoid recursion, this needs to be done before util_blitter_save_*()
   */
   if (ctx->validate_format) {
      ctx->validate_format(ctx, fd_resource(dst), info->dst.format);
               if (src == dst)
                              /* Initialize the surface. */
   default_dst_texture(&dst_templ, dst, info->dst.level, info->dst.box.z);
   dst_templ.format = info->dst.format;
            /* Initialize the sampler view. */
   default_src_texture(&src_templ, src, info->src.level);
   src_templ.format = info->src.format;
            /* Copy. */
   util_blitter_blit_generic(
      ctx->blitter, dst_view, &info->dst.box, src_view, &info->src.box,
   src->width0, src->height0, info->mask, info->filter,
         pipe_surface_reference(&dst_view, NULL);
                     /* While this shouldn't technically be necessary, it is required for
   * dEQP-GLES31.functional.stencil_texturing.format.stencil_index8_cube and
   * 2d_array to pass.
   */
            /* The fallback blitter must never fail: */
      }
      /* Generic clear implementation (partially) using u_blitter: */
   void
   fd_blitter_clear(struct pipe_context *pctx, unsigned buffers,
               {
      struct fd_context *ctx = fd_context(pctx);
   struct pipe_framebuffer_state *pfb = &ctx->batch->framebuffer;
            /* Note: don't use discard=true, if there was something to
   * discard, that would have been already handled in fd_clear().
   */
            util_blitter_save_fragment_constant_buffer_slot(
            util_blitter_common_clear_setup(blitter, pfb->width, pfb->height, buffers,
            struct pipe_stencil_ref sr = {.ref_value = {stencil & 0xff}};
            struct pipe_constant_buffer cb = {
      .buffer_size = 16,
      };
            unsigned rs_idx = pfb->samples > 1 ? 1 : 0;
   if (!ctx->clear_rs_state[rs_idx]) {
      const struct pipe_rasterizer_state tmpl = {
      .cull_face = PIPE_FACE_NONE,
   .half_pixel_center = 1,
   .bottom_edge_rule = 1,
   .flatshade = 1,
   .depth_clip_near = 1,
   .depth_clip_far = 1,
      };
      }
            struct pipe_viewport_state vp = {
      .scale = {0.5f * pfb->width, -0.5f * pfb->height, depth},
      };
            pctx->bind_vertex_elements_state(pctx, ctx->solid_vbuf_state.vtx);
   pctx->set_vertex_buffers(pctx, 1, 0, false,
                  if (pfb->layers > 1)
         else
                     /* Clear geom/tess shaders, lest the draw emit code think we are
   * trying to use use them:
   */
   pctx->bind_gs_state(pctx, NULL);
   pctx->bind_tcs_state(pctx, NULL);
            struct pipe_draw_info info = {
      .mode = MESA_PRIM_COUNT, /* maps to DI_PT_RECTLIST */
   .index_bounds_valid = true,
   .max_index = 1,
      };
   struct pipe_draw_start_count_bias draw = {
                           /* We expect that this should not have triggered a change in pfb: */
            util_blitter_restore_constant_buffer_state(blitter);
   util_blitter_restore_vertex_states(blitter);
   util_blitter_restore_fragment_states(blitter);
   util_blitter_restore_textures(blitter);
   util_blitter_restore_fb_state(blitter);
   util_blitter_restore_render_cond(blitter);
               }
      /* Partially generic clear_render_target implementation using u_blitter */
   void
   fd_blitter_clear_render_target(struct pipe_context *pctx, struct pipe_surface *ps,
                     {
               fd_blitter_pipe_begin(ctx, render_condition_enabled);
   util_blitter_clear_render_target(ctx->blitter, ps, color, x, y, w, h);
      }
      /* Partially generic clear_depth_stencil implementation using u_blitter */
   void
   fd_blitter_clear_depth_stencil(struct pipe_context *pctx, struct pipe_surface *ps,
                     {
               fd_blitter_pipe_begin(ctx, render_condition_enabled);
   util_blitter_clear_depth_stencil(ctx->blitter, ps, buffers, depth,
            }
      /**
   * Optimal hardware path for blitting pixels.
   * Scaling, format conversion, up- and downsampling (resolve) are allowed.
   */
   bool
   fd_blit(struct pipe_context *pctx, const struct pipe_blit_info *blit_info)
   {
      struct fd_context *ctx = fd_context(pctx);
            if (info.render_condition_enable && !fd_render_condition_check(pctx))
            if (ctx->blit && ctx->blit(ctx, &info))
            if (info.mask & PIPE_MASK_S) {
      DBG("cannot blit stencil, skipping");
               if (!util_blitter_is_blit_supported(ctx->blitter, &info)) {
      DBG("blit unsupported %s -> %s",
      util_format_short_name(info.src.resource->format),
                     }
      /**
   * _copy_region using pipe (3d engine)
   */
   static bool
   fd_blitter_pipe_copy_region(struct fd_context *ctx, struct pipe_resource *dst,
                           {
      /* not until we allow rendertargets to be buffers */
   if (dst->target == PIPE_BUFFER || src->target == PIPE_BUFFER)
            if (!util_blitter_is_copy_supported(ctx->blitter, dst, src))
            if (src == dst) {
      struct pipe_context *pctx = &ctx->base;
               /* TODO we could invalidate if dst box covers dst level fully. */
   fd_blitter_pipe_begin(ctx, false);
   util_blitter_copy_texture(ctx->blitter, dst, dst_level, dstx, dsty, dstz,
                     }
      /**
   * Copy a block of pixels from one resource to another.
   * The resource must be of the same format.
   */
   void
   fd_resource_copy_region(struct pipe_context *pctx, struct pipe_resource *dst,
                     {
               /* The blitter path handles compressed formats only if src and dst format
   * match, in other cases just fall back to sw:
   */
   if ((src->format != dst->format) &&
      (util_format_is_compressed(src->format) ||
   util_format_is_compressed(dst->format))) {
   perf_debug_ctx(ctx, "copy_region falls back to sw for {%"PRSC_FMT"} to {%"PRSC_FMT"}",
                     if (ctx->blit) {
               memset(&info, 0, sizeof info);
   info.dst.resource = dst;
   info.dst.level = dst_level;
   info.dst.box.x = dstx;
   info.dst.box.y = dsty;
   info.dst.box.z = dstz;
   info.dst.box.width = src_box->width;
   info.dst.box.height = src_box->height;
   assert(info.dst.box.width >= 0);
   assert(info.dst.box.height >= 0);
   info.dst.box.depth = 1;
   info.dst.format = dst->format;
   info.src.resource = src;
   info.src.level = src_level;
   info.src.box = *src_box;
   info.src.format = src->format;
   info.mask = util_format_get_mask(src->format);
   info.filter = PIPE_TEX_FILTER_NEAREST;
            if (ctx->blit(ctx, &info))
               /* try blit on 3d pipe: */
   if (fd_blitter_pipe_copy_region(ctx, dst, dst_level, dstx, dsty, dstz, src,
                     fallback:
      util_resource_copy_region(pctx, dst, dst_level, dstx, dsty, dstz, src,
      }
