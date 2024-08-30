   /*
   * Copyright © 2017 Valve Corporation.
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
      #include "util/glheader.h"
   #include "context.h"
   #include "enums.h"
      #include "hash.h"
   #include "mtypes.h"
   #include "shaderimage.h"
   #include "teximage.h"
   #include "texobj.h"
   #include "texturebindless.h"
      #include "util/hash_table.h"
   #include "util/u_memory.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_context.h"
   #include "state_tracker/st_cb_texture.h"
   #include "state_tracker/st_texture.h"
   #include "state_tracker/st_sampler_view.h"
      /**
   * Return the gl_texture_handle_object for a given 64-bit handle.
   */
   static struct gl_texture_handle_object *
   lookup_texture_handle(struct gl_context *ctx, GLuint64 id)
   {
               mtx_lock(&ctx->Shared->HandlesMutex);
   texHandleObj = (struct gl_texture_handle_object *)
                     }
      /**
   * Return the gl_image_handle_object for a given 64-bit handle.
   */
   static struct gl_image_handle_object *
   lookup_image_handle(struct gl_context *ctx, GLuint64 id)
   {
               mtx_lock(&ctx->Shared->HandlesMutex);
   imgHandleObj = (struct gl_image_handle_object *)
                     }
      /**
   * Delete a texture handle in the shared state.
   */
   static void
   delete_texture_handle(struct gl_context *ctx, GLuint64 id)
   {
      mtx_lock(&ctx->Shared->HandlesMutex);
   _mesa_hash_table_u64_remove(ctx->Shared->TextureHandles, id);
               }
      /**
   * Delete an image handle in the shared state.
   */
   static void
   delete_image_handle(struct gl_context *ctx, GLuint64 id)
   {
      mtx_lock(&ctx->Shared->HandlesMutex);
   _mesa_hash_table_u64_remove(ctx->Shared->ImageHandles, id);
               }
      /**
   * Return TRUE if the texture handle is resident in the current context.
   */
   static inline bool
   is_texture_handle_resident(struct gl_context *ctx, GLuint64 handle)
   {
      return _mesa_hash_table_u64_search(ctx->ResidentTextureHandles,
      }
      /**
   * Return TRUE if the image handle is resident in the current context.
   */
   static inline bool
   is_image_handle_resident(struct gl_context *ctx, GLuint64 handle)
   {
      return _mesa_hash_table_u64_search(ctx->ResidentImageHandles,
      }
      /**
   * Make a texture handle resident/non-resident in the current context.
   */
   static void
   make_texture_handle_resident(struct gl_context *ctx,
               {
      struct gl_sampler_object *sampObj = NULL;
   struct gl_texture_object *texObj = NULL;
            if (resident) {
               _mesa_hash_table_u64_insert(ctx->ResidentTextureHandles, handle,
                     /* Reference the texture object (and the separate sampler if needed) to
   * be sure it won't be deleted until it is not bound anywhere and there
   * are no handles using the object that are resident in any context.
   */
   _mesa_reference_texobj(&texObj, texHandleObj->texObj);
   if (texHandleObj->sampObj)
      } else {
                                 /* Unreference the texture object but keep the pointer intact, if
   * refcount hits zero, the texture and all handles will be deleted.
   */
   texObj = texHandleObj->texObj;
            /* Unreference the separate sampler object but keep the pointer intact,
   * if refcount hits zero, the sampler and all handles will be deleted.
   */
   if (texHandleObj->sampObj) {
      sampObj = texHandleObj->sampObj;
            }
      /**
   * Make an image handle resident/non-resident in the current context.
   */
   static void
   make_image_handle_resident(struct gl_context *ctx,
               {
      struct gl_texture_object *texObj = NULL;
            if (resident) {
               _mesa_hash_table_u64_insert(ctx->ResidentImageHandles, handle,
                     /* Reference the texture object to be sure it won't be deleted until it
   * is not bound anywhere and there are no handles using the object that
   * are resident in any context.
   */
      } else {
                                 /* Unreference the texture object but keep the pointer intact, if
   * refcount hits zero, the texture and all handles will be deleted.
   */
   texObj = imgHandleObj->imgObj.TexObj;
         }
      static struct gl_texture_handle_object *
   find_texhandleobj(struct gl_texture_object *texObj,
         {
      util_dynarray_foreach(&texObj->SamplerHandles,
            if ((*texHandleObj)->sampObj == sampObj)
      }
      }
      static GLuint64
   new_texture_handle(struct gl_context *ctx, struct gl_texture_object *texObj,
         {
      struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_sampler_view *view;
            if (texObj->Target != GL_TEXTURE_BUFFER) {
      if (!st_finalize_texture(ctx, pipe, texObj, 0))
                     /* TODO: Clarify the interaction of ARB_bindless_texture and EXT_texture_sRGB_decode */
   view = st_get_texture_sampler_view_from_stobj(st, texObj, sampObj, 0,
      } else {
      view = st_get_buffer_sampler_view_from_stobj(st, texObj, false);
                  }
      static GLuint64
   get_texture_handle(struct gl_context *ctx, struct gl_texture_object *texObj,
         {
      bool separate_sampler = &texObj->Sampler != sampObj;
   struct gl_texture_handle_object *texHandleObj;
            /* The ARB_bindless_texture spec says:
   *
   * "The handle for each texture or texture/sampler pair is unique; the same
   *  handle will be returned if GetTextureHandleARB is called multiple times
   *  for the same texture or if GetTextureSamplerHandleARB is called multiple
   *  times for the same texture/sampler pair."
   */
   mtx_lock(&ctx->Shared->HandlesMutex);
   texHandleObj = find_texhandleobj(texObj, separate_sampler ? sampObj : NULL);
   if (texHandleObj) {
      mtx_unlock(&ctx->Shared->HandlesMutex);
               /* Request a new texture handle from the driver. */
   handle = new_texture_handle(ctx, texObj, sampObj);
   if (!handle) {
      mtx_unlock(&ctx->Shared->HandlesMutex);
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexture*HandleARB()");
               texHandleObj = CALLOC_STRUCT(gl_texture_handle_object);
   if (!texHandleObj) {
      mtx_unlock(&ctx->Shared->HandlesMutex);
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexture*HandleARB()");
               /* Store the handle into the texture object. */
   texHandleObj->texObj = texObj;
   texHandleObj->sampObj = separate_sampler ? sampObj : NULL;
   texHandleObj->handle = handle;
   util_dynarray_append(&texObj->SamplerHandles,
            if (separate_sampler) {
      /* Store the handle into the separate sampler if needed. */
   util_dynarray_append(&sampObj->Handles,
               /* When referenced by one or more handles, texture objects are immutable. */
   texObj->HandleAllocated = true;
   if (texObj->Target == GL_TEXTURE_BUFFER)
                  /* Store the handle in the shared state for all contexts. */
   _mesa_hash_table_u64_insert(ctx->Shared->TextureHandles, handle,
                     }
      static struct gl_image_handle_object *
   find_imghandleobj(struct gl_texture_object *texObj, GLint level,
         {
      util_dynarray_foreach(&texObj->ImageHandles,
                     if (u->TexObj == texObj && u->Level == level && u->Layered == layered &&
      u->Layer == layer && u->Format == format)
   }
      }
      static GLuint64
   get_image_handle(struct gl_context *ctx, struct gl_texture_object *texObj,
         {
      struct gl_image_handle_object *imgHandleObj;
   struct gl_image_unit imgObj;
            /* The ARB_bindless_texture spec says:
   *
   * "The handle returned for each combination of <texture>, <level>,
   * <layered>, <layer>, and <format> is unique; the same handle will be
   * returned if GetImageHandleARB is called multiple times with the same
   * parameters."
   */
   mtx_lock(&ctx->Shared->HandlesMutex);
   imgHandleObj = find_imghandleobj(texObj, level, layered, layer, format);
   if (imgHandleObj) {
      mtx_unlock(&ctx->Shared->HandlesMutex);
               imgObj.TexObj = texObj; /* weak reference */
   imgObj.Level = level;
   imgObj.Access = GL_READ_WRITE;
   imgObj.Format = format;
            if (_mesa_tex_target_is_layered(texObj->Target)) {
      imgObj.Layered = layered;
   imgObj.Layer = layer;
      } else {
      imgObj.Layered = GL_FALSE;
   imgObj.Layer = 0;
               /* Request a new image handle from the driver. */
   struct pipe_image_view image;
   st_convert_image(st_context(ctx), &imgObj, &image, 0);
   handle = ctx->pipe->create_image_handle(ctx->pipe, &image);
   if (!handle) {
      mtx_unlock(&ctx->Shared->HandlesMutex);
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetImageHandleARB()");
               imgHandleObj = CALLOC_STRUCT(gl_image_handle_object);
   if (!imgHandleObj) {
      mtx_unlock(&ctx->Shared->HandlesMutex);
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetImageHandleARB()");
               /* Store the handle into the texture object. */
   memcpy(&imgHandleObj->imgObj, &imgObj, sizeof(struct gl_image_unit));
   imgHandleObj->handle = handle;
   util_dynarray_append(&texObj->ImageHandles,
            /* When referenced by one or more handles, texture objects are immutable. */
   texObj->HandleAllocated = true;
   if (texObj->Target == GL_TEXTURE_BUFFER)
                  /* Store the handle in the shared state for all contexts. */
   _mesa_hash_table_u64_insert(ctx->Shared->ImageHandles, handle, imgHandleObj);
               }
      /**
   * Init/free per-context resident handles.
   */
   void
   _mesa_init_resident_handles(struct gl_context *ctx)
   {
      ctx->ResidentTextureHandles = _mesa_hash_table_u64_create(NULL);
      }
      void
   _mesa_free_resident_handles(struct gl_context *ctx)
   {
      _mesa_hash_table_u64_destroy(ctx->ResidentTextureHandles);
      }
      /**
   * Init/free shared allocated handles.
   */
   void
   _mesa_init_shared_handles(struct gl_shared_state *shared)
   {
      shared->TextureHandles = _mesa_hash_table_u64_create(NULL);
   shared->ImageHandles = _mesa_hash_table_u64_create(NULL);
      }
      void
   _mesa_free_shared_handles(struct gl_shared_state *shared)
   {
      if (shared->TextureHandles)
            if (shared->ImageHandles)
               }
      /**
   * Init/free texture/image handles per-texture object.
   */
   void
   _mesa_init_texture_handles(struct gl_texture_object *texObj)
   {
      util_dynarray_init(&texObj->SamplerHandles, NULL);
      }
      void
   _mesa_make_texture_handles_non_resident(struct gl_context *ctx,
         {
               /* Texture handles */
   util_dynarray_foreach(&texObj->SamplerHandles,
            if (is_texture_handle_resident(ctx, (*texHandleObj)->handle))
               /* Image handles */
   util_dynarray_foreach(&texObj->ImageHandles,
            if (is_image_handle_resident(ctx, (*imgHandleObj)->handle))
                  }
      void
   _mesa_delete_texture_handles(struct gl_context *ctx,
         {
      /* Texture handles */
   util_dynarray_foreach(&texObj->SamplerHandles,
                     if (sampObj) {
      /* Delete the handle in the separate sampler object. */
   util_dynarray_delete_unordered(&sampObj->Handles,
            }
   delete_texture_handle(ctx, (*texHandleObj)->handle);
      }
            /* Image handles */
   util_dynarray_foreach(&texObj->ImageHandles,
            delete_image_handle(ctx, (*imgHandleObj)->handle);
      }
      }
      /**
   * Init/free texture handles per-sampler object.
   */
   void
   _mesa_init_sampler_handles(struct gl_sampler_object *sampObj)
   {
         }
      void
   _mesa_delete_sampler_handles(struct gl_context *ctx,
         {
      util_dynarray_foreach(&sampObj->Handles,
                     /* Delete the handle in the texture object. */
   util_dynarray_delete_unordered(&texObj->SamplerHandles,
                  delete_texture_handle(ctx, (*texHandleObj)->handle);
      }
      }
      static GLboolean
   is_sampler_border_color_valid(struct gl_sampler_object *samp)
   {
      static const GLfloat valid_float_border_colors[4][4] = {
      { 0.0, 0.0, 0.0, 0.0 },
   { 0.0, 0.0, 0.0, 1.0 },
   { 1.0, 1.0, 1.0, 0.0 },
      };
   static const GLint valid_integer_border_colors[4][4] = {
      { 0, 0, 0, 0 },
   { 0, 0, 0, 1 },
   { 1, 1, 1, 0 },
      };
            /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_OPERATION is generated if the border color (taken from
   *  the embedded sampler for GetTextureHandleARB or from the <sampler> for
   *  GetTextureSamplerHandleARB) is not one of the following allowed values.
   *  If the texture's base internal format is signed or unsigned integer,
   *  allowed values are (0,0,0,0), (0,0,0,1), (1,1,1,0), and (1,1,1,1). If
   *  the base internal format is not integer, allowed values are
   *  (0.0,0.0,0.0,0.0), (0.0,0.0,0.0,1.0), (1.0,1.0,1.0,0.0), and
   *  (1.0,1.0,1.0,1.0)."
   */
   if (!memcmp(samp->Attrib.state.border_color.f, valid_float_border_colors[0], size) ||
      !memcmp(samp->Attrib.state.border_color.f, valid_float_border_colors[1], size) ||
   !memcmp(samp->Attrib.state.border_color.f, valid_float_border_colors[2], size) ||
   !memcmp(samp->Attrib.state.border_color.f, valid_float_border_colors[3], size))
         if (!memcmp(samp->Attrib.state.border_color.ui, valid_integer_border_colors[0], size) ||
      !memcmp(samp->Attrib.state.border_color.ui, valid_integer_border_colors[1], size) ||
   !memcmp(samp->Attrib.state.border_color.ui, valid_integer_border_colors[2], size) ||
   !memcmp(samp->Attrib.state.border_color.ui, valid_integer_border_colors[3], size))
            }
      GLuint64 GLAPIENTRY
   _mesa_GetTextureHandleARB_no_error(GLuint texture)
   {
                        texObj = _mesa_lookup_texture(ctx, texture);
   if (!_mesa_is_texture_complete(texObj, &texObj->Sampler,
                     }
      GLuint64 GLAPIENTRY
   _mesa_GetTextureHandleARB(GLuint texture)
   {
                        if (!_mesa_has_ARB_bindless_texture(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_VALUE is generated by GetTextureHandleARB or
   *  GetTextureSamplerHandleARB if <texture> is zero or not the name of an
   *  existing texture object."
   */
   if (texture > 0)
            if (!texObj) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetTextureHandleARB(texture)");
               /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_OPERATION is generated by GetTextureHandleARB or
   *  GetTextureSamplerHandleARB if the texture object specified by <texture>
   *  is not complete."
   */
   if (!_mesa_is_texture_complete(texObj, &texObj->Sampler,
            _mesa_test_texobj_completeness(ctx, texObj);
   if (!_mesa_is_texture_complete(texObj, &texObj->Sampler,
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        if (!is_sampler_border_color_valid(&texObj->Sampler)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
      GLuint64 GLAPIENTRY
   _mesa_GetTextureSamplerHandleARB_no_error(GLuint texture, GLuint sampler)
   {
      struct gl_texture_object *texObj;
                     texObj = _mesa_lookup_texture(ctx, texture);
            if (!_mesa_is_texture_complete(texObj, sampObj,
                     }
      GLuint64 GLAPIENTRY
   _mesa_GetTextureSamplerHandleARB(GLuint texture, GLuint sampler)
   {
      struct gl_texture_object *texObj = NULL;
                     if (!_mesa_has_ARB_bindless_texture(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_VALUE is generated by GetTextureHandleARB or
   *  GetTextureSamplerHandleARB if <texture> is zero or not the name of an
   *  existing texture object."
   */
   if (texture > 0)
            if (!texObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_VALUE is generated by GetTextureSamplerHandleARB if
   *  <sampler> is zero or is not the name of an existing sampler object."
   */
   sampObj = _mesa_lookup_samplerobj(ctx, sampler);
   if (!sampObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_OPERATION is generated by GetTextureHandleARB or
   *  GetTextureSamplerHandleARB if the texture object specified by <texture>
   *  is not complete."
   */
   if (!_mesa_is_texture_complete(texObj, sampObj,
            _mesa_test_texobj_completeness(ctx, texObj);
   if (!_mesa_is_texture_complete(texObj, sampObj,
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        if (!is_sampler_border_color_valid(sampObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
      void GLAPIENTRY
   _mesa_MakeTextureHandleResidentARB_no_error(GLuint64 handle)
   {
                        texHandleObj = lookup_texture_handle(ctx, handle);
      }
      void GLAPIENTRY
   _mesa_MakeTextureHandleResidentARB(GLuint64 handle)
   {
                        if (!_mesa_has_ARB_bindless_texture(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_OPERATION is generated by MakeTextureHandleResidentARB
   *  if <handle> is not a valid texture handle, or if <handle> is already
   *  resident in the current GL context."
   */
   texHandleObj = lookup_texture_handle(ctx, handle);
   if (!texHandleObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (is_texture_handle_resident(ctx, handle)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
      void GLAPIENTRY
   _mesa_MakeTextureHandleNonResidentARB_no_error(GLuint64 handle)
   {
                        texHandleObj = lookup_texture_handle(ctx, handle);
      }
      void GLAPIENTRY
   _mesa_MakeTextureHandleNonResidentARB(GLuint64 handle)
   {
                        if (!_mesa_has_ARB_bindless_texture(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_OPERATION is generated by
   *  MakeTextureHandleNonResidentARB if <handle> is not a valid texture
   *  handle, or if <handle> is not resident in the current GL context."
   */
   texHandleObj = lookup_texture_handle(ctx, handle);
   if (!texHandleObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (!is_texture_handle_resident(ctx, handle)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
      GLuint64 GLAPIENTRY
   _mesa_GetImageHandleARB_no_error(GLuint texture, GLint level, GLboolean layered,
         {
                        texObj = _mesa_lookup_texture(ctx, texture);
   if (!_mesa_is_texture_complete(texObj, &texObj->Sampler,
                     }
      GLuint64 GLAPIENTRY
   _mesa_GetImageHandleARB(GLuint texture, GLint level, GLboolean layered,
         {
                        if (!_mesa_has_ARB_bindless_texture(ctx) ||
      !_mesa_has_ARB_shader_image_load_store(ctx)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_VALUE is generated by GetImageHandleARB if <texture>
   *  is zero or not the name of an existing texture object, if the image for
   *  <level> does not existing in <texture>, or if <layered> is FALSE and
   *  <layer> is greater than or equal to the number of layers in the image at
   *  <level>."
   */
   if (texture > 0)
            if (!texObj) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetImageHandleARB(texture)");
               if (level < 0 || level >= _mesa_max_texture_levels(ctx, texObj->Target)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetImageHandleARB(level)");
               if (!layered && layer > _mesa_get_texture_layers(texObj, level)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetImageHandleARB(layer)");
               if (!_mesa_is_shader_image_format_supported(ctx, format)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetImageHandleARB(format)");
               /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_OPERATION is generated by GetImageHandleARB if the
   *  texture object <texture> is not complete or if <layered> is TRUE and
   *  <texture> is not a three-dimensional, one-dimensional array, two
   *  dimensional array, cube map, or cube map array texture."
   */
   if (!_mesa_is_texture_complete(texObj, &texObj->Sampler,
            _mesa_test_texobj_completeness(ctx, texObj);
   if (!_mesa_is_texture_complete(texObj, &texObj->Sampler,
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        if (layered && !_mesa_tex_target_is_layered(texObj->Target)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
      void GLAPIENTRY
   _mesa_MakeImageHandleResidentARB_no_error(GLuint64 handle, GLenum access)
   {
                        imgHandleObj = lookup_image_handle(ctx, handle);
      }
      void GLAPIENTRY
   _mesa_MakeImageHandleResidentARB(GLuint64 handle, GLenum access)
   {
                        if (!_mesa_has_ARB_bindless_texture(ctx) ||
      !_mesa_has_ARB_shader_image_load_store(ctx)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (access != GL_READ_ONLY &&
      access != GL_WRITE_ONLY &&
   access != GL_READ_WRITE) {
   _mesa_error(ctx, GL_INVALID_ENUM,
                     /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_OPERATION is generated by MakeImageHandleResidentARB
   *  if <handle> is not a valid image handle, or if <handle> is already
   *  resident in the current GL context."
   */
   imgHandleObj = lookup_image_handle(ctx, handle);
   if (!imgHandleObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (is_image_handle_resident(ctx, handle)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
      void GLAPIENTRY
   _mesa_MakeImageHandleNonResidentARB_no_error(GLuint64 handle)
   {
                        imgHandleObj = lookup_image_handle(ctx, handle);
      }
      void GLAPIENTRY
   _mesa_MakeImageHandleNonResidentARB(GLuint64 handle)
   {
                        if (!_mesa_has_ARB_bindless_texture(ctx) ||
      !_mesa_has_ARB_shader_image_load_store(ctx)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_OPERATION is generated by
   *  MakeImageHandleNonResidentARB if <handle> is not a valid image handle,
   *  or if <handle> is not resident in the current GL context."
   */
   imgHandleObj = lookup_image_handle(ctx, handle);
   if (!imgHandleObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (!is_image_handle_resident(ctx, handle)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
      GLboolean GLAPIENTRY
   _mesa_IsTextureHandleResidentARB_no_error(GLuint64 handle)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      GLboolean GLAPIENTRY
   _mesa_IsTextureHandleResidentARB(GLuint64 handle)
   {
               if (!_mesa_has_ARB_bindless_texture(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_OPERATION will be generated by
   *  IsTextureHandleResidentARB and IsImageHandleResidentARB if <handle> is
   *  not a valid texture or image handle, respectively."
   */
   if (!lookup_texture_handle(ctx, handle)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
      GLboolean GLAPIENTRY
   _mesa_IsImageHandleResidentARB_no_error(GLuint64 handle)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      GLboolean GLAPIENTRY
   _mesa_IsImageHandleResidentARB(GLuint64 handle)
   {
               if (!_mesa_has_ARB_bindless_texture(ctx) ||
      !_mesa_has_ARB_shader_image_load_store(ctx)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_OPERATION will be generated by
   *  IsTextureHandleResidentARB and IsImageHandleResidentARB if <handle> is
   *  not a valid texture or image handle, respectively."
   */
   if (!lookup_image_handle(ctx, handle)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
