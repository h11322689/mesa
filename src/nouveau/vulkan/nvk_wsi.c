   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_wsi.h"
   #include "nvk_instance.h"
   #include "wsi_common.h"
      static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   nvk_wsi_proc_addr(VkPhysicalDevice physicalDevice, const char *pName)
   {
      VK_FROM_HANDLE(nvk_physical_device, pdev, physicalDevice);
      }
      VkResult
   nvk_init_wsi(struct nvk_physical_device *pdev)
   {
               struct wsi_device_options wsi_options = {
         };
   result = wsi_device_init(&pdev->wsi_device,
                     if (result != VK_SUCCESS)
                                 }
      void
   nvk_finish_wsi(struct nvk_physical_device *pdev)
   {
      pdev->vk.wsi_device = NULL;
      }
