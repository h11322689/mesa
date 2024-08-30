   /*
   * Copyright 2013 Christoph Bumiller
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
      #include "nine_helpers.h"
      static struct nine_range *
   nine_range_pool_more(struct nine_range_pool *pool)
   {
      struct nine_range *r = MALLOC(64 * sizeof(struct nine_range));
   int i;
            if (pool->num_slabs == pool->num_slabs_max) {
      unsigned p = pool->num_slabs_max;
   unsigned n = pool->num_slabs_max * 2;
   if (!n)
         pool->slabs = REALLOC(pool->slabs,
                  }
            for (i = 0; i < 63; ++i, r = r->next)
      r->next = (struct nine_range *)
                  }
      static inline struct nine_range *
   nine_range_pool_get(struct nine_range_pool *pool, int16_t bgn, int16_t end)
   {
      struct nine_range *r = pool->free;
   if (!r)
         assert(r);
   pool->free = r->next;
   r->bgn = bgn;
   r->end = end;
      }
      static inline void
   nine_ranges_coalesce(struct nine_range *r, struct nine_range_pool *pool)
   {
               while (r->next && r->end >= r->next->bgn) {
      n = r->next->next;
   r->end = (r->end >= r->next->end) ? r->end : r->next->end;
   nine_range_pool_put(pool, r->next);
         }
      void
   nine_ranges_insert(struct nine_range **head, int16_t bgn, int16_t end,
         {
                        if (!r || end < r->bgn) {
      *pn = nine_range_pool_get(pool, bgn, end);
      } else
   if (bgn < r->bgn) {
      r->bgn = bgn;
   if (end > r->end)
            } else
   if (end > r->end) {
      r->end = end;
         }
