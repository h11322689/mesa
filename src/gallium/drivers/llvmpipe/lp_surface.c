   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
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
      #include "util/u_rect.h"
   #include "util/u_surface.h"
   #include "util/u_memset.h"
   #include "lp_context.h"
   #include "lp_flush.h"
   #include "lp_limits.h"
   #include "lp_surface.h"
   #include "lp_texture.h"
   #include "lp_query.h"
   #include "lp_rast.h"
         static void
   lp_resource_copy_ms(struct pipe_context *pipe,
                     struct pipe_resource *dst, unsigned dst_level,
   {
      struct pipe_box dst_box = *src_box;
   dst_box.x = dstx;
   dst_box.y = dsty;
                     for (unsigned i = 0; i < MAX2(src->nr_samples, dst->nr_samples); i++) {
      struct pipe_transfer *src_trans, *dst_trans;
   const uint8_t *src_map =
      llvmpipe_transfer_map_ms(pipe,src, 0, PIPE_MAP_READ,
            if (!src_map)
            uint8_t *dst_map = llvmpipe_transfer_map_ms(pipe,
                     if (!dst_map) {
      pipe->texture_unmap(pipe, src_trans);
               util_copy_box(dst_map,
               src_format,
   dst_trans->stride, dst_trans->layer_stride,
   0, 0, 0,
   src_box->width, src_box->height, src_box->depth,
   src_map,
   pipe->texture_unmap(pipe, dst_trans);
         }
         static void
   lp_resource_copy(struct pipe_context *pipe,
                  struct pipe_resource *dst, unsigned dst_level,
      {
      llvmpipe_flush_resource(pipe,
                                    llvmpipe_flush_resource(pipe,
                                    if (dst->nr_samples > 1 &&
      (dst->nr_samples == src->nr_samples ||
   (src->nr_samples == 1 && dst->nr_samples > 1))) {
   lp_resource_copy_ms(pipe, dst, dst_level, dstx, dsty, dstz,
            }
   util_resource_copy_region(pipe, dst, dst_level, dstx, dsty, dstz,
      }
         static void
   lp_blit(struct pipe_context *pipe,
         {
      struct llvmpipe_context *lp = llvmpipe_context(pipe);
            if (blit_info->render_condition_enable && !llvmpipe_check_render_cond(lp))
            if (util_try_blit_via_copy_region(pipe, &info,
                        if (blit_info->src.resource->format == blit_info->src.format &&
      blit_info->dst.resource->format == blit_info->dst.format &&
   blit_info->src.format == blit_info->dst.format &&
   blit_info->src.resource->nr_samples > 1 &&
   blit_info->dst.resource->nr_samples < 2 &&
   blit_info->sample0_only) {
   util_resource_copy_region(pipe, blit_info->dst.resource,
                                       if (!util_blitter_is_blit_supported(lp->blitter, &info)) {
      debug_printf("llvmpipe: blit unsupported %s -> %s\n",
                           /* for 32-bit unorm depth, avoid the conversions to float and back,
         if (blit_info->src.format == PIPE_FORMAT_Z32_UNORM &&
      blit_info->dst.format == PIPE_FORMAT_Z32_UNORM &&
   info.filter == PIPE_TEX_FILTER_NEAREST) {
   info.src.format = PIPE_FORMAT_R32_UINT;
   info.dst.format = PIPE_FORMAT_R32_UINT;
               util_blitter_save_vertex_buffer_slot(lp->blitter, lp->vertex_buffer);
   util_blitter_save_vertex_elements(lp->blitter, (void*)lp->velems);
   util_blitter_save_vertex_shader(lp->blitter, (void*)lp->vs);
   util_blitter_save_geometry_shader(lp->blitter, (void*)lp->gs);
   util_blitter_save_so_targets(lp->blitter, lp->num_so_targets,
         util_blitter_save_rasterizer(lp->blitter, (void*)lp->rasterizer);
   util_blitter_save_viewport(lp->blitter, &lp->viewports[0]);
   util_blitter_save_scissor(lp->blitter, &lp->scissors[0]);
   util_blitter_save_fragment_shader(lp->blitter, lp->fs);
   util_blitter_save_blend(lp->blitter, (void*)lp->blend);
   util_blitter_save_tessctrl_shader(lp->blitter, (void*)lp->tcs);
   util_blitter_save_tesseval_shader(lp->blitter, (void*)lp->tes);
   util_blitter_save_depth_stencil_alpha(lp->blitter,
         util_blitter_save_stencil_ref(lp->blitter, &lp->stencil_ref);
   util_blitter_save_sample_mask(lp->blitter, lp->sample_mask,
         util_blitter_save_framebuffer(lp->blitter, &lp->framebuffer);
   util_blitter_save_fragment_sampler_states(lp->blitter,
               util_blitter_save_fragment_sampler_views(lp->blitter,
               util_blitter_save_render_condition(lp->blitter, lp->render_cond_query,
                  }
         static void
   lp_flush_resource(struct pipe_context *ctx, struct pipe_resource *resource)
   {
         }
         static struct pipe_surface *
   llvmpipe_create_surface(struct pipe_context *pipe,
               {
      if (!(pt->bind & (PIPE_BIND_DEPTH_STENCIL | PIPE_BIND_RENDER_TARGET))) {
      debug_printf("Illegal surface creation without bind flag\n");
   if (util_format_is_depth_or_stencil(surf_tmpl->format)) {
         }
   else {
                     struct pipe_surface *ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      pipe_reference_init(&ps->reference, 1);
   pipe_resource_reference(&ps->texture, pt);
   ps->context = pipe;
   ps->format = surf_tmpl->format;
   if (llvmpipe_resource_is_texture(pt)) {
      assert(surf_tmpl->u.tex.level <= pt->last_level);
   assert(surf_tmpl->u.tex.first_layer <= surf_tmpl->u.tex.last_layer);
   ps->width = u_minify(pt->width0, surf_tmpl->u.tex.level);
   ps->height = u_minify(pt->height0, surf_tmpl->u.tex.level);
   ps->u.tex.level = surf_tmpl->u.tex.level;
   ps->u.tex.first_layer = surf_tmpl->u.tex.first_layer;
      } else {
      /* setting width as number of elements should get us correct
   * renderbuffer width
   */
   ps->width = surf_tmpl->u.buf.last_element
         ps->height = pt->height0;
   ps->u.buf.first_element = surf_tmpl->u.buf.first_element;
   ps->u.buf.last_element = surf_tmpl->u.buf.last_element;
   assert(ps->u.buf.first_element <= ps->u.buf.last_element);
   assert(util_format_get_blocksize(surf_tmpl->format) *
         }
      }
         static void
   llvmpipe_surface_destroy(struct pipe_context *pipe,
         {
      /* Effectively do the texture_update work here - if texture images
   * needed post-processing to put them into hardware layout, this is
   * where it would happen.  For llvmpipe, nothing to do.
   */
   assert(surf->texture);
   pipe_resource_reference(&surf->texture, NULL);
      }
         static void
   llvmpipe_get_sample_position(struct pipe_context *pipe,
                     {
      switch (sample_count) {
   case 4:
      out_value[0] = lp_sample_pos_4x[sample_index][0];
   out_value[1] = lp_sample_pos_4x[sample_index][1];
      default:
            }
         static void
   lp_clear_color_texture_helper(struct pipe_transfer *dst_trans,
                                 {
                                 util_fill_box(dst_map, format,
            }
         static void
   lp_clear_color_texture_msaa(struct pipe_context *pipe,
                                 {
      struct pipe_transfer *dst_trans;
            dst_map = llvmpipe_transfer_map_ms(pipe, texture, 0, PIPE_MAP_WRITE,
         if (!dst_map)
            if (dst_trans->stride > 0) {
      lp_clear_color_texture_helper(dst_trans, dst_map, format, color,
      }
      }
         static void
   llvmpipe_clear_render_target(struct pipe_context *pipe,
                                 {
               if (render_condition_enabled && !llvmpipe_check_render_cond(llvmpipe))
            width = MIN2(width, dst->texture->width0 - dstx);
            if (dst->texture->nr_samples > 1) {
      struct pipe_box box;
   u_box_2d(dstx, dsty, width, height, &box);
   if (dst->texture->target != PIPE_BUFFER) {
      box.z = dst->u.tex.first_layer;
      }
   for (unsigned s = 0; s < util_res_sample_count(dst->texture); s++) {
      lp_clear_color_texture_msaa(pipe, dst->texture, dst->format,
         } else {
      util_clear_render_target(pipe, dst, color,
         }
         static void
   lp_clear_depth_stencil_texture_msaa(struct pipe_context *pipe,
                                 {
      struct pipe_transfer *dst_trans;
            if ((clear_flags & PIPE_CLEAR_DEPTHSTENCIL) &&
      ((clear_flags & PIPE_CLEAR_DEPTHSTENCIL) != PIPE_CLEAR_DEPTHSTENCIL) &&
   util_format_is_depth_and_stencil(format)) {
               uint8_t *dst_map = llvmpipe_transfer_map_ms(pipe,
                                 assert(dst_map);
   if (!dst_map)
                     util_fill_zs_box(dst_map, format, need_rmw, clear_flags,
                     }
         static void
   llvmpipe_clear_depth_stencil(struct pipe_context *pipe,
                              struct pipe_surface *dst,
   unsigned clear_flags,
      {
               if (render_condition_enabled && !llvmpipe_check_render_cond(llvmpipe))
            width = MIN2(width, dst->texture->width0 - dstx);
            if (dst->texture->nr_samples > 1) {
      uint64_t zstencil = util_pack64_z_stencil(dst->format, depth, stencil);
   struct pipe_box box;
   u_box_2d(dstx, dsty, width, height, &box);
   if (dst->texture->target != PIPE_BUFFER) {
      box.z = dst->u.tex.first_layer;
      }
   for (unsigned s = 0; s < util_res_sample_count(dst->texture); s++)
      lp_clear_depth_stencil_texture_msaa(pipe, dst->texture,
         } else {
      util_clear_depth_stencil(pipe, dst, clear_flags,
               }
         static void
   llvmpipe_clear_texture(struct pipe_context *pipe,
                           {
      const struct util_format_description *desc =
         if (tex->nr_samples <= 1) {
      util_clear_texture_sw(pipe, tex, level, box, data);
      }
            if (util_format_is_depth_or_stencil(tex->format)) {
      unsigned clear = 0;
   float depth = 0.0f;
   uint8_t stencil = 0;
            if (util_format_has_depth(desc)) {
      clear |= PIPE_CLEAR_DEPTH;
               if (util_format_has_stencil(desc)) {
      clear |= PIPE_CLEAR_STENCIL;
                        for (unsigned s = 0; s < util_res_sample_count(tex); s++)
      lp_clear_depth_stencil_texture_msaa(pipe, tex, tex->format, clear,
   } else {
               for (unsigned s = 0; s < util_res_sample_count(tex); s++) {
      lp_clear_color_texture_msaa(pipe, tex, tex->format, &color, s,
            }
         static void
   llvmpipe_clear_buffer(struct pipe_context *pipe,
                        struct pipe_resource *res,
      {
      struct pipe_transfer *dst_t;
                              switch (clear_value_size) {
   case 1:
      memset(dst, *(uint8_t *)clear_value, size);
      case 4:
      util_memset32(dst, *(uint32_t *)clear_value, size / 4);
      default:
      for (unsigned i = 0; i < size; i += clear_value_size)
            }
      }
         void
   llvmpipe_init_surface_functions(struct llvmpipe_context *lp)
   {
      lp->pipe.clear_render_target = llvmpipe_clear_render_target;
   lp->pipe.clear_depth_stencil = llvmpipe_clear_depth_stencil;
   lp->pipe.create_surface = llvmpipe_create_surface;
   lp->pipe.surface_destroy = llvmpipe_surface_destroy;
   /* These are not actually functions dealing with surfaces */
   lp->pipe.clear_texture = llvmpipe_clear_texture;
   lp->pipe.clear_buffer = llvmpipe_clear_buffer;
   lp->pipe.resource_copy_region = lp_resource_copy;
   lp->pipe.blit = lp_blit;
   lp->pipe.flush_resource = lp_flush_resource;
      }
