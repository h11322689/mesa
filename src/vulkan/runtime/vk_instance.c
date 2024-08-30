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
      #include "vk_instance.h"
      #include "util/libdrm.h"
   #include "util/perf/cpu_trace.h"
      #include "vk_alloc.h"
   #include "vk_common_entrypoints.h"
   #include "vk_dispatch_trampolines.h"
   #include "vk_log.h"
   #include "vk_util.h"
   #include "vk_debug_utils.h"
   #include "vk_physical_device.h"
      #include "compiler/glsl_types.h"
      #define VERSION_IS_1_0(version) \
            static const struct debug_control trace_options[] = {
      {"rmv", VK_TRACE_MODE_RMV},
      };
      VkResult
   vk_instance_init(struct vk_instance *instance,
                  const struct vk_instance_extension_table *supported_extensions,
      {
      memset(instance, 0, sizeof(*instance));
   vk_object_base_instance_init(instance, &instance->base, VK_OBJECT_TYPE_INSTANCE);
                     /* VK_EXT_debug_utils */
   /* These messengers will only be used during vkCreateInstance or
   * vkDestroyInstance calls.  We do this first so that it's safe to use
   * vk_errorf and friends.
   */
   list_inithead(&instance->debug_utils.instance_callbacks);
   vk_foreach_struct_const(ext, pCreateInfo->pNext) {
      if (ext->sType ==
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT) {
   const VkDebugUtilsMessengerCreateInfoEXT *debugMessengerCreateInfo =
         struct vk_debug_utils_messenger *messenger =
                                                messenger->alloc = *alloc;
   messenger->severity = debugMessengerCreateInfo->messageSeverity;
   messenger->type = debugMessengerCreateInfo->messageType;
                  list_addtail(&messenger->link,
                  uint32_t instance_version = VK_API_VERSION_1_0;
   if (dispatch_table->EnumerateInstanceVersion)
            instance->app_info = (struct vk_app_info) { .api_version = 0 };
   if (pCreateInfo->pApplicationInfo) {
               instance->app_info.app_name =
      vk_strdup(&instance->alloc, app->pApplicationName,
               instance->app_info.engine_name =
      vk_strdup(&instance->alloc, app->pEngineName,
                           /* From the Vulkan 1.2.199 spec:
   *
   *    "Note:
   *
   *    Providing a NULL VkInstanceCreateInfo::pApplicationInfo or providing
   *    an apiVersion of 0 is equivalent to providing an apiVersion of
   *    VK_MAKE_API_VERSION(0,1,0,0)."
   */
   if (instance->app_info.api_version == 0)
            /* From the Vulkan 1.2.199 spec:
   *
   *    VUID-VkApplicationInfo-apiVersion-04010
   *
   *    "If apiVersion is not 0, then it must be greater than or equal to
   *    VK_API_VERSION_1_0"
   */
            /* From the Vulkan 1.2.199 spec:
   *
   *    "Vulkan 1.0 implementations were required to return
   *    VK_ERROR_INCOMPATIBLE_DRIVER if apiVersion was larger than 1.0.
   *    Implementations that support Vulkan 1.1 or later must not return
   *    VK_ERROR_INCOMPATIBLE_DRIVER for any value of apiVersion."
   */
   if (VERSION_IS_1_0(instance_version) &&
      !VERSION_IS_1_0(instance->app_info.api_version))
                  for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
      int idx;
   for (idx = 0; idx < VK_INSTANCE_EXTENSION_COUNT; idx++) {
      if (strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                     if (idx >= VK_INSTANCE_EXTENSION_COUNT)
      return vk_errorf(instance, VK_ERROR_EXTENSION_NOT_PRESENT,
               if (!supported_extensions->extensions[idx])
      return vk_errorf(instance, VK_ERROR_EXTENSION_NOT_PRESENT,
         #ifdef ANDROID
         if (!vk_android_allowed_instance_extensions.extensions[idx])
      return vk_errorf(instance, VK_ERROR_EXTENSION_NOT_PRESENT,
      #endif
                                 /* Add common entrypoints without overwriting driver-provided ones. */
   vk_instance_dispatch_table_from_entrypoints(
            if (mtx_init(&instance->debug_report.callbacks_mutex, mtx_plain) != 0)
                     if (mtx_init(&instance->debug_utils.callbacks_mutex, mtx_plain) != 0) {
      mtx_destroy(&instance->debug_report.callbacks_mutex);
                                 if (mtx_init(&instance->physical_devices.mutex, mtx_plain) != 0) {
      mtx_destroy(&instance->debug_report.callbacks_mutex);
   mtx_destroy(&instance->debug_utils.callbacks_mutex);
               instance->trace_mode = parse_debug_string(getenv("MESA_VK_TRACE"), trace_options);
   instance->trace_frame = (uint32_t)debug_get_num_option("MESA_VK_TRACE_FRAME", 0xFFFFFFFF);
                        }
      static void
   destroy_physical_devices(struct vk_instance *instance)
   {
      list_for_each_entry_safe(struct vk_physical_device, pdevice,
            list_del(&pdevice->link);
         }
      void
   vk_instance_finish(struct vk_instance *instance)
   {
               glsl_type_singleton_decref();
   if (unlikely(!list_is_empty(&instance->debug_utils.callbacks))) {
      list_for_each_entry_safe(struct vk_debug_utils_messenger, messenger,
            list_del(&messenger->link);
   vk_object_base_finish(&messenger->base);
         }
   if (unlikely(!list_is_empty(&instance->debug_utils.instance_callbacks))) {
      list_for_each_entry_safe(struct vk_debug_utils_messenger, messenger,
                  list_del(&messenger->link);
   vk_object_base_finish(&messenger->base);
         }
   mtx_destroy(&instance->debug_report.callbacks_mutex);
   mtx_destroy(&instance->debug_utils.callbacks_mutex);
   mtx_destroy(&instance->physical_devices.mutex);
   vk_free(&instance->alloc, (char *)instance->app_info.app_name);
   vk_free(&instance->alloc, (char *)instance->app_info.engine_name);
      }
      VkResult
   vk_enumerate_instance_extension_properties(
      const struct vk_instance_extension_table *supported_extensions,
   uint32_t *pPropertyCount,
      {
               for (int i = 0; i < VK_INSTANCE_EXTENSION_COUNT; i++) {
      if (!supported_extensions->extensions[i])
      #ifdef ANDROID
         if (!vk_android_allowed_instance_extensions.extensions[i])
   #endif
            vk_outarray_append_typed(VkExtensionProperties, &out, prop) {
                        }
      PFN_vkVoidFunction
   vk_instance_get_proc_addr(const struct vk_instance *instance,
               {
               /* The Vulkan 1.0 spec for vkGetInstanceProcAddr has a table of exactly
   * when we have to return valid function pointers, NULL, or it's left
   * undefined.  See the table for exact details.
   */
   if (name == NULL)
         #define LOOKUP_VK_ENTRYPOINT(entrypoint) \
      if (strcmp(name, "vk" #entrypoint) == 0) \
            LOOKUP_VK_ENTRYPOINT(EnumerateInstanceExtensionProperties);
   LOOKUP_VK_ENTRYPOINT(EnumerateInstanceLayerProperties);
   LOOKUP_VK_ENTRYPOINT(EnumerateInstanceVersion);
            /* GetInstanceProcAddr() can also be called with a NULL instance.
   * See https://gitlab.khronos.org/vulkan/vulkan/issues/2057
   */
         #undef LOOKUP_VK_ENTRYPOINT
         if (instance == NULL)
            func = vk_instance_dispatch_table_get_if_supported(&instance->dispatch_table,
                     if (func != NULL)
            func = vk_physical_device_dispatch_table_get_if_supported(&vk_physical_device_trampolines,
                     if (func != NULL)
            func = vk_device_dispatch_table_get_if_supported(&vk_device_trampolines,
                           if (func != NULL)
               }
      PFN_vkVoidFunction
   vk_instance_get_proc_addr_unchecked(const struct vk_instance *instance,
         {
               if (instance == NULL || name == NULL)
            func = vk_instance_dispatch_table_get(&instance->dispatch_table, name);
   if (func != NULL)
            func = vk_physical_device_dispatch_table_get(
         if (func != NULL)
            func = vk_device_dispatch_table_get(&vk_device_trampolines, name);
   if (func != NULL)
               }
      PFN_vkVoidFunction
   vk_instance_get_physical_device_proc_addr(const struct vk_instance *instance,
         {
      if (instance == NULL || name == NULL)
            return vk_physical_device_dispatch_table_get_if_supported(&vk_physical_device_trampolines,
                  }
      void
   vk_instance_add_driver_trace_modes(struct vk_instance *instance,
         {
         }
      static VkResult
   enumerate_drm_physical_devices_locked(struct vk_instance *instance)
   {
      /* TODO: Check for more devices ? */
   drmDevicePtr devices[8];
            if (max_devices < 1)
            VkResult result;
   for (uint32_t i = 0; i < (uint32_t)max_devices; i++) {
      struct vk_physical_device *pdevice;
            /* Incompatible DRM device, skip. */
   if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
      result = VK_SUCCESS;
               /* Error creating the physical device, report the error. */
   if (result != VK_SUCCESS)
                        drmFreeDevices(devices, max_devices);
      }
      static VkResult
   enumerate_physical_devices_locked(struct vk_instance *instance)
   {
      if (instance->physical_devices.enumerate) {
      VkResult result = instance->physical_devices.enumerate(instance);
   if (result != VK_ERROR_INCOMPATIBLE_DRIVER)
                        if (instance->physical_devices.try_create_for_drm) {
      result = enumerate_drm_physical_devices_locked(instance);
   if (result != VK_SUCCESS) {
      destroy_physical_devices(instance);
                     }
      static VkResult
   enumerate_physical_devices(struct vk_instance *instance)
   {
               mtx_lock(&instance->physical_devices.mutex);
   if (!instance->physical_devices.enumerated) {
      result = enumerate_physical_devices_locked(instance);
   if (result == VK_SUCCESS)
      }
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_EnumeratePhysicalDevices(VkInstance _instance, uint32_t *pPhysicalDeviceCount,
         {
      VK_FROM_HANDLE(vk_instance, instance, _instance);
            VkResult result = enumerate_physical_devices(instance);
   if (result != VK_SUCCESS)
            list_for_each_entry(struct vk_physical_device, pdevice,
            vk_outarray_append_typed(VkPhysicalDevice, &out, element) {
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_EnumeratePhysicalDeviceGroups(VkInstance _instance, uint32_t *pGroupCount,
         {
      VK_FROM_HANDLE(vk_instance, instance, _instance);
   VK_OUTARRAY_MAKE_TYPED(VkPhysicalDeviceGroupProperties, out, pGroupProperties,
            VkResult result = enumerate_physical_devices(instance);
   if (result != VK_SUCCESS)
            list_for_each_entry(struct vk_physical_device, pdevice,
            vk_outarray_append_typed(VkPhysicalDeviceGroupProperties, &out, p) {
      p->physicalDeviceCount = 1;
   memset(p->physicalDevices, 0, sizeof(p->physicalDevices));
   p->physicalDevices[0] = vk_physical_device_to_handle(pdevice);
                     }
