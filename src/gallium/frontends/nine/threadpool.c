   /*
   * Copyright © 2012 Intel Corporation
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
      #include "swapchain9.h"
   #include "surface9.h"
   #include "device9.h"
      #include "nine_helpers.h"
   #include "nine_pipe.h"
   #include "nine_dump.h"
      #include "util/u_inlines.h"
   #include "util/u_surface.h"
   #include "hud/hud_context.h"
   #include "frontend/drm_driver.h"
      #include "util/u_thread.h"
   #include "threadpool.h"
      /* POSIX thread function */
   static void *
   threadpool_worker(void *data)
   {
                        while (!pool->shutdown) {
               /* Block (dropping the lock) until new work arrives for us. */
   while (!pool->workqueue && !pool->shutdown)
            if (pool->shutdown)
            /* Pull the first task from the list.  We don't free it -- it now lacks
      * a reference other than the worker creator's, whose responsibility it
   * is to call threadpool_wait_for_work() to free it.
      task = pool->workqueue;
            /* Call the task's work func. */
   pthread_mutex_unlock(&pool->m);
   task->work(task->data);
   pthread_mutex_lock(&pool->m);
   task->finished = true;
                           }
      /* Windows thread function */
   static DWORD NINE_WINAPI
   wthreadpool_worker(void *data)
   {
                  }
      struct threadpool *
   _mesa_threadpool_create(struct NineSwapChain9 *swapchain)
   {
               if (!pool)
            pthread_mutex_init(&pool->m, NULL);
            /* This uses WINE's CreateThread, so the thread function needs to use
   * the Windows ABI */
   pool->wthread = NineSwapChain9_CreateThread(swapchain, wthreadpool_worker, pool);
   if (!pool->wthread) {
      /* using pthread as fallback */
      }
      }
      void
   _mesa_threadpool_destroy(struct NineSwapChain9 *swapchain, struct threadpool *pool)
   {
      if (!pool)
            pthread_mutex_lock(&pool->m);
   pool->shutdown = true;
   pthread_cond_broadcast(&pool->new_work);
            if (pool->wthread) {
         } else {
                  pthread_cond_destroy(&pool->new_work);
   pthread_mutex_destroy(&pool->m);
      }
      /**
   * Queues a request for the work function to be asynchronously executed by the
   * thread pool.
   *
   * The work func will get the "data" argument as its parameter -- any
   * communication between the caller and the work function will occur through
   * that.
   *
   * If there is an error, the work function is called immediately and NULL is
   * returned.
   */
   struct threadpool_task *
   _mesa_threadpool_queue_task(struct threadpool *pool,
         {
               if (!pool) {
      work(data);
               task = calloc(1, sizeof(*task));
   if (!task) {
      work(data);
               task->work = work;
   task->data = data;
   task->next = NULL;
                     if (!pool->workqueue) {
         } else {
      previous = pool->workqueue;
   while (previous && previous->next)
               }
   pthread_cond_signal(&pool->new_work);
               }
      /**
   * Blocks on the completion of the given task and frees the task.
   */
   void
   _mesa_threadpool_wait_for_task(struct threadpool *pool,
         {
               if (!pool || !task)
            pthread_mutex_lock(&pool->m);
   while (!task->finished)
                  pthread_cond_destroy(&task->finish);
   free(task);
      }
