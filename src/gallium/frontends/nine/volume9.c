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
      #include "device9.h"
   #include "volume9.h"
   #include "basetexture9.h" /* for marking dirty */
   #include "volumetexture9.h"
   #include "nine_helpers.h"
   #include "nine_pipe.h"
   #include "nine_dump.h"
      #include "util/format/u_format.h"
   #include "util/u_surface.h"
      #define DBG_CHANNEL DBG_VOLUME
         static HRESULT
   NineVolume9_AllocateData( struct NineVolume9 *This )
   {
               DBG("(%p(This=%p),level=%u) Allocating 0x%x bytes of system memory.\n",
            This->data = (uint8_t *)align_calloc(size, 32);
   if (!This->data)
            }
      static HRESULT
   NineVolume9_ctor( struct NineVolume9 *This,
                     struct NineUnknownParams *pParams,
   struct NineUnknown *pContainer,
   {
                        DBG("This=%p pContainer=%p pDevice=%p pResource=%p Level=%u pDesc=%p\n",
            /* Mark this as a special surface held by another internal resource. */
            user_assert(!(pDesc->Usage & D3DUSAGE_DYNAMIC) ||
                     hr = NineUnknown_ctor(&This->base, pParams);
   if (FAILED(hr))
                     This->transfer = NULL;
            This->level = Level;
   This->level_actual = Level;
            This->info.screen = pParams->device->screen;
   This->info.target = PIPE_TEXTURE_3D;
   This->info.width0 = pDesc->Width;
   This->info.height0 = pDesc->Height;
   This->info.depth0 = pDesc->Depth;
   This->info.last_level = 0;
   This->info.array_size = 1;
   This->info.nr_samples = 0;
   This->info.nr_storage_samples = 0;
   This->info.usage = PIPE_USAGE_DEFAULT;
   This->info.bind = PIPE_BIND_SAMPLER_VIEW;
   This->info.flags = 0;
   This->info.format = d3d9_to_pipe_format_checked(This->info.screen,
                                    if (This->info.format == PIPE_FORMAT_NONE)
            This->stride = util_format_get_stride(This->info.format, pDesc->Width);
   This->stride = align(This->stride, 4);
   This->layer_stride = util_format_get_2d_size(This->info.format,
            /* Get true format */
   This->format_internal = d3d9_to_pipe_format_checked(This->info.screen,
                                 if (This->info.format != This->format_internal ||
      /* See surface9.c */
   (pParams->device->workarounds.dynamic_texture_workaround &&
         This->stride_internal = nine_format_get_stride(This->format_internal,
         This->layer_stride_internal = util_format_get_2d_size(This->format_internal,
               This->data_internal = align_calloc(This->layer_stride_internal *
         if (!This->data_internal)
               if (!This->resource) {
      hr = NineVolume9_AllocateData(This);
   if (FAILED(hr))
      }
      }
      static void
   NineVolume9_dtor( struct NineVolume9 *This )
   {
               if (This->transfer) {
      struct pipe_context *pipe = nine_context_get_pipe_multithread(This->base.device);
   pipe->texture_unmap(pipe, This->transfer);
               /* Note: Following condition cannot happen currently, since we
   * refcount the volume in the functions increasing
   * pending_uploads_counter. */
   if (p_atomic_read(&This->pending_uploads_counter))
            if (This->data)
         if (This->data_internal)
                        }
      HRESULT NINE_WINAPI
   NineVolume9_GetContainer( struct NineVolume9 *This,
               {
               DBG("This=%p riid=%p id=%s ppContainer=%p\n",
                     if (!NineUnknown(This)->container)
            }
      static inline void
   NineVolume9_MarkContainerDirty( struct NineVolume9 *This )
   {
         #if defined(DEBUG) || !defined(NDEBUG)
      /* This is always contained by a NineVolumeTexture9. */
   GUID id = IID_IDirect3DVolumeTexture9;
   REFIID ref = &id;
   assert(NineUnknown_QueryInterface(This->base.container, ref, (void **)&tex)
            #endif
         tex = NineBaseTexture9(This->base.container);
   assert(tex);
   if (This->desc.Pool == D3DPOOL_MANAGED)
               }
      HRESULT NINE_WINAPI
   NineVolume9_GetDesc( struct NineVolume9 *This,
         {
      user_assert(pDesc != NULL, E_POINTER);
   *pDesc = This->desc;
      }
      inline void
   NineVolume9_AddDirtyRegion( struct NineVolume9 *This,
         {
      D3DBOX dirty_region;
            if (!box) {
         } else {
      dirty_region.Left = box->x << This->level_actual;
   dirty_region.Top = box->y << This->level_actual;
   dirty_region.Front = box->z << This->level_actual;
   dirty_region.Right = dirty_region.Left + (box->width << This->level_actual);
   dirty_region.Bottom = dirty_region.Top + (box->height << This->level_actual);
   dirty_region.Back = dirty_region.Front + (box->depth << This->level_actual);
         }
      static inline unsigned
   NineVolume9_GetSystemMemOffset(enum pipe_format format, unsigned stride,
               {
                           }
      HRESULT NINE_WINAPI
   NineVolume9_LockBox( struct NineVolume9 *This,
                     {
      struct pipe_context *pipe;
   struct pipe_resource *resource = This->resource;
   struct pipe_box box;
            DBG("This=%p(%p) pLockedVolume=%p pBox=%p[%u..%u,%u..%u,%u..%u] Flags=%s\n",
      This, This->base.container, pLockedVolume, pBox,
   pBox ? pBox->Left : 0, pBox ? pBox->Right : 0,
   pBox ? pBox->Top : 0, pBox ? pBox->Bottom : 0,
   pBox ? pBox->Front : 0, pBox ? pBox->Back : 0,
         /* check if it's already locked */
            /* set pBits to NULL after lock_count check */
   user_assert(pLockedVolume, E_POINTER);
            user_assert(This->desc.Pool != D3DPOOL_DEFAULT ||
            user_assert(!((Flags & D3DLOCK_DISCARD) && (Flags & D3DLOCK_READONLY)),
            if (pBox && compressed_format (This->desc.Format)) { /* For volume all pools are checked */
      const unsigned w = util_format_get_blockwidth(This->info.format);
   const unsigned h = util_format_get_blockheight(This->info.format);
   user_assert((pBox->Left == 0 && pBox->Right == This->desc.Width &&
                                 if (Flags & D3DLOCK_DISCARD) {
         } else {
      usage = (Flags & D3DLOCK_READONLY) ?
      }
   if (Flags & D3DLOCK_DONOTWAIT)
            if (pBox) {
      user_assert(pBox->Right > pBox->Left, D3DERR_INVALIDCALL);
   user_assert(pBox->Bottom > pBox->Top, D3DERR_INVALIDCALL);
   user_assert(pBox->Back > pBox->Front, D3DERR_INVALIDCALL);
   user_assert(pBox->Right <= This->desc.Width, D3DERR_INVALIDCALL);
   user_assert(pBox->Bottom <= This->desc.Height, D3DERR_INVALIDCALL);
            d3dbox_to_pipe_box(&box, pBox);
   if (u_box_clip_2d(&box, &box, This->desc.Width, This->desc.Height) < 0) {
         DBG("Locked volume intersection empty.\n");
      } else {
      u_box_3d(0, 0, 0, This->desc.Width, This->desc.Height, This->desc.Depth,
               if (p_atomic_read(&This->pending_uploads_counter))
            if (This->data_internal || This->data) {
      enum pipe_format format = This->info.format;
   unsigned stride = This->stride;
   unsigned layer_stride = This->layer_stride;
   uint8_t *data = This->data;
   if (This->data_internal) {
         format = This->format_internal;
   stride = This->stride_internal;
   layer_stride = This->layer_stride_internal;
   }
   pLockedVolume->RowPitch = stride;
   pLockedVolume->SlicePitch = layer_stride;
   pLockedVolume->pBits = data +
         NineVolume9_GetSystemMemOffset(format, stride,
      } else {
      bool no_refs = !p_atomic_read(&This->base.bind) &&
         if (no_refs)
         else
         pLockedVolume->pBits =
         pipe->texture_map(pipe, resource, This->level, usage,
   if (no_refs)
         if (!This->transfer) {
         if (Flags & D3DLOCK_DONOTWAIT)
         }
   pLockedVolume->RowPitch = This->transfer->stride;
               if (!(Flags & (D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_READONLY))) {
      NineVolume9_MarkContainerDirty(This);
               ++This->lock_count;
      }
      HRESULT NINE_WINAPI
   NineVolume9_UnlockBox( struct NineVolume9 *This )
   {
               DBG("This=%p lock_count=%u\n", This, This->lock_count);
   user_assert(This->lock_count, D3DERR_INVALIDCALL);
   if (This->transfer) {
      pipe = nine_context_get_pipe_acquire(This->base.device);
   pipe->texture_unmap(pipe, This->transfer);
   This->transfer = NULL;
      }
            if (This->data_internal) {
               u_box_3d(0, 0, 0, This->desc.Width, This->desc.Height, This->desc.Depth,
               if (This->data) {
         (void) util_format_translate_3d(This->info.format,
                                 This->data, This->stride,
   This->layer_stride,
   0, 0, 0,
   This->format_internal,
   } else {
         nine_context_box_upload(This->base.device,
                           &This->pending_uploads_counter,
   (struct NineUnknown *)This,
   This->resource,
   This->level,
   &box,
                  }
      /* When this function is called, we have already checked
   * The copy regions fit the volumes */
   void
   NineVolume9_CopyMemToDefault( struct NineVolume9 *This,
                     {
      struct pipe_resource *r_dst = This->resource;
   struct pipe_box src_box;
            DBG("This=%p From=%p dstx=%u dsty=%u dstz=%u pSrcBox=%p\n",
            assert(This->desc.Pool == D3DPOOL_DEFAULT &&
            dst_box.x = dstx;
   dst_box.y = dsty;
            if (pSrcBox) {
         } else {
      src_box.x = 0;
   src_box.y = 0;
   src_box.z = 0;
   src_box.width = From->desc.Width;
   src_box.height = From->desc.Height;
               dst_box.width = src_box.width;
   dst_box.height = src_box.height;
            nine_context_box_upload(This->base.device,
                           &From->pending_uploads_counter,
   (struct NineUnknown *)From,
   r_dst,
   This->level,
            if (This->data_internal)
      (void) util_format_translate_3d(This->format_internal,
                                    This->data_internal,
   This->stride_internal,
   This->layer_stride_internal,
   dstx, dsty, dstz,
   From->info.format,
                        }
      HRESULT
   NineVolume9_UploadSelf( struct NineVolume9 *This,
         {
      struct pipe_resource *res = This->resource;
            DBG("This=%p damaged=%p data=%p res=%p\n", This, damaged,
            assert(This->desc.Pool == D3DPOOL_MANAGED);
            if (damaged) {
         } else {
      box.x = 0;
   box.y = 0;
   box.z = 0;
   box.width = This->desc.Width;
   box.height = This->desc.Height;
               nine_context_box_upload(This->base.device,
                           &This->pending_uploads_counter,
   (struct NineUnknown *)This,
   res,
   This->level,
               }
         IDirect3DVolume9Vtbl NineVolume9_vtable = {
      (void *)NineUnknown_QueryInterface,
   (void *)NineUnknown_AddRef,
   (void *)NineUnknown_Release,
   (void *)NineUnknown_GetDevice, /* actually part of Volume9 iface */
   (void *)NineUnknown_SetPrivateData,
   (void *)NineUnknown_GetPrivateData,
   (void *)NineUnknown_FreePrivateData,
   (void *)NineVolume9_GetContainer,
   (void *)NineVolume9_GetDesc,
   (void *)NineVolume9_LockBox,
      };
      static const GUID *NineVolume9_IIDs[] = {
      &IID_IDirect3DVolume9,
   &IID_IUnknown,
      };
      HRESULT
   NineVolume9_new( struct NineDevice9 *pDevice,
                  struct NineUnknown *pContainer,
   struct pipe_resource *pResource,
      {
      NINE_DEVICE_CHILD_NEW(Volume9, ppOut, pDevice, /* args */
      }
