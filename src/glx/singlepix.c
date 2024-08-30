   /*
   * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
   * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
   *
   * SPDX-License-Identifier: SGI-B-2.0
   */
      #include "packsingle.h"
   #include "indirect.h"
   #include "glapi.h"
   #include <GL/glxproto.h>
      void
   __indirect_glGetSeparableFilter(GLenum target, GLenum format, GLenum type,
         {
      __GLX_SINGLE_DECLARE_VARIABLES();
   const __GLXattribute *state;
   xGLXGetSeparableFilterReply reply;
            if (!dpy)
         __GLX_SINGLE_LOAD_VARIABLES();
            /* Send request */
   __GLX_SINGLE_BEGIN(X_GLsop_GetSeparableFilter, __GLX_PAD(13));
   __GLX_SINGLE_PUT_LONG(0, target);
   __GLX_SINGLE_PUT_LONG(4, format);
   __GLX_SINGLE_PUT_LONG(8, type);
   __GLX_SINGLE_PUT_CHAR(12, state->storePack.swapEndian);
   __GLX_SINGLE_READ_XREPLY();
            if (compsize != 0) {
      GLint width, height;
            width = reply.width;
            widthsize = __glImageSize(width, 1, 1, format, type, 0);
            /* Allocate a holding buffer to transform the data from */
   rowBuf = malloc(widthsize);
   if (!rowBuf) {
      /* Throw data away */
   _XEatData(dpy, compsize);
   __glXSetError(gc, GL_OUT_OF_MEMORY);
   UnlockDisplay(dpy);
   SyncHandle();
      }
   else {
      __GLX_SINGLE_GET_CHAR_ARRAY(((char *) rowBuf), widthsize);
   __glEmptyImage(gc, 1, width, 1, 1, format, type, rowBuf, row);
      }
   colBuf = malloc(heightsize);
   if (!colBuf) {
      /* Throw data away */
   _XEatData(dpy, compsize - __GLX_PAD(widthsize));
   __glXSetError(gc, GL_OUT_OF_MEMORY);
   UnlockDisplay(dpy);
   SyncHandle();
      }
   else {
      __GLX_SINGLE_GET_CHAR_ARRAY(((char *) colBuf), heightsize);
   __glEmptyImage(gc, 1, height, 1, 1, format, type, colBuf, column);
         }
   else {
      /*
   ** don't modify user's buffer.
      }
         }
