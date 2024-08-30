   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
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
   #include <assert.h>
   #include <fcntl.h>
   #include <stdbool.h>
   #include <string.h>
      #include "util/mesa-sha1.h"
   #include "radv_private.h"
   #include "sid.h"
   #include "vk_acceleration_structure.h"
   #include "vk_descriptors.h"
   #include "vk_format.h"
   #include "vk_util.h"
      static unsigned
   radv_descriptor_type_buffer_count(VkDescriptorType type)
   {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
   case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
         case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
         default:
            }
      static bool
   has_equal_immutable_samplers(const VkSampler *samplers, uint32_t count)
   {
      if (!samplers)
         for (uint32_t i = 1; i < count; ++i) {
      if (memcmp(radv_sampler_from_handle(samplers[0])->state, radv_sampler_from_handle(samplers[i])->state, 16)) {
            }
      }
      static uint32_t
   radv_descriptor_alignment(VkDescriptorType type)
   {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
   case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
         case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
         default:
            }
      static bool
   radv_mutable_descriptor_type_size_alignment(const VkMutableDescriptorTypeListVALVE *list, uint64_t *out_size,
         {
      uint32_t max_size = 0;
            for (uint32_t i = 0; i < list->descriptorTypeCount; i++) {
      uint32_t size = 0;
            switch (list->pDescriptorTypes[i]) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      size = 16;
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      size = 32;
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      size = 64;
      default:
                  max_size = MAX2(max_size, size);
               *out_size = max_size;
   *out_align = max_align;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateDescriptorSetLayout(VkDevice _device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
            assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
   const VkDescriptorSetLayoutBindingFlagsCreateInfo *variable_flags =
         const VkMutableDescriptorTypeCreateInfoEXT *mutable_info =
            uint32_t num_bindings = 0;
   uint32_t immutable_sampler_count = 0;
   uint32_t ycbcr_sampler_count = 0;
   for (uint32_t j = 0; j < pCreateInfo->bindingCount; j++) {
      num_bindings = MAX2(num_bindings, pCreateInfo->pBindings[j].binding + 1);
   if ((pCreateInfo->pBindings[j].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
      pCreateInfo->pBindings[j].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) &&
                  bool has_ycbcr_sampler = false;
   for (unsigned i = 0; i < pCreateInfo->pBindings[j].descriptorCount; ++i) {
      if (radv_sampler_from_handle(pCreateInfo->pBindings[j].pImmutableSamplers[i])->vk.ycbcr_conversion)
               if (has_ycbcr_sampler)
                  uint32_t samplers_offset = offsetof(struct radv_descriptor_set_layout, binding[num_bindings]);
   size_t size = samplers_offset + immutable_sampler_count * 4 * sizeof(uint32_t);
   if (ycbcr_sampler_count > 0) {
      /* Store block of offsets first, followed by the conversion descriptors (padded to the struct
   * alignment) */
   size += num_bindings * sizeof(uint32_t);
   size = ALIGN(size, alignof(struct vk_ycbcr_conversion_state));
               /* We need to allocate descriptor set layouts off the device allocator with DEVICE scope because
   * they are reference counted and may not be destroyed when vkDestroyDescriptorSetLayout is
   * called.
   */
   set_layout = vk_descriptor_set_layout_zalloc(&device->vk, size);
   if (!set_layout)
                     /* We just allocate all the samplers at the end of the struct */
   uint32_t *samplers = (uint32_t *)&set_layout->binding[num_bindings];
   struct vk_ycbcr_conversion_state *ycbcr_samplers = NULL;
            if (ycbcr_sampler_count > 0) {
      ycbcr_sampler_offsets = samplers + 4 * immutable_sampler_count;
            uintptr_t first_ycbcr_sampler_offset = (uintptr_t)ycbcr_sampler_offsets + sizeof(uint32_t) * num_bindings;
   first_ycbcr_sampler_offset = ALIGN(first_ycbcr_sampler_offset, alignof(struct vk_ycbcr_conversion_state));
      } else
            VkDescriptorSetLayoutBinding *bindings = NULL;
   VkResult result = vk_create_sorted_bindings(pCreateInfo->pBindings, pCreateInfo->bindingCount, &bindings);
   if (result != VK_SUCCESS) {
      vk_descriptor_set_layout_unref(&device->vk, &set_layout->vk);
               set_layout->binding_count = num_bindings;
   set_layout->shader_stages = 0;
   set_layout->dynamic_shader_stages = 0;
   set_layout->has_immutable_samplers = false;
            uint32_t buffer_count = 0;
            uint32_t first_alignment = 32;
   if (pCreateInfo->bindingCount > 0) {
      uint32_t last_alignment = radv_descriptor_alignment(bindings[pCreateInfo->bindingCount - 1].descriptorType);
   if (bindings[pCreateInfo->bindingCount - 1].descriptorType == VK_DESCRIPTOR_TYPE_MUTABLE_EXT) {
      uint64_t mutable_size = 0, mutable_align = 0;
   radv_mutable_descriptor_type_size_alignment(
                                 for (unsigned pass = 0; pass < 2; ++pass) {
      for (uint32_t j = 0; j < pCreateInfo->bindingCount; j++) {
      const VkDescriptorSetLayoutBinding *binding = bindings + j;
   uint32_t b = binding->binding;
   uint32_t alignment = radv_descriptor_alignment(binding->descriptorType);
   unsigned binding_buffer_count = radv_descriptor_type_buffer_count(binding->descriptorType);
                                 if (binding->descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && binding->pImmutableSamplers) {
      for (unsigned i = 0; i < binding->descriptorCount; ++i) {
                     if (conversion) {
      has_ycbcr_sampler = true;
   max_sampled_image_descriptors =
                     switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      assert(!(pCreateInfo->flags & VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR));
   set_layout->binding[b].dynamic_offset_count = 1;
   set_layout->dynamic_shader_stages |= binding->stageFlags;
   if (binding->stageFlags & RADV_RT_STAGE_BITS)
         set_layout->binding[b].size = 0;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      set_layout->binding[b].size = 16;
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      set_layout->binding[b].size = 32;
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      /* main descriptor + fmask descriptor */
   set_layout->binding[b].size = 64;
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      /* main descriptor + fmask descriptor + sampler */
   set_layout->binding[b].size = 96;
      case VK_DESCRIPTOR_TYPE_SAMPLER:
      set_layout->binding[b].size = 16;
      case VK_DESCRIPTOR_TYPE_MUTABLE_EXT: {
      uint64_t mutable_size = 0, mutable_align = 0;
   radv_mutable_descriptor_type_size_alignment(&mutable_info->pMutableDescriptorTypeLists[j], &mutable_size,
         assert(mutable_size && mutable_align);
   set_layout->binding[b].size = mutable_size;
   alignment = mutable_align;
      }
   case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      set_layout->binding[b].size = descriptor_count;
   descriptor_count = 1;
      case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
      set_layout->binding[b].size = 16;
      default:
                                 set_layout->size = align(set_layout->size, alignment);
   set_layout->binding[b].type = binding->descriptorType;
   set_layout->binding[b].array_size = descriptor_count;
   set_layout->binding[b].offset = set_layout->size;
                  if (variable_flags && binding->binding < variable_flags->bindingCount &&
      (variable_flags->pBindingFlags[binding->binding] & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT)) {
                              if ((binding->descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
      binding->descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) &&
   binding->pImmutableSamplers) {
                  /* Do not optimize space for descriptor buffers and embedded samplers, otherwise the set
   * layout size/offset are incorrect.
   */
   if (!(pCreateInfo->flags & (VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT |
                                             /* Don't reserve space for the samplers if they're not accessed. */
   if (set_layout->binding[b].immutable_samplers_equal) {
      if (binding->descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER &&
      max_sampled_image_descriptors <= 2)
      else if (binding->descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
      }
                  if (has_ycbcr_sampler) {
      ycbcr_sampler_offsets[b] = (const char *)ycbcr_samplers - (const char *)set_layout;
   for (uint32_t i = 0; i < binding->descriptorCount; i++) {
      if (radv_sampler_from_handle(binding->pImmutableSamplers[i])->vk.ycbcr_conversion)
      ycbcr_samplers[i] =
      else
      }
                  set_layout->size += descriptor_count * set_layout->binding[b].size;
   buffer_count += descriptor_count * binding_buffer_count;
   dynamic_offset_count += descriptor_count * set_layout->binding[b].dynamic_offset_count;
                           set_layout->buffer_count = buffer_count;
            /* Hash the entire set layout except vk_descriptor_set_layout. The rest of the set layout is
   * carefully constructed to not have pointers so a full hash instead of a per-field hash
   * should be ok.
   */
   uint32_t hash_offset = offsetof(struct radv_descriptor_set_layout, hash) + sizeof(set_layout->hash);
                        }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetDescriptorSetLayoutSupport(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
         {
      VkDescriptorSetLayoutBinding *bindings = NULL;
   VkResult result = vk_create_sorted_bindings(pCreateInfo->pBindings, pCreateInfo->bindingCount, &bindings);
   if (result != VK_SUCCESS) {
      pSupport->supported = false;
               const VkDescriptorSetLayoutBindingFlagsCreateInfo *variable_flags =
         VkDescriptorSetVariableDescriptorCountLayoutSupport *variable_count =
         const VkMutableDescriptorTypeCreateInfoEXT *mutable_info =
         if (variable_count) {
                  uint32_t first_alignment = 32;
   if (pCreateInfo->bindingCount > 0) {
      uint32_t last_alignment = radv_descriptor_alignment(bindings[pCreateInfo->bindingCount - 1].descriptorType);
   if (bindings[pCreateInfo->bindingCount - 1].descriptorType == VK_DESCRIPTOR_TYPE_MUTABLE_EXT) {
      uint64_t mutable_size = 0, mutable_align = 0;
   radv_mutable_descriptor_type_size_alignment(
                                 bool supported = true;
   uint64_t size = 0;
   for (unsigned pass = 0; pass < 2; ++pass) {
      for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
               uint64_t descriptor_size = 0;
   uint64_t descriptor_alignment = radv_descriptor_alignment(binding->descriptorType);
   uint32_t descriptor_count = binding->descriptorCount;
   switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      descriptor_size = 16;
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      descriptor_size = 32;
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      descriptor_size = 64;
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      if (!has_equal_immutable_samplers(binding->pImmutableSamplers, descriptor_count)) {
         } else {
         }
      case VK_DESCRIPTOR_TYPE_SAMPLER:
      if (!has_equal_immutable_samplers(binding->pImmutableSamplers, descriptor_count)) {
         }
      case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      descriptor_size = descriptor_count;
   descriptor_count = 1;
      case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
      if (!radv_mutable_descriptor_type_size_alignment(&mutable_info->pMutableDescriptorTypeLists[i],
               }
      case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
      descriptor_size = 16;
      default:
                  if ((pass == 0 && descriptor_alignment != first_alignment) ||
                  if (size && !align_u64(size, descriptor_alignment)) {
                        uint64_t max_count = INT32_MAX;
   if (binding->descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK)
                        if (max_count < descriptor_count) {
         }
   if (variable_flags && binding->binding < variable_flags->bindingCount && variable_count &&
      (variable_flags->pBindingFlags[binding->binding] & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT)) {
      }
                              }
      /*
   * Pipeline layouts.  These have nothing to do with the pipeline.  They are
   * just multiple descriptor set layouts pasted together.
   */
   void
   radv_pipeline_layout_init(struct radv_device *device, struct radv_pipeline_layout *layout, bool independent_sets)
   {
                           }
      void
   radv_pipeline_layout_add_set(struct radv_pipeline_layout *layout, uint32_t set_idx,
         {
      if (layout->set[set_idx].layout)
                     layout->set[set_idx].layout = set_layout;
                     layout->dynamic_offset_count += set_layout->dynamic_offset_count;
      }
      void
   radv_pipeline_layout_hash(struct radv_pipeline_layout *layout)
   {
               _mesa_sha1_init(&ctx);
   for (uint32_t i = 0; i < layout->num_sets; i++) {
               if (!set_layout)
               }
   _mesa_sha1_update(&ctx, &layout->push_constant_size, sizeof(layout->push_constant_size));
      }
      void
   radv_pipeline_layout_finish(struct radv_device *device, struct radv_pipeline_layout *layout)
   {
      for (uint32_t i = 0; i < layout->num_sets; i++) {
      if (!layout->set[i].layout)
                           }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreatePipelineLayout(VkDevice _device, const VkPipelineLayoutCreateInfo *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
                     layout = vk_alloc2(&device->vk.alloc, pAllocator, sizeof(*layout), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (layout == NULL)
                              for (uint32_t set = 0; set < pCreateInfo->setLayoutCount; set++) {
               if (set_layout == NULL) {
      layout->set[set].layout = NULL;
                                    for (unsigned i = 0; i < pCreateInfo->pushConstantRangeCount; ++i) {
      const VkPushConstantRange *range = pCreateInfo->pPushConstantRanges + i;
                                             }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyPipelineLayout(VkDevice _device, VkPipelineLayout _pipelineLayout, const VkAllocationCallbacks *pAllocator)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!pipeline_layout)
                        }
      static VkResult
   radv_descriptor_set_create(struct radv_device *device, struct radv_descriptor_pool *pool,
               {
      if (pool->entry_count == pool->max_entry_count)
            struct radv_descriptor_set *set;
   uint32_t buffer_count = layout->buffer_count;
   if (variable_count) {
      unsigned stride = radv_descriptor_type_buffer_count(layout->binding[layout->binding_count - 1].type);
      }
   unsigned range_offset = sizeof(struct radv_descriptor_set_header) + sizeof(struct radeon_winsys_bo *) * buffer_count;
   const unsigned dynamic_offset_count = layout->dynamic_offset_count;
            if (pool->host_memory_base) {
      if (pool->host_memory_end - pool->host_memory_ptr < mem_size)
            set = (struct radv_descriptor_set *)pool->host_memory_ptr;
      } else {
               if (!set)
                                 if (dynamic_offset_count) {
                  set->header.layout = layout;
   set->header.buffer_count = buffer_count;
   uint32_t layout_size = layout->size;
   if (variable_count) {
      uint32_t stride = layout->binding[layout->binding_count - 1].size;
   if (layout->binding[layout->binding_count - 1].type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK)
               }
   layout_size = align_u32(layout_size, 32);
            /* try to allocate linearly first, so that we don't spend
   * time looking for gaps if the app only allocates &
   * resets via the pool. */
   if (pool->current_offset + layout_size <= pool->size) {
      set->header.bo = pool->bo;
   set->header.mapped_ptr = (uint32_t *)(pool->mapped_ptr + pool->current_offset);
            if (!pool->host_memory_base) {
      pool->entries[pool->entry_count].offset = pool->current_offset;
   pool->entries[pool->entry_count].size = layout_size;
      } else {
                     } else if (!pool->host_memory_base) {
      uint64_t offset = 0;
            for (index = 0; index < pool->entry_count; ++index) {
      if (pool->entries[index].offset - offset >= layout_size)
                     if (pool->size - offset < layout_size) {
      vk_free2(&device->vk.alloc, NULL, set);
      }
   set->header.bo = pool->bo;
   set->header.mapped_ptr = (uint32_t *)(pool->mapped_ptr + offset);
   set->header.va = pool->bo ? (radv_buffer_get_va(set->header.bo) + offset) : 0;
   memmove(&pool->entries[index + 1], &pool->entries[index], sizeof(pool->entries[0]) * (pool->entry_count - index));
   pool->entries[index].offset = offset;
   pool->entries[index].size = layout_size;
      } else
            if (layout->has_immutable_samplers) {
      for (unsigned i = 0; i < layout->binding_count; ++i) {
                     unsigned offset = layout->binding[i].offset / 4;
                  const uint32_t *samplers =
         for (unsigned j = 0; j < layout->binding[i].array_size; ++j) {
      memcpy(set->header.mapped_ptr + offset, samplers + 4 * j, 16);
                     pool->entry_count++;
   vk_descriptor_set_layout_ref(&layout->vk);
   *out_set = set;
      }
      static void
   radv_descriptor_set_destroy(struct radv_device *device, struct radv_descriptor_pool *pool,
         {
                        if (free_bo && !pool->host_memory_base) {
      for (int i = 0; i < pool->entry_count; ++i) {
      if (pool->entries[i].set == set) {
      memmove(&pool->entries[i], &pool->entries[i + 1], sizeof(pool->entries[i]) * (pool->entry_count - i - 1));
   --pool->entry_count;
            }
   vk_object_base_finish(&set->header.base);
      }
      static void
   radv_destroy_descriptor_pool(struct radv_device *device, const VkAllocationCallbacks *pAllocator,
         {
         if (!pool->host_memory_base) {
      for (uint32_t i = 0; i < pool->entry_count; ++i) {
            } else {
      for (uint32_t i = 0; i < pool->entry_count; ++i) {
      vk_descriptor_set_layout_unref(&device->vk, &pool->sets[i]->header.layout->vk);
                  if (pool->bo) {
      radv_rmv_log_bo_destroy(device, pool->bo);
      }
   if (pool->host_bo)
            radv_rmv_log_resource_destroy(device, (uint64_t)radv_descriptor_pool_to_handle(pool));
   vk_object_base_finish(&pool->base);
      }
      VkResult
   radv_create_descriptor_pool(struct radv_device *device, const VkDescriptorPoolCreateInfo *pCreateInfo,
               {
      struct radv_descriptor_pool *pool;
   uint64_t size = sizeof(struct radv_descriptor_pool);
            const VkMutableDescriptorTypeCreateInfoEXT *mutable_info =
            vk_foreach_struct_const (ext, pCreateInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO: {
      const VkDescriptorPoolInlineUniformBlockCreateInfo *info =
         /* the sizes are 4 aligned, and we need to align to at
   * most 32, which needs at most 28 bytes extra per
   * binding. */
   bo_size += 28llu * info->maxInlineUniformBlockBindings;
      }
   default:
                     uint64_t num_16byte_descriptors = 0;
   for (unsigned i = 0; i < pCreateInfo->poolSizeCount; ++i) {
      bo_count += radv_descriptor_type_buffer_count(pCreateInfo->pPoolSizes[i].type) *
            switch (pCreateInfo->pPoolSizes[i].type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      range_count += pCreateInfo->pPoolSizes[i].descriptorCount;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
      bo_size += 16 * pCreateInfo->pPoolSizes[i].descriptorCount;
   num_16byte_descriptors += pCreateInfo->pPoolSizes[i].descriptorCount;
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      bo_size += 32 * pCreateInfo->pPoolSizes[i].descriptorCount;
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      bo_size += 64 * pCreateInfo->pPoolSizes[i].descriptorCount;
      case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
      /* Per spec, if a mutable descriptor type list is provided for the pool entry, we
   * allocate enough memory to hold any subset of that list.
   * If there is no mutable descriptor type list available,
   * we must allocate enough for any supported mutable descriptor type, i.e. 64 bytes. */
   if (mutable_info && i < mutable_info->mutableDescriptorTypeListCount) {
      uint64_t mutable_size, mutable_alignment;
   if (radv_mutable_descriptor_type_size_alignment(&mutable_info->pMutableDescriptorTypeLists[i],
            /* 32 as we may need to align for images */
   mutable_size = align(mutable_size, 32);
   bo_size += mutable_size * pCreateInfo->pPoolSizes[i].descriptorCount;
   if (mutable_size < 32)
         } else {
         }
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      bo_size += 96 * pCreateInfo->pPoolSizes[i].descriptorCount;
      case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      bo_size += pCreateInfo->pPoolSizes[i].descriptorCount;
      default:
                     if (num_16byte_descriptors) {
      /* Reserve space to align before image descriptors. Our layout code ensures at most one gap
   * per set. */
                        if (!(pCreateInfo->flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)) {
      size += pCreateInfo->maxSets * sizeof(struct radv_descriptor_set);
   size += sizeof(struct radeon_winsys_bo *) * bo_count;
            sets_size = sizeof(struct radv_descriptor_set *) * pCreateInfo->maxSets;
      } else {
                  pool = vk_alloc2(&device->vk.alloc, pAllocator, size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!pool)
                              if (!(pCreateInfo->flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)) {
      pool->host_memory_base = (uint8_t *)pool + sizeof(struct radv_descriptor_pool) + sets_size;
   pool->host_memory_ptr = pool->host_memory_base;
               if (bo_size) {
      if (!(pCreateInfo->flags & VK_DESCRIPTOR_POOL_CREATE_HOST_ONLY_BIT_VALVE)) {
                              VkResult result = device->ws->buffer_create(device->ws, bo_size, 32, RADEON_DOMAIN_VRAM, flags,
         if (result != VK_SUCCESS) {
      radv_destroy_descriptor_pool(device, pAllocator, pool);
      }
   pool->mapped_ptr = (uint8_t *)device->ws->buffer_map(pool->bo);
   if (!pool->mapped_ptr) {
      radv_destroy_descriptor_pool(device, pAllocator, pool);
         } else {
      pool->host_bo = vk_alloc2(&device->vk.alloc, pAllocator, bo_size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!pool->host_bo) {
      radv_destroy_descriptor_pool(device, pAllocator, pool);
      }
         }
   pool->size = bo_size;
            *pDescriptorPool = radv_descriptor_pool_to_handle(pool);
   radv_rmv_log_descriptor_pool_create(device, pCreateInfo, *pDescriptorPool, is_internal);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateDescriptorPool(VkDevice _device, const VkDescriptorPoolCreateInfo *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyDescriptorPool(VkDevice _device, VkDescriptorPool _pool, const VkAllocationCallbacks *pAllocator)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!pool)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_ResetDescriptorPool(VkDevice _device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!pool->host_memory_base) {
      for (uint32_t i = 0; i < pool->entry_count; ++i) {
            } else {
      for (uint32_t i = 0; i < pool->entry_count; ++i) {
      vk_descriptor_set_layout_unref(&device->vk, &pool->sets[i]->header.layout->vk);
                           pool->current_offset = 0;
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_AllocateDescriptorSets(VkDevice _device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
            VkResult result = VK_SUCCESS;
   uint32_t i;
            const VkDescriptorSetVariableDescriptorCountAllocateInfo *variable_counts =
                  /* allocate a set of buffers for each shader to contain descriptors */
   for (i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
               const uint32_t *variable_count = NULL;
   if (layout->has_variable_descriptors && variable_counts) {
      if (i < variable_counts->descriptorSetCount)
         else
                        result = radv_descriptor_set_create(device, pool, layout, variable_count, &set);
   if (result != VK_SUCCESS)
                        if (result != VK_SUCCESS) {
      radv_FreeDescriptorSets(_device, pAllocateInfo->descriptorPool, i, pDescriptorSets);
   for (i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
            }
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_FreeDescriptorSets(VkDevice _device, VkDescriptorPool descriptorPool, uint32_t count,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
            for (uint32_t i = 0; i < count; i++) {
               if (set && !pool->host_memory_base)
      }
      }
      static ALWAYS_INLINE void
   write_texel_buffer_descriptor(struct radv_device *device, struct radv_cmd_buffer *cmd_buffer, unsigned *dst,
         {
               if (!buffer_view) {
      memset(dst, 0, 4 * 4);
   if (!cmd_buffer)
                              if (device->use_global_bo_list)
            if (cmd_buffer)
         else
      }
      static ALWAYS_INLINE void
   write_buffer_descriptor(struct radv_device *device, unsigned *dst, uint64_t va, uint64_t range)
   {
      if (!va) {
      memset(dst, 0, 4 * 4);
               uint32_t rsrc_word3 = S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) | S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
            if (device->physical_device->rad_info.gfx_level >= GFX11) {
         } else if (device->physical_device->rad_info.gfx_level >= GFX10) {
      rsrc_word3 |= S_008F0C_FORMAT(V_008F0C_GFX10_FORMAT_32_FLOAT) | S_008F0C_OOB_SELECT(V_008F0C_OOB_SELECT_RAW) |
      } else {
      rsrc_word3 |=
               dst[0] = va;
   dst[1] = S_008F04_BASE_ADDRESS_HI(va >> 32);
   /* robustBufferAccess is relaxed enough to allow this (in combination with the alignment/size
   * we return from vkGetBufferMemoryRequirements) and this allows the shader compiler to create
   * more efficient 8/16-bit buffer accesses.
   */
   dst[2] = align(range, 4);
      }
      static ALWAYS_INLINE void
   write_buffer_descriptor_impl(struct radv_device *device, struct radv_cmd_buffer *cmd_buffer, unsigned *dst,
         {
      RADV_FROM_HANDLE(radv_buffer, buffer, buffer_info->buffer);
            if (buffer) {
               range = vk_buffer_range(&buffer->vk, buffer_info->offset, buffer_info->range);
                        if (device->use_global_bo_list)
            if (!buffer) {
      if (!cmd_buffer)
                     if (cmd_buffer)
         else
      }
      static ALWAYS_INLINE void
   write_block_descriptor(struct radv_device *device, struct radv_cmd_buffer *cmd_buffer, void *dst,
         {
      const VkWriteDescriptorSetInlineUniformBlock *inline_ub =
               }
      static ALWAYS_INLINE void
   write_dynamic_buffer_descriptor(struct radv_device *device, struct radv_descriptor_range *range,
         {
      RADV_FROM_HANDLE(radv_buffer, buffer, buffer_info->buffer);
   uint64_t va;
            if (!buffer) {
      range->va = 0;
   *buffer_list = NULL;
                        size = vk_buffer_range(&buffer->vk, buffer_info->offset, buffer_info->range);
            /* robustBufferAccess is relaxed enough to allow this (in combination
   * with the alignment/size we return from vkGetBufferMemoryRequirements)
   * and this allows the shader compiler to create more efficient 8/16-bit
   * buffer accesses. */
            va += buffer_info->offset + buffer->offset;
   range->va = va;
               }
      static ALWAYS_INLINE void
   write_image_descriptor(unsigned *dst, unsigned size, VkDescriptorType descriptor_type,
         {
      struct radv_image_view *iview = NULL;
            if (image_info)
            if (!iview) {
      memset(dst, 0, size);
               if (descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
         } else {
         }
               }
      static ALWAYS_INLINE void
   write_image_descriptor_impl(struct radv_device *device, struct radv_cmd_buffer *cmd_buffer, unsigned size,
               {
                        if (device->use_global_bo_list)
            if (!iview) {
      if (!cmd_buffer)
                     const uint32_t max_bindings = sizeof(iview->image->bindings) / sizeof(iview->image->bindings[0]);
   for (uint32_t b = 0; b < max_bindings; b++) {
      if (cmd_buffer) {
      if (iview->image->bindings[b].bo)
      } else {
      *buffer_list = iview->image->bindings[b].bo;
            }
      static ALWAYS_INLINE void
   write_combined_image_sampler_descriptor(struct radv_device *device, struct radv_cmd_buffer *cmd_buffer,
                     {
      write_image_descriptor_impl(device, cmd_buffer, sampler_offset, dst, buffer_list, descriptor_type, image_info);
   /* copy over sampler state */
   if (has_sampler) {
      RADV_FROM_HANDLE(radv_sampler, sampler, image_info->sampler);
         }
      static ALWAYS_INLINE void
   write_sampler_descriptor(unsigned *dst, VkSampler _sampler)
   {
      RADV_FROM_HANDLE(radv_sampler, sampler, _sampler);
      }
      static ALWAYS_INLINE void
   write_accel_struct(struct radv_device *device, void *ptr, VkDeviceAddress va)
   {
      if (!va) {
      RADV_FROM_HANDLE(vk_acceleration_structure, accel_struct,
                        }
      static ALWAYS_INLINE void
   radv_update_descriptor_sets_impl(struct radv_device *device, struct radv_cmd_buffer *cmd_buffer,
                     {
      uint32_t i, j;
   for (i = 0; i < descriptorWriteCount; i++) {
      const VkWriteDescriptorSet *writeset = &pDescriptorWrites[i];
   RADV_FROM_HANDLE(radv_descriptor_set, set, dstSetOverride ? dstSetOverride : writeset->dstSet);
   const struct radv_descriptor_set_binding_layout *binding_layout =
         uint32_t *ptr = set->header.mapped_ptr;
   struct radeon_winsys_bo **buffer_list = set->descriptors;
   /* Immutable samplers are not copied into push descriptors when they are
   * allocated, so if we are writing push descriptors we have to copy the
   * immutable samplers into them now.
   */
   const bool copy_immutable_samplers =
         const uint32_t *samplers = radv_immutable_samplers(set->header.layout, binding_layout);
                     if (writeset->descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      write_block_descriptor(device, cmd_buffer, (uint8_t *)ptr + writeset->dstArrayElement, writeset);
      } else if (writeset->descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
                  ptr += binding_layout->size * writeset->dstArrayElement / 4;
   buffer_list += binding_layout->buffer_offset;
   buffer_list += writeset->dstArrayElement;
   for (j = 0; j < writeset->descriptorCount; ++j) {
      switch (writeset->descriptorType) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
      unsigned idx = writeset->dstArrayElement + j;
   idx += binding_layout->dynamic_offset_offset;
   assert(!(set->header.layout->flags & VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR));
   write_dynamic_buffer_descriptor(device, set->header.dynamic_descriptors + idx, buffer_list,
            }
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      write_buffer_descriptor_impl(device, cmd_buffer, ptr, buffer_list, writeset->pBufferInfo + j);
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      write_texel_buffer_descriptor(device, cmd_buffer, ptr, buffer_list, writeset->pTexelBufferView[j]);
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      write_image_descriptor_impl(device, cmd_buffer, 32, ptr, buffer_list, writeset->descriptorType,
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      write_image_descriptor_impl(device, cmd_buffer, 64, ptr, buffer_list, writeset->descriptorType,
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
      unsigned sampler_offset = radv_combined_image_descriptor_sampler_offset(binding_layout);
   write_combined_image_sampler_descriptor(device, cmd_buffer, sampler_offset, ptr, buffer_list,
               if (copy_immutable_samplers) {
      const unsigned idx = writeset->dstArrayElement + j;
      }
      }
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      if (!binding_layout->immutable_samplers_offset) {
      const VkDescriptorImageInfo *pImageInfo = writeset->pImageInfo + j;
      } else if (copy_immutable_samplers) {
      unsigned idx = writeset->dstArrayElement + j;
      }
                        write_accel_struct(device, ptr, accel_struct ? vk_acceleration_structure_get_va(accel_struct) : 0);
      }
   default:
         }
   ptr += binding_layout->size / 4;
                  for (i = 0; i < descriptorCopyCount; i++) {
      const VkCopyDescriptorSet *copyset = &pDescriptorCopies[i];
   RADV_FROM_HANDLE(radv_descriptor_set, src_set, copyset->srcSet);
   RADV_FROM_HANDLE(radv_descriptor_set, dst_set, copyset->dstSet);
   const struct radv_descriptor_set_binding_layout *src_binding_layout =
         const struct radv_descriptor_set_binding_layout *dst_binding_layout =
         uint32_t *src_ptr = src_set->header.mapped_ptr;
   uint32_t *dst_ptr = dst_set->header.mapped_ptr;
   struct radeon_winsys_bo **src_buffer_list = src_set->descriptors;
            src_ptr += src_binding_layout->offset / 4;
            if (src_binding_layout->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
                     memcpy(dst_ptr, src_ptr, copyset->descriptorCount);
               src_ptr += src_binding_layout->size * copyset->srcArrayElement / 4;
            src_buffer_list += src_binding_layout->buffer_offset;
            dst_buffer_list += dst_binding_layout->buffer_offset;
            /* In case of copies between mutable descriptor types
   * and non-mutable descriptor types. */
            for (j = 0; j < copyset->descriptorCount; ++j) {
      switch (src_binding_layout->type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
      unsigned src_idx = copyset->srcArrayElement + j;
   unsigned dst_idx = copyset->dstArrayElement + j;
   struct radv_descriptor_range *src_range, *dst_range;
                  src_range = src_set->header.dynamic_descriptors + src_idx;
   dst_range = dst_set->header.dynamic_descriptors + dst_idx;
   *dst_range = *src_range;
      }
   default:
         }
                  unsigned src_buffer_count = radv_descriptor_type_buffer_count(src_binding_layout->type);
   unsigned dst_buffer_count = radv_descriptor_type_buffer_count(dst_binding_layout->type);
   for (unsigned k = 0; k < dst_buffer_count; k++) {
      if (k < src_buffer_count)
         else
               dst_buffer_list += dst_buffer_count;
            }
      VKAPI_ATTR void VKAPI_CALL
   radv_UpdateDescriptorSets(VkDevice _device, uint32_t descriptorWriteCount,
               {
               radv_update_descriptor_sets_impl(device, NULL, VK_NULL_HANDLE, descriptorWriteCount, pDescriptorWrites,
      }
      void
   radv_cmd_update_descriptor_sets(struct radv_device *device, struct radv_cmd_buffer *cmd_buffer,
                     {
      /* Assume cmd_buffer != NULL to optimize out cmd_buffer checks in generic code above. */
   assume(cmd_buffer != NULL);
   radv_update_descriptor_sets_impl(device, cmd_buffer, dstSetOverride, descriptorWriteCount, pDescriptorWrites,
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateDescriptorUpdateTemplate(VkDevice _device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo,
               {
      RADV_FROM_HANDLE(radv_device, device, _device);
   const uint32_t entry_count = pCreateInfo->descriptorUpdateEntryCount;
   const size_t size = sizeof(struct radv_descriptor_update_template) +
         struct radv_descriptor_set_layout *set_layout = NULL;
   struct radv_descriptor_update_template *templ;
            templ = vk_alloc2(&device->vk.alloc, pAllocator, size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!templ)
                              if (pCreateInfo->templateType == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR) {
               /* descriptorSetLayout should be ignored for push descriptors
   * and instead it refers to pipelineLayout and set.
   */
   assert(pCreateInfo->set < MAX_SETS);
               } else {
      assert(pCreateInfo->templateType == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET);
               for (i = 0; i < entry_count; i++) {
      const VkDescriptorUpdateTemplateEntry *entry = &pCreateInfo->pDescriptorUpdateEntries[i];
   const struct radv_descriptor_set_binding_layout *binding_layout = set_layout->binding + entry->dstBinding;
   const uint32_t buffer_offset = binding_layout->buffer_offset + entry->dstArrayElement;
   const uint32_t *immutable_samplers = NULL;
   uint32_t dst_offset;
            /* dst_offset is an offset into dynamic_descriptors when the descriptor
         switch (entry->descriptorType) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      assert(pCreateInfo->templateType == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET);
   dst_offset = binding_layout->dynamic_offset_offset + entry->dstArrayElement;
   dst_stride = 0; /* Not used */
      default:
      switch (entry->descriptorType) {
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      /* Immutable samplers are copied into push descriptors when they are pushed */
   if (pCreateInfo->templateType == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR &&
      binding_layout->immutable_samplers_offset && !binding_layout->immutable_samplers_equal) {
      }
      default:
         }
   dst_offset = binding_layout->offset / 4;
   if (entry->descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK)
                        dst_stride = binding_layout->size / 4;
               templ->entry[i] = (struct radv_descriptor_update_template_entry){
      .descriptor_type = entry->descriptorType,
   .descriptor_count = entry->descriptorCount,
   .src_offset = entry->offset,
   .src_stride = entry->stride,
   .dst_offset = dst_offset,
   .dst_stride = dst_stride,
   .buffer_offset = buffer_offset,
   .has_sampler = !binding_layout->immutable_samplers_offset,
   .sampler_offset = radv_combined_image_descriptor_sampler_offset(binding_layout),
            *pDescriptorUpdateTemplate = radv_descriptor_update_template_to_handle(templ);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyDescriptorUpdateTemplate(VkDevice _device, VkDescriptorUpdateTemplate descriptorUpdateTemplate,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!templ)
            vk_object_base_finish(&templ->base);
      }
      static ALWAYS_INLINE void
   radv_update_descriptor_set_with_template_impl(struct radv_device *device, struct radv_cmd_buffer *cmd_buffer,
               {
      RADV_FROM_HANDLE(radv_descriptor_update_template, templ, descriptorUpdateTemplate);
            for (i = 0; i < templ->entry_count; ++i) {
      struct radeon_winsys_bo **buffer_list = set->descriptors + templ->entry[i].buffer_offset;
   uint32_t *pDst = set->header.mapped_ptr + templ->entry[i].dst_offset;
   const uint8_t *pSrc = ((const uint8_t *)pData) + templ->entry[i].src_offset;
            if (templ->entry[i].descriptor_type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      memcpy((uint8_t *)pDst, pSrc, templ->entry[i].descriptor_count);
               for (j = 0; j < templ->entry[i].descriptor_count; ++j) {
      switch (templ->entry[i].descriptor_type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
      const unsigned idx = templ->entry[i].dst_offset + j;
   assert(!(set->header.layout->flags & VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR));
   write_dynamic_buffer_descriptor(device, set->header.dynamic_descriptors + idx, buffer_list,
            }
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      write_buffer_descriptor_impl(device, cmd_buffer, pDst, buffer_list, (struct VkDescriptorBufferInfo *)pSrc);
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      write_texel_buffer_descriptor(device, cmd_buffer, pDst, buffer_list, *(VkBufferView *)pSrc);
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      write_image_descriptor_impl(device, cmd_buffer, 32, pDst, buffer_list, templ->entry[i].descriptor_type,
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      write_image_descriptor_impl(device, cmd_buffer, 64, pDst, buffer_list, templ->entry[i].descriptor_type,
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      write_combined_image_sampler_descriptor(device, cmd_buffer, templ->entry[i].sampler_offset, pDst,
               if (cmd_buffer && templ->entry[i].immutable_samplers) {
         }
      case VK_DESCRIPTOR_TYPE_SAMPLER:
      if (templ->entry[i].has_sampler) {
      const VkDescriptorImageInfo *pImageInfo = (struct VkDescriptorImageInfo *)pSrc;
      } else if (cmd_buffer && templ->entry[i].immutable_samplers)
            case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: {
      RADV_FROM_HANDLE(vk_acceleration_structure, accel_struct, *(const VkAccelerationStructureKHR *)pSrc);
   write_accel_struct(device, pDst, accel_struct ? vk_acceleration_structure_get_va(accel_struct) : 0);
      }
   default:
         }
   pSrc += templ->entry[i].src_stride;
   pDst += templ->entry[i].dst_stride;
            }
      void
   radv_cmd_update_descriptor_set_with_template(struct radv_device *device, struct radv_cmd_buffer *cmd_buffer,
               {
      /* Assume cmd_buffer != NULL to optimize out cmd_buffer checks in generic code above. */
   assume(cmd_buffer != NULL);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_UpdateDescriptorSetWithTemplate(VkDevice _device, VkDescriptorSet descriptorSet,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
               }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetDescriptorSetLayoutHostMappingInfoVALVE(VkDevice _device,
               {
      struct radv_descriptor_set_layout *set_layout =
                     pHostMapping->descriptorOffset = binding_layout->offset;
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetDescriptorSetHostMappingVALVE(VkDevice _device, VkDescriptorSet descriptorSet, void **ppData)
   {
      RADV_FROM_HANDLE(radv_descriptor_set, set, descriptorSet);
      }
      /* VK_EXT_descriptor_buffer */
   VKAPI_ATTR void VKAPI_CALL
   radv_GetDescriptorSetLayoutSizeEXT(VkDevice device, VkDescriptorSetLayout layout, VkDeviceSize *pLayoutSizeInBytes)
   {
      RADV_FROM_HANDLE(radv_descriptor_set_layout, set_layout, layout);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetDescriptorSetLayoutBindingOffsetEXT(VkDevice device, VkDescriptorSetLayout layout, uint32_t binding,
         {
      RADV_FROM_HANDLE(radv_descriptor_set_layout, set_layout, layout);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetDescriptorEXT(VkDevice _device, const VkDescriptorGetInfoEXT *pDescriptorInfo, size_t dataSize,
         {
               switch (pDescriptorInfo->type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER: {
      write_sampler_descriptor(pDescriptor, *pDescriptorInfo->data.pSampler);
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      write_image_descriptor(pDescriptor, 64, pDescriptorInfo->type, pDescriptorInfo->data.pCombinedImageSampler);
   if (pDescriptorInfo->data.pCombinedImageSampler) {
         }
      case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      write_image_descriptor(pDescriptor, 64, pDescriptorInfo->type, pDescriptorInfo->data.pInputAttachmentImage);
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      write_image_descriptor(pDescriptor, 64, pDescriptorInfo->type, pDescriptorInfo->data.pSampledImage);
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      write_image_descriptor(pDescriptor, 32, pDescriptorInfo->type, pDescriptorInfo->data.pStorageImage);
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
               write_buffer_descriptor(device, pDescriptor, addr_info ? addr_info->address : 0,
            }
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
               write_buffer_descriptor(device, pDescriptor, addr_info ? addr_info->address : 0,
            }
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: {
               if (addr_info && addr_info->address) {
      radv_make_texel_buffer_descriptor(device, addr_info->address, addr_info->format, 0, addr_info->range,
      } else {
         }
      }
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
               if (addr_info && addr_info->address) {
      radv_make_texel_buffer_descriptor(device, addr_info->address, addr_info->format, 0, addr_info->range,
      } else {
         }
      }
   case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
      write_accel_struct(device, pDescriptor, pDescriptorInfo->data.accelerationStructure);
      }
   default:
            }
