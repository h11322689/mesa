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
   #include <assert.h>
   #include "glxclient.h"
   #include "apple_glx.h"
   #include "appledri.h"
   #include "apple_glx_drawable.h"
      static bool surface_make_current(struct apple_glx_context *ac,
            static void surface_destroy(Display * dpy, struct apple_glx_drawable *d);
         static struct apple_glx_drawable_callbacks callbacks = {
      .type = APPLE_GLX_DRAWABLE_SURFACE,
   .make_current = surface_make_current,
      };
      static void
   update_viewport_and_scissor(Display * dpy, GLXDrawable drawable)
   {
      Window root;
   int x, y;
                        }
      static bool
   surface_make_current(struct apple_glx_context *ac,
         {
      struct apple_glx_surface *s = &d->types.surface;
                     apple_glx_diagnostic("%s: ac->context_obj %p s->surface_id %u\n",
                     if (error) {
      fprintf(stderr, "error: xp_attach_gl_context returned: %d\n", error);
                  if (!ac->made_current) {
      /* 
   * The first time a new context is made current the glViewport
   * and glScissor should be updated.
   */
   update_viewport_and_scissor(ac->drawable->display,
                                 }
      static void
   surface_destroy(Display * dpy, struct apple_glx_drawable *d)
   {
                                 if (error) {
                  /* 
   * Check if this surface destroy came from the surface being destroyed
   * on the server.  If s->pending_destroy is true, then it did, and 
   * we don't want to try to destroy the surface on the server.
   */
   if (!s->pending_destroy) {
      /*
   * Warning: this causes other routines to be called (potentially)
   * from surface_notify_handler.  It's probably best to not have
   * any locks at this point locked.
   */
   XAppleDRIDestroySurface(d->display, DefaultScreen(d->display),
            apple_glx_diagnostic
      ("%s: destroyed a surface for drawable 0x%lx uid %u\n", __func__,
      }
      /* Return true if an error occurred. */
   static bool
   create_surface(Display * dpy, int screen, struct apple_glx_drawable *d)
   {
      struct apple_glx_surface *s = &d->types.surface;
   unsigned int key[2];
            id = apple_glx_get_client_id();
   if (0 == id)
                              if (XAppleDRICreateSurface(dpy, screen, d->drawable, id, key, &s->uid)) {
                        if (error) {
      fprintf(stderr, "error: xp_import_surface returned: %d\n", error);
               apple_glx_diagnostic("%s: created a surface for drawable 0x%lx"
                        }
      /* Return true if an error occurred. */
   /* This returns a referenced object via resultptr. */
   bool
   apple_glx_surface_create(Display * dpy, int screen,
               {
               if (apple_glx_drawable_create(dpy, screen, drawable, &d, &callbacks))
                     if (create_surface(dpy, screen, d)) {
      d->unlock(d);
   d->destroy(d);
                                    }
      /*
   * All surfaces are reference counted, and surfaces are only created
   * when the window is made current.  When all contexts no longer reference
   * a surface drawable the apple_glx_drawable gets destroyed, and thus
   * its surface is destroyed.  
   *
   * However we can make the destruction occur a bit sooner by setting
   * pending_destroy, which is then checked for in glViewport by
   * apple_glx_context_update.
   */
   void
   apple_glx_surface_destroy(unsigned int uid)
   {
               d = apple_glx_drawable_find_by_uid(uid, APPLE_GLX_DRAWABLE_REFERENCE
            if (d) {
      d->types.surface.pending_destroy = true;
            /* 
   * We release 2 references to the surface.  One was acquired by
   * the find, and the other was leftover from a context, or 
   * the surface being displayed, so the destroy() will decrease it
   * once more.
   *
   * If the surface is in a context, it will take one d->destroy(d);
   * to actually destroy it when the pending_destroy is processed
   * by a glViewport callback (see apple_glx_context_update()).
   */
   if (!d->destroy(d)) {
      /* apple_glx_drawable_find_by_uid returns a locked drawable */
            }
