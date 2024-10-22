   /**************************************************************************
   *
   * Copyright 2007-2008 VMware, Inc.
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
   * Buffer cache.
   * 
   * \author Jose Fonseca <jfonseca-at-vmware-dot-com>
   * \author Thomas Hellström <thellstrom-at-vmware-dot-com>
   */
         #include "util/compiler.h"
   #include "util/u_debug.h"
   #include "util/u_thread.h"
   #include "util/u_memory.h"
   #include "util/list.h"
      #include "pb_buffer.h"
   #include "pb_bufmgr.h"
   #include "pb_cache.h"
         /**
   * Wrapper around a pipe buffer which adds delayed destruction.
   */
   struct pb_cache_buffer
   {
      struct pb_buffer base;
   struct pb_buffer *buffer;
   struct pb_cache_manager *mgr;
      };
         struct pb_cache_manager
   {
      struct pb_manager base;
   struct pb_manager *provider;
      };
         static inline struct pb_cache_buffer *
   pb_cache_buffer(struct pb_buffer *buf)
   {
      assert(buf);
      }
         static inline struct pb_cache_manager *
   pb_cache_manager(struct pb_manager *mgr)
   {
      assert(mgr);
      }
         void
   pb_cache_manager_remove_buffer(struct pb_buffer *pb_buf)
   {
               /* the buffer won't be added if mgr is NULL */
      }
      /**
   * Actually destroy the buffer.
   */
   static void
   _pb_cache_buffer_destroy(void *winsys, struct pb_buffer *pb_buf)
   {
               assert(!pipe_is_referenced(&buf->base.reference));
   pb_reference(&buf->buffer, NULL);
      }
         static void
   pb_cache_buffer_destroy(void *winsys, struct pb_buffer *_buf)
   {
      struct pb_cache_buffer *buf = pb_cache_buffer(_buf);   
            if (!mgr) {
      pb_reference(&buf->buffer, NULL);
   FREE(buf);
                  }
         static void *
   pb_cache_buffer_map(struct pb_buffer *_buf, 
         {
      struct pb_cache_buffer *buf = pb_cache_buffer(_buf);   
      }
         static void
   pb_cache_buffer_unmap(struct pb_buffer *_buf)
   {
      struct pb_cache_buffer *buf = pb_cache_buffer(_buf);   
      }
         static enum pipe_error 
   pb_cache_buffer_validate(struct pb_buffer *_buf, 
               {
      struct pb_cache_buffer *buf = pb_cache_buffer(_buf);
      }
         static void
   pb_cache_buffer_fence(struct pb_buffer *_buf, 
         {
      struct pb_cache_buffer *buf = pb_cache_buffer(_buf);
      }
         static void
   pb_cache_buffer_get_base_buffer(struct pb_buffer *_buf,
               {
      struct pb_cache_buffer *buf = pb_cache_buffer(_buf);
      }
         const struct pb_vtbl 
   pb_cache_buffer_vtbl = {
         pb_cache_buffer_destroy,
   pb_cache_buffer_map,
   pb_cache_buffer_unmap,
   pb_cache_buffer_validate,
   pb_cache_buffer_fence,
   };
         static bool
   pb_cache_can_reclaim_buffer(void *winsys, struct pb_buffer *_buf)
   {
               if (buf->mgr->provider->is_buffer_busy) {
      if (buf->mgr->provider->is_buffer_busy(buf->mgr->provider, buf->buffer))
      } else {
               if (!ptr)
                           }
         static struct pb_buffer *
   pb_cache_manager_create_buffer(struct pb_manager *_mgr, 
               {
      struct pb_cache_manager *mgr = pb_cache_manager(_mgr);
                     /* get a buffer from the cache */
   buf = (struct pb_cache_buffer *)
         pb_cache_reclaim_buffer(&mgr->cache, aligned_size, desc->alignment,
   if (buf)
            /* create a new one */
   buf = CALLOC_STRUCT(pb_cache_buffer);
   if (!buf)
                     /* Empty the cache and try again. */
   if (!buf->buffer) {
      pb_cache_release_all_buffers(&mgr->cache);
               if(!buf->buffer) {
      FREE(buf);
      }
      assert(pipe_is_referenced(&buf->buffer->reference));
   assert(pb_check_alignment(desc->alignment, 1u << buf->buffer->alignment_log2));
   assert(buf->buffer->size >= aligned_size);
      pipe_reference_init(&buf->base.reference, 1);
   buf->base.alignment_log2 = buf->buffer->alignment_log2;
   buf->base.usage = buf->buffer->usage;
   buf->base.size = buf->buffer->size;
      buf->base.vtbl = &pb_cache_buffer_vtbl;
   buf->mgr = mgr;
   pb_cache_init_entry(&mgr->cache, &buf->cache_entry, &buf->base, 0);
         }
         static void
   pb_cache_manager_flush(struct pb_manager *_mgr)
   {
               pb_cache_release_all_buffers(&mgr->cache);
      assert(mgr->provider->flush);
   if(mgr->provider->flush)
      }
         static void
   pb_cache_manager_destroy(struct pb_manager *_mgr)
   {
               pb_cache_deinit(&mgr->cache);
      }
      /**
   * Create a caching buffer manager
   *
   * @param provider The buffer manager to which cache miss buffer requests
   * should be redirected.
   * @param usecs Unused buffers may be released from the cache after this
   * time
   * @param size_factor Declare buffers that are size_factor times bigger than
   * the requested size as cache hits.
   * @param bypass_usage Bitmask. If (requested usage & bypass_usage) != 0,
   * buffer allocation requests are redirected to the provider.
   * @param maximum_cache_size  Maximum size of all unused buffers the cache can
   * hold.
   */
   struct pb_manager *
   pb_cache_manager_create(struct pb_manager *provider, 
                           {
               if (!provider)
            mgr = CALLOC_STRUCT(pb_cache_manager);
   if (!mgr)
            mgr->base.destroy = pb_cache_manager_destroy;
   mgr->base.create_buffer = pb_cache_manager_create_buffer;
   mgr->base.flush = pb_cache_manager_flush;
   mgr->provider = provider;
   pb_cache_init(&mgr->cache, 1, usecs, size_factor, bypass_usage,
               maximum_cache_size, NULL,
      }
