   /**************************************************************************
   *
   * Copyright 2006-2008 VMware, Inc., USA
   * All Rights Reserved.
   *
   * Permission is hereby granted, FREE of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   *
   **************************************************************************/
      /**
   * @file
   * S-lab pool implementation.
   * 
   * @sa http://en.wikipedia.org/wiki/Slab_allocation
   * 
   * @author Thomas Hellstrom <thellstrom-at-vmware-dot-com>
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
      #include "util/compiler.h"
   #include "util/u_debug.h"
   #include "util/u_thread.h"
   #include "pipe/p_defines.h"
   #include "util/u_memory.h"
   #include "util/list.h"
      #include "pb_buffer.h"
   #include "pb_bufmgr.h"
         struct pb_slab;
         /**
   * Buffer in a slab.
   * 
   * Sub-allocation of a contiguous buffer.
   */
   struct pb_slab_buffer
   {
      struct pb_buffer base;
      struct pb_slab *slab;
      struct list_head head;
      unsigned mapCount;
      /** Offset relative to the start of the slab buffer. */
      };
         /**
   * Slab -- a contiguous piece of memory. 
   */
   struct pb_slab
   {
      struct list_head head;
   struct list_head freeBuffers;
   pb_size numBuffers;
   pb_size numFree;
      struct pb_slab_buffer *buffers;
   struct pb_slab_manager *mgr;
      /** Buffer from the provider */
   struct pb_buffer *bo;
         };
         /**
   * It adds/removes slabs as needed in order to meet the allocation/destruction 
   * of individual buffers.
   */
   struct pb_slab_manager 
   {
      struct pb_manager base;
      /** From where we get our buffers */
   struct pb_manager *provider;
      /** Size of the buffers we hand on downstream */
   pb_size bufSize;
      /** Size of the buffers we request upstream */
   pb_size slabSize;
      /** 
   * Alignment, usage to be used to allocate the slab buffers.
   * 
   * We can only provide buffers which are consistent (in alignment, usage) 
   * with this description.   
   */
            /** 
   * Partial slabs
   * 
   * Full slabs are not stored in any list. Empty slabs are destroyed 
   * immediatly.
   */
   struct list_head slabs;
         };
         /**
   * Wrapper around several slabs, therefore capable of handling buffers of 
   * multiple sizes. 
   * 
   * This buffer manager just dispatches buffer allocations to the appropriate slab
   * manager, according to the requested buffer size, or by passes the slab 
   * managers altogether for even greater sizes.
   * 
   * The data of this structure remains constant after
   * initialization and thus needs no mutex protection.
   */
   struct pb_slab_range_manager 
   {
               struct pb_manager *provider;
      pb_size minBufSize;
   pb_size maxBufSize;
      /** @sa pb_slab_manager::desc */ 
   struct pb_desc desc;
      unsigned numBuckets;
   pb_size *bucketSizes;
      /** Array of pb_slab_manager, one for each bucket size */
      };
         static inline struct pb_slab_buffer *
   pb_slab_buffer(struct pb_buffer *buf)
   {
      assert(buf);
      }
         static inline struct pb_slab_manager *
   pb_slab_manager(struct pb_manager *mgr)
   {
      assert(mgr);
      }
         static inline struct pb_slab_range_manager *
   pb_slab_range_manager(struct pb_manager *mgr)
   {
      assert(mgr);
      }
         /**
   * Delete a buffer from the slab delayed list and put
   * it on the slab FREE list.
   */
   static void
   pb_slab_buffer_destroy(void *winsys, struct pb_buffer *_buf)
   {
      struct pb_slab_buffer *buf = pb_slab_buffer(_buf);
   struct pb_slab *slab = buf->slab;
   struct pb_slab_manager *mgr = slab->mgr;
            mtx_lock(&mgr->mutex);
      assert(!pipe_is_referenced(&buf->base.reference));
               list_del(list);
   list_addtail(list, &slab->freeBuffers);
            if (slab->head.next == &slab->head)
            /* If the slab becomes totally empty, free it */
   if (slab->numFree == slab->numBuffers) {
      list = &slab->head;
   list_delinit(list);
   pb_unmap(slab->bo);
   pb_reference(&slab->bo, NULL);
   FREE(slab->buffers);
                  }
         static void *
   pb_slab_buffer_map(struct pb_buffer *_buf, 
               {
                        ++buf->mapCount;
      }
         static void
   pb_slab_buffer_unmap(struct pb_buffer *_buf)
   {
                  }
         static enum pipe_error 
   pb_slab_buffer_validate(struct pb_buffer *_buf, 
               {
      struct pb_slab_buffer *buf = pb_slab_buffer(_buf);
      }
         static void
   pb_slab_buffer_fence(struct pb_buffer *_buf, 
         {
      struct pb_slab_buffer *buf = pb_slab_buffer(_buf);
      }
         static void
   pb_slab_buffer_get_base_buffer(struct pb_buffer *_buf,
               {
      struct pb_slab_buffer *buf = pb_slab_buffer(_buf);
   pb_get_base_buffer(buf->slab->bo, base_buf, offset);
      }
         static const struct pb_vtbl 
   pb_slab_buffer_vtbl = {
         pb_slab_buffer_destroy,
   pb_slab_buffer_map,
   pb_slab_buffer_unmap,
   pb_slab_buffer_validate,
   pb_slab_buffer_fence,
   };
         /**
   * Create a new slab.
   * 
   * Called when we ran out of free slabs.
   */
   static enum pipe_error
   pb_slab_create(struct pb_slab_manager *mgr)
   {
      struct pb_slab *slab;
   struct pb_slab_buffer *buf;
   unsigned numBuffers;
   unsigned i;
            slab = CALLOC_STRUCT(pb_slab);
   if (!slab)
            slab->bo = mgr->provider->create_buffer(mgr->provider, mgr->slabSize, &mgr->desc);
   if(!slab->bo) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
               /* Note down the slab virtual address. All mappings are accessed directly 
   * through this address so it is required that the buffer is mapped
   * persistent */
   slab->virtual = pb_map(slab->bo, 
                     if(!slab->virtual) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
                        slab->buffers = CALLOC(numBuffers, sizeof(*slab->buffers));
   if (!slab->buffers) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
               list_inithead(&slab->head);
   list_inithead(&slab->freeBuffers);
   slab->numBuffers = numBuffers;
   slab->numFree = 0;
            buf = slab->buffers;
   for (i=0; i < numBuffers; ++i) {
      pipe_reference_init(&buf->base.reference, 0);
   buf->base.size = mgr->bufSize;
   buf->base.alignment_log2 = 0;
   buf->base.usage = 0;
   buf->base.vtbl = &pb_slab_buffer_vtbl;
   buf->slab = slab;
   buf->start = i* mgr->bufSize;
   buf->mapCount = 0;
   list_addtail(&buf->head, &slab->freeBuffers);
   slab->numFree++;
               /* Add this slab to the list of partial slabs */
                  out_err1: 
         out_err0: 
      FREE(slab);
      }
         static struct pb_buffer *
   pb_slab_manager_create_buffer(struct pb_manager *_mgr,
               {
      struct pb_slab_manager *mgr = pb_slab_manager(_mgr);
   static struct pb_slab_buffer *buf;
   struct pb_slab *slab;
            /* check size */
   assert(size <= mgr->bufSize);
   if(size > mgr->bufSize)
            /* check if we can provide the requested alignment */
   assert(pb_check_alignment(desc->alignment, mgr->desc.alignment));
   if(!pb_check_alignment(desc->alignment, mgr->desc.alignment))
         assert(pb_check_alignment(desc->alignment, mgr->bufSize));
   if(!pb_check_alignment(desc->alignment, mgr->bufSize))
            assert(pb_check_usage(desc->usage, mgr->desc.usage));
   if(!pb_check_usage(desc->usage, mgr->desc.usage))
            mtx_lock(&mgr->mutex);
      /* Create a new slab, if we run out of partial slabs */
   if (mgr->slabs.next == &mgr->slabs) {
      (void) pb_slab_create(mgr);
   mtx_unlock(&mgr->mutex);
   return NULL;
            }
      /* Allocate the buffer from a partial (or just created) slab */
   list = mgr->slabs.next;
   slab = list_entry(list, struct pb_slab, head);
      /* If totally full remove from the partial slab list */
   if (--slab->numFree == 0)
            list = slab->freeBuffers.next;
            mtx_unlock(&mgr->mutex);
   buf = list_entry(list, struct pb_slab_buffer, head);
      pipe_reference_init(&buf->base.reference, 1);
   buf->base.alignment_log2 = util_logbase2(desc->alignment);
   buf->base.usage = desc->usage;
         }
         static void
   pb_slab_manager_flush(struct pb_manager *_mgr)
   {
               assert(mgr->provider->flush);
   if(mgr->provider->flush)
      }
         static void
   pb_slab_manager_destroy(struct pb_manager *_mgr)
   {
               /* TODO: cleanup all allocated buffers */
      }
         struct pb_manager *
   pb_slab_manager_create(struct pb_manager *provider,
                     {
               mgr = CALLOC_STRUCT(pb_slab_manager);
   if (!mgr)
            mgr->base.destroy = pb_slab_manager_destroy;
   mgr->base.create_buffer = pb_slab_manager_create_buffer;
            mgr->provider = provider;
   mgr->bufSize = bufSize;
   mgr->slabSize = slabSize;
            list_inithead(&mgr->slabs);
                  }
         static struct pb_buffer *
   pb_slab_range_manager_create_buffer(struct pb_manager *_mgr,
               {
      struct pb_slab_range_manager *mgr = pb_slab_range_manager(_mgr);
   pb_size bufSize;
   pb_size reqSize = size;
            if(desc->alignment > reqSize)
            bufSize = mgr->minBufSize;
   for (i = 0; i < mgr->numBuckets; ++i) {
      return mgr->buckets[i]->create_buffer(mgr->buckets[i], size, desc);
                     /* Fall back to allocate a buffer object directly from the provider. */
      }
         static void
   pb_slab_range_manager_flush(struct pb_manager *_mgr)
   {
               /* Individual slabs don't hold any temporary buffers so no need to call them */
      assert(mgr->provider->flush);
   if(mgr->provider->flush)
      }
         static void
   pb_slab_range_manager_destroy(struct pb_manager *_mgr)
   {
      struct pb_slab_range_manager *mgr = pb_slab_range_manager(_mgr);
   unsigned i;
      for (i = 0; i < mgr->numBuckets; ++i)
         FREE(mgr->buckets);
   FREE(mgr->bucketSizes);
      }
         struct pb_manager *
   pb_slab_range_manager_create(struct pb_manager *provider,
                           {
      struct pb_slab_range_manager *mgr;
   pb_size bufSize;
            if (!provider)
            mgr = CALLOC_STRUCT(pb_slab_range_manager);
   if (!mgr)
            mgr->base.destroy = pb_slab_range_manager_destroy;
   mgr->base.create_buffer = pb_slab_range_manager_create_buffer;
            mgr->provider = provider;
   mgr->minBufSize = minBufSize;
            mgr->numBuckets = 1;
   bufSize = minBufSize;
   while(bufSize < maxBufSize) {
      bufSize *= 2;
      }
      mgr->buckets = CALLOC(mgr->numBuckets, sizeof(*mgr->buckets));
   if (!mgr->buckets)
            bufSize = minBufSize;
   for (i = 0; i < mgr->numBuckets; ++i) {
      mgr->buckets[i] = pb_slab_manager_create(provider, bufSize, slabSize, desc);
   goto out_err2;
                           out_err2: 
      for (i = 0; i < mgr->numBuckets; ++i)
         mgr->buckets[i]->destroy(mgr->buckets[i]);
      out_err1: 
         out_err0:
         }
