   /*
   * Copyright © 2009,2012 Intel Corporation
   * Copyright © 1988-2004 Keith Packard and Bart Massey.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   *
   * Except as contained in this notice, the names of the authors
   * or their institutions shall not be used in advertising or
   * otherwise to promote the sale, use or other dealings in this
   * Software without prior written authorization from the
   * authors.
   *
   * Authors:
   *    Eric Anholt <eric@anholt.net>
   *    Keith Packard <keithp@keithp.com>
   */
      /**
   * Implements an open-addressing, linear-reprobing hash table.
   *
   * For more information, see:
   *
   * http://cgit.freedesktop.org/~anholt/hash_table/tree/README
   */
      #include <stdlib.h>
   #include <string.h>
   #include <assert.h>
      #include "hash_table.h"
   #include "ralloc.h"
   #include "macros.h"
   #include "u_memory.h"
   #include "fast_urem_by_const.h"
   #include "util/u_memory.h"
      #define XXH_INLINE_ALL
   #include "xxhash.h"
      /**
   * Magic number that gets stored outside of the struct hash_table.
   *
   * The hash table needs a particular pointer to be the marker for a key that
   * was deleted from the table, along with NULL for the "never allocated in the
   * table" marker.  Legacy GL allows any GLuint to be used as a GL object name,
   * and we use a 1:1 mapping from GLuints to key pointers, so we need to be
   * able to track a GLuint that happens to match the deleted key outside of
   * struct hash_table.  We tell the hash table to use "1" as the deleted key
   * value, so that we test the deleted-key-in-the-table path as best we can.
   */
   #define DELETED_KEY_VALUE 1
      static inline void *
   uint_key(unsigned id)
   {
         }
      static const uint32_t deleted_key_value;
      /**
   * From Knuth -- a good choice for hash/rehash values is p, p-2 where
   * p and p-2 are both prime.  These tables are sized to have an extra 10%
   * free to avoid exponential performance degradation as the hash table fills
   */
   static const struct {
      uint32_t max_entries, size, rehash;
      } hash_sizes[] = {
   #define ENTRY(max_entries, size, rehash) \
      { max_entries, size, rehash, \
            ENTRY(2,            5,            3            ),
   ENTRY(4,            7,            5            ),
   ENTRY(8,            13,           11           ),
   ENTRY(16,           19,           17           ),
   ENTRY(32,           43,           41           ),
   ENTRY(64,           73,           71           ),
   ENTRY(128,          151,          149          ),
   ENTRY(256,          283,          281          ),
   ENTRY(512,          571,          569          ),
   ENTRY(1024,         1153,         1151         ),
   ENTRY(2048,         2269,         2267         ),
   ENTRY(4096,         4519,         4517         ),
   ENTRY(8192,         9013,         9011         ),
   ENTRY(16384,        18043,        18041        ),
   ENTRY(32768,        36109,        36107        ),
   ENTRY(65536,        72091,        72089        ),
   ENTRY(131072,       144409,       144407       ),
   ENTRY(262144,       288361,       288359       ),
   ENTRY(524288,       576883,       576881       ),
   ENTRY(1048576,      1153459,      1153457      ),
   ENTRY(2097152,      2307163,      2307161      ),
   ENTRY(4194304,      4613893,      4613891      ),
   ENTRY(8388608,      9227641,      9227639      ),
   ENTRY(16777216,     18455029,     18455027     ),
   ENTRY(33554432,     36911011,     36911009     ),
   ENTRY(67108864,     73819861,     73819859     ),
   ENTRY(134217728,    147639589,    147639587    ),
   ENTRY(268435456,    295279081,    295279079    ),
   ENTRY(536870912,    590559793,    590559791    ),
   ENTRY(1073741824,   1181116273,   1181116271   ),
      };
      ASSERTED static inline bool
   key_pointer_is_reserved(const struct hash_table *ht, const void *key)
   {
         }
      static int
   entry_is_free(const struct hash_entry *entry)
   {
         }
      static int
   entry_is_deleted(const struct hash_table *ht, struct hash_entry *entry)
   {
         }
      static int
   entry_is_present(const struct hash_table *ht, struct hash_entry *entry)
   {
         }
      bool
   _mesa_hash_table_init(struct hash_table *ht,
                           {
      ht->size_index = 0;
   ht->size = hash_sizes[ht->size_index].size;
   ht->rehash = hash_sizes[ht->size_index].rehash;
   ht->size_magic = hash_sizes[ht->size_index].size_magic;
   ht->rehash_magic = hash_sizes[ht->size_index].rehash_magic;
   ht->max_entries = hash_sizes[ht->size_index].max_entries;
   ht->key_hash_function = key_hash_function;
   ht->key_equals_function = key_equals_function;
   ht->table = rzalloc_array(mem_ctx, struct hash_entry, ht->size);
   ht->entries = 0;
   ht->deleted_entries = 0;
               }
      struct hash_table *
   _mesa_hash_table_create(void *mem_ctx,
                     {
               /* mem_ctx is used to allocate the hash table, but the hash table is used
   * to allocate all of the suballocations.
   */
   ht = ralloc(mem_ctx, struct hash_table);
   if (ht == NULL)
            if (!_mesa_hash_table_init(ht, ht, key_hash_function, key_equals_function)) {
      ralloc_free(ht);
                  }
      static uint32_t
   key_u32_hash(const void *key)
   {
      uint32_t u = (uint32_t)(uintptr_t)key;
      }
      static bool
   key_u32_equals(const void *a, const void *b)
   {
         }
      /* key == 0 and key == deleted_key are not allowed */
   struct hash_table *
   _mesa_hash_table_create_u32_keys(void *mem_ctx)
   {
         }
      struct hash_table *
   _mesa_hash_table_clone(struct hash_table *src, void *dst_mem_ctx)
   {
               ht = ralloc(dst_mem_ctx, struct hash_table);
   if (ht == NULL)
                     ht->table = ralloc_array(ht, struct hash_entry, ht->size);
   if (ht->table == NULL) {
      ralloc_free(ht);
                           }
      /**
   * Frees the given hash table.
   *
   * If delete_function is passed, it gets called on each entry present before
   * freeing.
   */
   void
   _mesa_hash_table_destroy(struct hash_table *ht,
         {
      if (!ht)
            if (delete_function) {
      hash_table_foreach(ht, entry) {
            }
      }
      static void
   hash_table_clear_fast(struct hash_table *ht)
   {
      memset(ht->table, 0, sizeof(struct hash_entry) * hash_sizes[ht->size_index].size);
      }
      /**
   * Deletes all entries of the given hash table without deleting the table
   * itself or changing its structure.
   *
   * If delete_function is passed, it gets called on each entry present.
   */
   void
   _mesa_hash_table_clear(struct hash_table *ht,
         {
      if (!ht)
                     if (delete_function) {
      for (entry = ht->table; entry != ht->table + ht->size; entry++) {
                        }
   ht->entries = 0;
      } else
      }
      /** Sets the value of the key pointer used for deleted entries in the table.
   *
   * The assumption is that usually keys are actual pointers, so we use a
   * default value of a pointer to an arbitrary piece of storage in the library.
   * But in some cases a consumer wants to store some other sort of value in the
   * table, like a uint32_t, in which case that pointer may conflict with one of
   * their valid keys.  This lets that user select a safe value.
   *
   * This must be called before any keys are actually deleted from the table.
   */
   void
   _mesa_hash_table_set_deleted_key(struct hash_table *ht, const void *deleted_key)
   {
         }
      static struct hash_entry *
   hash_table_search(struct hash_table *ht, uint32_t hash, const void *key)
   {
               uint32_t size = ht->size;
   uint32_t start_hash_address = util_fast_urem32(hash, size, ht->size_magic);
   uint32_t double_hash = 1 + util_fast_urem32(hash, ht->rehash,
                  do {
               if (entry_is_free(entry)) {
         } else if (entry_is_present(ht, entry) && entry->hash == hash) {
      if (ht->key_equals_function(key, entry->key)) {
                     hash_address += double_hash;
   if (hash_address >= size)
                  }
      /**
   * Finds a hash table entry with the given key and hash of that key.
   *
   * Returns NULL if no entry is found.  Note that the data pointer may be
   * modified by the user.
   */
   struct hash_entry *
   _mesa_hash_table_search(struct hash_table *ht, const void *key)
   {
      assert(ht->key_hash_function);
      }
      struct hash_entry *
   _mesa_hash_table_search_pre_hashed(struct hash_table *ht, uint32_t hash,
         {
      assert(ht->key_hash_function == NULL || hash == ht->key_hash_function(key));
      }
      static struct hash_entry *
   hash_table_insert(struct hash_table *ht, uint32_t hash,
            static void
   hash_table_insert_rehash(struct hash_table *ht, uint32_t hash,
         {
      uint32_t size = ht->size;
   uint32_t start_hash_address = util_fast_urem32(hash, size, ht->size_magic);
   uint32_t double_hash = 1 + util_fast_urem32(hash, ht->rehash,
         uint32_t hash_address = start_hash_address;
   do {
               if (likely(entry->key == NULL)) {
      entry->hash = hash;
   entry->key = key;
   entry->data = data;
               hash_address += double_hash;
   if (hash_address >= size)
         }
      static void
   _mesa_hash_table_rehash(struct hash_table *ht, unsigned new_size_index)
   {
      struct hash_table old_ht;
            if (ht->size_index == new_size_index && ht->deleted_entries == ht->max_entries) {
      hash_table_clear_fast(ht);
   assert(!ht->entries);
               if (new_size_index >= ARRAY_SIZE(hash_sizes))
            table = rzalloc_array(ralloc_parent(ht->table), struct hash_entry,
         if (table == NULL)
                     ht->table = table;
   ht->size_index = new_size_index;
   ht->size = hash_sizes[ht->size_index].size;
   ht->rehash = hash_sizes[ht->size_index].rehash;
   ht->size_magic = hash_sizes[ht->size_index].size_magic;
   ht->rehash_magic = hash_sizes[ht->size_index].rehash_magic;
   ht->max_entries = hash_sizes[ht->size_index].max_entries;
   ht->entries = 0;
            hash_table_foreach(&old_ht, entry) {
                              }
      static struct hash_entry *
   hash_table_get_entry(struct hash_table *ht, uint32_t hash, const void *key)
   {
                        if (ht->entries >= ht->max_entries) {
         } else if (ht->deleted_entries + ht->entries >= ht->max_entries) {
                  uint32_t size = ht->size;
   uint32_t start_hash_address = util_fast_urem32(hash, size, ht->size_magic);
   uint32_t double_hash = 1 + util_fast_urem32(hash, ht->rehash,
         uint32_t hash_address = start_hash_address;
   do {
               if (!entry_is_present(ht, entry)) {
      /* Stash the first available entry we find */
   if (available_entry == NULL)
         if (entry_is_free(entry))
               /* Implement replacement when another insert happens
   * with a matching key.  This is a relatively common
   * feature of hash tables, with the alternative
   * generally being "insert the new value as well, and
   * return it first when the key is searched for".
   *
   * Note that the hash table doesn't have a delete
   * callback.  If freeing of old data pointers is
   * required to avoid memory leaks, perform a search
   * before inserting.
   */
   if (!entry_is_deleted(ht, entry) &&
      entry->hash == hash &&
               hash_address += double_hash;
   if (hash_address >= size)
               if (available_entry) {
      if (entry_is_deleted(ht, available_entry))
         available_entry->hash = hash;
   ht->entries++;
               /* We could hit here if a required resize failed. An unchecked-malloc
   * application could ignore this result.
   */
      }
      static struct hash_entry *
   hash_table_insert(struct hash_table *ht, uint32_t hash,
         {
               if (entry) {
      entry->key = key;
                  }
      /**
   * Inserts the key with the given hash into the table.
   *
   * Note that insertion may rearrange the table on a resize or rehash,
   * so previously found hash_entries are no longer valid after this function.
   */
   struct hash_entry *
   _mesa_hash_table_insert(struct hash_table *ht, const void *key, void *data)
   {
      assert(ht->key_hash_function);
      }
      struct hash_entry *
   _mesa_hash_table_insert_pre_hashed(struct hash_table *ht, uint32_t hash,
         {
      assert(ht->key_hash_function == NULL || hash == ht->key_hash_function(key));
      }
      /**
   * This function deletes the given hash table entry.
   *
   * Note that deletion doesn't otherwise modify the table, so an iteration over
   * the table deleting entries is safe.
   */
   void
   _mesa_hash_table_remove(struct hash_table *ht,
         {
      if (!entry)
            entry->key = ht->deleted_key;
   ht->entries--;
      }
      /**
   * Removes the entry with the corresponding key, if exists.
   */
   void _mesa_hash_table_remove_key(struct hash_table *ht,
         {
         }
      /**
   * This function is an iterator over the hash_table when no deleted entries are present.
   *
   * Pass in NULL for the first entry, as in the start of a for loop.
   */
   struct hash_entry *
   _mesa_hash_table_next_entry_unsafe(const struct hash_table *ht, struct hash_entry *entry)
   {
      assert(!ht->deleted_entries);
   if (!ht->entries)
         if (entry == NULL)
         else
         if (entry != ht->table + ht->size)
               }
      /**
   * This function is an iterator over the hash table.
   *
   * Pass in NULL for the first entry, as in the start of a for loop.  Note that
   * an iteration over the table is O(table_size) not O(entries).
   */
   struct hash_entry *
   _mesa_hash_table_next_entry(struct hash_table *ht,
         {
      if (entry == NULL)
         else
            for (; entry != ht->table + ht->size; entry++) {
      if (entry_is_present(ht, entry)) {
                        }
      /**
   * Returns a random entry from the hash table.
   *
   * This may be useful in implementing random replacement (as opposed
   * to just removing everything) in caches based on this hash table
   * implementation.  @predicate may be used to filter entries, or may
   * be set to NULL for no filtering.
   */
   struct hash_entry *
   _mesa_hash_table_random_entry(struct hash_table *ht,
         {
      struct hash_entry *entry;
            if (ht->entries == 0)
            for (entry = ht->table + i; entry != ht->table + ht->size; entry++) {
      if (entry_is_present(ht, entry) &&
      (!predicate || predicate(entry))) {
                  for (entry = ht->table; entry != ht->table + i; entry++) {
      if (entry_is_present(ht, entry) &&
      (!predicate || predicate(entry))) {
                     }
         uint32_t
   _mesa_hash_data(const void *data, size_t size)
   {
         }
      uint32_t
   _mesa_hash_data_with_seed(const void *data, size_t size, uint32_t seed)
   {
         }
      uint32_t
   _mesa_hash_int(const void *key)
   {
         }
      uint32_t
   _mesa_hash_uint(const void *key)
   {
         }
      uint32_t
   _mesa_hash_u32(const void *key)
   {
         }
      /** FNV-1a string hash implementation */
   uint32_t
   _mesa_hash_string(const void *_key)
   {
         }
      uint32_t
   _mesa_hash_string_with_length(const void *_key, unsigned length)
   {
      uint32_t hash = 0;
      #if defined(_WIN64) || defined(__x86_64__)
         #else
         #endif
         }
      uint32_t
   _mesa_hash_pointer(const void *pointer)
   {
      uintptr_t num = (uintptr_t) pointer;
      }
      bool
   _mesa_key_int_equal(const void *a, const void *b)
   {
         }
      bool
   _mesa_key_uint_equal(const void *a, const void *b)
   {
            }
      bool
   _mesa_key_u32_equal(const void *a, const void *b)
   {
         }
      /**
   * String compare function for use as the comparison callback in
   * _mesa_hash_table_create().
   */
   bool
   _mesa_key_string_equal(const void *a, const void *b)
   {
         }
      bool
   _mesa_key_pointer_equal(const void *a, const void *b)
   {
         }
      /**
   * Helper to create a hash table with pointer keys.
   */
   struct hash_table *
   _mesa_pointer_hash_table_create(void *mem_ctx)
   {
      return _mesa_hash_table_create(mem_ctx, _mesa_hash_pointer,
      }
         bool
   _mesa_hash_table_reserve(struct hash_table *ht, unsigned size)
   {
      if (size < ht->max_entries)
         for (unsigned i = ht->size_index + 1; i < ARRAY_SIZE(hash_sizes); i++) {
      if (hash_sizes[i].max_entries >= size) {
      _mesa_hash_table_rehash(ht, i);
         }
      }
      /**
   * Hash table wrapper which supports 64-bit keys.
   *
   * TODO: unify all hash table implementations.
   */
      struct hash_key_u64 {
         };
      static uint32_t
   key_u64_hash(const void *key)
   {
         }
      static bool
   key_u64_equals(const void *a, const void *b)
   {
      const struct hash_key_u64 *aa = a;
               }
      #define FREED_KEY_VALUE 0
      static void _mesa_hash_table_u64_delete_keys(void *data)
   {
                  }
      struct hash_table_u64 *
   _mesa_hash_table_u64_create(void *mem_ctx)
   {
      STATIC_ASSERT(FREED_KEY_VALUE != DELETED_KEY_VALUE);
            ht = rzalloc(mem_ctx, struct hash_table_u64);
   if (!ht)
            if (sizeof(void *) == 8) {
      ht->table = _mesa_hash_table_create(ht, _mesa_hash_pointer,
      } else {
      ht->table = _mesa_hash_table_create(ht, key_u64_hash,
            /* Allocate a ralloc sub-context which takes the u64 hash table
   * as a parent and attach a destructor to it so we can free the
   * hash_key_u64 objects that were allocated by
   * _mesa_hash_table_u64_insert().
   *
   * The order of creation of this sub-context is crucial: it needs
   * to happen after the _mesa_hash_table_create() call to guarantee
   * that the destructor is called before ht->table and its children
   * are freed, otherwise the _mesa_hash_table_u64_clear() call in the
   * destructor leads to a use-after-free situation.
   */
   if (ht->table) {
               /* If we can't allocate a sub-context, free the hash table
   * immediately and return NULL to avoid future leaks.
   */
   if (!dummy_ctx) {
      ralloc_free(ht);
                              if (ht->table)
               }
      static void
   _mesa_hash_table_u64_delete_key(struct hash_entry *entry)
   {
      if (sizeof(void *) == 8)
                     if (_key)
      }
      void
   _mesa_hash_table_u64_clear(struct hash_table_u64 *ht)
   {
      if (!ht)
            _mesa_hash_table_clear(ht->table, _mesa_hash_table_u64_delete_key);
   ht->freed_key_data = NULL;
      }
      void
   _mesa_hash_table_u64_destroy(struct hash_table_u64 *ht)
   {
      if (!ht)
            }
      void
   _mesa_hash_table_u64_insert(struct hash_table_u64 *ht, uint64_t key,
         {
      if (key == FREED_KEY_VALUE) {
      ht->freed_key_data = data;
               if (key == DELETED_KEY_VALUE) {
      ht->deleted_key_data = data;
               if (sizeof(void *) == 8) {
         } else {
               if (!_key)
                  struct hash_entry *entry =
            if (!entry) {
      FREE(_key);
               entry->data = data;
   if (!entry_is_present(ht->table, entry))
         else
         }
      static struct hash_entry *
   hash_table_u64_search(struct hash_table_u64 *ht, uint64_t key)
   {
      if (sizeof(void *) == 8) {
         } else {
      struct hash_key_u64 _key = { .value = key };
         }
      void *
   _mesa_hash_table_u64_search(struct hash_table_u64 *ht, uint64_t key)
   {
               if (key == FREED_KEY_VALUE)
            if (key == DELETED_KEY_VALUE)
            entry = hash_table_u64_search(ht, key);
   if (!entry)
               }
      void
   _mesa_hash_table_u64_remove(struct hash_table_u64 *ht, uint64_t key)
   {
               if (key == FREED_KEY_VALUE) {
      ht->freed_key_data = NULL;
               if (key == DELETED_KEY_VALUE) {
      ht->deleted_key_data = NULL;
               entry = hash_table_u64_search(ht, key);
   if (!entry)
            if (sizeof(void *) == 8) {
         } else {
               _mesa_hash_table_remove(ht->table, entry);
         }
