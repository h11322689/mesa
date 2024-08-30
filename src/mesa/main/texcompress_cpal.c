   /*
   * Mesa 3-D graphics library
   *
   * Copyright 2008 VMware, Inc.
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
   * Code to convert compressed/paletted texture images to ordinary images.
   * See the GL_OES_compressed_paletted_texture spec at
   * http://khronos.org/registry/gles/extensions/OES/OES_compressed_paletted_texture.txt
   *
   * XXX this makes it impossible to add hardware support...
   */
         #include "util/glheader.h"
   #include "context.h"
   #include "mtypes.h"
      #include "pixelstore.h"
   #include "texcompress_cpal.h"
   #include "teximage.h"
   #include "api_exec_decl.h"
         static const struct cpal_format_info {
      GLenum cpal_format;
   GLenum format;
   GLenum type;
   GLuint palette_size;
      } formats[] = {
      { GL_PALETTE4_RGB8_OES,     GL_RGB,  GL_UNSIGNED_BYTE,           16, 3 },
   { GL_PALETTE4_RGBA8_OES,    GL_RGBA, GL_UNSIGNED_BYTE,           16, 4 },
   { GL_PALETTE4_R5_G6_B5_OES, GL_RGB,  GL_UNSIGNED_SHORT_5_6_5,    16, 2 },
   { GL_PALETTE4_RGBA4_OES,    GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4,  16, 2 },
   { GL_PALETTE4_RGB5_A1_OES,  GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1,  16, 2 },
   { GL_PALETTE8_RGB8_OES,     GL_RGB,  GL_UNSIGNED_BYTE,          256, 3 },
   { GL_PALETTE8_RGBA8_OES,    GL_RGBA, GL_UNSIGNED_BYTE,          256, 4 },
   { GL_PALETTE8_R5_G6_B5_OES, GL_RGB,  GL_UNSIGNED_SHORT_5_6_5,   256, 2 },
   { GL_PALETTE8_RGBA4_OES,    GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 256, 2 },
      };
         /**
   * Get a color/entry from the palette.
   */
   static GLuint
   get_palette_entry(const struct cpal_format_info *info, const GLubyte *palette,
         {
      memcpy(pixel, palette + info->size * index, info->size);
      }
         /**
   * Convert paletted texture to color texture.
   */
   static void
   paletted_to_color(const struct cpal_format_info *info, const GLubyte *palette,
         {
      GLubyte *pix = image;
            if (info->palette_size == 16) {
      /* 4 bits per index */
            /* two pixels per iteration */
   remain = num_pixels % 2;
   for (i = 0; i < num_pixels / 2; i++) {
      pix += get_palette_entry(info, palette, (ind[i] >> 4) & 0xf, pix);
      }
   if (remain) {
            }
   else {
      /* 8 bits per index */
   const GLubyte *ind = (const GLubyte *) indices;
   for (i = 0; i < num_pixels; i++)
         }
      unsigned
   _mesa_cpal_compressed_size(int level, GLenum internalFormat,
         {
      const struct cpal_format_info *info;
   const int num_levels = -level + 1;
   int lvl;
            if (internalFormat < GL_PALETTE4_RGB8_OES
      || internalFormat > GL_PALETTE8_RGB5_A1_OES) {
               info = &formats[internalFormat - GL_PALETTE4_RGB8_OES];
            expect_size = info->palette_size * info->size;
   for (lvl = 0; lvl < num_levels; lvl++) {
      w = width >> lvl;
   if (!w)
         h = height >> lvl;
   if (!h)
            if (info->palette_size == 16)
         else
                  }
         /**
   * Convert a call to glCompressedTexImage2D() where internalFormat is a
   *  compressed palette format into a regular GLubyte/RGBA glTexImage2D() call.
   */
   void
   _mesa_cpal_compressed_teximage2d(GLenum target, GLint level,
      GLenum internalFormat,
   GLsizei width, GLsizei height,
      {
      const struct cpal_format_info *info;
   GLint lvl, num_levels;
   const GLubyte *indices;
   GLint saved_align, align;
            /* By this point, the internalFormat should have been validated.
   */
   assert(internalFormat >= GL_PALETTE4_RGB8_OES
                              /* first image follows the palette */
            saved_align = ctx->Unpack.Alignment;
            for (lvl = 0; lvl < num_levels; lvl++) {
      GLsizei w, h;
   GLuint num_texels;
            w = width >> lvl;
   if (!w)
         h = height >> lvl;
   if (!h)
         num_texels = w * h;
   if (w * info->size % align) {
      _mesa_PixelStorei(GL_UNPACK_ALIGNMENT, 1);
               /* allocate and fill dest image buffer */
   if (palette) {
      image = malloc(num_texels * info->size);
               _mesa_TexImage2D(target, lvl, info->format, w, h, 0,
                  /* advance index pointer to point to next src mipmap */
   if (info->palette_size == 16)
         else
               if (saved_align != align)
      }
