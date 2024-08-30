   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_buffer.h"
      #include "venus-protocol/vn_protocol_driver_buffer.h"
   #include "venus-protocol/vn_protocol_driver_buffer_view.h"
      #include "vn_android.h"
   #include "vn_device.h"
   #include "vn_device_memory.h"
   #include "vn_physical_device.h"
      /* buffer commands */
      static inline uint64_t
   vn_buffer_get_cache_index(const VkBufferCreateInfo *create_info,
         {
      /* For simplicity, cache only when below conditions are met:
   * - pNext is NULL
   * - VK_SHARING_MODE_EXCLUSIVE or VK_SHARING_MODE_CONCURRENT across all
   *
   * Combine sharing mode, flags and usage bits to form a unique index.
   *
   * Btw, we assume VkBufferCreateFlagBits won't exhaust all 32bits, at least
   * no earlier than VkBufferUsageFlagBits.
   */
            const bool is_exclusive =
         const bool is_concurrent =
      create_info->sharingMode == VK_SHARING_MODE_CONCURRENT &&
      if (create_info->size <= cache->max_buffer_size &&
      create_info->pNext == NULL && (is_exclusive || is_concurrent)) {
   return (uint64_t)is_concurrent << 63 |
               /* index being zero suggests uncachable since usage must not be zero */
      }
      static inline uint64_t
   vn_buffer_get_max_buffer_size(struct vn_physical_device *physical_dev)
   {
      /* Without maintenance4, hardcode the min of supported drivers:
   * - anv:  1ull << 30
   * - radv: UINT32_MAX - 4
   * - tu:   UINT32_MAX + 1
   * - lvp:  UINT32_MAX
   * - mali: UINT32_MAX
   */
   static const uint64_t safe_max_buffer_size = 1ULL << 30;
   return physical_dev->base.base.supported_features.maintenance4
            }
      void
   vn_buffer_cache_init(struct vn_device *dev)
   {
               dev->buffer_cache.max_buffer_size =
         dev->buffer_cache.queue_family_count =
            simple_mtx_init(&dev->buffer_cache.mutex, mtx_plain);
   util_sparse_array_init(&dev->buffer_cache.entries,
      }
      static void
   vn_buffer_cache_debug_dump(struct vn_buffer_cache *cache)
   {
      vn_log(NULL, "dumping buffer cache statistics");
   vn_log(NULL, "  cache hit: %d", cache->debug.cache_hit_count);
   vn_log(NULL, "  cache miss: %d", cache->debug.cache_miss_count);
      }
      void
   vn_buffer_cache_fini(struct vn_device *dev)
   {
      util_sparse_array_finish(&dev->buffer_cache.entries);
            if (VN_DEBUG(CACHE))
      }
      static inline uint32_t
   vn_buffer_get_ahb_memory_type_bits(struct vn_device *dev)
   {
      struct vn_buffer_cache *cache = &dev->buffer_cache;
   if (unlikely(!cache->ahb_mem_type_bits_valid)) {
      simple_mtx_lock(&cache->mutex);
   if (!cache->ahb_mem_type_bits_valid) {
      cache->ahb_mem_type_bits =
            }
                  }
      static inline VkDeviceSize
   vn_buffer_get_aligned_memory_requirement_size(VkDeviceSize size,
         {
      /* TODO remove comment after mandating VK_KHR_maintenance4
   *
   * This is based on below implementation defined behavior:
   *    req.size <= align64(info.size, req.alignment)
   */
      }
      static struct vn_buffer_cache_entry *
   vn_buffer_get_cached_memory_requirements(
      struct vn_buffer_cache *cache,
   const VkBufferCreateInfo *create_info,
      {
      if (VN_PERF(NO_ASYNC_BUFFER_CREATE))
            /* 12.7. Resource Memory Association
   *
   * The memoryTypeBits member is identical for all VkBuffer objects created
   * with the same value for the flags and usage members in the
   * VkBufferCreateInfo structure and the handleTypes member of the
   * VkExternalMemoryBufferCreateInfo structure passed to vkCreateBuffer.
   */
   const uint64_t idx = vn_buffer_get_cache_index(create_info, cache);
   if (idx) {
      struct vn_buffer_cache_entry *entry =
            if (entry->valid) {
               out->memory.memoryRequirements.size =
                     } else {
                                          }
      static void
   vn_buffer_cache_entry_init(struct vn_buffer_cache *cache,
               {
               /* Entry might have already been initialized by another thread
   * before the lock
   */
   if (entry->valid)
                     const VkMemoryDedicatedRequirements *dedicated_req =
         if (dedicated_req)
                  unlock:
               /* ensure invariance of the memory requirement size */
   req->memoryRequirements.size =
      vn_buffer_get_aligned_memory_requirement_size(
         }
      static void
   vn_copy_cached_memory_requirements(
      const struct vn_buffer_memory_requirements *cached,
      {
      union {
      VkBaseOutStructure *pnext;
   VkMemoryRequirements2 *two;
               while (u.pnext) {
      switch (u.pnext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2:
      u.two->memoryRequirements = cached->memory.memoryRequirements;
      case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS:
      u.dedicated->prefersDedicatedAllocation =
         u.dedicated->requiresDedicatedAllocation =
            default:
         }
         }
      static VkResult
   vn_buffer_init(struct vn_device *dev,
               {
      VkDevice dev_handle = vn_device_to_handle(dev);
   VkBuffer buf_handle = vn_buffer_to_handle(buf);
   struct vn_buffer_cache *cache = &dev->buffer_cache;
            /* If cacheable and mem requirements found in cache, make async call */
   struct vn_buffer_cache_entry *entry =
      vn_buffer_get_cached_memory_requirements(cache, create_info,
         /* Check size instead of entry->valid to be lock free */
   if (buf->requirements.memory.memoryRequirements.size) {
      vn_async_vkCreateBuffer(dev->instance, dev_handle, create_info, NULL,
                     /* If cache miss or not cacheable, make synchronous call */
   result = vn_call_vkCreateBuffer(dev->instance, dev_handle, create_info,
         if (result != VK_SUCCESS)
            buf->requirements.memory.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
   buf->requirements.memory.pNext = &buf->requirements.dedicated;
   buf->requirements.dedicated.sType =
                  vn_call_vkGetBufferMemoryRequirements2(
      dev->instance, dev_handle,
   &(VkBufferMemoryRequirementsInfo2){
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
      },
         /* If cacheable, store mem requirements from the synchronous call */
   if (entry)
               }
      VkResult
   vn_buffer_create(struct vn_device *dev,
                     {
      struct vn_buffer *buf = NULL;
            buf = vk_zalloc(alloc, sizeof(*buf), VN_DEFAULT_ALIGN,
         if (!buf)
                     result = vn_buffer_init(dev, create_info, buf);
   if (result != VK_SUCCESS) {
      vn_object_base_fini(&buf->base);
   vk_free(alloc, buf);
                           }
      struct vn_buffer_create_info {
      VkBufferCreateInfo create;
   VkExternalMemoryBufferCreateInfo external;
      };
      static const VkBufferCreateInfo *
   vn_buffer_fix_create_info(
      const VkBufferCreateInfo *create_info,
   const VkExternalMemoryHandleTypeFlagBits renderer_handle_type,
      {
      local_info->create = *create_info;
            vk_foreach_struct_const(src, create_info->pNext) {
      void *next = NULL;
   switch (src->sType) {
   case VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO:
      memcpy(&local_info->external, src, sizeof(local_info->external));
   local_info->external.handleTypes = renderer_handle_type;
   next = &local_info->external;
      case VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO:
      memcpy(&local_info->capture, src, sizeof(local_info->capture));
   next = &local_info->capture;
      default:
                  if (next) {
      cur->pNext = next;
                              }
      VkResult
   vn_CreateBuffer(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
         const VkExternalMemoryHandleTypeFlagBits renderer_handle_type =
            struct vn_buffer_create_info local_info;
   const VkExternalMemoryBufferCreateInfo *external_info =
      vk_find_struct_const(pCreateInfo->pNext,
      if (external_info && external_info->handleTypes &&
      external_info->handleTypes != renderer_handle_type) {
   pCreateInfo = vn_buffer_fix_create_info(
               struct vn_buffer *buf;
   VkResult result = vn_buffer_create(dev, pCreateInfo, alloc, &buf);
   if (result != VK_SUCCESS)
            if (external_info &&
      external_info->handleTypes ==
         /* AHB backed buffer layers on top of renderer external memory, so here
   * we combine the queried type bits from both buffer memory requirement
   * and renderer external memory properties.
   */
   buf->requirements.memory.memoryRequirements.memoryTypeBits &=
                                    }
      void
   vn_DestroyBuffer(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_buffer *buf = vn_buffer_from_handle(buffer);
   const VkAllocationCallbacks *alloc =
            if (!buf)
                     vn_object_base_fini(&buf->base);
      }
      VkDeviceAddress
   vn_GetBufferDeviceAddress(VkDevice device,
         {
                  }
      uint64_t
   vn_GetBufferOpaqueCaptureAddress(VkDevice device,
         {
               return vn_call_vkGetBufferOpaqueCaptureAddress(dev->instance, device,
      }
      void
   vn_GetBufferMemoryRequirements2(VkDevice device,
               {
               vn_copy_cached_memory_requirements(&buf->requirements,
      }
      VkResult
   vn_BindBufferMemory2(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
            VkBindBufferMemoryInfo *local_infos = NULL;
   for (uint32_t i = 0; i < bindInfoCount; i++) {
      const VkBindBufferMemoryInfo *info = &pBindInfos[i];
   struct vn_device_memory *mem =
         if (!mem->base_memory)
            if (!local_infos) {
      const size_t size = sizeof(*local_infos) * bindInfoCount;
   local_infos = vk_alloc(alloc, size, VN_DEFAULT_ALIGN,
                                    local_infos[i].memory = vn_device_memory_to_handle(mem->base_memory);
      }
   if (local_infos)
            vn_async_vkBindBufferMemory2(dev->instance, device, bindInfoCount,
                        }
      /* buffer view commands */
      VkResult
   vn_CreateBufferView(VkDevice device,
                     {
      struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            struct vn_buffer_view *view =
      vk_zalloc(alloc, sizeof(*view), VN_DEFAULT_ALIGN,
      if (!view)
                     VkBufferView view_handle = vn_buffer_view_to_handle(view);
   vn_async_vkCreateBufferView(dev->instance, device, pCreateInfo, NULL,
                        }
      void
   vn_DestroyBufferView(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
   struct vn_buffer_view *view = vn_buffer_view_from_handle(bufferView);
   const VkAllocationCallbacks *alloc =
            if (!view)
                     vn_object_base_fini(&view->base);
      }
      void
   vn_GetDeviceBufferMemoryRequirements(
      VkDevice device,
   const VkDeviceBufferMemoryRequirements *pInfo,
      {
      struct vn_device *dev = vn_device_from_handle(device);
   struct vn_buffer_cache *cache = &dev->buffer_cache;
            /* If cacheable and mem requirements found in cache, skip host call */
   struct vn_buffer_cache_entry *entry =
      vn_buffer_get_cached_memory_requirements(cache, pInfo->pCreateInfo,
         /* Check size instead of entry->valid to be lock free */
   if (reqs.memory.memoryRequirements.size) {
      vn_copy_cached_memory_requirements(&reqs, pMemoryRequirements);
               /* Make the host call if not found in cache or not cacheable */
   vn_call_vkGetDeviceBufferMemoryRequirements(dev->instance, device, pInfo,
            /* If cacheable, store mem requirements from the host call */
   if (entry)
      }
