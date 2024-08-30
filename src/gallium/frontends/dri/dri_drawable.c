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
   #include "dri_context.h"
   #include "dri_drawable.h"
      #include "pipe/p_screen.h"
   #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
      #include "state_tracker/st_context.h"
      static uint32_t drifb_ID = 0;
      static bool
   dri_st_framebuffer_validate(struct st_context *st,
                                 {
      struct dri_context *ctx = (struct dri_context *)st->frontend_context;
   struct dri_drawable *drawable = (struct dri_drawable *)pdrawable;
   struct dri_screen *screen = drawable->screen;
   unsigned statt_mask, new_mask;
   bool new_stamp;
   int i;
   unsigned int lastStamp;
   struct pipe_resource **textures =
      drawable->stvis.samples > 1 ? drawable->msaa_textures
         statt_mask = 0x0;
   for (i = 0; i < count; i++)
            /* record newly allocated textures */
            do {
      lastStamp = drawable->lastStamp;
            if (new_stamp || new_mask) {
                              /* add existing textures */
   for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
      if (textures[i])
               drawable->texture_stamp = lastStamp;
                  /* Flush the pending set_damage_region request. */
            if (new_mask & (1 << ST_ATTACHMENT_BACK_LEFT) &&
      pscreen->set_damage_region) {
            pscreen->set_damage_region(pscreen, resource,
                     if (!out)
            /* Set the window-system buffers for the gallium frontend. */
   for (i = 0; i < count; i++)
         if (resolve && drawable->stvis.samples > 1) {
      if (statt_mask & BITFIELD_BIT(ST_ATTACHMENT_FRONT_LEFT))
         else if (statt_mask & BITFIELD_BIT(ST_ATTACHMENT_BACK_LEFT))
                  }
      static bool
   dri_st_framebuffer_flush_front(struct st_context *st,
               {
      struct dri_context *ctx = (struct dri_context *)st->frontend_context;
            /* XXX remove this and just set the correct one on the framebuffer */
      }
      /**
   * The gallium frontend framebuffer interface flush_swapbuffers callback
   */
   static bool
   dri_st_framebuffer_flush_swapbuffers(struct st_context *st,
         {
      struct dri_context *ctx = (struct dri_context *)st->frontend_context;
            if (drawable->flush_swapbuffers)
               }
      /**
   * This is called when we need to set up GL rendering to a new X window.
   */
   struct dri_drawable *
   dri_create_drawable(struct dri_screen *screen, const struct gl_config *visual,
         {
               if (isPixmap)
            drawable = CALLOC_STRUCT(dri_drawable);
   if (drawable == NULL)
            drawable->loaderPrivate = loaderPrivate;
   drawable->refcount = 1;
   drawable->lastStamp = 0;
   drawable->w = 0;
                     /* setup the pipe_frontend_drawable */
   drawable->base.visual = &drawable->stvis;
   drawable->base.flush_front = dri_st_framebuffer_flush_front;
   drawable->base.validate = dri_st_framebuffer_validate;
                     p_atomic_set(&drawable->base.stamp, 1);
   drawable->base.ID = p_atomic_inc_return(&drifb_ID);
               fail:
      FREE(drawable);
      }
      static void
   dri_destroy_drawable(struct dri_drawable *drawable)
   {
      struct dri_screen *screen = drawable->screen;
            for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
         for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
            screen->base.screen->fence_reference(screen->base.screen,
            /* Notify the st manager that this drawable is no longer valid */
            FREE(drawable->damage_rects);
      }
      void
   dri_put_drawable(struct dri_drawable *drawable)
   {
      if (drawable) {
      int refcount = --drawable->refcount;
            if (!refcount)
         }
      /**
   * Validate the texture at an attachment.  Allocate the texture if it does not
   * exist.  Used by the TFP extension.
   */
   static void
   dri_drawable_validate_att(struct dri_context *ctx,
               {
      enum st_attachment_type statts[ST_ATTACHMENT_COUNT];
            /* check if buffer already exists */
   if (drawable->texture_mask & (1 << statt))
            /* make sure DRI2 does not destroy existing buffers */
   for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
      if (drawable->texture_mask & (1 << i)) {
            }
                        }
      /**
   * These are used for GLX_EXT_texture_from_pixmap
   */
   static void
   dri_set_tex_buffer2(__DRIcontext *pDRICtx, GLint target,
         {
      struct dri_context *ctx = dri_context(pDRICtx);
   struct st_context *st = ctx->st;
   struct dri_drawable *drawable = dri_drawable(dPriv);
                              /* Use the pipe resource associated with the X drawable */
            if (pt) {
               if (format == __DRI_TEXTURE_FORMAT_RGB)  {
      /* only need to cover the formats recognized by dri_fill_st_visual */
   switch (internal_format) {
   case PIPE_FORMAT_R16G16B16A16_FLOAT:
      internal_format = PIPE_FORMAT_R16G16B16X16_FLOAT;
      case PIPE_FORMAT_B10G10R10A2_UNORM:
      internal_format = PIPE_FORMAT_B10G10R10X2_UNORM;
      case PIPE_FORMAT_R10G10B10A2_UNORM:
      internal_format = PIPE_FORMAT_R10G10B10X2_UNORM;
      case PIPE_FORMAT_BGRA8888_UNORM:
      internal_format = PIPE_FORMAT_BGRX8888_UNORM;
      case PIPE_FORMAT_ARGB8888_UNORM:
      internal_format = PIPE_FORMAT_XRGB8888_UNORM;
      default:
                                    }
      static void
   dri_set_tex_buffer(__DRIcontext *pDRICtx, GLint target,
         {
         }
      const __DRItexBufferExtension driTexBufferExtension = {
               .setTexBuffer       = dri_set_tex_buffer,
   .setTexBuffer2      = dri_set_tex_buffer2,
      };
      /**
   * Get the format and binding of an attachment.
   */
   void
   dri_drawable_get_format(struct dri_drawable *drawable,
                     {
      switch (statt) {
   case ST_ATTACHMENT_FRONT_LEFT:
   case ST_ATTACHMENT_BACK_LEFT:
   case ST_ATTACHMENT_FRONT_RIGHT:
   case ST_ATTACHMENT_BACK_RIGHT:
      /* Other pieces of the driver stack get confused and behave incorrectly
   * when they get an sRGB drawable. st/mesa receives "drawable->stvis"
   * though other means and handles it correctly, so we don't really need
   * to use an sRGB format here.
   */
   *format = util_format_linear(drawable->stvis.color_format);
   *bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
      case ST_ATTACHMENT_DEPTH_STENCIL:
      *format = drawable->stvis.depth_stencil_format;
   *bind = PIPE_BIND_DEPTH_STENCIL; /* XXX sampler? */
      default:
      *format = PIPE_FORMAT_NONE;
   *bind = 0;
         }
      void
   dri_pipe_blit(struct pipe_context *pipe,
               {
               if (!dst || !src)
            /* From the GL spec, version 4.2, section 4.1.11 (Additional Multisample
   *  Fragment Operations):
   *
   *      If a framebuffer object is not bound, after all operations have
   *      been completed on the multisample buffer, the sample values for
   *      each color in the multisample buffer are combined to produce a
   *      single color value, and that value is written into the
   *      corresponding color buffers selected by DrawBuffer or
   *      DrawBuffers. An implementation may defer the writing of the color
   *      buffers until a later time, but the state of the framebuffer must
   *      behave as if the color buffers were updated as each fragment was
   *      processed. The method of combination is not specified. If the
   *      framebuffer contains sRGB values, then it is recommended that the
   *      an average of sample values is computed in a linearized space, as
   *      for blending (see section 4.1.7).
   *
   * In other words, to do a resolve operation in a linear space, we have
   * to set sRGB formats if the original resources were sRGB, so don't use
   * util_format_linear.
            memset(&blit, 0, sizeof(blit));
   blit.dst.resource = dst;
   blit.dst.box.width = dst->width0;
   blit.dst.box.height = dst->height0;
   blit.dst.box.depth = 1;
   blit.dst.format = dst->format;
   blit.src.resource = src;
   blit.src.box.width = src->width0;
   blit.src.box.height = src->height0;
   blit.src.box.depth = 1;
   blit.src.format = src->format;
   blit.mask = PIPE_MASK_RGBA;
               }
      static void
   dri_postprocessing(struct dri_context *ctx,
               {
      struct pipe_resource *src = drawable->textures[att];
            if (ctx->pp && src)
      }
      struct notify_before_flush_cb_args {
      struct dri_context *ctx;
   struct dri_drawable *drawable;
   unsigned flags;
   enum __DRI2throttleReason reason;
      };
      static void
   notify_before_flush_cb(void* _args)
   {
      struct notify_before_flush_cb_args *args = (struct notify_before_flush_cb_args *) _args;
   struct st_context *st = args->ctx->st;
            /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
            if (args->drawable->stvis.samples > 1 &&
      (args->reason == __DRI2_THROTTLE_SWAPBUFFER ||
   args->reason == __DRI2_NOTHROTTLE_SWAPBUFFER ||
   args->reason == __DRI2_THROTTLE_COPYSUBBUFFER)) {
   /* Resolve the MSAA back buffer. */
   dri_pipe_blit(st->pipe,
                  if ((args->reason == __DRI2_THROTTLE_SWAPBUFFER ||
      args->reason == __DRI2_NOTHROTTLE_SWAPBUFFER) &&
   args->drawable->msaa_textures[ST_ATTACHMENT_FRONT_LEFT] &&
   args->drawable->msaa_textures[ST_ATTACHMENT_BACK_LEFT]) {
                                    if (pipe->invalidate_resource &&
      (args->flags & __DRI2_FLUSH_INVALIDATE_ANCILLARY)) {
   if (args->drawable->textures[ST_ATTACHMENT_DEPTH_STENCIL])
         if (args->drawable->msaa_textures[ST_ATTACHMENT_DEPTH_STENCIL])
               if (args->ctx->hud) {
      hud_run(args->ctx->hud, args->ctx->st->cso_context,
                  }
      /**
   * DRI2 flush extension, the flush_with_flags function.
   *
   * \param context           the context
   * \param drawable          the drawable to flush
   * \param flags             a combination of _DRI2_FLUSH_xxx flags
   * \param throttle_reason   the reason for throttling, 0 = no throttling
   */
   void
   dri_flush(__DRIcontext *cPriv,
            __DRIdrawable *dPriv,
      {
      struct dri_context *ctx = dri_context(cPriv);
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct st_context *st;
   unsigned flush_flags;
            if (!ctx) {
      assert(0);
               st = ctx->st;
            if (drawable) {
      /* prevent recursion */
   if (drawable->flushing)
               }
   else {
                  if ((flags & __DRI2_FLUSH_DRAWABLE) &&
      drawable->textures[ST_ATTACHMENT_BACK_LEFT]) {
   /* We can't do operations on the back buffer here, because there
   * may be some pending operations that will get flushed by the
   * call to st->flush (eg: FLUSH_VERTICES).
   * Instead we register a callback to be notified when all operations
   * have been submitted but before the call to st_flush.
   */
   args.ctx = ctx;
   args.drawable = drawable;
   args.flags = flags;
               flush_flags = 0;
   if (flags & __DRI2_FLUSH_CONTEXT)
         if (reason == __DRI2_THROTTLE_SWAPBUFFER ||
      reason == __DRI2_NOTHROTTLE_SWAPBUFFER)
         /* Flush the context and throttle if needed. */
   if (ctx->screen->throttle &&
      drawable &&
   (reason == __DRI2_THROTTLE_SWAPBUFFER ||
            struct pipe_screen *screen = drawable->screen->base.screen;
                     /* throttle on the previous fence */
   if (drawable->throttle_fence) {
      screen->fence_finish(screen, NULL, drawable->throttle_fence, OS_TIMEOUT_INFINITE);
      }
      }
   else if (flags & (__DRI2_FLUSH_DRAWABLE | __DRI2_FLUSH_CONTEXT)) {
                  if (drawable) {
                  /* Swap the MSAA front and back buffers, so that reading
   * from the front buffer after SwapBuffers returns what was
   * in the back buffer.
   */
   if (args.swap_msaa_buffers) {
      struct pipe_resource *tmp =
            drawable->msaa_textures[ST_ATTACHMENT_FRONT_LEFT] =
                  /* Now that we have swapped the buffers, this tells the gallium
   * frontend to revalidate the framebuffer.
   */
                  }
      /**
   * DRI2 flush extension.
   */
   void
   dri_flush_drawable(__DRIdrawable *dPriv)
   {
               if (ctx)
      }
      /**
   * dri_throttle - A DRI2ThrottleExtension throttling function.
   */
   static void
   dri_throttle(__DRIcontext *cPriv, __DRIdrawable *dPriv,
         {
         }
         const __DRI2throttleExtension dri2ThrottleExtension = {
                  };
         /* vim: set sw=3 ts=8 sts=3 expandtab: */
