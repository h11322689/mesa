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
      #undef NDEBUG
      #include <gtest/gtest.h>
   #include "util/bigmath.h"
   #include "util/fast_idiv_by_const.h"
   #include "util/u_math.h"
      #define RAND_TEST_ITERATIONS 100000
      static inline uint64_t
   utrunc(uint64_t x, unsigned num_bits)
   {
      if (num_bits == 64)
               }
      static inline int64_t
   strunc(int64_t x, unsigned num_bits)
   {
      if (num_bits == 64)
               }
      static inline bool
   uint_is_in_range(uint64_t x, unsigned num_bits)
   {
      if (num_bits == 64)
               }
      static inline bool
   sint_is_in_range(int64_t x, unsigned num_bits)
   {
      if (num_bits == 64)
            return x >= -(1ll << (num_bits - 1)) &&
      }
      static inline uint64_t
   uadd_sat(uint64_t a, uint64_t b, unsigned num_bits)
   {
      assert(uint_is_in_range(a, num_bits));
            uint64_t sum = a + b;
   if (num_bits == 64) {
      /* Standard overflow check */
      } else {
      /* Check if sum is more than num_bits */
         }
      static inline uint64_t
   umul_add_high(uint64_t a, uint64_t b, uint64_t c, unsigned num_bits)
   {
      assert(uint_is_in_range(a, num_bits));
   assert(uint_is_in_range(b, num_bits));
            if (num_bits == 64) {
      uint32_t a32[2] = { (uint32_t)a, (uint32_t)(a >> 32) };
   uint32_t b32[2] = { (uint32_t)b, (uint32_t)(b >> 32) };
            uint32_t ab32[4];
            uint32_t abc32[4];
               } else {
      assert(num_bits <= 32);
         }
      static inline int64_t
   smul_high(int64_t a, int64_t b, unsigned num_bits)
   {
      assert(sint_is_in_range(a, num_bits));
            if (num_bits == 64) {
      uint32_t a32[4] = {
      (uint32_t)a,
   (uint32_t)(a >> 32),
   (uint32_t)(a >> 63), /* sign extend */
      };
   uint32_t b32[4] = {
      (uint32_t)b,
   (uint32_t)(b >> 32),
   (uint32_t)(b >> 63), /* sign extend */
               uint32_t ab32[4];
               } else {
      assert(num_bits <= 32);
         }
      static inline uint64_t
   fast_udiv_add_sat(uint64_t n, struct util_fast_udiv_info m, unsigned num_bits)
   {
      assert(uint_is_in_range(n, num_bits));
            n = n >> m.pre_shift;
   n = uadd_sat(n, m.increment, num_bits);
   n = umul_add_high(n, m.multiplier, 0, num_bits);
               }
      static inline uint64_t
   fast_udiv_mul_add(uint64_t n, struct util_fast_udiv_info m, unsigned num_bits)
   {
      assert(uint_is_in_range(n, num_bits));
            n = n >> m.pre_shift;
   n = umul_add_high(n, m.multiplier,
                           }
      static inline uint64_t
   fast_sdiv(int64_t n, int64_t d, struct util_fast_sdiv_info m, unsigned num_bits)
   {
      assert(sint_is_in_range(n, num_bits));
   assert(sint_is_in_range(d, num_bits));
            int64_t res;
   res = smul_high(n, m.multiplier, num_bits);
   if (d > 0 && m.multiplier < 0)
         if (d < 0 && m.multiplier > 0)
         res = res >> m.shift;
               }
      static uint64_t
   rand_uint(unsigned bits, unsigned min)
   {
               /* Make sure we get some small and large numbers and powers of two every
   * once in a while
   */
   int k = rand() % 64;
   if (k == 17) {
         } else if (k == 42) {
         } else if (k == 9) {
      uint64_t r;
   do {
         } while (r < min);
               if (min == 0) {
      assert(bits <= 64);
   uint64_t r = 0;
   for (unsigned i = 0; i < 8; i++)
            } else {
      uint64_t r;
   do {
         } while (r < min);
         }
      static int64_t
   rand_sint(unsigned bits, unsigned min_abs)
   {
      /* Make sure we hit MIN_INT every once in a while */
   if (rand() % 64 == 37)
            int64_t s = rand_uint(bits - 1, min_abs);
      }
      static uint64_t
   udiv(uint64_t a, uint64_t b, unsigned bit_size)
   {
      switch (bit_size) {
   case 64: return (uint64_t)a / (uint64_t)b;
   case 32: return (uint32_t)a / (uint32_t)b;
   case 16: return (uint16_t)a / (uint16_t)b;
   case 8:  return  (uint8_t)a / (uint8_t)b;
   default:
      assert(!"Invalid bit size");
         }
      static int64_t
   sdiv(int64_t a, int64_t b, unsigned bit_size)
   {
      switch (bit_size) {
   case 64: return (int64_t)a / (int64_t)b;
   case 32: return (int32_t)a / (int32_t)b;
   case 16: return (int16_t)a / (int16_t)b;
   case 8:  return  (int8_t)a / (int8_t)b;
   default:
      assert(!"Invalid bit size");
         }
      static void
   random_udiv_add_sat_test(unsigned bits, bool bounded)
   {
      for (unsigned i = 0; i < RAND_TEST_ITERATIONS; i++) {
      uint64_t n = rand_uint(bits, 0);
   uint64_t d = rand_uint(bits, 2);
   assert(uint_is_in_range(n, bits));
                     struct util_fast_udiv_info m =
               }
      static void
   random_udiv_mul_add_test(unsigned bits, bool bounded)
   {
      for (unsigned i = 0; i < RAND_TEST_ITERATIONS; i++) {
      uint64_t n = rand_uint(bits, 0);
   uint64_t d = rand_uint(bits, 1);
   assert(uint_is_in_range(n, bits));
                     struct util_fast_udiv_info m =
               }
      static void
   random_sdiv_test(unsigned bits)
   {
      for (unsigned i = 0; i < RAND_TEST_ITERATIONS; i++) {
      int64_t n = rand_sint(bits, 0);
   int64_t d;
   do {
                  assert(sint_is_in_range(n, bits));
            struct util_fast_sdiv_info m =
               }
      TEST(fast_idiv_by_const, uint8_add_sat)
   {
      /* 8-bit is small enough we can brute-force the entire space */
   for (unsigned d = 2; d < 256; d++) {
      for (unsigned n_bits = 1; n_bits <= 8; n_bits++) {
                     for (unsigned n = 0; n < (1u << n_bits); n++)
            }
      TEST(fast_idiv_by_const, uint8_mul_add)
   {
      /* 8-bit is small enough we can brute-force the entire space */
   for (unsigned d = 2; d < 256; d++) {
      for (unsigned n_bits = 1; n_bits <= 8; n_bits++) {
                     for (unsigned n = 0; n < (1u << n_bits); n++)
            }
      TEST(fast_idiv_by_const, int8)
   {
      /* 8-bit is small enough we can brute-force the entire space */
   for (int n = -128; n < 128; n++) {
      for (int d = -128; d < 128; d++) {
                     struct util_fast_sdiv_info m =
                  }
      TEST(fast_idiv_by_const, uint16_add_sat_bounded)
   {
         }
      TEST(fast_idiv_by_const, uint16_add_sat_full)
   {
         }
      TEST(fast_idiv_by_const, uint16_mul_add_bounded)
   {
         }
      TEST(fast_idiv_by_const, uint16_mul_add_full)
   {
         }
      TEST(fast_idiv_by_const, int16)
   {
         }
      TEST(fast_idiv_by_const, uint32_add_sat_bounded)
   {
         }
      TEST(fast_idiv_by_const, uint32_add_sat_full)
   {
         }
      TEST(fast_idiv_by_const, uint32_mul_add_bounded)
   {
         }
      TEST(fast_idiv_by_const, uint32_mul_add_full)
   {
         }
      TEST(fast_idiv_by_const, int32)
   {
         }
      TEST(fast_idiv_by_const, util_fast_udiv32)
   {
      for (unsigned i = 0; i < RAND_TEST_ITERATIONS; i++) {
      uint32_t n = rand_uint(32, 0);
            struct util_fast_udiv_info m =
               }
      TEST(fast_idiv_by_const, util_fast_udiv32_nuw)
   {
      for (unsigned i = 0; i < RAND_TEST_ITERATIONS; i++) {
      uint32_t n = rand_uint(32, 0);
   if (n == UINT32_MAX)
                  struct util_fast_udiv_info m =
               }
      TEST(fast_idiv_by_const, util_fast_udiv32_u31_d_not_one)
   {
      for (unsigned i = 0; i < RAND_TEST_ITERATIONS; i++) {
      uint32_t n = rand_uint(31, 0);
            struct util_fast_udiv_info m =
               }
      TEST(fast_idiv_by_const, uint64_add_sat_bounded)
   {
         }
      TEST(fast_idiv_by_const, uint64_add_sat_full)
   {
         }
      TEST(fast_idiv_by_const, uint64_mul_add_bounded)
   {
         }
      TEST(fast_idiv_by_const, uint64_mul_add_full)
   {
         }
      TEST(fast_idiv_by_const, int64)
   {
         }
