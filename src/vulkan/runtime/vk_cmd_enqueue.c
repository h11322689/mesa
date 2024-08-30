   /*
   * Copyright © 2019 Red Hat.
   * Copyright © 2022 Collabora, LTD
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
      #include "vk_alloc.h"
   #include "vk_cmd_enqueue_entrypoints.h"
   #include "vk_command_buffer.h"
   #include "vk_device.h"
   #include "vk_pipeline_layout.h"
   #include "vk_util.h"
      VKAPI_ATTR void VKAPI_CALL
   vk_cmd_enqueue_CmdDrawMultiEXT(VkCommandBuffer commandBuffer,
                                 {
               struct vk_cmd_queue_entry *cmd =
      vk_zalloc(cmd_buffer->cmd_queue.alloc, sizeof(*cmd), 8,
      if (!cmd)
            cmd->type = VK_CMD_DRAW_MULTI_EXT;
            cmd->u.draw_multi_ext.draw_count = drawCount;
   if (pVertexInfo) {
      unsigned i = 0;
   cmd->u.draw_multi_ext.vertex_info =
      vk_zalloc(cmd_buffer->cmd_queue.alloc,
               vk_foreach_multi_draw(draw, i, pVertexInfo, drawCount, stride) {
      memcpy(&cmd->u.draw_multi_ext.vertex_info[i], draw,
         }
   cmd->u.draw_multi_ext.instance_count = instanceCount;
   cmd->u.draw_multi_ext.first_instance = firstInstance;
      }
      VKAPI_ATTR void VKAPI_CALL
   vk_cmd_enqueue_CmdDrawMultiIndexedEXT(VkCommandBuffer commandBuffer,
                                       {
               struct vk_cmd_queue_entry *cmd =
      vk_zalloc(cmd_buffer->cmd_queue.alloc, sizeof(*cmd), 8,
      if (!cmd)
            cmd->type = VK_CMD_DRAW_MULTI_INDEXED_EXT;
                     if (pIndexInfo) {
      unsigned i = 0;
   cmd->u.draw_multi_indexed_ext.index_info =
      vk_zalloc(cmd_buffer->cmd_queue.alloc,
               vk_foreach_multi_draw_indexed(draw, i, pIndexInfo, drawCount, stride) {
      cmd->u.draw_multi_indexed_ext.index_info[i].firstIndex = draw->firstIndex;
   cmd->u.draw_multi_indexed_ext.index_info[i].indexCount = draw->indexCount;
   if (pVertexOffset == NULL)
                  cmd->u.draw_multi_indexed_ext.instance_count = instanceCount;
   cmd->u.draw_multi_indexed_ext.first_instance = firstInstance;
            if (pVertexOffset) {
      cmd->u.draw_multi_indexed_ext.vertex_offset =
      vk_zalloc(cmd_buffer->cmd_queue.alloc,
               memcpy(cmd->u.draw_multi_indexed_ext.vertex_offset, pVertexOffset,
         }
      static void
   push_descriptors_set_free(struct vk_cmd_queue *queue,
         {
   struct vk_cmd_push_descriptor_set_khr *pds = &cmd->u.push_descriptor_set_khr;
   for (unsigned i = 0; i < pds->descriptor_write_count; i++) {
      VkWriteDescriptorSet *entry = &pds->descriptor_writes[i];
   switch (entry->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      vk_free(queue->alloc, (void *)entry->pImageInfo);
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      vk_free(queue->alloc, (void *)entry->pTexelBufferView);
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
   default:
      vk_free(queue->alloc, (void *)entry->pBufferInfo);
         }
   }
      VKAPI_ATTR void VKAPI_CALL
   vk_cmd_enqueue_CmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer,
                                 {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
            struct vk_cmd_queue_entry *cmd =
      vk_zalloc(cmd_buffer->cmd_queue.alloc, sizeof(*cmd), 8,
      if (!cmd)
                     cmd->type = VK_CMD_PUSH_DESCRIPTOR_SET_KHR;
   cmd->driver_free_cb = push_descriptors_set_free;
            pds->pipeline_bind_point = pipelineBindPoint;
   pds->layout = layout;
   pds->set = set;
            if (pDescriptorWrites) {
      pds->descriptor_writes =
      vk_zalloc(cmd_buffer->cmd_queue.alloc,
            memcpy(pds->descriptor_writes,
                  for (unsigned i = 0; i < descriptorWriteCount; i++) {
      switch (pds->descriptor_writes[i].descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      pds->descriptor_writes[i].pImageInfo =
      vk_zalloc(cmd_buffer->cmd_queue.alloc,
            memcpy((VkDescriptorImageInfo *)pds->descriptor_writes[i].pImageInfo,
         pDescriptorWrites[i].pImageInfo,
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      pds->descriptor_writes[i].pTexelBufferView =
      vk_zalloc(cmd_buffer->cmd_queue.alloc,
            memcpy((VkBufferView *)pds->descriptor_writes[i].pTexelBufferView,
         pDescriptorWrites[i].pTexelBufferView,
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
   default:
      pds->descriptor_writes[i].pBufferInfo =
      vk_zalloc(cmd_buffer->cmd_queue.alloc,
            memcpy((VkDescriptorBufferInfo *)pds->descriptor_writes[i].pBufferInfo,
         pDescriptorWrites[i].pBufferInfo,
               }
      static void
   unref_pipeline_layout(struct vk_cmd_queue *queue,
         {
      struct vk_command_buffer *cmd_buffer =
         VK_FROM_HANDLE(vk_pipeline_layout, layout,
                        }
      VKAPI_ATTR void VKAPI_CALL
   vk_cmd_enqueue_CmdBindDescriptorSets(VkCommandBuffer commandBuffer,
                                       VkPipelineBindPoint pipelineBindPoint,
   {
               struct vk_cmd_queue_entry *cmd =
      vk_zalloc(cmd_buffer->cmd_queue.alloc, sizeof(*cmd), 8,
      if (!cmd)
            cmd->type = VK_CMD_BIND_DESCRIPTOR_SETS;
            /* We need to hold a reference to the descriptor set as long as this
   * command is in the queue.  Otherwise, it may get deleted out from under
   * us before the command is replayed.
   */
   vk_pipeline_layout_ref(vk_pipeline_layout_from_handle(layout));
   cmd->u.bind_descriptor_sets.layout = layout;
            cmd->u.bind_descriptor_sets.pipeline_bind_point = pipelineBindPoint;
   cmd->u.bind_descriptor_sets.first_set = firstSet;
   cmd->u.bind_descriptor_sets.descriptor_set_count = descriptorSetCount;
   if (pDescriptorSets) {
      cmd->u.bind_descriptor_sets.descriptor_sets =
      vk_zalloc(cmd_buffer->cmd_queue.alloc,
               memcpy(cmd->u.bind_descriptor_sets.descriptor_sets, pDescriptorSets,
      }
   cmd->u.bind_descriptor_sets.dynamic_offset_count = dynamicOffsetCount;
   if (pDynamicOffsets) {
      cmd->u.bind_descriptor_sets.dynamic_offsets =
      vk_zalloc(cmd_buffer->cmd_queue.alloc,
               memcpy(cmd->u.bind_descriptor_sets.dynamic_offsets, pDynamicOffsets,
         }
      #ifdef VK_ENABLE_BETA_EXTENSIONS
   static void
   dispatch_graph_amdx_free(struct vk_cmd_queue *queue, struct vk_cmd_queue_entry *cmd)
   {
      VkDispatchGraphCountInfoAMDX *count_info = cmd->u.dispatch_graph_amdx.count_info;
            for (uint32_t i = 0; i < count_info->count; i++) {
      VkDispatchGraphInfoAMDX *info = (void *)((const uint8_t *)infos + i * count_info->stride);
                  }
      VKAPI_ATTR void VKAPI_CALL
   vk_cmd_enqueue_CmdDispatchGraphAMDX(VkCommandBuffer commandBuffer, VkDeviceAddress scratch,
         {
               if (vk_command_buffer_has_error(cmd_buffer))
            VkResult result = VK_SUCCESS;
            struct vk_cmd_queue_entry *cmd =
         if (!cmd) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               cmd->type = VK_CMD_DISPATCH_GRAPH_AMDX;
                     cmd->u.dispatch_graph_amdx.count_info =
         if (cmd->u.dispatch_graph_amdx.count_info == NULL)
            memcpy((void *)cmd->u.dispatch_graph_amdx.count_info, pCountInfo,
            uint32_t infos_size = pCountInfo->count * pCountInfo->stride;
   void *infos = vk_zalloc(alloc, infos_size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   cmd->u.dispatch_graph_amdx.count_info->infos.hostAddress = infos;
            for (uint32_t i = 0; i < pCountInfo->count; i++) {
               uint32_t payloads_size = info->payloadCount * info->payloadStride;
   void *dst_payload = vk_zalloc(alloc, payloads_size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   memcpy(dst_payload, info->payloads.hostAddress, payloads_size);
               list_addtail(&cmd->cmd_link, &cmd_buffer->cmd_queue.cmds);
      err:
      if (cmd) {
      vk_free(alloc, cmd);
            finish:
      if (unlikely(result != VK_SUCCESS))
      }
   #endif
