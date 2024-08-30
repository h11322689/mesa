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
      #include "vk_sync_binary.h"
      #include "vk_util.h"
      static struct vk_sync_binary *
   to_vk_sync_binary(struct vk_sync *sync)
   {
                  }
      VkResult
   vk_sync_binary_init(struct vk_device *device,
               {
               const struct vk_sync_binary_type *btype =
            assert(!(sync->flags & VK_SYNC_IS_TIMELINE));
                     return vk_sync_init(device, &binary->timeline, btype->timeline_type,
      }
      static void
   vk_sync_binary_finish(struct vk_device *device,
         {
                  }
      static VkResult
   vk_sync_binary_reset(struct vk_device *device,
         {
                           }
      static VkResult
   vk_sync_binary_signal(struct vk_device *device,
               {
                           }
      static VkResult
   vk_sync_binary_wait_many(struct vk_device *device,
                           {
               for (uint32_t i = 0; i < wait_count; i++) {
               timeline_waits[i] = (struct vk_sync_wait) {
      .sync = &binary->timeline,
   .stage_mask = waits[i].stage_mask,
                  VkResult result = vk_sync_wait_many(device, wait_count, timeline_waits, 
                        }
      struct vk_sync_binary_type
   vk_sync_binary_get_type(const struct vk_sync_type *timeline_type)
   {
               return (struct vk_sync_binary_type) {
      .sync = {
      .size = offsetof(struct vk_sync_binary, timeline) +
         .features = VK_SYNC_FEATURE_BINARY |
               VK_SYNC_FEATURE_GPU_WAIT |
   VK_SYNC_FEATURE_CPU_WAIT |
   VK_SYNC_FEATURE_CPU_RESET |
   VK_SYNC_FEATURE_CPU_SIGNAL |
   .init = vk_sync_binary_init,
   .finish = vk_sync_binary_finish,
   .reset = vk_sync_binary_reset,
   .signal = vk_sync_binary_signal,
      },
         }
