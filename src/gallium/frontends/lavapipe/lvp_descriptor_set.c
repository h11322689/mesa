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
   #include "vk_descriptors.h"
   #include "vk_util.h"
   #include "util/u_math.h"
   #include "util/u_inlines.h"
      static bool
   binding_has_immutable_samplers(const VkDescriptorSetLayoutBinding *binding)
   {
      switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            default:
            }
      static void
   lvp_descriptor_set_layout_destroy(struct vk_device *_device, struct vk_descriptor_set_layout *_layout)
   {
      struct lvp_device *device = container_of(_device, struct lvp_device, vk);
            _layout->ref_cnt = UINT32_MAX;
               }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateDescriptorSetLayout(
      VkDevice                                    _device,
   const VkDescriptorSetLayoutCreateInfo*      pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
   uint32_t num_bindings = 0;
   uint32_t immutable_sampler_count = 0;
   for (uint32_t j = 0; j < pCreateInfo->bindingCount; j++) {
      num_bindings = MAX2(num_bindings, pCreateInfo->pBindings[j].binding + 1);
   /* From the Vulkan 1.1.97 spec for VkDescriptorSetLayoutBinding:
   *
   *    "If descriptorType specifies a VK_DESCRIPTOR_TYPE_SAMPLER or
   *    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER type descriptor, then
   *    pImmutableSamplers can be used to initialize a set of immutable
   *    samplers. [...]  If descriptorType is not one of these descriptor
   *    types, then pImmutableSamplers is ignored.
   *
   * We need to be careful here and only parse pImmutableSamplers if we
   * have one of the right descriptor types.
   */
   if (binding_has_immutable_samplers(&pCreateInfo->pBindings[j]))
               size_t size = sizeof(struct lvp_descriptor_set_layout) +
                  set_layout = vk_descriptor_set_layout_zalloc(&device->vk, size);
   if (!set_layout)
            set_layout->immutable_sampler_count = immutable_sampler_count;
   /* We just allocate all the samplers at the end of the struct */
   struct lvp_sampler **samplers =
            set_layout->binding_count = num_bindings;
   set_layout->shader_stages = 0;
            VkDescriptorSetLayoutBinding *bindings = NULL;
   VkResult result = vk_create_sorted_bindings(pCreateInfo->pBindings,
               if (result != VK_SUCCESS) {
      vk_descriptor_set_layout_unref(&device->vk, &set_layout->vk);
                        uint32_t dynamic_offset_count = 0;
   for (uint32_t j = 0; j < pCreateInfo->bindingCount; j++) {
      const VkDescriptorSetLayoutBinding *binding = bindings + j;
            uint32_t descriptor_count = binding->descriptorCount;
   if (binding->descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK)
            set_layout->binding[b].array_size = descriptor_count;
   set_layout->binding[b].descriptor_index = set_layout->size;
   set_layout->binding[b].type = binding->descriptorType;
   set_layout->binding[b].valid = true;
   set_layout->binding[b].uniform_block_offset = 0;
            if (binding->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
      binding->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
   set_layout->binding[b].dynamic_index = dynamic_offset_count;
               uint8_t max_plane_count = 1;
   if (binding_has_immutable_samplers(binding)) {
                     for (uint32_t i = 0; i < binding->descriptorCount; i++) {
      VK_FROM_HANDLE(lvp_sampler, sampler, binding->pImmutableSamplers[i]);
   set_layout->binding[b].immutable_samplers[i] = sampler;
   const uint8_t sampler_plane_count = sampler->vk.ycbcr_conversion ?
         if (max_plane_count < sampler_plane_count)
                  set_layout->binding[b].stride = max_plane_count;
            switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
         case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      set_layout->binding[b].uniform_block_offset = uniform_block_size;
   set_layout->binding[b].uniform_block_size = binding->descriptorCount;
   uniform_block_size += binding->descriptorCount;
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
         case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
         default:
                              for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++)
                              if (set_layout->binding_count == set_layout->immutable_sampler_count) {
      /* create a bindable set with all the immutable samplers */
   lvp_descriptor_set_create(device, set_layout, &set_layout->immutable_set);
   vk_descriptor_set_layout_unref(&device->vk, &set_layout->vk);
                           }
      struct lvp_pipeline_layout *
   lvp_pipeline_layout_create(struct lvp_device *device,
               {
      struct lvp_pipeline_layout *layout = vk_pipeline_layout_zalloc(&device->vk, sizeof(*layout),
            layout->push_constant_size = 0;
   for (unsigned i = 0; i < pCreateInfo->pushConstantRangeCount; ++i) {
      const VkPushConstantRange *range = pCreateInfo->pPushConstantRanges + i;
   layout->push_constant_size = MAX2(layout->push_constant_size,
            }
   layout->push_constant_size = align(layout->push_constant_size, 16);
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreatePipelineLayout(
      VkDevice                                    _device,
   const VkPipelineLayoutCreateInfo*           pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   struct lvp_pipeline_layout *layout = lvp_pipeline_layout_create(device, pCreateInfo, pAllocator);
               }
      static struct pipe_resource *
   get_buffer_resource(struct pipe_context *ctx, const VkDescriptorAddressInfoEXT *bda)
   {
      struct pipe_screen *pscreen = ctx->screen;
            templ.screen = pscreen;
   templ.target = PIPE_BUFFER;
   templ.format = PIPE_FORMAT_R8_UNORM;
   templ.width0 = bda->range;
   templ.height0 = 1;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.bind |= PIPE_BIND_SAMPLER_VIEW;
   templ.bind |= PIPE_BIND_SHADER_IMAGE;
            uint64_t size;
   struct pipe_resource *pres = pscreen->resource_create_unbacked(pscreen, &templ, &size);
   assert(size == bda->range);
   pscreen->resource_bind_backing(pscreen, pres, (void *)(uintptr_t)bda->address, 0);
      }
      static struct lp_texture_handle
   get_texture_handle_bda(struct lvp_device *device, const VkDescriptorAddressInfoEXT *bda, enum pipe_format format)
   {
                        struct pipe_sampler_view templ;
   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_BUFFER;
   templ.swizzle_r = PIPE_SWIZZLE_X;
   templ.swizzle_g = PIPE_SWIZZLE_Y;
   templ.swizzle_b = PIPE_SWIZZLE_Z;
   templ.swizzle_a = PIPE_SWIZZLE_W;
   templ.format = format;
   templ.u.buf.size = bda->range;
   templ.texture = pres;
   templ.context = ctx;
                     struct lp_texture_handle *handle = (void *)(uintptr_t)ctx->create_texture_handle(ctx, view, NULL);
                     ctx->sampler_view_destroy(ctx, view);
               }
      static struct lp_texture_handle
   get_image_handle_bda(struct lvp_device *device, const VkDescriptorAddressInfoEXT *bda, enum pipe_format format)
   {
               struct pipe_resource *pres = get_buffer_resource(ctx, bda);
   struct pipe_image_view view = {0};
   view.resource = pres;
   view.format = format;
                     struct lp_texture_handle *handle = (void *)(uintptr_t)ctx->create_image_handle(ctx, &view);
                                 }
      VkResult
   lvp_descriptor_set_create(struct lvp_device *device,
               {
      struct lvp_descriptor_set *set = vk_zalloc(&device->vk.alloc /* XXX: Use the pool */,
         if (!set)
            vk_object_base_init(&device->vk, &set->base,
         set->layout = layout;
                     for (unsigned i = 0; i < layout->binding_count; i++)
            struct pipe_resource template = {
      .bind = PIPE_BIND_CONSTANT_BUFFER,
   .screen = device->pscreen,
   .target = PIPE_BUFFER,
   .format = PIPE_FORMAT_R8_UNORM,
   .width0 = bo_size,
   .height0 = 1,
   .depth0 = 1,
   .array_size = 1,
               set->bo = device->pscreen->resource_create_unbacked(device->pscreen, &template, &bo_size);
            set->map = device->pscreen->map_memory(device->pscreen, set->pmem);
                     for (uint32_t binding_index = 0; binding_index < layout->binding_count; binding_index++) {
      const struct lvp_descriptor_set_binding_layout *bind_layout = &set->layout->binding[binding_index];
   if (!bind_layout->immutable_samplers)
            struct lp_descriptor *desc = set->map;
            for (uint32_t sampler_index = 0; sampler_index < bind_layout->array_size; sampler_index++) {
      if (bind_layout->immutable_samplers[sampler_index]) {
      for (uint32_t s = 0; s < bind_layout->stride; s++)  {
      int idx = sampler_index * bind_layout->stride + s;
                                    }
      void
   lvp_descriptor_set_destroy(struct lvp_device *device,
         {
      pipe_resource_reference(&set->bo, NULL);
   device->pscreen->unmap_memory(device->pscreen, set->pmem);
            vk_descriptor_set_layout_unref(&device->vk, &set->layout->vk);
   vk_object_base_finish(&set->base);
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_AllocateDescriptorSets(
      VkDevice                                    _device,
   const VkDescriptorSetAllocateInfo*          pAllocateInfo,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   LVP_FROM_HANDLE(lvp_descriptor_pool, pool, pAllocateInfo->descriptorPool);
   VkResult result = VK_SUCCESS;
   struct lvp_descriptor_set *set;
            for (i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
      LVP_FROM_HANDLE(lvp_descriptor_set_layout, layout,
            result = lvp_descriptor_set_create(device, layout, &set);
   if (result != VK_SUCCESS)
            list_addtail(&set->link, &pool->sets);
               if (result != VK_SUCCESS)
      lvp_FreeDescriptorSets(_device, pAllocateInfo->descriptorPool,
            }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_FreeDescriptorSets(
      VkDevice                                    _device,
   VkDescriptorPool                            descriptorPool,
   uint32_t                                    count,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   for (uint32_t i = 0; i < count; i++) {
               if (!set)
         list_del(&set->link);
      }
      }
      VKAPI_ATTR void VKAPI_CALL lvp_UpdateDescriptorSets(
      VkDevice                                    _device,
   uint32_t                                    descriptorWriteCount,
   const VkWriteDescriptorSet*                 pDescriptorWrites,
   uint32_t                                    descriptorCopyCount,
      {
               for (uint32_t i = 0; i < descriptorWriteCount; i++) {
      const VkWriteDescriptorSet *write = &pDescriptorWrites[i];
   LVP_FROM_HANDLE(lvp_descriptor_set, set, write->dstSet);
   const struct lvp_descriptor_set_binding_layout *bind_layout =
            if (write->descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      const VkWriteDescriptorSetInlineUniformBlock *uniform_data =
         assert(uniform_data);
   memcpy((uint8_t *)set->map + bind_layout->uniform_block_offset + write->dstArrayElement, uniform_data->pData, uniform_data->dataSize);
               struct lp_descriptor *desc = set->map;
            switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      if (!bind_layout->immutable_samplers) {
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
                     for (unsigned k = 0; k < bind_layout->stride; k++) {
      desc[didx + k].sampler = sampler->desc.sampler;
                        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      LVP_FROM_HANDLE(lvp_image_view, iview,
         uint32_t didx = j * bind_layout->stride;
                     for (unsigned p = 0; p < plane_count; p++) {
                                                for (unsigned p = 0; p < plane_count; p++) {
      desc[didx + p].sampler = sampler->desc.sampler;
            } else {
      for (unsigned k = 0; k < bind_layout->stride; k++) {
      desc[didx + k].functions = device->null_texture_handle->functions;
                        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      LVP_FROM_HANDLE(lvp_image_view, iview,
         uint32_t didx = j * bind_layout->stride;
                     for (unsigned p = 0; p < plane_count; p++) {
      lp_jit_texture_from_pipe(&desc[didx + p].texture, iview->planes[p].sv);
         } else {
      for (unsigned k = 0; k < bind_layout->stride; k++) {
      desc[didx + k].functions = device->null_texture_handle->functions;
            }
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      LVP_FROM_HANDLE(lvp_image_view, iview,
         uint32_t didx = j * bind_layout->stride;
                     for (unsigned p = 0; p < plane_count; p++) {
      lp_jit_image_from_pipe(&desc[didx + p].image, &iview->planes[p].iv);
         } else {
      for (unsigned k = 0; k < bind_layout->stride; k++)
                     case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      LVP_FROM_HANDLE(lvp_buffer_view, bview,
         assert(bind_layout->stride == 1);
   if (bview) {
      lp_jit_texture_from_pipe(&desc[j].texture, bview->sv);
      } else {
      desc[j].functions = device->null_texture_handle->functions;
                     case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      LVP_FROM_HANDLE(lvp_buffer_view, bview,
         assert(bind_layout->stride == 1);
   if (bview) {
      lp_jit_image_from_pipe(&desc[j].image, &bview->iv);
      } else {
                        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      LVP_FROM_HANDLE(lvp_buffer, buffer, write->pBufferInfo[j].buffer);
   assert(bind_layout->stride == 1);
   if (buffer) {
      struct pipe_constant_buffer ubo = {
      .buffer = buffer->bo,
                                       } else {
                        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      LVP_FROM_HANDLE(lvp_buffer, buffer, write->pBufferInfo[j].buffer);
   assert(bind_layout->stride == 1);
   if (buffer) {
      struct pipe_shader_buffer ubo = {
      .buffer = buffer->bo,
                                       } else {
                        default:
                     for (uint32_t i = 0; i < descriptorCopyCount; i++) {
      const VkCopyDescriptorSet *copy = &pDescriptorCopies[i];
   LVP_FROM_HANDLE(lvp_descriptor_set, src, copy->srcSet);
            const struct lvp_descriptor_set_binding_layout *src_layout =
         struct lp_descriptor *src_desc = src->map;
            const struct lvp_descriptor_set_binding_layout *dst_layout =
         struct lp_descriptor *dst_desc = dst->map;
            if (src_layout->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      memcpy((uint8_t *)dst->map + dst_layout->uniform_block_offset + copy->dstArrayElement,
            } else {
                     for (uint32_t j = 0; j < copy->descriptorCount; j++)
            }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateDescriptorPool(
      VkDevice                                    _device,
   const VkDescriptorPoolCreateInfo*           pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   struct lvp_descriptor_pool *pool;
   size_t size = sizeof(struct lvp_descriptor_pool);
   pool = vk_zalloc2(&device->vk.alloc, pAllocator, size, 8,
         if (!pool)
            vk_object_base_init(&device->vk, &pool->base,
         pool->flags = pCreateInfo->flags;
   list_inithead(&pool->sets);
   *pDescriptorPool = lvp_descriptor_pool_to_handle(pool);
      }
      static void lvp_reset_descriptor_pool(struct lvp_device *device,
         {
      struct lvp_descriptor_set *set, *tmp;
   LIST_FOR_EACH_ENTRY_SAFE(set, tmp, &pool->sets, link) {
      list_del(&set->link);
         }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroyDescriptorPool(
      VkDevice                                    _device,
   VkDescriptorPool                            _pool,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            if (!_pool)
            lvp_reset_descriptor_pool(device, pool);
   vk_object_base_finish(&pool->base);
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_ResetDescriptorPool(
      VkDevice                                    _device,
   VkDescriptorPool                            _pool,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            lvp_reset_descriptor_pool(device, pool);
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetDescriptorSetLayoutSupport(VkDevice device,
               {
      const VkDescriptorSetLayoutBindingFlagsCreateInfo *variable_flags =
         VkDescriptorSetVariableDescriptorCountLayoutSupport *variable_count =
         if (variable_count) {
      variable_count->maxVariableDescriptorCount = 0;
   if (variable_flags) {
      for (unsigned i = 0; i < variable_flags->bindingCount; i++) {
      if (variable_flags->pBindingFlags[i] & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT)
            }
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateDescriptorUpdateTemplate(VkDevice _device,
                     {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   const uint32_t entry_count = pCreateInfo->descriptorUpdateEntryCount;
   const size_t size = sizeof(struct lvp_descriptor_update_template) +
                     templ = vk_alloc(&device->vk.alloc, size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!templ)
            vk_object_base_init(&device->vk, &templ->base,
            templ->ref_cnt = 1;
   templ->type = pCreateInfo->templateType;
   templ->bind_point = pCreateInfo->pipelineBindPoint;
   templ->set = pCreateInfo->set;
   /* This parameter is ignored if templateType is not VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR */
   if (pCreateInfo->templateType == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR)
         else
                  VkDescriptorUpdateTemplateEntry *entries = (VkDescriptorUpdateTemplateEntry *)(templ + 1);
   for (unsigned i = 0; i < entry_count; i++) {
                  *pDescriptorUpdateTemplate = lvp_descriptor_update_template_to_handle(templ);
      }
      void
   lvp_descriptor_template_destroy(struct lvp_device *device, struct lvp_descriptor_update_template *templ)
   {
      if (!templ)
            vk_object_base_finish(&templ->base);
      }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroyDescriptorUpdateTemplate(VkDevice _device,
               {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   LVP_FROM_HANDLE(lvp_descriptor_update_template, templ, descriptorUpdateTemplate);
      }
      uint32_t
   lvp_descriptor_update_template_entry_size(VkDescriptorType type)
   {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
         case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
   default:
            }
      void
   lvp_descriptor_set_update_with_template(VkDevice _device, VkDescriptorSet descriptorSet,
               {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   LVP_FROM_HANDLE(lvp_descriptor_set, set, descriptorSet);
   LVP_FROM_HANDLE(lvp_descriptor_update_template, templ, descriptorUpdateTemplate);
                     for (i = 0; i < templ->entry_count; ++i) {
               if (!push)
            const struct lvp_descriptor_set_binding_layout *bind_layout =
            if (entry->descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      memcpy((uint8_t *)set->map + bind_layout->uniform_block_offset + entry->dstArrayElement, pSrc, entry->descriptorCount);
               struct lp_descriptor *desc = set->map;
            for (j = 0; j < entry->descriptorCount; ++j) {
               idx *= bind_layout->stride;
   switch (entry->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER: {
                     for (unsigned k = 0; k < bind_layout->stride; k++) {
      desc[idx + k].sampler = sampler->desc.sampler;
      }
      }
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                     if (iview) {
      for (unsigned p = 0; p < iview->plane_count; p++) {
                                          for (unsigned p = 0; p < iview->plane_count; p++) {
      desc[idx + p].sampler = sampler->desc.sampler;
            } else {
      for (unsigned k = 0; k < bind_layout->stride; k++) {
      desc[idx + k].functions = device->null_texture_handle->functions;
         }
      }
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
                     if (iview) {
      for (unsigned p = 0; p < iview->plane_count; p++) {
      lp_jit_texture_from_pipe(&desc[idx + p].texture, iview->planes[p].sv);
         } else {
      for (unsigned k = 0; k < bind_layout->stride; k++) {
      desc[idx + k].functions = device->null_texture_handle->functions;
         }
      }
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
                     if (iview) {
      for (unsigned p = 0; p < iview->plane_count; p++) {
      lp_jit_image_from_pipe(&desc[idx + p].image, &iview->planes[p].iv);
         } else {
      for (unsigned k = 0; k < bind_layout->stride; k++)
      }
      }
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: {
      LVP_FROM_HANDLE(lvp_buffer_view, bview,
         assert(bind_layout->stride == 1);
   if (bview) {
      lp_jit_texture_from_pipe(&desc[idx].texture, bview->sv);
      } else {
      desc[j].functions = device->null_texture_handle->functions;
      }
      }
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
      LVP_FROM_HANDLE(lvp_buffer_view, bview,
         assert(bind_layout->stride == 1);
   if (bview) {
      lp_jit_image_from_pipe(&desc[idx].image, &bview->iv);
      } else {
         }
               case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: {
      VkDescriptorBufferInfo *info = (VkDescriptorBufferInfo *)pSrc;
   LVP_FROM_HANDLE(lvp_buffer, buffer, info->buffer);
   assert(bind_layout->stride == 1);
   if (buffer) {
      struct pipe_constant_buffer ubo = {
      .buffer = buffer->bo,
                                       } else {
         }
               case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
      VkDescriptorBufferInfo *info = (VkDescriptorBufferInfo *)pSrc;
                  if (buffer) {
      struct pipe_shader_buffer ubo = {
      .buffer = buffer->bo,
                                       } else {
         }
      }
   default:
                  if (push)
         else
            }
      VKAPI_ATTR void VKAPI_CALL
   lvp_UpdateDescriptorSetWithTemplate(VkDevice device, VkDescriptorSet descriptorSet,
               {
         }
      VKAPI_ATTR void VKAPI_CALL lvp_GetDescriptorSetLayoutSizeEXT(
      VkDevice                                    _device,
   VkDescriptorSetLayout                       _layout,
      {
                        for (unsigned i = 0; i < layout->binding_count; i++)
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetDescriptorSetLayoutBindingOffsetEXT(
      VkDevice                                    _device,
   VkDescriptorSetLayout                       _layout,
   uint32_t                                    binding,
      {
      LVP_FROM_HANDLE(lvp_descriptor_set_layout, layout, _layout);
            const struct lvp_descriptor_set_binding_layout *bind_layout = &layout->binding[binding];
   if (bind_layout->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK)
         else
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetDescriptorEXT(
      VkDevice                                        _device,
   const VkDescriptorGetInfoEXT*                   pCreateInfo,
   size_t                                          size,
      {
                        struct pipe_sampler_state sampler = {
      .seamless_cube_map = 1,
               switch (pCreateInfo->type) {
   case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK: {
      unreachable("this is a spec violation");
      }
   case VK_DESCRIPTOR_TYPE_SAMPLER: {
      if (pCreateInfo->data.pSampler) {
      LVP_FROM_HANDLE(lvp_sampler, sampler, pCreateInfo->data.pSampler[0]);
   desc->sampler = sampler->desc.sampler;
      } else {
      lp_jit_sampler_from_pipe(&desc->sampler, &sampler);
      }
               case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
      const VkDescriptorImageInfo *info = pCreateInfo->data.pCombinedImageSampler;
   if (info && info->imageView) {
                              if (info->sampler) {
      LVP_FROM_HANDLE(lvp_sampler, sampler, info->sampler);
   desc->sampler = sampler->desc.sampler;
      } else {
      lp_jit_sampler_from_pipe(&desc->sampler, &sampler);
         } else {
      desc->functions = device->null_texture_handle->functions;
                           case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
      if (pCreateInfo->data.pSampledImage && pCreateInfo->data.pSampledImage->imageView) {
      LVP_FROM_HANDLE(lvp_image_view, iview, pCreateInfo->data.pSampledImage->imageView);
   lp_jit_texture_from_pipe(&desc->texture, iview->planes[0].sv);
      } else {
      desc->functions = device->null_texture_handle->functions;
      }
               /* technically these use different pointers, but it's a union, so they're all the same */
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
      if (pCreateInfo->data.pStorageImage && pCreateInfo->data.pStorageImage->imageView) {
      LVP_FROM_HANDLE(lvp_image_view, iview, pCreateInfo->data.pStorageImage->imageView);
   lp_jit_image_from_pipe(&desc->image, &iview->planes[0].iv);
      } else {
         }
      }
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: {
      const VkDescriptorAddressInfoEXT *bda = pCreateInfo->data.pUniformTexelBuffer;
   if (bda && bda->address) {
      enum pipe_format pformat = vk_format_to_pipe_format(bda->format);
   lp_jit_texture_buffer_from_bda(&desc->texture, (void*)(uintptr_t)bda->address, bda->range, pformat);
      } else {
      desc->functions = device->null_texture_handle->functions;
      }
      }
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
      const VkDescriptorAddressInfoEXT *bda = pCreateInfo->data.pStorageTexelBuffer;
   if (bda && bda->address) {
      enum pipe_format pformat = vk_format_to_pipe_format(bda->format);
   lp_jit_image_buffer_from_bda(&desc->image, (void *)(uintptr_t)bda->address, bda->range, pformat);
      } else {
         }
      }
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
      const VkDescriptorAddressInfoEXT *bda = pCreateInfo->data.pUniformBuffer;
   if (bda && bda->address) {
      struct pipe_constant_buffer ubo = {
      .user_buffer = (void *)(uintptr_t)bda->address,
                  } else {
         }
      }
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
      const VkDescriptorAddressInfoEXT *bda = pCreateInfo->data.pStorageBuffer;
   if (bda && bda->address) {
         } else {
         }
      }
   default:
            }
