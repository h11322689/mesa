   /*
   * (C) Copyright IBM Corporation 2005
   * All Rights Reserved.
   * 
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   * 
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   * 
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   * IBM,
   * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
   * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <inttypes.h>
   #include <GL/gl.h>
   #include "indirect.h"
   #include "glxclient.h"
   #include "indirect_vertex_array.h"
   #include <GL/glxproto.h>
      #if !defined(__GNUC__)
   #  define __builtin_expect(x, y) x
   #endif
      static void
   do_vertex_attrib_enable(GLuint index, GLboolean val)
   {
      struct glx_context *gc = __glXGetCurrentContext();
            if (!__glXSetArrayEnable(state, GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB,
                  }
         void
   __indirect_glEnableVertexAttribArray(GLuint index)
   {
         }
         void
   __indirect_glDisableVertexAttribArray(GLuint index)
   {
         }
         static void
   get_parameter(unsigned opcode, unsigned size, GLenum target, GLuint index,
         {
      struct glx_context *const gc = __glXGetCurrentContext();
   Display *const dpy = gc->currentDpy;
            if (__builtin_expect(dpy != NULL, 1)) {
      GLubyte const *pc = __glXSetupVendorRequest(gc,
                  *((GLenum *) (pc + 0)) = target;
   *((GLuint *) (pc + 4)) = index;
            (void) __glXReadReply(dpy, size, params, GL_FALSE);
   UnlockDisplay(dpy);
      }
      }
         void
   __indirect_glGetProgramEnvParameterfvARB(GLenum target, GLuint index,
         {
         }
         void
   __indirect_glGetProgramEnvParameterdvARB(GLenum target, GLuint index,
         {
         }
         void
   __indirect_glGetProgramLocalParameterfvARB(GLenum target, GLuint index,
         {
         }
         void
   __indirect_glGetProgramLocalParameterdvARB(GLenum target, GLuint index,
         {
         }
         void
   __indirect_glGetVertexAttribPointerv(GLuint index, GLenum pname,
         {
      struct glx_context *const gc = __glXGetCurrentContext();
            if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB) {
                  if (!__glXGetArrayPointer(state, GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB,
                  }
         /**
   * Get the selected attribute from the vertex array state vector.
   * 
   * \returns
   * On success \c GL_TRUE is returned.  Otherwise, \c GL_FALSE is returned.
   */
   static GLboolean
   get_attrib_array_data(__GLXattribute * state, GLuint index, GLenum cap,
         {
      GLboolean retval = GL_FALSE;
            switch (cap) {
   case GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB:
      retval = __glXGetArrayEnable(state, attrib, index, data);
         case GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB:
      retval = __glXGetArraySize(state, attrib, index, data);
         case GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB:
      retval = __glXGetArrayStride(state, attrib, index, data);
         case GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB:
      retval = __glXGetArrayType(state, attrib, index, data);
         case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB:
      retval = __glXGetArrayNormalized(state, attrib, index, data);
                     }
         static void
   get_vertex_attrib(struct glx_context * gc, unsigned vop,
         {
      Display *const dpy = gc->currentDpy;
   GLubyte *const pc = __glXSetupVendorRequest(gc,
                  *((uint32_t *) (pc + 0)) = index;
               }
         void
   __indirect_glGetVertexAttribiv(GLuint index, GLenum pname, GLint * params)
   {
      struct glx_context *const gc = __glXGetCurrentContext();
   Display *const dpy = gc->currentDpy;
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
                        if (reply.size != 0) {
                  if (get_attrib_array_data(state, index, pname, &data)) {
         }
   else {
      if (reply.size == 1) {
         }
   else {
                        UnlockDisplay(dpy);
      }
         void
   __indirect_glGetVertexAttribfv(GLuint index, GLenum pname,
         {
      struct glx_context *const gc = __glXGetCurrentContext();
   Display *const dpy = gc->currentDpy;
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
                        if (reply.size != 0) {
                  if (get_attrib_array_data(state, index, pname, &data)) {
         }
   else {
      if (reply.size == 1) {
         }
   else {
                        UnlockDisplay(dpy);
      }
         void
   __indirect_glGetVertexAttribdv(GLuint index, GLenum pname,
         {
      struct glx_context *const gc = __glXGetCurrentContext();
   Display *const dpy = gc->currentDpy;
   __GLXattribute *state = (__GLXattribute *) (gc->client_state_private);
                        if (reply.size != 0) {
                  if (get_attrib_array_data(state, index, pname, &data)) {
         }
   else {
      if (reply.size == 1) {
         }
   else {
                        UnlockDisplay(dpy);
      }
