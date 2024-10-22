   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_device_memory.h"
      #include "venus-protocol/vn_protocol_driver_device_memory.h"
   #include "venus-protocol/vn_protocol_driver_transport.h"
      #include "vn_android.h"
   #include "vn_buffer.h"
   #include "vn_device.h"
   #include "vn_image.h"
   #include "vn_physical_device.h"
      /* device memory commands */
      static inline VkResult
   vn_device_memory_alloc_simple(struct vn_device *dev,
               {
      VkDevice dev_handle = vn_device_to_handle(dev);
   VkDeviceMemory mem_handle = vn_device_memory_to_handle(mem);
   if (VN_PERF(NO_ASYNC_MEM_ALLOC)) {
      return vn_call_vkAllocateMemory(dev->instance, dev_handle, alloc_info,
               struct vn_instance_submit_command instance_submit;
   vn_submit_vkAllocateMemory(dev->instance, 0, dev_handle, alloc_info, NULL,
         if (!instance_submit.ring_seqno_valid)
            mem->bo_ring_seqno_valid = true;
   mem->bo_ring_seqno = instance_submit.ring_seqno;
      }
      static inline void
   vn_device_memory_free_simple(struct vn_device *dev,
         {
      VkDevice dev_handle = vn_device_to_handle(dev);
   VkDeviceMemory mem_handle = vn_device_memory_to_handle(mem);
      }
      static VkResult
   vn_device_memory_wait_alloc(struct vn_device *dev,
         {
      if (!mem->bo_ring_seqno_valid)
            /* fine to false it here since renderer submission failure is fatal */
            uint32_t local_data[8];
   struct vn_cs_encoder local_enc =
         vn_encode_vkWaitRingSeqnoMESA(&local_enc, 0, dev->instance->ring.id,
         return vn_renderer_submit_simple(dev->renderer, local_data,
      }
      static inline VkResult
   vn_device_memory_bo_init(struct vn_device *dev,
               {
      VkResult result = vn_device_memory_wait_alloc(dev, mem);
   if (result != VK_SUCCESS)
            return vn_renderer_bo_create_from_device_memory(
      dev->renderer, mem->size, mem->base.id, mem->type.propertyFlags,
   }
      static inline void
   vn_device_memory_bo_fini(struct vn_device *dev, struct vn_device_memory *mem)
   {
      if (mem->base_bo) {
      vn_device_memory_wait_alloc(dev, mem);
         }
      static VkResult
   vn_device_memory_pool_grow_alloc(struct vn_device *dev,
                     {
      const VkAllocationCallbacks *alloc = &dev->base.base.alloc;
   const VkMemoryAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   .allocationSize = size,
      };
   struct vn_device_memory *mem =
      vk_zalloc(alloc, sizeof(*mem), VN_DEFAULT_ALIGN,
      if (!mem)
            vn_object_base_init(&mem->base, VK_OBJECT_TYPE_DEVICE_MEMORY, &dev->base);
   mem->size = size;
   mem->type =
            VkResult result = vn_device_memory_alloc_simple(dev, mem, &alloc_info);
   if (result != VK_SUCCESS)
            result = vn_device_memory_bo_init(dev, mem, 0);
   if (result != VK_SUCCESS)
            result =
         if (result != VK_SUCCESS)
            mem->bo_roundtrip_seqno_valid = true;
                  bo_unref:
         mem_free:
         obj_fini:
      vn_object_base_fini(&mem->base);
   vk_free(alloc, mem);
      }
      static struct vn_device_memory *
   vn_device_memory_pool_ref(struct vn_device *dev,
         {
                           }
      static void
   vn_device_memory_pool_unref(struct vn_device *dev,
         {
                        if (!vn_renderer_bo_unref(dev->renderer, pool_mem->base_bo))
            /* wait on valid bo_roundtrip_seqno before vkFreeMemory */
   if (pool_mem->bo_roundtrip_seqno_valid)
            vn_device_memory_free_simple(dev, pool_mem);
   vn_object_base_fini(&pool_mem->base);
      }
      void
   vn_device_memory_pool_fini(struct vn_device *dev, uint32_t mem_type_index)
   {
      struct vn_device_memory_pool *pool = &dev->memory_pools[mem_type_index];
   if (pool->memory)
            }
      static VkResult
   vn_device_memory_pool_grow_locked(struct vn_device *dev,
               {
      struct vn_device_memory *mem;
   VkResult result =
         if (result != VK_SUCCESS)
            struct vn_device_memory_pool *pool = &dev->memory_pools[mem_type_index];
   if (pool->memory)
            pool->memory = mem;
               }
      static VkResult
   vn_device_memory_pool_suballocate(struct vn_device *dev,
               {
      static const VkDeviceSize pool_size = 16 * 1024 * 1024;
   /* TODO fix https://gitlab.freedesktop.org/mesa/mesa/-/issues/9351
   * Before that, we use 64K default alignment because some GPUs have 64K
   * pages. It is also required by newer Intel GPUs. Meanwhile, use prior 4K
   * align on implementations known to fit.
   */
   const bool is_renderer_mali = dev->physical_device->renderer_driver_id ==
         const VkDeviceSize pool_align = is_renderer_mali ? 4096 : 64 * 1024;
                              if (!pool->memory || pool->used + mem->size > pool_size) {
      VkResult result =
         if (result != VK_SUCCESS) {
      mtx_unlock(&pool->mutex);
                           /* point mem->base_bo at pool base_bo and assign base_offset accordingly */
   mem->base_bo = pool->memory->base_bo;
   mem->base_offset = pool->used;
                        }
      static bool
   vn_device_memory_should_suballocate(const struct vn_device *dev,
               {
      const struct vn_instance *instance = dev->physical_device->instance;
            if (VN_PERF(NO_MEMORY_SUBALLOC))
            if (renderer->has_guest_vram)
            /* We should not support suballocations because apps can do better.  But
   * each BO takes up a KVM memslot currently and some CTS tests exhausts
   * them.  This might not be needed on newer (host) kernels where there are
   * many more KVM memslots.
            /* consider host-visible memory only */
   if (!(flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
            /* reject larger allocations */
   if (alloc_info->allocationSize > 64 * 1024)
            /* reject if there is any pnext struct other than
   * VkMemoryDedicatedAllocateInfo, or if dedicated allocation is required
   */
   if (alloc_info->pNext) {
      const VkMemoryDedicatedAllocateInfo *dedicated = alloc_info->pNext;
   if (dedicated->sType !=
                        const struct vn_image *img = vn_image_from_handle(dedicated->image);
   if (img) {
      for (uint32_t i = 0; i < ARRAY_SIZE(img->requirements); i++) {
      if (img->requirements[i].dedicated.requiresDedicatedAllocation)
                  const struct vn_buffer *buf = vn_buffer_from_handle(dedicated->buffer);
   if (buf && buf->requirements.dedicated.requiresDedicatedAllocation)
                  }
      VkResult
   vn_device_memory_import_dma_buf(struct vn_device *dev,
                           {
      const VkMemoryType *mem_type =
      &dev->physical_device->memory_properties
         struct vn_renderer_bo *bo;
   VkResult result = vn_renderer_bo_create_from_dma_buf(
      dev->renderer, alloc_info->allocationSize, fd,
      if (result != VK_SUCCESS)
                     const VkImportMemoryResourceInfoMESA import_memory_resource_info = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_RESOURCE_INFO_MESA,
   .pNext = alloc_info->pNext,
      };
   const VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   .pNext = &import_memory_resource_info,
   .allocationSize = alloc_info->allocationSize,
      };
   result = vn_device_memory_alloc_simple(dev, mem, &memory_allocate_info);
   if (result != VK_SUCCESS) {
      vn_renderer_bo_unref(dev->renderer, bo);
               /* need to close import fd on success to avoid fd leak */
   close(fd);
               }
      static VkResult
   vn_device_memory_alloc_guest_vram(
      struct vn_device *dev,
   struct vn_device_memory *mem,
   const VkMemoryAllocateInfo *alloc_info,
      {
      VkResult result = vn_renderer_bo_create_from_device_memory(
      dev->renderer, mem->size, 0, mem->type.propertyFlags, external_handles,
      if (result != VK_SUCCESS) {
                  const VkImportMemoryResourceInfoMESA import_memory_resource_info = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_RESOURCE_INFO_MESA,
   .pNext = alloc_info->pNext,
               const VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   .pNext = &import_memory_resource_info,
   .allocationSize = alloc_info->allocationSize,
                        result = vn_device_memory_alloc_simple(dev, mem, &memory_allocate_info);
   if (result != VK_SUCCESS) {
      vn_renderer_bo_unref(dev->renderer, mem->base_bo);
                  }
      static VkResult
   vn_device_memory_alloc_export(struct vn_device *dev,
                     {
      VkResult result = vn_device_memory_alloc_simple(dev, mem, alloc_info);
   if (result != VK_SUCCESS)
            result = vn_device_memory_bo_init(dev, mem, external_handles);
   if (result != VK_SUCCESS) {
      vn_device_memory_free_simple(dev, mem);
               result =
         if (result != VK_SUCCESS) {
      vn_renderer_bo_unref(dev->renderer, mem->base_bo);
   vn_device_memory_free_simple(dev, mem);
                           }
      struct vn_device_memory_alloc_info {
      VkMemoryAllocateInfo alloc;
   VkExportMemoryAllocateInfo export;
   VkMemoryAllocateFlagsInfo flags;
   VkMemoryDedicatedAllocateInfo dedicated;
      };
      static const VkMemoryAllocateInfo *
   vn_device_memory_fix_alloc_info(
      const VkMemoryAllocateInfo *alloc_info,
   const VkExternalMemoryHandleTypeFlagBits renderer_handle_type,
   bool has_guest_vram,
      {
      local_info->alloc = *alloc_info;
            vk_foreach_struct_const(src, alloc_info->pNext) {
      void *next = NULL;
   switch (src->sType) {
   case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO:
      /* guest vram turns export alloc into import, so drop export info */
   if (has_guest_vram)
         memcpy(&local_info->export, src, sizeof(local_info->export));
   local_info->export.handleTypes = renderer_handle_type;
   next = &local_info->export;
      case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO:
      memcpy(&local_info->flags, src, sizeof(local_info->flags));
   next = &local_info->flags;
      case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO:
      memcpy(&local_info->dedicated, src, sizeof(local_info->dedicated));
   next = &local_info->dedicated;
      case VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO:
      memcpy(&local_info->capture, src, sizeof(local_info->capture));
   next = &local_info->capture;
      default:
                  if (next) {
      cur->pNext = next;
                              }
      static VkResult
   vn_device_memory_alloc(struct vn_device *dev,
                     {
      const struct vn_instance *instance = dev->physical_device->instance;
            const VkExternalMemoryHandleTypeFlagBits renderer_handle_type =
         struct vn_device_memory_alloc_info local_info;
   if (external_handles && external_handles != renderer_handle_type) {
      alloc_info = vn_device_memory_fix_alloc_info(
                  /* ensure correct blob flags */
               if (renderer_info->has_guest_vram) {
      return vn_device_memory_alloc_guest_vram(dev, mem, alloc_info,
      } else if (external_handles) {
      return vn_device_memory_alloc_export(dev, mem, alloc_info,
      } else {
            }
      static void
   vn_device_memory_emit_report(struct vn_device *dev,
                     {
      if (likely(!dev->memory_reports))
            VkDeviceMemoryReportEventTypeEXT type;
   if (result != VK_SUCCESS) {
         } else if (is_alloc) {
      type = mem->is_import ? VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_IMPORT_EXT
      } else {
      type = mem->is_import ? VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_UNIMPORT_EXT
      }
   const uint64_t mem_obj_id =
         vn_device_emit_device_memory_report(dev, type, mem_obj_id, mem->size,
      }
      VkResult
   vn_AllocateMemory(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            /* see vn_physical_device_init_memory_properties */
   VkMemoryAllocateInfo local_info;
   if (pAllocateInfo->memoryTypeIndex ==
      dev->physical_device->incoherent_cached) {
   local_info = *pAllocateInfo;
   local_info.memoryTypeIndex = dev->physical_device->coherent_uncached;
               const VkExportMemoryAllocateInfo *export_info = NULL;
   const VkImportAndroidHardwareBufferInfoANDROID *import_ahb_info = NULL;
   const VkImportMemoryFdInfoKHR *import_fd_info = NULL;
   bool export_ahb = false;
   vk_foreach_struct_const(pnext, pAllocateInfo->pNext) {
      switch (pnext->sType) {
   case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO:
      export_info = (void *)pnext;
   if (export_info->handleTypes &
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID)
      else if (!export_info->handleTypes)
            case VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID:
      import_ahb_info = (void *)pnext;
      case VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR:
      import_fd_info = (void *)pnext;
      default:
                     struct vn_device_memory *mem =
      vk_zalloc(alloc, sizeof(*mem), VN_DEFAULT_ALIGN,
      if (!mem)
            vn_object_base_init(&mem->base, VK_OBJECT_TYPE_DEVICE_MEMORY, &dev->base);
   mem->size = pAllocateInfo->allocationSize;
   mem->type = dev->physical_device->memory_properties
         mem->is_import = import_ahb_info || import_fd_info;
            VkDeviceMemory mem_handle = vn_device_memory_to_handle(mem);
   VkResult result;
   if (import_ahb_info) {
      result = vn_android_device_import_ahb(dev, mem, pAllocateInfo, alloc,
      } else if (export_ahb) {
         } else if (import_fd_info) {
      result = vn_device_memory_import_dma_buf(dev, mem, pAllocateInfo, false,
      } else if (export_info) {
      result = vn_device_memory_alloc(dev, mem, pAllocateInfo,
      } else if (vn_device_memory_should_suballocate(dev, pAllocateInfo,
            result = vn_device_memory_pool_suballocate(
      } else {
                           if (result != VK_SUCCESS) {
      vn_object_base_fini(&mem->base);
   vk_free(alloc, mem);
                           }
      void
   vn_FreeMemory(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_device_memory *mem = vn_device_memory_from_handle(memory);
   const VkAllocationCallbacks *alloc =
            if (!mem)
                     if (mem->base_memory) {
         } else {
      /* ensure renderer side import still sees the resource */
            if (mem->bo_roundtrip_seqno_valid)
                        if (mem->ahb)
            vn_object_base_fini(&mem->base);
      }
      uint64_t
   vn_GetDeviceMemoryOpaqueCaptureAddress(
         {
      struct vn_device *dev = vn_device_from_handle(device);
   ASSERTED struct vn_device_memory *mem =
            assert(!mem->base_memory);
   return vn_call_vkGetDeviceMemoryOpaqueCaptureAddress(dev->instance, device,
      }
      VkResult
   vn_MapMemory(VkDevice device,
               VkDeviceMemory memory,
   VkDeviceSize offset,
   VkDeviceSize size,
   {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_device_memory *mem = vn_device_memory_from_handle(memory);
   const bool need_bo = !mem->base_bo;
   void *ptr = NULL;
                     /* We don't want to blindly create a bo for each HOST_VISIBLE memory as
   * that has a cost. By deferring bo creation until now, we can avoid the
   * cost unless a bo is really needed. However, that means
   * vn_renderer_bo_map will block until the renderer creates the resource
   * and injects the pages into the guest.
   *
   * XXX We also assume that a vn_renderer_bo can be created as long as the
   * renderer VkDeviceMemory has a mappable memory type.  That is plain
   * wrong.  It is impossible to fix though until some new extension is
   * created and supported by the driver, and that the renderer switches to
   * the extension.
   */
   if (need_bo) {
      result = vn_device_memory_bo_init(dev, mem, 0);
   if (result != VK_SUCCESS)
               ptr = vn_renderer_bo_map(dev->renderer, mem->base_bo);
   if (!ptr) {
      /* vn_renderer_bo_map implies a roundtrip on success, but not here. */
   if (need_bo) {
      result = vn_instance_submit_roundtrip(dev->instance,
                                                                     }
      void
   vn_UnmapMemory(VkDevice device, VkDeviceMemory memory)
   {
         }
      VkResult
   vn_FlushMappedMemoryRanges(VkDevice device,
               {
      VN_TRACE_FUNC();
            for (uint32_t i = 0; i < memoryRangeCount; i++) {
      const VkMappedMemoryRange *range = &pMemoryRanges[i];
   struct vn_device_memory *mem =
            const VkDeviceSize size = range->size == VK_WHOLE_SIZE
               vn_renderer_bo_flush(dev->renderer, mem->base_bo,
                  }
      VkResult
   vn_InvalidateMappedMemoryRanges(VkDevice device,
               {
      VN_TRACE_FUNC();
            for (uint32_t i = 0; i < memoryRangeCount; i++) {
      const VkMappedMemoryRange *range = &pMemoryRanges[i];
   struct vn_device_memory *mem =
            const VkDeviceSize size = range->size == VK_WHOLE_SIZE
               vn_renderer_bo_invalidate(dev->renderer, mem->base_bo,
                  }
      void
   vn_GetDeviceMemoryCommitment(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
   ASSERTED struct vn_device_memory *mem =
            assert(!mem->base_memory);
   vn_call_vkGetDeviceMemoryCommitment(dev->instance, device, memory,
      }
      VkResult
   vn_GetMemoryFdKHR(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_device_memory *mem =
            /* At the moment, we support only the below handle types. */
   assert(pGetFdInfo->handleType &
         (VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT |
   assert(!mem->base_memory && mem->base_bo);
   *pFd = vn_renderer_bo_export_dma_buf(dev->renderer, mem->base_bo);
   if (*pFd < 0)
               }
      VkResult
   vn_get_memory_dma_buf_properties(struct vn_device *dev,
                     {
               struct vn_renderer_bo *bo;
   VkResult result = vn_renderer_bo_create_from_dma_buf(
         if (result != VK_SUCCESS)
                     VkMemoryResourceAllocationSizePropertiesMESA alloc_size_props = {
      .sType =
      };
   VkMemoryResourcePropertiesMESA props = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_RESOURCE_PROPERTIES_MESA,
      };
   result = vn_call_vkGetMemoryResourcePropertiesMESA(dev->instance, device,
         vn_renderer_bo_unref(dev->renderer, bo);
   if (result != VK_SUCCESS)
            *out_alloc_size = alloc_size_props.allocationSize;
               }
      VkResult
   vn_GetMemoryFdPropertiesKHR(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   uint64_t alloc_size = 0;
   uint32_t mem_type_bits = 0;
            if (handleType != VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT)
            result =
         if (result != VK_SUCCESS)
                        }
