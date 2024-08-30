   /**********************************************************
   * Copyright 2009-2023 VMware, Inc.  All rights reserved.
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
         #include "vmw_screen.h"
      #include "vmw_buffer.h"
   #include "vmw_fence.h"
      #include "pipebuffer/pb_buffer.h"
   #include "pipebuffer/pb_bufmgr.h"
      /**
   * vmw_pools_cleanup - Destroy the buffer pools.
   *
   * @vws: pointer to a struct vmw_winsys_screen.
   */
   void
   vmw_pools_cleanup(struct vmw_winsys_screen *vws)
   {
      if (vws->pools.dma_slab_fenced)
      vws->pools.dma_slab_fenced->destroy
      if (vws->pools.dma_slab)
         if (vws->pools.dma_fenced)
         if (vws->pools.dma_cache)
            if (vws->pools.query_fenced)
         if (vws->pools.query_mm)
            if (vws->pools.dma_mm)
         if (vws->pools.dma_base)
      }
         /**
   * vmw_query_pools_init - Create a pool of query buffers.
   *
   * @vws: Pointer to a struct vmw_winsys_screen.
   *
   * Typically this pool should be created on demand when we
   * detect that the app will be using queries. There's nothing
   * special with this pool other than the backing kernel buffer sizes,
   * which are limited to 8192.
   * If there is a performance issue with allocation and freeing of the
   * query slabs, it should be easily fixable by allocating them out
   * of a buffer cache.
   */
   bool
   vmw_query_pools_init(struct vmw_winsys_screen *vws)
   {
               desc.alignment = 16;
            vws->pools.query_mm = pb_slab_range_manager_create(vws->pools.dma_base, 16, 128,
               if (!vws->pools.query_mm)
            vws->pools.query_fenced = simple_fenced_bufmgr_create(
            if(!vws->pools.query_fenced)
                  out_no_query_fenced:
      vws->pools.query_mm->destroy(vws->pools.query_mm);
      }
      /**
   * vmw_pool_init - Create a pool of buffers.
   *
   * @vws: Pointer to a struct vmw_winsys_screen.
   */
   bool
   vmw_pools_init(struct vmw_winsys_screen *vws)
   {
               vws->pools.dma_base = vmw_dma_bufmgr_create(vws);
   if (!vws->pools.dma_base)
            /*
   * A managed pool for DMA buffers.
   */
   vws->pools.dma_mm = mm_bufmgr_create(vws->pools.dma_base,
               if(!vws->pools.dma_mm)
            vws->pools.dma_cache =
      pb_cache_manager_create(vws->pools.dma_base, 100000, 2.0f,
               if (!vws->pools.dma_cache)
            vws->pools.dma_fenced =
      simple_fenced_bufmgr_create(vws->pools.dma_cache,
         if(!vws->pools.dma_fenced)
            /*
   * The slab pool allocates buffers directly from the kernel except
   * for very small buffers which are allocated from a slab in order
   * not to waste memory, since a kernel buffer is a minimum 4096 bytes.
   *
   * Here we use it only for emergency in the case our pre-allocated
   * managed buffer pool runs out of memory.
   */
   desc.alignment = 64;
   desc.usage = ~(SVGA_BUFFER_USAGE_PINNED | VMW_BUFFER_USAGE_SHARED |
         vws->pools.dma_slab =
      pb_slab_range_manager_create(vws->pools.dma_cache,
                        if(!vws->pools.dma_slab)
            vws->pools.dma_slab_fenced =
      simple_fenced_bufmgr_create(vws->pools.dma_slab,
      if (!vws->pools.dma_slab_fenced)
            vws->pools.query_fenced = NULL;
                  error:
      vmw_pools_cleanup(vws);
      }
