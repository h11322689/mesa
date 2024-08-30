   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
   * \file pixelstore.c
   * glPixelStore functions.
   */
         #include "util/glheader.h"
   #include "bufferobj.h"
   #include "context.h"
   #include "pixelstore.h"
   #include "mtypes.h"
   #include "util/rounding.h"
   #include "api_exec_decl.h"
         static ALWAYS_INLINE void
   pixel_storei(GLenum pname, GLint param, bool no_error)
   {
      /* NOTE: this call can't be compiled into the display list */
            switch (pname) {
      case GL_PACK_SWAP_BYTES:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         ctx->Pack.SwapBytes = param ? GL_TRUE : GL_FALSE;
      case GL_PACK_LSB_FIRST:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         ctx->Pack.LsbFirst = param ? GL_TRUE : GL_FALSE;
      case GL_PACK_ROW_LENGTH:
      if (!no_error && _mesa_is_gles1(ctx))
         if (!no_error && param<0)
         ctx->Pack.RowLength = param;
      case GL_PACK_IMAGE_HEIGHT:
      if (!no_error && !_mesa_is_desktop_gl(ctx) && !_mesa_is_gles3(ctx))
         if (!no_error && param<0)
         ctx->Pack.ImageHeight = param;
      case GL_PACK_SKIP_PIXELS:
      if (!no_error && _mesa_is_gles1(ctx))
         if (!no_error && param<0)
         ctx->Pack.SkipPixels = param;
      case GL_PACK_SKIP_ROWS:
      if (!no_error && _mesa_is_gles1(ctx))
         if (!no_error && param<0)
         ctx->Pack.SkipRows = param;
      case GL_PACK_SKIP_IMAGES:
      if (!no_error && !_mesa_is_desktop_gl(ctx) && !_mesa_is_gles3(ctx))
         if (!no_error && param<0)
         ctx->Pack.SkipImages = param;
      case GL_PACK_ALIGNMENT:
      if (!no_error && param!=1 && param!=2 && param!=4 && param!=8)
         ctx->Pack.Alignment = param;
      case GL_PACK_INVERT_MESA:
      if (!no_error && !_mesa_has_MESA_pack_invert(ctx))
         ctx->Pack.Invert = param;
      case GL_PACK_REVERSE_ROW_ORDER_ANGLE:
      if (!no_error && !_mesa_has_ANGLE_pack_reverse_row_order(ctx))
         ctx->Pack.Invert = param;
      case GL_PACK_COMPRESSED_BLOCK_WIDTH:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         if (!no_error && param<0)
         ctx->Pack.CompressedBlockWidth = param;
      case GL_PACK_COMPRESSED_BLOCK_HEIGHT:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         if (!no_error && param<0)
         ctx->Pack.CompressedBlockHeight = param;
      case GL_PACK_COMPRESSED_BLOCK_DEPTH:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         if (!no_error && param<0)
         ctx->Pack.CompressedBlockDepth = param;
      case GL_PACK_COMPRESSED_BLOCK_SIZE:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         if (!no_error && param<0)
                     case GL_UNPACK_SWAP_BYTES:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         ctx->Unpack.SwapBytes = param ? GL_TRUE : GL_FALSE;
      case GL_UNPACK_LSB_FIRST:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         ctx->Unpack.LsbFirst = param ? GL_TRUE : GL_FALSE;
      case GL_UNPACK_ROW_LENGTH:
      if (!no_error && _mesa_is_gles1(ctx))
         if (!no_error && param<0)
         ctx->Unpack.RowLength = param;
      case GL_UNPACK_IMAGE_HEIGHT:
      if (!no_error && !_mesa_is_desktop_gl(ctx) && !_mesa_is_gles3(ctx))
         if (!no_error && param<0)
         ctx->Unpack.ImageHeight = param;
      case GL_UNPACK_SKIP_PIXELS:
      if (!no_error && _mesa_is_gles1(ctx))
         if (!no_error && param<0)
         ctx->Unpack.SkipPixels = param;
      case GL_UNPACK_SKIP_ROWS:
      if (!no_error && _mesa_is_gles1(ctx))
         if (!no_error && param<0)
         ctx->Unpack.SkipRows = param;
      case GL_UNPACK_SKIP_IMAGES:
      if (!no_error && !_mesa_is_desktop_gl(ctx) && !_mesa_is_gles3(ctx))
         if (!no_error && param < 0)
         ctx->Unpack.SkipImages = param;
      case GL_UNPACK_ALIGNMENT:
      if (!no_error && param!=1 && param!=2 && param!=4 && param!=8)
         ctx->Unpack.Alignment = param;
      case GL_UNPACK_COMPRESSED_BLOCK_WIDTH:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         if (!no_error && param<0)
         ctx->Unpack.CompressedBlockWidth = param;
      case GL_UNPACK_COMPRESSED_BLOCK_HEIGHT:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         if (!no_error && param<0)
         ctx->Unpack.CompressedBlockHeight = param;
      case GL_UNPACK_COMPRESSED_BLOCK_DEPTH:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         if (!no_error && param<0)
         ctx->Unpack.CompressedBlockDepth = param;
      case GL_UNPACK_COMPRESSED_BLOCK_SIZE:
      if (!no_error && !_mesa_is_desktop_gl(ctx))
         if (!no_error && param<0)
         ctx->Unpack.CompressedBlockSize = param;
      default:
      if (!no_error)
         else
                  invalid_enum_error:
      _mesa_error(ctx, GL_INVALID_ENUM, "glPixelStore");
         invalid_value_error:
      _mesa_error(ctx, GL_INVALID_VALUE, "glPixelStore(param)");
      }
         void GLAPIENTRY
   _mesa_PixelStorei(GLenum pname, GLint param)
   {
         }
         void GLAPIENTRY
   _mesa_PixelStoref(GLenum pname, GLfloat param)
   {
         }
         void GLAPIENTRY
   _mesa_PixelStorei_no_error(GLenum pname, GLint param)
   {
         }
         void GLAPIENTRY
   _mesa_PixelStoref_no_error(GLenum pname, GLfloat param)
   {
         }
         /**
   * Initialize the context's pixel store state.
   */
   void
   _mesa_init_pixelstore(struct gl_context *ctx)
   {
      /* Pixel transfer */
   ctx->Pack.Alignment = 4;
   ctx->Pack.RowLength = 0;
   ctx->Pack.ImageHeight = 0;
   ctx->Pack.SkipPixels = 0;
   ctx->Pack.SkipRows = 0;
   ctx->Pack.SkipImages = 0;
   ctx->Pack.SwapBytes = GL_FALSE;
   ctx->Pack.LsbFirst = GL_FALSE;
   ctx->Pack.Invert = GL_FALSE;
   ctx->Pack.CompressedBlockWidth = 0;
   ctx->Pack.CompressedBlockHeight = 0;
   ctx->Pack.CompressedBlockDepth = 0;
   ctx->Pack.CompressedBlockSize = 0;
   _mesa_reference_buffer_object(ctx, &ctx->Pack.BufferObj, NULL);
   ctx->Unpack.Alignment = 4;
   ctx->Unpack.RowLength = 0;
   ctx->Unpack.ImageHeight = 0;
   ctx->Unpack.SkipPixels = 0;
   ctx->Unpack.SkipRows = 0;
   ctx->Unpack.SkipImages = 0;
   ctx->Unpack.SwapBytes = GL_FALSE;
   ctx->Unpack.LsbFirst = GL_FALSE;
   ctx->Unpack.Invert = GL_FALSE;
   ctx->Unpack.CompressedBlockWidth = 0;
   ctx->Unpack.CompressedBlockHeight = 0;
   ctx->Unpack.CompressedBlockDepth = 0;
   ctx->Unpack.CompressedBlockSize = 0;
            /*
   * _mesa_unpack_image() returns image data in this format.  When we
   * execute image commands (glDrawPixels(), glTexImage(), etc) from
   * within display lists we have to be sure to set the current
   * unpacking parameters to these values!
   */
   ctx->DefaultPacking.Alignment = 1;
   ctx->DefaultPacking.RowLength = 0;
   ctx->DefaultPacking.SkipPixels = 0;
   ctx->DefaultPacking.SkipRows = 0;
   ctx->DefaultPacking.ImageHeight = 0;
   ctx->DefaultPacking.SkipImages = 0;
   ctx->DefaultPacking.SwapBytes = GL_FALSE;
   ctx->DefaultPacking.LsbFirst = GL_FALSE;
   ctx->DefaultPacking.Invert = GL_FALSE;
      }
         /**
   * Check if the given compressed pixel storage parameters are legal.
   * Record a GL error if illegal.
   * \return  true if legal, false if illegal
   */
   bool
   _mesa_compressed_pixel_storage_error_check(
      struct gl_context *ctx,
   GLint dimensions,
   const struct gl_pixelstore_attrib *packing,
      {
      if (!_mesa_is_desktop_gl(ctx) || !packing->CompressedBlockSize)
            if (packing->CompressedBlockWidth &&
      packing->SkipPixels % packing->CompressedBlockWidth) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (dimensions > 1 &&
      packing->CompressedBlockHeight &&
   packing->SkipRows % packing->CompressedBlockHeight) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (dimensions > 2 &&
      packing->CompressedBlockDepth &&
   packing->SkipImages % packing->CompressedBlockDepth) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
