   /*
   * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
   * Copyright 2015 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_pipe.h"
   #include "util/format/u_format.h"
   #include "util/u_log.h"
   #include "util/u_surface.h"
   #include "util/hash_table.h"
      enum
   {
      SI_COPY =
                                 };
      void si_blitter_begin(struct si_context *sctx, enum si_blitter_op op)
   {
      util_blitter_save_vertex_shader(sctx->blitter, sctx->shader.vs.cso);
   util_blitter_save_tessctrl_shader(sctx->blitter, sctx->shader.tcs.cso);
   util_blitter_save_tesseval_shader(sctx->blitter, sctx->shader.tes.cso);
   util_blitter_save_geometry_shader(sctx->blitter, sctx->shader.gs.cso);
   util_blitter_save_so_targets(sctx->blitter, sctx->streamout.num_targets,
                  if (op & SI_SAVE_FRAGMENT_STATE) {
      struct pipe_constant_buffer fs_cb = {};
            if (op & SI_SAVE_FRAGMENT_CONSTANT)
            pipe_resource_reference(&fs_cb.buffer, NULL);
   util_blitter_save_blend(sctx->blitter, sctx->queued.named.blend);
   util_blitter_save_depth_stencil_alpha(sctx->blitter, sctx->queued.named.dsa);
   util_blitter_save_stencil_ref(sctx->blitter, &sctx->stencil_ref.state);
   util_blitter_save_fragment_shader(sctx->blitter, sctx->shader.ps.cso);
   util_blitter_save_sample_mask(sctx->blitter, sctx->sample_mask, sctx->ps_iter_samples);
   util_blitter_save_scissor(sctx->blitter, &sctx->scissors[0]);
   util_blitter_save_window_rectangles(sctx->blitter, sctx->window_rectangles_include,
               if (op & SI_SAVE_FRAMEBUFFER)
            if (op & SI_SAVE_TEXTURES) {
      util_blitter_save_fragment_sampler_states(
            util_blitter_save_fragment_sampler_views(sctx->blitter, 2,
               if (op & SI_DISABLE_RENDER_COND)
            if (sctx->screen->dpbb_allowed) {
      sctx->dpbb_force_off = true;
               /* Force-disable fbfetch because there are unsolvable recursion problems with u_blitter. */
               }
      void si_blitter_end(struct si_context *sctx)
   {
               if (sctx->screen->dpbb_allowed) {
      sctx->dpbb_force_off = false;
                        /* Restore shader pointers because the VS blit shader changed all
   * non-global VS user SGPRs. */
            if (sctx->gfx_level >= GFX11)
            /* Reset SI_SGPR_SMALL_PRIM_CULL_INFO: */
   if (sctx->screen->use_ngg_culling)
            sctx->vertex_buffers_dirty = sctx->num_vertex_elements > 0;
            /* We force-disabled fbfetch for u_blitter, so recompute the state. */
      }
      static unsigned u_max_sample(struct pipe_resource *r)
   {
         }
      static unsigned si_blit_dbcb_copy(struct si_context *sctx, struct si_texture *src,
                     {
      struct pipe_surface surf_tmpl = {{0}};
   unsigned layer, sample, checked_last_layer, max_layer;
                     if (planes & PIPE_MASK_Z)
         if (planes & PIPE_MASK_S)
                                    while (level_mask) {
               /* The smaller the mipmap level, the less layers there are
   * as far as 3D textures are concerned. */
   max_layer = util_max_layer(&src->buffer.b.b, level);
                     for (layer = first_layer; layer <= checked_last_layer; layer++) {
               surf_tmpl.format = src->buffer.b.b.format;
                                          for (sample = first_sample; sample <= last_sample; sample++) {
      if (sample != sctx->dbcb_copy_sample) {
                        si_blitter_begin(sctx, SI_DECOMPRESS);
   util_blitter_custom_depth_stencil(sctx->blitter, zsurf, cbsurf, 1 << sample,
                     pipe_surface_reference(&zsurf, NULL);
               if (first_layer == 0 && last_layer >= max_layer && first_sample == 0 &&
      last_sample >= u_max_sample(&src->buffer.b.b))
            sctx->decompression_enabled = false;
   sctx->dbcb_depth_copy_enabled = false;
   sctx->dbcb_stencil_copy_enabled = false;
               }
      /* Helper function for si_blit_decompress_zs_in_place.
   */
   static void si_blit_decompress_zs_planes_in_place(struct si_context *sctx,
                     {
      struct pipe_surface *zsurf, surf_tmpl = {{0}};
   unsigned layer, max_layer, checked_last_layer;
            if (!level_mask)
            if (planes & PIPE_MASK_S)
         if (planes & PIPE_MASK_Z)
                                    while (level_mask) {
                        /* The smaller the mipmap level, the less layers there are
   * as far as 3D textures are concerned. */
   max_layer = util_max_layer(&texture->buffer.b.b, level);
            for (layer = first_layer; layer <= checked_last_layer; layer++) {
                              si_blitter_begin(sctx, SI_DECOMPRESS);
   util_blitter_custom_depth_stencil(sctx->blitter, zsurf, NULL, ~0, sctx->custom_dsa_flush,
                              /* The texture will always be dirty if some layers aren't flushed.
   * I don't think this case occurs often though. */
   if (first_layer == 0 && last_layer >= max_layer) {
                     if (planes & PIPE_MASK_Z)
         if (planes & PIPE_MASK_S)
            sctx->decompression_enabled = false;
   sctx->db_flush_depth_inplace = false;
   sctx->db_flush_stencil_inplace = false;
      }
      /* Helper function of si_flush_depth_texture: decompress the given levels
   * of Z and/or S planes in place.
   */
   static void si_blit_decompress_zs_in_place(struct si_context *sctx, struct si_texture *texture,
               {
               /* First, do combined Z & S decompresses for levels that need it. */
   if (both) {
      si_blit_decompress_zs_planes_in_place(sctx, texture, PIPE_MASK_Z | PIPE_MASK_S, both,
         levels_z &= ~both;
               /* Now do separate Z and S decompresses. */
   if (levels_z) {
      si_blit_decompress_zs_planes_in_place(sctx, texture, PIPE_MASK_Z, levels_z, first_layer,
               if (levels_s) {
      si_blit_decompress_zs_planes_in_place(sctx, texture, PIPE_MASK_S, levels_s, first_layer,
         }
      static void si_decompress_depth(struct si_context *sctx, struct si_texture *tex,
               {
      unsigned inplace_planes = 0;
   unsigned copy_planes = 0;
   unsigned level_mask = u_bit_consecutive(first_level, last_level - first_level + 1);
   unsigned levels_z = 0;
            if (required_planes & PIPE_MASK_Z) {
               if (levels_z) {
      if (si_can_sample_zs(tex, false))
         else
         }
   if (required_planes & PIPE_MASK_S) {
               if (levels_s) {
      if (si_can_sample_zs(tex, true))
         else
                  if (unlikely(sctx->log))
      u_log_printf(sctx->log,
                     /* We may have to allocate the flushed texture here when called from
   * si_decompress_subresource.
   */
   if (copy_planes &&
      (tex->flushed_depth_texture || si_init_flushed_depth_texture(&sctx->b, &tex->buffer.b.b))) {
   struct si_texture *dst = tex->flushed_depth_texture;
   unsigned fully_copied_levels;
                     if (util_format_is_depth_and_stencil(dst->buffer.b.b.format))
            if (copy_planes & PIPE_MASK_Z) {
      levels |= levels_z;
      }
   if (copy_planes & PIPE_MASK_S) {
      levels |= levels_s;
               fully_copied_levels = si_blit_dbcb_copy(sctx, tex, dst, copy_planes, levels, first_layer,
            if (copy_planes & PIPE_MASK_Z)
         if (copy_planes & PIPE_MASK_S)
               if (inplace_planes) {
      bool has_htile = si_htile_enabled(tex, first_level, inplace_planes);
            /* Don't decompress if there is no HTILE or when HTILE is
   * TC-compatible. */
   if (has_htile && !tc_compat_htile) {
         } else {
      /* This is only a cache flush.
   *
   * Only clear the mask that we are flushing, because
   * si_make_DB_shader_coherent() treats different levels
   * and depth and stencil differently.
   */
   if (inplace_planes & PIPE_MASK_Z)
         if (inplace_planes & PIPE_MASK_S)
               /* We just had to completely decompress Z/S for texturing. Enable
   * TC-compatible HTILE on the next clear, so that the decompression
   * doesn't have to be done for this texture ever again.
   *
   * TC-compatible HTILE might slightly reduce Z/S performance, but
   * the decompression is much worse.
   */
   if (has_htile && !tc_compat_htile &&
      /* We can only transition the whole buffer in one clear, so no mipmapping: */
   tex->buffer.b.b.last_level == 0 &&
   tex->surface.flags & RADEON_SURF_TC_COMPATIBLE_HTILE &&
               /* Only in-place decompression needs to flush DB caches, or
   * when we don't decompress but TC-compatible planes are dirty.
   */
   si_make_DB_shader_coherent(sctx, tex->buffer.b.b.nr_samples, inplace_planes & PIPE_MASK_S,
      }
   /* set_framebuffer_state takes care of coherency for single-sample.
   * The DB->CB copy uses CB for the final writes.
   */
   if (copy_planes && tex->buffer.b.b.nr_samples > 1)
      }
      static bool si_decompress_sampler_depth_textures(struct si_context *sctx,
         {
      unsigned i;
   unsigned mask = textures->needs_depth_decompress_mask;
            while (mask) {
      struct pipe_sampler_view *view;
   struct si_sampler_view *sview;
                     view = textures->views[i];
   assert(view);
            tex = (struct si_texture *)view->texture;
            si_decompress_depth(sctx, tex, sview->is_stencil_sampler ? PIPE_MASK_S : PIPE_MASK_Z,
                  if (tex->need_flush_after_depth_decompression) {
      need_flush = true;
                     }
      static void si_blit_decompress_color(struct si_context *sctx, struct si_texture *tex,
                     {
      void *custom_blend;
   unsigned layer, checked_last_layer, max_layer;
            if (!need_dcc_decompress)
         if (!level_mask)
            /* No color decompression is needed on GFX11. */
            if (unlikely(sctx->log))
      u_log_printf(sctx->log,
                     if (need_dcc_decompress) {
               /* DCC_DECOMPRESS and ELIMINATE_FAST_CLEAR require MSAA_NUM_SAMPLES=0. */
   if (sctx->gfx_level >= GFX11) {
      sctx->gfx11_force_msaa_num_samples_zero = true;
                        /* disable levels without DCC */
   for (int i = first_level; i <= last_level; i++) {
      if (!vi_dcc_enabled(tex, i))
         } else if (tex->surface.fmask_size) {
      assert(sctx->gfx_level < GFX11);
      } else {
      assert(sctx->gfx_level < GFX11);
                        while (level_mask) {
               /* The smaller the mipmap level, the less layers there are
   * as far as 3D textures are concerned. */
   max_layer = util_max_layer(&tex->buffer.b.b, level);
            for (layer = first_layer; layer <= checked_last_layer; layer++) {
               surf_tmpl.format = tex->buffer.b.b.format;
   surf_tmpl.u.tex.level = level;
   surf_tmpl.u.tex.first_layer = layer;
                  /* Required before and after FMASK and DCC_DECOMPRESS. */
   if (custom_blend == sctx->custom_blend_fmask_decompress ||
      custom_blend == sctx->custom_blend_dcc_decompress) {
   sctx->flags |= SI_CONTEXT_FLUSH_AND_INV_CB;
               si_blitter_begin(sctx, SI_DECOMPRESS);
                  if (custom_blend == sctx->custom_blend_fmask_decompress ||
      custom_blend == sctx->custom_blend_dcc_decompress) {
   sctx->flags |= SI_CONTEXT_FLUSH_AND_INV_CB;
               /* When running FMASK decompression with DCC, we need to run the "eliminate fast clear" pass
   * separately because FMASK decompression doesn't eliminate DCC fast clear. This makes
   * render->texture transitions more expensive. It can be disabled by
   * allow_dcc_msaa_clear_to_reg_for_bpp.
   *
   * TODO: When we get here, change the compression to TC-compatible on the next clear
   *       to disable both the FMASK decompression and fast clear elimination passes.
   */
   if (sctx->screen->allow_dcc_msaa_clear_to_reg_for_bpp[util_logbase2(tex->surface.bpe)] &&
      custom_blend == sctx->custom_blend_fmask_decompress &&
   vi_dcc_enabled(tex, level)) {
   si_blitter_begin(sctx, SI_DECOMPRESS);
   util_blitter_custom_color(sctx->blitter, cbsurf, sctx->custom_blend_eliminate_fastclear);
                           /* The texture will always be dirty if some layers aren't flushed.
   * I don't think this case occurs often though. */
   if (first_layer == 0 && last_layer >= max_layer) {
                     sctx->decompression_enabled = false;
   si_make_CB_shader_coherent(sctx, tex->buffer.b.b.nr_samples, vi_dcc_enabled(tex, first_level),
            /* Restore gfx11_force_msaa_num_samples_zero. */
   if (sctx->gfx11_force_msaa_num_samples_zero) {
      sctx->gfx11_force_msaa_num_samples_zero = false;
            expand_fmask:
      if (need_fmask_expand && tex->surface.fmask_offset && !tex->fmask_is_identity) {
      assert(sctx->gfx_level < GFX11); /* no FMASK on gfx11 */
   si_compute_expand_fmask(&sctx->b, &tex->buffer.b.b);
         }
      static void si_decompress_color_texture(struct si_context *sctx, struct si_texture *tex,
               {
               /* CMASK or DCC can be discarded and we can still end up here. */
   if (!tex->cmask_buffer && !tex->surface.fmask_size &&
      !vi_dcc_enabled(tex, first_level))
         si_blit_decompress_color(sctx, tex, first_level, last_level, 0,
            }
      static void si_decompress_sampler_color_textures(struct si_context *sctx,
         {
      unsigned i;
                     while (mask) {
      struct pipe_sampler_view *view;
                     view = textures->views[i];
                     si_decompress_color_texture(sctx, tex, view->u.tex.first_level, view->u.tex.last_level,
         }
      static void si_decompress_image_color_textures(struct si_context *sctx, struct si_images *images)
   {
      unsigned i;
                     while (mask) {
      const struct pipe_image_view *view;
                     view = &images->views[i];
                     si_decompress_color_texture(sctx, tex, view->u.tex.level, view->u.tex.level,
         }
      static void si_check_render_feedback_texture(struct si_context *sctx, struct si_texture *tex,
               {
               if (!vi_dcc_enabled(tex, first_level))
            for (unsigned j = 0; j < sctx->framebuffer.state.nr_cbufs; ++j) {
               if (!sctx->framebuffer.state.cbufs[j])
                     if (tex == (struct si_texture *)surf->base.texture && surf->base.u.tex.level >= first_level &&
      surf->base.u.tex.level <= last_level && surf->base.u.tex.first_layer <= last_layer &&
   surf->base.u.tex.last_layer >= first_layer) {
   render_feedback = true;
                  if (render_feedback)
      }
      static void si_check_render_feedback_textures(struct si_context *sctx, struct si_samplers *textures,
         {
               while (mask) {
      const struct pipe_sampler_view *view;
                     view = textures->views[i];
   if (view->texture->target == PIPE_BUFFER)
                     si_check_render_feedback_texture(sctx, tex, view->u.tex.first_level, view->u.tex.last_level,
         }
      static void si_check_render_feedback_images(struct si_context *sctx, struct si_images *images,
         {
               while (mask) {
      const struct pipe_image_view *view;
                     view = &images->views[i];
   if (view->resource->target == PIPE_BUFFER)
                     si_check_render_feedback_texture(sctx, tex, view->u.tex.level, view->u.tex.level,
         }
      static void si_check_render_feedback_resident_textures(struct si_context *sctx)
   {
      util_dynarray_foreach (&sctx->resident_tex_handles, struct si_texture_handle *, tex_handle) {
      struct pipe_sampler_view *view;
            view = (*tex_handle)->view;
   if (view->texture->target == PIPE_BUFFER)
                     si_check_render_feedback_texture(sctx, tex, view->u.tex.first_level, view->u.tex.last_level,
         }
      static void si_check_render_feedback_resident_images(struct si_context *sctx)
   {
      util_dynarray_foreach (&sctx->resident_img_handles, struct si_image_handle *, img_handle) {
      struct pipe_image_view *view;
            view = &(*img_handle)->view;
   if (view->resource->target == PIPE_BUFFER)
                     si_check_render_feedback_texture(sctx, tex, view->u.tex.level, view->u.tex.level,
         }
      static void si_check_render_feedback(struct si_context *sctx)
   {
      if (!sctx->need_check_render_feedback)
            /* There is no render feedback if color writes are disabled.
   * (e.g. a pixel shader with image stores)
   */
   if (!si_get_total_colormask(sctx))
            for (int i = 0; i < SI_NUM_GRAPHICS_SHADERS; ++i) {
      if (!sctx->shaders[i].cso)
            struct si_shader_info *info = &sctx->shaders[i].cso->info;
   si_check_render_feedback_images(sctx, &sctx->images[i],
         si_check_render_feedback_textures(sctx, &sctx->samplers[i],
               si_check_render_feedback_resident_images(sctx);
               }
      static void si_decompress_resident_color_textures(struct si_context *sctx)
   {
               util_dynarray_foreach (&sctx->resident_tex_needs_color_decompress, struct si_texture_handle *,
            struct pipe_sampler_view *view = (*tex_handle)->view;
            si_decompress_color_texture(sctx, tex, view->u.tex.first_level, view->u.tex.last_level,
         }
      static void si_decompress_resident_depth_textures(struct si_context *sctx)
   {
      util_dynarray_foreach (&sctx->resident_tex_needs_depth_decompress, struct si_texture_handle *,
            struct pipe_sampler_view *view = (*tex_handle)->view;
   struct si_sampler_view *sview = (struct si_sampler_view *)view;
            si_decompress_depth(sctx, tex, sview->is_stencil_sampler ? PIPE_MASK_S : PIPE_MASK_Z,
               }
      static void si_decompress_resident_images(struct si_context *sctx)
   {
               util_dynarray_foreach (&sctx->resident_img_needs_color_decompress, struct si_image_handle *,
            struct pipe_image_view *view = &(*img_handle)->view;
            si_decompress_color_texture(sctx, tex, view->u.tex.level, view->u.tex.level,
         }
      void gfx6_decompress_textures(struct si_context *sctx, unsigned shader_mask)
   {
      unsigned compressed_colortex_counter, mask;
            if (sctx->blitter_running)
            /* Update the compressed_colortex_mask if necessary. */
   compressed_colortex_counter = p_atomic_read(&sctx->screen->compressed_colortex_counter);
   if (compressed_colortex_counter != sctx->last_compressed_colortex_counter) {
      sctx->last_compressed_colortex_counter = compressed_colortex_counter;
               /* Decompress color & depth textures if needed. */
   mask = sctx->shader_needs_decompress_mask & shader_mask;
   while (mask) {
               if (sctx->samplers[i].needs_depth_decompress_mask) {
         }
   if (sctx->samplers[i].needs_color_decompress_mask) {
         }
   if (sctx->images[i].needs_color_decompress_mask) {
                     if (sctx->gfx_level == GFX10_3 && need_flush) {
      /* This fixes a corruption with the following sequence:
   *   - fast clear depth
   *   - decompress depth
   *   - draw
   * (see https://gitlab.freedesktop.org/drm/amd/-/issues/1810#note_1170171)
   */
               if (shader_mask & u_bit_consecutive(0, SI_NUM_GRAPHICS_SHADERS)) {
      if (sctx->uses_bindless_samplers) {
      si_decompress_resident_color_textures(sctx);
      }
   if (sctx->uses_bindless_images)
            if (sctx->ps_uses_fbfetch) {
      struct pipe_surface *cb0 = sctx->framebuffer.state.cbufs[0];
   si_decompress_color_texture(sctx, (struct si_texture *)cb0->texture,
                  } else if (shader_mask & (1 << PIPE_SHADER_COMPUTE)) {
      if (sctx->cs_shader_state.program->sel.info.uses_bindless_samplers) {
      si_decompress_resident_color_textures(sctx);
      }
   if (sctx->cs_shader_state.program->sel.info.uses_bindless_images)
         }
      void gfx11_decompress_textures(struct si_context *sctx, unsigned shader_mask)
   {
      if (sctx->blitter_running)
            /* Decompress depth textures if needed. */
   unsigned mask = sctx->shader_needs_decompress_mask & shader_mask;
   u_foreach_bit(i, mask) {
      assert(sctx->samplers[i].needs_depth_decompress_mask);
               /* Decompress bindless depth textures and disable DCC for render feedback. */
   if (shader_mask & u_bit_consecutive(0, SI_NUM_GRAPHICS_SHADERS)) {
      if (sctx->uses_bindless_samplers)
               } else if (shader_mask & (1 << PIPE_SHADER_COMPUTE)) {
      if (sctx->cs_shader_state.program->sel.info.uses_bindless_samplers)
         }
      /* Helper for decompressing a portion of a color or depth resource before
   * blitting if any decompression is needed.
   * The driver doesn't decompress resources automatically while u_blitter is
   * rendering. */
   void si_decompress_subresource(struct pipe_context *ctx, struct pipe_resource *tex, unsigned planes,
               {
      struct si_context *sctx = (struct si_context *)ctx;
            if (stex->db_compatible) {
               if (!stex->surface.has_stencil)
            /* If we've rendered into the framebuffer and it's a blitting
   * source, make sure the decompression pass is invoked
   * by dirtying the framebuffer.
   */
   if (sctx->framebuffer.state.zsbuf && sctx->framebuffer.state.zsbuf->u.tex.level == level &&
                     } else if (stex->surface.fmask_size || stex->cmask_buffer ||
            /* If we've rendered into the framebuffer and it's a blitting
   * source, make sure the decompression pass is invoked
   * by dirtying the framebuffer.
   */
   for (unsigned i = 0; i < sctx->framebuffer.state.nr_cbufs; i++) {
      if (sctx->framebuffer.state.cbufs[i] &&
      sctx->framebuffer.state.cbufs[i]->u.tex.level == level &&
   sctx->framebuffer.state.cbufs[i]->texture == tex) {
   si_update_fb_dirtiness_after_rendering(sctx);
                  si_blit_decompress_color(sctx, stex, level, level, first_layer, last_layer, false,
         }
      struct texture_orig_info {
      unsigned format;
   unsigned width0;
   unsigned height0;
   unsigned npix_x;
   unsigned npix_y;
   unsigned npix0_x;
      };
      void si_resource_copy_region(struct pipe_context *ctx, struct pipe_resource *dst,
                     {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_texture *ssrc = (struct si_texture *)src;
   struct pipe_surface *dst_view, dst_templ;
   struct pipe_sampler_view src_templ, *src_view;
            /* Handle buffers first. */
   if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      si_copy_buffer(sctx, dst, src, dstx, src_box->x, src_box->width, SI_OP_SYNC_BEFORE_AFTER);
               if (si_compute_copy_image(sctx, dst, dst_level, src, src_level, dstx, dsty, dstz,
                           /* The driver doesn't decompress resources automatically while
   * u_blitter is rendering. */
   si_decompress_subresource(ctx, src, PIPE_MASK_RGBAZS, src_level, src_box->z,
            util_blitter_default_dst_texture(&dst_templ, dst, dst_level, dstz);
            assert(!util_format_is_compressed(src->format) && !util_format_is_compressed(dst->format));
            if (!util_blitter_is_copy_supported(sctx->blitter, dst, src)) {
      switch (ssrc->surface.bpe) {
   case 1:
      dst_templ.format = PIPE_FORMAT_R8_UNORM;
   src_templ.format = PIPE_FORMAT_R8_UNORM;
      case 2:
      dst_templ.format = PIPE_FORMAT_R8G8_UNORM;
   src_templ.format = PIPE_FORMAT_R8G8_UNORM;
      case 4:
      dst_templ.format = PIPE_FORMAT_R8G8B8A8_UNORM;
   src_templ.format = PIPE_FORMAT_R8G8B8A8_UNORM;
      case 8:
      dst_templ.format = PIPE_FORMAT_R16G16B16A16_UINT;
   src_templ.format = PIPE_FORMAT_R16G16B16A16_UINT;
      case 16:
      dst_templ.format = PIPE_FORMAT_R32G32B32A32_UINT;
   src_templ.format = PIPE_FORMAT_R32G32B32A32_UINT;
      default:
      fprintf(stderr, "Unhandled format %s with blocksize %u\n",
                        /* SNORM blitting has precision issues on some chips. Use the SINT
   * equivalent instead, which doesn't force DCC decompression.
   */
   if (util_format_is_snorm(dst_templ.format)) {
                  vi_disable_dcc_if_incompatible_format(sctx, dst, dst_level, dst_templ.format);
            /* Initialize the surface. */
            /* Initialize the sampler view. */
            u_box_3d(dstx, dsty, dstz, abs(src_box->width), abs(src_box->height), abs(src_box->depth),
            /* Copy. */
   si_blitter_begin(sctx, SI_COPY);
   util_blitter_blit_generic(sctx->blitter, dst_view, &dstbox, src_view, src_box, src->width0,
                        pipe_surface_reference(&dst_view, NULL);
      }
      static void si_do_CB_resolve(struct si_context *sctx, const struct pipe_blit_info *info,
               {
      /* Required before and after CB_RESOLVE. */
   sctx->flags |= SI_CONTEXT_FLUSH_AND_INV_CB;
            si_blitter_begin(
         util_blitter_custom_resolve_color(sctx->blitter, dst, dst_level, dst_z, info->src.resource,
                  /* Flush caches for possible texturing. */
      }
      static bool resolve_formats_compatible(enum pipe_format src, enum pipe_format dst,
         {
               if (src_swaps_rgb_to_bgr) {
      /* We must only check the swapped format. */
   enum pipe_format swapped_src = util_format_rgb_to_bgr(src);
   assert(swapped_src);
   return util_is_format_compatible(util_format_description(swapped_src),
               if (util_is_format_compatible(util_format_description(src), util_format_description(dst)))
            enum pipe_format swapped_src = util_format_rgb_to_bgr(src);
   *need_rgb_to_bgr = util_is_format_compatible(util_format_description(swapped_src),
            }
      bool si_msaa_resolve_blit_via_CB(struct pipe_context *ctx, const struct pipe_blit_info *info)
   {
               /* Gfx11 doesn't have CB_RESOLVE. */
   if (sctx->gfx_level >= GFX11)
            struct si_texture *src = (struct si_texture *)info->src.resource;
   struct si_texture *dst = (struct si_texture *)info->dst.resource;
   ASSERTED struct si_texture *stmp;
   unsigned dst_width = u_minify(info->dst.resource->width0, info->dst.level);
   unsigned dst_height = u_minify(info->dst.resource->height0, info->dst.level);
   enum pipe_format format = info->src.format;
   struct pipe_resource *tmp, templ;
            /* Check basic requirements for hw resolve. */
   if (!(info->src.resource->nr_samples > 1 && info->dst.resource->nr_samples <= 1 &&
         !util_format_is_pure_integer(format) && !util_format_is_depth_or_stencil(format) &&
            /* Hardware MSAA resolve doesn't work if SPI format = NORM16_ABGR and
   * the format is R16G16. Use R16A16, which does work.
   */
   if (format == PIPE_FORMAT_R16G16_UNORM)
         if (format == PIPE_FORMAT_R16G16_SNORM)
                     /* Check the remaining requirements for hw resolve. */
   if (util_max_layer(info->dst.resource, info->dst.level) == 0 && !info->scissor_enable &&
      (info->mask & PIPE_MASK_RGBA) == PIPE_MASK_RGBA &&
   resolve_formats_compatible(info->src.format, info->dst.format,
         dst_width == info->src.resource->width0 && dst_height == info->src.resource->height0 &&
   info->dst.box.x == 0 && info->dst.box.y == 0 && info->dst.box.width == dst_width &&
   info->dst.box.height == dst_height && info->dst.box.depth == 1 && info->src.box.x == 0 &&
   info->src.box.y == 0 && info->src.box.width == dst_width &&
   info->src.box.height == dst_height && info->src.box.depth == 1 && !dst->surface.is_linear &&
   (!dst->cmask_buffer || !dst->dirty_level_mask)) { /* dst cannot be fast-cleared */
   /* Check the remaining constraints. */
   if (src->surface.micro_tile_mode != dst->surface.micro_tile_mode ||
      need_rgb_to_bgr) {
   /* The next fast clear will switch to this mode to
   * get direct hw resolve next time if the mode is
   * different now.
   *
   * TODO-GFX10: This does not work in GFX10 because MSAA
   * is restricted to 64KB_R_X and 64KB_Z_X swizzle modes.
   * In some cases we could change the swizzle of the
   * destination texture instead, but the more general
   * solution is to implement compute shader resolve.
   */
   if (src->surface.micro_tile_mode != dst->surface.micro_tile_mode)
                                    /* Resolving into a surface with DCC is unsupported. Since
   * it's being overwritten anyway, clear it to uncompressed.
   * This is still the fastest codepath even with this clear.
   */
   if (vi_dcc_enabled(dst, info->dst.level)) {
                              si_execute_clears(sctx, &clear_info, 1, SI_CLEAR_TYPE_DCC);
               /* Resolve directly from src to dst. */
   si_do_CB_resolve(sctx, info, info->dst.resource, info->dst.level, info->dst.box.z, format);
            resolve_to_temp:
      /* Shader-based resolve is VERY SLOW. Instead, resolve into
   * a temporary texture and blit.
   */
   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = info->src.resource->format;
   templ.width0 = info->src.resource->width0;
   templ.height0 = info->src.resource->height0;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.usage = PIPE_USAGE_DEFAULT;
   templ.flags = SI_RESOURCE_FLAG_FORCE_MSAA_TILING | SI_RESOURCE_FLAG_FORCE_MICRO_TILE_MODE |
                  /* The src and dst microtile modes must be the same. */
   if (sctx->gfx_level <= GFX8 && src->surface.micro_tile_mode == RADEON_MICRO_MODE_DISPLAY)
         else
            tmp = ctx->screen->resource_create(ctx->screen, &templ);
   if (!tmp)
         stmp = (struct si_texture *)tmp;
   /* Match the channel order of src. */
            assert(!stmp->surface.is_linear);
            /* resolve */
            /* blit */
   blit = *info;
   blit.src.resource = tmp;
                     pipe_resource_reference(&tmp, NULL);
      }
      static void si_blit(struct pipe_context *ctx, const struct pipe_blit_info *info)
   {
      struct si_context *sctx = (struct si_context *)ctx;
            if (sctx->gfx_level >= GFX7 &&
      (info->dst.resource->bind & PIPE_BIND_PRIME_BLIT_DST) && sdst->surface.is_linear &&
   /* Use SDMA or async compute when copying to a DRI_PRIME imported linear surface. */
   info->dst.box.x == 0 && info->dst.box.y == 0 && info->dst.box.z == 0 &&
   info->src.box.x == 0 && info->src.box.y == 0 && info->src.box.z == 0 &&
   info->dst.level == 0 && info->src.level == 0 &&
   info->src.box.width == info->dst.resource->width0 &&
   info->src.box.height == info->dst.resource->height0 &&
   info->src.box.depth == 1 &&
   util_can_blit_via_copy_region(info, true, sctx->render_cond != NULL)) {
            /* Try SDMA first... */
   if (si_sdma_copy_image(sctx, sdst, ssrc))
            /* ... and use async compute as the fallback. */
            simple_mtx_lock(&sscreen->async_compute_context_lock);
   if (!sscreen->async_compute_context)
            if (sscreen->async_compute_context) {
      si_compute_copy_image((struct si_context*)sctx->screen->async_compute_context,
               si_flush_gfx_cs((struct si_context*)sctx->screen->async_compute_context, 0, NULL);
   simple_mtx_unlock(&sscreen->async_compute_context_lock);
                           if (unlikely(sctx->sqtt_enabled))
            if (si_msaa_resolve_blit_via_CB(ctx, info))
            if (unlikely(sctx->sqtt_enabled))
            /* Using compute for copying to a linear texture in GTT is much faster than
   * going through RBs (render backends). This improves DRI PRIME performance.
   */
   if (util_can_blit_via_copy_region(info, false, sctx->render_cond != NULL)) {
      si_resource_copy_region(ctx, info->dst.resource, info->dst.level,
                           if (si_compute_blit(sctx, info, false))
               }
      void si_gfx_blit(struct pipe_context *ctx, const struct pipe_blit_info *info)
   {
                        /* The driver doesn't decompress resources automatically while
   * u_blitter is rendering. */
   vi_disable_dcc_if_incompatible_format(sctx, info->src.resource, info->src.level,
         vi_disable_dcc_if_incompatible_format(sctx, info->dst.resource, info->dst.level,
         si_decompress_subresource(ctx, info->src.resource, PIPE_MASK_RGBAZS, info->src.level,
                  if (unlikely(sctx->sqtt_enabled))
            si_blitter_begin(sctx, SI_BLIT | (info->render_condition_enable ? 0 : SI_DISABLE_RENDER_COND));
   util_blitter_blit(sctx->blitter, info);
      }
      static bool si_generate_mipmap(struct pipe_context *ctx, struct pipe_resource *tex,
               {
      struct si_context *sctx = (struct si_context *)ctx;
            if (!util_blitter_is_copy_supported(sctx->blitter, tex, tex))
            /* The driver doesn't decompress resources automatically while
   * u_blitter is rendering. */
   vi_disable_dcc_if_incompatible_format(sctx, tex, base_level, format);
   si_decompress_subresource(ctx, tex, PIPE_MASK_RGBAZS, base_level, first_layer, last_layer,
            /* Clear dirty_level_mask for the levels that will be overwritten. */
   assert(base_level < last_level);
                     si_blitter_begin(sctx, SI_BLIT | SI_DISABLE_RENDER_COND);
   util_blitter_generate_mipmap(sctx->blitter, tex, format, base_level, last_level, first_layer,
                  sctx->generate_mipmap_for_depth = false;
      }
      static void si_flush_resource(struct pipe_context *ctx, struct pipe_resource *res)
   {
      struct si_context *sctx = (struct si_context *)ctx;
            if (res->target == PIPE_BUFFER)
            if (!tex->is_depth && (tex->cmask_buffer || vi_dcc_enabled(tex, 0))) {
      si_blit_decompress_color(sctx, tex, 0, res->last_level, 0, util_max_layer(res, 0),
            if (tex->surface.display_dcc_offset && tex->displayable_dcc_dirty) {
      si_retile_dcc(sctx, tex);
            }
      void si_flush_implicit_resources(struct si_context *sctx)
   {
      hash_table_foreach(sctx->dirty_implicit_resources, entry) {
      si_flush_resource(&sctx->b, entry->data);
      }
      }
      void si_decompress_dcc(struct si_context *sctx, struct si_texture *tex)
   {
               /* If graphics is disabled, we can't decompress DCC, but it shouldn't
   * be compressed either. The caller should simply discard it.
   * If blitter is running, we can't decompress DCC either because it
   * will cause a blitter recursion.
   */
   if (!tex->surface.meta_offset || !sctx->has_graphics || sctx->blitter_running)
            si_blit_decompress_color(sctx, tex, 0, tex->buffer.b.b.last_level, 0,
      }
      void si_init_blit_functions(struct si_context *sctx)
   {
               if (sctx->has_graphics) {
      sctx->b.blit = si_blit;
   sctx->b.flush_resource = si_flush_resource;
         }
