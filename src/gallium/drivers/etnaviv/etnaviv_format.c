   /*
   * Copyright (c) 2016 Etnaviv Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Christian Gmeiner <christian.gmeiner@gmail.com>
   */
      #include "etnaviv_format.h"
      #include "hw/common_3d.xml.h"
   #include "hw/state.xml.h"
   #include "hw/state_3d.xml.h"
      #include "pipe/p_defines.h"
   #include "util/compiler.h"
      /* Specifies the table of all the formats and their features. Also supplies
   * the helpers that look up various data in those tables.
   */
      struct etna_format {
      unsigned vtx;
   unsigned tex;
   unsigned pe;
      };
      #define PE_FORMAT_NONE ~0
      #define PE_FORMAT_MASK        0x7f
   #define PE_FORMAT(x)          ((x) & PE_FORMAT_MASK)
   #define PE_FORMAT_RB_SWAP     0x80
      #define PE_FORMAT_X8B8G8R8    (PE_FORMAT_X8R8G8B8 | PE_FORMAT_RB_SWAP)
   #define PE_FORMAT_A8B8G8R8    (PE_FORMAT_A8R8G8B8 | PE_FORMAT_RB_SWAP)
      #define TS_SAMPLER_FORMAT_NONE      ETNA_NO_MATCH
      /* vertex + texture */
   #define VT(pipe, vtxfmt, texfmt, rsfmt)          \
      [PIPE_FORMAT_##pipe] = {                               \
      .vtx = FE_DATA_TYPE_##vtxfmt, \
   .tex = TEXTURE_FORMAT_##texfmt,                     \
   .pe = PE_FORMAT_##rsfmt,                            \
            /* texture-only */
   #define _T(pipe, fmt, rsfmt) \
      [PIPE_FORMAT_##pipe] = {        \
      .vtx = ETNA_NO_MATCH,        \
   .tex = TEXTURE_FORMAT_##fmt, \
   .pe = PE_FORMAT_##rsfmt,     \
            /* vertex-only */
   #define V_(pipe, fmt, rsfmt)                           \
      [PIPE_FORMAT_##pipe] = {                            \
      .vtx = FE_DATA_TYPE_##fmt, \
   .tex = ETNA_NO_MATCH,                            \
   .pe = PE_FORMAT_##rsfmt,                         \
            static struct etna_format formats[PIPE_FORMAT_COUNT] = {
      /* 8-bit */
   VT(R8_UNORM,   UNSIGNED_BYTE, L8,                        R8),
   VT(R8_SNORM,   BYTE,          EXT_R8_SNORM | EXT_FORMAT, NONE),
   VT(R8_UINT,    BYTE_I,        EXT_R8I | EXT_FORMAT,      R8I),
   VT(R8_SINT,    BYTE_I,        EXT_R8I | EXT_FORMAT,      R8I),
   V_(R8_USCALED, UNSIGNED_BYTE, NONE),
            _T(A8_UNORM, A8, NONE),
   _T(L8_UNORM, L8, NONE),
            /* 16-bit */
   V_(R16_UNORM,   UNSIGNED_SHORT, NONE),
   V_(R16_SNORM,   SHORT,          NONE),
   VT(R16_UINT,    SHORT_I,        EXT_R16I | EXT_FORMAT, R16I),
   VT(R16_SINT,    SHORT_I,        EXT_R16I | EXT_FORMAT, R16I),
   V_(R16_USCALED, UNSIGNED_SHORT, NONE),
   V_(R16_SSCALED, SHORT,          NONE),
            _T(B4G4R4A4_UNORM, A4R4G4B4, A4R4G4B4),
                     _T(Z16_UNORM,      D16,      NONE),
   _T(B5G6R5_UNORM,   R5G6B5,   R5G6B5),
   _T(B5G5R5A1_UNORM, A1R5G5B5, A1R5G5B5),
            VT(R8G8_UNORM,   UNSIGNED_BYTE,  EXT_G8R8 | EXT_FORMAT,       G8R8),
   VT(R8G8_SNORM,   BYTE,           EXT_G8R8_SNORM | EXT_FORMAT, NONE),
   VT(R8G8_UINT,    BYTE_I,         EXT_G8R8I | EXT_FORMAT,      G8R8I),
   VT(R8G8_SINT,    BYTE_I,         EXT_G8R8I | EXT_FORMAT,      G8R8I),
   V_(R8G8_USCALED, UNSIGNED_BYTE,  NONE),
            /* 24-bit */
   V_(R8G8B8_UNORM,   UNSIGNED_BYTE, NONE),
   V_(R8G8B8_SNORM,   BYTE,          NONE),
   V_(R8G8B8_UINT,    BYTE_I,        NONE),
   V_(R8G8B8_SINT,    BYTE_I,        NONE),
   V_(R8G8B8_USCALED, UNSIGNED_BYTE, NONE),
            /* 32-bit */
   V_(R32_UNORM,   UNSIGNED_INT, NONE),
   V_(R32_SNORM,   INT,          NONE),
   VT(R32_SINT,    FLOAT,        EXT_R32F | EXT_FORMAT, R32F),
   VT(R32_UINT,    FLOAT,        EXT_R32F | EXT_FORMAT, R32F),
   V_(R32_USCALED, UNSIGNED_INT, NONE),
   V_(R32_SSCALED, INT,          NONE),
   VT(R32_FLOAT,   FLOAT,        EXT_R32F | EXT_FORMAT, R32F),
            V_(R16G16_UNORM,   UNSIGNED_SHORT, NONE),
   V_(R16G16_SNORM,   SHORT,          NONE),
   VT(R16G16_UINT,    SHORT_I,        EXT_G16R16I | EXT_FORMAT, G16R16I),
   VT(R16G16_SINT,    SHORT_I,        EXT_G16R16I | EXT_FORMAT, G16R16I),
   V_(R16G16_USCALED, UNSIGNED_SHORT, NONE),
   V_(R16G16_SSCALED, SHORT,          NONE),
                     VT(R8G8B8A8_UNORM,   UNSIGNED_BYTE, A8B8G8R8, A8B8G8R8),
   VT(R8G8B8A8_SNORM,   BYTE,          EXT_A8B8G8R8_SNORM | EXT_FORMAT, NONE),
   _T(R8G8B8X8_UNORM,   X8B8G8R8,      X8B8G8R8),
   _T(R8G8B8X8_SNORM,                  EXT_X8B8G8R8_SNORM | EXT_FORMAT, NONE),
   VT(R8G8B8A8_UINT,    BYTE_I,        EXT_A8B8G8R8I | EXT_FORMAT,      A8B8G8R8I),
   VT(R8G8B8A8_SINT,    BYTE_I,        EXT_A8B8G8R8I | EXT_FORMAT,      A8B8G8R8I),
   V_(R8G8B8A8_USCALED, UNSIGNED_BYTE, A8B8G8R8),
            _T(B8G8R8A8_UNORM, A8R8G8B8, A8R8G8B8),
            VT(R10G10B10A2_UNORM,   UNSIGNED_INT_2_10_10_10_REV, EXT_A2B10G10R10 | EXT_FORMAT, A2B10G10R10),
   _T(R10G10B10X2_UNORM,                                EXT_A2B10G10R10 | EXT_FORMAT, A2B10G10R10),
   V_(R10G10B10A2_SNORM,   INT_2_10_10_10_REV,          NONE),
   _T(R10G10B10A2_UINT,                                 EXT_A2B10G10R10UI | EXT_FORMAT, A2B10G10R10UI),
   V_(R10G10B10A2_USCALED, UNSIGNED_INT_2_10_10_10_REV, NONE),
            _T(X8Z24_UNORM,       D24X8, NONE),
            _T(R9G9B9E5_FLOAT,  E5B9G9R9,                    NONE),
            /* 48-bit */
   V_(R16G16B16_UNORM,   UNSIGNED_SHORT, NONE),
   V_(R16G16B16_SNORM,   SHORT,          NONE),
   V_(R16G16B16_UINT,    SHORT_I,        NONE),
   V_(R16G16B16_SINT,    SHORT_I,        NONE),
   V_(R16G16B16_USCALED, UNSIGNED_SHORT, NONE),
   V_(R16G16B16_SSCALED, SHORT,          NONE),
            /* 64-bit */
   V_(R16G16B16A16_UNORM,   UNSIGNED_SHORT, NONE),
   V_(R16G16B16A16_SNORM,   SHORT,          NONE),
   VT(R16G16B16A16_UINT,    SHORT_I,        EXT_A16B16G16R16I | EXT_FORMAT, A16B16G16R16I),
   VT(R16G16B16A16_SINT,    SHORT_I,        EXT_A16B16G16R16I | EXT_FORMAT, A16B16G16R16I),
   V_(R16G16B16A16_USCALED, UNSIGNED_SHORT, NONE),
   V_(R16G16B16A16_SSCALED, SHORT,          NONE),
            V_(R32G32_UNORM,   UNSIGNED_INT, NONE),
   V_(R32G32_SNORM,   INT,          NONE),
   VT(R32G32_UINT,    FLOAT,        EXT_G32R32F | EXT_FORMAT, G32R32F),
   VT(R32G32_SINT,    FLOAT,        EXT_G32R32F | EXT_FORMAT, G32R32F),
   V_(R32G32_USCALED, UNSIGNED_INT, NONE),
   V_(R32G32_SSCALED, INT,          NONE),
   VT(R32G32_FLOAT,   FLOAT,        EXT_G32R32F | EXT_FORMAT, G32R32F),
            /* 96-bit */
   V_(R32G32B32_UNORM,   UNSIGNED_INT, NONE),
   V_(R32G32B32_SNORM,   INT,          NONE),
   V_(R32G32B32_UINT,    FLOAT,        NONE),
   V_(R32G32B32_SINT,    FLOAT,        NONE),
   V_(R32G32B32_USCALED, UNSIGNED_INT, NONE),
   V_(R32G32B32_SSCALED, INT,          NONE),
   V_(R32G32B32_FLOAT,   FLOAT,        NONE),
            /* 128-bit */
   V_(R32G32B32A32_UNORM,   UNSIGNED_INT, NONE),
   V_(R32G32B32A32_SNORM,   INT,          NONE),
   V_(R32G32B32A32_UINT,    FLOAT,        NONE),
   V_(R32G32B32A32_SINT,    FLOAT,        NONE),
   V_(R32G32B32A32_USCALED, UNSIGNED_INT, NONE),
   V_(R32G32B32A32_SSCALED, INT,          NONE),
   V_(R32G32B32A32_FLOAT,   FLOAT,        NONE),
            /* compressed */
            _T(DXT1_RGB,  DXT1,      NONE),
   _T(DXT1_RGBA, DXT1,      NONE),
   _T(DXT3_RGBA, DXT2_DXT3, NONE),
            _T(ETC2_RGB8,       EXT_NONE | EXT_FORMAT,                          NONE), /* Extd. format NONE doubles as ETC2_RGB8 */
   _T(ETC2_RGB8A1,     EXT_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 | EXT_FORMAT, NONE),
   _T(ETC2_RGBA8,      EXT_RGBA8_ETC2_EAC | EXT_FORMAT,                NONE),
   _T(ETC2_R11_UNORM,  EXT_R11_EAC | EXT_FORMAT,                       NONE),
   _T(ETC2_R11_SNORM,  EXT_SIGNED_R11_EAC | EXT_FORMAT,                NONE),
   _T(ETC2_RG11_UNORM, EXT_RG11_EAC | EXT_FORMAT,                      NONE),
            _T(ASTC_4x4,        ASTC_RGBA_4x4 | ASTC_FORMAT,                    NONE),
   _T(ASTC_5x4,        ASTC_RGBA_5x4 | ASTC_FORMAT,                    NONE),
   _T(ASTC_5x5,        ASTC_RGBA_5x5 | ASTC_FORMAT,                    NONE),
   _T(ASTC_6x5,        ASTC_RGBA_6x5 | ASTC_FORMAT,                    NONE),
   _T(ASTC_6x6,        ASTC_RGBA_6x6 | ASTC_FORMAT,                    NONE),
   _T(ASTC_8x5,        ASTC_RGBA_8x5 | ASTC_FORMAT,                    NONE),
   _T(ASTC_8x6,        ASTC_RGBA_8x6 | ASTC_FORMAT,                    NONE),
   _T(ASTC_8x8,        ASTC_RGBA_8x8 | ASTC_FORMAT,                    NONE),
   _T(ASTC_10x5,       ASTC_RGBA_10x5 | ASTC_FORMAT,                   NONE),
   _T(ASTC_10x6,       ASTC_RGBA_10x6 | ASTC_FORMAT,                   NONE),
   _T(ASTC_10x8,       ASTC_RGBA_10x8 | ASTC_FORMAT,                   NONE),
   _T(ASTC_10x10,      ASTC_RGBA_10x10 | ASTC_FORMAT,                  NONE),
   _T(ASTC_12x10,      ASTC_RGBA_12x10 | ASTC_FORMAT,                  NONE),
            /* YUV */
   _T(YUYV, YUY2, YUY2),
      };
      uint32_t
   translate_texture_format(enum pipe_format fmt)
   {
               if (!formats[fmt].present)
               }
      bool
   texture_use_int_filter(const struct pipe_sampler_view *sv,
               {
      switch (sv->target) {
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
      if (tex_desc)
            case PIPE_TEXTURE_3D:
         default:
                  /* only unorm formats can use int filter */
   if (!util_format_is_unorm(sv->format))
            if (util_format_is_srgb(sv->format))
            if (util_format_description(sv->format)->layout == UTIL_FORMAT_LAYOUT_ASTC)
            if (ss->max_anisotropy > 1)
            switch (sv->format) {
   /* apparently D16 can't use int filter but D24 can */
   case PIPE_FORMAT_Z16_UNORM:
   case PIPE_FORMAT_R10G10B10A2_UNORM:
   case PIPE_FORMAT_R10G10B10X2_UNORM:
   case PIPE_FORMAT_ETC2_R11_UNORM:
   case PIPE_FORMAT_ETC2_RG11_UNORM:
         default:
            }
      bool
   texture_format_needs_swiz(enum pipe_format fmt)
   {
         }
      uint32_t
   get_texture_swiz(enum pipe_format fmt, unsigned swizzle_r,
         {
      unsigned char swiz[4] = {
                  if (unlikely(fmt == PIPE_FORMAT_DXT1_RGB)) {
      /* The HW uses the same decompression scheme for RGB and RGBA DXT1
   * textures, tell it to 1-fill the alpha channel for plain RGB.
   */
   for (unsigned i = 0; i < 4; i++) {
      if (swiz[i] == PIPE_SWIZZLE_W)
                  if (util_format_linear(fmt) == PIPE_FORMAT_R8_UNORM) {
      /* R8 is emulated with L8, needs yz channels set to zero */
   for (unsigned i = 0; i < 4; i++) {
      if (swiz[i] == PIPE_SWIZZLE_Y || swiz[i] == PIPE_SWIZZLE_Z)
                  /* PIPE_SWIZZLE_ maps 1:1 to TEXTURE_SWIZZLE_ */
   STATIC_ASSERT(PIPE_SWIZZLE_X == TEXTURE_SWIZZLE_RED);
   STATIC_ASSERT(PIPE_SWIZZLE_Y == TEXTURE_SWIZZLE_GREEN);
   STATIC_ASSERT(PIPE_SWIZZLE_Z == TEXTURE_SWIZZLE_BLUE);
   STATIC_ASSERT(PIPE_SWIZZLE_W == TEXTURE_SWIZZLE_ALPHA);
   STATIC_ASSERT(PIPE_SWIZZLE_0 == TEXTURE_SWIZZLE_ZERO);
            return VIVS_TE_SAMPLER_CONFIG1_SWIZZLE_R(swiz[0]) |
         VIVS_TE_SAMPLER_CONFIG1_SWIZZLE_G(swiz[1]) |
      }
      uint32_t
   translate_pe_format(enum pipe_format fmt)
   {
               if (!formats[fmt].present)
            if (formats[fmt].pe == ETNA_NO_MATCH)
               }
      int
   translate_pe_format_rb_swap(enum pipe_format fmt)
   {
      fmt = util_format_linear(fmt);
               }
      /* Return type flags for vertex element format */
   uint32_t
   translate_vertex_format_type(enum pipe_format fmt)
   {
      if (!formats[fmt].present)
               }
