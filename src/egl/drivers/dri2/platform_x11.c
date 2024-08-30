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
   #include <errno.h>
   #include <fcntl.h>
   #include <limits.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   /* clang-format off */
   #include <xcb/xcb.h>
   #include <vulkan/vulkan_core.h>
   #include <vulkan/vulkan_xcb.h>
   /* clang-format on */
   #ifdef HAVE_LIBDRM
   #include <xf86drm.h>
   #endif
   #include "util/bitscan.h"
   #include "util/macros.h"
   #include "util/u_debug.h"
   #include <sys/stat.h>
   #include <sys/types.h>
      #include "kopper_interface.h"
   #include "loader.h"
   #include "platform_x11.h"
      #ifdef HAVE_DRI3
   #include "platform_x11_dri3.h"
   #endif
      static EGLBoolean
   dri2_x11_swap_interval(_EGLDisplay *disp, _EGLSurface *surf, EGLint interval);
      static void
   swrastCreateDrawable(struct dri2_egl_display *dri2_dpy,
         {
      uint32_t mask;
   const uint32_t function = GXcopy;
            /* create GC's */
   dri2_surf->gc = xcb_generate_id(dri2_dpy->conn);
   mask = XCB_GC_FUNCTION;
   xcb_create_gc(dri2_dpy->conn, dri2_surf->gc, dri2_surf->drawable, mask,
            dri2_surf->swapgc = xcb_generate_id(dri2_dpy->conn);
   mask = XCB_GC_FUNCTION | XCB_GC_GRAPHICS_EXPOSURES;
   valgc[0] = function;
   valgc[1] = False;
   xcb_create_gc(dri2_dpy->conn, dri2_surf->swapgc, dri2_surf->drawable, mask,
         switch (dri2_surf->depth) {
   case 32:
   case 30:
   case 24:
      dri2_surf->bytes_per_pixel = 4;
      case 16:
      dri2_surf->bytes_per_pixel = 2;
      case 8:
      dri2_surf->bytes_per_pixel = 1;
      case 0:
      dri2_surf->bytes_per_pixel = 0;
      default:
            }
      static void
   swrastDestroyDrawable(struct dri2_egl_display *dri2_dpy,
         {
      xcb_free_gc(dri2_dpy->conn, dri2_surf->gc);
      }
      static bool
   x11_get_drawable_info(__DRIdrawable *draw, int *x, int *y, int *w, int *h,
         {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy =
            xcb_get_geometry_cookie_t cookie;
   xcb_get_geometry_reply_t *reply;
   xcb_generic_error_t *error;
            cookie = xcb_get_geometry(dri2_dpy->conn, dri2_surf->drawable);
   reply = xcb_get_geometry_reply(dri2_dpy->conn, cookie, &error);
   if (reply == NULL)
            if (error != NULL) {
      ret = false;
   _eglLog(_EGL_WARNING, "error in xcb_get_geometry");
      } else {
      *x = reply->x;
   *y = reply->y;
   *w = reply->width;
   *h = reply->height;
      }
   free(reply);
      }
      static void
   swrastGetDrawableInfo(__DRIdrawable *draw, int *x, int *y, int *w, int *h,
         {
      *x = *y = *w = *h = 0;
      }
      static void
   swrastPutImage(__DRIdrawable *draw, int op, int x, int y, int w, int h,
         {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy =
         size_t hdr_len = sizeof(xcb_put_image_request_t);
   int stride_b = dri2_surf->bytes_per_pixel * w;
   size_t size = (hdr_len + stride_b * h) >> 2;
            xcb_gcontext_t gc;
   xcb_void_cookie_t cookie;
   switch (op) {
   case __DRI_SWRAST_IMAGE_OP_DRAW:
      gc = dri2_surf->gc;
      case __DRI_SWRAST_IMAGE_OP_SWAP:
      gc = dri2_surf->swapgc;
      default:
                  if (size < max_req_len) {
      cookie = xcb_put_image(
      dri2_dpy->conn, XCB_IMAGE_FORMAT_Z_PIXMAP, dri2_surf->drawable, gc, w,
         } else {
      int num_lines = ((max_req_len << 2) - hdr_len) / stride_b;
   int y_start = 0;
   int y_todo = h;
   while (y_todo) {
      int this_lines = MIN2(num_lines, y_todo);
   cookie =
      xcb_put_image(dri2_dpy->conn, XCB_IMAGE_FORMAT_Z_PIXMAP,
                  xcb_discard_reply(dri2_dpy->conn, cookie.sequence);
   y_start += this_lines;
            }
      static void
   swrastGetImage(__DRIdrawable *read, int x, int y, int w, int h, char *data,
         {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy =
            xcb_get_image_cookie_t cookie;
   xcb_get_image_reply_t *reply;
            cookie = xcb_get_image(dri2_dpy->conn, XCB_IMAGE_FORMAT_Z_PIXMAP,
         reply = xcb_get_image_reply(dri2_dpy->conn, cookie, &error);
   if (reply == NULL)
            if (error != NULL) {
      _eglLog(_EGL_WARNING, "error in xcb_get_image");
      } else {
      uint32_t bytes = xcb_get_image_data_length(reply);
   uint8_t *idata = xcb_get_image_data(reply);
      }
      }
      static xcb_screen_t *
   get_xcb_screen(xcb_screen_iterator_t iter, int screen)
   {
      for (; iter.rem; --screen, xcb_screen_next(&iter))
      if (screen == 0)
            }
      static xcb_visualtype_t *
   get_xcb_visualtype_for_depth(struct dri2_egl_display *dri2_dpy, int depth)
   {
      xcb_visualtype_iterator_t visual_iter;
   xcb_screen_t *screen = dri2_dpy->screen;
            for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
      if (depth_iter.data->depth != depth)
            visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
   if (visual_iter.rem)
                  }
      /* Get red channel mask for given depth. */
   unsigned int
   dri2_x11_get_red_mask_for_depth(struct dri2_egl_display *dri2_dpy, int depth)
   {
               if (visual)
               }
      /**
   * Called via eglCreateWindowSurface(), drv->CreateWindowSurface().
   */
   static _EGLSurface *
   dri2_x11_create_surface(_EGLDisplay *disp, EGLint type, _EGLConfig *conf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct dri2_egl_surface *dri2_surf;
   xcb_get_geometry_cookie_t cookie;
   xcb_get_geometry_reply_t *reply;
   xcb_generic_error_t *error;
            dri2_surf = calloc(1, sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
               if (!dri2_init_surface(&dri2_surf->base, disp, type, conf, attrib_list,
                  dri2_surf->region = XCB_NONE;
   if (type == EGL_PBUFFER_BIT) {
      dri2_surf->drawable = xcb_generate_id(dri2_dpy->conn);
   xcb_create_pixmap(dri2_dpy->conn, conf->BufferSize, dri2_surf->drawable,
            } else {
      STATIC_ASSERT(sizeof(uintptr_t) == sizeof(native_surface));
                        if (!config) {
      _eglError(EGL_BAD_MATCH,
                     if (type != EGL_PBUFFER_BIT) {
      cookie = xcb_get_geometry(dri2_dpy->conn, dri2_surf->drawable);
   reply = xcb_get_geometry_reply(dri2_dpy->conn, cookie, &error);
   if (error != NULL) {
      if (error->error_code == BadAlloc)
         else if (type == EGL_WINDOW_BIT)
         else
         free(error);
   free(reply);
      } else if (reply == NULL) {
      _eglError(EGL_BAD_ALLOC, "xcb_get_geometry");
               dri2_surf->base.Width = reply->width;
   dri2_surf->base.Height = reply->height;
   dri2_surf->depth = reply->depth;
               if (!dri2_create_drawable(dri2_dpy, config, dri2_surf, dri2_surf))
            if (dri2_dpy->dri2) {
      xcb_void_cookie_t cookie;
            cookie =
         error = xcb_request_check(dri2_dpy->conn, cookie);
   conn_error = xcb_connection_has_error(dri2_dpy->conn);
   if (conn_error || error != NULL) {
      if (type == EGL_PBUFFER_BIT || conn_error ||
      error->error_code == BadAlloc)
      else if (type == EGL_WINDOW_BIT)
      _eglError(EGL_BAD_NATIVE_WINDOW,
      else
      _eglError(EGL_BAD_NATIVE_PIXMAP,
      free(error);
         } else {
      if (type == EGL_PBUFFER_BIT) {
         }
               /* we always copy the back buffer to front */
                  cleanup_dri_drawable:
         cleanup_pixmap:
      if (type == EGL_PBUFFER_BIT)
      cleanup_surf:
                  }
      /**
   * Called via eglCreateWindowSurface(), drv->CreateWindowSurface().
   */
   static _EGLSurface *
   dri2_x11_create_window_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            surf = dri2_x11_create_surface(disp, EGL_WINDOW_BIT, conf, native_window,
         if (surf != NULL) {
      /* When we first create the DRI2 drawable, its swap interval on the
   * server side is 1.
   */
            /* Override that with a driconf-set value. */
                  }
      static _EGLSurface *
   dri2_x11_create_pixmap_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      return dri2_x11_create_surface(disp, EGL_PIXMAP_BIT, conf, native_pixmap,
      }
      static _EGLSurface *
   dri2_x11_create_pbuffer_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      return dri2_x11_create_surface(disp, EGL_PBUFFER_BIT, conf, NULL,
      }
      static EGLBoolean
   dri2_x11_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
                     if (dri2_dpy->dri2) {
         } else {
      assert(dri2_dpy->swrast);
               if (surf->Type == EGL_PBUFFER_BIT)
            dri2_fini_surface(surf);
               }
      /**
   * Function utilizes swrastGetDrawableInfo to get surface
   * geometry from x server and calls default query surface
   * implementation that returns the updated values.
   *
   * In case of errors we still return values that we currently
   * have.
   */
   static EGLBoolean
   dri2_query_surface(_EGLDisplay *disp, _EGLSurface *surf, EGLint attribute,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
                     switch (attribute) {
   case EGL_WIDTH:
   case EGL_HEIGHT:
      if (x11_get_drawable_info(drawable, &x, &y, &w, &h, dri2_surf)) {
      bool changed = surf->Width != w || surf->Height != h;
   surf->Width = w;
   surf->Height = h;
   if (changed && dri2_dpy->flush)
      }
      default:
         }
      }
      /**
   * Process list of buffer received from the server
   *
   * Processes the list of buffers received in a reply from the server to either
   * \c DRI2GetBuffers or \c DRI2GetBuffersWithFormat.
   */
   static void
   dri2_x11_process_buffers(struct dri2_egl_surface *dri2_surf,
         {
      struct dri2_egl_display *dri2_dpy =
                           /* This assumes the DRI2 buffer attachment tokens matches the
   * __DRIbuffer tokens. */
   for (unsigned i = 0; i < count; i++) {
      dri2_surf->buffers[i].attachment = buffers[i].attachment;
   dri2_surf->buffers[i].name = buffers[i].name;
   dri2_surf->buffers[i].pitch = buffers[i].pitch;
   dri2_surf->buffers[i].cpp = buffers[i].cpp;
            /* We only use the DRI drivers single buffer configs.  This
   * means that if we try to render to a window, DRI2 will give us
   * the fake front buffer, which we'll use as a back buffer.
   * Note that EGL doesn't require that several clients rendering
   * to the same window must see the same aux buffers. */
   if (dri2_surf->buffers[i].attachment == __DRI_BUFFER_FAKE_FRONT_LEFT)
               if (dri2_surf->region != XCB_NONE)
            rectangle.x = 0;
   rectangle.y = 0;
   rectangle.width = dri2_surf->base.Width;
   rectangle.height = dri2_surf->base.Height;
   dri2_surf->region = xcb_generate_id(dri2_dpy->conn);
      }
      static __DRIbuffer *
   dri2_x11_get_buffers(__DRIdrawable *driDrawable, int *width, int *height,
               {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy =
         xcb_dri2_dri2_buffer_t *buffers;
   xcb_dri2_get_buffers_reply_t *reply;
                     cookie = xcb_dri2_get_buffers_unchecked(dri2_dpy->conn, dri2_surf->drawable,
         reply = xcb_dri2_get_buffers_reply(dri2_dpy->conn, cookie, NULL);
   if (reply == NULL)
         buffers = xcb_dri2_get_buffers_buffers(reply);
   if (buffers == NULL) {
      free(reply);
               *out_count = reply->count;
   dri2_surf->base.Width = *width = reply->width;
   dri2_surf->base.Height = *height = reply->height;
                        }
      static __DRIbuffer *
   dri2_x11_get_buffers_with_format(__DRIdrawable *driDrawable, int *width,
               {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct dri2_egl_display *dri2_dpy =
         xcb_dri2_dri2_buffer_t *buffers;
   xcb_dri2_get_buffers_with_format_reply_t *reply;
   xcb_dri2_get_buffers_with_format_cookie_t cookie;
                     format_attachments = (xcb_dri2_attach_format_t *)attachments;
   cookie = xcb_dri2_get_buffers_with_format_unchecked(
            reply = xcb_dri2_get_buffers_with_format_reply(dri2_dpy->conn, cookie, NULL);
   if (reply == NULL)
            buffers = xcb_dri2_get_buffers_with_format_buffers(reply);
   dri2_surf->base.Width = *width = reply->width;
   dri2_surf->base.Height = *height = reply->height;
   *out_count = reply->count;
                        }
      static void
   dri2_x11_flush_front_buffer(__DRIdrawable *driDrawable, void *loaderPrivate)
   {
                     #if 0
                  #else
         #endif
   }
      static int
   dri2_x11_do_authenticate(struct dri2_egl_display *dri2_dpy, uint32_t id)
   {
      xcb_dri2_authenticate_reply_t *authenticate;
   xcb_dri2_authenticate_cookie_t authenticate_cookie;
            authenticate_cookie = xcb_dri2_authenticate_unchecked(
         authenticate =
            if (authenticate == NULL || !authenticate->authenticated)
                        }
      static EGLBoolean
   dri2_x11_local_authenticate(struct dri2_egl_display *dri2_dpy)
   {
   #ifdef HAVE_LIBDRM
               if (drmGetMagic(dri2_dpy->fd_render_gpu, &magic)) {
      _eglLog(_EGL_WARNING, "DRI2: failed to get drm magic");
               if (dri2_x11_do_authenticate(dri2_dpy, magic) < 0) {
      _eglLog(_EGL_WARNING, "DRI2: failed to authenticate");
         #endif
         }
      static EGLBoolean
   dri2_x11_connect(struct dri2_egl_display *dri2_dpy)
   {
      xcb_xfixes_query_version_reply_t *xfixes_query;
   xcb_xfixes_query_version_cookie_t xfixes_query_cookie;
   xcb_dri2_query_version_reply_t *dri2_query;
   xcb_dri2_query_version_cookie_t dri2_query_cookie;
   xcb_dri2_connect_reply_t *connect;
   xcb_dri2_connect_cookie_t connect_cookie;
   xcb_generic_error_t *error;
   char *driver_name, *loader_driver_name, *device_name;
            xcb_prefetch_extension_data(dri2_dpy->conn, &xcb_xfixes_id);
            extension = xcb_get_extension_data(dri2_dpy->conn, &xcb_xfixes_id);
   if (!(extension && extension->present))
            extension = xcb_get_extension_data(dri2_dpy->conn, &xcb_dri2_id);
   if (!(extension && extension->present))
            xfixes_query_cookie = xcb_xfixes_query_version(
            dri2_query_cookie = xcb_dri2_query_version(
            connect_cookie = xcb_dri2_connect_unchecked(
            xfixes_query = xcb_xfixes_query_version_reply(dri2_dpy->conn,
         if (xfixes_query == NULL || error != NULL ||
      xfixes_query->major_version < 2) {
   _eglLog(_EGL_WARNING, "DRI2: failed to query xfixes version");
   free(error);
   free(xfixes_query);
      }
            dri2_query =
         if (dri2_query == NULL || error != NULL) {
      _eglLog(_EGL_WARNING, "DRI2: failed to query version");
   free(error);
   free(dri2_query);
      }
   dri2_dpy->dri2_major = dri2_query->major_version;
   dri2_dpy->dri2_minor = dri2_query->minor_version;
            connect = xcb_dri2_connect_reply(dri2_dpy->conn, connect_cookie, NULL);
   if (connect == NULL ||
      connect->driver_name_length + connect->device_name_length == 0) {
   _eglLog(_EGL_WARNING, "DRI2: failed to authenticate");
   free(connect);
                        dri2_dpy->fd_render_gpu = loader_open_device(device_name);
   if (dri2_dpy->fd_render_gpu == -1) {
      _eglLog(_EGL_WARNING, "DRI2: could not open %s (%s)", device_name,
         free(connect);
               if (!dri2_x11_local_authenticate(dri2_dpy)) {
      close(dri2_dpy->fd_render_gpu);
   free(connect);
                        /* If Mesa knows about the appropriate driver for this fd, then trust it.
   * Otherwise, default to the server's value.
   */
   loader_driver_name = loader_get_driver_for_fd(dri2_dpy->fd_render_gpu);
   if (loader_driver_name) {
         } else {
      dri2_dpy->driver_name =
               if (dri2_dpy->driver_name == NULL) {
      close(dri2_dpy->fd_render_gpu);
   free(connect);
            #ifdef HAVE_WAYLAND_PLATFORM
      dri2_dpy->device_name =
      #endif
                     }
      static int
   dri2_x11_authenticate(_EGLDisplay *disp, uint32_t id)
   {
                  }
      static EGLBoolean
   dri2_x11_add_configs_for_visuals(struct dri2_egl_display *dri2_dpy,
         {
      xcb_depth_iterator_t d;
   xcb_visualtype_t *visuals;
   int config_count = 0;
                              if (supports_preserved)
            while (d.rem > 0) {
      EGLBoolean class_added[6] = {
                           for (int i = 0; i < xcb_depth_visuals_length(d.data); i++) {
                              for (int j = 0; dri2_dpy->driver_configs[j]; j++) {
                     const EGLint config_attrs[] = {
      EGL_NATIVE_VISUAL_ID,
   visuals[i].visual_id,
   EGL_NATIVE_VISUAL_TYPE,
                     int rgba_shifts[4] = {
      ffs(visuals[i].red_mask) - 1,
   ffs(visuals[i].green_mask) - 1,
                     unsigned int rgba_sizes[4] = {
      util_bitcount(visuals[i].red_mask),
   util_bitcount(visuals[i].green_mask),
                     dri2_conf =
      dri2_add_config(disp, config, config_count + 1, surface_type,
      if (dri2_conf)
                  /* Allow a 24-bit RGB visual to match a 32-bit RGBA EGLConfig.
   * Ditto for 30-bit RGB visuals to match a 32-bit RGBA EGLConfig.
   * Otherwise it will only match a 32-bit RGBA visual.  On a
   * composited window manager on X11, this will make all of the
   * EGLConfigs with destination alpha get blended by the
   * compositor.  This is probably not what the application
   * wants... especially on drivers that only have 32-bit RGBA
   * EGLConfigs! */
   if (d.data->depth == 24 || d.data->depth == 30) {
      unsigned int rgba_mask =
      ~(visuals[i].red_mask | visuals[i].green_mask |
      rgba_shifts[3] = ffs(rgba_mask) - 1;
   rgba_sizes[3] = util_bitcount(rgba_mask);
   dri2_conf =
      dri2_add_config(disp, config, config_count + 1, surface_type,
      if (dri2_conf)
      if (dri2_conf->base.ConfigID == config_count + 1)
                              if (!config_count) {
      _eglLog(_EGL_WARNING, "DRI2: failed to create any config");
                  }
      static EGLBoolean
   dri2_copy_region(_EGLDisplay *disp, _EGLSurface *draw,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   enum xcb_dri2_attachment_t render_attachment;
            /* No-op for a pixmap or pbuffer surface */
   if (draw->Type == EGL_PIXMAP_BIT || draw->Type == EGL_PBUFFER_BIT)
            assert(!dri2_dpy->kopper);
            if (dri2_surf->have_fake_front)
         else
            cookie = xcb_dri2_copy_region_unchecked(
      dri2_dpy->conn, dri2_surf->drawable, region,
                  }
      static int64_t
   dri2_x11_swap_buffers_msc(_EGLDisplay *disp, _EGLSurface *draw, int64_t msc,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   uint32_t msc_hi = msc >> 32;
   uint32_t msc_lo = msc & 0xffffffff;
   uint32_t divisor_hi = divisor >> 32;
   uint32_t divisor_lo = divisor & 0xffffffff;
   uint32_t remainder_hi = remainder >> 32;
   uint32_t remainder_lo = remainder & 0xffffffff;
   xcb_dri2_swap_buffers_cookie_t cookie;
   xcb_dri2_swap_buffers_reply_t *reply;
            if (draw->SwapBehavior == EGL_BUFFER_PRESERVED ||
      !dri2_dpy->swap_available) {
      } else {
               cookie = xcb_dri2_swap_buffers_unchecked(
                           if (reply) {
      swap_count = combine_u32_into_u64(reply->swap_hi, reply->swap_lo);
                  /* Since we aren't watching for the server's invalidate events like we're
   * supposed to (due to XCB providing no mechanism for filtering the events
   * the way xlib does), and SwapBuffers is a common cause of invalidate
   * events, just shove one down to the driver, even though we haven't told
   * the driver that we're the kind of loader that provides reliable
   * invalidate events.  This causes the driver to request buffers again at
   * its next draw, so that we get the correct buffers if a pageflip
   * happened.  The driver should still be using the viewport hack to catch
   * window resizes.
   */
   if (dri2_dpy->flush->base.version >= 3 && dri2_dpy->flush->invalidate)
               }
      static EGLBoolean
   dri2_x11_swap_buffers(_EGLDisplay *disp, _EGLSurface *draw)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (dri2_dpy->kopper) {
      /* From the EGL 1.4 spec (page 52):
   *
   *     "The contents of ancillary buffers are always undefined
   *      after calling eglSwapBuffers."
   */
   dri2_dpy->kopper->swapBuffers(dri2_surf->dri_drawable,
            } else if (!dri2_dpy->flush) {
      /* aka the swrast path, which does the swap in the gallium driver. */
   dri2_dpy->core->swapBuffers(dri2_surf->dri_drawable);
               if (dri2_x11_swap_buffers_msc(disp, draw, 0, 0, 0) == -1) {
      /* Swap failed with a window drawable. */
      }
      }
      static EGLBoolean
   dri2_x11_swap_buffers_region(_EGLDisplay *disp, _EGLSurface *draw,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   EGLBoolean ret;
   xcb_xfixes_region_t region;
            if (numRects > (int)ARRAY_SIZE(rectangles))
            for (int i = 0; i < numRects; i++) {
      rectangles[i].x = rects[i * 4];
   rectangles[i].y =
         rectangles[i].width = rects[i * 4 + 2];
               region = xcb_generate_id(dri2_dpy->conn);
   xcb_xfixes_create_region(dri2_dpy->conn, region, numRects, rectangles);
   ret = dri2_copy_region(disp, draw, region);
               }
      static EGLBoolean
   dri2_x11_post_sub_buffer(_EGLDisplay *disp, _EGLSurface *draw, EGLint x,
         {
               if (x < 0 || y < 0 || width < 0 || height < 0)
               }
      static EGLBoolean
   dri2_x11_swap_interval(_EGLDisplay *disp, _EGLSurface *surf, EGLint interval)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (dri2_dpy->kopper) {
      dri2_dpy->kopper->setSwapInterval(dri2_surf->dri_drawable, interval);
               if (dri2_dpy->swap_available)
               }
      static EGLBoolean
   dri2_x11_copy_buffers(_EGLDisplay *disp, _EGLSurface *surf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   xcb_gcontext_t gc;
            STATIC_ASSERT(sizeof(uintptr_t) == sizeof(native_pixmap_target));
            if (dri2_dpy->flush)
         else {
      /* This should not be a swapBuffers, because it could present an
   * incomplete frame, and it could invalidate the back buffer if it's not
   * preserved.  We really do want to flush.  But it ends up working out
   * okay-ish on swrast because those aren't invalidating the back buffer on
   * swap.
   */
               gc = xcb_generate_id(dri2_dpy->conn);
   xcb_create_gc(dri2_dpy->conn, gc, target, 0, NULL);
   xcb_copy_area(dri2_dpy->conn, dri2_surf->drawable, target, gc, 0, 0, 0, 0,
                     }
      uint32_t
   dri2_format_for_depth(struct dri2_egl_display *dri2_dpy, uint32_t depth)
   {
      switch (depth) {
   case 16:
         case 24:
         case 30:
      /* Different preferred formats for different hw */
   if (dri2_x11_get_red_mask_for_depth(dri2_dpy, 30) == 0x3ff)
         else
      case 32:
         default:
            }
      static _EGLImage *
   dri2_create_image_khr_pixmap(_EGLDisplay *disp, _EGLContext *ctx,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img;
   unsigned int attachments[1];
   xcb_drawable_t drawable;
   xcb_dri2_get_buffers_cookie_t buffers_cookie;
   xcb_dri2_get_buffers_reply_t *buffers_reply;
   xcb_dri2_dri2_buffer_t *buffers;
   xcb_get_geometry_cookie_t geometry_cookie;
   xcb_get_geometry_reply_t *geometry_reply;
   xcb_generic_error_t *error;
                     drawable = (xcb_drawable_t)(uintptr_t)buffer;
   xcb_dri2_create_drawable(dri2_dpy->conn, drawable);
   attachments[0] = XCB_DRI2_ATTACHMENT_BUFFER_FRONT_LEFT;
   buffers_cookie = xcb_dri2_get_buffers_unchecked(dri2_dpy->conn, drawable, 1,
         geometry_cookie = xcb_get_geometry(dri2_dpy->conn, drawable);
   buffers_reply =
         if (buffers_reply == NULL)
            buffers = xcb_dri2_get_buffers_buffers(buffers_reply);
   if (buffers == NULL) {
      free(buffers_reply);
               geometry_reply =
         if (geometry_reply == NULL || error != NULL) {
      _eglError(EGL_BAD_ALLOC, "xcb_get_geometry");
   free(error);
   free(buffers_reply);
   free(geometry_reply);
               format = dri2_format_for_depth(dri2_dpy, geometry_reply->depth);
   if (format == __DRI_IMAGE_FORMAT_NONE) {
      _eglError(EGL_BAD_PARAMETER,
         free(buffers_reply);
   free(geometry_reply);
               dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      free(buffers_reply);
   free(geometry_reply);
   _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr");
                        stride = buffers[0].pitch / buffers[0].cpp;
   dri2_img->dri_image = dri2_dpy->image->createImageFromName(
      dri2_dpy->dri_screen_render_gpu, buffers_reply->width,
         free(buffers_reply);
               }
      static _EGLImage *
   dri2_x11_create_image_khr(_EGLDisplay *disp, _EGLContext *ctx, EGLenum target,
         {
      switch (target) {
   case EGL_NATIVE_PIXMAP_KHR:
         default:
            }
      static EGLBoolean
   dri2_x11_get_sync_values(_EGLDisplay *display, _EGLSurface *surface,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(display);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surface);
   xcb_dri2_get_msc_cookie_t cookie;
            cookie = xcb_dri2_get_msc(dri2_dpy->conn, dri2_surf->drawable);
            if (!reply)
            *ust = ((EGLuint64KHR)reply->ust_hi << 32) | reply->ust_lo;
   *msc = ((EGLuint64KHR)reply->msc_hi << 32) | reply->msc_lo;
   *sbc = ((EGLuint64KHR)reply->sbc_hi << 32) | reply->sbc_lo;
               }
      static int
   box_intersection_area(int16_t a_x, int16_t a_y, int16_t a_width,
               {
      int w = MIN2(a_x + a_width, b_x + b_width) - MAX2(a_x, b_x);
               }
      EGLBoolean
   dri2_x11_get_msc_rate(_EGLDisplay *display, _EGLSurface *surface,
         {
                        if (dri2_dpy->screen_resources.num_crtcs == 0) {
      /* If there's no CRTC active, use the present fake vblank of 1Hz */
   *numerator = 1;
   *denominator = 1;
               /* Default to the first CRTC in the list */
   *numerator = dri2_dpy->screen_resources.crtcs[0].refresh_numerator;
            /* If there's only one active CRTC, we're done */
   if (dri2_dpy->screen_resources.num_crtcs == 1)
            /* In a multi-monitor setup, look at each CRTC and perform a box
   * intersection between the CRTC and surface.  Use the CRTC whose
   * box intersection has the largest area.
   */
   if (surface->Type != EGL_WINDOW_BIT)
                     xcb_translate_coordinates_cookie_t cookie =
      xcb_translate_coordinates_unchecked(dri2_dpy->conn, window,
      xcb_translate_coordinates_reply_t *reply =
            if (!reply) {
      _eglError(EGL_BAD_SURFACE,
                              for (unsigned c = 0; c < dri2_dpy->screen_resources.num_crtcs; c++) {
               int c_area = box_intersection_area(
      reply->dst_x, reply->dst_y, surface->Width, surface->Height, crtc->x,
      if (c_area > area) {
      *numerator = crtc->refresh_numerator;
   *denominator = crtc->refresh_denominator;
                  /* If the window is entirely off-screen, then area will still be 0.
   * We defaulted to the first CRTC in the list's refresh rate, earlier.
               }
      static EGLBoolean
   dri2_kopper_swap_interval(_EGLDisplay *disp, _EGLSurface *surf, EGLint interval)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            /* This can legitimately be null for lavapipe */
   if (dri2_dpy->kopper)
               }
      static _EGLSurface *
   dri2_kopper_create_window_surface(_EGLDisplay *disp, _EGLConfig *conf,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            surf = dri2_x11_create_surface(disp, EGL_WINDOW_BIT, conf, native_window,
         if (surf != NULL) {
      /* When we first create the DRI2 drawable, its swap interval on the
   * server side is 1.
   */
            /* Override that with a driconf-set value. */
                  }
      static EGLint
   dri2_kopper_query_buffer_age(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            /* This can legitimately be null for lavapipe */
   if (dri2_dpy->kopper)
               }
      static const struct dri2_egl_display_vtbl dri2_x11_swrast_display_vtbl = {
      .authenticate = NULL,
   .create_window_surface = dri2_x11_create_window_surface,
   .create_pixmap_surface = dri2_x11_create_pixmap_surface,
   .create_pbuffer_surface = dri2_x11_create_pbuffer_surface,
   .destroy_surface = dri2_x11_destroy_surface,
   .create_image = dri2_create_image_khr,
   .swap_buffers = dri2_x11_swap_buffers,
   .swap_buffers_region = dri2_x11_swap_buffers_region,
   .post_sub_buffer = dri2_x11_post_sub_buffer,
   .copy_buffers = dri2_x11_copy_buffers,
   /* XXX: should really implement this since X11 has pixmaps */
   .query_surface = dri2_query_surface,
   .get_msc_rate = dri2_x11_get_msc_rate,
      };
      static const struct dri2_egl_display_vtbl dri2_x11_kopper_display_vtbl = {
      .authenticate = NULL,
   .create_window_surface = dri2_kopper_create_window_surface,
   .create_pixmap_surface = dri2_x11_create_pixmap_surface,
   .create_pbuffer_surface = dri2_x11_create_pbuffer_surface,
   .destroy_surface = dri2_x11_destroy_surface,
   .create_image = dri2_create_image_khr,
   .swap_interval = dri2_kopper_swap_interval,
   .swap_buffers = dri2_x11_swap_buffers,
   .swap_buffers_region = dri2_x11_swap_buffers_region,
   .post_sub_buffer = dri2_x11_post_sub_buffer,
   .copy_buffers = dri2_x11_copy_buffers,
   .query_buffer_age = dri2_kopper_query_buffer_age,
   /* XXX: should really implement this since X11 has pixmaps */
   .query_surface = dri2_query_surface,
   .get_msc_rate = dri2_x11_get_msc_rate,
      };
      static const struct dri2_egl_display_vtbl dri2_x11_display_vtbl = {
      .authenticate = dri2_x11_authenticate,
   .create_window_surface = dri2_x11_create_window_surface,
   .create_pixmap_surface = dri2_x11_create_pixmap_surface,
   .create_pbuffer_surface = dri2_x11_create_pbuffer_surface,
   .destroy_surface = dri2_x11_destroy_surface,
   .create_image = dri2_x11_create_image_khr,
   .swap_interval = dri2_x11_swap_interval,
   .swap_buffers = dri2_x11_swap_buffers,
   .swap_buffers_region = dri2_x11_swap_buffers_region,
   .post_sub_buffer = dri2_x11_post_sub_buffer,
   .copy_buffers = dri2_x11_copy_buffers,
   .query_surface = dri2_query_surface,
   .get_sync_values = dri2_x11_get_sync_values,
   .get_msc_rate = dri2_x11_get_msc_rate,
      };
      static const __DRIswrastLoaderExtension swrast_loader_extension = {
               .getDrawableInfo = swrastGetDrawableInfo,
   .putImage = swrastPutImage,
      };
      static_assert(sizeof(struct kopper_vk_surface_create_storage) >=
                  static void
   kopperSetSurfaceCreateInfo(void *_draw, struct kopper_loader_info *ci)
   {
      struct dri2_egl_surface *dri2_surf = _draw;
   struct dri2_egl_display *dri2_dpy =
                  if (dri2_surf->base.Type != EGL_WINDOW_BIT)
         xcb->sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
   xcb->pNext = NULL;
   xcb->flags = 0;
   xcb->connection = dri2_dpy->conn;
   xcb->window = dri2_surf->drawable;
      }
      static const __DRIkopperLoaderExtension kopper_loader_extension = {
                  };
      static const __DRIextension *swrast_loader_extensions[] = {
      &swrast_loader_extension.base,
   &image_lookup_extension.base,
   &kopper_loader_extension.base,
      };
      static int
   dri2_find_screen_for_display(const _EGLDisplay *disp, int fallback_screen)
   {
               if (!disp->Options.Attribs)
            for (attr = disp->Options.Attribs; attr[0] != EGL_NONE; attr += 2) {
      if (attr[0] == EGL_PLATFORM_X11_SCREEN_EXT ||
      attr[0] == EGL_PLATFORM_XCB_SCREEN_EXT)
               }
      static EGLBoolean
   dri2_get_xcb_connection(_EGLDisplay *disp, struct dri2_egl_display *dri2_dpy)
   {
      xcb_screen_iterator_t s;
   int screen;
            disp->DriverData = (void *)dri2_dpy;
   if (disp->PlatformDisplay == NULL) {
      dri2_dpy->conn = xcb_connect(NULL, &screen);
   dri2_dpy->own_device = true;
      } else if (disp->Platform == _EGL_PLATFORM_X11) {
      Display *dpy = disp->PlatformDisplay;
   dri2_dpy->conn = XGetXCBConnection(dpy);
      } else {
      /*   _EGL_PLATFORM_XCB   */
   dri2_dpy->conn = disp->PlatformDisplay;
               if (!dri2_dpy->conn || xcb_connection_has_error(dri2_dpy->conn)) {
      msg = "xcb_connect failed";
               s = xcb_setup_roots_iterator(xcb_get_setup(dri2_dpy->conn));
   dri2_dpy->screen = get_xcb_screen(s, screen);
   if (!dri2_dpy->screen) {
      msg = "failed to get xcb screen";
                  disconnect:
      if (disp->PlatformDisplay == NULL)
               }
      static void
   dri2_x11_setup_swap_interval(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            /* default behavior for no SwapBuffers support: no vblank syncing
   * either.
   */
   dri2_dpy->min_swap_interval = 0;
   dri2_dpy->max_swap_interval = 0;
            if (!dri2_dpy->swap_available)
            /* If we do have swapbuffers, then we can support pretty much any swap
   * interval. Unless we're kopper, for now.
   */
   if (dri2_dpy->kopper)
               }
      static EGLBoolean
   dri2_initialize_x11_swrast(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_display_create();
   if (!dri2_dpy)
            if (!dri2_get_xcb_connection(disp, dri2_dpy))
            /*
   * Every hardware driver_name is set using strdup. Doing the same in
   * here will allow is to simply free the memory at dri2_terminate().
   */
   dri2_dpy->driver_name = strdup(disp->Options.Zink ? "zink" : "swrast");
   if (disp->Options.Zink &&
      !debug_get_bool_option("LIBGL_DRI3_DISABLE", false))
      if (!dri2_load_driver_swrast(disp))
                     if (!dri2_create_screen(disp))
            if (!dri2_setup_extensions(disp))
            if (!dri2_setup_device(disp, true)) {
      _eglError(EGL_NOT_INITIALIZED, "DRI2: failed to setup EGLDevice");
                        if (disp->Options.Zink) {
      #ifdef HAVE_WAYLAND_PLATFORM
         #endif
         dri2_dpy->swap_available = EGL_TRUE;
   dri2_x11_setup_swap_interval(disp);
   if (dri2_dpy->fd_render_gpu == dri2_dpy->fd_display_gpu)
         disp->Extensions.NOK_texture_from_pixmap = EGL_TRUE;
   disp->Extensions.CHROMIUM_sync_control = EGL_TRUE;
   disp->Extensions.ANGLE_sync_control_rate = EGL_TRUE;
   disp->Extensions.EXT_buffer_age = EGL_TRUE;
            if (dri2_dpy->multibuffers_available)
      } else {
      /* swrast */
               if (!dri2_x11_add_configs_for_visuals(dri2_dpy, disp, !disp->Options.Zink))
            /* Fill vtbl last to prevent accidentally calling virtual function during
   * initialization.
   */
   if (disp->Options.Zink)
         else
                  cleanup:
      dri2_display_destroy(disp);
      }
      #ifdef HAVE_DRI3
      static const __DRIextension *dri3_image_loader_extensions[] = {
      &dri3_image_loader_extension.base,
   &image_lookup_extension.base,
   &use_invalidate.base,
   &background_callable_extension.base,
      };
      static EGLBoolean
   dri2_initialize_x11_dri3(_EGLDisplay *disp)
   {
               if (!dri2_dpy)
            if (!dri2_get_xcb_connection(disp, dri2_dpy))
            if (!dri3_x11_connect(dri2_dpy))
            if (!dri2_load_driver_dri3(disp))
                     dri2_dpy->swap_available = true;
            if (!dri2_create_screen(disp))
            if (!dri2_setup_extensions(disp))
            if (!dri2_setup_device(disp, false)) {
      _eglError(EGL_NOT_INITIALIZED, "DRI2: failed to setup EGLDevice");
                                 if (dri2_dpy->fd_render_gpu == dri2_dpy->fd_display_gpu)
         disp->Extensions.NOK_texture_from_pixmap = EGL_TRUE;
   disp->Extensions.CHROMIUM_sync_control = EGL_TRUE;
   disp->Extensions.ANGLE_sync_control_rate = EGL_TRUE;
   disp->Extensions.EXT_buffer_age = EGL_TRUE;
                     if (!dri2_x11_add_configs_for_visuals(dri2_dpy, disp, false))
            loader_init_screen_resources(&dri2_dpy->screen_resources, dri2_dpy->conn,
            dri2_dpy->loader_dri3_ext.core = dri2_dpy->core;
   dri2_dpy->loader_dri3_ext.image_driver = dri2_dpy->image_driver;
   dri2_dpy->loader_dri3_ext.flush = dri2_dpy->flush;
   dri2_dpy->loader_dri3_ext.tex_buffer = dri2_dpy->tex_buffer;
   dri2_dpy->loader_dri3_ext.image = dri2_dpy->image;
            /* Fill vtbl last to prevent accidentally calling virtual function during
   * initialization.
   */
                           cleanup:
      dri2_display_destroy(disp);
      }
   #endif
      static const __DRIdri2LoaderExtension dri2_loader_extension_old = {
               .getBuffers = dri2_x11_get_buffers,
   .flushFrontBuffer = dri2_x11_flush_front_buffer,
      };
      static const __DRIdri2LoaderExtension dri2_loader_extension = {
               .getBuffers = dri2_x11_get_buffers,
   .flushFrontBuffer = dri2_x11_flush_front_buffer,
      };
      static const __DRIextension *dri2_loader_extensions_old[] = {
      &dri2_loader_extension_old.base,
   &image_lookup_extension.base,
   &background_callable_extension.base,
      };
      static const __DRIextension *dri2_loader_extensions[] = {
      &dri2_loader_extension.base,
   &image_lookup_extension.base,
   &use_invalidate.base,
   &background_callable_extension.base,
      };
      static EGLBoolean
   dri2_initialize_x11_dri2(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_display_create();
   if (!dri2_dpy)
            if (!dri2_get_xcb_connection(disp, dri2_dpy))
            if (!dri2_x11_connect(dri2_dpy))
            if (!dri2_load_driver(disp))
            if (dri2_dpy->dri2_minor >= 1)
         else
            dri2_dpy->swap_available = (dri2_dpy->dri2_minor >= 2);
            if (!dri2_create_screen(disp))
            if (!dri2_setup_extensions(disp))
            if (!dri2_setup_device(disp, false)) {
      _eglError(EGL_NOT_INITIALIZED, "DRI2: failed to setup EGLDevice");
                                 disp->Extensions.KHR_image_pixmap = EGL_TRUE;
   disp->Extensions.NOK_swap_region = EGL_TRUE;
   disp->Extensions.NOK_texture_from_pixmap = EGL_TRUE;
   disp->Extensions.NV_post_sub_buffer = EGL_TRUE;
   disp->Extensions.CHROMIUM_sync_control = EGL_TRUE;
                     if (!dri2_x11_add_configs_for_visuals(dri2_dpy, disp, true))
            /* Fill vtbl last to prevent accidentally calling virtual function during
   * initialization.
   */
                           cleanup:
      dri2_display_destroy(disp);
      }
      EGLBoolean
   dri2_initialize_x11(_EGLDisplay *disp)
   {
      if (disp->Options.ForceSoftware || disp->Options.Zink)
         #ifdef HAVE_DRI3
      if (!debug_get_bool_option("LIBGL_DRI3_DISABLE", false))
      if (dri2_initialize_x11_dri3(disp))
   #endif
         if (!debug_get_bool_option("LIBGL_DRI2_DISABLE", false))
      if (dri2_initialize_x11_dri2(disp))
            }
      void
   dri2_teardown_x11(struct dri2_egl_display *dri2_dpy)
   {
      if (dri2_dpy->dri2_major >= 3)
            if (dri2_dpy->own_device)
      }
