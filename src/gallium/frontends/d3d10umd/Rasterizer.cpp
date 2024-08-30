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
   * Rasterizer.cpp --
   *    Functions that manipulate rasterizer state.
   */
         #include "Rasterizer.h"
   #include "State.h"
      #include "Debug.h"
         /*
   * ----------------------------------------------------------------------
   *
   * SetViewports --
   *
   *    The SetViewports function sets viewports.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   SetViewports(D3D10DDI_HDEVICE hDevice,                                        // IN
               UINT NumViewports,                                               // IN
   {
               struct pipe_context *pipe = CastPipeContext(hDevice);
            ASSERT(NumViewports + ClearViewports <=
            for (UINT i = 0; i < NumViewports; ++i) {
      const D3D10_DDI_VIEWPORT *pViewport = &pViewports[i];
   float width = pViewport->Width;
   float height = pViewport->Height;
   float x = pViewport->TopLeftX;
   float y = pViewport->TopLeftY;
   float z = pViewport->MinDepth;
   float half_width = width / 2.0f;
   float half_height = height / 2.0f;
            states[i].scale[0] = half_width;
   states[i].scale[1] = -half_height;
            states[i].translate[0] = half_width + x;
   states[i].translate[1] = half_height + y;
      }
   if (ClearViewports) {
      memset(states + NumViewports, 0,
      }
   pipe->set_viewport_states(pipe, 0, NumViewports + ClearViewports,
      }
         /*
   * ----------------------------------------------------------------------
   *
   * SetScissorRects --
   *
   *    The SetScissorRects function marks portions of render targets
   *    that rendering is confined to.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   SetScissorRects(D3D10DDI_HDEVICE hDevice,                            // IN
                     {
               struct pipe_context *pipe = CastPipeContext(hDevice);
            ASSERT(NumScissorRects + ClearScissorRects <=
            for (UINT i = 0; i < NumScissorRects; ++i) {
      const D3D10_DDI_RECT *pRect = &pRects[i];
   /* gallium scissor values are unsigned so lets make
   * sure that we don't overflow */
   states[i].minx = pRect->left   < 0 ? 0 : pRect->left;
   states[i].miny = pRect->top    < 0 ? 0 : pRect->top;
   states[i].maxx = pRect->right  < 0 ? 0 : pRect->right;
      }
   if (ClearScissorRects) {
      memset(states + NumScissorRects, 0,
      }
   pipe->set_scissor_states(pipe, 0, NumScissorRects + ClearScissorRects,
      }
         /*
   * ----------------------------------------------------------------------
   *
   * CalcPrivateRasterizerStateSize --
   *
   *    The CalcPrivateRasterizerStateSize function determines the size
   *    of the user-mode display driver's private region of memory
   *    (that is, the size of internal driver structures, not the size
   *    of the resource video memory) for a rasterizer state.
   *
   * ----------------------------------------------------------------------
   */
      SIZE_T APIENTRY
   CalcPrivateRasterizerStateSize(
      D3D10DDI_HDEVICE hDevice,                                // IN
      {
         }
         static uint
   translate_cull_mode(D3D10_DDI_CULL_MODE CullMode)
   {
      switch (CullMode) {
   case D3D10_DDI_CULL_NONE:
         case D3D10_DDI_CULL_FRONT:
         case D3D10_DDI_CULL_BACK:
         default:
      assert(0);
         }
      static uint
   translate_fill_mode(D3D10_DDI_FILL_MODE FillMode)
   {
      switch (FillMode) {
   case D3D10_DDI_FILL_WIREFRAME:
         case D3D10_DDI_FILL_SOLID:
         default:
      assert(0);
         }
         /*
   * ----------------------------------------------------------------------
   *
   * CreateRasterizerState --
   *
   *    The CreateRasterizerState function creates a rasterizer state.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   CreateRasterizerState(
      D3D10DDI_HDEVICE hDevice,                               // IN
   __in const D3D10_DDI_RASTERIZER_DESC *pRasterizerDesc,  // IN
   D3D10DDI_HRASTERIZERSTATE hRasterizerState,             // IN
      {
               struct pipe_context *pipe = CastPipeContext(hDevice);
            struct pipe_rasterizer_state state;
            state.flatshade_first = 1;
   state.front_ccw = (pRasterizerDesc->FrontCounterClockwise ? 1 : 0);
   state.cull_face = translate_cull_mode(pRasterizerDesc->CullMode);
   state.fill_front = translate_fill_mode(pRasterizerDesc->FillMode);
   state.fill_back = state.fill_front;
   state.scissor = (pRasterizerDesc->ScissorEnable ? 1 : 0);
   state.line_smooth = (pRasterizerDesc->AntialiasedLineEnable ? 1 : 0);
   state.offset_units = (float)pRasterizerDesc->DepthBias;
   state.offset_scale = pRasterizerDesc->SlopeScaledDepthBias;
   state.offset_clamp = pRasterizerDesc->DepthBiasClamp;
   state.multisample = /* pRasterizerDesc->MultisampleEnable */ 0;
   state.half_pixel_center = 1;
   state.bottom_edge_rule = 0;
   state.clip_halfz = 1;
   state.depth_clip_near = pRasterizerDesc->DepthClipEnable ? 1 : 0;
   state.depth_clip_far = pRasterizerDesc->DepthClipEnable ? 1 : 0;
            state.point_quad_rasterization = 1;
   state.point_size = 1.0f;
            state.line_width = 1.0f;
               }
         /*
   * ----------------------------------------------------------------------
   *
   * DestroyRasterizerState --
   *
   *    The DestroyRasterizerState function destroys the specified
   *    rasterizer state object. The rasterizer state object can be
   *    destoyed only if it is not currently bound to a display device.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DestroyRasterizerState(D3D10DDI_HDEVICE hDevice,                     // IN
         {
               struct pipe_context *pipe = CastPipeContext(hDevice);
               }
         /*
   * ----------------------------------------------------------------------
   *
   * SetRasterizerState --
   *
   *    The SetRasterizerState function sets the rasterizer state.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   SetRasterizerState(D3D10DDI_HDEVICE hDevice,                   // IN
         {
               struct pipe_context *pipe = CastPipeContext(hDevice);
               }
