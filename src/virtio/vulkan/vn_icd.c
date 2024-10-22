   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_icd.h"
      #include "vn_instance.h"
      /* we support all versions from version 1 up to version 5 */
   static uint32_t vn_icd_version = 5;
      VkResult
   vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t *pSupportedVersion)
   {
      vn_env_init();
            vn_icd_version = MIN2(vn_icd_version, *pSupportedVersion);
   if (VN_DEBUG(INIT))
            *pSupportedVersion = vn_icd_version;
      }
      PFN_vkVoidFunction
   vk_icdGetInstanceProcAddr(VkInstance instance, const char *pName)
   {
         }
      PFN_vkVoidFunction
   vk_icdGetPhysicalDeviceProcAddr(VkInstance _instance, const char *pName)
   {
      struct vn_instance *instance = vn_instance_from_handle(_instance);
   return vk_instance_get_physical_device_proc_addr(&instance->base.base,
      }
      bool
   vn_icd_supports_api_version(uint32_t api_version)
   {
         }
