   /*
   * Mesa 3-D graphics library
   *
   * Copyright 2018 Collabora
   *
   * Based on platform_surfaceless, which has:
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
   #ifdef HAVE_LIBDRM
   #include <xf86drm.h>
   #endif
      #include <dlfcn.h>
   #include <fcntl.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <sys/stat.h>
   #include <sys/types.h>
      #include "util/u_debug.h"
   #include "egl_dri2.h"
   #include "kopper_interface.h"
   #include "loader.h"
      static __DRIimage *
   device_alloc_image(struct dri2_egl_display *dri2_dpy,
         {
      return dri2_dpy->image->createImage(
      dri2_dpy->dri_screen_render_gpu, dri2_surf->base.Width,
   }
      static void
   device_free_images(struct dri2_egl_surface *dri2_surf)
   {
      struct dri2_egl_display *dri2_dpy =
            if (dri2_surf->front) {
      dri2_dpy->image->destroyImage(dri2_surf->front);
               free(dri2_surf->swrast_device_buffer);
      }
      static int
   device_image_get_buffers(__DRIdrawable *driDrawable, unsigned int format,
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
   dri2_device_create_surface(_EGLDisplay *disp, EGLint type, _EGLConfig *conf,
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
   device_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
                              dri2_fini_surface(surf);
   free(dri2_surf);
      }
      static _EGLSurface *
   dri2_device_create_pbuffer_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
         }
      static const struct dri2_egl_display_vtbl dri2_device_display_vtbl = {
      .create_pbuffer_surface = dri2_device_create_pbuffer_surface,
   .destroy_surface = device_destroy_surface,
   .create_image = dri2_create_image_khr,
      };
      static void
   device_flush_front_buffer(__DRIdrawable *driDrawable, void *loaderPrivate)
   {
   }
      static unsigned
   device_get_capability(void *loaderPrivate, enum dri_loader_cap cap)
   {
      /* Note: loaderPrivate is _EGLDisplay* */
   switch (cap) {
   case DRI_LOADER_CAP_FP16:
         default:
            }
      static const __DRIimageLoaderExtension image_loader_extension = {
      .base = {__DRI_IMAGE_LOADER, 2},
   .getBuffers = device_image_get_buffers,
   .flushFrontBuffer = device_flush_front_buffer,
      };
      static const __DRIkopperLoaderExtension kopper_loader_extension = {
                  };
      static const __DRIextension *image_loader_extensions[] = {
      &image_loader_extension.base,
   &image_lookup_extension.base,
   &use_invalidate.base,
   &kopper_loader_extension.base,
      };
      static const __DRIextension *swrast_loader_extensions[] = {
      &swrast_pbuffer_loader_extension.base,
   &image_lookup_extension.base,
   &use_invalidate.base,
   &kopper_loader_extension.base,
      };
      static int
   device_get_fd(_EGLDisplay *disp, _EGLDevice *dev)
   {
   #ifdef HAVE_LIBDRM
      int fd = disp->Options.fd;
   bool kms_swrast = disp->Options.ForceSoftware;
   /* The fcntl() code in _eglGetDeviceDisplay() ensures that valid fd >= 3,
   * and invalid one is 0.
   */
   if (fd) {
      /* According to the spec - if the FD does not match the EGLDevice
   * behaviour is undefined.
   *
   * Add a trivial sanity check since it doesn't cost us anything.
   */
   if (dev != _eglFindDevice(fd, false))
            /* kms_swrast only work with primary node. It used to work with render
   * node in the past because some downstream kernel carry a patch to enable
   * dumb bo ioctl on render nodes.
   */
   char *node = kms_swrast ? drmGetPrimaryDeviceNameFromFd(fd)
            /* Don't close the internal fd, get render node one based on it. */
   fd = loader_open_device(node);
   free(node);
      }
   const char *node = _eglQueryDeviceStringEXT(
            #else
      _eglLog(_EGL_FATAL,
            #endif
   }
      static bool
   device_probe_device(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   bool request_software =
            if (request_software)
      _eglLog(_EGL_WARNING, "Not allowed to force software rendering when "
      dri2_dpy->fd_render_gpu = device_get_fd(disp, disp->Device);
   if (dri2_dpy->fd_render_gpu < 0)
                     dri2_dpy->driver_name = loader_get_driver_for_fd(dri2_dpy->fd_render_gpu);
   if (!dri2_dpy->driver_name)
            /* When doing software rendering, some times user still want to explicitly
   * choose the render node device since cross node import doesn't work between
   * vgem/virtio_gpu yet. It would be nice to have a new EXTENSION for this.
   * For now, just fallback to kms_swrast. */
   if (disp->Options.ForceSoftware && !request_software &&
      (strcmp(dri2_dpy->driver_name, "vgem") == 0 ||
   strcmp(dri2_dpy->driver_name, "virtio_gpu") == 0)) {
   free(dri2_dpy->driver_name);
   _eglLog(_EGL_WARNING, "NEEDS EXTENSION: falling back to kms_swrast");
               if (!dri2_load_driver_dri3(disp))
            dri2_dpy->loader_extensions = image_loader_extensions;
         err_load:
      free(dri2_dpy->driver_name);
         err_name:
      close(dri2_dpy->fd_render_gpu);
   dri2_dpy->fd_render_gpu = dri2_dpy->fd_display_gpu = -1;
      }
      static bool
   device_probe_device_sw(_EGLDisplay *disp)
   {
               dri2_dpy->fd_render_gpu = -1;
   dri2_dpy->fd_display_gpu = -1;
   dri2_dpy->driver_name = strdup(disp->Options.Zink ? "zink" : "swrast");
   if (!dri2_dpy->driver_name)
            /* HACK: should be driver_swrast_null */
   if (!dri2_load_driver_swrast(disp)) {
      free(dri2_dpy->driver_name);
   dri2_dpy->driver_name = NULL;
               dri2_dpy->loader_extensions = swrast_loader_extensions;
      }
      EGLBoolean
   dri2_initialize_device(_EGLDisplay *disp)
   {
      _EGLDevice *dev;
   const char *err;
   struct dri2_egl_display *dri2_dpy = dri2_display_create();
   if (!dri2_dpy)
            /* Extension requires a PlatformDisplay - the EGLDevice. */
            disp->Device = dev;
   disp->DriverData = (void *)dri2_dpy;
   err = "DRI2: failed to load driver";
   if (_eglDeviceSupports(dev, _EGL_DEVICE_DRM)) {
      if (!device_probe_device(disp))
      } else if (_eglDeviceSupports(dev, _EGL_DEVICE_SOFTWARE)) {
      if (!device_probe_device_sw(disp))
      } else {
      _eglLog(_EGL_FATAL,
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
