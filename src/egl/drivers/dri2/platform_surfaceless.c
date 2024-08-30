   /*
   * Mesa 3-D graphics library
   *
   * Copyright (c) 2014 The Chromium OS Authors.
   * Copyright © 2011 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <dlfcn.h>
   #include <fcntl.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <xf86drm.h>
   #include <sys/stat.h>
   #include <sys/types.h>
      #include "egl_dri2.h"
   #include "eglglobals.h"
   #include "kopper_interface.h"
   #include "loader.h"
      static __DRIimage *
   surfaceless_alloc_image(struct dri2_egl_display *dri2_dpy,
         {
      return dri2_dpy->image->createImage(
      dri2_dpy->dri_screen_render_gpu, dri2_surf->base.Width,
   }
      static void
   surfaceless_free_images(struct dri2_egl_surface *dri2_surf)
   {
      struct dri2_egl_display *dri2_dpy =
            if (dri2_surf->front) {
      dri2_dpy->image->destroyImage(dri2_surf->front);
               free(dri2_surf->swrast_device_buffer);
      }
      static int
   surfaceless_image_get_buffers(__DRIdrawable *driDrawable, unsigned int format,
                     {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy =
            buffers->image_mask = 0;
   buffers->front = NULL;
            /* The EGL 1.5 spec states that pbuffers are single-buffered. Specifically,
   * the spec states that they have a back buffer but no front buffer, in
   * contrast to pixmaps, which have a front buffer but no back buffer.
   *
   * Single-buffered surfaces with no front buffer confuse Mesa; so we deviate
   * from the spec, following the precedent of Mesa's EGL X11 platform. The
   * X11 platform correctly assigns pbuffers to single-buffered configs, but
   * assigns the pbuffer a front buffer instead of a back buffer.
   *
   * Pbuffers in the X11 platform mostly work today, so let's just copy its
   * behavior instead of trying to fix (and hence potentially breaking) the
   * world.
                        if (!dri2_surf->front)
            buffers->image_mask |= __DRI_IMAGE_BUFFER_FRONT;
                  }
      static _EGLSurface *
   dri2_surfaceless_create_surface(_EGLDisplay *disp, EGLint type,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct dri2_egl_surface *dri2_surf;
            /* Make sure to calloc so all pointers
   * are originally NULL.
   */
            if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "eglCreatePbufferSurface");
               if (!dri2_init_surface(&dri2_surf->base, disp, type, conf, attrib_list,
                           if (!config) {
      _eglError(EGL_BAD_MATCH,
                     dri2_surf->visual = dri2_image_format_for_pbuffer_config(dri2_dpy, config);
   if (dri2_surf->visual == __DRI_IMAGE_FORMAT_NONE)
            if (!dri2_create_drawable(dri2_dpy, config, dri2_surf, dri2_surf))
                  cleanup_surface:
      free(dri2_surf);
      }
      static EGLBoolean
   surfaceless_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
                              dri2_fini_surface(surf);
   free(dri2_surf);
      }
      static _EGLSurface *
   dri2_surfaceless_create_pbuffer_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      return dri2_surfaceless_create_surface(disp, EGL_PBUFFER_BIT, conf,
      }
      static const struct dri2_egl_display_vtbl dri2_surfaceless_display_vtbl = {
      .create_pbuffer_surface = dri2_surfaceless_create_pbuffer_surface,
   .destroy_surface = surfaceless_destroy_surface,
   .create_image = dri2_create_image_khr,
      };
      static void
   surfaceless_flush_front_buffer(__DRIdrawable *driDrawable, void *loaderPrivate)
   {
   }
      static unsigned
   surfaceless_get_capability(void *loaderPrivate, enum dri_loader_cap cap)
   {
      /* Note: loaderPrivate is _EGLDisplay* */
   switch (cap) {
   case DRI_LOADER_CAP_FP16:
         default:
            }
      static const __DRIkopperLoaderExtension kopper_loader_extension = {
                  };
      static const __DRIimageLoaderExtension image_loader_extension = {
      .base = {__DRI_IMAGE_LOADER, 2},
   .getBuffers = surfaceless_image_get_buffers,
   .flushFrontBuffer = surfaceless_flush_front_buffer,
      };
      static const __DRIextension *image_loader_extensions[] = {
      &image_loader_extension.base,  &image_lookup_extension.base,
   &use_invalidate.base,          &background_callable_extension.base,
      };
      static const __DRIextension *swrast_loader_extensions[] = {
      &swrast_pbuffer_loader_extension.base, &image_loader_extension.base,
   &image_lookup_extension.base,          &use_invalidate.base,
      };
      static bool
   surfaceless_probe_device(_EGLDisplay *disp, bool swrast, bool zink)
   {
      const unsigned node_type = swrast ? DRM_NODE_PRIMARY : DRM_NODE_RENDER;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   _EGLDevice *dev_list = _eglGlobal.DeviceList;
            while (dev_list) {
      if (!_eglDeviceSupports(dev_list, _EGL_DEVICE_DRM))
            device = _eglDeviceDrm(dev_list);
            if (!(device->available_nodes & (1 << node_type)))
            dri2_dpy->fd_render_gpu = loader_open_device(device->nodes[node_type]);
   if (dri2_dpy->fd_render_gpu < 0)
                     char *driver_name = loader_get_driver_for_fd(dri2_dpy->fd_render_gpu);
   if (swrast) {
      /* Use kms swrast only with vgem / virtio_gpu.
   * virtio-gpu fallbacks to software rendering when 3D features
   * are unavailable since 6c5ab, and kms_swrast is more
   * feature complete than swrast.
   */
   if (driver_name && (strcmp(driver_name, "vgem") == 0 ||
                  } else {
      /* Use the given hardware driver */
               if (dri2_dpy->driver_name && dri2_load_driver_dri3(disp)) {
      if (swrast || zink)
         else
                     free(dri2_dpy->driver_name);
   dri2_dpy->driver_name = NULL;
   close(dri2_dpy->fd_render_gpu);
         next:
                  if (!dev_list)
               }
      static bool
   surfaceless_probe_device_sw(_EGLDisplay *disp)
   {
               dri2_dpy->fd_render_gpu = -1;
   disp->Device = _eglFindDevice(dri2_dpy->fd_render_gpu, true);
            dri2_dpy->driver_name = strdup(disp->Options.Zink ? "zink" : "swrast");
   if (!dri2_dpy->driver_name)
            if (!dri2_load_driver_swrast(disp)) {
      free(dri2_dpy->driver_name);
   dri2_dpy->driver_name = NULL;
               dri2_dpy->loader_extensions = swrast_loader_extensions;
      }
      EGLBoolean
   dri2_initialize_surfaceless(_EGLDisplay *disp)
   {
      const char *err;
   bool driver_loaded = false;
   struct dri2_egl_display *dri2_dpy = dri2_display_create();
   if (!dri2_dpy)
                     /* When ForceSoftware is false, we try the HW driver.  When ForceSoftware
   * is true, we try kms_swrast and swrast in order.
   */
   driver_loaded = surfaceless_probe_device(disp, disp->Options.ForceSoftware,
         if (!driver_loaded && disp->Options.ForceSoftware) {
      _eglLog(_EGL_DEBUG, "Falling back to surfaceless swrast without DRM.");
               if (!driver_loaded) {
      err = "DRI2: failed to load driver";
                        if (!dri2_create_screen(disp)) {
      err = "DRI2: failed to create screen";
               if (!dri2_setup_extensions(disp)) {
      err = "DRI2: failed to find required DRI extensions";
                  #ifdef HAVE_WAYLAND_PLATFORM
      dri2_dpy->device_name =
      #endif
               if (!dri2_add_pbuffer_configs_for_visuals(disp)) {
      err = "DRI2: failed to add configs";
               /* Fill vtbl last to prevent accidentally calling virtual function during
   * initialization.
   */
                  cleanup:
      dri2_display_destroy(disp);
      }
