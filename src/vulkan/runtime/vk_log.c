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
      #include "vk_log.h"
   #include "vk_debug_utils.h"
   #include "vk_debug_report.h"
      #include "vk_command_buffer.h"
   #include "vk_enum_to_str.h"
   #include "vk_queue.h"
   #include "vk_device.h"
   #include "vk_physical_device.h"
      #include "util/ralloc.h"
   #include "util/log.h"
      static struct vk_device *
   vk_object_to_device(struct vk_object_base *obj)
   {
      assert(obj->device);
      }
      static struct vk_physical_device *
   vk_object_to_physical_device(struct vk_object_base *obj)
   {
      switch (obj->type) {
   case VK_OBJECT_TYPE_INSTANCE:
         case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
         case VK_OBJECT_TYPE_SURFACE_KHR:
   case VK_OBJECT_TYPE_DISPLAY_KHR:
   case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
   case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
   case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
         default:
            }
      static struct vk_instance *
   vk_object_to_instance(struct vk_object_base *obj)
   {
      if (obj == NULL)
            if (obj->type == VK_OBJECT_TYPE_INSTANCE) {
         } else {
            }
      void
   __vk_log_impl(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
               VkDebugUtilsMessageTypeFlagsEXT types,
   int object_count,
   const void **objects_or_instance,
   const char *file,
   int line,
   {
      struct vk_instance *instance = NULL;
   struct vk_object_base **objects = NULL;
   if (object_count == 0) {
         } else {
      objects = (struct vk_object_base **) objects_or_instance;
   for (unsigned i = 0; i < object_count; i++) {
      if (unlikely(objects[i] == NULL)) {
      mesa_logw("vk_log*() called with NULL object\n");
               if (unlikely(!objects[i]->client_visible)) {
      mesa_logw("vk_log*() called with client-invisible object %p "
                     if (!instance) {
      instance = vk_object_to_instance(objects[i]);
      } else {
         }
               #ifndef DEBUG
      if (unlikely(!instance) ||
      (likely(list_is_empty(&instance->debug_utils.callbacks)) &&
   likely(list_is_empty(&instance->debug_report.callbacks))))
   #endif
         va_list va;
            va_start(va, format);
   message = ralloc_vasprintf(NULL, format, va);
                  #if DEBUG
      switch (severity) {
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      mesa_logd("%s: %s", message_idname, message);
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      mesa_logi("%s: %s", message_idname, message);
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      if (types & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
         else
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      mesa_loge("%s: %s", message_idname, message);
      default:
      unreachable("Invalid debug message severity");
               if (!instance) {
      ralloc_free(message);
   ralloc_free(message_idname);
         #endif
         if (!instance->base.client_visible) {
      vk_debug_message_instance(instance, severity, types,
         ralloc_free(message);
   ralloc_free(message_idname);
               /* If VK_EXT_debug_utils messengers have been set up, form the
   * message */
   if (!list_is_empty(&instance->debug_utils.callbacks)) {
      VkDebugUtilsMessengerCallbackDataEXT cb_data = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT,
   .pMessageIdName = message_idname,
   .messageIdNumber = 0,
               VkDebugUtilsObjectNameInfoEXT *object_name_infos =
            ASSERTED int cmdbuf_n = 0, queue_n = 0, obj_n = 0;
   for (int i = 0; i < object_count; i++) {
      struct vk_object_base *base = objects[i];
                  switch (base->type) {
   case VK_OBJECT_TYPE_COMMAND_BUFFER: {
      /* We allow at most one command buffer to be submitted at a time */
   assert(++cmdbuf_n <= 1);
   struct vk_command_buffer *cmd_buffer =
         if (cmd_buffer->labels.size > 0) {
      cb_data.cmdBufLabelCount = util_dynarray_num_elements(
            }
               case VK_OBJECT_TYPE_QUEUE: {
      /* We allow at most one queue to be submitted at a time */
   assert(++queue_n <= 1);
   struct vk_queue *queue = (struct vk_queue *)base;
   if (queue->labels.size > 0) {
      cb_data.queueLabelCount =
            }
      }
   default:
                  object_name_infos[obj_n++] = (VkDebugUtilsObjectNameInfoEXT){
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
   .pNext = NULL,
   .objectType = base->type,
   .objectHandle = (uint64_t)(uintptr_t)base,
         }
   cb_data.objectCount = obj_n;
                                 /* If VK_EXT_debug_report callbacks also have been set up, forward
   * the message there as well */
   if (!list_is_empty(&instance->debug_report.callbacks)) {
               switch (severity) {
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      if (types & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
         else
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
      default:
      unreachable("Invalid debug message severity");
               /* VK_EXT_debug_report-provided callback accepts only one object
   * related to the message. Since they are given to us in
   * decreasing order of importance, we're forwarding the first
   * one.
   */
   vk_debug_report(instance, flags, object_count ? objects[0] : NULL, 0,
               ralloc_free(message);
      }
      static struct vk_object_base *
   vk_object_for_error(struct vk_object_base *obj, VkResult error)
   {
      if (obj == NULL)
            switch (error) {
   case VK_ERROR_OUT_OF_HOST_MEMORY:
   case VK_ERROR_LAYER_NOT_PRESENT:
   case VK_ERROR_EXTENSION_NOT_PRESENT:
   case VK_ERROR_UNKNOWN:
         case VK_ERROR_FEATURE_NOT_PRESENT:
         case VK_ERROR_OUT_OF_DEVICE_MEMORY:
   case VK_ERROR_MEMORY_MAP_FAILED:
   case VK_ERROR_TOO_MANY_OBJECTS:
         default:
            }
      VkResult
   __vk_errorv(const void *_obj, VkResult error,
               {
      struct vk_object_base *object = (struct vk_object_base *)_obj;
   struct vk_instance *instance = vk_object_to_instance(object);
            /* If object->client_visible isn't set then the object hasn't been fully
   * constructed and we shouldn't hand it back to the client.  This typically
   * happens if an error is thrown during object construction.  This is safe
   * to do as long as vk_object_base_init() has already been called.
   */
   if (object && !object->client_visible)
                     if (format) {
               if (object) {
      __vk_log(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
   } else {
      __vk_log(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
               } else {
      if (object) {
      __vk_log(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
   } else {
      __vk_log(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
                  }
      VkResult
   __vk_errorf(const void *_obj, VkResult error,
               {
               va_start(va, format);
   VkResult result = __vk_errorv(_obj, error, file, line, format, va);
               }
