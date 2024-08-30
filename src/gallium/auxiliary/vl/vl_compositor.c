   /**************************************************************************
   *
   * Copyright 2009 Younes Manton.
   * All Rights Reserved.
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
      #include "util/u_sampler.h"
      #include "vl_compositor_gfx.h"
   #include "vl_compositor_cs.h"
      static bool
   init_shaders(struct vl_compositor *c)
   {
               if (c->pipe_cs_composit_supported) {
      if (!vl_compositor_cs_init_shaders(c))
         } else if (c->pipe_gfx_supported) {
      c->fs_video_buffer = create_frag_shader_video_buffer(c);
   if (!c->fs_video_buffer) {
      debug_printf("Unable to create YCbCr-to-RGB fragment shader.\n");
               c->fs_weave_rgb = create_frag_shader_weave_rgb(c);
   if (!c->fs_weave_rgb) {
      debug_printf("Unable to create YCbCr-to-RGB weave fragment shader.\n");
               c->fs_yuv.weave.y = create_frag_shader_deint_yuv(c, true, true);
   c->fs_yuv.weave.uv = create_frag_shader_deint_yuv(c, false, true);
   c->fs_yuv.bob.y = create_frag_shader_deint_yuv(c, true, false);
   c->fs_yuv.bob.uv = create_frag_shader_deint_yuv(c, false, false);
   if (!c->fs_yuv.weave.y || !c->fs_yuv.weave.uv ||
      !c->fs_yuv.bob.y || !c->fs_yuv.bob.uv) {
   debug_printf("Unable to create YCbCr i-to-YCbCr p deint fragment shader.\n");
               c->fs_rgb_yuv.y = create_frag_shader_rgb_yuv(c, true);
   c->fs_rgb_yuv.uv = create_frag_shader_rgb_yuv(c, false);
   if (!c->fs_rgb_yuv.y || !c->fs_rgb_yuv.uv) {
      debug_printf("Unable to create RGB-to-YUV fragment shader.\n");
                  if (c->pipe_gfx_supported) {
      c->vs = create_vert_shader(c);
   if (!c->vs) {
      debug_printf("Unable to create vertex shader.\n");
               c->fs_palette.yuv = create_frag_shader_palette(c, true);
   if (!c->fs_palette.yuv) {
      debug_printf("Unable to create YUV-Palette-to-RGB fragment shader.\n");
               c->fs_palette.rgb = create_frag_shader_palette(c, false);
   if (!c->fs_palette.rgb) {
      debug_printf("Unable to create RGB-Palette-to-RGB fragment shader.\n");
               c->fs_rgba = create_frag_shader_rgba(c);
   if (!c->fs_rgba) {
      debug_printf("Unable to create RGB-to-RGB fragment shader.\n");
                     }
      static void cleanup_shaders(struct vl_compositor *c)
   {
               if (c->pipe_cs_composit_supported) {
         } else if (c->pipe_gfx_supported) {
      c->pipe->delete_fs_state(c->pipe, c->fs_video_buffer);
   c->pipe->delete_fs_state(c->pipe, c->fs_weave_rgb);
   c->pipe->delete_fs_state(c->pipe, c->fs_yuv.weave.y);
   c->pipe->delete_fs_state(c->pipe, c->fs_yuv.weave.uv);
   c->pipe->delete_fs_state(c->pipe, c->fs_yuv.bob.y);
   c->pipe->delete_fs_state(c->pipe, c->fs_yuv.bob.uv);
   c->pipe->delete_fs_state(c->pipe, c->fs_rgb_yuv.y);
               if (c->pipe_gfx_supported) {
      c->pipe->delete_vs_state(c->pipe, c->vs);
   c->pipe->delete_fs_state(c->pipe, c->fs_palette.yuv);
   c->pipe->delete_fs_state(c->pipe, c->fs_palette.rgb);
         }
      static bool
   init_pipe_state(struct vl_compositor *c)
   {
      struct pipe_rasterizer_state rast;
   struct pipe_sampler_state sampler;
   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state dsa;
                     c->fb_state.nr_cbufs = 1;
            memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_REPEAT;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
                     sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
            if (c->pipe_gfx_supported) {
         memset(&blend, 0, sizeof blend);
   blend.independent_blend_enable = 0;
   blend.rt[0].blend_enable = 0;
   blend.logicop_enable = 0;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   blend.rt[0].colormask = PIPE_MASK_RGBA;
                  blend.rt[0].blend_enable = 1;
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
                  memset(&rast, 0, sizeof rast);
   rast.flatshade = 0;
   rast.front_ccw = 1;
   rast.cull_face = PIPE_FACE_NONE;
   rast.fill_back = PIPE_POLYGON_MODE_FILL;
   rast.fill_front = PIPE_POLYGON_MODE_FILL;
   rast.scissor = 1;
   rast.line_width = 1;
   rast.point_size_per_vertex = 1;
   rast.offset_units = 1;
   rast.offset_scale = 1;
   rast.half_pixel_center = 1;
   rast.bottom_edge_rule = 1;
                           memset(&dsa, 0, sizeof dsa);
   dsa.depth_enabled = 0;
   dsa.depth_writemask = 0;
   dsa.depth_func = PIPE_FUNC_ALWAYS;
   for (i = 0; i < 2; ++i) {
            dsa.stencil[i].enabled = 0;
   dsa.stencil[i].func = PIPE_FUNC_ALWAYS;
   dsa.stencil[i].fail_op = PIPE_STENCIL_OP_KEEP;
   dsa.stencil[i].zpass_op = PIPE_STENCIL_OP_KEEP;
   dsa.stencil[i].zfail_op = PIPE_STENCIL_OP_KEEP;
      }
   dsa.alpha_enabled = 0;
   dsa.alpha_func = PIPE_FUNC_ALWAYS;
   dsa.alpha_ref_value = 0;
   c->dsa = c->pipe->create_depth_stencil_alpha_state(c->pipe, &dsa);
               }
      static void cleanup_pipe_state(struct vl_compositor *c)
   {
               if (c->pipe_gfx_supported) {
         /* Asserted in softpipe_delete_fs_state() for some reason */
                  c->pipe->delete_depth_stencil_alpha_state(c->pipe, c->dsa);
   c->pipe->delete_blend_state(c->pipe, c->blend_clear);
   c->pipe->delete_blend_state(c->pipe, c->blend_add);
   }
   c->pipe->delete_sampler_state(c->pipe, c->sampler_linear);
      }
      static bool
   init_buffers(struct vl_compositor *c)
   {
      struct pipe_vertex_element vertex_elems[3];
                     /*
   * Create our vertex buffer and vertex buffer elements
   */
   c->vertex_buf.buffer_offset = 0;
   c->vertex_buf.buffer.resource = NULL;
            if (c->pipe_gfx_supported) {
         vertex_elems[0].src_offset = 0;
   vertex_elems[0].src_stride = VL_COMPOSITOR_VB_STRIDE;
   vertex_elems[0].instance_divisor = 0;
   vertex_elems[0].vertex_buffer_index = 0;
   vertex_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;
   vertex_elems[1].src_offset = sizeof(struct vertex2f);
   vertex_elems[1].src_stride = VL_COMPOSITOR_VB_STRIDE;
   vertex_elems[1].instance_divisor = 0;
   vertex_elems[1].vertex_buffer_index = 0;
   vertex_elems[1].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   vertex_elems[2].src_offset = sizeof(struct vertex2f) + sizeof(struct vertex4f);
   vertex_elems[1].src_stride = VL_COMPOSITOR_VB_STRIDE;
   vertex_elems[2].instance_divisor = 0;
   vertex_elems[2].vertex_buffer_index = 0;
   vertex_elems[2].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
               }
      static void
   cleanup_buffers(struct vl_compositor *c)
   {
               if (c->pipe_gfx_supported) {
         }
      }
      static inline struct u_rect
   default_rect(struct vl_compositor_layer *layer)
   {
      struct pipe_resource *res = layer->sampler_views[0]->texture;
   struct u_rect rect = { 0, res->width0, 0, res->height0 * res->array_size };
      }
      static inline struct vertex2f
   calc_topleft(struct vertex2f size, struct u_rect rect)
   {
      struct vertex2f res = { rect.x0 / size.x, rect.y0 / size.y };
      }
      static inline struct vertex2f
   calc_bottomright(struct vertex2f size, struct u_rect rect)
   {
      struct vertex2f res = { rect.x1 / size.x, rect.y1 / size.y };
      }
      static inline void
   calc_src_and_dst(struct vl_compositor_layer *layer, unsigned width, unsigned height,
         {
               layer->src.tl = calc_topleft(size, src);
   layer->src.br = calc_bottomright(size, src);
   layer->dst.tl = calc_topleft(size, dst);
   layer->dst.br = calc_bottomright(size, dst);
   layer->zw.x = 0.0f;
      }
      static void
   set_yuv_layer(struct vl_compositor_state *s, struct vl_compositor *c,
               unsigned layer, struct pipe_video_buffer *buffer,
   {
      struct pipe_sampler_view **sampler_views;
   float half_a_line;
                              s->used_layers |= 1 << layer;
   sampler_views = buffer->get_sampler_view_components(buffer);
   for (i = 0; i < 3; ++i) {
      s->layers[layer].samplers[i] = c->sampler_linear;
               calc_src_and_dst(&s->layers[layer], buffer->width, buffer->height,
                           switch(deinterlace) {
   case VL_COMPOSITOR_BOB_TOP:
      s->layers[layer].zw.x = 0.0f;
   s->layers[layer].src.tl.y += half_a_line;
   s->layers[layer].src.br.y += half_a_line;
   if (c->pipe_gfx_supported)
         if (c->pipe_cs_composit_supported)
               case VL_COMPOSITOR_BOB_BOTTOM:
      s->layers[layer].zw.x = 1.0f;
   s->layers[layer].src.tl.y -= half_a_line;
   s->layers[layer].src.br.y -= half_a_line;
   if (c->pipe_gfx_supported)
         if (c->pipe_cs_composit_supported)
               case VL_COMPOSITOR_NONE:
      if (c->pipe_cs_composit_supported) {
      s->layers[layer].cs = (y) ? c->cs_yuv.progressive.y : c->cs_yuv.progressive.uv;
      }
         default:
      if (c->pipe_gfx_supported)
         if (c->pipe_cs_composit_supported)
               }
      static void
   set_rgb_to_yuv_layer(struct vl_compositor_state *s, struct vl_compositor *c,
               {
                                 if (c->pipe_cs_composit_supported)
         else if (c->pipe_gfx_supported)
            s->layers[layer].samplers[0] = c->sampler_linear;
   s->layers[layer].samplers[1] = NULL;
            pipe_sampler_view_reference(&s->layers[layer].sampler_views[0], v);
   pipe_sampler_view_reference(&s->layers[layer].sampler_views[1], NULL);
            calc_src_and_dst(&s->layers[layer], v->texture->width0, v->texture->height0,
            }
      void
   vl_compositor_reset_dirty_area(struct u_rect *dirty)
   {
               dirty->x0 = dirty->y0 = VL_COMPOSITOR_MIN_DIRTY;
      }
      void
   vl_compositor_set_clear_color(struct vl_compositor_state *s, union pipe_color_union *color)
   {
      assert(s);
               }
      void
   vl_compositor_get_clear_color(struct vl_compositor_state *s, union pipe_color_union *color)
   {
      assert(s);
               }
      void
   vl_compositor_clear_layers(struct vl_compositor_state *s)
   {
               assert(s);
   s->used_layers = 0;
   for ( i = 0; i < VL_COMPOSITOR_MAX_LAYERS; ++i) {
      struct vertex4f v_one = { 1.0f, 1.0f, 1.0f, 1.0f };
   s->layers[i].clearing = i ? false : true;
   s->layers[i].blend = NULL;
   s->layers[i].fs = NULL;
   s->layers[i].cs = NULL;
   s->layers[i].viewport.scale[2] = 1;
   s->layers[i].viewport.translate[2] = 0;
   s->layers[i].viewport.swizzle_x = PIPE_VIEWPORT_SWIZZLE_POSITIVE_X;
   s->layers[i].viewport.swizzle_y = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Y;
   s->layers[i].viewport.swizzle_z = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Z;
   s->layers[i].viewport.swizzle_w = PIPE_VIEWPORT_SWIZZLE_POSITIVE_W;
            for ( j = 0; j < 3; j++)
         for ( j = 0; j < 4; ++j)
         }
      void
   vl_compositor_cleanup(struct vl_compositor *c)
   {
               cleanup_buffers(c);
   cleanup_shaders(c);
      }
      bool
   vl_compositor_set_csc_matrix(struct vl_compositor_state *s,
               {
               memcpy(&s->csc_matrix, matrix, sizeof(vl_csc_matrix));
   s->luma_min = luma_min;
               }
      void
   vl_compositor_set_dst_clip(struct vl_compositor_state *s, struct u_rect *dst_clip)
   {
               s->scissor_valid = dst_clip != NULL;
   if (dst_clip) {
      s->scissor.minx = dst_clip->x0;
   s->scissor.miny = dst_clip->y0;
   s->scissor.maxx = dst_clip->x1;
         }
      void
   vl_compositor_set_layer_blend(struct vl_compositor_state *s,
               {
                        s->layers[layer].clearing = is_clearing;
      }
      void
   vl_compositor_set_layer_dst_area(struct vl_compositor_state *s,
         {
                        s->layers[layer].viewport_valid = dst_area != NULL;
   if (dst_area) {
      s->layers[layer].viewport.scale[0] = dst_area->x1 - dst_area->x0;
   s->layers[layer].viewport.scale[1] = dst_area->y1 - dst_area->y0;
   s->layers[layer].viewport.translate[0] = dst_area->x0;
         }
      void
   vl_compositor_set_buffer_layer(struct vl_compositor_state *s,
                                 struct vl_compositor *c,
   {
      struct pipe_sampler_view **sampler_views;
                              s->used_layers |= 1 << layer;
   sampler_views = buffer->get_sampler_view_components(buffer);
   for (i = 0; i < 3; ++i) {
      s->layers[layer].samplers[i] = c->sampler_linear;
               calc_src_and_dst(&s->layers[layer], buffer->width, buffer->height,
                  if (buffer->interlaced) {
      float half_a_line = 0.5f / s->layers[layer].zw.y;
   switch(deinterlace) {
   case VL_COMPOSITOR_NONE:
   case VL_COMPOSITOR_MOTION_ADAPTIVE:
   case VL_COMPOSITOR_WEAVE:
      if (c->pipe_cs_composit_supported)
         else if (c->pipe_gfx_supported)
               case VL_COMPOSITOR_BOB_TOP:
      s->layers[layer].zw.x = 0.0f;
   s->layers[layer].src.tl.y += half_a_line;
   s->layers[layer].src.br.y += half_a_line;
   if (c->pipe_cs_composit_supported)
         else if (c->pipe_gfx_supported)
               case VL_COMPOSITOR_BOB_BOTTOM:
      s->layers[layer].zw.x = 1.0f;
   s->layers[layer].src.tl.y -= half_a_line;
   s->layers[layer].src.br.y -= half_a_line;
   if (c->pipe_cs_composit_supported)
         else if (c->pipe_gfx_supported)
                  } else {
      if (c->pipe_cs_composit_supported)
         else if (c->pipe_gfx_supported)
         }
      void
   vl_compositor_set_palette_layer(struct vl_compositor_state *s,
                                 struct vl_compositor *c,
   unsigned layer,
   {
                                 s->layers[layer].fs = include_color_conversion ?
            s->layers[layer].samplers[0] = c->sampler_linear;
   s->layers[layer].samplers[1] = c->sampler_nearest;
   s->layers[layer].samplers[2] = NULL;
   pipe_sampler_view_reference(&s->layers[layer].sampler_views[0], indexes);
   pipe_sampler_view_reference(&s->layers[layer].sampler_views[1], palette);
   pipe_sampler_view_reference(&s->layers[layer].sampler_views[2], NULL);
   calc_src_and_dst(&s->layers[layer], indexes->texture->width0, indexes->texture->height0,
            }
      void
   vl_compositor_set_rgba_layer(struct vl_compositor_state *s,
                              struct vl_compositor *c,
      {
                                 s->used_layers |= 1 << layer;
   s->layers[layer].fs = c->fs_rgba;
   s->layers[layer].samplers[0] = c->sampler_linear;
   s->layers[layer].samplers[1] = NULL;
   s->layers[layer].samplers[2] = NULL;
   pipe_sampler_view_reference(&s->layers[layer].sampler_views[0], rgba);
   pipe_sampler_view_reference(&s->layers[layer].sampler_views[1], NULL);
   pipe_sampler_view_reference(&s->layers[layer].sampler_views[2], NULL);
   calc_src_and_dst(&s->layers[layer], rgba->texture->width0, rgba->texture->height0,
                  if (colors)
      for (i = 0; i < 4; ++i)
   }
      void
   vl_compositor_set_layer_rotation(struct vl_compositor_state *s,
               {
      assert(s);
   assert(layer < VL_COMPOSITOR_MAX_LAYERS);
      }
      void
   vl_compositor_yuv_deint_full(struct vl_compositor_state *s,
                              struct vl_compositor *c,
      {
               dst_surfaces = dst->get_surfaces(dst);
            set_yuv_layer(s, c, 0, src, src_rect, NULL, true, deinterlace);
   vl_compositor_set_layer_dst_area(s, 0, dst_rect);
            if (dst_rect) {
      dst_rect->x0 /= 2;
   dst_rect->y0 /= 2;
   dst_rect->x1 /= 2;
               set_yuv_layer(s, c, 0, src, src_rect, NULL, false, deinterlace);
   vl_compositor_set_layer_dst_area(s, 0, dst_rect);
               }
      void
   vl_compositor_convert_rgb_to_yuv(struct vl_compositor_state *s,
                                       {
      struct pipe_sampler_view *sv, sv_templ;
                     memset(&sv_templ, 0, sizeof(sv_templ));
   u_sampler_view_default_template(&sv_templ, src_res, src_res->format);
                     set_rgb_to_yuv_layer(s, c, 0, sv, src_rect, NULL, true);
   vl_compositor_set_layer_dst_area(s, 0, dst_rect);
            if (dst_rect) {
      dst_rect->x0 /= 2;
   dst_rect->y0 /= 2;
   dst_rect->x1 /= 2;
               set_rgb_to_yuv_layer(s, c, 0, sv, src_rect, NULL, false);
   vl_compositor_set_layer_dst_area(s, 0, dst_rect);
   vl_compositor_render(s, c, dst_surfaces[1], NULL, false);
               }
      void
   vl_compositor_render(struct vl_compositor_state *s,
                           {
               if (s->layers->cs)
         else if (s->layers->fs)
         else
      }
      bool
   vl_compositor_init(struct vl_compositor *c, struct pipe_context *pipe)
   {
                        c->pipe_cs_composit_supported = pipe->screen->get_param(pipe->screen, PIPE_CAP_PREFER_COMPUTE_FOR_MULTIMEDIA) &&
                  c->pipe_gfx_supported = pipe->screen->get_param(pipe->screen, PIPE_CAP_GRAPHICS);
                     if (!init_pipe_state(c)) {
                  if (!init_shaders(c)) {
      cleanup_pipe_state(c);
               if (!init_buffers(c)) {
      cleanup_shaders(c);
   cleanup_pipe_state(c);
                  }
      bool
   vl_compositor_init_state(struct vl_compositor_state *s, struct pipe_context *pipe)
   {
                                          s->clear_color.f[0] = s->clear_color.f[1] = 0.0f;
            /*
   * Create our fragment shader's constant buffer
   * Const buffer contains the color conversion matrix and bias vectors
   */
   /* XXX: Create with IMMUTABLE/STATIC... although it does change every once in a long while... */
   s->shader_params = pipe_buffer_create_const0
   (
      pipe->screen,
   PIPE_BIND_CONSTANT_BUFFER,
   PIPE_USAGE_DEFAULT,
               if (!s->shader_params)
                     vl_csc_get_matrix(VL_CSC_COLOR_STANDARD_IDENTITY, NULL, true, &csc_matrix);
   if (!vl_compositor_set_csc_matrix(s, (const vl_csc_matrix *)&csc_matrix, 1.0f, 0.0f))
               }
      void
   vl_compositor_cleanup_state(struct vl_compositor_state *s)
   {
               vl_compositor_clear_layers(s);
      }
