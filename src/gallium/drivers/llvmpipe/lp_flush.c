   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
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
      /* Author:
   *    Keith Whitwell <keithw@vmware.com>
   */
         #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "util/u_debug_image.h"
   #include "util/u_string.h"
   #include "draw/draw_context.h"
   #include "lp_flush.h"
   #include "lp_context.h"
   #include "lp_setup.h"
   #include "lp_fence.h"
   #include "lp_screen.h"
   #include "lp_rast.h"
         /**
   * \param fence  if non-null, returns pointer to a fence which can be waited on
   */
   void
   llvmpipe_flush(struct pipe_context *pipe,
               {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
                     /* ask the setup module to flush */
            mtx_lock(&screen->rast_mutex);
   lp_rast_fence(screen->rast, (struct lp_fence **)fence);
            if (fence && (!*fence))
            /* Enable to dump BMPs of the color/depth buffers each frame */
   if (0) {
      static unsigned frame_no = 1;
   char filename[256];
            for (i = 0; i < llvmpipe->framebuffer.nr_cbufs; i++) {
      snprintf(filename, sizeof(filename), "cbuf%u_%u", i, frame_no);
               if (0) {
      snprintf(filename, sizeof(filename), "zsbuf_%u", frame_no);
                     }
         void
   llvmpipe_finish(struct pipe_context *pipe,
         {
      struct pipe_fence_handle *fence = NULL;
   llvmpipe_flush(pipe, &fence, reason);
   if (fence) {
      pipe->screen->fence_finish(pipe->screen, NULL, fence,
               }
         /**
   * Flush context if necessary.
   *
   * Returns FALSE if it would have block, but do_not_block was set, TRUE
   * otherwise.
   *
   * TODO: move this logic to an auxiliary library?
   */
   bool
   llvmpipe_flush_resource(struct pipe_context *pipe,
                           struct pipe_resource *resource,
   unsigned level,
   {
      unsigned referenced = 0;
            mtx_lock(&lp_screen->ctx_mutex);
   list_for_each_entry(struct llvmpipe_context, ctx, &lp_screen->ctx_list, list) {
      referenced |=
      llvmpipe_is_resource_referenced((struct pipe_context *)ctx,
   }
            if ((referenced & LP_REFERENCED_FOR_WRITE) ||
               if (cpu_access)
                  /*
   * Flush and wait.
   * Finish so VS can use FS results.
   */
                  }
