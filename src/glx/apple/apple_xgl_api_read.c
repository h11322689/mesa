   /*
   Copyright (c) 2008-2011 Apple Inc.
      Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation files
   (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:
      The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
   HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
      Except as contained in this notice, the name(s) of the above
   copyright holders shall not be used in advertising or otherwise to
   promote the sale, use or other dealings in this Software without
   prior written authorization.
   */
      /*
   * This file works with the glXMakeContextCurrent readable drawable. 
   *
   * The way it works is by swapping the currentDrawable for the currentReadable
   * drawable if they are different.
   */
   #include <stdbool.h>
   #include "glxclient.h"
   #include "apple_glx_context.h"
   #include "apple_xgl_api.h"
   #include "glapitable.h"
      extern struct _glapi_table * __ogl_framework_api;
      struct apple_xgl_saved_state
   {
         };
      static void
   SetRead(struct apple_xgl_saved_state *saved)
   {
               /*
   * By default indicate that the state was not swapped, so that UnsetRead
   * functions correctly.
   */
            /* 
   * If the readable drawable isn't the same as the drawable then 
   * the user has requested a readable drawable with glXMakeContextCurrent().
   * We emulate this behavior by switching the current drawable.
   */
   if (None != gc->currentReadable
      && gc->currentReadable != gc->currentDrawable) {
                     if (apple_glx_make_current_context(dpy, gc->driContext, gc->driContext,
            /* An error occurred, so try to restore the old context state. */
   (void) apple_glx_make_current_context(dpy, gc->driContext, gc->driContext,
                  }
      static void
   UnsetRead(struct apple_xgl_saved_state *saved)
   {
      if (saved->swapped) {
      struct glx_context *gc = __glXGetCurrentContext();
            if (apple_glx_make_current_context(dpy, gc->driContext, gc->driContext,
            /*
   * An error occurred restoring the drawable.
   * It's unclear what to do about that. 
            }
      void
   __applegl_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
         {
                                    }
      void
   __applegl_glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
   {
                                    }
      void
   __applegl_glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y,
         {
                                    }
