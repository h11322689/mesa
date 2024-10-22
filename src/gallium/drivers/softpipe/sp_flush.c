   /**************************************************************************
   * 
   * Copyright 2007 VMware, Inc.
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
      /* Author:
   *    Keith Whitwell <keithw@vmware.com>
   */
         #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "draw/draw_context.h"
   #include "sp_flush.h"
   #include "sp_context.h"
   #include "sp_state.h"
   #include "sp_tile_cache.h"
   #include "sp_tex_tile_cache.h"
   #include "util/u_debug_image.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
         void
   softpipe_flush( struct pipe_context *pipe,
               {
      struct softpipe_context *softpipe = softpipe_context(pipe);
                     if (flags & SP_FLUSH_TEXTURE_CACHE) {
               for (sh = 0; sh < ARRAY_SIZE(softpipe->tex_cache); sh++) {
      for (i = 0; i < softpipe->num_sampler_views[sh]; i++) {
                        /* If this is a swapbuffers, just flush color buffers.
   *
   * The zbuffer changes are not discarded, but held in the cache
   * in the hope that a later clear will wipe them out.
   */
   for (i = 0; i < softpipe->framebuffer.nr_cbufs; i++)
      if (softpipe->cbuf_cache[i])
         if (softpipe->zsbuf_cache)
                        #if 0
      if (flags & PIPE_FLUSH_END_OF_FRAME) {
      static unsigned frame_no = 1;
   static char filename[256];
   snprintf(filename, sizeof(filename), "cbuf_%u.bmp", frame_no);
   debug_dump_surface_bmp(pipe, filename, softpipe->framebuffer.cbufs[0]);
   snprintf(filename, sizeof(filename), "zsbuf_%u.bmp", frame_no);
   debug_dump_surface_bmp(pipe, filename, softpipe->framebuffer.zsbuf);
         #endif
         if (fence)
      }
      void
   softpipe_flush_wrapped(struct pipe_context *pipe,
               {
         }
         /**
   * Flush context if necessary.
   *
   * Returns FALSE if it would have block, but do_not_block was set, TRUE
   * otherwise.
   *
   * TODO: move this logic to an auxiliary library?
   */
   bool
   softpipe_flush_resource(struct pipe_context *pipe,
                           struct pipe_resource *texture,
   unsigned level,
   int layer,
   {
                        if ((referenced & SP_REFERENCED_FOR_WRITE) ||
               /*
   * TODO: The semantics of these flush flags are too obtuse. They should
   * disappear and the pipe driver should just ensure that all visible
   * side-effects happen when they need to happen.
   */
   if (referenced & SP_REFERENCED_FOR_READ)
            if (cpu_access) {
      /*
                                                   if (fence) {
      /*
   * This is for illustrative purposes only, as softpipe does not
   * have fences.
   */
   pipe->screen->fence_finish(pipe->screen, NULL, fence,
               } else {
      /*
                                    }
      void softpipe_texture_barrier(struct pipe_context *pipe, unsigned flags)
   {
      struct softpipe_context *softpipe = softpipe_context(pipe);
            for (sh = 0; sh < ARRAY_SIZE(softpipe->tex_cache); sh++) {
      for (i = 0; i < softpipe->num_sampler_views[sh]; i++) {
                     for (i = 0; i < softpipe->framebuffer.nr_cbufs; i++)
      if (softpipe->cbuf_cache[i])
         if (softpipe->zsbuf_cache)
               }
      void softpipe_memory_barrier(struct pipe_context *pipe, unsigned flags)
   {
      if (!(flags & ~PIPE_BARRIER_UPDATE))
               }
