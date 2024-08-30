   /*
   * Copyright © 2021 Intel Corporation
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
      #include "vk_fence.h"
      #include "util/os_time.h"
   #include "util/perf/cpu_trace.h"
      #ifndef _WIN32
   #include <unistd.h>
   #endif
      #include "vk_common_entrypoints.h"
   #include "vk_device.h"
   #include "vk_log.h"
   #include "vk_physical_device.h"
   #include "vk_util.h"
      static VkExternalFenceHandleTypeFlags
   vk_sync_fence_import_types(const struct vk_sync_type *type)
   {
               if (type->import_opaque_fd)
            if (type->import_sync_file)
               }
      static VkExternalFenceHandleTypeFlags
   vk_sync_fence_export_types(const struct vk_sync_type *type)
   {
               if (type->export_opaque_fd)
            if (type->export_sync_file)
               }
      static VkExternalFenceHandleTypeFlags
   vk_sync_fence_handle_types(const struct vk_sync_type *type)
   {
      return vk_sync_fence_export_types(type) &
      }
      static const struct vk_sync_type *
   get_fence_sync_type(struct vk_physical_device *pdevice,
         {
      static const enum vk_sync_features req_features =
      VK_SYNC_FEATURE_BINARY |
   VK_SYNC_FEATURE_CPU_WAIT |
         for (const struct vk_sync_type *const *t =
      pdevice->supported_sync_types; *t; t++) {
   if (req_features & ~(*t)->features)
            if (handle_types & ~vk_sync_fence_handle_types(*t))
                           }
      VkResult
   vk_fence_create(struct vk_device *device,
                     {
                        const VkExportFenceCreateInfo *export =
         VkExternalFenceHandleTypeFlags handle_types =
            const struct vk_sync_type *sync_type =
         if (sync_type == NULL) {
      /* We should always be able to get a fence type for internal */
   assert(get_fence_sync_type(device->physical, 0) != NULL);
   return vk_errorf(device, VK_ERROR_INVALID_EXTERNAL_HANDLE,
                     /* Allocate a vk_fence + vk_sync implementation. Because the permanent
   * field of vk_fence is the base field of the vk_sync implementation, we
   * can make the 2 structures overlap.
   */
   size_t size = offsetof(struct vk_fence, permanent) + sync_type->size;
   fence = vk_object_zalloc(device, pAllocator, size, VK_OBJECT_TYPE_FENCE);
   if (fence == NULL)
            enum vk_sync_flags sync_flags = 0;
   if (handle_types)
            bool signaled = pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT;
   VkResult result = vk_sync_init(device, &fence->permanent,
         if (result != VK_SUCCESS) {
      vk_object_free(device, pAllocator, fence);
                           }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_CreateFence(VkDevice _device,
                     {
      VK_FROM_HANDLE(vk_device, device, _device);
            VkResult result = vk_fence_create(device, pCreateInfo, pAllocator, &fence);
   if (result != VK_SUCCESS)
                        }
      void
   vk_fence_reset_temporary(struct vk_device *device,
         {
      if (fence->temporary == NULL)
            vk_sync_destroy(device, fence->temporary);
      }
      void
   vk_fence_destroy(struct vk_device *device,
               {
      vk_fence_reset_temporary(device, fence);
               }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DestroyFence(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_device, device, _device);
            if (fence == NULL)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_ResetFences(VkDevice _device,
               {
               for (uint32_t i = 0; i < fenceCount; i++) {
               /* From the Vulkan 1.2.194 spec:
   *
   *    "If any member of pFences currently has its payload imported with
   *    temporary permanence, that fence’s prior permanent payload is
   *    first restored. The remaining operations described therefore
   *    operate on the restored payload."
   */
            VkResult result = vk_sync_reset(device, &fence->permanent);
   if (result != VK_SUCCESS)
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_GetFenceStatus(VkDevice _device,
         {
      VK_FROM_HANDLE(vk_device, device, _device);
            if (vk_device_is_lost(device))
            VkResult result = vk_sync_wait(device, vk_fence_get_active_sync(fence),
                     if (result == VK_TIMEOUT)
         else
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_WaitForFences(VkDevice _device,
                           {
                        if (vk_device_is_lost(device))
            if (fenceCount == 0)
                              for (uint32_t i = 0; i < fenceCount; i++) {
      VK_FROM_HANDLE(vk_fence, fence, pFences[i]);
   waits[i] = (struct vk_sync_wait) {
      .sync = vk_fence_get_active_sync(fence),
                  enum vk_sync_wait_flags wait_flags = VK_SYNC_WAIT_COMPLETE;
   if (!waitAll)
            VkResult result = vk_sync_wait_many(device, fenceCount, waits,
                     VkResult device_status = vk_device_check_status(device);
   if (device_status != VK_SUCCESS)
               }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetPhysicalDeviceExternalFenceProperties(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo,
      {
               assert(pExternalFenceInfo->sType ==
         const VkExternalFenceHandleTypeFlagBits handle_type =
            const struct vk_sync_type *sync_type =
         if (sync_type == NULL) {
      pExternalFenceProperties->exportFromImportedHandleTypes = 0;
   pExternalFenceProperties->compatibleHandleTypes = 0;
   pExternalFenceProperties->externalFenceFeatures = 0;
               VkExternalFenceHandleTypeFlagBits import =
         VkExternalFenceHandleTypeFlagBits export =
            if (handle_type != VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT) {
      const struct vk_sync_type *opaque_sync_type =
            /* If we're a different vk_sync_type than the one selected when only
   * OPAQUE_FD is set, then we can't import/export OPAQUE_FD.  Put
   * differently, there can only be one OPAQUE_FD sync type.
   */
   if (sync_type != opaque_sync_type) {
      import &= ~VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT;
                  VkExternalFenceHandleTypeFlags compatible = import & export;
   VkExternalFenceFeatureFlags features = 0;
   if (handle_type & export)
         if (handle_type & import)
            pExternalFenceProperties->exportFromImportedHandleTypes = export;
   pExternalFenceProperties->compatibleHandleTypes = compatible;
      }
      #ifndef _WIN32
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_ImportFenceFdKHR(VkDevice _device,
         {
      VK_FROM_HANDLE(vk_device, device, _device);
            assert(pImportFenceFdInfo->sType ==
            const int fd = pImportFenceFdInfo->fd;
   const VkExternalFenceHandleTypeFlagBits handle_type =
            struct vk_sync *temporary = NULL, *sync;
   if (pImportFenceFdInfo->flags & VK_FENCE_IMPORT_TEMPORARY_BIT) {
      const struct vk_sync_type *sync_type =
            VkResult result = vk_sync_create(device, sync_type, 0 /* flags */,
         if (result != VK_SUCCESS)
               } else {
         }
            VkResult result;
   switch (pImportFenceFdInfo->handleType) {
   case VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT:
      result = vk_sync_import_opaque_fd(device, sync, fd);
         case VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT:
      result = vk_sync_import_sync_file(device, sync, fd);
         default:
                  if (result != VK_SUCCESS) {
      if (temporary != NULL)
                     /* From the Vulkan 1.2.194 spec:
   *
   *    "Importing a fence payload from a file descriptor transfers
   *    ownership of the file descriptor from the application to the
   *    Vulkan implementation. The application must not perform any
   *    operations on the file descriptor after a successful import."
   *
   * If the import fails, we leave the file descriptor open.
   */
   if (fd != -1)
            if (temporary) {
      vk_fence_reset_temporary(device, fence);
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_GetFenceFdKHR(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_device, device, _device);
                              VkResult result;
   switch (pGetFdInfo->handleType) {
   case VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT:
      result = vk_sync_export_opaque_fd(device, sync, pFd);
   if (unlikely(result != VK_SUCCESS))
               case VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT:
      /* There's no direct spec quote for this but the same rules as for
   * semaphore export apply.  We can't export a sync file from a fence
   * if the fence event hasn't been submitted to the kernel yet.
   */
   if (vk_device_supports_threaded_submit(device)) {
      result = vk_sync_wait(device, sync, 0,
               if (unlikely(result != VK_SUCCESS))
               result = vk_sync_export_sync_file(device, sync, pFd);
   if (unlikely(result != VK_SUCCESS))
            /* From the Vulkan 1.2.194 spec:
   *
   *    "Export operations have the same transference as the specified
   *    handle type’s import operations. Additionally, exporting a fence
   *    payload to a handle with copy transference has the same side
   *    effects on the source fence’s payload as executing a fence reset
   *    operation."
   *
   * In other words, exporting a sync file also resets the fence.  We
   * only care about this for the permanent payload because the temporary
   * payload will be destroyed below.
   */
   if (sync == &fence->permanent) {
      result = vk_sync_reset(device, sync);
   if (unlikely(result != VK_SUCCESS))
      }
         default:
                  /* From the Vulkan 1.2.194 spec:
   *
   *    "Export operations have the same transference as the specified
   *    handle type’s import operations. [...]  If the fence was using a
   *    temporarily imported payload, the fence’s prior permanent payload
   *    will be restored.
   */
               }
      #endif /* !defined(_WIN32) */
