   /*
   Copyright (c) 2008, 2009 Apple Inc.
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
      #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <assert.h>
   #include <stdarg.h>
   #include <dlfcn.h>
   #include <pthread.h>
   #include <inttypes.h>
   #include "appledri.h"
   #include "apple_glx.h"
   #include "apple_glx_context.h"
   #include "apple_cgl.h"
      static bool initialized = false;
   static int dri_event_base = 0;
      int
   apple_get_dri_event_base(void)
   {
      if (!initialized) {
      fprintf(stderr,
            }
      }
      static void
   surface_notify_handler(Display * dpy, unsigned int uid, int kind)
   {
         switch (kind) {
   case AppleDRISurfaceNotifyDestroyed:
      apple_glx_diagnostic("%s: surface destroyed %u\n", __func__, uid);
   apple_glx_surface_destroy(uid);
         case AppleDRISurfaceNotifyChanged:{
                              }
         default:
            }
      xp_client_id
   apple_glx_get_client_id(void)
   {
               if (0 == id) {
      if ((XP_Success != xp_init(XP_IN_BACKGROUND)) ||
      (Success != xp_get_client_id(&id))) {
                     }
      /* Return true if an error occurred. */
   bool
   apple_init_glx(Display * dpy)
   {
      int eventBase, errorBase;
            if (!XAppleDRIQueryExtension(dpy, &eventBase, &errorBase))
            if (!XAppleDRIQueryVersion(dpy, &major, &minor, &patch))
            if (initialized)
                              apple_cgl_init();
                     /* This should really be per display. */
   dri_event_base = eventBase;
               }
      void
   apple_glx_swap_buffers(void *ptr)
   {
                  }
      void
   apple_glx_waitx(Display * dpy, void *ptr)
   {
                        glFlush();
   glFinish();
      }
