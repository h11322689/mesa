   /*
   * Copyright 2010 Christoph Bumiller
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      #include <xf86drm.h>
   #include <nouveau_drm.h>
   #include <nvif/class.h>
   #include "util/format/u_format.h"
   #include "util/format/u_format_s3tc.h"
   #include "util/u_screen.h"
   #include "pipe/p_screen.h"
      #include "nouveau_vp3_video.h"
      #include "nv50_ir_driver.h"
      #include "nvc0/nvc0_context.h"
   #include "nvc0/nvc0_screen.h"
      #include "nvc0/mme/com9097.mme.h"
   #include "nvc0/mme/com90c0.mme.h"
   #include "nvc0/mme/comc597.mme.h"
      #include "nv50/g80_texture.xml.h"
      static bool
   nvc0_screen_is_format_supported(struct pipe_screen *pscreen,
                                 {
               if (sample_count > 8)
         if (!(0x117 & (1 << sample_count))) /* 0, 1, 2, 4 or 8 */
            if (MAX2(1, sample_count) != MAX2(1, storage_sample_count))
            /* Short-circuit the rest of the logic -- this is used by the gallium frontend
   * to determine valid MS levels in a no-attachments scenario.
   */
   if (format == PIPE_FORMAT_NONE && bindings & PIPE_BIND_RENDER_TARGET)
            if ((bindings & PIPE_BIND_SAMPLER_VIEW) && (target != PIPE_BUFFER))
      if (util_format_get_blocksizebits(format) == 3 * 32)
         if (bindings & PIPE_BIND_LINEAR)
      if (util_format_is_depth_or_stencil(format) ||
      (target != PIPE_TEXTURE_1D &&
   target != PIPE_TEXTURE_2D &&
   target != PIPE_TEXTURE_RECT) ||
            /* Restrict ETC2 and ASTC formats here. These are only supported on GK20A
   * and GM20B.
   */
   if ((desc->layout == UTIL_FORMAT_LAYOUT_ETC ||
      desc->layout == UTIL_FORMAT_LAYOUT_ASTC) &&
   nouveau_screen(pscreen)->device->chipset != 0x12b &&
   nouveau_screen(pscreen)->class_3d != NVEA_3D_CLASS)
         /* shared is always supported */
   bindings &= ~(PIPE_BIND_LINEAR |
            if (bindings & PIPE_BIND_SHADER_IMAGE) {
      if (format == PIPE_FORMAT_B8G8R8A8_UNORM &&
      nouveau_screen(pscreen)->class_3d < NVE4_3D_CLASS) {
   /* This should work on Fermi, but for currently unknown reasons it
   * does not and results in breaking reads from pbos. */
                  if (bindings & PIPE_BIND_INDEX_BUFFER) {
      if (format != PIPE_FORMAT_R8_UINT &&
      format != PIPE_FORMAT_R16_UINT &&
   format != PIPE_FORMAT_R32_UINT)
                  return (( nvc0_format_table[format].usage |
      }
      static int
   nvc0_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
   {
      const uint16_t class_3d = nouveau_screen(pscreen)->class_3d;
   const struct nouveau_screen *screen = nouveau_screen(pscreen);
            switch (param) {
   /* non-boolean caps */
   case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
         case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
         case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
         case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
         case PIPE_CAP_MIN_TEXEL_OFFSET:
         case PIPE_CAP_MAX_TEXEL_OFFSET:
         case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT:
         case PIPE_CAP_GLSL_FEATURE_LEVEL:
         case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
         case PIPE_CAP_MAX_RENDER_TARGETS:
         case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
         case PIPE_CAP_VIEWPORT_SUBPIXEL_BITS:
   case PIPE_CAP_RASTERIZER_SUBPIXEL_BITS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
         case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
   case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
         case PIPE_CAP_MAX_VERTEX_STREAMS:
         case PIPE_CAP_MAX_GS_INVOCATIONS:
         case PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT:
         case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
         case PIPE_CAP_MAX_VERTEX_ELEMENT_SRC_OFFSET:
         case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
      if (class_3d < GM107_3D_CLASS)
            case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
         case PIPE_CAP_MAX_VIEWPORTS:
         case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
         case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
         case PIPE_CAP_ENDIANNESS:
         case PIPE_CAP_MAX_SHADER_PATCH_VARYINGS:
         case PIPE_CAP_MAX_WINDOW_RECTANGLES:
         case PIPE_CAP_MAX_CONSERVATIVE_RASTER_SUBPIXEL_PRECISION_BIAS:
         case PIPE_CAP_MAX_TEXTURE_UPLOAD_MEMORY_BUDGET:
         case PIPE_CAP_MAX_VARYINGS:
      /* NOTE: These only count our slots for GENERIC varyings.
   * The address space may be larger, but the actual hard limit seems to be
   * less than what the address space layout permits, so don't add TEXCOORD,
   * COLOR, etc. here.
   */
      case PIPE_CAP_MAX_VERTEX_BUFFERS:
         case PIPE_CAP_GL_BEGIN_END_BUFFER_SIZE:
         case PIPE_CAP_MAX_TEXTURE_MB:
            case PIPE_CAP_TIMER_RESOLUTION:
            case PIPE_CAP_SUPPORTED_PRIM_MODES_WITH_RESTART:
   case PIPE_CAP_SUPPORTED_PRIM_MODES:
            /* supported caps */
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
   case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
   case PIPE_CAP_CUBE_MAP_ARRAY:
   case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
   case PIPE_CAP_TEXTURE_MULTISAMPLE:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_TGSI_TEXCOORD:
   case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
   case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_VERTEX_COLOR_CLAMPED:
   case PIPE_CAP_QUERY_TIMESTAMP:
   case PIPE_CAP_QUERY_TIME_ELAPSED:
   case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_STREAM_OUTPUT_INTERLEAVE_BUFFERS:
   case PIPE_CAP_QUERY_PIPELINE_STATISTICS:
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_INDEP_BLEND_FUNC:
   case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
   case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_TEXTURE_BARRIER:
   case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
   case PIPE_CAP_START_INSTANCE:
   case PIPE_CAP_DRAW_INDIRECT:
   case PIPE_CAP_USER_VERTEX_BUFFERS:
   case PIPE_CAP_TEXTURE_QUERY_LOD:
   case PIPE_CAP_SAMPLE_SHADING:
   case PIPE_CAP_TEXTURE_GATHER_OFFSETS:
   case PIPE_CAP_TEXTURE_GATHER_SM5:
   case PIPE_CAP_FS_FINE_DERIVATIVE:
   case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
   case PIPE_CAP_SAMPLER_VIEW_TARGET:
   case PIPE_CAP_CLIP_HALFZ:
   case PIPE_CAP_POLYGON_OFFSET_CLAMP:
   case PIPE_CAP_MULTISAMPLE_Z_RESOLVE:
   case PIPE_CAP_TEXTURE_FLOAT_LINEAR:
   case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
   case PIPE_CAP_DEPTH_BOUNDS_TEST:
   case PIPE_CAP_TEXTURE_QUERY_SAMPLES:
   case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
   case PIPE_CAP_FORCE_PERSAMPLE_INTERP:
   case PIPE_CAP_DRAW_PARAMETERS:
   case PIPE_CAP_SHADER_PACK_HALF_FLOAT:
   case PIPE_CAP_MULTI_DRAW_INDIRECT:
   case PIPE_CAP_MEMOBJ:
   case PIPE_CAP_MULTI_DRAW_INDIRECT_PARAMS:
   case PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL:
   case PIPE_CAP_QUERY_BUFFER_OBJECT:
   case PIPE_CAP_INVALIDATE_BUFFER:
   case PIPE_CAP_STRING_MARKER:
   case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
   case PIPE_CAP_CULL_DISTANCE:
   case PIPE_CAP_ROBUST_BUFFER_ACCESS_BEHAVIOR:
   case PIPE_CAP_SHADER_GROUP_VOTE:
   case PIPE_CAP_POLYGON_OFFSET_UNITS_UNSCALED:
   case PIPE_CAP_SHADER_ARRAY_COMPONENTS:
   case PIPE_CAP_LEGACY_MATH_RULES:
   case PIPE_CAP_DOUBLES:
   case PIPE_CAP_INT64:
   case PIPE_CAP_TGSI_TEX_TXF_LZ:
   case PIPE_CAP_SHADER_CLOCK:
   case PIPE_CAP_COMPUTE:
   case PIPE_CAP_CAN_BIND_CONST_BUFFER_AS_VERTEX:
   case PIPE_CAP_QUERY_SO_OVERFLOW:
   case PIPE_CAP_TGSI_DIV:
   case PIPE_CAP_IMAGE_ATOMIC_INC_WRAP:
   case PIPE_CAP_DEMOTE_TO_HELPER_INVOCATION:
   case PIPE_CAP_DEVICE_RESET_STATUS_QUERY:
   case PIPE_CAP_TEXTURE_SHADOW_LOD:
   case PIPE_CAP_CLEAR_SCISSORED:
   case PIPE_CAP_IMAGE_STORE_FORMATTED:
   case PIPE_CAP_QUERY_MEMORY_INFO:
         case PIPE_CAP_TEXTURE_TRANSFER_MODES:
         case PIPE_CAP_FBFETCH:
         case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
   case PIPE_CAP_SHADER_BALLOT:
         case PIPE_CAP_BINDLESS_TEXTURE:
         case PIPE_CAP_IMAGE_ATOMIC_FLOAT_ADD:
         case PIPE_CAP_POLYGON_MODE_FILL_RECTANGLE:
   case PIPE_CAP_VS_LAYER_VIEWPORT:
   case PIPE_CAP_TES_LAYER_VIEWPORT:
   case PIPE_CAP_POST_DEPTH_COVERAGE:
   case PIPE_CAP_CONSERVATIVE_RASTER_POST_SNAP_TRIANGLES:
   case PIPE_CAP_CONSERVATIVE_RASTER_POST_SNAP_POINTS_LINES:
   case PIPE_CAP_CONSERVATIVE_RASTER_POST_DEPTH_COVERAGE:
   case PIPE_CAP_PROGRAMMABLE_SAMPLE_LOCATIONS:
   case PIPE_CAP_VIEWPORT_SWIZZLE:
   case PIPE_CAP_VIEWPORT_MASK:
   case PIPE_CAP_SAMPLER_REDUCTION_MINMAX:
         case PIPE_CAP_CONSERVATIVE_RASTER_PRE_SNAP_TRIANGLES:
         case PIPE_CAP_RESOURCE_FROM_USER_MEMORY_COMPUTE_ONLY:
   case PIPE_CAP_SYSTEM_SVM:
            case PIPE_CAP_GL_SPIRV:
   case PIPE_CAP_GL_SPIRV_VARIABLE_POINTERS:
            /* nir related caps */
   case PIPE_CAP_NIR_IMAGES_AS_DEREF:
            case PIPE_CAP_PCI_GROUP:
   case PIPE_CAP_PCI_BUS:
   case PIPE_CAP_PCI_DEVICE:
   case PIPE_CAP_PCI_FUNCTION:
            case PIPE_CAP_OPENCL_INTEGER_FUNCTIONS: /* could be done */
   case PIPE_CAP_INTEGER_MULTIPLY_32X16: /* could be done */
   case PIPE_CAP_MAP_UNSYNCHRONIZED_THREAD_SAFE: /* when we fix MT stuff */
   case PIPE_CAP_ALPHA_TO_COVERAGE_DITHER_CONTROL: /* TODO */
   case PIPE_CAP_SHADER_ATOMIC_INT64: /* TODO */
   case PIPE_CAP_HARDWARE_GL_SELECT:
            case PIPE_CAP_VENDOR_ID:
         case PIPE_CAP_DEVICE_ID: {
      uint64_t device_id;
   if (nouveau_getparam(dev, NOUVEAU_GETPARAM_PCI_DEVICE, &device_id)) {
      NOUVEAU_ERR("NOUVEAU_GETPARAM_PCI_DEVICE failed.\n");
      }
      }
   case PIPE_CAP_ACCELERATED:
         case PIPE_CAP_VIDEO_MEMORY:
         case PIPE_CAP_UMA:
            default:
            }
      static int
   nvc0_screen_get_shader_param(struct pipe_screen *pscreen,
               {
      const struct nouveau_screen *screen = nouveau_screen(pscreen);
            switch (shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_FRAGMENT:
   case PIPE_SHADER_COMPUTE:
   case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL:
         default:
                  switch (param) {
   case PIPE_SHADER_CAP_SUPPORTED_IRS: {
      uint32_t irs = 1 << PIPE_SHADER_IR_NIR;
   if (screen->force_enable_cl)
            }
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         case PIPE_SHADER_CAP_MAX_INPUTS:
         case PIPE_SHADER_CAP_MAX_OUTPUTS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
         case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
      /* HW doesn't support indirect addressing of fragment program inputs
   * on Volta.  The binary driver generates a function to handle every
   * possible indirection, and indirectly calls the function to handle
   * this instead.
   */
   if (class_3d >= GV100_3D_CLASS)
            case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         case PIPE_SHADER_CAP_MAX_TEMPS:
         case PIPE_SHADER_CAP_CONT_SUPPORTED:
         case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
         case PIPE_SHADER_CAP_SUBROUTINES:
         case PIPE_SHADER_CAP_INTEGERS:
         case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
   case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
         case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
         case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
      if (class_3d >= NVE4_3D_CLASS)
         if (shader == PIPE_SHADER_FRAGMENT || shader == PIPE_SHADER_COMPUTE)
            default:
      NOUVEAU_ERR("unknown PIPE_SHADER_CAP %d\n", param);
         }
      static float
   nvc0_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
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
                  NOUVEAU_ERR("unknown PIPE_CAPF %d\n", param);
      }
      static int
   nvc0_screen_get_compute_param(struct pipe_screen *pscreen,
               {
      struct nvc0_screen *screen = nvc0_screen(pscreen);
   struct nouveau_device *dev = screen->base.device;
         #define RET(x) do {                  \
      if (data)                         \
            } while (0)
         switch (param) {
   case PIPE_COMPUTE_CAP_GRID_DIMENSION:
         case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
      if (obj_class >= NVE4_COMPUTE_CLASS) {
         } else {
            case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
         case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
         case PIPE_COMPUTE_CAP_MAX_VARIABLE_THREADS_PER_BLOCK:
      if (obj_class >= NVE4_COMPUTE_CLASS) {
         } else {
            case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE: /* g[] */
         case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE: /* s[] */
      switch (obj_class) {
   case GM200_COMPUTE_CLASS:
         case GM107_COMPUTE_CLASS:
         default:
            case PIPE_COMPUTE_CAP_MAX_PRIVATE_SIZE: /* l[] */
         case PIPE_COMPUTE_CAP_MAX_INPUT_SIZE: /* c[], arbitrary limit */
         case PIPE_COMPUTE_CAP_SUBGROUP_SIZES:
         case PIPE_COMPUTE_CAP_MAX_SUBGROUPS:
         case PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE:
         case PIPE_COMPUTE_CAP_IMAGES_SUPPORTED:
         case PIPE_COMPUTE_CAP_MAX_COMPUTE_UNITS:
         case PIPE_COMPUTE_CAP_MAX_CLOCK_FREQUENCY:
         case PIPE_COMPUTE_CAP_ADDRESS_BITS:
         default:
               #undef RET
   }
      static void
   nvc0_screen_get_sample_pixel_grid(struct pipe_screen *pscreen,
               {
      switch (sample_count) {
   case 0:
   case 1:
      /* this could be 4x4, but the GL state tracker makes it difficult to
   * create a 1x MSAA texture and smaller grids save CB space */
   *width = 2;
   *height = 4;
      case 2:
      *width = 2;
   *height = 4;
      case 4:
      *width = 2;
   *height = 2;
      case 8:
      *width = 1;
   *height = 2;
      default:
            }
      static void
   nvc0_screen_destroy(struct pipe_screen *pscreen)
   {
               if (!nouveau_drm_screen_unref(&screen->base))
            if (screen->blitter)
         if (screen->pm.prog) {
      screen->pm.prog->code = NULL; /* hardcoded, don't FREE */
   nvc0_program_destroy(NULL, screen->pm.prog);
               nouveau_bo_ref(NULL, &screen->text);
   nouveau_bo_ref(NULL, &screen->uniform_bo);
   nouveau_bo_ref(NULL, &screen->tls);
   nouveau_bo_ref(NULL, &screen->txc);
   nouveau_bo_ref(NULL, &screen->fence.bo);
            nouveau_heap_free(&screen->lib_code);
                     nouveau_object_del(&screen->eng3d);
   nouveau_object_del(&screen->eng2d);
   nouveau_object_del(&screen->m2mf);
   nouveau_object_del(&screen->copy);
   nouveau_object_del(&screen->compute);
            nouveau_screen_fini(&screen->base);
               }
      static int
   nvc0_graph_set_macro(struct nvc0_screen *screen, uint32_t m, unsigned pos,
         {
                                 BEGIN_NVC0(push, SUBC_3D(NVC0_GRAPH_MACRO_ID), 2);
   PUSH_DATA (push, (m - 0x3800) / 8);
   PUSH_DATA (push, pos);
   BEGIN_1IC0(push, SUBC_3D(NVC0_GRAPH_MACRO_UPLOAD_POS), size + 1);
   PUSH_DATA (push, pos);
               }
      static int
   tu102_graph_set_macro(struct nvc0_screen *screen, uint32_t m, unsigned pos,
         {
                                 BEGIN_NVC0(push, SUBC_3D(NVC0_GRAPH_MACRO_ID), 2);
   PUSH_DATA (push, (m - 0x3800) / 8);
   PUSH_DATA (push, pos);
   BEGIN_1IC0(push, SUBC_3D(NVC0_GRAPH_MACRO_UPLOAD_POS), size + 1);
   PUSH_DATA (push, pos);
               }
      static void
   nvc0_magic_3d_init(struct nouveau_pushbuf *push, uint16_t obj_class)
   {
      BEGIN_NVC0(push, SUBC_3D(0x10cc), 1);
   PUSH_DATA (push, 0xff);
   BEGIN_NVC0(push, SUBC_3D(0x10e0), 2);
   PUSH_DATA (push, 0xff);
   PUSH_DATA (push, 0xff);
   BEGIN_NVC0(push, SUBC_3D(0x10ec), 2);
   PUSH_DATA (push, 0xff);
   PUSH_DATA (push, 0xff);
   if (obj_class < GV100_3D_CLASS) {
      BEGIN_NVC0(push, SUBC_3D(0x074c), 1);
               BEGIN_NVC0(push, SUBC_3D(0x16a8), 1);
   PUSH_DATA (push, (3 << 16) | 3);
   BEGIN_NVC0(push, SUBC_3D(0x1794), 1);
            if (obj_class < GM107_3D_CLASS) {
      BEGIN_NVC0(push, SUBC_3D(0x12ac), 1);
      }
   BEGIN_NVC0(push, SUBC_3D(0x0218), 1);
   PUSH_DATA (push, 0x10);
   BEGIN_NVC0(push, SUBC_3D(0x10fc), 1);
   PUSH_DATA (push, 0x10);
   BEGIN_NVC0(push, SUBC_3D(0x1290), 1);
   PUSH_DATA (push, 0x10);
   BEGIN_NVC0(push, SUBC_3D(0x12d8), 2);
   PUSH_DATA (push, 0x10);
   PUSH_DATA (push, 0x10);
   BEGIN_NVC0(push, SUBC_3D(0x1140), 1);
   PUSH_DATA (push, 0x10);
   BEGIN_NVC0(push, SUBC_3D(0x1610), 1);
            BEGIN_NVC0(push, NVC0_3D(VERTEX_ID_GEN_MODE), 1);
   PUSH_DATA (push, NVC0_3D_VERTEX_ID_GEN_MODE_DRAW_ARRAYS_ADD_START);
   BEGIN_NVC0(push, SUBC_3D(0x030c), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, SUBC_3D(0x0300), 1);
            if (obj_class < GV100_3D_CLASS) {
      BEGIN_NVC0(push, SUBC_3D(0x02d0), 1);
      }
   BEGIN_NVC0(push, SUBC_3D(0x0fdc), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, SUBC_3D(0x19c0), 1);
            if (obj_class < GM107_3D_CLASS) {
      BEGIN_NVC0(push, SUBC_3D(0x075c), 1);
            if (obj_class >= NVE4_3D_CLASS) {
      BEGIN_NVC0(push, SUBC_3D(0x07fc), 1);
                  /* TODO: find out what software methods 0x1528, 0x1280 and (on nve4) 0x02dc
      }
      static void
   nvc0_screen_fence_emit(struct pipe_context *pcontext, u32 *sequence,
         {
      struct nvc0_context *nvc0 = nvc0_context(pcontext);
   struct nvc0_screen *screen = nvc0->screen;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            /* we need to do it after possible flush in MARK_RING */
            assert(PUSH_AVAIL(push) + push->rsvd_kick >= 5);
   PUSH_DATA (push, NVC0_FIFO_PKHDR_SQ(NVC0_3D(QUERY_ADDRESS_HIGH), 4));
   PUSH_DATAh(push, screen->fence.bo->offset);
   PUSH_DATA (push, screen->fence.bo->offset);
   PUSH_DATA (push, *sequence);
   PUSH_DATA (push, NVC0_3D_QUERY_GET_FENCE | NVC0_3D_QUERY_GET_SHORT |
               }
      static u32
   nvc0_screen_fence_update(struct pipe_screen *pscreen)
   {
      struct nvc0_screen *screen = nvc0_screen(pscreen);
      }
      static int
   nvc0_screen_init_compute(struct nvc0_screen *screen)
   {
      const struct nouveau_mclass computes[] = {
      { AD102_COMPUTE_CLASS, -1 },
   { GA102_COMPUTE_CLASS, -1 },
   { TU102_COMPUTE_CLASS, -1 },
   { GV100_COMPUTE_CLASS, -1 },
   { GP104_COMPUTE_CLASS, -1 },
   { GP100_COMPUTE_CLASS, -1 },
   { GM200_COMPUTE_CLASS, -1 },
   { GM107_COMPUTE_CLASS, -1 },
   {  NVF0_COMPUTE_CLASS, -1 },
   {  NVE4_COMPUTE_CLASS, -1 },
   /* In theory, GF110+ should also support NVC8_COMPUTE_CLASS but,
   //      {  NVC8_COMPUTE_CLASS, -1 },
         {  NVC0_COMPUTE_CLASS, -1 },
      };
   struct nouveau_object *chan = screen->base.channel;
                     ret = nouveau_object_mclass(chan, computes);
   if (ret < 0) {
      NOUVEAU_ERR("No supported compute class: %d\n", ret);
               ret = nouveau_object_new(chan, 0xbeef00c0, computes[ret].oclass, NULL, 0, &screen->compute);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate compute class: %d\n", ret);
               if (screen->compute->oclass < NVE4_COMPUTE_CLASS)
               }
      static int
   nvc0_screen_resize_tls_area(struct nvc0_screen *screen,
         {
      struct nouveau_bo *bo = NULL;
   int ret;
            if (size >= (1 << 20)) {
      NOUVEAU_ERR("requested TLS size too large: 0x%"PRIx64"\n", size);
               size *= (screen->base.device->chipset >= 0xe0) ? 64 : 48; /* max warps */
   size  = align(size, 0x8000);
                     ret = nouveau_bo_new(screen->base.device, NV_VRAM_DOMAIN(&screen->base), 1 << 17, size,
         if (ret)
            /* Make sure that the pushbuf has acquired a reference to the old tls
   * segment, as it may have commands that will reference it.
   */
   if (screen->tls)
      PUSH_REF1(screen->base.pushbuf, screen->tls,
      nouveau_bo_ref(NULL, &screen->tls);
   screen->tls = bo;
      }
      int
   nvc0_screen_resize_text_area(struct nvc0_screen *screen, struct nouveau_pushbuf *push,
         {
      struct nouveau_bo *bo;
            ret = nouveau_bo_new(screen->base.device, NV_VRAM_DOMAIN(&screen->base),
         if (ret)
            /* Make sure that the pushbuf has acquired a reference to the old text
   * segment, as it may have commands that will reference it.
   */
   if (screen->text)
      PUSH_REF1(screen->base.pushbuf, screen->text,
      nouveau_bo_ref(NULL, &screen->text);
            nouveau_heap_free(&screen->lib_code);
            /* XXX: getting a page fault at the end of the code buffer every few
   *  launches, don't use the last 256 bytes to work around them - prefetch ?
   */
            /* update the code segment setup */
   if (screen->eng3d->oclass < GV100_3D_CLASS) {
      BEGIN_NVC0(push, NVC0_3D(CODE_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->text->offset);
   PUSH_DATA (push, screen->text->offset);
   if (screen->compute) {
      BEGIN_NVC0(push, NVC0_CP(CODE_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->text->offset);
                     }
      void
   nvc0_screen_bind_cb_3d(struct nvc0_screen *screen, struct nouveau_pushbuf *push,
         {
               if (screen->base.class_3d >= GM107_3D_CLASS) {
               // TODO: Better figure out the conditions in which this is needed
   bool serialize = binding->addr == addr && binding->size != size;
   if (can_serialize)
         if (serialize) {
      IMMED_NVC0(push, NVC0_3D(SERIALIZE), 0);
   if (can_serialize)
               binding->addr = addr;
               if (size >= 0) {
      BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, size);
   PUSH_DATAh(push, addr);
      }
      }
      static const void *
   nvc0_screen_get_compiler_options(struct pipe_screen *pscreen,
               {
      struct nvc0_screen *screen = nvc0_screen(pscreen);
   if (ir == PIPE_SHADER_IR_NIR)
            }
      #define FAIL_SCREEN_INIT(str, err)                    \
      do {                                               \
      NOUVEAU_ERR(str, err);                          \
            struct nouveau_screen *
   nvc0_screen_create(struct nouveau_device *dev)
   {
      struct nvc0_screen *screen;
   struct pipe_screen *pscreen;
            struct nouveau_pushbuf *push;
   uint64_t value;
   uint32_t flags;
   int ret;
            switch (dev->chipset & ~0xf) {
   case 0xc0:
   case 0xd0:
   case 0xe0:
   case 0xf0:
   case 0x100:
   case 0x110:
   case 0x120:
   case 0x130:
   case 0x140:
   case 0x160:
   case 0x170:
   case 0x190:
         default:
                  screen = CALLOC_STRUCT(nvc0_screen);
   if (!screen)
         pscreen = &screen->base.base;
                     ret = nouveau_screen_init(&screen->base, dev);
   if (ret)
         chan = screen->base.channel;
   push = screen->base.pushbuf;
            /* TODO: could this be higher on Kepler+? how does reclocking vs no
   * reclocking affect performance?
   * TODO: could this be higher on Fermi?
   */
   if (dev->chipset >= 0xe0)
            screen->base.vidmem_bindings |= PIPE_BIND_CONSTANT_BUFFER |
      PIPE_BIND_SHADER_BUFFER |
   PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER |
      screen->base.sysmem_bindings |=
            if (screen->base.vram_domain & NOUVEAU_BO_GART) {
      screen->base.sysmem_bindings |= screen->base.vidmem_bindings;
               pscreen->context_create = nvc0_create;
   pscreen->is_format_supported = nvc0_screen_is_format_supported;
   pscreen->get_param = nvc0_screen_get_param;
   pscreen->get_shader_param = nvc0_screen_get_shader_param;
   pscreen->get_paramf = nvc0_screen_get_paramf;
   pscreen->get_sample_pixel_grid = nvc0_screen_get_sample_pixel_grid;
   pscreen->get_driver_query_info = nvc0_screen_get_driver_query_info;
   pscreen->get_driver_query_group_info = nvc0_screen_get_driver_query_group_info;
   /* nir stuff */
                     screen->base.base.get_video_param = nouveau_vp3_screen_get_video_param;
            flags = NOUVEAU_BO_GART | NOUVEAU_BO_MAP;
   if (screen->base.drm->version >= 0x01000202)
            ret = nouveau_bo_new(dev, flags, 0, 4096, NULL, &screen->fence.bo);
   if (ret)
         BO_MAP(&screen->base, screen->fence.bo, 0, NULL);
   screen->fence.map = screen->fence.bo->map;
   screen->base.fence.emit = nvc0_screen_fence_emit;
            if (dev->chipset < 0x140) {
      ret = nouveau_object_new(chan, (dev->chipset < 0xe0) ? 0x1f906e : 0x906e,
         if (ret)
            BEGIN_NVC0(push, SUBC_SW(NV01_SUBCHAN_OBJECT), 1);
               const struct nouveau_mclass m2mfs[] = {
      { NVF0_P2MF_CLASS, -1 },
   { NVE4_P2MF_CLASS, -1 },
   { NVC0_M2MF_CLASS, -1 },
               ret = nouveau_object_mclass(chan, m2mfs);
   if (ret < 0)
            ret = nouveau_object_new(chan, 0xbeef323f, m2mfs[ret].oclass, NULL, 0,
         if (ret)
            BEGIN_NVC0(push, SUBC_M2MF(NV01_SUBCHAN_OBJECT), 1);
            if (screen->m2mf->oclass >= NVE4_P2MF_CLASS) {
      const struct nouveau_mclass copys[] = {
      {  AMPERE_DMA_COPY_B, -1 },
   {  AMPERE_DMA_COPY_A, -1 },
   {  TURING_DMA_COPY_A, -1 },
   {   VOLTA_DMA_COPY_A, -1 },
   {  PASCAL_DMA_COPY_B, -1 },
   {  PASCAL_DMA_COPY_A, -1 },
   { MAXWELL_DMA_COPY_A, -1 },
   {  KEPLER_DMA_COPY_A, -1 },
               ret = nouveau_object_mclass(chan, copys);
   if (ret < 0)
            ret = nouveau_object_new(chan, 0, copys[ret].oclass, NULL, 0, &screen->copy);
   if (ret)
            BEGIN_NVC0(push, SUBC_COPY(NV01_SUBCHAN_OBJECT), 1);
               ret = nouveau_object_new(chan, 0xbeef902d, NVC0_2D_CLASS, NULL, 0,
         if (ret)
            BEGIN_NVC0(push, SUBC_2D(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push, screen->eng2d->oclass);
   BEGIN_NVC0(push, SUBC_2D(NVC0_2D_SINGLE_GPC), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_2D(OPERATION), 1);
   PUSH_DATA (push, NV50_2D_OPERATION_SRCCOPY);
   BEGIN_NVC0(push, NVC0_2D(CLIP_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_2D(COLOR_KEY_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_2D(SET_PIXELS_FROM_MEMORY_CORRAL_SIZE), 1);
   PUSH_DATA (push, 0x3f);
   BEGIN_NVC0(push, NVC0_2D(SET_PIXELS_FROM_MEMORY_SAFE_OVERLAP), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_2D(COND_MODE), 1);
            BEGIN_NVC0(push, SUBC_2D(NVC0_GRAPH_NOTIFY_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->fence.bo->offset + 16);
            const struct nouveau_mclass threeds[] = {
      { AD102_3D_CLASS, -1 },
   { GA102_3D_CLASS, -1 },
   { TU102_3D_CLASS, -1 },
   { GV100_3D_CLASS, -1 },
   { GP102_3D_CLASS, -1 },
   { GP100_3D_CLASS, -1 },
   { GM200_3D_CLASS, -1 },
   { GM107_3D_CLASS, -1 },
   {  NVF0_3D_CLASS, -1 },
   {  NVEA_3D_CLASS, -1 },
   {  NVE4_3D_CLASS, -1 },
   {  NVC8_3D_CLASS, -1 },
   {  NVC1_3D_CLASS, -1 },
   {  NVC0_3D_CLASS, -1 },
               ret = nouveau_object_mclass(chan, threeds);
   if (ret < 0)
            ret = nouveau_object_new(chan, 0xbeef003d, threeds[ret].oclass, NULL, 0,
         if (ret)
                  BEGIN_NVC0(push, SUBC_3D(NV01_SUBCHAN_OBJECT), 1);
            BEGIN_NVC0(push, NVC0_3D(COND_MODE), 1);
            if (debug_get_bool_option("NOUVEAU_SHADER_WATCHDOG", true)) {
      /* kill shaders after about 1 second (at 100 MHz) */
   BEGIN_NVC0(push, NVC0_3D(WATCHDOG_TIMER), 1);
               IMMED_NVC0(push, NVC0_3D(ZETA_COMP_ENABLE),
         BEGIN_NVC0(push, NVC0_3D(RT_COMP_ENABLE(0)), 8);
   for (i = 0; i < 8; ++i)
            BEGIN_NVC0(push, NVC0_3D(RT_CONTROL), 1);
            BEGIN_NVC0(push, NVC0_3D(CSAA_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(MULTISAMPLE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(MULTISAMPLE_MODE), 1);
   PUSH_DATA (push, NVC0_3D_MULTISAMPLE_MODE_MS1);
   BEGIN_NVC0(push, NVC0_3D(MULTISAMPLE_CTRL), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(LINE_WIDTH_SEPARATE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(PRIM_RESTART_WITH_DRAW_ARRAYS), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(BLEND_SEPARATE_ALPHA), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(BLEND_ENABLE_COMMON), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(SHADE_MODEL), 1);
   PUSH_DATA (push, NVC0_3D_SHADE_MODEL_SMOOTH);
   if (screen->eng3d->oclass < NVE4_3D_CLASS) {
         } else if (screen->eng3d->oclass < GA102_3D_CLASS) {
      BEGIN_NVC0(push, NVE4_3D(TEX_CB_INDEX), 1);
      }
   BEGIN_NVC0(push, NVC0_3D(CALL_LIMIT_LOG), 1);
   PUSH_DATA (push, 8); /* 128 */
   BEGIN_NVC0(push, NVC0_3D(ZCULL_STATCTRS_ENABLE), 1);
   PUSH_DATA (push, 1);
   if (screen->eng3d->oclass >= NVC1_3D_CLASS) {
      BEGIN_NVC0(push, NVC0_3D(CACHE_SPLIT), 1);
                        ret = nvc0_screen_resize_text_area(screen, push, 1 << 19);
   if (ret)
            /* 6 user uniform areas, 6 driver areas, and 1 for the runout */
   ret = nouveau_bo_new(dev, NV_VRAM_DOMAIN(&screen->base), 1 << 12, 13 << 16, NULL,
         if (ret)
                     /* return { 0.0, 0.0, 0.0, 0.0 } for out-of-bounds vtxbuf access */
   BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, 256);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_RUNOUT_INFO);
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_RUNOUT_INFO);
   BEGIN_1IC0(push, NVC0_3D(CB_POS), 5);
   PUSH_DATA (push, 0);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   BEGIN_NVC0(push, NVC0_3D(VERTEX_RUNOUT_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_RUNOUT_INFO);
            if (screen->base.drm->version >= 0x01000101) {
      ret = nouveau_getparam(dev, NOUVEAU_GETPARAM_GRAPH_UNITS, &value);
   if (ret)
      } else {
      if (dev->chipset >= 0xe0 && dev->chipset < 0xf0)
         else
      }
   screen->gpc_count = value & 0x000000ff;
   screen->mp_count = value >> 8;
            ret = nvc0_screen_resize_tls_area(screen, 128 * 16, 0, 0x200);
   if (ret)
            BEGIN_NVC0(push, NVC0_3D(TEMP_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, screen->tls->offset);
   PUSH_DATA (push, screen->tls->offset);
   PUSH_DATA (push, screen->tls->size >> 32);
   PUSH_DATA (push, screen->tls->size);
   BEGIN_NVC0(push, NVC0_3D(WARP_TEMP_ALLOC), 1);
   PUSH_DATA (push, 0);
   /* Reduce likelihood of collision with real buffers by placing the hole at
   * the top of the 4G area. This will have to be dealt with for real
   * eventually by blocking off that area from the VM.
   */
   BEGIN_NVC0(push, NVC0_3D(LOCAL_BASE), 1);
            if (screen->eng3d->oclass < GM107_3D_CLASS) {
      ret = nouveau_bo_new(dev, NV_VRAM_DOMAIN(&screen->base), 1 << 17, 1 << 20, NULL,
         if (ret)
            BEGIN_NVC0(push, NVC0_3D(VERTEX_QUARANTINE_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->poly_cache->offset);
   PUSH_DATA (push, screen->poly_cache->offset);
               ret = nouveau_bo_new(dev, NV_VRAM_DOMAIN(&screen->base), 1 << 17, 1 << 17, NULL,
         if (ret)
            BEGIN_NVC0(push, NVC0_3D(TIC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset);
   PUSH_DATA (push, screen->txc->offset);
   PUSH_DATA (push, NVC0_TIC_MAX_ENTRIES - 1);
   if (screen->eng3d->oclass >= GM107_3D_CLASS) {
      screen->tic.maxwell = true;
   if (screen->eng3d->oclass == GM107_3D_CLASS) {
      screen->tic.maxwell =
                        BEGIN_NVC0(push, NVC0_3D(TSC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset + 65536);
   PUSH_DATA (push, screen->txc->offset + 65536);
            BEGIN_NVC0(push, NVC0_3D(SCREEN_Y_CONTROL), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(WINDOW_OFFSET_X), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(ZCULL_REGION), 1); /* deactivate ZCULL */
            BEGIN_NVC0(push, NVC0_3D(CLIP_RECTS_MODE), 1);
   PUSH_DATA (push, NVC0_3D_CLIP_RECTS_MODE_INSIDE_ANY);
   BEGIN_NVC0(push, NVC0_3D(CLIP_RECT_HORIZ(0)), 8 * 2);
   for (i = 0; i < 8 * 2; ++i)
         BEGIN_NVC0(push, NVC0_3D(CLIP_RECTS_EN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(CLIPID_ENABLE), 1);
            /* neither scissors, viewport nor stencil mask should affect clears */
   BEGIN_NVC0(push, NVC0_3D(CLEAR_FLAGS), 1);
            BEGIN_NVC0(push, NVC0_3D(VIEWPORT_TRANSFORM_EN), 1);
   PUSH_DATA (push, 1);
   for (i = 0; i < NVC0_MAX_VIEWPORTS; i++) {
      BEGIN_NVC0(push, NVC0_3D(DEPTH_RANGE_NEAR(i)), 2);
   PUSH_DATAf(push, 0.0f);
      }
   BEGIN_NVC0(push, NVC0_3D(VIEW_VOLUME_CLIP_CTRL), 1);
            /* We use scissors instead of exact view volume clipping,
   * so they're always enabled.
   */
   for (i = 0; i < NVC0_MAX_VIEWPORTS; i++) {
      BEGIN_NVC0(push, NVC0_3D(SCISSOR_ENABLE(i)), 3);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 16384 << 16);
                  #define MK_MACRO(m, n) i = nvc0_graph_set_macro(screen, m, i, sizeof(n), n);
            i = 0;
   MK_MACRO(NVC0_3D_MACRO_VERTEX_ARRAY_PER_INSTANCE, mme9097_per_instance_bf);
   MK_MACRO(NVC0_3D_MACRO_BLEND_ENABLES, mme9097_blend_enables);
   MK_MACRO(NVC0_3D_MACRO_VERTEX_ARRAY_SELECT, mme9097_vertex_array_select);
   MK_MACRO(NVC0_3D_MACRO_TEP_SELECT, mme9097_tep_select);
   MK_MACRO(NVC0_3D_MACRO_GP_SELECT, mme9097_gp_select);
   MK_MACRO(NVC0_3D_MACRO_POLYGON_MODE_FRONT, mme9097_poly_mode_front);
   MK_MACRO(NVC0_3D_MACRO_POLYGON_MODE_BACK, mme9097_poly_mode_back);
   MK_MACRO(NVC0_3D_MACRO_DRAW_ARRAYS_INDIRECT, mme9097_draw_arrays_indirect);
   MK_MACRO(NVC0_3D_MACRO_DRAW_ELEMENTS_INDIRECT, mme9097_draw_elts_indirect);
   MK_MACRO(NVC0_3D_MACRO_DRAW_ARRAYS_INDIRECT_COUNT, mme9097_draw_arrays_indirect_count);
   MK_MACRO(NVC0_3D_MACRO_DRAW_ELEMENTS_INDIRECT_COUNT, mme9097_draw_elts_indirect_count);
   MK_MACRO(NVC0_3D_MACRO_QUERY_BUFFER_WRITE, mme9097_query_buffer_write);
   MK_MACRO(NVC0_3D_MACRO_CONSERVATIVE_RASTER_STATE, mme9097_conservative_raster_state);
   MK_MACRO(NVC0_3D_MACRO_SET_PRIV_REG, mme9097_set_priv_reg);
   MK_MACRO(NVC0_3D_MACRO_COMPUTE_COUNTER, mme9097_compute_counter);
   MK_MACRO(NVC0_3D_MACRO_COMPUTE_COUNTER_TO_QUERY, mme9097_compute_counter_to_query);
         #undef MK_MACRO
   #define MK_MACRO(m, n) i = tu102_graph_set_macro(screen, m, i, sizeof(n), n);
            i = 0;
   MK_MACRO(NVC0_3D_MACRO_VERTEX_ARRAY_PER_INSTANCE, mmec597_per_instance_bf);
   MK_MACRO(NVC0_3D_MACRO_BLEND_ENABLES, mmec597_blend_enables);
   MK_MACRO(NVC0_3D_MACRO_VERTEX_ARRAY_SELECT, mmec597_vertex_array_select);
   MK_MACRO(NVC0_3D_MACRO_TEP_SELECT, mmec597_tep_select);
   MK_MACRO(NVC0_3D_MACRO_GP_SELECT, mmec597_gp_select);
   MK_MACRO(NVC0_3D_MACRO_POLYGON_MODE_FRONT, mmec597_poly_mode_front);
   MK_MACRO(NVC0_3D_MACRO_POLYGON_MODE_BACK, mmec597_poly_mode_back);
   MK_MACRO(NVC0_3D_MACRO_DRAW_ARRAYS_INDIRECT, mmec597_draw_arrays_indirect);
   MK_MACRO(NVC0_3D_MACRO_DRAW_ELEMENTS_INDIRECT, mmec597_draw_elts_indirect);
   MK_MACRO(NVC0_3D_MACRO_DRAW_ARRAYS_INDIRECT_COUNT, mmec597_draw_arrays_indirect_count);
   MK_MACRO(NVC0_3D_MACRO_DRAW_ELEMENTS_INDIRECT_COUNT, mmec597_draw_elts_indirect_count);
   MK_MACRO(NVC0_3D_MACRO_QUERY_BUFFER_WRITE, mmec597_query_buffer_write);
   MK_MACRO(NVC0_3D_MACRO_CONSERVATIVE_RASTER_STATE, mmec597_conservative_raster_state);
   MK_MACRO(NVC0_3D_MACRO_SET_PRIV_REG, mmec597_set_priv_reg);
   MK_MACRO(NVC0_3D_MACRO_COMPUTE_COUNTER, mmec597_compute_counter);
               BEGIN_NVC0(push, NVC0_3D(RASTERIZE_ENABLE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(RT_SEPARATE_FRAG_DATA), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(MACRO_GP_SELECT), 1);
   PUSH_DATA (push, 0x40);
   BEGIN_NVC0(push, NVC0_3D(LAYER), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(MACRO_TEP_SELECT), 1);
   PUSH_DATA (push, 0x30);
   BEGIN_NVC0(push, NVC0_3D(PATCH_VERTICES), 1);
   PUSH_DATA (push, 3);
   BEGIN_NVC0(push, NVC0_3D(SP_SELECT(2)), 1);
   PUSH_DATA (push, 0x20);
   BEGIN_NVC0(push, NVC0_3D(SP_SELECT(0)), 1);
   PUSH_DATA (push, 0x00);
            BEGIN_NVC0(push, NVC0_3D(POINT_COORD_REPLACE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(POINT_RASTER_RULES), 1);
                     if (nvc0_screen_init_compute(screen))
            /* XXX: Compute and 3D are somehow aliased on Fermi. */
   for (i = 0; i < 5; ++i) {
      unsigned j = 0;
   for (j = 0; j < 16; j++)
            /* TIC and TSC entries for each unit (nve4+ only) */
   /* auxiliary constants (6 user clip planes, base instance id) */
   nvc0_screen_bind_cb_3d(screen, push, NULL, i, 15, NVC0_CB_AUX_SIZE,
         if (screen->eng3d->oclass >= NVE4_3D_CLASS) {
      unsigned j;
   BEGIN_1IC0(push, NVC0_3D(CB_POS), 9);
   PUSH_DATA (push, NVC0_CB_AUX_UNK_INFO);
   for (j = 0; j < 8; ++j)
      } else {
      BEGIN_NVC0(push, NVC0_3D(TEX_LIMITS(i)), 1);
               /* MS sample coordinate offsets: these do not work with _ALT modes ! */
   BEGIN_1IC0(push, NVC0_3D(CB_POS), 1 + 2 * 8);
   PUSH_DATA (push, NVC0_CB_AUX_MS_INFO);
   PUSH_DATA (push, 0); /* 0 */
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1); /* 1 */
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0); /* 2 */
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 1); /* 3 */
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 2); /* 4 */
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 3); /* 5 */
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 2); /* 6 */
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 3); /* 7 */
      }
   BEGIN_NVC0(push, NVC0_3D(LINKED_TSC), 1);
            /* requires Nvidia provided firmware */
   if (screen->eng3d->oclass >= GM200_3D_CLASS) {
      unsigned reg = screen->eng3d->oclass >= GV100_3D_CLASS ? 0x419ba4 : 0x419f78;
   BEGIN_1IC0(push, NVC0_3D(MACRO_SET_PRIV_REG), 3);
   PUSH_DATA (push, reg);
   PUSH_DATA (push, 0x00000000);
                        screen->tic.entries = CALLOC(
         NVC0_TIC_MAX_ENTRIES + NVC0_TSC_MAX_ENTRIES + NVE4_IMG_MAX_HANDLES,
   screen->tsc.entries = screen->tic.entries + NVC0_TIC_MAX_ENTRIES;
            if (!nvc0_blitter_create(screen))
                  fail:
      screen->base.base.context_create = NULL;
      }
      int
   nvc0_screen_tic_alloc(struct nvc0_screen *screen, void *entry)
   {
               while (screen->tic.lock[i / 32] & (1 << (i % 32)))
                     if (screen->tic.entries[i])
            screen->tic.entries[i] = entry;
      }
      int
   nvc0_screen_tsc_alloc(struct nvc0_screen *screen, void *entry)
   {
               while (screen->tsc.lock[i / 32] & (1 << (i % 32)))
                     if (screen->tsc.entries[i])
            screen->tsc.entries[i] = entry;
      }
