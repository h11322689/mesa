   /*
   * Copyright © 2014 Jon Turney
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "glxclient.h"
   #include "glx_error.h"
   #include "dri_common.h"
   #include "util/macros.h"
   #include "windows/xwindowsdri.h"
   #include "windows/windowsgl.h"
      struct driwindows_display
   {
      __GLXDRIdisplay base;
      };
      struct driwindows_context
   {
      struct glx_context base;
      };
      struct driwindows_config
   {
      struct glx_config base;
      };
      struct driwindows_screen
   {
      struct glx_screen base;
   __DRIscreen *driScreen;
   __GLXDRIscreen vtable;
      };
      struct driwindows_drawable
   {
      __GLXDRIdrawable base;
      };
      /**
   * GLXDRI functions
   */
      static void
   driwindows_destroy_context(struct glx_context *context)
   {
                                             }
      static int
   driwindows_bind_context(struct glx_context *context, GLXDrawable draw, GLXDrawable read)
   {
      struct driwindows_context *pcp = (struct driwindows_context *) context;
            pdraw = (struct driwindows_drawable *) driFetchDrawable(context, draw);
                     if (pdraw == NULL || pread == NULL)
            if (windows_bind_context(pcp->windowsContext,
                     }
      static void
   driwindows_unbind_context(struct glx_context *context)
   {
                  }
      static const struct glx_context_vtable driwindows_context_vtable = {
      .destroy             = driwindows_destroy_context,
   .bind                = driwindows_bind_context,
   .unbind              = driwindows_unbind_context,
   .wait_gl             = NULL,
      };
      static struct glx_context *
   driwindows_create_context(struct glx_screen *base,
               {
      struct driwindows_context *pcp, *pcp_shared;
   struct driwindows_config *config = (struct driwindows_config *) config_base;
   struct driwindows_screen *psc = (struct driwindows_screen *) base;
            if (!psc->base.driScreen)
            /* Check the renderType value */
   if (!validate_renderType_against_config(config_base, renderType))
            if (shareList) {
      /* If the shareList context is not on this renderer, we cannot possibly
   * create a context that shares with it.
   */
   if (shareList->vtable->destroy != driwindows_destroy_context) {
                  pcp_shared = (struct driwindows_context *) shareList;
               pcp = calloc(1, sizeof *pcp);
   if (pcp == NULL)
            if (!glx_context_init(&pcp->base, &psc->base, &config->base)) {
      free(pcp);
                                          if (!pcp->windowsContext) {
      free(pcp);
                           }
      static struct glx_context *
   driwindows_create_context_attribs(struct glx_screen *base,
                                 {
      struct driwindows_context *pcp, *pcp_shared;
   struct driwindows_config *config = (struct driwindows_config *) config_base;
   struct driwindows_screen *psc = (struct driwindows_screen *) base;
            int i;
            /* Extract renderType from attribs */
   for (i = 0; i < num_attribs; i++) {
      switch (attribs[i * 2]) {
   case GLX_RENDER_TYPE:
      renderType = attribs[i * 2 + 1];
                  /*
   Perhaps we should map GLX tokens to WGL tokens, but they appear to have
   identical values, so far
            if (!psc->base.driScreen || !config_base)
            /* Check the renderType value */
   if (!validate_renderType_against_config(config_base, renderType)) {
                  if (shareList) {
      /* If the shareList context is not on this renderer, we cannot possibly
   * create a context that shares with it.
   */
   if (shareList->vtable->destroy != driwindows_destroy_context) {
                  pcp_shared = (struct driwindows_context *) shareList;
               pcp = calloc(1, sizeof *pcp);
   if (pcp == NULL)
            if (!glx_context_init(&pcp->base, &psc->base, &config->base)) {
      free(pcp);
                                 pcp->windowsContext = windows_create_context_attribs(config->pxfi,
               if (pcp->windowsContext == NULL) {
      free(pcp);
                           }
      static void
   driwindowsDestroyDrawable(__GLXDRIdrawable * pdraw)
   {
                           }
      static __GLXDRIdrawable *
   driwindowsCreateDrawable(struct glx_screen *base, XID xDrawable,
               {
      struct driwindows_drawable *pdp;
            pdp = calloc(1, sizeof(*pdp));
   if (!pdp)
            pdp->base.xDrawable = xDrawable;
   pdp->base.drawable = drawable;
            /*
      By this stage, the X drawable already exists, but the GLX drawable may
            Query the server with the XID to find the correct HWND, HPBUFFERARB or
               unsigned int type;
            if (!XWindowsDRIQueryDrawable(psc->base.dpy, base->scr, drawable, &type, &handle))
   {
      free(pdp);
               /* No handle found is a failure */
   if (!handle) {
      free(pdp);
               /* Create a new drawable */
            if (!pdp->windowsDrawable) {
      free(pdp);
                           }
      static int64_t
   driwindowsSwapBuffers(__GLXDRIdrawable * pdraw,
               {
               (void) target_msc;
   (void) divisor;
            if (flush) {
                              }
      static void
   driwindowsCopySubBuffer(__GLXDRIdrawable * pdraw,
         {
               if (flush) {
                     }
      static void
   driwindowsDestroyScreen(struct glx_screen *base)
   {
               /* Free the direct rendering per screen data */
   psc->driScreen = NULL;
      }
      static const struct glx_screen_vtable driwindows_screen_vtable = {
      .create_context         = driwindows_create_context,
   .create_context_attribs = driwindows_create_context_attribs,
   .query_renderer_integer = NULL,
      };
      static Bool
   driwindowsBindExtensions(struct driwindows_screen *psc)
   {
               const struct
   {
      char *wglext;
   char *glxext;
      } extensionMap[] = {
      { "WGL_ARB_make_current_read", "GLX_SGI_make_current_read", 0 },
   { "WGL_EXT_swap_control", "GLX_SGI_swap_control", 0 },
   //      { "WGL_ARB_render_texture", "GLX_EXT_texture_from_pixmap", 0 },
   // Not exactly equivalent, needs some more glue to be written
         { "WGL_ARB_pbuffer", "GLX_SGIX_pbuffer", 1 },
   { "WGL_ARB_multisample", "GLX_ARB_multisample", 1 },
   { "WGL_ARB_multisample", "GLX_SGIS_multisample", 1 },
   { "WGL_ARB_create_context", "GLX_ARB_create_context", 0 },
   { "WGL_ARB_create_context_profile", "GLX_ARB_create_context_profile", 0 },
   { "WGL_ARB_create_context_robustness", "GLX_ARB_create_context_robustness", 0 },
               char *wgl_extensions;
   char *gl_extensions;
                     for (i = 0; i < ARRAY_SIZE(extensionMap); i++) {
      if (strstr(wgl_extensions, extensionMap[i].wglext)) {
      __glXEnableDirectExtension(&psc->base, extensionMap[i].glxext);
      }
   else if (extensionMap[i].mandatory) {
      ErrorMessageF("required WGL extension %s is missing\n", extensionMap[i].wglext);
                  /*
      Because it pre-dates WGL_EXT_extensions_string, GL_WIN_swap_hint might
      */
   if (strstr(gl_extensions, "GL_WIN_swap_hint")) {
      psc->copySubBuffer = 1;
   __glXEnableDirectExtension(&psc->base, "GLX_MESA_copy_sub_buffer");
               free(gl_extensions);
               }
      static struct glx_config *
   driwindowsMapConfigs(struct glx_display *priv, int screen, struct glx_config *configs, struct glx_config *fbconfigs)
   {
               tail = &head;
            for (m = configs; m; m = m->next) {
      int fbconfigID = GLX_DONT_CARE;
   if (fbconfigs) {
      /*
   visuals have fbconfigID of GLX_DONT_CARE, so search for a fbconfig
   with matching visualID and get the fbconfigID from there
   */
   struct glx_config *f;
   for (f = fbconfigs; f; f = f->next) {
      if (f->visualID == m->visualID)
         }
   else {
                  int pxfi;
   XWindowsDRIFBConfigToPixelFormat(priv->dpy, screen, fbconfigID, &pxfi);
   if (pxfi == 0)
                     tail->next = &config->base;
   if (tail->next == NULL)
            config->base = *m;
                           }
      static struct glx_screen *
   driwindowsCreateScreen(int screen, struct glx_display *priv)
   {
      __GLXDRIscreen *psp;
   struct driwindows_screen *psc;
   struct glx_config *configs = NULL, *visuals = NULL;
            psc = calloc(1, sizeof *psc);
   if (psc == NULL)
            if (!glx_screen_init(&psc->base, screen, priv)) {
      free(psc);
               if (!XWindowsDRIQueryDirectRenderingCapable(psc->base.dpy, screen, &directCapable) ||
      !directCapable) {
   ErrorMessageF("Screen is not Windows-DRI capable\n");
               /* discover native supported extensions */
   if (!driwindowsBindExtensions(psc)) {
                  /* Augment configs with pxfi information */
   configs = driwindowsMapConfigs(priv, screen, psc->base.configs, NULL);
            if (!configs || !visuals) {
      ErrorMessageF("No fbConfigs or visuals found\n");
               glx_config_destroy_list(psc->base.configs);
   psc->base.configs = configs;
   glx_config_destroy_list(psc->base.visuals);
            psc->base.vtable = &driwindows_screen_vtable;
   psp = &psc->vtable;
   psc->base.driScreen = psp;
   psp->destroyScreen = driwindowsDestroyScreen;
   psp->createDrawable = driwindowsCreateDrawable;
            if (psc->copySubBuffer)
                  handle_error:
                  }
      /* Called from __glXFreeDisplayPrivate.
   */
   static void
   driwindowsDestroyDisplay(__GLXDRIdisplay * dpy)
   {
         }
      /*
   * Allocate, initialize and return a  __GLXDRIdisplay object.
   * This is called from __glXInitialize() when we are given a new
   * display pointer.
   */
   _X_HIDDEN __GLXDRIdisplay *
   driwindowsCreateDisplay(Display * dpy)
   {
               int eventBase, errorBase;
            /* Verify server has Windows-DRI extension */
   if (!XWindowsDRIQueryExtension(dpy, &eventBase, &errorBase)) {
      ErrorMessageF("Windows-DRI extension not available\n");
               if (!XWindowsDRIQueryVersion(dpy, &major, &minor, &patch)) {
      ErrorMessageF("Fetching Windows-DRI extension version failed\n");
               if (!windows_check_renderer()) {
      ErrorMessageF("Windows-DRI extension disabled for GDI Generic renderer\n");
               pdpyp = malloc(sizeof *pdpyp);
   if (pdpyp == NULL)
            pdpyp->base.destroyDisplay = driwindowsDestroyDisplay;
                        }
