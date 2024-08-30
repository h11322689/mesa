   /*
   * Copyright © 2019 Red Hat.
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
   #include "pipe/p_context.h"
   #include "vk_util.h"
      #include "vk_common_entrypoints.h"
      static void
   lvp_cmd_buffer_destroy(struct vk_command_buffer *cmd_buffer)
   {
      vk_command_buffer_finish(cmd_buffer);
      }
      static VkResult
   lvp_create_cmd_buffer(struct vk_command_pool *pool,
         {
      struct lvp_device *device =
                  cmd_buffer = vk_alloc(&pool->alloc, sizeof(*cmd_buffer), 8,
         if (cmd_buffer == NULL)
            VkResult result = vk_command_buffer_init(pool, &cmd_buffer->vk,
         if (result != VK_SUCCESS) {
      vk_free(&pool->alloc, cmd_buffer);
                                    }
      static void
   lvp_reset_cmd_buffer(struct vk_command_buffer *vk_cmd_buffer,
         {
         }
      const struct vk_command_buffer_ops lvp_cmd_buffer_ops = {
      .create = lvp_create_cmd_buffer,
   .reset = lvp_reset_cmd_buffer,
      };
      VKAPI_ATTR VkResult VKAPI_CALL lvp_BeginCommandBuffer(
      VkCommandBuffer                             commandBuffer,
      {
                           }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_EndCommandBuffer(
         {
                  }
      static void
   lvp_free_CmdPushDescriptorSetWithTemplateKHR(struct vk_cmd_queue *queue, struct vk_cmd_queue_entry *cmd)
   {
      struct lvp_device *device = cmd->driver_data;
   LVP_FROM_HANDLE(lvp_descriptor_update_template, templ, cmd->u.push_descriptor_set_with_template_khr.descriptor_update_template);
      }
      VKAPI_ATTR void VKAPI_CALL lvp_CmdPushDescriptorSetWithTemplateKHR(
      VkCommandBuffer                             commandBuffer,
   VkDescriptorUpdateTemplate                  descriptorUpdateTemplate,
   VkPipelineLayout                            layout,
   uint32_t                                    set,
      {
      LVP_FROM_HANDLE(lvp_cmd_buffer, cmd_buffer, commandBuffer);
   LVP_FROM_HANDLE(lvp_descriptor_update_template, templ, descriptorUpdateTemplate);
   size_t info_size = 0;
   struct vk_cmd_queue_entry *cmd = vk_zalloc(cmd_buffer->vk.cmd_queue.alloc,
               if (!cmd)
                     list_addtail(&cmd->cmd_link, &cmd_buffer->vk.cmd_queue.cmds);
   cmd->driver_free_cb = lvp_free_CmdPushDescriptorSetWithTemplateKHR;
            cmd->u.push_descriptor_set_with_template_khr.descriptor_update_template = descriptorUpdateTemplate;
   lvp_descriptor_template_templ_ref(templ);
   cmd->u.push_descriptor_set_with_template_khr.layout = layout;
            for (unsigned i = 0; i < templ->entry_count; i++) {
               switch (entry->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      info_size += sizeof(VkDescriptorImageInfo) * entry->descriptorCount;
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      info_size += sizeof(VkBufferView) * entry->descriptorCount;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
   default:
      info_size += sizeof(VkDescriptorBufferInfo) * entry->descriptorCount;
                           uint64_t offset = 0;
   for (unsigned i = 0; i < templ->entry_count; i++) {
                        for (unsigned i = 0; i < entry->descriptorCount; i++) {
      memcpy((uint8_t*)cmd->u.push_descriptor_set_with_template_khr.data + offset, (const uint8_t*)pData + entry->offset + i * entry->stride, size);
            }
