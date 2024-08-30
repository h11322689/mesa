   /*
   * Copyright © Microsoft Corporation
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "pipe-loader/pipe_loader.h"
   #include "pipe/p_screen.h"
      #include "util/u_memory.h"
   #include "vl/vl_winsys.h"
   #include <X11/Xlib.h>
   #include <X11/Xutil.h>
      #include "gallium/include/frontend/xlibsw_api.h"
   #include "gallium/winsys/sw/xlib/xlib_sw_winsys.h"
   #include "loader.h"
   #include "target-helpers/sw_helper_public.h"
   #include "vl/vl_compositor.h"
      struct vl_xlib_screen
   {
      struct vl_screen base;
   struct pipe_context *pipe;
   Display *display;
   int screen;
   struct u_rect dirty_area;
   XVisualInfo xlib_drawable_visual_info;
   struct xlib_drawable xlib_drawable_handle;
      };
      static void
   vl_screen_destroy(struct vl_screen *vscreen)
   {
      if (vscreen == NULL)
            if (vscreen->pscreen)
            if (vscreen->dev)
               }
      struct pipe_resource *
   vl_swrast_texture_from_drawable(struct vl_screen *vscreen, void *drawable);
      struct u_rect *
   vl_swrast_get_dirty_area(struct vl_screen *vscreen);
      void *
   vl_swrast_get_private(struct vl_screen *vscreen);
      static void
   vl_xlib_screen_destroy(struct vl_screen *vscreen)
   {
      if (vscreen == NULL)
                     if (xlib_screen->drawable_texture)
            if (xlib_screen->pipe)
               }
      struct vl_screen *
   vl_xlib_swrast_screen_create(Display *display, int screen)
   {
      struct vl_xlib_screen *vscreen = CALLOC_STRUCT(vl_xlib_screen);
   if (!vscreen)
            struct sw_winsys *xlib_winsys = xlib_create_sw_winsys(display);
   if (xlib_winsys == NULL)
            vscreen->base.pscreen = sw_screen_create(xlib_winsys);
   if (!vscreen->base.pscreen)
            vscreen->base.get_private = vl_swrast_get_private;
   vscreen->base.texture_from_drawable = vl_swrast_texture_from_drawable;
   vscreen->base.get_dirty_area = vl_swrast_get_dirty_area;
   vscreen->base.destroy = vl_xlib_screen_destroy;
            vl_compositor_reset_dirty_area(&vscreen->dirty_area);
   vscreen->display = display;
                  handle_err_xlib_swrast_create:
      if (vscreen)
               }
      void
   vl_swrast_fill_xlib_drawable_desc(struct vl_screen *vscreen,
                  void
   vl_swrast_fill_xlib_drawable_desc(struct vl_screen *vscreen,
               {
      struct vl_xlib_screen *scrn = (struct vl_xlib_screen *) vscreen;
   XWindowAttributes x11_window_attrs = {};
   XGetWindowAttributes(scrn->display, x11_window, &x11_window_attrs);
   XMatchVisualInfo(scrn->display, scrn->screen, x11_window_attrs.depth, TrueColor, &scrn->xlib_drawable_visual_info);
   scrn->xlib_drawable_handle.depth = x11_window_attrs.depth;
   scrn->xlib_drawable_handle.drawable = x11_window;
      }
      struct pipe_resource *
   vl_swrast_texture_from_drawable(struct vl_screen *vscreen, void *drawable)
   {
      struct vl_xlib_screen *scrn = (struct vl_xlib_screen *) vscreen;
   Window x11_window = (Window) drawable;
            XWindowAttributes x11_window_attrs = {};
   XGetWindowAttributes(scrn->display, x11_window, &x11_window_attrs);
            bool needs_new_back_buffer_allocation = true;
   if (scrn->drawable_texture) {
      needs_new_back_buffer_allocation =
      (scrn->drawable_texture->width0 != x11_window_attrs.width || scrn->drawable_texture->height0 != x11_window_attrs.height ||
            if (needs_new_back_buffer_allocation) {
      if (scrn->drawable_texture)
            struct pipe_resource templat;
   memset(&templat, 0, sizeof(templat));
   templat.target = PIPE_TEXTURE_2D;
   templat.format = x11_window_format;
   templat.width0 = x11_window_attrs.width;
   templat.height0 = x11_window_attrs.height;
   templat.depth0 = 1;
   templat.array_size = 1;
   templat.last_level = 0;
               } else {
      struct pipe_resource *drawable_texture = NULL;
                  }
      void *
   vl_swrast_get_private(struct vl_screen *vscreen)
   {
      struct vl_xlib_screen *scrn = (struct vl_xlib_screen *) vscreen;
      }
      struct u_rect *
   vl_swrast_get_dirty_area(struct vl_screen *vscreen)
   {
      struct vl_xlib_screen *scrn = (struct vl_xlib_screen *) vscreen;
   vl_compositor_reset_dirty_area(&scrn->dirty_area);
      }
