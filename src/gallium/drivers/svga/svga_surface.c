   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
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
      #include "svga_cmd.h"
      #include "pipe/p_state.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/u_thread.h"
   #include "util/u_bitmask.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "svga_format.h"
   #include "svga_screen.h"
   #include "svga_context.h"
   #include "svga_sampler_view.h"
   #include "svga_resource_texture.h"
   #include "svga_surface.h"
   #include "svga_debug.h"
      static void svga_mark_surface_dirty(struct pipe_surface *surf);
      void
   svga_texture_copy_region(struct svga_context *svga,
                           struct svga_winsys_surface *src_handle,
   unsigned srcSubResource,
   unsigned src_x, unsigned src_y, unsigned src_z,
   {
                        box.x = dst_x;
   box.y = dst_y;
   box.z = dst_z;
   box.w = width;
   box.h = height;
   box.d = depth;
   box.srcx = src_x;
   box.srcy = src_y;
            SVGA_RETRY(svga, SVGA3D_vgpu10_PredCopyRegion
            }
         void
   svga_texture_copy_handle(struct svga_context *svga,
                           struct svga_winsys_surface *src_handle,
   unsigned src_x, unsigned src_y, unsigned src_z,
   unsigned src_level, unsigned src_layer,
   {
      struct svga_surface dst, src;
                     src.handle = src_handle;
   src.real_level = src_level;
   src.real_layer = src_layer;
            dst.handle = dst_handle;
   dst.real_level = dst_level;
   dst.real_layer = dst_layer;
            box.x = dst_x;
   box.y = dst_y;
   box.z = dst_z;
   box.w = width;
   box.h = height;
   box.d = depth;
   box.srcx = src_x;
   box.srcy = src_y;
         /*
      SVGA_DBG(DEBUG_VIEWS, "mipcopy src: %p %u (%ux%ux%u), dst: %p %u (%ux%ux%u)\n",
            */
         SVGA_RETRY(svga, SVGA3D_BeginSurfaceCopy(svga->swc,
                        *boxes = box;
      }
         /* A helper function to sync up the two surface handles.
   */
   static void
   svga_texture_copy_handle_resource(struct svga_context *svga,
                                    struct svga_texture *src_tex,
      {
      unsigned int i, j;
            /* A negative zslice_pick implies zoffset at 0, and depth to copy is
   * from the depth of the texture at the particular mipmap level.
   */
   if (zslice_pick >= 0)
            for (i = 0; i < numMipLevels; i++) {
               for (j = 0; j < numLayers; j++) {
      if (svga_is_texture_level_defined(src_tex, j+layeroffset, miplevel)) {
                     if (src_tex->b.nr_samples > 1) {
      unsigned subResource = j * numMipLevels + i;
   svga_texture_copy_region(svga, src_tex->handle,
                  }
   else {
      svga_texture_copy_handle(svga,
                           src_tex->handle,
   0, 0, zoffset,
   miplevel,
                  }
         struct svga_winsys_surface *
   svga_texture_view_surface(struct svga_context *svga,
                           struct svga_texture *tex,
   unsigned bind_flags,
   SVGA3dSurfaceAllFlags flags,
   SVGA3dSurfaceFormat format,
   unsigned start_mip,
   unsigned num_mip,
   int layer_pick,
   {
      struct svga_screen *ss = svga_screen(svga->pipe.screen);
   struct svga_winsys_surface *handle = NULL;
   bool invalidated;
            SVGA_DBG(DEBUG_PERF,
                           key->flags = flags;
   key->format = format;
   key->numMipLevels = num_mip;
   key->size.width = u_minify(tex->b.width0, start_mip);
   key->size.height = u_minify(tex->b.height0, start_mip);
   key->size.depth = zslice_pick < 0 ? u_minify(tex->b.depth0, start_mip) : 1;
   key->cachable = 1;
   key->arraySize = 1;
            /* single sample surface can be treated as non-multisamples surface */
            if (key->sampleCount > 1) {
      assert(ss->sws->have_sm4_1);
               if (tex->b.target == PIPE_TEXTURE_CUBE && layer_pick < 0) {
      key->flags |= SVGA3D_SURFACE_CUBEMAP;
      } else if (tex->b.target == PIPE_TEXTURE_1D_ARRAY ||
                        if (key->format == SVGA3D_FORMAT_INVALID) {
      key->cachable = 0;
               if (cacheable && tex->backed_handle &&
      memcmp(key, &tex->backed_key, sizeof *key) == 0) {
   handle = tex->backed_handle;
      } else {
      SVGA_DBG(DEBUG_DMA, "surface_create for texture view\n");
   handle = svga_screen_surface_create(ss, bind_flags, PIPE_USAGE_DEFAULT,
                  if (cacheable && !tex->backed_handle) {
      tex->backed_handle = handle;
                  if (!handle) {
      key->cachable = 0;
                        if (layer_pick < 0)
            if (needCopyResource) {
      svga_texture_copy_handle_resource(svga, tex, handle,
                              done:
                  }
         /**
   * A helper function to create a surface view.
   * The clone_resource boolean flag specifies whether to clone the resource
   * for the surface view.
   */
   static struct pipe_surface *
   svga_create_surface_view(struct pipe_context *pipe,
                     {
      struct svga_context *svga = svga_context(pipe);
   struct svga_texture *tex = svga_texture(pt);
   struct pipe_screen *screen = pipe->screen;
   struct svga_screen *ss = svga_screen(screen);
   struct svga_surface *s;
   unsigned layer, zslice, bind;
   unsigned nlayers = 1;
   SVGA3dSurfaceAllFlags flags = 0;
   SVGA3dSurfaceFormat format;
            s = CALLOC_STRUCT(svga_surface);
   if (!s)
                     if (pt->target == PIPE_TEXTURE_CUBE) {
      layer = surf_tmpl->u.tex.first_layer;
      }
   else if (pt->target == PIPE_TEXTURE_1D_ARRAY ||
            pt->target == PIPE_TEXTURE_2D_ARRAY ||
   layer = surf_tmpl->u.tex.first_layer;
   zslice = 0;
      }
   else {
      layer = 0;
               pipe_reference_init(&s->base.reference, 1);
   pipe_resource_reference(&s->base.texture, pt);
   s->base.context = pipe;
   s->base.format = surf_tmpl->format;
   s->base.width = u_minify(pt->width0, surf_tmpl->u.tex.level);
   s->base.height = u_minify(pt->height0, surf_tmpl->u.tex.level);
   s->base.u.tex.level = surf_tmpl->u.tex.level;
   s->base.u.tex.first_layer = surf_tmpl->u.tex.first_layer;
   s->base.u.tex.last_layer = surf_tmpl->u.tex.last_layer;
                     if (util_format_is_depth_or_stencil(surf_tmpl->format)) {
      flags = SVGA3D_SURFACE_HINT_DEPTHSTENCIL |
            }
   else {
      flags = SVGA3D_SURFACE_HINT_RENDERTARGET |
                     if (tex->imported) {
      /* imported resource (a window) */
   format = tex->key.format;
   if (util_format_is_srgb(surf_tmpl->format)) {
      /* sRGB rendering to window */
         }
   else {
                           if (clone_resource) {
      SVGA_DBG(DEBUG_VIEWS,
                  if (svga_have_vgpu10(svga)) {
      switch (pt->target) {
   case PIPE_TEXTURE_1D:
      flags |= SVGA3D_SURFACE_1D;
      case PIPE_TEXTURE_1D_ARRAY:
      flags |= SVGA3D_SURFACE_1D | SVGA3D_SURFACE_ARRAY;
      case PIPE_TEXTURE_2D_ARRAY:
      flags |= SVGA3D_SURFACE_ARRAY;
      case PIPE_TEXTURE_3D:
      flags |= SVGA3D_SURFACE_VOLUME;
      case PIPE_TEXTURE_CUBE:
      if (nlayers == 6)
            case PIPE_TEXTURE_CUBE_ARRAY:
      if (nlayers % 6 == 0)
            default:
                     /* When we clone the surface view resource, use the format used in
   * the creation of the original resource.
   */
   s->handle = svga_texture_view_surface(svga, tex, bind, flags,
                           if (!s->handle) {
      FREE(s);
               s->key.format = format;
   s->real_layer = 0;
   s->real_level = 0;
      } else {
      SVGA_DBG(DEBUG_VIEWS,
                  memset(&s->key, 0, sizeof s->key);
   s->key.format = format;
   s->handle = tex->handle;
   s->real_layer = layer;
   s->real_zslice = zslice;
               svga->hud.num_surface_views++;
         done:
      SVGA_STATS_TIME_POP(ss->sws);
      }
         static struct pipe_surface *
   svga_create_surface(struct pipe_context *pipe,
               {
      struct svga_context *svga = svga_context(pipe);
   struct pipe_screen *screen = pipe->screen;
   struct pipe_surface *surf = NULL;
                     if (svga_screen(screen)->debug.force_surface_view)
            if (surf_tmpl->u.tex.level != 0 &&
      svga_screen(screen)->debug.force_level_surface_view)
         if (pt->target == PIPE_TEXTURE_3D)
            if (svga_have_vgpu10(svga) || svga_screen(screen)->debug.no_surface_view)
                                 }
         /**
   * Create an alternate surface view and clone the resource if specified
   */
   static struct svga_surface *
   create_backed_surface_view(struct svga_context *svga, struct svga_surface *s,
         {
               if (!s->backed) {
               SVGA_STATS_TIME_PUSH(svga_sws(svga),
            backed_view = svga_create_surface_view(&svga->pipe,
                     if (!backed_view)
                        }
   else if (s->backed->handle != tex->handle &&
            /*
   * There is already an existing backing surface, but we still need to
   * sync the backing resource if the original resource has been modified
   * since the last copy.
   */
   struct svga_surface *bs = s->backed;
                     switch (tex->b.target) {
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
      layer = s->base.u.tex.first_layer;
   zslice = 0;
      default:
      layer = 0;
               svga_texture_copy_handle_resource(svga, tex, bs->handle,
                           svga_mark_surface_dirty(&s->backed->base);
                  done:
         }
      /**
   * Create a DX RenderTarget/DepthStencil View for the given surface,
   * if needed.
   */
   struct pipe_surface *
   svga_validate_surface_view(struct svga_context *svga, struct svga_surface *s)
   {
      enum pipe_error ret = PIPE_OK;
            assert(svga_have_vgpu10(svga));
            SVGA_STATS_TIME_PUSH(svga_sws(svga),
            /**
   * DX spec explicitly specifies that no resource can be bound to a render
   * target view and a shader resource view simultaneously.
   * So first check if the resource bound to this surface view collides with
   * a sampler view. If so, then we will clone this surface view and its
   * associated resource. We will then use the cloned surface view for
   * render target.
   */
   for (shader = PIPE_SHADER_VERTEX; shader <= PIPE_SHADER_COMPUTE; shader++) {
      if (svga_check_sampler_view_resource_collision(svga, s->handle, shader)) {
      SVGA_DBG(DEBUG_VIEWS,
                                       /* s may be null here if the function failed */
                  /**
   * Create an alternate surface view for the specified context if the
   * view was created for another context.
   */
   if (s && s->base.context != &svga->pipe) {
               if (s)
               if (s && s->view_id == SVGA3D_INVALID_ID) {
      SVGA3dResourceType resType;
   SVGA3dRenderTargetViewDesc desc;
            if (stex->surface_state < SVGA_SURFACE_STATE_INVALIDATED) {
               /* We are about to render into a surface that has not been validated.
   * First invalidate the surface so that the device does not
   * need to update the host-side copy with the invalid
   * content when the associated mob is first bound to the surface.
   */
   SVGA_RETRY(svga, SVGA3D_InvalidateGBSurface(svga->swc, stex->handle));
               desc.tex.mipSlice = s->real_level;
   desc.tex.firstArraySlice = s->real_layer + s->real_zslice;
   desc.tex.arraySize =
                                 /* Create depth stencil view only if the resource is created
   * with depth stencil bind flag.
   */
   if (stex->key.flags & SVGA3D_SURFACE_BIND_DEPTH_STENCIL) {
      s->view_id = util_bitmask_add(svga->surface_view_id_bm);
   ret = SVGA3D_vgpu10_DefineDepthStencilView(svga->swc,
                                 }
   else {
      /* Create render target view only if the resource is created
   * with render target bind flag.
   */
                     /* Can't create RGBA render target view of a RGBX surface so adjust
   * the view format.  We do something similar for texture samplers in
   * svga_validate_pipe_sampler_view().
   */
   if (view_format == SVGA3D_B8G8R8A8_UNORM &&
      (stex->key.format == SVGA3D_B8G8R8X8_UNORM ||
                     s->view_id = util_bitmask_add(svga->surface_view_id_bm);
   ret = SVGA3D_vgpu10_DefineRenderTargetView(svga->swc,
                                          if (ret != PIPE_OK) {
      util_bitmask_clear(svga->surface_view_id_bm, s->view_id);
   s->view_id = SVGA3D_INVALID_ID;
         }
                  }
            static void
   svga_surface_destroy(struct pipe_context *pipe,
         {
      struct svga_context *svga = svga_context(pipe);
   struct svga_surface *s = svga_surface(surf);
   struct svga_texture *t = svga_texture(surf->texture);
                     /* Destroy the backed view surface if it exists */
   if (s->backed) {
      svga_surface_destroy(pipe, &s->backed->base);
               /* Destroy the surface handle if this is a backed handle and
   * it is not being cached in the texture.
   */
   if (s->handle != t->handle && s->handle != t->backed_handle) {
      SVGA_DBG(DEBUG_DMA, "unref sid %p (tex surface)\n", s->handle);
   svga_screen_surface_destroy(ss, &s->key,
                     if (s->view_id != SVGA3D_INVALID_ID) {
      /* The SVGA3D device will generate a device error if the
   * render target view or depth stencil view is destroyed from
   * a context other than the one it was created with.
   * Similar to shader resource view, in this case, we will skip
   * the destroy for now.
   */
   if (surf->context != pipe) {
         }
   else {
      assert(svga_have_vgpu10(svga));
   if (util_format_is_depth_or_stencil(s->base.format)) {
      SVGA_RETRY(svga, SVGA3D_vgpu10_DestroyDepthStencilView(svga->swc,
      }
   else {
      SVGA_RETRY(svga, SVGA3D_vgpu10_DestroyRenderTargetView(svga->swc,
      }
                  pipe_resource_reference(&surf->texture, NULL);
            svga->hud.num_surface_views--;
      }
         static void
   svga_mark_surface_dirty(struct pipe_surface *surf)
   {
      struct svga_surface *s = svga_surface(surf);
            if (!s->dirty) {
               if (s->handle == tex->handle) {
      /* hmm so 3d textures always have all their slices marked ? */
   svga_define_texture_level(tex, surf->u.tex.first_layer,
      }
   else {
                     /* Increment the view_age and texture age for this surface's mipmap
   * level so that any sampler views into the texture are re-validated too.
   * Note: we age the texture for backed surface view only when the
   *       backed surface is propagated to the original surface.
   */
   if (s->handle == tex->handle)
      }
         void
   svga_mark_surfaces_dirty(struct svga_context *svga)
   {
      unsigned i;
                        /* For VGPU10, mark the dirty bit in the rendertarget/depth stencil view surface.
   * This surface can be the backed surface.
   */
   for (i = 0; i < hw->num_rendertargets; i++) {
      if (hw->rtv[i])
      }
   if (hw->dsv)
      } else {
      for (i = 0; i < svga->curr.framebuffer.nr_cbufs; i++) {
      if (svga->curr.framebuffer.cbufs[i])
      }
   if (svga->curr.framebuffer.zsbuf)
         }
         /**
   * Progagate any changes from surfaces to texture.
   * pipe is optional context to inline the blit command in.
   */
   void
   svga_propagate_surface(struct svga_context *svga, struct pipe_surface *surf,
         {
      struct svga_surface *s = svga_surface(surf);
   struct svga_texture *tex = svga_texture(surf->texture);
            if (!s->dirty)
                     /* Reset the dirty flag if specified. This is to ensure that
   * the dirty flag will not be reset and stay unset when the backing
   * surface is still being bound and rendered to.
   * The reset flag will be set to TRUE when the surface is propagated
   * and will be unbound.
   */
            ss->texture_timestamp++;
            if (s->handle != tex->handle) {
      unsigned zslice, layer;
   unsigned nlayers = 1;
   unsigned i;
   unsigned numMipLevels = tex->b.last_level + 1;
   unsigned srcLevel = s->real_level;
   unsigned dstLevel = surf->u.tex.level;
   unsigned width = u_minify(tex->b.width0, dstLevel);
            if (surf->texture->target == PIPE_TEXTURE_CUBE) {
      zslice = 0;
      }
   else if (surf->texture->target == PIPE_TEXTURE_1D_ARRAY ||
            surf->texture->target == PIPE_TEXTURE_2D_ARRAY ||
   zslice = 0;
   layer = surf->u.tex.first_layer;
      }
   else {
      zslice = surf->u.tex.first_layer;
               SVGA_DBG(DEBUG_VIEWS,
                  if (svga_have_vgpu10(svga)) {
               for (i = 0; i < nlayers; i++) {
                     svga_texture_copy_region(svga,
                           }
   else {
      for (i = 0; i < nlayers; i++) {
      svga_texture_copy_handle(svga,
                                                   /* Sync the surface view age with the texture age */
            /* If this backed surface is cached in the texture,
   * update the backed age as well.
   */
   if (tex->backed_handle == s->handle) {
                        }
         /**
   * If any of the render targets are in backing texture views, propagate any
   * changes to them back to the original texture.
   */
   void
   svga_propagate_rendertargets(struct svga_context *svga)
   {
               /* Early exit if there is no backing texture views in use */
   if (!svga->state.hw_draw.has_backed_views)
            /* Note that we examine the svga->state.hw_draw.framebuffer surfaces,
   * not the svga->curr.framebuffer surfaces, because it's the former
   * surfaces which may be backing surface views (the actual render targets).
   */
   for (i = 0; i < svga->state.hw_clear.num_rendertargets; i++) {
      struct pipe_surface *s = svga->state.hw_clear.rtv[i];
   if (s) {
                     if (svga->state.hw_clear.dsv) {
            }
         /**
   * Check if we should call svga_propagate_surface on the surface.
   */
   bool
   svga_surface_needs_propagation(const struct pipe_surface *surf)
   {
      const struct svga_surface *s = svga_surface_const(surf);
               }
         static void
   svga_get_sample_position(struct pipe_context *context,
               {
      /* We can't actually query the device to learn the sample positions.
   * These were grabbed from nvidia's driver.
   */
   static const float pos1[1][2] = {
         };
   static const float pos2[2][2] = {
      { 0.75, 0.75 },
      };
   static const float pos4[4][2] = {
      { 0.375000, 0.125000 },
   { 0.875000, 0.375000 },
   { 0.125000, 0.625000 },
      };
   static const float pos8[8][2] = {
      { 0.562500, 0.312500 },
   { 0.437500, 0.687500 },
   { 0.812500, 0.562500 },
   { 0.312500, 0.187500 },
   { 0.187500, 0.812500 },
   { 0.062500, 0.437500 },
   { 0.687500, 0.937500 },
      };
   static const float pos16[16][2] = {
      { 0.187500, 0.062500 },
   { 0.437500, 0.187500 },
   { 0.062500, 0.312500 },
   { 0.312500, 0.437500 },
   { 0.687500, 0.062500 },
   { 0.937500, 0.187500 },
   { 0.562500, 0.312500 },
   { 0.812500, 0.437500 },
   { 0.187500, 0.562500 },
   { 0.437500, 0.687500 },
   { 0.062500, 0.812500 },
   { 0.312500, 0.937500 },
   { 0.687500, 0.562500 },
   { 0.937500, 0.687500 },
   { 0.562500, 0.812500 },
      };
            switch (sample_count) {
   case 2:
      positions = pos2;
      case 4:
      positions = pos4;
      case 8:
      positions = pos8;
      case 16:
      positions = pos16;
      default:
                  pos_out[0] = positions[sample_index][0];
      }
         void
   svga_init_surface_functions(struct svga_context *svga)
   {
      svga->pipe.create_surface = svga_create_surface;
   svga->pipe.surface_destroy = svga_surface_destroy;
      }
