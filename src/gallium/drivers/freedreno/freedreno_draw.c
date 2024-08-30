   /*
   * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
      #include "pipe/p_state.h"
   #include "util/format/u_format.h"
   #include "util/u_draw.h"
   #include "util/u_helpers.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/u_string.h"
      #include "freedreno_blitter.h"
   #include "freedreno_context.h"
   #include "freedreno_draw.h"
   #include "freedreno_fence.h"
   #include "freedreno_query_acc.h"
   #include "freedreno_query_hw.h"
   #include "freedreno_resource.h"
   #include "freedreno_state.h"
   #include "freedreno_util.h"
      static bool
   batch_references_resource(struct fd_batch *batch, struct pipe_resource *prsc)
         {
         }
      static void
   resource_read(struct fd_batch *batch, struct pipe_resource *prsc) assert_dt
   {
      if (!prsc)
            }
      static void
   resource_written(struct fd_batch *batch, struct pipe_resource *prsc) assert_dt
   {
      if (!prsc)
            }
      static void
   batch_draw_tracking_for_dirty_bits(struct fd_batch *batch) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   enum fd_dirty_3d_state dirty = ctx->dirty_resource;
            if (dirty & (FD_DIRTY_FRAMEBUFFER | FD_DIRTY_ZSA)) {
      if (fd_depth_enabled(ctx)) {
      if (fd_resource(pfb->zsbuf->texture)->valid) {
      restore_buffers |= FD_BUFFER_DEPTH;
   /* storing packed d/s depth also stores stencil, so we need
   * the stencil restored too to avoid invalidating it.
   */
   if (pfb->zsbuf->texture->format == PIPE_FORMAT_Z24_UNORM_S8_UINT)
      } else {
         }
   batch->gmem_reason |= FD_GMEM_DEPTH_ENABLED;
   if (fd_depth_write_enabled(ctx)) {
      buffers |= FD_BUFFER_DEPTH;
      } else {
                     if (fd_stencil_enabled(ctx)) {
      if (fd_resource(pfb->zsbuf->texture)->valid) {
      restore_buffers |= FD_BUFFER_STENCIL;
   /* storing packed d/s stencil also stores depth, so we need
   * the depth restored too to avoid invalidating it.
   */
   if (pfb->zsbuf->texture->format == PIPE_FORMAT_Z24_UNORM_S8_UINT)
      } else {
         }
   batch->gmem_reason |= FD_GMEM_STENCIL_ENABLED;
   buffers |= FD_BUFFER_STENCIL;
                  if (dirty & FD_DIRTY_FRAMEBUFFER) {
      for (unsigned i = 0; i < pfb->nr_cbufs; i++) {
                                       if (fd_resource(surf)->valid) {
         } else {
                                          if (dirty & (FD_DIRTY_CONST | FD_DIRTY_TEX | FD_DIRTY_SSBO | FD_DIRTY_IMAGE)) {
      u_foreach_bit (s, ctx->bound_shader_stages) {
               /* Mark constbuf as being read: */
   if (dirty_shader & FD_DIRTY_SHADER_CONST) {
      u_foreach_bit (i, ctx->constbuf[s].enabled_mask)
               /* Mark textures as being read */
   if (dirty_shader & FD_DIRTY_SHADER_TEX) {
      u_foreach_bit (i, ctx->tex[s].valid_textures)
               /* Mark SSBOs as being read or written: */
                                    u_foreach_bit (i, so->enabled_mask & ~so->writable_mask)
               /* Mark Images as being read or written: */
   if (dirty_shader & FD_DIRTY_SHADER_IMAGE) {
      u_foreach_bit (i, ctx->shaderimg[s].enabled_mask) {
      struct pipe_image_view *img = &ctx->shaderimg[s].si[i];
   if (img->access & PIPE_IMAGE_ACCESS_WRITE)
         else
                        /* Mark VBOs as being read */
   if (dirty & FD_DIRTY_VTXBUF) {
      u_foreach_bit (i, ctx->vtx.vertexbuf.enabled_mask) {
      assert(!ctx->vtx.vertexbuf.vb[i].is_user_buffer);
                  /* Mark streamout buffers as being written.. */
   if (dirty & FD_DIRTY_STREAMOUT) {
      for (unsigned i = 0; i < ctx->streamout.num_targets; i++) {
                     if (target) {
      resource_written(batch, target->base.buffer);
                     if (dirty & FD_DIRTY_QUERY) {
      list_for_each_entry (struct fd_acc_query, aq, &ctx->acc_active_queries, node) {
                     /* any buffers that haven't been cleared yet, we need to restore: */
   batch->restore |= restore_buffers & (FD_BUFFER_ALL & ~batch->invalidated);
   /* and any buffers used, need to be resolved: */
      }
      static bool
   needs_draw_tracking(struct fd_batch *batch, const struct pipe_draw_info *info,
               {
               if (ctx->dirty_resource)
            if (info->index_size && !batch_references_resource(batch, info->index.resource))
            if (indirect) {
      if (indirect->buffer && !batch_references_resource(batch, indirect->buffer))
         if (indirect->indirect_draw_count &&
      !batch_references_resource(batch, indirect->indirect_draw_count))
      if (indirect->count_from_stream_output)
                  }
      static void
   batch_draw_tracking(struct fd_batch *batch, const struct pipe_draw_info *info,
         {
               if (!needs_draw_tracking(batch, info, indirect))
            /*
   * Figure out the buffers/features we need:
                     if (ctx->dirty_resource)
            /* Mark index buffer as being read */
   if (info->index_size)
            /* Mark indirect draw buffer as being read */
   if (indirect) {
      resource_read(batch, indirect->buffer);
   resource_read(batch, indirect->indirect_draw_count);
   if (indirect->count_from_stream_output)
      resource_read(
                                 out:
         }
      static void
   update_draw_stats(struct fd_context *ctx, const struct pipe_draw_info *info,
               {
               if (ctx->screen->gen < 6) {
      /* Counting prims in sw doesn't work for GS and tesselation. For older
   * gens we don't have those stages and don't have the hw counters enabled,
   * so keep the count accurate for non-patch geometry.
   */
   unsigned prims = 0;
   if ((info->mode != MESA_PRIM_PATCHES) && (info->mode != MESA_PRIM_COUNT)) {
      for (unsigned i = 0; i < num_draws; i++) {
                              if (ctx->streamout.num_targets > 0) {
      /* Clip the prims we're writing to the size of the SO buffers. */
   enum mesa_prim tf_prim = u_decomposed_prim(info->mode);
   unsigned verts_written = u_vertices_for_prims(tf_prim, prims);
   unsigned remaining_vert_space =
         if (verts_written > remaining_vert_space) {
      verts_written = remaining_vert_space;
                     ctx->stats.prims_emitted +=
            }
      static void
   fd_draw_vbo(struct pipe_context *pctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   {
               /* for debugging problems with indirect draw, it is convenient
   * to be able to emulate it, to determine if game is feeding us
   * bogus data:
   */
   if (indirect && indirect->buffer && FD_DBG(NOINDR)) {
      /* num_draws is only applicable for direct draws: */
   assert(num_draws == 1);
   util_draw_indirect(pctx, info, indirect);
               /* TODO: push down the region versions into the tiles */
   if (!fd_render_condition_check(pctx))
            /* Upload a user index buffer. */
   struct pipe_resource *indexbuf = NULL;
   unsigned index_offset = 0;
   struct pipe_draw_info new_info;
   if (info->index_size) {
      if (info->has_user_indices) {
      if (num_draws > 1) {
      util_draw_multi(pctx, info, drawid_offset, indirect, draws, num_draws);
      }
   if (!util_upload_index_buffer(pctx, info, &draws[0], &indexbuf,
               new_info = *info;
   new_info.index.resource = indexbuf;
   new_info.has_user_indices = false;
      } else {
                     if ((ctx->streamout.num_targets > 0) && (num_draws > 1)) {
      util_draw_multi(pctx, info, drawid_offset, indirect, draws, num_draws);
                                 while (unlikely(batch->flushed)) {
      /* The current batch was flushed in batch_draw_tracking()
   * so start anew.  We know this won't happen a second time
   * since we are dealing with a fresh batch:
   */
   fd_batch_reference(&batch, NULL);
   batch = fd_context_batch(ctx);
   batch_draw_tracking(batch, info, indirect);
               batch->num_draws++;
                     /* Marking the batch as needing flush must come after the batch
   * dependency tracking (resource_read()/resource_write()), as that
   * can trigger a flush
   */
            struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   DBG("%p: %ux%u num_draws=%u (%s/%s)", batch, pfb->width, pfb->height,
      batch->num_draws,
   util_format_short_name(pipe_surface_format(pfb->cbufs[0])),
                           if (unlikely(ctx->stats_users > 0))
            for (unsigned i = 0; i < ctx->streamout.num_targets; i++) {
      assert(num_draws == 1);
                        fd_batch_check_size(batch);
            if (info == &new_info)
      }
      static void
   fd_draw_vbo_dbg(struct pipe_context *pctx, const struct pipe_draw_info *info,
                  unsigned drawid_offset,
      {
               if (FD_DBG(DDRAW))
            if (FD_DBG(FLUSH))
      }
      static void
   batch_clear_tracking(struct fd_batch *batch, unsigned buffers) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
            /* pctx->clear() is only for full-surface clears, so scissor is
   * equivalent to having GL_SCISSOR_TEST disabled:
   */
   batch->max_scissor.minx = 0;
   batch->max_scissor.miny = 0;
   batch->max_scissor.maxx = pfb->width - 1;
            /* for bookkeeping about which buffers have been cleared (and thus
   * can fully or partially skip mem2gmem) we need to ignore buffers
   * that have already had a draw, in case apps do silly things like
   * clear after draw (ie. if you only clear the color buffer, but
   * something like alpha-test causes side effects from the draw in
   * the depth buffer, etc)
   */
   cleared_buffers = buffers & (FD_BUFFER_ALL & ~batch->restore);
   batch->cleared |= buffers;
                              if (buffers & PIPE_CLEAR_COLOR)
      for (unsigned i = 0; i < pfb->nr_cbufs; i++)
               if (buffers & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)) {
      resource_written(batch, pfb->zsbuf->texture);
                        list_for_each_entry (struct fd_acc_query, aq, &ctx->acc_active_queries, node)
               }
      static void
   fd_clear(struct pipe_context *pctx, unsigned buffers,
            const struct pipe_scissor_state *scissor_state,
      {
               /* TODO: push down the region versions into the tiles */
   if (!fd_render_condition_check(pctx))
                              while (unlikely(batch->flushed)) {
      /* The current batch was flushed in batch_clear_tracking()
   * so start anew.  We know this won't happen a second time
   * since we are dealing with a fresh batch:
   */
   fd_batch_reference(&batch, NULL);
   batch = fd_context_batch(ctx);
   batch_clear_tracking(batch, buffers);
               /* Marking the batch as needing flush must come after the batch
   * dependency tracking (resource_read()/resource_write()), as that
   * can trigger a flush
   */
            struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   DBG("%p: %x %ux%u depth=%f, stencil=%u (%s/%s)", batch, buffers, pfb->width,
      pfb->height, depth, stencil,
   util_format_short_name(pipe_surface_format(pfb->cbufs[0])),
         /* if per-gen backend doesn't implement ctx->clear() generic
   * blitter clear:
   */
            if (ctx->clear) {
               if (ctx->clear(ctx, buffers, color, depth, stencil)) {
                                             if (fallback) {
                              }
      static void
   fd_clear_render_target(struct pipe_context *pctx, struct pipe_surface *ps,
                     {
      if (render_condition_enabled && !fd_render_condition_check(pctx))
            fd_blitter_clear_render_target(pctx, ps, color, x, y, w, h,
      }
      static void
   fd_clear_depth_stencil(struct pipe_context *pctx, struct pipe_surface *ps,
                     {
      if (render_condition_enabled && !fd_render_condition_check(pctx))
            fd_blitter_clear_depth_stencil(pctx, ps, buffers,
            }
      static void
   fd_launch_grid(struct pipe_context *pctx,
         {
      struct fd_context *ctx = fd_context(pctx);
   const struct fd_shaderbuf_stateobj *so =
                  if (!fd_render_condition_check(pctx))
            batch = fd_context_batch_nondraw(ctx);
   fd_batch_reference(&save_batch, ctx->batch);
                     /* Mark SSBOs */
   u_foreach_bit (i, so->enabled_mask & so->writable_mask)
            u_foreach_bit (i, so->enabled_mask & ~so->writable_mask)
            u_foreach_bit (i, ctx->shaderimg[PIPE_SHADER_COMPUTE].enabled_mask) {
      struct pipe_image_view *img = &ctx->shaderimg[PIPE_SHADER_COMPUTE].si[i];
   if (img->access & PIPE_IMAGE_ACCESS_WRITE)
         else
               /* UBO's are read */
   u_foreach_bit (i, ctx->constbuf[PIPE_SHADER_COMPUTE].enabled_mask)
            /* Mark textures as being read */
   u_foreach_bit (i, ctx->tex[PIPE_SHADER_COMPUTE].valid_textures)
            /* For global buffers, we don't really know if read or written, so assume
   * the worst:
   */
   u_foreach_bit (i, ctx->global_bindings.enabled_mask)
            if (info->indirect)
            list_for_each_entry (struct fd_acc_query, aq, &ctx->acc_active_queries, node) {
                  /* If the saved batch has been flushed during the resource tracking,
   * don't re-install it:
   */
   if (save_batch && save_batch->flushed)
                              DBG("%p: work_dim=%u, block=%ux%ux%u, grid=%ux%ux%u",
      batch, info->work_dim,
   info->block[0], info->block[1], info->block[2],
         fd_batch_needs_flush(batch);
            fd_batch_reference(&ctx->batch, save_batch);
   fd_batch_reference(&save_batch, NULL);
      }
      void
   fd_draw_init(struct pipe_context *pctx)
   {
      if (FD_DBG(DDRAW) || FD_DBG(FLUSH)) {
         } else {
                  pctx->clear = fd_clear;
   pctx->clear_render_target = fd_clear_render_target;
            if (has_compute(fd_screen(pctx->screen))) {
            }
