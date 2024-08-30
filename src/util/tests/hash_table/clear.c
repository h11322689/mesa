   /*
   * Copyright (C) 2016 Advanced Micro Devices, Inc.
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #undef NDEBUG
      #include "util/hash_table.h"
      #define SIZE 1000
      static void *make_key(uint32_t i)
   {
         }
      static uint32_t key_id(const void *key)
   {
         }
      static uint32_t key_hash(const void *key)
   {
         }
      static bool key_equal(const void *a, const void *b)
   {
         }
      static void delete_function(struct hash_entry *entry)
   {
      bool *deleted = (bool *)entry->data;
   assert(!*deleted);
      }
      int main()
   {
      struct hash_table *ht;
   bool flags[SIZE];
                     for (i = 0; i < SIZE; ++i) {
      flags[i] = false;
               _mesa_hash_table_clear(ht, delete_function);
            /* Check that delete_function was called and that repopulating the table
   * works. */
   for (i = 0; i < SIZE; ++i) {
      assert(flags[i]);
   flags[i] = false;
               /* Check that exactly the right set of entries is in the table. */
   for (i = 0; i < SIZE; ++i) {
                  hash_table_foreach(ht, entry) {
         }
   _mesa_hash_table_clear(ht, NULL);
   assert(!ht->entries);
   assert(!ht->deleted_entries);
   hash_table_foreach(ht, entry) {
                  for (i = 0; i < SIZE; ++i) {
      flags[i] = false;
      }
   hash_table_foreach_remove(ht, entry) {
         }
   assert(!ht->entries);
                        }
