   /*
   * Copyright 2010 Marek Olšák <maraeo@gmail.com>
   * Copyright 2016 Advanced Micro Devices, Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
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
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      #include "slab.h"
   #include "macros.h"
   #include "u_atomic.h"
   #include <stdint.h>
   #include <stdbool.h>
   #include <string.h>
      #define SLAB_MAGIC_ALLOCATED 0xcafe4321
   #define SLAB_MAGIC_FREE 0x7ee01234
      #ifndef NDEBUG
   #define SET_MAGIC(element, value)   (element)->magic = (value)
   #define CHECK_MAGIC(element, value) assert((element)->magic == (value))
   #else
   #define SET_MAGIC(element, value)
   #define CHECK_MAGIC(element, value)
   #endif
      /* One array element within a big buffer. */
   struct slab_element_header {
      /* The next element in the free or migrated list. */
            /* This is either
   * - a pointer to the child pool to which this element belongs, or
   * - a pointer to the orphaned page of the element, with the least
   *   significant bit set to 1.
   */
         #ifndef NDEBUG
         #endif
   };
      /* The page is an array of allocations in one block. */
   struct slab_page_header {
      union {
      /* Next page in the same child pool. */
            /* Number of remaining, non-freed elements (for orphaned pages). */
      } u;
   /* Memory after the last member is dedicated to the page itself.
   * The allocated size is always larger than this structure.
      };
         static struct slab_element_header *
   slab_get_element(struct slab_parent_pool *parent,
         {
      return (struct slab_element_header*)
      }
      /* The given object/element belongs to an orphaned page (i.e. the owning child
   * pool has been destroyed). Mark the element as freed and free the whole page
   * when no elements are left in it.
   */
   static void
   slab_free_orphaned(struct slab_element_header *elt)
   {
                        page = (struct slab_page_header *)(elt->owner & ~(intptr_t)1);
   if (!p_atomic_dec_return(&page->u.num_remaining))
      }
      /**
   * Create a parent pool for the allocation of same-sized objects.
   *
   * \param item_size     Size of one object.
   * \param num_items     Number of objects to allocate at once.
   */
   void
   slab_create_parent(struct slab_parent_pool *parent,
               {
      simple_mtx_init(&parent->mutex, mtx_plain);
   parent->element_size = ALIGN_POT(sizeof(struct slab_element_header) + item_size,
         parent->num_elements = num_items;
      }
      void
   slab_destroy_parent(struct slab_parent_pool *parent)
   {
         }
      /**
   * Create a child pool linked to the given parent.
   */
   void slab_create_child(struct slab_child_pool *pool,
         {
      pool->parent = parent;
   pool->pages = NULL;
   pool->free = NULL;
      }
      /**
   * Destroy the child pool.
   *
   * Pages associated to the pool will be orphaned. They are eventually freed
   * when all objects in them are freed.
   */
   void slab_destroy_child(struct slab_child_pool *pool)
   {
      if (!pool->parent)
                     while (pool->pages) {
      struct slab_page_header *page = pool->pages;
   pool->pages = page->u.next;
            for (unsigned i = 0; i < pool->parent->num_elements; ++i) {
      struct slab_element_header *elt = slab_get_element(pool->parent, page, i);
                  while (pool->migrated) {
      struct slab_element_header *elt = pool->migrated;
   pool->migrated = elt->next;
                        while (pool->free) {
      struct slab_element_header *elt = pool->free;
   pool->free = elt->next;
               /* Guard against use-after-free. */
      }
      static bool
   slab_add_new_page(struct slab_child_pool *pool)
   {
      struct slab_page_header *page = malloc(sizeof(struct slab_page_header) +
            if (!page)
            for (unsigned i = 0; i < pool->parent->num_elements; ++i) {
      struct slab_element_header *elt = slab_get_element(pool->parent, page, i);
   elt->owner = (intptr_t)pool;
            elt->next = pool->free;
   pool->free = elt;
               page->u.next = pool->pages;
               }
      /**
   * Allocate an object from the child pool. Single-threaded (i.e. the caller
   * must ensure that no operation happens on the same child pool in another
   * thread).
   */
   void *
   slab_alloc(struct slab_child_pool *pool)
   {
               if (!pool->free) {
      /* First, collect elements that belong to us but were freed from a
   * different child pool.
   */
   simple_mtx_lock(&pool->parent->mutex);
   pool->free = pool->migrated;
   pool->migrated = NULL;
            /* Now allocate a new page. */
   if (!pool->free && !slab_add_new_page(pool))
               elt = pool->free;
            CHECK_MAGIC(elt, SLAB_MAGIC_FREE);
               }
      /**
   * Same as slab_alloc but memset the returned object to 0.
   */
   void *
   slab_zalloc(struct slab_child_pool *pool)
   {
      void *r = slab_alloc(pool);
   if (r)
            }
      /**
   * Free an object allocated from the slab. Single-threaded (i.e. the caller
   * must ensure that no operation happens on the same child pool in another
   * thread).
   *
   * Freeing an object in a different child pool from the one where it was
   * allocated is allowed, as long the pool belong to the same parent. No
   * additional locking is required in this case.
   */
   void slab_free(struct slab_child_pool *pool, void *ptr)
   {
      struct slab_element_header *elt = ((struct slab_element_header*)ptr - 1);
            CHECK_MAGIC(elt, SLAB_MAGIC_ALLOCATED);
            if (p_atomic_read(&elt->owner) == (intptr_t)pool) {
      /* This is the simple case: The caller guarantees that we can safely
   * access the free list.
   */
   elt->next = pool->free;
   pool->free = elt;
               /* The slow case: migration or an orphaned page. */
   if (pool->parent)
            /* Note: we _must_ re-read elt->owner here because the owning child pool
   * may have been destroyed by another thread in the meantime.
   */
            if (!(owner_int & 1)) {
      struct slab_child_pool *owner = (struct slab_child_pool *)owner_int;
   elt->next = owner->migrated;
   owner->migrated = elt;
   if (pool->parent)
      } else {
      if (pool->parent)
                  }
      /**
   * Allocate an object from the slab. Single-threaded (no mutex).
   */
   void *
   slab_alloc_st(struct slab_mempool *mempool)
   {
         }
      /**
   * Free an object allocated from the slab. Single-threaded (no mutex).
   */
   void
   slab_free_st(struct slab_mempool *mempool, void *ptr)
   {
         }
      void
   slab_destroy(struct slab_mempool *mempool)
   {
      slab_destroy_child(&mempool->child);
      }
      /**
   * Create an allocator for same-sized objects.
   *
   * \param item_size     Size of one object.
   * \param num_items     Number of objects to allocate at once.
   */
   void
   slab_create(struct slab_mempool *mempool,
               {
      slab_create_parent(&mempool->parent, item_size, num_items);
      }
