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
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * @file iris_screen.c
   *
   * Screen related driver hooks and capability lists.
   *
   * A program may use multiple rendering contexts (iris_context), but
   * they all share a common screen (iris_screen).  Global driver state
   * can be stored in the screen; it may be accessed by multiple threads.
   */
      #include <stdio.h>
   #include <errno.h>
   #include <sys/ioctl.h>
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/u_debug.h"
   #include "util/os_file.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_transfer_helper.h"
   #include "util/u_upload_mgr.h"
   #include "util/ralloc.h"
   #include "util/xmlconfig.h"
   #include "drm-uapi/i915_drm.h"
   #include "iris_context.h"
   #include "iris_defines.h"
   #include "iris_fence.h"
   #include "iris_pipe.h"
   #include "iris_resource.h"
   #include "iris_screen.h"
   #include "compiler/glsl_types.h"
   #include "intel/compiler/brw_compiler.h"
   #include "intel/common/intel_gem.h"
   #include "intel/common/intel_l3_config.h"
   #include "intel/common/intel_uuid.h"
   #include "iris_monitor.h"
      #define genX_call(devinfo, func, ...)             \
      switch ((devinfo)->verx10) {                   \
   case 200:                                      \
      gfx20_##func(__VA_ARGS__);                  \
      case 125:                                      \
      gfx125_##func(__VA_ARGS__);                 \
      case 120:                                      \
      gfx12_##func(__VA_ARGS__);                  \
      case 110:                                      \
      gfx11_##func(__VA_ARGS__);                  \
      case 90:                                       \
      gfx9_##func(__VA_ARGS__);                   \
      case 80:                                       \
      gfx8_##func(__VA_ARGS__);                   \
      default:                                       \
               static const char *
   iris_get_vendor(struct pipe_screen *pscreen)
   {
         }
      static const char *
   iris_get_device_vendor(struct pipe_screen *pscreen)
   {
         }
      static void
   iris_get_device_uuid(struct pipe_screen *pscreen, char *uuid)
   {
                  }
      static void
   iris_get_driver_uuid(struct pipe_screen *pscreen, char *uuid)
   {
      struct iris_screen *screen = (struct iris_screen *)pscreen;
               }
      static bool
   iris_enable_clover()
   {
      static int enable = -1;
   if (enable < 0)
            }
      static void
   iris_warn_cl()
   {
      static bool warned = false;
   if (warned)
            warned = true;
   fprintf(stderr, "WARNING: OpenCL support via iris driver is incomplete.\n"
            }
      static const char *
   iris_get_name(struct pipe_screen *pscreen)
   {
      struct iris_screen *screen = (struct iris_screen *)pscreen;
   const struct intel_device_info *devinfo = screen->devinfo;
            snprintf(buf, sizeof(buf), "Mesa %s", devinfo->name);
      }
      static const char *
   iris_get_cl_cts_version(struct pipe_screen *pscreen)
   {
      struct iris_screen *screen = (struct iris_screen *)pscreen;
            /* https://www.khronos.org/conformance/adopters/conformant-products/opencl#submission_405 */
   if (devinfo->verx10 == 120)
               }
      static int
   iris_get_video_memory(struct iris_screen *screen)
   {
      uint64_t vram = iris_bufmgr_vram_size(screen->bufmgr);
   uint64_t sram = iris_bufmgr_sram_size(screen->bufmgr);
   if (vram) {
         } else if (sram) {
         } else {
      /* This is the old code path, it get the GGTT size from the kernel
   * (which should always be 4Gb on Gfx8+).
   *
   * We should probably never end up here. This is just a fallback to get
   * some kind of value in case os_get_available_system_memory fails.
   */
   const struct intel_device_info *devinfo = screen->devinfo;
   /* Once a batch uses more than 75% of the maximum mappable size, we
   * assume that there's some fragmentation, and we start doing extra
   * flushing, etc.  That's the big cliff apps will care about.
   */
   const unsigned gpu_mappable_megabytes =
            const long system_memory_pages = sysconf(_SC_PHYS_PAGES);
            if (system_memory_pages <= 0 || system_page_size <= 0)
            const uint64_t system_memory_bytes =
            const unsigned system_memory_megabytes =
                  }
      static int
   iris_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
   {
      struct iris_screen *screen = (struct iris_screen *)pscreen;
            switch (param) {
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_QUERY_TIME_ELAPSED:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_INDEP_BLEND_FUNC:
   case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_INTEGER:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_TEXTURE_BARRIER:
   case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_COMPUTE:
   case PIPE_CAP_START_INSTANCE:
   case PIPE_CAP_QUERY_TIMESTAMP:
   case PIPE_CAP_TEXTURE_MULTISAMPLE:
   case PIPE_CAP_CUBE_MAP_ARRAY:
   case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
   case PIPE_CAP_QUERY_PIPELINE_STATISTICS_SINGLE:
   case PIPE_CAP_TEXTURE_QUERY_LOD:
   case PIPE_CAP_SAMPLE_SHADING:
   case PIPE_CAP_FORCE_PERSAMPLE_INTERP:
   case PIPE_CAP_DRAW_INDIRECT:
   case PIPE_CAP_MULTI_DRAW_INDIRECT:
   case PIPE_CAP_MULTI_DRAW_INDIRECT_PARAMS:
   case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_VS_LAYER_VIEWPORT:
   case PIPE_CAP_TES_LAYER_VIEWPORT:
   case PIPE_CAP_FS_FINE_DERIVATIVE:
   case PIPE_CAP_SHADER_PACK_HALF_FLOAT:
   case PIPE_CAP_ACCELERATED:
   case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
   case PIPE_CAP_CLIP_HALFZ:
   case PIPE_CAP_TGSI_TEXCOORD:
   case PIPE_CAP_STREAM_OUTPUT_INTERLEAVE_BUFFERS:
   case PIPE_CAP_DOUBLES:
   case PIPE_CAP_INT64:
   case PIPE_CAP_SAMPLER_VIEW_TARGET:
   case PIPE_CAP_ROBUST_BUFFER_ACCESS_BEHAVIOR:
   case PIPE_CAP_DEVICE_RESET_STATUS_QUERY:
   case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
   case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
   case PIPE_CAP_CULL_DISTANCE:
   case PIPE_CAP_PACKED_UNIFORMS:
   case PIPE_CAP_SIGNED_VERTEX_BUFFER_OFFSET:
   case PIPE_CAP_TEXTURE_FLOAT_LINEAR:
   case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
   case PIPE_CAP_POLYGON_OFFSET_CLAMP:
   case PIPE_CAP_QUERY_SO_OVERFLOW:
   case PIPE_CAP_QUERY_BUFFER_OBJECT:
   case PIPE_CAP_TGSI_TEX_TXF_LZ:
   case PIPE_CAP_TEXTURE_QUERY_SAMPLES:
   case PIPE_CAP_SHADER_CLOCK:
   case PIPE_CAP_SHADER_BALLOT:
   case PIPE_CAP_MULTISAMPLE_Z_RESOLVE:
   case PIPE_CAP_CLEAR_SCISSORED:
   case PIPE_CAP_SHADER_GROUP_VOTE:
   case PIPE_CAP_VS_WINDOW_SPACE_POSITION:
   case PIPE_CAP_TEXTURE_GATHER_SM5:
   case PIPE_CAP_SHADER_ARRAY_COMPONENTS:
   case PIPE_CAP_GLSL_TESS_LEVELS_AS_INPUTS:
   case PIPE_CAP_LOAD_CONSTBUF:
   case PIPE_CAP_NIR_COMPACT_ARRAYS:
   case PIPE_CAP_DRAW_PARAMETERS:
   case PIPE_CAP_FS_POSITION_IS_SYSVAL:
   case PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL:
   case PIPE_CAP_COMPUTE_SHADER_DERIVATIVES:
   case PIPE_CAP_INVALIDATE_BUFFER:
   case PIPE_CAP_SURFACE_REINTERPRET_BLOCKS:
   case PIPE_CAP_TEXTURE_SHADOW_LOD:
   case PIPE_CAP_SHADER_SAMPLES_IDENTICAL:
   case PIPE_CAP_GL_SPIRV:
   case PIPE_CAP_GL_SPIRV_VARIABLE_POINTERS:
   case PIPE_CAP_DEMOTE_TO_HELPER_INVOCATION:
   case PIPE_CAP_NATIVE_FENCE_FD:
   case PIPE_CAP_MEMOBJ:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
   case PIPE_CAP_FENCE_SIGNAL:
   case PIPE_CAP_IMAGE_STORE_FORMATTED:
   case PIPE_CAP_LEGACY_MATH_RULES:
   case PIPE_CAP_ALPHA_TO_COVERAGE_DITHER_CONTROL:
   case PIPE_CAP_MAP_UNSYNCHRONIZED_THREAD_SAFE:
   case PIPE_CAP_HAS_CONST_BW:
         case PIPE_CAP_UMA:
         case PIPE_CAP_PREFER_BACK_BUFFER_REUSE:
         case PIPE_CAP_FBFETCH:
         case PIPE_CAP_FBFETCH_COHERENT:
   case PIPE_CAP_CONSERVATIVE_RASTER_INNER_COVERAGE:
   case PIPE_CAP_POST_DEPTH_COVERAGE:
   case PIPE_CAP_SHADER_STENCIL_EXPORT:
   case PIPE_CAP_DEPTH_CLIP_DISABLE_SEPARATE:
   case PIPE_CAP_FRAGMENT_SHADER_INTERLOCK:
   case PIPE_CAP_ATOMIC_FLOAT_MINMAX:
         case PIPE_CAP_DEPTH_BOUNDS_TEST:
         case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
         case PIPE_CAP_MAX_RENDER_TARGETS:
         case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
         case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
         case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
         case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
         case PIPE_CAP_GLSL_FEATURE_LEVEL:
   case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
         case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
      /* 3DSTATE_CONSTANT_XS requires the start of UBOs to be 32B aligned */
      case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
         case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT:
         case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_LINEAR_IMAGE_PITCH_ALIGNMENT:
         case PIPE_CAP_LINEAR_IMAGE_BASE_ADDRESS_ALIGNMENT:
         case PIPE_CAP_TEXTURE_TRANSFER_MODES:
         case PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT:
         case PIPE_CAP_MAX_VIEWPORTS:
         case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
         case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
         case PIPE_CAP_MAX_GS_INVOCATIONS:
         case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
         case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_MAX_VERTEX_STREAMS:
         case PIPE_CAP_VENDOR_ID:
         case PIPE_CAP_DEVICE_ID:
         case PIPE_CAP_VIDEO_MEMORY:
         case PIPE_CAP_MAX_SHADER_PATCH_VARYINGS:
   case PIPE_CAP_MAX_VARYINGS:
         case PIPE_CAP_PREFER_IMM_ARRAYS_AS_CONSTBUF:
      /* We want immediate arrays to go get uploaded as nir->constant_data by
   * nir_opt_large_constants() instead.
   */
      case PIPE_CAP_RESOURCE_FROM_USER_MEMORY:
      /* AMD_pinned_memory assumes the flexibility of using client memory
   * for any buffer (incl. vertex buffers) which rules out the prospect
   * of using snooped buffers, as using snooped buffers without
   * cogniscience is likely to be detrimental to performance and require
   * extensive checking in the driver for correctness, e.g. to prevent
   * illegal snoop <-> snoop transfers.
   */
      case PIPE_CAP_THROTTLE:
            case PIPE_CAP_CONTEXT_PRIORITY_MASK:
      return PIPE_CONTEXT_PRIORITY_LOW |
               case PIPE_CAP_FRONTEND_NOOP:
            // XXX: don't hardcode 00:00:02.0 PCI here
   case PIPE_CAP_PCI_GROUP:
         case PIPE_CAP_PCI_BUS:
         case PIPE_CAP_PCI_DEVICE:
         case PIPE_CAP_PCI_FUNCTION:
            case PIPE_CAP_OPENCL_INTEGER_FUNCTIONS:
   case PIPE_CAP_INTEGER_MULTIPLY_32X16:
            case PIPE_CAP_ALLOW_DYNAMIC_VAO_FASTPATH:
      /* Internal details of VF cache make this optimization harmful on GFX
   * version 8 and 9, because generated VERTEX_BUFFER_STATEs are cached
   * separately.
   */
         case PIPE_CAP_QUERY_TIMESTAMP_BITS:
            case PIPE_CAP_TIMER_RESOLUTION:
            case PIPE_CAP_DEVICE_PROTECTED_CONTEXT:
            case PIPE_CAP_ASTC_VOID_EXTENTS_NEED_DENORM_FLUSH:
            default:
         }
      }
      static float
   iris_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
   {
      switch (param) {
   case PIPE_CAPF_MIN_LINE_WIDTH:
   case PIPE_CAPF_MIN_LINE_WIDTH_AA:
   case PIPE_CAPF_MIN_POINT_SIZE:
   case PIPE_CAPF_MIN_POINT_SIZE_AA:
            case PIPE_CAPF_POINT_SIZE_GRANULARITY:
   case PIPE_CAPF_LINE_WIDTH_GRANULARITY:
            case PIPE_CAPF_MAX_LINE_WIDTH:
   case PIPE_CAPF_MAX_LINE_WIDTH_AA:
            case PIPE_CAPF_MAX_POINT_SIZE:
   case PIPE_CAPF_MAX_POINT_SIZE_AA:
            case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
         case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
         case PIPE_CAPF_MIN_CONSERVATIVE_RASTER_DILATE:
   case PIPE_CAPF_MAX_CONSERVATIVE_RASTER_DILATE:
   case PIPE_CAPF_CONSERVATIVE_RASTER_DILATE_GRANULARITY:
         default:
            }
      static int
   iris_get_shader_param(struct pipe_screen *pscreen,
               {
               if (p_stage == PIPE_SHADER_MESH ||
      p_stage == PIPE_SHADER_TASK)
         /* this is probably not totally correct.. but it's a start: */
   switch (param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
         case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
            case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
            case PIPE_SHADER_CAP_MAX_INPUTS:
         case PIPE_SHADER_CAP_MAX_OUTPUTS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         case PIPE_SHADER_CAP_MAX_TEMPS:
         case PIPE_SHADER_CAP_CONT_SUPPORTED:
         case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
      /* Lie about these to avoid st/mesa's GLSL IR lowering of indirects,
   * which we don't want.  Our compiler backend will check brw_compiler's
   * options and call nir_lower_indirect_derefs appropriately anyway.
   */
      case PIPE_SHADER_CAP_SUBROUTINES:
         case PIPE_SHADER_CAP_INTEGERS:
         case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
         case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
         case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
         case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
         case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         case PIPE_SHADER_CAP_SUPPORTED_IRS: {
      int irs = 1 << PIPE_SHADER_IR_NIR;
   if (iris_enable_clover())
            }
   case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
   case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
         default:
            }
      static int
   iris_get_compute_param(struct pipe_screen *pscreen,
                     {
      struct iris_screen *screen = (struct iris_screen *)pscreen;
            const uint32_t max_invocations =
         #define RET(x) do {                  \
      if (ret)                          \
            } while (0)
         switch (param) {
   case PIPE_COMPUTE_CAP_ADDRESS_BITS:
      /* This gets queried on OpenCL device init and is never queried by the
   * OpenGL state tracker.
   */
   iris_warn_cl();
         case PIPE_COMPUTE_CAP_IR_TARGET:
      if (ret)
               case PIPE_COMPUTE_CAP_GRID_DIMENSION:
            case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
            case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
      /* MaxComputeWorkGroupSize[0..2] */
         case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
         case PIPE_COMPUTE_CAP_MAX_VARIABLE_THREADS_PER_BLOCK:
      /* MaxComputeVariableGroupInvocations */
         case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE:
      /* MaxComputeSharedMemorySize */
         case PIPE_COMPUTE_CAP_IMAGES_SUPPORTED:
            case PIPE_COMPUTE_CAP_SUBGROUP_SIZES:
            case PIPE_COMPUTE_CAP_MAX_SUBGROUPS:
            case PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE:
   case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE:
            case PIPE_COMPUTE_CAP_MAX_CLOCK_FREQUENCY:
            case PIPE_COMPUTE_CAP_MAX_COMPUTE_UNITS: {
                  case PIPE_COMPUTE_CAP_MAX_PRIVATE_SIZE:
      /* MaxComputeSharedMemorySize */
         case PIPE_COMPUTE_CAP_MAX_INPUT_SIZE:
      /* We could probably allow more; this is the OpenCL minimum */
         default:
            }
      static uint64_t
   iris_get_timestamp(struct pipe_screen *pscreen)
   {
      struct iris_screen *screen = (struct iris_screen *) pscreen;
            if (!intel_gem_read_render_timestamp(iris_bufmgr_get_fd(screen->bufmgr),
                  result = intel_device_info_timebase_scale(screen->devinfo, result);
               }
      void
   iris_screen_destroy(struct iris_screen *screen)
   {
      iris_destroy_screen_measure(screen);
   util_queue_destroy(&screen->shader_compiler_queue);
   glsl_type_singleton_decref();
   iris_bo_unreference(screen->workaround_bo);
   iris_bo_unreference(screen->breakpoint_bo);
   u_transfer_helper_destroy(screen->base.transfer_helper);
   iris_bufmgr_unref(screen->bufmgr);
   disk_cache_destroy(screen->disk_cache);
   close(screen->winsys_fd);
      }
      static void
   iris_screen_unref(struct pipe_screen *pscreen)
   {
         }
      static void
   iris_query_memory_info(struct pipe_screen *pscreen,
         {
   }
      static const void *
   iris_get_compiler_options(struct pipe_screen *pscreen,
               {
      struct iris_screen *screen = (struct iris_screen *) pscreen;
   gl_shader_stage stage = stage_from_pipe(pstage);
               }
      static struct disk_cache *
   iris_get_disk_shader_cache(struct pipe_screen *pscreen)
   {
      struct iris_screen *screen = (struct iris_screen *) pscreen;
      }
      static const struct intel_l3_config *
   iris_get_default_l3_config(const struct intel_device_info *devinfo,
         {
      bool wants_dc_cache = true;
   bool has_slm = compute;
   const struct intel_l3_weights w =
            }
      static void
   iris_shader_debug_log(void *data, unsigned *id, const char *fmt, ...)
   {
      struct util_debug_callback *dbg = data;
            if (!dbg->debug_message)
            va_start(args, fmt);
   dbg->debug_message(dbg->data, id, UTIL_DEBUG_TYPE_SHADER_INFO, fmt, args);
      }
      static void
   iris_shader_perf_log(void *data, unsigned *id, const char *fmt, ...)
   {
      struct util_debug_callback *dbg = data;
   va_list args;
            if (INTEL_DEBUG(DEBUG_PERF)) {
      va_list args_copy;
   va_copy(args_copy, args);
   vfprintf(stderr, fmt, args_copy);
               if (dbg->debug_message) {
                     }
      static void
   iris_detect_kernel_features(struct iris_screen *screen)
   {
      const struct intel_device_info *devinfo = screen->devinfo;
   /* Kernel 5.2+ */
   if (intel_gem_supports_syncobj_wait(screen->fd))
         if (intel_gem_supports_protected_context(screen->fd, devinfo->kmd_type))
      }
      static bool
   iris_init_identifier_bo(struct iris_screen *screen)
   {
               bo_map = iris_bo_map(NULL, screen->workaround_bo, MAP_READ | MAP_WRITE);
   if (!bo_map)
                     screen->workaround_bo->real.kflags |=
         screen->workaround_address = (struct iris_address) {
      .bo = screen->workaround_bo,
   .offset = ALIGN(
                           }
      static int
   iris_screen_get_fd(struct pipe_screen *pscreen)
   {
                  }
      struct pipe_screen *
   iris_screen_create(int fd, const struct pipe_screen_config *config)
   {
      struct iris_screen *screen = rzalloc(NULL, struct iris_screen);
   if (!screen)
            driParseConfigFiles(config->options, config->options_info, 0, "iris",
            bool bo_reuse = false;
   int bo_reuse_mode = driQueryOptioni(config->options, "bo_reuse");
   switch (bo_reuse_mode) {
   case DRI_CONF_BO_REUSE_DISABLED:
         case DRI_CONF_BO_REUSE_ALL:
      bo_reuse = true;
                        screen->bufmgr = iris_bufmgr_get_for_fd(fd, bo_reuse);
   if (!screen->bufmgr)
            screen->devinfo = iris_bufmgr_get_device_info(screen->bufmgr);
            /* Here are the i915 features we need for Iris (in chronological order) :
   *    - I915_PARAM_HAS_EXEC_NO_RELOC     (3.10)
   *    - I915_PARAM_HAS_EXEC_HANDLE_LUT   (3.10)
   *    - I915_PARAM_HAS_EXEC_BATCH_FIRST  (4.13)
   *    - I915_PARAM_HAS_EXEC_FENCE_ARRAY  (4.14)
   *    - I915_PARAM_HAS_CONTEXT_ISOLATION (4.16)
   *
   * Checking the last feature availability will include all previous ones.
   */
   if (!screen->devinfo->has_context_isolation) {
      debug_error("Kernel is too old (4.16+ required) or unusable for Iris.\n"
                     screen->fd = iris_bufmgr_get_fd(screen->bufmgr);
                     screen->workaround_bo =
      iris_bo_alloc(screen->bufmgr, "workaround", 4096, 4096,
      if (!screen->workaround_bo)
            screen->breakpoint_bo = iris_bo_alloc(screen->bufmgr, "breakpoint", 4, 4,
         if (!screen->breakpoint_bo)
            if (!iris_init_identifier_bo(screen))
            screen->driconf.dual_color_blend_by_location =
         screen->driconf.disable_throttling =
         screen->driconf.always_flush_cache = INTEL_DEBUG(DEBUG_STALL) ||
         screen->driconf.sync_compile =
         screen->driconf.limit_trig_input_range =
         screen->driconf.lower_depth_range_rate =
         screen->driconf.intel_enable_wa_14018912822 =
                              screen->compiler = brw_compiler_create(screen, screen->devinfo);
   screen->compiler->shader_debug_log = iris_shader_debug_log;
   screen->compiler->shader_perf_log = iris_shader_perf_log;
   screen->compiler->supports_shader_constants = true;
            screen->l3_config_3d = iris_get_default_l3_config(screen->devinfo, false);
                     slab_create_parent(&screen->transfer_pool,
                              iris_init_screen_fence_functions(pscreen);
   iris_init_screen_resource_functions(pscreen);
            pscreen->destroy = iris_screen_unref;
   pscreen->get_name = iris_get_name;
   pscreen->get_vendor = iris_get_vendor;
   pscreen->get_device_vendor = iris_get_device_vendor;
   pscreen->get_cl_cts_version = iris_get_cl_cts_version;
   pscreen->get_screen_fd = iris_screen_get_fd;
   pscreen->get_param = iris_get_param;
   pscreen->get_shader_param = iris_get_shader_param;
   pscreen->get_compute_param = iris_get_compute_param;
   pscreen->get_paramf = iris_get_paramf;
   pscreen->get_compiler_options = iris_get_compiler_options;
   pscreen->get_device_uuid = iris_get_device_uuid;
   pscreen->get_driver_uuid = iris_get_driver_uuid;
   pscreen->get_disk_shader_cache = iris_get_disk_shader_cache;
   pscreen->is_format_supported = iris_is_format_supported;
   pscreen->context_create = iris_create_context;
   pscreen->get_timestamp = iris_get_timestamp;
   pscreen->query_memory_info = iris_query_memory_info;
   pscreen->get_driver_query_group_info = iris_get_monitor_group_info;
   pscreen->get_driver_query_info = iris_get_monitor_info;
                                       /* FINISHME: Big core vs little core (for CPUs that have both kinds of
   * cores) and, possibly, thread vs core should be considered here too.
   */
   unsigned compiler_threads = 1;
   const struct util_cpu_caps_t *caps = util_get_cpu_caps();
            if (hw_threads >= 12) {
         } else if (hw_threads >= 6) {
         } else if (hw_threads >= 2) {
                  if (!util_queue_init(&screen->shader_compiler_queue,
                        "sh", 64, compiler_threads,
   iris_screen_destroy(screen);
                  }
