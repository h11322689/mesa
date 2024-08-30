   /*
   * Copyright © 2014-2017 Broadcom
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
   #include "util/hash_table.h"
   #include "util/ralloc.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_blitter.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_prim.h"
   #include "util/u_debug_cb.h"
   #include "pipe/p_screen.h"
      #include "v3d_screen.h"
   #include "v3d_context.h"
   #include "v3d_resource.h"
   #include "broadcom/compiler/v3d_compiler.h"
   #include "broadcom/common/v3d_util.h"
      void
   v3d_flush(struct pipe_context *pctx)
   {
                  hash_table_foreach(v3d->jobs, entry) {
               }
      static void
   v3d_pipe_flush(struct pipe_context *pctx, struct pipe_fence_handle **fence,
         {
                           if (fence) {
            struct pipe_screen *screen = pctx->screen;
   struct v3d_fence *f = v3d_fence_create(v3d);
      }
      static void
   v3d_memory_barrier(struct pipe_context *pctx, unsigned int flags)
   {
                  /* We only need to flush for SSBOs and images, because for everything
      * else we flush the job automatically when we needed.
      const unsigned int flush_flags = PIPE_BARRIER_SHADER_BUFFER |
      if (!(flags & flush_flags))
   return;
            /* We only need to flush jobs writing to SSBOs/images. */
   perf_debug("Flushing all jobs for glMemoryBarrier(), could do better");
   }
      static void
   v3d_invalidate_resource(struct pipe_context *pctx, struct pipe_resource *prsc)
   {
         struct v3d_context *v3d = v3d_context(pctx);
                     struct hash_entry *entry = _mesa_hash_table_search(v3d->write_jobs,
         if (!entry)
            struct v3d_job *job = entry->data;
   if (job->key.zsbuf && job->key.zsbuf->texture == prsc)
   }
      /**
   * Flushes the current job to get up-to-date primitive counts written to the
   * primitive counts BO, then accumulates the transform feedback primitive count
   * in the context and the corresponding vertex counts in the bound stream
   * output targets.
   */
   void
   v3d_update_primitive_counters(struct v3d_context *v3d)
   {
         struct v3d_job *job = v3d_get_job_for_fbo(v3d);
   if (job->draw_calls_queued == 0)
            /* In order to get up-to-date primitive counts we need to submit
      * the job for execution so we get the counts written to memory.
   * Notice that this will require a sync wait for the buffer write.
      uint32_t prims_before = v3d->tf_prims_generated;
   v3d_job_submit(v3d, job);
   uint32_t prims_after = v3d->tf_prims_generated;
   if (prims_before == prims_after)
            enum mesa_prim prim_type = u_base_prim_type(v3d->prim_mode);
   uint32_t num_verts = u_vertices_for_prims(prim_type,
         for (int i = 0; i < v3d->streamout.num_targets; i++) {
            struct v3d_stream_output_target *so =
      }
      bool
   v3d_line_smoothing_enabled(struct v3d_context *v3d)
   {
         if (!v3d->rasterizer->base.line_smooth)
            /* According to the OpenGL docs, line smoothing shouldn’t be applied
      * when multisampling
      if (v3d->job->msaa || v3d->rasterizer->base.multisample)
            if (v3d->framebuffer.nr_cbufs <= 0)
            struct pipe_surface *cbuf = v3d->framebuffer.cbufs[0];
   if (!cbuf)
            /* Modifying the alpha for pure integer formats probably
      * doesn’t make sense because we don’t know how the application
   * uses the alpha value.
      if (util_format_is_pure_integer(cbuf->format))
            }
      float
   v3d_get_real_line_width(struct v3d_context *v3d)
   {
                  if (v3d_line_smoothing_enabled(v3d)) {
            /* If line smoothing is enabled then we want to add some extra
   * pixels to the width in order to have some semi-transparent
   * edges.
               }
      void
   v3d_ensure_prim_counts_allocated(struct v3d_context *ctx)
   {
         if (ctx->prim_counts)
            /* Init all 7 counters and 1 padding to 0 */
   uint32_t zeroes[8] = { 0 };
   u_upload_data(ctx->uploader,
               }
      void
   v3d_flag_dirty_sampler_state(struct v3d_context *v3d,
         {
         switch (shader) {
   case PIPE_SHADER_VERTEX:
               case PIPE_SHADER_GEOMETRY:
               case PIPE_SHADER_FRAGMENT:
               case PIPE_SHADER_COMPUTE:
               default:
         }
      void
   v3d_get_tile_buffer_size(const struct v3d_device_info *devinfo,
                           bool is_msaa,
   bool double_buffer,
   uint32_t nr_cbufs,
   struct pipe_surface **cbufs,
   {
                  uint32_t max_cbuf_idx = 0;
   uint32_t total_bpp = 0;
   *max_bpp = 0;
   for (int i = 0; i < nr_cbufs; i++) {
            if (cbufs[i]) {
            struct v3d_surface *surf = v3d_surface(cbufs[i]);
   *max_bpp = MAX2(*max_bpp, surf->internal_bpp);
            if (bbuf) {
            struct v3d_surface *bsurf = v3d_surface(bbuf);
   assert(bbuf->texture->nr_samples <= 1 || is_msaa);
               v3d_choose_tile_size(devinfo, max_cbuf_idx + 1,
               }
      static void
   v3d_context_destroy(struct pipe_context *pctx)
   {
                           if (v3d->blitter)
            if (v3d->uploader)
         if (v3d->state_uploader)
            if (v3d->prim_counts)
                              if (v3d->sand8_blit_vs)
         if (v3d->sand8_blit_fs_luma)
         if (v3d->sand8_blit_fs_chroma)
         if (v3d->sand30_blit_vs)
         if (v3d->sand30_blit_fs)
                     }
      static void
   v3d_get_sample_position(struct pipe_context *pctx,
               {
                  if (sample_count <= 1) {
               } else {
            static const int xoffsets_v33[] = { 1, -3, 3, -1 };
                           }
      bool
   v3d_render_condition_check(struct v3d_context *v3d)
   {
         if (!v3d->cond_query)
                     union pipe_query_result res = { 0 };
   bool wait =
                  struct pipe_context *pctx = (struct pipe_context *)v3d;
   if (pctx->get_query_result(pctx, v3d->cond_query, wait, &res))
            }
      struct pipe_context *
   v3d_context_create(struct pipe_screen *pscreen, void *priv, unsigned flags)
   {
         struct v3d_screen *screen = v3d_screen(pscreen);
   struct v3d_context *v3d;
            /* Prevent dumping of the shaders built during context setup. */
   uint32_t saved_shaderdb_flag = v3d_mesa_debug & V3D_DEBUG_SHADERDB;
            v3d = rzalloc(NULL, struct v3d_context);
   if (!v3d)
                           int ret = drmSyncobjCreate(screen->fd, DRM_SYNCOBJ_CREATE_SIGNALED,
         if (ret) {
                        pctx->screen = pscreen;
   pctx->priv = priv;
   pctx->destroy = v3d_context_destroy;
   pctx->flush = v3d_pipe_flush;
   pctx->memory_barrier = v3d_memory_barrier;
   pctx->set_debug_callback = u_default_set_debug_callback;
   pctx->invalidate_resource = v3d_invalidate_resource;
            v3d_X(devinfo, draw_init)(pctx);
   v3d_X(devinfo, state_init)(pctx);
   v3d_program_init(pctx);
   v3d_query_init(pctx);
                                       v3d->uploader = u_upload_create_default(&v3d->base);
   v3d->base.stream_uploader = v3d->uploader;
   v3d->base.const_uploader = v3d->uploader;
   v3d->state_uploader = u_upload_create(&v3d->base,
                        v3d->blitter = util_blitter_create(pctx);
   if (!v3d->blitter)
                           v3d->sample_mask = (1 << V3D_MAX_SAMPLES) - 1;
               fail:
         pctx->destroy(pctx);
   }
