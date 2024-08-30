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
   * \file texgen.c
   *
   * glTexGen-related functions
   */
         #include "util/glheader.h"
   #include "main/context.h"
   #include "main/enums.h"
   #include "main/macros.h"
   #include "main/texparam.h"
   #include "main/texstate.h"
   #include "math/m_matrix.h"
   #include "main/texobj.h"
   #include "api_exec_decl.h"
         /**
   * Return texgen state for given coordinate
   */
   static struct gl_texgen *
   get_texgen(struct gl_context *ctx, GLuint texunitIndex, GLenum coord, const char* caller)
   {
      struct gl_fixedfunc_texture_unit* texUnit;
   if (texunitIndex >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(unit=%d)", caller, texunitIndex);
                        if (_mesa_is_gles1(ctx)) {
      return (coord == GL_TEXTURE_GEN_STR_OES)
               switch (coord) {
   case GL_S:
         case GL_T:
         case GL_R:
         case GL_Q:
         default:
            }
         /* Helper for glTexGenfv and glMultiTexGenfvEXT functions */
   static void
   texgenfv( GLuint texunitIndex, GLenum coord, GLenum pname,
         {
      struct gl_texgen *texgen;
            texgen = get_texgen(ctx, texunitIndex, coord, caller);
   if (!texgen) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(coord)", caller);
               struct gl_fixedfunc_texture_unit *unit = &ctx->Texture.FixedFuncUnit[texunitIndex];
            switch (pname) {
   case GL_TEXTURE_GEN_MODE:
      {
      GLenum mode = (GLenum) (GLint) params[0];
   GLbitfield bit = 0x0;
   if (texgen->Mode == mode)
         switch (mode) {
   case GL_OBJECT_LINEAR:
      bit = TEXGEN_OBJ_LINEAR;
      case GL_EYE_LINEAR:
      bit = TEXGEN_EYE_LINEAR;
      case GL_SPHERE_MAP:
      if (coord == GL_S || coord == GL_T)
            case GL_REFLECTION_MAP_NV:
      if (coord != GL_Q)
            case GL_NORMAL_MAP_NV:
      if (coord != GL_Q)
            default:
         }
   if (!bit) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
      }
   if (ctx->API != API_OPENGL_COMPAT
      && (bit & (TEXGEN_REFLECTION_MAP_NV | TEXGEN_NORMAL_MAP_NV)) == 0) {
   _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
               FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE | _NEW_FF_VERT_PROGRAM,
         texgen->Mode = mode;
      }
         case GL_OBJECT_PLANE:
      {
      if (ctx->API != API_OPENGL_COMPAT) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
      }
   if (TEST_EQ_4V(unit->ObjectPlane[index], params))
         FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE, GL_TEXTURE_BIT);
      }
         case GL_EYE_PLANE:
      {
               if (ctx->API != API_OPENGL_COMPAT) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
               /* Transform plane equation by the inverse modelview matrix */
   if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top)) {
         }
   _mesa_transform_vector(tmp, params,
         if (TEST_EQ_4V(unit->EyePlane[index], tmp))
         FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE, GL_TEXTURE_BIT);
      }
         default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
         }
         /* Helper for glGetTexGendv / glGetMultiTexGendvEXT */
   static void
   gettexgendv( GLuint texunitIndex, GLenum coord, GLenum pname,
         {
      struct gl_texgen *texgen;
            texgen = get_texgen(ctx, texunitIndex, coord, caller);
   if (!texgen) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(coord)", caller);
               struct gl_fixedfunc_texture_unit *unit = &ctx->Texture.FixedFuncUnit[texunitIndex];
            switch (pname) {
   case GL_TEXTURE_GEN_MODE:
      params[0] = ENUM_TO_DOUBLE(texgen->Mode);
      case GL_OBJECT_PLANE:
      COPY_4V(params, unit->ObjectPlane[index]);
      case GL_EYE_PLANE:
      COPY_4V(params, unit->EyePlane[index]);
      default:
            }
         /* Helper for glGetTexGenfv / glGetMultiTexGenfvEXT */
   static void
   gettexgenfv( GLenum texunitIndex, GLenum coord, GLenum pname,
         {
      struct gl_texgen *texgen;
            texgen = get_texgen(ctx, texunitIndex, coord, caller);
   if (!texgen) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(coord)", caller);
               struct gl_fixedfunc_texture_unit *unit = &ctx->Texture.FixedFuncUnit[texunitIndex];
            switch (pname) {
   case GL_TEXTURE_GEN_MODE:
      params[0] = ENUM_TO_FLOAT(texgen->Mode);
      case GL_OBJECT_PLANE:
      if (ctx->API != API_OPENGL_COMPAT) {
      _mesa_error( ctx, GL_INVALID_ENUM, "%s(param)", caller );
      }
   COPY_4V(params, unit->ObjectPlane[index]);
      case GL_EYE_PLANE:
      if (ctx->API != API_OPENGL_COMPAT) {
      _mesa_error( ctx, GL_INVALID_ENUM, "%s(param)", caller );
      }
   COPY_4V(params, unit->EyePlane[index]);
      default:
            }
         /* Helper for glGetTexGeniv / glGetMultiTexGenivEXT */
   static void
   gettexgeniv( GLenum texunitIndex, GLenum coord, GLenum pname,
         {
      struct gl_texgen *texgen;
            texgen = get_texgen(ctx, texunitIndex, coord, caller);
   if (!texgen) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(coord)", caller);
               struct gl_fixedfunc_texture_unit *unit = &ctx->Texture.FixedFuncUnit[texunitIndex];
            switch (pname) {
   case GL_TEXTURE_GEN_MODE:
      params[0] = texgen->Mode;
      case GL_OBJECT_PLANE:
      if (ctx->API != API_OPENGL_COMPAT) {
      _mesa_error( ctx, GL_INVALID_ENUM, "%s(param)" , caller);
      }
   params[0] = (GLint) unit->ObjectPlane[index][0];
   params[1] = (GLint) unit->ObjectPlane[index][1];
   params[2] = (GLint) unit->ObjectPlane[index][2];
   params[3] = (GLint) unit->ObjectPlane[index][3];
      case GL_EYE_PLANE:
      if (ctx->API != API_OPENGL_COMPAT) {
      _mesa_error( ctx, GL_INVALID_ENUM, "%s(param)" , caller);
      }
   params[0] = (GLint) unit->EyePlane[index][0];
   params[1] = (GLint) unit->EyePlane[index][1];
   params[2] = (GLint) unit->EyePlane[index][2];
   params[3] = (GLint) unit->EyePlane[index][3];
      default:
            }
         void GLAPIENTRY
   _mesa_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_MultiTexGenfvEXT( GLenum texunit, GLenum coord, GLenum pname, const GLfloat *params )
   {
         }
         void GLAPIENTRY
   _mesa_TexGeniv(GLenum coord, GLenum pname, const GLint *params )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   if (pname == GL_TEXTURE_GEN_MODE) {
         }
   else {
      p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
      }
      }
      void GLAPIENTRY
   _mesa_MultiTexGenivEXT(GLenum texunit, GLenum coord, GLenum pname, const GLint *params )
   {
      GLfloat p[4];
   p[0] = (GLfloat) params[0];
   if (pname == GL_TEXTURE_GEN_MODE) {
         }
   else {
      p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
      }
      }
         void GLAPIENTRY
   _mesa_TexGend(GLenum coord, GLenum pname, GLdouble param )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0F;
      }
         void GLAPIENTRY
   _mesa_MultiTexGendEXT(GLenum texunit, GLenum coord, GLenum pname, GLdouble param )
   {
      GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0F;
      }
         void GLAPIENTRY
   _mesa_TexGendv(GLenum coord, GLenum pname, const GLdouble *params )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   if (pname == GL_TEXTURE_GEN_MODE) {
         }
   else {
      p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
      }
      }
         void GLAPIENTRY
   _mesa_MultiTexGendvEXT(GLenum texunit, GLenum coord, GLenum pname, const GLdouble *params )
   {
      GLfloat p[4];
   p[0] = (GLfloat) params[0];
   if (pname == GL_TEXTURE_GEN_MODE) {
         }
   else {
      p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
      }
      }
         void GLAPIENTRY
   _mesa_TexGenf( GLenum coord, GLenum pname, GLfloat param )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat p[4];
   p[0] = param;
   p[1] = p[2] = p[3] = 0.0F;
      }
         void GLAPIENTRY
   _mesa_MultiTexGenfEXT( GLenum texunit, GLenum coord, GLenum pname, GLfloat param )
   {
      GLfloat p[4];
   p[0] = param;
   p[1] = p[2] = p[3] = 0.0F;
      }
         void GLAPIENTRY
   _mesa_TexGeni( GLenum coord, GLenum pname, GLint param )
   {
      GLint p[4];
   p[0] = param;
   p[1] = p[2] = p[3] = 0;
      }
         void GLAPIENTRY
   _mesa_MultiTexGeniEXT( GLenum texunit, GLenum coord, GLenum pname, GLint param )
   {
      GLint p[4];
   p[0] = param;
   p[1] = p[2] = p[3] = 0;
      }
         void GLAPIENTRY
   _mesa_GetTexGendv( GLenum coord, GLenum pname, GLdouble *params )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetMultiTexGendvEXT( GLenum texunit, GLenum coord, GLenum pname, GLdouble *params )
   {
         }
         void GLAPIENTRY
   _mesa_GetTexGenfv( GLenum coord, GLenum pname, GLfloat *params )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetMultiTexGenfvEXT( GLenum texunit, GLenum coord, GLenum pname, GLfloat *params )
   {
         }
         void GLAPIENTRY
   _mesa_GetTexGeniv( GLenum coord, GLenum pname, GLint *params )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetMultiTexGenivEXT( GLenum texunit, GLenum coord, GLenum pname, GLint *params )
   {
         }
