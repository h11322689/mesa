   /**************************************************************************
   *
   * Copyright 2019 Red Hat.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **************************************************************************/
      /**
   * compute shader thread pool.
   * based on threadpool.c but modified heavily to be compute shader tuned.
   */
      #include "util/u_thread.h"
   #include "util/u_memory.h"
   #include "lp_cs_tpool.h"
      static int
   lp_cs_tpool_worker(void *data)
   {
      struct lp_cs_tpool *pool = data;
            memset(&lmem, 0, sizeof(lmem));
            while (!pool->shutdown) {
      struct lp_cs_tpool_task *task;
            while (list_is_empty(&pool->workqueue) && !pool->shutdown)
            if (pool->shutdown)
            task = list_first_entry(&pool->workqueue, struct lp_cs_tpool_task,
                              if (task->iter_remainder &&
      task->iter_start + task->iter_remainder == task->iter_total) {
   task->iter_remainder--;
                        if (task->iter_start == task->iter_total)
            mtx_unlock(&pool->m);
   for (unsigned i = 0; i < iter_per_thread; i++)
            mtx_lock(&pool->m);
   task->iter_finished += iter_per_thread;
   if (task->iter_finished == task->iter_total)
      }
   mtx_unlock(&pool->m);
   FREE(lmem.local_mem_ptr);
      }
      struct lp_cs_tpool *
   lp_cs_tpool_create(unsigned num_threads)
   {
               if (!pool)
            (void) mtx_init(&pool->m, mtx_plain);
            list_inithead(&pool->workqueue);
   assert (num_threads <= LP_MAX_THREADS);
   for (unsigned i = 0; i < num_threads; i++) {
      if (thrd_success != u_thread_create(pool->threads + i, lp_cs_tpool_worker, pool)) {
      num_threads = i;  /* previous thread is max */
         }
   pool->num_threads = num_threads;
      }
      void
   lp_cs_tpool_destroy(struct lp_cs_tpool *pool)
   {
      if (!pool)
            mtx_lock(&pool->m);
   pool->shutdown = true;
   cnd_broadcast(&pool->new_work);
            for (unsigned i = 0; i < pool->num_threads; i++) {
                  cnd_destroy(&pool->new_work);
   mtx_destroy(&pool->m);
      }
      struct lp_cs_tpool_task *
   lp_cs_tpool_queue_task(struct lp_cs_tpool *pool,
         {
               if (pool->num_threads == 0) {
               memset(&lmem, 0, sizeof(lmem));
   for (unsigned t = 0; t < num_iters; t++) {
         }
   FREE(lmem.local_mem_ptr);
      }
   task = CALLOC_STRUCT(lp_cs_tpool_task);
   if (!task) {
                  task->work = work;
   task->data = data;
            task->iter_per_thread = num_iters / pool->num_threads;
                                       cnd_broadcast(&pool->new_work);
   mtx_unlock(&pool->m);
      }
      void
   lp_cs_tpool_wait_for_task(struct lp_cs_tpool *pool,
         {
               if (!pool || !task)
            mtx_lock(&pool->m);
   while (task->iter_finished < task->iter_total)
                  cnd_destroy(&task->finish);
   FREE(task);
      }
