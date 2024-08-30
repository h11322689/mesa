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
      #include "iunknown.h"
   #include "surface9.h"
   #include "device9.h"
      /* for marking dirty */
   #include "basetexture9.h"
   #include "texture9.h"
   #include "cubetexture9.h"
      #include "nine_helpers.h"
   #include "nine_pipe.h"
   #include "nine_dump.h"
   #include "nine_memory_helper.h"
   #include "nine_state.h"
      #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
      #include "util/u_math.h"
   #include "util/u_inlines.h"
   #include "util/u_surface.h"
      #define DBG_CHANNEL DBG_SURFACE
      static void
   NineSurface9_CreatePipeSurfaces( struct NineSurface9 *This );
      HRESULT
   NineSurface9_ctor( struct NineSurface9 *This,
                     struct NineUnknownParams *pParams,
   struct NineUnknown *pContainer,
   struct pipe_resource *pResource,
   struct nine_allocation *user_buffer,
   uint8_t TextureType,
   {
      HRESULT hr;
   bool allocate = !pContainer && pDesc->Format != D3DFMT_NULL;
            DBG("This=%p pDevice=%p pResource=%p Level=%u Layer=%u pDesc=%p\n",
            /* Mark this as a special surface held by another internal resource. */
   pParams->container = pContainer;
   This->base.base.device = pParams->device; /* Early fill this field in case of failure */
   /* Make sure there's a Desc */
            assert(allocate || pResource || user_buffer ||
         assert(!allocate || (!pResource && !user_buffer));
   assert(!pResource || !user_buffer);
   assert(!user_buffer || pDesc->Pool != D3DPOOL_DEFAULT);
   assert(!pResource || pDesc->Pool == D3DPOOL_DEFAULT);
   /* Allocation only from create_zs_or_rt_surface with params 0 0 0 */
                              /* Map MultiSampleQuality to MultiSampleType */
   hr = d3dmultisample_type_check(pParams->device->screen,
                           if (FAILED(hr)) {
                  /* TODO: this is (except width and height) duplicate from
   * container info (in the pContainer case). Some refactoring is
   * needed to avoid duplication */
   This->base.info.screen = pParams->device->screen;
   This->base.info.target = PIPE_TEXTURE_2D;
   This->base.info.width0 = pDesc->Width;
   This->base.info.height0 = pDesc->Height;
   This->base.info.depth0 = 1;
   This->base.info.last_level = 0;
   This->base.info.array_size = 1;
   This->base.info.nr_samples = multisample_type;
   This->base.info.nr_storage_samples = multisample_type;
   This->base.info.usage = PIPE_USAGE_DEFAULT;
            if (pDesc->Usage & D3DUSAGE_RENDERTARGET) {
         } else if (pDesc->Usage & D3DUSAGE_DEPTHSTENCIL) {
      if (!depth_stencil_format(pDesc->Format))
         This->base.info.bind = d3d9_get_pipe_depth_format_bindings(pDesc->Format);
   if (TextureType)
               This->base.info.flags = 0;
   This->base.info.format = d3d9_to_pipe_format_checked(This->base.info.screen,
                                          if (This->base.info.format == PIPE_FORMAT_NONE && pDesc->Format != D3DFMT_NULL)
            if (allocate && compressed_format(pDesc->Format)) {
      const unsigned w = util_format_get_blockwidth(This->base.info.format);
            /* Note: In the !allocate case, the test could fail (lower levels of a texture) */
               /* Get true format */
   This->format_internal = d3d9_to_pipe_format_checked(This->base.info.screen,
                                       if (This->base.info.format != This->format_internal ||
      /* DYNAMIC Textures requires same stride as ram buffers.
      * The workaround stores a copy in RAM for locks. It eats more virtual space,
      (pParams->device->workarounds.dynamic_texture_workaround &&
         This->data_internal = nine_allocate(pParams->device->allocator,
         nine_format_get_level_alloc_size(This->format_internal,
               if (!This->data_internal)
         This->stride_internal = nine_format_get_stride(This->format_internal,
               if ((allocate && pDesc->Pool != D3DPOOL_DEFAULT) || pDesc->Format == D3DFMT_NULL) {
      /* Ram buffer with no parent. Has to allocate the resource itself */
   assert(!user_buffer);
   This->data = nine_allocate(pParams->device->allocator,
         nine_format_get_level_alloc_size(This->base.info.format,
               if (!This->data)
               hr = NineResource9_ctor(&This->base, pParams, pResource,
                  if (FAILED(hr))
                     This->texture = TextureType;
   This->level = Level;
   This->level_actual = Level;
   This->layer = Layer;
                     if (This->base.resource && (pDesc->Usage & D3DUSAGE_DYNAMIC))
            if (This->base.resource && (pDesc->Usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)))
            /* TODO: investigate what else exactly needs to be cleared */
   if (This->base.resource && (pDesc->Usage & D3DUSAGE_RENDERTARGET))
                        }
      void
   NineSurface9_dtor( struct NineSurface9 *This )
   {
      bool is_worker = nine_context_is_worker(This->base.base.device);
            if (This->transfer) {
      struct pipe_context *pipe = nine_context_get_pipe_multithread(This->base.base.device);
   pipe->texture_unmap(pipe, This->transfer);
               /* Note: Following condition cannot happen currently, since we
   * refcount the surface in the functions increasing
   * pending_uploads_counter. */
   if (p_atomic_read(&This->pending_uploads_counter))
            pipe_surface_reference(&This->surface[0], NULL);
            if (!is_worker && This->lock_count && (This->data_internal || This->data)) {
      /* For is_worker nine_free_worker will handle it */
   nine_pointer_strongrelease(This->base.base.device->allocator,
               /* Release system memory when we have to manage it (no parent) */
   if (This->data) {
      if (is_worker)
         else
      }
   if (This->data_internal) {
      if (is_worker)
         else
      }
      }
      static void
   NineSurface9_CreatePipeSurfaces( struct NineSurface9 *This )
   {
      struct pipe_context *pipe;
   struct pipe_screen *screen = NineDevice9_GetScreen(This->base.base.device);
   struct pipe_resource *resource = This->base.resource;
   struct pipe_surface templ;
            assert(This->desc.Pool == D3DPOOL_DEFAULT);
            srgb_format = util_format_srgb(resource->format);
   if (srgb_format == PIPE_FORMAT_NONE ||
      !screen->is_format_supported(screen, srgb_format,
               memset(&templ, 0, sizeof(templ));
   templ.format = resource->format;
   templ.u.tex.level = This->level;
   templ.u.tex.first_layer = This->layer;
                              memset(&templ, 0, sizeof(templ));
   templ.format = srgb_format;
   templ.u.tex.level = This->level;
   templ.u.tex.first_layer = This->layer;
                              assert(This->surface[0]); /* TODO: Handle failure */
      }
      #if defined(DEBUG) || !defined(NDEBUG)
   void
   NineSurface9_Dump( struct NineSurface9 *This )
   {
      struct NineBaseTexture9 *tex;
   GUID id = IID_IDirect3DBaseTexture9;
            DBG("\nNineSurface9(%p->%p/%p): Pool=%s Type=%s Usage=%s\n"
      "Dims=%ux%u Format=%s Stride=%u Lockable=%i\n"
   "Level=%u(%u), Layer=%u\n", This, This->base.resource, This->data,
   nine_D3DPOOL_to_str(This->desc.Pool),
   nine_D3DRTYPE_to_str(This->desc.Type),
   nine_D3DUSAGE_to_str(This->desc.Usage),
   This->desc.Width, This->desc.Height,
   d3dformat_to_string(This->desc.Format), This->stride,
   This->base.resource &&
   (This->base.resource->flags & NINE_RESOURCE_FLAG_LOCKABLE),
         if (!This->base.base.container)
         NineUnknown_QueryInterface(This->base.base.container, ref, (void **)&tex);
   if (tex) {
      NineBaseTexture9_Dump(tex);
         }
   #endif /* DEBUG || !NDEBUG */
      HRESULT NINE_WINAPI
   NineSurface9_GetContainer( struct NineSurface9 *This,
               {
      HRESULT hr;
            DBG("This=%p riid=%p id=%s ppContainer=%p\n",
                              /* Use device for OffscreenPlainSurface, DepthStencilSurface and RenderTarget */
   hr = NineUnknown_QueryInterface(NineUnknown(This)->container ?
               if (FAILED(hr))
            }
      void
   NineSurface9_MarkContainerDirty( struct NineSurface9 *This )
   {
      if (This->texture) {
      struct NineBaseTexture9 *tex =
         assert(tex);
   assert(This->texture == D3DRTYPE_TEXTURE ||
         if (This->base.pool == D3DPOOL_MANAGED)
         else
   if (This->base.usage & D3DUSAGE_AUTOGENMIPMAP)
                  }
      HRESULT NINE_WINAPI
   NineSurface9_GetDesc( struct NineSurface9 *This,
         {
      user_assert(pDesc != NULL, E_POINTER);
   DBG("This=%p pDesc=%p\n", This, pDesc);
   *pDesc = This->desc;
      }
      /* Add the dirty rects to the source texture */
   inline void
   NineSurface9_AddDirtyRect( struct NineSurface9 *This,
         {
                        assert (This->base.pool != D3DPOOL_MANAGED ||
                  if (This->base.pool == D3DPOOL_DEFAULT)
            /* Add a dirty rect to level 0 of the parent texture */
   dirty_rect.left = box->x << This->level_actual;
   dirty_rect.right = dirty_rect.left + (box->width << This->level_actual);
   dirty_rect.top = box->y << This->level_actual;
            if (This->texture == D3DRTYPE_TEXTURE) {
      struct NineTexture9 *tex =
               } else if (This->texture == D3DRTYPE_CUBETEXTURE) {
      struct NineCubeTexture9 *ctex =
                  }
      static inline unsigned
   NineSurface9_GetSystemMemOffset(enum pipe_format format, unsigned stride,
         {
                           }
      HRESULT NINE_WINAPI
   NineSurface9_LockRect( struct NineSurface9 *This,
                     {
      struct pipe_resource *resource = This->base.resource;
   struct pipe_context *pipe;
   struct pipe_box box;
            DBG("This=%p pLockedRect=%p pRect=%p[%u..%u,%u..%u] Flags=%s\n", This,
      pLockedRect, pRect,
   pRect ? pRect->left : 0, pRect ? pRect->right : 0,
   pRect ? pRect->top : 0, pRect ? pRect->bottom : 0,
               /* check if it's already locked */
            /* set pBits to NULL after lock_count check */
   user_assert(pLockedRect, E_POINTER);
         #ifdef NINE_STRICT
      user_assert(This->base.pool != D3DPOOL_DEFAULT ||
            #endif
      user_assert(!((Flags & D3DLOCK_DISCARD) && (Flags & D3DLOCK_READONLY)),
            user_assert(This->desc.MultiSampleType == D3DMULTISAMPLE_NONE,
            if (pRect && This->desc.Pool == D3DPOOL_DEFAULT &&
      util_format_is_compressed(This->base.info.format)) {
   const unsigned w = util_format_get_blockwidth(This->base.info.format);
   const unsigned h = util_format_get_blockheight(This->base.info.format);
   user_assert((pRect->left == 0 && pRect->right == This->desc.Width &&
                                 if (Flags & D3DLOCK_DISCARD) {
         } else {
      usage = (Flags & D3DLOCK_READONLY) ?
      }
   if (Flags & D3DLOCK_DONOTWAIT)
            if (pRect) {
      /* Windows XP accepts invalid locking rectangles, Windows 7 rejects
            } else {
         }
                     if (p_atomic_read(&This->pending_uploads_counter))
            if (This->data_internal || This->data) {
      enum pipe_format format = This->base.info.format;
   unsigned stride = This->stride;
   uint8_t *data = nine_get_pointer(This->base.base.device->allocator, This->data_internal ? This->data_internal : This->data);
   if (This->data_internal) {
         format = This->format_internal;
   }
   /* ATI1 and ATI2 need special handling, because of d3d9 bug.
      * We must advertise to the application as if it is uncompressed
   * and bpp 8, and the app has a workaround to work with the fact
      if (is_ATI1_ATI2(format)) {
         pLockedRect->Pitch = This->desc.Width;
   } else {
         pLockedRect->Pitch = stride;
   pLockedRect->pBits = data +
      NineSurface9_GetSystemMemOffset(format,
            }
      } else {
      bool no_refs = !p_atomic_read(&This->base.base.bind) &&
         DBG("mapping pipe_resource %p (level=%u usage=%x)\n",
            /* if the object is not bound internally, there can't be any pending
         if (no_refs)
         else
         pLockedRect->pBits = pipe->texture_map(pipe, resource,
               if (no_refs)
         if (!This->transfer) {
         DBG("texture_map failed\n");
   if (Flags & D3DLOCK_DONOTWAIT)
         }
               if (!(Flags & (D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_READONLY))) {
      NineSurface9_MarkContainerDirty(This);
               ++This->lock_count;
      }
      HRESULT NINE_WINAPI
   NineSurface9_UnlockRect( struct NineSurface9 *This )
   {
      struct pipe_box dst_box, src_box;
   struct pipe_context *pipe;
   DBG("This=%p lock_count=%u\n", This, This->lock_count);
   user_assert(This->lock_count, D3DERR_INVALIDCALL);
   if (This->transfer) {
      pipe = nine_context_get_pipe_acquire(This->base.base.device);
   pipe->texture_unmap(pipe, This->transfer);
   nine_context_get_pipe_release(This->base.base.device);
      }
            if (This->data_internal) {
      nine_pointer_weakrelease(This->base.base.device->allocator, This->data_internal);
   if (This->data) {
         (void) util_format_translate(This->base.info.format,
                              nine_get_pointer(This->base.base.device->allocator, This->data),
   This->stride,
   0, 0,
      nine_pointer_weakrelease(This->base.base.device->allocator, This->data);
   } else {
         u_box_2d_zslice(0, 0, This->layer,
                        nine_context_box_upload(This->base.base.device,
                           &This->pending_uploads_counter,
   (struct NineUnknown *)This,
   This->base.resource,
   This->level,
   &dst_box,
   This->format_internal,
      } else if (This->data) {
                     }
      HRESULT NINE_WINAPI
   NineSurface9_GetDC( struct NineSurface9 *This,
         {
         }
      HRESULT NINE_WINAPI
   NineSurface9_ReleaseDC( struct NineSurface9 *This,
         {
         }
      IDirect3DSurface9Vtbl NineSurface9_vtable = {
      (void *)NineUnknown_QueryInterface,
   (void *)NineUnknown_AddRef,
   (void *)NineUnknown_Release,
   (void *)NineUnknown_GetDevice, /* actually part of Resource9 iface */
   (void *)NineUnknown_SetPrivateData,
   (void *)NineUnknown_GetPrivateData,
   (void *)NineUnknown_FreePrivateData,
   (void *)NineResource9_SetPriority,
   (void *)NineResource9_GetPriority,
   (void *)NineResource9_PreLoad,
   (void *)NineResource9_GetType,
   (void *)NineSurface9_GetContainer,
   (void *)NineSurface9_GetDesc,
   (void *)NineSurface9_LockRect,
   (void *)NineSurface9_UnlockRect,
   (void *)NineSurface9_GetDC,
      };
      /* When this function is called, we have already checked
   * The copy regions fit the surfaces */
   void
   NineSurface9_CopyMemToDefault( struct NineSurface9 *This,
                     {
      struct pipe_resource *r_dst = This->base.resource;
   struct pipe_box dst_box, src_box;
            assert(This->base.pool == D3DPOOL_DEFAULT &&
            if (pDestPoint) {
      dst_x = pDestPoint->x;
      } else {
      dst_x = 0;
               if (pSourceRect) {
      src_x = pSourceRect->left;
   src_y = pSourceRect->top;
   copy_width = pSourceRect->right - pSourceRect->left;
      } else {
      src_x = 0;
   src_y = 0;
   copy_width = From->desc.Width;
               u_box_2d_zslice(dst_x, dst_y, This->layer,
         u_box_2d_zslice(src_x, src_y, 0,
            if (This->data_internal) {
      (void) util_format_translate(This->format_internal,
                                 nine_get_pointer(This->base.base.device->allocator, This->data_internal),
   This->stride_internal,
   dst_x, dst_y,
   nine_pointer_weakrelease(This->base.base.device->allocator, From->data);
               nine_context_box_upload(This->base.base.device,
                           &From->pending_uploads_counter,
   (struct NineUnknown *)From,
   r_dst,
   This->level,
   &dst_box,
   From->base.info.format,
            if (From->texture == D3DRTYPE_TEXTURE) {
      struct NineTexture9 *tex =
         /* D3DPOOL_SYSTEMMEM with buffer content passed
      * from the user: execute the upload right now.
   * It is possible it is enough to delay upload
   * until the surface refcount is 0, but the
   * bind refcount may not be 0, and thus the dtor
   * is not executed (and doesn't trigger the
      if (!tex->managed_buffer)
                  }
      void
   NineSurface9_CopyDefaultToMem( struct NineSurface9 *This,
         {
      struct pipe_context *pipe;
   struct pipe_resource *r_src = From->base.resource;
   struct pipe_transfer *transfer;
   struct pipe_box src_box;
   uint8_t *p_dst;
            assert(This->base.pool == D3DPOOL_SYSTEMMEM &&
            assert(This->desc.Width == From->desc.Width);
            u_box_origin_2d(This->desc.Width, This->desc.Height, &src_box);
            if (p_atomic_read(&This->pending_uploads_counter))
            pipe = NineDevice9_GetPipe(This->base.base.device);
   p_src = pipe->texture_map(pipe, r_src, From->level,
                                 util_copy_rect(p_dst, This->base.info.format,
                  This->stride, 0, 0,
                     }
         /* Gladly, rendering to a MANAGED surface is not permitted, so we will
   * never have to do the reverse, i.e. download the surface.
   */
   HRESULT
   NineSurface9_UploadSelf( struct NineSurface9 *This,
         {
      struct pipe_resource *res = This->base.resource;
                              if (damaged) {
      box = *damaged;
   box.z = This->layer;
      } else {
      box.x = 0;
   box.y = 0;
   box.z = This->layer;
   box.width = This->desc.Width;
   box.height = This->desc.Height;
               nine_context_box_upload(This->base.base.device,
                           &This->pending_uploads_counter,
   (struct NineUnknown *)This,
   res,
   This->level,
   &box,
   res->format,
               }
      /* Currently nine_context uses the NineSurface9
   * fields when it is render target. Any modification requires
   * pending commands with the surface to be executed. If the bind
   * count is 0, there is no pending commands. */
   #define PROCESS_IF_BOUND(surf) \
      if (surf->base.base.bind) \
         void
   NineSurface9_SetResource( struct NineSurface9 *This,
         {
      /* No need to call PROCESS_IF_BOUND, because SetResource is used only
   * for MANAGED textures, and they are not render targets. */
   assert(This->base.pool == D3DPOOL_MANAGED);
   This->level = level;
      }
      void
   NineSurface9_SetMultiSampleType( struct NineSurface9 *This,
         {
      PROCESS_IF_BOUND(This);
      }
      void
   NineSurface9_SetResourceResize( struct NineSurface9 *This,
         {
      assert(This->level == 0 && This->level_actual == 0);
   assert(!This->lock_count);
   assert(This->desc.Pool == D3DPOOL_DEFAULT);
            PROCESS_IF_BOUND(This);
            This->desc.Width = This->base.info.width0 = resource->width0;
   This->desc.Height = This->base.info.height0 = resource->height0;
   This->base.info.nr_samples = resource->nr_samples;
            This->stride = nine_format_get_stride(This->base.info.format,
            pipe_surface_reference(&This->surface[0], NULL);
   pipe_surface_reference(&This->surface[1], NULL);
      }
         static const GUID *NineSurface9_IIDs[] = {
      &IID_IDirect3DSurface9,
   &IID_IDirect3DResource9,
   &IID_IUnknown,
      };
      HRESULT
   NineSurface9_new( struct NineDevice9 *pDevice,
                     struct NineUnknown *pContainer,
   struct pipe_resource *pResource,
   struct nine_allocation *user_buffer,
   uint8_t TextureType,
   unsigned Level,
   {
      NINE_DEVICE_CHILD_NEW(Surface9, ppOut, pDevice, /* args */
            }
