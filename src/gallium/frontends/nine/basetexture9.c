   /*
   * Copyright 2011 Joakim Sindholt <opensource@zhasha.com>
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
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      #include "basetexture9.h"
   #include "device9.h"
      /* For UploadSelf: */
   #include "texture9.h"
   #include "cubetexture9.h"
   #include "volumetexture9.h"
   #include "nine_pipe.h"
      #if defined(DEBUG) || !defined(NDEBUG)
   #include "nine_dump.h"
   #endif
      #include "util/format/u_format.h"
      #define DBG_CHANNEL DBG_BASETEXTURE
      HRESULT
   NineBaseTexture9_ctor( struct NineBaseTexture9 *This,
                        struct NineUnknownParams *pParams,
   struct pipe_resource *initResource,
      {
      BOOL alloc = (Pool == D3DPOOL_DEFAULT) && !initResource &&
                  DBG("This=%p, pParams=%p initResource=%p Type=%d format=%d Pool=%d Usage=%d\n",
            user_assert(!(Usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)) ||
         user_assert(!(Usage & D3DUSAGE_DYNAMIC) ||
                  hr = NineResource9_ctor(&This->base, pParams, initResource, alloc, Type, Pool, Usage);
   if (FAILED(hr))
            This->format = format;
   This->mipfilter = (Usage & D3DUSAGE_AUTOGENMIPMAP) ?
         /* In the case of D3DUSAGE_AUTOGENMIPMAP, only the first level is accessible,
   * and thus needs a surface created. */
   This->level_count = (Usage & D3DUSAGE_AUTOGENMIPMAP) ? 1 : (This->base.info.last_level+1);
   This->managed.lod = 0;
   This->managed.lod_resident = -1;
   /* Mark the texture as dirty to trigger first upload when we need the texture,
   * even if it wasn't set by the application */
   if (Pool == D3DPOOL_MANAGED)
         /* When a depth buffer is sampled, it is for shadow mapping, except for
   * D3DFMT_INTZ, D3DFMT_DF16 and D3DFMT_DF24.
   * In addition D3DFMT_INTZ can be used for both texturing and depth buffering
   * if z write is disabled. This particular feature may not work for us in
   * practice because OGL doesn't have that. However apparently it is known
   * some cards have performance issues with this feature, so real apps
   * shouldn't use it. */
   This->shadow = (This->format != D3DFMT_INTZ && This->format != D3DFMT_DF16 &&
                        list_inithead(&This->list);
   list_inithead(&This->list2);
   if (Pool == D3DPOOL_MANAGED)
               }
      void
   NineBaseTexture9_dtor( struct NineBaseTexture9 *This )
   {
               pipe_sampler_view_reference(&This->view[0], NULL);
            if (list_is_linked(&This->list))
         if (list_is_linked(&This->list2))
               }
      DWORD NINE_WINAPI
   NineBaseTexture9_SetLOD( struct NineBaseTexture9 *This,
         {
                                          if (This->managed.lod != old && This->bind_count && list_is_empty(&This->list))
               }
      DWORD NINE_WINAPI
   NineBaseTexture9_GetLOD( struct NineBaseTexture9 *This )
   {
                  }
      DWORD NINE_WINAPI
   NineBaseTexture9_GetLevelCount( struct NineBaseTexture9 *This )
   {
                  }
      HRESULT NINE_WINAPI
   NineBaseTexture9_SetAutoGenFilterType( struct NineBaseTexture9 *This,
         {
               if (!(This->base.usage & D3DUSAGE_AUTOGENMIPMAP))
                  This->mipfilter = FilterType;
   This->dirty_mip = true;
               }
      D3DTEXTUREFILTERTYPE NINE_WINAPI
   NineBaseTexture9_GetAutoGenFilterType( struct NineBaseTexture9 *This )
   {
                  }
      HRESULT
   NineBaseTexture9_UploadSelf( struct NineBaseTexture9 *This )
   {
      HRESULT hr;
   unsigned l, min_level_dirty = This->managed.lod;
            DBG("This=%p dirty=%i type=%s\n", This, This->managed.dirty,
                     update_lod = This->managed.lod_resident != This->managed.lod;
   if (!update_lod && !This->managed.dirty)
            /* Allocate a new resource with the correct number of levels,
   * Mark states for update, and tell the nine surfaces/volumes
   * their new resource. */
   if (update_lod) {
                        pipe_sampler_view_reference(&This->view[0], NULL);
            /* Allocate a new resource */
   hr = NineBaseTexture9_CreatePipeResource(This, This->managed.lod_resident != -1);
   if (FAILED(hr))
                  if (This->managed.lod_resident == -1) {/* no levels were resident */
         This->managed.dirty = false; /* We are going to upload everything. */
            if (This->base.type == D3DRTYPE_TEXTURE) {
                  /* last content (if apply) has been copied to the new resource.
   * Note: We cannot render to surfaces of managed textures.
   * Note2: the level argument passed is to get the level offset
   * right when the texture is uploaded (the texture first level
   * corresponds to This->managed.lod).
   * Note3: We don't care about the value passed for the surfaces
   * before This->managed.lod, negative with this implementation. */
   for (l = 0; l < This->level_count; ++l)
   } else
   if (This->base.type == D3DRTYPE_CUBETEXTURE) {
                        for (l = 0; l < This->level_count; ++l) {
      for (z = 0; z < 6; ++z)
      NineSurface9_SetResource(tex->surfaces[l * 6 + z],
   } else
   if (This->base.type == D3DRTYPE_VOLUMETEXTURE) {
                  for (l = 0; l < This->level_count; ++l)
   } else {
                  /* We are going to fully upload the new levels,
                     /* Update dirty parts of the texture */
   if (This->managed.dirty) {
      if (This->base.type == D3DRTYPE_TEXTURE) {
         struct NineTexture9 *tex = NineTexture9(This);
   struct pipe_box box;
                  DBG("TEXTURE: dirty rect=(%u,%u) (%ux%u)\n",
                  /* Note: for l < min_level_dirty, the resource is
   * either non-existing (and thus will be entirely re-uploaded
   * if the lod changes) or going to have a full upload */
   if (tex->dirty_rect.width) {
      for (l = min_level_dirty; l < This->level_count; ++l) {
      u_box_minify_2d(&box, &tex->dirty_rect, l);
      }
   memset(&tex->dirty_rect, 0, sizeof(tex->dirty_rect));
      } else
   if (This->base.type == D3DRTYPE_CUBETEXTURE) {
         struct NineCubeTexture9 *tex = NineCubeTexture9(This);
   unsigned z;
   struct pipe_box box;
                  for (z = 0; z < 6; ++z) {
                           if (tex->dirty_rect[z].width) {
      for (l = min_level_dirty; l < This->level_count; ++l) {
         u_box_minify_2d(&box, &tex->dirty_rect[z], l);
   }
   memset(&tex->dirty_rect[z], 0, sizeof(tex->dirty_rect[z]));
         } else
   if (This->base.type == D3DRTYPE_VOLUMETEXTURE) {
                        DBG("VOLUME: dirty_box=(%u,%u,%u) (%ux%ux%u)\n",
                  if (tex->dirty_box.width) {
      for (l = min_level_dirty; l < This->level_count; ++l) {
      u_box_minify_3d(&box, &tex->dirty_box, l);
      }
      } else {
         }
               /* Upload the new levels */
   if (update_lod) {
      if (This->base.type == D3DRTYPE_TEXTURE) {
                        box.x = box.y = box.z = 0;
   box.depth = 1;
   for (l = This->managed.lod; l < This->managed.lod_resident; ++l) {
      box.width = u_minify(This->base.info.width0, l);
   box.height = u_minify(This->base.info.height0, l);
      } else
   if (This->base.type == D3DRTYPE_CUBETEXTURE) {
         struct NineCubeTexture9 *tex = NineCubeTexture9(This);
                  box.x = box.y = box.z = 0;
   box.depth = 1;
   for (l = This->managed.lod; l < This->managed.lod_resident; ++l) {
      box.width = u_minify(This->base.info.width0, l);
   box.height = u_minify(This->base.info.height0, l);
   for (z = 0; z < 6; ++z)
      } else
   if (This->base.type == D3DRTYPE_VOLUMETEXTURE) {
                        box.x = box.y = box.z = 0;
   for (l = This->managed.lod; l < This->managed.lod_resident; ++l) {
      box.width = u_minify(This->base.info.width0, l);
   box.height = u_minify(This->base.info.height0, l);
   box.depth = u_minify(This->base.info.depth0, l);
      } else {
                              if (This->base.usage & D3DUSAGE_AUTOGENMIPMAP)
            /* Set again the textures currently bound to update the texture data */
   if (This->bind_count) {
      struct nine_state *state = &This->base.base.device->state;
   unsigned s;
   for (s = 0; s < NINE_MAX_SAMPLERS; ++s)
         /* Dirty tracking is done in device9 state, not nine_context. */
               DBG("DONE, generate mip maps = %i\n", This->dirty_mip);
      }
      void NINE_WINAPI
   NineBaseTexture9_GenerateMipSubLevels( struct NineBaseTexture9 *This )
   {
      unsigned base_level = 0;
   unsigned last_level = This->base.info.last_level - This->managed.lod;
   unsigned first_layer = 0;
   unsigned last_layer;
   unsigned filter = This->mipfilter == D3DTEXF_POINT ? PIPE_TEX_FILTER_NEAREST
                  if (This->base.pool == D3DPOOL_MANAGED)
         if (!This->dirty_mip)
         if (This->managed.lod) {
      ERR("AUTOGENMIPMAP if level 0 is not resident not supported yet !\n");
               if (!This->view[0])
                     nine_context_gen_mipmap(This->base.base.device, (struct NineUnknown *)This,
                           }
      HRESULT
   NineBaseTexture9_CreatePipeResource( struct NineBaseTexture9 *This,
         {
      struct pipe_context *pipe;
   struct pipe_screen *screen = This->base.info.screen;
   struct pipe_resource templ;
   unsigned l, m;
   struct pipe_resource *res;
            DBG("This=%p lod=%u last_level=%u\n", This,
                              if (This->managed.lod) {
      templ.width0 = u_minify(templ.width0, This->managed.lod);
   templ.height0 = u_minify(templ.height0, This->managed.lod);
      }
            if (old) {
      /* LOD might have changed. */
   if (old->width0 == templ.width0 &&
         old->height0 == templ.height0 &&
               res = nine_resource_create_with_retry(This->base.base.device, screen, &templ);
   if (!res)
                  if (old && CopyData) { /* Don't return without releasing old ! */
      struct pipe_box box;
   box.x = 0;
   box.y = 0;
            l = (This->managed.lod < This->managed.lod_resident) ? This->managed.lod_resident - This->managed.lod : 0;
            box.width = u_minify(templ.width0, l);
   box.height = u_minify(templ.height0, l);
                     for (; l <= templ.last_level; ++l, ++m) {
         pipe->resource_copy_region(pipe,
               box.width = u_minify(box.width, 1);
   box.height = u_minify(box.height, 1);
               }
               }
      #define SWIZZLE_TO_REPLACE(s) (s == PIPE_SWIZZLE_0 || \
                  HRESULT
   NineBaseTexture9_UpdateSamplerView( struct NineBaseTexture9 *This,
         {
      const struct util_format_description *desc;
   struct pipe_context *pipe;
   struct pipe_screen *screen = NineDevice9_GetScreen(This->base.base.device);
   struct pipe_resource *resource = This->base.resource;
   struct pipe_sampler_view templ;
   enum pipe_format srgb_format;
   unsigned i;
   uint8_t swizzle[4];
                        if (unlikely(This->format == D3DFMT_NULL))
                  }
                     swizzle[0] = PIPE_SWIZZLE_X;
   swizzle[1] = PIPE_SWIZZLE_Y;
   swizzle[2] = PIPE_SWIZZLE_Z;
   swizzle[3] = PIPE_SWIZZLE_W;
   desc = util_format_description(resource->format);
   if (desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
      /* msdn doc is incomplete here and wrong.
      * The only formats that can be read directly here
   * are DF16, DF24 and INTZ.
   * Tested on win the swizzle is
   * R = depth, G = B = 0, A = 1 for DF16 and DF24
   * R = G = B = A = depth for INTZ
   * For the other ZS formats that can't be read directly
   * but can be used as shadow map, the result is duplicated on
      if (This->format == D3DFMT_DF16 ||
         This->format == D3DFMT_DF24) {
   swizzle[1] = PIPE_SWIZZLE_0;
   swizzle[2] = PIPE_SWIZZLE_0;
   } else {
         swizzle[1] = PIPE_SWIZZLE_X;
   swizzle[2] = PIPE_SWIZZLE_X;
      } else if (resource->format == PIPE_FORMAT_RGTC2_UNORM) {
      swizzle[0] = PIPE_SWIZZLE_Y;
   swizzle[1] = PIPE_SWIZZLE_X;
   swizzle[2] = PIPE_SWIZZLE_1;
      } else if (resource->format != PIPE_FORMAT_A8_UNORM &&
            /* exceptions:
      * A8 should have 0.0 as default values for RGB.
   * ATI1/RGTC1 should be r 0 0 1 (tested on windows).
   * It is already what gallium does. All the other ones
      for (i = 0; i < 4; i++) {
         if (SWIZZLE_TO_REPLACE(desc->swizzle[i]))
               /* if requested and supported, convert to the sRGB format */
   srgb_format = util_format_srgb(resource->format);
   if (sRGB && srgb_format != PIPE_FORMAT_NONE &&
      screen->is_format_supported(screen, srgb_format,
            else
         templ.u.tex.first_layer = 0;
   templ.u.tex.last_layer = resource->target == PIPE_TEXTURE_3D ?
         templ.u.tex.first_level = 0;
   templ.u.tex.last_level = resource->last_level;
   templ.swizzle_r = swizzle[0];
   templ.swizzle_g = swizzle[1];
   templ.swizzle_b = swizzle[2];
   templ.swizzle_a = swizzle[3];
            pipe = nine_context_get_pipe_acquire(This->base.base.device);
   This->view[sRGB] = pipe->create_sampler_view(pipe, resource, &templ);
                        }
      void NINE_WINAPI
   NineBaseTexture9_PreLoad( struct NineBaseTexture9 *This )
   {
               if (This->base.pool == D3DPOOL_MANAGED)
      }
      void
   NineBaseTexture9_UnLoad( struct NineBaseTexture9 *This )
   {
               if (This->base.pool != D3DPOOL_MANAGED ||
      This->managed.lod_resident == -1)
         DBG("This=%p, releasing resource\n", This);
   pipe_resource_reference(&This->base.resource, NULL);
   This->managed.lod_resident = -1;
            /* If the texture is bound, we have to re-upload it */
      }
      #if defined(DEBUG) || !defined(NDEBUG)
   void
   NineBaseTexture9_Dump( struct NineBaseTexture9 *This )
   {
      DBG("\nNineBaseTexture9(%p->NULL/%p): Pool=%s Type=%s Usage=%s\n"
      "Format=%s Dims=%ux%ux%u/%u LastLevel=%u Lod=%u(%u)\n", This,
   This->base.resource,
   nine_D3DPOOL_to_str(This->base.pool),
   nine_D3DRTYPE_to_str(This->base.type),
   nine_D3DUSAGE_to_str(This->base.usage),
   d3dformat_to_string(This->format),
   This->base.info.width0, This->base.info.height0, This->base.info.depth0,
   This->base.info.array_size, This->base.info.last_level,
   }
   #endif /* DEBUG || !NDEBUG */
