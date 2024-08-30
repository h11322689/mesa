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
   #define XXH_INLINE_ALL
   #include "util/xxhash.h"
      #include "freedreno_batch.h"
   #include "freedreno_batch_cache.h"
   #include "freedreno_context.h"
   #include "freedreno_resource.h"
      /* Overview:
   *
   *   The batch cache provides lookup for mapping pipe_framebuffer_state
   *   to a batch.
   *
   *   It does this via hashtable, with key that roughly matches the
   *   pipe_framebuffer_state, as described below.
   *
   * Batch Cache hashtable key:
   *
   *   To serialize the key, and to avoid dealing with holding a reference to
   *   pipe_surface's (which hold a reference to pipe_resource and complicate
   *   the whole refcnting thing), the key is variable length and inline's the
   *   pertinent details of the pipe_surface.
   *
   * Batch:
   *
   *   Each batch needs to hold a reference to each resource it depends on (ie.
   *   anything that needs a mem2gmem).  And a weak reference to resources it
   *   renders to.  (If both src[n] and dst[n] are not NULL then they are the
   *   same.)
   *
   *   When a resource is destroyed, we need to remove entries in the batch
   *   cache that reference the resource, to avoid dangling pointer issues.
   *   So each resource holds a hashset of batches which have reference them
   *   in their hashtable key.
   *
   *   When a batch has weak reference to no more resources (ie. all the
   *   surfaces it rendered to are destroyed) the batch can be destroyed.
   *   Could happen in an app that renders and never uses the result.  More
   *   common scenario, I think, will be that some, but not all, of the
   *   surfaces are destroyed before the batch is submitted.
   *
   *   If (for example), batch writes to zsbuf but that surface is destroyed
   *   before batch is submitted, we can skip gmem2mem (but still need to
   *   alloc gmem space as before.  If the batch depended on previous contents
   *   of that surface, it would be holding a reference so the surface would
   *   not have been destroyed.
   */
      struct fd_batch_key {
      uint32_t width;
   uint32_t height;
   uint16_t layers;
   uint16_t samples;
   uint16_t num_surfs;
   uint16_t ctx_seqno;
   struct {
      struct pipe_resource *texture;
   union pipe_surface_desc u;
   uint8_t pos, samples;
         };
      static struct fd_batch_key *
   key_alloc(unsigned num_surfs)
   {
      struct fd_batch_key *key = CALLOC_VARIANT_LENGTH_STRUCT(
            }
      uint32_t
   fd_batch_key_hash(const void *_key)
   {
      const struct fd_batch_key *key = _key;
   uint32_t hash = 0;
   hash = XXH32(key, offsetof(struct fd_batch_key, surf[0]), hash);
   hash = XXH32(key->surf, sizeof(key->surf[0]) * key->num_surfs, hash);
      }
      bool
   fd_batch_key_equals(const void *_a, const void *_b)
   {
      const struct fd_batch_key *a = _a;
   const struct fd_batch_key *b = _b;
   return (memcmp(a, b, offsetof(struct fd_batch_key, surf[0])) == 0) &&
      }
      struct fd_batch_key *
   fd_batch_key_clone(void *mem_ctx, const struct fd_batch_key *key)
   {
      unsigned sz =
         struct fd_batch_key *new_key = rzalloc_size(mem_ctx, sz);
   memcpy(new_key, key, sz);
      }
      void
   fd_bc_init(struct fd_batch_cache *cache)
   {
      cache->ht =
      }
      void
   fd_bc_fini(struct fd_batch_cache *cache)
   {
         }
      /* Flushes all batches in the batch cache.  Used at glFlush() and similar times. */
   void
   fd_bc_flush(struct fd_context *ctx, bool deferred) assert_dt
   {
               /* fd_batch_flush() (and fd_batch_add_dep() which calls it indirectly)
   * can cause batches to be unref'd and freed under our feet, so grab
   * a reference to all the batches we need up-front.
   */
   struct fd_batch *batches[ARRAY_SIZE(cache->batches)] = {0};
   struct fd_batch *batch;
                     foreach_batch (batch, cache, cache->batch_mask) {
      if (batch->ctx == ctx) {
                     /* deferred flush doesn't actually flush, but it marks every other
   * batch associated with the context as dependent on the current
   * batch.  So when the current batch gets flushed, all other batches
   * that came before also get flushed.
   */
   if (deferred) {
      struct fd_batch *current_batch = fd_context_batch(ctx);
   struct fd_batch *deps[ARRAY_SIZE(cache->batches)] = {0};
            /* To avoid a dependency loop, pull out any batches that already
   * have a dependency on the current batch.  This ensures the
   * following loop adding a dependency to the current_batch, all
   * remaining batches do not have a direct or indirect dependency
   * on the current_batch.
   *
   * The batches that have a dependency on the current batch will
   * be flushed immediately (after dropping screen lock) instead
   */
   for (unsigned i = 0; i < n; i++) {
      if ((batches[i] != current_batch) &&
      fd_batch_has_dep(batches[i], current_batch)) {
   /* We can't immediately flush while we hold the screen lock,
   * but that doesn't matter.  We just want to skip adding any
   * deps that would result in a loop, we can flush after we've
   * updated the dependency graph and dropped the lock.
   */
   fd_batch_reference_locked(&deps[ndeps++], batches[i]);
                  for (unsigned i = 0; i < n; i++) {
      if (batches[i] && (batches[i] != current_batch) &&
                                             /* If we have any batches that we could add a dependency on (unlikely)
   * flush them immediately.
   */
   for (unsigned i = 0; i < ndeps; i++) {
      fd_batch_flush(deps[i]);
         } else {
               for (unsigned i = 0; i < n; i++) {
                     for (unsigned i = 0; i < n; i++) {
            }
      /**
   * Flushes the batch (if any) writing this resource.  Must not hold the screen
   * lock.
   */
   void
   fd_bc_flush_writer(struct fd_context *ctx, struct fd_resource *rsc) assert_dt
   {
      fd_screen_lock(ctx->screen);
   struct fd_batch *write_batch = NULL;
   fd_batch_reference_locked(&write_batch, rsc->track->write_batch);
            if (write_batch) {
      if (write_batch->ctx == ctx)
               }
      /**
   * Flushes any batches reading this resource.  Must not hold the screen lock.
   */
   void
   fd_bc_flush_readers(struct fd_context *ctx, struct fd_resource *rsc) assert_dt
   {
      struct fd_batch *batch, *batches[32] = {};
            /* This is a bit awkward, probably a fd_batch_flush_locked()
   * would make things simpler.. but we need to hold the lock
   * to iterate the batches which reference this resource.  So
   * we must first grab references under a lock, then flush.
   */
   fd_screen_lock(ctx->screen);
   foreach_batch (batch, &ctx->screen->batch_cache, rsc->track->batch_mask)
                  for (int i = 0; i < batch_count; i++) {
      if (batches[i]->ctx == ctx)
               }
      void
   fd_bc_dump(struct fd_context *ctx, const char *fmt, ...)
   {
               if (!FD_DBG(MSGS))
                     va_list ap;
   va_start(ap, fmt);
   vprintf(fmt, ap);
            for (int i = 0; i < ARRAY_SIZE(cache->batches); i++) {
      struct fd_batch *batch = cache->batches[i];
   if (batch) {
      printf("  %p<%u>%s\n", batch, batch->seqno,
                              }
      /**
   * Note that when batch is flushed, it needs to remain in the cache so
   * that fd_bc_invalidate_resource() can work.. otherwise we can have
   * the case where a rsc is destroyed while a batch still has a dangling
   * reference to it.
   *
   * Note that the cmdstream (or, after the SUBMIT ioctl, the kernel)
   * would have a reference to the underlying bo, so it is ok for the
   * rsc to be destroyed before the batch.
   */
   void
   fd_bc_invalidate_batch(struct fd_batch *batch, bool remove)
   {
      if (!batch)
            struct fd_batch_cache *cache = &batch->ctx->screen->batch_cache;
                     if (remove) {
      cache->batches[batch->idx] = NULL;
               if (!key)
            DBG("%p: key=%p", batch, batch->key);
   for (unsigned idx = 0; idx < key->num_surfs; idx++) {
      struct fd_resource *rsc = fd_resource(key->surf[idx].texture);
               struct hash_entry *entry =
            }
      void
   fd_bc_invalidate_resource(struct fd_resource *rsc, bool destroy)
   {
      struct fd_screen *screen = fd_screen(rsc->b.b.screen);
                     if (destroy) {
      foreach_batch (batch, &screen->batch_cache, rsc->track->batch_mask) {
      struct set_entry *entry = _mesa_set_search_pre_hashed(batch->resources, rsc->hash, rsc);
      }
                        foreach_batch (batch, &screen->batch_cache, rsc->track->bc_batch_mask)
                        }
      static struct fd_batch *
   alloc_batch_locked(struct fd_batch_cache *cache, struct fd_context *ctx,
         {
      struct fd_batch *batch;
                        #if 0
         for (unsigned i = 0; i < ARRAY_SIZE(cache->batches); i++) {
      batch = cache->batches[i];
   debug_printf("%d: needs_flush=%d, depends:", batch->idx, batch->needs_flush);
   set_foreach (batch->dependencies, entry) {
      struct fd_batch *dep = (struct fd_batch *)entry->key;
      }
      #endif
         /* TODO: is LRU the better policy?  Or perhaps the batch that
   * depends on the fewest other batches?
   */
   struct fd_batch *flush_batch = NULL;
   for (unsigned i = 0; i < ARRAY_SIZE(cache->batches); i++) {
      if (!flush_batch || (cache->batches[i]->seqno < flush_batch->seqno))
               /* we can drop lock temporarily here, since we hold a ref,
   * flush_batch won't disappear under us.
   */
   fd_screen_unlock(ctx->screen);
   DBG("%p: too many batches!  flush forced!", flush_batch);
   fd_batch_flush(flush_batch);
            /* While the resources get cleaned up automatically, the flush_batch
   * doesn't get removed from the dependencies of other batches, so
   * it won't be unref'd and will remain in the table.
   *
   * TODO maybe keep a bitmask of batches that depend on me, to make
   * this easier:
   */
   for (unsigned i = 0; i < ARRAY_SIZE(cache->batches); i++) {
      struct fd_batch *other = cache->batches[i];
   if (!other)
         if (fd_batch_has_dep(other, flush_batch)) {
      other->dependents_mask &= ~(1 << flush_batch->idx);
   struct fd_batch *ref = flush_batch;
                                       batch = fd_batch_create(ctx, nondraw);
   if (!batch)
            batch->seqno = seqno_next(&cache->cnt);
   batch->idx = idx;
            assert(cache->batches[idx] == NULL);
               }
      static void
   alloc_query_buf(struct fd_context *ctx, struct fd_batch *batch)
   {
      if (batch->query_buf)
            if ((ctx->screen->gen < 3) || (ctx->screen->gen > 4))
            /* For gens that use fd_hw_query, pre-allocate an initially zero-sized
   * (unbacked) query buffer.  This simplifies draw/grid/etc-time resource
   * tracking.
   */
   struct pipe_screen *pscreen = &ctx->screen->base;
   struct pipe_resource templ = {
      .target = PIPE_BUFFER,
   .format = PIPE_FORMAT_R8_UNORM,
   .bind = PIPE_BIND_QUERY_BUFFER,
   .width0 = 0, /* create initially zero size buffer */
   .height0 = 1,
   .depth0 = 1,
   .array_size = 1,
   .last_level = 0,
      };
      }
      struct fd_batch *
   fd_bc_alloc_batch(struct fd_context *ctx, bool nondraw)
   {
      struct fd_batch_cache *cache = &ctx->screen->batch_cache;
            /* For normal draw batches, pctx->set_framebuffer_state() handles
   * this, but for nondraw batches, this is a nice central location
   * to handle them all.
   */
   if (nondraw)
            fd_screen_lock(ctx->screen);
   batch = alloc_batch_locked(cache, ctx, nondraw);
                     if (batch && nondraw)
               }
      static struct fd_batch *
   batch_from_key(struct fd_context *ctx, struct fd_batch_key *key) assert_dt
   {
      struct fd_batch_cache *cache = &ctx->screen->batch_cache;
   struct fd_batch *batch = NULL;
   uint32_t hash = fd_batch_key_hash(key);
   struct hash_entry *entry =
            if (entry) {
      free(key);
   fd_batch_reference_locked(&batch, (struct fd_batch *)entry->data);
   assert(!batch->flushed);
                  #ifdef DEBUG
      DBG("%p: hash=0x%08x, %ux%u, %u layers, %u samples", batch, hash, key->width,
         for (unsigned idx = 0; idx < key->num_surfs; idx++) {
      DBG("%p:  surf[%u]: %p (%s) (%u,%u / %u,%u,%u)", batch,
      key->surf[idx].pos, key->surf[idx].texture,
   util_format_name(key->surf[idx].format),
   key->surf[idx].u.buf.first_element, key->surf[idx].u.buf.last_element,
   key->surf[idx].u.tex.first_layer, key->surf[idx].u.tex.last_layer,
      #endif
      if (!batch)
            /* reset max_scissor, which will be adjusted on draws
   * according to the actual scissor.
   */
   batch->max_scissor.minx = ~0;
   batch->max_scissor.miny = ~0;
   batch->max_scissor.maxx = 0;
            _mesa_hash_table_insert_pre_hashed(cache->ht, hash, key, batch);
   batch->key = key;
            for (unsigned idx = 0; idx < key->num_surfs; idx++) {
      struct fd_resource *rsc = fd_resource(key->surf[idx].texture);
                  }
      static void
   key_surf(struct fd_batch_key *key, unsigned idx, unsigned pos,
         {
      key->surf[idx].texture = psurf->texture;
   key->surf[idx].u = psurf->u;
   key->surf[idx].pos = pos;
   key->surf[idx].samples = MAX2(1, psurf->nr_samples);
      }
      struct fd_batch *
   fd_batch_from_fb(struct fd_context *ctx,
         {
      unsigned idx = 0, n = pfb->nr_cbufs + (pfb->zsbuf ? 1 : 0);
            key->width = pfb->width;
   key->height = pfb->height;
   key->layers = pfb->layers;
   key->samples = util_framebuffer_get_num_samples(pfb);
            if (pfb->zsbuf)
            for (unsigned i = 0; i < pfb->nr_cbufs; i++)
      if (pfb->cbufs[i])
                  fd_screen_lock(ctx->screen);
   struct fd_batch *batch = batch_from_key(ctx, key);
                                 }
