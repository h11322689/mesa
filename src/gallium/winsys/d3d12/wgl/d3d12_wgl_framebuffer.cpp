   /*
   * Copyright © Microsoft Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "d3d12_wgl_public.h"
      #include <new>
      #include <windows.h>
   #include <dxgi1_4.h>
   #include <directx/d3d12.h>
   #include <wrl.h>
   #include <dxguids/dxguids.h>
      #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "frontend/api.h"
   #include "frontend/winsys_handle.h"
      #include "stw_device.h"
   #include "stw_pixelformat.h"
   #include "stw_winsys.h"
      #include "d3d12/d3d12_format.h"
   #include "d3d12/d3d12_resource.h"
   #include "d3d12/d3d12_screen.h"
      using Microsoft::WRL::ComPtr;
   constexpr uint32_t num_buffers = 2;
      struct d3d12_wgl_framebuffer {
               struct d3d12_screen *screen;
   enum pipe_format pformat;
   HWND window;
   ComPtr<IDXGISwapChain3> swapchain;
      };
      static struct d3d12_wgl_framebuffer *
   d3d12_wgl_framebuffer(struct stw_winsys_framebuffer *fb)
   {
         }
      static void
   d3d12_wgl_framebuffer_destroy(struct stw_winsys_framebuffer *fb,
         {
      struct d3d12_wgl_framebuffer *framebuffer = d3d12_wgl_framebuffer(fb);
            if (ctx) {
      /* Ensure all resources are flushed */
   ctx->flush(ctx, &fence, PIPE_FLUSH_HINT_FINISH);
   if (fence) {
      ctx->screen->fence_finish(ctx->screen, ctx, fence, OS_TIMEOUT_INFINITE);
                  for (int i = 0; i < num_buffers; ++i) {
      if (framebuffer->buffers[i]) {
      d3d12_resource_release(d3d12_resource(framebuffer->buffers[i]));
                     }
      static void
   d3d12_wgl_framebuffer_resize(stw_winsys_framebuffer *fb,
               {
      struct d3d12_wgl_framebuffer *framebuffer = d3d12_wgl_framebuffer(fb);
            DXGI_SWAP_CHAIN_DESC1 desc = {};
   desc.BufferCount = num_buffers;
   desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
   desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
   desc.Format = d3d12_get_format(templ->format);
   desc.Width = templ->width0;
   desc.Height = templ->height0;
   desc.SampleDesc.Count = 1;
                     if (!framebuffer->swapchain) {
      ComPtr<IDXGISwapChain1> swapchain1;
   if (FAILED(screen->factory->CreateSwapChainForHwnd(
      screen->base.cmdqueue,
   framebuffer->window,
   &desc,
   nullptr,
   nullptr,
   &swapchain1))) {
   debug_printf("D3D12: failed to create swapchain");
                        screen->factory->MakeWindowAssociation(framebuffer->window,
      }
   else {
               /* Ensure all resources are flushed */
   ctx->flush(ctx, &fence, PIPE_FLUSH_HINT_FINISH);
   if (fence) {
      ctx->screen->fence_finish(ctx->screen, ctx, fence, OS_TIMEOUT_INFINITE);
               for (int i = 0; i < num_buffers; ++i) {
      if (framebuffer->buffers[i]) {
      d3d12_resource_release(d3d12_resource(framebuffer->buffers[i]));
         }
   if (FAILED(framebuffer->swapchain->ResizeBuffers(num_buffers, desc.Width, desc.Height, desc.Format, desc.Flags))) {
               }
      static bool
   d3d12_wgl_framebuffer_present(stw_winsys_framebuffer *fb, int interval)
   {
      auto framebuffer = d3d12_wgl_framebuffer(fb);
   if (!framebuffer->swapchain) {
      debug_printf("D3D12: Cannot present; no swapchain");
               if (interval < 1)
         else
      }
      static struct pipe_resource *
   d3d12_wgl_framebuffer_get_resource(struct stw_winsys_framebuffer *pframebuffer,
         {
      auto framebuffer = d3d12_wgl_framebuffer(pframebuffer);
            if (!framebuffer->swapchain)
            UINT index = framebuffer->swapchain->GetCurrentBackBufferIndex();
   if (statt == ST_ATTACHMENT_FRONT_LEFT)
            if (framebuffer->buffers[index]) {
      pipe_reference(NULL, &framebuffer->buffers[index]->reference);
               ID3D12Resource *res;
   framebuffer->swapchain->GetBuffer(index, IID_PPV_ARGS(&res));
   if (!res)
            struct winsys_handle handle;
   memset(&handle, 0, sizeof(handle));
   handle.type = WINSYS_HANDLE_TYPE_D3D12_RES;
   handle.format = framebuffer->pformat;
                     struct pipe_resource templ;
   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = framebuffer->pformat;
   templ.width0 = res_desc.Width;
   templ.height0 = res_desc.Height;
   templ.depth0 = 1;
   templ.array_size = res_desc.DepthOrArraySize;
   templ.nr_samples = res_desc.SampleDesc.Count;
   templ.last_level = res_desc.MipLevels - 1;
   templ.bind = PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_RENDER_TARGET;
   templ.usage = PIPE_USAGE_DEFAULT;
            pipe_resource_reference(&framebuffer->buffers[index],
                  }
      struct stw_winsys_framebuffer *
   d3d12_wgl_create_framebuffer(struct pipe_screen *screen,
               {
      const struct stw_pixelformat_info *pfi =
         if (!(pfi->pfd.dwFlags & PFD_DOUBLEBUFFER) ||
      (pfi->pfd.dwFlags & PFD_SUPPORT_GDI))
         if (pfi->stvis.color_format != PIPE_FORMAT_B8G8R8A8_UNORM &&
      pfi->stvis.color_format != PIPE_FORMAT_R8G8B8A8_UNORM &&
   pfi->stvis.color_format != PIPE_FORMAT_R10G10B10A2_UNORM &&
   pfi->stvis.color_format != PIPE_FORMAT_R16G16B16A16_FLOAT)
         struct d3d12_wgl_framebuffer *fb = CALLOC_STRUCT(d3d12_wgl_framebuffer);
   if (!fb)
                     fb->window = hWnd;
   fb->screen = d3d12_screen(screen);
   fb->base.destroy = d3d12_wgl_framebuffer_destroy;
   fb->base.resize = d3d12_wgl_framebuffer_resize;
   fb->base.present = d3d12_wgl_framebuffer_present;
               }
