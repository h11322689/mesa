   /*
   * Copyright © 2014-2017 Broadcom
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <sys/sysinfo.h>
      #include "common/v3d_device_info.h"
   #include "common/v3d_limits.h"
   #include "util/os_misc.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
      #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/format/u_format.h"
   #include "util/u_hash_table.h"
   #include "util/u_screen.h"
   #include "util/u_transfer_helper.h"
   #include "util/ralloc.h"
   #include "util/xmlconfig.h"
      #include <xf86drm.h>
   #include "v3d_screen.h"
   #include "v3d_context.h"
   #include "v3d_resource.h"
   #include "compiler/v3d_compiler.h"
   #include "drm-uapi/drm_fourcc.h"
      static const char *
   v3d_screen_get_name(struct pipe_screen *pscreen)
   {
                  if (!screen->name) {
            screen->name = ralloc_asprintf(screen,
                     }
      static const char *
   v3d_screen_get_vendor(struct pipe_screen *pscreen)
   {
         }
      static void
   v3d_screen_destroy(struct pipe_screen *pscreen)
   {
                  _mesa_hash_table_destroy(screen->bo_handles, NULL);
   v3d_bufmgr_destroy(pscreen);
   slab_destroy_parent(&screen->transfer_pool);
   if (screen->ro)
            if (using_v3d_simulator)
               #ifdef ENABLE_SHADER_CACHE
         if (screen->disk_cache)
   #endif
                     close(screen->fd);
   }
      static bool
   v3d_has_feature(struct v3d_screen *screen, enum drm_v3d_param feature)
   {
         struct drm_v3d_get_param p = {
         };
            if (ret != 0)
            }
      static int
   v3d_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
   {
                  switch (param) {
         case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_TEXTURE_MULTISAMPLE:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_START_INSTANCE:
   case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
   case PIPE_CAP_EMULATE_NONFIXED_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_DRAW_INDIRECT:
   case PIPE_CAP_MULTI_DRAW_INDIRECT:
   case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
   case PIPE_CAP_SIGNED_VERTEX_BUFFER_OFFSET:
   case PIPE_CAP_SHADER_CAN_READ_OUTPUTS:
   case PIPE_CAP_SHADER_PACK_HALF_FLOAT:
   case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
   case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
   case PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL:
   case PIPE_CAP_TGSI_TEXCOORD:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
   case PIPE_CAP_SAMPLER_VIEW_TARGET:
   case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
   case PIPE_CAP_INDEP_BLEND_FUNC:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
   case PIPE_CAP_CUBE_MAP_ARRAY:
   case PIPE_CAP_NIR_COMPACT_ARRAYS:
            case PIPE_CAP_POLYGON_OFFSET_CLAMP:
               case PIPE_CAP_TEXTURE_QUERY_LOD:
                  case PIPE_CAP_PACKED_UNIFORMS:
            /* We can't enable this flag, because it results in load_ubo
   * intrinsics across a 16b boundary, but v3d's TMU general
               case PIPE_CAP_NIR_IMAGES_AS_DEREF:
            case PIPE_CAP_TEXTURE_TRANSFER_MODES:
            /* XXX perf: we don't want to emit these extra blits for
   * glReadPixels(), since we still have to do an uncached read
   * from the GPU of the result after waiting for the TFU blit
   * to happen.  However, disabling this introduces instability
   * in
   * dEQP-GLES31.functional.image_load_store.early_fragment_tests.*
               case PIPE_CAP_COMPUTE:
            case PIPE_CAP_GENERATE_MIPMAP:
            case PIPE_CAP_INDEP_BLEND_ENABLE:
            case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
                        case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
            if (screen->has_cache_flush)
               case PIPE_CAP_GLSL_FEATURE_LEVEL:
            case PIPE_CAP_ESSL_FEATURE_LEVEL:
      case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
   return 140;
            case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
         case PIPE_CAP_FS_COORD_ORIGIN_LOWER_LEFT:
         case PIPE_CAP_FS_COORD_PIXEL_CENTER_INTEGER:
            if (screen->devinfo.ver >= 40)
            case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
            if (screen->devinfo.ver >= 40)
               case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
            case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
            case PIPE_CAP_MAX_VARYINGS:
                  case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
            if (screen->devinfo.ver < 40)
         else if (screen->nonmsaa_texture_size_limit)
            case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
            if (screen->devinfo.ver < 40)
            case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
            case PIPE_CAP_MAX_RENDER_TARGETS:
            case PIPE_CAP_VENDOR_ID:
         case PIPE_CAP_ACCELERATED:
         case PIPE_CAP_VIDEO_MEMORY: {
                                 }
   case PIPE_CAP_UMA:
            case PIPE_CAP_ALPHA_TEST:
   case PIPE_CAP_FLATSHADE:
   case PIPE_CAP_TWO_SIDED_COLOR:
   case PIPE_CAP_VERTEX_COLOR_CLAMPED:
   case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
   case PIPE_CAP_GL_CLAMP:
            /* Geometry shaders */
   case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
               case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
               case PIPE_CAP_MAX_GS_INVOCATIONS:
            case PIPE_CAP_SUPPORTED_PRIM_MODES:
   case PIPE_CAP_SUPPORTED_PRIM_MODES_WITH_RESTART:
            case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
            case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_IMAGE_STORE_FORMATTED:
            default:
         }
      static float
   v3d_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
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
   v3d_screen_get_shader_param(struct pipe_screen *pscreen, enum pipe_shader_type shader,
         {
                  switch (shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_FRAGMENT:
         case PIPE_SHADER_COMPUTE:
            if (!screen->has_csd)
      case PIPE_SHADER_GEOMETRY:
            if (screen->devinfo.ver < 41)
      default:
                  /* this is probably not totally correct.. but it's a start: */
   switch (param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
            case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
            case PIPE_SHADER_CAP_MAX_INPUTS:
            switch (shader) {
   case PIPE_SHADER_VERTEX:
         case PIPE_SHADER_GEOMETRY:
         case PIPE_SHADER_FRAGMENT:
         default:
      case PIPE_SHADER_CAP_MAX_OUTPUTS:
            if (shader == PIPE_SHADER_FRAGMENT)
            case PIPE_SHADER_CAP_MAX_TEMPS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
            /* Note: Limited by the offset size in
   * v3d_unit_data_create().
      case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         case PIPE_SHADER_CAP_CONT_SUPPORTED:
         case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
            /* We don't currently support this in the backend, but that is
   * okay because our NIR compiler sets the option
   * lower_all_io_to_temps, which will eliminate indirect
   * indexing on all input/output variables by translating it to
   * indirect indexing on temporary variables instead, which we
   * will then lower to scratch. We prefer this over setting this
   * to 0, which would cause if-ladder injection to eliminate
   * indirect indexing on inputs.
      case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
         case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
         case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         case PIPE_SHADER_CAP_SUBROUTINES:
         case PIPE_SHADER_CAP_INTEGERS:
         case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
   case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
   case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
            case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
            if (screen->has_cache_flush) {
            if (shader == PIPE_SHADER_VERTEX ||
      shader == PIPE_SHADER_GEOMETRY) {
                     case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
            if (screen->has_cache_flush) {
            if (screen->devinfo.ver < 41)
                        case PIPE_SHADER_CAP_SUPPORTED_IRS:
         default:
               }
   }
      static int
   v3d_get_compute_param(struct pipe_screen *pscreen, enum pipe_shader_ir ir_type,
         {
                  if (!screen->has_csd)
      #define RET(x) do {                                     \
                  if (ret)                                \
               switch (param) {
   case PIPE_COMPUTE_CAP_ADDRESS_BITS:
                  case PIPE_COMPUTE_CAP_IR_TARGET:
                  case PIPE_COMPUTE_CAP_GRID_DIMENSION:
            case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
            /* GL_MAX_COMPUTE_SHADER_WORK_GROUP_COUNT: The CSD has a
   * 16-bit field for the number of workgroups in each
               case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
                  case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
   case PIPE_COMPUTE_CAP_MAX_VARIABLE_THREADS_PER_BLOCK:
            /* GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS: This is
               case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE:
            case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE:
                  case PIPE_COMPUTE_CAP_MAX_PRIVATE_SIZE:
   case PIPE_COMPUTE_CAP_MAX_INPUT_SIZE:
            case PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE: {
            struct sysinfo si;
               case PIPE_COMPUTE_CAP_MAX_CLOCK_FREQUENCY:
                  case PIPE_COMPUTE_CAP_MAX_COMPUTE_UNITS:
            case PIPE_COMPUTE_CAP_IMAGES_SUPPORTED:
            case PIPE_COMPUTE_CAP_SUBGROUP_SIZES:
            case PIPE_COMPUTE_CAP_MAX_SUBGROUPS:
                     }
      static bool
   v3d_screen_is_format_supported(struct pipe_screen *pscreen,
                                 {
                  if (MAX2(1, sample_count) != MAX2(1, storage_sample_count))
            if (sample_count > 1 && sample_count != V3D_MAX_SAMPLES)
            if (target >= PIPE_MAX_TEXTURE_TYPES) {
                  if (usage & PIPE_BIND_VERTEX_BUFFER) {
            switch (format) {
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
   case PIPE_FORMAT_R32G32B32_FLOAT:
   case PIPE_FORMAT_R32G32_FLOAT:
   case PIPE_FORMAT_R32_FLOAT:
   case PIPE_FORMAT_R32G32B32A32_SNORM:
   case PIPE_FORMAT_R32G32B32_SNORM:
   case PIPE_FORMAT_R32G32_SNORM:
   case PIPE_FORMAT_R32_SNORM:
   case PIPE_FORMAT_R32G32B32A32_SSCALED:
   case PIPE_FORMAT_R32G32B32_SSCALED:
   case PIPE_FORMAT_R32G32_SSCALED:
   case PIPE_FORMAT_R32_SSCALED:
   case PIPE_FORMAT_R16G16B16A16_UNORM:
   case PIPE_FORMAT_R16G16B16A16_FLOAT:
   case PIPE_FORMAT_R16G16B16_UNORM:
   case PIPE_FORMAT_R16G16_UNORM:
   case PIPE_FORMAT_R16_UNORM:
   case PIPE_FORMAT_R16_FLOAT:
   case PIPE_FORMAT_R16G16B16A16_SNORM:
   case PIPE_FORMAT_R16G16B16_SNORM:
   case PIPE_FORMAT_R16G16_SNORM:
   case PIPE_FORMAT_R16G16_FLOAT:
   case PIPE_FORMAT_R16_SNORM:
   case PIPE_FORMAT_R16G16B16A16_USCALED:
   case PIPE_FORMAT_R16G16B16_USCALED:
   case PIPE_FORMAT_R16G16_USCALED:
   case PIPE_FORMAT_R16_USCALED:
   case PIPE_FORMAT_R16G16B16A16_SSCALED:
   case PIPE_FORMAT_R16G16B16_SSCALED:
   case PIPE_FORMAT_R16G16_SSCALED:
   case PIPE_FORMAT_R16_SSCALED:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_R8G8B8_UNORM:
   case PIPE_FORMAT_R8G8_UNORM:
   case PIPE_FORMAT_R8_UNORM:
   case PIPE_FORMAT_R8G8B8A8_SNORM:
   case PIPE_FORMAT_R8G8B8_SNORM:
   case PIPE_FORMAT_R8G8_SNORM:
   case PIPE_FORMAT_R8_SNORM:
   case PIPE_FORMAT_R8G8B8A8_USCALED:
   case PIPE_FORMAT_R8G8B8_USCALED:
   case PIPE_FORMAT_R8G8_USCALED:
   case PIPE_FORMAT_R8_USCALED:
   case PIPE_FORMAT_R8G8B8A8_SSCALED:
   case PIPE_FORMAT_R8G8B8_SSCALED:
   case PIPE_FORMAT_R8G8_SSCALED:
   case PIPE_FORMAT_R8_SSCALED:
   case PIPE_FORMAT_R10G10B10A2_UNORM:
   case PIPE_FORMAT_B10G10R10A2_UNORM:
   case PIPE_FORMAT_R10G10B10A2_SNORM:
   case PIPE_FORMAT_B10G10R10A2_SNORM:
   case PIPE_FORMAT_R10G10B10A2_USCALED:
   case PIPE_FORMAT_B10G10R10A2_USCALED:
   case PIPE_FORMAT_R10G10B10A2_SSCALED:
   case PIPE_FORMAT_B10G10R10A2_SSCALED:
         default:
               /* FORMAT_NONE gets allowed for ARB_framebuffer_no_attachments's probe
      * of FRAMEBUFFER_MAX_SAMPLES
      if ((usage & PIPE_BIND_RENDER_TARGET) &&
         format != PIPE_FORMAT_NONE &&
   !v3d_rt_format_supported(&screen->devinfo, format)) {
            if ((usage & PIPE_BIND_SAMPLER_VIEW) &&
         !v3d_tex_format_supported(&screen->devinfo, format)) {
            if ((usage & PIPE_BIND_DEPTH_STENCIL) &&
         !(format == PIPE_FORMAT_S8_UINT_Z24_UNORM ||
   format == PIPE_FORMAT_X8Z24_UNORM ||
   format == PIPE_FORMAT_Z16_UNORM ||
   format == PIPE_FORMAT_Z32_FLOAT ||
   format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT)) {
            if ((usage & PIPE_BIND_INDEX_BUFFER) &&
         !(format == PIPE_FORMAT_R8_UINT ||
   format == PIPE_FORMAT_R16_UINT ||
   format == PIPE_FORMAT_R32_UINT)) {
            if (usage & PIPE_BIND_SHADER_IMAGE) {
            switch (format) {
   /* FIXME: maybe we can implement a swizzle-on-writes to add
   * support for BGRA-alike formats.
   */
   case PIPE_FORMAT_A4B4G4R4_UNORM:
   case PIPE_FORMAT_A1B5G5R5_UNORM:
   case PIPE_FORMAT_B5G6R5_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_Z16_UNORM:
         default:
               }
      static const nir_shader_compiler_options v3d_nir_options = {
         .lower_uadd_sat = true,
   .lower_usub_sat = true,
   .lower_iadd_sat = true,
   .lower_all_io_to_temps = true,
   .lower_extract_byte = true,
   .lower_extract_word = true,
   .lower_insert_byte = true,
   .lower_insert_word = true,
   .lower_bitfield_insert = true,
   .lower_bitfield_extract = true,
   .lower_bitfield_reverse = true,
   .lower_bit_count = true,
   .lower_cs_local_id_to_index = true,
   .lower_ffract = true,
   .lower_fmod = true,
   .lower_pack_unorm_2x16 = true,
   .lower_pack_snorm_2x16 = true,
   .lower_pack_unorm_4x8 = true,
   .lower_pack_snorm_4x8 = true,
   .lower_unpack_unorm_4x8 = true,
   .lower_unpack_snorm_4x8 = true,
   .lower_pack_half_2x16 = true,
   .lower_unpack_half_2x16 = true,
   .lower_pack_32_2x16 = true,
   .lower_pack_32_2x16_split = true,
   .lower_unpack_32_2x16_split = true,
   .lower_fdiv = true,
   .lower_find_lsb = true,
   .lower_ffma16 = true,
   .lower_ffma32 = true,
   .lower_ffma64 = true,
   .lower_flrp32 = true,
   .lower_fpow = true,
   .lower_fsat = true,
   .lower_fsqrt = true,
   .lower_ifind_msb = true,
   .lower_isign = true,
   .lower_ldexp = true,
   .lower_mul_high = true,
   .lower_wpos_pntc = true,
   .lower_to_scalar = true,
   .lower_int64_options = nir_lower_imul_2x32_64,
   .lower_fquantize2f16 = true,
   .has_fsub = true,
   .has_isub = true,
   .divergence_analysis_options =
         /* This will enable loop unrolling in the state tracker so we won't
      * be able to selectively disable it in backend if it leads to
   * lower thread counts or TMU spills. Choose a conservative maximum to
   * limit register pressure impact.
      .max_unroll_iterations = 16,
   };
      static const void *
   v3d_screen_get_compiler_options(struct pipe_screen *pscreen,
         {
         }
      static const uint64_t v3d_available_modifiers[] = {
      DRM_FORMAT_MOD_BROADCOM_UIF,
   DRM_FORMAT_MOD_LINEAR,
      };
      static void
   v3d_screen_query_dmabuf_modifiers(struct pipe_screen *pscreen,
                           {
         int i;
            switch (format) {
   case PIPE_FORMAT_P030:
            /* Expose SAND128, but not LINEAR or UIF */
   *count = 1;
   if (modifiers && max > 0) {
            modifiers[0] = DRM_FORMAT_MOD_BROADCOM_SAND128;
            case PIPE_FORMAT_NV12:
            /* Expose UIF, LINEAR and SAND128 */
      case PIPE_FORMAT_R8_UNORM:
   case PIPE_FORMAT_R8G8_UNORM:
   case PIPE_FORMAT_R16_UNORM:
   case PIPE_FORMAT_R16G16_UNORM:
   if (!modifiers) break;
                  *count = MIN2(max, num_modifiers);
   for (i = 0; i < *count; i++) {
            modifiers[i] = v3d_available_modifiers[i];
            default:
                        if (!modifiers) {
                        *count = MIN2(max, num_modifiers);
   for (i = 0; i < *count; i++) {
            modifiers[i] = v3d_available_modifiers[i];
      }
      static bool
   v3d_screen_is_dmabuf_modifier_supported(struct pipe_screen *pscreen,
                     {
         int i;
   if (fourcc_mod_broadcom_mod(modifier) == DRM_FORMAT_MOD_BROADCOM_SAND128) {
            switch(format) {
   case PIPE_FORMAT_NV12:
   case PIPE_FORMAT_P030:
   case PIPE_FORMAT_R8_UNORM:
   case PIPE_FORMAT_R8G8_UNORM:
   case PIPE_FORMAT_R16_UNORM:
   case PIPE_FORMAT_R16G16_UNORM:
            if (external_only)
      default:
      } else if (format == PIPE_FORMAT_P030) {
                        /* We don't want to generally allow DRM_FORMAT_MOD_BROADCOM_SAND128
      * modifier, that is the last v3d_available_modifiers. We only accept
   * it in the case of having a PIPE_FORMAT_NV12 or PIPE_FORMAT_P030.
      assert(v3d_available_modifiers[ARRAY_SIZE(v3d_available_modifiers) - 1] ==
         for (i = 0; i < ARRAY_SIZE(v3d_available_modifiers) - 1; i++) {
                                             }
      static enum pipe_format
   v3d_screen_get_compatible_tlb_format(struct pipe_screen *screen,
         {
         switch (format) {
   case PIPE_FORMAT_R16G16_UNORM:
         default:
         }
      static struct disk_cache *
   v3d_screen_get_disk_shader_cache(struct pipe_screen *pscreen)
   {
                  }
      static int
   v3d_screen_get_fd(struct pipe_screen *pscreen)
   {
                  }
      struct pipe_screen *
   v3d_screen_create(int fd, const struct pipe_screen_config *config,
         {
         struct v3d_screen *screen = rzalloc(NULL, struct v3d_screen);
                     pscreen->destroy = v3d_screen_destroy;
   pscreen->get_screen_fd = v3d_screen_get_fd;
   pscreen->get_param = v3d_screen_get_param;
   pscreen->get_paramf = v3d_screen_get_paramf;
   pscreen->get_shader_param = v3d_screen_get_shader_param;
   pscreen->get_compute_param = v3d_get_compute_param;
   pscreen->context_create = v3d_context_create;
   pscreen->is_format_supported = v3d_screen_is_format_supported;
            screen->fd = fd;
            list_inithead(&screen->bo_cache.time_list);
   (void)mtx_init(&screen->bo_handles_mutex, mtx_plain);
      #if defined(USE_V3D_SIMULATOR)
         #endif
            if (!v3d_get_device_info(screen->fd, &screen->devinfo, &v3d_ioctl))
            driParseConfigFiles(config->options, config->options_info, 0, "v3d",
            /* We have to driCheckOption for the simulator mode to not assertion
      * fail on not having our XML config.
      const char *nonmsaa_name = "v3d_nonmsaa_texture_size_limit";
   screen->nonmsaa_texture_size_limit =
                           screen->has_csd = v3d_has_feature(screen, DRM_V3D_PARAM_SUPPORTS_CSD);
   screen->has_cache_flush =
                                                #ifdef ENABLE_SHADER_CACHE
         #endif
            pscreen->get_name = v3d_screen_get_name;
   pscreen->get_vendor = v3d_screen_get_vendor;
   pscreen->get_device_vendor = v3d_screen_get_vendor;
   pscreen->get_compiler_options = v3d_screen_get_compiler_options;
   pscreen->get_disk_shader_cache = v3d_screen_get_disk_shader_cache;
   pscreen->query_dmabuf_modifiers = v3d_screen_query_dmabuf_modifiers;
   pscreen->is_dmabuf_modifier_supported =
            if (screen->has_perfmon) {
                        /* Generate the bitmask of supported draw primitives. */
   screen->prim_types = BITFIELD_BIT(MESA_PRIM_POINTS) |
                        BITFIELD_BIT(MESA_PRIM_LINES) |
   BITFIELD_BIT(MESA_PRIM_LINE_LOOP) |
   BITFIELD_BIT(MESA_PRIM_LINE_STRIP) |
   BITFIELD_BIT(MESA_PRIM_TRIANGLES) |
   BITFIELD_BIT(MESA_PRIM_TRIANGLE_STRIP) |
                  fail:
         close(fd);
   ralloc_free(pscreen);
   }
