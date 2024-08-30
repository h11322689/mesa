   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_buffer_view.h"
      #include "nil_format.h"
   #include "nil_image.h"
   #include "nvk_buffer.h"
   #include "nvk_entrypoints.h"
   #include "nvk_device.h"
   #include "nvk_format.h"
   #include "nvk_physical_device.h"
      #include "vk_format.h"
      VkFormatFeatureFlags2
   nvk_get_buffer_format_features(struct nvk_physical_device *pdev,
         {
               if (nvk_get_va_format(pdev, vk_format))
            enum pipe_format p_format = vk_format_to_pipe_format(vk_format);
   if (nil_format_supports_buffer(&pdev->info, p_format)) {
               if (nil_format_supports_storage(&pdev->info, p_format)) {
      features |= VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_BIT |
               if (p_format == PIPE_FORMAT_R32_UINT || p_format == PIPE_FORMAT_R32_SINT)
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateBufferView(VkDevice _device,
                     {
      VK_FROM_HANDLE(nvk_device, device, _device);
   VK_FROM_HANDLE(nvk_buffer, buffer, pCreateInfo->buffer);
   struct nvk_buffer_view *view;
            view = vk_buffer_view_create(&device->vk, pCreateInfo,
         if (!view)
            uint32_t desc[8];
   nil_buffer_fill_tic(&nvk_device_physical(device)->info,
                        result = nvk_descriptor_table_add(device, &device->images,
         if (result != VK_SUCCESS) {
      vk_buffer_view_destroy(&device->vk, pAllocator, &view->vk);
                           }
      VKAPI_ATTR void VKAPI_CALL
   nvk_DestroyBufferView(VkDevice _device,
               {
      VK_FROM_HANDLE(nvk_device, device, _device);
            if (!view)
                        }
