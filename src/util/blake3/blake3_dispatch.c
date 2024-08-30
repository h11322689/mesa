   #include <stdbool.h>
   #include <stddef.h>
   #include <stdint.h>
      #include "blake3_impl.h"
      #if defined(IS_X86)
   #if defined(_MSC_VER)
   #include <intrin.h>
   #elif defined(__GNUC__)
   #include <immintrin.h>
   #else
   #undef IS_X86 /* Unimplemented! */
   #endif
   #endif
      #define MAYBE_UNUSED(x) (void)((x))
      #if defined(IS_X86)
   static uint64_t xgetbv(void) {
   #if defined(_MSC_VER)
   return _xgetbv(0);
   #else
   uint32_t eax = 0, edx = 0;
   __asm__ __volatile__("xgetbv\n" : "=a"(eax), "=d"(edx) : "c"(0));
   return ((uint64_t)edx << 32) | eax;
   #endif
   }
      static void cpuid(uint32_t out[4], uint32_t id) {
   #if defined(_MSC_VER)
   __cpuid((int *)out, id);
   #elif defined(__i386__) || defined(_M_IX86)
   __asm__ __volatile__("movl %%ebx, %1\n"
                           #else
   __asm__ __volatile__("cpuid\n"
               #endif
   }
      static void cpuidex(uint32_t out[4], uint32_t id, uint32_t sid) {
   #if defined(_MSC_VER)
   __cpuidex((int *)out, id, sid);
   #elif defined(__i386__) || defined(_M_IX86)
   __asm__ __volatile__("movl %%ebx, %1\n"
                           #else
   __asm__ __volatile__("cpuid\n"
               #endif
   }
      #endif
      enum cpu_feature {
   SSE2 = 1 << 0,
   SSSE3 = 1 << 1,
   SSE41 = 1 << 2,
   AVX = 1 << 3,
   AVX2 = 1 << 4,
   AVX512F = 1 << 5,
   AVX512VL = 1 << 6,
   /* ... */
   UNDEFINED = 1 << 30
   };
      #if !defined(BLAKE3_TESTING)
   static /* Allow the variable to be controlled manually for testing */
   #endif
            #if !defined(BLAKE3_TESTING)
   static
   #endif
      enum cpu_feature
         if (g_cpu_features != UNDEFINED) {
         } else {
   #if defined(IS_X86)
      uint32_t regs[4] = {0};
   uint32_t *eax = &regs[0], *ebx = &regs[1], *ecx = &regs[2], *edx = &regs[3];
   (void)edx;
   enum cpu_feature features = 0;
   cpuid(regs, 0);
   const int max_id = *eax;
      #if defined(__amd64__) || defined(_M_X64)
         #else
      if (*edx & (1UL << 26))
      #endif
      if (*ecx & (1UL << 0))
         if (*ecx & (1UL << 19))
            if (*ecx & (1UL << 27)) { // OSXSAVE
      const uint64_t mask = xgetbv();
   if ((mask & 6) == 6) { // SSE and AVX states
   if (*ecx & (1UL << 28))
         if (max_id >= 7) {
      cpuidex(regs, 7, 0);
   if (*ebx & (1UL << 5))
         if ((mask & 224) == 224) { // Opmask, ZMM_Hi256, Hi16_Zmm
      if (*ebx & (1UL << 31))
   features |= AVX512VL;
   if (*ebx & (1UL << 16))
         }
      }
   g_cpu_features = features;
      #else
      /* How to detect NEON? */
      #endif
   }
   }
      void blake3_compress_in_place(uint32_t cv[8],
                     #if defined(IS_X86)
   const enum cpu_feature features = get_cpu_features();
   MAYBE_UNUSED(features);
   #if !defined(BLAKE3_NO_AVX512)
   if (features & AVX512VL) {
      blake3_compress_in_place_avx512(cv, block, block_len, counter, flags);
      }
   #endif
   #if !defined(BLAKE3_NO_SSE41)
   if (features & SSE41) {
      blake3_compress_in_place_sse41(cv, block, block_len, counter, flags);
      }
   #endif
   #if !defined(BLAKE3_NO_SSE2)
   if (features & SSE2) {
      blake3_compress_in_place_sse2(cv, block, block_len, counter, flags);
      }
   #endif
   #endif
   blake3_compress_in_place_portable(cv, block, block_len, counter, flags);
   }
      void blake3_compress_xof(const uint32_t cv[8],
                     #if defined(IS_X86)
   const enum cpu_feature features = get_cpu_features();
   MAYBE_UNUSED(features);
   #if !defined(BLAKE3_NO_AVX512)
   if (features & AVX512VL) {
      blake3_compress_xof_avx512(cv, block, block_len, counter, flags, out);
      }
   #endif
   #if !defined(BLAKE3_NO_SSE41)
   if (features & SSE41) {
      blake3_compress_xof_sse41(cv, block, block_len, counter, flags, out);
      }
   #endif
   #if !defined(BLAKE3_NO_SSE2)
   if (features & SSE2) {
      blake3_compress_xof_sse2(cv, block, block_len, counter, flags, out);
      }
   #endif
   #endif
   blake3_compress_xof_portable(cv, block, block_len, counter, flags, out);
   }
      void blake3_hash_many(const uint8_t *const *inputs, size_t num_inputs,
                     #if defined(IS_X86)
   const enum cpu_feature features = get_cpu_features();
   MAYBE_UNUSED(features);
   #if !defined(BLAKE3_NO_AVX512)
   if ((features & (AVX512F|AVX512VL)) == (AVX512F|AVX512VL)) {
      blake3_hash_many_avx512(inputs, num_inputs, blocks, key, counter,
                  }
   #endif
   #if !defined(BLAKE3_NO_AVX2)
   if (features & AVX2) {
      blake3_hash_many_avx2(inputs, num_inputs, blocks, key, counter,
                  }
   #endif
   #if !defined(BLAKE3_NO_SSE41)
   if (features & SSE41) {
      blake3_hash_many_sse41(inputs, num_inputs, blocks, key, counter,
                  }
   #endif
   #if !defined(BLAKE3_NO_SSE2)
   if (features & SSE2) {
      blake3_hash_many_sse2(inputs, num_inputs, blocks, key, counter,
                  }
   #endif
   #endif
      #if BLAKE3_USE_NEON == 1
   blake3_hash_many_neon(inputs, num_inputs, blocks, key, counter,
         return;
   #endif
      blake3_hash_many_portable(inputs, num_inputs, blocks, key, counter,
               }
      // The dynamically detected SIMD degree of the current platform.
   size_t blake3_simd_degree(void) {
   #if defined(IS_X86)
   const enum cpu_feature features = get_cpu_features();
   MAYBE_UNUSED(features);
   #if !defined(BLAKE3_NO_AVX512)
   if ((features & (AVX512F|AVX512VL)) == (AVX512F|AVX512VL)) {
         }
   #endif
   #if !defined(BLAKE3_NO_AVX2)
   if (features & AVX2) {
         }
   #endif
   #if !defined(BLAKE3_NO_SSE41)
   if (features & SSE41) {
         }
   #endif
   #if !defined(BLAKE3_NO_SSE2)
   if (features & SSE2) {
         }
   #endif
   #endif
   #if BLAKE3_USE_NEON == 1
   return 4;
   #endif
   return 1;
   }
