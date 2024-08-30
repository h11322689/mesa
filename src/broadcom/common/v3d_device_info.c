   /*
   * Copyright © 2016 Broadcom
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
      #include <errno.h>
   #include <stdio.h>
   #include <string.h>
      #include "common/v3d_device_info.h"
   #include "drm-uapi/v3d_drm.h"
      bool
   v3d_get_device_info(int fd, struct v3d_device_info* devinfo, v3d_ioctl_fun drm_ioctl) {
      struct drm_v3d_get_param ident0 = {
         };
   struct drm_v3d_get_param ident1 = {
         };
   struct drm_v3d_get_param hub_ident3 = {
         };
            ret = drm_ioctl(fd, DRM_IOCTL_V3D_GET_PARAM, &ident0);
   if (ret != 0) {
            fprintf(stderr, "Couldn't get V3D core IDENT0: %s\n",
      }
   ret = drm_ioctl(fd, DRM_IOCTL_V3D_GET_PARAM, &ident1);
   if (ret != 0) {
            fprintf(stderr, "Couldn't get V3D core IDENT1: %s\n",
               uint32_t major = (ident0.value >> 24) & 0xff;
                              int nslc = (ident1.value >> 4) & 0xf;
   int qups = (ident1.value >> 8) & 0xf;
                     switch (devinfo->ver) {
      case 33:
   case 41:
   case 42:
   case 71:
         default:
            fprintf(stderr,
                        ret = drm_ioctl(fd, DRM_IOCTL_V3D_GET_PARAM, &hub_ident3);
   if (ret != 0) {
            fprintf(stderr, "Couldn't get V3D core HUB IDENT3: %s\n",
                           }
