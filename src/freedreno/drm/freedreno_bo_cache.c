   /*
   * Copyright (C) 2012-2018 Rob Clark <robclark@freedesktop.org>
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
      #include "freedreno_drmif.h"
   #include "freedreno_priv.h"
      #define FD_BO_CACHE_STATS 0
      #define BO_CACHE_LOG(cache, fmt, ...) do {                      \
         if (FD_BO_CACHE_STATS) {                                  \
                  static void
   bo_remove_from_bucket(struct fd_bo_bucket *bucket, struct fd_bo *bo)
   {
      list_delinit(&bo->node);
      }
      static void
   dump_cache_stats(struct fd_bo_cache *cache)
   {
      if (!FD_BO_CACHE_STATS)
                     if ((++cnt % 32))
            int size = 0;
   int count = 0;
   int hits = 0;
   int misses = 0;
            for (int i = 0; i < cache->num_buckets; i++) {
                        if (bucket->count > 0) {
      struct fd_bo *bo = first_bo(&bucket->list);
   if (fd_bo_state(bo) == FD_BO_STATE_IDLE)
               BO_CACHE_LOG(cache, "bucket[%u]: count=%d\thits=%d\tmisses=%d\texpired=%d%s",
                  size += bucket->size * bucket->count;
   count += bucket->count;
   hits += bucket->hits;
   misses += bucket->misses;
               BO_CACHE_LOG(cache, "size=%d\tcount=%d\thits=%d\tmisses=%d\texpired=%d",
      }
      static void
   add_bucket(struct fd_bo_cache *cache, int size)
   {
      unsigned int i = cache->num_buckets;
                     list_inithead(&bucket->list);
   bucket->size = size;
   bucket->count = 0;
   bucket->hits = 0;
   bucket->misses = 0;
   bucket->expired = 0;
      }
      /**
   * @coarse: if true, only power-of-two bucket sizes, otherwise
   *    fill in for a bit smoother size curve..
   */
   void
   fd_bo_cache_init(struct fd_bo_cache *cache, int coarse, const char *name)
   {
               cache->name = name;
            /* OK, so power of two buckets was too wasteful of memory.
   * Give 3 other sizes between each power of two, to hopefully
   * cover things accurately enough.  (The alternative is
   * probably to just go for exact matching of sizes, and assume
   * that for things like composited window resize the tiled
   * width/height alignment and rounding of sizes to pages will
   * get us useful cache hit rates anyway)
   */
   add_bucket(cache, 4096);
   add_bucket(cache, 4096 * 2);
   if (!coarse)
            /* Initialize the linked lists for BO reuse cache. */
   for (size = 4 * 4096; size <= cache_max_size; size *= 2) {
      add_bucket(cache, size);
   if (!coarse) {
      add_bucket(cache, size + size * 1 / 4);
   add_bucket(cache, size + size * 2 / 4);
            }
      /* Frees older cached buffers.  Called under table_lock */
   void
   fd_bo_cache_cleanup(struct fd_bo_cache *cache, time_t time)
   {
               if (cache->time == time)
                              simple_mtx_lock(&cache->lock);
   for (i = 0; i < cache->num_buckets; i++) {
      struct fd_bo_bucket *bucket = &cache->cache_bucket[i];
            while (!list_is_empty(&bucket->list)) {
               /* keep things in cache for at least 1 second: */
                  if (cnt == 0) {
      BO_CACHE_LOG(cache, "cache cleanup");
               VG_BO_OBTAIN(bo);
   bo_remove_from_bucket(bucket, bo);
                        }
                     if (cnt > 0) {
      BO_CACHE_LOG(cache, "cache cleaned %u BOs", cnt);
                  }
      static struct fd_bo_bucket *
   get_bucket(struct fd_bo_cache *cache, uint32_t size)
   {
               /* hmm, this is what intel does, but I suppose we could calculate our
   * way to the correct bucket size rather than looping..
   */
   for (i = 0; i < cache->num_buckets; i++) {
      struct fd_bo_bucket *bucket = &cache->cache_bucket[i];
   if (bucket->size >= size) {
                        }
      static struct fd_bo *
   find_in_bucket(struct fd_bo_bucket *bucket, uint32_t flags)
   {
               /* TODO .. if we had an ALLOC_FOR_RENDER flag like intel, we could
   * skip the busy check.. if it is only going to be a render target
   * then we probably don't need to stall..
   *
   * NOTE that intel takes ALLOC_FOR_RENDER bo's from the list tail
   * (MRU, since likely to be in GPU cache), rather than head (LRU)..
   */
   foreach_bo (entry, &bucket->list) {
      if (fd_bo_state(entry) != FD_BO_STATE_IDLE) {
         }
   if (entry->alloc_flags == flags) {
      bo = entry;
   bo_remove_from_bucket(bucket, bo);
                     }
      /* NOTE: size is potentially rounded up to bucket size: */
   struct fd_bo *
   fd_bo_cache_alloc(struct fd_bo_cache *cache, uint32_t *size, uint32_t flags)
   {
      struct fd_bo *bo = NULL;
            *size = align(*size, 4096);
                                 retry:
      if (bucket) {
      *size = bucket->size;
   simple_mtx_lock(&cache->lock);
   bo = find_in_bucket(bucket, flags);
   simple_mtx_unlock(&cache->lock);
   if (bo) {
      VG_BO_OBTAIN(bo);
   if (bo->funcs->madvise(bo, true) <= 0) {
      /* we've lost the backing pages, delete and try again: */
   list_addtail(&bo->node, &freelist);
      }
   p_atomic_set(&bo->refcnt, 1);
   bo->reloc_flags = FD_RELOC_FLAGS_INIT;
   bucket->hits++;
      }
                        BO_CACHE_LOG(cache, "miss on size=%u, flags=0x%x, bucket=%u", *size, flags,
                     }
      int
   fd_bo_cache_free(struct fd_bo_cache *cache, struct fd_bo *bo)
   {
      if (bo->alloc_flags & (FD_BO_SHARED | _FD_BO_NOSYNC))
                     /* see if we can be green and recycle: */
   if (bucket) {
                                 bo->free_time = time.tv_sec;
            simple_mtx_lock(&cache->lock);
   list_addtail(&bo->node, &bucket->list);
   bucket->count++;
                                    }
