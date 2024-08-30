   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2010 LunarG Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Chia-I Wu <olv@lunarg.com>
   */
      #include "main/mtypes.h"
   #include "main/extensions.h"
   #include "main/context.h"
   #include "main/debug_output.h"
   #include "main/framebuffer.h"
   #include "main/glthread.h"
   #include "main/texobj.h"
   #include "main/teximage.h"
   #include "main/texstate.h"
   #include "main/errors.h"
   #include "main/framebuffer.h"
   #include "main/fbobject.h"
   #include "main/renderbuffer.h"
   #include "main/version.h"
   #include "util/hash_table.h"
   #include "st_texture.h"
      #include "st_context.h"
   #include "st_debug.h"
   #include "st_extensions.h"
   #include "st_format.h"
   #include "st_cb_bitmap.h"
   #include "st_cb_flush.h"
   #include "st_manager.h"
   #include "st_sampler_view.h"
   #include "st_util.h"
      #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/format/u_format.h"
   #include "util/u_helpers.h"
   #include "util/u_pointer.h"
   #include "util/u_inlines.h"
   #include "util/u_atomic.h"
   #include "util/u_surface.h"
   #include "util/list.h"
   #include "util/u_memory.h"
   #include "util/perf/cpu_trace.h"
      struct hash_table;
      struct st_screen
   {
      struct hash_table *drawable_ht; /* pipe_frontend_drawable objects hash table */
      };
      /**
   * Cast wrapper to convert a struct gl_framebuffer to an gl_framebuffer.
   * Return NULL if the struct gl_framebuffer is a user-created framebuffer.
   * We'll only return non-null for window system framebuffers.
   * Note that this function may fail.
   */
   static inline struct gl_framebuffer *
   st_ws_framebuffer(struct gl_framebuffer *fb)
   {
      /* FBO cannot be casted.  See st_new_framebuffer */
   if (fb && _mesa_is_winsys_fbo(fb) &&
      fb != _mesa_get_incomplete_framebuffer())
         }
      /**
   * Map an attachment to a buffer index.
   */
   static inline gl_buffer_index
   attachment_to_buffer_index(enum st_attachment_type statt)
   {
               switch (statt) {
   case ST_ATTACHMENT_FRONT_LEFT:
      index = BUFFER_FRONT_LEFT;
      case ST_ATTACHMENT_BACK_LEFT:
      index = BUFFER_BACK_LEFT;
      case ST_ATTACHMENT_FRONT_RIGHT:
      index = BUFFER_FRONT_RIGHT;
      case ST_ATTACHMENT_BACK_RIGHT:
      index = BUFFER_BACK_RIGHT;
      case ST_ATTACHMENT_DEPTH_STENCIL:
      index = BUFFER_DEPTH;
      case ST_ATTACHMENT_ACCUM:
      index = BUFFER_ACCUM;
      default:
      index = BUFFER_COUNT;
                  }
         /**
   * Map a buffer index to an attachment.
   */
   static inline enum st_attachment_type
   buffer_index_to_attachment(gl_buffer_index index)
   {
               switch (index) {
   case BUFFER_FRONT_LEFT:
      statt = ST_ATTACHMENT_FRONT_LEFT;
      case BUFFER_BACK_LEFT:
      statt = ST_ATTACHMENT_BACK_LEFT;
      case BUFFER_FRONT_RIGHT:
      statt = ST_ATTACHMENT_FRONT_RIGHT;
      case BUFFER_BACK_RIGHT:
      statt = ST_ATTACHMENT_BACK_RIGHT;
      case BUFFER_DEPTH:
      statt = ST_ATTACHMENT_DEPTH_STENCIL;
      case BUFFER_ACCUM:
      statt = ST_ATTACHMENT_ACCUM;
      default:
      statt = ST_ATTACHMENT_INVALID;
                  }
         /**
   * Make sure a context picks up the latest cached state of the
   * drawables it binds to.
   */
   static void
   st_context_validate(struct st_context *st,
               {
      if (stdraw && stdraw->stamp != st->draw_stamp) {
      st->ctx->NewDriverState |= ST_NEW_FRAMEBUFFER;
   _mesa_resize_framebuffer(st->ctx, stdraw,
                           if (stread && stread->stamp != st->read_stamp) {
      if (stread != stdraw) {
      st->ctx->NewDriverState |= ST_NEW_FRAMEBUFFER;
   _mesa_resize_framebuffer(st->ctx, stread,
            }
         }
         void
   st_set_ws_renderbuffer_surface(struct gl_renderbuffer *rb,
         {
      pipe_surface_reference(&rb->surface_srgb, NULL);
            if (util_format_is_srgb(surf->format))
         else
            rb->surface = surf; /* just assign, don't ref */
            rb->Width = surf->width;
      }
         /**
   * Validate a framebuffer to make sure up-to-date pipe_textures are used.
   * The context is only used for creating pipe surfaces and for calling
   * _mesa_resize_framebuffer().
   * (That should probably be rethought, since those surfaces become
   * drawable state, not context state, and can be freed by another pipe
   * context).
   */
   static void
   st_framebuffer_validate(struct gl_framebuffer *stfb,
         {
      struct pipe_resource *textures[ST_ATTACHMENT_COUNT];
   struct pipe_resource *resolve = NULL;
   uint width, height;
   unsigned i;
   bool changed = false;
            new_stamp = p_atomic_read(&stfb->drawable->stamp);
   if (stfb->drawable_stamp == new_stamp)
                     /* validate the fb */
   do {
      if (!stfb->drawable->validate(st, stfb->drawable, stfb->statts,
                  stfb->drawable_stamp = new_stamp;
               width = stfb->Width;
            for (i = 0; i < stfb->num_statts; i++) {
      struct gl_renderbuffer *rb;
   struct pipe_surface *ps, surf_tmpl;
            if (!textures[i])
            idx = attachment_to_buffer_index(stfb->statts[i]);
   if (idx >= BUFFER_COUNT) {
      pipe_resource_reference(&textures[i], NULL);
               rb = stfb->Attachment[idx].Renderbuffer;
   assert(rb);
   if (rb->texture == textures[i] &&
      rb->Width == textures[i]->width0 &&
   rb->Height == textures[i]->height0) {
   pipe_resource_reference(&textures[i], NULL);
               u_surface_default_template(&surf_tmpl, textures[i]);
   ps = st->pipe->create_surface(st->pipe, textures[i], &surf_tmpl);
   if (ps) {
                              width = rb->Width;
                           changed |= resolve != stfb->resolve;
   /* ref is removed here */
   pipe_resource_reference(&stfb->resolve, NULL);
   /* ref is taken here */
            if (changed) {
      ++stfb->stamp;
         }
      /**
   * Return true if the visual has the specified buffers.
   */
   static inline bool
   st_visual_have_buffers(const struct st_visual *visual, unsigned mask)
   {
         }
      /**
   * Update the attachments to validate by looping the existing renderbuffers.
   */
   static void
   st_framebuffer_update_attachments(struct gl_framebuffer *stfb)
   {
                        for (enum st_attachment_type i = 0; i < ST_ATTACHMENT_COUNT; i++)
            for (idx = 0; idx < BUFFER_COUNT; idx++) {
      struct gl_renderbuffer *rb;
            rb = stfb->Attachment[idx].Renderbuffer;
   if (!rb || rb->software)
            statt = buffer_index_to_attachment(idx);
   if (statt != ST_ATTACHMENT_INVALID &&
      st_visual_have_buffers(stfb->drawable->visual, 1 << statt))
   }
      }
      /**
   * Allocate a renderbuffer for an on-screen window (not a user-created
   * renderbuffer).  The window system code determines the format.
   */
   static struct gl_renderbuffer *
   st_new_renderbuffer_fb(enum pipe_format format, unsigned samples, bool sw)
   {
               rb = CALLOC_STRUCT(gl_renderbuffer);
   if (!rb) {
      _mesa_error(NULL, GL_OUT_OF_MEMORY, "creating renderbuffer");
               _mesa_init_renderbuffer(rb, 0);
   rb->ClassID = 0x4242; /* just a unique value */
   rb->NumSamples = samples;
   rb->NumStorageSamples = samples;
   rb->Format = st_pipe_format_to_mesa_format(format);
   rb->_BaseFormat = _mesa_get_format_base_format(rb->Format);
            switch (format) {
   case PIPE_FORMAT_B10G10R10A2_UNORM:
   case PIPE_FORMAT_R10G10B10A2_UNORM:
      rb->InternalFormat = GL_RGB10_A2;
      case PIPE_FORMAT_R10G10B10X2_UNORM:
   case PIPE_FORMAT_B10G10R10X2_UNORM:
      rb->InternalFormat = GL_RGB10;
      case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      rb->InternalFormat = GL_RGBA8;
      case PIPE_FORMAT_R8G8B8X8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_X8R8G8B8_UNORM:
   case PIPE_FORMAT_R8G8B8_UNORM:
      rb->InternalFormat = GL_RGB8;
      case PIPE_FORMAT_R8G8B8A8_SRGB:
   case PIPE_FORMAT_B8G8R8A8_SRGB:
   case PIPE_FORMAT_A8R8G8B8_SRGB:
      rb->InternalFormat = GL_SRGB8_ALPHA8;
      case PIPE_FORMAT_R8G8B8X8_SRGB:
   case PIPE_FORMAT_B8G8R8X8_SRGB:
   case PIPE_FORMAT_X8R8G8B8_SRGB:
      rb->InternalFormat = GL_SRGB8;
      case PIPE_FORMAT_B5G5R5A1_UNORM:
      rb->InternalFormat = GL_RGB5_A1;
      case PIPE_FORMAT_B4G4R4A4_UNORM:
      rb->InternalFormat = GL_RGBA4;
      case PIPE_FORMAT_B5G6R5_UNORM:
      rb->InternalFormat = GL_RGB565;
      case PIPE_FORMAT_Z16_UNORM:
      rb->InternalFormat = GL_DEPTH_COMPONENT16;
      case PIPE_FORMAT_Z32_UNORM:
      rb->InternalFormat = GL_DEPTH_COMPONENT32;
      case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      rb->InternalFormat = GL_DEPTH24_STENCIL8_EXT;
      case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      rb->InternalFormat = GL_DEPTH_COMPONENT24;
      case PIPE_FORMAT_S8_UINT:
      rb->InternalFormat = GL_STENCIL_INDEX8_EXT;
      case PIPE_FORMAT_R16G16B16A16_SNORM:
      /* accum buffer */
   rb->InternalFormat = GL_RGBA16_SNORM;
      case PIPE_FORMAT_R16G16B16A16_UNORM:
      rb->InternalFormat = GL_RGBA16;
      case PIPE_FORMAT_R16G16B16_UNORM:
      rb->InternalFormat = GL_RGB16;
      case PIPE_FORMAT_R8_UNORM:
      rb->InternalFormat = GL_R8;
      case PIPE_FORMAT_R8G8_UNORM:
      rb->InternalFormat = GL_RG8;
      case PIPE_FORMAT_R16_UNORM:
      rb->InternalFormat = GL_R16;
      case PIPE_FORMAT_R16G16_UNORM:
      rb->InternalFormat = GL_RG16;
      case PIPE_FORMAT_R32G32B32A32_FLOAT:
      rb->InternalFormat = GL_RGBA32F;
      case PIPE_FORMAT_R32G32B32X32_FLOAT:
   case PIPE_FORMAT_R32G32B32_FLOAT:
      rb->InternalFormat = GL_RGB32F;
      case PIPE_FORMAT_R16G16B16A16_FLOAT:
      rb->InternalFormat = GL_RGBA16F;
      case PIPE_FORMAT_R16G16B16X16_FLOAT:
      rb->InternalFormat = GL_RGB16F;
      default:
      _mesa_problem(NULL,
               FREE(rb);
                           }
      /**
   * Add a renderbuffer to the framebuffer.  The framebuffer is one that
   * corresponds to a window and is not a user-created FBO.
   */
   static bool
   st_framebuffer_add_renderbuffer(struct gl_framebuffer *stfb,
         {
      struct gl_renderbuffer *rb;
   enum pipe_format format;
                     /* do not distinguish depth/stencil buffers */
   if (idx == BUFFER_STENCIL)
            switch (idx) {
   case BUFFER_DEPTH:
      format = stfb->drawable->visual->depth_stencil_format;
   sw = false;
      case BUFFER_ACCUM:
      format = stfb->drawable->visual->accum_format;
   sw = true;
      default:
      format = stfb->drawable->visual->color_format;
   if (prefer_srgb)
         sw = false;
               if (format == PIPE_FORMAT_NONE)
            rb = st_new_renderbuffer_fb(format, stfb->drawable->visual->samples, sw);
   if (!rb)
            if (idx != BUFFER_DEPTH) {
      _mesa_attach_and_own_rb(stfb, idx, rb);
               bool rb_ownership_taken = false;
   if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_ZS, 0)) {
      _mesa_attach_and_own_rb(stfb, BUFFER_DEPTH, rb);
               if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_ZS, 1)) {
      if (rb_ownership_taken)
         else
                  }
         /**
   * Intialize a struct gl_config from a visual.
   */
   static void
   st_visual_to_context_mode(const struct st_visual *visual,
         {
               if (st_visual_have_buffers(visual, ST_ATTACHMENT_BACK_LEFT_MASK))
            if (st_visual_have_buffers(visual,
                  if (visual->color_format != PIPE_FORMAT_NONE) {
      mode->redBits =
      util_format_get_component_bits(visual->color_format,
      mode->greenBits =
      util_format_get_component_bits(visual->color_format,
      mode->blueBits =
      util_format_get_component_bits(visual->color_format,
      mode->alphaBits =
                  mode->rgbBits = mode->redBits +
         mode->sRGBCapable = util_format_is_srgb(visual->color_format);
               if (visual->depth_stencil_format != PIPE_FORMAT_NONE) {
      mode->depthBits =
      util_format_get_component_bits(visual->depth_stencil_format,
      mode->stencilBits =
      util_format_get_component_bits(visual->depth_stencil_format,
            if (visual->accum_format != PIPE_FORMAT_NONE) {
      mode->accumRedBits =
      util_format_get_component_bits(visual->accum_format,
      mode->accumGreenBits =
      util_format_get_component_bits(visual->accum_format,
      mode->accumBlueBits =
      util_format_get_component_bits(visual->accum_format,
      mode->accumAlphaBits =
      util_format_get_component_bits(visual->accum_format,
            if (visual->samples > 1) {
            }
         /**
   * Create a framebuffer from a manager interface.
   */
   static struct gl_framebuffer *
   st_framebuffer_create(struct st_context *st,
         {
      struct gl_framebuffer *stfb;
   struct gl_config mode;
   gl_buffer_index idx;
            if (!drawable)
            stfb = CALLOC_STRUCT(gl_framebuffer);
   if (!stfb)
                     /*
   * For desktop GL, sRGB framebuffer write is controlled by both the
   * capability of the framebuffer and GL_FRAMEBUFFER_SRGB.  We should
   * advertise the capability when the pipe driver (and core Mesa) supports
   * it so that applications can enable sRGB write when they want to.
   *
   * This is not to be confused with GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB.  When
   * the attribute is GLX_TRUE, it tells the st manager to pick a color
   * format such that util_format_srgb(visual->color_format) can be supported
   * by the pipe driver.  We still need to advertise the capability here.
   *
   * For GLES, however, sRGB framebuffer write is initially only controlled
   * by the capability of the framebuffer, with GL_EXT_sRGB_write_control
   * control is given back to the applications, but GL_FRAMEBUFFER_SRGB is
   * still enabled by default since this is the behaviour when
   * EXT_sRGB_write_control is not available. Since GL_EXT_sRGB_write_control
   * brings GLES on par with desktop GLs EXT_framebuffer_sRGB, in mesa this
   * is also expressed by using the same extension flag
   */
   if (_mesa_has_EXT_framebuffer_sRGB(st->ctx)) {
      struct pipe_screen *screen = st->screen;
   const enum pipe_format srgb_format =
            if (srgb_format != PIPE_FORMAT_NONE &&
      st_pipe_format_to_mesa_format(srgb_format) != MESA_FORMAT_NONE &&
   screen->is_format_supported(screen, srgb_format,
                           mode.sRGBCapable = GL_TRUE;
   /* Since GL_FRAMEBUFFER_SRGB is enabled by default on GLES we must not
   * create renderbuffers with an sRGB format derived from the
   * visual->color_format, but we still want sRGB for desktop GL.
   */
                           stfb->drawable = drawable;
   stfb->drawable_ID = drawable->ID;
            /* add the color buffer */
   idx = stfb->_ColorDrawBufferIndexes[0];
   if (!st_framebuffer_add_renderbuffer(stfb, idx, prefer_srgb)) {
      FREE(stfb);
               st_framebuffer_add_renderbuffer(stfb, BUFFER_DEPTH, false);
            stfb->stamp = 0;
               }
         static uint32_t
   drawable_hash(const void *key)
   {
         }
         static bool
   drawable_equal(const void *a, const void *b)
   {
         }
         static bool
   drawable_lookup(struct pipe_frontend_screen *fscreen,
         {
      struct st_screen *screen =
                  assert(screen);
            simple_mtx_lock(&screen->st_mutex);
   entry = _mesa_hash_table_search(screen->drawable_ht, drawable);
               }
         static bool
   drawable_insert(struct pipe_frontend_screen *fscreen,
         {
      struct st_screen *screen =
                  assert(screen);
            simple_mtx_lock(&screen->st_mutex);
   entry = _mesa_hash_table_insert(screen->drawable_ht, drawable, drawable);
               }
         static void
   drawable_remove(struct pipe_frontend_screen *fscreen,
         {
      struct st_screen *screen =
                  if (!screen || !screen->drawable_ht)
            simple_mtx_lock(&screen->st_mutex);
   entry = _mesa_hash_table_search(screen->drawable_ht, drawable);
   if (!entry)
                  unlock:
         }
         /**
   * The framebuffer interface object is no longer valid.
   * Remove the object from the framebuffer interface hash table.
   */
   void
   st_api_destroy_drawable(struct pipe_frontend_drawable *drawable)
   {
      if (!drawable)
               }
         /**
   * Purge the winsys buffers list to remove any references to
   * non-existing framebuffer interface objects.
   */
   static void
   st_framebuffers_purge(struct st_context *st)
   {
      struct pipe_frontend_screen *fscreen = st->frontend_screen;
                     LIST_FOR_EACH_ENTRY_SAFE_REV(stfb, next, &st->winsys_buffers, head) {
                        /**
   * If the corresponding framebuffer interface object no longer exists,
   * remove the framebuffer object from the context's winsys buffers list,
   * and unreference the framebuffer object, so its resources can be
   * deleted.
   */
   if (!drawable_lookup(fscreen, drawable)) {
      list_del(&stfb->head);
            }
         void
   st_context_flush(struct st_context *st, unsigned flags,
               {
                        if (flags & ST_FLUSH_END_OF_FRAME)
         if (flags & ST_FLUSH_FENCE_FD)
            /* We can do these in any order because FLUSH_VERTICES will also flush
   * the bitmap cache if there are any unflushed vertices.
   */
   st_flush_bitmap_cache(st);
            /* Notify the caller that we're ready to flush */
   if (before_flush_cb)
                  if ((flags & ST_FLUSH_WAIT) && fence && *fence) {
      st->screen->fence_finish(st->screen, NULL, *fence,
                     if (flags & ST_FLUSH_FRONT)
      }
      /**
   * Replace the texture image of a texture object at the specified level.
   *
   * This is only for GLX_EXT_texture_from_pixmap and equivalent features
   * in EGL and WGL.
   */
   bool
   st_context_teximage(struct st_context *st, GLenum target,
               {
      struct gl_context *ctx = st->ctx;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLenum internalFormat;
                              /* switch to surface based */
   if (!texObj->surface_based) {
      _mesa_clear_texture_object(ctx, texObj, NULL);
               texImage = _mesa_get_tex_image(ctx, texObj, target, level);
   if (tex) {
               if (util_format_has_alpha(tex->format))
         else
            _mesa_init_teximage_fields(ctx, texImage,
                  width = tex->width0;
   height = tex->height0;
            /* grow the image size until we hit level = 0 */
   while (level > 0) {
      if (width != 1)
         if (height != 1)
         if (depth != 1)
               }
   else {
      _mesa_clear_texture_image(ctx, texImage);
      }
            pipe_resource_reference(&texObj->pt, tex);
   st_texture_release_all_sampler_views(st, texObj);
   pipe_resource_reference(&texImage->pt, tex);
                     _mesa_dirty_texobj(ctx, texObj);
   ctx->Shared->HasExternallySharedImages = true;
               }
         /**
   * Invalidate states to notify the frontend that driver states have been
   * changed behind its back.
   */
   void
   st_context_invalidate_state(struct st_context *st, unsigned flags)
   {
               if (flags & ST_INVALIDATE_FS_SAMPLER_VIEWS)
         if (flags & ST_INVALIDATE_FS_CONSTBUF0)
         if (flags & ST_INVALIDATE_VS_CONSTBUF0)
         if (flags & ST_INVALIDATE_VERTEX_BUFFERS) {
      ctx->Array.NewVertexElements = true;
      }
   if (flags & ST_INVALIDATE_FB_STATE)
      }
         void
   st_screen_destroy(struct pipe_frontend_screen *fscreen)
   {
               if (screen && screen->drawable_ht) {
      _mesa_hash_table_destroy(screen->drawable_ht, NULL);
   simple_mtx_destroy(&screen->st_mutex);
   FREE(screen);
         }
         /**
   * Create a rendering context.
   */
   struct st_context *
   st_api_create_context(struct pipe_frontend_screen *fscreen,
                     {
      struct st_context *st;
   struct pipe_context *pipe;
   struct gl_config mode, *mode_ptr = &mode;
                     /* Create a hash table for the framebuffer interface objects
   * if it has not been created for this st manager.
   */
   if (fscreen->st_screen == NULL) {
               screen = CALLOC_STRUCT(st_screen);
   simple_mtx_init(&screen->st_mutex, mtx_plain);
   screen->drawable_ht = _mesa_hash_table_create(NULL,
                           if (attribs->flags & ST_CONTEXT_FLAG_NO_ERROR)
            /* OpenGL ES 2.0+ does not support sampler state LOD bias. If we are creating
   * a GLES context, communicate that to the the driver to allow optimization.
   */
   bool is_gles = attribs->profile == API_OPENGLES2;
            pipe = fscreen->screen->context_create(fscreen->screen, NULL,
                     if (!pipe) {
      *error = ST_CONTEXT_ERROR_NO_MEMORY;
               st_visual_to_context_mode(&attribs->visual, &mode);
   if (attribs->visual.color_format == PIPE_FORMAT_NONE)
         st = st_create_context(attribs->profile, pipe, mode_ptr, shared_ctx,
               if (!st) {
      *error = ST_CONTEXT_ERROR_NO_MEMORY;
   pipe->destroy(pipe);
               if (attribs->flags & ST_CONTEXT_FLAG_DEBUG) {
      if (!_mesa_set_debug_state_int(st->ctx, GL_DEBUG_OUTPUT, GL_TRUE)) {
      *error = ST_CONTEXT_ERROR_NO_MEMORY;
                           if (st->ctx->Const.ContextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) {
                  if (attribs->flags & ST_CONTEXT_FLAG_FORWARD_COMPATIBLE)
            if (attribs->context_flags & PIPE_CONTEXT_ROBUST_BUFFER_ACCESS) {
      st->ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB;
               if (attribs->context_flags & PIPE_CONTEXT_LOSE_CONTEXT_ON_RESET) {
      st->ctx->Const.ResetStrategy = GL_LOSE_CONTEXT_ON_RESET_ARB;
               if (attribs->flags & ST_CONTEXT_FLAG_RELEASE_NONE)
            /* need to perform version check */
   if (attribs->major > 1 || attribs->minor > 0) {
      /* Is the actual version less than the requested version?
   */
   if (st->ctx->Version < attribs->major * 10U + attribs->minor) {
      *error = ST_CONTEXT_ERROR_BAD_VERSION;
   st_destroy_context(st);
                           st->ctx->invalidate_on_gl_viewport =
                     if (st->ctx->IntelBlackholeRender &&
      st->screen->get_param(st->screen, PIPE_CAP_FRONTEND_NOOP))
         *error = ST_CONTEXT_SUCCESS;
      }
         /**
   * Get the currently bound context in the calling thread.
   */
   struct st_context *
   st_api_get_current(void)
   {
                  }
         static struct gl_framebuffer *
   st_framebuffer_reuse_or_create(struct st_context *st,
         {
               if (!drawable)
            /* Check if there is already a framebuffer object for the specified
   * framebuffer interface in this context. If there is one, use it.
   */
   LIST_FOR_EACH_ENTRY(cur, &st->winsys_buffers, head) {
      if (cur->drawable_ID == drawable->ID) {
      _mesa_reference_framebuffer(&stfb, cur);
                  /* If there is not already a framebuffer object, create one */
   if (stfb == NULL) {
               if (cur) {
      /* add the referenced framebuffer interface object to
   * the framebuffer interface object hash table.
   */
   if (!drawable_insert(drawable->fscreen, drawable)) {
      _mesa_reference_framebuffer(&cur, NULL);
                                                }
         /**
   * Bind the context to the calling thread with draw and read as drawables.
   *
   * The framebuffers might be NULL, meaning the context is surfaceless.
   */
   bool
   st_api_make_current(struct st_context *st,
               {
      struct gl_framebuffer *stdraw, *stread;
            if (st) {
      /* reuse or create the draw fb */
   stdraw = st_framebuffer_reuse_or_create(st, stdrawi);
   if (streadi != stdrawi) {
      /* do the same for the read fb */
      }
   else {
      stread = NULL;
   /* reuse the draw fb for the read fb */
   if (stdraw)
               /* If framebuffers were asked for, we'd better have allocated them */
   if ((stdrawi && !stdraw) || (streadi && !stread))
            if (stdraw && stread) {
      st_framebuffer_validate(stdraw, st);
                           st->draw_stamp = stdraw->stamp - 1;
   st->read_stamp = stread->stamp - 1;
      }
   else {
      struct gl_framebuffer *incomplete = _mesa_get_incomplete_framebuffer();
               _mesa_reference_framebuffer(&stdraw, NULL);
            /* Purge the context's winsys_buffers list in case any
   * of the referenced drawables no longer exist.
   */
      }
   else {
               if (ctx) {
      /* Before releasing the context, release its associated
   * winsys buffers first. Then purge the context's winsys buffers list
   * to free the resources of any winsys buffers that no longer have
   * an existing drawable.
   */
   ret = _mesa_make_current(ctx, NULL, NULL);
                              }
         /**
   * Flush the front buffer if the current context renders to the front buffer.
   */
   void
   st_manager_flush_frontbuffer(struct st_context *st)
   {
      struct gl_framebuffer *stfb = st_ws_framebuffer(st->ctx->DrawBuffer);
            if (!stfb)
            /* If the context uses a doublebuffered visual, but the buffer is
   * single-buffered, guess that it's a pbuffer, which doesn't need
   * flushing.
   */
   if (st->ctx->Visual.doubleBufferMode &&
      !stfb->Visual.doubleBufferMode)
         /* Check front buffer used at the GL API level. */
   enum st_attachment_type statt = ST_ATTACHMENT_FRONT_LEFT;
   rb = stfb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
   if (!rb) {
      /* Check back buffer redirected by EGL_KHR_mutable_render_buffer. */
   statt = ST_ATTACHMENT_BACK_LEFT;
               /* Do we have a front color buffer and has it been drawn to since last
   * frontbuffer flush?
   */
   if (rb && rb->defined &&
      stfb->drawable->flush_front(st, stfb->drawable, statt)) {
            /* Trigger an update of rb->defined on next draw */
         }
         /**
   * Re-validate the framebuffers.
   */
   void
   st_manager_validate_framebuffers(struct st_context *st)
   {
      struct gl_framebuffer *stdraw = st_ws_framebuffer(st->ctx->DrawBuffer);
            if (stdraw)
         if (stread && stread != stdraw)
               }
         /**
   * Flush any outstanding swapbuffers on the current draw framebuffer.
   */
   void
   st_manager_flush_swapbuffers(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct st_context *st = (ctx) ? ctx->st : NULL;
            if (!st)
            stfb = st_ws_framebuffer(ctx->DrawBuffer);
   if (!stfb || !stfb->drawable->flush_swapbuffers)
               }
         /**
   * Add a color renderbuffer on demand.  The FBO must correspond to a window,
   * not a user-created FBO.
   */
   bool
   st_manager_add_color_renderbuffer(struct gl_context *ctx,
               {
               /* FBO */
   if (!stfb)
                     if (stfb->Attachment[idx].Renderbuffer)
            switch (idx) {
   case BUFFER_FRONT_LEFT:
   case BUFFER_BACK_LEFT:
   case BUFFER_FRONT_RIGHT:
   case BUFFER_BACK_RIGHT:
         default:
                  if (!st_framebuffer_add_renderbuffer(stfb, idx,
                           /*
   * Force a call to the frontend manager to validate the
   * new renderbuffer. It might be that there is a window system
   * renderbuffer available.
   */
   if (stfb->drawable)
                        }
         static unsigned
   get_version(struct pipe_screen *screen,
         {
      struct gl_constants consts = {0};
   struct gl_extensions extensions = {0};
            if (_mesa_override_gl_version_contextless(&consts, &api, &version)) {
                  _mesa_init_constants(&consts, api);
            st_init_limits(screen, &consts, &extensions, api);
   st_init_extensions(screen, &consts, &extensions, options, api);
   version = _mesa_get_version(&extensions, &consts, api);
   free(consts.SpirVExtensions);
      }
         /**
   * Query supported OpenGL versions. (if applicable)
   * The format is (major*10+minor).
   */
   void
   st_api_query_versions(struct pipe_frontend_screen *fscreen,
                        struct st_config_options *options,
      {
      *gl_core_version = get_version(fscreen->screen, options, API_OPENGL_CORE);
   *gl_compat_version = get_version(fscreen->screen, options, API_OPENGL_COMPAT);
   *gl_es1_version = get_version(fscreen->screen, options, API_OPENGLES);
      }
         void
   st_manager_invalidate_drawables(struct gl_context *ctx)
   {
      struct gl_framebuffer *stdraw;
            /*
   * Normally we'd want the frontend manager to mark the drawables
   * invalid only when needed. This will force the frontend manager
   * to revalidate the drawable, rather than just update the context with
   * the latest cached drawable info.
            stdraw = st_ws_framebuffer(ctx->DrawBuffer);
            if (stdraw)
         if (stread && stread != stdraw)
      }
