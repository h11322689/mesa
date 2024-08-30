   /*
   * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
   * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
   *
   * SPDX-License-Identifier: SGI-B-2.0
   */
      /**
   * \file glxcmds.c
   * Client-side GLX interface.
   */
      #include "glxclient.h"
   #include "glapi.h"
   #include "glxextensions.h"
   #include "indirect.h"
   #include "glx_error.h"
      #ifdef GLX_DIRECT_RENDERING
   #ifdef GLX_USE_APPLEGL
   #include "apple/apple_glx_context.h"
   #include "apple/apple_glx.h"
   #include "util/u_debug.h"
   #else
   #ifndef GLX_USE_WINDOWSGL
   #include <X11/extensions/xf86vmode.h>
   #endif /* GLX_USE_WINDOWSGL */
   #endif
   #endif
   #include <limits.h>
   #include <X11/Xlib-xcb.h>
   #include <xcb/xcb.h>
   #include <xcb/glx.h>
   #include "GL/mesa_glinterop.h"
      #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      /**
   * Get the __DRIdrawable for the drawable associated with a GLXContext
   *
   * \param dpy       The display associated with \c drawable.
   * \param drawable  GLXDrawable whose __DRIdrawable part is to be retrieved.
   * \param scrn_num  If non-NULL, the drawables screen is stored there
   * \returns  A pointer to the context's __DRIdrawable on success, or NULL if
   *           the drawable is not associated with a direct-rendering context.
   */
   _X_HIDDEN __GLXDRIdrawable *
   GetGLXDRIDrawable(Display * dpy, GLXDrawable drawable)
   {
      struct glx_display *priv = __glXInitialize(dpy);
            if (priv == NULL)
            if (__glxHashLookup(priv->drawHash, drawable, (void *) &pdraw) == 0)
               }
      #endif
      _X_HIDDEN struct glx_drawable *
   GetGLXDrawable(Display *dpy, GLXDrawable drawable)
   {
      struct glx_display *priv = __glXInitialize(dpy);
            if (priv == NULL)
            if (__glxHashLookup(priv->glXDrawHash, drawable, (void *) &glxDraw) == 0)
               }
      _X_HIDDEN int
   InitGLXDrawable(Display *dpy, struct glx_drawable *glxDraw, XID xDrawable,
   GLXDrawable drawable)
   {
               if (!priv)
            glxDraw->xDrawable = xDrawable;
   glxDraw->drawable = drawable;
   glxDraw->lastEventSbc = 0;
               }
      _X_HIDDEN void
   DestroyGLXDrawable(Display *dpy, GLXDrawable drawable)
   {
      struct glx_display *priv = __glXInitialize(dpy);
            if (!priv)
            glxDraw = GetGLXDrawable(dpy, drawable);
   __glxHashDelete(priv->glXDrawHash, drawable);
      }
      /**
   * Get the GLX per-screen data structure associated with a GLX context.
   *
   * \param dpy   Display for which the GLX per-screen information is to be
   *              retrieved.
   * \param scrn  Screen on \c dpy for which the GLX per-screen information is
   *              to be retrieved.
   * \returns A pointer to the GLX per-screen data if \c dpy and \c scrn
   *          specify a valid GLX screen, or NULL otherwise.
   *
   * \todo Should this function validate that \c scrn is within the screen
   *       number range for \c dpy?
   */
      _X_HIDDEN struct glx_screen *
   GetGLXScreenConfigs(Display * dpy, int scrn)
   {
               return (priv
            }
         static int
   GetGLXPrivScreenConfig(Display * dpy, int scrn, struct glx_display ** ppriv,
         {
      /* Initialize the extension, if needed .  This has the added value
   * of initializing/allocating the display private
            if (dpy == NULL) {
                  *ppriv = __glXInitialize(dpy);
   if (*ppriv == NULL) {
                  /* Check screen number to see if its valid */
   if ((scrn < 0) || (scrn >= ScreenCount(dpy))) {
                  /* Check to see if the GL is supported on this screen */
   *ppsc = (*ppriv)->screens[scrn];
   if ((*ppsc)->configs == NULL && (*ppsc)->visuals == NULL) {
      /* No support for GL on this screen regardless of visual */
                  }
         /**
   * Determine if a \c GLXFBConfig supplied by the application is valid.
   *
   * \param dpy     Application supplied \c Display pointer.
   * \param config  Application supplied \c GLXFBConfig.
   *
   * \returns If the \c GLXFBConfig is valid, the a pointer to the matching
   *          \c struct glx_config structure is returned.  Otherwise, \c NULL
   *          is returned.
   */
   static struct glx_config *
   ValidateGLXFBConfig(Display * dpy, GLXFBConfig fbconfig)
   {
      struct glx_display *const priv = __glXInitialize(dpy);
   int num_screens = ScreenCount(dpy);
   unsigned i;
            if (priv != NULL) {
      for (config = priv->screens[i]->configs; config != NULL;
            if (config == (struct glx_config *) fbconfig) {
            }
                        }
      /**
   * Verifies context's GLX_RENDER_TYPE value with config.
   *
   * \param config GLX FBConfig which will support the returned renderType.
   * \param renderType The context render type to be verified.
   * \return True if the value of context renderType was approved, or 0 if no
   * valid value was found.
   */
   Bool
   validate_renderType_against_config(const struct glx_config *config,
         {
      /* GLX_EXT_no_config_context supports any render type */
   if (!config)
            switch (renderType) {
      case GLX_RGBA_TYPE:
         case GLX_COLOR_INDEX_TYPE:
         case GLX_RGBA_FLOAT_TYPE_ARB:
         case GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT:
         default:
      }
      }
      _X_HIDDEN Bool
   glx_context_init(struct glx_context *gc,
         {
      gc->majorOpcode = __glXSetupForCommand(psc->display->dpy);
   if (!gc->majorOpcode)
            gc->psc = psc;
   gc->config = config;
   gc->isDirect = GL_TRUE;
            if (!config)
               }
      /**
   * Determine if a context uses direct rendering.
   *
   * \param dpy        Display where the context was created.
   * \param contextID  ID of the context to be tested.
   * \param error      Out parameter, set to True on error if not NULL,
   *                   otherwise raise the error to the application.
   *
   * \returns \c True if the context is direct rendering or not.
   */
   static Bool
   __glXIsDirect(Display * dpy, GLXContextID contextID, Bool *error)
   {
      xcb_connection_t *c;
   xcb_generic_error_t *err;
   xcb_glx_is_direct_reply_t *reply;
            c = XGetXCBConnection(dpy);
   reply = xcb_glx_is_direct_reply(c, xcb_glx_is_direct(c, contextID), &err);
            if (err != NULL) {
      if (error)
         else
                                 }
      /**
   * Create a new context.
   *
   * \param renderType   For FBConfigs, what is the rendering type?
   */
      static GLXContext
   CreateContext(Display *dpy, int generic_id, struct glx_config *config,
               {
      struct glx_context *gc;
   struct glx_screen *psc;
   struct glx_context *shareList = (struct glx_context *) shareList_user;
   if (dpy == NULL)
            psc = GetGLXScreenConfigs(dpy, config->screen);
   if (psc == NULL)
            if (generic_id == None)
            /* Some application may request an indirect context but we may want to force a direct
   * one because Xorg only allows indirect contexts if they were enabled.
   */
   if (!allowDirect &&
      psc->force_direct_context) {
                  #ifdef GLX_USE_APPLEGL
         #else
      if (allowDirect && psc->vtable->create_context)
         if (!gc)
      #endif
      if (!gc)
            LockDisplay(dpy);
   switch (code) {
   case X_GLXCreateContext: {
               /* Send the glXCreateContext request */
   GetReq(GLXCreateContext, req);
   req->reqType = gc->majorOpcode;
   req->glxCode = X_GLXCreateContext;
   req->context = gc->xid = XAllocID(dpy);
   req->visual = generic_id;
   req->screen = config->screen;
   req->shareList = shareList ? shareList->xid : None;
   req->isDirect = gc->isDirect;
               case X_GLXCreateNewContext: {
               /* Send the glXCreateNewContext request */
   GetReq(GLXCreateNewContext, req);
   req->reqType = gc->majorOpcode;
   req->glxCode = X_GLXCreateNewContext;
   req->context = gc->xid = XAllocID(dpy);
   req->fbconfig = generic_id;
   req->screen = config->screen;
   req->renderType = renderType;
   req->shareList = shareList ? shareList->xid : None;
   req->isDirect = gc->isDirect;
               default:
      /* What to do here?  This case is the sign of an internal error.  It
   * should never be reachable.
   */
               UnlockDisplay(dpy);
            gc->share_xid = shareList ? shareList->xid : None;
            /* Unlike most X resource creation requests, we're about to return a handle
   * with client-side state, not just an XID. To simplify error handling
   * elsewhere in libGL, force a round-trip here to ensure the CreateContext
   * request above succeeded.
   */
   {
      Bool error = False;
            if (error != False || isDirect != gc->isDirect) {
      gc->vtable->destroy(gc);
                     }
      _GLX_PUBLIC GLXContext
   glXCreateContext(Display * dpy, XVisualInfo * vis,
         {
      struct glx_config *config = NULL;
         #if defined(GLX_DIRECT_RENDERING) || defined(GLX_USE_APPLEGL)
               if (psc)
            if (config == NULL) {
      __glXSendError(dpy, BadValue, vis->visualid, X_GLXCreateContext, True);
               /* Choose the context render type based on DRI config values.  It is
   * unusual to set this type from config, but we have no other choice, as
   * this old API does not provide renderType parameter.
   */
   if (config->renderType & GLX_RGBA_FLOAT_BIT_ARB) {
         } else if (config->renderType & GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT) {
         } else if (config->renderType & GLX_RGBA_BIT) {
         } else if (config->renderType & GLX_COLOR_INDEX_BIT) {
            #endif
         return CreateContext(dpy, vis->visualid, config, shareList, allowDirect,
      }
      static void
   glx_send_destroy_context(Display *dpy, XID xid)
   {
      CARD8 opcode = __glXSetupForCommand(dpy);
            LockDisplay(dpy);
   GetReq(GLXDestroyContext, req);
   req->reqType = opcode;
   req->glxCode = X_GLXDestroyContext;
   req->context = xid;
   UnlockDisplay(dpy);
      }
      /*
   ** Destroy the named context
   */
      _GLX_PUBLIC void
   glXDestroyContext(Display * dpy, GLXContext ctx)
   {
               if (gc == NULL || gc->xid == None)
            __glXLock();
   if (!gc->imported)
            if (gc->currentDpy) {
      /* This context is bound to some thread.  According to the man page,
   * we should not actually delete the context until it's unbound.
   * Note that we set gc->xid = None above.  In MakeContextCurrent()
   * we check for that and delete the context there.
   */
      } else {
         }
      }
      /*
   ** Return the major and minor version #s for the GLX extension
   */
   _GLX_PUBLIC Bool
   glXQueryVersion(Display * dpy, int *major, int *minor)
   {
               /* Init the extension.  This fetches the major and minor version. */
   priv = __glXInitialize(dpy);
   if (!priv)
            if (major)
         if (minor)
            }
      /*
   ** Query the existence of the GLX extension
   */
   _GLX_PUBLIC Bool
   glXQueryExtension(Display * dpy, int *errorBase, int *eventBase)
   {
      int major_op, erb, evb;
            rv = XQueryExtension(dpy, GLX_EXTENSION_NAME, &major_op, &evb, &erb);
   if (rv) {
      if (errorBase)
         if (eventBase)
      }
      }
      /*
   ** Put a barrier in the token stream that forces the GL to finish its
   ** work before X can proceed.
   */
   _GLX_PUBLIC void
   glXWaitGL(void)
   {
               if (gc->vtable->wait_gl)
      }
      /*
   ** Put a barrier in the token stream that forces X to finish its
   ** work before GL can proceed.
   */
   _GLX_PUBLIC void
   glXWaitX(void)
   {
               if (gc->vtable->wait_x)
      }
      _GLX_PUBLIC void
   glXUseXFont(Font font, int first, int count, int listBase)
   {
      struct glx_context *gc = __glXGetCurrentContext();
   xGLXUseXFontReq *req;
         #ifdef GLX_DIRECT_RENDERING
      if (gc->isDirect) {
      DRI_glXUseXFont(gc, font, first, count, listBase);
         #endif
         /* Flush any pending commands out */
            /* Send the glXUseFont request */
   LockDisplay(dpy);
   GetReq(GLXUseXFont, req);
   req->reqType = gc->majorOpcode;
   req->glxCode = X_GLXUseXFont;
   req->contextTag = gc->currentContextTag;
   req->font = font;
   req->first = first;
   req->count = count;
   req->listBase = listBase;
   UnlockDisplay(dpy);
      }
      /************************************************************************/
      /*
   ** Copy the source context to the destination context using the
   ** attribute "mask".
   */
   _GLX_PUBLIC void
   glXCopyContext(Display * dpy, GLXContext source_user,
         {
      struct glx_context *source = (struct glx_context *) source_user;
            /* GLX spec 3.3: If the destination context is current for some thread
   * then a BadAccess error is generated
   */
   if (dest && dest->currentDpy) {
      __glXSendError(dpy, BadAccess, 0, X_GLXCopyContext, true);
         #ifdef GLX_USE_APPLEGL
      struct glx_context *gc = __glXGetCurrentContext();
   int errorcode;
            if(apple_glx_copy_context(gc->driContext, source->driContext, dest->driContext,
               }
      #else
      xGLXCopyContextReq *req;
   struct glx_context *gc = __glXGetCurrentContext();
   GLXContextTag tag;
            opcode = __glXSetupForCommand(dpy);
   if (!opcode) {
               #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      if (gc->isDirect) {
            #endif
         /*
   ** If the source is the current context, send its tag so that the context
   ** can be flushed before the copy.
   */
   if (source == gc && dpy == gc->currentDpy) {
         }
   else {
                  /* Send the glXCopyContext request */
   LockDisplay(dpy);
   GetReq(GLXCopyContext, req);
   req->reqType = opcode;
   req->glxCode = X_GLXCopyContext;
   req->source = source ? source->xid : None;
   req->dest = dest ? dest->xid : None;
   req->mask = mask;
   req->contextTag = tag;
   UnlockDisplay(dpy);
      #endif /* GLX_USE_APPLEGL */
   }
         _GLX_PUBLIC Bool
   glXIsDirect(Display * dpy, GLXContext gc_user)
   {
               /* This is set for us at context creation */
      }
      _GLX_PUBLIC void
   glXSwapBuffers(Display * dpy, GLXDrawable drawable)
   {
   #ifdef GLX_USE_APPLEGL
      struct glx_context * gc = __glXGetCurrentContext();
   if(gc != &dummyContext && apple_glx_is_current_drawable(dpy, gc->driContext, drawable)) {
         } else {
            #else
      struct glx_context *gc;
   GLXContextTag tag;
   CARD8 opcode;
                  #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      {
               if (pdraw != NULL) {
               if (pdraw->psc->driScreen->swapBuffers(pdraw, 0, 0, 0, flush) == -1)
                  #endif
         opcode = __glXSetupForCommand(dpy);
   if (!opcode) {
                  /*
   ** The calling thread may or may not have a current context.  If it
   ** does, send the context tag so the server can do a flush.
   */
   if ((gc != &dummyContext) && (dpy == gc->currentDpy) &&
      ((drawable == gc->currentDrawable)
   || (drawable == gc->currentReadable))) {
      }
   else {
                  c = XGetXCBConnection(dpy);
   xcb_glx_swap_buffers(c, tag, drawable);
      #endif /* GLX_USE_APPLEGL */
   }
         /*
   ** Return configuration information for the given display, screen and
   ** visual combination.
   */
   _GLX_PUBLIC int
   glXGetConfig(Display * dpy, XVisualInfo * vis, int attribute,
         {
      struct glx_display *priv;
   struct glx_screen *psc;
   struct glx_config *config;
            status = GetGLXPrivScreenConfig(dpy, vis->screen, &priv, &psc);
   if (status == Success) {
               /* Lookup attribute after first finding a match on the visual */
   return glx_config_get(config, attribute, value_return);
                              /*
   ** If we can't find the config for this visual, this visual is not
   ** supported by the OpenGL implementation on the server.
   */
   if ((status == GLX_BAD_VISUAL) && (attribute == GLX_USE_GL)) {
      *value_return = False;
                  }
      /************************************************************************/
      static void
   init_fbconfig_for_chooser(struct glx_config * config,
         {
      memset(config, 0, sizeof(struct glx_config));
   config->visualID = (XID) GLX_DONT_CARE;
            /* glXChooseFBConfig specifies different defaults for these properties than
   * glXChooseVisual.
   */
   if (fbconfig_style_tags) {
      config->doubleBufferMode = GLX_DONT_CARE;
               config->drawableType = GLX_WINDOW_BIT;
   config->visualRating = GLX_DONT_CARE;
   config->transparentPixel = GLX_NONE;
   config->transparentRed = GLX_DONT_CARE;
   config->transparentGreen = GLX_DONT_CARE;
   config->transparentBlue = GLX_DONT_CARE;
   config->transparentAlpha = GLX_DONT_CARE;
            config->xRenderable = GLX_DONT_CARE;
               }
      #define MATCH_DONT_CARE( param )        \
   do {                                  \
      if ( ((int) a-> param != (int) GLX_DONT_CARE)   \
                  } while ( 0 )
      #define MATCH_MINIMUM( param )                  \
   do {                                          \
      if ( ((int) a-> param != (int) GLX_DONT_CARE)	\
                  } while ( 0 )
      #define MATCH_EXACT( param )                    \
   do {                                          \
      if ( a-> param != b-> param) {              \
            } while ( 0 )
      /* Test that all bits from a are contained in b */
   #define MATCH_MASK(param)			\
   do {						\
      if ( ((int) a-> param != (int) GLX_DONT_CARE)	\
                  } while (0);
      /**
   * Determine if two GLXFBConfigs are compatible.
   *
   * \param a  Application specified config to test.
   * \param b  Server specified config to test against \c a.
   */
   static Bool
   fbconfigs_compatible(const struct glx_config * const a,
         {
      MATCH_DONT_CARE(doubleBufferMode);
   MATCH_DONT_CARE(visualType);
   MATCH_DONT_CARE(visualRating);
   MATCH_DONT_CARE(xRenderable);
            MATCH_MINIMUM(rgbBits);
   MATCH_MINIMUM(numAuxBuffers);
   MATCH_MINIMUM(redBits);
   MATCH_MINIMUM(greenBits);
   MATCH_MINIMUM(blueBits);
   MATCH_MINIMUM(alphaBits);
   MATCH_MINIMUM(depthBits);
   MATCH_MINIMUM(stencilBits);
   MATCH_MINIMUM(accumRedBits);
   MATCH_MINIMUM(accumGreenBits);
   MATCH_MINIMUM(accumBlueBits);
   MATCH_MINIMUM(accumAlphaBits);
   MATCH_MINIMUM(sampleBuffers);
   MATCH_MINIMUM(maxPbufferWidth);
   MATCH_MINIMUM(maxPbufferHeight);
   MATCH_MINIMUM(maxPbufferPixels);
            MATCH_DONT_CARE(stereoMode);
            MATCH_MASK(drawableType);
   MATCH_MASK(renderType);
   MATCH_DONT_CARE(sRGBCapable);
            /* There is a bug in a few of the XFree86 DDX drivers.  They contain
   * visuals with a "transparent type" of 0 when they really mean GLX_NONE.
   * Technically speaking, it is a bug in the DDX driver, but there is
   * enough of an installed base to work around the problem here.  In any
   * case, 0 is not a valid value of the transparent type, so we'll treat 0
   * from the app as GLX_DONT_CARE. We'll consider GLX_NONE from the app and
   * 0 from the server to be a match to maintain backward compatibility with
   * the (broken) drivers.
            if (a->transparentPixel != (int) GLX_DONT_CARE && a->transparentPixel != 0) {
      if (a->transparentPixel == GLX_NONE) {
      if (b->transparentPixel != GLX_NONE && b->transparentPixel != 0)
      }
   else {
                  switch (a->transparentPixel) {
   case GLX_TRANSPARENT_RGB:
      MATCH_DONT_CARE(transparentRed);
   MATCH_DONT_CARE(transparentGreen);
   MATCH_DONT_CARE(transparentBlue);
               case GLX_TRANSPARENT_INDEX:
                  default:
                        }
         /* There's some tricky language in the GLX spec about how this is supposed
   * to work.  Basically, if a given component size is either not specified
   * or the requested size is zero, it is supposed to act like PREFER_SMALLER.
   * Well, that's really hard to do with the code as-is.  This behavior is
   * closer to correct, but still not technically right.
   */
   #define PREFER_LARGER_OR_ZERO(comp)             \
   do {                                          \
      if ( ((*a)-> comp) != ((*b)-> comp) ) {     \
      if ( ((*a)-> comp) == 0 ) {               \
   return -1;                              \
   }                                         \
   else if ( ((*b)-> comp) == 0 ) {          \
   return 1;                               \
   }                                         \
   else {                                    \
   return ((*b)-> comp) - ((*a)-> comp) ;  \
         } while( 0 )
      #define PREFER_LARGER(comp)                     \
   do {                                          \
      if ( ((*a)-> comp) != ((*b)-> comp) ) {     \
            } while( 0 )
      #define PREFER_SMALLER(comp)                    \
   do {                                          \
      if ( ((*a)-> comp) != ((*b)-> comp) ) {     \
            } while( 0 )
      /**
   * Compare two GLXFBConfigs.  This function is intended to be used as the
   * compare function passed in to qsort.
   *
   * \returns If \c a is a "better" config, according to the specification of
   *          SGIX_fbconfig, a number less than zero is returned.  If \c b is
   *          better, then a number greater than zero is return.  If both are
   *          equal, zero is returned.
   * \sa qsort, glXChooseVisual, glXChooseFBConfig, glXChooseFBConfigSGIX
   */
   static int
   fbconfig_compare(struct glx_config **a, struct glx_config **b)
   {
      /* The order of these comparisons must NOT change.  It is defined by
   * the GLX 1.4 specification.
                     /* The sort order for the visualRating is GLX_NONE, GLX_SLOW, and
   * GLX_NON_CONFORMANT_CONFIG.  It just so happens that this is the
   * numerical sort order of the enums (0x8000, 0x8001, and 0x800D).
   */
            /* This isn't quite right.  It is supposed to compare the sum of the
   * components the user specifically set minimums for.
   */
   PREFER_LARGER_OR_ZERO(redBits);
   PREFER_LARGER_OR_ZERO(greenBits);
   PREFER_LARGER_OR_ZERO(blueBits);
                     if (((*a)->doubleBufferMode != (*b)->doubleBufferMode)) {
      /* Prefer single-buffer.
   */
                        PREFER_SMALLER(sampleBuffers);
            PREFER_LARGER_OR_ZERO(depthBits);
            /* This isn't quite right.  It is supposed to compare the sum of the
   * components the user specifically set minimums for.
   */
   PREFER_LARGER_OR_ZERO(accumRedBits);
   PREFER_LARGER_OR_ZERO(accumGreenBits);
   PREFER_LARGER_OR_ZERO(accumBlueBits);
                     /* None of the pbuffer or fbconfig specs say that this comparison needs
   * to happen at all, but it seems like it should.
   */
   PREFER_LARGER(maxPbufferWidth);
   PREFER_LARGER(maxPbufferHeight);
               }
         /**
   * Selects and sorts a subset of the supplied configs based on the attributes.
   * This function forms to basis of \c glXChooseFBConfig and
   * \c glXChooseFBConfigSGIX.
   *
   * \param configs   Array of pointers to possible configs.  The elements of
   *                  this array that do not meet the criteria will be set to
   *                  NULL.  The remaining elements will be sorted according to
   *                  the various visual / FBConfig selection rules.
   * \param num_configs  Number of elements in the \c configs array.
   * \param attribList   Attributes used select from \c configs.  This array is
   *                     terminated by a \c None tag.  The array is of the form
   *                     expected by \c glXChooseFBConfig (where every tag has a
   *                     value).
   * \returns The number of valid elements left in \c configs.
   *
   * \sa glXChooseFBConfig, glXChooseFBConfigSGIX
   */
   static int
   choose_fbconfig(struct glx_config ** configs, int num_configs,
         {
      struct glx_config test_config;
   int base;
            /* This is a fairly direct implementation of the selection method
   * described by GLX_SGIX_fbconfig.  Start by culling out all the
   * configs that are not compatible with the selected parameter
   * list.
            init_fbconfig_for_chooser(&test_config, GL_TRUE);
   __glXInitializeVisualConfigFromTags(&test_config, 512,
                  base = 0;
   for (i = 0; i < num_configs; i++) {
      if (fbconfigs_compatible(&test_config, configs[i])) {
      configs[base] = configs[i];
                  if (base == 0) {
                  if (base < num_configs) {
                  /* After the incompatible configs are removed, the resulting
   * list is sorted according to the rules set out in the various
   * specifications.
            qsort(configs, base, sizeof(struct glx_config *),
            }
               /*
   ** Return the visual that best matches the template.  Return None if no
   ** visual matches the template.
   */
   _GLX_PUBLIC XVisualInfo *
   glXChooseVisual(Display * dpy, int screen, int *attribList)
   {
      XVisualInfo *visualList = NULL;
   struct glx_display *priv;
   struct glx_screen *psc;
   struct glx_config test_config;
   struct glx_config *config;
            /*
   ** Get a list of all visuals, return if list is empty
   */
   if (GetGLXPrivScreenConfig(dpy, screen, &priv, &psc) != Success) {
                     /*
   ** Build a template from the defaults and the attribute list
   ** Free visual list and return if an unexpected token is encountered
   */
   init_fbconfig_for_chooser(&test_config, GL_FALSE);
   __glXInitializeVisualConfigFromTags(&test_config, 512,
                  /*
   ** Eliminate visuals that don't meet minimum requirements
   ** Compute a score for those that do
   ** Remember which visual, if any, got the highest score
   ** If no visual is acceptable, return None
   ** Otherwise, create an XVisualInfo list with just the selected X visual
   ** and return this.
   */
   for (config = psc->visuals; config != NULL; config = config->next) {
      if (fbconfigs_compatible(&test_config, config)
      && ((best_config == NULL) ||
         XVisualInfo visualTemplate;
                  visualTemplate.screen = screen;
   visualTemplate.visualid = config->visualID;
                  if (newList) {
      XFree(visualList);
   visualList = newList;
                  #ifdef GLX_USE_APPLEGL
      if(visualList && debug_get_bool_option("LIBGL_DUMP_VISUALID", false)) {
            #endif
            }
         _GLX_PUBLIC const char *
   glXQueryExtensionsString(Display * dpy, int screen)
   {
      struct glx_screen *psc;
   struct glx_display *priv;
            if (GetGLXPrivScreenConfig(dpy, screen, &priv, &psc) != Success) {
                  if (!psc->effectiveGLXexts) {
      if (!psc->serverGLXexts) {
      psc->serverGLXexts =
         #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
         #endif
                        }
      _GLX_PUBLIC const char *
   glXGetClientString(Display * dpy, int name)
   {
               switch (name) {
   case GLX_VENDOR:
         case GLX_VERSION:
         case GLX_EXTENSIONS:
         default:
            }
      _GLX_PUBLIC const char *
   glXQueryServerString(Display * dpy, int screen, int name)
   {
      struct glx_screen *psc;
   struct glx_display *priv;
            if (GetGLXPrivScreenConfig(dpy, screen, &priv, &psc) != Success) {
                  switch (name) {
   case GLX_VENDOR:
      str = &psc->serverGLXvendor;
      case GLX_VERSION:
      str = &psc->serverGLXversion;
      case GLX_EXTENSIONS:
      str = &psc->serverGLXexts;
      default:
                  if (*str == NULL) {
                     }
         /*
   ** EXT_import_context
   */
      _GLX_PUBLIC Display *
   glXGetCurrentDisplay(void)
   {
      struct glx_context *gc = __glXGetCurrentContext();
   if (gc == &dummyContext)
            }
      _GLX_PUBLIC
   GLX_ALIAS(Display *, glXGetCurrentDisplayEXT, (void), (),
            #ifndef GLX_USE_APPLEGL
   _GLX_PUBLIC GLXContext
   glXImportContextEXT(Display *dpy, GLXContextID contextID)
   {
      struct glx_display *priv = __glXInitialize(dpy);
   struct glx_screen *psc = NULL;
   xGLXQueryContextReply reply;
   CARD8 opcode;
   struct glx_context *ctx;
   int i, renderType = GLX_RGBA_TYPE; /* By default, assume RGBA context */
   XID share = None;
   struct glx_config *mode = NULL;
   uint32_t fbconfigID = 0;
   uint32_t visualID = 0;
   uint32_t screen = 0;
            if (priv == NULL)
            /* The GLX_EXT_import_context spec says:
   *
   *     "If <contextID> does not refer to a valid context, then a BadContext
   *     error is generated; if <contextID> refers to direct rendering
   *     context then no error is generated but glXImportContextEXT returns
   *     NULL."
   *
   * We can handle both conditions with the __glXIsDirect call, because
   * passing None to a GLXIsDirect request will throw GLXBadContext.
   */
   if (__glXIsDirect(dpy, contextID, NULL))
            opcode = __glXSetupForCommand(dpy);
   if (!opcode)
            /* Send the glXQueryContextInfoEXT request */
            xGLXQueryContextReq *req;
            req->reqType = opcode;
   req->glxCode = X_GLXQueryContext;
            if (_XReply(dpy, (xReply *) & reply, 0, False) &&
               for (i = 0; i < reply.n; i++) {
               _XRead(dpy, (char *)prop, sizeof(prop));
   switch (prop[0]) {
   case GLX_SCREEN:
      screen = prop[1];
   got_screen = True;
      case GLX_SHARE_CONTEXT_EXT:
      share = prop[1];
      case GLX_VISUAL_ID_EXT:
      visualID = prop[1];
      case GLX_FBCONFIG_ID:
      fbconfigID = prop[1];
      case GLX_RENDER_TYPE:
      renderType = prop[1];
            }
   UnlockDisplay(dpy);
            if (!got_screen)
            psc = GetGLXScreenConfigs(dpy, screen);
   if (psc == NULL)
            if (fbconfigID != 0) {
         } else if (visualID != 0) {
                  if (mode == NULL)
            ctx = indirect_create_context(psc, mode, NULL, renderType);
   if (ctx == NULL)
            ctx->xid = contextID;
   ctx->imported = GL_TRUE;
               }
      #endif
      _GLX_PUBLIC int
   glXQueryContext(Display * dpy, GLXContext ctx_user, int attribute, int *value)
   {
               switch (attribute) {
      case GLX_SHARE_CONTEXT_EXT:
   *value = ctx->share_xid;
      case GLX_VISUAL_ID_EXT:
      *value = ctx->config ? ctx->config->visualID : None;
      case GLX_SCREEN:
      *value = ctx->psc->scr;
      case GLX_FBCONFIG_ID:
      *value = ctx->config ? ctx->config->fbconfigID : None;
      case GLX_RENDER_TYPE:
      *value = ctx->renderType;
      default:
         }
      }
      _GLX_PUBLIC
   GLX_ALIAS(int, glXQueryContextInfoEXT,
                  _GLX_PUBLIC GLXContextID glXGetContextIDEXT(const GLXContext ctx_user)
   {
                  }
      _GLX_PUBLIC void
   glXFreeContextEXT(Display *dpy, GLXContext ctx)
   {
               if (gc == NULL || gc->xid == None)
            /* The GLX_EXT_import_context spec says:
   *
   *     "glXFreeContext does not free the server-side context information or
   *     the XID associated with the server-side context."
   *
   * Don't send any protocol.  Just destroy the client-side tracking of the
   * context.  Also, only release the context structure if it's not current.
   */
   __glXLock();
   if (gc->currentDpy) {
         } else {
         }
      }
      _GLX_PUBLIC GLXFBConfig *
   glXChooseFBConfig(Display * dpy, int screen,
         {
      struct glx_config **config_list;
               config_list = (struct glx_config **)
            if ((config_list != NULL) && (list_size > 0) && (attribList != NULL)) {
      list_size = choose_fbconfig(config_list, list_size, attribList);
   if (list_size == 0) {
      free(config_list);
                  *nitems = list_size;
      }
         _GLX_PUBLIC GLXContext
   glXCreateNewContext(Display * dpy, GLXFBConfig fbconfig,
         {
      struct glx_config *config = (struct glx_config *) fbconfig;
   struct glx_config **config_list;
   int list_size;
            if (!config) {
      __glXSendError(dpy, GLXBadFBConfig, 0, X_GLXCreateNewContext, false);
               config_list = (struct glx_config **)
            for (i = 0; i < list_size; i++) {
      if (config_list[i] == config)
      }
            if (i == list_size) {
      __glXSendError(dpy, GLXBadFBConfig, 0, X_GLXCreateNewContext, false);
               return CreateContext(dpy, config->fbconfigID, config, shareList,
      }
         _GLX_PUBLIC GLXDrawable
   glXGetCurrentReadDrawable(void)
   {
                  }
         _GLX_PUBLIC GLXFBConfig *
   glXGetFBConfigs(Display * dpy, int screen, int *nelements)
   {
      struct glx_display *priv = __glXInitialize(dpy);
   struct glx_config **config_list = NULL;
   struct glx_config *config;
   unsigned num_configs = 0;
            *nelements = 0;
   if (priv && (priv->screens != NULL)
      && (screen >= 0) && (screen < ScreenCount(dpy))
   && (priv->screens[screen]->configs != NULL)
                  for (config = priv->screens[screen]->configs; config != NULL;
      config = config->next) {
   if (config->fbconfigID != (int) GLX_DONT_CARE) {
                     config_list = malloc(num_configs * sizeof *config_list);
   if (config_list != NULL) {
      *nelements = num_configs;
   i = 0;
   for (config = priv->screens[screen]->configs; config != NULL;
      config = config->next) {
   if (config->fbconfigID != (int) GLX_DONT_CARE) {
      config_list[i] = config;
                           }
         _GLX_PUBLIC int
   glXGetFBConfigAttrib(Display * dpy, GLXFBConfig fbconfig,
         {
               if (config == NULL)
               }
         _GLX_PUBLIC XVisualInfo *
   glXGetVisualFromFBConfig(Display * dpy, GLXFBConfig fbconfig)
   {
      XVisualInfo visualTemplate;
   struct glx_config *config = (struct glx_config *) fbconfig;
            if (!config)
            /*
   ** Get a list of all visuals, return if list is empty
   */
   visualTemplate.visualid = config->visualID;
      }
      #ifndef GLX_USE_APPLEGL
   /*
   ** GLX_SGI_swap_control
   */
   _X_HIDDEN int
   glXSwapIntervalSGI(int interval)
   {
      xGLXVendorPrivateReq *req;
      #ifdef GLX_DIRECT_RENDERING
         #endif
      Display *dpy;
   CARD32 *interval_ptr;
            if (gc == &dummyContext) {
                  if (interval <= 0) {
               #ifdef GLX_DIRECT_RENDERING
      if (gc->isDirect && psc && psc->driScreen &&
            GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);
         /* Simply ignore the command if the GLX drawable has been destroyed but
   * the context is still bound.
   */
   if (pdraw)
               #endif
         dpy = gc->currentDpy;
   opcode = __glXSetupForCommand(dpy);
   if (!opcode) {
                  /* Send the glXSwapIntervalSGI request */
   LockDisplay(dpy);
   GetReqExtra(GLXVendorPrivate, sizeof(CARD32), req);
   req->reqType = opcode;
   req->glxCode = X_GLXVendorPrivate;
   req->vendorCode = X_GLXvop_SwapIntervalSGI;
            interval_ptr = (CARD32 *) (req + 1);
            UnlockDisplay(dpy);
   SyncHandle();
               }
         /*
   ** GLX_MESA_swap_control
   */
   _X_HIDDEN int
   glXSwapIntervalMESA(unsigned int interval)
   {
   #ifdef GLX_DIRECT_RENDERING
               if (interval > INT_MAX)
            if (gc != &dummyContext && gc->isDirect) {
      struct glx_screen *psc = gc->psc;
   if (psc && psc->driScreen && psc->driScreen->setSwapInterval) {
                     /* Simply ignore the command if the GLX drawable has been destroyed but
   * the context is still bound.
   */
                           #endif
            }
         _X_HIDDEN int
   glXGetSwapIntervalMESA(void)
   {
   #ifdef GLX_DIRECT_RENDERING
               if (gc != &dummyContext && gc->isDirect) {
      struct glx_screen *psc = gc->psc;
   if (psc && psc->driScreen && psc->driScreen->getSwapInterval) {
      GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);
         if (pdraw)
            #endif
            }
         /*
   ** GLX_EXT_swap_control
   */
   _X_HIDDEN void
   glXSwapIntervalEXT(Display *dpy, GLXDrawable drawable, int interval)
   {
   #ifdef GLX_DIRECT_RENDERING
               /*
   * Strictly, this should throw an error if drawable is not a Window or
   * GLXWindow. We don't actually track that, so, oh well.
   */
   if (!pdraw) {
      __glXSendError(dpy, BadWindow, drawable, 0, True);
               if (interval < 0 &&
      !__glXExtensionBitIsEnabled(pdraw->psc, EXT_swap_control_tear_bit)) {
   __glXSendError(dpy, BadValue, interval, 0, True);
      }
   if (pdraw->psc->driScreen->setSwapInterval)
      #endif
   }
         /*
   ** GLX_SGI_video_sync
   */
   _X_HIDDEN int
   glXGetVideoSyncSGI(unsigned int *count)
   {
   #ifdef GLX_DIRECT_RENDERING
      int64_t ust, msc, sbc;
   int ret;
   struct glx_context *gc = __glXGetCurrentContext();
   struct glx_screen *psc = gc->psc;
            if (gc == &dummyContext)
            if (!gc->isDirect)
            if (!gc->currentDrawable)
                     /* FIXME: Looking at the GLX_SGI_video_sync spec in the extension registry,
   * FIXME: there should be a GLX encoding for this call.  I can find no
   * FIXME: documentation for the GLX encoding.
   */
   if (psc && psc->driScreen && psc->driScreen->getDrawableMSC) {
      ret = psc->driScreen->getDrawableMSC(psc, pdraw, &ust, &msc, &sbc);
   *count = (unsigned) msc;
         #endif
            }
      _X_HIDDEN int
   glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count)
   {
         #ifdef GLX_DIRECT_RENDERING
      struct glx_screen *psc = gc->psc;
   __GLXDRIdrawable *pdraw;
   int64_t ust, msc, sbc;
      #endif
         if (divisor <= 0 || remainder < 0)
            if (gc == &dummyContext)
         #ifdef GLX_DIRECT_RENDERING
      if (!gc->isDirect)
            if (!gc->currentDrawable)
                     if (psc && psc->driScreen && psc->driScreen->waitForMSC) {
      ret = psc->driScreen->waitForMSC(pdraw, 0, divisor, remainder, &ust, &msc,
         *count = (unsigned) msc;
         #endif
            }
      #endif /* GLX_USE_APPLEGL */
      /*
   ** GLX_SGIX_fbconfig
   ** Many of these functions are aliased to GLX 1.3 entry points in the 
   ** GLX_functions table.
   */
      _GLX_PUBLIC
   GLX_ALIAS(int, glXGetFBConfigAttribSGIX,
                  _GLX_PUBLIC GLX_ALIAS(GLXFBConfigSGIX *, glXChooseFBConfigSGIX,
                        _GLX_PUBLIC GLX_ALIAS(XVisualInfo *, glXGetVisualFromFBConfigSGIX,
                  _GLX_PUBLIC GLX_ALIAS(GLXContext, glXCreateContextWithConfigSGIX,
                              _GLX_PUBLIC GLXFBConfigSGIX
   glXGetFBConfigFromVisualSGIX(Display * dpy, XVisualInfo * vis)
   {
      int attrib_list[] = { GLX_VISUAL_ID, vis->visualid, None };
   int nconfigs = 0;
   GLXFBConfig *config_list;
            config_list = glXChooseFBConfig(dpy, vis->screen, attrib_list, &nconfigs);
   if (nconfigs == 0)
            config = config_list[0];
   free(config_list);
      }
      #ifndef GLX_USE_APPLEGL
   /*
   ** GLX_OML_sync_control
   */
   _X_HIDDEN Bool
   glXGetSyncValuesOML(Display *dpy, GLXDrawable drawable,
         {
         #ifdef GLX_DIRECT_RENDERING
      int ret;
   __GLXDRIdrawable *pdraw;
      #endif
         if (!priv)
         #ifdef GLX_DIRECT_RENDERING
      pdraw = GetGLXDRIDrawable(dpy, drawable);
   psc = pdraw ? pdraw->psc : NULL;
   if (pdraw && psc->driScreen->getDrawableMSC) {
      ret = psc->driScreen->getDrawableMSC(psc, pdraw, ust, msc, sbc);
         #endif
            }
      #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   _X_HIDDEN GLboolean
   __glxGetMscRate(struct glx_screen *psc,
   int32_t * numerator, int32_t * denominator)
   {
   #if !defined(GLX_USE_WINDOWSGL)
      XF86VidModeModeLine mode_line;
   int dot_clock;
            if (XF86VidModeQueryVersion(psc->dpy, &i, &i) &&
      XF86VidModeGetModeLine(psc->dpy, psc->scr, &dot_clock, &mode_line)) {
   unsigned n = dot_clock * 1000;
      # define V_INTERLACE 0x010
   # define V_DBLSCAN   0x020
            if (mode_line.flags & V_INTERLACE)
         else if (mode_line.flags & V_DBLSCAN)
            /* The OML_sync_control spec requires that if the refresh rate is a
   * whole number, that the returned numerator be equal to the refresh
   * rate and the denominator be 1.
            if (n % d == 0) {
      n /= d;
      }
   else {
               /* This is a poor man's way to reduce a fraction.  It's far from
                  for (i = 0; f[i] != 0; i++) {
      while (n % f[i] == 0 && d % f[i] == 0) {
      d /= f[i];
                     *numerator = n;
                  #endif
            }
   #endif
      /**
   * Determine the refresh rate of the specified drawable and display.
   *
   * \param dpy          Display whose refresh rate is to be determined.
   * \param drawable     Drawable whose refresh rate is to be determined.
   * \param numerator    Numerator of the refresh rate.
   * \param denominator  Denominator of the refresh rate.
   * \return  If the refresh rate for the specified display and drawable could
   *          be calculated, True is returned.  Otherwise False is returned.
   *
   * \note This function is implemented entirely client-side.  A lot of other
   *       functionality is required to export GLX_OML_sync_control, so on
   *       XFree86 this function can be called for direct-rendering contexts
   *       when GLX_OML_sync_control appears in the client extension string.
   */
      _X_HIDDEN Bool
   glXGetMscRateOML(Display * dpy, GLXDrawable drawable,
         {
   #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL) && !defined(GLX_USE_WINDOWSGL)
               if (draw == NULL)
               #else
      (void) dpy;
   (void) drawable;
   (void) numerator;
      #endif
         }
         _X_HIDDEN int64_t
   glXSwapBuffersMscOML(Display *dpy, GLXDrawable drawable,
         {
         #ifdef GLX_DIRECT_RENDERING
      __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);
      #endif
         if (gc == &dummyContext) /* no GLX for this */
         #ifdef GLX_DIRECT_RENDERING
      if (!pdraw || !gc->isDirect)
      #endif
         /* The OML_sync_control spec says these should "generate a GLX_BAD_VALUE
   * error", but it also says "It [glXSwapBuffersMscOML] will return a value
   * of -1 if the function failed because of errors detected in the input
   * parameters"
   */
   if (divisor < 0 || remainder < 0 || target_msc < 0)
         if (divisor > 0 && remainder >= divisor)
            if (target_msc == 0 && divisor == 0 && remainder == 0)
         #ifdef GLX_DIRECT_RENDERING
      if (psc->driScreen && psc->driScreen->swapBuffers)
      return psc->driScreen->swapBuffers(pdraw, target_msc, divisor,
   #endif
            }
         _X_HIDDEN Bool
   glXWaitForMscOML(Display *dpy, GLXDrawable drawable, int64_t target_msc,
               {
   #ifdef GLX_DIRECT_RENDERING
      __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);
   struct glx_screen *psc = pdraw ? pdraw->psc : NULL;
      #endif
            /* The OML_sync_control spec says these should "generate a GLX_BAD_VALUE
   * error", but the return type in the spec is Bool.
   */
   if (divisor < 0 || remainder < 0 || target_msc < 0)
         if (divisor > 0 && remainder >= divisor)
         #ifdef GLX_DIRECT_RENDERING
      if (pdraw && psc->driScreen && psc->driScreen->waitForMSC) {
      ret = psc->driScreen->waitForMSC(pdraw, target_msc, divisor, remainder,
               #endif
            }
         _X_HIDDEN Bool
   glXWaitForSbcOML(Display *dpy, GLXDrawable drawable, int64_t target_sbc,
         {
   #ifdef GLX_DIRECT_RENDERING
      __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);
   struct glx_screen *psc = pdraw ? pdraw->psc : NULL;
      #endif
         /* The OML_sync_control spec says this should "generate a GLX_BAD_VALUE
   * error", but the return type in the spec is Bool.
   */
   if (target_sbc < 0)
         #ifdef GLX_DIRECT_RENDERING
      if (pdraw && psc->driScreen && psc->driScreen->waitForSBC) {
      ret = psc->driScreen->waitForSBC(pdraw, target_sbc, ust, msc, sbc);
         #endif
            }
      /*@}*/
         /**
   * GLX_MESA_copy_sub_buffer
   */
   #define X_GLXvop_CopySubBufferMESA 5154 /* temporary */
   _X_HIDDEN void
   glXCopySubBufferMESA(Display * dpy, GLXDrawable drawable,
         {
      xGLXVendorPrivateReq *req;
   struct glx_context *gc;
   GLXContextTag tag;
   CARD32 *drawable_ptr;
   INT32 *x_ptr, *y_ptr, *w_ptr, *h_ptr;
         #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);
   if (pdraw != NULL) {
      struct glx_screen *psc = pdraw->psc;
   if (psc->driScreen->copySubBuffer != NULL) {
                        #endif
         opcode = __glXSetupForCommand(dpy);
   if (!opcode)
            /*
   ** The calling thread may or may not have a current context.  If it
   ** does, send the context tag so the server can do a flush.
   */
   gc = __glXGetCurrentContext();
   if ((gc != &dummyContext) && (dpy == gc->currentDpy) &&
      ((drawable == gc->currentDrawable) ||
   (drawable == gc->currentReadable))) {
      }
   else {
                  LockDisplay(dpy);
   GetReqExtra(GLXVendorPrivate, sizeof(CARD32) + sizeof(INT32) * 4, req);
   req->reqType = opcode;
   req->glxCode = X_GLXVendorPrivate;
   req->vendorCode = X_GLXvop_CopySubBufferMESA;
            drawable_ptr = (CARD32 *) (req + 1);
   x_ptr = (INT32 *) (drawable_ptr + 1);
   y_ptr = (INT32 *) (drawable_ptr + 2);
   w_ptr = (INT32 *) (drawable_ptr + 3);
            *drawable_ptr = drawable;
   *x_ptr = x;
   *y_ptr = y;
   *w_ptr = width;
            UnlockDisplay(dpy);
      }
      /*@{*/
   _X_HIDDEN void
   glXBindTexImageEXT(Display *dpy, GLXDrawable drawable, int buffer,
         {
      xGLXVendorPrivateReq *req;
   struct glx_context *gc = __glXGetCurrentContext();
   CARD32 *drawable_ptr;
   INT32 *buffer_ptr;
   CARD32 *num_attrib_ptr;
   CARD32 *attrib_ptr;
   CARD8 opcode;
         #ifdef GLX_DIRECT_RENDERING
      __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);
   if (pdraw != NULL) {
      struct glx_screen *psc = pdraw->psc;
   if (psc->driScreen->bindTexImage != NULL)
                  #endif
         if (attrib_list) {
      while (attrib_list[i * 2] != None)
               opcode = __glXSetupForCommand(dpy);
   if (!opcode)
            LockDisplay(dpy);
   GetReqExtra(GLXVendorPrivate, 12 + 8 * i, req);
   req->reqType = opcode;
   req->glxCode = X_GLXVendorPrivate;
   req->vendorCode = X_GLXvop_BindTexImageEXT;
            drawable_ptr = (CARD32 *) (req + 1);
   buffer_ptr = (INT32 *) (drawable_ptr + 1);
   num_attrib_ptr = (CARD32 *) (buffer_ptr + 1);
            *drawable_ptr = drawable;
   *buffer_ptr = buffer;
            i = 0;
   if (attrib_list) {
      while (attrib_list[i * 2] != None) {
      *attrib_ptr++ = (CARD32) attrib_list[i * 2 + 0];
   *attrib_ptr++ = (CARD32) attrib_list[i * 2 + 1];
                  UnlockDisplay(dpy);
      }
      _X_HIDDEN void
   glXReleaseTexImageEXT(Display * dpy, GLXDrawable drawable, int buffer)
   {
      xGLXVendorPrivateReq *req;
   struct glx_context *gc = __glXGetCurrentContext();
   CARD32 *drawable_ptr;
   INT32 *buffer_ptr;
         #ifdef GLX_DIRECT_RENDERING
      __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);
   if (pdraw != NULL) {
      struct glx_screen *psc = pdraw->psc;
   if (psc->driScreen->releaseTexImage != NULL)
                  #endif
         opcode = __glXSetupForCommand(dpy);
   if (!opcode)
            LockDisplay(dpy);
   GetReqExtra(GLXVendorPrivate, sizeof(CARD32) + sizeof(INT32), req);
   req->reqType = opcode;
   req->glxCode = X_GLXVendorPrivate;
   req->vendorCode = X_GLXvop_ReleaseTexImageEXT;
            drawable_ptr = (CARD32 *) (req + 1);
            *drawable_ptr = drawable;
            UnlockDisplay(dpy);
      }
      /*@}*/
      #endif /* GLX_USE_APPLEGL */
      /*
   ** glXGetProcAddress support
   */
      struct name_address_pair
   {
      const char *Name;
      };
      #define GLX_FUNCTION(f) { # f, (GLvoid *) f }
   #define GLX_FUNCTION2(n,f) { # n, (GLvoid *) f }
      static const struct name_address_pair GLX_functions[] = {
      /*** GLX_VERSION_1_0 ***/
   GLX_FUNCTION(glXChooseVisual),
   GLX_FUNCTION(glXCopyContext),
   GLX_FUNCTION(glXCreateContext),
   GLX_FUNCTION(glXCreateGLXPixmap),
   GLX_FUNCTION(glXDestroyContext),
   GLX_FUNCTION(glXDestroyGLXPixmap),
   GLX_FUNCTION(glXGetConfig),
   GLX_FUNCTION(glXGetCurrentContext),
   GLX_FUNCTION(glXGetCurrentDrawable),
   GLX_FUNCTION(glXIsDirect),
   GLX_FUNCTION(glXMakeCurrent),
   GLX_FUNCTION(glXQueryExtension),
   GLX_FUNCTION(glXQueryVersion),
   GLX_FUNCTION(glXSwapBuffers),
   GLX_FUNCTION(glXUseXFont),
   GLX_FUNCTION(glXWaitGL),
            /*** GLX_VERSION_1_1 ***/
   GLX_FUNCTION(glXGetClientString),
   GLX_FUNCTION(glXQueryExtensionsString),
            /*** GLX_VERSION_1_2 ***/
            /*** GLX_VERSION_1_3 ***/
   GLX_FUNCTION(glXChooseFBConfig),
   GLX_FUNCTION(glXCreateNewContext),
   GLX_FUNCTION(glXCreatePbuffer),
   GLX_FUNCTION(glXCreatePixmap),
   GLX_FUNCTION(glXCreateWindow),
   GLX_FUNCTION(glXDestroyPbuffer),
   GLX_FUNCTION(glXDestroyPixmap),
   GLX_FUNCTION(glXDestroyWindow),
   GLX_FUNCTION(glXGetCurrentReadDrawable),
   GLX_FUNCTION(glXGetFBConfigAttrib),
   GLX_FUNCTION(glXGetFBConfigs),
   GLX_FUNCTION(glXGetSelectedEvent),
   GLX_FUNCTION(glXGetVisualFromFBConfig),
   GLX_FUNCTION(glXMakeContextCurrent),
   GLX_FUNCTION(glXQueryContext),
   GLX_FUNCTION(glXQueryDrawable),
            /*** GLX_SGIX_fbconfig ***/
   GLX_FUNCTION2(glXGetFBConfigAttribSGIX, glXGetFBConfigAttrib),
   GLX_FUNCTION2(glXChooseFBConfigSGIX, glXChooseFBConfig),
   GLX_FUNCTION(glXCreateGLXPixmapWithConfigSGIX),
   GLX_FUNCTION(glXCreateContextWithConfigSGIX),
   GLX_FUNCTION2(glXGetVisualFromFBConfigSGIX, glXGetVisualFromFBConfig),
            /*** GLX_ARB_get_proc_address ***/
            /*** GLX 1.4 ***/
         #ifndef GLX_USE_APPLEGL
      /*** GLX_SGI_swap_control ***/
            /*** GLX_SGI_video_sync ***/
   GLX_FUNCTION(glXGetVideoSyncSGI),
            /*** GLX_SGI_make_current_read ***/
   GLX_FUNCTION2(glXMakeCurrentReadSGI, glXMakeContextCurrent),
            /*** GLX_EXT_import_context ***/
   GLX_FUNCTION(glXFreeContextEXT),
   GLX_FUNCTION(glXGetContextIDEXT),
   GLX_FUNCTION2(glXGetCurrentDisplayEXT, glXGetCurrentDisplay),
   GLX_FUNCTION(glXImportContextEXT),
            /*** GLX_SGIX_pbuffer ***/
   GLX_FUNCTION(glXCreateGLXPbufferSGIX),
   GLX_FUNCTION(glXDestroyGLXPbufferSGIX),
   GLX_FUNCTION(glXQueryGLXPbufferSGIX),
   GLX_FUNCTION(glXSelectEventSGIX),
            /*** GLX_MESA_copy_sub_buffer ***/
            /*** GLX_MESA_swap_control ***/
   GLX_FUNCTION(glXSwapIntervalMESA),
            /*** GLX_OML_sync_control ***/
   GLX_FUNCTION(glXWaitForSbcOML),
   GLX_FUNCTION(glXWaitForMscOML),
   GLX_FUNCTION(glXSwapBuffersMscOML),
   GLX_FUNCTION(glXGetMscRateOML),
            /*** GLX_EXT_texture_from_pixmap ***/
   GLX_FUNCTION(glXBindTexImageEXT),
            /*** GLX_EXT_swap_control ***/
      #endif
      #if defined(GLX_DIRECT_RENDERING) && defined(GLX_USE_DRM)
      /*** DRI configuration ***/
   GLX_FUNCTION(glXGetScreenDriver),
      #endif
         /*** GLX_ARB_create_context and GLX_ARB_create_context_profile ***/
            /*** GLX_MESA_query_renderer ***/
   GLX_FUNCTION(glXQueryRendererIntegerMESA),
   GLX_FUNCTION(glXQueryRendererStringMESA),
   GLX_FUNCTION(glXQueryCurrentRendererIntegerMESA),
               #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      GLX_FUNCTION2(glXGLInteropQueryDeviceInfoMESA, MesaGLInteropGLXQueryDeviceInfo),
   GLX_FUNCTION2(glXGLInteropExportObjectMESA, MesaGLInteropGLXExportObject),
      #endif
            };
      static const GLvoid *
   get_glx_proc_address(const char *funcName)
   {
               /* try static functions */
   for (i = 0; GLX_functions[i].Name; i++) {
      if (strcmp(GLX_functions[i].Name, funcName) == 0)
                  }
      /**
   * Get the address of a named GL function.  This is the pre-GLX 1.4 name for
   * \c glXGetProcAddress.
   *
   * \param procName  Name of a GL or GLX function.
   * \returns         A pointer to the named function
   *
   * \sa glXGetProcAddress
   */
   _GLX_PUBLIC void (*glXGetProcAddressARB(const GLubyte * procName)) (void)
   {
      typedef void (*gl_function) (void);
            if (!strncmp((const char *) procName, "glX", 3))
            if (f == NULL)
         #ifdef GLX_USE_APPLEGL
      if (f == NULL)
      #endif
            }
      /**
   * Get the address of a named GL function.  This is the GLX 1.4 name for
   * \c glXGetProcAddressARB.
   *
   * \param procName  Name of a GL or GLX function.
   * \returns         A pointer to the named function
   *
   * \sa glXGetProcAddressARB
   */
   _GLX_PUBLIC
   GLX_ALIAS(__GLXextFuncPtr, glXGetProcAddress,
                  #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      PUBLIC int
   MesaGLInteropGLXQueryDeviceInfo(Display *dpy, GLXContext context,
         {
      struct glx_context *gc = (struct glx_context*)context;
                     if (!gc || gc->xid == None || !gc->isDirect) {
      __glXUnlock();
               if (!gc->vtable->interop_query_device_info) {
      __glXUnlock();
               ret = gc->vtable->interop_query_device_info(gc, out);
   __glXUnlock();
      }
      PUBLIC int
   MesaGLInteropGLXExportObject(Display *dpy, GLXContext context,
               {
      struct glx_context *gc = (struct glx_context*)context;
                     if (!gc || gc->xid == None || !gc->isDirect) {
      __glXUnlock();
               if (!gc->vtable->interop_export_object) {
      __glXUnlock();
               ret = gc->vtable->interop_export_object(gc, in, out);
   __glXUnlock();
      }
      PUBLIC int
   MesaGLInteropGLXFlushObjects(Display *dpy, GLXContext context,
                     {
      struct glx_context *gc = (struct glx_context*)context;
                     if (!gc || gc->xid == None || !gc->isDirect) {
      __glXUnlock();
               if (!gc->vtable->interop_flush_objects) {
      __glXUnlock();
               ret = gc->vtable->interop_flush_objects(gc, count, resources, sync);
   __glXUnlock();
      }
      #endif /* defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL) */
