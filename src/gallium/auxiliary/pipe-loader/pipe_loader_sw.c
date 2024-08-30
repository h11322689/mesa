   /**************************************************************************
   *
   * Copyright 2012 Francisco Jerez
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #ifdef HAVE_DRISW_KMS
   #include <fcntl.h>
   #endif
      #include "pipe_loader_priv.h"
      #include "util/os_file.h"
   #include "util/u_memory.h"
   #include "util/u_dl.h"
   #include "sw/dri/dri_sw_winsys.h"
   #include "sw/kms-dri/kms_dri_sw_winsys.h"
   #include "sw/null/null_sw_winsys.h"
   #include "sw/wrapper/wrapper_sw_winsys.h"
   #include "target-helpers/sw_helper_public.h"
   #include "target-helpers/inline_debug_helper.h"
   #include "frontend/drisw_api.h"
   #include "frontend/sw_driver.h"
   #include "frontend/sw_winsys.h"
   #include "util/driconf.h"
      struct pipe_loader_sw_device {
      struct pipe_loader_device base;
      #ifndef GALLIUM_STATIC_TARGETS
         #endif
      struct sw_winsys *ws;
      };
      #define pipe_loader_sw_device(dev) ((struct pipe_loader_sw_device *)dev)
      static const struct pipe_loader_ops pipe_loader_sw_ops;
   #if defined(HAVE_DRI) && defined(HAVE_ZINK)
   static const struct pipe_loader_ops pipe_loader_vk_ops;
   #endif
      #ifdef GALLIUM_STATIC_TARGETS
   static const struct sw_driver_descriptor driver_descriptors = {
      .create_screen = sw_screen_create_vk,
      #ifdef HAVE_DRI
         {
      .name = "dri",
      #endif
   #ifdef HAVE_DRISW_KMS
         {
      .name = "kms_dri",
      #endif
   #ifndef __ANDROID__
         {
      .name = "null",
      },
   {
      .name = "wrapped",
      #endif
               };
   #endif
      #if defined(GALLIUM_STATIC_TARGETS) && defined(HAVE_ZINK) && defined(HAVE_DRI)
   static const struct sw_driver_descriptor kopper_driver_descriptors = {
      .create_screen = sw_screen_create_zink,
   .winsys = {
      {
      .name = "dri",
      #ifdef HAVE_DRISW_KMS
         {
      .name = "kms_dri",
      #endif
   #ifndef __ANDROID__
         {
      .name = "null",
      },
   {
      .name = "wrapped",
      #endif
               };
   #endif
      static bool
   pipe_loader_sw_probe_init_common(struct pipe_loader_sw_device *sdev)
   {
      sdev->base.type = PIPE_LOADER_DEVICE_SOFTWARE;
   sdev->base.driver_name = "swrast";
   sdev->base.ops = &pipe_loader_sw_ops;
         #ifdef GALLIUM_STATIC_TARGETS
      sdev->dd = &driver_descriptors;
   if (!sdev->dd)
      #else
      const char *search_dir = getenv("GALLIUM_PIPE_SEARCH_DIR");
   if (search_dir == NULL)
            sdev->lib = pipe_loader_find_module("swrast", search_dir);
   if (!sdev->lib)
            sdev->dd = (const struct sw_driver_descriptor *)
            if (!sdev->dd){
      util_dl_close(sdev->lib);
   sdev->lib = NULL;
         #endif
            }
      #if defined(HAVE_DRI) && defined(HAVE_ZINK)
   static bool
   pipe_loader_vk_probe_init_common(struct pipe_loader_sw_device *sdev)
   {
      sdev->base.type = PIPE_LOADER_DEVICE_PLATFORM;
   sdev->base.driver_name = "kopper";
   sdev->base.ops = &pipe_loader_vk_ops;
         #ifdef GALLIUM_STATIC_TARGETS
      sdev->dd = &kopper_driver_descriptors;
   if (!sdev->dd)
      #else
      const char *search_dir = getenv("GALLIUM_PIPE_SEARCH_DIR");
   if (search_dir == NULL)
            sdev->lib = pipe_loader_find_module("swrast", search_dir);
   if (!sdev->lib)
            sdev->dd = (const struct sw_driver_descriptor *)
            if (!sdev->dd){
      util_dl_close(sdev->lib);
   sdev->lib = NULL;
         #endif
            }
   #endif
      static void
   pipe_loader_sw_probe_teardown_common(struct pipe_loader_sw_device *sdev)
   {
   #ifndef GALLIUM_STATIC_TARGETS
      if (sdev->lib)
      #endif
   }
      #ifdef HAVE_DRI
   bool
   pipe_loader_sw_probe_dri(struct pipe_loader_device **devs, const struct drisw_loader_funcs *drisw_lf)
   {
      struct pipe_loader_sw_device *sdev = CALLOC_STRUCT(pipe_loader_sw_device);
            if (!sdev)
            if (!pipe_loader_sw_probe_init_common(sdev))
            for (i = 0; sdev->dd->winsys[i].name; i++) {
      if (strcmp(sdev->dd->winsys[i].name, "dri") == 0) {
      sdev->ws = sdev->dd->winsys[i].create_winsys_dri(drisw_lf);
         }
   if (!sdev->ws)
            *devs = &sdev->base;
         fail:
      pipe_loader_sw_probe_teardown_common(sdev);
   FREE(sdev);
      }
   #ifdef HAVE_ZINK
   bool
   pipe_loader_vk_probe_dri(struct pipe_loader_device **devs, const struct drisw_loader_funcs *drisw_lf)
   {
      struct pipe_loader_sw_device *sdev = CALLOC_STRUCT(pipe_loader_sw_device);
            if (!sdev)
            if (!pipe_loader_vk_probe_init_common(sdev))
            for (i = 0; sdev->dd->winsys[i].name; i++) {
      if (strcmp(sdev->dd->winsys[i].name, "dri") == 0) {
      sdev->ws = sdev->dd->winsys[i].create_winsys_dri(drisw_lf);
         }
   if (!sdev->ws)
            *devs = &sdev->base;
         fail:
      pipe_loader_sw_probe_teardown_common(sdev);
   FREE(sdev);
      }
   #endif
   #endif
      #ifdef HAVE_DRISW_KMS
   bool
   pipe_loader_sw_probe_kms(struct pipe_loader_device **devs, int fd)
   {
      struct pipe_loader_sw_device *sdev = CALLOC_STRUCT(pipe_loader_sw_device);
            if (!sdev)
            if (!pipe_loader_sw_probe_init_common(sdev))
            if (fd < 0 || (sdev->fd = os_dupfd_cloexec(fd)) < 0)
            for (i = 0; sdev->dd->winsys[i].name; i++) {
      if (strcmp(sdev->dd->winsys[i].name, "kms_dri") == 0) {
      sdev->ws = sdev->dd->winsys[i].create_winsys_kms_dri(sdev->fd);
         }
   if (!sdev->ws)
            *devs = &sdev->base;
         fail:
      pipe_loader_sw_probe_teardown_common(sdev);
   if (sdev->fd != -1)
         FREE(sdev);
      }
   #endif
      bool
   pipe_loader_sw_probe_null(struct pipe_loader_device **devs)
   {
      struct pipe_loader_sw_device *sdev = CALLOC_STRUCT(pipe_loader_sw_device);
            if (!sdev)
            if (!pipe_loader_sw_probe_init_common(sdev))
            for (i = 0; sdev->dd->winsys[i].name; i++) {
      if (strcmp(sdev->dd->winsys[i].name, "null") == 0) {
      sdev->ws = sdev->dd->winsys[i].create_winsys();
         }
   if (!sdev->ws)
            *devs = &sdev->base;
         fail:
      pipe_loader_sw_probe_teardown_common(sdev);
   FREE(sdev);
      }
      int
   pipe_loader_sw_probe(struct pipe_loader_device **devs, int ndev)
   {
               if (i <= ndev) {
      if (!pipe_loader_sw_probe_null(devs)) {
                        }
      bool
   pipe_loader_sw_probe_wrapped(struct pipe_loader_device **dev,
         {
      struct pipe_loader_sw_device *sdev = CALLOC_STRUCT(pipe_loader_sw_device);
            if (!sdev)
            if (!pipe_loader_sw_probe_init_common(sdev))
            for (i = 0; sdev->dd->winsys[i].name; i++) {
      if (strcmp(sdev->dd->winsys[i].name, "wrapped") == 0) {
      sdev->ws = sdev->dd->winsys[i].create_winsys_wrapped(screen);
         }
   if (!sdev->ws)
            *dev = &sdev->base;
         fail:
      pipe_loader_sw_probe_teardown_common(sdev);
   FREE(sdev);
      }
      static void
   pipe_loader_sw_release(struct pipe_loader_device **dev)
   {
      UNUSED struct pipe_loader_sw_device *sdev =
               #ifndef GALLIUM_STATIC_TARGETS
      if (sdev->lib)
      #endif
      #ifdef HAVE_DRISW_KMS
      if (sdev->fd != -1)
      #endif
            }
      static const struct driOptionDescription *
   pipe_loader_sw_get_driconf(struct pipe_loader_device *dev, unsigned *count)
   {
      *count = 0;
      }
      #if defined(HAVE_DRI) && defined(HAVE_ZINK)
   static const driOptionDescription zink_driconf[] = {
         };
      static const struct driOptionDescription *
   pipe_loader_vk_get_driconf(struct pipe_loader_device *dev, unsigned *count)
   {
      *count = ARRAY_SIZE(zink_driconf);
      }
   #endif
      static struct pipe_screen *
   pipe_loader_sw_create_screen(struct pipe_loader_device *dev,
         {
      struct pipe_loader_sw_device *sdev = pipe_loader_sw_device(dev);
                        }
      static const struct pipe_loader_ops pipe_loader_sw_ops = {
      .create_screen = pipe_loader_sw_create_screen,
   .get_driconf = pipe_loader_sw_get_driconf,
      };
      #if defined(HAVE_DRI) && defined(HAVE_ZINK)
   static const struct pipe_loader_ops pipe_loader_vk_ops = {
      .create_screen = pipe_loader_sw_create_screen,
   .get_driconf = pipe_loader_vk_get_driconf,
      };
   #endif
