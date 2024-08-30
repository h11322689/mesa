   /*
   * Copyright (c) 2012-2015 Etnaviv Project
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
   * Authors:
   *    Wladimir J. van der Laan <laanwj@gmail.com>
   */
      #include "etnaviv_resource.h"
      #include "hw/common.xml.h"
      #include "etnaviv_context.h"
   #include "etnaviv_debug.h"
   #include "etnaviv_screen.h"
   #include "etnaviv_translate.h"
      #include "util/hash_table.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
      static enum etna_surface_layout modifier_to_layout(uint64_t modifier)
   {
      switch (modifier & ~VIVANTE_MOD_EXT_MASK) {
   case DRM_FORMAT_MOD_VIVANTE_TILED:
         case DRM_FORMAT_MOD_VIVANTE_SUPER_TILED:
         case DRM_FORMAT_MOD_VIVANTE_SPLIT_TILED:
         case DRM_FORMAT_MOD_VIVANTE_SPLIT_SUPER_TILED:
         case DRM_FORMAT_MOD_LINEAR:
         default:
            }
      static uint64_t layout_to_modifier(enum etna_surface_layout layout)
   {
      switch (layout) {
   case ETNA_LAYOUT_TILED:
         case ETNA_LAYOUT_SUPER_TILED:
         case ETNA_LAYOUT_MULTI_TILED:
         case ETNA_LAYOUT_MULTI_SUPERTILED:
         case ETNA_LAYOUT_LINEAR:
         default:
            }
      static uint64_t etna_resource_modifier(struct etna_resource *rsc)
   {
      if (etna_resource_ext_ts(rsc))
               }
      /* A tile is either 64 bytes or, when the GPU has the CACHE128B256BPERLINE
   * feature, 128/256 bytes of color/depth data, tracked by
   * 'screen->specs.bits_per_tile' bits of tile status.
   */
   bool
   etna_screen_resource_alloc_ts(struct pipe_screen *pscreen,
         {
      struct etna_screen *screen = etna_screen(pscreen);
   struct etna_resource_level *lvl = &rsc->levels[0];
   struct pipe_resource *prsc = &rsc->base;
   size_t tile_size, ts_size, ts_bo_size, ts_layer_stride, ts_data_offset = 0;
   uint8_t ts_mode = TS_MODE_128B;
   int8_t ts_compress_fmt = -1;
                     /* pre-v4 compression is largely useless, so disable it when not wanted for MSAA
   * v4 compression can be enabled everywhere without any known drawback,
   * except that in-place resolve must go through a slower path
   */
   if ((screen->specs.v4_compression &&
      (!modifier || (modifier & VIVANTE_MOD_COMP_DEC400))) ||
   (!modifier && prsc->nr_samples > 1))
         /* enable 256B ts mode with compression, as it improves performance
   * the size of the resource might also determine if we want to use it or not
   */
   if (VIV_FEATURE(screen, chipMinorFeatures6, CACHE128B256BPERLINE)) {
      if ((modifier & VIVANTE_MOD_TS_MASK) == VIVANTE_MOD_TS_128_4)
         else if ((modifier & VIVANTE_MOD_TS_MASK) == VIVANTE_MOD_TS_256_4)
         else {
      /* Without a TS modifier TS is only internal, so we can choose the
   * mode to use freely. Enable 256B ts mode with compression, as it
   * improves performance. The size of the resource might also determine
   * if we want to use it or not.
   */
   if (ts_compress_fmt >= 0 &&
      (rsc->layout != ETNA_LAYOUT_LINEAR || lvl->stride % 256 == 0))
      else
                  tile_size = etna_screen_get_tile_size(screen, ts_mode, prsc->nr_samples > 1);
   layers = prsc->target == PIPE_TEXTURE_3D ? prsc->depth0 : prsc->array_size;
   ts_layer_stride = align(DIV_ROUND_UP(lvl->layer_stride,
               ts_size = ts_bo_size = ts_layer_stride * layers;
   if (ts_size == 0)
            /* add space for the software meta */
   if (modifier & VIVANTE_MOD_TS_MASK) {
      /* The offset is a educated guess for a safe value based on the experience
   * that various object pointers on the GPU need to be 64B aligned. Maybe
   * some GPU needs even more alignment, or we could drop this to 32B. 64B
   * has worked well across various GPU generations so far.
   */
   ts_data_offset = 64;
   assert(ts_data_offset >= sizeof(struct etna_ts_sw_meta));
               DBG_F(ETNA_DBG_RESOURCE_MSGS, "%p: Allocating tile status of size %zu",
            if ((rsc->base.bind & PIPE_BIND_SCANOUT) && screen->ro) {
      struct pipe_resource scanout_templat;
            scanout_templat.format = PIPE_FORMAT_R8_UNORM;
   scanout_templat.width0 = align(ts_bo_size, 4096);
            rsc->ts_scanout = renderonly_scanout_for_resource(&scanout_templat,
         if (!rsc->ts_scanout) {
      BUG("Problem allocating kms memory for TS resource");
               assert(handle.type == WINSYS_HANDLE_TYPE_FD);
   rsc->ts_bo = etna_screen_bo_from_handle(pscreen, &handle);
      } else {
                  if (unlikely(!rsc->ts_bo)) {
      BUG("Problem allocating tile status for resource");
               lvl->ts_offset = ts_data_offset;
   lvl->ts_layer_stride = ts_layer_stride;
   lvl->ts_size = ts_size;
   lvl->ts_mode = ts_mode;
            /* fill software meta */
   if (modifier & VIVANTE_MOD_TS_MASK) {
      lvl->ts_meta = etna_bo_map(rsc->ts_bo);
   memset(lvl->ts_meta, 0, sizeof(struct etna_ts_sw_meta));
   lvl->ts_meta->version = 0;
   lvl->ts_meta->v0.data_size = ts_size;
   lvl->ts_meta->v0.data_offset = ts_data_offset;
   lvl->ts_meta->v0.layer_stride = ts_layer_stride;
      }
      }
      static bool
   etna_screen_can_create_resource(struct pipe_screen *pscreen,
         {
      struct etna_screen *screen = etna_screen(pscreen);
   if (!translate_samples_to_xyscale(templat->nr_samples, NULL, NULL))
            /* templat->bind is not set here, so we must use the minimum sizes */
   uint max_size =
            if (templat->width0 > max_size || templat->height0 > max_size)
               }
      static unsigned
   setup_miptree(struct etna_resource *rsc, unsigned paddingX, unsigned paddingY,
         {
      struct pipe_resource *prsc = &rsc->base;
   unsigned level, size = 0;
   unsigned width = prsc->width0;
   unsigned height = prsc->height0;
            for (level = 0; level <= prsc->last_level; level++) {
               mip->width = width;
   mip->height = height;
   mip->depth = depth;
   mip->padded_width = align(width * msaa_xscale, paddingX);
   mip->padded_height = align(height * msaa_yscale, paddingY);
   mip->stride = util_format_get_stride(prsc->format, mip->padded_width);
   mip->offset = size;
   mip->layer_stride = mip->stride * util_format_get_nblocksy(prsc->format, mip->padded_height);
            /* align levels to 64 bytes to be able to render to them */
            width = u_minify(width, 1);
   height = u_minify(height, 1);
                  }
      /* Compute the slice/miplevel alignment (in pixels) and the texture sampler
   * HALIGN parameter from the resource parameters and the target layout.
   */
   static void
   etna_layout_multiple(const struct etna_screen *screen,
               {
      const struct etna_specs *specs = &screen->specs;
   /* If we have the TEXTURE_HALIGN feature, we can always align to the resolve
   * engine's width.  If not, we must not align resources used only for
   * textures. If this GPU uses the BLT engine, never do RS align.
   */
   bool rs_align = !specs->use_blt && (!etna_resource_sampler_only(templat) ||
                  /* Compressed textures are padded to their block size, but we don't have
   * to do anything special for that.
   */
   if (unlikely(util_format_is_compressed(templat->format))) {
      assert(layout == ETNA_LAYOUT_LINEAR);
   *paddingX = 1;
   *paddingY = 1;
   *halign = TEXTURE_HALIGN_FOUR;
                        switch (layout) {
   case ETNA_LAYOUT_LINEAR:
      *paddingX = rs_align ? 16 : 4;
   *paddingY = !specs->use_blt && templat->target != PIPE_BUFFER ? 4 : 1;
   *halign = rs_align ? TEXTURE_HALIGN_SIXTEEN : TEXTURE_HALIGN_FOUR;
      case ETNA_LAYOUT_TILED:
      *paddingX = rs_align ? 16 * msaa_xscale : 4;
   *paddingY = 4 * msaa_yscale;
   *halign = rs_align ? TEXTURE_HALIGN_SIXTEEN : TEXTURE_HALIGN_FOUR;
      case ETNA_LAYOUT_SUPER_TILED:
      *paddingX = 64;
   *paddingY = 64;
   *halign = TEXTURE_HALIGN_SUPER_TILED;
      case ETNA_LAYOUT_MULTI_TILED:
      *paddingX = 16 * msaa_xscale;
   *paddingY = 4 * msaa_yscale * specs->pixel_pipes;
   *halign = TEXTURE_HALIGN_SPLIT_TILED;
      case ETNA_LAYOUT_MULTI_SUPERTILED:
      *paddingX = 64;
   *paddingY = 64 * specs->pixel_pipes;
   *halign = TEXTURE_HALIGN_SPLIT_SUPER_TILED;
      default:
            }
      /* Create a new resource object, using the given template info */
   struct pipe_resource *
   etna_resource_alloc(struct pipe_screen *pscreen, unsigned layout,
         {
      struct etna_screen *screen = etna_screen(pscreen);
   struct etna_resource *rsc;
            DBG_F(ETNA_DBG_RESOURCE_MSGS,
         "target=%d, format=%s, %ux%ux%u, array_size=%u, "
   "last_level=%u, nr_samples=%u, usage=%u, bind=%x, flags=%x",
   templat->target, util_format_name(templat->format), templat->width0,
   templat->height0, templat->depth0, templat->array_size,
            /* Determine scaling for antialiasing */
   int msaa_xscale = 1, msaa_yscale = 1;
   if (!translate_samples_to_xyscale(templat->nr_samples, &msaa_xscale, &msaa_yscale)) {
      /* Number of samples not supported */
               /* Determine needed padding (alignment of height/width) */
   unsigned paddingX, paddingY, halign;
            rsc = CALLOC_STRUCT(etna_resource);
   if (!rsc)
            rsc->base = *templat;
   rsc->base.screen = pscreen;
   rsc->base.nr_samples = templat->nr_samples;
   rsc->layout = layout;
   rsc->modifier = modifier;
   rsc->halign = halign;
            pipe_reference_init(&rsc->base.reference, 1);
                     if (unlikely(templat->bind & PIPE_BIND_SCANOUT) && screen->ro) {
      struct pipe_resource scanout_templat = *templat;
            scanout_templat.width0 = align(scanout_templat.width0, paddingX);
            rsc->scanout = renderonly_scanout_for_resource(&scanout_templat,
         if (!rsc->scanout) {
      BUG("Problem allocating kms memory for resource");
               assert(handle.type == WINSYS_HANDLE_TYPE_FD);
   rsc->levels[0].stride = handle.stride;
   rsc->bo = etna_screen_bo_from_handle(pscreen, &handle);
   close(handle.handle);
   if (unlikely(!rsc->bo))
      } else {
               if (templat->bind & PIPE_BIND_VERTEX_BUFFER)
            rsc->bo = etna_bo_new(screen->dev, size, flags);
   if (unlikely(!rsc->bo)) {
      BUG("Problem allocating video memory for resource");
                  /* If TS is externally visible set it up now, so it can be exported before
   * the first rendering to a surface.
   */
   if (etna_resource_ext_ts(rsc))
            if (DBG_ENABLED(ETNA_DBG_ZERO)) {
      void *map = etna_bo_map(rsc->bo);
   etna_bo_cpu_prep(rsc->bo, DRM_ETNA_PREP_WRITE);
   memset(map, 0, size);
                     free_rsc:
      FREE(rsc);
      }
      static struct pipe_resource *
   etna_resource_create(struct pipe_screen *pscreen,
         {
      struct etna_screen *screen = etna_screen(pscreen);
            /* At this point we don't know if the resource will be used as a texture,
   * render target, or both, because gallium sets the bits whenever possible
   * This matters because on some GPUs (GC2000) there is no tiling that is
   * compatible with both TE and PE.
   *
   * We expect that depth/stencil buffers will always be used by PE (rendering),
   * and any other non-scanout resource will be used as a texture at some point,
   * So allocate a render-compatible base buffer for scanout/depthstencil buffers,
   * and a texture-compatible base buffer in other cases
   *
   */
   if (templat->bind & PIPE_BIND_DEPTH_STENCIL) {
      if (screen->specs.pixel_pipes > 1 && !screen->specs.single_buffer)
         if (screen->specs.can_supertile)
      } else if (screen->specs.can_supertile &&
            VIV_FEATURE(screen, chipMinorFeatures2, SUPERTILED_TEXTURE) &&
               if (/* MSAA render target */
      (templat->nr_samples > 1) &&
   (templat->bind & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_DEPTH_STENCIL))) {
   if (screen->specs.pixel_pipes > 1 && !screen->specs.single_buffer)
         if (screen->specs.can_supertile)
               if (/* linear base or scanout without modifier requested */
      (templat->bind & (PIPE_BIND_LINEAR | PIPE_BIND_SCANOUT)) ||
   templat->target == PIPE_BUFFER || /* buffer always linear */
   /* compressed textures don't use tiling, they have their own "tiles" */
   util_format_is_compressed(templat->format)) {
               /* modifier is only used for scanout surfaces, so safe to use LINEAR here */
      }
      enum modifier_priority {
      MODIFIER_PRIORITY_INVALID = 0,
   MODIFIER_PRIORITY_LINEAR,
   MODIFIER_PRIORITY_SPLIT_TILED,
   MODIFIER_PRIORITY_SPLIT_SUPER_TILED,
   MODIFIER_PRIORITY_TILED,
      };
      static const uint64_t priority_to_modifier[] = {
      [MODIFIER_PRIORITY_INVALID] = DRM_FORMAT_MOD_INVALID,
   [MODIFIER_PRIORITY_LINEAR] = DRM_FORMAT_MOD_LINEAR,
   [MODIFIER_PRIORITY_SPLIT_TILED] = DRM_FORMAT_MOD_VIVANTE_SPLIT_TILED,
   [MODIFIER_PRIORITY_SPLIT_SUPER_TILED] = DRM_FORMAT_MOD_VIVANTE_SPLIT_SUPER_TILED,
   [MODIFIER_PRIORITY_TILED] = DRM_FORMAT_MOD_VIVANTE_TILED,
      };
      static uint64_t
   select_best_modifier(const struct etna_screen * screen,
         {
      enum modifier_priority prio = MODIFIER_PRIORITY_INVALID;
            for (int i = 0; i < count; i++) {
      switch (modifiers[i] & ~VIVANTE_MOD_EXT_MASK) {
   case DRM_FORMAT_MOD_VIVANTE_SUPER_TILED:
      if ((screen->specs.pixel_pipes > 1 && !screen->specs.single_buffer) ||
      !screen->specs.can_supertile)
      prio = MAX2(prio, MODIFIER_PRIORITY_SUPER_TILED);
      case DRM_FORMAT_MOD_VIVANTE_TILED:
      if (screen->specs.pixel_pipes > 1 && !screen->specs.single_buffer)
         prio = MAX2(prio, MODIFIER_PRIORITY_TILED);
      case DRM_FORMAT_MOD_VIVANTE_SPLIT_SUPER_TILED:
      if ((screen->specs.pixel_pipes < 2) || !screen->specs.can_supertile)
         prio = MAX2(prio, MODIFIER_PRIORITY_SPLIT_SUPER_TILED);
      case DRM_FORMAT_MOD_VIVANTE_SPLIT_TILED:
      if (screen->specs.pixel_pipes < 2)
         prio = MAX2(prio, MODIFIER_PRIORITY_SPLIT_TILED);
      case DRM_FORMAT_MOD_LINEAR:
      prio = MAX2(prio, MODIFIER_PRIORITY_LINEAR);
      case DRM_FORMAT_MOD_INVALID:
   default:
                              if (!DBG_ENABLED(ETNA_DBG_SHARED_TS) ||
      !VIV_FEATURE(screen, chipFeatures, FAST_CLEAR))
         /* Make a second pass to try and find the best TS modifier if any. */
   for (int i = 0; i < count; i++) {
      if ((modifiers[i] & ~VIVANTE_MOD_EXT_MASK) == base_modifier)
      if ((modifiers[i] & VIVANTE_MOD_TS_MASK) >
                  /* If no modifier with TS was found, there is no point in looking further,
   * as compression depends on TS.
   */
   if (best_modifier == base_modifier)
            /* Make a third pass to find a modifier allowing compression. */
   base_modifier = best_modifier;
   for (int i = 0; i < count; i++) {
      if ((modifiers[i] & ~VIVANTE_MOD_COMP_MASK) == base_modifier)
      if ((modifiers[i] & VIVANTE_MOD_COMP_MASK) >
                     }
      static struct pipe_resource *
   etna_resource_create_modifiers(struct pipe_screen *pscreen,
               {
      struct etna_screen *screen = etna_screen(pscreen);
   struct pipe_resource tmpl = *templat;
            if (modifier == DRM_FORMAT_MOD_INVALID)
               }
      static void
   etna_resource_changed(struct pipe_screen *pscreen, struct pipe_resource *prsc)
   {
               for (int level = 0; level <= prsc->last_level; level++)
      }
      static void
   etna_resource_destroy(struct pipe_screen *pscreen, struct pipe_resource *prsc)
   {
               if (rsc->bo)
            if (rsc->ts_bo)
            if (rsc->scanout)
            if (rsc->ts_scanout)
                     pipe_resource_reference(&rsc->texture, NULL);
            for (unsigned i = 0; i < ETNA_NUM_LOD; i++)
               }
      static void etna_resource_finish_ts_import(struct etna_screen *screen,
         {
      struct etna_resource *ts_rsc = etna_resource(rsc->base.next);
   uint64_t ts_modifier = rsc->modifier & VIVANTE_MOD_TS_MASK;
   struct etna_resource_level *lvl = &rsc->levels[0];
            if (ts_rsc->bo == rsc->bo)
      fprintf(stderr, "etnaviv: application bug: importing shared TS resource "
         if (ts_modifier == VIVANTE_MOD_TS_256_4)
                     rsc->ts_scanout = ts_rsc->scanout;
            /* Get TS layout/usage information from the SW meta. FIXME: clear color may
   * change over the lifetime of the resource, so might need to look this up
   * at a few other places. For now it's not an issue, as buffers with shared
   * TS get re-imported all the time. */
   lvl->ts_meta = etna_bo_map(rsc->ts_bo) + ts_rsc->levels[0].offset;
   lvl->ts_compress_fmt = drmfourcc_to_ts_format(lvl->ts_meta->v0.comp_format);
   lvl->ts_offset = ts_rsc->levels[0].offset + lvl->ts_meta->v0.data_offset;
   lvl->ts_layer_stride = lvl->ts_meta->v0.layer_stride;
   lvl->clear_value = lvl->ts_meta->v0.clear_value;
   lvl->ts_size = lvl->ts_meta->v0.data_size;
            etna_resource_destroy(&screen->base, rsc->base.next);
      }
      static struct pipe_resource *
   etna_resource_from_handle(struct pipe_screen *pscreen,
               {
      struct etna_screen *screen = etna_screen(pscreen);
   struct etna_resource *rsc;
   struct etna_resource_level *level;
   struct pipe_resource *prsc;
            DBG("target=%d, format=%s, %ux%ux%u, array_size=%u, last_level=%u, "
      "nr_samples=%u, usage=%u, bind=%x, flags=%x",
   tmpl->target, util_format_name(tmpl->format), tmpl->width0,
   tmpl->height0, tmpl->depth0, tmpl->array_size, tmpl->last_level,
         rsc = CALLOC_STRUCT(etna_resource);
   if (!rsc)
            level = &rsc->levels[0];
                     pipe_reference_init(&prsc->reference, 1);
   util_range_init(&rsc->valid_buffer_range);
            rsc->bo = etna_screen_bo_from_handle(pscreen, handle);
   if (!rsc->bo)
            if (modifier == DRM_FORMAT_MOD_INVALID)
            rsc->layout = modifier_to_layout(modifier);
            if (usage & PIPE_HANDLE_USAGE_EXPLICIT_FLUSH)
            level->width = tmpl->width0;
   level->height = tmpl->height0;
   level->depth = tmpl->depth0;
   level->stride = handle->stride;
   level->offset = handle->offset;
            /* Determine padding of the imported resource. */
   unsigned paddingX, paddingY;
   etna_layout_multiple(screen, tmpl, rsc->layout,
            level->padded_width = align(level->width, paddingX);
            level->layer_stride = level->stride * util_format_get_nblocksy(prsc->format,
                  if (screen->ro)
      rsc->scanout = renderonly_create_gpu_import_for_resource(prsc, screen->ro,
         /* If the buffer is for a TS plane, skip the RS compatible checks */
   if (handle->plane >= util_format_get_num_planes(prsc->format))
            /* The DDX must give us a BO which conforms to our padding size.
   * The stride of the BO must be greater or equal to our padded
   * stride. The size of the BO must accomodate the padded height. */
   if (level->stride < util_format_get_stride(tmpl->format, level->padded_width)) {
      BUG("BO stride %u is too small for RS engine width padding (%u, format %s)",
      level->stride, util_format_get_stride(tmpl->format, level->padded_width),
         }
   if (etna_bo_size(rsc->bo) < level->stride * level->padded_height) {
      BUG("BO size %u is too small for RS engine height padding (%u, format %s)",
      etna_bo_size(rsc->bo), level->stride * level->padded_height,
                  if (handle->plane == 0 && etna_resource_ext_ts(rsc))
                  fail:
                  }
      static bool
   etna_resource_get_handle(struct pipe_screen *pscreen,
                     {
      struct etna_screen *screen = etna_screen(pscreen);
   struct etna_resource *rsc = etna_resource(prsc);
   bool wants_ts = etna_resource_ext_ts(rsc) &&
         struct renderonly_scanout *scanout;
   struct etna_resource_level *lvl;
            if (handle->plane && !wants_ts) {
               for (int i = 0; i < handle->plane; i++) {
      cur = cur->next;
   if (!cur)
      }
               /* Scanout is always attached to the base resource */
                     if (wants_ts) {
      handle->stride = DIV_ROUND_UP(lvl->stride,
               handle->offset = lvl->ts_offset - lvl->ts_meta->v0.data_offset;
   scanout = rsc->ts_scanout;
      } else {
      handle->stride = lvl->stride;
   handle->offset = lvl->offset;
   scanout = rsc->scanout;
      }
            if (!(usage & PIPE_HANDLE_USAGE_EXPLICIT_FLUSH))
            if (handle->type == WINSYS_HANDLE_TYPE_SHARED) {
         } else if (handle->type == WINSYS_HANDLE_TYPE_KMS) {
      if (screen->ro) {
         } else {
      handle->handle = etna_bo_handle(bo);
         } else if (handle->type == WINSYS_HANDLE_TYPE_FD) {
      handle->handle = etna_bo_dmabuf(bo);
      } else {
            }
      static bool
   etna_resource_get_param(struct pipe_screen *pscreen,
                           {
      struct etna_screen *screen = etna_screen(pscreen);
   struct etna_resource *rsc = etna_resource(prsc);
   bool wants_ts = etna_resource_ext_ts(rsc) &&
                  if (param == PIPE_RESOURCE_PARAM_NPLANES) {
      if (etna_resource_ext_ts(rsc)) {
         } else {
               for (struct pipe_resource *cur = prsc; cur; cur = cur->next)
            }
               if (!wants_ts) {
      struct pipe_resource *cur = prsc;
   for (int i = 0; i < plane; i++) {
      cur = cur->next;
   if (!cur)
      }
                        switch (param) {
   case PIPE_RESOURCE_PARAM_STRIDE:
      if (wants_ts) {
      *value = DIV_ROUND_UP(lvl->stride,
                  } else {
         }
      case PIPE_RESOURCE_PARAM_OFFSET:
      if (wants_ts)
         else
            case PIPE_RESOURCE_PARAM_MODIFIER:
      *value = etna_resource_modifier(rsc);
      default:
            }
      void
   etna_resource_used(struct etna_context *ctx, struct pipe_resource *prsc,
         {
      struct etna_resource *rsc;
   struct hash_entry *entry;
            if (!prsc)
            rsc = etna_resource(prsc);
   hash = _mesa_hash_pointer(rsc);
   entry = _mesa_hash_table_search_pre_hashed(ctx->pending_resources,
            if (entry) {
      enum etna_resource_status tmp = (uintptr_t)entry->data;
   tmp |= status;
      } else {
      _mesa_hash_table_insert_pre_hashed(ctx->pending_resources, hash, rsc,
         }
      enum etna_resource_status
   etna_resource_status(struct etna_context *ctx, struct etna_resource *res)
   {
               if (entry)
         else
      }
      bool
   etna_resource_needs_flush(struct etna_resource *rsc)
   {
      if (!rsc->ts_bo)
            for (int level = 0; level <= rsc->base.last_level; level++)
      if (etna_resource_level_needs_flush(&rsc->levels[level]))
            }
      void
   etna_resource_screen_init(struct pipe_screen *pscreen)
   {
      pscreen->can_create_resource = etna_screen_can_create_resource;
   pscreen->resource_create = etna_resource_create;
   pscreen->resource_create_with_modifiers = etna_resource_create_modifiers;
   pscreen->resource_from_handle = etna_resource_from_handle;
   pscreen->resource_get_handle = etna_resource_get_handle;
   pscreen->resource_get_param = etna_resource_get_param;
   pscreen->resource_changed = etna_resource_changed;
      }
