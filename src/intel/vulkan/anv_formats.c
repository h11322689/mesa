   /*
   * Copyright © 2015 Intel Corporation
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
      #include "anv_private.h"
   #include "drm-uapi/drm_fourcc.h"
   #include "vk_android.h"
   #include "vk_enum_defines.h"
   #include "vk_enum_to_str.h"
   #include "vk_format.h"
   #include "vk_util.h"
      /*
   * gcc-4 and earlier don't allow compound literals where a constant
   * is required in -std=c99/gnu99 mode, so we can't use ISL_SWIZZLE()
   * here. -std=c89/gnu89 would allow it, but we depend on c99 features
   * so using -std=c89/gnu89 is not an option. Starting from gcc-5
   * compound literals can also be considered constant in -std=c99/gnu99
   * mode.
   */
   #define _ISL_SWIZZLE(r, g, b, a) { \
         ISL_CHANNEL_SELECT_##r, \
   ISL_CHANNEL_SELECT_##g, \
   ISL_CHANNEL_SELECT_##b, \
   }
      #define RGBA _ISL_SWIZZLE(RED, GREEN, BLUE, ALPHA)
   #define BGRA _ISL_SWIZZLE(BLUE, GREEN, RED, ALPHA)
   #define RGB1 _ISL_SWIZZLE(RED, GREEN, BLUE, ONE)
      #define swiz_fmt1(__vk_fmt, __hw_fmt, __swizzle) \
      [VK_ENUM_OFFSET(__vk_fmt)] = { \
      .planes = { \
      { .isl_format = __hw_fmt, .swizzle = __swizzle, \
   .aspect = VK_IMAGE_ASPECT_COLOR_BIT, \
      }, \
   .vk_format = __vk_fmt, \
            #define fmt1(__vk_fmt, __hw_fmt) \
            #define d_fmt(__vk_fmt, __hw_fmt) \
      [VK_ENUM_OFFSET(__vk_fmt)] = { \
      .planes = { \
      { .isl_format = __hw_fmt, .swizzle = RGBA, \
   .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, \
      }, \
   .vk_format = __vk_fmt, \
            #define s_fmt(__vk_fmt, __hw_fmt) \
      [VK_ENUM_OFFSET(__vk_fmt)] = { \
      .planes = { \
      { .isl_format = __hw_fmt, .swizzle = RGBA, \
   .aspect = VK_IMAGE_ASPECT_STENCIL_BIT, \
      }, \
   .vk_format = __vk_fmt, \
            #define ds_fmt2(__vk_fmt, __fmt1, __fmt2) \
      [VK_ENUM_OFFSET(__vk_fmt)] = { \
      .planes = { \
      { .isl_format = __fmt1, .swizzle = RGBA, \
   .aspect = VK_IMAGE_ASPECT_DEPTH_BIT, \
   }, \
   { .isl_format = __fmt2, .swizzle = RGBA, \
   .aspect = VK_IMAGE_ASPECT_STENCIL_BIT, \
      }, \
   .vk_format = __vk_fmt, \
            #define fmt_unsupported(__vk_fmt) \
      [VK_ENUM_OFFSET(__vk_fmt)] = { \
      .planes = { \
         }, \
            #define ycbcr_plane(__plane, __hw_fmt, __swizzle) \
      { .isl_format = __hw_fmt, \
   .swizzle = __swizzle, \
   .aspect = VK_IMAGE_ASPECT_PLANE_ ## __plane ## _BIT, \
         #define ycbcr_fmt(__vk_fmt, __n_planes, __can_ycbcr, __can_video, ...) \
      [VK_ENUM_OFFSET(__vk_fmt)] = { \
      .planes = { \
         }, \
   .vk_format = __vk_fmt, \
   .n_planes = __n_planes, \
   .can_ycbcr = __can_ycbcr, \
            /* HINT: For array formats, the ISL name should match the VK name.  For
   * packed formats, they should have the channels in reverse order from each
   * other.  The reason for this is that, for packed formats, the ISL (and
   * bspec) names are in LSB -> MSB order while VK formats are MSB -> LSB.
   */
   static const struct anv_format main_formats[] = {
      fmt_unsupported(VK_FORMAT_UNDEFINED),
   fmt_unsupported(VK_FORMAT_R4G4_UNORM_PACK8),
   fmt1(VK_FORMAT_R4G4B4A4_UNORM_PACK16,             ISL_FORMAT_A4B4G4R4_UNORM),
   swiz_fmt1(VK_FORMAT_B4G4R4A4_UNORM_PACK16,        ISL_FORMAT_A4B4G4R4_UNORM,  BGRA),
   fmt1(VK_FORMAT_R5G6B5_UNORM_PACK16,               ISL_FORMAT_B5G6R5_UNORM),
   swiz_fmt1(VK_FORMAT_B5G6R5_UNORM_PACK16,          ISL_FORMAT_B5G6R5_UNORM, BGRA),
   fmt1(VK_FORMAT_R5G5B5A1_UNORM_PACK16,             ISL_FORMAT_A1B5G5R5_UNORM),
   swiz_fmt1(VK_FORMAT_B5G5R5A1_UNORM_PACK16,        ISL_FORMAT_A1B5G5R5_UNORM, BGRA),
   fmt1(VK_FORMAT_A1R5G5B5_UNORM_PACK16,             ISL_FORMAT_B5G5R5A1_UNORM),
   fmt1(VK_FORMAT_R8_UNORM,                          ISL_FORMAT_R8_UNORM),
   fmt1(VK_FORMAT_R8_SNORM,                          ISL_FORMAT_R8_SNORM),
   fmt1(VK_FORMAT_R8_USCALED,                        ISL_FORMAT_R8_USCALED),
   fmt1(VK_FORMAT_R8_SSCALED,                        ISL_FORMAT_R8_SSCALED),
   fmt1(VK_FORMAT_R8_UINT,                           ISL_FORMAT_R8_UINT),
   fmt1(VK_FORMAT_R8_SINT,                           ISL_FORMAT_R8_SINT),
   swiz_fmt1(VK_FORMAT_R8_SRGB,                      ISL_FORMAT_L8_UNORM_SRGB,
         fmt1(VK_FORMAT_R8G8_UNORM,                        ISL_FORMAT_R8G8_UNORM),
   fmt1(VK_FORMAT_R8G8_SNORM,                        ISL_FORMAT_R8G8_SNORM),
   fmt1(VK_FORMAT_R8G8_USCALED,                      ISL_FORMAT_R8G8_USCALED),
   fmt1(VK_FORMAT_R8G8_SSCALED,                      ISL_FORMAT_R8G8_SSCALED),
   fmt1(VK_FORMAT_R8G8_UINT,                         ISL_FORMAT_R8G8_UINT),
   fmt1(VK_FORMAT_R8G8_SINT,                         ISL_FORMAT_R8G8_SINT),
   fmt_unsupported(VK_FORMAT_R8G8_SRGB),             /* L8A8_UNORM_SRGB */
   fmt1(VK_FORMAT_R8G8B8_UNORM,                      ISL_FORMAT_R8G8B8_UNORM),
   fmt1(VK_FORMAT_R8G8B8_SNORM,                      ISL_FORMAT_R8G8B8_SNORM),
   fmt1(VK_FORMAT_R8G8B8_USCALED,                    ISL_FORMAT_R8G8B8_USCALED),
   fmt1(VK_FORMAT_R8G8B8_SSCALED,                    ISL_FORMAT_R8G8B8_SSCALED),
   fmt1(VK_FORMAT_R8G8B8_UINT,                       ISL_FORMAT_R8G8B8_UINT),
   fmt1(VK_FORMAT_R8G8B8_SINT,                       ISL_FORMAT_R8G8B8_SINT),
   fmt1(VK_FORMAT_R8G8B8_SRGB,                       ISL_FORMAT_R8G8B8_UNORM_SRGB),
   fmt1(VK_FORMAT_R8G8B8A8_UNORM,                    ISL_FORMAT_R8G8B8A8_UNORM),
   fmt1(VK_FORMAT_R8G8B8A8_SNORM,                    ISL_FORMAT_R8G8B8A8_SNORM),
   fmt1(VK_FORMAT_R8G8B8A8_USCALED,                  ISL_FORMAT_R8G8B8A8_USCALED),
   fmt1(VK_FORMAT_R8G8B8A8_SSCALED,                  ISL_FORMAT_R8G8B8A8_SSCALED),
   fmt1(VK_FORMAT_R8G8B8A8_UINT,                     ISL_FORMAT_R8G8B8A8_UINT),
   fmt1(VK_FORMAT_R8G8B8A8_SINT,                     ISL_FORMAT_R8G8B8A8_SINT),
   fmt1(VK_FORMAT_R8G8B8A8_SRGB,                     ISL_FORMAT_R8G8B8A8_UNORM_SRGB),
   fmt1(VK_FORMAT_A8B8G8R8_UNORM_PACK32,             ISL_FORMAT_R8G8B8A8_UNORM),
   fmt1(VK_FORMAT_A8B8G8R8_SNORM_PACK32,             ISL_FORMAT_R8G8B8A8_SNORM),
   fmt1(VK_FORMAT_A8B8G8R8_USCALED_PACK32,           ISL_FORMAT_R8G8B8A8_USCALED),
   fmt1(VK_FORMAT_A8B8G8R8_SSCALED_PACK32,           ISL_FORMAT_R8G8B8A8_SSCALED),
   fmt1(VK_FORMAT_A8B8G8R8_UINT_PACK32,              ISL_FORMAT_R8G8B8A8_UINT),
   fmt1(VK_FORMAT_A8B8G8R8_SINT_PACK32,              ISL_FORMAT_R8G8B8A8_SINT),
   fmt1(VK_FORMAT_A8B8G8R8_SRGB_PACK32,              ISL_FORMAT_R8G8B8A8_UNORM_SRGB),
   fmt1(VK_FORMAT_A2R10G10B10_UNORM_PACK32,          ISL_FORMAT_B10G10R10A2_UNORM),
   fmt1(VK_FORMAT_A2R10G10B10_SNORM_PACK32,          ISL_FORMAT_B10G10R10A2_SNORM),
   fmt1(VK_FORMAT_A2R10G10B10_USCALED_PACK32,        ISL_FORMAT_B10G10R10A2_USCALED),
   fmt1(VK_FORMAT_A2R10G10B10_SSCALED_PACK32,        ISL_FORMAT_B10G10R10A2_SSCALED),
   fmt1(VK_FORMAT_A2R10G10B10_UINT_PACK32,           ISL_FORMAT_B10G10R10A2_UINT),
   fmt1(VK_FORMAT_A2R10G10B10_SINT_PACK32,           ISL_FORMAT_B10G10R10A2_SINT),
   fmt1(VK_FORMAT_A2B10G10R10_UNORM_PACK32,          ISL_FORMAT_R10G10B10A2_UNORM),
   fmt1(VK_FORMAT_A2B10G10R10_SNORM_PACK32,          ISL_FORMAT_R10G10B10A2_SNORM),
   fmt1(VK_FORMAT_A2B10G10R10_USCALED_PACK32,        ISL_FORMAT_R10G10B10A2_USCALED),
   fmt1(VK_FORMAT_A2B10G10R10_SSCALED_PACK32,        ISL_FORMAT_R10G10B10A2_SSCALED),
   fmt1(VK_FORMAT_A2B10G10R10_UINT_PACK32,           ISL_FORMAT_R10G10B10A2_UINT),
   fmt1(VK_FORMAT_A2B10G10R10_SINT_PACK32,           ISL_FORMAT_R10G10B10A2_SINT),
   fmt1(VK_FORMAT_R16_UNORM,                         ISL_FORMAT_R16_UNORM),
   fmt1(VK_FORMAT_R16_SNORM,                         ISL_FORMAT_R16_SNORM),
   fmt1(VK_FORMAT_R16_USCALED,                       ISL_FORMAT_R16_USCALED),
   fmt1(VK_FORMAT_R16_SSCALED,                       ISL_FORMAT_R16_SSCALED),
   fmt1(VK_FORMAT_R16_UINT,                          ISL_FORMAT_R16_UINT),
   fmt1(VK_FORMAT_R16_SINT,                          ISL_FORMAT_R16_SINT),
   fmt1(VK_FORMAT_R16_SFLOAT,                        ISL_FORMAT_R16_FLOAT),
   fmt1(VK_FORMAT_R16G16_UNORM,                      ISL_FORMAT_R16G16_UNORM),
   fmt1(VK_FORMAT_R16G16_SNORM,                      ISL_FORMAT_R16G16_SNORM),
   fmt1(VK_FORMAT_R16G16_USCALED,                    ISL_FORMAT_R16G16_USCALED),
   fmt1(VK_FORMAT_R16G16_SSCALED,                    ISL_FORMAT_R16G16_SSCALED),
   fmt1(VK_FORMAT_R16G16_UINT,                       ISL_FORMAT_R16G16_UINT),
   fmt1(VK_FORMAT_R16G16_SINT,                       ISL_FORMAT_R16G16_SINT),
   fmt1(VK_FORMAT_R16G16_SFLOAT,                     ISL_FORMAT_R16G16_FLOAT),
   fmt1(VK_FORMAT_R16G16B16_UNORM,                   ISL_FORMAT_R16G16B16_UNORM),
   fmt1(VK_FORMAT_R16G16B16_SNORM,                   ISL_FORMAT_R16G16B16_SNORM),
   fmt1(VK_FORMAT_R16G16B16_USCALED,                 ISL_FORMAT_R16G16B16_USCALED),
   fmt1(VK_FORMAT_R16G16B16_SSCALED,                 ISL_FORMAT_R16G16B16_SSCALED),
   fmt1(VK_FORMAT_R16G16B16_UINT,                    ISL_FORMAT_R16G16B16_UINT),
   fmt1(VK_FORMAT_R16G16B16_SINT,                    ISL_FORMAT_R16G16B16_SINT),
   fmt1(VK_FORMAT_R16G16B16_SFLOAT,                  ISL_FORMAT_R16G16B16_FLOAT),
   fmt1(VK_FORMAT_R16G16B16A16_UNORM,                ISL_FORMAT_R16G16B16A16_UNORM),
   fmt1(VK_FORMAT_R16G16B16A16_SNORM,                ISL_FORMAT_R16G16B16A16_SNORM),
   fmt1(VK_FORMAT_R16G16B16A16_USCALED,              ISL_FORMAT_R16G16B16A16_USCALED),
   fmt1(VK_FORMAT_R16G16B16A16_SSCALED,              ISL_FORMAT_R16G16B16A16_SSCALED),
   fmt1(VK_FORMAT_R16G16B16A16_UINT,                 ISL_FORMAT_R16G16B16A16_UINT),
   fmt1(VK_FORMAT_R16G16B16A16_SINT,                 ISL_FORMAT_R16G16B16A16_SINT),
   fmt1(VK_FORMAT_R16G16B16A16_SFLOAT,               ISL_FORMAT_R16G16B16A16_FLOAT),
   fmt1(VK_FORMAT_R32_UINT,                          ISL_FORMAT_R32_UINT),
   fmt1(VK_FORMAT_R32_SINT,                          ISL_FORMAT_R32_SINT),
   fmt1(VK_FORMAT_R32_SFLOAT,                        ISL_FORMAT_R32_FLOAT),
   fmt1(VK_FORMAT_R32G32_UINT,                       ISL_FORMAT_R32G32_UINT),
   fmt1(VK_FORMAT_R32G32_SINT,                       ISL_FORMAT_R32G32_SINT),
   fmt1(VK_FORMAT_R32G32_SFLOAT,                     ISL_FORMAT_R32G32_FLOAT),
   fmt1(VK_FORMAT_R32G32B32_UINT,                    ISL_FORMAT_R32G32B32_UINT),
   fmt1(VK_FORMAT_R32G32B32_SINT,                    ISL_FORMAT_R32G32B32_SINT),
   fmt1(VK_FORMAT_R32G32B32_SFLOAT,                  ISL_FORMAT_R32G32B32_FLOAT),
   fmt1(VK_FORMAT_R32G32B32A32_UINT,                 ISL_FORMAT_R32G32B32A32_UINT),
   fmt1(VK_FORMAT_R32G32B32A32_SINT,                 ISL_FORMAT_R32G32B32A32_SINT),
   fmt1(VK_FORMAT_R32G32B32A32_SFLOAT,               ISL_FORMAT_R32G32B32A32_FLOAT),
   fmt1(VK_FORMAT_R64_UINT,                          ISL_FORMAT_R64_PASSTHRU),
   fmt1(VK_FORMAT_R64_SINT,                          ISL_FORMAT_R64_PASSTHRU),
   fmt1(VK_FORMAT_R64_SFLOAT,                        ISL_FORMAT_R64_PASSTHRU),
   fmt1(VK_FORMAT_R64G64_UINT,                       ISL_FORMAT_R64G64_PASSTHRU),
   fmt1(VK_FORMAT_R64G64_SINT,                       ISL_FORMAT_R64G64_PASSTHRU),
   fmt1(VK_FORMAT_R64G64_SFLOAT,                     ISL_FORMAT_R64G64_PASSTHRU),
   fmt1(VK_FORMAT_R64G64B64_UINT,                    ISL_FORMAT_R64G64B64_PASSTHRU),
   fmt1(VK_FORMAT_R64G64B64_SINT,                    ISL_FORMAT_R64G64B64_PASSTHRU),
   fmt1(VK_FORMAT_R64G64B64_SFLOAT,                  ISL_FORMAT_R64G64B64_PASSTHRU),
   fmt1(VK_FORMAT_R64G64B64A64_UINT,                 ISL_FORMAT_R64G64B64A64_PASSTHRU),
   fmt1(VK_FORMAT_R64G64B64A64_SINT,                 ISL_FORMAT_R64G64B64A64_PASSTHRU),
   fmt1(VK_FORMAT_R64G64B64A64_SFLOAT,               ISL_FORMAT_R64G64B64A64_PASSTHRU),
   fmt1(VK_FORMAT_B10G11R11_UFLOAT_PACK32,           ISL_FORMAT_R11G11B10_FLOAT),
            d_fmt(VK_FORMAT_D16_UNORM,                        ISL_FORMAT_R16_UNORM),
   d_fmt(VK_FORMAT_X8_D24_UNORM_PACK32,              ISL_FORMAT_R24_UNORM_X8_TYPELESS),
   d_fmt(VK_FORMAT_D32_SFLOAT,                       ISL_FORMAT_R32_FLOAT),
   s_fmt(VK_FORMAT_S8_UINT,                          ISL_FORMAT_R8_UINT),
   fmt_unsupported(VK_FORMAT_D16_UNORM_S8_UINT),
   ds_fmt2(VK_FORMAT_D24_UNORM_S8_UINT,              ISL_FORMAT_R24_UNORM_X8_TYPELESS, ISL_FORMAT_R8_UINT),
            swiz_fmt1(VK_FORMAT_BC1_RGB_UNORM_BLOCK,          ISL_FORMAT_BC1_UNORM, RGB1),
   swiz_fmt1(VK_FORMAT_BC1_RGB_SRGB_BLOCK,           ISL_FORMAT_BC1_UNORM_SRGB, RGB1),
   fmt1(VK_FORMAT_BC1_RGBA_UNORM_BLOCK,              ISL_FORMAT_BC1_UNORM),
   fmt1(VK_FORMAT_BC1_RGBA_SRGB_BLOCK,               ISL_FORMAT_BC1_UNORM_SRGB),
   fmt1(VK_FORMAT_BC2_UNORM_BLOCK,                   ISL_FORMAT_BC2_UNORM),
   fmt1(VK_FORMAT_BC2_SRGB_BLOCK,                    ISL_FORMAT_BC2_UNORM_SRGB),
   fmt1(VK_FORMAT_BC3_UNORM_BLOCK,                   ISL_FORMAT_BC3_UNORM),
   fmt1(VK_FORMAT_BC3_SRGB_BLOCK,                    ISL_FORMAT_BC3_UNORM_SRGB),
   fmt1(VK_FORMAT_BC4_UNORM_BLOCK,                   ISL_FORMAT_BC4_UNORM),
   fmt1(VK_FORMAT_BC4_SNORM_BLOCK,                   ISL_FORMAT_BC4_SNORM),
   fmt1(VK_FORMAT_BC5_UNORM_BLOCK,                   ISL_FORMAT_BC5_UNORM),
   fmt1(VK_FORMAT_BC5_SNORM_BLOCK,                   ISL_FORMAT_BC5_SNORM),
   fmt1(VK_FORMAT_BC6H_UFLOAT_BLOCK,                 ISL_FORMAT_BC6H_UF16),
   fmt1(VK_FORMAT_BC6H_SFLOAT_BLOCK,                 ISL_FORMAT_BC6H_SF16),
   fmt1(VK_FORMAT_BC7_UNORM_BLOCK,                   ISL_FORMAT_BC7_UNORM),
   fmt1(VK_FORMAT_BC7_SRGB_BLOCK,                    ISL_FORMAT_BC7_UNORM_SRGB),
   fmt1(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,           ISL_FORMAT_ETC2_RGB8),
   fmt1(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,            ISL_FORMAT_ETC2_SRGB8),
   fmt1(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,         ISL_FORMAT_ETC2_RGB8_PTA),
   fmt1(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,          ISL_FORMAT_ETC2_SRGB8_PTA),
   fmt1(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,         ISL_FORMAT_ETC2_EAC_RGBA8),
   fmt1(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,          ISL_FORMAT_ETC2_EAC_SRGB8_A8),
   fmt1(VK_FORMAT_EAC_R11_UNORM_BLOCK,               ISL_FORMAT_EAC_R11),
   fmt1(VK_FORMAT_EAC_R11_SNORM_BLOCK,               ISL_FORMAT_EAC_SIGNED_R11),
   fmt1(VK_FORMAT_EAC_R11G11_UNORM_BLOCK,            ISL_FORMAT_EAC_RG11),
   fmt1(VK_FORMAT_EAC_R11G11_SNORM_BLOCK,            ISL_FORMAT_EAC_SIGNED_RG11),
   fmt1(VK_FORMAT_ASTC_4x4_SRGB_BLOCK,               ISL_FORMAT_ASTC_LDR_2D_4X4_U8SRGB),
   fmt1(VK_FORMAT_ASTC_5x4_SRGB_BLOCK,               ISL_FORMAT_ASTC_LDR_2D_5X4_U8SRGB),
   fmt1(VK_FORMAT_ASTC_5x5_SRGB_BLOCK,               ISL_FORMAT_ASTC_LDR_2D_5X5_U8SRGB),
   fmt1(VK_FORMAT_ASTC_6x5_SRGB_BLOCK,               ISL_FORMAT_ASTC_LDR_2D_6X5_U8SRGB),
   fmt1(VK_FORMAT_ASTC_6x6_SRGB_BLOCK,               ISL_FORMAT_ASTC_LDR_2D_6X6_U8SRGB),
   fmt1(VK_FORMAT_ASTC_8x5_SRGB_BLOCK,               ISL_FORMAT_ASTC_LDR_2D_8X5_U8SRGB),
   fmt1(VK_FORMAT_ASTC_8x6_SRGB_BLOCK,               ISL_FORMAT_ASTC_LDR_2D_8X6_U8SRGB),
   fmt1(VK_FORMAT_ASTC_8x8_SRGB_BLOCK,               ISL_FORMAT_ASTC_LDR_2D_8X8_U8SRGB),
   fmt1(VK_FORMAT_ASTC_10x5_SRGB_BLOCK,              ISL_FORMAT_ASTC_LDR_2D_10X5_U8SRGB),
   fmt1(VK_FORMAT_ASTC_10x6_SRGB_BLOCK,              ISL_FORMAT_ASTC_LDR_2D_10X6_U8SRGB),
   fmt1(VK_FORMAT_ASTC_10x8_SRGB_BLOCK,              ISL_FORMAT_ASTC_LDR_2D_10X8_U8SRGB),
   fmt1(VK_FORMAT_ASTC_10x10_SRGB_BLOCK,             ISL_FORMAT_ASTC_LDR_2D_10X10_U8SRGB),
   fmt1(VK_FORMAT_ASTC_12x10_SRGB_BLOCK,             ISL_FORMAT_ASTC_LDR_2D_12X10_U8SRGB),
   fmt1(VK_FORMAT_ASTC_12x12_SRGB_BLOCK,             ISL_FORMAT_ASTC_LDR_2D_12X12_U8SRGB),
   fmt1(VK_FORMAT_ASTC_4x4_UNORM_BLOCK,              ISL_FORMAT_ASTC_LDR_2D_4X4_FLT16),
   fmt1(VK_FORMAT_ASTC_5x4_UNORM_BLOCK,              ISL_FORMAT_ASTC_LDR_2D_5X4_FLT16),
   fmt1(VK_FORMAT_ASTC_5x5_UNORM_BLOCK,              ISL_FORMAT_ASTC_LDR_2D_5X5_FLT16),
   fmt1(VK_FORMAT_ASTC_6x5_UNORM_BLOCK,              ISL_FORMAT_ASTC_LDR_2D_6X5_FLT16),
   fmt1(VK_FORMAT_ASTC_6x6_UNORM_BLOCK,              ISL_FORMAT_ASTC_LDR_2D_6X6_FLT16),
   fmt1(VK_FORMAT_ASTC_8x5_UNORM_BLOCK,              ISL_FORMAT_ASTC_LDR_2D_8X5_FLT16),
   fmt1(VK_FORMAT_ASTC_8x6_UNORM_BLOCK,              ISL_FORMAT_ASTC_LDR_2D_8X6_FLT16),
   fmt1(VK_FORMAT_ASTC_8x8_UNORM_BLOCK,              ISL_FORMAT_ASTC_LDR_2D_8X8_FLT16),
   fmt1(VK_FORMAT_ASTC_10x5_UNORM_BLOCK,             ISL_FORMAT_ASTC_LDR_2D_10X5_FLT16),
   fmt1(VK_FORMAT_ASTC_10x6_UNORM_BLOCK,             ISL_FORMAT_ASTC_LDR_2D_10X6_FLT16),
   fmt1(VK_FORMAT_ASTC_10x8_UNORM_BLOCK,             ISL_FORMAT_ASTC_LDR_2D_10X8_FLT16),
   fmt1(VK_FORMAT_ASTC_10x10_UNORM_BLOCK,            ISL_FORMAT_ASTC_LDR_2D_10X10_FLT16),
   fmt1(VK_FORMAT_ASTC_12x10_UNORM_BLOCK,            ISL_FORMAT_ASTC_LDR_2D_12X10_FLT16),
   fmt1(VK_FORMAT_ASTC_12x12_UNORM_BLOCK,            ISL_FORMAT_ASTC_LDR_2D_12X12_FLT16),
   fmt_unsupported(VK_FORMAT_B8G8R8_UNORM),
   fmt_unsupported(VK_FORMAT_B8G8R8_SNORM),
   fmt_unsupported(VK_FORMAT_B8G8R8_USCALED),
   fmt_unsupported(VK_FORMAT_B8G8R8_SSCALED),
   fmt_unsupported(VK_FORMAT_B8G8R8_UINT),
   fmt_unsupported(VK_FORMAT_B8G8R8_SINT),
   fmt_unsupported(VK_FORMAT_B8G8R8_SRGB),
   fmt1(VK_FORMAT_B8G8R8A8_UNORM,                    ISL_FORMAT_B8G8R8A8_UNORM),
   fmt_unsupported(VK_FORMAT_B8G8R8A8_SNORM),
   fmt_unsupported(VK_FORMAT_B8G8R8A8_USCALED),
   fmt_unsupported(VK_FORMAT_B8G8R8A8_SSCALED),
   fmt_unsupported(VK_FORMAT_B8G8R8A8_UINT),
   fmt_unsupported(VK_FORMAT_B8G8R8A8_SINT),
      };
      static const struct anv_format _4444_formats[] = {
      fmt1(VK_FORMAT_A4R4G4B4_UNORM_PACK16, ISL_FORMAT_B4G4R4A4_UNORM),
      };
      static const struct anv_format ycbcr_formats[] = {
      ycbcr_fmt(VK_FORMAT_G8B8G8R8_422_UNORM, 1, true, false,
         ycbcr_fmt(VK_FORMAT_B8G8R8G8_422_UNORM, 1, true, false,
         ycbcr_fmt(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM, 3, true, false,
            ycbcr_plane(0, ISL_FORMAT_R8_UNORM, RGBA),
      ycbcr_fmt(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, 2, true, true,
               ycbcr_fmt(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM, 3, true, false,
            ycbcr_plane(0, ISL_FORMAT_R8_UNORM, RGBA),
      ycbcr_fmt(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM, 2, true, false,
               ycbcr_fmt(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM, 3, true, false,
            ycbcr_plane(0, ISL_FORMAT_R8_UNORM, RGBA),
         fmt_unsupported(VK_FORMAT_R10X6_UNORM_PACK16),
   fmt_unsupported(VK_FORMAT_R10X6G10X6_UNORM_2PACK16),
   fmt_unsupported(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16),
   fmt_unsupported(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16),
   fmt_unsupported(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16),
   fmt_unsupported(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16),
   ycbcr_fmt(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16, 2, false, true,
               fmt_unsupported(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16),
   fmt_unsupported(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16),
   fmt_unsupported(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16),
   fmt_unsupported(VK_FORMAT_R12X4_UNORM_PACK16),
   fmt_unsupported(VK_FORMAT_R12X4G12X4_UNORM_2PACK16),
   fmt_unsupported(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16),
   fmt_unsupported(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16),
   fmt_unsupported(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16),
   fmt_unsupported(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16),
   fmt_unsupported(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16),
   fmt_unsupported(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16),
   fmt_unsupported(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16),
   fmt_unsupported(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16),
   /* TODO: it is possible to enable the following 2 formats, but that
   * requires further refactoring of how we handle multiplanar formats.
   */
   fmt_unsupported(VK_FORMAT_G16B16G16R16_422_UNORM),
            ycbcr_fmt(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM, 3, true, false,
            ycbcr_plane(0, ISL_FORMAT_R16_UNORM, RGBA),
      ycbcr_fmt(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM, 2, true, false,
               ycbcr_fmt(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM, 3, true, false,
            ycbcr_plane(0, ISL_FORMAT_R16_UNORM, RGBA),
      ycbcr_fmt(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM, 2, true, false,
               ycbcr_fmt(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM, 3, true, false,
            ycbcr_plane(0, ISL_FORMAT_R16_UNORM, RGBA),
   };
      static const struct anv_format maintenance5_formats[] = {
      fmt1(VK_FORMAT_A8_UNORM_KHR,                   ISL_FORMAT_A8_UNORM),
      };
      #undef _fmt
   #undef swiz_fmt1
   #undef fmt1
   #undef fmt
      static const struct {
      const struct anv_format *formats;
      } anv_formats[] = {
      [0]                                       = { .formats = main_formats,
         [_VK_EXT_4444_formats_number]             = { .formats = _4444_formats,
         [_VK_KHR_sampler_ycbcr_conversion_number] = { .formats = ycbcr_formats,
         [_VK_KHR_maintenance5_number]             = { .formats = maintenance5_formats,
      };
      const struct anv_format *
   anv_get_format(VkFormat vk_format)
   {
      uint32_t enum_offset = VK_ENUM_OFFSET(vk_format);
            if (ext_number >= ARRAY_SIZE(anv_formats) ||
      enum_offset >= anv_formats[ext_number].n_formats)
         const struct anv_format *format =
         if (format->planes[0].isl_format == ISL_FORMAT_UNSUPPORTED)
               }
      /** Return true if any format plane has non-power-of-two bits-per-block. */
   static bool
   anv_format_has_npot_plane(const struct anv_format *anv_format) {
      for (uint32_t i = 0; i < anv_format->n_planes; ++i) {
      const struct isl_format_layout *isl_layout =
            if (!util_is_power_of_two_or_zero(isl_layout->bpb))
                  }
      /**
   * Exactly one bit must be set in \a aspect.
   *
   * If tiling is VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT, then return the
   * requested anv_format_plane without checking for compatibility with modifiers.
   * It is the caller's responsibility to verify that the the returned
   * anv_format_plane is compatible with a particular modifier.  (Observe that
   * this function has no parameter for the DRM format modifier, and therefore
   * _cannot_ check for compatibility).
   */
   struct anv_format_plane
   anv_get_format_plane(const struct intel_device_info *devinfo,
               {
      const struct anv_format *format = anv_get_format(vk_format);
   const struct anv_format_plane unsupported = {
                  if (format == NULL)
            assert(plane < format->n_planes);
   struct anv_format_plane plane_format = format->planes[plane];
   if (plane_format.isl_format == ISL_FORMAT_UNSUPPORTED)
            if (tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
            if (vk_format_is_depth_or_stencil(vk_format))
            const struct isl_format_layout *isl_layout =
            if (tiling == VK_IMAGE_TILING_OPTIMAL &&
      !util_is_power_of_two_or_zero(isl_layout->bpb)) {
   /* Tiled formats *must* be power-of-two because we need up upload
   * them with the render pipeline.  For 3-channel formats, we fix
   * this by switching them over to RGBX or RGBA formats under the
   * hood.
   */
   enum isl_format rgbx = isl_format_rgb_to_rgbx(plane_format.isl_format);
   if (rgbx != ISL_FORMAT_UNSUPPORTED &&
      isl_format_supports_rendering(devinfo, rgbx)) {
      } else {
      plane_format.isl_format =
                           }
      struct anv_format_plane
   anv_get_format_aspect(const struct intel_device_info *devinfo,
               {
      const uint32_t plane =
            }
      // Format capabilities
      VkFormatFeatureFlags2
   anv_get_image_format_features2(const struct anv_physical_device *physical_device,
                           {
      const struct intel_device_info *devinfo = &physical_device->info;
            if (anv_format == NULL)
            assert((isl_mod_info != NULL) ==
            if (anv_is_format_emulated(physical_device, vk_format)) {
               /* require optimal tiling so that we can decompress on upload */
   if (vk_tiling != VK_IMAGE_TILING_OPTIMAL)
            /* required features for compressed formats */
   flags |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT |
            VK_FORMAT_FEATURE_2_BLIT_SRC_BIT |
                                    if (aspects & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
      if (vk_tiling == VK_IMAGE_TILING_LINEAR ||
                  flags |= VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT |
   VK_FORMAT_FEATURE_2_BLIT_SRC_BIT |
               if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
      flags |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
                                          if (physical_device->video_decode_enabled &&
      anv_format->can_video) {
   flags |= VK_FORMAT_FEATURE_2_VIDEO_DECODE_OUTPUT_BIT_KHR |
               const struct anv_format_plane plane_format =
            if (plane_format.isl_format == ISL_FORMAT_UNSUPPORTED)
            struct anv_format_plane base_plane_format = plane_format;
   if (vk_tiling != VK_IMAGE_TILING_LINEAR) {
      base_plane_format = anv_get_format_plane(devinfo, vk_format, 0,
                                    /* Unlike other surface formats, our sampler requires that the ASTC
   * format only be used on surfaces in non-linearly-tiled memory.
   * Thankfully, we can make an exception for linearly-tiled images that
   * are only used for transfers. blorp_copy will reinterpret any
   * compressed format to an uncompressed one.
   *
   * We handle modifier tilings further down in this function.
   */
   if (vk_tiling == VK_IMAGE_TILING_LINEAR &&
      isl_format_get_layout(plane_format.isl_format)->txc == ISL_TXC_ASTC)
               flags |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT |
            if (isl_format_supports_filtering(devinfo, plane_format.isl_format))
               /* We can render to swizzled formats.  However, if the alpha channel is
   * moved, then blending won't work correctly.  The PRM tells us
   * straight-up not to render to such a surface.
   */
   if (isl_format_supports_rendering(devinfo, plane_format.isl_format) &&
      plane_format.swizzle.a == ISL_CHANNEL_SELECT_ALPHA) {
            /* While we can render to swizzled formats, they don't blend correctly
   * if there are blend constants involved.  The swizzle just remaps the
   * output of the shader to different channels in the texture.  It
   * doesn't change the interpretation of the constant blend factors in
   * COLOR_CALC_STATE.
   */
   if (isl_format_supports_alpha_blending(devinfo, plane_format.isl_format) &&
      isl_swizzle_is_identity(plane_format.swizzle))
            /* Load/store is determined based on base format.  This prevents RGB
   * formats from showing up as load/store capable.
   */
   if (isl_format_supports_typed_reads(devinfo, base_isl_format))
         if (isl_format_supports_typed_writes(devinfo, base_isl_format))
            /* Keep this old behavior on VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT.
   * When KHR_format_features2 is enabled, applications should only rely on
   * it for the list of shader storage extended formats [1]. Before that,
   * this applies to all VkFormats.
   *
   * [1] : https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#features-shaderStorageImageExtendedFormats
   */
   if (flags & VK_FORMAT_FEATURE_2_STORAGE_WRITE_WITHOUT_FORMAT_BIT)
            if (base_isl_format == ISL_FORMAT_R32_SINT ||
      base_isl_format == ISL_FORMAT_R32_UINT ||
   base_isl_format == ISL_FORMAT_R32_FLOAT)
         if (flags) {
      flags |= VK_FORMAT_FEATURE_2_BLIT_SRC_BIT |
                  /* Blit destination requires rendering support. */
   if (isl_format_supports_rendering(devinfo, plane_format.isl_format))
               /* XXX: We handle 3-channel formats by switching them out for RGBX or
   * RGBA formats behind-the-scenes.  This works fine for textures
   * because the upload process will fill in the extra channel.
   * We could also support it for render targets, but it will take
   * substantially more work and we have enough RGBX formats to handle
   * what most clients will want.
   */
   if (vk_tiling == VK_IMAGE_TILING_OPTIMAL &&
      base_isl_format != ISL_FORMAT_UNSUPPORTED &&
   !util_is_power_of_two_or_zero(isl_format_layouts[base_isl_format].bpb) &&
   isl_format_rgb_to_rgbx(base_isl_format) == ISL_FORMAT_UNSUPPORTED) {
   flags &= ~VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT;
               const VkFormatFeatureFlags2 disallowed_ycbcr_image_features =
      VK_FORMAT_FEATURE_2_BLIT_SRC_BIT |
   VK_FORMAT_FEATURE_2_BLIT_DST_BIT |
   VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT |
   VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BLEND_BIT |
         if (anv_format->can_ycbcr) {
      /* The sampler doesn't have support for mid point when it handles YUV on
   * its own.
   */
   if (isl_format_is_yuv(anv_format->planes[0].isl_format)) {
      /* TODO: We've disabled linear implicit reconstruction with the
   * sampler. The failures show a slightly out of range values on the
   * bottom left of the sampled image.
   */
      } else {
      flags |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT |
                     /* We can support cosited chroma locations when handle planes with our
   * own shader snippets.
   */
   const struct vk_format_ycbcr_info *ycbcr_info =
         assert(anv_format->n_planes == ycbcr_info->n_planes);
   for (unsigned p = 0; p < ycbcr_info->n_planes; p++) {
      if (ycbcr_info->planes[p].denominator_scales[0] > 1 ||
      ycbcr_info->planes[p].denominator_scales[1] > 1) {
   flags |= VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT;
                  if (anv_format->n_planes > 1)
               } else if (anv_format->can_video) {
      /* This format is for video decoding. */
               if (vk_tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      if (!isl_drm_modifier_get_score(devinfo, isl_mod_info->modifier))
            /* Try to restrict the supported formats to those in drm_fourcc.h. The
   * VK_EXT_image_drm_format_modifier does not require this (after all, two
   * Vulkan apps could share an image by exchanging its VkFormat instead of
   * a DRM_FORMAT), but there exist no users of such non-drm_fourcc formats
   * yet. And the restriction shrinks our test surface.
   */
   const struct isl_format_layout *isl_layout =
            switch (isl_layout->colorspace) {
   case ISL_COLORSPACE_LINEAR:
   case ISL_COLORSPACE_SRGB:
      /* Each DRM_FORMAT that we support uses unorm (if the DRM format name
   * has no type suffix) or sfloat (if it has suffix F). No format
   * contains mixed types. (as of 2021-06-14)
   */
   if (isl_layout->uniform_channel_type != ISL_UNORM &&
      isl_layout->uniform_channel_type != ISL_SFLOAT)
         case ISL_COLORSPACE_YUV:
      anv_finishme("support YUV colorspace with DRM format modifiers");
      case ISL_COLORSPACE_NONE:
                  /* We could support compressed formats if we wanted to. */
   if (isl_format_is_compressed(plane_format.isl_format))
            /* No non-power-of-two fourcc formats exist.
   *
   * Even if non-power-of-two fourcc formats existed, we could support them
   * only with DRM_FORMAT_MOD_LINEAR.  Tiled formats must be power-of-two
   * because we implement transfers with the render pipeline.
   */
   if (anv_format_has_npot_plane(anv_format))
            if (anv_format->n_planes > 1) {
                     /* VK_ANDROID_external_memory_android_hardware_buffer in Virtio-GPU
   * Venus driver layers on top of VK_EXT_image_drm_format_modifier of
   * the host Vulkan driver, and both VK_FORMAT_G8_B8R8_2PLANE_420_UNORM
   * and VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM and required to support
   * camera/media interop in Android.
   */
   if (vk_format != VK_FORMAT_G8_B8R8_2PLANE_420_UNORM &&
      vk_format != VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM) {
   anv_finishme("support more multi-planar formats with DRM modifiers");
               /* Currently there is no way to properly map memory planes to format
   * planes and aux planes due to the lack of defined ABI for external
   * multi-planar images.
   */
   if (isl_drm_modifier_has_aux(isl_mod_info->modifier)) {
                     if (isl_drm_modifier_has_aux(isl_mod_info->modifier) &&
      !anv_format_supports_ccs_e(devinfo, plane_format.isl_format)) {
               if (isl_drm_modifier_has_aux(isl_mod_info->modifier)) {
      /* Rejection DISJOINT for consistency with the GL driver. In
   * eglCreateImage, we require that the dma_buf for the primary surface
   * and the dma_buf for its aux surface refer to the same bo.
                  /* When the hardware accesses a storage image, it bypasses the aux
   * surface. We could support storage access on images with aux
   * modifiers by resolving the aux surface prior to the storage access.
   */
   flags &= ~VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT;
                  if (devinfo->has_coarse_pixel_primitive_and_cb &&
      vk_format == VK_FORMAT_R8_UINT &&
   vk_tiling == VK_IMAGE_TILING_OPTIMAL)
            }
      static VkFormatFeatureFlags2
   get_buffer_format_features2(const struct intel_device_info *devinfo,
               {
               if (anv_format == NULL)
                     if (isl_format == ISL_FORMAT_UNSUPPORTED)
            if (anv_format->n_planes > 1)
            if (anv_format->can_ycbcr || anv_format->can_video)
            if (vk_format_is_depth_or_stencil(vk_format))
            if (isl_format_supports_sampling(devinfo, isl_format) &&
      !isl_format_is_compressed(isl_format))
         if (isl_format_supports_vertex_fetch(devinfo, isl_format))
            if (isl_is_storage_image_format(devinfo, isl_format))
            if (isl_format == ISL_FORMAT_R32_SINT || isl_format == ISL_FORMAT_R32_UINT)
            if (isl_format_supports_typed_reads(devinfo, isl_format))
         if (isl_format_supports_typed_writes(devinfo, isl_format))
            if (devinfo->has_ray_tracing) {
      switch (vk_format) {
   case VK_FORMAT_R32G32_SFLOAT:
   case VK_FORMAT_R32G32B32_SFLOAT:
   case VK_FORMAT_R16G16_SFLOAT:
   case VK_FORMAT_R16G16B16A16_SFLOAT:
   case VK_FORMAT_R16G16_SNORM:
   case VK_FORMAT_R16G16B16A16_SNORM:
   case VK_FORMAT_R16G16B16A16_UNORM:
   case VK_FORMAT_R16G16_UNORM:
   case VK_FORMAT_R8G8B8A8_UNORM:
   case VK_FORMAT_R8G8_UNORM:
   case VK_FORMAT_R8G8B8A8_SNORM:
   case VK_FORMAT_R8G8_SNORM:
      flags |= VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR;
      default:
                        }
      static void
   get_drm_format_modifier_properties_list(const struct anv_physical_device *physical_device,
               {
               VK_OUTARRAY_MAKE_TYPED(VkDrmFormatModifierPropertiesEXT, out,
                  isl_drm_modifier_info_for_each(isl_mod_info) {
      VkFormatFeatureFlags2 features2 =
      anv_get_image_format_features2(physical_device, vk_format, anv_format,
            VkFormatFeatureFlags features = vk_format_features2_to_features(features2);
   if (!features)
            const uint32_t planes =
      isl_drm_modifier_get_plane_count(&physical_device->info,
               vk_outarray_append_typed(VkDrmFormatModifierPropertiesEXT, &out, out_props) {
      *out_props = (VkDrmFormatModifierPropertiesEXT) {
      .drmFormatModifier = isl_mod_info->modifier,
   .drmFormatModifierPlaneCount = planes,
               }
      static void
   get_drm_format_modifier_properties_list_2(const struct anv_physical_device *physical_device,
               {
               VK_OUTARRAY_MAKE_TYPED(VkDrmFormatModifierProperties2EXT, out,
                  isl_drm_modifier_info_for_each(isl_mod_info) {
      VkFormatFeatureFlags2 features2 =
      anv_get_image_format_features2(physical_device, vk_format, anv_format,
            if (!features2)
            const uint32_t planes =
      isl_drm_modifier_get_plane_count(&physical_device->info,
               vk_outarray_append_typed(VkDrmFormatModifierProperties2EXT, &out, out_props) {
      *out_props = (VkDrmFormatModifierProperties2EXT) {
      .drmFormatModifier = isl_mod_info->modifier,
   .drmFormatModifierPlaneCount = planes,
               }
      void anv_GetPhysicalDeviceFormatProperties2(
      VkPhysicalDevice                            physicalDevice,
   VkFormat                                    vk_format,
      {
      ANV_FROM_HANDLE(anv_physical_device, physical_device, physicalDevice);
   const struct intel_device_info *devinfo = &physical_device->info;
                     VkFormatFeatureFlags2 linear2, optimal2, buffer2;
   linear2 = anv_get_image_format_features2(physical_device, vk_format,
               optimal2 = anv_get_image_format_features2(physical_device, vk_format,
                        pFormatProperties->formatProperties = (VkFormatProperties) {
      .linearTilingFeatures = vk_format_features2_to_features(linear2),
   .optimalTilingFeatures = vk_format_features2_to_features(optimal2),
               vk_foreach_struct(ext, pFormatProperties->pNext) {
      /* Use unsigned since some cases are not in the VkStructureType enum. */
   switch ((unsigned)ext->sType) {
   case VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT:
      get_drm_format_modifier_properties_list(physical_device, vk_format,
               case VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_2_EXT:
      get_drm_format_modifier_properties_list_2(physical_device, vk_format,
               case VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3: {
      VkFormatProperties3 *props = (VkFormatProperties3 *)ext;
   props->linearTilingFeatures = linear2;
   props->optimalTilingFeatures = optimal2;
   props->bufferFeatures = buffer2;
      }
   case VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR:
      /* don't have any thing to use this for yet */
      default:
      anv_debug_ignored_stype(ext->sType);
            }
      static bool
   anv_format_supports_usage(
      VkFormatFeatureFlags2KHR format_feature_flags,
      {
      if (usage_flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
      if (!(format_feature_flags & (VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT |
                           if (usage_flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
      if (!(format_feature_flags & (VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT |
                           if (usage_flags & VK_IMAGE_USAGE_SAMPLED_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT)) {
                     if (usage_flags & VK_IMAGE_USAGE_STORAGE_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT)) {
                     if (usage_flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT)) {
                     if (usage_flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT)) {
                     if (usage_flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
                  if (usage_flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
      /* Ignore this flag because it was removed from the
   * provisional_I_20150910 header.
                  }
      static bool
   anv_formats_are_compatible(
      const struct anv_format *img_fmt, const struct anv_format *img_view_fmt,
   const struct intel_device_info *devinfo, VkImageTiling tiling,
      {
      if (img_view_fmt->vk_format == VK_FORMAT_UNDEFINED)
            if (img_fmt == img_view_fmt)
            /* TODO: Handle multi-planar images that can have view of a plane with
   * possibly different type.
   */
   if (img_fmt->n_planes != 1 || img_view_fmt->n_planes != 1)
            const enum isl_format img_isl_fmt =
         const enum isl_format img_view_isl_fmt =
         if (img_isl_fmt == ISL_FORMAT_UNSUPPORTED ||
      img_view_isl_fmt == ISL_FORMAT_UNSUPPORTED)
         const struct isl_format_layout *img_fmt_layout =
         const struct isl_format_layout *img_view_fmt_layout =
            /* From the Vulkan 1.3.230 spec "12.5. Image Views"
   *
   *    "If image was created with the
   *    VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT flag, format must be
   *    compatible with the image’s format as described above; or must be
   *    an uncompressed format, in which case it must be size-compatible
   *    with the image’s format."
   */
   if (allow_texel_compatible &&
      isl_format_is_compressed(img_isl_fmt) &&
   !isl_format_is_compressed(img_view_isl_fmt) &&
   img_fmt_layout->bpb == img_view_fmt_layout->bpb)
         if (isl_format_is_compressed(img_isl_fmt) !=
      isl_format_is_compressed(img_view_isl_fmt))
         if (!isl_format_is_compressed(img_isl_fmt)) {
      /* From the Vulkan 1.3.224 spec "43.1.6. Format Compatibility Classes":
   *
   *    "Uncompressed color formats are compatible with each other if they
   *    occupy the same number of bits per texel block."
   */
               /* From the Vulkan 1.3.224 spec "43.1.6. Format Compatibility Classes":
   *
   *    "Compressed color formats are compatible with each other if the only
   *    difference between them is the numerical type of the uncompressed
   *    pixels (e.g. signed vs. unsigned, or SRGB vs. UNORM encoding)."
   */
   return img_fmt_layout->txc == img_view_fmt_layout->txc &&
      }
      /* Returns a set of feature flags supported by any of the VkFormat listed in
   * format_list_info or any VkFormat compatible with format.
   */
   static VkFormatFeatureFlags2
   anv_formats_gather_format_features(
      const struct anv_physical_device *physical_device,
   const struct anv_format *format,
   VkImageTiling tiling,
   const struct isl_drm_modifier_info *isl_mod_info,
   const VkImageFormatListCreateInfo *format_list_info,
      {
      const struct intel_device_info *devinfo = &physical_device->info;
            /* We need to check that each of the usage bits are allowed for at least
   * one of the potential formats.
   */
   if (!format_list_info || format_list_info->viewFormatCount == 0) {
      /* If we specify no list of possible formats, we need to assume that
   * every compatible format is possible and consider the features
   * supported by each of them.
   */
   for (uint32_t fmt_arr_ind = 0;
      fmt_arr_ind < ARRAY_SIZE(anv_formats);
   ++fmt_arr_ind) {
   for (uint32_t fmt_ind = 0;
      fmt_ind < anv_formats[fmt_arr_ind].n_formats;
   ++fmt_ind) {
                  if (anv_formats_are_compatible(format, possible_anv_format,
                  VkFormatFeatureFlags2KHR view_format_features =
      anv_get_image_format_features2(physical_device,
                              } else {
      /* If we provide the list of possible formats, then check just them. */
   for (uint32_t i = 0; i < format_list_info->viewFormatCount; ++i) {
                              const struct anv_format *anv_view_format =
         VkFormatFeatureFlags2KHR view_format_features =
      anv_get_image_format_features2(physical_device,
                              }
         static VkResult
   anv_get_image_format_properties(
      struct anv_physical_device *physical_device,
   const VkPhysicalDeviceImageFormatInfo2 *info,
   VkImageFormatProperties *pImageFormatProperties,
   VkSamplerYcbcrConversionImageFormatProperties *pYcbcrImageFormatProperties,
      {
      VkFormatFeatureFlags2 format_feature_flags;
   VkExtent3D maxExtent;
   uint32_t maxMipLevels;
   uint32_t maxArraySize;
   VkSampleCountFlags sampleCounts;
   const struct intel_device_info *devinfo = &physical_device->info;
   const struct anv_format *format = anv_get_format(info->format);
   const struct isl_drm_modifier_info *isl_mod_info = NULL;
   const VkImageFormatListCreateInfo *format_list_info =
            if (format == NULL)
            if (info->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      const VkPhysicalDeviceImageDrmFormatModifierInfoEXT *vk_mod_info =
            isl_mod_info = isl_drm_modifier_get_info(vk_mod_info->drmFormatModifier);
   if (isl_mod_info == NULL)
            /* only allow Y-tiling/Tile4 for video decode. */
   if (info->usage & VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR) {
      if (isl_mod_info->tiling != ISL_TILING_Y0 && isl_mod_info->tiling != ISL_TILING_4)
                           switch (info->type) {
   default:
         case VK_IMAGE_TYPE_1D:
      maxExtent.width = 16384;
   maxExtent.height = 1;
   maxExtent.depth = 1;
   maxMipLevels = 15; /* log2(maxWidth) + 1 */
   maxArraySize = 2048;
   sampleCounts = VK_SAMPLE_COUNT_1_BIT;
      case VK_IMAGE_TYPE_2D:
      /* FINISHME: Does this really differ for cube maps? The documentation
   * for RENDER_SURFACE_STATE suggests so.
   */
   maxExtent.width = 16384;
   maxExtent.height = 16384;
   maxExtent.depth = 1;
   maxMipLevels = 15; /* log2(maxWidth) + 1 */
   maxArraySize = 2048;
   sampleCounts = VK_SAMPLE_COUNT_1_BIT;
      case VK_IMAGE_TYPE_3D:
      maxExtent.width = 2048;
   maxExtent.height = 2048;
   maxExtent.depth = 2048;
   maxMipLevels = 12; /* log2(maxWidth) + 1 */
   maxArraySize = 1;
   sampleCounts = VK_SAMPLE_COUNT_1_BIT;
               /* If any of the format in VkImageFormatListCreateInfo is completely
   * unsupported, report unsupported.
   */
   if ((info->flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) &&
      format_list_info != NULL) {
   for (uint32_t i = 0; i < format_list_info->viewFormatCount; i++) {
      const struct anv_format *view_format =
         if (view_format == NULL)
                  /* From the Vulkan 1.3.218 spec:
   *
   *    "For images created without VK_IMAGE_CREATE_EXTENDED_USAGE_BIT a usage
   *    bit is valid if it is supported for the format the image is created with.
   *    For images created with VK_IMAGE_CREATE_EXTENDED_USAGE_BIT a usage bit
   *    is valid if it is supported for at least one of the formats
   *    a VkImageView created from the image can have."
   *
   *    "VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT specifies that the image can be
   *    used to create a VkImageView with a different format from the image."
   *
   * So, if both VK_IMAGE_CREATE_EXTENDED_USAGE_BIT and
   * VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT are set, views can be created with
   * different usage than the image, so we can't always filter on usage.
   * There is one exception to this below for storage.
   */
   format_feature_flags = anv_get_image_format_features2(physical_device,
                        if (!anv_format_supports_usage(format_feature_flags, info->usage)) {
      /* If image format itself does not support the usage, and we don't allow
   * views formats to support it, then we can't support this usage at all.
   */
   if (!(info->flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) ||
                  /* We don't want emulated formats to gain unexpected usage (storage in
   * particular) from its compatible view formats.
   */
   if (anv_is_format_emulated(physical_device, info->format))
            /* From the Vulkan 1.3.224 spec "43.1.6. Format Compatibility Classes":
   *
   *    "Each depth/stencil format is only compatible with itself."
   *
   * So, other formats also can't help.
   */
   if (vk_format_is_depth_or_stencil(info->format))
            /* Gather all possible format feature flags for the formats listed in
   * the format list or all the compatible formats.
   */
   VkFormatFeatureFlags2 all_formats_feature_flags = format_feature_flags |
      anv_formats_gather_format_features(physical_device, format,
                     if (!anv_format_supports_usage(all_formats_feature_flags, info->usage))
               if (info->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      /* We support modifiers only for "simple" (that is, non-array
   * non-mipmapped single-sample) 2D images.
   */
   if (info->type != VK_IMAGE_TYPE_2D) {
      vk_errorf(physical_device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                           maxArraySize = 1;
   maxMipLevels = 1;
            if (isl_drm_modifier_has_aux(isl_mod_info->modifier) &&
      !anv_formats_ccs_e_compatible(devinfo, info->flags, info->format,
                              /* Our hardware doesn't support 1D compressed textures.
   *    From the SKL PRM, RENDER_SURFACE_STATE::SurfaceFormat:
   *    * This field cannot be a compressed (BC*, DXT*, FXT*, ETC*, EAC*) format
   *       if the Surface Type is SURFTYPE_1D.
   *    * This field cannot be ASTC format if the Surface Type is SURFTYPE_1D.
   */
   if (info->type == VK_IMAGE_TYPE_1D &&
      isl_format_is_compressed(format->planes[0].isl_format)) {
               if (info->tiling == VK_IMAGE_TILING_OPTIMAL &&
      info->type == VK_IMAGE_TYPE_2D &&
   (format_feature_flags & (VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT |
         !(info->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) &&
   !(info->usage & VK_IMAGE_USAGE_STORAGE_BIT) &&
   isl_format_supports_multisampling(devinfo, format->planes[0].isl_format)) {
               if (info->usage & VK_IMAGE_USAGE_STORAGE_BIT) {
      /* Non-power-of-two formats can never be used as storage images.  We
   * only check plane 0 because there are no YCbCr formats with
   * non-power-of-two planes.
   */
   const struct isl_format_layout *isl_layout =
         if (!util_is_power_of_two_or_zero(isl_layout->bpb))
               if (info->flags & VK_IMAGE_CREATE_DISJOINT_BIT) {
      /* From the Vulkan 1.2.149 spec, VkImageCreateInfo:
   *
   *    If format is a multi-planar format, and if imageCreateFormatFeatures
   *    (as defined in Image Creation Limits) does not contain
   *    VK_FORMAT_FEATURE_2_DISJOINT_BIT, then flags must not contain
   *    VK_IMAGE_CREATE_DISJOINT_BIT.
   */
   if (format->n_planes > 1 &&
      !(format_feature_flags & VK_FORMAT_FEATURE_2_DISJOINT_BIT)) {
               /* From the Vulkan 1.2.149 spec, VkImageCreateInfo:
   *
   * If format is not a multi-planar format, and flags does not include
   * VK_IMAGE_CREATE_ALIAS_BIT, flags must not contain
   * VK_IMAGE_CREATE_DISJOINT_BIT.
   */
   if (format->n_planes == 1 &&
      !(info->flags & VK_IMAGE_CREATE_ALIAS_BIT)) {
               if (info->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT &&
      isl_drm_modifier_has_aux(isl_mod_info->modifier)) {
   /* Rejection DISJOINT for consistency with the GL driver. In
   * eglCreateImage, we require that the dma_buf for the primary surface
   * and the dma_buf for its aux surface refer to the same bo.
   */
                  if ((info->flags & VK_IMAGE_CREATE_ALIAS_BIT) && !from_wsi) {
      /* Reject aliasing of images with non-linear DRM format modifiers because:
   *
   * 1. For modifiers with compression, we store aux tracking state in
   *    ANV_IMAGE_MEMORY_BINDING_PRIVATE, which is not aliasable because it's
   *    not client-bound.
   *
   * 2. For tiled modifiers without compression, we may attempt to compress
   *    them behind the scenes, in which case both the aux tracking state
   *    and the CCS data are bound to ANV_IMAGE_MEMORY_BINDING_PRIVATE.
   *
   * 3. For WSI we should ignore ALIAS_BIT because we have the ability to
   *    bind the ANV_MEMORY_BINDING_PRIVATE from the other WSI image.
   */
   if (info->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT &&
      isl_mod_info->modifier != DRM_FORMAT_MOD_LINEAR) {
                  /* From the bspec section entitled "Surface Layout and Tiling",
   * Gfx9 has a 256 GB limitation and Gfx11+ has a 16 TB limitation.
   */
   uint64_t maxResourceSize = 0;
   if (devinfo->ver < 11)
         else
            *pImageFormatProperties = (VkImageFormatProperties) {
      .maxExtent = maxExtent,
   .maxMipLevels = maxMipLevels,
   .maxArrayLayers = maxArraySize,
            /* FINISHME: Accurately calculate
   * VkImageFormatProperties::maxResourceSize.
   */
               if (pYcbcrImageFormatProperties) {
      pYcbcrImageFormatProperties->combinedImageSamplerDescriptorCount =
                     unsupported:
      *pImageFormatProperties = (VkImageFormatProperties) {
      .maxExtent = { 0, 0, 0 },
   .maxMipLevels = 0,
   .maxArrayLayers = 0,
   .sampleCounts = 0,
                  }
      VkResult anv_GetPhysicalDeviceImageFormatProperties(
      VkPhysicalDevice                            physicalDevice,
   VkFormat                                    format,
   VkImageType                                 type,
   VkImageTiling                               tiling,
   VkImageUsageFlags                           usage,
   VkImageCreateFlags                          createFlags,
      {
               const VkPhysicalDeviceImageFormatInfo2 info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
   .pNext = NULL,
   .format = format,
   .type = type,
   .tiling = tiling,
   .usage = usage,
               return anv_get_image_format_properties(physical_device, &info,
      }
         /* Supports opaque fd but not dma_buf. */
   static const VkExternalMemoryProperties opaque_fd_only_props = {
      .externalMemoryFeatures =
      VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT |
      .exportFromImportedHandleTypes =
         .compatibleHandleTypes =
      };
      /* Supports opaque fd and dma_buf. */
   static const VkExternalMemoryProperties opaque_fd_dma_buf_props = {
      .externalMemoryFeatures =
      VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT |
      .exportFromImportedHandleTypes =
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT |
      .compatibleHandleTypes =
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT |
   };
      static const VkExternalMemoryProperties userptr_props = {
      .externalMemoryFeatures = VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT,
   .exportFromImportedHandleTypes = 0,
   .compatibleHandleTypes =
      };
      static const VkExternalMemoryProperties android_buffer_props = {
      .externalMemoryFeatures = VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT |
         .exportFromImportedHandleTypes =
         .compatibleHandleTypes =
      };
         static const VkExternalMemoryProperties android_image_props = {
      /* VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT will be set dynamically */
   .externalMemoryFeatures = VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT |
         .exportFromImportedHandleTypes =
         .compatibleHandleTypes =
      };
      VkResult anv_GetPhysicalDeviceImageFormatProperties2(
      VkPhysicalDevice                            physicalDevice,
   const VkPhysicalDeviceImageFormatInfo2*     base_info,
      {
      ANV_FROM_HANDLE(anv_physical_device, physical_device, physicalDevice);
   const VkPhysicalDeviceExternalImageFormatInfo *external_info = NULL;
   VkExternalImageFormatProperties *external_props = NULL;
   VkSamplerYcbcrConversionImageFormatProperties *ycbcr_props = NULL;
   UNUSED VkAndroidHardwareBufferUsageANDROID *android_usage = NULL;
   VkResult result;
            /* Extract input structs */
   vk_foreach_struct_const(s, base_info->pNext) {
      switch ((unsigned)s->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO:
      external_info = (const void *) s;
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT:
   case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO:
      /* anv_get_image_format_properties will handle these */
      case VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO:
      /* Ignore but don't warn */
      case VK_STRUCTURE_TYPE_WSI_IMAGE_CREATE_INFO_MESA:
      from_wsi = true;
      case VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR:
      /* Ignore but don't warn */
      default:
      anv_debug_ignored_stype(s->sType);
                  /* Extract output structs */
   vk_foreach_struct(s, base_props->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES:
      external_props = (void *) s;
      case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES:
      ycbcr_props = (void *) s;
      case VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_USAGE_ANDROID:
      android_usage = (void *) s;
      default:
      anv_debug_ignored_stype(s->sType);
                  result = anv_get_image_format_properties(physical_device, base_info,
         if (result != VK_SUCCESS)
            bool ahw_supported =
            if (ahw_supported && android_usage) {
      android_usage->androidHardwareBufferUsage =
            /* Limit maxArrayLayers to 1 for AHardwareBuffer based images for now. */
               /* From the Vulkan 1.0.42 spec:
   *
   *    If handleType is 0, vkGetPhysicalDeviceImageFormatProperties2 will
   *    behave as if VkPhysicalDeviceExternalImageFormatInfo was not
   *    present and VkExternalImageFormatProperties will be ignored.
   */
   if (external_info && external_info->handleType != 0) {
      /* Does there exist a method for app and driver to explicitly communicate
   * to each other the image's memory layout?
   */
            switch (base_info->tiling) {
   default:
         case VK_IMAGE_TILING_LINEAR:
      /* The app can query the image's memory layout with
   * vkGetImageSubresourceLayout.
   */
   tiling_has_explicit_layout = true;
      case VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT:
      /* The app can provide the image's memory layout with
   * VkImageDrmFormatModifierExplicitCreateInfoEXT;
   * or the app can query it with vkGetImageSubresourceLayout.
   */
   tiling_has_explicit_layout = true;
      case VK_IMAGE_TILING_OPTIMAL:
      /* The app can neither query nor provide the image's memory layout. */
   tiling_has_explicit_layout = false;
               /* Compatibility between tiling and external memory handles
   * --------------------------------------------------------
   * When importing or exporting an image, there must exist a method that
   * enables the app and driver to agree on the image's memory layout. If no
   * method exists, then we reject image creation here.
   *
   * If the memory handle requires matching
   * VkPhysicalDeviceIDProperties::driverUUID and ::deviceUUID, then the
   * match-requirement guarantees that all users of the image agree on the
   * image's memory layout.
   *
   * If the memory handle does not require matching
   * VkPhysicalDeviceIDProperties::driverUUID nor ::deviceUUID, then we
   * require that the app and driver be able to explicitly communicate to
   * each other the image's memory layout.
   *
   * (For restrictions on driverUUID and deviceUUID, see the Vulkan 1.2.149
   * spec, Table 73 "External memory handle types").
   */
   switch (external_info->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
      if (external_props) {
      if (tiling_has_explicit_layout) {
      /* With an explicit memory layout, we don't care which type of fd
   * the image belongs too. Both OPAQUE_FD and DMA_BUF are
   * interchangeable here.
   */
      } else {
      /* With an implicit memory layout, we must rely on deviceUUID
   * and driverUUID to determine the layout. Therefore DMA_BUF is
   * incompatible here.
   */
         }
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      /* This memory handle has no restrictions on driverUUID nor deviceUUID,
   * and therefore requires explicit memory layout.
   */
   if (!tiling_has_explicit_layout) {
      result = vk_errorf(physical_device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                                 /* With an explicit memory layout, we don't care which type of fd
   * the image belongs too. Both OPAQUE_FD and DMA_BUF are
   * interchangeable here.
   */
   if (external_props)
            case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT:
      /* This memory handle has no restrictions on driverUUID nor deviceUUID,
   * and therefore requires explicit memory layout.
   */
   if (!tiling_has_explicit_layout) {
      result = vk_errorf(physical_device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                                 if (external_props)
            case VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID:
      /* This memory handle is magic. The Vulkan spec says it has no
   * requirements regarding deviceUUID nor driverUUID, but Android still
   * requires support for VK_IMAGE_TILING_OPTIMAL. Android systems
   * communicate the image's memory layout through backdoor channels.
   */
   if (ahw_supported) {
      if (external_props) {
      external_props->externalMemoryProperties = android_image_props;
   if (anv_ahb_format_for_vk_format(base_info->format)) {
      external_props->externalMemoryProperties.externalMemoryFeatures |=
         }
      }
      default:
      /* From the Vulkan 1.0.42 spec:
   *
   *    If handleType is not compatible with the [parameters] specified
   *    in VkPhysicalDeviceImageFormatInfo2, then
   *    vkGetPhysicalDeviceImageFormatProperties2 returns
   *    VK_ERROR_FORMAT_NOT_SUPPORTED.
   */
   result = vk_errorf(physical_device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                                    fail:
      if (result == VK_ERROR_FORMAT_NOT_SUPPORTED) {
      /* From the Vulkan 1.0.42 spec:
   *
   *    If the combination of parameters to
   *    vkGetPhysicalDeviceImageFormatProperties2 is not supported by
   *    the implementation for use in vkCreateImage, then all members of
   *    imageFormatProperties will be filled with zero.
   */
                  }
      void anv_GetPhysicalDeviceSparseImageFormatProperties2(
      VkPhysicalDevice                            physicalDevice,
   const VkPhysicalDeviceSparseImageFormatInfo2* pFormatInfo,
   uint32_t*                                   pPropertyCount,
      {
      ANV_FROM_HANDLE(anv_physical_device, physical_device, physicalDevice);
   const struct intel_device_info *devinfo = &physical_device->info;
   VkImageAspectFlags aspects = vk_format_aspects(pFormatInfo->format);
   VK_OUTARRAY_MAKE_TYPED(VkSparseImageFormatProperties2, props,
            if (!physical_device->has_sparse) {
      if (INTEL_DEBUG(DEBUG_SPARSE))
                     vk_foreach_struct_const(ext, pFormatInfo->pNext)
            if (anv_sparse_image_check_support(physical_device,
                                    VK_IMAGE_CREATE_SPARSE_BINDING_BIT |
               VkExtent3D ds_granularity = {};
            u_foreach_bit(b, aspects) {
               const uint32_t plane =
         struct anv_format_plane anv_format_plane =
      anv_get_format_plane(devinfo, pFormatInfo->format, plane,
      enum isl_format isl_format = anv_format_plane.isl_format;
            VkImageCreateFlags vk_create_flags =
                  isl_surf_usage_flags_t isl_usage =
                  const enum isl_surf_dim isl_surf_dim =
      pFormatInfo->type == VK_IMAGE_TYPE_1D ? ISL_SURF_DIM_1D :
               struct isl_surf isl_surf;
   bool ok = isl_surf_init(&physical_device->isl_dev, &isl_surf,
               .dim = isl_surf_dim,
   .format = isl_format,
   .width = 1,
   .height = 1,
   .depth = 1,
   .levels = 1,
   .array_len = 1,
   .samples = pFormatInfo->samples,
   .min_alignment_B = 0,
   .row_pitch_B = 0,
   if (!ok) {
      /* There's no way to return an error code! */
   assert(false);
   *pPropertyCount = 0;
               VkSparseImageFormatProperties format_props =
      anv_sparse_calc_image_format_properties(physical_device, aspect,
               /* If both depth and stencil are the same, unify them if possible. */
   if (aspect & (VK_IMAGE_ASPECT_DEPTH_BIT |
            if (!ds_props_ptr) {
         } else if (ds_granularity.width ==
            format_props.imageGranularity.width &&
   ds_granularity.height ==
   format_props.imageGranularity.height &&
   ds_granularity.depth ==
   ds_props_ptr->properties.aspectMask |= aspect;
                  vk_outarray_append_typed(VkSparseImageFormatProperties2, &props, p) {
      p->properties = format_props;
   if (aspect & (VK_IMAGE_ASPECT_DEPTH_BIT |
                  }
      void anv_GetPhysicalDeviceExternalBufferProperties(
      VkPhysicalDevice                             physicalDevice,
   const VkPhysicalDeviceExternalBufferInfo*    pExternalBufferInfo,
      {
      /* The Vulkan 1.0.42 spec says "handleType must be a valid
   * VkExternalMemoryHandleTypeFlagBits value" in
   * VkPhysicalDeviceExternalBufferInfo. This differs from
   * VkPhysicalDeviceExternalImageFormatInfo, which surprisingly permits
   * handleType == 0.
   */
            /* All of the current flags are for sparse which we don't support yet.
   * Even when we do support it, doing sparse on external memory sounds
   * sketchy.  Also, just disallowing flags is the safe option.
   */
   if (pExternalBufferInfo->flags)
                     switch (pExternalBufferInfo->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      pExternalBufferProperties->externalMemoryProperties = opaque_fd_dma_buf_props;
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT:
      pExternalBufferProperties->externalMemoryProperties = userptr_props;
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID:
      if (physical_device->vk.supported_extensions.ANDROID_external_memory_android_hardware_buffer) {
      pExternalBufferProperties->externalMemoryProperties = android_buffer_props;
      }
      default:
               unsupported:
      /* From the Vulkan 1.1.113 spec:
   *
   *    compatibleHandleTypes must include at least handleType.
   */
   pExternalBufferProperties->externalMemoryProperties =
      (VkExternalMemoryProperties) {
         }
