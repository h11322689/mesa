   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
   * \file texenv.c
   *
   * glTexEnv-related functions
   */
         #include "util/glheader.h"
   #include "main/context.h"
   #include "main/blend.h"
   #include "main/enums.h"
   #include "main/macros.h"
   #include "main/mtypes.h"
   #include "main/state.h"
   #include "main/texstate.h"
   #include "api_exec_decl.h"
         #define TE_ERROR(errCode, msg, value)				\
               /** Set texture env mode */
   static void
   set_env_mode(struct gl_context *ctx,
               {
               if (texUnit->EnvMode == mode)
            switch (mode) {
   case GL_MODULATE:
   case GL_BLEND:
   case GL_DECAL:
   case GL_REPLACE:
   case GL_ADD:
   case GL_COMBINE:
      legal = GL_TRUE;
      case GL_REPLACE_EXT:
      mode = GL_REPLACE; /* GL_REPLACE_EXT != GL_REPLACE */
   legal = GL_TRUE;
      case GL_COMBINE4_NV:
      legal = ctx->Extensions.NV_texture_env_combine4;
      default:
                  if (legal) {
      FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE, GL_TEXTURE_BIT);
      }
   else {
            }
         static void
   set_env_color(struct gl_context *ctx,
               {
      if (TEST_EQ_4V(color, texUnit->EnvColorUnclamped))
         FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE, GL_TEXTURE_BIT);
   COPY_4FV(texUnit->EnvColorUnclamped, color);
   texUnit->EnvColor[0] = CLAMP(color[0], 0.0F, 1.0F);
   texUnit->EnvColor[1] = CLAMP(color[1], 0.0F, 1.0F);
   texUnit->EnvColor[2] = CLAMP(color[2], 0.0F, 1.0F);
      }
         /** Set an RGB or A combiner mode/function */
   static bool
   set_combiner_mode(struct gl_context *ctx,
               {
               switch (mode) {
   case GL_REPLACE:
   case GL_MODULATE:
   case GL_ADD:
   case GL_ADD_SIGNED:
   case GL_INTERPOLATE:
   case GL_SUBTRACT:
      legal = GL_TRUE;
      case GL_DOT3_RGB_EXT:
   case GL_DOT3_RGBA_EXT:
      legal = (_mesa_is_desktop_gl_compat(ctx) &&
                  case GL_DOT3_RGB:
   case GL_DOT3_RGBA:
      legal = (pname == GL_COMBINE_RGB);
      case GL_MODULATE_ADD_ATI:
   case GL_MODULATE_SIGNED_ADD_ATI:
   case GL_MODULATE_SUBTRACT_ATI:
      legal = (_mesa_is_desktop_gl_compat(ctx) &&
            default:
                  if (!legal) {
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
               switch (pname) {
   case GL_COMBINE_RGB:
      if (texUnit->Combine.ModeRGB == mode)
         FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE, GL_TEXTURE_BIT);
   texUnit->Combine.ModeRGB = mode;
         case GL_COMBINE_ALPHA:
      if (texUnit->Combine.ModeA == mode)
         FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE, GL_TEXTURE_BIT);
   texUnit->Combine.ModeA = mode;
      default:
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
                  }
            /** Set an RGB or A combiner source term */
   static bool
   set_combiner_source(struct gl_context *ctx,
               {
      GLuint term;
            /*
   * Translate pname to (term, alpha).
   *
   * The enums were given sequential values for a reason.
   */
   switch (pname) {
   case GL_SOURCE0_RGB:
   case GL_SOURCE1_RGB:
   case GL_SOURCE2_RGB:
   case GL_SOURCE3_RGB_NV:
      term = pname - GL_SOURCE0_RGB;
   alpha = GL_FALSE;
      case GL_SOURCE0_ALPHA:
   case GL_SOURCE1_ALPHA:
   case GL_SOURCE2_ALPHA:
   case GL_SOURCE3_ALPHA_NV:
      term = pname - GL_SOURCE0_ALPHA;
   alpha = GL_TRUE;
      default:
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
               if ((term == 3) && (ctx->API != API_OPENGL_COMPAT
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
                        /*
   * Error-check param (the source term)
   */
   switch (param) {
   case GL_TEXTURE:
   case GL_CONSTANT:
   case GL_PRIMARY_COLOR:
   case GL_PREVIOUS:
      legal = GL_TRUE;
      case GL_TEXTURE0:
   case GL_TEXTURE1:
   case GL_TEXTURE2:
   case GL_TEXTURE3:
   case GL_TEXTURE4:
   case GL_TEXTURE5:
   case GL_TEXTURE6:
   case GL_TEXTURE7:
      legal = (param - GL_TEXTURE0 < ctx->Const.MaxTextureUnits);
      case GL_ZERO:
      legal = (_mesa_is_desktop_gl_compat(ctx) &&
                  case GL_ONE:
      legal = (_mesa_is_desktop_gl_compat(ctx) &&
            default:
                  if (!legal) {
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", param);
                        if (alpha)
         else
               }
         /** Set an RGB or A combiner operand term */
   static bool
   set_combiner_operand(struct gl_context *ctx,
               {
      GLuint term;
            /* The enums were given sequential values for a reason.
   */
   switch (pname) {
   case GL_OPERAND0_RGB:
   case GL_OPERAND1_RGB:
   case GL_OPERAND2_RGB:
   case GL_OPERAND3_RGB_NV:
      term = pname - GL_OPERAND0_RGB;
   alpha = GL_FALSE;
      case GL_OPERAND0_ALPHA:
   case GL_OPERAND1_ALPHA:
   case GL_OPERAND2_ALPHA:
   case GL_OPERAND3_ALPHA_NV:
      term = pname - GL_OPERAND0_ALPHA;
   alpha = GL_TRUE;
      default:
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
               if ((term == 3) && (ctx->API != API_OPENGL_COMPAT
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
                        /*
   * Error-check param (the source operand)
   */
   switch (param) {
   case GL_SRC_COLOR:
   case GL_ONE_MINUS_SRC_COLOR:
      legal = !alpha;
      case GL_ONE_MINUS_SRC_ALPHA:
   case GL_SRC_ALPHA:
      legal = GL_TRUE;
      default:
                  if (!legal) {
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", param);
                        if (alpha)
         else
               }
         static bool
   set_combiner_scale(struct gl_context *ctx,
               {
               if (scale == 1.0F) {
         }
   else if (scale == 2.0F) {
         }
   else if (scale == 4.0F) {
         }
   else {
      _mesa_error( ctx, GL_INVALID_VALUE,
                     switch (pname) {
   case GL_RGB_SCALE:
      if (texUnit->Combine.ScaleShiftRGB == shift)
         FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE, GL_TEXTURE_BIT);
   texUnit->Combine.ScaleShiftRGB = shift;
      case GL_ALPHA_SCALE:
      if (texUnit->Combine.ScaleShiftA == shift)
         FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE, GL_TEXTURE_BIT);
   texUnit->Combine.ScaleShiftA = shift;
      default:
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
                  }
         static void
   _mesa_texenvfv_indexed( struct gl_context* ctx, GLuint texunit, GLenum target,
         {
      const GLint iparam0 = (GLint) param[0];
            maxUnit = (target == GL_POINT_SPRITE && pname == GL_COORD_REPLACE)
         if (texunit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexEnvfv(texunit=%d)", texunit);
               if (target == GL_TEXTURE_ENV) {
      struct gl_fixedfunc_texture_unit *texUnit =
            /* The GL spec says that we should report an error if the unit is greater
   * than GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, but in practice, only
   * fixed-function units are usable. This is probably a spec bug.
   * Ignore glTexEnv(GL_TEXTURE_ENV) calls for non-fixed-func units,
   * because we don't want to process calls that have no effect.
   */
   if (!texUnit)
            switch (pname) {
   case GL_TEXTURE_ENV_MODE:
      set_env_mode(ctx, texUnit, (GLenum) iparam0);
      case GL_TEXTURE_ENV_COLOR:
      set_env_color(ctx, texUnit, param);
      case GL_COMBINE_RGB:
   case GL_COMBINE_ALPHA:
         break;
         case GL_SOURCE0_RGB:
   case GL_SOURCE1_RGB:
   case GL_SOURCE2_RGB:
   case GL_SOURCE3_RGB_NV:
   case GL_SOURCE0_ALPHA:
   case GL_SOURCE1_ALPHA:
   case GL_SOURCE2_ALPHA:
   case GL_SOURCE3_ALPHA_NV:
         break;
         case GL_OPERAND0_RGB:
   case GL_OPERAND1_RGB:
   case GL_OPERAND2_RGB:
   case GL_OPERAND3_RGB_NV:
   case GL_OPERAND0_ALPHA:
   case GL_OPERAND1_ALPHA:
   case GL_OPERAND2_ALPHA:
   case GL_OPERAND3_ALPHA_NV:
         break;
         case GL_RGB_SCALE:
   case GL_ALPHA_SCALE:
         break;
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname)" );
   return;
            }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
      struct gl_texture_unit *texUnit =
            if (texUnit->LodBias == param[0])
         FLUSH_VERTICES(ctx, _NEW_TEXTURE_OBJECT, GL_TEXTURE_BIT);
            texUnit->LodBias = param[0];
      }
   else {
   return;
            }
   else if (target == GL_POINT_SPRITE) {
      if (pname == GL_COORD_REPLACE) {
      /* It's kind of weird to set point state via glTexEnv,
   * but that's what the spec calls for.
   */
   if (iparam0 == GL_TRUE) {
      if (ctx->Point.CoordReplace & (1u << texunit))
         FLUSH_VERTICES(ctx, _NEW_POINT | _NEW_FF_VERT_PROGRAM,
            } else if (iparam0 == GL_FALSE) {
      if (~(ctx->Point.CoordReplace) & (1u << texunit))
         FLUSH_VERTICES(ctx, _NEW_POINT | _NEW_FF_VERT_PROGRAM,
            } else {
      _mesa_error( ctx, GL_INVALID_VALUE, "glTexEnv(param=0x%x)", iparam0);
         }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname=0x%x)", pname );
         }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexEnv(target=%s)",
                     if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexEnv %s %s %.1f(%s) ...\n",
               _mesa_enum_to_string(target),
   }
         void GLAPIENTRY
   _mesa_TexEnvfv( GLenum target, GLenum pname, const GLfloat *param )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_TexEnvf( GLenum target, GLenum pname, GLfloat param )
   {
      GLfloat p[4];
   p[0] = param;
   p[1] = p[2] = p[3] = 0.0;
      }
         void GLAPIENTRY
   _mesa_TexEnvi( GLenum target, GLenum pname, GLint param )
   {
      GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0;
      }
         void GLAPIENTRY
   _mesa_TexEnviv( GLenum target, GLenum pname, const GLint *param )
   {
      GLfloat p[4];
   if (pname == GL_TEXTURE_ENV_COLOR) {
      p[0] = INT_TO_FLOAT( param[0] );
   p[1] = INT_TO_FLOAT( param[1] );
   p[2] = INT_TO_FLOAT( param[2] );
      }
   else {
      p[0] = (GLfloat) param[0];
      }
      }
         void GLAPIENTRY
   _mesa_MultiTexEnvfEXT( GLenum texunit, GLenum target,
         {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat p[4];
   p[0] = param;
   p[1] = p[2] = p[3] = 0.0;
      }
      void GLAPIENTRY
   _mesa_MultiTexEnvfvEXT( GLenum texunit, GLenum target,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_MultiTexEnviEXT( GLenum texunit, GLenum target,
         {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0;
      }
         void GLAPIENTRY
   _mesa_MultiTexEnvivEXT( GLenum texunit, GLenum target,
         {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat p[4];
   if (pname == GL_TEXTURE_ENV_COLOR) {
      p[0] = INT_TO_FLOAT( param[0] );
   p[1] = INT_TO_FLOAT( param[1] );
   p[2] = INT_TO_FLOAT( param[2] );
      }
   else {
      p[0] = (GLfloat) param[0];
      }
      }
               /**
   * Helper for glGetTexEnvi/f()
   * \return  value of queried pname or -1 if error.
   */
   static GLint
   get_texenvi(struct gl_context *ctx,
               {
      switch (pname) {
   case GL_TEXTURE_ENV_MODE:
      return texUnit->EnvMode;
      case GL_COMBINE_RGB:
         case GL_COMBINE_ALPHA:
         case GL_SOURCE0_RGB:
   case GL_SOURCE1_RGB:
   case GL_SOURCE2_RGB: {
      const unsigned rgb_idx = pname - GL_SOURCE0_RGB;
      }
   case GL_SOURCE3_RGB_NV:
      if (_mesa_is_desktop_gl_compat(ctx) && ctx->Extensions.NV_texture_env_combine4) {
         }
   else {
         }
      case GL_SOURCE0_ALPHA:
   case GL_SOURCE1_ALPHA:
   case GL_SOURCE2_ALPHA: {
      const unsigned alpha_idx = pname - GL_SOURCE0_ALPHA;
      }
   case GL_SOURCE3_ALPHA_NV:
      if (_mesa_is_desktop_gl_compat(ctx) && ctx->Extensions.NV_texture_env_combine4) {
         }
   else {
         }
      case GL_OPERAND0_RGB:
   case GL_OPERAND1_RGB:
   case GL_OPERAND2_RGB: {
      const unsigned op_rgb = pname - GL_OPERAND0_RGB;
      }
   case GL_OPERAND3_RGB_NV:
      if (_mesa_is_desktop_gl_compat(ctx) && ctx->Extensions.NV_texture_env_combine4) {
         }
   else {
         }
      case GL_OPERAND0_ALPHA:
   case GL_OPERAND1_ALPHA:
   case GL_OPERAND2_ALPHA: {
      const unsigned op_alpha = pname - GL_OPERAND0_ALPHA;
      }
   case GL_OPERAND3_ALPHA_NV:
      if (_mesa_is_desktop_gl_compat(ctx) && ctx->Extensions.NV_texture_env_combine4) {
         }
   else {
         }
      case GL_RGB_SCALE:
         case GL_ALPHA_SCALE:
         default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
                  }
         static void
   _mesa_gettexenvfv_indexed( GLuint texunit, GLenum target, GLenum pname, GLfloat *params )
   {
      GLuint maxUnit;
            maxUnit = (target == GL_POINT_SPRITE && pname == GL_COORD_REPLACE)
         if (texunit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexEnvfv(texunit=%d)", texunit);
               if (target == GL_TEXTURE_ENV) {
      struct gl_fixedfunc_texture_unit *texUnit =
            /* The GL spec says that we should report an error if the unit is greater
   * than GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, but in practice, only
   * fixed-function units are usable. This is probably a spec bug.
   * Ignore calls for non-fixed-func units, because we don't process
   * glTexEnv for them either.
   */
   if (!texUnit)
            if (pname == GL_TEXTURE_ENV_COLOR) {
      if (_mesa_get_clamp_fragment_color(ctx, ctx->DrawBuffer))
         else
      }
   else {
      GLint val = get_texenvi(ctx, texUnit, pname);
   if (val >= 0) {
               }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
               if (pname == GL_TEXTURE_LOD_BIAS_EXT) {
         }
   else {
   return;
            }
   else if (target == GL_POINT_SPRITE) {
      if (pname == GL_COORD_REPLACE) {
      if (ctx->Point.CoordReplace & (1u << texunit))
         else
      }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)" );
         }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
         }
         static void
   _mesa_gettexenviv_indexed( GLuint texunit, GLenum target,
         {
      GLuint maxUnit;
            maxUnit = (target == GL_POINT_SPRITE && pname == GL_COORD_REPLACE)
         if (texunit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexEnviv(texunit=%d)",
                     if (target == GL_TEXTURE_ENV) {
      struct gl_fixedfunc_texture_unit *texUnit =
            /* The GL spec says that we should report an error if the unit is greater
   * than GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, but in practice, only
   * fixed-function units are usable. This is probably a spec bug.
   * Ignore calls for non-fixed-func units, because we don't process
   * glTexEnv for them either.
   */
   if (!texUnit)
            if (pname == GL_TEXTURE_ENV_COLOR) {
      params[0] = FLOAT_TO_INT( texUnit->EnvColor[0] );
   params[1] = FLOAT_TO_INT( texUnit->EnvColor[1] );
   params[2] = FLOAT_TO_INT( texUnit->EnvColor[2] );
      }
   else {
      GLint val = get_texenvi(ctx, texUnit, pname);
   if (val >= 0) {
               }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
               if (pname == GL_TEXTURE_LOD_BIAS_EXT) {
         }
   else {
   return;
            }
   else if (target == GL_POINT_SPRITE) {
      if (pname == GL_COORD_REPLACE) {
      if (ctx->Point.CoordReplace & (1u << texunit))
         else
      }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)" );
         }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(target)" );
         }
         void GLAPIENTRY
   _mesa_GetTexEnvfv( GLenum target, GLenum pname, GLfloat *params )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetMultiTexEnvfvEXT( GLenum texunit, GLenum target,
         {
         }
         void GLAPIENTRY
   _mesa_GetTexEnviv( GLenum target, GLenum pname, GLint *params )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetMultiTexEnvivEXT( GLenum texunit, GLenum target,
         {
         }
