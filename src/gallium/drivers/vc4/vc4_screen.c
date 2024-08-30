   /*
   * Copyright © 2014 Broadcom
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
      #include <xf86drm.h>
   #include "drm-uapi/drm_fourcc.h"
   #include "drm-uapi/vc4_drm.h"
   #include "vc4_screen.h"
   #include "vc4_context.h"
   #include "vc4_resource.h"
      static const struct debug_named_value vc4_debug_options[] = {
         { "cl",       VC4_DEBUG_CL,
         { "surf",       VC4_DEBUG_SURFACE,
         { "qpu",      VC4_DEBUG_QPU,
         { "qir",      VC4_DEBUG_QIR,
         { "nir",      VC4_DEBUG_NIR,
         { "tgsi",     VC4_DEBUG_TGSI,
         { "shaderdb", VC4_DEBUG_SHADERDB,
         { "perf",     VC4_DEBUG_PERF,
         { "norast",   VC4_DEBUG_NORAST,
         { "always_flush", VC4_DEBUG_ALWAYS_FLUSH,
         { "always_sync", VC4_DEBUG_ALWAYS_SYNC,
   #ifdef USE_VC4_SIMULATOR
         { "dump", VC4_DEBUG_DUMP,
   #endif
         };
      DEBUG_GET_ONCE_FLAGS_OPTION(vc4_debug, "VC4_DEBUG", vc4_debug_options, 0)
   uint32_t vc4_mesa_debug;
      static const char *
   vc4_screen_get_name(struct pipe_screen *pscreen)
   {
                  if (!screen->name) {
            screen->name = ralloc_asprintf(screen,
                     }
      static const char *
   vc4_screen_get_vendor(struct pipe_screen *pscreen)
   {
         }
      static void
   vc4_screen_destroy(struct pipe_screen *pscreen)
   {
                  _mesa_hash_table_destroy(screen->bo_handles, NULL);
   vc4_bufmgr_destroy(pscreen);
   slab_destroy_parent(&screen->transfer_pool);
   if (screen->ro)
      #ifdef USE_VC4_SIMULATOR
         #endif
                     close(screen->fd);
   }
      static bool
   vc4_has_feature(struct vc4_screen *screen, uint32_t feature)
   {
         struct drm_vc4_get_param p = {
         };
            if (ret != 0)
            }
      static int
   vc4_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
   {
                  switch (param) {
         case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_TEXTURE_MULTISAMPLE:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_TEXTURE_BARRIER:
   case PIPE_CAP_TGSI_TEXCOORD:
            case PIPE_CAP_NATIVE_FENCE_FD:
            case PIPE_CAP_TILE_RASTER_ORDER:
                  case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL:
            case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
                  case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
         case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
         case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
            case PIPE_CAP_MAX_VARYINGS:
            case PIPE_CAP_VENDOR_ID:
         case PIPE_CAP_ACCELERATED:
         case PIPE_CAP_VIDEO_MEMORY: {
                                 }
   case PIPE_CAP_UMA:
            case PIPE_CAP_ALPHA_TEST:
   case PIPE_CAP_VERTEX_COLOR_CLAMPED:
   case PIPE_CAP_TWO_SIDED_COLOR:
   case PIPE_CAP_TEXRECT:
   case PIPE_CAP_IMAGE_STORE_FORMATTED:
            case PIPE_CAP_SUPPORTED_PRIM_MODES:
            default:
         }
      static float
   vc4_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
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
   vc4_screen_get_shader_param(struct pipe_screen *pscreen,
               {
         if (shader != PIPE_SHADER_VERTEX &&
         shader != PIPE_SHADER_FRAGMENT) {
            /* this is probably not totally correct.. but it's a start: */
   switch (param) {
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
         case PIPE_SHADER_CAP_CONT_SUPPORTED:
         case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
         case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
         case PIPE_SHADER_CAP_SUBROUTINES:
         case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
         case PIPE_SHADER_CAP_INTEGERS:
         case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
   case PIPE_SHADER_CAP_INT16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
   case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
         case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
   case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
         case PIPE_SHADER_CAP_SUPPORTED_IRS:
         case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
   case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
         default:
               }
   }
      static bool
   vc4_screen_is_format_supported(struct pipe_screen *pscreen,
                                 {
                  if (MAX2(1, sample_count) != MAX2(1, storage_sample_count))
            if (sample_count > 1 && sample_count != VC4_MAX_SAMPLES)
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
   case PIPE_FORMAT_R16G16B16_UNORM:
   case PIPE_FORMAT_R16G16_UNORM:
   case PIPE_FORMAT_R16_UNORM:
   case PIPE_FORMAT_R16G16B16A16_SNORM:
   case PIPE_FORMAT_R16G16B16_SNORM:
   case PIPE_FORMAT_R16G16_SNORM:
   case PIPE_FORMAT_R16_SNORM:
   case PIPE_FORMAT_R16G16B16A16_USCALED:
   case PIPE_FORMAT_R16G16B16_USCALED:
   case PIPE_FORMAT_R16G16_USCALED:
   case PIPE_FORMAT_R16_USCALED:
   case PIPE_FORMAT_R16G16B16A16_SSCALED:
   case PIPE_FORMAT_R16G16B16_SSCALED:
   case PIPE_FORMAT_R16G16_SSCALED:
   case PIPE_FORMAT_R16_SSCALED:
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
         default:
               if ((usage & PIPE_BIND_RENDER_TARGET) &&
         !vc4_rt_format_supported(format)) {
            if ((usage & PIPE_BIND_SAMPLER_VIEW) &&
         (!vc4_tex_format_supported(format) ||
   (format == PIPE_FORMAT_ETC1_RGB8 && !screen->has_etc1))) {
            if ((usage & PIPE_BIND_DEPTH_STENCIL) &&
         format != PIPE_FORMAT_S8_UINT_Z24_UNORM &&
   format != PIPE_FORMAT_X8Z24_UNORM) {
            if ((usage & PIPE_BIND_INDEX_BUFFER) &&
         format != PIPE_FORMAT_R8_UINT &&
   format != PIPE_FORMAT_R16_UINT) {
            }
      static const uint64_t *vc4_get_modifiers(struct pipe_screen *pscreen, int *num)
   {
         struct vc4_screen *screen = vc4_screen(pscreen);
   static const uint64_t all_modifiers[] = {
               };
            /* We support both modifiers (tiled and linear) for all sampler
      * formats, but if we don't have the DRM_VC4_GET_TILING ioctl
   * we shouldn't advertise the tiled formats.
      if (screen->has_tiling_ioctl) {
               } else{
                        }
      static void
   vc4_screen_query_dmabuf_modifiers(struct pipe_screen *pscreen,
                           {
         const uint64_t *available_modifiers;
   int i;
   bool tex_will_lower;
                     if (!modifiers) {
                        *count = MIN2(max, num_modifiers);
   tex_will_lower = !vc4_tex_format_supported(format);
   for (i = 0; i < *count; i++) {
            modifiers[i] = available_modifiers[i];
      }
      static bool
   vc4_screen_is_dmabuf_modifier_supported(struct pipe_screen *pscreen,
                     {
         const uint64_t *available_modifiers;
                     for (i = 0; i < num_modifiers; i++) {
                                             }
      static bool
   vc4_get_chip_info(struct vc4_screen *screen)
   {
         struct drm_vc4_get_param ident0 = {
         };
   struct drm_vc4_get_param ident1 = {
         };
            ret = vc4_ioctl(screen->fd, DRM_IOCTL_VC4_GET_PARAM, &ident0);
   if (ret != 0) {
            if (errno == EINVAL) {
            /* Backwards compatibility with 2835 kernels which
   * only do V3D 2.1.
   */
      } else {
            fprintf(stderr, "Couldn't get V3D IDENT0: %s\n",
   }
   ret = vc4_ioctl(screen->fd, DRM_IOCTL_VC4_GET_PARAM, &ident1);
   if (ret != 0) {
            fprintf(stderr, "Couldn't get V3D IDENT1: %s\n",
               uint32_t major = (ident0.value >> 24) & 0xff;
   uint32_t minor = (ident1.value >> 0) & 0xf;
            if (screen->v3d_ver != 21 && screen->v3d_ver != 26) {
            fprintf(stderr,
            "V3D %d.%d not supported by this version of Mesa.\n",
            }
      static int
   vc4_screen_get_fd(struct pipe_screen *pscreen)
   {
                  }
      struct pipe_screen *
   vc4_screen_create(int fd, const struct pipe_screen_config *config,
         {
         struct vc4_screen *screen = rzalloc(NULL, struct vc4_screen);
   uint64_t syncobj_cap = 0;
   struct pipe_screen *pscreen;
                     pscreen->destroy = vc4_screen_destroy;
   pscreen->get_screen_fd = vc4_screen_get_fd;
   pscreen->get_param = vc4_screen_get_param;
   pscreen->get_paramf = vc4_screen_get_paramf;
   pscreen->get_shader_param = vc4_screen_get_shader_param;
   pscreen->context_create = vc4_context_create;
            screen->fd = fd;
            list_inithead(&screen->bo_cache.time_list);
   (void) mtx_init(&screen->bo_handles_mutex, mtx_plain);
            screen->has_control_flow =
         screen->has_etc1 =
         screen->has_threaded_fs =
         screen->has_madvise =
         screen->has_perfmon_ioctl =
            err = drmGetCap(fd, DRM_CAP_SYNCOBJ, &syncobj_cap);
   if (err == 0 && syncobj_cap)
            if (!vc4_get_chip_info(screen))
                                 #ifdef USE_VC4_SIMULATOR
         #endif
                     pscreen->get_name = vc4_screen_get_name;
   pscreen->get_vendor = vc4_screen_get_vendor;
   pscreen->get_device_vendor = vc4_screen_get_vendor;
   pscreen->get_compiler_options = vc4_screen_get_compiler_options;
   pscreen->query_dmabuf_modifiers = vc4_screen_query_dmabuf_modifiers;
            if (screen->has_perfmon_ioctl) {
                        /* Generate the bitmask of supported draw primitives. */
   screen->prim_types = BITFIELD_BIT(MESA_PRIM_POINTS) |
                        BITFIELD_BIT(MESA_PRIM_LINES) |
                     fail:
         close(fd);
   ralloc_free(pscreen);
   }
