   /**********************************************************
   * Copyright 2008-2023 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "svga3d_reg.h"
   #include "svga3d_surfacedefs.h"
      #include "pipe/p_state.h"
   #include "pipe/p_defines.h"
   #include "util/u_thread.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_resource.h"
   #include "util/u_upload_mgr.h"
      #include "svga_cmd.h"
   #include "svga_format.h"
   #include "svga_screen.h"
   #include "svga_context.h"
   #include "svga_resource_texture.h"
   #include "svga_resource_buffer.h"
   #include "svga_sampler_view.h"
   #include "svga_surface.h"
   #include "svga_winsys.h"
   #include "svga_debug.h"
         static void
   svga_transfer_dma_band(struct svga_context *svga,
                        struct svga_transfer *st,
   SVGA3dTransferType transfer,
      {
      struct svga_texture *texture = svga_texture(st->base.resource);
                     box.x = x;
   box.y = y;
   box.z = z;
   box.w = w;
   box.h = h;
   box.d = d;
   box.srcx = srcx;
   box.srcy = srcy;
            SVGA_DBG(DEBUG_DMA, "dma %s sid %p, face %u, (%u, %u, %u) - "
            "(%u, %u, %u), %ubpp\n",
   transfer == SVGA3D_WRITE_HOST_VRAM ? "to" : "from",
   texture->handle,
   st->slice,
   x,
   y,
   z,
   x + w,
   y + h,
   z + 1,
   util_format_get_blocksize(texture->b.format) * 8 /
            }
         static void
   svga_transfer_dma(struct svga_context *svga,
                     {
      struct svga_texture *texture = svga_texture(st->base.resource);
   struct svga_screen *screen = svga_screen(texture->b.screen);
   struct svga_winsys_screen *sws = screen->sws;
                     if (transfer == SVGA3D_READ_HOST_VRAM) {
                  /* Ensure any pending operations on host surfaces are queued on the command
   * buffer first.
   */
            if (!st->swbuf) {
      /* Do the DMA transfer in a single go */
   svga_transfer_dma_band(svga, st, transfer,
                              if (transfer == SVGA3D_READ_HOST_VRAM) {
      svga_context_flush(svga, &fence);
   sws->fence_finish(sws, fence, OS_TIMEOUT_INFINITE, 0);
         }
   else {
      int y, h, srcy;
   unsigned blockheight =
            h = st->hw_nblocksy * blockheight;
            for (y = 0; y < st->box.h; y += h) {
                                    /* Transfer band must be aligned to pixel block boundaries */
                                                            /* Wait for the previous DMAs to complete */
   /* TODO: keep one DMA (at half the size) in the background */
   if (y) {
                        hw = sws->buffer_map(sws, st->hwbuf, usage);
   assert(hw);
   if (hw) {
      memcpy(hw, sw, length);
                  svga_transfer_dma_band(svga, st, transfer,
                        /*
   * Prevent the texture contents to be discarded on the next band
   * upload.
                  if (transfer == SVGA3D_READ_HOST_VRAM) {
                     hw = sws->buffer_map(sws, st->hwbuf, PIPE_MAP_READ);
   assert(hw);
   if (hw) {
      memcpy(sw, hw, length);
                  }
            bool
   svga_resource_get_handle(struct pipe_screen *screen,
                           {
      struct svga_winsys_screen *sws = svga_winsys_screen(texture->screen);
            if (texture->target == PIPE_BUFFER)
            SVGA_DBG(DEBUG_DMA, "%s: texture=%p cachable=%d\n", __FUNCTION__,
                     stride = util_format_get_nblocksx(texture->format, texture->width0) *
            return sws->surface_get_handle(sws, svga_texture(texture)->handle,
      }
         /**
   * Determine if we need to read back a texture image before mapping it.
   */
   static inline bool
   need_tex_readback(struct svga_transfer *st)
   {
      if (st->base.usage & PIPE_MAP_READ)
            if ((st->base.usage & PIPE_MAP_WRITE) &&
      ((st->base.usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE) == 0)) {
                  }
         static void
   readback_texture_surface(struct svga_context *svga,
               {
               /* Mark the texture surface as UPDATED */
            svga->hud.num_readbacks++;
      }
      /**
   * Use DMA for the transfer request
   */
   static void *
   svga_texture_transfer_map_dma(struct svga_context *svga,
         {
      struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;
   struct pipe_resource *texture = st->base.resource;
   unsigned nblocksx, nblocksy;
   unsigned d;
            /* we'll put the data into a tightly packed buffer */
   nblocksx = util_format_get_nblocksx(texture->format, st->box.w);
   nblocksy = util_format_get_nblocksy(texture->format, st->box.h);
            st->base.stride = nblocksx*util_format_get_blocksize(texture->format);
   st->base.layer_stride = st->base.stride * nblocksy;
            st->hwbuf = svga_winsys_buffer_create(svga, 1, 0,
            while (!st->hwbuf && (st->hw_nblocksy /= 2)) {
      st->hwbuf =
      svga_winsys_buffer_create(svga, 1, 0,
            if (!st->hwbuf)
            if (st->hw_nblocksy < nblocksy) {
      /* We couldn't allocate a hardware buffer big enough for the transfer,
   * so allocate regular malloc memory instead
   */
   if (0) {
      debug_printf("%s: failed to allocate %u KB of DMA, "
               "splitting into %u x %u KB DMA transfers\n",
   __func__,
               st->swbuf = MALLOC(nblocksy * st->base.stride * d);
   if (!st->swbuf) {
      sws->buffer_destroy(sws, st->hwbuf);
                  if (usage & PIPE_MAP_READ) {
      SVGA3dSurfaceDMAFlags flags;
   memset(&flags, 0, sizeof flags);
               if (st->swbuf) {
         }
   else {
            }
         /**
   * Use direct map for the transfer request
   */
   static void *
   svga_texture_transfer_map_direct(struct svga_context *svga,
         {
      struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;
   struct pipe_transfer *transfer = &st->base;
   struct pipe_resource *texture = transfer->resource;
   struct svga_texture *tex = svga_texture(texture);
   struct svga_winsys_surface *surf = tex->handle;
   unsigned level = st->base.level;
   unsigned w, h, nblocksx, nblocksy;
            if (need_tex_readback(st)) {
               if (!svga->swc->force_coherent || tex->imported) {
                        }
   /*
   * Note: if PIPE_MAP_DISCARD_WHOLE_RESOURCE were specified
   * we could potentially clear the flag for all faces/layers/mips.
   */
      }
   else {
      assert(usage & PIPE_MAP_WRITE);
   if ((usage & PIPE_MAP_UNSYNCHRONIZED) == 0) {
      if (svga_is_texture_level_dirty(tex, st->slice, level)) {
      /*
   * do a surface flush if the subresource has been modified
   * in this command buffer.
   */
   svga_surfaces_flush(svga);
   if (!sws->surface_is_flushed(sws, surf)) {
      svga->hud.surface_write_flushes++;
   SVGA_STATS_COUNT_INC(sws, SVGA_STATS_COUNT_SURFACEWRITEFLUSH);
                        /* we'll directly access the guest-backed surface */
   w = u_minify(texture->width0, level);
   h = u_minify(texture->height0, level);
   nblocksx = util_format_get_nblocksx(texture->format, w);
   nblocksy = util_format_get_nblocksy(texture->format, h);
   st->hw_nblocksy = nblocksy;
   st->base.stride = nblocksx*util_format_get_blocksize(texture->format);
            /*
   * Begin mapping code
   */
   {
      SVGA3dSize baseLevelSize;
   uint8_t *map;
   bool retry, rebind;
   unsigned offset, mip_width, mip_height;
            if (swc->force_coherent) {
                  map = SVGA_TRY_MAP(svga->swc->surface_map
            if (map == NULL && retry) {
      /*
   * At this point, the svga_surfaces_flush() should already have
   * called in svga_texture_get_transfer().
   */
   svga->hud.surface_write_flushes++;
   svga_retry_enter(svga);
   svga_context_flush(svga, NULL);
   map = svga->swc->surface_map(svga->swc, surf, usage, &retry, &rebind);
      }
   if (map && rebind) {
               ret = SVGA3D_BindGBSurface(swc, surf);
   if (ret != PIPE_OK) {
      svga_context_flush(svga, NULL);
   ret = SVGA3D_BindGBSurface(swc, surf);
      }
               /*
   * Make sure we return NULL if the map fails
   */
   if (!map) {
                  /**
   * Compute the offset to the specific texture slice in the buffer.
   */
   baseLevelSize.width = tex->b.width0;
   baseLevelSize.height = tex->b.height0;
            if ((tex->b.target == PIPE_TEXTURE_1D_ARRAY) ||
      (tex->b.target == PIPE_TEXTURE_2D_ARRAY) ||
   (tex->b.target == PIPE_TEXTURE_CUBE_ARRAY)) {
   st->base.layer_stride =
      svga3dsurface_get_image_offset(tex->key.format, baseLevelSize,
            offset = svga3dsurface_get_image_offset(tex->key.format, baseLevelSize,
               if (level > 0) {
                  mip_width = u_minify(tex->b.width0, level);
            offset += svga3dsurface_get_pixel_offset(tex->key.format,
                                    }
         /**
   * Request a transfer map to the texture resource
   */
   void *
   svga_texture_transfer_map(struct pipe_context *pipe,
                           struct pipe_resource *texture,
   {
      struct svga_context *svga = svga_context(pipe);
   struct svga_winsys_screen *sws = svga_screen(pipe->screen)->sws;
   struct svga_texture *tex = svga_texture(texture);
   struct svga_transfer *st;
   struct svga_winsys_surface *surf = tex->handle;
   bool use_direct_map = svga_have_gb_objects(svga) &&
         void *map = NULL;
                     if (!surf)
            /* We can't map texture storage directly unless we have GB objects */
   if (usage & PIPE_MAP_DIRECTLY) {
      if (svga_have_gb_objects(svga))
         else
               st = CALLOC_STRUCT(svga_transfer);
   if (!st)
            st->base.level = level;
   st->base.usage = usage;
            /* The modified transfer map box with the array index removed from z.
   * The array index is specified in slice.
   */
   st->box.x = box->x;
   st->box.y = box->y;
   st->box.z = box->z;
   st->box.w = box->width;
   st->box.h = box->height;
            switch (tex->b.target) {
   case PIPE_TEXTURE_CUBE:
      st->slice = st->base.box.z;
   st->box.z = 0;   /* so we don't apply double offsets below */
      case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
      st->slice = st->base.box.z;
            /* Force direct map for transfering multiple slices */
   if (st->base.box.depth > 1)
               default:
      st->slice = 0;
               /* We never want to use DMA transfers on systems with GBObjects because
   * it causes serialization issues and in SVGAv3 vram is gone which
   * makes it impossible to support both at the same time.
   */
   if (svga_have_gb_objects(svga)) {
                  st->use_direct_map = use_direct_map;
            /* If this is the first time mapping to the surface in this
   * command buffer and there is no pending primitives, clear
   * the dirty masks of this surface.
   */
   if (sws->surface_is_flushed(sws, surf) &&
      (svga_have_vgpu10(svga) ||
   !svga_hwtnl_has_pending_prim(svga->hwtnl))) {
               if (!use_direct_map) {
      /* upload to the DMA buffer */
      }
   else {
      bool can_use_upload = tex->can_use_upload &&
         bool was_rendered_to =
                  /* If the texture was already rendered to or has pending changes and
   * upload buffer is supported, then we will use upload buffer to
   * avoid the need to read back the texture content; otherwise,
   * we'll first try to map directly to the GB surface, if it is blocked,
   * then we'll try the upload buffer.
   */
   if ((was_rendered_to || is_dirty) && can_use_upload) {
         }
   else {
               /* First try directly map to the GB surface */
   if (can_use_upload)
                        if (!map && can_use_upload) {
      /* if direct map with DONTBLOCK fails, then try upload to the
   * texture upload buffer.
   */
                  /* If upload fails, then try direct map again without forcing it
   * to DONTBLOCK.
   */
   if (!map) {
                     if (!map) {
         }
   else {
      *ptransfer = &st->base;
   svga->hud.num_textures_mapped++;
   if (usage & PIPE_MAP_WRITE) {
      /* record texture upload for HUD */
                  /* mark this texture level as dirty */
               done:
      svga->hud.map_buffer_time += (svga_get_time(svga) - begin);
   SVGA_STATS_TIME_POP(sws);
               }
      /**
   * Unmap a GB texture surface.
   */
   static void
   svga_texture_surface_unmap(struct svga_context *svga,
         {
      struct svga_winsys_surface *surf = svga_texture(transfer->resource)->handle;
   struct svga_winsys_context *swc = svga->swc;
                     swc->surface_unmap(swc, surf, &rebind);
   if (rebind) {
            }
         static void
   update_image_vgpu9(struct svga_context *svga,
                     struct svga_winsys_surface *surf,
   {
         }
         static void
   update_image_vgpu10(struct svga_context *svga,
                     struct svga_winsys_surface *surf,
   const SVGA3dBox *box,
   {
                        SVGA_RETRY(svga, SVGA3D_vgpu10_UpdateSubResource(svga->swc, surf, box,
      }
         /**
   * unmap DMA transfer request
   */
   static void
   svga_texture_transfer_unmap_dma(struct svga_context *svga,
         {
               if (!st->swbuf)
            if (st->base.usage & PIPE_MAP_WRITE) {
      /* Use DMA to transfer texture data */
   SVGA3dSurfaceDMAFlags flags;
   struct pipe_resource *texture = st->base.resource;
               memset(&flags, 0, sizeof flags);
   if (st->base.usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE) {
         }
   if (st->base.usage & PIPE_MAP_UNSYNCHRONIZED) {
                  svga_transfer_dma(svga, st, SVGA3D_WRITE_HOST_VRAM, flags);
               FREE(st->swbuf);
      }
         /**
   * unmap direct map transfer request
   */
   static void
   svga_texture_transfer_unmap_direct(struct svga_context *svga,
         {
      struct pipe_transfer *transfer = &st->base;
                     /* Now send an update command to update the content in the backend. */
   if (st->base.usage & PIPE_MAP_WRITE) {
                        /* update the effected region */
   SVGA3dBox box = st->box;
            switch (tex->b.target) {
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
   case PIPE_TEXTURE_1D_ARRAY:
      nlayers = box.d;
   box.d = 1;
      default:
      nlayers = 1;
                  if (0)
      debug_printf("%s %d, %d, %d  %d x %d x %d\n",
                     if (!svga->swc->force_coherent || tex->imported) {
                        for (i = 0; i < nlayers; i++) {
      update_image_vgpu10(svga, surf, &box,
               } else {
      assert(nlayers == 1);
   update_image_vgpu9(svga, surf, &box, st->slice,
                  /* Mark the texture surface state as UPDATED */
         }
         void
   svga_texture_transfer_unmap(struct pipe_context *pipe,
         {
      struct svga_context *svga = svga_context(pipe);
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_transfer *st = svga_transfer(transfer);
                     if (!st->use_direct_map) {
         }
   else if (st->upload.buf) {
         }
   else {
                  if (st->base.usage & PIPE_MAP_WRITE) {
               /* Mark the texture level as dirty */
   ss->texture_timestamp++;
   svga_age_texture_view(tex, transfer->level);
   if (transfer->resource->target == PIPE_TEXTURE_CUBE)
         else
               pipe_resource_reference(&st->base.resource, NULL);
   FREE(st);
   SVGA_STATS_TIME_POP(sws);
      }
         /**
   * Does format store depth values?
   */
   static inline bool
   format_has_depth(enum pipe_format format)
   {
      const struct util_format_description *desc = util_format_description(format);
      }
      struct pipe_resource *
   svga_texture_create(struct pipe_screen *screen,
         {
      struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_texture *tex;
            SVGA_STATS_TIME_PUSH(svgascreen->sws,
            assert(template->last_level < SVGA_MAX_TEXTURE_LEVELS);
   if (template->last_level >= SVGA_MAX_TEXTURE_LEVELS) {
                  /* Verify the number of mipmap levels isn't impossibly large.  For example,
   * if the base 2D image is 16x16, we can't have 8 mipmap levels.
   * the gallium frontend should never ask us to create a resource with invalid
   * parameters.
   */
   {
               switch (template->target) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      // nothing
      case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
      max_dim = MAX2(max_dim, template->height0);
      case PIPE_TEXTURE_3D:
      max_dim = MAX3(max_dim, template->height0, template->depth0);
      case PIPE_TEXTURE_RECT:
   case PIPE_BUFFER:
      assert(template->last_level == 0);
   /* the assertion below should always pass */
      default:
         }
               tex = CALLOC_STRUCT(svga_texture);
   if (!tex) {
                  tex->defined = CALLOC(template->depth0 * template->array_size,
         if (!tex->defined) {
      FREE(tex);
               tex->dirty = CALLOC(template->depth0 * template->array_size,
         if (!tex->dirty) {
                  tex->b = *template;
   pipe_reference_init(&tex->b.reference, 1);
            tex->key.flags = 0;
   tex->key.size.width = template->width0;
   tex->key.size.height = template->height0;
   tex->key.size.depth = template->depth0;
   tex->key.arraySize = 1;
            /* nr_samples=1 must be treated as a non-multisample texture */
   if (tex->b.nr_samples == 1) {
         }
   else if (tex->b.nr_samples > 1) {
      assert(svgascreen->sws->have_sm4_1);
                        if (svgascreen->sws->have_vgpu10) {
      switch (template->target) {
   case PIPE_TEXTURE_1D:
      tex->key.flags |= SVGA3D_SURFACE_1D;
      case PIPE_TEXTURE_1D_ARRAY:
      tex->key.flags |= SVGA3D_SURFACE_1D;
      case PIPE_TEXTURE_2D_ARRAY:
      tex->key.flags |= SVGA3D_SURFACE_ARRAY;
   tex->key.arraySize = template->array_size;
      case PIPE_TEXTURE_3D:
      tex->key.flags |= SVGA3D_SURFACE_VOLUME;
      case PIPE_TEXTURE_CUBE:
      tex->key.flags |= (SVGA3D_SURFACE_CUBEMAP | SVGA3D_SURFACE_ARRAY);
   tex->key.numFaces = 6;
      case PIPE_TEXTURE_CUBE_ARRAY:
      assert(svgascreen->sws->have_sm4_1);
   tex->key.flags |= (SVGA3D_SURFACE_CUBEMAP | SVGA3D_SURFACE_ARRAY);
   tex->key.numFaces = 1;  // arraySize already includes the 6 faces
   tex->key.arraySize = template->array_size;
      default:
            }
   else {
      switch (template->target) {
   case PIPE_TEXTURE_3D:
      tex->key.flags |= SVGA3D_SURFACE_VOLUME;
      case PIPE_TEXTURE_CUBE:
      tex->key.flags |= SVGA3D_SURFACE_CUBEMAP;
   tex->key.numFaces = 6;
      default:
                              if ((bindings & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_DEPTH_STENCIL)) &&
      !(bindings & PIPE_BIND_SAMPLER_VIEW)) {
   /* Also check if the format can be sampled from */
   if (screen->is_format_supported(screen, template->format,
                                             if (bindings & PIPE_BIND_SAMPLER_VIEW) {
      tex->key.flags |= SVGA3D_SURFACE_HINT_TEXTURE;
            if (!(bindings & PIPE_BIND_RENDER_TARGET)) {
      /* Also check if the format is color renderable */
   if (screen->is_format_supported(screen, template->format,
                                             if (!(bindings & PIPE_BIND_DEPTH_STENCIL)) {
      /* Also check if the format is depth/stencil renderable */
   if (screen->is_format_supported(screen, template->format,
                                                if (bindings & PIPE_BIND_DISPLAY_TARGET) {
                  if (bindings & PIPE_BIND_SHARED) {
                  if (bindings & (PIPE_BIND_SCANOUT | PIPE_BIND_CURSOR)) {
      tex->key.scanout = 1;
               /*
   * Note: Previously we never passed the
   * SVGA3D_SURFACE_HINT_RENDERTARGET hint. Mesa cannot
   * know beforehand whether a texture will be used as a rendertarget or not
   * and it always requests PIPE_BIND_RENDER_TARGET, therefore
   * passing the SVGA3D_SURFACE_HINT_RENDERTARGET here defeats its purpose.
   *
   * However, this was changed since other gallium frontends
   * (XA for example) uses it accurately and certain device versions
   * relies on it in certain situations to render correctly.
   */
   if ((bindings & PIPE_BIND_RENDER_TARGET) &&
      !util_format_is_s3tc(template->format)) {
   tex->key.flags |= SVGA3D_SURFACE_HINT_RENDERTARGET;
               if (bindings & PIPE_BIND_DEPTH_STENCIL) {
      tex->key.flags |= SVGA3D_SURFACE_HINT_DEPTHSTENCIL;
                        tex->key.format = svga_translate_format(svgascreen, template->format,
         if (tex->key.format == SVGA3D_FORMAT_INVALID) {
                  bool use_typeless = false;
   if (svgascreen->sws->have_gl43) {
      /* Do not use typeless for SHARED, SCANOUT or DISPLAY_TARGET surfaces. */
   use_typeless = !(bindings & (PIPE_BIND_SHARED | PIPE_BIND_SCANOUT |
      } else if (svgascreen->sws->have_vgpu10) {
      /* For VGPU10 device, use typeless formats only for sRGB and depth resources
   * if they do not have SHARED, SCANOUT or DISPLAY_TARGET bind flags
   */
   use_typeless = (util_format_is_srgb(template->format) ||
                           if (use_typeless) {
      SVGA3dSurfaceFormat typeless = svga_typeless_format(tex->key.format);
   if (0) {
      debug_printf("Convert resource type %s -> %s (bind 0x%x)\n",
                           if (svga_format_is_uncompressed_snorm(tex->key.format)) {
      /* We can't normally render to snorm surfaces, but once we
   * substitute a typeless format, we can if the rendertarget view
   * is unorm.  This can happen with GL_ARB_copy_image.
   */
   tex->key.flags |= SVGA3D_SURFACE_HINT_RENDERTARGET;
                           if (svgascreen->sws->have_sm5 &&
      bindings & (PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET)) {
   if (template->nr_samples < 2 &&
      screen->is_format_supported(screen, template->format,
                           /* Any non multi-samples texture that can be used as a render target
   * or sampler view can be bound to an image unit.
   * So make sure to set the UAV flag here.
   */
                  SVGA_DBG(DEBUG_DMA, "surface_create for texture\n");
   bool invalidated;
   tex->handle = svga_screen_surface_create(svgascreen, bindings,
               if (!tex->handle) {
         }
   if (invalidated) {
         } else {
                           debug_reference(&tex->b.reference,
                     /* Determine if texture upload buffer can be used to upload this texture */
   tex->can_use_upload = svga_texture_transfer_map_can_upload(svgascreen,
            /* Initialize the backing resource cache */
            svgascreen->hud.total_resource_bytes += tex->size;
                           fail:
      if (tex->dirty)
         if (tex->defined)
            fail_notex:
      SVGA_STATS_TIME_POP(svgascreen->sws);
      }
         struct pipe_resource *
   svga_texture_from_handle(struct pipe_screen *screen,
               {
      struct svga_winsys_screen *sws = svga_winsys_screen(screen);
   struct svga_screen *ss = svga_screen(screen);
   struct svga_winsys_surface *srf;
   struct svga_texture *tex;
   enum SVGA3dSurfaceFormat format = 0;
            /* Only supports one type */
   if ((template->target != PIPE_TEXTURE_2D &&
      template->target != PIPE_TEXTURE_RECT) ||
   template->last_level != 0 ||
   template->depth0 != 1) {
                        if (!srf)
            if (!svga_format_is_shareable(ss, template->format, format,
                  tex = CALLOC_STRUCT(svga_texture);
   if (!tex)
            tex->defined = CALLOC(template->depth0 * template->array_size,
         if (!tex->defined)
            tex->b = *template;
   pipe_reference_init(&tex->b.reference, 1);
                     tex->key.cachable = 0;
   tex->key.format = format;
               /* set bind flags for the imported texture handle according to the bind
   * flags in the template
   */
   if (template->bind & PIPE_BIND_RENDER_TARGET){
      tex->key.flags |= SVGA3D_SURFACE_HINT_RENDERTARGET;
               if (template->bind & PIPE_BIND_DEPTH_STENCIL) {
      tex->key.flags |= SVGA3D_SURFACE_HINT_DEPTHSTENCIL;
               if (template->bind & PIPE_BIND_SAMPLER_VIEW) {
      tex->key.flags |= SVGA3D_SURFACE_HINT_TEXTURE;
               tex->dirty = CALLOC(1, sizeof(tex->dirty[0]));
   if (!tex->dirty)
                                    out_no_dirty:
         out_no_defined:
         out_unref:
      sws->surface_reference(sws, &srf, NULL);
      }
      bool
   svga_texture_generate_mipmap(struct pipe_context *pipe,
                              struct pipe_resource *pt,
      {
      struct pipe_sampler_view templ, *psv;
   struct svga_pipe_sampler_view *sv;
   struct svga_context *svga = svga_context(pipe);
                     /* Fallback to the mipmap generation utility for those formats that
   * do not support hw generate mipmap
   */
   if (!svga_format_support_gen_mips(format))
            /* Make sure the texture surface was created with
   * SVGA3D_SURFACE_BIND_RENDER_TARGET
   */
   if (!tex->handle || !(tex->key.flags & SVGA3D_SURFACE_BIND_RENDER_TARGET))
            templ.format = format;
   templ.target = pt->target;
   templ.u.tex.first_layer = first_layer;
   templ.u.tex.last_layer = last_layer;
   templ.u.tex.first_level = base_level;
            if (pt->target == PIPE_TEXTURE_CUBE) {
      /**
   * state tracker generates mipmap one face at a time.
   * But SVGA generates mipmap for the entire cubemap.
   */
   templ.u.tex.first_layer = 0;
               psv = pipe->create_sampler_view(pipe, pt, &templ);
   if (psv == NULL)
            sv = svga_pipe_sampler_view(psv);
            SVGA_RETRY(svga, SVGA3D_vgpu10_GenMips(svga->swc, sv->id, tex->handle));
            /* Mark the texture surface as RENDERED */
                        }
         /* texture upload buffer default size in bytes */
   #define TEX_UPLOAD_DEFAULT_SIZE (1024 * 1024)
      /**
   * Create a texture upload buffer
   */
   bool
   svga_texture_transfer_map_upload_create(struct svga_context *svga)
   {
      svga->tex_upload = u_upload_create(&svga->pipe, TEX_UPLOAD_DEFAULT_SIZE,
         if (svga->tex_upload)
               }
         /**
   * Destroy the texture upload buffer
   */
   void
   svga_texture_transfer_map_upload_destroy(struct svga_context *svga)
   {
         }
         /**
   * Returns true if this transfer map request can use the upload buffer.
   */
   bool
   svga_texture_transfer_map_can_upload(const struct svga_screen *svgascreen,
         {
      if (svgascreen->sws->have_transfer_from_buffer_cmd == false)
            /* TransferFromBuffer command is not well supported with multi-samples surface */
   if (texture->nr_samples > 1)
            if (util_format_is_compressed(texture->format)) {
      /* XXX Need to take a closer look to see why texture upload
   * with 3D texture with compressed format fails
   */ 
   if (texture->target == PIPE_TEXTURE_3D)
      }
   else if (texture->format == PIPE_FORMAT_R9G9B9E5_FLOAT) {
                     }
         /**
   *  Return TRUE if the same texture is bound to the specified
   *  surface view and a backing resource is created for the surface view.
   */
   static bool
   need_update_texture_resource(struct pipe_surface *surf,
         {
      struct svga_texture *stex = svga_texture(surf->texture);
               }
         /**
   *  Make sure the texture resource is up-to-date. If the texture is
   *  currently bound to a render target view and a backing resource is
   *  created, we will need to update the original resource with the
   *  changes in the backing resource.
   */
   static void
   svga_validate_texture_resource(struct svga_context *svga,
         {
      if (svga_was_texture_rendered_to(tex) == false)
            if ((svga->state.hw_draw.has_backed_views == false) ||
      (tex->backed_handle == NULL))
         struct pipe_surface *s;
   for (unsigned i = 0; i < svga->state.hw_clear.num_rendertargets; i++) {
      s = svga->state.hw_clear.rtv[i];
   if (s && need_update_texture_resource(s, tex))
               s = svga->state.hw_clear.dsv;
   if (s && need_update_texture_resource(s, tex))
      }
         /**
   * Use upload buffer for the transfer map request.
   */
   void *
   svga_texture_transfer_map_upload(struct svga_context *svga,
         {
      struct pipe_resource *texture = st->base.resource;
   struct pipe_resource *tex_buffer = NULL;
   struct svga_texture *tex = svga_texture(texture);
   void *tex_map;
   unsigned nblocksx, nblocksy;
   unsigned offset;
                     /* Validate the texture resource in case there is any changes
   * in the backing resource that needs to be updated to the original
   * texture resource first before the transfer upload occurs, otherwise,
   * the later update from backing resource to original will overwrite the
   * changes in this transfer map update.
   */
            st->upload.box.x = st->base.box.x;
   st->upload.box.y = st->base.box.y;
   st->upload.box.z = st->base.box.z;
   st->upload.box.w = st->base.box.width;
   st->upload.box.h = st->base.box.height;
   st->upload.box.d = st->base.box.depth;
            switch (texture->target) {
   case PIPE_TEXTURE_CUBE:
      st->upload.box.z = 0;
      case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
      st->upload.nlayers = st->base.box.depth;
   st->upload.box.z = 0;
   st->upload.box.d = 1;
      case PIPE_TEXTURE_1D_ARRAY:
      st->upload.nlayers = st->base.box.depth;
   st->upload.box.y = st->upload.box.z = 0;
   st->upload.box.d = 1;
      default:
                  nblocksx = util_format_get_nblocksx(texture->format, st->base.box.width);
            st->base.stride = nblocksx * util_format_get_blocksize(texture->format);
            /* In order to use the TransferFromBuffer command to update the
   * texture content from the buffer, the layer stride for a multi-layers
   * surface needs to be in multiples of 16 bytes.
   */
   if (st->upload.nlayers > 1 && st->base.layer_stride & 15)
            upload_size = st->base.layer_stride * st->base.box.depth;
         #ifdef DEBUG
      if (util_format_is_compressed(texture->format)) {
                        /* dest box must start on block boundary */
   assert((st->base.box.x % blockw) == 0);
         #endif
         /* If the upload size exceeds the default buffer size, the
   * upload buffer manager code will try to allocate a new buffer
   * with the new buffer size.
   */
   u_upload_alloc(svga->tex_upload, 0, upload_size, 16,
            if (!tex_map) {
                  st->upload.buf = tex_buffer;
   st->upload.map = tex_map;
               }
         /**
   * Unmap upload map transfer request
   */
   void
   svga_texture_transfer_unmap_upload(struct svga_context *svga,
         {
      struct svga_winsys_surface *srcsurf;
   struct svga_winsys_surface *dstsurf;
   struct pipe_resource *texture = st->base.resource;
   struct svga_texture *tex = svga_texture(texture);
   unsigned subResource;
   unsigned numMipLevels;
   unsigned i, layer;
            assert(svga->tex_upload);
            /* unmap the texture upload buffer */
            srcsurf = svga_buffer_handle(svga, st->upload.buf, 0);
   dstsurf = svga_texture(texture)->handle;
                     for (i = 0, layer = st->slice; i < st->upload.nlayers; i++, layer++) {
               /* send a transferFromBuffer command to update the host texture surface */
            SVGA_RETRY(svga, SVGA3D_vgpu10_TransferFromBuffer(svga->swc, srcsurf,
                                             /* Mark the texture surface state as RENDERED */
               }
      /**
   * Does the device format backing this surface have an
   * alpha channel?
   *
   * \param texture[in]  The texture whose format we're querying
   * \return TRUE if the format has an alpha channel, FALSE otherwise
   *
   * For locally created textures, the device (svga) format is typically
   * identical to svga_format(texture->format), and we can use the gallium
   * format tests to determine whether the device format has an alpha channel
   * or not. However, for textures backed by imported svga surfaces that is
   * not always true, and we have to look at the SVGA3D utilities.
   */
   bool
   svga_texture_device_format_has_alpha(struct pipe_resource *texture)
   {
      /* the svga_texture() call below is invalid for PIPE_BUFFER resources */
            const struct svga3d_surface_desc *surf_desc =
                     return !!((block_desc & SVGA3DBLOCKDESC_ALPHA) ||
            }
