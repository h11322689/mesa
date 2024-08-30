   /*
   * Copyright © 2008 Red Hat, Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Soft-
   * ware"), to deal in the Software without restriction, including without
   * limitation the rights to use, copy, modify, merge, publish, distribute,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, provided that the above copyright
   * notice(s) and this permission notice appear in all copies of the Soft-
   * ware and that both the above copyright notice(s) and this permission
   * notice appear in supporting documentation.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
   * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
   * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
   * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
   * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
   * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
   * MANCE OF THIS SOFTWARE.
   *
   * Except as contained in this notice, the name of a copyright holder shall
   * not be used in advertising or otherwise to promote the sale, use or
   * other dealings in this Software without prior written authorization of
   * the copyright holder.
   *
   * Authors:
   *   Kristian Høgsberg (krh@redhat.com)
   */
      #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      #include <X11/Xlib.h>
   #include <X11/extensions/Xfixes.h>
   #include <X11/Xlib-xcb.h>
   #include <xcb/xcb.h>
   #include <xcb/dri2.h>
   #include "glxclient.h"
   #include <X11/extensions/dri2proto.h>
   #include <dlfcn.h>
   #include <fcntl.h>
   #include <unistd.h>
   #include <sys/types.h>
   #include <sys/mman.h>
   #include <sys/time.h>
   #include "dri2.h"
   #include "dri_common.h"
   #include "dri2_priv.h"
   #include "loader.h"
   #include "loader_dri_helper.h"
      #undef DRI2_MINOR
   #define DRI2_MINOR 1
      struct dri2_display
   {
                           };
      struct dri2_drawable
   {
      __GLXDRIdrawable base;
   __DRIdrawable *driDrawable;
   __DRIbuffer buffers[5];
   int bufferCount;
   int width, height;
   int have_back;
   int have_fake_front;
            uint64_t previous_time;
      };
      static const struct glx_context_vtable dri2_context_vtable;
      /* For XCB's handling of ust/msc/sbc counters, we have to hand it the high and
   * low halves separately.  This helps you split them.
   */
   static void
   split_counter(uint64_t counter, uint32_t *hi, uint32_t *lo)
   {
      *hi = (counter >> 32);
      }
      static uint64_t
   merge_counter(uint32_t hi, uint32_t lo)
   {
         }
      static void
   dri2_destroy_context(struct glx_context *context)
   {
                                             }
      static Bool
   dri2_bind_context(struct glx_context *context, GLXDrawable draw, GLXDrawable read)
   {
      struct dri2_screen *psc = (struct dri2_screen *) context->psc;
   struct dri2_drawable *pdraw, *pread;
            pdraw = (struct dri2_drawable *) driFetchDrawable(context, draw);
                     if (pdraw)
         else if (draw != None)
            if (pread)
         else if (read != None)
            if (!psc->core->bindContext(context->driContext, dri_draw, dri_read))
               }
      static void
   dri2_unbind_context(struct glx_context *context)
   {
                  }
      static struct glx_context *
   dri2_create_context_attribs(struct glx_screen *base,
         struct glx_config *config_base,
   struct glx_context *shareList,
   unsigned num_attribs,
   const uint32_t *attribs,
   {
      struct glx_context *pcp = NULL;
   struct dri2_screen *psc = (struct dri2_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
            struct dri_ctx_attribs dca;
   uint32_t ctx_attribs[2 * 6];
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
                           pcp = calloc(1, sizeof *pcp);
   if (pcp == NULL) {
      *error = BadAlloc;
               if (!glx_context_init(pcp, &psc->base, config_base))
            ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_MAJOR_VERSION;
   ctx_attribs[num_ctx_attribs++] = dca.major_ver;
   ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_MINOR_VERSION;
            /* Only send a value when the non-default value is requested.  By doing
   * this we don't have to check the driver's DRI2 version before sending the
   * default value.
   */
   if (dca.reset != __DRI_CTX_RESET_NO_NOTIFICATION) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_RESET_STRATEGY;
               if (dca.release != __DRI_CTX_RELEASE_BEHAVIOR_FLUSH) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_RELEASE_BEHAVIOR;
               if (dca.no_error) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_NO_ERROR;
   ctx_attribs[num_ctx_attribs++] = dca.no_error;
               if (dca.flags != 0) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_FLAGS;
               /* The renderType is retrieved from attribs, or set to default
   *  of GLX_RGBA_TYPE.
   */
            pcp->driContext =
      psc->dri2->createContextAttribs(psc->driScreen,
   dca.api,
   config ? config->driConfig : NULL,
   shared,
   num_ctx_attribs / 2,
   ctx_attribs,
   error,
                  if (pcp->driContext == NULL)
                           error_exit:
                  }
      static void
   dri2DestroyDrawable(__GLXDRIdrawable *base)
   {
      struct dri2_screen *psc = (struct dri2_screen *) base->psc;
   struct dri2_drawable *pdraw = (struct dri2_drawable *) base;
   struct glx_display *dpyPriv = psc->base.display;
            __glxHashDelete(pdp->dri2Hash, pdraw->base.xDrawable);
            /* If it's a GLX 1.3 drawables, we can destroy the DRI2 drawable
   * now, as the application explicitly asked to destroy the GLX
   * drawable.  Otherwise, for legacy drawables, we let the DRI2
   * drawable linger on the server, since there's no good way of
   * knowing when the application is done with it.  The server will
   * destroy the DRI2 drawable when it destroys the X drawable or the
   * client exits anyway. */
   if (pdraw->base.xDrawable != pdraw->base.drawable)
               }
      static __GLXDRIdrawable *
   dri2CreateDrawable(struct glx_screen *base, XID xDrawable,
               {
      struct dri2_drawable *pdraw;
   struct dri2_screen *psc = (struct dri2_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   struct glx_display *dpyPriv;
            dpyPriv = __glXInitialize(psc->base.dpy);
   if (dpyPriv == NULL)
            pdraw = calloc(1, sizeof(*pdraw));
   if (!pdraw)
            pdraw->base.destroyDrawable = dri2DestroyDrawable;
   pdraw->base.xDrawable = xDrawable;
   pdraw->base.drawable = drawable;
   pdraw->base.psc = &psc->base;
   pdraw->bufferCount = 0;
   pdraw->swap_interval = dri_get_initial_swap_interval(psc->driScreen, psc->config);
            DRI2CreateDrawable(psc->base.dpy, xDrawable);
   pdp = (struct dri2_display *)dpyPriv->dri2Display;
   /* Create a new drawable */
   pdraw->driDrawable =
      psc->dri2->createNewDrawable(psc->driScreen,
         if (!pdraw->driDrawable) {
      DRI2DestroyDrawable(psc->base.dpy, xDrawable);
   free(pdraw);
               if (__glxHashInsert(pdp->dri2Hash, xDrawable, pdraw)) {
      psc->core->destroyDrawable(pdraw->driDrawable);
   DRI2DestroyDrawable(psc->base.dpy, xDrawable);
   free(pdraw);
               /*
   * Make sure server has the same swap interval we do for the new
   * drawable.
   */
   if (psc->vtable.setSwapInterval)
               }
      static int
   dri2DrawableGetMSC(struct glx_screen *psc, __GLXDRIdrawable *pdraw,
         {
      xcb_connection_t *c = XGetXCBConnection(pdraw->psc->dpy);
   xcb_dri2_get_msc_cookie_t get_msc_cookie;
            get_msc_cookie = xcb_dri2_get_msc_unchecked(c, pdraw->xDrawable);
            if (!get_msc_reply)
            *ust = merge_counter(get_msc_reply->ust_hi, get_msc_reply->ust_lo);
   *msc = merge_counter(get_msc_reply->msc_hi, get_msc_reply->msc_lo);
   *sbc = merge_counter(get_msc_reply->sbc_hi, get_msc_reply->sbc_lo);
               }
      static int
   dri2WaitForMSC(__GLXDRIdrawable *pdraw, int64_t target_msc, int64_t divisor,
         {
      xcb_connection_t *c = XGetXCBConnection(pdraw->psc->dpy);
   xcb_dri2_wait_msc_cookie_t wait_msc_cookie;
   xcb_dri2_wait_msc_reply_t *wait_msc_reply;
   uint32_t target_msc_hi, target_msc_lo;
   uint32_t divisor_hi, divisor_lo;
            split_counter(target_msc, &target_msc_hi, &target_msc_lo);
   split_counter(divisor, &divisor_hi, &divisor_lo);
            wait_msc_cookie = xcb_dri2_wait_msc_unchecked(c, pdraw->xDrawable,
                              if (!wait_msc_reply)
            *ust = merge_counter(wait_msc_reply->ust_hi, wait_msc_reply->ust_lo);
   *msc = merge_counter(wait_msc_reply->msc_hi, wait_msc_reply->msc_lo);
   *sbc = merge_counter(wait_msc_reply->sbc_hi, wait_msc_reply->sbc_lo);
               }
      static int
   dri2WaitForSBC(__GLXDRIdrawable *pdraw, int64_t target_sbc, int64_t *ust,
         {
      xcb_connection_t *c = XGetXCBConnection(pdraw->psc->dpy);
   xcb_dri2_wait_sbc_cookie_t wait_sbc_cookie;
   xcb_dri2_wait_sbc_reply_t *wait_sbc_reply;
                     wait_sbc_cookie = xcb_dri2_wait_sbc_unchecked(c, pdraw->xDrawable,
                  if (!wait_sbc_reply)
            *ust = merge_counter(wait_sbc_reply->ust_hi, wait_sbc_reply->ust_lo);
   *msc = merge_counter(wait_sbc_reply->msc_hi, wait_sbc_reply->msc_lo);
   *sbc = merge_counter(wait_sbc_reply->sbc_hi, wait_sbc_reply->sbc_lo);
               }
      static __DRIcontext *
   dri2GetCurrentContext()
   {
                  }
      /**
   * dri2Throttle - Request driver throttling
   *
   * This function uses the DRI2 throttle extension to give the
   * driver the opportunity to throttle on flush front, copysubbuffer
   * and swapbuffers.
   */
   static void
   dri2Throttle(struct dri2_screen *psc,
         struct dri2_drawable *draw,
   {
      if (psc->throttle) {
                     }
      /**
   * Asks the driver to flush any queued work necessary for serializing with the
   * X command stream, and optionally the slightly more strict requirement of
   * glFlush() equivalence (which would require flushing even if nothing had
   * been drawn to a window system framebuffer, for example).
   */
   static void
   dri2Flush(struct dri2_screen *psc,
            __DRIcontext *ctx,
   struct dri2_drawable *draw,
      {
      if (ctx && psc->f && psc->f->base.version >= 4) {
         } else {
      if (flags & __DRI2_FLUSH_CONTEXT)
            if (psc->f)
                  }
      static void
   __dri2CopySubBuffer(__GLXDRIdrawable *pdraw, int x, int y,
         int width, int height,
   {
      struct dri2_drawable *priv = (struct dri2_drawable *) pdraw;
   struct dri2_screen *psc = (struct dri2_screen *) pdraw->psc;
   XRectangle xrect;
   XserverRegion region;
   __DRIcontext *ctx = dri2GetCurrentContext();
            /* Check we have the right attachments */
   if (!priv->have_back)
            xrect.x = x;
   xrect.y = priv->height - y - height;
   xrect.width = width;
            flags = __DRI2_FLUSH_DRAWABLE;
   if (flush)
                  region = XFixesCreateRegion(psc->base.dpy, &xrect, 1);
   DRI2CopyRegion(psc->base.dpy, pdraw->xDrawable, region,
            /* Refresh the fake front (if present) after we just damaged the real
   * front.
   */
   if (priv->have_fake_front)
      DRI2CopyRegion(psc->base.dpy, pdraw->xDrawable, region,
            }
      static void
   dri2CopySubBuffer(__GLXDRIdrawable *pdraw, int x, int y,
         {
      __dri2CopySubBuffer(pdraw, x, y, width, height,
      }
         static void
   dri2_copy_drawable(struct dri2_drawable *priv, int dest, int src)
   {
      XRectangle xrect;
   XserverRegion region;
            xrect.x = 0;
   xrect.y = 0;
   xrect.width = priv->width;
            if (psc->f)
            region = XFixesCreateRegion(psc->base.dpy, &xrect, 1);
   DRI2CopyRegion(psc->base.dpy, priv->base.xDrawable, region, dest, src);
         }
      static void
   dri2_wait_x(struct glx_context *gc)
   {
      struct dri2_drawable *priv = (struct dri2_drawable *)
            if (priv == NULL || !priv->have_fake_front)
               }
      static void
   dri2_wait_gl(struct glx_context *gc)
   {
      struct dri2_drawable *priv = (struct dri2_drawable *)
            if (priv == NULL || !priv->have_fake_front)
               }
      /**
   * Called by the driver when it needs to update the real front buffer with the
   * contents of its fake front buffer.
   */
   static void
   dri2FlushFrontBuffer(__DRIdrawable *driDrawable, void *loaderPrivate)
   {
      struct glx_display *priv;
   struct glx_context *gc;
   struct dri2_drawable *pdraw = loaderPrivate;
            if (!pdraw)
            if (!pdraw->base.psc)
                              if (priv == NULL)
                                 }
         static void
   dri2DestroyScreen(struct glx_screen *base)
   {
               /* Free the direct rendering per screen data */
   psc->core->destroyScreen(psc->driScreen);
   driDestroyConfigs(psc->driver_configs);
   free(psc->driverName);
   close(psc->fd);
      }
      /**
   * Process list of buffer received from the server
   *
   * Processes the list of buffers received in a reply from the server to either
   * \c DRI2GetBuffers or \c DRI2GetBuffersWithFormat.
   */
   static void
   process_buffers(struct dri2_drawable * pdraw, DRI2Buffer * buffers,
         {
               pdraw->bufferCount = count;
   pdraw->have_fake_front = 0;
            /* This assumes the DRI2 buffer attachment tokens matches the
   * __DRIbuffer tokens. */
   for (i = 0; i < count; i++) {
      pdraw->buffers[i].attachment = buffers[i].attachment;
   pdraw->buffers[i].name = buffers[i].name;
   pdraw->buffers[i].pitch = buffers[i].pitch;
   pdraw->buffers[i].cpp = buffers[i].cpp;
   pdraw->buffers[i].flags = buffers[i].flags;
   if (pdraw->buffers[i].attachment == __DRI_BUFFER_FAKE_FRONT_LEFT)
         if (pdraw->buffers[i].attachment == __DRI_BUFFER_BACK_LEFT)
            }
      unsigned dri2GetSwapEventType(Display* dpy, XID drawable)
   {
         struct glx_display *glx_dpy = __glXInitialize(dpy);
   __GLXDRIdrawable *pdraw;
   pdraw = dri2GetGlxDrawableFromXDrawableId(dpy, drawable);
   if (!pdraw || !(pdraw->eventMask & GLX_BUFFER_SWAP_COMPLETE_INTEL_MASK))
         }
      static int64_t
   dri2XcbSwapBuffers(Display *dpy,
                     __GLXDRIdrawable *pdraw,
   {
      xcb_dri2_swap_buffers_cookie_t swap_buffers_cookie;
   xcb_dri2_swap_buffers_reply_t *swap_buffers_reply;
   uint32_t target_msc_hi, target_msc_lo;
   uint32_t divisor_hi, divisor_lo;
   uint32_t remainder_hi, remainder_lo;
   int64_t ret = 0;
            split_counter(target_msc, &target_msc_hi, &target_msc_lo);
   split_counter(divisor, &divisor_hi, &divisor_lo);
            swap_buffers_cookie =
      xcb_dri2_swap_buffers_unchecked(c, pdraw->xDrawable,
                     /* Immediately wait on the swapbuffers reply.  If we didn't, we'd have
   * to do so some time before reusing a (non-pageflipped) backbuffer.
   * Otherwise, the new rendering could get ahead of the X Server's
   * dispatch of the swapbuffer and you'd display garbage.
   *
   * We use XSync() first to reap the invalidate events through the event
   * filter, to ensure that the next drawing doesn't use an invalidated
   * buffer.
   */
            swap_buffers_reply =
         if (swap_buffers_reply) {
      ret = merge_counter(swap_buffers_reply->swap_hi,
            }
      }
      static int64_t
   dri2SwapBuffers(__GLXDRIdrawable *pdraw, int64_t target_msc, int64_t divisor,
   int64_t remainder, Bool flush)
   {
      struct dri2_drawable *priv = (struct dri2_drawable *) pdraw;
   struct dri2_screen *psc = (struct dri2_screen *) priv->base.psc;
            /* Check we have the right attachments */
      return ret;
         __DRIcontext *ctx = dri2GetCurrentContext();
   unsigned flags = __DRI2_FLUSH_DRAWABLE;
   if (flush)
                  ret = dri2XcbSwapBuffers(pdraw->psc->dpy, pdraw,
               }
      static __DRIbuffer *
   dri2GetBuffers(__DRIdrawable * driDrawable,
                     {
      struct dri2_drawable *pdraw = loaderPrivate;
            buffers = DRI2GetBuffers(pdraw->base.psc->dpy, pdraw->base.xDrawable,
         if (buffers == NULL)
            pdraw->width = *width;
   pdraw->height = *height;
                        }
      static __DRIbuffer *
   dri2GetBuffersWithFormat(__DRIdrawable * driDrawable,
                     {
      struct dri2_drawable *pdraw = loaderPrivate;
            buffers = DRI2GetBuffersWithFormat(pdraw->base.psc->dpy,
                     if (buffers == NULL)
            pdraw->width = *width;
   pdraw->height = *height;
                        }
      static int
   dri2SetSwapInterval(__GLXDRIdrawable *pdraw, int interval)
   {
      xcb_connection_t *c = XGetXCBConnection(pdraw->psc->dpy);
   struct dri2_drawable *priv =  (struct dri2_drawable *) pdraw;
            if (!dri_valid_swap_interval(psc->driScreen, psc->config, interval))
            xcb_dri2_swap_interval(c, priv->base.xDrawable, interval);
               }
      static int
   dri2GetSwapInterval(__GLXDRIdrawable *pdraw)
   {
            return priv->swap_interval;
   }
      static void
   driSetBackgroundContext(void *loaderPrivate)
   {
         }
      static GLboolean
   driIsThreadSafe(void *loaderPrivate)
   {
      struct glx_context *pcp = (struct glx_context *) loaderPrivate;
   /* Check Xlib is running in thread safe mode
   *
   * 'lock_fns' is the XLockDisplay function pointer of the X11 display 'dpy'.
   * It will be NULL if XInitThreads wasn't called.
   */
      }
      static const __DRIdri2LoaderExtension dri2LoaderExtension = {
               .getBuffers              = dri2GetBuffers,
   .flushFrontBuffer        = dri2FlushFrontBuffer,
      };
      const __DRIuseInvalidateExtension dri2UseInvalidate = {
         };
      const __DRIbackgroundCallableExtension driBackgroundCallable = {
               .setBackgroundContext    = driSetBackgroundContext,
      };
      _X_HIDDEN void
   dri2InvalidateBuffers(Display *dpy, XID drawable)
   {
      __GLXDRIdrawable *pdraw =
         struct dri2_screen *psc;
            if (!pdraw)
                     if (psc->f && psc->f->base.version >= 3 && psc->f->invalidate)
      }
      static void
   dri2_bind_tex_image(__GLXDRIdrawable *base,
         {
      struct glx_context *gc = __glXGetCurrentContext();
   struct dri2_drawable *pdraw = (struct dri2_drawable *) base;
            if (pdraw != NULL) {
                     psc->texBuffer->setTexBuffer2(gc->driContext,
         pdraw->base.textureTarget,
   pdraw->base.textureFormat,
   pdraw->driDrawable);
   }
   psc->texBuffer->setTexBuffer(gc->driContext,
         pdraw->base.textureTarget,
   pdraw->driDrawable);
         }
      static void
   dri2_release_tex_image(__GLXDRIdrawable *base, int buffer)
   {
      struct glx_context *gc = __glXGetCurrentContext();
   struct dri2_drawable *pdraw = (struct dri2_drawable *) base;
            if (pdraw != NULL) {
               if (psc->texBuffer->base.version >= 3 &&
      psc->texBuffer->releaseTexBuffer != NULL) {
   psc->texBuffer->releaseTexBuffer(gc->driContext,
                  }
      static const struct glx_context_vtable dri2_context_vtable = {
      .destroy             = dri2_destroy_context,
   .bind                = dri2_bind_context,
   .unbind              = dri2_unbind_context,
   .wait_gl             = dri2_wait_gl,
   .wait_x              = dri2_wait_x,
   .interop_query_device_info = dri2_interop_query_device_info,
   .interop_export_object = dri2_interop_export_object,
      };
      static void
   dri2BindExtensions(struct dri2_screen *psc, struct glx_display * priv,
         {
      const unsigned mask = psc->dri2->getAPIMask(psc->driScreen);
   const __DRIextension **extensions;
                     __glXEnableDirectExtension(&psc->base, "GLX_EXT_swap_control");
   __glXEnableDirectExtension(&psc->base, "GLX_SGI_swap_control");
   __glXEnableDirectExtension(&psc->base, "GLX_MESA_swap_control");
            /*
   * GLX_INTEL_swap_event is broken on the server side, where it's
   * currently unconditionally enabled. This completely breaks
   * systems running on drivers which don't support that extension.
   * There's no way to test for its presence on this side, so instead
   * of disabling it unconditionally, just disable it for drivers
   * which are known to not support it.
   *
   * This was fixed in xserver 1.15.0 (190b03215), so now we only
   * disable the broken driver.
   */
   if (strcmp(driverName, "vmwgfx") != 0) {
                  __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context");
   __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context_profile");
   __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context_no_error");
            if ((mask & ((1 << __DRI_API_GLES) |
                  __glXEnableDirectExtension(&psc->base,
         __glXEnableDirectExtension(&psc->base,
               static const struct dri_extension_match exts[] = {
      { __DRI_TEX_BUFFER, 1, offsetof(struct dri2_screen, texBuffer), true },
   { __DRI2_FLUSH, 1, offsetof(struct dri2_screen, f), true },
   { __DRI2_CONFIG_QUERY, 1, offsetof(struct dri2_screen, config), true },
   { __DRI2_THROTTLE, 1, offsetof(struct dri2_screen, throttle), true },
   { __DRI2_RENDERER_QUERY, 1, offsetof(struct dri2_screen, rendererQuery), true },
      };
            /* Extensions where we don't care about the extension struct */
   for (i = 0; extensions[i]; i++) {
      if (strcmp(extensions[i]->name, __DRI2_ROBUSTNESS) == 0)
                  if (strcmp(extensions[i]->name, __DRI2_FLUSH_CONTROL) == 0)
      __glXEnableDirectExtension(&psc->base,
            if (psc->texBuffer)
            if (psc->rendererQuery)
            if (psc->interop)
      }
      static char *
   dri2_get_driver_name(struct glx_screen *glx_screen)
   {
                  }
      static const struct glx_screen_vtable dri2_screen_vtable = {
      .create_context         = dri_common_create_context,
   .create_context_attribs = dri2_create_context_attribs,
   .query_renderer_integer = dri2_query_renderer_integer,
   .query_renderer_string  = dri2_query_renderer_string,
      };
      static struct glx_screen *
   dri2CreateScreen(int screen, struct glx_display * priv)
   {
      const __DRIconfig **driver_configs;
   const __DRIextension **extensions;
   const struct dri2_display *const pdp = (struct dri2_display *)
         struct dri2_screen *psc;
   __GLXDRIscreen *psp;
   struct glx_config *configs = NULL, *visuals = NULL;
   char *driverName = NULL, *loader_driverName, *deviceName, *tmp;
            psc = calloc(1, sizeof *psc);
   if (psc == NULL)
                     if (!glx_screen_init(&psc->base, screen, priv)) {
      free(psc);
               if (!DRI2Connect(priv->dpy, RootWindow(priv->dpy, screen),
      &driverName, &deviceName)) {
   glx_screen_cleanup(&psc->base);
   free(psc);
   InfoMessageF("screen %d does not appear to be DRI2 capable\n", screen);
               psc->fd = loader_open_device(deviceName);
   if (psc->fd < 0) {
      ErrorMessageF("failed to open %s: %s\n", deviceName, strerror(errno));
               if (drmGetMagic(psc->fd, &magic)) {
      ErrorMessageF("failed to get magic\n");
               if (!DRI2Authenticate(priv->dpy, RootWindow(priv->dpy, screen), magic)) {
      ErrorMessageF("failed to authenticate magic %d\n", magic);
               /* If Mesa knows about the appropriate driver for this fd, then trust it.
   * Otherwise, default to the server's value.
   */
   loader_driverName = loader_get_driver_for_fd(psc->fd);
   if (loader_driverName) {
      free(driverName);
      }
            extensions = driOpenDriver(driverName, &psc->driver);
   if (extensions == NULL)
            static const struct dri_extension_match exts[] = {
      { __DRI_CORE, 1, offsetof(struct dri2_screen, core), false },
   { __DRI_DRI2, 4, offsetof(struct dri2_screen, dri2), false },
      };
   if (!loader_bind_extensions(psc, exts, ARRAY_SIZE(exts), extensions))
            psc->driScreen =
      psc->dri2->createNewScreen2(screen, psc->fd,
                     if (psc->driScreen == NULL) {
      ErrorMessageF("glx: failed to create dri2 screen\n");
                        configs = driConvertConfigs(psc->core, psc->base.configs, driver_configs);
            if (!configs || !visuals) {
      ErrorMessageF("No matching fbConfigs or visuals found\n");
               glx_config_destroy_list(psc->base.configs);
   psc->base.configs = configs;
   glx_config_destroy_list(psc->base.visuals);
                     psc->base.vtable = &dri2_screen_vtable;
   psc->base.context_vtable = &dri2_context_vtable;
   psp = &psc->vtable;
   psc->base.driScreen = psp;
   psp->destroyScreen = dri2DestroyScreen;
   psp->createDrawable = dri2CreateDrawable;
   psp->swapBuffers = dri2SwapBuffers;
   psp->getDrawableMSC = NULL;
   psp->waitForMSC = NULL;
   psp->waitForSBC = NULL;
   psp->setSwapInterval = NULL;
   psp->getSwapInterval = NULL;
   psp->getBufferAge = NULL;
   psp->bindTexImage = dri2_bind_tex_image;
            psp->getDrawableMSC = dri2DrawableGetMSC;
   psp->waitForMSC = dri2WaitForMSC;
   psp->waitForSBC = dri2WaitForSBC;
   psp->setSwapInterval = dri2SetSwapInterval;
   psp->getSwapInterval = dri2GetSwapInterval;
            __glXEnableDirectExtension(&psc->base, "GLX_OML_sync_control");
            if (psc->config->base.version > 1 &&
         psc->config->configQuerys(psc->driScreen, "glx_extension_override",
            if (psc->config->base.version > 1 &&
         psc->config->configQuerys(psc->driScreen,
                  if (psc->config->base.version > 1) {
      uint8_t force = false;
   if (psc->config->configQueryb(psc->driScreen, "force_direct_glx_context",
                        uint8_t invalid_glx_destroy_window = false;
   if (psc->config->configQueryb(psc->driScreen,
                                 /* DRI2 supports SubBuffer through DRI2CopyRegion, so it's always
   * available.*/
   psp->copySubBuffer = dri2CopySubBuffer;
                     tmp = getenv("LIBGL_SHOW_FPS");
   psc->show_fps_interval = (tmp) ? atoi(tmp) : 0;
   if (psc->show_fps_interval < 0)
                           handle_error:
               if (configs)
         if (visuals)
         if (psc->driScreen)
         psc->driScreen = NULL;
   if (psc->fd >= 0)
         if (psc->driver)
            free(deviceName);
   glx_screen_cleanup(&psc->base);
               }
      /* Called from __glXFreeDisplayPrivate.
   */
   static void
   dri2DestroyDisplay(__GLXDRIdisplay * dpy)
   {
               __glxHashDestroy(pdp->dri2Hash);
      }
      _X_HIDDEN __GLXDRIdrawable *
   dri2GetGlxDrawableFromXDrawableId(Display *dpy, XID id)
   {
      struct glx_display *d = __glXInitialize(dpy);
   struct dri2_display *pdp = (struct dri2_display *) d->dri2Display;
            if (__glxHashLookup(pdp->dri2Hash, id, (void *) &pdraw) == 0)
               }
      /*
   * Allocate, initialize and return a __DRIdisplayPrivate object.
   * This is called from __glXInitialize() when we are given a new
   * display pointer.
   */
   _X_HIDDEN __GLXDRIdisplay *
   dri2CreateDisplay(Display * dpy)
   {
      struct dri2_display *pdp;
   int eventBase, errorBase, i;
            if (!DRI2QueryExtension(dpy, &eventBase, &errorBase))
            pdp = malloc(sizeof *pdp);
   if (pdp == NULL)
            if (!DRI2QueryVersion(dpy, &driMajor, &driMinor) ||
      driMinor < 3) {
   free(pdp);
               pdp->base.destroyDisplay = dri2DestroyDisplay;
            i = 0;
   pdp->loader_extensions[i++] = &dri2LoaderExtension.base;
   pdp->loader_extensions[i++] = &dri2UseInvalidate.base;
   pdp->loader_extensions[i++] = &driBackgroundCallable.base;
            pdp->dri2Hash = __glxHashCreate();
   if (pdp->dri2Hash == NULL) {
      free(pdp);
                  }
      #endif /* GLX_DIRECT_RENDERING */
