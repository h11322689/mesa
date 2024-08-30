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
      static unsigned
   anv_descriptor_data_alignment(enum anv_descriptor_data data)
   {
               if (data & (ANV_DESCRIPTOR_INDIRECT_SAMPLED_IMAGE |
                        if (data & (ANV_DESCRIPTOR_SURFACE |
                  if (data & ANV_DESCRIPTOR_SAMPLER)
            if (data & ANV_DESCRIPTOR_INLINE_UNIFORM)
               }
      static enum anv_descriptor_data
   anv_indirect_descriptor_data_for_type(VkDescriptorType type)
   {
               switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      data = ANV_DESCRIPTOR_BTI_SAMPLER_STATE |
               case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      data = ANV_DESCRIPTOR_BTI_SURFACE_STATE |
         ANV_DESCRIPTOR_BTI_SAMPLER_STATE |
         case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      data = ANV_DESCRIPTOR_BTI_SURFACE_STATE |
               case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      data = ANV_DESCRIPTOR_BTI_SURFACE_STATE |
               case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      data = ANV_DESCRIPTOR_BTI_SURFACE_STATE |
               case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      data = ANV_DESCRIPTOR_BTI_SURFACE_STATE;
         case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      data = ANV_DESCRIPTOR_INLINE_UNIFORM;
         case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
      data = ANV_DESCRIPTOR_INDIRECT_ADDRESS_RANGE;
         default:
                  /* We also need to push SSBO address ranges so that we can use A64
   * messages in the shader.
   */
   if (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
      type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
   type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
   type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
            }
      static enum anv_descriptor_data
   anv_direct_descriptor_data_for_type(VkDescriptorType type)
   {
               switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      data = ANV_DESCRIPTOR_BTI_SAMPLER_STATE |
               case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      data = ANV_DESCRIPTOR_BTI_SURFACE_STATE |
         ANV_DESCRIPTOR_BTI_SAMPLER_STATE |
         case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      data = ANV_DESCRIPTOR_BTI_SURFACE_STATE |
               case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      data = ANV_DESCRIPTOR_INLINE_UNIFORM;
         case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
      data = ANV_DESCRIPTOR_INDIRECT_ADDRESS_RANGE;
         default:
                     }
      static enum anv_descriptor_data
   anv_descriptor_data_for_type(const struct anv_physical_device *device,
         {
      if (device->indirect_descriptors)
         else
      }
      static enum anv_descriptor_data
   anv_descriptor_data_for_mutable_type(const struct anv_physical_device *device,
               {
               if (!mutable_info || mutable_info->mutableDescriptorTypeListCount == 0) {
      for(VkDescriptorType i = 0; i <= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; i++) {
      if (i == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
      i == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                           desc_data |= anv_descriptor_data_for_type(
                        const VkMutableDescriptorTypeListEXT *type_list =
         for (uint32_t i = 0; i < type_list->descriptorTypeCount; i++) {
      desc_data |=
                  }
      static unsigned
   anv_descriptor_data_size(enum anv_descriptor_data data)
   {
               if (data & ANV_DESCRIPTOR_INDIRECT_SAMPLED_IMAGE)
            if (data & ANV_DESCRIPTOR_INDIRECT_STORAGE_IMAGE)
            if (data & ANV_DESCRIPTOR_INDIRECT_ADDRESS_RANGE)
            if (data & ANV_DESCRIPTOR_SURFACE)
            if (data & ANV_DESCRIPTOR_SAMPLER)
            if (data & ANV_DESCRIPTOR_SURFACE_SAMPLER) {
      size += ALIGN(ANV_SURFACE_STATE_SIZE + ANV_SAMPLER_STATE_SIZE,
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
               if (!mutable_info ||
      mutable_info->mutableDescriptorTypeListCount == 0 ||
   binding >= mutable_info->mutableDescriptorTypeListCount) {
               if (i == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
      i == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
               enum anv_descriptor_data desc_data =
                     enum anv_descriptor_data desc_data = anv_descriptor_data_for_type(
                              const VkMutableDescriptorTypeListEXT *type_list =
         for (uint32_t i = 0; i < type_list->descriptorTypeCount; i++) {
      enum anv_descriptor_data desc_data =
                        }
      static bool
   anv_descriptor_data_supports_bindless(const struct anv_physical_device *pdevice,
               {
      if (pdevice->indirect_descriptors) {
      return data & (ANV_DESCRIPTOR_INDIRECT_ADDRESS_RANGE |
                     /* Directly descriptor support bindless for everything */
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
      const VkMutableDescriptorTypeCreateInfoEXT *mutable_info =
      vk_find_struct_const(pCreateInfo->pNext,
         for (uint32_t b = 0; b < pCreateInfo->bindingCount; b++) {
               VkDescriptorBindingFlags flags = 0;
   if (binding_flags_info && binding_flags_info->bindingCount > 0) {
      assert(binding_flags_info->bindingCount == pCreateInfo->bindingCount);
               enum anv_descriptor_data desc_data =
      binding->descriptorType == VK_DESCRIPTOR_TYPE_MUTABLE_EXT ?
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
   set_layout->binding_count = num_bindings;
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
         const VkMutableDescriptorTypeCreateInfoEXT *mutable_info =
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
      binding->descriptorType == VK_DESCRIPTOR_TYPE_MUTABLE_EXT ?
               set_layout->binding[b].array_size = binding->descriptorCount;
   set_layout->binding[b].descriptor_index = set_layout->descriptor_count;
            if (set_layout->binding[b].data & ANV_DESCRIPTOR_BUFFER_VIEW) {
      set_layout->binding[b].buffer_view_index = buffer_view_count;
               switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
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
                  set_layout->binding[b].descriptor_data_size =
         set_layout->binding[b].descriptor_stride =
      binding->descriptorType == VK_DESCRIPTOR_TYPE_MUTABLE_EXT ?
               descriptor_buffer_size =
                  if (binding->descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      set_layout->binding[b].descriptor_offset = descriptor_buffer_size;
      } else {
      set_layout->binding[b].descriptor_offset = descriptor_buffer_size;
   descriptor_buffer_size +=
                           set_layout->buffer_view_count = buffer_view_count;
   set_layout->dynamic_offset_count = dynamic_offset_count;
            if (pCreateInfo->flags & VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT)
         else if (device->physical->indirect_descriptors)
         else
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
      static bool
   anv_descriptor_set_layout_empty(const struct anv_descriptor_set_layout *set_layout)
   {
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
      if (!sampler->vk.ycbcr_conversion)
            /* The only thing that affects the shader is ycbcr conversion */
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
      SHA1_UPDATE_VALUE(ctx, layout->flags);
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
      void
   anv_pipeline_sets_layout_init(struct anv_pipeline_sets_layout *layout,
               {
               layout->device = device;
   layout->push_descriptor_set_index = -1;
      }
      void
   anv_pipeline_sets_layout_add(struct anv_pipeline_sets_layout *layout,
               {
      if (layout->set[set_idx].layout)
            /* Workaround CTS : Internal CTS issue 3584 */
   if (layout->independent_sets && anv_descriptor_set_layout_empty(set_layout))
            if (layout->type == ANV_PIPELINE_DESCRIPTOR_SET_LAYOUT_TYPE_UNKNOWN)
         else
                     layout->set[set_idx].layout =
            layout->set[set_idx].dynamic_offset_start = layout->num_dynamic_buffers;
                     if (set_layout->flags &
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR) {
   assert(layout->push_descriptor_set_index == -1);
         }
      void
   anv_pipeline_sets_layout_hash(struct anv_pipeline_sets_layout *layout)
   {
      struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);
   for (unsigned s = 0; s < layout->num_sets; s++) {
      if (!layout->set[s].layout)
         sha1_update_descriptor_set_layout(&ctx, layout->set[s].layout);
   _mesa_sha1_update(&ctx, &layout->set[s].dynamic_offset_start,
      }
   _mesa_sha1_update(&ctx, &layout->num_sets, sizeof(layout->num_sets));
      }
      void
   anv_pipeline_sets_layout_fini(struct anv_pipeline_sets_layout *layout)
   {
      for (unsigned s = 0; s < layout->num_sets; s++) {
      if (!layout->set[s].layout)
                  }
      void
   anv_pipeline_sets_layout_print(const struct anv_pipeline_sets_layout *layout)
   {
      fprintf(stderr, "layout: dyn_count=%u sets=%u ind=%u\n",
         layout->num_dynamic_buffers,
   layout->num_sets,
   for (unsigned s = 0; s < layout->num_sets; s++) {
      if (!layout->set[s].layout)
            fprintf(stderr, "   set%i: dyn_start=%u flags=0x%x\n",
         }
      VkResult anv_CreatePipelineLayout(
      VkDevice                                    _device,
   const VkPipelineLayoutCreateInfo*           pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
                     layout = vk_object_zalloc(&device->vk, pAllocator, sizeof(*layout),
         if (layout == NULL)
            anv_pipeline_sets_layout_init(&layout->sets_layout, device,
            for (uint32_t set = 0; set < pCreateInfo->setLayoutCount; set++) {
      ANV_FROM_HANDLE(anv_descriptor_set_layout, set_layout,
            /* VUID-VkPipelineLayoutCreateInfo-graphicsPipelineLibrary-06753
   *
   *    "If graphicsPipelineLibrary is not enabled, elements of
   *     pSetLayouts must be valid VkDescriptorSetLayout objects"
   *
   * As a result of supporting graphicsPipelineLibrary, we need to allow
   * null descriptor set layouts.
   */
   if (set_layout == NULL)
                                             }
      void anv_DestroyPipelineLayout(
      VkDevice                                    _device,
   VkPipelineLayout                            _pipelineLayout,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!layout)
                        }
      /*
   * Descriptor pools.
   *
   * These are implemented using a big pool of memory and a vma heap for the
   * host memory allocations and a state_stream and a free list for the buffer
   * view surface state. The spec allows us to fail to allocate due to
   * fragmentation in all cases but two: 1) after pool reset, allocating up
   * until the pool size with no freeing must succeed and 2) allocating and
   * freeing only descriptor sets with the same layout. Case 1) is easy enough,
   * and the vma heap ensures case 2).
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
      const VkMutableDescriptorTypeCreateInfoEXT *mutable_info =
      vk_find_struct_const(pCreateInfo->pNext,
         uint32_t descriptor_count = 0;
   uint32_t buffer_view_count = 0;
            for (uint32_t i = 0; i < pCreateInfo->poolSizeCount; i++) {
      enum anv_descriptor_data desc_data =
      pCreateInfo->pPoolSizes[i].type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT ?
               if (desc_data & ANV_DESCRIPTOR_BUFFER_VIEW)
            unsigned desc_data_size =
      pCreateInfo->pPoolSizes[i].type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT ?
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
            const bool host_only =
            /* For host_only pools, allocate some memory to hold the written surface
   * states of the internal anv_buffer_view. With normal pools, the memory
   * holding surface state is allocated from the device surface_state_pool.
   */
   const size_t host_mem_size =
      pCreateInfo->maxSets * sizeof(struct anv_descriptor_set) +
   descriptor_count * sizeof(struct anv_descriptor) +
   buffer_view_count * sizeof(struct anv_buffer_view) +
         pool = vk_object_zalloc(&device->vk, pAllocator,
               if (!pool)
            pool->bo_mem_size = descriptor_bo_size;
   pool->host_mem_size = host_mem_size;
                     if (pool->bo_mem_size > 0) {
      if (pool->host_only) {
      pool->host_bo = vk_zalloc(&device->vk.alloc, pool->bo_mem_size, 8,
         if (pool->host_bo == NULL) {
      vk_object_free(&device->vk, pAllocator, pool);
         } else {
      VkResult result = anv_device_alloc_bo(device,
                                       device->physical->indirect_descriptors ?
   "indirect descriptors" :
   "direct descriptors",
   descriptor_bo_size,
   if (result != VK_SUCCESS) {
      vk_object_free(&device->vk, pAllocator, pool);
         }
               /* All the surface states allocated by the descriptor pool are internal. We
   * have to allocate them to handle the fact that we do not have surface
   * states for VkBuffers.
   */
   anv_state_stream_init(&pool->surface_state_stream,
                                       }
      void anv_DestroyDescriptorPool(
      VkDevice                                    _device,
   VkDescriptorPool                            _pool,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!pool)
            list_for_each_entry_safe(struct anv_descriptor_set, set,
                                 if (pool->bo_mem_size) {
      if (pool->host_bo)
         if (pool->bo)
            }
               }
      VkResult anv_ResetDescriptorPool(
      VkDevice                                    _device,
   VkDescriptorPool                            descriptorPool,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            list_for_each_entry_safe(struct anv_descriptor_set, set,
               }
            util_vma_heap_finish(&pool->host_heap);
            if (pool->bo_mem_size) {
      util_vma_heap_finish(&pool->bo_heap);
               anv_state_stream_finish(&pool->surface_state_stream);
   anv_state_stream_init(&pool->surface_state_stream,
                     }
      static VkResult
   anv_descriptor_pool_alloc_set(struct anv_descriptor_pool *pool,
               {
               if (vma_offset == 0) {
      if (size <= pool->host_heap.free_size) {
         } else {
                     assert(vma_offset >= POOL_HEAP_OFFSET);
            *set = (struct anv_descriptor_set *) (pool->host_mem + host_mem_offset);
               }
      static void
   anv_descriptor_pool_free_set(struct anv_descriptor_pool *pool,
         {
      util_vma_heap_free(&pool->host_heap,
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
   assert(state.alloc_size == ANV_SURFACE_STATE_SIZE);
      } else {
      struct anv_state state =
      anv_state_stream_alloc(&pool->surface_state_stream,
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
         descriptor_count * sizeof(struct anv_descriptor) +
      }
      static VkResult
   anv_descriptor_set_create(struct anv_device *device,
                           {
      struct anv_descriptor_set *set;
   const size_t size = anv_descriptor_set_layout_size(layout,
                  VkResult result = anv_descriptor_pool_alloc_set(pool, size, &set);
   if (result != VK_SUCCESS)
            uint32_t descriptor_buffer_size =
            set->desc_surface_state = ANV_STATE_NULL;
            if (descriptor_buffer_size) {
      uint64_t pool_vma_offset =
      util_vma_heap_alloc(&pool->bo_heap, descriptor_buffer_size,
      if (pool_vma_offset == 0) {
      anv_descriptor_pool_free_set(pool, set);
      }
   assert(pool_vma_offset >= POOL_HEAP_OFFSET &&
         set->desc_mem.offset = pool_vma_offset - POOL_HEAP_OFFSET;
            if (pool->host_only)
         else
            set->desc_addr = (struct anv_address) {
      .bo = pool->bo,
      };
   set->desc_offset = anv_address_physical(set->desc_addr) -
            enum isl_format format =
                  if (!pool->host_only) {
      set->desc_surface_state = anv_descriptor_pool_alloc_state(pool);
   anv_fill_buffer_surface_state(device, set->desc_surface_state.map,
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
                           /* Allocate surface states for real descriptor sets if we're using indirect
   * descriptors. For host only sets, we just store the surface state data in
   * malloc memory.
   */
   if (device->physical->indirect_descriptors) {
      if (!pool->host_only) {
      for (uint32_t b = 0; b < set->buffer_view_count; b++) {
      set->buffer_views[b].general.state =
         } else {
      void *host_surface_states =
         memset(host_surface_states, 0,
         for (uint32_t b = 0; b < set->buffer_view_count; b++) {
      set->buffer_views[b].general.state = (struct anv_state) {
      .alloc_size = ANV_SURFACE_STATE_SIZE,
                                             }
      static void
   anv_descriptor_set_destroy(struct anv_device *device,
               {
               if (set->desc_mem.alloc_size) {
      util_vma_heap_free(&pool->bo_heap,
               if (set->desc_surface_state.alloc_size)
               if (device->physical->indirect_descriptors) {
      if (!pool->host_only) {
      for (uint32_t b = 0; b < set->buffer_view_count; b++) {
      if (set->buffer_views[b].general.state.alloc_size) {
      anv_descriptor_pool_free_state(
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
                        if (result != VK_SUCCESS) {
      anv_FreeDescriptorSets(_device, pAllocateInfo->descriptorPool,
         /* The Vulkan 1.3.228 spec, section 14.2.3. Allocation of Descriptor Sets:
   *
   *   "If the creation of any of those descriptor sets fails, then the
   *    implementation must destroy all successfully created descriptor
   *    set objects from this command, set all entries of the
   *    pDescriptorSets array to VK_NULL_HANDLE and return the error."
   */
   for (i = 0; i < pAllocateInfo->descriptorSetCount; i++)
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
      void
   anv_push_descriptor_set_init(struct anv_cmd_buffer *cmd_buffer,
               {
               if (set->layout != layout) {
      if (set->layout) {
         } else {
      /* one-time initialization */
   vk_object_base_init(&cmd_buffer->device->vk, &set->base,
         set->is_push = true;
               anv_descriptor_set_layout_ref(layout);
   set->layout = layout;
               assert(set->is_push && set->buffer_views);
   set->size = anv_descriptor_set_layout_size(layout, false /* host_only */, 0);
   set->buffer_view_count = layout->buffer_view_count;
            if (layout->descriptor_buffer_size &&
      (push_set->set_used_on_gpu ||
   set->desc_mem.alloc_size < layout->descriptor_buffer_size)) {
   struct anv_physical_device *pdevice = cmd_buffer->device->physical;
   struct anv_state_stream *push_stream =
      pdevice->indirect_descriptors ?
   &cmd_buffer->push_descriptor_stream :
      uint64_t push_base_address = pdevice->indirect_descriptors ?
                  /* The previous buffer is either actively used by some GPU command (so
   * we can't modify it) or is too small.  Allocate a new one.
   */
   struct anv_state desc_mem =
      anv_state_stream_alloc(push_stream,
            if (set->desc_mem.alloc_size) {
      /* TODO: Do we really need to copy all the time? */
   memcpy(desc_mem.map, set->desc_mem.map,
      }
            set->desc_addr = anv_state_pool_state_address(
      push_stream->state_pool,
      set->desc_offset = anv_address_physical(set->desc_addr) -
         }
      void
   anv_push_descriptor_set_finish(struct anv_push_descriptor_set *push_set)
   {
      struct anv_descriptor_set *set = &push_set->set;
   if (set->layout) {
      struct anv_device *device =
               }
      static uint32_t
   anv_surface_state_to_handle(struct anv_physical_device *device,
         {
      /* Bits 31:12 of the bindless surface offset in the extended message
   * descriptor is bits 25:6 of the byte-based address.
   */
   assert(state.offset >= 0);
   uint32_t offset = state.offset;
   if (device->uses_ex_bso) {
      assert((offset & 0x3f) == 0);
      } else {
      assert((offset & 0x3f) == 0 && offset < (1 << 26));
         }
      static const void *
   anv_image_view_surface_data_for_plane_layout(struct anv_image_view *image_view,
                     {
      if (desc_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
      desc_type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
   desc_type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
   return layout == VK_IMAGE_LAYOUT_GENERAL ?
      &image_view->planes[plane].general_sampler.state_data :
            if (desc_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
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
               void *desc_map = set->desc_mem.map + bind_layout->descriptor_offset +
            enum anv_descriptor_data data =
      bind_layout->type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT ?
   anv_descriptor_data_for_type(device->physical, type) :
         if (data & ANV_DESCRIPTOR_INDIRECT_SAMPLED_IMAGE) {
      struct anv_sampled_image_descriptor desc_data[3];
            if (image_view) {
      for (unsigned p = 0; p < image_view->n_planes; p++) {
      const struct anv_surface_state *sstate =
      (desc->layout == VK_IMAGE_LAYOUT_GENERAL) ?
   &image_view->planes[p].general_sampler :
      desc_data[p].image =
                  if (sampler) {
      for (unsigned p = 0; p < sampler->n_planes; p++)
               /* We may have max_plane_count < 0 if this isn't a sampled image but it
   * can be no more than the size of our array of handles.
   */
   assert(bind_layout->max_plane_count <= ARRAY_SIZE(desc_data));
   memcpy(desc_map, desc_data,
               if (data & ANV_DESCRIPTOR_INDIRECT_STORAGE_IMAGE) {
      if (image_view) {
      assert(image_view->n_planes == 1);
   struct anv_storage_image_descriptor desc_data = {
      .vanilla = anv_surface_state_to_handle(
      device->physical,
         };
      } else {
                     if (data & ANV_DESCRIPTOR_SAMPLER) {
      if (sampler) {
      for (unsigned p = 0; p < sampler->n_planes; p++) {
      memcpy(desc_map + p * ANV_SAMPLER_STATE_SIZE,
         } else {
                     if (data & ANV_DESCRIPTOR_SURFACE) {
               for (unsigned p = 0; p < max_plane_count; p++) {
               if (image_view) {
      memcpy(plane_map,
         anv_image_view_surface_data_for_plane_layout(image_view, type,
      } else {
                        if (data & ANV_DESCRIPTOR_SURFACE_SAMPLER) {
      unsigned max_plane_count =
                  for (unsigned p = 0; p < max_plane_count; p++) {
               if (image_view) {
      memcpy(plane_map,
         anv_image_view_surface_data_for_plane_layout(image_view, type,
      } else {
                  if (sampler) {
      memcpy(plane_map + ANV_SURFACE_STATE_SIZE,
      } else {
      memset(plane_map + ANV_SURFACE_STATE_SIZE, 0,
               }
      static const void *
   anv_buffer_view_surface_data(struct anv_buffer_view *buffer_view,
         {
      if (desc_type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
            if (desc_type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
               }
      void
   anv_descriptor_set_write_buffer_view(struct anv_device *device,
                                 {
      const struct anv_descriptor_set_binding_layout *bind_layout =
         struct anv_descriptor *desc =
            assert(type == bind_layout->type ||
            *desc = (struct anv_descriptor) {
      .type = type,
               enum anv_descriptor_data data =
      bind_layout->type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT ?
   anv_descriptor_data_for_type(device->physical, type) :
         void *desc_map = set->desc_mem.map + bind_layout->descriptor_offset +
            if (buffer_view == NULL) {
      if (data & ANV_DESCRIPTOR_SURFACE)
         else
                     if (data & ANV_DESCRIPTOR_INDIRECT_SAMPLED_IMAGE) {
      struct anv_sampled_image_descriptor desc_data = {
      .image = anv_surface_state_to_handle(
      };
               if (data & ANV_DESCRIPTOR_INDIRECT_STORAGE_IMAGE) {
      struct anv_storage_image_descriptor desc_data = {
      .vanilla = anv_surface_state_to_handle(
      };
               if (data & ANV_DESCRIPTOR_SURFACE) {
      memcpy(desc_map,
               }
      void
   anv_descriptor_write_surface_state(struct anv_device *device,
               {
                                 isl_surf_usage_flags_t usage =
      (desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
   desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ?
   ISL_SURF_USAGE_CONSTANT_BUFFER_BIT :
         enum isl_format format =
         anv_fill_buffer_surface_state(device, bview->general.state.map,
            }
      void
   anv_descriptor_set_write_buffer(struct anv_device *device,
                                 struct anv_descriptor_set *set,
   VkDescriptorType type,
   {
      const struct anv_descriptor_set_binding_layout *bind_layout =
         const uint32_t descriptor_index = bind_layout->descriptor_index + element;
            assert(type == bind_layout->type ||
            *desc = (struct anv_descriptor) {
      .type = type,
   .offset = offset,
   .range = range,
               enum anv_descriptor_data data =
      bind_layout->type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT ?
   anv_descriptor_data_for_type(device->physical, type) :
         void *desc_map = set->desc_mem.map + bind_layout->descriptor_offset +
            if (buffer == NULL) {
      if (data & ANV_DESCRIPTOR_SURFACE)
         else
                     struct anv_address bind_addr = anv_address_add(buffer->address, offset);
            /* We report a bounds checking alignment of 32B for the sake of block
   * messages which read an entire register worth at a time.
   */
   if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
      type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
         if (data & ANV_DESCRIPTOR_INDIRECT_ADDRESS_RANGE) {
      struct anv_address_range_descriptor desc_data = {
      .address = anv_address_physical(bind_addr),
      };
               if (data & ANV_DESCRIPTOR_SURFACE) {
      isl_surf_usage_flags_t usage =
      desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ?
               enum isl_format format =
            isl_buffer_fill_state(&device->isl_dev, desc_map,
                        .address = anv_address_physical(bind_addr),
   .mocs = isl_mocs(&device->isl_dev, usage,
                  if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
      type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
         if (data & ANV_DESCRIPTOR_BUFFER_VIEW) {
      struct anv_buffer_view *bview =
                     bview->vk.range = desc->bind_range;
            if (set->is_push)
         else
         }
      void
   anv_descriptor_set_write_inline_uniform_data(struct anv_device *device,
                                 {
      const struct anv_descriptor_set_binding_layout *bind_layout =
                                 }
      void
   anv_descriptor_set_write_acceleration_structure(struct anv_device *device,
                           {
      const struct anv_descriptor_set_binding_layout *bind_layout =
         struct anv_descriptor *desc =
            assert(bind_layout->data & ANV_DESCRIPTOR_INDIRECT_ADDRESS_RANGE);
   *desc = (struct anv_descriptor) {
      .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
               struct anv_address_range_descriptor desc_data = { };
   if (accel != NULL) {
      desc_data.address = vk_acceleration_structure_get_va(accel);
      }
            void *desc_map = set->desc_mem.map + bind_layout->descriptor_offset +
            }
      void
   anv_descriptor_set_write(struct anv_device *device,
                     {
      for (uint32_t i = 0; i < write_count; i++) {
      const VkWriteDescriptorSet *write = &writes[i];
   struct anv_descriptor_set *set = unlikely(set_override) ?
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
                                                case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK: {
      const VkWriteDescriptorSetInlineUniformBlock *inline_write =
      vk_find_struct_const(write->pNext,
      assert(inline_write->dataSize == write->descriptorCount);
   anv_descriptor_set_write_inline_uniform_data(device, set,
                                       case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: {
      const VkWriteDescriptorSetAccelerationStructureKHR *accel_write =
         assert(accel_write->accelerationStructureCount ==
         for (uint32_t j = 0; j < write->descriptorCount; j++) {
      ANV_FROM_HANDLE(vk_acceleration_structure, accel,
         anv_descriptor_set_write_acceleration_structure(device, set, accel,
            }
               default:
               }
      void anv_UpdateDescriptorSets(
      VkDevice                                    _device,
   uint32_t                                    descriptorWriteCount,
   const VkWriteDescriptorSet*                 pDescriptorWrites,
   uint32_t                                    descriptorCopyCount,
      {
               anv_descriptor_set_write(device, NULL, descriptorWriteCount,
            for (uint32_t i = 0; i < descriptorCopyCount; i++) {
      const VkCopyDescriptorSet *copy = &pDescriptorCopies[i];
   ANV_FROM_HANDLE(anv_descriptor_set, src, copy->srcSet);
            const struct anv_descriptor_set_binding_layout *src_layout =
         const struct anv_descriptor_set_binding_layout *dst_layout =
            if (src_layout->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      anv_descriptor_set_write_inline_uniform_data(device, dst,
                                       uint32_t copy_element_size = MIN2(src_layout->descriptor_stride,
         for (uint32_t j = 0; j < copy->descriptorCount; j++) {
      struct anv_descriptor *src_desc =
      &src->descriptors[src_layout->descriptor_index +
      struct anv_descriptor *dst_desc =
                  /* Copy the memory containing one of the following structure read by
   * the shaders :
   *    - anv_sampled_image_descriptor
   *    - anv_storage_image_descriptor
   *    - anv_address_range_descriptor
   *    - RENDER_SURFACE_STATE
   *    - SAMPLER_STATE
   */
   memcpy(dst->desc_mem.map +
         dst_layout->descriptor_offset +
   (copy->dstArrayElement + j) * dst_layout->descriptor_stride,
   src->desc_mem.map +
                                 /* If the CPU side may contain a buffer view, we need to copy that as
   * well
   */
   const enum anv_descriptor_data data =
      src_layout->type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT ?
   anv_descriptor_data_for_type(device->physical, src_desc->type) :
      if (data & ANV_DESCRIPTOR_BUFFER_VIEW) {
      struct anv_buffer_view *src_bview =
      &src->buffer_views[src_layout->buffer_view_index +
      struct anv_buffer_view *dst_bview =
                                          memcpy(dst_bview->general.state.map,
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
                                 case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
      for (uint32_t j = 0; j < entry->array_count; j++) {
      VkAccelerationStructureKHR *accel_obj =
                  anv_descriptor_set_write_acceleration_structure(device, set,
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
