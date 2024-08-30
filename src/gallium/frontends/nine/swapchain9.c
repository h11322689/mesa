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
      #include "swapchain9.h"
   #include "surface9.h"
   #include "device9.h"
      #include "nine_helpers.h"
   #include "nine_pipe.h"
   #include "nine_dump.h"
      #include "util/u_atomic.h"
   #include "util/u_inlines.h"
   #include "util/u_surface.h"
   #include "hud/hud_context.h"
   #include "frontend/drm_driver.h"
      #include "threadpool.h"
      #define DBG_CHANNEL DBG_SWAPCHAIN
      #define UNTESTED(n) DBG("UNTESTED point %d. Please tell if it worked\n", n)
      HRESULT
   NineSwapChain9_ctor( struct NineSwapChain9 *This,
                        struct NineUnknownParams *pParams,
   BOOL implicit,
   ID3DPresent *pPresent,
      {
      HRESULT hr;
            DBG("This=%p pDevice=%p pPresent=%p pCTX=%p hFocusWindow=%p\n",
            hr = NineUnknown_ctor(&This->base, pParams);
   if (FAILED(hr))
            This->screen = NineDevice9_GetScreen(This->base.device);
   This->implicit = implicit;
   This->actx = pCTX;
   This->present = pPresent;
            ID3DPresent_AddRef(pPresent);
   if (This->base.device->minor_version_num > 2) {
               memset(&params2, 0, sizeof(D3DPRESENT_PARAMETERS2));
   params2.AllowDISCARDDelayedRelease = This->actx->discard_delayed_release;
   params2.TearFreeDISCARD = This->actx->tearfree_discard;
               if (!pPresentationParameters->hDeviceWindow)
            This->rendering_done = false;
   This->pool = NULL;
   for (i = 0; i < D3DPRESENT_BACK_BUFFERS_MAX_EX + 1; i++) {
      This->pending_presentation[i] = calloc(1, sizeof(BOOL));
   if (!This->pending_presentation[i])
      }
      }
      static D3DWindowBuffer *
   D3DWindowBuffer_create(struct NineSwapChain9 *This,
                     {
      D3DWindowBuffer *ret;
   struct pipe_context *pipe = nine_context_get_pipe_acquire(This->base.device);
   struct winsys_handle whandle;
   int stride, dmaBufFd;
            memset(&whandle, 0, sizeof(whandle));
   whandle.type = WINSYS_HANDLE_TYPE_FD;
   if (!This->screen->resource_get_handle(This->screen, pipe, resource,
                              ERR("Failed to get handle for resource\n");
      }
   nine_context_get_pipe_release(This->base.device);
   stride = whandle.stride;
   dmaBufFd = whandle.handle;
   hr = ID3DPresent_NewD3DWindowBufferFromDmaBuf(This->present,
                                                      if (FAILED(hr)) {
      ERR("Failed to create new D3DWindowBufferFromDmaBuf\n");
      }
      }
      static void
   D3DWindowBuffer_release(struct NineSwapChain9 *This,
         {
               /* IsBufferReleased API not available */
   if (This->base.device->minor_version_num <= 2) {
      ID3DPresent_DestroyD3DWindowBuffer(This->present, present_handle);
               /* Add it to the 'pending release' list */
   for (i = 0; i < D3DPRESENT_BACK_BUFFERS_MAX_EX + 1; i++) {
      if (!This->present_handles_pending_release[i]) {
         This->present_handles_pending_release[i] = present_handle;
      }
   if (i == (D3DPRESENT_BACK_BUFFERS_MAX_EX + 1)) {
      ERR("Server not releasing buffers...\n");
               /* Destroy elements of the list released by the server */
   for (i = 0; i < D3DPRESENT_BACK_BUFFERS_MAX_EX + 1; i++) {
      if (This->present_handles_pending_release[i] &&
         ID3DPresent_IsBufferReleased(This->present, This->present_handles_pending_release[i])) {
   /* WaitBufferReleased also waits the presentation feedback
   * (which should arrive at about the same time),
   * while IsBufferReleased doesn't. DestroyD3DWindowBuffer unfortunately
   * checks it to release immediately all data, else the release
   * is postponed for This->present release. To avoid leaks (we may handle
   * a lot of resize), call WaitBufferReleased. */
   ID3DPresent_WaitBufferReleased(This->present, This->present_handles_pending_release[i]);
   ID3DPresent_DestroyD3DWindowBuffer(This->present, This->present_handles_pending_release[i]);
         }
      static int
   NineSwapChain9_GetBackBufferCountForParams( struct NineSwapChain9 *This,
            HRESULT
   NineSwapChain9_Resize( struct NineSwapChain9 *This,
               {
      struct NineDevice9 *pDevice = This->base.device;
   D3DSURFACE_DESC desc;
   HRESULT hr;
   struct pipe_resource *resource, tmplt;
   enum pipe_format pf;
   BOOL has_present_buffers = false;
   int depth;
   unsigned i, oldBufferCount, newBufferCount;
            DBG("This=%p pParams=%p\n", This, pParams);
   user_assert(pParams != NULL, E_POINTER);
   user_assert(pParams->SwapEffect, D3DERR_INVALIDCALL);
   user_assert((pParams->SwapEffect != D3DSWAPEFFECT_COPY) ||
         user_assert(pDevice->ex || pParams->BackBufferCount <=
         user_assert(!pDevice->ex || pParams->BackBufferCount <=
         user_assert(pDevice->ex ||
                        DBG("pParams(%p):\n"
      "BackBufferWidth: %u\n"
   "BackBufferHeight: %u\n"
   "BackBufferFormat: %s\n"
   "BackBufferCount: %u\n"
   "MultiSampleType: %u\n"
   "MultiSampleQuality: %u\n"
   "SwapEffect: %u\n"
   "hDeviceWindow: %p\n"
   "Windowed: %i\n"
   "EnableAutoDepthStencil: %i\n"
   "AutoDepthStencilFormat: %s\n"
   "Flags: %s\n"
   "FullScreen_RefreshRateInHz: %u\n"
   "PresentationInterval: %x\n", pParams,
   pParams->BackBufferWidth, pParams->BackBufferHeight,
   d3dformat_to_string(pParams->BackBufferFormat),
   pParams->BackBufferCount,
   pParams->MultiSampleType, pParams->MultiSampleQuality,
   pParams->SwapEffect, pParams->hDeviceWindow, pParams->Windowed,
   pParams->EnableAutoDepthStencil,
   d3dformat_to_string(pParams->AutoDepthStencilFormat),
   nine_D3DPRESENTFLAG_to_str(pParams->Flags),
   pParams->FullScreen_RefreshRateInHz,
         if (pParams->BackBufferCount == 0) {
                  if (pParams->BackBufferFormat == D3DFMT_UNKNOWN) {
                  This->desired_fences = This->actx->throttling ? This->actx->throttling_value + 1 : 0;
   /* +1 because we add the fence of the current buffer before popping an old one */
   if (This->desired_fences > DRI_SWAP_FENCES_MAX)
            if (This->actx->vblank_mode == 0)
         else if (This->actx->vblank_mode == 3)
            if (mode && This->mode) {
         } else if (mode) {
      This->mode = malloc(sizeof(D3DDISPLAYMODEEX));
      } else {
      free(This->mode);
               /* Note: It is the role of the backend to fill if necessary
   * BackBufferWidth and BackBufferHeight */
   hr = ID3DPresent_SetPresentParameters(This->present, pParams, This->mode);
   if (hr != D3D_OK)
            oldBufferCount = This->num_back_buffers;
                     /* Map MultiSampleQuality to MultiSampleType */
   hr = d3dmultisample_type_check(This->screen, pParams->BackBufferFormat,
                     if (FAILED(hr)) {
                  pf = d3d9_to_pipe_format_checked(This->screen, pParams->BackBufferFormat,
                  if (This->actx->linear_framebuffer ||
      (pf != PIPE_FORMAT_B8G8R8X8_UNORM &&
   pf != PIPE_FORMAT_B8G8R8A8_UNORM) ||
   pParams->SwapEffect != D3DSWAPEFFECT_DISCARD ||
   multisample_type >= 2 ||
   (This->actx->ref && This->actx->ref == This->screen))
         /* Note: the buffer depth has to match the window depth.
   * In practice, ARGB buffers can be used with windows
   * of depth 24. Windows of depth 32 are extremely rare.
   * So even if the buffer is ARGB, say it is depth 24.
   * It is common practice, for example that's how
   * glamor implements depth 24.
   * TODO: handle windows with other depths. Not possible in the short term.
   * For example 16 bits.*/
            memset(&tmplt, 0, sizeof(tmplt));
   tmplt.target = PIPE_TEXTURE_2D;
   tmplt.width0 = pParams->BackBufferWidth;
   tmplt.height0 = pParams->BackBufferHeight;
   tmplt.depth0 = 1;
   tmplt.last_level = 0;
   tmplt.array_size = 1;
   tmplt.usage = PIPE_USAGE_DEFAULT;
            desc.Type = D3DRTYPE_SURFACE;
   desc.Pool = D3DPOOL_DEFAULT;
   desc.MultiSampleType = pParams->MultiSampleType;
   desc.MultiSampleQuality = pParams->MultiSampleQuality;
   desc.Width = pParams->BackBufferWidth;
            for (i = 0; i < oldBufferCount; i++) {
      if (This->tasks[i])
      }
            if (This->pool) {
      _mesa_threadpool_destroy(This, This->pool);
      }
   This->enable_threadpool = This->actx->thread_submit && (pParams->SwapEffect != D3DSWAPEFFECT_COPY);
   if (This->enable_threadpool)
         if (!This->pool)
            for (i = 0; i < oldBufferCount; i++) {
      D3DWindowBuffer_release(This, This->present_handles[i]);
   This->present_handles[i] = NULL;
   if (This->present_buffers[i])
               if (newBufferCount != oldBufferCount) {
      for (i = newBufferCount; i < oldBufferCount;
                  for (i = oldBufferCount; i < newBufferCount; ++i) {
         This->buffers[i] = NULL;
      }
            for (i = 0; i < newBufferCount; ++i) {
      tmplt.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   tmplt.nr_samples = multisample_type;
   tmplt.nr_storage_samples = multisample_type;
   if (!has_present_buffers)
         tmplt.format = d3d9_to_pipe_format_checked(This->screen,
                           if (tmplt.format == PIPE_FORMAT_NONE)
         resource = nine_resource_create_with_retry(pDevice, This->screen, &tmplt);
   if (!resource) {
         DBG("Failed to create pipe_resource.\n");
   }
   if (pParams->Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER)
         if (This->buffers[i]) {
         NineSurface9_SetMultiSampleType(This->buffers[i], desc.MultiSampleType);
   NineSurface9_SetResourceResize(This->buffers[i], resource);
   if (has_present_buffers)
   } else {
         desc.Format = pParams->BackBufferFormat;
   desc.Usage = D3DUSAGE_RENDERTARGET;
   hr = NineSurface9_new(pDevice, NineUnknown(This), resource, NULL, 0,
         if (has_present_buffers)
         if (FAILED(hr)) {
      DBG("Failed to create RT surface.\n");
      }
   }
   if (has_present_buffers) {
         tmplt.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   tmplt.bind = NINE_BIND_PRESENTBUFFER_FLAGS;
   tmplt.nr_samples = 0;
   tmplt.nr_storage_samples = 0;
   if (This->actx->linear_framebuffer)
         if (pParams->SwapEffect != D3DSWAPEFFECT_DISCARD)
         resource = nine_resource_create_with_retry(pDevice, This->screen, &tmplt);
   }
   This->present_handles[i] = D3DWindowBuffer_create(This, resource, depth, false);
   pipe_resource_reference(&resource, NULL);
   if (!This->present_handles[i]) {
            }
   if (pParams->EnableAutoDepthStencil) {
      tmplt.bind = d3d9_get_pipe_depth_format_bindings(pParams->AutoDepthStencilFormat);
   tmplt.nr_samples = multisample_type;
   tmplt.nr_storage_samples = multisample_type;
   tmplt.format = d3d9_to_pipe_format_checked(This->screen,
                                    if (tmplt.format == PIPE_FORMAT_NONE)
            if (This->zsbuf) {
         resource = nine_resource_create_with_retry(pDevice, This->screen, &tmplt);
   if (!resource) {
                        NineSurface9_SetMultiSampleType(This->zsbuf, desc.MultiSampleType);
   NineSurface9_SetResourceResize(This->zsbuf, resource);
   } else {
         hr = NineDevice9_CreateDepthStencilSurface(pDevice,
                                             pParams->BackBufferWidth,
   if (FAILED(hr)) {
      DBG("Failed to create ZS surface.\n");
      }
                           }
      /* Throttling: code adapted from the dri frontend */
      /**
   * swap_fences_pop_front - pull a fence from the throttle queue
   *
   * If the throttle queue is filled to the desired number of fences,
   * pull fences off the queue until the number is less than the desired
   * number of fences, and return the last fence pulled.
   */
   static struct pipe_fence_handle *
   swap_fences_pop_front(struct NineSwapChain9 *This)
   {
      struct pipe_screen *screen = This->screen;
            if (This->desired_fences == 0)
            if (This->cur_fences >= This->desired_fences) {
      screen->fence_reference(screen, &fence, This->swap_fences[This->tail]);
   screen->fence_reference(screen, &This->swap_fences[This->tail++], NULL);
   This->tail &= DRI_SWAP_FENCES_MASK;
      }
      }
         /**
   * swap_fences_see_front - same than swap_fences_pop_front without
   * pulling
   *
   */
      static struct pipe_fence_handle *
   swap_fences_see_front(struct NineSwapChain9 *This)
   {
      struct pipe_screen *screen = This->screen;
            if (This->desired_fences == 0)
            if (This->cur_fences >= This->desired_fences) {
         }
      }
         /**
   * swap_fences_push_back - push a fence onto the throttle queue at the back
   *
   * push a fence onto the throttle queue and pull fences of the queue
   * so that the desired number of fences are on the queue.
   */
   static void
   swap_fences_push_back(struct NineSwapChain9 *This,
         {
               if (!fence || This->desired_fences == 0)
            while(This->cur_fences == This->desired_fences)
            This->cur_fences++;
   screen->fence_reference(screen, &This->swap_fences[This->head++],
            }
         /**
   * swap_fences_unref - empty the throttle queue
   *
   * pulls fences of the throttle queue until it is empty.
   */
   static void
   swap_fences_unref(struct NineSwapChain9 *This)
   {
               while(This->cur_fences) {
      screen->fence_reference(screen, &This->swap_fences[This->tail++], NULL);
   This->tail &= DRI_SWAP_FENCES_MASK;
         }
      void
   NineSwapChain9_dtor( struct NineSwapChain9 *This )
   {
                        if (This->pool)
            for (i = 0; i < D3DPRESENT_BACK_BUFFERS_MAX_EX + 1; i++) {
      if (This->pending_presentation[i])
               for (i = 0; i < D3DPRESENT_BACK_BUFFERS_MAX_EX + 1; i++) {
      if (This->present_handles_pending_release[i])
               for (i = 0; i < This->num_back_buffers; i++) {
      if (This->buffers[i])
         if (This->present_handles[i])
         if (This->present_buffers[i])
      }
   if (This->zsbuf)
            if (This->present)
            swap_fences_unref(This);
      }
      static void
   create_present_buffer( struct NineSwapChain9 *This,
                     {
               memset(&tmplt, 0, sizeof(tmplt));
   tmplt.target = PIPE_TEXTURE_2D;
   tmplt.width0 = width;
   tmplt.height0 = height;
   tmplt.depth0 = 1;
   tmplt.last_level = 0;
   tmplt.array_size = 1;
   tmplt.usage = PIPE_USAGE_DEFAULT;
   tmplt.flags = 0;
   tmplt.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   tmplt.bind = NINE_BIND_BACKBUFFER_FLAGS |
         tmplt.nr_samples = 0;
   if (This->actx->linear_framebuffer)
                           if (!*present_handle) {
            }
      static void
   handle_draw_cursor_and_hud( struct NineSwapChain9 *This, struct pipe_resource *resource)
   {
      struct NineDevice9 *device = This->base.device;
   struct pipe_blit_info blit;
            if (device->cursor.software && device->cursor.visible && device->cursor.w) {
      memset(&blit, 0, sizeof(blit));
   blit.src.resource = device->cursor.image;
   blit.src.level = 0;
   blit.src.format = device->cursor.image->format;
   blit.src.box.x = 0;
   blit.src.box.y = 0;
   blit.src.box.z = 0;
   blit.src.box.depth = 1;
   blit.src.box.width = device->cursor.w;
            blit.dst.resource = resource;
   blit.dst.level = 0;
   blit.dst.format = resource->format;
   blit.dst.box.z = 0;
            blit.mask = PIPE_MASK_RGBA;
   blit.filter = PIPE_TEX_FILTER_NEAREST;
            /* NOTE: blit messes up when box.x + box.width < 0, fix driver
      * NOTE2: device->cursor.pos contains coordinates relative to the screen.
   * This happens to be also the position of the cursor when we are fullscreen.
      blit.dst.box.x = MAX2(device->cursor.pos.x, 0) - device->cursor.hotspot.x;
   blit.dst.box.y = MAX2(device->cursor.pos.y, 0) - device->cursor.hotspot.y;
   blit.dst.box.width = blit.src.box.width;
            DBG("Blitting cursor(%ux%u) to (%i,%i).\n",
                  blit.alpha_blend = true;
   pipe = NineDevice9_GetPipe(This->base.device);
               if (device->hud && resource) {
      /* Implicit use of context pipe */
   (void)NineDevice9_GetPipe(This->base.device);
   hud_run(device->hud, NULL, resource); /* XXX: no offset */
   /* HUD doesn't clobber stipple */
         }
      struct end_present_struct {
      struct pipe_screen *screen;
   struct pipe_fence_handle *fence_to_wait;
   ID3DPresent *present;
   D3DWindowBuffer *present_handle;
   BOOL *pending_presentation;
      };
      static void work_present(void *data)
   {
      struct end_present_struct *work = data;
   if (work->fence_to_wait) {
      (void) work->screen->fence_finish(work->screen, NULL, work->fence_to_wait, OS_TIMEOUT_INFINITE);
      }
   ID3DPresent_PresentBuffer(work->present, work->present_handle, work->hDestWindowOverride, NULL, NULL, NULL, 0);
   p_atomic_set(work->pending_presentation, false);
      }
      static void pend_present(struct NineSwapChain9 *This,
               {
               work->screen = This->screen;
   This->screen->fence_reference(This->screen, &work->fence_to_wait, fence);
   work->present = This->present;
   work->present_handle = This->present_handles[0];
   work->hDestWindowOverride = hDestWindowOverride;
   work->pending_presentation = This->pending_presentation[0];
   p_atomic_set(work->pending_presentation, true);
               }
      static inline HRESULT
   present( struct NineSwapChain9 *This,
            const RECT *pSourceRect,
   const RECT *pDestRect,
   HWND hDestWindowOverride,
      {
      struct pipe_context *pipe;
   struct pipe_resource *resource;
   struct pipe_fence_handle *fence;
   HRESULT hr;
   struct pipe_blit_info blit;
   int target_width, target_height, target_depth, i;
   RECT source_rect;
            DBG("present: This=%p pSourceRect=%p pDestRect=%p "
      "pDirtyRegion=%p hDestWindowOverride=%p"
   "dwFlags=%d resource=%p\n",
   This, pSourceRect, pDestRect, pDirtyRegion,
         /* We can choose to only update pDirtyRegion, but the backend can choose
   * to update everything. Let's ignore */
                     if (pSourceRect) {
      DBG("pSourceRect = (%u..%u)x(%u..%u)\n",
         pSourceRect->left, pSourceRect->right,
   source_rect = *pSourceRect;
   if (source_rect.top == 0 &&
         source_rect.left == 0 &&
   source_rect.bottom == resource->height0 &&
   source_rect.right == resource->width0)
   /* TODO: Handle more of pSourceRect.
      * Currently we should support:
   * . When there is no pSourceRect
   * . When pSourceRect is the full buffer.
   }
   if (pDestRect) {
      DBG("pDestRect = (%u..%u)x(%u..%u)\n",
         pDestRect->left, pDestRect->right,
               if (This->rendering_done)
            if (This->params.SwapEffect == D3DSWAPEFFECT_DISCARD)
            hr = ID3DPresent_GetWindowInfo(This->present, hDestWindowOverride, &target_width, &target_height, &target_depth);
            /* Can happen with old Wine (presentation can still succeed),
   * or at window destruction.
   * Also disable for very old wine as D3DWindowBuffer_release
   * cannot do the DestroyD3DWindowBuffer workaround. */
   if (FAILED(hr) || target_width == 0 || target_height == 0 ||
      This->base.device->minor_version_num <= 2) {
   target_width = resource->width0;
               if (pDestRect) {
      dest_rect.top = MAX2(0, dest_rect.top);
   dest_rect.left = MAX2(0, dest_rect.left);
   dest_rect.bottom = MIN2(target_height, dest_rect.bottom);
   dest_rect.right = MIN2(target_width, dest_rect.right);
   target_height = dest_rect.bottom - dest_rect.top;
               /* Switch to using presentation buffers on window resize.
   * Note: Most apps should resize the d3d back buffers when
   * a window resize is detected, which will result in a call to
   * NineSwapChain9_Resize. Thus everything will get released,
   * and it will switch back to not using separate presentation
   * buffers. */
   if (!This->present_buffers[0] &&
      (target_width != resource->width0 || target_height != resource->height0)) {
   BOOL failure = false;
   struct pipe_resource *new_resource[This->num_back_buffers];
   D3DWindowBuffer *new_handles[This->num_back_buffers];
   for (i = 0; i < This->num_back_buffers; i++) {
         /* Note: if (!new_handles[i]), new_resource[i]
   * gets released and contains NULL */
   create_present_buffer(This, target_width, target_height, &new_resource[i], &new_handles[i]);
   if (!new_handles[i])
   }
   if (failure) {
         for (i = 0; i < This->num_back_buffers; i++) {
      if (new_resource[i])
         if (new_handles[i])
      } else {
         for (i = 0; i < This->num_back_buffers; i++) {
      D3DWindowBuffer_release(This, This->present_handles[i]);
   This->present_handles[i] = new_handles[i];
   pipe_resource_reference(&This->present_buffers[i], new_resource[i]);
                           if (This->present_buffers[0]) {
      memset(&blit, 0, sizeof(blit));
   blit.src.resource = resource;
   blit.src.level = 0; /* Note: This->buffers[0]->level should always be 0 */
   blit.src.format = resource->format;
   blit.src.box.z = 0;
   blit.src.box.depth = 1;
   blit.src.box.x = 0;
   blit.src.box.y = 0;
   blit.src.box.width = resource->width0;
            /* Reallocate a new presentation buffer if the target window
         if (target_width != This->present_buffers[0]->width0 ||
         target_height != This->present_buffers[0]->height0) {
                  create_present_buffer(This, target_width, target_height, &new_resource, &new_handle);
   /* Switch to the new buffer */
   if (new_handle) {
      D3DWindowBuffer_release(This, This->present_handles[0]);
   This->present_handles[0] = new_handle;
   pipe_resource_reference(&This->present_buffers[0], new_resource);
                        blit.dst.resource = resource;
   blit.dst.level = 0;
   blit.dst.format = resource->format;
   blit.dst.box.z = 0;
   blit.dst.box.depth = 1;
   blit.dst.box.x = 0;
   blit.dst.box.y = 0;
   blit.dst.box.width = resource->width0;
            blit.mask = PIPE_MASK_RGBA;
   blit.filter = (blit.dst.box.width == blit.src.box.width &&
               blit.scissor_enable = false;
                        /* The resource we present has to resolve fast clears
   * if needed (and other things) */
            if (This->params.SwapEffect != D3DSWAPEFFECT_DISCARD)
            fence = NULL;
   /* When threadpool is enabled, we don't submit before the fence
   * tells us rendering was finished, thus we can flush async there */
            /* Present now for thread_submit, because we have the fence.
   * It's possible we return WASSTILLDRAWING and still Present,
   * but it should be fine. */
   if (This->enable_threadpool)
         if (fence) {
      swap_fences_push_back(This, fence);
                  bypass_rendering:
         if (dwFlags & D3DPRESENT_DONOTWAIT) {
      UNTESTED(2);
   BOOL still_draw = false;
   fence = swap_fences_see_front(This);
   if (fence) {
         still_draw = !This->screen->fence_finish(This->screen, NULL, fence, 0);
   }
   if (still_draw)
               /* Throttle rendering if needed */
   fence = swap_fences_pop_front(This);
   if (fence) {
      (void) This->screen->fence_finish(This->screen, NULL, fence, OS_TIMEOUT_INFINITE);
                        if (!This->enable_threadpool) {
                                    This->base.device->end_scene_since_present = 0;
   This->base.device->frame_count++;
      }
      HRESULT NINE_WINAPI
   NineSwapChain9_Present( struct NineSwapChain9 *This,
                           const RECT *pSourceRect,
   {
      struct pipe_resource *res = NULL;
   D3DWindowBuffer *handle_temp;
   struct threadpool_task *task_temp;
   BOOL *pending_presentation_temp;
   int i;
            DBG("This=%p pSourceRect=%p pDestRect=%p hDestWindowOverride=%p "
      "pDirtyRegion=%p dwFlags=%d\n",
   This, pSourceRect, pDestRect, hDestWindowOverride,
         if (This->base.device->ex) {
      if (NineSwapChain9_GetOccluded(This)) {
         DBG("Present is occluded. Returning S_PRESENT_OCCLUDED.\n");
      } else {
      if (NineSwapChain9_GetOccluded(This) ||
         NineSwapChain9_ResolutionMismatch(This)) {
   }
   if (This->base.device->device_needs_reset) {
         DBG("Device is lost. Returning D3DERR_DEVICELOST.\n");
                        hr = present(This, pSourceRect, pDestRect,
         if (hr == D3DERR_WASSTILLDRAWING)
            if (This->base.device->minor_version_num > 2 &&
      This->actx->discard_delayed_release &&
   This->params.SwapEffect == D3DSWAPEFFECT_DISCARD &&
   This->params.PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE) {
            while (next_buffer == -1) {
         /* Find a free backbuffer */
   for (i = 1; i < This->num_back_buffers; i++) {
      if (!p_atomic_read(This->pending_presentation[i]) &&
      ID3DPresent_IsBufferReleased(This->present, This->present_handles[i])) {
   DBG("Found buffer released: %d\n", i);
   next_buffer = i;
         }
   if (next_buffer == -1) {
      DBG("Found no buffer released. Waiting for event\n");
               /* Free the task (we already checked it is finished) */
   if (This->tasks[next_buffer])
         assert(!*This->pending_presentation[next_buffer] && !This->tasks[next_buffer]);
   This->tasks[next_buffer] = This->tasks[0];
   This->tasks[0] = NULL;
   pending_presentation_temp = This->pending_presentation[next_buffer];
   This->pending_presentation[next_buffer] = This->pending_presentation[0];
            /* Switch with the released buffer */
   pipe_resource_reference(&res, This->buffers[0]->base.resource);
   NineSurface9_SetResourceResize(
         NineSurface9_SetResourceResize(
                  if (This->present_buffers[0]) {
         pipe_resource_reference(&res, This->present_buffers[0]);
   pipe_resource_reference(&This->present_buffers[0], This->present_buffers[next_buffer]);
   pipe_resource_reference(&This->present_buffers[next_buffer], res);
            handle_temp = This->present_handles[0];
   This->present_handles[0] = This->present_handles[next_buffer];
      } else {
      switch (This->params.SwapEffect) {
         case D3DSWAPEFFECT_OVERLAY: /* Not implemented, fallback to FLIP */
   case D3DSWAPEFFECT_FLIPEX: /* Allows optimizations over FLIP for windowed mode. */
   case D3DSWAPEFFECT_DISCARD: /* Allows optimizations over FLIP */
   case D3DSWAPEFFECT_FLIP:
      /* rotate the queue */
   pipe_resource_reference(&res, This->buffers[0]->base.resource);
   for (i = 1; i < This->num_back_buffers; i++) {
      NineSurface9_SetResourceResize(This->buffers[i - 1],
      }
                        if (This->present_buffers[0]) {
      pipe_resource_reference(&res, This->present_buffers[0]);
   for (i = 1; i < This->num_back_buffers; i++)
                           handle_temp = This->present_handles[0];
   for (i = 1; i < This->num_back_buffers; i++) {
         }
   This->present_handles[This->num_back_buffers - 1] = handle_temp;
   task_temp = This->tasks[0];
   for (i = 1; i < This->num_back_buffers; i++) {
         }
   This->tasks[This->num_back_buffers - 1] = task_temp;
   pending_presentation_temp = This->pending_presentation[0];
   for (i = 1; i < This->num_back_buffers; i++) {
                           case D3DSWAPEFFECT_COPY:
                  if (This->tasks[0])
                                          }
      HRESULT NINE_WINAPI
   NineSwapChain9_GetFrontBufferData( struct NineSwapChain9 *This,
         {
      struct NineSurface9 *dest_surface = NineSurface9(pDestSurface);
   struct NineDevice9 *pDevice = This->base.device;
   unsigned int width, height;
   struct pipe_resource *temp_resource;
   struct NineSurface9 *temp_surface;
   D3DWindowBuffer *temp_handle;
   D3DSURFACE_DESC desc;
            DBG("GetFrontBufferData: This=%p pDestSurface=%p\n",
                     width = dest_surface->desc.Width;
            /* Note: front window size and destination size are supposed
   * to match. However it's not very clear what should get taken in Windowed
   * mode. It may need a fix */
            if (!temp_resource || !temp_handle) {
                  desc.Type = D3DRTYPE_SURFACE;
   desc.Pool = D3DPOOL_DEFAULT;
   desc.MultiSampleType = D3DMULTISAMPLE_NONE;
   desc.MultiSampleQuality = 0;
   desc.Width = width;
   desc.Height = height;
   /* NineSurface9_CopyDefaultToMem needs same format. */
   desc.Format = dest_surface->desc.Format;
   desc.Usage = D3DUSAGE_RENDERTARGET;
   hr = NineSurface9_new(pDevice, NineUnknown(This), temp_resource, NULL, 0,
         pipe_resource_reference(&temp_resource, NULL);
   if (FAILED(hr)) {
      DBG("Failed to create temp FrontBuffer surface.\n");
                                 ID3DPresent_DestroyD3DWindowBuffer(This->present, temp_handle);
               }
      HRESULT NINE_WINAPI
   NineSwapChain9_GetBackBuffer( struct NineSwapChain9 *This,
                     {
      DBG("GetBackBuffer: This=%p iBackBuffer=%d Type=%d ppBackBuffer=%p\n",
         (void)user_error(Type == D3DBACKBUFFER_TYPE_MONO);
   /* don't touch ppBackBuffer on error */
   user_assert(ppBackBuffer != NULL, D3DERR_INVALIDCALL);
            NineUnknown_AddRef(NineUnknown(This->buffers[iBackBuffer]));
   *ppBackBuffer = (IDirect3DSurface9 *)This->buffers[iBackBuffer];
      }
      HRESULT NINE_WINAPI
   NineSwapChain9_GetRasterStatus( struct NineSwapChain9 *This,
         {
      DBG("GetRasterStatus: This=%p pRasterStatus=%p\n",
         user_assert(pRasterStatus != NULL, E_POINTER);
      }
      HRESULT NINE_WINAPI
   NineSwapChain9_GetDisplayMode( struct NineSwapChain9 *This,
         {
      D3DDISPLAYMODEEX mode;
   D3DDISPLAYROTATION rot;
            DBG("GetDisplayMode: This=%p pMode=%p\n",
                  hr = ID3DPresent_GetDisplayMode(This->present, &mode, &rot);
   if (SUCCEEDED(hr)) {
      pMode->Width = mode.Width;
   pMode->Height = mode.Height;
   pMode->RefreshRate = mode.RefreshRate;
      }
      }
      HRESULT NINE_WINAPI
   NineSwapChain9_GetPresentParameters( struct NineSwapChain9 *This,
         {
      DBG("GetPresentParameters: This=%p pPresentationParameters=%p\n",
         user_assert(pPresentationParameters != NULL, E_POINTER);
   *pPresentationParameters = This->params;
      }
      IDirect3DSwapChain9Vtbl NineSwapChain9_vtable = {
      (void *)NineUnknown_QueryInterface,
   (void *)NineUnknown_AddRef,
   (void *)NineUnknown_Release,
   (void *)NineSwapChain9_Present,
   (void *)NineSwapChain9_GetFrontBufferData,
   (void *)NineSwapChain9_GetBackBuffer,
   (void *)NineSwapChain9_GetRasterStatus,
   (void *)NineSwapChain9_GetDisplayMode,
   (void *)NineUnknown_GetDevice, /* actually part of SwapChain9 iface */
      };
      static const GUID *NineSwapChain9_IIDs[] = {
      &IID_IDirect3DSwapChain9,
   &IID_IUnknown,
      };
      HRESULT
   NineSwapChain9_new( struct NineDevice9 *pDevice,
                     BOOL implicit,
   ID3DPresent *pPresent,
   D3DPRESENT_PARAMETERS *pPresentationParameters,
   {
      NINE_DEVICE_CHILD_NEW(SwapChain9, ppOut, pDevice, /* args */
            }
      BOOL
   NineSwapChain9_GetOccluded( struct NineSwapChain9 *This )
   {
      if (This->base.device->minor_version_num > 0) {
                     }
      BOOL
   NineSwapChain9_ResolutionMismatch( struct NineSwapChain9 *This )
   {
      if (This->base.device->minor_version_num > 1) {
                     }
      HANDLE
   NineSwapChain9_CreateThread( struct NineSwapChain9 *This,
               {
      if (This->base.device->minor_version_num > 1) {
                     }
      void
   NineSwapChain9_WaitForThread( struct NineSwapChain9 *This,
         {
      if (This->base.device->minor_version_num > 1) {
            }
      static int
   NineSwapChain9_GetBackBufferCountForParams( struct NineSwapChain9 *This,
         {
               /* When we have flip behaviour, d3d9 expects we get back the screen buffer when we flip.
   * Here we don't get back the initial content of the screen. To emulate the behaviour
   * we allocate an additional buffer */
   if (pParams->SwapEffect != D3DSWAPEFFECT_COPY)
         /* With DISCARD, as there is no guarantee about the buffer contents, we can use
   * an arbitrary number of buffers */
   if (pParams->SwapEffect == D3DSWAPEFFECT_DISCARD) {
      /* thread_submit's can have maximum count or This->actx->throttling_value + 1
      * frames in flight being rendered and not shown.
      if (This->actx->thread_submit && count < This->desired_fences)
         /* When we enable AllowDISCARDDelayedRelease, we must ensure
      * to have at least 4 buffers to meet INTERVAL_IMMEDIATE,
   * since the display server/compositor can hold 3 buffers
   * without releasing them:
   * . Buffer on screen.
   * . Buffer scheduled kernel side to be next on screen.
      if (This->base.device->minor_version_num > 2 &&
         This->actx->discard_delayed_release &&
   pParams->PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE) {
   if (This->actx->thread_submit && count < 4)
         /* When thread_submit is not used, 5 buffers are actually needed,
   * because in case a pageflip is missed because rendering wasn't finished,
   * the Xserver will hold 4 buffers. */
   else if (!This->actx->thread_submit && count < 5)
         /* Somehow this cases needs 5 with thread_submit, or else you get a small performance hit */
   if (This->actx->tearfree_discard && count < 5)
                  }
