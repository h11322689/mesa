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
      #include "vk_sync.h"
      #include <assert.h>
   #include <string.h>
      #include "util/u_debug.h"
   #include "util/macros.h"
   #include "util/os_time.h"
      #include "vk_alloc.h"
   #include "vk_device.h"
   #include "vk_log.h"
      static void
   vk_sync_type_validate(const struct vk_sync_type *type)
   {
      assert(type->init);
            assert(type->features & (VK_SYNC_FEATURE_BINARY |
            if (type->features & VK_SYNC_FEATURE_TIMELINE) {
      assert(type->features & VK_SYNC_FEATURE_GPU_WAIT);
   assert(type->features & VK_SYNC_FEATURE_CPU_WAIT);
   assert(type->features & VK_SYNC_FEATURE_CPU_SIGNAL);
   assert(type->features & (VK_SYNC_FEATURE_WAIT_BEFORE_SIGNAL |
         assert(type->signal);
               if (!(type->features & VK_SYNC_FEATURE_BINARY)) {
      assert(!(type->features & (VK_SYNC_FEATURE_GPU_MULTI_WAIT |
         assert(!type->import_sync_file);
               if (type->features & VK_SYNC_FEATURE_CPU_WAIT) {
         } else {
      assert(!(type->features & (VK_SYNC_FEATURE_WAIT_ANY |
               if (type->features & VK_SYNC_FEATURE_GPU_MULTI_WAIT)
            if (type->features & VK_SYNC_FEATURE_CPU_RESET)
            if (type->features & VK_SYNC_FEATURE_CPU_SIGNAL)
      }
      VkResult
   vk_sync_init(struct vk_device *device,
               struct vk_sync *sync,
   const struct vk_sync_type *type,
   {
               if (flags & VK_SYNC_IS_TIMELINE)
         else
            assert(type->size >= sizeof(*sync));
   memset(sync, 0, type->size);
   sync->type = type;
               }
      void
   vk_sync_finish(struct vk_device *device,
         {
         }
      VkResult
   vk_sync_create(struct vk_device *device,
                  const struct vk_sync_type *type,
      {
               sync = vk_alloc(&device->alloc, type->size, 8,
         if (sync == NULL)
            VkResult result = vk_sync_init(device, sync, type, flags, initial_value);
   if (result != VK_SUCCESS) {
      vk_free(&device->alloc, sync);
                           }
      void
   vk_sync_destroy(struct vk_device *device,
         {
      vk_sync_finish(device, sync);
      }
      VkResult
   vk_sync_signal(struct vk_device *device,
               {
               if (sync->flags & VK_SYNC_IS_TIMELINE)
         else
               }
      VkResult
   vk_sync_get_value(struct vk_device *device,
               {
      assert(sync->flags & VK_SYNC_IS_TIMELINE);
      }
      VkResult
   vk_sync_reset(struct vk_device *device,
         {
      assert(sync->type->features & VK_SYNC_FEATURE_CPU_RESET);
   assert(!(sync->flags & VK_SYNC_IS_TIMELINE));
      }
      VkResult vk_sync_move(struct vk_device *device,
               {
      assert(!(dst->flags & VK_SYNC_IS_TIMELINE));
   assert(!(src->flags & VK_SYNC_IS_TIMELINE));
               }
      static void
   assert_valid_wait(struct vk_sync *sync,
               {
               if (!(sync->flags & VK_SYNC_IS_TIMELINE))
            if (wait_flags & VK_SYNC_WAIT_PENDING)
      }
      static uint64_t
   get_max_abs_timeout_ns(void)
   {
      static int max_timeout_ms = -1;
   if (max_timeout_ms < 0)
            if (max_timeout_ms == 0)
         else
      }
      static VkResult
   __vk_sync_wait(struct vk_device *device,
                  struct vk_sync *sync,
      {
               /* This doesn't make sense for a single wait */
            if (sync->type->wait) {
      return sync->type->wait(device, sync, wait_value,
      } else {
      struct vk_sync_wait wait = {
      .sync = sync,
   .stage_mask = ~(VkPipelineStageFlags2)0,
      };
   return sync->type->wait_many(device, 1, &wait, wait_flags,
         }
      VkResult
   vk_sync_wait(struct vk_device *device,
               struct vk_sync *sync,
   uint64_t wait_value,
   {
      uint64_t max_abs_timeout_ns = get_max_abs_timeout_ns();
   if (abs_timeout_ns > max_abs_timeout_ns) {
      VkResult result =
      __vk_sync_wait(device, sync, wait_value, wait_flags,
      if (unlikely(result == VK_TIMEOUT))
            } else {
      return __vk_sync_wait(device, sync, wait_value, wait_flags,
         }
      static bool
   can_wait_many(uint32_t wait_count,
               {
      if (waits[0].sync->type->wait_many == NULL)
            if ((wait_flags & VK_SYNC_WAIT_ANY) &&
      !(waits[0].sync->type->features & VK_SYNC_FEATURE_WAIT_ANY))
         for (uint32_t i = 0; i < wait_count; i++) {
      assert_valid_wait(waits[i].sync, waits[i].wait_value, wait_flags);
   if (waits[i].sync->type != waits[0].sync->type)
                  }
      static VkResult
   __vk_sync_wait_many(struct vk_device *device,
                     uint32_t wait_count,
   {
      if (wait_count == 0)
            if (wait_count == 1) {
      return __vk_sync_wait(device, waits[0].sync, waits[0].wait_value,
               if (can_wait_many(wait_count, waits, wait_flags)) {
      return waits[0].sync->type->wait_many(device, wait_count, waits,
      } else if (wait_flags & VK_SYNC_WAIT_ANY) {
      /* If we have multiple syncs and they don't support wait_any or they're
   * not all the same type, there's nothing better we can do than spin.
   */
   do {
      for (uint32_t i = 0; i < wait_count; i++) {
      VkResult result = __vk_sync_wait(device, waits[i].sync,
                     if (result != VK_TIMEOUT)
                     } else {
      for (uint32_t i = 0; i < wait_count; i++) {
      VkResult result = __vk_sync_wait(device, waits[i].sync,
               if (result != VK_SUCCESS)
      }
         }
      VkResult
   vk_sync_wait_many(struct vk_device *device,
                     uint32_t wait_count,
   {
      uint64_t max_abs_timeout_ns = get_max_abs_timeout_ns();
   if (abs_timeout_ns > max_abs_timeout_ns) {
      VkResult result =
      __vk_sync_wait_many(device, wait_count, waits, wait_flags,
      if (unlikely(result == VK_TIMEOUT))
            } else {
      return __vk_sync_wait_many(device, wait_count, waits, wait_flags,
         }
      VkResult
   vk_sync_import_opaque_fd(struct vk_device *device,
               {
      VkResult result = sync->type->import_opaque_fd(device, sync, fd);
   if (unlikely(result != VK_SUCCESS))
            sync->flags |= VK_SYNC_IS_SHAREABLE |
               }
      VkResult
   vk_sync_export_opaque_fd(struct vk_device *device,
               {
               VkResult result = sync->type->export_opaque_fd(device, sync, fd);
   if (unlikely(result != VK_SUCCESS))
                        }
      VkResult
   vk_sync_import_sync_file(struct vk_device *device,
               {
               /* Silently handle negative file descriptors in case the driver doesn't
   * want to bother.
   */
   if (sync_file < 0 && sync->type->signal)
               }
      VkResult
   vk_sync_export_sync_file(struct vk_device *device,
               {
      assert(!(sync->flags & VK_SYNC_IS_TIMELINE));
      }
      VkResult
   vk_sync_import_win32_handle(struct vk_device *device,
                     {
      VkResult result = sync->type->import_win32_handle(device, sync, handle, name);
   if (unlikely(result != VK_SUCCESS))
            sync->flags |= VK_SYNC_IS_SHAREABLE |
               }
      VkResult
   vk_sync_export_win32_handle(struct vk_device *device,
               {
               VkResult result = sync->type->export_win32_handle(device, sync, handle);
   if (unlikely(result != VK_SUCCESS))
                        }
      VkResult
   vk_sync_set_win32_export_params(struct vk_device *device,
                           {
                  }
