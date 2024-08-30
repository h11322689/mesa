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
   * OutputMerger.cpp --
   *    Functions that manipulate the output merger state.
   */
         #include "OutputMerger.h"
   #include "State.h"
      #include "Debug.h"
   #include "Format.h"
      #include "util/u_framebuffer.h"
   #include "util/format/u_format.h"
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateRenderTargetViewSize --
   *
   *    The CalcPrivateRenderTargetViewSize function determines the size
   *    of the user-mode display driver's private region of memory
   *    (that is, the size of internal driver structures, not the size
   *    of the resource video memory) for a render target view.
   *
   * ----------------------------------------------------------------------
   */
         SIZE_T APIENTRY
   CalcPrivateRenderTargetViewSize(
      D3D10DDI_HDEVICE hDevice,                                               // IN
      {
         }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateRenderTargetView --
   *
   *    The CreateRenderTargetView function creates a render target view.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateRenderTargetView(
      D3D10DDI_HDEVICE hDevice,                                               // IN
   __in const D3D10DDIARG_CREATERENDERTARGETVIEW *pCreateRenderTargetView, // IN
   D3D10DDI_HRENDERTARGETVIEW hRenderTargetView,                           // IN
      {
               struct pipe_context *pipe = CastPipeContext(hDevice);
   struct pipe_resource *resource = CastPipeResource(pCreateRenderTargetView->hDrvResource);
                     memset(&desc, 0, sizeof desc);
            switch (pCreateRenderTargetView->ResourceDimension) {
   case D3D10DDIRESOURCE_BUFFER:
      desc.u.buf.first_element = pCreateRenderTargetView->Buffer.FirstElement;
   desc.u.buf.last_element = pCreateRenderTargetView->Buffer.NumElements - 1 +
            case D3D10DDIRESOURCE_TEXTURE1D:
      ASSERT(pCreateRenderTargetView->Tex1D.ArraySize != (UINT)-1);
   desc.u.tex.level = pCreateRenderTargetView->Tex1D.MipSlice;
   desc.u.tex.first_layer = pCreateRenderTargetView->Tex1D.FirstArraySlice;
   desc.u.tex.last_layer = pCreateRenderTargetView->Tex1D.ArraySize - 1 +
            case D3D10DDIRESOURCE_TEXTURE2D:
      ASSERT(pCreateRenderTargetView->Tex2D.ArraySize != (UINT)-1);
   desc.u.tex.level = pCreateRenderTargetView->Tex2D.MipSlice;
   desc.u.tex.first_layer = pCreateRenderTargetView->Tex2D.FirstArraySlice;
   desc.u.tex.last_layer = pCreateRenderTargetView->Tex2D.ArraySize - 1 +
            case D3D10DDIRESOURCE_TEXTURE3D:
      desc.u.tex.level = pCreateRenderTargetView->Tex3D.MipSlice;
   desc.u.tex.first_layer = pCreateRenderTargetView->Tex3D.FirstW;
   desc.u.tex.last_layer = pCreateRenderTargetView->Tex3D.WSize - 1 +
            case D3D10DDIRESOURCE_TEXTURECUBE:
      ASSERT(pCreateRenderTargetView->TexCube.ArraySize != (UINT)-1);
   desc.u.tex.level = pCreateRenderTargetView->TexCube.MipSlice;
   desc.u.tex.first_layer = pCreateRenderTargetView->TexCube.FirstArraySlice;
   desc.u.tex.last_layer = pCreateRenderTargetView->TexCube.ArraySize - 1 +
            default:
      ASSERT(0);
               pRTView->surface = pipe->create_surface(pipe, resource, &desc);
      }
         /*
   * ----------------------------------------------------------------------
   *
   * DestroyRenderTargetView --
   *
   *    The DestroyRenderTargetView function destroys the specified
   *    render target view object. The render target view object can
   *    be destoyed only if it is not currently bound to a display device.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DestroyRenderTargetView(D3D10DDI_HDEVICE hDevice,                       // IN
         {
                           }
         /*
   * ----------------------------------------------------------------------
   *
   * ClearRenderTargetView --
   *
   *    The ClearRenderTargetView function clears the specified
   *    render target view by setting it to a constant value.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   ClearRenderTargetView(D3D10DDI_HDEVICE hDevice,                      // IN
               {
               struct pipe_context *pipe = CastPipeContext(hDevice);
   struct pipe_surface *surface = CastPipeRenderTargetView(hRenderTargetView);
            /*
   * DX10 always uses float clear color but gallium does not.
   * Conversion should just be ordinary conversion. Actual clamping will
   * be done later but need to make sure values exceeding int/uint range
   * are handled correctly.
   */
   if (util_format_is_pure_integer(surface->format)) {
      if (util_format_is_pure_sint(surface->format)) {
      unsigned i;
   /* If only MIN_INT/UINT32 in c++ code would work... */
   int min_int32 = 0x80000000;
   int max_int32 = 0x7fffffff;
   for (i = 0; i < 4; i++) {
      float value = pColorRGBA[i];
   /* This is an expanded clamp to handle NaN and integer conversion. */
   if (util_is_nan(value)) {
         } else if (value <= (float)min_int32) {
         } else if (value >= (float)max_int32) {
         } else {
               }
   else {
      assert(util_format_is_pure_uint(surface->format));
   unsigned i;
   unsigned max_uint32 = 0xffffffffU;
   for (i = 0; i < 4; i++) {
      float value = pColorRGBA[i];
   /* This is an expanded clamp to handle NaN and integer conversion. */
   if (!(value >= 0.0f)) {
      /* Handles NaN. */
      } else if (value >= (float)max_uint32) {
         } else {
                  }
   else {
      clear_color.f[0] = pColorRGBA[0];
   clear_color.f[1] = pColorRGBA[1];
   clear_color.f[2] = pColorRGBA[2];
               pipe->clear_render_target(pipe,
                           surface,
      }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateDepthStencilViewSize --
   *
   *    The CalcPrivateDepthStencilViewSize function determines the size
   *    of the user-mode display driver's private region of memory
   *    (that is, the size of internal driver structures, not the size
   *    of the resource video memory) for a depth stencil view.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateDepthStencilViewSize(
      D3D10DDI_HDEVICE hDevice,                                               // IN
      {
         }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateDepthStencilView --
   *
   *    The CreateDepthStencilView function creates a depth stencil view.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateDepthStencilView(
      D3D10DDI_HDEVICE hDevice,                                               // IN
   __in const D3D10DDIARG_CREATEDEPTHSTENCILVIEW *pCreateDepthStencilView, // IN
   D3D10DDI_HDEPTHSTENCILVIEW hDepthStencilView,                           // IN
      {
               struct pipe_context *pipe = CastPipeContext(hDevice);
   struct pipe_resource *resource = CastPipeResource(pCreateDepthStencilView->hDrvResource);
                     memset(&desc, 0, sizeof desc);
            switch (pCreateDepthStencilView->ResourceDimension) {
   case D3D10DDIRESOURCE_TEXTURE1D:
      ASSERT(pCreateDepthStencilView->Tex1D.ArraySize != (UINT)-1);
   desc.u.tex.level = pCreateDepthStencilView->Tex1D.MipSlice;
   desc.u.tex.first_layer = pCreateDepthStencilView->Tex1D.FirstArraySlice;
   desc.u.tex.last_layer = pCreateDepthStencilView->Tex1D.ArraySize - 1 +
            case D3D10DDIRESOURCE_TEXTURE2D:
      ASSERT(pCreateDepthStencilView->Tex2D.ArraySize != (UINT)-1);
   desc.u.tex.level = pCreateDepthStencilView->Tex2D.MipSlice;
   desc.u.tex.first_layer = pCreateDepthStencilView->Tex2D.FirstArraySlice;
   desc.u.tex.last_layer = pCreateDepthStencilView->Tex2D.ArraySize - 1 +
            case D3D10DDIRESOURCE_TEXTURECUBE:
      ASSERT(pCreateDepthStencilView->TexCube.ArraySize != (UINT)-1);
   desc.u.tex.level = pCreateDepthStencilView->TexCube.MipSlice;
   desc.u.tex.first_layer = pCreateDepthStencilView->TexCube.FirstArraySlice;
   desc.u.tex.last_layer = pCreateDepthStencilView->TexCube.ArraySize - 1 +
            default:
      ASSERT(0);
               pDSView->surface = pipe->create_surface(pipe, resource, &desc);
      }
         /*
   * ----------------------------------------------------------------------
   *
   * DestroyDepthStencilView --
   *
   *    The DestroyDepthStencilView function destroys the specified
   *    depth stencil view object. The depth stencil view object can
   *    be destoyed only if it is not currently bound to a display device.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DestroyDepthStencilView(D3D10DDI_HDEVICE hDevice,                       // IN
         {
                           }
         /*
   * ----------------------------------------------------------------------
   *
   * ClearDepthStencilView --
   *
   *    The ClearDepthStencilView function clears the specified
   *    currently bound depth-stencil view.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   ClearDepthStencilView(D3D10DDI_HDEVICE hDevice,                      // IN
                           {
               struct pipe_context *pipe = CastPipeContext(hDevice);
            unsigned flags = 0;
   if (Flags & D3D10_DDI_CLEAR_DEPTH) {
         }
   if (Flags & D3D10_DDI_CLEAR_STENCIL) {
                  pipe->clear_depth_stencil(pipe,
                           surface,
   flags,
   Depth,
      }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateBlendStateSize --
   *
   *    The CalcPrivateBlendStateSize function determines the size of
   *    the user-mode display driver's private region of memory (that
   *    is, the size of internal driver structures, not the size of
   *    the resource video memory) for a blend state.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateBlendStateSize(D3D10DDI_HDEVICE hDevice,                     // IN
         {
         }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateBlendStateSize1 --
   *
   *    The CalcPrivateBlendStateSize function determines the size of
   *    the user-mode display driver's private region of memory (that
   *    is, the size of internal driver structures, not the size of
   *    the resource video memory) for a blend state.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateBlendStateSize1(D3D10DDI_HDEVICE hDevice,                     // IN
         {
         }
         /*
   * ----------------------------------------------------------------------
   *
   * translateBlend --
   *
   *   Translate blend function from svga3d to gallium representation.
   *
   * ----------------------------------------------------------------------
   */
   static uint
   translateBlendOp(D3D10_DDI_BLEND_OP op)
   {
      switch (op) {
   case D3D10_DDI_BLEND_OP_ADD:
         case D3D10_DDI_BLEND_OP_SUBTRACT:
         case D3D10_DDI_BLEND_OP_REV_SUBTRACT:
         case D3D10_DDI_BLEND_OP_MIN:
         case D3D10_DDI_BLEND_OP_MAX:
         default:
      assert(0);
         }
         /*
   * ----------------------------------------------------------------------
   *
   * translateBlend --
   *
   *   Translate blend factor from svga3d to gallium representation.
   *
   * ----------------------------------------------------------------------
   */
   static uint
   translateBlend(Device *pDevice,
         {
      if (!pDevice->max_dual_source_render_targets) {
      switch (blend) {
   case D3D10_DDI_BLEND_SRC1_COLOR:
   case D3D10_DDI_BLEND_SRC1_ALPHA:
      LOG_UNSUPPORTED(true);
      case D3D10_DDI_BLEND_INV_SRC1_COLOR:
   case D3D10_DDI_BLEND_INV_SRC1_ALPHA:
      LOG_UNSUPPORTED(true);
      default:
                     switch (blend) {
   case D3D10_DDI_BLEND_ZERO:
         case D3D10_DDI_BLEND_ONE:
         case D3D10_DDI_BLEND_SRC_COLOR:
         case D3D10_DDI_BLEND_INV_SRC_COLOR:
         case D3D10_DDI_BLEND_SRC_ALPHA:
         case D3D10_DDI_BLEND_INV_SRC_ALPHA:
         case D3D10_DDI_BLEND_DEST_ALPHA:
         case D3D10_DDI_BLEND_INV_DEST_ALPHA:
         case D3D10_DDI_BLEND_DEST_COLOR:
         case D3D10_DDI_BLEND_INV_DEST_COLOR:
         case D3D10_DDI_BLEND_SRC_ALPHASAT:
         case D3D10_DDI_BLEND_BLEND_FACTOR:
         case D3D10_DDI_BLEND_INVBLEND_FACTOR:
         case D3D10_DDI_BLEND_SRC1_COLOR:
         case D3D10_DDI_BLEND_INV_SRC1_COLOR:
         case D3D10_DDI_BLEND_SRC1_ALPHA:
         case D3D10_DDI_BLEND_INV_SRC1_ALPHA:
         default:
      assert(0);
         }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateBlendState --
   *
   *    The CreateBlendState function creates a blend state.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateBlendState(D3D10DDI_HDEVICE hDevice,                     // IN
                     {
                        Device *pDevice = CastDevice(hDevice);
   struct pipe_context *pipe = pDevice->pipe;
            struct pipe_blend_state state;
            for (i = 0; i < MIN2(PIPE_MAX_COLOR_BUFS, D3D10_DDI_SIMULTANEOUS_RENDER_TARGET_COUNT); ++i) {
      state.rt[i].blend_enable = pBlendDesc->BlendEnable[i];
            if (pBlendDesc->BlendEnable[0] != pBlendDesc->BlendEnable[i] ||
      pBlendDesc->RenderTargetWriteMask[0] != pBlendDesc->RenderTargetWriteMask[i]) {
                  state.rt[0].rgb_func = translateBlendOp(pBlendDesc->BlendOp);
   if (pBlendDesc->BlendOp == D3D10_DDI_BLEND_OP_MIN ||
      pBlendDesc->BlendOp == D3D10_DDI_BLEND_OP_MAX) {
   state.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      } else {
      state.rt[0].rgb_src_factor = translateBlend(pDevice, pBlendDesc->SrcBlend);
               state.rt[0].alpha_func = translateBlendOp(pBlendDesc->BlendOpAlpha);
   if (pBlendDesc->BlendOpAlpha == D3D10_DDI_BLEND_OP_MIN ||
      pBlendDesc->BlendOpAlpha == D3D10_DDI_BLEND_OP_MAX) {
   state.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      } else {
      state.rt[0].alpha_src_factor = translateBlend(pDevice, pBlendDesc->SrcBlendAlpha);
               /*
   * Propagate to all the other rendertargets
   */
   for (i = 1; i < MIN2(PIPE_MAX_COLOR_BUFS, D3D10_DDI_SIMULTANEOUS_RENDER_TARGET_COUNT); ++i) {
      state.rt[i].rgb_func = state.rt[0].rgb_func;
   state.rt[i].rgb_src_factor = state.rt[0].rgb_src_factor;
   state.rt[i].rgb_dst_factor = state.rt[0].rgb_dst_factor;
   state.rt[i].alpha_func = state.rt[0].alpha_func;
   state.rt[i].alpha_src_factor = state.rt[0].alpha_src_factor;
                           }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateBlendState1 --
   *
   *    The CreateBlendState function creates a blend state.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateBlendState1(D3D10DDI_HDEVICE hDevice,                     // IN
                     {
                        Device *pDevice = CastDevice(hDevice);
   struct pipe_context *pipe = pDevice->pipe;
            struct pipe_blend_state state;
            state.alpha_to_coverage = pBlendDesc->AlphaToCoverageEnable;
            for (i = 0; i < MIN2(PIPE_MAX_COLOR_BUFS, D3D10_DDI_SIMULTANEOUS_RENDER_TARGET_COUNT); ++i) {
      state.rt[i].blend_enable = pBlendDesc->RenderTarget[i].BlendEnable;
            state.rt[i].rgb_func = translateBlendOp(pBlendDesc->RenderTarget[i].BlendOp);
   if (pBlendDesc->RenderTarget[i].BlendOp == D3D10_DDI_BLEND_OP_MIN ||
      pBlendDesc->RenderTarget[i].BlendOp == D3D10_DDI_BLEND_OP_MAX) {
   state.rt[i].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      } else {
      state.rt[i].rgb_src_factor = translateBlend(pDevice, pBlendDesc->RenderTarget[i].SrcBlend);
               state.rt[i].alpha_func = translateBlendOp(pBlendDesc->RenderTarget[i].BlendOpAlpha);
   if (pBlendDesc->RenderTarget[i].BlendOpAlpha == D3D10_DDI_BLEND_OP_MIN ||
      pBlendDesc->RenderTarget[i].BlendOpAlpha == D3D10_DDI_BLEND_OP_MAX) {
   state.rt[i].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      } else {
      state.rt[i].alpha_src_factor = translateBlend(pDevice, pBlendDesc->RenderTarget[i].SrcBlendAlpha);
                     }
         /*
   * ----------------------------------------------------------------------
   *
   * DestroyBlendState --
   *
   *    The DestroyBlendState function destroys the specified blend
   *    state object. The blend state object can be destoyed only if
   *    it is not currently bound to a display device.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DestroyBlendState(D3D10DDI_HDEVICE hDevice,           // IN
         {
               struct pipe_context *pipe = CastPipeContext(hDevice);
               }
         /*
   * ----------------------------------------------------------------------
   *
   * SetBlendState --
   *
   *    The SetBlendState function sets a blend state.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   SetBlendState(D3D10DDI_HDEVICE hDevice,      // IN
               D3D10DDI_HBLENDSTATE hState,   // IN
   {
               struct pipe_context *pipe = CastPipeContext(hDevice);
                     struct pipe_blend_color color;
   color.color[0] = pBlendFactor[0];
   color.color[1] = pBlendFactor[1];
   color.color[2] = pBlendFactor[2];
                        }
         /*
   * ----------------------------------------------------------------------
   *
   * SetRenderTargets --
   *
   *    Set the rendertargets.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   SetRenderTargets(D3D10DDI_HDEVICE hDevice,                              // IN
                  __in_ecount (NumViews)
            {
                                 pDevice->fb.nr_cbufs = 0;
   for (unsigned i = 0; i < RTargets; ++i) {
      pipe_surface_reference(&pDevice->fb.cbufs[i],
         if (pDevice->fb.cbufs[i]) {
                     for (unsigned i = RTargets; i < PIPE_MAX_COLOR_BUFS; ++i) {
                  pipe_surface_reference(&pDevice->fb.zsbuf,
            /*
   * Calculate the width/height fields for this framebuffer.  D3D10
   * actually specifies that they be identical for all bound views.
   */
   unsigned width, height;
   util_framebuffer_min_size(&pDevice->fb, &width, &height);
   pDevice->fb.width = width;
               }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateDepthStencilStateSize --
   *
   *    The CalcPrivateDepthStencilStateSize function determines the size
   *    of the user-mode display driver's private region of memory (that
   *    is, the size of internal driver structures, not the size of the
   *    resource video memory) for a depth stencil state.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateDepthStencilStateSize(
      D3D10DDI_HDEVICE hDevice,                                   // IN
      {
         }
         /*
   * ----------------------------------------------------------------------
   *
   * translateComparison --
   *
   *   Translate comparison function from DX10 to gallium representation.
   *
   * ----------------------------------------------------------------------
   */
   static uint
   translateComparison(D3D10_DDI_COMPARISON_FUNC Func)
   {
      switch (Func) {
   case D3D10_DDI_COMPARISON_NEVER:
         case D3D10_DDI_COMPARISON_LESS:
         case D3D10_DDI_COMPARISON_EQUAL:
         case D3D10_DDI_COMPARISON_LESS_EQUAL:
         case D3D10_DDI_COMPARISON_GREATER:
         case D3D10_DDI_COMPARISON_NOT_EQUAL:
         case D3D10_DDI_COMPARISON_GREATER_EQUAL:
         case D3D10_DDI_COMPARISON_ALWAYS:
         default:
      assert(0);
         }
         /*
   * ----------------------------------------------------------------------
   *
   * translateStencilOp --
   *
   *   Translate stencil op from DX10 to gallium representation.
   *
   * ----------------------------------------------------------------------
   */
   static uint
   translateStencilOp(D3D10_DDI_STENCIL_OP StencilOp)
   {
      switch (StencilOp) {
   case D3D10_DDI_STENCIL_OP_KEEP:
         case D3D10_DDI_STENCIL_OP_ZERO:
         case D3D10_DDI_STENCIL_OP_REPLACE:
         case D3D10_DDI_STENCIL_OP_INCR_SAT:
         case D3D10_DDI_STENCIL_OP_DECR_SAT:
         case D3D10_DDI_STENCIL_OP_INVERT:
         case D3D10_DDI_STENCIL_OP_INCR:
         case D3D10_DDI_STENCIL_OP_DECR:
         default:
      assert(0);
         }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateDepthStencilState --
   *
   *    The CreateDepthStencilState function creates a depth stencil state.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateDepthStencilState(
      D3D10DDI_HDEVICE hDevice,                                   // IN
   __in const D3D10_DDI_DEPTH_STENCIL_DESC *pDepthStencilDesc, // IN
   D3D10DDI_HDEPTHSTENCILSTATE hDepthStencilState,             // IN
      {
               struct pipe_context *pipe = CastPipeContext(hDevice);
            struct pipe_depth_stencil_alpha_state state;
            /* Depth. */
   state.depth_enabled = (pDepthStencilDesc->DepthEnable ? 1 : 0);
   state.depth_writemask = (pDepthStencilDesc->DepthWriteMask ? 1 : 0);
            /* Stencil. */
   if (pDepthStencilDesc->StencilEnable) {
      struct pipe_stencil_state *face0 = &state.stencil[0];
            face0->enabled   = 1;
   face0->func      = translateComparison(pDepthStencilDesc->FrontFace.StencilFunc);
   face0->fail_op   = translateStencilOp(pDepthStencilDesc->FrontFace.StencilFailOp);
   face0->zpass_op  = translateStencilOp(pDepthStencilDesc->FrontFace.StencilPassOp);
   face0->zfail_op  = translateStencilOp(pDepthStencilDesc->FrontFace.StencilDepthFailOp);
   face0->valuemask = pDepthStencilDesc->StencilReadMask;
            face1->enabled   = 1;
   face1->func      = translateComparison(pDepthStencilDesc->BackFace.StencilFunc);
   face1->fail_op   = translateStencilOp(pDepthStencilDesc->BackFace.StencilFailOp);
   face1->zpass_op  = translateStencilOp(pDepthStencilDesc->BackFace.StencilPassOp);
   face1->zfail_op  = translateStencilOp(pDepthStencilDesc->BackFace.StencilDepthFailOp);
   face1->valuemask = pDepthStencilDesc->StencilReadMask;
   #ifdef DEBUG
         if (!pDepthStencilDesc->FrontEnable) {
      ASSERT(face0->func == PIPE_FUNC_ALWAYS);
   ASSERT(face0->fail_op == PIPE_STENCIL_OP_KEEP);
   ASSERT(face0->zpass_op == PIPE_STENCIL_OP_KEEP);
               if (!pDepthStencilDesc->BackEnable) {
      ASSERT(face1->func == PIPE_FUNC_ALWAYS);
   ASSERT(face1->fail_op == PIPE_STENCIL_OP_KEEP);
   ASSERT(face1->zpass_op == PIPE_STENCIL_OP_KEEP);
      #endif
               pDepthStencilState->handle =
      }
         /*
   * ----------------------------------------------------------------------
   *
   * DestroyDepthStencilState --
   *
   *    The CreateDepthStencilState function creates a depth stencil state.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DestroyDepthStencilState(D3D10DDI_HDEVICE hDevice,                         // IN
         {
               struct pipe_context *pipe = CastPipeContext(hDevice);
               }
         /*
   * ----------------------------------------------------------------------
   *
   * SetDepthStencilState --
   *
   *    The SetDepthStencilState function sets a depth-stencil state.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   SetDepthStencilState(D3D10DDI_HDEVICE hDevice,           // IN
               {
               struct pipe_context *pipe = CastPipeContext(hDevice);
   void *state = CastPipeDepthStencilState(hState);
            psr.ref_value[0] = StencilRef;
            pipe->bind_depth_stencil_alpha_state(pipe, state);
      }
