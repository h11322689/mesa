   /*
   * Copyright © 2018 Broadcom
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
      #include <sys/stat.h>
      #include "pipe/p_screen.h"
   #include "util/u_screen.h"
   #include "util/u_debug.h"
   #include "util/os_file.h"
   #include "util/os_time.h"
   #include "util/simple_mtx.h"
   #include "util/u_hash_table.h"
   #include "util/u_pointer.h"
   #include "util/macros.h"
      #ifdef HAVE_LIBDRM
   #include <xf86drm.h>
   #endif
      /**
   * Helper to use from a pipe_screen->get_param() implementation to return
   * default values for unsupported PIPE_CAPs.
   *
   * Call this function from your pipe_screen->get_param() implementation's
   * default case, so that implementors of new pipe caps don't need to
   */
   int
   u_pipe_screen_get_param_defaults(struct pipe_screen *pscreen,
         {
      UNUSED uint64_t cap;
                     /* Let's keep these sorted by position in p_defines.h. */
   switch (param) {
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
   case PIPE_CAP_ANISOTROPIC_FILTER:
            case PIPE_CAP_GRAPHICS:
   case PIPE_CAP_GL_CLAMP:
   case PIPE_CAP_MAX_RENDER_TARGETS:
   case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
   case PIPE_CAP_DITHERING:
            case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_QUERY_TIME_ELAPSED:
   case PIPE_CAP_TEXTURE_SWIZZLE:
            case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
   case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS: /* enables EXT_transform_feedback */
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_INDEP_BLEND_FUNC:
   case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS: /* Enables GL_EXT_texture_array */
   case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_INTEGER:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_DEPTH_CLIP_DISABLE_SEPARATE:
   case PIPE_CAP_DEPTH_CLAMP_ENABLE:
   case PIPE_CAP_SHADER_STENCIL_EXPORT:
   case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
            case PIPE_CAP_SUPPORTED_PRIM_MODES_WITH_RESTART:
   case PIPE_CAP_SUPPORTED_PRIM_MODES:
            case PIPE_CAP_MIN_TEXEL_OFFSET:
      /* GL 3.x minimum value. */
      case PIPE_CAP_MAX_TEXEL_OFFSET:
            case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_TEXTURE_BARRIER:
            case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
      /* GL_EXT_transform_feedback minimum value. */
      case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
            case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_VERTEX_COLOR_CLAMPED:
            case PIPE_CAP_GLSL_FEATURE_LEVEL:
   case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
      /* Minimum GLSL level implemented by gallium drivers. */
         case PIPE_CAP_ESSL_FEATURE_LEVEL:
      /* Tell gallium frontend to fallback to PIPE_CAP_GLSL_FEATURE_LEVEL */
         case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
   case PIPE_CAP_USER_VERTEX_BUFFERS:
   case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_ATTRIB_ELEMENT_ALIGNED_ONLY:
   case PIPE_CAP_COMPUTE:
            case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
      /* GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT default value. */
         case PIPE_CAP_START_INSTANCE:
   case PIPE_CAP_QUERY_TIMESTAMP:
   case PIPE_CAP_TIMER_RESOLUTION:
   case PIPE_CAP_TEXTURE_MULTISAMPLE:
            case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
      /* GL_ARB_map_buffer_alignment minimum value. All drivers expose the
   * extension.
   */
         case PIPE_CAP_CUBE_MAP_ARRAY:
   case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
            case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
      /* GL_EXT_texture_buffer minimum value. */
         case PIPE_CAP_BUFFER_SAMPLER_VIEW_RGBA_ONLY:
   case PIPE_CAP_TGSI_TEXCOORD:
            case PIPE_CAP_TEXTURE_TRANSFER_MODES:
            case PIPE_CAP_QUERY_PIPELINE_STATISTICS:
   case PIPE_CAP_QUERY_PIPELINE_STATISTICS_SINGLE:
   case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
            case PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT:
      /* GL_EXT_texture_buffer minimum value. */
         case PIPE_CAP_LINEAR_IMAGE_PITCH_ALIGNMENT:
            case PIPE_CAP_LINEAR_IMAGE_BASE_ADDRESS_ALIGNMENT:
            case PIPE_CAP_MAX_VIEWPORTS:
            case PIPE_CAP_ENDIANNESS:
            case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_VS_LAYER_VIEWPORT:
   case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
   case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
   case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS: /* Enables ARB_texture_gather */
   case PIPE_CAP_TEXTURE_GATHER_SM5:
            /* All new drivers should support persistent/coherent mappings. This CAP
   * should only be unset by layered drivers whose host drivers cannot support
   * coherent mappings.
   */
   case PIPE_CAP_BUFFER_MAP_PERSISTENT_COHERENT:
            case PIPE_CAP_FAKE_SW_MSAA:
   case PIPE_CAP_TEXTURE_QUERY_LOD:
            case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
            case PIPE_CAP_SAMPLE_SHADING:
   case PIPE_CAP_TEXTURE_GATHER_OFFSETS:
   case PIPE_CAP_VS_WINDOW_SPACE_POSITION:
   case PIPE_CAP_MAX_VERTEX_STREAMS:
   case PIPE_CAP_DRAW_INDIRECT:
   case PIPE_CAP_FS_FINE_DERIVATIVE:
            case PIPE_CAP_VENDOR_ID:
   case PIPE_CAP_DEVICE_ID:
            case PIPE_CAP_ACCELERATED:
   case PIPE_CAP_VIDEO_MEMORY:
   case PIPE_CAP_UMA:
            case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
            case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
      /* GL minimum value */
         case PIPE_CAP_SAMPLER_VIEW_TARGET:
   case PIPE_CAP_CLIP_HALFZ:
   case PIPE_CAP_POLYGON_OFFSET_CLAMP:
   case PIPE_CAP_MULTISAMPLE_Z_RESOLVE:
   case PIPE_CAP_RESOURCE_FROM_USER_MEMORY:
   case PIPE_CAP_RESOURCE_FROM_USER_MEMORY_COMPUTE_ONLY:
   case PIPE_CAP_DEVICE_RESET_STATUS_QUERY:
   case PIPE_CAP_DEVICE_PROTECTED_SURFACE:
   case PIPE_CAP_DEVICE_PROTECTED_CONTEXT:
   case PIPE_CAP_MAX_SHADER_PATCH_VARYINGS:
   case PIPE_CAP_TEXTURE_FLOAT_LINEAR:
   case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
   case PIPE_CAP_DEPTH_BOUNDS_TEST:
   case PIPE_CAP_TEXTURE_QUERY_SAMPLES:
   case PIPE_CAP_FORCE_PERSAMPLE_INTERP:
            /* All drivers should expose this cap, as it is required for applications to
   * be able to efficiently compile GL shaders from multiple threads during
   * load.
   */
   case PIPE_CAP_SHAREABLE_SHADERS:
            case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
   case PIPE_CAP_CLEAR_SCISSORED:
   case PIPE_CAP_DRAW_PARAMETERS:
   case PIPE_CAP_SHADER_PACK_HALF_FLOAT:
   case PIPE_CAP_MULTI_DRAW_INDIRECT:
   case PIPE_CAP_MULTI_DRAW_INDIRECT_PARAMS:
   case PIPE_CAP_FS_POSITION_IS_SYSVAL:
   case PIPE_CAP_FS_POINT_IS_SYSVAL:
   case PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL:
         case PIPE_CAP_MULTI_DRAW_INDIRECT_PARTIAL_STRIDE:
            case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
      /* Enables GL_ARB_shader_storage_buffer_object */
         case PIPE_CAP_INVALIDATE_BUFFER:
   case PIPE_CAP_GENERATE_MIPMAP:
   case PIPE_CAP_STRING_MARKER:
   case PIPE_CAP_SURFACE_REINTERPRET_BLOCKS:
   case PIPE_CAP_QUERY_BUFFER_OBJECT:
   case PIPE_CAP_QUERY_MEMORY_INFO: /* Enables GL_ATI_meminfo */
            case PIPE_CAP_PCI_GROUP:
   case PIPE_CAP_PCI_BUS:
   case PIPE_CAP_PCI_DEVICE:
   case PIPE_CAP_PCI_FUNCTION:
            case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
   case PIPE_CAP_ROBUST_BUFFER_ACCESS_BEHAVIOR:
   case PIPE_CAP_CULL_DISTANCE:
   case PIPE_CAP_CULL_DISTANCE_NOCOMBINE:
   case PIPE_CAP_SHADER_GROUP_VOTE:
   case PIPE_CAP_MAX_WINDOW_RECTANGLES: /* Enables EXT_window_rectangles */
   case PIPE_CAP_POLYGON_OFFSET_UNITS_UNSCALED:
   case PIPE_CAP_VIEWPORT_SUBPIXEL_BITS:
   case PIPE_CAP_VIEWPORT_SWIZZLE:
   case PIPE_CAP_VIEWPORT_MASK:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
   case PIPE_CAP_SHADER_ARRAY_COMPONENTS:
   case PIPE_CAP_STREAM_OUTPUT_INTERLEAVE_BUFFERS:
   case PIPE_CAP_SHADER_CAN_READ_OUTPUTS:
   case PIPE_CAP_NATIVE_FENCE_FD:
            case PIPE_CAP_RASTERIZER_SUBPIXEL_BITS:
            case PIPE_CAP_PREFER_BACK_BUFFER_REUSE:
            case PIPE_CAP_GLSL_TESS_LEVELS_AS_INPUTS:
            case PIPE_CAP_FBFETCH:
   case PIPE_CAP_FBFETCH_COHERENT:
   case PIPE_CAP_FBFETCH_ZS:
   case PIPE_CAP_BLEND_EQUATION_ADVANCED:
   case PIPE_CAP_LEGACY_MATH_RULES:
   case PIPE_CAP_DOUBLES:
   case PIPE_CAP_INT64:
   case PIPE_CAP_TGSI_TEX_TXF_LZ:
   case PIPE_CAP_SHADER_CLOCK:
   case PIPE_CAP_POLYGON_MODE_FILL_RECTANGLE:
   case PIPE_CAP_SPARSE_BUFFER_PAGE_SIZE:
   case PIPE_CAP_SHADER_BALLOT:
   case PIPE_CAP_TES_LAYER_VIEWPORT:
   case PIPE_CAP_CAN_BIND_CONST_BUFFER_AS_VERTEX:
   case PIPE_CAP_TGSI_DIV:
   case PIPE_CAP_NIR_ATOMICS_AS_DEREF:
            case PIPE_CAP_ALLOW_MAPPED_BUFFERS_DURING_EXECUTION:
      /* Drivers generally support this, and it reduces GL overhead just to
   * throw an error when buffers are mapped.
   */
         case PIPE_CAP_PREFER_IMM_ARRAYS_AS_CONSTBUF:
      /* Don't unset this unless your driver can do better, like using nir_opt_large_constants() */
         case PIPE_CAP_POST_DEPTH_COVERAGE:
   case PIPE_CAP_BINDLESS_TEXTURE:
   case PIPE_CAP_NIR_SAMPLERS_AS_DEREF:
   case PIPE_CAP_NIR_COMPACT_ARRAYS:
   case PIPE_CAP_QUERY_SO_OVERFLOW:
   case PIPE_CAP_MEMOBJ:
   case PIPE_CAP_LOAD_CONSTBUF:
   case PIPE_CAP_TILE_RASTER_ORDER:
            case PIPE_CAP_MAX_COMBINED_SHADER_OUTPUT_RESOURCES:
      /* nonzero overrides defaults */
         case PIPE_CAP_FRAMEBUFFER_MSAA_CONSTRAINTS:
   case PIPE_CAP_SIGNED_VERTEX_BUFFER_OFFSET:
   case PIPE_CAP_CONTEXT_PRIORITY_MASK:
   case PIPE_CAP_FENCE_SIGNAL:
   case PIPE_CAP_CONSTBUF0_FLAGS:
   case PIPE_CAP_PACKED_UNIFORMS:
   case PIPE_CAP_CONSERVATIVE_RASTER_POST_SNAP_TRIANGLES:
   case PIPE_CAP_CONSERVATIVE_RASTER_POST_SNAP_POINTS_LINES:
   case PIPE_CAP_CONSERVATIVE_RASTER_PRE_SNAP_TRIANGLES:
   case PIPE_CAP_CONSERVATIVE_RASTER_PRE_SNAP_POINTS_LINES:
   case PIPE_CAP_MAX_CONSERVATIVE_RASTER_SUBPIXEL_PRECISION_BIAS:
   case PIPE_CAP_CONSERVATIVE_RASTER_POST_DEPTH_COVERAGE:
   case PIPE_CAP_CONSERVATIVE_RASTER_INNER_COVERAGE:
   case PIPE_CAP_PROGRAMMABLE_SAMPLE_LOCATIONS:
   case PIPE_CAP_MAX_COMBINED_SHADER_BUFFERS:
   case PIPE_CAP_MAX_COMBINED_HW_ATOMIC_COUNTERS:
   case PIPE_CAP_MAX_COMBINED_HW_ATOMIC_COUNTER_BUFFERS:
   case PIPE_CAP_IMAGE_ATOMIC_FLOAT_ADD:
   case PIPE_CAP_IMAGE_LOAD_FORMATTED:
   case PIPE_CAP_IMAGE_STORE_FORMATTED:
   case PIPE_CAP_PREFER_COMPUTE_FOR_MULTIMEDIA:
   case PIPE_CAP_FRAGMENT_SHADER_INTERLOCK:
   case PIPE_CAP_ATOMIC_FLOAT_MINMAX:
   case PIPE_CAP_SHADER_SAMPLES_IDENTICAL:
   case PIPE_CAP_IMAGE_ATOMIC_INC_WRAP:
   case PIPE_CAP_TGSI_TG4_COMPONENT_IN_SWIZZLE:
   case PIPE_CAP_GLSL_ZERO_INIT:
   case PIPE_CAP_ALLOW_DRAW_OUT_OF_ORDER:
            case PIPE_CAP_MAX_GS_INVOCATIONS:
            case PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT:
            case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
   case PIPE_CAP_MAX_TEXTURE_UPLOAD_MEMORY_BUDGET:
            case PIPE_CAP_MAX_VERTEX_ELEMENT_SRC_OFFSET:
            case PIPE_CAP_SURFACE_SAMPLE_COUNT:
         case PIPE_CAP_DEST_SURFACE_SRGB_CONTROL:
            case PIPE_CAP_MAX_VARYINGS:
            case PIPE_CAP_COMPUTE_GRID_INFO_LAST_BLOCK:
            case PIPE_CAP_COMPUTE_SHADER_DERIVATIVES:
            case PIPE_CAP_THROTTLE:
            case PIPE_CAP_TEXTURE_SHADOW_LOD:
            case PIPE_CAP_GL_SPIRV:
   case PIPE_CAP_GL_SPIRV_VARIABLE_POINTERS:
            case PIPE_CAP_DEMOTE_TO_HELPER_INVOCATION:
               #if defined(HAVE_LIBDRM) && (DETECT_OS_LINUX || DETECT_OS_BSD)
         fd = pscreen->get_screen_fd(pscreen);
   if (fd != -1 && (drmGetCap(fd, DRM_CAP_PRIME, &cap) == 0))
         else
   #else
         #endif
         case PIPE_CAP_TEXTURE_SHADOW_MAP: /* Enables ARB_shadow */
            case PIPE_CAP_FLATSHADE:
   case PIPE_CAP_ALPHA_TEST:
   case PIPE_CAP_POINT_SIZE_FIXED:
   case PIPE_CAP_TWO_SIDED_COLOR:
   case PIPE_CAP_CLIP_PLANES:
            case PIPE_CAP_MAX_VERTEX_BUFFERS:
            case PIPE_CAP_OPENCL_INTEGER_FUNCTIONS:
   case PIPE_CAP_INTEGER_MULTIPLY_32X16:
         case PIPE_CAP_NIR_IMAGES_AS_DEREF:
            case PIPE_CAP_FRONTEND_NOOP:
      /* Enables INTEL_blackhole_render */
         case PIPE_CAP_PACKED_STREAM_OUTPUT:
            case PIPE_CAP_VIEWPORT_TRANSFORM_LOWERED:
   case PIPE_CAP_PSIZ_CLAMPED:
   case PIPE_CAP_MAP_UNSYNCHRONIZED_THREAD_SAFE:
            case PIPE_CAP_GL_BEGIN_END_BUFFER_SIZE:
            case PIPE_CAP_SYSTEM_SVM:
   case PIPE_CAP_ALPHA_TO_COVERAGE_DITHER_CONTROL:
   case PIPE_CAP_NO_CLIP_ON_COPY_TEX:
   case PIPE_CAP_MAX_TEXTURE_MB:
   case PIPE_CAP_PREFER_REAL_BUFFER_IN_CONSTBUF0:
            case PIPE_CAP_TEXRECT:
            case PIPE_CAP_SHADER_ATOMIC_INT64:
            case PIPE_CAP_SAMPLER_REDUCTION_MINMAX:
   case PIPE_CAP_SAMPLER_REDUCTION_MINMAX_ARB:
            case PIPE_CAP_ALLOW_DYNAMIC_VAO_FASTPATH:
            case PIPE_CAP_EMULATE_NONFIXED_PRIMITIVE_RESTART:
   case PIPE_CAP_DRAW_VERTEX_STATE:
            case PIPE_CAP_PREFER_POT_ALIGNED_VARYINGS:
            case PIPE_CAP_MAX_SPARSE_TEXTURE_SIZE:
   case PIPE_CAP_MAX_SPARSE_3D_TEXTURE_SIZE:
   case PIPE_CAP_MAX_SPARSE_ARRAY_TEXTURE_LAYERS:
   case PIPE_CAP_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS:
   case PIPE_CAP_QUERY_SPARSE_TEXTURE_RESIDENCY:
   case PIPE_CAP_CLAMP_SPARSE_TEXTURE_LOD:
   case PIPE_CAP_TIMELINE_SEMAPHORE_IMPORT:
   case PIPE_CAP_ALLOW_GLTHREAD_BUFFER_SUBDATA_OPT:
            case PIPE_CAP_MAX_CONSTANT_BUFFER_SIZE_UINT:
      return pscreen->get_shader_param(pscreen, PIPE_SHADER_FRAGMENT,
         case PIPE_CAP_HARDWARE_GL_SELECT: {
      /* =0: on CPU, always disabled
   * >0: on GPU, enable by default, user can disable it manually
   * <0: unknown, disable by default, user can enable it manually
   */
            return !!accel && debug_get_bool_option("MESA_HW_ACCEL_SELECT", accel > 0) &&
      /* internal geometry shader need indirect array access */
   pscreen->get_shader_param(pscreen, PIPE_SHADER_GEOMETRY,
         /* internal geometry shader need SSBO support */
   pscreen->get_shader_param(pscreen, PIPE_SHADER_GEOMETRY,
            case PIPE_CAP_QUERY_TIMESTAMP_BITS:
            case PIPE_CAP_VALIDATE_ALL_DIRTY_STATES:
   case PIPE_CAP_NULL_TEXTURES:
   case PIPE_CAP_ASTC_VOID_EXTENTS_NEED_DENORM_FLUSH:
   case PIPE_CAP_HAS_CONST_BW:
            default:
            }
      uint64_t u_default_get_timestamp(UNUSED struct pipe_screen *screen)
   {
         }
      static uint32_t
   hash_file_description(const void *key)
   {
      int fd = pointer_to_intptr(key);
            // File descriptions can't be hashed, but it should be safe to assume
   // that the same file description will always refer to he same file
   if (fstat(fd, &stat) == -1)
               }
         static bool
   equal_file_description(const void *key1, const void *key2)
   {
      int ret;
   int fd1 = pointer_to_intptr(key1);
   int fd2 = pointer_to_intptr(key2);
            // If the file descriptors are the same, the file description will be too
   // This will also catch sentinels, such as -1
   if (fd1 == fd2)
            ret = os_same_file_description(fd1, fd2);
   if (ret >= 0)
            {
      static bool has_warned;
   if (!has_warned)
      fprintf(stderr, "os_same_file_description couldn't determine if "
         "two DRM fds reference the same file description. (%s)\n"
   "Let's just assume that file descriptors for the same file probably"
                  // Let's at least check that it's the same file, different files can't
   // have the same file descriptions
   fstat(fd1, &stat1);
            return stat1.st_dev == stat2.st_dev &&
            }
         static struct hash_table *
   hash_table_create_file_description_keys(void)
   {
         }
      static struct hash_table *fd_tab = NULL;
      static simple_mtx_t screen_mutex = SIMPLE_MTX_INITIALIZER;
      static void
   drm_screen_destroy(struct pipe_screen *pscreen)
   {
               simple_mtx_lock(&screen_mutex);
   destroy = --pscreen->refcnt == 0;
   if (destroy) {
      int fd = pscreen->get_screen_fd(pscreen);
            if (!fd_tab->entries) {
      _mesa_hash_table_destroy(fd_tab, NULL);
         }
            if (destroy) {
      pscreen->destroy = pscreen->winsys_priv;
         }
      struct pipe_screen *
   u_pipe_screen_lookup_or_create(int gpu_fd,
                     {
               simple_mtx_lock(&screen_mutex);
   if (!fd_tab) {
      fd_tab = hash_table_create_file_description_keys();
   if (!fd_tab)
               pscreen = util_hash_table_get(fd_tab, intptr_to_pointer(gpu_fd));
   if (pscreen) {
         } else {
      pscreen = screen_create(gpu_fd, config, ro);
   if (pscreen) {
                     /* Bit of a hack, to avoid circular linkage dependency,
   * ie. pipe driver having to call in to winsys, we
   * override the pipe drivers screen->destroy() */
   pscreen->winsys_priv = pscreen->destroy;
               unlock:
      simple_mtx_unlock(&screen_mutex);
      }
