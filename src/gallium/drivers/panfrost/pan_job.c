   /*
   * Copyright (C) 2019-2020 Collabora, Ltd.
   * Copyright (C) 2019 Alyssa Rosenzweig
   * Copyright (C) 2014-2017 Broadcom
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
   */
      #include <assert.h>
      #include "drm-uapi/panfrost_drm.h"
      #include "util/format/u_format.h"
   #include "util/hash_table.h"
   #include "util/ralloc.h"
   #include "util/rounding.h"
   #include "util/u_framebuffer.h"
   #include "util/u_pack_color.h"
   #include "decode.h"
   #include "pan_bo.h"
   #include "pan_context.h"
   #include "pan_util.h"
      #define foreach_batch(ctx, idx)                                                \
            static unsigned
   panfrost_batch_idx(struct panfrost_batch *batch)
   {
         }
      static bool
   panfrost_any_batch_other_than(struct panfrost_context *ctx, unsigned index)
   {
      unsigned i;
   foreach_batch(ctx, i) {
      if (i != index)
                  }
      /* Adds the BO backing surface to a batch if the surface is non-null */
      static void
   panfrost_batch_add_surface(struct panfrost_batch *batch,
         {
      if (surf) {
      struct panfrost_resource *rsrc = pan_resource(surf->texture);
   pan_legalize_afbc_format(batch->ctx, rsrc, surf->format, true, false);
         }
      static void
   panfrost_batch_init(struct panfrost_context *ctx,
               {
      struct pipe_screen *pscreen = ctx->base.screen;
   struct panfrost_screen *screen = pan_screen(pscreen);
                                       batch->minx = batch->miny = ~0;
                     /* Preallocate the main pool, since every batch has at least one job
   * structure so it will be used */
   panfrost_pool_init(&batch->pool, NULL, dev, 0, 65536, "Batch pool", true,
            /* Don't preallocate the invisible pool, since not every batch will use
   * the pre-allocation, particularly if the varyings are larger than the
   * preallocation and a reallocation is needed after anyway. */
   panfrost_pool_init(&batch->invisible_pool, NULL, dev, PAN_BO_INVISIBLE,
            for (unsigned i = 0; i < batch->key.nr_cbufs; ++i)
                        }
      static void
   panfrost_batch_cleanup(struct panfrost_context *ctx,
         {
                        if (ctx->batch == batch)
                     pan_bo_access *flags = util_dynarray_begin(&batch->bos);
            for (int i = 0; i < end_bo; ++i) {
      if (!flags[i])
            struct panfrost_bo *bo = pan_lookup_bo(dev, i);
               /* There is no more writer for anything we wrote */
   hash_table_foreach(ctx->writers, ent) {
      if (ent->data == batch)
               panfrost_pool_cleanup(&batch->pool);
                              memset(batch, 0, sizeof(*batch));
      }
      static void panfrost_batch_submit(struct panfrost_context *ctx,
            static struct panfrost_batch *
   panfrost_get_batch(struct panfrost_context *ctx,
         {
               for (unsigned i = 0; i < PAN_MAX_BATCHES; i++) {
      if (ctx->batches.slots[i].seqnum &&
      util_framebuffer_state_equal(&ctx->batches.slots[i].key, key)) {
   /* We found a match, increase the seqnum for the LRU
   * eviction logic.
   */
   ctx->batches.slots[i].seqnum = ++ctx->batches.seqnum;
               if (!batch || batch->seqnum > ctx->batches.slots[i].seqnum)
                        /* The selected slot is used, we need to flush the batch */
   if (batch->seqnum) {
      perf_debug_ctx(ctx, "Flushing batch due to seqnum overflow");
                        unsigned batch_idx = panfrost_batch_idx(batch);
               }
      /* Get the job corresponding to the FBO we're currently rendering into */
      struct panfrost_batch *
   panfrost_get_batch_for_fbo(struct panfrost_context *ctx)
   {
               if (ctx->batch) {
      assert(util_framebuffer_state_equal(&ctx->batch->key,
                     /* If not, look up the job */
   struct panfrost_batch *batch =
            /* Set this job as the current FBO job. Will be reset when updating the
   * FB state and when submitting or releasing a job.
   */
   ctx->batch = batch;
   panfrost_dirty_state_all(ctx);
      }
      struct panfrost_batch *
   panfrost_get_fresh_batch_for_fbo(struct panfrost_context *ctx,
         {
               batch = panfrost_get_batch(ctx, &ctx->pipe_framebuffer);
            /* We only need to submit and get a fresh batch if there is no
            if (batch->scoreboard.first_job) {
      perf_debug_ctx(ctx, "Flushing the current FBO due to: %s", reason);
   panfrost_batch_submit(ctx, batch);
               ctx->batch = batch;
      }
      static bool panfrost_batch_uses_resource(struct panfrost_batch *batch,
            static void
   panfrost_batch_update_access(struct panfrost_batch *batch,
         {
      struct panfrost_context *ctx = batch->ctx;
            if (writes) {
                  /* The rest of this routine is just about flushing other batches. If there
   * aren't any, we can skip a lot of work.
   */
   if (!panfrost_any_batch_other_than(ctx, batch_idx))
            struct hash_entry *entry = _mesa_hash_table_search(ctx->writers, rsrc);
            /* Both reads and writes flush the existing writer */
   if (writer != NULL && writer != batch)
            /* Writes (only) flush readers too */
   if (writes) {
      unsigned i;
   foreach_batch(ctx, i) {
               /* Skip the entry if this our batch. */
                  /* Submit if it's a user */
   if (panfrost_batch_uses_resource(batch, rsrc))
            }
      static pan_bo_access *
   panfrost_batch_get_bo_access(struct panfrost_batch *batch, unsigned handle)
   {
               if (handle >= size) {
               memset(util_dynarray_grow(&batch->bos, pan_bo_access, grow), 0,
                  }
      static bool
   panfrost_batch_uses_resource(struct panfrost_batch *batch,
         {
      /* A resource is used iff its current BO is used */
   uint32_t handle = rsrc->image.data.bo->gem_handle;
            /* If out of bounds, certainly not used */
   if (handle >= size)
            /* Otherwise check if nonzero access */
      }
      static void
   panfrost_batch_add_bo_old(struct panfrost_batch *batch, struct panfrost_bo *bo,
         {
      if (!bo)
            pan_bo_access *entry = panfrost_batch_get_bo_access(batch, bo->gem_handle);
            if (!old_flags) {
      batch->num_bos++;
               if (old_flags == flags)
            flags |= old_flags;
      }
      static uint32_t
   panfrost_access_for_stage(enum pipe_shader_type stage)
   {
      return (stage == PIPE_SHADER_FRAGMENT) ? PAN_BO_ACCESS_FRAGMENT
      }
      void
   panfrost_batch_add_bo(struct panfrost_batch *batch, struct panfrost_bo *bo,
         {
      panfrost_batch_add_bo_old(
      }
      void
   panfrost_batch_write_bo(struct panfrost_batch *batch, struct panfrost_bo *bo,
         {
      panfrost_batch_add_bo_old(
      }
      void
   panfrost_batch_read_rsrc(struct panfrost_batch *batch,
               {
                        if (rsrc->separate_stencil)
      panfrost_batch_add_bo_old(batch, rsrc->separate_stencil->image.data.bo,
            }
      void
   panfrost_batch_write_rsrc(struct panfrost_batch *batch,
               {
                        if (rsrc->separate_stencil)
      panfrost_batch_add_bo_old(batch, rsrc->separate_stencil->image.data.bo,
            }
      struct panfrost_bo *
   panfrost_batch_create_bo(struct panfrost_batch *batch, size_t size,
               {
               bo = panfrost_bo_create(pan_device(batch->ctx->base.screen), size,
                  /* panfrost_batch_add_bo() has retained a reference and
   * panfrost_bo_create() initialize the refcnt to 1, so let's
   * unreference the BO here so it gets released when the batch is
   * destroyed (unless it's retained by someone else in the meantime).
   */
   panfrost_bo_unreference(bo);
      }
      struct panfrost_bo *
   panfrost_batch_get_scratchpad(struct panfrost_batch *batch,
               {
      unsigned size = panfrost_get_total_stack_size(
            if (batch->scratchpad) {
         } else {
      batch->scratchpad =
                                 }
      struct panfrost_bo *
   panfrost_batch_get_shared_memory(struct panfrost_batch *batch, unsigned size,
         {
      if (batch->shared_memory) {
         } else {
      batch->shared_memory = panfrost_batch_create_bo(
      batch, size, PAN_BO_INVISIBLE, PIPE_SHADER_VERTEX,
               }
      static void
   panfrost_batch_to_fb_info(const struct panfrost_batch *batch,
                     {
      memset(fb, 0, sizeof(*fb));
   memset(rts, 0, sizeof(*rts) * 8);
   memset(zs, 0, sizeof(*zs));
            fb->width = batch->key.width;
   fb->height = batch->key.height;
   fb->extent.minx = batch->minx;
   fb->extent.miny = batch->miny;
   fb->extent.maxx = batch->maxx - 1;
   fb->extent.maxy = batch->maxy - 1;
   fb->nr_samples = util_framebuffer_get_num_samples(&batch->key);
   fb->rt_count = batch->key.nr_cbufs;
   fb->sprite_coord_origin = pan_tristate_get(batch->sprite_coord_origin);
            static const unsigned char id_swz[] = {
      PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_Y,
   PIPE_SWIZZLE_Z,
               for (unsigned i = 0; i < fb->rt_count; i++) {
               if (!surf)
            struct panfrost_resource *prsrc = pan_resource(surf->texture);
            if (batch->clear & mask) {
      fb->rts[i].clear = true;
   memcpy(fb->rts[i].clear_value, batch->clear_color[i],
                        /* Clamp the rendering area to the damage extent. The
   * KHR_partial_update spec states that trying to render outside of
   * the damage region is "undefined behavior", so we should be safe.
   */
   if (!fb->rts[i].discard) {
      fb->extent.minx = MAX2(fb->extent.minx, prsrc->damage.extent.minx);
   fb->extent.miny = MAX2(fb->extent.miny, prsrc->damage.extent.miny);
   fb->extent.maxx = MIN2(fb->extent.maxx, prsrc->damage.extent.maxx - 1);
   fb->extent.maxy = MIN2(fb->extent.maxy, prsrc->damage.extent.maxy - 1);
   assert(fb->extent.minx <= fb->extent.maxx);
               rts[i].format = surf->format;
   rts[i].dim = MALI_TEXTURE_DIMENSION_2D;
   rts[i].last_level = rts[i].first_level = surf->u.tex.level;
   rts[i].first_layer = surf->u.tex.first_layer;
   rts[i].last_layer = surf->u.tex.last_layer;
   panfrost_set_image_view_planes(&rts[i], surf->texture);
   rts[i].nr_samples =
         memcpy(rts[i].swizzle, id_swz, sizeof(rts[i].swizzle));
   fb->rts[i].crc_valid = &prsrc->valid.crc;
            /* Preload if the RT is read or updated */
   if (!(batch->clear & mask) &&
      ((batch->read & mask) ||
   ((batch->draws & mask) &&
                  const struct pan_image_view *s_view = NULL, *z_view = NULL;
            if (batch->key.zsbuf) {
      struct pipe_surface *surf = batch->key.zsbuf;
            zs->format = surf->format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT
               zs->dim = MALI_TEXTURE_DIMENSION_2D;
   zs->last_level = zs->first_level = surf->u.tex.level;
   zs->first_layer = surf->u.tex.first_layer;
   zs->last_layer = surf->u.tex.last_layer;
   zs->planes[0] = &z_rsrc->image;
   zs->nr_samples = surf->nr_samples ?: MAX2(surf->texture->nr_samples, 1);
   memcpy(zs->swizzle, id_swz, sizeof(zs->swizzle));
   fb->zs.view.zs = zs;
   z_view = zs;
   if (util_format_is_depth_and_stencil(zs->format)) {
      s_view = zs;
               if (z_rsrc->separate_stencil) {
      s_rsrc = z_rsrc->separate_stencil;
   s->format = PIPE_FORMAT_S8_UINT;
   s->dim = MALI_TEXTURE_DIMENSION_2D;
   s->last_level = s->first_level = surf->u.tex.level;
   s->first_layer = surf->u.tex.first_layer;
   s->last_layer = surf->u.tex.last_layer;
   s->planes[0] = &s_rsrc->image;
   s->nr_samples = surf->nr_samples ?: MAX2(surf->texture->nr_samples, 1);
   memcpy(s->swizzle, id_swz, sizeof(s->swizzle));
   fb->zs.view.s = s;
                  if (batch->clear & PIPE_CLEAR_DEPTH) {
      fb->zs.clear.z = true;
               if (batch->clear & PIPE_CLEAR_STENCIL) {
      fb->zs.clear.s = true;
               fb->zs.discard.z = !reserve && !(batch->resolve & PIPE_CLEAR_DEPTH);
            if (!fb->zs.clear.z && z_rsrc &&
      ((batch->read & PIPE_CLEAR_DEPTH) ||
   ((batch->draws & PIPE_CLEAR_DEPTH) &&
               if (!fb->zs.clear.s && s_rsrc &&
      ((batch->read & PIPE_CLEAR_STENCIL) ||
   ((batch->draws & PIPE_CLEAR_STENCIL) &&
               /* Preserve both component if we have a combined ZS view and
   * one component needs to be preserved.
   */
   if (z_view && z_view == s_view && fb->zs.discard.z != fb->zs.discard.s) {
               fb->zs.discard.z = false;
   fb->zs.discard.s = false;
   fb->zs.preload.z = !fb->zs.clear.z && valid;
         }
      static int
   panfrost_batch_submit_ioctl(struct panfrost_batch *batch,
               {
      struct panfrost_context *ctx = batch->ctx;
   struct pipe_context *gallium = (struct pipe_context *)ctx;
   struct panfrost_device *dev = pan_device(gallium->screen);
   struct drm_panfrost_submit submit = {
         };
   uint32_t in_syncs[2];
   uint32_t *bo_handles;
            /* If we trace, we always need a syncobj, so make one of our own if we
   * weren't given one to use. Remember that we did so, so we can free it
   * after we're done but preventing double-frees if we were given a
            if (!out_sync && dev->debug & (PAN_DBG_TRACE | PAN_DBG_SYNC))
            submit.out_sync = out_sync;
   submit.jc = first_job_desc;
            if (in_sync)
            if (ctx->in_sync_fd >= 0) {
      ret =
                  in_syncs[submit.in_sync_count++] = ctx->in_sync_obj;
   close(ctx->in_sync_fd);
               if (submit.in_sync_count)
            bo_handles = calloc(panfrost_pool_num_bos(&batch->pool) +
                              pan_bo_access *flags = util_dynarray_begin(&batch->bos);
            for (int i = 0; i < end_bo; ++i) {
      if (!flags[i])
            assert(submit.bo_handle_count < batch->num_bos);
            /* Update the BO access flags so that panfrost_bo_wait() knows
   * about all pending accesses.
   * We only keep the READ/WRITE info since this is all the BO
   * wait logic cares about.
   * We also preserve existing flags as this batch might not
   * be the first one to access the BO.
   */
                        panfrost_pool_get_bo_handles(&batch->pool,
         submit.bo_handle_count += panfrost_pool_num_bos(&batch->pool);
   panfrost_pool_get_bo_handles(&batch->invisible_pool,
                  /* Add the tiler heap to the list of accessed BOs if the batch has at
   * least one tiler job. Tiler heap is written by tiler jobs and read
   * by fragment jobs (the polygon list is coming from this heap).
   */
   if (batch->scoreboard.first_tiler)
            /* Always used on Bifrost, occassionally used on Midgard */
            submit.bo_handles = (u64)(uintptr_t)bo_handles;
   if (ctx->is_noop)
         else
                  if (ret)
            /* Trace the job if we're doing that */
   if (dev->debug & (PAN_DBG_TRACE | PAN_DBG_SYNC)) {
      /* Wait so we can get errors reported back */
            if (dev->debug & PAN_DBG_TRACE)
            if (dev->debug & PAN_DBG_DUMP)
            /* Jobs won't be complete if blackhole rendering, that's ok */
   if (!ctx->is_noop && dev->debug & PAN_DBG_SYNC)
                  }
      static bool
   panfrost_has_fragment_job(struct panfrost_batch *batch)
   {
         }
      /* Submit both vertex/tiler and fragment jobs for a batch, possibly with an
   * outsync corresponding to the later of the two (since there will be an
   * implicit dep between them) */
      static int
   panfrost_batch_submit_jobs(struct panfrost_batch *batch,
               {
      struct pipe_screen *pscreen = batch->ctx->base.screen;
   struct panfrost_screen *screen = pan_screen(pscreen);
   struct panfrost_device *dev = pan_device(pscreen);
   bool has_draws = batch->scoreboard.first_job;
   bool has_tiler = batch->scoreboard.first_tiler;
   bool has_frag = panfrost_has_fragment_job(batch);
            /* Take the submit lock to make sure no tiler jobs from other context
   * are inserted between our tiler and fragment jobs, failing to do that
   * might result in tiler heap corruption.
   */
   if (has_tiler)
            if (has_draws) {
      ret = panfrost_batch_submit_ioctl(batch, batch->scoreboard.first_job, 0,
            if (ret)
               if (has_frag) {
      mali_ptr fragjob = screen->vtbl.emit_fragment_job(batch, fb);
   ret = panfrost_batch_submit_ioctl(batch, fragjob, PANFROST_JD_REQ_FS, 0,
         if (ret)
            done:
      if (has_tiler)
               }
      static void
   panfrost_emit_tile_map(struct panfrost_batch *batch, struct pan_fb_info *fb)
   {
      if (batch->key.nr_cbufs < 1 || !batch->key.cbufs[0])
            struct pipe_surface *surf = batch->key.cbufs[0];
            if (pres && pres->damage.tile_map.enable) {
      fb->tile_map.base =
      pan_pool_upload_aligned(&batch->pool.base, pres->damage.tile_map.data,
            }
      static void
   panfrost_batch_submit(struct panfrost_context *ctx,
         {
      struct pipe_screen *pscreen = ctx->base.screen;
   struct panfrost_screen *screen = pan_screen(pscreen);
            /* Nothing to do! */
   if (!batch->scoreboard.first_job && !batch->clear)
            if (batch->key.zsbuf && panfrost_has_fragment_job(batch)) {
      struct pipe_surface *surf = batch->key.zsbuf;
            /* Shared depth/stencil resources are not supported, and would
   * break this optimisation. */
            if (batch->clear & PIPE_CLEAR_STENCIL) {
      z_rsrc->stencil_value = batch->clear_stencil;
      } else if (z_rsrc->constant_stencil) {
      batch->clear_stencil = z_rsrc->stencil_value;
               if (batch->draws & PIPE_CLEAR_STENCIL)
               struct pan_fb_info fb;
                     screen->vtbl.preload(batch, &fb);
            /* Now that all draws are in, we can finally prepare the
            screen->vtbl.emit_tls(batch);
            if (batch->scoreboard.first_tiler || batch->clear)
                     if (ret)
            /* We must reset the damage info of our render targets here even
   * though a damage reset normally happens when the DRI layer swaps
   * buffers. That's because there can be implicit flushes the GL
   * app is not aware of, and those might impact the damage region: if
   * part of the damaged portion is drawn during those implicit flushes,
   * you have to reload those areas before next draws are pushed, and
   * since the driver can't easily know what's been modified by the draws
   * it flushed, the easiest solution is to reload everything.
   */
   for (unsigned i = 0; i < batch->key.nr_cbufs; i++) {
      if (!batch->key.cbufs[i])
            panfrost_resource_set_damage_region(
            out:
         }
      /* Submit all batches */
      void
   panfrost_flush_all_batches(struct panfrost_context *ctx, const char *reason)
   {
      if (reason)
            struct panfrost_batch *batch = panfrost_get_batch_for_fbo(ctx);
            for (unsigned i = 0; i < PAN_MAX_BATCHES; i++) {
      if (ctx->batches.slots[i].seqnum)
         }
      void
   panfrost_flush_writer(struct panfrost_context *ctx,
         {
               if (entry) {
      perf_debug_ctx(ctx, "Flushing writer due to: %s", reason);
         }
      void
   panfrost_flush_batches_accessing_rsrc(struct panfrost_context *ctx,
               {
      unsigned i;
   foreach_batch(ctx, i) {
               if (!panfrost_batch_uses_resource(batch, rsrc))
            perf_debug_ctx(ctx, "Flushing user due to: %s", reason);
         }
      bool
   panfrost_any_batch_reads_rsrc(struct panfrost_context *ctx,
         {
      unsigned i;
   foreach_batch(ctx, i) {
               if (panfrost_batch_uses_resource(batch, rsrc))
                  }
      bool
   panfrost_any_batch_writes_rsrc(struct panfrost_context *ctx,
         {
         }
      void
   panfrost_batch_adjust_stack_size(struct panfrost_batch *batch)
   {
               for (unsigned i = 0; i < PIPE_SHADER_TYPES; ++i) {
               if (!ss)
                  }
      void
   panfrost_batch_clear(struct panfrost_batch *batch, unsigned buffers,
               {
               if (buffers & PIPE_CLEAR_COLOR) {
      for (unsigned i = 0; i < ctx->pipe_framebuffer.nr_cbufs; ++i) {
                     enum pipe_format format = ctx->pipe_framebuffer.cbufs[i]->format;
                  if (buffers & PIPE_CLEAR_DEPTH) {
                  if (buffers & PIPE_CLEAR_STENCIL) {
                  batch->clear |= buffers;
            /* Clearing affects the entire framebuffer (by definition -- this is
   * the Gallium clear callback, which clears the whole framebuffer. If
   * the scissor test were enabled from the GL side, the gallium frontend
            panfrost_batch_union_scissor(batch, 0, 0, ctx->pipe_framebuffer.width,
      }
      /* Given a new bounding rectangle (scissor), let the job cover the union of the
   * new and old bounding rectangles */
      void
   panfrost_batch_union_scissor(struct panfrost_batch *batch, unsigned minx,
         {
      batch->minx = MIN2(batch->minx, minx);
   batch->miny = MIN2(batch->miny, miny);
   batch->maxx = MAX2(batch->maxx, maxx);
      }
      /**
   * Checks if rasterization should be skipped. If not, a TILER job must be
   * created for each draw, or the IDVS flow must be used.
   *
   * As a special case, if there is no vertex shader, no primitives are generated,
   * meaning the whole pipeline (including rasterization) should be skipped.
   */
   bool
   panfrost_batch_skip_rasterization(struct panfrost_batch *batch)
   {
      struct panfrost_context *ctx = batch->ctx;
            return (rast->rasterizer_discard || batch->scissor_culls_everything ||
      }
