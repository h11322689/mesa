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
   * DxgiFns.cpp --
   *    DXGI related functions.
   */
      #include <stdio.h>
      #include "DxgiFns.h"
   #include "Format.h"
   #include "State.h"
      #include "Debug.h"
      #include "util/format/u_format.h"
         /*
   * ----------------------------------------------------------------------
   *
   * _Present --
   *
   *    This is turned into kernel callbacks rather than directly emitted
   *    as fifo packets.
   *
   * ----------------------------------------------------------------------
   */
      HRESULT APIENTRY
   _Present(DXGI_DDI_ARG_PRESENT *pPresentData)
   {
                  struct pipe_context *pipe = CastPipeDevice(pPresentData->hDevice);
                              if (0) {
      DebugPrintf("  hWindow = 0x%08lx\n", pPresentInfo->hWindow);
   if (pPresentInfo->Flags.SrcRectValid) {
      DebugPrintf("  SrcRect.left = %li\n", pPresentInfo->SrcRect.left);
   DebugPrintf("  SrcRect.top = %li\n", pPresentInfo->SrcRect.top);
   DebugPrintf("  SrcRect.right = %li\n", pPresentInfo->SrcRect.right);
      }
   if (pPresentInfo->Flags.DstRectValid) {
      DebugPrintf("  DstRect.left = %li\n", pPresentInfo->DstRect.left);
   DebugPrintf("  DstRect.top = %li\n", pPresentInfo->DstRect.top);
   DebugPrintf("  DstRect.right = %li\n", pPresentInfo->DstRect.right);
                  RECT rect;
   if (!GetClientRect(hWnd, &rect)) {
      DebugPrintf("Invalid window.\n");
               int windowWidth  = rect.right  - rect.left;
                     unsigned w = pSrcResource->resource->width0;
            void *map;
   struct pipe_transfer *transfer;
   map = pipe_texture_map(pipe,
                                                memset(&bmi, 0, sizeof bmi);
   bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bmi.bmiHeader.biWidth = w;
   bmi.bmiHeader.biHeight= -(long)h;
   bmi.bmiHeader.biPlanes = 1;
   bmi.bmiHeader.biBitCount = 32;
   bmi.bmiHeader.biCompression = BI_RGB;
   bmi.bmiHeader.biSizeImage = 0;
   bmi.bmiHeader.biXPelsPerMeter = 0;
   bmi.bmiHeader.biYPelsPerMeter = 0;
   bmi.bmiHeader.biClrUsed = 0;
                                       util_format_translate(
         PIPE_FORMAT_B8G8R8X8_UNORM,
   (void *)pixels, w * 4,
   0, 0,
   pSrcResource->resource->format,
            if (0) {
      /*
                  FILE *fp = fopen("present.bmp", "wb");
   if (fp) {
      BITMAPFILEHEADER bmf;
   bmf.bfType = 0x4d42;
   bmf.bfSize = sizeof bmf + sizeof bmi + h * w * 4;
   bmf.bfReserved1 = 0;
                  fwrite(&bmf, sizeof bmf, 1, fp);
   fwrite(&bmi, sizeof bmi, 1, fp);
   fwrite(pixels, h, w * 4, fp);
                  HDC hdcMem;
   hdcMem = CreateCompatibleDC(hDC);
                     StretchBlt(hDC, 0, 0, windowWidth, windowHeight,
                  if (iStretchMode) {
                  SelectObject(hdcMem, hbmOld);
   DeleteDC(hdcMem);
                                    }
         /*
   * ----------------------------------------------------------------------
   *
   * _GetGammaCaps --
   *
   *    Return gamma capabilities.
   *
   * ----------------------------------------------------------------------
   */
      HRESULT APIENTRY
   _GetGammaCaps( DXGI_DDI_ARG_GET_GAMMA_CONTROL_CAPS *GetCaps )
   {
                                 pCaps->ScaleAndOffsetSupported = false;
   pCaps->MinConvertedValue = 0.0;
   pCaps->MaxConvertedValue = 1.0;
            for (UINT i = 0; i < pCaps->NumGammaControlPoints; i++) {
                     }
         /*
   * ----------------------------------------------------------------------
   *
   * _SetDisplayMode --
   *
   *    Set the resource that is used to scan out to the display.
   *
   * ----------------------------------------------------------------------
   */
      HRESULT APIENTRY
   _SetDisplayMode( DXGI_DDI_ARG_SETDISPLAYMODE *SetDisplayMode )
   {
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * _SetResourcePriority --
   *
   * ----------------------------------------------------------------------
   */
      HRESULT APIENTRY
   _SetResourcePriority( DXGI_DDI_ARG_SETRESOURCEPRIORITY *SetResourcePriority )
   {
                           }
         /*
   * ----------------------------------------------------------------------
   *
   * _QueryResourceResidency --
   *
   * ----------------------------------------------------------------------
   */
      HRESULT APIENTRY
   _QueryResourceResidency( DXGI_DDI_ARG_QUERYRESOURCERESIDENCY *QueryResourceResidency )
   {
               for (UINT i = 0; i < QueryResourceResidency->Resources; ++i) {
                     }
         /*
   * ----------------------------------------------------------------------
   *
   * _RotateResourceIdentities --
   *
   *    Rotate a list of resources by recreating their views with
   *    the updated rotations.
   *
   * ----------------------------------------------------------------------
   */
      HRESULT APIENTRY
   _RotateResourceIdentities( DXGI_DDI_ARG_ROTATE_RESOURCE_IDENTITIES *RotateResourceIdentities )
   {
               if (RotateResourceIdentities->Resources <= 1) {
                  struct pipe_context *pipe = CastPipeDevice(RotateResourceIdentities->hDevice);
                     assert(resource0);
            /*
   * XXX: Copying is not very efficient, but it is much simpler than the
   * alternative of recreating all views.
            struct pipe_resource *temp_resource;
   temp_resource = screen->resource_create(screen, resource0);
   assert(temp_resource);
   if (!temp_resource) {
                  struct pipe_box src_box;
   src_box.x = 0;
   src_box.y = 0;
   src_box.z = 0;
   src_box.width  = resource0->width0;
   src_box.height = resource0->height0;
            for (UINT i = 0; i < RotateResourceIdentities->Resources + 1; ++i) {
      struct pipe_resource *src_resource;
            if (i < RotateResourceIdentities->Resources) {
         } else {
                  if (i > 0) {
         } else {
                  assert(dst_resource);
            pipe->resource_copy_region(pipe,
                              dst_resource,
                        }
         /*
   * ----------------------------------------------------------------------
   *
   * _Blt --
   *
   *    Do a blt between two subresources. Apply MSAA resolve, format
   *    conversion and stretching.
   *
   * ----------------------------------------------------------------------
   */
      HRESULT APIENTRY
   _Blt(DXGI_DDI_ARG_BLT *Blt)
   {
                  }
