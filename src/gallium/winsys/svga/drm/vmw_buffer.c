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
      /**
   * @file
   * SVGA buffer manager for DMA buffers.
   * 
   * DMA buffers are used for pixel and vertex data upload/download to/from
   * the virtual SVGA hardware.
   *
   * This file implements a pipebuffer library's buffer manager, so that we can
   * use pipepbuffer's suballocation, fencing, and debugging facilities with
   * DMA buffers.
   * 
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "svga_cmd.h"
      #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "pipebuffer/pb_buffer.h"
   #include "pipebuffer/pb_bufmgr.h"
      #include "svga_winsys.h"
      #include "vmw_screen.h"
   #include "vmw_buffer.h"
      struct vmw_dma_bufmgr;
         struct vmw_dma_buffer
   {
      struct pb_buffer base;
      struct vmw_dma_bufmgr *mgr;
      struct vmw_region *region;
   void *map;
   unsigned map_flags;
      };
         extern const struct pb_vtbl vmw_dma_buffer_vtbl;
         static inline struct vmw_dma_buffer *
   vmw_pb_to_dma_buffer(struct pb_buffer *buf)
   {
      assert(buf);
   assert(buf->vtbl == &vmw_dma_buffer_vtbl);
      }
         struct vmw_dma_bufmgr
   {
      struct pb_manager base;
         };
         static inline struct vmw_dma_bufmgr *
   vmw_pb_to_dma_bufmgr(struct pb_manager *mgr)
   {
               /* Make sure our extra flags don't collide with pipebuffer's flags */
   STATIC_ASSERT((VMW_BUFFER_USAGE_SHARED & PB_USAGE_ALL) == 0);
               }
         static void
   vmw_dma_buffer_destroy(void *winsys, struct pb_buffer *_buf)
   {
               assert(buf->map_count == 0);
   if (buf->map) {
      assert(buf->mgr->vws->cache_maps);
                           }
         static void *
   vmw_dma_buffer_map(struct pb_buffer *_buf,
               {
      struct vmw_dma_buffer *buf = vmw_pb_to_dma_buffer(_buf);
            if (!buf->map)
            if (!buf->map)
            if ((_buf->usage & VMW_BUFFER_USAGE_SYNC) &&
      !(flags & PB_USAGE_UNSYNCHRONIZED)) {
   ret = vmw_ioctl_syncforcpu(buf->region,
                     if (ret)
               buf->map_count++;
      }
         static void
   vmw_dma_buffer_unmap(struct pb_buffer *_buf)
   {
      struct vmw_dma_buffer *buf = vmw_pb_to_dma_buffer(_buf);
            if ((_buf->usage & VMW_BUFFER_USAGE_SYNC) &&
      !(flags & PB_USAGE_UNSYNCHRONIZED)) {
   vmw_ioctl_releasefromcpu(buf->region,
                     assert(buf->map_count > 0);
   if (!--buf->map_count && !buf->mgr->vws->cache_maps) {
      vmw_ioctl_region_unmap(buf->region);
         }
         static void
   vmw_dma_buffer_get_base_buffer(struct pb_buffer *buf,
               {
      *base_buf = buf;
      }
         static enum pipe_error
   vmw_dma_buffer_validate( struct pb_buffer *_buf,
               {
      /* Always pinned */
      }
         static void
   vmw_dma_buffer_fence( struct pb_buffer *_buf,
         {
      /* We don't need to do anything, as the pipebuffer library
      }
         const struct pb_vtbl vmw_dma_buffer_vtbl = {
      .destroy = vmw_dma_buffer_destroy,
   .map = vmw_dma_buffer_map,
   .unmap = vmw_dma_buffer_unmap,
   .validate = vmw_dma_buffer_validate,
   .fence = vmw_dma_buffer_fence,
      };
         static struct pb_buffer *
   vmw_dma_bufmgr_create_buffer(struct pb_manager *_mgr,
               {
      struct vmw_dma_bufmgr *mgr = vmw_pb_to_dma_bufmgr(_mgr);
   struct vmw_winsys_screen *vws = mgr->vws;
   struct vmw_dma_buffer *buf;
   const struct vmw_buffer_desc *desc =
            buf = CALLOC_STRUCT(vmw_dma_buffer);
   if(!buf)
            pipe_reference_init(&buf->base.reference, 1);
   buf->base.alignment_log2 = util_logbase2(pb_desc->alignment);
   buf->base.usage = pb_desc->usage & ~VMW_BUFFER_USAGE_SHARED;
   buf->base.vtbl = &vmw_dma_buffer_vtbl;
   buf->mgr = mgr;
   buf->base.size = size;
   if ((pb_desc->usage & VMW_BUFFER_USAGE_SHARED) && desc->region) {
         } else {
      buf->region = vmw_ioctl_region_create(vws, size);
   goto error2;
      }
         error2:
         error1:
         }
         static void
   vmw_dma_bufmgr_flush(struct pb_manager *mgr)
   {
         }
         static void
   vmw_dma_bufmgr_destroy(struct pb_manager *_mgr)
   {
      struct vmw_dma_bufmgr *mgr = vmw_pb_to_dma_bufmgr(_mgr);
      }
         struct pb_manager *
   vmw_dma_bufmgr_create(struct vmw_winsys_screen *vws)
   {
      struct vmw_dma_bufmgr *mgr;
      mgr = CALLOC_STRUCT(vmw_dma_bufmgr);
   if(!mgr)
            mgr->base.destroy = vmw_dma_bufmgr_destroy;
   mgr->base.create_buffer = vmw_dma_bufmgr_create_buffer;
   mgr->base.flush = vmw_dma_bufmgr_flush;
      mgr->vws = vws;
         }
         bool
   vmw_dma_bufmgr_region_ptr(struct pb_buffer *buf,
         {
      struct pb_buffer *base_buf;
   pb_size offset = 0;
   struct vmw_dma_buffer *dma_buf;
      pb_get_base_buffer( buf, &base_buf, &offset );
      dma_buf = vmw_pb_to_dma_buffer(base_buf);
   if(!dma_buf)
            *ptr = vmw_ioctl_region_ptr(dma_buf->region);
      ptr->offset += offset;
         }
      #ifdef DEBUG
   struct svga_winsys_buffer {
      struct pb_buffer *pb_buf;
      };
      struct pb_buffer *
   vmw_pb_buffer(struct svga_winsys_buffer *buffer)
   {
      assert(buffer);
      }
      struct svga_winsys_buffer *
   vmw_svga_winsys_buffer_wrap(struct pb_buffer *buffer)
   {
               if (!buffer)
            buf = CALLOC_STRUCT(svga_winsys_buffer);
   if (!buf) {
      pb_reference(&buffer, NULL);
               buf->pb_buf = buffer;
   buf->fbuf = debug_flush_buf_create(false, VMW_DEBUG_FLUSH_STACK);
      }
      struct debug_flush_buf *
   vmw_debug_flush_buf(struct svga_winsys_buffer *buffer)
   {
         }
      #endif
      void
   vmw_svga_winsys_buffer_destroy(struct svga_winsys_screen *sws,
         {
      struct pb_buffer *pbuf = vmw_pb_buffer(buf);
   (void)sws;
      #ifdef DEBUG
      debug_flush_buf_reference(&buf->fbuf, NULL);
      #endif
   }
      void *
   vmw_svga_winsys_buffer_map(struct svga_winsys_screen *sws,
               {
      void *map;
            (void)sws;
   if (flags & PIPE_MAP_UNSYNCHRONIZED)
            if (flags & PIPE_MAP_READ)
         if (flags & PIPE_MAP_WRITE)
         if (flags & PIPE_MAP_DIRECTLY)
         if (flags & PIPE_MAP_DONTBLOCK)
         if (flags & PIPE_MAP_UNSYNCHRONIZED)
         if (flags & PIPE_MAP_PERSISTENT)
                  #ifdef DEBUG
      if (map != NULL)
      #endif
            }
         void
   vmw_svga_winsys_buffer_unmap(struct svga_winsys_screen *sws,
         {
            #ifdef DEBUG
         #endif
            }
