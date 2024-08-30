   /**************************************************************************
   *
   * Copyright 2012-2021 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
      /*
   * Resource.cpp --
   *    Functions that manipulate GPU resources.
   */
         #include "Resource.h"
   #include "Format.h"
   #include "State.h"
   #include "Query.h"
      #include "Debug.h"
      #include "util/u_math.h"
   #include "util/u_rect.h"
   #include "util/u_surface.h"
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateResourceSize --
   *
   *    The CalcPrivateResourceSize function determines the size of
   *    the user-mode display driver's private region of memory
   *    (that is, the size of internal driver structures, not the
   *    size of the resource video memory).
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateResourceSize(D3D10DDI_HDEVICE hDevice,                                // IN
         {
      LOG_ENTRYPOINT();
      }
         static unsigned
   translate_resource_usage( unsigned usage )
   {
               switch (usage) {
   case D3D10_DDI_USAGE_DEFAULT:
      resource_usage = PIPE_USAGE_DEFAULT;
      case D3D10_DDI_USAGE_IMMUTABLE:
      resource_usage = PIPE_USAGE_IMMUTABLE;
      case D3D10_DDI_USAGE_DYNAMIC:
      resource_usage = PIPE_USAGE_DYNAMIC;
      case D3D10_DDI_USAGE_STAGING:
      resource_usage = PIPE_USAGE_STAGING;
      default:
      assert(0);
                  }
         static unsigned
   translate_resource_flags(UINT flags)
   {
               if (flags & D3D10_DDI_BIND_VERTEX_BUFFER)
            if (flags & D3D10_DDI_BIND_INDEX_BUFFER)
            if (flags & D3D10_DDI_BIND_CONSTANT_BUFFER)
            if (flags & D3D10_DDI_BIND_SHADER_RESOURCE)
            if (flags & D3D10_DDI_BIND_RENDER_TARGET)
            if (flags & D3D10_DDI_BIND_DEPTH_STENCIL)
            if (flags & D3D10_DDI_BIND_STREAM_OUTPUT)
               }
         static enum pipe_texture_target
   translate_texture_target( D3D10DDIRESOURCE_TYPE ResourceDimension,
         {
      assert(ArraySize >= 1);
   switch(ResourceDimension) {
   case D3D10DDIRESOURCE_BUFFER:
      assert(ArraySize == 1);
      case D3D10DDIRESOURCE_TEXTURE1D:
         case D3D10DDIRESOURCE_TEXTURE2D:
         case D3D10DDIRESOURCE_TEXTURE3D:
      assert(ArraySize == 1);
      case D3D10DDIRESOURCE_TEXTURECUBE:
      assert(ArraySize % 6 == 0);
      default:
      assert(0);
         }
         static void
   subResourceBox(struct pipe_resource *resource, // IN
                     {
      UINT MipLevels = resource->last_level + 1;
   unsigned layer;
   unsigned width;
   unsigned height;
            *pLevel = SubResource % MipLevels;
            width  = u_minify(resource->width0,  *pLevel);
   height = u_minify(resource->height0, *pLevel);
            pBox->x = 0;
   pBox->y = 0;
   pBox->z = 0 + layer;
   pBox->width  = width;
   pBox->height = height;
      }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateResource --
   *
   *    The CreateResource function creates a resource.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateResource(D3D10DDI_HDEVICE hDevice,                                // IN
                     {
               if ((pCreateResource->MiscFlags & D3D10_DDI_RESOURCE_MISC_SHARED) ||
      (pCreateResource->pPrimaryDesc &&
            DebugPrintf("%s(%dx%dx%d hResource=%p)\n",
   __func__,
   pCreateResource->pMipInfoList[0].TexelWidth,
   pCreateResource->pMipInfoList[0].TexelHeight,
   pCreateResource->pMipInfoList[0].TexelDepth,
   hResource.pDrvPrivate);
   DebugPrintf("  ResourceDimension = %u\n",
   pCreateResource->ResourceDimension);
   DebugPrintf("  Usage = %u\n",
   pCreateResource->Usage);
   DebugPrintf("  BindFlags = 0x%x\n",
   pCreateResource->BindFlags);
   DebugPrintf("  MapFlags = 0x%x\n",
   pCreateResource->MapFlags);
   DebugPrintf("  MiscFlags = 0x%x\n",
   pCreateResource->MiscFlags);
   DebugPrintf("  Format = %s\n",
   FormatToName(pCreateResource->Format));
   DebugPrintf("  SampleDesc.Count = %u\n", pCreateResource->SampleDesc.Count);
   DebugPrintf("  SampleDesc.Quality = %u\n", pCreateResource->SampleDesc.Quality);
   DebugPrintf("  MipLevels = %u\n", pCreateResource->MipLevels);
   DebugPrintf("  ArraySize = %u\n", pCreateResource->ArraySize);
   DebugPrintf("  pPrimaryDesc = %p\n", pCreateResource->pPrimaryDesc);
   DebugPrintf("    Flags = 0x%x\n",
         DebugPrintf("    VidPnSourceId = %u\n", pCreateResource->pPrimaryDesc->VidPnSourceId);
   DebugPrintf("    ModeDesc.Width = %u\n", pCreateResource->pPrimaryDesc->ModeDesc.Width);
   DebugPrintf("    ModeDesc.Height = %u\n", pCreateResource->pPrimaryDesc->ModeDesc.Height);
   DebugPrintf("    ModeDesc.Format = %u)\n",
         DebugPrintf("    ModeDesc.RefreshRate.Numerator = %u\n", pCreateResource->pPrimaryDesc->ModeDesc.RefreshRate.Numerator);
   DebugPrintf("    ModeDesc.RefreshRate.Denominator = %u\n", pCreateResource->pPrimaryDesc->ModeDesc.RefreshRate.Denominator);
   DebugPrintf("    ModeDesc.ScanlineOrdering = %u\n",
         DebugPrintf("    ModeDesc.Rotation = %u\n",
         DebugPrintf("    ModeDesc.Scaling = %u\n",
         DebugPrintf("    DriverFlags = 0x%x\n",
      pCreateResource->pPrimaryDesc->DriverFlags);
                     struct pipe_context *pipe = CastPipeContext(hDevice);
                           #if 0
      if (pCreateResource->pPrimaryDesc) {
      pCreateResource->pPrimaryDesc->DriverFlags = DXGI_DDI_PRIMARY_DRIVER_FLAG_NO_SCANOUT;
   if (!(pCreateResource->pPrimaryDesc->DriverFlags & DXGI_DDI_PRIMARY_OPTIONAL)) {
      // http://msdn.microsoft.com/en-us/library/windows/hardware/ff568846.aspx
   SetError(hDevice, DXGI_DDI_ERR_UNSUPPORTED);
            #endif
         pResource->Format = pCreateResource->Format;
                              templat.target     = translate_texture_target( pCreateResource->ResourceDimension,
                  if (pCreateResource->Format == DXGI_FORMAT_UNKNOWN) {
      assert(pCreateResource->ResourceDimension == D3D10DDIRESOURCE_BUFFER);
      } else {
      BOOL bindDepthStencil = !!(pCreateResource->BindFlags & D3D10_DDI_BIND_DEPTH_STENCIL);
               templat.width0     = pCreateResource->pMipInfoList[0].TexelWidth;
   templat.height0    = pCreateResource->pMipInfoList[0].TexelHeight;
   templat.depth0     = pCreateResource->pMipInfoList[0].TexelDepth;
   templat.array_size = pCreateResource->ArraySize;
   templat.last_level = pCreateResource->MipLevels - 1;
   templat.nr_samples = pCreateResource->SampleDesc.Count;
   templat.nr_storage_samples = pCreateResource->SampleDesc.Count;
   templat.bind       = translate_resource_flags(pCreateResource->BindFlags);
            if (templat.target != PIPE_BUFFER) {
      if (!screen->is_format_supported(screen,
                                    debug_printf("%s: unsupported format %s\n",
         SetError(hDevice, E_OUTOFMEMORY);
                  pResource->resource = screen->resource_create(screen, &templat);
   if (!pResource) {
      DebugPrintf("%s: failed to create resource\n", __func__);
   SetError(hDevice, E_OUTOFMEMORY);
               pResource->NumSubResources = pCreateResource->MipLevels * pCreateResource->ArraySize;
   pResource->transfers = (struct pipe_transfer **)calloc(pResource->NumSubResources,
            if (pCreateResource->pInitialDataUP) {
      if (pResource->buffer) {
      assert(pResource->NumSubResources == 1);
                  unsigned level;
                  struct pipe_transfer *transfer;
   void *map;
   map = pipe->buffer_map(pipe,
                        pResource->resource,
   level,
      assert(map);
   if (map) {
      memcpy(map, pInitialDataUP->pSysMem, box.width);
         } else {
      for (UINT SubResource = 0; SubResource < pResource->NumSubResources; ++SubResource) {
                     unsigned level;
                  struct pipe_transfer *transfer;
   void *map;
   map = pipe->texture_map(pipe,
                           pResource->resource,
   level,
   assert(map);
   if (map) {
      for (int z = 0; z < box.depth; ++z) {
      uint8_t *dst = (uint8_t*)map + z*transfer->layer_stride;
   const uint8_t *src = (const uint8_t*)pInitialDataUP->pSysMem + z*pInitialDataUP->SysMemSlicePitch;
   util_copy_rect(dst,
                  templat.format,
   transfer->stride,
   0, 0, box.width, box.height,
   }
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateOpenedResourceSize --
   *
   *    The CalcPrivateOpenedResourceSize function determines the size
   *    of the user-mode display driver's private shared region of memory
   *    (that is, the size of internal driver structures, not the size
   *    of the resource video memory) for an opened resource.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateOpenedResourceSize(D3D10DDI_HDEVICE hDevice,                             // IN
         {
         }
         /*
   * ----------------------------------------------------------------------
   *
   * OpenResource --
   *
   *    The OpenResource function opens a shared resource.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   OpenResource(D3D10DDI_HDEVICE hDevice,                            // IN
               __in const D3D10DDIARG_OPENRESOURCE *pOpenResource,  // IN
   {
      LOG_UNSUPPORTED_ENTRYPOINT();
      }
         /*
   * ----------------------------------------------------------------------
   *
   * DestroyResource --
   *
   *    The DestroyResource function destroys the specified resource
   *    object. The resource object can be destoyed only if it is not
   *    currently bound to a display device, and if all views that
   *    refer to the resource are also destroyed.
   *
   * ----------------------------------------------------------------------
   */
         void APIENTRY
   DestroyResource(D3D10DDI_HDEVICE hDevice,       // IN
         {
               struct pipe_context *pipe = CastPipeContext(hDevice);
            if (pResource->so_target) {
                  for (UINT SubResource = 0; SubResource < pResource->NumSubResources; ++SubResource) {
      if (pResource->transfers[SubResource]) {
      if (pResource->buffer) {
         } else {
         }
         }
               }
         /*
   * ----------------------------------------------------------------------
   *
   * ResourceMap --
   *
   *    The ResourceMap function maps a subresource of a resource.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   ResourceMap(D3D10DDI_HDEVICE hDevice,                                // IN
               D3D10DDI_HRESOURCE hResource,                            // IN
   UINT SubResource,                                        // IN
   D3D10_DDI_MAP DDIMap,                                    // IN
   {
               struct pipe_context *pipe = CastPipeContext(hDevice);
   Resource *pResource = CastResource(hResource);
            unsigned usage;
   switch (DDIMap) {
   case D3D10_DDI_MAP_READ:
      usage = PIPE_MAP_READ;
      case D3D10_DDI_MAP_READWRITE:
      usage = PIPE_MAP_READ | PIPE_MAP_WRITE;
      case D3D10_DDI_MAP_WRITE:
      usage = PIPE_MAP_WRITE;
      case D3D10_DDI_MAP_WRITE_DISCARD:
      usage = PIPE_MAP_WRITE;
   if (resource->last_level == 0 && resource->array_size == 1) {
         } else {
         }
      case D3D10_DDI_MAP_WRITE_NOOVERWRITE:
      usage = PIPE_MAP_WRITE | PIPE_MAP_UNSYNCHRONIZED;
      default:
      assert(0);
                        unsigned level;
   struct pipe_box box;
                     void *map;
   if (pResource->buffer) {
      map = pipe->buffer_map(pipe,
                        resource,
   } else {
      map = pipe->texture_map(pipe,
                              }
   if (!map) {
      DebugPrintf("%s: failed to map resource\n", __func__);
   SetError(hDevice, E_FAIL);
               pMappedSubResource->pData = map;
   pMappedSubResource->RowPitch = pResource->transfers[SubResource]->stride;
      }
         /*
   * ----------------------------------------------------------------------
   *
   * ResourceUnmap --
   *
   *    The ResourceUnmap function unmaps a subresource of a resource.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   ResourceUnmap(D3D10DDI_HDEVICE hDevice,      // IN
               {
               struct pipe_context *pipe = CastPipeContext(hDevice);
                     if (pResource->transfers[SubResource]) {
      if (pResource->buffer) {
         } else {
         }
         }
         /*
   *----------------------------------------------------------------------
   *
   * areResourcesCompatible --
   *
   *      Check whether two resources can be safely passed to
   *      pipe_context::resource_copy_region method.
   *
   * Results:
   *      As above.
   *
   * Side effects:
   *      None.
   *
   *----------------------------------------------------------------------
   */
      static bool
   areResourcesCompatible(const struct pipe_resource *src_resource, // IN
         {
      if (src_resource->format == dst_resource->format) {
      /*
   * Trivial.
               } else if (src_resource->target == PIPE_BUFFER &&
            /*
   * Buffer resources are merely a collection of bytes.
               } else {
      /*
   * Check whether the formats are supported by
   * the resource_copy_region method.
            const struct util_format_description *src_format_desc;
            src_format_desc = util_format_description(src_resource->format);
            assert(src_format_desc->block.width  == dst_format_desc->block.width);
   assert(src_format_desc->block.height == dst_format_desc->block.height);
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * ResourceCopy --
   *
   *    The ResourceCopy function copies an entire source
   *    resource to a destination resource.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   ResourceCopy(D3D10DDI_HDEVICE hDevice,          // IN
               {
               Device *pDevice = CastDevice(hDevice);
   if (!CheckPredicate(pDevice)) {
                  struct pipe_context *pipe = pDevice->pipe;
   Resource *pDstResource = CastResource(hDstResource);
   Resource *pSrcResource = CastResource(hSrcResource);
   struct pipe_resource *dst_resource = pDstResource->resource;
   struct pipe_resource *src_resource = pSrcResource->resource;
            assert(dst_resource->target == src_resource->target);
   assert(dst_resource->width0 == src_resource->width0);
   assert(dst_resource->height0 == src_resource->height0);
   assert(dst_resource->depth0 == src_resource->depth0);
   assert(dst_resource->last_level == src_resource->last_level);
                     /* could also use one 3d copy for arrays */
   for (unsigned layer = 0; layer < dst_resource->array_size; ++layer) {
      for (unsigned level = 0; level <= dst_resource->last_level; ++level) {
      struct pipe_box box;
   box.x = 0;
   box.y = 0;
   box.z = 0 + layer;
   box.width  = u_minify(dst_resource->width0,  level);
         if (compatible) {
               pipe->resource_copy_region(pipe,
                        } else {
      util_resource_copy_region(pipe,
                                 }
         /*
   * ----------------------------------------------------------------------
   *
   * ResourceCopyRegion --
   *
   *    The ResourceCopyRegion function copies a source subresource
   *    region to a location on a destination subresource.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   ResourceCopyRegion(D3D10DDI_HDEVICE hDevice,                // IN
                     D3D10DDI_HRESOURCE hDstResource,         // IN
   UINT DstSubResource,                     // IN
   UINT DstX,                               // IN
   UINT DstY,                               // IN
   UINT DstZ,                               // IN
   {
               Device *pDevice = CastDevice(hDevice);
   if (!CheckPredicate(pDevice)) {
                  struct pipe_context *pipe = pDevice->pipe;
   Resource *pDstResource = CastResource(hDstResource);
   Resource *pSrcResource = CastResource(hSrcResource);
   struct pipe_resource *dst_resource = pDstResource->resource;
            unsigned dst_level = DstSubResource % (dst_resource->last_level + 1);
   unsigned dst_layer = DstSubResource / (dst_resource->last_level + 1);
   unsigned src_level = SrcSubResource % (src_resource->last_level + 1);
            struct pipe_box src_box;
   if (pSrcBox) {
      src_box.x = pSrcBox->left;
   src_box.y = pSrcBox->top;
   src_box.z = pSrcBox->front + src_layer;
   src_box.width  = pSrcBox->right  - pSrcBox->left;
   src_box.height = pSrcBox->bottom - pSrcBox->top;
      } else {
      src_box.x = 0;
   src_box.y = 0;
   src_box.z = 0 + src_layer;
   src_box.width  = u_minify(src_resource->width0,  src_level);
   src_box.height = u_minify(src_resource->height0, src_level);
               if (areResourcesCompatible(src_resource, dst_resource)) {
      pipe->resource_copy_region(pipe,
                        } else {
      util_resource_copy_region(pipe,
                           }
         /*
   * ----------------------------------------------------------------------
   *
   * ResourceResolveSubResource --
   *
   *    The ResourceResolveSubResource function resolves
   *    multiple samples to one pixel.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   ResourceResolveSubResource(D3D10DDI_HDEVICE hDevice,        // IN
                                 {
         }
         /*
   * ----------------------------------------------------------------------
   *
   * ResourceIsStagingBusy --
   *
   *    The ResourceIsStagingBusy function determines whether a
   *    resource is currently being used by the graphics pipeline.
   *
   * ----------------------------------------------------------------------
   */
      BOOL APIENTRY
   ResourceIsStagingBusy(D3D10DDI_HDEVICE hDevice,       // IN
         {
                           }
         /*
   * ----------------------------------------------------------------------
   *
   * ResourceReadAfterWriteHazard --
   *
   *    The ResourceReadAfterWriteHazard function informs the user-mode
   *    display driver that the specified resource was used as an output
   *    from the graphics processing unit (GPU) and that the resource
   *    will be used as an input to the GPU.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   ResourceReadAfterWriteHazard(D3D10DDI_HDEVICE hDevice,      // IN
         {
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * ResourceUpdateSubResourceUP --
   *
   *    The ResourceUpdateSubresourceUP function updates a
   *    destination subresource region from a source
   *    system memory region.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   ResourceUpdateSubResourceUP(D3D10DDI_HDEVICE hDevice,                // IN
                              D3D10DDI_HRESOURCE hDstResource,         // IN
      {
               Device *pDevice = CastDevice(hDevice);
   if (!CheckPredicate(pDevice)) {
                  struct pipe_context *pipe = pDevice->pipe;
   Resource *pDstResource = CastResource(hDstResource);
            unsigned level;
            if (pDstBox) {
      UINT DstMipLevels = dst_resource->last_level + 1;
   level = DstSubResource % DstMipLevels;
   unsigned dst_layer = DstSubResource / DstMipLevels;
   box.x = pDstBox->left;
   box.y = pDstBox->top;
   box.z = pDstBox->front + dst_layer;
   box.width  = pDstBox->right  - pDstBox->left;
   box.height = pDstBox->bottom - pDstBox->top;
      } else {
                  struct pipe_transfer *transfer;
   void *map;
   if (pDstResource->buffer) {
      map = pipe->buffer_map(pipe,
                              } else {
      map = pipe->texture_map(pipe,
                              }
   assert(map);
   if (map) {
      for (int z = 0; z < box.depth; ++z) {
      uint8_t *dst = (uint8_t*)map + z*transfer->layer_stride;
   const uint8_t *src = (const uint8_t*)pSysMemUP + z*DepthPitch;
   util_copy_rect(dst,
                  dst_resource->format,
   transfer->stride,
   0, 0, box.width, box.height,
   }
   if (pDstResource->buffer) {
         } else {
               }
   