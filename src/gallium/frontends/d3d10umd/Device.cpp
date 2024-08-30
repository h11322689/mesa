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
   * Device.cpp --
   *    Functions that provide the 3D device functionality.
   */
         #include "Draw.h"
   #include "DxgiFns.h"
   #include "InputAssembly.h"
   #include "OutputMerger.h"
   #include "Query.h"
   #include "Rasterizer.h"
   #include "Resource.h"
   #include "Shader.h"
   #include "State.h"
   #include "Format.h"
      #include "Debug.h"
      #include "util/u_sampler.h"
         static void APIENTRY DestroyDevice(D3D10DDI_HDEVICE hDevice);
   static void APIENTRY RelocateDeviceFuncs(D3D10DDI_HDEVICE hDevice,
         static void APIENTRY RelocateDeviceFuncs1(D3D10DDI_HDEVICE hDevice,
         static void APIENTRY Flush(D3D10DDI_HDEVICE hDevice);
   static void APIENTRY CheckFormatSupport(D3D10DDI_HDEVICE hDevice, DXGI_FORMAT Format,
         static void APIENTRY CheckMultisampleQualityLevels(D3D10DDI_HDEVICE hDevice,
                     static void APIENTRY SetTextFilterSize(D3D10DDI_HDEVICE hDevice, UINT Width, UINT Height);
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateDeviceSize --
   *
   *    The CalcPrivateDeviceSize function determines the size of a memory
   *    region that the user-mode display driver requires from the Microsoft
   *    Direct3D runtime to store frequently-accessed data.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateDeviceSize(D3D10DDI_HADAPTER hAdapter,                          // IN
         {
         }
      /*
   * ----------------------------------------------------------------------
   *
   * CreateDevice --
   *
   *    The CreateDevice function creates a graphics context that is
   *    referenced in subsequent calls.
   *
   * ----------------------------------------------------------------------
   */
      HRESULT APIENTRY
   CreateDevice(D3D10DDI_HADAPTER hAdapter,                 // IN
         {
               if (0) {
      DebugPrintf("hAdapter = %p\n", hAdapter);
   DebugPrintf("pKTCallbacks = %p\n", pCreateData->pKTCallbacks);
   DebugPrintf("p10_1DeviceFuncs = %p\n", pCreateData->p10_1DeviceFuncs);
   DebugPrintf("hDrvDevice = %p\n", pCreateData->hDrvDevice);
   DebugPrintf("DXGIBaseDDI = %p\n", pCreateData->DXGIBaseDDI);
   DebugPrintf("hRTCoreLayer = %p\n", pCreateData->hRTCoreLayer);
               switch (pCreateData->Interface) {
   case D3D10_0_DDI_INTERFACE_VERSION:
   case D3D10_0_x_DDI_INTERFACE_VERSION:
      #if SUPPORT_D3D10_1
      case D3D10_1_DDI_INTERFACE_VERSION:
   case D3D10_1_x_DDI_INTERFACE_VERSION:
      #endif
            default:
      DebugPrintf("%s: unsupported interface version 0x%08x\n",
                              Device *pDevice = CastDevice(pCreateData->hDrvDevice);
            struct pipe_screen *screen = pAdapter->screen;
   struct pipe_context *pipe = screen->context_create(screen, NULL, 0);
   pDevice->pipe = pipe;
            pDevice->empty_vs = CreateEmptyShader(pDevice, PIPE_SHADER_VERTEX);
            pipe->bind_vs_state(pipe, pDevice->empty_vs);
            pDevice->max_dual_source_render_targets =
            pDevice->hRTCoreLayer = pCreateData->hRTCoreLayer;
   pDevice->hDevice = (HANDLE)pCreateData->hRTDevice.handle;
   pDevice->KTCallbacks = *pCreateData->pKTCallbacks;
   pDevice->UMCallbacks = *pCreateData->pUMCallbacks;
                     if (0) {
                           /*
   * Fill in the D3D10 DDI functions
   */
   D3D10DDI_DEVICEFUNCS *pDeviceFuncs = pCreateData->pDeviceFuncs;
   pDeviceFuncs->pfnDefaultConstantBufferUpdateSubresourceUP = ResourceUpdateSubResourceUP;
   pDeviceFuncs->pfnVsSetConstantBuffers = VsSetConstantBuffers;
   pDeviceFuncs->pfnPsSetShaderResources = PsSetShaderResources;
   pDeviceFuncs->pfnPsSetShader = PsSetShader;
   pDeviceFuncs->pfnPsSetSamplers = PsSetSamplers;
   pDeviceFuncs->pfnVsSetShader = VsSetShader;
   pDeviceFuncs->pfnDrawIndexed = DrawIndexed;
   pDeviceFuncs->pfnDraw = Draw;
   pDeviceFuncs->pfnDynamicIABufferMapNoOverwrite = ResourceMap;
   pDeviceFuncs->pfnDynamicIABufferUnmap = ResourceUnmap;
   pDeviceFuncs->pfnDynamicConstantBufferMapDiscard = ResourceMap;
   pDeviceFuncs->pfnDynamicIABufferMapDiscard = ResourceMap;
   pDeviceFuncs->pfnDynamicConstantBufferUnmap = ResourceUnmap;
   pDeviceFuncs->pfnPsSetConstantBuffers = PsSetConstantBuffers;
   pDeviceFuncs->pfnIaSetInputLayout = IaSetInputLayout;
   pDeviceFuncs->pfnIaSetVertexBuffers = IaSetVertexBuffers;
   pDeviceFuncs->pfnIaSetIndexBuffer = IaSetIndexBuffer;
   pDeviceFuncs->pfnDrawIndexedInstanced = DrawIndexedInstanced;
   pDeviceFuncs->pfnDrawInstanced = DrawInstanced;
   pDeviceFuncs->pfnDynamicResourceMapDiscard = ResourceMap;
   pDeviceFuncs->pfnDynamicResourceUnmap = ResourceUnmap;
   pDeviceFuncs->pfnGsSetConstantBuffers = GsSetConstantBuffers;
   pDeviceFuncs->pfnGsSetShader = GsSetShader;
   pDeviceFuncs->pfnIaSetTopology = IaSetTopology;
   pDeviceFuncs->pfnStagingResourceMap = ResourceMap;
   pDeviceFuncs->pfnStagingResourceUnmap = ResourceUnmap;
   pDeviceFuncs->pfnVsSetShaderResources = VsSetShaderResources;
   pDeviceFuncs->pfnVsSetSamplers = VsSetSamplers;
   pDeviceFuncs->pfnGsSetShaderResources = GsSetShaderResources;
   pDeviceFuncs->pfnGsSetSamplers = GsSetSamplers;
   pDeviceFuncs->pfnSetRenderTargets = SetRenderTargets;
   pDeviceFuncs->pfnShaderResourceViewReadAfterWriteHazard = ShaderResourceViewReadAfterWriteHazard;
   pDeviceFuncs->pfnResourceReadAfterWriteHazard = ResourceReadAfterWriteHazard;
   pDeviceFuncs->pfnSetBlendState = SetBlendState;
   pDeviceFuncs->pfnSetDepthStencilState = SetDepthStencilState;
   pDeviceFuncs->pfnSetRasterizerState = SetRasterizerState;
   pDeviceFuncs->pfnQueryEnd = QueryEnd;
   pDeviceFuncs->pfnQueryBegin = QueryBegin;
   pDeviceFuncs->pfnResourceCopyRegion = ResourceCopyRegion;
   pDeviceFuncs->pfnResourceUpdateSubresourceUP = ResourceUpdateSubResourceUP;
   pDeviceFuncs->pfnSoSetTargets = SoSetTargets;
   pDeviceFuncs->pfnDrawAuto = DrawAuto;
   pDeviceFuncs->pfnSetViewports = SetViewports;
   pDeviceFuncs->pfnSetScissorRects = SetScissorRects;
   pDeviceFuncs->pfnClearRenderTargetView = ClearRenderTargetView;
   pDeviceFuncs->pfnClearDepthStencilView = ClearDepthStencilView;
   pDeviceFuncs->pfnSetPredication = SetPredication;
   pDeviceFuncs->pfnQueryGetData = QueryGetData;
   pDeviceFuncs->pfnFlush = Flush;
   pDeviceFuncs->pfnGenMips = GenMips;
   pDeviceFuncs->pfnResourceCopy = ResourceCopy;
   pDeviceFuncs->pfnResourceResolveSubresource = ResourceResolveSubResource;
   pDeviceFuncs->pfnResourceMap = ResourceMap;
   pDeviceFuncs->pfnResourceUnmap = ResourceUnmap;
   pDeviceFuncs->pfnResourceIsStagingBusy = ResourceIsStagingBusy;
   pDeviceFuncs->pfnRelocateDeviceFuncs = RelocateDeviceFuncs;
   pDeviceFuncs->pfnCalcPrivateResourceSize = CalcPrivateResourceSize;
   pDeviceFuncs->pfnCalcPrivateOpenedResourceSize = CalcPrivateOpenedResourceSize;
   pDeviceFuncs->pfnCreateResource = CreateResource;
   pDeviceFuncs->pfnOpenResource = OpenResource;
   pDeviceFuncs->pfnDestroyResource = DestroyResource;
   pDeviceFuncs->pfnCalcPrivateShaderResourceViewSize = CalcPrivateShaderResourceViewSize;
   pDeviceFuncs->pfnCreateShaderResourceView = CreateShaderResourceView;
   pDeviceFuncs->pfnDestroyShaderResourceView = DestroyShaderResourceView;
   pDeviceFuncs->pfnCalcPrivateRenderTargetViewSize = CalcPrivateRenderTargetViewSize;
   pDeviceFuncs->pfnCreateRenderTargetView = CreateRenderTargetView;
   pDeviceFuncs->pfnDestroyRenderTargetView = DestroyRenderTargetView;
   pDeviceFuncs->pfnCalcPrivateDepthStencilViewSize = CalcPrivateDepthStencilViewSize;
   pDeviceFuncs->pfnCreateDepthStencilView = CreateDepthStencilView;
   pDeviceFuncs->pfnDestroyDepthStencilView = DestroyDepthStencilView;
   pDeviceFuncs->pfnCalcPrivateElementLayoutSize = CalcPrivateElementLayoutSize;
   pDeviceFuncs->pfnCreateElementLayout = CreateElementLayout;
   pDeviceFuncs->pfnDestroyElementLayout = DestroyElementLayout;
   pDeviceFuncs->pfnCalcPrivateBlendStateSize = CalcPrivateBlendStateSize;
   pDeviceFuncs->pfnCreateBlendState = CreateBlendState;
   pDeviceFuncs->pfnDestroyBlendState = DestroyBlendState;
   pDeviceFuncs->pfnCalcPrivateDepthStencilStateSize = CalcPrivateDepthStencilStateSize;
   pDeviceFuncs->pfnCreateDepthStencilState = CreateDepthStencilState;
   pDeviceFuncs->pfnDestroyDepthStencilState = DestroyDepthStencilState;
   pDeviceFuncs->pfnCalcPrivateRasterizerStateSize = CalcPrivateRasterizerStateSize;
   pDeviceFuncs->pfnCreateRasterizerState = CreateRasterizerState;
   pDeviceFuncs->pfnDestroyRasterizerState = DestroyRasterizerState;
   pDeviceFuncs->pfnCalcPrivateShaderSize = CalcPrivateShaderSize;
   pDeviceFuncs->pfnCreateVertexShader = CreateVertexShader;
   pDeviceFuncs->pfnCreateGeometryShader = CreateGeometryShader;
   pDeviceFuncs->pfnCreatePixelShader = CreatePixelShader;
   pDeviceFuncs->pfnCalcPrivateGeometryShaderWithStreamOutput = CalcPrivateGeometryShaderWithStreamOutput;
   pDeviceFuncs->pfnCreateGeometryShaderWithStreamOutput = CreateGeometryShaderWithStreamOutput;
   pDeviceFuncs->pfnDestroyShader = DestroyShader;
   pDeviceFuncs->pfnCalcPrivateSamplerSize = CalcPrivateSamplerSize;
   pDeviceFuncs->pfnCreateSampler = CreateSampler;
   pDeviceFuncs->pfnDestroySampler = DestroySampler;
   pDeviceFuncs->pfnCalcPrivateQuerySize = CalcPrivateQuerySize;
   pDeviceFuncs->pfnCreateQuery = CreateQuery;
   pDeviceFuncs->pfnDestroyQuery = DestroyQuery;
   pDeviceFuncs->pfnCheckFormatSupport = CheckFormatSupport;
   pDeviceFuncs->pfnCheckMultisampleQualityLevels = CheckMultisampleQualityLevels;
   pDeviceFuncs->pfnCheckCounterInfo = CheckCounterInfo;
   pDeviceFuncs->pfnCheckCounter = CheckCounter;
   pDeviceFuncs->pfnDestroyDevice = DestroyDevice;
   pDeviceFuncs->pfnSetTextFilterSize = SetTextFilterSize;
   if (pCreateData->Interface == D3D10_1_DDI_INTERFACE_VERSION ||
      pCreateData->Interface == D3D10_1_x_DDI_INTERFACE_VERSION ||
   pCreateData->Interface == D3D10_1_7_DDI_INTERFACE_VERSION) {
   D3D10_1DDI_DEVICEFUNCS *p10_1DeviceFuncs = pCreateData->p10_1DeviceFuncs;
   p10_1DeviceFuncs->pfnRelocateDeviceFuncs = RelocateDeviceFuncs1;
   p10_1DeviceFuncs->pfnCalcPrivateShaderResourceViewSize = CalcPrivateShaderResourceViewSize1;
   p10_1DeviceFuncs->pfnCreateShaderResourceView = CreateShaderResourceView1;
   p10_1DeviceFuncs->pfnCalcPrivateBlendStateSize = CalcPrivateBlendStateSize1;
   p10_1DeviceFuncs->pfnCreateBlendState = CreateBlendState1;
   p10_1DeviceFuncs->pfnResourceConvert = ResourceCopy;
               /*
   * Fill in DXGI DDI functions
   */
   pCreateData->DXGIBaseDDI.pDXGIDDIBaseFunctions->pfnPresent =
         pCreateData->DXGIBaseDDI.pDXGIDDIBaseFunctions->pfnGetGammaCaps =
         pCreateData->DXGIBaseDDI.pDXGIDDIBaseFunctions->pfnSetDisplayMode =
         pCreateData->DXGIBaseDDI.pDXGIDDIBaseFunctions->pfnSetResourcePriority =
         pCreateData->DXGIBaseDDI.pDXGIDDIBaseFunctions->pfnQueryResourceResidency =
         pCreateData->DXGIBaseDDI.pDXGIDDIBaseFunctions->pfnRotateResourceIdentities =
         pCreateData->DXGIBaseDDI.pDXGIDDIBaseFunctions->pfnBlt =
            if (0) {
         } else {
      // Tell DXGI to not use the shared resource presentation path when
   // communicating with DWM:
   // http://msdn.microsoft.com/en-us/library/windows/hardware/ff569887(v=vs.85).aspx
         }
         /*
   * ----------------------------------------------------------------------
   *
   * DestroyDevice --
   *
   *    The DestroyDevice function destroys a graphics context.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DestroyDevice(D3D10DDI_HDEVICE hDevice)   // IN
   {
                        Device *pDevice = CastDevice(hDevice);
                     for (i = 0; i < PIPE_MAX_SO_BUFFERS; ++i) {
         }
   if (pDevice->draw_so_target) {
                  pipe->bind_fs_state(pipe, NULL);
   pipe->bind_vs_state(pipe, NULL);
            DeleteEmptyShader(pDevice, PIPE_SHADER_FRAGMENT, pDevice->empty_fs);
            pipe_surface_reference(&pDevice->fb.zsbuf, NULL);
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; ++i) {
                  for (i = 0; i < PIPE_MAX_ATTRIBS; ++i) {
      if (!pDevice->vertex_buffers[i].is_user_buffer) {
                              static struct pipe_sampler_view * sampler_views[PIPE_MAX_SHADER_SAMPLER_VIEWS];
   memset(sampler_views, 0, sizeof sampler_views);
   pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0,
         pipe->set_sampler_views(pipe, PIPE_SHADER_VERTEX, 0,
         pipe->set_sampler_views(pipe, PIPE_SHADER_GEOMETRY, 0,
               }
         /*
   * ----------------------------------------------------------------------
   *
   * RelocateDeviceFuncs --
   *
   *    The RelocateDeviceFuncs function notifies the user-mode
   *    display driver about the new location of the driver function table.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   RelocateDeviceFuncs(D3D10DDI_HDEVICE hDevice,                           // IN
         {
               /*
   * Nothing to do as we don't store a pointer to this entity.
      }
         /*
   * ----------------------------------------------------------------------
   *
   * RelocateDeviceFuncs1 --
   *
   *    The RelocateDeviceFuncs function notifies the user-mode
   *    display driver about the new location of the driver function table.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   RelocateDeviceFuncs1(D3D10DDI_HDEVICE hDevice,                           // IN
         {
               /*
   * Nothing to do as we don't store a pointer to this entity.
      }
         /*
   * ----------------------------------------------------------------------
   *
   * Flush --
   *
   *    The Flush function submits outstanding hardware commands that
   *    are in the hardware command buffer to the display miniport driver.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   Flush(D3D10DDI_HDEVICE hDevice)  // IN
   {
                           }
         /*
   * ----------------------------------------------------------------------
   *
   * CheckFormatSupport --
   *
   *    The CheckFormatSupport function retrieves the capabilites that
   *    the device has with the specified format.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CheckFormatSupport(D3D10DDI_HDEVICE hDevice, // IN
               {
               struct pipe_context *pipe = CastPipeContext(hDevice);
                     enum pipe_format format = FormatTranslate(Format, false);
   if (format == PIPE_FORMAT_NONE) {
      *pFormatCaps = D3D10_DDI_FORMAT_SUPPORT_NOT_SUPPORTED;
               if (Format == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) {
      /*
   * We only need to support creation.
   * http://msdn.microsoft.com/en-us/library/windows/hardware/ff552818.aspx
   */
               if (screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, 0,
            *pFormatCaps |= D3D10_DDI_FORMAT_SUPPORT_RENDERTARGET;
      #if SUPPORT_MSAA
         if (screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 4, 4,
               #endif
               if (screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, 0,
               #if SUPPORT_MSAA
         if (screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 4, 4,
               #endif
         }
         /*
   * ----------------------------------------------------------------------
   *
   * CheckMultisampleQualityLevels --
   *
   *    The CheckMultisampleQualityLevels function retrieves the number
   *    of quality levels that the device supports for the specified
   *    number of samples.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CheckMultisampleQualityLevels(D3D10DDI_HDEVICE hDevice,        // IN
                     {
               /* XXX: Disable MSAA */
      }
         /*
   * ----------------------------------------------------------------------
   *
   * SetTextFilterSize --
   *
   *    The SetTextFilterSize function sets the width and height
   *    of the monochrome convolution filter.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   SetTextFilterSize(D3D10DDI_HDEVICE hDevice,  // IN
               {
                  }
