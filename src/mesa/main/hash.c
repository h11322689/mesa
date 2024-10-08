   /**
   * \file hash.c
   * Generic hash table. 
   *
   * Used for display lists, texture objects, vertex/fragment programs,
   * buffer objects, etc.  The hash functions are thread-safe.
   * 
   * \note key=0 is illegal.
   *
   * \author Brian Paul
   */
      /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "errors.h"
   #include "util/glheader.h"
   #include "hash.h"
   #include "util/hash_table.h"
   #include "util/u_memory.h"
   #include "util/u_idalloc.h"
         /**
   * Create a new hash table.
   * 
   * \return pointer to a new, empty hash table.
   */
   struct _mesa_HashTable *
   _mesa_NewHashTable(void)
   {
               if (table) {
      table->ht = _mesa_hash_table_create(NULL, uint_key_hash,
         if (table->ht == NULL) {
      free(table);
   _mesa_error_no_memory(__func__);
               _mesa_hash_table_set_deleted_key(table->ht, uint_key(DELETED_KEY_VALUE));
      }
   else {
                     }
            /**
   * Delete a hash table.
   * Frees each entry on the hash table and then the hash table structure itself.
   * Note that the caller should have already traversed the table and deleted
   * the objects in the table (i.e. We don't free the entries' data pointer).
   *
   * \param table the hash table to delete.
   */
   void
   _mesa_DeleteHashTable(struct _mesa_HashTable *table)
   {
               if (_mesa_hash_table_next_entry(table->ht, NULL) != NULL) {
                  _mesa_hash_table_destroy(table->ht, NULL);
   if (table->id_alloc) {
      util_idalloc_fini(table->id_alloc);
               simple_mtx_destroy(&table->Mutex);
      }
      static void init_name_reuse(struct _mesa_HashTable *table)
   {
      assert(_mesa_hash_table_num_entries(table->ht) == 0);
   table->id_alloc = MALLOC_STRUCT(util_idalloc);
   util_idalloc_init(table->id_alloc, 8);
   ASSERTED GLuint reserve0 = util_idalloc_alloc(table->id_alloc);
      }
      void
   _mesa_HashEnableNameReuse(struct _mesa_HashTable *table)
   {
      _mesa_HashLockMutex(table);
   init_name_reuse(table);
      }
      /**
   * Lookup an entry in the hash table, without locking.
   * \sa _mesa_HashLookup
   */
   static inline void *
   _mesa_HashLookup_unlocked(struct _mesa_HashTable *table, GLuint key)
   {
               assert(table);
            if (key == DELETED_KEY_VALUE)
            entry = _mesa_hash_table_search_pre_hashed(table->ht,
               if (!entry)
               }
         /**
   * Lookup an entry in the hash table.
   * 
   * \param table the hash table.
   * \param key the key.
   * 
   * \return pointer to user's data or NULL if key not in table
   */
   void *
   _mesa_HashLookup(struct _mesa_HashTable *table, GLuint key)
   {
      void *res;
   _mesa_HashLockMutex(table);
   res = _mesa_HashLookup_unlocked(table, key);
   _mesa_HashUnlockMutex(table);
      }
         /**
   * Lookup an entry in the hash table without locking the mutex.
   *
   * The hash table mutex must be locked manually by calling
   * _mesa_HashLockMutex() before calling this function.
   *
   * \param table the hash table.
   * \param key the key.
   *
   * \return pointer to user's data or NULL if key not in table
   */
   void *
   _mesa_HashLookupLocked(struct _mesa_HashTable *table, GLuint key)
   {
         }
         static inline void
   _mesa_HashInsert_unlocked(struct _mesa_HashTable *table, GLuint key, void *data)
   {
      uint32_t hash = uint_hash(key);
            assert(table);
            if (key > table->MaxKey)
            if (key == DELETED_KEY_VALUE) {
         } else {
      entry = _mesa_hash_table_search_pre_hashed(table->ht, hash, uint_key(key));
   if (entry) {
         } else {
               }
         /**
   * Insert a key/pointer pair into the hash table without locking the mutex.
   * If an entry with this key already exists we'll replace the existing entry.
   *
   * The hash table mutex must be locked manually by calling
   * _mesa_HashLockMutex() before calling this function.
   *
   * \param table the hash table.
   * \param key the key (not zero).
   * \param data pointer to user data.
   * \param isGenName true if the key has been generated by a HashFindFreeKey* function
   */
   void
   _mesa_HashInsertLocked(struct _mesa_HashTable *table, GLuint key, void *data,
         {
      _mesa_HashInsert_unlocked(table, key, data);
   if (!isGenName && table->id_alloc)
      }
         /**
   * Insert a key/pointer pair into the hash table.
   * If an entry with this key already exists we'll replace the existing entry.
   *
   * \param table the hash table.
   * \param key the key (not zero).
   * \param data pointer to user data.
   * \param isGenName true if the key has been generated by a HashFindFreeKey* function
   */
   void
   _mesa_HashInsert(struct _mesa_HashTable *table, GLuint key, void *data,
         {
      _mesa_HashLockMutex(table);
   _mesa_HashInsert_unlocked(table, key, data);
   if (!isGenName && table->id_alloc)
            }
         /**
   * Remove an entry from the hash table.
   * 
   * \param table the hash table.
   * \param key key of entry to remove.
   *
   * While holding the hash table's lock, searches the entry with the matching
   * key and unlinks it.
   */
   static inline void
   _mesa_HashRemove_unlocked(struct _mesa_HashTable *table, GLuint key)
   {
               assert(table);
            #ifndef NDEBUG
   /* assert if _mesa_HashRemove illegally called from _mesa_HashDeleteAll
   * callback function. Have to check this outside of mutex lock.
   */
   assert(!table->InDeleteAll);
            if (key == DELETED_KEY_VALUE) {
         } else {
      entry = _mesa_hash_table_search_pre_hashed(table->ht,
                           if (table->id_alloc)
      }
         void
   _mesa_HashRemoveLocked(struct _mesa_HashTable *table, GLuint key)
   {
         }
      void
   _mesa_HashRemove(struct _mesa_HashTable *table, GLuint key)
   {
      _mesa_HashLockMutex(table);
   _mesa_HashRemove_unlocked(table, key);
      }
      /**
   * Delete all entries in a hash table, but don't delete the table itself.
   * Invoke the given callback function for each table entry.
   *
   * \param table  the hash table to delete
   * \param callback  the callback function
   * \param userData  arbitrary pointer to pass along to the callback
   *                  (this is typically a struct gl_context pointer)
   */
   void
   _mesa_HashDeleteAll(struct _mesa_HashTable *table,
               {
      assert(callback);
   _mesa_HashLockMutex(table);
   #ifndef NDEBUG
   table->InDeleteAll = GL_TRUE;
   #endif
   hash_table_foreach(table->ht, entry) {
      callback(entry->data, userData);
      }
   if (table->deleted_key_data) {
      callback(table->deleted_key_data, userData);
      }
   if (table->id_alloc) {
      util_idalloc_fini(table->id_alloc);
   free(table->id_alloc);
      }
   #ifndef NDEBUG
   table->InDeleteAll = GL_FALSE;
   #endif
   table->MaxKey = 0;
      }
         /**
   * Walk over all entries in a hash table, calling callback function for each.
   * \param table  the hash table to walk
   * \param callback  the callback function
   * \param userData  arbitrary pointer to pass along to the callback
   *                  (this is typically a struct gl_context pointer)
   */
   static void
   hash_walk_unlocked(const struct _mesa_HashTable *table,
               {
      assert(table);
            hash_table_foreach(table->ht, entry) {
         }
   if (table->deleted_key_data)
      }
         void
   _mesa_HashWalk(const struct _mesa_HashTable *table,
               {
      /* cast-away const */
            _mesa_HashLockMutex(table2);
   hash_walk_unlocked(table, callback, userData);
      }
      void
   _mesa_HashWalkLocked(const struct _mesa_HashTable *table,
               {
         }
      /**
   * Dump contents of hash table for debugging.
   *
   * \param table the hash table.
   */
   void
   _mesa_HashPrint(const struct _mesa_HashTable *table)
   {
      if (table->deleted_key_data)
            hash_table_foreach(table->ht, entry) {
      _mesa_debug(NULL, "%u %p\n", (unsigned)(uintptr_t) entry->key,
         }
         /**
   * Find a block of adjacent unused hash keys.
   * 
   * \param table the hash table.
   * \param numKeys number of keys needed.
   * 
   * \return Starting key of free block or 0 if failure.
   *
   * If there are enough free keys between the maximum key existing in the table
   * (_mesa_HashTable::MaxKey) and the maximum key possible, then simply return
   * the adjacent key. Otherwise do a full search for a free key block in the
   * allowable key range.
   */
   GLuint
   _mesa_HashFindFreeKeyBlock(struct _mesa_HashTable *table, GLuint numKeys)
   {
      const GLuint maxKey = ~((GLuint) 0) - 1;
   if (table->id_alloc && numKeys == 1) {
         } else if (maxKey - numKeys > table->MaxKey) {
      /* the quick solution */
      }
   else {
      /* the slow solution */
   GLuint freeCount = 0;
   GLuint freeStart = 1;
   GLuint key;
   if (_mesa_HashLookup_unlocked(table, key)) {
      /* darn, this key is already in use */
   freeCount = 0;
      }
   else {
      /* this key not in use, check if we've found enough */
   freeCount++;
   if (freeCount == numKeys) {
            }
         }
   /* cannot allocate a block of numKeys consecutive keys */
         }
         bool
   _mesa_HashFindFreeKeys(struct _mesa_HashTable *table, GLuint* keys, GLuint numKeys)
   {
      if (!table->id_alloc) {
      GLuint first = _mesa_HashFindFreeKeyBlock(table, numKeys);
   for (int i = 0; i < numKeys; i++) {
         }
               for (int i = 0; i < numKeys; i++) {
                     }
         /**
   * Return the number of entries in the hash table.
   */
   GLuint
   _mesa_HashNumEntries(const struct _mesa_HashTable *table)
   {
               if (table->deleted_key_data)
                        }
