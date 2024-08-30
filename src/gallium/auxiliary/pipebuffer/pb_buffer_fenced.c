   /**************************************************************************
   *
   * Copyright 2007-2010 VMware, Inc.
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
      /**
   * \file
   * Implementation of fenced buffers.
   *
   * \author Jose Fonseca <jfonseca-at-vmware-dot-com>
   * \author Thomas Hellström <thellstrom-at-vmware-dot-com>
   */
         #include "util/detect.h"
      #if DETECT_OS_LINUX || DETECT_OS_BSD || DETECT_OS_SOLARIS
   #include <unistd.h>
   #include <sched.h>
   #endif
   #include <inttypes.h>
      #include "util/compiler.h"
   #include "pipe/p_defines.h"
   #include "util/u_debug.h"
   #include "util/u_thread.h"
   #include "util/u_memory.h"
   #include "util/list.h"
      #include "pb_buffer.h"
   #include "pb_buffer_fenced.h"
   #include "pb_bufmgr.h"
            /**
   * Convenience macro (type safe).
   */
   #define SUPER(__derived) (&(__derived)->base)
         struct fenced_manager
   {
      struct pb_manager base;
   struct pb_manager *provider;
            /**
   * Maximum buffer size that can be safely allocated.
   */
            /**
   * Maximum cpu memory we can allocate before we start waiting for the
   * GPU to idle.
   */
            /**
   * Following members are mutable and protected by this mutex.
   */
            /**
   * Fenced buffer list.
   *
   * All fenced buffers are placed in this listed, ordered from the oldest
   * fence to the newest fence.
   */
   struct list_head fenced;
            struct list_head unfenced;
            /**
   * How much temporary CPU memory is being used to hold unvalidated buffers.
   */
      };
         /**
   * Fenced buffer.
   *
   * Wrapper around a pipe buffer which adds fencing and reference counting.
   */
   struct fenced_buffer
   {
      /**
   * Immutable members.
            struct pb_buffer base;
            /**
   * Following members are mutable and protected by fenced_manager::mutex.
                     /**
   * Buffer with storage.
   */
   struct pb_buffer *buffer;
   pb_size size;
            /**
   * Temporary CPU storage data. Used when there isn't enough GPU memory to
   * store the buffer.
   */
            /**
   * A bitmask of PB_USAGE_CPU/GPU_READ/WRITE describing the current
   * buffer usage.
   */
                     struct pb_validate *vl;
               };
         static inline struct fenced_manager *
   fenced_manager(struct pb_manager *mgr)
   {
      assert(mgr);
      }
         static inline struct fenced_buffer *
   fenced_buffer(struct pb_buffer *buf)
   {
      assert(buf);
      }
         static void
   fenced_buffer_destroy_cpu_storage_locked(struct fenced_buffer *fenced_buf);
      static enum pipe_error
   fenced_buffer_create_cpu_storage_locked(struct fenced_manager *fenced_mgr,
            static void
   fenced_buffer_destroy_gpu_storage_locked(struct fenced_buffer *fenced_buf);
      static enum pipe_error
   fenced_buffer_create_gpu_storage_locked(struct fenced_manager *fenced_mgr,
                  static enum pipe_error
   fenced_buffer_copy_storage_to_gpu_locked(struct fenced_buffer *fenced_buf);
      static enum pipe_error
   fenced_buffer_copy_storage_to_cpu_locked(struct fenced_buffer *fenced_buf);
         /**
   * Dump the fenced buffer list.
   *
   * Useful to understand failures to allocate buffers.
   */
   static void
   fenced_manager_dump_locked(struct fenced_manager *fenced_mgr)
   {
   #ifdef DEBUG
      struct pb_fence_ops *ops = fenced_mgr->ops;
   struct list_head *curr, *next;
            debug_printf("%10s %7s %8s %7s %10s %s\n",
            curr = fenced_mgr->unfenced.next;
   next = curr->next;
   while (curr != &fenced_mgr->unfenced) {
      fenced_buf = list_entry(curr, struct fenced_buffer, head);
   assert(!fenced_buf->fence);
   debug_printf("%10p %"PRIu64" %8u %7s\n",
               (void *) fenced_buf,
   fenced_buf->base.size,
   curr = next;
               curr = fenced_mgr->fenced.next;
   next = curr->next;
   while (curr != &fenced_mgr->fenced) {
      int signaled;
   fenced_buf = list_entry(curr, struct fenced_buffer, head);
   assert(fenced_buf->buffer);
   signaled = ops->fence_signalled(ops, fenced_buf->fence, 0);
   debug_printf("%10p %"PRIu64" %8u %7s %10p %s\n",
               (void *) fenced_buf,
   fenced_buf->base.size,
   p_atomic_read(&fenced_buf->base.reference.count),
   "gpu",
   curr = next;
         #else
         #endif
   }
         static inline void
   fenced_buffer_destroy_locked(struct fenced_manager *fenced_mgr,
         {
               assert(!fenced_buf->fence);
   assert(fenced_buf->head.prev);
   assert(fenced_buf->head.next);
   list_del(&fenced_buf->head);
   assert(fenced_mgr->num_unfenced);
            fenced_buffer_destroy_gpu_storage_locked(fenced_buf);
               }
         /**
   * Add the buffer to the fenced list.
   *
   * Reference count should be incremented before calling this function.
   */
   static inline void
   fenced_buffer_add_locked(struct fenced_manager *fenced_mgr,
         {
      assert(pipe_is_referenced(&fenced_buf->base.reference));
   assert(fenced_buf->flags & PB_USAGE_GPU_READ_WRITE);
                     list_del(&fenced_buf->head);
   assert(fenced_mgr->num_unfenced);
   --fenced_mgr->num_unfenced;
   list_addtail(&fenced_buf->head, &fenced_mgr->fenced);
      }
         /**
   * Remove the buffer from the fenced list, and potentially destroy the buffer
   * if the reference count reaches zero.
   *
   * Returns TRUE if the buffer was detroyed.
   */
   static inline bool
   fenced_buffer_remove_locked(struct fenced_manager *fenced_mgr,
         {
               assert(fenced_buf->fence);
            ops->fence_reference(ops, &fenced_buf->fence, NULL);
            assert(fenced_buf->head.prev);
            list_del(&fenced_buf->head);
   assert(fenced_mgr->num_fenced);
            list_addtail(&fenced_buf->head, &fenced_mgr->unfenced);
            if (p_atomic_dec_zero(&fenced_buf->base.reference.count)) {
      fenced_buffer_destroy_locked(fenced_mgr, fenced_buf);
                  }
         /**
   * Wait for the fence to expire, and remove it from the fenced list.
   *
   * This function will release and re-acquire the mutex, so any copy of mutable
   * state must be discarded after calling it.
   */
   static inline enum pipe_error
   fenced_buffer_finish_locked(struct fenced_manager *fenced_mgr,
         {
      struct pb_fence_ops *ops = fenced_mgr->ops;
         #if 0
         #endif
         assert(pipe_is_referenced(&fenced_buf->base.reference));
            if (fenced_buf->fence) {
      struct pipe_fence_handle *fence = NULL;
   int finished;
                                                         /* Only proceed if the fence object didn't change in the meanwhile.
   * Otherwise assume the work has been already carried out by another
   * thread that re-aquired the lock before us.
   */
                     if (proceed && finished == 0) {
                                                                        }
         /**
   * Remove as many fenced buffers from the fenced list as possible.
   *
   * Returns TRUE if at least one buffer was removed.
   */
   static bool
   fenced_manager_check_signalled_locked(struct fenced_manager *fenced_mgr,
         {
      struct pb_fence_ops *ops = fenced_mgr->ops;
   struct list_head *curr, *next;
   struct fenced_buffer *fenced_buf;
   struct pipe_fence_handle *prev_fence = NULL;
            curr = fenced_mgr->fenced.next;
   next = curr->next;
   while (curr != &fenced_mgr->fenced) {
               if (fenced_buf->fence != prev_fence) {
                                 /* Don't return just now. Instead preemptively check if the
   * following buffers' fences already expired, without further waits.
   */
      } else {
                  if (signaled != 0) {
                     } else {
      /* This buffer's fence object is identical to the previous buffer's
   * fence object, so no need to check the fence again.
   */
                                 curr = next;
                  }
         /**
   * Try to free some GPU memory by backing it up into CPU memory.
   *
   * Returns TRUE if at least one buffer was freed.
   */
   static bool
   fenced_manager_free_gpu_storage_locked(struct fenced_manager *fenced_mgr)
   {
      struct list_head *curr, *next;
            curr = fenced_mgr->unfenced.next;
   next = curr->next;
   while (curr != &fenced_mgr->unfenced) {
               /* We can only move storage if the buffer is not mapped and not
   * validated.
   */
   if (fenced_buf->buffer &&
      !fenced_buf->mapcount &&
                  ret = fenced_buffer_create_cpu_storage_locked(fenced_mgr, fenced_buf);
   if (ret == PIPE_OK) {
      ret = fenced_buffer_copy_storage_to_cpu_locked(fenced_buf);
   if (ret == PIPE_OK) {
      fenced_buffer_destroy_gpu_storage_locked(fenced_buf);
      }
                  curr = next;
                  }
         /**
   * Destroy CPU storage for this buffer.
   */
   static void
   fenced_buffer_destroy_cpu_storage_locked(struct fenced_buffer *fenced_buf)
   {
      if (fenced_buf->data) {
      align_free(fenced_buf->data);
   fenced_buf->data = NULL;
   assert(fenced_buf->mgr->cpu_total_size >= fenced_buf->size);
         }
         /**
   * Create CPU storage for this buffer.
   */
   static enum pipe_error
   fenced_buffer_create_cpu_storage_locked(struct fenced_manager *fenced_mgr,
         {
      assert(!fenced_buf->data);
   if (fenced_buf->data)
            if (fenced_mgr->cpu_total_size + fenced_buf->size > fenced_mgr->max_cpu_total_size)
            fenced_buf->data = align_malloc(fenced_buf->size, fenced_buf->desc.alignment);
   if (!fenced_buf->data)
                        }
         /**
   * Destroy the GPU storage.
   */
   static void
   fenced_buffer_destroy_gpu_storage_locked(struct fenced_buffer *fenced_buf)
   {
      if (fenced_buf->buffer) {
            }
         /**
   * Try to create GPU storage for this buffer.
   *
   * This function is a shorthand around pb_manager::create_buffer for
   * fenced_buffer_create_gpu_storage_locked()'s benefit.
   */
   static inline bool
   fenced_buffer_try_create_gpu_storage_locked(struct fenced_manager *fenced_mgr,
         {
                        fenced_buf->buffer = provider->create_buffer(fenced_mgr->provider,
                  }
         /**
   * Create GPU storage for this buffer.
   */
   static enum pipe_error
   fenced_buffer_create_gpu_storage_locked(struct fenced_manager *fenced_mgr,
               {
               /* Check for signaled buffers before trying to allocate. */
                     /* Keep trying while there is some sort of progress:
   * - fences are expiring,
   * - or buffers are being being swapped out from GPU memory into CPU memory.
   */
   while (!fenced_buf->buffer &&
         (fenced_manager_check_signalled_locked(fenced_mgr, false) ||
                  if (!fenced_buf->buffer && wait) {
      /* Same as before, but this time around, wait to free buffers if
   * necessary.
   */
   while (!fenced_buf->buffer &&
         (fenced_manager_check_signalled_locked(fenced_mgr, true) ||
                     if (!fenced_buf->buffer) {
      if (0)
            /* Give up. */
                  }
         static enum pipe_error
   fenced_buffer_copy_storage_to_gpu_locked(struct fenced_buffer *fenced_buf)
   {
               assert(fenced_buf->data);
            map = pb_map(fenced_buf->buffer, PB_USAGE_CPU_WRITE, NULL);
   if (!map)
                                 }
         static enum pipe_error
   fenced_buffer_copy_storage_to_cpu_locked(struct fenced_buffer *fenced_buf)
   {
               assert(fenced_buf->data);
            map = pb_map(fenced_buf->buffer, PB_USAGE_CPU_READ, NULL);
   if (!map)
                                 }
         static void
   fenced_buffer_destroy(void *winsys, struct pb_buffer *buf)
   {
      struct fenced_buffer *fenced_buf = fenced_buffer(buf);
                                          }
         static void *
   fenced_buffer_map(struct pb_buffer *buf,
         {
      struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   struct fenced_manager *fenced_mgr = fenced_buf->mgr;
   struct pb_fence_ops *ops = fenced_mgr->ops;
                              /* Serialize writes. */
   while ((fenced_buf->flags & PB_USAGE_GPU_WRITE) ||
                     /* Don't wait for the GPU to finish accessing it,
   * if blocking is forbidden.
   */
   if ((flags & PB_USAGE_DONTBLOCK) &&
      ops->fence_signalled(ops, fenced_buf->fence, 0) != 0) {
               if (flags & PB_USAGE_UNSYNCHRONIZED) {
                  /* Wait for the GPU to finish accessing. This will release and re-acquire
   * the mutex, so all copies of mutable state must be discarded.
   */
               if (fenced_buf->buffer) {
         } else {
      assert(fenced_buf->data);
               if (map) {
      ++fenced_buf->mapcount;
            done:
                  }
         static void
   fenced_buffer_unmap(struct pb_buffer *buf)
   {
      struct fenced_buffer *fenced_buf = fenced_buffer(buf);
                     assert(fenced_buf->mapcount);
   if (fenced_buf->mapcount) {
      if (fenced_buf->buffer)
         --fenced_buf->mapcount;
   if (!fenced_buf->mapcount)
                  }
         static enum pipe_error
   fenced_buffer_validate(struct pb_buffer *buf,
               {
      struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   struct fenced_manager *fenced_mgr = fenced_buf->mgr;
                     if (!vl) {
      /* Invalidate. */
   fenced_buf->vl = NULL;
   fenced_buf->validation_flags = 0;
   ret = PIPE_OK;
               assert(flags & PB_USAGE_GPU_READ_WRITE);
   assert(!(flags & ~PB_USAGE_GPU_READ_WRITE));
            /* Buffer cannot be validated in two different lists. */
   if (fenced_buf->vl && fenced_buf->vl != vl) {
      ret = PIPE_ERROR_RETRY;
               if (fenced_buf->vl == vl &&
      (fenced_buf->validation_flags & flags) == flags) {
   /* Nothing to do -- buffer already validated. */
   ret = PIPE_OK;
               /* Create and update GPU storage. */
   if (!fenced_buf->buffer) {
               ret = fenced_buffer_create_gpu_storage_locked(fenced_mgr, fenced_buf, true);
   if (ret != PIPE_OK) {
                  ret = fenced_buffer_copy_storage_to_gpu_locked(fenced_buf);
   if (ret != PIPE_OK) {
      fenced_buffer_destroy_gpu_storage_locked(fenced_buf);
               if (fenced_buf->mapcount) {
         } else {
                     ret = pb_validate(fenced_buf->buffer, vl, flags);
   if (ret != PIPE_OK)
            fenced_buf->vl = vl;
         done:
                  }
         static void
   fenced_buffer_fence(struct pb_buffer *buf,
         {
      struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   struct fenced_manager *fenced_mgr = fenced_buf->mgr;
                     assert(pipe_is_referenced(&fenced_buf->base.reference));
            if (fence != fenced_buf->fence) {
      assert(fenced_buf->vl);
            if (fenced_buf->fence) {
      ASSERTED bool destroyed = fenced_buffer_remove_locked(fenced_mgr, fenced_buf);
      }
   if (fence) {
      ops->fence_reference(ops, &fenced_buf->fence, fence);
   fenced_buf->flags |= fenced_buf->validation_flags;
                        fenced_buf->vl = NULL;
                  }
         static void
   fenced_buffer_get_base_buffer(struct pb_buffer *buf,
               {
      struct fenced_buffer *fenced_buf = fenced_buffer(buf);
                     /* This should only be called when the buffer is validated. Typically
   * when processing relocations.
   */
   assert(fenced_buf->vl);
            if (fenced_buf->buffer) {
         } else {
      *base_buf = buf;
                  }
         static const struct pb_vtbl
   fenced_buffer_vtbl = {
      fenced_buffer_destroy,
   fenced_buffer_map,
   fenced_buffer_unmap,
   fenced_buffer_validate,
   fenced_buffer_fence,
      };
         /**
   * Wrap a buffer in a fenced buffer.
   */
   static struct pb_buffer *
   fenced_bufmgr_create_buffer(struct pb_manager *mgr,
               {
      struct fenced_manager *fenced_mgr = fenced_manager(mgr);
   struct fenced_buffer *fenced_buf;
            /* Don't stall the GPU, waste time evicting buffers, or waste memory
   * trying to create a buffer that will most likely never fit into the
   * graphics aperture.
   */
   if (size > fenced_mgr->max_buffer_size) {
                  fenced_buf = CALLOC_STRUCT(fenced_buffer);
   if (!fenced_buf)
            pipe_reference_init(&fenced_buf->base.reference, 1);
   fenced_buf->base.alignment_log2 = util_logbase2(desc->alignment);
   fenced_buf->base.usage = desc->usage;
   fenced_buf->base.size = size;
   fenced_buf->size = size;
            fenced_buf->base.vtbl = &fenced_buffer_vtbl;
                     /* Try to create GPU storage without stalling. */
            /* Attempt to use CPU memory to avoid stalling the GPU. */
   if (ret != PIPE_OK) {
                  /* Create GPU storage, waiting for some to be available. */
   if (ret != PIPE_OK) {
                  /* Give up. */
   if (ret != PIPE_OK) {
                           list_addtail(&fenced_buf->head, &fenced_mgr->unfenced);
   ++fenced_mgr->num_unfenced;
                  no_storage:
      mtx_unlock(&fenced_mgr->mutex);
      no_buffer:
         }
         static void
   fenced_bufmgr_flush(struct pb_manager *mgr)
   {
               mtx_lock(&fenced_mgr->mutex);
   while (fenced_manager_check_signalled_locked(fenced_mgr, true))
                  assert(fenced_mgr->provider->flush);
   if (fenced_mgr->provider->flush)
      }
         static void
   fenced_bufmgr_destroy(struct pb_manager *mgr)
   {
                        /* Wait on outstanding fences. */
   while (fenced_mgr->num_fenced) {
      #if DETECT_OS_LINUX || DETECT_OS_BSD || DETECT_OS_SOLARIS
         #endif
         mtx_lock(&fenced_mgr->mutex);
   while (fenced_manager_check_signalled_locked(fenced_mgr, true))
            #ifdef DEBUG
         #endif
         mtx_unlock(&fenced_mgr->mutex);
            if (fenced_mgr->provider)
                        }
         struct pb_manager *
   fenced_bufmgr_create(struct pb_manager *provider,
                     {
               if (!provider)
            fenced_mgr = CALLOC_STRUCT(fenced_manager);
   if (!fenced_mgr)
            fenced_mgr->base.destroy = fenced_bufmgr_destroy;
   fenced_mgr->base.create_buffer = fenced_bufmgr_create_buffer;
            fenced_mgr->provider = provider;
   fenced_mgr->ops = ops;
   fenced_mgr->max_buffer_size = max_buffer_size;
            list_inithead(&fenced_mgr->fenced);
            list_inithead(&fenced_mgr->unfenced);
                        }
