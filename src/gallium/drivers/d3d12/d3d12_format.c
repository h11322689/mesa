   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "d3d12_format.h"
      #include "util/format/u_formats.h"
   #include "pipe/p_video_enums.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/compiler.h"
      #define MAP_FORMAT_YUV(NAME) \
         #define MAP_FORMAT_NO_TYPELESS(BITS, TYPE) \
         #define MAP_FORMAT2_NO_TYPELESS(BITS1, TYPE1, BITS2, TYPE2) \
            #define MAP_FORMAT(BITS, TYPE) MAP_FORMAT_NO_TYPELESS(BITS, TYPE)
   #define MAP_FORMAT2(BITS1, TYPE1, BITS2, TYPE2) \
            #define MAP_FORMAT_CUSTOM_TYPELESS(BITS, TYPE, TYPELESS_BITS) \
         #define MAP_FORMAT2_CUSTOM_TYPELESS(BITS1, TYPE1, BITS2, TYPE2) \
            #define MAP_FORMAT_NORM(FMT) \
      MAP_FORMAT(FMT, UNORM) \
         #define MAP_FORMAT_INT(FMT) \
      MAP_FORMAT(FMT, UINT) \
         #define MAP_FORMAT_SRGB(FMT) \
            #define MAP_FORMAT_FLOAT(FMT) \
            #define MAP_EMU_FORMAT_NO_ALPHA(BITS, TYPE) \
      MAP_FORMAT2(L ## BITS, TYPE, R ## BITS, TYPE) \
   MAP_FORMAT2(I ## BITS, TYPE, R ## BITS, TYPE) \
         #define MAP_EMU_FORMAT(BITS, TYPE) \
      MAP_FORMAT2(A ## BITS, TYPE, R ## BITS, TYPE) \
         #define MAP_FORMAT_X8(BITS, TYPE) \
            #define FORMAT_TABLE() \
      MAP_FORMAT_NORM(R8) \
      \
      MAP_FORMAT_NORM(R8G8) \
      \
      MAP_FORMAT_NORM(R8G8B8A8) \
   MAP_FORMAT_INT(R8G8B8A8) \
      \
      /* We can rely on st/mesa to force the alpha to 1 for these, so we can \
   * just use RGBA. This is needed to support RGB configs, since some apps \
   * will only choose RGB (not RGBA) configs. \
   */ \
   MAP_FORMAT_X8(R8G8B8, UNORM) \
   MAP_FORMAT_X8(R8G8B8, SNORM) \
   MAP_FORMAT_X8(R8G8B8, UINT) \
      \
      MAP_FORMAT(B8G8R8X8, UNORM) \
      \
         \
      MAP_FORMAT_INT(R32) \
   MAP_FORMAT_FLOAT(R32) \
   MAP_FORMAT_INT(R32G32) \
   MAP_FORMAT_FLOAT(R32G32) \
   MAP_FORMAT_INT(R32G32B32) \
   MAP_FORMAT_FLOAT(R32G32B32) \
   MAP_FORMAT_INT(R32G32B32A32) \
      \
      MAP_FORMAT_NORM(R16) \
   MAP_FORMAT_INT(R16) \
      \
      MAP_FORMAT_NORM(R16G16) \
   MAP_FORMAT_INT(R16G16) \
      \
      MAP_FORMAT_NORM(R16G16B16A16) \
   MAP_FORMAT_INT(R16G16B16A16) \
      \
      MAP_FORMAT_NO_TYPELESS(A8, UNORM) \
   MAP_EMU_FORMAT_NO_ALPHA(8, UNORM) \
   MAP_EMU_FORMAT(8, SNORM) \
   MAP_EMU_FORMAT(8, SINT) \
   MAP_EMU_FORMAT(8, UINT) \
   MAP_EMU_FORMAT(16, UNORM) \
   MAP_EMU_FORMAT(16, SNORM) \
   MAP_EMU_FORMAT(16, SINT) \
   MAP_EMU_FORMAT(16, UINT) \
   MAP_EMU_FORMAT(16, FLOAT) \
   MAP_EMU_FORMAT(32, SINT) \
   MAP_EMU_FORMAT(32, UINT) \
      \
      MAP_FORMAT2_NO_TYPELESS(R9G9B9E5, FLOAT, R9G9B9E5, SHAREDEXP) \
   MAP_FORMAT_NO_TYPELESS(R11G11B10, FLOAT) \
   MAP_FORMAT(R10G10B10A2, UINT) \
      \
      MAP_FORMAT_NO_TYPELESS(B5G6R5, UNORM) \
   MAP_FORMAT_NO_TYPELESS(B5G5R5A1, UNORM) \
      \
         \
      MAP_FORMAT2(DXT1, RGB, BC1, UNORM) \
   MAP_FORMAT2(DXT1, RGBA, BC1, UNORM) \
   MAP_FORMAT2(DXT3, RGBA, BC2, UNORM) \
      \
      MAP_FORMAT2(DXT1, SRGB, BC1, UNORM_SRGB) \
   MAP_FORMAT2(DXT1, SRGBA, BC1, UNORM_SRGB) \
   MAP_FORMAT2(DXT3, SRGBA, BC2, UNORM_SRGB) \
      \
      MAP_FORMAT2(RGTC1, UNORM, BC4, UNORM) \
   MAP_FORMAT2(RGTC1, SNORM, BC4, SNORM) \
   MAP_FORMAT2(RGTC2, UNORM, BC5, UNORM) \
      \
      MAP_FORMAT2(BPTC, RGBA_UNORM, BC7, UNORM) \
   MAP_FORMAT2(BPTC, SRGBA, BC7, UNORM_SRGB) \
   MAP_FORMAT2(BPTC, RGB_FLOAT, BC6H, SF16) \
      \
      MAP_FORMAT2(Z32, FLOAT, R32, TYPELESS) \
   MAP_FORMAT2(Z16, UNORM, R16, TYPELESS) \
   MAP_FORMAT2(Z24X8, UNORM, R24G8, TYPELESS) \
      \
      MAP_FORMAT2(Z24_UNORM_S8, UINT, R24G8, TYPELESS) \
   MAP_FORMAT2(Z32_FLOAT_S8X24, UINT, R32G8X24, TYPELESS) \
      \
      MAP_FORMAT_YUV(NV12) \
         static const DXGI_FORMAT formats[PIPE_FORMAT_COUNT] = {
         };
      #undef MAP_FORMAT
   #undef MAP_FORMAT2
   #undef MAP_FORMAT_CUSTOM_TYPELESS
   #undef MAP_FORMAT2_CUSTOM_TYPELESS
      #define MAP_FORMAT(BITS, TYPE) \
            #define MAP_FORMAT2(BITS1, TYPE1, BITS2, TYPE2) \
            #define MAP_FORMAT_CUSTOM_TYPELESS(BITS1, TYPE, BITS2) \
            #define MAP_FORMAT2_CUSTOM_TYPELESS(BITS1, TYPE1, BITS2, TYPE2) \
            static const DXGI_FORMAT typeless_formats[PIPE_FORMAT_COUNT] = {
         };
      DXGI_FORMAT
   d3d12_get_format(enum pipe_format format)
   {
         }
      DXGI_FORMAT
   d3d12_get_typeless_format(enum pipe_format format)
   {
         }
      enum pipe_format
   d3d12_get_pipe_format(DXGI_FORMAT format)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(formats); ++i) {
      if (formats[i] == format) {
            }
      }
      enum pipe_format
   d3d12_get_default_pipe_format(DXGI_FORMAT format)
   {
   #define TYPELESS_TO(channels, suffix) \
      case DXGI_FORMAT_##channels##_TYPELESS: \
            switch (format) {
      TYPELESS_TO(R8, UNORM);
   TYPELESS_TO(R8G8, UNORM);
   TYPELESS_TO(R8G8B8A8, UNORM);
   TYPELESS_TO(B8G8R8X8, UNORM);
   TYPELESS_TO(B8G8R8A8, UNORM);
   TYPELESS_TO(R16, FLOAT);
   TYPELESS_TO(R16G16, FLOAT);
   TYPELESS_TO(R16G16B16A16, FLOAT);
   TYPELESS_TO(R32, FLOAT);
   TYPELESS_TO(R32G32, FLOAT);
   TYPELESS_TO(R32G32B32, FLOAT);
      case DXGI_FORMAT_BC1_TYPELESS:
         case DXGI_FORMAT_BC2_TYPELESS:
         case DXGI_FORMAT_BC3_TYPELESS:
         case DXGI_FORMAT_BC4_TYPELESS:
         case DXGI_FORMAT_BC5_TYPELESS:
         case DXGI_FORMAT_BC6H_TYPELESS:
         case DXGI_FORMAT_BC7_TYPELESS:
         default:
            }
      DXGI_FORMAT
   d3d12_get_resource_rt_format(enum pipe_format f)
   {
      switch (f) {
   case PIPE_FORMAT_Z16_UNORM:
         case PIPE_FORMAT_Z32_FLOAT:
         case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_X24S8_UINT:
         case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
   case PIPE_FORMAT_X32_S8X24_UINT:
         case PIPE_FORMAT_Z24_UNORM_S8_UINT:
         default:
            }
      DXGI_FORMAT
   d3d12_get_resource_srv_format(enum pipe_format f, enum pipe_texture_target target)
   {
      switch (f) {
   case PIPE_FORMAT_Z16_UNORM:
         case PIPE_FORMAT_Z32_FLOAT:
         case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
         case PIPE_FORMAT_X24S8_UINT:
         case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
         case PIPE_FORMAT_X32_S8X24_UINT:
         case PIPE_FORMAT_A8_UNORM:
      if (target == PIPE_BUFFER)
            default:
            }
      #define DEF_SWIZZLE(name, X, Y, Z, W) \
      static const enum pipe_swizzle name ## _SWIZZLE[PIPE_SWIZZLE_MAX] = \
      { PIPE_SWIZZLE_ ## X, PIPE_SWIZZLE_ ## Y, PIPE_SWIZZLE_ ## Z, PIPE_SWIZZLE_ ## W, \
      struct d3d12_format_info
   d3d12_get_format_info(enum pipe_format resource_format, enum pipe_format pformat, enum pipe_texture_target target)
   {
      DEF_SWIZZLE(IDENTITY, X, Y, Z, W);
   DEF_SWIZZLE(RGB1, X, Y, Z, 1);
   DEF_SWIZZLE(ALPHA, 0, 0, 0, W);
   DEF_SWIZZLE(BUFFER, 0, 0, 0, X);
   DEF_SWIZZLE(INTENSITY, X, X, X, X);
   DEF_SWIZZLE(LUMINANCE, X, X, X, 1);
   DEF_SWIZZLE(LUMINANCE_ALPHA, X, X, X, Y);
   DEF_SWIZZLE(DEPTH, X, X, X, X);
            const enum pipe_swizzle *swizzle = IDENTITY_SWIZZLE;
            if (pformat == PIPE_FORMAT_DXT1_RGB ||
      pformat == PIPE_FORMAT_DXT1_SRGB)
         const struct util_format_description
         unsigned plane_count = util_format_get_num_planes(resource_format);
   if (!util_format_is_srgb(pformat)) {
      if (target == PIPE_BUFFER && util_format_is_alpha(pformat)) {
         } else if (plane_count > 1) {
      for (plane_slice = 0; plane_slice < plane_count; ++plane_slice) {
      if (util_format_get_plane_format(resource_format, plane_slice) == pformat)
      }
      } else if (pformat == PIPE_FORMAT_A8_UNORM) {
         } else if (util_format_is_intensity(pformat)) {
         } else if (util_format_is_luminance(pformat)) {
         } else if (util_format_is_luminance_alpha(pformat)) {
         } else if (util_format_is_alpha(pformat)) {
         } else if (util_format_has_depth(format_desc)) {
         } else if (util_format_has_stencil(format_desc)) {
      /* When reading from a stencil texture we have to use plane 1, and
   * the formats X24S8 and X32_S8X24 have the actual data in the y-channel
   * but the shader will read the x component so we need to adjust the swizzle. */
   plane_slice = 1;
      } else if (util_format_has_alpha1(pformat)) {
                        }
      enum pipe_format
   d3d12_emulated_vtx_format(enum pipe_format fmt)
   {
      switch (fmt) {
   case PIPE_FORMAT_R10G10B10A2_SNORM:
   case PIPE_FORMAT_R10G10B10A2_SSCALED:
   case PIPE_FORMAT_R10G10B10A2_USCALED:
   case PIPE_FORMAT_B10G10R10A2_UNORM:
   case PIPE_FORMAT_B10G10R10A2_SNORM:
   case PIPE_FORMAT_B10G10R10A2_SSCALED:
   case PIPE_FORMAT_B10G10R10A2_USCALED:
            case PIPE_FORMAT_R8G8B8_SINT:
         case PIPE_FORMAT_R8G8B8_UINT:
            case PIPE_FORMAT_R16G16B16_SINT:
         case PIPE_FORMAT_R16G16B16_UINT:
            case PIPE_FORMAT_R8G8B8A8_SSCALED:
         case PIPE_FORMAT_R8G8B8A8_USCALED:
         case PIPE_FORMAT_R16G16B16A16_SSCALED:
         case PIPE_FORMAT_R16G16B16A16_USCALED:
            default:
            }
         unsigned
   d3d12_non_opaque_plane_count(DXGI_FORMAT format)
   {
      switch (format) {
   case DXGI_FORMAT_V208:
   case DXGI_FORMAT_V408:
            case DXGI_FORMAT_NV12:
   case DXGI_FORMAT_P010:
   case DXGI_FORMAT_P016:
   case DXGI_FORMAT_YUY2:
   case DXGI_FORMAT_Y210:
   case DXGI_FORMAT_Y216:
   case DXGI_FORMAT_NV11:
            case DXGI_FORMAT_R24G8_TYPELESS:
   case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
   case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
   case DXGI_FORMAT_D24_UNORM_S8_UINT:
   case DXGI_FORMAT_R32G8X24_TYPELESS:
   case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
   case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
   case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            default:
            }
      unsigned
   d3d12_get_format_start_plane(enum pipe_format fmt)
   {
      const struct util_format_description *desc = util_format_description(fmt);
   if (util_format_has_stencil(desc) && !util_format_has_depth(desc))
               }
      unsigned
   d3d12_get_format_num_planes(enum pipe_format fmt)
   {
      return util_format_is_depth_or_stencil(fmt) ?
      }
      DXGI_FORMAT
   d3d12_convert_pipe_video_profile_to_dxgi_format(enum pipe_video_profile profile)
   {
      switch (profile) {
      case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_CONSTRAINED_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_EXTENDED:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
   case PIPE_VIDEO_PROFILE_HEVC_MAIN:
   case PIPE_VIDEO_PROFILE_AV1_MAIN:
   case PIPE_VIDEO_PROFILE_VP9_PROFILE0:   
         case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH10:
   case PIPE_VIDEO_PROFILE_HEVC_MAIN_10:
   case PIPE_VIDEO_PROFILE_VP9_PROFILE2:
         default:
   {
               }
      DXGI_COLOR_SPACE_TYPE
   d3d12_convert_from_legacy_color_space(bool rgb, uint32_t bits_per_element, bool studio_rgb, bool p709, bool studio_yuv)
   {
      if (rgb) {
      if (bits_per_element > 32) {
      // All 16 bit color channel data is assumed to be linear rather than SRGB
      } else {
      if (studio_rgb) {
         } else {
               } else {
      if (p709) {
      if (studio_yuv) {
         } else {
            } else {
      if (studio_yuv) {
         } else {
                  }
