   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009-2011  VMware, Inc.  All Rights Reserved.
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
   * \file pbo.c
   * \brief Functions related to Pixel Buffer Objects.
   */
            #include "errors.h"
   #include "util/glheader.h"
   #include "bufferobj.h"
   #include "glformats.h"
   #include "image.h"
   #include "mtypes.h"
   #include "macros.h"
   #include "pbo.h"
      /**
   * When we're about to read pixel data out of a PBO (via glDrawPixels,
   * glTexImage, etc) or write data into a PBO (via glReadPixels,
   * glGetTexImage, etc) we call this function to check that we're not
   * going to read/write out of bounds.
   *
   * XXX This would also be a convenient time to check that the PBO isn't
   * currently mapped.  Whoever calls this function should check for that.
   * Remember, we can't use a PBO when it's mapped!
   *
   * If we're not using a PBO, this is a no-op.
   *
   * \param width  width of image to read/write
   * \param height  height of image to read/write
   * \param depth  depth of image to read/write
   * \param format  format of image to read/write
   * \param type  datatype of image to read/write
   * \param clientMemSize  the maximum number of bytes to read/write
   * \param ptr  the user-provided pointer/offset
   * \return GL_TRUE if the buffer access is OK, GL_FALSE if the access would
   *         go out of bounds.
   */
   GLboolean
   _mesa_validate_pbo_access(GLuint dimensions,
                           {
      /* unsigned, to detect overflow/wrap-around */
            /* If no PBO is bound, 'ptr' is a pointer to client memory containing
      'clientMemSize' bytes.
   If a PBO is bound, 'ptr' is an offset into the bound PBO.
      */
   if (!pack->BufferObj) {
      offset = 0;
      } else {
      offset = (uintptr_t)ptr;
   size = pack->BufferObj->Size;
   /* The ARB_pixel_buffer_object spec says:
   *    "INVALID_OPERATION is generated by ColorTable, ColorSubTable,
   *    ConvolutionFilter2D, ConvolutionFilter1D, SeparableFilter2D,
   *    TexImage1D, TexImage2D, TexImage3D, TexSubImage1D,
   *    TexSubImage2D, TexSubImage3D, and DrawPixels if the current
   *    PIXEL_UNPACK_BUFFER_BINDING_ARB value is non-zero and the data
   *    parameter is not evenly divisible into the number of basic machine
   *    units needed to store in memory a datum indicated by the type
   *    parameter."
   */
   if (type != GL_BITMAP &&
      (offset % _mesa_sizeof_packed_type(type)))
            if (size == 0)
      /* no buffer! */
         /* If the size of the image is zero then no pixels are accessed so we
   * don't need to check anything else.
   */
   if (width == 0 || height == 0 || depth == 0)
            /* get the offset to the first pixel we'll read/write */
   start = _mesa_image_offset(dimensions, pack, width, height,
            /* get the offset to just past the last pixel we'll read/write */
   end =  _mesa_image_offset(dimensions, pack, width, height,
            start += offset;
            if (start > size) {
      /* This will catch negative values / wrap-around */
      }
   if (end > size) {
      /* Image read/write goes beyond end of buffer */
               /* OK! */
      }
         /**
   * For commands that read from a PBO (glDrawPixels, glTexImage,
   * glPolygonStipple, etc), if we're reading from a PBO, map it read-only
   * and return the pointer into the PBO.  If we're not reading from a
   * PBO, return \p src as-is.
   * If non-null return, must call _mesa_unmap_pbo_source() when done.
   *
   * \return NULL if error, else pointer to start of data
   */
   const GLvoid *
   _mesa_map_pbo_source(struct gl_context *ctx,
               {
               if (unpack->BufferObj) {
      /* unpack from PBO */
   buf = (GLubyte *) _mesa_bufferobj_map_range(ctx, 0,
                           if (!buf)
               }
   else {
      /* unpack from normal memory */
                  }
      /**
   * Perform PBO validation for read operations with uncompressed textures.
   * If any GL errors are detected, false is returned, otherwise returns true.
   * \sa _mesa_validate_pbo_access
   */
   bool
   _mesa_validate_pbo_source(struct gl_context *ctx, GLuint dimensions,
                           const struct gl_pixelstore_attrib *unpack,
   {
               if (!_mesa_validate_pbo_access(dimensions, unpack, width, height, depth,
            if (unpack->BufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            } else {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            }
               if (!unpack->BufferObj) {
      /* non-PBO access: no further validation to be done */
               if (_mesa_check_disallowed_mapping(unpack->BufferObj)) {
      /* buffer is already mapped - that's an error */
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s(PBO is mapped)",
                        }
      /**
   * Perform PBO validation for read operations with compressed textures.
   * If any GL errors are detected, false is returned, otherwise returns true.
   */
   bool
   _mesa_validate_pbo_source_compressed(struct gl_context *ctx, GLuint dimensions,
                     {
      if (!unpack->BufferObj) {
      /* not using a PBO */
               if ((const GLubyte *) pixels + imageSize >
      ((const GLubyte *) 0) + unpack->BufferObj->Size) {
   /* out of bounds read! */
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s(invalid PBO access)",
                     if (_mesa_check_disallowed_mapping(unpack->BufferObj)) {
      /* buffer is already mapped - that's an error */
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s(PBO is mapped)",
                        }
      /**
   * Perform PBO-read mapping.
   * If any GL errors are detected, they'll be recorded and NULL returned.
   * \sa _mesa_validate_pbo_source
   * \sa _mesa_map_pbo_source
   * A call to this function should have a matching call to
   * _mesa_unmap_pbo_source().
   */
   const GLvoid *
   _mesa_map_validate_pbo_source(struct gl_context *ctx,
                                 GLuint dimensions,
   {
      if (!_mesa_validate_pbo_source(ctx, dimensions, unpack,
               return NULL;
            ptr = _mesa_map_pbo_source(ctx, unpack, ptr);
      }
         /**
   * Counterpart to _mesa_map_pbo_source()
   */
   void
   _mesa_unmap_pbo_source(struct gl_context *ctx,
         {
      assert(unpack != &ctx->Pack); /* catch pack/unpack mismatch */
   if (unpack->BufferObj) {
            }
         /**
   * For commands that write to a PBO (glReadPixels, glGetColorTable, etc),
   * if we're writing to a PBO, map it write-only and return the pointer
   * into the PBO.  If we're not writing to a PBO, return \p dst as-is.
   * If non-null return, must call _mesa_unmap_pbo_dest() when done.
   *
   * \return NULL if error, else pointer to start of data
   */
   void *
   _mesa_map_pbo_dest(struct gl_context *ctx,
               {
               if (pack->BufferObj) {
      /* pack into PBO */
   buf = (GLubyte *) _mesa_bufferobj_map_range(ctx, 0,
                           if (!buf)
               }
   else {
      /* pack to normal memory */
                  }
         /**
   * Combine PBO-write validation and mapping.
   * If any GL errors are detected, they'll be recorded and NULL returned.
   * \sa _mesa_validate_pbo_access
   * \sa _mesa_map_pbo_dest
   * A call to this function should have a matching call to
   * _mesa_unmap_pbo_dest().
   */
   GLvoid *
   _mesa_map_validate_pbo_dest(struct gl_context *ctx,
                                 {
               if (!_mesa_validate_pbo_access(dimensions, unpack, width, height, depth,
            if (unpack->BufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
      } else {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            }
               if (!unpack->BufferObj) {
      /* non-PBO access: no further validation to be done */
               if (_mesa_check_disallowed_mapping(unpack->BufferObj)) {
      /* buffer is already mapped - that's an error */
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s(PBO is mapped)", where);
               ptr = _mesa_map_pbo_dest(ctx, unpack, ptr);
      }
         /**
   * Counterpart to _mesa_map_pbo_dest()
   */
   void
   _mesa_unmap_pbo_dest(struct gl_context *ctx,
         {
      assert(pack != &ctx->Unpack); /* catch pack/unpack mismatch */
   if (pack->BufferObj) {
            }
         /**
   * Check if an unpack PBO is active prior to fetching a texture image.
   * If so, do bounds checking and map the buffer into main memory.
   * Any errors detected will be recorded.
   * The caller _must_ call _mesa_unmap_teximage_pbo() too!
   */
   const GLvoid *
   _mesa_validate_pbo_teximage(struct gl_context *ctx, GLuint dimensions,
         GLsizei width, GLsizei height, GLsizei depth,
   GLenum format, GLenum type, const GLvoid *pixels,
   const struct gl_pixelstore_attrib *unpack,
   {
               if (!unpack->BufferObj) {
      /* no PBO */
      }
   if (!_mesa_validate_pbo_access(dimensions, unpack, width, height, depth,
            _mesa_error(ctx, GL_INVALID_OPERATION, "%s%uD(invalid PBO access)",
                     buf = (GLubyte *) _mesa_bufferobj_map_range(ctx, 0,
                           if (!buf) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s%uD(PBO is mapped)", funcName,
                        }
         /**
   * Check if an unpack PBO is active prior to fetching a compressed texture
   * image.
   * If so, do bounds checking and map the buffer into main memory.
   * Any errors detected will be recorded.
   * The caller _must_ call _mesa_unmap_teximage_pbo() too!
   */
   const GLvoid *
   _mesa_validate_pbo_compressed_teximage(struct gl_context *ctx,
                           {
               if (!_mesa_validate_pbo_source_compressed(ctx, dimensions, packing,
         /* error is already set during validation */
                  if (!packing->BufferObj) {
      /* not using a PBO - return pointer unchanged */
               buf = (GLubyte*) _mesa_bufferobj_map_range(ctx, 0,
                              /* Validation above already checked that PBO is not mapped, so buffer
   * should not be null.
   */
               }
         /**
   * This function must be called after either of the validate_pbo_*_teximage()
   * functions.  It unmaps the PBO buffer if it was mapped earlier.
   */
   void
   _mesa_unmap_teximage_pbo(struct gl_context *ctx,
         {
      if (unpack->BufferObj) {
            }
