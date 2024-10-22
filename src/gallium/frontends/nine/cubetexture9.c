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
      #include "c99_alloca.h"
      #include "device9.h"
   #include "cubetexture9.h"
   #include "nine_memory_helper.h"
   #include "nine_helpers.h"
   #include "nine_pipe.h"
      #define DBG_CHANNEL DBG_CUBETEXTURE
         static HRESULT
   NineCubeTexture9_ctor( struct NineCubeTexture9 *This,
                        struct NineUnknownParams *pParams,
   UINT EdgeLength, UINT Levels,
      {
      struct pipe_resource *info = &This->base.base.info;
   struct pipe_screen *screen = pParams->device->screen;
   enum pipe_format pf;
   unsigned i, l, f, offset, face_size = 0;
   unsigned *level_offsets = NULL;
   D3DSURFACE_DESC sfdesc;
   struct nine_allocation *p;
            DBG("This=%p pParams=%p EdgeLength=%u Levels=%u Usage=%d "
      "Format=%d Pool=%d pSharedHandle=%p\n",
   This, pParams, EdgeLength, Levels, Usage,
                           /* user_assert(!pSharedHandle || Pool == D3DPOOL_DEFAULT, D3DERR_INVALIDCALL); */
            user_assert(!(Usage & D3DUSAGE_AUTOGENMIPMAP) ||
            if (Usage & D3DUSAGE_AUTOGENMIPMAP)
            pf = d3d9_to_pipe_format_checked(screen, Format, PIPE_TEXTURE_CUBE, 0,
                  if (pf == PIPE_FORMAT_NONE)
            if (compressed_format(Format)) {
      const unsigned w = util_format_get_blockwidth(pf);
                        info->screen = pParams->device->screen;
   info->target = PIPE_TEXTURE_CUBE;
   info->format = pf;
   info->width0 = EdgeLength;
   info->height0 = EdgeLength;
   info->depth0 = 1;
   if (Levels)
         else
         info->array_size = 6;
   info->nr_samples = 0;
   info->nr_storage_samples = 0;
   info->bind = PIPE_BIND_SAMPLER_VIEW;
   info->usage = PIPE_USAGE_DEFAULT;
            if (Usage & D3DUSAGE_RENDERTARGET)
         if (Usage & D3DUSAGE_DEPTHSTENCIL)
            if (Usage & D3DUSAGE_DYNAMIC) {
         }
   if (Usage & D3DUSAGE_SOFTWAREPROCESSING)
      DBG("Application asked for Software Vertex Processing, "
         hr = NineBaseTexture9_ctor(&This->base, pParams, NULL, D3DRTYPE_CUBETEXTURE,
         if (FAILED(hr))
                  if (Pool != D3DPOOL_DEFAULT) {
      level_offsets = alloca(sizeof(unsigned) * This->base.level_count);
   face_size = nine_format_get_size_and_offsets(pf, level_offsets,
               This->managed_buffer = nine_allocate(pParams->device->allocator, 6 * face_size);
   if (!This->managed_buffer)
               This->surfaces = CALLOC(6 * This->base.level_count, sizeof(*This->surfaces));
   if (!This->surfaces)
            /* Create all the surfaces right away.
   * They manage backing storage, and transfers (LockRect) are deferred
   * to them.
   */
   sfdesc.Format = Format;
   sfdesc.Type = D3DRTYPE_SURFACE;
   sfdesc.Usage = Usage;
   sfdesc.Pool = Pool;
   sfdesc.MultiSampleType = D3DMULTISAMPLE_NONE;
   sfdesc.MultiSampleQuality = 0;
   /* We allocate the memory for the surfaces as continous blocks.
   * This is the expected behaviour, however we haven't tested for
   * cube textures in which order the faces/levels should be in memory
   */
   for (f = 0; f < 6; f++) {
      offset = f * face_size;
   for (l = 0; l < This->base.level_count; l++) {
         sfdesc.Width = sfdesc.Height = u_minify(EdgeLength, l);
                  hr = NineSurface9_new(This->base.base.base.device, NineUnknown(This),
               if (FAILED(hr))
               for (i = 0; i < 6; ++i) {
      /* Textures start initially dirty */
   This->dirty_rect[i].width = EdgeLength;
   This->dirty_rect[i].height = EdgeLength;
                  }
      static void
   NineCubeTexture9_dtor( struct NineCubeTexture9 *This )
   {
      unsigned i;
                     if (This->surfaces) {
      for (i = 0; i < This->base.level_count * 6; ++i)
         if (This->surfaces[i])
               if (This->managed_buffer) {
      if (is_worker)
         else
                  }
      HRESULT NINE_WINAPI
   NineCubeTexture9_GetLevelDesc( struct NineCubeTexture9 *This,
               {
                                    }
      HRESULT NINE_WINAPI
   NineCubeTexture9_GetCubeMapSurface( struct NineCubeTexture9 *This,
                     {
               DBG("This=%p FaceType=%d Level=%u ppCubeMapSurface=%p\n",
            user_assert(Level < This->base.level_count, D3DERR_INVALIDCALL);
            NineUnknown_AddRef(NineUnknown(This->surfaces[s]));
               }
      HRESULT NINE_WINAPI
   NineCubeTexture9_LockRect( struct NineCubeTexture9 *This,
                                 {
               DBG("This=%p FaceType=%d Level=%u pLockedRect=%p pRect=%p Flags=%d\n",
            user_assert(Level < This->base.level_count, D3DERR_INVALIDCALL);
               }
      HRESULT NINE_WINAPI
   NineCubeTexture9_UnlockRect( struct NineCubeTexture9 *This,
               {
                        user_assert(Level < This->base.level_count, D3DERR_INVALIDCALL);
               }
      HRESULT NINE_WINAPI
   NineCubeTexture9_AddDirtyRect( struct NineCubeTexture9 *This,
               {
                        if (This->base.base.pool != D3DPOOL_MANAGED) {
      if (This->base.base.usage & D3DUSAGE_AUTOGENMIPMAP) {
         This->base.dirty_mip = true;
   }
               if (This->base.base.pool == D3DPOOL_MANAGED) {
      This->base.managed.dirty = true;
               if (!pDirtyRect) {
      u_box_origin_2d(This->base.base.info.width0,
            } else {
      if (This->dirty_rect[FaceType].width == 0) {
         } else {
         struct pipe_box box;
   rect_to_pipe_box_clamp(&box, pDirtyRect);
   u_box_union_2d(&This->dirty_rect[FaceType], &This->dirty_rect[FaceType],
   }
   (void) u_box_clip_2d(&This->dirty_rect[FaceType],
                  }
      }
      IDirect3DCubeTexture9Vtbl NineCubeTexture9_vtable = {
      (void *)NineUnknown_QueryInterface,
   (void *)NineUnknown_AddRef,
   (void *)NineUnknown_Release,
   (void *)NineUnknown_GetDevice, /* actually part of Resource9 iface */
   (void *)NineUnknown_SetPrivateData,
   (void *)NineUnknown_GetPrivateData,
   (void *)NineUnknown_FreePrivateData,
   (void *)NineResource9_SetPriority,
   (void *)NineResource9_GetPriority,
   (void *)NineBaseTexture9_PreLoad,
   (void *)NineResource9_GetType,
   (void *)NineBaseTexture9_SetLOD,
   (void *)NineBaseTexture9_GetLOD,
   (void *)NineBaseTexture9_GetLevelCount,
   (void *)NineBaseTexture9_SetAutoGenFilterType,
   (void *)NineBaseTexture9_GetAutoGenFilterType,
   (void *)NineBaseTexture9_GenerateMipSubLevels,
   (void *)NineCubeTexture9_GetLevelDesc,
   (void *)NineCubeTexture9_GetCubeMapSurface,
   (void *)NineCubeTexture9_LockRect,
   (void *)NineCubeTexture9_UnlockRect,
      };
      static const GUID *NineCubeTexture9_IIDs[] = {
      &IID_IDirect3DCubeTexture9,
   &IID_IDirect3DBaseTexture9,
   &IID_IDirect3DResource9,
   &IID_IUnknown,
      };
      HRESULT
   NineCubeTexture9_new( struct NineDevice9 *pDevice,
                        UINT EdgeLength, UINT Levels,
   DWORD Usage,
      {
      NINE_DEVICE_CHILD_NEW(CubeTexture9, ppOut, pDevice,
            }
