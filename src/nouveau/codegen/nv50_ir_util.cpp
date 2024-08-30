   /*
   * Copyright 2011 Christoph Bumiller
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "nv50_ir_util.h"
      namespace nv50_ir {
      void DLList::clear()
   {
      for (Item *next, *item = head.next; item != &head; item = next) {
      next = item->next;
      }
      }
      void
   DLList::Iterator::erase()
   {
               if (rem == term)
                  DLLIST_DEL(rem);
      }
      void DLList::Iterator::moveToList(DLList& dest)
   {
               assert(term != &dest.head);
                     DLLIST_DEL(item);
      }
      bool
   DLList::Iterator::insert(void *data)
   {
               ins->next = pos->next;
   ins->prev = pos;
   pos->next->prev = ins;
            if (pos == term)
               }
      void
   Stack::moveTo(Stack& that)
   {
               while (newSize > that.limit)
                  that.size = newSize;
      }
      Interval::Interval(const Interval& that) : head(NULL), tail(NULL)
   {
         }
      Interval::~Interval()
   {
         }
      void
   Interval::clear()
   {
      for (Range *next, *r = head; r; r = next) {
      next = r->next;
      }
      }
      bool
   Interval::extend(int a, int b)
   {
               // NOTE: we need empty intervals for fixed registers
   // if (a == b)
   //   return false;
            for (r = head; r; r = r->next) {
      if (b < r->bgn)
         if (a > r->end) {
      // insert after
   nextp = &r->next;
               // overlap
   if (a < r->bgn) {
      r->bgn = a;
   if (b > r->end)
         r->coalesce(&tail);
      }
   if (b > r->end) {
      r->end = b;
   r->coalesce(&tail);
      }
   assert(a >= r->bgn);
   assert(b <= r->end);
               (*nextp) = new Range(a, b);
            for (r = (*nextp); r->next; r = r->next);
   tail = r;
      }
      bool Interval::contains(int pos) const
   {
      for (Range *r = head; r && r->bgn <= pos; r = r->next)
      if (r->end > pos)
         }
      bool Interval::overlaps(const Interval &that) const
   {
   #if 1
      Range *a = this->head;
            while (a && b) {
      if (b->bgn < a->end &&
      b->end > a->bgn)
      if (a->end <= b->bgn)
         else
         #else
      for (Range *rA = this->head; rA; rA = rA->next)
      for (Range *rB = iv.head; rB; rB = rB->next)
      if (rB->bgn < rA->end &&
      #endif
         }
      void Interval::insert(const Interval &that)
   {
      for (Range *r = that.head; r; r = r->next)
      }
      void Interval::unify(Interval &that)
   {
      assert(this != &that);
   for (Range *next, *r = that.head; r; r = next) {
      next = r->next;
   this->extend(r->bgn, r->end);
      }
      }
      int Interval::length() const
   {
      int len = 0;
   for (Range *r = head; r; r = r->next)
            }
      void Interval::print() const
   {
      if (!head)
         INFO("[%i %i)", head->bgn, head->end);
   for (const Range *r = head->next; r; r = r->next)
            }
      void
   BitSet::andNot(const BitSet &set)
   {
      assert(data && set.data);
   assert(size >= set.size);
   for (unsigned int i = 0; i < (set.size + 31) / 32; ++i)
      }
      BitSet& BitSet::operator|=(const BitSet &set)
   {
      assert(data && set.data);
   assert(size >= set.size);
   for (unsigned int i = 0; i < (set.size + 31) / 32; ++i)
            }
      bool BitSet::resize(unsigned int nBits)
   {
      if (!data || !nBits)
         const unsigned int p = (size + 31) / 32;
   const unsigned int n = (nBits + 31) / 32;
   if (n == p)
            data = (uint32_t *)REALLOC(data, 4 * p, 4 * n);
   if (!data) {
      size = 0;
      }
   if (n > p)
         if (nBits < size && (nBits % 32))
            size = nBits;
      }
      bool BitSet::allocate(unsigned int nBits, bool zero)
   {
      if (data && size < nBits) {
      FREE(data);
      }
            if (!data)
            if (zero)
         else
   if (size % 32) // clear unused bits (e.g. for popCount)
               }
      unsigned int BitSet::popCount() const
   {
               for (unsigned int i = 0; i < (size + 31) / 32; ++i)
      if (data[i])
         }
      void BitSet::fill(uint32_t val)
   {
      unsigned int i;
   for (i = 0; i < (size + 31) / 32; ++i)
         if (val && i)
      }
      void BitSet::setOr(BitSet *pA, BitSet *pB)
   {
      if (!pB) {
         } else {
      for (unsigned int i = 0; i < (size + 31) / 32; ++i)
         }
      int BitSet::findFreeRange(unsigned int count, unsigned int max) const
   {
      const uint32_t m = (1 << count) - 1;
   int pos = max;
   unsigned int i;
            if (count == 1) {
      for (i = 0; i < end; ++i) {
      pos = ffs(~data[i]) - 1;
   if (pos >= 0)
         } else
   if (count == 2) {
      for (i = 0; i < end; ++i) {
      if (data[i] != 0xffffffff) {
      uint32_t b = data[i] | (data[i] >> 1) | 0xaaaaaaaa;
   pos = ffs(~b) - 1;
   if (pos >= 0)
            } else
   if (count == 4 || count == 3) {
      for (i = 0; i < end; ++i) {
      if (data[i] != 0xffffffff) {
      uint32_t b =
      (data[i] >> 0) | (data[i] >> 1) |
      pos = ffs(~b) - 1;
   if (pos >= 0)
            } else {
      if (count <= 8)
         else
   if (count <= 16)
         else
            for (i = 0; i < end; ++i) {
      if (data[i] != 0xffffffff) {
      for (pos = 0; pos < 32; pos += count)
      if (!(data[i] & (m << pos)))
      if (pos < 32)
                     // If we couldn't find a position, we can have a left-over -1 in pos. Make
   // sure to abort in such a case.
   if (pos < 0)
                        }
      void BitSet::print() const
   {
      unsigned int n = 0;
   INFO("BitSet of size %u:\n", size);
   for (unsigned int i = 0; i < (size + 31) / 32; ++i) {
      uint32_t bits = data[i];
   while (bits) {
      int pos = ffs(bits) - 1;
   bits &= ~(1 << pos);
   INFO(" %i", i * 32 + pos);
   ++n;
   if ((n % 16) == 0)
         }
   if (n % 16)
      }
      } // namespace nv50_ir
