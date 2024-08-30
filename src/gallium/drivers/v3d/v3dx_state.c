   /*
   * Copyright © 2014-2017 Broadcom
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
   #include "util/format/u_format.h"
   #include "util/u_framebuffer.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/half_float.h"
   #include "util/u_helpers.h"
   #include "util/u_upload_mgr.h"
      #include "v3d_context.h"
   #include "broadcom/common/v3d_tiling.h"
   #include "broadcom/common/v3d_macros.h"
   #include "broadcom/common/v3d_util.h"
   #include "broadcom/compiler/v3d_compiler.h"
   #include "broadcom/cle/v3dx_pack.h"
      static void
   v3d_generic_cso_state_delete(struct pipe_context *pctx, void *hwcso)
   {
         }
      static void
   v3d_set_blend_color(struct pipe_context *pctx,
         {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->blend_color.f = *blend_color;
   for (int i = 0; i < 4; i++) {
               }
   }
      static void
   v3d_set_stencil_ref(struct pipe_context *pctx,
         {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->stencil_ref = stencil_ref;
   }
      static void
   v3d_set_clip_state(struct pipe_context *pctx,
         {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->clip = *clip;
   }
      static void
   v3d_set_sample_mask(struct pipe_context *pctx, unsigned sample_mask)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->sample_mask = sample_mask & ((1 << V3D_MAX_SAMPLES) - 1);
   }
      static void *
   v3d_create_rasterizer_state(struct pipe_context *pctx,
         {
                  so = CALLOC_STRUCT(v3d_rasterizer_state);
   if (!so)
                     /* Workaround: HW-2726 PTB does not handle zero-size points (BCM2835,
      * BCM21553).
               STATIC_ASSERT(sizeof(so->depth_offset) >=
         v3dx_pack(&so->depth_offset, DEPTH_OFFSET, depth) {
         #if V3D_VERSION >= 41
         #endif
                  /* V3d 4.x treats polygon offset units based on a Z24 buffer, so we
         #if V3D_VERSION <= 42
         v3dx_pack(&so->depth_offset_z16, DEPTH_OFFSET, depth) {
         #if V3D_VERSION >= 41
         #endif
         #endif
            }
      /* Blend state is baked into shaders. */
   static void *
   v3d_create_blend_state(struct pipe_context *pctx,
         {
                  so = CALLOC_STRUCT(v3d_blend_state);
   if (!so)
                     uint32_t max_rts = V3D_MAX_RENDER_TARGETS(V3D_VERSION);
   if (cso->independent_blend_enable) {
                                    /* V3D 4.x is when we got independent blend enables. */
   } else {
                        }
      static uint32_t
   translate_stencil_op(enum pipe_stencil_op op)
   {
         switch (op) {
   case PIPE_STENCIL_OP_KEEP:      return V3D_STENCIL_OP_KEEP;
   case PIPE_STENCIL_OP_ZERO:      return V3D_STENCIL_OP_ZERO;
   case PIPE_STENCIL_OP_REPLACE:   return V3D_STENCIL_OP_REPLACE;
   case PIPE_STENCIL_OP_INCR:      return V3D_STENCIL_OP_INCR;
   case PIPE_STENCIL_OP_DECR:      return V3D_STENCIL_OP_DECR;
   case PIPE_STENCIL_OP_INCR_WRAP: return V3D_STENCIL_OP_INCWRAP;
   case PIPE_STENCIL_OP_DECR_WRAP: return V3D_STENCIL_OP_DECWRAP;
   case PIPE_STENCIL_OP_INVERT:    return V3D_STENCIL_OP_INVERT;
   }
   }
      static void *
   v3d_create_depth_stencil_alpha_state(struct pipe_context *pctx,
         {
                  so = CALLOC_STRUCT(v3d_depth_stencil_alpha_state);
   if (!so)
                     if (cso->depth_enabled) {
            switch (cso->depth_func) {
   case PIPE_FUNC_LESS:
   case PIPE_FUNC_LEQUAL:
               case PIPE_FUNC_GREATER:
   case PIPE_FUNC_GEQUAL:
               case PIPE_FUNC_NEVER:
   case PIPE_FUNC_EQUAL:
               default:
                        /* If stencil is enabled and it's not a no-op, then it would
   * break EZ updates.
   */
   if (cso->stencil[0].enabled &&
      (cso->stencil[0].zfail_op != PIPE_STENCIL_OP_KEEP ||
      cso->stencil[0].func != PIPE_FUNC_ALWAYS ||
   (cso->stencil[1].enabled &&
   (cso->stencil[1].zfail_op != PIPE_STENCIL_OP_KEEP ||
               const struct pipe_stencil_state *front = &cso->stencil[0];
            if (front->enabled) {
            STATIC_ASSERT(sizeof(so->stencil_front) >=
         v3dx_pack(&so->stencil_front, STENCIL_CFG, config) {
            config.front_config = true;
                                             config.stencil_test_function = front->func;
   config.stencil_pass_op =
         config.depth_test_fail_op =
         }
   if (back->enabled) {
            STATIC_ASSERT(sizeof(so->stencil_back) >=
                                                      config.stencil_test_function = back->func;
   config.stencil_pass_op =
         config.depth_test_fail_op =
                  }
      static void
   v3d_set_polygon_stipple(struct pipe_context *pctx,
         {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->stipple = *stipple;
   }
      static void
   v3d_set_scissor_states(struct pipe_context *pctx,
                     {
                  v3d->scissor = *scissor;
   }
      static void
   v3d_set_viewport_states(struct pipe_context *pctx,
                     {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->viewport = *viewport;
   }
      static void
   v3d_set_vertex_buffers(struct pipe_context *pctx,
                           {
         struct v3d_context *v3d = v3d_context(pctx);
            util_set_vertex_buffers_mask(so->vb, &so->enabled_mask, vb,
                        }
      static void
   v3d_blend_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->blend = hwcso;
   }
      static void
   v3d_rasterizer_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->rasterizer = hwcso;
   }
      static void
   v3d_zsa_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->zsa = hwcso;
   }
         static bool
   needs_default_attribute_values(void)
   {
   #if V3D_VERSION <= 42
         /* FIXME: on vulkan we are able to refine even further, as we know in
      * advance when we create the pipeline if we have an integer vertex
   * attrib. Pending to check if we could do something similar here.
      #endif
         }
      static void *
   v3d_vertex_state_create(struct pipe_context *pctx, unsigned num_elements,
         {
         struct v3d_context *v3d = v3d_context(pctx);
            if (!so)
            memcpy(so->pipe, elements, sizeof(*elements) * num_elements);
            for (int i = 0; i < so->num_elements; i++) {
            const struct pipe_vertex_element *elem = &elements[i];
                                       v3dx_pack(&so->attrs[i * size],
            GL_SHADER_STATE_ATTRIBUTE_RECORD, attr) {
                                                         switch (desc->channel[0].type) {
   case UTIL_FORMAT_TYPE_FLOAT:
         if (r_size == 32) {
                                    case UTIL_FORMAT_TYPE_SIGNED:
   case UTIL_FORMAT_TYPE_UNSIGNED:
         switch (r_size) {
   case 32:
               case 16:
               case 10:
               case 8:
               default:
                                          default:
         fprintf(stderr,
                  if (needs_default_attribute_values()) {
            /* Set up the default attribute values in case any of the vertex
   * elements use them.
   */
   uint32_t *attrs;
                        for (int i = 0; i < V3D_MAX_VS_INPUTS / 4; i++) {
            attrs[i * 4 + 0] = 0;
   attrs[i * 4 + 1] = 0;
   attrs[i * 4 + 2] = 0;
   if (i < so->num_elements &&
      util_format_is_pure_integer(so->pipe[i].src_format)) {
      } else {
   } else {
                        u_upload_unmap(v3d->state_uploader);
   }
      static void
   v3d_vertex_state_delete(struct pipe_context *pctx, void *hwcso)
   {
                  pipe_resource_reference(&so->defaults, NULL);
   }
      static void
   v3d_vertex_state_bind(struct pipe_context *pctx, void *hwcso)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   v3d->vtx = hwcso;
   }
      static void
   v3d_set_constant_buffer(struct pipe_context *pctx, enum pipe_shader_type shader, uint index,
               {
         struct v3d_context *v3d = v3d_context(pctx);
                     /* Note that the gallium frontend can unbind constant buffers by
      * passing NULL here.
      if (unlikely(!cb)) {
            so->enabled_mask &= ~(1 << index);
               so->enabled_mask |= 1 << index;
   so->dirty_mask |= 1 << index;
   }
      static void
   v3d_set_framebuffer_state(struct pipe_context *pctx,
         {
         struct v3d_context *v3d = v3d_context(pctx);
                              v3d->swap_color_rb = 0;
   v3d->blend_dst_alpha_one = 0;
   for (int i = 0; i < v3d->framebuffer.nr_cbufs; i++) {
            struct pipe_surface *cbuf = v3d->framebuffer.cbufs[i];
                                       /* For BGRA8 formats (DRI window system default format), we
   * need to swap R and B, since the HW's format is RGBA8.  On
   * V3D 4.1+, the RCL can swap R and B on load/store.
                                    }
      static enum V3DX(Wrap_Mode)
   translate_wrap(uint32_t pipe_wrap)
   {
         switch (pipe_wrap) {
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
         default:
         }
      #if V3D_VERSION >= 40
   static void
   v3d_upload_sampler_state_variant(void *map,
               {
         v3dx_pack(map, SAMPLER_STATE, sampler) {
                                          sampler.fixed_bias = cso->lod_bias;
   sampler.depth_compare_function = cso->compare_mode ?
               sampler.min_filter_nearest =
         sampler.mag_filter_nearest =
                        sampler.min_level_of_detail = MIN2(MAX2(0, cso->min_lod),
                        /* If we're not doing inter-miplevel filtering, we need to
   * clamp the LOD so that we only sample from baselevel.
   * However, we need to still allow the calculated LOD to be
   * fractionally over the baselevel, so that the HW can decide
   * between the min and mag filters.
   */
   if (cso->min_mip_filter == PIPE_TEX_MIPFILTER_NONE) {
            sampler.min_level_of_detail =
                                             if (cso->max_anisotropy > 8)
         else if (cso->max_anisotropy > 4)
                     if (variant == V3D_SAMPLER_STATE_BORDER_0000) {
         } else if (variant == V3D_SAMPLER_STATE_BORDER_0001) {
         } else if (variant == V3D_SAMPLER_STATE_BORDER_1111) {
                                          /* First, reswizzle the border color for any
   * mismatching we're doing between the texture's
   * channel order in hardware (R) versus what it is at
   * the GL level (ALPHA)
   */
   switch (variant) {
   case V3D_SAMPLER_STATE_F16_BGRA:
   case V3D_SAMPLER_STATE_F16_BGRA_UNORM:
   case V3D_SAMPLER_STATE_F16_BGRA_SNORM:
                                    case V3D_SAMPLER_STATE_F16_A:
   case V3D_SAMPLER_STATE_F16_A_UNORM:
   case V3D_SAMPLER_STATE_F16_A_SNORM:
   case V3D_SAMPLER_STATE_32_A:
   case V3D_SAMPLER_STATE_32_A_UNORM:
   case V3D_SAMPLER_STATE_32_A_SNORM:
                                    case V3D_SAMPLER_STATE_F16_LA:
   case V3D_SAMPLER_STATE_F16_LA_UNORM:
   case V3D_SAMPLER_STATE_F16_LA_SNORM:
                                                         /* Perform any clamping. */
   switch (variant) {
   case V3D_SAMPLER_STATE_F16_UNORM:
   case V3D_SAMPLER_STATE_F16_BGRA_UNORM:
   case V3D_SAMPLER_STATE_F16_A_UNORM:
   case V3D_SAMPLER_STATE_F16_LA_UNORM:
   case V3D_SAMPLER_STATE_32_UNORM:
                              case V3D_SAMPLER_STATE_F16_SNORM:
   case V3D_SAMPLER_STATE_F16_BGRA_SNORM:
   case V3D_SAMPLER_STATE_F16_A_SNORM:
   case V3D_SAMPLER_STATE_F16_LA_SNORM:
   case V3D_SAMPLER_STATE_32_SNORM:
                              case V3D_SAMPLER_STATE_1010102U:
         border.ui[0] = CLAMP(border.ui[0],
         border.ui[1] = CLAMP(border.ui[1],
                                    case V3D_SAMPLER_STATE_16U:
                              case V3D_SAMPLER_STATE_16I:
                              case V3D_SAMPLER_STATE_8U:
                              case V3D_SAMPLER_STATE_8I:
                           #if V3D_VERSION <= 42
                           /* The TMU in V3D 7.x always takes 32-bit floats and handles conversions
   * for us. In V3D 4.x we need to manually convert floating point color
   * values to the expected format.
   */
   if (variant < V3D_SAMPLER_STATE_32) {
         #endif
                           sampler.border_color_word_0 = border.ui[0];
   sampler.border_color_word_1 = border.ui[1];
   }
   #endif
      static void *
   v3d_create_sampler_state(struct pipe_context *pctx,
         {
         UNUSED struct v3d_context *v3d = v3d_context(pctx);
            if (!so)
                     enum V3DX(Wrap_Mode) wrap_s = translate_wrap(cso->wrap_s);
   enum V3DX(Wrap_Mode) wrap_t = translate_wrap(cso->wrap_t);
      #if V3D_VERSION >= 40
         bool uses_border_color = (wrap_s == V3D_WRAP_MODE_BORDER ||
                           /* This is the variant with the default hardware settings */
            if (uses_border_color) {
            if (cso->border_color.ui[0] == 0 &&
      cso->border_color.ui[1] == 0 &&
   cso->border_color.ui[2] == 0 &&
   cso->border_color.ui[3] == 0) {
      } else if (cso->border_color.ui[0] == 0 &&
               cso->border_color.ui[1] == 0 &&
   cso->border_color.ui[2] == 0 &&
   } else if (cso->border_color.ui[0] == 0x3F800000 &&
               cso->border_color.ui[1] == 0x3F800000 &&
   cso->border_color.ui[2] == 0x3F800000 &&
   } else {
               void *map;
   int sampler_align = so->border_color_variants ? 32 : 8;
   int sampler_size = align(cl_packet_length(SAMPLER_STATE), sampler_align);
   int num_variants = (so->border_color_variants ? ARRAY_SIZE(so->sampler_state_offset) : 1);
   u_upload_alloc(v3d->state_uploader, 0,
                  sampler_size * num_variants,
               for (int i = 0; i < num_variants; i++) {
            so->sampler_state_offset[i] =
         v3d_upload_sampler_state_variant(map + i * sampler_size,
         #else /* V3D_VERSION < 40 */
         v3dx_pack(&so->p0, TEXTURE_UNIFORM_PARAMETER_0_CFG_MODE1, p0) {
            p0.s_wrap_mode = wrap_s;
               v3dx_pack(&so->texture_shader_state, TEXTURE_SHADER_STATE, tex) {
            tex.depth_compare_function = cso->compare_mode ?
            #endif /* V3D_VERSION < 40 */
         }
      static void
   v3d_sampler_states_bind(struct pipe_context *pctx,
               {
         struct v3d_context *v3d = v3d_context(pctx);
            assert(start == 0);
   unsigned i;
            for (i = 0; i < nr; i++) {
            if (hwcso[i])
               for (; i < stage_tex->num_samplers; i++) {
                           }
      static void
   v3d_sampler_state_delete(struct pipe_context *pctx,
         {
         struct pipe_sampler_state *psampler = hwcso;
            pipe_resource_reference(&sampler->sampler_state, NULL);
   }
      static void
   v3d_setup_texture_shader_state_from_buffer(struct V3DX(TEXTURE_SHADER_STATE) *tex,
                           {
                  tex->image_depth = 1;
            /* On 4.x, the height of a 1D texture is redefined to be the
      * upper 14 bits of the width (which is only usable with txf).
               tex->image_width &= (1 << 14) - 1;
            /* Note that we don't have a job to reference the texture's sBO
      * at state create time, so any time this sampler view is used
   * we need to add the texture to the job.
      tex->texture_base_pointer =
   }
      static void
   v3d_setup_texture_shader_state(const struct v3d_device_info *devinfo,
                                 {
         /* Due to ARB_texture_view, a cubemap array can be seen as 2D texture
      * array.
      assert(!sampling_cube_array ||
                  struct v3d_resource *rsc = v3d_resource(prsc);
            tex->image_width = prsc->width0 * msaa_scale;
      #if V3D_VERSION >= 40
         /* On 4.x, the height of a 1D texture is redefined to be the
      * upper 14 bits of the width (which is only usable with txf).
      if (prsc->target == PIPE_TEXTURE_1D ||
         prsc->target == PIPE_TEXTURE_1D_ARRAY) {
            tex->image_width &= (1 << 14) - 1;
   #endif
            if (prsc->target == PIPE_TEXTURE_3D) {
         } else {
                  /* Empirical testing with CTS shows that when we are sampling from
      * cube arrays we want to set image depth to layers / 6, but not when
   * doing image load/store or sampling from 2d image arrays.
      if (sampling_cube_array) {
                           #if V3D_VERSION >= 40
         tex->max_level = last_level;
   /* Note that we don't have a job to reference the texture's sBO
      * at state create time, so any time this sampler view is used
   * we need to add the texture to the job.
      const uint32_t base_offset = rsc->bo->offset +
            #endif
               #if V3D_VERSION >= 71
         tex->chroma_offset_x = 1;
   tex->chroma_offset_y = 1;
   /* See comment in XML field definition for rationale of the shifts */
   tex->texture_base_pointer_cb = base_offset >> 6;
   #endif
            /* Since other platform devices may produce UIF images even
      * when they're not big enough for V3D to assume they're UIF,
   * we force images with level 0 as UIF to be always treated
   * that way.
      tex->level_0_is_strictly_uif =
                        if (tex->level_0_is_strictly_uif)
      #if V3D_VERSION >= 40
         if (tex->uif_xor_disable ||
         tex->level_0_is_strictly_uif) {
   #endif /* V3D_VERSION >= 40 */
   }
      void
   v3dX(create_texture_shader_state_bo)(struct v3d_context *v3d,
         {
         struct pipe_resource *prsc = so->texture;
   struct v3d_resource *rsc = v3d_resource(prsc);
   const struct pipe_sampler_view *cso = &so->base;
                        #if V3D_VERSION >= 40
         v3d_bo_unreference(&so->bo);
   so->bo = v3d_bo_alloc(v3d->screen,
         #else /* V3D_VERSION < 40 */
         STATIC_ASSERT(sizeof(so->texture_shader_state) >=
         #endif
            v3dx_pack(map, TEXTURE_SHADER_STATE, tex) {
            if (prsc->target != PIPE_BUFFER) {
            v3d_setup_texture_shader_state(&v3d->screen->devinfo,
                              } else {
            v3d_setup_texture_shader_state_from_buffer(&tex, prsc,
            #if V3D_VERSION <= 42
         #endif
   #if V3D_VERSION >= 71
         #endif
      #if V3D_VERSION >= 40
                  tex.swizzle_r = v3d_translate_pipe_swizzle(so->swizzle[0]);
      #endif
                     if (prsc->nr_samples > 1 && V3D_VERSION < 40) {
            /* Using texture views to reinterpret formats on our
   * MSAA textures won't work, because we don't lay out
   * the bits in memory as it's expected -- for example,
   * RGBA8 and RGB10_A2 are compatible in the
   * ARB_texture_view spec, but in HW we lay them out as
   * 32bpp RGBA8 and 64bpp RGBA16F.  Just assert for now
   * to catch failures.
   *
   * We explicitly allow remapping S8Z24 to RGBA8888 for
   * v3d_blit.c's stencil blits.
   */
   assert((util_format_linear(cso->format) ==
         util_format_linear(prsc->format)) ||
   (prsc->format == PIPE_FORMAT_S8_UINT_Z24_UNORM &&
   uint32_t output_image_format =
         uint32_t internal_type;
                              switch (internal_type) {
   case V3D_INTERNAL_TYPE_8:
         tex.texture_type = TEXTURE_DATA_FORMAT_RGBA8;
   case V3D_INTERNAL_TYPE_16F:
                                    /* sRGB was stored in the tile buffer as linear and
   * would have been encoded to sRGB on resolved tile
   #if V3D_VERSION <= 42
         #endif
                     } else {
                     }
      static struct pipe_sampler_view *
   v3d_create_sampler_view(struct pipe_context *pctx, struct pipe_resource *prsc,
         {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_screen *screen = v3d->screen;
   struct v3d_sampler_view *so = CALLOC_STRUCT(v3d_sampler_view);
            if (!so)
                              /* Compute the sampler view's swizzle up front. This will be plugged
      * into either the sampler (for 16-bit returns) or the shader's
   * texture key (for 32)
      uint8_t view_swizzle[4] = {
            cso->swizzle_r,
   cso->swizzle_g,
      };
   const uint8_t *fmt_swizzle =
                  pipe_reference_init(&so->base.reference, 1);
   so->base.texture = prsc;
            if (rsc->separate_stencil &&
         cso->format == PIPE_FORMAT_X32_S8X24_UINT) {
                  /* If we're sampling depth from depth/stencil, demote the format to
      * just depth.  u_format will end up giving the answers for the
   * stencil channel, otherwise.
      enum pipe_format sample_format = cso->format;
   if (sample_format == PIPE_FORMAT_S8_UINT_Z24_UNORM)
      #if V3D_VERSION >= 40
         const struct util_format_description *desc =
            if (util_format_is_pure_integer(sample_format) &&
         !util_format_has_depth(desc)) {
      int chan = util_format_get_first_non_void_channel(sample_format);
   if (util_format_is_pure_uint(sample_format)) {
            switch (desc->channel[chan].size) {
   case 32:
         so->sampler_variant = V3D_SAMPLER_STATE_32;
   case 16:
         so->sampler_variant = V3D_SAMPLER_STATE_16U;
   case 10:
         so->sampler_variant = V3D_SAMPLER_STATE_1010102U;
   case 8:
            } else {
            switch (desc->channel[chan].size) {
   case 32:
         so->sampler_variant = V3D_SAMPLER_STATE_32;
   case 16:
         so->sampler_variant = V3D_SAMPLER_STATE_16I;
   case 8:
         } else {
            if (v3d_get_tex_return_size(&screen->devinfo, sample_format) == 32) {
            if (util_format_is_alpha(sample_format))
            } else {
            if (util_format_is_luminance_alpha(sample_format))
         else if (util_format_is_alpha(sample_format))
                                    if (util_format_is_unorm(sample_format)) {
               } else if (util_format_is_snorm(sample_format)){
            #endif
            /* V3D still doesn't support sampling from raster textures, so we will
      * have to copy to a temporary tiled texture.
      if (!rsc->tiled && !(prsc->target == PIPE_TEXTURE_1D ||
                        struct v3d_resource *shadow_parent = rsc;
   struct pipe_resource tmpl = {
            .target = prsc->target,
   .format = prsc->format,
   .width0 = u_minify(prsc->width0,
         .height0 = u_minify(prsc->height0,
         .depth0 = 1,
   .array_size = 1,
                     /* Create the shadow texture.  The rest of the sampler view
   * setup will use the shadow.
   */
   prsc = v3d_resource_create(pctx->screen, &tmpl);
   if (!prsc) {
                                                } else {
                           }
      static void
   v3d_sampler_view_destroy(struct pipe_context *pctx,
         {
                  v3d_bo_unreference(&sview->bo);
   pipe_resource_reference(&psview->texture, NULL);
   pipe_resource_reference(&sview->texture, NULL);
   }
      static void
   v3d_set_sampler_views(struct pipe_context *pctx,
                        enum pipe_shader_type shader,
      {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_texture_stateobj *stage_tex = &v3d->tex[shader];
   unsigned i;
                     for (i = 0; i < nr; i++) {
            if (views[i])
         if (take_ownership) {
               } else {
         }
   /* If our sampler serial doesn't match our texture serial it
   * means the texture has been updated with a new BO, in which
   * case we need to update the sampler state to point to the
   * new BO as well
   */
   if (stage_tex->textures[i]) {
            struct v3d_sampler_view *so =
         struct v3d_resource *rsc = v3d_resource(so->texture);
            for (; i < stage_tex->num_textures; i++) {
                           }
      static struct pipe_stream_output_target *
   v3d_create_stream_output_target(struct pipe_context *pctx,
                     {
                  target = CALLOC_STRUCT(v3d_stream_output_target);
   if (!target)
            pipe_reference_init(&target->base.reference, 1);
            target->base.context = pctx;
   target->base.buffer_offset = buffer_offset;
            }
      static void
   v3d_stream_output_target_destroy(struct pipe_context *pctx,
         {
         pipe_resource_reference(&target->buffer, NULL);
   }
      static void
   v3d_set_stream_output_targets(struct pipe_context *pctx,
                     {
         struct v3d_context *ctx = v3d_context(pctx);
   struct v3d_streamout_stateobj *so = &ctx->streamout;
                     /* Update recorded vertex counts when we are ending the recording of
      * transform feedback. We do this when we switch primitive types
   * at draw time, but if we haven't switched primitives in our last
   * draw we need to do it here as well.
      if (num_targets == 0 && so->num_targets > 0)
            /* If offset is (unsigned) -1, it means continue appending to the
      * buffer at the existing offset.
      for (i = 0; i < num_targets; i++) {
                                 for (; i < so->num_targets; i++)
                     /* Create primitive counters BO if needed */
   if (num_targets > 0)
            }
      static void
   v3d_set_shader_buffers(struct pipe_context *pctx,
                           {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_ssbo_stateobj *so = &v3d->ssbo[shader];
            if (buffers) {
                                                                                                   if (buf->buffer)
         } else {
                                                               }
      static void
   v3d_create_image_view_texture_shader_state(struct v3d_context *v3d,
               {
   #if V3D_VERSION >= 40
                  void *map;
   u_upload_alloc(v3d->uploader, 0, cl_packet_length(TEXTURE_SHADER_STATE),
                                       v3dx_pack(map, TEXTURE_SHADER_STATE, tex) {
            if (prsc->target != PIPE_BUFFER) {
            v3d_setup_texture_shader_state(&v3d->screen->devinfo,
                              } else {
            v3d_setup_texture_shader_state_from_buffer(&tex, prsc,
                     tex.swizzle_r = v3d_translate_pipe_swizzle(PIPE_SWIZZLE_X);
                           #else /* V3D_VERSION < 40 */
         /* V3D 3.x doesn't use support shader image load/store operations on
      * textures, so it would get lowered in the shader to general memory
      #endif
   }
      static void
   v3d_set_shader_images(struct pipe_context *pctx,
                           {
         struct v3d_context *v3d = v3d_context(pctx);
            if (images) {
                                          if ((iview->base.resource == images[i].resource) &&
                                             if (iview->base.resource) {
         so->enabled_mask |= 1 << n;
   v3d_create_image_view_texture_shader_state(v3d,
         } else {
         } else {
                                                      if (count == 32)
                              if (unbind_num_trailing_slots) {
               }
      void
   v3dX(state_init)(struct pipe_context *pctx)
   {
         pctx->set_blend_color = v3d_set_blend_color;
   pctx->set_stencil_ref = v3d_set_stencil_ref;
   pctx->set_clip_state = v3d_set_clip_state;
   pctx->set_sample_mask = v3d_set_sample_mask;
   pctx->set_constant_buffer = v3d_set_constant_buffer;
   pctx->set_framebuffer_state = v3d_set_framebuffer_state;
   pctx->set_polygon_stipple = v3d_set_polygon_stipple;
   pctx->set_scissor_states = v3d_set_scissor_states;
                     pctx->create_blend_state = v3d_create_blend_state;
   pctx->bind_blend_state = v3d_blend_state_bind;
            pctx->create_rasterizer_state = v3d_create_rasterizer_state;
   pctx->bind_rasterizer_state = v3d_rasterizer_state_bind;
            pctx->create_depth_stencil_alpha_state = v3d_create_depth_stencil_alpha_state;
   pctx->bind_depth_stencil_alpha_state = v3d_zsa_state_bind;
            pctx->create_vertex_elements_state = v3d_vertex_state_create;
   pctx->delete_vertex_elements_state = v3d_vertex_state_delete;
            pctx->create_sampler_state = v3d_create_sampler_state;
   pctx->delete_sampler_state = v3d_sampler_state_delete;
            pctx->create_sampler_view = v3d_create_sampler_view;
   pctx->sampler_view_destroy = v3d_sampler_view_destroy;
            pctx->set_shader_buffers = v3d_set_shader_buffers;
            pctx->create_stream_output_target = v3d_create_stream_output_target;
   pctx->stream_output_target_destroy = v3d_stream_output_target_destroy;
   }
