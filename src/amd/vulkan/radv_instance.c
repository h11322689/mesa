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
      #include "radv_debug.h"
   #include "radv_private.h"
      #include "util/driconf.h"
      #include "vk_instance.h"
   #include "vk_util.h"
      static const struct debug_control radv_debug_options[] = {{"nofastclears", RADV_DEBUG_NO_FAST_CLEARS},
                                                            {"nodcc", RADV_DEBUG_NO_DCC},
   {"shaders", RADV_DEBUG_DUMP_SHADERS},
   {"nocache", RADV_DEBUG_NO_CACHE},
   {"shaderstats", RADV_DEBUG_DUMP_SHADER_STATS},
   {"nohiz", RADV_DEBUG_NO_HIZ},
   {"nocompute", RADV_DEBUG_NO_COMPUTE_QUEUE},
   {"allbos", RADV_DEBUG_ALL_BOS},
   {"noibs", RADV_DEBUG_NO_IBS},
   {"spirv", RADV_DEBUG_DUMP_SPIRV},
   {"zerovram", RADV_DEBUG_ZERO_VRAM},
   {"syncshaders", RADV_DEBUG_SYNC_SHADERS},
   {"preoptir", RADV_DEBUG_PREOPTIR},
   {"nodynamicbounds", RADV_DEBUG_NO_DYNAMIC_BOUNDS},
   {"info", RADV_DEBUG_INFO},
   {"startup", RADV_DEBUG_STARTUP},
   {"checkir", RADV_DEBUG_CHECKIR},
   {"nobinning", RADV_DEBUG_NOBINNING},
   {"nongg", RADV_DEBUG_NO_NGG},
   {"metashaders", RADV_DEBUG_DUMP_META_SHADERS},
   {"nomemorycache", RADV_DEBUG_NO_MEMORY_CACHE},
   {"discardtodemote", RADV_DEBUG_DISCARD_TO_DEMOTE},
   {"llvm", RADV_DEBUG_LLVM},
   {"forcecompress", RADV_DEBUG_FORCE_COMPRESS},
   {"hang", RADV_DEBUG_HANG},
   {"img", RADV_DEBUG_IMG},
   {"noumr", RADV_DEBUG_NO_UMR},
   {"invariantgeom", RADV_DEBUG_INVARIANT_GEOM},
   {"splitfma", RADV_DEBUG_SPLIT_FMA},
   {"nodisplaydcc", RADV_DEBUG_NO_DISPLAY_DCC},
   {"notccompatcmask", RADV_DEBUG_NO_TC_COMPAT_CMASK},
   {"novrsflatshading", RADV_DEBUG_NO_VRS_FLAT_SHADING},
   {"noatocdithering", RADV_DEBUG_NO_ATOC_DITHERING},
   {"nonggc", RADV_DEBUG_NO_NGGC},
         const char *
   radv_get_debug_option_name(int id)
   {
      assert(id < ARRAY_SIZE(radv_debug_options) - 1);
      }
      static const struct debug_control radv_perftest_options[] = {{"localbos", RADV_PERFTEST_LOCAL_BOS},
                                                               {"dccmsaa", RADV_PERFTEST_DCC_MSAA},
   {"bolist", RADV_PERFTEST_BO_LIST},
   {"cswave32", RADV_PERFTEST_CS_WAVE_32},
   {"pswave32", RADV_PERFTEST_PS_WAVE_32},
   {"gewave32", RADV_PERFTEST_GE_WAVE_32},
      const char *
   radv_get_perftest_option_name(int id)
   {
      assert(id < ARRAY_SIZE(radv_perftest_options) - 1);
      }
      static const struct debug_control trace_options[] = {
      {"rgp", RADV_TRACE_MODE_RGP},
   {"rra", RADV_TRACE_MODE_RRA},
      };
      // clang-format off
   static const driOptionDescription radv_dri_options[] = {
      DRI_CONF_SECTION_PERFORMANCE
      DRI_CONF_ADAPTIVE_SYNC(true)
   DRI_CONF_VK_X11_OVERRIDE_MIN_IMAGE_COUNT(0)
   DRI_CONF_VK_X11_STRICT_IMAGE_COUNT(false)
   DRI_CONF_VK_X11_ENSURE_MIN_IMAGE_COUNT(false)
   DRI_CONF_VK_KHR_PRESENT_WAIT(false)
   DRI_CONF_VK_XWAYLAND_WAIT_READY(true)
   DRI_CONF_RADV_REPORT_LLVM9_VERSION_STRING(false)
   DRI_CONF_RADV_ENABLE_MRT_OUTPUT_NAN_FIXUP(false)
   DRI_CONF_RADV_DISABLE_SHRINK_IMAGE_STORE(false)
   DRI_CONF_RADV_NO_DYNAMIC_BOUNDS(false)
               DRI_CONF_SECTION_DEBUG
      DRI_CONF_OVERRIDE_VRAM_SIZE()
   DRI_CONF_VK_WSI_FORCE_BGRA8_UNORM_FIRST(false)
   DRI_CONF_VK_WSI_FORCE_SWAPCHAIN_TO_CURRENT_EXTENT(false)
   DRI_CONF_VK_REQUIRE_ETC2(false)
   DRI_CONF_VK_REQUIRE_ASTC(false)
   DRI_CONF_RADV_ZERO_VRAM(false)
   DRI_CONF_RADV_LOWER_DISCARD_TO_DEMOTE(false)
   DRI_CONF_RADV_INVARIANT_GEOM(false)
   DRI_CONF_RADV_SPLIT_FMA(false)
   DRI_CONF_RADV_DISABLE_TC_COMPAT_HTILE_GENERAL(false)
   DRI_CONF_RADV_DISABLE_DCC(false)
   DRI_CONF_RADV_DISABLE_ANISO_SINGLE_LEVEL(false)
   DRI_CONF_RADV_DISABLE_TRUNC_COORD(false)
   DRI_CONF_RADV_DISABLE_SINKING_LOAD_INPUT_FS(false)
   DRI_CONF_RADV_DGC(false)
   DRI_CONF_RADV_FLUSH_BEFORE_QUERY_COPY(false)
   DRI_CONF_RADV_ENABLE_UNIFIED_HEAP_ON_APU(false)
   DRI_CONF_RADV_TEX_NON_UNIFORM(false)
   DRI_CONF_RADV_FLUSH_BEFORE_TIMESTAMP_WRITE(false)
   DRI_CONF_RADV_RT_WAVE64(false)
   DRI_CONF_DUAL_COLOR_BLEND_BY_LOCATION(false)
   DRI_CONF_RADV_SSBO_NON_UNIFORM(false)
   DRI_CONF_RADV_FORCE_ACTIVE_ACCEL_STRUCT_LEAVES(false)
         };
   // clang-format on
      static void
   radv_init_dri_options(struct radv_instance *instance)
   {
      driParseOptionInfo(&instance->available_dri_options, radv_dri_options, ARRAY_SIZE(radv_dri_options));
   driParseConfigFiles(&instance->dri_options, &instance->available_dri_options, 0, "radv", NULL, NULL,
                                    instance->disable_tc_compat_htile_in_general =
            if (driQueryOptionb(&instance->dri_options, "radv_no_dynamic_bounds"))
            if (driQueryOptionb(&instance->dri_options, "radv_lower_discard_to_demote"))
            if (driQueryOptionb(&instance->dri_options, "radv_invariant_geom"))
            if (driQueryOptionb(&instance->dri_options, "radv_split_fma"))
            if (driQueryOptionb(&instance->dri_options, "radv_disable_dcc"))
                                       instance->disable_sinking_load_input_fs =
                                                         instance->flush_before_timestamp_write =
                              instance->force_active_accel_struct_leaves =
      }
      static const struct vk_instance_extension_table radv_instance_extensions_supported = {
      .KHR_device_group_creation = true,
   .KHR_external_fence_capabilities = true,
   .KHR_external_memory_capabilities = true,
   .KHR_external_semaphore_capabilities = true,
   .KHR_get_physical_device_properties2 = true,
   .EXT_debug_report = true,
         #ifdef RADV_USE_WSI_PLATFORM
      .KHR_get_surface_capabilities2 = true,
   .KHR_surface = true,
   .KHR_surface_protected_capabilities = true,
   .EXT_surface_maintenance1 = true,
      #endif
   #ifdef VK_USE_PLATFORM_WAYLAND_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XCB_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XLIB_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
         #endif
   #ifdef VK_USE_PLATFORM_DISPLAY_KHR
      .KHR_display = true,
   .KHR_get_display_properties2 = true,
   .EXT_direct_mode_display = true,
   .EXT_display_surface_counter = true,
      #endif
   };
      static void
   radv_handle_legacy_sqtt_trigger(struct vk_instance *instance)
   {
      char *trigger_file = secure_getenv("RADV_THREAD_TRACE_TRIGGER");
   if (trigger_file) {
      instance->trace_trigger_file = trigger_file;
   instance->trace_mode |= RADV_TRACE_MODE_RGP;
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
         {
      struct radv_instance *instance;
            if (!pAllocator)
            instance = vk_zalloc(pAllocator, sizeof(*instance), 8, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
   if (!instance)
            struct vk_instance_dispatch_table dispatch_table;
   vk_instance_dispatch_table_from_entrypoints(&dispatch_table, &radv_instance_entrypoints, true);
            result =
         if (result != VK_SUCCESS) {
      vk_free(pAllocator, instance);
               vk_instance_add_driver_trace_modes(&instance->vk, trace_options);
            instance->debug_flags = parse_debug_string(getenv("RADV_DEBUG"), radv_debug_options);
            /* When RADV_FORCE_FAMILY is set, the driver creates a null
   * device that allows to test the compiler without having an
   * AMDGPU instance.
   */
   if (getenv("RADV_FORCE_FAMILY"))
         else
                     if (instance->debug_flags & RADV_DEBUG_STARTUP)
                                          }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyInstance(VkInstance _instance, const VkAllocationCallbacks *pAllocator)
   {
               if (!instance)
                     driDestroyOptionCache(&instance->dri_options);
            vk_instance_finish(&instance->vk);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pPropertyCount,
         {
      if (pLayerName)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_EnumerateInstanceVersion(uint32_t *pApiVersion)
   {
      *pApiVersion = RADV_API_VERSION;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_EnumerateInstanceLayerProperties(uint32_t *pPropertyCount, VkLayerProperties *pProperties)
   {
      if (pProperties == NULL) {
      *pPropertyCount = 0;
               /* None supported at this time */
      }
      VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   radv_GetInstanceProcAddr(VkInstance _instance, const char *pName)
   {
      RADV_FROM_HANDLE(vk_instance, instance, _instance);
      }
      /* Windows will use a dll definition file to avoid build errors. */
   #ifdef _WIN32
   #undef PUBLIC
   #define PUBLIC
   #endif
      PUBLIC VKAPI_ATTR VkResult VKAPI_CALL
   vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t *pSupportedVersion)
   {
      /* For the full details on loader interface versioning, see
   * <https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/blob/master/loader/LoaderAndLayerInterface.md>.
   * What follows is a condensed summary, to help you navigate the large and
   * confusing official doc.
   *
   *   - Loader interface v0 is incompatible with later versions. We don't
   *     support it.
   *
   *   - In loader interface v1:
   *       - The first ICD entrypoint called by the loader is
   *         vk_icdGetInstanceProcAddr(). The ICD must statically expose this
   *         entrypoint.
   *       - The ICD must statically expose no other Vulkan symbol unless it is
   *         linked with -Bsymbolic.
   *       - Each dispatchable Vulkan handle created by the ICD must be
   *         a pointer to a struct whose first member is VK_LOADER_DATA. The
   *         ICD must initialize VK_LOADER_DATA.loadMagic to ICD_LOADER_MAGIC.
   *       - The loader implements vkCreate{PLATFORM}SurfaceKHR() and
   *         vkDestroySurfaceKHR(). The ICD must be capable of working with
   *         such loader-managed surfaces.
   *
   *    - Loader interface v2 differs from v1 in:
   *       - The first ICD entrypoint called by the loader is
   *         vk_icdNegotiateLoaderICDInterfaceVersion(). The ICD must
   *         statically expose this entrypoint.
   *
   *    - Loader interface v3 differs from v2 in:
   *        - The ICD must implement vkCreate{PLATFORM}SurfaceKHR(),
   *          vkDestroySurfaceKHR(), and other API which uses VKSurfaceKHR,
   *          because the loader no longer does so.
   *
   *    - Loader interface v4 differs from v3 in:
   *        - The ICD must implement vk_icdGetPhysicalDeviceProcAddr().
   *
   *    - Loader interface v5 differs from v4 in:
   *        - The ICD must support Vulkan API version 1.1 and must not return
   *          VK_ERROR_INCOMPATIBLE_DRIVER from vkCreateInstance() unless a
   *          Vulkan Loader with interface v4 or smaller is being used and the
   *          application provides an API version that is greater than 1.0.
   */
   *pSupportedVersion = MIN2(*pSupportedVersion, 5u);
      }
      /* The loader wants us to expose a second GetInstanceProcAddr function
   * to work around certain LD_PRELOAD issues seen in apps.
   */
   PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetInstanceProcAddr(VkInstance instance, const char *pName)
   {
         }
      PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetPhysicalDeviceProcAddr(VkInstance _instance, const char *pName)
   {
      RADV_FROM_HANDLE(radv_instance, instance, _instance);
      }
