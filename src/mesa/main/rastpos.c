   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
   * \file rastpos.c
   * Raster position operations.
   */
      #include "util/glheader.h"
   #include "context.h"
   #include "feedback.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "rastpos.h"
   #include "state.h"
   #include "main/light.h"
   #include "main/viewport.h"
   #include "util/bitscan.h"
      #include "state_tracker/st_cb_rasterpos.h"
   #include "api_exec_decl.h"
         /**
   * Clip a point against the view volume.
   *
   * \param v vertex vector describing the point to clip.
   *
   * \return zero if outside view volume, or one if inside.
   */
   static GLuint
   viewclip_point_xy( const GLfloat v[] )
   {
      if (   v[0] > v[3] || v[0] < -v[3]
      || v[1] > v[3] || v[1] < -v[3] ) {
      }
   else {
            }
         /**
   * Clip a point against the near Z clipping planes.
   *
   * \param v vertex vector describing the point to clip.
   *
   * \return zero if outside view volume, or one if inside.
   */
   static GLuint
   viewclip_point_near_z( const GLfloat v[] )
   {
      if (v[2] < -v[3]) {
         }
   else {
            }
         /**
   * Clip a point against the far Z clipping planes.
   *
   * \param v vertex vector describing the point to clip.
   *
   * \return zero if outside view volume, or one if inside.
   */
   static GLuint
   viewclip_point_far_z( const GLfloat v[] )
   {
      if (v[2] > v[3]) {
         }
   else {
            }
         /**
   * Clip a point against the user clipping planes.
   *
   * \param ctx GL context.
   * \param v vertex vector describing the point to clip.
   *
   * \return zero if the point was clipped, or one otherwise.
   */
   static GLuint
   userclip_point( struct gl_context *ctx, const GLfloat v[] )
   {
      GLbitfield mask = ctx->Transform.ClipPlanesEnabled;
   while (mask) {
      const int p = u_bit_scan(&mask);
   GLfloat dot = v[0] * ctx->Transform._ClipUserPlane[p][0]
      + v[1] * ctx->Transform._ClipUserPlane[p][1]
               if (dot < 0.0F) {
                        }
         /**
   * Compute lighting for the raster position.  RGB modes computed.
   * \param ctx the context
   * \param vertex vertex location
   * \param normal normal vector
   * \param Rcolor returned color
   * \param Rspec returned specular color (if separate specular enabled)
   */
   static void
   shade_rastpos(struct gl_context *ctx,
               const GLfloat vertex[4],
   const GLfloat normal[3],
   {
      /*const*/ GLfloat (*base)[3] = ctx->Light._BaseColor;
   GLbitfield mask;
                     COPY_3V(diffuseColor, base[0]);
   diffuseColor[3] = CLAMP(
                  mask = ctx->Light._EnabledLights;
   while (mask) {
      const int i = u_bit_scan(&mask);
   struct gl_light *light = &ctx->Light.Light[i];
   struct gl_light_uniforms *lu = &ctx->Light.LightSource[i];
   GLfloat attenuation = 1.0;
   GLfloat VP[3]; /* vector from vertex to light pos */
   GLfloat n_dot_VP;
            if (!(light->_Flags & LIGHT_POSITIONAL)) {
   COPY_3V(VP, light->_VP_inf_norm);
   attenuation = light->_VP_inf_spot_attenuation;
         }
   else {
   GLfloat d;
            SUB_3V(VP, light->_Position, vertex);
         d = (GLfloat) LEN_3FV( VP );
   if (d > 1.0e-6F) {
            GLfloat invd = 1.0F / d;
      }
            attenuation = 1.0F / (lu->ConstantAttenuation + d *
                  if (light->_Flags & LIGHT_SPOT) {
               if (PV_dot_dir<lu->_CosCutoff) {
         }
   else {
                  }
                  continue;
                     ACC_SCALE_SCALAR_3V(diffuseColor, attenuation, light->_MatAmbient[0]);
   continue;
                  /* Ambient + diffuse */
   COPY_3V(diffuseContrib, light->_MatAmbient[0]);
            /* Specular */
   {
                     if (ctx->Light.Model.LocalViewer) {
      GLfloat v[3];
   COPY_3V(v, vertex);
   NORMALIZE_3FV(v);
   SUB_3V(VP, VP, v);
            }
   else if (light->_Flags & LIGHT_POSITIONAL) {
      ACC_3V(VP, ctx->_EyeZDir);
            }
               }
      n_dot_h = DOT3(normal, h);
      if (n_dot_h > 0.0F) {
      GLfloat shine;
            shine = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_SHININESS][0];
            if (spec_coef > 1.0e-10F) {
               if (ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR) {
      ACC_SCALE_SCALAR_3V( specularContrib, spec_coef,
      }
   else {
            }
                  ACC_SCALE_SCALAR_3V( diffuseColor, attenuation, diffuseContrib );
               Rcolor[0] = CLAMP(diffuseColor[0], 0.0F, 1.0F);
   Rcolor[1] = CLAMP(diffuseColor[1], 0.0F, 1.0F);
   Rcolor[2] = CLAMP(diffuseColor[2], 0.0F, 1.0F);
   Rcolor[3] = CLAMP(diffuseColor[3], 0.0F, 1.0F);
   Rspec[0] = CLAMP(specularColor[0], 0.0F, 1.0F);
   Rspec[1] = CLAMP(specularColor[1], 0.0F, 1.0F);
   Rspec[2] = CLAMP(specularColor[2], 0.0F, 1.0F);
      }
         /**
   * Do texgen needed for glRasterPos.
   * \param ctx  rendering context
   * \param vObj  object-space vertex coordinate
   * \param vEye  eye-space vertex coordinate
   * \param normal  vertex normal
   * \param unit  texture unit number
   * \param texcoord  incoming texcoord and resulting texcoord
   */
   static void
   compute_texgen(struct gl_context *ctx, const GLfloat vObj[4], const GLfloat vEye[4],
         {
      const struct gl_fixedfunc_texture_unit *texUnit =
            /* always compute sphere map terms, just in case */
   GLfloat u[3], two_nu, rx, ry, rz, m, mInv;
   COPY_3V(u, vEye);
   NORMALIZE_3FV(u);
   two_nu = 2.0F * DOT3(normal, u);
   rx = u[0] - normal[0] * two_nu;
   ry = u[1] - normal[1] * two_nu;
   rz = u[2] - normal[2] * two_nu;
   m = rx * rx + ry * ry + (rz + 1.0F) * (rz + 1.0F);
   if (m > 0.0F)
         else
            if (texUnit->TexGenEnabled & S_BIT) {
      switch (texUnit->GenS.Mode) {
      case GL_OBJECT_LINEAR:
      texcoord[0] = DOT4(vObj, texUnit->ObjectPlane[GEN_S]);
      case GL_EYE_LINEAR:
      texcoord[0] = DOT4(vEye, texUnit->EyePlane[GEN_S]);
      case GL_SPHERE_MAP:
      texcoord[0] = rx * mInv + 0.5F;
      case GL_REFLECTION_MAP:
      texcoord[0] = rx;
      case GL_NORMAL_MAP:
      texcoord[0] = normal[0];
      default:
      _mesa_problem(ctx, "Bad S texgen in compute_texgen()");
               if (texUnit->TexGenEnabled & T_BIT) {
      switch (texUnit->GenT.Mode) {
      case GL_OBJECT_LINEAR:
      texcoord[1] = DOT4(vObj, texUnit->ObjectPlane[GEN_T]);
      case GL_EYE_LINEAR:
      texcoord[1] = DOT4(vEye, texUnit->EyePlane[GEN_T]);
      case GL_SPHERE_MAP:
      texcoord[1] = ry * mInv + 0.5F;
      case GL_REFLECTION_MAP:
      texcoord[1] = ry;
      case GL_NORMAL_MAP:
      texcoord[1] = normal[1];
      default:
      _mesa_problem(ctx, "Bad T texgen in compute_texgen()");
               if (texUnit->TexGenEnabled & R_BIT) {
      switch (texUnit->GenR.Mode) {
      case GL_OBJECT_LINEAR:
      texcoord[2] = DOT4(vObj, texUnit->ObjectPlane[GEN_R]);
      case GL_EYE_LINEAR:
      texcoord[2] = DOT4(vEye, texUnit->EyePlane[GEN_R]);
      case GL_REFLECTION_MAP:
      texcoord[2] = rz;
      case GL_NORMAL_MAP:
      texcoord[2] = normal[2];
      default:
      _mesa_problem(ctx, "Bad R texgen in compute_texgen()");
               if (texUnit->TexGenEnabled & Q_BIT) {
      switch (texUnit->GenQ.Mode) {
      case GL_OBJECT_LINEAR:
      texcoord[3] = DOT4(vObj, texUnit->ObjectPlane[GEN_Q]);
      case GL_EYE_LINEAR:
      texcoord[3] = DOT4(vEye, texUnit->EyePlane[GEN_Q]);
      default:
      _mesa_problem(ctx, "Bad Q texgen in compute_texgen()");
         }
         /**
   * glRasterPos transformation.
   *
   * \param vObj  vertex position in object space
   */
   void
   _mesa_RasterPos(struct gl_context *ctx, const GLfloat vObj[4])
   {
               if (_mesa_arb_vertex_program_enabled(ctx)) {
      /* XXX implement this */
   _mesa_problem(ctx, "Vertex programs not implemented for glRasterPos");
      }
   else {
      GLfloat eye[4], clip[4], ndc[3], d;
   GLfloat *norm, eyenorm[3];
   GLfloat *objnorm = ctx->Current.Attrib[VERT_ATTRIB_NORMAL];
            /* apply modelview matrix:  eye = MV * obj */
   TRANSFORM_POINT( eye, ctx->ModelviewMatrixStack.Top->m, vObj );
   /* apply projection matrix:  clip = Proj * eye */
            /* clip to view volume. */
   if (!ctx->Transform.DepthClampNear) {
      if (viewclip_point_near_z(clip) == 0) {
      ctx->Current.RasterPosValid = GL_FALSE;
         }
   if (!ctx->Transform.DepthClampFar) {
      if (viewclip_point_far_z(clip) == 0) {
      ctx->Current.RasterPosValid = GL_FALSE;
         }
   if (!ctx->Transform.RasterPositionUnclipped) {
      if (viewclip_point_xy(clip) == 0) {
      ctx->Current.RasterPosValid = GL_FALSE;
                  /* clip to user clipping planes */
   if (ctx->Transform.ClipPlanesEnabled && !userclip_point(ctx, clip)) {
      ctx->Current.RasterPosValid = GL_FALSE;
               /* ndc = clip / W */
   d = (clip[3] == 0.0F) ? 1.0F : 1.0F / clip[3];
   ndc[0] = clip[0] * d;
   ndc[1] = clip[1] * d;
   ndc[2] = clip[2] * d;
   /* wincoord = viewport_mapping(ndc) */
   _mesa_get_viewport_xform(ctx, 0, scale, translate);
   ctx->Current.RasterPos[0] = ndc[0] * scale[0] + translate[0];
   ctx->Current.RasterPos[1] = ndc[1] * scale[1] + translate[1];
   ctx->Current.RasterPos[2] = ndc[2] * scale[2] + translate[2];
            if (ctx->Transform.DepthClampNear &&
      ctx->Transform.DepthClampFar) {
   ctx->Current.RasterPos[3] = CLAMP(ctx->Current.RasterPos[3],
            } else {
      /* Clamp against near and far plane separately */
   if (ctx->Transform.DepthClampNear) {
      ctx->Current.RasterPos[3] = MAX2(ctx->Current.RasterPos[3],
               if (ctx->Transform.DepthClampFar) {
      ctx->Current.RasterPos[3] = MIN2(ctx->Current.RasterPos[3],
                  /* compute raster distance */
   if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT)
         else
                  /* compute transformed normal vector (for lighting or texgen) */
   if (ctx->_NeedEyeCoords) {
      const GLfloat *inv = ctx->ModelviewMatrixStack.Top->inv;
   TRANSFORM_NORMAL( eyenorm, objnorm, inv );
      }
   else {
                  /* update raster color */
   if (ctx->Light.Enabled) {
      /* lighting */
   shade_rastpos( ctx, vObj, norm,
            }
   else {
   COPY_4FV(ctx->Current.RasterColor,
         COPY_4FV(ctx->Current.RasterSecondaryColor,
      ctx->Current.Attrib[VERT_ATTRIB_COLOR1]);
               /* texture coords */
   {
      GLuint u;
   for (u = 0; u < ctx->Const.MaxTextureCoordUnits; u++) {
      GLfloat tc[4];
   COPY_4V(tc, ctx->Current.Attrib[VERT_ATTRIB_TEX0 + u]);
   if (ctx->Texture.FixedFuncUnit[u].TexGenEnabled) {
         }
   TRANSFORM_POINT(ctx->Current.RasterTexCoords[u],
                              if (ctx->RenderMode == GL_SELECT) {
            }
         /**
   * Helper function for all the RasterPos functions.
   */
   static void
   rasterpos(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
   {
      GET_CURRENT_CONTEXT(ctx);
            p[0] = x;
   p[1] = y;
   p[2] = z;
            FLUSH_VERTICES(ctx, 0, 0);
            if (ctx->NewState)
               }
         void GLAPIENTRY
   _mesa_RasterPos2d(GLdouble x, GLdouble y)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos2f(GLfloat x, GLfloat y)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos2i(GLint x, GLint y)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos2s(GLshort x, GLshort y)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos3i(GLint x, GLint y, GLint z)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos3s(GLshort x, GLshort y, GLshort z)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos2dv(const GLdouble *v)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos2fv(const GLfloat *v)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos2iv(const GLint *v)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos2sv(const GLshort *v)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos3dv(const GLdouble *v)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos3fv(const GLfloat *v)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos3iv(const GLint *v)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos3sv(const GLshort *v)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos4dv(const GLdouble *v)
   {
      rasterpos((GLfloat) v[0], (GLfloat) v[1], 
      }
      void GLAPIENTRY
   _mesa_RasterPos4fv(const GLfloat *v)
   {
         }
      void GLAPIENTRY
   _mesa_RasterPos4iv(const GLint *v)
   {
      rasterpos((GLfloat) v[0], (GLfloat) v[1], 
      }
      void GLAPIENTRY
   _mesa_RasterPos4sv(const GLshort *v)
   {
         }
         /**********************************************************************/
   /***           GL_ARB_window_pos / GL_MESA_window_pos               ***/
   /**********************************************************************/
         /**
   * All glWindowPosMESA and glWindowPosARB commands call this function to
   * update the current raster position.
   */
   static void
   window_pos3f(GLfloat x, GLfloat y, GLfloat z)
   {
      GET_CURRENT_CONTEXT(ctx);
            FLUSH_VERTICES(ctx, 0, GL_CURRENT_BIT);
            z2 = CLAMP(z, 0.0F, 1.0F)
      * (ctx->ViewportArray[0].Far - ctx->ViewportArray[0].Near)
         /* set raster position */
   ctx->Current.RasterPos[0] = x;
   ctx->Current.RasterPos[1] = y;
   ctx->Current.RasterPos[2] = z2;
                     if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT)
         else
            /* raster color = current color or index */
   ctx->Current.RasterColor[0]
         ctx->Current.RasterColor[1]
         ctx->Current.RasterColor[2]
         ctx->Current.RasterColor[3]
         ctx->Current.RasterSecondaryColor[0]
         ctx->Current.RasterSecondaryColor[1]
         ctx->Current.RasterSecondaryColor[2]
         ctx->Current.RasterSecondaryColor[3]
            /* raster texcoord = current texcoord */
   {
      GLuint texSet;
   for (texSet = 0; texSet < ctx->Const.MaxTextureCoordUnits; texSet++) {
      assert(texSet < ARRAY_SIZE(ctx->Current.RasterTexCoords));
   COPY_4FV( ctx->Current.RasterTexCoords[texSet],
                  if (ctx->RenderMode==GL_SELECT) {
            }
         /* This is just to support the GL_MESA_window_pos version */
   static void
   window_pos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
   {
      GET_CURRENT_CONTEXT(ctx);
   window_pos3f(x, y, z);
      }
         void GLAPIENTRY
   _mesa_WindowPos2d(GLdouble x, GLdouble y)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos2f(GLfloat x, GLfloat y)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos2i(GLint x, GLint y)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos2s(GLshort x, GLshort y)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos3d(GLdouble x, GLdouble y, GLdouble z)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos3f(GLfloat x, GLfloat y, GLfloat z)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos3i(GLint x, GLint y, GLint z)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos3s(GLshort x, GLshort y, GLshort z)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos4dMESA(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos4fMESA(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos4iMESA(GLint x, GLint y, GLint z, GLint w)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos4sMESA(GLshort x, GLshort y, GLshort z, GLshort w)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos2dv(const GLdouble *v)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos2fv(const GLfloat *v)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos2iv(const GLint *v)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos2sv(const GLshort *v)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos3dv(const GLdouble *v)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos3fv(const GLfloat *v)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos3iv(const GLint *v)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos3sv(const GLshort *v)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos4dvMESA(const GLdouble *v)
   {
      window_pos4f((GLfloat) v[0], (GLfloat) v[1], 
      }
      void GLAPIENTRY
   _mesa_WindowPos4fvMESA(const GLfloat *v)
   {
         }
      void GLAPIENTRY
   _mesa_WindowPos4ivMESA(const GLint *v)
   {
      window_pos4f((GLfloat) v[0], (GLfloat) v[1], 
      }
      void GLAPIENTRY
   _mesa_WindowPos4svMESA(const GLshort *v)
   {
         }
         #if 0
      /*
   * OpenGL implementation of glWindowPos*MESA()
   */
   void glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
   {
               /* Push current matrix mode and viewport attributes */
            /* Setup projection parameters */
   glMatrixMode( GL_PROJECTION );
   glPushMatrix();
   glLoadIdentity();
   glMatrixMode( GL_MODELVIEW );
   glPushMatrix();
            glDepthRange( z, z );
            /* set the raster (window) position */
   fx = x - (int) x;
   fy = y - (int) y;
            /* restore matrices, viewport and matrix mode */
   glPopMatrix();
   glMatrixMode( GL_PROJECTION );
               }
      #endif
         /**********************************************************************/
   /** \name Initialization                                              */
   /**********************************************************************/
   /*@{*/
      /**
   * Initialize the context current raster position information.
   *
   * \param ctx GL context.
   *
   * Initialize the current raster position information in
   * __struct gl_contextRec::Current, and adds the extension entry points to the
   * dispatcher.
   */
   void _mesa_init_rastpos( struct gl_context * ctx )
   {
               ASSIGN_4V( ctx->Current.RasterPos, 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.RasterDistance = 0.0;
   ASSIGN_4V( ctx->Current.RasterColor, 1.0, 1.0, 1.0, 1.0 );
   ASSIGN_4V( ctx->Current.RasterSecondaryColor, 0.0, 0.0, 0.0, 1.0 );
   for (i = 0; i < ARRAY_SIZE(ctx->Current.RasterTexCoords); i++)
            }
      /*@}*/
