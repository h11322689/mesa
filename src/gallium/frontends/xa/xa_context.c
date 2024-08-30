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
   #include "cso_cache/cso_context.h"
   #include "util/u_inlines.h"
   #include "util/u_rect.h"
   #include "util/u_surface.h"
   #include "pipe/p_context.h"
      XA_EXPORT void
   xa_context_flush(struct xa_context *ctx)
   {
      if (ctx->last_fence) {
      struct pipe_screen *screen = ctx->xa->screen;
      }
      }
      XA_EXPORT struct xa_context *
   xa_context_default(struct xa_tracker *xa)
   {
         }
      XA_EXPORT struct xa_context *
   xa_context_create(struct xa_tracker *xa)
   {
               ctx->xa = xa;
   ctx->pipe = xa->screen->context_create(xa->screen, NULL, 0);
   ctx->cso = cso_create_context(ctx->pipe, 0);
   ctx->shaders = xa_shaders_create(ctx);
               }
      XA_EXPORT void
   xa_context_destroy(struct xa_context *r)
   {
      struct pipe_resource **vsbuf = &r->vs_const_buffer;
               pipe_resource_reference(vsbuf, NULL);
            pipe_resource_reference(fsbuf, NULL);
            xa_shaders_destroy(r->shaders);
   r->shaders = NULL;
               xa_ctx_sampler_views_destroy(r);
   if (r->srf)
               cso_destroy_context(r->cso);
   r->cso = NULL;
               r->pipe->destroy(r->pipe);
      }
      XA_EXPORT int
   xa_surface_dma(struct xa_context *ctx,
         struct xa_surface *srf,
   void *data,
   unsigned int pitch,
   {
      struct pipe_transfer *transfer;
   void *map;
   int w, h, i;
   enum pipe_map_flags transfer_direction;
            transfer_direction = (to_surface ? PIPE_MAP_WRITE :
               w = boxes->x2 - boxes->x1;
   h = boxes->y2 - boxes->y1;
      map = pipe_texture_map(pipe, srf->tex, 0, 0,
               if (!map)
            if (to_surface) {
      util_copy_rect(map, srf->tex->format, transfer->stride,
      } else {
      util_copy_rect(data, srf->tex->format, pitch,
      boxes->x1, boxes->y1, w, h, map, transfer->stride, 0,
   }
   pipe->texture_unmap(pipe, transfer);
      }
      }
      XA_EXPORT void *
   xa_surface_map(struct xa_context *ctx,
         {
      void *map;
   unsigned int gallium_usage = 0;
            /*
   * A surface may only have a single map.
   */
      return NULL;
            gallium_usage |= PIPE_MAP_READ;
         gallium_usage |= PIPE_MAP_WRITE;
         gallium_usage |= PIPE_MAP_DIRECTLY;
         gallium_usage |= PIPE_MAP_UNSYNCHRONIZED;
         gallium_usage |= PIPE_MAP_DONTBLOCK;
         gallium_usage |= PIPE_MAP_DISCARD_WHOLE_RESOURCE;
            return NULL;
         map = pipe_texture_map(pipe, srf->tex, 0, 0,
                        return NULL;
         srf->mapping_pipe = pipe;
      }
      XA_EXPORT void
   xa_surface_unmap(struct xa_surface *srf)
   {
         struct pipe_context *pipe = srf->mapping_pipe;
      pipe->texture_unmap(pipe, srf->transfer);
   srf->transfer = NULL;
         }
      int
   xa_ctx_srf_create(struct xa_context *ctx, struct xa_surface *dst)
   {
      struct pipe_screen *screen = ctx->pipe->screen;
            /*
   * Cache surfaces unless we change render target
   */
   if (ctx->srf) {
      if (ctx->srf->texture == dst->tex)
                        if (!screen->is_format_supported(screen,  dst->tex->format,
            return -XA_ERR_INVAL;
         u_surface_default_template(&srf_templ, dst->tex);
   ctx->srf = ctx->pipe->create_surface(ctx->pipe, dst->tex, &srf_templ);
      return -XA_ERR_NORES;
            }
      void
   xa_ctx_srf_destroy(struct xa_context *ctx)
   {
      /*
   * Cache surfaces unless we change render target.
   * Final destruction on context destroy.
      }
      XA_EXPORT int
   xa_copy_prepare(struct xa_context *ctx,
   struct xa_surface *dst, struct xa_surface *src)
   {
         return -XA_ERR_INVAL;
            int ret = xa_ctx_srf_create(ctx, dst);
   if (ret != XA_ERR_NONE)
         renderer_copy_prepare(ctx, ctx->srf, src->tex,
               ctx->simple_copy = 0;
         ctx->simple_copy = 1;
         ctx->src = src;
   ctx->dst = dst;
               }
      XA_EXPORT void
   xa_copy(struct xa_context *ctx,
   int dx, int dy, int sx, int sy, int width, int height)
   {
                           u_box_2d(sx, sy, width, height, &src_box);
   ctx->pipe->resource_copy_region(ctx->pipe,
      ctx->dst->tex, 0, dx, dy, 0,
   ctx->src->tex,
   0, &src_box);
      renderer_copy(ctx, dx, dy, sx, sy, width, height,
         (float) ctx->src->tex->width0,
   }
      XA_EXPORT void
   xa_copy_done(struct xa_context *ctx)
   {
         renderer_draw_flush(ctx);
         }
      static void
   bind_solid_blend_state(struct xa_context *ctx)
   {
               memset(&blend, 0, sizeof(struct pipe_blend_state));
   blend.rt[0].blend_enable = 0;
            blend.rt[0].rgb_src_factor   = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor   = PIPE_BLENDFACTOR_ZERO;
               }
      XA_EXPORT int
   xa_solid_prepare(struct xa_context *ctx, struct xa_surface *dst,
         {
      unsigned vs_traits, fs_traits;
   struct xa_shader shader;
            ret = xa_ctx_srf_create(ctx, dst);
      return ret;
            xa_pixel_to_float4_a8(fg, ctx->solid_color);
         xa_pixel_to_float4(fg, ctx->solid_color);
                     #if 0
      debug_printf("Color Pixel=(%d, %d, %d, %d), RGBA=(%f, %f, %f, %f)\n",
   (fg >> 24) & 0xff, (fg >> 16) & 0xff,
   (fg >> 8) & 0xff,  (fg >> 0) & 0xff,
   exa->solid_color[0], exa->solid_color[1],
      #endif
         vs_traits = VS_SRC_SRC | VS_COMPOSITE;
            renderer_bind_destination(ctx, ctx->srf);
   bind_solid_blend_state(ctx);
   cso_set_samplers(ctx->cso, PIPE_SHADER_FRAGMENT, 0, NULL);
   ctx->pipe->set_sampler_views(ctx->pipe, PIPE_SHADER_FRAGMENT, 0, 0,
            shader = xa_shaders_get(ctx->shaders, vs_traits, fs_traits);
   cso_set_vertex_shader_handle(ctx->cso, shader.vs);
                     xa_ctx_srf_destroy(ctx);
      }
      XA_EXPORT void
   xa_solid(struct xa_context *ctx, int x, int y, int width, int height)
   {
      xa_scissor_update(ctx, x, y, x + width, y + height);
      }
      XA_EXPORT void
   xa_solid_done(struct xa_context *ctx)
   {
      renderer_draw_flush(ctx);
   ctx->comp = NULL;
   ctx->has_solid_src = false;
      }
      XA_EXPORT struct xa_fence *
   xa_fence_get(struct xa_context *ctx)
   {
      struct xa_fence *fence = calloc(1, sizeof(*fence));
               return NULL;
                     fence->pipe_fence = NULL;
         screen->fence_reference(screen, &fence->pipe_fence, ctx->last_fence);
            }
      XA_EXPORT int
   xa_fence_wait(struct xa_fence *fence, uint64_t timeout)
   {
         return XA_ERR_NONE;
            struct pipe_screen *screen = fence->xa->screen;
   bool timed_out;
      timed_out = !screen->fence_finish(screen, NULL, fence->pipe_fence, timeout);
   if (timed_out)
            screen->fence_reference(screen, &fence->pipe_fence, NULL);
      }
      }
      XA_EXPORT void
   xa_fence_destroy(struct xa_fence *fence)
   {
         return;
            struct pipe_screen *screen = fence->xa->screen;
      screen->fence_reference(screen, &fence->pipe_fence, NULL);
                  }
      void
   xa_ctx_sampler_views_destroy(struct xa_context *ctx)
   {
                  pipe_sampler_view_reference(&ctx->bound_sampler_views[i], NULL);
         }
