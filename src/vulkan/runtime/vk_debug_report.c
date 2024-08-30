   /*
   * Copyright © 2017 Intel Corporation
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
      #include "vk_debug_report.h"
      #include "vk_alloc.h"
   #include "vk_common_entrypoints.h"
   #include "vk_instance.h"
   #include "vk_util.h"
      struct vk_debug_report_callback {
               /* Link in the 'callbacks' list in anv_instance struct. */
   struct list_head                             link;
   VkDebugReportFlagsEXT                        flags;
   PFN_vkDebugReportCallbackEXT                 callback;
      };
      VK_DEFINE_NONDISP_HANDLE_CASTS(vk_debug_report_callback, base,
                  VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_CreateDebugReportCallbackEXT(VkInstance _instance,
                     {
               struct vk_debug_report_callback *cb =
      vk_alloc2(&instance->alloc, pAllocator,
               if (!cb)
            vk_object_base_instance_init(instance, &cb->base,
            cb->flags = pCreateInfo->flags;
   cb->callback = pCreateInfo->pfnCallback;
            mtx_lock(&instance->debug_report.callbacks_mutex);
   list_addtail(&cb->link, &instance->debug_report.callbacks);
                        }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DestroyDebugReportCallbackEXT(VkInstance _instance,
               {
      VK_FROM_HANDLE(vk_instance, instance, _instance);
            if (callback == NULL)
            /* Remove from list and destroy given callback. */
   mtx_lock(&instance->debug_report.callbacks_mutex);
   list_del(&callback->link);
   vk_object_base_finish(&callback->base);
   vk_free2(&instance->alloc, pAllocator, callback);
      }
      static void
   debug_report(struct vk_instance *instance,
               VkDebugReportFlagsEXT flags,
   VkDebugReportObjectTypeEXT object_type,
   uint64_t handle,
   size_t location,
   int32_t messageCode,
   {
      /* Allow NULL for convinience, return if no callbacks registered. */
   if (!instance || list_is_empty(&instance->debug_report.callbacks))
                     /* Section 33.2 of the Vulkan 1.0.59 spec says:
   *
   *    "callback is an externally synchronized object and must not be
   *    used on more than one thread at a time. This means that
   *    vkDestroyDebugReportCallbackEXT must not be called when a callback
   *    is active."
   */
   list_for_each_entry(struct vk_debug_report_callback, cb,
            if (cb->flags & flags)
      cb->callback(flags, object_type, handle, location, messageCode,
               }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DebugReportMessageEXT(VkInstance _instance,
                                 VkDebugReportFlagsEXT flags,
   VkDebugReportObjectTypeEXT objectType,
   {
      VK_FROM_HANDLE(vk_instance, instance, _instance);
   debug_report(instance, flags, objectType,
      }
      void
   vk_debug_report(struct vk_instance *instance,
                  VkDebugReportFlagsEXT flags,
   const struct vk_object_base *object,
   size_t location,
      {
      VkObjectType object_type =
         debug_report(instance, flags, (VkDebugReportObjectTypeEXT)object_type,
            }
