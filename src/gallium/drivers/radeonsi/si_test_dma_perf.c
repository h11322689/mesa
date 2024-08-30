   /*
   * Copyright 2018 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      /* This file implements tests on the si_clearbuffer function. */
      #include "si_pipe.h"
   #include "si_query.h"
      #define MIN_SIZE   512
   #define MAX_SIZE   (128 * 1024 * 1024)
   #define SIZE_SHIFT 1
   #define NUM_RUNS   128
      static double get_MBps_rate(unsigned num_bytes, unsigned ns)
   {
         }
      void si_test_dma_perf(struct si_screen *sscreen)
   {
      struct pipe_screen *screen = &sscreen->b;
   struct pipe_context *ctx = screen->context_create(screen, NULL, 0);
   struct si_context *sctx = (struct si_context *)ctx;
   const uint32_t clear_value = 0x12345678;
   static const unsigned cs_dwords_per_thread_list[] = {64, 32, 16, 8, 4, 2, 1};
         #define NUM_SHADERS ARRAY_SIZE(cs_dwords_per_thread_list)
   #define NUM_METHODS (3 + 3 * NUM_SHADERS * ARRAY_SIZE(cs_waves_per_sh_list))
         static const char *method_str[] = {
      "CP MC   ",
   "CP L2   ",
      };
   static const char *placement_str[] = {
      /* Clear */
   "fill->VRAM",
   "fill->GTT ",
   /* Copy */
   "VRAM->VRAM",
   "VRAM->GTT ",
               printf("DMA rate is in MB/s for each size. Slow cases are skipped and print 0.\n");
   printf("Heap       ,Method  ,L2p,Wa,");
   for (unsigned size = MIN_SIZE; size <= MAX_SIZE; size <<= SIZE_SHIFT) {
      if (size >= 1024)
         else
      }
            /* results[log2(size)][placement][method][] */
   struct si_result {
      bool is_valid;
   bool is_cp;
   bool is_cs;
   unsigned cache_policy;
   unsigned dwords_per_thread;
   unsigned waves_per_sh;
   unsigned score;
               /* Run benchmarks. */
   for (unsigned placement = 0; placement < ARRAY_SIZE(placement_str); placement++) {
               printf("-----------,--------,---,--,");
   for (unsigned size = MIN_SIZE; size <= MAX_SIZE; size <<= SIZE_SHIFT)
                  for (unsigned method = 0; method < NUM_METHODS; method++) {
      bool test_cp = method <= 2;
   bool test_cs = method >= 3;
   unsigned cs_method = method - 3;
   unsigned cs_waves_per_sh =
         cs_method %= 3 * NUM_SHADERS;
   unsigned cache_policy =
                        if (sctx->gfx_level == GFX6) {
      /* GFX6 doesn't support CP DMA operations through L2. */
   if (test_cp && cache_policy != L2_BYPASS)
         /* WAVES_PER_SH is in multiples of 16 on GFX6. */
   if (test_cs && cs_waves_per_sh % 16 != 0)
               /* SI_RESOURCE_FLAG_GL2_BYPASS setting RADEON_FLAG_GL2_BYPASS doesn't affect
   * chips before gfx9.
   */
                  printf("%s ,", placement_str[placement]);
   if (test_cs) {
      printf("CS x%-4u,%3s,", cs_dwords_per_thread,
      } else {
      printf("%s,%3s,", method_str[method],
      }
   if (test_cs && cs_waves_per_sh)
                        void *compute_shader = NULL;
   if (test_cs) {
      compute_shader = si_create_dma_compute_shader(ctx, cs_dwords_per_thread,
               double score = 0;
   for (unsigned size = MIN_SIZE; size <= MAX_SIZE; size <<= SIZE_SHIFT) {
      /* Don't test bigger sizes if it's too slow. Print 0. */
   if (size >= 512 * 1024 && score < 400 * (size / (4 * 1024 * 1024))) {
                        enum pipe_resource_usage dst_usage, src_usage;
   struct pipe_resource *dst, *src;
                  if (placement == 0 || placement == 2 || placement == 4)
                        if (placement == 2 || placement == 3)
                                       /* Wait for idle before testing, so that other processes don't mess up the results. */
   sctx->flags |= SI_CONTEXT_CS_PARTIAL_FLUSH |
                                       /* Run tests. */
   for (unsigned iter = 0; iter < NUM_RUNS; iter++) {
      if (test_cp) {
      /* CP DMA */
   if (is_copy) {
      si_cp_dma_copy_buffer(sctx, dst, src, 0, 0, size, SI_OP_SYNC_BEFORE_AFTER,
      } else {
      si_cp_dma_clear_buffer(sctx, &sctx->gfx_cs, dst, 0, size, clear_value,
               } else {
      /* Compute */
   /* The memory accesses are coalesced, meaning that the 1st instruction writes
   * the 1st contiguous block of data for the whole wave, the 2nd instruction
   * writes the 2nd contiguous block of data, etc.
   */
                                       struct pipe_grid_info info = {};
   info.block[0] = MIN2(64, num_instructions);
   info.block[1] = 1;
   info.block[2] = 1;
                                             if (is_copy) {
      sb[1].buffer = src;
      } else {
                                                                        /* Flush L2, so that we don't just test L2 cache performance except for L2_LRU. */
   sctx->flags |= SI_CONTEXT_INV_VCACHE |
                                                                                          score = get_MBps_rate(size, result.u64 / (double)NUM_RUNS);
                  struct si_result *r = &results[util_logbase2(size)][placement][method];
   r->is_valid = true;
   r->is_cp = test_cp;
   r->is_cs = test_cs;
   r->cache_policy = cache_policy;
   r->dwords_per_thread = cs_dwords_per_thread;
   r->waves_per_sh = cs_waves_per_sh;
   r->score = score;
                     if (compute_shader)
                  puts("");
   puts("static struct si_method");
   printf("get_best_clear_for_%s(enum radeon_bo_domain dst, uint64_t size64, bool async, bool "
         "cached)\n",
   puts("{");
            /* Analyze results and find the best methods. */
   for (unsigned placement = 0; placement < ARRAY_SIZE(placement_str); placement++) {
      if (placement == 0)
         else if (placement == 1)
         else if (placement == 2) {
      puts("}");
   puts("");
   puts("static struct si_method");
   printf("get_best_copy_for_%s(enum radeon_bo_domain dst, enum radeon_bo_domain src,\n",
         printf("                     uint64_t size64, bool async, bool cached)\n");
   puts("{");
   puts("   unsigned size = MIN2(size64, UINT_MAX);\n");
      } else if (placement == 3)
         else
            for (unsigned mode = 0; mode < 3; mode++) {
                     if (async)
         else if (cached)
                        /* The list of best chosen methods. */
   struct si_result *methods[32];
                  for (unsigned size = MIN_SIZE; size <= MAX_SIZE; size <<= SIZE_SHIFT) {
                                                      /* Ban CP DMA clears via MC on <= GFX8. They are super slow
   * on GTT, which we can get due to BO evictions.
   */
                        if (async) {
      /* The following constraints for compute IBs try to limit
                        /* Don't use CP DMA on asynchronous rings, because
   * the engine is shared with gfx IBs.
                        /* Don't use L2 caching on asynchronous rings to minimize
   * L2 usage.
                        /* Asynchronous compute recommends waves_per_sh != 0
   * to limit CU usage. */
   if (r->is_cs && r->waves_per_sh == 0)
      } else {
      if (cached && r->cache_policy == L2_BYPASS)
                           if (!best) {
                        /* Assume some measurement error. Earlier methods occupy fewer
   * resources, so the next method is always more greedy, and we
                                          if (num_methods > 0) {
      unsigned prev_index = num_methods - 1;
                        /* If the best one is also the best for the previous size,
   * just bump the size for the previous one.
   *
   * If there is no best, it means all methods were too slow
   * for this size and were not tested. Use the best one for
   * the previous size.
   */
   if (!best ||
      /* If it's the same method as for the previous size: */
   (prev->is_cp == best->is_cp &&
   prev->is_cs == best->is_cs && prev->cache_policy == best->cache_policy &&
   prev->dwords_per_thread == best->dwords_per_thread &&
   prev->waves_per_sh == best->waves_per_sh) ||
   /* If the method for the previous size is also the best
   * for this size: */
   (prev_this_size->is_valid && prev_this_size->score * 1.03 > best->score)) {
   method_max_size[prev_index] = size;
                  /* Add it to the list. */
   assert(num_methods < ARRAY_SIZE(methods));
   methods[num_methods] = best;
   method_max_size[num_methods] = size;
               for (unsigned i = 0; i < num_methods; i++) {
                     /* The size threshold is between the current benchmarked
   * size and the next benchmarked size. */
   if (i < num_methods - 1)
         else if (i > 0)
         else
                  assert(best);
   const char *cache_policy_str =
                  if (best->is_cp) {
         }
   if (best->is_cs) {
      printf("COMPUTE(%s, %u, %u);\n", cache_policy_str,
            }
      }
   puts("   }");
            ctx->destroy(ctx);
      }
