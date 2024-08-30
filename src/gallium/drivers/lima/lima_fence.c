   /*
   * Copyright (c) 2018-2019 Lima Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   */
      #include <fcntl.h>
   #include <libsync.h>
      #include "util/os_file.h"
   #include <util/u_memory.h>
   #include <util/u_inlines.h>
      #include "drm-uapi/lima_drm.h"
      #include "lima_screen.h"
   #include "lima_context.h"
   #include "lima_fence.h"
   #include "lima_job.h"
      struct pipe_fence_handle {
      struct pipe_reference reference;
      };
      static void
   lima_create_fence_fd(struct pipe_context *pctx,
               {
      assert(type == PIPE_FD_TYPE_NATIVE_SYNC);
      }
      static void
   lima_fence_server_sync(struct pipe_context *pctx,
         {
                  }
      void lima_fence_context_init(struct lima_context *ctx)
   {
      ctx->base.create_fence_fd = lima_create_fence_fd;
      }
      struct pipe_fence_handle *
   lima_fence_create(int fd)
   {
               fence = CALLOC_STRUCT(pipe_fence_handle);
   if (!fence)
            pipe_reference_init(&fence->reference, 1);
               }
      static int
   lima_fence_get_fd(struct pipe_screen *pscreen,
         {
         }
      static void
   lima_fence_destroy(struct pipe_fence_handle *fence)
   {
      if (fence->fd >= 0)
            }
      static void
   lima_fence_reference(struct pipe_screen *pscreen,
               {
      if (pipe_reference(&(*ptr)->reference, &fence->reference))
            }
      static bool
   lima_fence_finish(struct pipe_screen *pscreen, struct pipe_context *pctx,
         {
         }
      void
   lima_fence_screen_init(struct lima_screen *screen)
   {
      screen->base.fence_reference = lima_fence_reference;
   screen->base.fence_finish = lima_fence_finish;
      }
