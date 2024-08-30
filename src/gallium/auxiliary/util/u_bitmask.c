   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
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
   * Generic bitmask implementation.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "util/compiler.h"
   #include "util/u_debug.h"
      #include "util/u_memory.h"
   #include "util/u_bitmask.h"
         typedef uint32_t util_bitmask_word;
         #define UTIL_BITMASK_INITIAL_WORDS 16
   #define UTIL_BITMASK_BITS_PER_BYTE 8
   #define UTIL_BITMASK_BITS_PER_WORD (sizeof(util_bitmask_word) * UTIL_BITMASK_BITS_PER_BYTE)
         struct util_bitmask
   {
               /** Number of bits we can currently hold */
            /** Number of consecutive bits set at the start of the bitmask */
      };
         struct util_bitmask *
   util_bitmask_create(void)
   {
               bm = MALLOC_STRUCT(util_bitmask);
   if (!bm)
            bm->words = (util_bitmask_word *)
         if (!bm->words) {
      FREE(bm);
               bm->size = UTIL_BITMASK_INITIAL_WORDS * UTIL_BITMASK_BITS_PER_WORD;
               }
         /**
   * Resize the bitmask if necessary
   */
   static inline bool
   util_bitmask_resize(struct util_bitmask *bm,
         {
      const unsigned minimum_size = minimum_index + 1;
   unsigned new_size;
            /* Check integer overflow */
   if (!minimum_size)
            if (bm->size >= minimum_size)
            assert(bm->size % UTIL_BITMASK_BITS_PER_WORD == 0);
   new_size = bm->size;
   while (new_size < minimum_size) {
      new_size *= 2;
   /* Check integer overflow */
   if (new_size < bm->size)
      }
   assert(new_size);
            new_words = (util_bitmask_word *)
      REALLOC((void *)bm->words,
            if (!new_words)
            memset(new_words + bm->size/UTIL_BITMASK_BITS_PER_WORD,
                  bm->size = new_size;
               }
         /**
   * Check if we can increment the filled counter.
   */
   static inline void
   util_bitmask_filled_set(struct util_bitmask *bm,
         {
      assert(bm->filled <= bm->size);
            if (index == bm->filled) {
      ++bm->filled;
         }
         /**
   * Check if we need to decrement the filled counter.
   */
   static inline void
   util_bitmask_filled_unset(struct util_bitmask *bm,
         {
      assert(bm->filled <= bm->size);
            if (index < bm->filled)
      }
         unsigned
   util_bitmask_add(struct util_bitmask *bm)
   {
      unsigned word;
   unsigned bit;
                     /* linear search for an empty index, starting at filled position */
   word = bm->filled / UTIL_BITMASK_BITS_PER_WORD;
   bit  = bm->filled % UTIL_BITMASK_BITS_PER_WORD;
   mask = 1 << bit;
   while (word < bm->size / UTIL_BITMASK_BITS_PER_WORD) {
      while (bit < UTIL_BITMASK_BITS_PER_WORD) {
      if (!(bm->words[word] & mask))
         ++bm->filled;
   ++bit;
      }
   ++word;
   bit = 0;
         found:
         /* grow the bitmask if necessary */
   if (!util_bitmask_resize(bm, bm->filled))
            assert(!(bm->words[word] & mask));
               }
         unsigned
   util_bitmask_set(struct util_bitmask *bm,
         {
      unsigned word;
   unsigned bit;
                     /* grow the bitmask if necessary */
   if (!util_bitmask_resize(bm, index))
            word = index / UTIL_BITMASK_BITS_PER_WORD;
   bit  = index % UTIL_BITMASK_BITS_PER_WORD;
                                 }
         void
   util_bitmask_clear(struct util_bitmask *bm,
         {
      unsigned word;
   unsigned bit;
                     if (index >= bm->size)
            word = index / UTIL_BITMASK_BITS_PER_WORD;
   bit  = index % UTIL_BITMASK_BITS_PER_WORD;
                        }
         bool
   util_bitmask_get(struct util_bitmask *bm,
         {
      const unsigned word = index / UTIL_BITMASK_BITS_PER_WORD;
   const unsigned bit  = index % UTIL_BITMASK_BITS_PER_WORD;
                     if (index < bm->filled) {
      assert(bm->words[word] & mask);
               if (index >= bm->size)
            if (bm->words[word] & mask) {
      util_bitmask_filled_set(bm, index);
      }
   else
      }
         unsigned
   util_bitmask_get_next_index(struct util_bitmask *bm,
         {
      unsigned word = index / UTIL_BITMASK_BITS_PER_WORD;
   unsigned bit  = index % UTIL_BITMASK_BITS_PER_WORD;
            if (index < bm->filled) {
      assert(bm->words[word] & mask);
               if (index >= bm->size) {
                  /* Do a linear search */
   while (word < bm->size / UTIL_BITMASK_BITS_PER_WORD) {
      while (bit < UTIL_BITMASK_BITS_PER_WORD) {
      if (bm->words[word] & mask) {
      if (index == bm->filled) {
      ++bm->filled;
      }
      }
   ++index;
   ++bit;
      }
   ++word;
   bit = 0;
                  }
         unsigned
   util_bitmask_get_first_index(struct util_bitmask *bm)
   {
         }
         void
   util_bitmask_destroy(struct util_bitmask *bm)
   {
      if (bm) {
      FREE(bm->words);
         }
