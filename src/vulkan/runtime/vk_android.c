   /*
   * Copyright © 2022 Intel Corporation
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
      #include "vk_android.h"
      #include "vk_common_entrypoints.h"
   #include "vk_device.h"
   #include "vk_image.h"
   #include "vk_log.h"
   #include "vk_queue.h"
   #include "vk_util.h"
      #include "util/libsync.h"
      #include <hardware/gralloc.h>
      #if ANDROID_API_LEVEL >= 26
   #include <hardware/gralloc1.h>
   #endif
      #include <unistd.h>
      #if ANDROID_API_LEVEL >= 26
   #include <vndk/hardware_buffer.h>
      /* From the Android hardware_buffer.h header:
   *
   *    "The buffer will be written to by the GPU as a framebuffer attachment.
   *
   *    Note that the name of this flag is somewhat misleading: it does not
   *    imply that the buffer contains a color format. A buffer with depth or
   *    stencil format that will be used as a framebuffer attachment should
   *    also have this flag. Use the equivalent flag
   *    AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER to avoid this confusion."
   *
   * The flag was renamed from COLOR_OUTPUT to FRAMEBUFFER at Android API
   * version 29.
   */
   #if ANDROID_API_LEVEL < 29
   #define AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT
   #endif
      /* Convert an AHB format to a VkFormat, based on the "AHardwareBuffer Format
   * Equivalence" table in Vulkan spec.
   *
   * Note that this only covers a subset of AHB formats defined in NDK.  Drivers
   * can support more AHB formats, including private ones.
   */
   VkFormat
   vk_ahb_format_to_image_format(uint32_t ahb_format)
   {
      switch (ahb_format) {
   case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
   case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
         case AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM:
         case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM:
         case AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT:
         case AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM:
         case AHARDWAREBUFFER_FORMAT_D16_UNORM:
         case AHARDWAREBUFFER_FORMAT_D24_UNORM:
         case AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT:
         case AHARDWAREBUFFER_FORMAT_D32_FLOAT:
         case AHARDWAREBUFFER_FORMAT_D32_FLOAT_S8_UINT:
         case AHARDWAREBUFFER_FORMAT_S8_UINT:
         default:
            }
      /* Convert a VkFormat to an AHB format, based on the "AHardwareBuffer Format
   * Equivalence" table in Vulkan spec.
   *
   * Note that this only covers a subset of AHB formats defined in NDK.  Drivers
   * can support more AHB formats, including private ones.
   */
   uint32_t
   vk_image_format_to_ahb_format(VkFormat vk_format)
   {
      switch (vk_format) {
   case VK_FORMAT_R8G8B8A8_UNORM:
         case VK_FORMAT_R8G8B8_UNORM:
         case VK_FORMAT_R5G6B5_UNORM_PACK16:
         case VK_FORMAT_R16G16B16A16_SFLOAT:
         case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
         case VK_FORMAT_D16_UNORM:
         case VK_FORMAT_X8_D24_UNORM_PACK32:
         case VK_FORMAT_D24_UNORM_S8_UINT:
         case VK_FORMAT_D32_SFLOAT:
         case VK_FORMAT_D32_SFLOAT_S8_UINT:
         case VK_FORMAT_S8_UINT:
         default:
            }
      /* Construct ahw usage mask from image usage bits, see
   * 'AHardwareBuffer Usage Equivalence' in Vulkan spec.
   */
   uint64_t
   vk_image_usage_to_ahb_usage(const VkImageCreateFlags vk_create,
         {
      uint64_t ahb_usage = 0;
   if (vk_usage & (VK_IMAGE_USAGE_SAMPLED_BIT |
                  if (vk_usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                  if (vk_usage & VK_IMAGE_USAGE_STORAGE_BIT)
            if (vk_create & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
            if (vk_create & VK_IMAGE_CREATE_PROTECTED_BIT)
            /* No usage bits set - set at least one GPU usage. */
   if (ahb_usage == 0)
               }
      struct AHardwareBuffer *
   vk_alloc_ahardware_buffer(const VkMemoryAllocateInfo *pAllocateInfo)
   {
      const VkMemoryDedicatedAllocateInfo *dedicated_info =
      vk_find_struct_const(pAllocateInfo->pNext,
         uint32_t w = 0;
   uint32_t h = 1;
   uint32_t layers = 1;
   uint32_t format = 0;
            /* If caller passed dedicated information. */
   if (dedicated_info && dedicated_info->image) {
               if (!image->ahb_format)
            w = image->extent.width;
   h = image->extent.height;
   layers = image->array_layers;
   format = image->ahb_format;
   usage = vk_image_usage_to_ahb_usage(image->create_flags,
      } else {
      /* AHB export allocation for VkBuffer requires a valid allocationSize */
   assert(pAllocateInfo->allocationSize);
   w = pAllocateInfo->allocationSize;
   format = AHARDWAREBUFFER_FORMAT_BLOB;
   usage = AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER |
                     struct AHardwareBuffer_Desc desc = {
      .width = w,
   .height = h,
   .layers = layers,
   .format = format,
               struct AHardwareBuffer *ahb;
   if (AHardwareBuffer_allocate(&desc, &ahb) != 0)
               }
   #endif /* ANDROID_API_LEVEL >= 26 */
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_AcquireImageANDROID(VkDevice _device,
                           {
      VK_FROM_HANDLE(vk_device, device, _device);
            /* From https://source.android.com/devices/graphics/implement-vulkan :
   *
   *    "The driver takes ownership of the fence file descriptor and closes
   *    the fence file descriptor when no longer needed. The driver must do
   *    so even if neither a semaphore or fence object is provided, or even
   *    if vkAcquireImageANDROID fails and returns an error."
   *
   * The Vulkan spec for VkImportFence/SemaphoreFdKHR(), however, requires
   * the file descriptor to be left alone on failure.
   */
   int semaphore_fd = -1, fence_fd = -1;
   if (nativeFenceFd >= 0) {
      if (semaphore != VK_NULL_HANDLE && fence != VK_NULL_HANDLE) {
      /* We have both so we have to import the sync file twice. One of
   * them needs to be a dup.
   */
   semaphore_fd = nativeFenceFd;
   fence_fd = dup(nativeFenceFd);
   if (fence_fd < 0) {
      VkResult err = (errno == EMFILE) ? VK_ERROR_TOO_MANY_OBJECTS :
         close(nativeFenceFd);
         } else if (semaphore != VK_NULL_HANDLE) {
         } else if (fence != VK_NULL_HANDLE) {
         } else {
      /* Nothing to import into so we have to close the file */
                  if (semaphore != VK_NULL_HANDLE) {
      const VkImportSemaphoreFdInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR,
   .semaphore = semaphore,
   .flags = VK_SEMAPHORE_IMPORT_TEMPORARY_BIT,
   .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
      };
   result = device->dispatch_table.ImportSemaphoreFdKHR(_device, &info);
   if (result == VK_SUCCESS)
               if (result == VK_SUCCESS && fence != VK_NULL_HANDLE) {
      const VkImportFenceFdInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_FENCE_FD_INFO_KHR,
   .fence = fence,
   .flags = VK_FENCE_IMPORT_TEMPORARY_BIT,
   .handleType = VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT,
      };
   result = device->dispatch_table.ImportFenceFdKHR(_device, &info);
   if (result == VK_SUCCESS)
               if (semaphore_fd >= 0)
         if (fence_fd >= 0)
               }
      static VkResult
   vk_anb_semaphore_init_once(struct vk_queue *queue, struct vk_device *device)
   {
      if (queue->anb_semaphore != VK_NULL_HANDLE)
            const VkExportSemaphoreCreateInfo export_info = {
      .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
      };
   const VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      };
   return device->dispatch_table.CreateSemaphore(vk_device_to_handle(device),
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_QueueSignalReleaseImageANDROID(VkQueue _queue,
                           {
      VK_FROM_HANDLE(vk_queue, queue, _queue);
   struct vk_device *device = queue->base.device;
            STACK_ARRAY(VkPipelineStageFlags, stage_flags, MAX2(1, waitSemaphoreCount));
   for (uint32_t i = 0; i < MAX2(1, waitSemaphoreCount); i++)
            result = vk_anb_semaphore_init_once(queue, device);
   if (result != VK_SUCCESS) {
      STACK_ARRAY_FINISH(stage_flags);
               const VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
   .waitSemaphoreCount = waitSemaphoreCount,
   .pWaitSemaphores = pWaitSemaphores,
   .pWaitDstStageMask = stage_flags,
   .signalSemaphoreCount = 1,
      };
   result = device->dispatch_table.QueueSubmit(_queue, 1, &submit_info,
         STACK_ARRAY_FINISH(stage_flags);
   if (result != VK_SUCCESS)
            const VkSemaphoreGetFdInfoKHR get_fd = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
   .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
      };
   return device->dispatch_table.GetSemaphoreFdKHR(vk_device_to_handle(device),
      }
