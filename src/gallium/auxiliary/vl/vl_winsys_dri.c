   /**************************************************************************
   *
   * Copyright 2009 Younes Manton.
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
      /* directly referenced from target Makefile, because of X dependencies */
      #include <sys/types.h>
   #include <sys/stat.h>
      #include <X11/Xlib-xcb.h>
   #include <X11/extensions/dri2tokens.h>
   #include <xcb/dri2.h>
   #include <xf86drm.h>
   #include <errno.h>
      #include "loader.h"
      #include "pipe/p_screen.h"
   #include "pipe/p_context.h"
   #include "pipe/p_state.h"
   #include "pipe-loader/pipe_loader.h"
   #include "frontend/drm_driver.h"
      #include "util/u_memory.h"
   #include "util/crc32.h"
   #include "util/u_hash_table.h"
   #include "util/u_inlines.h"
      #include "vl/vl_compositor.h"
   #include "vl/vl_winsys.h"
      #include "drm-uapi/drm_fourcc.h"
      struct vl_dri_screen
   {
      struct vl_screen base;
   xcb_connection_t *conn;
                     bool current_buffer;
   uint32_t buffer_names[2];
            bool flushed;
   xcb_dri2_swap_buffers_cookie_t swap_cookie;
   xcb_dri2_wait_sbc_cookie_t wait_cookie;
               };
      static const unsigned attachments[1] = { XCB_DRI2_ATTACHMENT_BUFFER_BACK_LEFT };
      static void vl_dri2_screen_destroy(struct vl_screen *vscreen);
      static void
   vl_dri2_handle_stamps(struct vl_dri_screen *scrn,
               {
      int64_t ust = ((((uint64_t)ust_hi) << 32) | ust_lo) * 1000;
            if (scrn->last_ust && (ust > scrn->last_ust) &&
      scrn->last_msc && (msc > scrn->last_msc))
         scrn->last_ust = ust;
      }
      static xcb_dri2_get_buffers_reply_t *
   vl_dri2_get_flush_reply(struct vl_dri_screen *scrn)
   {
                        if (!scrn->flushed)
                              wait_sbc_reply = xcb_dri2_wait_sbc_reply(scrn->conn, scrn->wait_cookie, NULL);
   if (!wait_sbc_reply)
         vl_dri2_handle_stamps(scrn, wait_sbc_reply->ust_hi, wait_sbc_reply->ust_lo,
                     }
      static void
   vl_dri2_flush_frontbuffer(struct pipe_screen *screen,
                           {
      struct vl_dri_screen *scrn = (struct vl_dri_screen *)context_private;
            assert(screen);
   assert(resource);
                     msc_hi = scrn->next_msc >> 32;
            scrn->swap_cookie = xcb_dri2_swap_buffers_unchecked(scrn->conn, scrn->drawable,
         scrn->wait_cookie = xcb_dri2_wait_sbc_unchecked(scrn->conn, scrn->drawable, 0, 0);
   scrn->buffers_cookie = xcb_dri2_get_buffers_unchecked(scrn->conn, scrn->drawable,
            scrn->flushed = true;
      }
      static void
   vl_dri2_destroy_drawable(struct vl_dri_screen *scrn)
   {
      xcb_void_cookie_t destroy_cookie;
   if (scrn->drawable) {
      free(vl_dri2_get_flush_reply(scrn));
   destroy_cookie = xcb_dri2_destroy_drawable_checked(scrn->conn, scrn->drawable);
   /* ignore any error here, since the drawable can be destroyed long ago */
         }
      static void
   vl_dri2_set_drawable(struct vl_dri_screen *scrn, Drawable drawable)
   {
      assert(scrn);
            if (scrn->drawable == drawable)
                     xcb_dri2_create_drawable(scrn->conn, drawable);
   scrn->current_buffer = false;
   vl_compositor_reset_dirty_area(&scrn->dirty_areas[0]);
   vl_compositor_reset_dirty_area(&scrn->dirty_areas[1]);
      }
      static struct pipe_resource *
   vl_dri2_screen_texture_from_drawable(struct vl_screen *vscreen, void *drawable)
   {
               struct winsys_handle dri2_handle;
            xcb_dri2_get_buffers_reply_t *reply;
            unsigned depth = ((xcb_screen_t *)(vscreen->xcb_screen))->root_depth;
                     vl_dri2_set_drawable(scrn, (Drawable)drawable);
   reply = vl_dri2_get_flush_reply(scrn);
   if (!reply) {
      xcb_dri2_get_buffers_cookie_t cookie;
   cookie = xcb_dri2_get_buffers_unchecked(scrn->conn, (Drawable)drawable,
            }
   if (!reply)
            buffers = xcb_dri2_get_buffers_buffers(reply);
   if (!buffers)  {
      free(reply);
               for (i = 0; i < reply->count; ++i) {
      if (buffers[i].attachment == XCB_DRI2_ATTACHMENT_BUFFER_BACK_LEFT) {
      back_left = &buffers[i];
                  if (i == reply->count || !back_left) {
      free(reply);
               if (reply->width != scrn->width || reply->height != scrn->height) {
      vl_compositor_reset_dirty_area(&scrn->dirty_areas[0]);
   vl_compositor_reset_dirty_area(&scrn->dirty_areas[1]);
   scrn->width = reply->width;
         } else if (back_left->name != scrn->buffer_names[scrn->current_buffer]) {
      vl_compositor_reset_dirty_area(&scrn->dirty_areas[scrn->current_buffer]);
               memset(&dri2_handle, 0, sizeof(dri2_handle));
   dri2_handle.type = WINSYS_HANDLE_TYPE_SHARED;
   dri2_handle.handle = back_left->name;
   dri2_handle.stride = back_left->pitch;
            memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = vl_dri2_format_for_depth(vscreen, depth);
   templ.last_level = 0;
   templ.width0 = reply->width;
   templ.height0 = reply->height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.usage = PIPE_USAGE_DEFAULT;
   templ.bind = PIPE_BIND_RENDER_TARGET;
            tex = scrn->base.pscreen->resource_from_handle(scrn->base.pscreen, &templ,
                           }
      static struct u_rect *
   vl_dri2_screen_get_dirty_area(struct vl_screen *vscreen)
   {
      struct vl_dri_screen *scrn = (struct vl_dri_screen *)vscreen;
   assert(scrn);
      }
      static uint64_t
   vl_dri2_screen_get_timestamp(struct vl_screen *vscreen, void *drawable)
   {
      struct vl_dri_screen *scrn = (struct vl_dri_screen *)vscreen;
   xcb_dri2_get_msc_cookie_t cookie;
                     vl_dri2_set_drawable(scrn, (Drawable)drawable);
   if (!scrn->last_ust) {
      cookie = xcb_dri2_get_msc_unchecked(scrn->conn, (Drawable)drawable);
            if (reply) {
      vl_dri2_handle_stamps(scrn, reply->ust_hi, reply->ust_lo,
               }
      }
      static void
   vl_dri2_screen_set_next_timestamp(struct vl_screen *vscreen, uint64_t stamp)
   {
      struct vl_dri_screen *scrn = (struct vl_dri_screen *)vscreen;
   assert(scrn);
   if (stamp && scrn->last_ust && scrn->ns_frame && scrn->last_msc)
      scrn->next_msc = ((int64_t)stamp - scrn->last_ust + scrn->ns_frame/2) /
      else
      }
      static void *
   vl_dri2_screen_get_private(struct vl_screen *vscreen)
   {
         }
      static xcb_screen_t *
   get_xcb_screen(xcb_screen_iterator_t iter, int screen)
   {
      for (; iter.rem; --screen, xcb_screen_next(&iter))
      if (screen == 0)
            }
      static xcb_visualtype_t *
   get_xcb_visualtype_for_depth(struct vl_screen *vscreen, int depth)
   {
      xcb_visualtype_iterator_t visual_iter;
   xcb_screen_t *screen = vscreen->xcb_screen;
            if (!screen)
            depth_iter = xcb_screen_allowed_depths_iterator(screen);
   for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
      if (depth_iter.data->depth != depth)
            visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
   if (visual_iter.rem)
                  }
      static uint32_t
   get_red_mask_for_depth(struct vl_screen *vscreen, int depth)
   {
               if (visual) {
                     }
      uint32_t
   vl_dri2_format_for_depth(struct vl_screen *vscreen, int depth)
   {
      switch (depth) {
   case 24:
         case 30:
      /* Different preferred formats for different hw */
   if (get_red_mask_for_depth(vscreen, 30) == 0x3ff)
         else
      default:
            }
      struct vl_screen *
   vl_dri2_screen_create(Display *display, int screen)
   {
      struct vl_dri_screen *scrn;
   const xcb_query_extension_reply_t *extension;
   xcb_dri2_query_version_cookie_t dri2_query_cookie;
   xcb_dri2_query_version_reply_t *dri2_query = NULL;
   xcb_dri2_connect_cookie_t connect_cookie;
   xcb_dri2_connect_reply_t *connect = NULL;
   xcb_dri2_authenticate_cookie_t authenticate_cookie;
   xcb_dri2_authenticate_reply_t *authenticate = NULL;
   xcb_screen_iterator_t s;
   xcb_generic_error_t *error = NULL;
   char *device_name;
   int fd, device_name_length;
                              scrn = CALLOC_STRUCT(vl_dri_screen);
   if (!scrn)
            scrn->conn = XGetXCBConnection(display);
   if (!scrn->conn)
                     extension = xcb_get_extension_data(scrn->conn, &xcb_dri2_id);
   if (!(extension && extension->present))
            dri2_query_cookie = xcb_dri2_query_version (scrn->conn,
               dri2_query = xcb_dri2_query_version_reply (scrn->conn, dri2_query_cookie, &error);
   if (dri2_query == NULL || error != NULL || dri2_query->minor_version < 2)
            s = xcb_setup_roots_iterator(xcb_get_setup(scrn->conn));
   scrn->base.xcb_screen = get_xcb_screen(s, screen);
   if (!scrn->base.xcb_screen)
            driverType = XCB_DRI2_DRIVER_TYPE_DRI;
   {
      char *prime = getenv("DRI_PRIME");
   if (prime) {
      unsigned primeid;
   errno = 0;
   primeid = strtoul(prime, NULL, 0);
   if (errno == 0)
      driverType |=
               connect_cookie = xcb_dri2_connect_unchecked(
         connect = xcb_dri2_connect_reply(scrn->conn, connect_cookie, NULL);
   if (connect == NULL ||
      connect->driver_name_length + connect->device_name_length == 0)
         device_name_length = xcb_dri2_connect_device_name_length(connect);
   device_name = CALLOC(1, device_name_length + 1);
   if (!device_name)
         memcpy(device_name, xcb_dri2_connect_device_name(connect), device_name_length);
   fd = loader_open_device(device_name);
            if (fd < 0)
            if (drmGetMagic(fd, &magic))
            authenticate_cookie = xcb_dri2_authenticate_unchecked(
                  if (authenticate == NULL || !authenticate->authenticated)
            if (pipe_loader_drm_probe_fd(&scrn->base.dev, fd, false))
            if (!scrn->base.pscreen)
            scrn->base.destroy = vl_dri2_screen_destroy;
   scrn->base.texture_from_drawable = vl_dri2_screen_texture_from_drawable;
   scrn->base.get_dirty_area = vl_dri2_screen_get_dirty_area;
   scrn->base.get_timestamp = vl_dri2_screen_get_timestamp;
   scrn->base.set_next_timestamp = vl_dri2_screen_set_next_timestamp;
   scrn->base.get_private = vl_dri2_screen_get_private;
   scrn->base.pscreen->flush_frontbuffer = vl_dri2_flush_frontbuffer;
   vl_compositor_reset_dirty_area(&scrn->dirty_areas[0]);
            /* The pipe loader duplicates the fd */
   close(fd);
   free(authenticate);
   free(connect);
   free(dri2_query);
                  release_pipe:
      if (scrn->base.dev)
      free_authenticate:
         close_fd:
         free_connect:
         free_query:
      free(dri2_query);
         free_screen:
      FREE(scrn);
      }
      static void
   vl_dri2_screen_destroy(struct vl_screen *vscreen)
   {
                        if (scrn->flushed) {
      free(xcb_dri2_swap_buffers_reply(scrn->conn, scrn->swap_cookie, NULL));
   free(xcb_dri2_wait_sbc_reply(scrn->conn, scrn->wait_cookie, NULL));
               vl_dri2_destroy_drawable(scrn);
   scrn->base.pscreen->destroy(scrn->base.pscreen);
   pipe_loader_release(&scrn->base.dev, 1);
   /* There is no user provided fd */
      }
