   /*
   * Copyright © 2015 Intel Corporation
   * Copyright © 2022 Collabora, Ltd
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
      #include "vk_command_pool.h"
      #include "vk_alloc.h"
   #include "vk_command_buffer.h"
   #include "vk_common_entrypoints.h"
   #include "vk_device.h"
   #include "vk_log.h"
      static bool
   should_recycle_command_buffers(struct vk_device *device)
   {
      /* They have to be using the common allocation implementation, otherwise
   * the recycled command buffers will never actually get re-used
   */
   const struct vk_device_dispatch_table *disp = &device->dispatch_table;
   if (disp->AllocateCommandBuffers != vk_common_AllocateCommandBuffers)
            /* We need to be able to reset command buffers */
   if (device->command_buffer_ops->reset == NULL)
               }
      VkResult MUST_CHECK
   vk_command_pool_init(struct vk_device *device,
                     {
      memset(pool, 0, sizeof(*pool));
   vk_object_base_init(device, &pool->base,
            pool->flags = pCreateInfo->flags;
   pool->queue_family_index = pCreateInfo->queueFamilyIndex;
   pool->alloc = pAllocator ? *pAllocator : device->alloc;
   pool->command_buffer_ops = device->command_buffer_ops;
   pool->recycle_command_buffers = should_recycle_command_buffers(device);
   list_inithead(&pool->command_buffers);
               }
      void
   vk_command_pool_finish(struct vk_command_pool *pool)
   {
      list_for_each_entry_safe(struct vk_command_buffer, cmd_buffer,
               }
            list_for_each_entry_safe(struct vk_command_buffer, cmd_buffer,
               }
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_CreateCommandPool(VkDevice _device,
                     {
      VK_FROM_HANDLE(vk_device, device, _device);
   struct vk_command_pool *pool;
            pool = vk_alloc2(&device->alloc, pAllocator, sizeof(*pool), 8,
         if (pool == NULL)
            result = vk_command_pool_init(device, pool, pCreateInfo, pAllocator);
   if (unlikely(result != VK_SUCCESS)) {
      vk_free2(&device->alloc, pAllocator, pool);
                           }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DestroyCommandPool(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_device, device, _device);
            if (pool == NULL)
            vk_command_pool_finish(pool);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_ResetCommandPool(VkDevice device,
               {
      VK_FROM_HANDLE(vk_command_pool, pool, commandPool);
   const struct vk_device_dispatch_table *disp =
         #define COPY_FLAG(flag) \
      if (flags & VK_COMMAND_POOL_RESET_##flag) \
            VkCommandBufferResetFlags cb_flags = 0;
         #undef COPY_FLAG
         list_for_each_entry_safe(struct vk_command_buffer, cmd_buffer,
            VkResult result =
      disp->ResetCommandBuffer(vk_command_buffer_to_handle(cmd_buffer),
      if (result != VK_SUCCESS)
                  }
      static void
   vk_command_buffer_recycle_or_destroy(struct vk_command_pool *pool,
         {
               if (pool->recycle_command_buffers) {
               list_del(&cmd_buffer->pool_link);
      } else {
            }
      static struct vk_command_buffer *
   vk_command_pool_find_free(struct vk_command_pool *pool)
   {
      if (list_is_empty(&pool->free_command_buffers))
            struct vk_command_buffer *cmd_buffer =
      list_first_entry(&pool->free_command_buffers,
         list_del(&cmd_buffer->pool_link);
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_AllocateCommandBuffers(VkDevice device,
               {
      VK_FROM_HANDLE(vk_command_pool, pool, pAllocateInfo->commandPool);
   VkResult result;
                     for (i = 0; i < pAllocateInfo->commandBufferCount; i++) {
      struct vk_command_buffer *cmd_buffer = vk_command_pool_find_free(pool);
   if (cmd_buffer == NULL) {
      result = pool->command_buffer_ops->create(pool, &cmd_buffer);
   if (unlikely(result != VK_SUCCESS))
                                          fail:
      while (i--) {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, pCommandBuffers[i]);
      }
   for (i = 0; i < pAllocateInfo->commandBufferCount; i++)
               }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_FreeCommandBuffers(VkDevice device,
                     {
               for (uint32_t i = 0; i < commandBufferCount; i++) {
               if (cmd_buffer == NULL)
                  }
      void
   vk_command_pool_trim(struct vk_command_pool *pool,
         {
      list_for_each_entry_safe(struct vk_command_buffer, cmd_buffer,
               }
      }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_TrimCommandPool(VkDevice device,
               {
                  }
