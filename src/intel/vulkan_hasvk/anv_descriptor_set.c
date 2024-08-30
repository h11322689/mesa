   /*
   * Copyright © 2015 Intel Corporation
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
   #include <stdbool.h>
   #include <string.h>
   #include <unistd.h>
   #include <fcntl.h>
      #include "util/mesa-sha1.h"
   #include "vk_util.h"
      #include "anv_private.h"
      /*
   * Descriptor set layouts.
   */
      static enum anv_descriptor_data
   anv_descriptor_data_for_type(const struct anv_physical_device *device,
         {
               switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      data = ANV_DESCRIPTOR_SAMPLER_STATE;
   if (device->has_bindless_samplers)
               case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      data = ANV_DESCRIPTOR_SURFACE_STATE |
         if (device->has_bindless_samplers)
               case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      data = ANV_DESCRIPTOR_SURFACE_STATE;
         case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      data = ANV_DESCRIPTOR_SURFACE_STATE;
         case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      data = ANV_DESCRIPTOR_SURFACE_STATE;
   data |= ANV_DESCRIPTOR_IMAGE_PARAM;
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      data = ANV_DESCRIPTOR_SURFACE_STATE |
               case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      data = ANV_DESCRIPTOR_SURFACE_STATE;
         case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      data = ANV_DESCRIPTOR_INLINE_UNIFORM;
         default:
                  /* On gfx8 and above when we have softpin enabled, we also need to push
   * SSBO address ranges so that we can use A64 messages in the shader.
   */
   if (device->has_a64_buffer_access &&
      (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
   type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
   type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
   type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC))
         /* On Ivy Bridge and Bay Trail, we need swizzles textures in the shader
   * Do not handle VK_DESCRIPTOR_TYPE_STORAGE_IMAGE and
   * VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT because they already must
   * have identity swizzle.
   *
   * TODO: We need to handle swizzle on buffer views too for those same
   *       platforms.
   */
   if (device->info.verx10 == 70 &&
      (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
   type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER))
            }
      static enum anv_descriptor_data
   anv_descriptor_data_for_mutable_type(const struct anv_physical_device *device,
               {
               if (!mutable_info || mutable_info->mutableDescriptorTypeListCount == 0) {
      for(VkDescriptorType i = 0; i <= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; i++) {
      if (i == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
      i == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                                       const VkMutableDescriptorTypeListVALVE *type_list =
         for (uint32_t i = 0; i < type_list->descriptorTypeCount; i++) {
      desc_data |=
                  }
      static unsigned
   anv_descriptor_data_size(enum anv_descriptor_data data)
   {
               if (data & ANV_DESCRIPTOR_SAMPLED_IMAGE)
            if (data & ANV_DESCRIPTOR_STORAGE_IMAGE)
            if (data & ANV_DESCRIPTOR_IMAGE_PARAM)
            if (data & ANV_DESCRIPTOR_ADDRESS_RANGE)
            if (data & ANV_DESCRIPTOR_TEXTURE_SWIZZLE)
               }
      static bool
   anv_needs_descriptor_buffer(VkDescriptorType desc_type,
         {
      if (desc_type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK ||
      anv_descriptor_data_size(desc_data) > 0)
         }
      /** Returns the size in bytes of each descriptor with the given layout */
   static unsigned
   anv_descriptor_size(const struct anv_descriptor_set_binding_layout *layout)
   {
      if (layout->data & ANV_DESCRIPTOR_INLINE_UNIFORM) {
      assert(layout->data == ANV_DESCRIPTOR_INLINE_UNIFORM);
                        /* For multi-planar bindings, we make every descriptor consume the maximum
   * number of planes so we don't have to bother with walking arrays and
   * adding things up every time.  Fortunately, YCbCr samplers aren't all
   * that common and likely won't be in the middle of big arrays.
   */
   if (layout->max_plane_count > 1)
               }
      /** Returns size in bytes of the biggest descriptor in the given layout */
   static unsigned
   anv_descriptor_size_for_mutable_type(const struct anv_physical_device *device,
               {
               if (!mutable_info || mutable_info->mutableDescriptorTypeListCount == 0) {
                  if (i == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
      i == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
               enum anv_descriptor_data desc_data =
                                 const VkMutableDescriptorTypeListVALVE *type_list =
         for (uint32_t i = 0; i < type_list->descriptorTypeCount; i++) {
      enum anv_descriptor_data desc_data =
                        }
      static bool
   anv_descriptor_data_supports_bindless(const struct anv_physical_device *pdevice,
               {
      if (data & ANV_DESCRIPTOR_ADDRESS_RANGE) {
      assert(pdevice->has_a64_buffer_access);
               if (data & ANV_DESCRIPTOR_SAMPLED_IMAGE) {
      assert(pdevice->has_bindless_samplers);
                  }
      bool
   anv_descriptor_supports_bindless(const struct anv_physical_device *pdevice,
               {
      return anv_descriptor_data_supports_bindless(pdevice, binding->data,
      }
      bool
   anv_descriptor_requires_bindless(const struct anv_physical_device *pdevice,
               {
      if (pdevice->always_use_bindless)
            static const VkDescriptorBindingFlagBits flags_requiring_bindless =
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
   VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT |
            }
      void anv_GetDescriptorSetLayoutSupport(
      VkDevice                                    _device,
   const VkDescriptorSetLayoutCreateInfo*      pCreateInfo,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            uint32_t surface_count[MESA_VULKAN_SHADER_STAGES] = { 0, };
   VkDescriptorType varying_desc_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
            const VkDescriptorSetLayoutBindingFlagsCreateInfo *binding_flags_info =
      vk_find_struct_const(pCreateInfo->pNext,
      const VkMutableDescriptorTypeCreateInfoVALVE *mutable_info =
      vk_find_struct_const(pCreateInfo->pNext,
         for (uint32_t b = 0; b < pCreateInfo->bindingCount; b++) {
               VkDescriptorBindingFlags flags = 0;
   if (binding_flags_info && binding_flags_info->bindingCount > 0) {
      assert(binding_flags_info->bindingCount == pCreateInfo->bindingCount);
               enum anv_descriptor_data desc_data =
      binding->descriptorType == VK_DESCRIPTOR_TYPE_MUTABLE_VALVE ?
               if (anv_needs_descriptor_buffer(binding->descriptorType, desc_data))
            if (flags & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT)
            switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
                  case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
                  case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                     if (binding->pImmutableSamplers) {
      for (uint32_t i = 0; i < binding->descriptorCount; i++) {
      ANV_FROM_HANDLE(anv_sampler, sampler,
         anv_foreach_stage(s, binding->stageFlags)
         } else {
      anv_foreach_stage(s, binding->stageFlags)
                  default:
                     anv_foreach_stage(s, binding->stageFlags)
                        for (unsigned s = 0; s < ARRAY_SIZE(surface_count); s++) {
      if (needs_descriptor_buffer)
               VkDescriptorSetVariableDescriptorCountLayoutSupport *vdcls =
      vk_find_struct(pSupport->pNext,
      if (vdcls != NULL) {
      if (varying_desc_type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
         } else if (varying_desc_type != VK_DESCRIPTOR_TYPE_MAX_ENUM) {
         } else {
                     bool supported = true;
   for (unsigned s = 0; s < ARRAY_SIZE(surface_count); s++) {
      /* Our maximum binding table size is 240 and we need to reserve 8 for
   * render targets.
   */
   if (surface_count[s] > MAX_BINDING_TABLE_SIZE - MAX_RTS)
                  }
      VkResult anv_CreateDescriptorSetLayout(
      VkDevice                                    _device,
   const VkDescriptorSetLayoutCreateInfo*      pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
                        uint32_t num_bindings = 0;
   uint32_t immutable_sampler_count = 0;
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
   if ((desc_type == VK_DESCRIPTOR_TYPE_SAMPLER ||
      desc_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
   pCreateInfo->pBindings[j].pImmutableSamplers)
            /* We need to allocate descriptor set layouts off the device allocator
   * with DEVICE scope because they are reference counted and may not be
   * destroyed when vkDestroyDescriptorSetLayout is called.
   */
   VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct anv_descriptor_set_layout, set_layout, 1);
   VK_MULTIALLOC_DECL(&ma, struct anv_descriptor_set_binding_layout,
         VK_MULTIALLOC_DECL(&ma, struct anv_sampler *, samplers,
            if (!vk_object_multizalloc(&device->vk, &ma, NULL,
                  set_layout->ref_cnt = 1;
            for (uint32_t b = 0; b < num_bindings; b++) {
      /* Initialize all binding_layout entries to -1 */
            set_layout->binding[b].flags = 0;
   set_layout->binding[b].data = 0;
   set_layout->binding[b].max_plane_count = 0;
   set_layout->binding[b].array_size = 0;
               /* Initialize all samplers to 0 */
            uint32_t buffer_view_count = 0;
   uint32_t dynamic_offset_count = 0;
            for (uint32_t j = 0; j < pCreateInfo->bindingCount; j++) {
      const VkDescriptorSetLayoutBinding *binding = &pCreateInfo->pBindings[j];
   uint32_t b = binding->binding;
   /* We temporarily store pCreateInfo->pBindings[] index (plus one) in the
   * immutable_samplers pointer.  This provides us with a quick-and-dirty
   * way to sort the bindings by binding number.
   */
               const VkDescriptorSetLayoutBindingFlagsCreateInfo *binding_flags_info =
      vk_find_struct_const(pCreateInfo->pNext,
         const VkMutableDescriptorTypeCreateInfoVALVE *mutable_info =
      vk_find_struct_const(pCreateInfo->pNext,
         for (uint32_t b = 0; b < num_bindings; b++) {
      /* We stashed the pCreateInfo->pBindings[] index (plus one) in the
   * immutable_samplers pointer.  Check for NULL (empty binding) and then
   * reset it and compute the index.
   */
   if (set_layout->binding[b].immutable_samplers == NULL)
         const uint32_t info_idx =
                  const VkDescriptorSetLayoutBinding *binding =
            if (binding->descriptorCount == 0)
                     if (binding_flags_info && binding_flags_info->bindingCount > 0) {
      assert(binding_flags_info->bindingCount == pCreateInfo->bindingCount);
                  /* From the Vulkan spec:
   *
   *    "If VkDescriptorSetLayoutCreateInfo::flags includes
   *    VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR, then
   *    all elements of pBindingFlags must not include
   *    VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
   *    VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT, or
   *    VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT"
   */
   if (pCreateInfo->flags &
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR) {
   assert(!(set_layout->binding[b].flags &
      (VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
   VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT |
               set_layout->binding[b].data =
      binding->descriptorType == VK_DESCRIPTOR_TYPE_MUTABLE_VALVE ?
               set_layout->binding[b].array_size = binding->descriptorCount;
   set_layout->binding[b].descriptor_index = set_layout->descriptor_count;
            if (set_layout->binding[b].data & ANV_DESCRIPTOR_BUFFER_VIEW) {
      set_layout->binding[b].buffer_view_index = buffer_view_count;
               switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_MUTABLE_VALVE:
      set_layout->binding[b].max_plane_count = 1;
   if (binding->pImmutableSamplers) {
                     for (uint32_t i = 0; i < binding->descriptorCount; i++) {
                     set_layout->binding[b].immutable_samplers[i] = sampler;
   if (set_layout->binding[b].max_plane_count < sampler->n_planes)
                     case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                  default:
                  switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      set_layout->binding[b].dynamic_offset_index = dynamic_offset_count;
   set_layout->dynamic_offset_stages[dynamic_offset_count] = binding->stageFlags;
   dynamic_offset_count += binding->descriptorCount;
               default:
                  set_layout->binding[b].descriptor_stride =
      binding->descriptorType == VK_DESCRIPTOR_TYPE_MUTABLE_VALVE ?
               if (binding->descriptorType ==
      VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
   /* Inline uniform blocks are specified to use the descriptor array
   * size as the size in bytes of the block.
   */
   descriptor_buffer_size = align(descriptor_buffer_size,
         set_layout->binding[b].descriptor_offset = descriptor_buffer_size;
      } else {
      set_layout->binding[b].descriptor_offset = descriptor_buffer_size;
   descriptor_buffer_size +=
                           set_layout->buffer_view_count = buffer_view_count;
   set_layout->dynamic_offset_count = dynamic_offset_count;
                        }
      void
   anv_descriptor_set_layout_destroy(struct anv_device *device,
         {
      assert(layout->ref_cnt == 0);
      }
      static const struct anv_descriptor_set_binding_layout *
   set_layout_dynamic_binding(const struct anv_descriptor_set_layout *set_layout)
   {
      if (set_layout->binding_count == 0)
            const struct anv_descriptor_set_binding_layout *last_binding =
         if (!(last_binding->flags & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT))
               }
      static uint32_t
   set_layout_descriptor_count(const struct anv_descriptor_set_layout *set_layout,
         {
      const struct anv_descriptor_set_binding_layout *dynamic_binding =
         if (dynamic_binding == NULL)
            assert(var_desc_count <= dynamic_binding->array_size);
            if (dynamic_binding->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK)
               }
      static uint32_t
   set_layout_buffer_view_count(const struct anv_descriptor_set_layout *set_layout,
         {
      const struct anv_descriptor_set_binding_layout *dynamic_binding =
         if (dynamic_binding == NULL)
            assert(var_desc_count <= dynamic_binding->array_size);
            if (!(dynamic_binding->data & ANV_DESCRIPTOR_BUFFER_VIEW))
               }
      uint32_t
   anv_descriptor_set_layout_descriptor_buffer_size(const struct anv_descriptor_set_layout *set_layout,
         {
      const struct anv_descriptor_set_binding_layout *dynamic_binding =
         if (dynamic_binding == NULL)
            assert(var_desc_count <= dynamic_binding->array_size);
   uint32_t shrink = dynamic_binding->array_size - var_desc_count;
            if (dynamic_binding->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      /* Inline uniform blocks are specified to use the descriptor array
   * size as the size in bytes of the block.
   */
      } else {
      set_size = set_layout->descriptor_buffer_size -
                  }
      void anv_DestroyDescriptorSetLayout(
      VkDevice                                    _device,
   VkDescriptorSetLayout                       _set_layout,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!set_layout)
               }
      #define SHA1_UPDATE_VALUE(ctx, x) _mesa_sha1_update(ctx, &(x), sizeof(x));
      static void
   sha1_update_immutable_sampler(struct mesa_sha1 *ctx,
         {
      if (!sampler->conversion)
            /* The only thing that affects the shader is ycbcr conversion */
   _mesa_sha1_update(ctx, sampler->conversion,
      }
      static void
   sha1_update_descriptor_set_binding_layout(struct mesa_sha1 *ctx,
         {
      SHA1_UPDATE_VALUE(ctx, layout->flags);
   SHA1_UPDATE_VALUE(ctx, layout->data);
   SHA1_UPDATE_VALUE(ctx, layout->max_plane_count);
   SHA1_UPDATE_VALUE(ctx, layout->array_size);
   SHA1_UPDATE_VALUE(ctx, layout->descriptor_index);
   SHA1_UPDATE_VALUE(ctx, layout->dynamic_offset_index);
   SHA1_UPDATE_VALUE(ctx, layout->buffer_view_index);
            if (layout->immutable_samplers) {
      for (uint16_t i = 0; i < layout->array_size; i++)
         }
      static void
   sha1_update_descriptor_set_layout(struct mesa_sha1 *ctx,
         {
      SHA1_UPDATE_VALUE(ctx, layout->binding_count);
   SHA1_UPDATE_VALUE(ctx, layout->descriptor_count);
   SHA1_UPDATE_VALUE(ctx, layout->shader_stages);
   SHA1_UPDATE_VALUE(ctx, layout->buffer_view_count);
   SHA1_UPDATE_VALUE(ctx, layout->dynamic_offset_count);
            for (uint16_t i = 0; i < layout->binding_count; i++)
      }
      /*
   * Pipeline layouts.  These have nothing to do with the pipeline.  They are
   * just multiple descriptor set layouts pasted together
   */
      VkResult anv_CreatePipelineLayout(
      VkDevice                                    _device,
   const VkPipelineLayoutCreateInfo*           pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
                     layout = vk_object_alloc(&device->vk, pAllocator, sizeof(*layout),
         if (layout == NULL)
                              for (uint32_t set = 0; set < pCreateInfo->setLayoutCount; set++) {
      ANV_FROM_HANDLE(anv_descriptor_set_layout, set_layout,
         layout->set[set].layout = set_layout;
            layout->set[set].dynamic_offset_start = dynamic_offset_count;
      }
            struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);
   for (unsigned s = 0; s < layout->num_sets; s++) {
      sha1_update_descriptor_set_layout(&ctx, layout->set[s].layout);
   _mesa_sha1_update(&ctx, &layout->set[s].dynamic_offset_start,
      }
   _mesa_sha1_update(&ctx, &layout->num_sets, sizeof(layout->num_sets));
                        }
      void anv_DestroyPipelineLayout(
      VkDevice                                    _device,
   VkPipelineLayout                            _pipelineLayout,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!pipeline_layout)
            for (uint32_t i = 0; i < pipeline_layout->num_sets; i++)
               }
      /*
   * Descriptor pools.
   *
   * These are implemented using a big pool of memory and a free-list for the
   * host memory allocations and a state_stream and a free list for the buffer
   * view surface state. The spec allows us to fail to allocate due to
   * fragmentation in all cases but two: 1) after pool reset, allocating up
   * until the pool size with no freeing must succeed and 2) allocating and
   * freeing only descriptor sets with the same layout. Case 1) is easy enough,
   * and the free lists lets us recycle blocks for case 2).
   */
      /* The vma heap reserves 0 to mean NULL; we have to offset by some amount to
   * ensure we can allocate the entire BO without hitting zero.  The actual
   * amount doesn't matter.
   */
   #define POOL_HEAP_OFFSET 64
      #define EMPTY 1
      VkResult anv_CreateDescriptorPool(
      VkDevice                                    _device,
   const VkDescriptorPoolCreateInfo*           pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            const VkDescriptorPoolInlineUniformBlockCreateInfo *inline_info =
      vk_find_struct_const(pCreateInfo->pNext,
      const VkMutableDescriptorTypeCreateInfoVALVE *mutable_info =
      vk_find_struct_const(pCreateInfo->pNext,
         uint32_t descriptor_count = 0;
   uint32_t buffer_view_count = 0;
            for (uint32_t i = 0; i < pCreateInfo->poolSizeCount; i++) {
      enum anv_descriptor_data desc_data =
      pCreateInfo->pPoolSizes[i].type == VK_DESCRIPTOR_TYPE_MUTABLE_VALVE ?
               if (desc_data & ANV_DESCRIPTOR_BUFFER_VIEW)
            unsigned desc_data_size =
      pCreateInfo->pPoolSizes[i].type == VK_DESCRIPTOR_TYPE_MUTABLE_VALVE ?
                        /* Combined image sampler descriptors can take up to 3 slots if they
   * hold a YCbCr image.
   */
   if (pCreateInfo->pPoolSizes[i].type ==
                  if (pCreateInfo->pPoolSizes[i].type ==
      VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
   /* Inline uniform blocks are specified to use the descriptor array
   * size as the size in bytes of the block.
   */
   assert(inline_info);
                           }
   /* We have to align descriptor buffer allocations to 32B so that we can
   * push descriptor buffers.  This means that each descriptor buffer
   * allocated may burn up to 32B of extra space to get the right alignment.
   * (Technically, it's at most 28B because we're always going to start at
   * least 4B aligned but we're being conservative here.)  Allocate enough
   * extra space that we can chop it into maxSets pieces and align each one
   * of them to 32B.
   */
   descriptor_bo_size += ANV_UBO_ALIGNMENT * pCreateInfo->maxSets;
   /* We align inline uniform blocks to ANV_UBO_ALIGNMENT */
   if (inline_info) {
      descriptor_bo_size +=
      }
            const size_t pool_size =
      pCreateInfo->maxSets * sizeof(struct anv_descriptor_set) +
   descriptor_count * sizeof(struct anv_descriptor) +
               pool = vk_object_alloc(&device->vk, pAllocator, total_size,
         if (!pool)
            pool->size = pool_size;
   pool->next = 0;
   pool->free_list = EMPTY;
            if (descriptor_bo_size > 0) {
      VkResult result = anv_device_alloc_bo(device,
                                       if (result != VK_SUCCESS) {
      vk_object_free(&device->vk, pAllocator, pool);
                  } else {
                  anv_state_stream_init(&pool->surface_state_stream,
                                       }
      void anv_DestroyDescriptorPool(
      VkDevice                                    _device,
   VkDescriptorPool                            _pool,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!pool)
            list_for_each_entry_safe(struct anv_descriptor_set, set,
                        if (pool->bo) {
      util_vma_heap_finish(&pool->bo_heap);
      }
               }
      VkResult anv_ResetDescriptorPool(
      VkDevice                                    _device,
   VkDescriptorPool                            descriptorPool,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            list_for_each_entry_safe(struct anv_descriptor_set, set,
               }
            pool->next = 0;
            if (pool->bo) {
      util_vma_heap_finish(&pool->bo_heap);
               anv_state_stream_finish(&pool->surface_state_stream);
   anv_state_stream_init(&pool->surface_state_stream,
                     }
      struct pool_free_list_entry {
      uint32_t next;
      };
      static VkResult
   anv_descriptor_pool_alloc_set(struct anv_descriptor_pool *pool,
               {
      if (size <= pool->size - pool->next) {
      *set = (struct anv_descriptor_set *) (pool->data + pool->next);
   (*set)->size = size;
   pool->next += size;
      } else {
      struct pool_free_list_entry *entry;
   uint32_t *link = &pool->free_list;
   for (uint32_t f = pool->free_list; f != EMPTY; f = entry->next) {
      entry = (struct pool_free_list_entry *) (pool->data + f);
   if (size <= entry->size) {
      *link = entry->next;
   *set = (struct anv_descriptor_set *) entry;
   (*set)->size = entry->size;
      }
               if (pool->free_list != EMPTY) {
         } else {
               }
      static void
   anv_descriptor_pool_free_set(struct anv_descriptor_pool *pool,
         {
      /* Put the descriptor set allocation back on the free list. */
   const uint32_t index = (char *) set - pool->data;
   if (index + set->size == pool->next) {
         } else {
      struct pool_free_list_entry *entry = (struct pool_free_list_entry *) set;
   entry->next = pool->free_list;
   entry->size = set->size;
         }
      struct surface_state_free_list_entry {
      void *next;
      };
      static struct anv_state
   anv_descriptor_pool_alloc_state(struct anv_descriptor_pool *pool)
   {
               struct surface_state_free_list_entry *entry =
            if (entry) {
      struct anv_state state = entry->state;
   pool->surface_state_free_list = entry->next;
   assert(state.alloc_size == 64);
      } else {
            }
      static void
   anv_descriptor_pool_free_state(struct anv_descriptor_pool *pool,
         {
      assert(state.alloc_size);
   /* Put the buffer view surface state back on the free list. */
   struct surface_state_free_list_entry *entry = state.map;
   entry->next = pool->surface_state_free_list;
   entry->state = state;
      }
      size_t
   anv_descriptor_set_layout_size(const struct anv_descriptor_set_layout *layout,
         {
      const uint32_t descriptor_count =
         const uint32_t buffer_view_count =
            return sizeof(struct anv_descriptor_set) +
            }
      static VkResult
   anv_descriptor_set_create(struct anv_device *device,
                           {
      struct anv_descriptor_set *set;
            VkResult result = anv_descriptor_pool_alloc_set(pool, size, &set);
   if (result != VK_SUCCESS)
            uint32_t descriptor_buffer_size =
                     if (descriptor_buffer_size) {
      uint64_t pool_vma_offset =
      util_vma_heap_alloc(&pool->bo_heap, descriptor_buffer_size,
      if (pool_vma_offset == 0) {
      anv_descriptor_pool_free_set(pool, set);
      }
   assert(pool_vma_offset >= POOL_HEAP_OFFSET &&
         set->desc_mem.offset = pool_vma_offset - POOL_HEAP_OFFSET;
   set->desc_mem.alloc_size = descriptor_buffer_size;
            set->desc_addr = (struct anv_address) {
      .bo = pool->bo,
               enum isl_format format =
                  if (!pool->host_only) {
      set->desc_surface_state = anv_descriptor_pool_alloc_state(pool);
   anv_fill_buffer_surface_state(device, set->desc_surface_state,
                           } else {
      set->desc_mem = ANV_STATE_NULL;
               vk_object_base_init(&device->vk, &set->base,
         set->pool = pool;
   set->layout = layout;
            set->buffer_view_count =
         set->descriptor_count =
            set->buffer_views =
            /* By defining the descriptors to be zero now, we can later verify that
   * a descriptor has not been populated with user data.
   */
   memset(set->descriptors, 0,
            /* Go through and fill out immutable samplers if we have any */
   for (uint32_t b = 0; b < layout->binding_count; b++) {
      if (layout->binding[b].immutable_samplers) {
      for (uint32_t i = 0; i < layout->binding[b].array_size; i++) {
      /* The type will get changed to COMBINED_IMAGE_SAMPLER in
   * UpdateDescriptorSets if needed.  However, if the descriptor
   * set has an immutable sampler, UpdateDescriptorSets may never
   * touch it, so we need to make sure it's 100% valid now.
   *
   * We don't need to actually provide a sampler because the helper
   * will always write in the immutable sampler regardless of what
   * is in the sampler parameter.
   */
   VkDescriptorImageInfo info = { };
   anv_descriptor_set_write_image_view(device, set, &info,
                           /* Allocate null surface state for the buffer views since
   * we lazy allocate this in the write anyway.
   */
   if (!pool->host_only) {
      for (uint32_t b = 0; b < set->buffer_view_count; b++) {
      set->buffer_views[b].surface_state =
                                       }
      static void
   anv_descriptor_set_destroy(struct anv_device *device,
               {
               if (set->desc_mem.alloc_size) {
      util_vma_heap_free(&pool->bo_heap,
               if (set->desc_surface_state.alloc_size)
               if (!pool->host_only) {
      for (uint32_t b = 0; b < set->buffer_view_count; b++) {
      if (set->buffer_views[b].surface_state.alloc_size)
                           vk_object_base_finish(&set->base);
      }
      VkResult anv_AllocateDescriptorSets(
      VkDevice                                    _device,
   const VkDescriptorSetAllocateInfo*          pAllocateInfo,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            VkResult result = VK_SUCCESS;
   struct anv_descriptor_set *set = NULL;
            const VkDescriptorSetVariableDescriptorCountAllocateInfo *vdcai =
      vk_find_struct_const(pAllocateInfo->pNext,
         for (i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
      ANV_FROM_HANDLE(anv_descriptor_set_layout, layout,
            uint32_t var_desc_count = 0;
   if (vdcai != NULL && vdcai->descriptorSetCount > 0) {
      assert(vdcai->descriptorSetCount == pAllocateInfo->descriptorSetCount);
               result = anv_descriptor_set_create(device, pool, layout,
         if (result != VK_SUCCESS)
                        if (result != VK_SUCCESS)
      anv_FreeDescriptorSets(_device, pAllocateInfo->descriptorPool,
            }
      VkResult anv_FreeDescriptorSets(
      VkDevice                                    _device,
   VkDescriptorPool                            descriptorPool,
   uint32_t                                    count,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            for (uint32_t i = 0; i < count; i++) {
               if (!set)
                           }
      static void
   anv_descriptor_set_write_image_param(uint32_t *param_desc_map,
         {
   #define WRITE_PARAM_FIELD(field, FIELD) \
      for (unsigned i = 0; i < ARRAY_SIZE(param->field); i++) \
            WRITE_PARAM_FIELD(offset, OFFSET);
   WRITE_PARAM_FIELD(size, SIZE);
   WRITE_PARAM_FIELD(stride, STRIDE);
   WRITE_PARAM_FIELD(tiling, TILING);
   WRITE_PARAM_FIELD(swizzling, SWIZZLING);
         #undef WRITE_PARAM_FIELD
   }
      static uint32_t
   anv_surface_state_to_handle(struct anv_state state)
   {
      /* Bits 31:12 of the bindless surface offset in the extended message
   * descriptor is bits 25:6 of the byte-based address.
   */
   assert(state.offset >= 0);
   uint32_t offset = state.offset;
   assert((offset & 0x3f) == 0 && offset < (1 << 26));
      }
      void
   anv_descriptor_set_write_image_view(struct anv_device *device,
                                 {
      const struct anv_descriptor_set_binding_layout *bind_layout =
         struct anv_descriptor *desc =
         struct anv_image_view *image_view = NULL;
            /* We get called with just VK_DESCRIPTOR_TYPE_SAMPLER as part of descriptor
   * set initialization to set the bindless samplers.
   */
   assert(type == bind_layout->type ||
                  switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      sampler = bind_layout->immutable_samplers ?
                     case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      image_view = anv_image_view_from_handle(info->imageView);
   sampler = bind_layout->immutable_samplers ?
                     case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      image_view = anv_image_view_from_handle(info->imageView);
         default:
                  *desc = (struct anv_descriptor) {
      .type = type,
   .layout = info->imageLayout,
   .image_view = image_view,
               if (set->pool && set->pool->host_only)
            void *desc_map = set->desc_mem.map + bind_layout->descriptor_offset +
                  if (image_view == NULL && sampler == NULL)
            enum anv_descriptor_data data =
      bind_layout->type == VK_DESCRIPTOR_TYPE_MUTABLE_VALVE ?
   anv_descriptor_data_for_type(device->physical, type) :
            if (data & ANV_DESCRIPTOR_SAMPLED_IMAGE) {
      struct anv_sampled_image_descriptor desc_data[3];
            if (image_view) {
      for (unsigned p = 0; p < image_view->n_planes; p++) {
      struct anv_surface_state sstate =
      (desc->layout == VK_IMAGE_LAYOUT_GENERAL) ?
   image_view->planes[p].general_sampler_surface_state :
                     if (sampler) {
      for (unsigned p = 0; p < sampler->n_planes; p++)
               /* We may have max_plane_count < 0 if this isn't a sampled image but it
   * can be no more than the size of our array of handles.
   */
   assert(bind_layout->max_plane_count <= ARRAY_SIZE(desc_data));
   memcpy(desc_map, desc_data,
               if (image_view == NULL)
            if (data & ANV_DESCRIPTOR_STORAGE_IMAGE) {
      assert(!(data & ANV_DESCRIPTOR_IMAGE_PARAM));
   assert(image_view->n_planes == 1);
   struct anv_storage_image_descriptor desc_data = {
      .vanilla = anv_surface_state_to_handle(
         .lowered = anv_surface_state_to_handle(
      };
               if (data & ANV_DESCRIPTOR_IMAGE_PARAM) {
      /* Storage images can only ever have one plane */
   assert(image_view->n_planes == 1);
   const struct brw_image_param *image_param =
                        if (data & ANV_DESCRIPTOR_TEXTURE_SWIZZLE) {
      assert(!(data & ANV_DESCRIPTOR_SAMPLED_IMAGE));
   assert(image_view);
   struct anv_texture_swizzle_descriptor desc_data[3];
            for (unsigned p = 0; p < image_view->n_planes; p++) {
      desc_data[p] = (struct anv_texture_swizzle_descriptor) {
      .swizzle = {
      (uint8_t)image_view->planes[p].isl.swizzle.r,
   (uint8_t)image_view->planes[p].isl.swizzle.g,
   (uint8_t)image_view->planes[p].isl.swizzle.b,
            }
   memcpy(desc_map, desc_data,
         }
      void
   anv_descriptor_set_write_buffer_view(struct anv_device *device,
                                 {
      const struct anv_descriptor_set_binding_layout *bind_layout =
         struct anv_descriptor *desc =
            assert(type == bind_layout->type ||
            *desc = (struct anv_descriptor) {
      .type = type,
               if (set->pool && set->pool->host_only)
            enum anv_descriptor_data data =
      bind_layout->type == VK_DESCRIPTOR_TYPE_MUTABLE_VALVE ?
   anv_descriptor_data_for_type(device->physical, type) :
         void *desc_map = set->desc_mem.map + bind_layout->descriptor_offset +
            if (buffer_view == NULL) {
      memset(desc_map, 0, bind_layout->descriptor_stride);
               if (data & ANV_DESCRIPTOR_SAMPLED_IMAGE) {
      struct anv_sampled_image_descriptor desc_data = {
         };
               if (data & ANV_DESCRIPTOR_STORAGE_IMAGE) {
      assert(!(data & ANV_DESCRIPTOR_IMAGE_PARAM));
   struct anv_storage_image_descriptor desc_data = {
      .vanilla = anv_surface_state_to_handle(
         .lowered = anv_surface_state_to_handle(
      };
               if (data & ANV_DESCRIPTOR_IMAGE_PARAM) {
      anv_descriptor_set_write_image_param(desc_map,
         }
      void
   anv_descriptor_set_write_buffer(struct anv_device *device,
                                 struct anv_descriptor_set *set,
   struct anv_state_stream *alloc_stream,
   VkDescriptorType type,
   {
               const struct anv_descriptor_set_binding_layout *bind_layout =
         struct anv_descriptor *desc =
            assert(type == bind_layout->type ||
            *desc = (struct anv_descriptor) {
      .type = type,
   .offset = offset,
   .range = range,
               if (set->pool && set->pool->host_only)
            void *desc_map = set->desc_mem.map + bind_layout->descriptor_offset +
            if (buffer == NULL) {
      memset(desc_map, 0, bind_layout->descriptor_stride);
               struct anv_address bind_addr = anv_address_add(buffer->address, offset);
   uint64_t bind_range = vk_buffer_range(&buffer->vk, offset, range);
   enum anv_descriptor_data data =
      bind_layout->type == VK_DESCRIPTOR_TYPE_MUTABLE_VALVE ?
   anv_descriptor_data_for_type(device->physical, type) :
         /* We report a bounds checking alignment of 32B for the sake of block
   * messages which read an entire register worth at a time.
   */
   if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
      type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
         if (data & ANV_DESCRIPTOR_ADDRESS_RANGE) {
      struct anv_address_range_descriptor desc_data = {
      .address = anv_address_physical(bind_addr),
      };
               if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
      type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
         assert(data & ANV_DESCRIPTOR_BUFFER_VIEW);
   struct anv_buffer_view *bview =
            bview->range = bind_range;
            /* If we're writing descriptors through a push command, we need to
      * allocate the surface state from the command buffer. Otherwise it will
   * be allocated by the descriptor pool when calling
      if (alloc_stream) {
                           isl_surf_usage_flags_t usage =
      (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
   type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ?
   ISL_SURF_USAGE_CONSTANT_BUFFER_BIT :
         enum isl_format format = anv_isl_format_for_descriptor_type(device, type);
   anv_fill_buffer_surface_state(device, bview->surface_state,
                  }
      void
   anv_descriptor_set_write_inline_uniform_data(struct anv_device *device,
                                 {
      const struct anv_descriptor_set_binding_layout *bind_layout =
                                 }
      void anv_UpdateDescriptorSets(
      VkDevice                                    _device,
   uint32_t                                    descriptorWriteCount,
   const VkWriteDescriptorSet*                 pDescriptorWrites,
   uint32_t                                    descriptorCopyCount,
      {
               for (uint32_t i = 0; i < descriptorWriteCount; i++) {
      const VkWriteDescriptorSet *write = &pDescriptorWrites[i];
            switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      anv_descriptor_set_write_image_view(device, set,
                                    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
                     anv_descriptor_set_write_buffer_view(device, set,
                                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                        anv_descriptor_set_write_buffer(device, set,
                                 NULL,
                  case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK: {
      const VkWriteDescriptorSetInlineUniformBlock *inline_write =
      vk_find_struct_const(write->pNext,
      assert(inline_write->dataSize == write->descriptorCount);
   anv_descriptor_set_write_inline_uniform_data(device, set,
                                       default:
                     for (uint32_t i = 0; i < descriptorCopyCount; i++) {
      const VkCopyDescriptorSet *copy = &pDescriptorCopies[i];
   ANV_FROM_HANDLE(anv_descriptor_set, src, copy->srcSet);
            const struct anv_descriptor_set_binding_layout *src_layout =
         struct anv_descriptor *src_desc =
                  if (src_layout->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      anv_descriptor_set_write_inline_uniform_data(device, dst,
                                          /* Copy CPU side data */
   for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      switch(src_desc[j].type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
      VkDescriptorImageInfo info = {
      .sampler = anv_sampler_to_handle(src_desc[j].sampler),
   .imageView = anv_image_view_to_handle(src_desc[j].image_view),
      };
   anv_descriptor_set_write_image_view(device, dst,
                                       case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
      anv_descriptor_set_write_buffer_view(device, dst,
                                       case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
      anv_descriptor_set_write_buffer(device, dst,
                                 NULL,
   src_desc[j].type,
               default:
                  }
      /*
   * Descriptor update templates.
   */
      void
   anv_descriptor_set_write_template(struct anv_device *device,
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
      const VkDescriptorImageInfo *info =
         anv_descriptor_set_write_image_view(device, set,
                              case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t j = 0; j < entry->array_count; j++) {
      const VkBufferView *_bview =
                  anv_descriptor_set_write_buffer_view(device, set,
                                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      for (uint32_t j = 0; j < entry->array_count; j++) {
      const VkDescriptorBufferInfo *info =
                  anv_descriptor_set_write_buffer(device, set,
                                                case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      anv_descriptor_set_write_inline_uniform_data(device, set,
                                 default:
               }
      void anv_UpdateDescriptorSetWithTemplate(
      VkDevice                                    _device,
   VkDescriptorSet                             descriptorSet,
   VkDescriptorUpdateTemplate                  descriptorUpdateTemplate,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_descriptor_set, set, descriptorSet);
   VK_FROM_HANDLE(vk_descriptor_update_template, template,
               }
