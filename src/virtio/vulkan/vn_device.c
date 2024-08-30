   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_device.h"
      #include "util/disk_cache.h"
   #include "util/hex.h"
   #include "venus-protocol/vn_protocol_driver_device.h"
      #include "vn_android.h"
   #include "vn_instance.h"
   #include "vn_physical_device.h"
   #include "vn_queue.h"
      /* device commands */
      static void
   vn_queue_fini(struct vn_queue *queue)
   {
               if (queue->wait_fence != VK_NULL_HANDLE) {
         }
   if (queue->sparse_semaphore != VK_NULL_HANDLE) {
         }
      }
      static VkResult
   vn_queue_init(struct vn_device *dev,
               struct vn_queue *queue,
   {
      VkResult result =
         if (result != VK_SUCCESS)
            VkDeviceQueueTimelineInfoMESA timeline_info;
   const struct vn_renderer_info *renderer_info =
         if (renderer_info->supports_multiple_timelines) {
      int ring_idx = vn_instance_acquire_ring_idx(dev->instance);
   if (ring_idx < 0) {
      vn_log(dev->instance, "failed binding VkQueue to renderer timeline");
      }
            timeline_info = (VkDeviceQueueTimelineInfoMESA){
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_TIMELINE_INFO_MESA,
                  const VkDeviceQueueInfo2 device_queue_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
   .pNext =
         .flags = queue_info->flags,
   .queueFamilyIndex = queue_info->queueFamilyIndex,
               VkQueue queue_handle = vn_queue_to_handle(queue);
   vn_async_vkGetDeviceQueue2(dev->instance, vn_device_to_handle(dev),
               }
      static VkResult
   vn_device_init_queues(struct vn_device *dev,
         {
               uint32_t count = 0;
   for (uint32_t i = 0; i < create_info->queueCreateInfoCount; i++)
            struct vn_queue *queues =
      vk_zalloc(alloc, sizeof(*queues) * count, VN_DEFAULT_ALIGN,
      if (!queues)
            count = 0;
   for (uint32_t i = 0; i < create_info->queueCreateInfoCount; i++) {
               const VkDeviceQueueCreateInfo *queue_info =
         for (uint32_t j = 0; j < queue_info->queueCount; j++) {
      result = vn_queue_init(dev, &queues[count], queue_info, j);
   if (result != VK_SUCCESS) {
      for (uint32_t k = 0; k < count; k++)
                                             dev->queues = queues;
               }
      static bool
   vn_device_queue_family_init(struct vn_device *dev,
         {
      const VkAllocationCallbacks *alloc = &dev->base.base.alloc;
   uint32_t *queue_families = NULL;
            queue_families = vk_zalloc(
      alloc, sizeof(*queue_families) * create_info->queueCreateInfoCount,
      if (!queue_families)
            for (uint32_t i = 0; i < create_info->queueCreateInfoCount; i++) {
      const uint32_t index =
                  for (uint32_t j = 0; j < count; j++) {
      if (queue_families[j] == index) {
      new_index = false;
         }
   if (new_index)
               dev->queue_families = queue_families;
               }
      static inline void
   vn_device_queue_family_fini(struct vn_device *dev)
   {
         }
      static VkResult
   vn_device_memory_report_init(struct vn_device *dev,
         {
      const struct vk_features *app_feats = &dev->base.base.enabled_features;
   if (!app_feats->deviceMemoryReport)
            uint32_t count = 0;
   vk_foreach_struct_const(pnext, create_info->pNext) {
      if (pnext->sType ==
      VK_STRUCTURE_TYPE_DEVICE_DEVICE_MEMORY_REPORT_CREATE_INFO_EXT)
            struct vn_device_memory_report *mem_reports = NULL;
   if (count) {
      mem_reports =
      vk_alloc(&dev->base.base.alloc, sizeof(*mem_reports) * count,
      if (!mem_reports)
               count = 0;
   vk_foreach_struct_const(pnext, create_info->pNext) {
      if (pnext->sType ==
      VK_STRUCTURE_TYPE_DEVICE_DEVICE_MEMORY_REPORT_CREATE_INFO_EXT) {
   const struct VkDeviceDeviceMemoryReportCreateInfoEXT *report =
         mem_reports[count].callback = report->pfnUserCallback;
   mem_reports[count].data = report->pUserData;
                  dev->memory_report_count = count;
               }
      static inline void
   vn_device_memory_report_fini(struct vn_device *dev)
   {
         }
      static bool
   find_extension_names(const char *const *exts,
               {
      for (uint32_t i = 0; i < ext_count; i++) {
      if (!strcmp(exts[i], name))
      }
      }
      static bool
   merge_extension_names(const char *const *exts,
                        uint32_t ext_count,
   const char *const *extra_exts,
   uint32_t extra_count,
   const char *const *block_exts,
      {
      const char **merged =
      vk_alloc(alloc, sizeof(*merged) * (ext_count + extra_count),
      if (!merged)
            uint32_t count = 0;
   for (uint32_t i = 0; i < ext_count; i++) {
      if (!find_extension_names(block_exts, block_count, exts[i]))
      }
   for (uint32_t i = 0; i < extra_count; i++) {
      if (!find_extension_names(exts, ext_count, extra_exts[i]))
               *out_exts = merged;
   *out_count = count;
      }
      static const VkDeviceCreateInfo *
   vn_device_fix_create_info(const struct vn_device *dev,
                     {
      const struct vn_physical_device *physical_dev = dev->physical_device;
   const struct vk_device_extension_table *app_exts =
         /* extra_exts and block_exts must not overlap */
   const char *extra_exts[16];
   const char *block_exts[16];
   uint32_t extra_count = 0;
            /* fix for WSI (treat AHB as WSI extension for simplicity) */
   const bool has_wsi =
      app_exts->KHR_swapchain || app_exts->ANDROID_native_buffer ||
      if (has_wsi) {
      if (!app_exts->EXT_image_drm_format_modifier) {
                     if (physical_dev->renderer_version < VK_API_VERSION_1_2 &&
      !app_exts->KHR_image_format_list) {
   extra_exts[extra_count++] =
                  if (!app_exts->EXT_queue_family_foreign) {
      extra_exts[extra_count++] =
               if (app_exts->KHR_swapchain) {
      /* see vn_physical_device_get_native_extensions */
   block_exts[block_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
   block_exts[block_count++] =
         block_exts[block_count++] =
               if (app_exts->ANDROID_native_buffer) {
      /* see vn_QueueSignalReleaseImageANDROID */
   if (!app_exts->KHR_external_fence_fd) {
      assert(physical_dev->renderer_sync_fd.fence_exportable);
   extra_exts[extra_count++] =
                           if (app_exts->ANDROID_external_memory_android_hardware_buffer) {
      block_exts[block_count++] =
                  if (app_exts->KHR_external_memory_fd ||
      app_exts->EXT_external_memory_dma_buf || has_wsi) {
   if (physical_dev->external_memory.renderer_handle_type ==
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT) {
   if (!app_exts->EXT_external_memory_dma_buf) {
      extra_exts[extra_count++] =
      }
   if (!app_exts->KHR_external_memory_fd) {
      extra_exts[extra_count++] =
                     /* see vn_queue_submission_count_batch_semaphores */
   if (!app_exts->KHR_external_semaphore_fd && has_wsi) {
      assert(physical_dev->renderer_sync_fd.semaphore_importable);
               if (app_exts->EXT_device_memory_report) {
      /* see vn_physical_device_get_native_extensions */
               if (app_exts->EXT_physical_device_drm) {
      /* see vn_physical_device_get_native_extensions */
               if (app_exts->EXT_tooling_info) {
      /* see vn_physical_device_get_native_extensions */
               if (app_exts->EXT_pci_bus_info) {
      /* always filter for simplicity */
               assert(extra_count <= ARRAY_SIZE(extra_exts));
            if (!extra_count && (!block_count || !dev_info->enabledExtensionCount))
            *local_info = *dev_info;
   if (!merge_extension_names(dev_info->ppEnabledExtensionNames,
                                       }
      static inline VkResult
   vn_device_feedback_pool_init(struct vn_device *dev)
   {
      /* The feedback pool defaults to suballocate slots of 8 bytes each. Initial
   * pool size of 4096 corresponds to a total of 512 fences, semaphores and
   * events, which well covers the common scenarios. Pool can grow anyway.
   */
   static const uint32_t pool_size = 4096;
            if (VN_PERF(NO_EVENT_FEEDBACK) && VN_PERF(NO_FENCE_FEEDBACK) &&
      VN_PERF(NO_TIMELINE_SEM_FEEDBACK))
            }
      static inline void
   vn_device_feedback_pool_fini(struct vn_device *dev)
   {
      if (VN_PERF(NO_EVENT_FEEDBACK) && VN_PERF(NO_FENCE_FEEDBACK) &&
      VN_PERF(NO_TIMELINE_SEM_FEEDBACK))
            }
      static void
   vn_device_update_shader_cache_id(struct vn_device *dev)
   {
      /* venus utilizes the host side shader cache.
   * This is a WA to generate shader cache files containing headers
   * with a unique cache id that will change based on host driver
   * identifiers. This allows fossilize replay to detect if the host
   * side shader cach is no longer up to date.
   * The shader cache is destroyed after creating the necessary files
   * and not utilized by venus.
      #if !defined(ANDROID) && defined(ENABLE_SHADER_CACHE)
      const VkPhysicalDeviceProperties *vulkan_1_0_props =
            char uuid[VK_UUID_SIZE * 2 + 1];
            struct disk_cache *cache = disk_cache_create("venus", uuid, 0);
   if (!cache)
            /* The entry header is what contains the cache id / timestamp so we
   * need to create a fake entry.
   */
   uint8_t key[20];
            disk_cache_compute_key(cache, data, sizeof(data), key);
               #endif
   }
      static VkResult
   vn_device_init(struct vn_device *dev,
                     {
      struct vn_instance *instance = physical_dev->instance;
   VkPhysicalDevice physical_dev_handle =
         VkDevice dev_handle = vn_device_to_handle(dev);
   VkDeviceCreateInfo local_create_info;
            dev->instance = instance;
   dev->physical_device = physical_dev;
            create_info =
         if (!create_info)
            result = vn_call_vkCreateDevice(instance, physical_dev_handle, create_info,
            /* free the fixed extensions here since no longer needed below */
   if (create_info == &local_create_info)
            if (result != VK_SUCCESS)
            result = vn_device_memory_report_init(dev, create_info);
   if (result != VK_SUCCESS)
            if (!vn_device_queue_family_init(dev, create_info)) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               for (uint32_t i = 0; i < ARRAY_SIZE(dev->memory_pools); i++) {
      struct vn_device_memory_pool *pool = &dev->memory_pools[i];
               result = vn_device_feedback_pool_init(dev);
   if (result != VK_SUCCESS)
            result = vn_feedback_cmd_pools_init(dev);
   if (result != VK_SUCCESS)
            result = vn_device_init_queues(dev, create_info);
   if (result != VK_SUCCESS)
                     /* This is a WA to allow fossilize replay to detect if the host side shader
   * cache is no longer up to date.
   */
                  out_cmd_pools_fini:
            out_feedback_pool_fini:
            out_memory_pool_fini:
      for (uint32_t i = 0; i < ARRAY_SIZE(dev->memory_pools); i++)
                  out_memory_report_fini:
            out_destroy_device:
                  }
      VkResult
   vn_CreateDevice(VkPhysicalDevice physicalDevice,
                     {
      VN_TRACE_FUNC();
   struct vn_physical_device *physical_dev =
         struct vn_instance *instance = physical_dev->instance;
   const VkAllocationCallbacks *alloc =
         struct vn_device *dev;
            dev = vk_zalloc(alloc, sizeof(*dev), VN_DEFAULT_ALIGN,
         if (!dev)
            struct vk_device_dispatch_table dispatch_table;
   vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         result = vn_device_base_init(&dev->base, &physical_dev->base,
         if (result != VK_SUCCESS) {
      vk_free(alloc, dev);
               result = vn_device_init(dev, physical_dev, pCreateInfo, alloc);
   if (result != VK_SUCCESS) {
      vn_device_base_fini(&dev->base);
   vk_free(alloc, dev);
               if (VN_DEBUG(LOG_CTX_INFO)) {
      vn_log(instance, "%s", physical_dev->properties.vulkan_1_0.deviceName);
                           }
      void
   vn_DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator)
   {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            if (!dev)
                     for (uint32_t i = 0; i < dev->queue_count; i++)
                              for (uint32_t i = 0; i < ARRAY_SIZE(dev->memory_pools); i++)
                              /* We must emit vkDestroyDevice before freeing dev->queues.  Otherwise,
   * another thread might reuse their object ids while they still refer to
   * the queues in the renderer.
   */
            /* We must emit vn_call_vkDestroyDevice before releasing bound ring_idx.
   * Otherwise, another thread might reuse their ring_idx while they
   * are still bound to the queues in the renderer.
   */
   if (dev->instance->renderer->info.supports_multiple_timelines) {
      for (uint32_t i = 0; i < dev->queue_count; i++) {
                              vn_device_base_fini(&dev->base);
      }
      PFN_vkVoidFunction
   vn_GetDeviceProcAddr(VkDevice device, const char *pName)
   {
      struct vn_device *dev = vn_device_from_handle(device);
      }
      void
   vn_GetDeviceGroupPeerMemoryFeatures(
      VkDevice device,
   uint32_t heapIndex,
   uint32_t localDeviceIndex,
   uint32_t remoteDeviceIndex,
      {
               /* TODO get and cache the values in vkCreateDevice */
   vn_call_vkGetDeviceGroupPeerMemoryFeatures(
      dev->instance, device, heapIndex, localDeviceIndex, remoteDeviceIndex,
   }
      VkResult
   vn_GetCalibratedTimestampsEXT(
      VkDevice device,
   uint32_t timestampCount,
   const VkCalibratedTimestampInfoEXT *pTimestampInfos,
   uint64_t *pTimestamps,
      {
      struct vn_device *dev = vn_device_from_handle(device);
   uint64_t begin, end, max_clock_period = 0;
   VkResult ret;
         #ifdef CLOCK_MONOTONIC_RAW
         #else
         #endif
         for (domain = 0; domain < timestampCount; domain++) {
      switch (pTimestampInfos[domain].timeDomain) {
   case VK_TIME_DOMAIN_DEVICE_EXT: {
               ret = vn_call_vkGetCalibratedTimestampsEXT(
                                 max_clock_period = MAX2(max_clock_period, device_max_deviation);
      }
   case VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT:
      pTimestamps[domain] = vk_clock_gettime(CLOCK_MONOTONIC);
      #ifdef CLOCK_MONOTONIC_RAW
         case VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT:
         #endif
         default:
      pTimestamps[domain] = 0;
               #ifdef CLOCK_MONOTONIC_RAW
         #else
         #endif
                     }
