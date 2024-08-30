   /*
   * Mesa 3-D graphics library
   *
   * Copyright © 2021, Google Inc.
   * SPDX-License-Identifier: MIT
   */
      #include <assert.h>
   #include <dlfcn.h>
   #include <errno.h>
   #include <string.h>
   #include <hardware/gralloc.h>
      #include "util/log.h"
   #include "util/u_memory.h"
      #include "u_gralloc_internal.h"
      /* More recent CrOS gralloc has a perform op that fills out the struct below
   * with canonical information about the buffer and its modifier, planes,
   * offsets and strides.  If we have this, we can skip straight to
   * createImageFromDmaBufs2() and avoid all the guessing and recalculations.
   * This also gives us the modifier and plane offsets/strides for multiplanar
   * compressed buffers (eg Intel CCS buffers) in order to make that work in
   * Android.
   */
      struct cros_gralloc {
      struct u_gralloc base;
      };
      static const char cros_gralloc_module_name[] = "CrOS Gralloc";
      #define CROS_GRALLOC_DRM_GET_BUFFER_INFO               4
   #define CROS_GRALLOC_DRM_GET_USAGE                     5
   #define CROS_GRALLOC_DRM_GET_USAGE_FRONT_RENDERING_BIT 0x1
      struct cros_gralloc0_buffer_info {
      uint32_t drm_fourcc;
   int num_fds;
   int fds[4];
   uint64_t modifier;
   int offset[4];
      };
      static int
   cros_get_buffer_info(struct u_gralloc *gralloc,
               {
      struct cros_gralloc0_buffer_info info;
   struct cros_gralloc *gr = (struct cros_gralloc *)gralloc;
            if (gr_mod->perform(gr_mod, CROS_GRALLOC_DRM_GET_BUFFER_INFO, hnd->handle,
            out->drm_fourcc = info.drm_fourcc;
   out->modifier = info.modifier;
   out->num_planes = info.num_fds;
   for (int i = 0; i < out->num_planes; i++) {
      out->fds[i] = info.fds[i];
   out->offsets[i] = info.offset[i];
                              }
      static int
   cros_get_front_rendering_usage(struct u_gralloc *gralloc, uint64_t *out_usage)
   {
      struct cros_gralloc *gr = (struct cros_gralloc *)gralloc;
            if (gr->gralloc_module->perform(
         gr->gralloc_module, CROS_GRALLOC_DRM_GET_USAGE,
   CROS_GRALLOC_DRM_GET_USAGE_FRONT_RENDERING_BIT,
      *out_usage = front_rendering_usage;
                  }
      static int
   destroy(struct u_gralloc *gralloc)
   {
      struct cros_gralloc *gr = (struct cros_gralloc *)gralloc;
   if (gr->gralloc_module)
                        }
      struct u_gralloc *
   u_gralloc_cros_api_create()
   {
      struct cros_gralloc *gr = CALLOC_STRUCT(cros_gralloc);
            err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
            if (err)
            if (strcmp(gr->gralloc_module->common.name, cros_gralloc_module_name) != 0)
            if (!gr->gralloc_module->perform) {
      mesa_logw("Oops. CrOS gralloc doesn't have perform callback");
               gr->base.ops.get_buffer_basic_info = cros_get_buffer_info;
   gr->base.ops.get_front_rendering_usage = cros_get_front_rendering_usage;
                           fail:
                  }
