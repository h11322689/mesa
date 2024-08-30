   /*
   * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
   * Copyright 2018 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "drm-uapi/drm_fourcc.h"
   #include "si_pipe.h"
   #include "si_query.h"
   #include "sid.h"
   #include "frontend/drm_driver.h"
   #include "util/format/u_format.h"
   #include "util/os_time.h"
   #include "util/u_log.h"
   #include "util/u_memory.h"
   #include "util/u_pack_color.h"
   #include "util/u_resource.h"
   #include "util/u_surface.h"
   #include "util/u_transfer.h"
      #include <errno.h>
   #include <inttypes.h>
      #include "amd/addrlib/inc/addrinterface.h"
      static enum radeon_surf_mode si_choose_tiling(struct si_screen *sscreen,
                  static bool si_texture_is_aux_plane(const struct pipe_resource *resource);
      /* Same as resource_copy_region, except that both upsampling and downsampling are allowed. */
   static void si_copy_region_with_blit(struct pipe_context *pipe, struct pipe_resource *dst,
                     {
               memset(&blit, 0, sizeof(blit));
   blit.src.resource = src;
   blit.src.format = src->format;
   blit.src.level = src_level;
   blit.src.box = *src_box;
   blit.dst.resource = dst;
   blit.dst.format = dst->format;
   blit.dst.level = dst_level;
   blit.dst.box.x = dstx;
   blit.dst.box.y = dsty;
   blit.dst.box.z = dstz;
   blit.dst.box.width = src_box->width;
   blit.dst.box.height = src_box->height;
   blit.dst.box.depth = src_box->depth;
   blit.mask = util_format_get_mask(dst->format);
   blit.filter = PIPE_TEX_FILTER_NEAREST;
            if (blit.mask) {
      /* Only the gfx blit handles dst_sample. */
   if (dst_sample)
         else
         }
      /* Copy all planes of multi-plane texture */
   static bool si_copy_multi_plane_texture(struct pipe_context *ctx, struct pipe_resource *dst,
                     {
      unsigned i, dx, dy;
   struct si_texture *src_tex = (struct si_texture *)src;
   struct si_texture *dst_tex = (struct si_texture *)dst;
            if (src_tex->multi_plane_format == PIPE_FORMAT_NONE || src_tex->plane_index != 0)
            assert(src_tex->multi_plane_format == dst_tex->multi_plane_format);
                     for (i = 0; i < src_tex->num_planes && src && dst; ++i) {
      dx = util_format_get_plane_width(src_tex->multi_plane_format, i, dstx);
   dy = util_format_get_plane_height(src_tex->multi_plane_format, i, dsty);
   sbox.x = util_format_get_plane_width(src_tex->multi_plane_format, i, src_box->x);
   sbox.y = util_format_get_plane_height(src_tex->multi_plane_format, i, src_box->y);
   sbox.width = util_format_get_plane_width(src_tex->multi_plane_format, i, src_box->width);
                     src = src->next;
                  }
      /* Copy from a full GPU texture to a transfer's staging one. */
   static void si_copy_to_staging_texture(struct pipe_context *ctx, struct si_transfer *stransfer)
   {
      struct pipe_transfer *transfer = (struct pipe_transfer *)stransfer;
   struct pipe_resource *dst = &stransfer->staging->b.b;
   struct pipe_resource *src = transfer->resource;
   /* level means sample_index - 1 with MSAA. Used by texture uploads. */
            if (src->nr_samples > 1 || ((struct si_texture *)src)->is_depth) {
      si_copy_region_with_blit(ctx, dst, 0, 0, 0, 0, 0, src, src_level, &transfer->box);
               if (si_copy_multi_plane_texture(ctx, dst, 0, 0, 0, 0, src, src_level, &transfer->box))
               }
      /* Copy from a transfer's staging texture to a full GPU one. */
   static void si_copy_from_staging_texture(struct pipe_context *ctx, struct si_transfer *stransfer)
   {
      struct pipe_transfer *transfer = (struct pipe_transfer *)stransfer;
   struct pipe_resource *dst = transfer->resource;
   struct pipe_resource *src = &stransfer->staging->b.b;
                     if (dst->nr_samples > 1 || ((struct si_texture *)dst)->is_depth) {
      unsigned dst_level = dst->nr_samples > 1 ? 0 : transfer->level;
            si_copy_region_with_blit(ctx, dst, dst_level, dst_sample, transfer->box.x, transfer->box.y,
                     if (si_copy_multi_plane_texture(ctx, dst, transfer->level, transfer->box.x, transfer->box.y,
                  if (util_format_is_compressed(dst->format)) {
      sbox.width = util_format_get_nblocksx(dst->format, sbox.width);
               si_resource_copy_region(ctx, dst, transfer->level, transfer->box.x, transfer->box.y,
      }
      static uint64_t si_texture_get_offset(struct si_screen *sscreen, struct si_texture *tex,
               {
      if (sscreen->info.gfx_level >= GFX9) {
      unsigned pitch;
   if (tex->surface.is_linear) {
         } else {
                  *stride = pitch * tex->surface.bpe;
            if (!box)
            /* Each texture is an array of slices. Each slice is an array
   * of mipmap levels. */
   return tex->surface.u.gfx9.surf_offset + box->z * tex->surface.u.gfx9.surf_slice_size +
         tex->surface.u.gfx9.offset[level] +
      } else {
      *stride = tex->surface.u.legacy.level[level].nblk_x * tex->surface.bpe;
   assert((uint64_t)tex->surface.u.legacy.level[level].slice_size_dw * 4 <= UINT_MAX);
            if (!box)
            /* Each texture is an array of mipmap levels. Each level is
   * an array of slices. */
   return (uint64_t)tex->surface.u.legacy.level[level].offset_256B * 256 +
         box->z * (uint64_t)tex->surface.u.legacy.level[level].slice_size_dw * 4 +
   (box->y / tex->surface.blk_h * tex->surface.u.legacy.level[level].nblk_x +
         }
      static int si_init_surface(struct si_screen *sscreen, struct radeon_surf *surface,
                     {
      const struct util_format_description *desc = util_format_description(ptex->format);
   bool is_depth, is_stencil;
   int r;
   unsigned bpe;
            is_depth = util_format_has_depth(desc);
            if (!is_flushed_depth && ptex->format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT) {
         } else {
      bpe = util_format_get_blocksize(ptex->format);
               if (!is_flushed_depth && is_depth) {
               if ((sscreen->debug_flags & DBG(NO_HYPERZ)) ||
      (ptex->bind & PIPE_BIND_SHARED) || is_imported) {
      } else if (tc_compatible_htile &&
            /* TC-compatible HTILE only supports Z32_FLOAT.
   * GFX9 also supports Z16_UNORM.
   * On GFX8, promote Z16 to Z32. DB->CB copies will convert
   * the format for transfers.
   */
                              if (is_stencil)
               /* Disable DCC? (it can't be disabled if modifiers are used) */
   if (sscreen->info.gfx_level >= GFX8 && modifier == DRM_FORMAT_MOD_INVALID && !is_imported) {
      /* Global options that disable DCC. */
   if (ptex->flags & SI_RESOURCE_FLAG_DISABLE_DCC)
            if (ptex->nr_samples >= 2 && sscreen->debug_flags & DBG(NO_DCC_MSAA))
            /* Shared textures must always set up DCC. If it's not present, it will be disabled by
   * si_get_opaque_metadata later.
   */
   if (!is_imported && sscreen->debug_flags & DBG(NO_DCC))
            /* R9G9B9E5 isn't supported for rendering by older generations. */
   if (sscreen->info.gfx_level < GFX10_3 &&
                  /* If constant (non-data-dependent) format is requested, disable DCC: */
   if (ptex->bind & PIPE_BIND_CONST_BW)
            switch (sscreen->info.gfx_level) {
   case GFX8:
      /* Stoney: 128bpp MSAA textures randomly fail piglit tests with DCC. */
                  /* DCC clear for 4x and 8x MSAA array textures unimplemented. */
   if (ptex->nr_storage_samples >= 4 && ptex->array_size > 1)
               case GFX9:
      /* DCC MSAA fails this on Raven:
   *    https://www.khronos.org/registry/webgl/sdk/tests/deqp/functional/gles3/fbomultisample.2_samples.html
   * and this on Picasso:
   *    https://www.khronos.org/registry/webgl/sdk/tests/deqp/functional/gles3/fbomultisample.4_samples.html
   */
   if (sscreen->info.family == CHIP_RAVEN && ptex->nr_storage_samples >= 2 && bpe < 4)
               case GFX10:
   case GFX10_3:
      if (ptex->nr_storage_samples >= 2 && !sscreen->options.dcc_msaa)
               case GFX11:
   case GFX11_5:
            default:
                     if (is_scanout) {
      /* This should catch bugs in gallium users setting incorrect flags. */
   assert(ptex->nr_samples <= 1 && ptex->array_size == 1 && ptex->depth0 == 1 &&
                        if (ptex->bind & PIPE_BIND_SHARED)
         if (is_imported)
         if (sscreen->debug_flags & DBG(NO_FMASK))
            if (sscreen->info.gfx_level == GFX9 && (ptex->flags & SI_RESOURCE_FLAG_FORCE_MICRO_TILE_MODE)) {
      flags |= RADEON_SURF_FORCE_MICRO_TILE_MODE;
               if (ptex->flags & SI_RESOURCE_FLAG_FORCE_MSAA_TILING) {
      /* GFX11 shouldn't get here because the flag is only used by the CB MSAA resolving
   * that GFX11 doesn't have.
   */
                     if (sscreen->info.gfx_level >= GFX10)
               if (ptex->flags & PIPE_RESOURCE_FLAG_SPARSE) {
      flags |=
      RADEON_SURF_PRT |
   RADEON_SURF_NO_FMASK |
   RADEON_SURF_NO_HTILE |
                     r = sscreen->ws->surface_init(sscreen->ws, &sscreen->info, ptex, flags, bpe, array_mode,
         if (r) {
                     }
      void si_eliminate_fast_color_clear(struct si_context *sctx, struct si_texture *tex,
         {
               unsigned n = sctx->num_decompress_calls;
            /* Flush only if any fast clear elimination took place. */
   bool flushed = false;
   if (n != sctx->num_decompress_calls)
   {
      ctx->flush(ctx, NULL, 0);
      }
   if (ctx_flushed)
      }
      void si_texture_discard_cmask(struct si_screen *sscreen, struct si_texture *tex)
   {
      if (!tex->cmask_buffer)
                     /* Disable CMASK. */
   tex->cmask_base_address_reg = tex->buffer.gpu_address >> 8;
                     if (tex->cmask_buffer != &tex->buffer)
                     /* Notify all contexts about the change. */
   p_atomic_inc(&sscreen->dirty_tex_counter);
      }
      static bool si_can_disable_dcc(struct si_texture *tex)
   {
      /* We can't disable DCC if it can be written by another process. */
   return !tex->is_depth &&
         tex->surface.meta_offset &&
   (!tex->buffer.b.is_shared ||
      }
      static bool si_texture_discard_dcc(struct si_screen *sscreen, struct si_texture *tex)
   {
      if (!si_can_disable_dcc(tex))
            /* Disable DCC. */
            /* Notify all contexts about the change. */
   p_atomic_inc(&sscreen->dirty_tex_counter);
      }
      /**
   * Disable DCC for the texture. (first decompress, then discard metadata).
   *
   * There is unresolved multi-context synchronization issue between
   * screen::aux_context and the current context. If applications do this with
   * multiple contexts, it's already undefined behavior for them and we don't
   * have to worry about that. The scenario is:
   *
   * If context 1 disables DCC and context 2 has queued commands that write
   * to the texture via CB with DCC enabled, and the order of operations is
   * as follows:
   *   context 2 queues draw calls rendering to the texture, but doesn't flush
   *   context 1 disables DCC and flushes
   *   context 1 & 2 reset descriptors and FB state
   *   context 2 flushes (new compressed tiles written by the draw calls)
   *   context 1 & 2 read garbage, because DCC is disabled, yet there are
   *   compressed tiled
   *
   * \param sctx  the current context if you have one, or sscreen->aux_context
   *              if you don't.
   */
   bool si_texture_disable_dcc(struct si_context *sctx, struct si_texture *tex)
   {
               if (!sctx->has_graphics)
            if (!si_can_disable_dcc(tex))
            /* Decompress DCC. */
   si_decompress_dcc(sctx, tex);
               }
      static void si_reallocate_texture_inplace(struct si_context *sctx, struct si_texture *tex,
         {
      struct pipe_screen *screen = sctx->b.screen;
   struct si_texture *new_tex;
   struct pipe_resource templ = tex->buffer.b.b;
                     if (tex->buffer.b.is_shared || tex->num_planes > 1)
            if (new_bind_flag == PIPE_BIND_LINEAR) {
      if (tex->surface.is_linear)
            /* This fails with MSAA, depth, and compressed textures. */
   if (si_choose_tiling(sctx->screen, &templ, false) != RADEON_SURF_MODE_LINEAR_ALIGNED)
               /* Inherit the modifier from the old texture. */
   if (tex->surface.modifier != DRM_FORMAT_MOD_INVALID && screen->resource_create_with_modifiers)
      new_tex = (struct si_texture *)screen->resource_create_with_modifiers(screen, &templ,
      else
            if (!new_tex)
            /* Copy the pixels to the new texture. */
   if (!invalidate_storage) {
      for (i = 0; i <= templ.last_level; i++) {
                              si_resource_copy_region(&sctx->b, &new_tex->buffer.b.b,
                  if (new_bind_flag == PIPE_BIND_LINEAR) {
      si_texture_discard_cmask(sctx->screen, tex);
               /* Replace the structure fields of tex. */
   tex->buffer.b.b.bind = templ.bind;
   radeon_bo_reference(sctx->screen->ws, &tex->buffer.buf, new_tex->buffer.buf);
   tex->buffer.gpu_address = new_tex->buffer.gpu_address;
   tex->buffer.bo_size = new_tex->buffer.bo_size;
   tex->buffer.bo_alignment_log2 = new_tex->buffer.bo_alignment_log2;
   tex->buffer.domains = new_tex->buffer.domains;
            tex->surface = new_tex->surface;
            tex->surface.fmask_offset = new_tex->surface.fmask_offset;
   tex->surface.cmask_offset = new_tex->surface.cmask_offset;
            if (tex->cmask_buffer == &tex->buffer)
         else
            if (new_tex->cmask_buffer == &new_tex->buffer)
         else
            tex->surface.meta_offset = new_tex->surface.meta_offset;
   tex->cb_color_info = new_tex->cb_color_info;
   memcpy(tex->color_clear_value, new_tex->color_clear_value, sizeof(tex->color_clear_value));
            memcpy(tex->depth_clear_value, new_tex->depth_clear_value, sizeof(tex->depth_clear_value));
   tex->dirty_level_mask = new_tex->dirty_level_mask;
   tex->stencil_dirty_level_mask = new_tex->stencil_dirty_level_mask;
   tex->db_render_format = new_tex->db_render_format;
   memcpy(tex->stencil_clear_value, new_tex->stencil_clear_value, sizeof(tex->stencil_clear_value));
   tex->tc_compatible_htile = new_tex->tc_compatible_htile;
   tex->depth_cleared_level_mask_once = new_tex->depth_cleared_level_mask_once;
   tex->stencil_cleared_level_mask_once = new_tex->stencil_cleared_level_mask_once;
   tex->upgraded_depth = new_tex->upgraded_depth;
   tex->db_compatible = new_tex->db_compatible;
   tex->can_sample_z = new_tex->can_sample_z;
                     if (new_bind_flag == PIPE_BIND_LINEAR) {
      assert(!tex->surface.meta_offset);
   assert(!tex->cmask_buffer);
   assert(!tex->surface.fmask_size);
                           }
      static void si_set_tex_bo_metadata(struct si_screen *sscreen, struct si_texture *tex)
   {
      struct pipe_resource *res = &tex->buffer.b.b;
                              static const unsigned char swizzle[] = {PIPE_SWIZZLE_X, PIPE_SWIZZLE_Y, PIPE_SWIZZLE_Z,
         bool is_array = util_texture_is_array(res->target);
            sscreen->make_texture_descriptor(sscreen, tex, true, res->target, res->format, swizzle, 0,
               si_set_mutable_tex_desc_fields(sscreen, tex, &tex->surface.u.legacy.level[0], 0, 0,
            ac_surface_compute_umd_metadata(&sscreen->info, &tex->surface,
                        }
      static bool si_displayable_dcc_needs_explicit_flush(struct si_texture *tex)
   {
               if (sscreen->info.gfx_level <= GFX8)
            /* With modifiers and > 1 planes any applications will know that they
   * cannot do frontbuffer rendering with the texture. */
   if (ac_surface_get_nplanes(&tex->surface) > 1)
               }
      static bool si_resource_get_param(struct pipe_screen *screen, struct pipe_context *context,
                           {
      while (plane && resource->next && !si_texture_is_aux_plane(resource->next)) {
      --plane;
               struct si_screen *sscreen = (struct si_screen *)screen;
   struct si_texture *tex = (struct si_texture *)resource;
            switch (param) {
   case PIPE_RESOURCE_PARAM_NPLANES:
      if (resource->target == PIPE_BUFFER)
         else if (tex->num_planes > 1)
         else
               case PIPE_RESOURCE_PARAM_STRIDE:
      if (resource->target == PIPE_BUFFER)
         else
      *value = ac_surface_get_plane_stride(sscreen->info.gfx_level,
            case PIPE_RESOURCE_PARAM_OFFSET:
      if (resource->target == PIPE_BUFFER) {
         } else {
      uint64_t level_offset = 0;
   if (sscreen->info.gfx_level >= GFX9 && tex->surface.is_linear)
         *value = ac_surface_get_plane_offset(sscreen->info.gfx_level,
      }
         case PIPE_RESOURCE_PARAM_MODIFIER:
      *value = tex->surface.modifier;
         case PIPE_RESOURCE_PARAM_HANDLE_TYPE_SHARED:
   case PIPE_RESOURCE_PARAM_HANDLE_TYPE_KMS:
   case PIPE_RESOURCE_PARAM_HANDLE_TYPE_FD:
               if (param == PIPE_RESOURCE_PARAM_HANDLE_TYPE_SHARED)
         else if (param == PIPE_RESOURCE_PARAM_HANDLE_TYPE_KMS)
         else if (param == PIPE_RESOURCE_PARAM_HANDLE_TYPE_FD)
            if (!screen->resource_get_handle(screen, context, resource, &whandle, handle_usage))
            *value = whandle.handle;
      case PIPE_RESOURCE_PARAM_LAYER_STRIDE:
         }
      }
      static void si_texture_get_info(struct pipe_screen *screen, struct pipe_resource *resource,
         {
               if (pstride) {
      si_resource_get_param(screen, NULL, resource, 0, 0, 0, PIPE_RESOURCE_PARAM_STRIDE, 0, &value);
               if (poffset) {
      si_resource_get_param(screen, NULL, resource, 0, 0, 0, PIPE_RESOURCE_PARAM_OFFSET, 0, &value);
         }
      static bool si_texture_get_handle(struct pipe_screen *screen, struct pipe_context *ctx,
               {
      struct si_screen *sscreen = (struct si_screen *)screen;
   struct si_context *sctx;
   struct si_resource *res = si_resource(resource);
   struct si_texture *tex = (struct si_texture *)resource;
   bool update_metadata = false;
   unsigned stride, offset, slice_size;
   uint64_t modifier = DRM_FORMAT_MOD_INVALID;
            ctx = threaded_context_unwrap_sync(ctx);
            if (resource->target != PIPE_BUFFER) {
               /* Individual planes are chained pipe_resource instances. */
   while (plane && resource->next && !si_texture_is_aux_plane(resource->next)) {
      resource = resource->next;
               res = si_resource(resource);
            /* This is not supported now, but it might be required for OpenCL
   * interop in the future.
   */
   if (resource->nr_samples > 1 || tex->is_depth) {
      if (!ctx)
                              if (plane) {
      if (!ctx)
         whandle->offset = ac_surface_get_plane_offset(sscreen->info.gfx_level,
         whandle->stride = ac_surface_get_plane_stride(sscreen->info.gfx_level,
         whandle->modifier = tex->surface.modifier;
               /* Move a suballocated texture into a non-suballocated allocation. */
   if (sscreen->ws->buffer_is_suballocated(res->buf) || tex->surface.tile_swizzle ||
      (tex->buffer.flags & RADEON_FLAG_NO_INTERPROCESS_SHARING &&
   sscreen->info.has_local_buffers)) {
   assert(!res->b.is_shared);
   si_reallocate_texture_inplace(sctx, tex, PIPE_BIND_SHARED, false);
   flush = true;
   assert(res->b.b.bind & PIPE_BIND_SHARED);
   assert(res->flags & RADEON_FLAG_NO_SUBALLOC);
   assert(!(res->flags & RADEON_FLAG_NO_INTERPROCESS_SHARING));
               /* Since shader image stores don't support DCC on GFX8,
   * disable it for external clients that want write
   * access.
   */
   if (sscreen->debug_flags & DBG(NO_EXPORTED_DCC) ||
      (usage & PIPE_HANDLE_USAGE_SHADER_WRITE && !tex->is_depth && tex->surface.meta_offset) ||
   /* Displayable DCC requires an explicit flush. */
   (!(usage & PIPE_HANDLE_USAGE_EXPLICIT_FLUSH) &&
   si_displayable_dcc_needs_explicit_flush(tex))) {
   if (si_texture_disable_dcc(sctx, tex)) {
      update_metadata = true;
   /* si_texture_disable_dcc flushes the context */
                  if (!(usage & PIPE_HANDLE_USAGE_EXPLICIT_FLUSH) &&
      (tex->cmask_buffer || (!tex->is_depth && tex->surface.meta_offset))) {
   /* Eliminate fast clear (both CMASK and DCC) */
   bool flushed;
   si_eliminate_fast_color_clear(sctx, tex, &flushed);
                  /* Disable CMASK if flush_resource isn't going
   * to be called.
   */
   if (tex->cmask_buffer)
               /* Set metadata. */
   if ((!res->b.is_shared || update_metadata) && whandle->offset == 0)
            if (sscreen->info.gfx_level >= GFX9) {
         } else {
                     } else {
               /* Buffer exports are for the OpenCL interop. */
   /* Move a suballocated buffer into a non-suballocated allocation. */
   if (sscreen->ws->buffer_is_suballocated(res->buf) ||
      /* A DMABUF export always fails if the BO is local. */
   (tex->buffer.flags & RADEON_FLAG_NO_INTERPROCESS_SHARING &&
                  /* Allocate a new buffer with PIPE_BIND_SHARED. */
                  struct pipe_resource *newb = screen->resource_create(screen, &templ);
   if (!newb) {
      if (!ctx)
                     /* Copy the old buffer contents to the new one. */
   struct pipe_box box;
   u_box_1d(0, newb->width0, &box);
   sctx->b.resource_copy_region(&sctx->b, newb, 0, 0, 0, 0, &res->b.b, 0, &box);
   flush = true;
   /* Move the new buffer storage to the old pipe_resource. */
                  assert(res->b.b.bind & PIPE_BIND_SHARED);
               /* Buffers */
                        if (res->b.is_shared) {
      /* USAGE_EXPLICIT_FLUSH must be cleared if at least one user
   * doesn't set it.
   */
   res->external_usage |= usage & ~PIPE_HANDLE_USAGE_EXPLICIT_FLUSH;
   if (!(usage & PIPE_HANDLE_USAGE_EXPLICIT_FLUSH))
      } else {
      res->b.is_shared = true;
               if (flush && ctx)
         if (!ctx)
            whandle->stride = stride;
   whandle->offset = offset + slice_size * whandle->layer;
               }
      void si_print_texture_info(struct si_screen *sscreen, struct si_texture *tex,
         {
      int i;
   FILE *f;
   char *surf_info = NULL;
            /* Common parameters. */
   u_log_printf(log,
               "  Info: npix_x=%u, npix_y=%u, npix_z=%u, "
   "array_size=%u, last_level=%u, nsamples=%u",
            if (tex->is_depth && tex->surface.meta_offset)
            u_log_printf(log, ", %s\n",
            f = open_memstream(&surf_info, &surf_info_size);
   if (!f)
         ac_surface_print_info(f, &sscreen->info, &tex->surface);
   fclose(f);
   u_log_printf(log, "%s", surf_info);
            if (sscreen->info.gfx_level >= GFX9) {
                  if (!tex->is_depth && tex->surface.meta_offset) {
      for (i = 0; i <= tex->buffer.b.b.last_level; i++)
      u_log_printf(log,
               "    DCCLevel[%i]: enabled=%u, offset=%u, "
            for (i = 0; i <= tex->buffer.b.b.last_level; i++)
      u_log_printf(log,
               "    Level[%i]: offset=%" PRIu64 ", slice_size=%" PRIu64 ", "
   "npix_x=%u, npix_y=%u, npix_z=%u, nblk_x=%u, nblk_y=%u, "
   "mode=%u, tiling_index = %u\n",
   i, (uint64_t)tex->surface.u.legacy.level[i].offset_256B * 256,
   (uint64_t)tex->surface.u.legacy.level[i].slice_size_dw * 4,
   u_minify(tex->buffer.b.b.width0, i), u_minify(tex->buffer.b.b.height0, i),
         if (tex->surface.has_stencil) {
      for (i = 0; i <= tex->buffer.b.b.last_level; i++) {
      u_log_printf(log,
               "    StencilLevel[%i]: offset=%" PRIu64 ", "
   "slice_size=%" PRIu64 ", npix_x=%u, "
   "npix_y=%u, npix_z=%u, nblk_x=%u, nblk_y=%u, "
   "mode=%u, tiling_index = %u\n",
   i, (uint64_t)tex->surface.u.legacy.zs.stencil_level[i].offset_256B * 256,
   (uint64_t)tex->surface.u.legacy.zs.stencil_level[i].slice_size_dw * 4,
   u_minify(tex->buffer.b.b.width0, i), u_minify(tex->buffer.b.b.height0, i),
   u_minify(tex->buffer.b.b.depth0, i),
   tex->surface.u.legacy.zs.stencil_level[i].nblk_x,
            }
      /**
   * Common function for si_texture_create and si_texture_from_handle.
   *
   * \param screen       screen
   * \param base         resource template
   * \param surface      radeon_surf
   * \param plane0       if a non-zero plane is being created, this is the first plane
   * \param imported_buf from si_texture_from_handle
   * \param offset       offset for non-zero planes or imported buffers
   * \param alloc_size   the size to allocate if plane0 != NULL
   * \param alignment    alignment for the allocation
   */
   static struct si_texture *si_texture_create_object(struct pipe_screen *screen,
                                       {
      struct si_texture *tex;
   struct si_resource *resource;
            if (!sscreen->info.has_3d_cube_border_color_mipmap &&
      (base->last_level > 0 ||
   base->target == PIPE_TEXTURE_3D ||
   base->target == PIPE_TEXTURE_CUBE)) {
   assert(0);
               tex = CALLOC_STRUCT_CL(si_texture);
   if (!tex)
            resource = &tex->buffer;
   resource->b.b = *base;
   pipe_reference_init(&resource->b.b.reference, 1);
            /* don't include stencil-only formats which we don't support for rendering */
   tex->is_depth = util_format_has_depth(util_format_description(tex->buffer.b.b.format));
            if (!ac_surface_override_offset_stride(&sscreen->info, &tex->surface,
                              if (plane0) {
      /* The buffer is shared with the first plane. */
   resource->bo_size = plane0->buffer.bo_size;
   resource->bo_alignment_log2 = plane0->buffer.bo_alignment_log2;
   resource->flags = plane0->buffer.flags;
            radeon_bo_reference(sscreen->ws, &resource->buf, plane0->buffer.buf);
      } else if (!(surface->flags & RADEON_SURF_IMPORTED)) {
      if (base->flags & PIPE_RESOURCE_FLAG_SPARSE)
         if (base->bind & PIPE_BIND_PRIME_BLIT_DST)
            /* Create the backing buffer. */
            if (!si_alloc_resource(sscreen, resource))
      } else {
      resource->buf = imported_buf;
   resource->gpu_address = sscreen->ws->buffer_get_virtual_address(resource->buf);
   resource->bo_size = imported_buf->size;
   resource->bo_alignment_log2 = imported_buf->alignment_log2;
   resource->domains = sscreen->ws->buffer_get_initial_domain(resource->buf);
   if (sscreen->ws->buffer_get_flags)
               if (sscreen->debug_flags & DBG(VM)) {
      fprintf(stderr,
         "VM start=0x%" PRIX64 "  end=0x%" PRIX64
   " | Texture %ix%ix%i, %i levels, %i samples, %s | Flags: ",
   tex->buffer.gpu_address, tex->buffer.gpu_address + tex->buffer.buf->size,
   base->width0, base->height0, util_num_layers(base, 0), base->last_level + 1,
   si_res_print_flags(tex->buffer.flags);
               if (sscreen->debug_flags & DBG(TEX)) {
      puts("Texture:");
   struct u_log_context log;
   u_log_context_init(&log);
   si_print_texture_info(sscreen, tex, &log);
   u_log_new_page_print(&log, stdout);
   fflush(stdout);
               /* Use 1.0 as the default clear value to get optimal ZRANGE_PRECISION if we don't
   * get a fast clear.
   */
   for (unsigned i = 0; i < ARRAY_SIZE(tex->depth_clear_value); i++)
            /* On GFX8, HTILE uses different tiling depending on the TC_COMPATIBLE_HTILE
   * setting, so we have to enable it if we enabled it at allocation.
   *
   * GFX9 and later use the same tiling for both, so TC-compatible HTILE can be
   * enabled on demand.
   */
   tex->tc_compatible_htile = (sscreen->info.gfx_level == GFX8 &&
                                    /* TC-compatible HTILE:
   * - GFX8 only supports Z32_FLOAT.
   * - GFX9 only supports Z32_FLOAT and Z16_UNORM. */
   if (tex->surface.flags & RADEON_SURF_TC_COMPATIBLE_HTILE) {
      if (sscreen->info.gfx_level >= GFX9 && base->format == PIPE_FORMAT_Z16_UNORM)
         else {
      tex->db_render_format = PIPE_FORMAT_Z32_FLOAT;
   tex->upgraded_depth = base->format != PIPE_FORMAT_Z32_FLOAT &&
         } else {
                  /* Applies to GCN. */
            if (tex->is_depth) {
               if (sscreen->info.gfx_level >= GFX9) {
                     /* Stencil texturing with HTILE doesn't work
   * with mipmapping on Navi10-14. */
   if (sscreen->info.gfx_level == GFX10 && base->last_level > 0)
      } else {
                     /* GFX8 must keep stencil enabled because it can't use Z-only TC-compatible
   * HTILE because of a hw bug. This has only a small effect on performance
   * because we lose a little bit of Z precision in order to make space for
   * stencil in HTILE.
   */
   if (sscreen->info.gfx_level == GFX8 &&
      tex->surface.flags & RADEON_SURF_TC_COMPATIBLE_HTILE)
               } else {
      if (tex->surface.cmask_offset) {
      assert(sscreen->info.gfx_level < GFX11);
   tex->cb_color_info |= S_028C70_FAST_CLEAR(1);
                  /* Prepare metadata clears.  */
   struct si_clear_info clears[4];
            if (tex->cmask_buffer) {
      /* Initialize the cmask to 0xCC (= compressed state). */
   assert(num_clears < ARRAY_SIZE(clears));
   si_init_buffer_clear(&clears[num_clears++], &tex->cmask_buffer->b.b,
            }
   if (tex->is_depth && tex->surface.meta_offset) {
               if (sscreen->info.gfx_level >= GFX9 || tex->tc_compatible_htile)
            assert(num_clears < ARRAY_SIZE(clears));
   si_init_buffer_clear(&clears[num_clears++], &tex->buffer.b.b, tex->surface.meta_offset,
               /* Initialize DCC only if the texture is not being imported. */
   if (!(surface->flags & RADEON_SURF_IMPORTED) && !tex->is_depth && tex->surface.meta_offset) {
      /* Clear DCC to black for all tiles with DCC enabled.
   *
   * This fixes corruption in 3DMark Slingshot Extreme, which
   * uses uninitialized textures, causing corruption.
   */
   if (tex->surface.num_meta_levels == tex->buffer.b.b.last_level + 1 &&
      tex->buffer.b.b.nr_samples <= 2) {
   /* Simple case - all tiles have DCC enabled. */
   assert(num_clears < ARRAY_SIZE(clears));
   si_init_buffer_clear(&clears[num_clears++], &tex->buffer.b.b, tex->surface.meta_offset,
      } else if (sscreen->info.gfx_level >= GFX9) {
      /* Clear to uncompressed. Clearing this to black is complicated. */
   assert(num_clears < ARRAY_SIZE(clears));
   si_init_buffer_clear(&clears[num_clears++], &tex->buffer.b.b, tex->surface.meta_offset,
      } else {
      /* GFX8: Initialize mipmap levels and multisamples separately. */
   if (tex->buffer.b.b.nr_samples >= 2) {
      /* Clearing this to black is complicated. */
   assert(num_clears < ARRAY_SIZE(clears));
   si_init_buffer_clear(&clears[num_clears++], &tex->buffer.b.b, tex->surface.meta_offset,
      } else {
                     for (unsigned i = 0; i < tex->surface.num_meta_levels; i++) {
                                       /* Mipmap levels with DCC. */
   if (size) {
      assert(num_clears < ARRAY_SIZE(clears));
   si_init_buffer_clear(&clears[num_clears++], &tex->buffer.b.b, tex->surface.meta_offset, size,
      }
   /* Mipmap levels without DCC. */
   if (size != tex->surface.meta_size) {
      assert(num_clears < ARRAY_SIZE(clears));
   si_init_buffer_clear(&clears[num_clears++], &tex->buffer.b.b, tex->surface.meta_offset + size,
                        /* Initialize displayable DCC that requires the retile blit. */
   if (tex->surface.display_dcc_offset && !(surface->flags & RADEON_SURF_IMPORTED)) {
      /* Uninitialized DCC can hang the display hw.
   * Clear to white to indicate that. */
   assert(num_clears < ARRAY_SIZE(clears));
   si_init_buffer_clear(&clears[num_clears++], &tex->buffer.b.b, tex->surface.display_dcc_offset,
                           /* Execute the clears. */
   if (num_clears) {
      si_execute_clears(si_get_aux_context(&sscreen->aux_context.general), clears, num_clears, 0);
               /* Initialize the CMASK base register value. */
                  error:
      FREE_CL(tex);
      }
      static enum radeon_surf_mode si_choose_tiling(struct si_screen *sscreen,
               {
      const struct util_format_description *desc = util_format_description(templ->format);
   bool force_tiling = templ->flags & SI_RESOURCE_FLAG_FORCE_MSAA_TILING;
   bool is_depth_stencil = util_format_is_depth_or_stencil(templ->format) &&
            /* MSAA resources must be 2D tiled. */
   if (templ->nr_samples > 1)
            /* Transfer resources should be linear. */
   if (templ->flags & SI_RESOURCE_FLAG_FORCE_LINEAR)
            /* Avoid Z/S decompress blits by forcing TC-compatible HTILE on GFX8,
   * which requires 2D tiling.
   */
   if (sscreen->info.gfx_level == GFX8 && tc_compatible_htile)
            /* Handle common candidates for the linear mode.
   * Compressed textures and DB surfaces must always be tiled.
   */
   if (!force_tiling && !is_depth_stencil && !util_format_is_compressed(templ->format)) {
      if (sscreen->debug_flags & DBG(NO_TILING) ||
                  /* Tiling doesn't work with the 422 (SUBSAMPLED) formats. */
   if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED)
            /* Cursors are linear on AMD GCN.
   * (XXX double-check, maybe also use RADEON_SURF_SCANOUT) */
   if (templ->bind & PIPE_BIND_CURSOR)
            if (templ->bind & PIPE_BIND_LINEAR)
            /* Textures with a very small height are recommended to be linear. */
   if (templ->target == PIPE_TEXTURE_1D || templ->target == PIPE_TEXTURE_1D_ARRAY ||
      /* Only very thin and long 2D textures should benefit from
   * linear_aligned. */
               /* Textures likely to be mapped often. */
   if (templ->usage == PIPE_USAGE_STAGING || templ->usage == PIPE_USAGE_STREAM)
               /* Make small textures 1D tiled. */
   if (templ->width0 <= 16 || templ->height0 <= 16 || (sscreen->debug_flags & DBG(NO_2D_TILING)))
            /* The allocator will switch to 1D if needed. */
      }
      static struct pipe_resource *
   si_texture_create_with_modifier(struct pipe_screen *screen,
               {
      struct si_screen *sscreen = (struct si_screen *)screen;
            if (templ->nr_samples >= 2) {
      /* This is hackish (overwriting the const pipe_resource template),
   * but should be harmless and gallium frontends can also see
   * the overridden number of samples in the created pipe_resource.
   */
   if (is_zs && sscreen->eqaa_force_z_samples) {
      ((struct pipe_resource *)templ)->nr_samples =
      } else if (!is_zs && sscreen->eqaa_force_color_samples) {
      ((struct pipe_resource *)templ)->nr_samples = sscreen->eqaa_force_coverage_samples;
                  bool is_flushed_depth = templ->flags & SI_RESOURCE_FLAG_FLUSHED_DEPTH ||
         bool tc_compatible_htile =
      sscreen->info.gfx_level >= GFX8 &&
   /* There are issues with TC-compatible HTILE on Tonga (and
   * Iceland is the same design), and documented bug workarounds
   * don't help. For example, this fails:
   *   piglit/bin/tex-miplevel-selection 'texture()' 2DShadow -auto
   */
   sscreen->info.family != CHIP_TONGA && sscreen->info.family != CHIP_ICELAND &&
   (templ->flags & PIPE_RESOURCE_FLAG_TEXTURING_MORE_LIKELY) &&
   !(sscreen->debug_flags & DBG(NO_HYPERZ)) && !is_flushed_depth &&
               /* This allocates textures with multiple planes like NV12 in 1 buffer. */
   enum
   {
         };
   struct radeon_surf surface[SI_TEXTURE_MAX_PLANES] = {};
   struct pipe_resource plane_templ[SI_TEXTURE_MAX_PLANES];
   uint64_t plane_offset[SI_TEXTURE_MAX_PLANES] = {};
   uint64_t total_size = 0;
   unsigned max_alignment = 0;
   unsigned num_planes = util_format_get_num_planes(templ->format);
            /* Compute texture or plane layouts and offsets. */
   for (unsigned i = 0; i < num_planes; i++) {
      plane_templ[i] = *templ;
   plane_templ[i].format = util_format_get_plane_format(templ->format, i);
   plane_templ[i].width0 = util_format_get_plane_width(templ->format, i, templ->width0);
            /* Multi-plane allocations need PIPE_BIND_SHARED, because we can't
   * reallocate the storage to add PIPE_BIND_SHARED, because it's
   * shared by 3 pipe_resources.
   */
   if (num_planes > 1)
         /* Setting metadata on suballocated buffers is impossible. So use PIPE_BIND_CUSTOM to
   * request a non-suballocated buffer.
   */
   if (!is_zs && sscreen->debug_flags & DBG(EXTRA_METADATA))
            if (si_init_surface(sscreen, &surface[i], &plane_templ[i], tile_mode, modifier,
                                 plane_offset[i] = align64(total_size, 1 << surface[i].surf_alignment_log2);
   total_size = plane_offset[i] + surface[i].total_size;
                        for (unsigned i = 0; i < num_planes; i++) {
      struct si_texture *tex =
      si_texture_create_object(screen, &plane_templ[i], &surface[i], plane0, NULL,
      if (!tex) {
      si_texture_reference(&plane0, NULL);
               tex->plane_index = i;
            if (!plane0) {
         } else {
      last_plane->buffer.b.b.next = &tex->buffer.b.b;
      }
   if (i == 0 && !is_zs && sscreen->debug_flags & DBG(EXTRA_METADATA))
               if (num_planes >= 2)
               }
      struct pipe_resource *si_texture_create(struct pipe_screen *screen,
         {
         }
      bool si_texture_commit(struct si_context *ctx, struct si_resource *res, unsigned level,
         {
      struct si_texture *tex = (struct si_texture *)res;
   struct radeon_surf *surface = &tex->surface;
   enum pipe_format format = res->b.b.format;
   unsigned blks = util_format_get_blocksize(format);
                     unsigned row_pitch = surface->u.gfx9.prt_level_pitch[level] *
                  unsigned x = box->x / surface->prt_tile_width;
   unsigned y = box->y / surface->prt_tile_height;
            unsigned w = DIV_ROUND_UP(box->width, surface->prt_tile_width);
   unsigned h = DIV_ROUND_UP(box->height, surface->prt_tile_height);
            /* Align to tile block base, for levels in mip tail whose offset is inside
   * a tile block.
   */
   uint64_t level_base = ROUND_DOWN_TO(surface->u.gfx9.prt_level_offset[level],
         uint64_t commit_base = level_base +
            uint64_t size = (uint64_t)w * RADEON_SPARSE_PAGE_SIZE;
   for (int i = 0; i < d; i++) {
      uint64_t base = commit_base + i * depth_pitch;
   for (int j = 0; j < h; j++) {
      uint64_t offset = base + j * row_pitch;
   if (!ctx->ws->buffer_commit(ctx->ws, res->buf, offset, size, commit))
                     }
      static void si_query_dmabuf_modifiers(struct pipe_screen *screen,
                                 {
               unsigned ac_mod_count = max;
   ac_get_supported_modifiers(&sscreen->info, &(struct ac_modifier_options) {
         .dcc = !(sscreen->debug_flags & DBG(NO_DCC)),
   /* Do not support DCC with retiling yet. This needs explicit
   * resource flushes, but the app has no way to promise doing
   * flushes with modifiers. */
         if (max && external_only) {
      for (unsigned i = 0; i < ac_mod_count; ++i)
      }
      }
      static bool
   si_is_dmabuf_modifier_supported(struct pipe_screen *screen,
                     {
      int allowed_mod_count;
            uint64_t *allowed_modifiers = (uint64_t *)calloc(allowed_mod_count, sizeof(uint64_t));
   if (!allowed_modifiers)
            unsigned *external_array = NULL;
   if (external_only) {
      external_array = (unsigned *)calloc(allowed_mod_count, sizeof(unsigned));
   if (!external_array) {
      free(allowed_modifiers);
                  si_query_dmabuf_modifiers(screen, format, allowed_mod_count, allowed_modifiers,
            bool supported = false;
   for (int i = 0; i < allowed_mod_count && !supported; ++i) {
      if (allowed_modifiers[i] != modifier)
            supported = true;
   if (external_only)
               free(allowed_modifiers);
   free(external_array);
      }
      static unsigned
   si_get_dmabuf_modifier_planes(struct pipe_screen *pscreen, uint64_t modifier,
         {
               if (IS_AMD_FMT_MOD(modifier) && planes == 1) {
      if (AMD_FMT_MOD_GET(DCC_RETILE, modifier))
         else if (AMD_FMT_MOD_GET(DCC, modifier))
         else
                  }
      static bool
   si_modifier_supports_resource(struct pipe_screen *screen,
               {
      struct si_screen *sscreen = (struct si_screen *)screen;
            ac_modifier_max_extent(&sscreen->info, modifier, &max_width, &max_height);
      }
      static struct pipe_resource *
   si_texture_create_with_modifiers(struct pipe_screen *screen,
                     {
      /* Buffers with modifiers make zero sense. */
            /* Select modifier. */
   int allowed_mod_count;
            uint64_t *allowed_modifiers = (uint64_t *)calloc(allowed_mod_count, sizeof(uint64_t));
   if (!allowed_modifiers) {
                  /* This does not take external_only into account. We assume it is the same for all modifiers. */
                     /* Try to find the first allowed modifier that is in the application provided
   * list. We assume that the allowed modifiers are ordered in descending
   * preference in the list provided by si_query_dmabuf_modifiers. */
   for (int i = 0; i < allowed_mod_count; ++i) {
      bool found = false;
   for (int j = 0; j < modifier_count && !found; ++j)
                  if (found) {
      modifier = allowed_modifiers[i];
                           if (modifier == DRM_FORMAT_MOD_INVALID) {
         }
      }
      static bool si_texture_is_aux_plane(const struct pipe_resource *resource)
   {
         }
      static struct pipe_resource *si_texture_from_winsys_buffer(struct si_screen *sscreen,
                           {
      struct radeon_surf surface = {};
   struct radeon_bo_metadata metadata = {};
   struct si_texture *tex;
            /* Ignore metadata for non-zero planes. */
   if (offset != 0)
            if (dedicated) {
         } else {
      /**
   * The bo metadata is unset for un-dedicated images. So we fall
   * back to linear. See answer to question 5 of the
   * VK_KHX_external_memory spec for some details.
   *
   * It is possible that this case isn't going to work if the
   * surface pitch isn't correctly aligned by default.
   *
   * In order to support it correctly we require multi-image
   * metadata to be synchronized between radv and radeonsi. The
   * semantics of associating multiple image metadata to a memory
   * object on the vulkan export side are not concretely defined
   * either.
   *
   * All the use cases we are aware of at the moment for memory
   * objects use dedicated allocations. So lets keep the initial
   * implementation simple.
   *
   * A possible alternative is to attempt to reconstruct the
   * tiling information when the TexParameter TEXTURE_TILING_EXT
   * is set.
   */
               r = si_init_surface(sscreen, &surface, templ, metadata.mode, modifier, true,
         if (r)
            tex = si_texture_create_object(&sscreen->b, templ, &surface, NULL, buf,
         if (!tex)
            tex->buffer.b.is_shared = true;
   tex->buffer.external_usage = usage;
   tex->num_planes = 1;
   if (tex->buffer.flags & RADEON_FLAG_ENCRYPTED)
            /* Account for multiple planes with lowered yuv import. */
   struct pipe_resource *next_plane = tex->buffer.b.b.next;
   while (next_plane && !si_texture_is_aux_plane(next_plane)) {
      struct si_texture *next_tex = (struct si_texture *)next_plane;
   ++next_tex->num_planes;
   ++tex->num_planes;
               unsigned nplanes = ac_surface_get_nplanes(&tex->surface);
   unsigned plane = 1;
   while (next_plane) {
      struct si_auxiliary_texture *ptex = (struct si_auxiliary_texture *)next_plane;
   if (plane >= nplanes || ptex->buffer != tex->buffer.buf ||
      ptex->offset != ac_surface_get_plane_offset(sscreen->info.gfx_level,
         ptex->stride != ac_surface_get_plane_stride(sscreen->info.gfx_level,
         si_texture_reference(&tex, NULL);
      }
   ++plane;
               if (plane != nplanes && tex->num_planes == 1) {
      si_texture_reference(&tex, NULL);
               if (!ac_surface_apply_umd_metadata(&sscreen->info, &tex->surface,
                              si_texture_reference(&tex, NULL);
               if (ac_surface_get_plane_offset(sscreen->info.gfx_level, &tex->surface, 0, 0) +
      tex->surface.total_size > buf->size) {
   si_texture_reference(&tex, NULL);
               /* Displayable DCC requires an explicit flush. */
   if (dedicated && offset == 0 && !(usage & PIPE_HANDLE_USAGE_EXPLICIT_FLUSH) &&
      si_displayable_dcc_needs_explicit_flush(tex)) {
   /* TODO: do we need to decompress DCC? */
   if (si_texture_discard_dcc(sscreen, tex)) {
      /* Update BO metadata after disabling DCC. */
                  assert(tex->surface.tile_swizzle == 0);
      }
      static struct pipe_resource *si_texture_from_handle(struct pipe_screen *screen,
               {
      struct si_screen *sscreen = (struct si_screen *)screen;
            /* Support only 2D textures without mipmaps */
   if ((templ->target != PIPE_TEXTURE_2D && templ->target != PIPE_TEXTURE_RECT &&
      templ->target != PIPE_TEXTURE_2D_ARRAY) ||
   templ->last_level != 0)
         buf = sscreen->ws->buffer_from_handle(sscreen->ws, whandle,
               if (!buf)
            if (whandle->plane >= util_format_get_num_planes(whandle->format)) {
      struct si_auxiliary_texture *tex = CALLOC_STRUCT_CL(si_auxiliary_texture);
   if (!tex)
         tex->b.b = *templ;
   tex->b.b.flags |= SI_RESOURCE_AUX_PLANE;
   tex->stride = whandle->stride;
   tex->offset = whandle->offset;
   tex->buffer = buf;
   pipe_reference_init(&tex->b.b.reference, 1);
                        return si_texture_from_winsys_buffer(sscreen, templ, buf, whandle->stride, whandle->offset,
      }
      bool si_init_flushed_depth_texture(struct pipe_context *ctx, struct pipe_resource *texture)
   {
      struct si_texture *tex = (struct si_texture *)texture;
   struct pipe_resource resource;
                     if (!tex->can_sample_z && tex->can_sample_s) {
      switch (pipe_format) {
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      /* Save memory by not allocating the S plane. */
   pipe_format = PIPE_FORMAT_Z32_FLOAT;
      case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      /* Save memory bandwidth by not copying the
   * stencil part during flush.
   *
   * This potentially increases memory bandwidth
   * if an application uses both Z and S texturing
   * simultaneously (a flushed Z24S8 texture
   * would be stored compactly), but how often
   * does that really happen?
   */
   pipe_format = PIPE_FORMAT_Z24X8_UNORM;
      default:;
      } else if (!tex->can_sample_s && tex->can_sample_z) {
               /* DB->CB copies to an 8bpp surface don't work. */
               memset(&resource, 0, sizeof(resource));
   resource.target = texture->target;
   resource.format = pipe_format;
   resource.width0 = texture->width0;
   resource.height0 = texture->height0;
   resource.depth0 = texture->depth0;
   resource.array_size = texture->array_size;
   resource.last_level = texture->last_level;
   resource.nr_samples = texture->nr_samples;
   resource.nr_storage_samples = texture->nr_storage_samples;
   resource.usage = PIPE_USAGE_DEFAULT;
   resource.bind = texture->bind & ~PIPE_BIND_DEPTH_STENCIL;
            tex->flushed_depth_texture =
         if (!tex->flushed_depth_texture) {
      PRINT_ERR("failed to create temporary texture to hold flushed depth\n");
      }
      }
      /**
   * Initialize the pipe_resource descriptor to be of the same size as the box,
   * which is supposed to hold a subregion of the texture "orig" at the given
   * mipmap level.
   */
   static void si_init_temp_resource_from_box(struct pipe_resource *res, struct pipe_resource *orig,
               {
      struct si_texture *tex = (struct si_texture *)orig;
   enum pipe_format orig_format = tex->multi_plane_format != PIPE_FORMAT_NONE ?
            memset(res, 0, sizeof(*res));
   res->format = orig_format;
   res->width0 = box->width;
   res->height0 = box->height;
   res->depth0 = 1;
   res->array_size = 1;
   res->usage = usage;
            if (flags & SI_RESOURCE_FLAG_FORCE_LINEAR && util_format_is_compressed(orig_format)) {
      /* Transfer resources are allocated with linear tiling, which is
   * not supported for compressed formats.
   */
            if (blocksize == 8) {
         } else {
      assert(blocksize == 16);
               res->width0 = util_format_get_nblocksx(orig_format, box->width);
               /* We must set the correct texture target and dimensions for a 3D box. */
   if (box->depth > 1 && util_max_layer(orig, level) > 0) {
      res->target = PIPE_TEXTURE_2D_ARRAY;
      } else {
            }
      static bool si_can_invalidate_texture(struct si_screen *sscreen, struct si_texture *tex,
         {
      return !tex->buffer.b.is_shared && !(tex->surface.flags & RADEON_SURF_IMPORTED) &&
         !(transfer_usage & PIPE_MAP_READ) && tex->buffer.b.b.last_level == 0 &&
      }
      static void si_texture_invalidate_storage(struct si_context *sctx, struct si_texture *tex)
   {
               /* There is no point in discarding depth and tiled buffers. */
   assert(!tex->is_depth);
            /* Reallocate the buffer in the same pipe_resource. */
            /* Initialize the CMASK base address (needed even without CMASK). */
                        }
      static void *si_texture_transfer_map(struct pipe_context *ctx, struct pipe_resource *texture,
               {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_texture *tex = (struct si_texture *)texture;
   struct si_transfer *trans;
   struct si_resource *buf;
   uint64_t offset = 0;
   char *map;
   bool use_staging_texture = tex->buffer.flags & RADEON_FLAG_ENCRYPTED;
            assert(texture->target != PIPE_BUFFER);
   assert(!(texture->flags & SI_RESOURCE_FLAG_FORCE_LINEAR));
            if (tex->buffer.b.b.flags & SI_RESOURCE_AUX_PLANE)
            if ((tex->buffer.flags & RADEON_FLAG_ENCRYPTED) && usage & PIPE_MAP_READ)
            if (tex->is_depth || tex->buffer.flags & RADEON_FLAG_SPARSE) {
      /* Depth and sparse textures use staging unconditionally. */
      } else {
      /* Degrade the tile mode if we get too many transfers on APUs.
   * On dGPUs, the staging texture is always faster.
   * Only count uploads that are at least 4x4 pixels large.
   */
   if (!sctx->screen->info.has_dedicated_vram && real_level == 0 && box->width >= 4 &&
                                 /* Tiled textures need to be converted into a linear texture for CPU
   * access. The staging texture is always linear and is placed in GART.
   *
   * dGPU use a staging texture for VRAM, so that we don't map it and
   * don't relocate it to GTT.
   *
   * Reading from VRAM or GTT WC is slow, always use the staging
   * texture in this case.
   *
   * Use the staging texture for uploads if the underlying BO
   * is busy.
   */
   if (!tex->surface.is_linear || (tex->buffer.flags & RADEON_FLAG_ENCRYPTED) ||
      (tex->buffer.domains & RADEON_DOMAIN_VRAM && sctx->screen->info.has_dedicated_vram))
      else if (usage & PIPE_MAP_READ)
      use_staging_texture =
      /* Write & linear only: */
   else if (si_cs_is_buffer_referenced(sctx, tex->buffer.buf, RADEON_USAGE_READWRITE) ||
            /* It's busy. */
   if (si_can_invalidate_texture(sctx->screen, tex, usage, box))
         else
                  trans = CALLOC_STRUCT(si_transfer);
   if (!trans)
         pipe_resource_reference(&trans->b.b.resource, texture);
   trans->b.b.level = level;
   trans->b.b.usage = usage;
            if (use_staging_texture) {
      struct pipe_resource resource;
   struct si_texture *staging;
   unsigned bo_usage = usage & PIPE_MAP_READ ? PIPE_USAGE_STAGING : PIPE_USAGE_STREAM;
            si_init_temp_resource_from_box(&resource, texture, box, real_level, bo_usage,
            /* Since depth-stencil textures don't support linear tiling,
   * blit from ZS to color and vice versa. u_blitter will do
   * the packing for these formats.
   */
   if (tex->is_depth)
            /* Create the temporary texture. */
   staging = (struct si_texture *)ctx->screen->resource_create(ctx->screen, &resource);
   if (!staging) {
      PRINT_ERR("failed to create temporary texture to hold untiled copy\n");
      }
            /* Just get the strides. */
   si_texture_get_offset(sctx->screen, staging, 0, NULL, &trans->b.b.stride,
            if (usage & PIPE_MAP_READ)
         else
               } else {
      /* the resource is mapped directly */
   offset = si_texture_get_offset(sctx->screen, tex, real_level, box, &trans->b.b.stride,
                     /* Always unmap texture CPU mappings on 32-bit architectures, so that
   * we don't run out of the CPU address space.
   */
   if (sizeof(void *) == 4)
            if (!(map = si_buffer_map(sctx, buf, usage)))
            *ptransfer = &trans->b.b;
         fail_trans:
      si_resource_reference(&trans->staging, NULL);
   pipe_resource_reference(&trans->b.b.resource, NULL);
   FREE(trans);
      }
      static void si_texture_transfer_unmap(struct pipe_context *ctx, struct pipe_transfer *transfer)
   {
      struct si_context *sctx = (struct si_context *)ctx;
   struct si_transfer *stransfer = (struct si_transfer *)transfer;
   struct pipe_resource *texture = transfer->resource;
            /* Always unmap texture CPU mappings on 32-bit architectures, so that
   * we don't run out of the CPU address space.
   */
   if (sizeof(void *) == 4) {
                           if ((transfer->usage & PIPE_MAP_WRITE) && stransfer->staging)
            if (stransfer->staging) {
      sctx->num_alloc_tex_transfer_bytes += stransfer->staging->buf->size;
               /* Heuristic for {upload, draw, upload, draw, ..}:
   *
   * Flush the gfx IB if we've allocated too much texture storage.
   *
   * The idea is that we don't want to build IBs that use too much
   * memory and put pressure on the kernel memory manager and we also
   * want to make temporary and invalidated buffers go idle ASAP to
   * decrease the total memory usage or make them reusable. The memory
   * usage will be slightly higher than given here because of the buffer
   * cache in the winsys.
   *
   * The result is that the kernel memory manager is never a bottleneck.
   */
   if (sctx->num_alloc_tex_transfer_bytes > (uint64_t)sctx->screen->info.gart_size_kb * 1024 / 4) {
      si_flush_gfx_cs(sctx, RADEON_FLUSH_ASYNC_START_NEXT_GFX_IB_NOW, NULL);
               pipe_resource_reference(&transfer->resource, NULL);
      }
      /* Return if it's allowed to reinterpret one format as another with DCC enabled.
   */
   bool vi_dcc_formats_compatible(struct si_screen *sscreen, enum pipe_format format1,
         {
               /* All formats are compatible on GFX11. */
   if (sscreen->info.gfx_level >= GFX11)
            /* No format change - exit early. */
   if (format1 == format2)
            format1 = si_simplify_cb_format(format1);
            /* Check again after format adjustments. */
   if (format1 == format2)
            desc1 = util_format_description(format1);
            if (desc1->layout != UTIL_FORMAT_LAYOUT_PLAIN || desc2->layout != UTIL_FORMAT_LAYOUT_PLAIN)
            /* Float and non-float are totally incompatible. */
   if ((desc1->channel[0].type == UTIL_FORMAT_TYPE_FLOAT) !=
      (desc2->channel[0].type == UTIL_FORMAT_TYPE_FLOAT))
         /* Channel sizes must match across DCC formats.
   * Comparing just the first 2 channels should be enough.
   */
   if (desc1->channel[0].size != desc2->channel[0].size ||
      (desc1->nr_channels >= 2 && desc1->channel[1].size != desc2->channel[1].size))
         /* Everything below is not needed if the driver never uses the DCC
   * clear code with the value of 1.
            /* If the clear values are all 1 or all 0, this constraint can be
   * ignored. */
   if (vi_alpha_is_on_msb(sscreen, format1) != vi_alpha_is_on_msb(sscreen, format2))
            /* Channel types must match if the clear value of 1 is used.
   * The type categories are only float, signed, unsigned.
   * NORM and INT are always compatible.
   */
   if (desc1->channel[0].type != desc2->channel[0].type ||
      (desc1->nr_channels >= 2 && desc1->channel[1].type != desc2->channel[1].type))
            }
      bool vi_dcc_formats_are_incompatible(struct pipe_resource *tex, unsigned level,
         {
               return vi_dcc_enabled(stex, level) &&
      }
      /* This can't be merged with the above function, because
   * vi_dcc_formats_compatible should be called only when DCC is enabled. */
   void vi_disable_dcc_if_incompatible_format(struct si_context *sctx, struct pipe_resource *tex,
         {
               if (vi_dcc_formats_are_incompatible(tex, level, view_format))
      if (!si_texture_disable_dcc(sctx, stex))
   }
      static struct pipe_surface *si_create_surface(struct pipe_context *pipe, struct pipe_resource *tex,
         {
      unsigned level = templ->u.tex.level;
   unsigned width = u_minify(tex->width0, level);
   unsigned height = u_minify(tex->height0, level);
   unsigned width0 = tex->width0;
            if (tex->target != PIPE_BUFFER && templ->format != tex->format) {
      const struct util_format_description *tex_desc = util_format_description(tex->format);
                     /* Adjust size of surface if and only if the block width or
   * height is changed. */
   if (tex_desc->block.width != templ_desc->block.width ||
      tex_desc->block.height != templ_desc->block.height) {
                                 width0 = util_format_get_nblocksx(tex->format, width0);
                           if (!surface)
            assert(templ->u.tex.first_layer <= util_max_layer(tex, templ->u.tex.level));
            pipe_reference_init(&surface->base.reference, 1);
   pipe_resource_reference(&surface->base.texture, tex);
   surface->base.context = pipe;
   surface->base.format = templ->format;
   surface->base.width = width;
   surface->base.height = height;
            surface->width0 = width0;
            surface->dcc_incompatible =
      tex->target != PIPE_BUFFER &&
         }
      static void si_surface_destroy(struct pipe_context *pipe, struct pipe_surface *surface)
   {
      pipe_resource_reference(&surface->texture, NULL);
      }
      unsigned si_translate_colorswap(enum amd_gfx_level gfx_level, enum pipe_format format,
         {
            #define HAS_SWIZZLE(chan, swz) (desc->swizzle[chan] == PIPE_SWIZZLE_##swz)
         if (format == PIPE_FORMAT_R11G11B10_FLOAT) /* isn't plain */
            if (gfx_level >= GFX10_3 &&
      format == PIPE_FORMAT_R9G9B9E5_FLOAT) /* isn't plain */
         if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN)
            switch (desc->nr_channels) {
   case 1:
      if (HAS_SWIZZLE(0, X))
         else if (HAS_SWIZZLE(3, X))
            case 2:
      if ((HAS_SWIZZLE(0, X) && HAS_SWIZZLE(1, Y)) || (HAS_SWIZZLE(0, X) && HAS_SWIZZLE(1, NONE)) ||
      (HAS_SWIZZLE(0, NONE) && HAS_SWIZZLE(1, Y)))
      else if ((HAS_SWIZZLE(0, Y) && HAS_SWIZZLE(1, X)) ||
            (HAS_SWIZZLE(0, Y) && HAS_SWIZZLE(1, NONE)) ||
   /* YX__ */
      else if (HAS_SWIZZLE(0, X) && HAS_SWIZZLE(3, Y))
         else if (HAS_SWIZZLE(0, Y) && HAS_SWIZZLE(3, X))
            case 3:
      if (HAS_SWIZZLE(0, X))
         else if (HAS_SWIZZLE(0, Z))
            case 4:
      /* check the middle channels, the 1st and 4th channel can be NONE */
   if (HAS_SWIZZLE(1, Y) && HAS_SWIZZLE(2, Z)) {
         } else if (HAS_SWIZZLE(1, Z) && HAS_SWIZZLE(2, Y)) {
         } else if (HAS_SWIZZLE(1, Y) && HAS_SWIZZLE(2, X)) {
         } else if (HAS_SWIZZLE(1, Z) && HAS_SWIZZLE(2, W)) {
      /* YZWX */
   if (desc->is_array)
         else
      }
      }
      }
      static struct pipe_memory_object *
   si_memobj_from_handle(struct pipe_screen *screen, struct winsys_handle *whandle, bool dedicated)
   {
      struct si_screen *sscreen = (struct si_screen *)screen;
   struct si_memory_object *memobj = CALLOC_STRUCT(si_memory_object);
            if (!memobj)
            buf = sscreen->ws->buffer_from_handle(sscreen->ws, whandle, sscreen->info.max_alignment, false);
   if (!buf) {
      free(memobj);
               memobj->b.dedicated = dedicated;
   memobj->buf = buf;
               }
      static void si_memobj_destroy(struct pipe_screen *screen, struct pipe_memory_object *_memobj)
   {
               radeon_bo_reference(((struct si_screen*)screen)->ws, &memobj->buf, NULL);
      }
      static struct pipe_resource *si_resource_from_memobj(struct pipe_screen *screen,
                     {
      struct si_screen *sscreen = (struct si_screen *)screen;
   struct si_memory_object *memobj = (struct si_memory_object *)_memobj;
            if (templ->target == PIPE_BUFFER)
         else
      res = si_texture_from_winsys_buffer(sscreen, templ, memobj->buf,
                           if (!res)
            /* si_texture_from_winsys_buffer doesn't increment refcount of
   * memobj->buf, so increment it here.
   */
   struct pb_buffer *buf = NULL;
   radeon_bo_reference(sscreen->ws, &buf, memobj->buf);
      }
      static bool si_check_resource_capability(struct pipe_screen *screen, struct pipe_resource *resource,
         {
               /* Buffers only support the linear flag. */
   if (resource->target == PIPE_BUFFER)
            if (bind & PIPE_BIND_LINEAR && !tex->surface.is_linear)
            if (bind & PIPE_BIND_SCANOUT && !tex->surface.is_displayable)
            /* TODO: PIPE_BIND_CURSOR - do we care? */
      }
      static int si_get_sparse_texture_virtual_page_size(struct pipe_screen *screen,
                                 {
               /* Only support one type of page size. */
   if (offset != 0)
            static const int page_size_2d[][3] = {
      { 256, 256, 1 }, /* 8bpp   */
   { 256, 128, 1 }, /* 16bpp  */
   { 128, 128, 1 }, /* 32bpp  */
   { 128, 64,  1 }, /* 64bpp  */
      };
   static const int page_size_3d[][3] = {
      { 64,  32,  32 }, /* 8bpp   */
   { 32,  32,  32 }, /* 16bpp  */
   { 32,  32,  16 }, /* 32bpp  */
   { 32,  16,  16 }, /* 64bpp  */
                        /* Supported targets. */
   switch (target) {
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
      page_sizes = page_size_2d;
      case PIPE_TEXTURE_3D:
      page_sizes = page_size_3d;
      default:
                  /* ARB_sparse_texture2 need to query supported virtual page x/y/z without
   * knowing the actual sample count. So we need to return a fixed virtual page
   * x/y/z for all sample count which means the virtual page size can not be fixed
   * to 64KB.
   *
   * Only enabled for GFX9. GFX10+ removed MS texture support. By specification
   * ARB_sparse_texture2 need MS texture support, but we relax it by just return
   * no page size for GFX10+ to keep shader query capbility.
   */
   if (multi_sample && sscreen->info.gfx_level != GFX9)
            /* Unsupported formats. */
   /* TODO: support these formats. */
   if (util_format_is_depth_or_stencil(format) ||
      util_format_get_num_planes(format) > 1 ||
   util_format_is_compressed(format))
         int blk_size = util_format_get_blocksize(format);
   /* We don't support any non-power-of-two bpp formats, so
   * pipe_screen->is_format_supported() should already filter out these formats.
   */
            if (size) {
      unsigned index = util_logbase2(blk_size);
   if (x) *x = page_sizes[index][0];
   if (y) *y = page_sizes[index][1];
                  }
      void si_init_screen_texture_functions(struct si_screen *sscreen)
   {
      sscreen->b.resource_from_handle = si_texture_from_handle;
   sscreen->b.resource_get_handle = si_texture_get_handle;
   sscreen->b.resource_get_param = si_resource_get_param;
   sscreen->b.resource_get_info = si_texture_get_info;
   sscreen->b.resource_from_memobj = si_resource_from_memobj;
   sscreen->b.memobj_create_from_handle = si_memobj_from_handle;
   sscreen->b.memobj_destroy = si_memobj_destroy;
   sscreen->b.check_resource_capability = si_check_resource_capability;
   sscreen->b.get_sparse_texture_virtual_page_size =
            /* By not setting it the frontend will fall back to non-modifier create,
   * which works around some applications using modifiers that are not
   * allowed in combination with lack of error reporting in
   * gbm_dri_surface_create */
   if (sscreen->info.gfx_level >= GFX9 && sscreen->info.kernel_has_modifiers) {
      sscreen->b.resource_create_with_modifiers = si_texture_create_with_modifiers;
   sscreen->b.query_dmabuf_modifiers = si_query_dmabuf_modifiers;
   sscreen->b.is_dmabuf_modifier_supported = si_is_dmabuf_modifier_supported;
         }
      void si_init_context_texture_functions(struct si_context *sctx)
   {
      sctx->b.texture_map = si_texture_transfer_map;
   sctx->b.texture_unmap = si_texture_transfer_unmap;
   sctx->b.create_surface = si_create_surface;
      }
