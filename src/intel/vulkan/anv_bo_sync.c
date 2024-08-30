   /*
   * Copyright © 2021 Intel Corporation
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
      #include "anv_private.h"
      #include "util/os_time.h"
   #include "util/perf/cpu_trace.h"
      static struct anv_bo_sync *
   to_anv_bo_sync(struct vk_sync *sync)
   {
      assert(sync->type == &anv_bo_sync_type);
      }
      static VkResult
   anv_bo_sync_init(struct vk_device *vk_device,
               {
      struct anv_device *device = container_of(vk_device, struct anv_device, vk);
            sync->state = initial_value ? ANV_BO_SYNC_STATE_SIGNALED :
            return anv_device_alloc_bo(device, "bo-sync", 4096,
                        }
      static void
   anv_bo_sync_finish(struct vk_device *vk_device,
         {
      struct anv_device *device = container_of(vk_device, struct anv_device, vk);
               }
      static VkResult
   anv_bo_sync_reset(struct vk_device *vk_device,
         {
                           }
      static int64_t
   anv_get_relative_timeout(uint64_t abs_timeout)
   {
               /* We don't want negative timeouts.
   *
   * DRM_IOCTL_I915_GEM_WAIT uses a signed 64 bit timeout and is
   * supposed to block indefinitely timeouts < 0.  Unfortunately,
   * this was broken for a couple of kernel releases.  Since there's
   * no way to know whether or not the kernel we're using is one of
   * the broken ones, the best we can do is to clamp the timeout to
   * INT64_MAX.  This limits the maximum timeout from 584 years to
   * 292 years - likely not a big deal.
   */
   if (abs_timeout < now)
            uint64_t rel_timeout = abs_timeout - now;
   if (rel_timeout > (uint64_t) INT64_MAX)
               }
      static VkResult
   anv_bo_sync_wait(struct vk_device *vk_device,
                  uint32_t wait_count,
      {
      struct anv_device *device = container_of(vk_device, struct anv_device, vk);
   VkResult result;
            uint32_t pending = wait_count;
   while (pending) {
      pending = 0;
   bool signaled = false;
   for (uint32_t i = 0; i < wait_count; i++) {
      struct anv_bo_sync *sync = to_anv_bo_sync(waits[i].sync);
   switch (sync->state) {
   case ANV_BO_SYNC_STATE_RESET:
      /* This fence hasn't been submitted yet, we'll catch it the next
   * time around.  Yes, this may mean we dead-loop but, short of
   * lots of locking and a condition variable, there's not much that
   * we can do about that.
   */
   assert(!(wait_flags & VK_SYNC_WAIT_PENDING));
               case ANV_BO_SYNC_STATE_SIGNALED:
      /* This fence is not pending.  If waitAll isn't set, we can return
   * early.  Otherwise, we have to keep going.
   */
   if (wait_flags & VK_SYNC_WAIT_ANY)
               case ANV_BO_SYNC_STATE_SUBMITTED:
      /* These are the fences we really care about.  Go ahead and wait
   * on it until we hit a timeout.
   */
   if (!(wait_flags & VK_SYNC_WAIT_PENDING)) {
      uint64_t rel_timeout = anv_get_relative_timeout(abs_timeout_ns);
   result = anv_device_wait(device, sync->bo, rel_timeout);
                        sync->state = ANV_BO_SYNC_STATE_SIGNALED;
      }
   if (wait_flags & VK_SYNC_WAIT_ANY)
               default:
                     if (pending && !signaled) {
      /* If we've hit this then someone decided to vkWaitForFences before
   * they've actually submitted any of them to a queue.  This is a
   * fairly pessimal case, so it's ok to lock here and use a standard
   * pthreads condition variable.
                  /* It's possible that some of the fences have changed state since the
   * last time we checked.  Now that we have the lock, check for
   * pending fences again and don't wait if it's changed.
   */
   uint32_t now_pending = 0;
   for (uint32_t i = 0; i < wait_count; i++) {
      struct anv_bo_sync *sync = to_anv_bo_sync(waits[i].sync);
   if (sync->state == ANV_BO_SYNC_STATE_RESET)
                     if (now_pending == pending) {
      struct timespec abstime = {
                        ASSERTED int ret;
   ret = pthread_cond_timedwait(&device->queue_submit,
         assert(ret != EINVAL);
   if (os_time_get_nano() >= abs_timeout_ns) {
      pthread_mutex_unlock(&device->mutex);
                                    }
      const struct vk_sync_type anv_bo_sync_type = {
      .size = sizeof(struct anv_bo_sync),
   .features = VK_SYNC_FEATURE_BINARY |
               VK_SYNC_FEATURE_GPU_WAIT |
   VK_SYNC_FEATURE_GPU_MULTI_WAIT |
   VK_SYNC_FEATURE_CPU_WAIT |
   VK_SYNC_FEATURE_CPU_RESET |
   .init = anv_bo_sync_init,
   .finish = anv_bo_sync_finish,
   .reset = anv_bo_sync_reset,
      };
      VkResult
   anv_create_sync_for_memory(struct vk_device *device,
                     {
      ANV_FROM_HANDLE(anv_device_memory, mem, memory);
            bo_sync = vk_zalloc(&device->alloc, sizeof(*bo_sync), 8,
         if (bo_sync == NULL)
            bo_sync->sync.type = &anv_bo_sync_type;
   bo_sync->state = signal_memory ? ANV_BO_SYNC_STATE_RESET :
                              }
