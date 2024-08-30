   /*
   * Copyright 2014, 2015 Red Hat.
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
   #include "util/u_memory.h"
   #include "util/format/u_format.h"
   #include "util/format/u_format_s3tc.h"
   #include "util/u_screen.h"
   #include "util/u_video.h"
   #include "util/u_math.h"
   #include "util/u_inlines.h"
   #include "util/os_time.h"
   #include "util/xmlconfig.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "nir/nir_to_tgsi.h"
   #include "vl/vl_decoder.h"
   #include "vl/vl_video_buffer.h"
      #include "virgl_screen.h"
   #include "virgl_resource.h"
   #include "virgl_public.h"
   #include "virgl_context.h"
   #include "virgl_encode.h"
      int virgl_debug = 0;
   const struct debug_named_value virgl_debug_options[] = {
      { "verbose",         VIRGL_DEBUG_VERBOSE,                 NULL },
   { "tgsi",            VIRGL_DEBUG_TGSI,                    NULL },
   { "noemubgra",       VIRGL_DEBUG_NO_EMULATE_BGRA,         "Disable tweak to emulate BGRA as RGBA on GLES hosts" },
   { "nobgraswz",       VIRGL_DEBUG_NO_BGRA_DEST_SWIZZLE,    "Disable tweak to swizzle emulated BGRA on GLES hosts" },
   { "sync",            VIRGL_DEBUG_SYNC,                    "Sync after every flush" },
   { "xfer",            VIRGL_DEBUG_XFER,                    "Do not optimize for transfers" },
   { "r8srgb-readback", VIRGL_DEBUG_L8_SRGB_ENABLE_READBACK, "Enable redaback for L8 sRGB textures" },
   { "nocoherent",      VIRGL_DEBUG_NO_COHERENT,             "Disable coherent memory" },
   { "video",           VIRGL_DEBUG_VIDEO,                   "Video codec" },
   { "shader_sync",     VIRGL_DEBUG_SHADER_SYNC,             "Sync after every shader link" },
      };
   DEBUG_GET_ONCE_FLAGS_OPTION(virgl_debug, "VIRGL_DEBUG", virgl_debug_options, 0)
      static const char *
   virgl_get_vendor(struct pipe_screen *screen)
   {
         }
         static const char *
   virgl_get_name(struct pipe_screen *screen)
   {
      struct virgl_screen *vscreen = virgl_screen(screen);
   if (vscreen->caps.caps.v2.host_feature_check_version >= 5)
               }
      static int
   virgl_get_param(struct pipe_screen *screen, enum pipe_cap param)
   {
      struct virgl_screen *vscreen = virgl_screen(screen);
   switch (param) {
   case PIPE_CAP_NPOT_TEXTURES:
         case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
         case PIPE_CAP_ANISOTROPIC_FILTER:
         case PIPE_CAP_MAX_RENDER_TARGETS:
         case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
         case PIPE_CAP_OCCLUSION_QUERY:
         case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
      if (vscreen->caps.caps.v2.host_feature_check_version >= 20)
            case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
      if (vscreen->caps.caps.v2.host_feature_check_version >= 22)
         return vscreen->caps.caps.v1.bset.mirror_clamp &&
      case PIPE_CAP_TEXTURE_SWIZZLE:
         case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
      if (vscreen->caps.caps.v2.max_texture_2d_size)
            case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      if (vscreen->caps.caps.v2.max_texture_3d_size)
            case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      if (vscreen->caps.caps.v2.max_texture_cube_size)
            case PIPE_CAP_BLEND_EQUATION_SEPARATE:
         case PIPE_CAP_INDEP_BLEND_ENABLE:
         case PIPE_CAP_INDEP_BLEND_FUNC:
         case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_INTEGER:
         case PIPE_CAP_FS_COORD_ORIGIN_LOWER_LEFT:
         case PIPE_CAP_DEPTH_CLIP_DISABLE:
      if (vscreen->caps.caps.v1.bset.depth_clip_disable)
            case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
         case PIPE_CAP_SUPPORTED_PRIM_MODES:
      return BITFIELD_MASK(MESA_PRIM_COUNT) &
            case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
         case PIPE_CAP_SHADER_STENCIL_EXPORT:
         case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
         case PIPE_CAP_SEAMLESS_CUBE_MAP:
         case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
         case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
         case PIPE_CAP_MIN_TEXEL_OFFSET:
         case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_MAX_TEXEL_OFFSET:
         case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_CONDITIONAL_RENDER:
         case PIPE_CAP_TEXTURE_BARRIER:
         case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
         case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
   case PIPE_CAP_VERTEX_COLOR_CLAMPED:
         case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
      return (vscreen->caps.caps.v2.capability_bits & VIRGL_CAP_FBO_MIXED_COLOR_FORMATS) ||
      case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
      if (vscreen->caps.caps.v2.host_feature_check_version < 6)
            case PIPE_CAP_GLSL_FEATURE_LEVEL:
         case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
         case PIPE_CAP_DEPTH_CLIP_DISABLE_SEPARATE:
         case PIPE_CAP_COMPUTE:
         case PIPE_CAP_USER_VERTEX_BUFFERS:
         case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_STREAM_OUTPUT_INTERLEAVE_BUFFERS:
         case PIPE_CAP_START_INSTANCE:
         case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
   case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_TEXTURE_TRANSFER_MODES:
   case PIPE_CAP_NIR_IMAGES_AS_DEREF:
         case PIPE_CAP_QUERY_TIMESTAMP:
   case PIPE_CAP_QUERY_TIME_ELAPSED:
      if (vscreen->caps.caps.v2.host_feature_check_version >= 15)
            case PIPE_CAP_TGSI_TEXCOORD:
         case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
         case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
         case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_BUFFER_SAMPLER_VIEW_RGBA_ONLY:
         case PIPE_CAP_CUBE_MAP_ARRAY:
         case PIPE_CAP_TEXTURE_MULTISAMPLE:
         case PIPE_CAP_MAX_VIEWPORTS:
         case PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT:
         case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
   case PIPE_CAP_ENDIANNESS:
         case PIPE_CAP_QUERY_PIPELINE_STATISTICS:
         case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
         case PIPE_CAP_VS_LAYER_VIEWPORT:
      return (vscreen->caps.caps.v2.capability_bits_v2 & VIRGL_CAP_V2_VS_VERTEX_LAYER) &&
      case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
         case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
         case PIPE_CAP_TEXTURE_QUERY_LOD:
         case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
         case PIPE_CAP_DRAW_INDIRECT:
         case PIPE_CAP_SAMPLE_SHADING:
   case PIPE_CAP_FORCE_PERSAMPLE_INTERP:
         case PIPE_CAP_CULL_DISTANCE:
         case PIPE_CAP_MAX_VERTEX_STREAMS:
      return ((vscreen->caps.caps.v2.capability_bits & VIRGL_CAP_TRANSFORM_FEEDBACK3) ||
      case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
         case PIPE_CAP_FS_FINE_DERIVATIVE:
         case PIPE_CAP_POLYGON_OFFSET_CLAMP:
         case PIPE_CAP_QUERY_SO_OVERFLOW:
         case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_DOUBLES:
      return vscreen->caps.caps.v1.bset.has_fp64 ||
      case PIPE_CAP_MAX_SHADER_PATCH_VARYINGS:
         case PIPE_CAP_SAMPLER_VIEW_TARGET:
         case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
         case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
         case PIPE_CAP_TEXTURE_QUERY_SAMPLES:
         case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
         case PIPE_CAP_ROBUST_BUFFER_ACCESS_BEHAVIOR:
         case PIPE_CAP_FBFETCH:
      return (vscreen->caps.caps.v2.capability_bits &
      case PIPE_CAP_BLEND_EQUATION_ADVANCED:
         case PIPE_CAP_SHADER_CLOCK:
         case PIPE_CAP_SHADER_ARRAY_COMPONENTS:
         case PIPE_CAP_MAX_COMBINED_SHADER_BUFFERS:
         case PIPE_CAP_MAX_COMBINED_HW_ATOMIC_COUNTERS:
         case PIPE_CAP_MAX_COMBINED_HW_ATOMIC_COUNTER_BUFFERS:
         case PIPE_CAP_TEXTURE_FLOAT_LINEAR:
   case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
         case PIPE_CAP_QUERY_BUFFER_OBJECT:
         case PIPE_CAP_MAX_VARYINGS:
      if (vscreen->caps.caps.v1.glsl_level < 150)
            case PIPE_CAP_FAKE_SW_MSAA:
      /* If the host supports only one sample (e.g., if it is using softpipe),
   * fake multisampling to able to advertise higher GL versions. */
      case PIPE_CAP_MULTI_DRAW_INDIRECT:
         case PIPE_CAP_MULTI_DRAW_INDIRECT_PARAMS:
         case PIPE_CAP_BUFFER_MAP_PERSISTENT_COHERENT:
      return (vscreen->caps.caps.v2.capability_bits & VIRGL_CAP_ARB_BUFFER_STORAGE) &&
            case PIPE_CAP_PCI_GROUP:
   case PIPE_CAP_PCI_BUS:
   case PIPE_CAP_PCI_DEVICE:
   case PIPE_CAP_PCI_FUNCTION:
   case PIPE_CAP_ALLOW_MAPPED_BUFFERS_DURING_EXECUTION:
         case PIPE_CAP_CLIP_HALFZ:
         case PIPE_CAP_MAX_GS_INVOCATIONS:
         case PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT:
         case PIPE_CAP_VENDOR_ID:
         case PIPE_CAP_DEVICE_ID:
         case PIPE_CAP_ACCELERATED:
         case PIPE_CAP_UMA:
   case PIPE_CAP_VIDEO_MEMORY:
      if (vscreen->caps.caps.v2.capability_bits_v2 & VIRGL_CAP_V2_VIDEO_MEMORY)
            case PIPE_CAP_TEXTURE_SHADOW_LOD:
         case PIPE_CAP_NATIVE_FENCE_FD:
         case PIPE_CAP_DEST_SURFACE_SRGB_CONTROL:
      return (vscreen->caps.caps.v2.capability_bits & VIRGL_CAP_SRGB_WRITE_CONTROL) ||
      case PIPE_CAP_SHAREABLE_SHADERS:
      /* Shader creation emits the shader through the context's command buffer
   * in virgl_encode_shader_state().
   */
      case PIPE_CAP_QUERY_MEMORY_INFO:
         case PIPE_CAP_STRING_MARKER:
         case PIPE_CAP_SURFACE_SAMPLE_COUNT:
         case PIPE_CAP_DRAW_PARAMETERS:
         case PIPE_CAP_SHADER_GROUP_VOTE:
         case PIPE_CAP_IMAGE_STORE_FORMATTED:
   case PIPE_CAP_GL_SPIRV:
         case PIPE_CAP_MAX_CONSTANT_BUFFER_SIZE_UINT:
      if (vscreen->caps.caps.v2.host_feature_check_version >= 13)
            default:
            }
      #define VIRGL_SHADER_STAGE_CAP_V2(CAP, STAGE) \
            static int
   virgl_get_shader_param(struct pipe_screen *screen,
               {
               if ((shader == PIPE_SHADER_TESS_CTRL || shader == PIPE_SHADER_TESS_EVAL) &&
      !vscreen->caps.caps.v1.bset.has_tessellation_shaders)
         if (shader == PIPE_SHADER_COMPUTE &&
                  switch(shader)
   {
   case PIPE_SHADER_FRAGMENT:
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL:
   case PIPE_SHADER_COMPUTE:
      switch (param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
      if ((vscreen->caps.caps.v2.capability_bits & VIRGL_CAP_HOST_IS_GLES) &&
      (shader == PIPE_SHADER_VERTEX)) {
      }
      case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
         case PIPE_SHADER_CAP_MAX_INPUTS:
      if (vscreen->caps.caps.v1.glsl_level < 150)
         return (shader == PIPE_SHADER_VERTEX ||
      case PIPE_SHADER_CAP_MAX_OUTPUTS:
      if (shader == PIPE_SHADER_FRAGMENT)
         // case PIPE_SHADER_CAP_MAX_CONSTS:
   //    return 4096;
      case PIPE_SHADER_CAP_MAX_TEMPS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
      //  case PIPE_SHADER_CAP_MAX_ADDRS:
   //    return 1;
      case PIPE_SHADER_CAP_SUBROUTINES:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
      return MIN2(vscreen->caps.caps.v2.max_shader_sampler_views,
      case PIPE_SHADER_CAP_INTEGERS:
         case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
      if (vscreen->caps.caps.v2.host_feature_check_version < 12)
            case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
      if (shader == PIPE_SHADER_FRAGMENT || shader == PIPE_SHADER_COMPUTE)
         else
      case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
      if (shader == PIPE_SHADER_FRAGMENT || shader == PIPE_SHADER_COMPUTE)
         else
      case PIPE_SHADER_CAP_SUPPORTED_IRS:
         case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
         case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
         default:
            default:
            }
      static int
   virgl_get_video_param(struct pipe_screen *screen,
                     {
      unsigned i;
   bool drv_supported;
   struct virgl_video_caps *vcaps = NULL;
            if (!screen)
            vscreen = virgl_screen(screen);
   if (vscreen->caps.caps.v2.num_video_caps > ARRAY_SIZE(vscreen->caps.caps.v2.video_caps))
            /* Profiles and entrypoints supported by the driver */
   switch (u_reduce_video_profile(profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC: /* fall through */
   case PIPE_VIDEO_FORMAT_HEVC:
      drv_supported = (entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM ||
            case PIPE_VIDEO_FORMAT_MPEG12:
   case PIPE_VIDEO_FORMAT_VC1:
   case PIPE_VIDEO_FORMAT_JPEG:
   case PIPE_VIDEO_FORMAT_VP9:
   case PIPE_VIDEO_FORMAT_AV1:
      drv_supported = (entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM);
      default:
      drv_supported = false;
               if (drv_supported) {
      /* Check if the device supports it, vcaps is NULL means not supported */
   for (i = 0;  i < vscreen->caps.caps.v2.num_video_caps; i++) {
      if (vscreen->caps.caps.v2.video_caps[i].profile == profile &&
         vscreen->caps.caps.v2.video_caps[i].entrypoint == entrypoint) {
   vcaps = &vscreen->caps.caps.v2.video_caps[i];
                  /*
   * Since there are calls like this:
   *   pot_buffers = !pipe->screen->get_video_param
   *   (
   *      pipe->screen,
   *      PIPE_VIDEO_PROFILE_UNKNOWN,
   *      PIPE_VIDEO_ENTRYPOINT_UNKNOWN,
   *      PIPE_VIDEO_CAP_NPOT_TEXTURES
   *   );
   * All parameters need to check the vcaps.
   */
   switch (param) {
      case PIPE_VIDEO_CAP_SUPPORTED:
         case PIPE_VIDEO_CAP_NPOT_TEXTURES:
         case PIPE_VIDEO_CAP_MAX_WIDTH:
         case PIPE_VIDEO_CAP_MAX_HEIGHT:
         case PIPE_VIDEO_CAP_PREFERED_FORMAT:
         case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
         case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
         case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
         case PIPE_VIDEO_CAP_MAX_LEVEL:
         case PIPE_VIDEO_CAP_STACKED_FRAMES:
         case PIPE_VIDEO_CAP_MAX_MACROBLOCKS:
         case PIPE_VIDEO_CAP_MAX_TEMPORAL_LAYERS:
         default:
         }
      static float
   virgl_get_paramf(struct pipe_screen *screen, enum pipe_capf param)
   {
      struct virgl_screen *vscreen = virgl_screen(screen);
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
         }
   /* should only get here on unhandled cases */
   debug_printf("Unexpected PIPE_CAPF %d query\n", param);
      }
      static int
   virgl_get_compute_param(struct pipe_screen *screen,
                     {
      struct virgl_screen *vscreen = virgl_screen(screen);
   if (!(vscreen->caps.caps.v2.capability_bits & VIRGL_CAP_COMPUTE_SHADER))
         switch (param) {
   case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
      if (ret) {
      uint64_t *grid_size = ret;
   grid_size[0] = vscreen->caps.caps.v2.max_compute_grid_size[0];
   grid_size[1] = vscreen->caps.caps.v2.max_compute_grid_size[1];
      }
      case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
      if (ret) {
      uint64_t *block_size = ret;
   block_size[0] = vscreen->caps.caps.v2.max_compute_block_size[0];
   block_size[1] = vscreen->caps.caps.v2.max_compute_block_size[1];
      }
      case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
      if (ret) {
      uint64_t *max_threads_per_block = ret;
      }
      case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE:
      if (ret) {
      uint64_t *max_local_size = ret;
   /* Value reported by the closed source driver. */
      }
      default:
         }
      }
      static bool
   has_format_bit(struct virgl_supported_format_mask *mask,
         {
      assert(fmt < VIRGL_FORMAT_MAX);
   unsigned val = (unsigned)fmt;
   unsigned idx = val / 32;
   unsigned bit = val % 32;
   assert(idx < ARRAY_SIZE(mask->bitmask));
      }
      bool
   virgl_has_readback_format(struct pipe_screen *screen,
         {
      struct virgl_screen *vscreen = virgl_screen(screen);
   if (has_format_bit(&vscreen->caps.caps.v2.supported_readback_formats,
                  if (allow_tweak && fmt == VIRGL_FORMAT_L8_SRGB && vscreen->tweak_l8_srgb_readback) {
                     }
      static bool
   virgl_is_vertex_format_supported(struct pipe_screen *screen,
         {
      struct virgl_screen *vscreen = virgl_screen(screen);
   const struct util_format_description *format_desc;
                     if (format == PIPE_FORMAT_R11G11B10_FLOAT) {
      int vformat = VIRGL_FORMAT_R11G11B10_FLOAT;
   int big = vformat / 32;
   int small = vformat % 32;
   if (!(vscreen->caps.caps.v1.vertexbuffer.bitmask[big] & (1 << small)))
                     i = util_format_get_first_non_void_channel(format);
   if (i == -1)
            if (format_desc->layout != UTIL_FORMAT_LAYOUT_PLAIN)
            if (format_desc->channel[i].type == UTIL_FORMAT_TYPE_FIXED)
            }
      static bool
   virgl_format_check_bitmask(enum pipe_format format,
               {
      enum virgl_formats vformat = pipe_to_virgl_format(format);
   int big = vformat / 32;
   int small = vformat % 32;
   if ((bitmask[big] & (1u << small)))
            /* On GLES hosts we don't advertise BGRx_SRGB, but we may be able
   * emulate it by using a swizzled RGBx */
   if (may_emulate_bgra) {
      if (format == PIPE_FORMAT_B8G8R8A8_SRGB)
         else if (format == PIPE_FORMAT_B8G8R8X8_SRGB)
         else {
                  vformat = pipe_to_virgl_format(format);
   big = vformat / 32;
   small = vformat % 32;
   if (bitmask[big] & (1 << small))
      }
      }
      bool virgl_has_scanout_format(struct virgl_screen *vscreen,
               {
      return  virgl_format_check_bitmask(format,
            }
      /**
   * Query format support for creating a texture, drawing surface, etc.
   * \param format  the format to test
   * \param type  one of PIPE_TEXTURE, PIPE_SURFACE
   */
   static bool
   virgl_is_format_supported( struct pipe_screen *screen,
                                 {
      struct virgl_screen *vscreen = virgl_screen(screen);
   const struct util_format_description *format_desc;
            union virgl_caps *caps = &vscreen->caps.caps;
   bool may_emulate_bgra = (caps->v2.capability_bits &
                  if (MAX2(1, sample_count) != MAX2(1, storage_sample_count))
            if (!util_is_power_of_two_or_zero(sample_count))
            assert(target == PIPE_BUFFER ||
         target == PIPE_TEXTURE_1D ||
   target == PIPE_TEXTURE_1D_ARRAY ||
   target == PIPE_TEXTURE_2D ||
   target == PIPE_TEXTURE_2D_ARRAY ||
   target == PIPE_TEXTURE_RECT ||
   target == PIPE_TEXTURE_3D ||
                     if (util_format_is_intensity(format))
            if (sample_count > 1) {
      if (!caps->v1.bset.texture_multisample)
            if (bind & PIPE_BIND_SHADER_IMAGE) {
      if (sample_count > caps->v2.max_image_samples)
               if (sample_count > caps->v1.max_samples)
            if (caps->v2.host_feature_check_version >= 9 &&
      !has_format_bit(&caps->v2.supported_multisample_formats,
                  if (bind & PIPE_BIND_VERTEX_BUFFER) {
                  if (util_format_is_compressed(format) && target == PIPE_BUFFER)
            /* Allow 3-comp 32 bit textures only for TBOs (needed for ARB_tbo_rgb32) */
   if ((format == PIPE_FORMAT_R32G32B32_FLOAT ||
      format == PIPE_FORMAT_R32G32B32_SINT ||
   format == PIPE_FORMAT_R32G32B32_UINT) &&
   target != PIPE_BUFFER)
         if ((format_desc->layout == UTIL_FORMAT_LAYOUT_RGTC ||
      format_desc->layout == UTIL_FORMAT_LAYOUT_ETC ||
   format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) &&
   target == PIPE_TEXTURE_3D)
            if (bind & PIPE_BIND_RENDER_TARGET) {
      /* For ARB_framebuffer_no_attachments. */
   if (format == PIPE_FORMAT_NONE)
            if (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS)
            /*
   * Although possible, it is unnatural to render into compressed or YUV
   * surfaces. So disable these here to avoid going into weird paths
   * inside gallium frontends.
   */
   if (format_desc->block.width != 1 ||
                  if (!virgl_format_check_bitmask(format,
                           if (bind & PIPE_BIND_DEPTH_STENCIL) {
      if (format_desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS)
               if (bind & PIPE_BIND_SCANOUT) {
      if (!virgl_format_check_bitmask(format, caps->v2.scanout.bitmask, false))
               /*
   * All other operations (sampling, transfer, etc).
            if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
         }
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_RGTC) {
         }
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_BPTC) {
         }
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_ETC) {
                  if (format == PIPE_FORMAT_R11G11B10_FLOAT) {
         } else if (format == PIPE_FORMAT_R9G9B9E5_FLOAT) {
                  if (format_desc->layout == UTIL_FORMAT_LAYOUT_ASTC) {
   goto out_lookup;
            i = util_format_get_first_non_void_channel(format);
   if (i == -1)
            /* no L4A4 */
   if (format_desc->nr_channels < 4 && format_desc->channel[i].size == 4)
         out_lookup:
      return virgl_format_check_bitmask(format,
            }
      static bool virgl_is_video_format_supported(struct pipe_screen *screen,
                     {
         }
         static void virgl_flush_frontbuffer(struct pipe_screen *screen,
                           {
      struct virgl_screen *vscreen = virgl_screen(screen);
   struct virgl_winsys *vws = vscreen->vws;
   struct virgl_resource *vres = virgl_resource(res);
            if (vws->flush_frontbuffer) {
      virgl_flush_eq(vctx, vctx, NULL);
   vws->flush_frontbuffer(vws, vres->hw_res, level, layer, winsys_drawable_handle,
         }
      static void virgl_fence_reference(struct pipe_screen *screen,
               {
      struct virgl_screen *vscreen = virgl_screen(screen);
               }
      static bool virgl_fence_finish(struct pipe_screen *screen,
                     {
      struct virgl_screen *vscreen = virgl_screen(screen);
   struct virgl_winsys *vws = vscreen->vws;
            if (vctx && timeout)
               }
      static int virgl_fence_get_fd(struct pipe_screen *screen,
         {
      struct virgl_screen *vscreen = virgl_screen(screen);
               }
      static void
   virgl_destroy_screen(struct pipe_screen *screen)
   {
      struct virgl_screen *vscreen = virgl_screen(screen);
                     if (vws)
                        }
      static void
   fixup_formats(union virgl_caps *caps, struct virgl_supported_format_mask *mask)
   {
      const size_t size = ARRAY_SIZE(mask->bitmask);
   for (int i = 0; i < size; ++i) {
      if (mask->bitmask[i] != 0)
               /* old protocol used; fall back to considering all sampleable formats valid
   * readback-formats
   */
   for (int i = 0; i < size; ++i)
      }
      static void virgl_query_memory_info(struct pipe_screen *screen, struct pipe_memory_info *info)
   {
      struct virgl_screen *vscreen = virgl_screen(screen);
   struct pipe_context *ctx = screen->context_create(screen, NULL, 0);
   struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_resource *res;
   struct virgl_memory_info virgl_info = {0};
   const static struct pipe_resource templ = {
      .target = PIPE_BUFFER,
   .format = PIPE_FORMAT_R8_UNORM,
   .bind = PIPE_BIND_CUSTOM,
   .width0 = sizeof(struct virgl_memory_info),
   .height0 = 1,
   .depth0 = 1,
   .array_size = 1,
   .last_level = 0,
   .nr_samples = 0,
                        virgl_encode_get_memory_info(vctx, res);
   ctx->flush(ctx, NULL, 0);
   vscreen->vws->resource_wait(vscreen->vws, res->hw_res);
            info->avail_device_memory = virgl_info.avail_device_memory;
   info->avail_staging_memory = virgl_info.avail_staging_memory;
   info->device_memory_evicted = virgl_info.device_memory_evicted;
   info->nr_device_memory_evictions = virgl_info.nr_device_memory_evictions;
   info->total_device_memory = virgl_info.total_device_memory;
            screen->resource_destroy(screen, &res->b);
      }
      static struct disk_cache *virgl_get_disk_shader_cache (struct pipe_screen *pscreen)
   {
                  }
      static void virgl_disk_cache_create(struct virgl_screen *screen)
   {
      const struct build_id_note *note =
         unsigned build_id_len = build_id_length(note);
            const uint8_t *id_sha1 = build_id_data(note);
            struct mesa_sha1 sha1_ctx;
   _mesa_sha1_init(&sha1_ctx);
            /* When we switch the host the caps might change and then we might have to
   * apply different lowering. */
            uint8_t sha1[20];
   _mesa_sha1_final(&sha1_ctx, sha1);
   char timestamp[41];
               }
      static bool
   virgl_is_dmabuf_modifier_supported(UNUSED struct pipe_screen *pscreen,
                     {
      /* Always advertise support until virgl starts checking against host
   * virglrenderer or consuming valid non-linear modifiers here.
   */
      }
      static unsigned int
   virgl_get_dmabuf_modifier_planes(UNUSED struct pipe_screen *pscreen,
               {
      /* Return the format plane count queried from pipe_format. For virgl,
   * additional aux planes are entirely resolved on the host side.
   */
      }
      static void
   fixup_renderer(union virgl_caps *caps)
   {
      if (caps->v2.host_feature_check_version < 5)
            char renderer[64];
   int renderer_len = snprintf(renderer, sizeof(renderer), "virgl (%s)",
         if (renderer_len >= 64) {
      memcpy(renderer + 59, "...)", 4);
      }
      }
      static const void *
   virgl_get_compiler_options(struct pipe_screen *pscreen,
               {
                  }
      static int
   virgl_screen_get_fd(struct pipe_screen *pscreen)
   {
      struct virgl_screen *vscreen = virgl_screen(pscreen);
            if (vws->get_fd)
         else
      }
      struct pipe_screen *
   virgl_create_screen(struct virgl_winsys *vws, const struct pipe_screen_config *config)
   {
               const char *VIRGL_GLES_EMULATE_BGRA = "gles_emulate_bgra";
   const char *VIRGL_GLES_APPLY_BGRA_DEST_SWIZZLE = "gles_apply_bgra_dest_swizzle";
   const char *VIRGL_GLES_SAMPLES_PASSED_VALUE = "gles_samples_passed_value";
   const char *VIRGL_FORMAT_L8_SRGB_ENABLE_READBACK = "format_l8_srgb_enable_readback";
            if (!screen)
                     if (config && config->options) {
      driParseConfigFiles(config->options, config->options_info, 0, "virtio_gpu",
            screen->tweak_gles_emulate_bgra =
         screen->tweak_gles_apply_bgra_dest_swizzle =
         screen->tweak_gles_tf3_value =
         screen->tweak_l8_srgb_readback =
            }
   screen->tweak_gles_emulate_bgra &= !(virgl_debug & VIRGL_DEBUG_NO_EMULATE_BGRA);
   screen->tweak_gles_apply_bgra_dest_swizzle &= !(virgl_debug & VIRGL_DEBUG_NO_BGRA_DEST_SWIZZLE);
   screen->no_coherent = virgl_debug & VIRGL_DEBUG_NO_COHERENT;
   screen->tweak_l8_srgb_readback |= !!(virgl_debug & VIRGL_DEBUG_L8_SRGB_ENABLE_READBACK);
            screen->vws = vws;
   screen->base.get_name = virgl_get_name;
   screen->base.get_vendor = virgl_get_vendor;
   screen->base.get_screen_fd = virgl_screen_get_fd;
   screen->base.get_param = virgl_get_param;
   screen->base.get_shader_param = virgl_get_shader_param;
   screen->base.get_video_param = virgl_get_video_param;
   screen->base.get_compute_param = virgl_get_compute_param;
   screen->base.get_paramf = virgl_get_paramf;
   screen->base.get_compiler_options = virgl_get_compiler_options;
   screen->base.is_format_supported = virgl_is_format_supported;
   screen->base.is_video_format_supported = virgl_is_video_format_supported;
   screen->base.destroy = virgl_destroy_screen;
   screen->base.context_create = virgl_context_create;
   screen->base.flush_frontbuffer = virgl_flush_frontbuffer;
   screen->base.get_timestamp = u_default_get_timestamp;
   screen->base.fence_reference = virgl_fence_reference;
   //screen->base.fence_signalled = virgl_fence_signalled;
   screen->base.fence_finish = virgl_fence_finish;
   screen->base.fence_get_fd = virgl_fence_get_fd;
   screen->base.query_memory_info = virgl_query_memory_info;
   screen->base.get_disk_shader_cache = virgl_get_disk_shader_cache;
   screen->base.is_dmabuf_modifier_supported = virgl_is_dmabuf_modifier_supported;
                     vws->get_caps(vws, &screen->caps);
   fixup_formats(&screen->caps.caps,
         fixup_formats(&screen->caps.caps, &screen->caps.caps.v2.scanout);
            union virgl_caps *caps = &screen->caps.caps;
   screen->tweak_gles_emulate_bgra &= !virgl_format_check_bitmask(PIPE_FORMAT_B8G8R8A8_SRGB, caps->v1.render.bitmask, false);
            /* Set up the NIR shader compiler options now that we've figured out the caps. */
   screen->compiler_options = *(nir_shader_compiler_options *)
         if (virgl_get_param(&screen->base, PIPE_CAP_DOUBLES)) {
      /* virglrenderer is missing DFLR support, so avoid turning 64-bit
   * ffract+fsub back into ffloor.
   */
   screen->compiler_options.lower_ffloor = true;
      }
   screen->compiler_options.lower_ffma32 = true;
   screen->compiler_options.fuse_ffma32 = false;
   screen->compiler_options.lower_ldexp = true;
   screen->compiler_options.lower_image_offset_to_range_base = true;
                     virgl_disk_cache_create(screen);
      }
