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
   #include "vk_format.h"
   #include "vk_pipeline.h"
      #include "nir_builder.h"
      struct vk_meta_blit_key {
      enum vk_meta_object_key_type key_type;
   enum glsl_sampler_dim dim;
   VkSampleCountFlagBits src_samples;
   VkResolveModeFlagBits resolve_mode;
   VkResolveModeFlagBits stencil_resolve_mode;
   bool stencil_as_discard;
   VkFormat dst_format;
      };
      static enum glsl_sampler_dim
   vk_image_sampler_dim(const struct vk_image *image)
   {
      switch (image->image_type) {
   case VK_IMAGE_TYPE_1D: return GLSL_SAMPLER_DIM_1D;
   case VK_IMAGE_TYPE_2D:
      if (image->samples > 1)
         else
      case VK_IMAGE_TYPE_3D: return GLSL_SAMPLER_DIM_3D;
   default: unreachable("Invalid image type");
      }
      enum blit_desc_binding {
      BLIT_DESC_BINDING_SAMPLER,
   BLIT_DESC_BINDING_COLOR,
   BLIT_DESC_BINDING_DEPTH,
      };
      static enum blit_desc_binding
   aspect_to_tex_binding(VkImageAspectFlagBits aspect)
   {
      switch (aspect) {
   case VK_IMAGE_ASPECT_COLOR_BIT: return BLIT_DESC_BINDING_COLOR;
   case VK_IMAGE_ASPECT_DEPTH_BIT: return BLIT_DESC_BINDING_DEPTH;
   case VK_IMAGE_ASPECT_STENCIL_BIT: return BLIT_DESC_BINDING_STENCIL;
   default: unreachable("Unsupported aspect");
      }
      struct vk_meta_blit_push_data {
      float x_off, y_off, x_scale, y_scale;
   float z_off, z_scale;
   int32_t arr_delta;
      };
      static inline void
   compute_off_scale(uint32_t src_level_size,
                     uint32_t src0, uint32_t src1,
   {
               if (dst0 < dst1) {
      *dst0_out = dst0;
      } else {
      *dst0_out = dst1;
            /* Flip the source region */
   uint32_t tmp = src0;
   src0 = src1;
               double src_region_size = (double)src1 - (double)src0;
            double dst_region_size = (double)*dst1_out - (double)*dst0_out;
            double src_offset = src0 / (double)src_level_size;
   double dst_scale = src_region_size / (src_level_size * dst_region_size);
            *off_out = src_offset - dst_offset;
      }
      static inline nir_def *
   load_struct_var(nir_builder *b, nir_variable *var, uint32_t field)
   {
      nir_deref_instr *deref =
            }
      static nir_def *
   build_tex_resolve(nir_builder *b, nir_deref_instr *t,
                     {
      nir_def *accum = nir_txf_ms_deref(b, t, coord, nir_imm_int(b, 0));
   if (resolve_mode == VK_RESOLVE_MODE_SAMPLE_ZERO_BIT)
            const enum glsl_base_type base_type =
            for (unsigned i = 1; i < samples; i++) {
      nir_def *val = nir_txf_ms_deref(b, t, coord, nir_imm_int(b, i));
   switch (resolve_mode) {
   case VK_RESOLVE_MODE_AVERAGE_BIT:
      assert(base_type == GLSL_TYPE_FLOAT);
               case VK_RESOLVE_MODE_MIN_BIT:
      switch (base_type) {
   case GLSL_TYPE_UINT:
      accum = nir_umin(b, accum, val);
      case GLSL_TYPE_INT:
      accum = nir_imin(b, accum, val);
      case GLSL_TYPE_FLOAT:
      accum = nir_fmin(b, accum, val);
      default:
                     case VK_RESOLVE_MODE_MAX_BIT:
      switch (base_type) {
   case GLSL_TYPE_UINT:
      accum = nir_umax(b, accum, val);
      case GLSL_TYPE_INT:
      accum = nir_imax(b, accum, val);
      case GLSL_TYPE_FLOAT:
      accum = nir_fmax(b, accum, val);
      default:
                     default:
                     if (resolve_mode == VK_RESOLVE_MODE_AVERAGE_BIT)
               }
      static nir_shader *
   build_blit_shader(const struct vk_meta_blit_key *key)
   {
      nir_builder build;
   if (key->resolve_mode || key->stencil_resolve_mode) {
      build = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT, NULL,
      } else {
      build = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT,
      }
            struct glsl_struct_field push_fields[] = {
      { .type = glsl_vec4_type(), .name = "xy_xform", .offset = 0 },
      };
   const struct glsl_type *push_iface_type =
      glsl_interface_type(push_fields, ARRAY_SIZE(push_fields),
            nir_variable *push = nir_variable_create(b->shader, nir_var_mem_push_const,
            nir_def *xy_xform = load_struct_var(b, push, 0);
   nir_def *xy_off = nir_channels(b, xy_xform, 3 << 0);
            nir_def *out_coord_xy = nir_load_frag_coord(b);
   out_coord_xy = nir_trim_vector(b, out_coord_xy, 2);
            nir_def *z_xform = load_struct_var(b, push, 1);
   nir_def *out_layer = nir_load_layer_id(b);
   nir_def *src_coord;
   if (key->dim == GLSL_SAMPLER_DIM_3D) {
      nir_def *z_off = nir_channel(b, z_xform, 0);
   nir_def *z_scale = nir_channel(b, z_xform, 1);
   nir_def *out_coord_z = nir_fadd_imm(b, nir_u2f32(b, out_layer), 0.5);
   nir_def *src_coord_z = nir_ffma(b, out_coord_z, z_scale, z_off);
   src_coord = nir_vec3(b, nir_channel(b, src_coord_xy, 0),
            } else {
      nir_def *arr_delta = nir_channel(b, z_xform, 2);
   nir_def *in_layer = nir_iadd(b, out_layer, arr_delta);
   if (key->dim == GLSL_SAMPLER_DIM_1D) {
      src_coord = nir_vec2(b, nir_channel(b, src_coord_xy, 0),
      } else {
      assert(key->dim == GLSL_SAMPLER_DIM_2D ||
         src_coord = nir_vec3(b, nir_channel(b, src_coord_xy, 0),
                        nir_variable *sampler = nir_variable_create(b->shader, nir_var_uniform,
         sampler->data.descriptor_set = 0;
   sampler->data.binding = BLIT_DESC_BINDING_SAMPLER;
            u_foreach_bit(a, key->aspects) {
               enum glsl_base_type base_type;
   unsigned out_location, out_comps;
   const char *tex_name, *out_name;
   VkResolveModeFlagBits resolve_mode;
   switch (aspect) {
   case VK_IMAGE_ASPECT_COLOR_BIT:
      tex_name = "color_tex";
   if (vk_format_is_int(key->dst_format))
         else if (vk_format_is_uint(key->dst_format))
         else
         resolve_mode = key->resolve_mode;
   out_name = "gl_FragData[0]";
   out_location = FRAG_RESULT_DATA0;
   out_comps = 4;
      case VK_IMAGE_ASPECT_DEPTH_BIT:
      tex_name = "depth_tex";
   base_type = GLSL_TYPE_FLOAT;
   resolve_mode = key->resolve_mode;
   out_name = "gl_FragDepth";
   out_location = FRAG_RESULT_DEPTH;
   out_comps = 1;
      case VK_IMAGE_ASPECT_STENCIL_BIT:
      tex_name = "stencil_tex";
   base_type = GLSL_TYPE_UINT;
   resolve_mode = key->stencil_resolve_mode;
   out_name = "gl_FragStencilRef";
   out_location = FRAG_RESULT_STENCIL;
   out_comps = 1;
      default:
                  const bool is_array = key->dim != GLSL_SAMPLER_DIM_3D;
   const struct glsl_type *texture_type =
         nir_variable *texture = nir_variable_create(b->shader, nir_var_uniform,
         texture->data.descriptor_set = 0;
   texture->data.binding = aspect_to_tex_binding(aspect);
            nir_def *val;
   if (resolve_mode == VK_RESOLVE_MODE_NONE) {
         } else {
      val = build_tex_resolve(b, t, nir_f2u32(b, src_coord),
      }
            if (key->stencil_as_discard) {
      assert(key->aspects == VK_IMAGE_ASPECT_STENCIL_BIT);
   nir_def *stencil_bit = nir_channel(b, z_xform, 3);
   nir_discard_if(b, nir_ieq(b, nir_iand(b, val, stencil_bit),
      } else {
      const struct glsl_type *out_type =
         nir_variable *out = nir_variable_create(b->shader, nir_var_shader_out,
                                    }
      static VkResult
   get_blit_pipeline_layout(struct vk_device *device,
               {
               const VkDescriptorSetLayoutBinding bindings[] = {{
      .binding = BLIT_DESC_BINDING_SAMPLER,
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
   .descriptorCount = 1,
      }, {
      .binding = BLIT_DESC_BINDING_COLOR,
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   .descriptorCount = 1,
      }, {
      .binding = BLIT_DESC_BINDING_DEPTH,
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   .descriptorCount = 1,
      }, {
      .binding = BLIT_DESC_BINDING_STENCIL,
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   .descriptorCount = 1,
               const VkDescriptorSetLayoutCreateInfo desc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
   .bindingCount = ARRAY_SIZE(bindings),
               const VkPushConstantRange push_range = {
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
   .offset = 0,
               return vk_meta_get_pipeline_layout(device, meta, &desc_info, &push_range,
      }
      static VkResult
   get_blit_pipeline(struct vk_device *device,
                     struct vk_meta_device *meta,
   {
      VkPipeline from_cache = vk_meta_lookup_pipeline(meta, key, sizeof(*key));
   if (from_cache != VK_NULL_HANDLE) {
      *pipeline_out = from_cache;
               const VkPipelineShaderStageNirCreateInfoMESA fs_nir_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_NIR_CREATE_INFO_MESA,
      };
   const VkPipelineShaderStageCreateInfo fs_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .pNext = &fs_nir_info,
   .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
               VkPipelineDepthStencilStateCreateInfo ds_info = {
         };
   VkDynamicState dyn_tmp;
   VkPipelineDynamicStateCreateInfo dyn_info = {
         };
   struct vk_meta_rendering_info render = {
         };
   if (key->aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
      render.color_attachment_count = 1;
      }
   if (key->aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
      ds_info.depthTestEnable = VK_TRUE;
   ds_info.depthWriteEnable = VK_TRUE;
   ds_info.depthCompareOp = VK_COMPARE_OP_ALWAYS;
      }
   if (key->aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      ds_info.stencilTestEnable = VK_TRUE;
   ds_info.front.compareOp = VK_COMPARE_OP_ALWAYS;
   ds_info.front.passOp = VK_STENCIL_OP_REPLACE;
   ds_info.front.compareMask = ~0u;
   ds_info.front.writeMask = ~0u;
   ds_info.front.reference = ~0;
   ds_info.back = ds_info.front;
   if (key->stencil_as_discard) {
      dyn_tmp = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
   dyn_info.dynamicStateCount = 1;
      }
               const VkGraphicsPipelineCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
   .stageCount = 1,
   .pStages = &fs_info,
   .pDepthStencilState = &ds_info,
   .pDynamicState = &dyn_info,
               VkResult result = vk_meta_create_graphics_pipeline(device, meta, &info,
                                 }
      static VkResult
   get_blit_sampler(struct vk_device *device,
                     {
      struct {
      enum vk_meta_object_key_type key_type;
               memset(&key, 0, sizeof(key));
   key.key_type = VK_META_OBJECT_KEY_BLIT_SAMPLER;
            VkSampler from_cache = vk_meta_lookup_sampler(meta, &key, sizeof(key));
   if (from_cache != VK_NULL_HANDLE) {
      *sampler_out = from_cache;
               const VkSamplerCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
   .magFilter = filter,
   .minFilter = filter,
   .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
   .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
   .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
   .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
               return vk_meta_create_sampler(device, meta, &info,
      }
      static void
   do_blit(struct vk_command_buffer *cmd,
         struct vk_meta_device *meta,
   struct vk_image *src_image,
   VkFormat src_format,
   VkImageLayout src_image_layout,
   VkImageSubresourceLayers src_subres,
   struct vk_image *dst_image,
   VkFormat dst_format,
   VkImageLayout dst_image_layout,
   VkImageSubresourceLayers dst_subres,
   VkSampler sampler,
   struct vk_meta_blit_key *key,
   struct vk_meta_blit_push_data *push,
   const struct vk_meta_rect *dst_rect,
   {
      struct vk_device *device = cmd->base.device;
   const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            VkPipelineLayout pipeline_layout;
   result = get_blit_pipeline_layout(device, meta, &pipeline_layout);
   if (unlikely(result != VK_SUCCESS)) {
      vk_command_buffer_set_error(cmd, result);
               uint32_t desc_count = 0;
   VkDescriptorImageInfo image_infos[3];
            if (sampler != VK_NULL_HANDLE) {
      image_infos[desc_count] = (VkDescriptorImageInfo) {
         };
   desc_writes[desc_count] = (VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = BLIT_DESC_BINDING_SAMPLER,
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
   .descriptorCount = 1,
      };
               u_foreach_bit(a, src_subres.aspectMask) {
               VkImageView src_view;
   const VkImageViewUsageCreateInfo src_view_usage = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO,
      };
   const VkImageViewCreateInfo src_view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .pNext = &src_view_usage,
   .image = vk_image_to_handle(src_image),
   .viewType = vk_image_sampled_view_type(src_image),
   .format = src_format,
   .subresourceRange = {
      .aspectMask = aspect,
   .baseMipLevel = src_subres.mipLevel,
   .levelCount = 1,
   .baseArrayLayer = src_subres.baseArrayLayer,
         };
   result = vk_meta_create_image_view(cmd, meta, &src_view_info,
         if (unlikely(result != VK_SUCCESS)) {
      vk_command_buffer_set_error(cmd, result);
               assert(desc_count < ARRAY_SIZE(image_infos));
   assert(desc_count < ARRAY_SIZE(desc_writes));
   image_infos[desc_count] = (VkDescriptorImageInfo) {
         };
   desc_writes[desc_count] = (VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = aspect_to_tex_binding(aspect),
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   .descriptorCount = 1,
      };
               disp->CmdPushDescriptorSetKHR(vk_command_buffer_to_handle(cmd),
                        assert(dst_subres.aspectMask == src_subres.aspectMask);
            while (aspects_left) {
               /* If we need to write stencil via iterative discard, it has to be
   * written by itself because otherwise the discards would also throw
   * away color or depth data.
   */
   if ((key->aspects & VK_IMAGE_ASPECT_STENCIL_BIT) &&
      key->aspects != VK_IMAGE_ASPECT_STENCIL_BIT &&
               key->stencil_as_discard = key->aspects == VK_IMAGE_ASPECT_STENCIL_BIT &&
            VkImageView dst_view;
   const VkImageViewUsageCreateInfo dst_view_usage = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO,
   .usage = (key->aspects & VK_IMAGE_ASPECT_COLOR_BIT) ?
            };
   const VkImageViewCreateInfo dst_view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .pNext = &dst_view_usage,
   .image = vk_image_to_handle(dst_image),
   .viewType = vk_image_sampled_view_type(dst_image),
   .format = dst_format,
   .subresourceRange = {
      .aspectMask = dst_subres.aspectMask,
   .baseMipLevel = dst_subres.mipLevel,
   .levelCount = 1,
   .baseArrayLayer = dst_subres.baseArrayLayer,
         };
   result = vk_meta_create_image_view(cmd, meta, &dst_view_info,
         if (unlikely(result != VK_SUCCESS)) {
      vk_command_buffer_set_error(cmd, result);
               const VkRenderingAttachmentInfo vk_att = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = dst_view,
   .imageLayout = dst_image_layout,
   .loadOp = key->stencil_as_discard ? VK_ATTACHMENT_LOAD_OP_CLEAR :
            };
   VkRenderingInfo vk_render = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
   .renderArea = {
      .offset = {
      dst_rect->x0,
      },
   .extent = {
      dst_rect->x1 - dst_rect->x0,
         },
               if (key->aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
      vk_render.colorAttachmentCount = 1;
      }
   if (key->aspects & VK_IMAGE_ASPECT_DEPTH_BIT)
         if (key->aspects & VK_IMAGE_ASPECT_STENCIL_BIT)
                     VkPipeline pipeline;
   result = get_blit_pipeline(device, meta, key,
         if (unlikely(result != VK_SUCCESS)) {
      vk_command_buffer_set_error(cmd, result);
               disp->CmdBindPipeline(vk_command_buffer_to_handle(cmd),
            if (key->stencil_as_discard) {
      for (uint32_t i = 0; i < 8; i++) {
      push->stencil_bit = BITFIELD_BIT(i);
   disp->CmdPushConstants(vk_command_buffer_to_handle(cmd),
                        disp->CmdSetStencilWriteMask(vk_command_buffer_to_handle(cmd),
                        } else {
      disp->CmdPushConstants(vk_command_buffer_to_handle(cmd),
                                                   }
      void
   vk_meta_blit_image(struct vk_command_buffer *cmd,
                     struct vk_meta_device *meta,
   struct vk_image *src_image,
   VkFormat src_format,
   VkImageLayout src_image_layout,
   struct vk_image *dst_image,
   VkFormat dst_format,
   VkImageLayout dst_image_layout,
   {
      struct vk_device *device = cmd->base.device;
            VkSampler sampler;
   result = get_blit_sampler(device, meta, filter, &sampler);
   if (unlikely(result != VK_SUCCESS)) {
      vk_command_buffer_set_error(cmd, result);
               struct vk_meta_blit_key key;
   memset(&key, 0, sizeof(key));
   key.key_type = VK_META_OBJECT_KEY_BLIT_PIPELINE;
   key.src_samples = src_image->samples;
   key.dim = vk_image_sampler_dim(src_image);
            for (uint32_t r = 0; r < region_count; r++) {
      struct vk_meta_blit_push_data push = {0};
            uint32_t src_level = regions[r].srcSubresource.mipLevel;
            compute_off_scale(src_extent.width,
                     regions[r].srcOffsets[0].x,
   regions[r].srcOffsets[1].x,
   regions[r].dstOffsets[0].x,
   compute_off_scale(src_extent.height,
                     regions[r].srcOffsets[0].y,
   regions[r].srcOffsets[1].y,
            uint32_t dst_layer_count;
   if (src_image->image_type == VK_IMAGE_TYPE_3D) {
      uint32_t layer0, layer1;
   compute_off_scale(src_extent.depth,
                     regions[r].srcOffsets[0].z,
   regions[r].srcOffsets[1].z,
   regions[r].dstOffsets[0].z,
   dst_rect.layer = layer0;
      } else {
      assert(regions[r].srcSubresource.layerCount ==
         dst_layer_count = regions[r].dstSubresource.layerCount;
   push.arr_delta = regions[r].dstSubresource.baseArrayLayer -
               do_blit(cmd, meta,
         src_image, src_format, src_image_layout,
   regions[r].srcSubresource,
   dst_image, dst_format, dst_image_layout,
         }
      void
   vk_meta_blit_image2(struct vk_command_buffer *cmd,
               {
      VK_FROM_HANDLE(vk_image, src_image, blit->srcImage);
            vk_meta_blit_image(cmd, meta,
                  }
      void
   vk_meta_resolve_image(struct vk_command_buffer *cmd,
                        struct vk_meta_device *meta,
   struct vk_image *src_image,
   VkFormat src_format,
   VkImageLayout src_image_layout,
   struct vk_image *dst_image,
   VkFormat dst_format,
   VkImageLayout dst_image_layout,
      {
      struct vk_meta_blit_key key;
   memset(&key, 0, sizeof(key));
   key.key_type = VK_META_OBJECT_KEY_BLIT_PIPELINE;
   key.dim = vk_image_sampler_dim(src_image);
   key.src_samples = src_image->samples;
   key.resolve_mode = resolve_mode;
   key.stencil_resolve_mode = stencil_resolve_mode;
            for (uint32_t r = 0; r < region_count; r++) {
      struct vk_meta_blit_push_data push = {
      .x_off = regions[r].srcOffset.x - regions[r].dstOffset.x,
   .y_off = regions[r].srcOffset.y - regions[r].dstOffset.y,
   .x_scale = 1,
      };
   struct vk_meta_rect dst_rect = {
      .x0 = regions[r].dstOffset.x,
   .y0 = regions[r].dstOffset.y,
   .x1 = regions[r].dstOffset.x + regions[r].extent.width,
               do_blit(cmd, meta,
         src_image, src_format, src_image_layout,
   regions[r].srcSubresource,
   dst_image, dst_format, dst_image_layout,
   regions[r].dstSubresource,
         }
      void
   vk_meta_resolve_image2(struct vk_command_buffer *cmd,
               {
      VK_FROM_HANDLE(vk_image, src_image, resolve->srcImage);
            VkResolveModeFlagBits resolve_mode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
   if (vk_format_is_color(src_image->format) &&
      !vk_format_is_int(src_image->format))
         vk_meta_resolve_image(cmd, meta,
                        }
      static void
   vk_meta_resolve_attachment(struct vk_command_buffer *cmd,
                              struct vk_meta_device *meta,
   struct vk_image_view *src_view,
   VkImageLayout src_image_layout,
   struct vk_image_view *dst_view,
   VkImageLayout dst_image_layout,
      {
      VkImageResolve2 region = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2,
   .srcSubresource = {
      .aspectMask = resolve_aspects,
      },
   .srcOffset = { area.offset.x, area.offset.y, 0},
   .dstSubresource = {
      .aspectMask = resolve_aspects,
      },
   .dstOffset = { area.offset.x, area.offset.y, 0},
               if (view_mask) {
      u_foreach_bit(v, view_mask) {
      region.srcSubresource.baseArrayLayer = src_view->base_array_layer + v;
   region.srcSubresource.layerCount = 1;
                  vk_meta_resolve_image(cmd, meta,
                        src_view->image, src_view->format,
      } else {
      region.srcSubresource.baseArrayLayer = src_view->base_array_layer;
   region.srcSubresource.layerCount = layer_count;
   region.dstSubresource.baseArrayLayer = dst_view->base_array_layer;
            vk_meta_resolve_image(cmd, meta,
                        src_view->image, src_view->format,
      }
      void
   vk_meta_resolve_rendering(struct vk_command_buffer *cmd,
               {
      for (uint32_t c = 0; c < pRenderingInfo->colorAttachmentCount; c++) {
      const VkRenderingAttachmentInfo *att =
         if (att->resolveMode == VK_RESOLVE_MODE_NONE)
            VK_FROM_HANDLE(vk_image_view, view, att->imageView);
            vk_meta_resolve_attachment(cmd, meta, view, att->imageLayout,
                              res_view, att->resolveImageLayout,
            const VkRenderingAttachmentInfo *d_att = pRenderingInfo->pDepthAttachment;
   if (d_att && d_att->resolveMode == VK_RESOLVE_MODE_NONE)
            const VkRenderingAttachmentInfo *s_att = pRenderingInfo->pStencilAttachment;
   if (s_att && s_att->resolveMode == VK_RESOLVE_MODE_NONE)
            if (s_att != NULL || d_att != NULL) {
      if (s_att != NULL && d_att != NULL &&
      s_att->imageView == d_att->imageView &&
   s_att->resolveImageView == d_att->resolveImageView) {
                  vk_meta_resolve_attachment(cmd, meta, view, d_att->imageLayout,
                              res_view, d_att->resolveImageLayout,
   VK_IMAGE_ASPECT_DEPTH_BIT |
   } else {
      if (d_att != NULL) {
                     vk_meta_resolve_attachment(cmd, meta, view, d_att->imageLayout,
                                             if (s_att != NULL) {
                     vk_meta_resolve_attachment(cmd, meta, view, s_att->imageLayout,
                              res_view, s_att->resolveImageLayout,
            }
