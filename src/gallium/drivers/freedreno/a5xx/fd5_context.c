   /*
   * Copyright (C) 2016 Rob Clark <robclark@freedesktop.org>
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
      #include "freedreno_query_acc.h"
      #include "fd5_blend.h"
   #include "fd5_blitter.h"
   #include "fd5_compute.h"
   #include "fd5_context.h"
   #include "fd5_draw.h"
   #include "fd5_emit.h"
   #include "fd5_gmem.h"
   #include "fd5_program.h"
   #include "fd5_query.h"
   #include "fd5_rasterizer.h"
   #include "fd5_texture.h"
   #include "fd5_zsa.h"
      static void
   fd5_context_destroy(struct pipe_context *pctx) in_dt
   {
               u_upload_destroy(fd5_ctx->border_color_uploader);
                     fd_bo_del(fd5_ctx->vsc_size_mem);
                        }
      struct pipe_context *
   fd5_context_create(struct pipe_screen *pscreen, void *priv,
         {
      struct fd_screen *screen = fd_screen(pscreen);
   struct fd5_context *fd5_ctx = CALLOC_STRUCT(fd5_context);
            if (!fd5_ctx)
            pctx = &fd5_ctx->base.base;
            fd5_ctx->base.flags = flags;
   fd5_ctx->base.dev = fd_device_ref(screen->dev);
   fd5_ctx->base.screen = fd_screen(pscreen);
            pctx->destroy = fd5_context_destroy;
   pctx->create_blend_state = fd5_blend_state_create;
   pctx->create_rasterizer_state = fd5_rasterizer_state_create;
            fd5_draw_init(pctx);
   fd5_compute_init(pctx);
   fd5_gmem_init(pctx);
   fd5_texture_init(pctx);
   fd5_prog_init(pctx);
            if (!FD_DBG(NOBLIT))
            pctx = fd_context_init(&fd5_ctx->base, pscreen, priv, flags);
   if (!pctx)
                     fd5_ctx->vsc_size_mem =
            fd5_ctx->blit_mem =
                              fd5_ctx->border_color_uploader =
               }
