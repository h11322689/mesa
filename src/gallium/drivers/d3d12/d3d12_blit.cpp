   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "d3d12_context.h"
   #include "d3d12_compiler.h"
   #include "d3d12_debug.h"
   #include "d3d12_format.h"
   #include "d3d12_query.h"
   #include "d3d12_resource.h"
   #include "d3d12_screen.h"
      #include "util/u_blitter.h"
   #include "util/format/u_format.h"
      #include "nir_to_dxil.h"
   #include "nir_builder.h"
      static void
   copy_buffer_region_no_barriers(struct d3d12_context *ctx,
                                 {
      uint64_t dst_off, src_off;
   ID3D12Resource *dst_buf = d3d12_resource_underlying(dst, &dst_off);
            ctx->cmdlist->CopyBufferRegion(dst_buf, dst_offset + dst_off,
            }
      static bool
   is_resolve(const struct pipe_blit_info *info)
   {
      return info->src.resource->nr_samples > 1 &&
      }
      static bool
   resolve_supported(const struct pipe_blit_info *info)
   {
               // check for unsupported operations
   if (util_format_is_depth_or_stencil(info->src.format) &&
      info->mask != PIPE_MASK_Z) {
      } else {
      if (util_format_get_mask(info->dst.format) != info->mask ||
      util_format_get_mask(info->src.format) != info->mask ||
   util_format_has_alpha1(info->src.format))
            if (info->filter != PIPE_TEX_FILTER_NEAREST ||
      info->scissor_enable ||
   info->num_window_rectangles > 0 ||
   info->alpha_blend)
         // formats need to match
   struct d3d12_resource *src = d3d12_resource(info->src.resource);
   struct d3d12_resource *dst = d3d12_resource(info->dst.resource);
   if (src->dxgi_format != dst->dxgi_format)
            if (util_format_is_pure_integer(src->base.b.format))
            // sizes needs to match
   if (info->src.box.width != info->dst.box.width ||
      info->src.box.height != info->dst.box.height)
         // can only resolve full subresource
   if (info->src.box.width != (int)u_minify(info->src.resource->width0,
            info->src.box.height != (int)u_minify(info->src.resource->height0,
         info->dst.box.width != (int)u_minify(info->dst.resource->width0,
         info->dst.box.height != (int)u_minify(info->dst.resource->height0,
                  }
      static void
   blit_resolve(struct d3d12_context *ctx, const struct pipe_blit_info *info)
   {
      struct d3d12_batch *batch = d3d12_current_batch(ctx);
   struct d3d12_resource *src = d3d12_resource(info->src.resource);
            d3d12_transition_resource_state(ctx, src,
               d3d12_transition_resource_state(ctx, dst,
                           d3d12_batch_reference_resource(batch, src, false);
                     assert(src->dxgi_format == dst->dxgi_format);
   ctx->cmdlist->ResolveSubresource(
      d3d12_resource_resource(dst), info->dst.level,
   d3d12_resource_resource(src), info->src.level,
   }
      static bool
   formats_are_copy_compatible(enum pipe_format src, enum pipe_format dst)
   {
      if (src == dst)
            /* We can skip the stencil copy */
   if (util_format_get_depth_only(src) == dst ||
      util_format_get_depth_only(dst) == src)
            }
      static bool
   box_fits(const struct pipe_box *box, const struct pipe_resource *res, int level)
   {
      unsigned lwidth = u_minify(res->width0, level);
   unsigned lheight= u_minify(res->height0, level);
   unsigned ldepth = res->target == PIPE_TEXTURE_3D ? u_minify(res->depth0, level) :
            unsigned wb = box->x;
            unsigned hb = box->y;
            unsigned db = box->z;
            return (wb <= lwidth && we <= lwidth &&
            }
      static bool
   direct_copy_supported(struct d3d12_screen *screen,
               {
      if (info->scissor_enable || info->alpha_blend ||
      (have_predication && info->render_condition_enable) ||
   MAX2(info->src.resource->nr_samples, 1) != MAX2(info->dst.resource->nr_samples, 1)) {
               if (!formats_are_copy_compatible(info->src.format, info->dst.format))
            if (util_format_is_depth_or_stencil(info->src.format) && !(info->mask & PIPE_MASK_ZS)) {
                  if (!util_format_is_depth_or_stencil(info->src.format)) {
      if (util_format_get_mask(info->dst.format) != info->mask ||
      util_format_get_mask(info->src.format) != info->mask)
            if (abs(info->src.box.height) != info->dst.box.height) {
                  if (info->src.box.height != info->dst.box.height &&
      (!util_format_is_depth_or_stencil(info->src.format) ||
   screen->opts2.ProgrammableSamplePositionsTier ==
   D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED)) {
               if (!box_fits(&info->dst.box, info->dst.resource, info->dst.level)) {
         }
   if (!box_fits(&info->src.box, info->src.resource, info->src.level)) {
                  if (info->src.box.width != info->dst.box.width) {
                  if (info->src.box.depth != info->dst.box.depth) {
                  if ((screen->opts2.ProgrammableSamplePositionsTier ==
      D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED &&
   (info->src.resource->bind & PIPE_BIND_DEPTH_STENCIL ||
                  if (info->dst.box.x != 0 ||
      info->dst.box.y != 0 ||
               if (info->src.box.x != 0 ||
      info->src.box.y != 0 ||
   info->src.box.z != 0 ||
   info->src.box.width != (int)u_minify(info->src.resource->width0,
         info->src.box.height != (int)u_minify(info->src.resource->height0,
         info->src.box.depth != (int)u_minify(info->src.resource->depth0,
                     }
      inline static unsigned
   get_subresource_id(enum pipe_texture_target target, unsigned subres, unsigned stride,
         {
      if (d3d12_subresource_id_uses_layer(target)) {
      subres += stride * z;
   if (updated_z)
      }
      }
      static void
   copy_subregion_no_barriers(struct d3d12_context *ctx,
                              struct d3d12_resource *dst,
   unsigned dst_level,
      {
      UNUSED struct d3d12_screen *screen = d3d12_screen(ctx->base.screen);
   D3D12_TEXTURE_COPY_LOCATION src_loc, dst_loc;
            int src_subres_stride = src->base.b.last_level + 1;
            int src_array_size = src->base.b.array_size;
            int stencil_src_res_offset = 1;
            int src_nres = 1;
            if (dst->base.b.format == PIPE_FORMAT_Z24_UNORM_S8_UINT ||
      dst->base.b.format == PIPE_FORMAT_S8_UINT_Z24_UNORM ||
   dst->base.b.format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT) {
   stencil_dst_res_offset = dst_subres_stride * dst_array_size;
               if (src->base.b.format == PIPE_FORMAT_Z24_UNORM_S8_UINT ||
      src->base.b.format == PIPE_FORMAT_S8_UINT_Z24_UNORM ||
   dst->base.b.format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT) {
   stencil_src_res_offset = src_subres_stride * src_array_size;
               static_assert(PIPE_MASK_S == 0x20 && PIPE_MASK_Z == 0x10, "unexpected ZS format mask");
   int nsubres = MIN2(src_nres, dst_nres);
                        if (!(subresource_copy_mask & (1 << subres)))
            src_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
   src_loc.SubresourceIndex = get_subresource_id(src->base.b.target, src_level, src_subres_stride, src_z, &src_z, src_array_size, src->plane_slice) +
                  dst_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
   dst_loc.SubresourceIndex = get_subresource_id(dst->base.b.target, dst_level, dst_subres_stride, dstz, &dstz, dst_array_size, dst->plane_slice) +
                  if (psrc_box->x == 0 && psrc_box->y == 0 && psrc_box->z == 0 &&
      psrc_box->width == (int)u_minify(src->base.b.width0, src_level) &&
                  assert((dstx == 0 && dsty == 0 && dstz == 0) ||
         screen->opts2.ProgrammableSamplePositionsTier !=
   D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED ||
                              } else {
      D3D12_BOX src_box;
   src_box.left = psrc_box->x;
   src_box.right = MIN2(psrc_box->x + psrc_box->width, (int)u_minify(src->base.b.width0, src_level));
   src_box.top = psrc_box->y;
   src_box.bottom = MIN2(psrc_box->y + psrc_box->height, (int)u_minify(src->base.b.height0, src_level));
                  assert((screen->opts2.ProgrammableSamplePositionsTier !=
         D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED ||
                  ctx->cmdlist->CopyTextureRegion(&dst_loc, dstx, dsty, dstz,
            }
      static void
   copy_resource_y_flipped_no_barriers(struct d3d12_context *ctx,
                                       struct d3d12_resource *dst,
   {
      if (D3D12_DEBUG_BLIT & d3d12_debug) {
      debug_printf("D3D12 BLIT as COPY: from %s@%d %dx%dx%d + %dx%dx%d\n",
               util_format_name(src->base.b.format), src_level,
   debug_printf("      to   %s@%d %dx%dx%d\n",
                     struct pipe_box src_box = *psrc_box;
   int src_inc = psrc_box->height > 0 ? 1 : -1;
   int dst_inc = pdst_box->height > 0 ? 1 : -1;
   src_box.height = 1;
            if (psrc_box->height < 0)
            for (int y = 0, dest_y = pdst_box->y; y < rows_to_copy;
      ++y, src_box.y += src_inc, dest_y += dst_inc) {
   copy_subregion_no_barriers(ctx, dst, dst_level,
               }
      void
   d3d12_direct_copy(struct d3d12_context *ctx,
                     struct d3d12_resource *dst,
   unsigned dst_level,
   const struct pipe_box *pdst_box,
   struct d3d12_resource *src,
   {
               unsigned src_subres = get_subresource_id(src->base.b.target, src_level, src->base.b.last_level + 1,
         unsigned dst_subres = get_subresource_id(dst->base.b.target, dst_level, dst->base.b.last_level + 1,
            if (D3D12_DEBUG_BLIT & d3d12_debug)
      debug_printf("BLIT: Direct copy from subres %d to subres  %d\n",
         d3d12_transition_subresources_state(ctx, src, src_subres, 1, 0, 1,
                              d3d12_transition_subresources_state(ctx, dst, dst_subres, 1, 0, 1,
                                       d3d12_batch_reference_resource(batch, src, false);
            if (src->base.b.target == PIPE_BUFFER) {
      copy_buffer_region_no_barriers(ctx, dst, pdst_box->x,
      } else if (psrc_box->height == pdst_box->height) {
      /* No flipping, we can forward this directly to resource_copy_region */
   copy_subregion_no_barriers(ctx, dst, dst_level,
            } else {
      assert(psrc_box->height == -pdst_box->height);
   copy_resource_y_flipped_no_barriers(ctx, dst, dst_level, pdst_box,
         }
      static bool
   is_same_resource(const struct pipe_blit_info *info)
   {
      return d3d12_resource_resource(d3d12_resource(info->src.resource)) ==
            }
      static struct pipe_resource *
   create_staging_resource(struct d3d12_context *ctx,
                           struct d3d12_resource *src,
      {
      struct pipe_resource templ = {};
   struct pipe_resource *staging_res;
            u_box_3d(MIN2(src_box->x, src_box->x + src_box->width),
            MIN2(src_box->y, src_box->y + src_box->height),
   MIN2(src_box->z, src_box->z + src_box->depth),
         templ.format = src->base.b.format;
   templ.width0 = copy_src.width;
   templ.height0 = copy_src.height;
   templ.depth0 = copy_src.depth;
   templ.array_size = 1;
   templ.nr_samples = src->base.b.nr_samples;
   templ.nr_storage_samples = src->base.b.nr_storage_samples;
   templ.usage = PIPE_USAGE_STAGING;
   templ.bind = util_format_is_depth_or_stencil(templ.format) ? PIPE_BIND_DEPTH_STENCIL : PIPE_BIND_RENDER_TARGET;
                     dst_box->x = 0;
   dst_box->y = 0;
   dst_box->z = 0;
   dst_box->width = copy_src.width;
   dst_box->height = copy_src.height;
            d3d12_direct_copy(ctx, d3d12_resource(staging_res), 0, dst_box,
            if (src_box->width < 0) {
      dst_box->x = dst_box->width;
               if (src_box->height < 0) {
      dst_box->y = dst_box->height;
               if (src_box->depth < 0) {
      dst_box->z = dst_box->depth;
      }
      }
      static void
   blit_same_resource(struct d3d12_context *ctx,
         {
               dst_info.src.level = 0;
   dst_info.src.resource = create_staging_resource(ctx, d3d12_resource(info->src.resource),
                     ctx->base.blit(&ctx->base, &dst_info);
      }
      static void
   util_blit_save_state(struct d3d12_context *ctx)
   {
      util_blitter_save_blend(ctx->blitter, ctx->gfx_pipeline_state.blend);
   util_blitter_save_depth_stencil_alpha(ctx->blitter, ctx->gfx_pipeline_state.zsa);
   util_blitter_save_vertex_elements(ctx->blitter, ctx->gfx_pipeline_state.ves);
   util_blitter_save_stencil_ref(ctx->blitter, &ctx->stencil_ref);
   util_blitter_save_rasterizer(ctx->blitter, ctx->gfx_pipeline_state.rast);
   util_blitter_save_fragment_shader(ctx->blitter, ctx->gfx_stages[PIPE_SHADER_FRAGMENT]);
   util_blitter_save_vertex_shader(ctx->blitter, ctx->gfx_stages[PIPE_SHADER_VERTEX]);
   util_blitter_save_geometry_shader(ctx->blitter, ctx->gfx_stages[PIPE_SHADER_GEOMETRY]);
   util_blitter_save_tessctrl_shader(ctx->blitter, ctx->gfx_stages[PIPE_SHADER_TESS_CTRL]);
            util_blitter_save_framebuffer(ctx->blitter, &ctx->fb);
   util_blitter_save_viewport(ctx->blitter, ctx->viewport_states);
   util_blitter_save_scissor(ctx->blitter, ctx->scissor_states);
   util_blitter_save_fragment_sampler_states(ctx->blitter,
               util_blitter_save_fragment_sampler_views(ctx->blitter,
               util_blitter_save_fragment_constant_buffer_slot(ctx->blitter, ctx->cbufs[PIPE_SHADER_FRAGMENT]);
   util_blitter_save_vertex_buffer_slot(ctx->blitter, ctx->vbs);
   util_blitter_save_sample_mask(ctx->blitter, ctx->gfx_pipeline_state.sample_mask, 0);
      }
      static void
   util_blit(struct d3d12_context *ctx,
         {
                  }
      static bool
   resolve_stencil_supported(struct d3d12_context *ctx,
         {
               if (!util_format_is_depth_or_stencil(info->src.format) ||
      !(info->mask & PIPE_MASK_S))
         if (info->mask & PIPE_MASK_Z) {
      struct pipe_blit_info new_info = *info;
   new_info.mask = PIPE_MASK_Z;
   if (!resolve_supported(&new_info) &&
      !util_blitter_is_blit_supported(ctx->blitter, &new_info))
            struct pipe_blit_info new_info = *info;
   new_info.dst.format = PIPE_FORMAT_R8_UINT;
      }
      static struct pipe_resource *
   create_tmp_resource(struct pipe_screen *screen,
         {
      struct pipe_resource tpl = {};
   tpl.width0 = info->dst.box.width;
   tpl.height0 = info->dst.box.height;
   tpl.depth0 = info->dst.box.depth;
   tpl.array_size = 1;
   tpl.format = PIPE_FORMAT_R8_UINT;
   tpl.target = info->dst.resource->target;
   tpl.nr_samples = info->dst.resource->nr_samples;
   tpl.nr_storage_samples = info->dst.resource->nr_storage_samples;
   tpl.usage = PIPE_USAGE_STREAM;
   tpl.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
      }
      static void *
   get_stencil_resolve_vs(struct d3d12_context *ctx)
   {
      if (ctx->stencil_resolve_vs)
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_VERTEX,
                  const struct glsl_type *vec4 = glsl_vec4_type();
   nir_variable *pos_in = nir_variable_create(b.shader, nir_var_shader_in,
            nir_variable *pos_out = nir_variable_create(b.shader, nir_var_shader_out,
                           struct pipe_shader_state state = {};
   state.type = PIPE_SHADER_IR_NIR;
   state.ir.nir = b.shader;
               }
      static void *
   get_stencil_resolve_fs(struct d3d12_context *ctx, bool no_flip)
   {
      if (!no_flip && ctx->stencil_resolve_fs)
            if (no_flip && ctx->stencil_resolve_fs_no_flip)
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT,
                  nir_variable *stencil_out = nir_variable_create(b.shader,
                              const struct glsl_type *sampler_type =
         nir_variable *sampler = nir_variable_create(b.shader, nir_var_uniform,
         sampler->data.binding = 0;
                     nir_variable *pos_in = nir_variable_create(b.shader, nir_var_shader_in,
         pos_in->data.location = VARYING_SLOT_POS; // VARYING_SLOT_VAR0?
                     if (no_flip)
         else {
      nir_tex_instr *txs = nir_tex_instr_create(b.shader, 1);
   txs->op = nir_texop_txs;
   txs->sampler_dim = GLSL_SAMPLER_DIM_MS;
   txs->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_deref, tex_deref);
   txs->is_array = false;
            nir_def_init(&txs->instr, &txs->def, 2, 32);
            pos_src = nir_vec4(&b,
                     nir_channel(&b, pos, 0),
   /*Height - pos_dest.y - 1*/
   nir_fsub(&b,
            nir_fsub(&b,
            nir_tex_instr *tex = nir_tex_instr_create(b.shader, 3);
   tex->sampler_dim = GLSL_SAMPLER_DIM_MS;
   tex->op = nir_texop_txf_ms;
   tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_coord,
         tex->src[1] = nir_tex_src_for_ssa(nir_tex_src_ms_index, nir_imm_int(&b, 0)); /* just use first sample */
   tex->src[2] = nir_tex_src_for_ssa(nir_tex_src_texture_deref, tex_deref);
   tex->dest_type = nir_type_uint32;
   tex->is_array = false;
            nir_def_init(&tex->instr, &tex->def, 4, 32);
                     struct pipe_shader_state state = {};
   state.type = PIPE_SHADER_IR_NIR;
   state.ir.nir = b.shader;
   void *result;
   if (no_flip) {
      result = ctx->base.create_fs_state(&ctx->base, &state);
      } else {
      result = ctx->base.create_fs_state(&ctx->base, &state);
                  }
      static void *
   get_sampler_state(struct d3d12_context *ctx)
   {
      if (ctx->sampler_state)
            struct pipe_sampler_state state;
   memset(&state, 0, sizeof(state));
   state.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   state.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
               }
      static struct pipe_resource *
   resolve_stencil_to_temp(struct d3d12_context *ctx,
         {
      struct pipe_context *pctx = &ctx->base;
   struct pipe_resource *tmp = create_tmp_resource(pctx->screen, info);
   if (!tmp) {
      debug_printf("D3D12: failed to create stencil-resolve temp-resource\n");
      }
            /* resolve stencil into tmp */
   struct pipe_surface dst_tmpl;
   util_blitter_default_dst_texture(&dst_tmpl, tmp, 0, 0);
   dst_tmpl.format = tmp->format;
   struct pipe_surface *dst_surf = pctx->create_surface(pctx, tmp, &dst_tmpl);
   if (!dst_surf) {
      debug_printf("D3D12: failed to create stencil-resolve dst-surface\n");
               struct pipe_sampler_view src_templ, *src_view;
   util_blitter_default_src_texture(ctx->blitter, &src_templ,
         src_templ.format = util_format_stencil_only(info->src.format);
                     util_blit_save_state(ctx);
   pctx->set_sampler_views(pctx, PIPE_SHADER_FRAGMENT, 0, 1, 0, false, &src_view);
   pctx->bind_sampler_states(pctx, PIPE_SHADER_FRAGMENT, 0, 1, &sampler_state);
   util_blitter_custom_shader(ctx->blitter, dst_surf,
               util_blitter_restore_textures(ctx->blitter);
   pipe_surface_reference(&dst_surf, NULL);
   pipe_sampler_view_reference(&src_view, NULL);
      }
      static void
   blit_resolve_stencil(struct d3d12_context *ctx,
         {
               if (D3D12_DEBUG_BLIT & d3d12_debug)
            if (info->mask & PIPE_MASK_Z) {
      /* resolve depth into dst */
   struct pipe_blit_info new_info = *info;
            if (resolve_supported(&new_info))
         else
                           /* copy resolved stencil into dst */
   struct d3d12_resource *dst = d3d12_resource(info->dst.resource);
   d3d12_transition_subresources_state(ctx, d3d12_resource(tmp),
                     d3d12_transition_subresources_state(ctx, dst,
                              struct d3d12_batch *batch = d3d12_current_batch(ctx);
   d3d12_batch_reference_resource(batch, d3d12_resource(tmp), false);
            D3D12_BOX src_box;
   src_box.left = src_box.top = src_box.front = 0;
   src_box.right = tmp->width0;
   src_box.bottom = tmp->height0;
            D3D12_TEXTURE_COPY_LOCATION src_loc;
   src_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
   src_loc.SubresourceIndex = 0;
            D3D12_TEXTURE_COPY_LOCATION dst_loc;
   dst_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
   dst_loc.SubresourceIndex = 1;
            ctx->cmdlist->CopyTextureRegion(&dst_loc, info->dst.box.x,
                     }
      static bool
   replicate_stencil_supported(struct d3d12_context *ctx,
         {
      if (!util_format_is_depth_or_stencil(info->src.format) ||
      !(info->mask & PIPE_MASK_S))
         if (info->mask & PIPE_MASK_Z) {
      struct pipe_blit_info new_info = *info;
   new_info.mask = PIPE_MASK_Z;
   if (!util_blitter_is_blit_supported(ctx->blitter, &new_info))
                  }
      static void
   blit_replicate_stencil(struct d3d12_context *ctx,
         {
               if (D3D12_DEBUG_BLIT & d3d12_debug)
            if (info->mask & PIPE_MASK_Z) {
      /* resolve depth into dst */
   struct pipe_blit_info new_info = *info;
   new_info.mask = PIPE_MASK_Z;
               util_blit_save_state(ctx);
   util_blitter_stencil_fallback(ctx->blitter, info->dst.resource,
                                    }
      void
   d3d12_blit(struct pipe_context *pctx,
         {
               if (!info->render_condition_enable && ctx->current_predication) {
      if (D3D12_DEBUG_BLIT & d3d12_debug)
                     if (D3D12_DEBUG_BLIT & d3d12_debug) {
      debug_printf("D3D12 BLIT: from %s@%d msaa:%d %dx%dx%d + %dx%dx%d\n",
               util_format_name(info->src.format), info->src.level,
   info->src.resource->nr_samples,
   debug_printf("            to   %s@%d msaa:%d %dx%dx%d + %dx%dx%d ",
               util_format_name(info->dst.format), info->dst.level,
   info->dst.resource->nr_samples,
   debug_printf("| flags %s%s%s\n",
                           if (is_same_resource(info))
         else if (is_resolve(info)) {
      if (resolve_supported(info))
         else if (util_blitter_is_blit_supported(ctx->blitter, info))
         else if (resolve_stencil_supported(ctx, info))
         else
      debug_printf("D3D12: resolve unsupported %s -> %s\n",
         } else if (direct_copy_supported(d3d12_screen(pctx->screen), info,
            d3d12_direct_copy(ctx, d3d12_resource(info->dst.resource),
                  else if (util_blitter_is_blit_supported(ctx->blitter, info))
         else if (replicate_stencil_supported(ctx, info))
         else
      debug_printf("D3D12: blit unsupported %s -> %s\n",
               if (!info->render_condition_enable && ctx->current_predication) {
      d3d12_enable_predication(ctx);
   if (D3D12_DEBUG_BLIT & d3d12_debug)
            }
      static void
   d3d12_resource_copy_region(struct pipe_context *pctx,
                              struct pipe_resource *pdst,
      {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_resource *dst = d3d12_resource(pdst);
   struct d3d12_resource *src = d3d12_resource(psrc);
   struct pipe_resource *staging_res = NULL;
   const struct pipe_box *src_box = psrc_box;
            if (D3D12_DEBUG_BLIT & d3d12_debug) {
      debug_printf("D3D12 COPY: from %s@%d msaa:%d mips:%d %dx%dx%d + %dx%dx%d\n",
               util_format_name(psrc->format), src_level, psrc->nr_samples,
   psrc->last_level,
   debug_printf("            to   %s@%d msaa:%d mips:%d %dx%dx%d\n",
                     /* Use an intermediate resource if copying from/to the same subresource */
   if (d3d12_resource_resource(dst) == d3d12_resource_resource(src) && dst_level == src_level) {
      staging_res = create_staging_resource(ctx, src, src_level, psrc_box, &staging_box, PIPE_MASK_RGBAZS);
   src = d3d12_resource(staging_res);
   src_level = 0;
               dst_box.x = dstx;
   dst_box.y = dsty;
   dst_box.z = dstz;
   dst_box.width = psrc_box->width;
            d3d12_direct_copy(ctx, dst, dst_level, &dst_box,
            if (staging_res)
      }
      void
   d3d12_context_blit_init(struct pipe_context *ctx)
   {
      ctx->resource_copy_region = d3d12_resource_copy_region;
      }
