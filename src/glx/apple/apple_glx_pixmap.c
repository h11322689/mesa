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
      #include <stdio.h>
   #include <stdlib.h>
   #include <pthread.h>
   #include <fcntl.h>
   #include <sys/types.h>
   #include <sys/mman.h>
   #include <unistd.h>
   #include <assert.h>
   #include "apple_glx.h"
   #include "apple_cgl.h"
   #include "apple_visual.h"
   #include "apple_glx_drawable.h"
   #include "appledri.h"
   #include "glxconfig.h"
      static bool pixmap_make_current(struct apple_glx_context *ac,
            static void pixmap_destroy(Display * dpy, struct apple_glx_drawable *d);
      static struct apple_glx_drawable_callbacks callbacks = {
      .type = APPLE_GLX_DRAWABLE_PIXMAP,
   .make_current = pixmap_make_current,
      };
      static bool
   pixmap_make_current(struct apple_glx_context *ac,
         {
      CGLError cglerr;
                              if (kCGLNoError != cglerr) {
      fprintf(stderr, "set current context: %s\n",
                     cglerr = apple_cgl.set_off_screen(p->context_obj, p->width, p->height,
            if (kCGLNoError != cglerr) {
                           if (!ac->made_current) {
      apple_glapi_oglfw_viewport_scissor(0, 0, p->width, p->height);
                  }
      static void
   pixmap_destroy(Display * dpy, struct apple_glx_drawable *d)
   {
               if (p->pixel_format_obj)
            if (p->context_obj)
                     if (p->buffer) {
      if (munmap(p->buffer, p->size))
            if (-1 == close(p->fd))
            if (shm_unlink(p->path))
                  }
      /* Return true if an error occurred. */
   bool
   apple_glx_pixmap_create(Display * dpy, int screen, Pixmap pixmap,
         {
      struct apple_glx_drawable *d;
   struct apple_glx_pixmap *p;
   bool double_buffered;
   bool uses_stereo;
   CGLError error;
            if (apple_glx_drawable_create(dpy, screen, pixmap, &d, &callbacks))
                              p->xpixmap = pixmap;
            if (!XAppleDRICreatePixmap(dpy, screen, pixmap,
                  d->unlock(d);
   d->destroy(d);
                        if (p->fd < 0) {
      perror("shm_open");
   d->unlock(d);
   d->destroy(d);
               p->buffer = mmap(NULL, p->size, PROT_READ | PROT_WRITE,
            if (MAP_FAILED == p->buffer) {
      perror("mmap");
   d->unlock(d);
   d->destroy(d);
               apple_visual_create_pfobj(&p->pixel_format_obj, mode, &double_buffered,
            error = apple_cgl.create_context(p->pixel_format_obj, NULL,
            if (kCGLNoError != error) {
      d->unlock(d);
   d->destroy(d);
                                             }
      bool
   apple_glx_pixmap_query(GLXPixmap pixmap, int attr, unsigned int *value)
   {
      struct apple_glx_drawable *d;
   struct apple_glx_pixmap *p;
            d = apple_glx_drawable_find_by_type(pixmap, APPLE_GLX_DRAWABLE_PIXMAP,
            if (d) {
               switch (attr) {
   case GLX_WIDTH:
      *value = p->width;
               case GLX_HEIGHT:
      *value = p->height;
               case GLX_FBCONFIG_ID:
      *value = p->fbconfigID;
   result = true;
                              }
      /* Return true if the type is valid for pixmap. */
   bool
   apple_glx_pixmap_destroy(Display * dpy, GLXPixmap pixmap)
   {
      return !apple_glx_drawable_destroy_by_type(dpy, pixmap,
      }
