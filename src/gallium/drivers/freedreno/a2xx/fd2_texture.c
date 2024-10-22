   /*
   * Copyright (C) 2012-2013 Rob Clark <robclark@freedesktop.org>
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "pipe/p_state.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "fd2_texture.h"
   #include "fd2_util.h"
      static enum sq_tex_clamp
   tex_clamp(unsigned wrap)
   {
      switch (wrap) {
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_CLAMP:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
         case PIPE_TEX_WRAP_MIRROR_CLAMP:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
         default:
      DBG("invalid wrap: %u", wrap);
         }
      static enum sq_tex_filter
   tex_filter(unsigned filter)
   {
      switch (filter) {
   case PIPE_TEX_FILTER_NEAREST:
         case PIPE_TEX_FILTER_LINEAR:
         default:
      DBG("invalid filter: %u", filter);
         }
      static enum sq_tex_filter
   mip_filter(unsigned filter)
   {
      switch (filter) {
   case PIPE_TEX_MIPFILTER_NONE:
         case PIPE_TEX_MIPFILTER_NEAREST:
         case PIPE_TEX_MIPFILTER_LINEAR:
         default:
      DBG("invalid filter: %u", filter);
         }
      static void *
   fd2_sampler_state_create(struct pipe_context *pctx,
         {
               if (!so)
                     /* TODO
   * cso->max_anisotropy
   * cso->unnormalized_coords (dealt with by shader for rect textures?)
            /* SQ_TEX0_PITCH() must be OR'd in later when we know the bound texture: */
   so->tex0 = A2XX_SQ_TEX_0_CLAMP_X(tex_clamp(cso->wrap_s)) |
                  so->tex3 = A2XX_SQ_TEX_3_XY_MAG_FILTER(tex_filter(cso->mag_img_filter)) |
                  so->tex4 = 0;
   if (cso->min_mip_filter != PIPE_TEX_MIPFILTER_NONE)
               }
      static void
   fd2_sampler_states_bind(struct pipe_context *pctx, enum pipe_shader_type shader,
         {
      if (!hwcso)
            if (shader == PIPE_SHADER_FRAGMENT) {
               /* on a2xx, since there is a flat address space for textures/samplers,
   * a change in # of fragment textures/samplers will trigger patching and
   * re-emitting the vertex shader:
   */
   if (nr != ctx->tex[PIPE_SHADER_FRAGMENT].num_samplers)
                  }
      static enum sq_tex_dimension
   tex_dimension(unsigned target)
   {
      switch (target) {
   default:
         case PIPE_TEXTURE_1D:
      assert(0); /* TODO */
      case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D:
         case PIPE_TEXTURE_3D:
      assert(0); /* TODO */
      case PIPE_TEXTURE_CUBE:
            }
      static struct pipe_sampler_view *
   fd2_sampler_view_create(struct pipe_context *pctx, struct pipe_resource *prsc,
         {
      struct fd2_pipe_sampler_view *so = CALLOC_STRUCT(fd2_pipe_sampler_view);
   struct fd_resource *rsc = fd_resource(prsc);
            if (!so)
            so->base = *cso;
   pipe_reference(NULL, &prsc->reference);
   so->base.texture = prsc;
   so->base.reference.count = 1;
            so->tex0 = A2XX_SQ_TEX_0_SIGN_X(fmt.sign) | A2XX_SQ_TEX_0_SIGN_Y(fmt.sign) |
            A2XX_SQ_TEX_0_SIGN_Z(fmt.sign) | A2XX_SQ_TEX_0_SIGN_W(fmt.sign) |
   A2XX_SQ_TEX_0_PITCH(fdl2_pitch_pixels(&rsc->layout, 0) *
      so->tex1 = A2XX_SQ_TEX_1_FORMAT(fmt.format) |
         so->tex2 = A2XX_SQ_TEX_2_HEIGHT(prsc->height0 - 1) |
         so->tex3 = A2XX_SQ_TEX_3_NUM_FORMAT(fmt.num_format) |
            fd2_tex_swiz(cso->format, cso->swizzle_r, cso->swizzle_g,
         so->tex4 = A2XX_SQ_TEX_4_MIP_MIN_LEVEL(fd_sampler_first_level(cso)) |
                        }
      static void
   fd2_set_sampler_views(struct pipe_context *pctx, enum pipe_shader_type shader,
                           {
      if (shader == PIPE_SHADER_FRAGMENT) {
               /* on a2xx, since there is a flat address space for textures/samplers,
   * a change in # of fragment textures/samplers will trigger patching and
   * re-emitting the vertex shader:
   */
   if (nr != ctx->tex[PIPE_SHADER_FRAGMENT].num_textures)
               fd_set_sampler_views(pctx, shader, start, nr, unbind_num_trailing_slots,
      }
      /* map gallium sampler-id to hw const-idx.. adreno uses a flat address
   * space of samplers (const-idx), so we need to map the gallium sampler-id
   * which is per-shader to a global const-idx space.
   *
   * Fragment shader sampler maps directly to const-idx, and vertex shader
   * is offset by the # of fragment shader samplers.  If the # of fragment
   * shader samplers changes, this shifts the vertex shader indexes.
   *
   * TODO maybe we can do frag shader 0..N  and vert shader N..0 to avoid
   * this??
   */
   unsigned
   fd2_get_const_idx(struct fd_context *ctx, struct fd_texture_stateobj *tex,
         {
      if (tex == &ctx->tex[PIPE_SHADER_FRAGMENT])
            }
      void
   fd2_texture_init(struct pipe_context *pctx)
   {
      pctx->create_sampler_state = fd2_sampler_state_create;
   pctx->bind_sampler_states = fd2_sampler_states_bind;
   pctx->create_sampler_view = fd2_sampler_view_create;
      }
