   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * @file
   * Improved cache implementation.
   *
   * Fixed size array with linear probing on collision and LRU eviction
   * on full.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "util/compiler.h"
   #include "util/u_debug.h"
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_cache.h"
   #include "util/list.h"
      struct util_cache_entry
   {
      enum { EMPTY = 0, FILLED, DELETED } state;
                     void *key;
         #ifdef DEBUG
         #endif
   };
         struct util_cache
   {
      /** Hash function */
            /** Compare two keys */
            /** Destroy a (key, value) pair */
            /** Max entries in the cache */
            /** Array [size] of entries */
            /** Number of entries in the cache */
            /** Head of list, sorted from Least-recently used to Most-recently used */
      };
         static void
   ensure_sanity(const struct util_cache *cache);
      #define CACHE_DEFAULT_ALPHA 2
      /**
   * Create a new cache with 'size' entries.  Also provide functions for
   * hashing keys, comparing keys and destroying (key,value) pairs.
   */
   struct util_cache *
   util_cache_create(uint32_t (*hash)(const void *key),
                     {
               cache = CALLOC_STRUCT(util_cache);
   if (!cache)
            cache->hash = hash;
   cache->compare = compare;
                     size *= CACHE_DEFAULT_ALPHA;
            cache->entries = CALLOC(size, sizeof(struct util_cache_entry));
   if (!cache->entries) {
      FREE(cache);
               ensure_sanity(cache);
      }
         /**
   * Try to find a cache entry, given the key and hash of the key.
   */
   static struct util_cache_entry *
   util_cache_entry_get(struct util_cache *cache,
               {
      struct util_cache_entry *first_unfilled = NULL;
   uint32_t index = hash % cache->size;
            /* Probe until we find either a matching FILLED entry or an EMPTY
   * slot (which has never been occupied).
   *
   * Deleted or non-matching slots are not indicative of completion
   * as a previous linear probe for the same key could have continued
   * past this point.
   */
   for (probe = 0; probe < cache->size; probe++) {
      uint32_t i = (index + probe) % cache->size;
            if (current->state == FILLED) {
      if (current->hash == hash &&
      cache->compare(key, current->key) == 0)
   }
   else {
                     if (current->state == EMPTY)
                     }
      static inline void
   util_cache_entry_destroy(struct util_cache *cache,
         {
      void *key = entry->key;
            entry->key = NULL;
            if (entry->state == FILLED) {
      list_del(&entry->list);
            if (cache->destroy)
                  }
         /**
   * Insert an entry in the cache, given the key and value.
   */
   void
   util_cache_set(struct util_cache *cache,
               {
      struct util_cache_entry *entry;
            assert(cache);
   if (!cache)
         hash = cache->hash(key);
   entry = util_cache_entry_get(cache, hash, key);
   if (!entry)
            if (cache->count >= cache->size / CACHE_DEFAULT_ALPHA)
                  #ifdef DEBUG
         #endif
         entry->key = key;
   entry->hash = hash;
   entry->value = value;
   entry->state = FILLED;
   list_add(&cache->lru.list, &entry->list);
               }
         /**
   * Search the cache for an entry with the given key.  Return the corresponding
   * value or NULL if not found.
   */
   void *
   util_cache_get(struct util_cache *cache,
         {
      struct util_cache_entry *entry;
            assert(cache);
   if (!cache)
         hash = cache->hash(key);
   entry = util_cache_entry_get(cache, hash, key);
   if (!entry)
            if (entry->state == FILLED) {
                     }
         /**
   * Remove all entries from the cache.  The 'destroy' function will be called
   * for each entry's (key, value).
   */
   void
   util_cache_clear(struct util_cache *cache)
   {
               assert(cache);
   if (!cache)
            for (i = 0; i < cache->size; ++i) {
      util_cache_entry_destroy(cache, &cache->entries[i]);
               assert(cache->count == 0);
   assert(list_is_empty(&cache->lru.list));
      }
         /**
   * Destroy the cache and all entries.
   */
   void
   util_cache_destroy(struct util_cache *cache)
   {
      assert(cache);
   if (!cache)
         #ifdef DEBUG
      if (cache->count >= 20*cache->size) {
      /* Normal approximation of the Poisson distribution */
   double mean = (double)cache->count/(double)cache->size;
   double stddev = sqrt(mean);
   unsigned i;
   for (i = 0; i < cache->size; ++i) {
      double z = fabs(cache->entries[i].count - mean)/stddev;
   /* This assert should not fail 99.9999998027% of the times, unless
   * the hash function is a poor one */
            #endif
                  FREE(cache->entries);
      }
         /**
   * Remove the cache entry which matches the given key.
   */
   void
   util_cache_remove(struct util_cache *cache,
         {
      struct util_cache_entry *entry;
            assert(cache);
   if (!cache)
                     entry = util_cache_entry_get(cache, hash, key);
   if (!entry)
            if (entry->state == FILLED)
               }
         static void
   ensure_sanity(const struct util_cache *cache)
   {
   #ifdef DEBUG
               assert(cache);
   for (i = 0; i < cache->size; i++) {
               assert(header);
   assert(header->state == FILLED ||
         header->state == EMPTY ||
   if (header->state == FILLED) {
      cnt++;
                  assert(cnt == cache->count);
            if (cache->count == 0) {
         }
   else {
      struct util_cache_entry *header =
            assert (header);
            for (i = 0; i < cache->count; i++)
                  #endif
            }
