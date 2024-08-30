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
   #include <wrl.h>
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
      constexpr uint32_t num_buffers = 2;
   static int current_backbuffer_index = 0;
   static bool has_signaled_first_time = false;
   static int cached_interval = 1;
      struct d3d12_wgl_framebuffer {
               struct d3d12_screen *screen;
   enum pipe_format pformat;
   ID3D12Resource *images[num_buffers];
   D3D12_CPU_DESCRIPTOR_HANDLE rtvs[num_buffers];
   ID3D12DescriptorHeap *rtvHeap;
      };
      static struct d3d12_wgl_framebuffer*
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
                  framebuffer->rtvHeap->Release();
   for (int i = 0; i < num_buffers; ++i) {
      if (framebuffer->buffers[i]) {
      d3d12_resource_release(d3d12_resource(framebuffer->buffers[i]));
                     }
      static void
   d3d12_wgl_framebuffer_resize(stw_winsys_framebuffer *fb,
               {
               if (framebuffer->rtvHeap == NULL) {
      D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
   descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
   descHeapDesc.NumDescriptors = num_buffers;
   framebuffer->screen->dev->CreateDescriptorHeap(&descHeapDesc,
               // Release the old images
   for (int i = 0; i < num_buffers; i++) {
      if (framebuffer->buffers[i]) {
      d3d12_resource_release(d3d12_resource(framebuffer->buffers[i]));
                  D3D12_HEAP_PROPERTIES heapProps = {};
            D3D12_RESOURCE_DESC resDesc;
   resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
   resDesc.Width = templ->width0;
   resDesc.Height = templ->height0;
   resDesc.Alignment = 0;
   resDesc.DepthOrArraySize = 1;
   resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
   resDesc.Format = d3d12_get_format(templ->format);
   resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
   resDesc.MipLevels = 1;
   resDesc.SampleDesc.Count = 1;
            D3D12_CLEAR_VALUE optimizedClearValue = {};
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
   rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
   rtvDesc.Format = resDesc.Format;
   rtvDesc.Texture2D.MipSlice = 0;
            for (int i = 0; i < num_buffers; i++) {
      if (FAILED(framebuffer->screen->dev->CreateCommittedResource(
      &heapProps,
   D3D12_HEAP_FLAG_ALLOW_DISPLAY,
   &resDesc,
   D3D12_RESOURCE_STATE_PRESENT,
   &optimizedClearValue,
      ))) {
                  framebuffer->rtvs[i].ptr =
                  framebuffer->screen->dev->CreateRenderTargetView(
      framebuffer->images[i],
   &rtvDesc,
                     }
      static bool
   d3d12_wgl_framebuffer_present(stw_winsys_framebuffer *fb, int interval)
   {
      auto framebuffer = d3d12_wgl_framebuffer(fb);
   D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParams = {};
   planeParams.Token = framebuffer->screen->frame_token;
   planeParams.ResourceCount = 1;
            D3D12XBOX_PRESENT_PARAMETERS presentParams = {};
   presentParams.Flags = (interval == 0) ?
      D3D12XBOX_PRESENT_FLAG_IMMEDIATE :
         int clamped_interval = CLAMP(interval, 1, 4); // SetFrameIntervalX only supports values [1,4]
   if (cached_interval != clamped_interval) {
      framebuffer->screen->dev->SetFrameIntervalX(
      nullptr,
   D3D12XBOX_FRAME_INTERVAL_60_HZ,
   clamped_interval,
      );
   framebuffer->screen->dev->ScheduleFrameEventX(
      D3D12XBOX_FRAME_EVENT_ORIGIN,
   0,
   nullptr,
      );
                                 framebuffer->screen->frame_token = D3D12XBOX_FRAME_PIPELINE_TOKEN_NULL;
   framebuffer->screen->dev->WaitFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, INFINITE,
                     }
      static struct pipe_resource*
   d3d12_wgl_framebuffer_get_resource(struct stw_winsys_framebuffer *pframebuffer,
         {
      auto framebuffer = d3d12_wgl_framebuffer(pframebuffer);
            UINT index = current_backbuffer_index;
   if (statt == ST_ATTACHMENT_FRONT_LEFT)
            if (framebuffer->buffers[index]) {
      pipe_reference(NULL, &framebuffer->buffers[index]->reference);
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
      pscreen->resource_from_handle(pscreen, &templ, &handle,
         }
      struct stw_winsys_framebuffer*
   d3d12_wgl_create_framebuffer(struct pipe_screen *screen,
               {
      const struct stw_pixelformat_info *pfi =
         if (!(pfi->pfd.dwFlags & PFD_DOUBLEBUFFER) ||
      (pfi->pfd.dwFlags & PFD_SUPPORT_GDI))
         struct d3d12_wgl_framebuffer *fb = CALLOC_STRUCT(d3d12_wgl_framebuffer);
   if (!fb)
                     fb->screen = d3d12_screen(screen);
   fb->images[0] = NULL;
   fb->images[1] = NULL;
   fb->rtvHeap = NULL;
   fb->base.destroy = d3d12_wgl_framebuffer_destroy;
   fb->base.resize = d3d12_wgl_framebuffer_resize;
   fb->base.present = d3d12_wgl_framebuffer_present;
            // Xbox applications must manually handle Suspend/Resume events on the Command Queue.
   // To allow the application to access the queue, we store a pointer in the HWND's user data.
            // Schedule the frame interval and origin frame event
   fb->screen->dev->SetFrameIntervalX(
      nullptr,
   D3D12XBOX_FRAME_INTERVAL_60_HZ,
   cached_interval,
      );
   fb->screen->dev->ScheduleFrameEventX(
      D3D12XBOX_FRAME_EVENT_ORIGIN,
   0,
   nullptr,
                  }
