   /*
   * Copyright © 2012 Intel Corporation
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
      #include "mtypes.h"
   #include "context.h"
   #include "glformats.h"
   #include "macros.h"
   #include "enums.h"
   #include "fbobject.h"
   #include "formatquery.h"
   #include "teximage.h"
   #include "texparam.h"
   #include "texobj.h"
   #include "get.h"
   #include "genmipmap.h"
   #include "shaderimage.h"
   #include "texcompress.h"
   #include "textureview.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_format.h"
      static bool
   _is_renderable(struct gl_context *ctx, GLenum internalformat)
   {
      /*  Section 4.4.4 on page 212 of the  GLES 3.0.4 spec says:
   *
   *     "An internal format is color-renderable if it is one of the
   *     formats from table 3.13 noted as color-renderable or if it
   *     is unsized format RGBA or RGB."
   *
   * Therefore, we must accept GL_RGB and GL_RGBA here.
   */
   if (internalformat != GL_RGB && internalformat != GL_RGBA &&
      _mesa_base_fbo_format(ctx, internalformat) == 0)
            }
      /* Handles the cases where either ARB_internalformat_query or
   * ARB_internalformat_query2 have to return an error.
   */
   static bool
   _legal_parameters(struct gl_context *ctx, GLenum target, GLenum internalformat,
            {
               /* The ARB_internalformat_query2 spec says:
   *
   *    "The INVALID_ENUM error is generated if the <target> parameter to
   *    GetInternalformati*v is not one of the targets listed in Table 6.xx.
   */
   switch(target){
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_BUFFER:
      if (!query2) {
      /* The ARB_internalformat_query spec says:
   *
   *     "If the <target> parameter to GetInternalformativ is not one of
   *      TEXTURE_2D_MULTISAMPLE, TEXTURE_2D_MULTISAMPLE_ARRAY
   *      or RENDERBUFFER then an INVALID_ENUM error is generated.
   */
   _mesa_error(ctx, GL_INVALID_ENUM,
                     }
         case GL_RENDERBUFFER:
            case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      /* The non-existence of ARB_texture_multisample is treated in
   * ARB_internalformat_query implementation like an error.
   */
   if (!query2 &&
      !(_mesa_has_ARB_texture_multisample(ctx) || _mesa_is_gles31(ctx))) {
   _mesa_error(ctx, GL_INVALID_ENUM,
                     }
         default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                              /* The ARB_internalformat_query2 spec says:
   *
   *     "The INVALID_ENUM error is generated if the <pname> parameter is
   *     not one of the listed possibilities.
   */
   switch(pname){
   case GL_SAMPLES:
   case GL_NUM_SAMPLE_COUNTS:
            case GL_TEXTURE_REDUCTION_MODE_ARB:
      if (!_mesa_has_ARB_texture_filter_minmax(ctx)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  }
         case GL_NUM_VIRTUAL_PAGE_SIZES_ARB:
   case GL_VIRTUAL_PAGE_SIZE_X_ARB:
   case GL_VIRTUAL_PAGE_SIZE_Y_ARB:
   case GL_VIRTUAL_PAGE_SIZE_Z_ARB:
      if (!_mesa_has_ARB_sparse_texture(ctx)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  }
         case GL_CLEAR_TEXTURE:
      if (!_mesa_has_ARB_clear_texture(ctx)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  }
         case GL_SRGB_DECODE_ARB:
      /* The ARB_internalformat_query2 spec says:
   *
   *     "If ARB_texture_sRGB_decode or EXT_texture_sRGB_decode or
   *     equivalent functionality is not supported, queries for the
   *     SRGB_DECODE_ARB <pname> set the INVALID_ENUM error.
   */
   if (!_mesa_has_EXT_texture_sRGB_decode(ctx)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  }
      case GL_INTERNALFORMAT_SUPPORTED:
   case GL_INTERNALFORMAT_PREFERRED:
   case GL_INTERNALFORMAT_RED_SIZE:
   case GL_INTERNALFORMAT_GREEN_SIZE:
   case GL_INTERNALFORMAT_BLUE_SIZE:
   case GL_INTERNALFORMAT_ALPHA_SIZE:
   case GL_INTERNALFORMAT_DEPTH_SIZE:
   case GL_INTERNALFORMAT_STENCIL_SIZE:
   case GL_INTERNALFORMAT_SHARED_SIZE:
   case GL_INTERNALFORMAT_RED_TYPE:
   case GL_INTERNALFORMAT_GREEN_TYPE:
   case GL_INTERNALFORMAT_BLUE_TYPE:
   case GL_INTERNALFORMAT_ALPHA_TYPE:
   case GL_INTERNALFORMAT_DEPTH_TYPE:
   case GL_INTERNALFORMAT_STENCIL_TYPE:
   case GL_MAX_WIDTH:
   case GL_MAX_HEIGHT:
   case GL_MAX_DEPTH:
   case GL_MAX_LAYERS:
   case GL_MAX_COMBINED_DIMENSIONS:
   case GL_COLOR_COMPONENTS:
   case GL_DEPTH_COMPONENTS:
   case GL_STENCIL_COMPONENTS:
   case GL_COLOR_RENDERABLE:
   case GL_DEPTH_RENDERABLE:
   case GL_STENCIL_RENDERABLE:
   case GL_FRAMEBUFFER_RENDERABLE:
   case GL_FRAMEBUFFER_RENDERABLE_LAYERED:
   case GL_FRAMEBUFFER_BLEND:
   case GL_READ_PIXELS:
   case GL_READ_PIXELS_FORMAT:
   case GL_READ_PIXELS_TYPE:
   case GL_TEXTURE_IMAGE_FORMAT:
   case GL_TEXTURE_IMAGE_TYPE:
   case GL_GET_TEXTURE_IMAGE_FORMAT:
   case GL_GET_TEXTURE_IMAGE_TYPE:
   case GL_MIPMAP:
   case GL_MANUAL_GENERATE_MIPMAP:
   case GL_AUTO_GENERATE_MIPMAP:
   case GL_COLOR_ENCODING:
   case GL_SRGB_READ:
   case GL_SRGB_WRITE:
   case GL_FILTER:
   case GL_VERTEX_TEXTURE:
   case GL_TESS_CONTROL_TEXTURE:
   case GL_TESS_EVALUATION_TEXTURE:
   case GL_GEOMETRY_TEXTURE:
   case GL_FRAGMENT_TEXTURE:
   case GL_COMPUTE_TEXTURE:
   case GL_TEXTURE_SHADOW:
   case GL_TEXTURE_GATHER:
   case GL_TEXTURE_GATHER_SHADOW:
   case GL_SHADER_IMAGE_LOAD:
   case GL_SHADER_IMAGE_STORE:
   case GL_SHADER_IMAGE_ATOMIC:
   case GL_IMAGE_TEXEL_SIZE:
   case GL_IMAGE_COMPATIBILITY_CLASS:
   case GL_IMAGE_PIXEL_FORMAT:
   case GL_IMAGE_PIXEL_TYPE:
   case GL_IMAGE_FORMAT_COMPATIBILITY_TYPE:
   case GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST:
   case GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST:
   case GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_WRITE:
   case GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_WRITE:
   case GL_TEXTURE_COMPRESSED:
   case GL_TEXTURE_COMPRESSED_BLOCK_WIDTH:
   case GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT:
   case GL_TEXTURE_COMPRESSED_BLOCK_SIZE:
   case GL_CLEAR_BUFFER:
   case GL_TEXTURE_VIEW:
   case GL_VIEW_COMPATIBILITY_CLASS:
   case GL_NUM_TILING_TYPES_EXT:
   case GL_TILING_TYPES_EXT:
      /* The ARB_internalformat_query spec says:
   *
   *     "If the <pname> parameter to GetInternalformativ is not SAMPLES
   *     or NUM_SAMPLE_COUNTS, then an INVALID_ENUM error is generated."
   */
   if (!query2) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                     }
         default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                           /* The ARB_internalformat_query spec says:
   *
   *     "If the <bufSize> parameter to GetInternalformativ is negative, then
   *     an INVALID_VALUE error is generated."
   *
   * Nothing is said in ARB_internalformat_query2 but we assume the same.
   */
   if (bufSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           /* The ARB_internalformat_query spec says:
   *
   *     "If the <internalformat> parameter to GetInternalformativ is not
   *     color-, depth- or stencil-renderable, then an INVALID_ENUM error is
   *     generated."
   */
   if (!query2 && !_is_renderable(ctx, internalformat)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                              }
      /* Sets the appropriate "unsupported" response as defined by the
   * ARB_internalformat_query2 spec for each each <pname>.
   */
   static void
   _set_default_response(GLenum pname, GLint buffer[16])
   {
      /* The ARB_internalformat_query2 defines which is the reponse best
   * representing "not supported" or "not applicable" for each <pname>.
   *
   *     " In general:
   *          - size- or count-based queries will return zero,
   *          - support-, format- or type-based queries will return NONE,
   *          - boolean-based queries will return FALSE, and
   *          - list-based queries return no entries."
   */
   switch(pname) {
   case GL_SAMPLES:
   case GL_TILING_TYPES_EXT:
            case GL_MAX_COMBINED_DIMENSIONS:
      /* This value can be a 64-bit value. As the default is the 32-bit query,
   * we pack 2 32-bit integers. So we need to clean both */
   buffer[0] = 0;
   buffer[1] = 0;
         case GL_NUM_SAMPLE_COUNTS:
   case GL_INTERNALFORMAT_RED_SIZE:
   case GL_INTERNALFORMAT_GREEN_SIZE:
   case GL_INTERNALFORMAT_BLUE_SIZE:
   case GL_INTERNALFORMAT_ALPHA_SIZE:
   case GL_INTERNALFORMAT_DEPTH_SIZE:
   case GL_INTERNALFORMAT_STENCIL_SIZE:
   case GL_INTERNALFORMAT_SHARED_SIZE:
   case GL_MAX_WIDTH:
   case GL_MAX_HEIGHT:
   case GL_MAX_DEPTH:
   case GL_MAX_LAYERS:
   case GL_IMAGE_TEXEL_SIZE:
   case GL_TEXTURE_COMPRESSED_BLOCK_WIDTH:
   case GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT:
   case GL_TEXTURE_COMPRESSED_BLOCK_SIZE:
   case GL_NUM_TILING_TYPES_EXT:
   case GL_NUM_VIRTUAL_PAGE_SIZES_ARB:
   case GL_VIRTUAL_PAGE_SIZE_X_ARB:
   case GL_VIRTUAL_PAGE_SIZE_Y_ARB:
   case GL_VIRTUAL_PAGE_SIZE_Z_ARB:
      buffer[0] = 0;
         case GL_INTERNALFORMAT_PREFERRED:
   case GL_INTERNALFORMAT_RED_TYPE:
   case GL_INTERNALFORMAT_GREEN_TYPE:
   case GL_INTERNALFORMAT_BLUE_TYPE:
   case GL_INTERNALFORMAT_ALPHA_TYPE:
   case GL_INTERNALFORMAT_DEPTH_TYPE:
   case GL_INTERNALFORMAT_STENCIL_TYPE:
   case GL_FRAMEBUFFER_RENDERABLE:
   case GL_FRAMEBUFFER_RENDERABLE_LAYERED:
   case GL_FRAMEBUFFER_BLEND:
   case GL_READ_PIXELS:
   case GL_READ_PIXELS_FORMAT:
   case GL_READ_PIXELS_TYPE:
   case GL_TEXTURE_IMAGE_FORMAT:
   case GL_TEXTURE_IMAGE_TYPE:
   case GL_GET_TEXTURE_IMAGE_FORMAT:
   case GL_GET_TEXTURE_IMAGE_TYPE:
   case GL_MANUAL_GENERATE_MIPMAP:
   case GL_AUTO_GENERATE_MIPMAP:
   case GL_COLOR_ENCODING:
   case GL_SRGB_READ:
   case GL_SRGB_WRITE:
   case GL_SRGB_DECODE_ARB:
   case GL_FILTER:
   case GL_VERTEX_TEXTURE:
   case GL_TESS_CONTROL_TEXTURE:
   case GL_TESS_EVALUATION_TEXTURE:
   case GL_GEOMETRY_TEXTURE:
   case GL_FRAGMENT_TEXTURE:
   case GL_COMPUTE_TEXTURE:
   case GL_TEXTURE_SHADOW:
   case GL_TEXTURE_GATHER:
   case GL_TEXTURE_GATHER_SHADOW:
   case GL_SHADER_IMAGE_LOAD:
   case GL_SHADER_IMAGE_STORE:
   case GL_SHADER_IMAGE_ATOMIC:
   case GL_IMAGE_COMPATIBILITY_CLASS:
   case GL_IMAGE_PIXEL_FORMAT:
   case GL_IMAGE_PIXEL_TYPE:
   case GL_IMAGE_FORMAT_COMPATIBILITY_TYPE:
   case GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST:
   case GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST:
   case GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_WRITE:
   case GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_WRITE:
   case GL_CLEAR_BUFFER:
   case GL_CLEAR_TEXTURE:
   case GL_TEXTURE_VIEW:
   case GL_VIEW_COMPATIBILITY_CLASS:
      buffer[0] = GL_NONE;
         case GL_INTERNALFORMAT_SUPPORTED:
   case GL_COLOR_COMPONENTS:
   case GL_DEPTH_COMPONENTS:
   case GL_STENCIL_COMPONENTS:
   case GL_COLOR_RENDERABLE:
   case GL_DEPTH_RENDERABLE:
   case GL_STENCIL_RENDERABLE:
   case GL_MIPMAP:
   case GL_TEXTURE_COMPRESSED:
   case GL_TEXTURE_REDUCTION_MODE_ARB:
      buffer[0] = GL_FALSE;
         default:
            }
      static bool
   _is_target_supported(struct gl_context *ctx, GLenum target)
   {
      /* The ARB_internalformat_query2 spec says:
   *
   *     "if a particular type of <target> is not supported by the
   *     implementation the "unsupported" answer should be given.
   *     This is not an error."
   *
   * Note that legality of targets has already been verified.
   */
   switch(target){
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
            case GL_TEXTURE_1D_ARRAY:
      if (!_mesa_has_EXT_texture_array(ctx))
               case GL_TEXTURE_2D_ARRAY:
      if (!_mesa_has_EXT_texture_array(ctx))
               case GL_TEXTURE_CUBE_MAP:
      if (!_mesa_is_desktop_gl(ctx))
               case GL_TEXTURE_CUBE_MAP_ARRAY:
      if (!_mesa_has_ARB_texture_cube_map_array(ctx))
               case GL_TEXTURE_RECTANGLE:
      if (!_mesa_has_ARB_texture_rectangle(ctx))
               case GL_TEXTURE_BUFFER:
      if (!_mesa_has_ARB_texture_buffer_object(ctx))
               case GL_RENDERBUFFER:
      if (!(_mesa_has_ARB_framebuffer_object(ctx) ||
                     case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      if (!(_mesa_has_ARB_texture_multisample(ctx) ||
                     default:
                     }
      static bool
   _is_resource_supported(struct gl_context *ctx, GLenum target,
         {
      /* From the ARB_internalformat_query2 spec:
   *
   * In the following descriptions, the term /resource/ is used to generically
   * refer to an object of the appropriate type that has been created with
   * <internalformat> and <target>.  If the particular <target> and
   * <internalformat> combination do not make sense, ... the "unsupported"
   * answer should be given. This is not an error.
            /* In the ARB_internalformat_query2 spec wording, some <pnames> do not care
   * about the /resource/ being supported or not, we return 'true' for those.
   */
   switch (pname) {
   case GL_INTERNALFORMAT_SUPPORTED:
   case GL_INTERNALFORMAT_PREFERRED:
   case GL_COLOR_COMPONENTS:
   case GL_DEPTH_COMPONENTS:
   case GL_STENCIL_COMPONENTS:
   case GL_COLOR_RENDERABLE:
   case GL_DEPTH_RENDERABLE:
   case GL_STENCIL_RENDERABLE:
         default:
                  switch(target){
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_RECTANGLE:
      /* Based on what Mesa does for glTexImage1D/2D/3D and
   * glCompressedTexImage1D/2D/3D functions.
   */
   if (_mesa_base_tex_format(ctx, internalformat) < 0)
            /* additional checks for depth textures */
   if (!_mesa_legal_texture_base_format_for_target(ctx, target, internalformat) &&
                  /* additional checks for compressed textures */
   if (_mesa_is_compressed_format(ctx, internalformat) &&
                     case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      /* Based on what Mesa does for glTexImage2D/3DMultisample,
   * glTexStorage2D/3DMultisample and
   * glTextureStorage2D/3DMultisample functions.
   */
   if (!_mesa_is_renderable_texture_format(ctx, internalformat))
               case GL_TEXTURE_BUFFER:
      /* Based on what Mesa does for the glTexBuffer function. */
   if (_mesa_validate_texbuffer_format(ctx, internalformat) ==
                     case GL_RENDERBUFFER:
      /* Based on what Mesa does for glRenderbufferStorage(Multisample) and
   * glNamedRenderbufferStorage functions.
   */
   if (!_mesa_base_fbo_format(ctx, internalformat))
               default:
                     }
      static bool
   _is_internalformat_supported(struct gl_context *ctx, GLenum target,
         {
      /* From the ARB_internalformat_query2 specification:
   *
   *     "- INTERNALFORMAT_SUPPORTED: If <internalformat> is an internal format
   *     that is supported by the implementation in at least some subset of
   *     possible operations, TRUE is written to <params>.  If <internalformat>
   *     if not a valid token for any internal format usage, FALSE is returned.
   *
   *     <internalformats> that must be supported (in GL 4.2 or later) include
   *      the following:
   *         - "sized internal formats" from Table 3.12, 3.13, and 3.15,
   *         - any specific "compressed internal format" from Table 3.14,
   *         - any "image unit format" from Table 3.21.
   *         - any generic "compressed internal format" from Table 3.14, if the
   *         implementation accepts it for any texture specification commands, and
   *         - unsized or base internal format, if the implementation accepts
   *         it for texture or image specification.
   *
   * But also:
   * "If the particualar <target> and <internalformat> combination do not make
   * sense, or if a particular type of <target> is not supported by the
   * implementation the "unsupported" answer should be given. This is not an
   * error.
   */
            if (target == GL_RENDERBUFFER) {
      if (_mesa_base_fbo_format(ctx, internalformat) == 0) {
            } else if (target == GL_TEXTURE_BUFFER) {
      if (_mesa_validate_texbuffer_format(ctx, internalformat) ==
      MESA_FORMAT_NONE) {
         } else {
      if (_mesa_base_tex_format(ctx, internalformat) < 0) {
                     /* Let the driver have the final word */
   st_QueryInternalFormat(ctx, target, internalformat,
               }
      static bool
   _legal_target_for_framebuffer_texture_layer(struct gl_context *ctx,
         {
      switch (target) {
   case GL_TEXTURE_3D:
   case GL_TEXTURE_1D_ARRAY:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_TEXTURE_CUBE_MAP:
         default:
            }
      static GLenum
   _mesa_generic_type_for_internal_format(GLenum internalFormat)
   {
      if (_mesa_is_enum_format_unsigned_int(internalFormat))
         else if (_mesa_is_enum_format_signed_int(internalFormat))
         else
      }
      /* default implementation of QueryInternalFormat driverfunc, for
   * drivers not implementing ARB_internalformat_query2.
   */
   void
   _mesa_query_internal_format_default(struct gl_context *ctx, GLenum target,
               {
               switch (pname) {
   case GL_SAMPLES:
   case GL_NUM_SAMPLE_COUNTS:
      params[0] = 1;
         case GL_INTERNALFORMAT_SUPPORTED:
      params[0] = GL_TRUE;
         case GL_INTERNALFORMAT_PREFERRED:
      params[0] = internalFormat;
         case GL_READ_PIXELS_FORMAT: {
      GLenum base_format = _mesa_base_tex_format(ctx, internalFormat);
   switch (base_format) {
   case GL_STENCIL_INDEX:
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_STENCIL:
   case GL_RED:
   case GL_RGB:
   case GL_BGR:
   case GL_RGBA:
   case GL_BGRA:
      params[0] = base_format;
      default:
      params[0] = GL_NONE;
      }
               case GL_READ_PIXELS_TYPE:
   case GL_TEXTURE_IMAGE_TYPE:
   case GL_GET_TEXTURE_IMAGE_TYPE: {
      GLenum base_format = _mesa_base_tex_format(ctx, internalFormat);
   if (base_format > 0)
         else
                     case GL_TEXTURE_IMAGE_FORMAT:
   case GL_GET_TEXTURE_IMAGE_FORMAT: {
      GLenum format = GL_NONE;
   GLenum base_format = _mesa_base_tex_format(ctx, internalFormat);
   if (base_format > 0) {
      if (_mesa_is_enum_format_integer(internalFormat))
   format = _mesa_base_format_to_integer_format(base_format);
   else
               params[0] = format;
               case GL_MANUAL_GENERATE_MIPMAP:
   case GL_AUTO_GENERATE_MIPMAP:
   case GL_SRGB_READ:
   case GL_SRGB_WRITE:
   case GL_SRGB_DECODE_ARB:
   case GL_VERTEX_TEXTURE:
   case GL_TESS_CONTROL_TEXTURE:
   case GL_TESS_EVALUATION_TEXTURE:
   case GL_GEOMETRY_TEXTURE:
   case GL_FRAGMENT_TEXTURE:
   case GL_COMPUTE_TEXTURE:
   case GL_SHADER_IMAGE_LOAD:
   case GL_SHADER_IMAGE_STORE:
   case GL_SHADER_IMAGE_ATOMIC:
   case GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST:
   case GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST:
   case GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_WRITE:
   case GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_WRITE:
   case GL_CLEAR_BUFFER:
   case GL_CLEAR_TEXTURE:
   case GL_TEXTURE_VIEW:
   case GL_TEXTURE_SHADOW:
   case GL_TEXTURE_GATHER:
   case GL_TEXTURE_GATHER_SHADOW:
   case GL_FRAMEBUFFER_RENDERABLE:
   case GL_FRAMEBUFFER_RENDERABLE_LAYERED:
   case GL_FRAMEBUFFER_BLEND:
   case GL_FILTER:
      /*
   * TODO seems a tad optimistic just saying yes to everything here.
   * Even for combinations which make no sense...
   * And things like TESS_CONTROL_TEXTURE should definitely default to
   * NONE if the driver doesn't even support tessellation...
   */
   params[0] = GL_FULL_SUPPORT;
      case GL_NUM_TILING_TYPES_EXT:
      params[0] = 2;
   if (_mesa_has_MESA_texture_const_bandwidth(ctx))
            case GL_TILING_TYPES_EXT:
      params[0] = GL_OPTIMAL_TILING_EXT;
   params[1] = GL_LINEAR_TILING_EXT;
   if (_mesa_has_MESA_texture_const_bandwidth(ctx))
               default:
      _set_default_response(pname, params);
         }
      /*
   * For MAX_WIDTH/MAX_HEIGHT/MAX_DEPTH it returns the equivalent GetInteger
   * pname for a Getinternalformat pname/target combination. target/pname
   * combinations that would return 0 due dimension number or unsupported status
   * should be already filtered out
   *
   * Note that this means that the returned value would be independent of the
   * internalformat. This possibility is already mentioned at the Issue 7 of the
   * arb_internalformat_query2 spec.
   */
   static GLenum
   _equivalent_size_pname(GLenum target,
         {
      switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_MULTISAMPLE:
         case GL_TEXTURE_3D:
         case GL_TEXTURE_CUBE_MAP:
         case GL_TEXTURE_RECTANGLE:
         case GL_RENDERBUFFER:
         case GL_TEXTURE_1D_ARRAY:
      if (pname == GL_MAX_HEIGHT)
         else
      case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      if (pname == GL_MAX_DEPTH)
         else
      case GL_TEXTURE_CUBE_MAP_ARRAY:
      if (pname == GL_MAX_DEPTH)
         else
      case GL_TEXTURE_BUFFER:
         default:
            }
      /*
   * Returns the dimensions associated to a target. GL_TEXTURE_BUFFER and
   * GL_RENDERBUFFER have associated a dimension, but they are not textures
   * per-se, so we can't just call _mesa_get_texture_dimension directly.
   */
   static GLint
   _get_target_dimensions(GLenum target)
   {
      switch(target) {
   case GL_TEXTURE_BUFFER:
         case GL_RENDERBUFFER:
         default:
            }
      /*
   * Returns the minimum amount of dimensions associated to a pname. So for
   * example, if querying GL_MAX_HEIGHT, it is assumed that your target would
   * have as minimum 2 dimensions.
   *
   * Useful to handle sentences like this from query2 spec:
   *
   * "MAX_HEIGHT:
   *  <skip>
   *  If the resource does not have at least two dimensions
   *  <skip>."
   */
   static GLint
   _get_min_dimensions(GLenum pname)
   {
      switch(pname) {
   case GL_MAX_WIDTH:
         case GL_MAX_HEIGHT:
         case GL_MAX_DEPTH:
         default:
            }
      static bool
   _is_generic_compressed_format(const struct gl_context *ctx,
         {
      switch (intFormat) {
   case GL_COMPRESSED_SRGB:
   case GL_COMPRESSED_SRGB_ALPHA:
   case GL_COMPRESSED_SLUMINANCE:
   case GL_COMPRESSED_SLUMINANCE_ALPHA:
         case GL_COMPRESSED_RG:
   case GL_COMPRESSED_RED:
      return _mesa_is_gles(ctx) ?
            case GL_COMPRESSED_RGB:
   case GL_COMPRESSED_RGBA:
         default:
            }
      /*
   * Similar to teximage.c:check_multisample_target, but independent of the
   * dimensions.
   */
   bool
   _mesa_is_multisample_target(GLenum target)
   {
      switch(target) {
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
         default:
               }
      void GLAPIENTRY
   _mesa_GetInternalformativ(GLenum target, GLenum internalformat, GLenum pname,
         {
      GLint buffer[16];
                     /* ARB_internalformat_query is also mandatory for ARB_internalformat_query2 */
   if (!(_mesa_has_ARB_internalformat_query(ctx) ||
            _mesa_error(ctx, GL_INVALID_OPERATION, "glGetInternalformativ");
               if (!_legal_parameters(ctx, target, internalformat, pname, bufSize, params))
            /* initialize the contents of the temporary buffer */
            /* Use the 'unsupported' response defined by the spec for every pname
   * as the default answer.
   */
            if (!_is_target_supported(ctx, target) ||
      !_is_internalformat_supported(ctx, target, internalformat) ||
   !_is_resource_supported(ctx, target, internalformat, pname))
         switch (pname) {
   case GL_SAMPLES:
         case GL_NUM_SAMPLE_COUNTS:
      /* The ARB_internalformat_query2 sets the response as 'unsupported' for
   * SAMPLES and NUM_SAMPLE_COUNTS:
   *
   *     "If <internalformat> is not color-renderable, depth-renderable, or
   *     stencil-renderable (as defined in section 4.4.4), or if <target>
   *     does not support multiple samples (ie other than
   *     TEXTURE_2D_MULTISAMPLE,  TEXTURE_2D_MULTISAMPLE_ARRAY,
   *     or RENDERBUFFER)."
   */
   if ((target != GL_RENDERBUFFER &&
      target != GL_TEXTURE_2D_MULTISAMPLE &&
   target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY) ||
               /* The GL ES 3.0 specification, section 6.1.15 page 236 says:
   *
   *     "Since multisampling is not supported for signed and unsigned
   *     integer internal formats, the value of NUM_SAMPLE_COUNTS will be
   *     zero for such formats.
   *
   * Since OpenGL ES 3.1 adds support for multisampled integer formats, we
   * have to check the version for 30 exactly.
   */
   if (pname == GL_NUM_SAMPLE_COUNTS && _mesa_is_gles2(ctx) &&
      ctx->Version == 30 && _mesa_is_enum_format_integer(internalformat)) {
               st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_INTERNALFORMAT_SUPPORTED:
      /* Having a supported <internalformat> is implemented as a prerequisite
   * for all the <pnames>. Thus,  if we reach this point, the internalformat is
   * supported.
   */
   buffer[0] = GL_TRUE;
         case GL_INTERNALFORMAT_PREFERRED:
      /* The ARB_internalformat_query2 spec says:
   *
   *     "- INTERNALFORMAT_PREFERRED: The implementation-preferred internal
   *     format for representing resources of the specified <internalformat> is
   *     returned in <params>.
   *
   * Therefore, we let the driver answer. Note that if we reach this
   * point, it means that the internalformat is supported, so the driver
   * is called just to try to get a preferred format. If not supported,
   * GL_NONE was already returned and the driver is not called.
   */
   st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_INTERNALFORMAT_RED_SIZE:
   case GL_INTERNALFORMAT_GREEN_SIZE:
   case GL_INTERNALFORMAT_BLUE_SIZE:
   case GL_INTERNALFORMAT_ALPHA_SIZE:
   case GL_INTERNALFORMAT_DEPTH_SIZE:
   case GL_INTERNALFORMAT_STENCIL_SIZE:
   case GL_INTERNALFORMAT_SHARED_SIZE:
   case GL_INTERNALFORMAT_RED_TYPE:
   case GL_INTERNALFORMAT_GREEN_TYPE:
   case GL_INTERNALFORMAT_BLUE_TYPE:
   case GL_INTERNALFORMAT_ALPHA_TYPE:
   case GL_INTERNALFORMAT_DEPTH_TYPE:
   case GL_INTERNALFORMAT_STENCIL_TYPE: {
      GLint baseformat;
            if (target != GL_RENDERBUFFER) {
         } else {
                  /* Let the driver choose the texture format.
   *
   * Disclaimer: I am considering that drivers use for renderbuffers the
   * same format-choice logic as for textures.
   */
   texformat = st_ChooseTextureFormat(ctx, target, internalformat,
            if (texformat == MESA_FORMAT_NONE || baseformat <= 0)
            /* Implementation based on what Mesa does for glGetTexLevelParameteriv
   * and glGetRenderbufferParameteriv functions.
   */
   if (pname == GL_INTERNALFORMAT_SHARED_SIZE) {
      if (texformat == MESA_FORMAT_R9G9B9E5_FLOAT) {
         }
               if (!_mesa_base_format_has_channel(baseformat, pname))
            switch (pname) {
   case GL_INTERNALFORMAT_DEPTH_SIZE:
      if (!_mesa_is_desktop_gl(ctx) &&
      target != GL_RENDERBUFFER &&
   target != GL_TEXTURE_BUFFER)
         case GL_INTERNALFORMAT_RED_SIZE:
   case GL_INTERNALFORMAT_GREEN_SIZE:
   case GL_INTERNALFORMAT_BLUE_SIZE:
   case GL_INTERNALFORMAT_ALPHA_SIZE:
   case GL_INTERNALFORMAT_STENCIL_SIZE:
                  case GL_INTERNALFORMAT_DEPTH_TYPE:
      if (!_mesa_has_ARB_texture_float(ctx))
            case GL_INTERNALFORMAT_RED_TYPE:
   case GL_INTERNALFORMAT_GREEN_TYPE:
   case GL_INTERNALFORMAT_BLUE_TYPE:
   case GL_INTERNALFORMAT_ALPHA_TYPE:
   case GL_INTERNALFORMAT_STENCIL_TYPE:
                  default:
            }
                  /* For WIDTH/HEIGHT/DEPTH/LAYERS there is no reason to think that the
   * returned values should be different to the values returned by
      case GL_MAX_WIDTH:
   case GL_MAX_HEIGHT:
   case GL_MAX_DEPTH: {
      GLenum get_pname;
   GLint dimensions;
            /* From query2:MAX_HEIGHT spec (as example):
   *
   * "If the resource does not have at least two dimensions, or if the
   * resource is unsupported, zero is returned."
   */
   dimensions = _get_target_dimensions(target);
   min_dimensions = _get_min_dimensions(pname);
   if (dimensions < min_dimensions)
            get_pname = _equivalent_size_pname(target, pname);
   if (get_pname == 0)
            _mesa_GetIntegerv(get_pname, buffer);
               case GL_MAX_LAYERS:
      if (!_mesa_has_EXT_texture_array(ctx))
            if (!_mesa_is_array_texture(target))
            _mesa_GetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, buffer);
         case GL_MAX_COMBINED_DIMENSIONS:{
      GLint64 combined_value = 1;
   GLenum max_dimensions_pnames[] = {
      GL_MAX_WIDTH,
   GL_MAX_HEIGHT,
   GL_MAX_DEPTH,
      };
   unsigned i;
            /* Combining the dimensions. Note that for array targets, this would
   * automatically include the value of MAX_LAYERS, as that value is
   * returned as MAX_HEIGHT or MAX_DEPTH */
   for (i = 0; i < 4; i++) {
      if (max_dimensions_pnames[i] == GL_SAMPLES &&
                  _mesa_GetInternalformativ(target, internalformat,
                  if (current_value != 0)
               if (_mesa_is_cube_map_texture(target))
            /* We pack the 64-bit value on two 32-bit values. Calling the 32-bit
   * query, this would work as far as the value can be hold on a 32-bit
   * signed integer. For the 64-bit query, the wrapper around the 32-bit
   * query will unpack the value */
   memcpy(buffer, &combined_value, sizeof(GLint64));
               case GL_COLOR_COMPONENTS:
      /* The ARB_internalformat_query2 spec says:
   *
   *     "- COLOR_COMPONENTS: If the internal format contains any color
   *     components (R, G, B, or A), TRUE is returned in <params>.
   *     If the internal format is unsupported or contains no color
   *     components, FALSE is returned."
   */
   if (_mesa_is_color_format(internalformat))
               case GL_DEPTH_COMPONENTS:
      /* The ARB_internalformat_query2 spec says:
   *
   *     "- DEPTH_COMPONENTS: If the internal format contains a depth
   *     component (D), TRUE is returned in <params>. If the internal format
   *     is unsupported or contains no depth component, FALSE is returned."
   */
   if (_mesa_is_depth_format(internalformat) ||
      _mesa_is_depthstencil_format(internalformat))
            case GL_STENCIL_COMPONENTS:
      /* The ARB_internalformat_query2 spec says:
   *
   *     "- STENCIL_COMPONENTS: If the internal format contains a stencil
   *     component (S), TRUE is returned in <params>. If the internal format
   *     is unsupported or contains no stencil component, FALSE is returned.
   */
   if (_mesa_is_stencil_format(internalformat) ||
      _mesa_is_depthstencil_format(internalformat))
            case GL_COLOR_RENDERABLE:
   case GL_DEPTH_RENDERABLE:
   case GL_STENCIL_RENDERABLE:
      if (!_is_renderable(ctx, internalformat))
            if (pname == GL_COLOR_RENDERABLE) {
      if (!_mesa_is_color_format(internalformat))
      } else {
      GLenum baseFormat = _mesa_base_fbo_format(ctx, internalformat);
   if (baseFormat != GL_DEPTH_STENCIL &&
      ((pname == GL_DEPTH_RENDERABLE && baseFormat != GL_DEPTH_COMPONENT) ||
   (pname == GL_STENCIL_RENDERABLE && baseFormat != GL_STENCIL_INDEX)))
            buffer[0] = GL_TRUE;
         case GL_FRAMEBUFFER_RENDERABLE_LAYERED:
      if (!_mesa_has_EXT_texture_array(ctx) ||
      _legal_target_for_framebuffer_texture_layer(ctx, target))
         case GL_FRAMEBUFFER_RENDERABLE:
   case GL_FRAMEBUFFER_BLEND:
      if (!_mesa_has_ARB_framebuffer_object(ctx))
            if (target == GL_TEXTURE_BUFFER ||
                  st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_READ_PIXELS:
   case GL_READ_PIXELS_FORMAT:
   case GL_READ_PIXELS_TYPE:
      st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_TEXTURE_IMAGE_FORMAT:
   case GL_GET_TEXTURE_IMAGE_FORMAT:
   case GL_TEXTURE_IMAGE_TYPE:
   case GL_GET_TEXTURE_IMAGE_TYPE:
      st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_MIPMAP:
   case GL_MANUAL_GENERATE_MIPMAP:
   case GL_AUTO_GENERATE_MIPMAP:
      if (!_mesa_is_valid_generate_texture_mipmap_target(ctx, target) ||
      !_mesa_is_valid_generate_texture_mipmap_internalformat(ctx,
                     if (pname == GL_MIPMAP) {
      buffer[0] = GL_TRUE;
      }
   else if (pname == GL_MANUAL_GENERATE_MIPMAP) {
      if (!_mesa_has_ARB_framebuffer_object(ctx))
      }
   else {
      /* From ARB_internalformat_query2:
   *    "Dependencies on OpenGL 3.2 (Core Profile)
   *     In core profiles for OpenGL 3.2 and later versions, queries
   *     for the AUTO_GENERATE_MIPMAP <pname> return the appropriate
   *     unsupported response."
   */
   if (_mesa_is_desktop_gl(ctx) && ctx->Version >= 32)
               st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_COLOR_ENCODING:
      if (!_mesa_is_color_format(internalformat))
            if (_mesa_is_srgb_format(internalformat))
         else
               case GL_SRGB_READ:
      if (!_mesa_has_EXT_texture_sRGB(ctx) ||
      !_mesa_is_srgb_format(internalformat)) {
               st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_SRGB_WRITE:
      if (!ctx->Extensions.EXT_sRGB ||
      !_mesa_is_color_format(internalformat)) {
               st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_SRGB_DECODE_ARB:
      /* Presence of EXT_texture_sRGB_decode was already verified */
   if (!_mesa_has_EXT_texture_sRGB(ctx) ||
      target == GL_RENDERBUFFER ||
   !_mesa_is_srgb_format(internalformat)) {
               st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_FILTER:
      /* If it doesn't allow to set sampler parameters then it would not allow
   * to set a filter different to GL_NEAREST. In practice, this method
   * only filters out MULTISAMPLE/MULTISAMPLE_ARRAY */
   if (!_mesa_target_allows_setting_sampler_parameters(target))
            if (_mesa_is_enum_format_integer(internalformat))
            if (target == GL_TEXTURE_BUFFER)
            /* At this point we know that multi-texel filtering is supported. We
   * need to call the driver to know if it is CAVEAT_SUPPORT or
   * FULL_SUPPORT.
   */
   st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_VERTEX_TEXTURE:
   case GL_TESS_CONTROL_TEXTURE:
   case GL_TESS_EVALUATION_TEXTURE:
   case GL_GEOMETRY_TEXTURE:
   case GL_FRAGMENT_TEXTURE:
   case GL_COMPUTE_TEXTURE:
      if (target == GL_RENDERBUFFER)
            if ((pname == GL_TESS_CONTROL_TEXTURE ||
      pname == GL_TESS_EVALUATION_TEXTURE) &&
               if (pname == GL_GEOMETRY_TEXTURE && !_mesa_has_geometry_shaders(ctx))
            if (pname == GL_COMPUTE_TEXTURE && !_mesa_has_compute_shaders(ctx))
            st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_TEXTURE_GATHER:
   case GL_TEXTURE_GATHER_SHADOW:
      if (!_mesa_has_ARB_texture_gather(ctx))
               case GL_TEXTURE_SHADOW:
      /* Only depth or depth-stencil image formats make sense in shadow
         if (pname != GL_TEXTURE_GATHER &&
      !_mesa_is_depth_format(internalformat) &&
               /* Validate the target for shadow and gather operations */
   switch (target) {
   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_RECTANGLE:
            case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY:
      /* 1D and 1DArray textures are not admitted in gather operations */
   if (pname != GL_TEXTURE_SHADOW)
               default:
                  st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_SHADER_IMAGE_LOAD:
   case GL_SHADER_IMAGE_STORE:
      if (!_mesa_has_ARB_shader_image_load_store(ctx))
            /* We call to _mesa_is_shader_image_format_supported
   * using "internalformat" as parameter, because the
   * the ARB_internalformat_query2 spec says:
   * "In this case the <internalformat> is the value of the <format>
   * parameter that is passed to BindImageTexture."
   */
   if (target == GL_RENDERBUFFER ||
                  st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_SHADER_IMAGE_ATOMIC:
      if (!_mesa_has_ARB_shader_image_load_store(ctx))
            st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_IMAGE_TEXEL_SIZE: {
               if (!_mesa_has_ARB_shader_image_load_store(ctx) ||
                  image_format = _mesa_get_shader_image_format(internalformat);
   if (image_format == MESA_FORMAT_NONE)
            /* We return bits */
   buffer[0] = (_mesa_get_format_bytes(image_format) * 8);
               case GL_IMAGE_COMPATIBILITY_CLASS:
      if (!_mesa_has_ARB_shader_image_load_store(ctx) ||
                  buffer[0] = _mesa_get_image_format_class(internalformat);
         case GL_IMAGE_PIXEL_FORMAT: {
               if (!_mesa_has_ARB_shader_image_load_store(ctx) ||
      target == GL_RENDERBUFFER ||
               base_format = _mesa_base_tex_format(ctx, internalformat);
   if (base_format == -1)
            if (_mesa_is_enum_format_integer(internalformat))
         else
                     case GL_IMAGE_PIXEL_TYPE: {
      mesa_format image_format;
   GLenum datatype;
            if (!_mesa_has_ARB_shader_image_load_store(ctx) ||
                  image_format = _mesa_get_shader_image_format(internalformat);
   if (image_format == MESA_FORMAT_NONE)
            _mesa_uncompressed_format_to_type_and_comps(image_format, &datatype,
         if (!datatype)
            buffer[0] = datatype;
               case GL_IMAGE_FORMAT_COMPATIBILITY_TYPE: {
      if (!_mesa_has_ARB_shader_image_load_store(ctx))
            /* As pointed by the spec quote below, this pname query should return
   * the same value that GetTexParameter. So if the target is not valid
   * for GetTexParameter we return the unsupported value. The check below
   * is the same target check used by GetTexParameter.
   */
   int targetIndex = _mesa_tex_target_to_index(ctx, target);
   if (targetIndex < 0 || targetIndex == TEXTURE_BUFFER_INDEX)
            /* From spec: "Equivalent to calling GetTexParameter with <value> set
   * to IMAGE_FORMAT_COMPATIBILITY_TYPE."
   *
   * GetTexParameter just returns
   * tex_obj->ImageFormatCompatibilityType. We create a fake tex_obj
   * just with the purpose of getting the value.
   */
   struct gl_texture_object *tex_obj = _mesa_new_texture_object(ctx, 0, target);
   buffer[0] = tex_obj->Attrib.ImageFormatCompatibilityType;
                        case GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST:
   case GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST:
   case GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_WRITE:
   case GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_WRITE:
      if (target == GL_RENDERBUFFER)
            if (!_mesa_is_depthstencil_format(internalformat)) {
      if (((pname == GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST ||
            !_mesa_is_depth_format(internalformat)) ||
   ((pname == GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST ||
         !_mesa_is_stencil_format(internalformat)))
            st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_TEXTURE_COMPRESSED:
      buffer[0] = _mesa_is_compressed_format(ctx, internalformat);
         case GL_TEXTURE_COMPRESSED_BLOCK_WIDTH:
   case GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT:
   case GL_TEXTURE_COMPRESSED_BLOCK_SIZE: {
      mesa_format mesaformat;
            mesaformat = _mesa_glenum_to_compressed_format(internalformat);
   if (mesaformat == MESA_FORMAT_NONE)
            block_size = _mesa_get_format_bytes(mesaformat);
            if (pname == GL_TEXTURE_COMPRESSED_BLOCK_SIZE) {
         } else {
               /* Returns the width and height in pixels. We return bytes */
                  if (pname == GL_TEXTURE_COMPRESSED_BLOCK_WIDTH)
         else
      }
               case GL_CLEAR_BUFFER:
      if (target != GL_TEXTURE_BUFFER)
            st_QueryInternalFormat(ctx, target, internalformat, pname,
            case GL_CLEAR_TEXTURE: {
      if (target == GL_TEXTURE_BUFFER ||
                  if (_mesa_is_compressed_format(ctx, internalformat) ||
                  st_QueryInternalFormat(ctx, target, internalformat, pname,
                     case GL_TEXTURE_VIEW:
   case GL_VIEW_COMPATIBILITY_CLASS:
      if (!_mesa_has_ARB_texture_view(ctx) ||
      target == GL_TEXTURE_BUFFER ||
               if (pname == GL_TEXTURE_VIEW) {
      st_QueryInternalFormat(ctx, target, internalformat, pname,
      } else {
      GLenum view_class = _mesa_texture_view_lookup_view_class(ctx,
                           }
         case GL_NUM_TILING_TYPES_EXT:
   case GL_TILING_TYPES_EXT:
      st_QueryInternalFormat(ctx, target, internalformat, pname,
               case GL_TEXTURE_REDUCTION_MODE_ARB:
      if (ctx->Extensions.EXT_texture_filter_minmax)
         else if (ctx->Extensions.ARB_texture_filter_minmax)
      st_QueryInternalFormat(ctx, target, internalformat, pname,
      else
               case GL_NUM_VIRTUAL_PAGE_SIZES_ARB:
   case GL_VIRTUAL_PAGE_SIZE_X_ARB:
   case GL_VIRTUAL_PAGE_SIZE_Y_ARB:
   case GL_VIRTUAL_PAGE_SIZE_Z_ARB:
      st_QueryInternalFormat(ctx, target, internalformat, pname, buffer);
         default:
               end:
      if (bufSize != 0 && params == NULL) {
      /* Emit a warning to aid application debugging, but go ahead and do the
   * memcpy (and probably crash) anyway.
   */
   _mesa_warning(ctx,
                     /* Copy the data from the temporary buffer to the buffer supplied by the
   * application.  Clamp the size of the copy to the size supplied by the
   * application.
   */
               }
      void GLAPIENTRY
   _mesa_GetInternalformati64v(GLenum target, GLenum internalformat,
         {
      GLint params32[16];
   unsigned i;
   GLsizei realSize = MIN2(bufSize, 16);
                              if (!_mesa_has_ARB_internalformat_query2(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetInternalformati64v");
               /* For SAMPLES there are cases where params needs to remain unmodified. As
   * no pname can return a negative value, we fill params32 with negative
   * values as reference values, that can be used to know what copy-back to
   * params */
   for (i = 0; i < realSize; i++)
            /* For GL_MAX_COMBINED_DIMENSIONS we need to get back 2 32-bit integers,
   * and at the same time we only need 2. So for that pname, we call the
   * 32-bit query with bufSize 2, except on the case of bufSize 0, that is
   * basically like asking to not get the value, but that is a caller
   * problem. */
   if (pname == GL_MAX_COMBINED_DIMENSIONS && bufSize > 0)
         else
                     if (pname == GL_MAX_COMBINED_DIMENSIONS) {
         } else {
      for (i = 0; i < realSize; i++) {
      /* We only copy back the values that changed */
   if (params32[i] < 0)
                  }
