   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
   #include "fog.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "api_exec_decl.h"
         void GLAPIENTRY
   _mesa_Fogf(GLenum pname, GLfloat param)
   {
      GLfloat fparam[4];
   fparam[0] = param;
   fparam[1] = fparam[2] = fparam[3] = 0.0F;
      }
         void GLAPIENTRY
   _mesa_Fogi(GLenum pname, GLint param )
   {
      GLfloat fparam[4];
   fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0F;
      }
         void GLAPIENTRY
   _mesa_Fogiv(GLenum pname, const GLint *params )
   {
      GLfloat p[4];
   switch (pname) {
      case GL_FOG_MODE:
   case GL_FOG_DENSITY:
   case GL_FOG_START:
   case GL_FOG_END:
   case GL_FOG_INDEX:
   case GL_FOG_COORDINATE_SOURCE_EXT:
   p[0] = (GLfloat) *params;
   break;
         p[0] = INT_TO_FLOAT( params[0] );
   p[1] = INT_TO_FLOAT( params[1] );
   p[2] = INT_TO_FLOAT( params[2] );
   p[3] = INT_TO_FLOAT( params[3] );
   break;
         default:
      /* Error will be caught later in _mesa_Fogfv */
   }
      }
         void GLAPIENTRY
   _mesa_Fogfv( GLenum pname, const GLfloat *params )
   {
      GET_CURRENT_CONTEXT(ctx);
            switch (pname) {
      case GL_FOG_MODE:
   switch (m) {
   case GL_LINEAR:
      ctx->Fog._PackedMode = FOG_LINEAR;
      case GL_EXP:
      ctx->Fog._PackedMode = FOG_EXP;
      case GL_EXP2:
      ctx->Fog._PackedMode = FOG_EXP2;
      default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glFog" );
      }
   if (ctx->Fog.Mode == m)
         FLUSH_VERTICES(ctx, _NEW_FOG, GL_FOG_BIT);
   ctx->Fog.Mode = m;
            if (ctx->Fog.Enabled) {
      ctx->Fog._PackedEnabledMode = ctx->Fog._PackedMode;
   break;
         if (*params<0.0F) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glFog" );
      }
   if (ctx->Fog.Density == *params)
         FLUSH_VERTICES(ctx, _NEW_FOG, GL_FOG_BIT);
   ctx->Fog.Density = *params;
   break;
         case GL_FOG_START:
      if (ctx->Fog.Start == *params)
         FLUSH_VERTICES(ctx, _NEW_FOG, GL_FOG_BIT);
   ctx->Fog.Start = *params;
      case GL_FOG_END:
      if (ctx->Fog.End == *params)
         FLUSH_VERTICES(ctx, _NEW_FOG, GL_FOG_BIT);
   ctx->Fog.End = *params;
      case GL_FOG_INDEX:
      if (ctx->API != API_OPENGL_COMPAT)
   if (ctx->Fog.Index == *params)
      FLUSH_VERTICES(ctx, _NEW_FOG, GL_FOG_BIT);
         break;
         if (TEST_EQ_4V(ctx->Fog.Color, params))
         FLUSH_VERTICES(ctx, _NEW_FOG, GL_FOG_BIT);
   ctx->Fog.ColorUnclamped[0] = params[0];
   ctx->Fog.ColorUnclamped[1] = params[1];
   ctx->Fog.ColorUnclamped[2] = params[2];
   ctx->Fog.ColorUnclamped[3] = params[3];
   ctx->Fog.Color[0] = CLAMP(params[0], 0.0F, 1.0F);
   ctx->Fog.Color[1] = CLAMP(params[1], 0.0F, 1.0F);
   ctx->Fog.Color[2] = CLAMP(params[2], 0.0F, 1.0F);
   ctx->Fog.Color[3] = CLAMP(params[3], 0.0F, 1.0F);
               GLenum p = (GLenum) (GLint) *params;
            if (ctx->API != API_OPENGL_COMPAT ||
   _mesa_error(ctx, GL_INVALID_ENUM, "glFog");
      }
   if (ctx->Fog.FogCoordinateSource == p)
         FLUSH_VERTICES(ctx, _NEW_FOG | _NEW_FF_VERT_PROGRAM, GL_FOG_BIT);
   ctx->Fog.FogCoordinateSource = p;
   break;
         }
   GLenum p = (GLenum) (GLint) *params;
            if (ctx->API != API_OPENGL_COMPAT || !ctx->Extensions.NV_fog_distance ||
   _mesa_error(ctx, GL_INVALID_ENUM, "glFog");
      }
   if (ctx->Fog.FogDistanceMode == p)
         FLUSH_VERTICES(ctx, _NEW_FOG | _NEW_FF_VERT_PROGRAM, GL_FOG_BIT);
   ctx->Fog.FogDistanceMode = p;
   break;
         }
   default:
                     invalid_pname:
      _mesa_error( ctx, GL_INVALID_ENUM, "glFog" );
      }
         /**********************************************************************/
   /*****                      Initialization                        *****/
   /**********************************************************************/
      void _mesa_init_fog( struct gl_context * ctx )
   {
      /* Fog group */
   ctx->Fog.Enabled = GL_FALSE;
   ctx->Fog.Mode = GL_EXP;
   ctx->Fog._PackedMode = FOG_EXP;
   ctx->Fog._PackedEnabledMode = FOG_NONE;
   ASSIGN_4V( ctx->Fog.Color, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( ctx->Fog.ColorUnclamped, 0.0, 0.0, 0.0, 0.0 );
   ctx->Fog.Index = 0.0;
   ctx->Fog.Density = 1.0;
   ctx->Fog.Start = 0.0;
   ctx->Fog.End = 1.0;
   ctx->Fog.ColorSumEnabled = GL_FALSE;
   ctx->Fog.FogCoordinateSource = GL_FRAGMENT_DEPTH_EXT;
      }
