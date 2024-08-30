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
      #include <errno.h>
   #include <xf86drm.h>
   #include <nouveau_drm.h>
   #include "util/format/u_format.h"
   #include "util/format/u_format_s3tc.h"
   #include "util/u_screen.h"
   #include "pipe/p_screen.h"
      #include "nv50_ir_driver.h"
      #include "nv50/nv50_context.h"
   #include "nv50/nv50_screen.h"
      #include "nouveau_vp3_video.h"
      #include "nv_object.xml.h"
      /* affected by LOCAL_WARPS_LOG_ALLOC / LOCAL_WARPS_NO_CLAMP */
   #define LOCAL_WARPS_ALLOC 32
   /* affected by STACK_WARPS_LOG_ALLOC / STACK_WARPS_NO_CLAMP */
   #define STACK_WARPS_ALLOC 32
      #define THREADS_IN_WARP 32
      static bool
   nv50_screen_is_format_supported(struct pipe_screen *pscreen,
                                 {
      if (sample_count > 8)
         if (!(0x117 & (1 << sample_count))) /* 0, 1, 2, 4 or 8 */
         if (sample_count == 8 && util_format_get_blocksizebits(format) >= 128)
            if (MAX2(1, sample_count) != MAX2(1, storage_sample_count))
            /* Short-circuit the rest of the logic -- this is used by the gallium frontend
   * to determine valid MS levels in a no-attachments scenario.
   */
   if (format == PIPE_FORMAT_NONE && bindings & PIPE_BIND_RENDER_TARGET)
            switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      if (nv50_screen(pscreen)->tesla->oclass < NVA0_3D_CLASS)
            default:
                  if (bindings & PIPE_BIND_LINEAR)
      if (util_format_is_depth_or_stencil(format) ||
      (target != PIPE_TEXTURE_1D &&
   target != PIPE_TEXTURE_2D &&
   target != PIPE_TEXTURE_RECT) ||
            /* shared is always supported */
   bindings &= ~(PIPE_BIND_LINEAR |
            if (bindings & PIPE_BIND_INDEX_BUFFER) {
      if (format != PIPE_FORMAT_R8_UINT &&
      format != PIPE_FORMAT_R16_UINT &&
   format != PIPE_FORMAT_R32_UINT)
                  return (( nv50_format_table[format].usage |
      }
      static int
   nv50_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
   {
      struct nouveau_screen *screen = nouveau_screen(pscreen);
   const uint16_t class_3d = screen->class_3d;
            switch (param) {
   /* non-boolean caps */
   case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
         case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
         case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
         case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
         case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
   case PIPE_CAP_MIN_TEXEL_OFFSET:
         case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
   case PIPE_CAP_MAX_TEXEL_OFFSET:
         case PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT:
         case PIPE_CAP_GLSL_FEATURE_LEVEL:
         case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
         case PIPE_CAP_ESSL_FEATURE_LEVEL:
         case PIPE_CAP_MAX_RENDER_TARGETS:
         case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
         case PIPE_CAP_MAX_COMBINED_SHADER_OUTPUT_RESOURCES:
         case PIPE_CAP_VIEWPORT_SUBPIXEL_BITS:
   case PIPE_CAP_RASTERIZER_SUBPIXEL_BITS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
         case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
         case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
   case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
         case PIPE_CAP_MAX_VERTEX_STREAMS:
         case PIPE_CAP_MAX_GS_INVOCATIONS:
         case PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT:
         case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
         case PIPE_CAP_MAX_VERTEX_ELEMENT_SRC_OFFSET:
         case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
         case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
         case PIPE_CAP_MAX_VIEWPORTS:
         case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
         case PIPE_CAP_ENDIANNESS:
         case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
         case PIPE_CAP_MAX_WINDOW_RECTANGLES:
         case PIPE_CAP_MAX_TEXTURE_UPLOAD_MEMORY_BUDGET:
         case PIPE_CAP_MAX_VARYINGS:
         case PIPE_CAP_MAX_VERTEX_BUFFERS:
         case PIPE_CAP_GL_BEGIN_END_BUFFER_SIZE:
         case PIPE_CAP_MAX_TEXTURE_MB:
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
   case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
   case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_VERTEX_COLOR_CLAMPED:
   case PIPE_CAP_QUERY_TIMESTAMP:
   case PIPE_CAP_QUERY_TIME_ELAPSED:
   case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
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
   case PIPE_CAP_USER_VERTEX_BUFFERS:
   case PIPE_CAP_TEXTURE_MULTISAMPLE:
   case PIPE_CAP_FS_FINE_DERIVATIVE:
   case PIPE_CAP_SAMPLER_VIEW_TARGET:
   case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
   case PIPE_CAP_CLIP_HALFZ:
   case PIPE_CAP_MEMOBJ:
   case PIPE_CAP_POLYGON_OFFSET_CLAMP:
   case PIPE_CAP_QUERY_PIPELINE_STATISTICS:
   case PIPE_CAP_TEXTURE_FLOAT_LINEAR:
   case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
   case PIPE_CAP_DEPTH_BOUNDS_TEST:
   case PIPE_CAP_TEXTURE_QUERY_SAMPLES:
   case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
   case PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL:
   case PIPE_CAP_INVALIDATE_BUFFER:
   case PIPE_CAP_STRING_MARKER:
   case PIPE_CAP_CULL_DISTANCE:
   case PIPE_CAP_SHADER_ARRAY_COMPONENTS:
   case PIPE_CAP_LEGACY_MATH_RULES:
   case PIPE_CAP_TGSI_TEX_TXF_LZ:
   case PIPE_CAP_SHADER_CLOCK:
   case PIPE_CAP_CAN_BIND_CONST_BUFFER_AS_VERTEX:
   case PIPE_CAP_TGSI_DIV:
   case PIPE_CAP_CLEAR_SCISSORED:
   case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
   case PIPE_CAP_COMPUTE:
   case PIPE_CAP_QUERY_MEMORY_INFO:
            case PIPE_CAP_ALPHA_TEST:
      /* nvc0 has fixed function alpha test support, but nv50 doesn't.  If we
   * don't have it, then the frontend will lower it for us.
   */
         case PIPE_CAP_TEXTURE_TRANSFER_MODES:
         case PIPE_CAP_SEAMLESS_CUBE_MAP:
         /* supported on nva0+ */
   case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
         /* supported on nva3+ */
   case PIPE_CAP_CUBE_MAP_ARRAY:
   case PIPE_CAP_INDEP_BLEND_FUNC:
   case PIPE_CAP_TEXTURE_QUERY_LOD:
   case PIPE_CAP_SAMPLE_SHADING:
   case PIPE_CAP_FORCE_PERSAMPLE_INTERP:
            case PIPE_CAP_PCI_GROUP:
   case PIPE_CAP_PCI_BUS:
   case PIPE_CAP_PCI_DEVICE:
   case PIPE_CAP_PCI_FUNCTION:
            case PIPE_CAP_MULTISAMPLE_Z_RESOLVE: /* potentially supported on some hw */
   case PIPE_CAP_INTEGER_MULTIPLY_32X16: /* could be done */
   case PIPE_CAP_MAP_UNSYNCHRONIZED_THREAD_SAFE: /* when we fix MT stuff */
   case PIPE_CAP_NIR_IMAGES_AS_DEREF:
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
   nv50_screen_get_shader_param(struct pipe_screen *pscreen,
               {
      switch (shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_FRAGMENT:
   case PIPE_SHADER_COMPUTE:
         default:
                  switch (param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         case PIPE_SHADER_CAP_MAX_INPUTS:
      if (shader == PIPE_SHADER_VERTEX)
            case PIPE_SHADER_CAP_MAX_OUTPUTS:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
         case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
         case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         case PIPE_SHADER_CAP_MAX_TEMPS:
         case PIPE_SHADER_CAP_CONT_SUPPORTED:
         case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
         case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
   case PIPE_SHADER_CAP_SUBROUTINES:
         case PIPE_SHADER_CAP_INTEGERS:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
         case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
         case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
         case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
         case PIPE_SHADER_CAP_SUPPORTED_IRS:
         case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         default:
      NOUVEAU_ERR("unknown PIPE_SHADER_CAP %d\n", param);
         }
      static float
   nv50_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
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
   nv50_screen_get_compute_param(struct pipe_screen *pscreen,
               {
      struct nv50_screen *screen = nv50_screen(pscreen);
         #define RET(x) do {                  \
      if (data)                         \
            } while (0)
         switch (param) {
   case PIPE_COMPUTE_CAP_GRID_DIMENSION:
         case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
         case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
         case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
         case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE: /* g0-15[] */
         case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE: /* s[] */
         case PIPE_COMPUTE_CAP_MAX_PRIVATE_SIZE: /* l[] */
         case PIPE_COMPUTE_CAP_MAX_INPUT_SIZE: /* c[], arbitrary limit */
         case PIPE_COMPUTE_CAP_SUBGROUP_SIZES:
         case PIPE_COMPUTE_CAP_MAX_SUBGROUPS:
         case PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE:
         case PIPE_COMPUTE_CAP_IMAGES_SUPPORTED:
         case PIPE_COMPUTE_CAP_MAX_COMPUTE_UNITS:
         case PIPE_COMPUTE_CAP_MAX_CLOCK_FREQUENCY:
         case PIPE_COMPUTE_CAP_ADDRESS_BITS:
         case PIPE_COMPUTE_CAP_MAX_VARIABLE_THREADS_PER_BLOCK:
         default:
               #undef RET
   }
      static void
   nv50_screen_destroy(struct pipe_screen *pscreen)
   {
               if (!nouveau_drm_screen_unref(&screen->base))
            if (screen->blitter)
         if (screen->pm.prog) {
      screen->pm.prog->code = NULL; /* hardcoded, don't FREE */
   nv50_program_destroy(NULL, screen->pm.prog);
               nouveau_bo_ref(NULL, &screen->code);
   nouveau_bo_ref(NULL, &screen->tls_bo);
   nouveau_bo_ref(NULL, &screen->stack_bo);
   nouveau_bo_ref(NULL, &screen->txc);
   nouveau_bo_ref(NULL, &screen->uniforms);
            nouveau_heap_destroy(&screen->vp_code_heap);
   nouveau_heap_destroy(&screen->gp_code_heap);
                     nouveau_object_del(&screen->tesla);
   nouveau_object_del(&screen->eng2d);
   nouveau_object_del(&screen->m2mf);
   nouveau_object_del(&screen->compute);
            nouveau_screen_fini(&screen->base);
               }
      static void
   nv50_screen_fence_emit(struct pipe_context *pcontext, u32 *sequence,
         {
      struct nv50_context *nv50 = nv50_context(pcontext);
   struct nv50_screen *screen = nv50->screen;
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
            /* we need to do it after possible flush in MARK_RING */
            assert(PUSH_AVAIL(push) + push->rsvd_kick >= 5);
   PUSH_DATA (push, NV50_FIFO_PKHDR(NV50_3D(QUERY_ADDRESS_HIGH), 4));
   PUSH_DATAh(push, screen->fence.bo->offset);
   PUSH_DATA (push, screen->fence.bo->offset);
   PUSH_DATA (push, *sequence);
   PUSH_DATA (push, NV50_3D_QUERY_GET_MODE_WRITE_UNK0 |
                  NV50_3D_QUERY_GET_UNK4 |
   NV50_3D_QUERY_GET_UNIT_CROP |
            }
      static u32
   nv50_screen_fence_update(struct pipe_screen *pscreen)
   {
         }
      static void
   nv50_screen_init_hwctx(struct nv50_screen *screen)
   {
      struct nouveau_pushbuf *push = screen->base.pushbuf;
   struct nv04_fifo *fifo;
                     BEGIN_NV04(push, SUBC_M2MF(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push, screen->m2mf->handle);
   BEGIN_NV04(push, SUBC_M2MF(NV03_M2MF_DMA_NOTIFY), 3);
   PUSH_DATA (push, screen->sync->handle);
   PUSH_DATA (push, fifo->vram);
            BEGIN_NV04(push, SUBC_2D(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push, screen->eng2d->handle);
   BEGIN_NV04(push, NV50_2D(DMA_NOTIFY), 4);
   PUSH_DATA (push, screen->sync->handle);
   PUSH_DATA (push, fifo->vram);
   PUSH_DATA (push, fifo->vram);
   PUSH_DATA (push, fifo->vram);
   BEGIN_NV04(push, NV50_2D(OPERATION), 1);
   PUSH_DATA (push, NV50_2D_OPERATION_SRCCOPY);
   BEGIN_NV04(push, NV50_2D(CLIP_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_2D(COLOR_KEY_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_2D(SET_PIXELS_FROM_MEMORY_SAFE_OVERLAP), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_2D(COND_MODE), 1);
            BEGIN_NV04(push, SUBC_3D(NV01_SUBCHAN_OBJECT), 1);
            BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
            BEGIN_NV04(push, NV50_3D(DMA_NOTIFY), 1);
   PUSH_DATA (push, screen->sync->handle);
   BEGIN_NV04(push, NV50_3D(DMA_ZETA), 11);
   for (i = 0; i < 11; ++i)
         BEGIN_NV04(push, NV50_3D(DMA_COLOR(0)), NV50_3D_DMA_COLOR__LEN);
   for (i = 0; i < NV50_3D_DMA_COLOR__LEN; ++i)
            BEGIN_NV04(push, NV50_3D(REG_MODE), 1);
   PUSH_DATA (push, NV50_3D_REG_MODE_STRIPED);
   BEGIN_NV04(push, NV50_3D(UNK1400_LANES), 1);
            if (debug_get_bool_option("NOUVEAU_SHADER_WATCHDOG", true)) {
      BEGIN_NV04(push, NV50_3D(WATCHDOG_TIMER), 1);
               BEGIN_NV04(push, NV50_3D(ZETA_COMP_ENABLE), 1);
            BEGIN_NV04(push, NV50_3D(RT_COMP_ENABLE(0)), 8);
   for (i = 0; i < 8; ++i)
            BEGIN_NV04(push, NV50_3D(RT_CONTROL), 1);
            BEGIN_NV04(push, NV50_3D(CSAA_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(MULTISAMPLE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(MULTISAMPLE_MODE), 1);
   PUSH_DATA (push, NV50_3D_MULTISAMPLE_MODE_MS1);
   BEGIN_NV04(push, NV50_3D(MULTISAMPLE_CTRL), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(PRIM_RESTART_WITH_DRAW_ARRAYS), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(BLEND_SEPARATE_ALPHA), 1);
            if (screen->tesla->oclass >= NVA0_3D_CLASS) {
      BEGIN_NV04(push, SUBC_3D(NVA0_3D_TEX_MISC), 1);
               BEGIN_NV04(push, NV50_3D(SCREEN_Y_CONTROL), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(WINDOW_OFFSET_X), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(ZCULL_REGION), 1);
            BEGIN_NV04(push, NV50_3D(VP_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->code->offset + (NV50_SHADER_STAGE_VERTEX << NV50_CODE_BO_SIZE_LOG2));
            BEGIN_NV04(push, NV50_3D(FP_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->code->offset + (NV50_SHADER_STAGE_FRAGMENT << NV50_CODE_BO_SIZE_LOG2));
            BEGIN_NV04(push, NV50_3D(GP_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->code->offset + (NV50_SHADER_STAGE_GEOMETRY << NV50_CODE_BO_SIZE_LOG2));
            BEGIN_NV04(push, NV50_3D(LOCAL_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->tls_bo->offset);
   PUSH_DATA (push, screen->tls_bo->offset);
            BEGIN_NV04(push, NV50_3D(STACK_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->stack_bo->offset);
   PUSH_DATA (push, screen->stack_bo->offset);
            BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->uniforms->offset + (0 << 16));
   PUSH_DATA (push, screen->uniforms->offset + (0 << 16));
            BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->uniforms->offset + (1 << 16));
   PUSH_DATA (push, screen->uniforms->offset + (1 << 16));
            BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->uniforms->offset + (2 << 16));
   PUSH_DATA (push, screen->uniforms->offset + (2 << 16));
            BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->uniforms->offset + (4 << 16));
   PUSH_DATA (push, screen->uniforms->offset + (4 << 16));
            BEGIN_NI04(push, NV50_3D(SET_PROGRAM_CB), 3);
   PUSH_DATA (push, (NV50_CB_AUX << 12) | 0xf01);
   PUSH_DATA (push, (NV50_CB_AUX << 12) | 0xf21);
            /* return { 0.0, 0.0, 0.0, 0.0 } on out-of-bounds vtxbuf access */
   BEGIN_NV04(push, NV50_3D(CB_ADDR), 1);
   PUSH_DATA (push, (NV50_CB_AUX_RUNOUT_OFFSET << (8 - 2)) | NV50_CB_AUX);
   BEGIN_NI04(push, NV50_3D(CB_DATA(0)), 4);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   BEGIN_NV04(push, NV50_3D(VERTEX_RUNOUT_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->uniforms->offset + (4 << 16) + NV50_CB_AUX_RUNOUT_OFFSET);
            /* set the membar offset */
   BEGIN_NV04(push, NV50_3D(CB_ADDR), 1);
   PUSH_DATA (push, (NV50_CB_AUX_MEMBAR_OFFSET << (8 - 2)) | NV50_CB_AUX);
   BEGIN_NI04(push, NV50_3D(CB_DATA(0)), 1);
                     /* max TIC (bits 4:8) & TSC bindings, per program type */
   for (i = 0; i < NV50_MAX_3D_SHADER_STAGES; ++i) {
      BEGIN_NV04(push, NV50_3D(TEX_LIMITS(i)), 1);
               BEGIN_NV04(push, NV50_3D(TIC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset);
   PUSH_DATA (push, screen->txc->offset);
            BEGIN_NV04(push, NV50_3D(TSC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset + 65536);
   PUSH_DATA (push, screen->txc->offset + 65536);
            BEGIN_NV04(push, NV50_3D(LINKED_TSC), 1);
            BEGIN_NV04(push, NV50_3D(CLIP_RECTS_EN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(CLIP_RECTS_MODE), 1);
   PUSH_DATA (push, NV50_3D_CLIP_RECTS_MODE_INSIDE_ANY);
   BEGIN_NV04(push, NV50_3D(CLIP_RECT_HORIZ(0)), 8 * 2);
   for (i = 0; i < 8 * 2; ++i)
         BEGIN_NV04(push, NV50_3D(CLIPID_ENABLE), 1);
            BEGIN_NV04(push, NV50_3D(VIEWPORT_TRANSFORM_EN), 1);
   PUSH_DATA (push, 1);
   for (i = 0; i < NV50_MAX_VIEWPORTS; i++) {
      BEGIN_NV04(push, NV50_3D(DEPTH_RANGE_NEAR(i)), 2);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 1.0f);
   BEGIN_NV04(push, NV50_3D(VIEWPORT_HORIZ(i)), 2);
   PUSH_DATA (push, 8192 << 16);
                  #ifdef NV50_SCISSORS_CLIPPING
         #else
         #endif
         BEGIN_NV04(push, NV50_3D(CLEAR_FLAGS), 1);
            /* We use scissors instead of exact view volume clipping,
   * so they're always enabled.
   */
   for (i = 0; i < NV50_MAX_VIEWPORTS; i++) {
      BEGIN_NV04(push, NV50_3D(SCISSOR_ENABLE(i)), 3);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 8192 << 16);
               BEGIN_NV04(push, NV50_3D(RASTERIZE_ENABLE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(POINT_RASTER_RULES), 1);
   PUSH_DATA (push, NV50_3D_POINT_RASTER_RULES_OGL);
   BEGIN_NV04(push, NV50_3D(FRAG_COLOR_CLAMP_EN), 1);
   PUSH_DATA (push, 0x11111111);
   BEGIN_NV04(push, NV50_3D(EDGEFLAG), 1);
            BEGIN_NV04(push, NV50_3D(VB_ELEMENT_BASE), 1);
   PUSH_DATA (push, 0);
   if (screen->base.class_3d >= NV84_3D_CLASS) {
      BEGIN_NV04(push, NV84_3D(VERTEX_ID_BASE), 1);
               BEGIN_NV04(push, NV50_3D(UNK0FDC), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(UNK19C0), 1);
      }
      static int nv50_tls_alloc(struct nv50_screen *screen, unsigned tls_space,
         {
      struct nouveau_device *dev = screen->base.device;
            assert(tls_space % ONE_TEMP_SIZE == 0);
   screen->cur_tls_space = util_next_power_of_two(tls_space / ONE_TEMP_SIZE) *
         if (nouveau_mesa_debug)
      debug_printf("allocating space for %u temps\n",
      *tls_size = screen->cur_tls_space * util_next_power_of_two(screen->TPs) *
            ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16,
         if (ret) {
      NOUVEAU_ERR("Failed to allocate local bo: %d\n", ret);
                  }
      int nv50_tls_realloc(struct nv50_screen *screen, unsigned tls_space)
   {
      struct nouveau_pushbuf *push = screen->base.pushbuf;
   int ret;
            if (tls_space < screen->cur_tls_space)
         if (tls_space > screen->max_tls_space) {
      /* fixable by limiting number of warps (LOCAL_WARPS_LOG_ALLOC /
   * LOCAL_WARPS_NO_CLAMP) */
   NOUVEAU_ERR("Unsupported number of temporaries (%u > %u). Fixable if someone cares.\n",
         (unsigned)(tls_space / ONE_TEMP_SIZE),
               nouveau_bo_ref(NULL, &screen->tls_bo);
   ret = nv50_tls_alloc(screen, tls_space, &tls_size);
   if (ret)
            BEGIN_NV04(push, NV50_3D(LOCAL_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->tls_bo->offset);
   PUSH_DATA (push, screen->tls_bo->offset);
               }
      static const void *
   nv50_screen_get_compiler_options(struct pipe_screen *pscreen,
               {
      if (ir == PIPE_SHADER_IR_NIR)
            }
      struct nouveau_screen *
   nv50_screen_create(struct nouveau_device *dev)
   {
      struct nv50_screen *screen;
   struct pipe_screen *pscreen;
   struct nouveau_object *chan;
   uint64_t value;
   uint32_t tesla_class;
   unsigned stack_size;
            screen = CALLOC_STRUCT(nv50_screen);
   if (!screen)
         pscreen = &screen->base.base;
            simple_mtx_init(&screen->state_lock, mtx_plain);
   ret = nouveau_screen_init(&screen->base, dev);
   if (ret) {
      NOUVEAU_ERR("nouveau_screen_init failed: %d\n", ret);
               /* TODO: Prevent FIFO prefetch before transfer of index buffers and
   *  admit them to VRAM.
   */
   screen->base.vidmem_bindings |= PIPE_BIND_CONSTANT_BUFFER |
         screen->base.sysmem_bindings |=
                              pscreen->context_create = nv50_create;
   pscreen->is_format_supported = nv50_screen_is_format_supported;
   pscreen->get_param = nv50_screen_get_param;
   pscreen->get_shader_param = nv50_screen_get_shader_param;
   pscreen->get_paramf = nv50_screen_get_paramf;
   pscreen->get_compute_param = nv50_screen_get_compute_param;
   pscreen->get_driver_query_info = nv50_screen_get_driver_query_info;
            /* nir stuff */
                     if (screen->base.device->chipset < 0x84 ||
      debug_get_bool_option("NOUVEAU_PMPEG", false)) {
   /* PMPEG */
      } else if (screen->base.device->chipset < 0x98 ||
            /* VP2 */
   screen->base.base.get_video_param = nv84_screen_get_video_param;
      } else {
      /* VP3/4 */
   screen->base.base.get_video_param = nouveau_vp3_screen_get_video_param;
               ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0, 4096,
         if (ret) {
      NOUVEAU_ERR("Failed to allocate fence bo: %d\n", ret);
               BO_MAP(&screen->base, screen->fence.bo, 0, NULL);
   screen->fence.map = screen->fence.bo->map;
   screen->base.fence.emit = nv50_screen_fence_emit;
            ret = nouveau_object_new(chan, 0xbeef0301, NOUVEAU_NOTIFIER_CLASS,
               if (ret) {
      NOUVEAU_ERR("Failed to allocate notifier: %d\n", ret);
               ret = nouveau_object_new(chan, 0xbeef5039, NV50_M2MF_CLASS,
         if (ret) {
      NOUVEAU_ERR("Failed to allocate PGRAPH context for M2MF: %d\n", ret);
               ret = nouveau_object_new(chan, 0xbeef502d, NV50_2D_CLASS,
         if (ret) {
      NOUVEAU_ERR("Failed to allocate PGRAPH context for 2D: %d\n", ret);
               switch (dev->chipset & 0xf0) {
   case 0x50:
      tesla_class = NV50_3D_CLASS;
      case 0x80:
   case 0x90:
      tesla_class = NV84_3D_CLASS;
      case 0xa0:
      switch (dev->chipset) {
   case 0xa0:
   case 0xaa:
   case 0xac:
      tesla_class = NVA0_3D_CLASS;
      case 0xaf:
      tesla_class = NVAF_3D_CLASS;
      default:
      tesla_class = NVA3_3D_CLASS;
      }
      default:
      NOUVEAU_ERR("Not a known NV50 chipset: NV%02x\n", dev->chipset);
      }
            ret = nouveau_object_new(chan, 0xbeef5097, tesla_class,
         if (ret) {
      NOUVEAU_ERR("Failed to allocate PGRAPH context for 3D: %d\n", ret);
               /* This over-allocates by a page. The GP, which would execute at the end of
   * the last page, would trigger faults. The going theory is that it
   * prefetches up to a certain amount.
   */
   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16,
               if (ret) {
      NOUVEAU_ERR("Failed to allocate code bo: %d\n", ret);
               nouveau_heap_init(&screen->vp_code_heap, 0, 1 << NV50_CODE_BO_SIZE_LOG2);
   nouveau_heap_init(&screen->gp_code_heap, 0, 1 << NV50_CODE_BO_SIZE_LOG2);
                     screen->TPs = util_bitcount(value & 0xffff);
                     stack_size = util_next_power_of_two(screen->TPs) * screen->MPsInTP *
            ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16, stack_size, NULL,
         if (ret) {
      NOUVEAU_ERR("Failed to allocate stack bo: %d\n", ret);
               uint64_t size_of_one_temp = util_next_power_of_two(screen->TPs) *
         screen->MPsInTP * LOCAL_WARPS_ALLOC *  THREADS_IN_WARP *
   screen->max_tls_space = dev->vram_size / size_of_one_temp * ONE_TEMP_SIZE;
            /* hw can address max 64 KiB */
            uint64_t tls_size;
   unsigned tls_space = 4/*temps*/ * ONE_TEMP_SIZE;
   ret = nv50_tls_alloc(screen, tls_space, &tls_size);
   if (ret)
            if (nouveau_mesa_debug)
      debug_printf("TPs = %u, MPsInTP = %u, VRAM = %"PRIu64" MiB, tls_size = %"PRIu64" KiB\n",
         ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16, 5 << 16, NULL,
         if (ret) {
      NOUVEAU_ERR("Failed to allocate uniforms bo: %d\n", ret);
               ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16, 3 << 16, NULL,
         if (ret) {
      NOUVEAU_ERR("Failed to allocate TIC/TSC bo: %d\n", ret);
               screen->tic.entries = CALLOC(4096, sizeof(void *));
            if (!nv50_blitter_create(screen))
                     ret = nv50_screen_compute_setup(screen, screen->base.pushbuf);
   if (ret) {
      NOUVEAU_ERR("Failed to init compute context: %d\n", ret);
               // submit all initial state
                  fail:
      screen->base.base.context_create = NULL;
      }
      int
   nv50_screen_tic_alloc(struct nv50_screen *screen, void *entry)
   {
               while (screen->tic.lock[i / 32] & (1 << (i % 32)))
                     if (screen->tic.entries[i])
            screen->tic.entries[i] = entry;
      }
      int
   nv50_screen_tsc_alloc(struct nv50_screen *screen, void *entry)
   {
               while (screen->tsc.lock[i / 32] & (1 << (i % 32)))
                     if (screen->tsc.entries[i])
            screen->tsc.entries[i] = entry;
      }
