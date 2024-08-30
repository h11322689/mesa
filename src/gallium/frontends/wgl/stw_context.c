   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "state_tracker/st_context.h"
      #include <windows.h>
      #define WGL_WGLEXT_PROTOTYPES
      #include <GL/gl.h>
   #include <GL/wglext.h>
      #include "util/compiler.h"
   #include "pipe/p_context.h"
   #include "pipe/p_state.h"
   #include "util/compiler.h"
   #include "util/u_memory.h"
   #include "util/u_atomic.h"
   #include "hud/hud_context.h"
      #include "stw_gdishim.h"
   #include "gldrv.h"
   #include "stw_device.h"
   #include "stw_winsys.h"
   #include "stw_framebuffer.h"
   #include "stw_pixelformat.h"
   #include "stw_context.h"
   #include "stw_tls.h"
      #include "main/context.h"
      struct stw_context *
   stw_current_context(void)
   {
                           }
         BOOL APIENTRY
   DrvCopyContext(DHGLRC dhrcSource, DHGLRC dhrcDest, UINT fuMask)
   {
      struct stw_context *src;
   struct stw_context *dst;
            if (!stw_dev)
                     src = stw_lookup_context_locked( dhrcSource );
            if (src && dst) {
      /* FIXME */
   assert(0);
   (void) src;
   (void) dst;
                           }
         BOOL APIENTRY
   DrvShareLists(DHGLRC dhglrc1, DHGLRC dhglrc2)
   {
      struct stw_context *ctx1;
   struct stw_context *ctx2;
            if (!stw_dev)
                     ctx1 = stw_lookup_context_locked( dhglrc1 );
            if (ctx1 && ctx2) {
      ret = _mesa_share_state(ctx2->st->ctx, ctx1->st->ctx);
   ctx1->shared = true;
                           }
         DHGLRC APIENTRY
   DrvCreateContext(HDC hdc)
   {
         }
         DHGLRC APIENTRY
   DrvCreateLayerContext(HDC hdc, INT iLayerPlane)
   {
      if (!stw_dev)
            const struct stw_pixelformat_info *pfi = stw_pixelformat_get_info_from_hdc(hdc);
   if (!pfi)
            struct stw_context *ctx = stw_create_context_attribs(hdc, iLayerPlane, NULL, stw_dev->fscreen, 1, 0, 0,
               if (!ctx)
            DHGLRC ret = stw_create_context_handle(ctx, 0);
   if (!ret)
               }
      /**
   * Called via DrvCreateContext(), DrvCreateLayerContext() and
   * wglCreateContextAttribsARB() to actually create a rendering context.
   */
   struct stw_context *
   stw_create_context_attribs(HDC hdc, INT iLayerPlane, struct stw_context *shareCtx,
                                 {
      struct st_context_attribs attribs;
   struct stw_context *ctx = NULL;
            if (!stw_dev)
            if (iLayerPlane != 0)
            if (shareCtx != NULL)
            ctx = CALLOC_STRUCT( stw_context );
   if (ctx == NULL)
            ctx->hDrawDC = hdc;
   ctx->hReadDC = hdc;
   ctx->pfi = pfi;
            memset(&attribs, 0, sizeof(attribs));
   if (pfi)
         attribs.major = majorVersion;
   attribs.minor = minorVersion;
   if (contextFlags & WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB)
         if (contextFlags & WGL_CONTEXT_DEBUG_BIT_ARB)
         if (contextFlags & WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB)
         if (resetStrategy != WGL_NO_RESET_NOTIFICATION_ARB)
            switch (profileMask) {
   case WGL_CONTEXT_CORE_PROFILE_BIT_ARB:
      /* There are no profiles before OpenGL 3.2.  The
   * WGL_ARB_create_context_profile spec says:
   *
   *     "If the requested OpenGL version is less than 3.2,
   *     WGL_CONTEXT_PROFILE_MASK_ARB is ignored and the functionality
   *     of the context is determined solely by the requested version."
   */
   if (majorVersion > 3 || (majorVersion == 3 && minorVersion >= 2)) {
      attribs.profile = API_OPENGL_CORE;
      }
      case WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB:
      /*
   * The spec also says:
   *
   *     "If version 3.1 is requested, the context returned may implement
   *     any of the following versions:
   *
   *       * Version 3.1. The GL_ARB_compatibility extension may or may not
   *         be implemented, as determined by the implementation.
   *       * The core profile of version 3.2 or greater."
   *
   * But Mesa doesn't support GL_ARB_compatibility, while most prevalent
   * Windows OpenGL implementations do, and unfortunately many Windows
   * applications don't check whether they receive or not a context with
   * GL_ARB_compatibility, so returning a core profile here does more harm
   * than good.
   */
   attribs.profile = API_OPENGL_COMPAT;
      case WGL_CONTEXT_ES_PROFILE_BIT_EXT:
      if (majorVersion >= 2) {
         } else {
         }
      default:
      assert(0);
                        ctx->st = st_api_create_context(
         if (ctx->st == NULL)
                     if (ctx->st->cso_context) {
      ctx->hud = hud_create(ctx->st->cso_context, NULL, ctx->st,
                     no_st_ctx:
         no_ctx:
         }
      DHGLRC
   stw_create_context_handle(struct stw_context *ctx, DHGLRC handle)
   {
               stw_lock_contexts(stw_dev);
   if (handle) {
      /* We're replacing the context data for this handle. See the
   * wglCreateContextAttribsARB() function.
   */
   struct stw_context *old_ctx =
         if (old_ctx) {
                  /* replace table entry */
      }
   else {
      /* create new table entry */
                                    }
      void
   stw_destroy_context(struct stw_context *ctx)
   {
      if (ctx->hud) {
                  st_destroy_context(ctx->st);
      }
         BOOL APIENTRY
   DrvDeleteContext(DHGLRC dhglrc)
   {
      struct stw_context *ctx ;
            if (!stw_dev)
            stw_lock_contexts(stw_dev);
   ctx = stw_lookup_context_locked(dhglrc);
   handle_table_remove(stw_dev->ctx_table, dhglrc);
            if (ctx) {
               /* Unbind current if deleting current context. */
   if (curctx == ctx)
            stw_destroy_context(ctx);
                  }
      BOOL
   stw_unbind_context(struct stw_context *ctx)
   {
      if (!ctx)
            /* The expectation is that ctx is the same context which is
   * current for this thread.  We should check that and return False
   * if not the case.
   */
   if (ctx != stw_current_context())
            if (stw_make_current( NULL, NULL, NULL ) == false)
               }
      BOOL APIENTRY
   DrvReleaseContext(DHGLRC dhglrc)
   {
               if (!stw_dev)
            stw_lock_contexts(stw_dev);
   ctx = stw_lookup_context_locked( dhglrc );
               }
         DHGLRC
   stw_get_current_context( void )
   {
               ctx = stw_current_context();
   if (!ctx)
               }
         HDC
   stw_get_current_dc( void )
   {
               ctx = stw_current_context();
   if (!ctx)
               }
      HDC
   stw_get_current_read_dc( void )
   {
               ctx = stw_current_context();
   if (!ctx)
               }
      static void
   release_old_framebuffers(struct stw_framebuffer *old_fb, struct stw_framebuffer *old_fbRead,
         {
      if (old_fb || old_fbRead) {
      stw_lock_framebuffers(stw_dev);
   if (old_fb) {
      stw_framebuffer_lock(old_fb);
      }
   if (old_fbRead && old_fb != old_fbRead) {
      stw_framebuffer_lock(old_fbRead);
      }
         }
      BOOL
   stw_make_current(struct stw_framebuffer *fb, struct stw_framebuffer *fbRead, struct stw_context *ctx)
   {
      struct stw_context *old_ctx = NULL;
            if (!stw_dev)
            old_ctx = stw_current_context();
   if (old_ctx != NULL) {
      if (old_ctx == ctx) {
      if (old_ctx->current_framebuffer == fb && old_ctx->current_read_framebuffer == fbRead) {
      /* Return if already current. */
         } else {
      if (old_ctx->shared) {
      if (old_ctx->current_framebuffer) {
      stw_st_flush(old_ctx->st, old_ctx->current_framebuffer->drawable,
      } else {
      struct pipe_fence_handle *fence = NULL;
   st_context_flush(old_ctx->st,
               } else {
      if (old_ctx->current_framebuffer)
      stw_st_flush(old_ctx->st, old_ctx->current_framebuffer->drawable,
      else
                     if (ctx) {
      if (ctx->pfi && fb && fb->pfi != ctx->pfi) {
      SetLastError(ERROR_INVALID_PIXEL_FORMAT);
      }
   if (ctx->pfi && fbRead && fbRead->pfi != ctx->pfi) {
      SetLastError(ERROR_INVALID_PIXEL_FORMAT);
               if (fb) {
      stw_framebuffer_lock(fb);
   stw_framebuffer_update(fb);
   stw_framebuffer_reference_locked(fb);
               if (fbRead && fbRead != fb) {
      stw_framebuffer_lock(fbRead);
   stw_framebuffer_update(fbRead);
   stw_framebuffer_reference_locked(fbRead);
               struct stw_framebuffer *old_fb = ctx->current_framebuffer;
   struct stw_framebuffer *old_fbRead = ctx->current_read_framebuffer;
   ctx->current_framebuffer = fb;
            ret = st_api_make_current(ctx->st,
                  /* Release the old framebuffers from this context. */
      fail:
         /* fb and fbRead must be unlocked at this point. */
   if (fb)
         if (fbRead)
            /* On failure, make the thread's current rendering context not current
   * before returning.
   */
   if (!ret) {
            } else {
                  /* Unreference the previous framebuffer if any. It must be done after
   * make_current, as it can be referenced inside.
   */
   if (old_ctx && old_ctx != ctx) {
      release_old_framebuffers(old_ctx->current_framebuffer, old_ctx->current_read_framebuffer, old_ctx);
   old_ctx->current_framebuffer = NULL;
                  }
      static struct stw_framebuffer *
   get_unlocked_refd_framebuffer_from_dc(HDC hDC)
   {
      if (!hDC)
            /* This call locks fb's mutex */
   struct stw_framebuffer *fb = stw_framebuffer_from_hdc(hDC);
   if (!fb) {
      /* Applications should call SetPixelFormat before creating a context,
   * but not all do, and the opengl32 runtime seems to use a default
   * pixel format in some cases, so we must create a framebuffer for
   * those here.
   */
   int iPixelFormat = stw_pixelformat_guess(hDC);
   if (iPixelFormat)
         if (!fb)
      }
   stw_framebuffer_reference_locked(fb);
   stw_framebuffer_unlock(fb);
      }
      BOOL
   stw_make_current_by_handles(HDC hDrawDC, HDC hReadDC, DHGLRC dhglrc)
   {
      struct stw_context *ctx = stw_lookup_context(dhglrc);
   if (dhglrc && !ctx) {
      stw_make_current_by_handles(NULL, NULL, 0);
               struct stw_framebuffer *fb = get_unlocked_refd_framebuffer_from_dc(hDrawDC);
   if (ctx && !fb) {
      stw_make_current_by_handles(NULL, NULL, 0);
               struct stw_framebuffer *fbRead = (hDrawDC == hReadDC || hReadDC == NULL) ?
         if (ctx && !fbRead) {
      release_old_framebuffers(fb, NULL, ctx);
   stw_make_current_by_handles(NULL, NULL, 0);
                        if (ctx) {
      if (success) {
      ctx->hDrawDC = hDrawDC;
      } else {
      ctx->hDrawDC = NULL;
                  assert(!ctx || (fb && fbRead));
   if (fb || fbRead) {
      /* In the success case, the context took extra references on these framebuffers,
   * so release our local references.
   */
   stw_lock_framebuffers(stw_dev);
   if (fb) {
      stw_framebuffer_lock(fb);
      }
   if (fbRead && fbRead != fb) {
      stw_framebuffer_lock(fbRead);
      }
                  }
         /**
   * Notify the current context that the framebuffer has become invalid.
   */
   void
   stw_notify_current_locked( struct stw_framebuffer *fb )
   {
         }
         /**
   * Although WGL allows different dispatch entrypoints per context
   */
   static const GLCLTPROCTABLE cpt =
   {
      OPENGL_VERSION_110_ENTRIES,
   {
      &glNewList,
   &glEndList,
   &glCallList,
   &glCallLists,
   &glDeleteLists,
   &glGenLists,
   &glListBase,
   &glBegin,
   &glBitmap,
   &glColor3b,
   &glColor3bv,
   &glColor3d,
   &glColor3dv,
   &glColor3f,
   &glColor3fv,
   &glColor3i,
   &glColor3iv,
   &glColor3s,
   &glColor3sv,
   &glColor3ub,
   &glColor3ubv,
   &glColor3ui,
   &glColor3uiv,
   &glColor3us,
   &glColor3usv,
   &glColor4b,
   &glColor4bv,
   &glColor4d,
   &glColor4dv,
   &glColor4f,
   &glColor4fv,
   &glColor4i,
   &glColor4iv,
   &glColor4s,
   &glColor4sv,
   &glColor4ub,
   &glColor4ubv,
   &glColor4ui,
   &glColor4uiv,
   &glColor4us,
   &glColor4usv,
   &glEdgeFlag,
   &glEdgeFlagv,
   &glEnd,
   &glIndexd,
   &glIndexdv,
   &glIndexf,
   &glIndexfv,
   &glIndexi,
   &glIndexiv,
   &glIndexs,
   &glIndexsv,
   &glNormal3b,
   &glNormal3bv,
   &glNormal3d,
   &glNormal3dv,
   &glNormal3f,
   &glNormal3fv,
   &glNormal3i,
   &glNormal3iv,
   &glNormal3s,
   &glNormal3sv,
   &glRasterPos2d,
   &glRasterPos2dv,
   &glRasterPos2f,
   &glRasterPos2fv,
   &glRasterPos2i,
   &glRasterPos2iv,
   &glRasterPos2s,
   &glRasterPos2sv,
   &glRasterPos3d,
   &glRasterPos3dv,
   &glRasterPos3f,
   &glRasterPos3fv,
   &glRasterPos3i,
   &glRasterPos3iv,
   &glRasterPos3s,
   &glRasterPos3sv,
   &glRasterPos4d,
   &glRasterPos4dv,
   &glRasterPos4f,
   &glRasterPos4fv,
   &glRasterPos4i,
   &glRasterPos4iv,
   &glRasterPos4s,
   &glRasterPos4sv,
   &glRectd,
   &glRectdv,
   &glRectf,
   &glRectfv,
   &glRecti,
   &glRectiv,
   &glRects,
   &glRectsv,
   &glTexCoord1d,
   &glTexCoord1dv,
   &glTexCoord1f,
   &glTexCoord1fv,
   &glTexCoord1i,
   &glTexCoord1iv,
   &glTexCoord1s,
   &glTexCoord1sv,
   &glTexCoord2d,
   &glTexCoord2dv,
   &glTexCoord2f,
   &glTexCoord2fv,
   &glTexCoord2i,
   &glTexCoord2iv,
   &glTexCoord2s,
   &glTexCoord2sv,
   &glTexCoord3d,
   &glTexCoord3dv,
   &glTexCoord3f,
   &glTexCoord3fv,
   &glTexCoord3i,
   &glTexCoord3iv,
   &glTexCoord3s,
   &glTexCoord3sv,
   &glTexCoord4d,
   &glTexCoord4dv,
   &glTexCoord4f,
   &glTexCoord4fv,
   &glTexCoord4i,
   &glTexCoord4iv,
   &glTexCoord4s,
   &glTexCoord4sv,
   &glVertex2d,
   &glVertex2dv,
   &glVertex2f,
   &glVertex2fv,
   &glVertex2i,
   &glVertex2iv,
   &glVertex2s,
   &glVertex2sv,
   &glVertex3d,
   &glVertex3dv,
   &glVertex3f,
   &glVertex3fv,
   &glVertex3i,
   &glVertex3iv,
   &glVertex3s,
   &glVertex3sv,
   &glVertex4d,
   &glVertex4dv,
   &glVertex4f,
   &glVertex4fv,
   &glVertex4i,
   &glVertex4iv,
   &glVertex4s,
   &glVertex4sv,
   &glClipPlane,
   &glColorMaterial,
   &glCullFace,
   &glFogf,
   &glFogfv,
   &glFogi,
   &glFogiv,
   &glFrontFace,
   &glHint,
   &glLightf,
   &glLightfv,
   &glLighti,
   &glLightiv,
   &glLightModelf,
   &glLightModelfv,
   &glLightModeli,
   &glLightModeliv,
   &glLineStipple,
   &glLineWidth,
   &glMaterialf,
   &glMaterialfv,
   &glMateriali,
   &glMaterialiv,
   &glPointSize,
   &glPolygonMode,
   &glPolygonStipple,
   &glScissor,
   &glShadeModel,
   &glTexParameterf,
   &glTexParameterfv,
   &glTexParameteri,
   &glTexParameteriv,
   &glTexImage1D,
   &glTexImage2D,
   &glTexEnvf,
   &glTexEnvfv,
   &glTexEnvi,
   &glTexEnviv,
   &glTexGend,
   &glTexGendv,
   &glTexGenf,
   &glTexGenfv,
   &glTexGeni,
   &glTexGeniv,
   &glFeedbackBuffer,
   &glSelectBuffer,
   &glRenderMode,
   &glInitNames,
   &glLoadName,
   &glPassThrough,
   &glPopName,
   &glPushName,
   &glDrawBuffer,
   &glClear,
   &glClearAccum,
   &glClearIndex,
   &glClearColor,
   &glClearStencil,
   &glClearDepth,
   &glStencilMask,
   &glColorMask,
   &glDepthMask,
   &glIndexMask,
   &glAccum,
   &glDisable,
   &glEnable,
   &glFinish,
   &glFlush,
   &glPopAttrib,
   &glPushAttrib,
   &glMap1d,
   &glMap1f,
   &glMap2d,
   &glMap2f,
   &glMapGrid1d,
   &glMapGrid1f,
   &glMapGrid2d,
   &glMapGrid2f,
   &glEvalCoord1d,
   &glEvalCoord1dv,
   &glEvalCoord1f,
   &glEvalCoord1fv,
   &glEvalCoord2d,
   &glEvalCoord2dv,
   &glEvalCoord2f,
   &glEvalCoord2fv,
   &glEvalMesh1,
   &glEvalPoint1,
   &glEvalMesh2,
   &glEvalPoint2,
   &glAlphaFunc,
   &glBlendFunc,
   &glLogicOp,
   &glStencilFunc,
   &glStencilOp,
   &glDepthFunc,
   &glPixelZoom,
   &glPixelTransferf,
   &glPixelTransferi,
   &glPixelStoref,
   &glPixelStorei,
   &glPixelMapfv,
   &glPixelMapuiv,
   &glPixelMapusv,
   &glReadBuffer,
   &glCopyPixels,
   &glReadPixels,
   &glDrawPixels,
   &glGetBooleanv,
   &glGetClipPlane,
   &glGetDoublev,
   &glGetError,
   &glGetFloatv,
   &glGetIntegerv,
   &glGetLightfv,
   &glGetLightiv,
   &glGetMapdv,
   &glGetMapfv,
   &glGetMapiv,
   &glGetMaterialfv,
   &glGetMaterialiv,
   &glGetPixelMapfv,
   &glGetPixelMapuiv,
   &glGetPixelMapusv,
   &glGetPolygonStipple,
   &glGetString,
   &glGetTexEnvfv,
   &glGetTexEnviv,
   &glGetTexGendv,
   &glGetTexGenfv,
   &glGetTexGeniv,
   &glGetTexImage,
   &glGetTexParameterfv,
   &glGetTexParameteriv,
   &glGetTexLevelParameterfv,
   &glGetTexLevelParameteriv,
   &glIsEnabled,
   &glIsList,
   &glDepthRange,
   &glFrustum,
   &glLoadIdentity,
   &glLoadMatrixf,
   &glLoadMatrixd,
   &glMatrixMode,
   &glMultMatrixf,
   &glMultMatrixd,
   &glOrtho,
   &glPopMatrix,
   &glPushMatrix,
   &glRotated,
   &glRotatef,
   &glScaled,
   &glScalef,
   &glTranslated,
   &glTranslatef,
   &glViewport,
   &glArrayElement,
   &glBindTexture,
   &glColorPointer,
   &glDisableClientState,
   &glDrawArrays,
   &glDrawElements,
   &glEdgeFlagPointer,
   &glEnableClientState,
   &glIndexPointer,
   &glIndexub,
   &glIndexubv,
   &glInterleavedArrays,
   &glNormalPointer,
   &glPolygonOffset,
   &glTexCoordPointer,
   &glVertexPointer,
   &glAreTexturesResident,
   &glCopyTexImage1D,
   &glCopyTexImage2D,
   &glCopyTexSubImage1D,
   &glCopyTexSubImage2D,
   &glDeleteTextures,
   &glGenTextures,
   &glGetPointerv,
   &glIsTexture,
   &glPrioritizeTextures,
   &glTexSubImage1D,
   &glTexSubImage2D,
   &glPopClientAttrib,
         };
         PGLCLTPROCTABLE APIENTRY
   DrvSetContext(HDC hdc, DHGLRC dhglrc, PFN_SETPROCTABLE pfnSetProcTable)
   {
               if (!stw_make_current_by_handles(hdc, hdc, dhglrc))
               }
