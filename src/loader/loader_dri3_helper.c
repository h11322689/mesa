   /*
   * Copyright © 2013 Keith Packard
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
   #include <stdlib.h>
   #include <unistd.h>
   #include <string.h>
      #include <X11/xshmfence.h>
   #include <xcb/xcb.h>
   #include <xcb/dri3.h>
   #include <xcb/present.h>
   #include <xcb/xfixes.h>
      #include <X11/Xlib-xcb.h>
      #include "loader_dri_helper.h"
   #include "loader_dri3_helper.h"
   #include "util/macros.h"
   #include "util/simple_mtx.h"
   #include "drm-uapi/drm_fourcc.h"
      /**
   * A cached blit context.
   */
   struct loader_dri3_blit_context {
      simple_mtx_t mtx;
   __DRIcontext *ctx;
   __DRIscreen *cur_screen;
      };
      /* For simplicity we maintain the cache only for a single screen at a time */
   static struct loader_dri3_blit_context blit_context = {
         };
      static void
   dri3_flush_present_events(struct loader_dri3_drawable *draw);
      static struct loader_dri3_buffer *
   dri3_find_back_alloc(struct loader_dri3_drawable *draw);
      static xcb_screen_t *
   get_screen_for_root(xcb_connection_t *conn, xcb_window_t root)
   {
      xcb_screen_iterator_t screen_iter =
            for (; screen_iter.rem; xcb_screen_next (&screen_iter)) {
      if (screen_iter.data->root == root)
                  }
      static xcb_visualtype_t *
   get_xcb_visualtype_for_depth(struct loader_dri3_drawable *draw, int depth)
   {
      xcb_visualtype_iterator_t visual_iter;
   xcb_screen_t *screen = draw->screen;
            if (!screen)
            depth_iter = xcb_screen_allowed_depths_iterator(screen);
   for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
      if (depth_iter.data->depth != depth)
            visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
   if (visual_iter.rem)
                  }
      /* Sets the adaptive sync window property state. */
   static void
   set_adaptive_sync_property(xcb_connection_t *conn, xcb_drawable_t drawable,
         {
      static char const name[] = "_VARIABLE_REFRESH";
   xcb_intern_atom_cookie_t cookie;
   xcb_intern_atom_reply_t* reply;
            cookie = xcb_intern_atom(conn, 0, strlen(name), name);
   reply = xcb_intern_atom_reply(conn, cookie, NULL);
   if (reply == NULL)
            if (state)
      check = xcb_change_property_checked(conn, XCB_PROP_MODE_REPLACE,
            else
            xcb_discard_reply(conn, check.sequence);
      }
      /* Get red channel mask for given drawable at given depth. */
   static unsigned int
   dri3_get_red_mask_for_depth(struct loader_dri3_drawable *draw, int depth)
   {
               if (visual)
               }
      /**
   * Do we have blit functionality in the image blit extension?
   *
   * \param draw[in]  The drawable intended to blit from / to.
   * \return  true if we have blit functionality. false otherwise.
   */
   static bool loader_dri3_have_image_blit(const struct loader_dri3_drawable *draw)
   {
      return draw->ext->image->base.version >= 9 &&
      }
      /**
   * Get and lock (for use with the current thread) a dri context associated
   * with the drawable's dri screen. The context is intended to be used with
   * the dri image extension's blitImage method.
   *
   * \param draw[in]  Pointer to the drawable whose dri screen we want a
   * dri context for.
   * \return A dri context or NULL if context creation failed.
   *
   * When the caller is done with the context (even if the context returned was
   * NULL), the caller must call loader_dri3_blit_context_put.
   */
   static __DRIcontext *
   loader_dri3_blit_context_get(struct loader_dri3_drawable *draw)
   {
               if (blit_context.ctx && blit_context.cur_screen != draw->dri_screen_render_gpu) {
      blit_context.core->destroyContext(blit_context.ctx);
               if (!blit_context.ctx) {
      blit_context.ctx = draw->ext->core->createNewContext(draw->dri_screen_render_gpu,
         blit_context.cur_screen = draw->dri_screen_render_gpu;
                  }
      /**
   * Release (for use with other threads) a dri context previously obtained using
   * loader_dri3_blit_context_get.
   */
   static void
   loader_dri3_blit_context_put(void)
   {
         }
      /**
   * Blit (parts of) the contents of a DRI image to another dri image
   *
   * \param draw[in]  The drawable which owns the images.
   * \param dst[in]  The destination image.
   * \param src[in]  The source image.
   * \param dstx0[in]  Start destination coordinate.
   * \param dsty0[in]  Start destination coordinate.
   * \param width[in]  Blit width.
   * \param height[in] Blit height.
   * \param srcx0[in]  Start source coordinate.
   * \param srcy0[in]  Start source coordinate.
   * \param flush_flag[in]  Image blit flush flag.
   * \return true iff successful.
   */
   static bool
   loader_dri3_blit_image(struct loader_dri3_drawable *draw,
                     {
      __DRIcontext *dri_context;
            if (!loader_dri3_have_image_blit(draw))
                     if (!dri_context || !draw->vtable->in_current_context(draw)) {
      dri_context = loader_dri3_blit_context_get(draw);
   use_blit_context = true;
               if (dri_context)
      draw->ext->image->blitImage(dri_context, dst, src, dstx0, dsty0,
               if (use_blit_context)
               }
      static inline void
   dri3_fence_reset(xcb_connection_t *c, struct loader_dri3_buffer *buffer)
   {
         }
      static inline void
   dri3_fence_set(struct loader_dri3_buffer *buffer)
   {
         }
      static inline void
   dri3_fence_trigger(xcb_connection_t *c, struct loader_dri3_buffer *buffer)
   {
         }
      static inline void
   dri3_fence_await(xcb_connection_t *c, struct loader_dri3_drawable *draw,
         {
      xcb_flush(c);
   xshmfence_await(buffer->shm_fence);
   if (draw) {
      mtx_lock(&draw->mtx);
   dri3_flush_present_events(draw);
         }
      static void
   dri3_update_max_num_back(struct loader_dri3_drawable *draw)
   {
      switch (draw->last_present_mode) {
   case XCB_PRESENT_COMPLETE_MODE_FLIP: {
      if (draw->swap_interval == 0)
         else
            assert(draw->max_num_back <= LOADER_DRI3_MAX_BACK);
               case XCB_PRESENT_COMPLETE_MODE_SKIP:
            default:
            }
      void
   loader_dri3_set_swap_interval(struct loader_dri3_drawable *draw, int interval)
   {
      /* Wait all previous swap done before changing swap interval.
   *
   * This is for preventing swap out of order in the following cases:
   *   1. Change from sync swap mode (>0) to async mode (=0), so async swap occurs
   *      before previous pending sync swap.
   *   2. Change from value A to B and A > B, so the target_msc for the previous
   *      pending swap may be bigger than newer swap.
   *
   * PS. changing from value A to B and A < B won't cause swap out of order but
   * may still gets wrong target_msc value at the beginning.
   */
   if (draw->swap_interval != interval)
               }
      static void
   dri3_set_render_buffer(struct loader_dri3_drawable *draw, int buf_id,
         {
      if (buf_id != LOADER_DRI3_FRONT_ID && !draw->buffers[buf_id])
               }
      /** dri3_free_render_buffer
   *
   * Free everything associated with one render buffer including pixmap, fence
   * stuff and the driver image
   */
   static void
   dri3_free_render_buffer(struct loader_dri3_drawable *draw,
         {
               if (!buffer)
            if (buffer->own_pixmap)
         xcb_sync_destroy_fence(draw->conn, buffer->sync_fence);
   xshmfence_unmap_shm(buffer->shm_fence);
   draw->ext->image->destroyImage(buffer->image);
   if (buffer->linear_buffer)
                           if (buf_id != LOADER_DRI3_FRONT_ID)
      }
      void
   loader_dri3_drawable_fini(struct loader_dri3_drawable *draw)
   {
                        for (i = 0; i < ARRAY_SIZE(draw->buffers); i++)
            if (draw->special_event) {
      xcb_void_cookie_t cookie =
                  xcb_discard_reply(draw->conn, cookie.sequence);
               if (draw->region)
            cnd_destroy(&draw->event_cnd);
      }
      int
   loader_dri3_drawable_init(xcb_connection_t *conn,
                           xcb_drawable_t drawable,
   enum loader_dri3_drawable_type type,
   __DRIscreen *dri_screen_render_gpu,
   __DRIscreen *dri_screen_display_gpu,
   bool multiplanes_available,
   bool prefer_back_buffer_reuse,
   {
      xcb_get_geometry_cookie_t cookie;
   xcb_get_geometry_reply_t *reply;
            draw->conn = conn;
   draw->ext = ext;
   draw->vtable = vtable;
   draw->drawable = drawable;
   draw->type = type;
   draw->region = 0;
   draw->dri_screen_render_gpu = dri_screen_render_gpu;
   draw->dri_screen_display_gpu = dri_screen_display_gpu;
   draw->multiplanes_available = multiplanes_available;
   draw->prefer_back_buffer_reuse = prefer_back_buffer_reuse;
            draw->have_back = 0;
   draw->have_fake_front = 0;
   draw->first_init = true;
   draw->adaptive_sync = false;
   draw->adaptive_sync_active = false;
            draw->cur_blit_source = -1;
   draw->back_format = __DRI_IMAGE_FORMAT_NONE;
   mtx_init(&draw->mtx, mtx_plain);
            if (draw->ext->config) {
      unsigned char adaptive_sync = 0;
            draw->ext->config->configQueryb(draw->dri_screen_render_gpu,
                           draw->ext->config->configQueryb(draw->dri_screen_render_gpu,
                              if (!draw->adaptive_sync)
            draw->swap_interval = dri_get_initial_swap_interval(draw->dri_screen_render_gpu,
                     /* Create a new drawable */
   draw->dri_drawable =
      draw->ext->image_driver->createNewDrawable(dri_screen_render_gpu,
               if (!draw->dri_drawable)
            cookie = xcb_get_geometry(draw->conn, draw->drawable);
   reply = xcb_get_geometry_reply(draw->conn, cookie, &error);
   if (reply == NULL || error != NULL) {
      draw->ext->core->destroyDrawable(draw->dri_drawable);
               draw->screen = get_screen_for_root(draw->conn, reply->root);
   draw->width = reply->width;
   draw->height = reply->height;
   draw->depth = reply->depth;
   draw->vtable->set_drawable_size(draw, draw->width, draw->height);
            /*
   * Make sure server has the same swap interval we do for the new
   * drawable.
   */
               }
      /* XXX this belongs in presentproto */
   #ifndef PresentWindowDestroyed
   #define PresentWindowDestroyed (1 << 0)
   #endif
   /*
   * Process one Present event
   */
   static bool
   dri3_handle_present_event(struct loader_dri3_drawable *draw,
         {
      switch (ge->evtype) {
   case XCB_PRESENT_CONFIGURE_NOTIFY: {
      xcb_present_configure_notify_event_t *ce = (void *) ge;
   if (ce->pixmap_flags & PresentWindowDestroyed) {
      free(ge);
               draw->width = ce->width;
   draw->height = ce->height;
   draw->vtable->set_drawable_size(draw, draw->width, draw->height);
   draw->ext->flush->invalidate(draw->dri_drawable);
      }
   case XCB_PRESENT_COMPLETE_NOTIFY: {
               /* Compute the processed SBC number from the received 32-bit serial number
   * merged with the upper 32-bits of the sent 64-bit serial number while
   * checking for wrap.
   */
   if (ce->kind == XCB_PRESENT_COMPLETE_KIND_PIXMAP) {
               /* Only assume wraparound if that results in exactly the previous
   * SBC + 1, otherwise ignore received SBC > sent SBC (those are
   * probably from a previous loader_dri3_drawable instance) to avoid
   * calculating bogus target MSC values in loader_dri3_swap_buffers_msc
   */
   if (recv_sbc <= draw->send_sbc)
                        /* When moving from flip to copy, we assume that we can allocate in
   * a more optimal way if we don't need to cater for the display
   * controller.
   */
   if (ce->mode == XCB_PRESENT_COMPLETE_MODE_COPY &&
      draw->last_present_mode == XCB_PRESENT_COMPLETE_MODE_FLIP) {
   for (int b = 0; b < ARRAY_SIZE(draw->buffers); b++) {
      if (draw->buffers[b])
                  /* If the server tells us that our allocation is suboptimal, we
      #ifdef HAVE_DRI3_MODIFIERS
            if (ce->mode == XCB_PRESENT_COMPLETE_MODE_SUBOPTIMAL_COPY &&
      draw->last_present_mode != ce->mode) {
   for (int b = 0; b < ARRAY_SIZE(draw->buffers); b++) {
      if (draw->buffers[b])
      #endif
                     draw->ust = ce->ust;
      } else if (ce->serial == draw->eid) {
      draw->notify_ust = ce->ust;
      }
      }
   case XCB_PRESENT_EVENT_IDLE_NOTIFY: {
      xcb_present_idle_notify_event_t *ie = (void *) ge;
            for (b = 0; b < ARRAY_SIZE(draw->buffers); b++) {
               if (buf && buf->pixmap == ie->pixmap)
      }
      }
   }
   free(ge);
      }
      static bool
   dri3_wait_for_event_locked(struct loader_dri3_drawable *draw,
         {
      xcb_generic_event_t *ev;
                     /* Only have one thread waiting for events at a time */
   if (draw->has_event_waiter) {
      cnd_wait(&draw->event_cnd, &draw->mtx);
   if (full_sequence)
         /* Another thread has updated the protected info, so retest. */
      } else {
      draw->has_event_waiter = true;
   /* Allow other threads access to the drawable while we're waiting. */
   mtx_unlock(&draw->mtx);
   ev = xcb_wait_for_special_event(draw->conn, draw->special_event);
   mtx_lock(&draw->mtx);
   draw->has_event_waiter = false;
      }
   if (!ev)
         draw->last_special_event_sequence = ev->full_sequence;
   if (full_sequence)
         ge = (void *) ev;
      }
      /** loader_dri3_wait_for_msc
   *
   * Get the X server to send an event when the target msc/divisor/remainder is
   * reached.
   */
   bool
   loader_dri3_wait_for_msc(struct loader_dri3_drawable *draw,
                     {
      xcb_void_cookie_t cookie = xcb_present_notify_msc(draw->conn,
                                                   /* Wait for the event */
   do {
      if (!dri3_wait_for_event_locked(draw, &full_sequence)) {
      mtx_unlock(&draw->mtx);
                  *ust = draw->notify_ust;
   *msc = draw->notify_msc;
   *sbc = draw->recv_sbc;
               }
      /** loader_dri3_wait_for_sbc
   *
   * Wait for the completed swap buffer count to reach the specified
   * target. Presumably the application knows that this will be reached with
   * outstanding complete events, or we're going to be here awhile.
   */
   int
   loader_dri3_wait_for_sbc(struct loader_dri3_drawable *draw,
               {
      /* From the GLX_OML_sync_control spec:
   *
   *     "If <target_sbc> = 0, the function will block until all previous
   *      swaps requested with glXSwapBuffersMscOML for that window have
   *      completed."
   */
   mtx_lock(&draw->mtx);
   if (!target_sbc)
            while (draw->recv_sbc < target_sbc) {
      if (!dri3_wait_for_event_locked(draw, NULL)) {
      mtx_unlock(&draw->mtx);
                  *ust = draw->ust;
   *msc = draw->msc;
   *sbc = draw->recv_sbc;
   mtx_unlock(&draw->mtx);
      }
      /** loader_dri3_find_back
   *
   * Find an idle back buffer. If there isn't one, then
   * wait for a present idle notify event from the X server
   */
   static int
   dri3_find_back(struct loader_dri3_drawable *draw, bool prefer_a_different)
   {
      struct loader_dri3_buffer *buffer;
   int b;
   int max_num;
   int best_id = -1;
                     if (!prefer_a_different) {
      /* Increase the likelyhood of reusing current buffer */
            /* Reuse current back buffer if it's idle */
   buffer = draw->buffers[draw->cur_back];
   if (buffer && !buffer->busy) {
      best_id = draw->cur_back;
                  /* Check whether we need to reuse the current back buffer as new back.
   * In that case, wait until it's not busy anymore.
   */
   if (!loader_dri3_have_image_blit(draw) && draw->cur_blit_source != -1) {
      max_num = 1;
      } else {
                  /* In a DRI_PRIME situation, if prefer_a_different is true, we first try
   * to find an idle buffer that is not the last used one.
   * This is useful if we receive a XCB_PRESENT_EVENT_IDLE_NOTIFY event
   * for a pixmap but it's not actually idle (eg: the DRI_PRIME blit is
   * still in progress).
   * Unigine Superposition hits this and this allows to use 2 back buffers
   * instead of reusing the same one all the time, causing the next frame
   * to wait for the copy to finish.
   */
   int current_back_id = draw->cur_back;
   do {
      /* Find idle buffer with lowest buffer age, or an unallocated slot */
   for (b = 0; b < max_num; b++) {
               buffer = draw->buffers[id];
   if (buffer) {
      if (!buffer->busy &&
      (!prefer_a_different || id != current_back_id) &&
   (best_id == -1 || buffer->last_swap > best_swap)) {
   best_id = id;
         } else if (best_id == -1 &&
                           /* Prefer re-using the same buffer over blocking */
   if (prefer_a_different && best_id == -1 &&
      !draw->buffers[LOADER_DRI3_BACK_ID(current_back_id)]->busy)
            if (best_id != -1)
         unlock:
      mtx_unlock(&draw->mtx);
      }
      static xcb_gcontext_t
   dri3_drawable_gc(struct loader_dri3_drawable *draw)
   {
      if (!draw->gc) {
      uint32_t v = 0;
   xcb_create_gc(draw->conn,
               (draw->gc = xcb_generate_id(draw->conn)),
      }
      }
         static struct loader_dri3_buffer *
   dri3_back_buffer(struct loader_dri3_drawable *draw)
   {
         }
      static struct loader_dri3_buffer *
   dri3_front_buffer(struct loader_dri3_drawable *draw)
   {
         }
      static void
   dri3_copy_area(xcb_connection_t *c,
                  xcb_drawable_t    src_drawable,
   xcb_drawable_t    dst_drawable,
   xcb_gcontext_t    gc,
   int16_t           src_x,
   int16_t           src_y,
   int16_t           dst_x,
      {
               cookie = xcb_copy_area_checked(c,
                                 src_drawable,
   dst_drawable,
   gc,
   src_x,
      }
      /**
   * Asks the driver to flush any queued work necessary for serializing with the
   * X command stream, and optionally the slightly more strict requirement of
   * glFlush() equivalence (which would require flushing even if nothing had
   * been drawn to a window system framebuffer, for example).
   */
   void
   loader_dri3_flush(struct loader_dri3_drawable *draw,
               {
      /* NEED TO CHECK WHETHER CONTEXT IS NULL */
            if (dri_context) {
      draw->ext->flush->flush_with_flags(dri_context, draw->dri_drawable,
         }
      void
   loader_dri3_copy_sub_buffer(struct loader_dri3_drawable *draw,
                     {
      struct loader_dri3_buffer *back;
            /* Check we have the right attachments */
   if (!draw->have_back || draw->type != LOADER_DRI3_DRAWABLE_WINDOW)
            if (flush)
                  back = dri3_find_back_alloc(draw);
   if (!back)
                     if (draw->dri_screen_render_gpu != draw->dri_screen_display_gpu) {
      /* Update the linear buffer part of the back buffer
   * for the dri3_copy_area operation
   */
   (void) loader_dri3_blit_image(draw,
                                 loader_dri3_swapbuffer_barrier(draw);
   dri3_fence_reset(draw->conn, back);
   dri3_copy_area(draw->conn,
                  back->pixmap,
      dri3_fence_trigger(draw->conn, back);
   /* Refresh the fake front (if present) after we just damaged the real
   * front.
   */
   if (draw->have_fake_front &&
      !loader_dri3_blit_image(draw,
                           draw->dri_screen_render_gpu == draw->dri_screen_display_gpu) {
   dri3_fence_reset(draw->conn, dri3_front_buffer(draw));
   dri3_copy_area(draw->conn,
                  back->pixmap,
      dri3_fence_trigger(draw->conn, dri3_front_buffer(draw));
      }
      }
      void
   loader_dri3_copy_drawable(struct loader_dri3_drawable *draw,
               {
               struct loader_dri3_buffer *front = dri3_front_buffer(draw);
   if (front)
            dri3_copy_area(draw->conn,
                        if (front) {
      dri3_fence_trigger(draw->conn, front);
         }
      void
   loader_dri3_wait_x(struct loader_dri3_drawable *draw)
   {
               if (draw == NULL || !draw->have_fake_front)
                              /* In the psc->is_different_gpu case, the linear buffer has been updated,
   * but not yet the tiled buffer.
   * Copy back to the tiled buffer we use for rendering.
   * Note that we don't need flushing.
   */
   if (draw->dri_screen_render_gpu != draw->dri_screen_display_gpu)
      (void) loader_dri3_blit_image(draw,
                     }
      void
   loader_dri3_wait_gl(struct loader_dri3_drawable *draw)
   {
               if (draw == NULL || !draw->have_fake_front)
            front = dri3_front_buffer(draw);
   /* TODO: `front` is not supposed to be NULL here, fix the actual bug
   * https://gitlab.freedesktop.org/mesa/mesa/-/issues/8982
   */
   if (!front)
            /* In the psc->is_different_gpu case, we update the linear_buffer
   * before updating the real front.
   */
   if (draw->dri_screen_render_gpu != draw->dri_screen_display_gpu)
      (void) loader_dri3_blit_image(draw,
                        loader_dri3_swapbuffer_barrier(draw);
      }
      /** dri3_flush_present_events
   *
   * Process any present events that have been received from the X server
   */
   static void
   dri3_flush_present_events(struct loader_dri3_drawable *draw)
   {
      /* Check to see if any configuration changes have occurred
   * since we were last invoked
   */
   if (draw->has_event_waiter)
            if (draw->special_event) {
               while ((ev = xcb_poll_for_special_event(draw->conn,
            xcb_present_generic_event_t *ge = (void *) ev;
   if (!dri3_handle_present_event(draw, ge))
            }
      /** loader_dri3_swap_buffers_msc
   *
   * Make the current back buffer visible using the present extension
   */
   int64_t
   loader_dri3_swap_buffers_msc(struct loader_dri3_drawable *draw,
                           {
      struct loader_dri3_buffer *back;
   int64_t ret = 0;
            /* GLX spec:
   *   void glXSwapBuffers(Display *dpy, GLXDrawable draw);
   *   This operation is a no-op if draw was created with a non-double-buffered
   *   GLXFBConfig, or if draw is a GLXPixmap.
   *   ...
   *   GLX pixmaps may be created with a config that includes back buffers and
   *   stereoscopic buffers. However, glXSwapBuffers is ignored for these pixmaps.
   *   ...
   *   It is possible to create a pbuffer with back buffers and to swap the
   *   front and back buffers by calling glXSwapBuffers.
   *
   * EGL spec:
   *   EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface);
   *   If surface is a back-buffered window surface, then the color buffer is
   *   copied to the native window associated with that surface. If surface is
   *   a single-buffered window, pixmap, or pbuffer surface, eglSwapBuffers has
   *   no effect.
   *
   * SwapBuffer effect:
   *       |           GLX             |           EGL            |
   *       | window | pixmap | pbuffer | window | pixmap | pbuffer|
   *-------+--------+--------+---------+--------+--------+--------+
   * single|  nop   |  nop   |   nop   |  nop   |  nop   |   nop  |
   * double|  swap  |  nop   |   swap  |  swap  |  NA    |   NA   |
   */
   if (!draw->have_back || draw->type == LOADER_DRI3_DRAWABLE_PIXMAP)
                     back = dri3_find_back_alloc(draw);
   /* Could only happen when error case, like display is already closed. */
   if (!back)
                     if (draw->adaptive_sync && !draw->adaptive_sync_active) {
      set_adaptive_sync_property(draw->conn, draw->drawable, true);
               if (draw->dri_screen_render_gpu != draw->dri_screen_display_gpu) {
      /* Update the linear buffer before presenting the pixmap */
   (void) loader_dri3_blit_image(draw,
                                 /* If we need to preload the new back buffer, remember the source.
   * The force_copy parameter is used by EGL to attempt to preserve
   * the back buffer across a call to this function.
   */
   if (force_copy)
            /* Exchange the back and fake front. Even though the server knows about these
   * buffers, it has no notion of back and fake front.
   */
   if (draw->have_fake_front) {
               tmp = dri3_front_buffer(draw);
   draw->buffers[LOADER_DRI3_FRONT_ID] = back;
            if (force_copy)
                        if (draw->type == LOADER_DRI3_DRAWABLE_WINDOW) {
               /* Compute when we want the frame shown by taking the last known
   * successful MSC and adding in a swap interval for each outstanding swap
   * request. target_msc=divisor=remainder=0 means "Use glXSwapBuffers()
   * semantic"
   */
   ++draw->send_sbc;
   if (target_msc == 0 && divisor == 0 && remainder == 0)
      target_msc = draw->msc + abs(draw->swap_interval) *
      else if (divisor == 0 && remainder > 0) {
      /* From the GLX_OML_sync_control spec:
   *     "If <divisor> = 0, the swap will occur when MSC becomes
   *      greater than or equal to <target_msc>."
   *
   * Note that there's no mention of the remainder.  The Present
   * extension throws BadValue for remainder != 0 with divisor == 0, so
   * just drop the passed in value.
   */
               /* From the GLX_EXT_swap_control spec
   * and the EGL 1.4 spec (page 53):
   *
   *     "If <interval> is set to a value of 0, buffer swaps are not
   *      synchronized to a video frame."
   *
   * From GLX_EXT_swap_control_tear:
   *
   *     "If <interval> is negative, the minimum number of video frames
   *      between buffer swaps is the absolute value of <interval>. In this
   *      case, if abs(<interval>) video frames have already passed from
   *      the previous swap when the swap is ready to be performed, the
   *      swap will occur without synchronization to a video frame."
   *
   * Implementation note: It is possible to enable triple buffering
   * behaviour by not using XCB_PRESENT_OPTION_ASYNC, but this should not be
   * the default.
   */
   uint32_t options = XCB_PRESENT_OPTION_NONE;
   if (draw->swap_interval <= 0)
            /* If we need to populate the new back, but need to reuse the back
   * buffer slot due to lack of local blit capabilities, make sure
   * the server doesn't flip and we deadlock.
   */
   if (!loader_dri3_have_image_blit(draw) && draw->cur_blit_source != -1)
   #ifdef HAVE_DRI3_MODIFIERS
         if (draw->multiplanes_available)
   #endif
         back->busy = 1;
            if (!draw->region) {
      draw->region = xcb_generate_id(draw->conn);
               xcb_xfixes_region_t region = 0;
            if (n_rects > 0 && n_rects <= ARRAY_SIZE(xcb_rects)) {
      for (int i = 0; i < n_rects; i++) {
      const int *rect = &rects[i * 4];
   xcb_rects[i].x = rect[0];
   xcb_rects[i].y = draw->height - rect[1] - rect[3];
   xcb_rects[i].width = rect[2];
               region = draw->region;
               xcb_present_pixmap(draw->conn,
                     draw->drawable,
   back->pixmap,
   (uint32_t) draw->send_sbc,
   0,                                    /* valid */
   region,                               /* update */
   0,                                    /* x_off */
   0,                                    /* y_off */
   None,                                 /* target_crtc */
   None,
   back->sync_fence,
      } else {
      /* This can only be reached by double buffered GLXPbuffer. */
   assert(draw->type == LOADER_DRI3_DRAWABLE_PBUFFER);
   /* GLX does not have damage regions. */
            /* For wait and buffer age usage. */
   draw->send_sbc++;
            /* Pixmap is imported as front buffer image when same GPU case, so just
   * locally blit back buffer image to it is enough. Otherwise front buffer
   * is a fake one which needs to be synced with pixmap by xserver remotely.
   */
   if (draw->dri_screen_render_gpu != draw->dri_screen_display_gpu ||
      !loader_dri3_blit_image(draw,
                           dri3_copy_area(draw->conn, back->pixmap,
                                       /* Schedule a server-side back-preserving blit if necessary.
   * This happens iff all conditions below are satisfied:
   * a) We have a fake front,
   * b) We need to preserve the back buffer,
   * c) We don't have local blit capabilities.
   */
   if (!loader_dri3_have_image_blit(draw) && draw->cur_blit_source != -1 &&
      draw->cur_blit_source != LOADER_DRI3_BACK_ID(draw->cur_back)) {
   struct loader_dri3_buffer *new_back = dri3_back_buffer(draw);
            dri3_fence_reset(draw->conn, new_back);
   dri3_copy_area(draw->conn, src->pixmap,
                     dri3_fence_trigger(draw->conn, new_back);
               xcb_flush(draw->conn);
   if (draw->stamp)
            /* Waiting on a buffer is only sensible if all buffers are in use and the
   * client doesn't use the buffer age extension. In this case a client is
   * relying on it receiving back control immediately.
   *
   * As waiting on a buffer can at worst make us miss a frame the option has
   * to be enabled explicitly with the block_on_depleted_buffers DRI option.
   */
   wait_for_next_buffer = draw->cur_num_back == draw->max_num_back &&
                              /* Clients that use up all available buffers usually regulate their drawing
   * through swapchain contention backpressure. In such a scenario the client
   * draws whenever control returns to it. Its event loop is slowed down only
   * by us waiting on buffers becoming available again.
   *
   * By waiting here on a new buffer and only then returning back to the client
   * we ensure the client begins drawing only when the next buffer is available
   * and not draw first and then wait a refresh cycle on the next available
   * buffer to show it. This way we can reduce the latency between what is
   * being drawn by the client and what is shown on the screen by one frame.
   */
   if (wait_for_next_buffer)
               }
      int
   loader_dri3_query_buffer_age(struct loader_dri3_drawable *draw)
   {
      struct loader_dri3_buffer *back = dri3_find_back_alloc(draw);
            mtx_lock(&draw->mtx);
   draw->queries_buffer_age = true;
   if (back && back->last_swap != 0)
                     }
      /** loader_dri3_open
   *
   * Wrapper around xcb_dri3_open
   */
   int
   loader_dri3_open(xcb_connection_t *conn,
               {
      xcb_dri3_open_cookie_t       cookie;
   xcb_dri3_open_reply_t        *reply;
   xcb_xfixes_query_version_cookie_t fixes_cookie;
   xcb_xfixes_query_version_reply_t *fixes_reply;
            cookie = xcb_dri3_open(conn,
                           if (!reply || reply->nfd != 1) {
      free(reply);
               fd = xcb_dri3_open_reply_fds(conn, reply)[0];
   free(reply);
            /* let the server know our xfixes level */
   fixes_cookie = xcb_xfixes_query_version(conn,
               fixes_reply = xcb_xfixes_query_version_reply(conn, fixes_cookie, NULL);
               }
      static uint32_t
   dri3_cpp_for_format(uint32_t format) {
      switch (format) {
   case  __DRI_IMAGE_FORMAT_R8:
         case  __DRI_IMAGE_FORMAT_RGB565:
   case  __DRI_IMAGE_FORMAT_GR88:
         case  __DRI_IMAGE_FORMAT_XRGB8888:
   case  __DRI_IMAGE_FORMAT_ARGB8888:
   case  __DRI_IMAGE_FORMAT_ABGR8888:
   case  __DRI_IMAGE_FORMAT_XBGR8888:
   case  __DRI_IMAGE_FORMAT_XRGB2101010:
   case  __DRI_IMAGE_FORMAT_ARGB2101010:
   case  __DRI_IMAGE_FORMAT_XBGR2101010:
   case  __DRI_IMAGE_FORMAT_ABGR2101010:
   case  __DRI_IMAGE_FORMAT_SARGB8:
   case  __DRI_IMAGE_FORMAT_SABGR8:
   case  __DRI_IMAGE_FORMAT_SXRGB8:
         case __DRI_IMAGE_FORMAT_ABGR16161616:
   case __DRI_IMAGE_FORMAT_XBGR16161616:
   case __DRI_IMAGE_FORMAT_XBGR16161616F:
   case __DRI_IMAGE_FORMAT_ABGR16161616F:
         case  __DRI_IMAGE_FORMAT_NONE:
   default:
            }
      /* Map format of render buffer to corresponding format for the linear_buffer
   * used for sharing with the display gpu of a Prime setup (== is_different_gpu).
   * Usually linear_format == format, except for depth >= 30 formats, where
   * different gpu vendors have different preferences wrt. color channel ordering.
   */
   static uint32_t
   dri3_linear_format_for_format(struct loader_dri3_drawable *draw, uint32_t format)
   {
      switch (format) {
      case  __DRI_IMAGE_FORMAT_XRGB2101010:
   case  __DRI_IMAGE_FORMAT_XBGR2101010:
      /* Different preferred formats for different hw */
   if (dri3_get_red_mask_for_depth(draw, 30) == 0x3ff)
                     case  __DRI_IMAGE_FORMAT_ARGB2101010:
   case  __DRI_IMAGE_FORMAT_ABGR2101010:
      /* Different preferred formats for different hw */
   if (dri3_get_red_mask_for_depth(draw, 30) == 0x3ff)
                     default:
         }
      #ifdef HAVE_DRI3_MODIFIERS
   static bool
   has_supported_modifier(struct loader_dri3_drawable *draw, unsigned int format,
         {
      uint64_t *supported_modifiers;
   int32_t supported_modifiers_count;
   bool found = false;
            if (!draw->ext->image->queryDmaBufModifiers(draw->dri_screen_render_gpu,
                  supported_modifiers_count == 0)
         supported_modifiers = malloc(supported_modifiers_count * sizeof(uint64_t));
   if (!supported_modifiers)
            draw->ext->image->queryDmaBufModifiers(draw->dri_screen_render_gpu, format,
                        for (i = 0; !found && i < supported_modifiers_count; i++) {
      for (j = 0; !found && j < count; j++) {
      if (supported_modifiers[i] == modifiers[j])
                  free(supported_modifiers);
      }
   #endif
      /** loader_dri3_alloc_render_buffer
   *
   * Use the driver createImage function to construct a __DRIimage, then
   * get a file descriptor for that and create an X pixmap from that
   *
   * Allocate an xshmfence for synchronization
   */
   static struct loader_dri3_buffer *
   dri3_alloc_render_buffer(struct loader_dri3_drawable *draw, unsigned int format,
         {
      struct loader_dri3_buffer *buffer;
   __DRIimage *pixmap_buffer = NULL, *linear_buffer_display_gpu = NULL;
   xcb_pixmap_t pixmap;
   xcb_sync_fence_t sync_fence;
   struct xshmfence *shm_fence;
   int buffer_fds[4], fence_fd;
   int num_planes = 0;
   uint64_t *modifiers = NULL;
   uint32_t count = 0;
   int i, mod;
            /* Create an xshmfence object and
   * prepare to send that to the X server
            fence_fd = xshmfence_alloc_shm();
   if (fence_fd < 0)
            shm_fence = xshmfence_map_shm(fence_fd);
   if (shm_fence == NULL)
            /* Allocate the image from the driver
   */
   buffer = calloc(1, sizeof *buffer);
   if (!buffer)
            buffer->cpp = dri3_cpp_for_format(format);
   if (!buffer->cpp)
               #ifdef HAVE_DRI3_MODIFIERS
         if (draw->multiplanes_available &&
      draw->ext->image->base.version >= 15 &&
   draw->ext->image->queryDmaBufModifiers &&
   draw->ext->image->createImageWithModifiers) {
   xcb_dri3_get_supported_modifiers_cookie_t mod_cookie;
                  mod_cookie = xcb_dri3_get_supported_modifiers(draw->conn,
               mod_reply = xcb_dri3_get_supported_modifiers_reply(draw->conn,
                              if (mod_reply->num_window_modifiers) {
      count = mod_reply->num_window_modifiers;
   modifiers = malloc(count * sizeof(uint64_t));
   if (!modifiers) {
                        memcpy(modifiers,
                  if (!has_supported_modifier(draw, loader_image_format_to_fourcc(format),
            free(modifiers);
   count = 0;
                  if (mod_reply->num_screen_modifiers && modifiers == NULL) {
      count = mod_reply->num_screen_modifiers;
   modifiers = malloc(count * sizeof(uint64_t));
   if (!modifiers) {
                        memcpy(modifiers,
                        #endif
         buffer->image = loader_dri_create_image(draw->dri_screen_render_gpu, draw->ext->image,
                                                               if (!buffer->image)
      } else {
      buffer->image = draw->ext->image->createImage(draw->dri_screen_render_gpu,
                              if (!buffer->image)
            /* if driver name is same only then dri_screen_display_gpu is set.
   * This check is needed because for simplicity render gpu image extension
   * is also used for display gpu.
   */
   if (draw->dri_screen_display_gpu) {
      linear_buffer_display_gpu =
   draw->ext->image->createImage(draw->dri_screen_display_gpu,
                                 width, height,
   dri3_linear_format_for_format(draw, format),
               if (!pixmap_buffer) {
      buffer->linear_buffer =
   draw->ext->image->createImage(draw->dri_screen_render_gpu,
                                 width, height,
                  pixmap_buffer = buffer->linear_buffer;
   if (!buffer->linear_buffer) {
                        /* X want some information about the planes, so ask the image for it
   */
   if (!draw->ext->image->queryImage(pixmap_buffer, __DRI_IMAGE_ATTRIB_NUM_PLANES,
                  for (i = 0; i < num_planes; i++) {
               if (!image) {
      assert(i == 0);
                        ret = draw->ext->image->queryImage(image, __DRI_IMAGE_ATTRIB_FD,
         ret &= draw->ext->image->queryImage(image, __DRI_IMAGE_ATTRIB_STRIDE,
         ret &= draw->ext->image->queryImage(image, __DRI_IMAGE_ATTRIB_OFFSET,
         if (image != pixmap_buffer)
            if (!ret)
               ret = draw->ext->image->queryImage(pixmap_buffer,
         buffer->modifier = (uint64_t) mod << 32;
   ret &= draw->ext->image->queryImage(pixmap_buffer,
                  if (!ret)
            if (draw->dri_screen_render_gpu != draw->dri_screen_display_gpu &&
      draw->dri_screen_display_gpu && linear_buffer_display_gpu) {
   /* The linear buffer was created in the display GPU's vram, so we
   * need to make it visible to render GPU
   */
   if (draw->ext->image->base.version >= 20)
      buffer->linear_buffer =
      draw->ext->image->createImageFromFds2(draw->dri_screen_render_gpu,
                                       width,
   else
      buffer->linear_buffer =
      draw->ext->image->createImageFromFds(draw->dri_screen_render_gpu,
                                       if (!buffer->linear_buffer)
                           #ifdef HAVE_DRI3_MODIFIERS
      if (draw->multiplanes_available &&
      buffer->modifier != DRM_FORMAT_MOD_INVALID) {
   xcb_dri3_pixmap_from_buffers(draw->conn,
                              pixmap,
   draw->window,
   num_planes,
   width, height,
   buffer->strides[0], buffer->offsets[0],
   buffer->strides[1], buffer->offsets[1],
      #endif
      {
      xcb_dri3_pixmap_from_buffer(draw->conn,
                              pixmap,
            xcb_dri3_fence_from_fd(draw->conn,
                              buffer->pixmap = pixmap;
   buffer->own_pixmap = true;
   buffer->sync_fence = sync_fence;
   buffer->shm_fence = shm_fence;
   buffer->width = width;
            /* Mark the buffer as idle
   */
                  no_buffer_attrib:
      do {
      if (buffer_fds[i] != -1)
      } while (--i >= 0);
      no_linear_buffer:
      if (draw->dri_screen_render_gpu != draw->dri_screen_display_gpu)
      no_image:
         no_buffer:
         no_shm_fence:
      close(fence_fd);
      }
      static bool
   dri3_detect_drawable_is_window(struct loader_dri3_drawable *draw)
   {
      /* Try to select for input on the window.
   *
   * If the drawable is a window, this will get our events
   * delivered.
   *
   * Otherwise, we'll get a BadWindow error back from this request which
   * will let us know that the drawable is a pixmap instead.
            xcb_void_cookie_t cookie =
      xcb_present_select_input_checked(draw->conn, draw->eid, draw->drawable,
                     /* Check to see if our select input call failed. If it failed with a
   * BadWindow error, then assume the drawable is a pixmap.
   */
            if (error) {
      if (error->error_code != BadWindow) {
      free(error);
      }
            /* pixmap can't get here, see driFetchDrawable(). */
   draw->type = LOADER_DRI3_DRAWABLE_PBUFFER;
               draw->type = LOADER_DRI3_DRAWABLE_WINDOW;
      }
      static bool
   dri3_setup_present_event(struct loader_dri3_drawable *draw)
   {
      /* No need to setup for pixmap drawable. */
   if (draw->type == LOADER_DRI3_DRAWABLE_PIXMAP ||
      draw->type == LOADER_DRI3_DRAWABLE_PBUFFER)
                  if (draw->type == LOADER_DRI3_DRAWABLE_WINDOW) {
      xcb_present_select_input(draw->conn, draw->eid, draw->drawable,
                  } else {
               if (!dri3_detect_drawable_is_window(draw))
            if (draw->type != LOADER_DRI3_DRAWABLE_WINDOW)
               /* Create an XCB event queue to hold present events outside of the usual
   * application event queue
   */
   draw->special_event = xcb_register_for_special_xge(draw->conn,
                        }
      /** loader_dri3_update_drawable
   *
   * Called the first time we use the drawable and then
   * after we receive present configure notify events to
   * track the geometry of the drawable
   */
   static int
   dri3_update_drawable(struct loader_dri3_drawable *draw)
   {
      mtx_lock(&draw->mtx);
   if (draw->first_init) {
      xcb_get_geometry_cookie_t                 geom_cookie;
   xcb_get_geometry_reply_t                  *geom_reply;
                     if (!dri3_setup_present_event(draw)) {
      mtx_unlock(&draw->mtx);
                                 if (!geom_reply) {
      mtx_unlock(&draw->mtx);
      }
   draw->width = geom_reply->width;
   draw->height = geom_reply->height;
   draw->depth = geom_reply->depth;
   draw->vtable->set_drawable_size(draw, draw->width, draw->height);
                     if (draw->type != LOADER_DRI3_DRAWABLE_WINDOW)
         else
      }
   dri3_flush_present_events(draw);
   mtx_unlock(&draw->mtx);
      }
      __DRIimage *
   loader_dri3_create_image(xcb_connection_t *c,
                           xcb_dri3_buffer_from_pixmap_reply_t *bp_reply,
   {
      int                                  *fds;
   __DRIimage                           *image_planar, *ret;
            /* Get an FD for the pixmap object
   */
            stride = bp_reply->stride;
            /* createImageFromFds creates a wrapper __DRIimage structure which
   * can deal with multiple planes for things like Yuv images. So, once
   * we've gotten the planar wrapper, pull the single plane out of it and
   * discard the wrapper.
   */
   image_planar = image->createImageFromFds(dri_screen,
                                 close(fds[0]);
   if (!image_planar)
                     if (!ret)
         else
               }
      #ifdef HAVE_DRI3_MODIFIERS
   __DRIimage *
   loader_dri3_create_image_from_buffers(xcb_connection_t *c,
                                 {
      __DRIimage                           *ret;
   int                                  *fds;
   uint32_t                             *strides_in, *offsets_in;
   int                                   strides[4], offsets[4];
   unsigned                              error;
            if (bp_reply->nfd > 4)
            fds = xcb_dri3_buffers_from_pixmap_reply_fds(c, bp_reply);
   strides_in = xcb_dri3_buffers_from_pixmap_strides(bp_reply);
   offsets_in = xcb_dri3_buffers_from_pixmap_offsets(bp_reply);
   for (i = 0; i < bp_reply->nfd; i++) {
      strides[i] = strides_in[i];
               ret = image->createImageFromDmaBufs2(dri_screen,
                                       bp_reply->width,
            for (i = 0; i < bp_reply->nfd; i++)
               }
   #endif
      /** dri3_get_pixmap_buffer
   *
   * Get the DRM object for a pixmap from the X server and
   * wrap that with a __DRIimage structure using createImageFromFds
   */
   static struct loader_dri3_buffer *
   dri3_get_pixmap_buffer(__DRIdrawable *driDrawable, unsigned int format,
               {
      int                                  buf_id = loader_dri3_pixmap_buf_id(buffer_type);
   struct loader_dri3_buffer            *buffer = draw->buffers[buf_id];
   xcb_drawable_t                       pixmap;
   xcb_sync_fence_t                     sync_fence;
   struct xshmfence                     *shm_fence;
   int                                  width;
   int                                  height;
   int                                  fence_fd;
            if (buffer)
                     buffer = calloc(1, sizeof *buffer);
   if (!buffer)
            fence_fd = xshmfence_alloc_shm();
   if (fence_fd < 0)
         shm_fence = xshmfence_map_shm(fence_fd);
   if (shm_fence == NULL) {
      close (fence_fd);
               /* Get the currently-bound screen or revert to using the drawable's screen if
   * no contexts are currently bound. The latter case is at least necessary for
   * obs-studio, when using Window Capture (Xcomposite) as a Source.
   */
   cur_screen = draw->vtable->get_dri_screen();
   if (!cur_screen) {
                  xcb_dri3_fence_from_fd(draw->conn,
                        #ifdef HAVE_DRI3_MODIFIERS
      if (draw->multiplanes_available &&
      draw->ext->image->base.version >= 15 &&
   draw->ext->image->createImageFromDmaBufs2) {
   xcb_dri3_buffers_from_pixmap_cookie_t bps_cookie;
            bps_cookie = xcb_dri3_buffers_from_pixmap(draw->conn, pixmap);
   bps_reply = xcb_dri3_buffers_from_pixmap_reply(draw->conn, bps_cookie,
         if (!bps_reply)
         buffer->image =
      loader_dri3_create_image_from_buffers(draw->conn, bps_reply, format,
            width = bps_reply->width;
   height = bps_reply->height;
         #endif
      {
      xcb_dri3_buffer_from_pixmap_cookie_t bp_cookie;
            bp_cookie = xcb_dri3_buffer_from_pixmap(draw->conn, pixmap);
   bp_reply = xcb_dri3_buffer_from_pixmap_reply(draw->conn, bp_cookie, NULL);
   if (!bp_reply)
            buffer->image = loader_dri3_create_image(draw->conn, bp_reply, format,
               width = bp_reply->width;
   height = bp_reply->height;
               if (!buffer->image)
            buffer->pixmap = pixmap;
   buffer->own_pixmap = false;
   buffer->width = width;
   buffer->height = height;
   buffer->shm_fence = shm_fence;
                           no_image:
      xcb_sync_destroy_fence(draw->conn, sync_fence);
      no_fence:
         no_buffer:
         }
      /** dri3_get_buffer
   *
   * Find a front or back buffer, allocating new ones as necessary
   */
   static struct loader_dri3_buffer *
   dri3_get_buffer(__DRIdrawable *driDrawable,
                     {
      struct loader_dri3_buffer *buffer;
   bool fence_await = buffer_type == loader_dri3_buffer_back;
            if (buffer_type == loader_dri3_buffer_back) {
                        if (buf_id < 0)
      } else {
                           /* Allocate a new buffer if there isn't an old one, if that
   * old one is the wrong size, or if it's suboptimal
   */
   if (!buffer || buffer->width != draw->width ||
      buffer->height != draw->height ||
   buffer->reallocate) {
            /* Allocate the new buffers
   */
   new_buffer = dri3_alloc_render_buffer(draw,
                           if (!new_buffer)
            /* When resizing, copy the contents of the old buffer, waiting for that
   * copy to complete using our fences before proceeding
   */
   if ((buffer_type == loader_dri3_buffer_back ||
                     /* Fill the new buffer with data from an old buffer */
   if (!loader_dri3_blit_image(draw,
                              new_buffer->image,
   buffer->image,
   !buffer->linear_buffer) {
   dri3_fence_reset(draw->conn, new_buffer);
   dri3_copy_area(draw->conn,
                  buffer->pixmap,
   new_buffer->pixmap,
      dri3_fence_trigger(draw->conn, new_buffer);
      }
      } else if (buffer_type == loader_dri3_buffer_front) {
      /* Fill the new fake front with data from a real front */
   loader_dri3_swapbuffer_barrier(draw);
   dri3_fence_reset(draw->conn, new_buffer);
   dri3_copy_area(draw->conn,
                  draw->drawable,
   new_buffer->pixmap,
               if (new_buffer->linear_buffer) {
      dri3_fence_await(draw->conn, draw, new_buffer);
   (void) loader_dri3_blit_image(draw,
                        } else
      }
   buffer = new_buffer;
               if (fence_await)
            /*
   * Do we need to preserve the content of a previous buffer?
   *
   * Note that this blit is needed only to avoid a wait for a buffer that
   * is currently in the flip chain or being scanned out from. That's really
   * a tradeoff. If we're ok with the wait we can reduce the number of back
   * buffers to 1 for SWAP_EXCHANGE, and 1 for SWAP_COPY,
   * but in the latter case we must disallow page-flipping.
   */
   if (buffer_type == loader_dri3_buffer_back &&
      draw->cur_blit_source != -1 &&
   draw->buffers[draw->cur_blit_source] &&
                     /* Avoid flushing here. Will propably do good for tiling hardware. */
   (void) loader_dri3_blit_image(draw,
                           buffer->last_swap = source->last_swap;
      }
   /* Return the requested buffer */
      }
      /** dri3_free_buffers
   *
   * Free the front bufffer or all of the back buffers. Used
   * when the application changes which buffers it needs
   */
   static void
   dri3_free_buffers(__DRIdrawable *driDrawable,
               {
      int first_id;
   int n_id;
            switch (buffer_type) {
   case loader_dri3_buffer_back:
      first_id = LOADER_DRI3_BACK_ID(0);
   n_id = LOADER_DRI3_MAX_BACK;
   draw->cur_blit_source = -1;
      case loader_dri3_buffer_front:
      first_id = LOADER_DRI3_FRONT_ID;
   /* Don't free a fake front holding new backbuffer content. */
   n_id = (draw->cur_blit_source == LOADER_DRI3_FRONT_ID) ? 0 : 1;
      default:
                  for (buf_id = first_id; buf_id < first_id + n_id; buf_id++)
      }
      /** loader_dri3_get_buffers
   *
   * The published buffer allocation API.
   * Returns all of the necessary buffers, allocating
   * as needed.
   */
   int
   loader_dri3_get_buffers(__DRIdrawable *driDrawable,
                           unsigned int format,
   {
      struct loader_dri3_drawable *draw = loaderPrivate;
   struct loader_dri3_buffer   *front, *back;
            buffers->image_mask = 0;
   buffers->front = NULL;
            if (!dri3_update_drawable(draw))
                     /* Free no longer needed back buffers */
   for (buf_id = 0; buf_id < LOADER_DRI3_MAX_BACK; buf_id++) {
               back = draw->buffers[buf_id];
   if (!back || !back->last_swap || draw->cur_blit_source == buf_id)
            buffer_age = draw->send_sbc - back->last_swap + 1;
   if (buffer_age > 200)
               /* pixmaps always have front buffers.
   */
   if (draw->type != LOADER_DRI3_DRAWABLE_WINDOW)
            if (buffer_mask & __DRI_IMAGE_BUFFER_FRONT) {
      /* All pixmaps are owned by the server gpu.
   * When we use a different gpu, we can't use the pixmap
   * as buffer since it is potentially tiled a way
   * our device can't understand. In this case, use
   * a fake front buffer. Hopefully the pixmap
   * content will get synced with the fake front
   * buffer.
   */
   if (draw->type != LOADER_DRI3_DRAWABLE_WINDOW &&
      draw->dri_screen_render_gpu == draw->dri_screen_display_gpu)
   front = dri3_get_pixmap_buffer(driDrawable,
                  else
      front = dri3_get_buffer(driDrawable,
                     if (!front)
      } else {
      dri3_free_buffers(driDrawable, loader_dri3_buffer_front, draw);
   draw->have_fake_front = 0;
               if (buffer_mask & __DRI_IMAGE_BUFFER_BACK) {
      back = dri3_get_buffer(driDrawable,
                     if (!back)
            } else {
      dri3_free_buffers(driDrawable, loader_dri3_buffer_back, draw);
   draw->have_back = 0;
               if (front) {
      buffers->image_mask |= __DRI_IMAGE_BUFFER_FRONT;
   buffers->front = front->image;
   draw->have_fake_front =
      draw->dri_screen_render_gpu != draw->dri_screen_display_gpu ||
            if (back) {
      buffers->image_mask |= __DRI_IMAGE_BUFFER_BACK;
                           }
      /** loader_dri3_update_drawable_geometry
   *
   * Get the current drawable geometry.
   */
   void
   loader_dri3_update_drawable_geometry(struct loader_dri3_drawable *draw)
   {
      xcb_get_geometry_cookie_t geom_cookie;
                              if (geom_reply) {
      bool changed = draw->width != geom_reply->width || draw->height != geom_reply->height;
   draw->width = geom_reply->width;
   draw->height = geom_reply->height;
   if (changed) {
      draw->vtable->set_drawable_size(draw, draw->width, draw->height);
                     }
      /**
   * Make sure the server has flushed all pending swap buffers to hardware
   * for this drawable. Ideally we'd want to send an X protocol request to
   * have the server block our connection until the swaps are complete. That
   * would avoid the potential round-trip here.
   */
   void
   loader_dri3_swapbuffer_barrier(struct loader_dri3_drawable *draw)
   {
                  }
      /**
   * Perform any cleanup associated with a close screen operation.
   * \param dri_screen[in,out] Pointer to __DRIscreen about to be closed.
   *
   * This function destroys the screen's cached swap context if any.
   */
   void
   loader_dri3_close_screen(__DRIscreen *dri_screen)
   {
      simple_mtx_lock(&blit_context.mtx);
   if (blit_context.ctx && blit_context.cur_screen == dri_screen) {
      blit_context.core->destroyContext(blit_context.ctx);
      }
      }
      /**
   * Find a backbuffer slot - potentially allocating a back buffer
   *
   * \param draw[in,out]  Pointer to the drawable for which to find back.
   * \return Pointer to a new back buffer or NULL if allocation failed or was
   * not mandated.
   *
   * Find a potentially new back buffer, and if it's not been allocated yet and
   * in addition needs initializing, then try to allocate and initialize it.
   */
   #include <stdio.h>
   static struct loader_dri3_buffer *
   dri3_find_back_alloc(struct loader_dri3_drawable *draw)
   {
      struct loader_dri3_buffer *back;
            id = dri3_find_back(draw, false);
   if (id < 0)
            back = draw->buffers[id];
   /* Allocate a new back if we haven't got one */
   if (!back && draw->back_format != __DRI_IMAGE_FORMAT_NONE &&
      dri3_update_drawable(draw))
   back = dri3_alloc_render_buffer(draw, draw->back_format,
         if (!back)
                     /* If necessary, prefill the back with data. */
   if (draw->cur_blit_source != -1 &&
      draw->buffers[draw->cur_blit_source] &&
   back != draw->buffers[draw->cur_blit_source]) {
            dri3_fence_await(draw->conn, draw, source);
   dri3_fence_await(draw->conn, draw, back);
   (void) loader_dri3_blit_image(draw,
                           back->last_swap = source->last_swap;
                  }
