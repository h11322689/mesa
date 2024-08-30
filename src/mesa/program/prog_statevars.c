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
   * \file prog_statevars.c
   * Program state variable management.
   * \author Brian Paul
   */
         #include <stdio.h>
   #include <stddef.h>
   #include "util/glheader.h"
   #include "main/context.h"
   #include "main/blend.h"
      #include "main/macros.h"
   #include "main/fbobject.h"
   #include "prog_statevars.h"
   #include "prog_parameter.h"
   #include "main/samplerobj.h"
   #include "main/framebuffer.h"
         #define ONE_DIV_SQRT_LN2 (1.201122408786449815)
      static ALWAYS_INLINE void
   copy_matrix(float *value, const float *m, unsigned firstRow, unsigned lastRow)
   {
               assert(firstRow < 4);
            for (i = 0, row = firstRow; row <= lastRow; row++) {
      value[i++] = m[row + 0];
   value[i++] = m[row + 4];
   value[i++] = m[row + 8];
         }
      static ALWAYS_INLINE void
   copy_matrix_transposed(float *value, const float *m, unsigned firstRow, unsigned lastRow)
   {
      assert(firstRow < 4);
            memcpy(value, &m[firstRow * 4],
      }
      /**
   * Use the list of tokens in the state[] array to find global GL state
   * and return it in <value>.  Usually, four values are returned in <value>
   * but matrix queries may return as many as 16 values.
   * This function is used for ARB vertex/fragment programs.
   * The program parser will produce the state[] values.
   */
   static void
   fetch_state(struct gl_context *ctx, const gl_state_index16 state[],
         {
               switch (state[0]) {
   case STATE_MATERIAL:
      {
      /* state[1] is MAT_ATTRIB_FRONT_* */
   const GLuint index = (GLuint) state[1];
   const struct gl_material *mat = &ctx->Light.Material;
   assert(index >= MAT_ATTRIB_FRONT_AMBIENT &&
         if (index >= MAT_ATTRIB_FRONT_SHININESS) {
      value[0] = mat->Attrib[index][0];
   value[1] = 0.0F;
   value[2] = 0.0F;
      } else {
         }
         case STATE_LIGHT:
      {
      /* state[1] is the light number */
   const GLuint ln = (GLuint) state[1];
   /* state[2] is the light attribute */
   const unsigned index = state[2] - STATE_AMBIENT;
   assert(index < 8);
   if (index != STATE_SPOT_CUTOFF)
         else
               case STATE_LIGHT_ARRAY: {
      /* This must be exact because it must match the gl_LightSource layout
   * in GLSL.
   */
   STATIC_ASSERT(sizeof(struct gl_light_uniforms) == 29 * 4);
   STATIC_ASSERT(ARRAY_SIZE(ctx->Light.LightSourceData) == 29 * MAX_LIGHTS);
   /* state[1] is the index of the first value */
   /* state[2] is the number of values */
   assert(state[1] + state[2] <= ARRAY_SIZE(ctx->Light.LightSourceData));
   memcpy(value, &ctx->Light.LightSourceData[state[1]],
            }
   case STATE_LIGHT_ATTENUATION_ARRAY: {
      const unsigned first = state[1];
   const unsigned num_lights = state[2];
   for (unsigned i = 0; i < num_lights; i++) {
      COPY_4V(value,
            }
      }
   case STATE_LIGHTMODEL_AMBIENT:
      COPY_4V(value, ctx->Light.Model.Ambient);
      case STATE_LIGHTMODEL_SCENECOLOR:
      if (state[1] == 0) {
      /* front */
   GLint i;
   for (i = 0; i < 3; i++) {
      value[i] = ctx->Light.Model.Ambient[i]
         value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE][3];
         }
   else {
      /* back */
   GLint i;
   for (i = 0; i < 3; i++) {
      value[i] = ctx->Light.Model.Ambient[i]
         value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_DIFFUSE][3];
         }
      case STATE_LIGHTPROD:
      {
      const GLuint ln = (GLuint) state[1];
   const GLuint index = (GLuint) state[2];
   const GLuint attr = (index / 2) * 4;
   assert(index >= MAT_ATTRIB_FRONT_AMBIENT &&
         for (int i = 0; i < 3; i++) {
      /* We want attr to access out of bounds into the following Diffuse
   * and Specular fields. This is guaranteed to work because
   * STATE_LIGHT and STATE_LIGHT_ARRAY also rely on this memory
   * layout.
   */
   STATIC_ASSERT(offsetof(struct gl_light_uniforms, Ambient) + 16 ==
         STATIC_ASSERT(offsetof(struct gl_light_uniforms, Diffuse) + 16 ==
         value[i] = ctx->Light.LightSource[ln].Ambient[attr + i] *
      }
   /* [3] = material alpha */
   value[3] = ctx->Light.Material.Attrib[index][3];
         case STATE_LIGHTPROD_ARRAY_FRONT: {
      const unsigned first_light = state[1];
            for (unsigned i = 0; i < num_lights; i++) {
               for (unsigned attrib = MAT_ATTRIB_FRONT_AMBIENT;
      attrib <= MAT_ATTRIB_FRONT_SPECULAR; attrib += 2) {
   for (int chan = 0; chan < 3; chan++) {
      /* We want offset to access out of bounds into the following
   * Diffuse and Specular fields. This is guaranteed to work
   * because STATE_LIGHT and STATE_LIGHT_ATTRIBS also rely
   * on this memory layout.
   */
   unsigned offset = (attrib / 2) * 4 + chan;
   *value++ =
      (&ctx->Light.LightSource[light].Ambient[0])[offset] *
   }
   /* [3] = material alpha */
         }
      }
   case STATE_LIGHTPROD_ARRAY_BACK: {
      const unsigned first_light = state[1];
            for (unsigned i = 0; i < num_lights; i++) {
               for (unsigned attrib = MAT_ATTRIB_BACK_AMBIENT;
      attrib <= MAT_ATTRIB_BACK_SPECULAR; attrib += 2) {
   for (int chan = 0; chan < 3; chan++) {
      /* We want offset to access out of bounds into the following
   * Diffuse and Specular fields. This is guaranteed to work
   * because STATE_LIGHT and STATE_LIGHT_ATTRIBS also rely
   * on this memory layout.
   */
   unsigned offset = (attrib / 2) * 4 + chan;
   *value++ =
      (&ctx->Light.LightSource[light].Ambient[0])[offset] *
   }
   /* [3] = material alpha */
         }
      }
   case STATE_LIGHTPROD_ARRAY_TWOSIDE: {
      const unsigned first_light = state[1];
            for (unsigned i = 0; i < num_lights; i++) {
               for (unsigned attrib = MAT_ATTRIB_FRONT_AMBIENT;
      attrib <= MAT_ATTRIB_BACK_SPECULAR; attrib++) {
   for (int chan = 0; chan < 3; chan++) {
      /* We want offset to access out of bounds into the following
   * Diffuse and Specular fields. This is guaranteed to work
   * because STATE_LIGHT and STATE_LIGHT_ATTRIBS also rely
   * on this memory layout.
   */
   unsigned offset = (attrib / 2) * 4 + chan;
   *value++ =
      (&ctx->Light.LightSource[light].Ambient[0])[offset] *
   }
   /* [3] = material alpha */
         }
      }
   case STATE_TEXGEN:
      {
      /* state[1] is the texture unit */
   const GLuint unit = (GLuint) state[1];
      #define MEMBER_SIZEOF(type, member) sizeof(((type *)0)->member)
            STATIC_ASSERT(MEMBER_SIZEOF(struct gl_fixedfunc_texture_unit,
            #undef MEMBER_SIZEOF
            STATIC_ASSERT(STATE_TEXGEN_EYE_T - STATE_TEXGEN_EYE_S == GEN_T - GEN_S);
   STATIC_ASSERT(STATE_TEXGEN_EYE_R - STATE_TEXGEN_EYE_S == GEN_R - GEN_S);
   STATIC_ASSERT(STATE_TEXGEN_EYE_Q - STATE_TEXGEN_EYE_S == GEN_Q - GEN_S);
   STATIC_ASSERT(offsetof(struct gl_fixedfunc_texture_unit, ObjectPlane) -
               STATIC_ASSERT(STATE_TEXGEN_OBJECT_T - STATE_TEXGEN_OBJECT_S == GEN_T - GEN_S);
                  const float *attr = (float*)ctx->Texture.FixedFuncUnit[unit].EyePlane +
         COPY_4V(value, attr);
         case STATE_TEXENV_COLOR:
      {
      /* state[1] is the texture unit */
   const GLuint unit = (GLuint) state[1];
   if (_mesa_get_clamp_fragment_color(ctx, ctx->DrawBuffer))
         else
      }
      case STATE_FOG_COLOR:
      if (_mesa_get_clamp_fragment_color(ctx, ctx->DrawBuffer))
         else
            case STATE_FOG_PARAMS: {
      float scale = 1.0f / (ctx->Fog.End - ctx->Fog.Start);
   /* Pass +-FLT_MAX/2 to the shader instead of +-Inf because Infs have
   * undefined behavior without GLSL 4.10 or GL_ARB_shader_precision
   * enabled. Infs also have undefined behavior with Shader Model 3.
   *
   * The division by 2 makes it less likely that ALU ops will generate
   * Inf.
   */
   scale = CLAMP(scale, FLT_MIN / 2, FLT_MAX / 2);
   value[0] = ctx->Fog.Density;
   value[1] = ctx->Fog.Start;
   value[2] = ctx->Fog.End;
   value[3] = scale;
      }
   case STATE_CLIPPLANE:
      {
      const GLuint plane = (GLuint) state[1];
      }
      case STATE_POINT_SIZE:
      value[0] = ctx->Point.Size;
   value[1] = ctx->Point.MinSize;
   value[2] = ctx->Point.MaxSize;
   value[3] = ctx->Point.Threshold;
      case STATE_POINT_ATTENUATION:
      value[0] = ctx->Point.Params[0];
   value[1] = ctx->Point.Params[1];
   value[2] = ctx->Point.Params[2];
   value[3] = 1.0F;
      /* state[0] = modelview, projection, texture, etc. */
   /* state[1] = which texture matrix or program matrix */
   /* state[2] = first row to fetch */
   /* state[3] = last row to fetch */
   case STATE_MODELVIEW_MATRIX: {
      const GLmatrix *matrix = ctx->ModelviewMatrixStack.Top;
   copy_matrix(value, matrix->m, state[2], state[3]);
      }
   case STATE_MODELVIEW_MATRIX_INVERSE: {
      const GLmatrix *matrix = ctx->ModelviewMatrixStack.Top;
   copy_matrix(value, matrix->inv, state[2], state[3]);
      }
   case STATE_MODELVIEW_MATRIX_TRANSPOSE: {
      const GLmatrix *matrix = ctx->ModelviewMatrixStack.Top;
   copy_matrix_transposed(value, matrix->m, state[2], state[3]);
      }
   case STATE_MODELVIEW_MATRIX_INVTRANS: {
      const GLmatrix *matrix = ctx->ModelviewMatrixStack.Top;
   copy_matrix_transposed(value, matrix->inv, state[2], state[3]);
      }
   case STATE_PROJECTION_MATRIX: {
      const GLmatrix *matrix = ctx->ProjectionMatrixStack.Top;
   copy_matrix(value, matrix->m, state[2], state[3]);
      }
   case STATE_PROJECTION_MATRIX_INVERSE: {
      GLmatrix *matrix = ctx->ProjectionMatrixStack.Top;
   _math_matrix_analyse(matrix); /* make sure the inverse is up to date */
   copy_matrix(value, matrix->inv, state[2], state[3]);
      }
   case STATE_PROJECTION_MATRIX_TRANSPOSE: {
      const GLmatrix *matrix = ctx->ProjectionMatrixStack.Top;
   copy_matrix_transposed(value, matrix->m, state[2], state[3]);
      }
   case STATE_PROJECTION_MATRIX_INVTRANS: {
      GLmatrix *matrix = ctx->ProjectionMatrixStack.Top;
   _math_matrix_analyse(matrix); /* make sure the inverse is up to date */
   copy_matrix_transposed(value, matrix->inv, state[2], state[3]);
      }
   case STATE_MVP_MATRIX: {
      const GLmatrix *matrix = &ctx->_ModelProjectMatrix;
   copy_matrix(value, matrix->m, state[2], state[3]);
      }
   case STATE_MVP_MATRIX_INVERSE: {
      GLmatrix *matrix = &ctx->_ModelProjectMatrix;
   _math_matrix_analyse(matrix); /* make sure the inverse is up to date */
   copy_matrix(value, matrix->inv, state[2], state[3]);
      }
   case STATE_MVP_MATRIX_TRANSPOSE: {
      const GLmatrix *matrix = &ctx->_ModelProjectMatrix;
   copy_matrix_transposed(value, matrix->m, state[2], state[3]);
      }
   case STATE_MVP_MATRIX_INVTRANS: {
      GLmatrix *matrix = &ctx->_ModelProjectMatrix;
   _math_matrix_analyse(matrix); /* make sure the inverse is up to date */
   copy_matrix_transposed(value, matrix->inv, state[2], state[3]);
      }
   case STATE_TEXTURE_MATRIX: {
      const GLuint index = (GLuint) state[1];
   assert(index < ARRAY_SIZE(ctx->TextureMatrixStack));
   const GLmatrix *matrix = ctx->TextureMatrixStack[index].Top;
   copy_matrix(value, matrix->m, state[2], state[3]);
      }
   case STATE_TEXTURE_MATRIX_INVERSE: {
      const GLuint index = (GLuint) state[1];
   assert(index < ARRAY_SIZE(ctx->TextureMatrixStack));
   const GLmatrix *matrix = ctx->TextureMatrixStack[index].Top;
   copy_matrix(value, matrix->inv, state[2], state[3]);
      }
   case STATE_TEXTURE_MATRIX_TRANSPOSE: {
      const GLuint index = (GLuint) state[1];
   assert(index < ARRAY_SIZE(ctx->TextureMatrixStack));
   const GLmatrix *matrix = ctx->TextureMatrixStack[index].Top;
   copy_matrix_transposed(value, matrix->m, state[2], state[3]);
      }
   case STATE_TEXTURE_MATRIX_INVTRANS: {
      const GLuint index = (GLuint) state[1];
   assert(index < ARRAY_SIZE(ctx->TextureMatrixStack));
   const GLmatrix *matrix = ctx->TextureMatrixStack[index].Top;
   copy_matrix_transposed(value, matrix->inv, state[2], state[3]);
      }
   case STATE_PROGRAM_MATRIX: {
      const GLuint index = (GLuint) state[1];
   assert(index < ARRAY_SIZE(ctx->ProgramMatrixStack));
   const GLmatrix *matrix = ctx->ProgramMatrixStack[index].Top;
   copy_matrix(value, matrix->m, state[2], state[3]);
      }
   case STATE_PROGRAM_MATRIX_INVERSE: {
      const GLuint index = (GLuint) state[1];
   assert(index < ARRAY_SIZE(ctx->ProgramMatrixStack));
   const GLmatrix *matrix = ctx->ProgramMatrixStack[index].Top;
   _math_matrix_analyse((GLmatrix*)matrix); /* Be sure inverse is up to date: */
   copy_matrix(value, matrix->inv, state[2], state[3]);
      }
   case STATE_PROGRAM_MATRIX_TRANSPOSE: {
      const GLuint index = (GLuint) state[1];
   assert(index < ARRAY_SIZE(ctx->ProgramMatrixStack));
   const GLmatrix *matrix = ctx->ProgramMatrixStack[index].Top;
   copy_matrix_transposed(value, matrix->m, state[2], state[3]);
      }
   case STATE_PROGRAM_MATRIX_INVTRANS: {
      const GLuint index = (GLuint) state[1];
   assert(index < ARRAY_SIZE(ctx->ProgramMatrixStack));
   const GLmatrix *matrix = ctx->ProgramMatrixStack[index].Top;
   _math_matrix_analyse((GLmatrix*)matrix); /* Be sure inverse is up to date: */
   copy_matrix_transposed(value, matrix->inv, state[2], state[3]);
      }
   case STATE_NUM_SAMPLES:
      val[0].i = MAX2(1, _mesa_geometric_samples(ctx->DrawBuffer));
      case STATE_DEPTH_RANGE:
      value[0] = ctx->ViewportArray[0].Near;                /* near       */
   value[1] = ctx->ViewportArray[0].Far;                 /* far        */
   value[2] = ctx->ViewportArray[0].Far - ctx->ViewportArray[0].Near; /* far - near */
   value[3] = 1.0;
      case STATE_FRAGMENT_PROGRAM_ENV: {
      const int idx = (int) state[1];
   COPY_4V(value, ctx->FragmentProgram.Parameters[idx]);
      }
   case STATE_FRAGMENT_PROGRAM_ENV_ARRAY: {
      const unsigned idx = state[1];
   const unsigned bytes = state[2] * 16;
   memcpy(value, ctx->FragmentProgram.Parameters[idx], bytes);
      }
   case STATE_FRAGMENT_PROGRAM_LOCAL: {
      float (*params)[4] = ctx->FragmentProgram.Current->arb.LocalParams;
   if (unlikely(!params)) {
      /* Local parameters haven't been allocated yet.
   * ARB_fragment_program says that local parameters are
   * "initially set to (0,0,0,0)." Return that.
   */
   memset(value, 0, sizeof(float) * 4);
               const int idx = (int) state[1];
   COPY_4V(value, params[idx]);
      }
   case STATE_FRAGMENT_PROGRAM_LOCAL_ARRAY: {
      const unsigned idx = state[1];
   const unsigned bytes = state[2] * 16;
   float (*params)[4] = ctx->FragmentProgram.Current->arb.LocalParams;
   if (!params) {
      /* Local parameters haven't been allocated yet.
   * ARB_fragment_program says that local parameters are
   * "initially set to (0,0,0,0)." Return that.
   */
   memset(value, 0, bytes);
      }
   memcpy(value, params[idx], bytes);
      }
   case STATE_VERTEX_PROGRAM_ENV: {
      const int idx = (int) state[1];
   COPY_4V(value, ctx->VertexProgram.Parameters[idx]);
      }
   case STATE_VERTEX_PROGRAM_ENV_ARRAY: {
      const unsigned idx = state[1];
   const unsigned bytes = state[2] * 16;
   memcpy(value, ctx->VertexProgram.Parameters[idx], bytes);
      }
   case STATE_VERTEX_PROGRAM_LOCAL: {
      float (*params)[4] = ctx->VertexProgram.Current->arb.LocalParams;
   if (unlikely(!params)) {
      /* Local parameters haven't been allocated yet.
   * ARB_vertex_program says that local parameters are
   * "initially set to (0,0,0,0)." Return that.
   */
   memset(value, 0, sizeof(float) * 4);
               const int idx = (int) state[1];
   COPY_4V(value, params[idx]);
      }
   case STATE_VERTEX_PROGRAM_LOCAL_ARRAY: {
      const unsigned idx = state[1];
   const unsigned bytes = state[2] * 16;
   float (*params)[4] = ctx->VertexProgram.Current->arb.LocalParams;
   if (!params) {
      /* Local parameters haven't been allocated yet.
   * ARB_vertex_program says that local parameters are
   * "initially set to (0,0,0,0)." Return that.
   */
   memset(value, 0, bytes);
      }
   memcpy(value, params[idx], bytes);
               case STATE_NORMAL_SCALE_EYESPACE:
      ASSIGN_4V(value, ctx->_ModelViewInvScaleEyespace, 0, 0, 1);
         case STATE_CURRENT_ATTRIB:
      {
      const GLuint idx = (GLuint) state[1];
      }
         case STATE_CURRENT_ATTRIB_MAYBE_VP_CLAMPED:
      {
      const GLuint idx = (GLuint) state[1];
   if(ctx->Light._ClampVertexColor &&
      (idx == VERT_ATTRIB_COLOR0 ||
   idx == VERT_ATTRIB_COLOR1)) {
   value[0] = SATURATE(ctx->Current.Attrib[idx][0]);
   value[1] = SATURATE(ctx->Current.Attrib[idx][1]);
   value[2] = SATURATE(ctx->Current.Attrib[idx][2]);
      }
   else
      }
         case STATE_NORMAL_SCALE:
      ASSIGN_4V(value,
            ctx->_ModelViewInvScale,
   ctx->_ModelViewInvScale,
            case STATE_FOG_PARAMS_OPTIMIZED: {
      /* for simpler per-vertex/pixel fog calcs. POW (for EXP/EXP2 fog)
   * might be more expensive than EX2 on some hw, plus it needs
   * another constant (e) anyway. Linear fog can now be done with a
   * single MAD.
   * linear: fogcoord * -1/(end-start) + end/(end-start)
   * exp: 2^-(density/ln(2) * fogcoord)
   * exp2: 2^-((density/(sqrt(ln(2))) * fogcoord)^2)
   */
   float val =  (ctx->Fog.End == ctx->Fog.Start)
         value[0] = val;
   value[1] = ctx->Fog.End * -val;
   value[2] = (GLfloat)(ctx->Fog.Density * M_LOG2E); /* M_LOG2E == 1/ln(2) */
   value[3] = (GLfloat)(ctx->Fog.Density * ONE_DIV_SQRT_LN2);
               case STATE_POINT_SIZE_CLAMPED:
      {
   /* this includes implementation dependent limits, to avoid
      * another potentially necessary clamp.
   * Note: for sprites, point smooth (point AA) is ignored
   * and we'll clamp to MinPointSizeAA and MaxPointSize, because we
   * expect drivers will want to say their minimum for AA size is 0.0
   * but for non-AA it's 1.0 (because normal points with size below 1.0
   * need to get rounded up to 1.0, hence never disappear). GL does
   * not specify max clamp size for sprites, other than it needs to be
   * at least as large as max AA size, hence use non-AA size there.
   */
   GLfloat minImplSize;
   GLfloat maxImplSize;
   if (ctx->Point.PointSprite) {
      minImplSize = ctx->Const.MinPointSizeAA;
      }
   else if (ctx->Point.SmoothFlag || _mesa_is_multisample_enabled(ctx)) {
      minImplSize = ctx->Const.MinPointSizeAA;
      }
   else {
      minImplSize = ctx->Const.MinPointSize;
      }
   value[0] = ctx->Point.Size;
   value[1] = ctx->Point.MinSize >= minImplSize ? ctx->Point.MinSize : minImplSize;
   value[2] = ctx->Point.MaxSize <= maxImplSize ? ctx->Point.MaxSize : maxImplSize;
      }
      case STATE_LIGHT_SPOT_DIR_NORMALIZED:
      {
      /* here, state[1] is the light number */
   /* pre-normalize spot dir */
   const GLuint ln = (GLuint) state[1];
   COPY_3V(value, ctx->Light.Light[ln]._NormSpotDirection);
      }
         case STATE_LIGHT_POSITION:
      {
      const GLuint ln = (GLuint) state[1];
      }
         case STATE_LIGHT_POSITION_ARRAY: {
      const unsigned first = state[1];
   const unsigned num_lights = state[2];
   for (unsigned i = 0; i < num_lights; i++) {
      COPY_4V(value, ctx->Light.Light[first + i]._Position);
      }
               case STATE_LIGHT_POSITION_NORMALIZED:
      {
      const GLuint ln = (GLuint) state[1];
   float p[4];
   COPY_4V(p, ctx->Light.Light[ln]._Position);
   NORMALIZE_3FV(p);
      }
         case STATE_LIGHT_POSITION_NORMALIZED_ARRAY: {
      const unsigned first = state[1];
   const unsigned num_lights = state[2];
   for (unsigned i = 0; i < num_lights; i++) {
      float p[4];
   COPY_4V(p, ctx->Light.Light[first + i]._Position);
   NORMALIZE_3FV(p);
   COPY_4V(value, p);
      }
               case STATE_LIGHT_HALF_VECTOR:
      {
      const GLuint ln = (GLuint) state[1];
   GLfloat p[3];
   /* Compute infinite half angle vector:
   *   halfVector = normalize(normalize(lightPos) + (0, 0, 1))
   * light.EyePosition.w should be 0 for infinite lights.
   */
   COPY_3V(p, ctx->Light.Light[ln]._Position);
   NORMALIZE_3FV(p);
   ADD_3V(p, p, ctx->_EyeZDir);
   NORMALIZE_3FV(p);
   COPY_3V(value, p);
      }
         case STATE_PT_SCALE:
      value[0] = ctx->Pixel.RedScale;
   value[1] = ctx->Pixel.GreenScale;
   value[2] = ctx->Pixel.BlueScale;
   value[3] = ctx->Pixel.AlphaScale;
         case STATE_PT_BIAS:
      value[0] = ctx->Pixel.RedBias;
   value[1] = ctx->Pixel.GreenBias;
   value[2] = ctx->Pixel.BlueBias;
   value[3] = ctx->Pixel.AlphaBias;
         case STATE_FB_SIZE:
      value[0] = (GLfloat) (ctx->DrawBuffer->Width - 1);
   value[1] = (GLfloat) (ctx->DrawBuffer->Height - 1);
   value[2] = 0.0F;
   value[3] = 0.0F;
         case STATE_FB_WPOS_Y_TRANSFORM:
      /* A driver may negate this conditional by using ZW swizzle
   * instead of XY (based on e.g. some other state). */
   if (!ctx->DrawBuffer->FlipY) {
      /* Identity (XY) followed by flipping Y upside down (ZW). */
   value[0] = 1.0F;
   value[1] = 0.0F;
   value[2] = -1.0F;
      } else {
      /* Flipping Y upside down (XY) followed by identity (ZW). */
   value[0] = -1.0F;
   value[1] = _mesa_geometric_height(ctx->DrawBuffer);
   value[2] = 1.0F;
      }
         case STATE_FB_PNTC_Y_TRANSFORM:
      {
                     value[0] = flip_y ? -1.0F : 1.0F;
   value[1] = flip_y ? 1.0F : 0.0F;
   value[2] = 0.0F;
      }
         case STATE_TCS_PATCH_VERTICES_IN:
      val[0].i = ctx->TessCtrlProgram.patch_vertices;
         case STATE_TES_PATCH_VERTICES_IN:
      if (ctx->TessCtrlProgram._Current)
         else
               case STATE_ADVANCED_BLENDING_MODE:
      val[0].i = _mesa_get_advanced_blend_sh_constant(
               case STATE_ALPHA_REF:
      value[0] = ctx->Color.AlphaRefUnclamped;
         case STATE_CLIP_INTERNAL:
      {
      const GLuint plane = (GLuint) state[1];
      }
         case STATE_ATOMIC_COUNTER_OFFSET:
      {
      const GLuint counter = (GLuint) state[1];
      }
         }
      unsigned
   _mesa_program_state_value_size(const gl_state_index16 state[STATE_LENGTH])
   {
      if (state[0] == STATE_LIGHT && state[2] == STATE_SPOT_CUTOFF)
            /* Everything else is packed into vec4s */
      }
      /**
   * Return a bitmask of the Mesa state flags (_NEW_* values) which would
   * indicate that the given context state may have changed.
   * The bitmask is used during validation to determine if we need to update
   * vertex/fragment program parameters (like "state.material.color") when
   * some GL state has changed.
   */
   GLbitfield
   _mesa_program_state_flags(const gl_state_index16 state[STATE_LENGTH])
   {
      switch (state[0]) {
   case STATE_MATERIAL:
            case STATE_LIGHTPROD:
   case STATE_LIGHTPROD_ARRAY_FRONT:
   case STATE_LIGHTPROD_ARRAY_BACK:
   case STATE_LIGHTPROD_ARRAY_TWOSIDE:
   case STATE_LIGHTMODEL_SCENECOLOR:
            case STATE_LIGHT:
   case STATE_LIGHT_ARRAY:
   case STATE_LIGHT_ATTENUATION_ARRAY:
   case STATE_LIGHTMODEL_AMBIENT:
   case STATE_LIGHT_SPOT_DIR_NORMALIZED:
   case STATE_LIGHT_POSITION:
   case STATE_LIGHT_POSITION_ARRAY:
   case STATE_LIGHT_POSITION_NORMALIZED:
   case STATE_LIGHT_POSITION_NORMALIZED_ARRAY:
   case STATE_LIGHT_HALF_VECTOR:
            case STATE_TEXGEN:
         case STATE_TEXENV_COLOR:
            case STATE_FOG_COLOR:
         case STATE_FOG_PARAMS:
   case STATE_FOG_PARAMS_OPTIMIZED:
            case STATE_CLIPPLANE:
            case STATE_POINT_SIZE:
   case STATE_POINT_ATTENUATION:
            case STATE_MODELVIEW_MATRIX:
   case STATE_MODELVIEW_MATRIX_INVERSE:
   case STATE_MODELVIEW_MATRIX_TRANSPOSE:
   case STATE_MODELVIEW_MATRIX_INVTRANS:
   case STATE_NORMAL_SCALE_EYESPACE:
   case STATE_NORMAL_SCALE:
            case STATE_PROJECTION_MATRIX:
   case STATE_PROJECTION_MATRIX_INVERSE:
   case STATE_PROJECTION_MATRIX_TRANSPOSE:
   case STATE_PROJECTION_MATRIX_INVTRANS:
         case STATE_MVP_MATRIX:
   case STATE_MVP_MATRIX_INVERSE:
   case STATE_MVP_MATRIX_TRANSPOSE:
   case STATE_MVP_MATRIX_INVTRANS:
         case STATE_TEXTURE_MATRIX:
   case STATE_TEXTURE_MATRIX_INVERSE:
   case STATE_TEXTURE_MATRIX_TRANSPOSE:
   case STATE_TEXTURE_MATRIX_INVTRANS:
         case STATE_PROGRAM_MATRIX:
   case STATE_PROGRAM_MATRIX_INVERSE:
   case STATE_PROGRAM_MATRIX_TRANSPOSE:
   case STATE_PROGRAM_MATRIX_INVTRANS:
            case STATE_NUM_SAMPLES:
   case STATE_FB_SIZE:
   case STATE_FB_WPOS_Y_TRANSFORM:
            case STATE_FB_PNTC_Y_TRANSFORM:
            case STATE_DEPTH_RANGE:
            case STATE_FRAGMENT_PROGRAM_ENV:
   case STATE_FRAGMENT_PROGRAM_ENV_ARRAY:
   case STATE_FRAGMENT_PROGRAM_LOCAL:
   case STATE_FRAGMENT_PROGRAM_LOCAL_ARRAY:
   case STATE_VERTEX_PROGRAM_ENV:
   case STATE_VERTEX_PROGRAM_ENV_ARRAY:
   case STATE_VERTEX_PROGRAM_LOCAL:
   case STATE_VERTEX_PROGRAM_LOCAL_ARRAY:
            case STATE_CURRENT_ATTRIB:
         case STATE_CURRENT_ATTRIB_MAYBE_VP_CLAMPED:
            case STATE_POINT_SIZE_CLAMPED:
            case STATE_PT_SCALE:
   case STATE_PT_BIAS:
            case STATE_ADVANCED_BLENDING_MODE:
   case STATE_ALPHA_REF:
            case STATE_CLIP_INTERNAL:
            /* Needs to return any nonzero value to trigger constant updating */
   case STATE_ATOMIC_COUNTER_OFFSET:
            case STATE_TCS_PATCH_VERTICES_IN:
   case STATE_TES_PATCH_VERTICES_IN:
   case STATE_INTERNAL_DRIVER:
            case STATE_NOT_STATE_VAR:
            default:
      _mesa_problem(NULL, "unexpected state[0] in make_state_flags()");
         }
         static void
   append(char *dst, const char *src)
   {
      while (*dst)
         while (*src)
   *dst++ = *src++;
      }
         /**
   * Convert token 'k' to a string, append it onto 'dst' string.
   */
   static void
   append_token(char *dst, gl_state_index k)
   {
      switch (k) {
   case STATE_MATERIAL:
      append(dst, "material");
      case STATE_LIGHT:
      append(dst, "light");
      case STATE_LIGHT_ARRAY:
      append(dst, "light.array");
      case STATE_LIGHT_ATTENUATION_ARRAY:
      append(dst, "light.attenuation");
      case STATE_LIGHTMODEL_AMBIENT:
      append(dst, "lightmodel.ambient");
      case STATE_LIGHTMODEL_SCENECOLOR:
         case STATE_LIGHTPROD:
      append(dst, "lightprod");
      case STATE_LIGHTPROD_ARRAY_FRONT:
      append(dst, "lightprod.array.front");
      case STATE_LIGHTPROD_ARRAY_BACK:
      append(dst, "lightprod.array.back");
      case STATE_LIGHTPROD_ARRAY_TWOSIDE:
      append(dst, "lightprod.array.twoside");
      case STATE_TEXGEN:
      append(dst, "texgen");
      case STATE_FOG_COLOR:
      append(dst, "fog.color");
      case STATE_FOG_PARAMS:
      append(dst, "fog.params");
      case STATE_CLIPPLANE:
      append(dst, "clip");
      case STATE_POINT_SIZE:
      append(dst, "point.size");
      case STATE_POINT_ATTENUATION:
      append(dst, "point.attenuation");
      case STATE_MODELVIEW_MATRIX:
      append(dst, "matrix.modelview.");
      case STATE_MODELVIEW_MATRIX_INVERSE:
      append(dst, "matrix.modelview.inverse.");
      case STATE_MODELVIEW_MATRIX_TRANSPOSE:
      append(dst, "matrix.modelview.transpose.");
      case STATE_MODELVIEW_MATRIX_INVTRANS:
      append(dst, "matrix.modelview.invtrans.");
      case STATE_PROJECTION_MATRIX:
      append(dst, "matrix.projection.");
      case STATE_PROJECTION_MATRIX_INVERSE:
      append(dst, "matrix.projection.inverse.");
      case STATE_PROJECTION_MATRIX_TRANSPOSE:
      append(dst, "matrix.projection.transpose.");
      case STATE_PROJECTION_MATRIX_INVTRANS:
      append(dst, "matrix.projection.invtrans.");
      case STATE_MVP_MATRIX:
      append(dst, "matrix.mvp.");
      case STATE_MVP_MATRIX_INVERSE:
      append(dst, "matrix.mvp.inverse.");
      case STATE_MVP_MATRIX_TRANSPOSE:
      append(dst, "matrix.mvp.transpose.");
      case STATE_MVP_MATRIX_INVTRANS:
      append(dst, "matrix.mvp.invtrans.");
      case STATE_TEXTURE_MATRIX:
      append(dst, "matrix.texture");
      case STATE_TEXTURE_MATRIX_INVERSE:
      append(dst, "matrix.texture.inverse");
      case STATE_TEXTURE_MATRIX_TRANSPOSE:
      append(dst, "matrix.texture.transpose");
      case STATE_TEXTURE_MATRIX_INVTRANS:
      append(dst, "matrix.texture.invtrans");
      case STATE_PROGRAM_MATRIX:
      append(dst, "matrix.program");
      case STATE_PROGRAM_MATRIX_INVERSE:
      append(dst, "matrix.program.inverse");
      case STATE_PROGRAM_MATRIX_TRANSPOSE:
      append(dst, "matrix.program.transpose");
      case STATE_PROGRAM_MATRIX_INVTRANS:
      append(dst, "matrix.program.invtrans");
   break;
      case STATE_AMBIENT:
      append(dst, "ambient");
      case STATE_DIFFUSE:
      append(dst, "diffuse");
      case STATE_SPECULAR:
      append(dst, "specular");
      case STATE_EMISSION:
      append(dst, "emission");
      case STATE_SHININESS:
      append(dst, "shininess");
      case STATE_HALF_VECTOR:
      append(dst, "half");
      case STATE_POSITION:
      append(dst, "position");
      case STATE_ATTENUATION:
      append(dst, "attenuation");
      case STATE_SPOT_DIRECTION:
      append(dst, "spot.direction");
      case STATE_SPOT_CUTOFF:
      append(dst, "spot.cutoff");
      case STATE_TEXGEN_EYE_S:
      append(dst, "eye.s");
      case STATE_TEXGEN_EYE_T:
      append(dst, "eye.t");
      case STATE_TEXGEN_EYE_R:
      append(dst, "eye.r");
      case STATE_TEXGEN_EYE_Q:
      append(dst, "eye.q");
      case STATE_TEXGEN_OBJECT_S:
      append(dst, "object.s");
      case STATE_TEXGEN_OBJECT_T:
      append(dst, "object.t");
      case STATE_TEXGEN_OBJECT_R:
      append(dst, "object.r");
      case STATE_TEXGEN_OBJECT_Q:
      append(dst, "object.q");
      case STATE_TEXENV_COLOR:
      append(dst, "texenv");
      case STATE_NUM_SAMPLES:
      append(dst, "numsamples");
      case STATE_DEPTH_RANGE:
      append(dst, "depth.range");
      case STATE_VERTEX_PROGRAM_ENV:
   case STATE_FRAGMENT_PROGRAM_ENV:
      append(dst, "env");
      case STATE_VERTEX_PROGRAM_ENV_ARRAY:
   case STATE_FRAGMENT_PROGRAM_ENV_ARRAY:
      append(dst, "env.range");
      case STATE_VERTEX_PROGRAM_LOCAL:
   case STATE_FRAGMENT_PROGRAM_LOCAL:
      append(dst, "local");
      case STATE_VERTEX_PROGRAM_LOCAL_ARRAY:
   case STATE_FRAGMENT_PROGRAM_LOCAL_ARRAY:
      append(dst, "local.range");
      case STATE_CURRENT_ATTRIB:
      append(dst, "current");
      case STATE_CURRENT_ATTRIB_MAYBE_VP_CLAMPED:
      append(dst, "currentAttribMaybeVPClamped");
      case STATE_NORMAL_SCALE_EYESPACE:
      append(dst, "normalScaleEyeSpace");
      case STATE_NORMAL_SCALE:
      append(dst, "normalScale");
      case STATE_FOG_PARAMS_OPTIMIZED:
      append(dst, "fogParamsOptimized");
      case STATE_POINT_SIZE_CLAMPED:
      append(dst, "pointSizeClamped");
      case STATE_LIGHT_SPOT_DIR_NORMALIZED:
      append(dst, "lightSpotDirNormalized");
      case STATE_LIGHT_POSITION:
      append(dst, "light.position");
      case STATE_LIGHT_POSITION_ARRAY:
      append(dst, "light.position.array");
      case STATE_LIGHT_POSITION_NORMALIZED:
      append(dst, "light.position.normalized");
      case STATE_LIGHT_POSITION_NORMALIZED_ARRAY:
      append(dst, "light.position.normalized.array");
      case STATE_LIGHT_HALF_VECTOR:
      append(dst, "lightHalfVector");
      case STATE_PT_SCALE:
      append(dst, "PTscale");
      case STATE_PT_BIAS:
      append(dst, "PTbias");
      case STATE_FB_SIZE:
      append(dst, "FbSize");
      case STATE_FB_WPOS_Y_TRANSFORM:
      append(dst, "FbWposYTransform");
      case STATE_FB_PNTC_Y_TRANSFORM:
      append(dst, "PntcYTransform");
      case STATE_ADVANCED_BLENDING_MODE:
      append(dst, "AdvancedBlendingMode");
      case STATE_ALPHA_REF:
      append(dst, "alphaRef");
      case STATE_CLIP_INTERNAL:
      append(dst, "clipInternal");
      case STATE_ATOMIC_COUNTER_OFFSET:
      append(dst, "counterOffset");
      default:
      /* probably STATE_INTERNAL_DRIVER+i (driver private state) */
         }
      static void
   append_index(char *dst, GLint index, bool structure)
   {
      char s[20];
   sprintf(s, "[%d]%s", index, structure ? "." : "");
      }
      /**
   * Make a string from the given state vector.
   * For example, return "state.matrix.texture[2].inverse".
   * Use free() to deallocate the string.
   */
   char *
   _mesa_program_state_string(const gl_state_index16 state[STATE_LENGTH])
   {
      char str[1000] = "";
            append(str, "state.");
            switch (state[0]) {
   case STATE_LIGHT:
      append_index(str, state[1], true); /* light number [i]. */
   append_token(str, state[2]); /* coefficients */
      case STATE_LIGHTMODEL_AMBIENT:
         case STATE_LIGHTMODEL_SCENECOLOR:
      if (state[1] == 0) {
         }
   else {
         }
      case STATE_LIGHTPROD:
      append_index(str, state[1], false); /* light number [i] */
   append_index(str, state[2], false);
      case STATE_TEXGEN:
      append_index(str, state[1], true); /* tex unit [i] */
   append_token(str, state[2]); /* plane coef */
      case STATE_TEXENV_COLOR:
      append_index(str, state[1], true); /* tex unit [i] */
   append(str, "color");
      case STATE_CLIPPLANE:
      append_index(str, state[1], true); /* plane [i] */
   append(str, "plane");
      case STATE_MODELVIEW_MATRIX:
   case STATE_MODELVIEW_MATRIX_INVERSE:
   case STATE_MODELVIEW_MATRIX_TRANSPOSE:
   case STATE_MODELVIEW_MATRIX_INVTRANS:
   case STATE_PROJECTION_MATRIX:
   case STATE_PROJECTION_MATRIX_INVERSE:
   case STATE_PROJECTION_MATRIX_TRANSPOSE:
   case STATE_PROJECTION_MATRIX_INVTRANS:
   case STATE_MVP_MATRIX:
   case STATE_MVP_MATRIX_INVERSE:
   case STATE_MVP_MATRIX_TRANSPOSE:
   case STATE_MVP_MATRIX_INVTRANS:
   case STATE_TEXTURE_MATRIX:
   case STATE_TEXTURE_MATRIX_INVERSE:
   case STATE_TEXTURE_MATRIX_TRANSPOSE:
   case STATE_TEXTURE_MATRIX_INVTRANS:
   case STATE_PROGRAM_MATRIX:
   case STATE_PROGRAM_MATRIX_INVERSE:
   case STATE_PROGRAM_MATRIX_TRANSPOSE:
   case STATE_PROGRAM_MATRIX_INVTRANS:
      {
      /* state[0] = modelview, projection, texture, etc. */
   /* state[1] = which texture matrix or program matrix */
   /* state[2] = first row to fetch */
   /* state[3] = last row to fetch */
   const gl_state_index mat = state[0];
   const GLuint index = (GLuint) state[1];
   const GLuint firstRow = (GLuint) state[2];
   const GLuint lastRow = (GLuint) state[3];
   if (index ||
      (mat >= STATE_TEXTURE_MATRIX &&
   mat <= STATE_PROGRAM_MATRIX_INVTRANS))
      if (firstRow == lastRow)
         else
            }
      case STATE_LIGHT_ARRAY:
   case STATE_LIGHT_ATTENUATION_ARRAY:
   case STATE_FRAGMENT_PROGRAM_ENV_ARRAY:
   case STATE_FRAGMENT_PROGRAM_LOCAL_ARRAY:
   case STATE_VERTEX_PROGRAM_ENV_ARRAY:
   case STATE_VERTEX_PROGRAM_LOCAL_ARRAY:
   case STATE_LIGHTPROD_ARRAY_FRONT:
   case STATE_LIGHTPROD_ARRAY_BACK:
   case STATE_LIGHTPROD_ARRAY_TWOSIDE:
   case STATE_LIGHT_POSITION_ARRAY:
   case STATE_LIGHT_POSITION_NORMALIZED_ARRAY:
      sprintf(tmp, "[%d..%d]", state[1], state[1] + state[2] - 1);
   append(str, tmp);
      case STATE_MATERIAL:
   case STATE_FRAGMENT_PROGRAM_ENV:
   case STATE_FRAGMENT_PROGRAM_LOCAL:
   case STATE_VERTEX_PROGRAM_ENV:
   case STATE_VERTEX_PROGRAM_LOCAL:
   case STATE_CURRENT_ATTRIB:
   case STATE_CURRENT_ATTRIB_MAYBE_VP_CLAMPED:
   case STATE_LIGHT_SPOT_DIR_NORMALIZED:
   case STATE_LIGHT_POSITION:
   case STATE_LIGHT_POSITION_NORMALIZED:
   case STATE_LIGHT_HALF_VECTOR:
   case STATE_CLIP_INTERNAL:
   case STATE_ATOMIC_COUNTER_OFFSET:
      append_index(str, state[1], false);
      case STATE_POINT_SIZE:
   case STATE_POINT_ATTENUATION:
   case STATE_FOG_PARAMS:
   case STATE_FOG_COLOR:
   case STATE_NUM_SAMPLES:
   case STATE_DEPTH_RANGE:
   case STATE_NORMAL_SCALE_EYESPACE:
   case STATE_NORMAL_SCALE:
   case STATE_FOG_PARAMS_OPTIMIZED:
   case STATE_POINT_SIZE_CLAMPED:
   case STATE_PT_SCALE:
   case STATE_PT_BIAS:
   case STATE_FB_SIZE:
   case STATE_FB_WPOS_Y_TRANSFORM:
   case STATE_FB_PNTC_Y_TRANSFORM:
   case STATE_TCS_PATCH_VERTICES_IN:
   case STATE_TES_PATCH_VERTICES_IN:
   case STATE_ADVANCED_BLENDING_MODE:
   case STATE_ALPHA_REF:
         case STATE_NOT_STATE_VAR:
      append(str, "not_state");
      default:
      _mesa_problem(NULL, "Invalid state in _mesa_program_state_string: %d", state[0]);
                  }
         /**
   * Loop over all the parameters in a parameter list.  If the parameter
   * is a GL state reference, look up the current value of that state
   * variable and put it into the parameter's Value[4] array.
   * Other parameter types never change or are explicitly set by the user
   * with glUniform() or glProgramParameter(), etc.
   * This would be called at glBegin time.
   */
   void
   _mesa_load_state_parameters(struct gl_context *ctx,
         {
      if (!paramList)
                     for (int i = paramList->FirstStateVarIndex; i <= last; i++) {
      unsigned pvo = paramList->Parameters[i].ValueOffset;
   fetch_state(ctx, paramList->Parameters[i].StateIndexes,
         }
      void
   _mesa_upload_state_parameters(struct gl_context *ctx,
               {
               for (int i = paramList->FirstStateVarIndex; i <= last; i++) {
      unsigned pvo = paramList->Parameters[i].ValueOffset;
   fetch_state(ctx, paramList->Parameters[i].StateIndexes,
         }
      /* Merge consecutive state vars into one for the state vars that allow
   * multiple vec4s.
   *
   * This should be done after shader compilation, so that drivers don't
   * have to deal with multi-slot state parameters in their backends.
   * It's only meant to optimize _mesa_load/upload_state_parameters.
   */
   void
   _mesa_optimize_state_parameters(struct gl_constants *consts,
         {
      for (int first_param = list->FirstStateVarIndex;
      first_param < (int)list->NumParameters; first_param++) {
   int last_param = first_param;
            switch (list->Parameters[first_param].StateIndexes[0]) {
   case STATE_MODELVIEW_MATRIX:
   case STATE_MODELVIEW_MATRIX_INVERSE:
   case STATE_MODELVIEW_MATRIX_TRANSPOSE:
   case STATE_MODELVIEW_MATRIX_INVTRANS:
   case STATE_PROJECTION_MATRIX:
   case STATE_PROJECTION_MATRIX_INVERSE:
   case STATE_PROJECTION_MATRIX_TRANSPOSE:
   case STATE_PROJECTION_MATRIX_INVTRANS:
   case STATE_MVP_MATRIX:
   case STATE_MVP_MATRIX_INVERSE:
   case STATE_MVP_MATRIX_TRANSPOSE:
   case STATE_MVP_MATRIX_INVTRANS:
   case STATE_TEXTURE_MATRIX:
   case STATE_TEXTURE_MATRIX_INVERSE:
   case STATE_TEXTURE_MATRIX_TRANSPOSE:
   case STATE_TEXTURE_MATRIX_INVTRANS:
   case STATE_PROGRAM_MATRIX:
   case STATE_PROGRAM_MATRIX_INVERSE:
   case STATE_PROGRAM_MATRIX_TRANSPOSE:
   case STATE_PROGRAM_MATRIX_INVTRANS:
      /* Skip unaligned state vars. */
                  /* Search for adjacent state vars that refer to adjacent rows. */
   for (int i = first_param + 1; i < (int)list->NumParameters; i++) {
      if (list->Parameters[i].StateIndexes[0] ==
      list->Parameters[i - 1].StateIndexes[0] &&
   list->Parameters[i].StateIndexes[1] ==
   list->Parameters[i - 1].StateIndexes[1] &&
   list->Parameters[i].StateIndexes[2] ==         /* FirstRow */
   list->Parameters[i - 1].StateIndexes[3] + 1 && /* LastRow + 1 */
   list->Parameters[i].Size == 4) {
   last_param = i;
      }
      }
   if (last_param > first_param) {
                                    /* Update LastRow. */
                                 case STATE_LIGHT:
      /* Skip trimmed state vars. (this shouldn't occur though) */
   if (list->Parameters[first_param].Size !=
                  /* Search for light attributes that are adjacent in memory. */
   for (int i = first_param + 1; i < (int)list->NumParameters; i++) {
      if (list->Parameters[i].StateIndexes[0] == STATE_LIGHT &&
      /* Consecutive attributes of the same light: */
   ((list->Parameters[i].StateIndexes[1] ==
      list->Parameters[i - 1].StateIndexes[1] &&
   list->Parameters[i].StateIndexes[2] ==
      /* Consecutive attributes between 2 lights: */
   /* SPOT_CUTOFF should have only 1 component, which isn't true
         (consts->PackedDriverUniformStorage &&
      list->Parameters[i].StateIndexes[1] ==
   list->Parameters[i - 1].StateIndexes[1] + 1 &&
   list->Parameters[i].StateIndexes[2] == STATE_AMBIENT &&
      last_param = i;
      }
      }
   if (last_param > first_param) {
      /* Convert the state var to STATE_LIGHT_ARRAY. */
   list->Parameters[first_param].StateIndexes[0] = STATE_LIGHT_ARRAY;
   /* Set the offset in floats. */
   list->Parameters[first_param].StateIndexes[1] =
                        /* Set the real size in floats that we will upload (memcpy). */
   list->Parameters[first_param].StateIndexes[2] =
                        /* Set the allocated size, which can be aligned to 4 components. */
   list->Parameters[first_param].Size =
                        param_diff = last_param - first_param;
               /* We were not able to convert light attributes to STATE_LIGHT_ARRAY.
   * Another occuring pattern is light attentuation vectors placed back
   * to back. Find them.
   */
   if (list->Parameters[first_param].StateIndexes[2] == STATE_ATTENUATION) {
      for (int i = first_param + 1; i < (int)list->NumParameters; i++) {
      if (list->Parameters[i].StateIndexes[0] == STATE_LIGHT &&
      /* Consecutive light: */
   list->Parameters[i].StateIndexes[1] ==
   list->Parameters[i - 1].StateIndexes[1] + 1 &&
   /* Same attribute: */
   list->Parameters[i].StateIndexes[2] ==
   list->Parameters[i - 1].StateIndexes[2]) {
   last_param = i;
      }
      }
                     /* Convert the state var to STATE_LIGHT_ATTENUATION_ARRAY. */
   list->Parameters[first_param].StateIndexes[0] =
         /* Keep the light index the same. */
   /* Set the number of lights. */
   unsigned size = param_diff + 1;
   list->Parameters[first_param].StateIndexes[2] = size;
   list->Parameters[first_param].Size = size * 4;
                     case STATE_VERTEX_PROGRAM_ENV:
   case STATE_VERTEX_PROGRAM_LOCAL:
   case STATE_FRAGMENT_PROGRAM_ENV:
   case STATE_FRAGMENT_PROGRAM_LOCAL:
                     /* Search for adjacent mergeable state vars. */
   for (int i = first_param + 1; i < (int)list->NumParameters; i++) {
      if (list->Parameters[i].StateIndexes[0] ==
      list->Parameters[i - 1].StateIndexes[0] &&
   list->Parameters[i].StateIndexes[1] ==
   list->Parameters[i - 1].StateIndexes[1] + 1 &&
   list->Parameters[i].Size == 4) {
   last_param = i;
      }
      }
   if (last_param > first_param) {
      /* Set STATE_xxx_RANGE. */
   STATIC_ASSERT(STATE_VERTEX_PROGRAM_ENV + 1 ==
         STATIC_ASSERT(STATE_VERTEX_PROGRAM_LOCAL + 1 ==
         STATIC_ASSERT(STATE_FRAGMENT_PROGRAM_ENV + 1 ==
         STATIC_ASSERT(STATE_FRAGMENT_PROGRAM_LOCAL + 1 ==
                           /* Set the size. */
   unsigned size = param_diff + 1;
   list->Parameters[first_param].StateIndexes[2] = size;
                  case STATE_LIGHTPROD: {
                                    for (unsigned state_iter = STATE_LIGHTPROD_ARRAY_FRONT;
                     if (state_iter == STATE_LIGHTPROD_ARRAY_FRONT)  {
      num_attribs = 3;
   base_attrib = MAT_ATTRIB_FRONT_AMBIENT;
      } else if (state_iter == STATE_LIGHTPROD_ARRAY_BACK) {
      num_attribs = 3;
   base_attrib = MAT_ATTRIB_BACK_AMBIENT;
      } else if (state_iter == STATE_LIGHTPROD_ARRAY_TWOSIDE) {
      num_attribs = 6;
                     /* Find all attributes for one light. */
   while (first_param + (num_lights + 1) * num_attribs <=
                           /* Consecutive light indices: */
   if (list->Parameters[first_param].StateIndexes[1] + num_lights ==
      list->Parameters[base].StateIndexes[1]) {
   for (i = 0; i < num_attribs; i++) {
      if (list->Parameters[base + i].StateIndexes[0] ==
      STATE_LIGHTPROD &&
   list->Parameters[base + i].Size == 4 &&
   /* Equal light indices: */
   list->Parameters[base + i].StateIndexes[1] ==
   list->Parameters[base + 0].StateIndexes[1] &&
   /* Consecutive attributes: */
   list->Parameters[base + i].StateIndexes[2] ==
   base_attrib + i * attrib_incr)
            }
   if (i == num_attribs) {
      /* Accept all parameters for merging. */
   state = state_iter;
   last_param = base + num_attribs - 1;
      } else {
                                          list->Parameters[first_param].StateIndexes[0] = state;
   list->Parameters[first_param].StateIndexes[2] = num_lights;
      }
               case STATE_LIGHT_POSITION:
   case STATE_LIGHT_POSITION_NORMALIZED:
                     for (int i = first_param + 1; i < (int)list->NumParameters; i++) {
      if (list->Parameters[i].StateIndexes[0] ==
      list->Parameters[i - 1].StateIndexes[0] &&
   /* Consecutive light: */
   list->Parameters[i].StateIndexes[1] ==
   list->Parameters[i - 1].StateIndexes[1] + 1) {
   last_param = i;
      }
      }
                     /* Convert the state var to STATE_LIGHT_POSITION_*ARRAY. */
   STATIC_ASSERT(STATE_LIGHT_POSITION + 1 ==
         STATIC_ASSERT(STATE_LIGHT_POSITION_NORMALIZED + 1 ==
         list->Parameters[first_param].StateIndexes[0]++;
   /* Keep the light index the same. */
   unsigned size = param_diff + 1;
   /* Set the number of lights. */
   list->Parameters[first_param].StateIndexes[2] = size;
                  if (param_diff) {
      /* Update the name. */
   free((void*)list->Parameters[first_param].Name);
                  /* Free names that we are going to overwrite. */
                  /* Remove the merged state vars. */
   if (last_param + 1 < list->NumParameters) {
      memmove(&list->Parameters[first_param + 1],
         &list->Parameters[last_param + 1],
      }
                     }
