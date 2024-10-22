   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
         /**
   * \file mipmap.c  mipmap generation and teximage resizing functions.
   */
      #include "errors.h"
      #include "formats.h"
   #include "glformats.h"
   #include "mipmap.h"
   #include "mtypes.h"
   #include "teximage.h"
   #include "texobj.h"
   #include "texstore.h"
   #include "image.h"
   #include "macros.h"
   #include "util/half_float.h"
   #include "util/format_rgb9e5.h"
   #include "util/format_r11g11b10f.h"
      #include "state_tracker/st_cb_texture.h"
      /**
   * Compute the expected number of mipmap levels in the texture given
   * the width/height/depth of the base image and the GL_TEXTURE_BASE_LEVEL/
   * GL_TEXTURE_MAX_LEVEL settings.  This will tell us how many mipmap
   * levels should be generated.
   */
   unsigned
   _mesa_compute_num_levels(struct gl_context *ctx,
               {
      const struct gl_texture_image *baseImage;
                     numLevels = texObj->Attrib.BaseLevel + baseImage->MaxNumLevels;
   numLevels = MIN2(numLevels, (GLuint) texObj->Attrib.MaxLevel + 1);
   if (texObj->Immutable)
                     }
      static GLint
   bytes_per_pixel(GLenum datatype, GLuint comps)
   {
               if (datatype == GL_UNSIGNED_INT_8_24_REV_MESA ||
      datatype == GL_UNSIGNED_INT_24_8_MESA)
         b = _mesa_sizeof_packed_type(datatype);
            if (_mesa_type_is_packed(datatype))
         else
      }
         /**
   * \name Support macros for do_row and do_row_3d
   *
   * The macro madness is here for two reasons.  First, it compacts the code
   * slightly.  Second, it makes it much easier to adjust the specifics of the
   * filter to tune the rounding characteristics.
   */
   /*@{*/
   #define DECLARE_ROW_POINTERS(t, e) \
         const t(*rowA)[e] = (const t(*)[e]) srcRowA; \
   const t(*rowB)[e] = (const t(*)[e]) srcRowB; \
   const t(*rowC)[e] = (const t(*)[e]) srcRowC; \
   const t(*rowD)[e] = (const t(*)[e]) srcRowD; \
      #define DECLARE_ROW_POINTERS0(t) \
         const t *rowA = (const t *) srcRowA; \
   const t *rowB = (const t *) srcRowB; \
   const t *rowC = (const t *) srcRowC; \
   const t *rowD = (const t *) srcRowD; \
      #define FILTER_SUM_3D(Aj, Ak, Bj, Bk, Cj, Ck, Dj, Dk) \
      ((unsigned) Aj + (unsigned) Ak \
   + (unsigned) Bj + (unsigned) Bk \
   + (unsigned) Cj + (unsigned) Ck \
   + (unsigned) Dj + (unsigned) Dk \
         #define FILTER_3D(e) \
      do { \
      dst[i][e] = FILTER_SUM_3D(rowA[j][e], rowA[k][e], \
                        #define FILTER_SUM_3D_SIGNED(Aj, Ak, Bj, Bk, Cj, Ck, Dj, Dk) \
      (Aj + Ak \
   + Bj + Bk \
   + Cj + Ck \
   + Dj + Dk \
         #define FILTER_3D_SIGNED(e) \
      do { \
      dst[i][e] = FILTER_SUM_3D_SIGNED(rowA[j][e], rowA[k][e], \
                        #define FILTER_F_3D(e) \
      do { \
      dst[i][e] = (rowA[j][e] + rowA[k][e] \
                        #define FILTER_HF_3D(e) \
      do { \
      const GLfloat aj = _mesa_half_to_float(rowA[j][e]); \
   const GLfloat ak = _mesa_half_to_float(rowA[k][e]); \
   const GLfloat bj = _mesa_half_to_float(rowB[j][e]); \
   const GLfloat bk = _mesa_half_to_float(rowB[k][e]); \
   const GLfloat cj = _mesa_half_to_float(rowC[j][e]); \
   const GLfloat ck = _mesa_half_to_float(rowC[k][e]); \
   const GLfloat dj = _mesa_half_to_float(rowD[j][e]); \
   const GLfloat dk = _mesa_half_to_float(rowD[k][e]); \
   dst[i][e] = _mesa_float_to_half((aj + ak + bj + bk + cj + ck + dj + dk) \
         /*@}*/
         /**
   * Average together two rows of a source image to produce a single new
   * row in the dest image.  It's legal for the two source rows to point
   * to the same data.  The source width must be equal to either the
   * dest width or two times the dest width.
   * \param datatype  GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_FLOAT, etc.
   * \param comps  number of components per pixel (1..4)
   */
   static void
   do_row(GLenum datatype, GLuint comps, GLint srcWidth,
         const GLvoid *srcRowA, const GLvoid *srcRowB,
   {
      const GLuint k0 = (srcWidth == dstWidth) ? 0 : 1;
            assert(comps >= 1);
            /* This assertion is no longer valid with non-power-of-2 textures
   assert(srcWidth == dstWidth || srcWidth == 2 * dstWidth);
            if (datatype == GL_UNSIGNED_BYTE && comps == 4) {
      GLuint i, j, k;
   const GLubyte(*rowA)[4] = (const GLubyte(*)[4]) srcRowA;
   const GLubyte(*rowB)[4] = (const GLubyte(*)[4]) srcRowB;
   GLubyte(*dst)[4] = (GLubyte(*)[4]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
   dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
   dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         }
   else if (datatype == GL_UNSIGNED_BYTE && comps == 3) {
      GLuint i, j, k;
   const GLubyte(*rowA)[3] = (const GLubyte(*)[3]) srcRowA;
   const GLubyte(*rowB)[3] = (const GLubyte(*)[3]) srcRowB;
   GLubyte(*dst)[3] = (GLubyte(*)[3]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
   dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         }
   else if (datatype == GL_UNSIGNED_BYTE && comps == 2) {
      GLuint i, j, k;
   const GLubyte(*rowA)[2] = (const GLubyte(*)[2]) srcRowA;
   const GLubyte(*rowB)[2] = (const GLubyte(*)[2]) srcRowB;
   GLubyte(*dst)[2] = (GLubyte(*)[2]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) >> 2;
         }
   else if (datatype == GL_UNSIGNED_BYTE && comps == 1) {
      GLuint i, j, k;
   const GLubyte *rowA = (const GLubyte *) srcRowA;
   const GLubyte *rowB = (const GLubyte *) srcRowB;
   GLubyte *dst = (GLubyte *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
                  else if (datatype == GL_BYTE && comps == 4) {
      GLuint i, j, k;
   const GLbyte(*rowA)[4] = (const GLbyte(*)[4]) srcRowA;
   const GLbyte(*rowB)[4] = (const GLbyte(*)[4]) srcRowB;
   GLbyte(*dst)[4] = (GLbyte(*)[4]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
   dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
   dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         }
   else if (datatype == GL_BYTE && comps == 3) {
      GLuint i, j, k;
   const GLbyte(*rowA)[3] = (const GLbyte(*)[3]) srcRowA;
   const GLbyte(*rowB)[3] = (const GLbyte(*)[3]) srcRowB;
   GLbyte(*dst)[3] = (GLbyte(*)[3]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
   dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         }
   else if (datatype == GL_BYTE && comps == 2) {
      GLuint i, j, k;
   const GLbyte(*rowA)[2] = (const GLbyte(*)[2]) srcRowA;
   const GLbyte(*rowB)[2] = (const GLbyte(*)[2]) srcRowB;
   GLbyte(*dst)[2] = (GLbyte(*)[2]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         }
   else if (datatype == GL_BYTE && comps == 1) {
      GLuint i, j, k;
   const GLbyte *rowA = (const GLbyte *) srcRowA;
   const GLbyte *rowB = (const GLbyte *) srcRowB;
   GLbyte *dst = (GLbyte *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
                  else if (datatype == GL_UNSIGNED_SHORT && comps == 4) {
      GLuint i, j, k;
   const GLushort(*rowA)[4] = (const GLushort(*)[4]) srcRowA;
   const GLushort(*rowB)[4] = (const GLushort(*)[4]) srcRowB;
   GLushort(*dst)[4] = (GLushort(*)[4]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
   dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
   dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         }
   else if (datatype == GL_UNSIGNED_SHORT && comps == 3) {
      GLuint i, j, k;
   const GLushort(*rowA)[3] = (const GLushort(*)[3]) srcRowA;
   const GLushort(*rowB)[3] = (const GLushort(*)[3]) srcRowB;
   GLushort(*dst)[3] = (GLushort(*)[3]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
   dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         }
   else if (datatype == GL_UNSIGNED_SHORT && comps == 2) {
      GLuint i, j, k;
   const GLushort(*rowA)[2] = (const GLushort(*)[2]) srcRowA;
   const GLushort(*rowB)[2] = (const GLushort(*)[2]) srcRowB;
   GLushort(*dst)[2] = (GLushort(*)[2]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         }
   else if (datatype == GL_UNSIGNED_SHORT && comps == 1) {
      GLuint i, j, k;
   const GLushort *rowA = (const GLushort *) srcRowA;
   const GLushort *rowB = (const GLushort *) srcRowB;
   GLushort *dst = (GLushort *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
                  else if (datatype == GL_SHORT && comps == 4) {
      GLuint i, j, k;
   const GLshort(*rowA)[4] = (const GLshort(*)[4]) srcRowA;
   const GLshort(*rowB)[4] = (const GLshort(*)[4]) srcRowB;
   GLshort(*dst)[4] = (GLshort(*)[4]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
   dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
   dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         }
   else if (datatype == GL_SHORT && comps == 3) {
      GLuint i, j, k;
   const GLshort(*rowA)[3] = (const GLshort(*)[3]) srcRowA;
   const GLshort(*rowB)[3] = (const GLshort(*)[3]) srcRowB;
   GLshort(*dst)[3] = (GLshort(*)[3]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
   dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         }
   else if (datatype == GL_SHORT && comps == 2) {
      GLuint i, j, k;
   const GLshort(*rowA)[2] = (const GLshort(*)[2]) srcRowA;
   const GLshort(*rowB)[2] = (const GLshort(*)[2]) srcRowB;
   GLshort(*dst)[2] = (GLshort(*)[2]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         }
   else if (datatype == GL_SHORT && comps == 1) {
      GLuint i, j, k;
   const GLshort *rowA = (const GLshort *) srcRowA;
   const GLshort *rowB = (const GLshort *) srcRowB;
   GLshort *dst = (GLshort *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
                  else if (datatype == GL_FLOAT && comps == 4) {
      GLuint i, j, k;
   const GLfloat(*rowA)[4] = (const GLfloat(*)[4]) srcRowA;
   const GLfloat(*rowB)[4] = (const GLfloat(*)[4]) srcRowB;
   GLfloat(*dst)[4] = (GLfloat(*)[4]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] +
         dst[i][1] = (rowA[j][1] + rowA[k][1] +
         dst[i][2] = (rowA[j][2] + rowA[k][2] +
         dst[i][3] = (rowA[j][3] + rowA[k][3] +
         }
   else if (datatype == GL_FLOAT && comps == 3) {
      GLuint i, j, k;
   const GLfloat(*rowA)[3] = (const GLfloat(*)[3]) srcRowA;
   const GLfloat(*rowB)[3] = (const GLfloat(*)[3]) srcRowB;
   GLfloat(*dst)[3] = (GLfloat(*)[3]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] +
         dst[i][1] = (rowA[j][1] + rowA[k][1] +
         dst[i][2] = (rowA[j][2] + rowA[k][2] +
         }
   else if (datatype == GL_FLOAT && comps == 2) {
      GLuint i, j, k;
   const GLfloat(*rowA)[2] = (const GLfloat(*)[2]) srcRowA;
   const GLfloat(*rowB)[2] = (const GLfloat(*)[2]) srcRowB;
   GLfloat(*dst)[2] = (GLfloat(*)[2]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   dst[i][0] = (rowA[j][0] + rowA[k][0] +
         dst[i][1] = (rowA[j][1] + rowA[k][1] +
         }
   else if (datatype == GL_FLOAT && comps == 1) {
      GLuint i, j, k;
   const GLfloat *rowA = (const GLfloat *) srcRowA;
   const GLfloat *rowB = (const GLfloat *) srcRowB;
   GLfloat *dst = (GLfloat *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
                  else if (datatype == GL_HALF_FLOAT_ARB && comps == 4) {
      GLuint i, j, k, comp;
   const GLhalfARB(*rowA)[4] = (const GLhalfARB(*)[4]) srcRowA;
   const GLhalfARB(*rowB)[4] = (const GLhalfARB(*)[4]) srcRowB;
   GLhalfARB(*dst)[4] = (GLhalfARB(*)[4]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   for (comp = 0; comp < 4; comp++) {
      GLfloat aj, ak, bj, bk;
   aj = _mesa_half_to_float(rowA[j][comp]);
   ak = _mesa_half_to_float(rowA[k][comp]);
   bj = _mesa_half_to_float(rowB[j][comp]);
   bk = _mesa_half_to_float(rowB[k][comp]);
            }
   else if (datatype == GL_HALF_FLOAT_ARB && comps == 3) {
      GLuint i, j, k, comp;
   const GLhalfARB(*rowA)[3] = (const GLhalfARB(*)[3]) srcRowA;
   const GLhalfARB(*rowB)[3] = (const GLhalfARB(*)[3]) srcRowB;
   GLhalfARB(*dst)[3] = (GLhalfARB(*)[3]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   for (comp = 0; comp < 3; comp++) {
      GLfloat aj, ak, bj, bk;
   aj = _mesa_half_to_float(rowA[j][comp]);
   ak = _mesa_half_to_float(rowA[k][comp]);
   bj = _mesa_half_to_float(rowB[j][comp]);
   bk = _mesa_half_to_float(rowB[k][comp]);
            }
   else if (datatype == GL_HALF_FLOAT_ARB && comps == 2) {
      GLuint i, j, k, comp;
   const GLhalfARB(*rowA)[2] = (const GLhalfARB(*)[2]) srcRowA;
   const GLhalfARB(*rowB)[2] = (const GLhalfARB(*)[2]) srcRowB;
   GLhalfARB(*dst)[2] = (GLhalfARB(*)[2]) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   for (comp = 0; comp < 2; comp++) {
      GLfloat aj, ak, bj, bk;
   aj = _mesa_half_to_float(rowA[j][comp]);
   ak = _mesa_half_to_float(rowA[k][comp]);
   bj = _mesa_half_to_float(rowB[j][comp]);
   bk = _mesa_half_to_float(rowB[k][comp]);
            }
   else if (datatype == GL_HALF_FLOAT_ARB && comps == 1) {
      GLuint i, j, k;
   const GLhalfARB *rowA = (const GLhalfARB *) srcRowA;
   const GLhalfARB *rowB = (const GLhalfARB *) srcRowB;
   GLhalfARB *dst = (GLhalfARB *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   GLfloat aj, ak, bj, bk;
   aj = _mesa_half_to_float(rowA[j]);
   ak = _mesa_half_to_float(rowA[k]);
   bj = _mesa_half_to_float(rowB[j]);
   bk = _mesa_half_to_float(rowB[k]);
                  else if (datatype == GL_UNSIGNED_INT && comps == 1) {
      GLuint i, j, k;
   const GLuint *rowA = (const GLuint *) srcRowA;
   const GLuint *rowB = (const GLuint *) srcRowB;
   GLuint *dst = (GLuint *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
                  else if (datatype == GL_UNSIGNED_SHORT_5_6_5 && comps == 3) {
      GLuint i, j, k;
   const GLushort *rowA = (const GLushort *) srcRowA;
   const GLushort *rowB = (const GLushort *) srcRowB;
   GLushort *dst = (GLushort *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0x1f;
   const GLint rowAr1 = rowA[k] & 0x1f;
   const GLint rowBr0 = rowB[j] & 0x1f;
   const GLint rowBr1 = rowB[k] & 0x1f;
   const GLint rowAg0 = (rowA[j] >> 5) & 0x3f;
   const GLint rowAg1 = (rowA[k] >> 5) & 0x3f;
   const GLint rowBg0 = (rowB[j] >> 5) & 0x3f;
   const GLint rowBg1 = (rowB[k] >> 5) & 0x3f;
   const GLint rowAb0 = (rowA[j] >> 11) & 0x1f;
   const GLint rowAb1 = (rowA[k] >> 11) & 0x1f;
   const GLint rowBb0 = (rowB[j] >> 11) & 0x1f;
   const GLint rowBb1 = (rowB[k] >> 11) & 0x1f;
   const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
   const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
   const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         }
   else if (datatype == GL_UNSIGNED_SHORT_4_4_4_4 && comps == 4) {
      GLuint i, j, k;
   const GLushort *rowA = (const GLushort *) srcRowA;
   const GLushort *rowB = (const GLushort *) srcRowB;
   GLushort *dst = (GLushort *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0xf;
   const GLint rowAr1 = rowA[k] & 0xf;
   const GLint rowBr0 = rowB[j] & 0xf;
   const GLint rowBr1 = rowB[k] & 0xf;
   const GLint rowAg0 = (rowA[j] >> 4) & 0xf;
   const GLint rowAg1 = (rowA[k] >> 4) & 0xf;
   const GLint rowBg0 = (rowB[j] >> 4) & 0xf;
   const GLint rowBg1 = (rowB[k] >> 4) & 0xf;
   const GLint rowAb0 = (rowA[j] >> 8) & 0xf;
   const GLint rowAb1 = (rowA[k] >> 8) & 0xf;
   const GLint rowBb0 = (rowB[j] >> 8) & 0xf;
   const GLint rowBb1 = (rowB[k] >> 8) & 0xf;
   const GLint rowAa0 = (rowA[j] >> 12) & 0xf;
   const GLint rowAa1 = (rowA[k] >> 12) & 0xf;
   const GLint rowBa0 = (rowB[j] >> 12) & 0xf;
   const GLint rowBa1 = (rowB[k] >> 12) & 0xf;
   const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
   const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
   const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
   const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
         }
   else if (datatype == GL_UNSIGNED_SHORT_1_5_5_5_REV && comps == 4) {
      GLuint i, j, k;
   const GLushort *rowA = (const GLushort *) srcRowA;
   const GLushort *rowB = (const GLushort *) srcRowB;
   GLushort *dst = (GLushort *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0x1f;
   const GLint rowAr1 = rowA[k] & 0x1f;
   const GLint rowBr0 = rowB[j] & 0x1f;
   const GLint rowBr1 = rowB[k] & 0x1f;
   const GLint rowAg0 = (rowA[j] >> 5) & 0x1f;
   const GLint rowAg1 = (rowA[k] >> 5) & 0x1f;
   const GLint rowBg0 = (rowB[j] >> 5) & 0x1f;
   const GLint rowBg1 = (rowB[k] >> 5) & 0x1f;
   const GLint rowAb0 = (rowA[j] >> 10) & 0x1f;
   const GLint rowAb1 = (rowA[k] >> 10) & 0x1f;
   const GLint rowBb0 = (rowB[j] >> 10) & 0x1f;
   const GLint rowBb1 = (rowB[k] >> 10) & 0x1f;
   const GLint rowAa0 = (rowA[j] >> 15) & 0x1;
   const GLint rowAa1 = (rowA[k] >> 15) & 0x1;
   const GLint rowBa0 = (rowB[j] >> 15) & 0x1;
   const GLint rowBa1 = (rowB[k] >> 15) & 0x1;
   const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
   const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
   const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
   const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
         }
   else if (datatype == GL_UNSIGNED_SHORT_5_5_5_1 && comps == 4) {
      GLuint i, j, k;
   const GLushort *rowA = (const GLushort *) srcRowA;
   const GLushort *rowB = (const GLushort *) srcRowB;
   GLushort *dst = (GLushort *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = (rowA[j] >> 11) & 0x1f;
   const GLint rowAr1 = (rowA[k] >> 11) & 0x1f;
   const GLint rowBr0 = (rowB[j] >> 11) & 0x1f;
   const GLint rowBr1 = (rowB[k] >> 11) & 0x1f;
   const GLint rowAg0 = (rowA[j] >> 6) & 0x1f;
   const GLint rowAg1 = (rowA[k] >> 6) & 0x1f;
   const GLint rowBg0 = (rowB[j] >> 6) & 0x1f;
   const GLint rowBg1 = (rowB[k] >> 6) & 0x1f;
   const GLint rowAb0 = (rowA[j] >> 1) & 0x1f;
   const GLint rowAb1 = (rowA[k] >> 1) & 0x1f;
   const GLint rowBb0 = (rowB[j] >> 1) & 0x1f;
   const GLint rowBb1 = (rowB[k] >> 1) & 0x1f;
   const GLint rowAa0 = (rowA[j] & 0x1);
   const GLint rowAa1 = (rowA[k] & 0x1);
   const GLint rowBa0 = (rowB[j] & 0x1);
   const GLint rowBa1 = (rowB[k] & 0x1);
   const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
   const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
   const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
   const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
                  else if (datatype == GL_UNSIGNED_BYTE_3_3_2 && comps == 3) {
      GLuint i, j, k;
   const GLubyte *rowA = (const GLubyte *) srcRowA;
   const GLubyte *rowB = (const GLubyte *) srcRowB;
   GLubyte *dst = (GLubyte *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0x3;
   const GLint rowAr1 = rowA[k] & 0x3;
   const GLint rowBr0 = rowB[j] & 0x3;
   const GLint rowBr1 = rowB[k] & 0x3;
   const GLint rowAg0 = (rowA[j] >> 2) & 0x7;
   const GLint rowAg1 = (rowA[k] >> 2) & 0x7;
   const GLint rowBg0 = (rowB[j] >> 2) & 0x7;
   const GLint rowBg1 = (rowB[k] >> 2) & 0x7;
   const GLint rowAb0 = (rowA[j] >> 5) & 0x7;
   const GLint rowAb1 = (rowA[k] >> 5) & 0x7;
   const GLint rowBb0 = (rowB[j] >> 5) & 0x7;
   const GLint rowBb1 = (rowB[k] >> 5) & 0x7;
   const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
   const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
   const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
                  else if (datatype == MESA_UNSIGNED_BYTE_4_4 && comps == 2) {
      GLuint i, j, k;
   const GLubyte *rowA = (const GLubyte *) srcRowA;
   const GLubyte *rowB = (const GLubyte *) srcRowB;
   GLubyte *dst = (GLubyte *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0xf;
   const GLint rowAr1 = rowA[k] & 0xf;
   const GLint rowBr0 = rowB[j] & 0xf;
   const GLint rowBr1 = rowB[k] & 0xf;
   const GLint rowAg0 = (rowA[j] >> 4) & 0xf;
   const GLint rowAg1 = (rowA[k] >> 4) & 0xf;
   const GLint rowBg0 = (rowB[j] >> 4) & 0xf;
   const GLint rowBg1 = (rowB[k] >> 4) & 0xf;
   const GLint r = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
   const GLint g = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
                  else if (datatype == GL_UNSIGNED_INT_2_10_10_10_REV && comps == 4) {
      GLuint i, j, k;
   const GLuint *rowA = (const GLuint *) srcRowA;
   const GLuint *rowB = (const GLuint *) srcRowB;
   GLuint *dst = (GLuint *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0x3ff;
   const GLint rowAr1 = rowA[k] & 0x3ff;
   const GLint rowBr0 = rowB[j] & 0x3ff;
   const GLint rowBr1 = rowB[k] & 0x3ff;
   const GLint rowAg0 = (rowA[j] >> 10) & 0x3ff;
   const GLint rowAg1 = (rowA[k] >> 10) & 0x3ff;
   const GLint rowBg0 = (rowB[j] >> 10) & 0x3ff;
   const GLint rowBg1 = (rowB[k] >> 10) & 0x3ff;
   const GLint rowAb0 = (rowA[j] >> 20) & 0x3ff;
   const GLint rowAb1 = (rowA[k] >> 20) & 0x3ff;
   const GLint rowBb0 = (rowB[j] >> 20) & 0x3ff;
   const GLint rowBb1 = (rowB[k] >> 20) & 0x3ff;
   const GLint rowAa0 = (rowA[j] >> 30) & 0x3;
   const GLint rowAa1 = (rowA[k] >> 30) & 0x3;
   const GLint rowBa0 = (rowB[j] >> 30) & 0x3;
   const GLint rowBa1 = (rowB[k] >> 30) & 0x3;
   const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
   const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
   const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
   const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
                  else if (datatype == GL_UNSIGNED_INT_5_9_9_9_REV && comps == 3) {
      GLuint i, j, k;
   const GLuint *rowA = (const GLuint*) srcRowA;
   const GLuint *rowB = (const GLuint*) srcRowB;
   GLuint *dst = (GLuint*)dstRow;
   GLfloat res[3], rowAj[3], rowBj[3], rowAk[3], rowBk[3];
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   rgb9e5_to_float3(rowA[j], rowAj);
   rgb9e5_to_float3(rowB[j], rowBj);
   rgb9e5_to_float3(rowA[k], rowAk);
   rgb9e5_to_float3(rowB[k], rowBk);
   res[0] = (rowAj[0] + rowAk[0] + rowBj[0] + rowBk[0]) * 0.25F;
   res[1] = (rowAj[1] + rowAk[1] + rowBj[1] + rowBk[1]) * 0.25F;
   res[2] = (rowAj[2] + rowAk[2] + rowBj[2] + rowBk[2]) * 0.25F;
                  else if (datatype == GL_UNSIGNED_INT_10F_11F_11F_REV && comps == 3) {
      GLuint i, j, k;
   const GLuint *rowA = (const GLuint*) srcRowA;
   const GLuint *rowB = (const GLuint*) srcRowB;
   GLuint *dst = (GLuint*)dstRow;
   GLfloat res[3], rowAj[3], rowBj[3], rowAk[3], rowBk[3];
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   r11g11b10f_to_float3(rowA[j], rowAj);
   r11g11b10f_to_float3(rowB[j], rowBj);
   r11g11b10f_to_float3(rowA[k], rowAk);
   r11g11b10f_to_float3(rowB[k], rowBk);
   res[0] = (rowAj[0] + rowAk[0] + rowBj[0] + rowBk[0]) * 0.25F;
   res[1] = (rowAj[1] + rowAk[1] + rowBj[1] + rowBk[1]) * 0.25F;
   res[2] = (rowAj[2] + rowAk[2] + rowBj[2] + rowBk[2]) * 0.25F;
                  else if (datatype == GL_FLOAT_32_UNSIGNED_INT_24_8_REV && comps == 1) {
      GLuint i, j, k;
   const GLfloat *rowA = (const GLfloat *) srcRowA;
   const GLfloat *rowB = (const GLfloat *) srcRowB;
   GLfloat *dst = (GLfloat *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
                  else if (datatype == GL_UNSIGNED_INT_24_8_MESA && comps == 2) {
      GLuint i, j, k;
   const GLuint *rowA = (const GLuint *) srcRowA;
   const GLuint *rowB = (const GLuint *) srcRowB;
   GLuint *dst = (GLuint *) dstRow;
   /* note: averaging stencil values seems weird, but what else? */
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   GLuint z = (((rowA[j] >> 8) + (rowA[k] >> 8) +
         GLuint s = ((rowA[j] & 0xff) + (rowA[k] & 0xff) +
               }
   else if (datatype == GL_UNSIGNED_INT_8_24_REV_MESA && comps == 2) {
      GLuint i, j, k;
   const GLuint *rowA = (const GLuint *) srcRowA;
   const GLuint *rowB = (const GLuint *) srcRowB;
   GLuint *dst = (GLuint *) dstRow;
   for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   GLuint z = ((rowA[j] & 0xffffff) + (rowA[k] & 0xffffff) +
         GLuint s = (((rowA[j] >> 24) + (rowA[k] >> 24) +
                        else {
            }
         /**
   * Average together four rows of a source image to produce a single new
   * row in the dest image.  It's legal for the two source rows to point
   * to the same data.  The source width must be equal to either the
   * dest width or two times the dest width.
   *
   * \param datatype  GL pixel type \c GL_UNSIGNED_BYTE, \c GL_UNSIGNED_SHORT,
   *                  \c GL_FLOAT, etc.
   * \param comps     number of components per pixel (1..4)
   * \param srcWidth  Width of a row in the source data
   * \param srcRowA   Pointer to one of the rows of source data
   * \param srcRowB   Pointer to one of the rows of source data
   * \param srcRowC   Pointer to one of the rows of source data
   * \param srcRowD   Pointer to one of the rows of source data
   * \param dstWidth  Width of a row in the destination data
   * \param srcRowA   Pointer to the row of destination data
   */
   static void
   do_row_3D(GLenum datatype, GLuint comps, GLint srcWidth,
            const GLvoid *srcRowA, const GLvoid *srcRowB,
      {
      const GLuint k0 = (srcWidth == dstWidth) ? 0 : 1;
   const GLuint colStride = (srcWidth == dstWidth) ? 1 : 2;
            assert(comps >= 1);
            if ((datatype == GL_UNSIGNED_BYTE) && (comps == 4)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D(0);
   FILTER_3D(1);
   FILTER_3D(2);
         }
   else if ((datatype == GL_UNSIGNED_BYTE) && (comps == 3)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D(0);
   FILTER_3D(1);
         }
   else if ((datatype == GL_UNSIGNED_BYTE) && (comps == 2)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D(0);
         }
   else if ((datatype == GL_UNSIGNED_BYTE) && (comps == 1)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
         }
   else if ((datatype == GL_BYTE) && (comps == 4)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D_SIGNED(0);
   FILTER_3D_SIGNED(1);
   FILTER_3D_SIGNED(2);
         }
   else if ((datatype == GL_BYTE) && (comps == 3)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D_SIGNED(0);
   FILTER_3D_SIGNED(1);
         }
   else if ((datatype == GL_BYTE) && (comps == 2)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D_SIGNED(0);
         }
   else if ((datatype == GL_BYTE) && (comps == 1)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
         }
   else if ((datatype == GL_UNSIGNED_SHORT) && (comps == 4)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D(0);
   FILTER_3D(1);
   FILTER_3D(2);
         }
   else if ((datatype == GL_UNSIGNED_SHORT) && (comps == 3)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D(0);
   FILTER_3D(1);
         }
   else if ((datatype == GL_UNSIGNED_SHORT) && (comps == 2)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D(0);
         }
   else if ((datatype == GL_UNSIGNED_SHORT) && (comps == 1)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
         }
   else if ((datatype == GL_SHORT) && (comps == 4)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D(0);
   FILTER_3D(1);
   FILTER_3D(2);
         }
   else if ((datatype == GL_SHORT) && (comps == 3)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D(0);
   FILTER_3D(1);
         }
   else if ((datatype == GL_SHORT) && (comps == 2)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_3D(0);
         }
   else if ((datatype == GL_SHORT) && (comps == 1)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
         }
   else if ((datatype == GL_FLOAT) && (comps == 4)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_F_3D(0);
   FILTER_F_3D(1);
   FILTER_F_3D(2);
         }
   else if ((datatype == GL_FLOAT) && (comps == 3)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_F_3D(0);
   FILTER_F_3D(1);
         }
   else if ((datatype == GL_FLOAT) && (comps == 2)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_F_3D(0);
         }
   else if ((datatype == GL_FLOAT) && (comps == 1)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
         }
   else if ((datatype == GL_HALF_FLOAT_ARB) && (comps == 4)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_HF_3D(0);
   FILTER_HF_3D(1);
   FILTER_HF_3D(2);
         }
   else if ((datatype == GL_HALF_FLOAT_ARB) && (comps == 3)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_HF_3D(0);
   FILTER_HF_3D(1);
         }
   else if ((datatype == GL_HALF_FLOAT_ARB) && (comps == 2)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   FILTER_HF_3D(0);
         }
   else if ((datatype == GL_HALF_FLOAT_ARB) && (comps == 1)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
         }
   else if ((datatype == GL_UNSIGNED_INT) && (comps == 1)) {
      const GLuint *rowA = (const GLuint *) srcRowA;
   const GLuint *rowB = (const GLuint *) srcRowB;
   const GLuint *rowC = (const GLuint *) srcRowC;
   const GLuint *rowD = (const GLuint *) srcRowD;
            for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const uint64_t tmp = (((uint64_t) rowA[j] + (uint64_t) rowA[k])
                           }
   else if ((datatype == GL_UNSIGNED_SHORT_5_6_5) && (comps == 3)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0x1f;
   const GLint rowAr1 = rowA[k] & 0x1f;
   const GLint rowBr0 = rowB[j] & 0x1f;
   const GLint rowBr1 = rowB[k] & 0x1f;
   const GLint rowCr0 = rowC[j] & 0x1f;
   const GLint rowCr1 = rowC[k] & 0x1f;
   const GLint rowDr0 = rowD[j] & 0x1f;
   const GLint rowDr1 = rowD[k] & 0x1f;
   const GLint rowAg0 = (rowA[j] >> 5) & 0x3f;
   const GLint rowAg1 = (rowA[k] >> 5) & 0x3f;
   const GLint rowBg0 = (rowB[j] >> 5) & 0x3f;
   const GLint rowBg1 = (rowB[k] >> 5) & 0x3f;
   const GLint rowCg0 = (rowC[j] >> 5) & 0x3f;
   const GLint rowCg1 = (rowC[k] >> 5) & 0x3f;
   const GLint rowDg0 = (rowD[j] >> 5) & 0x3f;
   const GLint rowDg1 = (rowD[k] >> 5) & 0x3f;
   const GLint rowAb0 = (rowA[j] >> 11) & 0x1f;
   const GLint rowAb1 = (rowA[k] >> 11) & 0x1f;
   const GLint rowBb0 = (rowB[j] >> 11) & 0x1f;
   const GLint rowBb1 = (rowB[k] >> 11) & 0x1f;
   const GLint rowCb0 = (rowC[j] >> 11) & 0x1f;
   const GLint rowCb1 = (rowC[k] >> 11) & 0x1f;
   const GLint rowDb0 = (rowD[j] >> 11) & 0x1f;
   const GLint rowDb1 = (rowD[k] >> 11) & 0x1f;
   const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
               }
   else if ((datatype == GL_UNSIGNED_SHORT_4_4_4_4) && (comps == 4)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0xf;
   const GLint rowAr1 = rowA[k] & 0xf;
   const GLint rowBr0 = rowB[j] & 0xf;
   const GLint rowBr1 = rowB[k] & 0xf;
   const GLint rowCr0 = rowC[j] & 0xf;
   const GLint rowCr1 = rowC[k] & 0xf;
   const GLint rowDr0 = rowD[j] & 0xf;
   const GLint rowDr1 = rowD[k] & 0xf;
   const GLint rowAg0 = (rowA[j] >> 4) & 0xf;
   const GLint rowAg1 = (rowA[k] >> 4) & 0xf;
   const GLint rowBg0 = (rowB[j] >> 4) & 0xf;
   const GLint rowBg1 = (rowB[k] >> 4) & 0xf;
   const GLint rowCg0 = (rowC[j] >> 4) & 0xf;
   const GLint rowCg1 = (rowC[k] >> 4) & 0xf;
   const GLint rowDg0 = (rowD[j] >> 4) & 0xf;
   const GLint rowDg1 = (rowD[k] >> 4) & 0xf;
   const GLint rowAb0 = (rowA[j] >> 8) & 0xf;
   const GLint rowAb1 = (rowA[k] >> 8) & 0xf;
   const GLint rowBb0 = (rowB[j] >> 8) & 0xf;
   const GLint rowBb1 = (rowB[k] >> 8) & 0xf;
   const GLint rowCb0 = (rowC[j] >> 8) & 0xf;
   const GLint rowCb1 = (rowC[k] >> 8) & 0xf;
   const GLint rowDb0 = (rowD[j] >> 8) & 0xf;
   const GLint rowDb1 = (rowD[k] >> 8) & 0xf;
   const GLint rowAa0 = (rowA[j] >> 12) & 0xf;
   const GLint rowAa1 = (rowA[k] >> 12) & 0xf;
   const GLint rowBa0 = (rowB[j] >> 12) & 0xf;
   const GLint rowBa1 = (rowB[k] >> 12) & 0xf;
   const GLint rowCa0 = (rowC[j] >> 12) & 0xf;
   const GLint rowCa1 = (rowC[k] >> 12) & 0xf;
   const GLint rowDa0 = (rowD[j] >> 12) & 0xf;
   const GLint rowDa1 = (rowD[k] >> 12) & 0xf;
   const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                              }
   else if ((datatype == GL_UNSIGNED_SHORT_1_5_5_5_REV) && (comps == 4)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0x1f;
   const GLint rowAr1 = rowA[k] & 0x1f;
   const GLint rowBr0 = rowB[j] & 0x1f;
   const GLint rowBr1 = rowB[k] & 0x1f;
   const GLint rowCr0 = rowC[j] & 0x1f;
   const GLint rowCr1 = rowC[k] & 0x1f;
   const GLint rowDr0 = rowD[j] & 0x1f;
   const GLint rowDr1 = rowD[k] & 0x1f;
   const GLint rowAg0 = (rowA[j] >> 5) & 0x1f;
   const GLint rowAg1 = (rowA[k] >> 5) & 0x1f;
   const GLint rowBg0 = (rowB[j] >> 5) & 0x1f;
   const GLint rowBg1 = (rowB[k] >> 5) & 0x1f;
   const GLint rowCg0 = (rowC[j] >> 5) & 0x1f;
   const GLint rowCg1 = (rowC[k] >> 5) & 0x1f;
   const GLint rowDg0 = (rowD[j] >> 5) & 0x1f;
   const GLint rowDg1 = (rowD[k] >> 5) & 0x1f;
   const GLint rowAb0 = (rowA[j] >> 10) & 0x1f;
   const GLint rowAb1 = (rowA[k] >> 10) & 0x1f;
   const GLint rowBb0 = (rowB[j] >> 10) & 0x1f;
   const GLint rowBb1 = (rowB[k] >> 10) & 0x1f;
   const GLint rowCb0 = (rowC[j] >> 10) & 0x1f;
   const GLint rowCb1 = (rowC[k] >> 10) & 0x1f;
   const GLint rowDb0 = (rowD[j] >> 10) & 0x1f;
   const GLint rowDb1 = (rowD[k] >> 10) & 0x1f;
   const GLint rowAa0 = (rowA[j] >> 15) & 0x1;
   const GLint rowAa1 = (rowA[k] >> 15) & 0x1;
   const GLint rowBa0 = (rowB[j] >> 15) & 0x1;
   const GLint rowBa1 = (rowB[k] >> 15) & 0x1;
   const GLint rowCa0 = (rowC[j] >> 15) & 0x1;
   const GLint rowCa1 = (rowC[k] >> 15) & 0x1;
   const GLint rowDa0 = (rowD[j] >> 15) & 0x1;
   const GLint rowDa1 = (rowD[k] >> 15) & 0x1;
   const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                              }
   else if ((datatype == GL_UNSIGNED_SHORT_5_5_5_1) && (comps == 4)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = (rowA[j] >> 11) & 0x1f;
   const GLint rowAr1 = (rowA[k] >> 11) & 0x1f;
   const GLint rowBr0 = (rowB[j] >> 11) & 0x1f;
   const GLint rowBr1 = (rowB[k] >> 11) & 0x1f;
   const GLint rowCr0 = (rowC[j] >> 11) & 0x1f;
   const GLint rowCr1 = (rowC[k] >> 11) & 0x1f;
   const GLint rowDr0 = (rowD[j] >> 11) & 0x1f;
   const GLint rowDr1 = (rowD[k] >> 11) & 0x1f;
   const GLint rowAg0 = (rowA[j] >> 6) & 0x1f;
   const GLint rowAg1 = (rowA[k] >> 6) & 0x1f;
   const GLint rowBg0 = (rowB[j] >> 6) & 0x1f;
   const GLint rowBg1 = (rowB[k] >> 6) & 0x1f;
   const GLint rowCg0 = (rowC[j] >> 6) & 0x1f;
   const GLint rowCg1 = (rowC[k] >> 6) & 0x1f;
   const GLint rowDg0 = (rowD[j] >> 6) & 0x1f;
   const GLint rowDg1 = (rowD[k] >> 6) & 0x1f;
   const GLint rowAb0 = (rowA[j] >> 1) & 0x1f;
   const GLint rowAb1 = (rowA[k] >> 1) & 0x1f;
   const GLint rowBb0 = (rowB[j] >> 1) & 0x1f;
   const GLint rowBb1 = (rowB[k] >> 1) & 0x1f;
   const GLint rowCb0 = (rowC[j] >> 1) & 0x1f;
   const GLint rowCb1 = (rowC[k] >> 1) & 0x1f;
   const GLint rowDb0 = (rowD[j] >> 1) & 0x1f;
   const GLint rowDb1 = (rowD[k] >> 1) & 0x1f;
   const GLint rowAa0 = (rowA[j] & 0x1);
   const GLint rowAa1 = (rowA[k] & 0x1);
   const GLint rowBa0 = (rowB[j] & 0x1);
   const GLint rowBa1 = (rowB[k] & 0x1);
   const GLint rowCa0 = (rowC[j] & 0x1);
   const GLint rowCa1 = (rowC[k] & 0x1);
   const GLint rowDa0 = (rowD[j] & 0x1);
   const GLint rowDa1 = (rowD[k] & 0x1);
   const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                              }
   else if ((datatype == GL_UNSIGNED_BYTE_3_3_2) && (comps == 3)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0x3;
   const GLint rowAr1 = rowA[k] & 0x3;
   const GLint rowBr0 = rowB[j] & 0x3;
   const GLint rowBr1 = rowB[k] & 0x3;
   const GLint rowCr0 = rowC[j] & 0x3;
   const GLint rowCr1 = rowC[k] & 0x3;
   const GLint rowDr0 = rowD[j] & 0x3;
   const GLint rowDr1 = rowD[k] & 0x3;
   const GLint rowAg0 = (rowA[j] >> 2) & 0x7;
   const GLint rowAg1 = (rowA[k] >> 2) & 0x7;
   const GLint rowBg0 = (rowB[j] >> 2) & 0x7;
   const GLint rowBg1 = (rowB[k] >> 2) & 0x7;
   const GLint rowCg0 = (rowC[j] >> 2) & 0x7;
   const GLint rowCg1 = (rowC[k] >> 2) & 0x7;
   const GLint rowDg0 = (rowD[j] >> 2) & 0x7;
   const GLint rowDg1 = (rowD[k] >> 2) & 0x7;
   const GLint rowAb0 = (rowA[j] >> 5) & 0x7;
   const GLint rowAb1 = (rowA[k] >> 5) & 0x7;
   const GLint rowBb0 = (rowB[j] >> 5) & 0x7;
   const GLint rowBb1 = (rowB[k] >> 5) & 0x7;
   const GLint rowCb0 = (rowC[j] >> 5) & 0x7;
   const GLint rowCb1 = (rowC[k] >> 5) & 0x7;
   const GLint rowDb0 = (rowD[j] >> 5) & 0x7;
   const GLint rowDb1 = (rowD[k] >> 5) & 0x7;
   const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
               }
   else if (datatype == MESA_UNSIGNED_BYTE_4_4 && comps == 2) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0xf;
   const GLint rowAr1 = rowA[k] & 0xf;
   const GLint rowBr0 = rowB[j] & 0xf;
   const GLint rowBr1 = rowB[k] & 0xf;
   const GLint rowCr0 = rowC[j] & 0xf;
   const GLint rowCr1 = rowC[k] & 0xf;
   const GLint rowDr0 = rowD[j] & 0xf;
   const GLint rowDr1 = rowD[k] & 0xf;
   const GLint rowAg0 = (rowA[j] >> 4) & 0xf;
   const GLint rowAg1 = (rowA[k] >> 4) & 0xf;
   const GLint rowBg0 = (rowB[j] >> 4) & 0xf;
   const GLint rowBg1 = (rowB[k] >> 4) & 0xf;
   const GLint rowCg0 = (rowC[j] >> 4) & 0xf;
   const GLint rowCg1 = (rowC[k] >> 4) & 0xf;
   const GLint rowDg0 = (rowD[j] >> 4) & 0xf;
   const GLint rowDg1 = (rowD[k] >> 4) & 0xf;
   const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
               }
   else if ((datatype == GL_UNSIGNED_INT_2_10_10_10_REV) && (comps == 4)) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   const GLint rowAr0 = rowA[j] & 0x3ff;
   const GLint rowAr1 = rowA[k] & 0x3ff;
   const GLint rowBr0 = rowB[j] & 0x3ff;
   const GLint rowBr1 = rowB[k] & 0x3ff;
   const GLint rowCr0 = rowC[j] & 0x3ff;
   const GLint rowCr1 = rowC[k] & 0x3ff;
   const GLint rowDr0 = rowD[j] & 0x3ff;
   const GLint rowDr1 = rowD[k] & 0x3ff;
   const GLint rowAg0 = (rowA[j] >> 10) & 0x3ff;
   const GLint rowAg1 = (rowA[k] >> 10) & 0x3ff;
   const GLint rowBg0 = (rowB[j] >> 10) & 0x3ff;
   const GLint rowBg1 = (rowB[k] >> 10) & 0x3ff;
   const GLint rowCg0 = (rowC[j] >> 10) & 0x3ff;
   const GLint rowCg1 = (rowC[k] >> 10) & 0x3ff;
   const GLint rowDg0 = (rowD[j] >> 10) & 0x3ff;
   const GLint rowDg1 = (rowD[k] >> 10) & 0x3ff;
   const GLint rowAb0 = (rowA[j] >> 20) & 0x3ff;
   const GLint rowAb1 = (rowA[k] >> 20) & 0x3ff;
   const GLint rowBb0 = (rowB[j] >> 20) & 0x3ff;
   const GLint rowBb1 = (rowB[k] >> 20) & 0x3ff;
   const GLint rowCb0 = (rowC[j] >> 20) & 0x3ff;
   const GLint rowCb1 = (rowC[k] >> 20) & 0x3ff;
   const GLint rowDb0 = (rowD[j] >> 20) & 0x3ff;
   const GLint rowDb1 = (rowD[k] >> 20) & 0x3ff;
   const GLint rowAa0 = (rowA[j] >> 30) & 0x3;
   const GLint rowAa1 = (rowA[k] >> 30) & 0x3;
   const GLint rowBa0 = (rowB[j] >> 30) & 0x3;
   const GLint rowBa1 = (rowB[k] >> 30) & 0x3;
   const GLint rowCa0 = (rowC[j] >> 30) & 0x3;
   const GLint rowCa1 = (rowC[k] >> 30) & 0x3;
   const GLint rowDa0 = (rowD[j] >> 30) & 0x3;
   const GLint rowDa1 = (rowD[k] >> 30) & 0x3;
   const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                                       else if (datatype == GL_UNSIGNED_INT_5_9_9_9_REV && comps == 3) {
               GLfloat res[3];
   GLfloat rowAj[3], rowBj[3], rowCj[3], rowDj[3];
            for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   rgb9e5_to_float3(rowA[j], rowAj);
   rgb9e5_to_float3(rowB[j], rowBj);
   rgb9e5_to_float3(rowC[j], rowCj);
   rgb9e5_to_float3(rowD[j], rowDj);
   rgb9e5_to_float3(rowA[k], rowAk);
   rgb9e5_to_float3(rowB[k], rowBk);
   rgb9e5_to_float3(rowC[k], rowCk);
   rgb9e5_to_float3(rowD[k], rowDk);
   res[0] = (rowAj[0] + rowAk[0] + rowBj[0] + rowBk[0] +
         res[1] = (rowAj[1] + rowAk[1] + rowBj[1] + rowBk[1] +
         res[2] = (rowAj[2] + rowAk[2] + rowBj[2] + rowBk[2] +
                        else if (datatype == GL_UNSIGNED_INT_10F_11F_11F_REV && comps == 3) {
               GLfloat res[3];
   GLfloat rowAj[3], rowBj[3], rowCj[3], rowDj[3];
            for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
   r11g11b10f_to_float3(rowA[j], rowAj);
   r11g11b10f_to_float3(rowB[j], rowBj);
   r11g11b10f_to_float3(rowC[j], rowCj);
   r11g11b10f_to_float3(rowD[j], rowDj);
   r11g11b10f_to_float3(rowA[k], rowAk);
   r11g11b10f_to_float3(rowB[k], rowBk);
   r11g11b10f_to_float3(rowC[k], rowCk);
   r11g11b10f_to_float3(rowD[k], rowDk);
   res[0] = (rowAj[0] + rowAk[0] + rowBj[0] + rowBk[0] +
         res[1] = (rowAj[1] + rowAk[1] + rowBj[1] + rowBk[1] +
         res[2] = (rowAj[2] + rowAk[2] + rowBj[2] + rowBk[2] +
                        else if (datatype == GL_FLOAT_32_UNSIGNED_INT_24_8_REV && comps == 1) {
               for (i = j = 0, k = k0; i < (GLuint) dstWidth;
      i++, j += colStride, k += colStride) {
                  else {
            }
         /*
   * These functions generate a 1/2-size mipmap image from a source image.
   * Texture borders are handled by copying or averaging the source image's
   * border texels, depending on the scale-down factor.
   */
      static void
   make_1d_mipmap(GLenum datatype, GLuint comps, GLint border,
               {
      const GLint bpt = bytes_per_pixel(datatype, comps);
   const GLubyte *src;
            /* skip the border pixel, if any */
   src = srcPtr + border * bpt;
            /* we just duplicate the input row, kind of hack, saves code */
   do_row(datatype, comps, srcWidth - 2 * border, src, src,
            if (border) {
      /* copy left-most pixel from source */
   assert(dstPtr);
   assert(srcPtr);
   memcpy(dstPtr, srcPtr, bpt);
   /* copy right-most pixel from source */
   memcpy(dstPtr + (dstWidth - 1) * bpt,
               }
         static void
   make_2d_mipmap(GLenum datatype, GLuint comps, GLint border,
                  GLint srcWidth, GLint srcHeight,
      {
      const GLint bpt = bytes_per_pixel(datatype, comps);
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLubyte *srcA, *srcB;
   GLubyte *dst;
            /* Compute src and dst pointers, skipping any border */
   srcA = srcPtr + border * ((srcWidth + 1) * bpt);
   if (srcHeight > 1 && srcHeight > dstHeight) {
      /* sample from two source rows */
   srcB = srcA + srcRowStride;
      }
   else {
      /* sample from one source row */
   srcB = srcA;
                        for (row = 0; row < dstHeightNB; row++) {
      do_row(datatype, comps, srcWidthNB, srcA, srcB,
         srcA += srcRowStep * srcRowStride;
   srcB += srcRowStep * srcRowStride;
               /* This is ugly but probably won't be used much */
   if (border > 0) {
      /* fill in dest border */
   /* lower-left border pixel */
   assert(dstPtr);
   assert(srcPtr);
   memcpy(dstPtr, srcPtr, bpt);
   /* lower-right border pixel */
   memcpy(dstPtr + (dstWidth - 1) * bpt,
         /* upper-left border pixel */
   memcpy(dstPtr + dstWidth * (dstHeight - 1) * bpt,
         /* upper-right border pixel */
   memcpy(dstPtr + (dstWidth * dstHeight - 1) * bpt,
         /* lower border */
   do_row(datatype, comps, srcWidthNB,
         srcPtr + bpt,
   srcPtr + bpt,
   /* upper border */
   do_row(datatype, comps, srcWidthNB,
         srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
   srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
   dstWidthNB,
   /* left and right borders */
   if (srcHeight == dstHeight) {
      /* copy border pixel from src to dst */
   for (row = 1; row < srcHeight; row++) {
      memcpy(dstPtr + dstWidth * row * bpt,
         memcpy(dstPtr + (dstWidth * row + dstWidth - 1) * bpt,
         }
   else {
      /* average two src pixels each dest pixel */
   for (row = 0; row < dstHeightNB; row += 2) {
      do_row(datatype, comps, 1,
         srcPtr + (srcWidth * (row * 2 + 1)) * bpt,
   srcPtr + (srcWidth * (row * 2 + 2)) * bpt,
   do_row(datatype, comps, 1,
         srcPtr + (srcWidth * (row * 2 + 1) + srcWidth - 1) * bpt,
               }
         static void
   make_3d_mipmap(GLenum datatype, GLuint comps, GLint border,
                  GLint srcWidth, GLint srcHeight, GLint srcDepth,
      {
      const GLint bpt = bytes_per_pixel(datatype, comps);
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint srcDepthNB = srcDepth - 2 * border;
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLint dstDepthNB = dstDepth - 2 * border;
   GLint img, row;
   GLint bytesPerSrcImage, bytesPerDstImage;
                     bytesPerSrcImage = srcRowStride * srcHeight * bpt;
            /* Offset between adjacent src images to be averaged together */
            /* Offset between adjacent src rows to be averaged together */
            /*
   * Need to average together up to 8 src pixels for each dest pixel.
   * Break that down into 3 operations:
   *   1. take two rows from source image and average them together.
   *   2. take two rows from next source image and average them together.
   *   3. take the two averaged rows and average them for the final dst row.
            /*
   printf("mip3d %d x %d x %d  ->  %d x %d x %d\n",
                  for (img = 0; img < dstDepthNB; img++) {
      /* first source image pointer, skipping border */
   const GLubyte *imgSrcA = srcPtr[img * 2 + border]
         /* second source image pointer, skipping border */
   const GLubyte *imgSrcB = srcPtr[img * 2 + srcImageOffset + border]
            /* address of the dest image, skipping border */
   GLubyte *imgDst = dstPtr[img + border]
            /* setup the four source row pointers and the dest row pointer */
   const GLubyte *srcImgARowA = imgSrcA;
   const GLubyte *srcImgARowB = imgSrcA + srcRowOffset;
   const GLubyte *srcImgBRowA = imgSrcB;
   const GLubyte *srcImgBRowB = imgSrcB + srcRowOffset;
            for (row = 0; row < dstHeightNB; row++) {
      do_row_3D(datatype, comps, srcWidthNB,
                        /* advance to next rows */
   srcImgARowA += srcRowStride + srcRowOffset;
   srcImgARowB += srcRowStride + srcRowOffset;
   srcImgBRowA += srcRowStride + srcRowOffset;
   srcImgBRowB += srcRowStride + srcRowOffset;
                     /* Luckily we can leverage the make_2d_mipmap() function here! */
   if (border > 0) {
      /* do front border image */
   make_2d_mipmap(datatype, comps, 1,
               /* do back border image */
   make_2d_mipmap(datatype, comps, 1,
                  /* do four remaining border edges that span the image slices */
   if (srcDepth == dstDepth) {
      /* just copy border pixels from src to dst */
   for (img = 0; img < dstDepthNB; img++) {
                     /* do border along [img][row=0][col=0] */
   src = srcPtr[img * 2];
                  /* do border along [img][row=dstHeight-1][col=0] */
   src = srcPtr[img * 2] + (srcHeight - 1) * srcRowStride;
                  /* do border along [img][row=0][col=dstWidth-1] */
   src = srcPtr[img * 2] + (srcWidth - 1) * bpt;
                  /* do border along [img][row=dstHeight-1][col=dstWidth-1] */
   src = srcPtr[img * 2] + (bytesPerSrcImage - bpt);
   dst = dstPtr[img] + (bytesPerDstImage - bpt);
         }
   else {
      /* average border pixels from adjacent src image pairs */
   assert(srcDepthNB == 2 * dstDepthNB);
   for (img = 0; img < dstDepthNB; img++) {
                     /* do border along [img][row=0][col=0] */
   srcA = srcPtr[img * 2 + 0];
   srcB = srcPtr[img * 2 + srcImageOffset];
                  /* do border along [img][row=dstHeight-1][col=0] */
   srcA = srcPtr[img * 2 + 0]
         srcB = srcPtr[img * 2 + srcImageOffset]
                        /* do border along [img][row=0][col=dstWidth-1] */
   srcA = srcPtr[img * 2 + 0] + (srcWidth - 1) * bpt;
   srcB = srcPtr[img * 2 + srcImageOffset] + (srcWidth - 1) * bpt;
                  /* do border along [img][row=dstHeight-1][col=dstWidth-1] */
   srcA = srcPtr[img * 2 + 0] + (bytesPerSrcImage - bpt);
   srcB = srcPtr[img * 2 + srcImageOffset] + (bytesPerSrcImage - bpt);
   dst = dstPtr[img] + (bytesPerDstImage - bpt);
               }
         /**
   * Down-sample a texture image to produce the next lower mipmap level.
   * \param comps  components per texel (1, 2, 3 or 4)
   * \param srcData  array[slice] of pointers to source image slices
   * \param dstData  array[slice] of pointers to dest image slices
   * \param srcRowStride  stride between source rows, in bytes
   * \param dstRowStride  stride between destination rows, in bytes
   */
   static void
   _mesa_generate_mipmap_level(GLenum target,
                              GLenum datatype, GLuint comps,
   GLint border,
   GLint srcWidth, GLint srcHeight, GLint srcDepth,
      {
               switch (target) {
   case GL_TEXTURE_1D:
      make_1d_mipmap(datatype, comps, border,
                  case GL_TEXTURE_2D:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      make_2d_mipmap(datatype, comps, border,
                  case GL_TEXTURE_3D:
      make_3d_mipmap(datatype, comps, border,
                  srcWidth, srcHeight, srcDepth,
         case GL_TEXTURE_1D_ARRAY_EXT:
      assert(srcHeight == 1);
   assert(dstHeight == 1);
   for (i = 0; i < dstDepth; i++) {
      make_1d_mipmap(datatype, comps, border,
            }
      case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      for (i = 0; i < dstDepth; i++) {
      make_2d_mipmap(datatype, comps, border,
            }
      case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_EXTERNAL_OES:
      /* no mipmaps, do nothing */
      default:
            }
         /**
   * compute next (level+1) image size
   * \return GL_FALSE if no smaller size can be generated (eg. src is 1x1x1 size)
   */
   GLboolean
   _mesa_next_mipmap_level_size(GLenum target, GLint border,
               {
      if (srcWidth - 2 * border > 1) {
         }
   else {
                  if ((srcHeight - 2 * border > 1) &&
      target != GL_TEXTURE_1D_ARRAY_EXT &&
   target != GL_PROXY_TEXTURE_1D_ARRAY_EXT) {
      }
   else {
                  if ((srcDepth - 2 * border > 1) &&
      target != GL_TEXTURE_2D_ARRAY_EXT &&
   target != GL_PROXY_TEXTURE_2D_ARRAY_EXT &&
   target != GL_TEXTURE_CUBE_MAP_ARRAY &&
   target != GL_PROXY_TEXTURE_CUBE_MAP_ARRAY) {
      }
   else {
                  if (*dstWidth == srcWidth &&
      *dstHeight == srcHeight &&
   *dstDepth == srcDepth) {
      }
   else {
            }
         /**
   * Helper function for mipmap generation.
   * Make sure the specified destination mipmap level is the right size/format
   * for mipmap generation.  If not, (re) allocate it.
   * \return GL_TRUE if successful, GL_FALSE if mipmap generation should stop
   */
   static GLboolean
   prepare_mipmap_level(struct gl_context *ctx,
                     {
      const GLuint numFaces = _mesa_num_tex_faces(texObj->Target);
            if (texObj->Immutable) {
      /* The texture was created with glTexStorage() so the number/size of
   * mipmap levels is fixed and the storage for all images is already
   * allocated.
   */
   if (!texObj->Image[0][level]) {
      /* No more levels to create - we're done */
      }
   else {
      /* Nothing to do - the texture memory must have already been
   * allocated to the right size so we're all set.
   */
                  for (face = 0; face < numFaces; face++) {
      struct gl_texture_image *dstImage;
            dstImage = _mesa_get_tex_image(ctx, texObj, target, level);
   if (!dstImage) {
      /* out of memory */
               if (dstImage->Width != width ||
      dstImage->Height != height ||
   dstImage->Depth != depth ||
   dstImage->Border != border ||
   dstImage->InternalFormat != intFormat ||
   dstImage->TexFormat != format) {
                  _mesa_init_teximage_fields(ctx, dstImage,
                                          ctx->NewState |= _NEW_TEXTURE_OBJECT;
                     }
         /**
   * Prepare all mipmap levels beyond 'baseLevel' for mipmap generation.
   * When finished, all the gl_texture_image structures for the smaller
   * mipmap levels will be consistent with the base level (in terms of
   * dimensions, format, etc).
   */
   void
   _mesa_prepare_mipmap_levels(struct gl_context *ctx,
               {
      const struct gl_texture_image *baseImage =
            if (baseImage == NULL)
            const GLint border = 0;
   GLint width = baseImage->Width;
   GLint height = baseImage->Height;
   GLint depth = baseImage->Depth;
   const GLenum intFormat = baseImage->InternalFormat;
   const mesa_format texFormat = baseImage->TexFormat;
            /* Prepare baseLevel + 1, baseLevel + 2, ... */
   for (unsigned level = baseLevel + 1; level <= maxLevel; level++) {
      if (!_mesa_next_mipmap_level_size(texObj->Target, border,
                  /* all done */
               if (!prepare_mipmap_level(ctx, texObj, level,
                              width = newWidth;
   height = newHeight;
         }
         static void
   generate_mipmap_uncompressed(struct gl_context *ctx, GLenum target,
                     {
      GLuint level;
   GLenum datatype;
                     for (level = texObj->Attrib.BaseLevel; level < maxLevel; level++) {
      /* generate image[level+1] from image[level] */
   struct gl_texture_image *srcImage, *dstImage;
   GLint srcRowStride, dstRowStride;
   GLint srcWidth, srcHeight, srcDepth;
   GLint dstWidth, dstHeight, dstDepth;
   GLint border;
   GLint slice;
   GLubyte **srcMaps, **dstMaps;
            /* get src image parameters */
   srcImage = _mesa_select_tex_image(texObj, target, level);
   assert(srcImage);
   srcWidth = srcImage->Width;
   srcHeight = srcImage->Height;
   srcDepth = srcImage->Depth;
            /* get dest gl_texture_image */
   dstImage = _mesa_select_tex_image(texObj, target, level + 1);
   if (!dstImage) {
         }
   dstWidth = dstImage->Width;
   dstHeight = dstImage->Height;
            if (target == GL_TEXTURE_1D_ARRAY) {
      srcDepth = srcHeight;
   dstDepth = dstHeight;
   srcHeight = 1;
               /* Map src texture image slices */
   srcMaps = calloc(srcDepth, sizeof(GLubyte *));
   if (srcMaps) {
      for (slice = 0; slice < srcDepth; slice++) {
      st_MapTextureImage(ctx, srcImage, slice,
                     if (!srcMaps[slice]) {
      success = GL_FALSE;
            }
   else {
                  /* Map dst texture image slices */
   dstMaps = calloc(dstDepth, sizeof(GLubyte *));
   if (dstMaps) {
      for (slice = 0; slice < dstDepth; slice++) {
      st_MapTextureImage(ctx, dstImage, slice,
                     if (!dstMaps[slice]) {
      success = GL_FALSE;
            }
   else {
                  if (success) {
      /* generate one mipmap level (for 1D/2D/3D/array/etc texture) */
   _mesa_generate_mipmap_level(target, datatype, comps, border,
                                 /* Unmap src image slices */
   if (srcMaps) {
      for (slice = 0; slice < srcDepth; slice++) {
      if (srcMaps[slice]) {
            }
               /* Unmap dst image slices */
   if (dstMaps) {
      for (slice = 0; slice < dstDepth; slice++) {
      if (dstMaps[slice]) {
            }
               if (!success) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "mipmap generation");
            }
         static void
   generate_mipmap_compressed(struct gl_context *ctx, GLenum target,
                     {
      GLuint level;
   mesa_format temp_format;
   GLint components;
   GLuint temp_src_row_stride, temp_src_img_stride; /* in bytes */
   GLubyte *temp_src = NULL, *temp_dst = NULL;
   GLenum temp_datatype;
   GLenum temp_base_format;
            /* only two types of compressed textures at this time */
   assert(texObj->Target == GL_TEXTURE_2D ||
         texObj->Target == GL_TEXTURE_2D_ARRAY ||
            /*
   * Choose a format for the temporary, uncompressed base image.
   * Then, get number of components, choose temporary image datatype,
   * and get base format.
   */
                     switch (_mesa_get_format_datatype(srcImage->TexFormat)) {
   case GL_FLOAT:
      temp_datatype = GL_FLOAT;
      case GL_SIGNED_NORMALIZED:
      /* Revisit this if we get compressed formats with >8 bits per component */
   temp_datatype = GL_BYTE;
      default:
                              /* allocate storage for the temporary, uncompressed image */
   temp_src_row_stride = _mesa_format_row_stride(temp_format, srcImage->Width);
   temp_src_img_stride = _mesa_format_image_size(temp_format, srcImage->Width,
                  /* Allocate storage for arrays of slice pointers */
   temp_src_slices = malloc(srcImage->Depth * sizeof(GLubyte *));
            if (!temp_src || !temp_src_slices || !temp_dst_slices) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "generate mipmaps");
               /* decompress base image to the temporary src buffer */
   {
      /* save pixel packing mode */
   struct gl_pixelstore_attrib save = ctx->Pack;
   /* use default/tight packing parameters */
            /* Get the uncompressed image */
   assert(srcImage->Level == texObj->Attrib.BaseLevel);
   st_GetTexSubImage(ctx,
                     0, 0, 0,
   srcImage->Width, srcImage->Height,
   /* restore packing mode */
               for (level = texObj->Attrib.BaseLevel; level < maxLevel; level++) {
      /* generate image[level+1] from image[level] */
   const struct gl_texture_image *srcImage;
   struct gl_texture_image *dstImage;
   GLint srcWidth, srcHeight, srcDepth;
   GLint dstWidth, dstHeight, dstDepth;
   GLint border;
   GLuint temp_dst_row_stride, temp_dst_img_stride; /* in bytes */
            /* get src image parameters */
   srcImage = _mesa_select_tex_image(texObj, target, level);
   assert(srcImage);
   srcWidth = srcImage->Width;
   srcHeight = srcImage->Height;
   srcDepth = srcImage->Depth;
            /* get dest gl_texture_image */
   dstImage = _mesa_select_tex_image(texObj, target, level + 1);
   if (!dstImage) {
         }
   dstWidth = dstImage->Width;
   dstHeight = dstImage->Height;
            /* Compute dst image strides and alloc memory on first iteration */
   temp_dst_row_stride = _mesa_format_row_stride(temp_format, dstWidth);
   temp_dst_img_stride = _mesa_format_image_size(temp_format, dstWidth,
         if (!temp_dst) {
      temp_dst = malloc(temp_dst_img_stride * dstDepth);
   if (!temp_dst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "generate mipmaps");
                  /* for 2D arrays, setup array[depth] of slice pointers */
   for (i = 0; i < srcDepth; i++) {
         }
   for (i = 0; i < dstDepth; i++) {
                  /* Rescale src image to dest image.
   * This will loop over the slices of a 2D array.
   */
   _mesa_generate_mipmap_level(target, temp_datatype, components, border,
                                    /* The image space was allocated above so use glTexSubImage now */
   st_TexSubImage(ctx, 2, dstImage,
                        /* swap src and dest pointers */
   {
      GLubyte *temp = temp_src;
   temp_src = temp_dst;
   temp_dst = temp;
   temp_src_row_stride = temp_dst_row_stride;
               end:
      free(temp_src);
   free(temp_dst);
   free(temp_src_slices);
      }
      /**
   * Automatic mipmap generation.
   * This is the fallback/default function for mipmap generation.
   * Generate a complete set of mipmaps from texObj's BaseLevel image.
   * Stop at texObj's MaxLevel or when we get to the 1x1 texture.
   * For cube maps, target will be one of
   * GL_TEXTURE_CUBE_MAP_POSITIVE/NEGATIVE_X/Y/Z; never GL_TEXTURE_CUBE_MAP.
   */
   void
   _mesa_generate_mipmap(struct gl_context *ctx, GLenum target,
         {
      struct gl_texture_image *srcImage;
            assert(texObj);
   srcImage = _mesa_select_tex_image(texObj, target, texObj->Attrib.BaseLevel);
            maxLevel = _mesa_max_texture_levels(ctx, texObj->Target) - 1;
                              if (_mesa_is_format_compressed(srcImage->TexFormat)) {
         } else {
            }
