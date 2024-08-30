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
         #include "svga_cmd.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "pipe/p_defines.h"
   #include "vmw_surface.h"
   #include "vmw_screen.h"
   #include "vmw_buffer.h"
   #include "vmw_context.h"
   #include "pipebuffer/pb_bufmgr.h"
      void
   vmw_svga_winsys_surface_init(struct svga_winsys_screen *sws,
               {
      struct vmw_svga_winsys_surface *vsrf = vmw_svga_winsys_surface(srf);
   void *data = NULL;
   struct pb_buffer *pb_buf;
   uint32_t pb_flags;
   struct vmw_winsys_screen *vws = vsrf->screen;
            struct pb_manager *provider;
            mtx_lock(&vsrf->mutex);
   data = vmw_svga_winsys_buffer_map(&vws->base, vsrf->buf, pb_flags);
   if (data)
            provider = vws->pools.dma_fenced;
   memset(&desc, 0, sizeof(desc));
   desc.alignment = 4096;
   pb_buf = provider->create_buffer(provider, vsrf->size, &desc);
   if (pb_buf != NULL) {
      struct svga_winsys_buffer *vbuf =
            data = vmw_svga_winsys_buffer_map(&vws->base, vbuf, pb_flags);
   if (data) {
      vsrf->rebind = true;
   if (vsrf->buf)
         vsrf->buf = vbuf;
      } else {
      vmw_svga_winsys_buffer_destroy(&vws->base, vbuf);
         }
   else {
      /* Cannot create a buffer, just unlock */
            out_mapped:
               if (data) {
      if (flags & SVGA3D_SURFACE_BIND_STREAM_OUTPUT) {
         }
   else {
                     mtx_lock(&vsrf->mutex);
      out_unlock:
         }
         void *
   vmw_svga_winsys_surface_map(struct svga_winsys_context *swc,
                     {
      struct vmw_svga_winsys_surface *vsrf = vmw_svga_winsys_surface(srf);
   void *data = NULL;
   struct pb_buffer *pb_buf;
   uint32_t pb_flags;
            *retry = false;
   *rebind = false;
   assert((flags & (PIPE_MAP_READ | PIPE_MAP_WRITE)) != 0);
            if (vsrf->mapcount) {
      /* Other mappers will get confused if we discard. */
                        /*
   * If we intend to read, there's no point discarding the
   * data if busy.
   */
   if (flags & PIPE_MAP_READ || vsrf->shared)
            /*
   * Discard is a hint to a synchronized map.
   */
   if (flags & PIPE_MAP_DISCARD_WHOLE_RESOURCE)
            /*
   * The surface is allowed to be referenced on the command stream iff
   * we're mapping unsynchronized or discard. This is an early check.
   * We need to recheck after a failing discard map.
   */
   if (!(flags & (PIPE_MAP_DISCARD_WHOLE_RESOURCE |
            p_atomic_read(&vsrf->validated)) {
   *retry = true;
               pb_flags = flags & (PIPE_MAP_READ_WRITE | PIPE_MAP_UNSYNCHRONIZED |
            if (flags & PIPE_MAP_DISCARD_WHOLE_RESOURCE) {
      struct pb_manager *provider;
            /*
   * First, if possible, try to map existing storage with DONTBLOCK.
   */
   if (!p_atomic_read(&vsrf->validated)) {
      data = vmw_svga_winsys_buffer_map(&vws->base, vsrf->buf,
         if (data)
               /*
   * Attempt to get a new buffer.
   */
   provider = vws->pools.dma_fenced;
   memset(&desc, 0, sizeof(desc));
   desc.alignment = 4096;
   pb_buf = provider->create_buffer(provider, vsrf->size, &desc);
   if (pb_buf != NULL) {
                     data = vmw_svga_winsys_buffer_map(&vws->base, vbuf, pb_flags);
   if (data) {
      vsrf->rebind = true;
   /*
   * We've discarded data on this surface and thus
   * it's data is no longer consider referenced.
   */
   vmw_swc_surface_clear_reference(swc, vsrf);
   if (vsrf->buf)
                  /* Rebind persistent maps immediately */
   if (flags & PIPE_MAP_PERSISTENT) {
      *rebind = true;
      }
      } else
      }
   /*
   * We couldn't get and map a new buffer for some reason.
   * Fall through to an ordinary map.
   * But tell pipe driver to flush now if already on validate list,
   * Otherwise we'll overwrite previous contents.
   */
   if (!(flags & PIPE_MAP_UNSYNCHRONIZED) && 
      p_atomic_read(&vsrf->validated)) {
   *retry = true;
                  pb_flags |= (flags & PIPE_MAP_DONTBLOCK);
   data = vmw_svga_winsys_buffer_map(&vws->base, vsrf->buf, pb_flags);
   if (data == NULL)
         out_mapped:
      ++vsrf->mapcount;
   vsrf->data = data;
      out_unlock:
      mtx_unlock(&vsrf->mutex);
      }
         void
   vmw_svga_winsys_surface_unmap(struct svga_winsys_context *swc,
               {
      struct vmw_svga_winsys_surface *vsrf = vmw_svga_winsys_surface(srf);
   mtx_lock(&vsrf->mutex);
   if (--vsrf->mapcount == 0) {
      *rebind = vsrf->rebind;
      } else {
         }
   vmw_svga_winsys_buffer_unmap(&vsrf->screen->base, vsrf->buf);
      }
      void
   vmw_svga_winsys_surface_reference(struct vmw_svga_winsys_surface **pdst,
         {
      struct pipe_reference *src_ref;
   struct pipe_reference *dst_ref;
            if(pdst == NULL || *pdst == src)
                     src_ref = src ? &src->refcnt : NULL;
            if (pipe_reference(dst_ref, src_ref)) {
      if (dst->buf)
         #ifdef DEBUG
         /* to detect dangling pointers */
   assert(p_atomic_read(&dst->validated) == 0);
   #endif
         mtx_destroy(&dst->mutex);
                  }
