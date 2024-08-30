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
   #include "surface9.h"
   #include "texture9.h"
   #include "nine_helpers.h"
   #include "nine_memory_helper.h"
   #include "nine_pipe.h"
   #include "nine_dump.h"
      #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/u_inlines.h"
   #include "util/u_resource.h"
      #define DBG_CHANNEL DBG_TEXTURE
      static HRESULT
   NineTexture9_ctor( struct NineTexture9 *This,
                     struct NineUnknownParams *pParams,
   UINT Width, UINT Height, UINT Levels,
   DWORD Usage,
   {
      struct pipe_screen *screen = pParams->device->screen;
   struct pipe_resource *info = &This->base.base.info;
   enum pipe_format pf;
   unsigned *level_offsets = NULL;
   unsigned l;
   D3DSURFACE_DESC sfdesc;
   HRESULT hr;
                     DBG("(%p) Width=%u Height=%u Levels=%u Usage=%s Format=%s Pool=%s "
      "pSharedHandle=%p\n", This, Width, Height, Levels,
   nine_D3DUSAGE_to_str(Usage),
                  /* pSharedHandle: can be non-null for ex only.
   * D3DPOOL_SYSTEMMEM: Levels must be 1
   * D3DPOOL_DEFAULT: no restriction for Levels
   * Other Pools are forbidden. */
   user_assert(!pSharedHandle || pParams->device->ex, D3DERR_INVALIDCALL);
   user_assert(!pSharedHandle ||
                  user_assert(!(Usage & D3DUSAGE_AUTOGENMIPMAP) ||
                  /* TODO: implement pSharedHandle for D3DPOOL_DEFAULT (cross process
   * buffer sharing).
   *
   * Gem names may have fit but they're depreciated and won't work on render-nodes.
   * One solution is to use shm buffers. We would use a /dev/shm file, fill the first
   * values to tell it is a nine buffer, the size, which function created it, etc,
   * and then it would contain the data. The handle would be a number, corresponding to
   * the file to read (/dev/shm/nine-share-4 for example would be 4).
   *
   * Wine just ignores the argument, which works only if the app creates the handle
   * and won't use it. Instead of failing, we support that situation by putting an
   * invalid handle, that we would fail to import. Please note that we don't advertise
   * the flag indicating the support for that feature, but apps seem to not care.
            if (pSharedHandle && Pool == D3DPOOL_DEFAULT) {
      if (!*pSharedHandle) {
         DBG("Creating Texture with invalid handle. Importing will fail\n.");
   *pSharedHandle = (HANDLE)1; /* Wine would keep it NULL */
   } else {
         ERR("Application tries to use cross-process sharing feature. Nine "
                     if (Usage & D3DUSAGE_AUTOGENMIPMAP)
            pf = d3d9_to_pipe_format_checked(screen, Format, PIPE_TEXTURE_2D, 0,
                  if (Format != D3DFMT_NULL && pf == PIPE_FORMAT_NONE)
            if (compressed_format(Format)) {
      const unsigned w = util_format_get_blockwidth(pf);
                        info->screen = screen;
   info->target = PIPE_TEXTURE_2D;
   info->format = pf;
   info->width0 = Width;
   info->height0 = Height;
   info->depth0 = 1;
   if (Levels)
         else
         info->array_size = 1;
   info->nr_samples = 0;
   info->nr_storage_samples = 0;
   info->bind = PIPE_BIND_SAMPLER_VIEW;
   info->usage = PIPE_USAGE_DEFAULT;
            if (Usage & D3DUSAGE_RENDERTARGET)
         if (Usage & D3DUSAGE_DEPTHSTENCIL)
            if (Usage & D3DUSAGE_DYNAMIC) {
                  if (Usage & D3DUSAGE_SOFTWAREPROCESSING)
      DBG("Application asked for Software Vertex Processing, "
         hr = NineBaseTexture9_ctor(&This->base, pParams, NULL, D3DRTYPE_TEXTURE, Format, Pool, Usage);
   if (FAILED(hr))
                  if (pSharedHandle && *pSharedHandle) { /* Pool == D3DPOOL_SYSTEMMEM */
      user_buffer = nine_wrap_external_pointer(pParams->device->allocator, (void *)*pSharedHandle);
   level_offsets = alloca(sizeof(unsigned) * This->base.level_count);
   (void) nine_format_get_size_and_offsets(pf, level_offsets,
            } else if (Pool != D3DPOOL_DEFAULT) {
      level_offsets = alloca(sizeof(unsigned) * This->base.level_count);
   user_buffer = nine_allocate(pParams->device->allocator,
         nine_format_get_size_and_offsets(pf, level_offsets,
         This->managed_buffer = user_buffer;
   if (!This->managed_buffer)
               This->surfaces = CALLOC(This->base.level_count, sizeof(*This->surfaces));
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
            for (l = 0; l < This->base.level_count; ++l) {
      sfdesc.Width = u_minify(Width, l);
   sfdesc.Height = u_minify(Height, l);
   /* Some apps expect the memory to be allocated in
         user_buffer_for_level = user_buffer ?
            hr = NineSurface9_new(This->base.base.base.device, NineUnknown(This),
                     if (FAILED(hr))
               /* Textures start initially dirty */
   This->dirty_rect.width = Width;
   This->dirty_rect.height = Height;
            if (pSharedHandle && !*pSharedHandle) {/* Pool == D3DPOOL_SYSTEMMEM */
                     }
      static void
   NineTexture9_dtor( struct NineTexture9 *This )
   {
      bool is_worker = nine_context_is_worker(This->base.base.base.device);
                     if (This->surfaces) {
      /* The surfaces should have 0 references and be unbound now. */
   for (l = 0; l < This->base.level_count; ++l)
         if (This->surfaces[l])
               if (This->managed_buffer) {
      if (is_worker)
         else
                  }
      HRESULT NINE_WINAPI
   NineTexture9_GetLevelDesc( struct NineTexture9 *This,
               {
               user_assert(Level < This->base.level_count, D3DERR_INVALIDCALL);
                        }
      HRESULT NINE_WINAPI
   NineTexture9_GetSurfaceLevel( struct NineTexture9 *This,
               {
               user_assert(Level < This->base.level_count, D3DERR_INVALIDCALL);
            NineUnknown_AddRef(NineUnknown(This->surfaces[Level]));
               }
      HRESULT NINE_WINAPI
   NineTexture9_LockRect( struct NineTexture9 *This,
                           {
      DBG("This=%p Level=%u pLockedRect=%p pRect=%p Flags=%d\n",
                     return NineSurface9_LockRect(This->surfaces[Level], pLockedRect,
      }
      HRESULT NINE_WINAPI
   NineTexture9_UnlockRect( struct NineTexture9 *This,
         {
                           }
      HRESULT NINE_WINAPI
   NineTexture9_AddDirtyRect( struct NineTexture9 *This,
         {
      DBG("This=%p pDirtyRect=%p[(%u,%u)-(%u,%u)]\n", This, pDirtyRect,
      pDirtyRect ? pDirtyRect->left : 0, pDirtyRect ? pDirtyRect->top : 0,
         /* Tracking dirty regions on DEFAULT resources is pointless,
   * because we always write to the final storage. Just marked it dirty in
   * case we need to generate mip maps.
   */
   if (This->base.base.pool == D3DPOOL_DEFAULT) {
      if (This->base.base.usage & D3DUSAGE_AUTOGENMIPMAP) {
         This->base.dirty_mip = true;
   }
               if (This->base.base.pool == D3DPOOL_MANAGED) {
      This->base.managed.dirty = true;
               if (!pDirtyRect) {
      u_box_origin_2d(This->base.base.info.width0,
      } else {
      if (This->dirty_rect.width == 0) {
         } else {
         struct pipe_box box;
   rect_to_pipe_box_clamp(&box, pDirtyRect);
   }
   (void) u_box_clip_2d(&This->dirty_rect, &This->dirty_rect,
            }
      }
      IDirect3DTexture9Vtbl NineTexture9_vtable = {
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
   (void *)NineTexture9_GetLevelDesc,
   (void *)NineTexture9_GetSurfaceLevel,
   (void *)NineTexture9_LockRect,
   (void *)NineTexture9_UnlockRect,
      };
      static const GUID *NineTexture9_IIDs[] = {
      &IID_IDirect3DTexture9,
   &IID_IDirect3DBaseTexture9,
   &IID_IDirect3DResource9,
   &IID_IUnknown,
      };
      HRESULT
   NineTexture9_new( struct NineDevice9 *pDevice,
                     UINT Width, UINT Height, UINT Levels,
   DWORD Usage,
   D3DFORMAT Format,
   {
      NINE_DEVICE_CHILD_NEW(Texture9, ppOut, pDevice,
            }
