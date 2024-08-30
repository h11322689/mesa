   /**********************************************************
   * Copyright 2008-2023 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "git_sha1.h" /* For MESA_GIT_SHA1 */
   #include "compiler/nir/nir.h"
   #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "util/u_process.h"
   #include "util/u_screen.h"
   #include "util/u_string.h"
   #include "util/u_math.h"
      #include "svga_winsys.h"
   #include "svga_public.h"
   #include "svga_context.h"
   #include "svga_format.h"
   #include "svga_screen.h"
   #include "svga_tgsi.h"
   #include "svga_resource_texture.h"
   #include "svga_resource.h"
   #include "svga_debug.h"
      #include "svga3d_shaderdefs.h"
   #include "VGPU10ShaderTokens.h"
      /* NOTE: this constant may get moved into a svga3d*.h header file */
   #define SVGA3D_DX_MAX_RESOURCE_SIZE (128 * 1024 * 1024)
      #ifndef MESA_GIT_SHA1
   #define MESA_GIT_SHA1 "(unknown git revision)"
   #endif
      #ifdef DEBUG
   int SVGA_DEBUG = 0;
      static const struct debug_named_value svga_debug_flags[] = {
      { "dma",         DEBUG_DMA, NULL },
   { "tgsi",        DEBUG_TGSI, NULL },
   { "pipe",        DEBUG_PIPE, NULL },
   { "state",       DEBUG_STATE, NULL },
   { "screen",      DEBUG_SCREEN, NULL },
   { "tex",         DEBUG_TEX, NULL },
   { "swtnl",       DEBUG_SWTNL, NULL },
   { "const",       DEBUG_CONSTS, NULL },
   { "viewport",    DEBUG_VIEWPORT, NULL },
   { "views",       DEBUG_VIEWS, NULL },
   { "perf",        DEBUG_PERF, NULL },
   { "flush",       DEBUG_FLUSH, NULL },
   { "sync",        DEBUG_SYNC, NULL },
   { "cache",       DEBUG_CACHE, NULL },
   { "streamout",   DEBUG_STREAMOUT, NULL },
   { "query",       DEBUG_QUERY, NULL },
   { "samplers",    DEBUG_SAMPLERS, NULL },
   { "image",       DEBUG_IMAGE, NULL },
   { "uav",         DEBUG_UAV, NULL },
   { "retry",       DEBUG_RETRY, NULL },
      };
   #endif
      static const char *
   svga_get_vendor( struct pipe_screen *pscreen )
   {
         }
         static const char *
   svga_get_name( struct pipe_screen *pscreen )
   {
      const char *build = "", *llvm = "", *mutex = "";
      #ifdef DEBUG
      /* Only return internal details in the DEBUG version:
   */
   build = "build: DEBUG;";
      #else
         #endif
   #ifdef DRAW_LLVM_AVAILABLE
         #endif
         snprintf(name, sizeof(name), "SVGA3D; %s %s %s", build, mutex, llvm);
      }
         /** Helper for querying float-valued device cap */
   static float
   get_float_cap(struct svga_winsys_screen *sws, SVGA3dDevCapIndex cap,
         {
      SVGA3dDevCapResult result;
   if (sws->get_cap(sws, cap, &result))
         else
      }
         /** Helper for querying uint-valued device cap */
   static unsigned
   get_uint_cap(struct svga_winsys_screen *sws, SVGA3dDevCapIndex cap,
         {
      SVGA3dDevCapResult result;
   if (sws->get_cap(sws, cap, &result))
         else
      }
         /** Helper for querying boolean-valued device cap */
   static bool
   get_bool_cap(struct svga_winsys_screen *sws, SVGA3dDevCapIndex cap,
         {
      SVGA3dDevCapResult result;
   if (sws->get_cap(sws, cap, &result))
         else
      }
         static float
   svga_get_paramf(struct pipe_screen *screen, enum pipe_capf param)
   {
      struct svga_screen *svgascreen = svga_screen(screen);
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
                     debug_printf("Unexpected PIPE_CAPF_ query %u\n", param);
      }
         static int
   svga_get_param(struct pipe_screen *screen, enum pipe_cap param)
   {
      struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
            switch (param) {
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
         case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
      /*
   * "In virtually every OpenGL implementation and hardware,
   * GL_MAX_DUAL_SOURCE_DRAW_BUFFERS is 1"
   * http://www.opengl.org/wiki/Blending
   */
      case PIPE_CAP_ANISOTROPIC_FILTER:
         case PIPE_CAP_MAX_RENDER_TARGETS:
         case PIPE_CAP_OCCLUSION_QUERY:
         case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
         case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_TEXTURE_SWIZZLE:
         case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
      {
      unsigned size = 1 << (SVGA_MAX_TEXTURE_LEVELS - 1);
   if (sws->get_cap(sws, SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH, &result))
         else
         if (sws->get_cap(sws, SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT, &result))
         else
                  case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      if (!sws->get_cap(sws, SVGA3D_DEVCAP_MAX_VOLUME_EXTENT, &result))
               case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
      return sws->have_sm5 ? SVGA3D_SM5_MAX_SURFACE_ARRAYSIZE :
         case PIPE_CAP_BLEND_EQUATION_SEPARATE: /* req. for GL 1.5 */
            case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
         case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
         case PIPE_CAP_FS_COORD_PIXEL_CENTER_INTEGER:
            case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
         case PIPE_CAP_VERTEX_COLOR_CLAMPED:
            case PIPE_CAP_GLSL_FEATURE_LEVEL:
   case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
      if (sws->have_gl43) {
         } else if (sws->have_sm5) {
         } else if (sws->have_vgpu10) {
         } else {
               case PIPE_CAP_TEXTURE_TRANSFER_MODES:
            case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
            case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_QUERY_TIMESTAMP:
   case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
   case PIPE_CAP_FAKE_SW_MSAA:
            case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
      return sws->have_sm5 ? SVGA3D_MAX_STREAMOUT_DECLS :
      case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
         case PIPE_CAP_STREAM_OUTPUT_INTERLEAVE_BUFFERS:
         case PIPE_CAP_TEXTURE_MULTISAMPLE:
            case PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT:
      /* convert bytes to texels for the case of the largest texel
   * size: float[4].
   */
         case PIPE_CAP_MIN_TEXEL_OFFSET:
         case PIPE_CAP_MAX_TEXEL_OFFSET:
            case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
   case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
            case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
         case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
            case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
            case PIPE_CAP_GENERATE_MIPMAP:
            case PIPE_CAP_NATIVE_FENCE_FD:
            case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
            case PIPE_CAP_CUBE_MAP_ARRAY:
   case PIPE_CAP_INDEP_BLEND_FUNC:
   case PIPE_CAP_SAMPLE_SHADING:
   case PIPE_CAP_FORCE_PERSAMPLE_INTERP:
   case PIPE_CAP_TEXTURE_QUERY_LOD:
            case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
      /* SM4_1 supports only single-channel textures where as SM5 supports
   * all four channel textures */
   return sws->have_sm5 ? 4 :
      case PIPE_CAP_DRAW_INDIRECT:
         case PIPE_CAP_MAX_VERTEX_STREAMS:
         case PIPE_CAP_COMPUTE:
         case PIPE_CAP_MAX_VARYINGS:
      /* According to the spec, max varyings does not include the components
   * for position, so remove one count from the max for position.
   */
      case PIPE_CAP_BUFFER_MAP_PERSISTENT_COHERENT:
            case PIPE_CAP_START_INSTANCE:
         case PIPE_CAP_ROBUST_BUFFER_ACCESS_BEHAVIOR:
            case PIPE_CAP_SAMPLER_VIEW_TARGET:
            case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
            case PIPE_CAP_CLIP_HALFZ:
         case PIPE_CAP_SHAREABLE_SHADERS:
            case PIPE_CAP_PCI_GROUP:
   case PIPE_CAP_PCI_BUS:
   case PIPE_CAP_PCI_DEVICE:
   case PIPE_CAP_PCI_FUNCTION:
         case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_MAX_COMBINED_SHADER_OUTPUT_RESOURCES:
   case PIPE_CAP_MAX_COMBINED_SHADER_BUFFERS:
         case PIPE_CAP_MAX_COMBINED_HW_ATOMIC_COUNTERS:
   case PIPE_CAP_MAX_COMBINED_HW_ATOMIC_COUNTER_BUFFERS:
         case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
         case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
         case PIPE_CAP_VERTEX_ATTRIB_ELEMENT_ALIGNED_ONLY:
      /* This CAP cannot be used with any other alignment-requiring CAPs */
      case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
         case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
         case PIPE_CAP_MAX_VIEWPORTS:
      assert((!sws->have_vgpu10 && svgascreen->max_viewports == 1) ||
         (sws->have_vgpu10 &&
      case PIPE_CAP_ENDIANNESS:
            case PIPE_CAP_VENDOR_ID:
         case PIPE_CAP_DEVICE_ID:
      if (sws->device_id) {
         } else {
            case PIPE_CAP_ACCELERATED:
         case PIPE_CAP_VIDEO_MEMORY:
      /* XXX: Query the host ? */
      case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
         case PIPE_CAP_DOUBLES:
         case PIPE_CAP_UMA:
   case PIPE_CAP_ALLOW_MAPPED_BUFFERS_DURING_EXECUTION:
         case PIPE_CAP_TGSI_DIV:
         case PIPE_CAP_MAX_GS_INVOCATIONS:
         case PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT:
         /* Verify this once protocol is finalized. Setting it to minimum value. */
   case PIPE_CAP_MAX_SHADER_PATCH_VARYINGS:
         case PIPE_CAP_TEXTURE_FLOAT_LINEAR:
         case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
         case PIPE_CAP_TGSI_TEXCOORD:
         case PIPE_CAP_IMAGE_STORE_FORMATTED:
         default:
            }
         static int
   vgpu9_get_shader_param(struct pipe_screen *screen,
               {
      struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
                     switch (shader)
   {
   case PIPE_SHADER_FRAGMENT:
      switch (param)
   {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
      return get_uint_cap(sws,
            case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         case PIPE_SHADER_CAP_MAX_INPUTS:
         case PIPE_SHADER_CAP_MAX_OUTPUTS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         case PIPE_SHADER_CAP_MAX_TEMPS:
      val = get_uint_cap(sws, SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS, 32);
      case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
      /*
   * Although PS 3.0 has some addressing abilities it can only represent
   * loops that can be statically determined and unrolled. Given we can
   * only handle a subset of the cases that the gallium frontend already
   * does it is better to defer loop unrolling to the gallium frontend.
   */
      case PIPE_SHADER_CAP_CONT_SUPPORTED:
         case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
         case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         case PIPE_SHADER_CAP_SUBROUTINES:
         case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_INTEGERS:
         case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
         case PIPE_SHADER_CAP_SUPPORTED_IRS:
         case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
   case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
   case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         }
   /* If we get here, we failed to handle a cap above */
   debug_printf("Unexpected fragment shader query %u\n", param);
      case PIPE_SHADER_VERTEX:
      switch (param)
   {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
      return get_uint_cap(sws, SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS,
      case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
      /* XXX: until we have vertex texture support */
      case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         case PIPE_SHADER_CAP_MAX_INPUTS:
         case PIPE_SHADER_CAP_MAX_OUTPUTS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         case PIPE_SHADER_CAP_MAX_TEMPS:
      val = get_uint_cap(sws, SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS, 32);
      case PIPE_SHADER_CAP_CONT_SUPPORTED:
         case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
         case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
         case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
         case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         case PIPE_SHADER_CAP_SUBROUTINES:
         case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_INTEGERS:
         case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
         case PIPE_SHADER_CAP_SUPPORTED_IRS:
         case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
   case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
   case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         }
   /* If we get here, we failed to handle a cap above */
   debug_printf("Unexpected vertex shader query %u\n", param);
      case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_COMPUTE:
   case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL:
      /* no support for geometry, tess or compute shaders at this time */
      case PIPE_SHADER_MESH:
   case PIPE_SHADER_TASK:
         default:
      debug_printf("Unexpected shader type (%u) query\n", shader);
      }
      }
         static int
   vgpu10_get_shader_param(struct pipe_screen *screen,
               {
      struct svga_screen *svgascreen = svga_screen(screen);
            assert(sws->have_vgpu10);
            if (shader == PIPE_SHADER_MESH || shader == PIPE_SHADER_TASK)
            if ((!sws->have_sm5) &&
      (shader == PIPE_SHADER_TESS_CTRL || shader == PIPE_SHADER_TESS_EVAL))
         if ((!sws->have_gl43) && (shader == PIPE_SHADER_COMPUTE))
                     /* Generally the same limits for vertex, geometry and fragment shaders */
   switch (param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         case PIPE_SHADER_CAP_MAX_INPUTS:
      if (shader == PIPE_SHADER_FRAGMENT)
         else if (shader == PIPE_SHADER_GEOMETRY)
         else if (shader == PIPE_SHADER_TESS_CTRL)
         else if (shader == PIPE_SHADER_TESS_EVAL)
         else
      case PIPE_SHADER_CAP_MAX_OUTPUTS:
      if (shader == PIPE_SHADER_FRAGMENT)
         else if (shader == PIPE_SHADER_GEOMETRY)
         else if (shader == PIPE_SHADER_TESS_CTRL)
         else if (shader == PIPE_SHADER_TESS_EVAL)
         else
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         case PIPE_SHADER_CAP_MAX_TEMPS:
         case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         case PIPE_SHADER_CAP_CONT_SUPPORTED:
   case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
   case PIPE_SHADER_CAP_SUBROUTINES:
   case PIPE_SHADER_CAP_INTEGERS:
         case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
         case PIPE_SHADER_CAP_SUPPORTED_IRS:
      if (sws->have_gl43)
         else
         case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
            case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
            case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
            case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
   case PIPE_SHADER_CAP_INT64_ATOMICS:
         default:
      debug_printf("Unexpected vgpu10 shader query %u\n", param);
      }
      }
      #define COMMON_OPTIONS                                                        \
      .lower_extract_byte = true,                                                \
   .lower_extract_word = true,                                                \
   .lower_insert_byte = true,                                                 \
   .lower_insert_word = true,                                                 \
   .lower_int64_options = nir_lower_imul_2x32_64 | nir_lower_divmod64,        \
   .lower_fdph = true,                                                        \
   .lower_flrp64 = true,                                                      \
   .lower_ldexp = true,                                                       \
   .lower_uniforms_to_ubo = true,                                             \
   .lower_vector_cmp = true,                                                  \
   .lower_cs_local_index_to_id = true,                                        \
   .max_unroll_iterations = 32,                                               \
         #define VGPU10_OPTIONS                                                        \
      .lower_doubles_options = nir_lower_dfloor | nir_lower_dsign | nir_lower_dceil | nir_lower_dtrunc | nir_lower_dround_even, \
   .lower_fmod = true,                                                        \
         static const nir_shader_compiler_options svga_vgpu9_fragment_compiler_options = {
      COMMON_OPTIONS,
   .lower_bitops = true,
   .force_indirect_unrolling = nir_var_all,
      };
      static const nir_shader_compiler_options svga_vgpu9_vertex_compiler_options = {
      COMMON_OPTIONS,
   .lower_bitops = true,
   .force_indirect_unrolling = nir_var_function_temp,
      };
      static const nir_shader_compiler_options svga_vgpu10_compiler_options = {
      COMMON_OPTIONS,
   VGPU10_OPTIONS,
      };
      static const nir_shader_compiler_options svga_gl4_compiler_options = {
      COMMON_OPTIONS,
      };
      static const void *
   svga_get_compiler_options(struct pipe_screen *pscreen,
               {
      struct svga_screen *svgascreen = svga_screen(pscreen);
                     if (sws->have_gl43 || sws->have_sm5)
         else if (sws->have_vgpu10)
         else {
      if (shader == PIPE_SHADER_FRAGMENT)
         else
         }
      static int
   svga_get_shader_param(struct pipe_screen *screen, enum pipe_shader_type shader,
         {
      struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
   if (sws->have_vgpu10) {
         }
   else {
            }
         static int
   svga_sm5_get_compute_param(struct pipe_screen *screen,
                     {
      ASSERTED struct svga_screen *svgascreen = svga_screen(screen);
   ASSERTED struct svga_winsys_screen *sws = svgascreen->sws;
                     switch (param) {
   case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
      iret[0] = 65535;
   iret[1] = 65535;
   iret[2] = 65535;
      case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
      iret[0] = 1024;
   iret[1] = 1024;
   iret[2] = 64;
      case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
      *iret = 1024;
      case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE:
      *iret = 32768;
      case PIPE_COMPUTE_CAP_MAX_VARIABLE_THREADS_PER_BLOCK:
      *iret = 0;
      default:
         }
      }
      static void
   svga_fence_reference(struct pipe_screen *screen,
               {
      struct svga_winsys_screen *sws = svga_screen(screen)->sws;
      }
         static bool
   svga_fence_finish(struct pipe_screen *screen,
                     {
      struct svga_winsys_screen *sws = svga_screen(screen)->sws;
                     if (!timeout) {
         }
   else {
      SVGA_DBG(DEBUG_DMA|DEBUG_PERF, "%s fence_ptr %p\n",
                                    }
         static int
   svga_fence_get_fd(struct pipe_screen *screen,
         {
                  }
         static int
   svga_get_driver_query_info(struct pipe_screen *screen,
               {
   #define QUERY(NAME, ENUM, UNITS) \
               static const struct pipe_driver_query_info queries[] = {
      /* per-frame counters */
   QUERY("num-draw-calls", SVGA_QUERY_NUM_DRAW_CALLS,
         QUERY("num-fallbacks", SVGA_QUERY_NUM_FALLBACKS,
         QUERY("num-flushes", SVGA_QUERY_NUM_FLUSHES,
         QUERY("num-validations", SVGA_QUERY_NUM_VALIDATIONS,
         QUERY("map-buffer-time", SVGA_QUERY_MAP_BUFFER_TIME,
         QUERY("num-buffers-mapped", SVGA_QUERY_NUM_BUFFERS_MAPPED,
         QUERY("num-textures-mapped", SVGA_QUERY_NUM_TEXTURES_MAPPED,
         QUERY("num-bytes-uploaded", SVGA_QUERY_NUM_BYTES_UPLOADED,
         QUERY("num-command-buffers", SVGA_QUERY_NUM_COMMAND_BUFFERS,
         QUERY("command-buffer-size", SVGA_QUERY_COMMAND_BUFFER_SIZE,
         QUERY("flush-time", SVGA_QUERY_FLUSH_TIME,
         QUERY("surface-write-flushes", SVGA_QUERY_SURFACE_WRITE_FLUSHES,
         QUERY("num-readbacks", SVGA_QUERY_NUM_READBACKS,
         QUERY("num-resource-updates", SVGA_QUERY_NUM_RESOURCE_UPDATES,
         QUERY("num-buffer-uploads", SVGA_QUERY_NUM_BUFFER_UPLOADS,
         QUERY("num-const-buf-updates", SVGA_QUERY_NUM_CONST_BUF_UPDATES,
         QUERY("num-const-updates", SVGA_QUERY_NUM_CONST_UPDATES,
         QUERY("num-shader-relocations", SVGA_QUERY_NUM_SHADER_RELOCATIONS,
         QUERY("num-surface-relocations", SVGA_QUERY_NUM_SURFACE_RELOCATIONS,
            /* running total counters */
   QUERY("memory-used", SVGA_QUERY_MEMORY_USED,
         QUERY("num-shaders", SVGA_QUERY_NUM_SHADERS,
         QUERY("num-resources", SVGA_QUERY_NUM_RESOURCES,
         QUERY("num-state-objects", SVGA_QUERY_NUM_STATE_OBJECTS,
         QUERY("num-surface-views", SVGA_QUERY_NUM_SURFACE_VIEWS,
         QUERY("num-generate-mipmap", SVGA_QUERY_NUM_GENERATE_MIPMAP,
         QUERY("num-failed-allocations", SVGA_QUERY_NUM_FAILED_ALLOCATIONS,
         QUERY("num-commands-per-draw", SVGA_QUERY_NUM_COMMANDS_PER_DRAW,
         QUERY("shader-mem-used", SVGA_QUERY_SHADER_MEM_USED,
         #undef QUERY
         if (!info)
            if (index >= ARRAY_SIZE(queries))
            *info = queries[index];
      }
         static void
   init_logging(struct pipe_screen *screen)
   {
      struct svga_screen *svgascreen = svga_screen(screen);
   static const char *log_prefix = "Mesa: ";
            /* Log Version to Host */
   snprintf(host_log, sizeof(host_log) - strlen(log_prefix),
                  snprintf(host_log, sizeof(host_log) - strlen(log_prefix),
                  /* If the SVGA_EXTRA_LOGGING env var is set, log the process's command
   * line (program name and arguments).
   */
   if (debug_get_bool_option("SVGA_EXTRA_LOGGING", false)) {
      char cmdline[1000];
   if (util_get_command_line(cmdline, sizeof(cmdline))) {
      snprintf(host_log, sizeof(host_log) - strlen(log_prefix),
                  }
         /**
   * no-op logging function to use when SVGA_NO_LOGGING is set.
   */
   static void
   nop_host_log(struct svga_winsys_screen *sws, const char *message)
   {
         }
         static void
   svga_destroy_screen( struct pipe_screen *screen )
   {
                        mtx_destroy(&svgascreen->swc_mutex);
                        }
         static int
   svga_screen_get_fd( struct pipe_screen *screen )
   {
                  }
         /**
   * Create a new svga_screen object
   */
   struct pipe_screen *
   svga_screen_create(struct svga_winsys_screen *sws)
   {
      struct svga_screen *svgascreen;
         #ifdef DEBUG
         #endif
         svgascreen = CALLOC_STRUCT(svga_screen);
   if (!svgascreen)
            svgascreen->debug.force_level_surface_view =
         svgascreen->debug.force_surface_view =
         svgascreen->debug.force_sampler_view =
         svgascreen->debug.no_surface_view =
         svgascreen->debug.no_sampler_view =
         svgascreen->debug.no_cache_index_buffers =
                     screen->destroy = svga_destroy_screen;
   screen->get_name = svga_get_name;
   screen->get_vendor = svga_get_vendor;
   screen->get_device_vendor = svga_get_vendor; // TODO actual device vendor
   screen->get_screen_fd = svga_screen_get_fd;
   screen->get_param = svga_get_param;
   screen->get_shader_param = svga_get_shader_param;
   screen->get_compiler_options = svga_get_compiler_options;
   screen->get_paramf = svga_get_paramf;
   screen->get_timestamp = NULL;
   screen->is_format_supported = svga_is_format_supported;
   screen->context_create = svga_context_create;
   screen->fence_reference = svga_fence_reference;
   screen->fence_finish = svga_fence_finish;
                                                if (sws->get_hw_version) {
         } else {
                  if (svgascreen->hw_version < SVGA3D_HWVERSION_WS8_B1) {
      /* too old for 3D acceleration */
   debug_printf("Hardware version 0x%x is too old for accerated 3D\n",
                     if (sws->have_gl43) {
      svgascreen->forcedSampleCount =
                     /* Allow a temporary environment variable to enable/disable GL43 support.
   */
   sws->have_gl43 =
            svgascreen->debug.sampler_state_mapping =
      }
   else {
      /* sampler state mapping code is only enabled with GL43
   * due to the limitation in SW Renderer. (VMware bug 2825014)
   */
               debug_printf("%s enabled\n",
               sws->have_gl43 ? "SM5+" :
            debug_printf("Mesa: %s %s (%s)\n", svga_get_name(screen),
            /*
   * The D16, D24X8, and D24S8 formats always do an implicit shadow compare
   * when sampled from, where as the DF16, DF24, and D24S8_INT do not.  So
   * we prefer the later when available.
   *
   * This mimics hardware vendors extensions for D3D depth sampling. See also
   * http://aras-p.info/texts/D3D9GPUHacks.html
            {
      bool has_df16, has_df24, has_d24s8_int;
   SVGA3dSurfaceFormatCaps caps;
   SVGA3dSurfaceFormatCaps mask;
   mask.value = 0;
   mask.zStencil = 1;
            svgascreen->depth.z16 = SVGA3D_Z_D16;
   svgascreen->depth.x8z24 = SVGA3D_Z_D24X8;
            svga_get_format_cap(svgascreen, SVGA3D_Z_DF16, &caps);
            svga_get_format_cap(svgascreen, SVGA3D_Z_DF24, &caps);
            svga_get_format_cap(svgascreen, SVGA3D_Z_D24S8_INT, &caps);
            /* XXX: We might want some other logic here.
   * Like if we only have d24s8_int we should
   * emulate the other formats with that.
   */
   if (has_df16) {
         }
   if (has_df24) {
         }
   if (has_d24s8_int) {
                     /* Query device caps
   */
   if (sws->have_vgpu10) {
      svgascreen->haveProvokingVertex
         svgascreen->haveLineSmooth = true;
   svgascreen->maxPointSize = 80.0F;
            /* Multisample samples per pixel */
   if (sws->have_sm4_1 && debug_get_bool_option("SVGA_MSAA", true)) {
      if (get_bool_cap(sws, SVGA3D_DEVCAP_MULTISAMPLE_2X, false))
         if (get_bool_cap(sws, SVGA3D_DEVCAP_MULTISAMPLE_4X, false))
               if (sws->have_sm5 && debug_get_bool_option("SVGA_MSAA", true)) {
      if (get_bool_cap(sws, SVGA3D_DEVCAP_MULTISAMPLE_8X, false))
               /* Maximum number of constant buffers */
   if (sws->have_gl43) {
         }
   else {
      svgascreen->max_const_buffers =
         svgascreen->max_const_buffers = MIN2(svgascreen->max_const_buffers,
               svgascreen->haveBlendLogicops =
                              /* Shader limits */
   if (sws->have_sm4_1) {
      svgascreen->max_vs_inputs  = VGPU10_1_MAX_VS_INPUTS;
   svgascreen->max_vs_outputs = VGPU10_1_MAX_VS_OUTPUTS;
      }
   else {
      svgascreen->max_vs_inputs  = VGPU10_MAX_VS_INPUTS;
   svgascreen->max_vs_outputs = VGPU10_MAX_VS_OUTPUTS;
         }
   else {
      /* VGPU9 */
   unsigned vs_ver = get_uint_cap(sws, SVGA3D_DEVCAP_VERTEX_SHADER_VERSION,
         unsigned fs_ver = get_uint_cap(sws, SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION,
            /* we require Shader model 3.0 or later */
   if (fs_ver < SVGA3DPSVERSION_30 || vs_ver < SVGA3DVSVERSION_30) {
                           svgascreen->haveLineSmooth =
            svgascreen->maxPointSize =
         /* Keep this to a reasonable size to avoid failures in conform/pntaa.c */
            /* The SVGA3D device always supports 4 targets at this time, regardless
   * of what querying SVGA3D_DEVCAP_MAX_RENDER_TARGETS might return.
   */
            /* Only support one constant buffer
   */
            /* No multisampling */
            /* Only one viewport */
            /* Shader limits */
   svgascreen->max_vs_inputs  = 16;
   svgascreen->max_vs_outputs = 10;
               /* common VGPU9 / VGPU10 caps */
   svgascreen->haveLineStipple =
            svgascreen->maxLineWidth =
            svgascreen->maxLineWidthAA =
            if (0) {
      debug_printf("svga: haveProvokingVertex %u\n",
         debug_printf("svga: haveLineStip %u  "
               "haveLineSmooth %u  maxLineWidth %.2f  maxLineWidthAA %.2f\n",
   debug_printf("svga: maxPointSize %g\n", svgascreen->maxPointSize);
               (void) mtx_init(&svgascreen->tex_mutex, mtx_plain);
                     if (debug_get_bool_option("SVGA_NO_LOGGING", false) == true) {
         } else {
                     error2:
         error1:
         }
         struct svga_winsys_screen *
   svga_winsys_screen(struct pipe_screen *screen)
   {
         }
         #ifdef DEBUG
   struct svga_screen *
   svga_screen(struct pipe_screen *screen)
   {
      assert(screen);
   assert(screen->destroy == svga_destroy_screen);
      }
   #endif
