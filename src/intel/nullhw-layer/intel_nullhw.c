   /*
   * Copyright © 2019 Intel Corporation
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
      #include <string.h>
   #include <stdlib.h>
   #include <assert.h>
   #include <stdio.h>
      #include <vulkan/vulkan.h>
   #include <vulkan/vk_layer.h>
      #include "util/u_debug.h"
   #include "util/hash_table.h"
   #include "util/macros.h"
   #include "util/simple_mtx.h"
      #include "vk_dispatch_table.h"
   #include "vk_enum_to_str.h"
   #include "vk_util.h"
      struct instance_data {
      struct vk_instance_dispatch_table vtable;
      };
      struct device_data {
                        struct vk_device_dispatch_table vtable;
   VkPhysicalDevice physical_device;
      };
      static struct hash_table_u64 *vk_object_to_data = NULL;
   static simple_mtx_t vk_object_to_data_mutex = SIMPLE_MTX_INITIALIZER;
      static inline void ensure_vk_object_map(void)
   {
      if (!vk_object_to_data)
      }
      #define HKEY(obj) ((uint64_t)(obj))
   #define FIND(type, obj) ((type *)find_object_data(HKEY(obj)))
      static void *find_object_data(uint64_t obj)
   {
      simple_mtx_lock(&vk_object_to_data_mutex);
   ensure_vk_object_map();
   void *data = _mesa_hash_table_u64_search(vk_object_to_data, obj);
   simple_mtx_unlock(&vk_object_to_data_mutex);
      }
      static void map_object(uint64_t obj, void *data)
   {
      simple_mtx_lock(&vk_object_to_data_mutex);
   ensure_vk_object_map();
   _mesa_hash_table_u64_insert(vk_object_to_data, obj, data);
      }
      static void unmap_object(uint64_t obj)
   {
      simple_mtx_lock(&vk_object_to_data_mutex);
   _mesa_hash_table_u64_remove(vk_object_to_data, obj);
      }
      /**/
      #define VK_CHECK(expr) \
      do { \
      VkResult __result = (expr); \
   if (__result != VK_SUCCESS) { \
      fprintf(stderr, "'%s' line %i failed with %s\n", \
               /**/
      static void override_queue(struct device_data *device_data,
                     {
      VkCommandPoolCreateInfo cmd_buffer_pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      };
   VkCommandPool cmd_pool;
   VK_CHECK(device_data->vtable.CreateCommandPool(device,
                     VkCommandBufferAllocateInfo cmd_buffer_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
   .commandPool = cmd_pool,
   .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      };
   VkCommandBuffer cmd_buffer;
   VK_CHECK(device_data->vtable.AllocateCommandBuffers(device,
                        VkCommandBufferBeginInfo buffer_begin_info = {
         };
            VkPerformanceOverrideInfoINTEL override_info = {
      .sType = VK_STRUCTURE_TYPE_PERFORMANCE_OVERRIDE_INFO_INTEL,
   .type = VK_PERFORMANCE_OVERRIDE_TYPE_NULL_HARDWARE_INTEL,
      };
                     VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
   .commandBufferCount = 1,
      };
                        }
      static void device_override_queues(struct device_data *device_data,
         {
      for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      for (uint32_t j = 0; j < pCreateInfo->pQueueCreateInfos[i].queueCount; j++) {
      VkQueue queue;
   device_data->vtable.GetDeviceQueue(device_data->device,
                           override_queue(device_data, device_data->device,
            }
      static VkLayerDeviceCreateInfo *get_device_chain_info(const VkDeviceCreateInfo *pCreateInfo,
         {
      vk_foreach_struct_const(item, pCreateInfo->pNext) {
      if (item->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO &&
      ((VkLayerDeviceCreateInfo *) item)->function == func)
   }
   unreachable("device chain info not found");
      }
      static struct device_data *new_device_data(VkDevice device, struct instance_data *instance)
   {
      struct device_data *data = calloc(1, sizeof(*data));
   data->instance = instance;
   data->device = device;
   map_object(HKEY(data->device), data);
      }
      static void destroy_device_data(struct device_data *data)
   {
      unmap_object(HKEY(data->device));
      }
      static VkResult nullhw_CreateDevice(
      VkPhysicalDevice                            physicalDevice,
   const VkDeviceCreateInfo*                   pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      VkLayerDeviceCreateInfo *chain_info =
            assert(chain_info->u.pLayerInfo);
   PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
   PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
   PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(NULL, "vkCreateDevice");
   if (fpCreateDevice == NULL) {
                  // Advance the link info for the next element on the chain
            VkDeviceCreateInfo device_info = *pCreateInfo;
   const char **extensions = calloc(device_info.enabledExtensionCount + 1, sizeof(*extensions));
   bool found = false;
   for (uint32_t i = 0; i < device_info.enabledExtensionCount; i++) {
      if (!strcmp(device_info.ppEnabledExtensionNames[i], "VK_INTEL_performance_query")) {
      found = true;
         }
   if (!found) {
      memcpy(extensions, device_info.ppEnabledExtensionNames,
         extensions[device_info.enabledExtensionCount++] = "VK_INTEL_performance_query";
               VkResult result = fpCreateDevice(physicalDevice, &device_info, pAllocator, pDevice);
   free(extensions);
            struct instance_data *instance_data = FIND(struct instance_data, physicalDevice);
   struct device_data *device_data = new_device_data(*pDevice, instance_data);
   device_data->physical_device = physicalDevice;
            VkLayerDeviceCreateInfo *load_data_info =
                              }
      static void nullhw_DestroyDevice(
      VkDevice                                    device,
      {
      struct device_data *device_data = FIND(struct device_data, device);
   device_data->vtable.DestroyDevice(device, pAllocator);
      }
      static struct instance_data *new_instance_data(VkInstance instance)
   {
      struct instance_data *data = calloc(1, sizeof(*data));
   data->instance = instance;
   map_object(HKEY(data->instance), data);
      }
      static void destroy_instance_data(struct instance_data *data)
   {
      unmap_object(HKEY(data->instance));
      }
      static VkLayerInstanceCreateInfo *get_instance_chain_info(const VkInstanceCreateInfo *pCreateInfo,
         {
      vk_foreach_struct_const(item, pCreateInfo->pNext) {
      if (item->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO &&
      ((VkLayerInstanceCreateInfo *) item)->function == func)
   }
   unreachable("instance chain info not found");
      }
      static VkResult nullhw_CreateInstance(
      const VkInstanceCreateInfo*                 pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      VkLayerInstanceCreateInfo *chain_info =
            assert(chain_info->u.pLayerInfo);
   PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr =
         PFN_vkCreateInstance fpCreateInstance =
         if (fpCreateInstance == NULL) {
                  // Advance the link info for the next element on the chain
            VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
            struct instance_data *instance_data = new_instance_data(*pInstance);
   vk_instance_dispatch_table_load(&instance_data->vtable,
                     }
      static void nullhw_DestroyInstance(
      VkInstance                                  instance,
      {
      struct instance_data *instance_data = FIND(struct instance_data, instance);
   instance_data->vtable.DestroyInstance(instance, pAllocator);
      }
      static const struct {
      const char *name;
      } name_to_funcptr_map[] = {
         #define ADD_HOOK(fn) { "vk" # fn, (void *) nullhw_ ## fn }
      ADD_HOOK(CreateInstance),
   ADD_HOOK(DestroyInstance),
   ADD_HOOK(CreateDevice),
      };
      static void *find_ptr(const char *name)
   {
      for (uint32_t i = 0; i < ARRAY_SIZE(name_to_funcptr_map); i++) {
      if (strcmp(name, name_to_funcptr_map[i].name) == 0)
                  }
      PUBLIC VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev,
         {
      void *ptr = find_ptr(funcName);
                     struct device_data *device_data = FIND(struct device_data, dev);
   if (device_data->vtable.GetDeviceProcAddr == NULL) return NULL;
      }
      PUBLIC VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance,
         {
      void *ptr = find_ptr(funcName);
            struct instance_data *instance_data = FIND(struct instance_data, instance);
   if (instance_data->vtable.GetInstanceProcAddr == NULL) return NULL;
      }
