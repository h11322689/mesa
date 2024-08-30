   /*
   * Copyright (c) 2017-2019 Lima Project
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
      #include "util/u_memory.h"
   #include "util/u_blitter.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_debug.h"
   #include "util/u_transfer.h"
   #include "util/u_surface.h"
   #include "util/u_transfer_helper.h"
   #include "util/hash_table.h"
   #include "util/ralloc.h"
   #include "util/u_drm.h"
   #include "renderonly/renderonly.h"
      #include "frontend/drm_driver.h"
      #include "drm-uapi/drm_fourcc.h"
   #include "drm-uapi/lima_drm.h"
      #include "lima_screen.h"
   #include "lima_context.h"
   #include "lima_resource.h"
   #include "lima_bo.h"
   #include "lima_util.h"
   #include "lima_blit.h"
      #include "pan_minmax_cache.h"
   #include "pan_tiling.h"
      static struct pipe_resource *
   lima_resource_create_scanout(struct pipe_screen *pscreen,
               {
      struct lima_screen *screen = lima_screen(pscreen);
   struct renderonly_scanout *scanout;
            struct lima_resource *res = CALLOC_STRUCT(lima_resource);
   if (!res)
            struct pipe_resource scanout_templat = *templat;
   scanout_templat.width0 = width;
   scanout_templat.height0 = height;
            scanout = renderonly_scanout_for_resource(&scanout_templat,
         if (!scanout)
            res->base = *templat;
   res->base.screen = pscreen;
   pipe_reference_init(&res->base.reference, 1);
   res->levels[0].offset = handle.offset;
            assert(handle.type == WINSYS_HANDLE_TYPE_FD);
   res->bo = lima_bo_import(screen, &handle);
   if (!res->bo) {
      FREE(res);
                        close(handle.handle);
   if (!res->bo) {
      renderonly_scanout_destroy(scanout, screen->ro);
   FREE(res);
                           }
      static uint32_t
   setup_miptree(struct lima_resource *res,
               {
      struct pipe_resource *pres = &res->base;
   unsigned level;
   unsigned width = width0;
   unsigned height = height0;
   unsigned depth = pres->depth0;
   unsigned nr_samples = MAX2(pres->nr_samples, 1);
            for (level = 0; level <= pres->last_level; level++) {
      uint32_t actual_level_size;
   uint32_t stride;
   unsigned aligned_width;
            if (align_to_tile) {
      aligned_width = align(width, 16);
      } else {
      aligned_width = width;
               stride = util_format_get_stride(pres->format, aligned_width);
   actual_level_size = stride *
                  res->levels[level].stride = stride;
   res->levels[level].offset = size;
            if (util_format_is_compressed(pres->format))
                     width = u_minify(width, 1);
   height = u_minify(height, 1);
               if (nr_samples > 1)
                        }
      static struct pipe_resource *
   lima_resource_create_bo(struct pipe_screen *pscreen,
                     {
      struct lima_screen *screen = lima_screen(pscreen);
   struct lima_resource *res;
            res = CALLOC_STRUCT(lima_resource);
   if (!res)
            res->base = *templat;
   res->base.screen = pscreen;
                     uint32_t size = setup_miptree(res, width, height, align_to_tile);
            res->bo = lima_bo_create(screen, size, 0);
   if (!res->bo) {
      FREE(res);
                  }
      static struct pipe_resource *
   _lima_resource_create_with_modifiers(struct pipe_screen *pscreen,
                     {
      struct lima_screen *screen = lima_screen(pscreen);
   bool should_tile = lima_debug & LIMA_DEBUG_NO_TILING ? false : true;
   unsigned width, height;
   bool has_user_modifiers = true;
            if (count == 1 && modifiers[0] == DRM_FORMAT_MOD_INVALID)
            /* VBOs/PBOs are untiled (and 1 height). */
   if (templat->target == PIPE_BUFFER)
            if (templat->bind & (PIPE_BIND_LINEAR | PIPE_BIND_SCANOUT))
            /* If there's no user modifiers and buffer is shared we use linear */
   if (!has_user_modifiers && (templat->bind & PIPE_BIND_SHARED))
            if (has_user_modifiers &&
      !drm_find_modifier(DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED,
               width = templat->width0;
            /* Don't align index, vertex or constant buffers */
   if (!(templat->bind & (PIPE_BIND_INDEX_BUFFER |
                  if (templat->bind & PIPE_BIND_SHARED) {
      width = align(width, 16);
      }
               struct pipe_resource *pres;
   if (screen->ro && (templat->bind & PIPE_BIND_SCANOUT))
         else
            if (pres) {
      struct lima_resource *res = lima_resource(pres);
            if (templat->bind & PIPE_BIND_INDEX_BUFFER)
            debug_printf("%s: pres=%p width=%u height=%u depth=%u target=%d "
                  }
      }
      static struct pipe_resource *
   lima_resource_create(struct pipe_screen *pscreen,
         {
                  }
      static struct pipe_resource *
   lima_resource_create_with_modifiers(struct pipe_screen *pscreen,
                     {
               /* gbm_bo_create_with_modifiers & gbm_surface_create_with_modifiers
   * don't have usage parameter, but buffer created by these functions
   * may be used for scanout. So we assume buffer created by this
   * function always enable scanout if linear modifier is permitted.
   */
   if (drm_find_modifier(DRM_FORMAT_MOD_LINEAR, modifiers, count))
               }
      static void
   lima_resource_destroy(struct pipe_screen *pscreen, struct pipe_resource *pres)
   {
      struct lima_screen *screen = lima_screen(pscreen);
            if (res->bo)
            if (res->scanout)
            if (res->damage.region)
            if (res->index_cache)
               }
      static struct pipe_resource *
   lima_resource_from_handle(struct pipe_screen *pscreen,
         const struct pipe_resource *templat,
   {
      if (templat->bind & (PIPE_BIND_SAMPLER_VIEW |
                  /* sampler hardware need offset alignment 64, while render hardware
   * need offset alignment 8, but due to render target may be reloaded
   * which uses the sampler, set alignment requrement to 64 for all
   */
   if (handle->offset & 0x3f) {
      debug_error("import buffer offset not properly aligned\n");
                  struct lima_resource *res = CALLOC_STRUCT(lima_resource);
   if (!res)
            struct pipe_resource *pres = &res->base;
   *pres = *templat;
   pres->screen = pscreen;
   pipe_reference_init(&pres->reference, 1);
   res->levels[0].offset = handle->offset;
            struct lima_screen *screen = lima_screen(pscreen);
   res->bo = lima_bo_import(screen, handle);
   if (!res->bo) {
      FREE(res);
                        switch (handle->modifier) {
   case DRM_FORMAT_MOD_LINEAR:
      res->tiled = false;
      case DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED:
      res->tiled = true;
      case DRM_FORMAT_MOD_INVALID:
      /* Modifier wasn't specified and it's shared buffer. We create these
   * as linear, so disable tiling.
   */
   res->tiled = false;
      default:
      fprintf(stderr, "Attempted to import unsupported modifier 0x%llx\n",
                     /* check alignment for the buffer */
   if (res->tiled ||
      (pres->bind & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_DEPTH_STENCIL))) {
            width = align(pres->width0, 16);
   height = align(pres->height0, 16);
   stride = util_format_get_stride(pres->format, width);
            if (res->tiled && res->levels[0].stride != stride) {
      fprintf(stderr, "tiled imported buffer has mismatching stride: %d (BO) != %d (expected)",
                     if (!res->tiled && (res->levels[0].stride % 8)) {
      fprintf(stderr, "linear imported buffer stride is not aligned to 8 bytes: %d\n",
               if (!res->tiled && res->levels[0].stride < stride) {
      fprintf(stderr, "linear imported buffer stride is smaller than minimal: %d (BO) < %d (min)",
                     if ((res->bo->size - res->levels[0].offset) < size) {
      fprintf(stderr, "imported bo size is smaller than expected: %d (BO) < %d (expected)\n",
                        if (screen->ro) {
      /* Make sure that renderonly has a handle to our buffer in the
   * display's fd, so that a later renderonly_get_handle()
   * returns correct handles or GEM names.
   */
   res->scanout =
      renderonly_create_gpu_import_for_resource(pres,
                              err_out:
      lima_resource_destroy(pscreen, pres);
      }
      static bool
   lima_resource_get_handle(struct pipe_screen *pscreen,
                     {
      struct lima_screen *screen = lima_screen(pscreen);
            if (res->tiled)
         else
                     if (handle->type == WINSYS_HANDLE_TYPE_KMS && screen->ro)
            if (!lima_bo_export(res->bo, handle))
            handle->offset = res->levels[0].offset;
   handle->stride = res->levels[0].stride;
      }
      static bool
   lima_resource_get_param(struct pipe_screen *pscreen,
                           struct pipe_context *pctx,
   {
               switch (param) {
   case PIPE_RESOURCE_PARAM_STRIDE:
      *value = res->levels[level].stride;
      case PIPE_RESOURCE_PARAM_OFFSET:
      *value = res->levels[level].offset;
      case PIPE_RESOURCE_PARAM_MODIFIER:
      if (res->tiled)
         else
               default:
            }
      static void
   get_scissor_from_box(struct pipe_scissor_state *s,
         {
      int y = h - (b->y + b->height);
   /* region in tile unit */
   s->minx = b->x >> 4;
   s->miny = y >> 4;
   s->maxx = (b->x + b->width + 0xf) >> 4;
      }
      static void
   get_damage_bound_box(struct pipe_resource *pres,
                     {
               for (int i = 1; i < nrects; i++)
            int ret = u_box_clip_2d(&b, &b, pres->width0, pres->height0);
   if (ret < 0)
         else
      }
      static void
   lima_resource_set_damage_region(struct pipe_screen *pscreen,
                     {
      struct lima_resource *res = lima_resource(pres);
   struct lima_damage_region *damage = &res->damage;
            if (damage->region) {
      FREE(damage->region);
   damage->region = NULL;
               if (!nrects)
            /* check full damage
   *
   * TODO: currently only check if there is any single damage
   * region that can cover the full render target; there may
   * be some accurate way, but a single window size damage
   * region is most of the case from weston
   */
   for (i = 0; i < nrects; i++) {
      if (rects[i].x <= 0 && rects[i].y <= 0 &&
      rects[i].x + rects[i].width >= pres->width0 &&
   rects[i].y + rects[i].height >= pres->height0)
            struct pipe_scissor_state *bound = &damage->bound;
            damage->region = CALLOC(nrects, sizeof(*damage->region));
   if (!damage->region)
            for (i = 0; i < nrects; i++)
      get_scissor_from_box(damage->region + i, rects + i,
         /* is region aligned to tiles? */
   damage->aligned = true;
   for (i = 0; i < nrects; i++) {
      if (rects[i].x & 0xf || rects[i].y & 0xf ||
      rects[i].width & 0xf || rects[i].height & 0xf) {
   damage->aligned = false;
                     }
      static struct pipe_surface *
   lima_surface_create(struct pipe_context *pctx,
               {
               if (!surf)
                     struct pipe_surface *psurf = &surf->base;
            pipe_reference_init(&psurf->reference, 1);
            psurf->context = pctx;
   psurf->format = surf_tmpl->format;
   psurf->width = u_minify(pres->width0, level);
   psurf->height = u_minify(pres->height0, level);
   psurf->nr_samples = surf_tmpl->nr_samples;
   psurf->u.tex.level = level;
   psurf->u.tex.first_layer = surf_tmpl->u.tex.first_layer;
            surf->tiled_w = align(psurf->width, 16) >> 4;
            surf->reload = 0;
   if (util_format_has_stencil(util_format_description(psurf->format)))
         if (util_format_has_depth(util_format_description(psurf->format)))
         if (!util_format_is_depth_or_stencil(psurf->format))
               }
      static void
   lima_surface_destroy(struct pipe_context *pctx, struct pipe_surface *psurf)
   {
               pipe_resource_reference(&psurf->texture, NULL);
      }
      static void *
   lima_transfer_map(struct pipe_context *pctx,
                     struct pipe_resource *pres,
   unsigned level,
   {
      struct lima_screen *screen = lima_screen(pres->screen);
   struct lima_context *ctx = lima_context(pctx);
   struct lima_resource *res = lima_resource(pres);
   struct lima_bo *bo = res->bo;
   struct lima_transfer *trans;
            /* No direct mappings of tiled, since we need to manually
   * tile/untile.
   */
   if (res->tiled && (usage & PIPE_MAP_DIRECTLY))
            /* bo might be in use in a previous stream draw. Allocate a new
   * one for the resource to avoid overwriting data in use. */
   if (usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE) {
      struct lima_bo *new_bo;
            new_bo = lima_bo_create(screen, res->bo->size, res->bo->flags);
   if (!new_bo)
            lima_bo_unreference(res->bo);
            if (pres->bind & PIPE_BIND_VERTEX_BUFFER)
               }
   else if (!(usage & PIPE_MAP_UNSYNCHRONIZED) &&
            /* use once buffers are made sure to not read/write overlapped
   * range, so no need to sync */
            unsigned op = usage & PIPE_MAP_WRITE ?
                     if (!lima_bo_map(bo))
            trans = slab_zalloc(&ctx->transfer_pool);
   if (!trans)
                     pipe_resource_reference(&ptrans->resource, pres);
   ptrans->level = level;
   ptrans->usage = usage;
                     if (res->tiled) {
      ptrans->stride = util_format_get_stride(pres->format, ptrans->box.width);
                     if (usage & PIPE_MAP_READ) {
      unsigned line_stride = res->levels[level].stride;
                  unsigned i;
   for (i = 0; i < ptrans->box.depth; i++)
      panfrost_load_tiled_image(
      trans->staging + i * ptrans->stride * ptrans->box.height,
   bo->map + res->levels[level].offset + (i + box->z) * res->levels[level].layer_stride,
   ptrans->box.x, ptrans->box.y,
   ptrans->box.width, ptrans->box.height,
   ptrans->stride,
                  } else {
      unsigned dpw = PIPE_MAP_DIRECTLY | PIPE_MAP_WRITE |
         if ((usage & dpw) == dpw && res->index_cache)
            ptrans->stride = res->levels[level].stride;
            if ((usage & PIPE_MAP_WRITE) && (usage & PIPE_MAP_DIRECTLY))
            return bo->map + res->levels[level].offset +
      box->z * res->levels[level].layer_stride +
   box->y / util_format_get_blockheight(pres->format) * ptrans->stride +
   box->x / util_format_get_blockwidth(pres->format) *
      }
      static bool
   lima_should_convert_linear(struct lima_resource *res,
         {
      if (res->modifier_constant)
            /* Overwriting the entire resource indicates streaming, for which
   * linear layout is most efficient due to the lack of expensive
   * conversion.
   *
   * For now we just switch to linear after a number of complete
   * overwrites to keep things simple, but we could do better.
            unsigned depth = res->base.target == PIPE_TEXTURE_3D ?
         bool entire_overwrite =
         res->base.last_level == 0 &&
   ptrans->box.width == res->base.width0 &&
   ptrans->box.height == res->base.height0 &&
   ptrans->box.depth == depth &&
   ptrans->box.x == 0 &&
            if (entire_overwrite)
               }
      static void
   lima_transfer_flush_region(struct pipe_context *pctx,
               {
      struct lima_context *ctx = lima_context(pctx);
   struct lima_resource *res = lima_resource(ptrans->resource);
   struct lima_transfer *trans = lima_transfer(ptrans);
   struct lima_bo *bo = res->bo;
            if (trans->staging) {
      pres = &res->base;
   if (trans->base.usage & PIPE_MAP_WRITE) {
      unsigned i;
   if (lima_should_convert_linear(res, ptrans)) {
      /* It's safe to re-use the same BO since tiled BO always has
   * aligned dimensions */
   for (i = 0; i < trans->base.box.depth; i++) {
      util_copy_rect(bo->map + res->levels[0].offset +
                        res->base.format,
   res->levels[0].stride,
   0, 0,
   ptrans->box.width,
   ptrans->box.height,
   }
   res->tiled = false;
   res->modifier_constant = true;
   /* Update texture descriptor */
      } else {
      unsigned line_stride = res->levels[ptrans->level].stride;
                  for (i = 0; i < trans->base.box.depth; i++)
      panfrost_store_tiled_image(
      bo->map + res->levels[trans->base.level].offset + (i + trans->base.box.z) * res->levels[trans->base.level].layer_stride,
   trans->staging + i * ptrans->stride * ptrans->box.height,
   ptrans->box.x, ptrans->box.y,
   ptrans->box.width, ptrans->box.height,
   row_stride,
               }
      static void
   lima_transfer_unmap(struct pipe_context *pctx,
         {
      struct lima_context *ctx = lima_context(pctx);
   struct lima_transfer *trans = lima_transfer(ptrans);
            struct pipe_box box;
   u_box_2d(0, 0, ptrans->box.width, ptrans->box.height, &box);
   lima_transfer_flush_region(pctx, ptrans, &box);
   if (trans->staging)
                  pipe_resource_reference(&ptrans->resource, NULL);
      }
      static void
   lima_util_blitter_save_states(struct lima_context *ctx)
   {
      util_blitter_save_blend(ctx->blitter, (void *)ctx->blend);
   util_blitter_save_depth_stencil_alpha(ctx->blitter, (void *)ctx->zsa);
   util_blitter_save_stencil_ref(ctx->blitter, &ctx->stencil_ref);
   util_blitter_save_rasterizer(ctx->blitter, (void *)ctx->rasterizer);
   util_blitter_save_fragment_shader(ctx->blitter, ctx->uncomp_fs);
   util_blitter_save_vertex_shader(ctx->blitter, ctx->uncomp_vs);
   util_blitter_save_viewport(ctx->blitter,
         util_blitter_save_scissor(ctx->blitter, &ctx->scissor);
   util_blitter_save_vertex_elements(ctx->blitter,
         util_blitter_save_vertex_buffer_slot(ctx->blitter,
                     util_blitter_save_fragment_sampler_states(ctx->blitter,
               util_blitter_save_fragment_sampler_views(ctx->blitter,
            }
      static void
   lima_blit(struct pipe_context *pctx, const struct pipe_blit_info *blit_info)
   {
      struct lima_context *ctx = lima_context(pctx);
            if (lima_do_blit(pctx, blit_info)) {
                  if (util_try_blit_via_copy_region(pctx, &info, false)) {
                  if (info.mask & PIPE_MASK_S) {
      debug_printf("lima: cannot blit stencil, skipping\n");
               if (!util_blitter_is_blit_supported(ctx->blitter, &info)) {
      debug_printf("lima: blit unsupported %s -> %s\n",
                                       }
      static void
   lima_flush_resource(struct pipe_context *pctx, struct pipe_resource *resource)
   {
      }
      static void
   lima_texture_subdata(struct pipe_context *pctx,
                        struct pipe_resource *prsc,
   unsigned level,
   unsigned usage,
      {
      struct lima_context *ctx = lima_context(pctx);
            if (!res->tiled) {
      u_default_texture_subdata(pctx, prsc, level, usage, box,
                              struct lima_transfer t = {
      .base = {
      .resource = prsc,
   .usage = PIPE_MAP_WRITE,
   .level = level,
   .box = *box,
   .stride = stride,
      },
               lima_flush_job_accessing_bo(ctx, res->bo, true);
   lima_bo_wait(res->bo, LIMA_GEM_WAIT_WRITE, OS_TIMEOUT_INFINITE);
   if (!lima_bo_map(res->bo))
            struct pipe_box tbox;
   u_box_2d(0, 0, t.base.box.width, t.base.box.height, &tbox);
      }
      static const struct u_transfer_vtbl transfer_vtbl = {
      .resource_create       = lima_resource_create,
   .resource_destroy      = lima_resource_destroy,
   .transfer_map          = lima_transfer_map,
   .transfer_unmap        = lima_transfer_unmap,
      };
      void
   lima_resource_screen_init(struct lima_screen *screen)
   {
      screen->base.resource_create = lima_resource_create;
   screen->base.resource_create_with_modifiers = lima_resource_create_with_modifiers;
   screen->base.resource_from_handle = lima_resource_from_handle;
   screen->base.resource_destroy = lima_resource_destroy;
   screen->base.resource_get_handle = lima_resource_get_handle;
   screen->base.resource_get_param = lima_resource_get_param;
   screen->base.set_damage_region = lima_resource_set_damage_region;
   screen->base.transfer_helper = u_transfer_helper_create(&transfer_vtbl,
      }
      void
   lima_resource_screen_destroy(struct lima_screen *screen)
   {
         }
      void
   lima_resource_context_init(struct lima_context *ctx)
   {
      ctx->base.create_surface = lima_surface_create;
            ctx->base.buffer_subdata = u_default_buffer_subdata;
   ctx->base.texture_subdata = lima_texture_subdata;
   /* TODO: optimize resource_copy_region to do copy directly
   * between 2 tiled or tiled and linear resources instead of
   * using staging buffer.
   */
                     ctx->base.buffer_map = u_transfer_helper_transfer_map;
   ctx->base.texture_map = u_transfer_helper_transfer_map;
   ctx->base.transfer_flush_region = u_transfer_helper_transfer_flush_region;
   ctx->base.buffer_unmap = u_transfer_helper_transfer_unmap;
               }
