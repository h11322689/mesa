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
   * Thomas Hellstrom <thellstrom-at-vmware-dot-com>
   */
      #include "xa_context.h"
   #include "xa_priv.h"
   #include "util/u_inlines.h"
   #include "util/u_sampler.h"
   #include "util/u_surface.h"
   #include "cso_cache/cso_context.h"
      static void
   xa_yuv_bind_blend_state(struct xa_context *r)
   {
               memset(&blend, 0, sizeof(struct pipe_blend_state));
   blend.rt[0].blend_enable = 0;
            /* porter&duff src */
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
               }
      static void
   xa_yuv_bind_shaders(struct xa_context *r)
   {
      unsigned vs_traits = 0, fs_traits = 0;
            vs_traits |= VS_YUV;
            shader = xa_shaders_get(r->shaders, vs_traits, fs_traits);
   cso_set_vertex_shader_handle(r->cso, shader.vs);
      }
      static void
   xa_yuv_bind_samplers(struct xa_context *r, struct xa_surface *yuv[])
   {
      struct pipe_sampler_state *samplers[3];
   struct pipe_sampler_state sampler;
   struct pipe_sampler_view view_templ;
                     sampler.wrap_s = PIPE_TEX_WRAP_CLAMP;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
               samplers[i] = &sampler;
   u_sampler_view_default_template(&view_templ, yuv[i]->tex,
            r->bound_sampler_views[i] =
      r->pipe->create_sampler_view(r->pipe, yuv[i]->tex, &view_templ);
   }
   r->num_bound_samplers = 3;
   cso_set_samplers(r->cso, PIPE_SHADER_FRAGMENT, 3, (const struct pipe_sampler_state **)samplers);
      }
      static void
   xa_yuv_fs_constants(struct xa_context *r, const float conversion_matrix[])
   {
               renderer_set_constants(r, PIPE_SHADER_FRAGMENT,
      }
      XA_EXPORT int
   xa_yuv_planar_blit(struct xa_context *r,
      int src_x,
   int src_y,
   int src_w,
   int src_h,
   int dst_x,
   int dst_y,
   int dst_w,
   int dst_h,
   struct xa_box *box,
   unsigned int num_boxes,
   const float conversion_matrix[],
      {
      float scale_x;
   float scale_y;
               return XA_ERR_NONE;
         ret = xa_ctx_srf_create(r, dst);
      return -XA_ERR_NORES;
         renderer_bind_destination(r, r->srf);
   xa_yuv_bind_blend_state(r);
   xa_yuv_bind_shaders(r);
   xa_yuv_bind_samplers(r, yuv);
            scale_x = (float)src_w / (float)dst_w;
               int x = box->x1;
   int y = box->y1;
   int w = box->x2 - box->x1;
   int h = box->y2 - box->y1;
            renderer_draw_yuv(r,
      (float)src_x + scale_x * (x - dst_x),
   (float)src_y + scale_y * (y - dst_y),
      box++;
                        xa_ctx_sampler_views_destroy(r);
               }
