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
      #include "state_pool_test_helper.h"
      void state_pool_free_list_only_test(void);
      void state_pool_free_list_only_test(void)
   {
      const unsigned num_threads = 8;
            struct anv_physical_device physical_device = { };
   struct anv_device device = {};
            test_device_info_init(&physical_device.info);
   anv_device_set_physical(&device, &physical_device);
   device.kmd_backend = anv_kmd_backend_get(INTEL_KMD_TYPE_STUB);
   pthread_mutex_init(&device.mutex, NULL);
   anv_bo_cache_init(&device.bo_cache, &device);
            /* Grab one so a zero offset is impossible */
            /* Grab and return enough states that the state pool test below won't
   * actually ever resize anything.
   */
   {
      struct anv_state states[num_threads * states_per_thread];
   for (unsigned i = 0; i < ARRAY_SIZE(states); i++) {
      states[i] = anv_state_pool_alloc(&state_pool, 16, 16);
               for (unsigned i = 0; i < ARRAY_SIZE(states); i++)
                        anv_state_pool_finish(&state_pool);
   anv_bo_cache_finish(&device.bo_cache);
      }
