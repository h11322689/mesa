   /**************************************************************************
   *
   * Copyright 2013 Marek Olšák <maraeo@gmail.com>
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
   * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /* This file contains code for reading CPU load for displaying on the HUD.
   */
      #include "hud/hud_private.h"
   #include "util/os_time.h"
   #include "util/u_thread.h"
   #include "util/u_memory.h"
   #include "util/u_queue.h"
   #include <stdio.h>
   #include <inttypes.h>
   #if DETECT_OS_WINDOWS
   #include <windows.h>
   #endif
   #if DETECT_OS_BSD
   #include <sys/types.h>
   #include <sys/sysctl.h>
   #if DETECT_OS_NETBSD || DETECT_OS_OPENBSD
   #include <sys/sched.h>
   #else
   #include <sys/resource.h>
   #endif
   #endif
         #if DETECT_OS_WINDOWS
      static inline uint64_t
   filetime_to_scalar(FILETIME ft)
   {
      ULARGE_INTEGER uli;
   uli.LowPart = ft.dwLowDateTime;
   uli.HighPart = ft.dwHighDateTime;
      }
      static bool
   get_cpu_stats(unsigned cpu_index, uint64_t *busy_time, uint64_t *total_time)
   {
      SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
   assert(sysInfo.dwNumberOfProcessors >= 1);
   if (cpu_index != ALL_CPUS && cpu_index >= sysInfo.dwNumberOfProcessors) {
      /* Tell hud_get_num_cpus there are only this many CPUs. */
               /* Get accumulated user and sys time for all threads */
   if (!GetProcessTimes(GetCurrentProcess(), &ftCreation, &ftExit,
                           *busy_time = filetime_to_scalar(ftUser) + filetime_to_scalar(ftKernel);
            /* busy_time already has the time accross all cpus.
   * XXX: if we want 100% to mean one CPU, 200% two cpus, eliminate the
   * following line.
   */
            /* XXX: we ignore cpu_index, i.e, we assume that the individual CPU usage
   * and the system usage are one and the same.
   */
      }
      #elif DETECT_OS_BSD
      static bool
   get_cpu_stats(unsigned cpu_index, uint64_t *busy_time, uint64_t *total_time)
   {
   #if DETECT_OS_NETBSD || DETECT_OS_OPENBSD
         #else
         #endif
               if (cpu_index == ALL_CPUS) {
         #if DETECT_OS_NETBSD
                  if (sysctl(mib, ARRAY_SIZE(mib), cp_time, &len, NULL, 0) == -1)
   #elif DETECT_OS_OPENBSD
         int mib[] = { CTL_KERN, KERN_CPTIME };
            len = sizeof(sum_cp_time);
   if (sysctl(mib, ARRAY_SIZE(mib), sum_cp_time, &len, NULL, 0) == -1)
            for (int state = 0; state < CPUSTATES; state++)
   #else
         if (sysctlbyname("kern.cp_time", cp_time, &len, NULL, 0) == -1)
   #endif
         #if DETECT_OS_NETBSD
                  len = sizeof(cp_time);
   if (sysctl(mib, ARRAY_SIZE(mib), cp_time, &len, NULL, 0) == -1)
   #elif DETECT_OS_OPENBSD
                  len = sizeof(cp_time);
   if (sysctl(mib, ARRAY_SIZE(mib), cp_time, &len, NULL, 0) == -1)
   #else
                  if (sysctlbyname("kern.cp_times", NULL, &len, NULL, 0) == -1)
            if (len < (cpu_index + 1) * sizeof(cp_time))
                     if (sysctlbyname("kern.cp_times", cp_times, &len, NULL, 0) == -1)
            memcpy(cp_time, cp_times + (cpu_index * CPUSTATES),
         #endif
               *busy_time = cp_time[CP_USER] + cp_time[CP_NICE] +
                        }
      #else
      static bool
   get_cpu_stats(unsigned cpu_index, uint64_t *busy_time, uint64_t *total_time)
   {
      char cpuname[32];
   char line[1024];
            if (cpu_index == ALL_CPUS)
         else
            f = fopen("/proc/stat", "r");
   if (!f)
            while (!feof(f) && fgets(line, sizeof(line), f)) {
      if (strstr(line, cpuname) == line) {
                     num = sscanf(line,
               "%s %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64
   " %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64
   " %"PRIu64" %"PRIu64"",
   if (num < 5) {
      fclose(f);
               /* user + nice + system */
                  /* ... + idle + iowait + irq + softirq + ...  */
   for (i = 3; i < num-1; i++) {
         }
   fclose(f);
         }
   fclose(f);
      }
   #endif
         struct cpu_info {
      unsigned cpu_index;
      };
      static void
   query_cpu_load(struct hud_graph *gr, struct pipe_context *pipe)
   {
      struct cpu_info *info = gr->query_data;
            if (info->last_time) {
      if (info->last_time + gr->pane->period <= now) {
                              cpu_load = (cpu_busy - info->last_cpu_busy) * 100 /
                  info->last_cpu_busy = cpu_busy;
   info->last_cpu_total = cpu_total;
         }
   else {
      /* initialize */
   info->last_time = now;
   get_cpu_stats(info->cpu_index, &info->last_cpu_busy,
         }
      static void
   free_query_data(void *p, struct pipe_context *pipe)
   {
         }
      void
   hud_cpu_graph_install(struct hud_pane *pane, unsigned cpu_index)
   {
      struct hud_graph *gr;
   struct cpu_info *info;
            /* see if the cpu exists */
   if (cpu_index != ALL_CPUS && !get_cpu_stats(cpu_index, &busy, &total)) {
                  gr = CALLOC_STRUCT(hud_graph);
   if (!gr)
            if (cpu_index == ALL_CPUS)
         else
            gr->query_data = CALLOC_STRUCT(cpu_info);
   if (!gr->query_data) {
      FREE(gr);
                        /* Don't use free() as our callback as that messes up Gallium's
   * memory debugger.  Use simple free_query_data() wrapper.
   */
            info = gr->query_data;
            hud_pane_add_graph(pane, gr);
      }
      int
   hud_get_num_cpus(void)
   {
      uint64_t busy, total;
            while (get_cpu_stats(i, &busy, &total))
               }
      struct thread_info {
      bool main_thread;
   int64_t last_time;
      };
      static void
   query_api_thread_busy_status(struct hud_graph *gr, struct pipe_context *pipe)
   {
      struct thread_info *info = gr->query_data;
            if (info->last_time) {
      if (info->last_time + gr->pane->period*1000 <= now) {
               if (info->main_thread) {
                           if (mon && mon->queue)
         else
                              /* Check if the context changed a thread, so that we don't show
   * a random value. When a thread is changed, the new thread clock
   * is different, which can result in "percent" being very high.
   */
   if (percent > 100.0)
                  info->last_thread_time = thread_now;
         } else {
      /* initialize */
   info->last_time = now;
         }
      void
   hud_thread_busy_install(struct hud_pane *pane, const char *name, bool main)
   {
               gr = CALLOC_STRUCT(hud_graph);
   if (!gr)
                     gr->query_data = CALLOC_STRUCT(thread_info);
   if (!gr->query_data) {
      FREE(gr);
               ((struct thread_info*)gr->query_data)->main_thread = main;
            /* Don't use free() as our callback as that messes up Gallium's
   * memory debugger.  Use simple free_query_data() wrapper.
   */
            hud_pane_add_graph(pane, gr);
      }
      struct counter_info {
      enum hud_counter counter;
      };
      static unsigned get_counter(struct hud_graph *gr, enum hud_counter counter)
   {
      struct util_queue_monitoring *mon = gr->pane->hud->monitored_queue;
            if (!mon || !mon->queue)
            /* Reset the counters to 0 to only display values for 1 frame. */
   switch (counter) {
   case HUD_COUNTER_OFFLOADED:
      value = mon->num_offloaded_items;
   mon->num_offloaded_items = 0;
      case HUD_COUNTER_DIRECT:
      value = mon->num_direct_items;
   mon->num_direct_items = 0;
      case HUD_COUNTER_SYNCS:
      value = mon->num_syncs;
   mon->num_syncs = 0;
      case HUD_COUNTER_BATCHES:
      value = mon->num_batches;
   mon->num_batches = 0;
      default:
      assert(0);
         }
      static void
   query_thread_counter(struct hud_graph *gr, struct pipe_context *pipe)
   {
      struct counter_info *info = gr->query_data;
   int64_t now = os_time_get_nano();
            if (info->last_time) {
      if (info->last_time + gr->pane->period*1000 <= now) {
      hud_graph_add_value(gr, value);
         } else {
      /* initialize */
         }
      void hud_thread_counter_install(struct hud_pane *pane, const char *name,
         {
      struct hud_graph *gr = CALLOC_STRUCT(hud_graph);
   if (!gr)
                     gr->query_data = CALLOC_STRUCT(counter_info);
   if (!gr->query_data) {
      FREE(gr);
               ((struct counter_info*)gr->query_data)->counter = counter;
            /* Don't use free() as our callback as that messes up Gallium's
   * memory debugger.  Use simple free_query_data() wrapper.
   */
            hud_pane_add_graph(pane, gr);
      }
