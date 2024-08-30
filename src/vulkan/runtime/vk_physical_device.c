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
      #include "vk_physical_device.h"
      #include "vk_common_entrypoints.h"
   #include "vk_util.h"
      VkResult
   vk_physical_device_init(struct vk_physical_device *pdevice,
                           struct vk_instance *instance,
   {
      memset(pdevice, 0, sizeof(*pdevice));
   vk_object_base_instance_init(instance, &pdevice->base, VK_OBJECT_TYPE_PHYSICAL_DEVICE);
            if (supported_extensions != NULL)
            if (supported_features != NULL)
            if (properties != NULL)
                     /* Add common entrypoints without overwriting driver-provided ones. */
   vk_physical_device_dispatch_table_from_entrypoints(
            /* TODO */
               }
      void
   vk_physical_device_finish(struct vk_physical_device *physical_device)
   {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice,
               {
      if (pProperties == NULL) {
      *pPropertyCount = 0;
               /* None supported at this time */
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                     {
      VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
            for (int i = 0; i < VK_DEVICE_EXTENSION_COUNT; i++) {
      if (!pdevice->supported_extensions.extensions[i])
      #ifdef ANDROID
         if (!vk_android_allowed_device_extensions.extensions[i])
   #endif
            vk_outarray_append_typed(VkExtensionProperties, &out, prop) {
                        }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice,
         {
               /* Don't zero-init this struct since the driver fills it out entirely */
   VkPhysicalDeviceFeatures2 features2;
   features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            pdevice->dispatch_table.GetPhysicalDeviceFeatures2(physicalDevice,
            }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice,
         {
               /* Don't zero-init this struct since the driver fills it out entirely */
   VkPhysicalDeviceProperties2 props2;
   props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            pdevice->dispatch_table.GetPhysicalDeviceProperties2(physicalDevice,
            }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice,
               {
               if (!pQueueFamilyProperties) {
      pdevice->dispatch_table.GetPhysicalDeviceQueueFamilyProperties2(physicalDevice,
                                    for (unsigned i = 0; i < *pQueueFamilyPropertyCount; ++i) {
      props2[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
               pdevice->dispatch_table.GetPhysicalDeviceQueueFamilyProperties2(physicalDevice,
                  for (unsigned i = 0; i < *pQueueFamilyPropertyCount; ++i)
               }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice,
         {
               /* Don't zero-init this struct since the driver fills it out entirely */
   VkPhysicalDeviceMemoryProperties2 props2;
   props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
            pdevice->dispatch_table.GetPhysicalDeviceMemoryProperties2(physicalDevice,
            }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice,
               {
               /* Don't zero-init this struct since the driver fills it out entirely */
   VkFormatProperties2 props2;
   props2.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
            pdevice->dispatch_table.GetPhysicalDeviceFormatProperties2(physicalDevice,
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_GetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice,
                                       {
               VkPhysicalDeviceImageFormatInfo2 info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
   .format = format,
   .type = type,
   .tiling = tiling,
   .usage = usage,
               /* Don't zero-init this struct since the driver fills it out entirely */
   VkImageFormatProperties2 props2;
   props2.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
            VkResult result =
      pdevice->dispatch_table.GetPhysicalDeviceImageFormatProperties2(physicalDevice,
                  }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice,
                                             {
               VkPhysicalDeviceSparseImageFormatInfo2 info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SPARSE_IMAGE_FORMAT_INFO_2,
   .format = format,
   .type = type,
   .samples = samples,
   .usage = usage,
               if (!pProperties) {
      pdevice->dispatch_table.GetPhysicalDeviceSparseImageFormatProperties2(physicalDevice,
                                          for (unsigned i = 0; i < *pNumProperties; ++i) {
      props2[i].sType = VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2;
               pdevice->dispatch_table.GetPhysicalDeviceSparseImageFormatProperties2(physicalDevice,
                        for (unsigned i = 0; i < *pNumProperties; ++i)
               }
      /* VK_EXT_tooling_info */
   VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_GetPhysicalDeviceToolProperties(VkPhysicalDevice physicalDevice,
               {
                  }
