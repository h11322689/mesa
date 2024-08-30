   /*
   * Copyright (C) 2008 VMware, Inc.
   * Copyright (C) 2014 Broadcom
   * Copyright (C) 2018 Alyssa Rosenzweig
   * Copyright (C) 2019 Collabora, Ltd.
   * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   */
      #include "draw/draw_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "util/format/u_format.h"
   #include "util/format/u_format_s3tc.h"
   #include "util/os_time.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/u_process.h"
   #include "util/u_screen.h"
   #include "util/u_video.h"
      #include <fcntl.h>
      #include "drm-uapi/drm_fourcc.h"
   #include "drm-uapi/panfrost_drm.h"
      #include "decode.h"
   #include "pan_bo.h"
   #include "pan_fence.h"
   #include "pan_public.h"
   #include "pan_resource.h"
   #include "pan_screen.h"
   #include "pan_shader.h"
   #include "pan_util.h"
      #include "pan_context.h"
      #define DEFAULT_MAX_AFBC_PACKING_RATIO 90
      /* clang-format off */
   static const struct debug_named_value panfrost_debug_options[] = {
      {"perf",       PAN_DBG_PERF,     "Enable performance warnings"},
   {"trace",      PAN_DBG_TRACE,    "Trace the command stream"},
   {"dirty",      PAN_DBG_DIRTY,    "Always re-emit all state"},
   {"sync",       PAN_DBG_SYNC,     "Wait for each job's completion and abort on GPU faults"},
   {"nofp16",     PAN_DBG_NOFP16,    "Disable 16-bit support"},
   {"gl3",        PAN_DBG_GL3,      "Enable experimental GL 3.x implementation, up to 3.3"},
   {"noafbc",     PAN_DBG_NO_AFBC,  "Disable AFBC support"},
   {"crc",        PAN_DBG_CRC,      "Enable transaction elimination"},
   {"msaa16",     PAN_DBG_MSAA16,   "Enable MSAA 8x and 16x support"},
   {"linear",     PAN_DBG_LINEAR,   "Force linear textures"},
   {"nocache",    PAN_DBG_NO_CACHE, "Disable BO cache"},
      #ifdef PAN_DBG_OVERFLOW
         #endif
      {"yuv",        PAN_DBG_YUV,      "Tint YUV textures with blue for 1-plane and green for 2-plane"},
   {"forcepack",  PAN_DBG_FORCE_PACK,  "Force packing of AFBC textures on upload"},
      };
   /* clang-format on */
      static const char *
   panfrost_get_name(struct pipe_screen *screen)
   {
         }
      static const char *
   panfrost_get_vendor(struct pipe_screen *screen)
   {
         }
      static const char *
   panfrost_get_device_vendor(struct pipe_screen *screen)
   {
         }
      static int
   panfrost_get_param(struct pipe_screen *screen, enum pipe_cap param)
   {
               /* Our GL 3.x implementation is WIP */
            /* Native MRT is introduced with v5 */
            /* Only kernel drivers >= 1.1 can allocate HEAP BOs */
   bool has_heap = dev->kernel_version->version_major > 1 ||
            switch (param) {
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
   case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_DEPTH_CLIP_DISABLE_SEPARATE:
   case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_FRONTEND_NOOP:
   case PIPE_CAP_SAMPLE_SHADING:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
   case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
   case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
   case PIPE_CAP_SHADER_PACK_HALF_FLOAT:
   case PIPE_CAP_HAS_CONST_BW:
            case PIPE_CAP_MAX_RENDER_TARGETS:
   case PIPE_CAP_FBFETCH:
   case PIPE_CAP_FBFETCH_COHERENT:
            case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
            case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
            case PIPE_CAP_ANISOTROPIC_FILTER:
            /* Compile side is done for Bifrost, Midgard TODO. Needs some kernel
   * work to turn on, since CYCLE_COUNT_START needs to be issued. In
   * kbase, userspace requests this via BASE_JD_REQ_PERMON. There is not
   * yet way to request this with mainline TODO */
   case PIPE_CAP_SHADER_CLOCK:
            case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_TEXTURE_MULTISAMPLE:
   case PIPE_CAP_SURFACE_SAMPLE_COUNT:
            case PIPE_CAP_SAMPLER_VIEW_TARGET:
   case PIPE_CAP_CLIP_HALFZ:
   case PIPE_CAP_POLYGON_OFFSET_CLAMP:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_INDEP_BLEND_FUNC:
   case PIPE_CAP_GENERATE_MIPMAP:
   case PIPE_CAP_ACCELERATED:
   case PIPE_CAP_UMA:
   case PIPE_CAP_TEXTURE_FLOAT_LINEAR:
   case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
   case PIPE_CAP_SHADER_ARRAY_COMPONENTS:
   case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
   case PIPE_CAP_PACKED_UNIFORMS:
   case PIPE_CAP_IMAGE_LOAD_FORMATTED:
   case PIPE_CAP_CUBE_MAP_ARRAY:
   case PIPE_CAP_COMPUTE:
   case PIPE_CAP_INT64:
            /* We need this for OES_copy_image, but currently there are some awful
   * interactions with AFBC that need to be worked out. */
   case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
            case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
            case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
            case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_STREAM_OUTPUT_INTERLEAVE_BUFFERS:
            case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
            case PIPE_CAP_GLSL_FEATURE_LEVEL:
   case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
         case PIPE_CAP_ESSL_FEATURE_LEVEL:
            case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
            /* v7 (only) restricts component orders with AFBC. To workaround, we
   * compose format swizzles with texture swizzles. pan_texture.c motsly
   * handles this but we need to fix up the border colour.
   */
   case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
      if (dev->arch == 7)
         else
         case PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT:
            /* Must be at least 64 for correct behaviour */
   case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_QUERY_TIMESTAMP:
            /* The hardware requires element alignment for data conversion to work
   * as expected. If data conversion is not required, this restriction is
   * lifted on Midgard at a performance penalty. We conservatively
   * require element alignment for vertex buffers, using u_vbuf to
   * translate to match the hardware requirement.
   *
   * This is less heavy-handed than the 4BYTE_ALIGNED_ONLY caps, which
   * would needlessly require alignment even for 8-bit formats.
   */
   case PIPE_CAP_VERTEX_ATTRIB_ELEMENT_ALIGNED_ONLY:
            case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
            case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            case PIPE_CAP_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_INTEGER:
      /* Hardware is upper left. Pixel center at (0.5, 0.5) */
         case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_TGSI_TEXCOORD:
            /* We would prefer varyings on Midgard, but proper sysvals on Bifrost */
   case PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL:
   case PIPE_CAP_FS_POSITION_IS_SYSVAL:
   case PIPE_CAP_FS_POINT_IS_SYSVAL:
            case PIPE_CAP_SEAMLESS_CUBE_MAP:
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
            case PIPE_CAP_MAX_VERTEX_ELEMENT_SRC_OFFSET:
            case PIPE_CAP_TEXTURE_TRANSFER_MODES:
            case PIPE_CAP_ENDIANNESS:
            case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
            case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
            case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
            case PIPE_CAP_VIDEO_MEMORY: {
               if (!os_get_total_physical_memory(&system_memory))
                        case PIPE_CAP_SHADER_STENCIL_EXPORT:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
            case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_MAX_VARYINGS:
      /* Return the GLSL maximum. The internal maximum
   * PAN_MAX_VARYINGS accommodates internal varyings. */
         /* Removed in v6 (Bifrost) */
   case PIPE_CAP_GL_CLAMP:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_ALPHA_TEST:
            /* Removed in v9 (Valhall). PRIMTIIVE_RESTART_FIXED_INDEX is of course
   * still supported as it is core GLES3.0 functionality
   */
   case PIPE_CAP_EMULATE_NONFIXED_PRIMITIVE_RESTART:
            case PIPE_CAP_FLATSHADE:
   case PIPE_CAP_TWO_SIDED_COLOR:
   case PIPE_CAP_CLIP_PLANES:
            case PIPE_CAP_PACKED_STREAM_OUTPUT:
            case PIPE_CAP_VIEWPORT_TRANSFORM_LOWERED:
   case PIPE_CAP_PSIZ_CLAMPED:
            case PIPE_CAP_NIR_IMAGES_AS_DEREF:
            case PIPE_CAP_DRAW_INDIRECT:
            case PIPE_CAP_START_INSTANCE:
   case PIPE_CAP_DRAW_PARAMETERS:
            case PIPE_CAP_SUPPORTED_PRIM_MODES:
   case PIPE_CAP_SUPPORTED_PRIM_MODES_WITH_RESTART: {
      /* Mali supports GLES and QUADS. Midgard and v6 Bifrost
   * support more */
            if (dev->arch <= 6) {
      modes |= BITFIELD_BIT(MESA_PRIM_QUAD_STRIP);
               if (dev->arch >= 9) {
      /* Although Valhall is supposed to support quads, they
   * don't seem to work correctly. Disable to fix
   * arb-provoking-vertex-render.
   */
                           case PIPE_CAP_IMAGE_STORE_FORMATTED:
            case PIPE_CAP_NATIVE_FENCE_FD:
            default:
            }
      static int
   panfrost_get_shader_param(struct pipe_screen *screen,
               {
      struct panfrost_device *dev = pan_device(screen);
            switch (shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_FRAGMENT:
   case PIPE_SHADER_COMPUTE:
         default:
                  /* We only allow observable side effects (memory writes) in compute and
   * fragment shaders. Side effects in the geometry pipeline cause
   * trouble with IDVS and conflict with our transform feedback lowering.
   */
            switch (param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
            case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
            case PIPE_SHADER_CAP_MAX_INPUTS:
      /* Used as ABI on Midgard */
         case PIPE_SHADER_CAP_MAX_OUTPUTS:
            case PIPE_SHADER_CAP_MAX_TEMPS:
            case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
            case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
      STATIC_ASSERT(PAN_MAX_CONST_BUFFERS < 0x100);
         case PIPE_SHADER_CAP_CONT_SUPPORTED:
            case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
         case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
            case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
            case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
            case PIPE_SHADER_CAP_SUBROUTINES:
            case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
            case PIPE_SHADER_CAP_INTEGERS:
               /* The Bifrost compiler supports full 16-bit. Midgard could but int16
   * support is untested, so restrict INT16 to Bifrost. Midgard
         case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
         case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
         case PIPE_SHADER_CAP_INT16:
      /* Blocked on https://gitlab.freedesktop.org/mesa/mesa/-/issues/6075 */
         case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
            case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
      STATIC_ASSERT(PIPE_MAX_SAMPLERS < 0x10000);
         case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
      STATIC_ASSERT(PIPE_MAX_SHADER_SAMPLER_VIEWS < 0x10000);
         case PIPE_SHADER_CAP_SUPPORTED_IRS:
            case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
            case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
            case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
            default:
                     }
      static float
   panfrost_get_paramf(struct pipe_screen *screen, enum pipe_capf param)
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
      debug_printf("Unexpected PIPE_CAPF %d query\n", param);
         }
      static uint32_t
   pipe_to_pan_bind_flags(uint32_t pipe_bind_flags)
   {
      static_assert(PIPE_BIND_DEPTH_STENCIL == PAN_BIND_DEPTH_STENCIL, "");
   static_assert(PIPE_BIND_RENDER_TARGET == PAN_BIND_RENDER_TARGET, "");
   static_assert(PIPE_BIND_SAMPLER_VIEW == PAN_BIND_SAMPLER_VIEW, "");
            return pipe_bind_flags & (PAN_BIND_DEPTH_STENCIL | PAN_BIND_RENDER_TARGET |
      }
      /**
   * Query format support for creating a texture, drawing surface, etc.
   * \param format  the format to test
   * \param type  one of PIPE_TEXTURE, PIPE_SURFACE
   */
   static bool
   panfrost_is_format_supported(struct pipe_screen *screen,
                           {
               assert(target == PIPE_BUFFER || target == PIPE_TEXTURE_1D ||
         target == PIPE_TEXTURE_1D_ARRAY || target == PIPE_TEXTURE_2D ||
   target == PIPE_TEXTURE_2D_ARRAY || target == PIPE_TEXTURE_RECT ||
            /* MSAA 2x gets rounded up to 4x. MSAA 8x/16x only supported on v5+.
            switch (sample_count) {
   case 0:
   case 1:
   case 4:
         case 8:
   case 16:
      if (dev->debug & PAN_DBG_MSAA16)
         else
      default:
                  if (MAX2(sample_count, 1) != MAX2(storage_sample_count, 1))
            /* Z16 causes dEQP failures on t720 */
   if (format == PIPE_FORMAT_Z16_UNORM && dev->arch <= 4)
                                       /* Also check that compressed texture formats are supported on this
   * particular chip. They may not be depending on system integration
            bool supported =
            if (!supported)
               }
      /* We always support linear and tiled operations, both external and internal.
   * We support AFBC for a subset of formats, and colourspace transform for a
   * subset of those. */
      static void
   panfrost_walk_dmabuf_modifiers(struct pipe_screen *screen,
                     {
      /* Query AFBC status */
   struct panfrost_device *dev = pan_device(screen);
   bool afbc = dev->has_afbc && panfrost_format_supports_afbc(dev, format);
   bool ytr = panfrost_afbc_can_ytr(format);
                     for (unsigned i = 0; i < PAN_MODIFIER_COUNT; ++i) {
      if (drm_is_afbc(pan_best_modifiers[i]) && !afbc)
            if ((pan_best_modifiers[i] & AFBC_FORMAT_MOD_YTR) && !ytr)
            if ((pan_best_modifiers[i] & AFBC_FORMAT_MOD_TILED) && !tiled_afbc)
            if (test_modifier != DRM_FORMAT_MOD_INVALID &&
                  if (max > (int)count) {
               if (external_only)
      }
                  }
      static void
   panfrost_query_dmabuf_modifiers(struct pipe_screen *screen,
                     {
      panfrost_walk_dmabuf_modifiers(screen, format, max, modifiers, external_only,
      }
      static bool
   panfrost_is_dmabuf_modifier_supported(struct pipe_screen *screen,
                     {
      uint64_t unused;
   unsigned int uint_extern_only = 0;
            panfrost_walk_dmabuf_modifiers(screen, format, 1, &unused, &uint_extern_only,
            if (external_only)
               }
      static int
   panfrost_get_compute_param(struct pipe_screen *pscreen,
               {
      struct panfrost_device *dev = pan_device(pscreen);
         #define RET(x)                                                                 \
      do {                                                                        \
      if (ret)                                                                 \
                     switch (param) {
   case PIPE_COMPUTE_CAP_ADDRESS_BITS:
            case PIPE_COMPUTE_CAP_IR_TARGET:
      if (ret)
               case PIPE_COMPUTE_CAP_GRID_DIMENSION:
            case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
            case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
      /* Unpredictable behaviour at larger sizes. Mali-G52 advertises
   * 384x384x384.
   *
   * On Midgard, we don't allow more than 128 threads in each
   * direction to match PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK.
   * That still exceeds the minimum-maximum.
   */
   if (dev->arch >= 6)
         else
         case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
      /* On Bifrost and newer, all GPUs can support at least 256 threads
   * regardless of register usage, so we report 256.
   *
   * On Midgard, with maximum register usage, the maximum
   * thread count is only 64. We would like to report 64 here, but
   * the GLES3.1 spec minimum is 128, so we report 128 and limit
   * the register allocation of affected compute kernels.
   */
         case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE:
            case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE:
            case PIPE_COMPUTE_CAP_MAX_PRIVATE_SIZE:
   case PIPE_COMPUTE_CAP_MAX_INPUT_SIZE:
            case PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE:
            case PIPE_COMPUTE_CAP_MAX_CLOCK_FREQUENCY:
            case PIPE_COMPUTE_CAP_MAX_COMPUTE_UNITS:
            case PIPE_COMPUTE_CAP_IMAGES_SUPPORTED:
            case PIPE_COMPUTE_CAP_SUBGROUP_SIZES:
            case PIPE_COMPUTE_CAP_MAX_SUBGROUPS:
            case PIPE_COMPUTE_CAP_MAX_VARIABLE_THREADS_PER_BLOCK:
                     }
      static void
   panfrost_destroy_screen(struct pipe_screen *pscreen)
   {
      struct panfrost_device *dev = pan_device(pscreen);
            panfrost_resource_screen_destroy(pscreen);
   panfrost_pool_cleanup(&screen->blitter.bin_pool);
   panfrost_pool_cleanup(&screen->blitter.desc_pool);
            if (screen->vtbl.screen_destroy)
            if (dev->ro)
                  disk_cache_destroy(screen->disk_cache);
      }
      static const void *
   panfrost_screen_get_compiler_options(struct pipe_screen *pscreen,
               {
         }
      static struct disk_cache *
   panfrost_get_disk_shader_cache(struct pipe_screen *pscreen)
   {
         }
      static int
   panfrost_get_screen_fd(struct pipe_screen *pscreen)
   {
         }
      int
   panfrost_get_driver_query_info(struct pipe_screen *pscreen, unsigned index,
         {
               if (!info)
            if (index >= num_queries)
                        }
      struct pipe_screen *
   panfrost_create_screen(int fd, const struct pipe_screen_config *config,
         {
      /* Create the screen */
            if (!screen)
                     /* Debug must be set first for pandecode to work correctly */
   dev->debug =
         screen->max_afbc_packing_ratio = debug_get_num_option(
                  if (dev->debug & PAN_DBG_NO_AFBC)
            /* Bail early on unsupported hardware */
   if (dev->model == NULL) {
      debug_printf("panfrost: Unsupported model %X", dev->gpu_id);
   panfrost_destroy_screen(&(screen->base));
                                 screen->base.get_screen_fd = panfrost_get_screen_fd;
   screen->base.get_name = panfrost_get_name;
   screen->base.get_vendor = panfrost_get_vendor;
   screen->base.get_device_vendor = panfrost_get_device_vendor;
   screen->base.get_driver_query_info = panfrost_get_driver_query_info;
   screen->base.get_param = panfrost_get_param;
   screen->base.get_shader_param = panfrost_get_shader_param;
   screen->base.get_compute_param = panfrost_get_compute_param;
   screen->base.get_paramf = panfrost_get_paramf;
   screen->base.get_timestamp = u_default_get_timestamp;
   screen->base.is_format_supported = panfrost_is_format_supported;
   screen->base.query_dmabuf_modifiers = panfrost_query_dmabuf_modifiers;
   screen->base.is_dmabuf_modifier_supported =
         screen->base.context_create = panfrost_create_context;
   screen->base.get_compiler_options = panfrost_screen_get_compiler_options;
   screen->base.get_disk_shader_cache = panfrost_get_disk_shader_cache;
   screen->base.fence_reference = panfrost_fence_reference;
   screen->base.fence_finish = panfrost_fence_finish;
   screen->base.fence_get_fd = panfrost_fence_get_fd;
            panfrost_resource_screen_init(&screen->base);
                     panfrost_pool_init(&screen->blitter.bin_pool, NULL, dev, PAN_BO_EXECUTE,
         panfrost_pool_init(&screen->blitter.desc_pool, NULL, dev, 0, 65536,
         if (dev->arch == 4)
         else if (dev->arch == 5)
         else if (dev->arch == 6)
         else if (dev->arch == 7)
         else if (dev->arch == 9)
         else
               }
