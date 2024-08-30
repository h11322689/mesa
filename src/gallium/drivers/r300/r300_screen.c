   /*
   * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
   * Copyright 2010 Marek Olšák <maraeo@gmail.com>
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
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      #include "compiler/nir/nir.h"
   #include "util/format/u_format.h"
   #include "util/format/u_format_s3tc.h"
   #include "util/u_screen.h"
   #include "util/u_memory.h"
   #include "util/hex.h"
   #include "util/os_time.h"
   #include "vl/vl_decoder.h"
   #include "vl/vl_video_buffer.h"
      #include "r300_context.h"
   #include "r300_texture.h"
   #include "r300_screen_buffer.h"
   #include "r300_state_inlines.h"
   #include "r300_public.h"
   #include "compiler/r300_nir.h"
      #include "draw/draw_context.h"
      /* Return the identifier behind whom the brave coders responsible for this
   * amalgamation of code, sweat, and duct tape, routinely obscure their names.
   *
   * ...I should have just put "Corbin Simpson", but I'm not that cool.
   *
   * (Or egotistical. Yet.) */
   static const char* r300_get_vendor(struct pipe_screen* pscreen)
   {
         }
      static const char* r300_get_device_vendor(struct pipe_screen* pscreen)
   {
         }
      static const char* chip_families[] = {
      "unknown",
   "ATI R300",
   "ATI R350",
   "ATI RV350",
   "ATI RV370",
   "ATI RV380",
   "ATI RS400",
   "ATI RC410",
   "ATI RS480",
   "ATI R420",
   "ATI R423",
   "ATI R430",
   "ATI R480",
   "ATI R481",
   "ATI RV410",
   "ATI RS600",
   "ATI RS690",
   "ATI RS740",
   "ATI RV515",
   "ATI R520",
   "ATI RV530",
   "ATI R580",
   "ATI RV560",
      };
      static const char* r300_get_family_name(struct r300_screen* r300screen)
   {
         }
      static const char* r300_get_name(struct pipe_screen* pscreen)
   {
                  }
      static void r300_disk_cache_create(struct r300_screen* r300screen)
   {
      struct mesa_sha1 ctx;
   unsigned char sha1[20];
            _mesa_sha1_init(&ctx);
   if (!disk_cache_get_function_identifier(r300_disk_cache_create,
                  _mesa_sha1_final(&ctx, sha1);
            r300screen->disk_shader_cache =
                  }
      static struct disk_cache* r300_get_disk_shader_cache(struct pipe_screen* pscreen)
   {
   struct r300_screen* r300screen = r300_screen(pscreen);
   return r300screen->disk_shader_cache;
   }
      static int r300_get_param(struct pipe_screen* pscreen, enum pipe_cap param)
   {
      struct r300_screen* r300screen = r300_screen(pscreen);
            switch (param) {
      /* Supported features (boolean caps). */
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
   case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_TEXTURE_BARRIER:
   case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
   case PIPE_CAP_CLIP_HALFZ:
   case PIPE_CAP_ALLOW_MAPPED_BUFFERS_DURING_EXECUTION:
   case PIPE_CAP_LEGACY_MATH_RULES:
   case PIPE_CAP_TGSI_TEXCOORD:
            case PIPE_CAP_TEXTURE_TRANSFER_MODES:
            case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
            case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_GLSL_FEATURE_LEVEL:
   case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
            /* r300 cannot do swizzling of compressed textures. Supported otherwise. */
   case PIPE_CAP_TEXTURE_SWIZZLE:
            /* We don't support color clamping on r500, so that we can use color
         case PIPE_CAP_VERTEX_COLOR_CLAMPED:
            /* Supported on r500 only. */
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
   case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
            case PIPE_CAP_SHAREABLE_SHADERS:
            case PIPE_CAP_MAX_GS_INVOCATIONS:
         case PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT:
            /* SWTCL-only features. */
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
   case PIPE_CAP_USER_VERTEX_BUFFERS:
   case PIPE_CAP_VS_WINDOW_SPACE_POSITION:
            /* HWTCL-only features / limitations. */
   case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
            /* Texturing. */
   case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
         case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
                  /* Render targets. */
   case PIPE_CAP_MAX_RENDER_TARGETS:
         case PIPE_CAP_ENDIANNESS:
            case PIPE_CAP_MAX_VIEWPORTS:
            case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
            case PIPE_CAP_MAX_VARYINGS:
      case PIPE_CAP_PREFER_IMM_ARRAYS_AS_CONSTBUF:
                  case PIPE_CAP_VENDOR_ID:
         case PIPE_CAP_DEVICE_ID:
         case PIPE_CAP_ACCELERATED:
         case PIPE_CAP_VIDEO_MEMORY:
         case PIPE_CAP_UMA:
         case PIPE_CAP_PCI_GROUP:
         case PIPE_CAP_PCI_BUS:
         case PIPE_CAP_PCI_DEVICE:
         case PIPE_CAP_PCI_FUNCTION:
         default:
         }
      static int r300_get_shader_param(struct pipe_screen *pscreen,
               {
      struct r300_screen* r300screen = r300_screen(pscreen);
   bool is_r400 = r300screen->caps.is_r400;
            switch (param) {
   case PIPE_SHADER_CAP_SUPPORTED_IRS:
         default:
                  switch (shader) {
   case PIPE_SHADER_FRAGMENT:
      switch (param)
   {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
         case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
         case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
         case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         return is_r500 ? 64 : 0; /* Actually unlimited on r500. */
   case PIPE_SHADER_CAP_MAX_INPUTS:
         /* 2 colors + 8 texcoords are always supported
   * (minus fog and wpos).
   *
   * R500 has the ability to turn 3rd and 4th color into
   * additional texcoords but there is no two-sided color
   * selection then. However the facing bit can be used instead. */
   case PIPE_SHADER_CAP_MAX_OUTPUTS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
   case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
         case PIPE_SHADER_CAP_MAX_TEMPS:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
         case PIPE_SHADER_CAP_CONT_SUPPORTED:
   case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
   case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
   case PIPE_SHADER_CAP_SUBROUTINES:
   case PIPE_SHADER_CAP_INTEGERS:
   case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
   case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
   case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         default:
         }
      case PIPE_SHADER_VERTEX:
      switch (param)
   {
   case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
   case PIPE_SHADER_CAP_SUBROUTINES:
         default:;
            if (!r300screen->caps.has_tcl) {
         switch (param) {
   case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
                  /* mesa/st requires that this cap is the same across stages, and the FS
   * can't do ints.
   */
                  /* Even if gallivm NIR can do this, we call nir_to_tgsi manually and
   * TGSI can't.
   */
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
                  /* While draw could normally handle this for the VS, the NIR lowering
   * to regs can't handle our non-native-integers, so we have to lower to
   * if ladders.
   */
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
         default:
                  switch (param)
   {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
         case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         case PIPE_SHADER_CAP_MAX_INPUTS:
         case PIPE_SHADER_CAP_MAX_OUTPUTS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         case PIPE_SHADER_CAP_MAX_TEMPS:
         case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
   case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
         case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
   case PIPE_SHADER_CAP_CONT_SUPPORTED:
   case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
   case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_SUBROUTINES:
   case PIPE_SHADER_CAP_INTEGERS:
   case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
   case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
   case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
   case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         default:
         }
      default:
         }
      }
      static float r300_get_paramf(struct pipe_screen* pscreen,
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
         /* The maximum dimensions of the colorbuffer are our practical
   * rendering limits. 2048 pixels should be enough for anybody. */
   if (r300screen->caps.is_r500) {
         } else if (r300screen->caps.is_r400) {
         } else {
         case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
         case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
         case PIPE_CAPF_MIN_CONSERVATIVE_RASTER_DILATE:
   case PIPE_CAPF_MAX_CONSERVATIVE_RASTER_DILATE:
   case PIPE_CAPF_CONSERVATIVE_RASTER_DILATE_GRANULARITY:
         default:
         debug_printf("r300: Warning: Unknown CAP %d in get_paramf.\n",
         }
      static int r300_get_video_param(struct pipe_screen *screen,
      enum pipe_video_profile profile,
   enum pipe_video_entrypoint entrypoint,
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
      #define COMMON_NIR_OPTIONS                    \
      .fdot_replicates = true,                   \
   .fuse_ffma32 = true,                       \
   .fuse_ffma64 = true,                       \
   .lower_bitops = true,                      \
   .lower_extract_byte = true,                \
   .lower_extract_word = true,                \
   .lower_fceil = true,                       \
   .lower_fdiv = true,                        \
   .lower_fdph = true,                        \
   .lower_ffloor = true,                      \
   .lower_flrp32 = true,                      \
   .lower_flrp64 = true,                      \
   .lower_fmod = true,                        \
   .lower_fsign = true,                       \
   .lower_fsqrt = true,                       \
   .lower_ftrunc = true,                      \
   .lower_insert_byte = true,                 \
   .lower_insert_word = true,                 \
   .lower_uniforms_to_ubo = true,             \
   .lower_vector_cmp = true,                  \
   .no_integers = true,                       \
         static const nir_shader_compiler_options r500_vs_compiler_options = {
      COMMON_NIR_OPTIONS,
            /* Have HW loops support and 1024 max instr count, but don't unroll *too*
   * hard.
   */
      };
      static const nir_shader_compiler_options r500_fs_compiler_options = {
      COMMON_NIR_OPTIONS,
   .lower_fpow = true, /* POW is only in the VS */
            /* Have HW loops support and 512 max instr count, but don't unroll *too*
   * hard.
   */
      };
      static const nir_shader_compiler_options r300_vs_compiler_options = {
      COMMON_NIR_OPTIONS,
   .lower_fsat = true, /* No fsat in pre-r500 VS */
            /* Note: has HW loops support, but only 256 ALU instructions. */
      };
      static const nir_shader_compiler_options r300_fs_compiler_options = {
      COMMON_NIR_OPTIONS,
   .lower_fpow = true, /* POW is only in the VS */
   .lower_sincos = true,
            /* No HW loops support, so set it equal to ALU instr max */
      };
      static const void *
   r300_get_compiler_options(struct pipe_screen *pscreen,
               {
                        if (r300screen->caps.is_r500) {
      if (shader == PIPE_SHADER_VERTEX)
         else
      } else {
      if (shader == PIPE_SHADER_VERTEX)
         else
         }
      /**
   * Whether the format matches:
   *   PIPE_FORMAT_?10?10?10?2_UNORM
   */
   static inline bool
   util_format_is_rgba1010102_variant(const struct util_format_description *desc)
   {
      static const unsigned size[4] = {10, 10, 10, 2};
            if (desc->block.width != 1 ||
      desc->block.height != 1 ||
   desc->block.bits != 32)
         for (chan = 0; chan < 4; ++chan) {
      if(desc->channel[chan].type != UTIL_FORMAT_TYPE_UNSIGNED &&
      desc->channel[chan].type != UTIL_FORMAT_TYPE_VOID)
      if (desc->channel[chan].size != size[chan])
                  }
      static bool r300_is_blending_supported(struct r300_screen *rscreen,
         {
      int c;
   const struct util_format_description *desc =
            if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN)
                     /* RGBA16F */
   if (rscreen->caps.is_r500 &&
      desc->nr_channels == 4 &&
   desc->channel[c].size == 16 &&
   desc->channel[c].type == UTIL_FORMAT_TYPE_FLOAT)
         if (desc->channel[c].normalized &&
      desc->channel[c].type == UTIL_FORMAT_TYPE_UNSIGNED &&
   desc->channel[c].size >= 4 &&
   desc->channel[c].size <= 10) {
   /* RGB10_A2, RGBA8, RGB5_A1, RGBA4, RGB565 */
   if (desc->nr_channels >= 3)
            if (format == PIPE_FORMAT_R8G8_UNORM)
            /* R8, I8, L8, A8 */
   if (desc->nr_channels == 1)
                  }
      static bool r300_is_format_supported(struct pipe_screen* screen,
                                 {
      uint32_t retval = 0;
   bool is_r500 = r300_screen(screen)->caps.is_r500;
   bool is_r400 = r300_screen(screen)->caps.is_r400;
   bool is_color2101010 = format == PIPE_FORMAT_R10G10B10A2_UNORM ||
                           bool is_ati1n = format == PIPE_FORMAT_RGTC1_UNORM ||
                     bool is_ati2n = format == PIPE_FORMAT_RGTC2_UNORM ||
                     bool is_half_float = format == PIPE_FORMAT_R16_FLOAT ||
                                    if (MAX2(1, sample_count) != MAX2(1, storage_sample_count))
            /* Check multisampling support. */
   switch (sample_count) {
      case 0:
   case 1:
         case 2:
   case 4:
   case 6:
         /* No texturing and scanout. */
   if (usage & (PIPE_BIND_SAMPLER_VIEW |
                                       if (is_r500) {
      /* Only allow depth/stencil, RGBA8, RGBA1010102, RGBA16F. */
   if (!util_format_is_depth_or_stencil(format) &&
      !util_format_is_rgba8_variant(desc) &&
   !util_format_is_rgba1010102_variant(desc) &&
   format != PIPE_FORMAT_R16G16B16A16_FLOAT &&
   format != PIPE_FORMAT_R16G16B16X16_FLOAT) {
         } else {
      /* Only allow depth/stencil, RGBA8. */
   if (!util_format_is_depth_or_stencil(format) &&
      !util_format_is_rgba8_variant(desc)) {
         }
   default:
               /* Check sampler format support. */
   if ((usage & PIPE_BIND_SAMPLER_VIEW) &&
      /* these two are broken for an unknown reason */
   format != PIPE_FORMAT_R8G8B8X8_SNORM &&
   format != PIPE_FORMAT_R16G16B16X16_SNORM &&
   /* ATI1N is r5xx-only. */
   (is_r500 || !is_ati1n) &&
   /* ATI2N is supported on r4xx-r5xx. */
   (is_r400 || is_r500 || !is_ati2n) &&
   r300_is_sampler_format_supported(format)) {
               /* Check colorbuffer format support. */
   if ((usage & (PIPE_BIND_RENDER_TARGET |
                  PIPE_BIND_DISPLAY_TARGET |
   PIPE_BIND_SCANOUT |
   /* 2101010 cannot be rendered to on non-r5xx. */
   (!is_color2101010 || is_r500) &&
   r300_is_colorbuffer_format_supported(format)) {
   retval |= usage &
         (PIPE_BIND_RENDER_TARGET |
   PIPE_BIND_DISPLAY_TARGET |
            if (r300_is_blending_supported(r300_screen(screen), format)) {
                     /* Check depth-stencil format support. */
   if (usage & PIPE_BIND_DEPTH_STENCIL &&
      r300_is_zs_format_supported(format)) {
               /* Check vertex buffer format support. */
   if (usage & PIPE_BIND_VERTEX_BUFFER) {
      if (r300_screen(screen)->caps.has_tcl) {
         /* Half float is supported on >= R400. */
   if ((is_r400 || is_r500 || !is_half_float) &&
      r300_translate_vertex_data_type(format) != R300_INVALID_FORMAT) {
      } else {
         /* SW TCL */
   if (!util_format_is_pure_integer(format)) {
                     if (usage & PIPE_BIND_INDEX_BUFFER) {
      if (format == PIPE_FORMAT_R8_UINT ||
      format == PIPE_FORMAT_R16_UINT ||
   format == PIPE_FORMAT_R32_UINT)
               }
      static void r300_destroy_screen(struct pipe_screen* pscreen)
   {
      struct r300_screen* r300screen = r300_screen(pscreen);
            if (rws && !rws->unref(rws))
            mtx_destroy(&r300screen->cmask_mutex);
                     if (rws)
               }
      static void r300_fence_reference(struct pipe_screen *screen,
               {
                  }
      static bool r300_fence_finish(struct pipe_screen *screen,
                     {
                  }
      static int r300_screen_get_fd(struct pipe_screen *screen)
   {
                  }
      struct pipe_screen* r300_screen_create(struct radeon_winsys *rws,
         {
               if (!r300screen) {
      FREE(r300screen);
                        r300_init_debug(r300screen);
            if (SCREEN_DBG_ON(r300screen, DBG_NO_ZMASK))
         if (SCREEN_DBG_ON(r300screen, DBG_NO_HIZ))
         if (SCREEN_DBG_ON(r300screen, DBG_NO_TCL))
            r300screen->rws = rws;
   r300screen->screen.destroy = r300_destroy_screen;
   r300screen->screen.get_name = r300_get_name;
   r300screen->screen.get_vendor = r300_get_vendor;
   r300screen->screen.get_compiler_options = r300_get_compiler_options;
   r300screen->screen.finalize_nir = r300_finalize_nir;
   r300screen->screen.get_device_vendor = r300_get_device_vendor;
   r300screen->screen.get_disk_shader_cache = r300_get_disk_shader_cache;
   r300screen->screen.get_screen_fd = r300_screen_get_fd;
   r300screen->screen.get_param = r300_get_param;
   r300screen->screen.get_shader_param = r300_get_shader_param;
   r300screen->screen.get_paramf = r300_get_paramf;
   r300screen->screen.get_video_param = r300_get_video_param;
   r300screen->screen.is_format_supported = r300_is_format_supported;
   r300screen->screen.is_video_format_supported = vl_video_buffer_is_format_supported;
   r300screen->screen.context_create = r300_create_context;
   r300screen->screen.fence_reference = r300_fence_reference;
                                                   }
