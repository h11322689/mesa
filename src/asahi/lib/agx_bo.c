   /*
   * Copyright 2021 Alyssa Rosenzweig
   * Copyright 2019 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include "agx_bo.h"
   #include <inttypes.h>
   #include "agx_device.h"
   #include "decode.h"
      /* Helper to calculate the bucket index of a BO */
   static unsigned
   agx_bucket_index(unsigned size)
   {
      /* Round down to POT to compute a bucket index */
            /* Clamp to supported buckets. Huge allocations use the largest bucket */
            /* Reindex from 0 */
      }
      static struct list_head *
   agx_bucket(struct agx_device *dev, unsigned size)
   {
         }
      static bool
   agx_bo_wait(struct agx_bo *bo, int64_t timeout_ns)
   {
      /* TODO: When we allow parallelism we'll need to implement this for real */
      }
      static void
   agx_bo_cache_remove_locked(struct agx_device *dev, struct agx_bo *bo)
   {
      simple_mtx_assert_locked(&dev->bo_cache.lock);
   list_del(&bo->bucket_link);
   list_del(&bo->lru_link);
      }
      /* Tries to fetch a BO of sufficient size with the appropriate flags from the
   * BO cache. If it succeeds, it returns that BO and removes the BO from the
   * cache. If it fails, it returns NULL signaling the caller to allocate a new
   * BO. */
      struct agx_bo *
   agx_bo_cache_fetch(struct agx_device *dev, size_t size, uint32_t flags,
         {
      simple_mtx_lock(&dev->bo_cache.lock);
   struct list_head *bucket = agx_bucket(dev, size);
            /* Iterate the bucket looking for something suitable */
   list_for_each_entry_safe(struct agx_bo, entry, bucket, bucket_link) {
      if (entry->size < size || entry->flags != flags)
            /* Do not return more than 2x oversized BOs. */
   if (entry->size > 2 * size)
            /* If the oldest BO in the cache is busy, likely so is
   * everything newer, so bail. */
   if (!agx_bo_wait(entry, dontwait ? 0 : INT64_MAX))
            /* This one works, use it */
   agx_bo_cache_remove_locked(dev, entry);
   bo = entry;
      }
               }
      static void
   agx_bo_cache_evict_stale_bos(struct agx_device *dev, unsigned tv_sec)
   {
               clock_gettime(CLOCK_MONOTONIC, &time);
   list_for_each_entry_safe(struct agx_bo, entry, &dev->bo_cache.lru,
            /* We want all entries that have been used more than 1 sec ago to be
   * dropped, others can be kept.  Note the <= 2 check and not <= 1. It's
   * here to account for the fact that we're only testing ->tv_sec, not
   * ->tv_nsec.  That means we might keep entries that are between 1 and 2
   * seconds old, but we don't really care, as long as unused BOs are
   * dropped at some point.
   */
   if (time.tv_sec - entry->last_used <= 2)
            agx_bo_cache_remove_locked(dev, entry);
         }
      static void
   agx_bo_cache_put_locked(struct agx_bo *bo)
   {
      struct agx_device *dev = bo->dev;
   struct list_head *bucket = agx_bucket(dev, bo->size);
            /* Add us to the bucket */
            /* Add us to the LRU list and update the last_used field. */
   list_addtail(&bo->lru_link, &dev->bo_cache.lru);
   clock_gettime(CLOCK_MONOTONIC, &time);
            /* Update statistics */
            if (0) {
      printf("BO cache: %zu KiB (+%zu KiB from %s, hit/miss %" PRIu64
         "/%" PRIu64 ")\n",
   DIV_ROUND_UP(dev->bo_cache.size, 1024),
   DIV_ROUND_UP(bo->size, 1024), bo->label,
               /* Update label for debug */
            /* Let's do some cleanup in the BO cache while we hold the lock. */
      }
      /* Tries to add a BO to the cache. Returns if it was successful */
   static bool
   agx_bo_cache_put(struct agx_bo *bo)
   {
               if (bo->flags & AGX_BO_SHARED) {
         } else {
      simple_mtx_lock(&dev->bo_cache.lock);
   agx_bo_cache_put_locked(bo);
                  }
      void
   agx_bo_cache_evict_all(struct agx_device *dev)
   {
      simple_mtx_lock(&dev->bo_cache.lock);
   for (unsigned i = 0; i < ARRAY_SIZE(dev->bo_cache.buckets); ++i) {
               list_for_each_entry_safe(struct agx_bo, entry, bucket, bucket_link) {
      agx_bo_cache_remove_locked(dev, entry);
         }
      }
      void
   agx_bo_reference(struct agx_bo *bo)
   {
      if (bo) {
      ASSERTED int count = p_atomic_inc_return(&bo->refcnt);
         }
      void
   agx_bo_unreference(struct agx_bo *bo)
   {
      if (!bo)
            /* Don't return to cache if there are still references */
   if (p_atomic_dec_return(&bo->refcnt))
                              /* Someone might have imported this BO while we were waiting for the
   * lock, let's make sure it's still not referenced before freeing it.
   */
   if (p_atomic_read(&bo->refcnt) == 0) {
               if (dev->debug & AGX_DBG_TRACE)
            if (!agx_bo_cache_put(bo))
                  }
      struct agx_bo *
   agx_bo_create(struct agx_device *dev, unsigned size, enum agx_bo_flags flags,
         {
      struct agx_bo *bo;
            /* To maximize BO cache usage, don't allocate tiny BOs */
            /* See if we have a BO already in the cache */
            /* Update stats based on the first attempt to fetch */
   if (bo != NULL)
         else
            /* Otherwise, allocate a fresh BO. If allocation fails, we can try waiting
   * for something in the cache. But if there's no nothing suitable, we should
   * flush the cache to make space for the new allocation.
   */
   if (!bo)
         if (!bo)
         if (!bo) {
      agx_bo_cache_evict_all(dev);
               if (!bo) {
      fprintf(stderr, "BO creation failed\n");
               bo->label = label;
            if (dev->debug & AGX_DBG_TRACE)
               }
