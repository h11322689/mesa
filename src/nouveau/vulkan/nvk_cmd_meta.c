   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_buffer.h"
   #include "nvk_cmd_buffer.h"
   #include "nvk_device.h"
   #include "nvk_entrypoints.h"
   #include "nvk_image.h"
   #include "nvk_physical_device.h"
      #include "nvk_cl9097.h"
   #include "nvk_clb197.h"
      static VkResult
   nvk_cmd_bind_map_buffer(struct vk_command_buffer *vk_cmd,
               {
      struct nvk_cmd_buffer *cmd =
         VK_FROM_HANDLE(nvk_buffer, buffer, _buffer);
            uint64_t addr;
   assert(buffer->vk.size < UINT_MAX);
   result = nvk_cmd_buffer_upload_alloc(cmd, buffer->vk.size, 16,
         if (unlikely(result != VK_SUCCESS))
                        }
      VkResult
   nvk_device_init_meta(struct nvk_device *dev)
   {
               VkResult result = vk_meta_device_init(&dev->vk, &dev->meta);
   if (result != VK_SUCCESS)
            dev->meta.use_gs_for_layer = pdev->info.cls_eng3d < MAXWELL_B,
   dev->meta.cmd_bind_map_buffer = nvk_cmd_bind_map_buffer;
               }
      void
   nvk_device_finish_meta(struct nvk_device *dev)
   {
         }
      struct nvk_meta_save {
      struct vk_vertex_input_state _dynamic_vi;
   struct vk_sample_locations_state _dynamic_sl;
   struct vk_dynamic_graphics_state dynamic;
   struct nvk_graphics_pipeline *pipeline;
   struct nvk_addr_range vb0;
   struct nvk_descriptor_set *desc0;
   bool has_push_desc0;
   struct nvk_push_descriptor_set push_desc0;
      };
      static void
   nvk_meta_begin(struct nvk_cmd_buffer *cmd,
         {
      save->dynamic = cmd->vk.dynamic_graphics_state;
   save->_dynamic_vi = cmd->state.gfx._dynamic_vi;
            save->pipeline = cmd->state.gfx.pipeline;
            save->desc0 = cmd->state.gfx.descriptors.sets[0];
   save->has_push_desc0 = cmd->state.gfx.descriptors.push[0];
   if (save->has_push_desc0)
            STATIC_ASSERT(sizeof(save->push) ==
                  struct nv_push *p = nvk_cmd_buffer_push(cmd, 2);
   P_IMMD(p, NV9097, SET_STATISTICS_COUNTER, {
      .da_vertices_generated_enable = false,
   .da_primitives_generated_enable = false,
   .vs_invocations_enable = false,
   .gs_invocations_enable = false,
   .gs_primitives_generated_enable = false,
   .streaming_primitives_succeeded_enable = false,
   .streaming_primitives_needed_enable = false,
   .clipper_invocations_enable = false,
   .clipper_primitives_generated_enable = false,
   .ps_invocations_enable = false,
   .ti_invocations_enable = false,
   .ts_invocations_enable = false,
   .ts_primitives_generated_enable = false,
   .total_streaming_primitives_needed_succeeded_enable = false,
         }
      static void
   nvk_meta_init_render(struct nvk_cmd_buffer *cmd,
         {
               *info = (struct vk_meta_rendering_info) {
      .view_mask = render->view_mask,
   .color_attachment_count = render->color_att_count,
   .depth_attachment_format = render->depth_att.vk_format,
      };
   for (uint32_t a = 0; a < render->color_att_count; a++)
      }
      static void
   nvk_meta_end(struct nvk_cmd_buffer *cmd,
         {
      if (save->desc0) {
      cmd->state.gfx.descriptors.sets[0] = save->desc0;
   cmd->state.gfx.descriptors.root.sets[0] = nvk_descriptor_set_addr(save->desc0);
   cmd->state.gfx.descriptors.sets_dirty |= BITFIELD_BIT(0);
      } else if (save->has_push_desc0) {
      *cmd->state.gfx.descriptors.push[0] = save->push_desc0;
               /* Restore the dynamic state */
   assert(save->dynamic.vi == &cmd->state.gfx._dynamic_vi);
   assert(save->dynamic.ms.sample_locations == &cmd->state.gfx._dynamic_sl);
   cmd->vk.dynamic_graphics_state = save->dynamic;
   cmd->state.gfx._dynamic_vi = save->_dynamic_vi;
   cmd->state.gfx._dynamic_sl = save->_dynamic_sl;
   memcpy(cmd->vk.dynamic_graphics_state.dirty,
                  if (save->pipeline)
                              struct nv_push *p = nvk_cmd_buffer_push(cmd, 2);
   P_IMMD(p, NV9097, SET_STATISTICS_COUNTER, {
      .da_vertices_generated_enable = true,
   .da_primitives_generated_enable = true,
   .vs_invocations_enable = true,
   .gs_invocations_enable = true,
   .gs_primitives_generated_enable = true,
   .streaming_primitives_succeeded_enable = true,
   .streaming_primitives_needed_enable = true,
   .clipper_invocations_enable = true,
   .clipper_primitives_generated_enable = true,
   .ps_invocations_enable = true,
   .ti_invocations_enable = true,
   .ts_invocations_enable = true,
   .ts_primitives_generated_enable = true,
   .total_streaming_primitives_needed_succeeded_enable = true,
         }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdBlitImage2(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
            struct nvk_meta_save save;
                        }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdResolveImage2(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
            struct nvk_meta_save save;
                        }
      void
   nvk_meta_resolve_rendering(struct nvk_cmd_buffer *cmd,
         {
               struct nvk_meta_save save;
                        }
