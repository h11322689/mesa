   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * \file ffvertex_prog.c
   *
   * Create a vertex program to execute the current fixed function T&L pipeline.
   * \author Keith Whitwell
   */
         #include "main/errors.h"
   #include "util/glheader.h"
   #include "main/mtypes.h"
   #include "main/macros.h"
   #include "main/enums.h"
   #include "main/context.h"
   #include "main/ffvertex_prog.h"
   #include "program/program.h"
   #include "program/prog_cache.h"
   #include "program/prog_statevars.h"
   #include "util/bitscan.h"
      #include "state_tracker/st_program.h"
   #include "state_tracker/st_nir.h"
      #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_builtin_builder.h"
      /** Max of number of lights and texture coord units */
   #define NUM_UNITS MAX2(MAX_TEXTURE_COORD_UNITS, MAX_LIGHTS)
      struct state_key {
                        unsigned light_color_material_mask:12;
   unsigned light_global_enabled:1;
   unsigned light_local_viewer:1;
   unsigned light_twoside:1;
   unsigned material_shininess_is_zero:1;
   unsigned need_eye_coords:1;
   unsigned normalize:1;
            unsigned fog_distance_mode:2;
   unsigned separate_specular:1;
            struct {
      unsigned char light_enabled:1;
   unsigned char light_eyepos3_is_zero:1;
   unsigned char light_spotcutoff_is_180:1;
   unsigned char light_attenuated:1;
   unsigned char texmat_enabled:1;
   unsigned char coord_replace:1;
   unsigned char texgen_enabled:1;
   unsigned char texgen_mode0:4;
   unsigned char texgen_mode1:4;
   unsigned char texgen_mode2:4;
         };
         #define TXG_NONE           0
   #define TXG_OBJ_LINEAR     1
   #define TXG_EYE_LINEAR     2
   #define TXG_SPHERE_MAP     3
   #define TXG_REFLECTION_MAP 4
   #define TXG_NORMAL_MAP     5
      static GLuint translate_texgen( GLboolean enabled, GLenum mode )
   {
      if (!enabled)
            switch (mode) {
   case GL_OBJECT_LINEAR: return TXG_OBJ_LINEAR;
   case GL_EYE_LINEAR: return TXG_EYE_LINEAR;
   case GL_SPHERE_MAP: return TXG_SPHERE_MAP;
   case GL_REFLECTION_MAP_NV: return TXG_REFLECTION_MAP;
   case GL_NORMAL_MAP_NV: return TXG_NORMAL_MAP;
   default: return TXG_NONE;
      }
      #define FDM_EYE_RADIAL    0
   #define FDM_EYE_PLANE     1
   #define FDM_EYE_PLANE_ABS 2
   #define FDM_FROM_ARRAY    3
      static GLuint translate_fog_distance_mode(GLenum source, GLenum mode)
   {
      if (source == GL_FRAGMENT_DEPTH_EXT) {
      switch (mode) {
   case GL_EYE_RADIAL_NV:
         case GL_EYE_PLANE:
         default: /* shouldn't happen; fall through to a sensible default */
   case GL_EYE_PLANE_ABSOLUTE_NV:
            } else {
            }
      static GLboolean check_active_shininess( struct gl_context *ctx,
               {
               if ((key->varying_vp_inputs & VERT_BIT_COLOR0) &&
      (key->light_color_material_mask & (1 << attr)))
         if (key->varying_vp_inputs & VERT_BIT_MAT(attr))
            if (ctx->Light.Material.Attrib[attr][0] != 0.0F)
               }
         static void make_state_key( struct gl_context *ctx, struct state_key *key )
   {
      const struct gl_program *fp = ctx->FragmentProgram._Current;
                     if (_mesa_hw_select_enabled(ctx)) {
      /* GL_SELECT mode only need position calculation.
   * glBegin/End use VERT_BIT_SELECT_RESULT_OFFSET for multi name stack in one draw.
   * glDrawArrays may also be called without user shader, fallback to FF one.
   */
   key->varying_vp_inputs = ctx->VertexProgram._VaryingInputs &
                     /* This now relies on texenvprogram.c being active:
   */
                     key->fragprog_inputs_read = fp->info.inputs_read;
            if (ctx->RenderMode == GL_FEEDBACK) {
      /* make sure the vertprog emits color and tex0 */
               if (ctx->Light.Enabled) {
               if (ctx->Light.Model.LocalViewer)
            if (ctx->Light.Model.TwoSide)
            if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
            if (ctx->Light.ColorMaterialEnabled) {
                  mask = ctx->Light._EnabledLights;
   while (mask) {
                                                            if (lu->ConstantAttenuation != 1.0F ||
      lu->LinearAttenuation != 0.0F ||
   lu->QuadraticAttenuation != 0.0F)
            if (check_active_shininess(ctx, key, 0)) {
         }
   else if (key->light_twoside &&
               }
   else {
                     if (ctx->Transform.Normalize)
            if (ctx->Transform.RescaleNormals)
            /* Only distinguish fog parameters if we actually need */
   if (key->fragprog_inputs_read & VARYING_BIT_FOGC)
      key->fog_distance_mode =
               if (ctx->Point._Attenuated)
            mask = ctx->Texture._EnabledCoordUnits | ctx->Texture._TexGenEnabled
         while (mask) {
      const int i = u_bit_scan(&mask);
   struct gl_fixedfunc_texture_unit *texUnit =
            if (ctx->Point.PointSprite)
                  if (ctx->Texture._TexMatEnabled & ENABLE_TEXMAT(i))
            if (texUnit->TexGenEnabled) {
               key->unit[i].texgen_mode0 =
      translate_texgen( texUnit->TexGenEnabled & (1<<0),
      key->unit[i].texgen_mode1 =
      translate_texgen( texUnit->TexGenEnabled & (1<<1),
      key->unit[i].texgen_mode2 =
      translate_texgen( texUnit->TexGenEnabled & (1<<2),
      key->unit[i].texgen_mode3 =
      translate_texgen( texUnit->TexGenEnabled & (1<<3),
         }
      struct tnl_program {
      const struct state_key *state;
   struct gl_program_parameter_list *state_params;
                     nir_def *eye_position;
   nir_def *eye_position_z;
   nir_def *eye_position_normalized;
            GLuint materials;
      };
      static nir_variable *
   register_state_var(struct tnl_program *p,
                     gl_state_index s0,
   gl_state_index s1,
   {
      gl_state_index16 tokens[STATE_LENGTH];
   tokens[0] = s0;
   tokens[1] = s1;
   tokens[2] = s2;
   tokens[3] = s3;
   nir_variable *var = nir_find_state_variable(p->b->shader, tokens);
   if (var)
            var = st_nir_state_variable_create(p->b->shader, type, tokens);
               }
      static nir_def *
   load_state_var(struct tnl_program *p,
                  gl_state_index s0,
   gl_state_index s1,
      {
      nir_variable *var = register_state_var(p, s0, s1, s2, s3, type);
      }
      static nir_def *
   load_state_vec4(struct tnl_program *p,
                  gl_state_index s0,
      {
         }
      static void
   load_state_mat4(struct tnl_program *p, nir_def *out[4],
         {
      for (int i = 0; i < 4; ++i)
      }
      static nir_def *
   load_input(struct tnl_program *p, gl_vert_attrib attr,
         {
      if (p->state->varying_vp_inputs & VERT_BIT(attr)) {
      nir_variable *var = nir_get_variable_with_location(p->b->shader, nir_var_shader_in,
         p->b->shader->info.inputs_read |= (uint64_t)VERT_BIT(attr);
      } else
      }
      static nir_def *
   load_input_vec4(struct tnl_program *p, gl_vert_attrib attr)
   {
         }
      static nir_variable *
   register_output(struct tnl_program *p, gl_varying_slot slot,
         {
      nir_variable *var = nir_get_variable_with_location(p->b->shader, nir_var_shader_out,
         p->b->shader->info.outputs_written |= BITFIELD64_BIT(slot);
      }
      static void
   store_output_vec4_masked(struct tnl_program *p, gl_varying_slot slot,
         {
      assert(mask <= 0xf);
   nir_variable *var = register_output(p, slot, glsl_vec4_type());
      }
      static void
   store_output_vec4(struct tnl_program *p, gl_varying_slot slot,
         {
         }
      static void
   store_output_float(struct tnl_program *p, gl_varying_slot slot,
         {
      nir_variable *var = register_output(p, slot, glsl_float_type());
      }
         static nir_def *
   emit_matrix_transform_vec4(nir_builder *b,
               {
      return nir_vec4(b,
                  nir_fdot4(b, src, mat[0]),
   }
      static nir_def *
   emit_transpose_matrix_transform_vec4(nir_builder *b,
               {
      nir_def *result;
   result = nir_fmul(b, nir_channel(b, src, 0), mat[0]);
   result = nir_fmad(b, nir_channel(b, src, 1), mat[1], result);
   result = nir_fmad(b, nir_channel(b, src, 2), mat[2], result);
   result = nir_fmad(b, nir_channel(b, src, 3), mat[3], result);
      }
      static nir_def *
   emit_matrix_transform_vec3(nir_builder *b,
               {
      return nir_vec3(b,
                  }
      static nir_def *
   emit_normalize_vec3(nir_builder *b, nir_def *src)
   {
      nir_def *tmp = nir_frsq(b, nir_fdot3(b, src, src));
      }
      static void
   emit_passthrough(struct tnl_program *p, gl_vert_attrib attr,
         {
      nir_def *val = load_input_vec4(p, attr);
      }
      static nir_def *
   get_eye_position(struct tnl_program *p)
   {
      if (!p->eye_position) {
      nir_def *pos =
         if (p->mvp_with_dp4) {
      nir_def *modelview[4];
   load_state_mat4(p, modelview, STATE_MODELVIEW_MATRIX, 0);
   p->eye_position =
      } else {
      nir_def *modelview[4];
   load_state_mat4(p, modelview,
         p->eye_position =
                     }
      static nir_def *
   get_eye_position_z(struct tnl_program *p)
   {
         }
      static nir_def *
   get_eye_position_normalized(struct tnl_program *p)
   {
      if (!p->eye_position_normalized) {
      nir_def *eye = get_eye_position(p);
                  }
      static nir_def *
   get_transformed_normal(struct tnl_program *p)
   {
      if (!p->transformed_normal &&
      !p->state->need_eye_coords &&
   !p->state->normalize &&
   !(p->state->need_eye_coords == p->state->rescale_normals)) {
   p->transformed_normal =
      load_input(p, VERT_ATTRIB_NORMAL,
   } else if (!p->transformed_normal) {
      nir_def *normal =
                  if (p->state->need_eye_coords) {
      nir_def *mvinv[4];
   load_state_mat4(p, mvinv, STATE_MODELVIEW_MATRIX_INVTRANS, 0);
               /* Normalize/Rescale:
   */
   if (p->state->normalize)
         else if (p->state->need_eye_coords == p->state->rescale_normals) {
      nir_def *scale =
      load_state_var(p, STATE_NORMAL_SCALE, 0, 0, 0,
                                 }
      static GLuint material_attrib( GLuint side, GLuint property )
   {
      switch (property) {
   case STATE_AMBIENT:
         case STATE_DIFFUSE:
         case STATE_SPECULAR:
         case STATE_EMISSION:
         case STATE_SHININESS:
         default:
            }
         /**
   * Get a bitmask of which material values vary on a per-vertex basis.
   */
   static void set_material_flags( struct tnl_program *p )
   {
      p->color_materials = 0;
            if (p->state->varying_vp_inputs & VERT_BIT_COLOR0) {
      p->materials =
               p->materials |= ((p->state->varying_vp_inputs & VERT_BIT_MAT_ALL)
      }
         static nir_def *
   get_material(struct tnl_program *p, GLuint side,
         {
               if (p->color_materials & (1<<attrib))
         else if (p->materials & (1<<attrib)) {
      /* Put material values in the GENERIC slots -- they are not used
   * for anything in fixed function mode.
   */
      } else {
            }
      #define SCENE_COLOR_BITS(side) (( MAT_BIT_FRONT_EMISSION | \
                     /**
   * Either return a precalculated constant value or emit code to
   * calculate these values dynamically in the case where material calls
   * are present between begin/end pairs.
   *
   * Probably want to shift this to the program compilation phase - if
   * we always emitted the calculation here, a smart compiler could
   * detect that it was constant (given a certain set of inputs), and
   * lift it out of the main loop.  That way the programs created here
   * would be independent of the vertex_buffer details.
   */
   static nir_def *
   get_scenecolor(struct tnl_program *p, GLuint side)
   {
      if (p->materials & SCENE_COLOR_BITS(side)) {
      nir_def *lm_ambient =
         nir_def *material_emission =
         nir_def *material_ambient =
         nir_def *material_diffuse =
            // rgb: material_emission + material_ambient * lm_ambient
   // alpha: material_diffuse.a
   return nir_vector_insert_imm(p->b, nir_fmad(p->b,
                                          }
   else
      }
      static nir_def *
   get_lightprod(struct tnl_program *p, GLuint light,
         {
      GLuint attrib = material_attrib(side, property);
   if (p->materials & (1<<attrib)) {
      *is_state_light = true;
      } else {
      *is_state_light = false;
         }
         static nir_def *
   calculate_light_attenuation(struct tnl_program *p,
                     {
      nir_def *attenuation = NULL;
            /* Calculate spot attenuation:
   */
   if (!p->state->unit[i].light_spotcutoff_is_180) {
      nir_def *spot_dir_norm =
         attenuation =
            nir_def *spot = nir_fdot3(p->b, nir_fneg(p->b, VPpli),
         nir_def *cmp = nir_flt(p->b, nir_channel(p->b, spot_dir_norm, 3),
         spot = nir_fpow(p->b, spot, nir_channel(p->b, attenuation, 3));
               /* Calculate distance attenuation(See formula (2.4) at glspec 2.1 page 62):
   *
   * Skip the calucation when _dist_ is undefined(light_eyepos3_is_zero)
   */
   if (p->state->unit[i].light_attenuated && dist) {
      if (!attenuation) {
      attenuation = load_state_vec4(p, STATE_LIGHT, i,
               /* dist is the reciprocal of ||VP|| used in the distance
   * attenuation formula. So need to get the reciprocal of dist first
   * before applying to the formula.
   */
            /* 1, d, d*d */
   nir_def *tmp = nir_vec3(p->b,
      nir_imm_float(p->b, 1.0f),
   dist,
      );
            if (!p->state->unit[i].light_spotcutoff_is_180)
                        }
      static nir_def *
   emit_lit(nir_builder *b,
         {
      nir_def *zero = nir_imm_zero(b, 1, 32);
   nir_def *one = nir_imm_float(b, 1.0f);
   nir_def *src_x = nir_channel(b, src, 0);
   nir_def *src_y = nir_channel(b, src, 1);
            nir_def *wclamp = nir_fmax(b, nir_fmin(b, src_w,
                        return nir_vec4(b,
                  one,
   nir_fmax(b, src_x, zero),
   nir_bcsel(b,
         }
      /**
   * Compute:
   *   lit.y = MAX(0, dots.x)
   *   lit.z = SLT(0, dots.x)
   */
   static nir_def *
   emit_degenerate_lit(nir_builder *b,
         {
               /* Note that lit.x & lit.w will not be examined.  Note also that
   * dots.xyzw == dots.xxxx.
            nir_def *zero = nir_imm_zero(b, 1, 32);
   nir_def *dots_x = nir_channel(b, dots, 0);
   nir_def *tmp = nir_fmax(b, id, dots);
      }
         /* Need to add some addtional parameters to allow lighting in object
   * space - STATE_SPOT_DIRECTION and STATE_HALF_VECTOR implicitly assume eye
   * space lighting.
   */
   static void build_lighting( struct tnl_program *p )
   {
      const GLboolean twoside = p->state->light_twoside;
   const GLboolean separate = p->state->separate_specular;
   GLuint nr_lights = 0;
   nir_def *lit = NULL;
   nir_def *dots = nir_imm_zero(p->b, 4, 32);
   nir_def *normal = get_transformed_normal(p);
   nir_def *_col0 = NULL, *_col1 = NULL;
   nir_def *_bfc0 = NULL, *_bfc1 = NULL;
            /*
   * NOTE:
   * dots.x = dot(normal, VPpli)
   * dots.y = dot(normal, halfAngle)
   * dots.z = back.shininess
   * dots.w = front.shininess
            for (i = 0; i < MAX_LIGHTS; i++)
      if (p->state->unit[i].light_enabled)
                  {
      if (!p->state->material_shininess_is_zero) {
      nir_def *shininess = get_material(p, 0, STATE_SHININESS);
   nir_def *tmp = nir_channel(p->b, shininess, 0);
               _col0 = get_scenecolor(p, 0);
   if (separate)
               if (twoside) {
      if (!p->state->material_shininess_is_zero) {
      /* Note that we negate the back-face specular exponent here.
   * The negation will be un-done later in the back-face code below.
   */
   nir_def *shininess = get_material(p, 1, STATE_SHININESS);
   nir_def *tmp = nir_channel(p->b, shininess, 0);
   tmp = nir_fneg(p->b, tmp);
               _bfc0 = get_scenecolor(p, 1);
   if (separate)
               /* If no lights, still need to emit the scenecolor.
   */
            if (separate)
            if (twoside)
            if (twoside && separate)
            if (nr_lights == 0)
            /* Declare light products first to place them sequentially next to each
   * other for optimal constant uploads.
   */
   nir_def *lightprod_front[MAX_LIGHTS][3];
   nir_def *lightprod_back[MAX_LIGHTS][3];
   bool lightprod_front_is_state_light[MAX_LIGHTS][3];
            for (i = 0; i < MAX_LIGHTS; i++) {
      if (p->state->unit[i].light_enabled) {
      lightprod_front[i][0] = get_lightprod(p, i, 0, STATE_AMBIENT,
         if (twoside)
                  lightprod_front[i][1] = get_lightprod(p, i, 0, STATE_DIFFUSE,
         if (twoside)
                  lightprod_front[i][2] = get_lightprod(p, i, 0, STATE_SPECULAR,
         if (twoside)
      lightprod_back[i][2] = get_lightprod(p, i, 1, STATE_SPECULAR,
               /* Add more variables now that we'll use later, so that they are nicely
   * sorted in the parameter list.
   */
   for (i = 0; i < MAX_LIGHTS; i++) {
      if (p->state->unit[i].light_enabled) {
      if (p->state->unit[i].light_eyepos3_is_zero)
      register_state_var(p, STATE_LIGHT_POSITION_NORMALIZED,
            else
      register_state_var(p, STATE_LIGHT_POSITION, i, 0, 0,
      }
   for (i = 0; i < MAX_LIGHTS; i++) {
      if (p->state->unit[i].light_enabled &&
      (!p->state->unit[i].light_spotcutoff_is_180 ||
   (p->state->unit[i].light_attenuated &&
         register_state_var(p, STATE_LIGHT, i, STATE_ATTENUATION, 0,
            for (i = 0; i < MAX_LIGHTS; i++) {
      if (p->state->unit[i].light_enabled) {
      nir_def *half = NULL;
                  if (p->state->unit[i].light_eyepos3_is_zero) {
      VPpli = load_state_var(p, STATE_LIGHT_POSITION_NORMALIZED,
            } else {
                                    /* Normalize VPpli.  The dist value also used in
   * attenuation below.
   */
   dist = nir_frsq(p->b, nir_fdot3(p->b, VPpli, VPpli));
               /* Calculate attenuation:
                  /* Calculate viewer direction, or use infinite viewer:
   */
   if (!p->state->material_shininess_is_zero) {
      if (p->state->light_local_viewer) {
      nir_def *eye_hat = get_eye_position_normalized(p);
   half = emit_normalize_vec3(p->b,
      } else if (p->state->unit[i].light_eyepos3_is_zero) {
      half =
      load_state_var(p, STATE_LIGHT_HALF_VECTOR,
         } else {
      nir_def *tmp =
      nir_fadd(p->b,
                           /* Calculate dot products:
   */
   nir_def *dot = nir_fdot3(p->b, normal, VPpli);
   if (p->state->material_shininess_is_zero) {
         } else {
      dots = nir_vector_insert_imm(p->b, dots, dot, 0);
   dot = nir_fdot3(p->b, normal, half);
               /* Front face lighting:
   */
   {
      /* Transform STATE_LIGHT into STATE_LIGHTPROD if needed. This isn't done in
   * get_lightprod to avoid using too many temps.
   */
   for (int j = 0; j < 3; j++) {
      if (lightprod_front_is_state_light[i][j]) {
      nir_def *material =
         lightprod_front[i][j] =
                  nir_def *ambient = lightprod_front[i][0];
                  if (att) {
      /* light is attenuated by distance */
   lit = emit_lit(p->b, dots);
   lit = nir_fmul(p->b, lit, att);
      } else if (!p->state->material_shininess_is_zero) {
      /* there's a non-zero specular term */
   lit = emit_lit(p->b, dots);
      } else {
      /* no attenutation, no specular */
                     _col0 = nir_fmad(p->b, nir_channel(p->b, lit, 1),
         if (separate)
      _col1 = nir_fmad(p->b, nir_channel(p->b, lit, 2),
      else
      _col0 = nir_fmad(p->b, nir_channel(p->b, lit, 2),
   }
   /* Back face lighting:
   */
   nir_def *old_dots = dots;
   if (twoside) {
      /* Transform STATE_LIGHT into STATE_LIGHTPROD if needed. This isn't done in
   * get_lightprod to avoid using too many temps.
   */
   for (int j = 0; j < 3; j++) {
      if (lightprod_back_is_state_light[i][j]) {
      nir_def *material =
         lightprod_back[i][j] =
                  nir_def *ambient = lightprod_back[i][0];
                  /* For the back face we need to negate the X and Y component
   * dot products.  dots.Z has the negated back-face specular
   * exponent.  We swizzle that into the W position.  This
   * negation makes the back-face specular term positive again.
   */
   unsigned swiz_xywz[] = {0, 1, 3, 2};
                  if (att) {
      /* light is attenuated by distance */
   lit = emit_lit(p->b, dots);
   lit = nir_fmul(p->b, lit, att);
      } else if (!p->state->material_shininess_is_zero) {
      /* there's a non-zero specular term */
   lit = emit_lit(p->b, dots);
      } else {
      /* no attenutation, no specular */
                     _bfc0 = nir_fmad(p->b, nir_channel(p->b, lit, 1),
         if (separate)
      _bfc1 = nir_fmad(p->b, nir_channel(p->b, lit, 2),
      else
      _bfc0 = nir_fmad(p->b, nir_channel(p->b, lit, 2),
                  store_output_vec4_masked(p, VARYING_SLOT_COL0, _col0, 0x7);
   if (separate)
            if (twoside) {
      store_output_vec4_masked(p, VARYING_SLOT_BFC0, _bfc0, 0x7);
   if (separate)
         }
         static void build_fog( struct tnl_program *p )
   {
      nir_def *fog;
   switch (p->state->fog_distance_mode) {
   case FDM_EYE_RADIAL:
      /* Z = sqrt(Xe*Xe + Ye*Ye + Ze*Ze) */
   fog = nir_fast_length(p->b,
            case FDM_EYE_PLANE: /* Z = Ze */
      fog = get_eye_position_z(p);
      case FDM_EYE_PLANE_ABS: /* Z = abs(Ze) */
      fog = nir_fabs(p->b, get_eye_position_z(p));
      case FDM_FROM_ARRAY:
      fog = load_input(p, VERT_ATTRIB_FOG, glsl_float_type());
      default:
      assert(!"Bad fog mode in build_fog()");
                  }
         static nir_def *
   build_reflect_texgen(struct tnl_program *p)
   {
      nir_def *normal = get_transformed_normal(p);
   nir_def *eye_hat = get_eye_position_normalized(p);
   /* n.u */
   nir_def *tmp = nir_fdot3(p->b, normal, eye_hat);
   /* 2n.u */
   tmp = nir_fadd(p->b, tmp, tmp);
   /* (-2n.u)n + u */
      }
         static nir_def *
   build_sphere_texgen(struct tnl_program *p)
   {
      nir_def *normal = get_transformed_normal(p);
            /* Could share the above calculations, but it would be
   * a fairly odd state for someone to set (both sphere and
   * reflection active for different texture coordinate
   * components.  Of course - if two texture units enable
   * reflect and/or sphere, things start to tilt in favour
   * of seperating this out:
            /* n.u */
   nir_def *tmp = nir_fdot3(p->b, normal, eye_hat);
   /* 2n.u */
   tmp = nir_fadd(p->b, tmp, tmp);
   /* (-2n.u)n + u */
   nir_def *r = nir_fmad(p->b, nir_fneg(p->b, tmp), normal, eye_hat);
   /* r + 0,0,1 */
   tmp = nir_fadd(p->b, r, nir_imm_vec4(p->b, 0.0f, 0.0f, 1.0f, 0.0f));
   /* rx^2 + ry^2 + (rz+1)^2 */
   tmp = nir_fdot3(p->b, tmp, tmp);
   /* 2/m */
   tmp = nir_frsq(p->b, tmp);
   /* 1/m */
   nir_def *inv_m = nir_fmul_imm(p->b, tmp, 0.5f);
   /* r/m + 1/2 */
      }
      static void build_texture_transform( struct tnl_program *p )
   {
                           if (!(p->state->fragprog_inputs_read & VARYING_BIT_TEX(i)))
            if (p->state->unit[i].coord_replace)
            nir_def *texcoord;
   if (p->state->unit[i].texgen_enabled) {
      GLuint copy_mask = 0;
   GLuint sphere_mask = 0;
   GLuint reflect_mask = 0;
   GLuint normal_mask = 0;
                  modes[0] = p->state->unit[i].texgen_mode0;
   modes[1] = p->state->unit[i].texgen_mode1;
                  for (j = 0; j < 4; j++) {
      switch (modes[j]) {
   case TXG_OBJ_LINEAR: {
      nir_def *obj = load_input_vec4(p, VERT_ATTRIB_POS);
   nir_def *plane =
      load_state_vec4(p, STATE_TEXGEN, i,
      comps[j] = nir_fdot4(p->b, obj, plane);
      }
   case TXG_EYE_LINEAR: {
      nir_def *eye = get_eye_position(p);
   nir_def *plane =
      load_state_vec4(p, STATE_TEXGEN, i,
      comps[j] = nir_fdot4(p->b, eye, plane);
      }
   case TXG_SPHERE_MAP:
      sphere_mask |= 1u << j;
      case TXG_REFLECTION_MAP:
      reflect_mask |= 1u << j;
      case TXG_NORMAL_MAP:
      normal_mask |= 1u << j;
      case TXG_NONE:
                     if (sphere_mask) {
      nir_def *sphere = build_sphere_texgen(p);
   for (j = 0; j < 4; j++)
                     if (reflect_mask) {
      nir_def *reflect = build_reflect_texgen(p);
   for (j = 0; j < 4; j++)
                     if (normal_mask) {
      nir_def *normal = get_transformed_normal(p);
   for (j = 0; j < 4; j++)
                     if (copy_mask) {
      nir_def *in = load_input_vec4(p, VERT_ATTRIB_TEX0 + i);
   for (j = 0; j < 4; j++)
                        } else
            if (p->state->unit[i].texmat_enabled) {
      nir_def *texmat[4];
   if (p->mvp_with_dp4) {
      load_state_mat4(p, texmat, STATE_TEXTURE_MATRIX, i);
   texcoord =
      } else {
      load_state_mat4(p, texmat,
         texcoord =
      emit_transpose_matrix_transform_vec4(p->b, texmat,
                     }
         /**
   * Point size attenuation computation.
   */
   static void build_atten_pointsize( struct tnl_program *p )
   {
      nir_def *eye = get_eye_position_z(p);
   nir_def *in_size =
         nir_def *att =
            /* dist = |eyez| */
            /* p1 + dist * (p2 + dist * p3); */
   nir_def *factor = nir_fmad(p->b, dist, nir_channel(p->b, att, 2),
                  /* 1 / sqrt(factor) */
            /* pointSize / sqrt(factor) */
   nir_def *size = nir_fmul(p->b, factor,
         #if 1
      /* this is a good place to clamp the point size since there's likely
   * no hardware registers to clamp point size at rasterization time.
   */
   size = nir_fclamp(p->b, size, nir_channel(p->b, in_size, 1),
      #endif
            }
         /**
   * Pass-though per-vertex point size, from user's point size array.
   */
   static void build_array_pointsize( struct tnl_program *p )
   {
      nir_def *val = load_input(p, VERT_ATTRIB_POINT_SIZE,
            }
         static void build_tnl_program( struct tnl_program *p )
   {
               /* Lighting calculations:
   */
   if (p->state->fragprog_inputs_read &
      (VARYING_BIT_COL0 | VARYING_BIT_COL1)) {
   if (p->state->light_global_enabled)
         else {
                     if (p->state->fragprog_inputs_read & VARYING_BIT_COL1)
                  if (p->state->fragprog_inputs_read & VARYING_BIT_FOGC)
            if (p->state->fragprog_inputs_read & VARYING_BITS_TEX_ANY)
            if (p->state->point_attenuated)
         else if (p->state->varying_vp_inputs & VERT_BIT_POINT_SIZE)
            if (p->state->varying_vp_inputs & VERT_BIT_SELECT_RESULT_OFFSET)
      emit_passthrough(p, VERT_ATTRIB_SELECT_RESULT_OFFSET,
   }
         static nir_shader *
   create_new_program( const struct state_key *key,
                     {
               memset(&p, 0, sizeof(p));
   p.state = key;
            program->Parameters = _mesa_new_parameter_list();
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_VERTEX,
                                                               /* Emit the MVP position transformation */
            _mesa_add_separate_state_parameters(program, p.state_params);
               }
         /**
   * Return a vertex program which implements the current fixed-function
   * transform/lighting/texgen operations.
   */
   struct gl_program *
   _mesa_get_fixed_func_vertex_program(struct gl_context *ctx)
   {
      struct gl_program *prog;
            /* We only update ctx->VertexProgram._VaryingInputs when in VP_MODE_FF _VPMode */
            /* Grab all the relevant state and put it in a single structure:
   */
            /* Look for an already-prepared program for this state:
   */
   prog = _mesa_search_program_cache(ctx->VertexProgram.Cache, &key,
            if (!prog) {
      /* OK, we'll have to build a new one */
   if (0)
            prog = ctx->Driver.NewProgram(ctx, MESA_SHADER_VERTEX, 0, false);
   if (!prog)
            const struct nir_shader_compiler_options *options =
            nir_shader *s =
      create_new_program( &key, prog,
               prog->state.type = PIPE_SHADER_IR_NIR;
                     _mesa_program_cache_insert(ctx, ctx->VertexProgram.Cache, &key,
                  }
