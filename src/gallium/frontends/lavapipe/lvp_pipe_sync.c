   /*
   * Copyright © 2022 Collabora Ltd.
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "lvp_private.h"
   #include "util/timespec.h"
      static void
   lvp_pipe_sync_validate(ASSERTED struct lvp_pipe_sync *sync)
   {
      if (sync->signaled)
      }
      static VkResult
   lvp_pipe_sync_init(UNUSED struct vk_device *vk_device,
               {
               mtx_init(&sync->lock, mtx_plain);
   cnd_init(&sync->changed);
   sync->signaled = (initial_value != 0);
               }
      static void
   lvp_pipe_sync_finish(struct vk_device *vk_device,
         {
      struct lvp_device *device = container_of(vk_device, struct lvp_device, vk);
            lvp_pipe_sync_validate(sync);
   if (sync->fence)
         cnd_destroy(&sync->changed);
      }
      void
   lvp_pipe_sync_signal_with_fence(struct lvp_device *device,
               {
      mtx_lock(&sync->lock);
   lvp_pipe_sync_validate(sync);
   sync->signaled = fence == NULL;
   device->pscreen->fence_reference(device->pscreen, &sync->fence, fence);
   cnd_broadcast(&sync->changed);
      }
      static VkResult
   lvp_pipe_sync_signal(struct vk_device *vk_device,
               {
      struct lvp_device *device = container_of(vk_device, struct lvp_device, vk);
            mtx_lock(&sync->lock);
   lvp_pipe_sync_validate(sync);
   sync->signaled = true;
   if (sync->fence)
         cnd_broadcast(&sync->changed);
               }
      static VkResult
   lvp_pipe_sync_reset(struct vk_device *vk_device,
         {
      struct lvp_device *device = container_of(vk_device, struct lvp_device, vk);
            mtx_lock(&sync->lock);
   lvp_pipe_sync_validate(sync);
   sync->signaled = false;
   if (sync->fence)
         cnd_broadcast(&sync->changed);
               }
      static VkResult
   lvp_pipe_sync_move(struct vk_device *vk_device,
               {
      struct lvp_device *device = container_of(vk_device, struct lvp_device, vk);
   struct lvp_pipe_sync *dst = vk_sync_as_lvp_pipe_sync(vk_dst);
            /* Pull the fence out of the source */
   mtx_lock(&src->lock);
   struct pipe_fence_handle *fence = src->fence;
   bool signaled = src->signaled;
   src->fence = NULL;
   src->signaled = false;
   cnd_broadcast(&src->changed);
            mtx_lock(&dst->lock);
   if (dst->fence)
         dst->fence = fence;
   dst->signaled = signaled;
   cnd_broadcast(&dst->changed);
               }
      static VkResult
   lvp_pipe_sync_wait_locked(struct lvp_device *device,
                           {
                        uint64_t now_ns = os_time_get_nano();
   while (!sync->signaled && !sync->fence) {
      if (now_ns >= abs_timeout_ns)
            int ret;
   if (abs_timeout_ns >= INT64_MAX) {
      /* Common infinite wait case */
      } else {
      /* This is really annoying.  The C11 threads API uses CLOCK_REALTIME
   * while all our absolute timeouts are in CLOCK_MONOTONIC.  Best
   * thing we can do is to convert and hope the system admin doesn't
   * change the time out from under us.
                  struct timespec now_ts, abs_timeout_ts;
   timespec_get(&now_ts, TIME_UTC);
   if (timespec_add_nsec(&abs_timeout_ts, &now_ts, rel_timeout_ns)) {
      /* Overflowed; may as well be infinite */
      } else {
            }
   if (ret == thrd_error)
                     /* We don't trust the timeout condition on cnd_timedwait() because of
   * the potential clock issues caused by using CLOCK_REALTIME.  Instead,
   * update now_ns, go back to the top of the loop, and re-check.
   */
               if (sync->signaled || (wait_flags & VK_SYNC_WAIT_PENDING))
            /* Grab a reference before we drop the lock */
   struct pipe_fence_handle *fence = NULL;
                     uint64_t rel_timeout_ns =
         bool complete = device->pscreen->fence_finish(device->pscreen, NULL,
                                       if (!complete)
            if (sync->fence == fence) {
      device->pscreen->fence_reference(device->pscreen, &sync->fence, NULL);
                  }
      static VkResult
   lvp_pipe_sync_wait(struct vk_device *vk_device,
                     struct vk_sync *vk_sync,
   {
      struct lvp_device *device = container_of(vk_device, struct lvp_device, vk);
                     VkResult result = lvp_pipe_sync_wait_locked(device, sync, wait_value,
                        }
      const struct vk_sync_type lvp_pipe_sync_type = {
      .size = sizeof(struct lvp_pipe_sync),
   .features = VK_SYNC_FEATURE_BINARY |
               VK_SYNC_FEATURE_GPU_WAIT |
   VK_SYNC_FEATURE_GPU_MULTI_WAIT |
   VK_SYNC_FEATURE_CPU_WAIT |
   VK_SYNC_FEATURE_CPU_RESET |
   .init = lvp_pipe_sync_init,
   .finish = lvp_pipe_sync_finish,
   .signal = lvp_pipe_sync_signal,
   .reset = lvp_pipe_sync_reset,
   .move = lvp_pipe_sync_move,
      };
