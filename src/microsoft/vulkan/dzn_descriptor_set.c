   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "dzn_private.h"
      #include "vk_alloc.h"
   #include "vk_descriptors.h"
   #include "vk_util.h"
      #include "dxil_spirv_nir.h"
      #include "util/mesa-sha1.h"
      static uint32_t
   translate_desc_stages(VkShaderStageFlags in)
   {
      if (in == VK_SHADER_STAGE_ALL)
                     u_foreach_bit(s, in)
               }
      static D3D12_SHADER_VISIBILITY
   translate_desc_visibility(VkShaderStageFlags in)
   {
      switch (in) {
   case VK_SHADER_STAGE_VERTEX_BIT: return D3D12_SHADER_VISIBILITY_VERTEX;
   case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return D3D12_SHADER_VISIBILITY_HULL;
   case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return D3D12_SHADER_VISIBILITY_DOMAIN;
   case VK_SHADER_STAGE_GEOMETRY_BIT: return D3D12_SHADER_VISIBILITY_GEOMETRY;
   case VK_SHADER_STAGE_FRAGMENT_BIT: return D3D12_SHADER_VISIBILITY_PIXEL;
   default: return D3D12_SHADER_VISIBILITY_ALL;
      }
      static D3D12_DESCRIPTOR_RANGE_TYPE
   desc_type_to_range_type(VkDescriptorType in, bool writeable)
   {
      switch (in) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
         default:
            }
      static bool
   is_dynamic_desc_type(VkDescriptorType desc_type)
   {
      return (desc_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
      }
      static bool
   is_buffer_desc_type_without_view(VkDescriptorType desc_type)
   {
      return (desc_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
      }
      static bool
   dzn_descriptor_type_depends_on_shader_usage(VkDescriptorType type, bool bindless)
   {
      if (bindless)
         return type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER ||
         type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
      }
      static inline bool
   dzn_desc_type_has_sampler(VkDescriptorType type)
   {
      return type == VK_DESCRIPTOR_TYPE_SAMPLER ||
      }
      static uint32_t
   num_descs_for_type(VkDescriptorType type, bool static_sampler, bool bindless)
   {
      if (bindless)
                     /* Some type map to an SRV or UAV depending on how the shaders is using the
   * resource (NONWRITEABLE flag set or not), in that case we need to reserve
   * slots for both the UAV and SRV descs.
   */
   if (dzn_descriptor_type_depends_on_shader_usage(type, false))
            /* There's no combined SRV+SAMPLER type in d3d12, we need an descriptor
   * for the sampler.
   */
   if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            /* Don't count immutable samplers, they have their own descriptor. */
   if (static_sampler && dzn_desc_type_has_sampler(type))
               }
      static VkResult
   dzn_descriptor_set_layout_create(struct dzn_device *device,
                     {
      const VkDescriptorSetLayoutBinding *bindings = pCreateInfo->pBindings;
   uint32_t binding_count = 0, static_sampler_count = 0, total_ranges = 0;
   uint32_t dynamic_ranges_offset = 0, immutable_sampler_count = 0;
   uint32_t dynamic_buffer_count = 0;
            const VkDescriptorSetLayoutBindingFlagsCreateInfo *binding_flags =
                  for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
               if (!bindings[i].descriptorCount)
            D3D12_SHADER_VISIBILITY visibility = device->bindless ?
      D3D12_SHADER_VISIBILITY_ALL :
      VkDescriptorType desc_type = bindings[i].descriptorType;
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
   bool immutable_samplers =
      has_sampler &&
               if (device->support_static_samplers &&
                     if (sampler->static_border_color != -1)
               if (static_sampler) {
         } else if (has_sampler) {
      unsigned type = device->bindless ? D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV : D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
                  if (immutable_samplers)
               if (desc_type != VK_DESCRIPTOR_TYPE_SAMPLER) {
                     if (dzn_descriptor_type_depends_on_shader_usage(desc_type, device->bindless)) {
      range_count[visibility][D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]++;
               uint32_t factor =
         if (is_dynamic_desc_type(desc_type))
         else
               if (binding_flags && binding_flags->bindingCount &&
      (binding_flags->pBindingFlags[i] & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT))
                     /* We need to allocate decriptor set layouts off the device allocator
   * with DEVICE scope because they are reference counted and may not be
   * destroyed when vkDestroyDescriptorSetLayout is called.
   */
   VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct dzn_descriptor_set_layout, set_layout, 1);
   VK_MULTIALLOC_DECL(&ma, D3D12_DESCRIPTOR_RANGE1,
         VK_MULTIALLOC_DECL(&ma, D3D12_STATIC_SAMPLER_DESC1, static_samplers,
         VK_MULTIALLOC_DECL(&ma, const struct dzn_sampler *, immutable_samplers,
         VK_MULTIALLOC_DECL(&ma, struct dzn_descriptor_set_layout_binding, binfos,
            if (!vk_descriptor_set_layout_multizalloc(&device->vk, &ma))
            set_layout->static_samplers = static_samplers;
   set_layout->static_sampler_count = static_sampler_count;
   set_layout->immutable_samplers = immutable_samplers;
   set_layout->immutable_sampler_count = immutable_sampler_count;
   set_layout->bindings = binfos;
   set_layout->binding_count = binding_count;
   set_layout->dynamic_buffers.range_offset = dynamic_ranges_offset;
            for (uint32_t i = 0; i < MAX_SHADER_VISIBILITIES; i++) {
      dzn_foreach_pool_type (type) {
      if (range_count[i][type]) {
      set_layout->ranges[i][type] = ranges;
   set_layout->range_count[i][type] = range_count[i][type];
                     VkDescriptorSetLayoutBinding *ordered_bindings;
   VkResult ret =
      vk_create_sorted_bindings(pCreateInfo->pBindings,
            if (ret != VK_SUCCESS) {
      vk_descriptor_set_layout_destroy(&device->vk, &set_layout->vk);
               assert(binding_count ==
                  uint32_t range_idx[MAX_SHADER_VISIBILITIES][NUM_POOL_TYPES] = { 0 };
   uint32_t static_sampler_idx = 0, immutable_sampler_idx = 0;
   uint32_t dynamic_buffer_idx = 0;
            for (uint32_t i = 0; i < binding_count; i++) {
      binfos[i].immutable_sampler_idx = ~0;
   binfos[i].buffer_idx = ~0;
   dzn_foreach_pool_type (type)
               for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
      VkDescriptorType desc_type = ordered_bindings[i].descriptorType;
   uint32_t binding = ordered_bindings[i].binding;
   uint32_t desc_count = ordered_bindings[i].descriptorCount;
   bool has_sampler = dzn_desc_type_has_sampler(desc_type);
   bool has_immutable_samplers =
      has_sampler &&
      bool has_static_sampler = device->support_static_samplers &&
                  D3D12_SHADER_VISIBILITY visibility = device->bindless ?
      D3D12_SHADER_VISIBILITY_ALL :
      binfos[binding].type = desc_type;
   binfos[binding].stages =
         set_layout->stages |= binfos[binding].stages;
   binfos[binding].visibility = visibility;
   /* Only the last binding can have variable size */
   if (binding == binding_count - 1)
         if (is_dynamic && device->bindless) {
      /* Assign these into a separate 0->N space */
      } else {
      binfos[binding].base_shader_register = base_register;
   assert(base_register + desc_count >= base_register);
               if (has_static_sampler) {
               /* Not all border colors are supported. */
   if (sampler->static_border_color != -1) {
                     desc->Filter = sampler->desc.Filter;
   desc->AddressU = sampler->desc.AddressU;
   desc->AddressV = sampler->desc.AddressV;
   desc->AddressW = sampler->desc.AddressW;
   desc->MipLODBias = sampler->desc.MipLODBias;
   desc->MaxAnisotropy = sampler->desc.MaxAnisotropy;
   desc->ComparisonFunc = sampler->desc.ComparisonFunc;
   desc->BorderColor = sampler->static_border_color;
   desc->MinLOD = sampler->desc.MinLOD;
   desc->MaxLOD = sampler->desc.MaxLOD;
   desc->ShaderRegister = binfos[binding].base_shader_register;
   desc->ShaderVisibility = translate_desc_visibility(ordered_bindings[i].stageFlags);
   desc->Flags = sampler->desc.Flags;
   if (device->bindless && desc_type == VK_DESCRIPTOR_TYPE_SAMPLER) {
      /* Avoid using space in the descriptor set buffer for pure static samplers. The meaning
   * of base register index is different for them - it's just an ID to map to the root
   * signature, as opposed to all other bindings where it's an offset into the buffer. */
   binfos[binding].base_shader_register = desc->ShaderRegister = static_sampler_idx;
      }
      } else {
                     if (has_static_sampler) {
         } else if (has_immutable_samplers) {
      binfos[binding].immutable_sampler_idx = immutable_sampler_idx;
                                    if (is_dynamic) {
      binfos[binding].buffer_idx = dynamic_buffer_idx;
   for (uint32_t d = 0; d < desc_count; d++)
         dynamic_buffer_idx += desc_count;
      } else if (is_buffer_desc_type_without_view(desc_type)) {
      binfos[binding].buffer_idx = set_layout->buffer_count;
               if (!ordered_bindings[i].descriptorCount)
            unsigned num_descs =
                           bool has_range[NUM_POOL_TYPES] = { 0 };
   has_range[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] =
         has_range[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] =
            dzn_foreach_pool_type (type) {
                              binfos[binding].range_idx[type] = idx;
   D3D12_DESCRIPTOR_RANGE1 *range = (D3D12_DESCRIPTOR_RANGE1 *)
         VkDescriptorType range_type = desc_type;
   if (desc_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
      range_type = type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
            }
   range->RangeType = desc_type_to_range_type(range_type, true);
   range->NumDescriptors = desc_count;
   range->BaseShaderRegister = binfos[binding].base_shader_register;
   range->Flags = type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
      D3D12_DESCRIPTOR_RANGE_FLAG_NONE :
      if (is_dynamic) {
      range->OffsetInDescriptorsFromTableStart =
      set_layout->dynamic_buffers.range_offset +
      set_layout->dynamic_buffers.count += range->NumDescriptors;
      } else {
      range->OffsetInDescriptorsFromTableStart = set_layout->range_desc_count[type];
   if (!binfos[binding].variable_size)
                              assert(idx + 1 < range_count[visibility][type]);
   range_idx[visibility][type]++;
   range[1] = range[0];
   range++;
   range->RangeType = desc_type_to_range_type(range_type, false);
   if (is_dynamic) {
      range->OffsetInDescriptorsFromTableStart =
      set_layout->dynamic_buffers.range_offset +
         } else {
      range->OffsetInDescriptorsFromTableStart = set_layout->range_desc_count[type];
                              *out = dzn_descriptor_set_layout_to_handle(set_layout);
      }
      static uint32_t
   dzn_descriptor_set_layout_get_heap_offset(const struct dzn_descriptor_set_layout *layout,
                           {
      assert(b < layout->binding_count);
   D3D12_SHADER_VISIBILITY visibility = layout->bindings[b].visibility;
   assert(visibility < ARRAY_SIZE(layout->ranges));
            if (bindless)
                     if (range_idx == ~0)
            if (alt &&
      !dzn_descriptor_type_depends_on_shader_usage(layout->bindings[b].type, bindless))
         if (alt)
            assert(range_idx < layout->range_count[visibility][type]);
      }
      static uint32_t
   dzn_descriptor_set_layout_get_desc_count(const struct dzn_descriptor_set_layout *layout,
         {
      D3D12_SHADER_VISIBILITY visibility = layout->bindings[b].visibility;
            dzn_foreach_pool_type (type) {
      uint32_t range_idx = layout->bindings[b].range_idx[type];
            if (range_idx != ~0)
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateDescriptorSetLayout(VkDevice device,
                     {
      return dzn_descriptor_set_layout_create(dzn_device_from_handle(device),
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetDescriptorSetLayoutSupport(VkDevice _device,
               {
      VK_FROM_HANDLE(dzn_device, device, _device);
   const VkDescriptorSetLayoutBinding *bindings = pCreateInfo->pBindings;
            for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
      VkDescriptorType desc_type = bindings[i].descriptorType;
            if (has_sampler)
         if (desc_type != VK_DESCRIPTOR_TYPE_SAMPLER)
         if (dzn_descriptor_type_depends_on_shader_usage(desc_type, device->bindless))
               pSupport->supported = device->bindless ||
      (sampler_count <= MAX_DESCS_PER_SAMPLER_HEAP &&
   }
      static void
   dzn_pipeline_layout_destroy(struct vk_device *vk_device,
         {
      struct dzn_pipeline_layout *layout =
            if (layout->root.sig)
               }
      // Reserve two root parameters for the push constants and sysvals CBVs.
   #define MAX_INTERNAL_ROOT_PARAMS 2
      // One root parameter for samplers and the other one for views, multiplied by
   // the number of visibility combinations, plus the internal root parameters.
   #define MAX_ROOT_PARAMS ((MAX_SHADER_VISIBILITIES * 2) + MAX_INTERNAL_ROOT_PARAMS)
      // Maximum number of DWORDS (32-bit words) that can be used for a root signature
   #define MAX_ROOT_DWORDS 64
      static void
   dzn_pipeline_layout_hash_stages(struct dzn_pipeline_layout *layout,
         {
      uint32_t stages = 0;
   for (uint32_t stage = 0; stage < ARRAY_SIZE(layout->stages); stage++) {
      for (uint32_t set = 0; set < info->setLayoutCount; set++) {
                              for (uint32_t stage = 0; stage < ARRAY_SIZE(layout->stages); stage++) {
      if (!(stages & BITFIELD_BIT(stage)))
                     _mesa_sha1_init(&ctx);
   for (uint32_t set = 0; set < info->setLayoutCount; set++) {
      VK_FROM_HANDLE(dzn_descriptor_set_layout, set_layout, info->pSetLayouts[set]);
                  for (uint32_t b = 0; b < set_layout->binding_count; b++) {
                     _mesa_sha1_update(&ctx, &b, sizeof(b));
   _mesa_sha1_update(&ctx, &set_layout->bindings[b].base_shader_register,
         }
         }
      static VkResult
   dzn_pipeline_layout_create(struct dzn_device *device,
                     {
      struct dzn_physical_device *pdev = container_of(device->vk.physical, struct dzn_physical_device, vk);
            for (uint32_t s = 0; s < pCreateInfo->setLayoutCount; s++) {
               if (!set_layout)
                        VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct dzn_pipeline_layout, layout, 1);
   VK_MULTIALLOC_DECL(&ma, uint32_t, binding_translation, binding_count);
            if (!vk_pipeline_layout_multizalloc(&device->vk, &ma, pCreateInfo))
                     for (uint32_t s = 0; s < pCreateInfo->setLayoutCount; s++) {
               if (!set_layout || !set_layout->binding_count)
            layout->binding_translation[s].base_reg = binding_translation;
   layout->binding_translation[s].binding_class = binding_class;
   binding_translation += set_layout->binding_count;
               uint32_t range_count = 0, static_sampler_count = 0;
            layout->root.param_count = 0;
   dzn_foreach_pool_type (type)
            layout->set_count = pCreateInfo->setLayoutCount;
   for (uint32_t j = 0; j < layout->set_count; j++) {
      VK_FROM_HANDLE(dzn_descriptor_set_layout, set_layout, pCreateInfo->pSetLayouts[j]);
            layout->sets[j].dynamic_buffer_count = set_layout->dynamic_buffers.count;
   memcpy(layout->sets[j].range_desc_count, set_layout->range_desc_count,
                  for (uint32_t b = 0; b < set_layout->binding_count; b++) {
      binding_trans[b] = set_layout->bindings[b].base_shader_register;
   if (is_dynamic_desc_type(set_layout->bindings[b].type)) {
      layout->binding_translation[j].binding_class[b] = DZN_PIPELINE_BINDING_DYNAMIC_BUFFER;
   if (device->bindless)
      } else if (dzn_desc_type_has_sampler(set_layout->bindings[b].type) &&
               else
               static_sampler_count += set_layout->static_sampler_count;
   dzn_foreach_pool_type (type) {
      layout->sets[j].heap_offsets[type] = layout->desc_count[type];
   layout->desc_count[type] += set_layout->range_desc_count[type];
   for (uint32_t i = 0; i < MAX_SHADER_VISIBILITIES; i++)
               if (!device->bindless)
            dynamic_buffer_base += set_layout->dynamic_buffers.count;
   for (uint32_t o = 0, elem = 0; o < set_layout->dynamic_buffers.count; o++, elem++) {
      if (device->bindless) {
      layout->sets[j].dynamic_buffer_heap_offsets[o].primary = layout->dynamic_buffer_count++;
                                       uint32_t heap_offset =
      dzn_descriptor_set_layout_get_heap_offset(set_layout, b, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      uint32_t alt_heap_offset =
                  layout->sets[j].dynamic_buffer_heap_offsets[o].primary = heap_offset != ~0 ? heap_offset + elem : ~0;
                     D3D12_DESCRIPTOR_RANGE1 *ranges =
      vk_alloc2(&device->vk.alloc, pAllocator, sizeof(*ranges) * range_count, 8,
      if (range_count && !ranges) {
      vk_pipeline_layout_unref(&device->vk, &layout->vk);
               static_assert(sizeof(D3D12_STATIC_SAMPLER_DESC1) > sizeof(D3D12_STATIC_SAMPLER_DESC),
         D3D12_STATIC_SAMPLER_DESC1 *static_sampler_descs =
      vk_alloc2(&device->vk.alloc, pAllocator,
            if (static_sampler_count && !static_sampler_descs) {
      vk_free2(&device->vk.alloc, pAllocator, ranges);
   vk_pipeline_layout_unref(&device->vk, &layout->vk);
                  D3D12_ROOT_PARAMETER1 root_params[MAX_ROOT_PARAMS] = { 0 };
   D3D12_DESCRIPTOR_RANGE1 *range_ptr = ranges;
   D3D12_ROOT_PARAMETER1 *root_param;
            if (device->bindless) {
      for (uint32_t j = 0; j < pCreateInfo->setLayoutCount; j++) {
      root_param = &root_params[layout->root.param_count++];
   root_param->ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
   root_param->Descriptor.RegisterSpace = 0;
   root_param->Descriptor.ShaderRegister = j;
   root_param->Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
   root_param->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
         } else {
      for (uint32_t i = 0; i < MAX_SHADER_VISIBILITIES; i++) {
      dzn_foreach_pool_type(type) {
      root_param = &root_params[layout->root.param_count];
   root_param->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
   root_param->DescriptorTable.pDescriptorRanges = range_ptr;
                  for (uint32_t j = 0; j < pCreateInfo->setLayoutCount; j++) {
                     memcpy(range_ptr, set_layout->ranges[i][type],
         for (uint32_t k = 0; k < range_count; k++) {
      range_ptr[k].RegisterSpace = j;
   range_ptr[k].OffsetInDescriptorsFromTableStart +=
      }
                     if (root_param->DescriptorTable.NumDescriptorRanges) {
      layout->root.type[layout->root.param_count++] = (D3D12_DESCRIPTOR_HEAP_TYPE)type;
                                 if (layout->dynamic_buffer_count > 0 && device->bindless) {
      layout->root.dynamic_buffer_bindless_param_idx = layout->root.param_count;
            root_param->ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
   root_param->Descriptor.RegisterSpace = 0;
   root_param->Descriptor.ShaderRegister = layout->root.sets_param_count;
   root_param->Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
   root_param->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
               /* Add our sysval CBV, and make it visible to all shaders */
   layout->root.sysval_cbv_param_idx = layout->root.param_count;
   root_param = &root_params[layout->root.param_count++];
   root_param->ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
   root_param->Descriptor.RegisterSpace = DZN_REGISTER_SPACE_SYSVALS;
   root_param->Constants.ShaderRegister = 0;
   root_param->Constants.Num32BitValues =
      DIV_ROUND_UP(MAX2(sizeof(struct dxil_spirv_vertex_runtime_data),
            root_param->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            if (pdev->root_sig_version >= D3D_ROOT_SIGNATURE_VERSION_1_2) {
      D3D12_STATIC_SAMPLER_DESC1 *static_sampler_ptr = static_sampler_descs;
   for (uint32_t j = 0; j < pCreateInfo->setLayoutCount; j++) {
               memcpy(static_sampler_ptr, set_layout->static_samplers,
         if (j > 0) {
      for (uint32_t k = 0; k < set_layout->static_sampler_count; k++)
      }
         } else {
      D3D12_STATIC_SAMPLER_DESC *static_sampler_ptr = (void *)static_sampler_descs;
   for (uint32_t j = 0; j < pCreateInfo->setLayoutCount; j++) {
               for (uint32_t k = 0; k < set_layout->static_sampler_count; k++) {
      memcpy(static_sampler_ptr, &set_layout->static_samplers[k],
         static_sampler_ptr->RegisterSpace = j;
                     uint32_t push_constant_size = 0;
   uint32_t push_constant_flags = 0;
   for (uint32_t j = 0; j < pCreateInfo->pushConstantRangeCount; j++) {
      const VkPushConstantRange *range = pCreateInfo->pPushConstantRanges + j;
   push_constant_size = MAX2(push_constant_size, range->offset + range->size);
               if (push_constant_size > 0) {
      layout->root.push_constant_cbv_param_idx = layout->root.param_count;
            root_param->ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
   root_param->Constants.ShaderRegister = 0;
   root_param->Constants.Num32BitValues = ALIGN(push_constant_size, 4) / 4;
   root_param->Constants.RegisterSpace = DZN_REGISTER_SPACE_PUSH_CONSTANT;
   root_param->ShaderVisibility = translate_desc_visibility(push_constant_flags);
               assert(layout->root.param_count <= ARRAY_SIZE(root_params));
            D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc = {
         };
   /* TODO Only enable this flag when needed (optimization) */
   D3D12_ROOT_SIGNATURE_FLAGS root_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
   if (device->bindless)
      root_flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED |
      if (pdev->root_sig_version >= D3D_ROOT_SIGNATURE_VERSION_1_2) {
      root_sig_desc.Desc_1_2 = (D3D12_ROOT_SIGNATURE_DESC2){
      .NumParameters = layout->root.param_count,
   .pParameters = layout->root.param_count ? root_params : NULL,
   .NumStaticSamplers = static_sampler_count,
   .pStaticSamplers = static_sampler_descs,
         } else {
      root_sig_desc.Desc_1_1 = (D3D12_ROOT_SIGNATURE_DESC1){
         .NumParameters = layout->root.param_count,
   .pParameters = layout->root.param_count ? root_params : NULL,
   .NumStaticSamplers = static_sampler_count,
   .pStaticSamplers = (void *)static_sampler_descs,
               layout->root.sig = dzn_device_create_root_sig(device, &root_sig_desc);
   vk_free2(&device->vk.alloc, pAllocator, ranges);
            if (!layout->root.sig) {
      vk_pipeline_layout_unref(&device->vk, &layout->vk);
               dzn_pipeline_layout_hash_stages(layout, pCreateInfo);
   *out = dzn_pipeline_layout_to_handle(layout);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreatePipelineLayout(VkDevice device,
                     {
      return dzn_pipeline_layout_create(dzn_device_from_handle(device),
      }
      void
   dzn_descriptor_heap_finish(struct dzn_descriptor_heap *heap)
   {
      if (heap->heap)
      }
      VkResult
   dzn_descriptor_heap_init(struct dzn_descriptor_heap *heap,
                           {
      heap->desc_count = desc_count;
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
      .Type = type,
   .NumDescriptors = desc_count,
   .Flags = shader_visible ?
                     if (FAILED(ID3D12Device1_CreateDescriptorHeap(device->dev, &desc,
                  return vk_error(device,
                     D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = dzn_ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap->heap);
   heap->cpu_base = cpu_handle.ptr;
   if (shader_visible) {
      D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = dzn_ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap->heap);
                  }
      D3D12_CPU_DESCRIPTOR_HANDLE
   dzn_descriptor_heap_get_cpu_handle(const struct dzn_descriptor_heap *heap, uint32_t desc_offset)
   {
      return (D3D12_CPU_DESCRIPTOR_HANDLE) {
            }
      D3D12_GPU_DESCRIPTOR_HANDLE
   dzn_descriptor_heap_get_gpu_handle(const struct dzn_descriptor_heap *heap, uint32_t desc_offset)
   {
      return (D3D12_GPU_DESCRIPTOR_HANDLE) {
            }
      void
   dzn_descriptor_heap_write_sampler_desc(struct dzn_device *device,
                     {
      struct dzn_physical_device *pdev = container_of(device->vk.physical, struct dzn_physical_device, vk);
   if (device->dev11 && pdev->options14.AdvancedTextureOpsSupported)
      ID3D12Device11_CreateSampler2(device->dev11, &sampler->desc,
      else
      ID3D12Device1_CreateSampler(device->dev, (D3D12_SAMPLER_DESC *)&sampler->desc,
   }
      void
   dzn_descriptor_heap_write_image_view_desc(struct dzn_device *device,
                           {
      D3D12_CPU_DESCRIPTOR_HANDLE view_handle =
                  if (writeable) {
         } else if (cube_as_2darray &&
            (iview->srv_desc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBEARRAY ||
   D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = iview->srv_desc;
   srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
   srv_desc.Texture2DArray.PlaneSlice = 0;
   srv_desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
   if (iview->srv_desc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBEARRAY) {
      srv_desc.Texture2DArray.MostDetailedMip =
         srv_desc.Texture2DArray.MipLevels =
         srv_desc.Texture2DArray.FirstArraySlice =
         srv_desc.Texture2DArray.ArraySize =
      } else {
      srv_desc.Texture2DArray.MostDetailedMip =
         srv_desc.Texture2DArray.MipLevels =
         srv_desc.Texture2DArray.FirstArraySlice = 0;
                  } else {
            }
      void
   dzn_descriptor_heap_write_buffer_view_desc(struct dzn_device *device,
                           {
      D3D12_CPU_DESCRIPTOR_HANDLE view_handle =
            if (writeable)
         else
      }
      void
   dzn_descriptor_heap_write_buffer_desc(struct dzn_device *device,
                           {
      D3D12_CPU_DESCRIPTOR_HANDLE view_handle =
            VkDeviceSize size =
      info->range == VK_WHOLE_SIZE ?
   info->buffer->size - info->offset :
         if (info->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
      info->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
   assert(!writeable);
   D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {
      .BufferLocation = info->buffer->gpuva + info->offset,
      };
      } else if (writeable) {
      D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {
      .Format = DXGI_FORMAT_R32_TYPELESS,
   .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
   .Buffer = {
      .FirstElement = info->offset / sizeof(uint32_t),
   .NumElements = (UINT)DIV_ROUND_UP(size, sizeof(uint32_t)),
         };
      } else {
      D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
      .Format = DXGI_FORMAT_R32_TYPELESS,
   .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
   .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
   .Buffer = {
      .FirstElement = info->offset / sizeof(uint32_t),
   .NumElements = (UINT)DIV_ROUND_UP(size, sizeof(uint32_t)),
         };
         }
      static void
   dzn_bindless_descriptor_set_write_sampler_desc(volatile struct dxil_spirv_bindless_entry *map,
               {
         }
      static void
   dzn_bindless_descriptor_set_write_image_view_desc(volatile struct dxil_spirv_bindless_entry *map,
                     {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      map[desc_offset].texture_idx = iview->uav_bindless_slot;
      case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      map[desc_offset].texture_idx = iview->srv_bindless_slot;
      default:
            }
      static void
   dzn_bindless_descriptor_set_write_buffer_view_desc(volatile struct dxil_spirv_bindless_entry *map,
                     {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      map[desc_offset].texture_idx = bview->srv_bindless_slot;
      case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      map[desc_offset].texture_idx = bview->uav_bindless_slot;
      default:
            }
      static void
   dzn_bindless_descriptor_set_write_buffer_desc(struct dzn_device *device,
                     {
         }
      static bool
   need_custom_buffer_descriptor(struct dzn_device *device, const struct dzn_buffer_desc *info,
         {
      *out_desc = *info;
   uint32_t upper_bound_default_descriptor;
   uint32_t size_align, offset_align;
   /* Canonicalize descriptor types for hash/compare, and get size/align info */
   switch (info->type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      out_desc->type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      upper_bound_default_descriptor =
         size_align = offset_align = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      out_desc->type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      default:
      upper_bound_default_descriptor = MIN2(UINT32_MAX, info->buffer->size);
   offset_align = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
   size_align = 4;
               uint64_t upper_bound = info->range == VK_WHOLE_SIZE ?
      info->buffer->size :
      /* Addressing the whole buffer, no custom descriptor needed. */
   if (upper_bound == upper_bound_default_descriptor)
            out_desc->range = ALIGN_POT(upper_bound, size_align);
   if (out_desc->range <= upper_bound_default_descriptor) {
      /* Use a larger descriptor with the hope that we'll be more likely
   * to be able to re-use it. The shader is already doing the offset
   * add, so there's not really a cost to putting a nonzero value there. */
      } else {
      /* At least align-down the base offset to ensure that's a valid view to create */
   out_desc->offset = (out_desc->offset / offset_align) * offset_align;
      }
      }
      static uint32_t
   hash_buffer_desc(const void *data)
   {
      const struct dzn_buffer_desc *bdesc = data;
   /* Avoid any potential padding in the struct */
   uint32_t type_hash = _mesa_hash_data(&bdesc->type, sizeof(bdesc->type));
      }
      static bool
   compare_buffer_desc(const void *_a, const void *_b)
   {
      const struct dzn_buffer_desc *a = _a, *b = _b;
   assert(a->buffer == b->buffer);
   /* Avoid any potential padding in the struct */
   return a->type == b->type &&
            }
      static int
   handle_custom_descriptor_cache(struct dzn_device *device,
         {
               /* Initialize hash map */
   if (!stack_desc->buffer->custom_views)
            if (!stack_desc->buffer->custom_views)
            uint32_t hash = hash_buffer_desc(stack_desc);
   struct hash_entry *entry = _mesa_hash_table_search_pre_hashed(stack_desc->buffer->custom_views, hash, stack_desc);
   if (entry)
            int slot = dzn_device_descriptor_heap_alloc_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   if (slot < 0)
            struct dzn_buffer_desc *key = malloc(sizeof(*stack_desc));
   if (!key) {
      dzn_device_descriptor_heap_free_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, slot);
               *key = *stack_desc;
   entry = _mesa_hash_table_insert_pre_hashed(stack_desc->buffer->custom_views, hash, key, (void *)(intptr_t)slot);
   if (!entry) {
      free(key);
   dzn_device_descriptor_heap_free_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, slot);
               dzn_descriptor_heap_write_buffer_desc(device, &device->device_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].heap,
            }
      void
   dzn_buffer_get_bindless_buffer_descriptor(struct dzn_device *device,
               {
      int slot;
   uint32_t offset = bdesc->offset;
   switch (bdesc->type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      slot = bdesc->buffer->cbv_bindless_slot;
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      slot = bdesc->buffer->uav_bindless_slot;
      default:
                  struct dzn_buffer_desc local_desc;
   if (need_custom_buffer_descriptor(device, bdesc, &local_desc)) {
               int new_slot = handle_custom_descriptor_cache(device, &local_desc);
   if (new_slot >= 0) {
      slot = new_slot;
      }
   /* In the case of cache failure, just use the base view and try
                        out->buffer_idx = slot;
      }
      void
   dzn_descriptor_heap_copy(struct dzn_device *device,
                           struct dzn_descriptor_heap *dst_heap,
   uint32_t dst_offset,
   {
      D3D12_CPU_DESCRIPTOR_HANDLE dst_handle =
         D3D12_CPU_DESCRIPTOR_HANDLE src_handle =
            ID3D12Device1_CopyDescriptorsSimple(device->dev, desc_count,
                  }
      struct dzn_descriptor_set_ptr {
         };
      static void
   dzn_descriptor_set_ptr_validate(const struct dzn_descriptor_set_layout *layout,
         {
         if (ptr->binding >= layout->binding_count) {
      ptr->binding = ~0;
   ptr->elem = ~0;
               uint32_t desc_count =
         if (ptr->elem >= desc_count) {
      ptr->binding = ~0;
         }
      static void
   dzn_descriptor_set_ptr_init(const struct dzn_descriptor_set_layout *layout,
               {
      ptr->binding = binding;
   ptr->elem = elem;
      }
      static void
   dzn_descriptor_set_ptr_move(const struct dzn_descriptor_set_layout *layout,
               {
      if (ptr->binding == ~0)
            while (count) {
      uint32_t desc_count =
            if (count >= desc_count - ptr->elem) {
      count -= desc_count - ptr->elem;
   ptr->binding++;
      } else {
      ptr->elem += count;
                     }
      static bool
   dzn_descriptor_set_ptr_is_valid(const struct dzn_descriptor_set_ptr *ptr)
   {
         }
      static uint32_t
   dzn_descriptor_set_remaining_descs_in_binding(const struct dzn_descriptor_set_layout *layout,
         {
      if (ptr->binding >= layout->binding_count)
            uint32_t desc_count =
               }
         static uint32_t
   dzn_descriptor_set_ptr_get_heap_offset(const struct dzn_descriptor_set_layout *layout,
                           {
      if (ptr->binding == ~0)
            uint32_t base =
         if (base == ~0)
               }
      static void
   dzn_descriptor_set_write_sampler_desc(struct dzn_device *device,
                     {
      if (heap_offset == ~0)
            D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
   if (device->bindless) {
      dzn_bindless_descriptor_set_write_sampler_desc(set->pool->bindless.map,
            } else {
      dzn_descriptor_heap_write_sampler_desc(device,
                     }
      static void
   dzn_descriptor_set_ptr_write_sampler_desc(struct dzn_device *device,
                     {
      D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
   uint32_t heap_offset =
               }
      static uint32_t
   dzn_descriptor_set_ptr_get_buffer_idx(const struct dzn_descriptor_set_layout *layout,
         {
      if (ptr->binding == ~0)
                     if (base == ~0)
               }
      static void
   dzn_descriptor_set_write_dynamic_buffer_desc(struct dzn_device *device,
                     {
      if (dynamic_buffer_idx == ~0)
            assert(dynamic_buffer_idx < set->layout->dynamic_buffers.count);
      }
      static void
   dzn_descriptor_set_ptr_write_dynamic_buffer_desc(struct dzn_device *device,
                     {
      uint32_t dynamic_buffer_idx =
               }
      static VkDescriptorType
   dzn_descriptor_set_ptr_get_vk_type(const struct dzn_descriptor_set_layout *layout,
         {
      if (ptr->binding >= layout->binding_count)
               }
      static void
   dzn_descriptor_set_write_image_view_desc(struct dzn_device *device,
                                       {
               if (heap_offset == ~0)
            if (device->bindless) {
      dzn_bindless_descriptor_set_write_image_view_desc(set->pool->bindless.map,
                                          dzn_descriptor_heap_write_image_view_desc(device,
                              if (alt_heap_offset != ~0) {
      assert(primary_writable);
   dzn_descriptor_heap_write_image_view_desc(device,
                           }
      static void
   dzn_descriptor_set_ptr_write_image_view_desc(struct dzn_device *device,
                                 {
      D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
   uint32_t heap_offset =
         uint32_t alt_heap_offset =
            dzn_descriptor_set_write_image_view_desc(device, desc_type, set, heap_offset, alt_heap_offset,
      }
      static void
   dzn_descriptor_set_write_buffer_view_desc(struct dzn_device *device,
                                 {
      if (heap_offset == ~0)
            if (device->bindless) {
      dzn_bindless_descriptor_set_write_buffer_view_desc(set->pool->bindless.map,
                                 D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            dzn_descriptor_heap_write_buffer_view_desc(device,
                              if (alt_heap_offset != ~0) {
      assert(primary_writable);
   dzn_descriptor_heap_write_buffer_view_desc(device,
                           }
      static void
   dzn_descriptor_set_ptr_write_buffer_view_desc(struct dzn_device *device,
                           {
      D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
   uint32_t heap_offset =
         uint32_t alt_heap_offset =
               }
      static void
   dzn_descriptor_set_write_buffer_desc(struct dzn_device *device,
                                 {
      D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
   if (heap_offset == ~0)
            if (device->bindless) {
      dzn_bindless_descriptor_set_write_buffer_desc(device,
                                          dzn_descriptor_heap_write_buffer_desc(device, &set->pool->heaps[type],
                  if (alt_heap_offset != ~0) {
      assert(primary_writable);
   dzn_descriptor_heap_write_buffer_desc(device, &set->pool->heaps[type],
                     }
      static void
   dzn_descriptor_set_ptr_write_buffer_desc(struct dzn_device *device,
                           {
      D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
   uint32_t heap_offset =
         uint32_t alt_heap_offset =
               }
      static VkResult
   dzn_descriptor_set_init(struct dzn_descriptor_set *set,
                           struct dzn_device *device,
   {
               set->pool = pool;
            if (!reuse) {
      dzn_foreach_pool_type(type) {
      set->heap_offsets[type] = pool->free_offset[type];
   if (device->bindless)
         set->heap_sizes[type] = layout->range_desc_count[type] + variable_descriptor_count[type];
                  /* Pre-fill the immutable samplers */
   if (layout->immutable_sampler_count) {
      for (uint32_t b = 0; b < layout->binding_count; b++) {
                     if (!has_samplers ||
      layout->bindings[b].immutable_sampler_idx == ~0 ||
               struct dzn_descriptor_set_ptr ptr;
   const struct dzn_sampler **sampler =
         for (dzn_descriptor_set_ptr_init(set->layout, &ptr, b, 0);
      dzn_descriptor_set_ptr_is_valid(&ptr);
   dzn_descriptor_set_ptr_move(set->layout, &ptr, 1)) {
   dzn_descriptor_set_ptr_write_sampler_desc(device, set, &ptr, *sampler);
            }
      }
      static void
   dzn_descriptor_set_finish(struct dzn_descriptor_set *set)
   {
      vk_object_base_finish(&set->base);
   set->pool = NULL;
      }
      static void
   dzn_descriptor_pool_destroy(struct dzn_descriptor_pool *pool,
         {
      if (!pool)
                     if (device->bindless) {
      if (pool->bindless.buf)
      } else {
      dzn_foreach_pool_type(type) {
      if (pool->desc_count[type])
                  vk_object_base_finish(&pool->base);
      }
      static VkResult
   dzn_descriptor_pool_create(struct dzn_device *device,
                     {
      VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct dzn_descriptor_pool, pool, 1);
            if (!vk_multialloc_zalloc2(&ma, &device->vk.alloc, pAllocator,
                  pool->alloc = pAllocator ? *pAllocator : device->vk.alloc;
   pool->sets = sets;
                     for (uint32_t p = 0; p < pCreateInfo->poolSizeCount; p++) {
      VkDescriptorType type = pCreateInfo->pPoolSizes[p].type;
            if (device->bindless) {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
         default:
      pool->desc_count[0] += num_desc;
         } else {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      pool->desc_count[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] += num_desc;
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      pool->desc_count[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] += num_desc;
   pool->desc_count[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] += num_desc;
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      pool->desc_count[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] += num_desc;
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      /* Reserve one UAV and one SRV slot for those. */
   pool->desc_count[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] += num_desc * 2;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
         default:
                        if (device->bindless) {
      if (pool->desc_count[0]) {
      /* Include extra descriptors so that we can align each allocated descriptor set to a 16-byte boundary */
   static_assert(D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT / sizeof(struct dxil_spirv_bindless_entry) == 2,
                        /* Going to raw APIs to avoid allocating descriptors for this */
   D3D12_RESOURCE_DESC buf_desc = {
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
   .Width = pool->desc_count[0] * sizeof(struct dxil_spirv_bindless_entry),
   .Height = 1, .DepthOrArraySize = 1,
   .MipLevels = 1, .SampleDesc = {.Count = 1},
      };
   D3D12_HEAP_PROPERTIES heap_props = { .Type = D3D12_HEAP_TYPE_UPLOAD };
   if (FAILED(ID3D12Device_CreateCommittedResource(device->dev, &heap_props, 0, &buf_desc, D3D12_RESOURCE_STATE_GENERIC_READ,
            dzn_descriptor_pool_destroy(pool, pAllocator);
      }
   pool->bindless.gpuva = ID3D12Resource_GetGPUVirtualAddress(pool->bindless.buf);
         } else {
      dzn_foreach_pool_type(type) {
                     VkResult result =
         if (result != VK_SUCCESS) {
      dzn_descriptor_pool_destroy(pool, pAllocator);
                     *out = dzn_descriptor_pool_to_handle(pool);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateDescriptorPool(VkDevice device,
                     {
      return dzn_descriptor_pool_create(dzn_device_from_handle(device),
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroyDescriptorPool(VkDevice device,
               {
      dzn_descriptor_pool_destroy(dzn_descriptor_pool_from_handle(descriptorPool),
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_ResetDescriptorPool(VkDevice device,
               {
               for (uint32_t s = 0; s < pool->set_count; s++)
            dzn_foreach_pool_type(type) {
      pool->free_offset[type] = 0;
      }
               }
      void
   dzn_descriptor_heap_pool_finish(struct dzn_descriptor_heap_pool *pool)
   {
      list_splicetail(&pool->active_heaps, &pool->free_heaps);
   list_for_each_entry_safe(struct dzn_descriptor_heap_pool_entry, entry, &pool->free_heaps, link) {
      list_del(&entry->link);
   dzn_descriptor_heap_finish(&entry->heap);
         }
      void
   dzn_descriptor_heap_pool_init(struct dzn_descriptor_heap_pool *pool,
                           {
      assert(!shader_visible ||
                  pool->alloc = alloc;
   pool->type = type;
   pool->shader_visible = shader_visible;
   list_inithead(&pool->active_heaps);
   list_inithead(&pool->free_heaps);
   pool->offset = 0;
      }
      VkResult
   dzn_descriptor_heap_pool_alloc_slots(struct dzn_descriptor_heap_pool *pool,
                     {
      struct dzn_descriptor_heap *last_heap =
      list_is_empty(&pool->active_heaps) ?
   NULL :
      uint32_t last_heap_desc_count =
            if (pool->offset + desc_count > last_heap_desc_count) {
      uint32_t granularity =
      (pool->type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
   pool->type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) ?
      uint32_t alloc_step = ALIGN_POT(desc_count * pool->desc_sz, granularity);
            /* Maximum of 2048 samplers per heap when shader_visible is true. */
   if (pool->shader_visible &&
      pool->type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
   assert(desc_count <= MAX_DESCS_PER_SAMPLER_HEAP);
                        list_for_each_entry_safe(struct dzn_descriptor_heap_pool_entry, entry, &pool->free_heaps, link) {
      if (entry->heap.desc_count >= heap_desc_count) {
      new_heap = entry;
   list_del(&entry->link);
                  if (!new_heap) {
      new_heap =
      vk_zalloc(pool->alloc, sizeof(*new_heap), 8,
                     VkResult result =
      dzn_descriptor_heap_init(&new_heap->heap, device, pool->type,
      if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, new_heap);
                  list_addtail(&new_heap->link, &pool->active_heaps);
   pool->offset = 0;
               *heap = last_heap;
   *first_slot = pool->offset;
   pool->offset += desc_count;
      }
      void
   dzn_descriptor_heap_pool_reset(struct dzn_descriptor_heap_pool *pool)
   {
      pool->offset = 0;
   list_splicetail(&pool->active_heaps, &pool->free_heaps);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_AllocateDescriptorSets(VkDevice dev,
               {
      VK_FROM_HANDLE(dzn_descriptor_pool, pool, pAllocateInfo->descriptorPool);
            const struct VkDescriptorSetVariableDescriptorCountAllocateInfo *variable_counts =
            uint32_t set_idx = 0;
   for (unsigned i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
      VK_FROM_HANDLE(dzn_descriptor_set_layout, layout, pAllocateInfo->pSetLayouts[i]);
   uint32_t additional_desc_counts[NUM_POOL_TYPES];
   additional_desc_counts[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] =
      variable_counts && variable_counts->descriptorSetCount ?
      additional_desc_counts[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = 0;
            dzn_foreach_pool_type(type) {
      total_desc_count[type] = layout->range_desc_count[type] + additional_desc_counts[type];
   if (pool->used_desc_count[type] + total_desc_count[type] > pool->desc_count[type]) {
      dzn_FreeDescriptorSets(dev, pAllocateInfo->descriptorPool, i, pDescriptorSets);
                  struct dzn_descriptor_set *set = NULL;
   bool is_reuse = false;
   bool found_any_unused = false;
   for (; set_idx < pool->set_count; set_idx++) {
      /* Find a free set */
   if (!pool->sets[set_idx].layout) {
      /* Found one. If it's re-use, check if it has enough space */
   if (set_idx < pool->used_set_count) {
      is_reuse = true;
   found_any_unused = true;
   dzn_foreach_pool_type(type) {
      if (pool->sets[set_idx].heap_sizes[type] < total_desc_count[type]) {
      /* No, not enough space */
   is_reuse = false;
         }
   if (!is_reuse)
      }
   set = &pool->sets[set_idx];
                  /* Either all occupied, or no free space large enough */
   if (!set) {
      dzn_FreeDescriptorSets(dev, pAllocateInfo->descriptorPool, i, pDescriptorSets);
               /* Couldn't find one for re-use, check if there's enough space at the end */
   if (!is_reuse) {
      dzn_foreach_pool_type(type) {
      if (pool->free_offset[type] + total_desc_count[type] > pool->desc_count[type]) {
      dzn_FreeDescriptorSets(dev, pAllocateInfo->descriptorPool, i, pDescriptorSets);
                     VkResult result = dzn_descriptor_set_init(set, device, pool, layout, is_reuse, additional_desc_counts);
   if (result != VK_SUCCESS) {
      dzn_FreeDescriptorSets(dev, pAllocateInfo->descriptorPool, i, pDescriptorSets);
               pool->used_set_count = MAX2(pool->used_set_count, set_idx + 1);
   dzn_foreach_pool_type(type)
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_FreeDescriptorSets(VkDevice dev,
                     {
               for (uint32_t s = 0; s < count; s++) {
               if (!set)
            assert(set->pool == pool);
   dzn_foreach_pool_type(type)
                        pool->used_set_count = 0;
   dzn_foreach_pool_type(type)
            for (uint32_t s = 0; s < pool->set_count; s++) {
               if (set->layout) {
      pool->used_set_count = MAX2(pool->used_set_count, s + 1);
   dzn_foreach_pool_type (type) {
      pool->free_offset[type] =
      MAX2(pool->free_offset[type],
                           }
      static void
   dzn_descriptor_set_write(struct dzn_device *device,
         {
                        dzn_descriptor_set_ptr_init(set->layout, &ptr,
                        uint32_t d = 0;
   bool cube_as_2darray =
            switch (pDescriptorWrite->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      for (; dzn_descriptor_set_ptr_is_valid(&ptr) && d < desc_count;
      dzn_descriptor_set_ptr_move(set->layout, &ptr, 1)) {
   assert(dzn_descriptor_set_ptr_get_vk_type(set->layout, &ptr) == pDescriptorWrite->descriptorType);
                                    }
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      for (; dzn_descriptor_set_ptr_is_valid(&ptr) && d < desc_count;
      dzn_descriptor_set_ptr_move(set->layout, &ptr, 1)) {
   assert(dzn_descriptor_set_ptr_get_vk_type(set->layout, &ptr) == pDescriptorWrite->descriptorType);
   const VkDescriptorImageInfo *pImageInfo = pDescriptorWrite->pImageInfo + d;
                                 if (iview)
                     }
         case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      for (; dzn_descriptor_set_ptr_is_valid(&ptr) && d < desc_count;
      dzn_descriptor_set_ptr_move(set->layout, &ptr, 1)) {
   assert(dzn_descriptor_set_ptr_get_vk_type(set->layout, &ptr) == pDescriptorWrite->descriptorType);
                  if (iview)
                     }
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      for (; dzn_descriptor_set_ptr_is_valid(&ptr) && d < desc_count;
      dzn_descriptor_set_ptr_move(set->layout, &ptr, 1)) {
   assert(dzn_descriptor_set_ptr_get_vk_type(set->layout, &ptr) == pDescriptorWrite->descriptorType);
   const VkDescriptorBufferInfo *binfo = &pDescriptorWrite->pBufferInfo[d];
   struct dzn_buffer_desc desc = {
      pDescriptorWrite->descriptorType,
   dzn_buffer_from_handle(binfo->buffer),
                                 }
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      for (; dzn_descriptor_set_ptr_is_valid(&ptr) && d < desc_count;
      dzn_descriptor_set_ptr_move(set->layout, &ptr, 1)) {
   assert(dzn_descriptor_set_ptr_get_vk_type(set->layout, &ptr) == pDescriptorWrite->descriptorType);
   const VkDescriptorBufferInfo *binfo = &pDescriptorWrite->pBufferInfo[d];
   struct dzn_buffer_desc desc = {
      pDescriptorWrite->descriptorType,
   dzn_buffer_from_handle(binfo->buffer),
                                 }
         case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (; dzn_descriptor_set_ptr_is_valid(&ptr) && d < desc_count;
      dzn_descriptor_set_ptr_move(set->layout, &ptr, 1)) {
                  if (bview)
                     }
         default:
      unreachable("invalid descriptor type");
                  }
      static void
   dzn_descriptor_set_copy(struct dzn_device *device,
         {
      VK_FROM_HANDLE(dzn_descriptor_set, src_set, pDescriptorCopy->srcSet);
   VK_FROM_HANDLE(dzn_descriptor_set, dst_set, pDescriptorCopy->dstSet);
            dzn_descriptor_set_ptr_init(src_set->layout, &src_ptr,
               dzn_descriptor_set_ptr_init(dst_set->layout, &dst_ptr,
                           while (dzn_descriptor_set_ptr_is_valid(&src_ptr) &&
         dzn_descriptor_set_ptr_is_valid(&dst_ptr) &&
      VkDescriptorType src_type =
         ASSERTED VkDescriptorType dst_type =
            assert(src_type == dst_type);
   uint32_t count =
                  if (src_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
      src_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
   uint32_t src_idx =
                        memcpy(&dst_set->dynamic_buffers[dst_idx],
            } else {
      dzn_foreach_pool_type(type) {
      uint32_t src_heap_offset =
                        if (src_heap_offset == ~0) {
                                       if (device->bindless) {
      memcpy((void *)&dst_set->pool->bindless.map[dst_heap_offset],
         (const void *)&src_set->pool->bindless.map[src_heap_offset],
   /* There's never a reason to loop and memcpy again for bindless */
      } else {
      dzn_descriptor_heap_copy(device,
                                       if (dzn_descriptor_type_depends_on_shader_usage(src_type, device->bindless)) {
      src_heap_offset = src_set->heap_offsets[type] +
         dst_heap_offset = dst_set->heap_offsets[type] +
         assert(src_heap_offset != ~0);
   assert(dst_heap_offset != ~0);
   dzn_descriptor_heap_copy(device,
                                             dzn_descriptor_set_ptr_move(src_set->layout, &src_ptr, count);
   dzn_descriptor_set_ptr_move(dst_set->layout, &dst_ptr, count);
                  }
      VKAPI_ATTR void VKAPI_CALL
   dzn_UpdateDescriptorSets(VkDevice _device,
                           {
      VK_FROM_HANDLE(dzn_device, device, _device);
   for (unsigned i = 0; i < descriptorWriteCount; i++)
            for (unsigned i = 0; i < descriptorCopyCount; i++)
      }
      static void
   dzn_descriptor_update_template_destroy(struct dzn_descriptor_update_template *templ,
         {
      if (!templ)
            struct dzn_device *device =
            vk_object_base_finish(&templ->base);
      }
      static VkResult
   dzn_descriptor_update_template_create(struct dzn_device *device,
                     {
                        uint32_t entry_count = 0;
   for (uint32_t e = 0; e < info->descriptorUpdateEntryCount; e++) {
      struct dzn_descriptor_set_ptr ptr;
   dzn_descriptor_set_ptr_init(set_layout, &ptr,
               uint32_t desc_count = info->pDescriptorUpdateEntries[e].descriptorCount;
   ASSERTED VkDescriptorType type = info->pDescriptorUpdateEntries[e].descriptorType;
            while (dzn_descriptor_set_ptr_is_valid(&ptr) && d < desc_count) {
               assert(dzn_descriptor_set_ptr_get_vk_type(set_layout, &ptr) == type);
   d += ndescs;
   dzn_descriptor_set_ptr_move(set_layout, &ptr, ndescs);
                           VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct dzn_descriptor_update_template, templ, 1);
            if (!vk_multialloc_zalloc2(&ma, &device->vk.alloc, alloc,
                  vk_object_base_init(&device->vk, &templ->base, VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE);
   templ->entry_count = entry_count;
                     for (uint32_t e = 0; e < info->descriptorUpdateEntryCount; e++) {
      struct dzn_descriptor_set_ptr ptr;
   dzn_descriptor_set_ptr_init(set_layout, &ptr,
               uint32_t desc_count = info->pDescriptorUpdateEntries[e].descriptorCount;
   VkDescriptorType type = info->pDescriptorUpdateEntries[e].descriptorType;
   size_t user_data_offset = info->pDescriptorUpdateEntries[e].offset;
   size_t user_data_stride = info->pDescriptorUpdateEntries[e].stride;
            while (dzn_descriptor_set_ptr_is_valid(&ptr) && d < desc_count) {
               entry->type = type;
   entry->desc_count = MIN2(desc_count - d, ndescs);
   entry->user_data.stride = user_data_stride;
                  assert(dzn_descriptor_set_ptr_get_vk_type(set_layout, &ptr) == type);
   if (dzn_desc_type_has_sampler(type)) {
      entry->heap_offsets.sampler =
      dzn_descriptor_set_ptr_get_heap_offset(set_layout,
                  if (is_dynamic_desc_type(type)) {
         } else if (type != VK_DESCRIPTOR_TYPE_SAMPLER) {
                     entry->heap_offsets.cbv_srv_uav =
      dzn_descriptor_set_ptr_get_heap_offset(set_layout,
            if (dzn_descriptor_type_depends_on_shader_usage(type, device->bindless)) {
      entry->heap_offsets.extra_srv =
      dzn_descriptor_set_ptr_get_heap_offset(set_layout,
                     d += ndescs;
   dzn_descriptor_set_ptr_move(set_layout, &ptr, ndescs);
   user_data_offset += user_data_stride * ndescs;
                  *out = dzn_descriptor_update_template_to_handle(templ);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateDescriptorUpdateTemplate(VkDevice device,
                     {
      return dzn_descriptor_update_template_create(dzn_device_from_handle(device),
            }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroyDescriptorUpdateTemplate(VkDevice device,
               {
      dzn_descriptor_update_template_destroy(dzn_descriptor_update_template_from_handle(descriptorUpdateTemplate),
      }
      static const void *
   dzn_descriptor_update_template_get_desc_data(const struct dzn_descriptor_update_template *templ,
               {
      return (const void *)((const uint8_t *)user_data +
            }
      VKAPI_ATTR void VKAPI_CALL
   dzn_UpdateDescriptorSetWithTemplate(VkDevice _device,
                     {
      VK_FROM_HANDLE(dzn_device, device, _device);
   VK_FROM_HANDLE(dzn_descriptor_set, set, descriptorSet);
            for (uint32_t e = 0; e < templ->entry_count; e++) {
      const struct dzn_descriptor_update_template_entry *entry = &templ->entries[e];
   bool cube_as_2darray =
            switch (entry->type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      for (uint32_t d = 0; d < entry->desc_count; d++) {
      const VkDescriptorImageInfo *info = (const VkDescriptorImageInfo *)
                  if (sampler)
                  case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      for (uint32_t d = 0; d < entry->desc_count; d++) {
      const VkDescriptorImageInfo *info = (const VkDescriptorImageInfo *)
                                       if (iview)
      dzn_descriptor_set_write_image_view_desc(device, entry->type, set,
                     case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      for (uint32_t d = 0; d < entry->desc_count; d++) {
      const VkDescriptorImageInfo *info = (const VkDescriptorImageInfo *)
         uint32_t heap_offset = entry->heap_offsets.cbv_srv_uav + d;
   uint32_t alt_heap_offset =
                        if (iview)
      dzn_descriptor_set_write_image_view_desc(device, entry->type, set,
         }
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      for (uint32_t d = 0; d < entry->desc_count; d++) {
      const VkDescriptorBufferInfo *info = (const VkDescriptorBufferInfo *)
         uint32_t heap_offset = entry->heap_offsets.cbv_srv_uav + d;
   uint32_t alt_heap_offset =
                  struct dzn_buffer_desc desc = {
      entry->type,
                     if (desc.buffer)
      dzn_descriptor_set_write_buffer_desc(device, entry->type, set, heap_offset,
               case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      for (uint32_t d = 0; d < entry->desc_count; d++) {
      const VkDescriptorBufferInfo *info = (const VkDescriptorBufferInfo *)
                  struct dzn_buffer_desc desc = {
      entry->type,
                     if (desc.buffer)
                  case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t d = 0; d < entry->desc_count; d++) {
      VkBufferView *info = (VkBufferView *)
         VK_FROM_HANDLE(dzn_buffer_view, bview, *info);
   uint32_t heap_offset = entry->heap_offsets.cbv_srv_uav + d;
   uint32_t alt_heap_offset =
                  if (bview)
                  default:
               }
