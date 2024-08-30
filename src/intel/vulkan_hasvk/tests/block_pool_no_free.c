   /*
   * Copyright © 2015 Intel Corporation
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
      #include <pthread.h>
      #include "anv_private.h"
   #include "test_common.h"
      #define NUM_THREADS 16
   #define BLOCKS_PER_THREAD 1024
   #define NUM_RUNS 64
      static struct job {
      pthread_t thread;
   unsigned id;
   struct anv_block_pool *pool;
   int32_t blocks[BLOCKS_PER_THREAD];
      } jobs[NUM_THREADS];
         static void *alloc_blocks(void *_job)
   {
      struct job *job = _job;
   uint32_t job_id = job - jobs;
   uint32_t block_size = 16 * ((job_id % 4) + 1);
            for (unsigned i = 0; i < BLOCKS_PER_THREAD; i++) {
      block = anv_block_pool_alloc(job->pool, block_size, NULL);
   data = anv_block_pool_map(job->pool, block, block_size);
   *data = block;
   ASSERT(block >= 0);
            block = anv_block_pool_alloc_back(job->pool, block_size);
   data = anv_block_pool_map(job->pool, block, block_size);
   *data = block;
   ASSERT(block < 0);
               for (unsigned i = 0; i < BLOCKS_PER_THREAD; i++) {
      block = job->blocks[i];
   data = anv_block_pool_map(job->pool, block, block_size);
            block = -job->back_blocks[i];
   data = anv_block_pool_map(job->pool, block, block_size);
                  }
      static void validate_monotonic(int32_t **blocks)
   {
      /* A list of indices, one per thread */
   unsigned next[NUM_THREADS];
            int highest = -1;
   while (true) {
      /* First, we find which thread has the lowest next element */
   int32_t thread_min = INT32_MAX;
   int min_thread_idx = -1;
   for (unsigned i = 0; i < NUM_THREADS; i++) {
                     if (thread_min > blocks[i][next[i]]) {
      thread_min = blocks[i][next[i]];
                  /* The only way this can happen is if all of the next[] values are at
   * BLOCKS_PER_THREAD, in which case, we're done.
   */
   if (thread_min == INT32_MAX)
            /* That next element had better be higher than the previous highest */
            highest = blocks[min_thread_idx][next[min_thread_idx]];
         }
      static void run_test()
   {
      struct anv_physical_device physical_device = {
         };
   struct anv_device device = {};
            anv_device_set_physical(&device, &physical_device);
   pthread_mutex_init(&device.mutex, NULL);
   anv_bo_cache_init(&device.bo_cache, &device);
            for (unsigned i = 0; i < NUM_THREADS; i++) {
      jobs[i].pool = &pool;
   jobs[i].id = i;
               for (unsigned i = 0; i < NUM_THREADS; i++)
            /* Validate that the block allocations were monotonic */
   int32_t *block_ptrs[NUM_THREADS];
   for (unsigned i = 0; i < NUM_THREADS; i++)
                  /* Validate that the back block allocations were monotonic */
   for (unsigned i = 0; i < NUM_THREADS; i++)
                  anv_block_pool_finish(&pool);
   anv_bo_cache_finish(&device.bo_cache);
      }
      void block_pool_no_free_test(void);
      void block_pool_no_free_test(void)
   {
      for (unsigned i = 0; i < NUM_RUNS; i++)
      }
