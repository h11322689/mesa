   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_descriptor_set.h"
      #include "venus-protocol/vn_protocol_driver_descriptor_pool.h"
   #include "venus-protocol/vn_protocol_driver_descriptor_set.h"
   #include "venus-protocol/vn_protocol_driver_descriptor_set_layout.h"
   #include "venus-protocol/vn_protocol_driver_descriptor_update_template.h"
      #include "vn_device.h"
   #include "vn_pipeline.h"
      void
   vn_descriptor_set_layout_destroy(struct vn_device *dev,
         {
      VkDevice dev_handle = vn_device_to_handle(dev);
   VkDescriptorSetLayout layout_handle =
                  vn_async_vkDestroyDescriptorSetLayout(dev->instance, dev_handle,
            vn_object_base_fini(&layout->base);
      }
      static void
   vn_descriptor_set_destroy(struct vn_device *dev,
               {
                        vn_object_base_fini(&set->base);
      }
      /* Mapping VkDescriptorType to array index */
   static enum vn_descriptor_type
   vn_descriptor_type_index(VkDescriptorType type)
   {
      switch (type) {
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
         case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
         case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
         case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
         default:
                     }
      /* descriptor set layout commands */
      void
   vn_GetDescriptorSetLayoutSupport(
      VkDevice device,
   const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
      {
               /* TODO per-device cache */
   vn_call_vkGetDescriptorSetLayoutSupport(dev->instance, device, pCreateInfo,
      }
      static void
   vn_descriptor_set_layout_init(
      struct vn_device *dev,
   const VkDescriptorSetLayoutCreateInfo *create_info,
   uint32_t last_binding,
      {
      VkDevice dev_handle = vn_device_to_handle(dev);
   VkDescriptorSetLayout layout_handle =
         const VkDescriptorSetLayoutBindingFlagsCreateInfo *binding_flags =
      vk_find_struct_const(create_info->pNext,
         const VkMutableDescriptorTypeCreateInfoEXT *mutable_descriptor_info =
      vk_find_struct_const(create_info->pNext,
         /* 14.2.1. Descriptor Set Layout
   *
   * If bindingCount is zero or if this structure is not included in
   * the pNext chain, the VkDescriptorBindingFlags for each descriptor
   * set layout binding is considered to be zero.
   */
   if (binding_flags && !binding_flags->bindingCount)
            layout->is_push_descriptor =
      create_info->flags &
         layout->refcount = VN_REFCOUNT_INIT(1);
            for (uint32_t i = 0; i < create_info->bindingCount; i++) {
      const VkDescriptorSetLayoutBinding *binding_info =
         struct vn_descriptor_set_layout_binding *binding =
            if (binding_info->binding == last_binding) {
      /* 14.2.1. Descriptor Set Layout
   *
   * VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT must only be
   * used for the last binding in the descriptor set layout (i.e. the
   * binding with the largest value of binding).
   *
   * 41. Features
   *
   * descriptorBindingVariableDescriptorCount indicates whether the
   * implementation supports descriptor sets with a variable-sized last
   * binding. If this feature is not enabled,
   * VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT must not be
   * used.
   */
   layout->has_variable_descriptor_count =
      binding_flags &&
   (binding_flags->pBindingFlags[i] &
            binding->type = binding_info->descriptorType;
            switch (binding_info->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      binding->has_immutable_samplers = binding_info->pImmutableSamplers;
      case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
      assert(mutable_descriptor_info->mutableDescriptorTypeListCount &&
                  const VkMutableDescriptorTypeListEXT *list =
         for (uint32_t j = 0; j < list->descriptorTypeCount; j++) {
      BITSET_SET(binding->mutable_descriptor_types,
                  default:
                     vn_async_vkCreateDescriptorSetLayout(dev->instance, dev_handle,
      }
      VkResult
   vn_CreateDescriptorSetLayout(
      VkDevice device,
   const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
   const VkAllocationCallbacks *pAllocator,
      {
      struct vn_device *dev = vn_device_from_handle(device);
   /* ignore pAllocator as the layout is reference-counted */
            uint32_t last_binding = 0;
   VkDescriptorSetLayoutBinding *local_bindings = NULL;
   VkDescriptorSetLayoutCreateInfo local_create_info;
   if (pCreateInfo->bindingCount) {
      /* the encoder does not ignore
   * VkDescriptorSetLayoutBinding::pImmutableSamplers when it should
   */
   const size_t binding_size =
         local_bindings = vk_alloc(alloc, binding_size, VN_DEFAULT_ALIGN,
         if (!local_bindings)
            memcpy(local_bindings, pCreateInfo->pBindings, binding_size);
   for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
                              switch (binding->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
         default:
      binding->pImmutableSamplers = NULL;
                  local_create_info = *pCreateInfo;
   local_create_info.pBindings = local_bindings;
               const size_t layout_size =
         /* allocated with the device scope */
   struct vn_descriptor_set_layout *layout =
      vk_zalloc(alloc, layout_size, VN_DEFAULT_ALIGN,
      if (!layout) {
      vk_free(alloc, local_bindings);
               vn_object_base_init(&layout->base, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                          }
      void
   vn_DestroyDescriptorSetLayout(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
   struct vn_descriptor_set_layout *layout =
            if (!layout)
               }
      /* descriptor pool commands */
      VkResult
   vn_CreateDescriptorPool(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            const VkDescriptorPoolInlineUniformBlockCreateInfo *iub_info =
      vk_find_struct_const(pCreateInfo->pNext,
         uint32_t mutable_states_count = 0;
   for (uint32_t i = 0; i < pCreateInfo->poolSizeCount; i++) {
      const VkDescriptorPoolSize *pool_size = &pCreateInfo->pPoolSizes[i];
   if (pool_size->type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT)
      }
   struct vn_descriptor_pool *pool;
            VK_MULTIALLOC(ma);
   vk_multialloc_add(&ma, &pool, __typeof__(*pool), 1);
   vk_multialloc_add(&ma, &mutable_states, __typeof__(*mutable_states),
            if (!vk_multialloc_zalloc(&ma, alloc, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT))
            vn_object_base_init(&pool->base, VK_OBJECT_TYPE_DESCRIPTOR_POOL,
            pool->allocator = *alloc;
            const VkMutableDescriptorTypeCreateInfoEXT *mutable_descriptor_info =
      vk_find_struct_const(pCreateInfo->pNext,
         /* Without VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, the set
   * allocation must not fail due to a fragmented pool per spec. In this
   * case, set allocation can be asynchronous with pool resource tracking.
   */
   pool->async_set_allocation =
      !VN_PERF(NO_ASYNC_SET_ALLOC) &&
   !(pCreateInfo->flags &
                  if (iub_info)
            uint32_t next_mutable_state = 0;
   for (uint32_t i = 0; i < pCreateInfo->poolSizeCount; i++) {
      const VkDescriptorPoolSize *pool_size = &pCreateInfo->pPoolSizes[i];
                     if (pool_size->type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT) {
      struct vn_descriptor_pool_state_mutable *mutable_state = NULL;
   BITSET_DECLARE(mutable_types, VN_NUM_DESCRIPTOR_TYPES);
   if (!mutable_descriptor_info ||
      i >= mutable_descriptor_info->mutableDescriptorTypeListCount) {
      } else {
                     for (uint32_t j = 0; j < list->descriptorTypeCount; j++) {
      BITSET_SET(mutable_types, vn_descriptor_type_index(
         }
   for (uint32_t j = 0; j < next_mutable_state; j++) {
      if (BITSET_EQUAL(mutable_types, pool->mutable_states[j].types)) {
      mutable_state = &pool->mutable_states[j];
                  if (!mutable_state) {
      /* The application must ensure that partial overlap does not
   * exist in pPoolSizes. so this entry must have a disjoint set
   * of types.
   */
   mutable_state = &pool->mutable_states[next_mutable_state++];
                  } else {
      pool->max.descriptor_counts[type_index] +=
               assert((pCreateInfo->pPoolSizes[i].type !=
                     pool->mutable_states_count = next_mutable_state;
            VkDescriptorPool pool_handle = vn_descriptor_pool_to_handle(pool);
   vn_async_vkCreateDescriptorPool(dev->instance, device, pCreateInfo, NULL,
                        }
      void
   vn_DestroyDescriptorPool(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_descriptor_pool *pool =
                  if (!pool)
                     /* We must emit vkDestroyDescriptorPool before freeing the sets in
   * pool->descriptor_sets.  Otherwise, another thread might reuse their
   * object ids while they still refer to the sets in the renderer.
   */
   vn_async_vkDestroyDescriptorPool(dev->instance, device, descriptorPool,
            list_for_each_entry_safe(struct vn_descriptor_set, set,
                  vn_object_base_fini(&pool->base);
      }
      static struct vn_descriptor_pool_state_mutable *
   vn_find_mutable_state_for_binding(
      const struct vn_descriptor_pool *pool,
      {
      for (uint32_t i = 0; i < pool->mutable_states_count; i++) {
      struct vn_descriptor_pool_state_mutable *mutable_state =
         BITSET_DECLARE(shared_types, VN_NUM_DESCRIPTOR_TYPES);
   BITSET_AND(shared_types, mutable_state->types,
            if (BITSET_EQUAL(shared_types, binding->mutable_descriptor_types)) {
      /* The application must ensure that partial overlap does not
   * exist in pPoolSizes. so there should be at most 1
   * matching pool entry for a binding.
   */
         }
      }
      static void
   vn_pool_restore_mutable_states(struct vn_descriptor_pool *pool,
                     {
      for (uint32_t i = 0; i <= max_binding_index; i++) {
      if (layout->bindings[i].type != VK_DESCRIPTOR_TYPE_MUTABLE_EXT)
            const uint32_t count = i == layout->last_binding
                  struct vn_descriptor_pool_state_mutable *mutable_state =
         assert(mutable_state && mutable_state->used >= count);
         }
      static bool
   vn_descriptor_pool_alloc_descriptors(
      struct vn_descriptor_pool *pool,
   const struct vn_descriptor_set_layout *layout,
      {
               if (!pool->async_set_allocation)
            if (pool->used.set_count == pool->max.set_count)
            /* backup current pool state to recovery */
                              while (binding_index <= layout->last_binding) {
      const VkDescriptorType type = layout->bindings[binding_index].type;
   const uint32_t count = binding_index == layout->last_binding
                  /* Allocation may fail if a call to vkAllocateDescriptorSets would cause
   * the total number of inline uniform block bindings allocated from the
   * pool to exceed the value of
   * VkDescriptorPoolInlineUniformBlockCreateInfo::maxInlineUniformBlockBindings
   * used to create the descriptor pool.
   *
   * If descriptorCount is zero this binding entry is reserved and the
   * resource must not be accessed from any stage via this binding within
   * any pipeline using the set layout.
   */
   if (type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK && count != 0) {
      if (++pool->used.iub_binding_count > pool->max.iub_binding_count)
               if (type == VK_DESCRIPTOR_TYPE_MUTABLE_EXT) {
      /* A mutable descriptor can be allocated if below are satisfied:
   * - vn_descriptor_pool_state_mutable::types is a superset
   * - vn_descriptor_pool_state_mutable::{max - used} is enough
   */
   struct vn_descriptor_pool_state_mutable *mutable_state =
                  if (mutable_state &&
      mutable_state->max - mutable_state->used >= count) {
      } else
      } else {
                     if (pool->used.descriptor_counts[type_index] >
      pool->max.descriptor_counts[type_index])
   }
                     fail:
      /* restore pool state before this allocation */
   pool->used = recovery;
   if (binding_index > 0) {
      vn_pool_restore_mutable_states(pool, layout, binding_index - 1,
      }
      }
      static void
   vn_descriptor_pool_free_descriptors(
      struct vn_descriptor_pool *pool,
   const struct vn_descriptor_set_layout *layout,
      {
      if (!pool->async_set_allocation)
            vn_pool_restore_mutable_states(pool, layout, layout->last_binding,
            for (uint32_t i = 0; i <= layout->last_binding; i++) {
      const uint32_t count = i == layout->last_binding
                  if (layout->bindings[i].type != VK_DESCRIPTOR_TYPE_MUTABLE_EXT) {
                     if (layout->bindings[i].type ==
      VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK)
                  }
      static void
   vn_descriptor_pool_reset_descriptors(struct vn_descriptor_pool *pool)
   {
      if (!pool->async_set_allocation)
                     for (uint32_t i = 0; i < pool->mutable_states_count; i++)
      }
      VkResult
   vn_ResetDescriptorPool(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_descriptor_pool *pool =
                  vn_async_vkResetDescriptorPool(dev->instance, device, descriptorPool,
            list_for_each_entry_safe(struct vn_descriptor_set, set,
                              }
      /* descriptor set commands */
      VkResult
   vn_AllocateDescriptorSets(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_descriptor_pool *pool =
         const VkAllocationCallbacks *alloc = &pool->allocator;
   const VkDescriptorSetVariableDescriptorCountAllocateInfo *variable_info =
                  /* 14.2.3. Allocation of Descriptor Sets
   *
   * If descriptorSetCount is zero or this structure is not included in
   * the pNext chain, then the variable lengths are considered to be zero.
   */
   variable_info = vk_find_struct_const(
      pAllocateInfo->pNext,
         if (variable_info && !variable_info->descriptorSetCount)
            for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
      struct vn_descriptor_set_layout *layout =
         uint32_t last_binding_descriptor_count = 0;
            /* 14.2.3. Allocation of Descriptor Sets
   *
   * If VkDescriptorSetAllocateInfo::pSetLayouts[i] does not include a
   * variable count descriptor binding, then pDescriptorCounts[i] is
   * ignored.
   */
   if (!layout->has_variable_descriptor_count) {
      last_binding_descriptor_count =
      } else if (variable_info) {
                  if (!vn_descriptor_pool_alloc_descriptors(
            pDescriptorSets[i] = VK_NULL_HANDLE;
   result = VK_ERROR_OUT_OF_POOL_MEMORY;
               set = vk_zalloc(alloc, sizeof(*set), VN_DEFAULT_ALIGN,
         if (!set) {
      vn_descriptor_pool_free_descriptors(pool, layout,
         pDescriptorSets[i] = VK_NULL_HANDLE;
   result = VK_ERROR_OUT_OF_HOST_MEMORY;
               vn_object_base_init(&set->base, VK_OBJECT_TYPE_DESCRIPTOR_SET,
            /* We might reorder vkCmdBindDescriptorSets after
   * vkDestroyDescriptorSetLayout due to batching.  The spec says
   *
   *   VkDescriptorSetLayout objects may be accessed by commands that
   *   operate on descriptor sets allocated using that layout, and those
   *   descriptor sets must not be updated with vkUpdateDescriptorSets
   *   after the descriptor set layout has been destroyed. Otherwise, a
   *   VkDescriptorSetLayout object passed as a parameter to create
   *   another object is not further accessed by that object after the
   *   duration of the command it is passed into.
   *
   * It is ambiguous but the reordering is likely invalid.  Let's keep the
   * layout alive with the set to defer vkDestroyDescriptorSetLayout.
   */
   set->layout = vn_descriptor_set_layout_ref(dev, layout);
   set->last_binding_descriptor_count = last_binding_descriptor_count;
            VkDescriptorSet set_handle = vn_descriptor_set_to_handle(set);
               if (pool->async_set_allocation) {
      vn_async_vkAllocateDescriptorSets(dev->instance, device, pAllocateInfo,
      } else {
      result = vn_call_vkAllocateDescriptorSets(
         if (result != VK_SUCCESS)
                     fail:
      for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
      struct vn_descriptor_set *set =
         if (!set)
            vn_descriptor_pool_free_descriptors(pool, set->layout,
                        memset(pDescriptorSets, 0,
               }
      VkResult
   vn_FreeDescriptorSets(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_descriptor_pool *pool =
                  vn_async_vkFreeDescriptorSets(dev->instance, device, descriptorPool,
            for (uint32_t i = 0; i < descriptorSetCount; i++) {
      struct vn_descriptor_set *set =
            if (!set)
                           }
      static struct vn_update_descriptor_sets *
   vn_update_descriptor_sets_alloc(uint32_t write_count,
                                 uint32_t image_count,
   {
      const size_t writes_offset = sizeof(struct vn_update_descriptor_sets);
   const size_t images_offset =
         const size_t buffers_offset =
         const size_t views_offset =
         const size_t iubs_offset =
         const size_t alloc_size =
      iubs_offset +
         void *storage = vk_alloc(alloc, alloc_size, VN_DEFAULT_ALIGN, scope);
   if (!storage)
            struct vn_update_descriptor_sets *update = storage;
   update->write_count = write_count;
   update->writes = storage + writes_offset;
   update->images = storage + images_offset;
   update->buffers = storage + buffers_offset;
   update->views = storage + views_offset;
               }
      bool
   vn_should_sanitize_descriptor_set_writes(
      uint32_t write_count,
   const VkWriteDescriptorSet *writes,
      {
      /* the encoder does not ignore
   * VkWriteDescriptorSet::{pImageInfo,pBufferInfo,pTexelBufferView} when it
   * should
   *
   * TODO make the encoder smarter
   */
   const struct vn_pipeline_layout *pipeline_layout =
         for (uint32_t i = 0; i < write_count; i++) {
      const struct vn_descriptor_set_layout *set_layout =
      pipeline_layout
      ? pipeline_layout->push_descriptor_set_layout
   const struct vn_descriptor_set_layout_binding *binding =
         const VkWriteDescriptorSet *write = &writes[i];
            switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                     for (uint32_t j = 0; j < write->descriptorCount; j++) {
      switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      if (imgs[j].imageView != VK_NULL_HANDLE)
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      if (binding->has_immutable_samplers &&
      imgs[j].sampler != VK_NULL_HANDLE)
         case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      if (imgs[j].sampler != VK_NULL_HANDLE)
            default:
            }
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                        case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
   case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
   default:
      if (write->pImageInfo != NULL || write->pBufferInfo != NULL ||
                                    }
      struct vn_update_descriptor_sets *
   vn_update_descriptor_sets_parse_writes(uint32_t write_count,
                     {
      uint32_t img_count = 0;
   for (uint32_t i = 0; i < write_count; i++) {
      const VkWriteDescriptorSet *write = &writes[i];
   switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      img_count += write->descriptorCount;
      default:
                     struct vn_update_descriptor_sets *update =
      vn_update_descriptor_sets_alloc(write_count, img_count, 0, 0, 0, alloc,
      if (!update)
            /* the encoder does not ignore
   * VkWriteDescriptorSet::{pImageInfo,pBufferInfo,pTexelBufferView} when it
   * should
   *
   * TODO make the encoder smarter
   */
   memcpy(update->writes, writes, sizeof(*writes) * write_count);
   img_count = 0;
   const struct vn_pipeline_layout *pipeline_layout =
         for (uint32_t i = 0; i < write_count; i++) {
      const struct vn_descriptor_set_layout *set_layout =
      pipeline_layout
      ? pipeline_layout->push_descriptor_set_layout
   const struct vn_descriptor_set_layout_binding *binding =
         VkWriteDescriptorSet *write = &update->writes[i];
            switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      memcpy(imgs, write->pImageInfo,
                  for (uint32_t j = 0; j < write->descriptorCount; j++) {
      switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
      imgs[j].imageView = VK_NULL_HANDLE;
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      if (binding->has_immutable_samplers)
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      imgs[j].sampler = VK_NULL_HANDLE;
      default:
                     write->pImageInfo = imgs;
   write->pBufferInfo = NULL;
   write->pTexelBufferView = NULL;
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      write->pImageInfo = NULL;
   write->pBufferInfo = NULL;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      write->pImageInfo = NULL;
   write->pTexelBufferView = NULL;
      case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
   case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
   default:
      write->pImageInfo = NULL;
   write->pBufferInfo = NULL;
   write->pTexelBufferView = NULL;
                     }
      void
   vn_UpdateDescriptorSets(VkDevice device,
                           {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
            struct vn_update_descriptor_sets *update =
      vn_update_descriptor_sets_parse_writes(
      if (!update) {
      /* TODO update one-by-one? */
   vn_log(dev->instance, "TODO descriptor set update ignored due to OOM");
               vn_async_vkUpdateDescriptorSets(dev->instance, device, update->write_count,
                     }
      /* descriptor update template commands */
      static struct vn_update_descriptor_sets *
   vn_update_descriptor_sets_parse_template(
      const VkDescriptorUpdateTemplateCreateInfo *create_info,
   const VkAllocationCallbacks *alloc,
      {
      uint32_t img_count = 0;
   uint32_t buf_count = 0;
   uint32_t view_count = 0;
   uint32_t iub_count = 0;
   for (uint32_t i = 0; i < create_info->descriptorUpdateEntryCount; i++) {
      const VkDescriptorUpdateTemplateEntry *entry =
            switch (entry->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      img_count += entry->descriptorCount;
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      view_count += entry->descriptorCount;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      buf_count += entry->descriptorCount;
      case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      iub_count += 1;
      case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
         default:
      unreachable("unhandled descriptor type");
                  struct vn_update_descriptor_sets *update = vn_update_descriptor_sets_alloc(
      create_info->descriptorUpdateEntryCount, img_count, buf_count,
      if (!update)
            img_count = 0;
   buf_count = 0;
   view_count = 0;
   iub_count = 0;
   for (uint32_t i = 0; i < create_info->descriptorUpdateEntryCount; i++) {
      const VkDescriptorUpdateTemplateEntry *entry =
                  write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   write->pNext = NULL;
   write->dstBinding = entry->dstBinding;
   write->dstArrayElement = entry->dstArrayElement;
   write->descriptorCount = entry->descriptorCount;
            entries[i].offset = entry->offset;
            switch (entry->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      write->pImageInfo = &update->images[img_count];
   write->pBufferInfo = NULL;
   write->pTexelBufferView = NULL;
   img_count += entry->descriptorCount;
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      write->pImageInfo = NULL;
   write->pBufferInfo = NULL;
   write->pTexelBufferView = &update->views[view_count];
   view_count += entry->descriptorCount;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      write->pImageInfo = NULL;
   write->pBufferInfo = &update->buffers[buf_count];
   write->pTexelBufferView = NULL;
   buf_count += entry->descriptorCount;
      case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
      write->pImageInfo = NULL;
   write->pBufferInfo = NULL;
   write->pTexelBufferView = NULL;
   VkWriteDescriptorSetInlineUniformBlock *iub_data =
         iub_data->sType =
         iub_data->pNext = write->pNext;
   iub_data->dataSize = entry->descriptorCount;
   write->pNext = iub_data;
   iub_count += 1;
      default:
                        }
      VkResult
   vn_CreateDescriptorUpdateTemplate(
      VkDevice device,
   const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo,
   const VkAllocationCallbacks *pAllocator,
      {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            const size_t templ_size =
      offsetof(struct vn_descriptor_update_template,
      struct vn_descriptor_update_template *templ = vk_zalloc(
         if (!templ)
            vn_object_base_init(&templ->base,
            templ->update = vn_update_descriptor_sets_parse_template(
         if (!templ->update) {
      vk_free(alloc, templ);
               templ->is_push_descriptor =
      pCreateInfo->templateType ==
      if (templ->is_push_descriptor) {
      templ->pipeline_bind_point = pCreateInfo->pipelineBindPoint;
   templ->pipeline_layout =
                        /* no host object */
   VkDescriptorUpdateTemplate templ_handle =
                     }
      void
   vn_DestroyDescriptorUpdateTemplate(
      VkDevice device,
   VkDescriptorUpdateTemplate descriptorUpdateTemplate,
      {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_descriptor_update_template *templ =
         const VkAllocationCallbacks *alloc =
            if (!templ)
            /* no host object */
   vk_free(alloc, templ->update);
            vn_object_base_fini(&templ->base);
      }
      struct vn_update_descriptor_sets *
   vn_update_descriptor_set_with_template_locked(
      struct vn_descriptor_update_template *templ,
   struct vn_descriptor_set *set,
      {
               for (uint32_t i = 0; i < update->write_count; i++) {
      const struct vn_descriptor_update_template_entry *entry =
            const struct vn_descriptor_set_layout *set_layout =
      templ->is_push_descriptor
      ? templ->pipeline_layout->push_descriptor_set_layout
   const struct vn_descriptor_set_layout_binding *binding =
                     write->dstSet = templ->is_push_descriptor
                  switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      const bool need_sampler =
      (write->descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
   write->descriptorType ==
            const bool need_view =
         const VkDescriptorImageInfo *src =
                        dst->sampler = need_sampler ? src->sampler : VK_NULL_HANDLE;
   dst->imageView = need_view ? src->imageView : VK_NULL_HANDLE;
      }
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      const VkBufferView *src =
         VkBufferView *dst = (VkBufferView *)&write->pTexelBufferView[j];
      }
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      const VkDescriptorBufferInfo *src =
         VkDescriptorBufferInfo *dst =
            }
      case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:;
      VkWriteDescriptorSetInlineUniformBlock *iub_data =
      (VkWriteDescriptorSetInlineUniformBlock *)vk_find_struct_const(
      iub_data->pData = data + entry->offset;
      case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
         default:
      unreachable("unhandled descriptor type");
         }
      }
      void
   vn_UpdateDescriptorSetWithTemplate(
      VkDevice device,
   VkDescriptorSet descriptorSet,
   VkDescriptorUpdateTemplate descriptorUpdateTemplate,
      {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_descriptor_update_template *templ =
         struct vn_descriptor_set *set =
                  struct vn_update_descriptor_sets *update =
            vn_async_vkUpdateDescriptorSets(dev->instance, device, update->write_count,
               }
