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
      #include "vk_command_buffer.h"
      #include "vk_command_pool.h"
   #include "vk_common_entrypoints.h"
   #include "vk_device.h"
      VkResult
   vk_command_buffer_init(struct vk_command_pool *pool,
                     {
      memset(command_buffer, 0, sizeof(*command_buffer));
   vk_object_base_init(pool->base.device, &command_buffer->base,
            command_buffer->pool = pool;
   command_buffer->level = level;
   command_buffer->ops = ops;
   vk_dynamic_graphics_state_init(&command_buffer->dynamic_graphics_state);
   command_buffer->state = MESA_VK_COMMAND_BUFFER_STATE_INITIAL;
   command_buffer->record_result = VK_SUCCESS;
   vk_cmd_queue_init(&command_buffer->cmd_queue, &pool->alloc);
   vk_meta_object_list_init(&command_buffer->meta_objects);
   util_dynarray_init(&command_buffer->labels, NULL);
                        }
      void
   vk_command_buffer_reset(struct vk_command_buffer *command_buffer)
   {
      vk_dynamic_graphics_state_clear(&command_buffer->dynamic_graphics_state);
   command_buffer->state = MESA_VK_COMMAND_BUFFER_STATE_INITIAL;
   command_buffer->record_result = VK_SUCCESS;
   vk_command_buffer_reset_render_pass(command_buffer);
   vk_cmd_queue_reset(&command_buffer->cmd_queue);
   vk_meta_object_list_reset(command_buffer->base.device,
         util_dynarray_clear(&command_buffer->labels);
      }
      void
   vk_command_buffer_begin(struct vk_command_buffer *command_buffer,
         {
      if (command_buffer->state != MESA_VK_COMMAND_BUFFER_STATE_INITIAL &&
      command_buffer->ops->reset != NULL)
            }
      VkResult
   vk_command_buffer_end(struct vk_command_buffer *command_buffer)
   {
               if (vk_command_buffer_has_error(command_buffer))
         else
               }
      void
   vk_command_buffer_finish(struct vk_command_buffer *command_buffer)
   {
      list_del(&command_buffer->pool_link);
   vk_command_buffer_reset_render_pass(command_buffer);
   vk_cmd_queue_finish(&command_buffer->cmd_queue);
   util_dynarray_fini(&command_buffer->labels);
   vk_meta_object_list_finish(command_buffer->base.device,
            }
      void
   vk_command_buffer_recycle(struct vk_command_buffer *cmd_buffer)
   {
      /* Reset, returning resources to the pool.  The command buffer object
   * itself will be recycled but, if the driver supports returning other
   * resources such as batch buffers to the pool, it should do so so they're
   * not tied up in recycled command buffer objects.
   */
   cmd_buffer->ops->reset(cmd_buffer,
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_ResetCommandBuffer(VkCommandBuffer commandBuffer,
         {
               if (cmd_buffer->state != MESA_VK_COMMAND_BUFFER_STATE_INITIAL)
               }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdExecuteCommands(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(vk_command_buffer, primary, commandBuffer);
   const struct vk_device_dispatch_table *disp =
            for (uint32_t i = 0; i < commandBufferCount; i++) {
                     }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdBindVertexBuffers(VkCommandBuffer commandBuffer,
                           {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
   const struct vk_device_dispatch_table *disp =
            disp->CmdBindVertexBuffers2(commandBuffer, firstBinding, bindingCount,
      }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdBindIndexBuffer(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    buffer,
   VkDeviceSize                                offset,
      {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
   const struct vk_device_dispatch_table *disp =
            disp->CmdBindIndexBuffer2KHR(commandBuffer, buffer, offset,
      }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdDispatch(VkCommandBuffer commandBuffer,
                     {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
   const struct vk_device_dispatch_table *disp =
            disp->CmdDispatchBase(commandBuffer, 0, 0, 0,
      }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_CmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask)
   {
      /* Nothing to do here since we only support a single device */
      }
