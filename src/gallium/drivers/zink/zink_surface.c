   /*
   * Copyright 2018 Collabora Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "zink_context.h"
   #include "zink_framebuffer.h"
   #include "zink_format.h"
   #include "zink_resource.h"
   #include "zink_screen.h"
   #include "zink_surface.h"
   #include "zink_kopper.h"
      #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
      VkImageViewCreateInfo
   create_ivci(struct zink_screen *screen,
               struct zink_resource *res,
   {
      VkImageViewCreateInfo ivci;
   /* zero holes since this is hashed */
   memset(&ivci, 0, sizeof(VkImageViewCreateInfo));
   ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            switch (target) {
   case PIPE_TEXTURE_1D:
      ivci.viewType = res->need_2D ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_1D;
         case PIPE_TEXTURE_1D_ARRAY:
      ivci.viewType = res->need_2D ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_1D_ARRAY;
         case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
         case PIPE_TEXTURE_2D_ARRAY:
      ivci.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
         case PIPE_TEXTURE_CUBE:
      ivci.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
         case PIPE_TEXTURE_CUBE_ARRAY:
      ivci.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
         case PIPE_TEXTURE_3D:
      ivci.viewType = VK_IMAGE_VIEW_TYPE_3D;
         default:
                  ivci.format = res->base.b.format == PIPE_FORMAT_A8_UNORM ? res->format : zink_get_format(screen, templ->format);
            /* TODO: it's currently illegal to use non-identity swizzles for framebuffer attachments,
   * but if that ever changes, this will be useful
   const struct util_format_description *desc = util_format_description(templ->format);
   ivci.components.r = zink_component_mapping(zink_clamp_void_swizzle(desc, PIPE_SWIZZLE_X));
   ivci.components.g = zink_component_mapping(zink_clamp_void_swizzle(desc, PIPE_SWIZZLE_Y));
   ivci.components.b = zink_component_mapping(zink_clamp_void_swizzle(desc, PIPE_SWIZZLE_Z));
   ivci.components.a = zink_component_mapping(zink_clamp_void_swizzle(desc, PIPE_SWIZZLE_W));
   */
   ivci.components.r = VK_COMPONENT_SWIZZLE_R;
   ivci.components.g = VK_COMPONENT_SWIZZLE_G;
   ivci.components.b = VK_COMPONENT_SWIZZLE_B;
            ivci.subresourceRange.aspectMask = res->aspect;
   ivci.subresourceRange.baseMipLevel = templ->u.tex.level;
   ivci.subresourceRange.levelCount = 1;
   ivci.subresourceRange.baseArrayLayer = templ->u.tex.first_layer;
   ivci.subresourceRange.layerCount = 1 + templ->u.tex.last_layer - templ->u.tex.first_layer;
   assert(ivci.viewType != VK_IMAGE_VIEW_TYPE_3D || ivci.subresourceRange.baseArrayLayer == 0);
   assert(ivci.viewType != VK_IMAGE_VIEW_TYPE_3D || ivci.subresourceRange.layerCount == 1);
   /* ensure cube image types get clamped to 2D/2D_ARRAY as expected for partial views */
               }
      /* this is used for framebuffer attachments to set up imageless framebuffers */
   static void
   init_surface_info(struct zink_screen *screen, struct zink_surface *surface, struct zink_resource *res, VkImageViewCreateInfo *ivci)
   {
      VkImageViewUsageCreateInfo *usage_info = (VkImageViewUsageCreateInfo *)ivci->pNext;
   surface->info.flags = res->obj->vkflags;
   surface->info.usage = usage_info ? usage_info->usage : res->obj->vkusage;
   surface->info.width = surface->base.width;
   surface->info.height = surface->base.height;
   surface->info.layerCount = ivci->subresourceRange.layerCount;
   surface->info.format[0] = ivci->format;
   if (res->obj->dt) {
      struct kopper_displaytarget *cdt = res->obj->dt;
   if (zink_kopper_has_srgb(cdt))
      } else {
      enum pipe_format srgb = util_format_is_srgb(surface->base.format) ? util_format_linear(surface->base.format) : util_format_srgb(surface->base.format);
   if (srgb == surface->base.format)
         if (srgb) {
      VkFormat format = zink_get_format(screen, srgb);
   if (format)
            }
      static void
   init_pipe_surface_info(struct pipe_context *pctx, struct pipe_surface *psurf, const struct pipe_surface *templ, const struct pipe_resource *pres)
   {
      unsigned int level = templ->u.tex.level;
   psurf->context = pctx;
   psurf->format = templ->format;
   psurf->width = u_minify(pres->width0, level);
   assert(psurf->width);
   psurf->height = u_minify(pres->height0, level);
   assert(psurf->height);
   psurf->nr_samples = templ->nr_samples;
   psurf->u.tex.level = level;
   psurf->u.tex.first_layer = templ->u.tex.first_layer;
      }
      static void
   apply_view_usage_for_format(struct zink_screen *screen, struct zink_resource *res, struct zink_surface *surface, enum pipe_format format, VkImageViewCreateInfo *ivci)
   {
      VkFormatFeatureFlags feats = res->linear ?
               VkImageUsageFlags attachment = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
   surface->usage_info.usage = res->obj->vkusage & ~attachment;
   if (res->obj->modifier_aspect) {
      feats = res->obj->vkfeats;
   /* intersect format features for current modifier */
   for (unsigned i = 0; i < screen->modifier_props[format].drmFormatModifierCount; i++) {
      if (res->obj->modifier == screen->modifier_props[format].pDrmFormatModifierProperties[i].drmFormatModifier)
         }
   /* if the format features don't support framebuffer attachment, use VkImageViewUsageCreateInfo to remove it */
   if ((res->obj->vkusage & attachment) &&
      !(feats & (VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))) {
         }
      static struct zink_surface *
   create_surface(struct pipe_context *pctx,
                  struct pipe_resource *pres,
      {
      struct zink_screen *screen = zink_screen(pctx->screen);
            struct zink_surface *surface = CALLOC_STRUCT(zink_surface);
   if (!surface)
            surface->usage_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
   surface->usage_info.pNext = NULL;
            pipe_resource_reference(&surface->base.texture, pres);
   pipe_reference_init(&surface->base.reference, 1);
   init_pipe_surface_info(pctx, &surface->base, templ, pres);
                     if (!actually)
         assert(ivci->image);
   VkResult result = VKSCR(CreateImageView)(screen->dev, ivci, NULL,
         if (result != VK_SUCCESS) {
      mesa_loge("ZINK: vkCreateImageView failed (%s)", vk_Result_to_str(result));
   FREE(surface);
                  }
      static uint32_t
   hash_ivci(const void *key)
   {
         }
      static struct zink_surface *
   do_create_surface(struct pipe_context *pctx, struct pipe_resource *pres, const struct pipe_surface *templ, VkImageViewCreateInfo *ivci, uint32_t hash, bool actually)
   {
      /* create a new surface */
   struct zink_surface *surface = create_surface(pctx, pres, templ, ivci, actually);
   /* only transient surfaces have nr_samples set */
   surface->base.nr_samples = zink_screen(pctx->screen)->info.have_EXT_multisampled_render_to_single_sampled ? templ->nr_samples : 0;
   surface->hash = hash;
   surface->ivci = *ivci;
      }
      /* get a cached surface for a shader descriptor */
   struct pipe_surface *
   zink_get_surface(struct zink_context *ctx,
               struct pipe_resource *pres,
   {
      struct zink_surface *surface = NULL;
   struct zink_resource *res = zink_resource(pres);
            simple_mtx_lock(&res->surface_mtx);
            if (!entry) {
      /* create a new surface, but don't actually create the imageview if mutable isn't set and the format is different;
   * mutable will be set later and the imageview will be filled in
   */
   bool actually = !zink_format_needs_mutable(pres->format, templ->format) || (pres->bind & ZINK_BIND_MUTABLE);
   surface = do_create_surface(&ctx->base, pres, templ, ivci, hash, actually);
   entry = _mesa_hash_table_insert_pre_hashed(&res->surface_cache, hash, &surface->ivci, surface);
   if (!entry) {
      simple_mtx_unlock(&res->surface_mtx);
                  } else {
      surface = entry->data;
      }
               }
      /* wrap a surface for use as a framebuffer attachment */
   static struct pipe_surface *
   wrap_surface(struct pipe_context *pctx, const struct pipe_surface *psurf)
   {
      struct zink_ctx_surface *csurf = CALLOC_STRUCT(zink_ctx_surface);
   if (!csurf) {
      mesa_loge("ZINK: failed to allocate csurf!");
      }
         csurf->base = *psurf;
   pipe_reference_init(&csurf->base.reference, 1);
   csurf->surf = (struct zink_surface*)psurf;
               }
      /* this the context hook that returns a zink_ctx_surface */
   static struct pipe_surface *
   zink_create_surface(struct pipe_context *pctx,
               {
      struct zink_resource *res = zink_resource(pres);
   bool is_array = templ->u.tex.last_layer != templ->u.tex.first_layer;
   bool needs_mutable = false;
   enum pipe_texture_target target_2d[] = {PIPE_TEXTURE_2D, PIPE_TEXTURE_2D_ARRAY};
   if (!res->obj->dt && zink_format_needs_mutable(pres->format, templ->format)) {
      /* mutable not set by default */
   needs_mutable = !(res->base.b.bind & ZINK_BIND_MUTABLE);
   /*
      VUID-VkImageViewCreateInfo-image-07072
   If image was created with the VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT flag and
   format is a non-compressed format, the levelCount and layerCount members of
      */
   if (util_format_is_compressed(pres->format) && templ->u.tex.first_layer != templ->u.tex.last_layer)
               if (!zink_screen(pctx->screen)->threaded && needs_mutable) {
      /* this is fine without tc */
   needs_mutable = false;
               if (!zink_get_format(zink_screen(pctx->screen), templ->format))
            VkImageViewCreateInfo ivci = create_ivci(zink_screen(pctx->screen), res, templ,
            struct pipe_surface *psurf = NULL;
   if (res->obj->dt) {
      /* don't cache swapchain surfaces. that's weird. */
   struct zink_surface *surface = do_create_surface(pctx, pres, templ, &ivci, 0, false);
   if (surface) {
      surface->is_swapchain = true;
         } else if (!needs_mutable) {
         }
   if (!psurf && !needs_mutable)
            struct zink_ctx_surface *csurf = (struct zink_ctx_surface*)wrap_surface(pctx, needs_mutable ? templ : psurf);
   csurf->needs_mutable = needs_mutable;
   if (needs_mutable) {
      csurf->surf = NULL;
   pipe_resource_reference(&csurf->base.texture, pres);
               if (templ->nr_samples && !zink_screen(pctx->screen)->info.have_EXT_multisampled_render_to_single_sampled) {
      /* transient fb attachment: not cached */
   struct pipe_resource rtempl = *pres;
   rtempl.nr_samples = templ->nr_samples;
   rtempl.bind |= ZINK_BIND_TRANSIENT;
   struct zink_resource *transient = zink_resource(pctx->screen->resource_create(pctx->screen, &rtempl));
   if (!transient)
         ivci.image = transient->obj->image;
   csurf->transient = (struct zink_ctx_surface*)wrap_surface(pctx, (struct pipe_surface*)create_surface(pctx, &transient->base.b, templ, &ivci, true));
   if (!csurf->transient) {
      pipe_resource_reference((struct pipe_resource**)&transient, NULL);
   pipe_surface_release(pctx, &psurf);
      }
                  }
      void
   zink_destroy_surface(struct zink_screen *screen, struct pipe_surface *psurface)
   {
      struct zink_surface *surface = zink_surface(psurface);
   struct zink_resource *res = zink_resource(psurface->texture);
   if ((!psurface->nr_samples || screen->info.have_EXT_multisampled_render_to_single_sampled) && !surface->is_swapchain) {
      simple_mtx_lock(&res->surface_mtx);
   if (psurface->reference.count) {
      /* a different context got a cache hit during deletion: this surface is alive again */
   simple_mtx_unlock(&res->surface_mtx);
      }
   struct hash_entry *he = _mesa_hash_table_search_pre_hashed(&res->surface_cache, surface->hash, &surface->ivci);
   assert(he);
   assert(he->data == surface);
   _mesa_hash_table_remove(&res->surface_cache, he);
      }
   /* this surface is dead now */
   simple_mtx_lock(&res->obj->view_lock);
   /* imageviews are never destroyed directly to ensure lifetimes for in-use surfaces */
   if (surface->is_swapchain) {
      for (unsigned i = 0; i < surface->swapchain_size; i++)
            } else
         simple_mtx_unlock(&res->obj->view_lock);
   pipe_resource_reference(&psurface->texture, NULL);
      }
      /* this is the context hook, so only zink_ctx_surfaces will reach it */
   static void
   zink_surface_destroy(struct pipe_context *pctx,
         {
      struct zink_ctx_surface *csurf = (struct zink_ctx_surface *)psurface;
   if (csurf->needs_mutable)
      /* this has an extra resource ref */
      zink_surface_reference(zink_screen(pctx->screen), &csurf->surf, NULL);
   pipe_surface_release(pctx, (struct pipe_surface**)&csurf->transient);
      }
      /* this is called when a surface is rebound for mutable/storage use */
   bool
   zink_rebind_surface(struct zink_context *ctx, struct pipe_surface **psurface)
   {
      struct zink_surface *surface = zink_surface(*psurface);
   struct zink_resource *res = zink_resource((*psurface)->texture);
   struct zink_screen *screen = zink_screen(ctx->base.screen);
   if (surface->obj == res->obj)
         assert(!res->obj->dt);
   VkImageViewCreateInfo ivci = surface->ivci;
   ivci.image = res->obj->image;
            simple_mtx_lock(&res->surface_mtx);
   struct hash_entry *new_entry = _mesa_hash_table_search_pre_hashed(&res->surface_cache, hash, &ivci);
   if (new_entry) {
      /* reuse existing surface; old one will be cleaned up naturally */
   struct zink_surface *new_surface = new_entry->data;
   simple_mtx_unlock(&res->surface_mtx);
   zink_surface_reference(screen, (struct zink_surface**)psurface, new_surface);
      }
   struct hash_entry *entry = _mesa_hash_table_search_pre_hashed(&res->surface_cache, surface->hash, &surface->ivci);
   assert(entry);
   _mesa_hash_table_remove(&res->surface_cache, entry);
   VkImageView image_view;
   apply_view_usage_for_format(screen, res, surface, surface->base.format, &ivci);
   VkResult result = VKSCR(CreateImageView)(screen->dev, &ivci, NULL, &image_view);
   if (result != VK_SUCCESS) {
      mesa_loge("ZINK: failed to create new imageview (%s)", vk_Result_to_str(result));
   simple_mtx_unlock(&res->surface_mtx);
      }
   surface->hash = hash;
   surface->ivci = ivci;
   entry = _mesa_hash_table_insert_pre_hashed(&res->surface_cache, surface->hash, &surface->ivci, surface);
   assert(entry);
   simple_mtx_lock(&res->obj->view_lock);
   util_dynarray_append(&res->obj->views, VkImageView, surface->image_view);
   simple_mtx_unlock(&res->obj->view_lock);
   surface->image_view = image_view;
   surface->obj = zink_resource(surface->base.texture)->obj;
   /* update for imageless fb */
   surface->info.flags = res->obj->vkflags;
   surface->info.usage = res->obj->vkusage;
   simple_mtx_unlock(&res->surface_mtx);
      }
      /* dummy surfaces are used for null framebuffer/descriptors */
   struct pipe_surface *
   zink_surface_create_null(struct zink_context *ctx, enum pipe_texture_target target, unsigned width, unsigned height, unsigned samples)
   {
               struct pipe_resource *pres;
   struct pipe_resource templ = {0};
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.format = PIPE_FORMAT_R8G8B8A8_UNORM;
   templ.target = target;
   templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
   if (samples < 2)
                  pres = ctx->base.screen->resource_create(ctx->base.screen, &templ);
   if (!pres)
            surf_templ.format = PIPE_FORMAT_R8G8B8A8_UNORM;
   surf_templ.nr_samples = 0;
   struct pipe_surface *psurf = ctx->base.create_surface(&ctx->base, pres, &surf_templ);
   pipe_resource_reference(&pres, NULL);
      }
      void
   zink_context_surface_init(struct pipe_context *context)
   {
      context->create_surface = zink_create_surface;
      }
      /* must be called before a swapchain image is used to ensure correct imageview is used */
   void
   zink_surface_swapchain_update(struct zink_context *ctx, struct zink_surface *surface)
   {
      struct zink_screen *screen = zink_screen(ctx->base.screen);
   struct zink_resource *res = zink_resource(surface->base.texture);
   struct kopper_displaytarget *cdt = res->obj->dt;
   if (!cdt)
         if (cdt->swapchain != surface->dt_swapchain) {
      /* new swapchain: clear out previous swapchain imageviews/array and setup a new one;
   * old views will be pruned normally in zink_batch or on object destruction
   */
   simple_mtx_lock(&res->obj->view_lock);
   for (unsigned i = 0; i < surface->swapchain_size; i++)
         simple_mtx_unlock(&res->obj->view_lock);
   free(surface->swapchain);
   surface->swapchain_size = cdt->swapchain->num_images;
   surface->swapchain = calloc(surface->swapchain_size, sizeof(VkImageView));
   if (!surface->swapchain) {
      mesa_loge("ZINK: failed to allocate surface->swapchain!");
      }
   surface->base.width = res->base.b.width0;
   surface->base.height = res->base.b.height0;
   init_surface_info(screen, surface, res, &surface->ivci);
      }
   if (!surface->swapchain[res->obj->dt_idx]) {
      /* no current swapchain imageview exists: create it */
   assert(res->obj->image && cdt->swapchain->images[res->obj->dt_idx].image == res->obj->image);
   surface->ivci.image = res->obj->image;
   assert(surface->ivci.image);
      }
   /* the current swapchain imageview is now the view for the current swapchain image */
      }
