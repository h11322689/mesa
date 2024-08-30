   /*
   * Copyright 2017 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir.h"
   #include "radeon_uvd_enc.h"
   #include "radeon_vce.h"
   #include "radeon_video.h"
   #include "si_pipe.h"
   #include "ac_llvm_util.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_screen.h"
   #include "util/u_video.h"
   #include "vl/vl_decoder.h"
   #include "vl/vl_video_buffer.h"
   #include <sys/utsname.h>
      /* The capabilities reported by the kernel has priority
         #define QUERYABLE_KERNEL   (!!(sscreen->info.drm_minor >= 41))
   #define KERNEL_DEC_CAP(codec, attrib)    \
      (codec > PIPE_VIDEO_FORMAT_UNKNOWN && codec <= PIPE_VIDEO_FORMAT_AV1) ? \
   (sscreen->info.dec_caps.codec_info[codec - 1].valid ? \
      #define KERNEL_ENC_CAP(codec, attrib)    \
      (codec > PIPE_VIDEO_FORMAT_UNKNOWN && codec <= PIPE_VIDEO_FORMAT_AV1) ? \
   (sscreen->info.enc_caps.codec_info[codec - 1].valid ? \
         static const char *si_get_vendor(struct pipe_screen *pscreen)
   {
         }
      static const char *si_get_device_vendor(struct pipe_screen *pscreen)
   {
         }
      static int si_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
   {
               /* Gfx8 (Polaris11) hangs, so don't enable this on Gfx8 and older chips. */
   bool enable_sparse = sscreen->info.gfx_level >= GFX9 &&
            switch (param) {
   /* Supported features (boolean caps). */
   case PIPE_CAP_ACCELERATED:
   case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
   case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_TEXTURE_SHADOW_LOD:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_DEPTH_CLIP_DISABLE_SEPARATE:
   case PIPE_CAP_SHADER_STENCIL_EXPORT:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_INTEGER:
   case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_TEXTURE_BARRIER:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_INDEP_BLEND_FUNC:
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_START_INSTANCE:
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
   case PIPE_CAP_VERTEX_COLOR_CLAMPED:
   case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
   case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_COMPUTE:
   case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
   case PIPE_CAP_VS_LAYER_VIEWPORT:
   case PIPE_CAP_QUERY_PIPELINE_STATISTICS:
   case PIPE_CAP_SAMPLE_SHADING:
   case PIPE_CAP_DRAW_INDIRECT:
   case PIPE_CAP_CLIP_HALFZ:
   case PIPE_CAP_VS_WINDOW_SPACE_POSITION:
   case PIPE_CAP_POLYGON_OFFSET_CLAMP:
   case PIPE_CAP_MULTISAMPLE_Z_RESOLVE:
   case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
   case PIPE_CAP_TGSI_TEXCOORD:
   case PIPE_CAP_FS_FINE_DERIVATIVE:
   case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
   case PIPE_CAP_TEXTURE_FLOAT_LINEAR:
   case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
   case PIPE_CAP_DEPTH_BOUNDS_TEST:
   case PIPE_CAP_SAMPLER_VIEW_TARGET:
   case PIPE_CAP_TEXTURE_QUERY_LOD:
   case PIPE_CAP_TEXTURE_GATHER_SM5:
   case PIPE_CAP_TEXTURE_QUERY_SAMPLES:
   case PIPE_CAP_FORCE_PERSAMPLE_INTERP:
   case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
   case PIPE_CAP_FS_POSITION_IS_SYSVAL:
   case PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL:
   case PIPE_CAP_INVALIDATE_BUFFER:
   case PIPE_CAP_SURFACE_REINTERPRET_BLOCKS:
   case PIPE_CAP_QUERY_BUFFER_OBJECT:
   case PIPE_CAP_QUERY_MEMORY_INFO:
   case PIPE_CAP_SHADER_PACK_HALF_FLOAT:
   case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
   case PIPE_CAP_ROBUST_BUFFER_ACCESS_BEHAVIOR:
   case PIPE_CAP_POLYGON_OFFSET_UNITS_UNSCALED:
   case PIPE_CAP_STRING_MARKER:
   case PIPE_CAP_CULL_DISTANCE:
   case PIPE_CAP_SHADER_ARRAY_COMPONENTS:
   case PIPE_CAP_SHADER_CAN_READ_OUTPUTS:
   case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_STREAM_OUTPUT_INTERLEAVE_BUFFERS:
   case PIPE_CAP_DOUBLES:
   case PIPE_CAP_TGSI_TEX_TXF_LZ:
   case PIPE_CAP_TES_LAYER_VIEWPORT:
   case PIPE_CAP_BINDLESS_TEXTURE:
   case PIPE_CAP_QUERY_TIMESTAMP:
   case PIPE_CAP_QUERY_TIME_ELAPSED:
   case PIPE_CAP_NIR_SAMPLERS_AS_DEREF:
   case PIPE_CAP_MEMOBJ:
   case PIPE_CAP_LOAD_CONSTBUF:
   case PIPE_CAP_INT64:
   case PIPE_CAP_SHADER_CLOCK:
   case PIPE_CAP_CAN_BIND_CONST_BUFFER_AS_VERTEX:
   case PIPE_CAP_ALLOW_MAPPED_BUFFERS_DURING_EXECUTION:
   case PIPE_CAP_SIGNED_VERTEX_BUFFER_OFFSET:
   case PIPE_CAP_SHADER_BALLOT:
   case PIPE_CAP_SHADER_GROUP_VOTE:
   case PIPE_CAP_FBFETCH:
   case PIPE_CAP_COMPUTE_GRID_INFO_LAST_BLOCK:
   case PIPE_CAP_IMAGE_LOAD_FORMATTED:
   case PIPE_CAP_PREFER_COMPUTE_FOR_MULTIMEDIA:
   case PIPE_CAP_TGSI_DIV:
   case PIPE_CAP_PACKED_UNIFORMS:
   case PIPE_CAP_GL_SPIRV:
   case PIPE_CAP_ALPHA_TO_COVERAGE_DITHER_CONTROL:
   case PIPE_CAP_MAP_UNSYNCHRONIZED_THREAD_SAFE:
   case PIPE_CAP_NO_CLIP_ON_COPY_TEX:
   case PIPE_CAP_SHADER_ATOMIC_INT64:
   case PIPE_CAP_FRONTEND_NOOP:
   case PIPE_CAP_DEMOTE_TO_HELPER_INVOCATION:
   case PIPE_CAP_PREFER_REAL_BUFFER_IN_CONSTBUF0:
   case PIPE_CAP_COMPUTE_SHADER_DERIVATIVES:
   case PIPE_CAP_IMAGE_ATOMIC_INC_WRAP:
   case PIPE_CAP_IMAGE_STORE_FORMATTED:
   case PIPE_CAP_ALLOW_DRAW_OUT_OF_ORDER:
   case PIPE_CAP_QUERY_SO_OVERFLOW:
   case PIPE_CAP_GLSL_TESS_LEVELS_AS_INPUTS:
   case PIPE_CAP_DEVICE_RESET_STATUS_QUERY:
   case PIPE_CAP_TEXTURE_MULTISAMPLE:
   case PIPE_CAP_ALLOW_GLTHREAD_BUFFER_SUBDATA_OPT: /* TODO: remove if it's slow */
   case PIPE_CAP_NULL_TEXTURES:
   case PIPE_CAP_HAS_CONST_BW:
            case PIPE_CAP_TEXTURE_TRANSFER_MODES:
            case PIPE_CAP_DRAW_VERTEX_STATE:
            case PIPE_CAP_SHADER_SAMPLES_IDENTICAL:
            case PIPE_CAP_GLSL_ZERO_INIT:
            case PIPE_CAP_GENERATE_MIPMAP:
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
   case PIPE_CAP_CUBE_MAP_ARRAY:
            case PIPE_CAP_POST_DEPTH_COVERAGE:
            case PIPE_CAP_GRAPHICS:
            case PIPE_CAP_RESOURCE_FROM_USER_MEMORY:
            case PIPE_CAP_DEVICE_PROTECTED_SURFACE:
            case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
            case PIPE_CAP_MAX_VERTEX_BUFFERS:
            case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
   case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
   case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
   case PIPE_CAP_MAX_VERTEX_STREAMS:
   case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
   case PIPE_CAP_MAX_WINDOW_RECTANGLES:
            case PIPE_CAP_GLSL_FEATURE_LEVEL:
   case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
            case PIPE_CAP_MAX_TEXTURE_UPLOAD_MEMORY_BUDGET:
      /* Optimal number for good TexSubImage performance on Polaris10. */
         case PIPE_CAP_GL_BEGIN_END_BUFFER_SIZE:
            case PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT: {
      unsigned max_texels =
                     /* Gfx8 and older use the size in bytes for bounds checking, and the max element size
   * is 16B. Gfx9 and newer use the VGPR index for bounds checking.
   */
   if (sscreen->info.gfx_level <= GFX8)
         else
      /* Gallium has a limitation that it can only bind UINT32_MAX bytes, not texels.
                           case PIPE_CAP_MAX_CONSTANT_BUFFER_SIZE_UINT:
   case PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT: {
      /* Return 1/4th of the heap size as the maximum because the max size is not practically
   * allocatable. Also, this can only return UINT32_MAX at most.
   */
            /* Allow max 512 MB to pass CTS with a 32-bit build. */
   if (sizeof(void*) == 4)
                        case PIPE_CAP_MAX_TEXTURE_MB:
      /* Allow 1/4th of the heap size. */
         case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_PREFER_BACK_BUFFER_REUSE:
            case PIPE_CAP_SPARSE_BUFFER_PAGE_SIZE:
            case PIPE_CAP_UMA:
   case PIPE_CAP_PREFER_IMM_ARRAYS_AS_CONSTBUF:
            case PIPE_CAP_CONTEXT_PRIORITY_MASK:
      if (!(sscreen->info.is_amdgpu && sscreen->info.drm_minor >= 22))
         return PIPE_CONTEXT_PRIORITY_LOW |
               case PIPE_CAP_FENCE_SIGNAL:
            case PIPE_CAP_CONSTBUF0_FLAGS:
            case PIPE_CAP_NATIVE_FENCE_FD:
            case PIPE_CAP_DRAW_PARAMETERS:
   case PIPE_CAP_MULTI_DRAW_INDIRECT:
   case PIPE_CAP_MULTI_DRAW_INDIRECT_PARAMS:
            case PIPE_CAP_MAX_SHADER_PATCH_VARYINGS:
            case PIPE_CAP_MAX_VARYINGS:
            case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
            /* Stream output. */
   case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
            /* Geometry shader output. */
   case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
      /* gfx9 has to report 256 to make piglit/gs-max-output pass.
   * gfx8 and earlier can do 1024.
   */
      case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
         case PIPE_CAP_MAX_GS_INVOCATIONS:
      /* Even though the hw supports more, we officially wanna expose only 32. */
         case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
            /* Texturing. */
   case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
         case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      if (!sscreen->info.has_3d_cube_border_color_mipmap)
            case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      if (!sscreen->info.has_3d_cube_border_color_mipmap)
         if (sscreen->info.gfx_level >= GFX10)
         /* textures support 8192, but layered rendering supports 2048 */
      case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
      if (sscreen->info.gfx_level >= GFX10)
         /* textures support 8192, but layered rendering supports 2048 */
         /* Sparse texture */
   case PIPE_CAP_MAX_SPARSE_TEXTURE_SIZE:
      return enable_sparse ?
      case PIPE_CAP_MAX_SPARSE_3D_TEXTURE_SIZE:
      return enable_sparse ?
      case PIPE_CAP_MAX_SPARSE_ARRAY_TEXTURE_LAYERS:
      return enable_sparse ?
      case PIPE_CAP_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS:
   case PIPE_CAP_QUERY_SPARSE_TEXTURE_RESIDENCY:
   case PIPE_CAP_CLAMP_SPARSE_TEXTURE_LOD:
            /* Viewports and render targets. */
   case PIPE_CAP_MAX_VIEWPORTS:
         case PIPE_CAP_VIEWPORT_SUBPIXEL_BITS:
   case PIPE_CAP_RASTERIZER_SUBPIXEL_BITS:
   case PIPE_CAP_MAX_RENDER_TARGETS:
         case PIPE_CAP_FRAMEBUFFER_MSAA_CONSTRAINTS:
            case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
   case PIPE_CAP_MIN_TEXEL_OFFSET:
            case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
   case PIPE_CAP_MAX_TEXEL_OFFSET:
            case PIPE_CAP_ENDIANNESS:
            case PIPE_CAP_VENDOR_ID:
         case PIPE_CAP_DEVICE_ID:
         case PIPE_CAP_VIDEO_MEMORY:
         case PIPE_CAP_PCI_GROUP:
         case PIPE_CAP_PCI_BUS:
         case PIPE_CAP_PCI_DEVICE:
         case PIPE_CAP_PCI_FUNCTION:
            case PIPE_CAP_TIMER_RESOLUTION:
      /* Conversion to nanos from cycles per millisecond */
         default:
            }
      static float si_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
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
      /* This depends on the quant mode, though the precise interactions
   * are unknown. */
      case PIPE_CAPF_MAX_POINT_SIZE:
   case PIPE_CAPF_MAX_POINT_SIZE_AA:
         case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
         case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
      /* This is the maximum value of the LOD_BIAS sampler field. */
      case PIPE_CAPF_MIN_CONSERVATIVE_RASTER_DILATE:
   case PIPE_CAPF_MAX_CONSERVATIVE_RASTER_DILATE:
   case PIPE_CAPF_CONSERVATIVE_RASTER_DILATE_GRANULARITY:
         }
      }
      static int si_get_shader_param(struct pipe_screen *pscreen, enum pipe_shader_type shader,
         {
               if (shader == PIPE_SHADER_MESH ||
      shader == PIPE_SHADER_TASK)
         switch (param) {
   /* Shader limits. */
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
   case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         case PIPE_SHADER_CAP_MAX_INPUTS:
         case PIPE_SHADER_CAP_MAX_OUTPUTS:
         case PIPE_SHADER_CAP_MAX_TEMPS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
         case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
         case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
            case PIPE_SHADER_CAP_SUPPORTED_IRS:
      if (shader == PIPE_SHADER_COMPUTE) {
      return (1 << PIPE_SHADER_IR_NATIVE) |
            }
   return (1 << PIPE_SHADER_IR_TGSI) |
         /* Supported boolean features. */
   case PIPE_SHADER_CAP_CONT_SUPPORTED:
   case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
   case PIPE_SHADER_CAP_INTEGERS:
   case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
   case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR: /* lowered in finalize_nir */
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR: /* lowered in finalize_nir */
            case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
      /* We need f16c for fast FP16 conversions in glUniform. */
   if (!util_get_cpu_caps()->has_f16c)
            case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
   case PIPE_SHADER_CAP_INT16:
            /* Unsupported boolean features. */
   case PIPE_SHADER_CAP_SUBROUTINES:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         }
      }
      static const void *si_get_compiler_options(struct pipe_screen *screen, enum pipe_shader_ir ir,
         {
               assert(ir == PIPE_SHADER_IR_NIR);
      }
      static void si_get_driver_uuid(struct pipe_screen *pscreen, char *uuid)
   {
         }
      static void si_get_device_uuid(struct pipe_screen *pscreen, char *uuid)
   {
                  }
      static const char *si_get_name(struct pipe_screen *pscreen)
   {
                  }
      static int si_get_video_param_no_video_hw(struct pipe_screen *screen, enum pipe_video_profile profile,
               {
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
         default:
            }
      static int si_get_video_param(struct pipe_screen *screen, enum pipe_video_profile profile,
         {
      struct si_screen *sscreen = (struct si_screen *)screen;
   enum pipe_video_format codec = u_reduce_video_profile(profile);
   bool fully_supported_profile = ((profile >= PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE) &&
                        if (entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
      if (!(sscreen->info.ip[AMD_IP_VCE].num_queues ||
         sscreen->info.ip[AMD_IP_UVD_ENC].num_queues ||
            return 0;
            switch (param) {
   case PIPE_VIDEO_CAP_SUPPORTED:
      return (
      /* in case it is explicitly marked as not supported by the kernel */
   ((QUERYABLE_KERNEL && fully_supported_profile) ? KERNEL_ENC_CAP(codec, valid) : 1) &&
   ((codec == PIPE_VIDEO_FORMAT_MPEG4_AVC && profile != PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH10 &&
   (sscreen->info.vcn_ip_version >= VCN_1_0_0 || si_vce_is_fw_version_supported(sscreen))) ||
   (profile == PIPE_VIDEO_PROFILE_HEVC_MAIN &&
   (sscreen->info.vcn_ip_version >= VCN_1_0_0 || si_radeon_uvd_enc_supported(sscreen))) ||
   (profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10 && sscreen->info.vcn_ip_version >= VCN_2_0_0) ||
   (sscreen->info.vcn_ip_version >= VCN_4_0_0 && sscreen->info.vcn_ip_version != VCN_4_0_3))));
   case PIPE_VIDEO_CAP_NPOT_TEXTURES:
         case PIPE_VIDEO_CAP_MIN_WIDTH:
         case PIPE_VIDEO_CAP_MIN_HEIGHT:
         case PIPE_VIDEO_CAP_MAX_WIDTH:
      if (codec != PIPE_VIDEO_FORMAT_UNKNOWN && QUERYABLE_KERNEL)
         else
      case PIPE_VIDEO_CAP_MAX_HEIGHT:
      if (codec != PIPE_VIDEO_FORMAT_UNKNOWN && QUERYABLE_KERNEL)
         else
      case PIPE_VIDEO_CAP_PREFERED_FORMAT:
      if (profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10)
         else
      case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
         case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
         case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
         case PIPE_VIDEO_CAP_STACKED_FRAMES:
         case PIPE_VIDEO_CAP_MAX_TEMPORAL_LAYERS:
      return (codec == PIPE_VIDEO_FORMAT_MPEG4_AVC &&
      case PIPE_VIDEO_CAP_ENC_QUALITY_LEVEL:
         case PIPE_VIDEO_CAP_ENC_SUPPORTS_MAX_FRAME_SIZE:
            case PIPE_VIDEO_CAP_ENC_HEVC_FEATURE_FLAGS:
      if ((sscreen->info.vcn_ip_version >= VCN_1_0_0) &&
            profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10)) {
                  pipe_features.bits.amp = PIPE_ENC_FEATURE_SUPPORTED;
   pipe_features.bits.strong_intra_smoothing = PIPE_ENC_FEATURE_SUPPORTED;
   pipe_features.bits.constrained_intra_pred = PIPE_ENC_FEATURE_SUPPORTED;
   pipe_features.bits.deblocking_filter_disable
                                       case PIPE_VIDEO_CAP_ENC_HEVC_BLOCK_SIZES:
      if (sscreen->info.vcn_ip_version >= VCN_1_0_0 &&
      (profile == PIPE_VIDEO_PROFILE_HEVC_MAIN ||
   profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10)) {
                  pipe_block_sizes.bits.log2_max_coding_tree_block_size_minus3 = 3;
   pipe_block_sizes.bits.log2_min_coding_tree_block_size_minus3 = 3;
   pipe_block_sizes.bits.log2_min_luma_coding_block_size_minus3 = 0;
                                 case PIPE_VIDEO_CAP_ENC_SUPPORTS_ASYNC_OPERATION:
            case PIPE_VIDEO_CAP_ENC_MAX_SLICES_PER_FRAME:
            case PIPE_VIDEO_CAP_ENC_SLICES_STRUCTURE:
      if (sscreen->info.vcn_ip_version >= VCN_2_0_0) {
      int value = (PIPE_VIDEO_CAP_SLICE_STRUCTURE_POWER_OF_TWO_ROWS |
                              case PIPE_VIDEO_CAP_ENC_AV1_FEATURE:
      if (sscreen->info.vcn_ip_version >= VCN_4_0_0 && sscreen->info.vcn_ip_version != VCN_4_0_3) {
                     attrib.bits.support_128x128_superblock = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   attrib.bits.support_filter_intra = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   attrib.bits.support_intra_edge_filter = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   attrib.bits.support_interintra_compound = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   attrib.bits.support_masked_compound = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   attrib.bits.support_warped_motion = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   attrib.bits.support_palette_mode = PIPE_ENC_FEATURE_SUPPORTED;
   attrib.bits.support_dual_filter = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   attrib.bits.support_jnt_comp = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   attrib.bits.support_ref_frame_mvs = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   attrib.bits.support_superres = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   attrib.bits.support_restoration = PIPE_ENC_FEATURE_NOT_SUPPORTED;
                                 case PIPE_VIDEO_CAP_ENC_AV1_FEATURE_EXT1:
      if (sscreen->info.vcn_ip_version >= VCN_4_0_0 && sscreen->info.vcn_ip_version != VCN_4_0_3) {
      union pipe_av1_enc_cap_features_ext1 attrib_ext1;
   attrib_ext1.value = 0;
   attrib_ext1.bits.interpolation_filter = PIPE_VIDEO_CAP_ENC_AV1_INTERPOLATION_FILTER_EIGHT_TAP |
                  PIPE_VIDEO_CAP_ENC_AV1_INTERPOLATION_FILTER_EIGHT_TAP_SMOOTH |
                                    case PIPE_VIDEO_CAP_ENC_AV1_FEATURE_EXT2:
      if (sscreen->info.vcn_ip_version >= VCN_4_0_0 && sscreen->info.vcn_ip_version != VCN_4_0_3) {
                  attrib_ext2.bits.tile_size_bytes_minus1 = 1;
   attrib_ext2.bits.obu_size_bytes_minus1 = 1;
   /**
      * tx_mode supported.
   * (tx_mode_support & 0x01) == 1: ONLY_4X4 is supported, 0: not.
   * (tx_mode_support & 0x02) == 1: TX_MODE_LARGEST is supported, 0: not.
   * (tx_mode_support & 0x04) == 1: TX_MODE_SELECT is supported, 0: not.
                           } else
      case PIPE_VIDEO_CAP_ENC_SUPPORTS_TILE:
      if ((sscreen->info.vcn_ip_version >= VCN_4_0_0 && sscreen->info.vcn_ip_version != VCN_4_0_3) &&
      profile == PIPE_VIDEO_PROFILE_AV1_MAIN)
      else
      case PIPE_VIDEO_CAP_EFC_SUPPORTED:
                  case PIPE_VIDEO_CAP_ENC_MAX_REFERENCES_PER_FRAME:
      if (sscreen->info.vcn_ip_version >= VCN_3_0_0) {
      int refPicList0 = 1;
   int refPicList1 = codec == PIPE_VIDEO_FORMAT_MPEG4_AVC ? 1 : 0;
                  default:
                     switch (param) {
   case PIPE_VIDEO_CAP_SUPPORTED:
      if (codec != PIPE_VIDEO_FORMAT_JPEG &&
      !(sscreen->info.ip[AMD_IP_UVD].num_queues ||
      sscreen->info.ip[AMD_IP_VCN_UNIFIED].num_queues :
   sscreen->info.ip[AMD_IP_VCN_DEC].num_queues)))
         if (QUERYABLE_KERNEL && fully_supported_profile &&
      sscreen->info.vcn_ip_version >= VCN_1_0_0)
      if (codec < PIPE_VIDEO_FORMAT_MPEG4_AVC &&
                  switch (codec) {
   case PIPE_VIDEO_FORMAT_MPEG12:
         case PIPE_VIDEO_FORMAT_MPEG4:
         case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      if ((sscreen->info.family == CHIP_POLARIS10 || sscreen->info.family == CHIP_POLARIS11) &&
      sscreen->info.uvd_fw_version < UVD_FW_1_66_16) {
   RVID_ERR("POLARIS10/11 firmware version need to be updated.\n");
      }
      case PIPE_VIDEO_FORMAT_VC1:
         case PIPE_VIDEO_FORMAT_HEVC:
      /* Carrizo only supports HEVC Main */
   if (sscreen->info.family >= CHIP_STONEY)
      return (profile == PIPE_VIDEO_PROFILE_HEVC_MAIN ||
      else if (sscreen->info.family >= CHIP_CARRIZO)
            case PIPE_VIDEO_FORMAT_JPEG:
      if (sscreen->info.vcn_ip_version >= VCN_1_0_0) {
      if (!sscreen->info.ip[AMD_IP_VCN_JPEG].num_queues)
         else
      }
   if (sscreen->info.family < CHIP_CARRIZO || sscreen->info.family >= CHIP_VEGA10)
         if (!(sscreen->info.is_amdgpu && sscreen->info.drm_minor >= 19)) {
      RVID_ERR("No MJPEG support for the kernel version\n");
      }
      case PIPE_VIDEO_FORMAT_VP9:
         case PIPE_VIDEO_FORMAT_AV1:
         default:
            case PIPE_VIDEO_CAP_NPOT_TEXTURES:
         case PIPE_VIDEO_CAP_MIN_WIDTH:
   case PIPE_VIDEO_CAP_MIN_HEIGHT:
         case PIPE_VIDEO_CAP_MAX_WIDTH:
      if (codec != PIPE_VIDEO_FORMAT_UNKNOWN && QUERYABLE_KERNEL)
         else {
      switch (codec) {
   case PIPE_VIDEO_FORMAT_HEVC:
   case PIPE_VIDEO_FORMAT_VP9:
   case PIPE_VIDEO_FORMAT_AV1:
      return (sscreen->info.vcn_ip_version < VCN_2_0_0) ?
      default:
               case PIPE_VIDEO_CAP_MAX_HEIGHT:
      if (codec != PIPE_VIDEO_FORMAT_UNKNOWN && QUERYABLE_KERNEL)
         else {
      switch (codec) {
   case PIPE_VIDEO_FORMAT_HEVC:
   case PIPE_VIDEO_FORMAT_VP9:
   case PIPE_VIDEO_FORMAT_AV1:
      return (sscreen->info.vcn_ip_version < VCN_2_0_0) ?
      default:
               case PIPE_VIDEO_CAP_PREFERED_FORMAT:
      if (profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10)
         else if (profile == PIPE_VIDEO_PROFILE_VP9_PROFILE2)
         else
         case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
         case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED: {
               if (format >= PIPE_VIDEO_FORMAT_HEVC)
            }
   case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
         case PIPE_VIDEO_CAP_MAX_LEVEL:
      if ((profile == PIPE_VIDEO_PROFILE_MPEG2_SIMPLE ||
      profile == PIPE_VIDEO_PROFILE_MPEG2_MAIN ||
   profile == PIPE_VIDEO_PROFILE_MPEG4_ADVANCED_SIMPLE ||
   profile == PIPE_VIDEO_PROFILE_VC1_ADVANCED) &&
   sscreen->info.dec_caps.codec_info[codec - 1].valid) {
      } else {
      switch (profile) {
   case PIPE_VIDEO_PROFILE_MPEG1:
         case PIPE_VIDEO_PROFILE_MPEG2_SIMPLE:
   case PIPE_VIDEO_PROFILE_MPEG2_MAIN:
         case PIPE_VIDEO_PROFILE_MPEG4_SIMPLE:
         case PIPE_VIDEO_PROFILE_MPEG4_ADVANCED_SIMPLE:
         case PIPE_VIDEO_PROFILE_VC1_SIMPLE:
         case PIPE_VIDEO_PROFILE_VC1_MAIN:
         case PIPE_VIDEO_PROFILE_VC1_ADVANCED:
         case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
         case PIPE_VIDEO_PROFILE_HEVC_MAIN:
   case PIPE_VIDEO_PROFILE_HEVC_MAIN_10:
         default:
               case PIPE_VIDEO_CAP_SUPPORTS_CONTIGUOUS_PLANES_MAP:
         default:
            }
      static bool si_vid_is_format_supported(struct pipe_screen *screen, enum pipe_format format,
               {
               /* HEVC 10 bit decoding should use P010 instead of NV12 if possible */
   if (profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10)
      return (format == PIPE_FORMAT_NV12) || (format == PIPE_FORMAT_P010) ||
         /* Vp9 profile 2 supports 10 bit decoding using P016 */
   if (profile == PIPE_VIDEO_PROFILE_VP9_PROFILE2)
            if (profile == PIPE_VIDEO_PROFILE_AV1_MAIN && entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM)
      return (format == PIPE_FORMAT_P010) || (format == PIPE_FORMAT_P016) ||
         /* JPEG supports YUV400 and YUV444 */
   if (profile == PIPE_VIDEO_PROFILE_JPEG_BASELINE) {
      switch (format) {
   case PIPE_FORMAT_NV12:
   case PIPE_FORMAT_YUYV:
   case PIPE_FORMAT_L8_UNORM:
   case PIPE_FORMAT_Y8_400_UNORM:
         case PIPE_FORMAT_Y8_U8_V8_444_UNORM:
      if (sscreen->info.vcn_ip_version >= VCN_2_0_0)
         else
      case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_R8_G8_B8_UNORM:
      if (sscreen->info.vcn_ip_version == VCN_4_0_3)
         else
      default:
                     if ((entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) &&
         (((profile == PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH) &&
   (sscreen->info.vcn_ip_version >= VCN_2_0_0)) ||
   ((profile == PIPE_VIDEO_PROFILE_AV1_MAIN) &&
   (sscreen->info.vcn_ip_version >= VCN_4_0_0 &&
               /* we can only handle this one with UVD */
   if (profile != PIPE_VIDEO_PROFILE_UNKNOWN)
               }
      static unsigned get_max_threads_per_block(struct si_screen *screen, enum pipe_shader_ir ir_type)
   {
      if (ir_type == PIPE_SHADER_IR_NATIVE)
            /* LLVM only supports 1024 threads per block. */
      }
      static int si_get_compute_param(struct pipe_screen *screen, enum pipe_shader_ir ir_type,
         {
               // TODO: select these params by asic
   switch (param) {
   case PIPE_COMPUTE_CAP_IR_TARGET: {
               triple = "amdgcn-mesa-mesa3d";
   gpu = ac_get_llvm_processor_name(sscreen->info.family);
   if (ret) {
         }
   /* +2 for dash and terminating NIL byte */
      }
   case PIPE_COMPUTE_CAP_GRID_DIMENSION:
      if (ret) {
      uint64_t *grid_dimension = ret;
      }
         case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
      if (ret) {
      uint64_t *grid_size = ret;
   /* Use this size, so that internal counters don't overflow 64 bits. */
   grid_size[0] = UINT32_MAX;
   grid_size[1] = UINT16_MAX;
      }
         case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
      if (ret) {
      uint64_t *block_size = ret;
   unsigned threads_per_block = get_max_threads_per_block(sscreen, ir_type);
   block_size[0] = threads_per_block;
   block_size[1] = threads_per_block;
      }
         case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
      if (ret) {
      uint64_t *max_threads_per_block = ret;
      }
      case PIPE_COMPUTE_CAP_ADDRESS_BITS:
      if (ret) {
      uint32_t *address_bits = ret;
      }
         case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE:
      if (ret) {
                                    /* In OpenCL, the MAX_MEM_ALLOC_SIZE must be at least
   * 1/4 of the MAX_GLOBAL_SIZE.  Since the
   * MAX_MEM_ALLOC_SIZE is fixed for older kernels,
   * make sure we never report more than
   * 4 * MAX_MEM_ALLOC_SIZE.
   */
   *max_global_size =
      }
         case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE:
      if (ret) {
      uint64_t *max_local_size = ret;
   /* Value reported by the closed source driver. */
   if (sscreen->info.gfx_level == GFX6)
         else
      }
         case PIPE_COMPUTE_CAP_MAX_INPUT_SIZE:
      if (ret) {
      uint64_t *max_input_size = ret;
   /* Value reported by the closed source driver. */
      }
         case PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE:
      if (ret) {
               /* Return 1/4 of the heap size as the maximum because the max size is not practically
   * allocatable.
   */
      }
         case PIPE_COMPUTE_CAP_MAX_CLOCK_FREQUENCY:
      if (ret) {
      uint32_t *max_clock_frequency = ret;
      }
         case PIPE_COMPUTE_CAP_MAX_COMPUTE_UNITS:
      if (ret) {
      uint32_t *max_compute_units = ret;
      }
         case PIPE_COMPUTE_CAP_IMAGES_SUPPORTED:
      if (ret) {
      uint32_t *images_supported = ret;
      }
      case PIPE_COMPUTE_CAP_MAX_PRIVATE_SIZE:
         case PIPE_COMPUTE_CAP_MAX_SUBGROUPS: {
      if (ret) {
      uint32_t *max_subgroups = ret;
                  if (sscreen->debug_flags & DBG(W64_CS) || sscreen->info.gfx_level < GFX10)
                           }
      }
   case PIPE_COMPUTE_CAP_SUBGROUP_SIZES:
      if (ret) {
      uint32_t *subgroup_size = ret;
   if (sscreen->debug_flags & DBG(W32_CS))
         else if (sscreen->debug_flags & DBG(W64_CS))
         else
      }
      case PIPE_COMPUTE_CAP_MAX_VARIABLE_THREADS_PER_BLOCK:
      if (ret) {
      uint64_t *max_variable_threads_per_block = ret;
   if (ir_type == PIPE_SHADER_IR_NATIVE)
         else
      }
               fprintf(stderr, "unknown PIPE_COMPUTE_CAP %d\n", param);
      }
      static uint64_t si_get_timestamp(struct pipe_screen *screen)
   {
               return 1000000 * sscreen->ws->query_value(sscreen->ws, RADEON_TIMESTAMP) /
      }
      static void si_query_memory_info(struct pipe_screen *screen, struct pipe_memory_info *info)
   {
      struct si_screen *sscreen = (struct si_screen *)screen;
   struct radeon_winsys *ws = sscreen->ws;
            info->total_device_memory = sscreen->info.vram_size_kb;
            /* The real TTM memory usage is somewhat random, because:
   *
   * 1) TTM delays freeing memory, because it can only free it after
   *    fences expire.
   *
   * 2) The memory usage can be really low if big VRAM evictions are
   *    taking place, but the real usage is well above the size of VRAM.
   *
   * Instead, return statistics of this process.
   */
   vram_usage = ws->query_value(ws, RADEON_VRAM_USAGE) / 1024;
            info->avail_device_memory =
         info->avail_staging_memory =
                     if (sscreen->info.is_amdgpu)
         else
      /* Just return the number of evicted 64KB pages. */
   }
      static struct disk_cache *si_get_disk_shader_cache(struct pipe_screen *pscreen)
   {
                  }
      static void si_init_renderer_string(struct si_screen *sscreen)
   {
      char first_name[256], second_name[32] = {}, kernel_version[128] = {};
            snprintf(first_name, sizeof(first_name), "%s",
                  if (uname(&uname_data) == 0)
            snprintf(sscreen->renderer_string, sizeof(sscreen->renderer_string),
            }
      static int si_get_screen_fd(struct pipe_screen *screen)
   {
      struct si_screen *sscreen = (struct si_screen *)screen;
               }
      void si_init_screen_get_functions(struct si_screen *sscreen)
   {
      sscreen->b.get_name = si_get_name;
   sscreen->b.get_vendor = si_get_vendor;
   sscreen->b.get_device_vendor = si_get_device_vendor;
   sscreen->b.get_screen_fd = si_get_screen_fd;
   sscreen->b.get_param = si_get_param;
   sscreen->b.get_paramf = si_get_paramf;
   sscreen->b.get_compute_param = si_get_compute_param;
   sscreen->b.get_timestamp = si_get_timestamp;
   sscreen->b.get_shader_param = si_get_shader_param;
   sscreen->b.get_compiler_options = si_get_compiler_options;
   sscreen->b.get_device_uuid = si_get_device_uuid;
   sscreen->b.get_driver_uuid = si_get_driver_uuid;
   sscreen->b.query_memory_info = si_query_memory_info;
            if (sscreen->info.ip[AMD_IP_UVD].num_queues ||
      sscreen->info.ip[AMD_IP_VCN_UNIFIED].num_queues : sscreen->info.ip[AMD_IP_VCN_DEC].num_queues) ||
         sscreen->info.ip[AMD_IP_VCN_JPEG].num_queues || sscreen->info.ip[AMD_IP_VCE].num_queues ||
   sscreen->info.ip[AMD_IP_UVD_ENC].num_queues || sscreen->info.ip[AMD_IP_VCN_ENC].num_queues) {
   sscreen->b.get_video_param = si_get_video_param;
      } else {
      sscreen->b.get_video_param = si_get_video_param_no_video_hw;
                        bool use_fma32 =
      sscreen->info.gfx_level >= GFX10_3 ||
   (sscreen->info.family >= CHIP_GFX940 && !sscreen->info.has_graphics) ||
   /* fma32 is too slow for gpu < gfx9, so apply the option only for gpu >= gfx9 */
         const struct nir_shader_compiler_options nir_options = {
      .vertex_id_zero_based = true,
   .lower_scmp = true,
   .lower_flrp16 = true,
   .lower_flrp32 = true,
   .lower_flrp64 = true,
   .lower_fdiv = true,
   .lower_bitfield_insert = true,
   .lower_bitfield_extract = true,
   /*        |---------------------------------- Performance & Availability --------------------------------|
   *        |MAD/MAC/MADAK/MADMK|MAD_LEGACY|MAC_LEGACY|    FMA     |FMAC/FMAAK/FMAMK|FMA_LEGACY|PK_FMA_F16,|Best choice
   * Arch   |    F32,F16,F64    | F32,F16  | F32,F16  |F32,F16,F64 |    F32,F16     | F32,F16  |PK_FMAC_F16|F16,F32,F64
   * ------------------------------------------------------------------------------------------------------------------
   * gfx6,7 |     1 , - , -     |  1 , -   |  1 , -   |1/4, - ,1/16|     - , -      |  - , -   |   - , -   | - ,MAD,FMA
   * gfx8   |     1 , 1 , -     |  1 , -   |  - , -   |1/4, 1 ,1/16|     - , -      |  - , -   |   - , -   |MAD,MAD,FMA
   * gfx9   |     1 ,1|0, -     |  1 , -   |  - , -   | 1 , 1 ,1/16|    0|1, -      |  - , 1   |   2 , -   |FMA,MAD,FMA
   * gfx10  |     1 , - , -     |  1 , -   |  1 , -   | 1 , 1 ,1/16|     1 , 1      |  - , -   |   2 , 2   |FMA,MAD,FMA
   * gfx10.3|     - , - , -     |  - , -   |  - , -   | 1 , 1 ,1/16|     1 , 1      |  1 , -   |   2 , 2   |  all FMA
   *
   * Tahiti, Hawaii, Carrizo, Vega20: FMA_F32 is full rate, FMA_F64 is 1/4
   * gfx9 supports MAD_F16 only on Vega10, Raven, Raven2, Renoir.
   * gfx9 supports FMAC_F32 only on Vega20, but doesn't support FMAAK and FMAMK.
   *
   * gfx8 prefers MAD for F16 because of MAC/MADAK/MADMK.
   * gfx9 and newer prefer FMA for F16 because of the packed instruction.
   * gfx10 and older prefer MAD for F32 because of the legacy instruction.
   */
   .lower_ffma16 = sscreen->info.gfx_level < GFX9,
   .lower_ffma32 = !use_fma32,
   .lower_ffma64 = false,
   .fuse_ffma16 = sscreen->info.gfx_level >= GFX9,
   .fuse_ffma32 = use_fma32,
   .fuse_ffma64 = true,
   .lower_fmod = true,
   .lower_fpow = true,
   .lower_ineg = true,
   .lower_pack_snorm_4x8 = true,
   .lower_pack_unorm_4x8 = true,
   .lower_pack_half_2x16 = true,
   .lower_pack_64_2x32 = true,
   .lower_pack_64_4x16 = true,
   .lower_pack_32_2x16 = true,
   .lower_unpack_snorm_2x16 = true,
   .lower_unpack_snorm_4x8 = true,
   .lower_unpack_unorm_2x16 = true,
   .lower_unpack_unorm_4x8 = true,
   .lower_unpack_half_2x16 = true,
   .lower_extract_byte = true,
   .lower_extract_word = true,
   .lower_insert_byte = true,
   .lower_insert_word = true,
   .lower_hadd = true,
   .lower_hadd64 = true,
   .lower_fisnormal = true,
   .lower_to_scalar = true,
   .lower_to_scalar_filter = sscreen->info.has_packed_math_16bit ?
         .has_sdot_4x8 = sscreen->info.has_accelerated_dot_product,
   .has_sudot_4x8 = sscreen->info.has_accelerated_dot_product && sscreen->info.gfx_level >= GFX11,
   .has_udot_4x8 = sscreen->info.has_accelerated_dot_product,
   .has_dot_2x16 = sscreen->info.has_accelerated_dot_product && sscreen->info.gfx_level < GFX11,
   .has_bfe = true,
   .has_bfm = true,
   .has_bitfield_select = true,
   .optimize_sample_mask_in = true,
   .max_unroll_iterations = 128,
   .max_unroll_iterations_aggressive = 128,
   .use_interpolated_input_intrinsics = true,
   .lower_uniforms_to_ubo = true,
   .support_16bit_alu = sscreen->info.gfx_level >= GFX8,
   .vectorize_vec2_16bit = sscreen->info.has_packed_math_16bit,
   .pack_varying_options =
      nir_pack_varying_interp_mode_none |
   nir_pack_varying_interp_mode_smooth |
   nir_pack_varying_interp_mode_noperspective |
   nir_pack_varying_interp_loc_center |
   nir_pack_varying_interp_loc_sample |
      .lower_io_variables = true,
   /* HW supports indirect indexing for: | Enabled in driver
   * -------------------------------------------------------
   * TCS inputs                         | Yes
   * TES inputs                         | Yes
   * GS inputs                          | No
   * -------------------------------------------------------
   * VS outputs before TCS              | No
   * TCS outputs                        | Yes
   * VS/TES outputs before GS           | No
   */
   .support_indirect_inputs = BITFIELD_BIT(MESA_SHADER_TESS_CTRL) |
         .support_indirect_outputs = BITFIELD_BIT(MESA_SHADER_TESS_CTRL),
   .lower_int64_options =
      nir_lower_imul64 | nir_lower_imul_high64 | nir_lower_imul_2x32_64 |
   nir_lower_divmod64 | nir_lower_minmax64 | nir_lower_iabs64 |
   };
      }
