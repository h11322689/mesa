   /*
   * Copyright 2012 Red Hat Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   *
   * Authors: Ben Skeggs
   *
   */
      #include "util/u_inlines.h"
   #include "util/format/u_format.h"
      #include "nv_object.xml.h"
   #include "nv30/nv30-40_3d.xml.h"
   #include "nv30/nv30_context.h"
   #include "nv30/nv30_format.h"
      #define NV30_3D_TEX_WRAP_S_MIRROR_REPEAT NV30_3D_TEX_WRAP_S_MIRRORED_REPEAT
   #define NV30_WRAP(n) \
         #define NV40_WRAP(n) \
            static inline unsigned
   wrap_mode(unsigned pipe)
   {
               switch (pipe) {
   NV30_WRAP(REPEAT);
   NV30_WRAP(MIRROR_REPEAT);
   NV30_WRAP(CLAMP_TO_EDGE);
   NV30_WRAP(CLAMP_TO_BORDER);
   NV30_WRAP(CLAMP);
   NV40_WRAP(MIRROR_CLAMP_TO_EDGE);
   NV40_WRAP(MIRROR_CLAMP_TO_BORDER);
   NV40_WRAP(MIRROR_CLAMP);
   default:
                     }
      static inline unsigned
   filter_mode(const struct pipe_sampler_state *cso)
   {
               switch (cso->mag_img_filter) {
   case PIPE_TEX_FILTER_LINEAR:
      filter = NV30_3D_TEX_FILTER_MAG_LINEAR;
      default:
      filter = NV30_3D_TEX_FILTER_MAG_NEAREST;
               switch (cso->min_img_filter) {
   case PIPE_TEX_FILTER_LINEAR:
      switch (cso->min_mip_filter) {
   case PIPE_TEX_MIPFILTER_NEAREST:
      filter |= NV30_3D_TEX_FILTER_MIN_LINEAR_MIPMAP_NEAREST;
      case PIPE_TEX_MIPFILTER_LINEAR:
      filter |= NV30_3D_TEX_FILTER_MIN_LINEAR_MIPMAP_LINEAR;
      default:
      filter |= NV30_3D_TEX_FILTER_MIN_LINEAR;
      }
      default:
      switch (cso->min_mip_filter) {
   case PIPE_TEX_MIPFILTER_NEAREST:
      filter |= NV30_3D_TEX_FILTER_MIN_NEAREST_MIPMAP_NEAREST;
      case PIPE_TEX_MIPFILTER_LINEAR:
      filter |= NV30_3D_TEX_FILTER_MIN_NEAREST_MIPMAP_LINEAR;
      default:
      filter |= NV30_3D_TEX_FILTER_MIN_NEAREST;
      }
                  }
      static inline unsigned
   compare_mode(const struct pipe_sampler_state *cso)
   {
      if (cso->compare_mode != PIPE_TEX_COMPARE_R_TO_TEXTURE)
            switch (cso->compare_func) {
   case PIPE_FUNC_NEVER   : return NV30_3D_TEX_WRAP_RCOMP_NEVER;
   case PIPE_FUNC_GREATER : return NV30_3D_TEX_WRAP_RCOMP_GREATER;
   case PIPE_FUNC_EQUAL   : return NV30_3D_TEX_WRAP_RCOMP_EQUAL;
   case PIPE_FUNC_GEQUAL  : return NV30_3D_TEX_WRAP_RCOMP_GEQUAL;
   case PIPE_FUNC_LESS    : return NV30_3D_TEX_WRAP_RCOMP_LESS;
   case PIPE_FUNC_NOTEQUAL: return NV30_3D_TEX_WRAP_RCOMP_NOTEQUAL;
   case PIPE_FUNC_LEQUAL  : return NV30_3D_TEX_WRAP_RCOMP_LEQUAL;
   case PIPE_FUNC_ALWAYS  : return NV30_3D_TEX_WRAP_RCOMP_ALWAYS;
   default:
            }
      static void *
   nv30_sampler_state_create(struct pipe_context *pipe,
         {
      struct nouveau_object *eng3d = nv30_context(pipe)->screen->eng3d;
   struct nv30_sampler_state *so;
            so = MALLOC_STRUCT(nv30_sampler_state);
   if (!so)
            so->pipe  = *cso;
   so->fmt   = 0;
   so->wrap  = (wrap_mode(cso->wrap_s) << NV30_3D_TEX_WRAP_S__SHIFT) |
               so->en    = 0;
   so->wrap |= compare_mode(cso);
   so->filt  = filter_mode(cso) | 0x00002000;
   so->bcol  = (float_to_ubyte(cso->border_color.f[3]) << 24) |
                        if (eng3d->oclass >= NV40_3D_CLASS) {
               if (cso->unnormalized_coords)
            if (aniso > 1) {
      if      (aniso >= 16) so->en |= NV40_3D_TEX_ENABLE_ANISO_16X;
   else if (aniso >= 12) so->en |= NV40_3D_TEX_ENABLE_ANISO_12X;
   else if (aniso >= 10) so->en |= NV40_3D_TEX_ENABLE_ANISO_10X;
   else if (aniso >=  8) so->en |= NV40_3D_TEX_ENABLE_ANISO_8X;
   else if (aniso >=  6) so->en |= NV40_3D_TEX_ENABLE_ANISO_6X;
                        } else {
               if      (cso->max_anisotropy >= 8) so->en |= NV30_3D_TEX_ENABLE_ANISO_8X;
   else if (cso->max_anisotropy >= 4) so->en |= NV30_3D_TEX_ENABLE_ANISO_4X;
               so->filt |= (int)(cso->lod_bias * 256.0) & 0x1fff;
   so->max_lod = (int)(CLAMP(cso->max_lod, 0.0, max_lod) * 256.0);
   so->min_lod = (int)(CLAMP(cso->min_lod, 0.0, max_lod) * 256.0);
      }
      static void
   nv30_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
   {
         }
      static void
   nv30_bind_sampler_states(struct pipe_context *pipe,
               {
      switch (shader) {
   case PIPE_SHADER_VERTEX:
      nv40_verttex_sampler_states_bind(pipe, num_samplers, samplers);
      case PIPE_SHADER_FRAGMENT:
      nv30_fragtex_sampler_states_bind(pipe, num_samplers, samplers);
      default:
      assert(!"unexpected shader type");
         }
      static inline uint32_t
   swizzle(const struct nv30_texfmt *fmt, unsigned cmp, unsigned swz)
   {
      uint32_t data = fmt->swz[swz].src << 8;
   if (swz <= PIPE_SWIZZLE_W)
         else
            }
      static struct pipe_sampler_view *
   nv30_sampler_view_create(struct pipe_context *pipe, struct pipe_resource *pt,
         {
      const struct nv30_texfmt *fmt = nv30_texfmt(pipe->screen, tmpl->format);
   struct nouveau_object *eng3d = nv30_context(pipe)->screen->eng3d;
   struct nv30_miptree *mt = nv30_miptree(pt);
            so = MALLOC_STRUCT(nv30_sampler_view);
   if (!so)
         so->pipe = *tmpl;
   so->pipe.reference.count = 1;
   so->pipe.texture = NULL;
   so->pipe.context = pipe;
            so->fmt = NV30_3D_TEX_FORMAT_NO_BORDER;
   switch (pt->target) {
   case PIPE_TEXTURE_1D:
      so->fmt |= NV30_3D_TEX_FORMAT_DIMS_1D;
      case PIPE_TEXTURE_CUBE:
      so->fmt |= NV30_3D_TEX_FORMAT_CUBIC;
      case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      so->fmt |= NV30_3D_TEX_FORMAT_DIMS_2D;
      case PIPE_TEXTURE_3D:
      so->fmt |= NV30_3D_TEX_FORMAT_DIMS_3D;
      default:
      assert(0);
   so->fmt |= NV30_3D_TEX_FORMAT_DIMS_1D;
               so->filt = fmt->filter;
   so->wrap = fmt->wrap;
   so->swz  = fmt->swizzle;
   so->swz |= swizzle(fmt, 3, tmpl->swizzle_a);
   so->swz |= swizzle(fmt, 0, tmpl->swizzle_r) << 2;
   so->swz |= swizzle(fmt, 1, tmpl->swizzle_g) << 4;
            /* apparently, we need to ignore the t coordinate for 1D textures to
   * fix piglit tex1d-2dborder
   */
   so->wrap_mask = ~0;
   if (pt->target == PIPE_TEXTURE_1D) {
      so->wrap_mask &= ~NV30_3D_TEX_WRAP_T__MASK;
               /* yet more hardware suckage, can't filter 32-bit float formats */
   switch (tmpl->format) {
   case PIPE_FORMAT_R32_FLOAT:
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      so->filt_mask = ~(NV30_3D_TEX_FILTER_MIN__MASK |
         so->filt     |= NV30_3D_TEX_FILTER_MIN_NEAREST |
            default:
      so->filt_mask = ~0;
               so->npot_size0 = (pt->width0 << 16) | pt->height0;
   if (eng3d->oclass >= NV40_3D_CLASS) {
      so->npot_size1 = (pt->depth0 << 20) | mt->uniform_pitch;
   if (mt->uniform_pitch)
         so->fmt |= 0x00008000;
      } else {
      so->swz |= mt->uniform_pitch << NV30_3D_TEX_SWIZZLE_RECT_PITCH__SHIFT;
   if (pt->last_level)
         so->fmt |= util_logbase2(pt->width0)  << 20;
   so->fmt |= util_logbase2(pt->height0) << 24;
   so->fmt |= util_logbase2(pt->depth0)  << 28;
               so->base_lod = so->pipe.u.tex.first_level << 8;
   so->high_lod = MIN2(pt->last_level, so->pipe.u.tex.last_level) << 8;
      }
      static void
   nv30_sampler_view_destroy(struct pipe_context *pipe,
         {
      pipe_resource_reference(&view->texture, NULL);
      }
      void
   nv30_texture_init(struct pipe_context *pipe)
   {
      pipe->create_sampler_state = nv30_sampler_state_create;
   pipe->delete_sampler_state = nv30_sampler_state_delete;
            pipe->create_sampler_view = nv30_sampler_view_create;
      }
