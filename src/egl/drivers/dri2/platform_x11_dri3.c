   /*
   * Copyright © 2015 Boyan Ding
   *
   * Permission to use, copy, modify, distribute, and sell this software and its
   * documentation for any purpose is hereby granted without fee, provided that
   * the above copyright notice appear in all copies and that both that copyright
   * notice and this permission notice appear in supporting documentation, and
   * that the name of the copyright holders not be used in advertising or
   * publicity pertaining to distribution of the software without specific,
   * written prior permission.  The copyright holders make no representations
   * about the suitability of this software for any purpose.  It is provided "as
   * is" without express or implied warranty.
   *
   * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
   * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
   * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
   * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
   * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   * OF THIS SOFTWARE.
   */
      #include <fcntl.h>
   #include <stdbool.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
      #include <xcb/dri3.h>
   #include <xcb/present.h>
   #include <xcb/xcb.h>
   #include <xcb/xfixes.h>
      #include <xf86drm.h>
   #include "util/macros.h"
      #include "egl_dri2.h"
   #include "platform_x11_dri3.h"
      #include "loader.h"
   #include "loader_dri3_helper.h"
      static struct dri3_egl_surface *
   loader_drawable_to_egl_surface(struct loader_dri3_drawable *draw)
   {
      size_t offset = offsetof(struct dri3_egl_surface, loader_drawable);
      }
      static void
   egl_dri3_set_drawable_size(struct loader_dri3_drawable *draw, int width,
         {
               dri3_surf->surf.base.Width = width;
      }
      static bool
   egl_dri3_in_current_context(struct loader_dri3_drawable *draw)
   {
      struct dri3_egl_surface *dri3_surf = loader_drawable_to_egl_surface(draw);
               }
      static __DRIcontext *
   egl_dri3_get_dri_context(struct loader_dri3_drawable *draw)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
   struct dri2_egl_context *dri2_ctx;
   if (!ctx)
         dri2_ctx = dri2_egl_context(ctx);
      }
      static __DRIscreen *
   egl_dri3_get_dri_screen(void)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
   struct dri2_egl_context *dri2_ctx;
   if (!ctx)
         dri2_ctx = dri2_egl_context(ctx);
   return dri2_egl_display(dri2_ctx->base.Resource.Display)
      }
      static void
   egl_dri3_flush_drawable(struct loader_dri3_drawable *draw, unsigned flags)
   {
      struct dri3_egl_surface *dri3_surf = loader_drawable_to_egl_surface(draw);
               }
      static const struct loader_dri3_vtable egl_dri3_vtable = {
      .set_drawable_size = egl_dri3_set_drawable_size,
   .in_current_context = egl_dri3_in_current_context,
   .get_dri_context = egl_dri3_get_dri_context,
   .get_dri_screen = egl_dri3_get_dri_screen,
      };
      static EGLBoolean
   dri3_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri3_egl_surface *dri3_surf = dri3_egl_surface(surf);
                     if (surf->Type == EGL_PBUFFER_BIT)
            dri2_fini_surface(surf);
               }
      static EGLBoolean
   dri3_set_swap_interval(_EGLDisplay *disp, _EGLSurface *surf, EGLint interval)
   {
               dri3_surf->surf.base.SwapInterval = interval;
               }
      static enum loader_dri3_drawable_type
   egl_to_loader_dri3_drawable_type(EGLint type)
   {
      switch (type) {
   case EGL_WINDOW_BIT:
         case EGL_PIXMAP_BIT:
         case EGL_PBUFFER_BIT:
         default:
            }
      static _EGLSurface *
   dri3_create_surface(_EGLDisplay *disp, EGLint type, _EGLConfig *conf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct dri3_egl_surface *dri3_surf;
   const __DRIconfig *dri_config;
            dri3_surf = calloc(1, sizeof *dri3_surf);
   if (!dri3_surf) {
      _eglError(EGL_BAD_ALLOC, "dri3_create_surface");
               if (!dri2_init_surface(&dri3_surf->surf.base, disp, type, conf, attrib_list,
                  if (type == EGL_PBUFFER_BIT) {
      drawable = xcb_generate_id(dri2_dpy->conn);
   xcb_create_pixmap(dri2_dpy->conn, conf->BufferSize, drawable,
            } else {
      STATIC_ASSERT(sizeof(uintptr_t) == sizeof(native_surface));
               dri_config =
            if (!dri_config) {
      _eglError(EGL_BAD_MATCH,
                     if (loader_dri3_drawable_init(
         dri2_dpy->conn, drawable, egl_to_loader_dri3_drawable_type(type),
   dri2_dpy->dri_screen_render_gpu, dri2_dpy->dri_screen_display_gpu,
   dri2_dpy->multibuffers_available, true, dri_config,
   &dri2_dpy->loader_dri3_ext, &egl_dri3_vtable,
      _eglError(EGL_BAD_ALLOC, "dri3_surface_create");
               if (dri3_surf->surf.base.ProtectedContent &&
      dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu) {
   _eglError(EGL_BAD_ALLOC, "dri3_surface_create");
               dri3_surf->loader_drawable.is_protected_content =
                  cleanup_pixmap:
      if (type == EGL_PBUFFER_BIT)
      cleanup_surf:
                  }
      static int
   dri3_authenticate(_EGLDisplay *disp, uint32_t id)
   {
   #ifdef HAVE_WAYLAND_PLATFORM
               if (dri2_dpy->device_name) {
      _eglLog(_EGL_WARNING,
                     _eglLog(_EGL_WARNING,
      #endif
            }
      /**
   * Called via eglCreateWindowSurface(), drv->CreateWindowSurface().
   */
   static _EGLSurface *
   dri3_create_window_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            surf = dri3_create_surface(disp, EGL_WINDOW_BIT, conf, native_window,
         if (surf != NULL)
               }
      static _EGLSurface *
   dri3_create_pixmap_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      return dri3_create_surface(disp, EGL_PIXMAP_BIT, conf, native_pixmap,
      }
      static _EGLSurface *
   dri3_create_pbuffer_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
         }
      static EGLBoolean
   dri3_get_sync_values(_EGLDisplay *display, _EGLSurface *surface,
         {
               return loader_dri3_wait_for_msc(&dri3_surf->loader_drawable, 0, 0, 0,
                        }
      static _EGLImage *
   dri3_create_image_khr_pixmap(_EGLDisplay *disp, _EGLContext *ctx,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img;
   xcb_drawable_t drawable;
   xcb_dri3_buffer_from_pixmap_cookie_t bp_cookie;
   xcb_dri3_buffer_from_pixmap_reply_t *bp_reply;
            drawable = (xcb_drawable_t)(uintptr_t)buffer;
   bp_cookie = xcb_dri3_buffer_from_pixmap(dri2_dpy->conn, drawable);
   bp_reply =
         if (!bp_reply) {
      _eglError(EGL_BAD_ALLOC, "xcb_dri3_buffer_from_pixmap");
               format = dri2_format_for_depth(dri2_dpy, bp_reply->depth);
   if (format == __DRI_IMAGE_FORMAT_NONE) {
      _eglError(EGL_BAD_PARAMETER,
         free(bp_reply);
               dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri3_create_image_khr");
   free(bp_reply);
                        dri2_img->dri_image = loader_dri3_create_image(
      dri2_dpy->conn, bp_reply, format, dri2_dpy->dri_screen_render_gpu,
                     }
      #ifdef HAVE_DRI3_MODIFIERS
   static _EGLImage *
   dri3_create_image_khr_pixmap_from_buffers(_EGLDisplay *disp, _EGLContext *ctx,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img;
   xcb_dri3_buffers_from_pixmap_cookie_t bp_cookie;
   xcb_dri3_buffers_from_pixmap_reply_t *bp_reply;
   xcb_drawable_t drawable;
            drawable = (xcb_drawable_t)(uintptr_t)buffer;
   bp_cookie = xcb_dri3_buffers_from_pixmap(dri2_dpy->conn, drawable);
   bp_reply =
            if (!bp_reply) {
      _eglError(EGL_BAD_ATTRIBUTE, "dri3_create_image_khr");
               format = dri2_format_for_depth(dri2_dpy, bp_reply->depth);
   if (format == __DRI_IMAGE_FORMAT_NONE) {
      _eglError(EGL_BAD_PARAMETER,
         free(bp_reply);
               dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri3_create_image_khr");
   free(bp_reply);
                        dri2_img->dri_image = loader_dri3_create_image_from_buffers(
      dri2_dpy->conn, bp_reply, format, dri2_dpy->dri_screen_render_gpu,
               if (!dri2_img->dri_image) {
      _eglError(EGL_BAD_ATTRIBUTE, "dri3_create_image_khr");
   free(dri2_img);
                  }
   #endif
      static _EGLImage *
   dri3_create_image_khr(_EGLDisplay *disp, _EGLContext *ctx, EGLenum target,
         {
   #ifdef HAVE_DRI3_MODIFIERS
         #endif
         switch (target) {
      #ifdef HAVE_DRI3_MODIFIERS
         if (dri2_dpy->multibuffers_available)
         #endif
            default:
            }
      /**
   * Called by the driver when it needs to update the real front buffer with the
   * contents of its fake front buffer.
   */
   static void
   dri3_flush_front_buffer(__DRIdrawable *driDrawable, void *loaderPrivate)
   {
      struct loader_dri3_drawable *draw = loaderPrivate;
            /* There does not seem to be any kind of consensus on whether we should
   * support front-buffer rendering or not:
   * http://lists.freedesktop.org/archives/mesa-dev/2013-June/040129.html
   */
   if (draw->type == LOADER_DRI3_DRAWABLE_WINDOW)
      _eglLog(_EGL_WARNING,
   }
      const __DRIimageLoaderExtension dri3_image_loader_extension = {
               .getBuffers = loader_dri3_get_buffers,
      };
      static EGLBoolean
   dri3_swap_buffers_with_damage(_EGLDisplay *disp, _EGLSurface *draw,
         {
               return loader_dri3_swap_buffers_msc(
            }
      static EGLBoolean
   dri3_swap_buffers(_EGLDisplay *disp, _EGLSurface *draw)
   {
         }
      static EGLBoolean
   dri3_copy_buffers(_EGLDisplay *disp, _EGLSurface *surf,
         {
      struct dri3_egl_surface *dri3_surf = dri3_egl_surface(surf);
            STATIC_ASSERT(sizeof(uintptr_t) == sizeof(native_pixmap_target));
            loader_dri3_copy_drawable(&dri3_surf->loader_drawable, target,
               }
      static int
   dri3_query_buffer_age(_EGLDisplay *disp, _EGLSurface *surf)
   {
                  }
      static EGLBoolean
   dri3_query_surface(_EGLDisplay *disp, _EGLSurface *surf, EGLint attribute,
         {
               switch (attribute) {
   case EGL_WIDTH:
   case EGL_HEIGHT:
      loader_dri3_update_drawable_geometry(&dri3_surf->loader_drawable);
      default:
                     }
      static __DRIdrawable *
   dri3_get_dri_drawable(_EGLSurface *surf)
   {
                  }
      static void
   dri3_close_screen_notify(_EGLDisplay *disp)
   {
                  }
      struct dri2_egl_display_vtbl dri3_x11_display_vtbl = {
      .authenticate = dri3_authenticate,
   .create_window_surface = dri3_create_window_surface,
   .create_pixmap_surface = dri3_create_pixmap_surface,
   .create_pbuffer_surface = dri3_create_pbuffer_surface,
   .destroy_surface = dri3_destroy_surface,
   .create_image = dri3_create_image_khr,
   .swap_interval = dri3_set_swap_interval,
   .swap_buffers = dri3_swap_buffers,
   .swap_buffers_with_damage = dri3_swap_buffers_with_damage,
   .copy_buffers = dri3_copy_buffers,
   .query_buffer_age = dri3_query_buffer_age,
   .query_surface = dri3_query_surface,
   .get_sync_values = dri3_get_sync_values,
   .get_msc_rate = dri2_x11_get_msc_rate,
   .get_dri_drawable = dri3_get_dri_drawable,
      };
      /* Only request versions of these protocols which we actually support. */
   #define DRI3_SUPPORTED_MAJOR    1
   #define PRESENT_SUPPORTED_MAJOR 1
      #ifdef HAVE_DRI3_MODIFIERS
   #define DRI3_SUPPORTED_MINOR    2
   #define PRESENT_SUPPORTED_MINOR 2
   #else
   #define PRESENT_SUPPORTED_MINOR 0
   #define DRI3_SUPPORTED_MINOR    0
   #endif
      EGLBoolean
   dri3_x11_connect(struct dri2_egl_display *dri2_dpy)
   {
      xcb_dri3_query_version_reply_t *dri3_query;
   xcb_dri3_query_version_cookie_t dri3_query_cookie;
   xcb_present_query_version_reply_t *present_query;
   xcb_present_query_version_cookie_t present_query_cookie;
   xcb_xfixes_query_version_reply_t *xfixes_query;
   xcb_xfixes_query_version_cookie_t xfixes_query_cookie;
   xcb_generic_error_t *error;
            dri2_dpy->dri3_major_version = 0;
   dri2_dpy->dri3_minor_version = 0;
   dri2_dpy->present_major_version = 0;
            xcb_prefetch_extension_data(dri2_dpy->conn, &xcb_dri3_id);
   xcb_prefetch_extension_data(dri2_dpy->conn, &xcb_present_id);
            extension = xcb_get_extension_data(dri2_dpy->conn, &xcb_dri3_id);
   if (!(extension && extension->present))
            extension = xcb_get_extension_data(dri2_dpy->conn, &xcb_present_id);
   if (!(extension && extension->present))
            extension = xcb_get_extension_data(dri2_dpy->conn, &xcb_xfixes_id);
   if (!(extension && extension->present))
            dri3_query_cookie = xcb_dri3_query_version(
            present_query_cookie = xcb_present_query_version(
            xfixes_query_cookie = xcb_xfixes_query_version(
            dri3_query =
         if (dri3_query == NULL || error != NULL) {
      _eglLog(_EGL_WARNING, "DRI3: failed to query the version");
   free(dri3_query);
   free(error);
               dri2_dpy->dri3_major_version = dri3_query->major_version;
   dri2_dpy->dri3_minor_version = dri3_query->minor_version;
            present_query = xcb_present_query_version_reply(
         if (present_query == NULL || error != NULL) {
      _eglLog(_EGL_WARNING, "DRI3: failed to query Present version");
   free(present_query);
   free(error);
               dri2_dpy->present_major_version = present_query->major_version;
   dri2_dpy->present_minor_version = present_query->minor_version;
            xfixes_query = xcb_xfixes_query_version_reply(dri2_dpy->conn,
         if (xfixes_query == NULL || error != NULL ||
      xfixes_query->major_version < 2) {
   _eglLog(_EGL_WARNING, "DRI3: failed to query xfixes version");
   free(error);
   free(xfixes_query);
      }
            dri2_dpy->fd_render_gpu =
         if (dri2_dpy->fd_render_gpu < 0) {
      int conn_error = xcb_connection_has_error(dri2_dpy->conn);
            if (conn_error)
                        loader_get_user_preferred_fd(&dri2_dpy->fd_render_gpu,
            if (!dri2_dpy->driver_name)
         if (!dri2_dpy->driver_name) {
      _eglLog(_EGL_WARNING, "DRI3: No driver found");
   close(dri2_dpy->fd_render_gpu);
            #ifdef HAVE_WAYLAND_PLATFORM
      /* Only try to get a render device name since dri3 doesn't provide a
   * mechanism for authenticating client opened device node fds. If this
   * fails then don't advertise the extension. */
   dri2_dpy->device_name =
      #endif
            }
