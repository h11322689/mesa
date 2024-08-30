   /*
   * Copyright © 2023 Intel Corporation
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "xe/anv_device.h"
   #include "anv_private.h"
      #include "drm-uapi/gpu_scheduler.h"
   #include "drm-uapi/xe_drm.h"
      bool anv_xe_device_destroy_vm(struct anv_device *device)
   {
      struct drm_xe_vm_destroy destroy = {
         };
      }
      VkResult anv_xe_device_setup_vm(struct anv_device *device)
   {
      struct drm_xe_vm_create create = {
         };
   if (intel_ioctl(device->fd, DRM_IOCTL_XE_VM_CREATE, &create) != 0)
      return vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
         device->vm_id = create.vm_id;
      }
      static VkQueueGlobalPriorityKHR
   drm_sched_priority_to_vk_priority(enum drm_sched_priority drm_sched_priority)
   {
      switch (drm_sched_priority) {
   case DRM_SCHED_PRIORITY_MIN:
         case DRM_SCHED_PRIORITY_NORMAL:
         case DRM_SCHED_PRIORITY_HIGH:
         default:
      unreachable("Invalid drm_sched_priority");
         }
      static void *
   xe_query_alloc_fetch(struct anv_physical_device *device, uint32_t query_id)
   {
      struct drm_xe_device_query query = {
         };
   if (intel_ioctl(device->local_fd, DRM_IOCTL_XE_DEVICE_QUERY, &query))
            void *data = calloc(1, query.size);
   if (!data)
            query.data = (uintptr_t)data;
   if (intel_ioctl(device->local_fd, DRM_IOCTL_XE_DEVICE_QUERY, &query)) {
      free(data);
                  }
      VkResult
   anv_xe_physical_device_get_parameters(struct anv_physical_device *device)
   {
               config = xe_query_alloc_fetch(device, DRM_XE_DEVICE_QUERY_CONFIG);
   if (!config)
      return vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
         device->has_exec_timeline = true;
   device->max_context_priority =
            free(config);
      }
      VkResult
   anv_xe_physical_device_init_memory_types(struct anv_physical_device *device)
   {
      if (anv_physical_device_has_vram(device)) {
      device->memory.type_count = 3;
   device->memory.types[0] = (struct anv_memory_type) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      };
   device->memory.types[1] = (struct anv_memory_type) {
      .propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  };
   device->memory.types[2] = (struct anv_memory_type) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
               /* This memory type either comes from heaps[0] if there is only
   * mappable vram region, or from heaps[2] if there is both mappable &
   * non-mappable vram regions.
   */
         } else if (device->info.has_llc) {
      /* Big core GPUs share LLC with the CPU and thus one memory type can be
   * both cached and coherent at the same time.
   *
   * But some game engines can't handle single type well
   * https://gitlab.freedesktop.org/mesa/mesa/-/issues/7360#note_1719438
   *
   * TODO: But with current UAPI we can't change the mmap mode in Xe, so
   * here only supporting two memory types.
   */
   device->memory.type_count = 2;
   device->memory.types[0] = (struct anv_memory_type) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      };
   device->memory.types[1] = (struct anv_memory_type) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                           } else {
      return vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
      }
      }
      static VkResult
   anv_xe_get_device_status(struct anv_device *device, uint32_t exec_queue_id)
   {
      VkResult result = VK_SUCCESS;
   struct drm_xe_exec_queue_get_property exec_queue_get_property = {
      .exec_queue_id = exec_queue_id,
      };
   int ret = intel_ioctl(device->fd, DRM_IOCTL_XE_EXEC_QUEUE_GET_PROPERTY,
            if (ret || exec_queue_get_property.value)
               }
      VkResult
   anv_xe_device_check_status(struct vk_device *vk_device)
   {
      struct anv_device *device = container_of(vk_device, struct anv_device, vk);
            for (uint32_t i = 0; i < device->queue_count; i++) {
      result = anv_xe_get_device_status(device, device->queues[i].exec_queue_id);
   if (result != VK_SUCCESS)
            if (device->queues[i].companion_rcs_id != 0) {
      uint32_t exec_queue_id = device->queues[i].companion_rcs_id;
   result = anv_xe_get_device_status(device, exec_queue_id);
   if (result != VK_SUCCESS)
                     }
