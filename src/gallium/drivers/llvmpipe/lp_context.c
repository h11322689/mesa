   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   * Copyright 2008 VMware, Inc.  All rights reserved.
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
      #include "draw/draw_context.h"
   #include "draw/draw_vbuf.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/list.h"
   #include "util/u_upload_mgr.h"
   #include "lp_clear.h"
   #include "lp_context.h"
   #include "lp_flush.h"
   #include "lp_perf.h"
   #include "lp_state.h"
   #include "lp_surface.h"
   #include "lp_query.h"
   #include "lp_setup.h"
   #include "lp_screen.h"
   #include "lp_fence.h"
      static void
   llvmpipe_destroy(struct pipe_context *pipe)
   {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct llvmpipe_screen *lp_screen = llvmpipe_screen(pipe->screen);
            mtx_lock(&lp_screen->ctx_mutex);
   list_del(&llvmpipe->list);
   mtx_unlock(&lp_screen->ctx_mutex);
            if (llvmpipe->csctx) {
         }
   if (llvmpipe->task_ctx) {
         }
   if (llvmpipe->mesh_ctx) {
         }
   if (llvmpipe->blitter) {
                  if (llvmpipe->pipe.stream_uploader)
            /* This will also destroy llvmpipe->setup:
   */
   if (llvmpipe->draw)
                     for (enum pipe_shader_type s = PIPE_SHADER_VERTEX; s < PIPE_SHADER_MESH_TYPES; s++) {
      for (i = 0; i < ARRAY_SIZE(llvmpipe->sampler_views[0]); i++) {
         }
   for (i = 0; i < LP_MAX_TGSI_SHADER_IMAGES; i++) {
         }
   for (i = 0; i < LP_MAX_TGSI_SHADER_BUFFERS; i++) {
         }
   for (i = 0; i < ARRAY_SIZE(llvmpipe->constants[s]); i++) {
                     for (i = 0; i < llvmpipe->num_vertex_buffers; i++) {
                                 #ifndef USE_GLOBAL_LLVM_CONTEXT
         #endif
                  }
         static void
   do_flush(struct pipe_context *pipe,
               {
         }
         static void
   llvmpipe_fence_server_sync(struct pipe_context *pipe,
         {
               if (!f->issued)
            }
         static void
   llvmpipe_render_condition(struct pipe_context *pipe,
                     {
               llvmpipe->render_cond_query = query;
   llvmpipe->render_cond_mode = mode;
      }
         static void
   llvmpipe_render_condition_mem(struct pipe_context *pipe,
                     {
               llvmpipe->render_cond_buffer = llvmpipe_resource(buffer);
   llvmpipe->render_cond_offset = offset;
      }
         static void
   llvmpipe_texture_barrier(struct pipe_context *pipe, unsigned flags)
   {
         }
         static void
   lp_draw_disk_cache_find_shader(void *cookie,
               {
      struct llvmpipe_screen *screen = cookie;
      }
         static void
   lp_draw_disk_cache_insert_shader(void *cookie,
               {
      struct llvmpipe_screen *screen = cookie;
      }
         static enum pipe_reset_status
   llvmpipe_get_device_reset_status(struct pipe_context *pipe)
   {
         }
         struct pipe_context *
   llvmpipe_create_context(struct pipe_screen *screen, void *priv,
         {
      struct llvmpipe_context *llvmpipe;
            if (!llvmpipe_screen_late_init(lp_screen))
            llvmpipe = align_malloc(sizeof(struct llvmpipe_context), 16);
   if (!llvmpipe)
                                                llvmpipe->pipe.screen = screen;
            /* Init the pipe context methods */
   llvmpipe->pipe.destroy = llvmpipe_destroy;
   llvmpipe->pipe.set_framebuffer_state = llvmpipe_set_framebuffer_state;
   llvmpipe->pipe.clear = llvmpipe_clear;
   llvmpipe->pipe.flush = do_flush;
            llvmpipe->pipe.render_condition = llvmpipe_render_condition;
            llvmpipe->pipe.fence_server_sync = llvmpipe_fence_server_sync;
   llvmpipe->pipe.get_device_reset_status = llvmpipe_get_device_reset_status;
   llvmpipe_init_blend_funcs(llvmpipe);
   llvmpipe_init_clip_funcs(llvmpipe);
   llvmpipe_init_draw_funcs(llvmpipe);
   llvmpipe_init_compute_funcs(llvmpipe);
   llvmpipe_init_sampler_funcs(llvmpipe);
   llvmpipe_init_query_funcs(llvmpipe);
   llvmpipe_init_vertex_funcs(llvmpipe);
   llvmpipe_init_so_funcs(llvmpipe);
   llvmpipe_init_fs_funcs(llvmpipe);
   llvmpipe_init_vs_funcs(llvmpipe);
   llvmpipe_init_gs_funcs(llvmpipe);
   llvmpipe_init_tess_funcs(llvmpipe);
   llvmpipe_init_task_funcs(llvmpipe);
   llvmpipe_init_mesh_funcs(llvmpipe);
   llvmpipe_init_rasterizer_funcs(llvmpipe);
   llvmpipe_init_context_resource_funcs(&llvmpipe->pipe);
                  #ifdef USE_GLOBAL_LLVM_CONTEXT
         #else
         #endif
         if (!llvmpipe->context)
         #if LLVM_VERSION_MAJOR == 15
         #endif
         /*
   * Create drawing context and plug our rendering stage into it.
   */
   llvmpipe->draw = draw_create_with_llvm_context(&llvmpipe->pipe,
         if (!llvmpipe->draw)
            draw_set_disk_cache_callbacks(llvmpipe->draw,
                        draw_set_constant_buffer_stride(llvmpipe->draw,
                     llvmpipe->setup = lp_setup_create(&llvmpipe->pipe, llvmpipe->draw);
   if (!llvmpipe->setup)
            llvmpipe->csctx = lp_csctx_create(&llvmpipe->pipe);
   if (!llvmpipe->csctx)
            llvmpipe->task_ctx = lp_csctx_create(&llvmpipe->pipe);
   if (!llvmpipe->task_ctx)
            llvmpipe->mesh_ctx = lp_csctx_create(&llvmpipe->pipe);
   if (!llvmpipe->mesh_ctx)
            llvmpipe->pipe.stream_uploader = u_upload_create_default(&llvmpipe->pipe);
   if (!llvmpipe->pipe.stream_uploader)
                     llvmpipe->blitter = util_blitter_create(&llvmpipe->pipe);
   if (!llvmpipe->blitter) {
                  /* must be done before installing Draw stages */
            /* plug in AA line/point stages */
   draw_install_aaline_stage(llvmpipe->draw, &llvmpipe->pipe);
   draw_install_aapoint_stage(llvmpipe->draw, &llvmpipe->pipe, nir_type_bool32);
            /* convert points and lines into triangles:
   * (otherwise, draw points and lines natively)
   */
   draw_wide_point_sprites(llvmpipe->draw, false);
   draw_enable_point_sprites(llvmpipe->draw, false);
   draw_wide_point_threshold(llvmpipe->draw, 10000.0);
            /* initial state for clipping - enabled, with no guardband */
                     /* If llvmpipe_set_scissor_states() is never called, we still need to
   * make sure that derived scissor state is computed.
   * See https://bugs.freedesktop.org/show_bug.cgi?id=101709
   */
            mtx_lock(&lp_screen->ctx_mutex);
   list_addtail(&llvmpipe->list, &lp_screen->ctx_list);
   mtx_unlock(&lp_screen->ctx_mutex);
         fail:
      llvmpipe_destroy(&llvmpipe->pipe);
      }
