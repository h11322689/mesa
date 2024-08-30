   /*
   * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
   * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
   *
   * SPDX-License-Identifier: SGI-B-2.0
   */
      /**
   * \file glxext.c
   * GLX protocol interface boot-strap code.
   *
   * Direct rendering support added by Precision Insight, Inc.
   *
   * \author Kevin E. Martin <kevin@precisioninsight.com>
   */
      #include <assert.h>
   #include <stdbool.h>
   #include <stdarg.h>
      #include "glxclient.h"
   #include <X11/extensions/Xext.h>
   #include <X11/extensions/extutil.h>
   #ifdef GLX_USE_APPLEGL
   #include "apple/apple_glx.h"
   #include "apple/apple_visual.h"
   #endif
   #include "glxextensions.h"
      #include "util/u_debug.h"
   #ifndef GLX_USE_APPLEGL
   #include "dri_common.h"
   #endif
      #include <X11/Xlib-xcb.h>
   #include <xcb/xcb.h>
   #include <xcb/glx.h>
      #define __GLX_MIN_CONFIG_PROPS	18
   #define __GLX_EXT_CONFIG_PROPS	32
      /*
   ** Since we send all non-core visual properties as token, value pairs,
   ** we require 2 words across the wire. In order to maintain backwards
   ** compatibility, we need to send the total number of words that the
   ** VisualConfigs are sent back in so old libraries can simply "ignore"
   ** the new properties.
   */
   #define __GLX_TOTAL_CONFIG \
            _X_HIDDEN void
   glx_message(int level, const char *f, ...)
   {
      va_list args;
   int threshold = _LOADER_WARNING;
            libgl_debug = getenv("LIBGL_DEBUG");
   if (libgl_debug) {
      if (strstr(libgl_debug, "quiet"))
         else if (strstr(libgl_debug, "verbose"))
               /* Note that the _LOADER_* levels are lower numbers for more severe. */
   if (level <= threshold) {
      va_start(args, f);
   vfprintf(stderr, f, args);
         }
      /*
   ** You can set this cell to 1 to force the gl drawing stuff to be
   ** one command per packet
   */
   _X_HIDDEN int __glXDebug = 0;
      /* Extension required boiler plate */
      static const char __glXExtensionName[] = GLX_EXTENSION_NAME;
   static struct glx_display *glx_displays;
      static /* const */ char *error_list[] = {
      "GLXBadContext",
   "GLXBadContextState",
   "GLXBadDrawable",
   "GLXBadPixmap",
   "GLXBadContextTag",
   "GLXBadCurrentWindow",
   "GLXBadRenderRequest",
   "GLXBadLargeRequest",
   "GLXUnsupportedPrivateRequest",
   "GLXBadFBConfig",
   "GLXBadPbuffer",
   "GLXBadCurrentDrawable",
   "GLXBadWindow",
      };
      #ifdef GLX_USE_APPLEGL
   static char *__glXErrorString(Display *dpy, int code, XExtCodes *codes,
         #endif
      static
   XEXT_GENERATE_ERROR_STRING(__glXErrorString, __glXExtensionName,
            /*
   * GLX events are a bit funky.  We don't stuff the X event code into
   * our user exposed (via XNextEvent) structure.  Instead we use the GLX
   * private event code namespace (and hope it doesn't conflict).  Clients
   * have to know that bit 15 in the event type field means they're getting
   * a GLX event, and then handle the various sub-event types there, rather
   * than simply checking the event code and handling it directly.
   */
      static Bool
   __glXWireToEvent(Display *dpy, XEvent *event, xEvent *wire)
   {
               if (glx_dpy == NULL)
            switch ((wire->u.u.type & 0x7f) - glx_dpy->codes.first_event) {
   case GLX_PbufferClobber:
   {
      GLXPbufferClobberEvent *aevent = (GLXPbufferClobberEvent *)event;
   xGLXPbufferClobberEvent *awire = (xGLXPbufferClobberEvent *)wire;
   aevent->event_type = awire->type;
   aevent->serial = awire->sequenceNumber;
   aevent->event_type = awire->event_type;
   aevent->draw_type = awire->draw_type;
   aevent->drawable = awire->drawable;
   aevent->buffer_mask = awire->buffer_mask;
   aevent->aux_buffer = awire->aux_buffer;
   aevent->x = awire->x;
   aevent->y = awire->y;
   aevent->width = awire->width;
   aevent->height = awire->height;
   aevent->count = awire->count;
      }
   case GLX_BufferSwapComplete:
   {
      GLXBufferSwapComplete *aevent = (GLXBufferSwapComplete *)event;
   xGLXBufferSwapComplete2 *awire = (xGLXBufferSwapComplete2 *)wire;
            return False;
            aevent->serial = _XSetLastRequestRead(dpy, (xGenericReply *) wire);
   aevent->send_event = (awire->type & 0x80) != 0;
   aevent->display = dpy;
   aevent->event_type = awire->event_type;
   aevent->drawable = glxDraw->xDrawable;
   aevent->ust = ((CARD64)awire->ust_hi << 32) | awire->ust_lo;
            /* Handle 32-Bit wire sbc wraparound in both directions to cope with out
   * of sequence 64-Bit sbc's
   */
   if ((int64_t) awire->sbc < ((int64_t) glxDraw->lastEventSbc - 0x40000000))
         if ((int64_t) awire->sbc > ((int64_t) glxDraw->lastEventSbc + 0x40000000))
         glxDraw->lastEventSbc = awire->sbc;
   aevent->sbc = awire->sbc + glxDraw->eventSbcWrap;
      }
   default:
      /* client doesn't support server event */
                  }
      /* We don't actually support this.  It doesn't make sense for clients to
   * send each other GLX events.
   */
   static Status
   __glXEventToWire(Display *dpy, XEvent *event, xEvent *wire)
   {
               if (glx_dpy == NULL)
            switch (event->type) {
   case GLX_DAMAGED:
         case GLX_SAVED:
         case GLX_EXCHANGE_COMPLETE_INTEL:
         case GLX_COPY_COMPLETE_INTEL:
         case GLX_FLIP_COMPLETE_INTEL:
         default:
      /* client doesn't support server event */
                  }
      /************************************************************************/
   /*
   ** Free the per screen configs data as well as the array of
   ** __glXScreenConfigs.
   */
   static void
   FreeScreenConfigs(struct glx_display * priv)
   {
      struct glx_screen *psc;
            /* Free screen configuration information */
   screens = ScreenCount(priv->dpy);
   for (i = 0; i < screens; i++) {
      psc = priv->screens[i];
   if (!psc)
            #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
         if (psc->driScreen) {
         free(psc);
         #else
         #endif
      }
   free((char *) priv->screens);
      }
      #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   static void
   free_zombie_glx_drawable(struct set_entry *entry)
   {
                  }
   #endif
      static void
   glx_display_free(struct glx_display *priv)
   {
               gc = __glXGetCurrentContext();
   if (priv->dpy == gc->currentDpy) {
      if (gc != &dummyContext)
            gc->vtable->destroy(gc);
                  #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
         #endif
                        #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
               /* Free the direct rendering per display data */
   if (priv->driswDisplay)
               #if defined (GLX_USE_DRM)
      if (priv->dri2Display)
                  if (priv->dri3Display)
            #endif /* GLX_USE_DRM */
      #if defined(GLX_USE_WINDOWSGL)
      if (priv->windowsdriDisplay)
            #endif /* GLX_USE_WINDOWSGL */
      #endif /* GLX_DIRECT_RENDERING && !GLX_USE_APPLEGL */
            }
      static int
   __glXCloseDisplay(Display * dpy, XExtCodes * codes)
   {
               _XLockMutex(_Xglobal_lock);
   prev = &glx_displays;
   for (priv = glx_displays; priv; prev = &priv->next, priv = priv->next) {
      if (priv->dpy == dpy) {
   break;
            }
            if (priv != NULL)
               }
      /*
   ** Query the version of the GLX extension.  This procedure works even if
   ** the client extension is not completely set up.
   */
   static Bool
   QueryVersion(Display * dpy, int opcode, int *major, int *minor)
   {
      xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_glx_query_version_reply_t *reply = xcb_glx_query_version_reply(c,
                                    if (!reply)
            if (reply->major_version != GLX_MAJOR_VERSION) {
      free(reply);
      }
   *major = reply->major_version;
   *minor = min(reply->minor_version, GLX_MINOR_VERSION);
   free(reply);
      }
      /*
   * We don't want to enable this GLX_OML_swap_method in glxext.h,
   * because we can't support it.  The X server writes it out though,
   * so we should handle it somehow, to avoid false warnings.
   */
   enum {
         };
         static GLint
   convert_from_x_visual_type(int visualType)
   {
      static const int glx_visual_types[] = {
      [StaticGray]  = GLX_STATIC_GRAY,
   [GrayScale]   = GLX_GRAY_SCALE,
   [StaticColor] = GLX_STATIC_COLOR,
   [PseudoColor] = GLX_PSEUDO_COLOR,
   [TrueColor]   = GLX_TRUE_COLOR,
               if (visualType < ARRAY_SIZE(glx_visual_types))
               }
      /*
   * getVisualConfigs uses the !tagged_only path.
   * getFBConfigs uses the tagged_only path.
   */
   _X_HIDDEN void
   __glXInitializeVisualConfigFromTags(struct glx_config * config, int count,
               {
               if (!tagged_only) {
      /* Copy in the first set of properties */
                              config->redBits = *bp++;
   config->greenBits = *bp++;
   config->blueBits = *bp++;
   config->alphaBits = *bp++;
   config->accumRedBits = *bp++;
   config->accumGreenBits = *bp++;
   config->accumBlueBits = *bp++;
            config->doubleBufferMode = *bp++;
            config->rgbBits = *bp++;
   config->depthBits = *bp++;
   config->stencilBits = *bp++;
   config->numAuxBuffers = *bp++;
      #ifdef GLX_USE_APPLEGL
         /* AppleSGLX supports pixmap and pbuffers with all config. */
   config->drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT;
   /* Unfortunately this can create an ABI compatibility problem. */
   #else
         #endif
               /*
   ** Additional properties may be in a list at the end
   ** of the reply.  They are in pairs of property type
   ** and property value.
         #define FETCH_OR_SET(tag) \
               for (i = 0; i < count; i += 2) {
               switch (tag) {
   case GLX_RGBA:
      if (fbconfig_style_tags)
         else
            case GLX_BUFFER_SIZE:
      config->rgbBits = *bp++;
      case GLX_LEVEL:
      config->level = *bp++;
      case GLX_DOUBLEBUFFER:
      FETCH_OR_SET(doubleBufferMode);
      case GLX_STEREO:
      FETCH_OR_SET(stereoMode);
      case GLX_AUX_BUFFERS:
      config->numAuxBuffers = *bp++;
      case GLX_RED_SIZE:
      config->redBits = *bp++;
      case GLX_GREEN_SIZE:
      config->greenBits = *bp++;
      case GLX_BLUE_SIZE:
      config->blueBits = *bp++;
      case GLX_ALPHA_SIZE:
      config->alphaBits = *bp++;
      case GLX_DEPTH_SIZE:
      config->depthBits = *bp++;
      case GLX_STENCIL_SIZE:
      config->stencilBits = *bp++;
      case GLX_ACCUM_RED_SIZE:
      config->accumRedBits = *bp++;
      case GLX_ACCUM_GREEN_SIZE:
      config->accumGreenBits = *bp++;
      case GLX_ACCUM_BLUE_SIZE:
      config->accumBlueBits = *bp++;
      case GLX_ACCUM_ALPHA_SIZE:
      config->accumAlphaBits = *bp++;
      case GLX_VISUAL_CAVEAT_EXT:
      config->visualRating = *bp++;
      case GLX_X_VISUAL_TYPE:
      config->visualType = *bp++;
      case GLX_TRANSPARENT_TYPE:
      config->transparentPixel = *bp++;
      case GLX_TRANSPARENT_INDEX_VALUE:
      config->transparentIndex = *bp++;
      case GLX_TRANSPARENT_RED_VALUE:
      config->transparentRed = *bp++;
      case GLX_TRANSPARENT_GREEN_VALUE:
      config->transparentGreen = *bp++;
      case GLX_TRANSPARENT_BLUE_VALUE:
      config->transparentBlue = *bp++;
      case GLX_TRANSPARENT_ALPHA_VALUE:
      config->transparentAlpha = *bp++;
      case GLX_VISUAL_ID:
      config->visualID = *bp++;
      case GLX_DRAWABLE_TYPE:
   #ifdef GLX_USE_APPLEGL
               #endif
               case GLX_RENDER_TYPE: /* fbconfig render type bits */
      config->renderType = *bp++;
      case GLX_X_RENDERABLE:
      config->xRenderable = *bp++;
      case GLX_FBCONFIG_ID:
      config->fbconfigID = *bp++;
      case GLX_MAX_PBUFFER_WIDTH:
      config->maxPbufferWidth = *bp++;
      case GLX_MAX_PBUFFER_HEIGHT:
      config->maxPbufferHeight = *bp++;
      case GLX_MAX_PBUFFER_PIXELS:
         #ifndef GLX_USE_APPLEGL
         case GLX_OPTIMAL_PBUFFER_WIDTH_SGIX:
      config->optimalPbufferWidth = *bp++;
      case GLX_OPTIMAL_PBUFFER_HEIGHT_SGIX:
      config->optimalPbufferHeight = *bp++;
      case GLX_VISUAL_SELECT_GROUP_SGIX:
         #endif
         case GLX_SAMPLE_BUFFERS_SGIS:
      config->sampleBuffers = *bp++;
      case GLX_SAMPLES_SGIS:
      config->samples = *bp++;
      case IGNORE_GLX_SWAP_METHOD_OML:
      /* We ignore this tag.  See the comment above this function. */
      #ifndef GLX_USE_APPLEGL
         case GLX_BIND_TO_TEXTURE_RGB_EXT:
      config->bindToTextureRgb = *bp++;
      case GLX_BIND_TO_TEXTURE_RGBA_EXT:
      config->bindToTextureRgba = *bp++;
      case GLX_BIND_TO_MIPMAP_TEXTURE_EXT:
      config->bindToMipmapTexture = *bp++;
      case GLX_BIND_TO_TEXTURE_TARGETS_EXT:
      config->bindToTextureTargets = *bp++;
      case GLX_Y_INVERTED_EXT:
         #endif
         case GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT:
                  case GLX_USE_GL:
      if (fbconfig_style_tags)
            case GLX_FLOAT_COMPONENTS_NV:
      config->floatComponentsNV = *bp++;
      case None:
      i = count;
      default: {
         long int tagvalue = *bp++;
   DebugMessageF("WARNING: unknown fbconfig attribute from server: "
                     }
      static struct glx_config *
   createConfigsFromProperties(Display * dpy, int nvisuals, int nprops,
         {
      INT32 buf[__GLX_TOTAL_CONFIG], *props;
   unsigned prop_size;
   struct glx_config *modes, *m;
            if (nprops == 0)
            /* Check number of properties */
   if (nprops < __GLX_MIN_CONFIG_PROPS)
            /* Allocate memory for our config structure */
   modes = glx_config_create_list(nvisuals);
   if (!modes)
            prop_size = nprops * __GLX_SIZE_INT32;
   if (prop_size <= sizeof(buf))
         else
            /* Read each config structure and convert it into our format */
   m = modes;
   for (i = 0; i < nvisuals; i++) {
      _XRead(dpy, (char *) props, prop_size);
   /* If this is GLXGetVisualConfigs then the reply will not include
   * any drawable type info, but window support is implied because
   * that's what a Visual describes, and pixmap support is implied
   * because you almost certainly have a pixmap format corresponding
   * to your visual format. 
   */
   if (!tagged_only)
         __glXInitializeVisualConfigFromTags(m, nprops, props,
         m->screen = screen;
               if (props != buf)
               }
      static GLboolean
   getVisualConfigs(struct glx_screen *psc,
         {
      xGLXGetVisualConfigsReq *req;
   xGLXGetVisualConfigsReply reply;
                     psc->visuals = NULL;
   GetReq(GLXGetVisualConfigs, req);
   req->reqType = priv->codes.major_opcode;
   req->glxCode = X_GLXGetVisualConfigs;
            if (!_XReply(dpy, (xReply *) & reply, 0, False))
            psc->visuals = createConfigsFromProperties(dpy,
                     out:
      UnlockDisplay(dpy);
      }
      static GLboolean
   getFBConfigs(struct glx_screen *psc, struct glx_display *priv, int screen)
   {
      xGLXGetFBConfigsReq *fb_req;
   xGLXGetFBConfigsReply reply;
                     if (psc->serverGLXexts == NULL) {
                           psc->configs = NULL;
   GetReq(GLXGetFBConfigs, fb_req);
   fb_req->reqType = priv->codes.major_opcode;
   fb_req->glxCode = X_GLXGetFBConfigs;
            if (!_XReply(dpy, (xReply *) & reply, 0, False))
            psc->configs = createConfigsFromProperties(dpy,
                     out:
      UnlockDisplay(dpy);
      }
      _X_HIDDEN Bool
   glx_screen_init(struct glx_screen *psc,
         {
      /* Initialize per screen dynamic client GLX extensions */
   psc->ext_list_first_time = GL_TRUE;
   psc->scr = screen;
   psc->dpy = priv->dpy;
            if (!getVisualConfigs(psc, priv, screen))
            if (!getFBConfigs(psc, priv, screen))
               }
      _X_HIDDEN void
   glx_screen_cleanup(struct glx_screen *psc)
   {
      if (psc->configs) {
      glx_config_destroy_list(psc->configs);
   free(psc->effectiveGLXexts);
      }
   if (psc->visuals) {
      glx_config_destroy_list(psc->visuals);
      }
   free((char *) psc->serverGLXexts);
   free((char *) psc->serverGLXvendor);
      }
      /*
   ** Allocate the memory for the per screen configs for each screen.
   ** If that works then fetch the per screen configs data.
   */
   static Bool
   AllocAndFetchScreenConfigs(Display * dpy, struct glx_display * priv, Bool zink)
   {
      struct glx_screen *psc;
   GLint i, screens;
            /*
   ** First allocate memory for the array of per screen configs.
   */
   screens = ScreenCount(dpy);
   priv->screens = calloc(screens, sizeof *priv->screens);
   if (!priv->screens)
            for (i = 0; i < screens; i++) {
      #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   #if defined(GLX_USE_DRM)
   #if defined(HAVE_DRI3)
         if (priv->dri3Display)
   #endif /* HAVE_DRI3 */
         psc = priv->dri2Display->createScreen(i, priv);
   #endif /* GLX_USE_DRM */
      #ifdef GLX_USE_WINDOWSGL
         psc = priv->windowsdriDisplay->createScreen(i, priv);
   #endif
            psc = priv->driswDisplay->createScreen(i, priv);
   #endif /* GLX_DIRECT_RENDERING && !GLX_USE_APPLEGL */
               #if defined(GLX_USE_APPLEGL)
         if (psc == NULL)
   #else
         if (psc == NULL && !zink)
   {
      psc = indirect_create_screen(i, priv);
      #endif
         priv->screens[i] = psc;
   if (psc)
            if(indirect) /* Load extensions required only for indirect glx */
      }
   if (zink && !screen_count)
         SyncHandle();
      }
      /*
   ** Initialize the client side extension code.
   */
   _X_HIDDEN struct glx_display *
   __glXInitialize(Display * dpy)
   {
      XExtCodes *codes;
   struct glx_display *dpyPriv, *d;
                     for (dpyPriv = glx_displays; dpyPriv; dpyPriv = dpyPriv->next) {
      _XUnlockMutex(_Xglobal_lock);
   return dpyPriv;
                     /* Drop the lock while we create the display private. */
            dpyPriv = calloc(1, sizeof *dpyPriv);
   if (!dpyPriv)
            codes = XInitExtension(dpy, __glXExtensionName);
   if (!codes) {
      free(dpyPriv);
               dpyPriv->codes = *codes;
            /* This GLX implementation requires GLX 1.3 */
   if (!QueryVersion(dpy, dpyPriv->codes.major_opcode,
      &majorVersion, &dpyPriv->minorVersion)
   || (majorVersion != 1)
   || (majorVersion == 1 && dpyPriv->minorVersion < 3)) {
   free(dpyPriv);
               for (i = 0; i < __GLX_NUMBER_EVENTS; i++) {
      XESetWireToEvent(dpy, dpyPriv->codes.first_event + i, __glXWireToEvent);
               XESetCloseDisplay(dpy, dpyPriv->codes.extension, __glXCloseDisplay);
                  #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      Bool glx_direct = !debug_get_bool_option("LIBGL_ALWAYS_INDIRECT", false);
   Bool glx_accel = !debug_get_bool_option("LIBGL_ALWAYS_SOFTWARE", false);
   const char *env = getenv("MESA_LOADER_DRIVER_OVERRIDE");
   Bool zink = env && !strcmp(env, "zink");
                              /* Set the logger before the *CreateDisplay functions. */
            /*
   ** Initialize the direct rendering per display data and functions.
   ** Note: This _must_ be done before calling any other DRI routines
   ** (e.g., those called in AllocAndFetchScreenConfigs).
      #if defined(GLX_USE_DRM)
         #if defined(HAVE_DRI3)
         if (!debug_get_bool_option("LIBGL_DRI3_DISABLE", false))
   #endif /* HAVE_DRI3 */
         if (!debug_get_bool_option("LIBGL_DRI2_DISABLE", false))
         if (!dpyPriv->dri3Display && !dpyPriv->dri2Display)
      try_zink = !debug_get_bool_option("LIBGL_KOPPER_DISABLE", false) &&
      #endif /* GLX_USE_DRM */
      if (glx_direct)
         #ifdef GLX_USE_WINDOWSGL
      if (glx_direct && glx_accel)
      #endif
   #endif /* GLX_DIRECT_RENDERING && !GLX_USE_APPLEGL */
      #ifdef GLX_USE_APPLEGL
      if (!applegl_create_display(dpyPriv)) {
      free(dpyPriv);
         #endif
         if (!AllocAndFetchScreenConfigs(dpy, dpyPriv, zink | try_zink)) {
      Bool fail = True;
   if (try_zink) {
      free(dpyPriv->screens);
   dpyPriv->driswDisplay->destroyDisplay(dpyPriv->driswDisplay);
   dpyPriv->driswDisplay = driswCreateDisplay(dpy, false);
      }
   if (fail) {
      free(dpyPriv);
                           /* Grab the lock again and add the display private, unless somebody
   * beat us to initializing on this display in the meantime. */
            for (d = glx_displays; d; d = d->next) {
      _XUnlockMutex(_Xglobal_lock);
   glx_display_free(dpyPriv);
   return d;
                     dpyPriv->next = glx_displays;
                        }
      /*
   ** Setup for sending a GLX command on dpy.  Make sure the extension is
   ** initialized.  Try to avoid calling __glXInitialize as its kinda slow.
   */
   _X_HIDDEN CARD8
   __glXSetupForCommand(Display * dpy)
   {
      struct glx_context *gc;
            /* If this thread has a current context, flush its rendering commands */
   gc = __glXGetCurrentContext();
   if (gc->currentDpy) {
      /* Flush rendering buffer of the current context, if any */
            if (gc->currentDpy == dpy) {
      /* Use opcode from gc because its right */
      }
   else {
      /*
   ** Have to get info about argument dpy because it might be to
   ** a different server
                  /* Forced to lookup extension via the slow initialize route */
   priv = __glXInitialize(dpy);
   if (!priv) {
         }
      }
      /**
   * Flush the drawing command transport buffer.
   *
   * \param ctx  Context whose transport buffer is to be flushed.
   * \param pc   Pointer to first unused buffer location.
   *
   * \todo
   * Modify this function to use \c ctx->pc instead of the explicit
   * \c pc parameter.
   */
   _X_HIDDEN GLubyte *
   __glXFlushRenderBuffer(struct glx_context * ctx, GLubyte * pc)
   {
      Display *const dpy = ctx->currentDpy;
   xcb_connection_t *c = XGetXCBConnection(dpy);
            if ((dpy != NULL) && (size > 0)) {
      xcb_glx_render(c, ctx->currentContextTag, size,
               /* Reset pointer and return it */
   ctx->pc = ctx->buf;
      }
         /**
   * Send a portion of a GLXRenderLarge command to the server.  The advantage of
   * this function over \c __glXSendLargeCommand is that callers can use the
   * data buffer in the GLX context and may be able to avoid allocating an
   * extra buffer.  The disadvantage is the clients will have to do more
   * GLX protocol work (i.e., calculating \c totalRequests, etc.).
   *
   * \sa __glXSendLargeCommand
   *
   * \param gc             GLX context
   * \param requestNumber  Which part of the whole command is this?  The first
   *                       request is 1.
   * \param totalRequests  How many requests will there be?
   * \param data           Command data.
   * \param dataLen        Size, in bytes, of the command data.
   */
   _X_HIDDEN void
   __glXSendLargeChunk(struct glx_context * gc, GLint requestNumber,
         {
      Display *dpy = gc->currentDpy;
   xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_glx_render_large(c, gc->currentContextTag, requestNumber,
      }
         /**
   * Send a command that is too large for the GLXRender protocol request.
   *
   * Send a large command, one that is too large for some reason to
   * send using the GLXRender protocol request.  One reason to send
   * a large command is to avoid copying the data.
   *
   * \param ctx        GLX context
   * \param header     Header data.
   * \param headerLen  Size, in bytes, of the header data.  It is assumed that
   *                   the header data will always be small enough to fit in
   *                   a single X protocol packet.
   * \param data       Command data.
   * \param dataLen    Size, in bytes, of the command data.
   */
   _X_HIDDEN void
   __glXSendLargeCommand(struct glx_context * ctx,
               {
      GLint maxSize;
            /*
   ** Calculate the maximum amount of data can be stuffed into a single
   ** packet.  sz_xGLXRenderReq is added because bufSize is the maximum
   ** packet size minus sz_xGLXRenderReq.
   */
   maxSize = (ctx->bufSize + sz_xGLXRenderReq) - sz_xGLXRenderLargeReq;
   totalRequests = 1 + (dataLen / maxSize);
   if (dataLen % maxSize)
            /*
   ** Send all of the command, except the large array, as one request.
   */
   assert(headerLen <= maxSize);
            /*
   ** Send enough requests until the whole array is sent.
   */
   for (requestNumber = 2; requestNumber <= (totalRequests - 1);
      requestNumber++) {
   __glXSendLargeChunk(ctx, requestNumber, totalRequests, data, maxSize);
   data = (const GLvoid *) (((const GLubyte *) data) + maxSize);
   dataLen -= maxSize;
               assert(dataLen <= maxSize);
      }
