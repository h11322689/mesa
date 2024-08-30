   /*
   * Copyright 2018 Collabora Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "zink_screen.h"
      #include "zink_kopper.h"
   #include "zink_compiler.h"
   #include "zink_context.h"
   #include "zink_descriptors.h"
   #include "zink_fence.h"
   #include "vk_format.h"
   #include "zink_format.h"
   #include "zink_framebuffer.h"
   #include "zink_program.h"
   #include "zink_public.h"
   #include "zink_query.h"
   #include "zink_resource.h"
   #include "zink_state.h"
   #include "nir_to_spirv/nir_to_spirv.h" // for SPIRV_VERSION
      #include "util/u_debug.h"
   #include "util/u_dl.h"
   #include "util/os_file.h"
   #include "util/u_memory.h"
   #include "util/u_screen.h"
   #include "util/u_string.h"
   #include "util/perf/u_trace.h"
   #include "util/u_transfer_helper.h"
   #include "util/hex.h"
   #include "util/xmlconfig.h"
      #include "util/u_cpu_detect.h"
      #ifdef HAVE_LIBDRM
   #include <xf86drm.h>
   #include <fcntl.h>
   #include <sys/stat.h>
   #ifdef MAJOR_IN_MKDEV
   #include <sys/mkdev.h>
   #endif
   #ifdef MAJOR_IN_SYSMACROS
   #include <sys/sysmacros.h>
   #endif
   #endif
      static int num_screens = 0;
   bool zink_tracing = false;
      #if DETECT_OS_WINDOWS
   #include <io.h>
   #define VK_LIBNAME "vulkan-1.dll"
   #else
   #include <unistd.h>
   #if DETECT_OS_APPLE
   #define VK_LIBNAME "libvulkan.1.dylib"
   #elif DETECT_OS_ANDROID
   #define VK_LIBNAME "libvulkan.so"
   #else
   #define VK_LIBNAME "libvulkan.so.1"
   #endif
   #endif
      #if defined(__APPLE__)
   // Source of MVK_VERSION
   #include "MoltenVK/vk_mvk_moltenvk.h"
   #endif
      #ifdef HAVE_LIBDRM
   #include "drm-uapi/dma-buf.h"
   #include <xf86drm.h>
   #endif
      static const struct debug_named_value
   zink_debug_options[] = {
      { "nir", ZINK_DEBUG_NIR, "Dump NIR during program compile" },
   { "spirv", ZINK_DEBUG_SPIRV, "Dump SPIR-V during program compile" },
   { "tgsi", ZINK_DEBUG_TGSI, "Dump TGSI during program compile" },
   { "validation", ZINK_DEBUG_VALIDATION, "Dump Validation layer output" },
   { "vvl", ZINK_DEBUG_VALIDATION, "Dump Validation layer output" },
   { "sync", ZINK_DEBUG_SYNC, "Force synchronization before draws/dispatches" },
   { "compact", ZINK_DEBUG_COMPACT, "Use only 4 descriptor sets" },
   { "noreorder", ZINK_DEBUG_NOREORDER, "Do not reorder command streams" },
   { "gpl", ZINK_DEBUG_GPL, "Force using Graphics Pipeline Library for all shaders" },
   { "shaderdb", ZINK_DEBUG_SHADERDB, "Do stuff to make shader-db work" },
   { "rp", ZINK_DEBUG_RP, "Enable renderpass tracking/optimizations" },
   { "norp", ZINK_DEBUG_NORP, "Disable renderpass tracking/optimizations" },
   { "map", ZINK_DEBUG_MAP, "Track amount of mapped VRAM" },
   { "flushsync", ZINK_DEBUG_FLUSHSYNC, "Force synchronous flushes/presents" },
   { "noshobj", ZINK_DEBUG_NOSHOBJ, "Disable EXT_shader_object" },
   { "optimal_keys", ZINK_DEBUG_OPTIMAL_KEYS, "Debug/use optimal_keys" },
   { "noopt", ZINK_DEBUG_NOOPT, "Disable async optimized pipeline compiles" },
   { "nobgc", ZINK_DEBUG_NOBGC, "Disable all async pipeline compiles" },
   { "dgc", ZINK_DEBUG_DGC, "Use DGC (driver testing only)" },
   { "mem", ZINK_DEBUG_MEM, "Debug memory allocations" },
   { "quiet", ZINK_DEBUG_QUIET, "Suppress warnings" },
      };
      DEBUG_GET_ONCE_FLAGS_OPTION(zink_debug, "ZINK_DEBUG", zink_debug_options, 0)
      uint32_t
   zink_debug;
         static const struct debug_named_value
   zink_descriptor_options[] = {
      { "auto", ZINK_DESCRIPTOR_MODE_AUTO, "Automatically detect best mode" },
   { "lazy", ZINK_DESCRIPTOR_MODE_LAZY, "Don't cache, do least amount of updates" },
   { "db", ZINK_DESCRIPTOR_MODE_DB, "Use descriptor buffers" },
      };
      DEBUG_GET_ONCE_FLAGS_OPTION(zink_descriptor_mode, "ZINK_DESCRIPTORS", zink_descriptor_options, ZINK_DESCRIPTOR_MODE_AUTO)
      enum zink_descriptor_mode zink_descriptor_mode;
      static const char *
   zink_get_vendor(struct pipe_screen *pscreen)
   {
         }
      static const char *
   zink_get_device_vendor(struct pipe_screen *pscreen)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   static char buf[1000];
   snprintf(buf, sizeof(buf), "Unknown (vendor-id: 0x%04x)", screen->info.props.vendorID);
      }
      static const char *
   zink_get_name(struct pipe_screen *pscreen)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   const char *driver_name = vk_DriverId_to_str(screen->info.driver_props.driverID) + strlen("VK_DRIVER_ID_");
   static char buf[1000];
   snprintf(buf, sizeof(buf), "zink Vulkan %d.%d(%s (%s))",
            VK_VERSION_MAJOR(screen->info.device_version),
   VK_VERSION_MINOR(screen->info.device_version),
   screen->info.props.deviceName,
         }
      static void
   zink_get_driver_uuid(struct pipe_screen *pscreen, char *uuid)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   if (screen->vk_version >= VK_MAKE_VERSION(1,2,0)) {
         } else {
            }
      static void
   zink_get_device_uuid(struct pipe_screen *pscreen, char *uuid)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   if (screen->vk_version >= VK_MAKE_VERSION(1,2,0)) {
         } else {
            }
      static void
   zink_get_device_luid(struct pipe_screen *pscreen, char *luid)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   if (screen->info.have_vulkan12) {
         } else {
            }
      static uint32_t
   zink_get_device_node_mask(struct pipe_screen *pscreen)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   if (screen->info.have_vulkan12) {
         } else {
            }
      static void
   zink_set_max_shader_compiler_threads(struct pipe_screen *pscreen, unsigned max_threads)
   {
      struct zink_screen *screen = zink_screen(pscreen);
      }
      static bool
   zink_is_parallel_shader_compilation_finished(struct pipe_screen *screen, void *shader, enum pipe_shader_type shader_type)
   {
      if (shader_type == MESA_SHADER_COMPUTE) {
      struct zink_program *pg = shader;
               struct zink_shader *zs = shader;
   if (!util_queue_fence_is_signalled(&zs->precompile.fence))
         bool finished = true;
   set_foreach(zs->programs, entry) {
      struct zink_gfx_program *prog = (void*)entry->key;
      }
      }
      static VkDeviceSize
   get_video_mem(struct zink_screen *screen)
   {
      VkDeviceSize size = 0;
   for (uint32_t i = 0; i < screen->info.mem_props.memoryHeapCount; ++i) {
      if (screen->info.mem_props.memoryHeaps[i].flags &
      VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
   }
      }
      /**
   * Creates the disk cache used by mesa/st frontend for caching the GLSL -> NIR
   * path.
   *
   * The output that gets stored in the frontend's cache is the result of
   * zink_shader_finalize().  So, our sha1 cache key here needs to include
   * everything that would change the NIR we generate from a given set of GLSL
   * source, including our driver build, the Vulkan device and driver (which could
   * affect the pipe caps we show the frontend), and any debug flags that change
   * codegen.
   *
   * This disk cache also gets used by zink itself for storing its output from NIR
   * -> SPIRV translation.
   */
   static bool
   disk_cache_init(struct zink_screen *screen)
   {
      if (zink_debug & ZINK_DEBUG_SHADERDB)
         #ifdef ENABLE_SHADER_CACHE
      struct mesa_sha1 ctx;
         #ifdef HAVE_DL_ITERATE_PHDR
      /* Hash in the zink driver build. */
   const struct build_id_note *note =
         unsigned build_id_len = build_id_length(note);
   assert(note && build_id_len == 20); /* sha1 */
      #endif
         /* Hash in the Vulkan pipeline cache UUID to identify the combination of
   *  vulkan device and driver (or any inserted layer that would invalidate our
   *  cached pipelines).
   *
   * "Although they have identical descriptions, VkPhysicalDeviceIDProperties
   *  ::deviceUUID may differ from
   *  VkPhysicalDeviceProperties2::pipelineCacheUUID. The former is intended to
   *  identify and correlate devices across API and driver boundaries, while the
   *  latter is used to identify a compatible device and driver combination to
   *  use when serializing and de-serializing pipeline state."
   */
            /* Hash in our debug flags that affect NIR generation as of finalize_nir */
   unsigned shader_debug_flags = zink_debug & ZINK_DEBUG_COMPACT;
            /* Some of the driconf options change shaders.  Let's just hash the whole
   * thing to not forget any (especially as options get added).
   */
            /* EXT_shader_object causes different descriptor layouts for separate shaders */
            /* Finish the sha1 and format it as text. */
   unsigned char sha1[20];
            char cache_id[20 * 2 + 1];
                     if (!screen->disk_cache)
            if (!util_queue_init(&screen->cache_put_thread, "zcq", 8, 1, UTIL_QUEUE_INIT_RESIZE_IF_FULL, screen)) {
               disk_cache_destroy(screen->disk_cache);
                  #endif
            }
         static void
   cache_put_job(void *data, void *gdata, int thread_index)
   {
      struct zink_program *pg = data;
   struct zink_screen *screen = gdata;
   size_t size = 0;
   u_rwlock_rdlock(&pg->pipeline_cache_lock);
   VkResult result = VKSCR(GetPipelineCacheData)(screen->dev, pg->pipeline_cache, &size, NULL);
   if (result != VK_SUCCESS) {
      u_rwlock_rdunlock(&pg->pipeline_cache_lock);
   mesa_loge("ZINK: vkGetPipelineCacheData failed (%s)", vk_Result_to_str(result));
      }
   if (pg->pipeline_cache_size == size) {
      u_rwlock_rdunlock(&pg->pipeline_cache_lock);
      }
   void *pipeline_data = malloc(size);
   if (!pipeline_data) {
      u_rwlock_rdunlock(&pg->pipeline_cache_lock);
      }
   result = VKSCR(GetPipelineCacheData)(screen->dev, pg->pipeline_cache, &size, pipeline_data);
   u_rwlock_rdunlock(&pg->pipeline_cache_lock);
   if (result == VK_SUCCESS) {
               cache_key key;
   disk_cache_compute_key(screen->disk_cache, pg->sha1, sizeof(pg->sha1), key);
      } else {
            }
      void
   zink_screen_update_pipeline_cache(struct zink_screen *screen, struct zink_program *pg, bool in_thread)
   {
      if (!screen->disk_cache || !pg->pipeline_cache)
            if (in_thread)
         else if (util_queue_fence_is_signalled(&pg->cache_fence))
      }
      static void
   cache_get_job(void *data, void *gdata, int thread_index)
   {
      struct zink_program *pg = data;
            VkPipelineCacheCreateInfo pcci;
   pcci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
   pcci.pNext = NULL;
   pcci.flags = screen->info.have_EXT_pipeline_creation_cache_control ? VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT_EXT : 0;
   pcci.initialDataSize = 0;
            cache_key key;
   disk_cache_compute_key(screen->disk_cache, pg->sha1, sizeof(pg->sha1), key);
   pcci.pInitialData = disk_cache_get(screen->disk_cache, key, &pg->pipeline_cache_size);
            VkResult res = VKSCR(CreatePipelineCache)(screen->dev, &pcci, NULL, &pg->pipeline_cache);
   if (res != VK_SUCCESS) {
         }
      }
      void
   zink_screen_get_pipeline_cache(struct zink_screen *screen, struct zink_program *pg, bool in_thread)
   {
      if (!screen->disk_cache)
            if (in_thread)
         else
      }
      static int
   zink_get_compute_param(struct pipe_screen *pscreen, enum pipe_shader_ir ir_type,
         {
         #define RET(x) do {                  \
      if (ret)                          \
            } while (0)
         switch (param) {
   case PIPE_COMPUTE_CAP_ADDRESS_BITS:
            case PIPE_COMPUTE_CAP_IR_TARGET:
      if (ret)
               case PIPE_COMPUTE_CAP_GRID_DIMENSION:
            case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
      RET(((uint64_t []) { screen->info.props.limits.maxComputeWorkGroupCount[0],
               case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
      /* MaxComputeWorkGroupSize[0..2] */
   RET(((uint64_t []) {screen->info.props.limits.maxComputeWorkGroupSize[0],
               case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
   case PIPE_COMPUTE_CAP_MAX_VARIABLE_THREADS_PER_BLOCK:
            case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE:
            case PIPE_COMPUTE_CAP_IMAGES_SUPPORTED:
            case PIPE_COMPUTE_CAP_SUBGROUP_SIZES:
            case PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE:
            case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE:
            case PIPE_COMPUTE_CAP_MAX_COMPUTE_UNITS:
      // no way in vulkan to retrieve this information.
         case PIPE_COMPUTE_CAP_MAX_SUBGROUPS:
   case PIPE_COMPUTE_CAP_MAX_CLOCK_FREQUENCY:
   case PIPE_COMPUTE_CAP_MAX_PRIVATE_SIZE:
   case PIPE_COMPUTE_CAP_MAX_INPUT_SIZE:
      // XXX: I think these are for Clover...
         default:
            }
      static uint32_t
   get_smallest_buffer_heap(struct zink_screen *screen)
   {
      enum zink_heap heaps[] = {
      ZINK_HEAP_DEVICE_LOCAL,
   ZINK_HEAP_DEVICE_LOCAL_VISIBLE,
   ZINK_HEAP_HOST_VISIBLE_COHERENT,
      };
   unsigned size = UINT32_MAX;
   for (unsigned i = 0; i < ARRAY_SIZE(heaps); i++) {
      for (unsigned j = 0; j < screen->heap_count[i]; j++) {
      unsigned heap_idx = screen->info.mem_props.memoryTypes[screen->heap_map[i][j]].heapIndex;
         }
      }
      static inline bool
   have_fp32_filter_linear(struct zink_screen *screen)
   {
      const VkFormat fp32_formats[] = {
      VK_FORMAT_R32_SFLOAT,
   VK_FORMAT_R32G32_SFLOAT,
   VK_FORMAT_R32G32B32_SFLOAT,
   VK_FORMAT_R32G32B32A32_SFLOAT,
      };
   for (int i = 0; i < ARRAY_SIZE(fp32_formats); ++i) {
      VkFormatProperties props;
   VKSCR(GetPhysicalDeviceFormatProperties)(screen->pdev,
               if (((props.linearTilingFeatures | props.optimalTilingFeatures) &
      (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
         VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
         }
      }
      static int
   zink_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
   {
               switch (param) {
   case PIPE_CAP_NULL_TEXTURES:
         case PIPE_CAP_TEXRECT:
   case PIPE_CAP_MULTI_DRAW_INDIRECT_PARTIAL_STRIDE:
         case PIPE_CAP_ANISOTROPIC_FILTER:
         case PIPE_CAP_EMULATE_NONFIXED_PRIMITIVE_RESTART:
         case PIPE_CAP_SUPPORTED_PRIM_MODES_WITH_RESTART: {
      uint32_t modes = BITFIELD_BIT(MESA_PRIM_LINE_STRIP) |
                     if (screen->have_triangle_fans)
         if (screen->info.have_EXT_primitive_topology_list_restart) {
      modes |= BITFIELD_BIT(MESA_PRIM_POINTS) |
            BITFIELD_BIT(MESA_PRIM_LINES) |
   BITFIELD_BIT(MESA_PRIM_LINES_ADJACENCY) |
      if (screen->info.list_restart_feats.primitiveTopologyPatchListRestart)
      }
      }
   case PIPE_CAP_SUPPORTED_PRIM_MODES: {
      uint32_t modes = BITFIELD_MASK(MESA_PRIM_COUNT);
   modes &= ~BITFIELD_BIT(MESA_PRIM_QUAD_STRIP);
   modes &= ~BITFIELD_BIT(MESA_PRIM_POLYGON);
   modes &= ~BITFIELD_BIT(MESA_PRIM_LINE_LOOP);
   if (!screen->have_triangle_fans)
                     case PIPE_CAP_FBFETCH:
         case PIPE_CAP_FBFETCH_COHERENT:
            case PIPE_CAP_MEMOBJ:
         case PIPE_CAP_FENCE_SIGNAL:
         case PIPE_CAP_NATIVE_FENCE_FD:
         case PIPE_CAP_RESOURCE_FROM_USER_MEMORY:
            case PIPE_CAP_SURFACE_REINTERPRET_BLOCKS:
            case PIPE_CAP_VALIDATE_ALL_DIRTY_STATES:
   case PIPE_CAP_ALLOW_MAPPED_BUFFERS_DURING_EXECUTION:
   case PIPE_CAP_MAP_UNSYNCHRONIZED_THREAD_SAFE:
   case PIPE_CAP_SHAREABLE_SHADERS:
   case PIPE_CAP_DEVICE_RESET_STATUS_QUERY:
   case PIPE_CAP_QUERY_MEMORY_INFO:
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_TGSI_TEXCOORD:
   case PIPE_CAP_DRAW_INDIRECT:
   case PIPE_CAP_TEXTURE_QUERY_LOD:
   case PIPE_CAP_GLSL_TESS_LEVELS_AS_INPUTS:
   case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
   case PIPE_CAP_FORCE_PERSAMPLE_INTERP:
   case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
   case PIPE_CAP_SHADER_ARRAY_COMPONENTS:
   case PIPE_CAP_QUERY_BUFFER_OBJECT:
   case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
   case PIPE_CAP_CLIP_HALFZ:
   case PIPE_CAP_TEXTURE_QUERY_SAMPLES:
   case PIPE_CAP_TEXTURE_BARRIER:
   case PIPE_CAP_QUERY_SO_OVERFLOW:
   case PIPE_CAP_GL_SPIRV:
   case PIPE_CAP_CLEAR_SCISSORED:
   case PIPE_CAP_INVALIDATE_BUFFER:
   case PIPE_CAP_PREFER_REAL_BUFFER_IN_CONSTBUF0:
   case PIPE_CAP_PACKED_UNIFORMS:
   case PIPE_CAP_SHADER_PACK_HALF_FLOAT:
   case PIPE_CAP_CULL_DISTANCE_NOCOMBINE:
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
   case PIPE_CAP_LOAD_CONSTBUF:
   case PIPE_CAP_MULTISAMPLE_Z_RESOLVE:
   case PIPE_CAP_ALLOW_GLTHREAD_BUFFER_SUBDATA_OPT:
            case PIPE_CAP_DRAW_VERTEX_STATE:
            case PIPE_CAP_SURFACE_SAMPLE_COUNT:
            case PIPE_CAP_SHADER_GROUP_VOTE:
      if (screen->info.have_vulkan11 &&
      (screen->info.subgroup.supportedOperations & VK_SUBGROUP_FEATURE_VOTE_BIT) &&
   (screen->info.subgroup.supportedStages & VK_SHADER_STAGE_COMPUTE_BIT))
      if (screen->info.have_EXT_shader_subgroup_vote)
            case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
            case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
            case PIPE_CAP_POLYGON_OFFSET_UNITS_UNSCALED:
            case PIPE_CAP_POLYGON_OFFSET_CLAMP:
            case PIPE_CAP_QUERY_PIPELINE_STATISTICS_SINGLE:
            case PIPE_CAP_ROBUST_BUFFER_ACCESS_BEHAVIOR:
      return screen->info.feats.features.robustBufferAccess &&
         case PIPE_CAP_MULTI_DRAW_INDIRECT:
            case PIPE_CAP_IMAGE_ATOMIC_FLOAT_ADD:
      return (screen->info.have_EXT_shader_atomic_float &&
            case PIPE_CAP_SHADER_ATOMIC_INT64:
      return (screen->info.have_KHR_shader_atomic_int64 &&
               case PIPE_CAP_MULTI_DRAW_INDIRECT_PARAMS:
            case PIPE_CAP_START_INSTANCE:
   case PIPE_CAP_DRAW_PARAMETERS:
      return (screen->info.have_vulkan12 && screen->info.feats11.shaderDrawParameters) ||
         case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
            case PIPE_CAP_MAX_VERTEX_STREAMS:
            case PIPE_CAP_COMPUTE_SHADER_DERIVATIVES:
            case PIPE_CAP_INT64:
   case PIPE_CAP_DOUBLES:
            case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
      if (!screen->info.feats.features.dualSrcBlend)
               case PIPE_CAP_MAX_RENDER_TARGETS:
            case PIPE_CAP_OCCLUSION_QUERY:
            case PIPE_CAP_PROGRAMMABLE_SAMPLE_LOCATIONS:
            case PIPE_CAP_QUERY_TIME_ELAPSED:
            case PIPE_CAP_TEXTURE_MULTISAMPLE:
            case PIPE_CAP_FRAGMENT_SHADER_INTERLOCK:
            case PIPE_CAP_SHADER_CLOCK:
            case PIPE_CAP_SHADER_BALLOT:
      if (screen->info.props11.subgroupSize > 64)
         if (screen->info.have_vulkan11 &&
      screen->info.subgroup.supportedOperations & VK_SUBGROUP_FEATURE_BALLOT_BIT)
      if (screen->info.have_EXT_shader_subgroup_ballot)
               case PIPE_CAP_DEMOTE_TO_HELPER_INVOCATION:
      return screen->spirv_version >= SPIRV_VERSION(1, 6) ||
         case PIPE_CAP_SAMPLE_SHADING:
            case PIPE_CAP_TEXTURE_SWIZZLE:
            case PIPE_CAP_VERTEX_ATTRIB_ELEMENT_ALIGNED_ONLY:
            case PIPE_CAP_GL_CLAMP:
            case PIPE_CAP_PREFER_IMM_ARRAYS_AS_CONSTBUF:
            case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK: {
      enum pipe_quirk_texture_border_color_swizzle quirk = PIPE_QUIRK_TEXTURE_BORDER_COLOR_SWIZZLE_ALPHA_NOT_W;
   if (!screen->info.border_color_feats.customBorderColorWithoutFormat)
         /* assume that if drivers don't implement this extension they either:
   * - don't support custom border colors
   * - handle things correctly
   * - hate border color accuracy
   */
   if (screen->info.have_EXT_border_color_swizzle &&
      !screen->info.border_swizzle_feats.borderColorSwizzleFromImage)
                  case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
      return MIN2(screen->info.props.limits.maxImageDimension1D,
      case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
         case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
            case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_INDEP_BLEND_FUNC:
            case PIPE_CAP_DITHERING:
            case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
         case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_STREAM_OUTPUT_INTERLEAVE_BUFFERS:
            case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
            case PIPE_CAP_DEPTH_CLIP_DISABLE:
            case PIPE_CAP_SHADER_STENCIL_EXPORT:
            case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
            case PIPE_CAP_MIN_TEXEL_OFFSET:
         case PIPE_CAP_MAX_TEXEL_OFFSET:
            case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
            case PIPE_CAP_CONDITIONAL_RENDER:
            case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
   case PIPE_CAP_GLSL_FEATURE_LEVEL:
            case PIPE_CAP_COMPUTE:
            case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_QUERY_TIMESTAMP:
            case PIPE_CAP_QUERY_TIMESTAMP_BITS:
            case PIPE_CAP_TIMER_RESOLUTION:
            case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
            case PIPE_CAP_CUBE_MAP_ARRAY:
            case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
   case PIPE_CAP_PRIMITIVE_RESTART:
            case PIPE_CAP_BINDLESS_TEXTURE:
            case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_TEXTURE_TRANSFER_MODES: {
      enum pipe_texture_transfer_mode mode = PIPE_TEXTURE_TRANSFER_BLIT;
   if (!screen->is_cpu &&
      /* this needs substantial perf tuning */
   screen->info.driver_props.driverID != VK_DRIVER_ID_MESA_TURNIP &&
   screen->info.have_KHR_8bit_storage &&
   screen->info.have_KHR_16bit_storage &&
   screen->info.have_KHR_shader_float16_int8)
                  case PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT:
      return MIN2(get_smallest_buffer_heap(screen),
         case PIPE_CAP_ENDIANNESS:
            case PIPE_CAP_MAX_VIEWPORTS:
            case PIPE_CAP_IMAGE_LOAD_FORMATTED:
            case PIPE_CAP_IMAGE_STORE_FORMATTED:
            case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
            case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
         case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
            case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
            case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
            case PIPE_CAP_SAMPLER_REDUCTION_MINMAX_ARB:
            case PIPE_CAP_OPENCL_INTEGER_FUNCTIONS:
   case PIPE_CAP_INTEGER_MULTIPLY_32X16:
            case PIPE_CAP_FS_FINE_DERIVATIVE:
            case PIPE_CAP_VENDOR_ID:
         case PIPE_CAP_DEVICE_ID:
            case PIPE_CAP_ACCELERATED:
         case PIPE_CAP_VIDEO_MEMORY:
         case PIPE_CAP_UMA:
            case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
            case PIPE_CAP_SAMPLER_VIEW_TARGET:
            case PIPE_CAP_VS_LAYER_VIEWPORT:
   case PIPE_CAP_TES_LAYER_VIEWPORT:
      return screen->info.have_EXT_shader_viewport_index_layer ||
         (screen->spirv_version >= SPIRV_VERSION(1, 5) &&
         case PIPE_CAP_TEXTURE_FLOAT_LINEAR:
            case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
            case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_PCI_GROUP:
   case PIPE_CAP_PCI_BUS:
   case PIPE_CAP_PCI_DEVICE:
   case PIPE_CAP_PCI_FUNCTION:
            case PIPE_CAP_CULL_DISTANCE:
            case PIPE_CAP_SPARSE_BUFFER_PAGE_SIZE:
            /* Sparse texture */
   case PIPE_CAP_MAX_SPARSE_TEXTURE_SIZE:
      return screen->info.feats.features.sparseResidencyImage2D ?
      case PIPE_CAP_MAX_SPARSE_3D_TEXTURE_SIZE:
      return screen->info.feats.features.sparseResidencyImage3D ?
      case PIPE_CAP_MAX_SPARSE_ARRAY_TEXTURE_LAYERS:
      return screen->info.feats.features.sparseResidencyImage2D ?
      case PIPE_CAP_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS:
         case PIPE_CAP_QUERY_SPARSE_TEXTURE_RESIDENCY:
      return screen->info.feats.features.sparseResidency2Samples &&
      case PIPE_CAP_CLAMP_SPARSE_TEXTURE_LOD:
      return screen->info.feats.features.shaderResourceMinLod &&
               case PIPE_CAP_VIEWPORT_SUBPIXEL_BITS:
            case PIPE_CAP_MAX_GS_INVOCATIONS:
            case PIPE_CAP_MAX_COMBINED_SHADER_BUFFERS:
      /* gallium handles this automatically */
         case PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT:
      /* 1<<27 is required by VK spec */
   assert(screen->info.props.limits.maxStorageBufferRange >= 1 << 27);
   /* clamp to VK spec minimum */
         case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
            case PIPE_CAP_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_INTEGER:
            case PIPE_CAP_NIR_COMPACT_ARRAYS:
            case PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL:
   case PIPE_CAP_FS_POINT_IS_SYSVAL:
            case PIPE_CAP_VIEWPORT_TRANSFORM_LOWERED:
            case PIPE_CAP_FLATSHADE:
   case PIPE_CAP_ALPHA_TEST:
   case PIPE_CAP_CLIP_PLANES:
   case PIPE_CAP_POINT_SIZE_FIXED:
   case PIPE_CAP_TWO_SIDED_COLOR:
            case PIPE_CAP_MAX_SHADER_PATCH_VARYINGS:
         case PIPE_CAP_MAX_VARYINGS:
      /* need to reserve up to 60 of our varying components and 16 slots for streamout */
            #if defined(HAVE_LIBDRM) && (DETECT_OS_LINUX || DETECT_OS_BSD)
         return screen->info.have_KHR_external_memory_fd &&
         screen->info.have_EXT_external_memory_dma_buf &&
   screen->info.have_EXT_queue_family_foreign
   #else
         #endif
         case PIPE_CAP_DEPTH_BOUNDS_TEST:
            case PIPE_CAP_POST_DEPTH_COVERAGE:
            case PIPE_CAP_STRING_MARKER:
            default:
            }
      static float
   zink_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
   {
               switch (param) {
   case PIPE_CAPF_MIN_LINE_WIDTH:
   case PIPE_CAPF_MIN_LINE_WIDTH_AA:
      if (!screen->info.feats.features.wideLines)
               case PIPE_CAPF_MIN_POINT_SIZE:
   case PIPE_CAPF_MIN_POINT_SIZE_AA:
      if (!screen->info.feats.features.largePoints)
                  case PIPE_CAPF_LINE_WIDTH_GRANULARITY:
      if (!screen->info.feats.features.wideLines)
               case PIPE_CAPF_POINT_SIZE_GRANULARITY:
      if (!screen->info.feats.features.largePoints)
                  case PIPE_CAPF_MAX_LINE_WIDTH:
   case PIPE_CAPF_MAX_LINE_WIDTH_AA:
      if (!screen->info.feats.features.wideLines)
               case PIPE_CAPF_MAX_POINT_SIZE:
   case PIPE_CAPF_MAX_POINT_SIZE_AA:
      if (!screen->info.feats.features.largePoints)
               case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
      if (!screen->info.feats.features.samplerAnisotropy)
               case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
            case PIPE_CAPF_MIN_CONSERVATIVE_RASTER_DILATE:
   case PIPE_CAPF_MAX_CONSERVATIVE_RASTER_DILATE:
   case PIPE_CAPF_CONSERVATIVE_RASTER_DILATE_GRANULARITY:
                  /* should only get here on unhandled cases */
      }
      static int
   zink_get_shader_param(struct pipe_screen *pscreen,
               {
               switch (param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
      switch (shader) {
   case MESA_SHADER_FRAGMENT:
   case MESA_SHADER_VERTEX:
         case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
      if (screen->info.feats.features.tessellationShader &&
      screen->info.have_KHR_maintenance2)
            case MESA_SHADER_GEOMETRY:
      if (screen->info.feats.features.geometryShader)
               case MESA_SHADER_COMPUTE:
         default:
         }
      case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
   case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
            case PIPE_SHADER_CAP_MAX_INPUTS: {
      uint32_t max = 0;
   switch (shader) {
   case MESA_SHADER_VERTEX:
      max = MIN2(screen->info.props.limits.maxVertexInputAttributes, PIPE_MAX_ATTRIBS);
      case MESA_SHADER_TESS_CTRL:
      max = screen->info.props.limits.maxTessellationControlPerVertexInputComponents / 4;
      case MESA_SHADER_TESS_EVAL:
      max = screen->info.props.limits.maxTessellationEvaluationInputComponents / 4;
      case MESA_SHADER_GEOMETRY:
      max = screen->info.props.limits.maxGeometryInputComponents / 4;
      case MESA_SHADER_FRAGMENT:
      /* intel drivers report fewer components, but it's a value that's compatible
   * with what we need for GL, so we can still force a conformant value here
   */
   if (screen->info.driver_props.driverID == VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA ||
      screen->info.driver_props.driverID == VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS)
      max = screen->info.props.limits.maxFragmentInputComponents / 4;
      default:
         }
   switch (shader) {
   case MESA_SHADER_VERTEX:
   case MESA_SHADER_TESS_EVAL:
   case MESA_SHADER_GEOMETRY:
      /* last vertex stage must support streamout, and this is capped in glsl compiler */
      default: break;
   }
               case PIPE_SHADER_CAP_MAX_OUTPUTS: {
      uint32_t max = 0;
   switch (shader) {
   case MESA_SHADER_VERTEX:
      max = screen->info.props.limits.maxVertexOutputComponents / 4;
      case MESA_SHADER_TESS_CTRL:
      max = screen->info.props.limits.maxTessellationControlPerVertexOutputComponents / 4;
      case MESA_SHADER_TESS_EVAL:
      max = screen->info.props.limits.maxTessellationEvaluationOutputComponents / 4;
      case MESA_SHADER_GEOMETRY:
      max = screen->info.props.limits.maxGeometryOutputComponents / 4;
      case MESA_SHADER_FRAGMENT:
      max = screen->info.props.limits.maxColorAttachments;
      default:
         }
               case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
      /* At least 16384 is guaranteed by VK spec */
   assert(screen->info.props.limits.maxUniformBufferRange >= 16384);
   /* but Gallium can't handle values that are too big */
   return MIN3(get_smallest_buffer_heap(screen),
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
      return  MIN2(screen->info.props.limits.maxPerStageDescriptorUniformBuffers,
         case PIPE_SHADER_CAP_MAX_TEMPS:
            case PIPE_SHADER_CAP_INTEGERS:
            case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
            case PIPE_SHADER_CAP_SUBROUTINES:
   case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
            case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
      //enabling this breaks GTF-GL46.gtf21.GL2Tests.glGetUniform.glGetUniform
   //return screen->info.feats11.uniformAndStorageBuffer16BitAccess ||
            case PIPE_SHADER_CAP_FP16_DERIVATIVES:
         case PIPE_SHADER_CAP_FP16:
      return screen->info.feats12.shaderFloat16 ||
               case PIPE_SHADER_CAP_INT16:
            case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
            case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
      return MIN2(MIN2(screen->info.props.limits.maxPerStageDescriptorSamplers,
               case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
            case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
      switch (shader) {
   case MESA_SHADER_VERTEX:
   case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
   case MESA_SHADER_GEOMETRY:
      if (!screen->info.feats.features.vertexPipelineStoresAndAtomics)
               case MESA_SHADER_FRAGMENT:
      if (!screen->info.feats.features.fragmentStoresAndAtomics)
               default:
                  /* TODO: this limitation is dumb, and will need some fixes in mesa */
         case PIPE_SHADER_CAP_SUPPORTED_IRS:
            case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
      if (screen->info.feats.features.shaderStorageImageExtendedFormats &&
      screen->info.feats.features.shaderStorageImageWriteWithoutFormat)
   return MIN2(screen->info.props.limits.maxPerStageDescriptorStorageImages,
            case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         case PIPE_SHADER_CAP_CONT_SUPPORTED:
                  /* should only get here on unhandled cases */
      }
      static VkSampleCountFlagBits
   vk_sample_count_flags(uint32_t sample_count)
   {
      switch (sample_count) {
   case 1: return VK_SAMPLE_COUNT_1_BIT;
   case 2: return VK_SAMPLE_COUNT_2_BIT;
   case 4: return VK_SAMPLE_COUNT_4_BIT;
   case 8: return VK_SAMPLE_COUNT_8_BIT;
   case 16: return VK_SAMPLE_COUNT_16_BIT;
   case 32: return VK_SAMPLE_COUNT_32_BIT;
   case 64: return VK_SAMPLE_COUNT_64_BIT;
   default:
            }
      static bool
   zink_is_compute_copy_faster(struct pipe_screen *pscreen,
                              enum pipe_format src_format,
      {
      if (cpu)
      /* very basic for now, probably even worse for some cases,
   * but fixes lots of others
   */
         }
      static bool
   zink_is_format_supported(struct pipe_screen *pscreen,
                           enum pipe_format format,
   {
               if (storage_sample_count && !screen->info.feats.features.shaderStorageImageMultisample && bind & PIPE_BIND_SHADER_IMAGE)
            if (format == PIPE_FORMAT_NONE)
      return screen->info.props.limits.framebufferNoAttachmentsSampleCounts &
         if (bind & PIPE_BIND_INDEX_BUFFER) {
      if (format == PIPE_FORMAT_R8_UINT &&
      !screen->info.have_EXT_index_type_uint8)
      if (format != PIPE_FORMAT_R8_UINT &&
      format != PIPE_FORMAT_R16_UINT &&
   format != PIPE_FORMAT_R32_UINT)
            /* always use superset to determine feature support */
   VkFormat vkformat = zink_get_format(screen, PIPE_FORMAT_A8_UNORM ? zink_format_get_emulated_alpha(format) : format);
   if (vkformat == VK_FORMAT_UNDEFINED)
            if (sample_count >= 1) {
      VkSampleCountFlagBits sample_mask = vk_sample_count_flags(sample_count);
   if (!sample_mask)
         const struct util_format_description *desc = util_format_description(format);
   if (util_format_is_depth_or_stencil(format)) {
      if (util_format_has_depth(desc)) {
      if (bind & PIPE_BIND_DEPTH_STENCIL &&
      (screen->info.props.limits.framebufferDepthSampleCounts & sample_mask) != sample_mask)
      if (bind & PIPE_BIND_SAMPLER_VIEW &&
      (screen->info.props.limits.sampledImageDepthSampleCounts & sample_mask) != sample_mask)
   }
   if (util_format_has_stencil(desc)) {
      if (bind & PIPE_BIND_DEPTH_STENCIL &&
      (screen->info.props.limits.framebufferStencilSampleCounts & sample_mask) != sample_mask)
      if (bind & PIPE_BIND_SAMPLER_VIEW &&
      (screen->info.props.limits.sampledImageStencilSampleCounts & sample_mask) != sample_mask)
      } else if (util_format_is_pure_integer(format)) {
      if (bind & PIPE_BIND_RENDER_TARGET &&
      !(screen->info.props.limits.framebufferColorSampleCounts & sample_mask))
      if (bind & PIPE_BIND_SAMPLER_VIEW &&
      !(screen->info.props.limits.sampledImageIntegerSampleCounts & sample_mask))
   } else {
      if (bind & PIPE_BIND_RENDER_TARGET &&
      !(screen->info.props.limits.framebufferColorSampleCounts & sample_mask))
      if (bind & PIPE_BIND_SAMPLER_VIEW &&
      !(screen->info.props.limits.sampledImageColorSampleCounts & sample_mask))
   }
   if (bind & PIPE_BIND_SHADER_IMAGE) {
      if (!(screen->info.props.limits.storageImageSampleCounts & sample_mask))
                           if (target == PIPE_BUFFER) {
      if (bind & PIPE_BIND_VERTEX_BUFFER) {
      if (!(props.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)) {
      enum pipe_format new_format = zink_decompose_vertex_format(format);
   if (!new_format)
         if (!(screen->format_props[new_format].bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT))
                  if (bind & PIPE_BIND_SAMPLER_VIEW &&
                  if (bind & PIPE_BIND_SHADER_IMAGE &&
      !(props.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))
   } else {
      /* all other targets are texture-targets */
   if (bind & PIPE_BIND_RENDER_TARGET &&
                  if (bind & PIPE_BIND_BLENDABLE &&
                  if (bind & PIPE_BIND_SAMPLER_VIEW &&
                  if (bind & PIPE_BIND_SAMPLER_REDUCTION_MINMAX &&
                  if ((bind & PIPE_BIND_SAMPLER_VIEW) || (bind & PIPE_BIND_RENDER_TARGET)) {
      /* if this is a 3-component texture, force gallium to give us 4 components by rejecting this one */
   const struct util_format_description *desc = util_format_description(format);
   if (desc->nr_channels == 3 &&
      (desc->block.bits == 24 || desc->block.bits == 48 || desc->block.bits == 96))
            if (bind & PIPE_BIND_DEPTH_STENCIL &&
                  if (bind & PIPE_BIND_SHADER_IMAGE &&
      !(props.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
               }
      static void
   zink_destroy_screen(struct pipe_screen *pscreen)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   struct zink_batch_state *bs = screen->free_batch_states;
   while (bs) {
      struct zink_batch_state *bs_next = bs->next;
   zink_batch_state_destroy(screen, bs);
            #ifdef HAVE_RENDERDOC_APP_H
      if (screen->renderdoc_capture_all && p_atomic_dec_zero(&num_screens))
      #endif
         hash_table_foreach(&screen->dts, entry)
            if (screen->copy_context)
            if (VK_NULL_HANDLE != screen->debugUtilsCallbackHandle) {
                           if (screen->gfx_push_constant_layout)
            u_transfer_helper_destroy(pscreen->transfer_helper);
   if (util_queue_is_initialized(&screen->cache_get_thread)) {
      util_queue_finish(&screen->cache_get_thread);
         #ifdef ENABLE_SHADER_CACHE
      if (screen->disk_cache && util_queue_is_initialized(&screen->cache_put_thread)) {
      util_queue_finish(&screen->cache_put_thread);
   disk_cache_wait_for_idle(screen->disk_cache);
         #endif
               /* we don't have an API to check if a set is already initialized */
   for (unsigned i = 0; i < ARRAY_SIZE(screen->pipeline_libs); i++)
      if (screen->pipeline_libs[i].table)
         zink_bo_deinit(screen);
                     if (screen->sem)
            if (screen->fence)
            if (util_queue_is_initialized(&screen->flush_queue))
            while (util_dynarray_contains(&screen->semaphores, VkSemaphore))
         while (util_dynarray_contains(&screen->fd_semaphores, VkSemaphore))
         if (screen->bindless_layout)
            if (screen->dev)
            if (screen->instance)
                     if (screen->loader_lib)
            if (screen->drm_fd != -1)
            slab_destroy_parent(&screen->transfer_pool);
   ralloc_free(screen);
      }
      static int
   zink_get_display_device(const struct zink_screen *screen, uint32_t pdev_count,
               {
      VkPhysicalDeviceDrmPropertiesEXT drm_props = {
         };
   VkPhysicalDeviceProperties2 props = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
               for (uint32_t i = 0; i < pdev_count; ++i) {
      VKSCR(GetPhysicalDeviceProperties2)(pdevs[i], &props);
   if (drm_props.renderMajor == dev_major &&
      drm_props.renderMinor == dev_minor)
               }
      static int
   zink_get_cpu_device_type(const struct zink_screen *screen, uint32_t pdev_count,
         {
               for (uint32_t i = 0; i < pdev_count; ++i) {
               /* if user wants cpu, only give them cpu */
   if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
                           }
      static void
   choose_pdev(struct zink_screen *screen, int64_t dev_major, int64_t dev_minor)
   {
      bool cpu = debug_get_bool_option("LIBGL_ALWAYS_SOFTWARE", false) ||
            if (cpu || (dev_major > 0 && dev_major < 255)) {
      uint32_t pdev_count;
   int idx;
   VkPhysicalDevice *pdevs;
   VkResult result = VKSCR(EnumeratePhysicalDevices)(screen->instance, &pdev_count, NULL);
   if (result != VK_SUCCESS) {
      mesa_loge("ZINK: vkEnumeratePhysicalDevices failed (%s)", vk_Result_to_str(result));
                        pdevs = malloc(sizeof(*pdevs) * pdev_count);
   if (!pdevs) {
      mesa_loge("ZINK: failed to allocate pdevs!");
      }
   result = VKSCR(EnumeratePhysicalDevices)(screen->instance, &pdev_count, pdevs);
   assert(result == VK_SUCCESS);
            if (cpu)
         else
                  if (idx != -1)
                           if (idx == -1)
         } else {
      VkPhysicalDevice pdev;
   unsigned pdev_count = 1;
   VkResult result = VKSCR(EnumeratePhysicalDevices)(screen->instance, &pdev_count, &pdev);
   if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
      mesa_loge("ZINK: vkEnumeratePhysicalDevices failed (%s)", vk_Result_to_str(result));
      }
      }
            /* allow software rendering only if forced by the user */
   if (!cpu && screen->info.props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
      screen->pdev = VK_NULL_HANDLE;
                        /* runtime version is the lesser of the instance version and device version */
            /* calculate SPIR-V version based on VK version */
   if (screen->vk_version >= VK_MAKE_VERSION(1, 3, 0))
         else if (screen->vk_version >= VK_MAKE_VERSION(1, 2, 0))
         else if (screen->vk_version >= VK_MAKE_VERSION(1, 1, 0))
         else
      }
      static void
   update_queue_props(struct zink_screen *screen)
   {
      uint32_t num_queues;
   VKSCR(GetPhysicalDeviceQueueFamilyProperties)(screen->pdev, &num_queues, NULL);
            VkQueueFamilyProperties *props = malloc(sizeof(*props) * num_queues);
   if (!props) {
      mesa_loge("ZINK: failed to allocate props!");
      }
                  bool found_gfx = false;
   uint32_t sparse_only = UINT32_MAX;
   screen->sparse_queue = UINT32_MAX;
   for (uint32_t i = 0; i < num_queues; i++) {
      if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      if (found_gfx)
         screen->sparse_queue = screen->gfx_queue = i;
   screen->max_queues = props[i].queueCount;
   screen->timestamp_valid_bits = props[i].timestampValidBits;
      } else if (props[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
      }
   if (sparse_only != UINT32_MAX)
            }
      static void
   init_queue(struct zink_screen *screen)
   {
      simple_mtx_init(&screen->queue_lock, mtx_plain);
   VKSCR(GetDeviceQueue)(screen->dev, screen->gfx_queue, 0, &screen->queue);
   if (screen->sparse_queue != screen->gfx_queue)
         else
      }
      static void
   zink_flush_frontbuffer(struct pipe_screen *pscreen,
                        struct pipe_context *pctx,
      {
      struct zink_screen *screen = zink_screen(pscreen);
   struct zink_resource *res = zink_resource(pres);
            /* if the surface is no longer a swapchain, this is a no-op */
   if (!zink_is_swapchain(res))
                     if (!zink_kopper_acquired(res->obj->dt, res->obj->dt_idx)) {
      /* swapbuffers to an undefined surface: acquire and present garbage */
   zink_kopper_acquire(ctx, res, UINT64_MAX);
   ctx->needs_present = res;
   /* set batch usage to submit acquire semaphore */
   zink_batch_resource_usage_set(&ctx->batch, res, true, false);
   /* ensure the resource is set up to present garbage */
               /* handle any outstanding acquire submits (not just from above) */
   if (ctx->batch.swapchain || ctx->needs_present) {
      ctx->batch.has_work = true;
   pctx->flush(pctx, NULL, PIPE_FLUSH_END_OF_FRAME);
   if (ctx->last_fence && screen->threaded_submit) {
      struct zink_batch_state *bs = zink_batch_state(ctx->last_fence);
                  /* always verify that this was acquired */
   assert(zink_kopper_acquired(res->obj->dt, res->obj->dt_idx));
      }
      bool
   zink_is_depth_format_supported(struct zink_screen *screen, VkFormat format)
   {
      VkFormatProperties props;
   VKSCR(GetPhysicalDeviceFormatProperties)(screen->pdev, format, &props);
   return (props.linearTilingFeatures | props.optimalTilingFeatures) &
      }
      static enum pipe_format
   emulate_x8(enum pipe_format format)
   {
      /* convert missing Xn variants to An */
   switch (format) {
   case PIPE_FORMAT_B8G8R8X8_UNORM:
            case PIPE_FORMAT_B8G8R8X8_SRGB:
         case PIPE_FORMAT_R8G8B8X8_SRGB:
            case PIPE_FORMAT_R8G8B8X8_SINT:
         case PIPE_FORMAT_R8G8B8X8_SNORM:
         case PIPE_FORMAT_R8G8B8X8_UNORM:
            case PIPE_FORMAT_R16G16B16X16_FLOAT:
         case PIPE_FORMAT_R16G16B16X16_SINT:
         case PIPE_FORMAT_R16G16B16X16_SNORM:
         case PIPE_FORMAT_R16G16B16X16_UNORM:
            case PIPE_FORMAT_R32G32B32X32_FLOAT:
         case PIPE_FORMAT_R32G32B32X32_SINT:
            default:
            }
      VkFormat
   zink_get_format(struct zink_screen *screen, enum pipe_format format)
   {
      if (format == PIPE_FORMAT_A8_UNORM && !screen->driver_workarounds.missing_a8_unorm)
         else if (!screen->driver_workarounds.broken_l4a4 || format != PIPE_FORMAT_L4A4_UNORM)
                     if (format == PIPE_FORMAT_X32_S8X24_UINT &&
      screen->have_D32_SFLOAT_S8_UINT)
         if (format == PIPE_FORMAT_X24S8_UINT)
      /* valid when using aspects to extract stencil,
   * fails format test because it's emulated */
         if (ret == VK_FORMAT_X8_D24_UNORM_PACK32 &&
      !screen->have_X8_D24_UNORM_PACK32) {
   assert(zink_is_depth_format_supported(screen, VK_FORMAT_D32_SFLOAT));
               if (ret == VK_FORMAT_D24_UNORM_S8_UINT &&
      !screen->have_D24_UNORM_S8_UINT) {
   assert(screen->have_D32_SFLOAT_S8_UINT);
               if ((ret == VK_FORMAT_A4B4G4R4_UNORM_PACK16 &&
      !screen->info.format_4444_feats.formatA4B4G4R4) ||
   (ret == VK_FORMAT_A4R4G4B4_UNORM_PACK16 &&
   !screen->info.format_4444_feats.formatA4R4G4B4))
         if (format == PIPE_FORMAT_R4A4_UNORM)
               }
      void
   zink_convert_color(const struct zink_screen *screen, enum pipe_format format,
               {
      const struct util_format_description *desc = util_format_description(format);
            for (unsigned i = 0; i < 4; i++)
            if (zink_format_is_emulated_alpha(format) &&
      /* Don't swizzle colors if the driver supports real A8_UNORM */
   (format != PIPE_FORMAT_A8_UNORM ||
         if (util_format_is_alpha(format)) {
      tmp.ui[0] = tmp.ui[3];
   tmp.ui[1] = 0;
   tmp.ui[2] = 0;
      } else if (util_format_is_luminance(format)) {
      tmp.ui[1] = 0;
   tmp.ui[2] = 0;
      } else if (util_format_is_luminance_alpha(format)) {
      tmp.ui[1] = tmp.ui[3];
   tmp.ui[2] = 0;
      } else /* zink_format_is_red_alpha */ {
      tmp.ui[1] = tmp.ui[3];
   tmp.ui[2] = 0;
                     }
      static bool
   check_have_device_time(struct zink_screen *screen)
   {
      uint32_t num_domains = 0;
   VkTimeDomainEXT domains[8]; //current max is 4
   VkResult result = VKSCR(GetPhysicalDeviceCalibrateableTimeDomainsEXT)(screen->pdev, &num_domains, NULL);
   if (result != VK_SUCCESS) {
         }
   assert(num_domains > 0);
            result = VKSCR(GetPhysicalDeviceCalibrateableTimeDomainsEXT)(screen->pdev, &num_domains, domains);
   if (result != VK_SUCCESS) {
                  /* VK_TIME_DOMAIN_DEVICE_EXT is used for the ctx->get_timestamp hook and is the only one we really need */
   for (unsigned i = 0; i < num_domains; i++) {
      if (domains[i] == VK_TIME_DOMAIN_DEVICE_EXT) {
                        }
      static void
   zink_error(const char *msg)
   {
   }
      static void
   zink_warn(const char *msg)
   {
   }
      static void
   zink_info(const char *msg)
   {
   }
      static void
   zink_msg(const char *msg)
   {
   }
      static VKAPI_ATTR VkBool32 VKAPI_CALL
   zink_debug_util_callback(
      VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
   VkDebugUtilsMessageTypeFlagsEXT                  messageType,
   const VkDebugUtilsMessengerCallbackDataEXT      *pCallbackData,
      {
      // Pick message prefix and color to use.
   // Only MacOS and Linux have been tested for color support
   if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
         } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
         } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
         } else
               }
      static bool
   create_debug(struct zink_screen *screen)
   {
      VkDebugUtilsMessengerCreateInfoEXT vkDebugUtilsMessengerCreateInfoEXT = {
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
   NULL,
   0,  // flags
   VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
   VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
   zink_debug_util_callback,
                        VkResult result = VKSCR(CreateDebugUtilsMessengerEXT)(
         screen->instance,
   &vkDebugUtilsMessengerCreateInfoEXT,
   NULL,
   if (result != VK_SUCCESS) {
                              }
      static bool
   zink_internal_setup_moltenvk(struct zink_screen *screen)
   {
   #if defined(MVK_VERSION)
      if (!screen->instance_info.have_MVK_moltenvk)
            GET_PROC_ADDR_INSTANCE_LOCAL(screen, screen->instance, GetMoltenVKConfigurationMVK);
   GET_PROC_ADDR_INSTANCE_LOCAL(screen, screen->instance, SetMoltenVKConfigurationMVK);
            if (vk_GetVersionStringsMVK) {
      char molten_version[64] = {0};
                                 if (vk_GetMoltenVKConfigurationMVK && vk_SetMoltenVKConfigurationMVK) {
      MVKConfiguration molten_config = {0};
            VkResult res = vk_GetMoltenVKConfigurationMVK(screen->instance, &molten_config, &molten_config_size);
   if (res == VK_SUCCESS || res == VK_INCOMPLETE) {
      // Needed to allow MoltenVK to accept VkImageView swizzles.
   // Encountered when using VK_FORMAT_R8G8_UNORM
   molten_config.fullImageViewSwizzle = VK_TRUE;
            #endif // MVK_VERSION
            }
      static void
   check_vertex_formats(struct zink_screen *screen)
   {
      /* from vbuf */
   enum pipe_format format_list[] = {
      /* not supported by vk
   PIPE_FORMAT_R32_FIXED,
   PIPE_FORMAT_R32G32_FIXED,
   PIPE_FORMAT_R32G32B32_FIXED,
   PIPE_FORMAT_R32G32B32A32_FIXED,
   */
   PIPE_FORMAT_R16_FLOAT,
   PIPE_FORMAT_R16G16_FLOAT,
   PIPE_FORMAT_R16G16B16_FLOAT,
   PIPE_FORMAT_R16G16B16A16_FLOAT,
   /* not supported by vk
   PIPE_FORMAT_R64_FLOAT,
   PIPE_FORMAT_R64G64_FLOAT,
   PIPE_FORMAT_R64G64B64_FLOAT,
   PIPE_FORMAT_R64G64B64A64_FLOAT,
   PIPE_FORMAT_R32_UNORM,
   PIPE_FORMAT_R32G32_UNORM,
   PIPE_FORMAT_R32G32B32_UNORM,
   PIPE_FORMAT_R32G32B32A32_UNORM,
   PIPE_FORMAT_R32_SNORM,
   PIPE_FORMAT_R32G32_SNORM,
   PIPE_FORMAT_R32G32B32_SNORM,
   PIPE_FORMAT_R32G32B32A32_SNORM,
   PIPE_FORMAT_R32_USCALED,
   PIPE_FORMAT_R32G32_USCALED,
   PIPE_FORMAT_R32G32B32_USCALED,
   PIPE_FORMAT_R32G32B32A32_USCALED,
   PIPE_FORMAT_R32_SSCALED,
   PIPE_FORMAT_R32G32_SSCALED,
   PIPE_FORMAT_R32G32B32_SSCALED,
   PIPE_FORMAT_R32G32B32A32_SSCALED,
   */
   PIPE_FORMAT_R16_UNORM,
   PIPE_FORMAT_R16G16_UNORM,
   PIPE_FORMAT_R16G16B16_UNORM,
   PIPE_FORMAT_R16G16B16A16_UNORM,
   PIPE_FORMAT_R16_SNORM,
   PIPE_FORMAT_R16G16_SNORM,
   PIPE_FORMAT_R16G16B16_SNORM,
   PIPE_FORMAT_R16G16B16_SINT,
   PIPE_FORMAT_R16G16B16_UINT,
   PIPE_FORMAT_R16G16B16A16_SNORM,
   PIPE_FORMAT_R16_USCALED,
   PIPE_FORMAT_R16G16_USCALED,
   PIPE_FORMAT_R16G16B16_USCALED,
   PIPE_FORMAT_R16G16B16A16_USCALED,
   PIPE_FORMAT_R16_SSCALED,
   PIPE_FORMAT_R16G16_SSCALED,
   PIPE_FORMAT_R16G16B16_SSCALED,
   PIPE_FORMAT_R16G16B16A16_SSCALED,
   PIPE_FORMAT_R8_UNORM,
   PIPE_FORMAT_R8G8_UNORM,
   PIPE_FORMAT_R8G8B8_UNORM,
   PIPE_FORMAT_R8G8B8A8_UNORM,
   PIPE_FORMAT_R8_SNORM,
   PIPE_FORMAT_R8G8_SNORM,
   PIPE_FORMAT_R8G8B8_SNORM,
   PIPE_FORMAT_R8G8B8A8_SNORM,
   PIPE_FORMAT_R8_USCALED,
   PIPE_FORMAT_R8G8_USCALED,
   PIPE_FORMAT_R8G8B8_USCALED,
   PIPE_FORMAT_R8G8B8A8_USCALED,
   PIPE_FORMAT_R8_SSCALED,
   PIPE_FORMAT_R8G8_SSCALED,
   PIPE_FORMAT_R8G8B8_SSCALED,
      };
   for (unsigned i = 0; i < ARRAY_SIZE(format_list); i++) {
      if (zink_is_format_supported(&screen->base, format_list[i], PIPE_BUFFER, 0, 0, PIPE_BIND_VERTEX_BUFFER))
         if (util_format_get_nr_components(format_list[i]) == 1)
         enum pipe_format decomposed = zink_decompose_vertex_format(format_list[i]);
   if (zink_is_format_supported(&screen->base, decomposed, PIPE_BUFFER, 0, 0, PIPE_BIND_VERTEX_BUFFER)) {
      screen->need_decompose_attrs = true;
            }
      static void
   populate_format_props(struct zink_screen *screen)
   {
      for (unsigned i = 0; i < PIPE_FORMAT_COUNT; i++) {
      retry:
         format = zink_get_format(screen, i);
   if (!format)
         if (VKSCR(GetPhysicalDeviceFormatProperties2)) {
                     VkDrmFormatModifierPropertiesListEXT mod_props;
   VkDrmFormatModifierPropertiesEXT mods[128];
   if (screen->info.have_EXT_image_drm_format_modifier) {
      mod_props.sType = VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT;
   mod_props.pNext = NULL;
   mod_props.drmFormatModifierCount = ARRAY_SIZE(mods);
   mod_props.pDrmFormatModifierProperties = mods;
      }
   VkFormatProperties3 props3 = {0};
   props3.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3;
   props3.pNext = props.pNext;
   props.pNext = &props3;
   VKSCR(GetPhysicalDeviceFormatProperties2)(screen->pdev, format, &props);
   screen->format_props[i].linearTilingFeatures = props3.linearTilingFeatures;
   screen->format_props[i].optimalTilingFeatures = props3.optimalTilingFeatures;
   screen->format_props[i].bufferFeatures = props3.bufferFeatures;
   if (props3.linearTilingFeatures & VK_FORMAT_FEATURE_2_LINEAR_COLOR_ATTACHMENT_BIT_NV)
         if (screen->info.have_EXT_image_drm_format_modifier && mod_props.drmFormatModifierCount) {
      screen->modifier_props[i].drmFormatModifierCount = mod_props.drmFormatModifierCount;
   screen->modifier_props[i].pDrmFormatModifierProperties = ralloc_array(screen, VkDrmFormatModifierPropertiesEXT, mod_props.drmFormatModifierCount);
   if (mod_props.pDrmFormatModifierProperties) {
      for (unsigned j = 0; j < mod_props.drmFormatModifierCount; j++)
            } else {
      VkFormatProperties props = {0};
   VKSCR(GetPhysicalDeviceFormatProperties)(screen->pdev, format, &props);
   screen->format_props[i].linearTilingFeatures = props.linearTilingFeatures;
   screen->format_props[i].optimalTilingFeatures = props.optimalTilingFeatures;
      }
   if (i == PIPE_FORMAT_A8_UNORM && !screen->driver_workarounds.missing_a8_unorm) {
      if (!screen->format_props[i].linearTilingFeatures &&
      !screen->format_props[i].optimalTilingFeatures &&
   !screen->format_props[i].bufferFeatures) {
   screen->driver_workarounds.missing_a8_unorm = true;
         }
   if (zink_format_is_emulated_alpha(i)) {
      VkFormatFeatureFlags blocked = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
   screen->format_props[i].linearTilingFeatures &= ~blocked;
   screen->format_props[i].optimalTilingFeatures &= ~blocked;
         }
   check_vertex_formats(screen);
   VkImageFormatProperties image_props;
   VkResult ret = VKSCR(GetPhysicalDeviceImageFormatProperties)(screen->pdev, VK_FORMAT_D32_SFLOAT,
                           if (ret != VK_SUCCESS && ret != VK_ERROR_FORMAT_NOT_SUPPORTED) {
         }
            if (screen->info.feats.features.sparseResidencyImage2D)
      }
      static void
   setup_renderdoc(struct zink_screen *screen)
   {
   #ifdef HAVE_RENDERDOC_APP_H
      const char *capture_id = debug_get_option("ZINK_RENDERDOC", NULL);
   if (!capture_id)
         void *renderdoc = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
   /* not loaded */
   if (!renderdoc)
            pRENDERDOC_GetAPI get_api = dlsym(renderdoc, "RENDERDOC_GetAPI");
   if (!get_api)
            /* need synchronous dispatch for renderdoc coherency */
   screen->threaded_submit = false;
   get_api(eRENDERDOC_API_Version_1_0_0, (void*)&screen->renderdoc_api);
            int count = sscanf(capture_id, "%u:%u", &screen->renderdoc_capture_start, &screen->renderdoc_capture_end);
   if (count != 2) {
      count = sscanf(capture_id, "%u", &screen->renderdoc_capture_start);
   if (!count) {
      if (!strcmp(capture_id, "all")) {
         } else {
      printf("`ZINK_RENDERDOC` usage: ZINK_RENDERDOC=all|frame_no[:end_frame_no]\n");
         }
      }
      #endif
   }
      bool
   zink_screen_init_semaphore(struct zink_screen *screen)
   {
      VkSemaphoreCreateInfo sci = {0};
   VkSemaphoreTypeCreateInfo tci = {0};
   sci.pNext = &tci;
   sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
   tci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
               }
      VkSemaphore
   zink_create_exportable_semaphore(struct zink_screen *screen)
   {
      VkExportSemaphoreCreateInfo eci = {
      VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
   NULL,
      };
   VkSemaphoreCreateInfo sci = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
   &eci,
               VkSemaphore sem = VK_NULL_HANDLE;
   if (util_dynarray_contains(&screen->fd_semaphores, VkSemaphore)) {
      simple_mtx_lock(&screen->semaphores_lock);
   if (util_dynarray_contains(&screen->fd_semaphores, VkSemaphore))
            }
   if (sem)
         VkResult ret = VKSCR(CreateSemaphore)(screen->dev, &sci, NULL, &sem);
      }
      VkSemaphore
   zink_screen_export_dmabuf_semaphore(struct zink_screen *screen, struct zink_resource *res)
   {
         #if defined(HAVE_LIBDRM) && (DETECT_OS_LINUX || DETECT_OS_BSD)
      struct dma_buf_export_sync_file export = {
      .flags = DMA_BUF_SYNC_RW,
               int fd;
   if (res->obj->is_aux) {
         } else {
      VkMemoryGetFdInfoKHR fd_info = {0};
   fd_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
   fd_info.memory = zink_bo_get_mem(res->obj->bo);
   fd_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
               int ret = drmIoctl(fd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &export);
   if (ret) {
      if (errno == ENOTTY || errno == EBADF || errno == ENOSYS) {
      assert(!"how did this fail?");
      } else {
      mesa_loge("MESA: failed to import sync file '%s'", strerror(errno));
                           const VkImportSemaphoreFdInfoKHR sdi = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR,
   .semaphore = sem,
   .flags = VK_SEMAPHORE_IMPORT_TEMPORARY_BIT,
   .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
      };
   bool success = VKSCR(ImportSemaphoreFdKHR)(screen->dev, &sdi) == VK_SUCCESS;
   close(fd);
   if (!success) {
      VKSCR(DestroySemaphore)(screen->dev, sem, NULL);
         #endif
         }
      bool
   zink_screen_import_dmabuf_semaphore(struct zink_screen *screen, struct zink_resource *res, VkSemaphore sem)
   {
   #if defined(HAVE_LIBDRM) && (DETECT_OS_LINUX || DETECT_OS_BSD)
      const VkSemaphoreGetFdInfoKHR get_fd_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
   .semaphore = sem,
      };
   int sync_file_fd = -1;
   VkResult result = VKSCR(GetSemaphoreFdKHR)(screen->dev, &get_fd_info, &sync_file_fd);
   if (result != VK_SUCCESS) {
                  bool ret = false;
   int fd;
   if (res->obj->is_aux) {
         } else {
      VkMemoryGetFdInfoKHR fd_info = {0};
   fd_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
   fd_info.memory = zink_bo_get_mem(res->obj->bo);
   fd_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
   if (VKSCR(GetMemoryFdKHR)(screen->dev, &fd_info, &fd) != VK_SUCCESS)
      }
   if (fd != -1) {
      struct dma_buf_import_sync_file import = {
      .flags = DMA_BUF_SYNC_RW,
      };
   int ret = drmIoctl(fd, DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &import);
   if (ret) {
      if (errno == ENOTTY || errno == EBADF || errno == ENOSYS) {
         } else {
            }
      }
   close(sync_file_fd);
      #else
         #endif
   }
      bool
   zink_screen_timeline_wait(struct zink_screen *screen, uint64_t batch_id, uint64_t timeout)
   {
               if (zink_screen_check_last_finished(screen, batch_id))
            wi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
   wi.semaphoreCount = 1;
   wi.pSemaphores = &screen->sem;
   wi.pValues = &batch_id;
   bool success = false;
   if (screen->device_lost)
         VkResult ret = VKSCR(WaitSemaphores)(screen->dev, &wi, timeout);
            if (success)
               }
      static uint32_t
   zink_get_loader_version(struct zink_screen *screen)
   {
                  // Get the Loader version
   GET_PROC_ADDR_INSTANCE_LOCAL(screen, NULL, EnumerateInstanceVersion);
   if (vk_EnumerateInstanceVersion) {
      uint32_t loader_version_temp = VK_API_VERSION_1_0;
   VkResult result = (*vk_EnumerateInstanceVersion)(&loader_version_temp);
   if (VK_SUCCESS == result) {
         } else {
                        }
      static void
   zink_query_memory_info(struct pipe_screen *pscreen, struct pipe_memory_info *info)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   memset(info, 0, sizeof(struct pipe_memory_info));
   if (screen->info.have_EXT_memory_budget && VKSCR(GetPhysicalDeviceMemoryProperties2)) {
      VkPhysicalDeviceMemoryProperties2 mem = {0};
            VkPhysicalDeviceMemoryBudgetPropertiesEXT budget = {0};
   budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
   mem.pNext = &budget;
            for (unsigned i = 0; i < mem.memoryProperties.memoryHeapCount; i++) {
      if (mem.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
      /* VRAM */
   info->total_device_memory += mem.memoryProperties.memoryHeaps[i].size / 1024;
      } else {
      /* GART */
   info->total_staging_memory += mem.memoryProperties.memoryHeaps[i].size / 1024;
         }
      } else {
      for (unsigned i = 0; i < screen->info.mem_props.memoryHeapCount; i++) {
      if (screen->info.mem_props.memoryHeaps[i].flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
      /* VRAM */
   info->total_device_memory += screen->info.mem_props.memoryHeaps[i].size / 1024;
   /* free real estate! */
      } else {
      /* GART */
   info->total_staging_memory += screen->info.mem_props.memoryHeaps[i].size / 1024;
   /* free real estate! */
               }
      static void
   zink_query_dmabuf_modifiers(struct pipe_screen *pscreen, enum pipe_format format, int max, uint64_t *modifiers, unsigned int *external_only, int *count)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   *count = screen->modifier_props[format].drmFormatModifierCount;
   for (int i = 0; i < MIN2(max, *count); i++) {
      if (external_only)
                  }
      static bool
   zink_is_dmabuf_modifier_supported(struct pipe_screen *pscreen, uint64_t modifier, enum pipe_format format, bool *external_only)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   for (unsigned i = 0; i < screen->modifier_props[format].drmFormatModifierCount; i++)
      if (screen->modifier_props[format].pDrmFormatModifierProperties[i].drmFormatModifier == modifier)
         }
      static unsigned
   zink_get_dmabuf_modifier_planes(struct pipe_screen *pscreen, uint64_t modifier, enum pipe_format format)
   {
      struct zink_screen *screen = zink_screen(pscreen);
   for (unsigned i = 0; i < screen->modifier_props[format].drmFormatModifierCount; i++)
      if (screen->modifier_props[format].pDrmFormatModifierProperties[i].drmFormatModifier == modifier)
         }
      static int
   zink_get_sparse_texture_virtual_page_size(struct pipe_screen *pscreen,
                                 {
      struct zink_screen *screen = zink_screen(pscreen);
   static const int page_size_2d[][3] = {
      { 256, 256, 1 }, /* 8bpp   */
   { 256, 128, 1 }, /* 16bpp  */
   { 128, 128, 1 }, /* 32bpp  */
   { 128, 64,  1 }, /* 64bpp  */
      };
   static const int page_size_3d[][3] = {
      { 64,  32,  32 }, /* 8bpp   */
   { 32,  32,  32 }, /* 16bpp  */
   { 32,  32,  16 }, /* 32bpp  */
   { 32,  16,  16 }, /* 64bpp  */
      };
   /* Only support one type of page size. */
   if (offset != 0)
            /* reject multisample if 2x isn't supported; assume none are */
   if (multi_sample && !screen->info.feats.features.sparseResidency2Samples)
            VkFormat format = zink_get_format(screen, pformat);
   bool is_zs = util_format_is_depth_or_stencil(pformat);
   VkImageType type;
   switch (target) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      type = (screen->need_2D_sparse || (screen->need_2D_zs && is_zs)) ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D;
         case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
      type = VK_IMAGE_TYPE_2D;
         case PIPE_TEXTURE_3D:
      type = VK_IMAGE_TYPE_3D;
         case PIPE_BUFFER:
            default:
         }
   VkImageUsageFlags flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
         flags |= is_zs ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   VkSparseImageFormatProperties props[4]; //planar?
   unsigned prop_count = ARRAY_SIZE(props);
   VKSCR(GetPhysicalDeviceSparseImageFormatProperties)(screen->pdev, format, type,
                           if (!prop_count) {
      if (pformat == PIPE_FORMAT_R9G9B9E5_FLOAT) {
      screen->faked_e5sparse = true;
      }
               if (size) {
      if (x)
         if (y)
         if (z)
                  hack_it_up:
      {
      const int (*page_sizes)[3] = target == PIPE_TEXTURE_3D ? page_size_3d : page_size_2d;
            if (size) {
      unsigned index = util_logbase2(blk_size);
   if (x) *x = page_sizes[index][0];
   if (y) *y = page_sizes[index][1];
         }
      }
      static VkDevice
   zink_create_logical_device(struct zink_screen *screen)
   {
               VkDeviceQueueCreateInfo qci[2] = {0};
   uint32_t queues[3] = {
      screen->gfx_queue,
      };
   float dummy = 0.0f;
   for (unsigned i = 0; i < ARRAY_SIZE(qci); i++) {
      qci[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
   qci[i].queueFamilyIndex = queues[i];
   qci[i].queueCount = 1;
               unsigned num_queues = 1;
   if (screen->sparse_queue != screen->gfx_queue)
            VkDeviceCreateInfo dci = {0};
   dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   dci.queueCreateInfoCount = num_queues;
   dci.pQueueCreateInfos = qci;
   /* extensions don't have bool members in pEnabledFeatures.
   * this requires us to pass the whole VkPhysicalDeviceFeatures2 struct
   */
   if (screen->info.feats.sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2) {
         } else {
                  dci.ppEnabledExtensionNames = screen->info.extensions;
            VkResult result = VKSCR(CreateDevice)(screen->pdev, &dci, NULL, &dev);
   if (result != VK_SUCCESS)
               }
      static void
   check_base_requirements(struct zink_screen *screen)
   {
      if (zink_debug & ZINK_DEBUG_QUIET)
         if (screen->info.driver_props.driverID == VK_DRIVER_ID_MESA_V3DV) {
      /* v3dv doesn't support straddling i/o, but zink doesn't do that so this is effectively supported:
   * don't spam errors in this case
   */
   screen->info.feats12.scalarBlockLayout = true;
      }
   if (!screen->info.feats.features.logicOp ||
      !screen->info.feats.features.fillModeNonSolid ||
   !screen->info.feats.features.shaderClipDistance ||
   !(screen->info.feats12.scalarBlockLayout ||
         !screen->info.have_KHR_maintenance1 ||
   !screen->info.have_EXT_custom_border_color ||
   !screen->info.have_EXT_line_rasterization) {
   fprintf(stderr, "WARNING: Some incorrect rendering "
         #define CHECK_OR_PRINT(X) \
         if (!screen->info.X) \
         CHECK_OR_PRINT(feats.features.logicOp);
   CHECK_OR_PRINT(feats.features.fillModeNonSolid);
   CHECK_OR_PRINT(feats.features.shaderClipDistance);
   if (!screen->info.feats12.scalarBlockLayout && !screen->info.have_EXT_scalar_block_layout)
         CHECK_OR_PRINT(have_KHR_maintenance1);
   CHECK_OR_PRINT(have_EXT_custom_border_color);
   CHECK_OR_PRINT(have_EXT_line_rasterization);
      }
   if (screen->info.driver_props.driverID == VK_DRIVER_ID_MESA_V3DV) {
      screen->info.feats12.scalarBlockLayout = false;
         }
      static void
   zink_get_sample_pixel_grid(struct pipe_screen *pscreen, unsigned sample_count,
         {
      struct zink_screen *screen = zink_screen(pscreen);
   unsigned idx = util_logbase2_ceil(MAX2(sample_count, 1));
   assert(idx < ARRAY_SIZE(screen->maxSampleLocationGridSize));
   *width = screen->maxSampleLocationGridSize[idx].width;
      }
      static void
   init_driver_workarounds(struct zink_screen *screen)
   {
      /* enable implicit sync for all non-mesa drivers */
   screen->driver_workarounds.implicit_sync = true;
   switch (screen->info.driver_props.driverID) {
   case VK_DRIVER_ID_MESA_RADV:
   case VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA:
   case VK_DRIVER_ID_MESA_LLVMPIPE:
   case VK_DRIVER_ID_MESA_TURNIP:
   case VK_DRIVER_ID_MESA_V3DV:
   case VK_DRIVER_ID_MESA_PANVK:
   case VK_DRIVER_ID_MESA_VENUS:
      screen->driver_workarounds.implicit_sync = false;
      default:
         }
   /* TODO: maybe compile multiple variants for different set counts for compact mode? */
   if (screen->info.props.limits.maxBoundDescriptorSets < ZINK_DESCRIPTOR_ALL_TYPES ||
      zink_debug & (ZINK_DEBUG_COMPACT | ZINK_DEBUG_NOSHOBJ))
      /* EDS2 is only used with EDS1 */
   if (!screen->info.have_EXT_extended_dynamic_state) {
      screen->info.have_EXT_extended_dynamic_state2 = false;
   /* CWE usage needs EDS1 */
      }
   if (screen->info.driver_props.driverID == VK_DRIVER_ID_AMD_PROPRIETARY)
      /* this completely breaks xfb somehow */
      /* EDS3 is only used with EDS2 */
   if (!screen->info.have_EXT_extended_dynamic_state2)
         /* EXT_vertex_input_dynamic_state is only used with EDS2 and above */
   if (!screen->info.have_EXT_extended_dynamic_state2)
         if (screen->info.line_rast_feats.stippledRectangularLines &&
      screen->info.line_rast_feats.stippledBresenhamLines &&
   screen->info.line_rast_feats.stippledSmoothLines &&
   !screen->info.dynamic_state3_feats.extendedDynamicState3LineStippleEnable)
      if (!screen->info.dynamic_state3_feats.extendedDynamicState3PolygonMode ||
      !screen->info.dynamic_state3_feats.extendedDynamicState3DepthClampEnable ||
   !screen->info.dynamic_state3_feats.extendedDynamicState3DepthClipNegativeOneToOne ||
   !screen->info.dynamic_state3_feats.extendedDynamicState3DepthClipEnable ||
   !screen->info.dynamic_state3_feats.extendedDynamicState3ProvokingVertexMode ||
   !screen->info.dynamic_state3_feats.extendedDynamicState3LineRasterizationMode)
      else if (screen->info.dynamic_state3_feats.extendedDynamicState3SampleMask &&
            screen->info.dynamic_state3_feats.extendedDynamicState3AlphaToCoverageEnable &&
   (!screen->info.feats.features.alphaToOne || screen->info.dynamic_state3_feats.extendedDynamicState3AlphaToOneEnable) &&
   screen->info.dynamic_state3_feats.extendedDynamicState3ColorBlendEnable &&
   screen->info.dynamic_state3_feats.extendedDynamicState3RasterizationSamples &&
   screen->info.dynamic_state3_feats.extendedDynamicState3ColorWriteMask &&
   screen->info.dynamic_state3_feats.extendedDynamicState3ColorBlendEquation &&
   screen->info.dynamic_state3_feats.extendedDynamicState3LogicOpEnable &&
      if (screen->info.have_EXT_graphics_pipeline_library)
      screen->info.have_EXT_graphics_pipeline_library = screen->info.have_EXT_extended_dynamic_state &&
                                                      screen->driver_workarounds.broken_l4a4 = screen->info.driver_props.driverID == VK_DRIVER_ID_NVIDIA_PROPRIETARY;
   if (screen->info.driver_props.driverID == VK_DRIVER_ID_MESA_TURNIP) {
      /* performance */
      }
   if (!screen->info.have_KHR_maintenance5)
            if ((!screen->info.have_EXT_line_rasterization ||
      !screen->info.line_rast_feats.stippledBresenhamLines) &&
   screen->info.feats.features.geometryShader &&
   screen->info.feats.features.sampleRateShading) {
   /* we're using stippledBresenhamLines as a proxy for all of these, to
   * avoid accidentally changing behavior on VK-drivers where we don't
   * want to add emulation.
   */
               if (screen->info.driver_props.driverID ==
      VK_DRIVER_ID_IMAGINATION_PROPRIETARY) {
   assert(screen->info.feats.features.geometryShader);
               /* This is a workarround for the lack of
   * gl_PointSize + glPolygonMode(..., GL_LINE), in the imagination
   * proprietary driver.
   */
   switch (screen->info.driver_props.driverID) {
   case VK_DRIVER_ID_IMAGINATION_PROPRIETARY:
      screen->driver_workarounds.no_hw_gl_point = true;
      default:
      screen->driver_workarounds.no_hw_gl_point = false;
               if (screen->info.driver_props.driverID == VK_DRIVER_ID_AMD_OPEN_SOURCE || 
      screen->info.driver_props.driverID == VK_DRIVER_ID_AMD_PROPRIETARY || 
   screen->info.driver_props.driverID == VK_DRIVER_ID_NVIDIA_PROPRIETARY || 
   screen->info.driver_props.driverID == VK_DRIVER_ID_MESA_RADV)
      else
         if (screen->info.driver_props.driverID == VK_DRIVER_ID_NVIDIA_PROPRIETARY)
         else
         /* these drivers don't use VK_PIPELINE_CREATE_COLOR_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT, so it can always be set */
   switch (screen->info.driver_props.driverID) {
   case VK_DRIVER_ID_MESA_RADV:
   case VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA:
   case VK_DRIVER_ID_MESA_LLVMPIPE:
   case VK_DRIVER_ID_MESA_VENUS:
   case VK_DRIVER_ID_NVIDIA_PROPRIETARY:
   case VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS:
   case VK_DRIVER_ID_IMAGINATION_PROPRIETARY:
      screen->driver_workarounds.always_feedback_loop = screen->info.have_EXT_attachment_feedback_loop_layout;
      default:
         }
   /* these drivers don't use VK_PIPELINE_CREATE_DEPTH_STENCIL_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT, so it can always be set */
   switch (screen->info.driver_props.driverID) {
   case VK_DRIVER_ID_MESA_LLVMPIPE:
   case VK_DRIVER_ID_MESA_VENUS:
   case VK_DRIVER_ID_NVIDIA_PROPRIETARY:
   case VK_DRIVER_ID_IMAGINATION_PROPRIETARY:
      screen->driver_workarounds.always_feedback_loop_zs = screen->info.have_EXT_attachment_feedback_loop_layout;
      default:
         }
   /* use same mechanics if dynamic state is supported */
   screen->driver_workarounds.always_feedback_loop |= screen->info.have_EXT_attachment_feedback_loop_dynamic_state;
            /* these drivers cannot handle OOB gl_Layer values, and therefore need clamping in shader.
   * TODO: Vulkan extension that details whether vulkan driver can handle OOB layer values
   */
   switch (screen->info.driver_props.driverID) {
   case VK_DRIVER_ID_IMAGINATION_PROPRIETARY:
      screen->driver_workarounds.needs_sanitised_layer = true;
      default:
      screen->driver_workarounds.needs_sanitised_layer = false;
      }
   /* these drivers will produce undefined results when using swizzle 1 with combined z/s textures
   * TODO: use a future device property when available
   */
   switch (screen->info.driver_props.driverID) {
   case VK_DRIVER_ID_IMAGINATION_PROPRIETARY:
   case VK_DRIVER_ID_IMAGINATION_OPEN_SOURCE_MESA:
      screen->driver_workarounds.needs_zs_shader_swizzle = true;
      default:
      screen->driver_workarounds.needs_zs_shader_swizzle = false;
               /* When robust contexts are advertised but robustImageAccess2 is not available */
   screen->driver_workarounds.lower_robustImageAccess2 =
      !screen->info.rb2_feats.robustImageAccess2 &&
   screen->info.feats.features.robustBufferAccess &&
         /* once more testing has been done, use the #if 0 block */
   unsigned illegal = ZINK_DEBUG_RP | ZINK_DEBUG_NORP;
   if ((zink_debug & illegal) == illegal) {
      mesa_loge("Cannot specify ZINK_DEBUG=rp and ZINK_DEBUG=norp");
               /* these drivers benefit from renderpass optimization */
   switch (screen->info.driver_props.driverID) {
   case VK_DRIVER_ID_MESA_LLVMPIPE:
   case VK_DRIVER_ID_MESA_TURNIP:
   case VK_DRIVER_ID_MESA_PANVK:
   case VK_DRIVER_ID_MESA_V3DV:
   case VK_DRIVER_ID_IMAGINATION_PROPRIETARY:
   case VK_DRIVER_ID_QUALCOMM_PROPRIETARY:
   case VK_DRIVER_ID_BROADCOM_PROPRIETARY:
   case VK_DRIVER_ID_ARM_PROPRIETARY:
      screen->driver_workarounds.track_renderpasses = true; //screen->info.primgen_feats.primitivesGeneratedQueryWithRasterizerDiscard
      default:
         }
   if (zink_debug & ZINK_DEBUG_RP)
         else if (zink_debug & ZINK_DEBUG_NORP)
            /* these drivers can't optimize non-overlapping copy ops */
   switch (screen->info.driver_props.driverID) {
   case VK_DRIVER_ID_MESA_TURNIP:
   case VK_DRIVER_ID_QUALCOMM_PROPRIETARY:
      screen->driver_workarounds.broken_cache_semantics = true;
      default:
                  /* these drivers can successfully do INVALID <-> LINEAR dri3 modifier swap */
   switch (screen->info.driver_props.driverID) {
   case VK_DRIVER_ID_MESA_TURNIP:
   case VK_DRIVER_ID_MESA_VENUS:
      screen->driver_workarounds.can_do_invalid_linear_modifier = true;
      default:
                  /* these drivers have no difference between unoptimized and optimized shader compilation */
   switch (screen->info.driver_props.driverID) {
   case VK_DRIVER_ID_MESA_LLVMPIPE:
      screen->driver_workarounds.disable_optimized_compile = true;
      default:
      if (zink_debug & ZINK_DEBUG_NOOPT)
                     switch (screen->info.driver_props.driverID) {
   case VK_DRIVER_ID_MESA_RADV:
   case VK_DRIVER_ID_AMD_OPEN_SOURCE:
   case VK_DRIVER_ID_AMD_PROPRIETARY:
      /* this has bad perf on AMD */
   screen->info.have_KHR_push_descriptor = false;
      default:
                  if (!screen->resizable_bar)
      }
      static void
   fixup_driver_props(struct zink_screen *screen)
   {
      VkPhysicalDeviceProperties2 props = {
         };
   if (screen->info.have_EXT_host_image_copy) {
      /* fill in layouts */
   screen->info.hic_props.pNext = props.pNext;
   props.pNext = &screen->info.hic_props;
   screen->info.hic_props.pCopySrcLayouts = ralloc_array(screen, VkImageLayout, screen->info.hic_props.copySrcLayoutCount);
      }
   if (props.pNext)
            if (screen->info.have_EXT_host_image_copy) {
      for (unsigned i = 0; i < screen->info.hic_props.copyDstLayoutCount; i++) {
      if (screen->info.hic_props.pCopyDstLayouts[i] == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      screen->can_hic_shader_read = true;
               }
      static void
   init_optimal_keys(struct zink_screen *screen)
   {
      /* assume that anyone who knows enough to enable optimal_keys on turnip doesn't care about missing line stipple */
   if (zink_debug & ZINK_DEBUG_OPTIMAL_KEYS && screen->info.driver_props.driverID == VK_DRIVER_ID_MESA_TURNIP)
         screen->optimal_keys = !screen->need_decompose_attrs &&
                        screen->info.have_EXT_non_seamless_cube_map &&
   screen->info.have_EXT_provoking_vertex &&
   !screen->driconf.inline_uniforms &&
   !screen->driver_workarounds.no_linestipple &&
   !screen->driver_workarounds.no_linesmooth &&
      if (!screen->optimal_keys && zink_debug & ZINK_DEBUG_OPTIMAL_KEYS && !(zink_debug & ZINK_DEBUG_QUIET)) {
      fprintf(stderr, "The following criteria are preventing optimal_keys enablement:\n");
   if (screen->need_decompose_attrs)
         if (screen->driconf.inline_uniforms)
         if (screen->driconf.emulate_point_smooth)
         if (screen->driver_workarounds.needs_zs_shader_swizzle)
         CHECK_OR_PRINT(have_EXT_line_rasterization);
   CHECK_OR_PRINT(line_rast_feats.stippledBresenhamLines);
   CHECK_OR_PRINT(feats.features.geometryShader);
   CHECK_OR_PRINT(feats.features.sampleRateShading);
   CHECK_OR_PRINT(have_EXT_non_seamless_cube_map);
   CHECK_OR_PRINT(have_EXT_provoking_vertex);
   if (screen->driver_workarounds.no_linesmooth)
         if (screen->driver_workarounds.no_hw_gl_point)
         CHECK_OR_PRINT(rb2_feats.robustImageAccess2);
   CHECK_OR_PRINT(feats.features.robustBufferAccess);
   CHECK_OR_PRINT(rb_image_feats.robustImageAccess);
   printf("\n");
      }
   if (zink_debug & ZINK_DEBUG_OPTIMAL_KEYS)
         if (!screen->optimal_keys)
            if (!screen->optimal_keys ||
      !screen->info.have_KHR_maintenance5 ||
   /* EXT_shader_object needs either dynamic feedback loop or per-app enablement */
   (!screen->driconf.zink_shader_object_enable && !screen->info.have_EXT_attachment_feedback_loop_dynamic_state))
      if (screen->info.have_EXT_shader_object)
         if (zink_debug & ZINK_DEBUG_DGC) {
      if (!screen->optimal_keys) {
      mesa_loge("zink: can't DGC without optimal_keys!");
      } else {
      screen->info.have_EXT_multi_draw = false;
   screen->info.have_EXT_shader_object = false;
   screen->info.have_EXT_graphics_pipeline_library = false;
            }
      static struct disk_cache *
   zink_get_disk_shader_cache(struct pipe_screen *_screen)
   {
                  }
      VkSemaphore
   zink_create_semaphore(struct zink_screen *screen)
   {
      VkSemaphoreCreateInfo sci = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
   NULL,
      };
   VkSemaphore sem = VK_NULL_HANDLE;
   if (util_dynarray_contains(&screen->semaphores, VkSemaphore)) {
      simple_mtx_lock(&screen->semaphores_lock);
   if (util_dynarray_contains(&screen->semaphores, VkSemaphore))
            }
   if (sem)
         VkResult ret = VKSCR(CreateSemaphore)(screen->dev, &sci, NULL, &sem);
      }
      void
   zink_screen_lock_context(struct zink_screen *screen)
   {
      simple_mtx_lock(&screen->copy_context_lock);
   if (!screen->copy_context)
         if (!screen->copy_context) {
      mesa_loge("zink: failed to create copy context");
         }
      void
   zink_screen_unlock_context(struct zink_screen *screen)
   {
         }
      static bool
   init_layouts(struct zink_screen *screen)
   {
      if (screen->info.have_EXT_descriptor_indexing) {
      VkDescriptorSetLayoutBinding bindings[4];
   const unsigned num_bindings = 4;
   VkDescriptorSetLayoutCreateInfo dcslci = {0};
   dcslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   dcslci.pNext = NULL;
   VkDescriptorSetLayoutBindingFlagsCreateInfo fci = {0};
   VkDescriptorBindingFlags flags[4];
   dcslci.pNext = &fci;
   if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_DB)
         else
         fci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
   fci.bindingCount = num_bindings;
   fci.pBindingFlags = flags;
   for (unsigned i = 0; i < num_bindings; i++) {
      flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
   if (zink_descriptor_mode != ZINK_DESCRIPTOR_MODE_DB)
      }
   /* there is exactly 1 bindless descriptor set per context, and it has 4 bindings, 1 for each descriptor type */
   for (unsigned i = 0; i < num_bindings; i++) {
      bindings[i].binding = i;
   bindings[i].descriptorType = zink_descriptor_type_from_bindless_index(i);
   bindings[i].descriptorCount = ZINK_MAX_BINDLESS_HANDLES;
   bindings[i].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;
               dcslci.bindingCount = num_bindings;
   dcslci.pBindings = bindings;
   VkResult result = VKSCR(CreateDescriptorSetLayout)(screen->dev, &dcslci, 0, &screen->bindless_layout);
   if (result != VK_SUCCESS) {
      mesa_loge("ZINK: vkCreateDescriptorSetLayout failed (%s)", vk_Result_to_str(result));
                  screen->gfx_push_constant_layout = zink_pipeline_layout_create(screen, NULL, 0, false, 0);
      }
      static int
   zink_screen_get_fd(struct pipe_screen *pscreen)
   {
                  }
      static struct zink_screen *
   zink_internal_create_screen(const struct pipe_screen_config *config, int64_t dev_major, int64_t dev_minor)
   {
      if (getenv("ZINK_USE_LAVAPIPE")) {
      mesa_loge("ZINK_USE_LAVAPIPE is obsolete. Use LIBGL_ALWAYS_SOFTWARE\n");
               struct zink_screen *screen = rzalloc(NULL, struct zink_screen);
   if (!screen) {
      mesa_loge("ZINK: failed to allocate screen");
                        glsl_type_singleton_init_or_ref();
   zink_debug = debug_get_option_zink_debug();
   if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_AUTO)
            screen->threaded = util_get_cpu_caps()->nr_cpus > 1 && debug_get_bool_option("GALLIUM_THREAD", util_get_cpu_caps()->nr_cpus > 1);
   if (zink_debug & ZINK_DEBUG_FLUSHSYNC)
         else
                              screen->loader_lib = util_dl_open(VK_LIBNAME);
   if (!screen->loader_lib) {
      mesa_loge("ZINK: failed to load "VK_LIBNAME);
               screen->vk_GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)util_dl_get_proc_address(screen->loader_lib, "vkGetInstanceProcAddr");
   screen->vk_GetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)util_dl_get_proc_address(screen->loader_lib, "vkGetDeviceProcAddr");
   if (!screen->vk_GetInstanceProcAddr ||
      !screen->vk_GetDeviceProcAddr) {
   mesa_loge("ZINK: failed to get proc address");
               screen->instance_info.loader_version = zink_get_loader_version(screen);
   if (config) {
      driParseConfigFiles(config->options, config->options_info, 0, "zink",
         screen->driconf.dual_color_blend_by_location = driQueryOptionb(config->options, "dual_color_blend_by_location");
   screen->driconf.glsl_correct_derivatives_after_discard = driQueryOptionb(config->options, "glsl_correct_derivatives_after_discard");
   //screen->driconf.inline_uniforms = driQueryOptionb(config->options, "radeonsi_inline_uniforms");
   screen->driconf.emulate_point_smooth = driQueryOptionb(config->options, "zink_emulate_point_smooth");
               if (!zink_create_instance(screen, dev_major > 0 && dev_major < 255))
            if (zink_debug & ZINK_DEBUG_VALIDATION) {
      if (!screen->instance_info.have_layer_KHRONOS_validation &&
      !screen->instance_info.have_layer_LUNARG_standard_validation) {
   mesa_loge("Failed to load validation layer");
                  vk_instance_dispatch_table_load(&screen->vk.instance,
               vk_physical_device_dispatch_table_load(&screen->vk.physical_device,
                           if (screen->instance_info.have_EXT_debug_utils &&
      (zink_debug & ZINK_DEBUG_VALIDATION) && !create_debug(screen))
         choose_pdev(screen, dev_major, dev_minor);
   if (screen->pdev == VK_NULL_HANDLE) {
      mesa_loge("ZINK: failed to choose pdev");
      }
                     screen->have_X8_D24_UNORM_PACK32 = zink_is_depth_format_supported(screen,
         screen->have_D24_UNORM_S8_UINT = zink_is_depth_format_supported(screen,
         screen->have_D32_SFLOAT_S8_UINT = zink_is_depth_format_supported(screen,
            if (!zink_get_physical_device_info(screen)) {
      debug_printf("ZINK: failed to detect features\n");
               memset(&screen->heap_map, UINT8_MAX, sizeof(screen->heap_map));
   for (enum zink_heap i = 0; i < ZINK_HEAP_MAX; i++) {
      for (unsigned j = 0; j < screen->info.mem_props.memoryTypeCount; j++) {
      VkMemoryPropertyFlags domains = vk_domain_from_heap(i);
   if ((screen->info.mem_props.memoryTypes[j].propertyFlags & domains) == domains) {
               }
   /* iterate again to check for missing heaps */
   for (enum zink_heap i = 0; i < ZINK_HEAP_MAX; i++) {
      /* not found: use compatible heap */
   if (screen->heap_map[i][0] == UINT8_MAX) {
      /* only cached mem has a failure case for now */
   assert(i == ZINK_HEAP_HOST_VISIBLE_COHERENT_CACHED || i == ZINK_HEAP_DEVICE_LOCAL_LAZY ||
         if (i == ZINK_HEAP_HOST_VISIBLE_COHERENT_CACHED) {
      memcpy(screen->heap_map[i], screen->heap_map[ZINK_HEAP_HOST_VISIBLE_COHERENT], screen->heap_count[ZINK_HEAP_HOST_VISIBLE_COHERENT]);
      } else {
      memcpy(screen->heap_map[i], screen->heap_map[ZINK_HEAP_DEVICE_LOCAL], screen->heap_count[ZINK_HEAP_DEVICE_LOCAL]);
            }
   {
      uint64_t biggest_vis_vram = 0;
   for (unsigned i = 0; i < screen->heap_count[ZINK_HEAP_DEVICE_LOCAL_VISIBLE]; i++)
         uint64_t biggest_vram = 0;
   for (unsigned i = 0; i < screen->heap_count[ZINK_HEAP_DEVICE_LOCAL]; i++)
         /* determine if vis vram is roughly equal to total vram */
   if (biggest_vis_vram > biggest_vram * 0.9)
               setup_renderdoc(screen);
   if (screen->threaded_submit && !util_queue_init(&screen->flush_queue, "zfq", 8, 1, UTIL_QUEUE_INIT_RESIZE_IF_FULL, screen)) {
      mesa_loge("zink: Failed to create flush queue.\n");
               zink_internal_setup_moltenvk(screen);
   if (!screen->info.have_KHR_timeline_semaphore && !screen->info.feats12.timelineSemaphore) {
      mesa_loge("zink: KHR_timeline_semaphore is required");
      }
   if (zink_debug & ZINK_DEBUG_DGC) {
      if (!screen->info.have_NV_device_generated_commands) {
      mesa_loge("zink: can't use DGC without NV_device_generated_commands");
                  if (zink_debug & ZINK_DEBUG_MEM) {
      simple_mtx_init(&screen->debug_mem_lock, mtx_plain);
                                 screen->dev = zink_create_logical_device(screen);
   if (!screen->dev)
            vk_device_dispatch_table_load(&screen->vk.device,
                                    /* descriptor set indexing is determined by 'compact' descriptor mode:
   * by default, 6 sets are used to provide more granular updating
   * in compact mode, a maximum of 4 sets are used, with like-types combined
   */
   if ((zink_debug & ZINK_DEBUG_COMPACT) ||
      screen->info.props.limits.maxBoundDescriptorSets < ZINK_MAX_DESCRIPTOR_SETS) {
   screen->desc_set_id[ZINK_DESCRIPTOR_TYPE_UNIFORMS] = 0;
   screen->desc_set_id[ZINK_DESCRIPTOR_TYPE_UBO] = 1;
   screen->desc_set_id[ZINK_DESCRIPTOR_TYPE_SSBO] = 1;
   screen->desc_set_id[ZINK_DESCRIPTOR_TYPE_SAMPLER_VIEW] = 2;
   screen->desc_set_id[ZINK_DESCRIPTOR_TYPE_IMAGE] = 2;
   screen->desc_set_id[ZINK_DESCRIPTOR_BINDLESS] = 3;
      } else {
      screen->desc_set_id[ZINK_DESCRIPTOR_TYPE_UNIFORMS] = 0;
   screen->desc_set_id[ZINK_DESCRIPTOR_TYPE_UBO] = 1;
   screen->desc_set_id[ZINK_DESCRIPTOR_TYPE_SAMPLER_VIEW] = 2;
   screen->desc_set_id[ZINK_DESCRIPTOR_TYPE_SSBO] = 3;
   screen->desc_set_id[ZINK_DESCRIPTOR_TYPE_IMAGE] = 4;
               if (screen->info.have_EXT_calibrated_timestamps && !check_have_device_time(screen))
               #if defined(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)
      if (screen->info.have_KHR_portability_subset) {
            #endif // VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
         check_base_requirements(screen);
            screen->base.get_name = zink_get_name;
   if (screen->instance_info.have_KHR_external_memory_capabilities) {
      screen->base.get_device_uuid = zink_get_device_uuid;
      }
   if (screen->info.have_KHR_external_memory_win32) {
      screen->base.get_device_luid = zink_get_device_luid;
      }
   screen->base.set_max_shader_compiler_threads = zink_set_max_shader_compiler_threads;
   screen->base.is_parallel_shader_compilation_finished = zink_is_parallel_shader_compilation_finished;
   screen->base.get_vendor = zink_get_vendor;
   screen->base.get_device_vendor = zink_get_device_vendor;
   screen->base.get_compute_param = zink_get_compute_param;
   screen->base.get_timestamp = zink_get_timestamp;
   screen->base.query_memory_info = zink_query_memory_info;
   screen->base.get_param = zink_get_param;
   screen->base.get_paramf = zink_get_paramf;
   screen->base.get_shader_param = zink_get_shader_param;
   screen->base.get_compiler_options = zink_get_compiler_options;
   screen->base.get_sample_pixel_grid = zink_get_sample_pixel_grid;
   screen->base.is_compute_copy_faster = zink_is_compute_copy_faster;
   screen->base.is_format_supported = zink_is_format_supported;
   screen->base.driver_thread_add_job = zink_driver_thread_add_job;
   if (screen->info.have_EXT_image_drm_format_modifier && screen->info.have_EXT_external_memory_dma_buf) {
      screen->base.query_dmabuf_modifiers = zink_query_dmabuf_modifiers;
   screen->base.is_dmabuf_modifier_supported = zink_is_dmabuf_modifier_supported;
         #if defined(_WIN32)
      if (screen->info.have_KHR_external_memory_win32)
      #endif
      screen->base.context_create = zink_context_create;
   screen->base.flush_frontbuffer = zink_flush_frontbuffer;
   screen->base.destroy = zink_destroy_screen;
   screen->base.finalize_nir = zink_shader_finalize;
   screen->base.get_disk_shader_cache = zink_get_disk_shader_cache;
            if (screen->info.have_EXT_sample_locations) {
      VkMultisamplePropertiesEXT prop;
   prop.sType = VK_STRUCTURE_TYPE_MULTISAMPLE_PROPERTIES_EXT;
   prop.pNext = NULL;
   for (unsigned i = 0; i < ARRAY_SIZE(screen->maxSampleLocationGridSize); i++) {
      if (screen->info.sample_locations_props.sampleLocationSampleCounts & (1 << i)) {
      VKSCR(GetPhysicalDeviceMultisamplePropertiesEXT)(screen->pdev, 1 << i, &prop);
                     if (!zink_screen_resource_init(&screen->base))
         if (!zink_bo_init(screen)) {
      mesa_loge("ZINK: failed to initialize suballocator");
      }
            zink_screen_init_compiler(screen);
   if (!disk_cache_init(screen)) {
      mesa_loge("ZINK: failed to initialize disk cache");
      }
   if (!util_queue_init(&screen->cache_get_thread, "zcfq", 8, 4,
                                          screen->total_video_mem = get_video_mem(screen);
   screen->clamp_video_mem = screen->total_video_mem * 0.8;
   if (!os_get_total_physical_memory(&screen->total_mem)) {
      mesa_loge("ZINK: failed to get total physical memory");
               if (!zink_screen_init_semaphore(screen)) {
      mesa_loge("zink: failed to create timeline semaphore");
               bool can_db = true;
   {
      if (!screen->info.have_EXT_descriptor_buffer) {
      if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_DB) {
      mesa_loge("Cannot use db descriptor mode without EXT_descriptor_buffer");
      }
      }
   if (!screen->resizable_bar) {
      if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_DB) {
      mesa_loge("Cannot use db descriptor mode without resizable bar");
      }
      }
   if (!screen->info.have_EXT_non_seamless_cube_map) {
      if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_DB) {
      mesa_loge("Cannot use db descriptor mode without EXT_non_seamless_cube_map");
      }
      }
   if (!screen->info.rb2_feats.nullDescriptor) {
      if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_DB) {
      mesa_loge("Cannot use db descriptor mode without robustness2.nullDescriptor");
      }
      }
   if (ZINK_FBFETCH_DESCRIPTOR_SIZE < screen->info.db_props.inputAttachmentDescriptorSize) {
      if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_DB) {
      mesa_loge("Cannot use db descriptor mode with inputAttachmentDescriptorSize(%u) > %u", (unsigned)screen->info.db_props.inputAttachmentDescriptorSize, ZINK_FBFETCH_DESCRIPTOR_SIZE);
      }
   mesa_logw("zink: bug detected: inputAttachmentDescriptorSize(%u) > %u", (unsigned)screen->info.db_props.inputAttachmentDescriptorSize, ZINK_FBFETCH_DESCRIPTOR_SIZE);
      }
   if (screen->compact_descriptors) {
      if (screen->info.db_props.maxDescriptorBufferBindings < 3) {
      if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_DB) {
      mesa_loge("Cannot use db descriptor mode with compact descriptors with maxDescriptorBufferBindings < 3");
      }
         } else {
      if (screen->info.db_props.maxDescriptorBufferBindings < 5) {
      if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_DB) {
      mesa_loge("Cannot use db descriptor mode with maxDescriptorBufferBindings < 5");
      }
            }
   if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_AUTO) {
      /* descriptor buffer is not performant with virt yet */
   if (screen->info.driver_props.driverID == VK_DRIVER_ID_MESA_VENUS)
         else
      }
   if (zink_descriptor_mode == ZINK_DESCRIPTOR_MODE_DB) {
      const uint32_t sampler_size = MAX2(screen->info.db_props.combinedImageSamplerDescriptorSize, screen->info.db_props.robustUniformTexelBufferDescriptorSize);
   const uint32_t image_size = MAX2(screen->info.db_props.storageImageDescriptorSize, screen->info.db_props.robustStorageTexelBufferDescriptorSize);
   if (screen->compact_descriptors) {
      screen->db_size[ZINK_DESCRIPTOR_TYPE_UBO] = screen->info.db_props.robustUniformBufferDescriptorSize +
            } else {
      screen->db_size[ZINK_DESCRIPTOR_TYPE_UBO] = screen->info.db_props.robustUniformBufferDescriptorSize;
   screen->db_size[ZINK_DESCRIPTOR_TYPE_SAMPLER_VIEW] = sampler_size;
   screen->db_size[ZINK_DESCRIPTOR_TYPE_SSBO] = screen->info.db_props.robustStorageBufferDescriptorSize;
      }
   screen->db_size[ZINK_DESCRIPTOR_TYPE_UNIFORMS] = screen->info.db_props.robustUniformBufferDescriptorSize;
   screen->info.have_KHR_push_descriptor = false;
               simple_mtx_init(&screen->free_batch_states_lock, mtx_plain);
                     simple_mtx_init(&screen->semaphores_lock, mtx_plain);
   util_dynarray_init(&screen->semaphores, screen);
            util_vertex_state_cache_init(&screen->vertex_state_cache,
         screen->base.create_vertex_state = zink_cache_create_vertex_state;
                              if (!init_layouts(screen)) {
      mesa_loge("ZINK: failed to initialize layouts");
               if (!zink_descriptor_layouts_init(screen)) {
      mesa_loge("ZINK: failed to initialize descriptor layouts");
                                 screen->screen_id = p_atomic_inc_return(&num_screens);
   zink_tracing = screen->instance_info.have_EXT_debug_utils &&
                           fail:
      zink_destroy_screen(&screen->base);
      }
      struct pipe_screen *
   zink_create_screen(struct sw_winsys *winsys, const struct pipe_screen_config *config)
   {
      struct zink_screen *ret = zink_internal_create_screen(config, -1, -1);
   if (ret) {
                     }
      static inline int
   zink_render_rdev(int fd, int64_t *dev_major, int64_t *dev_minor)
   {
      int ret = 0;
      #ifdef HAVE_LIBDRM
      struct stat stx;
            if (fd == -1)
            if (drmGetDevice2(fd, 0, &dev))
            if(!(dev->available_nodes & (1 << DRM_NODE_RENDER))) {
      ret = -1;
               if(stat(dev->nodes[DRM_NODE_RENDER], &stx)) {
      ret = -1;
               *dev_major = major(stx.st_rdev);
         free_device:
         #endif //HAVE_LIBDRM
            }
      struct pipe_screen *
   zink_drm_create_screen(int fd, const struct pipe_screen_config *config)
   {
      int64_t dev_major, dev_minor;
            if (zink_render_rdev(fd, &dev_major, &dev_minor))
                     if (ret)
         if (ret && !ret->info.have_KHR_external_memory_fd) {
      debug_printf("ZINK: KHR_external_memory_fd required!\n");
   zink_destroy_screen(&ret->base);
                  }
      void zink_stub_function_not_loaded()
   {
      /* this will be used by the zink_verify_*_extensions() functions on a
   * release build
   */
   mesa_loge("ZINK: a Vulkan function was called without being loaded");
      }
      bool
   zink_screen_debug_marker_begin(struct zink_screen *screen, const char *fmt, ...)
   {
      if (!zink_tracing)
            char *name;
   va_list va;
   va_start(va, fmt);
   int ret = vasprintf(&name, fmt, va);
            if (ret == -1)
            VkDebugUtilsLabelEXT info = { 0 };
   info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
                     free(name);
      }
      void
   zink_screen_debug_marker_end(struct zink_screen *screen, bool emitted)
   {
      if (emitted)
      }
