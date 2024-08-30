   /* Copyright (c) 2017-2023 Hans-Kristian Arntzen
   *
   * Permission is hereby granted, free of charge, to any person obtaining
   * a copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sublicense, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "vk_texcompress_astc.h"
   #include "util/texcompress_astc_luts_wrap.h"
   #include "vk_alloc.h"
   #include "vk_buffer.h"
   #include "vk_device.h"
   #include "vk_format.h"
   #include "vk_image.h"
   #include "vk_physical_device.h"
      /* type_indexes_mask bits are set/clear for support memory type index as per
   * struct VkPhysicalDeviceMemoryProperties.memoryTypes[] */
   static uint32_t
   get_mem_type_index(struct vk_device *device, uint32_t type_indexes_mask,
         {
      const struct vk_physical_device_dispatch_table *disp = &device->physical->dispatch_table;
            VkPhysicalDeviceMemoryProperties2 props2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
      };
            for (uint32_t i = 0; i < props2.memoryProperties.memoryTypeCount; i++) {
      if ((type_indexes_mask & (1 << i)) &&
      ((props2.memoryProperties.memoryTypes[i].propertyFlags & mem_property) == mem_property)) {
                     }
      static VkResult
   vk_create_buffer(struct vk_device *device, VkAllocationCallbacks *allocator,
                     {
      VkResult result;
   VkDevice _device = vk_device_to_handle(device);
            VkBufferCreateInfo buffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
   .size = size,
   .usage = usage_flags,
      };
   result =
         if (unlikely(result != VK_SUCCESS))
            VkBufferMemoryRequirementsInfo2 mem_req_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
      };
   VkMemoryRequirements2 mem_req = {
         };
            uint32_t mem_type_index = get_mem_type_index(
         if (mem_type_index == -1)
            VkMemoryAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   .allocationSize = mem_req.memoryRequirements.size,
      };
   result = disp->AllocateMemory(_device, &alloc_info, allocator, vk_mem);
   if (unlikely(result != VK_SUCCESS))
                        }
      static VkResult
   create_buffer_view(struct vk_device *device, VkAllocationCallbacks *allocator,
               {
      VkResult result;
   VkDevice _device = vk_device_to_handle(device);
            VkBufferViewCreateInfo buffer_view_create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
   .buffer = buf,
   .format = format,
   .offset = offset,
      };
   result = disp->CreateBufferView(_device, &buffer_view_create_info,
            }
      static uint8_t
   get_partition_table_index(VkFormat format)
   {
      switch (format) {
   case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
   case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
         case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
   case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
         case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
   case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
         case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
   case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
         case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
   case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
         case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
   case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
         case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
   case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
         case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
   case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
         case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
   case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
         case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
   case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
         case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
   case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
         case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
   case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
         case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
   case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
         case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
   case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
         default:
      unreachable("bad astc format\n");
         }
      static VkResult
   astc_prepare_buffer(struct vk_device *device,
                     struct vk_texcompress_astc_state *astc,
   VkAllocationCallbacks *allocator,
   {
      VkResult result;
   astc_decoder_lut_holder astc_lut_holder;
                     const astc_decoder_lut *luts[] = {
      &astc_lut_holder.color_endpoint,
   &astc_lut_holder.color_endpoint_unquant,
   &astc_lut_holder.weights,
   &astc_lut_holder.weights_unquant,
               for (unsigned i = 0; i < ARRAY_SIZE(luts); i++) {
      offset = align(offset, minTexelBufferOffsetAlignment);
   if (single_buf_ptr) {
      memcpy(single_buf_ptr + offset, luts[i]->data, luts[i]->size_B);
   result = create_buffer_view(device, allocator, &astc->luts_buf_view[i], astc->luts_buf,
               if (result != VK_SUCCESS)
      }
               const VkFormat formats[] = {
      VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
   VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
   VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
   VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
   VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
   VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
   VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
   VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
   VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
   VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
   VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
   VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
   VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
               for (uint32_t i = 0; i < ARRAY_SIZE(formats); i++) {
      unsigned lut_width;
   unsigned lut_height;
   const void *lut_data = _mesa_get_astc_decoder_partition_table(
         vk_format_get_blockwidth(formats[i]),
   vk_format_get_blockheight(formats[i]),
            offset = align(offset, minTexelBufferOffsetAlignment);
   if (single_buf_ptr) {
               result = create_buffer_view(device, allocator, &astc->partition_tbl_buf_view[i],
               if (result != VK_SUCCESS)
      }
               *single_buf_size = offset;
      }
      static VkResult
   create_fill_all_luts_vulkan(struct vk_device *device,
               {
      VkResult result;
   VkDevice _device = vk_device_to_handle(device);
   const struct vk_device_dispatch_table *disp = &device->dispatch_table;
   VkPhysicalDevice _phy_device = vk_physical_device_to_handle(device->physical);
   const struct vk_physical_device_dispatch_table *phy_disp = &device->physical->dispatch_table;
   VkDeviceSize single_buf_size;
            VkPhysicalDeviceProperties2 phy_dev_prop = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      };
            /* get the single_buf_size */
   result = astc_prepare_buffer(device, astc, allocator,
                  /* create gpu buffer for all the luts */
   result = vk_create_buffer(device, allocator, single_buf_size,
                           if (unlikely(result != VK_SUCCESS))
                     /* fill all the luts and create views */
   result = astc_prepare_buffer(device, astc, allocator,
                  disp->UnmapMemory(_device, astc->luts_mem);
      }
      static VkResult
   create_layout(struct vk_device *device, VkAllocationCallbacks *allocator,
         {
      VkResult result;
   VkDevice _device = vk_device_to_handle(device);
            VkDescriptorSetLayoutBinding bindings[] = {
      {
      .binding = 0, /* OutputImage2DArray */
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      },
   {
      .binding = 1, /* PayloadInput2DArray */
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      },
   {
      .binding = 2, /* LUTRemainingBitsToEndpointQuantizer */
   .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      },
   {
      .binding = 3, /* LUTEndpointUnquantize */
   .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      },
   {
      .binding = 4, /* LUTWeightQuantizer */
   .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      },
   {
      .binding = 5, /* LUTWeightUnquantize */
   .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      },
   {
      .binding = 6, /* LUTTritQuintDecode */
   .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      },
   {
      .binding = 7, /* LUTPartitionTable */
   .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
   .descriptorCount = 1,
   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                  VkDescriptorSetLayoutCreateInfo ds_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
   .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
   .bindingCount = ARRAY_SIZE(bindings),
               result = disp->CreateDescriptorSetLayout(_device, &ds_create_info,
         if (result != VK_SUCCESS)
            VkPipelineLayoutCreateInfo pl_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &astc->ds_layout,
   .pushConstantRangeCount = 1,
      };
   result = disp->CreatePipelineLayout(_device, &pl_create_info, allocator,
      fail:
         }
      static const uint32_t astc_spv[] = {
   #include "astc_spv.h"
   };
      static VkResult
   vk_astc_create_shader_module(struct vk_device *device,
               {
      VkDevice _device = vk_device_to_handle(device);
            VkShaderModuleCreateInfo shader_module_create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
   .pNext = NULL,
   .flags = 0,
   .codeSize = sizeof(astc_spv),
               return disp->CreateShaderModule(_device, &shader_module_create_info,
      }
      static VkResult
   create_astc_decode_pipeline(struct vk_device *device,
                     {
      VkResult result;
   VkDevice _device = vk_device_to_handle(device);
   const struct vk_device_dispatch_table *disp = &device->dispatch_table;
   VkPipeline pipeline;
                     uint32_t special_data[3] = {
      vk_format_get_blockwidth(format),
   vk_format_get_blockheight(format),
      };
   VkSpecializationMapEntry special_map_entry[3] = {{
                                                      .constantID = 0,
   .offset = 0,
      },
   {
            VkSpecializationInfo specialization_info = {
      .mapEntryCount = 3,
   .pMapEntries = special_map_entry,
   .dataSize = 12,
               /* compute shader */
   VkPipelineShaderStageCreateInfo pipeline_shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = astc->shader_module,
   .pName = "main",
               VkComputePipelineCreateInfo vk_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = pipeline_shader_stage,
   .flags = 0,
               result = disp->CreateComputePipelines(
         if (result != VK_SUCCESS)
            astc->pipeline[t_i] = pipeline;
               }
      VkPipeline
   vk_texcompress_astc_get_decode_pipeline(struct vk_device *device, VkAllocationCallbacks *allocator,
               {
      VkResult result;
                     if (astc->pipeline[t_i])
            if (!astc->shader_module) {
      result = vk_astc_create_shader_module(device, allocator, astc);
   if (result != VK_SUCCESS)
                     unlock:
      simple_mtx_unlock(&astc->mutex);
      }
      static inline void
   fill_desc_image_info_struct(VkDescriptorImageInfo *info, VkImageView img_view,
         {
      info->sampler = VK_NULL_HANDLE;
   info->imageView = img_view;
      }
      static inline void
   fill_write_descriptor_set_image(VkWriteDescriptorSet *set, uint8_t bind_i,
         {
      set->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   set->pNext = NULL;
   set->dstSet = VK_NULL_HANDLE;
   set->dstBinding = bind_i;
   set->dstArrayElement = 0;
   set->descriptorCount = 1;
   set->descriptorType = desc_type;
   set->pImageInfo = image_info;
   set->pBufferInfo = NULL;
      }
      static inline void
   fill_write_descriptor_set_uniform_texel(VkWriteDescriptorSet *set,
               {
      set->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   set->pNext = NULL;
   set->dstSet = VK_NULL_HANDLE;
   set->dstBinding = bind_i;
   set->dstArrayElement = 0;
   set->descriptorCount = 1;
   set->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
   set->pImageInfo = NULL;
   set->pBufferInfo = NULL;
      }
      void
   vk_texcompress_astc_fill_write_descriptor_sets(struct vk_texcompress_astc_state *astc,
                           {
               desc_i = 0;
   fill_desc_image_info_struct(&set->dst_desc_image_info, dst_img_view, VK_IMAGE_LAYOUT_GENERAL);
   fill_write_descriptor_set_image(&set->descriptor_set[desc_i], desc_i,
         desc_i++;
   fill_desc_image_info_struct(&set->src_desc_image_info, src_img_view, src_img_layout);
   fill_write_descriptor_set_image(&set->descriptor_set[desc_i], desc_i,
         /* fill luts descriptor */
   desc_i++;
   for (unsigned i = 0; i < VK_TEXCOMPRESS_ASTC_NUM_LUTS; i++) {
      fill_write_descriptor_set_uniform_texel(&set->descriptor_set[desc_i + i], desc_i + i,
      }
   desc_i += VK_TEXCOMPRESS_ASTC_NUM_LUTS;
   uint8_t t_i = get_partition_table_index(format);
   fill_write_descriptor_set_uniform_texel(&set->descriptor_set[desc_i], desc_i,
         desc_i++;
      }
      VkResult
   vk_texcompress_astc_init(struct vk_device *device, VkAllocationCallbacks *allocator,
               {
               /* astc memory to be freed as part of vk_astc_decode_finish() */
   *astc = vk_zalloc(allocator, sizeof(struct vk_texcompress_astc_state), 8,
         if (*astc == NULL)
                     result = create_fill_all_luts_vulkan(device, allocator, *astc);
   if (result != VK_SUCCESS)
                  fail:
         }
      void
   vk_texcompress_astc_finish(struct vk_device *device,
               {
      VkDevice _device = vk_device_to_handle(device);
            while (astc->pipeline_mask) {
      uint8_t t_i = u_bit_scan(&astc->pipeline_mask);
               disp->DestroyPipelineLayout(_device, astc->p_layout, allocator);
   disp->DestroyShaderModule(_device, astc->shader_module, allocator);
            for (unsigned i = 0; i < VK_TEXCOMPRESS_ASTC_NUM_LUTS; i++)
            for (unsigned i = 0; i < VK_TEXCOMPRESS_ASTC_NUM_PARTITION_TABLES; i++)
            disp->DestroyBuffer(_device, astc->luts_buf, allocator);
               }
