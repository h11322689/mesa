   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   *
   * based in part on anv driver which is:
   * Copyright © 2015 Intel Corporation
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
      #include "radv_private.h"
      static void
   radv_destroy_event(struct radv_device *device, const VkAllocationCallbacks *pAllocator, struct radv_event *event)
   {
      if (event->bo)
            vk_object_base_finish(&event->base);
      }
      VkResult
   radv_create_event(struct radv_device *device, const VkEventCreateInfo *pCreateInfo,
         {
      enum radeon_bo_domain bo_domain;
   enum radeon_bo_flag bo_flags;
   struct radv_event *event;
            event = vk_alloc2(&device->vk.alloc, pAllocator, sizeof(*event), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!event)
                     if (pCreateInfo->flags & VK_EVENT_CREATE_DEVICE_ONLY_BIT) {
      bo_domain = RADEON_DOMAIN_VRAM;
      } else {
      bo_domain = RADEON_DOMAIN_GTT;
               result = device->ws->buffer_create(device->ws, 8, 8, bo_domain,
               if (result != VK_SUCCESS) {
      radv_destroy_event(device, pAllocator, event);
               if (!(pCreateInfo->flags & VK_EVENT_CREATE_DEVICE_ONLY_BIT)) {
      event->map = (uint64_t *)device->ws->buffer_map(event->bo);
   if (!event->map) {
      radv_destroy_event(device, pAllocator, event);
                  *pEvent = radv_event_to_handle(event);
   radv_rmv_log_event_create(device, *pEvent, pCreateInfo->flags, is_internal);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateEvent(VkDevice _device, const VkEventCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
   VkResult result = radv_create_event(device, pCreateInfo, pAllocator, pEvent, false);
   if (result != VK_SUCCESS)
               }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyEvent(VkDevice _device, VkEvent _event, const VkAllocationCallbacks *pAllocator)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!event)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetEventStatus(VkDevice _device, VkEvent _event)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (vk_device_is_lost(&device->vk))
            if (*event->map == 1)
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_SetEvent(VkDevice _device, VkEvent _event)
   {
      RADV_FROM_HANDLE(radv_event, event, _event);
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_ResetEvent(VkDevice _device, VkEvent _event)
   {
      RADV_FROM_HANDLE(radv_event, event, _event);
               }
