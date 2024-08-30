   /**********************************************************
   * Copyright 2009-2011 VMware, Inc. All rights reserved.
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
   *********************************************************
   * Authors:
   * Zack Rusin <zackr-at-vmware-dot-com>
   */
      #include "xa_context.h"
   #include "xa_priv.h"
   #include <math.h>
   #include "cso_cache/cso_context.h"
   #include "util/u_inlines.h"
   #include "util/u_sampler.h"
   #include "util/u_draw_quad.h"
      #define floatsEqual(x, y) (fabsf(x - y) <= 0.00001f * MIN2(fabsf(x), fabsf(y)))
   #define floatIsZero(x) (floatsEqual((x) + 1.0f, 1.0f))
      #define NUM_COMPONENTS 4
      void
         renderer_set_constants(struct xa_context *r,
            static inline bool
   is_affine(const float *matrix)
   {
         && floatsEqual(matrix[8], 1.0f);
   }
      static inline void
   map_point(const float *mat, float x, float y, float *out_x, float *out_y)
   {
         *out_x = x;
   *out_y = y;
   return;
               *out_x = mat[0] * x + mat[3] * y + mat[6];
   *out_y = mat[1] * x + mat[4] * y + mat[7];
      float w = 1 / (mat[2] * x + mat[5] * y + mat[8]);
      *out_x *= w;
   *out_y *= w;
         }
      static inline void
   renderer_draw(struct xa_context *r)
   {
                  return;
            r->scissor.minx = 0;
   r->scissor.miny = 0;
   r->scissor.maxx = r->dst->tex->width0;
   r->scissor.maxy = r->dst->tex->height0;
                        struct cso_velems_state velems;
   velems.count = r->attrs_per_vertex;
   memcpy(velems.velems, r->velems, sizeof(r->velems[0]) * velems.count);
   for (unsigned i = 0; i < velems.count; i++)
            cso_set_vertex_elements(r->cso, &velems);
   util_draw_user_vertex_buffer(r->cso, r->buffer, MESA_PRIM_QUADS,
                           }
      static inline void
   renderer_draw_conditional(struct xa_context *r, int next_batch)
   {
         (next_batch == 0 && r->buffer_size)) {
   renderer_draw(r);
         }
      void
   renderer_init_state(struct xa_context *r)
   {
      struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_rasterizer_state raster;
            /* set common initial clip state */
   memset(&dsa, 0, sizeof(struct pipe_depth_stencil_alpha_state));
            /* XXX: move to renderer_init_state? */
   memset(&raster, 0, sizeof(struct pipe_rasterizer_state));
   raster.half_pixel_center = 1;
   raster.bottom_edge_rule = 1;
   raster.depth_clip_near = 1;
   raster.depth_clip_far = 1;
   raster.scissor = 1;
            /* vertex elements state */
   memset(&r->velems[0], 0, sizeof(r->velems[0]) * 3);
      r->velems[i].src_offset = i * 4 * sizeof(float);
   r->velems[i].instance_divisor = 0;
   r->velems[i].vertex_buffer_index = 0;
   r->velems[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
         }
      static inline void
   add_vertex_none(struct xa_context *r, float x, float y)
   {
               vertex[0] = x;
   vertex[1] = y;
   vertex[2] = 0.f;		/*z */
               }
      static inline void
   add_vertex_1tex(struct xa_context *r, float x, float y, float s, float t)
   {
               vertex[0] = x;
   vertex[1] = y;
   vertex[2] = 0.f;		/*z */
            vertex[4] = s;		/*s */
   vertex[5] = t;		/*t */
   vertex[6] = 0.f;		/*r */
               }
      static inline void
   add_vertex_2tex(struct xa_context *r,
   float x, float y, float s0, float t0, float s1, float t1)
   {
               vertex[0] = x;
   vertex[1] = y;
   vertex[2] = 0.f;		/*z */
            vertex[4] = s0;		/*s */
   vertex[5] = t0;		/*t */
   vertex[6] = 0.f;		/*r */
            vertex[8] = s1;		/*s */
   vertex[9] = t1;		/*t */
   vertex[10] = 0.f;		/*r */
               }
      static void
   compute_src_coords(float sx, float sy, const struct pipe_resource *src,
                     {
      tc0[0] = sx;
   tc0[1] = sy;
   tc1[0] = sx + width;
   tc1[1] = sy;
   tc2[0] = sx + width;
   tc2[1] = sy + height;
   tc3[0] = sx;
               map_point(src_matrix, tc0[0], tc0[1], &tc0[0], &tc0[1]);
   map_point(src_matrix, tc1[0], tc1[1], &tc1[0], &tc1[1]);
   map_point(src_matrix, tc2[0], tc2[1], &tc2[0], &tc2[1]);
   map_point(src_matrix, tc3[0], tc3[1], &tc3[0], &tc3[1]);
               tc0[0] /= src->width0;
   tc1[0] /= src->width0;
   tc2[0] /= src->width0;
   tc3[0] /= src->width0;
   tc0[1] /= src->height0;
   tc1[1] /= src->height0;
   tc2[1] /= src->height0;
      }
      static void
   add_vertex_data1(struct xa_context *r,
                     {
               compute_src_coords(srcX, srcY, src, src_matrix, width, height,
         /* 1st vertex */
   add_vertex_1tex(r, dstX, dstY, tc0[0], tc0[1]);
   /* 2nd vertex */
   add_vertex_1tex(r, dstX + width, dstY, tc1[0], tc1[1]);
   /* 3rd vertex */
   add_vertex_1tex(r, dstX + width, dstY + height, tc2[0], tc2[1]);
   /* 4th vertex */
      }
      static void
   add_vertex_data2(struct xa_context *r,
                  float srcX, float srcY, float maskX, float maskY,
   float dstX, float dstY, float width, float height,
      {
      float spt0[2], spt1[2], spt2[2], spt3[2];
            compute_src_coords(srcX, srcY, src, src_matrix, width, height,
         compute_src_coords(maskX, maskY, mask, mask_matrix, width, height,
            /* 1st vertex */
   add_vertex_2tex(r, dstX, dstY,
         /* 2nd vertex */
   add_vertex_2tex(r, dstX + width, dstY,
         /* 3rd vertex */
   add_vertex_2tex(r, dstX + width, dstY + height,
         /* 4th vertex */
   add_vertex_2tex(r, dstX, dstY + height,
      }
      static void
   setup_vertex_data_yuv(struct xa_context *r,
         float srcX,
   float srcY,
   float srcW,
   float srcH,
   float dstX,
   float dstY,
   {
      float s0, t0, s1, t1;
   float spt0[2], spt1[2];
            spt0[0] = srcX;
   spt0[1] = srcY;
   spt1[0] = srcX + srcW;
            tex = srf[0]->tex;
   s0 = spt0[0] / tex->width0;
   t0 = spt0[1] / tex->height0;
   s1 = spt1[0] / tex->width0;
            /* 1st vertex */
   add_vertex_1tex(r, dstX, dstY, s0, t0);
   /* 2nd vertex */
   add_vertex_1tex(r, dstX + dstW, dstY, s1, t0);
   /* 3rd vertex */
   add_vertex_1tex(r, dstX + dstW, dstY + dstH, s1, t1);
   /* 4th vertex */
      }
      /* Set up framebuffer, viewport and vertex shader constant buffer
   * state for a particular destinaton surface.  In all our rendering,
   * these concepts are linked.
   */
   void
   renderer_bind_destination(struct xa_context *r,
         {
      int width = surface->width;
            struct pipe_framebuffer_state fb;
                     /* Framebuffer uses actual surface width/height
   */
   memset(&fb, 0, sizeof fb);
   fb.width = surface->width;
   fb.height = surface->height;
   fb.nr_cbufs = 1;
   fb.cbufs[0] = surface;
            /* Viewport just touches the bit we're interested in:
   */
   viewport.scale[0] = width / 2.f;
   viewport.scale[1] = height / 2.f;
   viewport.scale[2] = 1.0;
   viewport.translate[0] = width / 2.f;
   viewport.translate[1] = height / 2.f;
   viewport.translate[2] = 0.0;
   viewport.swizzle_x = PIPE_VIEWPORT_SWIZZLE_POSITIVE_X;
   viewport.swizzle_y = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Y;
   viewport.swizzle_z = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Z;
            /* Constant buffer set up to match viewport dimensions:
   */
      float vs_consts[8] = {
      2.f / width, 2.f / height, 1, 1,
      };
      r->fb_width = width;
   r->fb_height = height;
      renderer_set_constants(r, PIPE_SHADER_VERTEX,
                     cso_set_framebuffer(r->cso, &fb);
      }
      void
   renderer_set_constants(struct xa_context *r,
         {
         (shader_type == PIPE_SHADER_VERTEX) ? &r->vs_const_buffer :
   &r->fs_const_buffer;
         pipe_resource_reference(cbuf, NULL);
   *cbuf = pipe_buffer_create_const0(r->pipe->screen,
                           pipe_buffer_write(r->pipe, *cbuf, 0, param_bytes, params);
      }
      }
      void
   renderer_copy_prepare(struct xa_context *r,
         struct pipe_surface *dst_surface,
   struct pipe_resource *src_texture,
   const enum xa_formats src_xa_format,
   {
      struct pipe_context *pipe = r->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct xa_shader shader;
            assert(screen->is_format_supported(screen, dst_surface->format,
         PIPE_TEXTURE_2D, 0, 0,
                     /* set misc state we care about */
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   cso_set_blend(r->cso, &blend);
               /* sampler */
      struct pipe_sampler_state sampler;
            memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
         cso_set_samplers(r->cso, PIPE_SHADER_FRAGMENT, 1, &p_sampler);
               /* texture/sampler view */
      struct pipe_sampler_view templ;
   struct pipe_sampler_view *src_view;
      u_sampler_view_default_template(&templ,
         src_view = pipe->create_sampler_view(pipe, src_texture, &templ);
   pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0, 1, 0, false, &src_view);
   pipe_sampler_view_reference(&src_view, NULL);
               /* shaders */
   if (src_texture->format == PIPE_FORMAT_L8_UNORM ||
      fs_traits |= FS_SRC_LUMINANCE;
      if (dst_surface->format == PIPE_FORMAT_L8_UNORM ||
      fs_traits |= FS_DST_LUMINANCE;
         xa_format_a(src_xa_format) == 0)
   fs_traits |= FS_SRC_SET_ALPHA;
         shader = xa_shaders_get(r->shaders, VS_COMPOSITE, fs_traits);
   cso_set_vertex_shader_handle(r->cso, shader.vs);
            r->buffer_size = 0;
      }
      void
   renderer_copy(struct xa_context *r,
         int dx,
   int dy,
   int sx,
   int sy,
   {
      float s0, t0, s1, t1;
            /* XXX: could put the texcoord scaling calculation into the vertex
   * shader.
   */
   s0 = sx / src_width;
   s1 = (sx + width) / src_width;
   t0 = sy / src_height;
            x0 = dx;
   x1 = dx + width;
   y0 = dy;
            /* draw quad */
   renderer_draw_conditional(r, 4 * 8);
   add_vertex_1tex(r, x0, y0, s0, t0);
   add_vertex_1tex(r, x1, y0, s1, t0);
   add_vertex_1tex(r, x1, y1, s1, t1);
      }
      void
   renderer_draw_yuv(struct xa_context *r,
      float src_x,
   float src_y,
   float src_w,
   float src_h,
   int dst_x,
      {
               setup_vertex_data_yuv(r,
                  if (!r->scissor_valid) {
      r->scissor.minx = 0;
   r->scissor.miny = 0;
   r->scissor.maxx = r->dst->tex->width0;
                        struct cso_velems_state velems;
   velems.count = num_attribs;
            cso_set_vertex_elements(r->cso, &velems);
   util_draw_user_vertex_buffer(r->cso, r->buffer, MESA_PRIM_QUADS,
                           }
      void
   renderer_begin_solid(struct xa_context *r)
   {
      r->buffer_size = 0;
   r->attrs_per_vertex = 1;
   renderer_set_constants(r, PIPE_SHADER_FRAGMENT, r->solid_color,
      }
      void
   renderer_solid(struct xa_context *r,
         {
      /*
   * debug_printf("solid rect[(%d, %d), (%d, %d)], rgba[%f, %f, %f, %f]\n",
                     /* 1st vertex */
   add_vertex_none(r, x0, y0);
   /* 2nd vertex */
   add_vertex_none(r, x1, y0);
   /* 3rd vertex */
   add_vertex_none(r, x1, y1);
   /* 4th vertex */
      }
      void
   renderer_draw_flush(struct xa_context *r)
   {
         }
      void
   renderer_begin_textures(struct xa_context *r)
   {
      r->attrs_per_vertex = 1 + r->num_bound_samplers;
   r->buffer_size = 0;
   if (r->has_solid_src || r->has_solid_mask)
      renderer_set_constants(r, PIPE_SHADER_FRAGMENT, r->solid_color,
   }
      void
   renderer_texture(struct xa_context *r,
      int *pos,
   int width, int height,
   const float *src_matrix,
      {
            #if 0
         debug_printf("src_matrix = \n");
   debug_printf("%f, %f, %f\n", src_matrix[0], src_matrix[1], src_matrix[2]);
   debug_printf("%f, %f, %f\n", src_matrix[3], src_matrix[4], src_matrix[5]);
   debug_printf("%f, %f, %f\n", src_matrix[6], src_matrix[7], src_matrix[8]);
      }
      debug_printf("mask_matrix = \n");
   debug_printf("%f, %f, %f\n", mask_matrix[0], mask_matrix[1], mask_matrix[2]);
   debug_printf("%f, %f, %f\n", mask_matrix[3], mask_matrix[4], mask_matrix[5]);
   debug_printf("%f, %f, %f\n", mask_matrix[6], mask_matrix[7], mask_matrix[8]);
         #endif
         switch(r->attrs_per_vertex) {
      renderer_draw_conditional(r, 4 * 8);
         if (!r->has_solid_src) {
      add_vertex_data1(r,
                        } else {
      add_vertex_data1(r,
                        break;
         renderer_draw_conditional(r, 4 * 12);
   add_vertex_data2(r,
      pos[0], pos[1], /* src */
   pos[2], pos[3], /* mask */
   pos[4], pos[5], /* dst */
   width, height,
   sampler_view[0]->texture, sampler_view[1]->texture,
      break;
         break;
         }
