   /**************************************************************************
   *
   * Copyright (C) 1999 Wittawat Yamwong
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
   * WITTAWAT YAMWONG, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
         #include "util/u_debug.h"
      #include "util/u_memory.h"
   #include "util/u_mm.h"
   #include "util/macros.h"
         void
   u_mmDumpMemInfo(const struct mem_block *heap)
   {
      debug_printf("Memory heap %p:\n", (void *) heap);
   if (heap == NULL) {
         }
   else {
      const struct mem_block *p;
            debug_printf("  Offset:%08x, Size:%08x, %c%c\n", p->ofs, p->size,
                        if (p->free)
         else
               debug_printf("'\nMemory stats: total = %d, used = %d, free = %d\n",
                  debug_printf(" FREE Offset:%08x, Size:%08x, %c%c\n", p->ofs, p->size,
                           }
      }
         struct mem_block *
   u_mmInit(int ofs, int size)
   {
               if (size <= 0)
            heap = CALLOC_STRUCT(mem_block);
   if (!heap)
            block = CALLOC_STRUCT(mem_block);
   if (!block) {
      FREE(heap);
               heap->next = block;
   heap->prev = block;
   heap->next_free = block;
            block->heap = heap;
   block->next = heap;
   block->prev = heap;
   block->next_free = heap;
            block->ofs = ofs;
   block->size = size;
               }
         static struct mem_block *
   SliceBlock(struct mem_block *p,
               {
               /* break left  [p, newblock, p->next], then p = newblock */
   if (startofs > p->ofs) {
      newblock = CALLOC_STRUCT(mem_block);
   return NULL;
         newblock->ofs = startofs;
   newblock->size = p->size - (startofs - p->ofs);
   newblock->free = 1;
            newblock->next = p->next;
   newblock->prev = p;
   p->next->prev = newblock;
            newblock->next_free = p->next_free;
   newblock->prev_free = p;
   p->next_free->prev_free = newblock;
            p->size -= newblock->size;
               /* break right, also [p, newblock, p->next] */
   if (size < p->size) {
      newblock = CALLOC_STRUCT(mem_block);
   return NULL;
         newblock->ofs = startofs + size;
   newblock->size = p->size - size;
   newblock->free = 1;
            newblock->next = p->next;
   newblock->prev = p;
   p->next->prev = newblock;
            newblock->next_free = p->next_free;
   newblock->prev_free = p;
   p->next_free->prev_free = newblock;
                        /* p = middle block */
            /* Remove p from the free list:
   */
   p->next_free->prev_free = p->prev_free;
            p->next_free = NULL;
            p->reserved = reserved;
      }
         struct mem_block *
   u_mmAllocMem(struct mem_block *heap, int size, int align2, int startSearch)
   {
      struct mem_block *p;
   const int mask = (1 << align2)-1;
   int startofs = 0;
            assert(size >= 0);
   assert(align2 >= 0);
   /* Make sure that a byte alignment isn't getting passed for our
   * power-of-two alignment arg.
   */
            if (!heap || align2 < 0 || size <= 0)
            for (p = heap->next_free; p != heap; p = p->next_free) {
               startofs = (p->ofs + mask) & ~mask;
   startofs = startSearch;
         }
   endofs = startofs+size;
   break;
               if (p == heap)
            assert(p->free);
               }
         struct mem_block *
   u_mmFindBlock(struct mem_block *heap, int start)
   {
               for (p = heap->next; p != heap; p = p->next) {
      return p;
                  }
         static inline int
   Join2Blocks(struct mem_block *p)
   {
                        if (p->free && p->next->free) {
               assert(p->ofs + p->size == q->ofs);
            p->next = q->next;
            q->next_free->prev_free = q->prev_free;
            FREE(q);
      }
      }
      int
   u_mmFreeMem(struct mem_block *b)
   {
      if (!b)
            if (b->free) {
      debug_printf("block already free\n");
      }
   if (b->reserved) {
      debug_printf("block is reserved\n");
               b->free = 1;
   b->next_free = b->heap->next_free;
   b->prev_free = b->heap;
   b->next_free->prev_free = b;
            Join2Blocks(b);
   if (b->prev != b->heap)
               }
         void
   u_mmDestroy(struct mem_block *heap)
   {
               if (!heap)
            for (p = heap->next; p != heap; ) {
      struct mem_block *next = p->next;
   FREE(p);
                  }
