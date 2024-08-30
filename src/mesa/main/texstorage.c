   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2011  VMware, Inc.  All Rights Reserved.
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
      /**
   * \file texstorage.c
   * GL_ARB_texture_storage functions
   */
      #include "util/glheader.h"
   #include "context.h"
   #include "enums.h"
      #include "macros.h"
   #include "teximage.h"
   #include "texobj.h"
   #include "mipmap.h"
   #include "texstorage.h"
   #include "textureview.h"
   #include "mtypes.h"
   #include "glformats.h"
   #include "hash.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_texture.h"
      /**
   * Check if the given texture target is a legal texture object target
   * for a glTexStorage() command.
   * This is a bit different than legal_teximage_target() when it comes
   * to cube maps.
   */
   bool
   _mesa_is_legal_tex_storage_target(const struct gl_context *ctx,
         {
      if (dims < 1 || dims > 3) {
      _mesa_problem(ctx, "invalid dims=%u in _mesa_is_legal_tex_storage_target()", dims);
               switch (dims) {
   case 2:
      switch (target) {
   case GL_TEXTURE_2D:
   case GL_TEXTURE_CUBE_MAP:
         }
      case 3:
      switch (target) {
   case GL_TEXTURE_3D:
         case GL_TEXTURE_2D_ARRAY:
         case GL_TEXTURE_CUBE_MAP_ARRAY:
         }
               if (!_mesa_is_desktop_gl(ctx))
            switch (dims) {
   case 1:
      switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
         default:
            case 2:
      switch (target) {
   case GL_PROXY_TEXTURE_2D:
   case GL_PROXY_TEXTURE_CUBE_MAP:
         case GL_TEXTURE_RECTANGLE:
   case GL_PROXY_TEXTURE_RECTANGLE:
         case GL_TEXTURE_1D_ARRAY:
   case GL_PROXY_TEXTURE_1D_ARRAY:
         default:
            case 3:
      switch (target) {
   case GL_PROXY_TEXTURE_3D:
         case GL_PROXY_TEXTURE_2D_ARRAY:
         case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
         default:
            default:
            }
         /** Helper to get a particular texture image in a texture object */
   static struct gl_texture_image *
   get_tex_image(struct gl_context *ctx,
               {
      const GLenum faceTarget =
      (texObj->Target == GL_TEXTURE_CUBE_MAP ||
   texObj->Target == GL_PROXY_TEXTURE_CUBE_MAP)
         }
            static GLboolean
   initialize_texture_fields(struct gl_context *ctx,
                           {
      const GLenum target = texObj->Target;
   const GLuint numFaces = _mesa_num_tex_faces(target);
   GLint level, levelWidth = width, levelHeight = height, levelDepth = depth;
            /* Set up all the texture object's gl_texture_images */
   for (level = 0; level < levels; level++) {
      for (face = 0; face < numFaces; face++) {
            if (!texImage) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexStorage");
      }
               _mesa_init_teximage_fields(ctx, texImage,
                     _mesa_next_mipmap_level_size(target, 0,
            }
   _mesa_update_texture_object_swizzle(ctx, texObj);
      }
         /**
   * Clear all fields of texture object to zeros.  Used for proxy texture tests
   * and to clean up when a texture memory allocation fails.
   */
   static void
   clear_texture_fields(struct gl_context *ctx,
         {
      const GLenum target = texObj->Target;
   const GLuint numFaces = _mesa_num_tex_faces(target);
   GLint level;
            for (level = 0; level < ARRAY_SIZE(texObj->Image[0]); level++) {
      for (face = 0; face < numFaces; face++) {
            if (!texImage) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexStorage");
      }
                        }
         /**
   * Update/re-validate framebuffer object.
   */
   static void
   update_fbo_texture(struct gl_context *ctx, struct gl_texture_object *texObj)
   {
      const unsigned numFaces = _mesa_num_tex_faces(texObj->Target);
   for (int level = 0; level < ARRAY_SIZE(texObj->Image[0]); level++) {
      for (unsigned face = 0; face < numFaces; face++)
         }
         GLboolean
   _mesa_is_legal_tex_storage_format(const struct gl_context *ctx,
         {
      /* check internal format - note that only sized formats are allowed */
   switch (internalformat) {
   case GL_ALPHA:
   case GL_LUMINANCE:
   case GL_LUMINANCE_ALPHA:
   case GL_INTENSITY:
   case GL_RED:
   case GL_RG:
   case GL_RGB:
   case GL_RGBA:
   case GL_BGRA:
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_STENCIL:
   case GL_COMPRESSED_ALPHA:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
   case GL_COMPRESSED_LUMINANCE:
   case GL_COMPRESSED_INTENSITY:
   case GL_COMPRESSED_RGB:
   case GL_COMPRESSED_RGBA:
   case GL_COMPRESSED_SRGB:
   case GL_COMPRESSED_SRGB_ALPHA:
   case GL_COMPRESSED_SLUMINANCE:
   case GL_COMPRESSED_SLUMINANCE_ALPHA:
   case GL_RED_INTEGER:
   case GL_GREEN_INTEGER:
   case GL_BLUE_INTEGER:
   case GL_ALPHA_INTEGER:
   case GL_RGB_INTEGER:
   case GL_RGBA_INTEGER:
   case GL_BGR_INTEGER:
   case GL_BGRA_INTEGER:
   case GL_LUMINANCE_INTEGER_EXT:
   case GL_LUMINANCE_ALPHA_INTEGER_EXT:
      /* these unsized formats are illegal */
      default:
            }
         /**
   * Do error checking for calls to glTexStorage1/2/3D().
   * If an error is found, record it with _mesa_error(), unless the target
   * is a proxy texture.
   * \return GL_TRUE if any error, GL_FALSE otherwise.
   */
   static GLboolean
   tex_storage_error_check(struct gl_context *ctx,
                           struct gl_texture_object *texObj,
   struct gl_memory_object *memObj,
   {
      const char* suffix = dsa ? (memObj ? "tureMem" : "ture") :
            /* Legal format checking has been moved to texstorage and texturestorage in
            /* size check */
   if (!_mesa_valid_tex_storage_dim(width, height, depth)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           if (_mesa_is_compressed_format(ctx, internalformat)) {
      GLenum err;
   if (!_mesa_target_can_be_compressed(ctx, target, internalformat, &err)) {
      _mesa_error(ctx, err,
                              /* levels check */
   if (levels < 1) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glTex%sStorage%uD(levels < 1)",
                     /* check levels against maximum (note different error than above) */
   if (levels > (GLint) _mesa_max_texture_levels(ctx, target)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           /* check levels against width/height/depth */
   if (levels > _mesa_get_tex_max_num_levels(target, width, height, depth)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               "glTex%sStorage%uD(too many levels"
               /* non-default texture object check */
   if (!_mesa_is_proxy_texture(target) && (!texObj || (texObj->Name == 0))) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           /* Check if texObj->Immutable is set */
   if (!_mesa_is_proxy_texture(target) && texObj->Immutable) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTex%sStorage%uD(immutable)",
                     /* additional checks for depth textures */
   if (!_mesa_legal_texture_base_format_for_target(ctx, target, internalformat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTex%sStorage%uD(bad target for texture)",
                        }
      GLboolean
   _mesa_sparse_texture_error_check(struct gl_context *ctx, GLuint dims,
                           {
      int px, py, pz;
   int index = texObj->VirtualPageSizeIndex;
   if (!st_GetSparseTextureVirtualPageSize(ctx, target, format, index,
            _mesa_error(ctx, GL_INVALID_OPERATION, "%s(sparse index = %d)",
                     if (target == GL_TEXTURE_3D) {
      if (width > ctx->Const.MaxSparse3DTextureSize ||
      height > ctx->Const.MaxSparse3DTextureSize ||
   depth > ctx->Const.MaxSparse3DTextureSize)
   } else {
      if (width > ctx->Const.MaxSparseTextureSize ||
                  if (target == GL_TEXTURE_2D_ARRAY ||
      target == GL_TEXTURE_CUBE_MAP_ARRAY) {
   if (depth > ctx->Const.MaxSparseArrayTextureLayers)
      } else if (target == GL_TEXTURE_1D_ARRAY) {
      if (height > ctx->Const.MaxSparseArrayTextureLayers)
                  /* ARB_sparse_texture2 allow non-page-aligned base texture size. */
   if (!_mesa_has_ARB_sparse_texture2(ctx) &&
      (width % px || height % py || depth % pz)) {
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(sparse page size)", func);
               /* ARB_sparse_texture spec:
   *
   *   If the value of SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB is FALSE,
   *   then TexStorage* will generate an INVALID_OPERATION error if
   *     * the texture's TEXTURE_SPARSE_ARB parameter is TRUE,
   *     * <target> is one of TEXTURE_1D_ARRAY, TEXTURE_2D_ARRAY,
   *       TEXTURE_CUBE_MAP, or TEXTURE_CUBE_MAP_ARRAY, and
   *     * for the virtual page size corresponding to the
   *       VIRTUAL_PAGE_SIZE_INDEX_ARB parameter, either of the following is
   *       true:
   *         - <width> is not a multiple of VIRTUAL_PAGE_SIZE_X_ARB *
   *            2^(<levels>-1), or
   *         - <height> is not a multiple of VIRTUAL_PAGE_SIZE_Y_ARB *
   *            2^(<levels>-1).
   *
   * This make sure all allocated mipmap level size is multiple of virtual
   * page size when SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB is FALSE.
   */
   if (!ctx->Const.SparseTextureFullArrayCubeMipmaps &&
      (target == GL_TEXTURE_1D_ARRAY ||
   target == GL_TEXTURE_2D_ARRAY ||
   target == GL_TEXTURE_CUBE_MAP ||
   target == GL_TEXTURE_CUBE_MAP_ARRAY) &&
   (width % (px << (levels - 1)) ||
   height % (py << (levels - 1)))) {
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s(sparse array align)", func);
                     exceed_max_size:
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(exceed max sparse size)", func);
      }
      /**
   * Helper that does the storage allocation for _mesa_TexStorage1/2/3D()
   * and _mesa_TextureStorage1/2/3D().
   */
   static ALWAYS_INLINE void
   texture_storage(struct gl_context *ctx, GLuint dims,
                  struct gl_texture_object *texObj,
   struct gl_memory_object *memObj, GLenum target,
      {
      GLboolean sizeOK = GL_TRUE, dimensionsOK = GL_TRUE;
   mesa_format texFormat;
   const char* suffix = dsa ? (memObj ? "tureMem" : "ture") :
                     if (!no_error) {
      if (tex_storage_error_check(ctx, texObj, memObj, dims, target, levels,
                           texFormat = _mesa_choose_texture_format(ctx, texObj, target, 0,
            if (!no_error) {
      /* check that width, height, depth are legal for the mipmap level */
   dimensionsOK = _mesa_legal_texture_dimensions(ctx, target, 0,
            sizeOK = st_TestProxyTexImage(ctx, target, levels, 0, texFormat,
               if (_mesa_is_proxy_texture(target)) {
      if (dimensionsOK && sizeOK) {
      initialize_texture_fields(ctx, texObj, levels, width, height, depth,
      }
   else {
      /* clear all image fields for [levels] */
         }
   else {
      if (!no_error) {
      if (!dimensionsOK) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           if (!sizeOK) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                           if (texObj->IsSparse) {
      char func[32];
   snprintf(func, 32, "glTex%sStorage%uD", suffix, dims);
   if (_mesa_sparse_texture_error_check(ctx, dims, texObj, texFormat, target,
                        assert(levels > 0);
   assert(width > 0);
   assert(height > 0);
            if (!initialize_texture_fields(ctx, texObj, levels, width, height, depth,
                        /* Setup the backing memory */
   if (memObj) {
      if (!st_SetTextureStorageForMemoryObject(ctx, texObj, memObj,
                           clear_texture_fields(ctx, texObj);
         }
   else {
      if (!st_AllocTextureStorage(ctx, texObj, levels,
            /* Reset the texture images' info to zeros.
   * Strictly speaking, we probably don't have to do this since
   * generating GL_OUT_OF_MEMORY can leave things in an undefined
   * state but this puts things in a consistent state.
   */
   clear_texture_fields(ctx, texObj);
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTex%sStorage%uD",
                                       }
         static void
   texture_storage_error(struct gl_context *ctx, GLuint dims,
                           {
      texture_storage(ctx, dims, texObj, NULL, target, levels, internalformat,
      }
         static void
   texture_storage_no_error(struct gl_context *ctx, GLuint dims,
                           {
      texture_storage(ctx, dims, texObj, NULL, target, levels, internalformat,
      }
         /**
   * Helper used by _mesa_TexStorage1/2/3D().
   */
   static void
   texstorage_error(GLuint dims, GLenum target, GLsizei levels,
               {
      struct gl_texture_object *texObj;
            /* Check target.  This is done here so that texture_storage
   * can receive unsized formats.
   */
   if (!_mesa_is_legal_tex_storage_target(ctx, dims, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                           if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "%s %s %d %s %d %d %d\n", caller,
                     /* Check the format to make sure it is sized. */
   if (!_mesa_is_legal_tex_storage_format(ctx, internalformat)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                           texObj = _mesa_get_current_tex_object(ctx, target);
   if (!texObj)
            texture_storage_error(ctx, dims, texObj, target, levels,
      }
         static void
   texstorage_no_error(GLuint dims, GLenum target, GLsizei levels,
               {
               struct gl_texture_object *texObj = _mesa_get_current_tex_object(ctx, target);
   texture_storage_no_error(ctx, dims, texObj, target, levels,
      }
         /**
   * Helper used by _mesa_TextureStorage1/2/3D().
   */
   static void
   texturestorage_error(GLuint dims, GLuint texture, GLsizei levels,
               {
      struct gl_texture_object *texObj;
            if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "%s %d %d %s %d %d %d\n",
                     /* Check the format to make sure it is sized. */
   if (!_mesa_is_legal_tex_storage_format(ctx, internalformat)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                           texObj = _mesa_lookup_texture_err(ctx, texture, caller);
   if (!texObj)
            /* Check target.  This is done here so that texture_storage
   * can receive unsized formats.
   */
   if (!_mesa_is_legal_tex_storage_target(ctx, dims, texObj->Target)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           texture_storage_error(ctx, dims, texObj, texObj->Target,
      }
         static void
   texturestorage_no_error(GLuint dims, GLuint texture, GLsizei levels,
               {
               struct gl_texture_object *texObj = _mesa_lookup_texture(ctx, texture);
   texture_storage_no_error(ctx, dims, texObj, texObj->Target,
      }
         void GLAPIENTRY
   _mesa_TexStorage1D_no_error(GLenum target, GLsizei levels,
         {
      texstorage_no_error(1, target, levels, internalformat, width, 1, 1,
      }
         void GLAPIENTRY
   _mesa_TexStorage1D(GLenum target, GLsizei levels, GLenum internalformat,
         {
      texstorage_error(1, target, levels, internalformat, width, 1, 1,
      }
         void GLAPIENTRY
   _mesa_TexStorage2D_no_error(GLenum target, GLsizei levels,
               {
      texstorage_no_error(2, target, levels, internalformat, width, height, 1,
      }
         void GLAPIENTRY
   _mesa_TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat,
         {
      texstorage_error(2, target, levels, internalformat, width, height, 1,
      }
         void GLAPIENTRY
   _mesa_TexStorage3D_no_error(GLenum target, GLsizei levels,
               {
      texstorage_no_error(3, target, levels, internalformat, width, height, depth,
      }
         void GLAPIENTRY
   _mesa_TexStorage3D(GLenum target, GLsizei levels, GLenum internalformat,
         {
      texstorage_error(3, target, levels, internalformat, width, height, depth,
      }
         void GLAPIENTRY
   _mesa_TextureStorage1D_no_error(GLuint texture, GLsizei levels,
         {
      texturestorage_no_error(1, texture, levels, internalformat, width, 1, 1,
      }
         void GLAPIENTRY
   _mesa_TextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat,
         {
      texturestorage_error(1, texture, levels, internalformat, width, 1, 1,
      }
         void GLAPIENTRY
   _mesa_TextureStorage2D_no_error(GLuint texture, GLsizei levels,
               {
      texturestorage_no_error(2, texture, levels, internalformat, width, height, 1,
      }
         void GLAPIENTRY
   _mesa_TextureStorage2D(GLuint texture, GLsizei levels,
               {
      texturestorage_error(2, texture, levels, internalformat, width, height, 1,
      }
         void GLAPIENTRY
   _mesa_TextureStorage3D_no_error(GLuint texture, GLsizei levels,
               {
      texturestorage_no_error(3, texture, levels, internalformat, width, height,
      }
         void GLAPIENTRY
   _mesa_TextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat,
         {
      texturestorage_error(3, texture, levels, internalformat, width, height, depth,
      }
         void GLAPIENTRY
   _mesa_TextureStorage1DEXT(GLuint texture, GLenum target, GLsizei levels,
               {
      GET_CURRENT_CONTEXT(ctx);
   /* 'texture' must always be initialized, even if the call to
   * glTextureStorage1DEXT will generate an error.
   */
   if (!_mesa_lookup_or_create_texture(ctx, target, texture, false, true,
               texturestorage_error(1, texture, levels, internalformat, width, 1, 1,
      }
         void GLAPIENTRY
   _mesa_TextureStorage2DEXT(GLuint texture, GLenum target, GLsizei levels,
               {
      GET_CURRENT_CONTEXT(ctx);
   /* 'texture' must always be initialized, even if the call to
   * glTextureStorage2DEXT will generate an error.
   */
   if (!_mesa_lookup_or_create_texture(ctx, target, texture, false, true,
               texturestorage_error(2, texture, levels, internalformat, width, height, 1,
      }
         void GLAPIENTRY
   _mesa_TextureStorage3DEXT(GLuint texture, GLenum target, GLsizei levels,
               {
      GET_CURRENT_CONTEXT(ctx);
   /* 'texture' must always be initialized, even if the call to
   * glTextureStorage3DEXT will generate an error.
   */
   if (!_mesa_lookup_or_create_texture(ctx, target, texture, false, true,
               texturestorage_error(3, texture, levels, internalformat, width, height, depth,
      }
         void
   _mesa_texture_storage_memory(struct gl_context *ctx, GLuint dims,
                              struct gl_texture_object *texObj,
      {
               texture_storage(ctx, dims, texObj, memObj, target, levels, internalformat,
      }
