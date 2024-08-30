   /*
   * Copyright 2022 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      /* See the big comment.
   *
   * Compile: gcc find_hash_func.c -fopenmp -O3 -g -o find_hash_func
   */
      #include <stdio.h>
   #include <string.h>
   #include <stdbool.h>
   #include <alloca.h>
   #include <GL/gl.h>
   #include <GL/glext.h>
   #include <GLES2/gl2.h>
   #include <GLES2/gl2ext.h>
      #define MAX_GLENUM_BITS 16
      /* Duplicate this here. Don't pull the whole Mesa's built system into this. */
   static inline unsigned
   util_next_power_of_two(unsigned x)
   {
      if (x <= 1)
               }
      struct entry {
      unsigned result;
   const char *name;
      };
      /* Given a list of large values (such as GLenums), find a simple perfect hash
   * function that maps the large values to smallest possible numbers for use as
   * array indices, so that we can index arrays by hash(GLenum). This is useful
   * when a switch statement for conversions from GLenums to indices would be
   * undesirable.
   *
   * The final hash function is always in this form:
   *      hash(x) = ((x * mul) >> rshift) & BITFIELD_MASK(bits)
   *
   * This is a brute force algorithm that tries to find all injective
   * (mul, rshift, bits) hash functions and return the one whose maximum
   * generated value is the smallest.
   */
   static bool
   find_perfect_hash_func(const struct entry *list, unsigned *best_mul,
               {
      bool found = false;
   *best_mul = 1;
   *best_rshift = 0;
   *best_mask = ~0;
            for (unsigned mul = 1; mul < (1 << 16); mul++) {
      for (unsigned rshift = 1; rshift <= 31; rshift++) {
      for (unsigned bits = 1; bits <= MAX_GLENUM_BITS; bits++) {
                                                for (unsigned b = a + 1; list[b].name; b++) {
      /* Skip if the mapping is not injective. */
   if (hash == (((list[b].value * mul) >> rshift) & mask))
         }
   if (max < *best_max) {
      *best_mul = mul;
   *best_rshift = rshift;
   *best_mask = mask;
   *best_max = max;
         fail:;
         }
      }
      static bool
   find_translate_func(const struct entry *list, unsigned *out_mul,
               {
      unsigned mask = util_next_power_of_two(max_result + 1) - 1;
   unsigned num_threads = 24;
   unsigned start_mul = 1;
            int thread_id_finished = -1;
   unsigned *result_mul = alloca(4 * num_threads);
         #pragma omp parallel for
      for (unsigned thread_id = 0; thread_id < num_threads; thread_id++) {
      for (unsigned mul = start_mul; mul < end_mul; mul += num_threads) {
      for (unsigned rshift = 1; rshift <= 31; rshift++) {
      for (unsigned add = 0; add <= max_result; add++) {
                        /* Reject the mapping if it doesn't return the expected result. */
                     result_mul[thread_id] = mul;
   result_rshift[thread_id] = rshift;
   __atomic_store_n(&thread_id_finished, thread_id, __ATOMIC_RELEASE);
   puts("found");
      fail:;
            done:;
            if (__atomic_load_n(&thread_id_finished, __ATOMIC_ACQUIRE) >= 0) {
      *out_mul = result_mul[thread_id_finished];
   *out_rshift = result_rshift[thread_id_finished];
   *out_mask = mask;
                  }
      static void
   print_hash_code(const char *uppercase_name, const char *lowercase_name,
         {
      unsigned mul, rshift, mask, max;
            for (unsigned i = 0; list[i].name; i++) {
      int len = strlen(list[i].name);
   max_strlen = len > max_strlen ? len : max_strlen;
               /* Find the hash function that can be used as a translation function (no table). */
   if (get_translate_func) {
      if (find_translate_func(list, &mul, &rshift, &mask, max_result)) {
      printf("/* Translate enums to desired values arithmetically (without a switch) */\n");
                  for (unsigned i = 0; list[i].name; i++) {
      printf("static_assert(TRANSLATE_%s(%s) == %u)\n",
      }
      } else {
            } else {
      /* Find the hash function that can be used for indexing into a table. */
   if (find_perfect_hash_func(list, &mul, &rshift, &mask, &max)) {
      printf("/* Map enums to smaller enums arithmetically (without a switch) */\n");
                  /* Print the translation table. */
   printf("static const uint%u_t %s_table[16] = {\n",
         printf("   /* These elements are sorted by meaning, not value. */\n");
   for (unsigned i = 0; list[i].name; i++)
      printf("   [/*%2u*/ PERF_HASH_%s(%s)] = 0,\n",
               /* Print the uniqueness compile check. */
   printf("static inline void\n");
   printf("compile_check_uniqueness_of_%s(unsigned x)\n", lowercase_name);
   printf("{\n");
   printf("   /* This switch has the same purpose as static_assert.\n");
   printf("    * It should fail compilation if any case is not unique.\n");
   printf("    */\n");
   printf("   switch (x) {\n");
   for (unsigned i = 0; list[i].name; i++)
         printf("      break;\n");
                  printf("/* GL enums mapped to smaller numbers. The number are not contiguous. */\n");
   printf("typedef enum {\n");
   for (unsigned i = 0; list[i].name; i++) {
      printf("   MESA_%s = %*s/*%2u*/ PERF_HASH_%s(%s),\n",
         list[i].name + 3,
   1 + max_strlen - (int)strlen(list[i].name), " ",
      }
   printf("\n   NUM_%sS = %u,\n", uppercase_name, max + 1);
   printf("   NUM_%sS_POW2 = %u,\n",
            } else {
               }
      #define S(x) #x, x
      int main(int argc, char **argv)
   {
      struct entry vertex_types[] = {
      {0, S(GL_BYTE)},
   {0, S(GL_UNSIGNED_BYTE)},
   {0, S(GL_INT_2_10_10_10_REV)},
   {0, S(GL_UNSIGNED_INT_2_10_10_10_REV)},
   {1, S(GL_SHORT)},
   {1, S(GL_UNSIGNED_SHORT)},
   {1, S(GL_HALF_FLOAT_ARB)},
   {1, S(GL_HALF_FLOAT_OES)},
   {2, S(GL_INT)},
   {2, S(GL_UNSIGNED_INT)},
   {2, S(GL_FLOAT)},
   {2, S(GL_FIXED)},
   {3, S(GL_DOUBLE)},
   {3, S(GL_UNSIGNED_INT64_ARB)},
      };
               }
