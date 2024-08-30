   /*
   * Copyright © 2015-2017 Broadcom
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
   #include "v3d_context.h"
   #include "broadcom/common/v3d_tiling.h"
   #include "broadcom/common/v3d_tfu.h"
      /**
   * The param @op_blit is used to tell if we are saving state for blitter_blit
   * (if true) or blitter_clear (if false). If other blitter functions are used
   * that require different state we may need something more elaborated than
   * this.
   */
      void
   v3d_blitter_save(struct v3d_context *v3d, bool op_blit, bool render_cond)
   {
         util_blitter_save_fragment_constant_buffer_slot(v3d->blitter,
         util_blitter_save_vertex_buffer_slot(v3d->blitter, v3d->vertexbuf.vb);
   util_blitter_save_vertex_elements(v3d->blitter, v3d->vtx);
   util_blitter_save_vertex_shader(v3d->blitter, v3d->prog.bind_vs);
   util_blitter_save_geometry_shader(v3d->blitter, v3d->prog.bind_gs);
   util_blitter_save_so_targets(v3d->blitter, v3d->streamout.num_targets,
         util_blitter_save_rasterizer(v3d->blitter, v3d->rasterizer);
   util_blitter_save_viewport(v3d->blitter, &v3d->viewport);
   util_blitter_save_fragment_shader(v3d->blitter, v3d->prog.bind_fs);
   util_blitter_save_blend(v3d->blitter, v3d->blend);
   util_blitter_save_depth_stencil_alpha(v3d->blitter, v3d->zsa);
   util_blitter_save_stencil_ref(v3d->blitter, &v3d->stencil_ref);
   util_blitter_save_sample_mask(v3d->blitter, v3d->sample_mask, 0);
   util_blitter_save_so_targets(v3d->blitter, v3d->streamout.num_targets,
                  if (op_blit) {
            util_blitter_save_scissor(v3d->blitter, &v3d->scissor);
   util_blitter_save_fragment_sampler_states(v3d->blitter,
               util_blitter_save_fragment_sampler_views(v3d->blitter,
               if (!render_cond) {
               }
      static void
   v3d_render_blit(struct pipe_context *ctx, struct pipe_blit_info *info)
   {
         struct v3d_context *v3d = v3d_context(ctx);
   struct v3d_resource *src = v3d_resource(info->src.resource);
            if (!info->mask)
            if (!src->tiled &&
         info->src.resource->target != PIPE_TEXTURE_1D &&
   info->src.resource->target != PIPE_TEXTURE_1D_ARRAY) {
      struct pipe_box box = {
            .x = 0,
   .y = 0,
   .width = u_minify(info->src.resource->width0,
         .height = u_minify(info->src.resource->height0,
      };
   struct pipe_resource tmpl = {
            .target = info->src.resource->target,
   .format = info->src.resource->format,
   .width0 = box.width,
   .height0 = box.height,
      };
   tiled = ctx->screen->resource_create(ctx->screen, &tmpl);
   if (!tiled) {
               }
   ctx->resource_copy_region(ctx,
                                       if (!util_blitter_is_blit_supported(v3d->blitter, info)) {
            fprintf(stderr, "blit unsupported %s -> %s\n",
      util_format_short_name(info->src.format),
            v3d_blitter_save(v3d, true, info->render_condition_enable);
            pipe_resource_reference(&tiled, NULL);
   }
      /* Implement stencil blits by reinterpreting the stencil data as an RGBA8888
   * or R8 texture.
   */
   static void
   v3d_stencil_blit(struct pipe_context *ctx, struct pipe_blit_info *info)
   {
         struct v3d_context *v3d = v3d_context(ctx);
   struct v3d_resource *src = v3d_resource(info->src.resource);
   struct v3d_resource *dst = v3d_resource(info->dst.resource);
            if ((info->mask & PIPE_MASK_S) == 0)
            if (src->separate_stencil) {
               } else {
                  if (dst->separate_stencil) {
               } else {
                  /* Initialize the surface. */
   struct pipe_surface dst_tmpl = {
            .u.tex = {
            .level = info->dst.level,
         };
   struct pipe_surface *dst_surf =
            /* Initialize the sampler view. */
   struct pipe_sampler_view src_tmpl = {
            .target = (src->base.target == PIPE_TEXTURE_CUBE_ARRAY) ?
               .format = src_format,
   .u.tex = {
            .first_level = info->src.level,
   .last_level = info->src.level,
   .first_layer = 0,
   .last_layer = (PIPE_TEXTURE_3D ?
            },
   .swizzle_r = PIPE_SWIZZLE_X,
   .swizzle_g = PIPE_SWIZZLE_Y,
      };
   struct pipe_sampler_view *src_view =
            v3d_blitter_save(v3d, true, info->render_condition_enable);
   util_blitter_blit_generic(v3d->blitter, dst_surf, &info->dst.box,
                                          pipe_surface_reference(&dst_surf, NULL);
            }
      bool
   v3d_generate_mipmap(struct pipe_context *pctx,
                     struct pipe_resource *prsc,
   enum pipe_format format,
   unsigned int base_level,
   {
         if (format != prsc->format)
            /* We could maybe support looping over layers for array textures, but
      * we definitely don't support 3D.
      if (first_layer != last_layer)
            struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_screen *screen = v3d->screen;
            return v3d_X(devinfo, tfu)(pctx,
                           }
      static void
   v3d_tfu_blit(struct pipe_context *pctx, struct pipe_blit_info *info)
   {
         int dst_width = u_minify(info->dst.resource->width0, info->dst.level);
            if ((info->mask & PIPE_MASK_RGBA) == 0)
            if (info->scissor_enable ||
         info->dst.box.x != 0 ||
   info->dst.box.y != 0 ||
   info->dst.box.width != dst_width ||
   info->dst.box.height != dst_height ||
   info->dst.box.depth != 1 ||
   info->src.box.x != 0 ||
   info->src.box.y != 0 ||
   info->src.box.width != info->dst.box.width ||
   info->src.box.height != info->dst.box.height ||
   info->src.box.depth != 1) {
            if (info->dst.format != info->src.format)
            struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_screen *screen = v3d->screen;
            if (v3d_X(devinfo, tfu)(pctx, info->dst.resource, info->src.resource,
                           info->src.level,
   }
      static struct pipe_surface *
   v3d_get_blit_surface(struct pipe_context *pctx,
                           {
                  tmpl.format = format;
   tmpl.u.tex.level = level;
   tmpl.u.tex.first_layer = layer;
            }
      static bool
   is_tile_unaligned(unsigned size, unsigned tile_size)
   {
         }
      static void
   v3d_tlb_blit(struct pipe_context *pctx, struct pipe_blit_info *info)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_screen *screen = v3d->screen;
            if (devinfo->ver < 40 || !info->mask)
            bool is_color_blit = info->mask & PIPE_MASK_RGBA;
   bool is_depth_blit = info->mask & PIPE_MASK_Z;
            /* We should receive either a depth/stencil blit, or color blit, but
      * not both.
      assert ((is_color_blit && !is_depth_blit && !is_stencil_blit) ||
            if (info->scissor_enable)
            if (info->src.box.x != info->dst.box.x ||
         info->src.box.y != info->dst.box.y ||
   info->src.box.width != info->dst.box.width ||
            if (is_color_blit &&
                  if ((is_depth_blit || is_stencil_blit) &&
                  if (!v3d_rt_format_supported(devinfo, info->src.format))
            if (v3d_get_rt_format(devinfo, info->src.format) !=
                  bool msaa = (info->src.resource->nr_samples > 1 ||
         bool is_msaa_resolve = (info->src.resource->nr_samples > 1 &&
            if (is_msaa_resolve &&
                           struct pipe_surface *dst_surf =
         struct pipe_surface *src_surf =
            struct pipe_surface *surfaces[V3D_MAX_DRAW_BUFFERS] = { 0 };
   if (is_color_blit)
                     uint32_t tile_width, tile_height, max_bpp;
   v3d_get_tile_buffer_size(devinfo, msaa, double_buffer,
                  int dst_surface_width = u_minify(info->dst.resource->width0,
         int dst_surface_height = u_minify(info->dst.resource->height0,
         if (is_tile_unaligned(info->dst.box.x, tile_width) ||
         is_tile_unaligned(info->dst.box.y, tile_height) ||
   (is_tile_unaligned(info->dst.box.width, tile_width) &&
   info->dst.box.x + info->dst.box.width != dst_surface_width) ||
   (is_tile_unaligned(info->dst.box.height, tile_height) &&
   info->dst.box.y + info->dst.box.height != dst_surface_height)) {
      pipe_surface_reference(&dst_surf, NULL);
               struct v3d_job *job = v3d_get_job(v3d,
                           job->msaa = msaa;
   job->double_buffer = double_buffer;
   job->tile_width = tile_width;
   job->tile_height = tile_height;
   job->internal_bpp = max_bpp;
   job->draw_min_x = info->dst.box.x;
   job->draw_min_y = info->dst.box.y;
   job->draw_max_x = info->dst.box.x + info->dst.box.width;
   job->draw_max_y = info->dst.box.y + info->dst.box.height;
            /* The simulator complains if we do a TLB load from a source with a
      * stride that is smaller than the destination's, so we program the
   * 'frame region' to match the smallest dimensions of the two surfaces.
   * This should be fine because we only get here if the src and dst boxes
   * match, so we know the blit involves the same tiles on both surfaces.
      job->draw_width = MIN2(dst_surf->width, src_surf->width);
   job->draw_height = MIN2(dst_surf->height, src_surf->height);
   job->draw_tiles_x = DIV_ROUND_UP(job->draw_width,
         job->draw_tiles_y = DIV_ROUND_UP(job->draw_height,
            job->needs_flush = true;
            job->store = 0;
   if (is_color_blit) {
               }
   if (is_depth_blit) {
               }
   if (is_stencil_blit){
                                          pipe_surface_reference(&dst_surf, NULL);
   }
      /**
   * Creates the VS of the custom blit shader to convert YUV plane from
   * the NV12 format with BROADCOM_SAND_COL128 modifier to UIF tiled format.
   * This vertex shader is mostly a pass-through VS.
   */
   static void *
   v3d_get_sand8_vs(struct pipe_context *pctx)
   {
         struct v3d_context *v3d = v3d_context(pctx);
            if (v3d->sand8_blit_vs)
            const struct nir_shader_compiler_options *options =
                        nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_VERTEX,
                  const struct glsl_type *vec4 = glsl_vec4_type();
   nir_variable *pos_in = nir_variable_create(b.shader,
                  nir_variable *pos_out = nir_variable_create(b.shader,
               pos_out->data.location = VARYING_SLOT_POS;
            struct pipe_shader_state shader_tmpl = {
                                 }
   /**
   * Creates the FS of the custom blit shader to convert YUV plane from
   * the NV12 format with BROADCOM_SAND_COL128 modifier to UIF tiled format.
   * The result texture is equivalent to a chroma (cpp=2) or luma (cpp=1)
   * plane for a NV12 format without the SAND modifier.
   */
   static void *
   v3d_get_sand8_fs(struct pipe_context *pctx, int cpp)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   struct pipe_screen *pscreen = pctx->screen;
   struct pipe_shader_state **cached_shader;
            if (cpp == 1) {
               } else {
                        if (*cached_shader)
            const struct nir_shader_compiler_options *options =
                        nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT,
         b.shader->info.num_ubos = 1;
   b.shader->num_outputs = 1;
   b.shader->num_inputs = 1;
                              nir_variable *color_out =
                        nir_variable *pos_in =
         pos_in->data.location = VARYING_SLOT_POS;
            nir_def *zero = nir_imm_int(&b, 0);
   nir_def *one = nir_imm_int(&b, 1);
   nir_def *two = nir_imm_int(&b, 2);
   nir_def *six = nir_imm_int(&b, 6);
   nir_def *seven = nir_imm_int(&b, 7);
            nir_def *x = nir_f2i32(&b, nir_channel(&b, pos, 0));
            nir_variable *stride_in =
               nir_def *stride =
            nir_load_uniform(&b, 1, 32, zero,
               nir_def *x_offset;
            /* UIF tiled format is composed by UIF blocks, Each block has
      * four 64 byte microtiles. Inside each microtile pixels are stored
   * in raster format. But microtiles have different dimensions
   * based in the bits per pixel of the image.
   *
   *   8bpp microtile dimensions are 8x8
   *  16bpp microtile dimensions are 8x4
   *  32bpp microtile dimensions are 4x4
   *
   * As we are reading and writing with 32bpp to optimize
   * the number of texture operations during the blit, we need
   * to adjust the offsets were we read and write as data will
   * be later read using 8bpp (luma) and 16bpp (chroma).
   *
   * For chroma 8x4 16bpp raster order is compatible with 4x4
   * 32bpp. In both layouts each line has 8*2 == 4*4 == 16 bytes.
   * But luma 8x8 8bpp raster order is not compatible
   * with 4x4 32bpp. 8bpp has 8 bytes per line, and 32bpp has
   * 16 bytes per line. So if we read a 8bpp texture that was
   * written as 32bpp texture. Bytes would be misplaced.
   *
   * inter/intra_utile_x_offsets takes care of mapping the offsets
   * between microtiles to deal with this issue for luma planes.
      if (cpp == 1) {
            nir_def *intra_utile_x_offset =
         nir_def *inter_utile_x_offset =
         nir_def *stripe_offset=
                        x_offset = nir_iadd(&b, stripe_offset,
               y_offset = nir_iadd(&b,
      } else  {
            nir_def *stripe_offset=
            nir_ishl(&b,nir_imul(&b,nir_ishr_imm(&b, x, 5),
      x_offset = nir_iadd(&b, stripe_offset,
      }
   nir_def *ubo_offset = nir_iadd(&b, x_offset, y_offset);
   nir_def *load =
   nir_load_ubo(&b, 1, 32, zero, ubo_offset,
               .align_mul = 4,
                     nir_store_var(&b, color_out,
                  struct pipe_shader_state shader_tmpl = {
                                 }
      /**
   * Turns NV12 with SAND8 format modifier from raster-order with interleaved
   * luma and chroma 128-byte-wide-columns to tiled format for luma and chroma.
   *
   * This implementation is based on vc4_yuv_blit.
   */
   static void
   v3d_sand8_blit(struct pipe_context *pctx, struct pipe_blit_info *info)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_resource *src = v3d_resource(info->src.resource);
            if (!src->sand_col128_stride)
         if (src->tiled)
         if (src->base.format != PIPE_FORMAT_R8_UNORM &&
         src->base.format != PIPE_FORMAT_R8G8_UNORM)
   if (!(info->mask & PIPE_MASK_RGBA))
            assert(dst->base.format == src->base.format);
            assert(info->src.box.x == 0 && info->dst.box.x == 0);
   assert(info->src.box.y == 0 && info->dst.box.y == 0);
   assert(info->src.box.width == info->dst.box.width);
                     struct pipe_surface dst_tmpl;
   util_blitter_default_dst_texture(&dst_tmpl, info->dst.resource,
         /* Although the src textures are cpp=1 or cpp=2, the dst texture
      * uses a cpp=4 dst texture. So, all read/write texture ops will
   * be done using 32-bit read and writes.
      dst_tmpl.format = PIPE_FORMAT_R8G8B8A8_UNORM;
   struct pipe_surface *dst_surf =
         if (!dst_surf) {
            fprintf(stderr, "Failed to create YUV dst surface\n");
                        /* Adjust the dimensions of dst luma/chroma to match src
      * size now we are using a cpp=4 format. Next dimension take into
   * account the UIF microtile layouts.
      dst_surf->width = align(dst_surf->width, 8) / 2;
   if (src->cpp == 1)
            /* Set the constant buffer. */
   struct pipe_constant_buffer cb_uniforms = {
                        pctx->set_constant_buffer(pctx, PIPE_SHADER_FRAGMENT, 0, false,
         struct pipe_constant_buffer saved_fs_cb1 = { 0 };
   pipe_resource_reference(&saved_fs_cb1.buffer,
         memcpy(&saved_fs_cb1, &v3d->constbuf[PIPE_SHADER_FRAGMENT].cb[1],
         struct pipe_constant_buffer cb_src = {
            .buffer = info->src.resource,
   .buffer_offset = src->slices[info->src.level].offset,
      };
   pctx->set_constant_buffer(pctx, PIPE_SHADER_FRAGMENT, 1, false,
         /* Unbind the textures, to make sure we don't try to recurse into the
      * shadow blit.
      pctx->set_sampler_views(pctx, PIPE_SHADER_FRAGMENT, 0, 0, 0, false, NULL);
            util_blitter_custom_shader(v3d->blitter, dst_surf,
                  util_blitter_restore_textures(v3d->blitter);
            /* Restore cb1 (util_blitter doesn't handle this one). */
   pctx->set_constant_buffer(pctx, PIPE_SHADER_FRAGMENT, 1, true,
                     }
         /**
   * Creates the VS of the custom blit shader to convert YUV plane from
   * the P030 format with BROADCOM_SAND_COL128 modifier to UIF tiled P010
   * format.
   * This vertex shader is mostly a pass-through VS.
   */
   static void *
   v3d_get_sand30_vs(struct pipe_context *pctx)
   {
         struct v3d_context *v3d = v3d_context(pctx);
            if (v3d->sand30_blit_vs)
            const struct nir_shader_compiler_options *options =
                        nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_VERTEX,
                  const struct glsl_type *vec4 = glsl_vec4_type();
   nir_variable *pos_in = nir_variable_create(b.shader,
                  nir_variable *pos_out = nir_variable_create(b.shader,
               pos_out->data.location = VARYING_SLOT_POS;
            struct pipe_shader_state shader_tmpl = {
                                 }
      /**
   * Given an uvec2 value with rgb10a2 components, it extracts four 10-bit
   * components, then converts them from unorm10 to unorm16 and returns them
   * in an uvec4. The start parameter defines where the sequence of 4 values
   * begins.
   */
   static nir_def *
   extract_unorm_2xrgb10a2_component_to_4xunorm16(nir_builder *b,
               {
                  nir_def *shiftw0 = nir_imul_imm(b, start, 10);
   nir_def *word0 = nir_iand_imm(b, nir_channel(b, value, 0),
         nir_def *finalword0 = nir_ushr(b, word0, shiftw0);
   nir_def *word1 = nir_channel(b, value, 1);
   nir_def *shiftw0tow1 = nir_isub_imm(b, 30, shiftw0);
   nir_def *word1toword0 =  nir_ishl(b, word1, shiftw0tow1);
   finalword0 = nir_ior(b, finalword0, word1toword0);
            nir_def *val0 = nir_ishl_imm(b, nir_iand_imm(b, finalword0,
         nir_def *val1 = nir_ishr_imm(b, nir_iand_imm(b, finalword0,
         nir_def *val2 = nir_ishr_imm(b, nir_iand_imm(b, finalword0,
         nir_def *val3 = nir_ishl_imm(b, nir_iand_imm(b, finalword1,
            }
      /**
   * Creates the FS of the custom blit shader to convert YUV plane from
   * the P030 format with BROADCOM_SAND_COL128 modifier to UIF tiled P10
   * format a 16-bit representation per component.
   *
   * The result texture is equivalent to a chroma (cpp=4) or luma (cpp=2)
   * plane for a P010 format without the SAND128 modifier.
   */
   static void *
   v3d_get_sand30_fs(struct pipe_context *pctx)
   {
         struct v3d_context *v3d = v3d_context(pctx);
            if (v3d->sand30_blit_fs)
            const struct nir_shader_compiler_options *options =
                        nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT,
               b.shader->info.num_ubos = 1;
   b.shader->num_outputs = 1;
   b.shader->num_inputs = 1;
                     const struct glsl_type *glsl_uint = glsl_uint_type();
   const struct glsl_type *glsl_uvec4 = glsl_vector_type(GLSL_TYPE_UINT,
            nir_variable *color_out = nir_variable_create(b.shader,
                        nir_variable *pos_in =
         pos_in->data.location = VARYING_SLOT_POS;
            nir_def *zero = nir_imm_int(&b, 0);
            /* With a SAND128 stripe, in 128-bytes with rgb10a2 format we have 96
      * 10-bit values. So, it represents 96 pixels for Y plane and 48 pixels
   * for UV frame, but as we are reading 4 10-bit-values at a time we
   * will have 24 groups (pixels) of 4 10-bit values.
               nir_def *x = nir_f2i32(&b, nir_channel(&b, pos, 0));
            /* UIF tiled format is composed by UIF blocks. Each block has four 64
      * byte microtiles. Inside each microtile pixels are stored in raster
   * format. But microtiles have different dimensions based in the bits
   * per pixel of the image.
   *
   *  16bpp microtile dimensions are 8x4
   *  32bpp microtile dimensions are 4x4
   *  64bpp microtile dimensions are 4x2
   *
   * As we are reading and writing with 64bpp to optimize the number of
   * texture operations during the blit, we adjust the offsets so when
   * the microtile is sampled using the 16bpp (luma) and the 32bpp
   * (chroma) the expected pixels are in the correct position, that
   * would be different if we were using a 64bpp sampling.
   *
   * For luma 8x4 16bpp and chroma 4x4 32bpp luma raster order is
   * incompatible with 4x2 64bpp. 16bpp has 16 bytes per line, 32bpp has
   * also 16byte per line. But 64bpp has 32 bytes per line. So if we
   * read a 16bpp or 32bpp texture that was written as 64bpp texture,
   * pixels would be misplaced.
   *
   * inter/intra_utile_x_offsets takes care of mapping the offsets
   * between microtiles to deal with this issue for luma and chroma
   * planes.
   *
   * We reduce the luma and chroma planes to the same blit case
   * because 16bpp and 32bpp have compatible microtile raster layout.
   * So just doubling the width of the chroma plane before calling the
   * blit makes them equivalent.
      nir_variable *stride_in =
               nir_def *stride =
            nir_load_uniform(&b, 1, 32, zero,
               nir_def *real_x = nir_ior(&b, nir_iand_imm(&b, x, 1),
               nir_def *x_pos_in_stripe = nir_umod_imm(&b, real_x, pixels_stripe);
   nir_def *component = nir_umod(&b, real_x, three);
            nir_def *inter_utile_x_offset =
            nir_def *stripe_offset=
            nir_ishl_imm(&b,
                     nir_def *x_offset = nir_iadd(&b, stripe_offset,
               nir_def *y_offset =
                        nir_def *load = nir_load_ubo(&b, 2, 32, zero, ubo_offset,
                           nir_def *output =
               nir_store_var(&b, color_out,
               struct pipe_shader_state shader_tmpl = {
                                 }
      /**
   * Turns P030 with SAND30 format modifier from raster-order with interleaved
   * luma and chroma 128-byte-wide-columns to a P010 UIF tiled format for luma
   * and chroma.
   */
   static void
   v3d_sand30_blit(struct pipe_context *pctx, struct pipe_blit_info *info)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_resource *src = v3d_resource(info->src.resource);
            if (!src->sand_col128_stride)
         if (src->tiled)
         if (src->base.format != PIPE_FORMAT_R16_UNORM &&
         src->base.format != PIPE_FORMAT_R16G16_UNORM)
   if (!(info->mask & PIPE_MASK_RGBA))
            assert(dst->base.format == src->base.format);
            assert(info->src.box.x == 0 && info->dst.box.x == 0);
   assert(info->src.box.y == 0 && info->dst.box.y == 0);
   assert(info->src.box.width == info->dst.box.width);
                     struct pipe_surface dst_tmpl;
   util_blitter_default_dst_texture(&dst_tmpl, info->dst.resource,
                     struct pipe_surface *dst_surf =
         if (!dst_surf) {
            fprintf(stderr, "Failed to create YUV dst surface\n");
                        /* Adjust the dimensions of dst luma/chroma to match src
      * size now we are using a cpp=8 format. Next dimension take into
   * account the UIF microtile layouts.
      dst_surf->height /= 2;
   dst_surf->width = align(dst_surf->width, 8);
   if (src->cpp == 2)
         /* Set the constant buffer. */
   struct pipe_constant_buffer cb_uniforms = {
                        pctx->set_constant_buffer(pctx, PIPE_SHADER_FRAGMENT, 0, false,
            struct pipe_constant_buffer saved_fs_cb1 = { 0 };
   pipe_resource_reference(&saved_fs_cb1.buffer,
         memcpy(&saved_fs_cb1, &v3d->constbuf[PIPE_SHADER_FRAGMENT].cb[1],
         struct pipe_constant_buffer cb_src = {
            .buffer = info->src.resource,
   .buffer_offset = src->slices[info->src.level].offset,
      };
   pctx->set_constant_buffer(pctx, PIPE_SHADER_FRAGMENT, 1, false,
         /* Unbind the textures, to make sure we don't try to recurse into the
      * shadow blit.
      pctx->set_sampler_views(pctx, PIPE_SHADER_FRAGMENT, 0, 0, 0, false,
                  util_blitter_custom_shader(v3d->blitter, dst_surf,
                  util_blitter_restore_textures(v3d->blitter);
            /* Restore cb1 (util_blitter doesn't handle this one). */
   pctx->set_constant_buffer(pctx, PIPE_SHADER_FRAGMENT, 1, true,
                  info->mask &= ~PIPE_MASK_RGBA;
   }
      /* Optimal hardware path for blitting pixels.
   * Scaling, format conversion, up- and downsampling (resolve) are allowed.
   */
   void
   v3d_blit(struct pipe_context *pctx, const struct pipe_blit_info *blit_info)
   {
         struct v3d_context *v3d = v3d_context(pctx);
            if (info.render_condition_enable && !v3d_render_condition_check(v3d))
                                                                  /* Flush our blit jobs immediately.  They're unlikely to get reused by
      * normal drawing or other blits, and without flushing we can easily
   * run into unexpected OOMs when blits are used for a large series of
   * texture uploads before using the textures.
      v3d_flush_jobs_writing_resource(v3d, info.dst.resource,
   }
