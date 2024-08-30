   /*
   Copyright (c) 2009 Apple Inc.
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
      /* Must be before OpenGL.framework is included.  Remove once fixed:
   * <rdar://problem/7872773>
   */
   #include <GL/gl.h>
   #include <GL/glext.h>
   #define __gltypes_h_ 1
      /* Must be first for:
   * <rdar://problem/6953344>
   */
   #include "apple_glx_context.h"
   #include "apple_glx_drawable.h"
      #include <stdbool.h>
   #include <stdlib.h>
   #include <pthread.h>
   #include <assert.h>
   #include "glxclient.h"
   #include "apple_glx.h"
   #include "glxconfig.h"
   #include "apple_cgl.h"
   #include "util/u_debug.h"
      /* mesa defines in glew.h, Apple in glext.h.
   * Due to namespace nightmares, just do it here.
   */
   #ifndef GL_TEXTURE_RECTANGLE_EXT
   #define GL_TEXTURE_RECTANGLE_EXT 0x84F5
   #endif
      static bool pbuffer_make_current(struct apple_glx_context *ac,
            static void pbuffer_destroy(Display * dpy, struct apple_glx_drawable *d);
      static struct apple_glx_drawable_callbacks callbacks = {
      .type = APPLE_GLX_DRAWABLE_PBUFFER,
   .make_current = pbuffer_make_current,
      };
         /* Return true if an error occurred. */
   bool
   pbuffer_make_current(struct apple_glx_context *ac,
         {
      struct apple_glx_pbuffer *pbuf = &d->types.pbuffer;
                              if (kCGLNoError != cglerr) {
      fprintf(stderr, "set_pbuffer: %s\n", apple_cgl.error_string(cglerr));
               if (!ac->made_current) {
      apple_glapi_oglfw_viewport_scissor(0, 0, pbuf->width, pbuf->height);
                           }
      void
   pbuffer_destroy(Display * dpy, struct apple_glx_drawable *d)
   {
                        apple_glx_diagnostic("destroying pbuffer for drawable 0x%lx\n",
            apple_cgl.destroy_pbuffer(pbuf->buffer_obj);
      }
      /* Return true if an error occurred. */
   bool
   apple_glx_pbuffer_destroy(Display * dpy, GLXPbuffer pbuf)
   {
      return !apple_glx_drawable_destroy_by_type(dpy, pbuf,
      }
      /* Return true if an error occurred. */
   bool
   apple_glx_pbuffer_create(Display * dpy, GLXFBConfig config,
               {
      struct apple_glx_drawable *d;
   struct apple_glx_pbuffer *pbuf = NULL;
   CGLError err;
   Window root;
   int screen;
   Pixmap xid;
            root = DefaultRootWindow(dpy);
            /*
   * This pixmap is only used for a persistent XID.
   * The XC-MISC extension cleans up XIDs and reuses them transparently,
   * so we need to retain a server-side reference.
   */
   xid = XCreatePixmap(dpy, root, (unsigned int) 1,
            if (None == xid) {
      *errorcode = BadAlloc;
               if (apple_glx_drawable_create(dpy, screen, xid, &d, &callbacks)) {
      *errorcode = BadAlloc;
               /* The lock is held in d from create onward. */
            pbuf->xid = xid;
   pbuf->width = width;
            err = apple_cgl.create_pbuffer(width, height, GL_TEXTURE_RECTANGLE_EXT,
                  if (kCGLNoError != err) {
      d->unlock(d);
   d->destroy(d);
   *errorcode = BadMatch;
                                                      }
            /* Return true if an error occurred. */
   static bool
   get_max_size(int *widthresult, int *heightresult)
   {
      CGLContextObj oldcontext;
                     if (!oldcontext) {
      /* 
   * There is no current context, so we need to make one in order
   * to call glGetInteger.
   */
   CGLPixelFormatObj pfobj;
   CGLError err;
   CGLPixelFormatAttribute attr[10];
   int c = 0;
   GLint vsref = 0;
            attr[c++] = kCGLPFAColorSize;
   attr[c++] = 32;
            err = apple_cgl.choose_pixel_format(attr, &pfobj, &vsref);
   if (kCGLNoError != err) {
                                             if (kCGLNoError != err) {
                                                   if (kCGLNoError != err) {
      DebugMessageF("set_current_context error in %s: %s\n", __func__,
                                 apple_cgl.set_current_context(oldcontext);
   apple_cgl.destroy_context(newcontext);
      }
   else {
                           *widthresult = ar[0];
               }
      bool
   apple_glx_pbuffer_query(GLXPbuffer p, int attr, unsigned int *value)
   {
      bool result = false;
   struct apple_glx_drawable *d;
            d = apple_glx_drawable_find_by_type(p, APPLE_GLX_DRAWABLE_PBUFFER,
            if (d) {
               switch (attr) {
   case GLX_WIDTH:
      *value = pbuf->width;
               case GLX_HEIGHT:
      *value = pbuf->height;
               case GLX_PRESERVED_CONTENTS:
      *value = true;
               case GLX_LARGEST_PBUFFER:{
         int width, height;
   if (get_max_size(&width, &height)) {
      fprintf(stderr, "internal error: "
      }
   else {
      *value = width;
                     case GLX_FBCONFIG_ID:
      *value = pbuf->fbconfigID;
   result = true;
                              }
      bool
   apple_glx_pbuffer_set_event_mask(GLXDrawable drawable, unsigned long mask)
   {
      struct apple_glx_drawable *d;
            d = apple_glx_drawable_find_by_type(drawable, APPLE_GLX_DRAWABLE_PBUFFER,
            if (d) {
      d->types.pbuffer.event_mask = mask;
   result = true;
                  }
      bool
   apple_glx_pbuffer_get_event_mask(GLXDrawable drawable, unsigned long *mask)
   {
      struct apple_glx_drawable *d;
            d = apple_glx_drawable_find_by_type(drawable, APPLE_GLX_DRAWABLE_PBUFFER,
         if (d) {
      *mask = d->types.pbuffer.event_mask;
   result = true;
                  }
