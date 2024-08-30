   /*
   * Copyright © 2011-2012 Intel Corporation
   * Copyright © 2012 Collabora, Ltd.
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
   *    Benjamin Franzke <benjaminfranzke@googlemail.com>
   */
      #include <dlfcn.h>
   #include <errno.h>
   #include <fcntl.h>
   #include <limits.h>
   #include <stdint.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <xf86drm.h>
   #include "drm-uapi/drm_fourcc.h"
   #include <sys/mman.h>
   #include <vulkan/vulkan_core.h>
   #include <vulkan/vulkan_wayland.h>
      #include "util/anon_file.h"
   #include "util/u_vector.h"
   #include "egl_dri2.h"
   #include "eglglobals.h"
   #include "kopper_interface.h"
   #include "loader.h"
   #include "loader_dri_helper.h"
      #include "linux-dmabuf-unstable-v1-client-protocol.h"
   #include "wayland-drm-client-protocol.h"
   #include <wayland-client.h>
   #include <wayland-egl-backend.h>
      /*
   * The index of entries in this table is used as a bitmask in
   * dri2_dpy->formats.formats_bitmap, which tracks the formats supported
   * by our server.
   */
   static const struct dri2_wl_visual {
      const char *format_name;
   uint32_t wl_drm_format;
   uint32_t wl_shm_format;
   int dri_image_format;
   /* alt_dri_image_format is a substitute wl_buffer format to use for a
   * wl-server unsupported dri_image_format, ie. some other dri_image_format in
   * the table, of the same precision but with different channel ordering, or
   * __DRI_IMAGE_FORMAT_NONE if an alternate format is not needed or supported.
   * The code checks if alt_dri_image_format can be used as a fallback for a
   * dri_image_format for a given wl-server implementation.
   */
   int alt_dri_image_format;
   int bpp;
   int rgba_shifts[4];
      } dri2_wl_visuals[] = {
      {
      "ABGR16F",
   WL_DRM_FORMAT_ABGR16F,
   WL_SHM_FORMAT_ABGR16161616F,
   __DRI_IMAGE_FORMAT_ABGR16161616F,
   0,
   64,
   {0, 16, 32, 48},
      },
   {
      "XBGR16F",
   WL_DRM_FORMAT_XBGR16F,
   WL_SHM_FORMAT_XBGR16161616F,
   __DRI_IMAGE_FORMAT_XBGR16161616F,
   0,
   64,
   {0, 16, 32, -1},
      },
   {
      "XRGB2101010",
   WL_DRM_FORMAT_XRGB2101010,
   WL_SHM_FORMAT_XRGB2101010,
   __DRI_IMAGE_FORMAT_XRGB2101010,
   __DRI_IMAGE_FORMAT_XBGR2101010,
   32,
   {20, 10, 0, -1},
      },
   {
      "ARGB2101010",
   WL_DRM_FORMAT_ARGB2101010,
   WL_SHM_FORMAT_ARGB2101010,
   __DRI_IMAGE_FORMAT_ARGB2101010,
   __DRI_IMAGE_FORMAT_ABGR2101010,
   32,
   {20, 10, 0, 30},
      },
   {
      "XBGR2101010",
   WL_DRM_FORMAT_XBGR2101010,
   WL_SHM_FORMAT_XBGR2101010,
   __DRI_IMAGE_FORMAT_XBGR2101010,
   __DRI_IMAGE_FORMAT_XRGB2101010,
   32,
   {0, 10, 20, -1},
      },
   {
      "ABGR2101010",
   WL_DRM_FORMAT_ABGR2101010,
   WL_SHM_FORMAT_ABGR2101010,
   __DRI_IMAGE_FORMAT_ABGR2101010,
   __DRI_IMAGE_FORMAT_ARGB2101010,
   32,
   {0, 10, 20, 30},
      },
   {
      "XRGB8888",
   WL_DRM_FORMAT_XRGB8888,
   WL_SHM_FORMAT_XRGB8888,
   __DRI_IMAGE_FORMAT_XRGB8888,
   __DRI_IMAGE_FORMAT_NONE,
   32,
   {16, 8, 0, -1},
      },
   {
      "ARGB8888",
   WL_DRM_FORMAT_ARGB8888,
   WL_SHM_FORMAT_ARGB8888,
   __DRI_IMAGE_FORMAT_ARGB8888,
   __DRI_IMAGE_FORMAT_NONE,
   32,
   {16, 8, 0, 24},
      },
   {
      "ABGR8888",
   WL_DRM_FORMAT_ABGR8888,
   WL_SHM_FORMAT_ABGR8888,
   __DRI_IMAGE_FORMAT_ABGR8888,
   __DRI_IMAGE_FORMAT_NONE,
   32,
   {0, 8, 16, 24},
      },
   {
      "XBGR8888",
   WL_DRM_FORMAT_XBGR8888,
   WL_SHM_FORMAT_XBGR8888,
   __DRI_IMAGE_FORMAT_XBGR8888,
   __DRI_IMAGE_FORMAT_NONE,
   32,
   {0, 8, 16, -1},
      },
   {
      "RGB565",
   WL_DRM_FORMAT_RGB565,
   WL_SHM_FORMAT_RGB565,
   __DRI_IMAGE_FORMAT_RGB565,
   __DRI_IMAGE_FORMAT_NONE,
   16,
   {11, 5, 0, -1},
      },
   {
      "ARGB1555",
   WL_DRM_FORMAT_ARGB1555,
   WL_SHM_FORMAT_ARGB1555,
   __DRI_IMAGE_FORMAT_ARGB1555,
   __DRI_IMAGE_FORMAT_ABGR1555,
   16,
   {10, 5, 0, 15},
      },
   {
      "XRGB1555",
   WL_DRM_FORMAT_XRGB1555,
   WL_SHM_FORMAT_XRGB1555,
   __DRI_IMAGE_FORMAT_XRGB1555,
   __DRI_IMAGE_FORMAT_XBGR1555,
   16,
   {10, 5, 0, -1},
      },
   {
      "ARGB4444",
   WL_DRM_FORMAT_ARGB4444,
   WL_SHM_FORMAT_ARGB4444,
   __DRI_IMAGE_FORMAT_ARGB4444,
   __DRI_IMAGE_FORMAT_XBGR4444,
   16,
   {8, 4, 0, 12},
      },
   {
      "XRGB4444",
   WL_DRM_FORMAT_XRGB4444,
   WL_SHM_FORMAT_XRGB4444,
   __DRI_IMAGE_FORMAT_XRGB4444,
   __DRI_IMAGE_FORMAT_XBGR4444,
   16,
   {8, 4, 0, -1},
         };
      static int
   dri2_wl_visual_idx_from_config(struct dri2_egl_display *dri2_dpy,
         {
      int shifts[4];
                     for (unsigned int i = 0; i < ARRAY_SIZE(dri2_wl_visuals); i++) {
               int cmp_rgb_shifts =
         int cmp_rgb_sizes =
            if (cmp_rgb_shifts == 0 && cmp_rgb_sizes == 0 &&
      wl_visual->rgba_shifts[3] == (force_opaque ? -1 : shifts[3]) &&
   wl_visual->rgba_sizes[3] == (force_opaque ? 0 : sizes[3])) {
                     }
      static int
   dri2_wl_visual_idx_from_fourcc(uint32_t fourcc)
   {
      for (int i = 0; i < ARRAY_SIZE(dri2_wl_visuals); i++) {
      /* wl_drm format codes overlap with DRIImage FourCC codes for all formats
   * we support. */
   if (dri2_wl_visuals[i].wl_drm_format == fourcc)
                  }
      static int
   dri2_wl_visual_idx_from_dri_image_format(uint32_t dri_image_format)
   {
      for (int i = 0; i < ARRAY_SIZE(dri2_wl_visuals); i++) {
      if (dri2_wl_visuals[i].dri_image_format == dri_image_format)
                  }
      static int
   dri2_wl_visual_idx_from_shm_format(uint32_t shm_format)
   {
      for (int i = 0; i < ARRAY_SIZE(dri2_wl_visuals); i++) {
      if (dri2_wl_visuals[i].wl_shm_format == shm_format)
                  }
      bool
   dri2_wl_is_format_supported(void *user_data, uint32_t format)
   {
      _EGLDisplay *disp = (_EGLDisplay *)user_data;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (j == -1)
            for (int i = 0; dri2_dpy->driver_configs[i]; i++)
      if (j == dri2_wl_visual_idx_from_config(
                  }
      static int
   roundtrip(struct dri2_egl_display *dri2_dpy)
   {
         }
      static void
   wl_buffer_release(void *data, struct wl_buffer *buffer)
   {
      struct dri2_egl_surface *dri2_surf = data;
            for (i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); ++i)
      if (dri2_surf->color_buffers[i].wl_buffer == buffer)
                  if (dri2_surf->color_buffers[i].wl_release) {
      wl_buffer_destroy(buffer);
   dri2_surf->color_buffers[i].wl_release = false;
   dri2_surf->color_buffers[i].wl_buffer = NULL;
                  }
      static const struct wl_buffer_listener wl_buffer_listener = {
         };
      static void
   dri2_wl_formats_fini(struct dri2_wl_formats *formats)
   {
               for (i = 0; i < formats->num_formats; i++)
            free(formats->modifiers);
      }
      static int
   dri2_wl_formats_init(struct dri2_wl_formats *formats)
   {
               /* formats->formats_bitmap tells us if a format in dri2_wl_visuals is present
   * or not. So we must compute the amount of unsigned int's needed to
   * represent all the formats of dri2_wl_visuals. We use BITSET_WORDS for
   * this task. */
   formats->num_formats = ARRAY_SIZE(dri2_wl_visuals);
   formats->formats_bitmap = calloc(BITSET_WORDS(formats->num_formats),
         if (!formats->formats_bitmap)
            /* Here we have an array of u_vector's to store the modifiers supported by
   * each format in the bitmask. */
   formats->modifiers =
         if (!formats->modifiers)
            for (i = 0; i < formats->num_formats; i++)
      if (!u_vector_init_pow2(&formats->modifiers[i], 4, sizeof(uint64_t))) {
      j = i;
                  err_vector_init:
      for (i = 0; i < j; i++)
            err_modifier:
         err:
      _eglError(EGL_BAD_ALLOC, "dri2_wl_formats_init");
      }
      static void
   dmabuf_feedback_format_table_fini(
         {
      if (format_table->data && format_table->data != MAP_FAILED)
      }
      static void
   dmabuf_feedback_format_table_init(
         {
         }
      static void
   dmabuf_feedback_tranche_fini(struct dmabuf_feedback_tranche *tranche)
   {
         }
      static int
   dmabuf_feedback_tranche_init(struct dmabuf_feedback_tranche *tranche)
   {
               if (dri2_wl_formats_init(&tranche->formats) < 0)
               }
      static void
   dmabuf_feedback_fini(struct dmabuf_feedback *dmabuf_feedback)
   {
               util_dynarray_foreach (&dmabuf_feedback->tranches,
                           }
      static int
   dmabuf_feedback_init(struct dmabuf_feedback *dmabuf_feedback)
   {
               if (dmabuf_feedback_tranche_init(&dmabuf_feedback->pending_tranche) < 0)
                                 }
      static void
   resize_callback(struct wl_egl_window *wl_win, void *data)
   {
      struct dri2_egl_surface *dri2_surf = data;
   struct dri2_egl_display *dri2_dpy =
            if (dri2_surf->base.Width == wl_win->width &&
      dri2_surf->base.Height == wl_win->height)
                  /* Update the surface size as soon as native window is resized; from user
   * pov, this makes the effect that resize is done immediately after native
   * window resize, without requiring to wait until the first draw.
   *
   * A more detailed and lengthy explanation can be found at
   * https://lists.freedesktop.org/archives/mesa-dev/2018-June/196474.html
   */
   if (!dri2_surf->back) {
      dri2_surf->base.Width = wl_win->width;
      }
      }
      static void
   destroy_window_callback(void *data)
   {
      struct dri2_egl_surface *dri2_surf = data;
      }
      static struct wl_surface *
   get_wl_surface_proxy(struct wl_egl_window *window)
   {
      /* Version 3 of wl_egl_window introduced a version field at the same
   * location where a pointer to wl_surface was stored. Thus, if
   * window->version is dereferenceable, we've been given an older version of
   * wl_egl_window, and window->version points to wl_surface */
   if (_eglPointerIsDereferenceable((void *)(window->version))) {
         }
      }
      static void
   surface_dmabuf_feedback_format_table(
      void *data,
   struct zwp_linux_dmabuf_feedback_v1 *zwp_linux_dmabuf_feedback_v1,
      {
      struct dri2_egl_surface *dri2_surf = data;
            feedback->format_table.size = size;
   feedback->format_table.data =
               }
      static void
   surface_dmabuf_feedback_main_device(
      void *data, struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
      {
      struct dri2_egl_surface *dri2_surf = data;
                     /* Compositors may support switching render devices and change the main
   * device of the dma-buf feedback. In this case, when we reallocate the
   * buffers of the surface we must ensure that it is not allocated in memory
   * that is only visible to the GPU that EGL is using, as the compositor will
   * have to import them to the render device it is using.
   *
   * TODO: we still don't know how to allocate such buffers.
   */
   if (dri2_surf->dmabuf_feedback.main_device != 0 &&
      (feedback->main_device != dri2_surf->dmabuf_feedback.main_device))
      else
      }
      static void
   surface_dmabuf_feedback_tranche_target_device(
      void *data, struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
      {
      struct dri2_egl_surface *dri2_surf = data;
            memcpy(&feedback->pending_tranche.target_device, device->data,
      }
      static void
   surface_dmabuf_feedback_tranche_flags(
      void *data, struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
      {
      struct dri2_egl_surface *dri2_surf = data;
               }
      static void
   surface_dmabuf_feedback_tranche_formats(
      void *data, struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
      {
      struct dri2_egl_surface *dri2_surf = data;
   struct dmabuf_feedback *feedback = &dri2_surf->pending_dmabuf_feedback;
   uint64_t *modifier_ptr, modifier;
   uint32_t format;
   uint16_t *index;
            /* Compositor may advertise or not a format table. If it does, we use it.
   * Otherwise, we steal the most recent advertised format table. If we don't
   * have a most recent advertised format table, compositor did something
   * wrong. */
   if (feedback->format_table.data == NULL) {
      feedback->format_table = dri2_surf->dmabuf_feedback.format_table;
   dmabuf_feedback_format_table_init(
      }
   if (feedback->format_table.data == MAP_FAILED) {
      _eglLog(_EGL_WARNING, "wayland-egl: we could not map the format table "
                  }
   if (feedback->format_table.data == NULL) {
      _eglLog(_EGL_WARNING,
         "wayland-egl: compositor didn't advertise a format "
   "table, so we won't be able to use this batch of dma-buf "
               wl_array_for_each (index, indices) {
      format = feedback->format_table.data[*index].format;
            /* Skip formats that are not the one the surface is already using. We
   * can't switch to another format. */
   if (format != dri2_surf->format)
            /* We are sure that the format is supported because of the check above. */
   visual_idx = dri2_wl_visual_idx_from_fourcc(format);
            BITSET_SET(feedback->pending_tranche.formats.formats_bitmap, visual_idx);
   modifier_ptr =
         if (modifier_ptr)
         }
      static void
   surface_dmabuf_feedback_tranche_done(
         {
      struct dri2_egl_surface *dri2_surf = data;
            /* Add tranche to array of tranches. */
   util_dynarray_append(&feedback->tranches, struct dmabuf_feedback_tranche,
               }
      static void
   surface_dmabuf_feedback_done(
         {
               /* The dma-buf feedback protocol states that surface dma-buf feedback should
   * be sent by the compositor only if its buffers are using a suboptimal pair
   * of format and modifier. We can't change the buffer format, but we can
   * reallocate with another modifier. So we raise this flag in order to force
   * buffer reallocation based on the dma-buf feedback sent. */
            dmabuf_feedback_fini(&dri2_surf->dmabuf_feedback);
   dri2_surf->dmabuf_feedback = dri2_surf->pending_dmabuf_feedback;
      }
      static const struct zwp_linux_dmabuf_feedback_v1_listener
      surface_dmabuf_feedback_listener = {
      .format_table = surface_dmabuf_feedback_format_table,
   .main_device = surface_dmabuf_feedback_main_device,
   .tranche_target_device = surface_dmabuf_feedback_tranche_target_device,
   .tranche_flags = surface_dmabuf_feedback_tranche_flags,
   .tranche_formats = surface_dmabuf_feedback_tranche_formats,
   .tranche_done = surface_dmabuf_feedback_tranche_done,
   };
      /**
   * Called via eglCreateWindowSurface(), drv->CreateWindowSurface().
   */
   static _EGLSurface *
   dri2_wl_create_window_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct wl_egl_window *window = native_window;
   struct dri2_egl_surface *dri2_surf;
   struct zwp_linux_dmabuf_v1 *dmabuf_wrapper;
   int visual_idx;
            if (!window) {
      _eglError(EGL_BAD_NATIVE_WINDOW, "dri2_create_surface");
               if (window->driver_private) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
               dri2_surf = calloc(1, sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
               if (!dri2_init_surface(&dri2_surf->base, disp, EGL_WINDOW_BIT, conf,
                  config = dri2_get_dri_config(dri2_conf, EGL_WINDOW_BIT,
            if (!config) {
      _eglError(EGL_BAD_MATCH,
                     dri2_surf->base.Width = window->width;
         #ifndef NDEBUG
      /* Enforce that every visual has an opaque variant (requirement to support
   * EGL_EXT_present_opaque)
   */
   for (unsigned int i = 0; i < ARRAY_SIZE(dri2_wl_visuals); i++) {
      const struct dri2_wl_visual *transparent_visual = &dri2_wl_visuals[i];
   if (transparent_visual->rgba_sizes[3] == 0) {
                  bool found_opaque_equivalent = false;
   for (unsigned int j = 0; j < ARRAY_SIZE(dri2_wl_visuals); j++) {
      const struct dri2_wl_visual *opaque_visual = &dri2_wl_visuals[j];
   if (opaque_visual->rgba_sizes[3] != 0) {
                  int cmp_rgb_shifts =
      memcmp(transparent_visual->rgba_shifts, opaque_visual->rgba_shifts,
      int cmp_rgb_sizes =
                  if (cmp_rgb_shifts == 0 && cmp_rgb_sizes == 0) {
      found_opaque_equivalent = true;
                        #endif
         visual_idx = dri2_wl_visual_idx_from_config(dri2_dpy, config,
                  if (dri2_dpy->wl_dmabuf || dri2_dpy->wl_drm) {
         } else {
      assert(dri2_dpy->wl_shm);
               dri2_surf->wl_queue = wl_display_create_queue(dri2_dpy->wl_dpy);
   if (!dri2_surf->wl_queue) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
               if (dri2_dpy->wl_drm) {
      dri2_surf->wl_drm_wrapper = wl_proxy_create_wrapper(dri2_dpy->wl_drm);
   if (!dri2_surf->wl_drm_wrapper) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      }
   wl_proxy_set_queue((struct wl_proxy *)dri2_surf->wl_drm_wrapper,
               dri2_surf->wl_dpy_wrapper = wl_proxy_create_wrapper(dri2_dpy->wl_dpy);
   if (!dri2_surf->wl_dpy_wrapper) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      }
   wl_proxy_set_queue((struct wl_proxy *)dri2_surf->wl_dpy_wrapper,
            dri2_surf->wl_surface_wrapper = get_wl_surface_proxy(window);
   if (!dri2_surf->wl_surface_wrapper) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      }
   wl_proxy_set_queue((struct wl_proxy *)dri2_surf->wl_surface_wrapper,
            if (dri2_dpy->wl_dmabuf &&
      zwp_linux_dmabuf_v1_get_version(dri2_dpy->wl_dmabuf) >=
         dmabuf_wrapper = wl_proxy_create_wrapper(dri2_dpy->wl_dmabuf);
   if (!dmabuf_wrapper) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      }
   wl_proxy_set_queue((struct wl_proxy *)dmabuf_wrapper,
         dri2_surf->wl_dmabuf_feedback = zwp_linux_dmabuf_v1_get_surface_feedback(
                  zwp_linux_dmabuf_feedback_v1_add_listener(
                  if (dmabuf_feedback_init(&dri2_surf->pending_dmabuf_feedback) < 0) {
      zwp_linux_dmabuf_feedback_v1_destroy(dri2_surf->wl_dmabuf_feedback);
      }
   if (dmabuf_feedback_init(&dri2_surf->dmabuf_feedback) < 0) {
      dmabuf_feedback_fini(&dri2_surf->pending_dmabuf_feedback);
   zwp_linux_dmabuf_feedback_v1_destroy(dri2_surf->wl_dmabuf_feedback);
               if (roundtrip(dri2_dpy) < 0)
               dri2_surf->wl_win = window;
   dri2_surf->wl_win->driver_private = dri2_surf;
   dri2_surf->wl_win->destroy_window_callback = destroy_window_callback;
   if (dri2_dpy->flush)
            if (!dri2_create_drawable(dri2_dpy, config, dri2_surf, dri2_surf))
                           cleanup_dmabuf_feedback:
      if (dri2_surf->wl_dmabuf_feedback) {
      zwp_linux_dmabuf_feedback_v1_destroy(dri2_surf->wl_dmabuf_feedback);
   dmabuf_feedback_fini(&dri2_surf->dmabuf_feedback);
         cleanup_surf_wrapper:
         cleanup_dpy_wrapper:
         cleanup_drm:
      if (dri2_surf->wl_drm_wrapper)
      cleanup_queue:
         cleanup_surf:
                  }
      static _EGLSurface *
   dri2_wl_create_pixmap_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      /* From the EGL_EXT_platform_wayland spec, version 3:
   *
   *   It is not valid to call eglCreatePlatformPixmapSurfaceEXT with a <dpy>
   *   that belongs to Wayland. Any such call fails and generates
   *   EGL_BAD_PARAMETER.
   */
   _eglError(EGL_BAD_PARAMETER, "cannot create EGL pixmap surfaces on "
            }
      /**
   * Called via eglDestroySurface(), drv->DestroySurface().
   */
   static EGLBoolean
   dri2_wl_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
                     for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (dri2_surf->color_buffers[i].wl_buffer)
         if (dri2_surf->color_buffers[i].dri_image)
         if (dri2_surf->color_buffers[i].linear_copy)
         if (dri2_surf->color_buffers[i].data)
      munmap(dri2_surf->color_buffers[i].data,
            if (dri2_dpy->dri2)
            if (dri2_surf->throttle_callback)
            if (dri2_surf->wl_win) {
      dri2_surf->wl_win->driver_private = NULL;
   dri2_surf->wl_win->resize_callback = NULL;
               wl_proxy_wrapper_destroy(dri2_surf->wl_surface_wrapper);
   wl_proxy_wrapper_destroy(dri2_surf->wl_dpy_wrapper);
   if (dri2_surf->wl_drm_wrapper)
         if (dri2_surf->wl_dmabuf_feedback) {
      zwp_linux_dmabuf_feedback_v1_destroy(dri2_surf->wl_dmabuf_feedback);
   dmabuf_feedback_fini(&dri2_surf->dmabuf_feedback);
      }
            dri2_fini_surface(surf);
               }
      static EGLBoolean
   dri2_wl_swap_interval(_EGLDisplay *disp, _EGLSurface *surf, EGLint interval)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (dri2_dpy->kopper)
               }
      static void
   dri2_wl_release_buffers(struct dri2_egl_surface *dri2_surf)
   {
      struct dri2_egl_display *dri2_dpy =
            for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (dri2_surf->color_buffers[i].wl_buffer) {
      if (dri2_surf->color_buffers[i].locked) {
         } else {
      wl_buffer_destroy(dri2_surf->color_buffers[i].wl_buffer);
         }
   if (dri2_surf->color_buffers[i].dri_image)
         if (dri2_surf->color_buffers[i].linear_copy)
         if (dri2_surf->color_buffers[i].data)
                  dri2_surf->color_buffers[i].dri_image = NULL;
   dri2_surf->color_buffers[i].linear_copy = NULL;
   dri2_surf->color_buffers[i].data = NULL;
               if (dri2_dpy->dri2)
      }
      static void
   create_dri_image_from_dmabuf_feedback(struct dri2_egl_surface *dri2_surf,
               {
      struct dri2_egl_display *dri2_dpy =
         int visual_idx;
   uint64_t *modifiers;
   unsigned int num_modifiers;
            /* We don't have valid dma-buf feedback, so return */
   if (dri2_surf->dmabuf_feedback.main_device == 0)
            visual_idx = dri2_wl_visual_idx_from_fourcc(dri2_surf->format);
            /* Iterates through the dma-buf feedback to pick a new set of modifiers. The
   * tranches are sent in descending order of preference by the compositor, so
   * the first set that we can pick is the best one. For now we still can't
   * specify the target device in order to make the render device try its best
   * to allocate memory that can be directly scanned out by the KMS device. But
   * in the future this may change (newer versions of
   * createImageWithModifiers). Also, we are safe to pick modifiers from
   * tranches whose target device differs from the main device, as compositors
   * do not expose (in dma-buf feedback tranches) formats/modifiers that are
   * incompatible with the main device. */
   util_dynarray_foreach (&dri2_surf->dmabuf_feedback.tranches,
            /* Ignore tranches that do not contain dri2_surf->format */
   if (!BITSET_TEST(tranche->formats.formats_bitmap, visual_idx))
         modifiers = u_vector_tail(&tranche->formats.modifiers[visual_idx]);
            /* For the purposes of this function, an INVALID modifier on
   * its own means the modifiers aren't supported. */
   if (num_modifiers == 0 ||
      (num_modifiers == 1 && modifiers[0] == DRM_FORMAT_MOD_INVALID)) {
   num_modifiers = 0;
               flags = use_flags;
   if (tranche->flags & ZWP_LINUX_DMABUF_FEEDBACK_V1_TRANCHE_FLAGS_SCANOUT)
            dri2_surf->back->dri_image = loader_dri_create_image(
      dri2_dpy->dri_screen_render_gpu, dri2_dpy->image,
   dri2_surf->base.Width, dri2_surf->base.Height, dri_image_format,
               if (dri2_surf->back->dri_image)
         }
      static void
   create_dri_image(struct dri2_egl_surface *dri2_surf,
         {
      struct dri2_egl_display *dri2_dpy =
         int visual_idx;
   uint64_t *modifiers;
            visual_idx = dri2_wl_visual_idx_from_fourcc(dri2_surf->format);
   modifiers = u_vector_tail(&dri2_dpy->formats.modifiers[visual_idx]);
            /* For the purposes of this function, an INVALID modifier on
   * its own means the modifiers aren't supported. */
   if (num_modifiers == 0 ||
      (num_modifiers == 1 && modifiers[0] == DRM_FORMAT_MOD_INVALID)) {
   num_modifiers = 0;
               /* If our DRIImage implementation does not support createImageWithModifiers,
   * then fall back to the old createImage, and hope it allocates an image
   * which is acceptable to the winsys. */
   dri2_surf->back->dri_image = loader_dri_create_image(
      dri2_dpy->dri_screen_render_gpu, dri2_dpy->image, dri2_surf->base.Width,
   dri2_surf->base.Height, dri_image_format,
   (dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu) ? 0 : use_flags,
   }
      static int
   get_back_bo(struct dri2_egl_surface *dri2_surf)
   {
      struct dri2_egl_display *dri2_dpy =
         int use_flags;
   int visual_idx;
   unsigned int dri_image_format;
            visual_idx = dri2_wl_visual_idx_from_fourcc(dri2_surf->format);
   assert(visual_idx != -1);
   dri_image_format = dri2_wl_visuals[visual_idx].dri_image_format;
            /* Substitute dri image format if server does not support original format */
   if (!BITSET_TEST(dri2_dpy->formats.formats_bitmap, visual_idx))
      linear_dri_image_format =
         /* These asserts hold, as long as dri2_wl_visuals[] is self-consistent and
   * the PRIME substitution logic in dri2_wl_add_configs_for_visuals() is free
   * of bugs.
   */
   assert(linear_dri_image_format != __DRI_IMAGE_FORMAT_NONE);
   assert(BITSET_TEST(
      dri2_dpy->formats.formats_bitmap,
         /* There might be a buffer release already queued that wasn't processed */
            while (dri2_surf->back == NULL) {
      for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      /* Get an unlocked buffer, preferably one with a dri_buffer
   * already allocated and with minimum age.
   */
                  if (!dri2_surf->back || !dri2_surf->back->dri_image ||
      (dri2_surf->color_buffers[i].age > 0 &&
   dri2_surf->color_buffers[i].age < dri2_surf->back->age))
            if (dri2_surf->back)
            /* If we don't have a buffer, then block on the server to release one for
   * us, and try again. wl_display_dispatch_queue will process any pending
   * events, however not all servers flush on issuing a buffer release
   * event. So, we spam the server with roundtrips as they always cause a
   * client flush.
   */
   if (wl_display_roundtrip_queue(dri2_dpy->wl_dpy, dri2_surf->wl_queue) < 0)
               if (dri2_surf->back == NULL)
                     if (dri2_surf->base.ProtectedContent) {
      /* Protected buffers can't be read from another GPU */
   if (dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu)
                     if (dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu &&
      dri2_surf->back->linear_copy == NULL) {
   uint64_t linear_mod = DRM_FORMAT_MOD_LINEAR;
            if (dri2_dpy->dri_screen_display_gpu) {
      linear_copy_display_gpu_image = loader_dri_create_image(
      dri2_dpy->dri_screen_display_gpu, dri2_dpy->image,
   dri2_surf->base.Width, dri2_surf->base.Height,
               if (linear_copy_display_gpu_image) {
      int i, ret;
   int num_planes = 0;
   int buffer_fds[4];
   int strides[4];
                  if (!dri2_dpy->image->queryImage(linear_copy_display_gpu_image,
                        for (i = 0; i < num_planes; i++) {
                     if (!image) {
                        buffer_fds[i] = -1;
   ret = dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_FD,
         ret &= dri2_dpy->image->queryImage(
                                       if (!ret) {
      do {
      if (buffer_fds[i] != -1)
      } while (--i >= 0);
   dri2_dpy->image->destroyImage(linear_copy_display_gpu_image);
                  /* The linear buffer was created in the display GPU's vram, so we
   * need to make it visible to render GPU
   */
   dri2_surf->back->linear_copy =
      dri2_dpy->image->createImageFromDmaBufs3(
      dri2_dpy->dri_screen_render_gpu, dri2_surf->base.Width,
   dri2_surf->base.Height,
   loader_image_format_to_fourcc(linear_dri_image_format),
   linear_mod, &buffer_fds[0], num_planes, &strides[0],
   &offsets[0], __DRI_YUV_COLOR_SPACE_UNDEFINED,
                  for (i = 0; i < num_planes; ++i) {
      if (buffer_fds[i] != -1)
      }
                  if (!dri2_surf->back->linear_copy) {
      dri2_surf->back->linear_copy = loader_dri_create_image(
      dri2_dpy->dri_screen_render_gpu, dri2_dpy->image,
   dri2_surf->base.Width, dri2_surf->base.Height,
   linear_dri_image_format, use_flags | __DRI_IMAGE_USE_LINEAR,
            if (dri2_surf->back->linear_copy == NULL)
               if (dri2_surf->back->dri_image == NULL) {
      if (dri2_surf->wl_dmabuf_feedback)
      create_dri_image_from_dmabuf_feedback(dri2_surf, dri_image_format,
      if (dri2_surf->back->dri_image == NULL)
                     if (dri2_surf->back->dri_image == NULL)
                        }
      static void
   back_bo_to_dri_buffer(struct dri2_egl_surface *dri2_surf, __DRIbuffer *buffer)
   {
      struct dri2_egl_display *dri2_dpy =
         __DRIimage *image;
                     dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_NAME, &name);
            buffer->attachment = __DRI_BUFFER_BACK_LEFT;
   buffer->name = name;
   buffer->pitch = pitch;
   buffer->cpp = 4;
      }
      /* Value chosen empirically as a compromise between avoiding frequent
   * reallocations and extended time of increased memory consumption due to
   * unused buffers being kept.
   */
   #define BUFFER_TRIM_AGE_HYSTERESIS 20
      static int
   update_buffers(struct dri2_egl_surface *dri2_surf)
   {
      struct dri2_egl_display *dri2_dpy =
            if (dri2_surf->wl_win &&
      (dri2_surf->base.Width != dri2_surf->wl_win->width ||
            dri2_surf->base.Width = dri2_surf->wl_win->width;
   dri2_surf->base.Height = dri2_surf->wl_win->height;
   dri2_surf->dx = dri2_surf->wl_win->dx;
               if (dri2_surf->resized || dri2_surf->received_dmabuf_feedback) {
      dri2_wl_release_buffers(dri2_surf);
   dri2_surf->resized = false;
               if (get_back_bo(dri2_surf) < 0) {
      _eglError(EGL_BAD_ALLOC, "failed to allocate color buffer");
               /* If we have an extra unlocked buffer at this point, we had to do triple
   * buffering for a while, but now can go back to just double buffering.
   * That means we can free any unlocked buffer now. To avoid toggling between
   * going back to double buffering and needing to allocate another buffer too
   * fast we let the unneeded buffer sit around for a short while. */
   for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (!dri2_surf->color_buffers[i].locked &&
      dri2_surf->color_buffers[i].wl_buffer &&
   dri2_surf->color_buffers[i].age > BUFFER_TRIM_AGE_HYSTERESIS) {
   wl_buffer_destroy(dri2_surf->color_buffers[i].wl_buffer);
   dri2_dpy->image->destroyImage(dri2_surf->color_buffers[i].dri_image);
   if (dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu)
      dri2_dpy->image->destroyImage(
      dri2_surf->color_buffers[i].wl_buffer = NULL;
   dri2_surf->color_buffers[i].dri_image = NULL;
   dri2_surf->color_buffers[i].linear_copy = NULL;
                     }
      static int
   update_buffers_if_needed(struct dri2_egl_surface *dri2_surf)
   {
      if (dri2_surf->back != NULL)
               }
      static int
   image_get_buffers(__DRIdrawable *driDrawable, unsigned int format,
               {
               if (update_buffers_if_needed(dri2_surf) < 0)
            buffers->image_mask = __DRI_IMAGE_BUFFER_BACK;
               }
      static void
   dri2_wl_flush_front_buffer(__DRIdrawable *driDrawable, void *loaderPrivate)
   {
      (void)driDrawable;
      }
      static unsigned
   dri2_wl_get_capability(void *loaderPrivate, enum dri_loader_cap cap)
   {
      switch (cap) {
   case DRI_LOADER_CAP_FP16:
         case DRI_LOADER_CAP_RGBA_ORDERING:
         default:
            }
      static const __DRIimageLoaderExtension image_loader_extension = {
               .getBuffers = image_get_buffers,
   .flushFrontBuffer = dri2_wl_flush_front_buffer,
      };
      static void
   wayland_throttle_callback(void *data, struct wl_callback *callback,
         {
               dri2_surf->throttle_callback = NULL;
      }
      static const struct wl_callback_listener throttle_listener = {
            static EGLBoolean
   get_fourcc(struct dri2_egl_display *dri2_dpy, __DRIimage *image, int *fourcc)
   {
      EGLBoolean query;
   int dri_format;
            query =
         if (query)
            query = dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_FORMAT,
         if (!query)
            visual_idx = dri2_wl_visual_idx_from_dri_image_format(dri_format);
   if (visual_idx == -1)
            *fourcc = dri2_wl_visuals[visual_idx].wl_drm_format;
      }
      static struct wl_buffer *
   create_wl_buffer(struct dri2_egl_display *dri2_dpy,
         {
      struct wl_buffer *ret = NULL;
   EGLBoolean query;
   int width, height, fourcc, num_planes;
   uint64_t modifier = DRM_FORMAT_MOD_INVALID;
            query = dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_WIDTH, &width);
   query &=
         query &= get_fourcc(dri2_dpy, image, &fourcc);
   if (!query)
            query = dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_NUM_PLANES,
         if (!query)
            query = dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_MODIFIER_UPPER,
         query &= dri2_dpy->image->queryImage(
         if (query) {
                  bool supported_modifier = false;
   bool mod_invalid_supported = false;
   int visual_idx = dri2_wl_visual_idx_from_fourcc(fourcc);
            uint64_t *mod;
   u_vector_foreach(mod, &dri2_dpy->formats.modifiers[visual_idx])
   {
      if (*mod == DRM_FORMAT_MOD_INVALID) {
         }
   if (*mod == modifier) {
      supported_modifier = true;
         }
   if (!supported_modifier && mod_invalid_supported) {
      /* If the server has advertised DRM_FORMAT_MOD_INVALID then we trust
   * that the client has allocated the buffer with the right implicit
   * modifier for the format, even though it's allocated a buffer the
   * server hasn't explicitly claimed to support. */
   modifier = DRM_FORMAT_MOD_INVALID;
               if (dri2_dpy->wl_dmabuf && supported_modifier) {
      struct zwp_linux_buffer_params_v1 *params;
            /* We don't need a wrapper for wl_dmabuf objects, because we have to
   * create the intermediate params object; we can set the queue on this,
   * and the wl_buffer inherits it race-free. */
   params = zwp_linux_dmabuf_v1_create_params(dri2_dpy->wl_dmabuf);
   if (dri2_surf)
            for (i = 0; i < num_planes; i++) {
      __DRIimage *p_image;
                  p_image = dri2_dpy->image->fromPlanar(image, i, NULL);
   if (!p_image) {
      assert(i == 0);
               query =
         query &= dri2_dpy->image->queryImage(
         query &= dri2_dpy->image->queryImage(
                        if (!query) {
      if (fd >= 0)
         zwp_linux_buffer_params_v1_destroy(params);
               zwp_linux_buffer_params_v1_add(params, fd, i, offset, stride,
                     ret = zwp_linux_buffer_params_v1_create_immed(params, width, height,
            } else {
      struct wl_drm *wl_drm =
                  if (num_planes > 1)
            query = dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_FD, &fd);
   query &=
         if (!query) {
      if (fd >= 0)
                     ret = wl_drm_create_prime_buffer(wl_drm, fd, width, height, fourcc, 0,
                        }
      static EGLBoolean
   try_damage_buffer(struct dri2_egl_surface *dri2_surf, const EGLint *rects,
         {
      if (wl_proxy_get_version((struct wl_proxy *)dri2_surf->wl_surface_wrapper) <
      WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION)
         for (int i = 0; i < n_rects; i++) {
               wl_surface_damage_buffer(dri2_surf->wl_surface_wrapper, rect[0],
            }
      }
      /**
   * Called via eglSwapBuffers(), drv->SwapBuffers().
   */
   static EGLBoolean
   dri2_wl_swap_buffers_with_damage(_EGLDisplay *disp, _EGLSurface *draw,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (!dri2_surf->wl_win)
            /* Flush (and finish glthread) before:
   *   - update_buffers_if_needed because the unmarshalling thread
   *     may be running currently, and we would concurrently alloc/free
   *     the back bo.
   *   - swapping current/back because flushing may free the buffer and
   *     dri_image and reallocate them using get_back_bo (which causes a
   *     a crash because 'current' becomes NULL).
   *   - using any wl_* function because accessing them from this thread
   *     and glthread causes troubles (see #7624 and #8136)
   */
   dri2_flush_drawable_for_swapbuffers(disp, draw);
            while (dri2_surf->throttle_callback != NULL)
      if (wl_display_dispatch_queue(dri2_dpy->wl_dpy, dri2_surf->wl_queue) ==
               for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++)
      if (dri2_surf->color_buffers[i].age > 0)
         /* Make sure we have a back buffer in case we're swapping without ever
   * rendering. */
   if (update_buffers_if_needed(dri2_surf) < 0)
            if (draw->SwapInterval > 0) {
      dri2_surf->throttle_callback =
         wl_callback_add_listener(dri2_surf->throttle_callback, &throttle_listener,
               dri2_surf->back->age = 1;
   dri2_surf->current = dri2_surf->back;
            if (!dri2_surf->current->wl_buffer) {
               if (dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu)
         else
            dri2_surf->current->wl_buffer =
            if (dri2_surf->current->wl_buffer == NULL)
                     wl_buffer_add_listener(dri2_surf->current->wl_buffer, &wl_buffer_listener,
               wl_surface_attach(dri2_surf->wl_surface_wrapper,
                  dri2_surf->wl_win->attached_width = dri2_surf->base.Width;
   dri2_surf->wl_win->attached_height = dri2_surf->base.Height;
   /* reset resize growing parameters */
   dri2_surf->dx = 0;
            /* If the compositor doesn't support damage_buffer, we deliberately
   * ignore the damage region and post maximum damage, due to
   * https://bugs.freedesktop.org/78190 */
   if (!n_rects || !try_damage_buffer(dri2_surf, rects, n_rects))
      wl_surface_damage(dri2_surf->wl_surface_wrapper, 0, 0, INT32_MAX,
         if (dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu) {
      _EGLContext *ctx = _eglGetCurrentContext();
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   dri2_dpy->image->blitImage(
      dri2_ctx->dri_context, dri2_surf->current->linear_copy,
   dri2_surf->current->dri_image, 0, 0, dri2_surf->base.Width,
   dri2_surf->base.Height, 0, 0, dri2_surf->base.Width,
                     /* If we're not waiting for a frame callback then we'll at least throttle
   * to a sync callback so that we always give a chance for the compositor to
   * handle the commit and send a release event before checking for a free
   * buffer */
   if (dri2_surf->throttle_callback == NULL) {
      dri2_surf->throttle_callback = wl_display_sync(dri2_surf->wl_dpy_wrapper);
   wl_callback_add_listener(dri2_surf->throttle_callback, &throttle_listener,
                           }
      static EGLint
   dri2_wl_query_buffer_age(_EGLDisplay *disp, _EGLSurface *surface)
   {
               if (update_buffers_if_needed(dri2_surf) < 0) {
      _eglError(EGL_BAD_ALLOC, "dri2_query_buffer_age");
                  }
      static EGLBoolean
   dri2_wl_swap_buffers(_EGLDisplay *disp, _EGLSurface *draw)
   {
         }
      static struct wl_buffer *
   dri2_wl_create_wayland_buffer_from_image(_EGLDisplay *disp, _EGLImage *img)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img = dri2_egl_image(img);
   __DRIimage *image = dri2_img->dri_image;
   struct wl_buffer *buffer;
            /* Check the upstream display supports this buffer's format. */
   dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_FORMAT, &format);
   visual_idx = dri2_wl_visual_idx_from_dri_image_format(format);
   if (visual_idx == -1)
            if (!BITSET_TEST(dri2_dpy->formats.formats_bitmap, visual_idx))
                     /* The buffer object will have been created with our internal event queue
   * because it is using wl_dmabuf/wl_drm as a proxy factory. We want the
   * buffer to be used by the application so we'll reset it to the display's
   * default event queue. This isn't actually racy, as the only event the
   * buffer can get is a buffer release, which doesn't happen with an explicit
   * attach. */
   if (buffer)
                  bad_format:
      _eglError(EGL_BAD_MATCH, "unsupported image format");
      }
      static int
   dri2_wl_authenticate(_EGLDisplay *disp, uint32_t id)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (dri2_dpy->is_render_node) {
      _eglLog(_EGL_WARNING, "wayland-egl: client asks server to "
            }
            wl_drm_authenticate(dri2_dpy->wl_drm, id);
   if (roundtrip(dri2_dpy) < 0)
            if (!dri2_dpy->authenticated)
            /* reset authenticated */
               }
      static void
   drm_handle_device(void *data, struct wl_drm *drm, const char *device)
   {
      struct dri2_egl_display *dri2_dpy = data;
            dri2_dpy->device_name = strdup(device);
   if (!dri2_dpy->device_name)
            dri2_dpy->fd_render_gpu = loader_open_device(dri2_dpy->device_name);
   if (dri2_dpy->fd_render_gpu == -1) {
      _eglLog(_EGL_WARNING, "wayland-egl: could not open %s (%s)",
         free(dri2_dpy->device_name);
   dri2_dpy->device_name = NULL;
               if (drmGetNodeTypeFromFd(dri2_dpy->fd_render_gpu) == DRM_NODE_RENDER) {
         } else {
      if (drmGetMagic(dri2_dpy->fd_render_gpu, &magic)) {
      close(dri2_dpy->fd_render_gpu);
   dri2_dpy->fd_render_gpu = -1;
   free(dri2_dpy->device_name);
   dri2_dpy->device_name = NULL;
   _eglLog(_EGL_WARNING, "wayland-egl: drmGetMagic failed");
      }
         }
      static void
   drm_handle_format(void *data, struct wl_drm *drm, uint32_t format)
   {
      struct dri2_egl_display *dri2_dpy = data;
            if (visual_idx == -1)
               }
      static void
   drm_handle_capabilities(void *data, struct wl_drm *drm, uint32_t value)
   {
                  }
      static void
   drm_handle_authenticated(void *data, struct wl_drm *drm)
   {
                  }
      static const struct wl_drm_listener drm_listener = {
      .device = drm_handle_device,
   .format = drm_handle_format,
   .authenticated = drm_handle_authenticated,
      };
      static void
   dmabuf_ignore_format(void *data, struct zwp_linux_dmabuf_v1 *dmabuf,
         {
         }
      static void
   dmabuf_handle_modifier(void *data, struct zwp_linux_dmabuf_v1 *dmabuf,
               {
      struct dri2_egl_display *dri2_dpy = data;
   int visual_idx = dri2_wl_visual_idx_from_fourcc(format);
            /* Ignore this if the compositor advertised dma-buf feedback. From version 4
   * onwards (when dma-buf feedback was introduced), the compositor should not
   * advertise this event anymore, but let's keep this for safety. */
   if (dri2_dpy->wl_dmabuf_feedback)
            if (visual_idx == -1)
                     mod = u_vector_add(&dri2_dpy->formats.modifiers[visual_idx]);
   if (mod)
      }
      static const struct zwp_linux_dmabuf_v1_listener dmabuf_listener = {
      .format = dmabuf_ignore_format,
      };
      static void
   wl_drm_bind(struct dri2_egl_display *dri2_dpy)
   {
      dri2_dpy->wl_drm =
      wl_registry_bind(dri2_dpy->wl_registry, dri2_dpy->wl_drm_name,
         }
      static void
   default_dmabuf_feedback_format_table(
      void *data,
   struct zwp_linux_dmabuf_feedback_v1 *zwp_linux_dmabuf_feedback_v1,
      {
               dri2_dpy->format_table.size = size;
   dri2_dpy->format_table.data =
               }
      static void
   default_dmabuf_feedback_main_device(
      void *data, struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
      {
      struct dri2_egl_display *dri2_dpy = data;
   char *node;
   int fd;
            /* Given the device, look for a render node and try to open it. */
   memcpy(&dev, device->data, sizeof(dev));
   node = loader_get_render_node(dev);
   if (!node)
         fd = loader_open_device(node);
   if (fd == -1) {
      free(node);
               dri2_dpy->device_name = node;
   dri2_dpy->fd_render_gpu = fd;
      }
      static void
   default_dmabuf_feedback_tranche_target_device(
      void *data, struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
      {
         }
      static void
   default_dmabuf_feedback_tranche_flags(
      void *data, struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
      {
         }
      static void
   default_dmabuf_feedback_tranche_formats(
      void *data, struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
      {
      struct dri2_egl_display *dri2_dpy = data;
   uint64_t *modifier_ptr, modifier;
   uint32_t format;
   uint16_t *index;
            if (dri2_dpy->format_table.data == MAP_FAILED) {
      _eglLog(_EGL_WARNING, "wayland-egl: we could not map the format table "
                  }
   if (dri2_dpy->format_table.data == NULL) {
      _eglLog(_EGL_WARNING,
         "wayland-egl: compositor didn't advertise a format "
   "table, so we won't be able to use this batch of dma-buf "
               wl_array_for_each (index, indices) {
      format = dri2_dpy->format_table.data[*index].format;
            /* skip formats that we don't support */
   visual_idx = dri2_wl_visual_idx_from_fourcc(format);
   if (visual_idx == -1)
            BITSET_SET(dri2_dpy->formats.formats_bitmap, visual_idx);
   modifier_ptr = u_vector_add(&dri2_dpy->formats.modifiers[visual_idx]);
   if (modifier_ptr)
         }
      static void
   default_dmabuf_feedback_tranche_done(
         {
         }
      static void
   default_dmabuf_feedback_done(
         {
         }
      static const struct zwp_linux_dmabuf_feedback_v1_listener
      dmabuf_feedback_listener = {
      .format_table = default_dmabuf_feedback_format_table,
   .main_device = default_dmabuf_feedback_main_device,
   .tranche_target_device = default_dmabuf_feedback_tranche_target_device,
   .tranche_flags = default_dmabuf_feedback_tranche_flags,
   .tranche_formats = default_dmabuf_feedback_tranche_formats,
   .tranche_done = default_dmabuf_feedback_tranche_done,
   };
      static void
   registry_handle_global_drm(void *data, struct wl_registry *registry,
               {
               if (strcmp(interface, wl_drm_interface.name) == 0) {
      dri2_dpy->wl_drm_version = MIN2(version, 2);
      } else if (strcmp(interface, zwp_linux_dmabuf_v1_interface.name) == 0 &&
            dri2_dpy->wl_dmabuf = wl_registry_bind(
      registry, name, &zwp_linux_dmabuf_v1_interface,
      zwp_linux_dmabuf_v1_add_listener(dri2_dpy->wl_dmabuf, &dmabuf_listener,
         }
      static void
   registry_handle_global_remove(void *data, struct wl_registry *registry,
         {
   }
      static const struct wl_registry_listener registry_listener_drm = {
      .global = registry_handle_global_drm,
      };
      static void
   dri2_wl_setup_swap_interval(_EGLDisplay *disp)
   {
      /* We can't use values greater than 1 on Wayland because we are using the
   * frame callback to synchronise the frame and the only way we be sure to
   * get a frame callback is to attach a new buffer. Therefore we can't just
               }
      static const struct dri2_egl_display_vtbl dri2_wl_display_vtbl = {
      .authenticate = dri2_wl_authenticate,
   .create_window_surface = dri2_wl_create_window_surface,
   .create_pixmap_surface = dri2_wl_create_pixmap_surface,
   .destroy_surface = dri2_wl_destroy_surface,
   .swap_interval = dri2_wl_swap_interval,
   .create_image = dri2_create_image_khr,
   .swap_buffers = dri2_wl_swap_buffers,
   .swap_buffers_with_damage = dri2_wl_swap_buffers_with_damage,
   .query_buffer_age = dri2_wl_query_buffer_age,
   .create_wayland_buffer_from_image = dri2_wl_create_wayland_buffer_from_image,
      };
      static const __DRIextension *dri2_loader_extensions[] = {
      &image_loader_extension.base,
   &image_lookup_extension.base,
   &use_invalidate.base,
      };
      static EGLBoolean
   dri2_wl_add_configs_for_visuals(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   unsigned int format_count[ARRAY_SIZE(dri2_wl_visuals)] = {0};
   unsigned int count = 0;
            for (unsigned i = 0; dri2_dpy->driver_configs[i]; i++) {
               for (unsigned j = 0; j < ARRAY_SIZE(dri2_wl_visuals); j++) {
                              dri2_conf = dri2_add_config(
      disp, dri2_dpy->driver_configs[i], count + 1, EGL_WINDOW_BIT, NULL,
      if (dri2_conf) {
      if (dri2_conf->base.ConfigID == count + 1)
         format_count[j]++;
                  if (!assigned && dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu) {
                     /* No match for config. Try if we can blitImage convert to a visual */
                                 /* Find optimal target visual for blitImage conversion, if any. */
                                 /* Visual s works for the Wayland server, and c can be converted into s
   * by our client gpu during PRIME blitImage conversion to a linear
   * wl_buffer, so add visual c as supported by the client renderer.
   */
   dri2_conf = dri2_add_config(
      disp, dri2_dpy->driver_configs[i], count + 1, EGL_WINDOW_BIT, NULL,
      if (dri2_conf) {
      if (dri2_conf->base.ConfigID == count + 1)
         format_count[c]++;
   if (format_count[c] == 1)
      _eglLog(_EGL_DEBUG,
         "Client format %s to server format %s via "
   "PRIME blitImage.",
                  for (unsigned i = 0; i < ARRAY_SIZE(format_count); i++) {
      if (!format_count[i]) {
      _eglLog(_EGL_DEBUG, "No DRI config supports native format %s",
                     }
      static bool
   dri2_initialize_wayland_drm_extensions(struct dri2_egl_display *dri2_dpy)
   {
      /* Get default dma-buf feedback */
   if (dri2_dpy->wl_dmabuf &&
      zwp_linux_dmabuf_v1_get_version(dri2_dpy->wl_dmabuf) >=
         dmabuf_feedback_format_table_init(&dri2_dpy->format_table);
   dri2_dpy->wl_dmabuf_feedback =
         zwp_linux_dmabuf_feedback_v1_add_listener(
               if (roundtrip(dri2_dpy) < 0)
            /* Destroy the default dma-buf feedback and the format table. */
   if (dri2_dpy->wl_dmabuf_feedback) {
      zwp_linux_dmabuf_feedback_v1_destroy(dri2_dpy->wl_dmabuf_feedback);
   dri2_dpy->wl_dmabuf_feedback = NULL;
               /* We couldn't retrieve a render node from the dma-buf feedback (or the
   * feedback was not advertised at all), so we must fallback to wl_drm. */
   if (dri2_dpy->fd_render_gpu == -1) {
      /* wl_drm not advertised by compositor, so can't continue */
   if (dri2_dpy->wl_drm_name == 0)
                  if (dri2_dpy->wl_drm == NULL)
         if (roundtrip(dri2_dpy) < 0 || dri2_dpy->fd_render_gpu == -1)
            if (!dri2_dpy->authenticated &&
      (roundtrip(dri2_dpy) < 0 || !dri2_dpy->authenticated))
   }
      }
      static EGLBoolean
   dri2_initialize_wayland_drm(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_display_create();
   if (!dri2_dpy)
                     if (dri2_wl_formats_init(&dri2_dpy->formats) < 0)
            if (disp->PlatformDisplay == NULL) {
      dri2_dpy->wl_dpy = wl_display_connect(NULL);
   if (dri2_dpy->wl_dpy == NULL)
            } else {
                           dri2_dpy->wl_dpy_wrapper = wl_proxy_create_wrapper(dri2_dpy->wl_dpy);
   if (dri2_dpy->wl_dpy_wrapper == NULL)
            wl_proxy_set_queue((struct wl_proxy *)dri2_dpy->wl_dpy_wrapper,
            if (dri2_dpy->own_device)
            dri2_dpy->wl_registry = wl_display_get_registry(dri2_dpy->wl_dpy_wrapper);
   wl_registry_add_listener(dri2_dpy->wl_registry, &registry_listener_drm,
            if (roundtrip(dri2_dpy) < 0)
            if (!dri2_initialize_wayland_drm_extensions(dri2_dpy))
            loader_get_user_preferred_fd(&dri2_dpy->fd_render_gpu,
            if (dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu) {
      free(dri2_dpy->device_name);
   dri2_dpy->device_name =
         if (!dri2_dpy->device_name) {
      _eglError(EGL_BAD_ALLOC, "wayland-egl: failed to get device name "
                        /* we have to do the check now, because loader_get_user_preferred_fd
   * will return a render-node when the requested gpu is different
   * to the server, but also if the client asks for the same gpu than
   * the server by requesting its pci-id */
   dri2_dpy->is_render_node =
            dri2_dpy->driver_name = loader_get_driver_for_fd(dri2_dpy->fd_render_gpu);
   if (dri2_dpy->driver_name == NULL) {
      _eglError(EGL_BAD_ALLOC, "DRI2: failed to get driver name");
               dri2_dpy->loader_extensions = dri2_loader_extensions;
   if (!dri2_load_driver_dri3(disp)) {
      _eglError(EGL_BAD_ALLOC, "DRI2: failed to load driver");
               if (!dri2_create_screen(disp))
            if (!dri2_setup_extensions(disp))
            if (!dri2_setup_device(disp, false)) {
      _eglError(EGL_NOT_INITIALIZED, "DRI2: failed to setup EGLDevice");
                                 if (dri2_dpy->wl_drm) {
      /* To use Prime, we must have _DRI_IMAGE v7 at least. createImageFromFds
   * support indicates that Prime export/import is supported by the driver.
   * We deprecated the support to GEM names API, so we bail out if the
   * driver does not support Prime. */
   if (!(dri2_dpy->capabilities & WL_DRM_CAPABILITY_PRIME) ||
      (dri2_dpy->image->createImageFromFds == NULL)) {
   _eglLog(_EGL_WARNING, "wayland-egl: display does not support prime");
                  if (dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu &&
      dri2_dpy->image->blitImage == NULL) {
   _eglLog(_EGL_WARNING, "wayland-egl: Different GPU selected, but the "
                           if (!dri2_wl_add_configs_for_visuals(disp)) {
      _eglError(EGL_NOT_INITIALIZED, "DRI2: failed to add configs");
               dri2_set_WL_bind_wayland_display(disp);
   /* When cannot convert EGLImage to wl_buffer when on a different gpu,
   * because the buffer of the EGLImage has likely a tiling mode the server
   * gpu won't support. These is no way to check for now. Thus do not support
   * the extension */
   if (dri2_dpy->fd_render_gpu == dri2_dpy->fd_display_gpu)
                                       /* Fill vtbl last to prevent accidentally calling virtual function during
   * initialization.
   */
                  cleanup:
      dri2_display_destroy(disp);
      }
      static int
   dri2_wl_swrast_get_stride_for_format(int format, int w)
   {
                           }
      static EGLBoolean
   dri2_wl_swrast_allocate_buffer(struct dri2_egl_surface *dri2_surf, int format,
               {
      struct dri2_egl_display *dri2_dpy =
         struct wl_shm_pool *pool;
   int fd, stride, size_map;
            stride = dri2_wl_swrast_get_stride_for_format(format, w);
            /* Create a shareable buffer */
   fd = os_create_anonymous_file(size_map, NULL);
   if (fd < 0)
            data_map = mmap(NULL, size_map, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (data_map == MAP_FAILED) {
      close(fd);
               /* Share it in a wl_buffer */
   pool = wl_shm_create_pool(dri2_dpy->wl_shm, fd, size_map);
   wl_proxy_set_queue((struct wl_proxy *)pool, dri2_surf->wl_queue);
   *buffer = wl_shm_pool_create_buffer(pool, 0, w, h, stride, format);
   wl_shm_pool_destroy(pool);
            *data = data_map;
   *size = size_map;
      }
      static int
   swrast_update_buffers(struct dri2_egl_surface *dri2_surf)
   {
      struct dri2_egl_display *dri2_dpy =
                  /* we need to do the following operations only once per frame */
   if (dri2_surf->back)
            if (dri2_surf->wl_win &&
      (dri2_surf->base.Width != dri2_surf->wl_win->width ||
            if (!zink)
            dri2_surf->base.Width = dri2_surf->wl_win->width;
   dri2_surf->base.Height = dri2_surf->wl_win->height;
   dri2_surf->dx = dri2_surf->wl_win->dx;
   dri2_surf->dy = dri2_surf->wl_win->dy;
               /* find back buffer */
   if (zink)
            /* There might be a buffer release already queued that wasn't processed */
            /* Try to get free buffer already created and with minimum age */
   for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (dri2_surf->color_buffers[i].locked ||
                  if (!dri2_surf->back ||
      (dri2_surf->color_buffers[i].age > 0 &&
   dri2_surf->color_buffers[i].age < dri2_surf->back->age))
            /* else choose any another free location */
   while (!dri2_surf->back) {
      for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (!dri2_surf->color_buffers[i].locked) {
      dri2_surf->back = &dri2_surf->color_buffers[i];
   if (!dri2_wl_swrast_allocate_buffer(
         dri2_surf, dri2_surf->format, dri2_surf->base.Width,
   dri2_surf->base.Height, &dri2_surf->back->data,
      _eglError(EGL_BAD_ALLOC, "failed to allocate color buffer");
      }
   wl_buffer_add_listener(dri2_surf->back->wl_buffer,
                        /* wait for the compositor to release a buffer */
   if (!dri2_surf->back) {
      if (wl_display_dispatch_queue(dri2_dpy->wl_dpy, dri2_surf->wl_queue) ==
      -1) {
   _eglError(EGL_BAD_ALLOC, "waiting for a free buffer failed");
                              /* If we have an extra unlocked buffer at this point, we had to do triple
   * buffering for a while, but now can go back to just double buffering.
   * That means we can free any unlocked buffer now. To avoid toggling between
   * going back to double buffering and needing to allocate another buffer too
   * fast we let the unneeded buffer sit around for a short while. */
   for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (!dri2_surf->color_buffers[i].locked &&
      dri2_surf->color_buffers[i].wl_buffer &&
   dri2_surf->color_buffers[i].age > BUFFER_TRIM_AGE_HYSTERESIS) {
   wl_buffer_destroy(dri2_surf->color_buffers[i].wl_buffer);
   munmap(dri2_surf->color_buffers[i].data,
         dri2_surf->color_buffers[i].wl_buffer = NULL;
   dri2_surf->color_buffers[i].data = NULL;
                     }
      static void *
   dri2_wl_swrast_get_frontbuffer_data(struct dri2_egl_surface *dri2_surf)
   {
      /* if there has been a resize: */
   if (!dri2_surf->current)
               }
      static void *
   dri2_wl_swrast_get_backbuffer_data(struct dri2_egl_surface *dri2_surf)
   {
      assert(dri2_surf->back);
      }
      static void
   dri2_wl_swrast_commit_backbuffer(struct dri2_egl_surface *dri2_surf)
   {
      struct dri2_egl_display *dri2_dpy =
            while (dri2_surf->throttle_callback != NULL)
      if (wl_display_dispatch_queue(dri2_dpy->wl_dpy, dri2_surf->wl_queue) ==
               if (dri2_surf->base.SwapInterval > 0) {
      dri2_surf->throttle_callback =
         wl_callback_add_listener(dri2_surf->throttle_callback, &throttle_listener,
               dri2_surf->current = dri2_surf->back;
            wl_surface_attach(dri2_surf->wl_surface_wrapper,
                  dri2_surf->wl_win->attached_width = dri2_surf->base.Width;
   dri2_surf->wl_win->attached_height = dri2_surf->base.Height;
   /* reset resize growing parameters */
   dri2_surf->dx = 0;
            wl_surface_damage(dri2_surf->wl_surface_wrapper, 0, 0, INT32_MAX, INT32_MAX);
            /* If we're not waiting for a frame callback then we'll at least throttle
   * to a sync callback so that we always give a chance for the compositor to
   * handle the commit and send a release event before checking for a free
   * buffer */
   if (dri2_surf->throttle_callback == NULL) {
      dri2_surf->throttle_callback = wl_display_sync(dri2_surf->wl_dpy_wrapper);
   wl_callback_add_listener(dri2_surf->throttle_callback, &throttle_listener,
                  }
      static void
   dri2_wl_swrast_get_drawable_info(__DRIdrawable *draw, int *x, int *y, int *w,
         {
               (void)swrast_update_buffers(dri2_surf);
   *x = 0;
   *y = 0;
   *w = dri2_surf->base.Width;
      }
      static void
   dri2_wl_swrast_get_image(__DRIdrawable *read, int x, int y, int w, int h,
         {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int copy_width = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, w);
   int x_offset = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, x);
   int src_stride = dri2_wl_swrast_get_stride_for_format(dri2_surf->format,
         int dst_stride = copy_width;
            src = dri2_wl_swrast_get_frontbuffer_data(dri2_surf);
   if (!src) {
      memset(data, 0, copy_width * h);
               assert(data != src);
            src += x_offset;
   src += y * src_stride;
            if (copy_width > src_stride - x_offset)
         if (h > dri2_surf->base.Height - y)
            for (; h > 0; h--) {
      memcpy(dst, src, copy_width);
   src += src_stride;
         }
      static void
   dri2_wl_swrast_put_image2(__DRIdrawable *draw, int op, int x, int y, int w,
         {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int copy_width = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, w);
   int dst_stride = dri2_wl_swrast_get_stride_for_format(dri2_surf->format,
         int x_offset = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, x);
                     (void)swrast_update_buffers(dri2_surf);
            /* partial copy, copy old content */
   if (copy_width < dst_stride)
      dri2_wl_swrast_get_image(draw, 0, 0, dri2_surf->base.Width,
         dst += x_offset;
                     /* drivers expect we do these checks (and some rely on it) */
   if (copy_width > dst_stride - x_offset)
         if (h > dri2_surf->base.Height - y)
            for (; h > 0; h--) {
      memcpy(dst, src, copy_width);
   src += stride;
      }
      }
      static void
   dri2_wl_swrast_put_image(__DRIdrawable *draw, int op, int x, int y, int w,
         {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
            stride = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, w);
      }
      static EGLBoolean
   dri2_wl_swrast_swap_buffers(_EGLDisplay *disp, _EGLSurface *draw)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (!dri2_surf->wl_win)
            dri2_dpy->core->swapBuffers(dri2_surf->dri_drawable);
   if (disp->Options.Zink) {
      dri2_surf->current = dri2_surf->back;
      }
      }
      static void
   shm_handle_format(void *data, struct wl_shm *shm, uint32_t format)
   {
      struct dri2_egl_display *dri2_dpy = data;
            if (visual_idx == -1)
               }
      static const struct wl_shm_listener shm_listener = {
         };
      static void
   registry_handle_global_swrast(void *data, struct wl_registry *registry,
               {
               if (strcmp(interface, wl_shm_interface.name) == 0) {
      dri2_dpy->wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
      }
   if (dri2_dpy->fd_render_gpu != -1 || dri2_dpy->fd_display_gpu != -1) {
      if (strcmp(interface, wl_drm_interface.name) == 0) {
      dri2_dpy->wl_drm_version = MIN2(version, 2);
      } else if (strcmp(interface, zwp_linux_dmabuf_v1_interface.name) == 0 &&
            dri2_dpy->wl_dmabuf = wl_registry_bind(
      registry, name, &zwp_linux_dmabuf_v1_interface,
   MIN2(version,
      zwp_linux_dmabuf_v1_add_listener(dri2_dpy->wl_dmabuf, &dmabuf_listener,
            }
      static const struct wl_registry_listener registry_listener_swrast = {
      .global = registry_handle_global_swrast,
      };
      static const struct dri2_egl_display_vtbl dri2_wl_swrast_display_vtbl = {
      .authenticate = NULL,
   .create_window_surface = dri2_wl_create_window_surface,
   .create_pixmap_surface = dri2_wl_create_pixmap_surface,
   .destroy_surface = dri2_wl_destroy_surface,
   .create_image = dri2_create_image_khr,
   .swap_buffers = dri2_wl_swrast_swap_buffers,
      };
      static const __DRIswrastLoaderExtension swrast_loader_extension = {
               .getDrawableInfo = dri2_wl_swrast_get_drawable_info,
   .putImage = dri2_wl_swrast_put_image,
   .getImage = dri2_wl_swrast_get_image,
      };
      static_assert(sizeof(struct kopper_vk_surface_create_storage) >=
                  static void
   kopperSetSurfaceCreateInfo(void *_draw, struct kopper_loader_info *out)
   {
      struct dri2_egl_surface *dri2_surf = _draw;
   struct dri2_egl_display *dri2_dpy =
         VkWaylandSurfaceCreateInfoKHR *wlsci =
            wlsci->sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
   wlsci->pNext = NULL;
   wlsci->flags = 0;
   wlsci->display = dri2_dpy->wl_dpy;
      }
      static const __DRIkopperLoaderExtension kopper_loader_extension = {
                  };
   static const __DRIextension *swrast_loader_extensions[] = {
      &swrast_loader_extension.base,
   &image_lookup_extension.base,
   &kopper_loader_extension.base,
      };
      static EGLBoolean
   dri2_initialize_wayland_swrast(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_display_create();
   if (!dri2_dpy)
                     if (dri2_wl_formats_init(&dri2_dpy->formats) < 0)
            if (disp->PlatformDisplay == NULL) {
      dri2_dpy->wl_dpy = wl_display_connect(NULL);
   if (dri2_dpy->wl_dpy == NULL)
            } else {
                           dri2_dpy->wl_dpy_wrapper = wl_proxy_create_wrapper(dri2_dpy->wl_dpy);
   if (dri2_dpy->wl_dpy_wrapper == NULL)
            wl_proxy_set_queue((struct wl_proxy *)dri2_dpy->wl_dpy_wrapper,
            if (dri2_dpy->own_device)
            dri2_dpy->wl_registry = wl_display_get_registry(dri2_dpy->wl_dpy_wrapper);
   wl_registry_add_listener(dri2_dpy->wl_registry, &registry_listener_swrast,
            if (roundtrip(dri2_dpy) < 0 || dri2_dpy->wl_shm == NULL)
            if (roundtrip(dri2_dpy) < 0 ||
      !BITSET_TEST_RANGE(dri2_dpy->formats.formats_bitmap, 0,
               if (disp->Options.Zink)
            dri2_dpy->driver_name = strdup(disp->Options.Zink ? "zink" : "swrast");
   if (!dri2_load_driver_swrast(disp))
                     if (!dri2_create_screen(disp))
            if (!dri2_setup_extensions(disp))
            if (!dri2_setup_device(disp, true)) {
      _eglError(EGL_NOT_INITIALIZED, "DRI2: failed to setup EGLDevice");
                                 if (!dri2_wl_add_configs_for_visuals(disp)) {
      _eglError(EGL_NOT_INITIALIZED, "DRI2: failed to add configs");
               if (disp->Options.Zink && dri2_dpy->fd_render_gpu >= 0 &&
      (dri2_dpy->wl_dmabuf || dri2_dpy->wl_drm))
      disp->Extensions.EXT_swap_buffers_with_damage = EGL_TRUE;
            /* Fill vtbl last to prevent accidentally calling virtual function during
   * initialization.
   */
                  cleanup:
      dri2_display_destroy(disp);
      }
      EGLBoolean
   dri2_initialize_wayland(_EGLDisplay *disp)
   {
      if (disp->Options.ForceSoftware || disp->Options.Zink)
         else
      }
      void
   dri2_teardown_wayland(struct dri2_egl_display *dri2_dpy)
   {
      dri2_wl_formats_fini(&dri2_dpy->formats);
   if (dri2_dpy->wl_drm)
         if (dri2_dpy->wl_dmabuf)
         if (dri2_dpy->wl_shm)
         if (dri2_dpy->wl_registry)
         if (dri2_dpy->wl_dpy_wrapper)
         if (dri2_dpy->wl_queue)
            if (dri2_dpy->own_device)
      }
