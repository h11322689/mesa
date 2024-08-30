   /**************************************************************************
   *
   * Copyright 2009, VMware, Inc.
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
   /*
   * Author: Keith Whitwell <keithw@vmware.com>
   * Author: Jakob Bornecrantz <wallbraker@gmail.com>
   */
      #include "dri_screen.h"
   #include "dri_drawable.h"
   #include "dri_context.h"
   #include "frontend/drm_driver.h"
      #include "pipe/p_context.h"
   #include "pipe-loader/pipe_loader.h"
   #include "state_tracker/st_context.h"
      #include "util/u_cpu_detect.h"
   #include "util/u_memory.h"
   #include "util/u_debug.h"
      struct dri_context *
   dri_create_context(struct dri_screen *screen,
                     gl_api api, const struct gl_config *visual,
   const struct __DriverContextConfig *ctx_config,
   {
      struct dri_context *ctx = NULL;
   struct st_context *st_share = NULL;
   struct st_context_attribs attribs;
   enum st_context_error ctx_err = 0;
   unsigned allowed_flags = __DRI_CTX_FLAG_DEBUG |
         unsigned allowed_attribs =
      __DRIVER_CONTEXT_ATTRIB_PRIORITY |
   __DRIVER_CONTEXT_ATTRIB_RELEASE_BEHAVIOR |
      const __DRIbackgroundCallableExtension *backgroundCallable =
                  /* This is effectively doing error checking for GLX context creation (by both
   * Mesa and the X server) when the driver doesn't support the robustness ext.
   * EGL already checks, so it won't send us the flags if the ext isn't
   * available.
   */
   if (screen->has_reset_status_query) {
      allowed_flags |= __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS;
               if (screen->has_protected_context)
            if (ctx_config->flags & ~allowed_flags) {
      *error = __DRI_CTX_ERROR_UNKNOWN_FLAG;
               if (ctx_config->attribute_mask & ~allowed_attribs) {
      *error = __DRI_CTX_ERROR_UNKNOWN_ATTRIBUTE;
               memset(&attribs, 0, sizeof(attribs));
   switch (api) {
   case API_OPENGLES:
      attribs.profile = API_OPENGLES;
      case API_OPENGLES2:
      attribs.profile = API_OPENGLES2;
      case API_OPENGL_COMPAT:
   case API_OPENGL_CORE:
      if (driQueryOptionb(optionCache, "force_compat_profile")) {
         } else {
      attribs.profile = api == API_OPENGL_COMPAT ? API_OPENGL_COMPAT
               attribs.major = ctx_config->major_version;
            attribs.flags |= ST_CONTEXT_FLAG_FORWARD_COMPATIBLE;
            default:
      *error = __DRI_CTX_ERROR_BAD_API;
               if ((ctx_config->flags & __DRI_CTX_FLAG_DEBUG) != 0)
            if (ctx_config->flags & __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS)
            if (ctx_config->attribute_mask & __DRIVER_CONTEXT_ATTRIB_RESET_STRATEGY)
      if (ctx_config->reset_strategy != __DRI_CTX_RESET_NO_NOTIFICATION)
         if (ctx_config->attribute_mask & __DRIVER_CONTEXT_ATTRIB_NO_ERROR)
            if (ctx_config->attribute_mask & __DRIVER_CONTEXT_ATTRIB_PRIORITY) {
      switch (ctx_config->priority) {
   case __DRI_CTX_PRIORITY_LOW:
      attribs.context_flags |= PIPE_CONTEXT_LOW_PRIORITY;
      case __DRI_CTX_PRIORITY_HIGH:
      attribs.context_flags |= PIPE_CONTEXT_HIGH_PRIORITY;
      default:
                     if ((ctx_config->attribute_mask & __DRIVER_CONTEXT_ATTRIB_RELEASE_BEHAVIOR)
      && (ctx_config->release_behavior == __DRI_CTX_RELEASE_BEHAVIOR_NONE))
         if (ctx_config->attribute_mask & __DRIVER_CONTEXT_ATTRIB_PROTECTED)
            struct dri_context *share_ctx = NULL;
   if (sharedContextPrivate) {
      share_ctx = (struct dri_context *)sharedContextPrivate;
               ctx = CALLOC_STRUCT(dri_context);
   if (ctx == NULL) {
      *error = __DRI_CTX_ERROR_NO_MEMORY;
               ctx->screen = screen;
            /* KHR_no_error is likely to crash, overflow memory, etc if an application
   * has errors so don't enable it for setuid processes.
   */
   if (debug_get_bool_option("MESA_NO_ERROR", false) ||
      #if !defined(_WIN32)
         #endif
               attribs.options = screen->options;
   dri_fill_st_visual(&attribs.visual, screen, visual);
   ctx->st = st_api_create_context(&screen->base, &attribs, &ctx_err,
         if (ctx->st == NULL) {
      switch (ctx_err) {
   *error = __DRI_CTX_ERROR_SUCCESS;
   break;
         *error = __DRI_CTX_ERROR_NO_MEMORY;
   break;
         *error = __DRI_CTX_ERROR_BAD_VERSION;
   break;
         }
      }
            if (ctx->st->cso_context) {
      ctx->pp = pp_init(ctx->st->pipe, screen->pp_enabled, ctx->st->cso_context,
         ctx->hud = hud_create(ctx->st->cso_context,
                     /* order of precedence (least to most):
   * - driver setting
   * - app setting
   * - user setting
   */
            /* always disable glthread by default if fewer than 5 "big" CPUs are active */
   unsigned nr_big_cpus = util_get_cpu_caps()->nr_big_cpus;
   if (util_get_cpu_caps()->nr_cpus < 4 || (nr_big_cpus && nr_big_cpus < 5))
            int app_enable_glthread = driQueryOptioni(&screen->dev->option_cache, "mesa_glthread_app_profile");
   if (app_enable_glthread != -1) {
      /* if set (not -1), apply the app setting */
      }
   if (getenv("mesa_glthread")) {
      /* only apply the env var if set */
   bool user_enable_glthread = debug_get_bool_option("mesa_glthread", false);
   if (user_enable_glthread != enable_glthread) {
      /* print warning to mimic old behavior */
      }
      }
   /* Do this last. */
   if (enable_glthread) {
               /* This is only needed by X11/DRI2, which can be unsafe. */
   if (backgroundCallable &&
      backgroundCallable->base.version >= 2 &&
   backgroundCallable->isThreadSafe &&
               if (safe)
               *error = __DRI_CTX_ERROR_SUCCESS;
         fail:
      if (ctx && ctx->st)
            free(ctx);
      }
      void
   dri_destroy_context(struct dri_context *ctx)
   {
      /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
            if (ctx->hud) {
                  if (ctx->pp)
            /* No particular reason to wait for command completion before
   * destroying a context, but we flush the context here
   * to avoid having to add code elsewhere to cope with flushing a
   * partially destroyed context.
   */
   st_context_flush(ctx->st, 0, NULL, NULL, NULL);
   st_destroy_context(ctx->st);
      }
      /* This is called inside MakeCurrent to unbind the context. */
   bool
   dri_unbind_context(struct dri_context *ctx)
   {
      /* dri_util.c ensures cPriv is not null */
            if (st == st_api_get_current()) {
               /* Record HUD queries for the duration the context was "current". */
   if (ctx->hud)
                        if (ctx->draw || ctx->read) {
                        if (ctx->read != ctx->draw)
            ctx->draw = NULL;
                  }
      bool
   dri_make_current(struct dri_context *ctx,
      struct dri_drawable *draw,
      {
      /* dri_unbind_context() is always called before this, so drawables are
   * always NULL here.
   */
   assert(!ctx->draw);
            if ((draw && !read) || (!draw && read))
            /* Wait for glthread to finish because we can't use st_context from
   * multiple threads.
   */
            /* There are 2 cases that can occur here. Either we bind drawables, or we
   * bind NULL for configless and surfaceless contexts.
   */
   if (!draw && !read)
            /* Bind drawables to the context */
   ctx->draw = draw;
            dri_get_drawable(draw);
            if (draw != read) {
      dri_get_drawable(read);
                        /* This is ok to call here. If they are already init, it's a no-op. */
   if (ctx->pp && draw->textures[ST_ATTACHMENT_BACK_LEFT])
      pp_init_fbos(ctx->pp, draw->textures[ST_ATTACHMENT_BACK_LEFT]->width0,
            }
      struct dri_context *
   dri_get_current(void)
   {
                  }
      /* vim: set sw=3 ts=8 sts=3 expandtab: */
