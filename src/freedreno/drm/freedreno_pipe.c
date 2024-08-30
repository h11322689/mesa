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
      #include "freedreno_drmif.h"
   #include "freedreno_priv.h"
      /**
   * priority of zero is highest priority, and higher numeric values are
   * lower priorities
   */
   struct fd_pipe *
   fd_pipe_new2(struct fd_device *dev, enum fd_pipe_id id, uint32_t prio)
   {
      struct fd_pipe *pipe;
            if (id > FD_PIPE_MAX) {
      ERROR_MSG("invalid pipe id: %d", id);
               if ((prio != 1) && (fd_device_version(dev) < FD_VERSION_SUBMIT_QUEUES)) {
      ERROR_MSG("invalid priority!");
               pipe = dev->funcs->pipe_new(dev, id, prio);
   if (!pipe) {
      ERROR_MSG("allocation failed");
               pipe->dev = dev;
   pipe->id = id;
            fd_pipe_get_param(pipe, FD_GPU_ID, &val);
            fd_pipe_get_param(pipe, FD_CHIP_ID, &val);
                     /* Use the _NOSYNC flags because we don't want the control_mem bo to hold
   * a reference to the ourself.  This also means that we won't be able
   * to determine if the buffer is idle which is needed by bo-cache.  But
   * pipe creation/destroy is not a high frequency event.
   */
   pipe->control_mem = fd_bo_new(dev, sizeof(*pipe->control),
                        /* We could be getting a bo from the bo-cache, make sure the fence value
   * is not garbage:
   */
   pipe->control->fence = 0;
               }
      struct fd_pipe *
   fd_pipe_new(struct fd_device *dev, enum fd_pipe_id id)
   {
         }
      struct fd_pipe *
   fd_pipe_ref(struct fd_pipe *pipe)
   {
      simple_mtx_lock(&fence_lock);
   fd_pipe_ref_locked(pipe);
   simple_mtx_unlock(&fence_lock);
      }
      struct fd_pipe *
   fd_pipe_ref_locked(struct fd_pipe *pipe)
   {
      simple_mtx_assert_locked(&fence_lock);
   pipe->refcnt++;
      }
      void
   fd_pipe_del(struct fd_pipe *pipe)
   {
      simple_mtx_lock(&fence_lock);
   fd_pipe_del_locked(pipe);
      }
      void
   fd_pipe_del_locked(struct fd_pipe *pipe)
   {
      simple_mtx_assert_locked(&fence_lock);
   if (--pipe->refcnt)
            fd_bo_del(pipe->control_mem);
      }
      /**
   * Flush any unflushed deferred submits.  This is called at context-
   * destroy to make sure we don't leak unflushed submits.
   */
   void
   fd_pipe_purge(struct fd_pipe *pipe)
   {
      struct fd_device *dev = pipe->dev;
                     /* We only queue up deferred submits for a single pipe at a time, so
   * if there is a deferred_submits_fence on the same pipe as us, we
   * know we have deferred_submits queued, which need to be flushed:
   */
   if (dev->deferred_submits_fence && dev->deferred_submits_fence->pipe == pipe) {
                           if (unflushed_fence) {
      fd_fence_flush(unflushed_fence);
         }
      int
   fd_pipe_get_param(struct fd_pipe *pipe, enum fd_param_id param, uint64_t *value)
   {
         }
      int
   fd_pipe_set_param(struct fd_pipe *pipe, enum fd_param_id param, uint64_t value)
   {
         }
      const struct fd_dev_id *
   fd_pipe_dev_id(struct fd_pipe *pipe)
   {
         }
      int
   fd_pipe_wait(struct fd_pipe *pipe, const struct fd_fence *fence)
   {
         }
      int
   fd_pipe_wait_timeout(struct fd_pipe *pipe, const struct fd_fence *fence,
         {
      if (!fd_fence_after(fence->ufence, pipe->control->fence))
            if (!timeout)
                        }
      uint32_t
   fd_pipe_emit_fence(struct fd_pipe *pipe, struct fd_ringbuffer *ring)
   {
               if (pipe->is_64bit) {
      OUT_PKT7(ring, CP_EVENT_WRITE, 4);
   OUT_RING(ring, CP_EVENT_WRITE_0_EVENT(CACHE_FLUSH_TS));
   OUT_RELOC(ring, control_ptr(pipe, fence));   /* ADDR_LO/HI */
      } else {
      OUT_PKT3(ring, CP_EVENT_WRITE, 3);
   OUT_RING(ring, CP_EVENT_WRITE_0_EVENT(CACHE_FLUSH_TS));
   OUT_RELOC(ring, control_ptr(pipe, fence));   /* ADDR */
                  }
      struct fd_fence *
   fd_fence_new(struct fd_pipe *pipe, bool use_fence_fd)
   {
               f->refcnt = 1;
   f->pipe = fd_pipe_ref(pipe);
   util_queue_fence_init(&f->ready);
   f->use_fence_fd = use_fence_fd;
               }
      struct fd_fence *
   fd_fence_ref(struct fd_fence *f)
   {
      simple_mtx_lock(&fence_lock);
   fd_fence_ref_locked(f);
               }
      struct fd_fence *
   fd_fence_ref_locked(struct fd_fence *f)
   {
      simple_mtx_assert_locked(&fence_lock);
   f->refcnt++;
      }
      void
   fd_fence_del(struct fd_fence *f)
   {
      simple_mtx_lock(&fence_lock);
   fd_fence_del_locked(f);
      }
      void
   fd_fence_del_locked(struct fd_fence *f)
   {
               if (--f->refcnt)
                     if (f->use_fence_fd && (f->fence_fd != -1))
               }
      /**
   * Wait until corresponding submit is flushed to kernel
   */
   void
   fd_fence_flush(struct fd_fence *f)
   {
      MESA_TRACE_FUNC();
   /*
   * TODO we could simplify this to remove the flush_sync part of
   * fd_pipe_sp_flush() and just rely on the util_queue_fence_wait()
   */
   fd_pipe_flush(f->pipe, f->ufence);
      }
      int
   fd_fence_wait(struct fd_fence *f)
   {
      MESA_TRACE_FUNC();
      }
