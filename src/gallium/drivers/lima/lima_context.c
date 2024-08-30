   /*
   * Copyright (c) 2017-2019 Lima Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   */
      #include "util/u_memory.h"
   #include "util/u_blitter.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_math.h"
   #include "util/u_debug.h"
   #include "util/ralloc.h"
   #include "util/u_inlines.h"
   #include "util/u_debug_cb.h"
   #include "util/hash_table.h"
      #include "lima_screen.h"
   #include "lima_context.h"
   #include "lima_resource.h"
   #include "lima_bo.h"
   #include "lima_job.h"
   #include "lima_util.h"
   #include "lima_fence.h"
      #include <drm-uapi/lima_drm.h>
   #include <xf86drm.h>
      int lima_ctx_num_plb = LIMA_CTX_PLB_DEF_NUM;
      uint32_t
   lima_ctx_buff_va(struct lima_context *ctx, enum lima_ctx_buff buff)
   {
      struct lima_job *job = lima_job_get(ctx);
   struct lima_ctx_buff_state *cbs = ctx->buffer_state + buff;
   struct lima_resource *res = lima_resource(cbs->res);
                        }
      void *
   lima_ctx_buff_map(struct lima_context *ctx, enum lima_ctx_buff buff)
   {
      struct lima_ctx_buff_state *cbs = ctx->buffer_state + buff;
               }
      void *
   lima_ctx_buff_alloc(struct lima_context *ctx, enum lima_ctx_buff buff,
         {
      struct lima_ctx_buff_state *cbs = ctx->buffer_state + buff;
                     u_upload_alloc(ctx->uploader, 0, cbs->size, 0x40, &cbs->offset,
               }
      static int
   lima_context_create_drm_ctx(struct lima_screen *screen)
   {
               int ret = drmIoctl(screen->fd, DRM_IOCTL_LIMA_CTX_CREATE, &req);
   if (ret)
               }
      static void
   lima_context_free_drm_ctx(struct lima_screen *screen, int id)
   {
      struct drm_lima_ctx_free req = {
                     }
      static void
   lima_invalidate_resource(struct pipe_context *pctx, struct pipe_resource *prsc)
   {
               struct hash_entry *entry = _mesa_hash_table_search(ctx->write_jobs, prsc);
   if (!entry)
            struct lima_job *job = entry->data;
   if (job->key.zsbuf && (job->key.zsbuf->texture == prsc))
            if (job->key.cbuf && (job->key.cbuf->texture == prsc))
               }
      static void
   plb_pp_stream_delete_fn(struct hash_entry *entry)
   {
               lima_bo_unreference(s->bo);
   list_del(&s->lru_list);
      }
      static void
   lima_context_destroy(struct pipe_context *pctx)
   {
      struct lima_context *ctx = lima_context(pctx);
            if (ctx->jobs)
            for (int i = 0; i < lima_ctx_buff_num; i++)
            lima_program_fini(ctx);
   lima_state_fini(ctx);
            if (ctx->blitter)
            if (ctx->uploader)
                     for (int i = 0; i < LIMA_CTX_PLB_MAX_NUM; i++) {
      if (ctx->plb[i])
         if (ctx->gp_tile_heap[i])
               if (ctx->plb_gp_stream)
            if (ctx->gp_output)
            _mesa_hash_table_destroy(ctx->plb_pp_stream,
                        }
      static uint32_t
   plb_pp_stream_hash(const void *key)
   {
         }
      static bool
   plb_pp_stream_compare(const void *key1, const void *key2)
   {
         }
      struct pipe_context *
   lima_context_create(struct pipe_screen *pscreen, void *priv, unsigned flags)
   {
      struct lima_screen *screen = lima_screen(pscreen);
            ctx = rzalloc(NULL, struct lima_context);
   if (!ctx)
            ctx->id = lima_context_create_drm_ctx(screen);
   if (ctx->id < 0) {
      ralloc_free(ctx);
                        ctx->base.screen = pscreen;
   ctx->base.destroy = lima_context_destroy;
   ctx->base.set_debug_callback = u_default_set_debug_callback;
            lima_resource_context_init(ctx);
   lima_fence_context_init(ctx);
   lima_state_init(ctx);
   lima_draw_init(ctx);
   lima_program_init(ctx);
                     ctx->blitter = util_blitter_create(&ctx->base);
   if (!ctx->blitter)
            ctx->uploader = u_upload_create_default(&ctx->base);
   if (!ctx->uploader)
         ctx->base.stream_uploader = ctx->uploader;
            ctx->plb_size = screen->plb_max_blk * LIMA_CTX_PLB_BLK_SIZE;
            uint32_t heap_flags;
   if (screen->has_growable_heap_buffer) {
      /* growable size buffer, initially will allocate 32K (by default)
   * backup memory in kernel driver, and will allocate more when GP
   * get out of memory interrupt. Max to 16M set here.
   */
   ctx->gp_tile_heap_size = 0x1000000;
      } else {
      /* fix size buffer */
   ctx->gp_tile_heap_size = 0x100000;
               for (int i = 0; i < lima_ctx_num_plb; i++) {
      ctx->plb[i] = lima_bo_create(screen, ctx->plb_size, 0);
   if (!ctx->plb[i])
         ctx->gp_tile_heap[i] = lima_bo_create(screen, ctx->gp_tile_heap_size, heap_flags);
   if (!ctx->gp_tile_heap[i])
               unsigned plb_gp_stream_size =
         ctx->plb_gp_stream =
         if (!ctx->plb_gp_stream)
                  /* plb gp stream is static for any framebuffer */
   for (int i = 0; i < lima_ctx_num_plb; i++) {
      uint32_t *plb_gp_stream = ctx->plb_gp_stream->map + i * ctx->plb_gp_size;
   for (int j = 0; j < screen->plb_max_blk; j++)
               list_inithead(&ctx->plb_pp_stream_lru_list);
   ctx->plb_pp_stream = _mesa_hash_table_create(
         if (!ctx->plb_pp_stream)
            if (!lima_job_init(ctx))
                  err_out:
      lima_context_destroy(&ctx->base);
      }
