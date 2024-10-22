   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "pipe/p_defines.h"
   #include "util/u_debug_image.h"
   #include "util/u_string.h"
   #include "svga_screen.h"
   #include "svga_surface.h"
   #include "svga_context.h"
   #include "svga_debug.h"
         static void svga_flush( struct pipe_context *pipe,
               {
               /* Emit buffered drawing commands, and any back copies.
   */
            if (flags & PIPE_FLUSH_FENCE_FD)
            /* Flush command queue.
   */
            SVGA_DBG(DEBUG_DMA|DEBUG_PERF, "%s fence_ptr %p\n",
            /* Enable to dump BMPs of the color/depth buffers each frame */
   if (0) {
      struct pipe_framebuffer_state *fb = &svga->curr.framebuffer;
   static unsigned frame_no = 1;
   char filename[256];
            for (i = 0; i < fb->nr_cbufs; i++) {
      snprintf(filename, sizeof(filename), "cbuf%u_%04u.bmp", i, frame_no);
               if (0 && fb->zsbuf) {
      snprintf(filename, sizeof(filename), "zsbuf_%04u.bmp", frame_no);
                     }
         /**
   * svga_create_fence_fd
   *
   * Wraps a SVGA fence around an imported file descriptor.  This
   * fd represents a fence from another process/device.  The fence created
   * here can then be fed into fence_server_sync() so SVGA can synchronize
   * with an external process
   */
   static void
   svga_create_fence_fd(struct pipe_context *pipe,
                     {
               assert(type == PIPE_FD_TYPE_NATIVE_SYNC);
      }
         /**
   * svga_fence_server_sync
   *
   * This function imports a fence from another process/device into the current
   * software context so that SVGA can synchronize with it.
   */
   static void
   svga_fence_server_sync(struct pipe_context *pipe,
         {
      struct svga_winsys_screen *sws = svga_winsys_screen(pipe->screen);
               }
         void svga_init_flush_functions( struct svga_context *svga )
   {
      svga->pipe.flush = svga_flush;
   svga->pipe.create_fence_fd = svga_create_fence_fd;
      }
