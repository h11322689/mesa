   /*
   * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
   * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
   *
   * SPDX-License-Identifier: SGI-B-2.0
   */
      #include <stdio.h>
   #include <assert.h>
   #include "glxclient.h"
   #include "packsingle.h"
   #include "glxextensions.h"
   #include "indirect.h"
   #include "indirect_vertex_array.h"
   #include "glapi.h"
   #include <xcb/xcb.h>
   #include <xcb/glx.h>
   #include <X11/Xlib-xcb.h>
      #if !defined(__GNUC__)
   #  define __builtin_expect(x, y) x
   #endif
      /* Used for GL_ARB_transpose_matrix */
   static void
   TransposeMatrixf(GLfloat m[16])
   {
      int i, j;
   for (i = 0; i < 4; i++) {
      for (j = 0; j < i; j++) {
      GLfloat tmp = m[i * 4 + j];
   m[i * 4 + j] = m[j * 4 + i];
            }
      /* Used for GL_ARB_transpose_matrix */
   static void
   TransposeMatrixb(GLboolean m[16])
   {
      int i, j;
   for (i = 0; i < 4; i++) {
      for (j = 0; j < i; j++) {
      GLboolean tmp = m[i * 4 + j];
   m[i * 4 + j] = m[j * 4 + i];
            }
      /* Used for GL_ARB_transpose_matrix */
   static void
   TransposeMatrixd(GLdouble m[16])
   {
      int i, j;
   for (i = 0; i < 4; i++) {
      for (j = 0; j < i; j++) {
      GLdouble tmp = m[i * 4 + j];
   m[i * 4 + j] = m[j * 4 + i];
            }
      /* Used for GL_ARB_transpose_matrix */
   static void
   TransposeMatrixi(GLint m[16])
   {
      int i, j;
   for (i = 0; i < 4; i++) {
      for (j = 0; j < i; j++) {
      GLint tmp = m[i * 4 + j];
   m[i * 4 + j] = m[j * 4 + i];
            }
         /**
   * Remap a transpose-matrix enum to a non-transpose-matrix enum.  Enums
   * that are not transpose-matrix enums are unaffected.
   */
   static GLenum
   RemapTransposeEnum(GLenum e)
   {
      switch (e) {
   case GL_TRANSPOSE_MODELVIEW_MATRIX:
   case GL_TRANSPOSE_PROJECTION_MATRIX:
   case GL_TRANSPOSE_TEXTURE_MATRIX:
         case GL_TRANSPOSE_COLOR_MATRIX:
         default:
            }
         GLenum
   __indirect_glGetError(void)
   {
      __GLX_SINGLE_DECLARE_VARIABLES();
   GLuint retval = GL_NO_ERROR;
            if (gc->error) {
      /* Use internal error first */
   retval = gc->error;
   gc->error = GL_NO_ERROR;
               __GLX_SINGLE_LOAD_VARIABLES();
   __GLX_SINGLE_BEGIN(X_GLsop_GetError, 0);
   __GLX_SINGLE_READ_XREPLY();
   retval = reply.error;
               }
         /**
   * Get the selected attribute from the client state.
   *
   * \returns
   * On success \c GL_TRUE is returned.  Otherwise, \c GL_FALSE is returned.
   */
   static GLboolean
   get_client_data(struct glx_context * gc, GLenum cap, GLintptr * data)
   {
      GLboolean retval = GL_TRUE;
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
               switch (cap) {
   case GL_VERTEX_ARRAY:
   case GL_NORMAL_ARRAY:
   case GL_COLOR_ARRAY:
   case GL_INDEX_ARRAY:
   case GL_EDGE_FLAG_ARRAY:
   case GL_SECONDARY_COLOR_ARRAY:
   case GL_FOG_COORD_ARRAY:
      retval = __glXGetArrayEnable(state, cap, 0, data);
         case GL_VERTEX_ARRAY_SIZE:
      retval = __glXGetArraySize(state, GL_VERTEX_ARRAY, 0, data);
      case GL_COLOR_ARRAY_SIZE:
      retval = __glXGetArraySize(state, GL_COLOR_ARRAY, 0, data);
      case GL_SECONDARY_COLOR_ARRAY_SIZE:
      retval = __glXGetArraySize(state, GL_SECONDARY_COLOR_ARRAY, 0, data);
         case GL_VERTEX_ARRAY_TYPE:
      retval = __glXGetArrayType(state, GL_VERTEX_ARRAY, 0, data);
      case GL_NORMAL_ARRAY_TYPE:
      retval = __glXGetArrayType(state, GL_NORMAL_ARRAY, 0, data);
      case GL_INDEX_ARRAY_TYPE:
      retval = __glXGetArrayType(state, GL_INDEX_ARRAY, 0, data);
      case GL_COLOR_ARRAY_TYPE:
      retval = __glXGetArrayType(state, GL_COLOR_ARRAY, 0, data);
      case GL_SECONDARY_COLOR_ARRAY_TYPE:
      retval = __glXGetArrayType(state, GL_SECONDARY_COLOR_ARRAY, 0, data);
      case GL_FOG_COORD_ARRAY_TYPE:
      retval = __glXGetArrayType(state, GL_FOG_COORD_ARRAY, 0, data);
         case GL_VERTEX_ARRAY_STRIDE:
      retval = __glXGetArrayStride(state, GL_VERTEX_ARRAY, 0, data);
      case GL_NORMAL_ARRAY_STRIDE:
      retval = __glXGetArrayStride(state, GL_NORMAL_ARRAY, 0, data);
      case GL_INDEX_ARRAY_STRIDE:
      retval = __glXGetArrayStride(state, GL_INDEX_ARRAY, 0, data);
      case GL_EDGE_FLAG_ARRAY_STRIDE:
      retval = __glXGetArrayStride(state, GL_EDGE_FLAG_ARRAY, 0, data);
      case GL_COLOR_ARRAY_STRIDE:
      retval = __glXGetArrayStride(state, GL_COLOR_ARRAY, 0, data);
      case GL_SECONDARY_COLOR_ARRAY_STRIDE:
      retval = __glXGetArrayStride(state, GL_SECONDARY_COLOR_ARRAY, 0, data);
      case GL_FOG_COORD_ARRAY_STRIDE:
      retval = __glXGetArrayStride(state, GL_FOG_COORD_ARRAY, 0, data);
         case GL_TEXTURE_COORD_ARRAY:
      retval =
            case GL_TEXTURE_COORD_ARRAY_SIZE:
      retval =
            case GL_TEXTURE_COORD_ARRAY_TYPE:
      retval =
            case GL_TEXTURE_COORD_ARRAY_STRIDE:
      retval =
               case GL_MAX_ELEMENTS_VERTICES:
   case GL_MAX_ELEMENTS_INDICES:
      retval = GL_TRUE;
   *data = ~0UL;
            case GL_PACK_ROW_LENGTH:
      *data = (GLintptr) state->storePack.rowLength;
      case GL_PACK_IMAGE_HEIGHT:
      *data = (GLintptr) state->storePack.imageHeight;
      case GL_PACK_SKIP_ROWS:
      *data = (GLintptr) state->storePack.skipRows;
      case GL_PACK_SKIP_PIXELS:
      *data = (GLintptr) state->storePack.skipPixels;
      case GL_PACK_SKIP_IMAGES:
      *data = (GLintptr) state->storePack.skipImages;
      case GL_PACK_ALIGNMENT:
      *data = (GLintptr) state->storePack.alignment;
      case GL_PACK_SWAP_BYTES:
      *data = (GLintptr) state->storePack.swapEndian;
      case GL_PACK_LSB_FIRST:
      *data = (GLintptr) state->storePack.lsbFirst;
      case GL_UNPACK_ROW_LENGTH:
      *data = (GLintptr) state->storeUnpack.rowLength;
      case GL_UNPACK_IMAGE_HEIGHT:
      *data = (GLintptr) state->storeUnpack.imageHeight;
      case GL_UNPACK_SKIP_ROWS:
      *data = (GLintptr) state->storeUnpack.skipRows;
      case GL_UNPACK_SKIP_PIXELS:
      *data = (GLintptr) state->storeUnpack.skipPixels;
      case GL_UNPACK_SKIP_IMAGES:
      *data = (GLintptr) state->storeUnpack.skipImages;
      case GL_UNPACK_ALIGNMENT:
      *data = (GLintptr) state->storeUnpack.alignment;
      case GL_UNPACK_SWAP_BYTES:
      *data = (GLintptr) state->storeUnpack.swapEndian;
      case GL_UNPACK_LSB_FIRST:
      *data = (GLintptr) state->storeUnpack.lsbFirst;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
      *data = (GLintptr) (gc->attributes.stackPointer - gc->attributes.stack);
      case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
      *data = (GLintptr) __GL_CLIENT_ATTRIB_STACK_DEPTH;
      case GL_CLIENT_ACTIVE_TEXTURE:
      *data = (GLintptr) (tex_unit + GL_TEXTURE0);
         default:
      retval = GL_FALSE;
                     }
         void
   __indirect_glGetBooleanv(GLenum val, GLboolean * b)
   {
      const GLenum origVal = val;
   __GLX_SINGLE_DECLARE_VARIABLES();
                     __GLX_SINGLE_LOAD_VARIABLES();
   __GLX_SINGLE_BEGIN(X_GLsop_GetBooleanv, 4);
   __GLX_SINGLE_PUT_LONG(0, val);
   __GLX_SINGLE_READ_XREPLY();
            if (compsize == 0) {
      /*
   ** Error occurred; don't modify user's buffer.
      }
   else {
               /*
   ** We still needed to send the request to the server in order to
   ** find out whether it was legal to make a query (it's illegal,
   ** for example, to call a query between glBegin() and glEnd()).
            if (get_client_data(gc, val, &data)) {
         }
   else {
      /*
   ** Not a local value, so use what we got from the server.
   */
   if (compsize == 1) {
         }
   else {
      __GLX_SINGLE_GET_CHAR_ARRAY(b, compsize);
   if (val != origVal) {
      /* matrix transpose */
               }
      }
      void
   __indirect_glGetDoublev(GLenum val, GLdouble * d)
   {
      const GLenum origVal = val;
   __GLX_SINGLE_DECLARE_VARIABLES();
                     __GLX_SINGLE_LOAD_VARIABLES();
   __GLX_SINGLE_BEGIN(X_GLsop_GetDoublev, 4);
   __GLX_SINGLE_PUT_LONG(0, val);
   __GLX_SINGLE_READ_XREPLY();
            if (compsize == 0) {
      /*
   ** Error occurred; don't modify user's buffer.
      }
   else {
               /*
   ** We still needed to send the request to the server in order to
   ** find out whether it was legal to make a query (it's illegal,
   ** for example, to call a query between glBegin() and glEnd()).
            if (get_client_data(gc, val, &data)) {
         }
   else {
      /*
   ** Not a local value, so use what we got from the server.
   */
   if (compsize == 1) {
         }
   else {
      __GLX_SINGLE_GET_DOUBLE_ARRAY(d, compsize);
   if (val != origVal) {
      /* matrix transpose */
               }
      }
      void
   __indirect_glGetFloatv(GLenum val, GLfloat * f)
   {
      const GLenum origVal = val;
   __GLX_SINGLE_DECLARE_VARIABLES();
                     __GLX_SINGLE_LOAD_VARIABLES();
   __GLX_SINGLE_BEGIN(X_GLsop_GetFloatv, 4);
   __GLX_SINGLE_PUT_LONG(0, val);
   __GLX_SINGLE_READ_XREPLY();
            if (compsize == 0) {
      /*
   ** Error occurred; don't modify user's buffer.
      }
   else {
               /*
   ** We still needed to send the request to the server in order to
   ** find out whether it was legal to make a query (it's illegal,
   ** for example, to call a query between glBegin() and glEnd()).
            if (get_client_data(gc, val, &data)) {
         }
   else {
      /*
   ** Not a local value, so use what we got from the server.
   */
   if (compsize == 1) {
         }
   else {
      __GLX_SINGLE_GET_FLOAT_ARRAY(f, compsize);
   if (val != origVal) {
      /* matrix transpose */
               }
      }
      void
   __indirect_glGetIntegerv(GLenum val, GLint * i)
   {
      const GLenum origVal = val;
   __GLX_SINGLE_DECLARE_VARIABLES();
                     __GLX_SINGLE_LOAD_VARIABLES();
   __GLX_SINGLE_BEGIN(X_GLsop_GetIntegerv, 4);
   __GLX_SINGLE_PUT_LONG(0, val);
   __GLX_SINGLE_READ_XREPLY();
            if (compsize == 0) {
      /*
   ** Error occurred; don't modify user's buffer.
      }
   else {
               /*
   ** We still needed to send the request to the server in order to
   ** find out whether it was legal to make a query (it's illegal,
   ** for example, to call a query between glBegin() and glEnd()).
            if (get_client_data(gc, val, &data)) {
         }
   else {
      /*
   ** Not a local value, so use what we got from the server.
   */
   if (compsize == 1) {
         }
   else {
      __GLX_SINGLE_GET_LONG_ARRAY(i, compsize);
   if (val != origVal) {
      /* matrix transpose */
               }
      }
      /*
   ** Send all pending commands to server.
   */
   void
   __indirect_glFlush(void)
   {
               if (!dpy)
            __GLX_SINGLE_LOAD_VARIABLES();
   __GLX_SINGLE_BEGIN(X_GLsop_Flush, 0);
            /* And finally flush the X protocol data */
      }
      void
   __indirect_glFeedbackBuffer(GLsizei size, GLenum type, GLfloat * buffer)
   {
               if (!dpy)
            __GLX_SINGLE_LOAD_VARIABLES();
   __GLX_SINGLE_BEGIN(X_GLsop_FeedbackBuffer, 8);
   __GLX_SINGLE_PUT_LONG(0, size);
   __GLX_SINGLE_PUT_LONG(4, type);
               }
      void
   __indirect_glSelectBuffer(GLsizei numnames, GLuint * buffer)
   {
               if (!dpy)
            __GLX_SINGLE_LOAD_VARIABLES();
   __GLX_SINGLE_BEGIN(X_GLsop_SelectBuffer, 4);
   __GLX_SINGLE_PUT_LONG(0, numnames);
               }
      GLint
   __indirect_glRenderMode(GLenum mode)
   {
      __GLX_SINGLE_DECLARE_VARIABLES();
   GLint retval = 0;
            if (!dpy)
            __GLX_SINGLE_LOAD_VARIABLES();
   __GLX_SINGLE_BEGIN(X_GLsop_RenderMode, 4);
   __GLX_SINGLE_PUT_LONG(0, mode);
   __GLX_SINGLE_READ_XREPLY();
            if (reply.newMode != mode) {
      /*
   ** Switch to new mode did not take effect, therefore an error
   ** occurred.  When an error happens the server won't send us any
   ** other data.
      }
   else {
      /* Read the feedback or selection data */
   if (gc->renderMode == GL_FEEDBACK) {
      __GLX_SINGLE_GET_SIZE(compsize);
      }
   else if (gc->renderMode == GL_SELECT) {
      __GLX_SINGLE_GET_SIZE(compsize);
      }
      }
               }
      void
   __indirect_glFinish(void)
   {
      __GLX_SINGLE_DECLARE_VARIABLES();
            __GLX_SINGLE_LOAD_VARIABLES();
   __GLX_SINGLE_BEGIN(X_GLsop_Finish, 0);
   __GLX_SINGLE_READ_XREPLY();
      }
         /**
   * Extract the major and minor version numbers from a version string.
   */
   static void
   version_from_string(const char *ver, int *major_version, int *minor_version)
   {
      const char *end;
   long major;
            major = strtol(ver, (char **) &end, 10);
   minor = strtol(end + 1, NULL, 10);
   *major_version = major;
      }
         const GLubyte *
   __indirect_glGetString(GLenum name)
   {
      struct glx_context *gc = __glXGetCurrentContext();
   Display *dpy = gc->currentDpy;
            if (!dpy)
            /*
   ** Return the cached copy if the string has already been fetched
   */
   switch (name) {
   case GL_VENDOR:
      if (gc->vendor)
            case GL_RENDERER:
      if (gc->renderer)
            case GL_VERSION:
      if (gc->version)
            case GL_EXTENSIONS:
      if (gc->extensions)
            default:
      __glXSetError(gc, GL_INVALID_ENUM);
               /*
   ** Get requested string from server
            (void) __glXFlushRenderBuffer(gc, gc->pc);
   s = (GLubyte *) __glXGetString(dpy, gc->currentContextTag, name);
   if (!s) {
      /* Throw data on the floor */
      }
   else {
      /*
   ** Update local cache
   */
   switch (name) {
   case GL_VENDOR:
                  case GL_RENDERER:
                  case GL_VERSION:{
                                       if ((gc->server_major < client_major)
      || ((gc->server_major == client_major)
            }
   else {
      /* Allow 7 bytes for the client-side GL version.  This allows
   * for upto version 999.999.  I'm not holding my breath for
   * that one!  The extra 4 is for the ' ()\0' that will be
                        gc->version = malloc(size);
   if (gc->version == NULL) {
      /* If we couldn't allocate memory for the new string,
   * make a best-effort and just copy the client-side version
   * to the string and use that.  It probably doesn't
   * matter what is done here.  If there not memory available
   * for a short string, the system is probably going to die
   * soon anyway.
   */
   snprintf((char *) s, strlen((char *) s) + 1, "%u.%u",
            }
   else {
      snprintf((char *) gc->version, size, "%u.%u (%s)",
         free(s);
         }
            case GL_EXTENSIONS:{
         __glXCalculateUsableGLExtensions(gc, (char *) s);
   free(s);
   s = gc->extensions;
            }
      }
      GLboolean
   __indirect_glIsEnabled(GLenum cap)
   {
      __GLX_SINGLE_DECLARE_VARIABLES();
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
   xGLXSingleReply reply;
   GLboolean retval = 0;
            if (!dpy)
            switch (cap) {
   case GL_VERTEX_ARRAY:
   case GL_NORMAL_ARRAY:
   case GL_COLOR_ARRAY:
   case GL_INDEX_ARRAY:
   case GL_EDGE_FLAG_ARRAY:
   case GL_SECONDARY_COLOR_ARRAY:
   case GL_FOG_COORD_ARRAY:
      retval = __glXGetArrayEnable(state, cap, 0, &enable);
   assert(retval);
   return (GLboolean) enable;
      case GL_TEXTURE_COORD_ARRAY:
      retval = __glXGetArrayEnable(state, GL_TEXTURE_COORD_ARRAY,
         assert(retval);
   return (GLboolean) enable;
               __GLX_SINGLE_LOAD_VARIABLES();
   __GLX_SINGLE_BEGIN(X_GLsop_IsEnabled, 4);
   __GLX_SINGLE_PUT_LONG(0, cap);
   __GLX_SINGLE_READ_XREPLY();
   __GLX_SINGLE_GET_RETVAL(retval, GLboolean);
   __GLX_SINGLE_END();
      }
      void
   __indirect_glGetPointerv(GLenum pname, void **params)
   {
      struct glx_context *gc = __glXGetCurrentContext();
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
            if (!dpy)
            switch (pname) {
   case GL_VERTEX_ARRAY_POINTER:
   case GL_NORMAL_ARRAY_POINTER:
   case GL_COLOR_ARRAY_POINTER:
   case GL_INDEX_ARRAY_POINTER:
   case GL_EDGE_FLAG_ARRAY_POINTER:
      __glXGetArrayPointer(state, pname - GL_VERTEX_ARRAY_POINTER
            case GL_TEXTURE_COORD_ARRAY_POINTER:
      __glXGetArrayPointer(state, GL_TEXTURE_COORD_ARRAY,
            case GL_SECONDARY_COLOR_ARRAY_POINTER:
   case GL_FOG_COORD_ARRAY_POINTER:
      __glXGetArrayPointer(state, pname - GL_FOG_COORD_ARRAY_POINTER
            case GL_FEEDBACK_BUFFER_POINTER:
      *params = (void *) gc->feedbackBuf;
      case GL_SELECTION_BUFFER_POINTER:
      *params = (void *) gc->selectBuf;
      default:
      __glXSetError(gc, GL_INVALID_ENUM);
         }
            /**
   * This was previously auto-generated, but we need to special-case
   * how we handle writing into the 'residences' buffer when n%4!=0.
   */
   #define X_GLsop_AreTexturesResident 143
   GLboolean
   __indirect_glAreTexturesResident(GLsizei n, const GLuint * textures,
         {
      struct glx_context *const gc = __glXGetCurrentContext();
   Display *const dpy = gc->currentDpy;
   GLboolean retval = (GLboolean) 0;
   if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
      xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_glx_are_textures_resident_reply_t *reply;
   (void) __glXFlushRenderBuffer(gc, gc->pc);
   reply =
      xcb_glx_are_textures_resident_reply(c,
                  (void) memcpy(residences, xcb_glx_are_textures_resident_data(reply),
               retval = reply->ret_val;
      }
      }
         /**
   * This was previously auto-generated, but we need to special-case
   * how we handle writing into the 'residences' buffer when n%4!=0.
   */
   #define X_GLvop_AreTexturesResidentEXT 11
   GLboolean
   glAreTexturesResidentEXT(GLsizei n, const GLuint * textures,
         {
               if (gc->isDirect) {
      const _glapi_proc *const table = (_glapi_proc *) GET_DISPATCH();
   PFNGLARETEXTURESRESIDENTEXTPROC p =
               }
   else {
      struct glx_context *const gc = __glXGetCurrentContext();
   Display *const dpy = gc->currentDpy;
   GLboolean retval = (GLboolean) 0;
   const GLuint cmdlen = 4 + __GLX_PAD((n * 4));
   if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
      GLubyte const *pc =
      __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply,
            (void) memcpy((void *) (pc + 0), (void *) (&n), 4);
   (void) memcpy((void *) (pc + 4), (void *) (textures), (n * 4));
   if (n & 3) {
      /* see comments in __indirect_glAreTexturesResident() */
   GLboolean *res4 = malloc((n + 3) & ~3);
   retval = (GLboolean) __glXReadReply(dpy, 1, res4, GL_TRUE);
   memcpy(residences, res4, n);
      }
   else {
         }
   UnlockDisplay(dpy);
      }
         }
