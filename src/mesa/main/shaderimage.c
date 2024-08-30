   /*
   * Copyright 2013 Intel Corporation
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
   *
   * Authors:
   *    Francisco Jerez <currojerez@riseup.net>
   */
      #include <assert.h>
      #include "shaderimage.h"
   #include "mtypes.h"
   #include "formats.h"
   #include "errors.h"
   #include "hash.h"
   #include "context.h"
   #include "texobj.h"
   #include "teximage.h"
   #include "enums.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_context.h"
      mesa_format
   _mesa_get_shader_image_format(GLenum format)
   {
      switch (format) {
   case GL_RGBA32F:
            case GL_RGBA16F:
            case GL_RG32F:
            case GL_RG16F:
            case GL_R11F_G11F_B10F:
            case GL_R32F:
            case GL_R16F:
            case GL_RGBA32UI:
            case GL_RGBA16UI:
            case GL_RGB10_A2UI:
            case GL_RGBA8UI:
            case GL_RG32UI:
            case GL_RG16UI:
            case GL_RG8UI:
            case GL_R32UI:
            case GL_R16UI:
            case GL_R8UI:
            case GL_RGBA32I:
            case GL_RGBA16I:
            case GL_RGBA8I:
            case GL_RG32I:
            case GL_RG16I:
            case GL_RG8I:
            case GL_R32I:
            case GL_R16I:
            case GL_R8I:
            case GL_RGBA16:
            case GL_RGB10_A2:
            case GL_RGBA8:
            case GL_RG16:
            case GL_RG8:
            case GL_R16:
            case GL_R8:
            case GL_RGBA16_SNORM:
            case GL_RGBA8_SNORM:
            case GL_RG16_SNORM:
            case GL_RG8_SNORM:
            case GL_R16_SNORM:
            case GL_R8_SNORM:
            default:
            }
      enum image_format_class
   {
      /** Not a valid image format. */
            /** Classes of image formats you can cast into each other. */
   /** \{ */
   IMAGE_FORMAT_CLASS_1X8,
   IMAGE_FORMAT_CLASS_1X16,
   IMAGE_FORMAT_CLASS_1X32,
   IMAGE_FORMAT_CLASS_2X8,
   IMAGE_FORMAT_CLASS_2X16,
   IMAGE_FORMAT_CLASS_2X32,
   IMAGE_FORMAT_CLASS_10_11_11,
   IMAGE_FORMAT_CLASS_4X8,
   IMAGE_FORMAT_CLASS_4X16,
   IMAGE_FORMAT_CLASS_4X32,
   IMAGE_FORMAT_CLASS_2_10_10_10
      };
      static enum image_format_class
   get_image_format_class(mesa_format format)
   {
      switch (format) {
   case MESA_FORMAT_RGBA_FLOAT32:
            case MESA_FORMAT_RGBA_FLOAT16:
            case MESA_FORMAT_RG_FLOAT32:
            case MESA_FORMAT_RG_FLOAT16:
            case MESA_FORMAT_R11G11B10_FLOAT:
            case MESA_FORMAT_R_FLOAT32:
            case MESA_FORMAT_R_FLOAT16:
            case MESA_FORMAT_RGBA_UINT32:
            case MESA_FORMAT_RGBA_UINT16:
            case MESA_FORMAT_R10G10B10A2_UINT:
            case MESA_FORMAT_RGBA_UINT8:
            case MESA_FORMAT_RG_UINT32:
            case MESA_FORMAT_RG_UINT16:
            case MESA_FORMAT_RG_UINT8:
            case MESA_FORMAT_R_UINT32:
            case MESA_FORMAT_R_UINT16:
            case MESA_FORMAT_R_UINT8:
            case MESA_FORMAT_RGBA_SINT32:
            case MESA_FORMAT_RGBA_SINT16:
            case MESA_FORMAT_RGBA_SINT8:
            case MESA_FORMAT_RG_SINT32:
            case MESA_FORMAT_RG_SINT16:
            case MESA_FORMAT_RG_SINT8:
            case MESA_FORMAT_R_SINT32:
            case MESA_FORMAT_R_SINT16:
            case MESA_FORMAT_R_SINT8:
            case MESA_FORMAT_RGBA_UNORM16:
            case MESA_FORMAT_R10G10B10A2_UNORM:
            case MESA_FORMAT_RGBA_UNORM8:
            case MESA_FORMAT_RG_UNORM16:
            case MESA_FORMAT_RG_UNORM8:
            case MESA_FORMAT_R_UNORM16:
            case MESA_FORMAT_R_UNORM8:
            case MESA_FORMAT_RGBA_SNORM16:
            case MESA_FORMAT_RGBA_SNORM8:
            case MESA_FORMAT_RG_SNORM16:
            case MESA_FORMAT_RG_SNORM8:
            case MESA_FORMAT_R_SNORM16:
            case MESA_FORMAT_R_SNORM8:
            default:
            }
      static GLenum
   _image_format_class_to_glenum(enum image_format_class class)
   {
      switch (class) {
   case IMAGE_FORMAT_CLASS_NONE:
         case IMAGE_FORMAT_CLASS_1X8:
         case IMAGE_FORMAT_CLASS_1X16:
         case IMAGE_FORMAT_CLASS_1X32:
         case IMAGE_FORMAT_CLASS_2X8:
         case IMAGE_FORMAT_CLASS_2X16:
         case IMAGE_FORMAT_CLASS_2X32:
         case IMAGE_FORMAT_CLASS_10_11_11:
         case IMAGE_FORMAT_CLASS_4X8:
         case IMAGE_FORMAT_CLASS_4X16:
         case IMAGE_FORMAT_CLASS_4X32:
         case IMAGE_FORMAT_CLASS_2_10_10_10:
         default:
      assert(!"Invalid image_format_class");
         }
      GLenum
   _mesa_get_image_format_class(GLenum format)
   {
      mesa_format tex_format = _mesa_get_shader_image_format(format);
   if (tex_format == MESA_FORMAT_NONE)
            enum image_format_class class = get_image_format_class(tex_format);
      }
      bool
   _mesa_is_shader_image_format_supported(const struct gl_context *ctx,
         {
      switch (format) {
   /* Formats supported on both desktop and ES GL, c.f. table 8.27 of the
   * OpenGL ES 3.1 specification.
   */
   case GL_RGBA32F:
   case GL_RGBA16F:
   case GL_R32F:
   case GL_RGBA32UI:
   case GL_RGBA16UI:
   case GL_RGBA8UI:
   case GL_R32UI:
   case GL_RGBA32I:
   case GL_RGBA16I:
   case GL_RGBA8I:
   case GL_R32I:
   case GL_RGBA8:
   case GL_RGBA8_SNORM:
            /* Formats supported on unextended desktop GL and the original
   * ARB_shader_image_load_store extension, c.f. table 3.21 of the OpenGL 4.2
   * specification or by GLES 3.1 with GL_NV_image_formats extension.
   */
   case GL_RG32F:
   case GL_RG16F:
   case GL_R11F_G11F_B10F:
   case GL_R16F:
   case GL_RGB10_A2UI:
   case GL_RG32UI:
   case GL_RG16UI:
   case GL_RG8UI:
   case GL_R16UI:
   case GL_R8UI:
   case GL_RG32I:
   case GL_RG16I:
   case GL_RG8I:
   case GL_R16I:
   case GL_R8I:
   case GL_RGB10_A2:
   case GL_RG8:
   case GL_R8:
   case GL_RG8_SNORM:
   case GL_R8_SNORM:
            /* Formats supported on unextended desktop GL and the original
   * ARB_shader_image_load_store extension, c.f. table 3.21 of the OpenGL 4.2
   * specification.
   *
   * Following formats are supported by GLES 3.1 with GL_NV_image_formats &
   * GL_EXT_texture_norm16 extensions.
   */
   case GL_RGBA16:
   case GL_RGBA16_SNORM:
   case GL_RG16:
   case GL_RG16_SNORM:
   case GL_R16:
   case GL_R16_SNORM:
            default:
            }
      struct gl_image_unit
   _mesa_default_image_unit(struct gl_context *ctx)
   {
      const GLenum format = _mesa_is_desktop_gl(ctx) ? GL_R8 : GL_R32UI;
   const struct gl_image_unit u = {
      .Access = GL_READ_ONLY,
   .Format = format,
      };
      }
      void
   _mesa_init_image_units(struct gl_context *ctx)
   {
                        for (i = 0; i < ARRAY_SIZE(ctx->ImageUnits); ++i)
      }
         void
   _mesa_free_image_textures(struct gl_context *ctx)
   {
               for (i = 0; i < ARRAY_SIZE(ctx->ImageUnits); ++i)
      }
      GLboolean
   _mesa_is_image_unit_valid(struct gl_context *ctx, struct gl_image_unit *u)
   {
      struct gl_texture_object *t = u->TexObj;
            if (!t)
            if (!t->_BaseComplete && !t->_MipmapComplete)
            if (u->Level < t->Attrib.BaseLevel ||
      u->Level > t->_MaxLevel ||
   (u->Level == t->Attrib.BaseLevel && !t->_BaseComplete) ||
   (u->Level != t->Attrib.BaseLevel && !t->_MipmapComplete))
         if (_mesa_tex_target_is_layered(t->Target) &&
      u->_Layer >= _mesa_get_texture_layers(t, u->Level))
         if (t->Target == GL_TEXTURE_BUFFER) {
            } else {
      struct gl_texture_image *img = (t->Target == GL_TEXTURE_CUBE_MAP ?
                  if (!img || img->Border || img->NumSamples > ctx->Const.MaxImageSamples)
                        if (!tex_format)
            switch (t->Attrib.ImageFormatCompatibilityType) {
   case GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE:
      if (_mesa_get_format_bytes(tex_format) !=
      _mesa_get_format_bytes(u->_ActualFormat))
            case GL_IMAGE_FORMAT_COMPATIBILITY_BY_CLASS:
      if (get_image_format_class(tex_format) !=
      get_image_format_class(u->_ActualFormat))
            default:
                     }
      static GLboolean
   validate_bind_image_texture(struct gl_context *ctx, GLuint unit,
               {
               if (unit >= ctx->Const.MaxImageUnits) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(unit)");
               if (check_level_layer) {
      /* EXT_shader_image_load_store doesn't throw an error if level or
   * layer is negative.
   */
   if (level < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(level)");
                  if (layer < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(layer)");
               if (access != GL_READ_ONLY &&
      access != GL_WRITE_ONLY &&
   access != GL_READ_WRITE) {
   _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(access)");
               if (!_mesa_is_shader_image_format_supported(ctx, format)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(format)");
                  }
      static void
   set_image_binding(struct gl_image_unit *u, struct gl_texture_object *texObj,
               {
      u->Level = level;
   u->Access = access;
   u->Format = format;
            if (texObj && _mesa_tex_target_is_layered(texObj->Target)) {
      u->Layered = layered;
      } else {
      u->Layered = GL_FALSE;
      }
               }
      static void
   bind_image_texture(struct gl_context *ctx, struct gl_texture_object *texObj,
               {
                        FLUSH_VERTICES(ctx, 0, 0);
               }
      void GLAPIENTRY
   _mesa_BindImageTexture_no_error(GLuint unit, GLuint texture, GLint level,
               {
                        if (texture)
               }
      void GLAPIENTRY
   _mesa_BindImageTexture(GLuint unit, GLuint texture, GLint level,
               {
                        if (!validate_bind_image_texture(ctx, unit, texture, level, layer, access,
                  if (texture) {
               if (!texObj) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTexture(texture)");
               /* From section 8.22 "Texture Image Loads and Stores" of the OpenGL ES
   * 3.1 spec:
   *
   * "An INVALID_OPERATION error is generated if texture is not the name
   *  of an immutable texture object."
   *
   * However note that issue 7 of the GL_OES_texture_buffer spec
   * recognizes that there is no way to create immutable buffer textures,
   * so those are excluded from this requirement.
   *
   * Additionally, issue 10 of the OES_EGL_image_external_essl3 spec
   * states that glBindImageTexture must accept external texture objects.
   */
   if (_mesa_is_gles(ctx) && !texObj->Immutable && !texObj->External &&
      texObj->Target != GL_TEXTURE_BUFFER) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                           }
      void GLAPIENTRY
   _mesa_BindImageTextureEXT(GLuint index, GLuint texture, GLint level,
               {
                        if (!validate_bind_image_texture(ctx, index, texture, level, layer, access,
                  if (texture) {
               if (!texObj) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindImageTextureEXT(texture)");
                     }
      static ALWAYS_INLINE void
   bind_image_textures(struct gl_context *ctx, GLuint first, GLuint count,
         {
               /* Assume that at least one binding will be changed */
   FLUSH_VERTICES(ctx, 0, 0);
            /* Note that the error semantics for multi-bind commands differ from
   * those of other GL commands.
   *
   * The Issues section in the ARB_multi_bind spec says:
   *
   *    "(11) Typically, OpenGL specifies that if an error is generated by
   *          a command, that command has no effect.  This is somewhat
   *          unfortunate for multi-bind commands, because it would require
   *          a first pass to scan the entire list of bound objects for
   *          errors and then a second pass to actually perform the
   *          bindings.  Should we have different error semantics?
   *
   *       RESOLVED:  Yes.  In this specification, when the parameters for
   *       one of the <count> binding points are invalid, that binding
   *       point is not updated and an error will be generated.  However,
   *       other binding points in the same command will be updated if
   *       their parameters are valid and no other error occurs."
                     for (i = 0; i < count; i++) {
      struct gl_image_unit *u = &ctx->ImageUnits[first + i];
            if (texture) {
                     if (!texObj || texObj->Name != texture) {
      texObj = _mesa_lookup_texture_locked(ctx, texture);
   if (!no_error && !texObj) {
      /* The ARB_multi_bind spec says:
   *
   *    "An INVALID_OPERATION error is generated if any value
   *     in <textures> is not zero or the name of an existing
   *     texture object (per binding)."
   */
   _mesa_error(ctx, GL_INVALID_OPERATION,
               "glBindImageTextures(textures[%d]=%u "
                  if (texObj->Target == GL_TEXTURE_BUFFER) {
                           if (!no_error && (!image || image->Width == 0 ||
            /* The ARB_multi_bind spec says:
   *
   *    "An INVALID_OPERATION error is generated if the width,
   *     height, or depth of the level zero texture image of
   *     any texture in <textures> is zero (per binding)."
   */
   _mesa_error(ctx, GL_INVALID_OPERATION,
                                             if (!no_error &&
      !_mesa_is_shader_image_format_supported(ctx, tex_format)) {
   /* The ARB_multi_bind spec says:
   *
   *   "An INVALID_OPERATION error is generated if the internal
   *    format of the level zero texture image of any texture
   *    in <textures> is not found in table 8.33 (per binding)."
   */
   _mesa_error(ctx, GL_INVALID_OPERATION,
               "glBindImageTextures(the internal format %s of "
   "the level zero texture image of textures[%d]=%u "
   "is not supported)",
               /* Update the texture binding */
   set_image_binding(u, texObj, 0,
            } else {
      /* Unbind the texture from the unit */
                     }
      void GLAPIENTRY
   _mesa_BindImageTextures_no_error(GLuint first, GLsizei count,
         {
                  }
      void GLAPIENTRY
   _mesa_BindImageTextures(GLuint first, GLsizei count, const GLuint *textures)
   {
               if (!ctx->Extensions.ARB_shader_image_load_store &&
      !_mesa_is_gles31(ctx)) {
   _mesa_error(ctx, GL_INVALID_OPERATION, "glBindImageTextures()");
               if (first + count > ctx->Const.MaxImageUnits) {
      /* The ARB_multi_bind spec says:
   *
   *    "An INVALID_OPERATION error is generated if <first> + <count>
   *     is greater than the number of image units supported by
   *     the implementation."
   */
   _mesa_error(ctx, GL_INVALID_OPERATION,
               "glBindImageTextures(first=%u + count=%d > the value of "
                  }
