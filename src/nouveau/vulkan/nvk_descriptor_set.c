   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_descriptor_set.h"
      #include "nvk_buffer.h"
   #include "nvk_buffer_view.h"
   #include "nvk_descriptor_set_layout.h"
   #include "nvk_device.h"
   #include "nvk_entrypoints.h"
   #include "nvk_image_view.h"
   #include "nvk_physical_device.h"
   #include "nvk_sampler.h"
      #include "nouveau_bo.h"
      static inline uint32_t
   align_u32(uint32_t v, uint32_t a)
   {
      assert(a != 0 && a == (a & -a));
      }
      static inline void *
   desc_ubo_data(struct nvk_descriptor_set *set, uint32_t binding,
         {
      const struct nvk_descriptor_set_binding_layout *binding_layout =
            uint32_t offset = binding_layout->offset + elem * binding_layout->stride;
            if (size_out != NULL)
               }
      static void
   write_desc(struct nvk_descriptor_set *set, uint32_t binding, uint32_t elem,
         {
      ASSERTED uint32_t dst_size;
   void *dst = desc_ubo_data(set, binding, elem, &dst_size);
   assert(desc_size <= dst_size);
      }
      static void
   write_image_view_desc(struct nvk_descriptor_set *set,
                     {
      struct nvk_image_descriptor desc[3] = { };
            if (descriptor_type != VK_DESCRIPTOR_TYPE_SAMPLER &&
      info && info->imageView != VK_NULL_HANDLE) {
   VK_FROM_HANDLE(nvk_image_view, view, info->imageView);
   plane_count = view->plane_count;
   if (descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
      /* Storage images are always single plane */
                                 /* The nv50 compiler currently does some whacky stuff with images.
   * For now, just assert that we never do storage on 3D images and
   * that our descriptor index is at most 11 bits.
                     } else {
      for (uint8_t plane = 0; plane < plane_count; plane++) {
      assert(view->planes[plane].sampled_desc_index > 0);
   assert(view->planes[plane].sampled_desc_index < (1 << 20));
                     if (descriptor_type == VK_DESCRIPTOR_TYPE_SAMPLER ||
      descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
   const struct nvk_descriptor_set_binding_layout *binding_layout =
            struct nvk_sampler *sampler;
   if (binding_layout->immutable_samplers) {
         } else {
                           for (uint8_t plane = 0; plane < plane_count; plane++) {
      /* We need to replicate the last sampler plane out to all image
   * planes due to sampler table entry limitations. See
   * nvk_CreateSampler in nvk_sampler.c for more details.
   */
   uint8_t sampler_plane = MIN2(plane, sampler->plane_count - 1);
   assert(sampler->planes[sampler_plane].desc_index < (1 << 12));
         }
      }
      static void
   write_buffer_desc(struct nvk_descriptor_set *set,
               {
               const struct nvk_addr_range addr_range =
                  const struct nvk_buffer_address desc = {
      .base_addr = addr_range.addr,
      };
      }
      static void
   write_dynamic_buffer_desc(struct nvk_descriptor_set *set,
               {
      VK_FROM_HANDLE(nvk_buffer, buffer, info->buffer);
   const struct nvk_descriptor_set_binding_layout *binding_layout =
            const struct nvk_addr_range addr_range =
                  struct nvk_buffer_address *desc =
         *desc = (struct nvk_buffer_address){
      .base_addr = addr_range.addr,
         }
      static void
   write_buffer_view_desc(struct nvk_descriptor_set *set,
               {
      struct nvk_image_descriptor desc = { };
   if (bufferView != VK_NULL_HANDLE) {
               assert(view->desc_index < (1 << 20));
      }
      }
      static void
   write_inline_uniform_data(struct nvk_descriptor_set *set,
               {
      assert(set->layout->binding[binding].stride == 1);
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_UpdateDescriptorSets(VkDevice device,
                           {
      for (uint32_t w = 0; w < descriptorWriteCount; w++) {
      const VkWriteDescriptorSet *write = &pDescriptorWrites[w];
            switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      write_image_view_desc(set, write->pImageInfo + j,
                              case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      write_buffer_view_desc(set, write->pTexelBufferView[j],
                  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      write_buffer_desc(set, write->pBufferInfo + j, write->dstBinding,
                  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      write_dynamic_buffer_desc(set, write->pBufferInfo + j,
                        case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK: {
      const VkWriteDescriptorSetInlineUniformBlock *write_inline =
      vk_find_struct_const(write->pNext,
      assert(write_inline->dataSize == write->descriptorCount);
   write_inline_uniform_data(set, write_inline, write->dstBinding,
                     default:
                     for (uint32_t i = 0; i < descriptorCopyCount; i++) {
      const VkCopyDescriptorSet *copy = &pDescriptorCopies[i];
   VK_FROM_HANDLE(nvk_descriptor_set, src, copy->srcSet);
            const struct nvk_descriptor_set_binding_layout *src_binding_layout =
         const struct nvk_descriptor_set_binding_layout *dst_binding_layout =
            if (dst_binding_layout->stride > 0 && src_binding_layout->stride > 0) {
      for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      ASSERTED uint32_t dst_max_size, src_max_size;
   void *dst_map = desc_ubo_data(dst, copy->dstBinding,
               const void *src_map = desc_ubo_data(src, copy->srcBinding,
               const uint32_t copy_size = MIN2(dst_binding_layout->stride,
         assert(copy_size <= dst_max_size && copy_size <= src_max_size);
                  switch (src_binding_layout->type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
      const uint32_t dst_dyn_start =
         const uint32_t src_dyn_start =
         typed_memcpy(&dst->dynamic_buffers[dst_dyn_start],
                  }
   default:
               }
      void
   nvk_push_descriptor_set_update(struct nvk_push_descriptor_set *push_set,
                     {
      assert(layout->non_variable_descriptor_buffer_size < sizeof(push_set->data));
   struct nvk_descriptor_set set = {
      .layout = layout,
   .size = sizeof(push_set->data),
               for (uint32_t w = 0; w < write_count; w++) {
      const VkWriteDescriptorSet *write = &writes[w];
            switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      write_image_view_desc(&set, write->pImageInfo + j,
                              case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      write_buffer_view_desc(&set, write->pTexelBufferView[j],
                  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      write_buffer_desc(&set, write->pBufferInfo + j, write->dstBinding,
                  default:
               }
      static void
   nvk_descriptor_set_destroy(struct nvk_device *dev,
               {
      if (free_bo) {
      for (int i = 0; i < pool->entry_count; ++i) {
      if (pool->entries[i].set == set) {
      memmove(&pool->entries[i], &pool->entries[i + 1],
         --pool->entry_count;
                  if (pool->entry_count == 0)
                           }
      static void
   nvk_destroy_descriptor_pool(struct nvk_device *dev,
               {
      for (int i = 0; i < pool->entry_count; ++i) {
                  if (pool->bo) {
      nouveau_ws_bo_unmap(pool->bo, pool->mapped_ptr);
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateDescriptorPool(VkDevice _device,
                     {
      VK_FROM_HANDLE(nvk_device, dev, _device);
   struct nvk_descriptor_pool *pool;
   uint64_t size = sizeof(struct nvk_descriptor_pool);
            const VkMutableDescriptorTypeCreateInfoEXT *mutable_info =
      vk_find_struct_const(pCreateInfo->pNext,
         uint32_t max_align = 0;
   for (unsigned i = 0; i < pCreateInfo->poolSizeCount; ++i) {
      const VkMutableDescriptorTypeListEXT *type_list = NULL;
   if (pCreateInfo->pPoolSizes[i].type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT &&
                  uint32_t stride, align;
   nvk_descriptor_stride_align_for_type(pCreateInfo->pPoolSizes[i].type,
                     for (unsigned i = 0; i < pCreateInfo->poolSizeCount; ++i) {
      const VkMutableDescriptorTypeListEXT *type_list = NULL;
   if (pCreateInfo->pPoolSizes[i].type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT &&
                  uint32_t stride, align;
   nvk_descriptor_stride_align_for_type(pCreateInfo->pPoolSizes[i].type,
         bo_size += MAX2(stride, max_align) *
               /* Individual descriptor sets are aligned to the min UBO alignment to
   * ensure that we don't end up with unaligned data access in any shaders.
   * This means that each descriptor buffer allocated may burn up to 16B of
   * extra space to get the right alignment.  (Technically, it's at most 28B
   * because we're always going to start at least 4B aligned but we're being
   * conservative here.)  Allocate enough extra space that we can chop it
   * into maxSets pieces and align each one of them to 32B.
   */
            uint64_t entries_size = sizeof(struct nvk_descriptor_pool_entry) *
                  pool = vk_object_zalloc(&dev->vk, pAllocator, size,
         if (!pool)
            if (bo_size) {
      uint32_t flags = NOUVEAU_WS_BO_GART | NOUVEAU_WS_BO_MAP | NOUVEAU_WS_BO_NO_SHARE;
   pool->bo = nouveau_ws_bo_new(dev->ws_dev, bo_size, 0, flags);
   if (!pool->bo) {
      nvk_destroy_descriptor_pool(dev, pAllocator, pool);
      }
   pool->mapped_ptr = nouveau_ws_bo_map(pool->bo, NOUVEAU_WS_BO_WR);
   if (!pool->mapped_ptr) {
      nvk_destroy_descriptor_pool(dev, pAllocator, pool);
                  pool->size = bo_size;
            *pDescriptorPool = nvk_descriptor_pool_to_handle(pool);
      }
      static VkResult
   nvk_descriptor_set_create(struct nvk_device *dev,
                           {
               uint32_t mem_size = sizeof(struct nvk_descriptor_set) +
            set = vk_object_zalloc(&dev->vk, NULL, mem_size,
         if (!set)
            if (pool->entry_count == pool->max_entry_count)
                     if (layout->binding_count > 0 &&
      (layout->binding[layout->binding_count - 1].flags &
   VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT)) {
   uint32_t stride = layout->binding[layout->binding_count-1].stride;
               if (set->size > 0) {
      if (pool->current_offset + set->size > pool->size)
            set->mapped_ptr = (uint32_t *)(pool->mapped_ptr + pool->current_offset);
               pool->entries[pool->entry_count].offset = pool->current_offset;
   pool->entries[pool->entry_count].size = set->size;
   pool->entries[pool->entry_count].set = set;
   pool->current_offset += ALIGN(set->size, NVK_MIN_UBO_ALIGNMENT);
            vk_descriptor_set_layout_ref(&layout->vk);
            for (uint32_t b = 0; b < layout->binding_count; b++) {
      if (layout->binding[b].type != VK_DESCRIPTOR_TYPE_SAMPLER &&
                  if (layout->binding[b].immutable_samplers == NULL)
            uint32_t array_size = layout->binding[b].array_size;
   if (layout->binding[b].flags &
                  for (uint32_t j = 0; j < array_size; j++)
                           }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_AllocateDescriptorSets(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            VkResult result = VK_SUCCESS;
                     const VkDescriptorSetVariableDescriptorCountAllocateInfo *var_desc_count =
      vk_find_struct_const(pAllocateInfo->pNext,
         /* allocate a set of buffers for each shader to contain descriptors */
   for (i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
      VK_FROM_HANDLE(nvk_descriptor_set_layout, layout,
         /* If descriptorSetCount is zero or this structure is not included in
   * the pNext chain, then the variable lengths are considered to be zero.
   */
   const uint32_t variable_count =
                  result = nvk_descriptor_set_create(dev, pool, layout,
         if (result != VK_SUCCESS)
                        if (result != VK_SUCCESS) {
      nvk_FreeDescriptorSets(device, pAllocateInfo->descriptorPool, i, pDescriptorSets);
   for (i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
            }
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_FreeDescriptorSets(VkDevice device,
                     {
      VK_FROM_HANDLE(nvk_device, dev, device);
            for (uint32_t i = 0; i < descriptorSetCount; i++) {
               if (set)
      }
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_DestroyDescriptorPool(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            if (!_pool)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_ResetDescriptorPool(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            for (int i = 0; i < pool->entry_count; ++i) {
         }
   pool->entry_count = 0;
               }
      static void
   nvk_descriptor_set_write_template(struct nvk_descriptor_set *set,
               {
      for (uint32_t i = 0; i < template->entry_count; i++) {
      const struct vk_descriptor_template_entry *entry =
            switch (entry->type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      for (uint32_t j = 0; j < entry->array_count; j++) {
                     write_image_view_desc(set, info,
                              case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t j = 0; j < entry->array_count; j++) {
                     write_buffer_view_desc(set, *bview,
                        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      for (uint32_t j = 0; j < entry->array_count; j++) {
                     write_buffer_desc(set, info,
                        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      for (uint32_t j = 0; j < entry->array_count; j++) {
                     write_dynamic_buffer_desc(set, info,
                        case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      write_desc(set,
            entry->binding,
   entry->array_element,
            default:
               }
      VKAPI_ATTR void VKAPI_CALL
   nvk_UpdateDescriptorSetWithTemplate(VkDevice device,
                     {
      VK_FROM_HANDLE(nvk_descriptor_set, set, descriptorSet);
   VK_FROM_HANDLE(vk_descriptor_update_template, template,
               }
      void
   nvk_push_descriptor_set_update_template(
      struct nvk_push_descriptor_set *push_set,
   struct nvk_descriptor_set_layout *layout,
   const struct vk_descriptor_update_template *template,
      {
      struct nvk_descriptor_set tmp_set = {
      .layout = layout,
   .size = sizeof(push_set->data),
      };
      }
