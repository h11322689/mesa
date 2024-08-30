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
      #include "util/hash_table.h"
   #include "util/list.h"
   #include "util/set.h"
   #include "util/u_string.h"
      #include "freedreno_batch.h"
   #include "freedreno_context.h"
   #include "freedreno_fence.h"
   #include "freedreno_query_hw.h"
   #include "freedreno_resource.h"
      static struct fd_ringbuffer *
   alloc_ring(struct fd_batch *batch, unsigned sz, enum fd_ringbuffer_flags flags)
   {
               /* if kernel is too old to support unlimited # of cmd buffers, we
   * have no option but to allocate large worst-case sizes so that
   * we don't need to grow the ringbuffer.  Performance is likely to
   * suffer, but there is no good alternative.
   *
   * Otherwise if supported, allocate a growable ring with initial
   * size of zero.
   */
   if ((fd_device_version(ctx->screen->dev) >= FD_VERSION_UNLIMITED_CMDS) &&
      !FD_DBG(NOGROW)) {
   flags |= FD_RINGBUFFER_GROWABLE;
                  }
      static struct fd_batch_subpass *
   subpass_create(struct fd_batch *batch)
   {
                        /* Replace batch->draw with reference to current subpass, for
   * backwards compat with code that is not subpass aware.
   */
   if (batch->draw)
                              }
      static void
   subpass_destroy(struct fd_batch_subpass *subpass)
   {
      fd_ringbuffer_del(subpass->draw);
   if (subpass->subpass_clears)
         list_del(&subpass->node);
   if (subpass->lrz)
            }
      struct fd_batch *
   fd_batch_create(struct fd_context *ctx, bool nondraw)
   {
               if (!batch)
                     pipe_reference_init(&batch->reference, 1);
   batch->ctx = ctx;
            batch->resources =
                     batch->submit = fd_submit_new(ctx->pipe);
   if (batch->nondraw) {
         } else {
               /* a6xx+ re-uses draw rb for both draw and binning pass: */
   if (ctx->screen->gen < 6) {
                     /* Pre-attach private BOs: */
   for (unsigned i = 0; i < ctx->num_private_bos; i++)
                     batch->in_fence_fd = -1;
            /* Work around problems on earlier gens with submit merging, etc,
   * by always creating a fence to request that the submit is flushed
   * immediately:
   */
   if (ctx->screen->gen < 6)
                     util_dynarray_init(&batch->draw_patches, NULL);
            if (is_a2xx(ctx->screen)) {
      util_dynarray_init(&batch->shader_patches, NULL);
               if (is_a3xx(ctx->screen))
                     u_trace_init(&batch->trace, &ctx->trace_context);
               }
      struct fd_batch_subpass *
   fd_batch_create_subpass(struct fd_batch *batch)
   {
                        /* This new subpass inherits the current subpass.. this is replaced
   * if there is a depth clear
   */
   if (batch->subpass->lrz)
                        }
      /**
   * Cleanup that we normally do when the submit is flushed, like dropping
   * rb references.  But also called when batch is destroyed just in case
   * it wasn't flushed.
   */
   static void
   cleanup_submit(struct fd_batch *batch)
   {
      if (!batch->submit)
            foreach_subpass_safe (subpass, batch) {
                  fd_ringbuffer_del(batch->draw);
            if (batch->binning) {
      fd_ringbuffer_del(batch->binning);
               if (batch->prologue) {
      fd_ringbuffer_del(batch->prologue);
               if (batch->tile_epilogue) {
      fd_ringbuffer_del(batch->tile_epilogue);
               if (batch->epilogue) {
      fd_ringbuffer_del(batch->epilogue);
               if (batch->tile_loads) {
      fd_ringbuffer_del(batch->tile_loads);
               if (batch->tile_store) {
      fd_ringbuffer_del(batch->tile_store);
               fd_submit_del(batch->submit);
      }
      static void
   batch_flush_dependencies(struct fd_batch *batch) assert_dt
   {
      struct fd_batch_cache *cache = &batch->ctx->screen->batch_cache;
            foreach_batch (dep, cache, batch->dependents_mask) {
      assert(dep->ctx == batch->ctx);
   fd_batch_flush(dep);
                  }
      static void
   batch_reset_dependencies(struct fd_batch *batch)
   {
      struct fd_batch_cache *cache = &batch->ctx->screen->batch_cache;
            foreach_batch (dep, cache, batch->dependents_mask) {
                     }
      static void
   batch_reset_resources(struct fd_batch *batch)
   {
               set_foreach (batch->resources, entry) {
      struct fd_resource *rsc = (struct fd_resource *)entry->key;
   _mesa_set_remove(batch->resources, entry);
   assert(rsc->track->batch_mask & (1 << batch->idx));
   rsc->track->batch_mask &= ~(1 << batch->idx);
   if (rsc->track->write_batch == batch)
         }
      void
   __fd_batch_destroy_locked(struct fd_batch *batch)
   {
                                          batch_reset_resources(batch);
   assert(batch->resources->entries == 0);
            fd_screen_unlock(ctx->screen);
   batch_reset_dependencies(batch);
                              if (batch->in_fence_fd != -1)
            /* in case batch wasn't flushed but fence was created: */
   if (batch->fence)
                              util_dynarray_fini(&batch->draw_patches);
            if (is_a2xx(batch->ctx->screen)) {
      util_dynarray_fini(&batch->shader_patches);
               if (is_a3xx(batch->ctx->screen))
            while (batch->samples.size > 0) {
      struct fd_hw_sample *samp =
            }
                     free(batch->key);
   free(batch);
      }
      void
   __fd_batch_destroy(struct fd_batch *batch)
   {
      struct fd_screen *screen = batch->ctx->screen;
   fd_screen_lock(screen);
   __fd_batch_destroy_locked(batch);
      }
      void
   __fd_batch_describe(char *buf, const struct fd_batch *batch)
   {
         }
      /* Get per-batch prologue */
   struct fd_ringbuffer *
   fd_batch_get_prologue(struct fd_batch *batch)
   {
      if (!batch->prologue)
            }
      /* Only called from fd_batch_flush() */
   static void
   batch_flush(struct fd_batch *batch) assert_dt
   {
               if (batch->flushed)
                              /* close out the draw cmds by making sure any active queries are
   * paused:
   */
                     fd_screen_lock(batch->ctx->screen);
   batch_reset_resources(batch);
   /* NOTE: remove=false removes the batch from the hashtable, so future
   * lookups won't cache-hit a flushed batch, but leaves the weak reference
   * to the batch to avoid having multiple batches with same batch->idx, as
   * that causes all sorts of hilarity.
   */
   fd_bc_invalidate_batch(batch, false);
            if (batch == batch->ctx->batch)
            if (batch == batch->ctx->batch_nondraw)
                     if (batch->fence)
                                 }
      void
   fd_batch_set_fb(struct fd_batch *batch, const struct pipe_framebuffer_state *pfb)
   {
                        if (!pfb->zsbuf)
                     /* Switching back to a batch we'd previously started constructing shouldn't
   * result in a different lrz.  The dependency tracking should avoid another
   * batch writing/clearing our depth buffer.
   */
   if (batch->subpass->lrz) {
         } else if (zsbuf->lrz) {
            }
         /* NOTE: could drop the last ref to batch
   */
   void
   fd_batch_flush(struct fd_batch *batch)
   {
               /* NOTE: we need to hold an extra ref across the body of flush,
   * since the last ref to this batch could be dropped when cleaning
   * up used_resources
   */
   fd_batch_reference(&tmp, batch);
   batch_flush(tmp);
      }
      /* find a batches dependents mask, including recursive dependencies: */
   static uint32_t
   recursive_dependents_mask(struct fd_batch *batch)
   {
      struct fd_batch_cache *cache = &batch->ctx->screen->batch_cache;
   struct fd_batch *dep;
            foreach_batch (dep, cache, batch->dependents_mask)
               }
      bool
   fd_batch_has_dep(struct fd_batch *batch, struct fd_batch *dep)
   {
         }
      void
   fd_batch_add_dep(struct fd_batch *batch, struct fd_batch *dep)
   {
                        if (fd_batch_has_dep(batch, dep))
            /* a loop should not be possible */
            struct fd_batch *other = NULL;
   fd_batch_reference_locked(&other, dep);
   batch->dependents_mask |= (1 << dep->idx);
      }
      static void
   flush_write_batch(struct fd_resource *rsc) assert_dt
   {
      struct fd_batch *b = NULL;
            fd_screen_unlock(b->ctx->screen);
   fd_batch_flush(b);
               }
      static void
   fd_batch_add_resource(struct fd_batch *batch, struct fd_resource *rsc)
   {
      if (likely(fd_batch_references_resource(batch, rsc))) {
      assert(_mesa_set_search_pre_hashed(batch->resources, rsc->hash, rsc));
                        _mesa_set_add_pre_hashed(batch->resources, rsc->hash, rsc);
            fd_ringbuffer_attach_bo(batch->draw, rsc->bo);
   if (unlikely(rsc->b.b.next)) {
      struct fd_resource *n = fd_resource(rsc->b.b.next);
         }
      void
   fd_batch_resource_write(struct fd_batch *batch, struct fd_resource *rsc)
   {
                                 /* Must do this before the early out, so we unset a previous resource
   * invalidate (which may have left the write_batch state in place).
   */
            if (track->write_batch == batch)
            if (rsc->stencil)
            /* note, invalidate write batch, to avoid further writes to rsc
   * resulting in a write-after-read hazard.
            /* if we are pending read or write by any other batch, they need to
   * be ordered before the current batch:
   */
   if (unlikely(track->batch_mask & ~(1 << batch->idx))) {
      struct fd_batch_cache *cache = &batch->ctx->screen->batch_cache;
            if (track->write_batch) {
      /* Cross-context writes without flush/barrier are undefined.
   * Lets simply protect ourself from crashing by avoiding cross-
   * ctx dependencies and let the app have the undefined behavior
   * it asked for:
   */
   if (track->write_batch->ctx != batch->ctx) {
      fd_ringbuffer_attach_bo(batch->draw, rsc->bo);
                           foreach_batch (dep, cache, track->batch_mask) {
      struct fd_batch *b = NULL;
   if ((dep == batch) || (dep->ctx != batch->ctx))
         /* note that batch_add_dep could flush and unref dep, so
   * we need to hold a reference to keep it live for the
   * fd_bc_invalidate_batch()
   */
   fd_batch_reference(&b, dep);
   fd_batch_add_dep(batch, b);
   fd_bc_invalidate_batch(b, false);
         }
                        }
      void
   fd_batch_resource_read_slowpath(struct fd_batch *batch, struct fd_resource *rsc)
   {
               if (rsc->stencil)
                              /* If reading a resource pending a write, go ahead and flush the
   * writer.  This avoids situations where we end up having to
   * flush the current batch in _resource_used()
   */
   if (unlikely(track->write_batch && track->write_batch != batch)) {
      if (track->write_batch->ctx != batch->ctx) {
      /* Reading results from another context without flush/barrier
   * is undefined.  Let's simply protect ourself from crashing
   * by avoiding cross-ctx dependencies and let the app have the
   * undefined behavior it asked for:
   */
   fd_ringbuffer_attach_bo(batch->draw, rsc->bo);
                              }
      void
   fd_batch_check_size(struct fd_batch *batch)
   {
      if (batch->num_draws > 100000) {
      fd_batch_flush(batch);
               /* Place a reasonable upper bound on prim/draw stream buffer size: */
   const unsigned limit_bits = 8 * 8 * 1024 * 1024;
   if ((batch->prim_strm_bits > limit_bits) ||
      (batch->draw_strm_bits > limit_bits)) {
   fd_batch_flush(batch);
               if (!fd_ringbuffer_check_size(batch->draw))
      }
      /* emit a WAIT_FOR_IDLE only if needed, ie. if there has not already
   * been one since last draw:
   */
   void
   fd_wfi(struct fd_batch *batch, struct fd_ringbuffer *ring)
   {
      if (batch->needs_wfi) {
      if (batch->ctx->screen->gen >= 5)
         else
               }
