   /*
   * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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
      #include "freedreno_query_hw.h"
      #include "fd3_blend.h"
   #include "fd3_context.h"
   #include "fd3_draw.h"
   #include "fd3_emit.h"
   #include "fd3_gmem.h"
   #include "fd3_program.h"
   #include "fd3_query.h"
   #include "fd3_rasterizer.h"
   #include "fd3_texture.h"
   #include "fd3_zsa.h"
      static void
   fd3_context_destroy(struct pipe_context *pctx) in_dt
   {
               u_upload_destroy(fd3_ctx->border_color_uploader);
                     fd_bo_del(fd3_ctx->vs_pvt_mem);
   fd_bo_del(fd3_ctx->fs_pvt_mem);
                                 }
      struct pipe_context *
   fd3_context_create(struct pipe_screen *pscreen, void *priv,
         {
      struct fd_screen *screen = fd_screen(pscreen);
   struct fd3_context *fd3_ctx = CALLOC_STRUCT(fd3_context);
            if (!fd3_ctx)
            pctx = &fd3_ctx->base.base;
            fd3_ctx->base.flags = flags;
   fd3_ctx->base.dev = fd_device_ref(screen->dev);
   fd3_ctx->base.screen = fd_screen(pscreen);
            pctx->destroy = fd3_context_destroy;
   pctx->create_blend_state = fd3_blend_state_create;
   pctx->create_rasterizer_state = fd3_rasterizer_state_create;
            fd3_draw_init(pctx);
   fd3_gmem_init(pctx);
   fd3_texture_init(pctx);
   fd3_prog_init(pctx);
            pctx = fd_context_init(&fd3_ctx->base, pscreen, priv, flags);
   if (!pctx)
                     fd3_ctx->vs_pvt_mem =
            fd3_ctx->fs_pvt_mem =
            fd3_ctx->vsc_size_mem =
                              fd3_ctx->border_color_uploader =
               }
