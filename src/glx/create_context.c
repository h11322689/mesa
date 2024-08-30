   /*
   * Copyright © 2011 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <limits.h>
   #include "glxclient.h"
   #include "glx_error.h"
   #include <xcb/glx.h>
   #include <X11/Xlib-xcb.h>
      #include <assert.h>
      #if INT_MAX != 2147483647
   #error This code requires sizeof(uint32_t) == sizeof(int).
   #endif
      /* An "Atrribs/Attribs" typo was fixed in glxproto.h in Nov 2014.
   * This is in case we don't have the updated header.
   */
   #if !defined(X_GLXCreateContextAttribsARB) && \
         #define X_GLXCreateContextAttribsARB X_GLXCreateContextAtrribsARB
   #endif
      _X_HIDDEN GLXContext
   glXCreateContextAttribsARB(Display *dpy, GLXFBConfig config,
               {
      xcb_connection_t *const c = XGetXCBConnection(dpy);
   struct glx_config *const cfg = (struct glx_config *) config;
   struct glx_context *const share = (struct glx_context *) share_context;
   struct glx_context *gc = NULL;
   unsigned num_attribs = 0;
   struct glx_screen *psc;
   xcb_generic_error_t *err;
   xcb_void_cookie_t cookie;
   unsigned error = BadImplementation;
   uint32_t xid, share_xid;
            if (dpy == NULL)
            /* Count the number of attributes specified by the application.  All
   * attributes appear in pairs, except the terminating None.
   */
   if (attrib_list != NULL) {
      for (/* empty */; attrib_list[num_attribs * 2] != 0; num_attribs++)
               if (cfg) {
         } else {
      for (unsigned int i = 0; i < num_attribs; i++) {
      if (attrib_list[i * 2] == GLX_SCREEN)
      }
   if (screen == -1) {
      __glXSendError(dpy, BadValue, 0, X_GLXCreateContextAttribsARB, True);
                  /* This means that either the caller passed the wrong display pointer or
   * one of the internal GLX data structures (probably the fbconfig) has an
   * error.  There is nothing sensible to do, so return an error.
   */
   psc = GetGLXScreenConfigs(dpy, screen);
   if (psc == NULL)
                     /* Some application may request an indirect context but we may want to force a direct
   * one because Xorg only allows indirect contexts if they were enabled.
   */
   if (!direct &&
      psc->force_direct_context) {
            #ifdef GLX_USE_APPLEGL
         #else
      if (direct && psc->vtable->create_context_attribs) {
      gc = psc->vtable->create_context_attribs(psc, cfg, share, num_attribs,
            } else if (!direct) {
      gc = indirect_create_context_attribs(psc, cfg, share, num_attribs,
               #endif
         if (gc == NULL) {
      /* Increment dpy->request in order to give a unique serial number to the error.
   * This may break creating contexts on some video cards, if libx11 <1.7.4 is used.
   * However, this fixes creating contexts (on some video cards) if libx11 >=1.7.4 is used.
   */
   XNoOp(dpy);
   /* -1 isn't a legal XID, which is sort of the point, we've failed
   * before we even got to XID allocation.
   */
   if (error == GLXBadContext || error == GLXBadFBConfig ||
      error == GLXBadProfileARB)
      else
                     xid = xcb_generate_id(c);
            /* The manual pages for glXCreateContext and glXCreateNewContext say:
   *
   *     "NULL is returned if execution fails on the client side."
   *
   * If the server generates an error, the application is supposed to catch
   * the protocol error and handle it.  Part of handling the error is freeing
   * the possibly non-NULL value returned by this function.
   */
   cookie =
      xcb_glx_create_context_attribs_arb_checked(c,
                                                err = xcb_request_check(c, cookie);
   if (err != NULL) {
      if (gc)
                  __glXSendErrorForXcb(dpy, err);
      } else {
      gc->xid = xid;
                  }
