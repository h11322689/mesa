   /**************************************************************************
   *
   * Copyright 2016 Advanced Micro Devices, Inc.
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
      #include <fcntl.h>
      #include <X11/Xlib-xcb.h>
   #include <X11/xshmfence.h>
   #include <xcb/dri3.h>
   #include <xcb/present.h>
   #include <xcb/xfixes.h>
      #include "loader.h"
      #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
   #include "pipe-loader/pipe_loader.h"
      #include "util/u_memory.h"
   #include "util/u_inlines.h"
      #include "vl/vl_compositor.h"
   #include "vl/vl_winsys.h"
      #include "drm-uapi/drm_fourcc.h"
      #define BACK_BUFFER_NUM 3
      struct vl_dri3_buffer
   {
      struct pipe_resource *texture;
            uint32_t pixmap;
   uint32_t region;
   uint32_t sync_fence;
            bool busy;
      };
      struct vl_dri3_screen
   {
      struct vl_screen base;
   xcb_connection_t *conn;
                     xcb_present_event_t eid;
            struct pipe_context *pipe;
   struct pipe_resource *output_texture;
            struct vl_dri3_buffer *back_buffers[BACK_BUFFER_NUM];
   int cur_back;
                     struct vl_dri3_buffer *front_buffer;
            uint32_t send_msc_serial, recv_msc_serial;
   uint64_t send_sbc, recv_sbc;
               };
      static void
   dri3_free_front_buffer(struct vl_dri3_screen *scrn,
         {
      xcb_sync_destroy_fence(scrn->conn, buffer->sync_fence);
   xshmfence_unmap_shm(buffer->shm_fence);
   pipe_resource_reference(&buffer->texture, NULL);
      }
      static void
   dri3_free_back_buffer(struct vl_dri3_screen *scrn,
         {
      if (buffer->region)
         xcb_free_pixmap(scrn->conn, buffer->pixmap);
   xcb_sync_destroy_fence(scrn->conn, buffer->sync_fence);
   xshmfence_unmap_shm(buffer->shm_fence);
   if (!scrn->output_texture)
         if (buffer->linear_texture)
            }
      static void
   dri3_handle_stamps(struct vl_dri3_screen *scrn, uint64_t ust, uint64_t msc)
   {
               if (scrn->last_ust && (ust_ns > scrn->last_ust) &&
      scrn->last_msc && (msc > scrn->last_msc))
         scrn->last_ust = ust_ns;
      }
      /* XXX this belongs in presentproto */
   #ifndef PresentWindowDestroyed
   #define PresentWindowDestroyed (1 << 0)
   #endif
   static bool
   dri3_handle_present_event(struct vl_dri3_screen *scrn,
         {
      switch (ge->evtype) {
   case XCB_PRESENT_CONFIGURE_NOTIFY: {
      xcb_present_configure_notify_event_t *ce = (void *) ge;
   if (ce->pixmap_flags & PresentWindowDestroyed) {
      free(ge);
      }
   scrn->width = ce->width;
   scrn->height = ce->height;
      }
   case XCB_PRESENT_COMPLETE_NOTIFY: {
      xcb_present_complete_notify_event_t *ce = (void *) ge;
   if (ce->kind == XCB_PRESENT_COMPLETE_KIND_PIXMAP) {
      scrn->recv_sbc = (scrn->send_sbc & 0xffffffff00000000LL) | ce->serial;
   if (scrn->recv_sbc > scrn->send_sbc)
            } else if (ce->kind == XCB_PRESENT_COMPLETE_KIND_NOTIFY_MSC) {
      scrn->recv_msc_serial = ce->serial;
      }
      }
   case XCB_PRESENT_EVENT_IDLE_NOTIFY: {
      xcb_present_idle_notify_event_t *ie = (void *) ge;
   int b;
   for (b = 0; b < BACK_BUFFER_NUM; b++) {
      struct vl_dri3_buffer *buf = scrn->back_buffers[b];
   if (buf && buf->pixmap == ie->pixmap) {
      buf->busy = false;
         }
      }
   }
   free(ge);
      }
      static void
   dri3_flush_present_events(struct vl_dri3_screen *scrn)
   {
      if (scrn->special_event) {
      xcb_generic_event_t *ev;
   while ((ev = xcb_poll_for_special_event(
            if (!dri3_handle_present_event(scrn, (xcb_present_generic_event_t *)ev))
            }
      static bool
   dri3_wait_present_events(struct vl_dri3_screen *scrn)
   {
      if (scrn->special_event) {
      xcb_generic_event_t *ev;
   ev = xcb_wait_for_special_event(scrn->conn, scrn->special_event);
   if (!ev)
            }
      }
      static int
   dri3_find_back(struct vl_dri3_screen *scrn)
   {
               for (;;) {
      for (b = 0; b < BACK_BUFFER_NUM; b++) {
      int id = (b + scrn->cur_back) % BACK_BUFFER_NUM;
   struct vl_dri3_buffer *buffer = scrn->back_buffers[id];
   if (!buffer || !buffer->busy)
      }
   xcb_flush(scrn->conn);
   if (!dri3_wait_present_events(scrn))
         }
      static struct vl_dri3_buffer *
   dri3_alloc_back_buffer(struct vl_dri3_screen *scrn)
   {
      struct vl_dri3_buffer *buffer;
   xcb_pixmap_t pixmap;
   xcb_sync_fence_t sync_fence;
   struct xshmfence *shm_fence;
   int buffer_fd, fence_fd;
   struct pipe_resource templ, *pixmap_buffer_texture;
            buffer = CALLOC_STRUCT(vl_dri3_buffer);
   if (!buffer)
            fence_fd = xshmfence_alloc_shm();
   if (fence_fd < 0)
            shm_fence = xshmfence_map_shm(fence_fd);
   if (!shm_fence)
            memset(&templ, 0, sizeof(templ));
   templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
   templ.format = vl_dri2_format_for_depth(&scrn->base, scrn->depth);
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = (scrn->output_texture) ?
         templ.height0 = (scrn->output_texture) ?
         templ.depth0 = 1;
            if (scrn->is_different_gpu) {
      buffer->texture = (scrn->output_texture) ? scrn->output_texture :
         if (!buffer->texture)
            templ.bind |= PIPE_BIND_SCANOUT | PIPE_BIND_SHARED |
         buffer->linear_texture =
                  if (!buffer->linear_texture)
      } else {
      templ.bind |= PIPE_BIND_SCANOUT | PIPE_BIND_SHARED;
   buffer->texture = (scrn->output_texture) ? scrn->output_texture :
         if (!buffer->texture)
            }
   memset(&whandle, 0, sizeof(whandle));
   whandle.type= WINSYS_HANDLE_TYPE_FD;
   scrn->base.pscreen->resource_get_handle(scrn->base.pscreen, NULL,
         buffer_fd = whandle.handle;
   buffer->pitch = whandle.stride;
   buffer->width = templ.width0;
            xcb_dri3_pixmap_from_buffer(scrn->conn,
                              (pixmap = xcb_generate_id(scrn->conn)),
      xcb_dri3_fence_from_fd(scrn->conn,
                              buffer->pixmap = pixmap;
   buffer->sync_fence = sync_fence;
                           no_linear_texture:
         unmap_shm:
         close_fd:
         free_buffer:
      FREE(buffer);
      }
      static struct vl_dri3_buffer *
   dri3_get_back_buffer(struct vl_dri3_screen *scrn)
   {
      struct vl_dri3_buffer *buffer;
   struct pipe_resource *texture = NULL;
   bool allocate_new_buffer = false;
                     scrn->cur_back = dri3_find_back(scrn);
   if (scrn->cur_back < 0)
                  if (scrn->output_texture) {
      if (!buffer || buffer->width < scrn->width ||
      buffer->height < scrn->height)
      else if (scrn->is_different_gpu)
      /* In case of different gpu we can reuse the linear
   * texture so we only need to set the external
   * texture for copying
   */
      else {
      /* In case of a single gpu we search if the texture is
   * already present as buffer if not we get the
   * handle and pixmap for the texture that is set
   */
   for (b = 0; b < BACK_BUFFER_NUM; b++) {
      id = (b + scrn->cur_back) % BACK_BUFFER_NUM;
   buffer = scrn->back_buffers[id];
   if (buffer && !buffer->busy &&
      buffer->texture == scrn->output_texture) {
   scrn->cur_back = id;
                  if (b == BACK_BUFFER_NUM) {
      allocate_new_buffer = true;
   scrn->cur_back = scrn->next_back;
   scrn->next_back = (scrn->next_back + 1) % BACK_BUFFER_NUM;
               } else {
      if (!buffer || buffer->width != scrn->width ||
      buffer->height != scrn->height)
            if (allocate_new_buffer) {
               new_buffer = dri3_alloc_back_buffer(scrn);
   if (!new_buffer)
            if (buffer)
            if (!scrn->output_texture)
         buffer = new_buffer;
               pipe_resource_reference(&texture, buffer->texture);
   xcb_flush(scrn->conn);
               }
      static bool
   dri3_set_drawable(struct vl_dri3_screen *scrn, Drawable drawable)
   {
      xcb_get_geometry_cookie_t geom_cookie;
   xcb_get_geometry_reply_t *geom_reply;
   xcb_void_cookie_t cookie;
   xcb_generic_error_t *error;
                     if (scrn->drawable == drawable)
                     geom_cookie = xcb_get_geometry(scrn->conn, scrn->drawable);
   geom_reply = xcb_get_geometry_reply(scrn->conn, geom_cookie, NULL);
   if (!geom_reply)
            scrn->width = geom_reply->width;
   scrn->height = geom_reply->height;
   scrn->depth = geom_reply->depth;
            if (scrn->special_event) {
      xcb_unregister_for_special_event(scrn->conn, scrn->special_event);
   scrn->special_event = NULL;
   cookie = xcb_present_select_input_checked(scrn->conn, scrn->eid,
                           scrn->is_pixmap = false;
   scrn->eid = xcb_generate_id(scrn->conn);
   cookie =
      xcb_present_select_input_checked(scrn->conn, scrn->eid, scrn->drawable,
                     error = xcb_request_check(scrn->conn, cookie);
   if (error) {
      if (error->error_code != BadWindow)
         else {
      scrn->is_pixmap = true;
   scrn->base.set_back_texture_from_output = NULL;
   if (scrn->front_buffer) {
      dri3_free_front_buffer(scrn, scrn->front_buffer);
         }
      } else
      scrn->special_event =
                     }
      static struct vl_dri3_buffer *
   dri3_get_front_buffer(struct vl_dri3_screen *scrn)
   {
      xcb_dri3_buffer_from_pixmap_cookie_t bp_cookie;
   xcb_dri3_buffer_from_pixmap_reply_t *bp_reply;
   xcb_sync_fence_t sync_fence;
   struct xshmfence *shm_fence;
   int fence_fd, *fds;
   struct winsys_handle whandle;
            if (scrn->front_buffer) {
      pipe_resource_reference(&texture, scrn->front_buffer->texture);
               scrn->front_buffer = CALLOC_STRUCT(vl_dri3_buffer);
   if (!scrn->front_buffer)
            fence_fd = xshmfence_alloc_shm();
   if (fence_fd < 0)
            shm_fence = xshmfence_map_shm(fence_fd);
   if (!shm_fence)
            bp_cookie = xcb_dri3_buffer_from_pixmap(scrn->conn, scrn->drawable);
   bp_reply = xcb_dri3_buffer_from_pixmap_reply(scrn->conn, bp_cookie, NULL);
   if (!bp_reply)
            fds = xcb_dri3_buffer_from_pixmap_reply_fds(scrn->conn, bp_reply);
   if (fds[0] < 0)
            memset(&whandle, 0, sizeof(whandle));
   whandle.type = WINSYS_HANDLE_TYPE_FD;
   whandle.handle = (unsigned)fds[0];
   whandle.stride = bp_reply->stride;
   whandle.modifier = DRM_FORMAT_MOD_INVALID;
   memset(&templ, 0, sizeof(templ));
   templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
   templ.format = vl_dri2_format_for_depth(&scrn->base, bp_reply->depth);
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = bp_reply->width;
   templ.height0 = bp_reply->height;
   templ.depth0 = 1;
   templ.array_size = 1;
   scrn->front_buffer->texture =
      scrn->base.pscreen->resource_from_handle(scrn->base.pscreen,
            close(fds[0]);
   if (!scrn->front_buffer->texture)
            xcb_dri3_fence_from_fd(scrn->conn,
                              pipe_resource_reference(&texture, scrn->front_buffer->texture);
   scrn->front_buffer->pixmap = scrn->drawable;
   scrn->front_buffer->width = bp_reply->width;
   scrn->front_buffer->height = bp_reply->height;
   scrn->front_buffer->shm_fence = shm_fence;
   scrn->front_buffer->sync_fence = sync_fence;
                  free_reply:
         unmap_shm:
         close_fd:
         free_buffer:
      FREE(scrn->front_buffer);
      }
      static xcb_screen_t *
   dri3_get_screen_for_root(xcb_connection_t *conn, xcb_window_t root)
   {
      xcb_screen_iterator_t screen_iter =
            for (; screen_iter.rem; xcb_screen_next (&screen_iter)) {
      if (screen_iter.data->root == root)
                  }
      static void
   vl_dri3_flush_frontbuffer(struct pipe_screen *screen,
                           {
      struct vl_dri3_screen *scrn = (struct vl_dri3_screen *)context_private;
   uint32_t options = XCB_PRESENT_OPTION_NONE;
   struct vl_dri3_buffer *back;
   struct pipe_box src_box;
            back = scrn->back_buffers[scrn->cur_back];
   if (!back)
            while (scrn->special_event && scrn->recv_sbc < scrn->send_sbc)
      if (!dri3_wait_present_events(scrn))
         rectangle.x = 0;
   rectangle.y = 0;
   rectangle.width = (scrn->output_texture) ? scrn->clip_width : scrn->width;
            if (!back->region) {
      back->region = xcb_generate_id(scrn->conn);
      }
            if (scrn->is_different_gpu) {
      u_box_origin_2d(back->width, back->height, &src_box);
   scrn->pipe->resource_copy_region(scrn->pipe,
                                 }
   xshmfence_reset(back->shm_fence);
            xcb_present_pixmap(scrn->conn,
                     scrn->drawable,
   back->pixmap,
   (uint32_t)(++scrn->send_sbc),
   0, back->region, 0, 0,
   None, None,
                        }
      static struct pipe_resource *
   vl_dri3_screen_texture_from_drawable(struct vl_screen *vscreen, void *drawable)
   {
      struct vl_dri3_screen *scrn = (struct vl_dri3_screen *)vscreen;
                     if (!dri3_set_drawable(scrn, (Drawable)drawable))
            buffer = (scrn->is_pixmap) ?
               if (!buffer)
               }
      static struct u_rect *
   vl_dri3_screen_get_dirty_area(struct vl_screen *vscreen)
   {
                           }
      static uint64_t
   vl_dri3_screen_get_timestamp(struct vl_screen *vscreen, void *drawable)
   {
                        if (!dri3_set_drawable(scrn, (Drawable)drawable))
            if (!scrn->last_ust) {
      xcb_present_notify_msc(scrn->conn,
                              while (scrn->special_event &&
            if (!dri3_wait_present_events(scrn))
                     }
      static void
   vl_dri3_screen_set_next_timestamp(struct vl_screen *vscreen, uint64_t stamp)
   {
                        if (stamp && scrn->last_ust && scrn->ns_frame && scrn->last_msc)
      scrn->next_msc = ((int64_t)stamp - scrn->last_ust + scrn->ns_frame/2) /
      else
      }
      static void *
   vl_dri3_screen_get_private(struct vl_screen *vscreen)
   {
         }
      static void
   vl_dri3_screen_set_back_texture_from_output(struct vl_screen *vscreen,
               {
                        scrn->output_texture = buffer;
   scrn->clip_width = (width) ? width : scrn->width;
      }
      static void
   vl_dri3_screen_destroy(struct vl_screen *vscreen)
   {
      struct vl_dri3_screen *scrn = (struct vl_dri3_screen *)vscreen;
                              if (scrn->front_buffer) {
      dri3_free_front_buffer(scrn, scrn->front_buffer);
               for (i = 0; i < BACK_BUFFER_NUM; ++i) {
      if (scrn->back_buffers[i]) {
      dri3_free_back_buffer(scrn, scrn->back_buffers[i]);
                  if (scrn->special_event) {
      xcb_void_cookie_t cookie =
      xcb_present_select_input_checked(scrn->conn, scrn->eid,
               xcb_discard_reply(scrn->conn, cookie.sequence);
      }
   scrn->pipe->destroy(scrn->pipe);
   scrn->base.pscreen->destroy(scrn->base.pscreen);
   pipe_loader_release(&scrn->base.dev, 1);
               }
      struct vl_screen *
   vl_dri3_screen_create(Display *display, int screen)
   {
      struct vl_dri3_screen *scrn;
   const xcb_query_extension_reply_t *extension;
   xcb_dri3_open_cookie_t open_cookie;
   xcb_dri3_open_reply_t *open_reply;
   xcb_get_geometry_cookie_t geom_cookie;
   xcb_get_geometry_reply_t *geom_reply;
   xcb_xfixes_query_version_cookie_t xfixes_cookie;
   xcb_xfixes_query_version_reply_t *xfixes_reply;
   xcb_generic_error_t *error;
                     scrn = CALLOC_STRUCT(vl_dri3_screen);
   if (!scrn)
            scrn->conn = XGetXCBConnection(display);
   if (!scrn->conn)
            xcb_prefetch_extension_data(scrn->conn , &xcb_dri3_id);
   xcb_prefetch_extension_data(scrn->conn, &xcb_present_id);
   xcb_prefetch_extension_data (scrn->conn, &xcb_xfixes_id);
   extension = xcb_get_extension_data(scrn->conn, &xcb_dri3_id);
   if (!(extension && extension->present))
         extension = xcb_get_extension_data(scrn->conn, &xcb_present_id);
   if (!(extension && extension->present))
         extension = xcb_get_extension_data(scrn->conn, &xcb_xfixes_id);
   if (!(extension && extension->present))
            xfixes_cookie = xcb_xfixes_query_version(scrn->conn, XCB_XFIXES_MAJOR_VERSION,
         xfixes_reply = xcb_xfixes_query_version_reply(scrn->conn, xfixes_cookie, &error);
   if (!xfixes_reply || error || xfixes_reply->major_version < 2) {
      free(error);
   free(xfixes_reply);
      }
            open_cookie = xcb_dri3_open(scrn->conn, RootWindow(display, screen), None);
   open_reply = xcb_dri3_open_reply(scrn->conn, open_cookie, NULL);
   if (!open_reply)
         if (open_reply->nfd != 1) {
      free(open_reply);
               fd = xcb_dri3_open_reply_fds(scrn->conn, open_reply)[0];
   if (fd < 0) {
      free(open_reply);
      }
   fcntl(fd, F_SETFD, FD_CLOEXEC);
                     geom_cookie = xcb_get_geometry(scrn->conn, RootWindow(display, screen));
   geom_reply = xcb_get_geometry_reply(scrn->conn, geom_cookie, NULL);
   if (!geom_reply)
            scrn->base.xcb_screen = dri3_get_screen_for_root(scrn->conn, geom_reply->root);
   if (!scrn->base.xcb_screen) {
      free(geom_reply);
               /* TODO support depth other than 24 or 30 */
   if (geom_reply->depth != 24 && geom_reply->depth != 30) {
      free(geom_reply);
      }
   scrn->base.color_depth = geom_reply->depth;
            if (pipe_loader_drm_probe_fd(&scrn->base.dev, fd, false))
            if (!scrn->base.pscreen)
            scrn->pipe = pipe_create_multimedia_context(scrn->base.pscreen);
   if (!scrn->pipe)
            scrn->base.destroy = vl_dri3_screen_destroy;
   scrn->base.texture_from_drawable = vl_dri3_screen_texture_from_drawable;
   scrn->base.get_dirty_area = vl_dri3_screen_get_dirty_area;
   scrn->base.get_timestamp = vl_dri3_screen_get_timestamp;
   scrn->base.set_next_timestamp = vl_dri3_screen_set_next_timestamp;
   scrn->base.get_private = vl_dri3_screen_get_private;
   scrn->base.pscreen->flush_frontbuffer = vl_dri3_flush_frontbuffer;
                     close(fd);
            no_context:
         release_pipe:
      if (scrn->base.dev) {
      pipe_loader_release(&scrn->base.dev, 1);
         close_fd:
      if (fd != -1)
      free_screen:
      FREE(scrn);
      }
