   /*
   * Copyright 2022 Alyssa Rosenzweig
   * Copyright 2018 Broadcom
   * SPDX-License-Identifier: MIT
   */
      #include "drm-shim/drm_shim.h"
      bool drm_shim_driver_prefers_first_render_node = true;
      static ioctl_fn_t driver_ioctls[] = {
         };
      void
   drm_shim_driver_init(void)
   {
      shim_device.bus_type = DRM_BUS_PLATFORM;
   shim_device.driver_name = "asahi";
   shim_device.driver_ioctls = driver_ioctls;
            drm_shim_override_file("DRIVER=asahi\n"
                        "OF_FULLNAME=/soc/agx\n"
   }
