   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/crc32.h"
      #include "svga_debug.h"
   #include "svga_format.h"
   #include "svga_winsys.h"
   #include "svga_screen.h"
   #include "svga_screen_cache.h"
   #include "svga_context.h"
   #include "svga_cmd.h"
      #define SVGA_SURFACE_CACHE_ENABLED 1
         /**
   * Return the size of the surface described by the key (in bytes).
   */
   unsigned
   svga_surface_size(const struct svga_host_surface_cache_key *key)
   {
               assert(key->numMipLevels > 0);
   assert(key->numFaces > 0);
            if (key->format == SVGA3D_BUFFER) {
      /* Special case: we don't want to count vertex/index buffers
   * against the cache size limit, so view them as zero-sized.
   */
                                 for (i = 0; i < key->numMipLevels; i++) {
      unsigned w = u_minify(key->size.width, i);
   unsigned h = u_minify(key->size.height, i);
   unsigned d = u_minify(key->size.depth, i);
   unsigned img_size = ((w + bw - 1) / bw) * ((h + bh - 1) / bh) * d * bpb;
                           }
         /**
   * Compute the bucket for this key.
   */
   static inline unsigned
   svga_screen_cache_bucket(const struct svga_host_surface_cache_key *key)
   {
         }
         /**
   * Search the cache for a surface that matches the key.  If a match is
   * found, remove it from the cache and return the surface pointer.
   * Return NULL otherwise.
   */
   static struct svga_winsys_surface *
   svga_screen_cache_lookup(struct svga_screen *svgascreen,
         {
      struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_host_surface_cache_entry *entry;
   struct svga_winsys_surface *handle = NULL;
   struct list_head *curr, *next;
   unsigned bucket;
                                       curr = cache->bucket[bucket].next;
   next = curr->next;
   while (curr != &cache->bucket[bucket]) {
                                 /* If the key matches and the fence is signalled (the surface is no
   * longer needed) the lookup was successful.  We found a surface that
   * can be reused.
   * We unlink the surface from the cache entry and we add the entry to
   * the 'empty' list.
   */
   if (memcmp(&entry->key, key, sizeof *key) == 0 &&
                                                                                          /* update the cache size */
   surf_size = svga_surface_size(&entry->key);
   assert(surf_size <= cache->total_size);
   if (surf_size > cache->total_size)
                                    curr = next;
                        if (SVGA_DEBUG & DEBUG_DMA)
      debug_printf("%s: cache %s after %u tries (bucket %d)\n", __func__,
            }
         /**
   * Free the least recently used entries in the surface cache until the
   * cache size is <= the target size OR there are no unused entries left
   * to discard.  We don't do any flushing to try to free up additional
   * surfaces.
   */
   static void
   svga_screen_cache_shrink(struct svga_screen *svgascreen,
         {
      struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
            /* Walk over the list of unused buffers in reverse order: from oldest
   * to newest.
   */
   LIST_FOR_EACH_ENTRY_SAFE_REV(entry, next_entry, &cache->unused, head) {
      if (entry->key.format != SVGA3D_BUFFER) {
                                       list_del(&entry->bucket_head);
                  if (cache->total_size <= target_size) {
      /* all done */
               }
         /**
   * Add a surface to the cache.  This is done when the driver deletes
   * the surface.  Note: transfers a handle reference.
   */
   static void
   svga_screen_cache_add(struct svga_screen *svgascreen,
                     {
      struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_host_surface_cache_entry *entry = NULL;
   struct svga_winsys_surface *handle = *p_handle;
                     if (!handle)
                     *p_handle = NULL;
            if (surf_size >= SVGA_HOST_SURFACE_CACHE_BYTES) {
      /* this surface is too large to cache, just free it */
   sws->surface_reference(sws, &handle, NULL);
   mtx_unlock(&cache->mutex);
               if (cache->total_size + surf_size > SVGA_HOST_SURFACE_CACHE_BYTES) {
      /* Adding this surface would exceed the cache size.
   * Try to discard least recently used entries until we hit the
   * new target cache size.
   */
                     if (cache->total_size > target_size) {
      /* we weren't able to shrink the cache as much as we wanted so
   * just discard this surface.
   */
   sws->surface_reference(sws, &handle, NULL);
   mtx_unlock(&cache->mutex);
                  if (!list_is_empty(&cache->empty)) {
      /* An empty entry has no surface associated with it.
   * Use the first empty entry.
   */
   entry = list_entry(cache->empty.next,
                  /* Remove from LRU list */
      }
   else if (!list_is_empty(&cache->unused)) {
      /* free the last used buffer and reuse its entry */
   entry = list_entry(cache->unused.prev,
               SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
                              /* Remove from hash table */
            /* Remove from LRU list */
               if (entry) {
      assert(entry->handle == NULL);
   entry->handle = handle;
            SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
            /* If we don't have gb objects, we don't need to invalidate. */
   if (sws->have_gb_objects) {
      if (to_invalidate)
         else
      }
   else
               }
   else {
      /* Couldn't cache the buffer -- this really shouldn't happen */
   SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
                        }
         /* Maximum number of invalidate surface commands in a command buffer */
   # define SVGA_MAX_SURFACE_TO_INVALIDATE 1000
      /**
   * Called during the screen flush to move all buffers not in a validate list
   * into the unused list.
   */
   void
   svga_screen_cache_flush(struct svga_screen *svgascreen,
               {
      struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_host_surface_cache_entry *entry;
   struct list_head *curr, *next;
                     /* Loop over entries in the invalidated list */
   curr = cache->invalidated.next;
   next = curr->next;
   while (curr != &cache->invalidated) {
                        if (sws->surface_is_flushed(sws, entry->handle)) {
                                             /* Add entry to the hash table bucket */
   bucket = svga_screen_cache_bucket(&entry->key);
               curr = next;
               unsigned nsurf = 0;
   curr = cache->validated.next;
   next = curr->next;
   while (curr != &cache->validated) {
               assert(entry->handle);
            if (sws->surface_is_flushed(sws, entry->handle)) {
                     /* It is now safe to invalidate the surface content.
   * It will be done using the current context.
   */
   if (SVGA_TRY(SVGA3D_InvalidateGBSurface(svga->swc, entry->handle))
                     /* Even though surface invalidation here is done after the command
   * buffer is flushed, it is still possible that it will
   * fail because there might be just enough of this command that is
   * filling up the command buffer, so in this case we will call
   * the winsys flush directly to flush the buffer.
   * Note, we don't want to call svga_context_flush() here because
   * this function itself is called inside svga_context_flush().
   */
   svga_retry_enter(svga);
   svga->swc->flush(svga->swc, NULL);
   nsurf = 0;
   ret = SVGA3D_InvalidateGBSurface(svga->swc, entry->handle);
   svga_retry_exit(svga);
                        list_add(&entry->head, &cache->invalidated);
               curr = next;
                        /**
   * In some rare cases (when running ARK survival), we hit the max number
   * of surface relocations with invalidated surfaces during context flush.
   * So if the number of invalidated surface exceeds a certain limit (1000),
   * we'll do another winsys flush.
   */
   if (nsurf > SVGA_MAX_SURFACE_TO_INVALIDATE) {
            }
         /**
   * Free all the surfaces in the cache.
   * Called when destroying the svga screen object.
   */
   void
   svga_screen_cache_cleanup(struct svga_screen *svgascreen)
   {
      struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
            for (i = 0; i < SVGA_HOST_SURFACE_CACHE_SIZE; ++i) {
      SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
         sws->surface_reference(sws, &cache->entries[i].handle, NULL);
                           if (cache->entries[i].fence)
                  }
         enum pipe_error
   svga_screen_cache_init(struct svga_screen *svgascreen)
   {
      struct svga_host_surface_cache *cache = &svgascreen->cache;
                              for (i = 0; i < SVGA_HOST_SURFACE_CACHE_BUCKETS; ++i)
                                       list_inithead(&cache->empty);
   for (i = 0; i < SVGA_HOST_SURFACE_CACHE_SIZE; ++i)
               }
         /**
   * Allocate a new host-side surface.  If the surface is marked as cachable,
   * first try re-using a surface in the cache of freed surfaces.  Otherwise,
   * allocate a new surface.
   * \param bind_flags  bitmask of PIPE_BIND_x flags
   * \param usage  one of PIPE_USAGE_x values
   * \param validated return True if the surface is a reused surface
   */
   struct svga_winsys_surface *
   svga_screen_surface_create(struct svga_screen *svgascreen,
                     {
      struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_winsys_surface *handle = NULL;
            SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
            "%s sz %dx%dx%d mips %d faces %d arraySize %d cachable %d\n",
   __func__,
   key->size.width,
   key->size.height,
   key->size.depth,
   key->numMipLevels,
   key->numFaces,
         if (cachable) {
      /* Try to re-cycle a previously freed, cached surface */
   if (key->format == SVGA3D_BUFFER) {
               /* For buffers, round the buffer size up to the nearest power
   * of two to increase the probability of cache hits.  Keep
   * texture surface dimensions unchanged.
   */
   uint32_t size = 1;
   while (size < key->size.width)
                  /* Determine whether the buffer is static or dynamic.
   * This is a bit of a heuristic which can be tuned as needed.
   */
   if (usage == PIPE_USAGE_DEFAULT ||
      usage == PIPE_USAGE_IMMUTABLE) {
      }
   else if (bind_flags & PIPE_BIND_INDEX_BUFFER) {
      /* Index buffers don't change too often.  Mark them as static.
   */
      }
   else {
      /* Since we're reusing buffers we're effectively transforming all
   * of them into dynamic buffers.
   *
   * It would be nice to not cache long lived static buffers. But there
   * is no way to detect the long lived from short lived ones yet. A
   * good heuristic would be buffer size.
   */
               key->flags &= ~(SVGA3D_SURFACE_HINT_STATIC |
                     handle = svga_screen_cache_lookup(svgascreen, key);
   if (handle) {
      if (key->format == SVGA3D_BUFFER)
      SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
            else
      SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
            "reuse sid %p sz %dx%dx%d mips %d faces %d arraySize %d\n", handle,
   key->size.width,
   key->size.height,
   key->size.depth,
   key->numMipLevels,
                  if (!handle) {
      /* Unable to recycle surface, allocate a new one */
            /* mark the surface as shareable if the surface is not
   * cachable or the RENDER_TARGET bind flag is set.
   */
   if (!key->cachable ||
      ((bind_flags & PIPE_BIND_RENDER_TARGET) != 0))
      if (key->scanout)
         if (key->coherent)
            handle = sws->surface_create(sws,
                              key->flags,
   key->format,
      if (handle)
      SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
            "  CREATE sid %p sz %dx%dx%d\n",
   handle,
                           }
         /**
   * Release a surface.  We don't actually free the surface- we put
   * it into the cache of freed surfaces (if it's cachable).
   */
   void
   svga_screen_surface_destroy(struct svga_screen *svgascreen,
                     {
               /* We only set the cachable flag for surfaces of which we are the
   * exclusive owner.  So just hold onto our existing reference in
   * that case.
   */
   if (SVGA_SURFACE_CACHE_ENABLED && key->cachable) {
         }
   else {
      SVGA_DBG(DEBUG_DMA,
               }
         /**
   * Print/dump the contents of the screen cache.  For debugging.
   */
   void
   svga_screen_cache_dump(const struct svga_screen *svgascreen)
   {
      const struct svga_host_surface_cache *cache = &svgascreen->cache;
   unsigned bucket;
            debug_printf("svga3d surface cache:\n");
   for (bucket = 0; bucket < SVGA_HOST_SURFACE_CACHE_BUCKETS; bucket++) {
      struct list_head *curr;
   curr = cache->bucket[bucket].next;
   while (curr && curr != &cache->bucket[bucket]) {
      struct svga_host_surface_cache_entry *entry =
         if (entry->key.format == SVGA3D_BUFFER) {
      debug_printf("  %p: buffer %u bytes\n",
            }
   else {
      debug_printf("  %p: %u x %u x %u format %u\n",
               entry->handle,
   entry->key.size.width,
      }
   curr = curr->next;
                     }
