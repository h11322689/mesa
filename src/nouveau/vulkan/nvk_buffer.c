   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_buffer.h"
      #include "nvk_entrypoints.h"
   #include "nvk_device.h"
   #include "nvk_device_memory.h"
   #include "nvk_physical_device.h"
      uint32_t
   nvk_get_buffer_alignment(UNUSED const struct nv_device_info *info,
               {
               if (usage_flags & VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT_KHR)
            if (usage_flags & VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT_KHR)
            if (usage_flags & (VK_BUFFER_USAGE_2_UNIFORM_TEXEL_BUFFER_BIT_KHR |
                  if (create_flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateBuffer(VkDevice device,
                     {
      VK_FROM_HANDLE(nvk_device, dev, device);
            if (pCreateInfo->size > NVK_MAX_BUFFER_SIZE)
            buffer = vk_buffer_create(&dev->vk, pCreateInfo, pAllocator,
         if (!buffer)
            if (buffer->vk.size > 0 &&
      (buffer->vk.create_flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT)) {
   const uint32_t alignment =
      nvk_get_buffer_alignment(&nvk_device_physical(dev)->info,
            assert(alignment >= 4096);
            const bool sparse_residency =
            buffer->addr = nouveau_ws_alloc_vma(dev->ws_dev, buffer->vma_size_B,
                           }
      VKAPI_ATTR void VKAPI_CALL
   nvk_DestroyBuffer(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            if (!buffer)
            if (buffer->vma_size_B > 0) {
      const bool sparse_residency =
            nouveau_ws_bo_unbind_vma(dev->ws_dev, buffer->addr, buffer->vma_size_B);
   nouveau_ws_free_vma(dev->ws_dev, buffer->addr, buffer->vma_size_B,
                  }
      VKAPI_ATTR void VKAPI_CALL
   nvk_GetDeviceBufferMemoryRequirements(
      VkDevice device,
   const VkDeviceBufferMemoryRequirements *pInfo,
      {
               const uint32_t alignment =
      nvk_get_buffer_alignment(&nvk_device_physical(dev)->info,
               pMemoryRequirements->memoryRequirements = (VkMemoryRequirements) {
      .size = align64(pInfo->pCreateInfo->size, alignment),
   .alignment = alignment,
               vk_foreach_struct_const(ext, pMemoryRequirements->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS: {
      VkMemoryDedicatedRequirements *dedicated = (void *)ext;
   dedicated->prefersDedicatedAllocation = false;
   dedicated->requiresDedicatedAllocation = false;
      }
   default:
      nvk_debug_ignored_stype(ext->sType);
            }
      VKAPI_ATTR void VKAPI_CALL
   nvk_GetPhysicalDeviceExternalBufferProperties(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo,
      {
      /* The Vulkan 1.3.256 spec says:
   *
   *    VUID-VkPhysicalDeviceExternalBufferInfo-handleType-parameter
   *
   *    "handleType must be a valid VkExternalMemoryHandleTypeFlagBits value"
   *
   * This differs from VkPhysicalDeviceExternalImageFormatInfo, which
   * surprisingly permits handleType == 0.
   */
            /* All of the current flags are for sparse which we don't support yet.
   * Even when we do support it, doing sparse on external memory sounds
   * sketchy.  Also, just disallowing flags is the safe option.
   */
   if (pExternalBufferInfo->flags)
            switch (pExternalBufferInfo->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      pExternalBufferProperties->externalMemoryProperties =
            default:
               unsupported:
      /* From the Vulkan 1.3.256 spec:
   *
   *    compatibleHandleTypes must include at least handleType.
   */
   pExternalBufferProperties->externalMemoryProperties =
      (VkExternalMemoryProperties) {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_BindBufferMemory2(VkDevice device,
               {
      for (uint32_t i = 0; i < bindInfoCount; ++i) {
      VK_FROM_HANDLE(nvk_device_memory, mem, pBindInfos[i].memory);
            buffer->is_local = !(mem->bo->flags & NOUVEAU_WS_BO_GART);
   if (buffer->vma_size_B) {
      VK_FROM_HANDLE(nvk_device, dev, device);
   if (mem != NULL) {
      nouveau_ws_bo_bind_vma(dev->ws_dev,
                        mem->bo,
   } else {
      nouveau_ws_bo_unbind_vma(dev->ws_dev,
               } else {
      buffer->addr =
         }
      }
      VKAPI_ATTR VkDeviceAddress VKAPI_CALL
   nvk_GetBufferDeviceAddress(UNUSED VkDevice device,
         {
                  }
      VKAPI_ATTR uint64_t VKAPI_CALL
   nvk_GetBufferOpaqueCaptureAddress(UNUSED VkDevice device,
         {
                  }
