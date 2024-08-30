   /*
   * Copyright © 2021 Collabora Ltd.
   *
   * Derived from:
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "genxml/gen_macros.h"
      #include "panvk_private.h"
      #include <assert.h>
   #include <fcntl.h>
   #include <stdbool.h>
   #include <string.h>
   #include <unistd.h>
      #include "util/mesa-sha1.h"
   #include "vk_descriptor_update_template.h"
   #include "vk_descriptors.h"
   #include "vk_util.h"
      #include "pan_bo.h"
   #include "panvk_cs.h"
      #define PANVK_DESCRIPTOR_ALIGN 8
      struct panvk_bview_desc {
         };
      static void
   panvk_fill_bview_desc(struct panvk_bview_desc *desc,
         {
         }
      struct panvk_image_desc {
      uint16_t width;
   uint16_t height;
   uint16_t depth;
   uint8_t levels;
      };
      static void
   panvk_fill_image_desc(struct panvk_image_desc *desc,
         {
      desc->width = view->vk.extent.width - 1;
   desc->height = view->vk.extent.height - 1;
   desc->depth = view->vk.extent.depth - 1;
   desc->levels = view->vk.level_count;
            /* Stick array layer count after the last valid size component */
   if (view->vk.image->image_type == VK_IMAGE_TYPE_1D)
         else if (view->vk.image->image_type == VK_IMAGE_TYPE_2D)
      }
      VkResult
   panvk_per_arch(CreateDescriptorSetLayout)(
      VkDevice _device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
      {
      VK_FROM_HANDLE(panvk_device, device, _device);
   struct panvk_descriptor_set_layout *set_layout;
   VkDescriptorSetLayoutBinding *bindings = NULL;
   unsigned num_bindings = 0;
            if (pCreateInfo->bindingCount) {
      result = vk_create_sorted_bindings(pCreateInfo->pBindings,
         if (result != VK_SUCCESS)
                        unsigned num_immutable_samplers = 0;
   for (unsigned i = 0; i < pCreateInfo->bindingCount; i++) {
      if (bindings[i].pImmutableSamplers)
               size_t size =
      sizeof(*set_layout) +
   (sizeof(struct panvk_descriptor_set_binding_layout) * num_bindings) +
      set_layout = vk_descriptor_set_layout_zalloc(&device->vk, size);
   if (!set_layout) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               struct panvk_sampler **immutable_samplers =
      (struct panvk_sampler **)((uint8_t *)set_layout + sizeof(*set_layout) +
                              unsigned sampler_idx = 0, tex_idx = 0, ubo_idx = 0;
   unsigned dyn_ubo_idx = 0, dyn_ssbo_idx = 0, img_idx = 0;
            for (unsigned i = 0; i < pCreateInfo->bindingCount; i++) {
      const VkDescriptorSetLayoutBinding *binding = &bindings[i];
   struct panvk_descriptor_set_binding_layout *binding_layout =
            binding_layout->type = binding->descriptorType;
   binding_layout->array_size = binding->descriptorCount;
   binding_layout->shader_stages = binding->stageFlags;
   binding_layout->desc_ubo_stride = 0;
   if (binding->pImmutableSamplers) {
      binding_layout->immutable_samplers = immutable_samplers;
   immutable_samplers += binding_layout->array_size;
   for (unsigned j = 0; j < binding_layout->array_size; j++) {
      VK_FROM_HANDLE(panvk_sampler, sampler,
                        switch (binding_layout->type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      binding_layout->sampler_idx = sampler_idx;
   sampler_idx += binding_layout->array_size;
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      binding_layout->sampler_idx = sampler_idx;
   binding_layout->tex_idx = tex_idx;
   sampler_idx += binding_layout->array_size;
   tex_idx += binding_layout->array_size;
   binding_layout->desc_ubo_stride = sizeof(struct panvk_image_desc);
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      binding_layout->tex_idx = tex_idx;
   tex_idx += binding_layout->array_size;
   binding_layout->desc_ubo_stride = sizeof(struct panvk_image_desc);
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      binding_layout->tex_idx = tex_idx;
   tex_idx += binding_layout->array_size;
   binding_layout->desc_ubo_stride = sizeof(struct panvk_bview_desc);
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      binding_layout->dyn_ubo_idx = dyn_ubo_idx;
   dyn_ubo_idx += binding_layout->array_size;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      binding_layout->ubo_idx = ubo_idx;
   ubo_idx += binding_layout->array_size;
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      binding_layout->dyn_ssbo_idx = dyn_ssbo_idx;
   dyn_ssbo_idx += binding_layout->array_size;
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      binding_layout->desc_ubo_stride = sizeof(struct panvk_ssbo_addr);
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      binding_layout->img_idx = img_idx;
   img_idx += binding_layout->array_size;
   binding_layout->desc_ubo_stride = sizeof(struct panvk_image_desc);
      case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      binding_layout->img_idx = img_idx;
   img_idx += binding_layout->array_size;
   binding_layout->desc_ubo_stride = sizeof(struct panvk_bview_desc);
      default:
                  desc_ubo_size = ALIGN_POT(desc_ubo_size, PANVK_DESCRIPTOR_ALIGN);
   binding_layout->desc_ubo_offset = desc_ubo_size;
   desc_ubo_size +=
               set_layout->desc_ubo_size = desc_ubo_size;
   if (desc_ubo_size > 0)
            set_layout->num_samplers = sampler_idx;
   set_layout->num_textures = tex_idx;
   set_layout->num_ubos = ubo_idx;
   set_layout->num_dyn_ubos = dyn_ubo_idx;
   set_layout->num_dyn_ssbos = dyn_ssbo_idx;
            free(bindings);
   *pSetLayout = panvk_descriptor_set_layout_to_handle(set_layout);
         err_free_bindings:
      free(bindings);
      }
      static void panvk_write_sampler_desc_raw(struct panvk_descriptor_set *set,
                  static VkResult
   panvk_per_arch(descriptor_set_create)(
      struct panvk_device *device, struct panvk_descriptor_pool *pool,
   const struct panvk_descriptor_set_layout *layout,
      {
               /* TODO: Allocate from the pool! */
   set =
      vk_object_zalloc(&device->vk, NULL, sizeof(struct panvk_descriptor_set),
      if (!set)
                     if (layout->num_ubos) {
      set->ubos = vk_zalloc(&device->vk.alloc,
               if (!set->ubos)
               if (layout->num_dyn_ubos) {
      set->dyn_ubos = vk_zalloc(&device->vk.alloc,
               if (!set->dyn_ubos)
               if (layout->num_dyn_ssbos) {
      set->dyn_ssbos = vk_zalloc(
      &device->vk.alloc, sizeof(*set->dyn_ssbos) * layout->num_dyn_ssbos, 8,
      if (!set->dyn_ssbos)
               if (layout->num_samplers) {
      set->samplers =
      vk_zalloc(&device->vk.alloc, pan_size(SAMPLER) * layout->num_samplers,
      if (!set->samplers)
               if (layout->num_textures) {
      set->textures =
      vk_zalloc(&device->vk.alloc, pan_size(TEXTURE) * layout->num_textures,
      if (!set->textures)
               if (layout->num_imgs) {
      set->img_fmts =
      vk_zalloc(&device->vk.alloc, sizeof(*set->img_fmts) * layout->num_imgs,
      if (!set->img_fmts)
            set->img_attrib_bufs = vk_zalloc(
      &device->vk.alloc, pan_size(ATTRIBUTE_BUFFER) * 2 * layout->num_imgs,
      if (!set->img_attrib_bufs)
               if (layout->desc_ubo_size) {
      set->desc_bo =
      panfrost_bo_create(&device->physical_device->pdev,
      if (!set->desc_bo)
                     panvk_per_arch(emit_ubo)(set->desc_bo->ptr.gpu, layout->desc_ubo_size,
               for (unsigned i = 0; i < layout->binding_count; i++) {
      if (!layout->bindings[i].immutable_samplers)
            for (unsigned j = 0; j < layout->bindings[i].array_size; j++) {
      struct panvk_sampler *sampler =
                        *out_set = set;
         err_free_set:
      vk_free(&device->vk.alloc, set->textures);
   vk_free(&device->vk.alloc, set->samplers);
   vk_free(&device->vk.alloc, set->ubos);
   vk_free(&device->vk.alloc, set->dyn_ubos);
   vk_free(&device->vk.alloc, set->dyn_ssbos);
   vk_free(&device->vk.alloc, set->img_fmts);
   vk_free(&device->vk.alloc, set->img_attrib_bufs);
   if (set->desc_bo)
         vk_object_free(&device->vk, NULL, set);
      }
      VkResult
   panvk_per_arch(AllocateDescriptorSets)(
      VkDevice _device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
      {
      VK_FROM_HANDLE(panvk_device, device, _device);
   VK_FROM_HANDLE(panvk_descriptor_pool, pool, pAllocateInfo->descriptorPool);
   VkResult result;
            for (i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
      VK_FROM_HANDLE(panvk_descriptor_set_layout, layout,
                  result =
         if (result != VK_SUCCESS)
                              err_free_sets:
      panvk_FreeDescriptorSets(_device, pAllocateInfo->descriptorPool, i,
         for (i = 0; i < pAllocateInfo->descriptorSetCount; i++)
               }
      static void *
   panvk_desc_ubo_data(struct panvk_descriptor_set *set, uint32_t binding,
         {
      const struct panvk_descriptor_set_binding_layout *binding_layout =
            return (char *)set->desc_bo->ptr.cpu + binding_layout->desc_ubo_offset +
      }
      static struct mali_sampler_packed *
   panvk_sampler_desc(struct panvk_descriptor_set *set, uint32_t binding,
         {
      const struct panvk_descriptor_set_binding_layout *binding_layout =
                        }
      static void
   panvk_write_sampler_desc_raw(struct panvk_descriptor_set *set, uint32_t binding,
         {
      memcpy(panvk_sampler_desc(set, binding, elem), &sampler->desc,
      }
      static void
   panvk_write_sampler_desc(UNUSED struct panvk_device *dev,
                     {
      const struct panvk_descriptor_set_binding_layout *binding_layout =
            if (binding_layout->immutable_samplers)
            VK_FROM_HANDLE(panvk_sampler, sampler, pImageInfo->sampler);
      }
      static void
   panvk_copy_sampler_desc(struct panvk_descriptor_set *dst_set,
                     {
      const struct panvk_descriptor_set_binding_layout *dst_binding_layout =
            if (dst_binding_layout->immutable_samplers)
            memcpy(panvk_sampler_desc(dst_set, dst_binding, dst_elem),
            }
      static struct mali_texture_packed *
   panvk_tex_desc(struct panvk_descriptor_set *set, uint32_t binding,
         {
      const struct panvk_descriptor_set_binding_layout *binding_layout =
                        }
      static void
   panvk_write_tex_desc(UNUSED struct panvk_device *dev,
                     {
               memcpy(panvk_tex_desc(set, binding, elem), view->descs.tex,
               }
      static void
   panvk_copy_tex_desc(struct panvk_descriptor_set *dst_set, uint32_t dst_binding,
               {
      *panvk_tex_desc(dst_set, dst_binding, dst_elem) =
               }
      static void
   panvk_write_tex_buf_desc(UNUSED struct panvk_device *dev,
               {
               memcpy(panvk_tex_desc(set, binding, elem), view->descs.tex,
               }
      static uint32_t
   panvk_img_idx(struct panvk_descriptor_set *set, uint32_t binding, uint32_t elem)
   {
      const struct panvk_descriptor_set_binding_layout *binding_layout =
               }
      static void
   panvk_write_img_desc(struct panvk_device *dev, struct panvk_descriptor_set *set,
               {
      const struct panfrost_device *pdev = &dev->physical_device->pdev;
            unsigned img_idx = panvk_img_idx(set, binding, elem);
   void *attrib_buf = (uint8_t *)set->img_attrib_bufs +
            set->img_fmts[img_idx] = pdev->formats[view->pview.format].hw;
   memcpy(attrib_buf, view->descs.img_attrib_buf,
               }
      static void
   panvk_copy_img_desc(struct panvk_descriptor_set *dst_set, uint32_t dst_binding,
               {
      unsigned dst_img_idx = panvk_img_idx(dst_set, dst_binding, dst_elem);
            void *dst_attrib_buf = (uint8_t *)dst_set->img_attrib_bufs +
         void *src_attrib_buf = (uint8_t *)src_set->img_attrib_bufs +
            dst_set->img_fmts[dst_img_idx] = src_set->img_fmts[src_img_idx];
               }
      static void
   panvk_write_img_buf_desc(struct panvk_device *dev,
               {
      const struct panfrost_device *pdev = &dev->physical_device->pdev;
            unsigned img_idx = panvk_img_idx(set, binding, elem);
   void *attrib_buf = (uint8_t *)set->img_attrib_bufs +
            set->img_fmts[img_idx] = pdev->formats[view->fmt].hw;
   memcpy(attrib_buf, view->descs.img_attrib_buf,
               }
      static struct mali_uniform_buffer_packed *
   panvk_ubo_desc(struct panvk_descriptor_set *set, uint32_t binding,
         {
      const struct panvk_descriptor_set_binding_layout *binding_layout =
                        }
      static void
   panvk_write_ubo_desc(UNUSED struct panvk_device *dev,
               {
               mali_ptr ptr = panvk_buffer_gpu_ptr(buffer, pBufferInfo->offset);
   size_t size =
               }
      static void
   panvk_copy_ubo_desc(struct panvk_descriptor_set *dst_set, uint32_t dst_binding,
               {
      *panvk_ubo_desc(dst_set, dst_binding, dst_elem) =
      }
      static struct panvk_buffer_desc *
   panvk_dyn_ubo_desc(struct panvk_descriptor_set *set, uint32_t binding,
         {
      const struct panvk_descriptor_set_binding_layout *binding_layout =
               }
      static void
   panvk_write_dyn_ubo_desc(UNUSED struct panvk_device *dev,
                     {
               *panvk_dyn_ubo_desc(set, binding, elem) = (struct panvk_buffer_desc){
      .buffer = buffer,
   .offset = pBufferInfo->offset,
         }
      static void
   panvk_copy_dyn_ubo_desc(struct panvk_descriptor_set *dst_set,
                     {
      *panvk_dyn_ubo_desc(dst_set, dst_binding, dst_elem) =
      }
      static void
   panvk_write_ssbo_desc(UNUSED struct panvk_device *dev,
               {
               struct panvk_ssbo_addr *desc = panvk_desc_ubo_data(set, binding, elem);
   *desc = (struct panvk_ssbo_addr){
      .base_addr = panvk_buffer_gpu_ptr(buffer, pBufferInfo->offset),
   .size =
         }
      static void
   panvk_copy_ssbo_desc(struct panvk_descriptor_set *dst_set, uint32_t dst_binding,
               {
         }
      static struct panvk_buffer_desc *
   panvk_dyn_ssbo_desc(struct panvk_descriptor_set *set, uint32_t binding,
         {
      const struct panvk_descriptor_set_binding_layout *binding_layout =
               }
      static void
   panvk_write_dyn_ssbo_desc(UNUSED struct panvk_device *dev,
                     {
               *panvk_dyn_ssbo_desc(set, binding, elem) = (struct panvk_buffer_desc){
      .buffer = buffer,
   .offset = pBufferInfo->offset,
         }
      static void
   panvk_copy_dyn_ssbo_desc(struct panvk_descriptor_set *dst_set,
                     {
      *panvk_dyn_ssbo_desc(dst_set, dst_binding, dst_elem) =
      }
      void
   panvk_per_arch(UpdateDescriptorSets)(
      VkDevice _device, uint32_t descriptorWriteCount,
   const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount,
      {
               for (unsigned i = 0; i < descriptorWriteCount; i++) {
      const VkWriteDescriptorSet *write = &pDescriptorWrites[i];
            switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      panvk_write_sampler_desc(dev, set, write->dstBinding,
                        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      panvk_write_sampler_desc(dev, set, write->dstBinding,
               panvk_write_tex_desc(dev, set, write->dstBinding,
                        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      panvk_write_tex_desc(dev, set, write->dstBinding,
                        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      panvk_write_img_desc(dev, set, write->dstBinding,
                        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      panvk_write_tex_buf_desc(dev, set, write->dstBinding,
                        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      panvk_write_img_buf_desc(dev, set, write->dstBinding,
                        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      panvk_write_ubo_desc(dev, set, write->dstBinding,
                        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      panvk_write_dyn_ubo_desc(dev, set, write->dstBinding,
                        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      panvk_write_ssbo_desc(dev, set, write->dstBinding,
                        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      panvk_write_dyn_ssbo_desc(dev, set, write->dstBinding,
                        default:
                     for (unsigned i = 0; i < descriptorCopyCount; i++) {
      const VkCopyDescriptorSet *copy = &pDescriptorCopies[i];
   VK_FROM_HANDLE(panvk_descriptor_set, src_set, copy->srcSet);
            const struct panvk_descriptor_set_binding_layout *dst_binding_layout =
         const struct panvk_descriptor_set_binding_layout *src_binding_layout =
                     if (dst_binding_layout->desc_ubo_stride > 0 &&
      src_binding_layout->desc_ubo_stride > 0) {
   for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      memcpy(panvk_desc_ubo_data(dst_set, copy->dstBinding,
               panvk_desc_ubo_data(src_set, copy->srcBinding,
                        switch (src_binding_layout->type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      panvk_copy_sampler_desc(
      dst_set, copy->dstBinding, copy->dstArrayElement + j, src_set,
               case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      panvk_copy_sampler_desc(
      dst_set, copy->dstBinding, copy->dstArrayElement + j, src_set,
      panvk_copy_tex_desc(dst_set, copy->dstBinding,
                        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      panvk_copy_tex_desc(dst_set, copy->dstBinding,
                        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      panvk_copy_img_desc(dst_set, copy->dstBinding,
                        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      panvk_copy_ubo_desc(dst_set, copy->dstBinding,
                        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      panvk_copy_dyn_ubo_desc(
      dst_set, copy->dstBinding, copy->dstArrayElement + j, src_set,
               case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      panvk_copy_ssbo_desc(dst_set, copy->dstBinding,
                        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      panvk_copy_dyn_ssbo_desc(
      dst_set, copy->dstBinding, copy->dstArrayElement + j, src_set,
               default:
               }
      void
   panvk_per_arch(UpdateDescriptorSetWithTemplate)(
      VkDevice _device, VkDescriptorSet descriptorSet,
      {
      VK_FROM_HANDLE(panvk_device, dev, _device);
   VK_FROM_HANDLE(panvk_descriptor_set, set, descriptorSet);
   VK_FROM_HANDLE(vk_descriptor_update_template, template,
                     for (uint32_t i = 0; i < template->entry_count; i++) {
      const struct vk_descriptor_template_entry *entry = &template->entries[i];
   const struct panvk_descriptor_set_binding_layout *binding_layout =
            switch (entry->type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      for (unsigned j = 0; j < entry->array_count; j++) {
                     if ((entry->type == VK_DESCRIPTOR_TYPE_SAMPLER ||
                                                         panvk_write_tex_desc(dev, set, entry->binding,
                     case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      for (unsigned j = 0; j < entry->array_count; j++) {
                     panvk_write_img_desc(dev, set, entry->binding,
                  case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                        panvk_write_tex_buf_desc(dev, set, entry->binding,
                  case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                        panvk_write_img_buf_desc(dev, set, entry->binding,
                  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      for (unsigned j = 0; j < entry->array_count; j++) {
                     panvk_write_ubo_desc(dev, set, entry->binding,
                  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      for (unsigned j = 0; j < entry->array_count; j++) {
                     panvk_write_dyn_ubo_desc(dev, set, entry->binding,
                  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      for (unsigned j = 0; j < entry->array_count; j++) {
                     panvk_write_ssbo_desc(dev, set, entry->binding,
                  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      for (unsigned j = 0; j < entry->array_count; j++) {
                     panvk_write_dyn_ssbo_desc(dev, set, entry->binding,
      }
      default:
               }
