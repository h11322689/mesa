   /*
   * Copyright 2008 George Sapountzis
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
      #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      #include <xcb/xcb.h>
   #include <xcb/xproto.h>
   #include <xcb/shm.h>
   #include <X11/Xlib.h>
   #include <X11/Xlib-xcb.h>
   #include "glxclient.h"
   #include <dlfcn.h>
   #include "dri_common.h"
   #include "drisw_priv.h"
   #include "dri3_priv.h"
   #include <X11/extensions/shmproto.h>
   #include <assert.h>
   #include <vulkan/vulkan_core.h>
   #include <vulkan/vulkan_xcb.h>
   #include "util/u_debug.h"
   #include "kopper_interface.h"
   #include "loader_dri_helper.h"
      static int xshm_error = 0;
   static int xshm_opcode = -1;
      /**
   * Catches potential Xlib errors.
   */
   static int
   handle_xerror(Display *dpy, XErrorEvent *event)
   {
               assert(xshm_opcode != -1);
   if (event->request_code != xshm_opcode)
            xshm_error = event->error_code;
      }
      static Bool
   XCreateDrawable(struct drisw_drawable * pdp, int shmid, Display * dpy)
   {
      if (pdp->ximage) {
      XDestroyImage(pdp->ximage);
   pdp->ximage = NULL;
   if ((pdp->shminfo.shmid > 0) && (shmid != pdp->shminfo.shmid))
               if (!xshm_error && shmid >= 0) {
      pdp->shminfo.shmid = shmid;
   pdp->ximage = XShmCreateImage(dpy,
                                 NULL,
   if (pdp->ximage != NULL) {
                              old_handler = XSetErrorHandler(handle_xerror);
   /* This may trigger the X protocol error we're ready to catch: */
                  if (xshm_error) {
   /* we are on a remote display, this error is normal, don't print it */
      XDestroyImage(pdp->ximage);
                              if (pdp->ximage == NULL) {
      pdp->shminfo.shmid = -1;
   pdp->ximage = XCreateImage(dpy,
                              NULL,
   pdp->xDepth,
         /**
      * swrast does not handle 24-bit depth with 24 bpp, so let X do the
   * the conversion for us.
      if (pdp->ximage->bits_per_pixel == 24)
                  }
      static void
   XDestroyDrawable(struct drisw_drawable * pdp, Display * dpy, XID drawable)
   {
      if (pdp->ximage)
            if (pdp->shminfo.shmid > 0)
               }
      /**
   * swrast loader functions
   */
      static void
   swrastGetDrawableInfo(__DRIdrawable * draw,
               {
      struct drisw_drawable *pdp = loaderPrivate;
   __GLXDRIdrawable *pdraw = &(pdp->base);
   Display *dpy = pdraw->psc->dpy;
            Window root;
                     XGetGeometry(dpy, drawable, &root, x, y, &uw, &uh, &bw, &depth);
   *w = uw;
      }
      /**
   * Align renderbuffer pitch.
   *
   * This should be chosen by the driver and the loader (libGL, xserver/glx)
   * should use the driver provided pitch.
   *
   * It seems that the xorg loader (that is the xserver loading swrast_dri for
   * indirect rendering, not client-side libGL) requires that the pitch is
   * exactly the image width padded to 32 bits. XXX
   *
   * The above restriction can probably be overcome by using ScratchPixmap and
   * CopyArea in the xserver, similar to ShmPutImage, and setting the width of
   * the scratch pixmap to 'pitch / cpp'.
   */
   static inline int
   bytes_per_line(unsigned pitch_bits, unsigned mul)
   {
                  }
      static void
   swrastXPutImage(__DRIdrawable * draw, int op,
                     {
      struct drisw_drawable *pdp = loaderPrivate;
   __GLXDRIdrawable *pdraw = &(pdp->base);
   Display *dpy = pdraw->psc->dpy;
   Drawable drawable;
   XImage *ximage;
            if (!pdp->ximage || shmid != pdp->shminfo.shmid) {
      if (!XCreateDrawable(pdp, shmid, dpy))
               drawable = pdraw->xDrawable;
   ximage = pdp->ximage;
   ximage->bytes_per_line = stride ? stride : bytes_per_line(w * ximage->bits_per_pixel, 32);
            ximage->width = ximage->bytes_per_line / ((ximage->bits_per_pixel + 7)/ 8);
            if (pdp->shminfo.shmid >= 0) {
      XShmPutImage(dpy, drawable, gc, ximage, srcx, srcy, x, y, w, h, False);
      } else {
         }
      }
      static void
   swrastPutImageShm(__DRIdrawable * draw, int op,
                     {
               if (!pdp)
            pdp->shminfo.shmaddr = shmaddr;
   swrastXPutImage(draw, op, 0, 0, x, y, w, h, stride, shmid,
      }
      static void
   swrastPutImageShm2(__DRIdrawable * draw, int op,
                     int x, int y,
   {
               if (!pdp)
            pdp->shminfo.shmaddr = shmaddr;
   swrastXPutImage(draw, op, x, 0, x, y, w, h, stride, shmid,
      }
      static void
   swrastPutImage2(__DRIdrawable * draw, int op,
               {
      if (!loaderPrivate)
            swrastXPutImage(draw, op, 0, 0, x, y, w, h, stride, -1,
      }
      static void
   swrastPutImage(__DRIdrawable * draw, int op,
               {
      if (!loaderPrivate)
            swrastXPutImage(draw, op, 0, 0, x, y, w, h, 0, -1,
      }
      static void
   swrastGetImage2(__DRIdrawable * read,
               {
      struct drisw_drawable *prp = loaderPrivate;
   __GLXDRIdrawable *pread = &(prp->base);
   Display *dpy = pread->psc->dpy;
   Drawable readable;
            if (!prp->ximage || prp->shminfo.shmid >= 0) {
      if (!XCreateDrawable(prp, -1, dpy))
                        ximage = prp->ximage;
   ximage->data = data;
   ximage->width = w;
   ximage->height = h;
                        }
      static void
   swrastGetImage(__DRIdrawable * read,
               {
         }
      static GLboolean
   swrastGetImageShm2(__DRIdrawable * read,
               {
      struct drisw_drawable *prp = loaderPrivate;
   __GLXDRIdrawable *pread = &(prp->base);
   Display *dpy = pread->psc->dpy;
   Drawable readable;
            if (!prp->ximage || shmid != prp->shminfo.shmid) {
      if (!XCreateDrawable(prp, shmid, dpy))
               if (prp->shminfo.shmid == -1)
                  ximage = prp->ximage;
   ximage->data = prp->shminfo.shmaddr; /* no offset */
   ximage->width = w;
   ximage->height = h;
            XShmGetImage(dpy, readable, ximage, x, y, ~0L);
      }
      static void
   swrastGetImageShm(__DRIdrawable * read,
               {
         }
      static const __DRIswrastLoaderExtension swrastLoaderExtension_shm = {
               .getDrawableInfo     = swrastGetDrawableInfo,
   .putImage            = swrastPutImage,
   .getImage            = swrastGetImage,
   .putImage2           = swrastPutImage2,
   .getImage2           = swrastGetImage2,
   .putImageShm         = swrastPutImageShm,
   .getImageShm         = swrastGetImageShm,
   .putImageShm2        = swrastPutImageShm2,
      };
      static const __DRIswrastLoaderExtension swrastLoaderExtension = {
               .getDrawableInfo     = swrastGetDrawableInfo,
   .putImage            = swrastPutImage,
   .getImage            = swrastGetImage,
   .putImage2           = swrastPutImage2,
      };
      static_assert(sizeof(struct kopper_vk_surface_create_storage) >= sizeof(VkXcbSurfaceCreateInfoKHR), "");
      static void
   kopperSetSurfaceCreateInfo(void *_draw, struct kopper_loader_info *out)
   {
      __GLXDRIdrawable *draw = _draw;
            xcb->sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
   xcb->pNext = NULL;
   xcb->flags = 0;
   xcb->connection = XGetXCBConnection(draw->psc->dpy);
      }
      static const __DRIkopperLoaderExtension kopperLoaderExtension = {
                  };
      static const __DRIextension *loader_extensions_shm[] = {
      &swrastLoaderExtension_shm.base,
   &kopperLoaderExtension.base,
      };
      static const __DRIextension *loader_extensions_noshm[] = {
      &swrastLoaderExtension.base,
   &kopperLoaderExtension.base,
      };
      extern const __DRIuseInvalidateExtension dri2UseInvalidate;
   extern const __DRIbackgroundCallableExtension driBackgroundCallable;
      static const __DRIextension *kopper_extensions_noshm[] = {
      &swrastLoaderExtension.base,
   &kopperLoaderExtension.base,
   &dri2UseInvalidate.base,
   &driBackgroundCallable.base,
      };
      /**
   * GLXDRI functions
   */
      static void
   drisw_destroy_context(struct glx_context *context)
   {
                                             }
      static int
   drisw_bind_context(struct glx_context *context, GLXDrawable draw, GLXDrawable read)
   {
      struct drisw_screen *psc = (struct drisw_screen *) context->psc;
            pdraw = (struct drisw_drawable *) driFetchDrawable(context, draw);
                     if (!psc->core->bindContext(context->driContext,
                     if (psc->f) {
      if (pdraw)
         if (pread && (!pdraw || pread->driDrawable != pdraw->driDrawable))
                  }
      static void
   drisw_unbind_context(struct glx_context *context)
   {
                  }
      static void
   drisw_wait_gl(struct glx_context *context)
   {
         }
      static void
   drisw_wait_x(struct glx_context *context)
   {
         }
      static void
   drisw_bind_tex_image(__GLXDRIdrawable *base,
         {
      struct glx_context *gc = __glXGetCurrentContext();
   struct drisw_drawable *pdraw = (struct drisw_drawable *) base;
            if (pdraw != NULL) {
               if (!psc->texBuffer)
            if (psc->texBuffer->base.version >= 2 &&
      psc->texBuffer->setTexBuffer2 != NULL) {
   psc->texBuffer->setTexBuffer2(gc->driContext,
                  }
   else {
      psc->texBuffer->setTexBuffer(gc->driContext,
                  }
      static void
   drisw_release_tex_image(__GLXDRIdrawable *base, int buffer)
   {
      struct glx_context *gc = __glXGetCurrentContext();
   struct drisw_drawable *pdraw = (struct drisw_drawable *) base;
            if (pdraw != NULL) {
               if (!psc->texBuffer)
            if (psc->texBuffer->base.version >= 3 &&
      psc->texBuffer->releaseTexBuffer != NULL) {
   psc->texBuffer->releaseTexBuffer(gc->driContext,
                  }
      static int
   kopper_get_buffer_age(__GLXDRIdrawable *pdraw)
   {
               if (pdp) {
               if (psc->kopper)
      }
      }
      static const struct glx_context_vtable drisw_context_vtable = {
      .destroy             = drisw_destroy_context,
   .bind                = drisw_bind_context,
   .unbind              = drisw_unbind_context,
   .wait_gl             = drisw_wait_gl,
      };
      static struct glx_context *
   drisw_create_context_attribs(struct glx_screen *base,
                                 {
      struct glx_context *pcp, *pcp_shared;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   struct drisw_screen *psc = (struct drisw_screen *) base;
            struct dri_ctx_attribs dca;
   uint32_t ctx_attribs[2 * 5];
            if (!psc->base.driScreen)
            *error = dri_convert_glx_attribs(num_attribs, attribs, &dca);
   if (*error != __DRI_CTX_ERROR_SUCCESS)
            /* Check the renderType value */
   if (!validate_renderType_against_config(config_base, dca.render_type)) {
      *error = BadValue;
               if (shareList) {
      /* We can't share with an indirect context */
   if (!shareList->isDirect)
            /* The GLX_ARB_create_context_no_error specs say:
   *
   *    BadMatch is generated if the value of GLX_CONTEXT_OPENGL_NO_ERROR_ARB
   *    used to create <share_context> does not match the value of
   *    GLX_CONTEXT_OPENGL_NO_ERROR_ARB for the context being created.
   */
   if (!!shareList->noError != !!dca.no_error) {
      *error = BadMatch;
               pcp_shared = (struct glx_context *) shareList;
               pcp = calloc(1, sizeof *pcp);
   if (pcp == NULL)
            if (!glx_context_init(pcp, &psc->base, config_base)) {
      free(pcp);
               ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_MAJOR_VERSION;
   ctx_attribs[num_ctx_attribs++] = dca.major_ver;
   ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_MINOR_VERSION;
   ctx_attribs[num_ctx_attribs++] = dca.minor_ver;
   if (dca.reset != __DRI_CTX_RESET_NO_NOTIFICATION) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_RESET_STRATEGY;
               if (dca.release != __DRI_CTX_RELEASE_BEHAVIOR_FLUSH) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_RELEASE_BEHAVIOR;
      }
   if (dca.no_error) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_NO_ERROR;
   ctx_attribs[num_ctx_attribs++] = GL_TRUE;
               if (dca.flags != 0) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_FLAGS;
                        pcp->driContext =
      psc->swrast->createContextAttribs(psc->driScreen,
                                    dca.api,
            if (pcp->driContext == NULL) {
      free(pcp);
                           }
      static void
   driswDestroyDrawable(__GLXDRIdrawable * pdraw)
   {
      struct drisw_drawable *pdp = (struct drisw_drawable *) pdraw;
                     XDestroyDrawable(pdp, pdraw->psc->dpy, pdraw->drawable);
      }
      static __GLXDRIdrawable *
   driswCreateDrawable(struct glx_screen *base, XID xDrawable,
               {
      struct drisw_drawable *pdp;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) modes;
   unsigned depth;
   struct drisw_screen *psc = (struct drisw_screen *) base;
   const __DRIswrastExtension *swrast = psc->swrast;
   const __DRIkopperExtension *kopper = psc->kopper;
            xcb_connection_t *conn = XGetXCBConnection(dpy);
   xcb_generic_error_t *error;
   xcb_get_geometry_cookie_t cookie = xcb_get_geometry(conn, xDrawable);
   xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(conn, cookie, &error);
   if (reply)
         free(reply);
   if (!reply || error)
            pdp = calloc(1, sizeof(*pdp));
   if (!pdp)
            pdp->base.xDrawable = xDrawable;
   pdp->base.drawable = drawable;
   pdp->base.psc = &psc->base;
   pdp->config = modes;
   pdp->gc = XCreateGC(dpy, xDrawable, 0, NULL);
            /* Use the visual depth, if this fbconfig corresponds to a visual */
   if (pdp->config->visualID != 0) {
      int matches = 0;
            template.visualid = pdp->config->visualID;
   template.screen = pdp->config->screen;
   visinfo = XGetVisualInfo(dpy, VisualIDMask | VisualScreenMask,
            if (visinfo && matches) {
      pdp->xDepth = visinfo->depth;
                  /* Otherwise, or if XGetVisualInfo failed, ask the server */
   if (pdp->xDepth == 0) {
                  /* Create a new drawable */
   if (kopper) {
      pdp->driDrawable =
      kopper->createNewDrawable(psc->driScreen, config->driConfig, pdp,
   &(__DRIkopperDrawableInfo){
      .multiplanes_available = psc->has_multibuffer,
            pdp->swapInterval = dri_get_initial_swap_interval(psc->driScreen, psc->config);
      }
   else
      pdp->driDrawable =
         if (!pdp->driDrawable) {
      XDestroyDrawable(pdp, psc->base.dpy, xDrawable);
   free(pdp);
                           }
      static int64_t
   driswSwapBuffers(__GLXDRIdrawable * pdraw,
               {
      struct drisw_drawable *pdp = (struct drisw_drawable *) pdraw;
            (void) target_msc;
   (void) divisor;
            if (flush) {
                  if (psc->kopper)
                        }
      static void
   driswCopySubBuffer(__GLXDRIdrawable * pdraw,
         {
      struct drisw_drawable *pdp = (struct drisw_drawable *) pdraw;
            if (flush) {
                     }
      static void
   driswDestroyScreen(struct glx_screen *base)
   {
               /* Free the direct rendering per screen data */
   psc->core->destroyScreen(psc->driScreen);
   driDestroyConfigs(psc->driver_configs);
   psc->driScreen = NULL;
   if (psc->driver)
            }
      static char *
   drisw_get_driver_name(struct glx_screen *glx_screen)
   {
      struct drisw_screen *psc = (struct drisw_screen *) glx_screen;
      }
      static const struct glx_screen_vtable drisw_screen_vtable = {
      .create_context         = dri_common_create_context,
   .create_context_attribs = drisw_create_context_attribs,
   .query_renderer_integer = drisw_query_renderer_integer,
   .query_renderer_string  = drisw_query_renderer_string,
      };
      static void
   driswBindExtensions(struct drisw_screen *psc, const __DRIextension **extensions)
   {
               __glXEnableDirectExtension(&psc->base, "GLX_SGI_make_current_read");
   __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context");
   __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context_profile");
   __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context_no_error");
            /* DRISW version >= 2 implies support for OpenGL ES. */
   __glXEnableDirectExtension(&psc->base,
         __glXEnableDirectExtension(&psc->base,
            if (psc->copySubBuffer)
            /* FIXME: Figure out what other extensions can be ported here from dri2. */
   static const struct dri_extension_match exts[] = {
      { __DRI_TEX_BUFFER, 1, offsetof(struct drisw_screen, texBuffer), true },
   { __DRI2_RENDERER_QUERY, 1, offsetof(struct drisw_screen, rendererQuery), true },
   { __DRI2_FLUSH, 1, offsetof(struct drisw_screen, f), true },
      };
            /* Extensions where we don't care about the extension struct */
   for (i = 0; extensions[i]; i++) {
      if (strcmp(extensions[i]->name, __DRI2_ROBUSTNESS) == 0)
                  if (strcmp(extensions[i]->name, __DRI2_FLUSH_CONTROL) == 0) {
      __glXEnableDirectExtension(&psc->base,
                  if (psc->texBuffer)
            if (psc->rendererQuery) {
                  if (psc->kopper) {
      __glXEnableDirectExtension(&psc->base, "GLX_EXT_buffer_age");
   __glXEnableDirectExtension(&psc->base, "GLX_EXT_swap_control");
   __glXEnableDirectExtension(&psc->base, "GLX_SGI_swap_control");
   __glXEnableDirectExtension(&psc->base, "GLX_MESA_swap_control");
   // This needs to check whether RELAXED is available
         }
      static int
   check_xshm(Display *dpy)
   {
      xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_void_cookie_t cookie;
   xcb_generic_error_t *error;
   int ret = True;
   xcb_query_extension_cookie_t shm_cookie;
   xcb_query_extension_reply_t *shm_reply;
            shm_cookie = xcb_query_extension(c, 7, "MIT-SHM");
   shm_reply = xcb_query_extension_reply(c, shm_cookie, NULL);
            has_mit_shm = shm_reply->present;
   free(shm_reply);
   if (!has_mit_shm)
            cookie = xcb_shm_detach_checked(c, 0);
   if ((error = xcb_request_check(c, cookie))) {
      /* BadRequest means we're a remote client. If we were local we'd
   * expect BadValue since 'info' has an invalid segment name.
   */
   if (error->error_code == BadRequest)
                        }
      static int
   kopperSetSwapInterval(__GLXDRIdrawable *pdraw, int interval)
   {
      struct drisw_drawable *pdp = (struct drisw_drawable *) pdraw;
            if (!dri_valid_swap_interval(psc->driScreen, psc->config, interval))
            psc->kopper->setSwapInterval(pdp->driDrawable, interval);
               }
      static int
   kopperGetSwapInterval(__GLXDRIdrawable *pdraw)
   {
                  }
      static struct glx_screen *
   driswCreateScreenDriver(int screen, struct glx_display *priv,
         {
      __GLXDRIscreen *psp;
   const __DRIconfig **driver_configs;
   const __DRIextension **extensions;
   struct drisw_screen *psc;
   struct glx_config *configs = NULL, *visuals = NULL;
   const __DRIextension **loader_extensions_local;
            psc = calloc(1, sizeof *psc);
   if (psc == NULL)
            if (!glx_screen_init(&psc->base, screen, priv)) {
      free(psc);
               extensions = driOpenDriver(driver, &psc->driver);
   if (extensions == NULL)
                  if (pdpyp->zink)
         else if (!check_xshm(psc->base.dpy))
         else
            static const struct dri_extension_match exts[] = {
      { __DRI_CORE, 1, offsetof(struct drisw_screen, core), false },
   { __DRI_SWRAST, 4, offsetof(struct drisw_screen, swrast), false },
   { __DRI_KOPPER, 1, offsetof(struct drisw_screen, kopper), true },
   { __DRI_COPY_SUB_BUFFER, 1, offsetof(struct drisw_screen, copySubBuffer), true },
      };
   if (!loader_bind_extensions(psc, exts, ARRAY_SIZE(exts), extensions))
            psc->driScreen =
      psc->swrast->createNewScreen2(screen, loader_extensions_local,
            if (psc->driScreen == NULL) {
      ErrorMessageF("glx: failed to create drisw screen\n");
               extensions = psc->core->getExtensions(psc->driScreen);
            configs = driConvertConfigs(psc->core, psc->base.configs, driver_configs);
            if (!configs || !visuals) {
      ErrorMessageF("No matching fbConfigs or visuals found\n");
               if (pdpyp->zink) {
      bool err;
   psc->has_multibuffer = dri3_check_multibuffer(priv->dpy, &err);
   if (!psc->has_multibuffer &&
      !debug_get_bool_option("LIBGL_ALWAYS_SOFTWARE", false) &&
   !debug_get_bool_option("LIBGL_KOPPER_DRI2", false)) {
   CriticalErrorMessageF("DRI3 not available\n");
                  glx_config_destroy_list(psc->base.configs);
   psc->base.configs = configs;
   glx_config_destroy_list(psc->base.visuals);
                     psc->base.vtable = &drisw_screen_vtable;
   psc->base.context_vtable = &drisw_context_vtable;
   psp = &psc->vtable;
   psc->base.driScreen = psp;
   psp->destroyScreen = driswDestroyScreen;
   psp->createDrawable = driswCreateDrawable;
   psp->swapBuffers = driswSwapBuffers;
   psp->bindTexImage = drisw_bind_tex_image;
            if (psc->copySubBuffer)
            if (psc->kopper) {
      psp->getBufferAge = kopper_get_buffer_age;
   psp->setSwapInterval = kopperSetSwapInterval;
   psp->getSwapInterval = kopperGetSwapInterval;
                     handle_error:
      if (configs)
         if (visuals)
         if (psc->driScreen)
                  if (psc->driver)
         glx_screen_cleanup(&psc->base);
                        }
      static struct glx_screen *
   driswCreateScreen(int screen, struct glx_display *priv)
   {
      const struct drisw_display *pdpyp = (struct drisw_display *)priv->driswDisplay;
   if (pdpyp->zink && !debug_get_bool_option("LIBGL_KOPPER_DISABLE", false)) {
                     }
      /* Called from __glXFreeDisplayPrivate.
   */
   static void
   driswDestroyDisplay(__GLXDRIdisplay * dpy)
   {
         }
      /*
   * Allocate, initialize and return a __DRIdisplayPrivate object.
   * This is called from __glXInitialize() when we are given a new
   * display pointer.
   */
   _X_HIDDEN __GLXDRIdisplay *
   driswCreateDisplay(Display * dpy, bool zink)
   {
               pdpyp = malloc(sizeof *pdpyp);
   if (pdpyp == NULL)
            pdpyp->base.destroyDisplay = driswDestroyDisplay;
   pdpyp->base.createScreen = driswCreateScreen;
               }
      #endif /* GLX_DIRECT_RENDERING */
