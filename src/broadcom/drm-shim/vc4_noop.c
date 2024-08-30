   /*
   * Copyright (c) 2021 Etnaviv Project
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
   #include <sys/ioctl.h>
   #include "drm-uapi/vc4_drm.h"
   #include "drm-shim/drm_shim.h"
      bool drm_shim_driver_prefers_first_render_node = true;
      static int
   vc4_ioctl_noop(int fd, unsigned long request, void *arg)
   {
         }
      static int
   vc4_ioctl_create_bo(int fd, unsigned long request, void *arg)
   {
         struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_vc4_create_bo *create = arg;
            drm_shim_bo_init(bo, create->size);
   create->handle = drm_shim_bo_get_handle(shim_fd, bo);
            }
      static int
   vc4_ioctl_create_shader_bo(int fd, unsigned long request, void *arg)
   {
         struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_vc4_create_shader_bo *create = arg;
            drm_shim_bo_init(bo, create->size);
   create->handle = drm_shim_bo_get_handle(shim_fd, bo);
            }
      static int
   vc4_ioctl_mmap_bo(int fd, unsigned long request, void *arg)
   {
         struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_vc4_mmap_bo *map = arg;
                              }
      static int
   vc4_ioctl_get_param(int fd, unsigned long request, void *arg)
   {
         struct drm_vc4_get_param *gp = arg;
   static const uint32_t param_map[] = {
                        switch (gp->param) {
   case DRM_VC4_PARAM_SUPPORTS_BRANCHES:
   case DRM_VC4_PARAM_SUPPORTS_ETC1:
   case DRM_VC4_PARAM_SUPPORTS_THREADED_FS:
   case DRM_VC4_PARAM_SUPPORTS_FIXED_RCL_ORDER:
                  case DRM_VC4_PARAM_SUPPORTS_MADVISE:
   case DRM_VC4_PARAM_SUPPORTS_PERFMON:
                  default:
                  if (gp->param < ARRAY_SIZE(param_map) && param_map[gp->param]) {
                        fprintf(stderr, "Unknown DRM_IOCTL_VC4_GET_PARAM %d\n", gp->param);
   }
      static ioctl_fn_t driver_ioctls[] = {
         [DRM_VC4_CREATE_BO] = vc4_ioctl_create_bo,
   [DRM_VC4_CREATE_SHADER_BO] = vc4_ioctl_create_shader_bo,
   [DRM_VC4_MMAP_BO] = vc4_ioctl_mmap_bo,
   [DRM_VC4_GET_PARAM] = vc4_ioctl_get_param,
   [DRM_VC4_GET_TILING] = vc4_ioctl_noop,
   };
      void
   drm_shim_driver_init(void)
   {
         shim_device.bus_type = DRM_BUS_PLATFORM;
   shim_device.driver_name = "vc4";
   shim_device.driver_ioctls = driver_ioctls;
            drm_shim_override_file("OF_FULLNAME=/rdb/vc4\n"
                     }
