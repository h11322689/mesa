   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
         #include "compiler/nir/nir.h"
   #include "util/u_helpers.h"
   #include "util/u_memory.h"
   #include "util/format/u_format.h"
   #include "util/format/u_format_s3tc.h"
   #include "util/u_screen.h"
   #include "util/u_video.h"
   #include "util/os_misc.h"
   #include "util/os_time.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "draw/draw_context.h"
      #include "frontend/sw_winsys.h"
   #include "tgsi/tgsi_exec.h"
      #include "sp_texture.h"
   #include "sp_screen.h"
   #include "sp_context.h"
   #include "sp_fence.h"
   #include "sp_public.h"
      static const struct debug_named_value sp_debug_options[] = {
      {"vs",        SP_DBG_VS,         "dump vertex shader assembly to stderr"},
   {"gs",        SP_DBG_GS,         "dump geometry shader assembly to stderr"},
   {"fs",        SP_DBG_FS,         "dump fragment shader assembly to stderr"},
   {"cs",        SP_DBG_CS,         "dump compute shader assembly to stderr"},
   {"no_rast",   SP_DBG_NO_RAST,    "no-ops rasterization, for profiling purposes"},
   {"use_llvm",  SP_DBG_USE_LLVM,   "Use LLVM if available for shaders"},
      };
      int sp_debug;
   DEBUG_GET_ONCE_FLAGS_OPTION(sp_debug, "SOFTPIPE_DEBUG", sp_debug_options, 0)
      static const char *
   softpipe_get_vendor(struct pipe_screen *screen)
   {
         }
         static const char *
   softpipe_get_name(struct pipe_screen *screen)
   {
         }
      static const nir_shader_compiler_options sp_compiler_options = {
      .fdot_replicates = true,
   .fuse_ffma32 = true,
   .fuse_ffma64 = true,
   .lower_extract_byte = true,
   .lower_extract_word = true,
   .lower_insert_byte = true,
   .lower_insert_word = true,
   .lower_fdph = true,
   .lower_flrp64 = true,
   .lower_fmod = true,
   .lower_uniforms_to_ubo = true,
   .lower_vector_cmp = true,
   .lower_int64_options = nir_lower_imul_2x32_64,
   .max_unroll_iterations = 32,
            /* TGSI doesn't have a semantic for local or global index, just local and
   * workgroup id.
   */
      };
      static const void *
   softpipe_get_compiler_options(struct pipe_screen *pscreen,
               {
      assert(ir == PIPE_SHADER_IR_NIR);
      }
      static int
   softpipe_get_param(struct pipe_screen *screen, enum pipe_cap param)
   {
      struct softpipe_screen *sp_screen = softpipe_screen(screen);
   switch (param) {
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
         case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
         case PIPE_CAP_ANISOTROPIC_FILTER:
         case PIPE_CAP_MAX_RENDER_TARGETS:
         case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
         case PIPE_CAP_OCCLUSION_QUERY:
         case PIPE_CAP_QUERY_TIME_ELAPSED:
         case PIPE_CAP_QUERY_PIPELINE_STATISTICS:
         case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
         case PIPE_CAP_TEXTURE_SWIZZLE:
         case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
         case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
         case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
         case PIPE_CAP_BLEND_EQUATION_SEPARATE:
         case PIPE_CAP_INDEP_BLEND_ENABLE:
         case PIPE_CAP_INDEP_BLEND_FUNC:
         case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_INTEGER:
         case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_DEPTH_BOUNDS_TEST:
         case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
         case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
   case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
         case PIPE_CAP_MAX_VERTEX_STREAMS:
      if (sp_screen->use_llvm)
         else
      case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
         case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
         case PIPE_CAP_SHADER_STENCIL_EXPORT:
         case PIPE_CAP_IMAGE_ATOMIC_FLOAT_ADD:
   case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_START_INSTANCE:
         case PIPE_CAP_SEAMLESS_CUBE_MAP:
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
         case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
         case PIPE_CAP_MIN_TEXEL_OFFSET:
         case PIPE_CAP_MAX_TEXEL_OFFSET:
         case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED: /* draw module */
   case PIPE_CAP_VERTEX_COLOR_CLAMPED: /* draw module */
         case PIPE_CAP_GLSL_FEATURE_LEVEL:
   case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
         case PIPE_CAP_COMPUTE:
         case PIPE_CAP_USER_VERTEX_BUFFERS:
   case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_STREAM_OUTPUT_INTERLEAVE_BUFFERS:
   case PIPE_CAP_VS_LAYER_VIEWPORT:
   case PIPE_CAP_DOUBLES:
   case PIPE_CAP_INT64:
   case PIPE_CAP_TGSI_DIV:
         case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
         case PIPE_CAP_QUERY_TIMESTAMP:
   case PIPE_CAP_TIMER_RESOLUTION:
   case PIPE_CAP_CUBE_MAP_ARRAY:
         case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
         case PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT:
         case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_TEXTURE_TRANSFER_MODES:
         case PIPE_CAP_MAX_VIEWPORTS:
         case PIPE_CAP_ENDIANNESS:
         case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
         case PIPE_CAP_TEXTURE_GATHER_SM5:
   case PIPE_CAP_TEXTURE_QUERY_LOD:
         case PIPE_CAP_VS_WINDOW_SPACE_POSITION:
         case PIPE_CAP_FS_FINE_DERIVATIVE:
         case PIPE_CAP_SAMPLER_VIEW_TARGET:
         case PIPE_CAP_FAKE_SW_MSAA:
         case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_DRAW_INDIRECT:
         case PIPE_CAP_QUERY_SO_OVERFLOW:
         case PIPE_CAP_NIR_IMAGES_AS_DEREF:
            case PIPE_CAP_SHAREABLE_SHADERS:
      /* Can't expose shareable shaders because the draw shaders reference the
   * draw module's state, which is per-context.
   */
         case PIPE_CAP_VENDOR_ID:
         case PIPE_CAP_DEVICE_ID:
         case PIPE_CAP_ACCELERATED:
         case PIPE_CAP_VIDEO_MEMORY: {
      /* XXX: Do we want to return the full amount fo system memory ? */
            if (!os_get_total_physical_memory(&system_memory))
            if (sizeof(void *) == 4)
      /* Cap to 2 GB on 32 bits system. We do this because softpipe does
   * eat application memory, which is quite limited on 32 bits. App
                  }
   case PIPE_CAP_UMA:
         case PIPE_CAP_QUERY_MEMORY_INFO:
         case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
         case PIPE_CAP_CLIP_HALFZ:
   case PIPE_CAP_TEXTURE_FLOAT_LINEAR:
   case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
         case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
   case PIPE_CAP_CULL_DISTANCE:
   case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
   case PIPE_CAP_SHADER_ARRAY_COMPONENTS:
   case PIPE_CAP_TGSI_TEXCOORD:
         case PIPE_CAP_MAX_VARYINGS:
         case PIPE_CAP_PCI_GROUP:
   case PIPE_CAP_PCI_BUS:
   case PIPE_CAP_PCI_DEVICE:
   case PIPE_CAP_PCI_FUNCTION:
         case PIPE_CAP_MAX_GS_INVOCATIONS:
         case PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT:
         case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_IMAGE_STORE_FORMATTED:
         default:
            }
      static int
   softpipe_get_shader_param(struct pipe_screen *screen,
               {
               switch (param) {
   case PIPE_SHADER_CAP_SUPPORTED_IRS:
         default:
                  switch(shader)
   {
   case PIPE_SHADER_FRAGMENT:
         case PIPE_SHADER_COMPUTE:
         case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
      if (sp_screen->use_llvm)
         else
      default:
            }
      static float
   softpipe_get_paramf(struct pipe_screen *screen, enum pipe_capf param)
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
         }
   /* should only get here on unhandled cases */
   debug_printf("Unexpected PIPE_CAPF %d query\n", param);
      }
      /**
   * Query format support for creating a texture, drawing surface, etc.
   * \param format  the format to test
   * \param type  one of PIPE_TEXTURE, PIPE_SURFACE
   */
   static bool
   softpipe_is_format_supported( struct pipe_screen *screen,
                                 {
      struct sw_winsys *winsys = softpipe_screen(screen)->winsys;
            assert(target == PIPE_BUFFER ||
         target == PIPE_TEXTURE_1D ||
   target == PIPE_TEXTURE_1D_ARRAY ||
   target == PIPE_TEXTURE_2D ||
   target == PIPE_TEXTURE_2D_ARRAY ||
   target == PIPE_TEXTURE_RECT ||
   target == PIPE_TEXTURE_3D ||
            if (MAX2(1, sample_count) != MAX2(1, storage_sample_count))
                     if (sample_count > 1)
            if (bind & (PIPE_BIND_DISPLAY_TARGET |
                  if(!winsys->is_displaytarget_format_supported(winsys, bind, format))
               if (bind & PIPE_BIND_RENDER_TARGET) {
      if (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS)
            /*
   * Although possible, it is unnatural to render into compressed or YUV
   * surfaces. So disable these here to avoid going into weird paths
   * inside gallium frontends.
   */
   if (format_desc->block.width != 1 ||
      format_desc->block.height != 1)
            if (bind & PIPE_BIND_DEPTH_STENCIL) {
      if (format_desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS)
               if (format_desc->layout == UTIL_FORMAT_LAYOUT_ASTC ||
      format_desc->layout == UTIL_FORMAT_LAYOUT_ATC) {
   /* Software decoding is not hooked up. */
               if ((bind & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW)) &&
      ((bind & PIPE_BIND_DISPLAY_TARGET) == 0) &&
   target != PIPE_BUFFER) {
   const struct util_format_description *desc =
         if (desc->nr_channels == 3 && desc->is_array) {
      /* Don't support any 3-component formats for rendering/texturing
   * since we don't support the corresponding 8-bit 3 channel UNORM
   * formats.  This allows us to support GL_ARB_copy_image between
   * GL_RGB8 and GL_RGB8UI, for example.  Otherwise, we may be asked to
   * do a resource copy between PIPE_FORMAT_R8G8B8_UINT and
   * PIPE_FORMAT_R8G8B8X8_UNORM, for example, which will not work
   * (different bpp).
   */
                  if (format_desc->layout == UTIL_FORMAT_LAYOUT_ETC &&
      format != PIPE_FORMAT_ETC1_RGB8)
         /*
   * All other operations (sampling, transfer, etc).
            /*
   * Everything else should be supported by u_format.
   */
      }
         static void
   softpipe_destroy_screen( struct pipe_screen *screen )
   {
         }
         /* This is often overriden by the co-state tracker.
   */
   static void
   softpipe_flush_frontbuffer(struct pipe_screen *_screen,
                                 {
      struct softpipe_screen *screen = softpipe_screen(_screen);
   struct sw_winsys *winsys = screen->winsys;
            assert(texture->dt);
   if (texture->dt)
      }
      static int
   softpipe_get_compute_param(struct pipe_screen *_screen,
                     {
      switch (param) {
   case PIPE_COMPUTE_CAP_IR_TARGET:
         case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
      if (ret) {
      uint64_t *grid_size = ret;
   grid_size[0] = 65535;
   grid_size[1] = 65535;
      }
      case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
      if (ret) {
      uint64_t *block_size = ret;
   block_size[0] = 1024;
   block_size[1] = 1024;
      }
      case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
      if (ret) {
      uint64_t *max_threads_per_block = ret;
      }
      case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE:
      if (ret) {
      uint64_t *max_local_size = ret;
      }
      case PIPE_COMPUTE_CAP_GRID_DIMENSION:
   case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE:
   case PIPE_COMPUTE_CAP_MAX_PRIVATE_SIZE:
   case PIPE_COMPUTE_CAP_MAX_INPUT_SIZE:
   case PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE:
   case PIPE_COMPUTE_CAP_MAX_CLOCK_FREQUENCY:
   case PIPE_COMPUTE_CAP_MAX_COMPUTE_UNITS:
   case PIPE_COMPUTE_CAP_IMAGES_SUPPORTED:
   case PIPE_COMPUTE_CAP_SUBGROUP_SIZES:
   case PIPE_COMPUTE_CAP_MAX_SUBGROUPS:
   case PIPE_COMPUTE_CAP_ADDRESS_BITS:
   case PIPE_COMPUTE_CAP_MAX_VARIABLE_THREADS_PER_BLOCK:
         }
      }
      static int
   softpipe_screen_get_fd(struct pipe_screen *screen)
   {
               if (winsys->get_fd)
         else
      }
      /**
   * Create a new pipe_screen object
   * Note: we're not presently subclassing pipe_screen (no softpipe_screen).
   */
   struct pipe_screen *
   softpipe_create_screen(struct sw_winsys *winsys)
   {
               if (!screen)
                                       screen->base.get_name = softpipe_get_name;
   screen->base.get_vendor = softpipe_get_vendor;
   screen->base.get_device_vendor = softpipe_get_vendor; // TODO should be the CPU vendor
   screen->base.get_screen_fd = softpipe_screen_get_fd;
   screen->base.get_param = softpipe_get_param;
   screen->base.get_shader_param = softpipe_get_shader_param;
   screen->base.get_paramf = softpipe_get_paramf;
   screen->base.get_timestamp = u_default_get_timestamp;
   screen->base.query_memory_info = util_sw_query_memory_info;
   screen->base.is_format_supported = softpipe_is_format_supported;
   screen->base.context_create = softpipe_create_context;
   screen->base.flush_frontbuffer = softpipe_flush_frontbuffer;
   screen->base.get_compute_param = softpipe_get_compute_param;
   screen->base.get_compiler_options = softpipe_get_compiler_options;
            softpipe_init_screen_texture_funcs(&screen->base);
               }
