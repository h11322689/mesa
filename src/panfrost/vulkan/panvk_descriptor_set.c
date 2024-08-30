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
   #include "panvk_private.h"
      #include <assert.h>
   #include <fcntl.h>
   #include <stdbool.h>
   #include <string.h>
   #include <unistd.h>
      #include "util/mesa-sha1.h"
   #include "vk_descriptors.h"
   #include "vk_util.h"
      #include "pan_bo.h"
      /* FIXME: make sure those values are correct */
   #define PANVK_MAX_TEXTURES (1 << 16)
   #define PANVK_MAX_IMAGES   (1 << 8)
   #define PANVK_MAX_SAMPLERS (1 << 16)
   #define PANVK_MAX_UBOS     255
      void
   panvk_GetDescriptorSetLayoutSupport(
      VkDevice _device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
      {
                        VkDescriptorSetLayoutBinding *bindings;
   VkResult result = vk_create_sorted_bindings(
         if (result != VK_SUCCESS) {
      vk_error(device, result);
               unsigned sampler_idx = 0, tex_idx = 0, ubo_idx = 0;
   unsigned img_idx = 0;
            for (unsigned i = 0; i < pCreateInfo->bindingCount; i++) {
               switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      sampler_idx += binding->descriptorCount;
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      sampler_idx += binding->descriptorCount;
   tex_idx += binding->descriptorCount;
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      tex_idx += binding->descriptorCount;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      dynoffset_idx += binding->descriptorCount;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      ubo_idx += binding->descriptorCount;
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      dynoffset_idx += binding->descriptorCount;
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
         case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      img_idx += binding->descriptorCount;
      default:
                     /* The maximum values apply to all sets attached to a pipeline since all
   * sets descriptors have to be merged in a single array.
   */
   if (tex_idx > PANVK_MAX_TEXTURES / MAX_SETS ||
      sampler_idx > PANVK_MAX_SAMPLERS / MAX_SETS ||
   ubo_idx > PANVK_MAX_UBOS / MAX_SETS ||
   img_idx > PANVK_MAX_IMAGES / MAX_SETS)
            }
      /*
   * Pipeline layouts.  These have nothing to do with the pipeline.  They are
   * just multiple descriptor set layouts pasted together.
   */
      VkResult
   panvk_CreatePipelineLayout(VkDevice _device,
                     {
      VK_FROM_HANDLE(panvk_device, device, _device);
   struct panvk_pipeline_layout *layout;
            layout =
         if (layout == NULL)
                     unsigned sampler_idx = 0, tex_idx = 0, ubo_idx = 0;
   unsigned dyn_ubo_idx = 0, dyn_ssbo_idx = 0, img_idx = 0;
   for (unsigned set = 0; set < pCreateInfo->setLayoutCount; set++) {
      const struct panvk_descriptor_set_layout *set_layout =
            layout->sets[set].sampler_offset = sampler_idx;
   layout->sets[set].tex_offset = tex_idx;
   layout->sets[set].ubo_offset = ubo_idx;
   layout->sets[set].dyn_ubo_offset = dyn_ubo_idx;
   layout->sets[set].dyn_ssbo_offset = dyn_ssbo_idx;
   layout->sets[set].img_offset = img_idx;
   sampler_idx += set_layout->num_samplers;
   tex_idx += set_layout->num_textures;
   ubo_idx += set_layout->num_ubos;
   dyn_ubo_idx += set_layout->num_dyn_ubos;
   dyn_ssbo_idx += set_layout->num_dyn_ssbos;
            for (unsigned b = 0; b < set_layout->binding_count; b++) {
                     if (binding_layout->immutable_samplers) {
      for (unsigned s = 0; s < binding_layout->array_size; s++) {
                           }
   _mesa_sha1_update(&ctx, &binding_layout->type,
         _mesa_sha1_update(&ctx, &binding_layout->array_size,
         _mesa_sha1_update(&ctx, &binding_layout->shader_stages,
                  for (unsigned range = 0; range < pCreateInfo->pushConstantRangeCount;
      range++) {
   layout->push_constants.size =
      MAX2(pCreateInfo->pPushConstantRanges[range].offset +
                  layout->num_samplers = sampler_idx;
   layout->num_textures = tex_idx;
   layout->num_ubos = ubo_idx;
   layout->num_dyn_ubos = dyn_ubo_idx;
   layout->num_dyn_ssbos = dyn_ssbo_idx;
            /* Some NIR texture operations don't require a sampler, but Bifrost/Midgard
   * ones always expect one. Add a dummy sampler to deal with this limitation.
   */
   if (layout->num_textures) {
      layout->num_samplers++;
   for (unsigned set = 0; set < pCreateInfo->setLayoutCount; set++)
                        *pPipelineLayout = panvk_pipeline_layout_to_handle(layout);
      }
      VkResult
   panvk_CreateDescriptorPool(VkDevice _device,
                     {
      VK_FROM_HANDLE(panvk_device, device, _device);
            pool = vk_object_zalloc(&device->vk, pAllocator,
               if (!pool)
                     for (unsigned i = 0; i < pCreateInfo->poolSizeCount; ++i) {
               switch (pCreateInfo->pPoolSizes[i].type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      pool->max.samplers += desc_count;
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      pool->max.combined_image_samplers += desc_count;
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      pool->max.sampled_images += desc_count;
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      pool->max.storage_images += desc_count;
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      pool->max.uniform_texel_bufs += desc_count;
      case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      pool->max.storage_texel_bufs += desc_count;
      case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      pool->max.input_attachments += desc_count;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      pool->max.uniform_bufs += desc_count;
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      pool->max.storage_bufs += desc_count;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      pool->max.uniform_dyn_bufs += desc_count;
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      pool->max.storage_dyn_bufs += desc_count;
      default:
                     *pDescriptorPool = panvk_descriptor_pool_to_handle(pool);
      }
      void
   panvk_DestroyDescriptorPool(VkDevice _device, VkDescriptorPool _pool,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
            if (pool)
      }
      VkResult
   panvk_ResetDescriptorPool(VkDevice _device, VkDescriptorPool _pool,
         {
      VK_FROM_HANDLE(panvk_descriptor_pool, pool, _pool);
   memset(&pool->cur, 0, sizeof(pool->cur));
      }
      static void
   panvk_descriptor_set_destroy(struct panvk_device *device,
               {
      vk_free(&device->vk.alloc, set->textures);
   vk_free(&device->vk.alloc, set->samplers);
   vk_free(&device->vk.alloc, set->ubos);
   vk_free(&device->vk.alloc, set->dyn_ubos);
   vk_free(&device->vk.alloc, set->dyn_ssbos);
   vk_free(&device->vk.alloc, set->img_fmts);
   vk_free(&device->vk.alloc, set->img_attrib_bufs);
   if (set->desc_bo)
            }
      VkResult
   panvk_FreeDescriptorSets(VkDevice _device, VkDescriptorPool descriptorPool,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
            for (unsigned i = 0; i < count; i++) {
               if (set)
      }
      }
      VkResult
   panvk_CreateSamplerYcbcrConversion(
      VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo,
   const VkAllocationCallbacks *pAllocator,
      {
      panvk_stub();
      }
      void
   panvk_DestroySamplerYcbcrConversion(VkDevice device,
               {
         }
