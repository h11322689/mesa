   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_instance.h"
      #include "util/driconf.h"
   #include "venus-protocol/vn_protocol_driver_info.h"
   #include "venus-protocol/vn_protocol_driver_instance.h"
   #include "venus-protocol/vn_protocol_driver_transport.h"
      #include "vn_icd.h"
   #include "vn_physical_device.h"
   #include "vn_renderer.h"
      #define VN_INSTANCE_RING_SIZE (128 * 1024)
   #define VN_INSTANCE_RING_DIRECT_THRESHOLD (VN_INSTANCE_RING_SIZE / 16)
      /*
   * Instance extensions add instance-level or physical-device-level
   * functionalities.  It seems renderer support is either unnecessary or
   * optional.  We should be able to advertise them or lie about them locally.
   */
   static const struct vk_instance_extension_table
      vn_instance_supported_extensions = {
      /* promoted to VK_VERSION_1_1 */
   .KHR_device_group_creation = true,
   .KHR_external_fence_capabilities = true,
   .KHR_external_memory_capabilities = true,
   .KHR_external_semaphore_capabilities = true,
      #ifdef VN_USE_WSI_PLATFORM
         .KHR_get_surface_capabilities2 = true,
   .KHR_surface = true,
   #endif
   #ifdef VK_USE_PLATFORM_WAYLAND_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XCB_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XLIB_KHR
         #endif
            static const driOptionDescription vn_dri_options[] = {
      /* clang-format off */
   DRI_CONF_SECTION_PERFORMANCE
      DRI_CONF_VK_X11_ENSURE_MIN_IMAGE_COUNT(false)
   DRI_CONF_VK_X11_OVERRIDE_MIN_IMAGE_COUNT(0)
   DRI_CONF_VK_X11_STRICT_IMAGE_COUNT(false)
   DRI_CONF_VK_XWAYLAND_WAIT_READY(true)
      DRI_CONF_SECTION_END
   DRI_CONF_SECTION_DEBUG
      DRI_CONF_VK_WSI_FORCE_BGRA8_UNORM_FIRST(false)
      DRI_CONF_SECTION_END
      };
      static VkResult
   vn_instance_init_renderer_versions(struct vn_instance *instance)
   {
      uint32_t instance_version = 0;
   VkResult result =
         if (result != VK_SUCCESS) {
      if (VN_DEBUG(INIT))
                     if (instance_version < VN_MIN_RENDERER_VERSION) {
      if (VN_DEBUG(INIT)) {
      vn_log(instance, "unsupported renderer instance version %d.%d",
            }
               if (VN_DEBUG(INIT)) {
      vn_log(instance, "renderer instance version %d.%d.%d",
         VK_VERSION_MAJOR(instance_version),
               /* request at least VN_MIN_RENDERER_VERSION internally */
   instance->renderer_api_version =
            /* instance version for internal use is capped */
   instance_version = MIN3(instance_version, instance->renderer_api_version,
                              }
      static VkResult
   vn_instance_init_ring(struct vn_instance *instance)
   {
      /* 32-bit seqno for renderer roundtrips */
   const size_t extra_size = sizeof(uint32_t);
   struct vn_ring_layout layout;
            instance->ring.shmem =
         if (!instance->ring.shmem) {
      if (VN_DEBUG(INIT))
                              struct vn_ring *ring = &instance->ring.ring;
   vn_ring_init(ring, instance->renderer, &layout,
                     ring->monitor.report_period_us = 3000000;
            /* ring monitor should be alive at all time */
            const struct VkRingMonitorInfoMESA monitor_info = {
      .sType = VK_STRUCTURE_TYPE_RING_MONITOR_INFO_MESA,
      };
   const struct VkRingCreateInfoMESA info = {
      .sType = VK_STRUCTURE_TYPE_RING_CREATE_INFO_MESA,
   .pNext = &monitor_info,
   .resourceId = instance->ring.shmem->res_id,
   .size = layout.shmem_size,
   .idleTimeout = 50ull * 1000 * 1000,
   .headOffset = layout.head_offset,
   .tailOffset = layout.tail_offset,
   .statusOffset = layout.status_offset,
   .bufferOffset = layout.buffer_offset,
   .bufferSize = layout.buffer_size,
   .extraOffset = layout.extra_offset,
               uint32_t create_ring_data[64];
   struct vn_cs_encoder local_enc = VN_CS_ENCODER_INITIALIZER_LOCAL(
         vn_encode_vkCreateRingMESA(&local_enc, 0, instance->ring.id, &info);
   vn_renderer_submit_simple(instance->renderer, create_ring_data,
            vn_cs_encoder_init(&instance->ring.upload, instance,
            mtx_init(&instance->ring.roundtrip_mutex, mtx_plain);
               }
      static VkResult
   vn_instance_init_renderer(struct vn_instance *instance)
   {
               VkResult result = vn_renderer_create(instance, alloc, &instance->renderer);
   if (result != VK_SUCCESS)
            struct vn_renderer_info *renderer_info = &instance->renderer->info;
   uint32_t version = vn_info_wire_format_version();
   if (renderer_info->wire_format_version != version) {
      if (VN_DEBUG(INIT)) {
      vn_log(instance, "wire format version %d != %d",
      }
               version = vn_info_vk_xml_version();
   if (renderer_info->vk_xml_version > version)
         if (renderer_info->vk_xml_version < VN_MIN_RENDERER_VERSION) {
      if (VN_DEBUG(INIT)) {
      vn_log(instance, "vk xml version %d.%d.%d < %d.%d.%d",
         VK_VERSION_MAJOR(renderer_info->vk_xml_version),
   VK_VERSION_MINOR(renderer_info->vk_xml_version),
   VK_VERSION_PATCH(renderer_info->vk_xml_version),
   VK_VERSION_MAJOR(VN_MIN_RENDERER_VERSION),
      }
               uint32_t spec_version =
         if (renderer_info->vk_ext_command_serialization_spec_version >
      spec_version) {
               spec_version = vn_extension_get_spec_version("VK_MESA_venus_protocol");
   if (renderer_info->vk_mesa_venus_protocol_spec_version > spec_version)
            if (VN_DEBUG(INIT)) {
      vn_log(instance, "connected to renderer");
   vn_log(instance, "wire format version %d",
         vn_log(instance, "vk xml version %d.%d.%d",
         VK_VERSION_MAJOR(renderer_info->vk_xml_version),
   VK_VERSION_MINOR(renderer_info->vk_xml_version),
   vn_log(instance, "VK_EXT_command_serialization spec version %d",
         vn_log(instance, "VK_MESA_venus_protocol spec version %d",
         vn_log(instance, "supports blob id 0: %d",
         vn_log(instance, "allow_vk_wait_syncs: %d",
         vn_log(instance, "supports_multiple_timelines: %d",
                  }
      VkResult
   vn_instance_submit_roundtrip(struct vn_instance *instance,
         {
      uint32_t local_data[8];
   struct vn_cs_encoder local_enc =
            mtx_lock(&instance->ring.roundtrip_mutex);
   const uint64_t seqno = instance->ring.roundtrip_next++;
   vn_encode_vkSubmitVirtqueueSeqnoMESA(&local_enc, 0, instance->ring.id,
         VkResult result = vn_renderer_submit_simple(
                  *roundtrip_seqno = seqno;
      }
      void
   vn_instance_wait_roundtrip(struct vn_instance *instance,
         {
         }
      struct vn_instance_submission {
      const struct vn_cs_encoder *cs;
            struct {
      struct vn_cs_encoder cs;
   struct vn_cs_encoder_buffer buffer;
         };
      static const struct vn_cs_encoder *
   vn_instance_submission_get_cs(struct vn_instance_submission *submit,
               {
      if (direct)
            VkCommandStreamDescriptionMESA local_descs[8];
   VkCommandStreamDescriptionMESA *descs = local_descs;
   if (cs->buffer_count > ARRAY_SIZE(local_descs)) {
      descs =
         if (!descs)
               uint32_t desc_count = 0;
   for (uint32_t i = 0; i < cs->buffer_count; i++) {
      const struct vn_cs_encoder_buffer *buf = &cs->buffers[i];
   if (buf->committed_size) {
      descs[desc_count++] = (VkCommandStreamDescriptionMESA){
      .resourceId = buf->shmem->res_id,
   .offset = buf->offset,
                     const size_t exec_size = vn_sizeof_vkExecuteCommandStreamsMESA(
         void *exec_data = submit->indirect.data;
   if (exec_size > sizeof(submit->indirect.data)) {
      exec_data = malloc(exec_size);
   if (!exec_data) {
      if (descs != local_descs)
                        submit->indirect.buffer = VN_CS_ENCODER_BUFFER_INITIALIZER(exec_data);
   submit->indirect.cs =
         vn_encode_vkExecuteCommandStreamsMESA(&submit->indirect.cs, 0, desc_count,
                  if (descs != local_descs)
               }
      static struct vn_ring_submit *
   vn_instance_submission_get_ring_submit(struct vn_ring *ring,
                     {
      const uint32_t shmem_count =
         struct vn_ring_submit *submit = vn_ring_get_submit(ring, shmem_count);
   if (!submit)
            submit->shmem_count = shmem_count;
   if (!direct) {
      for (uint32_t i = 0; i < cs->buffer_count; i++) {
      submit->shmems[i] =
         }
   if (extra_shmem) {
      submit->shmems[shmem_count - 1] =
                  }
      static void
   vn_instance_submission_cleanup(struct vn_instance_submission *submit)
   {
      if (submit->cs == &submit->indirect.cs &&
      submit->indirect.buffer.base != submit->indirect.data)
   }
      static VkResult
   vn_instance_submission_prepare(struct vn_instance_submission *submit,
                           {
      submit->cs = vn_instance_submission_get_cs(submit, cs, direct);
   if (!submit->cs)
            submit->submit =
         if (!submit->submit) {
      vn_instance_submission_cleanup(submit);
                  }
      static inline bool
   vn_instance_submission_can_direct(const struct vn_instance *instance,
         {
         }
      static struct vn_cs_encoder *
   vn_instance_ring_cs_upload_locked(struct vn_instance *instance,
         {
      VN_TRACE_FUNC();
   assert(cs->storage_type == VN_CS_ENCODER_STORAGE_POINTER &&
         const void *cs_data = cs->buffers[0].base;
   const size_t cs_size = cs->total_committed_size;
            struct vn_cs_encoder *upload = &instance->ring.upload;
            if (!vn_cs_encoder_reserve(upload, cs_size))
            vn_cs_encoder_write(upload, cs_size, cs_data, cs_size);
               }
      static VkResult
   vn_instance_ring_submit_locked(struct vn_instance *instance,
                     {
               const bool direct = vn_instance_submission_can_direct(instance, cs);
   if (!direct && cs->storage_type == VN_CS_ENCODER_STORAGE_POINTER) {
      cs = vn_instance_ring_cs_upload_locked(instance, cs);
   if (!cs)
                     struct vn_instance_submission submit;
   VkResult result =
         if (result != VK_SUCCESS)
            uint32_t seqno;
   const bool notify = vn_ring_submit(ring, submit.submit, submit.cs, &seqno);
   if (notify) {
      uint32_t notify_ring_data[8];
   struct vn_cs_encoder local_enc = VN_CS_ENCODER_INITIALIZER_LOCAL(
         vn_encode_vkNotifyRingMESA(&local_enc, 0, instance->ring.id, seqno, 0);
   vn_renderer_submit_simple(instance->renderer, notify_ring_data,
                        if (ring_seqno)
               }
      VkResult
   vn_instance_ring_submit(struct vn_instance *instance,
         {
      mtx_lock(&instance->ring.mutex);
   VkResult result = vn_instance_ring_submit_locked(instance, cs, NULL, NULL);
               }
      static struct vn_renderer_shmem *
   vn_instance_get_reply_shmem_locked(struct vn_instance *instance,
               {
      VN_TRACE_FUNC();
   struct vn_renderer_shmem_pool *pool = &instance->reply_shmem_pool;
            size_t offset;
   struct vn_renderer_shmem *shmem =
         if (!shmem)
            assert(shmem == pool->shmem);
            if (shmem != saved_pool_shmem) {
      uint32_t set_reply_command_stream_data[16];
   struct vn_cs_encoder local_enc = VN_CS_ENCODER_INITIALIZER_LOCAL(
      set_reply_command_stream_data,
      const struct VkCommandStreamDescriptionMESA stream = {
      .resourceId = shmem->res_id,
      };
   vn_encode_vkSetReplyCommandStreamMESA(&local_enc, 0, &stream);
   vn_cs_encoder_commit(&local_enc);
               /* TODO avoid this seek command and go lock-free? */
   uint32_t seek_reply_command_stream_data[8];
   struct vn_cs_encoder local_enc = VN_CS_ENCODER_INITIALIZER_LOCAL(
         vn_encode_vkSeekReplyCommandStreamMESA(&local_enc, 0, offset);
   vn_cs_encoder_commit(&local_enc);
               }
      void
   vn_instance_submit_command(struct vn_instance *instance,
         {
      void *reply_ptr = NULL;
                     if (vn_cs_encoder_is_empty(&submit->command))
                  if (submit->reply_size) {
      submit->reply_shmem = vn_instance_get_reply_shmem_locked(
         if (!submit->reply_shmem)
               submit->ring_seqno_valid =
      VK_SUCCESS == vn_instance_ring_submit_locked(instance, &submit->command,
                        if (submit->reply_size) {
      submit->reply =
            if (submit->ring_seqno_valid)
                     fail:
      instance->ring.command_dropped++;
      }
      /* instance commands */
      VkResult
   vn_EnumerateInstanceVersion(uint32_t *pApiVersion)
   {
      *pApiVersion = VN_MAX_API_VERSION;
      }
      VkResult
   vn_EnumerateInstanceExtensionProperties(const char *pLayerName,
               {
      if (pLayerName)
            return vk_enumerate_instance_extension_properties(
      }
      VkResult
   vn_EnumerateInstanceLayerProperties(uint32_t *pPropertyCount,
         {
      *pPropertyCount = 0;
      }
      VkResult
   vn_CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
               {
      vn_trace_init();
            const VkAllocationCallbacks *alloc =
         struct vn_instance *instance;
                     instance = vk_zalloc(alloc, sizeof(*instance), VN_DEFAULT_ALIGN,
         if (!instance)
            struct vk_instance_dispatch_table dispatch_table;
   vk_instance_dispatch_table_from_entrypoints(
         vk_instance_dispatch_table_from_entrypoints(
         result = vn_instance_base_init(&instance->base,
               if (result != VK_SUCCESS) {
      vk_free(alloc, instance);
               /* ring_idx = 0 reserved for CPU timeline */
            mtx_init(&instance->physical_device.mutex, mtx_plain);
   mtx_init(&instance->cs_shmem.mutex, mtx_plain);
            if (!vn_icd_supports_api_version(
            result = VK_ERROR_INCOMPATIBLE_DRIVER;
               if (pCreateInfo->enabledLayerCount) {
      result = VK_ERROR_LAYER_NOT_PRESENT;
               result = vn_instance_init_renderer(instance);
   if (result != VK_SUCCESS)
                     vn_renderer_shmem_pool_init(instance->renderer,
            result = vn_instance_init_ring(instance);
   if (result != VK_SUCCESS)
            result = vn_instance_init_renderer_versions(instance);
   if (result != VK_SUCCESS)
            vn_renderer_shmem_pool_init(instance->renderer, &instance->cs_shmem.pool,
            VkInstanceCreateInfo local_create_info = *pCreateInfo;
   local_create_info.ppEnabledExtensionNames = NULL;
   local_create_info.enabledExtensionCount = 0;
            VkApplicationInfo local_app_info;
   if (instance->base.base.app_info.api_version <
      instance->renderer_api_version) {
   if (pCreateInfo->pApplicationInfo) {
      local_app_info = *pCreateInfo->pApplicationInfo;
      } else {
      local_app_info = (const VkApplicationInfo){
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
         }
               VkInstance instance_handle = vn_instance_to_handle(instance);
   result =
         if (result != VK_SUCCESS)
            driParseOptionInfo(&instance->available_dri_options, vn_dri_options,
         driParseConfigFiles(&instance->dri_options,
                     &instance->available_dri_options, 0, "venus", NULL,
            instance->renderer->info.has_implicit_fencing =
                           fail:
      if (instance->ring.shmem) {
      uint32_t destroy_ring_data[4];
   struct vn_cs_encoder local_enc = VN_CS_ENCODER_INITIALIZER_LOCAL(
         vn_encode_vkDestroyRingMESA(&local_enc, 0, instance->ring.id);
   vn_renderer_submit_simple(instance->renderer, destroy_ring_data,
            mtx_destroy(&instance->ring.roundtrip_mutex);
   vn_cs_encoder_fini(&instance->ring.upload);
   vn_renderer_shmem_unref(instance->renderer, instance->ring.shmem);
   vn_ring_fini(&instance->ring.ring);
               vn_renderer_shmem_pool_fini(instance->renderer,
            if (instance->renderer)
            mtx_destroy(&instance->physical_device.mutex);
   mtx_destroy(&instance->ring_idx_mutex);
            vn_instance_base_fini(&instance->base);
               }
      void
   vn_DestroyInstance(VkInstance _instance,
         {
      VN_TRACE_FUNC();
   struct vn_instance *instance = vn_instance_from_handle(_instance);
   const VkAllocationCallbacks *alloc =
            if (!instance)
            if (instance->physical_device.initialized) {
      for (uint32_t i = 0; i < instance->physical_device.device_count; i++)
         vk_free(alloc, instance->physical_device.devices);
      }
   mtx_destroy(&instance->physical_device.mutex);
                     vn_renderer_shmem_pool_fini(instance->renderer, &instance->cs_shmem.pool);
            uint32_t destroy_ring_data[4];
   struct vn_cs_encoder local_enc = VN_CS_ENCODER_INITIALIZER_LOCAL(
         vn_encode_vkDestroyRingMESA(&local_enc, 0, instance->ring.id);
   vn_renderer_submit_simple(instance->renderer, destroy_ring_data,
            mtx_destroy(&instance->ring.roundtrip_mutex);
   vn_cs_encoder_fini(&instance->ring.upload);
   vn_ring_fini(&instance->ring.ring);
   mtx_destroy(&instance->ring.mutex);
            vn_renderer_shmem_pool_fini(instance->renderer,
                     driDestroyOptionCache(&instance->dri_options);
            vn_instance_base_fini(&instance->base);
      }
      PFN_vkVoidFunction
   vn_GetInstanceProcAddr(VkInstance _instance, const char *pName)
   {
      struct vn_instance *instance = vn_instance_from_handle(_instance);
   return vk_instance_get_proc_addr(&instance->base.base,
      }
