   /*
   * Copyright 2016 VMware, Inc.
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
   */
      #include "pipe/p_context.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
      #include "main/context.h"
   #include "main/macros.h"
   #include "main/mtypes.h"
   #include "main/teximage.h"
   #include "main/texobj.h"
   #include "program/prog_instruction.h"
      #include "st_context.h"
   #include "st_sampler_view.h"
   #include "st_texture.h"
   #include "st_format.h"
   #include "st_cb_texture.h"
      /* Subtract remaining private references. Typically used before
   * destruction. See the header file for explanation.
   */
   static void
   st_remove_private_references(struct st_sampler_view *sv)
   {
      if (sv->private_refcount) {
      assert(sv->private_refcount > 0);
   p_atomic_add(&sv->view->reference.count, -sv->private_refcount);
         }
      /* Return a sampler view while incrementing the refcount by 1. */
   static struct pipe_sampler_view *
   get_sampler_view_reference(struct st_sampler_view *sv,
         {
      if (unlikely(sv->private_refcount <= 0)) {
               /* This is the number of atomic increments we will skip. */
   sv->private_refcount = 100000000;
               /* Return a reference while decrementing the private refcount. */
   sv->private_refcount--;
      }
      /**
   * Set the given view as the current context's view for the texture.
   *
   * Overwrites any pre-existing view of the context.
   *
   * Takes ownership of the view (i.e., stores the view without incrementing the
   * reference count).
   *
   * \return the view, or NULL on error. In case of error, the reference to the
   * view is released.
   */
   static struct pipe_sampler_view *
   st_texture_set_sampler_view(struct st_context *st,
                           {
      struct st_sampler_views *views;
   struct st_sampler_view *free = NULL;
   struct st_sampler_view *sv;
            if (!locked)
                  for (i = 0; i < views->count; ++i) {
               /* Is the array entry used ? */
   if (sv->view) {
      /* check if the context matches */
   if (sv->view->context == st->pipe) {
      st_remove_private_references(sv);
   pipe_sampler_view_reference(&sv->view, NULL);
         } else {
      /* Found a free slot, remember that */
                  /* Couldn't find a slot for our context, create a new one */
   if (free) {
         } else {
      if (views->count >= views->max) {
      /* Allocate a larger container. */
                  if (new_max < views->max ||
      new_max > (UINT_MAX - sizeof(*views)) / sizeof(views->views[0])) {
   pipe_sampler_view_reference(&view, NULL);
               struct st_sampler_views *new_views = malloc(new_size);
   if (!new_views) {
      pipe_sampler_view_reference(&view, NULL);
               new_views->count = views->count;
   new_views->max = new_max;
                  /* Initialize the pipe_sampler_view pointers to zero so that we don't
   * have to worry about racing against readers when incrementing
   * views->count.
   */
                  /* Use memory release semantics to ensure that concurrent readers will
   * get the correct contents of the new container.
   *
   * Also, the write should be atomic, but that's guaranteed anyway on
   * all supported platforms.
                  /* We keep the old container around until the texture object is
   * deleted, because another thread may still be reading from it. We
   * double the size of the container each time, so we end up with
   * at most twice the total memory allocation.
   */
                                       /* Since modification is guarded by the lock, only the write part of the
   * increment has to be atomic, and that's already guaranteed on all
   * supported platforms without using an atomic intrinsic.
   */
            found:
               sv->glsl130_or_later = glsl130_or_later;
   sv->srgb_skip_decode = srgb_skip_decode;
   sv->view = view;
            if (get_reference)
         out:
      if (!locked)
            }
         /**
   * Return the most-recently validated sampler view for the texture \p stObj
   * in the given context, if any.
   *
   * Performs no additional validation.
   */
   struct st_sampler_view *
   st_texture_get_current_sampler_view(const struct st_context *st,
         {
               for (unsigned i = 0; i < views->count; ++i) {
      struct st_sampler_view *sv = &views->views[i];
   if (sv->view && sv->view->context == st->pipe)
                  }
         /**
   * For the given texture object, release any sampler views which belong
   * to the calling context.  This is used to free any sampler views
   * which belong to the context before the context is destroyed.
   */
   void
   st_texture_release_context_sampler_view(struct st_context *st,
         {
               simple_mtx_lock(&stObj->validate_mutex);
   struct st_sampler_views *views = stObj->sampler_views;
   for (i = 0; i < views->count; ++i) {
               if (sv->view && sv->view->context == st->pipe) {
      st_remove_private_references(sv);
   pipe_sampler_view_reference(&sv->view, NULL);
         }
      }
         /**
   * Release all sampler views attached to the given texture object, regardless
   * of the context.  This is called fairly frequently.  For example, whenever
   * the texture's base level, max level or swizzle change.
   */
   void
   st_texture_release_all_sampler_views(struct st_context *st,
         {
      /* TODO: This happens while a texture is deleted, because the Driver API
   * is asymmetric: the driver allocates the texture object memory, but
   * mesa/main frees it.
   */
   if (!stObj->sampler_views)
            simple_mtx_lock(&stObj->validate_mutex);
   struct st_sampler_views *views = stObj->sampler_views;
   for (unsigned i = 0; i < views->count; ++i) {
      struct st_sampler_view *stsv = &views->views[i];
   if (stsv->view) {
               if (stsv->st && stsv->st != st) {
      /* Transfer this reference to the zombie list.  It will
   * likely be freed when the zombie list is freed.
   */
   st_save_zombie_sampler_view(stsv->st, stsv->view);
      } else {
               }
   views->count = 0;
      }
         /*
   * Delete the texture's sampler views and st_sampler_views containers.
   * This is to be called just before a texture is deleted.
   */
   void
   st_delete_texture_sampler_views(struct st_context *st,
         {
               /* Free the container of the current per-context sampler views */
   assert(stObj->sampler_views->count == 0);
   free(stObj->sampler_views);
            /* Free old sampler view containers */
   while (stObj->sampler_views_old) {
      struct st_sampler_views *views = stObj->sampler_views_old;
   stObj->sampler_views_old = views->next;
         }
      /**
   * Return TRUE if the texture's sampler view swizzle is not equal to
   * the texture's swizzle.
   *
   * \param texObj  the st texture object,
   */
   ASSERTED static bool
   check_sampler_swizzle(const struct st_context *st,
                     {
               return ((sv->swizzle_r != GET_SWZ(swizzle, 0)) ||
         (sv->swizzle_g != GET_SWZ(swizzle, 1)) ||
      }
         static unsigned
   last_level(const struct gl_texture_object *texObj)
   {
      unsigned ret = MIN2(texObj->Attrib.MinLevel + texObj->_MaxLevel,
         if (texObj->Immutable)
      ret = MIN2(ret, texObj->Attrib.MinLevel +
         }
         static unsigned
   last_layer(const struct gl_texture_object *texObj)
   {
      if (texObj->Immutable && texObj->pt->array_size > 1)
      return MIN2(texObj->Attrib.MinLayer +
               }
         /**
   * Determine the format for the texture sampler view.
   */
   enum pipe_format
   st_get_sampler_view_format(const struct st_context *st,
               {
               GLenum baseFormat = _mesa_base_tex_image(texObj)->_BaseFormat;
            /* From OpenGL 4.3 spec, "Combined Depth/Stencil Textures":
   *
   *    "The DEPTH_STENCIL_TEXTURE_MODE is ignored for non
   *     depth/stencil textures.
   */
   const bool has_combined_ds =
            if (baseFormat == GL_DEPTH_COMPONENT ||
      baseFormat == GL_DEPTH_STENCIL ||
   baseFormat == GL_STENCIL_INDEX) {
   if ((texObj->StencilSampling && has_combined_ds) ||
                              /* If sRGB decoding is off, use the linear format */
   if (srgb_skip_decode)
            /* if resource format matches then YUV wasn't lowered */
   if (format == texObj->pt->format)
            /* Use R8_UNORM for video formats */
   switch (format) {
   case PIPE_FORMAT_NV12:
      if (texObj->pt->format == PIPE_FORMAT_R8_G8B8_420_UNORM) {
      format = PIPE_FORMAT_R8_G8B8_420_UNORM;
      }
      case PIPE_FORMAT_NV21:
      if (texObj->pt->format == PIPE_FORMAT_R8_B8G8_420_UNORM) {
      format = PIPE_FORMAT_R8_B8G8_420_UNORM;
      }
      case PIPE_FORMAT_IYUV:
      if (texObj->pt->format == PIPE_FORMAT_R8_G8_B8_420_UNORM ||
      texObj->pt->format == PIPE_FORMAT_R8_B8_G8_420_UNORM) {
   format = texObj->pt->format;
      }
   format = PIPE_FORMAT_R8_UNORM;
      case PIPE_FORMAT_P010:
   case PIPE_FORMAT_P012:
   case PIPE_FORMAT_P016:
   case PIPE_FORMAT_P030:
      format = PIPE_FORMAT_R16_UNORM;
      case PIPE_FORMAT_Y210:
   case PIPE_FORMAT_Y212:
   case PIPE_FORMAT_Y216:
      format = PIPE_FORMAT_R16G16_UNORM;
      case PIPE_FORMAT_Y410:
      format = PIPE_FORMAT_R10G10B10A2_UNORM;
      case PIPE_FORMAT_Y412:
   case PIPE_FORMAT_Y416:
      format = PIPE_FORMAT_R16G16B16A16_UNORM;
      case PIPE_FORMAT_YUYV:
   case PIPE_FORMAT_YVYU:
   case PIPE_FORMAT_UYVY:
   case PIPE_FORMAT_VYUY:
      if (texObj->pt->format == PIPE_FORMAT_R8G8_R8B8_UNORM ||
      texObj->pt->format == PIPE_FORMAT_R8B8_R8G8_UNORM ||
   texObj->pt->format == PIPE_FORMAT_B8R8_G8R8_UNORM ||
   texObj->pt->format == PIPE_FORMAT_G8R8_B8R8_UNORM) {
   format = texObj->pt->format;
      }
   format = PIPE_FORMAT_R8G8_UNORM;
      case PIPE_FORMAT_AYUV:
      format = PIPE_FORMAT_RGBA8888_UNORM;
      case PIPE_FORMAT_XYUV:
      format = PIPE_FORMAT_RGBX8888_UNORM;
      default:
         }
      }
         static struct pipe_sampler_view *
   st_create_texture_sampler_view_from_stobj(struct st_context *st,
                     {
      /* There is no need to clear this structure (consider CPU overhead). */
   struct pipe_sampler_view templ;
            templ.format = format;
            if (texObj->level_override >= 0) {
         } else {
      templ.u.tex.first_level = texObj->Attrib.MinLevel +
            }
   if (texObj->layer_override >= 0) {
         } else {
      templ.u.tex.first_layer = texObj->Attrib.MinLayer;
      }
   assert(templ.u.tex.first_layer <= templ.u.tex.last_layer);
   assert(templ.u.tex.first_level <= templ.u.tex.last_level);
            templ.swizzle_r = GET_SWZ(swizzle, 0);
   templ.swizzle_g = GET_SWZ(swizzle, 1);
   templ.swizzle_b = GET_SWZ(swizzle, 2);
               }
      struct pipe_sampler_view *
   st_get_texture_sampler_view_from_stobj(struct st_context *st,
                                 {
      struct st_sampler_view *sv;
            if (!ignore_srgb_decode && samp->Attrib.sRGBDecode == GL_SKIP_DECODE_EXT)
            simple_mtx_lock(&texObj->validate_mutex);
            if (sv &&
      sv->glsl130_or_later == glsl130_or_later &&
   sv->srgb_skip_decode == srgb_skip_decode) {
   /* Debug check: make sure that the sampler view's parameters are
   * what they're supposed to be.
   */
   struct pipe_sampler_view *view = sv->view;
   assert(texObj->pt == view->texture);
   assert(!check_sampler_swizzle(st, texObj, view, glsl130_or_later));
   assert(st_get_sampler_view_format(st, texObj, srgb_skip_decode) == view->format);
   assert(gl_target_to_pipe(texObj->Target) == view->target);
   assert(texObj->level_override >= 0 ||
         texObj->Attrib.MinLevel +
   assert(texObj->level_override >= 0 || last_level(texObj) == view->u.tex.last_level);
   assert(texObj->layer_override >= 0 ||
         assert(texObj->layer_override >= 0 || last_layer(texObj) == view->u.tex.last_layer);
   assert(texObj->layer_override < 0 ||
         (texObj->layer_override == view->u.tex.first_layer &&
   if (get_reference)
         simple_mtx_unlock(&texObj->validate_mutex);
               /* create new sampler view */
   enum pipe_format format = st_get_sampler_view_format(st, texObj,
         struct pipe_sampler_view *view =
                  view = st_texture_set_sampler_view(st, texObj, view,
                           }
         struct pipe_sampler_view *
   st_get_buffer_sampler_view_from_stobj(struct st_context *st,
               {
      struct st_sampler_view *sv;
   struct gl_buffer_object *stBuf =
            if (!stBuf || !stBuf->buffer)
                              if (sv) {
               if (view->texture == buf) {
      /* Debug check: make sure that the sampler view's parameters are
   * what they're supposed to be.
   */
   assert(st_mesa_format_to_pipe_format(st,
               assert(view->target == PIPE_BUFFER);
   ASSERTED unsigned base = texObj->BufferOffset;
   ASSERTED unsigned size = MIN2(buf->width0 - base,
         assert(view->u.buf.offset == base);
   assert(view->u.buf.size == size);
   if (get_reference)
                                 if (base >= buf->width0)
            unsigned size = buf->width0 - base;
   size = MIN2(size, (unsigned)texObj->BufferSize);
   if (!size)
            /* Create a new sampler view. There is no need to clear the entire
   * structure (consider CPU overhead).
   */
            templ.is_tex2d_from_buf = false;
   templ.format =
         templ.target = PIPE_BUFFER;
   templ.swizzle_r = PIPE_SWIZZLE_X;
   templ.swizzle_g = PIPE_SWIZZLE_Y;
   templ.swizzle_b = PIPE_SWIZZLE_Z;
   templ.swizzle_a = PIPE_SWIZZLE_W;
   templ.u.buf.offset = base;
            struct pipe_sampler_view *view =
            view = st_texture_set_sampler_view(st, texObj, view, false, false,
               }
