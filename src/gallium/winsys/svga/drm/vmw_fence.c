   /**********************************************************
   * Copyright 2009-2015 VMware, Inc.  All rights reserved.
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
   #include <libsync.h>
      #include "util/u_memory.h"
   #include "util/u_atomic.h"
   #include "util/list.h"
   #include "util/u_thread.h"
      #include "pipebuffer/pb_buffer_fenced.h"
      #include "vmw_screen.h"
   #include "vmw_fence.h"
      struct vmw_fence_ops
   {
      /*
   * Immutable members.
   */
   struct pb_fence_ops base;
                     /*
   * Protected by mutex;
   */
   struct list_head not_signaled;
   uint32_t last_signaled;
      };
      struct vmw_fence
   {
      struct list_head ops_list;
   int32_t refcount;
   uint32_t handle;
   uint32_t mask;
   int32_t signalled;
   uint32_t seqno;
   int32_t fence_fd;
      };
      /**
   * vmw_fence_seq_is_signaled - Check whether a fence seqno is
   * signaled.
   *
   * @ops: Pointer to a struct pb_fence_ops.
   *
   */
   static inline bool
   vmw_fence_seq_is_signaled(uint32_t seq, uint32_t last, uint32_t cur)
   {
         }
         /**
   * vmw_fence_ops - Return the vmw_fence_ops structure backing a
   * struct pb_fence_ops pointer.
   *
   * @ops: Pointer to a struct pb_fence_ops.
   *
   */
   static inline struct vmw_fence_ops *
   vmw_fence_ops(struct pb_fence_ops *ops)
   {
      assert(ops);
      }
         /**
   * vmw_fences_release - Release all fences from the not_signaled
   * list.
   *
   * @ops: Pointer to a struct vmw_fence_ops.
   *
   */
   static void
   vmw_fences_release(struct vmw_fence_ops *ops)
   {
               mtx_lock(&ops->mutex);
   LIST_FOR_EACH_ENTRY_SAFE(fence, n, &ops->not_signaled, ops_list)
            }
      /**
   * vmw_fences_signal - Traverse the not_signaled list and try to
   * signal unsignaled fences.
   *
   * @ops: Pointer to a struct pb_fence_ops.
   * @signaled: Seqno that has signaled.
   * @emitted: Last seqno emitted by the kernel.
   * @has_emitted: Whether we provide the emitted value.
   *
   */
   void
   vmw_fences_signal(struct pb_fence_ops *fence_ops,
                     {
      struct vmw_fence_ops *ops = NULL;
            if (fence_ops == NULL)
            ops = vmw_fence_ops(fence_ops);
            if (!has_emitted) {
      emitted = ops->last_emitted;
   emitted = signaled;
               if (signaled == ops->last_signaled && emitted == ops->last_emitted)
            LIST_FOR_EACH_ENTRY_SAFE(fence, n, &ops->not_signaled, ops_list) {
      if (!vmw_fence_seq_is_signaled(fence->seqno, signaled, emitted))
            p_atomic_set(&fence->signalled, 1);
      }
   ops->last_signaled = signaled;
         out_unlock:
         }
         /**
   * vmw_fence - return the vmw_fence object identified by a
   * struct pipe_fence_handle *
   *
   * @fence: The opaque pipe fence handle.
   */
   static inline struct vmw_fence *
   vmw_fence(struct pipe_fence_handle *fence)
   {
         }
         /**
   * vmw_fence_create - Create a user-space fence object.
   *
   * @fence_ops: The fence_ops manager to register with.
   * @handle: Handle identifying the kernel fence object.
   * @mask: Mask of flags that this fence object may signal.
   * @fd: File descriptor to associate with the fence
   *
   * Returns NULL on failure.
   */
   struct pipe_fence_handle *
   vmw_fence_create(struct pb_fence_ops *fence_ops, uint32_t handle,
         {
      struct vmw_fence *fence = CALLOC_STRUCT(vmw_fence);
            if (!fence)
            p_atomic_set(&fence->refcount, 1);
   fence->handle = handle;
   fence->mask = mask;
   fence->seqno = seqno;
   fence->fence_fd = fd;
            /*
   * If the fence was not created by our device, then we won't
   * manage it with our ops
   */
   if (!fence_ops) {
      fence->imported = true;
                                 if (vmw_fence_seq_is_signaled(seqno, ops->last_signaled, seqno)) {
      p_atomic_set(&fence->signalled, 1);
      } else {
      p_atomic_set(&fence->signalled, 0);
                           }
         /**
   * vmw_fence_destroy - Frees a vmw fence object.
   *
   * Also closes the file handle associated with the object, if any
   */
   static
   void vmw_fence_destroy(struct vmw_fence *vfence)
   {
      if (vfence->fence_fd != -1)
               }
         /**
   * vmw_fence_reference - Reference / unreference a vmw fence object.
   *
   * @vws: Pointer to the winsys screen.
   * @ptr: Pointer to reference transfer destination.
   * @fence: Pointer to object to reference. May be NULL.
   */
   void
   vmw_fence_reference(struct vmw_winsys_screen *vws,
         struct pipe_fence_handle **ptr,
   {
      if (*ptr) {
               if (p_atomic_dec_zero(&vfence->refcount)) {
                                 mtx_lock(&ops->mutex);
   list_delinit(&vfence->ops_list);
                              if (fence) {
                              }
         /**
   * vmw_fence_signalled - Check whether a fence object is signalled.
   *
   * @vws: Pointer to the winsys screen.
   * @fence: Handle to the fence object.
   * @flag: Fence flags to check. If the fence object can't signal
   * a flag, it is assumed to be already signaled.
   *
   * Returns 0 if the fence object was signaled, nonzero otherwise.
   */
   int
   vmw_fence_signalled(struct vmw_winsys_screen *vws,
      struct pipe_fence_handle *fence,
      {
      struct vmw_fence *vfence;
   int32_t vflags = SVGA_FENCE_FLAG_EXEC;
   int ret;
            if (!fence)
            vfence = vmw_fence(fence);
                     if ((old & vflags) == vflags)
            /*
   * Currently we update signaled fences on each execbuf call.
   * That should really be sufficient, and we can avoid
   * a lot of kernel calls this way.
      #if 1
               if (ret == 0)
            #else
      (void) ret;
      #endif
   }
      /**
   * vmw_fence_finish - Wait for a fence object to signal.
   *
   * @vws: Pointer to the winsys screen.
   * @fence: Handle to the fence object.
   * @timeout: How long to wait before timing out.
   * @flag: Fence flags to wait for. If the fence object can't signal
   * a flag, it is assumed to be already signaled.
   *
   * Returns 0 if the wait succeeded. Nonzero otherwise.
   */
   int
   vmw_fence_finish(struct vmw_winsys_screen *vws,
      struct pipe_fence_handle *fence,
   uint64_t timeout,
      {
      struct vmw_fence *vfence;
   int32_t vflags = SVGA_FENCE_FLAG_EXEC;
   int ret;
            if (!fence)
                     if (vfence->imported) {
               if (!ret)
                        old = p_atomic_read(&vfence->signalled);
            if ((old & vflags) == vflags)
                     if (ret == 0) {
               old = prev;
   prev = p_atomic_cmpxchg(&vfence->signalled, old, old | vflags);
                        }
      /**
   * vmw_fence_get_fd
   *
   * Returns the file descriptor associated with the fence
   */
   int
   vmw_fence_get_fd(struct pipe_fence_handle *fence)
   {
               if (!fence)
            vfence = vmw_fence(fence);
      }
         /**
   * vmw_fence_ops_fence_reference - wrapper for the pb_fence_ops api.
   *
   * wrapper around vmw_fence_reference.
   */
   static void
   vmw_fence_ops_fence_reference(struct pb_fence_ops *ops,
               {
                  }
      /**
   * vmw_fence_ops_fence_signalled - wrapper for the pb_fence_ops api.
   *
   * wrapper around vmw_fence_signalled.
   */
   static int
   vmw_fence_ops_fence_signalled(struct pb_fence_ops *ops,
               {
                  }
         /**
   * vmw_fence_ops_fence_finish - wrapper for the pb_fence_ops api.
   *
   * wrapper around vmw_fence_finish.
   */
   static int
   vmw_fence_ops_fence_finish(struct pb_fence_ops *ops,
               {
                  }
         /**
   * vmw_fence_ops_destroy - Destroy a pb_fence_ops function table.
   *
   * @ops: The function table to destroy.
   *
   * Part of the pb_fence_ops api.
   */
   static void
   vmw_fence_ops_destroy(struct pb_fence_ops *ops)
   {
      vmw_fences_release(vmw_fence_ops(ops));
      }
         /**
   * vmw_fence_ops_create - Create a pb_fence_ops function table.
   *
   * @vws: Pointer to a struct vmw_winsys_screen.
   *
   * Returns a pointer to a pb_fence_ops function table to interface
   * with pipe_buffer. This function is typically called on driver setup.
   *
   * Returns NULL on failure.
   */
   struct pb_fence_ops *
   vmw_fence_ops_create(struct vmw_winsys_screen *vws) 
   {
               ops = CALLOC_STRUCT(vmw_fence_ops);
   if(!ops)
            (void) mtx_init(&ops->mutex, mtx_plain);
   list_inithead(&ops->not_signaled);
   ops->base.destroy = &vmw_fence_ops_destroy;
   ops->base.fence_reference = &vmw_fence_ops_fence_reference;
   ops->base.fence_signalled = &vmw_fence_ops_fence_signalled;
                        }
