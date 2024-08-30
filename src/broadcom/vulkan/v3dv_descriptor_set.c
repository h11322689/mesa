   /*
   * Copyright © 2019 Raspberry Pi Ltd
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
      #include "vk_descriptors.h"
   #include "vk_util.h"
      #include "v3dv_private.h"
      /*
   * For a given descriptor defined by the descriptor_set it belongs, its
   * binding layout, array_index, and plane, it returns the map region assigned
   * to it from the descriptor pool bo.
   */
   static void *
   descriptor_bo_map(struct v3dv_device *device,
                     {
      /* Inline uniform blocks use BO memory to store UBO contents, not
   * descriptor data, so their descriptor BO size is 0 even though they
   * do use BO memory.
   */
   uint32_t bo_size = v3dv_X(device, descriptor_bo_size)(binding_layout->type);
   assert(bo_size > 0 ||
            return set->pool->bo->map +
      set->base_offset + binding_layout->descriptor_offset +
   }
      static bool
   descriptor_type_is_dynamic(VkDescriptorType type)
   {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      return true;
      default:
            }
      /*
   * Tries to get a real descriptor using a descriptor map index from the
   * descriptor_state + pipeline_layout.
   */
   struct v3dv_descriptor *
   v3dv_descriptor_map_get_descriptor(struct v3dv_descriptor_state *descriptor_state,
                           {
               uint32_t set_number = map->set[index];
            struct v3dv_descriptor_set *set =
                  uint32_t binding_number = map->binding[index];
            const struct v3dv_descriptor_set_binding_layout *binding_layout =
            uint32_t array_index = map->array_index[index];
            if (descriptor_type_is_dynamic(binding_layout->type)) {
      uint32_t dynamic_offset_index =
                                 }
      /* Equivalent to map_get_descriptor but it returns a reloc with the bo
   * associated with that descriptor (suballocation of the descriptor pool bo)
   *
   * It also returns the descriptor type, so the caller could do extra
   * validation or adding extra offsets if the bo contains more that one field.
   */
   struct v3dv_cl_reloc
   v3dv_descriptor_map_get_descriptor_bo(struct v3dv_device *device,
                                 {
               uint32_t set_number = map->set[index];
            struct v3dv_descriptor_set *set =
                  uint32_t binding_number = map->binding[index];
            const struct v3dv_descriptor_set_binding_layout *binding_layout =
                        assert(binding_layout->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK ||
         if (out_type)
            uint32_t array_index = map->array_index[index];
            struct v3dv_cl_reloc reloc = {
      .bo = set->pool->bo,
   .offset = set->base_offset + binding_layout->descriptor_offset +
                  }
      /*
   * The difference between this method and v3dv_descriptor_map_get_descriptor,
   * is that if the sampler are added as immutable when creating the set layout,
   * they are bound to the set layout, so not part of the descriptor per
   * se. This method return early in that case.
   */
   const struct v3dv_sampler *
   v3dv_descriptor_map_get_sampler(struct v3dv_descriptor_state *descriptor_state,
                     {
               uint32_t set_number = map->set[index];
            struct v3dv_descriptor_set *set =
                  uint32_t binding_number = map->binding[index];
            const struct v3dv_descriptor_set_binding_layout *binding_layout =
            uint32_t array_index = map->array_index[index];
            if (binding_layout->immutable_samplers_offset != 0) {
      assert(binding_layout->type == VK_DESCRIPTOR_TYPE_SAMPLER ||
            const struct v3dv_sampler *immutable_samplers =
            assert(immutable_samplers);
   const struct v3dv_sampler *sampler = &immutable_samplers[array_index];
                        struct v3dv_descriptor *descriptor =
            assert(descriptor->type == VK_DESCRIPTOR_TYPE_SAMPLER ||
                        }
         struct v3dv_cl_reloc
   v3dv_descriptor_map_get_sampler_state(struct v3dv_device *device,
                           {
      VkDescriptorType type;
   struct v3dv_cl_reloc reloc =
      v3dv_descriptor_map_get_descriptor_bo(device, descriptor_state, map,
               assert(type == VK_DESCRIPTOR_TYPE_SAMPLER ||
            if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
               }
      struct v3dv_bo*
   v3dv_descriptor_map_get_texture_bo(struct v3dv_descriptor_state *descriptor_state,
                        {
      struct v3dv_descriptor *descriptor =
      v3dv_descriptor_map_get_descriptor(descriptor_state, map,
         switch (descriptor->type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      assert(descriptor->buffer_view);
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
      assert(descriptor->image_view);
   struct v3dv_image *image =
         assert(map->plane[index] < image->plane_count);
      }
   default:
            }
      struct v3dv_cl_reloc
   v3dv_descriptor_map_get_texture_shader_state(struct v3dv_device *device,
                           {
      VkDescriptorType type;
   struct v3dv_cl_reloc reloc =
      v3dv_descriptor_map_get_descriptor_bo(device,
                     assert(type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
         type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
   type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT ||
   type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
            if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
               }
      #define SHA1_UPDATE_VALUE(ctx, x) _mesa_sha1_update(ctx, &(x), sizeof(x));
      static void
   sha1_update_ycbcr_conversion(struct mesa_sha1 *ctx,
         {
      SHA1_UPDATE_VALUE(ctx, conversion->format);
   SHA1_UPDATE_VALUE(ctx, conversion->ycbcr_model);
   SHA1_UPDATE_VALUE(ctx, conversion->ycbcr_range);
   SHA1_UPDATE_VALUE(ctx, conversion->mapping);
   SHA1_UPDATE_VALUE(ctx, conversion->chroma_offsets);
      }
      static void
   sha1_update_descriptor_set_binding_layout(struct mesa_sha1 *ctx,
               {
      SHA1_UPDATE_VALUE(ctx, layout->type);
   SHA1_UPDATE_VALUE(ctx, layout->array_size);
   SHA1_UPDATE_VALUE(ctx, layout->descriptor_index);
   SHA1_UPDATE_VALUE(ctx, layout->dynamic_offset_count);
   SHA1_UPDATE_VALUE(ctx, layout->dynamic_offset_index);
   SHA1_UPDATE_VALUE(ctx, layout->descriptor_offset);
   SHA1_UPDATE_VALUE(ctx, layout->immutable_samplers_offset);
            if (layout->immutable_samplers_offset) {
      const struct v3dv_sampler *immutable_samplers =
            for (unsigned i = 0; i < layout->array_size; i++) {
      const struct v3dv_sampler *sampler = &immutable_samplers[i];
   if (sampler->conversion)
            }
      static void
   sha1_update_descriptor_set_layout(struct mesa_sha1 *ctx,
         {
      SHA1_UPDATE_VALUE(ctx, layout->flags);
   SHA1_UPDATE_VALUE(ctx, layout->binding_count);
   SHA1_UPDATE_VALUE(ctx, layout->shader_stages);
   SHA1_UPDATE_VALUE(ctx, layout->descriptor_count);
            for (uint16_t i = 0; i < layout->binding_count; i++)
      }
         /*
   * As anv and tu already points:
   *
   * "Pipeline layouts.  These have nothing to do with the pipeline.  They are
   * just multiple descriptor set layouts pasted together."
   */
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreatePipelineLayout(VkDevice _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            assert(pCreateInfo->sType ==
            layout = vk_object_zalloc(&device->vk, pAllocator, sizeof(*layout),
         if (layout == NULL)
            layout->num_sets = pCreateInfo->setLayoutCount;
            uint32_t dynamic_offset_count = 0;
   for (uint32_t set = 0; set < pCreateInfo->setLayoutCount; set++) {
      V3DV_FROM_HANDLE(v3dv_descriptor_set_layout, set_layout,
         v3dv_descriptor_set_layout_ref(set_layout);
   layout->set[set].layout = set_layout;
   layout->set[set].dynamic_offset_start = dynamic_offset_count;
   for (uint32_t b = 0; b < set_layout->binding_count; b++) {
      dynamic_offset_count += set_layout->binding[b].array_size *
                           layout->push_constant_size = 0;
   for (unsigned i = 0; i < pCreateInfo->pushConstantRangeCount; ++i) {
      const VkPushConstantRange *range = pCreateInfo->pPushConstantRanges + i;
   layout->push_constant_size =
                                 struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);
   for (unsigned s = 0; s < layout->num_sets; s++) {
      sha1_update_descriptor_set_layout(&ctx, layout->set[s].layout);
   _mesa_sha1_update(&ctx, &layout->set[s].dynamic_offset_start,
      }
   _mesa_sha1_update(&ctx, &layout->num_sets, sizeof(layout->num_sets));
                        }
      void
   v3dv_pipeline_layout_destroy(struct v3dv_device *device,
               {
               for (uint32_t i = 0; i < layout->num_sets; i++)
               }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyPipelineLayout(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (!pipeline_layout)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateDescriptorPool(VkDevice _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
   struct v3dv_descriptor_pool *pool;
   /* size is for the vulkan object descriptor pool. The final size would
   * depend on some of FREE_DESCRIPTOR flags used
   */
   uint64_t size = sizeof(struct v3dv_descriptor_pool);
   /* bo_size is for the descriptor related info that we need to have on a GPU
   * address (so on v3dv_bo_alloc allocated memory), like for example the
   * texture sampler state. Note that not all the descriptors use it
   */
   uint32_t bo_size = 0;
            const VkDescriptorPoolInlineUniformBlockCreateInfo *inline_info =
      vk_find_struct_const(pCreateInfo->pNext,
         for (unsigned i = 0; i < pCreateInfo->poolSizeCount; ++i) {
      /* Verify supported descriptor type */
   switch(pCreateInfo->pPoolSizes[i].type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
         default:
      unreachable("Unimplemented descriptor type");
               assert(pCreateInfo->pPoolSizes[i].descriptorCount > 0);
   if (pCreateInfo->pPoolSizes[i].type ==
      VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
   /* Inline uniform blocks are specified to use the descriptor array
   * size as the size in bytes of the block.
   */
   assert(inline_info);
   descriptor_count += inline_info->maxInlineUniformBlockBindings;
      } else {
      descriptor_count += pCreateInfo->pPoolSizes[i].descriptorCount;
   bo_size += v3dv_X(device, descriptor_bo_size)(pCreateInfo->pPoolSizes[i].type) *
                  /* We align all our buffers to V3D_NON_COHERENT_ATOM_SIZE, make sure we
   * allocate enough memory to honor that requirement for all our inline
   * buffers too.
   */
   if (inline_info) {
      bo_size += V3D_NON_COHERENT_ATOM_SIZE *
               if (!(pCreateInfo->flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)) {
      uint64_t host_size =
         host_size += sizeof(struct v3dv_descriptor) * descriptor_count;
      } else {
                  pool = vk_object_zalloc(&device->vk, pAllocator, size,
            if (!pool)
            if (!(pCreateInfo->flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)) {
      pool->host_memory_base = (uint8_t*)pool + sizeof(struct v3dv_descriptor_pool);
   pool->host_memory_ptr = pool->host_memory_base;
                        if (bo_size > 0) {
      pool->bo = v3dv_bo_alloc(device, bo_size, "descriptor pool bo", true);
   if (!pool->bo)
            bool ok = v3dv_bo_map(device, pool->bo, pool->bo->size);
   if (!ok)
               } else {
                                          out_of_device_memory:
      vk_object_free(&device->vk, pAllocator, pool);
      }
      static void
   descriptor_set_destroy(struct v3dv_device *device,
                     {
               if (free_bo && !pool->host_memory_base) {
      for (uint32_t i = 0; i < pool->entry_count; i++) {
      if (pool->entries[i].set == set) {
      memmove(&pool->entries[i], &pool->entries[i+1],
         --pool->entry_count;
            }
      }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyDescriptorPool(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (!pool)
            list_for_each_entry_safe(struct v3dv_descriptor_set, set,
                        if (!pool->host_memory_base) {
      for(int i = 0; i < pool->entry_count; ++i) {
                     if (pool->bo) {
      v3dv_bo_free(device, pool->bo);
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_ResetDescriptorPool(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            list_for_each_entry_safe(struct v3dv_descriptor_set, set,
               }
            if (!pool->host_memory_base) {
      for(int i = 0; i < pool->entry_count; ++i) {
            } else {
      /* We clean-up the host memory, so when allocating a new set from the
   * pool, it is already 0
   */
   uint32_t host_size = pool->host_memory_end - pool->host_memory_base;
               pool->entry_count = 0;
   pool->host_memory_ptr = pool->host_memory_base;
               }
      void
   v3dv_descriptor_set_layout_destroy(struct v3dv_device *device,
         {
      assert(set_layout->ref_cnt == 0);
   vk_object_base_finish(&set_layout->base);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateDescriptorSetLayout(VkDevice _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
                     uint32_t num_bindings = 0;
            /* for immutable descriptors, the plane stride is the largest plane
   * count of all combined image samplers. For mutable descriptors
   * this is always 1 since multiplanar images are restricted to
   * immutable combined image samplers.
   */
   uint8_t plane_stride = 1;
   for (uint32_t j = 0; j < pCreateInfo->bindingCount; j++) {
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
   VkDescriptorType desc_type = pCreateInfo->pBindings[j].descriptorType;
   if ((desc_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
      desc_type == VK_DESCRIPTOR_TYPE_SAMPLER) &&
   pCreateInfo->pBindings[j].pImmutableSamplers) {
                  for (uint32_t i = 0; i < descriptor_count; i++) {
      const VkSampler vk_sampler =
         VK_FROM_HANDLE(v3dv_sampler, sampler, vk_sampler);
                     /* We place immutable samplers after the binding data. We want to use
   * offsetof instead of any sizeof(struct v3dv_descriptor_set_layout)
   * because the latter may include padding at the end of the struct.
   */
   uint32_t samplers_offset =
            uint32_t size = samplers_offset +
            /* Descriptor set layouts are reference counted and therefore can survive
   * vkDestroyPipelineSetLayout, so they need to be allocated with a device
   * scope.
   */
   set_layout =
         if (!set_layout)
            vk_object_base_init(&device->vk, &set_layout->base,
                              VkDescriptorSetLayoutBinding *bindings = NULL;
   VkResult result = vk_create_sorted_bindings(pCreateInfo->pBindings,
         if (result != VK_SUCCESS) {
      v3dv_descriptor_set_layout_destroy(device, set_layout);
               set_layout->binding_count = num_bindings;
   set_layout->flags = pCreateInfo->flags;
   set_layout->shader_stages = 0;
   set_layout->bo_size = 0;
            uint32_t descriptor_count = 0;
            for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
      const VkDescriptorSetLayoutBinding *binding = bindings + i;
            switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      set_layout->binding[binding_number].dynamic_offset_count = 1;
      case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      /* Nothing here, just to keep the descriptor type filtering below */
      default:
      unreachable("Unknown descriptor type\n");
               set_layout->binding[binding_number].type = binding->descriptorType;
   set_layout->binding[binding_number].array_size = binding->descriptorCount;
   set_layout->binding[binding_number].descriptor_index = descriptor_count;
   set_layout->binding[binding_number].dynamic_offset_index = dynamic_offset_count;
            if ((binding->descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                                                                                 if (binding->descriptorType != VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
                              set_layout->binding[binding_number].descriptor_offset =
         set_layout->bo_size +=
      v3dv_X(device, descriptor_bo_size)(set_layout->binding[binding_number].type) *
   } else {
      /* We align all our buffers, inline buffers too. We made sure to take
   * this account when calculating total BO size requirements at pool
   * creation time.
   */
                                 /* Inline uniform blocks are not arrayed, instead descriptorCount
   * specifies the size of the buffer in bytes.
   */
   set_layout->bo_size += binding->descriptorCount;
                           set_layout->descriptor_count = descriptor_count;
                        }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyDescriptorSetLayout(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (!set_layout)
               }
      static inline VkResult
   out_of_pool_memory(const struct v3dv_device *device,
         {
      /* Don't log OOPM errors for internal driver pools, we handle these properly
   * by allocating a new pool, so they don't point to real issues.
   */
   if (!pool->is_driver_internal)
         else
      }
      static VkResult
   descriptor_set_create(struct v3dv_device *device,
                     {
      struct v3dv_descriptor_set *set;
   uint32_t descriptor_count = layout->descriptor_count;
   unsigned mem_size = sizeof(struct v3dv_descriptor_set) +
            if (pool->host_memory_base) {
      if (pool->host_memory_end - pool->host_memory_ptr < mem_size)
            set = (struct v3dv_descriptor_set*)pool->host_memory_ptr;
               } else {
      set = vk_object_zalloc(&device->vk, NULL, mem_size,
            if (!set)
                                 /* FIXME: VK_EXT_descriptor_indexing introduces
   * VARIABLE_DESCRIPTOR_LAYOUT_COUNT. That would affect the layout_size used
   * below for bo allocation
            uint32_t offset = 0;
            if (layout->bo_size) {
      if (!pool->host_memory_base && pool->entry_count == pool->max_entry_count) {
      vk_object_free(&device->vk, NULL, set);
               /* We first try to allocate linearly fist, so that we don't spend time
   * looking for gaps if the app only allocates & resets via the pool.
   *
   * If that fails, we try to find a gap from previously freed subregions
   * iterating through the descriptor pool entries. Note that we are not
   * doing that if we have a pool->host_memory_base. We only have that if
   * VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT is not set, so in
   * that case the user can't free subregions, so it doesn't make sense to
   * even try (or track those subregions).
   */
   if (pool->current_offset + layout->bo_size <= pool->bo->size) {
      offset = pool->current_offset;
      } else if (!pool->host_memory_base) {
      for (index = 0; index < pool->entry_count; index++) {
      if (pool->entries[index].offset - offset >= layout->bo_size)
            }
   if (pool->bo->size - offset < layout->bo_size) {
      vk_object_free(&device->vk, NULL, set);
      }
   memmove(&pool->entries[index + 1], &pool->entries[index],
      } else {
      assert(pool->host_memory_base);
                           if (!pool->host_memory_base) {
      pool->entries[index].set = set;
   pool->entries[index].offset = offset;
   pool->entries[index].size = layout->bo_size;
               /* Go through and fill out immutable samplers if we have any */
   for (uint32_t b = 0; b < layout->binding_count; b++) {
      if (layout->binding[b].immutable_samplers_offset == 0)
            const struct v3dv_sampler *samplers =
                  for (uint32_t i = 0; i < layout->binding[b].array_size; i++) {
      assert(samplers[i].plane_count <= V3DV_MAX_PLANE_COUNT);
   for (uint8_t plane = 0; plane < samplers[i].plane_count; plane++) {
      uint32_t combined_offset =
      layout->binding[b].type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ?
      void *desc_map =
                  memcpy(desc_map, samplers[i].sampler_state,
                     v3dv_descriptor_set_layout_ref(layout);
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_AllocateDescriptorSets(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            VkResult result = VK_SUCCESS;
   struct v3dv_descriptor_set *set = NULL;
            for (i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
      V3DV_FROM_HANDLE(v3dv_descriptor_set_layout, layout,
            result = descriptor_set_create(device, pool, layout, &set);
   if (result != VK_SUCCESS)
                        if (result != VK_SUCCESS) {
      v3dv_FreeDescriptorSets(_device, pAllocateInfo->descriptorPool,
         for (i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_FreeDescriptorSets(VkDevice _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            for (uint32_t i = 0; i < count; i++) {
               if (set) {
      v3dv_descriptor_set_layout_unref(device, set->layout);
   list_del(&set->pool_link);
   if (!pool->host_memory_base)
                     }
      static void
   descriptor_bo_copy(struct v3dv_device *device,
                     struct v3dv_descriptor_set *dst_set,
   const struct v3dv_descriptor_set_binding_layout *dst_binding_layout,
   uint32_t dst_array_index,
   {
      assert(dst_binding_layout->type == src_binding_layout->type);
            void *dst_map = descriptor_bo_map(device, dst_set, dst_binding_layout,
         void *src_map = descriptor_bo_map(device, src_set, src_binding_layout,
            memcpy(dst_map, src_map,
            }
      static void
   write_buffer_descriptor(struct v3dv_descriptor *descriptor,
               {
               descriptor->type = desc_type;
   descriptor->buffer = buffer;
   descriptor->offset = buffer_info->offset;
   if (buffer_info->range == VK_WHOLE_SIZE) {
         } else {
      assert(descriptor->range <= UINT32_MAX);
         }
      static void
   write_image_descriptor(struct v3dv_device *device,
                        struct v3dv_descriptor *descriptor,
   VkDescriptorType desc_type,
   struct v3dv_descriptor_set *set,
      {
      descriptor->type = desc_type;
   descriptor->sampler = sampler;
            assert(iview || sampler);
            void *desc_map = descriptor_bo_map(device, set,
            for (uint8_t plane = 0; plane < plane_count; plane++) {
      if (iview) {
                              const uint32_t tex_state_index =
      iview->vk.view_type != VK_IMAGE_VIEW_TYPE_CUBE_ARRAY ||
      memcpy(plane_desc_map,
                     if (sampler && !binding_layout->immutable_samplers_offset) {
                     void *plane_desc_map = desc_map + offset;
   /* For immutable samplers this was already done as part of the
   * descriptor set create, as that info can't change later
   */
   memcpy(plane_desc_map,
                  }
         static void
   write_buffer_view_descriptor(struct v3dv_device *device,
                              struct v3dv_descriptor *descriptor,
      {
      assert(bview);
   descriptor->type = desc_type;
                     memcpy(desc_map,
            }
      static void
   write_inline_uniform_descriptor(struct v3dv_device *device,
                                 struct v3dv_descriptor *descriptor,
   {
      assert(binding_layout->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK);
   descriptor->type = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
            void *desc_map = descriptor_bo_map(device, set, binding_layout, 0);
            /* Inline uniform buffers allocate BO space in the pool for all inline
   * buffers it may allocate and then this space is assigned to individual
   * descriptors when they are written, so we define the range of an inline
   * buffer as the largest range of data that the client has written to it.
   */
   descriptor->offset = 0;
      }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_UpdateDescriptorSets(VkDevice  _device,
                           {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
   for (uint32_t i = 0; i < descriptorWriteCount; i++) {
      const VkWriteDescriptorSet *writeset = &pDescriptorWrites[i];
            const struct v3dv_descriptor_set_binding_layout *binding_layout =
                              /* Inline uniform blocks are not arrayed, instead they use dstArrayElement
   * to specify the byte offset of the uniform update and descriptorCount
   * to specify the size (in bytes) of the update.
   */
   uint32_t descriptor_count;
   if (writeset->descriptorType != VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      descriptor += writeset->dstArrayElement;
      } else {
                  for (uint32_t j = 0; j < descriptor_count; ++j) {
               case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
      const VkDescriptorBufferInfo *buffer_info = writeset->pBufferInfo + j;
   write_buffer_descriptor(descriptor, writeset->descriptorType,
            }
   case VK_DESCRIPTOR_TYPE_SAMPLER: {
      /* If we are here we shouldn't be modifying an immutable sampler */
   assert(!binding_layout->immutable_samplers_offset);
                  write_image_descriptor(device, descriptor, writeset->descriptorType,
                     }
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
                     write_image_descriptor(device, descriptor, writeset->descriptorType,
                     }
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
      const VkDescriptorImageInfo *image_info = writeset->pImageInfo + j;
   V3DV_FROM_HANDLE(v3dv_image_view, iview, image_info->imageView);
   struct v3dv_sampler *sampler = NULL;
   if (!binding_layout->immutable_samplers_offset) {
      /* In general we ignore the sampler when updating a combined
   * image sampler, but for YCbCr we kwnow that we must use
   * immutable combined image samplers
   */
   assert(iview->plane_count == 1);
                     write_image_descriptor(device, descriptor, writeset->descriptorType,
                     }
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: {
      V3DV_FROM_HANDLE(v3dv_buffer_view, buffer_view,
         write_buffer_view_descriptor(device, descriptor, writeset->descriptorType,
                  }
   case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK: {
      const VkWriteDescriptorSetInlineUniformBlock *inline_write =
      vk_find_struct_const(writeset->pNext,
      assert(inline_write->dataSize == writeset->descriptorCount);
   write_inline_uniform_descriptor(device, descriptor, set,
                              }
   default:
      unreachable("unimplemented descriptor type");
      }
                  for (uint32_t i = 0; i < descriptorCopyCount; i++) {
      const VkCopyDescriptorSet *copyset = &pDescriptorCopies[i];
   V3DV_FROM_HANDLE(v3dv_descriptor_set, src_set,
         V3DV_FROM_HANDLE(v3dv_descriptor_set, dst_set,
            const struct v3dv_descriptor_set_binding_layout *src_binding_layout =
         const struct v3dv_descriptor_set_binding_layout *dst_binding_layout =
                     struct v3dv_descriptor *src_descriptor = src_set->descriptors;
            src_descriptor += src_binding_layout->descriptor_index;
            if (src_binding_layout->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      /* {src,dst}ArrayElement specifies src/dst start offset and
   * descriptorCount specifies size (in bytes) to copy.
   */
   const void *src_data = src_set->pool->bo->map +
                     write_inline_uniform_descriptor(device, dst_descriptor, dst_set,
                                       src_descriptor += copyset->srcArrayElement;
            for (uint32_t j = 0; j < copyset->descriptorCount; j++) {
      *dst_descriptor = *src_descriptor;
                  if (v3dv_X(device, descriptor_bo_size)(src_binding_layout->type) > 0) {
      descriptor_bo_copy(device,
                                    }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetDescriptorSetLayoutSupport(
      VkDevice _device,
   const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
      {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
   VkDescriptorSetLayoutBinding *bindings = NULL;
   VkResult result = vk_create_sorted_bindings(
         if (result != VK_SUCCESS) {
      pSupport->supported = false;
                        uint32_t desc_host_size = sizeof(struct v3dv_descriptor);
   uint32_t host_size = sizeof(struct v3dv_descriptor_set);
   uint32_t bo_size = 0;
   for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
               if ((UINT32_MAX - host_size) / desc_host_size < binding->descriptorCount) {
      supported = false;
               uint32_t desc_bo_size = v3dv_X(device, descriptor_bo_size)(binding->descriptorType);
   if (desc_bo_size > 0 &&
      (UINT32_MAX - bo_size) / desc_bo_size < binding->descriptorCount) {
   supported = false;
               host_size += binding->descriptorCount * desc_host_size;
                           }
      void
   v3dv_UpdateDescriptorSetWithTemplate(
      VkDevice _device,
   VkDescriptorSet descriptorSet,
   VkDescriptorUpdateTemplate descriptorUpdateTemplate,
      {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
   V3DV_FROM_HANDLE(v3dv_descriptor_set, set, descriptorSet);
   V3DV_FROM_HANDLE(vk_descriptor_update_template, template,
            for (int i = 0; i < template->entry_count; i++) {
      const struct vk_descriptor_template_entry *entry =
            const struct v3dv_descriptor_set_binding_layout *binding_layout =
            struct v3dv_descriptor *descriptor =
                  switch (entry->type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      for (uint32_t j = 0; j < entry->array_count; j++) {
      const VkDescriptorBufferInfo *info =
         write_buffer_descriptor(descriptor + entry->array_element + j,
                  case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      for (uint32_t j = 0; j < entry->array_count; j++) {
      const VkDescriptorImageInfo *info =
         V3DV_FROM_HANDLE(v3dv_image_view, iview, info->imageView);
   V3DV_FROM_HANDLE(v3dv_sampler, sampler, info->sampler);
   write_image_descriptor(device, descriptor + entry->array_element + j,
                        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t j = 0; j < entry->array_count; j++) {
      const VkBufferView *_bview =
         V3DV_FROM_HANDLE(v3dv_buffer_view, bview, *_bview);
   write_buffer_view_descriptor(device,
                              case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK: {
      write_inline_uniform_descriptor(device, descriptor, set,
                                       default:
               }
