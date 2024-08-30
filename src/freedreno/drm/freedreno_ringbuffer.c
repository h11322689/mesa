   /*
   * Copyright (C) 2012-2018 Rob Clark <robclark@freedesktop.org>
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include <assert.h>
      #include "freedreno_drmif.h"
   #include "freedreno_priv.h"
   #include "freedreno_ringbuffer.h"
      struct fd_submit *
   fd_submit_new(struct fd_pipe *pipe)
   {
      struct fd_submit *submit = pipe->funcs->submit_new(pipe);
   submit->refcnt = 1;
   submit->pipe = fd_pipe_ref(pipe);
      }
      void
   fd_submit_del(struct fd_submit *submit)
   {
      if (!unref(&submit->refcnt))
            if (submit->primary)
                        }
      struct fd_submit *
   fd_submit_ref(struct fd_submit *submit)
   {
      ref(&submit->refcnt);
      }
      struct fd_fence *
   fd_submit_flush(struct fd_submit *submit, int in_fence_fd, bool use_fence_fd)
   {
      submit->fence = fd_pipe_emit_fence(submit->pipe, submit->primary);
      }
      struct fd_ringbuffer *
   fd_submit_new_ringbuffer(struct fd_submit *submit, uint32_t size,
         {
      assert(!(flags & _FD_RINGBUFFER_OBJECT));
   if (flags & FD_RINGBUFFER_STREAMING) {
      assert(!(flags & FD_RINGBUFFER_GROWABLE));
      }
   struct fd_ringbuffer *ring =
            if (flags & FD_RINGBUFFER_PRIMARY) {
      assert(!submit->primary);
                  }
      struct fd_ringbuffer *
   fd_ringbuffer_new_object(struct fd_pipe *pipe, uint32_t size)
   {
         }
