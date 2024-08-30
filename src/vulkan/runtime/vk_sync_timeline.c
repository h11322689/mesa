   /*
   * Copyright © 2021 Intel Corporation
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
      #include "vk_sync_timeline.h"
      #include <inttypes.h>
      #include "util/os_time.h"
   #include "util/timespec.h"
      #include "vk_alloc.h"
   #include "vk_device.h"
   #include "vk_log.h"
      static struct vk_sync_timeline *
   to_vk_sync_timeline(struct vk_sync *sync)
   {
                  }
      static void
   vk_sync_timeline_type_validate(const struct vk_sync_timeline_type *ttype)
   {
      ASSERTED const enum vk_sync_features req_features =
      VK_SYNC_FEATURE_BINARY |
   VK_SYNC_FEATURE_GPU_WAIT |
   VK_SYNC_FEATURE_GPU_MULTI_WAIT |
   VK_SYNC_FEATURE_CPU_WAIT |
            }
      VkResult
   vk_sync_timeline_init(struct vk_device *device,
               {
      struct vk_sync_timeline *timeline = to_vk_sync_timeline(sync);
            ASSERTED const struct vk_sync_timeline_type *ttype =
                  ret = mtx_init(&timeline->mutex, mtx_plain);
   if (ret != thrd_success)
            ret = cnd_init(&timeline->cond);
   if (ret != thrd_success) {
      mtx_destroy(&timeline->mutex);
               timeline->highest_past =
         list_inithead(&timeline->pending_points);
               }
      static void
   vk_sync_timeline_finish(struct vk_device *device,
         {
               list_for_each_entry_safe(struct vk_sync_timeline_point, point,
            list_del(&point->link);
   vk_sync_finish(device, &point->sync);
      }
   list_for_each_entry_safe(struct vk_sync_timeline_point, point,
            list_del(&point->link);
   vk_sync_finish(device, &point->sync);
               cnd_destroy(&timeline->cond);
      }
      static struct vk_sync_timeline_point *
   vk_sync_timeline_first_point(struct vk_sync_timeline *timeline)
   {
      struct vk_sync_timeline_point *point =
      list_first_entry(&timeline->pending_points,
         assert(point->value <= timeline->highest_pending);
               }
      static VkResult
   vk_sync_timeline_gc_locked(struct vk_device *device,
                  static VkResult
   vk_sync_timeline_alloc_point_locked(struct vk_device *device,
                     {
      struct vk_sync_timeline_point *point;
            result = vk_sync_timeline_gc_locked(device, timeline, false);
   if (unlikely(result != VK_SUCCESS))
            if (list_is_empty(&timeline->free_points)) {
      const struct vk_sync_timeline_type *ttype =
                  size_t size = offsetof(struct vk_sync_timeline_point, sync) +
            point = vk_zalloc(&device->alloc, size, 8,
         if (!point)
                     result = vk_sync_init(device, &point->sync, point_sync_type,
         if (unlikely(result != VK_SUCCESS)) {
      vk_free(&device->alloc, point);
         } else {
      point = list_first_entry(&timeline->free_points,
            if (point->sync.type->reset) {
      result = vk_sync_reset(device, &point->sync);
   if (unlikely(result != VK_SUCCESS))
                           point->value = value;
               }
      VkResult
   vk_sync_timeline_alloc_point(struct vk_device *device,
                     {
               mtx_lock(&timeline->mutex);
   result = vk_sync_timeline_alloc_point_locked(device, timeline, value, point_out);
               }
      static void
   vk_sync_timeline_point_free_locked(struct vk_sync_timeline *timeline,
         {
      assert(point->refcount == 0 && !point->pending);
      }
      void
   vk_sync_timeline_point_free(struct vk_device *device,
         {
               mtx_lock(&timeline->mutex);
   vk_sync_timeline_point_free_locked(timeline, point);
      }
      static void
   vk_sync_timeline_point_ref(struct vk_sync_timeline_point *point)
   {
         }
      static void
   vk_sync_timeline_point_unref(struct vk_sync_timeline *timeline,
         {
      assert(point->refcount > 0);
   point->refcount--;
   if (point->refcount == 0 && !point->pending)
      }
      static void
   vk_sync_timeline_point_complete(struct vk_sync_timeline *timeline,
         {
      if (!point->pending)
            assert(timeline->highest_past < point->value);
            point->pending = false;
            if (point->refcount == 0)
      }
      static VkResult
   vk_sync_timeline_gc_locked(struct vk_device *device,
               {
      list_for_each_entry_safe(struct vk_sync_timeline_point, point,
            /* timeline->higest_pending is only incremented once submission has
   * happened. If this point has a greater serial, it means the point
   * hasn't been submitted yet.
   */
   if (point->value > timeline->highest_pending)
            /* If someone is waiting on this time point, consider it busy and don't
   * try to recycle it. There's a slim possibility that it's no longer
   * busy by the time we look at it but we would be recycling it out from
   * under a waiter and that can lead to weird races.
   *
   * We walk the list in-order so if this time point is still busy so is
   * every following time point
   */
   assert(point->refcount >= 0);
   if (point->refcount > 0 && !drain)
            /* Garbage collect any signaled point. */
   VkResult result = vk_sync_wait(device, &point->sync, 0,
               if (result == VK_TIMEOUT) {
      /* We walk the list in-order so if this time point is still busy so
   * is every following time point
   */
      } else if (result != VK_SUCCESS) {
                                 }
      VkResult
   vk_sync_timeline_point_install(struct vk_device *device,
         {
                        assert(point->value > timeline->highest_pending);
            assert(point->refcount == 0);
   point->pending = true;
                              if (ret == thrd_error)
               }
      static VkResult
   vk_sync_timeline_get_point_locked(struct vk_device *device,
                     {
      if (timeline->highest_past >= wait_value) {
      /* Nothing to wait on */
   *point_out = NULL;
               list_for_each_entry(struct vk_sync_timeline_point, point,
            if (point->value >= wait_value) {
      vk_sync_timeline_point_ref(point);
   *point_out = point;
                     }
      VkResult
   vk_sync_timeline_get_point(struct vk_device *device,
                     {
      mtx_lock(&timeline->mutex);
   VkResult result = vk_sync_timeline_get_point_locked(device, timeline,
                     }
      void
   vk_sync_timeline_point_release(struct vk_device *device,
         {
               mtx_lock(&timeline->mutex);
   vk_sync_timeline_point_unref(timeline, point);
      }
      static VkResult
   vk_sync_timeline_signal_locked(struct vk_device *device,
               {
      VkResult result = vk_sync_timeline_gc_locked(device, timeline, true);
   if (unlikely(result != VK_SUCCESS))
            if (unlikely(value <= timeline->highest_past)) {
      return vk_device_set_lost(device, "Timeline values must only ever "
               assert(list_is_empty(&timeline->pending_points));
   assert(timeline->highest_pending == timeline->highest_past);
            int ret = cnd_broadcast(&timeline->cond);
   if (ret == thrd_error)
               }
      static VkResult
   vk_sync_timeline_signal(struct vk_device *device,
               {
               mtx_lock(&timeline->mutex);
   VkResult result = vk_sync_timeline_signal_locked(device, timeline, value);
               }
      static VkResult
   vk_sync_timeline_get_value(struct vk_device *device,
               {
               mtx_lock(&timeline->mutex);
   VkResult result = vk_sync_timeline_gc_locked(device, timeline, true);
            if (result != VK_SUCCESS)
                        }
      static VkResult
   vk_sync_timeline_wait_locked(struct vk_device *device,
                           {
      /* Wait on the queue_submit condition variable until the timeline has a
   * time point pending that's at least as high as wait_value.
   */
   uint64_t now_ns = os_time_get_nano();
   while (timeline->highest_pending < wait_value) {
      if (now_ns >= abs_timeout_ns)
            int ret;
   if (abs_timeout_ns >= INT64_MAX) {
      /* Common infinite wait case */
      } else {
      /* This is really annoying.  The C11 threads API uses CLOCK_REALTIME
   * while all our absolute timeouts are in CLOCK_MONOTONIC.  Best
   * thing we can do is to convert and hope the system admin doesn't
   * change the time out from under us.
                  struct timespec now_ts, abs_timeout_ts;
   timespec_get(&now_ts, TIME_UTC);
   if (timespec_add_nsec(&abs_timeout_ts, &now_ts, rel_timeout_ns)) {
      /* Overflowed; may as well be infinite */
      } else {
      ret = cnd_timedwait(&timeline->cond, &timeline->mutex,
         }
   if (ret == thrd_error)
            /* We don't trust the timeout condition on cnd_timedwait() because of
   * the potential clock issues caused by using CLOCK_REALTIME.  Instead,
   * update now_ns, go back to the top of the loop, and re-check.
   */
               if (wait_flags & VK_SYNC_WAIT_PENDING)
            VkResult result = vk_sync_timeline_gc_locked(device, timeline, false);
   if (result != VK_SUCCESS)
            while (timeline->highest_past < wait_value) {
               /* Drop the lock while we wait. */
   vk_sync_timeline_point_ref(point);
            result = vk_sync_wait(device, &point->sync, 0,
                  /* Pick the mutex back up */
   mtx_lock(&timeline->mutex);
            /* This covers both VK_TIMEOUT and VK_ERROR_DEVICE_LOST */
   if (result != VK_SUCCESS)
                           }
      static VkResult
   vk_sync_timeline_wait(struct vk_device *device,
                           {
               mtx_lock(&timeline->mutex);
   VkResult result = vk_sync_timeline_wait_locked(device, timeline,
                           }
      struct vk_sync_timeline_type
   vk_sync_timeline_get_type(const struct vk_sync_type *point_sync_type)
   {
      return (struct vk_sync_timeline_type) {
      .sync = {
      .size = sizeof(struct vk_sync_timeline),
   .features = VK_SYNC_FEATURE_TIMELINE |
               VK_SYNC_FEATURE_GPU_WAIT |
   VK_SYNC_FEATURE_CPU_WAIT |
   VK_SYNC_FEATURE_CPU_SIGNAL |
   .init = vk_sync_timeline_init,
   .finish = vk_sync_timeline_finish,
   .signal = vk_sync_timeline_signal,
   .get_value = vk_sync_timeline_get_value,
      },
         }
