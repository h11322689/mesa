   /*
   * Copyright (c) 2011-2013 Luc Verhaegen <libv@skynet.be>
   * Copyright (c) 2017-2019 Lima Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   */
      #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "util/u_helpers.h"
   #include "util/u_debug.h"
   #include "util/u_framebuffer.h"
   #include "util/u_viewport.h"
      #include "pipe/p_state.h"
      #include "lima_screen.h"
   #include "lima_context.h"
   #include "lima_format.h"
   #include "lima_resource.h"
      static void
   lima_set_framebuffer_state(struct pipe_context *pctx,
         {
               /* make sure there are always single job in this context */
   if (lima_debug & LIMA_DEBUG_SINGLE_JOB)
                              ctx->job = NULL;
      }
      static void
   lima_set_polygon_stipple(struct pipe_context *pctx,
         {
      }
      static void *
   lima_create_depth_stencil_alpha_state(struct pipe_context *pctx,
         {
               so = CALLOC_STRUCT(lima_depth_stencil_alpha_state);
   if (!so)
                        }
      static void
   lima_bind_depth_stencil_alpha_state(struct pipe_context *pctx, void *hwcso)
   {
               ctx->zsa = hwcso;
      }
      static void
   lima_delete_depth_stencil_alpha_state(struct pipe_context *pctx, void *hwcso)
   {
         }
      static void *
   lima_create_rasterizer_state(struct pipe_context *pctx,
         {
               so = CALLOC_STRUCT(lima_rasterizer_state);
   if (!so)
                        }
      static void
   lima_bind_rasterizer_state(struct pipe_context *pctx, void *hwcso)
   {
               ctx->rasterizer = hwcso;
      }
      static void
   lima_delete_rasterizer_state(struct pipe_context *pctx, void *hwcso)
   {
         }
      static void *
   lima_create_blend_state(struct pipe_context *pctx,
         {
               so = CALLOC_STRUCT(lima_blend_state);
   if (!so)
                        }
      static void
   lima_bind_blend_state(struct pipe_context *pctx, void *hwcso)
   {
               ctx->blend = hwcso;
      }
      static void
   lima_delete_blend_state(struct pipe_context *pctx, void *hwcso)
   {
         }
      static void *
   lima_create_vertex_elements_state(struct pipe_context *pctx, unsigned num_elements,
         {
               so = CALLOC_STRUCT(lima_vertex_element_state);
   if (!so)
            memcpy(so->pipe, elements, sizeof(*elements) * num_elements);
               }
      static void
   lima_bind_vertex_elements_state(struct pipe_context *pctx, void *hwcso)
   {
               ctx->vertex_elements = hwcso;
      }
      static void
   lima_delete_vertex_elements_state(struct pipe_context *pctx, void *hwcso)
   {
         }
      static void
   lima_set_vertex_buffers(struct pipe_context *pctx,
                           {
      struct lima_context *ctx = lima_context(pctx);
            util_set_vertex_buffers_mask(so->vb, &so->enabled_mask,
                                 }
      static void
   lima_set_viewport_states(struct pipe_context *pctx,
                     {
               /* reverse calculate the parameter of glViewport */
   ctx->viewport.left = ctx->ext_viewport.left =
         ctx->viewport.right = ctx->ext_viewport.right =
         ctx->viewport.bottom = ctx->ext_viewport.bottom =
         ctx->viewport.top = ctx->ext_viewport.top =
            /* reverse calculate the parameter of glDepthRange */
   float near, far;
   bool halfz = ctx->rasterizer && ctx->rasterizer->base.clip_halfz;
            ctx->viewport.near = ctx->rasterizer && ctx->rasterizer->base.depth_clip_near ? near : 0.0f;
            ctx->viewport.transform = *viewport;
      }
      static void
   lima_set_scissor_states(struct pipe_context *pctx,
                     {
               ctx->scissor = *scissor;
      }
      static void
   lima_set_blend_color(struct pipe_context *pctx,
         {
               ctx->blend_color = *blend_color;
      }
      static void
   lima_set_stencil_ref(struct pipe_context *pctx,
         {
               ctx->stencil_ref = stencil_ref;
      }
      static void
   lima_set_clip_state(struct pipe_context *pctx,
         {
      struct lima_context *ctx = lima_context(pctx);
               }
      static void
   lima_set_constant_buffer(struct pipe_context *pctx,
                     {
      struct lima_context *ctx = lima_context(pctx);
                     if (unlikely(!cb)) {
      so->buffer = NULL;
      } else {
               so->buffer = cb->user_buffer + cb->buffer_offset;
               so->dirty = true;
         }
      static void *
   lima_create_sampler_state(struct pipe_context *pctx,
         {
      struct lima_sampler_state *so = CALLOC_STRUCT(lima_sampler_state);
   if (!so)
                        }
      static void
   lima_sampler_state_delete(struct pipe_context *pctx, void *sstate)
   {
         }
      static void
   lima_sampler_states_bind(struct pipe_context *pctx,
               {
      struct lima_context *ctx = lima_context(pctx);
   struct lima_texture_stateobj *lima_tex = &ctx->tex_stateobj;
   unsigned i;
                     for (i = 0; i < nr; i++) {
      if (hwcso[i])
                     for (; i < lima_tex->num_samplers; i++) {
                  lima_tex->num_samplers = new_nr;
      }
      static struct pipe_sampler_view *
   lima_create_sampler_view(struct pipe_context *pctx, struct pipe_resource *prsc,
         {
               if (!so)
                     pipe_reference(NULL, &prsc->reference);
   so->base.texture = prsc;
   so->base.reference.count = 1;
            uint8_t sampler_swizzle[4] = { cso->swizzle_r, cso->swizzle_g,
         const uint8_t *format_swizzle = lima_format_get_texel_swizzle(cso->format);
               }
      static void
   lima_sampler_view_destroy(struct pipe_context *pctx,
         {
                           }
      static void
   lima_set_sampler_views(struct pipe_context *pctx,
                        enum pipe_shader_type shader,
      {
      struct lima_context *ctx = lima_context(pctx);
   struct lima_texture_stateobj *lima_tex = &ctx->tex_stateobj;
   int i;
                     for (i = 0; i < nr; i++) {
      if (views[i])
            if (take_ownership) {
      pipe_sampler_view_reference(&lima_tex->textures[i], NULL);
      } else {
                     for (; i < lima_tex->num_textures; i++) {
                  lima_tex->num_textures = new_nr;
      }
      static void
   lima_set_sample_mask(struct pipe_context *pctx,
         {
      struct lima_context *ctx = lima_context(pctx);
   ctx->sample_mask = sample_mask & ((1 << LIMA_MAX_SAMPLES) - 1);
      }
      void
   lima_state_init(struct lima_context *ctx)
   {
      ctx->base.set_framebuffer_state = lima_set_framebuffer_state;
   ctx->base.set_polygon_stipple = lima_set_polygon_stipple;
   ctx->base.set_viewport_states = lima_set_viewport_states;
   ctx->base.set_scissor_states = lima_set_scissor_states;
   ctx->base.set_blend_color = lima_set_blend_color;
   ctx->base.set_stencil_ref = lima_set_stencil_ref;
            ctx->base.set_vertex_buffers = lima_set_vertex_buffers;
            ctx->base.create_depth_stencil_alpha_state = lima_create_depth_stencil_alpha_state;
   ctx->base.bind_depth_stencil_alpha_state = lima_bind_depth_stencil_alpha_state;
            ctx->base.create_rasterizer_state = lima_create_rasterizer_state;
   ctx->base.bind_rasterizer_state = lima_bind_rasterizer_state;
            ctx->base.create_blend_state = lima_create_blend_state;
   ctx->base.bind_blend_state = lima_bind_blend_state;
            ctx->base.create_vertex_elements_state = lima_create_vertex_elements_state;
   ctx->base.bind_vertex_elements_state = lima_bind_vertex_elements_state;
            ctx->base.create_sampler_state = lima_create_sampler_state;
   ctx->base.delete_sampler_state = lima_sampler_state_delete;
            ctx->base.create_sampler_view = lima_create_sampler_view;
   ctx->base.sampler_view_destroy = lima_sampler_view_destroy;
               }
      void
   lima_state_fini(struct lima_context *ctx)
   {
               util_set_vertex_buffers_mask(so->vb, &so->enabled_mask, NULL,
      }
