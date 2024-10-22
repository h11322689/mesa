   /*
   * Copyright © 2022 Collabora, Ltd.
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
      #include "vk_query_pool.h"
      #include "vk_alloc.h"
   #include "vk_command_buffer.h"
   #include "vk_common_entrypoints.h"
   #include "vk_device.h"
      void
   vk_query_pool_init(struct vk_device *device,
               {
                        query_pool->query_type = pCreateInfo->queryType;
   query_pool->query_count = pCreateInfo->queryCount;
   query_pool->pipeline_statistics =
      pCreateInfo->queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS ?
   }
      void *
   vk_query_pool_create(struct vk_device *device,
                     {
      struct vk_query_pool *query_pool =
      vk_zalloc2(&device->alloc, alloc, size, 8,
      if (query_pool == NULL)
                        }
      void
   vk_query_pool_finish(struct vk_query_pool *query_pool)
   {
         }
      void
   vk_query_pool_destroy(struct vk_device *device,
               {
         }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdBeginQuery(VkCommandBuffer commandBuffer,
                     {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
   const struct vk_device_dispatch_table *disp =
               }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdEndQuery(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
   const struct vk_device_dispatch_table *disp =
               }
