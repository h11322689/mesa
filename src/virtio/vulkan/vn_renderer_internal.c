   /*
   * Copyright 2021 Google LLC
   * SPDX-License-Identifier: MIT
   */
      #include "vn_renderer_internal.h"
      /* 3 seconds */
   #define VN_RENDERER_SHMEM_CACHE_EXPIRACY (3ll * 1000 * 1000)
      void
   vn_renderer_shmem_cache_init(struct vn_renderer_shmem_cache *cache,
               {
      /* cache->bucket_mask is 32-bit and u_bit_scan is used */
            cache->renderer = renderer;
                     for (uint32_t i = 0; i < ARRAY_SIZE(cache->buckets); i++) {
      struct vn_renderer_shmem_bucket *bucket = &cache->buckets[i];
                  }
      void
   vn_renderer_shmem_cache_fini(struct vn_renderer_shmem_cache *cache)
   {
      if (!cache->initialized)
            while (cache->bucket_mask) {
      const int idx = u_bit_scan(&cache->bucket_mask);
            list_for_each_entry_safe(struct vn_renderer_shmem, shmem,
                        }
      static struct vn_renderer_shmem_bucket *
   choose_bucket(struct vn_renderer_shmem_cache *cache,
               {
      assert(size);
   if (unlikely(!util_is_power_of_two_or_zero64(size)))
            const uint32_t idx = ffsll(size) - 1;
   if (unlikely(idx >= ARRAY_SIZE(cache->buckets)))
            *out_idx = idx;
      }
      static void
   vn_renderer_shmem_cache_remove_expired_locked(
         {
      uint32_t bucket_mask = cache->bucket_mask;
   while (bucket_mask) {
      const int idx = u_bit_scan(&bucket_mask);
            assert(!list_is_empty(&bucket->shmems));
   const struct vn_renderer_shmem *last_shmem = list_last_entry(
            /* remove expired shmems but keep at least the last one */
   list_for_each_entry_safe(struct vn_renderer_shmem, shmem,
            if (shmem == last_shmem ||
                  list_del(&shmem->cache_head);
            }
      bool
   vn_renderer_shmem_cache_add(struct vn_renderer_shmem_cache *cache,
         {
               int idx;
   struct vn_renderer_shmem_bucket *bucket =
         if (!bucket)
            const int64_t now = os_time_get();
                              list_addtail(&shmem->cache_head, &bucket->shmems);
                        }
      struct vn_renderer_shmem *
   vn_renderer_shmem_cache_get(struct vn_renderer_shmem_cache *cache,
         {
      int idx;
   struct vn_renderer_shmem_bucket *bucket = choose_bucket(cache, size, &idx);
   if (!bucket) {
      VN_TRACE_SCOPE("shmem cache skip");
   simple_mtx_lock(&cache->mutex);
   cache->debug.cache_skip_count++;
   simple_mtx_unlock(&cache->mutex);
                        simple_mtx_lock(&cache->mutex);
   if (cache->bucket_mask & (1 << idx)) {
      assert(!list_is_empty(&bucket->shmems));
   shmem = list_first_entry(&bucket->shmems, struct vn_renderer_shmem,
                  if (list_is_empty(&bucket->shmems))
               } else {
      VN_TRACE_SCOPE("shmem cache miss");
      }
               }
      /* for debugging only */
   void
   vn_renderer_shmem_cache_debug_dump(struct vn_renderer_shmem_cache *cache)
   {
               vn_log(NULL, "dumping shmem cache");
   vn_log(NULL, "  cache skip: %d", cache->debug.cache_skip_count);
   vn_log(NULL, "  cache hit: %d", cache->debug.cache_hit_count);
            uint32_t bucket_mask = cache->bucket_mask;
   while (bucket_mask) {
      const int idx = u_bit_scan(&bucket_mask);
   const struct vn_renderer_shmem_bucket *bucket = &cache->buckets[idx];
   uint32_t count = 0;
   list_for_each_entry(struct vn_renderer_shmem, shmem, &bucket->shmems,
               if (count)
                  }
