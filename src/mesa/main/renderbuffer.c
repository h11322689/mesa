   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
         #include "util/glheader.h"
      #include "context.h"
   #include "bufferobj.h"
   #include "fbobject.h"
   #include "formats.h"
   #include "glformats.h"
   #include "mtypes.h"
   #include "renderbuffer.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
      #include "state_tracker/st_context.h"
   #include "state_tracker/st_format.h"
      /**
   * Called by FBO code to choose a PIPE_FORMAT_ for drawing surfaces.
   */
   static enum pipe_format
   choose_renderbuffer_format(struct gl_context *ctx,
               {
      unsigned bindings;
   if (_mesa_is_depth_or_stencil_format(internalFormat))
         else
         return st_choose_format(st_context(ctx), internalFormat, GL_NONE, GL_NONE,
                  }
            /**
   * Delete a gl_framebuffer.
   * This is the default function for renderbuffer->Delete().
   * Drivers which subclass gl_renderbuffer should probably implement their
   * own delete function.  But the driver might also call this function to
   * free the object in the end.
   */
   static void
   delete_renderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
   {
      if (ctx) {
      pipe_surface_release(ctx->pipe, &rb->surface_srgb);
      } else {
      pipe_surface_release_no_context(&rb->surface_srgb);
      }
   rb->surface = NULL;
   pipe_resource_reference(&rb->texture, NULL);
   free(rb->data);
   free(rb->Label);
      }
      static GLboolean
   renderbuffer_alloc_sw_storage(struct gl_context *ctx,
                     {
      enum pipe_format format;
            free(rb->data);
            if (internalFormat == GL_RGBA16_SNORM) {
      /* Special case for software accum buffers.  Otherwise, if the
   * call to choose_renderbuffer_format() fails (because the
   * driver doesn't support signed 16-bit/channel colors) we'd
   * just return without allocating the software accum buffer.
   */
      }
   else {
               /* Not setting gl_renderbuffer::Format here will cause
   * FRAMEBUFFER_UNSUPPORTED and ValidateFramebuffer will not be called.
   */
   if (format == PIPE_FORMAT_NONE) {
                              size = _mesa_format_image_size(rb->Format, width, height, 1);
   rb->data = malloc(size);
      }
         /**
   * gl_renderbuffer::AllocStorage()
   * This is called to allocate the original drawing surface, and
   * during window resize.
   */
   static GLboolean
   renderbuffer_alloc_storage(struct gl_context * ctx,
                     {
      struct st_context *st = st_context(ctx);
   struct pipe_screen *screen = ctx->screen;
   enum pipe_format format = PIPE_FORMAT_NONE;
            /* init renderbuffer fields */
   rb->Width  = width;
   rb->Height = height;
   rb->_BaseFormat = _mesa_base_fbo_format(ctx, internalFormat);
            if (rb->software) {
      return renderbuffer_alloc_sw_storage(ctx, rb, internalFormat,
               /* Free the old surface and texture
   */
   pipe_surface_reference(&rb->surface_srgb, NULL);
   pipe_surface_reference(&rb->surface_linear, NULL);
   rb->surface = NULL;
            /* If an sRGB framebuffer is unsupported, sRGB formats behave like linear
   * formats.
   */
   if (!ctx->Extensions.EXT_sRGB) {
                  /* Handle multisample renderbuffers first.
   *
   * From ARB_framebuffer_object:
   *   If <samples> is zero, then RENDERBUFFER_SAMPLES is set to zero.
   *   Otherwise <samples> represents a request for a desired minimum
   *   number of samples. Since different implementations may support
   *   different sample counts for multisampled rendering, the actual
   *   number of samples allocated for the renderbuffer image is
   *   implementation dependent.  However, the resulting value for
   *   RENDERBUFFER_SAMPLES is guaranteed to be greater than or equal
   *   to <samples> and no more than the next larger sample count supported
   *   by the implementation.
   *
   * Find the supported number of samples >= rb->NumSamples
   */
   if (rb->NumSamples > 0) {
               if (ctx->Const.MaxSamples > 1 &&  rb->NumSamples == 1) {
      /* don't try num_samples = 1 with drivers that support real msaa */
   start = 2;
      } else {
      start = rb->NumSamples;
               if (ctx->Extensions.AMD_framebuffer_multisample_advanced) {
      if (rb->_BaseFormat == GL_DEPTH_COMPONENT ||
      rb->_BaseFormat == GL_DEPTH_STENCIL ||
   rb->_BaseFormat == GL_STENCIL_INDEX) {
   /* Find a supported depth-stencil format. */
   for (unsigned samples = start;
      samples <= ctx->Const.MaxDepthStencilFramebufferSamples;
                        if (format != PIPE_FORMAT_NONE) {
      rb->NumSamples = samples;
   rb->NumStorageSamples = samples;
            } else {
      /* Find a supported color format, samples >= storage_samples. */
   for (unsigned storage_samples = start_storage;
      storage_samples <= ctx->Const.MaxColorFramebufferStorageSamples;
   storage_samples++) {
   for (unsigned samples = MAX2(start, storage_samples);
      samples <= ctx->Const.MaxColorFramebufferSamples;
   samples++) {
                        if (format != PIPE_FORMAT_NONE) {
      rb->NumSamples = samples;
   rb->NumStorageSamples = storage_samples;
            }
         } else {
      for (unsigned samples = start; samples <= ctx->Const.MaxSamples;
      samples++) {
                  if (format != PIPE_FORMAT_NONE) {
      rb->NumSamples = samples;
   rb->NumStorageSamples = samples;
               } else {
                  /* Not setting gl_renderbuffer::Format here will cause
   * FRAMEBUFFER_UNSUPPORTED and ValidateFramebuffer will not be called.
   */
   if (format == PIPE_FORMAT_NONE) {
                           if (width == 0 || height == 0) {
      /* if size is zero, nothing to allocate */
               /* Setup new texture template.
   */
   memset(&templ, 0, sizeof(templ));
   templ.target = st->internal_target;
   templ.format = format;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.nr_samples = rb->NumSamples;
            if (util_format_is_depth_or_stencil(format)) {
         }
   else if (rb->Name != 0) {
      /* this is a user-created renderbuffer */
      }
   else {
      /* this is a window-system buffer */
   templ.bind = (PIPE_BIND_DISPLAY_TARGET |
                        if (!rb->texture)
            _mesa_update_renderbuffer_surface(ctx, rb);
      }
      /**
   * Initialize the fields of a gl_renderbuffer to default values.
   */
   void
   _mesa_init_renderbuffer(struct gl_renderbuffer *rb, GLuint name)
   {
               rb->ClassID = 0;
   rb->Name = name;
   rb->RefCount = 1;
            /* The rest of these should be set later by the caller of this function or
   * the AllocStorage method:
   */
            rb->Width = 0;
   rb->Height = 0;
            /* In GL 3, the initial format is GL_RGBA according to Table 6.26
   * on page 302 of the GL 3.3 spec.
   *
   * In GLES 3, the initial format is GL_RGBA4 according to Table 6.15
   * on page 258 of the GLES 3.0.4 spec.
   *
   * If the context is current, set the initial format based on the
   * specs. If the context is not current, we cannot determine the
   * API, so default to GL_RGBA.
   */
   if (ctx && _mesa_is_gles(ctx)) {
         } else {
                              }
      static void
   validate_and_init_renderbuffer_attachment(struct gl_framebuffer *fb,
               {
      assert(fb);
   assert(rb);
            /* There should be no previous renderbuffer on this attachment point,
   * with the exception of depth/stencil since the same renderbuffer may
   * be used for both.
   */
   assert(bufferName == BUFFER_DEPTH ||
                  /* winsys vs. user-created buffer cross check */
   if (_mesa_is_user_fbo(fb)) {
         }
   else {
                  fb->Attachment[bufferName].Type = GL_RENDERBUFFER_EXT;
      }
         /**
   * Attach a renderbuffer to a framebuffer.
   * \param bufferName  one of the BUFFER_x tokens
   *
   * This function avoids adding a reference and is therefore intended to be
   * used with a freshly created renderbuffer.
   */
   void
   _mesa_attach_and_own_rb(struct gl_framebuffer *fb,
               {
                        _mesa_reference_renderbuffer(&fb->Attachment[bufferName].Renderbuffer,
            }
      /**
   * Attach a renderbuffer to a framebuffer.
   * \param bufferName  one of the BUFFER_x tokens
   */
   void
   _mesa_attach_and_reference_rb(struct gl_framebuffer *fb,
               {
      validate_and_init_renderbuffer_attachment(fb, bufferName, rb);
      }
         /**
   * Remove the named renderbuffer from the given framebuffer.
   * \param bufferName  one of the BUFFER_x tokens
   */
   void
   _mesa_remove_renderbuffer(struct gl_framebuffer *fb,
         {
      assert(bufferName < BUFFER_COUNT);
   _mesa_reference_renderbuffer(&fb->Attachment[bufferName].Renderbuffer,
      }
         /**
   * Set *ptr to point to rb.  If *ptr points to another renderbuffer,
   * dereference that buffer first.  The new renderbuffer's refcount will
   * be incremented.  The old renderbuffer's refcount will be decremented.
   * This is normally only called from the _mesa_reference_renderbuffer() macro
   * when there's a real pointer change.
   */
   void
   _mesa_reference_renderbuffer_(struct gl_renderbuffer **ptr,
         {
      if (*ptr) {
      /* Unreference the old renderbuffer */
                     if (p_atomic_dec_zero(&oldRb->RefCount)) {
      GET_CURRENT_CONTEXT(ctx);
                  if (rb) {
      /* reference new renderbuffer */
                  }
      void
   _mesa_map_renderbuffer(struct gl_context *ctx,
                        struct gl_renderbuffer *rb,
      {
      struct pipe_context *pipe = ctx->pipe;
   const GLboolean invert = flip_y;
   GLuint y2;
            if (rb->software) {
      /* software-allocated renderbuffer (probably an accum buffer) */
   if (rb->data) {
      GLint bpp = _mesa_get_format_bytes(rb->Format);
   GLint stride = _mesa_format_row_stride(rb->Format,
         *mapOut = (GLubyte *) rb->data + y * stride + x * bpp;
      }
   else {
      *mapOut = NULL;
      }
               /* Check for unexpected flags */
   assert((mode & ~(GL_MAP_READ_BIT |
                  const enum pipe_map_flags transfer_flags =
            /* Note: y=0=bottom of buffer while y2=0=top of buffer.
   * 'invert' will be true for window-system buffers and false for
   * user-allocated renderbuffers and textures.
   */
   if (invert)
         else
            map = pipe_texture_map(pipe,
                           if (map) {
      if (invert) {
      *rowStrideOut = -(int) rb->transfer->stride;
      }
   else {
         }
      }
   else {
      *mapOut = NULL;
         }
      void
   _mesa_unmap_renderbuffer(struct gl_context *ctx,
         {
               if (rb->software) {
      /* software-allocated renderbuffer (probably an accum buffer) */
               pipe_texture_unmap(pipe, rb->transfer);
      }
      void
   _mesa_regen_renderbuffer_surface(struct gl_context *ctx,
         {
      struct pipe_context *pipe = ctx->pipe;
            struct pipe_surface **psurf =
         struct pipe_surface *surf = *psurf;
   /* create a new pipe_surface */
   struct pipe_surface surf_tmpl;
   memset(&surf_tmpl, 0, sizeof(surf_tmpl));
   surf_tmpl.format = surf->format;
   surf_tmpl.nr_samples = rb->rtt_nr_samples;
   surf_tmpl.u.tex.level = surf->u.tex.level;
   surf_tmpl.u.tex.first_layer = surf->u.tex.first_layer;
            /* create -> destroy to avoid blowing up cached surfaces */
   surf = pipe->create_surface(pipe, resource, &surf_tmpl);
   pipe_surface_release(pipe, psurf);
               }
      /**
   * Create or update the pipe_surface of a FBO renderbuffer.
   * This is usually called after st_finalize_texture.
   */
   void
   _mesa_update_renderbuffer_surface(struct gl_context *ctx,
         {
      struct pipe_context *pipe = ctx->pipe;
   struct pipe_resource *resource = rb->texture;
   const struct gl_texture_object *stTexObj = NULL;
   unsigned rtt_width = rb->Width;
   unsigned rtt_height = rb->Height;
            /*
   * For winsys fbo, it is possible that the renderbuffer is sRGB-capable but
   * the format of rb->texture is linear (because we have no control over
   * the format).  Check rb->Format instead of rb->texture->format
   * to determine if the rb is sRGB-capable.
   */
   bool enable_srgb = ctx->Color.sRGBEnabled &&
                  if (rb->is_rtt) {
      stTexObj = rb->TexImage->TexObject;
   if (stTexObj->surface_based)
                        if (resource->target == PIPE_TEXTURE_1D_ARRAY) {
      rtt_depth = rtt_height;
               /* find matching mipmap level size */
   unsigned level;
   for (level = 0; level <= resource->last_level; level++) {
      if (u_minify(resource->width0, level) == rtt_width &&
      u_minify(resource->height0, level) == rtt_height &&
   (resource->target != PIPE_TEXTURE_3D ||
   u_minify(resource->depth0, level) == rtt_depth)) {
         }
            /* determine the layer bounds */
   unsigned first_layer, last_layer;
   if (rb->rtt_layered) {
      first_layer = 0;
      }
   else {
      first_layer =
               /* Adjust for texture views */
   if (rb->is_rtt && resource->array_size > 1 &&
      stTexObj->Immutable) {
   const struct gl_texture_object *tex = stTexObj;
   first_layer += tex->Attrib.MinLayer;
   if (!rb->rtt_layered)
         else
      last_layer = MIN2(first_layer + tex->Attrib.NumLayers - 1,
            struct pipe_surface **psurf =
                  if (!surf ||
      surf->texture->nr_samples != rb->NumSamples ||
   surf->texture->nr_storage_samples != rb->NumStorageSamples ||
   surf->format != format ||
   surf->texture != resource ||
   surf->width != rtt_width ||
   surf->height != rtt_height ||
   surf->nr_samples != rb->rtt_nr_samples ||
   surf->u.tex.level != level ||
   surf->u.tex.first_layer != first_layer ||
   surf->u.tex.last_layer != last_layer) {
   /* create a new pipe_surface */
   struct pipe_surface surf_tmpl;
   memset(&surf_tmpl, 0, sizeof(surf_tmpl));
   surf_tmpl.format = format;
   surf_tmpl.nr_samples = rb->rtt_nr_samples;
   surf_tmpl.u.tex.level = level;
   surf_tmpl.u.tex.first_layer = first_layer;
            /* create -> destroy to avoid blowing up cached surfaces */
   struct pipe_surface *surf = pipe->create_surface(pipe, resource, &surf_tmpl);
   pipe_surface_release(pipe, psurf);
      }
      }
