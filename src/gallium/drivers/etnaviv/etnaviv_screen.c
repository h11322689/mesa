   /*
   * Copyright (c) 2012-2015 Etnaviv Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Wladimir J. van der Laan <laanwj@gmail.com>
   *    Christian Gmeiner <christian.gmeiner@gmail.com>
   */
      #include "etnaviv_screen.h"
      #include "hw/common.xml.h"
      #include "etnaviv_compiler.h"
   #include "etnaviv_context.h"
   #include "etnaviv_debug.h"
   #include "etnaviv_fence.h"
   #include "etnaviv_format.h"
   #include "etnaviv_query.h"
   #include "etnaviv_resource.h"
   #include "etnaviv_translate.h"
      #include "util/hash_table.h"
   #include "util/os_time.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_screen.h"
   #include "util/u_string.h"
      #include "frontend/drm_driver.h"
      #include "drm-uapi/drm_fourcc.h"
      #define ETNA_DRM_VERSION_FENCE_FD      ETNA_DRM_VERSION(1, 1)
   #define ETNA_DRM_VERSION_PERFMON       ETNA_DRM_VERSION(1, 2)
      static const struct debug_named_value etna_debug_options[] = {
      {"dbg_msgs",       ETNA_DBG_MSGS, "Print debug messages"},
   {"drm_msgs",       ETNA_DRM_MSGS, "Print drm messages"},
   {"frame_msgs",     ETNA_DBG_FRAME_MSGS, "Print frame messages"},
   {"resource_msgs",  ETNA_DBG_RESOURCE_MSGS, "Print resource messages"},
   {"compiler_msgs",  ETNA_DBG_COMPILER_MSGS, "Print compiler messages"},
   {"linker_msgs",    ETNA_DBG_LINKER_MSGS, "Print linker messages"},
   {"dump_shaders",   ETNA_DBG_DUMP_SHADERS, "Dump shaders"},
   {"no_ts",          ETNA_DBG_NO_TS, "Disable TS"},
   {"no_autodisable", ETNA_DBG_NO_AUTODISABLE, "Disable autodisable"},
   {"no_supertile",   ETNA_DBG_NO_SUPERTILE, "Disable supertiles"},
   {"no_early_z",     ETNA_DBG_NO_EARLY_Z, "Disable early z"},
   {"cflush_all",     ETNA_DBG_CFLUSH_ALL, "Flush every cache before state update"},
   {"flush_all",      ETNA_DBG_FLUSH_ALL, "Flush after every rendered primitive"},
   {"zero",           ETNA_DBG_ZERO, "Zero all resources after allocation"},
   {"draw_stall",     ETNA_DBG_DRAW_STALL, "Stall FE/PE after each rendered primitive"},
   {"shaderdb",       ETNA_DBG_SHADERDB, "Enable shaderdb output"},
   {"no_singlebuffer",ETNA_DBG_NO_SINGLEBUF, "Disable single buffer feature"},
   {"deqp",           ETNA_DBG_DEQP, "Hacks to run dEQP GLES3 tests"}, /* needs MESA_GLES_VERSION_OVERRIDE=3.0 */
   {"nocache",        ETNA_DBG_NOCACHE,    "Disable shader cache"},
   {"linear_pe",      ETNA_DBG_LINEAR_PE, "Enable linear PE"},
   {"msaa",           ETNA_DBG_MSAA, "Enable MSAA support"},
   {"shared_ts",      ETNA_DBG_SHARED_TS, "Enable TS sharing"},
   {"perf",           ETNA_DBG_PERF, "Enable performance warnings"},
      };
      DEBUG_GET_ONCE_FLAGS_OPTION(etna_mesa_debug, "ETNA_MESA_DEBUG", etna_debug_options, 0)
   int etna_mesa_debug = 0;
      static void
   etna_screen_destroy(struct pipe_screen *pscreen)
   {
               if (screen->dummy_desc_reloc.bo)
            if (screen->dummy_rt_reloc.bo)
            if (screen->perfmon)
                              if (screen->pipe)
            if (screen->gpu)
            if (screen->ro)
            if (screen->dev)
               }
      static const char *
   etna_screen_get_name(struct pipe_screen *pscreen)
   {
      struct etna_screen *priv = etna_screen(pscreen);
            snprintf(buffer, sizeof(buffer), "Vivante GC%x rev %04x", priv->model,
               }
      static const char *
   etna_screen_get_vendor(struct pipe_screen *pscreen)
   {
         }
      static const char *
   etna_screen_get_device_vendor(struct pipe_screen *pscreen)
   {
         }
      static int
   etna_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
   {
               switch (param) {
   /* Supported features (boolean caps). */
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
   case PIPE_CAP_TEXTURE_BARRIER:
   case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
   case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_TGSI_TEXCOORD:
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
   case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_STRING_MARKER:
   case PIPE_CAP_FRONTEND_NOOP:
         case PIPE_CAP_NATIVE_FENCE_FD:
         case PIPE_CAP_FS_POSITION_IS_SYSVAL:
   case PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL: /* note: not integer */
         case PIPE_CAP_FS_POINT_IS_SYSVAL:
            /* Memory */
   case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
            case PIPE_CAP_NPOT_TEXTURES:
      return true; /* VIV_FEATURE(priv->dev, chipMinorFeatures1,
         case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
            case PIPE_CAP_ALPHA_TEST:
            /* Unsupported features. */
   case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
   case PIPE_CAP_TEXRECT:
            /* Stream output. */
   case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
            case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
         case PIPE_CAP_MAX_VERTEX_ELEMENT_SRC_OFFSET:
         case PIPE_CAP_MAX_VERTEX_BUFFERS:
         case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
               /* Texturing. */
   case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
         case PIPE_CAP_TEXTURE_SHADOW_MAP:
         case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
         case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS: /* TODO: verify */
         case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      if (screen->specs.halti < 0)
            case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
   {
      int log2_max_tex_size = util_last_bit(screen->specs.max_texture_size);
   assert(log2_max_tex_size > 0);
               case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
   case PIPE_CAP_MIN_TEXEL_OFFSET:
         case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
   case PIPE_CAP_MAX_TEXEL_OFFSET:
         case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
            /* Queries. */
   case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
            /* Preferences */
   case PIPE_CAP_TEXTURE_TRANSFER_MODES:
         case PIPE_CAP_MAX_TEXTURE_UPLOAD_MEMORY_BUDGET: {
      /* etnaviv is being run on systems as small as 256MB total RAM so
   * we need to provide a sane value for such a device. Limit the
   * memory budget to min(~3% of pyhiscal memory, 64MB).
   *
   * a simple divison by 32 provides the numbers we want.
   *    256MB / 32 =  8MB
   *   2048MB / 32 = 64MB
   */
            if (!os_get_total_physical_memory(&system_memory))
                        case PIPE_CAP_MAX_VARYINGS:
            case PIPE_CAP_SUPPORTED_PRIM_MODES:
   case PIPE_CAP_SUPPORTED_PRIM_MODES_WITH_RESTART: {
      /* Generate the bitmask of supported draw primitives. */
   uint32_t modes = 1 << MESA_PRIM_POINTS |
                              /* TODO: The bug relates only to indexed draws, but here we signal
   * that there is no support for triangle strips at all. This should
   * be refined.
   */
   if (VIV_FEATURE(screen, chipMinorFeatures2, BUG_FIXES8))
            if (VIV_FEATURE(screen, chipMinorFeatures2, LINE_LOOP))
                        case PIPE_CAP_PCI_GROUP:
   case PIPE_CAP_PCI_BUS:
   case PIPE_CAP_PCI_DEVICE:
   case PIPE_CAP_PCI_FUNCTION:
         case PIPE_CAP_ACCELERATED:
         case PIPE_CAP_VIDEO_MEMORY:
         case PIPE_CAP_UMA:
         default:
            }
      static float
   etna_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
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
                  debug_printf("unknown paramf %d", param);
      }
      static int
   etna_screen_get_shader_param(struct pipe_screen *pscreen,
               {
      struct etna_screen *screen = etna_screen(pscreen);
            if (DBG_ENABLED(ETNA_DBG_DEQP))
            switch (shader) {
   case PIPE_SHADER_FRAGMENT:
   case PIPE_SHADER_VERTEX:
         case PIPE_SHADER_COMPUTE:
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL:
         default:
      DBG("unknown shader type %d", shader);
               switch (param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         case PIPE_SHADER_CAP_MAX_INPUTS:
      /* Maximum number of inputs for the vertex shader is the number
   * of vertex elements - each element defines one vertex shader
   * input register.  For the fragment shader, this is the number
   * of varyings. */
   return shader == PIPE_SHADER_FRAGMENT ? screen->specs.max_varyings
      case PIPE_SHADER_CAP_MAX_OUTPUTS:
         case PIPE_SHADER_CAP_MAX_TEMPS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         case PIPE_SHADER_CAP_CONT_SUPPORTED:
         case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         case PIPE_SHADER_CAP_SUBROUTINES:
         case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
         case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
         case PIPE_SHADER_CAP_INTEGERS:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
      return shader == PIPE_SHADER_FRAGMENT
            case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
      if (ubo_enable)
         return shader == PIPE_SHADER_FRAGMENT
            case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
         case PIPE_SHADER_CAP_SUPPORTED_IRS:
      return (1 << PIPE_SHADER_IR_TGSI) |
      case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
   case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
                  debug_printf("unknown shader param %d", param);
      }
      static bool
   gpu_supports_texture_target(struct etna_screen *screen,
         {
      if (target == PIPE_TEXTURE_CUBE_ARRAY)
            /* pre-halti has no array/3D */
   if (screen->specs.halti < 0 &&
      (target == PIPE_TEXTURE_1D_ARRAY ||
   target == PIPE_TEXTURE_2D_ARRAY ||
   target == PIPE_TEXTURE_3D))
            }
      static bool
   gpu_supports_texture_format(struct etna_screen *screen, uint32_t fmt,
         {
               /* Requires split sampler support, which the driver doesn't support, yet. */
   if (!util_format_is_compressed(format) &&
      util_format_get_blocksizebits(format) > 32)
         if (fmt == TEXTURE_FORMAT_ETC1)
            if (fmt >= TEXTURE_FORMAT_DXT1 && fmt <= TEXTURE_FORMAT_DXT4_DXT5)
            if (util_format_is_srgb(format))
            if (fmt & EXT_FORMAT)
            if (fmt & ASTC_FORMAT) {
                  if (util_format_is_snorm(format))
            if (format != PIPE_FORMAT_S8_UINT_Z24_UNORM &&
      (util_format_is_pure_integer(format) || util_format_is_float(format)))
            if (!supported)
            if (texture_format_needs_swiz(format))
               }
      static bool
   gpu_supports_render_format(struct etna_screen *screen, enum pipe_format format,
         {
               if (fmt == ETNA_NO_MATCH)
            /* Requires split target support, which the driver doesn't support, yet. */
   if (util_format_get_blocksizebits(format) > 32)
            if (sample_count > 1) {
      /* Explicitly enabled. */
   if (!DBG_ENABLED(ETNA_DBG_MSAA))
            /* The hardware supports it. */
   if (!VIV_FEATURE(screen, chipFeatures, MSAA))
            /* Number of samples must be allowed. */
   if (!translate_samples_to_xyscale(sample_count, NULL, NULL))
            /* On SMALL_MSAA hardware 2x MSAA does not work. */
   if (sample_count == 2 && VIV_FEATURE(screen, chipMinorFeatures4, SMALL_MSAA))
            /* BLT/RS supports the format. */
   if (screen->specs.use_blt) {
      if (translate_blt_format(format) == ETNA_NO_MATCH)
      } else {
      if (translate_rs_format(format) == ETNA_NO_MATCH)
                  if (format == PIPE_FORMAT_R8_UNORM)
            /* figure out 8bpp RS clear to enable these formats */
   if (format == PIPE_FORMAT_R8_SINT || format == PIPE_FORMAT_R8_UINT)
            if (util_format_is_srgb(format))
            if (util_format_is_pure_integer(format) || util_format_is_float(format))
            if (format == PIPE_FORMAT_R8G8_UNORM)
            /* any other extended format is HALTI0 (only R10G10B10A2?) */
   if (fmt >= PE_FORMAT_R16F)
               }
      static bool
   gpu_supports_vertex_format(struct etna_screen *screen, enum pipe_format format)
   {
      if (translate_vertex_format_type(format) == ETNA_NO_MATCH)
            if (util_format_is_pure_integer(format))
               }
      static bool
   etna_screen_is_format_supported(struct pipe_screen *pscreen,
                                 {
      struct etna_screen *screen = etna_screen(pscreen);
            if (!gpu_supports_texture_target(screen, target))
            if (MAX2(1, sample_count) != MAX2(1, storage_sample_count))
            if (usage & PIPE_BIND_RENDER_TARGET) {
      if (gpu_supports_render_format(screen, format, sample_count))
               if (usage & PIPE_BIND_DEPTH_STENCIL) {
      if (translate_depth_format(format) != ETNA_NO_MATCH)
               if (usage & PIPE_BIND_SAMPLER_VIEW) {
               if (!gpu_supports_texture_format(screen, fmt, format))
            if (sample_count < 2 && fmt != ETNA_NO_MATCH)
               if (usage & PIPE_BIND_VERTEX_BUFFER) {
      if (gpu_supports_vertex_format(screen, format))
               if (usage & PIPE_BIND_INDEX_BUFFER) {
      /* must be supported index format */
   if (format == PIPE_FORMAT_R8_UINT || format == PIPE_FORMAT_R16_UINT ||
      (format == PIPE_FORMAT_R32_UINT &&
   VIV_FEATURE(screen, chipFeatures, 32_BIT_INDICES))) {
                  /* Always allowed */
   allowed |=
            if (usage != allowed) {
      DBG("not supported: format=%s, target=%d, sample_count=%d, "
      "usage=%x, allowed=%x",
               }
      const uint64_t supported_modifiers[] = {
      DRM_FORMAT_MOD_LINEAR,
   DRM_FORMAT_MOD_VIVANTE_TILED,
   DRM_FORMAT_MOD_VIVANTE_SUPER_TILED,
   DRM_FORMAT_MOD_VIVANTE_SPLIT_TILED,
      };
      static int etna_get_num_modifiers(struct etna_screen *screen)
   {
               /* don't advertise split tiled formats on single pipe/buffer GPUs */
   if (screen->specs.pixel_pipes == 1 || screen->specs.single_buffer)
               }
      static void
   etna_screen_query_dmabuf_modifiers(struct pipe_screen *pscreen,
                     {
      struct etna_screen *screen = etna_screen(pscreen);
   int num_base_mods = etna_get_num_modifiers(screen);
   int mods_multiplier = 1;
            if (DBG_ENABLED(ETNA_DBG_SHARED_TS) &&
      VIV_FEATURE(screen, chipFeatures, FAST_CLEAR)) {
   /* If TS is supported expose the TS modifiers. GPUs with feature
   * CACHE128B256BPERLINE have both 128B and 256B color tile TS modes,
   * older cores support exactly one TS layout.
   */
   if (VIV_FEATURE(screen, chipMinorFeatures6, CACHE128B256BPERLINE))
      if (screen->specs.v4_compression &&
      translate_ts_format(format) != ETNA_NO_MATCH)
      else
      else
               if (max > num_base_mods * mods_multiplier)
            if (!max) {
      modifiers = NULL;
               for (i = 0, *count = 0; *count < max && i < num_base_mods; i++) {
      for (j = 0; *count < max && j < mods_multiplier; j++, (*count)++) {
               if (j == 0) {
         } else if (VIV_FEATURE(screen, chipMinorFeatures6,
            switch (j) {
   case 1:
      ts_mod = VIVANTE_MOD_TS_128_4;
      case 2:
      ts_mod = VIVANTE_MOD_TS_256_4;
      case 3:
      ts_mod = VIVANTE_MOD_TS_128_4 | VIVANTE_MOD_COMP_DEC400;
      case 4:
            } else {
      if (screen->specs.bits_per_tile == 2)
         else
               if (modifiers)
         if (external_only)
            }
      static bool
   etna_screen_is_dmabuf_modifier_supported(struct pipe_screen *pscreen,
                     {
      struct etna_screen *screen = etna_screen(pscreen);
   int num_base_mods = etna_get_num_modifiers(screen);
   uint64_t base_mod = modifier & ~VIVANTE_MOD_EXT_MASK;
   uint64_t ts_mod = modifier & VIVANTE_MOD_TS_MASK;
            for (i = 0; i < num_base_mods; i++) {
      if (base_mod != supported_modifiers[i])
            if ((modifier & VIVANTE_MOD_COMP_DEC400) &&
                  if (ts_mod) {
                     if (VIV_FEATURE(screen, chipMinorFeatures6, CACHE128B256BPERLINE)) {
      if (ts_mod != VIVANTE_MOD_TS_128_4 &&
      ts_mod != VIVANTE_MOD_TS_256_4)
   } else {
      if ((screen->specs.bits_per_tile == 2 &&
      ts_mod != VIVANTE_MOD_TS_64_2) ||
   (screen->specs.bits_per_tile == 4 &&
   ts_mod != VIVANTE_MOD_TS_64_4))
               if (external_only)
                           }
      static unsigned int
   etna_screen_get_dmabuf_modifier_planes(struct pipe_screen *pscreen,
               {
               if (modifier & VIVANTE_MOD_TS_MASK)
               }
      static void
   etna_determine_uniform_limits(struct etna_screen *screen)
   {
      /* values for the non unified case are taken from
   * gcmCONFIGUREUNIFORMS in the Vivante kernel driver file
   * drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_base.h.
   */
   if (screen->model == chipModel_GC2000 &&
      (screen->revision == 0x5118 || screen->revision == 0x5140)) {
   screen->specs.max_vs_uniforms = 256;
      } else if (screen->specs.num_constants == 320) {
      screen->specs.max_vs_uniforms = 256;
      } else if (screen->specs.num_constants > 256 &&
            /* All GC1000 series chips can only support 64 uniforms for ps on non-unified const mode. */
   screen->specs.max_vs_uniforms = 256;
      } else if (screen->specs.num_constants > 256) {
      screen->specs.max_vs_uniforms = 256;
      } else if (screen->specs.num_constants == 256) {
      screen->specs.max_vs_uniforms = 256;
      } else {
      screen->specs.max_vs_uniforms = 168;
         }
      static void
   etna_determine_sampler_limits(struct etna_screen *screen)
   {
      /* vertex and fragment samplers live in one address space */
   if (screen->specs.halti >= 1) {
      screen->specs.vertex_sampler_offset = 16;
   screen->specs.fragment_sampler_count = 16;
      } else {
      screen->specs.vertex_sampler_offset = 8;
   screen->specs.fragment_sampler_count = 8;
               if (screen->model == 0x400)
      }
      static bool
   etna_get_specs(struct etna_screen *screen)
   {
      uint64_t val;
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_INSTRUCTION_COUNT, &val)) {
      DBG("could not get ETNA_GPU_INSTRUCTION_COUNT");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_VERTEX_OUTPUT_BUFFER_SIZE,
            DBG("could not get ETNA_GPU_VERTEX_OUTPUT_BUFFER_SIZE");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_VERTEX_CACHE_SIZE, &val)) {
      DBG("could not get ETNA_GPU_VERTEX_CACHE_SIZE");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_SHADER_CORE_COUNT, &val)) {
      DBG("could not get ETNA_GPU_SHADER_CORE_COUNT");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_STREAM_COUNT, &val)) {
      DBG("could not get ETNA_GPU_STREAM_COUNT");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_REGISTER_MAX, &val)) {
      DBG("could not get ETNA_GPU_REGISTER_MAX");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_PIXEL_PIPES, &val)) {
      DBG("could not get ETNA_GPU_PIXEL_PIPES");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_NUM_CONSTANTS, &val)) {
      DBG("could not get %s", "ETNA_GPU_NUM_CONSTANTS");
      }
   if (val == 0) {
      fprintf(stderr, "Warning: zero num constants (update kernel?)\n");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_NUM_VARYINGS, &val)) {
      DBG("could not get ETNA_GPU_NUM_VARYINGS");
      }
            /* Figure out gross GPU architecture. See rnndb/common.xml for a specific
   * description of the differences. */
   if (VIV_FEATURE(screen, chipMinorFeatures5, HALTI5))
         else if (VIV_FEATURE(screen, chipMinorFeatures5, HALTI4))
         else if (VIV_FEATURE(screen, chipMinorFeatures5, HALTI3))
         else if (VIV_FEATURE(screen, chipMinorFeatures4, HALTI2))
         else if (VIV_FEATURE(screen, chipMinorFeatures2, HALTI1))
         else if (VIV_FEATURE(screen, chipMinorFeatures1, HALTI0))
         else
         if (screen->specs.halti >= 0)
         else
            screen->specs.can_supertile =
         screen->specs.bits_per_tile =
      !VIV_FEATURE(screen, chipMinorFeatures0, 2BITPERTILE) ||
         screen->specs.ts_clear_value =
      VIV_FEATURE(screen, chipMinorFeatures10, DEC400) ? 0xffffffff :
         screen->specs.vs_need_z_div =
         screen->specs.has_sin_cos_sqrt =
         screen->specs.has_sign_floor_ceil =
         screen->specs.has_shader_range_registers =
         screen->specs.npot_tex_any_wrap =
         screen->specs.has_new_transcendentals =
         screen->specs.has_halti2_instructions =
         screen->specs.has_no_oneconst_limit =
         screen->specs.v4_compression =
         screen->specs.seamless_cube_map =
      (screen->model != 0x880) && /* Seamless cubemap is broken on GC880? */
         if (screen->specs.halti >= 5) {
      /* GC7000 - this core must load shaders from memory. */
   screen->specs.vs_offset = 0;
   screen->specs.ps_offset = 0;
   screen->specs.max_instructions = 0; /* Do not program shaders manually */
      } else if (VIV_FEATURE(screen, chipMinorFeatures3, INSTRUCTION_CACHE)) {
      /* GC3000 - this core is capable of loading shaders from
   * memory. It can also run shaders from registers, as a fallback, but
   * "max_instructions" does not have the correct value. It has place for
   * 2*256 instructions just like GC2000, but the offsets are slightly
   * different.
   */
   screen->specs.vs_offset = 0xC000;
   /* State 08000-0C000 mirrors 0C000-0E000, and the Vivante driver uses
   * this mirror for writing PS instructions, probably safest to do the
   * same.
   */
   screen->specs.ps_offset = 0x8000 + 0x1000;
   screen->specs.max_instructions = 256; /* maximum number instructions for non-icache use */
      } else {
      if (instruction_count > 256) { /* unified instruction memory? */
      screen->specs.vs_offset = 0xC000;
   screen->specs.ps_offset = 0xD000; /* like vivante driver */
      } else {
      screen->specs.vs_offset = 0x4000;
   screen->specs.ps_offset = 0x6000;
      }
               if (VIV_FEATURE(screen, chipMinorFeatures1, HALTI0)) {
         } else {
      /* Etna_viv documentation seems confused over the correct value
   * here so choose the lower to be safe: HALTI0 says 16 i.s.o.
   * 10, but VERTEX_ELEMENT_CONFIG register says 16 i.s.o. 12. */
               etna_determine_uniform_limits(screen);
            if (screen->specs.halti >= 5) {
      screen->specs.has_unified_uniforms = true;
   screen->specs.vs_uniforms_offset = VIVS_SH_HALTI5_UNIFORMS_MIRROR(0);
      } else if (screen->specs.halti >= 1) {
      /* unified uniform memory on GC3000 - HALTI1 feature bit is just a guess
   */
   screen->specs.has_unified_uniforms = true;
   screen->specs.vs_uniforms_offset = VIVS_SH_UNIFORMS(0);
   /* hardcode PS uniforms to start after end of VS uniforms -
   * for more flexibility this offset could be variable based on the
   * shader.
   */
      } else {
      screen->specs.has_unified_uniforms = false;
   screen->specs.vs_uniforms_offset = VIVS_VS_UNIFORMS(0);
               screen->specs.max_texture_size =
         screen->specs.max_rendertarget_size =
            screen->specs.single_buffer = VIV_FEATURE(screen, chipMinorFeatures4, SINGLE_BUFFER);
   if (screen->specs.single_buffer)
            screen->specs.tex_astc = VIV_FEATURE(screen, chipMinorFeatures4, TEXTURE_ASTC) &&
                     /* Only allow fast clear with MC2.0 or MMUv2, as the TS unit bypasses the
   * memory offset for the MMUv1 linear window on MC1.0 and we have no way to
   * fixup the address.
   */
   if (!VIV_FEATURE(screen, chipMinorFeatures0, MC20) &&
      !VIV_FEATURE(screen, chipMinorFeatures1, MMU_VERSION))
               fail:
         }
      struct etna_bo *
   etna_screen_bo_from_handle(struct pipe_screen *pscreen,
         {
      struct etna_screen *screen = etna_screen(pscreen);
            if (whandle->type == WINSYS_HANDLE_TYPE_SHARED) {
         } else if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
         } else {
      DBG("Attempt to import unsupported handle type %d", whandle->type);
               if (!bo) {
      DBG("ref name 0x%08x failed", whandle->handle);
                  }
      static const void *
   etna_get_compiler_options(struct pipe_screen *pscreen,
         {
         }
      static struct disk_cache *
   etna_get_disk_shader_cache(struct pipe_screen *pscreen)
   {
      struct etna_screen *screen = etna_screen(pscreen);
               }
      static int
   etna_screen_get_fd(struct pipe_screen *pscreen)
   {
      struct etna_screen *screen = etna_screen(pscreen);
      }
      struct pipe_screen *
   etna_screen_create(struct etna_device *dev, struct etna_gpu *gpu,
         {
      struct etna_screen *screen = CALLOC_STRUCT(etna_screen);
   struct pipe_screen *pscreen;
            if (!screen)
            pscreen = &screen->base;
   screen->dev = dev;
   screen->gpu = gpu;
            screen->drm_version = etnaviv_device_version(screen->dev);
            /* Disable autodisable for correct rendering with TS */
            screen->pipe = etna_pipe_new(gpu, ETNA_PIPE_3D);
   if (!screen->pipe) {
      DBG("could not create 3d pipe");
               if (etna_gpu_get_param(screen->gpu, ETNA_GPU_MODEL, &val)) {
      DBG("could not get ETNA_GPU_MODEL");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_REVISION, &val)) {
      DBG("could not get ETNA_GPU_REVISION");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_0, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_0");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_1, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_1");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_2, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_2");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_3, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_3");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_4, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_4");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_5, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_5");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_6, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_6");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_7, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_7");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_8, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_8");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_9, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_9");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_10, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_10");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_11, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_11");
      }
            if (etna_gpu_get_param(screen->gpu, ETNA_GPU_FEATURES_12, &val)) {
      DBG("could not get ETNA_GPU_FEATURES_12");
      }
            if (!etna_get_specs(screen))
            if (screen->specs.halti >= 5 && !etnaviv_device_softpin_capable(dev)) {
      DBG("halti5 requires softpin");
               /* apply debug options that disable individual features */
   if (DBG_ENABLED(ETNA_DBG_NO_EARLY_Z))
         if (DBG_ENABLED(ETNA_DBG_NO_TS))
         if (DBG_ENABLED(ETNA_DBG_NO_AUTODISABLE))
         if (DBG_ENABLED(ETNA_DBG_NO_SUPERTILE))
         if (DBG_ENABLED(ETNA_DBG_NO_SINGLEBUF))
         if (!DBG_ENABLED(ETNA_DBG_LINEAR_PE))
            pscreen->destroy = etna_screen_destroy;
   pscreen->get_screen_fd = etna_screen_get_fd;
   pscreen->get_param = etna_screen_get_param;
   pscreen->get_paramf = etna_screen_get_paramf;
   pscreen->get_shader_param = etna_screen_get_shader_param;
   pscreen->get_compiler_options = etna_get_compiler_options;
            pscreen->get_name = etna_screen_get_name;
   pscreen->get_vendor = etna_screen_get_vendor;
            pscreen->get_timestamp = u_default_get_timestamp;
   pscreen->context_create = etna_context_create;
   pscreen->is_format_supported = etna_screen_is_format_supported;
   pscreen->query_dmabuf_modifiers = etna_screen_query_dmabuf_modifiers;
   pscreen->is_dmabuf_modifier_supported = etna_screen_is_dmabuf_modifier_supported;
            if (!etna_shader_screen_init(pscreen))
            etna_fence_screen_init(pscreen);
   etna_query_screen_init(pscreen);
            util_dynarray_init(&screen->supported_pm_queries, NULL);
            if (screen->drm_version >= ETNA_DRM_VERSION_PERFMON)
               /* create dummy RT buffer, used when rendering with no color buffer */
   screen->dummy_rt_reloc.bo = etna_bo_new(screen->dev, 64 * 64 * 4,
         if (!screen->dummy_rt_reloc.bo)
            screen->dummy_rt_reloc.offset = 0;
            if (screen->specs.halti >= 5) {
               /* create an empty dummy texture descriptor */
   screen->dummy_desc_reloc.bo = etna_bo_new(screen->dev, 0x100,
         if (!screen->dummy_desc_reloc.bo)
            buf = etna_bo_map(screen->dummy_desc_reloc.bo);
   etna_bo_cpu_prep(screen->dummy_desc_reloc.bo, DRM_ETNA_PREP_WRITE);
   memset(buf, 0, 0x100);
   etna_bo_cpu_fini(screen->dummy_desc_reloc.bo);
   screen->dummy_desc_reloc.offset = 0;
                     fail:
      etna_screen_destroy(pscreen);
      }
