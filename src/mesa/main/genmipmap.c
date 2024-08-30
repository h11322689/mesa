   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 1999-2013  VMware, Inc.  All Rights Reserved.
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
         /*
   * glGenerateMipmap function
   */
      #include "context.h"
   #include "enums.h"
   #include "genmipmap.h"
   #include "glformats.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "teximage.h"
   #include "texobj.h"
   #include "hash.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_gen_mipmap.h"
      bool
   _mesa_is_valid_generate_texture_mipmap_target(struct gl_context *ctx,
         {
               switch (target) {
   case GL_TEXTURE_1D:
      error = _mesa_is_gles(ctx);
      case GL_TEXTURE_2D:
      error = false;
      case GL_TEXTURE_3D:
      error = _mesa_is_gles1(ctx);
      case GL_TEXTURE_CUBE_MAP:
      error = false;
      case GL_TEXTURE_1D_ARRAY:
      error = _mesa_is_gles(ctx) || !ctx->Extensions.EXT_texture_array;
      case GL_TEXTURE_2D_ARRAY:
      error = (_mesa_is_gles(ctx) && ctx->Version < 30)
            case GL_TEXTURE_CUBE_MAP_ARRAY:
      error = !_mesa_has_texture_cube_map_array(ctx);
      default:
                     }
      bool
   _mesa_is_valid_generate_texture_mipmap_internalformat(struct gl_context *ctx,
         {
      if (_mesa_is_gles3(ctx)) {
      /* From the ES 3.2 specification's description of GenerateMipmap():
   * "An INVALID_OPERATION error is generated if the levelbase array was
   *  not specified with an unsized internal format from table 8.3 or a
   *  sized internal format that is both color-renderable and
   *  texture-filterable according to table 8.10."
   *
   * GL_EXT_texture_format_BGRA8888 adds a GL_BGRA_EXT unsized internal
   * format, and includes it in a very similar looking table.  So we
   * include it here as well.
   */
   return internalformat == GL_RGBA || internalformat == GL_RGB ||
         internalformat == GL_LUMINANCE_ALPHA ||
   internalformat == GL_LUMINANCE || internalformat == GL_ALPHA ||
   internalformat == GL_BGRA_EXT ||
               return (!_mesa_is_enum_format_integer(internalformat) &&
         !_mesa_is_depthstencil_format(internalformat) &&
      }
      /**
   * Implements glGenerateMipmap and glGenerateTextureMipmap.
   * Generates all the mipmap levels below the base level.
   * Error-checking is done only if caller is not NULL.
   */
   static ALWAYS_INLINE void
   generate_texture_mipmap(struct gl_context *ctx,
               {
                        if (texObj->Attrib.BaseLevel >= texObj->Attrib.MaxLevel) {
      /* nothing to do */
               if (caller && texObj->Target == GL_TEXTURE_CUBE_MAP &&
      !_mesa_cube_complete(texObj)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                                       srcImage = _mesa_select_tex_image(texObj, target, texObj->Attrib.BaseLevel);
   if (caller) {
      if (!srcImage) {
      _mesa_unlock_texture(ctx, texObj);
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (!_mesa_is_valid_generate_texture_mipmap_internalformat(ctx,
            _mesa_unlock_texture(ctx, texObj);
   _mesa_error(ctx, GL_INVALID_OPERATION,
                           /* The GLES 2.0 spec says:
   *
   *    "If the level zero array is stored in a compressed internal format,
   *     the error INVALID_OPERATION is generated."
   *
   * and this text is gone from the GLES 3.0 spec.
   */
   if (_mesa_is_gles2(ctx) && ctx->Version < 30 &&
      _mesa_is_format_compressed(srcImage->TexFormat)) {
   _mesa_unlock_texture(ctx, texObj);
   _mesa_error(ctx, GL_INVALID_OPERATION, "generate mipmaps on compressed texture");
                  if (srcImage->Width == 0 || srcImage->Height == 0) {
      _mesa_unlock_texture(ctx, texObj);
               if (target == GL_TEXTURE_CUBE_MAP) {
      GLuint face;
   for (face = 0; face < 6; face++) {
      st_generate_mipmap(ctx,
         }
   else {
         }
      }
      /**
   * Generate all the mipmap levels below the base level.
   * Note: this GL function would be more useful if one could specify a
   * cube face, a set of array slices, etc.
   */
   void GLAPIENTRY
   _mesa_GenerateMipmap_no_error(GLenum target)
   {
               struct gl_texture_object *texObj = _mesa_get_current_tex_object(ctx, target);
      }
      void GLAPIENTRY
   _mesa_GenerateMipmap(GLenum target)
   {
      struct gl_texture_object *texObj;
            if (!_mesa_is_valid_generate_texture_mipmap_target(ctx, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGenerateMipmap(target=%s)",
                     texObj = _mesa_get_current_tex_object(ctx, target);
   if (!texObj)
               }
      /**
   * Generate all the mipmap levels below the base level.
   */
   void GLAPIENTRY
   _mesa_GenerateTextureMipmap_no_error(GLuint texture)
   {
               struct gl_texture_object *texObj = _mesa_lookup_texture(ctx, texture);
      }
      static void
   validate_params_and_generate_mipmap(struct gl_texture_object *texObj, const char* caller)
   {
               if (!texObj)
            if (!_mesa_is_valid_generate_texture_mipmap_target(ctx, texObj->Target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(target=%s)",
                              }
      void GLAPIENTRY
   _mesa_GenerateTextureMipmap(GLuint texture)
   {
      struct gl_texture_object *texObj;
            texObj = _mesa_lookup_texture_err(ctx, texture, "glGenerateTextureMipmap");
      }
      void GLAPIENTRY
   _mesa_GenerateTextureMipmapEXT(GLuint texture, GLenum target)
   {
      struct gl_texture_object *texObj;
            texObj = _mesa_lookup_or_create_texture(ctx, target, texture,
               validate_params_and_generate_mipmap(texObj,
      }
      void GLAPIENTRY
   _mesa_GenerateMultiTexMipmapEXT(GLenum texunit, GLenum target)
   {
      struct gl_texture_object *texObj;
            texObj = _mesa_get_texobj_by_target_and_texunit(ctx, target,
                     validate_params_and_generate_mipmap(texObj,
      }
