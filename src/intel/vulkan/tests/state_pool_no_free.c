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
   #define STATES_PER_THREAD 1024
   #define NUM_RUNS 64
      static struct job {
      pthread_t thread;
   unsigned id;
   struct anv_state_pool *pool;
      } jobs[NUM_THREADS];
      static pthread_barrier_t barrier;
      static void *alloc_states(void *_job)
   {
                        for (unsigned i = 0; i < STATES_PER_THREAD; i++) {
      struct anv_state state = anv_state_pool_alloc(job->pool, 16, 16);
                  }
      static void run_test()
   {
      struct anv_physical_device physical_device = { };
   struct anv_device device = {};
            test_device_info_init(&physical_device.info);
   anv_device_set_physical(&device, &physical_device);
   device.kmd_backend = anv_kmd_backend_get(INTEL_KMD_TYPE_STUB);
   pthread_mutex_init(&device.mutex, NULL);
   anv_bo_cache_init(&device.bo_cache, &device);
                     for (unsigned i = 0; i < NUM_THREADS; i++) {
      jobs[i].pool = &state_pool;
   jobs[i].id = i;
               for (unsigned i = 0; i < NUM_THREADS; i++)
            /* A list of indices, one per thread */
   unsigned next[NUM_THREADS];
            int highest = -1;
   while (true) {
      /* First, we find which thread has the highest next element */
   int thread_max = -1;
   int max_thread_idx = -1;
   for (unsigned i = 0; i < NUM_THREADS; i++) {
                     if (thread_max < jobs[i].offsets[next[i]]) {
      thread_max = jobs[i].offsets[next[i]];
                  /* The only way this can happen is if all of the next[] values are at
   * BLOCKS_PER_THREAD, in which case, we're done.
   */
   if (thread_max == -1)
            /* That next element had better be higher than the previous highest */
            highest = jobs[max_thread_idx].offsets[next[max_thread_idx]];
               anv_state_pool_finish(&state_pool);
   anv_bo_cache_finish(&device.bo_cache);
      }
      void state_pool_no_free_test(void);
      void state_pool_no_free_test(void)
   {
      for (unsigned i = 0; i < NUM_RUNS; i++)
      }
