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
      #include "vk_drm_syncobj.h"
      #include <sched.h>
   #include <xf86drm.h>
      #include "drm-uapi/drm.h"
      #include "util/os_time.h"
      #include "vk_device.h"
   #include "vk_log.h"
   #include "vk_util.h"
      static struct vk_drm_syncobj *
   to_drm_syncobj(struct vk_sync *sync)
   {
      assert(vk_sync_type_is_drm_syncobj(sync->type));
      }
      static VkResult
   vk_drm_syncobj_init(struct vk_device *device,
               {
               uint32_t flags = 0;
   if (!(sync->flags & VK_SYNC_IS_TIMELINE) && initial_value)
            assert(device->drm_fd >= 0);
   int err = drmSyncobjCreate(device->drm_fd, flags, &sobj->syncobj);
   if (err < 0) {
      return vk_errorf(device, VK_ERROR_OUT_OF_HOST_MEMORY,
               if ((sync->flags & VK_SYNC_IS_TIMELINE) && initial_value) {
      err = drmSyncobjTimelineSignal(device->drm_fd, &sobj->syncobj,
         if (err < 0) {
      vk_drm_syncobj_finish(device, sync);
   return vk_errorf(device, VK_ERROR_OUT_OF_HOST_MEMORY,
                     }
      void
   vk_drm_syncobj_finish(struct vk_device *device,
         {
               assert(device->drm_fd >= 0);
   ASSERTED int err = drmSyncobjDestroy(device->drm_fd, sobj->syncobj);
      }
      static VkResult
   vk_drm_syncobj_signal(struct vk_device *device,
               {
               assert(device->drm_fd >= 0);
   int err;
   if (sync->flags & VK_SYNC_IS_TIMELINE)
         else
         if (err) {
      return vk_errorf(device, VK_ERROR_UNKNOWN,
                  }
      static VkResult
   vk_drm_syncobj_get_value(struct vk_device *device,
               {
               assert(device->drm_fd >= 0);
   int err = drmSyncobjQuery(device->drm_fd, &sobj->syncobj, value, 1);
   if (err) {
      return vk_errorf(device, VK_ERROR_UNKNOWN,
                  }
      static VkResult
   vk_drm_syncobj_reset(struct vk_device *device,
         {
               assert(device->drm_fd >= 0);
   int err = drmSyncobjReset(device->drm_fd, &sobj->syncobj, 1);
   if (err) {
      return vk_errorf(device, VK_ERROR_UNKNOWN,
                  }
      static VkResult
   sync_has_sync_file(struct vk_device *device, struct vk_sync *sync)
   {
               int fd = -1;
   int err = drmSyncobjExportSyncFile(device->drm_fd, handle, &fd);
   if (!err) {
      close(fd);
               /* On the off chance the sync_file export repeatedly fails for some
   * unexpected reason, we want to ensure this function will return success
   * eventually.  Do a zero-time syncobj wait if the export failed.
   */
   err = drmSyncobjWait(device->drm_fd, &handle, 1, 0 /* timeout */,
               if (!err) {
         } else if (errno == ETIME) {
         } else {
      return vk_errorf(device, VK_ERROR_UNKNOWN,
         }
      static VkResult
   spin_wait_for_sync_file(struct vk_device *device,
                           {
      if (wait_flags & VK_SYNC_WAIT_ANY) {
      while (1) {
      for (uint32_t i = 0; i < wait_count; i++) {
      VkResult result = sync_has_sync_file(device, waits[i].sync);
   if (result != VK_TIMEOUT)
                                    } else {
      for (uint32_t i = 0; i < wait_count; i++) {
      while (1) {
      VkResult result = sync_has_sync_file(device, waits[i].sync);
                                                      }
      static VkResult
   vk_drm_syncobj_wait_many(struct vk_device *device,
                           {
      if ((wait_flags & VK_SYNC_WAIT_PENDING) &&
      !(waits[0].sync->type->features & VK_SYNC_FEATURE_TIMELINE)) {
   /* Sadly, DRM_SYNCOBJ_WAIT_FLAGS_WAIT_AVAILABLE was never implemented
   * for drivers that don't support timelines.  Instead, we have to spin
   * on DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE until it succeeds.
   */
   return spin_wait_for_sync_file(device, wait_count, waits,
               /* Syncobj timeouts are signed */
            STACK_ARRAY(uint32_t, handles, wait_count);
            uint32_t j = 0;
   bool has_timeline = false;
   for (uint32_t i = 0; i < wait_count; i++) {
      /* The syncobj API doesn't like wait values of 0 but it's safe to skip
   * them because a wait for 0 is a no-op.
   */
   if (waits[i].sync->flags & VK_SYNC_IS_TIMELINE) {
                                 handles[j] = to_drm_syncobj(waits[i].sync)->syncobj;
   wait_values[j] = waits[i].wait_value;
      }
   assert(j <= wait_count);
            uint32_t syncobj_wait_flags = DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT;
   if (!(wait_flags & VK_SYNC_WAIT_ANY))
            assert(device->drm_fd >= 0);
   int err;
   if (wait_count == 0) {
         } else if (wait_flags & VK_SYNC_WAIT_PENDING) {
      /* We always use a timeline wait for WAIT_PENDING, even for binary
   * syncobjs because the non-timeline wait doesn't support
   * DRM_SYNCOBJ_WAIT_FLAGS_WAIT_AVAILABLE.
   */
   err = drmSyncobjTimelineWait(device->drm_fd, handles, wait_values,
                        } else if (has_timeline) {
      err = drmSyncobjTimelineWait(device->drm_fd, handles, wait_values,
                  } else {
      err = drmSyncobjWait(device->drm_fd, handles,
                           STACK_ARRAY_FINISH(handles);
            if (err && errno == ETIME) {
         } else if (err) {
      return vk_errorf(device, VK_ERROR_UNKNOWN,
                  }
      static VkResult
   vk_drm_syncobj_import_opaque_fd(struct vk_device *device,
               {
               assert(device->drm_fd >= 0);
   uint32_t new_handle;
   int err = drmSyncobjFDToHandle(device->drm_fd, fd, &new_handle);
   if (err) {
      return vk_errorf(device, VK_ERROR_UNKNOWN,
               err = drmSyncobjDestroy(device->drm_fd, sobj->syncobj);
                        }
      static VkResult
   vk_drm_syncobj_export_opaque_fd(struct vk_device *device,
               {
               assert(device->drm_fd >= 0);
   int err = drmSyncobjHandleToFD(device->drm_fd, sobj->syncobj, fd);
   if (err) {
      return vk_errorf(device, VK_ERROR_UNKNOWN,
                  }
      static VkResult
   vk_drm_syncobj_import_sync_file(struct vk_device *device,
               {
               assert(device->drm_fd >= 0);
   int err = drmSyncobjImportSyncFile(device->drm_fd, sobj->syncobj, sync_file);
   if (err) {
      return vk_errorf(device, VK_ERROR_UNKNOWN,
                  }
      static VkResult
   vk_drm_syncobj_export_sync_file(struct vk_device *device,
               {
               assert(device->drm_fd >= 0);
   int err = drmSyncobjExportSyncFile(device->drm_fd, sobj->syncobj, sync_file);
   if (err) {
      return vk_errorf(device, VK_ERROR_UNKNOWN,
                  }
      static VkResult
   vk_drm_syncobj_move(struct vk_device *device,
               {
      struct vk_drm_syncobj *dst_sobj = to_drm_syncobj(dst);
   struct vk_drm_syncobj *src_sobj = to_drm_syncobj(src);
            if (!(dst->flags & VK_SYNC_IS_SHARED) &&
      !(src->flags & VK_SYNC_IS_SHARED)) {
   result = vk_drm_syncobj_reset(device, dst);
   if (unlikely(result != VK_SUCCESS))
            uint32_t tmp = dst_sobj->syncobj;
   dst_sobj->syncobj = src_sobj->syncobj;
               } else {
      int fd;
   result = vk_drm_syncobj_export_sync_file(device, src, &fd);
   if (result != VK_SUCCESS)
            result = vk_drm_syncobj_import_sync_file(device, dst, fd);
   if (fd >= 0)
         if (result != VK_SUCCESS)
                  }
      struct vk_sync_type
   vk_drm_syncobj_get_type(int drm_fd)
   {
      uint32_t syncobj = 0;
   int err = drmSyncobjCreate(drm_fd, DRM_SYNCOBJ_CREATE_SIGNALED, &syncobj);
   if (err < 0)
            struct vk_sync_type type = {
      .size = sizeof(struct vk_drm_syncobj),
   .features = VK_SYNC_FEATURE_BINARY |
               VK_SYNC_FEATURE_GPU_WAIT |
   VK_SYNC_FEATURE_CPU_RESET |
   .init = vk_drm_syncobj_init,
   .finish = vk_drm_syncobj_finish,
   .signal = vk_drm_syncobj_signal,
   .reset = vk_drm_syncobj_reset,
   .move = vk_drm_syncobj_move,
   .import_opaque_fd = vk_drm_syncobj_import_opaque_fd,
   .export_opaque_fd = vk_drm_syncobj_export_opaque_fd,
   .import_sync_file = vk_drm_syncobj_import_sync_file,
               err = drmSyncobjWait(drm_fd, &syncobj, 1, 0,
               if (err == 0) {
      type.wait_many = vk_drm_syncobj_wait_many;
   type.features |= VK_SYNC_FEATURE_CPU_WAIT |
               uint64_t cap;
   err = drmGetCap(drm_fd, DRM_CAP_SYNCOBJ_TIMELINE, &cap);
   if (err == 0 && cap != 0) {
      type.get_value = vk_drm_syncobj_get_value;
               err = drmSyncobjDestroy(drm_fd, syncobj);
               }
