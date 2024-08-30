   /*
   * Copyright © 2011 Intel Corporation
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Kristian Høgsberg <krh@bitplanet.net>
   */
      #include <dlfcn.h>
   #include <fcntl.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <xf86drm.h>
   #include <sys/stat.h>
   #include <sys/types.h>
      #include "util/os_file.h"
      #include "egl_dri2.h"
   #include "egldevice.h"
   #include "loader.h"
      static struct gbm_bo *
   lock_front_buffer(struct gbm_surface *_surf)
   {
      struct gbm_dri_surface *surf = gbm_dri_surface(_surf);
   struct dri2_egl_surface *dri2_surf = surf->dri_private;
   struct gbm_dri_device *device = gbm_dri_device(_surf->gbm);
            if (dri2_surf->current == NULL) {
      _eglError(EGL_BAD_SURFACE, "no front buffer");
                        if (!device->swrast) {
      dri2_surf->current->locked = true;
                  }
      static void
   release_buffer(struct gbm_surface *_surf, struct gbm_bo *bo)
   {
      struct gbm_dri_surface *surf = gbm_dri_surface(_surf);
            for (unsigned i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (dri2_surf->color_buffers[i].bo == bo) {
      dri2_surf->color_buffers[i].locked = false;
            }
      static int
   has_free_buffers(struct gbm_surface *_surf)
   {
      struct gbm_dri_surface *surf = gbm_dri_surface(_surf);
            for (unsigned i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++)
      if (!dri2_surf->color_buffers[i].locked)
            }
      static bool
   dri2_drm_config_is_compatible(struct dri2_egl_display *dri2_dpy,
               {
      const struct gbm_dri_visual *visual = NULL;
   int shifts[4];
   unsigned int sizes[4];
   bool is_float;
            /* Check that the EGLConfig being used to render to the surface is
   * compatible with the surface format. Since mixing ARGB and XRGB of
   * otherwise-compatible formats is relatively common, explicitly allow
   * this.
   */
                     for (i = 0; i < dri2_dpy->gbm_dri->num_visuals; i++) {
      visual = &dri2_dpy->gbm_dri->visual_table[i];
   if (visual->gbm_format == surface->v0.format)
               if (i == dri2_dpy->gbm_dri->num_visuals)
            if (shifts[0] != visual->rgba_shifts.red ||
      shifts[1] != visual->rgba_shifts.green ||
   shifts[2] != visual->rgba_shifts.blue ||
   (shifts[3] > -1 && visual->rgba_shifts.alpha > -1 &&
   shifts[3] != visual->rgba_shifts.alpha) ||
   sizes[0] != visual->rgba_sizes.red ||
   sizes[1] != visual->rgba_sizes.green ||
   sizes[2] != visual->rgba_sizes.blue ||
   (sizes[3] > 0 && visual->rgba_sizes.alpha > 0 &&
   sizes[3] != visual->rgba_sizes.alpha) ||
   is_float != visual->is_float) {
                  }
      static _EGLSurface *
   dri2_drm_create_window_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct dri2_egl_surface *dri2_surf;
   struct gbm_surface *surface = native_surface;
   struct gbm_dri_surface *surf;
            dri2_surf = calloc(1, sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
               if (!dri2_init_surface(&dri2_surf->base, disp, EGL_WINDOW_BIT, conf,
                  config = dri2_get_dri_config(dri2_conf, EGL_WINDOW_BIT,
            if (!config) {
      _eglError(EGL_BAD_MATCH,
                     if (!dri2_drm_config_is_compatible(dri2_dpy, config, surface)) {
      _eglError(EGL_BAD_MATCH, "EGL config not compatible with GBM format");
               surf = gbm_dri_surface(surface);
   dri2_surf->gbm_surf = surf;
   dri2_surf->base.Width = surf->base.v0.width;
   dri2_surf->base.Height = surf->base.v0.height;
            if (!dri2_create_drawable(dri2_dpy, config, dri2_surf, dri2_surf->gbm_surf))
                  cleanup_surf:
                  }
      static _EGLSurface *
   dri2_drm_create_pixmap_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      /* From the EGL_MESA_platform_gbm spec, version 5:
   *
   *  It is not valid to call eglCreatePlatformPixmapSurfaceEXT with a <dpy>
   *  that belongs to the GBM platform. Any such call fails and generates
   *  EGL_BAD_PARAMETER.
   */
   _eglError(EGL_BAD_PARAMETER, "cannot create EGL pixmap surfaces on GBM");
      }
      static EGLBoolean
   dri2_drm_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
                     for (unsigned i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (dri2_surf->color_buffers[i].bo)
                        dri2_fini_surface(surf);
               }
      static int
   get_back_bo(struct dri2_egl_surface *dri2_surf)
   {
      struct dri2_egl_display *dri2_dpy =
         struct gbm_dri_surface *surf = dri2_surf->gbm_surf;
            if (dri2_surf->back == NULL) {
      for (unsigned i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (!dri2_surf->color_buffers[i].locked &&
      dri2_surf->color_buffers[i].age >= age) {
   dri2_surf->back = &dri2_surf->color_buffers[i];
                     if (dri2_surf->back == NULL)
         if (dri2_surf->back->bo == NULL) {
      if (surf->base.v0.modifiers)
      dri2_surf->back->bo = gbm_bo_create_with_modifiers(
      &dri2_dpy->gbm_dri->base, surf->base.v0.width, surf->base.v0.height,
   else {
      unsigned flags = surf->base.v0.flags;
   if (dri2_surf->base.ProtectedContent)
         dri2_surf->back->bo =
      gbm_bo_create(&dri2_dpy->gbm_dri->base, surf->base.v0.width,
      }
   if (dri2_surf->back->bo == NULL)
               }
      static int
   get_swrast_front_bo(struct dri2_egl_surface *dri2_surf)
   {
      struct dri2_egl_display *dri2_dpy =
                  if (dri2_surf->current == NULL) {
      assert(!dri2_surf->color_buffers[0].locked);
               if (dri2_surf->current->bo == NULL)
      dri2_surf->current->bo = gbm_bo_create(
      &dri2_dpy->gbm_dri->base, surf->base.v0.width, surf->base.v0.height,
   if (dri2_surf->current->bo == NULL)
               }
      static int
   dri2_drm_image_get_buffers(__DRIdrawable *driDrawable, unsigned int format,
               {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
            if (get_back_bo(dri2_surf) < 0)
            bo = gbm_dri_bo(dri2_surf->back->bo);
   buffers->image_mask = __DRI_IMAGE_BUFFER_BACK;
               }
      static void
   dri2_drm_flush_front_buffer(__DRIdrawable *driDrawable, void *loaderPrivate)
   {
      (void)driDrawable;
      }
      static EGLBoolean
   dri2_drm_swap_buffers(_EGLDisplay *disp, _EGLSurface *draw)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (!dri2_dpy->flush) {
      dri2_dpy->core->swapBuffers(dri2_surf->dri_drawable);
               if (dri2_surf->current)
         for (unsigned i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++)
      if (dri2_surf->color_buffers[i].age > 0)
         /* Flushing must be done before get_back_bo to make sure glthread's
   * unmarshalling thread is idle otherwise it might concurrently
   * call get_back_bo (eg: through dri2_drm_image_get_buffers).
   */
   dri2_flush_drawable_for_swapbuffers(disp, draw);
            /* Make sure we have a back buffer in case we're swapping without
   * ever rendering. */
   if (get_back_bo(dri2_surf) < 0)
            dri2_surf->current = dri2_surf->back;
   dri2_surf->current->age = 1;
               }
      static EGLint
   dri2_drm_query_buffer_age(_EGLDisplay *disp, _EGLSurface *surface)
   {
               if (get_back_bo(dri2_surf) < 0) {
      _eglError(EGL_BAD_ALLOC, "dri2_query_buffer_age");
                  }
      static _EGLImage *
   dri2_drm_create_image_khr_pixmap(_EGLDisplay *disp, _EGLContext *ctx,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct gbm_dri_bo *dri_bo = gbm_dri_bo((struct gbm_bo *)buffer);
            dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr_pixmap");
                        dri2_img->dri_image = dri2_dpy->image->dupImage(dri_bo->image, dri2_img);
   if (dri2_img->dri_image == NULL) {
      free(dri2_img);
   _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr_pixmap");
                  }
      static _EGLImage *
   dri2_drm_create_image_khr(_EGLDisplay *disp, _EGLContext *ctx, EGLenum target,
         {
      switch (target) {
   case EGL_NATIVE_PIXMAP_KHR:
         default:
            }
      static int
   dri2_drm_authenticate(_EGLDisplay *disp, uint32_t id)
   {
                  }
      static void
   swrast_put_image2(__DRIdrawable *driDrawable, int op, int x, int y, int width,
         {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int internal_stride;
   struct gbm_dri_bo *bo;
   uint32_t bpp;
   int x_bytes, width_bytes;
            if (op != __DRI_SWRAST_IMAGE_OP_DRAW && op != __DRI_SWRAST_IMAGE_OP_SWAP)
            if (get_swrast_front_bo(dri2_surf) < 0)
                     bpp = gbm_bo_get_bpp(&bo->base);
   if (bpp == 0)
            x_bytes = x * (bpp >> 3);
            if (gbm_dri_bo_map_dumb(bo) == NULL)
                     dst = bo->map + x_bytes + (y * internal_stride);
            for (int i = 0; i < height; i++) {
      memcpy(dst, src, width_bytes);
   dst += internal_stride;
                  }
      static void
   swrast_get_image(__DRIdrawable *driDrawable, int x, int y, int width,
         {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int internal_stride, stride;
   struct gbm_dri_bo *bo;
   uint32_t bpp;
   int x_bytes, width_bytes;
            if (get_swrast_front_bo(dri2_surf) < 0)
                     bpp = gbm_bo_get_bpp(&bo->base);
   if (bpp == 0)
            x_bytes = x * (bpp >> 3);
            internal_stride = bo->base.v0.stride;
            if (gbm_dri_bo_map_dumb(bo) == NULL)
            dst = data;
            for (int i = 0; i < height; i++) {
      memcpy(dst, src, width_bytes);
   dst += stride;
                  }
      static EGLBoolean
   drm_add_configs_for_visuals(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   const struct gbm_dri_visual *visuals = dri2_dpy->gbm_dri->visual_table;
   int num_visuals = dri2_dpy->gbm_dri->num_visuals;
   unsigned int format_count[num_visuals];
                     for (unsigned i = 0; dri2_dpy->driver_configs[i]; i++) {
      const __DRIconfig *config = dri2_dpy->driver_configs[i];
   int shifts[4];
   unsigned int sizes[4];
                              for (unsigned j = 0; j < num_visuals; j++) {
               if (visuals[j].rgba_shifts.red != shifts[0] ||
      visuals[j].rgba_shifts.green != shifts[1] ||
   visuals[j].rgba_shifts.blue != shifts[2] ||
   visuals[j].rgba_shifts.alpha != shifts[3] ||
   visuals[j].rgba_sizes.red != sizes[0] ||
   visuals[j].rgba_sizes.green != sizes[1] ||
   visuals[j].rgba_sizes.blue != sizes[2] ||
   visuals[j].rgba_sizes.alpha != sizes[3] ||
               const EGLint attr_list[] = {
      EGL_NATIVE_VISUAL_ID,
   visuals[j].gbm_format,
               dri2_conf =
      dri2_add_config(disp, dri2_dpy->driver_configs[i], config_count + 1,
      if (dri2_conf) {
      if (dri2_conf->base.ConfigID == config_count + 1)
                           for (unsigned i = 0; i < ARRAY_SIZE(format_count); i++) {
      if (!format_count[i]) {
      struct gbm_format_name_desc desc;
   _eglLog(_EGL_DEBUG, "No DRI config supports native format %s",
                     }
      static const struct dri2_egl_display_vtbl dri2_drm_display_vtbl = {
      .authenticate = dri2_drm_authenticate,
   .create_window_surface = dri2_drm_create_window_surface,
   .create_pixmap_surface = dri2_drm_create_pixmap_surface,
   .destroy_surface = dri2_drm_destroy_surface,
   .create_image = dri2_drm_create_image_khr,
   .swap_buffers = dri2_drm_swap_buffers,
   .query_buffer_age = dri2_drm_query_buffer_age,
      };
      static int
   get_fd_render_gpu_drm(struct gbm_dri_device *gbm_dri, int fd_display_gpu)
   {
      /* This doesn't make sense for the software case. */
            /* Render-capable device, so just return the same fd. */
   if (loader_is_device_render_capable(fd_display_gpu))
            /* Display-only device, so return a compatible render-only device. */
      }
      EGLBoolean
   dri2_initialize_drm(_EGLDisplay *disp)
   {
      struct gbm_device *gbm;
   const char *err;
   struct dri2_egl_display *dri2_dpy = dri2_display_create();
   if (!dri2_dpy)
                     gbm = disp->PlatformDisplay;
   if (gbm == NULL) {
      if (disp->Device) {
               if (!_eglDeviceSupports(disp->Device, _EGL_DEVICE_DRM)) {
      err = "DRI2: Device isn't of _EGL_DEVICE_DRM type";
               if (!(drm->available_nodes & (1 << DRM_NODE_PRIMARY))) {
      err = "DRI2: Device does not have DRM_NODE_PRIMARY node";
               dri2_dpy->fd_display_gpu =
      } else {
      char buf[64];
   int n = snprintf(buf, sizeof(buf), DRM_DEV_NAME, DRM_DIR_NAME, 0);
   if (n != -1 && n < sizeof(buf))
               gbm = gbm_create_device(dri2_dpy->fd_display_gpu);
   if (gbm == NULL) {
      err = "DRI2: failed to create gbm device";
      }
      } else {
      dri2_dpy->fd_display_gpu = os_dupfd_cloexec(gbm_device_get_fd(gbm));
   if (dri2_dpy->fd_display_gpu < 0) {
      err = "DRI2: failed to fcntl() existing gbm device";
         }
   dri2_dpy->gbm_dri = gbm_dri_device(gbm);
   if (!dri2_dpy->gbm_dri->software) {
      dri2_dpy->fd_render_gpu =
         if (dri2_dpy->fd_render_gpu < 0) {
      err = "DRI2: failed to get compatible render device";
                  if (strcmp(gbm_device_get_backend_name(gbm), "drm") != 0) {
      err = "DRI2: gbm device using incorrect/incompatible backend";
                        if (!dri2_load_driver_dri3(disp)) {
      err = "DRI3: failed to load driver";
               dri2_dpy->dri_screen_render_gpu = dri2_dpy->gbm_dri->screen;
   dri2_dpy->core = dri2_dpy->gbm_dri->core;
   dri2_dpy->image_driver = dri2_dpy->gbm_dri->image_driver;
   dri2_dpy->swrast = dri2_dpy->gbm_dri->swrast;
   dri2_dpy->kopper = dri2_dpy->gbm_dri->kopper;
            dri2_dpy->gbm_dri->lookup_image = dri2_lookup_egl_image;
   dri2_dpy->gbm_dri->validate_image = dri2_validate_egl_image;
   dri2_dpy->gbm_dri->lookup_image_validated = dri2_lookup_egl_image_validated;
            dri2_dpy->gbm_dri->flush_front_buffer = dri2_drm_flush_front_buffer;
   dri2_dpy->gbm_dri->image_get_buffers = dri2_drm_image_get_buffers;
   dri2_dpy->gbm_dri->swrast_put_image2 = swrast_put_image2;
            dri2_dpy->gbm_dri->base.v0.surface_lock_front_buffer = lock_front_buffer;
   dri2_dpy->gbm_dri->base.v0.surface_release_buffer = release_buffer;
            if (!dri2_setup_extensions(disp)) {
      err = "DRI2: failed to find required DRI extensions";
               if (!dri2_setup_device(disp, dri2_dpy->gbm_dri->software)) {
      err = "DRI2: failed to setup EGLDevice";
                        if (!drm_add_configs_for_visuals(disp)) {
      err = "DRI2: failed to add configs";
               disp->Extensions.KHR_image_pixmap = EGL_TRUE;
   if (dri2_dpy->image_driver)
         #ifdef HAVE_WAYLAND_PLATFORM
      dri2_dpy->device_name =
      #endif
               /* Fill vtbl last to prevent accidentally calling virtual function during
   * initialization.
   */
                  cleanup:
      dri2_display_destroy(disp);
      }
      void
   dri2_teardown_drm(struct dri2_egl_display *dri2_dpy)
   {
      if (dri2_dpy->own_device)
      }
