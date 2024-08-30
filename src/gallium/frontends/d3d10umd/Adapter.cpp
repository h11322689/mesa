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
   * Adpater.cpp --
   *    Driver entry point.
   */
         #include "DriverIncludes.h"
   #include "Device.h"
   #include "State.h"
      #include "Debug.h"
      #include "util/u_memory.h"
         EXTERN_C struct pipe_screen *
   d3d10_create_screen(void);
         static HRESULT APIENTRY CloseAdapter(D3D10DDI_HADAPTER hAdapter);
      static unsigned long numAdapters = 0;
   #if 0
   static unsigned long memdbg_no = 0;
   #endif
      /*
   * ----------------------------------------------------------------------
   *
   * OpenAdapterCommon --
   *
   *    Common code for OpenAdapter10 and OpenAdapter10_2
   *
   * ----------------------------------------------------------------------
   */
         static HRESULT
   OpenAdapterCommon(__inout D3D10DDIARG_OPENADAPTER *pOpenData)   // IN
   {
   #if 0
      if (numAdapters == 0) {
            #endif
               Adapter *pAdaptor = (Adapter *)calloc(sizeof *pAdaptor, 1);
   if (!pAdaptor) {
      --numAdapters;
               pAdaptor->screen = d3d10_create_screen();
   if (!pAdaptor->screen) {
      free(pAdaptor);
   --numAdapters;
                        pOpenData->pAdapterFuncs->pfnCalcPrivateDeviceSize = CalcPrivateDeviceSize;
   pOpenData->pAdapterFuncs->pfnCreateDevice = CreateDevice;
               }
         /*
   * ----------------------------------------------------------------------
   *
   * OpenAdapter10 --
   *
   *    The OpenAdapter10 function creates a graphics adapter object
   *    that is referenced in subsequent calls.
   *
   * ----------------------------------------------------------------------
   */
         EXTERN_C HRESULT APIENTRY
   OpenAdapter10(__inout D3D10DDIARG_OPENADAPTER *pOpenData)   // IN
   {
               /*
   * This is checked here and not on the common code because MSDN docs
   * state that it should be ignored on OpenAdapter10_2.
   */
   switch (pOpenData->Interface) {
   case D3D10_0_DDI_INTERFACE_VERSION:
   case D3D10_0_x_DDI_INTERFACE_VERSION:
      #if SUPPORT_D3D10_1
      case D3D10_1_DDI_INTERFACE_VERSION:
   case D3D10_1_x_DDI_INTERFACE_VERSION:
      #endif
   #if SUPPORT_D3D11
      case D3D11_0_DDI_INTERFACE_VERSION:
      #endif
            default:
      if (0) {
      DebugPrintf("%s: unsupported interface version 0x%08x\n",
      }
                  }
         static const UINT64
   SupportedDDIInterfaceVersions[] = {
      D3D10_0_DDI_SUPPORTED,
   D3D10_0_x_DDI_SUPPORTED,
      #if SUPPORT_D3D10_1
      D3D10_1_DDI_SUPPORTED,
   D3D10_1_x_DDI_SUPPORTED,
      #endif
   #if SUPPORT_D3D11
      D3D11_0_DDI_SUPPORTED,
      #endif
   };
         /*
   * ----------------------------------------------------------------------
   *
   * GetSupportedVersions --
   *
   *    Return a list of interface versions supported by the graphics
   *    adapter.
   *
   * ----------------------------------------------------------------------
   */
      static HRESULT APIENTRY
   GetSupportedVersions(D3D10DDI_HADAPTER hAdapter,
               {
               if (pSupportedDDIInterfaceVersions &&
      *puEntries < ARRAYSIZE(SupportedDDIInterfaceVersions)) {
                        if (pSupportedDDIInterfaceVersions) {
      memcpy(pSupportedDDIInterfaceVersions,
                        }
         /*
   * ----------------------------------------------------------------------
   *
   * GetCaps --
   *
   *    Return the capabilities of the graphics adapter.
   *
   * ----------------------------------------------------------------------
   */
      static HRESULT APIENTRY
   GetCaps(D3D10DDI_HADAPTER hAdapter,
         {
      LOG_ENTRYPOINT();
   memset(pData->pData, 0, pData->DataSize);
      }
         /*
   * ----------------------------------------------------------------------
   *
   * OpenAdapter10 --
   *
   *    The OpenAdapter10 function creates a graphics adapter object
   *    that is referenced in subsequent calls.
   *
   * ----------------------------------------------------------------------
   */
         EXTERN_C HRESULT APIENTRY
   OpenAdapter10_2(__inout D3D10DDIARG_OPENADAPTER *pOpenData)   // IN
   {
                        if (SUCCEEDED(hr)) {
      pOpenData->pAdapterFuncs_2->pfnGetSupportedVersions = GetSupportedVersions;
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * CloseAdapter --
   *
   *    The CloseAdapter function releases resources for a
   *    graphics adapter object.
   *
   * ----------------------------------------------------------------------
   */
      HRESULT APIENTRY
   CloseAdapter(D3D10DDI_HADAPTER hAdapter)  // IN
   {
               Adapter *pAdapter = CastAdapter(hAdapter);
   struct pipe_screen *screen = pAdapter->screen;
   screen->destroy(screen);
               #if 0
      if (numAdapters == 0) {
            #endif
            }
