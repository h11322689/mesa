   /**************************************************************************
   *
   * Copyright 2009-2010 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
         /*
   *  Test case for util_barrier.
   *
   *  The test succeeds if no thread exits before all the other threads reach
   *  the barrier.
   */
         #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
      #include "util/os_time.h"
   #include "util/u_atomic.h"
   #include "util/u_thread.h"
         #define NUM_THREADS 10
      static int verbosity = 0;
      static thrd_t threads[NUM_THREADS];
   static util_barrier barrier;
   static int thread_ids[NUM_THREADS];
      static volatile int waiting = 0;
   static volatile int proceeded = 0;
         #define LOG(fmt, ...) \
      if (verbosity > 0) { \
               #define CHECK(_cond) \
      if (!(_cond)) { \
      fprintf(stderr, "%s:%u: `%s` failed\n", __FILE__, __LINE__, #_cond); \
               static int
   thread_function(void *thread_data)
   {
               LOG("thread %d starting\n", thread_id);
   os_time_sleep(thread_id * 100 * 1000);
            CHECK(p_atomic_read(&proceeded) == 0);
                                                   }
         int main(int argc, char *argv[])
   {
               for (i = 1; i < argc; ++i) {
      const char *arg = argv[i];
   if (strcmp(arg, "-v") == 0) {
         } else {
      fprintf(stderr, "error: unrecognized option `%s`\n", arg);
                  // Disable buffering
                              for (i = 0; i < NUM_THREADS; i++) {
      thread_ids[i] = i;
               for (i = 0; i < NUM_THREADS; i++ ) {
                                                }
