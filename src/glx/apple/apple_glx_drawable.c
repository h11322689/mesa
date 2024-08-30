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
      #include <stdbool.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <assert.h>
   #include <pthread.h>
   #include <string.h>
   #include "apple_glx.h"
   #include "apple_glx_context.h"
   #include "apple_glx_drawable.h"
   #include "appledri.h"
      static pthread_mutex_t drawables_lock = PTHREAD_MUTEX_INITIALIZER;
   static struct apple_glx_drawable *drawables_list = NULL;
      static void
   lock_drawables_list(void)
   {
                        if (err) {
      fprintf(stderr, "pthread_mutex_lock failure in %s: %s\n",
               }
      static void
   unlock_drawables_list(void)
   {
                        if (err) {
      fprintf(stderr, "pthread_mutex_unlock failure in %s: %s\n",
               }
      struct apple_glx_drawable *
   apple_glx_find_drawable(Display * dpy, GLXDrawable drawable)
   {
                        for (i = drawables_list; i; i = i->next) {
      if (i->drawable == drawable) {
      agd = i;
                              }
      static void
   drawable_lock(struct apple_glx_drawable *agd)
   {
                        if (err) {
      fprintf(stderr, "pthread_mutex_lock error: %s\n", strerror(err));
         }
      static void
   drawable_unlock(struct apple_glx_drawable *d)
   {
                        if (err) {
      fprintf(stderr, "pthread_mutex_unlock error: %s\n", strerror(err));
         }
         static void
   reference_drawable(struct apple_glx_drawable *d)
   {
      d->lock(d);
   d->reference_count++;
      }
      static void
   release_drawable(struct apple_glx_drawable *d)
   {
      d->lock(d);
   d->reference_count--;
      }
      /* The drawables list must be locked prior to calling this. */
   /* Return true if the drawable was destroyed. */
   static bool
   destroy_drawable(struct apple_glx_drawable *d)
   {
                        if (d->reference_count > 0) {
      d->unlock(d);
                        if (d->previous) {
         }
   else {
      /*
   * The item must be at the head of the list, if it
   * has no previous pointer. 
   */
               if (d->next)
                     if (d->callbacks.destroy) {
      /*
   * Warning: this causes other routines to be called (potentially)
   * from surface_notify_handler.  It's probably best to not have
   * any locks at this point locked.
   */
                        /* Stupid recursive locks */
            err = pthread_mutex_destroy(&d->mutex);
   if (err) {
      fprintf(stderr, "pthread_mutex_destroy error: %s\n", strerror(err));
      }
               /* So that the locks are balanced and the caller correctly unlocks. */
               }
      /*
   * This is typically called when a context is destroyed or the current
   * drawable is made None.
   */
   static bool
   destroy_drawable_callback(struct apple_glx_drawable *d)
   {
                        apple_glx_diagnostic("%s: %p ->reference_count before -- %d\n", __func__,
                     if (d->reference_count > 0) {
      d->unlock(d);
                                                      }
      static bool
   is_pbuffer(struct apple_glx_drawable *d)
   {
         }
      static bool
   is_pixmap(struct apple_glx_drawable *d)
   {
         }
      static void
   common_init(Display * dpy, GLXDrawable drawable, struct apple_glx_drawable *d)
   {
      int err;
            d->display = dpy;
   d->reference_count = 0;
   d->drawable = drawable;
                     if (err) {
      fprintf(stderr, "pthread_mutexattr_init error: %s\n", strerror(err));
               /* 
   * There are some patterns that require a recursive mutex,
   * when working with locks that protect the apple_glx_drawable,
   * and reference functions like ->reference, and ->release.
   */
            if (err) {
      fprintf(stderr, "error: setting pthread mutex type: %s\n", strerror(err));
                        if (err) {
      fprintf(stderr, "pthread_mutex_init error: %s\n", strerror(err));
                        d->lock = drawable_lock;
            d->reference = reference_drawable;
                     d->is_pbuffer = is_pbuffer;
            d->width = -1;
   d->height = -1;
   d->row_bytes = 0;
   d->path[0] = '\0';
   d->fd = -1;
   d->buffer = NULL;
            d->previous = NULL;
      }
      static void
   link_tail(struct apple_glx_drawable *agd)
   {
               /* Link the new drawable into the global list. */
            if (drawables_list)
                        }
      /*WARNING: this returns a locked and referenced object. */
   bool
   apple_glx_drawable_create(Display * dpy,
                           {
                        if (NULL == d) {
      perror("malloc");
               common_init(dpy, drawable, d);
   d->type = callbacks->type;
            d->reference(d);
                                          }
      static int error_count = 0;
      static int
   error_handler(Display * dpy, XErrorEvent * err)
   {
      if (err->error_code == BadWindow) {
                     }
      void
   apple_glx_garbage_collect_drawables(Display * dpy)
   {
      struct apple_glx_drawable *d, *dnext;
   Window root;
   int x, y;
   unsigned int width, height, bd, depth;
               if (NULL == drawables_list)
                                       for (d = drawables_list; d;) {
                        if (d->reference_count > 0) {
      /* 
   * Skip this, because some context still retains a reference 
   * to the drawable.
   */
   d->unlock(d);
   d = dnext;
                                 /* 
   * Mesa uses XGetWindowAttributes, but some of these things are 
   * most definitely not Windows, and that's against the rules.
   * XGetGeometry on the other hand is legal with a Pixmap and Window.
   */
   XGetGeometry(dpy, d->drawable, &root, &x, &y, &width, &height, &bd,
            if (error_count > 0) {
      /*
   * Note: this may not actually destroy the drawable.
   * If another context retains a reference to the drawable
   * after the reference count test above. 
   */
   (void) destroy_drawable(d);
                                       }
      unsigned int
   apple_glx_get_drawable_count(void)
   {
      unsigned int result = 0;
                     for (d = drawables_list; d; d = d->next)
                        }
      struct apple_glx_drawable *
   apple_glx_drawable_find_by_type(GLXDrawable drawable, int type, int flags)
   {
                        for (d = drawables_list; d; d = d->next) {
      if (d->type == type && d->drawable == drawable) {
                                                                        }
      struct apple_glx_drawable *
   apple_glx_drawable_find(GLXDrawable drawable, int flags)
   {
                        for (d = drawables_list; d; d = d->next) {
      if (d->drawable == drawable) {
                                                                        }
      /* Return true if the type is valid for the drawable. */
   bool
   apple_glx_drawable_destroy_by_type(Display * dpy,
         {
                        for (d = drawables_list; d; d = d->next) {
      if (drawable == d->drawable && type == d->type) {
      /*
   * The user has requested that we destroy this resource.
   * However, there may be references in the contexts to it, so
   * release it, and call destroy_drawable which doesn't destroy
   * if the reference_count is > 0.
                                 destroy_drawable(d);
   unlock_drawables_list();
                              }
      struct apple_glx_drawable *
   apple_glx_drawable_find_by_uid(unsigned int uid, int flags)
   {
                        for (d = drawables_list; d; d = d->next) {
      /* Only surfaces have a uid. */
   if (APPLE_GLX_DRAWABLE_SURFACE == d->type) {
      if (d->types.surface.uid == uid) {
                                                                           }
