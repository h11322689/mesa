   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <stdbool.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <unistd.h>
   #include <poll.h>
   #include <vulkan/vulkan.h>
      #include "pvr_private.h"
   #include "pvr_srv.h"
   #include "pvr_srv_sync.h"
   #include "util/libsync.h"
   #include "util/macros.h"
   #include "util/timespec.h"
   #include "vk_alloc.h"
   #include "vk_log.h"
   #include "vk_sync.h"
   #include "vk_util.h"
      static VkResult pvr_srv_sync_init(struct vk_device *device,
               {
               srv_sync->signaled = initial_value ? true : false;
               }
      void pvr_srv_sync_finish(struct vk_device *device, struct vk_sync *sync)
   {
               if (srv_sync->fd != -1)
      }
      /* Note: function closes the fd. */
   static void pvr_set_sync_state(struct pvr_srv_sync *srv_sync, bool signaled)
   {
      if (srv_sync->fd != -1) {
      close(srv_sync->fd);
                  }
      void pvr_srv_set_sync_payload(struct pvr_srv_sync *srv_sync, int payload)
   {
      if (srv_sync->fd != -1)
            srv_sync->fd = payload;
      }
      static VkResult pvr_srv_sync_signal(struct vk_device *device,
               {
                           }
      static VkResult pvr_srv_sync_reset(struct vk_device *device,
         {
                           }
      /* Careful, timeout might overflow. */
   static inline void pvr_start_timeout(struct timespec *timeout,
         {
      clock_gettime(CLOCK_MONOTONIC, timeout);
      }
      /* Careful, a negative value might be returned. */
   static inline struct timespec
   pvr_get_remaining_time(const struct timespec *timeout)
   {
               clock_gettime(CLOCK_MONOTONIC, &time);
               }
      static inline int pvr_get_relative_time_ms(uint64_t abs_timeout_ns)
   {
      uint64_t cur_time_ms;
            if (abs_timeout_ns >= INT64_MAX) {
      /* This is treated as an infinite wait */
               cur_time_ms = os_time_get_nano() / 1000000;
            if (abs_timeout_ms <= cur_time_ms)
               }
      /* pvr_srv_sync can have pending state, which means they would need spin waits
   */
   static VkResult pvr_srv_sync_get_status(struct vk_sync *wait,
         {
               if (srv_sync->signaled) {
      assert(srv_sync->fd == -1);
               /* If fd is -1 and this is not signaled, the fence is in pending mode */
   if (srv_sync->fd == -1)
            if (sync_wait(srv_sync->fd, pvr_get_relative_time_ms(abs_timeout_ns))) {
      if (errno == ETIME)
         else if (errno == ENOMEM)
         else
                           }
      /* abs_timeout_ns == 0 -> Get status without waiting.
   * abs_timeout_ns == ~0 -> Wait infinitely.
   * else wait for the given abs_timeout_ns in nanoseconds. */
   static VkResult pvr_srv_sync_wait_many(struct vk_device *device,
                           {
               while (true) {
               for (uint32_t i = 0; i < wait_count; i++) {
      VkResult result =
                  if (result != VK_TIMEOUT && (wait_any || result != VK_SUCCESS))
         else if (result == VK_TIMEOUT)
               if (!have_unsignaled)
            if (os_time_get_nano() >= abs_timeout_ns)
            /* TODO: Use pvrsrvkm global events to stop busy waiting and for a bonus
   * catch device loss.
   */
                  }
      static VkResult pvr_srv_sync_move(struct vk_device *device,
               {
      struct pvr_srv_sync *srv_dst_sync = to_srv_sync(dst);
            if (!(dst->flags & VK_SYNC_IS_SHARED) && !(src->flags & VK_SYNC_IS_SHARED)) {
      pvr_srv_set_sync_payload(srv_dst_sync, srv_src_sync->fd);
   srv_src_sync->fd = -1;
   pvr_srv_sync_reset(device, src);
               unreachable("srv_sync doesn't support move for shared sync objects.");
      }
      static VkResult pvr_srv_sync_import_sync_file(struct vk_device *device,
               {
      struct pvr_srv_sync *srv_sync = to_srv_sync(sync);
            if (sync_file >= 0) {
      fd = dup(sync_file);
   if (fd < 0)
                           }
      static VkResult pvr_srv_sync_export_sync_file(struct vk_device *device,
               {
      struct pvr_srv_sync *srv_sync = to_srv_sync(sync);
   VkResult result;
            if (srv_sync->fd < 0) {
      struct pvr_device *driver_device =
            result = pvr_srv_sync_get_presignaled_sync(driver_device, &srv_sync);
   if (result != VK_SUCCESS)
                        fd = dup(srv_sync->fd);
   if (fd < 0)
                        }
      const struct vk_sync_type pvr_srv_sync_type = {
      .size = sizeof(struct pvr_srv_sync),
   /* clang-format off */
   .features = VK_SYNC_FEATURE_BINARY |
               VK_SYNC_FEATURE_GPU_WAIT |
   VK_SYNC_FEATURE_GPU_MULTI_WAIT |
   VK_SYNC_FEATURE_CPU_WAIT |
   VK_SYNC_FEATURE_CPU_RESET |
   /* clang-format on */
   .init = pvr_srv_sync_init,
   .finish = pvr_srv_sync_finish,
   .signal = pvr_srv_sync_signal,
   .reset = pvr_srv_sync_reset,
   .wait_many = pvr_srv_sync_wait_many,
   .move = pvr_srv_sync_move,
   .import_sync_file = pvr_srv_sync_import_sync_file,
      };
