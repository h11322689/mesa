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
         #include "util/glheader.h"
   #include "context.h"
   #include "enums.h"
   #include "light.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "math/m_matrix.h"
   #include "util/bitscan.h"
   #include "api_exec_decl.h"
      #include <math.h>
      void GLAPIENTRY
   _mesa_ShadeModel( GLenum mode )
   {
               if (MESA_VERBOSE & VERBOSE_API)
            if (ctx->Light.ShadeModel == mode)
            if (mode != GL_FLAT && mode != GL_SMOOTH) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glShadeModel");
               FLUSH_VERTICES(ctx, _NEW_LIGHT_STATE, GL_LIGHTING_BIT);
      }
         /**
   * Set the provoking vertex (the vertex which specifies the prim's
   * color when flat shading) to either the first or last vertex of the
   * triangle or line.
   */
   void GLAPIENTRY
   _mesa_ProvokingVertex(GLenum mode)
   {
               if (MESA_VERBOSE&VERBOSE_API)
            if (ctx->Light.ProvokingVertex == mode)
            switch (mode) {
   case GL_FIRST_VERTEX_CONVENTION_EXT:
   case GL_LAST_VERTEX_CONVENTION_EXT:
         default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glProvokingVertexEXT(0x%x)", mode);
               FLUSH_VERTICES(ctx, _NEW_LIGHT_STATE, GL_LIGHTING_BIT);
      }
         /**
   * Helper function called by _mesa_Lightfv and _mesa_PopAttrib to set
   * per-light state.
   * For GL_POSITION and GL_SPOT_DIRECTION the params position/direction
   * will have already been transformed by the modelview matrix!
   * Also, all error checking should have already been done.
   */
   static void
   do_light(struct gl_context *ctx, GLuint lnum, GLenum pname, const GLfloat *params)
   {
               assert(lnum < MAX_LIGHTS);
                     switch (pname) {
   case GL_AMBIENT:
      return;
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS, GL_LIGHTING_BIT);
   COPY_4V( lu->Ambient, params );
      case GL_DIFFUSE:
      return;
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS, GL_LIGHTING_BIT);
   COPY_4V( lu->Diffuse, params );
      case GL_SPECULAR:
      return;
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS, GL_LIGHTING_BIT);
   COPY_4V( lu->Specular, params );
      case GL_POSITION: {
      /* NOTE: position has already been transformed by ModelView! */
   return;
                  bool old_positional = lu->EyePosition[3] != 0.0f;
   bool positional = params[3] != 0.0f;
            if (positional != old_positional) {
      if (positional)
                        /* Used by fixed-func vertex program. */
               static const GLfloat eye_z[] = {0, 0, 1};
   GLfloat p[3];
   /* Compute infinite half angle vector:
   *   halfVector = normalize(normalize(lightPos) + (0, 0, 1))
   * light.EyePosition.w should be 0 for infinite lights.
   */
   COPY_3V(p, params);
   NORMALIZE_3FV(p);
   ADD_3V(p, p, eye_z);
   NORMALIZE_3FV(p);
   COPY_3V(lu->_HalfVector, p);
   lu->_HalfVector[3] = 1.0;
      }
   case GL_SPOT_DIRECTION:
      /* NOTE: Direction already transformed by inverse ModelView! */
   return;
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS, GL_LIGHTING_BIT);
   COPY_3V(lu->SpotDirection, params);
      case GL_SPOT_EXPONENT:
      assert(params[0] >= 0.0F);
   assert(params[0] <= ctx->Const.MaxSpotExponent);
   return;
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS, GL_LIGHTING_BIT);
   lu->SpotExponent = params[0];
      case GL_SPOT_CUTOFF: {
      assert(params[0] == 180.0F || (params[0] >= 0.0F && params[0] <= 90.0F));
   if (lu->SpotCutoff == params[0])
                  bool old_is_180 = lu->SpotCutoff == 180.0f;
   bool is_180 = params[0] == 180.0f;
   lu->SpotCutoff = params[0];
   lu->_CosCutoff = (cosf(lu->SpotCutoff * M_PI / 180.0));
   if (lu->_CosCutoff < 0)
            if (is_180 != old_is_180) {
      if (!is_180)
                        /* Used by fixed-func vertex program. */
      }
      }
   case GL_CONSTANT_ATTENUATION: {
      assert(params[0] >= 0.0F);
   return;
                  bool old_is_one = lu->ConstantAttenuation == 1.0f;
   bool is_one = params[0] == 1.0f;
            if (old_is_one != is_one) {
      /* Used by fixed-func vertex program. */
      }
      }
   case GL_LINEAR_ATTENUATION: {
      assert(params[0] >= 0.0F);
   return;
                  bool old_is_zero = lu->LinearAttenuation == 0.0f;
   bool is_zero = params[0] == 0.0f;
            if (old_is_zero != is_zero) {
      /* Used by fixed-func vertex program. */
      }
      }
   case GL_QUADRATIC_ATTENUATION: {
      assert(params[0] >= 0.0F);
   return;
                  bool old_is_zero = lu->QuadraticAttenuation == 0.0f;
   bool is_zero = params[0] == 0.0f;
            if (old_is_zero != is_zero) {
      /* Used by fixed-func vertex program. */
      }
      }
   default:
            }
         void GLAPIENTRY
   _mesa_Lightf( GLenum light, GLenum pname, GLfloat param )
   {
      GLfloat fparam[4];
   fparam[0] = param;
   fparam[1] = fparam[2] = fparam[3] = 0.0F;
      }
         void GLAPIENTRY
   _mesa_Lightfv( GLenum light, GLenum pname, const GLfloat *params )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLint i = (GLint) (light - GL_LIGHT0);
            if (i < 0 || i >= (GLint) ctx->Const.MaxLights) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glLight(light=0x%x)", light );
               /* do particular error checks, transformations */
   switch (pname) {
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
      /* nothing */
      case GL_POSITION:
      /* transform position by ModelView matrix */
   TRANSFORM_POINT(temp, ctx->ModelviewMatrixStack.Top->m, params);
   params = temp;
      case GL_SPOT_DIRECTION:
      /* transform direction by inverse modelview */
   _math_matrix_analyse(ctx->ModelviewMatrixStack.Top);
         }
   TRANSFORM_DIRECTION(temp, params, ctx->ModelviewMatrixStack.Top->m);
   params = temp;
      case GL_SPOT_EXPONENT:
      _mesa_error(ctx, GL_INVALID_VALUE, "glLight");
   return;
         }
      case GL_SPOT_CUTOFF:
      _mesa_error(ctx, GL_INVALID_VALUE, "glLight");
   return;
         }
      case GL_CONSTANT_ATTENUATION:
   case GL_LINEAR_ATTENUATION:
   case GL_QUADRATIC_ATTENUATION:
      _mesa_error(ctx, GL_INVALID_VALUE, "glLight");
   return;
         }
      default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glLight(pname=0x%x)", pname);
                  }
         void GLAPIENTRY
   _mesa_Lighti( GLenum light, GLenum pname, GLint param )
   {
      GLint iparam[4];
   iparam[0] = param;
   iparam[1] = iparam[2] = iparam[3] = 0;
      }
         void GLAPIENTRY
   _mesa_Lightiv( GLenum light, GLenum pname, const GLint *params )
   {
               switch (pname) {
      case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
      fparam[0] = INT_TO_FLOAT( params[0] );
   fparam[1] = INT_TO_FLOAT( params[1] );
   fparam[2] = INT_TO_FLOAT( params[2] );
   fparam[3] = INT_TO_FLOAT( params[3] );
      case GL_POSITION:
      fparam[0] = (GLfloat) params[0];
   fparam[1] = (GLfloat) params[1];
   fparam[2] = (GLfloat) params[2];
   fparam[3] = (GLfloat) params[3];
      case GL_SPOT_DIRECTION:
      fparam[0] = (GLfloat) params[0];
   fparam[1] = (GLfloat) params[1];
   fparam[2] = (GLfloat) params[2];
      case GL_SPOT_EXPONENT:
   case GL_SPOT_CUTOFF:
   case GL_CONSTANT_ATTENUATION:
   case GL_LINEAR_ATTENUATION:
   case GL_QUADRATIC_ATTENUATION:
      fparam[0] = (GLfloat) params[0];
      default:
      /* error will be caught later in gl_Lightfv */
               }
            void GLAPIENTRY
   _mesa_GetLightfv( GLenum light, GLenum pname, GLfloat *params )
   {
      GET_CURRENT_CONTEXT(ctx);
            if (l < 0 || l >= (GLint) ctx->Const.MaxLights) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetLightfv" );
               switch (pname) {
      case GL_AMBIENT:
      COPY_4V( params, ctx->Light.LightSource[l].Ambient );
      case GL_DIFFUSE:
      COPY_4V( params, ctx->Light.LightSource[l].Diffuse );
      case GL_SPECULAR:
      COPY_4V( params, ctx->Light.LightSource[l].Specular );
      case GL_POSITION:
      COPY_4V( params, ctx->Light.LightSource[l].EyePosition );
      case GL_SPOT_DIRECTION:
      COPY_3V( params, ctx->Light.LightSource[l].SpotDirection );
      case GL_SPOT_EXPONENT:
      params[0] = ctx->Light.LightSource[l].SpotExponent;
      case GL_SPOT_CUTOFF:
      params[0] = ctx->Light.LightSource[l].SpotCutoff;
      case GL_CONSTANT_ATTENUATION:
      params[0] = ctx->Light.LightSource[l].ConstantAttenuation;
      case GL_LINEAR_ATTENUATION:
      params[0] = ctx->Light.LightSource[l].LinearAttenuation;
      case GL_QUADRATIC_ATTENUATION:
      params[0] = ctx->Light.LightSource[l].QuadraticAttenuation;
      default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetLightfv" );
      }
         void GLAPIENTRY
   _mesa_GetLightiv( GLenum light, GLenum pname, GLint *params )
   {
      GET_CURRENT_CONTEXT(ctx);
            if (l < 0 || l >= (GLint) ctx->Const.MaxLights) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetLightiv" );
               switch (pname) {
      case GL_AMBIENT:
      params[0] = FLOAT_TO_INT(ctx->Light.LightSource[l].Ambient[0]);
   params[1] = FLOAT_TO_INT(ctx->Light.LightSource[l].Ambient[1]);
   params[2] = FLOAT_TO_INT(ctx->Light.LightSource[l].Ambient[2]);
   params[3] = FLOAT_TO_INT(ctx->Light.LightSource[l].Ambient[3]);
      case GL_DIFFUSE:
      params[0] = FLOAT_TO_INT(ctx->Light.LightSource[l].Diffuse[0]);
   params[1] = FLOAT_TO_INT(ctx->Light.LightSource[l].Diffuse[1]);
   params[2] = FLOAT_TO_INT(ctx->Light.LightSource[l].Diffuse[2]);
   params[3] = FLOAT_TO_INT(ctx->Light.LightSource[l].Diffuse[3]);
      case GL_SPECULAR:
      params[0] = FLOAT_TO_INT(ctx->Light.LightSource[l].Specular[0]);
   params[1] = FLOAT_TO_INT(ctx->Light.LightSource[l].Specular[1]);
   params[2] = FLOAT_TO_INT(ctx->Light.LightSource[l].Specular[2]);
   params[3] = FLOAT_TO_INT(ctx->Light.LightSource[l].Specular[3]);
      case GL_POSITION:
      params[0] = (GLint) ctx->Light.LightSource[l].EyePosition[0];
   params[1] = (GLint) ctx->Light.LightSource[l].EyePosition[1];
   params[2] = (GLint) ctx->Light.LightSource[l].EyePosition[2];
   params[3] = (GLint) ctx->Light.LightSource[l].EyePosition[3];
      case GL_SPOT_DIRECTION:
      params[0] = (GLint) ctx->Light.LightSource[l].SpotDirection[0];
   params[1] = (GLint) ctx->Light.LightSource[l].SpotDirection[1];
   params[2] = (GLint) ctx->Light.LightSource[l].SpotDirection[2];
      case GL_SPOT_EXPONENT:
      params[0] = (GLint) ctx->Light.LightSource[l].SpotExponent;
      case GL_SPOT_CUTOFF:
      params[0] = (GLint) ctx->Light.LightSource[l].SpotCutoff;
      case GL_CONSTANT_ATTENUATION:
      params[0] = (GLint) ctx->Light.LightSource[l].ConstantAttenuation;
      case GL_LINEAR_ATTENUATION:
      params[0] = (GLint) ctx->Light.LightSource[l].LinearAttenuation;
      case GL_QUADRATIC_ATTENUATION:
      params[0] = (GLint) ctx->Light.LightSource[l].QuadraticAttenuation;
      default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetLightiv" );
      }
            /**********************************************************************/
   /***                        Light Model                             ***/
   /**********************************************************************/
         void GLAPIENTRY
   _mesa_LightModelfv( GLenum pname, const GLfloat *params )
   {
      GLenum newenum;
   GLboolean newbool;
            switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS, GL_LIGHTING_BIT);
            COPY_4V( ctx->Light.Model.Ambient, params );
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
      if (ctx->API != API_OPENGL_COMPAT)
      if (ctx->Light.Model.LocalViewer == newbool)
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS | _NEW_FF_VERT_PROGRAM,
         ctx->Light.Model.LocalViewer = newbool;
               case GL_LIGHT_MODEL_TWO_SIDE:
   if (ctx->Light.Model.TwoSide == newbool)
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS | _NEW_FF_VERT_PROGRAM |
         ctx->Light.Model.TwoSide = newbool;
               case GL_LIGHT_MODEL_COLOR_CONTROL:
      if (ctx->API != API_OPENGL_COMPAT)
         newenum = GL_SINGLE_COLOR;
            else {
                  return;
      if (ctx->Light.Model.ColorControl == newenum)
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS | _NEW_FF_VERT_PROGRAM |
         ctx->Light.Model.ColorControl = newenum;
               default:
                     invalid_pname:
      _mesa_error( ctx, GL_INVALID_ENUM, "glLightModel(pname=0x%x)", pname );
      }
         void GLAPIENTRY
   _mesa_LightModeliv( GLenum pname, const GLint *params )
   {
               switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
      fparam[0] = INT_TO_FLOAT( params[0] );
   fparam[1] = INT_TO_FLOAT( params[1] );
   fparam[2] = INT_TO_FLOAT( params[2] );
   fparam[3] = INT_TO_FLOAT( params[3] );
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
   case GL_LIGHT_MODEL_TWO_SIDE:
   case GL_LIGHT_MODEL_COLOR_CONTROL:
      fparam[0] = (GLfloat) params[0];
      default:
      /* Error will be caught later in gl_LightModelfv */
   }
      }
         void GLAPIENTRY
   _mesa_LightModeli( GLenum pname, GLint param )
   {
      GLint iparam[4];
   iparam[0] = param;
   iparam[1] = iparam[2] = iparam[3] = 0;
      }
         void GLAPIENTRY
   _mesa_LightModelf( GLenum pname, GLfloat param )
   {
      GLfloat fparam[4];
   fparam[0] = param;
   fparam[1] = fparam[2] = fparam[3] = 0.0F;
      }
            /********** MATERIAL **********/
         /*
   * Given a face and pname value (ala glColorMaterial), compute a bitmask
   * of the targeted material values.
   */
   GLuint
   _mesa_material_bitmask( struct gl_context *ctx, GLenum face, GLenum pname,
         {
               /* Make a bitmask indicating what material attribute(s) we're updating */
   switch (pname) {
      case GL_EMISSION:
      bitmask |= MAT_BIT_FRONT_EMISSION | MAT_BIT_BACK_EMISSION;
      case GL_AMBIENT:
      bitmask |= MAT_BIT_FRONT_AMBIENT | MAT_BIT_BACK_AMBIENT;
      case GL_DIFFUSE:
      bitmask |= MAT_BIT_FRONT_DIFFUSE | MAT_BIT_BACK_DIFFUSE;
      case GL_SPECULAR:
      bitmask |= MAT_BIT_FRONT_SPECULAR | MAT_BIT_BACK_SPECULAR;
      case GL_SHININESS:
      bitmask |= MAT_BIT_FRONT_SHININESS | MAT_BIT_BACK_SHININESS;
      case GL_AMBIENT_AND_DIFFUSE:
      bitmask |= MAT_BIT_FRONT_AMBIENT | MAT_BIT_BACK_AMBIENT;
   bitmask |= MAT_BIT_FRONT_DIFFUSE | MAT_BIT_BACK_DIFFUSE;
      case GL_COLOR_INDEXES:
      bitmask |= MAT_BIT_FRONT_INDEXES  | MAT_BIT_BACK_INDEXES;
      default:
      _mesa_error( ctx, GL_INVALID_ENUM, "%s", where );
            if (face==GL_FRONT) {
         }
   else if (face==GL_BACK) {
         }
   else if (face != GL_FRONT_AND_BACK) {
      _mesa_error( ctx, GL_INVALID_ENUM, "%s", where );
               if (bitmask & ~legal) {
      _mesa_error( ctx, GL_INVALID_ENUM, "%s", where );
                  }
            /* Update derived values following a change in ctx->Light.Material
   */
   void
   _mesa_update_material( struct gl_context *ctx, GLuint bitmask )
   {
               if (MESA_VERBOSE & VERBOSE_MATERIAL)
            if (!bitmask)
            /* update material ambience */
   if (bitmask & MAT_BIT_FRONT_AMBIENT) {
      GLbitfield mask = ctx->Light._EnabledLights;
   while (mask) {
      const int i = u_bit_scan(&mask);
   struct gl_light *light = &ctx->Light.Light[i];
   struct gl_light_uniforms *lu = &ctx->Light.LightSource[i];
   mat[MAT_ATTRIB_FRONT_AMBIENT]);
                  if (bitmask & MAT_BIT_BACK_AMBIENT) {
      GLbitfield mask = ctx->Light._EnabledLights;
   while (mask) {
      const int i = u_bit_scan(&mask);
   struct gl_light *light = &ctx->Light.Light[i];
   struct gl_light_uniforms *lu = &ctx->Light.LightSource[i];
   mat[MAT_ATTRIB_BACK_AMBIENT]);
                  /* update BaseColor = emission + scene's ambience * material's ambience */
   if (bitmask & (MAT_BIT_FRONT_EMISSION | MAT_BIT_FRONT_AMBIENT)) {
      COPY_3V( ctx->Light._BaseColor[0], mat[MAT_ATTRIB_FRONT_EMISSION] );
   ACC_SCALE_3V( ctx->Light._BaseColor[0], mat[MAT_ATTRIB_FRONT_AMBIENT],
               if (bitmask & (MAT_BIT_BACK_EMISSION | MAT_BIT_BACK_AMBIENT)) {
      COPY_3V( ctx->Light._BaseColor[1], mat[MAT_ATTRIB_BACK_EMISSION] );
   ACC_SCALE_3V( ctx->Light._BaseColor[1], mat[MAT_ATTRIB_BACK_AMBIENT],
               /* update material diffuse values */
   if (bitmask & MAT_BIT_FRONT_DIFFUSE) {
      GLbitfield mask = ctx->Light._EnabledLights;
   while (mask) {
      const int i = u_bit_scan(&mask);
      SCALE_3V( light->_MatDiffuse[0], lu->Diffuse,
      mat[MAT_ATTRIB_FRONT_DIFFUSE] );
                  if (bitmask & MAT_BIT_BACK_DIFFUSE) {
      GLbitfield mask = ctx->Light._EnabledLights;
   while (mask) {
      const int i = u_bit_scan(&mask);
      SCALE_3V( light->_MatDiffuse[1], lu->Diffuse,
      mat[MAT_ATTRIB_BACK_DIFFUSE] );
                  /* update material specular values */
   if (bitmask & MAT_BIT_FRONT_SPECULAR) {
      GLbitfield mask = ctx->Light._EnabledLights;
   while (mask) {
      const int i = u_bit_scan(&mask);
      SCALE_3V( light->_MatSpecular[0], lu->Specular,
      mat[MAT_ATTRIB_FRONT_SPECULAR]);
                  if (bitmask & MAT_BIT_BACK_SPECULAR) {
      GLbitfield mask = ctx->Light._EnabledLights;
   while (mask) {
      const int i = u_bit_scan(&mask);
      SCALE_3V( light->_MatSpecular[1], lu->Specular,
      mat[MAT_ATTRIB_BACK_SPECULAR]);
            }
         /*
   * Update the current materials from the given rgba color
   * according to the bitmask in _ColorMaterialBitmask, which is
   * set by glColorMaterial().
   */
   void
   _mesa_update_color_material( struct gl_context *ctx, const GLfloat color[4] )
   {
      GLbitfield bitmask = ctx->Light._ColorMaterialBitmask;
            while (bitmask) {
               if (memcmp(mat->Attrib[i], color, sizeof(mat->Attrib[i]))) {
      COPY_4FV(mat->Attrib[i], color);
            }
         void GLAPIENTRY
   _mesa_ColorMaterial( GLenum face, GLenum mode )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLuint bitmask;
   GLuint legal = (MAT_BIT_FRONT_EMISSION | MAT_BIT_BACK_EMISSION |
   MAT_BIT_FRONT_SPECULAR | MAT_BIT_BACK_SPECULAR |
   MAT_BIT_FRONT_DIFFUSE  | MAT_BIT_BACK_DIFFUSE  |
            if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glColorMaterial %s %s\n",
               bitmask = _mesa_material_bitmask(ctx, face, mode, legal, "glColorMaterial");
   if (bitmask == 0)
            if (ctx->Light._ColorMaterialBitmask == bitmask &&
      ctx->Light.ColorMaterialFace == face &&
   ctx->Light.ColorMaterialMode == mode)
         FLUSH_VERTICES(ctx, 0, GL_LIGHTING_BIT);
   ctx->Light._ColorMaterialBitmask = bitmask;
   ctx->Light.ColorMaterialFace = face;
            if (ctx->Light.ColorMaterialEnabled) {
      /* Used by fixed-func vertex program. */
   FLUSH_CURRENT(ctx, _NEW_FF_VERT_PROGRAM);
         }
         void GLAPIENTRY
   _mesa_GetMaterialfv( GLenum face, GLenum pname, GLfloat *params )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLuint f;
            FLUSH_VERTICES(ctx, 0, 0); /* update materials */
            if (face==GL_FRONT) {
         }
   else if (face==GL_BACK) {
         }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetMaterialfv(face)" );
               switch (pname) {
      case GL_AMBIENT:
      COPY_4FV( params, mat[MAT_ATTRIB_AMBIENT(f)] );
      case GL_DIFFUSE:
   break;
         case GL_SPECULAR:
   break;
         COPY_4FV( params, mat[MAT_ATTRIB_EMISSION(f)] );
   break;
         *params = mat[MAT_ATTRIB_SHININESS(f)][0];
   break;
         case GL_COLOR_INDEXES:
      if (ctx->API != API_OPENGL_COMPAT) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetMaterialfv(pname)" );
   params[0] = mat[MAT_ATTRIB_INDEXES(f)][0];
   params[1] = mat[MAT_ATTRIB_INDEXES(f)][1];
   params[2] = mat[MAT_ATTRIB_INDEXES(f)][2];
   break;
         default:
         }
         void GLAPIENTRY
   _mesa_GetMaterialiv( GLenum face, GLenum pname, GLint *params )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLuint f;
                     FLUSH_VERTICES(ctx, 0, 0); /* update materials */
            if (face==GL_FRONT) {
         }
   else if (face==GL_BACK) {
         }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetMaterialiv(face)" );
      }
   switch (pname) {
      case GL_AMBIENT:
      params[0] = FLOAT_TO_INT( mat[MAT_ATTRIB_AMBIENT(f)][0] );
   params[1] = FLOAT_TO_INT( mat[MAT_ATTRIB_AMBIENT(f)][1] );
   params[2] = FLOAT_TO_INT( mat[MAT_ATTRIB_AMBIENT(f)][2] );
   params[3] = FLOAT_TO_INT( mat[MAT_ATTRIB_AMBIENT(f)][3] );
      case GL_DIFFUSE:
      params[0] = FLOAT_TO_INT( mat[MAT_ATTRIB_DIFFUSE(f)][0] );
   params[1] = FLOAT_TO_INT( mat[MAT_ATTRIB_DIFFUSE(f)][1] );
      break;
         case GL_SPECULAR:
      params[0] = FLOAT_TO_INT( mat[MAT_ATTRIB_SPECULAR(f)][0] );
   params[1] = FLOAT_TO_INT( mat[MAT_ATTRIB_SPECULAR(f)][1] );
      break;
         case GL_EMISSION:
      params[0] = FLOAT_TO_INT( mat[MAT_ATTRIB_EMISSION(f)][0] );
   params[1] = FLOAT_TO_INT( mat[MAT_ATTRIB_EMISSION(f)][1] );
      break;
         case GL_SHININESS:
   break;
         params[0] = lroundf( mat[MAT_ATTRIB_INDEXES(f)][0] );
   params[1] = lroundf( mat[MAT_ATTRIB_INDEXES(f)][1] );
   params[2] = lroundf( mat[MAT_ATTRIB_INDEXES(f)][2] );
   break;
         default:
         }
            /**
   * Examine current lighting parameters to determine if the optimized lighting
   * function can be used.
   * Also, precompute some lighting values such as the products of light
   * source and material ambient, diffuse and specular coefficients.
   */
   GLbitfield
   _mesa_update_lighting( struct gl_context *ctx )
   {
      GLbitfield flags = 0;
   bool old_need_eye_coords = ctx->Light._NeedEyeCoords;
            if (!ctx->Light.Enabled) {
      return old_need_eye_coords != ctx->Light._NeedEyeCoords ?
               GLbitfield mask = ctx->Light._EnabledLights;
   while (mask) {
      const int i = u_bit_scan(&mask);
   struct gl_light *light = &ctx->Light.Light[i];
               ctx->Light._NeedVertices =
      ((flags & (LIGHT_POSITIONAL|LIGHT_SPOT)) ||
   ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR ||
         ctx->Light._NeedEyeCoords = ((flags & LIGHT_POSITIONAL) ||
            /* XXX: This test is overkill & needs to be fixed both for software and
   * hardware t&l drivers.  The above should be sufficient & should
   * be tested to verify this.
   */
   if (ctx->Light._NeedVertices)
            return old_need_eye_coords != ctx->Light._NeedEyeCoords ?
      }
      void
   _mesa_update_light_materials(struct gl_context *ctx)
   {
      /* Precompute some shading values.  Although we reference
   * Light.Material here, we can get away without flushing
   * FLUSH_UPDATE_CURRENT, as when any outstanding material changes
   * are flushed, they will update the derived state at that time.
   */
   if (ctx->Light.Model.TwoSide)
      _mesa_update_material(ctx,
   MAT_BIT_FRONT_EMISSION |
   MAT_BIT_FRONT_AMBIENT |
   MAT_BIT_FRONT_DIFFUSE |
   MAT_BIT_FRONT_SPECULAR |
   MAT_BIT_BACK_EMISSION |
   MAT_BIT_BACK_AMBIENT |
   MAT_BIT_BACK_DIFFUSE |
      else
      _mesa_update_material(ctx,
   MAT_BIT_FRONT_EMISSION |
   MAT_BIT_FRONT_AMBIENT |
   MAT_BIT_FRONT_DIFFUSE |
   }
         /**
   * Update state derived from light position, spot direction.
   * Called upon:
   *   _NEW_MODELVIEW
   *   _NEW_LIGHT_CONSTANTS
   *   _TNL_NEW_NEED_EYE_COORDS
   *
   * Update on (_NEW_MODELVIEW | _NEW_LIGHT_CONSTANTS) when lighting is enabled.
   * Also update on lighting space changes.
   */
   static void
   compute_light_positions( struct gl_context *ctx )
   {
               if (!ctx->Light.Enabled)
            if (ctx->_NeedEyeCoords) {
         }
   else {
                  GLbitfield mask = ctx->Light._EnabledLights;
   while (mask) {
      const int i = u_bit_scan(&mask);
   struct gl_light *light = &ctx->Light.Light[i];
            if (ctx->_NeedEyeCoords) {
   COPY_4FV( light->_Position, lu->EyePosition );
         }
   else {
   TRANSFORM_POINT( light->_Position, ctx->ModelviewMatrixStack.Top->inv,
      lu->EyePosition );
               /* VP (VP) = Normalize( Position ) */
   COPY_3V( light->_VP_inf_norm, light->_Position );
   NORMALIZE_3FV( light->_VP_inf_norm );
      if (!ctx->Light.Model.LocalViewer) {
      /* _h_inf_norm = Normalize( V_to_P + <0,0,1> ) */
   ADD_3V( light->_h_inf_norm, light->_VP_inf_norm, ctx->_EyeZDir);
      }
   light->_VP_inf_spot_attenuation = 1.0;
         }
   else {
      /* positional light w/ homogeneous coordinate, divide by W */
   GLfloat wInv = 1.0F / light->_Position[3];
   light->_Position[0] *= wInv;
   light->_Position[1] *= wInv;
               if (light->_Flags & LIGHT_SPOT) {
      if (ctx->_NeedEyeCoords) {
      COPY_3V( light->_NormSpotDirection, lu->SpotDirection );
      }
            else {
      GLfloat spotDir[3];
      TRANSFORM_NORMAL( light->_NormSpotDirection,
            }
      NORMALIZE_3FV( light->_NormSpotDirection );
      if (!(light->_Flags & LIGHT_POSITIONAL)) {
      GLfloat PV_dot_dir = - DOT3(light->_VP_inf_norm,
            if (PV_dot_dir > lu->_CosCutoff) {
      light->_VP_inf_spot_attenuation =
      }
   else {
      light->_VP_inf_spot_attenuation = 0;
   }
               }
            static void
   update_modelview_scale( struct gl_context *ctx )
   {
      ctx->_ModelViewInvScale = 1.0F;
   ctx->_ModelViewInvScaleEyespace = 1.0F;
   if (!_math_matrix_is_length_preserving(ctx->ModelviewMatrixStack.Top)) {
      const GLfloat *m = ctx->ModelviewMatrixStack.Top->inv;
   GLfloat f = m[2] * m[2] + m[6] * m[6] + m[10] * m[10];
   if (f < 1e-12f) f = 1.0f;
   ctx->_ModelViewInvScale = 1.0f / sqrtf(f);
         ctx->_ModelViewInvScale = sqrtf(f);
               }
         /**
   * Bring up to date any state that relies on _NeedEyeCoords.
   *
   * Return true if ctx->_NeedEyeCoords has been changed.
   */
   bool
   _mesa_update_tnl_spaces( struct gl_context *ctx, GLuint new_state )
   {
               (void) new_state;
            if ((ctx->Texture._GenFlags & TEXGEN_NEED_EYE_COORD) ||
      ctx->Point._Attenuated ||
   ctx->Light._NeedEyeCoords)
         if (ctx->Light.Enabled &&
      !_math_matrix_is_length_preserving(ctx->ModelviewMatrixStack.Top))
         /* Check if the truth-value interpretations of the bitfields have
   * changed:
   */
   if (oldneedeyecoords != ctx->_NeedEyeCoords) {
      /* Recalculate all state that depends on _NeedEyeCoords.
   */
   update_modelview_scale(ctx);
               }
   else {
               /* Recalculate that same state only if it has been invalidated
   * by other statechanges.
   */
   update_modelview_scale(ctx);
            compute_light_positions( ctx );
                  }
         /**********************************************************************/
   /*****                      Initialization                        *****/
   /**********************************************************************/
      /**
   * Initialize the n-th light data structure.
   *
   * \param l pointer to the gl_light structure to be initialized.
   * \param n number of the light.
   * \note The defaults for light 0 are different than the other lights.
   */
   static void
   init_light( struct gl_light *l, struct gl_light_uniforms *lu, GLuint n )
   {
      ASSIGN_4V( lu->Ambient, 0.0, 0.0, 0.0, 1.0 );
   if (n==0) {
      ASSIGN_4V( lu->Diffuse, 1.0, 1.0, 1.0, 1.0 );
      }
   else {
      ASSIGN_4V( lu->Diffuse, 0.0, 0.0, 0.0, 1.0 );
      }
   ASSIGN_4V( lu->EyePosition, 0.0, 0.0, 1.0, 0.0 );
   ASSIGN_3V( lu->SpotDirection, 0.0, 0.0, -1.0 );
   lu->SpotExponent = 0.0;
   lu->SpotCutoff = 180.0;
   lu->_CosCutoff = 0.0;		/* KW: -ve values not admitted */
   lu->ConstantAttenuation = 1.0;
   lu->LinearAttenuation = 0.0;
   lu->QuadraticAttenuation = 0.0;
      }
         /**
   * Initialize the light model data structure.
   *
   * \param lm pointer to the gl_lightmodel structure to be initialized.
   */
   static void
   init_lightmodel( struct gl_lightmodel *lm )
   {
      ASSIGN_4V( lm->Ambient, 0.2F, 0.2F, 0.2F, 1.0F );
   lm->LocalViewer = GL_FALSE;
   lm->TwoSide = GL_FALSE;
      }
         /**
   * Initialize the material data structure.
   *
   * \param m pointer to the gl_material structure to be initialized.
   */
   static void
   init_material( struct gl_material *m )
   {
      ASSIGN_4V( m->Attrib[MAT_ATTRIB_FRONT_AMBIENT],  0.2F, 0.2F, 0.2F, 1.0F );
   ASSIGN_4V( m->Attrib[MAT_ATTRIB_FRONT_DIFFUSE],  0.8F, 0.8F, 0.8F, 1.0F );
   ASSIGN_4V( m->Attrib[MAT_ATTRIB_FRONT_SPECULAR], 0.0F, 0.0F, 0.0F, 1.0F );
   ASSIGN_4V( m->Attrib[MAT_ATTRIB_FRONT_EMISSION], 0.0F, 0.0F, 0.0F, 1.0F );
   ASSIGN_4V( m->Attrib[MAT_ATTRIB_FRONT_SHININESS], 0.0F, 0.0F, 0.0F, 0.0F );
            ASSIGN_4V( m->Attrib[MAT_ATTRIB_BACK_AMBIENT],  0.2F, 0.2F, 0.2F, 1.0F );
   ASSIGN_4V( m->Attrib[MAT_ATTRIB_BACK_DIFFUSE],  0.8F, 0.8F, 0.8F, 1.0F );
   ASSIGN_4V( m->Attrib[MAT_ATTRIB_BACK_SPECULAR], 0.0F, 0.0F, 0.0F, 1.0F );
   ASSIGN_4V( m->Attrib[MAT_ATTRIB_BACK_EMISSION], 0.0F, 0.0F, 0.0F, 1.0F );
   ASSIGN_4V( m->Attrib[MAT_ATTRIB_BACK_SHININESS], 0.0F, 0.0F, 0.0F, 0.0F );
      }
         /**
   * Initialize all lighting state for the given context.
   */
   void
   _mesa_init_lighting( struct gl_context *ctx )
   {
               /* Lighting group */
   ctx->Light._EnabledLights = 0;
   for (i = 0; i < MAX_LIGHTS; i++) {
                  init_lightmodel( &ctx->Light.Model );
   init_material( &ctx->Light.Material );
   ctx->Light.ShadeModel = GL_SMOOTH;
   ctx->Light.ProvokingVertex = GL_LAST_VERTEX_CONVENTION_EXT;
   ctx->Light.Enabled = GL_FALSE;
   ctx->Light.ColorMaterialFace = GL_FRONT_AND_BACK;
   ctx->Light.ColorMaterialMode = GL_AMBIENT_AND_DIFFUSE;
   ctx->Light._ColorMaterialBitmask = _mesa_material_bitmask( ctx,
                        ctx->Light.ColorMaterialEnabled = GL_FALSE;
   ctx->Light.ClampVertexColor = _mesa_is_desktop_gl_compat(ctx);
            /* Miscellaneous */
   ctx->Light._NeedEyeCoords = GL_FALSE;
   ctx->_NeedEyeCoords = GL_FALSE;
   ctx->_ModelViewInvScale = 1.0;
      }
