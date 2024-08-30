   /*
   * Copyright © 2019 Red Hat
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      /* connect to an X server and work out the default device. */
      #include <xcb/xcb.h>
   #include <xcb/dri3.h>
   #include <unistd.h>
   #include <stdlib.h>
   #include <fcntl.h>
   #include <xf86drm.h>
      #include "device_select.h"
   static int
   ds_dri3_open(xcb_connection_t *conn,
         xcb_window_t root,
   {
      xcb_dri3_open_cookie_t       cookie;
   xcb_dri3_open_reply_t        *reply;
            cookie = xcb_dri3_open(conn,
                  reply = xcb_dri3_open_reply(conn, cookie, NULL);
   if (!reply)
            if (reply->nfd != 1) {
      free(reply);
               fd = xcb_dri3_open_reply_fds(conn, reply)[0];
   free(reply);
               }
      int device_select_find_xcb_pci_default(struct device_pci_info *devices, uint32_t device_count)
   {
   const xcb_setup_t *setup;
   xcb_screen_iterator_t iter;
   int scrn;
   xcb_connection_t *conn;
   int default_idx = -1;
   drmDevicePtr xdev = NULL;
      conn = xcb_connect(NULL, &scrn);
   if (!conn)
            xcb_query_extension_cookie_t dri3_cookie;
   xcb_query_extension_reply_t *dri3_reply = NULL;
      dri3_cookie = xcb_query_extension(conn, 4, "DRI3");
   dri3_reply = xcb_query_extension_reply(conn, dri3_cookie, NULL);
      if (!dri3_reply)
            if (dri3_reply->present == 0)
            setup = xcb_get_setup(conn);
   iter = xcb_setup_roots_iterator(setup);
      xcb_screen_t *screen = iter.data;
      int dri3_fd = ds_dri3_open(conn, screen->root, 0);
   if (dri3_fd == -1)
            int ret = drmGetDevice2(dri3_fd, 0, &xdev);
   close(dri3_fd);
   if (ret < 0)
            for (unsigned i = 0; i < device_count; i++) {
      if (devices[i].has_bus_info) {
         xdev->businfo.pci->bus == devices[i].bus_info.bus &&
   xdev->businfo.pci->dev == devices[i].bus_info.dev &&
      default_idx = i;
            } else {
            default_idx = i;
      }
   if (default_idx != -1)
      }
      out:
   free(dri3_reply);
   drmFreeDevice(&xdev); /* Is NULL pointer safe. */
   xcb_disconnect(conn);
   return default_idx;
   }
