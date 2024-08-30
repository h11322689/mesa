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
      #include "util/format/u_format.h"
   #include "util/format/u_format_rgtc.h"
   #include "util/format/u_format_zs.h"
   #include "util/set.h"
   #include "util/u_drm.h"
   #include "util/u_inlines.h"
   #include "util/u_sample_positions.h"
   #include "util/u_string.h"
   #include "util/u_surface.h"
   #include "util/u_transfer.h"
      #include "decode/util.h"
      #include "freedreno_batch_cache.h"
   #include "freedreno_blitter.h"
   #include "freedreno_context.h"
   #include "freedreno_fence.h"
   #include "freedreno_query_hw.h"
   #include "freedreno_resource.h"
   #include "freedreno_screen.h"
   #include "freedreno_surface.h"
   #include "freedreno_util.h"
      #include <errno.h>
   #include "drm-uapi/drm_fourcc.h"
      /* XXX this should go away, needed for 'struct winsys_handle' */
   #include "frontend/drm_driver.h"
      /**
   * Go through the entire state and see if the resource is bound
   * anywhere. If it is, mark the relevant state as dirty. This is
   * called on realloc_bo to ensure the necessary state is re-
   * emitted so the GPU looks at the new backing bo.
   */
   static void
   rebind_resource_in_ctx(struct fd_context *ctx,
         {
               if (ctx->rebind_resource)
            /* VBOs */
   if (rsc->dirty & FD_DIRTY_VTXBUF) {
      struct fd_vertexbuf_stateobj *vb = &ctx->vtx.vertexbuf;
   for (unsigned i = 0; i < vb->count && !(ctx->dirty & FD_DIRTY_VTXBUF);
      i++) {
   if (vb->vb[i].buffer.resource == prsc)
                  /* xfb/so buffers: */
   if (rsc->dirty & FD_DIRTY_STREAMOUT) {
               for (unsigned i = 0;
         i < so->num_targets && !(ctx->dirty & FD_DIRTY_STREAMOUT);
      if (so->targets[i]->buffer == prsc)
                  const enum fd_dirty_3d_state per_stage_dirty =
            if (!(rsc->dirty & per_stage_dirty))
            /* per-shader-stage resources: */
   for (unsigned stage = 0; stage < PIPE_SHADER_TYPES; stage++) {
      /* Constbufs.. note that constbuf[0] is normal uniforms emitted in
   * cmdstream rather than by pointer..
   */
   if ((rsc->dirty & FD_DIRTY_CONST) &&
      !(ctx->dirty_shader[stage] & FD_DIRTY_CONST)) {
   struct fd_constbuf_stateobj *cb = &ctx->constbuf[stage];
   const unsigned num_ubos = util_last_bit(cb->enabled_mask);
   for (unsigned i = 1; i < num_ubos; i++) {
      if (cb->cb[i].buffer == prsc) {
      fd_dirty_shader_resource(ctx, prsc, stage,
                           /* Textures */
   if ((rsc->dirty & FD_DIRTY_TEX) &&
      !(ctx->dirty_shader[stage] & FD_DIRTY_TEX)) {
   struct fd_texture_stateobj *tex = &ctx->tex[stage];
   for (unsigned i = 0; i < tex->num_textures; i++) {
      if (tex->textures[i] && (tex->textures[i]->texture == prsc)) {
      fd_dirty_shader_resource(ctx, prsc, stage,
                           /* Images */
   if ((rsc->dirty & FD_DIRTY_IMAGE) &&
      !(ctx->dirty_shader[stage] & FD_DIRTY_IMAGE)) {
   struct fd_shaderimg_stateobj *si = &ctx->shaderimg[stage];
   const unsigned num_images = util_last_bit(si->enabled_mask);
   for (unsigned i = 0; i < num_images; i++) {
      if (si->si[i].resource == prsc) {
      bool write = si->si[i].access & PIPE_IMAGE_ACCESS_WRITE;
   fd_dirty_shader_resource(ctx, prsc, stage,
                           /* SSBOs */
   if ((rsc->dirty & FD_DIRTY_SSBO) &&
      !(ctx->dirty_shader[stage] & FD_DIRTY_SSBO)) {
   struct fd_shaderbuf_stateobj *sb = &ctx->shaderbuf[stage];
   const unsigned num_ssbos = util_last_bit(sb->enabled_mask);
   for (unsigned i = 0; i < num_ssbos; i++) {
      if (sb->sb[i].buffer == prsc) {
      bool write = sb->writable_mask & BIT(i);
   fd_dirty_shader_resource(ctx, prsc, stage,
                        }
      static void
   rebind_resource(struct fd_resource *rsc) assert_dt
   {
               fd_screen_lock(screen);
            if (rsc->dirty)
      list_for_each_entry (struct fd_context, ctx, &screen->context_list, node)
         fd_resource_unlock(rsc);
      }
      static inline void
   fd_resource_set_bo(struct fd_resource *rsc, struct fd_bo *bo)
   {
               rsc->bo = bo;
      }
      int
   __fd_resource_wait(struct fd_context *ctx, struct fd_resource *rsc, unsigned op,
         {
      if (op & FD_BO_PREP_NOSYNC)
                     perf_time_ctx (ctx, 10000, "%s: a busy \"%" PRSC_FMT "\" BO stalled", func,
                           }
      static void
   realloc_bo(struct fd_resource *rsc, uint32_t size)
   {
      struct pipe_resource *prsc = &rsc->b.b;
   struct fd_screen *screen = fd_screen(rsc->b.b.screen);
   uint32_t flags =
      COND(rsc->layout.tile_mode, FD_BO_NOMAP) |
   COND((prsc->usage & PIPE_USAGE_STAGING) &&
      (prsc->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT),
      COND(prsc->bind & PIPE_BIND_SHARED, FD_BO_SHARED) |
               /* if we start using things other than write-combine,
   * be sure to check for PIPE_RESOURCE_FLAG_MAP_COHERENT
            if (rsc->bo)
            struct fd_bo *bo =
      fd_bo_new(screen->dev, size, flags, "%ux%ux%u@%u:%x", prsc->width0,
               /* Zero out the UBWC area on allocation.  This fixes intermittent failures
   * with UBWC, which I suspect are due to the HW having a hard time
   * interpreting arbitrary values populating the flags buffer when the BO
   * was recycled through the bo cache (instead of fresh allocations from
   * the kernel, which are zeroed).  sleep(1) in this spot didn't work
   * around the issue, but any memset value seems to.
   */
   if (rsc->layout.ubwc) {
                  util_range_set_empty(&rsc->valid_buffer_range);
      }
      static void
   do_blit(struct fd_context *ctx, const struct pipe_blit_info *blit,
         {
               assert(!ctx->in_blit);
            /* TODO size threshold too?? */
   if (fallback || !fd_blit(pctx, blit)) {
      /* do blit on cpu: */
   util_resource_copy_region(pctx, blit->dst.resource, blit->dst.level,
                              }
      /**
   * Replace the storage of dst with src.  This is only used by TC in the
   * DISCARD_WHOLE_RESOURCE path, and src is a freshly allocated buffer.
   */
   void
   fd_replace_buffer_storage(struct pipe_context *pctx, struct pipe_resource *pdst,
               {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_resource *dst = fd_resource(pdst);
                     /* This should only be called with buffers.. which side-steps some tricker
   * cases, like a rsc that is in a batch-cache key...
   */
   assert(pdst->target == PIPE_BUFFER);
   assert(psrc->target == PIPE_BUFFER);
   assert(dst->track->bc_batch_mask == 0);
   assert(src->track->bc_batch_mask == 0);
   assert(src->track->batch_mask == 0);
   assert(src->track->write_batch == NULL);
            /* get rid of any references that batch-cache might have to us (which
   * should empty/destroy rsc->batches hashset)
   *
   * Note that we aren't actually destroying dst, but we are replacing
   * it's storage so we want to go thru the same motions of decoupling
   * it's batch connections.
   */
   fd_bc_invalidate_resource(dst, true);
                              fd_bo_del(dst->bo);
            fd_resource_tracking_reference(&dst->track, src->track);
                        }
      static unsigned
   translate_usage(unsigned usage)
   {
               if (usage & PIPE_MAP_READ)
            if (usage & PIPE_MAP_WRITE)
               }
      bool
   fd_resource_busy(struct pipe_screen *pscreen, struct pipe_resource *prsc,
         {
               if (pending(rsc, !!(usage & PIPE_MAP_WRITE)))
            if (resource_busy(rsc, translate_usage(usage)))
               }
      static void flush_resource(struct fd_context *ctx, struct fd_resource *rsc,
            /**
   * Helper to check if the format is something that we can blit/render
   * to.. if the format is not renderable, there is no point in trying
   * to do a staging blit (as it will still end up being a cpu copy)
   */
   static bool
   is_renderable(struct pipe_resource *prsc)
   {
      struct pipe_screen *pscreen = prsc->screen;
   return pscreen->is_format_supported(
            }
      /**
   * @rsc: the resource to shadow
   * @level: the level to discard (if box != NULL, otherwise ignored)
   * @box: the box to discard (or NULL if none)
   * @modifier: the modifier for the new buffer state
   */
   static bool
   fd_try_shadow_resource(struct fd_context *ctx, struct fd_resource *rsc,
               {
      struct pipe_context *pctx = &ctx->base;
   struct pipe_resource *prsc = &rsc->b.b;
   struct fd_screen *screen = fd_screen(pctx->screen);
   struct fd_batch *batch;
            if (prsc->next)
            /* Flush any pending batches writing the resource before we go mucking around
   * in its insides.  The blit would immediately cause the batch to be flushed,
   * anyway.
   */
            /* Because IB1 ("gmem") cmdstream is built only when we flush the
   * batch, we need to flush any batches that reference this rsc as
   * a render target.  Otherwise the framebuffer state emitted in
   * IB1 will reference the resources new state, and not the state
   * at the point in time that the earlier draws referenced it.
   *
   * Note that being in the gmem key doesn't necessarily mean the
   * batch was considered a writer!
   */
   foreach_batch (batch, &screen->batch_cache, rsc->track->bc_batch_mask) {
                  /* TODO: somehow munge dimensions and format to copy unsupported
   * render target format to something that is supported?
   */
   if (!is_renderable(prsc))
            /* do shadowing back-blits on the cpu for buffers -- requires about a page of
   * DMA to make GPU copies worth it according to robclark.  Note, if you
   * decide to do it on the GPU then you'll need to update valid_buffer_range
   * in the swap()s below.
   */
   if (prsc->target == PIPE_BUFFER)
            bool discard_whole_level = box && util_texrange_covers_whole_level(
                  /* TODO need to be more clever about current level */
   if ((prsc->target >= PIPE_TEXTURE_2D) && box && !discard_whole_level)
            struct pipe_resource *pshadow = pctx->screen->resource_create_with_modifiers(
            if (!pshadow)
            assert(!ctx->in_shadow);
            /* get rid of any references that batch-cache might have to us (which
   * should empty/destroy rsc->batches hashset)
   */
   fd_bc_invalidate_resource(rsc, false);
                     /* Swap the backing bo's, so shadow becomes the old buffer,
   * blit from shadow to new buffer.  From here on out, we
   * cannot fail.
   *
   * Note that we need to do it in this order, otherwise if
   * we go down cpu blit path, the recursive transfer_map()
   * sees the wrong status..
   */
            DBG("shadow: %p (%d, %p) -> %p (%d, %p)", rsc, rsc->b.b.reference.count,
            SWAP(rsc->bo, shadow->bo);
            /* swap() doesn't work because you can't typeof() the bitfield. */
   bool temp = shadow->needs_ubwc_clear;
   shadow->needs_ubwc_clear = rsc->needs_ubwc_clear;
            SWAP(rsc->layout, shadow->layout);
            /* at this point, the newly created shadow buffer is not referenced
   * by any batches, but the existing rsc (probably) is.  We need to
   * transfer those references over:
   */
   assert(shadow->track->batch_mask == 0);
   foreach_batch (batch, &ctx->screen->batch_cache, rsc->track->batch_mask) {
      struct set_entry *entry = _mesa_set_search_pre_hashed(batch->resources, rsc->hash, rsc);
   _mesa_set_remove(batch->resources, entry);
      }
                     struct pipe_blit_info blit = {};
   blit.dst.resource = prsc;
   blit.dst.format = prsc->format;
   blit.src.resource = pshadow;
   blit.src.format = pshadow->format;
   blit.mask = util_format_get_mask(prsc->format);
         #define set_box(field, val)                                                    \
      do {                                                                        \
      blit.dst.field = (val);                                                  \
               /* Disable occlusion queries during shadow blits. */
   bool saved_active_queries = ctx->active_queries;
            /* blit the other levels in their entirety: */
   for (unsigned l = 0; l <= prsc->last_level; l++) {
      if (box && l == level)
            /* just blit whole level: */
   set_box(level, l);
   set_box(box.width, u_minify(prsc->width0, l));
   set_box(box.height, u_minify(prsc->height0, l));
            for (int i = 0; i < prsc->array_size; i++) {
      set_box(box.z, i);
                  /* deal w/ current level specially, since we might need to split
   * it up into a couple blits:
   */
   if (box && !discard_whole_level) {
               switch (prsc->target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
      set_box(box.y, 0);
   set_box(box.z, 0);
                  if (box->x > 0) {
                        }
   if ((box->x + box->width) < u_minify(prsc->width0, level)) {
      set_box(box.x, box->x + box->width);
                     }
      case PIPE_TEXTURE_2D:
         default:
                                                   }
      /**
   * Uncompress an UBWC compressed buffer "in place".  This works basically
   * like resource shadowing, creating a new resource, and doing an uncompress
   * blit, and swapping the state between shadow and original resource so it
   * appears to the gallium frontends as if nothing changed.
   */
   void
   fd_resource_uncompress(struct fd_context *ctx, struct fd_resource *rsc, bool linear)
   {
                                 /* shadow should not fail in any cases where we need to uncompress: */
      }
      /**
   * Debug helper to hexdump a resource.
   */
   void
   fd_resource_dump(struct fd_resource *rsc, const char *name)
   {
      fd_bo_cpu_prep(rsc->bo, NULL, FD_BO_PREP_READ);
   printf("%s: \n", name);
      }
      static struct fd_resource *
   fd_alloc_staging(struct fd_context *ctx, struct fd_resource *rsc,
               {
      struct pipe_context *pctx = &ctx->base;
            /* We cannot currently do stencil export on earlier gens, and
   * u_blitter cannot do blits involving stencil otherwise:
   */
   if ((ctx->screen->gen < 6) && !ctx->blit &&
      (util_format_get_mask(tmpl.format) & PIPE_MASK_S))
         tmpl.width0 = box->width;
   tmpl.height0 = box->height;
   /* for array textures, box->depth is the array_size, otherwise
   * for 3d textures, it is the depth:
   */
   if (tmpl.array_size > 1) {
      if (tmpl.target == PIPE_TEXTURE_CUBE)
         tmpl.array_size = box->depth;
      } else {
      tmpl.array_size = 1;
      }
   tmpl.last_level = 0;
   tmpl.bind |= PIPE_BIND_LINEAR;
   tmpl.usage = PIPE_USAGE_STAGING;
            struct pipe_resource *pstaging =
         if (!pstaging)
               }
      static void
   fd_blit_from_staging(struct fd_context *ctx,
         {
      DBG("");
   struct pipe_resource *dst = trans->b.b.resource;
            blit.dst.resource = dst;
   blit.dst.format = dst->format;
   blit.dst.level = trans->b.b.level;
   blit.dst.box = trans->b.b.box;
   blit.src.resource = trans->staging_prsc;
   blit.src.format = trans->staging_prsc->format;
   blit.src.level = 0;
   blit.src.box = trans->staging_box;
   blit.mask = util_format_get_mask(trans->staging_prsc->format);
               }
      static void
   fd_blit_to_staging(struct fd_context *ctx, struct fd_transfer *trans) assert_dt
   {
      DBG("");
   struct pipe_resource *src = trans->b.b.resource;
            blit.src.resource = src;
   blit.src.format = src->format;
   blit.src.level = trans->b.b.level;
   blit.src.box = trans->b.b.box;
   blit.dst.resource = trans->staging_prsc;
   blit.dst.format = trans->staging_prsc->format;
   blit.dst.level = 0;
   blit.dst.box = trans->staging_box;
   blit.mask = util_format_get_mask(trans->staging_prsc->format);
               }
      static void
   fd_resource_transfer_flush_region(struct pipe_context *pctx,
               {
               if (ptrans->resource->target == PIPE_BUFFER)
      util_range_add(&rsc->b.b, &rsc->valid_buffer_range,
         }
      static void
   flush_resource(struct fd_context *ctx, struct fd_resource *rsc,
         {
      if (usage & PIPE_MAP_WRITE) {
         } else {
            }
      static void
   fd_flush_resource(struct pipe_context *pctx, struct pipe_resource *prsc)
         {
      struct fd_context *ctx = fd_context(pctx);
            /* Flushing the resource is only required if we are relying on
   * implicit-sync, in which case the rendering must be flushed
   * to the kernel for the fence to be added to the backing GEM
   * object.
   */
   if (ctx->no_implicit_sync)
                     /* If we had to flush a batch, make sure it makes it's way all the
   * way to the kernel:
   */
      }
      static void
   fd_resource_transfer_unmap(struct pipe_context *pctx,
               {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_resource *rsc = fd_resource(ptrans->resource);
            if (trans->staging_prsc) {
      if (ptrans->usage & PIPE_MAP_WRITE)
                     if (trans->upload_ptr) {
      fd_bo_upload(rsc->bo, trans->upload_ptr, ptrans->box.x, ptrans->box.width);
               util_range_add(&rsc->b.b, &rsc->valid_buffer_range, ptrans->box.x,
                              /* Don't use pool_transfers_unsync. We are always in the driver
   * thread. Freeing an object into a different pool is allowed.
   */
      }
      static void
   invalidate_resource(struct fd_resource *rsc, unsigned usage) assert_dt
   {
      bool needs_flush = pending(rsc, !!(usage & PIPE_MAP_WRITE));
            if (needs_flush || resource_busy(rsc, op)) {
      rebind_resource(rsc);
      } else {
            }
      static bool
   valid_range(struct fd_resource *rsc, const struct pipe_box *box)
   {
         }
      static void *
   resource_transfer_map_staging(struct pipe_context *pctx,
                                 {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_resource *rsc = fd_resource(prsc);
                     staging_rsc = fd_alloc_staging(ctx, rsc, level, box, usage);
   if (!staging_rsc)
            trans->staging_prsc = &staging_rsc->b.b;
   trans->b.b.stride = fd_resource_pitch(staging_rsc, 0);
   trans->b.b.layer_stride = fd_resource_layer_stride(staging_rsc, 0);
   trans->staging_box = *box;
   trans->staging_box.x = 0;
   trans->staging_box.y = 0;
            if (usage & PIPE_MAP_READ) {
                                       }
      static void *
   resource_transfer_map_unsync(struct pipe_context *pctx,
                     {
      struct fd_resource *rsc = fd_resource(prsc);
   enum pipe_format format = prsc->format;
   uint32_t offset;
            if ((prsc->target == PIPE_BUFFER) &&
      !(usage & (PIPE_MAP_READ | PIPE_MAP_DIRECTLY | PIPE_MAP_PERSISTENT)) &&
   ((usage & PIPE_MAP_DISCARD_RANGE) || !valid_range(rsc, box)) &&
   fd_bo_prefer_upload(rsc->bo, box->width)) {
   trans->upload_ptr = malloc(box->width);
                        /* With imported bo's allocated by something outside of mesa, when
   * running in a VM (using virtio_gpu kernel driver) we could end up in
   * a situation where we have a linear bo, but are unable to mmap it
   * because it was allocated without the VIRTGPU_BLOB_FLAG_USE_MAPPABLE
   * flag.  So we need end up needing to do a staging blit instead:
   */
   if (!buf)
            offset = box->y / util_format_get_blockheight(format) * trans->b.b.stride +
                  if (usage & PIPE_MAP_WRITE)
               }
      /**
   * Note, with threaded_context, resource_transfer_map() is only called
   * in driver thread, but resource_transfer_map_unsync() can be called in
   * either driver or frontend thread.
   */
   static void *
   resource_transfer_map(struct pipe_context *pctx, struct pipe_resource *prsc,
                     {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_resource *rsc = fd_resource(prsc);
   char *buf;
                     /* Strip the read flag if the buffer has been invalidated (or is freshly
   * created). Avoids extra staging blits of undefined data on glTexSubImage of
   * a fresh DEPTH_COMPONENT or STENCIL_INDEX texture being stored as z24s8.
   */
   if (!rsc->valid)
            /* we always need a staging texture for tiled buffers:
   *
   * TODO we might sometimes want to *also* shadow the resource to avoid
   * splitting a batch.. for ex, mid-frame texture uploads to a tiled
   * texture.
   */
   if (rsc->layout.tile_mode) {
         } else if ((usage & PIPE_MAP_READ) && !fd_bo_is_cached(rsc->bo)) {
      perf_debug_ctx(ctx, "wc readback: prsc=%p, level=%u, usage=%x, box=%dx%d+%d,%d",
               if (usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE) {
         } else {
      unsigned op = translate_usage(usage);
            /* If the GPU is writing to the resource, or if it is reading from the
   * resource and we're trying to write to it, flush the renders.
   */
            /* if we need to flush/stall, see if we can make a shadow buffer
   * to avoid this:
   *
   * TODO we could go down this path !reorder && !busy_for_read
   * ie. we only *don't* want to go down this path if the blit
   * will trigger a flush!
   */
   if (ctx->screen->reorder && busy && !(usage & PIPE_MAP_READ) &&
               /* try shadowing only if it avoids a flush, otherwise staging would
   * be better:
   */
   if (needs_flush && !(usage & TC_TRANSFER_MAP_NO_INVALIDATE) &&
            needs_flush = busy = false;
                        if (needs_flush) {
      perf_debug_ctx(ctx, "flushing: %" PRSC_FMT, PRSC_ARGS(prsc));
                     /* in this case, we don't need to shadow the whole resource,
   * since any draw that references the previous contents has
   * already had rendering flushed for all tiles.  So we can
   * use a staging buffer to do the upload.
   */
   if (is_renderable(prsc))
         if (staging_rsc) {
      trans->staging_prsc = &staging_rsc->b.b;
   trans->b.b.stride = fd_resource_pitch(staging_rsc, 0);
   trans->b.b.layer_stride =
         trans->staging_box = *box;
   trans->staging_box.x = 0;
                                                   if (needs_flush) {
      flush_resource(ctx, rsc, usage);
               /* The GPU keeps track of how the various bo's are being used, and
   * will wait if necessary for the proper operation to have
   * completed.
   */
   if (busy) {
      ret = fd_resource_wait(ctx, rsc, op);
   if (ret)
                     }
      static unsigned
   improve_transfer_map_usage(struct fd_context *ctx, struct fd_resource *rsc,
            /* Not *strictly* true, but the access to things that must only be in driver-
   * thread are protected by !(usage & TC_TRANSFER_MAP_THREADED_UNSYNC):
   */
      {
      if (usage & TC_TRANSFER_MAP_NO_INVALIDATE) {
                  if (usage & TC_TRANSFER_MAP_THREADED_UNSYNC)
            if (!(usage &
            if (ctx->in_shadow && !(usage & PIPE_MAP_READ)) {
         } else if ((usage & PIPE_MAP_WRITE) && (rsc->b.b.target == PIPE_BUFFER) &&
            /* We are trying to write to a previously uninitialized range. No need
   * to synchronize.
   */
                     }
      static void *
   fd_resource_transfer_map(struct pipe_context *pctx, struct pipe_resource *prsc,
                     {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_resource *rsc = fd_resource(prsc);
   struct fd_transfer *trans;
            DBG("prsc=%p, level=%u, usage=%x, box=%dx%d+%d,%d", prsc, level, usage,
            if ((usage & PIPE_MAP_DIRECTLY) && rsc->layout.tile_mode) {
      DBG("CANNOT MAP DIRECTLY!\n");
               if (usage & TC_TRANSFER_MAP_THREADED_UNSYNC) {
         } else {
                  if (!ptrans)
                              pipe_resource_reference(&ptrans->resource, prsc);
   ptrans->level = level;
   ptrans->usage = usage;
   ptrans->box = *box;
   ptrans->stride = fd_resource_pitch(rsc, level);
            void *ret;
   if (usage & PIPE_MAP_UNSYNCHRONIZED) {
         } else {
                  if (ret) {
         } else {
                     }
      static void
   fd_resource_destroy(struct pipe_screen *pscreen, struct pipe_resource *prsc)
   {
      struct fd_screen *screen = fd_screen(prsc->screen);
            if (!rsc->is_replacement)
         if (rsc->bo)
         if (rsc->lrz)
         if (rsc->scanout)
            if (prsc->target == PIPE_BUFFER)
                     util_range_destroy(&rsc->valid_buffer_range);
   simple_mtx_destroy(&rsc->lock);
               }
      static uint64_t
   fd_resource_modifier(struct fd_resource *rsc)
   {
      if (!rsc->layout.tile_mode)
            if (rsc->layout.ubwc_layer_size)
            /* TODO invent a modifier for tiled but not UBWC buffers: */
      }
      static bool
   fd_resource_get_handle(struct pipe_screen *pscreen, struct pipe_context *pctx,
                     {
                        if (prsc->target == PIPE_BUFFER)
                              bool ret = fd_screen_bo_get_handle(pscreen, rsc->bo, rsc->scanout,
                                 struct fd_context *ctx = pctx ?
            /* Since gl is horrible, we can end up getting asked to export a handle
   * for a rsc which was not originally allocated in a way that can be
   * exported (for ex, sub-allocation or in the case of virtgpu we need
   * to tell the kernel at allocation time that the buffer can be shared)
   *
   * If we get into this scenario we can try to reallocate.
                              if (!pctx)
            if (!ret)
                           }
      /* special case to resize query buf after allocated.. */
   void
   fd_resource_resize(struct pipe_resource *prsc, uint32_t sz)
   {
               assert(prsc->width0 == 0);
   assert(prsc->target == PIPE_BUFFER);
            prsc->width0 = sz;
      }
      static void
   fd_resource_layout_init(struct pipe_resource *prsc)
   {
      struct fd_resource *rsc = fd_resource(prsc);
                     layout->width0 = prsc->width0;
   layout->height0 = prsc->height0;
            layout->cpp = util_format_get_blocksize(prsc->format);
   layout->cpp *= fd_resource_nr_samples(prsc);
      }
      static struct fd_resource *
   alloc_resource_struct(struct pipe_screen *pscreen,
         {
      struct fd_screen *screen = fd_screen(pscreen);
            if (!rsc)
            struct pipe_resource *prsc = &rsc->b.b;
            pipe_reference_init(&prsc->reference, 1);
   prsc->screen = pscreen;
            util_range_init(&rsc->valid_buffer_range);
            rsc->track = CALLOC_STRUCT(fd_resource_tracking);
   if (!rsc->track) {
      free(rsc);
                        bool allow_cpu_storage = (tmpl->target == PIPE_BUFFER) &&
                  if (tmpl->target == PIPE_BUFFER)
               }
      enum fd_layout_type {
      ERROR,
   LINEAR,
   TILED,
      };
      static bool
   has_implicit_modifier(const uint64_t *modifiers, int count)
   {
      return count == 0 ||
      }
      static bool
   has_explicit_modifier(const uint64_t *modifiers, int count)
   {
      for (int i = 0; i < count; i++) {
      if (modifiers[i] != DRM_FORMAT_MOD_INVALID)
      }
      }
      static enum fd_layout_type
   get_best_layout(struct fd_screen *screen,
               {
      const bool can_implicit = has_implicit_modifier(modifiers, count);
            /* First, find all the conditions which would force us to linear */
   if (!screen->tile_mode)
            if (!screen->tile_mode(tmpl))
            if (tmpl->target == PIPE_BUFFER)
            if (tmpl->bind & PIPE_BIND_LINEAR) {
      if (tmpl->usage != PIPE_USAGE_STAGING)
      perf_debug("%" PRSC_FMT ": forcing linear: bind flags",
                  if (FD_DBG(NOTILE))
            /* Shared resources without explicit modifiers must always be linear */
   if (!can_explicit && (tmpl->bind & PIPE_BIND_SHARED)) {
      perf_debug("%" PRSC_FMT
                           bool ubwc_ok = is_a6xx(screen);
   if (FD_DBG(NOUBWC))
            /* Disallow UBWC for front-buffer rendering.  The GPU does not atomically
   * write pixel and header data, nor does the display atomically read it.
   * The result can be visual corruption (ie. moreso than normal tearing).
   */
   if (tmpl->bind & PIPE_BIND_USE_FRONT_RENDERING)
            /* Disallow UBWC when asked not to use data dependent bandwidth compression:
   */
   if (tmpl->bind & PIPE_BIND_CONST_BW)
            if (ubwc_ok && !can_implicit &&
      !drm_find_modifier(DRM_FORMAT_MOD_QCOM_COMPRESSED, modifiers, count)) {
   perf_debug("%" PRSC_FMT
                           if (ubwc_ok)
            if (can_implicit ||
      drm_find_modifier(DRM_FORMAT_MOD_QCOM_TILED3, modifiers, count))
         if (!drm_find_modifier(DRM_FORMAT_MOD_LINEAR, modifiers, count)) {
      perf_debug("%" PRSC_FMT ": need linear but not in modifier set",
                     perf_debug("%" PRSC_FMT ": not using tiling: explicit modifiers and no UBWC",
            }
      /**
   * Helper that allocates a resource and resolves its layout (but doesn't
   * allocate its bo).
   *
   * It returns a pipe_resource (as fd_resource_create_with_modifiers()
   * would do), and also bo's minimum required size as an output argument.
   */
   static struct pipe_resource *
   fd_resource_allocate_and_resolve(struct pipe_screen *pscreen,
                     {
      struct fd_screen *screen = fd_screen(pscreen);
   struct fd_resource *rsc;
   struct pipe_resource *prsc;
   enum pipe_format format = tmpl->format;
            rsc = alloc_resource_struct(pscreen, tmpl);
   if (!rsc)
                     /* Clover creates buffers with PIPE_FORMAT_NONE: */
   if ((prsc->target == PIPE_BUFFER) && (format == PIPE_FORMAT_NONE))
                     if (tmpl->bind & PIPE_BIND_SHARED)
                     enum fd_layout_type layout =
         if (layout == ERROR) {
      free(prsc);
               if (layout >= TILED)
         if (layout == UBWC)
                     if (prsc->target == PIPE_BUFFER) {
      assert(prsc->format == PIPE_FORMAT_R8_UNORM);
   size = prsc->width0;
      } else {
                  /* special case for hw-query buffer, which we need to allocate before we
   * know the size:
   */
   if (size == 0) {
      /* note, semi-intention == instead of & */
   assert(prsc->bind == PIPE_BIND_QUERY_BUFFER);
   *psize = 0;
               /* Set the layer size if the (non-a6xx) backend hasn't done so. */
   if (rsc->layout.layer_first && !rsc->layout.layer_size) {
      rsc->layout.layer_size = align(size, 4096);
               if (FD_DBG(LAYOUT))
            /* Hand out the resolved size. */
   if (psize)
               }
      /**
   * Create a new texture object, using the given template info.
   */
   static struct pipe_resource *
   fd_resource_create_with_modifiers(struct pipe_screen *pscreen,
               {
      struct fd_screen *screen = fd_screen(pscreen);
   struct fd_resource *rsc;
   struct pipe_resource *prsc;
            /* when using kmsro, scanout buffers are allocated on the display device
   * create_with_modifiers() doesn't give us usage flags, so we have to
   * assume that all calls with modifiers are scanout-possible
   */
   if (screen->ro &&
      ((tmpl->bind & PIPE_BIND_SCANOUT) ||
   has_explicit_modifier(modifiers, count))) {
   struct pipe_resource scanout_templat = *tmpl;
   struct renderonly_scanout *scanout;
            /* note: alignment is wrong for a6xx */
            scanout =
         if (!scanout)
                     assert(handle.type == WINSYS_HANDLE_TYPE_FD);
   rsc = fd_resource(pscreen->resource_from_handle(
         close(handle.handle);
   if (!rsc)
                        prsc =
         if (!prsc)
                  realloc_bo(rsc, size);
   if (!rsc->bo)
               fail:
      fd_resource_destroy(pscreen, prsc);
      }
      static struct pipe_resource *
   fd_resource_create(struct pipe_screen *pscreen,
         {
      const uint64_t mod = DRM_FORMAT_MOD_INVALID;
      }
      /**
   * Create a texture from a winsys_handle. The handle is often created in
   * another process by first creating a pipe texture and then calling
   * resource_get_handle.
   */
   static struct pipe_resource *
   fd_resource_from_handle(struct pipe_screen *pscreen,
               {
      struct fd_screen *screen = fd_screen(pscreen);
            if (!rsc)
            if (tmpl->target == PIPE_BUFFER)
            struct fdl_slice *slice = fd_resource_slice(rsc, 0);
                                       struct fd_bo *bo = fd_screen_bo_from_handle(pscreen, handle);
   if (!bo)
                     rsc->internal_format = tmpl->format;
   rsc->layout.layer_first = true;
   rsc->layout.pitch0 = handle->stride;
   slice->offset = handle->offset;
            /* use a pitchalign of gmem_align_w pixels, because GMEM resolve for
   * lower alignments is not implemented (but possible for a6xx at least)
   *
   * for UBWC-enabled resources, layout_resource_for_modifier will further
   * validate the pitch and set the right pitchalign
   */
   rsc->layout.pitchalign =
            /* apply the minimum pitchalign (note: actually 4 for a3xx but doesn't
   * matter) */
   if (is_a6xx(screen) || is_a5xx(screen))
         else
            if (rsc->layout.pitch0 < (prsc->width0 * rsc->layout.cpp) ||
      fd_resource_pitch(rsc, 0) != rsc->layout.pitch0)
                  if (screen->layout_resource_for_modifier(rsc, handle->modifier) < 0)
            if (screen->ro) {
      rsc->scanout =
                                    fail:
      fd_resource_destroy(pscreen, prsc);
      }
      bool
   fd_render_condition_check(struct pipe_context *pctx)
   {
               if (!ctx->cond_query)
                     union pipe_query_result res = {0};
   bool wait = ctx->cond_mode != PIPE_RENDER_COND_NO_WAIT &&
            if (pctx->get_query_result(pctx, ctx->cond_query, wait, &res))
               }
      static void
   fd_invalidate_resource(struct pipe_context *pctx,
         {
      struct fd_context *ctx = fd_context(pctx);
            if (prsc->target == PIPE_BUFFER) {
      /* Handle the glInvalidateBufferData() case:
   */
      } else if (rsc->track->write_batch) {
      /* Handle the glInvalidateFramebuffer() case, telling us that
   * we can skip resolve.
            struct fd_batch *batch = rsc->track->write_batch;
            if (pfb->zsbuf && pfb->zsbuf->texture == prsc) {
      batch->resolve &= ~(FD_BUFFER_DEPTH | FD_BUFFER_STENCIL);
               for (unsigned i = 0; i < pfb->nr_cbufs; i++) {
      if (pfb->cbufs[i] && pfb->cbufs[i]->texture == prsc) {
      batch->resolve &= ~(PIPE_CLEAR_COLOR0 << i);
                        }
      static enum pipe_format
   fd_resource_get_internal_format(struct pipe_resource *prsc)
   {
         }
      static void
   fd_resource_set_stencil(struct pipe_resource *prsc,
         {
         }
      static struct pipe_resource *
   fd_resource_get_stencil(struct pipe_resource *prsc)
   {
      struct fd_resource *rsc = fd_resource(prsc);
   if (rsc->stencil)
            }
      static const struct u_transfer_vtbl transfer_vtbl = {
      .resource_create = fd_resource_create,
   .resource_destroy = fd_resource_destroy,
   .transfer_map = fd_resource_transfer_map,
   .transfer_flush_region = fd_resource_transfer_flush_region,
   .transfer_unmap = fd_resource_transfer_unmap,
   .get_internal_format = fd_resource_get_internal_format,
   .set_stencil = fd_resource_set_stencil,
      };
      static int
   fd_layout_resource_for_modifier(struct fd_resource *rsc, uint64_t modifier)
   {
      switch (modifier) {
   case DRM_FORMAT_MOD_LINEAR:
      /* The dri gallium frontend will pass DRM_FORMAT_MOD_INVALID to us
   * when it's called through any of the non-modifier BO create entry
   * points.  Other drivers will determine tiling from the kernel or
   * other legacy backchannels, but for freedreno it just means
      case DRM_FORMAT_MOD_INVALID:
         default:
            }
      static struct pipe_resource *
   fd_resource_from_memobj(struct pipe_screen *pscreen,
               {
      struct fd_screen *screen = fd_screen(pscreen);
   struct fd_memory_object *memobj = fd_memory_object(pmemobj);
   struct pipe_resource *prsc;
   struct fd_resource *rsc;
   uint32_t size;
            /* We shouldn't get a scanout buffer here. */
            uint64_t modifiers = DRM_FORMAT_MOD_INVALID;
   if (tmpl->bind & PIPE_BIND_LINEAR) {
         } else if (is_a6xx(screen) && tmpl->width0 >= FDL_MIN_UBWC_WIDTH) {
                  /* Allocate new pipe resource. */
   prsc = fd_resource_allocate_and_resolve(pscreen, tmpl, &modifiers, 1, &size);
   if (!prsc)
         rsc = fd_resource(prsc);
            /* bo's size has to be large enough, otherwise cleanup resource and fail
   * gracefully.
   */
   if (fd_bo_size(memobj->bo) < size) {
      fd_resource_destroy(pscreen, prsc);
               /* Share the bo with the memory object. */
               }
      static struct pipe_memory_object *
   fd_memobj_create_from_handle(struct pipe_screen *pscreen,
         {
      struct fd_memory_object *memobj = CALLOC_STRUCT(fd_memory_object);
   if (!memobj)
            struct fd_bo *bo = fd_screen_bo_from_handle(pscreen, whandle);
   if (!bo) {
      free(memobj);
               memobj->b.dedicated = dedicated;
               }
      static void
   fd_memobj_destroy(struct pipe_screen *pscreen,
         {
               assert(memobj->bo);
               }
      void
   fd_resource_screen_init(struct pipe_screen *pscreen)
   {
               pscreen->resource_create = u_transfer_helper_resource_create;
   /* NOTE: u_transfer_helper does not yet support the _with_modifiers()
   * variant:
   */
   pscreen->resource_create_with_modifiers = fd_resource_create_with_modifiers;
   pscreen->resource_from_handle = fd_resource_from_handle;
   pscreen->resource_get_handle = fd_resource_get_handle;
            pscreen->transfer_helper =
      u_transfer_helper_create(&transfer_vtbl,
               if (!screen->layout_resource_for_modifier)
            /* GL_EXT_memory_object */
   pscreen->memobj_create_from_handle = fd_memobj_create_from_handle;
   pscreen->memobj_destroy = fd_memobj_destroy;
      }
      static void
   fd_blit_pipe(struct pipe_context *pctx,
         {
      /* wrap fd_blit to return void */
      }
      void
   fd_resource_context_init(struct pipe_context *pctx)
   {
      pctx->buffer_map = u_transfer_helper_transfer_map;
   pctx->texture_map = u_transfer_helper_transfer_map;
   pctx->transfer_flush_region = u_transfer_helper_transfer_flush_region;
   pctx->buffer_unmap = u_transfer_helper_transfer_unmap;
   pctx->texture_unmap = u_transfer_helper_transfer_unmap;
   pctx->buffer_subdata = u_default_buffer_subdata;
   pctx->texture_subdata = u_default_texture_subdata;
   pctx->create_surface = fd_create_surface;
   pctx->surface_destroy = fd_surface_destroy;
   pctx->resource_copy_region = fd_resource_copy_region;
   pctx->blit = fd_blit_pipe;
   pctx->flush_resource = fd_flush_resource;
   pctx->invalidate_resource = fd_invalidate_resource;
      }
