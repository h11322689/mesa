   /*
   * Copyright © 2016 Advanced Micro Devices, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining
   * a copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
   * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   */
      #include "u_queue.h"
      #include "c11/threads.h"
   #include "util/u_cpu_detect.h"
   #include "util/os_time.h"
   #include "util/u_string.h"
   #include "util/u_thread.h"
   #include "u_process.h"
      #if defined(__linux__)
   #include <sys/time.h>
   #include <sys/resource.h>
   #include <sys/syscall.h>
   #endif
         /* Define 256MB */
   #define S_256MB (256 * 1024 * 1024)
      static void
   util_queue_kill_threads(struct util_queue *queue, unsigned keep_num_threads,
            /****************************************************************************
   * Wait for all queues to assert idle when exit() is called.
   *
   * Otherwise, C++ static variable destructors can be called while threads
   * are using the static variables.
   */
      static once_flag atexit_once_flag = ONCE_FLAG_INIT;
   static struct list_head queue_list = {
      .next = &queue_list,
      };
   static mtx_t exit_mutex;
      static void
   atexit_handler(void)
   {
               mtx_lock(&exit_mutex);
   /* Wait for all queues to assert idle. */
   LIST_FOR_EACH_ENTRY(iter, &queue_list, head) {
         }
      }
      static void
   global_init(void)
   {
      mtx_init(&exit_mutex, mtx_plain);
      }
      static void
   add_to_atexit_list(struct util_queue *queue)
   {
               mtx_lock(&exit_mutex);
   list_add(&queue->head, &queue_list);
      }
      static void
   remove_from_atexit_list(struct util_queue *queue)
   {
               mtx_lock(&exit_mutex);
   LIST_FOR_EACH_ENTRY_SAFE(iter, tmp, &queue_list, head) {
      if (iter == queue) {
      list_del(&iter->head);
         }
      }
      /****************************************************************************
   * util_queue_fence
   */
      #ifdef UTIL_QUEUE_FENCE_FUTEX
   static bool
   do_futex_fence_wait(struct util_queue_fence *fence,
         {
      uint32_t v = p_atomic_read_relaxed(&fence->val);
   struct timespec ts;
   ts.tv_sec = abs_timeout / (1000*1000*1000);
            while (v != 0) {
      if (v != 2) {
      v = p_atomic_cmpxchg(&fence->val, 1, 2);
   if (v == 0)
               int r = futex_wait(&fence->val, 2, timeout ? &ts : NULL);
   if (timeout && r < 0) {
      if (errno == ETIMEDOUT)
                              }
      void
   _util_queue_fence_wait(struct util_queue_fence *fence)
   {
         }
      bool
   _util_queue_fence_wait_timeout(struct util_queue_fence *fence,
         {
         }
      #endif
      #ifdef UTIL_QUEUE_FENCE_STANDARD
   void
   util_queue_fence_signal(struct util_queue_fence *fence)
   {
      mtx_lock(&fence->mutex);
   fence->signalled = true;
   cnd_broadcast(&fence->cond);
      }
      void
   _util_queue_fence_wait(struct util_queue_fence *fence)
   {
      mtx_lock(&fence->mutex);
   while (!fence->signalled)
            }
      bool
   _util_queue_fence_wait_timeout(struct util_queue_fence *fence,
         {
      /* This terrible hack is made necessary by the fact that we really want an
   * internal interface consistent with os_time_*, but cnd_timedwait is spec'd
   * to be relative to the TIME_UTC clock.
   */
            if (rel > 0) {
                        ts.tv_sec += abs_timeout / (1000*1000*1000);
   ts.tv_nsec += abs_timeout % (1000*1000*1000);
   if (ts.tv_nsec >= (1000*1000*1000)) {
      ts.tv_sec++;
               mtx_lock(&fence->mutex);
   while (!fence->signalled) {
      if (cnd_timedwait(&fence->cond, &fence->mutex, &ts) != thrd_success)
      }
                  }
      void
   util_queue_fence_init(struct util_queue_fence *fence)
   {
      memset(fence, 0, sizeof(*fence));
   (void) mtx_init(&fence->mutex, mtx_plain);
   cnd_init(&fence->cond);
      }
      void
   util_queue_fence_destroy(struct util_queue_fence *fence)
   {
               /* Ensure that another thread is not in the middle of
   * util_queue_fence_signal (having set the fence to signalled but still
   * holding the fence mutex).
   *
   * A common contract between threads is that as soon as a fence is signalled
   * by thread A, thread B is allowed to destroy it. Since
   * util_queue_fence_is_signalled does not lock the fence mutex (for
   * performance reasons), we must do so here.
   */
   mtx_lock(&fence->mutex);
            cnd_destroy(&fence->cond);
      }
   #endif
      /****************************************************************************
   * util_queue implementation
   */
      struct thread_input {
      struct util_queue *queue;
      };
      static int
   util_queue_thread_func(void *input)
   {
      struct util_queue *queue = ((struct thread_input*)input)->queue;
                     if (queue->flags & UTIL_QUEUE_INIT_SET_FULL_THREAD_AFFINITY) {
      /* Don't inherit the thread affinity from the parent thread.
   * Set the full mask.
   */
                     util_set_current_thread_affinity(mask, NULL,
            #if defined(__linux__)
      if (queue->flags & UTIL_QUEUE_INIT_USE_MINIMUM_PRIORITY) {
      /* The nice() function can only set a maximum of 19. */
         #endif
         if (strlen(queue->name) > 0) {
      char name[16];
   snprintf(name, sizeof(name), "%s%i", queue->name, thread_index);
               while (1) {
               mtx_lock(&queue->lock);
            /* wait if the queue is empty */
   while (thread_index < queue->num_threads && queue->num_queued == 0)
            /* only kill threads that are above "num_threads" */
   if (thread_index >= queue->num_threads) {
      mtx_unlock(&queue->lock);
               job = queue->jobs[queue->read_idx];
   memset(&queue->jobs[queue->read_idx], 0, sizeof(struct util_queue_job));
            queue->num_queued--;
   cnd_signal(&queue->has_space_cond);
   if (job.job)
                  if (job.job) {
      job.execute(job.job, job.global_data, thread_index);
   if (job.fence)
         if (job.cleanup)
                  /* signal remaining jobs if all threads are being terminated */
   mtx_lock(&queue->lock);
   if (queue->num_threads == 0) {
      for (unsigned i = queue->read_idx; i != queue->write_idx;
      i = (i + 1) % queue->max_jobs) {
   if (queue->jobs[i].job) {
      if (queue->jobs[i].fence)
               }
   queue->read_idx = queue->write_idx;
      }
   mtx_unlock(&queue->lock);
      }
      static bool
   util_queue_create_thread(struct util_queue *queue, unsigned index)
   {
      struct thread_input *input =
         input->queue = queue;
            if (thrd_success != u_thread_create(queue->threads + index, util_queue_thread_func, input)) {
      free(input);
                  #if defined(__linux__) && defined(SCHED_BATCH)
                  /* The nice() function can only set a maximum of 19.
   * SCHED_BATCH gives the scheduler a hint that this is a latency
   * insensitive thread.
   *
   * Note that Linux only allows decreasing the priority. The original
   * priority can't be restored.
   */
   #endif
      }
      }
      void
   util_queue_adjust_num_threads(struct util_queue *queue, unsigned num_threads,
         {
      num_threads = MIN2(num_threads, queue->max_threads);
            if (!locked)
                     if (num_threads == old_num_threads) {
      if (!locked)
                     if (num_threads < old_num_threads) {
      util_queue_kill_threads(queue, num_threads, true);
   if (!locked)
                     /* Create threads.
   *
   * We need to update num_threads first, because threads terminate
   * when thread_index < num_threads.
   */
   queue->num_threads = num_threads;
   for (unsigned i = old_num_threads; i < num_threads; i++) {
      if (!util_queue_create_thread(queue, i)) {
      queue->num_threads = i;
                  if (!locked)
      }
      bool
   util_queue_init(struct util_queue *queue,
                  const char *name,
   unsigned max_jobs,
      {
               /* Form the thread name from process_name and name, limited to 13
   * characters. Characters 14-15 are reserved for the thread number.
   * Character 16 should be 0. Final form: "process:name12"
   *
   * If name is too long, it's truncated. If any space is left, the process
   * name fills it.
   */
   const char *process_name = util_get_process_name();
   int process_len = process_name ? strlen(process_name) : 0;
   int name_len = strlen(name);
                     /* See if there is any space left for the process name, reserve 1 for
   * the colon. */
   process_len = MIN2(process_len, max_chars - name_len - 1);
                     if (process_len) {
      snprintf(queue->name, sizeof(queue->name), "%.*s:%s",
      } else {
                  queue->create_threads_on_demand = true;
   queue->flags = flags;
   queue->max_threads = num_threads;
   queue->num_threads = 1;
   queue->max_jobs = max_jobs;
                     queue->num_queued = 0;
   cnd_init(&queue->has_queued_cond);
            queue->jobs = (struct util_queue_job*)
         if (!queue->jobs)
            queue->threads = (thrd_t*) calloc(queue->max_threads, sizeof(thrd_t));
   if (!queue->threads)
            /* start threads */
   for (i = 0; i < queue->num_threads; i++) {
      if (!util_queue_create_thread(queue, i)) {
      if (i == 0) {
      /* no threads created, fail */
      } else {
      /* at least one thread created, so use it */
   queue->num_threads = i;
                     add_to_atexit_list(queue);
         fail:
               if (queue->jobs) {
      cnd_destroy(&queue->has_space_cond);
   cnd_destroy(&queue->has_queued_cond);
   mtx_destroy(&queue->lock);
      }
   /* also util_queue_is_initialized can be used to check for success */
   memset(queue, 0, sizeof(*queue));
      }
      static void
   util_queue_kill_threads(struct util_queue *queue, unsigned keep_num_threads,
         {
      /* Signal all threads to terminate. */
   if (!locked)
            if (keep_num_threads >= queue->num_threads) {
      if (!locked)
                     unsigned old_num_threads = queue->num_threads;
   /* Setting num_threads is what causes the threads to terminate.
   * Then cnd_broadcast wakes them up and they will exit their function.
   */
   queue->num_threads = keep_num_threads;
            /* Wait for threads to terminate. */
   if (keep_num_threads < old_num_threads) {
      /* We need to unlock the mutex to allow threads to terminate. */
   mtx_unlock(&queue->lock);
   for (unsigned i = keep_num_threads; i < old_num_threads; i++)
         if (locked)
      } else {
      if (!locked)
         }
      static void
   util_queue_finish_execute(void *data, void *gdata, int num_thread)
   {
      util_barrier *barrier = data;
   if (util_barrier_wait(barrier))
      }
      void
   util_queue_destroy(struct util_queue *queue)
   {
               /* This makes it safe to call on a queue that failed util_queue_init. */
   if (queue->head.next != NULL)
            cnd_destroy(&queue->has_space_cond);
   cnd_destroy(&queue->has_queued_cond);
   mtx_destroy(&queue->lock);
   free(queue->jobs);
      }
      static void
   util_queue_add_job_locked(struct util_queue *queue,
                           void *job,
   struct util_queue_fence *fence,
   {
               if (!locked)
         if (queue->num_threads == 0) {
      if (!locked)
         /* well no good option here, but any leaks will be
   * short-lived as things are shutting down..
   */
               if (fence)
                     /* Scale the number of threads up if there's already one job waiting. */
   if (queue->num_queued > 0 &&
      queue->create_threads_on_demand &&
   execute != util_queue_finish_execute &&
   queue->num_threads < queue->max_threads) {
               if (queue->num_queued == queue->max_jobs) {
      if (queue->flags & UTIL_QUEUE_INIT_RESIZE_IF_FULL &&
      queue->total_jobs_size + job_size < S_256MB) {
   /* If the queue is full, make it larger to avoid waiting for a free
   * slot.
   */
   unsigned new_max_jobs = queue->max_jobs + 8;
   struct util_queue_job *jobs =
      (struct util_queue_job*)calloc(new_max_jobs,
               /* Copy all queued jobs into the new list. */
                  do {
      jobs[num_jobs++] = queue->jobs[i];
                        free(queue->jobs);
   queue->jobs = jobs;
   queue->read_idx = 0;
   queue->write_idx = num_jobs;
      } else {
      /* Wait until there is a free slot. */
   while (queue->num_queued == queue->max_jobs)
                  ptr = &queue->jobs[queue->write_idx];
   assert(ptr->job == NULL);
   ptr->job = job;
   ptr->global_data = queue->global_data;
   ptr->fence = fence;
   ptr->execute = execute;
   ptr->cleanup = cleanup;
            queue->write_idx = (queue->write_idx + 1) % queue->max_jobs;
            queue->num_queued++;
   cnd_signal(&queue->has_queued_cond);
   if (!locked)
      }
      void
   util_queue_add_job(struct util_queue *queue,
                     void *job,
   struct util_queue_fence *fence,
   {
      util_queue_add_job_locked(queue, job, fence, execute, cleanup, job_size,
      }
      /**
   * Remove a queued job. If the job hasn't started execution, it's removed from
   * the queue. If the job has started execution, the function waits for it to
   * complete.
   *
   * In all cases, the fence is signalled when the function returns.
   *
   * The function can be used when destroying an object associated with the job
   * when you don't care about the job completion state.
   */
   void
   util_queue_drop_job(struct util_queue *queue, struct util_queue_fence *fence)
   {
               if (util_queue_fence_is_signalled(fence))
            mtx_lock(&queue->lock);
   for (unsigned i = queue->read_idx; i != queue->write_idx;
      i = (i + 1) % queue->max_jobs) {
   if (queue->jobs[i].fence == fence) {
                     /* Just clear it. The threads will treat as a no-op job. */
   memset(&queue->jobs[i], 0, sizeof(queue->jobs[i]));
   removed = true;
         }
            if (removed)
         else
      }
      /**
   * Wait until all previously added jobs have completed.
   */
   void
   util_queue_finish(struct util_queue *queue)
   {
      util_barrier barrier;
            /* If 2 threads were adding jobs for 2 different barries at the same time,
   * a deadlock would happen, because 1 barrier requires that all threads
   * wait for it exclusively.
   */
            /* The number of threads can be changed to 0, e.g. by the atexit handler. */
   if (!queue->num_threads) {
      mtx_unlock(&queue->lock);
               /* We need to disable adding new threads in util_queue_add_job because
   * the finish operation requires a fixed number of threads.
   *
   * Also note that util_queue_add_job can unlock the mutex if there is not
   * enough space in the queue and wait for space.
   */
            fences = malloc(queue->num_threads * sizeof(*fences));
            for (unsigned i = 0; i < queue->num_threads; ++i) {
      util_queue_fence_init(&fences[i]);
   util_queue_add_job_locked(queue, &barrier, &fences[i],
      }
   queue->create_threads_on_demand = true;
            for (unsigned i = 0; i < queue->num_threads; ++i) {
      util_queue_fence_wait(&fences[i]);
                  }
      int64_t
   util_queue_get_thread_time_nano(struct util_queue *queue, unsigned thread_index)
   {
      /* Allow some flexibility by not raising an error. */
   if (thread_index >= queue->num_threads)
               }
