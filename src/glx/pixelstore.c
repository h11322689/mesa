   /*
   * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
   * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
   *
   * SPDX-License-Identifier: SGI-B-2.0
   */
      #include "glxclient.h"
   #include "indirect.h"
   #include "util/rounding.h"
      #if !defined(__GNUC__)
   #  define __builtin_expect(x, y) x
   #endif
      /**
   * Send glPixelStore command to the server
   * 
   * \param gc     Current GLX context
   * \param sop    Either \c X_GLsop_PixelStoref or \c X_GLsop_PixelStorei
   * \param pname  Selector of which pixel parameter is to be set.
   * \param param  Value that \c pname is set to.
   *
   * \sa __indirect_glPixelStorei,  __indirect_glPixelStoref
   */
   static void
   send_PixelStore(struct glx_context * gc, unsigned sop, GLenum pname,
         {
      Display *const dpy = gc->currentDpy;
   const GLuint cmdlen = 8;
   if (__builtin_expect(dpy != NULL, 1)) {
      GLubyte const *pc = __glXSetupSingleRequest(gc, sop, cmdlen);
   (void) memcpy((void *) (pc + 0), (void *) (&pname), 4);
   (void) memcpy((void *) (pc + 4), param, 4);
   UnlockDisplay(dpy);
      }
      }
      /*
   ** Specify parameters that control the storage format of pixel arrays.
   */
   void
   __indirect_glPixelStoref(GLenum pname, GLfloat param)
   {
      struct glx_context *gc = __glXGetCurrentContext();
   __GLXattribute *state = gc->client_state_private;
   Display *dpy = gc->currentDpy;
            if (!dpy)
            switch (pname) {
   case GL_PACK_ROW_LENGTH:
      a = _mesa_lroundevenf(param);
   if (((GLint) a) < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storePack.rowLength = a;
      case GL_PACK_IMAGE_HEIGHT:
      a = _mesa_lroundevenf(param);
   if (((GLint) a) < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storePack.imageHeight = a;
      case GL_PACK_SKIP_ROWS:
      a = _mesa_lroundevenf(param);
   if (((GLint) a) < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storePack.skipRows = a;
      case GL_PACK_SKIP_PIXELS:
      a = _mesa_lroundevenf(param);
   if (((GLint) a) < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storePack.skipPixels = a;
      case GL_PACK_SKIP_IMAGES:
      a = _mesa_lroundevenf(param);
   if (((GLint) a) < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storePack.skipImages = a;
      case GL_PACK_ALIGNMENT:
      a = _mesa_lroundevenf(param);
   switch (a) {
   case 1:
   case 2:
   case 4:
   case 8:
      state->storePack.alignment = a;
      default:
      __glXSetError(gc, GL_INVALID_VALUE);
      }
      case GL_PACK_SWAP_BYTES:
      state->storePack.swapEndian = (param != 0);
      case GL_PACK_LSB_FIRST:
      state->storePack.lsbFirst = (param != 0);
         case GL_UNPACK_ROW_LENGTH:
      a = _mesa_lroundevenf(param);
   if (((GLint) a) < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storeUnpack.rowLength = a;
      case GL_UNPACK_IMAGE_HEIGHT:
      a = _mesa_lroundevenf(param);
   if (((GLint) a) < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storeUnpack.imageHeight = a;
      case GL_UNPACK_SKIP_ROWS:
      a = _mesa_lroundevenf(param);
   if (((GLint) a) < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storeUnpack.skipRows = a;
      case GL_UNPACK_SKIP_PIXELS:
      a = _mesa_lroundevenf(param);
   if (((GLint) a) < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storeUnpack.skipPixels = a;
      case GL_UNPACK_SKIP_IMAGES:
      a = _mesa_lroundevenf(param);
   if (((GLint) a) < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storeUnpack.skipImages = a;
      case GL_UNPACK_ALIGNMENT:
      a = _mesa_lroundevenf(param);
   switch (a) {
   case 1:
   case 2:
   case 4:
   case 8:
      state->storeUnpack.alignment = a;
      default:
      __glXSetError(gc, GL_INVALID_VALUE);
      }
      case GL_UNPACK_SWAP_BYTES:
      state->storeUnpack.swapEndian = (param != 0);
      case GL_UNPACK_LSB_FIRST:
      state->storeUnpack.lsbFirst = (param != 0);
            /* Group all of the pixel store modes that need to be sent to the
   * server here.  Care must be used to only send modes to the server that
   * won't affect the size of the data sent to or received from the
   * server.  GL_PACK_INVERT_MESA is safe in this respect, but other,
   * future modes may not be.
      case GL_PACK_INVERT_MESA:
      send_PixelStore(gc, X_GLsop_PixelStoref, pname, &param);
         default:
      __glXSetError(gc, GL_INVALID_ENUM);
         }
      void
   __indirect_glPixelStorei(GLenum pname, GLint param)
   {
      struct glx_context *gc = __glXGetCurrentContext();
   __GLXattribute *state = gc->client_state_private;
            if (!dpy)
            switch (pname) {
   case GL_PACK_ROW_LENGTH:
      if (param < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storePack.rowLength = param;
      case GL_PACK_IMAGE_HEIGHT:
      if (param < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storePack.imageHeight = param;
      case GL_PACK_SKIP_ROWS:
      if (param < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storePack.skipRows = param;
      case GL_PACK_SKIP_PIXELS:
      if (param < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storePack.skipPixels = param;
      case GL_PACK_SKIP_IMAGES:
      if (param < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storePack.skipImages = param;
      case GL_PACK_ALIGNMENT:
      switch (param) {
   case 1:
   case 2:
   case 4:
   case 8:
      state->storePack.alignment = param;
      default:
      __glXSetError(gc, GL_INVALID_VALUE);
      }
      case GL_PACK_SWAP_BYTES:
      state->storePack.swapEndian = (param != 0);
      case GL_PACK_LSB_FIRST:
      state->storePack.lsbFirst = (param != 0);
         case GL_UNPACK_ROW_LENGTH:
      if (param < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storeUnpack.rowLength = param;
      case GL_UNPACK_IMAGE_HEIGHT:
      if (param < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storeUnpack.imageHeight = param;
      case GL_UNPACK_SKIP_ROWS:
      if (param < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storeUnpack.skipRows = param;
      case GL_UNPACK_SKIP_PIXELS:
      if (param < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storeUnpack.skipPixels = param;
      case GL_UNPACK_SKIP_IMAGES:
      if (param < 0) {
      __glXSetError(gc, GL_INVALID_VALUE);
      }
   state->storeUnpack.skipImages = param;
      case GL_UNPACK_ALIGNMENT:
      switch (param) {
   case 1:
   case 2:
   case 4:
   case 8:
      state->storeUnpack.alignment = param;
      default:
      __glXSetError(gc, GL_INVALID_VALUE);
      }
      case GL_UNPACK_SWAP_BYTES:
      state->storeUnpack.swapEndian = (param != 0);
      case GL_UNPACK_LSB_FIRST:
      state->storeUnpack.lsbFirst = (param != 0);
            /* Group all of the pixel store modes that need to be sent to the
   * server here.  Care must be used to only send modes to the server that
   * won't affect the size of the data sent to or received from the
   * server.  GL_PACK_INVERT_MESA is safe in this respect, but other,
   * future modes may not be.
      case GL_PACK_INVERT_MESA:
      send_PixelStore(gc, X_GLsop_PixelStorei, pname, &param);
         default:
      __glXSetError(gc, GL_INVALID_ENUM);
         }
