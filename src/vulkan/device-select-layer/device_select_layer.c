   /*
   * Copyright © 2017 Google
   * Copyright © 2019 Red Hat
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
      /* Rules for device selection.
   * Is there an X or wayland connection open (or DISPLAY set).
   * If no - try and find which device was the boot_vga device.
   * If yes - try and work out which device is the connection primary,
   * DRI_PRIME tagged overrides only work if bus info, =1 will just pick an alternate.
   */
      #include <vulkan/vk_layer.h>
   #include <vulkan/vulkan.h>
      #include <assert.h>
   #include <stdio.h>
   #include <string.h>
   #include <fcntl.h>
   #include <unistd.h>
      #include "device_select.h"
   #include "util/hash_table.h"
   #include "vk_util.h"
   #include "util/simple_mtx.h"
   #include "util/u_debug.h"
      struct instance_info {
      PFN_vkDestroyInstance DestroyInstance;
   PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
   PFN_vkEnumeratePhysicalDeviceGroups EnumeratePhysicalDeviceGroups;
   PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
   PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties;
   PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
   PFN_vkGetPhysicalDeviceProperties2 GetPhysicalDeviceProperties2;
   bool has_pci_bus, has_vulkan11;
      };
      static struct hash_table *device_select_instance_ht = NULL;
   static simple_mtx_t device_select_mutex = SIMPLE_MTX_INITIALIZER;
      static void
   device_select_init_instances(void)
   {
      simple_mtx_lock(&device_select_mutex);
   if (!device_select_instance_ht)
      device_select_instance_ht = _mesa_hash_table_create(NULL, _mesa_hash_pointer,
         }
      static void
   device_select_try_free_ht(void)
   {
      simple_mtx_lock(&device_select_mutex);
   if (device_select_instance_ht) {
      _mesa_hash_table_destroy(device_select_instance_ht, NULL);
   device_select_instance_ht = NULL;
            }
      }
      static void
   device_select_layer_add_instance(VkInstance instance, struct instance_info *info)
   {
      device_select_init_instances();
   simple_mtx_lock(&device_select_mutex);
   _mesa_hash_table_insert(device_select_instance_ht, instance, info);
      }
      static struct instance_info *
   device_select_layer_get_instance(VkInstance instance)
   {
      struct hash_entry *entry;
   struct instance_info *info = NULL;
   simple_mtx_lock(&device_select_mutex);
   entry = _mesa_hash_table_search(device_select_instance_ht, (void *)instance);
   if (entry)
         simple_mtx_unlock(&device_select_mutex);
      }
      static void
   device_select_layer_remove_instance(VkInstance instance)
   {
      simple_mtx_lock(&device_select_mutex);
   _mesa_hash_table_remove_key(device_select_instance_ht, instance);
   simple_mtx_unlock(&device_select_mutex);
      }
      static VkResult device_select_CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
               {
      VkLayerInstanceCreateInfo *chain_info;
   for(chain_info = (VkLayerInstanceCreateInfo*)pCreateInfo->pNext; chain_info; chain_info = (VkLayerInstanceCreateInfo*)chain_info->pNext)
      if(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chain_info->function == VK_LAYER_LINK_INFO)
         assert(chain_info->u.pLayerInfo);
            info->GetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
   PFN_vkCreateInstance fpCreateInstance =
         if (fpCreateInstance == NULL) {
      free(info);
                        VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
   if (result != VK_SUCCESS) {
      free(info);
                  #ifdef VK_USE_PLATFORM_WAYLAND_KHR
         if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME))
   #endif
   #ifdef VK_USE_PLATFORM_XCB_KHR
         if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME))
   #endif
               /*
   * The loader is currently not able to handle GetPhysicalDeviceProperties2KHR calls in
   * EnumeratePhysicalDevices when there are other layers present. To avoid mysterious crashes
   * for users just use only the vulkan version for now.
   */
   info->has_vulkan11 = pCreateInfo->pApplicationInfo &&
         #define DEVSEL_GET_CB(func) info->func = (PFN_vk##func)info->GetInstanceProcAddr(*pInstance, "vk" #func)
      DEVSEL_GET_CB(DestroyInstance);
   DEVSEL_GET_CB(EnumeratePhysicalDevices);
   DEVSEL_GET_CB(EnumeratePhysicalDeviceGroups);
   DEVSEL_GET_CB(GetPhysicalDeviceProperties);
   DEVSEL_GET_CB(EnumerateDeviceExtensionProperties);
   if (info->has_vulkan11)
      #undef DEVSEL_GET_CB
                     }
      static void device_select_DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
   {
               device_select_layer_remove_instance(instance);
   info->DestroyInstance(instance, pAllocator);
      }
      static void get_device_properties(const struct instance_info *info, VkPhysicalDevice device, VkPhysicalDeviceProperties2 *properties)
   {
               if (info->GetPhysicalDeviceProperties2 && properties->properties.apiVersion >= VK_API_VERSION_1_1)
      }
      static void print_gpu(const struct instance_info *info, unsigned index, VkPhysicalDevice device)
   {
      const char *type = "";
   VkPhysicalDevicePCIBusInfoPropertiesEXT ext_pci_properties = (VkPhysicalDevicePCIBusInfoPropertiesEXT) {
         };
   VkPhysicalDeviceProperties2 properties = (VkPhysicalDeviceProperties2){
         };
   if (info->has_vulkan11 && info->has_pci_bus)
                  switch(properties.properties.deviceType) {
   case VK_PHYSICAL_DEVICE_TYPE_OTHER:
   default:
      type = "other";
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      type = "integrated GPU";
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      type = "discrete GPU";
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      type = "virtual GPU";
      case VK_PHYSICAL_DEVICE_TYPE_CPU:
      type = "CPU";
      }
   fprintf(stderr, "  GPU %d: %x:%x \"%s\" %s", index, properties.properties.vendorID,
         if (info->has_vulkan11 && info->has_pci_bus)
      fprintf(stderr, " %04x:%02x:%02x.%x", ext_pci_properties.pciDomain,
               }
      static bool fill_drm_device_info(const struct instance_info *info,
               {
      VkPhysicalDevicePCIBusInfoPropertiesEXT ext_pci_properties = (VkPhysicalDevicePCIBusInfoPropertiesEXT) {
                  VkPhysicalDeviceProperties2 properties = (VkPhysicalDeviceProperties2){
                  if (info->has_vulkan11 && info->has_pci_bus)
                  drm_device->cpu_device = properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
   drm_device->dev_info.vendor_id = properties.properties.vendorID;
   drm_device->dev_info.device_id = properties.properties.deviceID;
   if (info->has_vulkan11 && info->has_pci_bus) {
   drm_device->has_bus_info = true;
   drm_device->bus_info.domain = ext_pci_properties.pciDomain;
   drm_device->bus_info.bus = ext_pci_properties.pciBus;
   drm_device->bus_info.dev = ext_pci_properties.pciDevice;
   drm_device->bus_info.func = ext_pci_properties.pciFunction;
   }
      }
      static int device_select_find_explicit_default(struct device_pci_info *pci_infos,
               {
      int default_idx = -1;
   unsigned vendor_id, device_id;
   int matched = sscanf(selection, "%x:%x", &vendor_id, &device_id);
   if (matched != 2)
            for (unsigned i = 0; i < device_count; ++i) {
      if (pci_infos[i].dev_info.vendor_id == vendor_id &&
      pci_infos[i].dev_info.device_id == device_id)
   }
      }
      static int device_select_find_dri_prime_tag_default(struct device_pci_info *pci_infos,
               {
               /* Drop the trailing '!' if present. */
   int ref = strlen("pci-xxxx_yy_zz_w");
   int n = strlen(dri_prime);
   if (n < ref)
         if (n == ref + 1 && dri_prime[n - 1] == '!')
            for (unsigned i = 0; i < device_count; ++i) {
      char *tag = NULL;
   if (asprintf(&tag, "pci-%04x_%02x_%02x_%1u",
               pci_infos[i].bus_info.domain,
   pci_infos[i].bus_info.bus,
      if (strncmp(dri_prime, tag, n) == 0)
      }
      }
      }
      static int device_select_find_boot_vga_vid_did(struct device_pci_info *pci_infos,
         {
      char path[1024];
   int fd;
   int default_idx = -1;
   uint8_t boot_vga = 0;
   ssize_t size_ret;
   #pragma pack(push, 1)
   struct id {
      uint16_t vid;
      }id;
            for (unsigned i = 0; i < 64; i++) {
      snprintf(path, 1023, "/sys/class/drm/card%d/device/boot_vga", i);
   fd = open(path, O_RDONLY);
   if (fd != -1) {
      uint8_t val;
   size_ret = read(fd, &val, 1);
   close(fd);
   if (size_ret == 1 && val == '1')
      } else {
                  if (boot_vga) {
      snprintf(path, 1023, "/sys/class/drm/card%d/device/config", i);
   fd = open(path, O_RDONLY);
   if (fd != -1) {
      size_ret = read(fd, &id, 4);
   close(fd);
   if (size_ret != 4)
      } else {
         }
                  if (!boot_vga)
            for (unsigned i = 0; i < device_count; ++i) {
      if (id.vid == pci_infos[i].dev_info.vendor_id &&
      id.did == pci_infos[i].dev_info.device_id) {
   default_idx = i;
                     }
      static int device_select_find_boot_vga_default(struct device_pci_info *pci_infos,
         {
      char boot_vga_path[1024];
   int default_idx = -1;
   for (unsigned i = 0; i < device_count; ++i) {
      /* fallback to probing the pci bus boot_vga device. */
   snprintf(boot_vga_path, 1023, "/sys/bus/pci/devices/%04x:%02x:%02x.%x/boot_vga", pci_infos[i].bus_info.domain,
         int fd = open(boot_vga_path, O_RDONLY);
   if (fd != -1) {
      uint8_t val;
   if (read(fd, &val, 1) == 1) {
      if (val == '1')
      }
      }
   if (default_idx != -1)
      }
      }
      static int device_select_find_non_cpu(struct device_pci_info *pci_infos,
         {
               /* pick first GPU device */
   for (unsigned i = 0; i < device_count; ++i) {
      if (!pci_infos[i].cpu_device){
      default_idx = i;
         }
      }
      static int find_non_cpu_skip(struct device_pci_info *pci_infos,
                     {
      for (unsigned i = 0; i < device_count; ++i) {
      if (i == skip_idx)
         if (pci_infos[i].cpu_device)
         skip_count--;
   if (skip_count > 0)
               }
      }
      static bool should_debug_device_selection() {
      return debug_get_bool_option("MESA_VK_DEVICE_SELECT_DEBUG", false) ||
      }
      static bool ends_with_exclamation_mark(const char *str) {
      size_t n = strlen(str);
      }
      static uint32_t get_default_device(const struct instance_info *info,
                           {
      int default_idx = -1;
   const char *dri_prime = getenv("DRI_PRIME");
   bool debug = should_debug_device_selection();
   int dri_prime_as_int = -1;
   int cpu_count = 0;
   if (dri_prime) {
      if (strchr(dri_prime, ':') == NULL)
            if (dri_prime_as_int < 0)
               struct device_pci_info *pci_infos = (struct device_pci_info *)calloc(physical_device_count, sizeof(struct device_pci_info));
   if (!pci_infos)
            for (unsigned i = 0; i < physical_device_count; ++i) {
                  if (selection)
         if (default_idx != -1) {
                  if (default_idx == -1 && dri_prime && dri_prime_as_int == 0) {
      /* Try DRI_PRIME=vendor_id:device_id */
   default_idx = device_select_find_explicit_default(pci_infos, physical_device_count, dri_prime);
   if (default_idx != -1) {
      if (debug)
                     if (default_idx == -1) {
      /* Try DRI_PRIME=pci-xxxx_yy_zz_w */
   if (!info->has_vulkan11 && !info->has_pci_bus)
                        if (default_idx != -1) {
      if (debug)
                  }
   if (default_idx == -1 && info->has_wayland) {
      default_idx = device_select_find_wayland_pci_default(pci_infos, physical_device_count);
   if (debug && default_idx != -1)
      }
   if (default_idx == -1 && info->has_xcb) {
      default_idx = device_select_find_xcb_pci_default(pci_infos, physical_device_count);
   if (debug && default_idx != -1)
      }
   if (default_idx == -1) {
      if (info->has_vulkan11 && info->has_pci_bus)
         else
         if (debug && default_idx != -1)
      }
   if (default_idx == -1 && cpu_count) {
      default_idx = device_select_find_non_cpu(pci_infos, physical_device_count);
   if (debug && default_idx != -1)
      }
   /* If no GPU has been selected so far, select the first non-CPU device. If none are available,
   * pick the first CPU device.
   */
   if (default_idx == -1) {
      default_idx = device_select_find_non_cpu(pci_infos, physical_device_count);
   if (default_idx != -1) {
      if (debug)
      } else if (cpu_count) {
            }
   /* DRI_PRIME=n handling - pick any other device than default. */
   if (dri_prime_as_int > 0 && debug)
         if (dri_prime_as_int > 0 && physical_device_count > (cpu_count + 1)) {
      if (default_idx == 0 || default_idx == 1) {
      default_idx = find_non_cpu_skip(pci_infos, physical_device_count, default_idx, dri_prime_as_int);
   if (default_idx != -1) {
      if (debug)
                  }
   free(pci_infos);
      }
      static VkResult device_select_EnumeratePhysicalDevices(VkInstance instance,
               {
      struct instance_info *info = device_select_layer_get_instance(instance);
   uint32_t physical_device_count = 0;
   uint32_t selected_physical_device_count = 0;
   const char* selection = getenv("MESA_VK_DEVICE_SELECT");
   bool expose_only_one_dev = false;
   VkResult result = info->EnumeratePhysicalDevices(instance, &physical_device_count, NULL);
   VK_OUTARRAY_MAKE_TYPED(VkPhysicalDevice, out, pPhysicalDevices, pPhysicalDeviceCount);
   if (result != VK_SUCCESS)
            VkPhysicalDevice *physical_devices = (VkPhysicalDevice*)calloc(sizeof(VkPhysicalDevice),  physical_device_count);
   VkPhysicalDevice *selected_physical_devices = (VkPhysicalDevice*)calloc(sizeof(VkPhysicalDevice),
            if (!physical_devices || !selected_physical_devices) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               result = info->EnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);
   if (result != VK_SUCCESS)
            for (unsigned i = 0; i < physical_device_count; i++) {
      uint32_t count;
   info->EnumerateDeviceExtensionProperties(physical_devices[i], NULL, &count, NULL);
   VkExtensionProperties *extensions = calloc(count, sizeof(VkExtensionProperties));
            for (unsigned j = 0; j < count; j++) {
               if (!strcmp(extensions[j].extensionName, VK_EXT_PCI_BUS_INFO_EXTENSION_NAME))
      free(extensions);
            }
   if (should_debug_device_selection() || (selection && strcmp(selection, "list") == 0)) {
      fprintf(stderr, "selectable devices:\n");
   for (unsigned i = 0; i < physical_device_count; ++i)
            if (selection && strcmp(selection, "list") == 0)
               unsigned selected_index = get_default_device(info, selection, physical_device_count,
         selected_physical_device_count = physical_device_count;
   selected_physical_devices[0] = physical_devices[selected_index];
   for (unsigned i = 0; i < physical_device_count - 1; ++i) {
      unsigned  this_idx = i < selected_index ? i : i + 1;
               if (selected_physical_device_count == 0) {
                           /* do not give multiple device option to app if force default device */
   const char *force_default_device = getenv("MESA_VK_DEVICE_SELECT_FORCE_DEFAULT_DEVICE");
   if (force_default_device && !strcmp(force_default_device, "1") && selected_physical_device_count != 0)
            if (expose_only_one_dev)
            for (unsigned i = 0; i < selected_physical_device_count; i++) {
      vk_outarray_append_typed(VkPhysicalDevice, &out, ent) {
            }
      out:
      free(physical_devices);
   free(selected_physical_devices);
      }
      static VkResult device_select_EnumeratePhysicalDeviceGroups(VkInstance instance,
               {
      struct instance_info *info = device_select_layer_get_instance(instance);
   uint32_t physical_device_group_count = 0;
   uint32_t selected_physical_device_group_count = 0;
   VkResult result = info->EnumeratePhysicalDeviceGroups(instance, &physical_device_group_count, NULL);
            if (result != VK_SUCCESS)
            VkPhysicalDeviceGroupProperties *physical_device_groups = (VkPhysicalDeviceGroupProperties*)calloc(sizeof(VkPhysicalDeviceGroupProperties), physical_device_group_count);
            if (!physical_device_groups || !selected_physical_device_groups) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               for (unsigned i = 0; i < physical_device_group_count; i++)
            result = info->EnumeratePhysicalDeviceGroups(instance, &physical_device_group_count, physical_device_groups);
   if (result != VK_SUCCESS)
            /* just sort groups with CPU devices to the end? - assume nobody will mix these */
   int num_gpu_groups = 0;
   int num_cpu_groups = 0;
   selected_physical_device_group_count = physical_device_group_count;
   for (unsigned i = 0; i < physical_device_group_count; i++) {
      bool group_has_cpu_device = false;
   for (unsigned j = 0; j < physical_device_groups[i].physicalDeviceCount; j++) {
      VkPhysicalDevice physical_device = physical_device_groups[i].physicalDevices[j];
   VkPhysicalDeviceProperties2 properties = (VkPhysicalDeviceProperties2){
         };
   info->GetPhysicalDeviceProperties(physical_device, &properties.properties);
               if (group_has_cpu_device) {
      selected_physical_device_groups[physical_device_group_count - num_cpu_groups - 1] = physical_device_groups[i];
      } else {
      selected_physical_device_groups[num_gpu_groups] = physical_device_groups[i];
                           for (unsigned i = 0; i < selected_physical_device_group_count; i++) {
      vk_outarray_append_typed(VkPhysicalDeviceGroupProperties, &out, ent) {
            }
      out:
      free(physical_device_groups);
   free(selected_physical_device_groups);
      }
      static void  (*get_instance_proc_addr(VkInstance instance, const char* name))()
   {
      if (strcmp(name, "vkGetInstanceProcAddr") == 0)
         if (strcmp(name, "vkCreateInstance") == 0)
         if (strcmp(name, "vkDestroyInstance") == 0)
         if (strcmp(name, "vkEnumeratePhysicalDevices") == 0)
         if (strcmp(name, "vkEnumeratePhysicalDeviceGroups") == 0)
            struct instance_info *info = device_select_layer_get_instance(instance);
      }
      PUBLIC VkResult vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface *pVersionStruct)
   {
      if (pVersionStruct->loaderLayerInterfaceVersion < 2)
                              }
