   /*
   * Copyright (c) 2011-2013 Luc Verhaegen <libv@skynet.be>
   * Copyright (c) 2018-2019 Lima Project
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
      #include "util/compiler.h"
   #include "util/u_memory.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_math.h"
   #include "util/u_debug.h"
   #include "util/u_transfer.h"
      #include "lima_bo.h"
   #include "lima_context.h"
   #include "lima_screen.h"
   #include "lima_texture.h"
   #include "lima_resource.h"
   #include "lima_job.h"
   #include "lima_util.h"
   #include "lima_format.h"
      #include <drm-uapi/lima_drm.h>
         #define lima_tex_list_size 64
      static_assert(offsetof(lima_tex_desc, va) == 24, "lima_tex_desc->va offset isn't 24");
         static void
   lima_texture_desc_set_va(lima_tex_desc *desc,
               {
      unsigned va_bit_idx = VA_BIT_OFFSET + (VA_BIT_SIZE * idx);
   unsigned va_idx = va_bit_idx / 32;
                     desc->va[va_idx] |= va << va_bit_idx;
   if (va_bit_idx <= 6)
            }
      /*
   * Note: this function is used by both draw and flush code path,
   * make sure no lima_job_get() is called inside this.
   */
   void
   lima_texture_desc_set_res(struct lima_context *ctx, lima_tex_desc *desc,
                     {
      unsigned width, height, depth, layout, i;
            width = prsc->width0;
   height = prsc->height0;
   depth = prsc->depth0;
   if (first_level != 0) {
      width = u_minify(width, first_level);
   height = u_minify(height, first_level);
               desc->format = lima_format_get_texel(prsc->format);
   desc->swap_r_b = lima_format_get_texel_swap_rb(prsc->format);
   desc->width  = width;
   desc->height = height;
            if (lima_res->tiled)
         else {
      desc->stride = lima_res->levels[first_level].stride;
   desc->has_stride = 1;
                        /* attach first level */
   uint32_t first_va = base_va + lima_res->levels[first_level].offset +
               desc->va_s.va_0 = first_va >> 6;
            /* Attach remaining levels.
   * Each subsequent mipmap address is specified using the 26 msbs.
   * These addresses are then packed continuously in memory */
   for (i = 1; i <= (last_level - first_level); i++) {
      uint32_t address = base_va + lima_res->levels[first_level + i].offset;
         }
      static unsigned
   pipe_wrap_to_lima(unsigned pipe_wrap, bool using_nearest)
   {
      switch (pipe_wrap) {
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_CLAMP:
      if (using_nearest)
         else
      case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_MIRROR_CLAMP:
      if (using_nearest)
         else
      case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
         default:
            }
      static void
   lima_update_tex_desc(struct lima_context *ctx, struct lima_sampler_state *sampler,
               {
      /* unit is 1/16 since lod_bias is in fixed format */
   int lod_bias_delta = 0;
   lima_tex_desc *desc = pdesc;
   unsigned first_level;
   unsigned last_level;
   unsigned first_layer;
                     if (!texture)
            switch (texture->base.target) {
   case PIPE_TEXTURE_1D:
      desc->sampler_dim = LIMA_SAMPLER_DIM_1D;
      case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      desc->sampler_dim = LIMA_SAMPLER_DIM_2D;
      case PIPE_TEXTURE_CUBE:
      desc->cube_map = 1;
      case PIPE_TEXTURE_3D:
      desc->sampler_dim = LIMA_SAMPLER_DIM_3D;
      default:
                  if (sampler->base.unnormalized_coords)
            first_level = texture->base.u.tex.first_level;
   last_level = texture->base.u.tex.last_level;
   first_layer = texture->base.u.tex.first_layer;
   if (last_level - first_level >= LIMA_MAX_MIP_LEVELS)
            desc->min_lod = lima_float_to_fixed8(sampler->base.min_lod);
   max_lod = MIN2(sampler->base.max_lod, sampler->base.min_lod +
         desc->max_lod = lima_float_to_fixed8(max_lod);
            switch (sampler->base.min_mip_filter) {
      case PIPE_TEX_MIPFILTER_LINEAR:
      desc->min_mipfilter_2 = 3;
      case PIPE_TEX_MIPFILTER_NEAREST:
      desc->min_mipfilter_2 = 0;
      case PIPE_TEX_MIPFILTER_NONE:
      desc->max_lod = desc->min_lod;
      default:
               switch (sampler->base.mag_img_filter) {
   case PIPE_TEX_FILTER_LINEAR:
      desc->mag_img_filter_nearest = 0;
      case PIPE_TEX_FILTER_NEAREST:
   default:
      desc->mag_img_filter_nearest = 1;
               switch (sampler->base.min_img_filter) {
         case PIPE_TEX_FILTER_LINEAR:
      desc->min_img_filter_nearest = 0;
      case PIPE_TEX_FILTER_NEAREST:
   default:
      lod_bias_delta = 8;
   desc->min_img_filter_nearest = 1;
               /* Panfrost mentions that GL_CLAMP is broken for NEAREST filter on Midgard,
   * looks like it also broken on Utgard, since it fails in piglit
   */
            desc->wrap_s = pipe_wrap_to_lima(sampler->base.wrap_s, using_nearest);
   desc->wrap_t = pipe_wrap_to_lima(sampler->base.wrap_t, using_nearest);
            desc->border_red = float_to_ushort(sampler->base.border_color.f[0]);
   desc->border_green = float_to_ushort(sampler->base.border_color.f[1]);
   desc->border_blue = float_to_ushort(sampler->base.border_color.f[2]);
            if (desc->min_img_filter_nearest && desc->mag_img_filter_nearest &&
      desc->min_mipfilter_2 == 0 &&
                        lima_texture_desc_set_res(ctx, desc, texture->base.texture,
      }
      static unsigned
   lima_calc_tex_desc_size(struct lima_sampler_view *texture)
   {
      unsigned size = offsetof(lima_tex_desc, va);
            if (!texture)
            unsigned first_level = texture->base.u.tex.first_level;
            if (last_level - first_level >= LIMA_MAX_MIP_LEVELS)
            va_bit_size = VA_BIT_OFFSET + VA_BIT_SIZE * (last_level - first_level + 1);
   size += (va_bit_size + 7) >> 3;
               }
      void
   lima_update_textures(struct lima_context *ctx)
   {
      struct lima_job *job = lima_job_get(ctx);
                     /* Nothing to do - we have no samplers or textures */
   if (!lima_tex->num_samplers || !lima_tex->num_textures)
            /* we always need to add texture bo to job */
   for (int i = 0; i < lima_tex->num_samplers; i++) {
      struct lima_sampler_view *texture = lima_sampler_view(lima_tex->textures[i]);
   if (!texture)
         struct lima_resource *rsc = lima_resource(texture->base.texture);
   lima_flush_previous_job_writing_resource(ctx, texture->base.texture);
               /* do not regenerate texture desc if no change */
   if (!(ctx->dirty & LIMA_CONTEXT_DIRTY_TEXTURES))
            unsigned size = lima_tex_list_size;
   for (int i = 0; i < lima_tex->num_samplers; i++) {
      struct lima_sampler_view *texture = lima_sampler_view(lima_tex->textures[i]);
               uint32_t *descs =
            off_t offset = lima_tex_list_size;
   for (int i = 0; i < lima_tex->num_samplers; i++) {
      struct lima_sampler_state *sampler = lima_sampler_state(lima_tex->samplers[i]);
   struct lima_sampler_view *texture = lima_sampler_view(lima_tex->textures[i]);
            descs[i] = lima_ctx_buff_va(ctx, lima_ctx_buff_pp_tex_desc) + offset;
   lima_update_tex_desc(ctx, sampler, texture, (void *)descs + offset, desc_size);
               lima_dump_command_stream_print(
      job->dump, descs, size, false, "add textures_desc at va %x\n",
         lima_dump_texture_descriptor(
      job->dump, descs, size,
   lima_ctx_buff_va(ctx, lima_ctx_buff_pp_tex_desc) + lima_tex_list_size,
   }
