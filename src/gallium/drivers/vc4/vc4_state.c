   /*
   * Copyright © 2014 Broadcom
   * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
      #include "pipe/p_state.h"
   #include "util/u_framebuffer.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_helpers.h"
      #include "vc4_context.h"
      static void *
   vc4_generic_cso_state_create(const void *src, uint32_t size)
   {
         void *dst = calloc(1, size);
   if (!dst)
         memcpy(dst, src, size);
   }
      static void
   vc4_generic_cso_state_delete(struct pipe_context *pctx, void *hwcso)
   {
         }
      static void
   vc4_set_blend_color(struct pipe_context *pctx,
         {
         struct vc4_context *vc4 = vc4_context(pctx);
   vc4->blend_color.f = *blend_color;
   for (int i = 0; i < 4; i++)
         }
      static void
   vc4_set_stencil_ref(struct pipe_context *pctx,
         {
         struct vc4_context *vc4 = vc4_context(pctx);
   vc4->stencil_ref = stencil_ref;
   }
      static void
   vc4_set_clip_state(struct pipe_context *pctx,
         {
         struct vc4_context *vc4 = vc4_context(pctx);
   vc4->clip = *clip;
   }
      static void
   vc4_set_sample_mask(struct pipe_context *pctx, unsigned sample_mask)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
   vc4->sample_mask = sample_mask & ((1 << VC4_MAX_SAMPLES) - 1);
   }
      static uint16_t
   float_to_187_half(float f)
   {
         }
      static void *
   vc4_create_rasterizer_state(struct pipe_context *pctx,
         {
         struct vc4_rasterizer_state *so;
   struct V3D21_DEPTH_OFFSET depth_offset = { V3D21_DEPTH_OFFSET_header };
   struct V3D21_POINT_SIZE point_size = { V3D21_POINT_SIZE_header };
            so = CALLOC_STRUCT(vc4_rasterizer_state);
   if (!so)
                     if (!(cso->cull_face & PIPE_FACE_FRONT))
         if (!(cso->cull_face & PIPE_FACE_BACK))
            /* Workaround: HW-2726 PTB does not handle zero-size points (BCM2835,
      * BCM21553).
                        if (cso->front_ccw)
            if (cso->offset_tri) {
                     depth_offset.depth_offset_units =
                     if (cso->multisample)
            V3D21_DEPTH_OFFSET_pack(NULL, so->packed.depth_offset, &depth_offset);
   V3D21_POINT_SIZE_pack(NULL, so->packed.point_size, &point_size);
            if (cso->tile_raster_order_fixed) {
            so->tile_raster_order_flags |= VC4_SUBMIT_CL_FIXED_RCL_ORDER;
   if (cso->tile_raster_order_increasing_x) {
               }
   if (cso->tile_raster_order_increasing_y) {
                     }
      /* Blend state is baked into shaders. */
   static void *
   vc4_create_blend_state(struct pipe_context *pctx,
         {
         }
      /**
   * The TLB_STENCIL_SETUP data has a little bitfield for common writemask
   * values, so you don't have to do a separate writemask setup.
   */
   static uint8_t
   tlb_stencil_setup_writemask(uint8_t mask)
   {
         switch (mask) {
   case 0x1: return 0;
   case 0x3: return 1;
   case 0xf: return 2;
   case 0xff: return 3;
   default: return 0xff;
   }
      static uint32_t
   tlb_stencil_setup_bits(const struct pipe_stencil_state *state,
         {
         static const uint8_t op_map[] = {
            [PIPE_STENCIL_OP_ZERO] = 0,
   [PIPE_STENCIL_OP_KEEP] = 1,
   [PIPE_STENCIL_OP_REPLACE] = 2,
   [PIPE_STENCIL_OP_INCR] = 3,
   [PIPE_STENCIL_OP_DECR] = 4,
   [PIPE_STENCIL_OP_INVERT] = 5,
      };
            if (writemask_bits != 0xff)
         bits |= op_map[state->zfail_op] << 25;
   bits |= op_map[state->zpass_op] << 22;
   bits |= op_map[state->fail_op] << 19;
   bits |= state->func << 16;
   /* Ref is filled in at uniform upload time */
            }
      static void *
   vc4_create_depth_stencil_alpha_state(struct pipe_context *pctx,
         {
                  so = CALLOC_STRUCT(vc4_depth_stencil_alpha_state);
   if (!so)
                     /* We always keep the early Z state correct, since a later state using
      * early Z may want it.
               if (cso->depth_enabled) {
            if (cso->depth_writemask) {
                              /* We only handle early Z in the < direction because otherwise
   * we'd have to runtime guess which direction to set in the
   * render config.
   */
   if ((cso->depth_func == PIPE_FUNC_LESS ||
            (!cso->stencil[0].enabled ||
      (cso->stencil[0].zfail_op == PIPE_STENCIL_OP_KEEP &&
   (!cso->stencil[1].enabled ||
      } else {
                        if (cso->stencil[0].enabled) {
                           uint8_t front_writemask_bits =
                        so->stencil_uniforms[0] =
         if (back->enabled) {
                                 so->stencil_uniforms[0] |= (1 << 30);
   so->stencil_uniforms[1] =
                           if (front_writemask_bits == 0xff ||
      back_writemask_bits == 0xff) {
                  }
      static void
   vc4_set_polygon_stipple(struct pipe_context *pctx,
         {
         struct vc4_context *vc4 = vc4_context(pctx);
   vc4->stipple = *stipple;
   }
      static void
   vc4_set_scissor_states(struct pipe_context *pctx,
                     {
                  vc4->scissor = *scissor;
   }
      static void
   vc4_set_viewport_states(struct pipe_context *pctx,
                     {
         struct vc4_context *vc4 = vc4_context(pctx);
   vc4->viewport = *viewport;
   }
      static void
   vc4_set_vertex_buffers(struct pipe_context *pctx,
                           {
         struct vc4_context *vc4 = vc4_context(pctx);
            util_set_vertex_buffers_mask(so->vb, &so->enabled_mask, vb,
                        }
      static void
   vc4_blend_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
   vc4->blend = hwcso;
   }
      static void
   vc4_rasterizer_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
            if (vc4->rasterizer && rast &&
         vc4->rasterizer->base.flatshade != rast->base.flatshade) {
            vc4->rasterizer = hwcso;
   }
      static void
   vc4_zsa_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
   vc4->zsa = hwcso;
   }
      static void *
   vc4_vertex_state_create(struct pipe_context *pctx, unsigned num_elements,
         {
                  if (!so)
            memcpy(so->pipe, elements, sizeof(*elements) * num_elements);
            }
      static void
   vc4_vertex_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
   vc4->vtx = hwcso;
   }
      static void
   vc4_set_constant_buffer(struct pipe_context *pctx,
                     {
         struct vc4_context *vc4 = vc4_context(pctx);
            /* Note that the gallium frontend can unbind constant buffers by
      * passing NULL here.
      if (unlikely(!cb)) {
            so->enabled_mask &= ~(1 << index);
               if (index == 1 && so->cb[index].buffer_size != cb->buffer_size)
                     so->enabled_mask |= 1 << index;
   so->dirty_mask |= 1 << index;
   }
      static void
   vc4_set_framebuffer_state(struct pipe_context *pctx,
         {
         struct vc4_context *vc4 = vc4_context(pctx);
                              /* Nonzero texture mipmap levels are laid out as if they were in
      * power-of-two-sized spaces.  The renderbuffer config infers its
   * stride from the width parameter, so we need to configure our
   * framebuffer.  Note that if the z/color buffers were mismatched
   * sizes, we wouldn't be able to do this.
      if (cso->cbufs[0] && cso->cbufs[0]->u.tex.level) {
            struct vc4_resource *rsc =
         cso->width =
      } else if (cso->zsbuf && cso->zsbuf->u.tex.level){
            struct vc4_resource *rsc =
         cso->width =
               }
      static struct vc4_texture_stateobj *
   vc4_get_stage_tex(struct vc4_context *vc4, enum pipe_shader_type shader)
   {
         switch (shader) {
   case PIPE_SHADER_FRAGMENT:
            vc4->dirty |= VC4_DIRTY_FRAGTEX;
      case PIPE_SHADER_VERTEX:
            vc4->dirty |= VC4_DIRTY_VERTTEX;
      default:
               }
      static uint32_t translate_wrap(uint32_t p_wrap, bool using_nearest)
   {
         switch (p_wrap) {
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_CLAMP:
         default:
            fprintf(stderr, "Unknown wrap mode %d\n", p_wrap);
      }
      static void *
   vc4_create_sampler_state(struct pipe_context *pctx,
         {
         static const uint8_t minfilter_map[6] = {
            VC4_TEX_P1_MINFILT_NEAR_MIP_NEAR,
   VC4_TEX_P1_MINFILT_LIN_MIP_NEAR,
   VC4_TEX_P1_MINFILT_NEAR_MIP_LIN,
   VC4_TEX_P1_MINFILT_LIN_MIP_LIN,
      };
   static const uint32_t magfilter_map[] = {
               };
   bool either_nearest =
                        if (!so)
                     so->texture_p1 =
            (VC4_SET_FIELD(magfilter_map[cso->mag_img_filter],
         VC4_SET_FIELD(minfilter_map[cso->min_mip_filter * 2 +
               VC4_SET_FIELD(translate_wrap(cso->wrap_s, either_nearest),
               }
      static void
   vc4_sampler_states_bind(struct pipe_context *pctx,
               {
         struct vc4_context *vc4 = vc4_context(pctx);
            assert(start == 0);
   unsigned i;
            for (i = 0; i < nr; i++) {
            if (hwcso[i])
               for (; i < stage_tex->num_samplers; i++) {
                  }
      static struct pipe_sampler_view *
   vc4_create_sampler_view(struct pipe_context *pctx, struct pipe_resource *prsc,
         {
         struct vc4_sampler_view *so = CALLOC_STRUCT(vc4_sampler_view);
            if (!so)
                     so->base.texture = NULL;
   pipe_resource_reference(&so->base.texture, prsc);
   so->base.reference.count = 1;
            /* There is no hardware level clamping, and the start address of a
      * texture may be misaligned, so in that case we have to copy to a
   * temporary.
   *
   * Also, Raspberry Pi doesn't support sampling from raster textures,
   * so we also have to copy to a temporary then.
      if ((cso->u.tex.first_level &&
         (cso->u.tex.first_level != cso->u.tex.last_level)) ||
   rsc->vc4_format == VC4_TEXTURE_TYPE_RGBA32R ||
   rsc->vc4_format == ~0) {
      struct vc4_resource *shadow_parent = rsc;
   struct pipe_resource tmpl = {
            .target = prsc->target,
   .format = prsc->format,
   .width0 = u_minify(prsc->width0,
         .height0 = u_minify(prsc->height0,
                           /* Create the shadow texture.  The rest of the texture
   * parameter setup will use the shadow.
   */
   prsc = vc4_resource_create(pctx->screen, &tmpl);
   if (!prsc) {
               }
   rsc = vc4_resource(prsc);
                                          } else {
                     if (cso->u.tex.first_level) {
               so->texture_p0 =
            (VC4_SET_FIELD((rsc->slices[0].offset +
               VC4_SET_FIELD(rsc->vc4_format & 15, VC4_TEX_P0_TYPE) |
   VC4_SET_FIELD(so->force_first_level ?
                        so->texture_p1 =
                        if (prsc->format == PIPE_FORMAT_ETC1_RGB8)
            }
      static void
   vc4_sampler_view_destroy(struct pipe_context *pctx,
         {
         struct vc4_sampler_view *view = vc4_sampler_view(pview);
   pipe_resource_reference(&pview->texture, NULL);
   pipe_resource_reference(&view->texture, NULL);
   }
      static void
   vc4_set_sampler_views(struct pipe_context *pctx,
                        enum pipe_shader_type shader,
      {
         struct vc4_context *vc4 = vc4_context(pctx);
   struct vc4_texture_stateobj *stage_tex = vc4_get_stage_tex(vc4, shader);
   unsigned i;
                     for (i = 0; i < nr; i++) {
            if (views[i])
         if (take_ownership) {
               } else {
               for (; i < stage_tex->num_textures; i++) {
                  }
      void
   vc4_state_init(struct pipe_context *pctx)
   {
         pctx->set_blend_color = vc4_set_blend_color;
   pctx->set_stencil_ref = vc4_set_stencil_ref;
   pctx->set_clip_state = vc4_set_clip_state;
   pctx->set_sample_mask = vc4_set_sample_mask;
   pctx->set_constant_buffer = vc4_set_constant_buffer;
   pctx->set_framebuffer_state = vc4_set_framebuffer_state;
   pctx->set_polygon_stipple = vc4_set_polygon_stipple;
   pctx->set_scissor_states = vc4_set_scissor_states;
                     pctx->create_blend_state = vc4_create_blend_state;
   pctx->bind_blend_state = vc4_blend_state_bind;
            pctx->create_rasterizer_state = vc4_create_rasterizer_state;
   pctx->bind_rasterizer_state = vc4_rasterizer_state_bind;
            pctx->create_depth_stencil_alpha_state = vc4_create_depth_stencil_alpha_state;
   pctx->bind_depth_stencil_alpha_state = vc4_zsa_state_bind;
            pctx->create_vertex_elements_state = vc4_vertex_state_create;
   pctx->delete_vertex_elements_state = vc4_generic_cso_state_delete;
            pctx->create_sampler_state = vc4_create_sampler_state;
   pctx->delete_sampler_state = vc4_generic_cso_state_delete;
            pctx->create_sampler_view = vc4_create_sampler_view;
   pctx->sampler_view_destroy = vc4_sampler_view_destroy;
   }
