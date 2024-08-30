   /*
   * (C) Copyright IBM Corporation 2004
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * \file glx_pbuffer.c
   * Implementation of pbuffer related functions.
   *
   * \author Ian Romanick <idr@us.ibm.com>
   */
      #include <inttypes.h>
   #include "glxclient.h"
   #include <X11/extensions/extutil.h>
   #include <X11/extensions/Xext.h>
   #include <assert.h>
   #include <string.h>
   #include <limits.h>
   #include "glxextensions.h"
      #include <X11/Xlib-xcb.h>
   #include <xcb/xproto.h>
      #ifdef GLX_USE_APPLEGL
   #include <pthread.h>
   #include "apple/apple_glx_drawable.h"
   #endif
      #include "glx_error.h"
      #ifndef GLX_USE_APPLEGL
   /**
   * Change a drawable's attribute.
   *
   * This function is used to implement \c glXSelectEvent and
   * \c glXSelectEventSGIX.
   */
   static void
   ChangeDrawableAttribute(Display * dpy, GLXDrawable drawable,
         {
         #ifdef GLX_DIRECT_RENDERING
      __GLXDRIdrawable *pdraw;
      #endif
      CARD32 *output;
            if ((priv == NULL) || (dpy == NULL) || (drawable == 0)) {
                  opcode = __glXSetupForCommand(dpy);
   if (!opcode)
                     xGLXChangeDrawableAttributesReq *req;
   GetReqExtra(GLXChangeDrawableAttributes, 8 * num_attribs, req);
            req->reqType = opcode;
   req->glxCode = X_GLXChangeDrawableAttributes;
   req->drawable = drawable;
                     UnlockDisplay(dpy);
         #ifdef GLX_DIRECT_RENDERING
               if (!pdraw)
            for (i = 0; i < num_attribs; i++) {
      switch(attribs[i * 2]) {
   /* Keep a local copy for masking out DRI2 proto events as needed */
   pdraw->eventMask = attribs[i * 2 + 1];
   break;
               #endif
            }
         #ifdef GLX_DIRECT_RENDERING
   static GLenum
   determineTextureTarget(const int *attribs, int numAttribs)
   {
      GLenum target = 0;
            for (i = 0; i < numAttribs; i++) {
      if (attribs[2 * i] == GLX_TEXTURE_TARGET_EXT) {
      switch (attribs[2 * i + 1]) {
   case GLX_TEXTURE_2D_EXT:
      target = GL_TEXTURE_2D;
      case GLX_TEXTURE_RECTANGLE_EXT:
      target = GL_TEXTURE_RECTANGLE_ARB;
                        }
      static GLenum
   determineTextureFormat(const int *attribs, int numAttribs)
   {
               for (i = 0; i < numAttribs; i++) {
      if (attribs[2 * i] == GLX_TEXTURE_FORMAT_EXT)
                  }
   #endif
      static GLboolean
   CreateDRIDrawable(Display *dpy, struct glx_config *config,
      XID drawable, XID glxdrawable, int type,
      {
   #ifdef GLX_DIRECT_RENDERING
      struct glx_display *const priv = __glXInitialize(dpy);
   __GLXDRIdrawable *pdraw;
            if (priv == NULL) {
      fprintf(stderr, "failed to create drawable\n");
               psc = priv->screens[config->screen];
   if (psc->driScreen == NULL)
            pdraw = psc->driScreen->createDrawable(psc, drawable, glxdrawable,
         if (pdraw == NULL) {
      fprintf(stderr, "failed to create drawable\n");
               if (__glxHashInsert(priv->drawHash, glxdrawable, pdraw)) {
      pdraw->destroyDrawable(pdraw);
               pdraw->textureTarget = determineTextureTarget(attrib_list, num_attribs);
               #endif
            }
      static void
   DestroyDRIDrawable(Display *dpy, GLXDrawable drawable)
   {
   #ifdef GLX_DIRECT_RENDERING
      struct glx_display *const priv = __glXInitialize(dpy);
            if (priv != NULL && pdraw != NULL) {
      pdraw->destroyDrawable(pdraw);
         #endif
   }
      /**
   * Get a drawable's attribute.
   *
   * This function is used to implement \c glXGetSelectedEvent and
   * \c glXGetSelectedEventSGIX.
   *
   * \todo
   * The number of attributes returned is likely to be small, probably less than
   * 10.  Given that, this routine should try to use an array on the stack to
   * capture the reply rather than always calling Xmalloc.
   */
   int
   __glXGetDrawableAttribute(Display * dpy, GLXDrawable drawable,
         {
      struct glx_display *priv;
   xGLXGetDrawableAttributesReply reply;
   CARD32 *data;
   CARD8 opcode;
   unsigned int length;
   unsigned int i;
   unsigned int num_attributes;
         #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
         #endif
         if (dpy == NULL)
            /* Page 38 (page 52 of the PDF) of glxencode1.3.pdf says:
   *
   *     "If drawable is not a valid GLX drawable, a GLXBadDrawable error is
   *     generated."
   */
   if (drawable == 0) {
      XNoOp(dpy);
   __glXSendError(dpy, GLXBadDrawable, 0, X_GLXGetDrawableAttributes, false);
               priv = __glXInitialize(dpy);
   if (priv == NULL)
                     opcode = __glXSetupForCommand(dpy);
   if (!opcode)
         #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
               if (attribute == GLX_BACK_BUFFER_AGE_EXT) {
      struct glx_context *gc = __glXGetCurrentContext();
            /* The GLX_EXT_buffer_age spec says:
   *
   *   "If querying GLX_BACK_BUFFER_AGE_EXT and <draw> is not bound to
   *   the calling thread's current context a GLXBadDrawable error is
   *   generated."
   */
   if (pdraw == NULL || gc == &dummyContext || gc->currentDpy != dpy ||
      (gc->currentDrawable != drawable &&
   gc->currentReadable != drawable)) {
   XNoOp(dpy);
   __glXSendError(dpy, GLXBadDrawable, drawable,
                              if (psc->driScreen->getBufferAge != NULL)
                        if (pdraw) {
      if (attribute == GLX_SWAP_INTERVAL_EXT) {
      *value = pdraw->psc->driScreen->getSwapInterval(pdraw);
      } else if (attribute == GLX_MAX_SWAP_INTERVAL_EXT) {
      *value = pdraw->psc->driScreen->maxSwapInterval;
      } else if (attribute == GLX_LATE_SWAPS_TEAR_EXT) {
      *value = __glXExtensionBitIsEnabled(pdraw->psc,
                  #endif
                  xGLXGetDrawableAttributesReq *req;
   GetReq(GLXGetDrawableAttributes, req);
   req->reqType = opcode;
   req->glxCode = X_GLXGetDrawableAttributes;
                     if (reply.type == X_Error) {
      UnlockDisplay(dpy);
   SyncHandle();
               length = reply.length;
   if (length) {
      num_attributes = reply.numAttribs;
   data = malloc(length * sizeof(CARD32));
   if (data == NULL) {
      /* Throw data on the floor */
      }
   else {
               /* Search the set of returned attributes for the attribute requested by
   * the caller.
   */
   for (i = 0; i < num_attributes; i++) {
      if (data[i * 2] == attribute) {
      found = 1;
   *value = data[(i * 2) + 1];
         #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
            if (pdraw != NULL) {
      if (!pdraw->textureTarget)
      pdraw->textureTarget =
      if (!pdraw->textureFormat)
         #endif
                              UnlockDisplay(dpy);
         #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      if (pdraw && attribute == GLX_FBCONFIG_ID && !found) {
      /* If we failed to lookup the GLX_FBCONFIG_ID, it may be because the drawable is
   * a bare Window, so try differently by first figure out its visual, then GLX
   * visual like driInferDrawableConfig does.
   */
   xcb_get_window_attributes_cookie_t cookie = { 0 };
                     if (conn) {
      cookie = xcb_get_window_attributes(conn, drawable);
   attr = xcb_get_window_attributes_reply(conn, cookie, NULL);
   if (attr) {
      /* Find the Window's GLX Visual */
                  if (conf)
               #endif
            }
      static int dummyErrorHandler(Display *display, xError *err, XExtCodes *codes,
         {
         }
      static void
   protocolDestroyDrawable(Display *dpy, GLXDrawable drawable, CARD32 glxCode)
   {
      xGLXDestroyPbufferReq *req;
            opcode = __glXSetupForCommand(dpy);
   if (!opcode)
                     GetReq(GLXDestroyPbuffer, req);
   req->reqType = opcode;
   req->glxCode = glxCode;
            UnlockDisplay(dpy);
            /* Viewperf2020/Sw calls XDestroyWindow(win) and then glXDestroyWindow(win),
   * causing an X error and abort. This is the workaround.
   */
            if (priv->screens[0] &&
      priv->screens[0]->allow_invalid_glx_destroy_window) {
   void *old = XESetError(priv->dpy, priv->codes.extension,
         XSync(dpy, false);
         }
      /**
   * Create a non-pbuffer GLX drawable.
   */
   static GLXDrawable
   CreateDrawable(Display *dpy, struct glx_config *config,
         {
      xGLXCreateWindowReq *req;
   struct glx_drawable *glxDraw;
   CARD32 *data;
   unsigned int i;
   CARD8 opcode;
            if (!config)
            i = 0;
   if (attrib_list) {
      while (attrib_list[i * 2] != None)
               opcode = __glXSetupForCommand(dpy);
   if (!opcode)
            glxDraw = malloc(sizeof(*glxDraw));
   if (!glxDraw)
            LockDisplay(dpy);
   GetReqExtra(GLXCreateWindow, 8 * i, req);
            req->reqType = opcode;
   req->screen = config->screen;
   req->fbconfig = config->fbconfigID;
   req->window = drawable;
   req->glxwindow = xid = XAllocID(dpy);
            if (type == GLX_WINDOW_BIT)
         else
            if (attrib_list)
            UnlockDisplay(dpy);
            if (InitGLXDrawable(dpy, glxDraw, drawable, xid)) {
      free(glxDraw);
               if (!CreateDRIDrawable(dpy, config, drawable, xid, type, attrib_list, i)) {
      CARD8 glxCode;
   if (type == GLX_PIXMAP_BIT)
         else
         protocolDestroyDrawable(dpy, xid, glxCode);
                  }
         /**
   * Destroy a non-pbuffer GLX drawable.
   */
   static void
   DestroyDrawable(Display * dpy, GLXDrawable drawable, CARD32 glxCode)
   {
               DestroyGLXDrawable(dpy, drawable);
               }
         /**
   * Create a pbuffer.
   *
   * This function is used to implement \c glXCreatePbuffer and
   * \c glXCreateGLXPbufferSGIX.
   */
   static GLXDrawable
   CreatePbuffer(Display * dpy, struct glx_config *config,
               {
      struct glx_display *priv = __glXInitialize(dpy);
   GLXDrawable id = 0;
   CARD32 *data;
   CARD8 opcode;
            if (priv == NULL)
            i = 0;
   if (attrib_list) {
      while (attrib_list[i * 2])
               opcode = __glXSetupForCommand(dpy);
   if (!opcode)
            LockDisplay(dpy);
            xGLXCreatePbufferReq *req;
   unsigned int extra = (size_in_attribs) ? 0 : 2;
   GetReqExtra(GLXCreatePbuffer, (8 * (i + extra)), req);
            req->reqType = opcode;
   req->glxCode = X_GLXCreatePbuffer;
   req->screen = config->screen;
   req->fbconfig = config->fbconfigID;
   req->pbuffer = id;
            if (!size_in_attribs) {
      data[(2 * i) + 0] = GLX_PBUFFER_WIDTH;
   data[(2 * i) + 1] = width;
   data[(2 * i) + 2] = GLX_PBUFFER_HEIGHT;
   data[(2 * i) + 3] = height;
                        UnlockDisplay(dpy);
            /* xserver created a pixmap with the same id as pbuffer */
   if (!CreateDRIDrawable(dpy, config, id, id, GLX_PBUFFER_BIT, attrib_list, i)) {
      protocolDestroyDrawable(dpy, id, X_GLXDestroyPbuffer);
                  }
      /**
   * Destroy a pbuffer.
   *
   * This function is used to implement \c glXDestroyPbuffer and
   * \c glXDestroyGLXPbufferSGIX.
   */
   static void
   DestroyPbuffer(Display * dpy, GLXDrawable drawable)
   {
      struct glx_display *priv = __glXInitialize(dpy);
            if ((priv == NULL) || (dpy == NULL) || (drawable == 0)) {
                  opcode = __glXSetupForCommand(dpy);
   if (!opcode)
                     xGLXDestroyPbufferReq *req;
   GetReq(GLXDestroyPbuffer, req);
   req->reqType = opcode;
   req->glxCode = X_GLXDestroyPbuffer;
            UnlockDisplay(dpy);
                        }
      /**
   * Create a new pbuffer.
   */
   _GLX_PUBLIC GLXPbufferSGIX
   glXCreateGLXPbufferSGIX(Display * dpy, GLXFBConfigSGIX config,
               {
      return (GLXPbufferSGIX) CreatePbuffer(dpy, (struct glx_config *) config,
            }
      #endif /* GLX_USE_APPLEGL */
      /**
   * Create a new pbuffer.
   */
   _GLX_PUBLIC GLXPbuffer
   glXCreatePbuffer(Display * dpy, GLXFBConfig config, const int *attrib_list)
   {
         #ifdef GLX_USE_APPLEGL
      GLXPbuffer result;
      #endif
         width = 0;
         #ifdef GLX_USE_APPLEGL
      for (i = 0; attrib_list[i]; ++i) {
      switch (attrib_list[i]) {
   case GLX_PBUFFER_WIDTH:
      width = attrib_list[i + 1];
               case GLX_PBUFFER_HEIGHT:
      height = attrib_list[i + 1];
               case GLX_LARGEST_PBUFFER:
      /* This is a hint we should probably handle, but how? */
               case GLX_PRESERVED_CONTENTS:
      /* The contents are always preserved with AppleSGLX with CGL. */
               default:
                     if (apple_glx_pbuffer_create(dpy, config, width, height, &errorcode,
            /* 
   * apple_glx_pbuffer_create only sets the errorcode to core X11
   * errors. 
   */
                           #else
      for (i = 0; attrib_list[i * 2]; i++) {
      switch (attrib_list[i * 2]) {
   case GLX_PBUFFER_WIDTH:
      width = attrib_list[i * 2 + 1];
      case GLX_PBUFFER_HEIGHT:
      height = attrib_list[i * 2 + 1];
                  return (GLXPbuffer) CreatePbuffer(dpy, (struct glx_config *) config,
      #endif
   }
         /**
   * Destroy an existing pbuffer.
   */
   _GLX_PUBLIC void
   glXDestroyPbuffer(Display * dpy, GLXPbuffer pbuf)
   {
   #ifdef GLX_USE_APPLEGL
      if (apple_glx_pbuffer_destroy(dpy, pbuf)) {
            #else
         #endif
   }
         /**
   * Query an attribute of a drawable.
   */
   _GLX_PUBLIC void
   glXQueryDrawable(Display * dpy, GLXDrawable drawable,
         {
   #ifdef GLX_USE_APPLEGL
      Window root;
   int x, y;
            if (apple_glx_pixmap_query(drawable, attribute, value))
            if (apple_glx_pbuffer_query(drawable, attribute, value))
            /*
   * The OpenGL spec states that we should report GLXBadDrawable if
   * the drawable is invalid, however doing so would require that we
   * use XSetErrorHandler(), which is known to not be thread safe.
   * If we use a round-trip call to validate the drawable, there could
   * be a race, so instead we just opt in favor of letting the
   * XGetGeometry request fail with a GetGeometry request X error 
   * rather than GLXBadDrawable, in what is hoped to be a rare
   * case of an invalid drawable.  In practice most and possibly all
   * X11 apps using GLX shouldn't notice a difference.
   */
   if (XGetGeometry
      (dpy, drawable, &root, &x, &y, &width, &height, &bd, &depth)) {
   switch (attribute) {
   case GLX_WIDTH:
                  case GLX_HEIGHT:
      *value = height;
            #else
         #endif
   }
         #ifndef GLX_USE_APPLEGL
   /**
   * Query an attribute of a pbuffer.
   */
   _GLX_PUBLIC void
   glXQueryGLXPbufferSGIX(Display * dpy, GLXPbufferSGIX drawable,
         {
         }
   #endif
      /**
   * Select the event mask for a drawable.
   */
   _GLX_PUBLIC void
   glXSelectEvent(Display * dpy, GLXDrawable drawable, unsigned long mask)
   {
   #ifdef GLX_USE_APPLEGL
               if (apple_glx_pbuffer_set_event_mask(drawable, mask))
            /* 
   * The spec allows a window, but currently there are no valid
   * events for a window, so do nothing.
   */
   if (XGetWindowAttributes(dpy, drawable, &xwattr))
                  __glXSendError(dpy, GLXBadDrawable, drawable,
      #else
               attribs[0] = (CARD32) GLX_EVENT_MASK;
               #endif
   }
         /**
   * Get the selected event mask for a drawable.
   */
   _GLX_PUBLIC void
   glXGetSelectedEvent(Display * dpy, GLXDrawable drawable, unsigned long *mask)
   {
   #ifdef GLX_USE_APPLEGL
               if (apple_glx_pbuffer_get_event_mask(drawable, mask))
            /* 
   * The spec allows a window, but currently there are no valid
   * events for a window, so do nothing, but set the mask to 0.
   */
   if (XGetWindowAttributes(dpy, drawable, &xwattr)) {
      /* The window is valid, so set the mask to 0. */
   *mask = 0;
      }
            __glXSendError(dpy, GLXBadDrawable, drawable, X_GLXGetDrawableAttributes,
      #else
                  /* The non-sense with value is required because on LP64 platforms
   * sizeof(unsigned int) != sizeof(unsigned long).  On little-endian
   * we could just type-cast the pointer, but why?
            __glXGetDrawableAttribute(dpy, drawable, GLX_EVENT_MASK_SGIX, &value);
      #endif
   }
         _GLX_PUBLIC GLXPixmap
   glXCreatePixmap(Display * dpy, GLXFBConfig config, Pixmap pixmap,
         {
   #ifdef GLX_USE_APPLEGL
               if (apple_glx_pixmap_create(dpy, modes->screen, pixmap, modes))
               #else
      return CreateDrawable(dpy, (struct glx_config *) config,
      #endif
   }
         _GLX_PUBLIC GLXWindow
   glXCreateWindow(Display * dpy, GLXFBConfig config, Window win,
         {
   #ifdef GLX_USE_APPLEGL
      XWindowAttributes xwattr;
                                       if (NULL == visinfo) {
      __glXSendError(dpy, GLXBadFBConfig, 0, X_GLXCreateWindow, false);
               if (visinfo->visualid != XVisualIDFromVisual(xwattr.visual)) {
      __glXSendError(dpy, BadMatch, 0, X_GLXCreateWindow, true);
                           #else
      return CreateDrawable(dpy, (struct glx_config *) config,
      #endif
   }
         _GLX_PUBLIC void
   glXDestroyPixmap(Display * dpy, GLXPixmap pixmap)
   {
   #ifdef GLX_USE_APPLEGL
      if (apple_glx_pixmap_destroy(dpy, pixmap))
      #else
         #endif
   }
         _GLX_PUBLIC void
   glXDestroyWindow(Display * dpy, GLXWindow win)
   {
   #ifndef GLX_USE_APPLEGL
         #endif
   }
      _GLX_PUBLIC
   GLX_ALIAS_VOID(glXDestroyGLXPbufferSGIX,
                  _GLX_PUBLIC
   GLX_ALIAS_VOID(glXSelectEventSGIX,
                  _GLX_PUBLIC
   GLX_ALIAS_VOID(glXGetSelectedEventSGIX,
                        _GLX_PUBLIC GLXPixmap
   glXCreateGLXPixmap(Display * dpy, XVisualInfo * vis, Pixmap pixmap)
   {
   #ifdef GLX_USE_APPLEGL
      int screen = vis->screen;
   struct glx_screen *const psc = GetGLXScreenConfigs(dpy, screen);
                     if(apple_glx_pixmap_create(dpy, vis->screen, pixmap, config))
               #else
      xGLXCreateGLXPixmapReq *req;
   struct glx_drawable *glxDraw;
   GLXPixmap xid;
         #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
               if (priv == NULL)
      #endif
         opcode = __glXSetupForCommand(dpy);
   if (!opcode) {
                  glxDraw = malloc(sizeof(*glxDraw));
   if (!glxDraw)
            /* Send the glXCreateGLXPixmap request */
   LockDisplay(dpy);
   GetReq(GLXCreateGLXPixmap, req);
   req->reqType = opcode;
   req->glxCode = X_GLXCreateGLXPixmap;
   req->screen = vis->screen;
   req->visual = vis->visualid;
   req->pixmap = pixmap;
   req->glxpixmap = xid = XAllocID(dpy);
   UnlockDisplay(dpy);
            if (InitGLXDrawable(dpy, glxDraw, pixmap, req->glxpixmap)) {
      free(glxDraw);
            #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      do {
      /* FIXME: Maybe delay __DRIdrawable creation until the drawable
            struct glx_screen *psc = GetGLXScreenConfigs(dpy, vis->screen);
   struct glx_config *config = glx_config_find_visual(psc->visuals,
            if (!CreateDRIDrawable(dpy, config, pixmap, xid, GLX_PIXMAP_BIT,
            protocolDestroyDrawable(dpy, xid, X_GLXDestroyGLXPixmap);
            #endif
            #endif
   }
      /*
   ** Destroy the named pixmap
   */
   _GLX_PUBLIC void
   glXDestroyGLXPixmap(Display * dpy, GLXPixmap glxpixmap)
   {
   #ifdef GLX_USE_APPLEGL
      if(apple_glx_pixmap_destroy(dpy, glxpixmap))
      #else
         #endif /* GLX_USE_APPLEGL */
   }
      _GLX_PUBLIC GLXPixmap
   glXCreateGLXPixmapWithConfigSGIX(Display * dpy,
               {
         }
