   /*
   * Copyright © 2019 Google LLC
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
   #include "drm-uapi/msm_drm.h"
   #include <sys/ioctl.h>
      #include "util/u_math.h"
      bool drm_shim_driver_prefers_first_render_node = true;
      struct msm_device_info {
      uint32_t gpu_id;
   uint32_t chip_id;
      };
      static const struct msm_device_info *device_info;
      static int
   msm_ioctl_noop(int fd, unsigned long request, void *arg)
   {
         }
      static int
   msm_ioctl_gem_new(int fd, unsigned long request, void *arg)
   {
      struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_msm_gem_new *create = arg;
            if (!size)
            struct shim_bo *bo = calloc(1, sizeof(*bo));
            ret = drm_shim_bo_init(bo, size);
   if (ret) {
      free(bo);
                                    }
      static int
   msm_ioctl_gem_info(int fd, unsigned long request, void *arg)
   {
      struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_msm_gem_info *args = arg;
            if (!bo)
            switch (args->info) {
   case MSM_INFO_GET_OFFSET:
      args->value = drm_shim_bo_get_mmap_offset(shim_fd, bo);
      case MSM_INFO_GET_IOVA:
      args->value = bo->mem_addr;
      case MSM_INFO_SET_IOVA:
   case MSM_INFO_SET_NAME:
         default:
      fprintf(stderr, "Unknown DRM_IOCTL_MSM_GEM_INFO %d\n", args->info);
   drm_shim_bo_put(bo);
                           }
      static int
   msm_ioctl_get_param(int fd, unsigned long request, void *arg)
   {
               switch (gp->param) {
   case MSM_PARAM_GPU_ID:
      gp->value = device_info->gpu_id;
      case MSM_PARAM_GMEM_SIZE:
      gp->value = device_info->gmem_size;
      case MSM_PARAM_GMEM_BASE:
      gp->value = 0x100000;
      case MSM_PARAM_CHIP_ID:
      gp->value = device_info->chip_id;
      case MSM_PARAM_NR_RINGS:
      gp->value = 1;
      case MSM_PARAM_MAX_FREQ:
      gp->value = 1000000;
      case MSM_PARAM_TIMESTAMP:
      gp->value = 0;
      case MSM_PARAM_PP_PGTABLE:
      gp->value = 1;
      case MSM_PARAM_FAULTS:
   case MSM_PARAM_SUSPENDS:
      gp->value = 0;
      case MSM_PARAM_VA_START:
   case MSM_PARAM_VA_SIZE:
      gp->value = 0x100000000ULL;
      default:
      fprintf(stderr, "Unknown DRM_IOCTL_MSM_GET_PARAM %d\n", gp->param);
         }
      static int
   msm_ioctl_gem_madvise(int fd, unsigned long request, void *arg)
   {
                           }
      static ioctl_fn_t driver_ioctls[] = {
      [DRM_MSM_GET_PARAM] = msm_ioctl_get_param,
   [DRM_MSM_SET_PARAM] = msm_ioctl_noop,
   [DRM_MSM_GEM_NEW] = msm_ioctl_gem_new,
   [DRM_MSM_GEM_INFO] = msm_ioctl_gem_info,
   [DRM_MSM_GEM_CPU_PREP] = msm_ioctl_noop,
   [DRM_MSM_GEM_CPU_FINI] = msm_ioctl_noop,
   [DRM_MSM_GEM_SUBMIT] = msm_ioctl_noop,
   [DRM_MSM_WAIT_FENCE] = msm_ioctl_noop,
   [DRM_MSM_GEM_MADVISE] = msm_ioctl_gem_madvise,
   [DRM_MSM_SUBMITQUEUE_NEW] = msm_ioctl_noop,
   [DRM_MSM_SUBMITQUEUE_CLOSE] = msm_ioctl_noop,
      };
      #define CHIPID(maj, min, rev, pat)                                             \
            static const struct msm_device_info device_infos[] = {
      {
      /* First entry is default */
   .gpu_id = 630,
   .chip_id = CHIPID(6, 3, 0, 0xff),
      },
   {
      .gpu_id = 200,
   .chip_id = CHIPID(2, 0, 0, 0),
      },
   {
      .gpu_id = 201,
   .chip_id = CHIPID(2, 0, 0, 1),
      },
   {
      .gpu_id = 220,
   .chip_id = CHIPID(2, 2, 0, 0xff),
      },
   {
      .gpu_id = 305,
   .chip_id = CHIPID(3, 0, 5, 0xff),
      },
   {
      .gpu_id = 307,
   .chip_id = CHIPID(3, 0, 6, 0),
      },
   {
      .gpu_id = 320,
   .chip_id = CHIPID(3, 2, 0xff, 0xff),
      },
   {
      .gpu_id = 330,
   .chip_id = CHIPID(3, 3, 0, 0xff),
      },
   {
      .gpu_id = 420,
   .chip_id = CHIPID(4, 2, 0, 0xff),
      },
   {
      .gpu_id = 430,
   .chip_id = CHIPID(4, 3, 0, 0xff),
      },
   {
      .gpu_id = 510,
   .chip_id = CHIPID(5, 1, 0, 0xff),
      },
   {
      .gpu_id = 530,
   .chip_id = CHIPID(5, 3, 0, 2),
      },
   {
      .gpu_id = 540,
   .chip_id = CHIPID(5, 4, 0, 2),
      },
   {
      .gpu_id = 618,
   .chip_id = CHIPID(6, 1, 8, 0xff),
      },
   {
      .gpu_id = 630,
   .chip_id = CHIPID(6, 3, 0, 0xff),
      },
   {
      .gpu_id = 660,
   .chip_id = CHIPID(6, 6, 0, 0xff),
         };
      static void
   msm_driver_get_device_info(void)
   {
               if (!env) {
      device_info = &device_infos[0];
               int gpu_id = atoi(env);
   for (int i = 0; i < ARRAY_SIZE(device_infos); i++) {
      if (device_infos[i].gpu_id == gpu_id) {
      device_info = &device_infos[i];
                  fprintf(stderr, "FD_GPU_ID unrecognized, shim supports %d",
         for (int i = 1; i < ARRAY_SIZE(device_infos); i++)
         fprintf(stderr, "\n");
      }
      void
   drm_shim_driver_init(void)
   {
      shim_device.bus_type = DRM_BUS_PLATFORM;
   shim_device.driver_name = "msm";
   shim_device.driver_ioctls = driver_ioctls;
            /* msm uses the DRM version to expose features, instead of getparam. */
   shim_device.version_major = 1;
   shim_device.version_minor = 9;
                     drm_shim_override_file("OF_FULLNAME=/rdb/msm\n"
                        }
