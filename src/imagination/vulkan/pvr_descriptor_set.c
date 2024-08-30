   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <assert.h>
   #include <limits.h>
   #include <stdint.h>
   #include <stdlib.h>
   #include <string.h>
   #include <vulkan/vulkan.h>
      #include "hwdef/rogue_hw_utils.h"
   #include "pvr_bo.h"
   #include "pvr_debug.h"
   #include "pvr_private.h"
   #include "pvr_types.h"
   #include "util/compiler.h"
   #include "util/list.h"
   #include "util/log.h"
   #include "util/macros.h"
   #include "vk_alloc.h"
   #include "vk_format.h"
   #include "vk_log.h"
   #include "vk_object.h"
   #include "vk_util.h"
      static const struct {
      const char *raw;
   const char *primary;
   const char *secondary;
   const char *primary_dynamic;
      } stage_names[] = {
      { "Vertex",
   "Vertex Primary",
   "Vertex Secondary",
   "Vertex Dynamic Primary",
   "Vertex Dynamic Secondary" },
   { "Fragment",
   "Fragment Primary",
   "Fragment Secondary",
   "Fragment Dynamic Primary",
   "Fragment Dynamic Secondary" },
   { "Compute",
   "Compute Primary",
   "Compute Secondary",
   "Compute Dynamic Primary",
      };
      static const char *descriptor_names[] = { "VK SAMPLER",
                                             "VK COMBINED_IMAGE_SAMPLER",
   "VK SAMPLED_IMAGE",
   "VK STORAGE_IMAGE",
      #define PVR_DESC_IMAGE_SECONDARY_OFFSET_ARRAYBASE 0U
   #define PVR_DESC_IMAGE_SECONDARY_SIZE_ARRAYBASE 2U
   #define PVR_DESC_IMAGE_SECONDARY_OFFSET_ARRAYSTRIDE \
      (PVR_DESC_IMAGE_SECONDARY_OFFSET_ARRAYBASE +     \
      #define PVR_DESC_IMAGE_SECONDARY_SIZE_ARRAYSTRIDE 1U
      #define PVR_DESC_IMAGE_SECONDARY_OFFSET_ARRAYMAXINDEX(dev_info) \
      (PVR_HAS_FEATURE(dev_info, tpu_array_textures)               \
      ? (0)                                                    \
   : PVR_DESC_IMAGE_SECONDARY_OFFSET_ARRAYSTRIDE +          \
      #define PVR_DESC_IMAGE_SECONDARY_SIZE_ARRAYMAXINDEX 1U
   #define PVR_DESC_IMAGE_SECONDARY_OFFSET_WIDTH(dev_info)       \
      (PVR_DESC_IMAGE_SECONDARY_OFFSET_ARRAYMAXINDEX(dev_info) + \
      #define PVR_DESC_IMAGE_SECONDARY_SIZE_WIDTH 1U
   #define PVR_DESC_IMAGE_SECONDARY_OFFSET_HEIGHT(dev_info) \
      (PVR_DESC_IMAGE_SECONDARY_OFFSET_WIDTH(dev_info) +    \
      #define PVR_DESC_IMAGE_SECONDARY_SIZE_HEIGHT 1U
   #define PVR_DESC_IMAGE_SECONDARY_OFFSET_DEPTH(dev_info) \
      (PVR_DESC_IMAGE_SECONDARY_OFFSET_HEIGHT(dev_info) +  \
      #define PVR_DESC_IMAGE_SECONDARY_SIZE_DEPTH 1U
   #define PVR_DESC_IMAGE_SECONDARY_TOTAL_SIZE(dev_info) \
      (PVR_DESC_IMAGE_SECONDARY_OFFSET_DEPTH(dev_info) + \
         void pvr_descriptor_size_info_init(
      const struct pvr_device *device,
   VkDescriptorType type,
      {
      /* UINT_MAX is a place holder. These values will be filled by calling the
   * init function, and set appropriately based on device features.
   */
   static const struct pvr_descriptor_size_info template_size_infos[] = {
      /* VK_DESCRIPTOR_TYPE_SAMPLER */
   { PVR_SAMPLER_DESCRIPTOR_SIZE, 0, 4 },
   /* VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER */
   { PVR_IMAGE_DESCRIPTOR_SIZE + PVR_SAMPLER_DESCRIPTOR_SIZE, UINT_MAX, 4 },
   /* VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE */
   { 4, UINT_MAX, 4 },
   /* VK_DESCRIPTOR_TYPE_STORAGE_IMAGE */
   { 4, UINT_MAX, 4 },
   /* VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER */
   { 4, UINT_MAX, 4 },
   /* VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER */
   { 4, UINT_MAX, 4 },
   /* VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER */
   { 2, UINT_MAX, 2 },
   /* VK_DESCRIPTOR_TYPE_STORAGE_BUFFER */
   { 2, 1, 2 },
   /* VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC */
   { 2, UINT_MAX, 2 },
   /* VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC */
   { 2, 1, 2 },
   /* VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT */
                        switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      size_info_out->secondary =
               case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      size_info_out->secondary =
               default:
            }
      static uint8_t vk_to_pvr_shader_stage_flags(VkShaderStageFlags vk_flags)
   {
                        if (vk_flags & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT))
            if (vk_flags & VK_SHADER_STAGE_FRAGMENT_BIT)
            if (vk_flags & VK_SHADER_STAGE_COMPUTE_BIT)
               }
      /* If allocator == NULL, the internal one will be used. */
   static struct pvr_descriptor_set_layout *
   pvr_descriptor_set_layout_allocate(struct pvr_device *device,
                           {
      struct pvr_descriptor_set_layout_binding *bindings;
   struct pvr_descriptor_set_layout *layout;
   __typeof__(layout->per_stage_descriptor_count) counts;
            VK_MULTIALLOC(ma);
   vk_multialloc_add(&ma, &layout, __typeof__(*layout), 1);
   vk_multialloc_add(&ma, &bindings, __typeof__(*bindings), binding_count);
   vk_multialloc_add(&ma,
                        for (uint32_t stage = 0; stage < ARRAY_SIZE(counts); stage++) {
      vk_multialloc_add(&ma,
                           /* pvr_CreateDescriptorSetLayout() relies on this being zero allocated. */
   if (!vk_multialloc_zalloc2(&ma,
                                    layout->bindings = bindings;
                     vk_object_base_init(&device->vk,
                     }
      /* If allocator == NULL, the internal one will be used. */
   static void
   pvr_descriptor_set_layout_free(struct pvr_device *device,
               {
      vk_object_base_finish(&layout->base);
      }
      static int pvr_binding_compare(const void *a, const void *b)
   {
      uint32_t binding_a = ((VkDescriptorSetLayoutBinding *)a)->binding;
            if (binding_a < binding_b)
            if (binding_a > binding_b)
               }
      /* If allocator == NULL, the internal one will be used. */
   static VkDescriptorSetLayoutBinding *
   pvr_create_sorted_bindings(struct pvr_device *device,
                     {
      VkDescriptorSetLayoutBinding *sorted_bindings =
      vk_alloc2(&device->vk.alloc,
            allocator,
   binding_count * sizeof(*sorted_bindings),
   if (!sorted_bindings)
                     qsort(sorted_bindings,
         binding_count,
               }
      struct pvr_register_usage {
      uint32_t primary;
   uint32_t primary_dynamic;
   uint32_t secondary;
      };
      static void pvr_setup_in_memory_layout_sizes(
      struct pvr_descriptor_set_layout *layout,
      {
      for (uint32_t stage = 0;
      stage < ARRAY_SIZE(layout->memory_layout_in_dwords_per_stage);
   stage++) {
            layout->memory_layout_in_dwords_per_stage[stage].primary_offset =
         layout->memory_layout_in_dwords_per_stage[stage].primary_size =
            layout->total_size_in_dwords += reg_usage[stage].primary;
            layout->memory_layout_in_dwords_per_stage[stage].secondary_offset =
         layout->memory_layout_in_dwords_per_stage[stage].secondary_size =
                              layout->memory_layout_in_dwords_per_stage[stage].primary_dynamic_size =
                  layout->memory_layout_in_dwords_per_stage[stage].secondary_dynamic_size =
         layout->total_dynamic_size_in_dwords +=
         }
      static void
   pvr_dump_in_memory_layout_sizes(const struct pvr_descriptor_set_layout *layout)
   {
      const char *const separator =
         const char *const big_separator =
            mesa_logd("=== SET LAYOUT ===");
   mesa_logd("%s", separator);
   mesa_logd(" in memory:");
            for (uint32_t stage = 0;
      stage < ARRAY_SIZE(layout->memory_layout_in_dwords_per_stage);
   stage++) {
   mesa_logd(
      "| %-18s @   %04u                |",
   stage_names[stage].primary,
               /* Print primaries. */
   for (uint32_t i = 0; i < layout->binding_count; i++) {
                     if (binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                  mesa_logd("|   %s %04u | %-26s[%3u] |",
            (binding->shader_stage_mask & (1U << stage)) ? " " : "X",
   binding->per_stage_offset_in_dwords[stage].primary,
            /* Print dynamic primaries. */
   for (uint32_t i = 0; i < layout->binding_count; i++) {
                     if (binding->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC &&
                  mesa_logd("| * %s %04u | %-26s[%3u] |",
            (binding->shader_stage_mask & (1U << stage)) ? " " : "X",
   binding->per_stage_offset_in_dwords[stage].primary,
            mesa_logd("%s", separator);
   mesa_logd(
      "| %-18s @   %04u                |",
   stage_names[stage].secondary,
               /* Print secondaries. */
   for (uint32_t i = 0; i < layout->binding_count; i++) {
                     if (binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                  mesa_logd("|   %s %04u | %-26s[%3u] |",
            (binding->shader_stage_mask & (1U << stage)) ? " " : "X",
   binding->per_stage_offset_in_dwords[stage].secondary,
            /* Print dynamic secondaries. */
   for (uint32_t i = 0; i < layout->binding_count; i++) {
                     if (binding->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC &&
                  mesa_logd("| * %s %04u | %-26s[%3u] |",
            (binding->shader_stage_mask & (1U << stage)) ? " " : "X",
   binding->per_stage_offset_in_dwords[stage].secondary,
                  }
      VkResult pvr_CreateDescriptorSetLayout(
      VkDevice _device,
   const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
   const VkAllocationCallbacks *pAllocator,
      {
      /* Used to accumulate sizes and set each descriptor's offsets per stage. */
   struct pvr_register_usage reg_usage[PVR_STAGE_ALLOCATION_COUNT] = { 0 };
   PVR_FROM_HANDLE(pvr_device, device, _device);
   struct pvr_descriptor_set_layout *layout;
   VkDescriptorSetLayoutBinding *bindings;
            assert(pCreateInfo->sType ==
            vk_foreach_struct_const (ext, pCreateInfo->pNext) {
                  if (pCreateInfo->bindingCount == 0) {
      layout = pvr_descriptor_set_layout_allocate(device, pAllocator, 0, 0, 0);
   if (!layout)
            *pSetLayout = pvr_descriptor_set_layout_to_handle(layout);
               /* TODO: Instead of sorting, maybe do what anvil does? */
   bindings = pvr_create_sorted_bindings(device,
                     if (!bindings)
            immutable_sampler_count = 0;
   for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
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
   const VkDescriptorType descriptor_type = bindings[i].descriptorType;
   if ((descriptor_type == VK_DESCRIPTOR_TYPE_SAMPLER ||
      descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
   bindings[i].pImmutableSamplers)
            /* From the Vulkan 1.2.190 spec for VkDescriptorSetLayoutCreateInfo:
   *
   *     "The VkDescriptorSetLayoutBinding::binding members of the elements
   *     of the pBindings array must each have different values."
   *
   * So we don't worry about duplicates and just allocate for bindingCount
   * amount of bindings.
   */
   layout = pvr_descriptor_set_layout_allocate(
      device,
   pAllocator,
   pCreateInfo->bindingCount,
   immutable_sampler_count,
      if (!layout) {
      vk_free2(&device->vk.alloc, pAllocator, bindings);
                        for (uint32_t bind_num = 0; bind_num < layout->binding_count; bind_num++) {
      const VkDescriptorSetLayoutBinding *const binding = &bindings[bind_num];
   struct pvr_descriptor_set_layout_binding *const internal_binding =
                  internal_binding->type = binding->descriptorType;
            /* From Vulkan spec 1.2.189:
   *
   *    "If descriptorCount is zero this binding entry is reserved and the
   *    resource must not be accessed from any stage via this binding"
   *
   * So do not use bindings->stageFlags, use shader_stages instead.
   */
   if (binding->descriptorCount) {
               internal_binding->descriptor_count = binding->descriptorCount;
   internal_binding->descriptor_index = layout->descriptor_count;
               switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      if (binding->pImmutableSamplers && binding->descriptorCount > 0) {
      internal_binding->has_immutable_samplers = true;
                  for (uint32_t j = 0; j < binding->descriptorCount; j++) {
      PVR_FROM_HANDLE(pvr_sampler,
                                                   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            default:
      unreachable("Unknown descriptor type");
               if (!shader_stages)
            internal_binding->shader_stage_mask = shader_stages;
            for (uint32_t stage = 0;
      stage < ARRAY_SIZE(layout->bindings[0].per_stage_offset_in_dwords);
                                 /* We don't allocate any space for dynamic primaries and secondaries.
   * They will be all be collected together in the pipeline layout.
   * Having them all in one place makes updating them easier when the
   * user updates the dynamic offsets.
   */
   if (descriptor_type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC &&
                              STATIC_ASSERT(
                                 internal_binding->per_stage_offset_in_dwords[stage].primary =
                        internal_binding->per_stage_offset_in_dwords[stage].secondary =
         reg_usage[stage].secondary +=
               STATIC_ASSERT(
                  layout->per_stage_descriptor_count[stage][descriptor_type] +=
                  for (uint32_t bind_num = 0; bind_num < layout->binding_count; bind_num++) {
      struct pvr_descriptor_set_layout_binding *const internal_binding =
                  if (descriptor_type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC &&
                  for (uint32_t stage = 0;
      stage < ARRAY_SIZE(layout->bindings[0].per_stage_offset_in_dwords);
                                          /* TODO: align primary like we did with other descriptors? */
   internal_binding->per_stage_offset_in_dwords[stage].primary =
                        internal_binding->per_stage_offset_in_dwords[stage].secondary =
         reg_usage[stage].secondary_dynamic +=
                           if (PVR_IS_DEBUG_SET(VK_DUMP_DESCRIPTOR_SET_LAYOUT))
                                 }
      void pvr_DestroyDescriptorSetLayout(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_descriptor_set_layout, layout, _set_layout);
            if (!layout)
               }
      static void
   pvr_dump_in_register_layout_sizes(const struct pvr_device *device,
         {
      const char *const separator =
         const char *const big_separator =
            mesa_logd("=== SET LAYOUT ===");
   mesa_logd("%s", separator);
   mesa_logd(" in registers:");
            for (uint32_t stage = 0;
      stage < ARRAY_SIZE(layout->register_layout_in_dwords_per_stage);
   stage++) {
            mesa_logd("| %-64s |", stage_names[stage].primary_dynamic);
            if (layout->per_stage_reg_info[stage].primary_dynamic_size_in_dwords) {
      /* Print dynamic primaries. */
   for (uint32_t set_num = 0; set_num < layout->set_count; set_num++) {
                     for (uint32_t i = 0; i < set_layout->binding_count; i++) {
                                                               mesa_logd("| +%04u | set = %u, binding = %03u | %-26s[%3u] |",
            dynamic_offset,
                                                mesa_logd("%s", separator);
   mesa_logd("| %-64s |", stage_names[stage].secondary_dynamic);
            if (layout->per_stage_reg_info[stage].secondary_dynamic_size_in_dwords) {
      /* Print dynamic secondaries. */
   for (uint32_t set_num = 0; set_num < layout->set_count; set_num++) {
      const struct pvr_descriptor_set_layout *const set_layout =
                                       for (uint32_t i = 0; i < set_layout->binding_count; i++) {
                                                               mesa_logd("| +%04u | set = %u, binding = %03u | %-26s[%3u] |",
            dynamic_offset,
                                                mesa_logd("%s", separator);
   mesa_logd("| %-64s |", stage_names[stage].primary);
            /* Print primaries. */
   for (uint32_t set_num = 0; set_num < layout->set_count; set_num++) {
      const struct pvr_descriptor_set_layout *const set_layout =
         const uint32_t base =
                  for (uint32_t i = 0; i < set_layout->binding_count; i++) {
                                    if (binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                  mesa_logd("| +%04u | set = %u, binding = %03u | %-26s[%3u] |",
            base + binding->per_stage_offset_in_dwords[stage].primary,
   set_num,
   i,
               mesa_logd("%s", separator);
   mesa_logd("| %-64s |", stage_names[stage].secondary);
            /* Print secondaries. */
   for (uint32_t set_num = 0; set_num < layout->set_count; set_num++) {
      const struct pvr_descriptor_set_layout *const set_layout =
         const struct pvr_descriptor_set_layout_mem_layout *const mem_layout =
                                 for (uint32_t i = 0; i < set_layout->binding_count; i++) {
                                    if (binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                  mesa_logd("| +%04u | set = %u, binding = %03u | %-26s[%3u] |",
            base +
         set_num,
   i,
                     }
      /* Pipeline layouts. These have nothing to do with the pipeline. They are
   * just multiple descriptor set layouts pasted together.
   */
   VkResult pvr_CreatePipelineLayout(VkDevice _device,
                     {
      uint32_t next_free_reg[PVR_STAGE_ALLOCATION_COUNT];
   PVR_FROM_HANDLE(pvr_device, device, _device);
            assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
            layout = vk_object_alloc(&device->vk,
                     if (!layout)
            layout->set_count = pCreateInfo->setLayoutCount;
   layout->shader_stage_mask = 0;
   for (uint32_t stage = 0; stage < PVR_STAGE_ALLOCATION_COUNT; stage++) {
      uint32_t descriptor_counts
         struct pvr_pipeline_layout_reg_info *const reg_info =
                              for (uint32_t set_num = 0; set_num < layout->set_count; set_num++) {
      /* So we don't write these again and again. Just do it once. */
   if (stage == 0) {
      PVR_FROM_HANDLE(pvr_descriptor_set_layout,
                  layout->set_layout[set_num] = set_layout;
               const struct pvr_descriptor_set_layout_mem_layout *const mem_layout =
                  /* Allocate registers counts for dynamic descriptors. */
   reg_info->primary_dynamic_size_in_dwords +=
                        for (VkDescriptorType type = 0;
      type < PVR_PIPELINE_LAYOUT_SUPPORTED_DESCRIPTOR_TYPE_COUNT;
                                                                              switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                        /* We don't need to keep track of the counts or masks for other
   * descriptor types so there is no assert() here since other
   * types are not invalid or unsupported.
   */
   /* TODO: Improve the comment above to specify why, when we find
   * out.
   */
   default:
                        next_free_reg[stage] = reg_info->primary_dynamic_size_in_dwords +
               /* Allocate registers counts for primary and secondary descriptors. */
   for (uint32_t stage = 0; stage < PVR_STAGE_ALLOCATION_COUNT; stage++) {
      for (uint32_t set_num = 0; set_num < layout->set_count; set_num++) {
      const struct pvr_descriptor_set_layout_mem_layout *const mem_layout =
      &layout->set_layout[set_num]
                                                         /* To optimize the total shared layout allocation used by the shader,
   * secondary descriptors come last since they're less likely to be used.
   */
   for (uint32_t set_num = 0; set_num < layout->set_count; set_num++) {
      const struct pvr_descriptor_set_layout_mem_layout *const mem_layout =
      &layout->set_layout[set_num]
                     /* Should we be aligning next_free_reg like it's done with the
                                                layout->push_constants_shader_stages = 0;
   for (uint32_t i = 0; i < pCreateInfo->pushConstantRangeCount; i++) {
               layout->push_constants_shader_stages |=
            /* From the Vulkan spec. 1.3.237
   * VUID-VkPipelineLayoutCreateInfo-pPushConstantRanges-00292 :
   *
   *    "Any two elements of pPushConstantRanges must not include the same
   *     stage in stageFlags"
   */
   if (range->stageFlags & VK_SHADER_STAGE_VERTEX_BIT)
            if (range->stageFlags & VK_SHADER_STAGE_FRAGMENT_BIT)
            if (range->stageFlags & VK_SHADER_STAGE_COMPUTE_BIT)
               if (PVR_IS_DEBUG_SET(VK_DUMP_DESCRIPTOR_SET_LAYOUT))
                        }
      void pvr_DestroyPipelineLayout(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            if (!layout)
               }
      VkResult pvr_CreateDescriptorPool(VkDevice _device,
                     {
      PVR_FROM_HANDLE(pvr_device, device, _device);
                     pool = vk_object_alloc(&device->vk,
                     if (!pool)
            if (pAllocator)
         else
            pool->max_sets = pCreateInfo->maxSets;
            pool->total_size_in_dwords = 0;
   for (uint32_t i = 0; i < pCreateInfo->poolSizeCount; i++) {
      struct pvr_descriptor_size_info size_info;
   const uint32_t descriptor_count =
            pvr_descriptor_size_info_init(device,
                  const uint32_t secondary = ALIGN_POT(size_info.secondary, 4);
               }
   pool->total_size_in_dwords *= PVR_STAGE_ALLOCATION_COUNT;
                                 }
      static void pvr_free_descriptor_set(struct pvr_device *device,
               {
      list_del(&set->link);
   pvr_bo_suballoc_free(set->pvr_bo);
      }
      void pvr_DestroyDescriptorPool(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            if (!pool)
            list_for_each_entry_safe (struct pvr_descriptor_set,
                                       }
      VkResult pvr_ResetDescriptorPool(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_descriptor_pool, pool, descriptorPool);
            list_for_each_entry_safe (struct pvr_descriptor_set,
                                                }
      static uint16_t pvr_get_descriptor_primary_offset(
      const struct pvr_device *device,
   const struct pvr_descriptor_set_layout *layout,
   const struct pvr_descriptor_set_layout_binding *binding,
   const uint32_t stage,
      {
      struct pvr_descriptor_size_info size_info;
            assert(stage < ARRAY_SIZE(layout->memory_layout_in_dwords_per_stage));
                     offset = layout->memory_layout_in_dwords_per_stage[stage].primary_offset;
   offset += binding->per_stage_offset_in_dwords[stage].primary;
            /* Offset must be less than 16bits. */
               }
      static uint16_t pvr_get_descriptor_secondary_offset(
      const struct pvr_device *device,
   const struct pvr_descriptor_set_layout *layout,
   const struct pvr_descriptor_set_layout_binding *binding,
   const uint32_t stage,
      {
      struct pvr_descriptor_size_info size_info;
            assert(stage < ARRAY_SIZE(layout->memory_layout_in_dwords_per_stage));
                     offset = layout->memory_layout_in_dwords_per_stage[stage].secondary_offset;
   offset += binding->per_stage_offset_in_dwords[stage].secondary;
            /* Offset must be less than 16bits. */
               }
      #define PVR_MAX_DESCRIPTOR_MEM_SIZE_IN_DWORDS (4 * 1024)
      static VkResult
   pvr_descriptor_set_create(struct pvr_device *device,
                     {
      struct pvr_descriptor_set *set;
   VkResult result;
                     /* TODO: Add support to allocate descriptors from descriptor pool, also
   * check the required descriptors must not exceed max allowed descriptors.
   */
   set = vk_object_zalloc(&device->vk,
                     if (!set)
            /* TODO: Add support to allocate device memory from a common pool. Look at
   * something like anv. Also we can allocate a whole chunk of device memory
   * for max descriptors supported by pool as done by v3dv. Also check the
   * possibility if this can be removed from here and done on need basis.
            if (layout->binding_count > 0) {
      const uint32_t cache_line_size =
         uint64_t bo_size = MIN2(pool->total_size_in_dwords,
                  result = pvr_bo_suballoc(&device->suballoc_general,
                           if (result != VK_SUCCESS)
               set->layout = layout;
            for (uint32_t i = 0; i < layout->binding_count; i++) {
      const struct pvr_descriptor_set_layout_binding *binding =
            if (binding->descriptor_count == 0 || !binding->has_immutable_samplers)
            for (uint32_t stage = 0;
      stage < ARRAY_SIZE(binding->per_stage_offset_in_dwords);
   stage++) {
                  for (uint32_t j = 0; j < binding->descriptor_count; j++) {
      uint32_t idx = binding->immutable_samplers_index + j;
   const struct pvr_sampler *sampler = layout->immutable_samplers[idx];
   unsigned int offset_in_dwords =
      pvr_get_descriptor_primary_offset(device,
                                          memcpy((uint8_t *)pvr_bo_suballoc_get_map_addr(set->pvr_bo) +
                                                         err_free_descriptor_set:
                  }
      VkResult
   pvr_AllocateDescriptorSets(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_descriptor_pool, pool, pAllocateInfo->descriptorPool);
   PVR_FROM_HANDLE(pvr_device, device, _device);
   VkResult result;
            vk_foreach_struct_const (ext, pAllocateInfo->pNext) {
                  for (i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
      PVR_FROM_HANDLE(pvr_descriptor_set_layout,
                        result = pvr_descriptor_set_create(device, pool, layout, &set);
   if (result != VK_SUCCESS)
                              err_free_descriptor_sets:
      pvr_FreeDescriptorSets(_device,
                        for (i = 0; i < pAllocateInfo->descriptorSetCount; i++)
               }
      VkResult pvr_FreeDescriptorSets(VkDevice _device,
                     {
      PVR_FROM_HANDLE(pvr_descriptor_pool, pool, descriptorPool);
            for (uint32_t i = 0; i < count; i++) {
               if (!pDescriptorSets[i])
            set = pvr_descriptor_set_from_handle(pDescriptorSets[i]);
                  }
      static void pvr_descriptor_update_buffer_info(
      const struct pvr_device *device,
   const VkWriteDescriptorSet *write_set,
   struct pvr_descriptor_set *set,
   const struct pvr_descriptor_set_layout_binding *binding,
   uint32_t *mem_ptr,
   uint32_t start_stage,
      {
      struct pvr_descriptor_size_info size_info;
            is_dynamic = (binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                     for (uint32_t i = 0; i < write_set->descriptorCount; i++) {
      const VkDescriptorBufferInfo *buffer_info = &write_set->pBufferInfo[i];
   PVR_FROM_HANDLE(pvr_buffer, buffer, buffer_info->buffer);
   const uint32_t desc_idx =
         const pvr_dev_addr_t addr =
         const uint32_t whole_range = buffer->vk.size - buffer_info->offset;
   uint32_t range = (buffer_info->range == VK_WHOLE_SIZE)
                  set->descriptors[desc_idx].type = write_set->descriptorType;
   set->descriptors[desc_idx].buffer_dev_addr = addr;
   set->descriptors[desc_idx].buffer_whole_range = whole_range;
            if (is_dynamic)
            /* Update the entries in the descriptor memory for static buffer. */
   for (uint32_t j = start_stage; j < end_stage; j++) {
                                    /* Offset calculation functions expect descriptor_index to be
   * binding relative not layout relative, so we have used
   * write_set->dstArrayElement + i rather than desc_idx.
   */
   primary_offset =
      pvr_get_descriptor_primary_offset(device,
                        secondary_offset =
      pvr_get_descriptor_secondary_offset(device,
                           memcpy(mem_ptr + primary_offset,
         &addr,
   memcpy(mem_ptr + secondary_offset,
                  }
      static void pvr_descriptor_update_sampler(
      const struct pvr_device *device,
   const VkWriteDescriptorSet *write_set,
   struct pvr_descriptor_set *set,
   const struct pvr_descriptor_set_layout_binding *binding,
   uint32_t *mem_ptr,
   uint32_t start_stage,
      {
                        for (uint32_t i = 0; i < write_set->descriptorCount; i++) {
      PVR_FROM_HANDLE(pvr_sampler, sampler, write_set->pImageInfo[i].sampler);
   const uint32_t desc_idx =
            set->descriptors[desc_idx].type = write_set->descriptorType;
            for (uint32_t j = start_stage; j < end_stage; j++) {
                              /* Offset calculation functions expect descriptor_index to be binding
   * relative not layout relative, so we have used
   * write_set->dstArrayElement + i rather than desc_idx.
   */
   primary_offset =
      pvr_get_descriptor_primary_offset(device,
                           memcpy(mem_ptr + primary_offset,
                  }
      static void
   pvr_write_image_descriptor_primaries(const struct pvr_device_info *dev_info,
                     {
               if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE &&
      (iview->vk.view_type == VK_IMAGE_VIEW_TYPE_CUBE ||
   iview->vk.view_type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)) {
   qword_ptr[0] = iview->texture_state[PVR_TEXTURE_STATE_STORAGE][0];
      } else if (descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
      qword_ptr[0] = iview->texture_state[PVR_TEXTURE_STATE_ATTACHMENT][0];
      } else {
      qword_ptr[0] = iview->texture_state[PVR_TEXTURE_STATE_SAMPLE][0];
               if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE &&
      !PVR_HAS_FEATURE(dev_info, tpu_extended_integer_lookup)) {
            pvr_csb_pack (&tmp, TEXSTATE_STRIDE_IMAGE_WORD1, word1) {
                        }
      static void
   pvr_write_image_descriptor_secondaries(const struct pvr_device_info *dev_info,
                     {
      const bool cube_array_adjust =
      descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE &&
         if (!PVR_HAS_FEATURE(dev_info, tpu_array_textures)) {
      const struct pvr_image *image = pvr_image_view_get_image(iview);
   uint64_t addr =
            secondary[PVR_DESC_IMAGE_SECONDARY_OFFSET_ARRAYBASE] = (uint32_t)addr;
   secondary[PVR_DESC_IMAGE_SECONDARY_OFFSET_ARRAYBASE + 1U] =
            secondary[PVR_DESC_IMAGE_SECONDARY_OFFSET_ARRAYSTRIDE] =
               if (cube_array_adjust) {
      secondary[PVR_DESC_IMAGE_SECONDARY_OFFSET_ARRAYMAXINDEX(dev_info)] =
      } else {
      secondary[PVR_DESC_IMAGE_SECONDARY_OFFSET_ARRAYMAXINDEX(dev_info)] =
               secondary[PVR_DESC_IMAGE_SECONDARY_OFFSET_WIDTH(dev_info)] =
         secondary[PVR_DESC_IMAGE_SECONDARY_OFFSET_HEIGHT(dev_info)] =
         secondary[PVR_DESC_IMAGE_SECONDARY_OFFSET_DEPTH(dev_info)] =
      }
      static void pvr_descriptor_update_sampler_texture(
      const struct pvr_device *device,
   const VkWriteDescriptorSet *write_set,
   struct pvr_descriptor_set *set,
   const struct pvr_descriptor_set_layout_binding *binding,
   uint32_t *mem_ptr,
   uint32_t start_stage,
      {
               for (uint32_t i = 0; i < write_set->descriptorCount; i++) {
      PVR_FROM_HANDLE(pvr_image_view,
               const uint32_t desc_idx =
            set->descriptors[desc_idx].type = write_set->descriptorType;
   set->descriptors[desc_idx].iview = iview;
            for (uint32_t j = start_stage; j < end_stage; j++) {
                                    /* Offset calculation functions expect descriptor_index to be
   * binding relative not layout relative, so we have used
   * write_set->dstArrayElement + i rather than desc_idx.
   */
   primary_offset =
      pvr_get_descriptor_primary_offset(device,
                        secondary_offset =
      pvr_get_descriptor_secondary_offset(device,
                           pvr_write_image_descriptor_primaries(dev_info,
                        /* We don't need to update the sampler words if they belong to an
   * immutable sampler.
   */
   if (!binding->has_immutable_samplers) {
      PVR_FROM_HANDLE(pvr_sampler,
                        /* Sampler words are located at the end of the primary image words.
   */
   memcpy(mem_ptr + primary_offset + PVR_IMAGE_DESCRIPTOR_SIZE,
                     pvr_write_image_descriptor_secondaries(dev_info,
                        }
      static void pvr_descriptor_update_texture(
      const struct pvr_device *device,
   const VkWriteDescriptorSet *write_set,
   struct pvr_descriptor_set *set,
   const struct pvr_descriptor_set_layout_binding *binding,
   uint32_t *mem_ptr,
   uint32_t start_stage,
      {
               for (uint32_t i = 0; i < write_set->descriptorCount; i++) {
      PVR_FROM_HANDLE(pvr_image_view,
               const uint32_t desc_idx =
            set->descriptors[desc_idx].type = write_set->descriptorType;
   set->descriptors[desc_idx].iview = iview;
            for (uint32_t j = start_stage; j < end_stage; j++) {
                                    /* Offset calculation functions expect descriptor_index to be
   * binding relative not layout relative, so we have used
   * write_set->dstArrayElement + i rather than desc_idx.
   */
   primary_offset =
      pvr_get_descriptor_primary_offset(device,
                        secondary_offset =
      pvr_get_descriptor_secondary_offset(device,
                           pvr_write_image_descriptor_primaries(dev_info,
                        pvr_write_image_descriptor_secondaries(dev_info,
                        }
      static void pvr_write_buffer_descriptor(const struct pvr_device_info *dev_info,
                           {
               qword_ptr[0] = bview->texture_state[0];
            if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER &&
      !PVR_HAS_FEATURE(dev_info, tpu_extended_integer_lookup)) {
            pvr_csb_pack (&tmp, TEXSTATE_STRIDE_IMAGE_WORD1, word1) {
                              if (secondary) {
      /* NOTE: Range check for texture buffer writes is not strictly required as
   * we have already validated that the index is in range. We'd need a
   * compiler change to allow us to skip the range check.
   */
   secondary[PVR_DESC_IMAGE_SECONDARY_OFFSET_WIDTH(dev_info)] =
               }
      static void pvr_descriptor_update_buffer_view(
      const struct pvr_device *device,
   const VkWriteDescriptorSet *write_set,
   struct pvr_descriptor_set *set,
   const struct pvr_descriptor_set_layout_binding *binding,
   uint32_t *mem_ptr,
   uint32_t start_stage,
      {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
                     for (uint32_t i = 0; i < write_set->descriptorCount; i++) {
      PVR_FROM_HANDLE(pvr_buffer_view, bview, write_set->pTexelBufferView[i]);
   const uint32_t desc_idx =
            set->descriptors[desc_idx].type = write_set->descriptorType;
            for (uint32_t j = start_stage; j < end_stage; j++) {
                                    /* Offset calculation functions expect descriptor_index to be
   * binding relative not layout relative, so we have used
   * write_set->dstArrayElement + i rather than desc_idx.
   */
   primary_offset =
      pvr_get_descriptor_primary_offset(device,
                        secondary_offset =
      pvr_get_descriptor_secondary_offset(device,
                           pvr_write_buffer_descriptor(
      dev_info,
   bview,
   write_set->descriptorType,
   mem_ptr + primary_offset,
         }
      static void pvr_descriptor_update_input_attachment(
      const struct pvr_device *device,
   const VkWriteDescriptorSet *write_set,
   struct pvr_descriptor_set *set,
   const struct pvr_descriptor_set_layout_binding *binding,
   uint32_t *mem_ptr,
   uint32_t start_stage,
      {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
                     for (uint32_t i = 0; i < write_set->descriptorCount; i++) {
      PVR_FROM_HANDLE(pvr_image_view,
               const uint32_t desc_idx =
            set->descriptors[desc_idx].type = write_set->descriptorType;
   set->descriptors[desc_idx].iview = iview;
            for (uint32_t j = start_stage; j < end_stage; j++) {
                              /* Offset calculation functions expect descriptor_index to be
   * binding relative not layout relative, so we have used
   * write_set->dstArrayElement + i rather than desc_idx.
   */
   primary_offset =
      pvr_get_descriptor_primary_offset(device,
                           pvr_write_image_descriptor_primaries(dev_info,
                                       if (!PVR_HAS_FEATURE(dev_info, tpu_array_textures)) {
      const uint32_t secondary_offset =
      pvr_get_descriptor_secondary_offset(device,
                                 pvr_write_image_descriptor_secondaries(dev_info,
                           }
      static void pvr_write_descriptor_set(struct pvr_device *device,
         {
      PVR_FROM_HANDLE(pvr_descriptor_set, set, write_set->dstSet);
   uint32_t *map = pvr_bo_suballoc_get_map_addr(set->pvr_bo);
   const struct pvr_descriptor_set_layout_binding *binding =
            /* Binding should not be NULL. */
            /* Only need to update the descriptor if it is actually being used. If it
   * was not used in any stage, then the shader_stage_mask would be 0 and we
   * can skip this update.
   */
   if (binding->shader_stage_mask == 0)
            vk_foreach_struct_const (ext, write_set->pNext) {
                  switch (write_set->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      pvr_descriptor_update_sampler(device,
                                 write_set,
         case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      pvr_descriptor_update_sampler_texture(device,
                                             case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      pvr_descriptor_update_texture(device,
                                 write_set,
         case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      pvr_descriptor_update_buffer_view(device,
                                             case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      pvr_descriptor_update_input_attachment(device,
                                             case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      pvr_descriptor_update_buffer_info(device,
                                             default:
      unreachable("Unknown descriptor type");
         }
      static void pvr_copy_descriptor_set(struct pvr_device *device,
         {
      PVR_FROM_HANDLE(pvr_descriptor_set, src_set, copy_set->srcSet);
   PVR_FROM_HANDLE(pvr_descriptor_set, dst_set, copy_set->dstSet);
   const struct pvr_descriptor_set_layout_binding *src_binding =
         const struct pvr_descriptor_set_layout_binding *dst_binding =
         struct pvr_descriptor_size_info size_info;
   uint32_t *src_mem_ptr;
            switch (src_binding->type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
      const uint32_t src_idx =
         const uint32_t dst_idx =
            for (uint32_t j = 0; j < copy_set->descriptorCount; j++) {
                                       default:
      unreachable("Unknown descriptor type");
               /* Dynamic buffer descriptors don't have any data stored in the descriptor
   * set memory. They only exist in the set->descriptors list which we've
   * already updated above.
   */
   if (src_binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
      src_binding->type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
               src_mem_ptr = pvr_bo_suballoc_get_map_addr(src_set->pvr_bo);
            /* From the Vulkan 1.3.232 spec VUID-VkCopyDescriptorSet-dstBinding-02632:
   *
   *    The type of dstBinding within dstSet must be equal to the type of
   *    srcBinding within srcSet.
   *
   * So both bindings have the same descriptor size and we don't need to
   * handle size differences.
   */
                     u_foreach_bit (stage, dst_binding->shader_stage_mask) {
      uint16_t src_secondary_offset;
   uint16_t dst_secondary_offset;
   uint16_t src_primary_offset;
            /* Offset calculation functions expect descriptor_index to be
   * binding relative not layout relative.
   */
   src_primary_offset =
      pvr_get_descriptor_primary_offset(device,
                        dst_primary_offset =
      pvr_get_descriptor_primary_offset(device,
                        src_secondary_offset =
      pvr_get_descriptor_secondary_offset(device,
                        dst_secondary_offset =
      pvr_get_descriptor_secondary_offset(device,
                           memcpy(dst_mem_ptr + dst_primary_offset,
                  memcpy(dst_mem_ptr + dst_secondary_offset,
               }
      void pvr_UpdateDescriptorSets(VkDevice _device,
                           {
               for (uint32_t i = 0; i < descriptorWriteCount; i++)
            for (uint32_t i = 0; i < descriptorCopyCount; i++)
      }
