   /*
   * Copyright © 2015 Broadcom
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
      #include "util/format/u_format.h"
   #include "util/u_surface.h"
   #include "util/u_blitter.h"
   #include "compiler/nir/nir_builder.h"
   #include "vc4_context.h"
      static struct pipe_surface *
   vc4_get_blit_surface(struct pipe_context *pctx,
               {
                  memset(&tmpl, 0, sizeof(tmpl));
   tmpl.format = prsc->format;
   tmpl.u.tex.level = level;
            }
      static bool
   is_tile_unaligned(unsigned size, unsigned tile_size)
   {
         }
      static void
   vc4_tile_blit(struct pipe_context *pctx, struct pipe_blit_info *info)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
   bool msaa = (info->src.resource->nr_samples > 1 ||
         int tile_width = msaa ? 32 : 64;
            if (!info->mask)
            bool is_color_blit = info->mask & PIPE_MASK_RGBA;
   bool is_depth_blit = info->mask & PIPE_MASK_Z;
            /* Either we receive a depth/stencil blit, or color blit, but not both.
         assert ((is_color_blit && !(is_depth_blit || is_stencil_blit)) ||
            if (info->scissor_enable)
            if (info->dst.box.x != info->src.box.x ||
         info->dst.box.y != info->src.box.y ||
   info->dst.box.width != info->src.box.width ||
   info->dst.box.height != info->src.box.height ||
   info->dst.box.depth != info->src.box.depth ||
   info->dst.box.depth != 1) {
            if (is_color_blit &&
                  if ((is_depth_blit || is_stencil_blit) &&
                  int dst_surface_width = u_minify(info->dst.resource->width0,
         int dst_surface_height = u_minify(info->dst.resource->height0,
         if (is_tile_unaligned(info->dst.box.x, tile_width) ||
         is_tile_unaligned(info->dst.box.y, tile_height) ||
   (is_tile_unaligned(info->dst.box.width, tile_width) &&
   info->dst.box.x + info->dst.box.width != dst_surface_width) ||
   (is_tile_unaligned(info->dst.box.height, tile_height) &&
   info->dst.box.y + info->dst.box.height != dst_surface_height)) {
            /* VC4_PACKET_LOAD_TILE_BUFFER_GENERAL uses the
      * VC4_PACKET_TILE_RENDERING_MODE_CONFIG's width (determined by our
   * destination surface) to determine the stride.  This may be wrong
   * when reading from texture miplevels > 0, which are stored in
   * POT-sized areas.  For MSAA, the tile addresses are computed
   * explicitly by the RCL, but still use the destination width to
   * determine the stride (which could be fixed by explicitly supplying
   * it in the ABI).
                        if (info->src.resource->nr_samples > 1)
         else if (rsc->slices[info->src.level].tiling == VC4_TILING_FORMAT_T)
         else
            if (stride != rsc->slices[info->src.level].stride)
            if (info->dst.resource->format != info->src.resource->format)
            if (false) {
            fprintf(stderr, "RCL blit from %d,%d to %d,%d (%d,%d)\n",
            info->src.box.x,
   info->src.box.y,
   info->dst.box.x,
            struct pipe_surface *dst_surf =
               struct pipe_surface *src_surf =
                           struct vc4_job *job;
   if (is_color_blit) {
               } else {
                        job->draw_min_x = info->dst.box.x;
   job->draw_min_y = info->dst.box.y;
   job->draw_max_x = info->dst.box.x + info->dst.box.width;
   job->draw_max_y = info->dst.box.y + info->dst.box.height;
   job->draw_width = dst_surf->width;
            job->tile_width = tile_width;
   job->tile_height = tile_height;
   job->msaa = msaa;
            if (is_color_blit) {
                        if (is_depth_blit) {
                        if (is_stencil_blit) {
                                 pipe_surface_reference(&dst_surf, NULL);
   }
      void
   vc4_blitter_save(struct vc4_context *vc4)
   {
         util_blitter_save_fragment_constant_buffer_slot(vc4->blitter,
         util_blitter_save_vertex_buffer_slot(vc4->blitter, vc4->vertexbuf.vb);
   util_blitter_save_vertex_elements(vc4->blitter, vc4->vtx);
   util_blitter_save_vertex_shader(vc4->blitter, vc4->prog.bind_vs);
   util_blitter_save_rasterizer(vc4->blitter, vc4->rasterizer);
   util_blitter_save_viewport(vc4->blitter, &vc4->viewport);
   util_blitter_save_scissor(vc4->blitter, &vc4->scissor);
   util_blitter_save_fragment_shader(vc4->blitter, vc4->prog.bind_fs);
   util_blitter_save_blend(vc4->blitter, vc4->blend);
   util_blitter_save_depth_stencil_alpha(vc4->blitter, vc4->zsa);
   util_blitter_save_stencil_ref(vc4->blitter, &vc4->stencil_ref);
   util_blitter_save_sample_mask(vc4->blitter, vc4->sample_mask, 0);
   util_blitter_save_framebuffer(vc4->blitter, &vc4->framebuffer);
   util_blitter_save_fragment_sampler_states(vc4->blitter,
               util_blitter_save_fragment_sampler_views(vc4->blitter,
   }
      static void *vc4_get_yuv_vs(struct pipe_context *pctx)
   {
      struct vc4_context *vc4 = vc4_context(pctx);
            if (vc4->yuv_linear_blit_vs)
            const struct nir_shader_compiler_options *options =
         pscreen->get_compiler_options(pscreen,
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_VERTEX, options,
            const struct glsl_type *vec4 = glsl_vec4_type();
   nir_variable *pos_in = nir_variable_create(b.shader, nir_var_shader_in,
            nir_variable *pos_out = nir_variable_create(b.shader, nir_var_shader_out,
                           struct pipe_shader_state shader_tmpl = {
         .type = PIPE_SHADER_IR_NIR,
                        }
      static void *vc4_get_yuv_fs(struct pipe_context *pctx, int cpp)
   {
      struct vc4_context *vc4 = vc4_context(pctx);
   struct pipe_screen *pscreen = pctx->screen;
   struct pipe_shader_state **cached_shader;
            if (cpp == 1) {
         cached_shader = &vc4->yuv_linear_blit_fs_8bit;
   } else {
         cached_shader = &vc4->yuv_linear_blit_fs_16bit;
            if (*cached_shader)
            const struct nir_shader_compiler_options *options =
         pscreen->get_compiler_options(pscreen,
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT,
            const struct glsl_type *vec4 = glsl_vec4_type();
            nir_variable *color_out = nir_variable_create(b.shader, nir_var_shader_out,
                  nir_variable *pos_in = nir_variable_create(b.shader, nir_var_shader_in,
         pos_in->data.location = VARYING_SLOT_POS;
            nir_def *one = nir_imm_int(&b, 1);
            nir_def *x = nir_f2i32(&b, nir_channel(&b, pos, 0));
            nir_variable *stride_in = nir_variable_create(b.shader, nir_var_uniform,
                  nir_def *x_offset;
   nir_def *y_offset;
   if (cpp == 1) {
         nir_def *intra_utile_x_offset =
                        x_offset = nir_iadd(&b,
               y_offset = nir_imul(&b,
                     } else {
         x_offset = nir_ishl(&b, x, two);
            nir_def *load =
      nir_load_ubo(&b, 1, 32, one, nir_iadd(&b, x_offset, y_offset),
               .align_mul = 4,
         nir_store_var(&b, color_out,
                  struct pipe_shader_state shader_tmpl = {
         .type = PIPE_SHADER_IR_NIR,
                        }
      static void
   vc4_yuv_blit(struct pipe_context *pctx, struct pipe_blit_info *info)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
   struct vc4_resource *src = vc4_resource(info->src.resource);
   struct vc4_resource *dst = vc4_resource(info->dst.resource);
            if (!(info->mask & PIPE_MASK_RGBA))
            if (src->tiled)
            if (src->base.format != PIPE_FORMAT_R8_UNORM &&
                  /* YUV blits always turn raster-order to tiled */
   assert(dst->base.format == src->base.format);
            /* Always 1:1 and at the origin */
   assert(info->src.box.x == 0 && info->dst.box.x == 0);
   assert(info->src.box.y == 0 && info->dst.box.y == 0);
   assert(info->src.box.width == info->dst.box.width);
            if ((src->slices[info->src.level].offset & 3) ||
         (src->slices[info->src.level].stride & 3)) {
      perf_debug("YUV-blit src texture offset/stride misaligned: 0x%08x/%d\n",
                              /* Create a renderable surface mapping the T-tiled shadow buffer.
         struct pipe_surface dst_tmpl;
   util_blitter_default_dst_texture(&dst_tmpl, info->dst.resource,
         dst_tmpl.format = PIPE_FORMAT_RGBA8888_UNORM;
   struct pipe_surface *dst_surf =
         if (!dst_surf) {
            fprintf(stderr, "Failed to create YUV dst surface\n");
      }
   dst_surf->width = align(dst_surf->width, 8) / 2;
   if (dst->cpp == 1)
            /* Set the constant buffer. */
   uint32_t stride = src->slices[info->src.level].stride;
   struct pipe_constant_buffer cb_uniforms = {
               };
   pctx->set_constant_buffer(pctx, PIPE_SHADER_FRAGMENT, 0, false, &cb_uniforms);
   struct pipe_constant_buffer cb_src = {
            .buffer = info->src.resource,
   .buffer_offset = src->slices[info->src.level].offset,
      };
            /* Unbind the textures, to make sure we don't try to recurse into the
      * shadow blit.
      pctx->set_sampler_views(pctx, PIPE_SHADER_FRAGMENT, 0, 0, 0, false, NULL);
            util_blitter_custom_shader(vc4->blitter, dst_surf,
                  util_blitter_restore_textures(vc4->blitter);
   util_blitter_restore_constant_buffer_state(vc4->blitter);
   /* Restore cb1 (util_blitter doesn't handle this one). */
   struct pipe_constant_buffer cb_disabled = { 0 };
                                 fallback:
         /* Do an immediate SW fallback, since the render blit path
      * would just recurse.
      ok = util_try_blit_via_copy_region(pctx, info, false);
            }
      static void
   vc4_render_blit(struct pipe_context *ctx, struct pipe_blit_info *info)
   {
                  if (!info->mask)
            if (!util_blitter_is_blit_supported(vc4->blitter, info)) {
            fprintf(stderr, "blit unsupported %s -> %s\n",
      util_format_short_name(info->src.resource->format),
            /* Enable the scissor, so we get a minimal set of tiles rendered. */
   if (!info->scissor_enable) {
            info->scissor_enable = true;
   info->scissor.minx = info->dst.box.x;
   info->scissor.miny = info->dst.box.y;
               vc4_blitter_save(vc4);
            }
      /* Implement stencil and stencil/depth blit by reinterpreting stencil data as
   * an RGBA8888 texture.
   */
   static void
   vc4_stencil_blit(struct pipe_context *ctx, struct pipe_blit_info *info)
   {
         struct vc4_context *vc4 = vc4_context(ctx);
   struct vc4_resource *src = vc4_resource(info->src.resource);
   struct vc4_resource *dst = vc4_resource(info->dst.resource);
            if ((info->mask & PIPE_MASK_S) == 0)
            src_format = (info->mask & PIPE_MASK_ZS) ?
                  dst_format = (info->mask & PIPE_MASK_ZS) ?
                  /* Initialize the surface */
   struct pipe_surface dst_tmpl = {
            .u.tex = {
            .level = info->dst.level,
         };
   struct pipe_surface *dst_surf =
            /* Initialize the sampler view */
   struct pipe_sampler_view src_tmpl = {
            .target = (src->base.target == PIPE_TEXTURE_CUBE_ARRAY) ?
               .format = src_format,
   .u.tex =  {
            .first_level = info->src.level,
   .last_level = info->src.level,
   .first_layer = 0,
   .last_layer = (PIPE_TEXTURE_2D ?
            },
   .swizzle_r = PIPE_SWIZZLE_X,
   .swizzle_g = PIPE_SWIZZLE_Y,
      };
   struct pipe_sampler_view *src_view =
            vc4_blitter_save(vc4);
   util_blitter_blit_generic(vc4->blitter, dst_surf, &info->dst.box,
                              src_view, &info->src.box,
               pipe_surface_reference(&dst_surf, NULL);
            }
      /* Optimal hardware path for blitting pixels.
   * Scaling, format conversion, up- and downsampling (resolve) are allowed.
   */
   void
   vc4_blit(struct pipe_context *pctx, const struct pipe_blit_info *blit_info)
   {
                                    if (info.mask &&
                                    if (info.mask)
   }
