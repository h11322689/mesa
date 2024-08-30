   /*
   * Copyright © 2022 Collabora Ltd
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
      #include "vk_meta_private.h"
      #include "vk_command_buffer.h"
   #include "vk_device.h"
   #include "vk_pipeline.h"
   #include "vk_util.h"
      #include "util/hash_table.h"
      #include <string.h>
      struct cache_key {
      VkObjectType obj_type;
   uint32_t key_size;
      };
      static struct cache_key *
   cache_key_create(VkObjectType obj_type, const void *key_data, size_t key_size)
   {
               struct cache_key *key = malloc(sizeof(*key) + key_size);
   *key = (struct cache_key) {
      .obj_type = obj_type,
   .key_size = key_size,
      };
               }
      static uint32_t
   cache_key_hash(const void *_key)
   {
               assert(sizeof(key->obj_type) == 4);
   uint32_t hash = _mesa_hash_u32(&key->obj_type);
      }
      static bool
   cache_key_equal(const void *_a, const void *_b)
   {
      const struct cache_key *a = _a, *b = _b;
   if (a->obj_type != b->obj_type || a->key_size != b->key_size)
               }
      static void
   destroy_object(struct vk_device *device, struct vk_object_base *obj)
   {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            switch (obj->type) {
   case VK_OBJECT_TYPE_BUFFER:
      disp->DestroyBuffer(_device, (VkBuffer)(uintptr_t)obj, NULL);
      case VK_OBJECT_TYPE_IMAGE_VIEW:
      disp->DestroyImageView(_device, (VkImageView)(uintptr_t)obj, NULL);
      case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
      disp->DestroyDescriptorSetLayout(_device, (VkDescriptorSetLayout)(uintptr_t)obj, NULL);
      case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
      disp->DestroyPipelineLayout(_device, (VkPipelineLayout)(uintptr_t)obj, NULL);
      case VK_OBJECT_TYPE_PIPELINE:
      disp->DestroyPipeline(_device, (VkPipeline)(uintptr_t)obj, NULL);
      case VK_OBJECT_TYPE_SAMPLER:
      disp->DestroySampler(_device, (VkSampler)(uintptr_t)obj, NULL);
      default:
            }
      VkResult
   vk_meta_device_init(struct vk_device *device,
         {
               meta->cache = _mesa_hash_table_create(NULL, cache_key_hash,
                  meta->cmd_draw_rects = vk_meta_draw_rects;
               }
      void
   vk_meta_device_finish(struct vk_device *device,
         {
      hash_table_foreach(meta->cache, entry) {
      free((void *)entry->key);
      }
   _mesa_hash_table_destroy(meta->cache, NULL);
      }
      uint64_t
   vk_meta_lookup_object(struct vk_meta_device *meta,
               {
      assert(key_size >= sizeof(enum vk_meta_object_key_type));
   assert(*(enum vk_meta_object_key_type *)key_data !=
            struct cache_key key = {
      .obj_type = obj_type,
   .key_size = key_size,
                        simple_mtx_lock(&meta->cache_mtx);
   struct hash_entry *entry =
                  if (entry == NULL)
            struct vk_object_base *obj = entry->data;
               }
      uint64_t
   vk_meta_cache_object(struct vk_device *device,
                           {
      assert(key_size >= sizeof(enum vk_meta_object_key_type));
   assert(*(enum vk_meta_object_key_type *)key_data !=
            struct cache_key *key = cache_key_create(obj_type, key_data, key_size);
   struct vk_object_base *obj =
                     simple_mtx_lock(&meta->cache_mtx);
   struct hash_entry *entry =
         if (entry == NULL)
                  if (entry != NULL) {
      /* We raced and found that object already in the cache */
   free(key);
   destroy_object(device, obj);
      } else {
      /* Return the newly inserted object */
         }
      VkResult
   vk_meta_create_sampler(struct vk_device *device,
                           {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            VkSampler sampler;
   VkResult result = disp->CreateSampler(_device, info, NULL, &sampler);
   if (result != VK_SUCCESS)
            *sampler_out = (VkSampler)
      vk_meta_cache_object(device, meta, key_data, key_size,
               }
      VkResult
   vk_meta_create_descriptor_set_layout(struct vk_device *device,
                           {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            VkDescriptorSetLayout layout;
   VkResult result = disp->CreateDescriptorSetLayout(_device, info,
         if (result != VK_SUCCESS)
            *layout_out = (VkDescriptorSetLayout)
      vk_meta_cache_object(device, meta, key_data, key_size,
               }
      static VkResult
   vk_meta_get_descriptor_set_layout(struct vk_device *device,
                           {
      VkDescriptorSetLayout cached =
         if (cached != VK_NULL_HANDLE) {
      *layout_out = cached;
               return vk_meta_create_descriptor_set_layout(device, meta, info,
            }
      VkResult
   vk_meta_create_pipeline_layout(struct vk_device *device,
                           {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            VkPipelineLayout layout;
   VkResult result = disp->CreatePipelineLayout(_device, info, NULL, &layout);
   if (result != VK_SUCCESS)
            *layout_out = (VkPipelineLayout)
      vk_meta_cache_object(device, meta, key_data, key_size,
               }
      VkResult
   vk_meta_get_pipeline_layout(struct vk_device *device,
                                 {
      VkPipelineLayout cached =
         if (cached != VK_NULL_HANDLE) {
      *layout_out = cached;
               VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
   if (desc_info != NULL) {
      VkResult result =
      vk_meta_get_descriptor_set_layout(device, meta, desc_info,
      if (result != VK_SUCCESS)
               const VkPipelineLayoutCreateInfo layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = set_layout != VK_NULL_HANDLE ? 1 : 0,
   .pSetLayouts = &set_layout,
   .pushConstantRangeCount = push_range != NULL ? 1 : 0,
               return vk_meta_create_pipeline_layout(device, meta, &layout_info,
      }
      static VkResult
   create_rect_list_pipeline(struct vk_device *device,
                     {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
                     /* We always configure for layered rendering for now */
            STACK_ARRAY(VkPipelineShaderStageCreateInfo, stages,
                  VkPipelineShaderStageNirCreateInfoMESA vs_nir_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_NIR_CREATE_INFO_MESA,
      };
   stages[stage_count++] = (VkPipelineShaderStageCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .pNext = &vs_nir_info,
   .stage = VK_SHADER_STAGE_VERTEX_BIT,
               VkPipelineShaderStageNirCreateInfoMESA gs_nir_info;
   if (use_gs) {
      gs_nir_info = (VkPipelineShaderStageNirCreateInfoMESA) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_NIR_CREATE_INFO_MESA,
      };
   stages[stage_count++] = (VkPipelineShaderStageCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .pNext = &gs_nir_info,
   .stage = VK_SHADER_STAGE_GEOMETRY_BIT,
                  for (uint32_t i = 0; i < info->stageCount; i++) {
      assert(info->pStages[i].stage != VK_SHADER_STAGE_VERTEX_BIT);
   if (use_gs)
                     info_local.stageCount = stage_count;
   info_local.pStages = stages;
   info_local.pVertexInputState = &vk_meta_draw_rects_vi_state;
            uint32_t dyn_count = info->pDynamicState != NULL ?
            STACK_ARRAY(VkDynamicState, dyn_state, dyn_count + 2);
   for (uint32_t i = 0; i < dyn_count; i++)
            dyn_state[dyn_count + 0] = VK_DYNAMIC_STATE_VIEWPORT;
            const VkPipelineDynamicStateCreateInfo dyn_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
   .dynamicStateCount = dyn_count + 2,
                        VkResult result = disp->CreateGraphicsPipelines(_device, VK_NULL_HANDLE,
                  STACK_ARRAY_FINISH(dyn_state);
               }
      static const VkPipelineRasterizationStateCreateInfo default_rs_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
   .depthClampEnable = false,
   .depthBiasEnable = false,
   .polygonMode = VK_POLYGON_MODE_FILL,
   .cullMode = VK_CULL_MODE_NONE,
      };
      static const VkPipelineDepthStencilStateCreateInfo default_ds_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
   .depthTestEnable = false,
   .depthBoundsTestEnable = false,
      };
      VkResult
   vk_meta_create_graphics_pipeline(struct vk_device *device,
                                 {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
   VkDevice _device = vk_device_to_handle(device);
                     /* Add in the rendering info */
   VkPipelineRenderingCreateInfo r_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
   .viewMask = render->view_mask,
   .colorAttachmentCount = render->color_attachment_count,
   .pColorAttachmentFormats = render->color_attachment_formats,
   .depthAttachmentFormat = render->depth_attachment_format,
      };
            /* Assume rectangle pipelines */
   if (info_local.pInputAssemblyState == NULL)
            if (info_local.pRasterizationState == NULL)
            VkPipelineMultisampleStateCreateInfo ms_info;
   if (info_local.pMultisampleState == NULL) {
      ms_info = (VkPipelineMultisampleStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      };
               if (info_local.pDepthStencilState == NULL)
            VkPipelineColorBlendStateCreateInfo cb_info;
   VkPipelineColorBlendAttachmentState cb_att[MESA_VK_MAX_COLOR_ATTACHMENTS];
   if (info_local.pColorBlendState == NULL) {
      for (uint32_t i = 0; i < render->color_attachment_count; i++) {
      cb_att[i] = (VkPipelineColorBlendAttachmentState) {
      .blendEnable = false,
   .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                     }
   cb_info = (VkPipelineColorBlendStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
   .attachmentCount = render->color_attachment_count,
      };
               VkPipeline pipeline;
   if (info_local.pInputAssemblyState->topology ==
      VK_PRIMITIVE_TOPOLOGY_META_RECT_LIST_MESA) {
   result = create_rect_list_pipeline(device, meta,
            } else {
      result = disp->CreateGraphicsPipelines(_device, VK_NULL_HANDLE,
            }
   if (unlikely(result != VK_SUCCESS))
            *pipeline_out = (VkPipeline)vk_meta_cache_object(device, meta,
                        }
      VkResult
   vk_meta_create_compute_pipeline(struct vk_device *device,
                           {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            VkPipeline pipeline;
   VkResult result = disp->CreateComputePipelines(_device, VK_NULL_HANDLE,
         if (result != VK_SUCCESS)
            *pipeline_out = (VkPipeline)vk_meta_cache_object(device, meta,
                        }
      void
   vk_meta_object_list_init(struct vk_meta_object_list *mol)
   {
         }
      void
   vk_meta_object_list_reset(struct vk_device *device,
         {
      util_dynarray_foreach(&mol->arr, struct vk_object_base *, obj)
               }
      void
   vk_meta_object_list_finish(struct vk_device *device,
         {
      vk_meta_object_list_reset(device, mol);
      }
      VkResult
   vk_meta_create_buffer(struct vk_command_buffer *cmd,
                     {
      struct vk_device *device = cmd->base.device;
   const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            VkResult result = disp->CreateBuffer(_device, info, NULL, buffer_out);
   if (unlikely(result != VK_SUCCESS))
            vk_meta_object_list_add_handle(&cmd->meta_objects,
                  }
      VkResult
   vk_meta_create_image_view(struct vk_command_buffer *cmd,
                     {
      struct vk_device *device = cmd->base.device;
   const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            VkResult result = disp->CreateImageView(_device, info, NULL, image_view_out);
   if (unlikely(result != VK_SUCCESS))
            vk_meta_object_list_add_handle(&cmd->meta_objects,
                  }
