   /*
   * Copyright 1999-2006 Brian Paul
   * Copyright 2008 VMware, Inc.
   * Copyright 2022 Yonggang Luo
   * SPDX-License-Identifier: MIT
   */
      #include "util/u_thread.h"
      #include "macros.h"
      #ifdef HAVE_PTHREAD
   #include <signal.h>
   #ifdef HAVE_PTHREAD_NP_H
   #include <pthread_np.h>
   #endif
   #endif
      #ifdef __HAIKU__
   #include <OS.h>
   #endif
      #if DETECT_OS_LINUX && !defined(ANDROID)
   #include <sched.h>
   #elif defined(_WIN32) && !defined(HAVE_PTHREAD)
   #include <windows.h>
   #endif
      #ifdef __FreeBSD__
   /* pthread_np.h -> sys/param.h -> machine/param.h
   * - defines ALIGN which clashes with our ALIGN
   */
   #undef ALIGN
   #define cpu_set_t cpuset_t
   #endif
      int
   util_get_current_cpu(void)
   {
   #if DETECT_OS_LINUX && !defined(ANDROID)
            #elif defined(_WIN32) && !defined(HAVE_PTHREAD)
            #else
         #endif
   }
      int u_thread_create(thrd_t *thrd, int (*routine)(void *), void *param)
   {
         #ifdef HAVE_PTHREAD
               sigfillset(&new_set);
            /* SIGSEGV is commonly used by Vulkan API tracing layers in order to track
   * accesses in device memory mapped to user space. Blocking the signal hinders
   * that tracking mechanism.
   */
   sigdelset(&new_set, SIGSEGV);
   pthread_sigmask(SIG_BLOCK, &new_set, &saved_set);
   ret = thrd_create(thrd, routine, param);
      #else
         #endif
            }
      void u_thread_setname( const char *name )
   {
   #if defined(HAVE_PTHREAD)
   #if DETECT_OS_LINUX || DETECT_OS_CYGWIN || DETECT_OS_SOLARIS || defined(__GLIBC__)
      int ret = pthread_setname_np(pthread_self(), name);
   if (ret == ERANGE) {
      char buf[16];
   const size_t len = MIN2(strlen(name), ARRAY_SIZE(buf) - 1);
   memcpy(buf, name, len);
   buf[len] = '\0';
         #elif DETECT_OS_FREEBSD || DETECT_OS_OPENBSD
         #elif DETECT_OS_NETBSD
         #elif DETECT_OS_APPLE
         #elif DETECT_OS_HAIKU
         #else
   #warning Not sure how to call pthread_setname_np
   #endif
   #endif
         }
      bool
   util_set_thread_affinity(thrd_t thread,
                     {
   #if defined(HAVE_PTHREAD_SETAFFINITY)
               if (old_mask) {
      if (pthread_getaffinity_np(thread, sizeof(cpuset), &cpuset) != 0)
            memset(old_mask, 0, num_mask_bits / 8);
   for (unsigned i = 0; i < num_mask_bits && i < CPU_SETSIZE; i++) {
      if (CPU_ISSET(i, &cpuset))
                  CPU_ZERO(&cpuset);
   for (unsigned i = 0; i < num_mask_bits && i < CPU_SETSIZE; i++) {
      if (mask[i / 32] & (1u << (i % 32)))
      }
         #elif defined(_WIN32) && !defined(HAVE_PTHREAD)
               if (sizeof(m) > 4 && num_mask_bits > 32)
            m = SetThreadAffinityMask(thread.handle, m);
   if (!m)
            if (old_mask) {
               #ifdef _WIN64
         #endif
                  #else
         #endif
   }
      int64_t
   util_thread_get_time_nano(thrd_t thread)
   {
   #if defined(HAVE_PTHREAD) && !defined(__APPLE__) && !defined(__HAIKU__)
      struct timespec ts;
            pthread_getcpuclockid(thread, &cid);
   clock_gettime(cid, &ts);
      #elif defined(_WIN32)
      union {
      FILETIME time;
      } kernel_time, user_time;
   GetThreadTimes((HANDLE)thread.handle, NULL, NULL, &kernel_time.time, &user_time.time);
      #else
      (void)thread;
      #endif
   }
      #if defined(HAVE_PTHREAD) && !defined(__APPLE__) && !defined(__HAIKU__)
      void util_barrier_init(util_barrier *barrier, unsigned count)
   {
         }
      void util_barrier_destroy(util_barrier *barrier)
   {
         }
      bool util_barrier_wait(util_barrier *barrier)
   {
         }
      #else /* If the OS doesn't have its own, implement barriers using a mutex and a condvar */
      void util_barrier_init(util_barrier *barrier, unsigned count)
   {
      barrier->count = count;
   barrier->waiters = 0;
   barrier->sequence = 0;
   (void) mtx_init(&barrier->mutex, mtx_plain);
      }
      void util_barrier_destroy(util_barrier *barrier)
   {
      assert(barrier->waiters == 0);
   mtx_destroy(&barrier->mutex);
      }
      bool util_barrier_wait(util_barrier *barrier)
   {
               assert(barrier->waiters < barrier->count);
            if (barrier->waiters < barrier->count) {
               do {
            } else {
      barrier->waiters = 0;
   barrier->sequence++;
                           }
      #endif
