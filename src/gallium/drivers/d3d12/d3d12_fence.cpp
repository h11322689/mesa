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
      #include "d3d12_fence.h"
      #include "d3d12_context.h"
   #include "d3d12_screen.h"
      #include "util/u_memory.h"
      #include <dxguids/dxguids.h>
      static void
   destroy_fence(struct d3d12_fence *fence)
   {
      d3d12_fence_close_event(fence->event, fence->event_fd);
      }
      struct d3d12_fence *
   d3d12_create_fence(struct d3d12_screen *screen)
   {
      struct d3d12_fence *ret = CALLOC_STRUCT(d3d12_fence);
   if (!ret) {
      debug_printf("CALLOC_STRUCT failed\n");
               ret->cmdqueue_fence = screen->fence;
   ret->value = ++screen->fence_value;
   ret->event = d3d12_fence_create_event(&ret->event_fd);
   if (FAILED(screen->cmdqueue->Signal(screen->fence, ret->value)))
         if (FAILED(screen->fence->SetEventOnCompletion(ret->value, ret->event)))
            pipe_reference_init(&ret->reference, 1);
         fail:
      destroy_fence(ret);
      }
      struct d3d12_fence *
   d3d12_open_fence(struct d3d12_screen *screen, HANDLE handle, const void *name)
   {
      struct d3d12_fence *ret = CALLOC_STRUCT(d3d12_fence);
   if (!ret) {
      debug_printf("CALLOC_STRUCT failed\n");
               HANDLE handle_to_close = nullptr;
   assert(!!handle ^ !!name);
   if (name) {
      screen->dev->OpenSharedHandleByName((LPCWSTR)name, GENERIC_ALL, &handle_to_close);
               screen->dev->OpenSharedHandle(handle, IID_PPV_ARGS(&ret->cmdqueue_fence));
   if (!ret->cmdqueue_fence) {
      free(ret);
               /* A new value will be assigned later */
   ret->value = 0;
   pipe_reference_init(&ret->reference, 1);
      }
      void
   d3d12_fence_reference(struct d3d12_fence **ptr, struct d3d12_fence *fence)
   {
      if (pipe_reference(&(*ptr)->reference, &fence->reference))
               }
      static void
   fence_reference(struct pipe_screen *pscreen,
               {
         }
      bool
   d3d12_fence_finish(struct d3d12_fence *fence, uint64_t timeout_ns)
   {
      if (fence->signaled)
            bool complete = fence->cmdqueue_fence->GetCompletedValue() >= fence->value;
   if (!complete && timeout_ns)
            fence->signaled = complete;
      }
      static bool
   fence_finish(struct pipe_screen *pscreen, struct pipe_context *pctx,
         {
      bool ret = d3d12_fence_finish(d3d12_fence(pfence), timeout_ns);
   if (ret && pctx) {
      pctx = threaded_context_unwrap_sync(pctx);
   struct d3d12_context *ctx = d3d12_context(pctx);
   d3d12_foreach_submitted_batch(ctx, batch)
      }
      }
      void
   d3d12_screen_fence_init(struct pipe_screen *pscreen)
   {
      pscreen->fence_reference = fence_reference;
      }
