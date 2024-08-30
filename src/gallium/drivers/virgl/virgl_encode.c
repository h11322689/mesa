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
   #include <stdint.h>
   #include <assert.h>
   #include <string.h>
      #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "pipe/p_state.h"
   #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_parse.h"
      #include "virgl_context.h"
   #include "virgl_encode.h"
   #include "virtio-gpu/virgl_protocol.h"
   #include "virgl_resource.h"
   #include "virgl_screen.h"
   #include "virgl_video.h"
      #define VIRGL_ENCODE_MAX_DWORDS MIN2(VIRGL_MAX_CMDBUF_DWORDS, VIRGL_CMD0_MAX_DWORDS)
      #define CONV_FORMAT(f) [PIPE_FORMAT_##f] = VIRGL_FORMAT_##f,
      static const enum virgl_formats virgl_formats_conv_table[PIPE_FORMAT_COUNT] = {
      CONV_FORMAT(NONE)
   CONV_FORMAT(B8G8R8A8_UNORM)
   CONV_FORMAT(B8G8R8X8_UNORM)
   CONV_FORMAT(A8R8G8B8_UNORM)
   CONV_FORMAT(X8R8G8B8_UNORM)
   CONV_FORMAT(B5G5R5A1_UNORM)
   CONV_FORMAT(B4G4R4A4_UNORM)
   CONV_FORMAT(B5G6R5_UNORM)
   CONV_FORMAT(R10G10B10A2_UNORM)
   CONV_FORMAT(L8_UNORM)
   CONV_FORMAT(A8_UNORM)
   CONV_FORMAT(I8_UNORM)
   CONV_FORMAT(L8A8_UNORM)
   CONV_FORMAT(L16_UNORM)
   CONV_FORMAT(UYVY)
   CONV_FORMAT(YUYV)
   CONV_FORMAT(Z16_UNORM)
   CONV_FORMAT(Z32_UNORM)
   CONV_FORMAT(Z32_FLOAT)
   CONV_FORMAT(Z24_UNORM_S8_UINT)
   CONV_FORMAT(S8_UINT_Z24_UNORM)
   CONV_FORMAT(Z24X8_UNORM)
   CONV_FORMAT(X8Z24_UNORM)
   CONV_FORMAT(S8_UINT)
   CONV_FORMAT(R64_FLOAT)
   CONV_FORMAT(R64G64_FLOAT)
   CONV_FORMAT(R64G64B64_FLOAT)
   CONV_FORMAT(R64G64B64A64_FLOAT)
   CONV_FORMAT(R32_FLOAT)
   CONV_FORMAT(R32G32_FLOAT)
   CONV_FORMAT(R32G32B32_FLOAT)
   CONV_FORMAT(R32G32B32A32_FLOAT)
   CONV_FORMAT(R32_UNORM)
   CONV_FORMAT(R32G32_UNORM)
   CONV_FORMAT(R32G32B32_UNORM)
   CONV_FORMAT(R32G32B32A32_UNORM)
   CONV_FORMAT(R32_USCALED)
   CONV_FORMAT(R32G32_USCALED)
   CONV_FORMAT(R32G32B32_USCALED)
   CONV_FORMAT(R32G32B32A32_USCALED)
   CONV_FORMAT(R32_SNORM)
   CONV_FORMAT(R32G32_SNORM)
   CONV_FORMAT(R32G32B32_SNORM)
   CONV_FORMAT(R32G32B32A32_SNORM)
   CONV_FORMAT(R32_SSCALED)
   CONV_FORMAT(R32G32_SSCALED)
   CONV_FORMAT(R32G32B32_SSCALED)
   CONV_FORMAT(R32G32B32A32_SSCALED)
   CONV_FORMAT(R16_UNORM)
   CONV_FORMAT(R16G16_UNORM)
   CONV_FORMAT(R16G16B16_UNORM)
   CONV_FORMAT(R16G16B16A16_UNORM)
   CONV_FORMAT(R16_USCALED)
   CONV_FORMAT(R16G16_USCALED)
   CONV_FORMAT(R16G16B16_USCALED)
   CONV_FORMAT(R16G16B16A16_USCALED)
   CONV_FORMAT(R16_SNORM)
   CONV_FORMAT(R16G16_SNORM)
   CONV_FORMAT(R16G16B16_SNORM)
   CONV_FORMAT(R16G16B16A16_SNORM)
   CONV_FORMAT(R16_SSCALED)
   CONV_FORMAT(R16G16_SSCALED)
   CONV_FORMAT(R16G16B16_SSCALED)
   CONV_FORMAT(R16G16B16A16_SSCALED)
   CONV_FORMAT(R8_UNORM)
   CONV_FORMAT(R8G8_UNORM)
   CONV_FORMAT(R8G8B8_UNORM)
   CONV_FORMAT(R8G8B8A8_UNORM)
   CONV_FORMAT(X8B8G8R8_UNORM)
   CONV_FORMAT(R8_USCALED)
   CONV_FORMAT(R8G8_USCALED)
   CONV_FORMAT(R8G8B8_USCALED)
   CONV_FORMAT(R8G8B8A8_USCALED)
   CONV_FORMAT(R8_SNORM)
   CONV_FORMAT(R8G8_SNORM)
   CONV_FORMAT(R8G8B8_SNORM)
   CONV_FORMAT(R8G8B8A8_SNORM)
   CONV_FORMAT(R8_SSCALED)
   CONV_FORMAT(R8G8_SSCALED)
   CONV_FORMAT(R8G8B8_SSCALED)
   CONV_FORMAT(R8G8B8A8_SSCALED)
   CONV_FORMAT(R32_FIXED)
   CONV_FORMAT(R32G32_FIXED)
   CONV_FORMAT(R32G32B32_FIXED)
   CONV_FORMAT(R32G32B32A32_FIXED)
   CONV_FORMAT(R16_FLOAT)
   CONV_FORMAT(R16G16_FLOAT)
   CONV_FORMAT(R16G16B16_FLOAT)
   CONV_FORMAT(R16G16B16A16_FLOAT)
   CONV_FORMAT(L8_SRGB)
   CONV_FORMAT(L8A8_SRGB)
   CONV_FORMAT(R8G8B8_SRGB)
   CONV_FORMAT(A8B8G8R8_SRGB)
   CONV_FORMAT(X8B8G8R8_SRGB)
   CONV_FORMAT(B8G8R8A8_SRGB)
   CONV_FORMAT(B8G8R8X8_SRGB)
   CONV_FORMAT(A8R8G8B8_SRGB)
   CONV_FORMAT(X8R8G8B8_SRGB)
   CONV_FORMAT(R8G8B8A8_SRGB)
   CONV_FORMAT(DXT1_RGB)
   CONV_FORMAT(DXT1_RGBA)
   CONV_FORMAT(DXT3_RGBA)
   CONV_FORMAT(DXT5_RGBA)
   CONV_FORMAT(DXT1_SRGB)
   CONV_FORMAT(DXT1_SRGBA)
   CONV_FORMAT(DXT3_SRGBA)
   CONV_FORMAT(DXT5_SRGBA)
   CONV_FORMAT(RGTC1_UNORM)
   CONV_FORMAT(RGTC1_SNORM)
   CONV_FORMAT(RGTC2_UNORM)
   CONV_FORMAT(RGTC2_SNORM)
   CONV_FORMAT(R8G8_B8G8_UNORM)
   CONV_FORMAT(G8R8_G8B8_UNORM)
   CONV_FORMAT(R8SG8SB8UX8U_NORM)
   CONV_FORMAT(R5SG5SB6U_NORM)
   CONV_FORMAT(A8B8G8R8_UNORM)
   CONV_FORMAT(B5G5R5X1_UNORM)
   CONV_FORMAT(R10G10B10A2_USCALED)
   CONV_FORMAT(R11G11B10_FLOAT)
   CONV_FORMAT(R9G9B9E5_FLOAT)
   CONV_FORMAT(Z32_FLOAT_S8X24_UINT)
   CONV_FORMAT(R1_UNORM)
   CONV_FORMAT(R10G10B10X2_USCALED)
   CONV_FORMAT(R10G10B10X2_SNORM)
   CONV_FORMAT(L4A4_UNORM)
   CONV_FORMAT(B10G10R10A2_UNORM)
   CONV_FORMAT(R10SG10SB10SA2U_NORM)
   CONV_FORMAT(R8G8Bx_SNORM)
   CONV_FORMAT(R8G8B8X8_UNORM)
   CONV_FORMAT(B4G4R4X4_UNORM)
   CONV_FORMAT(X24S8_UINT)
   CONV_FORMAT(S8X24_UINT)
   CONV_FORMAT(X32_S8X24_UINT)
   CONV_FORMAT(B2G3R3_UNORM)
   CONV_FORMAT(L16A16_UNORM)
   CONV_FORMAT(A16_UNORM)
   CONV_FORMAT(I16_UNORM)
   CONV_FORMAT(LATC1_UNORM)
   CONV_FORMAT(LATC1_SNORM)
   CONV_FORMAT(LATC2_UNORM)
   CONV_FORMAT(LATC2_SNORM)
   CONV_FORMAT(A8_SNORM)
   CONV_FORMAT(L8_SNORM)
   CONV_FORMAT(L8A8_SNORM)
   CONV_FORMAT(I8_SNORM)
   CONV_FORMAT(A16_SNORM)
   CONV_FORMAT(L16_SNORM)
   CONV_FORMAT(L16A16_SNORM)
   CONV_FORMAT(I16_SNORM)
   CONV_FORMAT(A16_FLOAT)
   CONV_FORMAT(L16_FLOAT)
   CONV_FORMAT(L16A16_FLOAT)
   CONV_FORMAT(I16_FLOAT)
   CONV_FORMAT(A32_FLOAT)
   CONV_FORMAT(L32_FLOAT)
   CONV_FORMAT(L32A32_FLOAT)
   CONV_FORMAT(I32_FLOAT)
   CONV_FORMAT(YV12)
   CONV_FORMAT(YV16)
   CONV_FORMAT(IYUV)
   CONV_FORMAT(NV12)
   CONV_FORMAT(NV21)
   CONV_FORMAT(A4R4_UNORM)
   CONV_FORMAT(R4A4_UNORM)
   CONV_FORMAT(R8A8_UNORM)
   CONV_FORMAT(A8R8_UNORM)
   CONV_FORMAT(R10G10B10A2_SSCALED)
   CONV_FORMAT(R10G10B10A2_SNORM)
   CONV_FORMAT(B10G10R10A2_USCALED)
   CONV_FORMAT(B10G10R10A2_SSCALED)
   CONV_FORMAT(B10G10R10A2_SNORM)
   CONV_FORMAT(R8_UINT)
   CONV_FORMAT(R8G8_UINT)
   CONV_FORMAT(R8G8B8_UINT)
   CONV_FORMAT(R8G8B8A8_UINT)
   CONV_FORMAT(R8_SINT)
   CONV_FORMAT(R8G8_SINT)
   CONV_FORMAT(R8G8B8_SINT)
   CONV_FORMAT(R8G8B8A8_SINT)
   CONV_FORMAT(R16_UINT)
   CONV_FORMAT(R16G16_UINT)
   CONV_FORMAT(R16G16B16_UINT)
   CONV_FORMAT(R16G16B16A16_UINT)
   CONV_FORMAT(R16_SINT)
   CONV_FORMAT(R16G16_SINT)
   CONV_FORMAT(R16G16B16_SINT)
   CONV_FORMAT(R16G16B16A16_SINT)
   CONV_FORMAT(R32_UINT)
   CONV_FORMAT(R32G32_UINT)
   CONV_FORMAT(R32G32B32_UINT)
   CONV_FORMAT(R32G32B32A32_UINT)
   CONV_FORMAT(R32_SINT)
   CONV_FORMAT(R32G32_SINT)
   CONV_FORMAT(R32G32B32_SINT)
   CONV_FORMAT(R32G32B32A32_SINT)
   CONV_FORMAT(A8_UINT)
   CONV_FORMAT(I8_UINT)
   CONV_FORMAT(L8_UINT)
   CONV_FORMAT(L8A8_UINT)
   CONV_FORMAT(A8_SINT)
   CONV_FORMAT(I8_SINT)
   CONV_FORMAT(L8_SINT)
   CONV_FORMAT(L8A8_SINT)
   CONV_FORMAT(A16_UINT)
   CONV_FORMAT(I16_UINT)
   CONV_FORMAT(L16_UINT)
   CONV_FORMAT(L16A16_UINT)
   CONV_FORMAT(A16_SINT)
   CONV_FORMAT(I16_SINT)
   CONV_FORMAT(L16_SINT)
   CONV_FORMAT(L16A16_SINT)
   CONV_FORMAT(A32_UINT)
   CONV_FORMAT(I32_UINT)
   CONV_FORMAT(L32_UINT)
   CONV_FORMAT(L32A32_UINT)
   CONV_FORMAT(A32_SINT)
   CONV_FORMAT(I32_SINT)
   CONV_FORMAT(L32_SINT)
   CONV_FORMAT(L32A32_SINT)
   CONV_FORMAT(B10G10R10A2_UINT)
   CONV_FORMAT(ETC1_RGB8)
   CONV_FORMAT(R8G8_R8B8_UNORM)
   CONV_FORMAT(G8R8_B8R8_UNORM)
   CONV_FORMAT(R8G8B8X8_SNORM)
   CONV_FORMAT(R8G8B8X8_SRGB)
   CONV_FORMAT(R8G8B8X8_UINT)
   CONV_FORMAT(R8G8B8X8_SINT)
   CONV_FORMAT(B10G10R10X2_UNORM)
   CONV_FORMAT(R16G16B16X16_UNORM)
   CONV_FORMAT(R16G16B16X16_SNORM)
   CONV_FORMAT(R16G16B16X16_FLOAT)
   CONV_FORMAT(R16G16B16X16_UINT)
   CONV_FORMAT(R16G16B16X16_SINT)
   CONV_FORMAT(R32G32B32X32_FLOAT)
   CONV_FORMAT(R32G32B32X32_UINT)
   CONV_FORMAT(R32G32B32X32_SINT)
   CONV_FORMAT(R8A8_SNORM)
   CONV_FORMAT(R16A16_UNORM)
   CONV_FORMAT(R16A16_SNORM)
   CONV_FORMAT(R16A16_FLOAT)
   CONV_FORMAT(R32A32_FLOAT)
   CONV_FORMAT(R8A8_UINT)
   CONV_FORMAT(R8A8_SINT)
   CONV_FORMAT(R16A16_UINT)
   CONV_FORMAT(R16A16_SINT)
   CONV_FORMAT(R32A32_UINT)
   CONV_FORMAT(R32A32_SINT)
   CONV_FORMAT(R10G10B10A2_UINT)
   CONV_FORMAT(B5G6R5_SRGB)
   CONV_FORMAT(BPTC_RGBA_UNORM)
   CONV_FORMAT(BPTC_SRGBA)
   CONV_FORMAT(BPTC_RGB_FLOAT)
   CONV_FORMAT(BPTC_RGB_UFLOAT)
   CONV_FORMAT(G8R8_UNORM)
   CONV_FORMAT(G8R8_SNORM)
   CONV_FORMAT(G16R16_UNORM)
   CONV_FORMAT(G16R16_SNORM)
   CONV_FORMAT(A8B8G8R8_SNORM)
   CONV_FORMAT(X8B8G8R8_SNORM)
   CONV_FORMAT(ETC2_RGB8)
   CONV_FORMAT(ETC2_SRGB8)
   CONV_FORMAT(ETC2_RGB8A1)
   CONV_FORMAT(ETC2_SRGB8A1)
   CONV_FORMAT(ETC2_RGBA8)
   CONV_FORMAT(ETC2_SRGBA8)
   CONV_FORMAT(ETC2_R11_UNORM)
   CONV_FORMAT(ETC2_R11_SNORM)
   CONV_FORMAT(ETC2_RG11_UNORM)
   CONV_FORMAT(ETC2_RG11_SNORM)
   CONV_FORMAT(ASTC_4x4)
   CONV_FORMAT(ASTC_5x4)
   CONV_FORMAT(ASTC_5x5)
   CONV_FORMAT(ASTC_6x5)
   CONV_FORMAT(ASTC_6x6)
   CONV_FORMAT(ASTC_8x5)
   CONV_FORMAT(ASTC_8x6)
   CONV_FORMAT(ASTC_8x8)
   CONV_FORMAT(ASTC_10x5)
   CONV_FORMAT(ASTC_10x6)
   CONV_FORMAT(ASTC_10x8)
   CONV_FORMAT(ASTC_10x10)
   CONV_FORMAT(ASTC_12x10)
   CONV_FORMAT(ASTC_12x12)
   CONV_FORMAT(ASTC_4x4_SRGB)
   CONV_FORMAT(ASTC_5x4_SRGB)
   CONV_FORMAT(ASTC_5x5_SRGB)
   CONV_FORMAT(ASTC_6x5_SRGB)
   CONV_FORMAT(ASTC_6x6_SRGB)
   CONV_FORMAT(ASTC_8x5_SRGB)
   CONV_FORMAT(ASTC_8x6_SRGB)
   CONV_FORMAT(ASTC_8x8_SRGB)
   CONV_FORMAT(ASTC_10x5_SRGB)
   CONV_FORMAT(ASTC_10x6_SRGB)
   CONV_FORMAT(ASTC_10x8_SRGB)
   CONV_FORMAT(ASTC_10x10_SRGB)
   CONV_FORMAT(ASTC_12x10_SRGB)
   CONV_FORMAT(ASTC_12x12_SRGB)
   CONV_FORMAT(R10G10B10X2_UNORM)
   CONV_FORMAT(A4B4G4R4_UNORM)
   CONV_FORMAT(R8_SRGB)
   CONV_FORMAT(R8G8_SRGB)
   CONV_FORMAT(P010)
   CONV_FORMAT(P012)
   CONV_FORMAT(P016)
   CONV_FORMAT(B8G8R8_UNORM)
   CONV_FORMAT(R3G3B2_UNORM)
   CONV_FORMAT(R4G4B4A4_UNORM)
   CONV_FORMAT(R5G5B5A1_UNORM)
   CONV_FORMAT(R5G6B5_UNORM)
   CONV_FORMAT(Y8_400_UNORM)
   CONV_FORMAT(Y8_U8_V8_444_UNORM)
   CONV_FORMAT(Y8_U8_V8_422_UNORM)
   CONV_FORMAT(Y8_U8V8_422_UNORM)
   CONV_FORMAT(Y8_UNORM)
   CONV_FORMAT(YVYU)
   CONV_FORMAT(Z16_UNORM_S8_UINT)
   CONV_FORMAT(Z24_UNORM_S8_UINT_AS_R8G8B8A8)
   CONV_FORMAT(A1B5G5R5_UINT)
   CONV_FORMAT(A1B5G5R5_UNORM)
   CONV_FORMAT(A1R5G5B5_UINT)
   CONV_FORMAT(A1R5G5B5_UNORM)
   CONV_FORMAT(A2B10G10R10_UINT)
   CONV_FORMAT(A2B10G10R10_UNORM)
   CONV_FORMAT(A2R10G10B10_UINT)
   CONV_FORMAT(A2R10G10B10_UNORM)
   CONV_FORMAT(A4B4G4R4_UINT)
   CONV_FORMAT(A4R4G4B4_UINT)
   CONV_FORMAT(A4R4G4B4_UNORM)
   CONV_FORMAT(A8B8G8R8_SINT)
   CONV_FORMAT(A8B8G8R8_SSCALED)
   CONV_FORMAT(A8B8G8R8_UINT)
   CONV_FORMAT(A8B8G8R8_USCALED)
   CONV_FORMAT(A8R8G8B8_SINT)
   CONV_FORMAT(A8R8G8B8_SNORM)
   CONV_FORMAT(A8R8G8B8_UINT)
   CONV_FORMAT(ASTC_3x3x3)
   CONV_FORMAT(ASTC_3x3x3_SRGB)
   CONV_FORMAT(ASTC_4x3x3)
   CONV_FORMAT(ASTC_4x3x3_SRGB)
   CONV_FORMAT(ASTC_4x4x3)
   CONV_FORMAT(ASTC_4x4x3_SRGB)
   CONV_FORMAT(ASTC_4x4x4)
   CONV_FORMAT(ASTC_4x4x4_SRGB)
   CONV_FORMAT(ASTC_5x4x4)
   CONV_FORMAT(ASTC_5x4x4_SRGB)
   CONV_FORMAT(ASTC_5x5x4)
   CONV_FORMAT(ASTC_5x5x4_SRGB)
   CONV_FORMAT(ASTC_5x5x5)
   CONV_FORMAT(ASTC_5x5x5_SRGB)
   CONV_FORMAT(ASTC_6x5x5)
   CONV_FORMAT(ASTC_6x5x5_SRGB)
   CONV_FORMAT(ASTC_6x6x5)
   CONV_FORMAT(ASTC_6x6x5_SRGB)
   CONV_FORMAT(ASTC_6x6x6)
   CONV_FORMAT(ASTC_6x6x6_SRGB)
   CONV_FORMAT(ATC_RGB)
   CONV_FORMAT(ATC_RGBA_EXPLICIT)
   CONV_FORMAT(ATC_RGBA_INTERPOLATED)
   CONV_FORMAT(AYUV)
   CONV_FORMAT(B10G10R10A2_SINT)
   CONV_FORMAT(B10G10R10X2_SINT)
   CONV_FORMAT(B10G10R10X2_SNORM)
   CONV_FORMAT(B2G3R3_UINT)
   CONV_FORMAT(B4G4R4A4_UINT)
   CONV_FORMAT(B5G5R5A1_UINT)
   CONV_FORMAT(B5G6R5_UINT)
   CONV_FORMAT(B8G8R8A8_SINT)
   CONV_FORMAT(B8G8R8A8_SNORM)
   CONV_FORMAT(B8G8R8A8_SSCALED)
   CONV_FORMAT(B8G8R8A8_UINT)
   CONV_FORMAT(B8G8R8A8_USCALED)
   CONV_FORMAT(B8G8_R8G8_UNORM)
   CONV_FORMAT(B8G8R8_SINT)
   CONV_FORMAT(B8G8R8_SNORM)
   CONV_FORMAT(B8G8R8_SRGB)
   CONV_FORMAT(B8G8R8_SSCALED)
   CONV_FORMAT(B8G8R8_UINT)
   CONV_FORMAT(B8G8R8_USCALED)
   CONV_FORMAT(B8G8R8X8_SINT)
   CONV_FORMAT(B8G8R8X8_SNORM)
   CONV_FORMAT(B8G8R8X8_UINT)
   CONV_FORMAT(B8R8_G8R8_UNORM)
   CONV_FORMAT(FXT1_RGB)
   CONV_FORMAT(FXT1_RGBA)
   CONV_FORMAT(G16R16_SINT)
   CONV_FORMAT(G8B8_G8R8_UNORM)
   CONV_FORMAT(G8_B8_R8_420_UNORM)
   CONV_FORMAT(G8_B8R8_420_UNORM)
   CONV_FORMAT(G8R8_SINT)
   CONV_FORMAT(P030)
   CONV_FORMAT(R10G10B10A2_SINT)
   CONV_FORMAT(R10G10B10X2_SINT)
   CONV_FORMAT(R3G3B2_UINT)
   CONV_FORMAT(R4G4B4A4_UINT)
   CONV_FORMAT(R4G4B4X4_UNORM)
   CONV_FORMAT(R5G5B5A1_UINT)
   CONV_FORMAT(R5G5B5X1_UNORM)
   CONV_FORMAT(R5G6B5_SRGB)
   CONV_FORMAT(R5G6B5_UINT)
   CONV_FORMAT(R64G64B64A64_SINT)
   CONV_FORMAT(R64G64B64A64_UINT)
   CONV_FORMAT(R64G64B64_SINT)
   CONV_FORMAT(R64G64B64_UINT)
   CONV_FORMAT(R64G64_SINT)
   CONV_FORMAT(R64G64_UINT)
   CONV_FORMAT(R64_SINT)
   CONV_FORMAT(R64_UINT)
   CONV_FORMAT(R8_B8_G8_420_UNORM)
   CONV_FORMAT(R8_B8G8_420_UNORM)
   CONV_FORMAT(R8B8_R8G8_UNORM)
   CONV_FORMAT(R8_G8_B8_420_UNORM)
   CONV_FORMAT(R8_G8B8_420_UNORM)
   CONV_FORMAT(R8_G8_B8_UNORM)
   CONV_FORMAT(VYUY)
   CONV_FORMAT(X1B5G5R5_UNORM)
   CONV_FORMAT(X1R5G5B5_UNORM)
   CONV_FORMAT(XYUV)
   CONV_FORMAT(X8B8G8R8_SINT)
   CONV_FORMAT(X8R8G8B8_SINT)
   CONV_FORMAT(X8R8G8B8_SNORM)
   CONV_FORMAT(Y16_U16_V16_420_UNORM)
   CONV_FORMAT(Y16_U16_V16_422_UNORM)
   CONV_FORMAT(Y16_U16V16_422_UNORM)
   CONV_FORMAT(Y16_U16_V16_444_UNORM)
   CONV_FORMAT(Y210)
   CONV_FORMAT(Y212)
   CONV_FORMAT(Y216)
   CONV_FORMAT(Y410)
   CONV_FORMAT(Y412)
      };
   #undef CONV_FORMAT
      enum virgl_formats pipe_to_virgl_format(enum pipe_format format)
   {
      enum virgl_formats vformat = virgl_formats_conv_table[format];
   if (format != PIPE_FORMAT_NONE && !vformat)
            }
      enum pipe_format virgl_to_pipe_format(enum virgl_formats format)
   {
               for (pformat = PIPE_FORMAT_NONE; pformat < PIPE_FORMAT_COUNT; pformat++)
      if (virgl_formats_conv_table[pformat] == format)
         debug_printf("VIRGL: virgl format %u not in the format table\n", format);
      }
      static int virgl_encoder_write_cmd_dword(struct virgl_context *ctx,
         {
               if ((ctx->cbuf->cdw + len + 1) > VIRGL_MAX_CMDBUF_DWORDS)
            virgl_encoder_write_dword(ctx->cbuf, dword);
      }
      static void virgl_encoder_emit_resource(struct virgl_screen *vs,
               {
      struct virgl_winsys *vws = vs->vws;
   if (res && res->hw_res)
         else {
            }
      static void virgl_encoder_write_res(struct virgl_context *ctx,
         {
      struct virgl_screen *vs = virgl_screen(ctx->base.screen);
      }
      int virgl_encode_bind_object(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_BIND_OBJECT, object, 1));
   virgl_encoder_write_dword(ctx->cbuf, handle);
      }
      int virgl_encode_delete_object(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_DESTROY_OBJECT, object, 1));
   virgl_encoder_write_dword(ctx->cbuf, handle);
      }
      int virgl_encode_blend_state(struct virgl_context *ctx,
               {
      uint32_t tmp;
            virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_OBJECT, VIRGL_OBJECT_BLEND, VIRGL_OBJ_BLEND_SIZE));
            tmp =
      VIRGL_OBJ_BLEND_S0_INDEPENDENT_BLEND_ENABLE(blend_state->independent_blend_enable) |
   VIRGL_OBJ_BLEND_S0_LOGICOP_ENABLE(blend_state->logicop_enable) |
   VIRGL_OBJ_BLEND_S0_DITHER(blend_state->dither) |
   VIRGL_OBJ_BLEND_S0_ALPHA_TO_COVERAGE(blend_state->alpha_to_coverage) |
                  tmp = VIRGL_OBJ_BLEND_S1_LOGICOP_FUNC(blend_state->logicop_func);
            for (i = 0; i < VIRGL_MAX_COLOR_BUFS; i++) {
      /* We use alpha src factor to pass the advanced blend equation value
   * to the host. By doing so, we don't have to change the protocol.
   */
   uint32_t alpha = (i == 0 && blend_state->advanced_blend_func)
               tmp =
      VIRGL_OBJ_BLEND_S2_RT_BLEND_ENABLE(blend_state->rt[i].blend_enable) |
   VIRGL_OBJ_BLEND_S2_RT_RGB_FUNC(blend_state->rt[i].rgb_func) |
   VIRGL_OBJ_BLEND_S2_RT_RGB_SRC_FACTOR(blend_state->rt[i].rgb_src_factor) |
   VIRGL_OBJ_BLEND_S2_RT_RGB_DST_FACTOR(blend_state->rt[i].rgb_dst_factor)|
   VIRGL_OBJ_BLEND_S2_RT_ALPHA_FUNC(blend_state->rt[i].alpha_func) |
   VIRGL_OBJ_BLEND_S2_RT_ALPHA_SRC_FACTOR(alpha) |
   VIRGL_OBJ_BLEND_S2_RT_ALPHA_DST_FACTOR(blend_state->rt[i].alpha_dst_factor) |
         }
      }
      int virgl_encode_dsa_state(struct virgl_context *ctx,
               {
      uint32_t tmp;
   int i;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_OBJECT, VIRGL_OBJECT_DSA, VIRGL_OBJ_DSA_SIZE));
            tmp = VIRGL_OBJ_DSA_S0_DEPTH_ENABLE(dsa_state->depth_enabled) |
      VIRGL_OBJ_DSA_S0_DEPTH_WRITEMASK(dsa_state->depth_writemask) |
   VIRGL_OBJ_DSA_S0_DEPTH_FUNC(dsa_state->depth_func) |
   VIRGL_OBJ_DSA_S0_ALPHA_ENABLED(dsa_state->alpha_enabled) |
               for (i = 0; i < 2; i++) {
      tmp = VIRGL_OBJ_DSA_S1_STENCIL_ENABLED(dsa_state->stencil[i].enabled) |
      VIRGL_OBJ_DSA_S1_STENCIL_FUNC(dsa_state->stencil[i].func) |
   VIRGL_OBJ_DSA_S1_STENCIL_FAIL_OP(dsa_state->stencil[i].fail_op) |
   VIRGL_OBJ_DSA_S1_STENCIL_ZPASS_OP(dsa_state->stencil[i].zpass_op) |
   VIRGL_OBJ_DSA_S1_STENCIL_ZFAIL_OP(dsa_state->stencil[i].zfail_op) |
   VIRGL_OBJ_DSA_S1_STENCIL_VALUEMASK(dsa_state->stencil[i].valuemask) |
                  virgl_encoder_write_dword(ctx->cbuf, fui(dsa_state->alpha_ref_value));
      }
   int virgl_encode_rasterizer_state(struct virgl_context *ctx,
               {
               virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_OBJECT, VIRGL_OBJECT_RASTERIZER, VIRGL_OBJ_RS_SIZE));
            tmp = VIRGL_OBJ_RS_S0_FLATSHADE(state->flatshade) |
      VIRGL_OBJ_RS_S0_DEPTH_CLIP(state->depth_clip_near) |
   VIRGL_OBJ_RS_S0_CLIP_HALFZ(state->clip_halfz) |
   VIRGL_OBJ_RS_S0_RASTERIZER_DISCARD(state->rasterizer_discard) |
   VIRGL_OBJ_RS_S0_FLATSHADE_FIRST(state->flatshade_first) |
   VIRGL_OBJ_RS_S0_LIGHT_TWOSIZE(state->light_twoside) |
   VIRGL_OBJ_RS_S0_SPRITE_COORD_MODE(state->sprite_coord_mode) |
   VIRGL_OBJ_RS_S0_POINT_QUAD_RASTERIZATION(state->point_quad_rasterization) |
   VIRGL_OBJ_RS_S0_CULL_FACE(state->cull_face) |
   VIRGL_OBJ_RS_S0_FILL_FRONT(state->fill_front) |
   VIRGL_OBJ_RS_S0_FILL_BACK(state->fill_back) |
   VIRGL_OBJ_RS_S0_SCISSOR(state->scissor) |
   VIRGL_OBJ_RS_S0_FRONT_CCW(state->front_ccw) |
   VIRGL_OBJ_RS_S0_CLAMP_VERTEX_COLOR(state->clamp_vertex_color) |
   VIRGL_OBJ_RS_S0_CLAMP_FRAGMENT_COLOR(state->clamp_fragment_color) |
   VIRGL_OBJ_RS_S0_OFFSET_LINE(state->offset_line) |
   VIRGL_OBJ_RS_S0_OFFSET_POINT(state->offset_point) |
   VIRGL_OBJ_RS_S0_OFFSET_TRI(state->offset_tri) |
   VIRGL_OBJ_RS_S0_POLY_SMOOTH(state->poly_smooth) |
   VIRGL_OBJ_RS_S0_POLY_STIPPLE_ENABLE(state->poly_stipple_enable) |
   VIRGL_OBJ_RS_S0_POINT_SMOOTH(state->point_smooth) |
   VIRGL_OBJ_RS_S0_POINT_SIZE_PER_VERTEX(state->point_size_per_vertex) |
   VIRGL_OBJ_RS_S0_MULTISAMPLE(state->multisample) |
   VIRGL_OBJ_RS_S0_LINE_SMOOTH(state->line_smooth) |
   VIRGL_OBJ_RS_S0_LINE_STIPPLE_ENABLE(state->line_stipple_enable) |
   VIRGL_OBJ_RS_S0_LINE_LAST_PIXEL(state->line_last_pixel) |
   VIRGL_OBJ_RS_S0_HALF_PIXEL_CENTER(state->half_pixel_center) |
   VIRGL_OBJ_RS_S0_BOTTOM_EDGE_RULE(state->bottom_edge_rule) |
         virgl_encoder_write_dword(ctx->cbuf, tmp); /* S0 */
   virgl_encoder_write_dword(ctx->cbuf, fui(state->point_size)); /* S1 */
   virgl_encoder_write_dword(ctx->cbuf, state->sprite_coord_enable); /* S2 */
   tmp = VIRGL_OBJ_RS_S3_LINE_STIPPLE_PATTERN(state->line_stipple_pattern) |
      VIRGL_OBJ_RS_S3_LINE_STIPPLE_FACTOR(state->line_stipple_factor) |
      virgl_encoder_write_dword(ctx->cbuf, tmp); /* S3 */
   virgl_encoder_write_dword(ctx->cbuf, fui(state->line_width)); /* S4 */
   virgl_encoder_write_dword(ctx->cbuf, fui(state->offset_units)); /* S5 */
   virgl_encoder_write_dword(ctx->cbuf, fui(state->offset_scale)); /* S6 */
   virgl_encoder_write_dword(ctx->cbuf, fui(state->offset_clamp)); /* S7 */
      }
      static void virgl_emit_shader_header(struct virgl_context *ctx,
                     {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_OBJECT, VIRGL_OBJECT_SHADER, len));
   virgl_encoder_write_dword(ctx->cbuf, handle);
   virgl_encoder_write_dword(ctx->cbuf, type);
   virgl_encoder_write_dword(ctx->cbuf, offlen);
      }
      static void virgl_emit_shader_streamout(struct virgl_context *ctx,
         {
      int num_outputs = 0;
   int i;
            if (so_info)
            virgl_encoder_write_dword(ctx->cbuf, num_outputs);
   if (num_outputs) {
      for (i = 0; i < 4; i++)
            for (i = 0; i < so_info->num_outputs; i++) {
      tmp =
   VIRGL_OBJ_SHADER_SO_OUTPUT_REGISTER_INDEX(so_info->output[i].register_index) |
   VIRGL_OBJ_SHADER_SO_OUTPUT_START_COMPONENT(so_info->output[i].start_component) |
   VIRGL_OBJ_SHADER_SO_OUTPUT_NUM_COMPONENTS(so_info->output[i].num_components) |
   VIRGL_OBJ_SHADER_SO_OUTPUT_BUFFER(so_info->output[i].output_buffer) |
   VIRGL_OBJ_SHADER_SO_OUTPUT_DST_OFFSET(so_info->output[i].dst_offset);
   virgl_encoder_write_dword(ctx->cbuf, tmp);
            }
      int virgl_encode_shader_state(struct virgl_context *ctx,
                                 {
      char *str, *sptr;
   uint32_t shader_len, len;
   bool bret;
   int num_tokens = tgsi_num_tokens(tokens);
   int str_total_size = 65536;
   int retry_size = 1;
   uint32_t left_bytes, base_hdr_size, strm_hdr_size, thispass;
   bool first_pass;
   str = CALLOC(1, str_total_size);
   if (!str)
            do {
               bret = tgsi_dump_str(tokens, TGSI_DUMP_FLOAT_AS_HEX, str, str_total_size);
   if (bret == false) {
      if (virgl_debug & VIRGL_DEBUG_VERBOSE)
         old_size = str_total_size;
   str_total_size = 65536 * retry_size;
   retry_size *= 2;
   str = REALLOC(str, old_size, str_total_size);
   if (!str)
                  if (bret == false)
            if (virgl_debug & VIRGL_DEBUG_TGSI)
            /* virglrenderer before addbd9c5058dcc9d561b20ab747aed58c53499da mis-counts
   * the tokens needed for a BARRIER, so ask it to allocate some more space.
   */
   const char *barrier = str;
   while ((barrier = strstr(barrier + 1, "BARRIER")))
                              base_hdr_size = 5;
   strm_hdr_size = so_info->num_outputs ? so_info->num_outputs * 2 + 4 : 0;
   first_pass = true;
   sptr = str;
   while (left_bytes) {
      uint32_t length, offlen;
   int hdr_len = base_hdr_size + (first_pass ? strm_hdr_size : 0);
   if (ctx->cbuf->cdw + hdr_len + 1 >= VIRGL_ENCODE_MAX_DWORDS)
                     length = MIN2(thispass, left_bytes);
            if (first_pass)
         else
                     if (type == PIPE_SHADER_COMPUTE)
         else
                     sptr += length;
   first_pass = false;
               FREE(str);
      }
         int virgl_encode_clear(struct virgl_context *ctx,
                     {
      int i;
            STATIC_ASSERT(sizeof(qword) == sizeof(depth));
            virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CLEAR, 0, VIRGL_OBJ_CLEAR_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, buffers);
   for (i = 0; i < 4; i++)
         virgl_encoder_write_qword(ctx->cbuf, qword);
   virgl_encoder_write_dword(ctx->cbuf, stencil);
      }
      int virgl_encode_clear_texture(struct virgl_context *ctx,
                           {
      const struct util_format_description *desc = util_format_description(res->b.format);
   unsigned block_bits = desc->block.bits;
   uint32_t arr[4] = {0};
   /* The spec describe <data> as a pointer to an array of between one
   * and four components of texel data that will be used as the source
   * for the constant fill value.
   * Here, we are just copying the memory into <arr>. We do not try to
   * re-create the data array. The host part will take care of interpreting
   * the memory and applying the correct format to the clear call.
   */
            virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CLEAR_TEXTURE, 0, VIRGL_CLEAR_TEXTURE_SIZE));
   virgl_encoder_write_res(ctx, res);
   virgl_encoder_write_dword(ctx->cbuf, level);
   virgl_encoder_write_dword(ctx->cbuf, box->x);
   virgl_encoder_write_dword(ctx->cbuf, box->y);
   virgl_encoder_write_dword(ctx->cbuf, box->z);
   virgl_encoder_write_dword(ctx->cbuf, box->width);
   virgl_encoder_write_dword(ctx->cbuf, box->height);
   virgl_encoder_write_dword(ctx->cbuf, box->depth);
   for (unsigned i = 0; i < 4; i++)
            }
      int virgl_encoder_set_framebuffer_state(struct virgl_context *ctx,
         {
      struct virgl_surface *zsurf = virgl_surface(state->zsbuf);
            virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_FRAMEBUFFER_STATE, 0, VIRGL_SET_FRAMEBUFFER_STATE_SIZE(state->nr_cbufs)));
   virgl_encoder_write_dword(ctx->cbuf, state->nr_cbufs);
   virgl_encoder_write_dword(ctx->cbuf, zsurf ? zsurf->handle : 0);
   for (i = 0; i < state->nr_cbufs; i++) {
      struct virgl_surface *surf = virgl_surface(state->cbufs[i]);
               struct virgl_screen *rs = virgl_screen(ctx->base.screen);
   if (rs->caps.caps.v2.capability_bits & VIRGL_CAP_FB_NO_ATTACH) {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_FRAMEBUFFER_STATE_NO_ATTACH, 0, VIRGL_SET_FRAMEBUFFER_STATE_NO_ATTACH_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, state->width | (state->height << 16));
      }
      }
      int virgl_encoder_set_viewport_states(struct virgl_context *ctx,
                     {
      int i,v;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_VIEWPORT_STATE, 0, VIRGL_SET_VIEWPORT_STATE_SIZE(num_viewports)));
   virgl_encoder_write_dword(ctx->cbuf, start_slot);
   for (v = 0; v < num_viewports; v++) {
      for (i = 0; i < 3; i++)
         for (i = 0; i < 3; i++)
      }
      }
      int virgl_encoder_create_vertex_elements(struct virgl_context *ctx,
                     {
      int i;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_OBJECT, VIRGL_OBJECT_VERTEX_ELEMENTS, VIRGL_OBJ_VERTEX_ELEMENTS_SIZE(num_elements)));
   virgl_encoder_write_dword(ctx->cbuf, handle);
   for (i = 0; i < num_elements; i++) {
      virgl_encoder_write_dword(ctx->cbuf, element[i].src_offset);
   virgl_encoder_write_dword(ctx->cbuf, element[i].instance_divisor);
   virgl_encoder_write_dword(ctx->cbuf, element[i].vertex_buffer_index);
      }
      }
      int virgl_encoder_set_vertex_buffers(struct virgl_context *ctx,
               {
      int i;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_VERTEX_BUFFERS, 0, VIRGL_SET_VERTEX_BUFFERS_SIZE(num_buffers)));
   for (i = 0; i < num_buffers; i++) {
      struct virgl_resource *res = virgl_resource(buffers[i].buffer.resource);
   virgl_encoder_write_dword(ctx->cbuf, ctx->vertex_elements ? ctx->vertex_elements->strides[i] : 0);
   virgl_encoder_write_dword(ctx->cbuf, buffers[i].buffer_offset);
      }
      }
      int virgl_encoder_set_index_buffer(struct virgl_context *ctx,
         {
      int length = VIRGL_SET_INDEX_BUFFER_SIZE(ib);
   struct virgl_resource *res = NULL;
   if (ib)
            virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_INDEX_BUFFER, 0, length));
   virgl_encoder_write_res(ctx, res);
   if (ib) {
      virgl_encoder_write_dword(ctx->cbuf, ib->index_size);
      }
      }
      int virgl_encoder_draw_vbo(struct virgl_context *ctx,
                           {
      uint32_t length = VIRGL_DRAW_VBO_SIZE;
   if (info->mode == MESA_PRIM_PATCHES || drawid_offset > 0)
         if (indirect && indirect->buffer)
         virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_DRAW_VBO, 0, length));
   virgl_encoder_write_dword(ctx->cbuf, draw->start);
   virgl_encoder_write_dword(ctx->cbuf, draw->count);
   virgl_encoder_write_dword(ctx->cbuf, info->mode);
   virgl_encoder_write_dword(ctx->cbuf, !!info->index_size);
   virgl_encoder_write_dword(ctx->cbuf, info->instance_count);
   virgl_encoder_write_dword(ctx->cbuf, info->index_size ? draw->index_bias : 0);
   virgl_encoder_write_dword(ctx->cbuf, info->start_instance);
   virgl_encoder_write_dword(ctx->cbuf, info->primitive_restart);
   virgl_encoder_write_dword(ctx->cbuf, info->primitive_restart ? info->restart_index : 0);
   virgl_encoder_write_dword(ctx->cbuf, info->index_bounds_valid ? info->min_index : 0);
   virgl_encoder_write_dword(ctx->cbuf, info->index_bounds_valid ? info->max_index : ~0);
   if (indirect && indirect->count_from_stream_output)
         else
         if (length >= VIRGL_DRAW_VBO_SIZE_TESS) {
      virgl_encoder_write_dword(ctx->cbuf, ctx->patch_vertices); /* vertices per patch */
      }
   if (length == VIRGL_DRAW_VBO_SIZE_INDIRECT) {
      virgl_encoder_write_res(ctx, virgl_resource(indirect->buffer));
   virgl_encoder_write_dword(ctx->cbuf, indirect->offset);
   virgl_encoder_write_dword(ctx->cbuf, indirect->stride); /* indirect stride */
   virgl_encoder_write_dword(ctx->cbuf, indirect->draw_count); /* indirect draw count */
   virgl_encoder_write_dword(ctx->cbuf, indirect->indirect_draw_count_offset); /* indirect draw count offset */
   if (indirect->indirect_draw_count)
         else
      }
      }
      static int virgl_encoder_create_surface_common(struct virgl_context *ctx,
                     {
      virgl_encoder_write_dword(ctx->cbuf, handle);
   virgl_encoder_write_res(ctx, res);
            assert(templat->texture->target != PIPE_BUFFER);
   virgl_encoder_write_dword(ctx->cbuf, templat->u.tex.level);
               }
      int virgl_encoder_create_surface(struct virgl_context *ctx,
                     {
      if (templat->nr_samples > 0) {
      ASSERTED struct virgl_screen *rs = virgl_screen(ctx->base.screen);
            virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_OBJECT, VIRGL_OBJECT_MSAA_SURFACE, VIRGL_OBJ_MSAA_SURFACE_SIZE));
   virgl_encoder_create_surface_common(ctx, handle, res, templat);
      } else {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_OBJECT, VIRGL_OBJECT_SURFACE, VIRGL_OBJ_SURFACE_SIZE));
                  }
      int virgl_encoder_create_so_target(struct virgl_context *ctx,
                           {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_OBJECT, VIRGL_OBJECT_STREAMOUT_TARGET, VIRGL_OBJ_STREAMOUT_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, handle);
   virgl_encoder_write_res(ctx, res);
   virgl_encoder_write_dword(ctx->cbuf, buffer_offset);
   virgl_encoder_write_dword(ctx->cbuf, buffer_size);
      }
      enum virgl_transfer3d_encode_stride {
      /* The stride and layer_stride are explicitly specified in the command. */
   virgl_transfer3d_explicit_stride,
   /* The stride and layer_stride are inferred by the host. In this case, the
   * host will use the image stride and layer_stride for the specified level.
   */
      };
      static void virgl_encoder_transfer3d_common(struct virgl_screen *vs,
                        {
      struct pipe_transfer *transfer = &xfer->base;
   unsigned stride;
            if (encode_stride == virgl_transfer3d_explicit_stride) {
      stride = transfer->stride;
      } else if (encode_stride == virgl_transfer3d_host_inferred_stride) {
      stride = 0;
      } else {
                  /* We cannot use virgl_encoder_emit_resource with transfer->resource here
   * because transfer->resource might have a different virgl_hw_res than what
   * this transfer targets, which is saved in xfer->hw_res.
   */
   vs->vws->emit_res(vs->vws, buf, xfer->hw_res, true);
   virgl_encoder_write_dword(buf, transfer->level);
   virgl_encoder_write_dword(buf, transfer->usage);
   virgl_encoder_write_dword(buf, stride);
   virgl_encoder_write_dword(buf, layer_stride);
   virgl_encoder_write_dword(buf, transfer->box.x);
   virgl_encoder_write_dword(buf, transfer->box.y);
   virgl_encoder_write_dword(buf, transfer->box.z);
   virgl_encoder_write_dword(buf, transfer->box.width);
   virgl_encoder_write_dword(buf, transfer->box.height);
      }
      int virgl_encoder_flush_frontbuffer(struct virgl_context *ctx,
         {
   //   virgl_encoder_write_dword(ctx->cbuf, VIRGL_CMD0(VIRGL_CCMD_FLUSH_FRONTUBFFER, 0, 1));
   //   virgl_encoder_write_dword(ctx->cbuf, res_handle);
         }
      int virgl_encode_sampler_state(struct virgl_context *ctx,
               {
      uint32_t tmp;
   int i;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_OBJECT, VIRGL_OBJECT_SAMPLER_STATE, VIRGL_OBJ_SAMPLER_STATE_SIZE));
            tmp = VIRGL_OBJ_SAMPLE_STATE_S0_WRAP_S(state->wrap_s) |
      VIRGL_OBJ_SAMPLE_STATE_S0_WRAP_T(state->wrap_t) |
   VIRGL_OBJ_SAMPLE_STATE_S0_WRAP_R(state->wrap_r) |
   VIRGL_OBJ_SAMPLE_STATE_S0_MIN_IMG_FILTER(state->min_img_filter) |
   VIRGL_OBJ_SAMPLE_STATE_S0_MIN_MIP_FILTER(state->min_mip_filter) |
   VIRGL_OBJ_SAMPLE_STATE_S0_MAG_IMG_FILTER(state->mag_img_filter) |
   VIRGL_OBJ_SAMPLE_STATE_S0_COMPARE_MODE(state->compare_mode) |
   VIRGL_OBJ_SAMPLE_STATE_S0_COMPARE_FUNC(state->compare_func) |
   VIRGL_OBJ_SAMPLE_STATE_S0_SEAMLESS_CUBE_MAP(state->seamless_cube_map) |
         virgl_encoder_write_dword(ctx->cbuf, tmp);
   virgl_encoder_write_dword(ctx->cbuf, fui(state->lod_bias));
   virgl_encoder_write_dword(ctx->cbuf, fui(state->min_lod));
   virgl_encoder_write_dword(ctx->cbuf, fui(state->max_lod));
   for (i = 0; i <  4; i++)
            }
         int virgl_encode_sampler_view(struct virgl_context *ctx,
                     {
      unsigned elem_size = util_format_get_blocksize(state->format);
   struct virgl_screen *rs = virgl_screen(ctx->base.screen);
   uint32_t tmp;
   uint32_t dword_fmt_target = pipe_to_virgl_format(state->format);
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_OBJECT, VIRGL_OBJECT_SAMPLER_VIEW, VIRGL_OBJ_SAMPLER_VIEW_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, handle);
   virgl_encoder_write_res(ctx, res);
   if (rs->caps.caps.v2.capability_bits & VIRGL_CAP_TEXTURE_VIEW)
   dword_fmt_target |= (state->target << 24);
   virgl_encoder_write_dword(ctx->cbuf, dword_fmt_target);
   if (res->b.target == PIPE_BUFFER) {
      virgl_encoder_write_dword(ctx->cbuf, state->u.buf.offset / elem_size);
      } else {
      if (res->metadata.plane) {
      assert(state->u.tex.first_layer == 0 && state->u.tex.last_layer == 0);
      } else {
         }
      }
   tmp = VIRGL_OBJ_SAMPLER_VIEW_SWIZZLE_R(state->swizzle_r) |
      VIRGL_OBJ_SAMPLER_VIEW_SWIZZLE_G(state->swizzle_g) |
   VIRGL_OBJ_SAMPLER_VIEW_SWIZZLE_B(state->swizzle_b) |
      virgl_encoder_write_dword(ctx->cbuf, tmp);
      }
      int virgl_encode_set_sampler_views(struct virgl_context *ctx,
                           {
      int i;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_SAMPLER_VIEWS, 0, VIRGL_SET_SAMPLER_VIEWS_SIZE(num_views)));
   virgl_encoder_write_dword(ctx->cbuf, virgl_shader_stage_convert(shader_type));
   virgl_encoder_write_dword(ctx->cbuf, start_slot);
   for (i = 0; i < num_views; i++) {
      uint32_t handle = views[i] ? views[i]->handle : 0;
      }
      }
      int virgl_encode_bind_sampler_states(struct virgl_context *ctx,
                           {
      int i;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_BIND_SAMPLER_STATES, 0, VIRGL_BIND_SAMPLER_STATES(num_handles)));
   virgl_encoder_write_dword(ctx->cbuf, virgl_shader_stage_convert(shader_type));
   virgl_encoder_write_dword(ctx->cbuf, start_slot);
   for (i = 0; i < num_handles; i++)
            }
      int virgl_encoder_write_constant_buffer(struct virgl_context *ctx,
                           {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_CONSTANT_BUFFER, 0, size + 2));
   virgl_encoder_write_dword(ctx->cbuf, virgl_shader_stage_convert(shader));
   virgl_encoder_write_dword(ctx->cbuf, index);
   if (data)
            }
      int virgl_encoder_set_uniform_buffer(struct virgl_context *ctx,
                                 {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_UNIFORM_BUFFER, 0, VIRGL_SET_UNIFORM_BUFFER_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, virgl_shader_stage_convert(shader));
   virgl_encoder_write_dword(ctx->cbuf, index);
   virgl_encoder_write_dword(ctx->cbuf, offset);
   virgl_encoder_write_dword(ctx->cbuf, length);
   virgl_encoder_write_res(ctx, res);
      }
         int virgl_encoder_set_stencil_ref(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_STENCIL_REF, 0, VIRGL_SET_STENCIL_REF_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, VIRGL_STENCIL_REF_VAL(ref->ref_value[0] , (ref->ref_value[1])));
      }
      int virgl_encoder_set_blend_color(struct virgl_context *ctx,
         {
      int i;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_BLEND_COLOR, 0, VIRGL_SET_BLEND_COLOR_SIZE));
   for (i = 0; i < 4; i++)
            }
      int virgl_encoder_set_scissor_state(struct virgl_context *ctx,
                     {
      int i;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_SCISSOR_STATE, 0, VIRGL_SET_SCISSOR_STATE_SIZE(num_scissors)));
   virgl_encoder_write_dword(ctx->cbuf, start_slot);
   for (i = 0; i < num_scissors; i++) {
      virgl_encoder_write_dword(ctx->cbuf, (ss[i].minx | ss[i].miny << 16));
      }
      }
      void virgl_encoder_set_polygon_stipple(struct virgl_context *ctx,
         {
      int i;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_POLYGON_STIPPLE, 0, VIRGL_POLYGON_STIPPLE_SIZE));
   for (i = 0; i < VIRGL_POLYGON_STIPPLE_SIZE; i++) {
            }
      void virgl_encoder_set_sample_mask(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_SAMPLE_MASK, 0, VIRGL_SET_SAMPLE_MASK_SIZE));
      }
      void virgl_encoder_set_min_samples(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_MIN_SAMPLES, 0, VIRGL_SET_MIN_SAMPLES_SIZE));
      }
      void virgl_encoder_set_clip_state(struct virgl_context *ctx,
         {
      int i, j;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_CLIP_STATE, 0, VIRGL_SET_CLIP_STATE_SIZE));
   for (i = 0; i < VIRGL_MAX_CLIP_PLANES; i++) {
      for (j = 0; j < 4; j++) {
               }
      int virgl_encode_resource_copy_region(struct virgl_context *ctx,
                                       {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_RESOURCE_COPY_REGION, 0, VIRGL_CMD_RESOURCE_COPY_REGION_SIZE));
   virgl_encoder_write_res(ctx, dst_res);
   virgl_encoder_write_dword(ctx->cbuf, dst_level);
   virgl_encoder_write_dword(ctx->cbuf, dstx);
   virgl_encoder_write_dword(ctx->cbuf, dsty);
   virgl_encoder_write_dword(ctx->cbuf, dstz);
   virgl_encoder_write_res(ctx, src_res);
   virgl_encoder_write_dword(ctx->cbuf, src_level);
   virgl_encoder_write_dword(ctx->cbuf, src_box->x);
   virgl_encoder_write_dword(ctx->cbuf, src_box->y);
   virgl_encoder_write_dword(ctx->cbuf, src_box->z);
   virgl_encoder_write_dword(ctx->cbuf, src_box->width);
   virgl_encoder_write_dword(ctx->cbuf, src_box->height);
   virgl_encoder_write_dword(ctx->cbuf, src_box->depth);
      }
      int virgl_encode_blit(struct virgl_context *ctx,
                     {
      uint32_t tmp;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_BLIT, 0, VIRGL_CMD_BLIT_SIZE));
   tmp = VIRGL_CMD_BLIT_S0_MASK(blit->mask) |
      VIRGL_CMD_BLIT_S0_FILTER(blit->filter) |
   VIRGL_CMD_BLIT_S0_SCISSOR_ENABLE(blit->scissor_enable) |
   VIRGL_CMD_BLIT_S0_RENDER_CONDITION_ENABLE(blit->render_condition_enable) |
      virgl_encoder_write_dword(ctx->cbuf, tmp);
   virgl_encoder_write_dword(ctx->cbuf, (blit->scissor.minx | blit->scissor.miny << 16));
            virgl_encoder_write_res(ctx, dst_res);
   virgl_encoder_write_dword(ctx->cbuf, blit->dst.level);
   virgl_encoder_write_dword(ctx->cbuf, pipe_to_virgl_format(blit->dst.format));
   virgl_encoder_write_dword(ctx->cbuf, blit->dst.box.x);
   virgl_encoder_write_dword(ctx->cbuf, blit->dst.box.y);
   virgl_encoder_write_dword(ctx->cbuf, blit->dst.box.z);
   virgl_encoder_write_dword(ctx->cbuf, blit->dst.box.width);
   virgl_encoder_write_dword(ctx->cbuf, blit->dst.box.height);
            virgl_encoder_write_res(ctx, src_res);
   virgl_encoder_write_dword(ctx->cbuf, blit->src.level);
   virgl_encoder_write_dword(ctx->cbuf, pipe_to_virgl_format(blit->src.format));
   virgl_encoder_write_dword(ctx->cbuf, blit->src.box.x);
   virgl_encoder_write_dword(ctx->cbuf, blit->src.box.y);
   virgl_encoder_write_dword(ctx->cbuf, blit->src.box.z);
   virgl_encoder_write_dword(ctx->cbuf, blit->src.box.width);
   virgl_encoder_write_dword(ctx->cbuf, blit->src.box.height);
   virgl_encoder_write_dword(ctx->cbuf, blit->src.box.depth);
      }
      int virgl_encoder_create_query(struct virgl_context *ctx,
                                 {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_OBJECT, VIRGL_OBJECT_QUERY, VIRGL_OBJ_QUERY_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, handle);
   virgl_encoder_write_dword(ctx->cbuf, ((query_type & 0xffff) | (query_index << 16)));
   virgl_encoder_write_dword(ctx->cbuf, offset);
   virgl_encoder_write_res(ctx, res);
      }
      int virgl_encoder_begin_query(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_BEGIN_QUERY, 0, 1));
   virgl_encoder_write_dword(ctx->cbuf, handle);
      }
      int virgl_encoder_end_query(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_END_QUERY, 0, 1));
   virgl_encoder_write_dword(ctx->cbuf, handle);
      }
      int virgl_encoder_get_query_result(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_GET_QUERY_RESULT, 0, 2));
   virgl_encoder_write_dword(ctx->cbuf, handle);
   virgl_encoder_write_dword(ctx->cbuf, wait ? 1 : 0);
      }
      int virgl_encoder_render_condition(struct virgl_context *ctx,
               {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_RENDER_CONDITION, 0, VIRGL_RENDER_CONDITION_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, handle);
   virgl_encoder_write_dword(ctx->cbuf, condition);
   virgl_encoder_write_dword(ctx->cbuf, mode);
      }
      int virgl_encoder_set_so_targets(struct virgl_context *ctx,
                     {
               virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_STREAMOUT_TARGETS, 0, num_targets + 1));
   virgl_encoder_write_dword(ctx->cbuf, append_bitmask);
   for (i = 0; i < num_targets; i++) {
      struct virgl_so_target *tg = virgl_so_target(targets[i]);
      }
      }
         int virgl_encoder_set_sub_ctx(struct virgl_context *ctx, uint32_t sub_ctx_id)
   {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_SUB_CTX, 0, 1));
   virgl_encoder_write_dword(ctx->cbuf, sub_ctx_id);
      }
      int virgl_encoder_create_sub_ctx(struct virgl_context *ctx, uint32_t sub_ctx_id)
   {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_SUB_CTX, 0, 1));
   virgl_encoder_write_dword(ctx->cbuf, sub_ctx_id);
      }
      int virgl_encoder_destroy_sub_ctx(struct virgl_context *ctx, uint32_t sub_ctx_id)
   {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_DESTROY_SUB_CTX, 0, 1));
   virgl_encoder_write_dword(ctx->cbuf, sub_ctx_id);
      }
      int virgl_encode_link_shader(struct virgl_context *ctx, uint32_t *handles)
   {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_LINK_SHADER, 0, VIRGL_LINK_SHADER_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, handles[PIPE_SHADER_VERTEX]);
   virgl_encoder_write_dword(ctx->cbuf, handles[PIPE_SHADER_FRAGMENT]);
   virgl_encoder_write_dword(ctx->cbuf, handles[PIPE_SHADER_GEOMETRY]);
   virgl_encoder_write_dword(ctx->cbuf, handles[PIPE_SHADER_TESS_CTRL]);
   virgl_encoder_write_dword(ctx->cbuf, handles[PIPE_SHADER_TESS_EVAL]);
   virgl_encoder_write_dword(ctx->cbuf, handles[PIPE_SHADER_COMPUTE]);
      }
      int virgl_encode_bind_shader(struct virgl_context *ctx,
               {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_BIND_SHADER, 0, 2));
   virgl_encoder_write_dword(ctx->cbuf, handle);
   virgl_encoder_write_dword(ctx->cbuf, virgl_shader_stage_convert(type));
      }
      int virgl_encode_set_tess_state(struct virgl_context *ctx,
               {
      int i;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_TESS_STATE, 0, 6));
   for (i = 0; i < 4; i++)
         for (i = 0; i < 2; i++)
            }
      int virgl_encode_set_shader_buffers(struct virgl_context *ctx,
                     {
      int i;
            virgl_encoder_write_dword(ctx->cbuf, virgl_shader_stage_convert(shader));
   virgl_encoder_write_dword(ctx->cbuf, start_slot);
   for (i = 0; i < count; i++) {
      if (buffers && buffers[i].buffer) {
      struct virgl_resource *res = virgl_resource(buffers[i].buffer);
   virgl_encoder_write_dword(ctx->cbuf, buffers[i].buffer_offset);
                  util_range_add(&res->b, &res->valid_buffer_range, buffers[i].buffer_offset,
            } else {
      virgl_encoder_write_dword(ctx->cbuf, 0);
   virgl_encoder_write_dword(ctx->cbuf, 0);
         }
      }
      int virgl_encode_set_hw_atomic_buffers(struct virgl_context *ctx,
               {
      int i;
            virgl_encoder_write_dword(ctx->cbuf, start_slot);
   for (i = 0; i < count; i++) {
      if (buffers && buffers[i].buffer) {
      struct virgl_resource *res = virgl_resource(buffers[i].buffer);
   virgl_encoder_write_dword(ctx->cbuf, buffers[i].buffer_offset);
                  util_range_add(&res->b, &res->valid_buffer_range, buffers[i].buffer_offset,
            } else {
      virgl_encoder_write_dword(ctx->cbuf, 0);
   virgl_encoder_write_dword(ctx->cbuf, 0);
         }
      }
      int virgl_encode_set_shader_images(struct virgl_context *ctx,
                     {
      int i;
            virgl_encoder_write_dword(ctx->cbuf, virgl_shader_stage_convert(shader));
   virgl_encoder_write_dword(ctx->cbuf, start_slot);
   for (i = 0; i < count; i++) {
      if (images && images[i].resource) {
      struct virgl_resource *res = virgl_resource(images[i].resource);
   virgl_encoder_write_dword(ctx->cbuf, pipe_to_virgl_format(images[i].format));
   virgl_encoder_write_dword(ctx->cbuf, images[i].access);
   virgl_encoder_write_dword(ctx->cbuf, images[i].u.buf.offset);
                  if (res->b.target == PIPE_BUFFER) {
      util_range_add(&res->b, &res->valid_buffer_range, images[i].u.buf.offset,
      }
      } else {
      virgl_encoder_write_dword(ctx->cbuf, 0);
   virgl_encoder_write_dword(ctx->cbuf, 0);
   virgl_encoder_write_dword(ctx->cbuf, 0);
   virgl_encoder_write_dword(ctx->cbuf, 0);
         }
      }
      int virgl_encode_memory_barrier(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_MEMORY_BARRIER, 0, 1));
   virgl_encoder_write_dword(ctx->cbuf, flags);
      }
      int virgl_encode_launch_grid(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_LAUNCH_GRID, 0, VIRGL_LAUNCH_GRID_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, grid_info->block[0]);
   virgl_encoder_write_dword(ctx->cbuf, grid_info->block[1]);
   virgl_encoder_write_dword(ctx->cbuf, grid_info->block[2]);
   virgl_encoder_write_dword(ctx->cbuf, grid_info->grid[0]);
   virgl_encoder_write_dword(ctx->cbuf, grid_info->grid[1]);
   virgl_encoder_write_dword(ctx->cbuf, grid_info->grid[2]);
   if (grid_info->indirect) {
      struct virgl_resource *res = virgl_resource(grid_info->indirect);
      } else
         virgl_encoder_write_dword(ctx->cbuf, grid_info->indirect_offset);
      }
      int virgl_encode_texture_barrier(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_TEXTURE_BARRIER, 0, 1));
   virgl_encoder_write_dword(ctx->cbuf, flags);
      }
      int virgl_encode_host_debug_flagstring(struct virgl_context *ctx,
         {
      unsigned long slen = strlen(flagstring) + 1;
   uint32_t sslen;
            if (!slen)
            if (slen > 4 * 0xffff) {
      debug_printf("VIRGL: host debug flag string too long, will be truncated\n");
               sslen = (uint32_t )(slen + 3) / 4;
            virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_DEBUG_FLAGS, 0, sslen));
   virgl_encoder_write_block(ctx->cbuf, (const uint8_t *)flagstring, string_length);
      }
      int virgl_encode_tweak(struct virgl_context *ctx, enum vrend_tweak_type tweak, uint32_t value)
   {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_SET_TWEAKS, 0, VIRGL_SET_TWEAKS_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, tweak);
   virgl_encoder_write_dword(ctx->cbuf, value);
      }
         int virgl_encode_get_query_result_qbo(struct virgl_context *ctx,
                                 {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_GET_QUERY_RESULT_QBO, 0, VIRGL_QUERY_RESULT_QBO_SIZE));
   virgl_encoder_write_dword(ctx->cbuf, handle);
   virgl_encoder_write_res(ctx, res);
   virgl_encoder_write_dword(ctx->cbuf, wait ? 1 : 0);
   virgl_encoder_write_dword(ctx->cbuf, result_type);
   virgl_encoder_write_dword(ctx->cbuf, offset);
   virgl_encoder_write_dword(ctx->cbuf, index);
      }
      void virgl_encode_transfer(struct virgl_screen *vs, struct virgl_cmd_buf *buf,
         {
      uint32_t command;
   struct virgl_resource *vres = virgl_resource(trans->base.resource);
   enum virgl_transfer3d_encode_stride stride_type =
            if (trans->base.box.depth == 1 && trans->base.level == 0 &&
      trans->base.resource->target == PIPE_TEXTURE_2D &&
   vres->blob_mem == VIRGL_BLOB_MEM_HOST3D_GUEST)
         command = VIRGL_CMD0(VIRGL_CCMD_TRANSFER3D, 0, VIRGL_TRANSFER3D_SIZE);
   virgl_encoder_write_dword(buf, command);
   virgl_encoder_transfer3d_common(vs, buf, trans, stride_type);
   virgl_encoder_write_dword(buf, trans->offset);
      }
      void virgl_encode_copy_transfer(struct virgl_context *ctx,
         {
      uint32_t command;
   struct virgl_screen *vs = virgl_screen(ctx->base.screen);
   // set always synchronized to 1, second bit is used for direction
            if (vs->caps.caps.v2.capability_bits_v2 & VIRGL_CAP_V2_COPY_TRANSFER_BOTH_DIRECTIONS) {
      if (trans->direction == VIRGL_TRANSFER_TO_HOST) {
         } else if (trans->direction == VIRGL_TRANSFER_FROM_HOST) {
         } else {
      // something wrong happened here
         }
   assert(trans->copy_src_hw_res);
   command = VIRGL_CMD0(VIRGL_CCMD_COPY_TRANSFER3D, 0, VIRGL_COPY_TRANSFER3D_SIZE);
      virgl_encoder_write_cmd_dword(ctx, command);
   /* Copy transfers need to explicitly specify the stride, since it may differ
   * from the image stride.
   */
   virgl_encoder_transfer3d_common(vs, ctx->cbuf, trans, virgl_transfer3d_explicit_stride);
   vs->vws->emit_res(vs->vws, ctx->cbuf, trans->copy_src_hw_res, true);
   virgl_encoder_write_dword(ctx->cbuf, trans->copy_src_offset);
      }
      void virgl_encode_end_transfers(struct virgl_cmd_buf *buf)
   {
      uint32_t command, diff;
   diff = VIRGL_MAX_TBUF_DWORDS - buf->cdw;
   if (diff) {
      command = VIRGL_CMD0(VIRGL_CCMD_END_TRANSFERS, 0, diff - 1);
         }
      void virgl_encode_get_memory_info(struct virgl_context *ctx, struct virgl_resource *res)
   {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_GET_MEMORY_INFO, 0, 1));
      }
      void virgl_encode_emit_string_marker(struct virgl_context *ctx,
         {
      /* len is guaranteed to be non-negative but be defensive */
   assert(len >= 0);
   if (len <= 0)
            if (len > 4 * 0xffff) {
      debug_printf("VIRGL: host debug flag string too long, will be truncated\n");
               uint32_t buf_len = (uint32_t )(len + 3) / 4 + 1;
   virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_EMIT_STRING_MARKER, 0, buf_len));
   virgl_encoder_write_dword(ctx->cbuf, len);
      }
      void virgl_encode_create_video_codec(struct virgl_context *ctx,
         {
      struct virgl_screen *rs = virgl_screen(ctx->base.screen);
            virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_VIDEO_CODEC, 0, len));
   virgl_encoder_write_dword(ctx->cbuf, cdc->handle);
   virgl_encoder_write_dword(ctx->cbuf, cdc->base.profile);
   virgl_encoder_write_dword(ctx->cbuf, cdc->base.entrypoint);
   virgl_encoder_write_dword(ctx->cbuf, cdc->base.chroma_format);
   virgl_encoder_write_dword(ctx->cbuf, cdc->base.level);
   virgl_encoder_write_dword(ctx->cbuf, cdc->base.width);
   virgl_encoder_write_dword(ctx->cbuf, cdc->base.height);
   if (rs->caps.caps.v2.host_feature_check_version >= 14)
      }
      void virgl_encode_destroy_video_codec(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_DESTROY_VIDEO_CODEC, 0, 1));
      }
      void virgl_encode_create_video_buffer(struct virgl_context *ctx,
         {
               virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_CREATE_VIDEO_BUFFER, 0,
         virgl_encoder_write_dword(ctx->cbuf, vbuf->handle);
   virgl_encoder_write_dword(ctx->cbuf, pipe_to_virgl_format(vbuf->buf->buffer_format));
   virgl_encoder_write_dword(ctx->cbuf, vbuf->buf->width);
   virgl_encoder_write_dword(ctx->cbuf, vbuf->buf->height);
   for (i = 0; i < vbuf->num_planes; i++)
      }
      void virgl_encode_destroy_video_buffer(struct virgl_context *ctx,
         {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_DESTROY_VIDEO_BUFFER, 0, 1));
      }
      void virgl_encode_begin_frame(struct virgl_context *ctx,
               {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_BEGIN_FRAME, 0, 2));
   virgl_encoder_write_dword(ctx->cbuf, cdc->handle);
      }
      void virgl_encode_decode_bitstream(struct virgl_context *ctx,
                     {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_DECODE_BITSTREAM, 0, 5));
   virgl_encoder_write_dword(ctx->cbuf, cdc->handle);
   virgl_encoder_write_dword(ctx->cbuf, buf->handle);
   virgl_encoder_write_res(ctx, virgl_resource(cdc->desc_buffers[cdc->cur_buffer]));
   virgl_encoder_write_res(ctx, virgl_resource(cdc->bs_buffers[cdc->cur_buffer]));
      }
      void virgl_encode_encode_bitstream(struct virgl_context *ctx,
                     {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_ENCODE_BITSTREAM, 0, 5));
   virgl_encoder_write_dword(ctx->cbuf, cdc->handle);
   virgl_encoder_write_dword(ctx->cbuf, buf->handle);
   virgl_encoder_write_res(ctx, tgt);
   virgl_encoder_write_res(ctx, virgl_resource(cdc->desc_buffers[cdc->cur_buffer]));
      }
      void virgl_encode_end_frame(struct virgl_context *ctx,
               {
      virgl_encoder_write_cmd_dword(ctx, VIRGL_CMD0(VIRGL_CCMD_END_FRAME, 0, 2));
   virgl_encoder_write_dword(ctx->cbuf, cdc->handle);
      }
