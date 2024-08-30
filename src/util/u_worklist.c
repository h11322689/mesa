   /*
   * Copyright (c) 2022 Collabora Ltd.
   * Copyright © 2014 Intel Corporation
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
   */
      #include "u_worklist.h"
   #include "ralloc.h"
      void
   u_worklist_init(u_worklist *w, unsigned num_entries, void *mem_ctx)
   {
      w->size = num_entries;
   w->count = 0;
            w->present = rzalloc_array(mem_ctx, BITSET_WORD, BITSET_WORDS(num_entries));
      }
      void
   u_worklist_fini(u_worklist *w)
   {
      ralloc_free(w->present);
      }
      void
   u_worklist_push_head_index(u_worklist *w, unsigned *index)
   {
      /* Pushing a block we already have is a no-op */
   if (BITSET_TEST(w->present, *index))
                     if (w->start == 0)
         else
                     w->entries[w->start] = index;
      }
      unsigned *
   u_worklist_peek_head_index(const u_worklist *w)
   {
                  }
      unsigned *
   u_worklist_pop_head_index(u_worklist *w)
   {
                        w->start = (w->start + 1) % w->size;
            BITSET_CLEAR(w->present, *(w->entries[head]));
      }
      void
   u_worklist_push_tail_index(u_worklist *w, unsigned *index)
   {
      /* Pushing a block we already have is a no-op */
   if (BITSET_TEST(w->present, *index))
                                       w->entries[tail] = index;
      }
      unsigned *
   u_worklist_peek_tail_index(const u_worklist *w)
   {
                           }
      unsigned *
   u_worklist_pop_tail_index(u_worklist *w)
   {
                                 BITSET_CLEAR(w->present, *(w->entries[tail]));
      }
