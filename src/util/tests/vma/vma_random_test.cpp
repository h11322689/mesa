   /*
   * Copyright © 2018 Intel Corporation
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
      /* it is a test after all */
   #undef NDEBUG
      #include <algorithm>
   #include <cassert>
   #include <climits>
   #include <cstdint>
   #include <cstdio>
   #include <cstdlib>
   #include <random>
   #include <set>
   #include <vector>
      #ifndef _WIN32
   #include <err.h>
   #else
   #define errx(code, msg, ...)             \
      do {                                  \
      fprintf(stderr, msg, __VA_ARGS__); \
         #endif
      #include "util/vma.h"
      namespace {
      static const uint64_t MEM_PAGE_SIZE = 4096;
      struct allocation {
      uint64_t start_page;
      };
      struct allocation_less {
      bool operator()(const allocation& lhs, const allocation& rhs) const
   {
      assert(lhs.start_page + lhs.num_pages > lhs.start_page);
         };
      constexpr uint64_t allocation_end_page(const allocation& a) {
         }
      struct random_test {
      static const uint64_t MEM_START_PAGE = 1;
   static const uint64_t MEM_SIZE = 0xfffffffffffff000;
            random_test(uint_fast32_t seed)
         {
                  ~random_test()
   {
                  void test(unsigned long count)
   {
      std::uniform_int_distribution<> one_to_thousand(1, 1000);
   while (count-- > 0) {
      int action = one_to_thousand(rand);
   if (action == 1)          fill();
   else if (action == 2)     empty();
   else if (action < 374)    dealloc();
                  bool alloc(uint64_t size_order=52, uint64_t align_order=52)
   {
               if (align_order > 51)
         uint64_t align_pages = 1ULL << align_order;
            if (size_order > 51)
         uint64_t size_pages = 1ULL << size_order;
                     if (addr == 0) {
      /* assert no gaps are present in the tracker that could satisfy this
   * allocation.
   */
   for (const auto& hole : heap_holes) {
      uint64_t hole_alignment_pages =
            }
      } else {
      assert(addr % align == 0);
   uint64_t addr_page = addr / MEM_PAGE_SIZE;
   allocation a{addr_page, size_pages};
   auto i = heap_holes.find(a);
                                 heap_holes.erase(i);
   if (hole.start_page < a.start_page) {
      heap_holes.emplace(allocation{hole.start_page,
      }
   if (allocation_end_page(hole) > allocation_end_page(a)) {
      heap_holes.emplace(allocation{allocation_end_page(a),
               allocations.push_back(a);
                  void dealloc()
   {
      if (allocations.size() == 0)
            std::uniform_int_distribution<> dist(0, allocations.size() - 1);
            std::swap(allocations.at(to_dealloc), allocations.back());
   allocation a = allocations.back();
            util_vma_heap_free(&heap, a.start_page * MEM_PAGE_SIZE,
            assert(heap_holes.find(a) == end(heap_holes));
   auto next = heap_holes.upper_bound(a);
   if (next != end(heap_holes)) {
      if (next->start_page == allocation_end_page(a)) {
      allocation x {a.start_page, a.num_pages + next->num_pages};
                  if (next != begin(heap_holes)) {
      auto prev = next;
   prev--;
                           heap_holes.erase(prev);
   next = heap_holes.erase(next);
                                 if (next != begin(heap_holes)) {
      auto prev = next;
   prev--;
   if (allocation_end_page(*prev) == a.start_page) {
      allocation x {prev->start_page, prev->num_pages + a.num_pages};
                                             void fill()
   {
      for (int sz = 51; sz >= 0; sz--) {
      while (alloc(sz, 0))
      }
               void empty()
   {
      while (allocations.size() != 0)
         assert(heap_holes.size() == 1);
   auto& hole = *begin(heap_holes);
               struct util_vma_heap heap;
   std::set<allocation, allocation_less> heap_holes;
   std::default_random_engine rand;
      };
      }
      int main(int argc, char **argv)
   {
      unsigned long seed, count;
   if (argc == 3) {
      char *arg_end = NULL;
   seed = strtoul(argv[1], &arg_end, 0);
   if (!arg_end || *arg_end || seed == ULONG_MAX)
            arg_end = NULL;
   count = strtoul(argv[2], &arg_end, 0);
   if (!arg_end || *arg_end || count == ULONG_MAX)
      } else if (argc == 1) {
      /* importantly chosen prime numbers. */
   seed = 8675309;
      } else {
                  random_test r{(uint_fast32_t)seed};
            printf("ok\n");
      }
