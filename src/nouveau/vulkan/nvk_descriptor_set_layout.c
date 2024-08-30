   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_descriptor_set_layout.h"
      #include "nvk_descriptor_set.h"
   #include "nvk_device.h"
   #include "nvk_entrypoints.h"
   #include "nvk_sampler.h"
      #include "vk_pipeline_layout.h"
      #include "util/mesa-sha1.h"
      static bool
   binding_has_immutable_samplers(const VkDescriptorSetLayoutBinding *binding)
   {
      switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            default:
            }
      void
   nvk_descriptor_stride_align_for_type(VkDescriptorType type,
               {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
         case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      *stride = *align = sizeof(struct nvk_image_descriptor);
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      *stride = *align = sizeof(struct nvk_buffer_address);
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      *stride = *align = 0; /* These don't take up buffer space */
         case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      *stride = 1; /* Array size is bytes */
   *align = NVK_MIN_UBO_ALIGNMENT;
         case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
      *stride = *align = 0;
   if (type_list == NULL)
         for (unsigned i = 0; type_list && i < type_list->descriptorTypeCount; i++) {
      /* This shouldn't recurse */
   assert(type_list->pDescriptorTypes[i] !=
         uint32_t desc_stride, desc_align;
   nvk_descriptor_stride_align_for_type(type_list->pDescriptorTypes[i],
         *stride = MAX2(*stride, desc_stride);
      }
   *stride = ALIGN(*stride, *align);
         default:
                     }
      static const VkMutableDescriptorTypeListEXT *
   nvk_descriptor_get_type_list(VkDescriptorType type,
               {
      const VkMutableDescriptorTypeListVALVE *type_list = NULL;
   if (type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT) {
      assert(info != NULL);
   assert(info_idx < info->mutableDescriptorTypeListCount);
      }
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateDescriptorSetLayout(VkDevice _device,
                     {
               uint32_t num_bindings = 0;
   uint32_t immutable_sampler_count = 0;
   for (uint32_t j = 0; j < pCreateInfo->bindingCount; j++) {
      const VkDescriptorSetLayoutBinding *binding = &pCreateInfo->pBindings[j];
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
      if (binding_has_immutable_samplers(binding))
               VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct nvk_descriptor_set_layout, layout, 1);
   VK_MULTIALLOC_DECL(&ma, struct nvk_descriptor_set_binding_layout, bindings,
         VK_MULTIALLOC_DECL(&ma, struct nvk_sampler *, samplers,
            if (!vk_descriptor_set_layout_multizalloc(&device->vk, &ma))
                     for (uint32_t j = 0; j < pCreateInfo->bindingCount; j++) {
      const VkDescriptorSetLayoutBinding *binding = &pCreateInfo->pBindings[j];
   uint32_t b = binding->binding;
      * immutable_samplers pointer.  This provides us with a quick-and-dirty
   * way to sort the bindings by binding number.
   */
                  const VkDescriptorSetLayoutBindingFlagsCreateInfo *binding_flags_info =
      vk_find_struct_const(pCreateInfo->pNext,
      const VkMutableDescriptorTypeCreateInfoEXT *mutable_info =
      vk_find_struct_const(pCreateInfo->pNext,
         uint32_t buffer_size = 0;
   uint8_t dynamic_buffer_count = 0;
   for (uint32_t b = 0; b < num_bindings; b++) {
      /* We stashed the pCreateInfo->pBindings[] index (plus one) in the
   * immutable_samplers pointer.  Check for NULL (empty binding) and then
   * reset it and compute the index.
   */
   if (layout->binding[b].immutable_samplers == NULL)
         const uint32_t info_idx =
                  const VkDescriptorSetLayoutBinding *binding =
            if (binding->descriptorCount == 0)
                     if (binding_flags_info && binding_flags_info->bindingCount > 0) {
      assert(binding_flags_info->bindingCount == pCreateInfo->bindingCount);
                        switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      layout->binding[b].dynamic_buffer_index = dynamic_buffer_count;
   dynamic_buffer_count += binding->descriptorCount;
      default:
                  const VkMutableDescriptorTypeListEXT *type_list =
                  uint32_t stride, align;
   nvk_descriptor_stride_align_for_type(binding->descriptorType, type_list,
                     if (binding_has_immutable_samplers(binding)) {
      layout->binding[b].immutable_samplers = samplers;
   samplers += binding->descriptorCount;
   for (uint32_t i = 0; i < binding->descriptorCount; i++) {
      VK_FROM_HANDLE(nvk_sampler, sampler, binding->pImmutableSamplers[i]);
   layout->binding[b].immutable_samplers[i] = sampler;
   const uint8_t sampler_plane_count = sampler->vk.ycbcr_conversion ?
         if (max_plane_count < sampler_plane_count)
                           if (stride > 0) {
                     buffer_size = align64(buffer_size, align);
                  if (layout->binding[b].flags &
      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT) {
   /* From the Vulkan 1.3.256 spec:
   *
   *    VUID-VkDescriptorSetLayoutBindingFlagsCreateInfo-pBindingFlags-03004
   *    "If an element of pBindingFlags includes
   *    VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT, then
   *    all other elements of
   *    VkDescriptorSetLayoutCreateInfo::pBindings must have a
   *    smaller value of binding"
   *
   * In other words, it has to be the last binding.
   */
      } else {
      /* the allocation size will be computed at descriptor allocation,
   * but the buffer size will be already aligned as this binding will
   * be the last
   */
                        layout->non_variable_descriptor_buffer_size = buffer_size;
            struct mesa_sha1 sha1_ctx;
         #define SHA1_UPDATE_VALUE(x) _mesa_sha1_update(&sha1_ctx, &(x), sizeof(x));
      SHA1_UPDATE_VALUE(layout->non_variable_descriptor_buffer_size);
   SHA1_UPDATE_VALUE(layout->dynamic_buffer_count);
            for (uint32_t b = 0; b < num_bindings; b++) {
      SHA1_UPDATE_VALUE(layout->binding[b].type);
   SHA1_UPDATE_VALUE(layout->binding[b].flags);
   SHA1_UPDATE_VALUE(layout->binding[b].array_size);
   SHA1_UPDATE_VALUE(layout->binding[b].offset);
   SHA1_UPDATE_VALUE(layout->binding[b].stride);
   SHA1_UPDATE_VALUE(layout->binding[b].dynamic_buffer_index);
         #undef SHA1_UPDATE_VALUE
                              }
      VKAPI_ATTR void VKAPI_CALL
   nvk_GetDescriptorSetLayoutSupport(VkDevice _device,
               {
      const VkMutableDescriptorTypeCreateInfoEXT *mutable_info =
      vk_find_struct_const(pCreateInfo->pNext,
      const VkDescriptorSetLayoutBindingFlagsCreateInfo *binding_flags =
      vk_find_struct_const(pCreateInfo->pNext,
         /* Figure out the maximum alignment up-front.  Otherwise, we need to sort
   * the list of descriptors by binding number in order to get the size
   * accumulation right.
   */
   uint32_t max_align = 0;
   for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
      const VkDescriptorSetLayoutBinding *binding = &pCreateInfo->pBindings[i];
   const VkMutableDescriptorTypeListEXT *type_list =
                  uint32_t stride, align;
   nvk_descriptor_stride_align_for_type(binding->descriptorType, type_list,
                     uint64_t non_variable_size = 0;
   uint32_t variable_stride = 0;
   uint32_t variable_count = 0;
            for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
               VkDescriptorBindingFlags flags = 0;
   if (binding_flags != NULL && binding_flags->bindingCount > 0)
            switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      dynamic_buffer_count += binding->descriptorCount;
      default:
                  const VkMutableDescriptorTypeListEXT *type_list =
                  uint32_t stride, align;
   nvk_descriptor_stride_align_for_type(binding->descriptorType, type_list,
            if (stride > 0) {
                     if (flags & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT) {
      /* From the Vulkan 1.3.256 spec:
   *
   *    "For the purposes of this command, a variable-sized
   *    descriptor binding with a descriptorCount of zero is treated
   *    as if the descriptorCount is one"
   */
   variable_count = MAX2(1, binding->descriptorCount);
      } else {
      /* Since we're aligning to the maximum and since this is just a
   * check for whether or not the max buffer size is big enough, we
   * keep non_variable_size aligned to max_align.
   */
   non_variable_size += stride * binding->descriptorCount;
                     uint64_t buffer_size = non_variable_size;
   if (variable_stride > 0) {
      buffer_size += variable_stride * variable_count;
               uint32_t max_buffer_size;
   if (pCreateInfo->flags &
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR)
      else
            pSupport->supported = dynamic_buffer_count <= NVK_MAX_DYNAMIC_BUFFERS &&
            vk_foreach_struct(ext, pSupport->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT: {
      VkDescriptorSetVariableDescriptorCountLayoutSupport *vs = (void *)ext;
   if (variable_stride > 0) {
      vs->maxVariableDescriptorCount =
      } else {
         }
               default:
      nvk_debug_ignored_stype(ext->sType);
            }
      uint8_t
   nvk_descriptor_set_layout_dynbuf_start(const struct vk_pipeline_layout *pipeline_layout,
         {
                        for (uint32_t i = 0; i < set_layout_idx; i++) {
      const struct nvk_descriptor_set_layout *set_layout =
               }
      }
