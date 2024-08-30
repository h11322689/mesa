   /*
   * Copyright (C) 2021 Icecream95
   * Copyright (C) 2019 Google LLC
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <limits.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include "drm-shim/drm_shim.h"
   #include "drm-uapi/lima_drm.h"
      #include "util/u_math.h"
      bool drm_shim_driver_prefers_first_render_node = true;
      static int
   lima_ioctl_noop(int fd, unsigned long request, void *arg)
   {
         }
      static int
   lima_ioctl_get_param(int fd, unsigned long request, void *arg)
   {
               switch (gp->param) {
   case DRM_LIMA_PARAM_GPU_ID:
      gp->value = DRM_LIMA_PARAM_GPU_ID_MALI450;
      case DRM_LIMA_PARAM_NUM_PP:
      gp->value = 6;
      default:
      fprintf(stderr, "Unknown DRM_IOCTL_LIMA_GET_PARAM %d\n", gp->param);
         }
      static int
   lima_ioctl_gem_create(int fd, unsigned long request, void *arg)
   {
               struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct shim_bo *bo = calloc(1, sizeof(*bo));
                                          }
      static int
   lima_ioctl_gem_info(int fd, unsigned long request, void *arg)
   {
               struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
            gem_info->va = bo->mem_addr;
               }
      static ioctl_fn_t driver_ioctls[] = {
      [DRM_LIMA_GET_PARAM] = lima_ioctl_get_param,
   [DRM_LIMA_GEM_CREATE] = lima_ioctl_gem_create,
   [DRM_LIMA_GEM_INFO] = lima_ioctl_gem_info,
   [DRM_LIMA_GEM_SUBMIT] = lima_ioctl_noop,
   [DRM_LIMA_GEM_WAIT] = lima_ioctl_noop,
   [DRM_LIMA_CTX_CREATE] = lima_ioctl_noop,
      };
      void
   drm_shim_driver_init(void)
   {
      shim_device.bus_type = DRM_BUS_PLATFORM;
   shim_device.driver_name = "lima";
   shim_device.driver_ioctls = driver_ioctls;
            /* lima uses the DRM version to expose features, instead of getparam. */
   shim_device.version_major = 1;
   shim_device.version_minor = 1;
            drm_shim_override_file("DRIVER=lima\n"
                        "OF_FULLNAME=/soc/mali\n"
   }
