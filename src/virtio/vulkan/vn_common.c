   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_common.h"
      #include <stdarg.h>
   #include <sys/syscall.h>
      #include "util/log.h"
   #include "util/os_misc.h"
   #include "util/u_debug.h"
   #include "venus-protocol/vn_protocol_driver_info.h"
   #include "vk_enum_to_str.h"
      #include "vn_ring.h"
      #define VN_RELAX_MIN_BASE_SLEEP_US (160)
      static const struct debug_control vn_debug_options[] = {
      /* clang-format off */
   { "init", VN_DEBUG_INIT },
   { "result", VN_DEBUG_RESULT },
   { "vtest", VN_DEBUG_VTEST },
   { "wsi", VN_DEBUG_WSI },
   { "no_abort", VN_DEBUG_NO_ABORT },
   { "log_ctx_info", VN_DEBUG_LOG_CTX_INFO },
   { "cache", VN_DEBUG_CACHE },
   { "no_sparse", VN_DEBUG_NO_SPARSE },
   { "gpl", VN_DEBUG_GPL },
   { NULL, 0 },
      };
      static const struct debug_control vn_perf_options[] = {
      /* clang-format off */
   { "no_async_set_alloc", VN_PERF_NO_ASYNC_SET_ALLOC },
   { "no_async_buffer_create", VN_PERF_NO_ASYNC_BUFFER_CREATE },
   { "no_async_queue_submit", VN_PERF_NO_ASYNC_QUEUE_SUBMIT },
   { "no_event_feedback", VN_PERF_NO_EVENT_FEEDBACK },
   { "no_fence_feedback", VN_PERF_NO_FENCE_FEEDBACK },
   { "no_memory_suballoc", VN_PERF_NO_MEMORY_SUBALLOC },
   { "no_cmd_batching", VN_PERF_NO_CMD_BATCHING },
   { "no_timeline_sem_feedback", VN_PERF_NO_TIMELINE_SEM_FEEDBACK },
   { "no_query_feedback", VN_PERF_NO_QUERY_FEEDBACK },
   { "no_async_mem_alloc", VN_PERF_NO_ASYNC_MEM_ALLOC },
   { NULL, 0 },
      };
      struct vn_env vn_env;
      static void
   vn_env_init_once(void)
   {
      vn_env.debug =
         vn_env.perf =
         vn_env.draw_cmd_batch_limit =
         if (!vn_env.draw_cmd_batch_limit)
         vn_env.relax_base_sleep_us = debug_get_num_option(
      }
      void
   vn_env_init(void)
   {
      static once_flag once = ONCE_FLAG_INIT;
            /* log per VkInstance creation */
   if (VN_DEBUG(INIT)) {
      vn_log(NULL,
         "vn_env is as below:"
   "\n\tdebug = 0x%" PRIx64 ""
   "\n\tperf = 0x%" PRIx64 ""
   "\n\tdraw_cmd_batch_limit = %u"
   "\n\trelax_base_sleep_us = %u",
         }
      void
   vn_trace_init(void)
   {
   #ifdef ANDROID
         #else
         #endif
   }
      void
   vn_log(struct vn_instance *instance, const char *format, ...)
   {
               va_start(ap, format);
   mesa_log_v(MESA_LOG_DEBUG, "MESA-VIRTIO", format, ap);
               }
      VkResult
   vn_log_result(struct vn_instance *instance,
               {
      vn_log(instance, "%s: %s", where, vk_Result_to_str(result));
      }
      uint32_t
   vn_extension_get_spec_version(const char *name)
   {
      const int32_t index = vn_info_extension_index(name);
      }
      static bool
   vn_ring_monitor_acquire(struct vn_ring *ring)
   {
      pid_t tid = syscall(SYS_gettid);
   if (!ring->monitor.threadid && tid != ring->monitor.threadid &&
      mtx_trylock(&ring->monitor.mutex) == thrd_success) {
   /* register as the only waiting thread that monitors the ring. */
      }
      }
      void
   vn_ring_monitor_release(struct vn_ring *ring)
   {
      if (syscall(SYS_gettid) != ring->monitor.threadid)
            ring->monitor.threadid = 0;
      }
      struct vn_relax_state
   vn_relax_init(struct vn_ring *ring, const char *reason)
   {
         #ifndef NDEBUG
         /* ensure minimum check period is greater than maximum renderer
   * reporting period (with margin of safety to ensure no false
   * positives).
   *
   * first_warn_time is pre-calculated based on parameters in vn_relax
   * and must update together.
   */
   const uint32_t first_warn_time = 3481600;
   const uint32_t safety_margin = 250000;
   assert(first_warn_time - safety_margin >=
   #endif
            if (vn_ring_monitor_acquire(ring))
               return (struct vn_relax_state){
      .ring = ring,
   .iter = 0,
         }
      void
   vn_relax(struct vn_relax_state *state)
   {
      struct vn_ring *ring = state->ring;
   uint32_t *iter = &state->iter;
            /* Yield for the first 2^busy_wait_order times and then sleep for
   * base_sleep_us microseconds for the same number of times.  After that,
   * keep doubling both sleep length and count.
   * Must also update pre-calculated "first_warn_time" in vn_relax_init().
   */
   const uint32_t busy_wait_order = 8;
   const uint32_t base_sleep_us = vn_env.relax_base_sleep_us;
   const uint32_t warn_order = 12;
            (*iter)++;
   if (*iter < (1 << busy_wait_order)) {
      thrd_yield();
               /* warn occasionally if we have slept at least 1.28ms for 2048 times (plus
   * another 2047 shorter sleeps)
   */
   if (unlikely(*iter % (1 << warn_order) == 0)) {
               const uint32_t status = vn_ring_load_status(ring);
   if (status & VK_RING_STATUS_FATAL_BIT_MESA) {
      vn_log(NULL, "aborting on ring fatal error at iter %d", *iter);
               if (ring->monitor.report_period_us) {
      if (vn_ring_monitor_acquire(ring)) {
      ring->monitor.alive = status & VK_RING_STATUS_ALIVE_BIT_MESA;
               if (!ring->monitor.alive && !VN_DEBUG(NO_ABORT)) {
      vn_log(NULL, "aborting on expired ring alive status at iter %d",
                        if (*iter >= (1 << abort_order) && !VN_DEBUG(NO_ABORT)) {
      vn_log(NULL, "aborting");
                  const uint32_t shift = util_last_bit(*iter) - busy_wait_order - 1;
      }
