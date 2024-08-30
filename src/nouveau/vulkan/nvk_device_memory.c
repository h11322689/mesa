   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_device_memory.h"
      #include "nouveau_bo.h"
      #include "nvk_device.h"
   #include "nvk_entrypoints.h"
   #include "nvk_image.h"
   #include "nvk_physical_device.h"
      #include "nv_push.h"
      #include <inttypes.h>
   #include <sys/mman.h>
      #include "nvtypes.h"
   #include "nvk_cl902d.h"
      /* Supports opaque fd only */
   const VkExternalMemoryProperties nvk_opaque_fd_mem_props = {
      .externalMemoryFeatures =
      VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT |
      .exportFromImportedHandleTypes =
         .compatibleHandleTypes =
      };
      /* Supports opaque fd and dma_buf. */
   const VkExternalMemoryProperties nvk_dma_buf_mem_props = {
      .externalMemoryFeatures =
      VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT |
      .exportFromImportedHandleTypes =
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT |
      .compatibleHandleTypes =
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT |
   };
      static VkResult
   zero_vram(struct nvk_device *dev, struct nouveau_ws_bo *bo)
   {
      uint32_t push_data[256];
   struct nv_push push;
   nv_push_init(&push, push_data, ARRAY_SIZE(push_data));
                     /* can't go higher for whatever reason */
                     P_MTHD(p, NV902D, SET_DST_FORMAT);
   P_NV902D_SET_DST_FORMAT(p, V_A8B8G8R8);
            P_MTHD(p, NV902D, SET_DST_PITCH);
            P_MTHD(p, NV902D, SET_DST_OFFSET_UPPER);
   P_NV902D_SET_DST_OFFSET_UPPER(p, addr >> 32);
            P_MTHD(p, NV902D, SET_RENDER_SOLID_PRIM_COLOR_FORMAT);
   P_NV902D_SET_RENDER_SOLID_PRIM_COLOR_FORMAT(p, V_A8B8G8R8);
            uint32_t height = bo->size / pitch;
            if (height > 0) {
               P_MTHD(p, NV902D, RENDER_SOLID_PRIM_POINT_SET_X(0));
   P_NV902D_RENDER_SOLID_PRIM_POINT_SET_X(p, 0, 0);
   P_NV902D_RENDER_SOLID_PRIM_POINT_Y(p, 0, 0);
   P_NV902D_RENDER_SOLID_PRIM_POINT_SET_X(p, 1, pitch / 4);
                        P_MTHD(p, NV902D, RENDER_SOLID_PRIM_POINT_SET_X(0));
   P_NV902D_RENDER_SOLID_PRIM_POINT_SET_X(p, 0, 0);
   P_NV902D_RENDER_SOLID_PRIM_POINT_Y(p, 0, height);
   P_NV902D_RENDER_SOLID_PRIM_POINT_SET_X(p, 1, extra / 4);
            return nvk_queue_submit_simple(&dev->queue, nv_push_dw_count(&push),
      }
      static enum nouveau_ws_bo_flags
   nvk_memory_type_flags(const VkMemoryType *type,
         {
      enum nouveau_ws_bo_flags flags = 0;
   if (type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
         else
            if (type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            if (handle_types == 0)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_GetMemoryFdPropertiesKHR(VkDevice device,
                     {
      VK_FROM_HANDLE(nvk_device, dev, device);
   struct nvk_physical_device *pdev = nvk_device_physical(dev);
            switch (handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      bo = nouveau_ws_bo_from_dma_buf(dev->ws_dev, fd);
   if (bo == NULL)
            default:
                  uint32_t type_bits = 0;
   for (unsigned t = 0; t < ARRAY_SIZE(pdev->mem_types); t++) {
      const enum nouveau_ws_bo_flags flags =
         if (!(flags & ~bo->flags))
                                    }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_AllocateMemory(VkDevice device,
                     {
      VK_FROM_HANDLE(nvk_device, dev, device);
   struct nvk_physical_device *pdev = nvk_device_physical(dev);
   struct nvk_device_memory *mem;
            const VkImportMemoryFdInfoKHR *fd_info =
         const VkExportMemoryAllocateInfo *export_info =
         const VkMemoryType *type =
            VkExternalMemoryHandleTypeFlagBits handle_types = 0;
   if (export_info != NULL)
         if (fd_info != NULL)
            const enum nouveau_ws_bo_flags flags =
            uint32_t alignment = (1ULL << 12);
   if (!(flags & NOUVEAU_WS_BO_GART))
            const uint64_t aligned_size =
            mem = vk_device_memory_create(&dev->vk, pAllocateInfo,
         if (!mem)
               mem->map = NULL;
   if (fd_info && fd_info->handleType) {
      assert(fd_info->handleType ==
                        mem->bo = nouveau_ws_bo_from_dma_buf(dev->ws_dev, fd_info->fd);
   if (mem->bo == NULL) {
      result = vk_error(dev, VK_ERROR_INVALID_EXTERNAL_HANDLE);
      }
      } else {
      mem->bo = nouveau_ws_bo_new(dev->ws_dev, aligned_size, alignment, flags);
   if (!mem->bo) {
      result = vk_error(dev, VK_ERROR_OUT_OF_DEVICE_MEMORY);
                  if (dev->ws_dev->debug_flags & NVK_DEBUG_ZERO_MEMORY) {
      if (type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      void *map = nouveau_ws_bo_map(mem->bo, NOUVEAU_WS_BO_RDWR);
   if (map == NULL) {
      result = vk_errorf(dev, VK_ERROR_OUT_OF_HOST_MEMORY,
            }
   memset(map, 0, mem->bo->size);
      } else {
      result = zero_vram(dev, mem->bo);
   if (result != VK_SUCCESS)
                  if (fd_info && fd_info->handleType) {
      /* From the Vulkan spec:
   *
   *    "Importing memory from a file descriptor transfers ownership of
   *    the file descriptor from the application to the Vulkan
   *    implementation. The application must not perform any operations on
   *    the file descriptor after a successful import."
   *
   * If the import fails, we leave the file descriptor open.
   */
                              fail_bo:
         fail_alloc:
      vk_device_memory_destroy(&dev->vk, pAllocator, &mem->vk);
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_FreeMemory(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            if (!mem)
            if (mem->map)
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_MapMemory2KHR(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            if (mem == NULL) {
      *ppData = NULL;
               const VkDeviceSize offset = pMemoryMapInfo->offset;
   const VkDeviceSize size =
      vk_device_memory_range(&mem->vk, pMemoryMapInfo->offset,
         /* From the Vulkan spec version 1.0.32 docs for MapMemory:
   *
   *  * If size is not equal to VK_WHOLE_SIZE, size must be greater than 0
   *    assert(size != 0);
   *  * If size is not equal to VK_WHOLE_SIZE, size must be less than or
   *    equal to the size of the memory minus offset
   */
   assert(size > 0);
            if (size != (size_t)size) {
      return vk_errorf(dev, VK_ERROR_MEMORY_MAP_FAILED,
                     /* From the Vulkan 1.2.194 spec:
   *
   *    "memory must not be currently host mapped"
   */
   if (mem->map != NULL) {
      return vk_errorf(dev, VK_ERROR_MEMORY_MAP_FAILED,
               mem->map = nouveau_ws_bo_map(mem->bo, NOUVEAU_WS_BO_RDWR);
   if (mem->map == NULL) {
      return vk_errorf(dev, VK_ERROR_MEMORY_MAP_FAILED,
                           }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_UnmapMemory2KHR(VkDevice device,
         {
               if (mem == NULL)
            nouveau_ws_bo_unmap(mem->bo, mem->map);
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_FlushMappedMemoryRanges(VkDevice device,
               {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_InvalidateMappedMemoryRanges(VkDevice device,
               {
         }
      VKAPI_ATTR void VKAPI_CALL
   nvk_GetDeviceMemoryCommitment(VkDevice device,
               {
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_GetMemoryFdKHR(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            switch (pGetFdInfo->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      if (nouveau_ws_bo_dma_buf(memory->bo, pFD))
            default:
      assert(!"unsupported handle type");
         }
      VKAPI_ATTR uint64_t VKAPI_CALL
   nvk_GetDeviceMemoryOpaqueCaptureAddress(
      UNUSED VkDevice device,
      {
                  }
