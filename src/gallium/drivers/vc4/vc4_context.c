   /*
   * Copyright © 2014 Broadcom
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <xf86drm.h>
   #include <err.h>
      #include "pipe/p_defines.h"
   #include "util/ralloc.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_blitter.h"
   #include "util/u_upload_mgr.h"
   #include "pipe/p_screen.h"
      #include "vc4_screen.h"
   #include "vc4_context.h"
   #include "vc4_resource.h"
      void
   vc4_flush(struct pipe_context *pctx)
   {
                  hash_table_foreach(vc4->jobs, entry) {
               }
      static void
   vc4_pipe_flush(struct pipe_context *pctx, struct pipe_fence_handle **fence,
         {
                           if (fence) {
                           if (flags & PIPE_FLUSH_FENCE_FD) {
                              struct vc4_fence *f = vc4_fence_create(vc4->screen,
                  }
      /* We can't flush the texture cache within rendering a tile, so we have to
   * flush all rendering to the kernel so that the next job reading from the
   * tile gets a flushed cache.
   */
   static void
   vc4_texture_barrier(struct pipe_context *pctx, unsigned flags)
   {
         }
      static void
   vc4_invalidate_resource(struct pipe_context *pctx, struct pipe_resource *prsc)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
                     struct hash_entry *entry = _mesa_hash_table_search(vc4->write_jobs,
         if (!entry)
            struct vc4_job *job = entry->data;
   if (job->key.zsbuf && job->key.zsbuf->texture == prsc)
   }
      static void
   vc4_context_destroy(struct pipe_context *pctx)
   {
                           if (vc4->blitter)
            if (vc4->uploader)
                              if (vc4->yuv_linear_blit_vs)
         if (vc4->yuv_linear_blit_fs_8bit)
         if (vc4->yuv_linear_blit_fs_16bit)
                     if (vc4->screen->has_syncobj) {
               }
   if (vc4->in_fence_fd >= 0)
            }
      struct pipe_context *
   vc4_context_create(struct pipe_screen *pscreen, void *priv, unsigned flags)
   {
         struct vc4_screen *screen = vc4_screen(pscreen);
   struct vc4_context *vc4;
            /* Prevent dumping of the shaders built during context setup. */
   uint32_t saved_shaderdb_flag = vc4_mesa_debug & VC4_DEBUG_SHADERDB;
            vc4 = rzalloc(NULL, struct vc4_context);
   if (!vc4)
                           pctx->screen = pscreen;
   pctx->priv = priv;
   pctx->destroy = vc4_context_destroy;
   pctx->flush = vc4_pipe_flush;
   pctx->set_debug_callback = u_default_set_debug_callback;
   pctx->invalidate_resource = vc4_invalidate_resource;
            vc4_draw_init(pctx);
   vc4_state_init(pctx);
   vc4_program_init(pctx);
   vc4_query_init(pctx);
                     err = vc4_job_init(vc4);
   if (err)
            err = vc4_fence_context_init(vc4);
   if (err)
               vc4->uploader = u_upload_create_default(&vc4->base);
   vc4->base.stream_uploader = vc4->uploader;
   vc4->base.const_uploader = vc4->uploader;
      vc4->blitter = util_blitter_create(pctx);
         if (!vc4->blitter)
                                 fail:
         pctx->destroy(pctx);
   }
